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
    EventWarStomp = 1, // 战争践踏（原版/T1基础）
    EventDeepSlumber,  // 深度沉睡（原版/T1基础）
    EventAcidBreath    // 酸息术（原版/T1基础）
};

enum Spells : uint32
{
    SpellPassiveVisual = 12535, // 伊兰尼库斯之影被动视觉（原版/T1基础）
    SpellThrashAura = 8876,     // 痛击（原版/T1基础）
    SpellWarStomp = 11876,      // 战争践踏（原版/T1基础）
    SpellDeepSlumber = 12890,   // 深度沉睡（原版/T1基础）
    SpellAcidBreath = 12884     // 酸息术（原版/T1基础）
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
        events.ScheduleEvent(EventWarStomp, 17000ms);
        events.ScheduleEvent(EventDeepSlumber, 10000ms);
        events.ScheduleEvent(EventAcidBreath, 5000ms);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventWarStomp:
                CastIfConfigured(me, SpellWarStomp);
                events.ScheduleEvent(EventWarStomp, 25000ms);
                break;
            case EventDeepSlumber:
                CastIfConfigured(SelectRandomPlayer(30.0f), SpellDeepSlumber);
                events.ScheduleEvent(EventDeepSlumber, 23000ms);
                break;
            case EventAcidBreath:
                CastIfConfigured(me->GetVictim(), SpellAcidBreath);
                events.ScheduleEvent(EventAcidBreath, 13000ms);
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
