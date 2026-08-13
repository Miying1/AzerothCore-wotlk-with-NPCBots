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
    EventFlameShock = 1, // 烈焰震击（T1基础）
    EventEarthShock,     // 大地震击（T1基础）
    EventGroundTremor,   // 大地震颤（T1基础）
    EventTier2Skill,     // 火焰冲击（T2新增）
    EventTier3Skill      // 熔岩喷溅（T3新增）
};

enum Spells : uint32
{
    SpellFlameShock = 13729,  // 烈焰震击
    SpellEarthShock = 13728,  // 大地震击
    SpellGroundTremor = 6524, // 大地震颤
    SpellFireBlast = 13342,   // 火焰冲击
    SpellMagmaSplash = 13880  // 熔岩喷溅
};
}

struct boss_rift_roccor : public BossAIBase
{
    explicit boss_rift_roccor(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventFlameShock, 9000, 7000, 5500);
        ScheduleTieredEvent(EventEarthShock, 8000, 6500, 5200);
        ScheduleTieredEvent(EventGroundTremor, 12000, 9500, 7500);
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
                ScheduleTieredEvent(EventFlameShock, 13000, 10500, 8500);
                break;
            case EventEarthShock:
                CastIfConfigured(SelectRandomPlayer(), SpellEarthShock);
                ScheduleTieredEvent(EventEarthShock, 10000, 8000, 6500);
                break;
            case EventGroundTremor:
                CastIfConfigured(me, SpellGroundTremor);
                ScheduleTieredEvent(EventGroundTremor, 14000, 11000, 9000);
                break;
            case EventTier2Skill: // T2新增：火焰冲击，瞬发
                CastIfConfigured(me->GetVictim(), SpellFireBlast, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 6s : 8s);
                break;
            case EventTier3Skill: // T3新增：熔岩喷溅，瞬发
                CastIfConfigured(me->GetVictim(), SpellMagmaSplash, true);
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
