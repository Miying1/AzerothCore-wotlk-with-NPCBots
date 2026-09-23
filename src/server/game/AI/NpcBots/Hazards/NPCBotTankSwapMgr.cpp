/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "NPCBotTankSwapMgr.h"

#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "QueryResult.h" // 需要 ResultSet 完整定义才能调用 Fetch()
#include "SpellMgr.h"
#include "Timer.h"

NPCBotTankSwapMgr* NPCBotTankSwapMgr::instance()
{
    static NPCBotTankSwapMgr instance;
    return &instance;
}

void NPCBotTankSwapMgr::LoadFromDB()
{
    uint32 oldMSTime = getMSTime();
    uint32 loadedCount = 0;
    uint32 skippedCount = 0;
    std::unordered_map<uint32, TankSwapRulesByBoss> rulesByMap;
    TankSwapRulesByBoss globalRules;

    QueryResult result = WorldDatabase.Query(
        "SELECT map_id, boss_entry, spell_id, aura_stacks, enabled FROM npcbot_tank_swap");
    if (!result)
    {
        _rulesByMap.clear();
        _globalRules.clear();
        LOG_INFO("server.loading", ">> Loaded 0 NPCBot tank swap definitions. DB table `npcbot_tank_swap` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 mapId = fields[0].Get<uint16>();
        uint32 bossEntry = fields[1].Get<uint32>();
        uint32 spellId = fields[2].Get<uint32>();
        uint8 auraStacks = fields[3].Get<uint8>();
        bool enabled = fields[4].Get<uint8>() != 0;

        // enabled = 0 的配置不参与运行时处理
        if (!enabled)
            continue;

        // 校验：地图有效 + BOSS Entry 存在
        if ((mapId && !sMapStore.LookupEntry(mapId)) || !sObjectMgr->GetCreatureTemplate(bossEntry))
        {
            ++skippedCount;
            LOG_ERROR("sql.sql", "Table `npcbot_tank_swap` has invalid rule for map {} and boss {}", mapId, bossEntry);
            continue;
        }

        // 校验：换嘲 Aura 必须存在
        if (!sSpellMgr->GetSpellInfo(spellId))
        {
            ++skippedCount;
            LOG_ERROR("sql.sql", "Table `npcbot_tank_swap` references missing spell {} for map {} and boss {}", spellId, mapId, bossEntry);
            continue;
        }

        // 校验：层数必须大于 0
        if (auraStacks == 0)
        {
            ++skippedCount;
            LOG_ERROR("sql.sql", "Table `npcbot_tank_swap` has invalid aura_stacks for map {} and boss {}", mapId, bossEntry);
            continue;
        }

        BotTankSwapRule rule{ mapId, bossEntry, spellId, auraStacks };
        if (mapId)
            rulesByMap[mapId][bossEntry].push_back(rule);
        else
            globalRules[bossEntry].push_back(rule);
        ++loadedCount;
    } while (result->NextRow());

    _rulesByMap = std::move(rulesByMap);
    _globalRules = std::move(globalRules);

    LOG_INFO("server.loading", ">> Loaded {} NPCBot tank swap definitions in {} ms ({} skipped)",
        loadedCount, GetMSTimeDiffToNow(oldMSTime), skippedCount);
}

std::vector<BotTankSwapRule> const* NPCBotTankSwapMgr::GetRules(uint32 mapId, uint32 bossEntry) const
{
    // 优先地图专用配置
    if (auto mapItr = _rulesByMap.find(mapId); mapItr != _rulesByMap.end())
        if (auto bossItr = mapItr->second.find(bossEntry); bossItr != mapItr->second.end())
            return &bossItr->second;

    // 回退到全局配置
    if (auto bossItr = _globalRules.find(bossEntry); bossItr != _globalRules.end())
        return &bossItr->second;

    return nullptr;
}

bool NPCBotTankSwapMgr::HasRule(uint32 mapId, uint32 bossEntry) const
{
    return GetRules(mapId, bossEntry) != nullptr;
}
