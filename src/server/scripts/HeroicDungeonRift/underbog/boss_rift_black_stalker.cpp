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
// 幽暗沼泽 - 黑色阔步者（The Black Stalker）
constexpr uint32 RiftEntrySporeStrider = 102042; // 原型：孢子漫游者 22299
constexpr int32 LightningCloudRaidDirectDamage = 3000;
constexpr int32 LightningCloudRaidDamagePerTick = 1800;
constexpr int32 ThunderclapRaidDamage = 3500;

enum BlackStalkerEvents : uint32
{
    EventLevitate = 1,   // 漂浮术（T1基础）
    EventChainLightning, // 闪电链（T1基础）
    EventStaticCharge,   // 静电充能（T1基础）
    EventSporeStrider,   // 召唤孢子漫游者（T1英雄机制）
    EventLightningCloud, // 闪电云（T2新增）
    EventThunderclap     // 雷霆一击（T3新增）
};

enum SporeStriderEvents : uint32
{
    EventStriderLightningBolt = 1
};

enum Spells : uint32
{
    SpellLevitate = 31704,        // 漂浮术，保留原版完整触发链
    SpellChainLightning = 31717,  // 闪电链
    SpellStaticCharge = 31715,    // 静电充能
    SpellLightningBolt = 20824,   // 孢子漫游者闪电箭
    SpellLightningCloud = 6535,   // 闪电云
    SpellThunderclap = 15588      // 雷霆一击
};
}

struct npc_rift_spore_strider : public RiftLevel70SummonAI // 裂隙孢子漫游者
{
    explicit npc_rift_spore_strider(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void ScheduleAbilities() override
    {
        // 孢子漫游者属于T1英雄机制，所有Tier保持原版施法节奏。
        _events.ScheduleEvent(EventStriderLightningBolt, 2s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventStriderLightningBolt)
            return;

        DoCastVictim(SpellLightningBolt);
        _events.ScheduleEvent(EventStriderLightningBolt, 5500ms);
    }
};

struct boss_rift_black_stalker : public BossAIBase
{
    explicit boss_rift_black_stalker(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        // T1 原版技能在所有 Tier 均保持原版首次施放窗口与 CD。
        events.ScheduleEvent(EventLevitate, Milliseconds(urand(8000, 12000)));
        events.ScheduleEvent(EventChainLightning, 6s);
        events.ScheduleEvent(EventStaticCharge, 10s);
        events.ScheduleEvent(EventSporeStrider, Milliseconds(urand(10000, 15000)));
        if (_tier >= 2)
            events.ScheduleEvent(EventLightningCloud, 9s);
        if (_tier >= 3)
            events.ScheduleEvent(EventThunderclap, 13s);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventLevitate:
                CastIfConfigured(me, SpellLevitate);
                events.ScheduleEvent(EventLevitate, Milliseconds(urand(18000, 24000)));
                break;
            case EventChainLightning:
                CastIfConfigured(SelectRandomPlayer(), SpellChainLightning);
                events.ScheduleEvent(EventChainLightning, 9s);
                break;
            case EventStaticCharge:
                CastIfConfigured(SelectRandomPlayer(), SpellStaticCharge);
                events.ScheduleEvent(EventStaticCharge, 10s);
                break;
            case EventSporeStrider:
                SummonTieredCreature(RiftEntrySporeStrider, me->GetRandomNearPosition(7.0f), 0.45f, 0.65f);
                events.ScheduleEvent(EventSporeStrider, Milliseconds(urand(10000, 15000)));
                break;
            case EventLightningCloud: // T2新增：随机目标脚下闪电云
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellLightningCloud,
                    LightningCloudRaidDirectDamage, LightningCloudRaidDamagePerTick, true);
                events.ScheduleEvent(EventLightningCloud, _tier == 3 ? 10s : 13s);
                break;
            case EventThunderclap: // T3新增：近战范围自然伤害与减速
                CastFinalRaidDamageSpell(me, SpellThunderclap, SPELLVALUE_BASE_POINT0,
                    ThunderclapRaidDamage, true);
                events.ScheduleEvent(EventThunderclap, 15s);
                break;
            default:
                break;
        }
    }
};

// 31716 - 静电充能的跳跃伤害
// 31715的第三个效果是每1秒触发的PERIODIC_TRIGGER_SPELL，核心在SpellAuraEffects.cpp里因31716
// 不需要显式单位目标而改用"光环承受者"（被点名的玩家）施放该子法术，于是这份伤害既不过
// BossAIBase::DamageDealt（没有Tier倍率），也不受核心的非玩家施法等级缩放，恒为DBC原始值。
// 这里在命中时按31715光环施法者（裂隙Boss）的Tier重写伤害，使其与主DoT保持同一T1基线与倍率。
class spell_rift_black_stalker_static_charge : public SpellScript
{
    PrepareSpellScript(spell_rift_black_stalker_static_charge);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SpellStaticCharge });
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        RiftSpellDamageTuning const* tuning = GetRiftSpellDamageTuning(SpellStaticCharge);
        if (!caster || !tuning)
            return;

        Aura* charge = caster->GetAura(SpellStaticCharge);
        Creature* boss = charge && charge->GetCaster() ? charge->GetCaster()->ToCreature() : nullptr;
        TierConfig const* tierConfig = boss ? GetTierConfigForCreature(boss) : nullptr;
        if (!tierConfig)
            return; // 非裂隙Boss（例如原版幽暗沼泽的黑色潜猎者）保留DBC原始伤害

        int32 damage = int32(std::lround(double(tuning->EffectBasePoints[0]) * double(tierConfig->DamageMultiplier)));
        SetEffectValue(std::max<int32>(1, damage));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_rift_black_stalker_static_charge::HandleDamage,
            EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

void AddSC_boss_rift_black_stalker()
{
    RegisterCreatureAI(boss_rift_black_stalker);
    RegisterCreatureAI(npc_rift_spore_strider);
    RegisterSpellScript(spell_rift_black_stalker_static_charge);
}

} // namespace HeroicDungeonRift
