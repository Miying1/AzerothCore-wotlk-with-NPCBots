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
// 厄运之槌东区 - 蕾瑟塔蒂丝（Lethtendris）
enum Events : uint32
{
    EventVoidBolt = 1,      // 虚空箭（T1基础）
    EventShadowBoltVolley,  // 暗影箭雨（T1基础）
    EventImmolate,          // 献祭（T1基础）
    EventCurseOfThorns,     // 荆棘诅咒（T2新增）
    EventCurseOfTongues     // 语言诅咒（T3新增）
};

enum Spells : uint32
{
    SpellVoidBolt = 22709,        // 虚空箭
    SpellShadowBoltVolley = 14887,// 暗影箭雨
    SpellImmolate = 20787,        // 献祭
    SpellCurseOfThorns = 16247,   // 荆棘诅咒
    SpellCurseOfTongues = 13338   // 语言诅咒
};
}

struct boss_rift_lethtendris : public BossAIBase
{
    explicit boss_rift_lethtendris(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventVoidBolt, 3000, 2400, 1800);
        ScheduleTieredEvent(EventShadowBoltVolley, 8000, 6500, 5200);
        ScheduleTieredEvent(EventImmolate, 10000, 8000, 6500);
        if (_tier >= 2)
            events.ScheduleEvent(EventCurseOfThorns, 12s);
        if (_tier >= 3)
            events.ScheduleEvent(EventCurseOfTongues, 15s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventVoidBolt:
                CastIfConfigured(me->GetVictim(), SpellVoidBolt);
                ScheduleTieredEvent(EventVoidBolt, 3200, 2600, 2000);
                break;
            case EventShadowBoltVolley:
                CastIfConfigured(me, SpellShadowBoltVolley);
                ScheduleTieredEvent(EventShadowBoltVolley, 14000, 11000, 9000);
                break;
            case EventImmolate:
                CastIfConfigured(SelectRandomPlayer(), SpellImmolate);
                ScheduleTieredEvent(EventImmolate, 18000, 14500, 11500);
                break;
            case EventCurseOfThorns: // T2新增：荆棘诅咒，顺发
                CastIfConfigured(me->GetVictim(), SpellCurseOfThorns, true);
                events.ScheduleEvent(EventCurseOfThorns, _tier == 3 ? 22s : 28s);
                break;
            case EventCurseOfTongues: // T3新增：语言诅咒，点名随机目标，瞬发
                CastIfConfigured(SelectRandomPlayer(), SpellCurseOfTongues, true);
                events.ScheduleEvent(EventCurseOfTongues, 24s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_lethtendris()
{
    RegisterCreatureAI(boss_rift_lethtendris);
}

} // namespace HeroicDungeonRift
