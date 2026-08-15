/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or any later version.
 */

#ifndef HEROIC_DUNGEON_RIFT_BOSS_BASE_H
#define HEROIC_DUNGEON_RIFT_BOSS_BASE_H

#include "rift_defines.h"
#include "rift_spell_damage.h"

#include "ObjectMgr.h"
#include "ScriptedCreature.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StringFormat.h"
#include "ThreatManager.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace HeroicDungeonRift
{
inline int32 CompensateRiftCreatureLevelScaling(Creature const* caster, SpellInfo const* spellInfo,
    uint8 effectIndex, int32 intendedBasePoint)
{
    if (!caster || !spellInfo || effectIndex >= MAX_SPELL_EFFECTS || !intendedBasePoint ||
        caster->IsControlledByPlayer() || !spellInfo->SpellLevel || spellInfo->SpellLevel == caster->GetLevel() ||
        spellInfo->Effects[effectIndex].RealPointsPerLevel ||
        !spellInfo->HasAttribute(SPELL_ATTR0_SCALES_WITH_CREATURE_LEVEL))
        return intendedBasePoint;

    SpellEffectInfo const& effect = spellInfo->Effects[effectIndex];
    bool canEffectScale = false;
    switch (effect.Effect)
    {
        case SPELL_EFFECT_SCHOOL_DAMAGE:
        case SPELL_EFFECT_DUMMY:
        case SPELL_EFFECT_POWER_DRAIN:
        case SPELL_EFFECT_HEALTH_LEECH:
        case SPELL_EFFECT_HEAL:
        case SPELL_EFFECT_WEAPON_DAMAGE:
        case SPELL_EFFECT_POWER_BURN:
        case SPELL_EFFECT_SCRIPT_EFFECT:
        case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
        case SPELL_EFFECT_FORCE_CAST_WITH_VALUE:
        case SPELL_EFFECT_TRIGGER_SPELL_WITH_VALUE:
        case SPELL_EFFECT_TRIGGER_MISSILE_SPELL_WITH_VALUE:
            canEffectScale = true;
            break;
        default:
            break;
    }

    switch (effect.ApplyAuraName)
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_DUMMY:
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_DAMAGE_SHIELD:
        case SPELL_AURA_PROC_TRIGGER_DAMAGE:
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_PERIODIC_MANA_LEECH:
        case SPELL_AURA_SCHOOL_ABSORB:
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
            canEffectScale = true;
            break;
        default:
            break;
    }

    SpellInfo const* triggerSpell = sSpellMgr->GetSpellInfo(effect.TriggerSpell);
    if (!canEffectScale || (triggerSpell && triggerSpell->HasAttribute(SPELL_ATTR0_SCALES_WITH_CREATURE_LEVEL)))
        return intendedBasePoint;

    CreatureTemplate const* creatureTemplate = caster->GetCreatureTemplate();
    CreatureBaseStats const* casterStats = sObjectMgr->GetCreatureBaseStats(caster->GetLevel(), caster->getClass());
    CreatureBaseStats const* spellStats = sObjectMgr->GetCreatureBaseStats(spellInfo->SpellLevel, caster->getClass());
    if (!creatureTemplate || !casterStats || !spellStats)
        return intendedBasePoint;

    float casterPower = casterStats->BaseDamage[creatureTemplate->expansion];
    float spellPower = spellStats->BaseDamage[creatureTemplate->expansion];
    if (casterPower <= 0.0f || spellPower <= 0.0f)
        return intendedBasePoint;

    // CalcValue稍后会乘以casterPower / spellPower，此处先乘倒数，使最终数值回到配置的T1基线。
    double compensated = double(intendedBasePoint) * spellPower / casterPower;
    compensated = std::clamp(compensated, double(std::numeric_limits<int32>::min()),
        double(std::numeric_limits<int32>::max()));
    int32 rounded = int32(std::lround(compensated));
    if (intendedBasePoint > 0)
        return std::max<int32>(1, rounded);

    return std::min<int32>(-1, rounded);
}

inline int32 CompensateRiftCreatureLevelScaling(Creature const* caster, uint32 spellId,
    uint8 effectIndex, int32 intendedBasePoint)
{
    return CompensateRiftCreatureLevelScaling(caster, sSpellMgr->GetSpellInfo(spellId), effectIndex,
        intendedBasePoint);
}

