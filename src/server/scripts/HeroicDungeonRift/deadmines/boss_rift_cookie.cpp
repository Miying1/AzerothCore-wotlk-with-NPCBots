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
    EventCooking = 1, // 烹饪（原版/T1基础）
    EventAcid, // 酸液（原版/T1基础）
    EventTier2Skill, // 酸性之牙（T2新增；T3沿用并缩短循环间隔）
    EventTier3Skill // 毒云（T3新增）
};

enum Spells : uint32
{
    SpellCooking = 5174, // 烹饪（原版/T1基础）：对自身施放
    SpellAcid = 6306, // 酸液（原版/T1基础）：使用裂隙伤害校准施放
    SpellAcidicFang = 29901, // 酸性之牙（T2新增）：基础点0为每跳伤害
    SpellPoisonCloud = 23861 // 毒云（T3新增）：基础点0为每跳伤害
};

// 裂隙伤害校准：酸液覆盖基础点0；T2/T3新增持续伤害按T1基准反算后再应用Tier倍率。
enum RaidTunedDamage : int32
{
    AcidSplashDamagePerTick = 2000, // 酸液每跳校准为2000点
    AcidicFangTier1DamagePerTick = 1800, // 酸性之牙T1基准每跳1800点
    PoisonCloudTier1DamagePerTick = 1800 // 毒云T1基准每跳1800点
};
}

struct boss_rift_cookie : public BossAIBase
{
    explicit boss_rift_cookie(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventCooking, Milliseconds(7000));
        events.ScheduleEvent(EventAcid, Milliseconds(11000));
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
                events.ScheduleEvent(EventCooking, Milliseconds(7000));
                break;
            case EventAcid:
                CastRaidTunedSpell(me, SpellAcid, AcidSplashDamagePerTick);
                events.ScheduleEvent(EventAcid, Milliseconds(11000));
                break;
            case EventTier2Skill:
                CastFinalRaidDamageSpell(me->GetVictim(), SpellAcidicFang, SPELLVALUE_BASE_POINT0, AcidicFangTier1DamagePerTick);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 13s : 16s);
                break;
            case EventTier3Skill:
                CastFinalRaidDamageSpell(me, SpellPoisonCloud, SPELLVALUE_BASE_POINT0, PoisonCloudTier1DamagePerTick);
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
