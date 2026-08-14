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
    EventBoulder = 1,      // 投石（原版/T1基础）
    EventDustField,        // 灰尘力场（原版/T1基础）
    EventRepulsiveGaze,    // 憎恨怒视（T2新增）
    EventTier3Skill        // 击退（T3新增）
};

enum Spells : uint32
{
    SpellThrash = 8876,         // 痛击（原版/T1基础）
    SpellBoulder = 21832,       // 投石（原版/T1基础）
    SpellDustField = 21909,     // 灰尘力场（原版/T1基础）
    SpellRepulsiveGaze = 21869, // 憎恨怒视（T2新增）
    SpellKnockAway = 10101      // 击退（T3新增）
};
}

struct boss_rift_theradras : public BossAIBase
{
    explicit boss_rift_theradras(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        CastIfConfigured(me, SpellThrash, true);
        events.ScheduleEvent(EventBoulder, 5000ms);
        events.ScheduleEvent(EventDustField, 15000ms);
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
                events.ScheduleEvent(EventBoulder, 17000ms);
                break;
            case EventDustField:
                CastIfConfigured(me, SpellDustField);
                events.ScheduleEvent(EventDustField, 30000ms);
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