inline SpellCastResult CastRiftTunedSpell(Creature* caster, Unit* target, uint32 spellId, bool triggered = false,
    uint32* lastSpellId = nullptr)
{
    if (!caster || !target || !spellId)
        return SPELL_FAILED_BAD_TARGETS;

    if (lastSpellId)
        *lastSpellId = spellId;

    if (RiftSpellDamageTuning const* tuning = GetRiftSpellDamageTuning(spellId))
    {
        CustomSpellValues values;
        for (uint8 effectIndex = 0; effectIndex < tuning->EffectBasePoints.size(); ++effectIndex)
            if (int32 basePoint = tuning->EffectBasePoints[effectIndex])
                values.AddSpellMod(SpellValueMod(SPELLVALUE_BASE_POINT0 + effectIndex),
                    CompensateRiftCreatureLevelScaling(caster, spellId, effectIndex, basePoint));

        if (!values.empty())
            return caster->CastCustomSpell(spellId, values, target,
                triggered ? TRIGGERED_FULL_MASK : TRIGGERED_NONE);
    }

    return caster->CastSpell(target, spellId, triggered);
}

inline void ScaleRiftSummonSpellDamage(uint32 spellId, uint32& damage, DamageEffectType damageType,
    uint32 spellDamagePermille, uint32 spellDamageBaseFactor)
{
    if (damageType == DIRECT_DAMAGE || !spellDamageBaseFactor)
        return;

    // spellDamagePermille包含Tier配置倍率、召唤物伤害系数以及60级内容沿用的历史法术基准系数。
    // 无论AI回调能否识别触发型子法术，都必须先移除该基准系数，否则T1技能会退回旧的15倍修正并被严重放大。
    double multiplier = double(spellDamagePermille) / (1000.0 * spellDamageBaseFactor);
    if (RiftSpellDamageTuning const* tuning = GetRiftSpellDamageTuning(spellId))
        multiplier *= tuning->WeaponDamageMultiplier;

    uint64 scaledDamage = uint64(double(damage) * multiplier);
    damage = uint32(std::min<uint64>(scaledDamage, std::numeric_limits<uint32>::max()));
}

class BossAIBase : public ScriptedAI
{
public:
    explicit BossAIBase(Creature* creature) : ScriptedAI(creature), _baseHealth(std::max<uint32>(1, creature->GetMaxHealth())) { }

    void Reset() override
    {
        DespawnRiftSummons();
        events.Reset();
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, false);
        _interruptImmuneSpells.clear();
        _lastCastSpellId = 0;
        _tier = GetTierForCreature(me);
        _tierConfig = GetTierConfigForCreature(me);
        if (!_tierConfig || _tier < 1 || _tier > MaxTier)
        {
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            return;
        }

        ApplyTierStats(me, *_tierConfig, _baseHealth);
        me->SetVisible(true);
        me->SetStandState(UNIT_STAND_STATE_STAND);
        me->SetReactState(REACT_AGGRESSIVE);
        me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC |
            UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE_2 | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE);
        me->RemoveUnitFlag2(UNIT_FLAG2_FEIGN_DEATH | UNIT_FLAG2_HIDE_BODY);
        ConfigureTier();
    }

    // 仅对注册过的读条技能免疫打断：读条开始时开启、读条结束时关闭，
    // 其余技能（含 T1 基础技能）保持与源 Boss 一致的可打断性。
    void OnSpellCast(SpellInfo const* spell) override
    {
        if (spell)
            _lastCastSpellId = spell->Id;
        RemoveInterruptImmunity(spell);
    }

    void OnSpellFailed(SpellInfo const* spell) override
    {
        RemoveInterruptImmunity(spell);
    }

    void DamageDealt(Unit* victim, uint32& damage, DamageEffectType damageType,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!_tierConfig)
            return;

        double multiplier = _tierConfig->DamageMultiplier;
        if (damageType != DIRECT_DAMAGE)
        {
            uint32 spellId = GetDamageSpellId(victim, damageType);
            if (RiftSpellDamageTuning const* tuning = GetRiftSpellDamageTuning(spellId))
            {
                // 固定伤害/DoT在施法时已写入T1基线；武器技能则保留独立武器倍率。
                if (tuning->WeaponDamageMultiplier > 1.0f)
                    multiplier *= tuning->WeaponDamageMultiplier;
            }
        }

        damage = uint32(std::min<double>(double(damage) * multiplier, std::numeric_limits<uint32>::max()));
    }

    void JustDied(Unit* /*killer*/) override
    {
        DespawnRiftSummons();
    }

    void OnSpellStart(SpellInfo const* spell) override
    {
        if (spell)
            _lastCastSpellId = spell->Id;
        if (spell && std::find(_interruptImmuneSpells.begin(), _interruptImmuneSpells.end(), spell->Id) != _interruptImmuneSpells.end())
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
            ExecuteRiftEvent(eventId);

        DoMeleeAttackIfReady();
    }

