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
// 禁魔监狱 - 天怒预言者苏克拉底（Wrath-Scryer Soccothrates）
enum Events : uint32
{
    EventFelfireShock = 1, // 魔火震击（T1基础）
    EventKnockAway,       // 击退（T1基础）
    EventCharge,          // 冲锋（T1基础，击退后施放）
    EventFelfireTrail,    // 魔火路径（T1基础）
    EventShadowPower,     // 暗影能量（T2新增）
    EventShadowfury       // 暗影之怒（T3新增）
};

enum Spells : uint32
{
    SpellFelImmolation = 36051,       // 邪能献祭（T1基础，自身父Aura）
    SpellFelImmolationDamage = 35959, // 邪能献祭父Aura每3秒触发的范围火焰伤害
    SpellFelfireShock = 35759,
    SpellKnockAway = 36512,
    SpellFelfire = 35769,             // 邪火（T1基础，冲锋路径上的地面区域光环）
    SpellFelfireDamage = 35767,       // 邪火区域光环每1秒触发的地面火焰伤害
    SpellCharge = 35754,
    SpellShadowPower = 35322,
    SpellShadowfury = 39082
};

constexpr int32 ShadowfuryRaidDamage = 3500;

// 父Aura不会把自身效果基础点传给触发的子法术，必须在这里按T1基线补写子法术基础点。
// 邪能献祭每3秒一跳（Boss周围范围），取1500；地面邪火每1秒一跳且可躲避，取1000。
// 两者数值需与 rift_spell_damage.h 中 35959/35767 的表项保持一致。
constexpr int32 FelImmolationTier1DamagePerTick = 1500;
constexpr int32 FelfireTier1DamagePerTick = 1000;

// 原版中文喊话与语音；裂隙版本去掉战前对话，生成后即可直接攻击。
constexpr char const* SoccothratesAggroText = "终于有个发泄怒气的目标了！";
constexpr char const* SoccothratesSlayText = "啊，真令人满足。";
constexpr char const* SoccothratesKnockAwayText = "看招！";
constexpr char const* SoccothratesDeathText = "我就知道……这是唯一的结局。";
constexpr uint32 SoccothratesAggroSound = 11238;
constexpr uint32 SoccothratesSlaySound = 11239;
constexpr uint32 SoccothratesKnockAwaySound = 11241;
constexpr uint32 SoccothratesDeathSound = 11243;

// 父Aura触发子法术的统一入口：保持引擎原有的施法者/目标语义，只把子法术基础点换成T1基线。
// 引擎按NeedsToBeTriggeredByCaster决定子法术由谁施放；当光环承载者自行施放时（如地面邪火），
// 伤害不经过Boss的DamageDealt，这里补上Tier倍率；由Boss施放时交给DamageDealt乘算，避免重复放大。
void CastTieredAuraTriggerSpell(Creature* caster, AuraEffect const* aurEff, uint32 childSpellId,
    Unit* target, int32 tier1BasePoint)
{
    SpellInfo const* childInfo = sSpellMgr->GetSpellInfo(childSpellId);
    if (!childInfo)
        return;

    int32 damage = CompensateRiftCreatureLevelScaling(caster, childSpellId, EFFECT_0, tier1BasePoint);
    Unit* triggerCaster = childInfo->NeedsToBeTriggeredByCaster(aurEff->GetSpellInfo(), aurEff->GetEffIndex())
        ? caster : target;
    if (!triggerCaster)
        return;

    if (triggerCaster != caster)
        if (TierConfig const* tierConfig = GetTierConfigForCreature(caster))
            damage = int32(damage * tierConfig->DamageMultiplier);

    triggerCaster->CastCustomSpell(childSpellId, SPELLVALUE_BASE_POINT0, damage, target,
        TRIGGERED_FULL_MASK, nullptr, aurEff);
}

// 邪能献祭（36051）：Boss常驻光环，每3秒对周围敌人造成35959火焰伤害。
class spell_rift_soccothrates_fel_immolation : public AuraScript
{
    PrepareAuraScript(spell_rift_soccothrates_fel_immolation);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SpellFelImmolationDamage });
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        Creature* caster = GetCaster() ? GetCaster()->ToCreature() : nullptr;
        Unit* target = GetTarget();
        // 原版苏克拉底同样施放36051，非裂隙生物保留DBC原始行为，不接管。
        if (!caster || !target || !GetTierForCreature(caster))
            return;

        PreventDefaultAction();
        CastTieredAuraTriggerSpell(caster, aurEff, SpellFelImmolationDamage, target,
            FelImmolationTier1DamagePerTick);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_rift_soccothrates_fel_immolation::HandlePeriodic,
            EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

