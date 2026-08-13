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
// 影牙城堡 - 阿鲁高的虚空行者（Arugal's Voidwalker，T2召唤物）
struct npc_rift_arugal_voidwalker : public RiftSummonAI
{
    explicit npc_rift_arugal_voidwalker(Creature* creature) : RiftSummonAI(creature) { }

    void JustDied(Unit* /*killer*/) override
    {
        me->DespawnOrUnsummon(1s);
    }
};
} // namespace

void AddSC_npc_rift_arugal_voidwalker()
{
    RegisterCreatureAI(npc_rift_arugal_voidwalker);
}

} // namespace HeroicDungeonRift
