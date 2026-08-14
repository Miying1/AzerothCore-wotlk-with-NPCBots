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
    EventChainBolt = 1, // 连锁闪电（原版/T1基础）
    EventPurity,        // 净化术（原版/T1基础）
    EventRenew,         // 恢复（原版/T1基础；生命值低于85%时施放）
    EventManaSpike      // 法力尖刺（原版/T1基础；法力值不高于20%时施放）
};

enum Spells : uint32
{
    SpellChainBolt = 8292, // 连锁闪电（原版/T1基础）
    SpellPurity = 8361,    // 净化术（原版/T1基础）
    SpellRenew = 6077,     // 恢复（原版/T1基础）
    SpellManaSpike = 8358  // 法力尖刺（原版/T1基础）
};
}

struct boss_rift_charlga : public BossAIBase
{
    explicit boss_rift_charlga(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventChainBolt, Milliseconds(3000));
        events.ScheduleEvent(EventPurity, Milliseconds(10000));
        events.ScheduleEvent(EventRenew, Milliseconds(12000));
        events.ScheduleEvent(EventManaSpike, 18s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventChainBolt:
                CastIfConfigured(me->GetVictim(), SpellChainBolt);
                events.ScheduleEvent(EventChainBolt, Milliseconds(6500));
                break;
            case EventPurity:
                CastIfConfigured(me, SpellPurity);
                events.ScheduleEvent(EventPurity, Milliseconds(18000));
                break;
            case EventRenew:
                if (me->HealthBelowPct(85))
                    CastIfConfigured(me, SpellRenew);
                events.ScheduleEvent(EventRenew, Milliseconds(18000));
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
