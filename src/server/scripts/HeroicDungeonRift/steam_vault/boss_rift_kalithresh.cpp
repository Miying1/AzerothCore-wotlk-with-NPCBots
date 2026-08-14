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
// 蒸汽地窟 - 督军卡利瑟里斯（Warlord Kalithresh）
constexpr uint32 RiftEntryNagaDistiller = 102040; // 原型：纳迦蒸馏器 17954

enum KalithreshEvents : uint32
{
    EventSpellReflection = 1, // 法术反射（T1基础）
    EventImpale,              // 穿刺（T1基础）
    EventHeadCrack,           // 裂颅（T1基础）
    EventWarlordsRage,        // 督军之怒（T1基础机制）
    EventMortalStrike,        // 致死打击（T2新增）
    EventWarStomp             // 战争践踏（T3新增）
};

enum Spells : uint32
{
    SpellSpellReflection = 31534, // 法术反射
    SpellImpale = 39061,          // 穿刺
    SpellHeadCrack = 16172,             // 裂颅
    SpellWarlordsRageDistiller = 31543, // 蒸馏器引导的督军之怒
    SpellMortalStrike = 31911,          // 致死打击
    SpellWarStomp = 38750         // 战争践踏
};

constexpr char const* KalithreshIntroText = "你们以为击败了我的卫兵就够资格挑战我？我们的计划绝不会被破坏！";
constexpr char const* KalithreshAggroText = "我要砍下你们的脑袋！";
constexpr char const* KalithreshRageText = "这一切远没有结束……";
constexpr char const* KalithreshSlayText = "挣扎吧，肮脏的陆地生物！";
constexpr char const* KalithreshDeathText = "为了瓦丝琪女王……";
}

struct npc_rift_naga_distiller : public RiftLevel70SummonAI // 裂隙纳迦蒸馏器
{
    explicit npc_rift_naga_distiller(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void Reset() override
    {
        RiftLevel70SummonAI::Reset();
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();
    }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();
    }

    void UpdateAI(uint32 /*diff*/) override { }
};

struct boss_rift_kalithresh : public BossAIBase
{
    explicit boss_rift_kalithresh(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _introDone = false;
    }

    void MoveInLineOfSight(Unit* who) override
    {
        if (!_introDone && who && who->IsPlayer() && me->IsWithinDistInMap(who, 35.0f))
        {
            me->Yell(KalithreshIntroText, LANG_UNIVERSAL);
            _introDone = true;
        }

        ScriptedAI::MoveInLineOfSight(who);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(KalithreshAggroText, LANG_UNIVERSAL);
        // T1 原版技能在所有 Tier 均保持原版首次施放窗口与 CD。
        events.ScheduleEvent(EventSpellReflection, Milliseconds(urand(20000, 36000)));
        events.ScheduleEvent(EventImpale, Milliseconds(urand(7000, 14000)));
        events.ScheduleEvent(EventHeadCrack, 15s);
        events.ScheduleEvent(EventWarlordsRage, 20s);
        if (_tier >= 2)
            events.ScheduleEvent(EventMortalStrike, 10s);
        if (_tier >= 3)
            events.ScheduleEvent(EventWarStomp, 14s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
            me->Yell(KalithreshSlayText, LANG_UNIVERSAL, victim);
    }

    void JustDied(Unit* killer) override
    {
        me->Yell(KalithreshDeathText, LANG_UNIVERSAL);
        BossAIBase::JustDied(killer);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventSpellReflection:
                CastIfConfigured(me, SpellSpellReflection);
                events.ScheduleEvent(EventSpellReflection, Milliseconds(urand(20000, 36000)));
                break;
            case EventImpale:
                CastIfConfigured(SelectRandomPlayer(10.0f), SpellImpale);
                events.ScheduleEvent(EventImpale, Milliseconds(urand(7500, 12500)));
                break;
            case EventHeadCrack:
                CastIfConfigured(me->GetVictim(), SpellHeadCrack);
                events.ScheduleEvent(EventHeadCrack, Milliseconds(urand(45000, 55000)));
                break;
            case EventWarlordsRage:
                ChannelDistiller();
                events.ScheduleEvent(EventWarlordsRage, 45s);
                break;
            case EventMortalStrike: // T2新增：强化当前目标承伤压力
                CastIfConfigured(me->GetVictim(), SpellMortalStrike, true);
                events.ScheduleEvent(EventMortalStrike, _tier == 3 ? 11s : 14s);
                break;
            case EventWarStomp: // T3新增：近战范围击退
                CastIfConfigured(me, SpellWarStomp, true);
                events.ScheduleEvent(EventWarStomp, 18s);
                break;
            default:
                break;
        }
    }

private:
    void ChannelDistiller()
    {
        me->Yell(KalithreshRageText, LANG_UNIVERSAL);
        if (Creature* distiller = SummonTieredCreature(RiftEntryNagaDistiller, me->GetRandomNearPosition(6.0f),
            0.35f, 0.1f, TEMPSUMMON_TIMED_DESPAWN, 12 * IN_MILLISECONDS))
        {
            // 原版由可被击杀的蒸馏器向Boss施加督军之怒；玩家通过击杀蒸馏器阻止其完整生效。
            distiller->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            distiller->CastSpell(me, SpellWarlordsRageDistiller, true);
        }
    }

    bool _introDone = false;
};

void AddSC_boss_rift_kalithresh()
{
    RegisterCreatureAI(boss_rift_kalithresh);
    RegisterCreatureAI(npc_rift_naga_distiller);
}

} // namespace HeroicDungeonRift
