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
// 奥金顿：暗影迷宫 - 摩摩尔（Murmur，源 Entry 18708；裂隙 Entry 100187-100189）
enum Events : uint32
{
    EventSonicBoom = 1,       // 声波爆炸（T1基础）
    EventSonicBoomEffect,     // 声波爆炸伤害（T1基础）
    EventMurmursTouch,        // 摩摩尔之触（T1基础）
    EventMagneticPull,        // 磁力吸引（T1基础）
    EventResonanceCheck,      // 共鸣条件检查（T1基础）
    EventResonanceCast,       // 共鸣施法（T1基础）
    EventThunderingStorm,     // 雷霆风暴（T1英雄基础）
    EventSonicShock,          // 音波震击（T1英雄基础）
    EventLightningNova,       // 闪电新星（T2新增）
    EventSonicScreech         // 音速尖啸（T3新增）
};

enum Spells : uint32
{
    SpellSonicBoomCast = 33923,
    SpellSonicBoomEffect = 38795,
    SpellMurmursTouch = 33711,
    SpellMagneticPull = 33689,
    SpellResonance = 33657,
    SpellThunderingStorm = 39365,
    SpellSonicShock = 38797,
    SpellLightningNova = 52960, // 3.3.5：闪电大厅闪电新星
    SpellSonicScreech = 64422   // 3.3.5：奥杜尔音速尖啸
};

constexpr int32 ResonanceRaidDamage = 4000;
constexpr int32 ThunderingStormRaidDamage = 5000;
constexpr int32 SonicShockRaidDamage = 5500;
constexpr int32 LightningNovaRaidDamage = 6500;
// DBC 64422约60k总锥形分摊伤害；T1基准下调为30k，T3应用2倍后恢复为约60k总量。
constexpr int32 SonicScreechTier1SharedDamage = 30000;

constexpr char const* SonicBoomText = "摩摩尔从空气中汲取能量……";
constexpr char const* LightningNovaText = "摩摩尔周围的空气迸发出强烈电光！";
constexpr char const* SonicScreechText = "摩摩尔汇聚出一道毁灭性的音波！";
}

struct boss_rift_murmur : public BossAIBase
{
    explicit boss_rift_murmur(Creature* creature) : BossAIBase(creature)
    {
        me->SetCombatMovement(false);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        // 裂隙版本直接以满血激活，不保留原版压制者前置与40%初始生命。
        events.ScheduleEvent(EventSonicBoom, 28s);
        events.ScheduleEvent(EventMurmursTouch, Milliseconds(urand(14600, 25500)));
        events.ScheduleEvent(EventMagneticPull, Milliseconds(urand(15000, 30000)));
        events.ScheduleEvent(EventResonanceCheck, 3s);
        events.ScheduleEvent(EventThunderingStorm, 5s);
        events.ScheduleEvent(EventSonicShock, 3650ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventLightningNova, 20s);
        if (_tier >= 3)
            events.ScheduleEvent(EventSonicScreech, 47s);
    }

    void ConfigureTier() override
    {
        // 仅保留全局 Tier 伤害倍率；TBC 固定伤害技能在事件中按83级目标值逐项覆盖。
        SetRaidSpellDamageMultiplier(1.0f);
        AddInterruptImmuneSpell(SpellSonicBoomCast);
        AddInterruptImmuneSpell(SpellLightningNova);
        AddInterruptImmuneSpell(SpellSonicScreech);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventSonicBoom:
                me->TextEmote(SonicBoomText, nullptr, false);
                CastIfConfigured(me, SpellSonicBoomCast);
                events.ScheduleEvent(EventSonicBoomEffect, 1500ms);
                events.ScheduleEvent(EventSonicBoom, Milliseconds(urand(34000, 40000)));
                break;
            case EventSonicBoomEffect:
                CastIfConfigured(me, SpellSonicBoomEffect, true);
                break;
            case EventMurmursTouch:
                CastIfConfigured(SelectRandomPlayer(80.0f), SpellMurmursTouch);
                events.ScheduleEvent(EventMurmursTouch, Milliseconds(urand(14600, 25500)));
                break;
            case EventMagneticPull:
                CastIfConfigured(SelectRandomPlayer(80.0f), SpellMagneticPull);
                events.ScheduleEvent(EventMagneticPull, Milliseconds(urand(15000, 30000)));
                break;
            case EventResonanceCheck:
                if (ShouldCastResonance() && !events.HasTimeUntilEvent(EventResonanceCast))
                    events.ScheduleEvent(EventResonanceCast, 5s);
                events.ScheduleEvent(EventResonanceCheck, 3s);
                break;
            case EventResonanceCast:
                if (ShouldCastResonance())
                {
                    CastRaidTunedSpell(me, SpellResonance, ResonanceRaidDamage);
                    events.ScheduleEvent(EventResonanceCast, Milliseconds(urand(6000, 18000)));
                }
                break;
            case EventThunderingStorm:
                CastRaidTunedSpell(me, SpellThunderingStorm, ThunderingStormRaidDamage);
                events.ScheduleEvent(EventThunderingStorm, Milliseconds(urand(6050, 10000)));
                break;
            case EventSonicShock:
                CastRaidTunedSpell(me->GetVictim(), SpellSonicShock, SonicShockRaidDamage);
                events.ScheduleEvent(EventSonicShock, Milliseconds(urand(3650, 9150)));
                break;
            case EventLightningNova: // T2新增：5秒读条的范围爆发，避开28秒音爆窗口
                me->TextEmote(LightningNovaText, nullptr, false);
                CastFinalRaidDamageSpell(me, SpellLightningNova, SPELLVALUE_BASE_POINT0,
                    LightningNovaRaidDamage);
                events.ScheduleEvent(EventLightningNova, 45s);
                break;
            case EventSonicScreech: // T3新增：2.5秒锥形分摊伤害，保留DBC原始读条
                me->TextEmote(SonicScreechText, nullptr, false);
                CastFinalRaidDamageSpell(me, SpellSonicScreech, SPELLVALUE_BASE_POINT0,
                    SonicScreechTier1SharedDamage);
                events.ScheduleEvent(EventSonicScreech, 50s);
                break;
            default:
                break;
        }
    }

private:
    bool ShouldCastResonance() const
    {
        Unit* victim = me->GetVictim();
        if (!victim || !me->IsWithinMeleeRange(victim))
            return true;
        if (Unit* victimTarget = victim->GetVictim())
            return victimTarget != me;
        return true;
    }
};

void AddSC_boss_rift_murmur()
{
    RegisterCreatureAI(boss_rift_murmur);
}

} // namespace HeroicDungeonRift
