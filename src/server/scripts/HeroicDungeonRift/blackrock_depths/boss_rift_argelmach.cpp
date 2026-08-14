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

struct boss_rift_argelmach : public BossAIBase
{
    explicit boss_rift_argelmach(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(ArgelmachAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(ArgelmachAggroSound);
        CastIfConfigured(me, SpellLightningShield, true);

        // 原版/T1：裂隙开战同时召唤4个构造体同伴
        SummonConstructs();

        events.ScheduleEvent(EventChainLightning, 15000ms);
        events.ScheduleEvent(EventShock, 6500ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 8s);  // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 14s); // T3新增
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

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
    void SummonConstructs()
    {
        SummonTieredCreature(RiftEntryArcanasmith, me->GetRandomNearPosition(5.0f), 0.7f, 0.7f);
        SummonTieredCreature(RiftEntryRagereaverGolem, me->GetRandomNearPosition(5.0f), 0.7f, 0.7f);
        SummonTieredCreature(RiftEntryWrathHammer, me->GetRandomNearPosition(5.0f), 0.7f, 0.7f);
        SummonTieredCreature(RiftEntryWeaponTechnician, me->GetRandomNearPosition(5.0f), 0.7f, 0.7f);
    }
};

void AddSC_boss_rift_argelmach()
{
    RegisterCreatureAI(boss_rift_argelmach);
    RegisterCreatureAI(npc_rift_argelmach_arcanasmith);
    RegisterCreatureAI(npc_rift_argelmach_golem);
    RegisterCreatureAI(npc_rift_argelmach_wrath_hammer);
    RegisterCreatureAI(npc_rift_argelmach_technician);
}

} // namespace HeroicDungeonRift
