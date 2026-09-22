/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "NPCBotHazardMgr.h"

#include "CellImpl.h"
#include "Creature.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "SpellAuraDefines.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Unit.h"

#include <algorithm>
#include <list>
#include <set>

namespace
{
constexpr float CREATURE_HAZARD_SCAN_DISTANCE = 60.0f;

bool IsDamageEffect(SpellEffectInfo const& effect)
{
    switch (effect.Effect)
    {
        case SPELL_EFFECT_INSTAKILL:
        case SPELL_EFFECT_SCHOOL_DAMAGE:
        case SPELL_EFFECT_ENVIRONMENTAL_DAMAGE:
        case SPELL_EFFECT_HEALTH_LEECH:
        case SPELL_EFFECT_POWER_BURN:
            return true;
        default:
            break;
    }

    switch (effect.ApplyAuraName)
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_POWER_BURN:
            return true;
        default:
            return false;
    }
}
}

NPCBotHazardMgr* NPCBotHazardMgr::instance()
{
    static NPCBotHazardMgr instance;
    return &instance;
}

bool NPCBotCreatureHazardStateKey::operator<(NPCBotCreatureHazardStateKey const& other) const
{
    if (MapId != other.MapId)
        return MapId < other.MapId;
    if (InstanceId != other.InstanceId)
        return InstanceId < other.InstanceId;
    return SourceGuid < other.SourceGuid;
}

float NPCBotHazardMgr::GetDamageSpellRadius(uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return 0.0f;

    float radius = 0.0f;
    for (SpellEffectInfo const& effect : spellInfo->Effects)
        if (IsDamageEffect(effect) && !spellInfo->IsPositiveEffect(effect.EffectIndex))
            radius = std::max(radius, effect.CalcRadius());

    return radius;
}

