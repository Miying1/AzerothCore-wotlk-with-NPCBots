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
    EventDiseasePulse = 1
};

enum Spells : uint32
{
    SpellDiseaseCloud = 12627,
    SpellFrenzy = 12795
};
}

struct boss_rift_glutton : public BossAIBase
{
    explicit boss_rift_glutton(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _frenzied = false;
        if (_tierConfig)
            CastIfConfigured(me, SpellDiseaseCloud, true);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (_tier >= 2)
            ScheduleTieredEvent(EventDiseasePulse, 0, 14000, 10000);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!_frenzied && me->HealthBelowPctDamaged(20, damage))
        {
            _frenzied = true;
            CastIfConfigured(me, SpellFrenzy, true);
        }
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        if (eventId != EventDiseasePulse)
            return;
        CastIfConfigured(me, SpellDiseaseCloud, true);
        ScheduleTieredEvent(EventDiseasePulse, 0, 14000, 10000);
    }

private:
    bool _frenzied = false;
};

void AddSC_boss_rift_glutton()
{
    RegisterCreatureAI(boss_rift_glutton);
}

} // namespace HeroicDungeonRift
