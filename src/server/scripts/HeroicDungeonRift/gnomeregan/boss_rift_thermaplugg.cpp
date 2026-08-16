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
    EventKnockAway = 1, // 击退（原版/T1基础）
    EventWalkingBomb // 召唤行走炸弹（原版/T1基础；T2/T3提高存活上限）
};

enum Spells : uint32
{
    SpellKnockAway = 10101, // 击退（原版/T1基础）：对当前目标施放
    SpellWalkingBombEffect = 11504 // 行走炸弹爆炸（原版/T1基础召唤物技能）：接近目标时对自身施放
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
        events.ScheduleEvent(EventWalkingBomb, Milliseconds(10000));
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventKnockAway:
                CastIfConfigured(me->GetVictim(), SpellKnockAway);
                events.ScheduleEvent(EventKnockAway, Milliseconds(13500));
                break;
            case EventWalkingBomb:
                // 原版/T1召唤物：每次尝试召唤1个行走炸弹；T1/T2/T3存活上限为1/2/3个。
                // 行走炸弹在45秒后或留尸时消失。
                PruneWalkingBombs();
                if (_walkingBombs.size() < _tier)
                    if (Creature* bomb = SummonTieredCreature(RiftEntryWalkingBomb, me->GetRandomNearPosition(8.0f), 1.0f, 1.0f,
                        TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 45 * IN_MILLISECONDS))
                        _walkingBombs.push_back(bomb->GetGUID());
                events.ScheduleEvent(EventWalkingBomb, Milliseconds(15000));
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
        _tier = 1;
        _damagePermille = 1000;
        _exploded = false;
        _targetGuid.Clear();
    }

    void SetData(uint32 id, uint32 value) override
    {
        if (id == RiftDataTier)
            _tier = uint8(std::clamp<uint32>(value, 1, MaxTier));
        else if (id == RiftDataDamagePermille)
            _damagePermille = std::max<uint32>(1, value);
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
        if (RiftSpellDamageTuning const* tuning = GetRiftSpellDamageTuning(SpellWalkingBombEffect))
        {
            CustomSpellValues values;
            for (uint8 effectIndex = 0; effectIndex < tuning->EffectBasePoints.size(); ++effectIndex)
                if (int32 basePoint = tuning->EffectBasePoints[effectIndex])
                {
                    int32 intendedBasePoint = int32(int64(basePoint) * _damagePermille / 1000);
                    values.AddSpellMod(SpellValueMod(SPELLVALUE_BASE_POINT0 + effectIndex),
                        CompensateRiftCreatureLevelScaling(me, SpellWalkingBombEffect, effectIndex,
                            intendedBasePoint));
                }

            if (!values.empty())
                me->CastCustomSpell(SpellWalkingBombEffect, values, me, TRIGGERED_FULL_MASK);
            else
                me->CastSpell(me, SpellWalkingBombEffect, true);
        }
        else
            me->CastSpell(me, SpellWalkingBombEffect, true);

        me->DespawnOrUnsummon(500ms);
    }

private:
    uint8 _tier = 1; // 当前裂隙层数，由SetData(RiftDataTier)写入
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
