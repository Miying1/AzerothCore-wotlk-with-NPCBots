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
    EventSleep = 1, // 催眠术（原版/T1基础）
    EventMindBlast  // 心灵震爆（原版/T1基础）
};

enum Spells : uint32
{
    SpellResetVisual = 8734, // 黑暗深渊引导（原版/T1基础；重置视觉）
    SpellSleep = 8399,       // 催眠术（原版/T1基础）
    SpellMindBlast = 15587   // 心灵震爆（原版/T1基础）
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
        events.ScheduleEvent(EventSleep, Milliseconds(12000));
        events.ScheduleEvent(EventMindBlast, Milliseconds(7000));
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
                events.ScheduleEvent(EventSleep, Milliseconds(18000));
                break;
            case EventMindBlast:
                CastIfConfigured(me->GetVictim(), SpellMindBlast);
                events.ScheduleEvent(EventMindBlast, Milliseconds(10000));
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
