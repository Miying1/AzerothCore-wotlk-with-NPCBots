/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "ScriptMgr.h"

namespace HeroicDungeonRift
{
namespace
{
// 黑石深渊 - 简单近战召唤物
struct npc_rift_voidwalker_minion : public RiftSummonAI // 裂隙虚空行者仆从（七贤召唤）
{
    explicit npc_rift_voidwalker_minion(Creature* creature) : RiftSummonAI(creature) { }
};

struct npc_rift_baelgar_spawn : public RiftSummonAI // 裂隙贝尔加幼体（贝尔加召唤）
{
    explicit npc_rift_baelgar_spawn(Creature* creature) : RiftSummonAI(creature) { }
};
}

void AddSC_npc_rift_voidwalker_minion()
{
    RegisterCreatureAI(npc_rift_voidwalker_minion);
}

void AddSC_npc_rift_baelgar_spawn()
{
    RegisterCreatureAI(npc_rift_baelgar_spawn);
}

} // namespace HeroicDungeonRift
