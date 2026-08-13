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
// 黑石深渊 - 伊森迪奥斯（Lord Incendius）
enum Events : uint32
{
    EventFieryBurst = 1,         // 火焰爆发（T1基础）
    EventFireStorm,              // 火焰风暴（T1基础）
    EventMightyBlow,             // 全力一击（T1基础）
    EventCurseOfElementalLord,   // 元素领主的诅咒（T2新增）
    EventTier3Skill              // 烈焰震击（T3新增）
};

enum Spells : uint32
{
    SpellFieryBurst = 13900,         // 火焰爆发
    SpellFireStorm = 13899,          // 火焰风暴
    SpellMightyBlow = 14099,         // 全力一击
    SpellCurseOfElementalLord = 26977, // 元素领主的诅咒
    SpellFlameShock = 13729          // 烈焰震击
};

constexpr char const* IncendiusDeathText = "我不会被摧毁！以拉格纳罗斯的意志，我将重生！";
constexpr uint32 IncendiusDeathSound = 5061;
}

struct boss_rift_incendius : public BossAIBase
{
    explicit boss_rift_incendius(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventFieryBurst, 11000, 9000, 7200);
        ScheduleTieredEvent(EventFireStorm, 7000, 5500, 4500);
        ScheduleTieredEvent(EventMightyBlow, 16000, 13000, 10500);
        if (_tier >= 2)
            events.ScheduleEvent(EventCurseOfElementalLord, 18s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 13s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->Yell(IncendiusDeathText, LANG_UNIVERSAL);
        me->PlayDirectSound(IncendiusDeathSound);
        BossAIBase::JustDied(nullptr);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventFieryBurst:
                CastIfConfigured(me->GetVictim(), SpellFieryBurst);
                ScheduleTieredEvent(EventFieryBurst, 13000, 10500, 8500);
                break;
            case EventFireStorm:
                CastIfConfigured(me->GetVictim(), SpellFireStorm);
                ScheduleTieredEvent(EventFireStorm, 8000, 6500, 5200);
                break;
            case EventMightyBlow:
                CastIfConfigured(me->GetVictim(), SpellMightyBlow);
                ScheduleTieredEvent(EventMightyBlow, 18000, 14500, 11500);
                break;
            case EventCurseOfElementalLord: // T2新增：元素领主的诅咒，读条不可打断
                CastIfConfigured(me, SpellCurseOfElementalLord, true);
                events.ScheduleEvent(EventCurseOfElementalLord, _tier == 3 ? 18s : 22s);
                break;
            case EventTier3Skill: // T3新增：烈焰震击，点名随机目标，读条不可打断
                CastIfConfigured(SelectRandomPlayer(), SpellFlameShock, true);
                events.ScheduleEvent(EventTier3Skill, 10s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_incendius()
{
    RegisterCreatureAI(boss_rift_incendius);
}

} // namespace HeroicDungeonRift
