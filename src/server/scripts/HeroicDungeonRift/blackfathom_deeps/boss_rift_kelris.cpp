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
    EventSleep = 1,
    EventMindBlast
};

enum Spells : uint32
{
    SpellResetVisual = 8734,
    SpellSleep = 8399,
    SpellMindBlast = 15587
};
}

struct boss_rift_kelris : public BossAIBase
{
    explicit boss_rift_kelris(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        if (_tierConfig)
            CastIfConfigured(me, SpellResetVisual, true);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventSleep, 12000, 9000, 7000);
        ScheduleTieredEvent(EventMindBlast, 7000, 5500, 4000);
    }

    void JustDied(Unit* /*killer*/) override
    {
        DespawnRiftSummons();
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventSleep:
                CastIfConfigured(SelectRandomPlayer(), SpellSleep);
                ScheduleTieredEvent(EventSleep, 18000, 14000, 11000);
                break;
            case EventMindBlast:
                CastIfConfigured(me->GetVictim(), SpellMindBlast);
                ScheduleTieredEvent(EventMindBlast, 10000, 7500, 5500);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_kelris()
{
    RegisterCreatureAI(boss_rift_kelris);
}

} // namespace HeroicDungeonRift