void NPCBotHazardMgr::LoadFromDB()
{
    uint32 oldMSTime = getMSTime();
    uint32 loadedCount = 0;
    uint32 skippedCount = 0;
    std::unordered_map<uint32, CreatureHazardRulesByEntry> rulesByMap;
    CreatureHazardRulesByEntry globalRules;

    QueryResult result = WorldDatabase.Query(
        "SELECT map_id, creature_entry, radius, damage_spell_id, safety_distance, deactivation_delay_ms, required_aura_spell_id "
        "FROM npcbot_creature_hazard");
    if (!result)
    {
        _rulesByMap.clear();
        _globalRules.clear();
        LOG_INFO("server.loading", ">> Loaded 0 NPCBot creature hazard definitions. DB table `npcbot_creature_hazard` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 mapId = fields[0].Get<uint16>();
        uint32 creatureEntry = fields[1].Get<uint32>();
        float configuredRadius = fields[2].Get<float>();
        uint32 damageSpellId = fields[3].Get<uint32>();
        float safetyDistance = fields[4].Get<float>();
        uint32 deactivationDelayMs = fields[5].Get<uint32>();
        uint32 requiredAuraSpellId = fields[6].Get<uint32>();

        if ((mapId && !sMapStore.LookupEntry(mapId)) || !sObjectMgr->GetCreatureTemplate(creatureEntry) ||
            configuredRadius < 0.0f || safetyDistance < 0.0f)
        {
            ++skippedCount;
            LOG_ERROR("sql.sql", "Table `npcbot_creature_hazard` has invalid rule for map {} and creature {}", mapId, creatureEntry);
            continue;
        }

        float radius = configuredRadius;
        if (damageSpellId)
        {
            SpellInfo const* damageSpell = sSpellMgr->GetSpellInfo(damageSpellId);
            float spellRadius = 0.0f;
            if (!damageSpell || damageSpell->IsPositive())
            {
                // 取不到法术半径时按配置的 radius 处理，不丢弃该危险区域规则。
                LOG_WARN("sql.sql", "NPCBot creature hazard for map {} and creature {} references positive or missing damage spell {}; using configured radius {}", mapId, creatureEntry, damageSpellId, configuredRadius);
            }
            else
            {
                spellRadius = GetDamageSpellRadius(damageSpellId);
                if (spellRadius <= 0.0f)
                    LOG_WARN("sql.sql", "NPCBot creature hazard for map {} and creature {} cannot get radius from spell {}; using configured radius {}", mapId, creatureEntry, damageSpellId, configuredRadius);
            }

            // 配置的 radius 作为下限：法术半径取不到或小于配置值时用配置值。
            // 否则危险区可能小于实际伤害范围（例如火山 42052 的法术半径大于配置值），
            // 出现“BOT 站在伤害边缘却不躲”的现象。
            radius = std::max(configuredRadius, spellRadius);
        }

        if (radius <= 0.0f)
        {
            ++skippedCount;
            LOG_ERROR("sql.sql", "NPCBot creature hazard for map {} and creature {} has no valid radius", mapId, creatureEntry);
            continue;
        }

        if (requiredAuraSpellId && !sSpellMgr->GetSpellInfo(requiredAuraSpellId))
            LOG_WARN("sql.sql", "NPCBot creature hazard for map {} and creature {} references missing required aura spell {}; aura check will never match", mapId, creatureEntry, requiredAuraSpellId);

        BotCreatureHazardRule rule{ mapId, creatureEntry, damageSpellId, radius, safetyDistance, deactivationDelayMs, requiredAuraSpellId };
        if (mapId)
            rulesByMap[mapId][creatureEntry] = rule;
        else
            globalRules[creatureEntry] = rule;
        ++loadedCount;
    } while (result->NextRow());

    _rulesByMap = std::move(rulesByMap);
    _globalRules = std::move(globalRules);

    LOG_INFO("server.loading", ">> Loaded {} NPCBot creature hazard definitions in {} ms ({} skipped)", loadedCount,
        GetMSTimeDiffToNow(oldMSTime), skippedCount);
}

BotCreatureHazardRule const* NPCBotHazardMgr::GetRule(uint32 mapId, uint32 creatureEntry) const
{
    if (auto mapItr = _rulesByMap.find(mapId); mapItr != _rulesByMap.end())
        if (auto ruleItr = mapItr->second.find(creatureEntry); ruleItr != mapItr->second.end())
            return &ruleItr->second;

    if (auto ruleItr = _globalRules.find(creatureEntry); ruleItr != _globalRules.end())
        return &ruleItr->second;

    return nullptr;
}

bool NPCBotHazardMgr::HasRule(uint32 mapId, uint32 creatureEntry) const
{
    return GetRule(mapId, creatureEntry) != nullptr;
}

void NPCBotHazardMgr::CollectCreatureHazards(Unit const* unit, AoeSpotsVec& spots,
    NPCBotCreatureHazardStateMap& states)
{
    if (!unit || !unit->IsInWorld())
        return;

    if (_globalRules.empty() && _rulesByMap.find(unit->GetMapId()) == _rulesByMap.end())
    {
        states.clear();
        return;
    }

    uint32 mapId = unit->GetMapId();
    uint32 instanceId = unit->GetMap()->GetInstanceId();
    uint32 now = GameTime::GetGameTimeMS().count();
    std::set<ObjectGuid> seenGuids;
    std::list<Creature*> creatures;

    auto check = [this, unit, mapId](Creature const* creature)
    {
        if (!creature || !creature->IsInWorld() || !creature->IsAlive() || !unit->InSamePhase(creature) ||
            !unit->IsWithinDistInMap(creature, CREATURE_HAZARD_SCAN_DISTANCE))
            return false;

        BotCreatureHazardRule const* rule = GetRule(mapId, creature->GetEntry());
        if (!rule)
            return false;

        // 配置了 required_aura_spell_id 时，要求生物身上存在该技能光环才视为危险源
        if (rule->RequiredAuraSpellId && !creature->HasAura(rule->RequiredAuraSpellId))
            return false;

        return true;
    };
    Bcore::CreatureListSearcher<decltype(check)> searcher(unit, creatures, check);
    Cell::VisitObjects(unit, searcher, CREATURE_HAZARD_SCAN_DISTANCE);

    float combatReach = unit->GetVehicle() ? unit->GetVehicleBase()->GetCombatReach() : unit->GetCombatReach();
    for (Creature const* creature : creatures)
    {
        BotCreatureHazardRule const* rule = GetRule(mapId, creature->GetEntry());
        if (!rule)
            continue;

        float finalRadius = rule->Radius + rule->SafetyDistance + combatReach * 1.2f;
        NPCBotCreatureHazardStateKey key{ mapId, instanceId, creature->GetGUID() };
        states[key] = { creature->GetPosition(), finalRadius, now + rule->DeactivationDelayMs };
        seenGuids.insert(creature->GetGUID());
        spots.emplace_back(creature->GetPosition(), finalRadius);
    }

    for (auto itr = states.begin(); itr != states.end();)
    {
        NPCBotCreatureHazardStateKey const& key = itr->first;
        NPCBotCreatureHazardState const& state = itr->second;
        if (key.MapId != mapId || key.InstanceId != instanceId)
        {
            itr = states.erase(itr);
            continue;
        }

        if (seenGuids.contains(key.SourceGuid))
        {
            ++itr;
            continue;
        }

        if (now < state.ExpireTimeMs)
        {
            spots.emplace_back(state.LastPosition, state.Radius);
            ++itr;
        }
        else
            itr = states.erase(itr);
    }
}
