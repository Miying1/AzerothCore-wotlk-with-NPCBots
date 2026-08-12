/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace HeroicDungeonRift
{
namespace
{
enum BossEvents : uint32
{
    EventEarthgrabTotem = 1,
    EventFlamestrike,
    EventHealingWave,
    EventHex
};

enum TotemEvents : uint32
{
    EventEarthgrab = 1
};

enum Spells : uint32
{
    SpellEarthgrab = 8377,
    SpellFlamestrike = 12468,
    SpellHealingWave = 12492,
    SpellHexAura = 12479,
    SpellHexTransform = 12480,
    SpellHexCharm = 12483,
    SpellGreenChanneling = 13540
};
}

struct npc_rift_earthgrab_totem : public ScriptedAI
{
    explicit npc_rift_earthgrab_totem(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _tier = 1;
        me->SetReactState(REACT_PASSIVE);
        _events.ScheduleEvent(EventEarthgrab, 1s);
    }

    void SetData(uint32 type, uint32 data) override
    {
        if (type == RiftDataTier)
        {
            _tier = uint8(std::clamp<uint32>(data, 1, MaxTier));
            _events.RescheduleEvent(EventEarthgrab, 1s);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        if (_events.ExecuteEvent() == EventEarthgrab)
        {
            DoCast(me, SpellEarthgrab);
            _events.ScheduleEvent(EventEarthgrab, _tier == 1 ? 15s : (_tier == 2 ? 12s : 10s));
        }
    }

private:
    EventMap _events;
    uint8 _tier = 1;
};

struct boss_rift_jammalan : public BossAIBase
{
    explicit boss_rift_jammalan(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        ClearHexAuras();
        BossAIBase::Reset();
        _lowHealthTriggered = false;
        if (_tierConfig)
            CastIfConfigured(me, SpellGreenChanneling, true);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventEarthgrabTotem, 7000, 5500, 4200);
        ScheduleTieredEvent(EventFlamestrike, 5000, 4000, 3000);
        ScheduleTieredEvent(EventHealingWave, 9000, 7500, 6000);
        ScheduleTieredEvent(EventHex, 12000, 10000, 8000);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!_lowHealthTriggered && me->HealthBelowPctDamaged(10, damage))
        {
            _lowHealthTriggered = true;
            CastIfConfigured(me, SpellHealingWave);
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        ClearHexAuras();
        DespawnRiftSummons();
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventEarthgrabTotem:
                SummonTotem();
                ScheduleTieredEvent(EventEarthgrabTotem, 30000, 25000, 20000);
                break;
            case EventFlamestrike:
                CastIfConfigured(SelectRandomPlayer(30.0f), SpellFlamestrike);
                ScheduleTieredEvent(EventFlamestrike, 16000, 13000, 10000);
                break;
            case EventHealingWave:
                if (me->HealthBelowPct(85))
                    CastIfConfigured(me, SpellHealingWave);
                ScheduleTieredEvent(EventHealingWave, 11000, 9000, 7000);
                break;
            case EventHex:
                CastIfConfigured(SelectRandomPlayer(30.0f), SpellHexAura);
                ScheduleTieredEvent(EventHex, 40000, 34000, 28000);
                break;
            default:
                break;
        }
    }

private:
    uint32 CountTotems() const
    {
        uint32 count = 0;
        for (ObjectGuid const& guid : _riftSummons)
            if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                if (summon->IsAlive() && summon->GetEntry() == RiftEntryEarthgrabTotem)
                    ++count;
        return count;
    }

    void SummonTotem()
    {
        uint32 cap = _tier == 3 ? 2 : 1;
        if (CountTotems() >= cap)
            return;
        SummonTieredCreature(RiftEntryEarthgrabTotem, me->GetRandomNearPosition(7.0f), 0.2f, 1.0f,
            TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 35 * IN_MILLISECONDS);
    }

    void ClearHexAuras()
    {
        Map::PlayerList const& players = me->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* player = itr->GetSource();
            if (!player || !me->IsWithinDistInMap(player, 120.0f))
                continue;
            player->RemoveAurasDueToSpell(SpellHexAura);
            player->RemoveAurasDueToSpell(SpellHexTransform);
            player->RemoveAurasDueToSpell(SpellHexCharm);
        }
    }

    bool _lowHealthTriggered = false;
};

void AddSC_boss_rift_jammalan()
{
    RegisterCreatureAI(boss_rift_jammalan);
    RegisterCreatureAI(npc_rift_earthgrab_totem);
}

} // namespace HeroicDungeonRift
