/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef NPCBOT_HAZARD_MGR_H
#define NPCBOT_HAZARD_MGR_H

#include "../botcommon.h"
#include "../../../Entities/Object/ObjectGuid.h"
#include "../../../Entities/Object/Position.h"

#include <map>
#include <unordered_map>

class Unit;

struct BotCreatureHazardRule
{
    uint32 MapId;
    uint32 CreatureEntry;
    uint32 DamageSpellId;
    float Radius;
    float SafetyDistance;
    uint32 DeactivationDelayMs;
};

struct NPCBotCreatureHazardStateKey
{
    uint32 MapId;
    uint32 InstanceId;
    ObjectGuid SourceGuid;

    bool operator<(NPCBotCreatureHazardStateKey const& other) const;
};

struct NPCBotCreatureHazardState
{
    Position LastPosition;
    float Radius;
    uint32 ExpireTimeMs;
};

using NPCBotCreatureHazardStateMap = std::map<NPCBotCreatureHazardStateKey, NPCBotCreatureHazardState>;

class NPCBotHazardMgr
{
public:
    static NPCBotHazardMgr* instance();

    void LoadFromDB();
    void CollectCreatureHazards(Unit const* unit, AoeSpotsVec& spots, NPCBotCreatureHazardStateMap& states);

private:
    using CreatureHazardRulesByEntry = std::unordered_map<uint32, BotCreatureHazardRule>;

    NPCBotHazardMgr() = default;

    BotCreatureHazardRule const* GetRule(uint32 mapId, uint32 creatureEntry) const;
    bool HasRule(uint32 mapId, uint32 creatureEntry) const;
    static float GetDamageSpellRadius(uint32 spellId);

    std::unordered_map<uint32, CreatureHazardRulesByEntry> _rulesByMap;
    CreatureHazardRulesByEntry _globalRules;
};

#define sNPCBotHazardMgr NPCBotHazardMgr::instance()

#endif
