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
// 地狱火城墙 - 无疤者奥摩尔（Omor the Unscarred）
enum BossEvents : uint32
{
    EventTreacherousAura = 1, // 背叛光环（T1基础）
    EventSummonHound,         // 召唤残忍的军犬（T1基础）
    EventShadowBolt,          // 目标脱离近战后的暗影箭（T1基础）
    EventDemonicShield,       // 21%生命阶段恶魔之盾（T1基础）
    EventShadowfury,          // 暗影之怒（T3新增）
    EventResetSlayText        // 击杀喊话冷却
};

enum HoundEvents : uint32
{
    EventDrainLife = 1,
    EventManaBurn
};

enum Spells : uint32
{
    SpellShadowBolt = 30686,
    SpellTreacherousAura = 30695,
    SpellDemonicShield = 31901,
    SpellDrainLife = 35748,
    SpellManaBurn = 15785,
    SpellShadowfury = 39082
};

enum RiftEntries : uint32
{
    RiftEntryFiendishHound = 102039
};

char const* const AggroTexts[] =
{
    "你们竟敢跟我作对？",
    "我不会被打败的！",
    "你们死定了！"
};

constexpr char const* SummonText = "Achor she-ki！吞噬他们的躯体！";
constexpr char const* CurseText = "A-kreesh！";
constexpr char const* KillText = "死吧！蠢货！";
constexpr char const* DeathText = "你们……等着瞧。";
}

struct npc_rift_fiendish_hound : public RiftLevel70SummonAI
{
    explicit npc_rift_fiendish_hound(Creature* creature) : RiftLevel70SummonAI(creature) { }

protected:
    void ScheduleAbilities() override
    {
        // 军犬是T1原版召唤机制，所有Tier保持原版英雄SAI窗口。
        _events.ScheduleEvent(EventDrainLife, Milliseconds(urand(0, 3000)));
        _events.ScheduleEvent(EventManaBurn, Milliseconds(urand(5000, 6000)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventDrainLife:
                DoCastVictim(SpellDrainLife);
                _events.ScheduleEvent(EventDrainLife, Milliseconds(urand(7000, 10000)));
                break;
            case EventManaBurn:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SpellManaBurn);
                _events.ScheduleEvent(EventManaBurn, Milliseconds(urand(10000, 15000)));
                break;
            default:
                break;
        }
    }
};

struct boss_rift_omor : public BossAIBase
{
    explicit boss_rift_omor(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _shieldStarted = false;
        _hasSpoken = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(AggroTexts[urand(0, 2)], LANG_UNIVERSAL);
        // T1 原版技能在所有 Tier 均保持原版首次施放窗口与 CD。
        events.ScheduleEvent(EventTreacherousAura, 6s);
        events.ScheduleEvent(EventSummonHound, 10s);
        events.ScheduleEvent(EventShadowBolt, 2s);
        if (_tier >= 3)
            events.ScheduleEvent(EventShadowfury, 16s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_shieldStarted || !me->HealthBelowPctDamaged(21, damage))
            return;

        _shieldStarted = true;
        events.ScheduleEvent(EventDemonicShield, 1ms);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer() && !_hasSpoken)
        {
            _hasSpoken = true;
            me->Yell(KillText, LANG_UNIVERSAL);
            events.ScheduleEvent(EventResetSlayText, 6s);
        }
    }

    void JustDied(Unit* killer) override
    {
        me->Yell(DeathText, LANG_UNIVERSAL);
        BossAIBase::JustDied(killer);
    }

protected:
    void ConfigureTier() override
    {
        // TBC 暗影法术基础伤害约千点，3 倍足以匹配 83 级团队承伤。
        SetRaidSpellDamageMultiplier(3.0f);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventTreacherousAura:
                if (roll_chance_i(33))
                    me->Yell(CurseText, LANG_UNIVERSAL);
                CastIfConfigured(SelectRandomPlayer(), SpellTreacherousAura);
                events.ScheduleEvent(EventTreacherousAura, Milliseconds(urand(12000, 18000)));
                break;
            case EventSummonHound:
                me->Yell(SummonText, LANG_UNIVERSAL);
                SummonTieredCreature(RiftEntryFiendishHound, me->GetRandomNearPosition(6.0f),
                    0.55f, 0.7f);
                events.ScheduleEvent(EventSummonHound, 15s);
                break;
            case EventShadowBolt:
                if (Unit* victim = me->GetVictim())
                    if (!me->IsWithinMeleeRange(victim))
                        CastIfConfigured(victim, SpellShadowBolt);
                events.ScheduleEvent(EventShadowBolt, 2s);
                break;
            case EventDemonicShield:
                CastIfConfigured(me, SpellDemonicShield);
                events.ScheduleEvent(EventDemonicShield, 15s);
                break;
            case EventShadowfury: // T3新增：随机目标暗影之怒
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellShadowfury, SPELLVALUE_BASE_POINT0,
                    3500);
                events.ScheduleEvent(EventShadowfury, 20s);
                break;
            case EventResetSlayText:
                _hasSpoken = false;
                break;
            default:
                break;
        }
    }

private:
    bool _shieldStarted = false;
    bool _hasSpoken = false;
};

void AddSC_boss_rift_omor()
{
    RegisterCreatureAI(boss_rift_omor);
    RegisterCreatureAI(npc_rift_fiendish_hound);
}

} // namespace HeroicDungeonRift
