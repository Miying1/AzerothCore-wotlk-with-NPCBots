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
enum Events : uint32
{
    EventEnrage = 1, // 恐吓（变量名沿用历史命名）：原版/T1基础
    EventSlow, // 干扰之痛（原版/T1基础）
    EventTier2Skill, // 雷霆一击（T2新增；T3沿用并缩短循环间隔）
    EventTier3Skill // 击退（T3新增）
};

enum Spells : uint32
{
    SpellEnrage = 7399, // 恐吓（变量名沿用历史命名）：经DBC确认，原版/T1基础
    SpellSlow = 3603, // 干扰之痛（原版/T1基础）：对当前目标施放
    SpellThunderclap = 15588, // 雷霆一击（T2新增）：直接伤害由调用处校准
    SpellKnockAway = 10101 // 击退（T3新增）：使用裂隙伤害校准施放
};

// 裂隙伤害校准：雷霆一击以3500点作为T1基准直接伤害，不改变其余DBC效果。
constexpr int32 ThunderclapTier1DirectDamage = 3500;
// 裂隙伤害校准：击退以2000点覆盖基础点0，后续仍统一应用Tier伤害倍率。
constexpr int32 KnockAwayRaidAdditionalDamage = 2000;
}

struct boss_rift_sneed : public BossAIBase
{
    explicit boss_rift_sneed(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventEnrage, 15s);
        events.ScheduleEvent(EventSlow, Milliseconds(9000));
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 13s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 19s);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventEnrage:
                CastIfConfigured(me, SpellEnrage);
                events.ScheduleEvent(EventEnrage, 30s);
                break;
            case EventSlow:
                CastIfConfigured(me->GetVictim(), SpellSlow);
                events.ScheduleEvent(EventSlow, Milliseconds(9000));
                break;
            case EventTier2Skill:
                CastFinalRaidDamageSpell(me, SpellThunderclap, SPELLVALUE_BASE_POINT0, ThunderclapTier1DirectDamage);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 12s : 15s);
                break;
            case EventTier3Skill:
                CastRaidTunedSpell(me->GetVictim(), SpellKnockAway, KnockAwayRaidAdditionalDamage);
                events.ScheduleEvent(EventTier3Skill, 18s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_sneed()
{
    RegisterCreatureAI(boss_rift_sneed);
}

} // namespace HeroicDungeonRift