protected:
    virtual void ConfigureTier() = 0;
    virtual void ExecuteRiftEvent(uint32 eventId) = 0;

    void ScheduleTieredEvent(uint32 eventId, uint32 tier1Delay, uint32 tier2Delay, uint32 tier3Delay)
    {
        uint32 delay = _tier == 1 ? tier1Delay : (_tier == 2 ? tier2Delay : tier3Delay);
        events.ScheduleEvent(eventId, Milliseconds(delay));
    }

    SpellCastResult DoCast(Unit* target, uint32 spellId, bool triggered = false)
    {
        return CastIfConfigured(target, spellId, triggered);
    }

    SpellCastResult DoCastVictim(uint32 spellId, bool triggered = false)
    {
        return DoCast(me->GetVictim(), spellId, triggered);
    }

    SpellCastResult CastIfConfigured(Unit* target, uint32 spellId, bool triggered = false)
    {
        return CastRiftTunedSpell(me, target, spellId, triggered, &_lastCastSpellId);
    }

    bool CastRaidTunedSpell(Unit* target, uint32 spellId, int32 basePoint0, bool triggered = false)
    {
        if (!target || !spellId || basePoint0 <= 0)
            return false;
        _lastCastSpellId = spellId;
        int32 compensatedBasePoint = CompensateRiftCreatureLevelScaling(me, spellId, EFFECT_0, basePoint0);
        me->CastCustomSpell(spellId, SPELLVALUE_BASE_POINT0, compensatedBasePoint, target, triggered);
        return true;
    }

    // T2/T3新增技能按Spell_dbc的具体效果单独给出T1基准伤害。
    // 运行时Tier倍率继续由DamageDealt统一应用。
    bool CastFinalRaidDamageSpell(Unit* target, uint32 spellId, SpellValueMod damageEffect,
        int32 tier1BaseDamage, bool triggered = false)
    {
        if (!target || !spellId || tier1BaseDamage <= 0 || damageEffect > SPELLVALUE_BASE_POINT2)
            return false;

        _lastCastSpellId = spellId;
        uint8 effectIndex = uint8(damageEffect - SPELLVALUE_BASE_POINT0);
        int32 compensatedBasePoint = CompensateRiftCreatureLevelScaling(me, spellId, effectIndex, tier1BaseDamage);
        me->CastCustomSpell(spellId, damageEffect, compensatedBasePoint, target, triggered);
        return true;
    }

    bool CastFinalRaidDamageSpell(Unit* target, uint32 spellId, int32 tier1BaseDamage0,
        int32 tier1BaseDamage1, bool triggered = false)
    {
        if (!target || !spellId || tier1BaseDamage0 <= 0 || tier1BaseDamage1 <= 0)
            return false;

        _lastCastSpellId = spellId;
        CustomSpellValues values;
        values.AddSpellMod(SPELLVALUE_BASE_POINT0,
            CompensateRiftCreatureLevelScaling(me, spellId, EFFECT_0, tier1BaseDamage0));
        values.AddSpellMod(SPELLVALUE_BASE_POINT1,
            CompensateRiftCreatureLevelScaling(me, spellId, EFFECT_1, tier1BaseDamage1));
        me->CastCustomSpell(spellId, values, target, triggered ? TRIGGERED_FULL_MASK : TRIGGERED_NONE);
        return true;
    }

    Unit* SelectRandomPlayer(float range = 100.0f, bool alive = true)
    {
        return SelectTarget(SelectTargetMethod::Random, 0, range, alive);
    }

    // 优先选中正在读条的真实玩家（供冲锋带打断类技能使用）；无读条目标时回退为随机玩家。
    Unit* SelectCastingPlayer(float range = 100.0f)
    {
        std::vector<Unit*> casters;
        for (ThreatReference const* ref : me->GetThreatMgr().GetUnsortedThreatList())
        {
            Unit* unit = ref->GetVictim();
            if (unit && unit->IsAlive() && unit->IsPlayer() && unit->HasUnitState(UNIT_STATE_CASTING) &&
                me->IsWithinDistInMap(unit, range))
                casters.push_back(unit);
        }

        if (!casters.empty())
            return casters[urand(0, casters.size() - 1)];

        return SelectRandomPlayer(range);
    }

    Creature* SummonTieredCreature(uint32 entry, Position const& position, float healthCoefficient = 1.0f,
        float damageCoefficient = 1.0f, TempSummonType summonType = TEMPSUMMON_CORPSE_TIMED_DESPAWN,
        uint32 despawnMilliseconds = 10 * IN_MILLISECONDS, bool preserveStonedState = false)
    {
        if (!entry || !_tierConfig)
            return nullptr;

        Creature* summon = me->SummonCreature(entry, position, summonType, despawnMilliseconds);
        if (!summon)
            return nullptr;

        ApplySummonTierStats(summon, healthCoefficient, damageCoefficient, preserveStonedState);
        _riftSummons.push_back(summon->GetGUID());
        // 走石化唤醒流程的召唤物（如奥达曼石像守卫）在苏醒后才进入战斗，此处不提前拉入战斗。
        if (!preserveStonedState)
            summon->SetInCombatWithZone();
        return summon;
    }

    void ApplySummonTierStats(Creature* summon, float healthCoefficient = 1.0f, float damageCoefficient = 1.0f,
        bool preserveStonedState = false)
    {
        if (!summon || !_tierConfig)
            return;

        uint64 scaledHealth = uint64(std::max<uint32>(1, summon->GetMaxHealth())) *
            _tierConfig->HealthMultiplier * std::max(0.01f, healthCoefficient);
        scaledHealth = std::min<uint64>(scaledHealth, std::numeric_limits<uint32>::max());
        summon->SetCreateHealth(uint32(scaledHealth));
        summon->SetMaxHealth(uint32(scaledHealth));
        summon->SetFullHealth();

        float finalDamageMultiplier = _tierConfig->DamageMultiplier * std::max(0.01f, damageCoefficient);
        for (WeaponAttackType attackType : { BASE_ATTACK, OFF_ATTACK, RANGED_ATTACK })
        {
            float minDamage = summon->GetWeaponDamageRange(attackType, MINDAMAGE);
            float maxDamage = summon->GetWeaponDamageRange(attackType, MAXDAMAGE);
            summon->SetBaseWeaponDamage(attackType, MINDAMAGE, minDamage * finalDamageMultiplier);
            summon->SetBaseWeaponDamage(attackType, MAXDAMAGE, maxDamage * finalDamageMultiplier);
            summon->UpdateDamagePhysical(attackType);
        }

        // 召唤物是Boss战斗增援，必须立即处于可被攻击、可主动进攻的状态。
        // 部分源模板（如奥达曼的石像守卫）在 creature_template_addon 里自带石化光环(10255)，
        // 该光环会把单位置为被动(REACT_PASSIVE)并免疫一切伤害，而裂隙召唤物不走源模板的SmartAI来解除石化，
        // 这里显式清除石化并恢复可攻击状态（与 BossAIBase::Reset 的处理保持一致）。
        // 若调用方要求保留石化状态（奥达曼石像守卫复刻原版唤醒动画），则跳过此清理，由守卫AI自行苏醒。
        if (!preserveStonedState)
        {
            summon->RemoveAurasDueToSpell(10255);
            summon->SetStandState(UNIT_STAND_STATE_STAND);
            summon->SetReactState(REACT_AGGRESSIVE);
            summon->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC |
                UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE_2 | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE);
            summon->RemoveUnitFlag2(UNIT_FLAG2_FEIGN_DEATH | UNIT_FLAG2_HIDE_BODY);
        }

        summon->AI()->SetData(RiftDataTier, _tier);
        summon->AI()->SetData(RiftDataDamagePermille, uint32(finalDamageMultiplier * 1000.0f));

        // 召唤物名称与所属Boss保持一致，仅追加Tier后缀（如"源名 [T1]"），由运行时根据当前Tier动态生成。
        summon->SetName(Acore::StringFormat("{} [T{}]", summon->GetName(), uint32(_tier)));
    }

    void DespawnRiftSummons()
    {
        for (ObjectGuid const& guid : _riftSummons)
            if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                summon->DespawnOrUnsummon();
        _riftSummons.clear();
    }

    // 旧Boss脚本仍调用此接口，但Spell ID调谐后不再使用Boss级法术倍率。
    void SetRaidSpellDamageMultiplier(float /*multiplier*/) { }

    void AddInterruptImmuneSpell(uint32 spellId)
    {
        if (spellId && std::find(_interruptImmuneSpells.begin(), _interruptImmuneSpells.end(), spellId) == _interruptImmuneSpells.end())
            _interruptImmuneSpells.push_back(spellId);
    }

    uint32 GetDamageSpellId(Unit* victim, DamageEffectType damageType) const
    {
        if (damageType == DOT)
            if (uint32 spellId = GetTunedPeriodicSpellId(victim))
                return spellId;

        // OnSpellStart/OnSpellCast会记录实际产生本次伤害的法术，优先级应高于GetCurrentSpell()。
        // 触发型伤害命中时，当前法术槽可能仍保留上一次施法，进而错误选中其他调谐项。
        if (_lastCastSpellId)
            return _lastCastSpellId;

        return GetCurrentDamageSpellId();
    }

    uint32 GetCurrentDamageSpellId() const
    {
        for (CurrentSpellTypes spellType : { CURRENT_GENERIC_SPELL, CURRENT_CHANNELED_SPELL, CURRENT_MELEE_SPELL,
            CURRENT_AUTOREPEAT_SPELL })
            if (Spell const* spell = me->GetCurrentSpell(spellType))
                if (SpellInfo const* spellInfo = spell->GetSpellInfo())
                    return spellInfo->Id;

        return 0;
    }

    uint32 GetTunedPeriodicSpellId(Unit* victim) const
    {
        if (!victim)
            return 0;

        for (AuraEffect const* auraEffect : victim->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE))
            if (auraEffect && auraEffect->GetCasterGUID() == me->GetGUID())
                if (GetRiftSpellDamageTuning(auraEffect->GetSpellInfo()->Id))
                    return auraEffect->GetSpellInfo()->Id;

        return 0;
    }

    void RemoveInterruptImmunity(SpellInfo const* spell)
    {
        if (spell && std::find(_interruptImmuneSpells.begin(), _interruptImmuneSpells.end(), spell->Id) != _interruptImmuneSpells.end())
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, false);
    }

    uint32 _baseHealth = 1;
    uint32 _lastCastSpellId = 0;
    uint8 _tier = 0;
    TierConfig const* _tierConfig = nullptr;
    EventMap events;
    std::vector<ObjectGuid> _riftSummons;
    std::vector<uint32> _interruptImmuneSpells;
};

