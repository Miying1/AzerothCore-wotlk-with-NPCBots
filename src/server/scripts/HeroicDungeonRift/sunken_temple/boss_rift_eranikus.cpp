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
    EventWarStomp = 1,
    EventDeepSlumber,
    EventAcidBreath
};

enum Spells : uint32
{
    SpellPassiveVisual = 12535,
    SpellThrashAura = 8876,
    SpellWarStomp = 11876,
    SpellDeepSlumber = 12890,
    SpellAcidBreath = 12884
};
}

struct boss_rift_eranikus : public BossAIBase
{
    explicit boss_rift_eranikus(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        if (_tierConfig)
        {
            CastIfConfigured(me, SpellPassiveVisual, true);
            CastIfConfigured(me, SpellThrashAura, true);
        }
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventWarStomp, 17000, 13500, 10500);
        ScheduleTieredEvent(EventDeepSlumber, 10000, 8000, 6500);
        ScheduleTieredEvent(EventAcidBreath, 5000, 4000, 3200);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventWarStomp:
                CastIfConfigured(me, SpellWarStomp);
                ScheduleTieredEvent(EventWarStomp, 25000, 20000, 16000);
                break;
            case EventDeepSlumber:
                CastIfConfigured(SelectRandomPlayer(30.0f), SpellDeepSlumber);
                ScheduleTieredEvent(EventDeepSlumber, 23000, 18000, 14500);
                break;
            case EventAcidBreath:
                CastIfConfigured(me->GetVictim(), SpellAcidBreath);
                ScheduleTieredEvent(EventAcidBreath, 13000, 10500, 8000);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_eranikus()
{
    RegisterCreatureAI(boss_rift_eranikus);
}

} // namespace HeroicDungeonRift
