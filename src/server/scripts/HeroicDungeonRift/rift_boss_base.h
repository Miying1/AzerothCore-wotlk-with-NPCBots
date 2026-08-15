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

#include "ScriptedCreature.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "StringFormat.h"
#include "ThreatManager.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace HeroicDungeonRift
{
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
                values.AddSpellMod(SpellValueMod(SPELLVALUE_BASE_POINT0 + effectIndex), basePoint);

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

    double multiplier = double(spellDamagePermille) / 1000.0;
    if (RiftSpellDamageTuning const* tuning = GetRiftSpellDamageTuning(spellId))
    {
        multiplier /= spellDamageBaseFactor;
        if (tuning->WeaponDamageMultiplier > 1.0f)
            multiplier *= tuning->WeaponDamageMultiplier;
    }

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
        _raidSpellDamageMultiplier = 1.0f;
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
            uint32 spellId = damageType == DOT ? GetTunedPeriodicSpellId(victim) : GetCurrentDamageSpellId();
            if (!spellId)
                spellId = GetCurrentDamageSpellId();
            if (!spellId)
                spellId = _lastCastSpellId;

            if (RiftSpellDamageTuning const* tuning = GetRiftSpellDamageTuning(spellId))
            {
                // 固定伤害/DoT在施法时已写入T1基线；武器技能则保留独立武器倍率。
                if (tuning->WeaponDamageMultiplier > 1.0f)
                    multiplier *= tuning->WeaponDamageMultiplier;
            }
            else
                // 未识别的触发子法术保留旧Boss级倍率作为兼容兜底。
                multiplier *= _raidSpellDamageMultiplier;
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
        me->CastCustomSpell(spellId, SPELLVALUE_BASE_POINT0, basePoint0, target, triggered);
        return true;
    }

    // T2/T3新增技能按Spell_dbc的具体效果单独给出T1基准伤害。
    // 运行时Tier倍率继续由DamageDealt统一应用。
    bool CastFinalRaidDamageSpell(Unit* target, uint32 spellId, SpellValueMod damageEffect,
        int32 tier1BaseDamage, bool triggered = false)
    {
        if (!target || !spellId || tier1BaseDamage <= 0)
            return false;

        _lastCastSpellId = spellId;
        me->CastCustomSpell(spellId, damageEffect, tier1BaseDamage, target, triggered);
        return true;
    }

    bool CastFinalRaidDamageSpell(Unit* target, uint32 spellId, int32 tier1BaseDamage0,
        int32 tier1BaseDamage1, bool triggered = false)
    {
        if (!target || !spellId || tier1BaseDamage0 <= 0 || tier1BaseDamage1 <= 0)
            return false;

        _lastCastSpellId = spellId;
        CustomSpellValues values;
        values.AddSpellMod(SPELLVALUE_BASE_POINT0, tier1BaseDamage0);
        values.AddSpellMod(SPELLVALUE_BASE_POINT1, tier1BaseDamage1);
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

    void SetRaidSpellDamageMultiplier(float multiplier)
    {
        _raidSpellDamageMultiplier = std::max(1.0f, multiplier);
    }

    void AddInterruptImmuneSpell(uint32 spellId)
    {
        if (spellId && std::find(_interruptImmuneSpells.begin(), _interruptImmuneSpells.end(), spellId) == _interruptImmuneSpells.end())
            _interruptImmuneSpells.push_back(spellId);
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
    float _raidSpellDamageMultiplier = 1.0f;
    uint32 _lastCastSpellId = 0;
    uint8 _tier = 0;
    TierConfig const* _tierConfig = nullptr;
    EventMap events;
    std::vector<ObjectGuid> _riftSummons;
    std::vector<uint32> _interruptImmuneSpells;
};

// Shared AI base for rift-summoned creatures (companions and adds). It mirrors the
// scaling contract used by BossAIBase::ApplySummonTierStats: melee weapon damage is
// already rescaled on the instance attributes, while spell damage applies the value
// received through RiftDataDamagePermille on top of a content-specific base factor.
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

        uint32 spellId = damageType == DOT ? GetTunedPeriodicSpellId(victim) : GetCurrentDamageSpellId();
        if (!spellId)
            spellId = _lastCastSpellId;

        // 表中数值是T1基线；召唤物的Tier和damageCoefficient由ApplySummonTierStats传入，
        // 经典副本保留旧的15倍法术基准仅用于未纳入独立表的触发子法术。
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

// Level-70 dungeon spells already carry TBC heroic damage values. They must only
// receive the configured tier/damage coefficient, without the classic-content 15x
// base correction retained by RiftSummonAI for the existing level-60 encounters.
class RiftLevel70SummonAI : public RiftSummonAI
{
public:
    explicit RiftLevel70SummonAI(Creature* creature) : RiftSummonAI(creature) { }

protected:
    uint32 GetSpellDamageBaseFactor() const override { return 1; }
};

} // namespace HeroicDungeonRift

#endif
