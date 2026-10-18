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
// 黑石深渊 - 傀儡统帅阿格曼奇（Golem Lord Argelmach）
// 原版/T1多成员机制：裂隙开战同时召唤4个构造体同伴协同战斗。
enum Events : uint32
{
    EventChainLightning = 1, // 闪电链（原版/T1基础）
    EventShock,              // 震击（原版/T1基础）
    EventTier2Skill,         // 闪电箭（T2新增）
    EventTier3Skill          // 雷霆一击（T3新增）
};

// 以下均为原版同伴/T1基础技能；各类构造体使用独立EventMap。
enum ArcanasmithEvents : uint32
{
    EventArcaneBolt = 1, // 末日熔炉奥术铁匠：奥术箭
    EventArcaneExplosion // 末日熔炉奥术铁匠：魔爆术
};

enum WrathHammerEvents : uint32
{
    EventFlameCannon = 1, // 怒火之锤构造体：烈焰火炮
    EventUppercut         // 怒火之锤构造体：上钩拳
};

enum TechnicianEvents : uint32
{
    EventShoot = 1,      // 武器技师：射击
    EventExplodingShot   // 武器技师：爆炸射击
};

enum Spells : uint32
{
    // 主Boss 阿格曼奇
    SpellLightningShield = 15507, // 闪电之盾（原版/T1基础）
    SpellChainLightning = 15305,  // 闪电链（原版/T1基础）
    SpellShock = 15605,           // 震击（原版/T1基础）
    SpellLightningBolt = 15801,   // 闪电箭（T2新增）
    SpellThunderclap = 15588,     // 雷霆一击（T3新增）
    // 末日熔炉奥术铁匠（原版同伴/T1基础）
    SpellArcaneBolt = 13748,      // 奥术箭
    SpellArcaneExplosion = 13745, // 魔爆术
    // 怒削魔像（原版同伴/T1基础）
    SpellFlurry = 15088, // 乱舞
    SpellFrenzy = 12795, // 狂乱
    // 怒火之锤构造体（原版同伴/T1基础）
    SpellFlameCannon = 15575, // 烈焰火炮
    SpellUppercut = 10966,    // 上钩拳
    // 武器技师（原版同伴/T1基础）
    SpellShoot = 6660,       // 射击
    SpellExplodingShot = 7896 // 爆炸射击
};

constexpr int32 LightningBoltTier1DirectDamage = 4500;
constexpr int32 ThunderclapTier1DirectDamage = 3500;

// 喊话（中文，对应原版 creature_text）
constexpr char const* ArgelmachAggroText = "工厂里有入侵者？我的构造体会毁灭你们！";
constexpr uint32 ArgelmachAggroSound = 5297;
}

// 末日熔炉奥术铁匠：奥术箭 + 魔爆术
struct npc_rift_argelmach_arcanasmith : public RiftSummonAI
{
    explicit npc_rift_argelmach_arcanasmith(Creature* creature) : RiftSummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventArcaneBolt, 2500ms);
        _events.ScheduleEvent(EventArcaneExplosion, 10000ms);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventArcaneBolt:
                DoCastVictim(SpellArcaneBolt);
                _events.ScheduleEvent(EventArcaneBolt, 3000ms);
                break;
            case EventArcaneExplosion:
                DoCast(me, SpellArcaneExplosion);
                _events.ScheduleEvent(EventArcaneExplosion, 12000ms);
                break;
            default:
                break;
        }
    }
};

// 怒削魔像：乱舞(开战) + 狂乱(<30%)
struct npc_rift_argelmach_golem : public RiftSummonAI
{
    explicit npc_rift_argelmach_golem(Creature* creature) : RiftSummonAI(creature) { }

    void Reset() override
    {
        _frenzied = false;
        RiftSummonAI::Reset();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        DoCast(me, SpellFlurry, true);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_frenzied || !me->HealthBelowPctDamaged(30, damage))
            return;

        _frenzied = true;
        DoCast(me, SpellFrenzy, true);
    }

private:
    bool _frenzied = false;
};

// 怒火之锤构造体：烈焰火炮 + 上钩拳
struct npc_rift_argelmach_wrath_hammer : public RiftSummonAI
{
    explicit npc_rift_argelmach_wrath_hammer(Creature* creature) : RiftSummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventFlameCannon, 6000ms);
        _events.ScheduleEvent(EventUppercut, 10000ms);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventFlameCannon:
                DoCastVictim(SpellFlameCannon);
                _events.ScheduleEvent(EventFlameCannon, 8000ms);
                break;
            case EventUppercut:
                DoCastVictim(SpellUppercut);
                _events.ScheduleEvent(EventUppercut, 12000ms);
                break;
            default:
                break;
        }
    }
};

