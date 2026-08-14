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
enum Events : uint32
{
    EventCleave = 1, // 顺劈斩（原版/T1基础）
    EventWhirlwind,  // 旋风斩（原版/T1基础）
    EventTraineeWave // 血色预备兵周期波（T2新增；T3提高数量与频率）
};

enum Spells : uint32
{
    SpellCleave = 15496,   // 顺劈斩（原版/T1基础）
    SpellWhirlwind = 8989, // 旋风斩（原版/T1基础）
    SpellEnrage = 8269     // 狂乱（原版/T1基础；生命值低于40%时触发）
};
}

struct npc_rift_scarlet_trainee : public ScriptedAI
{
    explicit npc_rift_scarlet_trainee(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        if (Unit* owner = summoner->ToUnit())
            if (Unit* victim = owner->GetVictim())
                AttackStart(victim);
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        if (!UpdateVictim())
            return;
        DoMeleeAttackIfReady();
    }
};

struct boss_rift_herod : public BossAIBase
{
    explicit boss_rift_herod(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _enraged = false;
        _finalWave = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventCleave, Milliseconds(5000));
        events.ScheduleEvent(EventWhirlwind, Milliseconds(15000));
        if (_tier >= 2)
            events.ScheduleEvent(EventTraineeWave, 18s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!_enraged && me->HealthBelowPctDamaged(40, damage))
        {
            _enraged = true;
            CastIfConfigured(me, SpellEnrage, true);
        }
        // 15%最终波为全Tier一次性触发；不同于仅T2/T3启用且会持续重排的周期学员波。
        if (!_finalWave && me->HealthBelowPctDamaged(15, damage))
        {
            _finalWave = true;
            SummonTrainees(_tier == 1 ? 4 : (_tier == 2 ? 6 : 8));
        }
    }

    void JustDied(Unit* /*killer*/) override { DespawnRiftSummons(); }
    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventCleave:
                CastIfConfigured(me->GetVictim(), SpellCleave);
                events.ScheduleEvent(EventCleave, Milliseconds(8000));
                break;
            case EventWhirlwind:
                CastIfConfigured(me, SpellWhirlwind);
                events.ScheduleEvent(EventWhirlwind, Milliseconds(35000));
                break;
            case EventTraineeWave:
                SummonTrainees(_tier == 2 ? 3 : 4);
                events.ScheduleEvent(EventTraineeWave, _tier == 2 ? 30s : 23s);
                break;
            default:
                break;
        }
    }

private:
    uint32 CountAliveTrainees() const
    {
        uint32 count = 0;
        for (ObjectGuid const& guid : _riftSummons)
            if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                if (summon->IsAlive() && summon->GetEntry() == RiftEntryScarletTrainee)
                    ++count;
        return count;
    }

    void SummonTrainees(uint32 amount)
    {
        // 周期波与15%最终波共用存活上限：T1/T2/T3分别为4/8/12只。
        uint32 cap = _tier == 1 ? 4 : (_tier == 2 ? 8 : 12);
        for (uint32 alive = CountAliveTrainees(); alive < cap && amount > 0; ++alive, --amount)
        {
            Position position = me->GetRandomNearPosition(9.0f);
            if (Creature* summon = SummonTieredCreature(RiftEntryScarletTrainee, position, 0.25f, 0.45f))
                if (Unit* victim = me->GetVictim())
                    summon->AI()->AttackStart(victim);
        }
    }

    bool _enraged = false;
    bool _finalWave = false;
};

void AddSC_boss_rift_herod()
{
    RegisterCreatureAI(boss_rift_herod);
    RegisterCreatureAI(npc_rift_scarlet_trainee);
}

} // namespace HeroicDungeonRift
