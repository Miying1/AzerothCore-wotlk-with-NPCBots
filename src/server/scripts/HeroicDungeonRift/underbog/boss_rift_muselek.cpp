/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "ScriptMgr.h"

#include <vector>

namespace HeroicDungeonRift
{
namespace
{
// 幽暗沼泽 - 沼地领主穆塞雷克（Swamplord Musel'ek）+ 克劳恩（Claw）
constexpr uint32 RiftEntryClaw = 102041; // 原型：克劳恩 17827
constexpr int32 SerpentStingRaidDamagePerTick = 1500;

enum MuselekEvents : uint32
{
    EventShoot = 1,        // 射击（T1基础）
    EventKnockAway,        // 击退（T1基础）
    EventMultiShot,        // 多重射击（T1基础）
    EventFreezingTrap,     // 冰冻陷阱（T1基础机制）
    EventAimedShot,        // 瞄准射击（陷阱后连招）
    EventRaptorStrike,     // 猛禽一击（T2新增）
    EventSerpentSting      // 毒蛇钉刺（T3新增）
};

enum ClawEvents : uint32
{
    EventClawCharge = 1, // 野性冲锋
    EventClawRoar,       // 震耳咆哮
    EventClawMaul,       // 重槌
    EventClawFrenzy      // 狂乱
};

enum Spells : uint32
{
    SpellShoot = 22907,        // 射击
    SpellKnockAway = 18813,    // 击退
    SpellRaptorStrike = 31566, // 猛禽一击
    SpellMultiShot = 34974,    // 多重射击
    SpellFreezingTrap = 31946, // 投掷冰冻陷阱
    SpellAimedShot = 31623,    // 瞄准射击
    SpellHuntersMark = 31615,  // 猎人印记
    SpellSerpentSting = 31975, // 毒蛇钉刺
    SpellFeralCharge = 39435,  // 野性冲锋
    SpellEchoingRoar = 31429,  // 震耳咆哮
    SpellClawMaul = 34298,     // 重槌
    SpellClawFrenzy = 34971    // 狂乱
};

constexpr char const* MuselekPetText = "野兽！服从我！立刻杀了他们！";
constexpr char const* MuselekAggroText = "我们决一死战！";
constexpr char const* MuselekSlayText = "结束了。";
constexpr char const* MuselekDeathText = "干得……好……";
}

struct npc_rift_claw : public RiftLevel70SummonAI // 裂隙克劳恩
{
    explicit npc_rift_claw(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void ScheduleAbilities() override
    {
        // 克劳恩属于T1原版伴生实体，所有Tier保持原版首次施放窗口。
        _events.ScheduleEvent(EventClawCharge, 7400ms);
        _events.ScheduleEvent(EventClawRoar, 2400ms);
        _events.ScheduleEvent(EventClawMaul, 5300ms);
        _events.ScheduleEvent(EventClawFrenzy, 5s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventClawCharge:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SpellFeralCharge);
                _events.ScheduleEvent(EventClawCharge, 20s);
                break;
            case EventClawRoar:
                DoCast(me, SpellEchoingRoar);
                _events.ScheduleEvent(EventClawRoar, 16s);
                break;
            case EventClawMaul:
                DoCastVictim(SpellClawMaul);
                _events.ScheduleEvent(EventClawMaul, 14s);
                break;
            case EventClawFrenzy:
                if (me->HealthBelowPct(50))
                    DoCast(me, SpellClawFrenzy);
                _events.ScheduleEvent(EventClawFrenzy, 30s);
                break;
            default:
                break;
        }
    }
};

struct boss_rift_muselek : public BossAIBase
{
    explicit boss_rift_muselek(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        ClearMarkedTargets();
        BossAIBase::Reset();
        _markedTarget.Clear();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(MuselekAggroText, LANG_UNIVERSAL);
        me->Yell(MuselekPetText, LANG_UNIVERSAL);
        SummonTieredCreature(RiftEntryClaw, me->GetRandomNearPosition(5.0f), 0.75f, 0.8f);

        // T1 原版技能在所有 Tier 均保持原版首次施放窗口与 CD。
        events.ScheduleEvent(EventShoot, 3s);
        events.ScheduleEvent(EventKnockAway, Milliseconds(urand(15000, 30000)));
        events.ScheduleEvent(EventMultiShot, Milliseconds(urand(10000, 15000)));
        events.ScheduleEvent(EventFreezingTrap, Milliseconds(urand(30000, 40000)));
        if (_tier >= 2)
            events.ScheduleEvent(EventRaptorStrike, 8s);
        if (_tier >= 3)
            events.ScheduleEvent(EventSerpentSting, 12s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
            me->Yell(MuselekSlayText, LANG_UNIVERSAL, victim);
    }

    void JustDied(Unit* killer) override
    {
        me->Yell(MuselekDeathText, LANG_UNIVERSAL);
        ClearMarkedTargets();
        BossAIBase::JustDied(killer);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventShoot:
                if (Unit* victim = me->GetVictim())
                    if (me->IsWithinLOSInMap(victim) && me->IsWithinDistInMap(victim, 30.0f) && !me->IsWithinDistInMap(victim, 10.0f))
                        CastIfConfigured(victim, SpellShoot);
                events.ScheduleEvent(EventShoot, 3s);
                break;
            case EventKnockAway:
                if (Unit* victim = me->GetVictim())
                    if (me->IsWithinMeleeRange(victim))
                        CastIfConfigured(victim, SpellKnockAway);
                events.ScheduleEvent(EventKnockAway, Milliseconds(urand(15000, 30000)));
                break;
            case EventMultiShot:
                CastIfConfigured(me->GetVictim(), SpellMultiShot);
                events.ScheduleEvent(EventMultiShot, Milliseconds(urand(20000, 30000)));
                break;
            case EventFreezingTrap:
                if (Unit* target = SelectRandomPlayer(40.0f))
                {
                    CastIfConfigured(target, SpellFreezingTrap);
                    _markedTarget = target->GetGUID();
                    events.ScheduleEvent(EventAimedShot, 6s);
                }
                events.ScheduleEvent(EventFreezingTrap, Milliseconds(urand(12000, 16000)));
                break;
            case EventAimedShot:
                if (Unit* target = ObjectAccessor::GetUnit(*me, _markedTarget))
                {
                    CastIfConfigured(target, SpellHuntersMark, true);
                    _markedTargets.push_back(target->GetGUID());
                    CastIfConfigured(target, SpellAimedShot);
                }
                break;
            case EventRaptorStrike: // T2新增：目标进入近战范围时补充猛禽一击
                if (Unit* victim = me->GetVictim())
                    if (me->IsWithinMeleeRange(victim))
                        CastIfConfigured(victim, SpellRaptorStrike, true);
                events.ScheduleEvent(EventRaptorStrike, _tier == 3 ? 7s : 9s);
                break;
            case EventSerpentSting: // T3新增：随机目标持续自然伤害
                CastFinalRaidDamageSpell(SelectRandomPlayer(40.0f), SpellSerpentSting, SPELLVALUE_BASE_POINT0,
                    SerpentStingRaidDamagePerTick, true);
                events.ScheduleEvent(EventSerpentSting, 12s);
                break;
            default:
                break;
        }
    }

private:
    void ClearMarkedTargets()
    {
        for (ObjectGuid const& guid : _markedTargets)
            if (Unit* target = ObjectAccessor::GetUnit(*me, guid))
                target->RemoveAurasDueToSpell(SpellHuntersMark, me->GetGUID());
        _markedTargets.clear();
    }

    ObjectGuid _markedTarget;
    std::vector<ObjectGuid> _markedTargets;
};

void AddSC_boss_rift_muselek()
{
    RegisterCreatureAI(boss_rift_muselek);
    RegisterCreatureAI(npc_rift_claw);
}

} // namespace HeroicDungeonRift
