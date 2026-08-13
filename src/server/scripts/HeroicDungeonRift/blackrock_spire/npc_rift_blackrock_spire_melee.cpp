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
// 黑石塔下层 - 简单近战增援（维姆萨拉克/烟网蛛后的召唤物）
struct RiftBlackrockSpireMeleeAI : public RiftSummonAI
{
    explicit RiftBlackrockSpireMeleeAI(Creature* creature) : RiftSummonAI(creature) { }
};

struct npc_rift_spirestone_warlord : public RiftBlackrockSpireMeleeAI // 裂隙尖石军阀
{
    explicit npc_rift_spirestone_warlord(Creature* creature) : RiftBlackrockSpireMeleeAI(creature) { }
};

struct npc_rift_smolderthorn_berserker : public RiftBlackrockSpireMeleeAI // 裂隙燃棘狂战士
{
    explicit npc_rift_smolderthorn_berserker(Creature* creature) : RiftBlackrockSpireMeleeAI(creature) { }
};

struct npc_rift_spire_spiderling : public RiftBlackrockSpireMeleeAI // 裂隙尖塔小蜘蛛
{
    explicit npc_rift_spire_spiderling(Creature* creature) : RiftBlackrockSpireMeleeAI(creature) { }
};
}

void AddSC_boss_rift_smolderweb();
void AddSC_boss_rift_wyrmthalak();

void AddSC_npc_rift_spirestone_warlord()
{
    RegisterCreatureAI(npc_rift_spirestone_warlord);
}

void AddSC_npc_rift_smolderthorn_berserker()
{
    RegisterCreatureAI(npc_rift_smolderthorn_berserker);
}

void AddSC_npc_rift_spire_spiderling()
{
    RegisterCreatureAI(npc_rift_spire_spiderling);
}

} // namespace HeroicDungeonRift
