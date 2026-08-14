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
    EventMothersMilk = 1, // 蛛后的乳汁（原版/T1基础）
    EventCrystallize,     // 结晶（原版/T1基础）
    EventTier2Skill,      // 毒液喷吐（T2新增）
    EventTier3Skill       // 蛛网（T3新增）
};

enum Spells : uint32
{
    SpellMothersMilk = 16468, // 蛛后的乳汁（原版/T1基础）
    SpellCrystallize = 16104, // 结晶（原版/T1基础）
    SpellPoisonSpit = 15664,  // 毒液喷吐（T2新增，混合BP0直伤/BP1周期）
    SpellWeb = 12023          // 蛛网（T3新增）
};

constexpr int32 PoisonSpitTier1DirectDamage = 3500;
constexpr int32 PoisonSpitTier1DamagePerTick = 1800;
}

struct boss_rift_smolderweb : public BossAIBase
{
    explicit boss_rift_smolderweb(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventMothersMilk, 11000ms);
        events.ScheduleEvent(EventCrystallize, 16000ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 8s);  // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 15s); // T3新增
    }

    void JustDied(Unit* /*killer*/) override
    {
        // 原版/T1死亡阶段召唤：尖塔小蜘蛛数量随Tier增加。小蜘蛛须存活到Boss清理之后，
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
                events.ScheduleEvent(EventMothersMilk, 12000ms);
                break;
            case EventCrystallize:
                CastIfConfigured(me->GetVictim(), SpellCrystallize);
                events.ScheduleEvent(EventCrystallize, 16000ms);
                break;
            case EventTier2Skill: // T2新增：毒液喷吐，混合BP0直伤/BP1周期，瞬发
                CastFinalRaidDamageSpell(me->GetVictim(), SpellPoisonSpit,
                    PoisonSpitTier1DirectDamage, PoisonSpitTier1DamagePerTick, true);
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
