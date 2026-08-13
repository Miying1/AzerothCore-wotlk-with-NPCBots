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
// 黑石塔下层 - 烟网蛛后（Mother Smolderweb）
enum Events : uint32
{
    EventMothersMilk = 1, // 蛛后的乳汁（T1基础）
    EventCrystallize,     // 结晶（T1基础）
    EventTier2Skill,      // 毒液喷吐（T2新增）
    EventTier3Skill       // 蛛网（T3新增）
};

enum Spells : uint32
{
    SpellMothersMilk = 16468, // 蛛后的乳汁
    SpellCrystallize = 16104, // 结晶
    SpellPoisonSpit = 15664,  // 毒液喷吐
    SpellWeb = 12023          // 蛛网
};
}

struct boss_rift_smolderweb : public BossAIBase
{
    explicit boss_rift_smolderweb(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventMothersMilk, 11000, 9000, 7200);
        ScheduleTieredEvent(EventCrystallize, 16000, 13000, 10500);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 8s);  // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 15s); // T3新增
    }

    void JustDied(Unit* /*killer*/) override
    {
        // 原版机制：死亡时召唤尖塔小蜘蛛。小蜘蛛须存活到Boss清理之后，
        // 因此直接召唤而不计入战斗召唤物跟踪。
        for (uint32 i = 0; i < _tier + 1; ++i)
            if (Creature* spiderling = me->SummonCreature(RiftEntrySpireSpiderling, me->GetRandomNearPosition(5.0f),
                TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS))
            {
                ApplySummonTierStats(spiderling, 0.5f, 0.6f);
                spiderling->SetInCombatWithZone();
            }
        BossAIBase::JustDied(nullptr);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventMothersMilk:
                CastIfConfigured(me, SpellMothersMilk);
                ScheduleTieredEvent(EventMothersMilk, 12000, 9500, 7500);
                break;
            case EventCrystallize:
                CastIfConfigured(me->GetVictim(), SpellCrystallize);
                ScheduleTieredEvent(EventCrystallize, 16000, 13000, 10500);
                break;
            case EventTier2Skill: // T2新增：毒液喷吐，顺发
                CastIfConfigured(me->GetVictim(), SpellPoisonSpit, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 9s : 12s);
                break;
            case EventTier3Skill: // T3新增：蛛网，点名随机目标，瞬发
                CastIfConfigured(SelectRandomPlayer(), SpellWeb, true);
                events.ScheduleEvent(EventTier3Skill, 18s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_smolderweb()
{
    RegisterCreatureAI(boss_rift_smolderweb);
}

} // namespace HeroicDungeonRift
