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
    EventChainBolt = 1,
    EventPurity,
    EventRenew,
    EventManaSpike
};

enum Spells : uint32
{
    SpellChainBolt = 8292,
    SpellPurity = 8361,
    SpellRenew = 6077,
    SpellManaSpike = 8358
};
}

struct boss_rift_charlga : public BossAIBase
{
    explicit boss_rift_charlga(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventChainBolt, 3000, 2200, 1600);
        ScheduleTieredEvent(EventPurity, 10000, 8000, 6500);
        ScheduleTieredEvent(EventRenew, 12000, 9500, 7500);
        events.ScheduleEvent(EventManaSpike, 18s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventChainBolt:
                CastIfConfigured(me->GetVictim(), SpellChainBolt);
                ScheduleTieredEvent(EventChainBolt, 6500, 5000, 3800);
                break;
            case EventPurity:
                CastIfConfigured(me, SpellPurity);
                ScheduleTieredEvent(EventPurity, 18000, 14500, 11500);
                break;
            case EventRenew:
                if (me->HealthBelowPct(85))
                    CastIfConfigured(me, SpellRenew);
                ScheduleTieredEvent(EventRenew, 18000, 14500, 11500);
                break;
            case EventManaSpike:
                if (me->GetPowerPct(POWER_MANA) <= 20.0f)
                    CastIfConfigured(me, SpellManaSpike);
                events.ScheduleEvent(EventManaSpike, 15s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_charlga()
{
    RegisterCreatureAI(boss_rift_charlga);
}

} // namespace HeroicDungeonRift
