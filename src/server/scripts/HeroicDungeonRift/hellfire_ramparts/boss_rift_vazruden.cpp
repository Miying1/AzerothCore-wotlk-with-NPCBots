/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "MotionMaster.h"
#include "ScriptMgr.h"

namespace HeroicDungeonRift
{
namespace
{
// 地狱火城墙 - 维斯路登/纳杉（脚本召唤遭遇）。裂隙主模板直接可攻击，并承载维斯路登本体逻辑。
constexpr uint32 RiftEntryNazan = 102058; // 源 Entry 17536

// 原版由17307控制器在该点召唤17537维斯路登和17536纳杉；裂隙直接复用该战斗点。
constexpr Position NazanSpawnPosition = { -1406.5f, 1746.5f, 81.2f, 5.46f };

enum Events : uint32
{
    EventRevenge = 1,
    EventNazanFireball,
    EventNazanConeOfFire,
    EventNazanRoar,
    EventTier2MortalStrike,
    EventTier3Whirlwind
};

enum Spells : uint32
{
    SpellRevenge = 19130,
    SpellCallNazan = 30693,
    SpellFireball = 33793,
    SpellConeOfFire = 30926,
    SpellBellowingRoar = 39427,
    SpellMortalStrike = 16856,
    SpellWhirlwind = 15589
};

constexpr char const* AggroText = "你们休想活着离开这里！";
constexpr char const* SlayText = "又一个倒下了！";
constexpr char const* DeathText = "不……这不可能……";
}

struct npc_rift_nazan : public RiftLevel70SummonAI
{
    explicit npc_rift_nazan(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void Reset() override
    {
        RiftLevel70SummonAI::Reset();
        _landed = false;
        me->SetCanFly(true);
        me->SetDisableGravity(true);
    }

    void DoAction(int32 action) override
    {
        if (action != 1 || _landed)
            return;

        _landed = true;
        me->SetCanFly(false);
        me->SetDisableGravity(false);
        me->SetReactState(REACT_AGGRESSIVE);
        _events.Reset();
        _events.ScheduleEvent(EventNazanFireball, 6s);
        _events.ScheduleEvent(EventNazanConeOfFire, 5s);
        // 裂隙T1对应原版英雄遭遇，落地后的咆哮属于T1原版机制。
        _events.ScheduleEvent(EventNazanRoar, 10s);
    }

protected:
    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventNazanFireball, 5s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventNazanFireball:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 60.0f, true))
                    DoCast(target, SpellFireball);
                _events.ScheduleEvent(EventNazanFireball, Milliseconds(urand(4000, 6000)));
                break;
            case EventNazanConeOfFire:
                DoCastVictim(SpellConeOfFire);
                _events.ScheduleEvent(EventNazanConeOfFire, 12s);
                break;
            case EventNazanRoar:
                DoCast(me, SpellBellowingRoar);
                _events.ScheduleEvent(EventNazanRoar, 30s);
                break;
            default:
                break;
        }
    }

private:
    bool _landed = false;
};

struct boss_rift_vazruden : public BossAIBase
{
    explicit boss_rift_vazruden(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        _nazanGuid.Clear();
        _nazanCalled = false;
        BossAIBase::Reset();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(AggroText, LANG_UNIVERSAL);
        events.ScheduleEvent(EventRevenge, 4s);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2MortalStrike, 11s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Whirlwind, 19s);

        if (Creature* nazan = SummonTieredCreature(RiftEntryNazan, NazanSpawnPosition, 0.8f, 0.8f,
            TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS))
            _nazanGuid = nazan->GetGUID();
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_nazanCalled || !me->HealthBelowPctDamaged(35, damage))
            return;

        _nazanCalled = true;
        CastIfConfigured(me, SpellCallNazan, true);
        if (Creature* nazan = ObjectAccessor::GetCreature(*me, _nazanGuid))
            nazan->AI()->DoAction(1);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
            me->Yell(SlayText, LANG_UNIVERSAL, victim);
    }

    void JustDied(Unit* killer) override
    {
        me->Yell(DeathText, LANG_UNIVERSAL);
        BossAIBase::JustDied(killer);
    }

    void ConfigureTier() override
    {
        // TBC 70级技能已接近目标伤害区间，仅做小幅83级法术基线修正。
        SetRaidSpellDamageMultiplier(2.0f);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventRevenge:
                CastIfConfigured(me->GetVictim(), SpellRevenge);
                events.ScheduleEvent(EventRevenge, 6s);
                break;
            case EventTier2MortalStrike:
                CastIfConfigured(me->GetVictim(), SpellMortalStrike);
                events.ScheduleEvent(EventTier2MortalStrike, 16s);
                break;
            case EventTier3Whirlwind:
                CastIfConfigured(me, SpellWhirlwind);
                events.ScheduleEvent(EventTier3Whirlwind, 24s);
                break;
            default:
                break;
        }
    }

private:
    ObjectGuid _nazanGuid;
    bool _nazanCalled = false;
};

void AddSC_boss_rift_vazruden()
{
    RegisterCreatureAI(boss_rift_vazruden);
    RegisterCreatureAI(npc_rift_nazan);
}

} // namespace HeroicDungeonRift
