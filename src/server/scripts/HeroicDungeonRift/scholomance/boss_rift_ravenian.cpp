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
// 通灵学院 - 拉文尼亚（The Ravenian）
enum Events : uint32
{
    EventCleave = 1,   // 顺劈斩（Spell 20691，T1原版）
    EventTrample,      // 践踏（Spell 15550，T1原版）
    EventKnockAway,    // 击退（Spell 10101，T1原版）
    EventTier2Skill,   // 致死打击（Spell 15708，T2新增）
    EventTier3Skill    // 破甲顺劈（Spell 25174，T3新增）
};

enum Spells : uint32
{
    SpellCleave = 20691,        // 顺劈斩（T1原版）
    SpellTrample = 15550,       // 践踏（T1原版）
    SpellKnockAway = 10101,     // 击退（T1原版）
    SpellMortalStrike = 15708,  // 致死打击（T2新增）
    SpellSunderingCleave = 25174 // 破甲顺劈（T3新增）
};
}

struct boss_rift_ravenian : public BossAIBase
{
    explicit boss_rift_ravenian(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventCleave, Milliseconds(6000));
        events.ScheduleEvent(EventTrample, Milliseconds(10000));
        events.ScheduleEvent(EventKnockAway, Milliseconds(12000));
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 9s);  // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 14s); // T3新增
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventCleave:
                CastIfConfigured(me->GetVictim(), SpellCleave);
                events.ScheduleEvent(EventCleave, Milliseconds(8000));
                break;
            case EventTrample:
                CastIfConfigured(me, SpellTrample);
                events.ScheduleEvent(EventTrample, Milliseconds(12000));
                break;
            case EventKnockAway:
                CastIfConfigured(me->GetVictim(), SpellKnockAway);
                events.ScheduleEvent(EventKnockAway, Milliseconds(14000));
                break;
            case EventTier2Skill: // T2新增：致死打击，瞬发
                CastIfConfigured(me->GetVictim(), SpellMortalStrike, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 10s : 12s);
                break;
            case EventTier3Skill: // T3新增：破甲顺劈，瞬发
                CastIfConfigured(me->GetVictim(), SpellSunderingCleave, true);
                events.ScheduleEvent(EventTier3Skill, 16s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_ravenian()
{
    RegisterCreatureAI(boss_rift_ravenian);
}

} // namespace HeroicDungeonRift
