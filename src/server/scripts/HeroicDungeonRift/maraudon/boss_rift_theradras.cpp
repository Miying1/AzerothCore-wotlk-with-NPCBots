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
// 玛拉顿 - 瑟莱德丝公主（Princess Theradras）
enum Events : uint32
{
    EventBoulder = 1,      // 投石（原版/T1基础）
    EventDustField,        // 灰尘力场（原版/T1基础）
    EventRepulsiveGaze,    // 憎恨怒视（T2新增）
    EventTier3Skill        // 击退（T3新增）
};

enum Spells : uint32
{
    SpellThrash = 8876,         // 痛击（原版/T1基础）
    SpellBoulder = 21832,       // 投石（原版/T1基础）
    SpellDustField = 21909,     // 灰尘力场（原版/T1基础）
    SpellDustFieldDamage = 21868, // 灰尘力场每跳触发的伤害子法术
    SpellRepulsiveGaze = 21869, // 憎恨怒视（T2新增）
    SpellKnockAway = 10101      // 击退（T3新增）
};

// 灰尘力场每跳T1基准伤害；最终伤害 = 该基准 × 当前Tier倍率（由BossAIBase::DamageDealt统一应用）。
constexpr int32 DustFieldTier1DamagePerTick = 1700;
}

// 灰尘力场(21909)是挂在Boss身上的自我光环：每1秒通过PERIODIC_TRIGGER_SPELL触发子法术21868
// （以Boss为中心的范围伤害+击退）。覆盖父光环的BP无效，必须在每跳里改写子法术的BP0；
// 伤害仍由Boss施放，因此继续经过BossAIBase::DamageDealt乘Tier倍率。
// 仅对裂隙Boss生效，普通生物（如斯莱格、原版瑟莱德丝）保留DBC默认行为。
class spell_rift_theradras_dust_field : public AuraScript
{
    PrepareAuraScript(spell_rift_theradras_dust_field);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SpellDustFieldDamage });
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        Creature* caster = GetCaster() ? GetCaster()->ToCreature() : nullptr;
        if (!caster || !GetTierForCreature(caster))
            return;

        PreventDefaultAction();
        int32 damage = CompensateRiftCreatureLevelScaling(caster, SpellDustFieldDamage, EFFECT_0,
            DustFieldTier1DamagePerTick);
        caster->CastCustomSpell(SpellDustFieldDamage, SPELLVALUE_BASE_POINT0, damage, caster,
            TRIGGERED_FULL_MASK, nullptr, aurEff);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_rift_theradras_dust_field::HandlePeriodic,
            EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

struct boss_rift_theradras : public BossAIBase
{
    explicit boss_rift_theradras(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        CastIfConfigured(me, SpellThrash, true);
        events.ScheduleEvent(EventBoulder, 5000ms);
        events.ScheduleEvent(EventDustField, 15000ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventRepulsiveGaze, 10s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 18s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventBoulder:
                CastIfConfigured(SelectRandomPlayer(), SpellBoulder);
                events.ScheduleEvent(EventBoulder, 17000ms);
                break;
            case EventDustField:
                CastIfConfigured(me, SpellDustField);
                events.ScheduleEvent(EventDustField, 30000ms);
                break;
            case EventRepulsiveGaze: // T2新增：憎恨怒视，瞬发
                CastIfConfigured(me, SpellRepulsiveGaze, true);
                // 恐惧冷却：T2 40秒，T3 35秒。
                events.ScheduleEvent(EventRepulsiveGaze, _tier == 3 ? 35s : 40s);
                break;
            case EventTier3Skill: // T3新增：击退，瞬发
                CastIfConfigured(me->GetVictim(), SpellKnockAway, true);
                events.ScheduleEvent(EventTier3Skill, 14s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_theradras()
{
    RegisterCreatureAI(boss_rift_theradras);
    RegisterSpellScript(spell_rift_theradras_dust_field);
}

} // namespace HeroicDungeonRift
