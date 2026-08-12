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
    EventCooking = 1,
    EventAcid,
    EventTier2Skill,
    EventTier3Skill
};

enum Spells : uint32
{
    SpellCooking = 5174,
    SpellAcid = 6306,
    SpellAcidicFang = 29901,
    SpellPoisonCloud = 23861
};

enum RaidTunedDamage : int32
{
    AcidSplashDamagePerTick = 2000,
    AcidicFangDamagePerTick = 1000,
    PoisonCloudDamagePerTick = 1500
};
}

struct boss_rift_cookie : public BossAIBase
{
    explicit boss_rift_cookie(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventCooking, 7000, 6000, 4500);
        ScheduleTieredEvent(EventAcid, 11000, 8500, 6500);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 14s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 20s);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventCooking:
                CastIfConfigured(me, SpellCooking);
                ScheduleTieredEvent(EventCooking, 7000, 6000, 4500);
                break;
            case EventAcid:
                CastRaidTunedSpell(me, SpellAcid, AcidSplashDamagePerTick);
                ScheduleTieredEvent(EventAcid, 11000, 8500, 6500);
                break;
            case EventTier2Skill:
                CastRaidTunedSpell(me->GetVictim(), SpellAcidicFang, AcidicFangDamagePerTick);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 13s : 16s);
                break;
            case EventTier3Skill:
                CastRaidTunedSpell(me, SpellPoisonCloud, PoisonCloudDamagePerTick);
                events.ScheduleEvent(EventTier3Skill, 21s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_cookie()
{
    RegisterCreatureAI(boss_rift_cookie);
}

} // namespace HeroicDungeonRift
