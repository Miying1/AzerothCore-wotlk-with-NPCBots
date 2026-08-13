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
// 原版机制：进战后唤醒身边的4个构造体（末日熔炉奥术铁匠、怒削魔像、怒火之锤构造体、武器技师）协同战斗。
enum Events : uint32
{
    EventChainLightning = 1, // 闪电链（T1基础）
    EventShock,              // 震击（T1基础）
    EventTier2Skill,         // 闪电箭（T2新增）
    EventTier3Skill          // 雷霆一击（T3新增）
};

// 各构造体事件（EventMap 独立）
enum ArcanasmithEvents : uint32 { EventArcaneBolt = 1, EventArcaneExplosion };
enum WrathHammerEvents : uint32 { EventFlameCannon = 1, EventUppercut };
enum TechnicianEvents : uint32 { EventShoot = 1, EventExplodingShot };

enum Spells : uint32
{
    // 主Boss 阿格曼奇
    SpellLightningShield = 15507, // 闪电之盾
    SpellChainLightning = 15305,  // 闪电链
    SpellShock = 15605,           // 震击
    SpellLightningBolt = 15801,   // 闪电箭
    SpellThunderclap = 15588,     // 雷霆一击
    // 末日熔炉奥术铁匠
    SpellArcaneBolt = 13748,    // 奥术箭
    SpellArcaneExplosion = 13745, // 魔爆术
    // 怒削魔像
    SpellFlurry = 15088, // 乱舞
    SpellFrenzy = 12795, // 狂乱
    // 怒火之锤构造体
    SpellFlameCannon = 15575, // 烈焰火炮
    SpellUppercut = 10966,    // 上钩拳
    // 武器技师
    SpellShoot = 6660,       // 射击
    SpellExplodingShot = 7896 // 爆炸射击
};

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
        _events.ScheduleEvent(EventArcaneBolt, Milliseconds(TierDelay(2500, 2000, 1600)));
        _events.ScheduleEvent(EventArcaneExplosion, Milliseconds(TierDelay(10000, 8000, 6500)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventArcaneBolt:
                DoCastVictim(SpellArcaneBolt);
                _events.ScheduleEvent(EventArcaneBolt, Milliseconds(TierDelay(3000, 2400, 1900)));
                break;
            case EventArcaneExplosion:
                DoCast(me, SpellArcaneExplosion);
                _events.ScheduleEvent(EventArcaneExplosion, Milliseconds(TierDelay(12000, 9500, 7500)));
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
        _events.ScheduleEvent(EventFlameCannon, Milliseconds(TierDelay(6000, 4800, 3800)));
        _events.ScheduleEvent(EventUppercut, Milliseconds(TierDelay(10000, 8000, 6500)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventFlameCannon:
                DoCastVictim(SpellFlameCannon);
                _events.ScheduleEvent(EventFlameCannon, Milliseconds(TierDelay(8000, 6500, 5200)));
                break;
            case EventUppercut:
                DoCastVictim(SpellUppercut);
                _events.ScheduleEvent(EventUppercut, Milliseconds(TierDelay(12000, 9500, 7500)));
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
        _events.ScheduleEvent(EventShoot, Milliseconds(TierDelay(2500, 2000, 1600)));
        _events.ScheduleEvent(EventExplodingShot, Milliseconds(TierDelay(8000, 6500, 5200)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventShoot:
                DoCastVictim(SpellShoot);
                _events.ScheduleEvent(EventShoot, Milliseconds(TierDelay(3000, 2400, 1900)));
                break;
            case EventExplodingShot:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SpellExplodingShot);
                _events.ScheduleEvent(EventExplodingShot, Milliseconds(TierDelay(11000, 9000, 7200)));
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

        // 原版：进战后唤醒身边的4个构造体
        SummonConstructs();

        ScheduleTieredEvent(EventChainLightning, 15000, 12000, 9500);
        ScheduleTieredEvent(EventShock, 6500, 5200, 4200);
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
                ScheduleTieredEvent(EventChainLightning, 16000, 13000, 10500);
                break;
            case EventShock:
                CastIfConfigured(me->GetVictim(), SpellShock);
                ScheduleTieredEvent(EventShock, 8000, 6500, 5200);
                break;
            case EventTier2Skill: // T2新增：闪电箭，顺发
                CastIfConfigured(me->GetVictim(), SpellLightningBolt, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 6s : 8s);
                break;
            case EventTier3Skill: // T3新增：雷霆一击，瞬发
                CastIfConfigured(me, SpellThunderclap, true);
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