// 武器技师：射击 + 爆炸射击
struct npc_rift_argelmach_technician : public RiftSummonAI
{
    explicit npc_rift_argelmach_technician(Creature* creature) : RiftSummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventShoot, 2500ms);
        _events.ScheduleEvent(EventExplodingShot, 8000ms);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventShoot:
                DoCastVictim(SpellShoot);
                _events.ScheduleEvent(EventShoot, 3000ms);
                break;
            case EventExplodingShot:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SpellExplodingShot);
                _events.ScheduleEvent(EventExplodingShot, 11000ms);
                break;
            default:
                break;
        }
    }
};

// 15305 闪电链：CastCustomSpell 对链式法术写入 BASE_POINT 后实测仍按 DBC 值结算，
// 这里在命中时按裂隙T1基线重写 EFFECT_0 伤害；非裂隙Boss（原版阿格曼奇等）保留DBC原始行为。
class spell_rift_chain_lightning : public SpellScript
{
    PrepareSpellScript(spell_rift_chain_lightning);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SpellChainLightning });
    }

    void HandleDamage(SpellEffIndex effIndex)
    {
        if (effIndex != EFFECT_0)
            return;

        Creature* caster = GetCaster() ? GetCaster()->ToCreature() : nullptr;
        if (!caster || !GetTierConfigForCreature(caster))
            return;

        RiftSpellDamageTuning const* tuning = GetRiftSpellDamageTuning(SpellChainLightning);
        if (!tuning || !tuning->EffectBasePoints[EFFECT_0])
            return;

        int32 damage = CompensateRiftCreatureLevelScaling(caster, SpellChainLightning, EFFECT_0,
            tuning->EffectBasePoints[EFFECT_0]);
        SetEffectValue(std::max<int32>(1, damage));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_rift_chain_lightning::HandleDamage, EFFECT_0,
            SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

struct boss_rift_argelmach : public BossAIBase
{
    explicit boss_rift_argelmach(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _nextSpawnIndex = 0;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(ArgelmachAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(ArgelmachAggroSound);
        CastIfConfigured(me, SpellLightningShield, true);

        events.ScheduleEvent(EventChainLightning, 15000ms);
        events.ScheduleEvent(EventShock, 6500ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 8s);  // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 14s); // T3新增
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        // 原版的4个构造体同伴改为按血量阈值依次召唤：90%、75%、60%、50%各出1只。
        if (_nextSpawnIndex >= SpawnOrderCount ||
            !me->HealthBelowPctDamaged(SpawnThresholds[_nextSpawnIndex], damage))
            return;

        SummonTieredCreature(SpawnEntries[_nextSpawnIndex], me->GetRandomNearPosition(5.0f), 0.7f, 0.9f);
        ++_nextSpawnIndex;
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventChainLightning:
                CastIfConfigured(me->GetVictim(), SpellChainLightning);
                events.ScheduleEvent(EventChainLightning, 16000ms);
                break;
            case EventShock:
                CastIfConfigured(me->GetVictim(), SpellShock);
                events.ScheduleEvent(EventShock, 8000ms);
                break;
            case EventTier2Skill: // T2新增：闪电箭，瞬发
                CastFinalRaidDamageSpell(me->GetVictim(), SpellLightningBolt, SPELLVALUE_BASE_POINT0,
                    LightningBoltTier1DirectDamage, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 6s : 8s);
                break;
            case EventTier3Skill: // T3新增：雷霆一击，瞬发
                CastFinalRaidDamageSpell(me, SpellThunderclap, SPELLVALUE_BASE_POINT0,
                    ThunderclapTier1DirectDamage, true);
                events.ScheduleEvent(EventTier3Skill, 16s);
                break;
            default:
                break;
        }
    }

private:
    // 按顺序召唤的四类构造体及其血量阈值：90% 铁匠 → 75% 魔像 → 60% 怒火之锤 → 50% 武器技师。
    static constexpr uint8 SpawnOrderCount = 4;
    static constexpr int32 SpawnThresholds[SpawnOrderCount] = { 90, 75, 60, 50 };
    static constexpr uint32 SpawnEntries[SpawnOrderCount] =
    {
        RiftEntryArcanasmith,
        RiftEntryRagereaverGolem,
        RiftEntryWrathHammer,
        RiftEntryWeaponTechnician
    };

    uint8 _nextSpawnIndex = 0;
};

void AddSC_boss_rift_argelmach()
{
    RegisterCreatureAI(boss_rift_argelmach);
    RegisterCreatureAI(npc_rift_argelmach_arcanasmith);
    RegisterCreatureAI(npc_rift_argelmach_golem);
    RegisterCreatureAI(npc_rift_argelmach_wrath_hammer);
    RegisterCreatureAI(npc_rift_argelmach_technician);
    RegisterSpellScript(spell_rift_chain_lightning);
}

} // namespace HeroicDungeonRift
