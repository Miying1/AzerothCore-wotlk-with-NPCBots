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
// 奴隶围栏 - 背叛者门努（Mennu the Betrayer）
constexpr uint32 RiftEntryMennuHealingWard = 102043; // 原型：门努的治疗图腾 20208
constexpr uint32 RiftEntryMennuEarthgrabTotem = 102044;   // 原型：腐蚀地缚图腾 18176
constexpr uint32 RiftEntryStoneskinTotem = 102045;   // 原型：腐蚀石肤图腾 18177
constexpr uint32 RiftEntryNovaTotem = 102046;        // 原型：堕落新星图腾 18179
constexpr int32 ChainLightningRaidDamage = 4500;
constexpr int32 FrostShockRaidDamage = 3500;

enum MennuEvents : uint32
{
    EventLightningBolt = 1, // 闪电箭（T1基础）
    EventHealingWard,       // 治疗图腾（T1基础）
    EventNovaTotem,         // 新星图腾（T1基础）
    EventEarthgrabTotem,    // 地缚图腾（T1基础）
    EventStoneskinTotem,    // 石肤图腾（T1基础）
    EventChainLightning,    // 闪电链（T2新增）
    EventFrostShock         // 冰霜震击（T3新增）
};

enum TotemEvents : uint32
{
    EventHealingPulse = 1,
    EventEarthgrabPulse,
    EventStoneskinPulse,
    EventNovaExplosion
};

enum Spells : uint32
{
    SpellLightningBolt = 35010,       // 闪电箭
    SpellHealingWardPassive = 34978,  // 门努的治疗图腾被动
    SpellEarthgrab = 31983,           // 陷地
    SpellStoneskin = 31986,           // 石肤术
    SpellFireNova = 33132,            // 火焰新星
    SpellChainLightning = 16006,      // 闪电链
    SpellFrostShock = 15499           // 冰霜震击
};

constexpr char const* MennuAggroText = "工作必须继续！";
constexpr char const* MennuSlayText = "这件事必须有人来做……";
constexpr char const* MennuDeathText = "这是……我应得的……";
}

struct npc_rift_mennu_healing_ward : public RiftLevel70SummonAI // 裂隙门努治疗图腾
{
    explicit npc_rift_mennu_healing_ward(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();
        DoCast(me, SpellHealingWardPassive, true);
    }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventHealingPulse, 1s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventHealingPulse)
            return;

        if (!me->HasAura(SpellHealingWardPassive))
            DoCast(me, SpellHealingWardPassive, true);
        _events.ScheduleEvent(EventHealingPulse, 2s);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        while (uint32 eventId = _events.ExecuteEvent())
            ExecuteAbility(eventId);
    }
};

struct npc_rift_mennu_earthgrab_totem : public RiftLevel70SummonAI // 裂隙地缚图腾
{
    explicit npc_rift_mennu_earthgrab_totem(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();
    }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventEarthgrabPulse, 1s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventEarthgrabPulse)
            return;

        DoCast(me, SpellEarthgrab, true);
        _events.ScheduleEvent(EventEarthgrabPulse, 5s);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        while (uint32 eventId = _events.ExecuteEvent())
            ExecuteAbility(eventId);
    }
};

struct npc_rift_mennu_stoneskin_totem : public RiftLevel70SummonAI // 裂隙石肤图腾
{
    explicit npc_rift_mennu_stoneskin_totem(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();
        DoCast(me, SpellStoneskin, true);
    }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventStoneskinPulse, 1s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventStoneskinPulse)
            return;

        if (!me->HasAura(SpellStoneskin))
            DoCast(me, SpellStoneskin, true);
        _events.ScheduleEvent(EventStoneskinPulse, 5s);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        while (uint32 eventId = _events.ExecuteEvent())
            ExecuteAbility(eventId);
    }
};

struct npc_rift_mennu_nova_totem : public RiftLevel70SummonAI // 裂隙新星图腾
{
    explicit npc_rift_mennu_nova_totem(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();
    }

