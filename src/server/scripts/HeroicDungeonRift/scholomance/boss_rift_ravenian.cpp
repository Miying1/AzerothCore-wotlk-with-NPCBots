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
    EventCleave = 1,   // 顺劈斩（T1基础）
    EventTrample,      // 践踏（T1基础）
    EventKnockAway,    // 击退（T1基础）
    EventTier2Skill,   // 致死打击（T2新增）
    EventTier3Skill    // 破甲顺劈（T3新增）
};

enum Spells : uint32
{
    SpellCleave = 20691,        // 顺劈斩
    SpellTrample = 15550,       // 践踏
    SpellKnockAway = 10101,     // 击退
    SpellMortalStrike = 15708,  // 致死打击
    SpellSunderingCleave = 25174 // 破甲顺劈
};
}

struct boss_rift_ravenian : public BossAIBase
{
    explicit boss_rift_ravenian(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventCleave, 6000, 4800, 3800);
        ScheduleTieredEvent(EventTrample, 10000, 8000, 6500);
        ScheduleTieredEvent(EventKnockAway, 12000, 9500, 7500);
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
                ScheduleTieredEvent(EventCleave, 8000, 6500, 5200);
                break;
            case EventTrample:
                CastIfConfigured(me, SpellTrample);
                ScheduleTieredEvent(EventTrample, 12000, 9500, 7500);
                break;
            case EventKnockAway:
                CastIfConfigured(me->GetVictim(), SpellKnockAway);
                ScheduleTieredEvent(EventKnockAway, 14000, 11000, 9000);
                break;
            case EventTier2Skill: // T2新增：致死打击，读条不可打断
                CastIfConfigured(me->GetVictim(), SpellMortalStrike, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 10s : 12s);
                break;
            case EventTier3Skill: // T3新增：破甲顺劈，读条不可打断
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
