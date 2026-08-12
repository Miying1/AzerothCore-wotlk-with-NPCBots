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
    EventMegavolt,
    EventShock
};

enum Spells : uint32
{
    SpellShock = 11084,
    SpellMegavolt = 11082,
    SpellChainBolt = 11085
};
}

struct boss_rift_electrocutioner : public BossAIBase
{
    explicit boss_rift_electrocutioner(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventChainBolt, 3s);
        events.ScheduleEvent(EventMegavolt, 10s);
        events.ScheduleEvent(EventShock, 17s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventChainBolt:
                CastIfConfigured(me->GetVictim(), SpellChainBolt);
                ScheduleTieredEvent(EventChainBolt, 21000, 18000, 15000);
                break;
            case EventMegavolt:
                CastIfConfigured(me->GetVictim(), SpellMegavolt);
                ScheduleTieredEvent(EventMegavolt, 21000, 18000, 15000);
                break;
            case EventShock:
                CastIfConfigured(SelectRandomPlayer(20.0f), SpellShock);
                ScheduleTieredEvent(EventShock, 21000, 18000, 15000);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_electrocutioner()
{
    RegisterCreatureAI(boss_rift_electrocutioner);
}

} // namespace HeroicDungeonRift
