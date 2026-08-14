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

#include "ScriptedCreature.h"
#include "SpellInfo.h"
#include "ThreatManager.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace HeroicDungeonRift
{
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
    void OnSpellStart(SpellInfo const* spell) override
    {
        if (spell && std::find(_interruptImmuneSpells.begin(), _interruptImmuneSpells.end(), spell->Id) != _interruptImmuneSpells.end())
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, true);
    }

    void OnSpellCast(SpellInfo const* spell) override
    {
        RemoveInterruptImmunity(spell);
    }

    void OnSpellFailed(SpellInfo const* spell) override
    {
        RemoveInterruptImmunity(spell);
    }

    void DamageDealt(Unit* /*victim*/, uint32& damage, DamageEffectType damageType, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_tierConfig)
        {
            double multiplier = _tierConfig->DamageMultiplier;
            if (damageType != DIRECT_DAMAGE)
                multiplier *= _raidSpellDamageMultiplier;
            damage = uint32(std::min<double>(double(damage) * multiplier, std::numeric_limits<uint32>::max()));
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        DespawnRiftSummons();
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

    bool CastIfConfigured(Unit* target, uint32 spellId, bool triggered = false)
    {
        if (!target || !spellId)
            return false;
        DoCast(target, spellId, triggered);
        return true;
    }

    bool CastRaidTunedSpell(Unit* target, uint32 spellId, int32 basePoint0, bool triggered = false)
    {
        if (!target || !spellId || basePoint0 <= 0)
            return false;
        me->CastCustomSpell(spellId, SPELLVALUE_BASE_POINT0, basePoint0, target, triggered);
        return true;
    }

    // T2/T3新增技能按Spell_dbc的具体效果单独给出T1基准伤害。
    // 这里只反算Boss原版法术倍率；运行时Tier倍率继续由DamageDealt统一应用。
    bool CastFinalRaidDamageSpell(Unit* target, uint32 spellId, SpellValueMod damageEffect,
        int32 tier1BaseDamage, bool triggered = false)
    {
        if (!target || !spellId || tier1BaseDamage <= 0)
            return false;

        me->CastCustomSpell(spellId, damageEffect, GetUnscaledRaidDamage(tier1BaseDamage), target, triggered);
        return true;
    }

    bool CastFinalRaidDamageSpell(Unit* target, uint32 spellId, int32 tier1BaseDamage0,
        int32 tier1BaseDamage1, bool triggered = false)
    {
        if (!target || !spellId || tier1BaseDamage0 <= 0 || tier1BaseDamage1 <= 0)
            return false;

        CustomSpellValues values;
        values.AddSpellMod(SPELLVALUE_BASE_POINT0, GetUnscaledRaidDamage(tier1BaseDamage0));
        values.AddSpellMod(SPELLVALUE_BASE_POINT1, GetUnscaledRaidDamage(tier1BaseDamage1));
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
        uint32 despawnMilliseconds = 10 * IN_MILLISECONDS)
    {
        if (!entry || !_tierConfig)
            return nullptr;

        Creature* summon = me->SummonCreature(entry, position, summonType, despawnMilliseconds);
        if (!summon)
            return nullptr;

        ApplySummonTierStats(summon, healthCoefficient, damageCoefficient);
        _riftSummons.push_back(summon->GetGUID());
        summon->SetInCombatWithZone();
        return summon;
    }

    void ApplySummonTierStats(Creature* summon, float healthCoefficient = 1.0f, float damageCoefficient = 1.0f)
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

        summon->AI()->SetData(RiftDataTier, _tier);
        summon->AI()->SetData(RiftDataDamagePermille, uint32(finalDamageMultiplier * 1000.0f));
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

    int32 GetUnscaledRaidDamage(int32 tier1BaseDamage) const
    {
        return std::max<int32>(1, int32(std::lround(
            double(tier1BaseDamage) / std::max(1.0f, _raidSpellDamageMultiplier))));
    }

    void AddInterruptImmuneSpell(uint32 spellId)
    {
        if (spellId && std::find(_interruptImmuneSpells.begin(), _interruptImmuneSpells.end(), spellId) == _interruptImmuneSpells.end())
            _interruptImmuneSpells.push_back(spellId);
    }

    void RemoveInterruptImmunity(SpellInfo const* spell)
    {
        if (spell && std::find(_interruptImmuneSpells.begin(), _interruptImmuneSpells.end(), spell->Id) != _interruptImmuneSpells.end())
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, false);
    }

    uint32 _baseHealth = 1;
    float _raidSpellDamageMultiplier = 1.0f;
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
        _events.Reset();
        ScheduleAbilities();
    }

    void IsSummonedBy(WorldObject* summoner) override
    {
        if (Unit* unit = summoner->ToUnit())
            if (Unit* victim = unit->GetVictim())
                AttackStart(victim);
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

    void DamageDealt(Unit* /*victim*/, uint32& damage, DamageEffectType damageType, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (damageType == DIRECT_DAMAGE)
            return;

        uint64 scaledDamage = uint64(damage) * _spellDamagePermille / 1000;
        damage = uint32(std::min<uint64>(scaledDamage, std::numeric_limits<uint32>::max()));
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

    uint32 TierDelay(uint32 tier1Delay, uint32 tier2Delay, uint32 tier3Delay) const
    {
        return _tier == 1 ? tier1Delay : (_tier == 2 ? tier2Delay : tier3Delay);
    }

    EventMap _events;
    uint8 _tier = 1;
    uint32 _spellDamagePermille = 15000;
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
