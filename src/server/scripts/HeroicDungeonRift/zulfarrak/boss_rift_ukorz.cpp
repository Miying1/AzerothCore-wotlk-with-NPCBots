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
// 祖尔法拉克 - 乌克兹·沙顶（Chief Ukorz Sandscalp）+ 卢兹鲁（Ruuzlu，双Boss同伴）
enum UkorzEvents : uint32
{
    EventUkorzCleave = 1, // 顺劈斩（原版/T1基础）
    EventSunderArmor,     // 破甲攻击（T2新增）
    EventTier3Skill       // 雷霆一击（T3新增）
};

enum RuuzluEvents : uint32
{
    EventRuuzluCleave = 1, // 顺劈斩（原版同伴/T1基础）
    EventRuuzluExecute     // 斩杀（原版同伴/T1基础）
};

enum Spells : uint32
{
    SpellBerserkerStance = 7366, // 狂暴姿态（原版/T1基础；Boss与同伴）
    SpellCleave = 15496,         // 顺劈斩（原版/T1基础；Boss与同伴）
    SpellFrenzy = 8269,          // 狂乱（原版/T1基础，低血量狂暴）
    SpellSunderArmor = 15572,    // 破甲攻击（T2新增）
    SpellThunderclap = 15588,    // 雷霆一击（T3新增）
    SpellExecute = 38959         // 斩杀（原版同伴/T1基础，卢兹鲁）
};

constexpr int32 ThunderclapTier1DirectDamage = 3500;

// 喊话（中文，对应原版 creature_text）
constexpr char const* UkorzAggroText = "沙怒至高无上！";
constexpr char const* UkorzFrenzyText = "感受沙漠的怒火吧！";
constexpr char const* UkorzKillText = "这片沙漠是我的！";
}

struct npc_rift_ruuzlu : public RiftSummonAI // 裂隙卢兹鲁
{
    explicit npc_rift_ruuzlu(Creature* creature) : RiftSummonAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        DoCast(me, SpellBerserkerStance, true);
        _events.ScheduleEvent(EventRuuzluCleave, 7s);
        _events.ScheduleEvent(EventRuuzluExecute, 10s);
    }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventRuuzluCleave, 7s);
        _events.ScheduleEvent(EventRuuzluExecute, 10s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventRuuzluCleave:
                DoCastVictim(SpellCleave);
                _events.ScheduleEvent(EventRuuzluCleave, 9s);
                break;
            case EventRuuzluExecute:
                if (Unit* victim = me->GetVictim())
                    if (victim->HealthBelowPct(20))
                        DoCastVictim(SpellExecute);
                _events.ScheduleEvent(EventRuuzluExecute, 9s);
                break;
            default:
                break;
        }
    }
};

struct boss_rift_ukorz : public BossAIBase
{
    explicit boss_rift_ukorz(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _frenzied = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(UkorzAggroText, LANG_UNIVERSAL);
        CastIfConfigured(me, SpellBerserkerStance, true);
        SummonRuuzlu(); // 原版同伴：裂隙开战时与乌克兹同时参战
        events.ScheduleEvent(EventUkorzCleave, 6000ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventSunderArmor, 7s); // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 12s); // T3新增
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_frenzied || !me->HealthBelowPctDamaged(30, damage))
            return;

        _frenzied = true;
        me->Yell(UkorzFrenzyText, LANG_UNIVERSAL);
        CastIfConfigured(me, SpellFrenzy);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
            me->Yell(UkorzKillText, LANG_UNIVERSAL, victim);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventUkorzCleave:
                CastIfConfigured(me->GetVictim(), SpellCleave);
                events.ScheduleEvent(EventUkorzCleave, 9000ms);
                break;
            case EventSunderArmor: // T2新增：破甲攻击，瞬发
                CastIfConfigured(me->GetVictim(), SpellSunderArmor, true);
                events.ScheduleEvent(EventSunderArmor, _tier == 3 ? 8s : 10s);
                break;
            case EventTier3Skill: // T3新增：雷霆一击，瞬发
                CastFinalRaidDamageSpell(me, SpellThunderclap, SPELLVALUE_BASE_POINT0,
                    ThunderclapTier1DirectDamage, true);
                events.ScheduleEvent(EventTier3Skill, 14s);
                break;
            default:
                break;
        }
    }

private:
    void SummonRuuzlu()
    {
        SummonTieredCreature(RiftEntryRuuzlu, me->GetRandomNearPosition(3.0f), 0.8f, 0.8f);
    }

    bool _frenzied = false;
};

void AddSC_boss_rift_ukorz()
{
    RegisterCreatureAI(boss_rift_ukorz);
    RegisterCreatureAI(npc_rift_ruuzlu);
}

} // namespace HeroicDungeonRift
