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
    EventWrath = 1,
    EventFrostbolt,
    EventFrostNova
};

enum Spells : uint32
{
    SpellFrostArmor = 12556,
    SpellWrath = 13009,
    SpellFrostbolt = 15530,
    SpellFrostNova = 15531
};
}

struct npc_rift_frost_spectre : public ScriptedAI
{
    explicit npc_rift_frost_spectre(Creature* creature) : ScriptedAI(creature) { }

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

struct boss_rift_amnennar : public BossAIBase
{
    explicit boss_rift_amnennar(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _summonedAt55 = false;
        _summonedAt30 = false;
        if (_tierConfig)
            CastIfConfigured(me, SpellFrostArmor, true);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventWrath, 9000, 7500, 6000);
        ScheduleTieredEvent(EventFrostbolt, 1500, 1200, 900);
        ScheduleTieredEvent(EventFrostNova, 8000, 6500, 5000);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!_summonedAt55 && me->HealthBelowPctDamaged(55, damage))
        {
            _summonedAt55 = true;
            SummonSpectres(_tier);
        }
        if (!_summonedAt30 && me->HealthBelowPctDamaged(30, damage))
        {
            _summonedAt30 = true;
            SummonSpectres(_tier);
        }
    }

    void JustDied(Unit* /*killer*/) override { DespawnRiftSummons(); }
    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventWrath:
                CastIfConfigured(me->GetVictim(), SpellWrath);
                ScheduleTieredEvent(EventWrath, 13000, 10500, 8500);
                break;
            case EventFrostbolt:
                CastIfConfigured(me->GetVictim(), SpellFrostbolt);
                ScheduleTieredEvent(EventFrostbolt, 4000, 3300, 2700);
                break;
            case EventFrostNova:
                if (me->GetVictim() && me->IsWithinDistInMap(me->GetVictim(), 10.0f))
                    CastIfConfigured(me, SpellFrostNova);
                ScheduleTieredEvent(EventFrostNova, 17000, 14000, 11000);
                break;
            default:
                break;
        }
    }

private:
    void SummonSpectres(uint32 count)
    {
        for (uint32 i = 0; i < count; ++i)
        {
            Position position = me->GetRandomNearPosition(6.0f);
            if (Creature* summon = SummonTieredCreature(RiftEntryFrostSpectre, position, 0.45f, 0.65f))
                if (Unit* victim = me->GetVictim())
                    summon->AI()->AttackStart(victim);
        }
    }

    bool _summonedAt55 = false;
    bool _summonedAt30 = false;
};

void AddSC_boss_rift_amnennar()
{
    RegisterCreatureAI(boss_rift_amnennar);
    RegisterCreatureAI(npc_rift_frost_spectre);
}

} // namespace HeroicDungeonRift
