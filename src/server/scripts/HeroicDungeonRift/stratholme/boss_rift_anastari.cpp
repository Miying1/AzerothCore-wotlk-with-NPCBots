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
// 斯坦索姆 - 安娜丝塔丽男爵夫人（Baroness Anastari）
enum Events : uint32
{
    EventBansheeWail = 1, // 女妖哀嚎（Spell 16565，T1原版）
    EventBansheeCurse,    // 女妖诅咒（Spell 16867，T1原版）
    EventSilence,         // 沉默（Spell 18327，T1原版）
    EventPossess,         // 占据（Spell 17244，T2新增）
    EventTier3Skill       // 支配心灵（Spell 14515，T3新增）
};

enum Spells : uint32
{
    SpellBansheeWail = 16565,   // 女妖哀嚎（T1原版）
    SpellBansheeCurse = 16867,  // 女妖诅咒（T1原版）
    SpellSilence = 18327,       // 沉默（T1原版）
    SpellPossess = 17244,       // 占据（T2新增）
    SpellDominateMind = 14515   // 支配心灵（T3新增）
};
}

struct boss_rift_anastari : public BossAIBase
{
    explicit boss_rift_anastari(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventBansheeWail, Milliseconds(4000));
        events.ScheduleEvent(EventBansheeCurse, Milliseconds(12000));
        events.ScheduleEvent(EventSilence, Milliseconds(14000));
        if (_tier >= 2)
            events.ScheduleEvent(EventPossess, 20s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 16s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventBansheeWail:
                CastIfConfigured(me->GetVictim(), SpellBansheeWail);
                events.ScheduleEvent(EventBansheeWail, Milliseconds(5000));
                break;
            case EventBansheeCurse:
                CastIfConfigured(me->GetVictim(), SpellBansheeCurse);
                events.ScheduleEvent(EventBansheeCurse, Milliseconds(18000));
                break;
            case EventSilence:
                CastIfConfigured(me->GetVictim(), SpellSilence);
                events.ScheduleEvent(EventSilence, Milliseconds(13000));
                break;
            case EventPossess: // T2新增：占据，点名随机目标，瞬发
                CastIfConfigured(SelectRandomPlayer(), SpellPossess, true);
                events.ScheduleEvent(EventPossess, _tier == 3 ? 22s : 28s);
                break;
            case EventTier3Skill: // T3新增：支配心灵，点名随机目标，瞬发
                CastIfConfigured(SelectRandomPlayer(), SpellDominateMind, true);
                events.ScheduleEvent(EventTier3Skill, 24s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_anastari()
{
    RegisterCreatureAI(boss_rift_anastari);
}

} // namespace HeroicDungeonRift
