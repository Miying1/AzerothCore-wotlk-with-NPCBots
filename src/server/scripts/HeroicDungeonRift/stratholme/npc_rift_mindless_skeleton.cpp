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
// 斯坦索姆 - 无脑骷髅（Mindless Skeleton，瑞文戴尔召唤）
struct npc_rift_mindless_skeleton : public RiftSummonAI
{
    explicit npc_rift_mindless_skeleton(Creature* creature) : RiftSummonAI(creature) { }
};
}

void AddSC_npc_rift_mindless_skeleton()
{
    RegisterCreatureAI(npc_rift_mindless_skeleton);
}

} // namespace HeroicDungeonRift
