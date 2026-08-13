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
// 厄运之槌东区 - 奥兹恩的仆从（Alzzin's Minion，血量<50%召唤）
struct npc_rift_alzzin_minion : public RiftSummonAI
{
    explicit npc_rift_alzzin_minion(Creature* creature) : RiftSummonAI(creature) { }
};
}

void AddSC_npc_rift_alzzin_minion()
{
    RegisterCreatureAI(npc_rift_alzzin_minion);
}

} // namespace HeroicDungeonRift
