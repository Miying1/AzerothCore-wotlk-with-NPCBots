/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "MotionMaster.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <algorithm>

namespace HeroicDungeonRift
{
namespace
{
enum Events : uint32
{
    EventKnockAway = 1,
    EventWalkingBomb
};

enum Spells : uint32
{
    SpellKnockAway = 10101,
    SpellWalkingBombEffect = 11504
};

constexpr float WalkingBombTriggerRange = 5.0f;
constexpr float WalkingBombSearchRange = 100.0f;
}

struct boss_rift_thermaplugg : public BossAIBase
{
    explicit boss_rift_thermaplugg(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventKnockAway, 3s);
        ScheduleTieredEvent(EventWalkingBomb, 10000, 8000, 6000);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventKnockAway:
                CastIfConfigured(me->GetVictim(), SpellKnockAway);
                ScheduleTieredEvent(EventKnockAway, 13500, 11500, 9500);
                break;
            case EventWalkingBomb:
                PruneWalkingBombs();
                if (_walkingBombs.size() < _tier)
                    if (Creature* bomb = SummonTieredCreature(RiftEntryWalkingBomb, me->GetRandomNearPosition(8.0f), 1.0f, 1.0f,
                        TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 45 * IN_MILLISECONDS))
                        _walkingBombs.push_back(bomb->GetGUID());
                ScheduleTieredEvent(EventWalkingBomb, 15000, 11000, 7500);
                break;
            default:
                break;
        }
    }

private:
    void PruneWalkingBombs()
    {
        _walkingBombs.erase(std::remove_if(_walkingBombs.begin(), _walkingBombs.end(), [this](ObjectGuid const& guid)
        {
            Creature* bomb = ObjectAccessor::GetCreature(*me, guid);
            return !bomb || !bomb->IsAlive();
        }), _walkingBombs.end());
    }

    std::vector<ObjectGuid> _walkingBombs;
};

struct npc_rift_walking_bomb : public ScriptedAI
{
    explicit npc_rift_walking_bomb(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _damagePermille = 1000;
        _exploded = false;
        _targetGuid.Clear();
    }

    void SetData(uint32 id, uint32 value) override
    {
        if (id == RiftDataDamagePermille)
            _damagePermille = value * 15;
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->DespawnOrUnsummon(1ms);
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        if (_exploded)
            return;

        Player* target = ObjectAccessor::GetPlayer(*me, _targetGuid);
        if (!target || !target->IsAlive())
        {
            target = me->SelectNearestPlayer(WalkingBombSearchRange);
            if (!target)
                return;

            _targetGuid = target->GetGUID();
            me->GetMotionMaster()->MoveChase(target);
        }

        if (!me->IsWithinDistInMap(target, WalkingBombTriggerRange))
            return;

        _exploded = true;
        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SpellWalkingBombEffect))
        {
            int32 basePoint0 = spellInfo->Effects[EFFECT_0].CalcValue(me);
            if (basePoint0 > 0)
            {
                int32 scaledBasePoint0 = int32(int64(basePoint0) * _damagePermille / 1000);
                me->CastCustomSpell(SpellWalkingBombEffect, SPELLVALUE_BASE_POINT0, scaledBasePoint0, me, true);
            }
            else
                me->CastSpell(me, SpellWalkingBombEffect, true);
        }
        else
            me->CastSpell(me, SpellWalkingBombEffect, true);

        me->DespawnOrUnsummon(500ms);
    }

private:
    uint32 _damagePermille = 1000;
    bool _exploded = false;
    ObjectGuid _targetGuid;
};

void AddSC_boss_rift_thermaplugg()
{
    RegisterCreatureAI(boss_rift_thermaplugg);
}

void AddSC_npc_rift_walking_bomb()
{
    RegisterCreatureAI(npc_rift_walking_bomb);
}

} // namespace HeroicDungeonRift
