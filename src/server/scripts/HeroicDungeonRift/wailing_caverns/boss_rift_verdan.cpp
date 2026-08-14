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
// 哀嚎洞穴 - 永生者沃尔丹（Verdan the Everliving）
enum Events : uint32
{
    EventGraspingVines = 1, // 缠绕之藤（原版/T1基础）
    EventThrash,            // 痛击（T2新增）
    EventEntanglingRoots    // 纠缠根须（T3新增）
};

enum Spells : uint32
{
    SpellGraspingVines = 8142,  // 缠绕之藤（原版/T1基础）
    SpellThrash = 3391,         // 痛击（T2新增）
    SpellEntanglingRoots = 12747 // 纠缠根须（T3新增）
};

constexpr int32 EntanglingRootsTier1DamagePerTick = 1800;
}

struct boss_rift_verdan : public BossAIBase
{
    explicit boss_rift_verdan(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventGraspingVines, Milliseconds(8000));
        if (_tier >= 2)
            events.ScheduleEvent(EventThrash, 6s);
        if (_tier >= 3)
            events.ScheduleEvent(EventEntanglingRoots, 12s);
    }

    void ConfigureTier() override { AddInterruptImmuneSpell(SpellEntanglingRoots); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventGraspingVines:
                CastIfConfigured(me, SpellGraspingVines);
                events.ScheduleEvent(EventGraspingVines, Milliseconds(12000));
                break;
            case EventThrash: // T2新增：痛击，瞬发
                CastIfConfigured(me, SpellThrash, true);
                events.ScheduleEvent(EventThrash, _tier == 3 ? 12s : 16s);
                break;
            case EventEntanglingRoots: // T3新增：纠缠根须，点名随机目标，免疫打断
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellEntanglingRoots, SPELLVALUE_BASE_POINT1,
                    EntanglingRootsTier1DamagePerTick);
                events.ScheduleEvent(EventEntanglingRoots, 20s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_verdan()
{
    RegisterCreatureAI(boss_rift_verdan);
}

} // namespace HeroicDungeonRift
