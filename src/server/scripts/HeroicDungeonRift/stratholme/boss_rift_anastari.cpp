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
    EventBansheeWail = 1, // 女妖哀嚎（T1基础）
    EventBansheeCurse,    // 女妖诅咒（T1基础）
    EventSilence,         // 沉默（T1基础）
    EventPossess,         // 占据（T2新增）
    EventTier3Skill       // 支配心灵（T3新增）
};

enum Spells : uint32
{
    SpellBansheeWail = 16565,   // 女妖哀嚎
    SpellBansheeCurse = 16867,  // 女妖诅咒
    SpellSilence = 18327,       // 沉默
    SpellPossess = 17244,       // 占据
    SpellDominateMind = 14515   // 支配心灵
};
}

struct boss_rift_anastari : public BossAIBase
{
    explicit boss_rift_anastari(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventBansheeWail, 4000, 3200, 2500);
        ScheduleTieredEvent(EventBansheeCurse, 12000, 9500, 7500);
        ScheduleTieredEvent(EventSilence, 14000, 11000, 9000);
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
                ScheduleTieredEvent(EventBansheeWail, 5000, 4000, 3200);
                break;
            case EventBansheeCurse:
                CastIfConfigured(me->GetVictim(), SpellBansheeCurse);
                ScheduleTieredEvent(EventBansheeCurse, 18000, 14500, 11500);
                break;
            case EventSilence:
                CastIfConfigured(me->GetVictim(), SpellSilence);
                ScheduleTieredEvent(EventSilence, 13000, 10500, 8500);
                break;
            case EventPossess: // T2新增：占据，点名随机目标，读条不可打断
                CastIfConfigured(SelectRandomPlayer(), SpellPossess, true);
                events.ScheduleEvent(EventPossess, _tier == 3 ? 22s : 28s);
                break;
            case EventTier3Skill: // T3新增：支配心灵，点名随机目标，读条不可打断
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
