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
// 黑石深渊 - 洛考尔（Lord Roccor，地火元素）
enum Events : uint32
{
    EventFlameShock = 1, // 烈焰震击（原版/T1基础）
    EventEarthShock,     // 大地震击（原版/T1基础）
    EventGroundTremor,   // 大地震颤（原版/T1基础）
    EventTier2Skill,     // 火焰冲击（T2新增）
    EventTier3Skill      // 熔岩喷溅（T3新增）
};

enum Spells : uint32
{
    SpellFlameShock = 13729,  // 烈焰震击（原版/T1基础）
    SpellEarthShock = 13728,  // 大地震击（原版/T1基础）
    SpellGroundTremor = 6524, // 大地震颤（原版/T1基础）
    SpellFireBlast = 13342,   // 火焰冲击（T2新增）
    SpellMagmaSplash = 13880  // 熔岩喷溅（T3新增，混合BP0直伤/BP1周期）
};

constexpr int32 FireBlastTier1DirectDamage = 4500;
constexpr int32 MagmaSplashTier1DirectDamage = 3500;
constexpr int32 MagmaSplashTier1DamagePerTick = 1800;
}

struct boss_rift_roccor : public BossAIBase
{
    explicit boss_rift_roccor(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventFlameShock, 9000ms);
        events.ScheduleEvent(EventEarthShock, 8000ms);
        events.ScheduleEvent(EventGroundTremor, 12000ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 7s);  // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 16s); // T3新增
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventFlameShock:
                CastIfConfigured(me->GetVictim(), SpellFlameShock);
                events.ScheduleEvent(EventFlameShock, 13000ms);
                break;
            case EventEarthShock:
                CastIfConfigured(SelectRandomPlayer(), SpellEarthShock);
                events.ScheduleEvent(EventEarthShock, 10000ms);
                break;
            case EventGroundTremor:
                CastIfConfigured(me, SpellGroundTremor);
                events.ScheduleEvent(EventGroundTremor, 14000ms);
                break;
            case EventTier2Skill: // T2新增：火焰冲击，瞬发
                CastFinalRaidDamageSpell(me->GetVictim(), SpellFireBlast, SPELLVALUE_BASE_POINT0,
                    FireBlastTier1DirectDamage, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 6s : 8s);
                break;
            case EventTier3Skill: // T3新增：熔岩喷溅，混合BP0直伤/BP1周期，瞬发
                CastFinalRaidDamageSpell(me->GetVictim(), SpellMagmaSplash,
                    MagmaSplashTier1DirectDamage, MagmaSplashTier1DamagePerTick, true);
                events.ScheduleEvent(EventTier3Skill, 12s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_roccor()
{
    RegisterCreatureAI(boss_rift_roccor);
}

} // namespace HeroicDungeonRift