// 裂隙召唤生物（同伴和增援）的共享AI基类。
// 缩放规则与BossAIBase::ApplySummonTierStats保持一致：近战武器伤害已在单位属性中完成缩放，
// 法术伤害则先移除对应内容的基准系数，再应用Tier倍率和召唤物伤害系数。
class RiftSummonAI : public ScriptedAI
{
public:
    explicit RiftSummonAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _tier = 1;
        _spellDamagePermille = 1000 * GetSpellDamageBaseFactor();
        _lastCastSpellId = 0;
        _events.Reset();
        ScheduleAbilities();
    }

    void IsSummonedBy(WorldObject* summoner) override
    {
        if (Unit* unit = summoner->ToUnit())
            if (Unit* victim = unit->GetVictim())
                AttackStart(victim);
    }

    void OnSpellCast(SpellInfo const* spell) override
    {
        if (spell)
            _lastCastSpellId = spell->Id;
    }

    void OnSpellStart(SpellInfo const* spell) override
    {
        if (spell)
            _lastCastSpellId = spell->Id;
    }

    void SetData(uint32 type, uint32 data) override
    {
        if (type == RiftDataTier)
        {
            _tier = uint8(std::clamp<uint32>(data, 1, MaxTier));
            _events.Reset();
            ScheduleAbilities();
        }
        else if (type == RiftDataDamagePermille)
            _spellDamagePermille = std::max<uint32>(1, data) * GetSpellDamageBaseFactor();
    }

    void DamageDealt(Unit* victim, uint32& damage, DamageEffectType damageType, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (damageType == DIRECT_DAMAGE)
            return;

        uint32 spellId = GetDamageSpellId(victim, damageType);

        // 表中数值是T1基线；召唤物的Tier和damageCoefficient由ApplySummonTierStats传入，
        // 不再为未识别的触发子法术额外保留15倍基准。
        ScaleRiftSummonSpellDamage(spellId, damage, damageType, _spellDamagePermille, GetSpellDamageBaseFactor());
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
            ExecuteAbility(eventId);

        DoMeleeAttackIfReady();
    }

