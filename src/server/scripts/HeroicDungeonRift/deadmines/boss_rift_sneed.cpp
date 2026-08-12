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
    EventEnrage = 1,
    EventSlow,
    EventTier2Skill,
    EventTier3Skill
};

enum Spells : uint32
{
    SpellEnrage = 7399,
    SpellSlow = 3603,
    SpellThunderclap = 15588,
    SpellKnockAway = 10101
};

constexpr int32 ThunderclapRaidDamage = 2500;
constexpr int32 KnockAwayRaidAdditionalDamage = 2000;
}

struct boss_rift_sneed : public BossAIBase
{
    explicit boss_rift_sneed(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventEnrage, 15s);
        ScheduleTieredEvent(EventSlow, 9000, 7000, 5500);
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
                events.ScheduleEvent(EventEnrage, _tier == 3 ? 22s : 30s);
                break;
            case EventSlow:
                CastIfConfigured(me->GetVictim(), SpellSlow);
                ScheduleTieredEvent(EventSlow, 9000, 7000, 5500);
                break;
            case EventTier2Skill:
                CastRaidTunedSpell(me, SpellThunderclap, ThunderclapRaidDamage);
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
