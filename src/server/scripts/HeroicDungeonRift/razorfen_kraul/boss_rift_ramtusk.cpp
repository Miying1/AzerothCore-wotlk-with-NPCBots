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
    EventBattleShout = 1,
    EventThunderclap
};

enum Spells : uint32
{
    SpellBattleShout = 9128,
    SpellThunderclap = 15548
};
constexpr int32 ThunderclapRaidDamage = 2500;
}

struct boss_rift_ramtusk : public BossAIBase
{
    explicit boss_rift_ramtusk(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventBattleShout, 2s);
        events.ScheduleEvent(EventThunderclap, 4s);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventBattleShout:
                CastIfConfigured(me, SpellBattleShout);
                ScheduleTieredEvent(EventBattleShout, 30000, 24000, 19000);
                break;
            case EventThunderclap:
                CastRaidTunedSpell(me, SpellThunderclap, ThunderclapRaidDamage);
                ScheduleTieredEvent(EventThunderclap, 13000, 10500, 8000);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_ramtusk()
{
    RegisterCreatureAI(boss_rift_ramtusk);
}

} // namespace HeroicDungeonRift
