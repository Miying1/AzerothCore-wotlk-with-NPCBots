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
// 玛拉顿 - 维利塔恩（Lord Vyletongue）
enum Events : uint32
{
    EventShoot = 1,     // 射击（T1基础）
    EventMultiShot,     // 多重射击（T1基础）
    EventSmokeBomb,     // 烟雾弹（T2新增）
    EventTier3Skill     // 闪现术（T3新增）
};

enum Spells : uint32
{
    SpellDualWield = 42459, // 双武器
    SpellShoot = 16100,     // 射击
    SpellMultiShot = 21390, // 多重射击
    SpellSmokeBomb = 7964,  // 烟雾弹
    SpellBlink = 21655      // 闪现术
};
}

struct boss_rift_vyletongue : public BossAIBase
{
    explicit boss_rift_vyletongue(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        CastIfConfigured(me, SpellDualWield, true);
        ScheduleTieredEvent(EventShoot, 2500, 2000, 1600);
        ScheduleTieredEvent(EventMultiShot, 6000, 4800, 3800);
        if (_tier >= 2)
            events.ScheduleEvent(EventSmokeBomb, 14s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 11s);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventShoot:
                CastIfConfigured(me->GetVictim(), SpellShoot);
                ScheduleTieredEvent(EventShoot, 2600, 2100, 1700);
                break;
            case EventMultiShot:
                CastIfConfigured(me->GetVictim(), SpellMultiShot);
                ScheduleTieredEvent(EventMultiShot, 9000, 7000, 5500);
                break;
            case EventSmokeBomb: // T2新增：烟雾弹，瞬发
                CastIfConfigured(me, SpellSmokeBomb, true);
                events.ScheduleEvent(EventSmokeBomb, _tier == 3 ? 13s : 16s);
                break;
            case EventTier3Skill: // T3新增：闪现术，瞬发
                CastIfConfigured(me, SpellBlink, true);
                events.ScheduleEvent(EventTier3Skill, 15s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_vyletongue()
{
    RegisterCreatureAI(boss_rift_vyletongue);
}

} // namespace HeroicDungeonRift
