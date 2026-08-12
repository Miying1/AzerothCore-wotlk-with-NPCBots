/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"

#include <algorithm>
#include <limits>

namespace HeroicDungeonRift
{
namespace
{
enum BossEvents : uint32
{
    EventGroundTremor = 1,
    EventSupportWave
};

enum GuardEvents : uint32
{
    EventGuardianWhirlwind = 1,
    EventWarderTrample,
    EventHallshaperHeal
};

enum Spells : uint32
{
    SpellGroundTremor = 6524,
    SpellGuardianWhirlwind = 17207,
    SpellWarderTrample = 5568,
    SpellHallshaperHealVisual = 10260
};

class EarthenGuardAI : public ScriptedAI
{
public:
    explicit EarthenGuardAI(Creature* creature) : ScriptedAI(creature) { }

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
    virtual void ScheduleAbilities() { }
    virtual void ExecuteAbility(uint32 /*eventId*/) { }

    uint32 TierDelay(uint32 tier1Delay, uint32 tier2Delay, uint32 tier3Delay) const
    {
        return _tier == 1 ? tier1Delay : (_tier == 2 ? tier2Delay : tier3Delay);
    }

    EventMap _events;
    uint8 _tier = 1;
    uint32 _damagePermille = 1000;
};
}

struct npc_rift_earthen_guardian : public EarthenGuardAI
{
    explicit npc_rift_earthen_guardian(Creature* creature) : EarthenGuardAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventGuardianWhirlwind, Milliseconds(TierDelay(9000, 7000, 5500)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventGuardianWhirlwind)
            return;

        DoCast(me, SpellGuardianWhirlwind);
        _events.ScheduleEvent(EventGuardianWhirlwind, Milliseconds(TierDelay(13000, 10000, 7500)));
    }
};

struct npc_rift_vault_warder : public EarthenGuardAI
{
    explicit npc_rift_vault_warder(Creature* creature) : EarthenGuardAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventWarderTrample, Milliseconds(TierDelay(8000, 6500, 5000)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventWarderTrample)
            return;

        DoCast(me, SpellWarderTrample);
        _events.ScheduleEvent(EventWarderTrample, Milliseconds(TierDelay(12000, 9000, 7000)));
    }
};

struct npc_rift_earthen_hallshaper : public EarthenGuardAI
{
    explicit npc_rift_earthen_hallshaper(Creature* creature) : EarthenGuardAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventHallshaperHeal, Milliseconds(TierDelay(9000, 7500, 6000)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventHallshaperHeal)
            return;

        if (TempSummon* summon = me->ToTempSummon())
        {
            if (Unit* owner = summon->GetSummonerUnit())
            {
                if (owner->IsAlive() && owner->IsInCombat() && owner->GetDistance(me) <= 60.0f)
                {
                    uint32 healAmount = owner->CountPctFromMaxHealth(_tier == 1 ? 2 : (_tier == 2 ? 3 : 4));
                    if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SpellHallshaperHealVisual))
                    {
                        HealInfo healInfo(me, owner, healAmount, spellInfo, spellInfo->GetSchoolMask());
                        me->HealBySpell(healInfo);
                    }
                    else
                        Unit::DealHeal(me, owner, healAmount);
                }
            }
        }

        _events.ScheduleEvent(EventHallshaperHeal, Milliseconds(TierDelay(12000, 9500, 7500)));
    }
};

struct npc_rift_earthen_custodian : public EarthenGuardAI
{
    explicit npc_rift_earthen_custodian(Creature* creature) : EarthenGuardAI(creature) { }
};

struct boss_rift_archaedas : public BossAIBase
{
    explicit boss_rift_archaedas(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _guardianWaveTriggered = false;
        _warderWaveTriggered = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventGroundTremor, 11000, 8500, 6500);
        events.ScheduleEvent(EventSupportWave, _tier == 1 ? 24s : (_tier == 2 ? 19s : 15s));
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!_guardianWaveTriggered && me->HealthBelowPctDamaged(70, damage))
        {
            _guardianWaveTriggered = true;
            SummonGuards(RiftEntryEarthenGuardian, _tier, 0.65f, 0.7f);
        }

        if (!_warderWaveTriggered && me->HealthBelowPctDamaged(40, damage))
        {
            _warderWaveTriggered = true;
            SummonGuards(RiftEntryVaultWarder, _tier == 1 ? 1 : _tier - 1, 0.75f, 0.8f);
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
            case EventGroundTremor:
                CastIfConfigured(me, SpellGroundTremor);
                ScheduleTieredEvent(EventGroundTremor, 14000, 10500, 8000);
                break;
            case EventSupportWave:
                SummonGuards(RiftEntryEarthenHallshaper, 1, 0.55f, 0.7f);
                SummonGuards(RiftEntryEarthenCustodian, _tier == 3 ? 2 : 1, 0.7f, 0.8f);
                events.ScheduleEvent(EventSupportWave, _tier == 1 ? 30s : (_tier == 2 ? 23s : 18s));
                break;
            default:
                break;
        }
    }

private:
    uint32 GetEntryCap(uint32 entry) const
    {
        switch (entry)
        {
            case RiftEntryEarthenGuardian:
                return _tier == 1 ? 1 : (_tier == 2 ? 2 : 3);
            case RiftEntryVaultWarder:
                return _tier == 1 ? 1 : (_tier == 2 ? 1 : 2);
            case RiftEntryEarthenHallshaper:
                return _tier == 1 ? 1 : (_tier == 2 ? 2 : 3);
            case RiftEntryEarthenCustodian:
                return _tier == 1 ? 2 : (_tier == 2 ? 3 : 4);
            default:
                return 0;
        }
    }

    uint32 CountAlive(uint32 entry) const
    {
        uint32 count = 0;
        for (ObjectGuid const& guid : _riftSummons)
            if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                if (summon->IsAlive() && summon->GetEntry() == entry)
                    ++count;
        return count;
    }

    void SummonGuards(uint32 entry, uint32 amount, float healthCoefficient, float damageCoefficient)
    {
        uint32 cap = GetEntryCap(entry);
        uint32 alive = CountAlive(entry);
        for (uint32 i = alive; i < cap && amount > 0; ++i, --amount)
        {
            Position position = me->GetRandomNearPosition(10.0f);
            if (Creature* summon = SummonTieredCreature(entry, position, healthCoefficient, damageCoefficient))
                if (Unit* victim = me->GetVictim())
                    summon->AI()->AttackStart(victim);
        }
    }

    bool _guardianWaveTriggered = false;
    bool _warderWaveTriggered = false;
};

void AddSC_boss_rift_archaedas()
{
    RegisterCreatureAI(boss_rift_archaedas);
    RegisterCreatureAI(npc_rift_earthen_guardian);
    RegisterCreatureAI(npc_rift_vault_warder);
    RegisterCreatureAI(npc_rift_earthen_hallshaper);
    RegisterCreatureAI(npc_rift_earthen_custodian);
}

} // namespace HeroicDungeonRift
