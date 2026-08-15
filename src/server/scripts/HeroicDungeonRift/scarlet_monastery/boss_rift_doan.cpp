/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "ScriptMgr.h"
#include "SpellScript.h"

namespace HeroicDungeonRift
{
namespace
{
// 血色图书馆 - 奥法师杜安（Arcanist Doan）
enum Events : uint32
{
    EventArcaneExplosion = 1, // 奥术爆炸（原版/T1基础，自身范围）
    EventSilence,             // 沉默（原版/T1基础，自身范围）
    EventPolymorph,           // 变形术（原版/T1基础，随机玩家）
    EventArcaneBubble,        // 奥术气泡（原版/T1基础，血量<50%无敌）
    EventDetonation,          // 引爆（原版/T1基础，气泡后范围火焰伤害）
    EventTier2Skill,          // 奥术飞弹（T2新增）
    EventTier3Skill           // 烈焰风暴（T3新增）
};

enum Spells : uint32
{
    SpellArcaneExplosion = 9433,        // 奥术爆炸（原版/T1基础）
    SpellSilence = 8988,                // 沉默（原版/T1基础）
    SpellPolymorph = 13323,             // 变形术（原版/T1基础）
    SpellArcaneBubble = 9438,           // 奥术气泡（原版/T1基础）
    SpellDetonation = 9435,             // 引爆（原版/T1基础）
    SpellArcaneMissiles = 15790,        // 奥术飞弹（T2新增，父Aura）
    SpellArcaneMissilesDamage = 15791,  // 奥术飞弹（T2新增，父Aura每跳触发）
    SpellFlamestrike = 12468            // 烈焰风暴（T3新增，混合BP0直伤/BP1周期）
};

constexpr int32 ArcaneMissilesTier1DamagePerTick = 1800;
constexpr int32 FlamestrikeTier1DirectDamage = 4500;
constexpr int32 FlamestrikeTier1DamagePerTick = 1800;

// 喊话（中文，对应原版 creature_text）
constexpr char const* DoanAggroText = "你们不能玷污这些奥秘！";
constexpr char const* DoanDetonateText = "在正义之火中燃烧吧！";
constexpr uint32 DoanAggroSound = 5842;
constexpr uint32 DoanDetonateSound = 5843;
}

class spell_rift_doan_arcane_missiles : public AuraScript
{
    PrepareAuraScript(spell_rift_doan_arcane_missiles);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SpellArcaneMissilesDamage });
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        Creature* caster = GetCaster() ? GetCaster()->ToCreature() : nullptr;
        Unit* target = GetTarget();
        if (!caster || !target || GetTierForCreature(caster) < 2)
            return;

        PreventDefaultAction();
        int32 damage = ArcaneMissilesTier1DamagePerTick;
        caster->CastCustomSpell(SpellArcaneMissilesDamage, SPELLVALUE_BASE_POINT0, damage, target,
            TRIGGERED_FULL_MASK, nullptr, aurEff);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_rift_doan_arcane_missiles::HandlePeriodic,
            EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

struct boss_rift_doan : public BossAIBase
{
    explicit boss_rift_doan(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(DoanAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(DoanAggroSound);

        events.ScheduleEvent(EventArcaneExplosion, Milliseconds(5000));
        events.ScheduleEvent(EventSilence, Milliseconds(9000));
        events.ScheduleEvent(EventPolymorph, Milliseconds(7000));
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 11s); // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 16s); // T3新增
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        // 原版机制：血量低于50%时开启奥术气泡无敌并准备引爆
        if (_bubbleTriggered || !me->HealthBelowPctDamaged(50, damage))
            return;

        _bubbleTriggered = true;
        me->Yell(DoanDetonateText, LANG_UNIVERSAL);
        me->PlayDirectSound(DoanDetonateSound);
        CastIfConfigured(me, SpellArcaneBubble);
        events.ScheduleEvent(EventDetonation, 6s);
    }

    void ConfigureTier() override
    {
        SetRaidSpellDamageMultiplier(15.0f);
        AddInterruptImmuneSpell(SpellArcaneMissiles);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventArcaneExplosion:
                CastIfConfigured(me, SpellArcaneExplosion);
                events.ScheduleEvent(EventArcaneExplosion, Milliseconds(8000));
                break;
            case EventSilence:
                CastIfConfigured(me, SpellSilence);
                events.ScheduleEvent(EventSilence, Milliseconds(20000));
                break;
            case EventPolymorph:
                CastIfConfigured(SelectRandomPlayer(), SpellPolymorph);
                events.ScheduleEvent(EventPolymorph, Milliseconds(20000));
                break;
            case EventArcaneBubble:
                break;
            case EventDetonation:
                CastRaidTunedSpell(me, SpellDetonation, 4000);
                break;
            case EventTier2Skill: // T2新增：奥术飞弹；伤害由15790父Aura每跳触发15791
                CastIfConfigured(me->GetVictim(), SpellArcaneMissiles);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 8s : 10s);
                break;
            case EventTier3Skill: // T3新增：烈焰风暴，点名随机目标；混合BP0直伤/BP1周期，瞬发
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellFlamestrike,
                    FlamestrikeTier1DirectDamage, FlamestrikeTier1DamagePerTick, true);
                events.ScheduleEvent(EventTier3Skill, 22s);
                break;
            default:
                break;
        }
    }

private:
    bool _bubbleTriggered = false;
};

void AddSC_boss_rift_doan()
{
    RegisterCreatureAI(boss_rift_doan);
    RegisterSpellScript(spell_rift_doan_arcane_missiles);
}

} // namespace HeroicDungeonRift
