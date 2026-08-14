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
    EventFieryBurst = 1,         // 火焰爆发（原版/T1基础）
    EventFireStorm,              // 火焰风暴（原版/T1基础）
    EventMightyBlow,             // 全力一击（原版/T1基础）
    EventCurseOfElementalLord,   // 元素领主的诅咒（T2新增）
    EventTier3Skill              // 烈焰震击（T3新增）
};

enum Spells : uint32
{
    SpellFieryBurst = 13900,           // 火焰爆发（原版/T1基础）
    SpellFireStorm = 13899,            // 火焰风暴（原版/T1基础）
    SpellMightyBlow = 14099,           // 全力一击（原版/T1基础）
    SpellCurseOfElementalLord = 26977, // 元素领主的诅咒（T2新增）
    SpellFlameShock = 13729            // 烈焰震击（T3新增，混合BP0直伤/BP1周期）
};

constexpr int32 FlameShockTier1DirectDamage = 3500;
constexpr int32 FlameShockTier1DamagePerTick = 1800;
constexpr char const* IncendiusDeathText = "我不会被摧毁！以拉格纳罗斯的意志，我将重生！";
constexpr uint32 IncendiusDeathSound = 5061;
}

struct boss_rift_incendius : public BossAIBase
{
    explicit boss_rift_incendius(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventFieryBurst, 11000ms);
        events.ScheduleEvent(EventFireStorm, 7000ms);
        events.ScheduleEvent(EventMightyBlow, 16000ms);
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
                events.ScheduleEvent(EventFieryBurst, 13000ms);
                break;
            case EventFireStorm:
                CastIfConfigured(me->GetVictim(), SpellFireStorm);
                events.ScheduleEvent(EventFireStorm, 8000ms);
                break;
            case EventMightyBlow:
                CastIfConfigured(me->GetVictim(), SpellMightyBlow);
                events.ScheduleEvent(EventMightyBlow, 18000ms);
                break;
            case EventCurseOfElementalLord: // T2新增：元素领主的诅咒，瞬发
                CastIfConfigured(me, SpellCurseOfElementalLord, true);
                events.ScheduleEvent(EventCurseOfElementalLord, _tier == 3 ? 18s : 22s);
                break;
            case EventTier3Skill: // T3新增：烈焰震击，点名随机目标；混合BP0直伤/BP1周期，瞬发
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellFlameShock,
                    FlameShockTier1DirectDamage, FlameShockTier1DamagePerTick, true);
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
