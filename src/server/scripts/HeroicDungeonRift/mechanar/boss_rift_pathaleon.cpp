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
// 能源舰 - 计算者帕萨雷恩（Pathaleon the Calculator）
enum Events : uint32
{
    EventSummonWraiths = 1, // 召唤虚空怨灵（T1基础）
    EventManaTap,           // 法力分流（T1基础）
    EventArcaneTorrent,     // 奥术洪流（T1基础）
    EventDomination,        // 支配（T1基础）
    EventDisgruntledAnger,  // 不满的怒气（T1基础）
    EventArcaneExplosion,   // 奥术爆炸（T1英雄机制）
    EventPolarityShift,     // 极性转化（T2新增）
    EventArcaneBlast        // 奥术冲击（T3新增）
};

enum Spells : uint32
{
    SpellArcaneExplosion = 15453,
    SpellDisgruntledAnger = 35289,
    SpellArcaneTorrent = 36022,
    SpellManaTap = 36021,
    SpellDomination = 35280,
    SpellFrenzy = 36992,
    SpellPolarityShift = 39096,
    SpellArcaneBlast = 35314,
    SpellWraithArcaneBolt = 20720,
    SpellWraithNetherExplosion = 35058
};

enum Entries : uint32
{
    RiftEntryNetherWraith = 102050 // source 21062
};

constexpr int32 ArcaneBlastRaidDamage = 5000;

constexpr char const* PathaleonAggroText = "我们的时间非常紧迫。你们休想添乱！";
constexpr char const* PathaleonDominationText = "我正在寻找一个伙伴……";
constexpr char const* PathaleonSummonText = "是叫人帮忙的时候了。";
constexpr char const* PathaleonEnrageText = "我喜欢自己动手……";
constexpr char const* PathaleonSlayText = "一点小麻烦。";
constexpr char const* PathaleonDeathText = "计划……要继续下去。";
constexpr uint32 PathaleonAggroSound = 11193;
constexpr uint32 PathaleonDominationSound = 11197;
constexpr uint32 PathaleonSummonSound = 11196;
constexpr uint32 PathaleonEnrageSound = 11199;
constexpr uint32 PathaleonSlaySound = 11194;
constexpr uint32 PathaleonDeathSound = 11200;
}

struct npc_rift_nether_wraith : public RiftLevel70SummonAI
{
    explicit npc_rift_nether_wraith(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(1, 2500ms);
        _events.ScheduleEvent(2, 20s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case 1:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SpellWraithArcaneBolt);
                _events.ScheduleEvent(1, 5s);
                break;
            case 2:
                DoCast(me, SpellWraithNetherExplosion);
                _events.ScheduleEvent(2, 20s);
                break;
            default:
                break;
        }
    }
};

struct boss_rift_pathaleon : public BossAIBase
{
    explicit boss_rift_pathaleon(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        _enraged = false;
        BossAIBase::Reset();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(PathaleonAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(PathaleonAggroSound);

        events.ScheduleEvent(EventSummonWraiths, Milliseconds(urand(10000, 16000)));
        events.ScheduleEvent(EventManaTap, 12s);
        events.ScheduleEvent(EventArcaneTorrent, 16s);
        events.ScheduleEvent(EventDomination, Milliseconds(urand(10000, 16000)));
        events.ScheduleEvent(EventDisgruntledAnger, 25s);
        events.ScheduleEvent(EventArcaneExplosion, 8s);
        if (_tier >= 2)
            events.ScheduleEvent(EventPolarityShift, 22s);
        if (_tier >= 3)
            events.ScheduleEvent(EventArcaneBlast, 14s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_enraged || !me->HealthBelowPctDamaged(20, damage))
            return;

        _enraged = true;
        DespawnRiftSummons();
        me->Yell(PathaleonEnrageText, LANG_UNIVERSAL);
        me->PlayDirectSound(PathaleonEnrageSound);
        CastIfConfigured(me, SpellFrenzy, true);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
        {
            me->Yell(PathaleonSlayText, LANG_UNIVERSAL, victim);
            me->PlayDirectSound(PathaleonSlaySound, victim->ToPlayer());
        }
    }

    void JustDied(Unit* killer) override
    {
        BossAIBase::JustDied(killer);
        me->Yell(PathaleonDeathText, LANG_UNIVERSAL);
        me->PlayDirectSound(PathaleonDeathSound);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(5.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventSummonWraiths:
                if (!_enraged)
                {
                    me->Yell(PathaleonSummonText, LANG_UNIVERSAL);
                    me->PlayDirectSound(PathaleonSummonSound);
                    uint32 summonCount = _tier + 2;
                    for (uint32 i = 0; i < summonCount; ++i)
                        SummonTieredCreature(RiftEntryNetherWraith, me->GetRandomNearPosition(7.0f), 0.42f, 0.6f);
                }
                events.ScheduleEvent(EventSummonWraiths, Milliseconds(urand(45000, 50000)));
                break;
            case EventManaTap:
                if (Unit* target = SelectRandomPlayer(40.0f))
                    CastIfConfigured(target, SpellManaTap);
                events.ScheduleEvent(EventManaTap, 18s);
                break;
            case EventArcaneTorrent:
                me->RemoveAurasDueToSpell(SpellManaTap);
                CastIfConfigured(me, SpellArcaneTorrent);
                events.ScheduleEvent(EventArcaneTorrent, 15s);
                break;
            case EventDomination:
                if (Unit* target = SelectRandomPlayer(50.0f))
                {
                    me->Yell(PathaleonDominationText, LANG_UNIVERSAL, target);
                    me->PlayDirectSound(PathaleonDominationSound, target->ToPlayer());
                    CastIfConfigured(target, SpellDomination);
                }
                events.ScheduleEvent(EventDomination, Milliseconds(urand(27000, 40000)));
                break;
            case EventDisgruntledAnger:
                CastIfConfigured(SelectRandomPlayer(), SpellDisgruntledAnger);
                events.ScheduleEvent(EventDisgruntledAnger, Milliseconds(urand(40000, 90000)));
                break;
            case EventArcaneExplosion:
                CastIfConfigured(me, SpellArcaneExplosion);
                events.ScheduleEvent(EventArcaneExplosion, 12s);
                break;
            case EventPolarityShift: // T2新增：能源舰极性转化
                CastIfConfigured(me, SpellPolarityShift, true);
                events.ScheduleEvent(EventPolarityShift, _tier == 3 ? 28s : 36s);
                break;
            case EventArcaneBlast: // T3新增：能源舰奥术冲击
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellArcaneBlast, SPELLVALUE_BASE_POINT1,
                    ArcaneBlastRaidDamage, true);
                events.ScheduleEvent(EventArcaneBlast, 9s);
                break;
            default:
                break;
        }
    }

private:
    bool _enraged = false;
};

void AddSC_boss_rift_pathaleon()
{
    RegisterCreatureAI(boss_rift_pathaleon);
    RegisterCreatureAI(npc_rift_nether_wraith);
}

} // namespace HeroicDungeonRift
