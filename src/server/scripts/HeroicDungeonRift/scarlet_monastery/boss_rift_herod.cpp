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
    EventCleave = 1,
    EventWhirlwind,
    EventTraineeWave
};

enum Spells : uint32
{
    SpellCleave = 15496,
    SpellWhirlwind = 8989,
    SpellEnrage = 8269
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
        ScheduleTieredEvent(EventCleave, 5000, 4000, 3200);
        ScheduleTieredEvent(EventWhirlwind, 15000, 12000, 9500);
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
                ScheduleTieredEvent(EventCleave, 8000, 6500, 5000);
                break;
            case EventWhirlwind:
                CastIfConfigured(me, SpellWhirlwind);
                ScheduleTieredEvent(EventWhirlwind, 35000, 29000, 23000);
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