// 邪火（35769）：冲锋路径上的地面区域光环，每1秒对范围内敌人造成35767火焰伤害。
class spell_rift_soccothrates_felfire : public AuraScript
{
    PrepareAuraScript(spell_rift_soccothrates_felfire);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SpellFelfireDamage });
    }

    void HandlePeriodic(AuraEffect const* aurEff)
    {
        Creature* caster = GetCaster() ? GetCaster()->ToCreature() : nullptr;
        Unit* target = GetTarget();
        if (!caster || !target || !GetTierForCreature(caster))
            return;

        PreventDefaultAction();
        CastTieredAuraTriggerSpell(caster, aurEff, SpellFelfireDamage, target, FelfireTier1DamagePerTick);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_rift_soccothrates_felfire::HandlePeriodic,
            EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};
}

struct boss_rift_soccothrates : public BossAIBase
{
    explicit boss_rift_soccothrates(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _felfireCount = 0;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(SoccothratesAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(SoccothratesAggroSound);

        events.ScheduleEvent(EventFelfireShock, Milliseconds(urand(8500, 22000)));
        events.ScheduleEvent(EventKnockAway, Milliseconds(urand(30000, 35000)));
        if (_tier >= 2)
            events.ScheduleEvent(EventShadowPower, 11s);
        if (_tier >= 3)
            events.ScheduleEvent(EventShadowfury, 18s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
        {
            me->Yell(SoccothratesSlayText, LANG_UNIVERSAL, victim);
            me->PlayDirectSound(SoccothratesSlaySound, victim->ToPlayer());
        }
    }

    void JustDied(Unit* killer) override
    {
        BossAIBase::JustDied(killer);
        me->Yell(SoccothratesDeathText, LANG_UNIVERSAL);
        me->PlayDirectSound(SoccothratesDeathSound);
    }

    void ConfigureTier() override
    {
        SetRaidSpellDamageMultiplier(2.5f);
        CastIfConfigured(me, SpellFelImmolation, true);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventFelfireShock:
                CastIfConfigured(me->GetVictim(), SpellFelfireShock);
                events.ScheduleEvent(EventFelfireShock, Milliseconds(urand(8500, 22000)));
                break;
            case EventKnockAway:
                me->Yell(SoccothratesKnockAwayText, LANG_UNIVERSAL);
                me->PlayDirectSound(SoccothratesKnockAwaySound);
                me->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
                CastIfConfigured(me, SpellKnockAway);
                events.ScheduleEvent(EventCharge, 4600ms);
                events.ScheduleEvent(EventKnockAway, Milliseconds(urand(20000, 35000)));
                break;
            case EventCharge:
                CastIfConfigured(SelectRandomPlayer(), SpellCharge);
                events.ScheduleEvent(EventFelfireTrail, 300ms);
                break;
            case EventFelfireTrail:
                CastIfConfigured(me, SpellFelfire, true);
                if (++_felfireCount < 7)
                    events.ScheduleEvent(EventFelfireTrail, 300ms);
                else
                    _felfireCount = 0;
                break;
            case EventShadowPower: // T2新增：能源舰暗影能量
                CastIfConfigured(me, SpellShadowPower, true);
                events.ScheduleEvent(EventShadowPower, _tier == 3 ? 22s : 28s);
                break;
            case EventShadowfury: // T3新增：当前分支外域暗影之怒
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellShadowfury, SPELLVALUE_BASE_POINT0,
                    ShadowfuryRaidDamage, true);
                events.ScheduleEvent(EventShadowfury, 20s);
                break;
            default:
                break;
        }
    }

private:
    uint8 _felfireCount = 0;
};

void AddSC_boss_rift_soccothrates()
{
    RegisterCreatureAI(boss_rift_soccothrates);
    RegisterSpellScript(spell_rift_soccothrates_fel_immolation);
    RegisterSpellScript(spell_rift_soccothrates_felfire);
}

} // namespace HeroicDungeonRift
