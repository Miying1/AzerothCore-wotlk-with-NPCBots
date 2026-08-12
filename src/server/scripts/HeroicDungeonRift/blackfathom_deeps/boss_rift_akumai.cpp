/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "ScriptMgr.h"

#include <algorithm>
#include <limits>

namespace HeroicDungeonRift
{
namespace
{
enum BossEvents : uint32
{
    EventPoisonCloud = 1,
    EventFrenziedRage,
    EventSnapjawWave,
    EventServantWave
};

enum AddEvents : uint32
{
    EventSnapjawAbility = 1,
    EventServantFrostbolt,
    EventServantFrostNova
};

enum Spells : uint32
{
    SpellPoisonCloud = 3815,
    SpellFrenziedRage = 3490,
    SpellSnapjawAbility = 8391,
    SpellServantFrostbolt = 15043,
    SpellServantFrostNova = 865
};

class AkumaiAddAI : public ScriptedAI
{
public:
    explicit AkumaiAddAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _tier = 1;
        _damagePermille = 1000;
        ScheduleAbilities();
    }

    void IsSummonedBy(WorldObject* summoner) override
    {
        if (Unit* unit = summoner->ToUnit())
            if (Unit* victim = unit->GetVictim())
                AttackStart(victim);
    }

    void SetData(uint32 type, uint32 data) override
    {
        if (type == RiftDataTier)
        {
            _tier = uint8(std::clamp<uint32>(data, 1, MaxTier));
            _events.Reset();
            ScheduleAbilities();
        }
        else if (type == RiftDataDamagePermille)
            _damagePermille = std::max<uint32>(1, data) * 15;
    }

    void DamageDealt(Unit* /*victim*/, uint32& damage, DamageEffectType damageType, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (damageType != DIRECT_DAMAGE)
        {
            uint64 scaledDamage = uint64(damage) * _damagePermille / 1000;
            damage = uint32(std::min<uint64>(scaledDamage, std::numeric_limits<uint32>::max()));
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        _events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
            ExecuteAbility(eventId);

        DoMeleeAttackIfReady();
    }

protected:
    virtual void ScheduleAbilities() = 0;
    virtual void ExecuteAbility(uint32 eventId) = 0;

    uint32 TierDelay(uint32 tier1Delay, uint32 tier2Delay, uint32 tier3Delay) const
    {
        return _tier == 1 ? tier1Delay : (_tier == 2 ? tier2Delay : tier3Delay);
    }

    EventMap _events;
    uint8 _tier = 1;
    uint32 _damagePermille = 1000;
};
}

struct npc_rift_akumai_snapjaw : public AkumaiAddAI
{
    explicit npc_rift_akumai_snapjaw(Creature* creature) : AkumaiAddAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventSnapjawAbility, Milliseconds(TierDelay(7000, 5500, 4000)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventSnapjawAbility)
            return;

        DoCastVictim(SpellSnapjawAbility);
        _events.ScheduleEvent(EventSnapjawAbility, Milliseconds(TierDelay(10000, 7500, 5500)));
    }
};

struct npc_rift_akumai_servant : public AkumaiAddAI
{
    explicit npc_rift_akumai_servant(Creature* creature) : AkumaiAddAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventServantFrostbolt, Milliseconds(TierDelay(4000, 3000, 2200)));
        _events.ScheduleEvent(EventServantFrostNova, Milliseconds(TierDelay(9000, 7500, 6000)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventServantFrostbolt:
                DoCastVictim(SpellServantFrostbolt);
                _events.ScheduleEvent(EventServantFrostbolt, Milliseconds(TierDelay(5500, 4000, 3000)));
                break;
            case EventServantFrostNova:
                DoCast(me, SpellServantFrostNova);
                _events.ScheduleEvent(EventServantFrostNova, Milliseconds(TierDelay(14000, 11000, 8500)));
                break;
            default:
                break;
        }
    }
};

struct boss_rift_akumai : public BossAIBase
{
    explicit boss_rift_akumai(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _snapjawPhaseStarted = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventPoisonCloud, 9000, 7000, 5500);
        ScheduleTieredEvent(EventFrenziedRage, 18000, 15000, 12000);
        if (_tier >= 2)
            events.ScheduleEvent(EventSnapjawWave, 20s);
        if (_tier >= 3)
            events.ScheduleEvent(EventServantWave, 14s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_tier >= 2 && !_snapjawPhaseStarted && me->HealthBelowPctDamaged(65, damage))
        {
            _snapjawPhaseStarted = true;
            SummonWave(RiftEntryAkumaiSnapjaw, _tier == 3 ? 3 : 2, 0.55f, 0.65f);
            events.RescheduleEvent(EventSnapjawWave, _tier == 3 ? 16s : 21s);
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        DespawnRiftSummons();
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventPoisonCloud:
                CastIfConfigured(me->GetVictim(), SpellPoisonCloud);
                ScheduleTieredEvent(EventPoisonCloud, 12000, 9000, 7000);
                break;
            case EventFrenziedRage:
                CastIfConfigured(me, SpellFrenziedRage);
                ScheduleTieredEvent(EventFrenziedRage, 26000, 21000, 17000);
                break;
            case EventSnapjawWave:
                if (_snapjawPhaseStarted)
                    SummonWave(RiftEntryAkumaiSnapjaw, _tier == 3 ? 2 : 1, 0.55f, 0.65f);
                events.ScheduleEvent(EventSnapjawWave, _tier == 3 ? 18s : 24s);
                break;
            case EventServantWave:
                SummonWave(RiftEntryAkumaiServant, 1, 0.6f, 0.8f);
                events.ScheduleEvent(EventServantWave, 20s);
                break;
            default:
                break;
        }
    }

private:
    uint32 CountAlive(uint32 entry) const
    {
        uint32 count = 0;
        for (ObjectGuid const& guid : _riftSummons)
            if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                if (summon->IsAlive() && summon->GetEntry() == entry)
                    ++count;
        return count;
    }

    void SummonWave(uint32 entry, uint32 amount, float healthCoefficient, float damageCoefficient)
    {
        uint32 cap = entry == RiftEntryAkumaiSnapjaw ? (_tier == 2 ? 3 : 5) : 3;
        uint32 alive = CountAlive(entry);
        for (uint32 i = alive; i < cap && amount > 0; ++i, --amount)
        {
            Position position = me->GetRandomNearPosition(8.0f);
            if (Creature* summon = SummonTieredCreature(entry, position, healthCoefficient, damageCoefficient))
                if (Unit* victim = me->GetVictim())
                    summon->AI()->AttackStart(victim);
        }
    }

    bool _snapjawPhaseStarted = false;
};

void AddSC_boss_rift_akumai()
{
    RegisterCreatureAI(boss_rift_akumai);
    RegisterCreatureAI(npc_rift_akumai_snapjaw);
    RegisterCreatureAI(npc_rift_akumai_servant);
}

} // namespace HeroicDungeonRift