    void ScheduleAbilities() override
    {
        // 门努图腾均为T1原版机制，Tier只缩放属性，不加速脉冲或爆炸。
        _events.ScheduleEvent(EventNovaExplosion, 5s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventNovaExplosion)
            return;

        DoCast(me, SpellFireNova, true);
        me->DespawnOrUnsummon(500ms);
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        while (uint32 eventId = _events.ExecuteEvent())
            ExecuteAbility(eventId);
    }
};

struct boss_rift_mennu : public BossAIBase
{
    explicit boss_rift_mennu(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _healingWardSummoned = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(MennuAggroText, LANG_UNIVERSAL);
        // T1 原版技能在所有 Tier 均保持原版首次施放窗口与 CD。
        events.ScheduleEvent(EventLightningBolt, Milliseconds(urand(5000, 8000)));
        events.ScheduleEvent(EventNovaTotem, 20s);
        events.ScheduleEvent(EventEarthgrabTotem, 19200ms);
        events.ScheduleEvent(EventStoneskinTotem, 18s);
        if (_tier >= 2)
            events.ScheduleEvent(EventChainLightning, 9s);
        if (_tier >= 3)
            events.ScheduleEvent(EventFrostShock, 12s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_healingWardSummoned || !me->HealthBelowPctDamaged(60, damage))
            return;

        _healingWardSummoned = true;
        events.ScheduleEvent(EventHealingWard, 1ms);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
            me->Yell(MennuSlayText, LANG_UNIVERSAL, victim);
    }

    void JustDied(Unit* killer) override
    {
        me->Yell(MennuDeathText, LANG_UNIVERSAL);
        BossAIBase::JustDied(killer);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventLightningBolt:
                CastIfConfigured(me->GetVictim(), SpellLightningBolt);
                events.ScheduleEvent(EventLightningBolt, Milliseconds(urand(7000, 10000)));
                break;
            case EventHealingWard:
                SummonTotem(RiftEntryMennuHealingWard, 0.3f, 20 * IN_MILLISECONDS);
                break;
            case EventNovaTotem:
                SummonTotem(RiftEntryNovaTotem, 0.2f, 8 * IN_MILLISECONDS);
                events.ScheduleEvent(EventNovaTotem, 26s);
                break;
            case EventEarthgrabTotem:
                SummonTotem(RiftEntryMennuEarthgrabTotem, 0.25f, 20 * IN_MILLISECONDS);
                events.ScheduleEvent(EventEarthgrabTotem, 26s);
                break;
            case EventStoneskinTotem:
                SummonTotem(RiftEntryStoneskinTotem, 0.25f, 20 * IN_MILLISECONDS);
                events.ScheduleEvent(EventStoneskinTotem, 26s);
                break;
            case EventChainLightning: // T2新增：随机目标闪电链
                CastFinalRaidDamageSpell(SelectRandomPlayer(40.0f), SpellChainLightning, SPELLVALUE_BASE_POINT0,
                    ChainLightningRaidDamage, true);
                events.ScheduleEvent(EventChainLightning, _tier == 3 ? 10s : 13s);
                break;
            case EventFrostShock: // T3新增：限制随机远程目标移动
                CastFinalRaidDamageSpell(SelectRandomPlayer(40.0f), SpellFrostShock, SPELLVALUE_BASE_POINT1,
                    FrostShockRaidDamage, true);
                events.ScheduleEvent(EventFrostShock, 11s);
                break;
            default:
                break;
        }
    }

private:
    void SummonTotem(uint32 entry, float healthCoefficient, uint32 lifetime)
    {
        SummonTieredCreature(entry, me->GetRandomNearPosition(4.0f), healthCoefficient, 0.5f,
            TEMPSUMMON_TIMED_DESPAWN, lifetime);
    }

    bool _healingWardSummoned = false;
};

void AddSC_boss_rift_mennu()
{
    RegisterCreatureAI(boss_rift_mennu);
    RegisterCreatureAI(npc_rift_mennu_healing_ward);
    RegisterCreatureAI(npc_rift_mennu_earthgrab_totem);
    RegisterCreatureAI(npc_rift_mennu_stoneskin_totem);
    RegisterCreatureAI(npc_rift_mennu_nova_totem);
}

} // namespace HeroicDungeonRift
