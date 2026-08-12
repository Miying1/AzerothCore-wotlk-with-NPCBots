/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace HeroicDungeonRift
{
namespace
{
enum Events : uint32
{
    EventThrash = 1,
    EventTier2Skill,
    EventTier3Skill
};

enum Spells : uint32
{
    SpellThrash = 3391,
    SpellMortalStrike = 16856,
    SpellWhirlwind = 15589
};

constexpr int32 WhirlwindRaidAdditionalDamage = 2500;
}

struct boss_rift_vancleef : public BossAIBase
{
    explicit boss_rift_vancleef(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventThrash, 8000, 6500, 5000);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 12s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 18s);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventThrash:
                CastIfConfigured(me->GetVictim(), SpellThrash);
                ScheduleTieredEvent(EventThrash, 8000, 6500, 5000);
                break;
            case EventTier2Skill:
                CastIfConfigured(me->GetVictim(), SpellMortalStrike);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 12s : 15s);
                break;
            case EventTier3Skill:
                CastRaidTunedSpell(me, SpellWhirlwind, WhirlwindRaidAdditionalDamage);
                events.ScheduleEvent(EventTier3Skill, 24s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_vancleef()
{
    RegisterCreatureAI(boss_rift_vancleef);
}

} // namespace HeroicDungeonRift
