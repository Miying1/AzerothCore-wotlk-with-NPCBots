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
// 通灵学院 - 复生的守卫（Risen Guardian，加丁召唤）
struct npc_rift_risen_guardian : public RiftSummonAI
{
    explicit npc_rift_risen_guardian(Creature* creature) : RiftSummonAI(creature) { }
};
}

void AddSC_npc_rift_risen_guardian()
{
    RegisterCreatureAI(npc_rift_risen_guardian);
}

} // namespace HeroicDungeonRift