protected:
    virtual void ScheduleAbilities() { }
    virtual void ExecuteAbility(uint32 /*eventId*/) { }
    virtual uint32 GetSpellDamageBaseFactor() const { return 15; }

    SpellCastResult DoCast(Unit* target, uint32 spellId, bool triggered = false)
    {
        return CastIfConfigured(target, spellId, triggered);
    }

    SpellCastResult DoCastVictim(uint32 spellId, bool triggered = false)
    {
        return DoCast(me->GetVictim(), spellId, triggered);
    }

    SpellCastResult CastIfConfigured(Unit* target, uint32 spellId, bool triggered = false)
    {
        return CastRiftTunedSpell(me, target, spellId, triggered, &_lastCastSpellId);
    }

    uint32 TierDelay(uint32 tier1Delay, uint32 tier2Delay, uint32 tier3Delay) const
    {
        return _tier == 1 ? tier1Delay : (_tier == 2 ? tier2Delay : tier3Delay);
    }

protected:
    uint32 GetDamageSpellId(Unit* victim, DamageEffectType damageType) const
    {
        if (damageType == DOT)
            if (uint32 spellId = GetTunedPeriodicSpellId(victim))
                return spellId;

        // OnSpellStart/OnSpellCast会记录实际产生本次伤害的法术，优先级应高于GetCurrentSpell()。
        // 触发型伤害命中时，当前法术槽可能仍保留上一次施法，进而错误选中其他调谐项。
        if (_lastCastSpellId)
            return _lastCastSpellId;

        return GetCurrentDamageSpellId();
    }

    uint32 GetCurrentDamageSpellId() const
    {
        for (CurrentSpellTypes spellType : { CURRENT_GENERIC_SPELL, CURRENT_CHANNELED_SPELL, CURRENT_MELEE_SPELL,
            CURRENT_AUTOREPEAT_SPELL })
            if (Spell const* spell = me->GetCurrentSpell(spellType))
                if (SpellInfo const* spellInfo = spell->GetSpellInfo())
                    return spellInfo->Id;

        return 0;
    }

    uint32 GetTunedPeriodicSpellId(Unit* victim) const
    {
        if (!victim)
            return 0;

        for (AuraEffect const* auraEffect : victim->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE))
            if (auraEffect && auraEffect->GetCasterGUID() == me->GetGUID())
                if (GetRiftSpellDamageTuning(auraEffect->GetSpellInfo()->Id))
                    return auraEffect->GetSpellInfo()->Id;

        return 0;
    }

    EventMap _events;
    uint8 _tier = 1;
    uint32 _spellDamagePermille = 15000;
    uint32 _lastCastSpellId = 0;
};

// 70级副本法术已经包含TBC英雄难度伤害，只应用配置的Tier倍率和伤害系数，
// 不使用RiftSummonAI为既有60级遭遇保留的经典内容15倍基准修正。
class RiftLevel70SummonAI : public RiftSummonAI
{
public:
    explicit RiftLevel70SummonAI(Creature* creature) : RiftSummonAI(creature) { }

protected:
    uint32 GetSpellDamageBaseFactor() const override { return 1; }
};

} // namespace HeroicDungeonRift

#endif
