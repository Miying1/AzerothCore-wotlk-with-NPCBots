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
// 黑石深渊 - 达格兰·索瑞森大帝（Emperor Dagran Thaurissan）+ 茉艾拉公主（Princess Moira，同伴）
enum BossEvents : uint32
{
    EventHandOfThaurissan = 1, // 索瑞森之手（原版/T1基础）
    EventAvatarOfFlame,        // 烈焰化身（原版/T1基础）
    EventTier2Skill,           // 火焰冲击（T2新增）
    EventTier3Skill            // 暗影箭（T3新增）
};

enum MoiraEvents : uint32
{
    EventMoiraHeal = 1, // 治疗术（原版同伴/T1基础）
    EventMoiraRenew,    // 恢复（原版同伴/T1基础）
    EventMoiraMindBlast // 心灵震爆（原版同伴/T1基础）
};

enum Spells : uint32
{
    SpellHandOfThaurissan = 17492, // 索瑞森之手（原版/T1基础）
    SpellAvatarOfFlame = 15636,    // 烈焰化身（原版/T1基础，父Aura）
    SpellFlame = 15643,            // 烈焰（烈焰化身近战出手时触发的proc子法术）
    SpellFireBlast = 13342,        // 火焰冲击（T2新增）
    SpellShadowBolt = 20791,       // 暗影箭（T3新增）
    SpellMoiraHeal = 15586,        // 治疗术（原版同伴/T1基础）
    SpellMoiraRenew = 8362,        // 恢复（原版同伴/T1基础）
    SpellMoiraMindBlast = 15587    // 心灵震爆（原版同伴/T1基础）
};

constexpr int32 FireBlastTier1DirectDamage = 4500;
constexpr int32 ShadowBoltTier1DirectDamage = 4500;
// 烈焰（15643）每次触发的T1基准伤害；最终伤害 = 基准 × 当前Tier倍率（由BossAIBase::DamageDealt统一应用）。
constexpr int32 FlameTier1DirectDamage = 1400;

// 喊话（中文，对应原版 creature_text）
constexpr char const* ThaurissanCrushText = "我要把你们碾成碎片！";
constexpr char const* ThaurissanKillText = "向国王致敬，宝贝！";
constexpr uint32 ThaurissanCrushSound = 5457;
constexpr uint32 ThaurissanKillSound = 5431;
}

// 烈焰化身(15636)的第三个效果是光环42（PROC_TRIGGER_SPELL）：Boss每次近战出手都会触发子法术15643「烈焰」。
// 父光环的BP不会传给子法术，且proc由引擎直接施放、无法走CastRiftTunedSpell，必须在proc里改写15643的BP0；
// 伤害仍由Boss施放，因此继续经过BossAIBase::DamageDealt乘Tier倍率。
// 仅对裂隙Boss生效，原版索瑞森大帝（9019）保留DBC默认行为。
class spell_rift_thaurissan_avatar_of_flame : public AuraScript
{
    PrepareAuraScript(spell_rift_thaurissan_avatar_of_flame);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SpellFlame });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        Creature* caster = GetCaster() ? GetCaster()->ToCreature() : nullptr;
        Unit* target = GetTarget();
        if (!caster || !target || !GetTierForCreature(caster))
            return;

        // 与引擎默认取目标的方式保持一致：光环承载者出手时打被攻击者，被攻击时打攻击者。
        Unit* victim = target == eventInfo.GetActor() ? eventInfo.GetActionTarget() : eventInfo.GetActor();
        if (!victim || !victim->IsAlive())
            return;

        PreventDefaultAction();
        int32 damage = CompensateRiftCreatureLevelScaling(caster, SpellFlame, EFFECT_0, FlameTier1DirectDamage);
        caster->CastCustomSpell(SpellFlame, SPELLVALUE_BASE_POINT0, damage, victim,
            TRIGGERED_FULL_MASK, nullptr, aurEff);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_rift_thaurissan_avatar_of_flame::HandleProc,
            EFFECT_2, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

struct npc_rift_moira : public RiftSummonAI // 裂隙茉艾拉公主
{
    explicit npc_rift_moira(Creature* creature) : RiftSummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventMoiraHeal, 8000ms);
        _events.ScheduleEvent(EventMoiraRenew, 6000ms);
        _events.ScheduleEvent(EventMoiraMindBlast, 12000ms);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventMoiraHeal:
            {
                Unit* target = me;
                if (TempSummon* summon = me->ToTempSummon())
                    if (Unit* owner = summon->GetSummonerUnit())
                        if (owner->IsAlive() && owner->HealthBelowPct(90))
                            target = owner;
                DoCast(target, SpellMoiraHeal);
                _events.ScheduleEvent(EventMoiraHeal, 9000ms);
                break;
            }
            case EventMoiraRenew:
                if (TempSummon* summon = me->ToTempSummon())
                    if (Unit* owner = summon->GetSummonerUnit())
                        if (owner->IsAlive())
                            DoCast(owner, SpellMoiraRenew);
                _events.ScheduleEvent(EventMoiraRenew, 7000ms);
                break;
            case EventMoiraMindBlast:
                if (Unit* victim = me->GetVictim())
                    DoCast(victim, SpellMoiraMindBlast);
                _events.ScheduleEvent(EventMoiraMindBlast, 14000ms);
                break;
            default:
                break;
        }
    }
};

struct boss_rift_thaurissan : public BossAIBase
{
    explicit boss_rift_thaurissan(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(ThaurissanCrushText, LANG_UNIVERSAL);
        me->PlayDirectSound(ThaurissanCrushSound);

        // 原版同伴：裂隙开战时召唤茉艾拉，与索瑞森同时参战。
        SummonTieredCreature(RiftEntryMoira, me->GetRandomNearPosition(4.0f), 0.7f, 0.7f);
        events.ScheduleEvent(EventHandOfThaurissan, 5000ms);
        events.ScheduleEvent(EventAvatarOfFlame, 11000ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 9s);  // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 14s); // T3新增
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
        {
            me->Yell(ThaurissanKillText, LANG_UNIVERSAL, victim);
            me->PlayDirectSound(ThaurissanKillSound, victim->ToPlayer());
        }
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventHandOfThaurissan:
                CastIfConfigured(SelectRandomPlayer(), SpellHandOfThaurissan);
                events.ScheduleEvent(EventHandOfThaurissan, 6000ms);
                break;
            case EventAvatarOfFlame:
                CastIfConfigured(me, SpellAvatarOfFlame);
                events.ScheduleEvent(EventAvatarOfFlame, 25000ms);
                break;
            case EventTier2Skill: // T2新增：火焰冲击，瞬发
                CastFinalRaidDamageSpell(me->GetVictim(), SpellFireBlast, SPELLVALUE_BASE_POINT0,
                    FireBlastTier1DirectDamage, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 5s : 7s);
                break;
            case EventTier3Skill: // T3新增：暗影箭，随机点名，瞬发
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellShadowBolt, SPELLVALUE_BASE_POINT0,
                    ShadowBoltTier1DirectDamage, true);
                events.ScheduleEvent(EventTier3Skill, 8s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_thaurissan()
{
    RegisterCreatureAI(boss_rift_thaurissan);
    RegisterCreatureAI(npc_rift_moira);
    RegisterSpellScript(spell_rift_thaurissan_avatar_of_flame);
}

} // namespace HeroicDungeonRift
