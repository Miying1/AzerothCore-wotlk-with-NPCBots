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
// 玛拉顿 - 瑟莱德丝公主（Princess Theradras）
enum Events : uint32
{
    EventBoulder = 1,      // 投石（T1基础）
    EventDustField,        // 灰尘力场（T1基础）
    EventRepulsiveGaze,    // 憎恨怒视（T2新增）
    EventTier3Skill        // 击退（T3新增）
};

enum Spells : uint32
{
    SpellThrash = 8876,         // 痛击
    SpellBoulder = 21832,       // 投石
    SpellDustField = 21909,     // 灰尘力场
    SpellRepulsiveGaze = 21869, // 憎恨怒视
    SpellKnockAway = 10101      // 击退
};
}

struct boss_rift_theradras : public BossAIBase
{
    explicit boss_rift_theradras(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        CastIfConfigured(me, SpellThrash, true);
        ScheduleTieredEvent(EventBoulder, 5000, 4000, 3200);
        ScheduleTieredEvent(EventDustField, 15000, 12000, 9500);
        if (_tier >= 2)
            events.ScheduleEvent(EventRepulsiveGaze, 10s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 18s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventBoulder:
                CastIfConfigured(SelectRandomPlayer(), SpellBoulder);
                ScheduleTieredEvent(EventBoulder, 17000, 14000, 11000);
                break;
            case EventDustField:
                CastIfConfigured(me, SpellDustField);
                ScheduleTieredEvent(EventDustField, 30000, 24000, 19000);
                break;
            case EventRepulsiveGaze: // T2新增：憎恨怒视，瞬发
                CastIfConfigured(me, SpellRepulsiveGaze, true);
                events.ScheduleEvent(EventRepulsiveGaze, _tier == 3 ? 16s : 20s);
                break;
            case EventTier3Skill: // T3新增：击退，瞬发
                CastIfConfigured(me->GetVictim(), SpellKnockAway, true);
                events.ScheduleEvent(EventTier3Skill, 14s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_theradras()
{
    RegisterCreatureAI(boss_rift_theradras);
}

} // namespace HeroicDungeonRift
