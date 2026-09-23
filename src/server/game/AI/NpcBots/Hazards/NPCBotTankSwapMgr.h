/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef NPCBOT_TANK_SWAP_MGR_H
#define NPCBOT_TANK_SWAP_MGR_H

#include "../botcommon.h"

#include <unordered_map>
#include <vector>

// 单条坦克换嘲规则：BOSS 目标身上的换嘲 Aura 达到层数后，由攻击该 BOSS 的另一名坦克嘲讽
struct BotTankSwapRule
{
    uint32 MapId;
    uint32 BossEntry;
    uint32 SpellId;
    uint8 AuraStacks;
};

class NPCBotTankSwapMgr
{
public:
    static NPCBotTankSwapMgr* instance();

    void LoadFromDB();

    // 返回指定地图下该 BOSS 的全部换嘲规则（无配置返回 nullptr）
    std::vector<BotTankSwapRule> const* GetRules(uint32 mapId, uint32 bossEntry) const;
    bool HasRule(uint32 mapId, uint32 bossEntry) const;

private:
    // key: bossEntry -> 该 BOSS 的换嘲规则列表（允许一个 BOSS 配置多个换嘲 Aura）
    using TankSwapRulesByBoss = std::unordered_map<uint32, std::vector<BotTankSwapRule>>;

    NPCBotTankSwapMgr() = default;

    std::unordered_map<uint32, TankSwapRulesByBoss> _rulesByMap; // key: mapId
    TankSwapRulesByBoss _globalRules;                            // mapId == 0 的全局配置
};

#define sNPCBotTankSwapMgr NPCBotTankSwapMgr::instance()

#endif
