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
        // 除按生命百分比结算的音爆外，所有技能的T1基线统一由 rift_spell_damage.h 调谐表提供，
        // 运行时再由 BossAIBase::DamageDealt 应用当前Tier的伤害倍率。
        AddInterruptImmuneSpell(SpellSonicBoomCast);
        AddInterruptImmuneSpell(SpellLightningNova);
        AddInterruptImmuneSpell(SpellSonicScreech);
        // 38795的伤害由 spell_murmur_sonic_boom_effect 按目标当前生命的80%结算，
        // 百分比机制不应再叠加Tier倍率，否则T2/T3会超过目标生命上限变成必死。
        AddUnscaledSpell(SpellSonicBoomEffect);
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
                    CastIfConfigured(me, SpellResonance);
                    events.ScheduleEvent(EventResonanceCast, Milliseconds(urand(6000, 18000)));
                }
                break;
            case EventThunderingStorm:
                CastIfConfigured(me, SpellThunderingStorm);
                events.ScheduleEvent(EventThunderingStorm, Milliseconds(urand(6050, 10000)));
                break;
            case EventSonicShock:
                CastIfConfigured(me->GetVictim(), SpellSonicShock);
                events.ScheduleEvent(EventSonicShock, Milliseconds(urand(3650, 9150)));
                break;
            case EventLightningNova: // T2新增：5秒读条的范围爆发，避开28秒音爆窗口
                me->TextEmote(LightningNovaText, nullptr, false);
                CastIfConfigured(me, SpellLightningNova);
                events.ScheduleEvent(EventLightningNova, 45s);
                break;
            case EventSonicScreech: // T3新增：2.5秒锥形范围伤害，保留DBC原始读条； 
                me->TextEmote(SonicScreechText, nullptr, false);
                CastIfConfigured(me, SpellSonicScreech);
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
