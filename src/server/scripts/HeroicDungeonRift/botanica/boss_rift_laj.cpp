/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "ScriptMgr.h"

#include <array>

namespace HeroicDungeonRift
{
namespace
{
// 生态船 - 拉伊（Laj）
enum Events : uint32
{
    EventAllergicReaction = 1, // 过敏反应（T1基础）
    EventTransform,            // 元素形态（T1基础）
    EventSummonPlants,         // 传送并准备召唤荆棘植物（T1基础）
    EventSummonPlantsComplete, // 原版传送后2.5秒完成召唤
    EventArcaneResonance,      // 奥术共鸣（T2新增）
    EventArcaneDevastation     // 奥术毁灭（T3新增）
};

enum Spells : uint32
{
    SpellAllergicReaction = 34697,
    SpellTeleportSelf = 34673,
    SpellDamageImmuneArcane = 34304,
    SpellDamageImmuneFire = 34305,
    SpellDamageImmuneFrost = 34306,
    SpellDamageImmuneNature = 34308,
    SpellDamageImmuneShadow = 34309,
    SpellArcaneResonance = 34794,
    SpellArcaneDevastation = 34799,
    SpellThornShot = 34745,
    SpellMindFlay = 35507
};

enum Entries : uint32
{
    RiftEntryThornLasher = 102047, // source 19919
    RiftEntryThornFlayer = 102048  // source 19920
};

enum Models : uint32
{
    ModelDefault = 13109,
    ModelArcane = 14213,
    ModelFire = 13110,
    ModelFrost = 14112,
    ModelNature = 14214
};

struct TransformData
{
    uint32 SpellId;
    uint32 ModelId;
};

constexpr std::array<TransformData, 5> LajTransforms = {{
    { SpellDamageImmuneShadow, ModelDefault },
    { SpellDamageImmuneArcane, ModelArcane },
    { SpellDamageImmuneFire, ModelFire },
    { SpellDamageImmuneFrost, ModelFrost },
    { SpellDamageImmuneNature, ModelNature }
}};

constexpr char const* LajSummonText = "%s发出奇怪的声音。";
}

struct npc_rift_thorn_lasher : public RiftLevel70SummonAI
{
    explicit npc_rift_thorn_lasher(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(1, 3s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != 1)
            return;

        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
            DoCast(target, SpellThornShot); // 荆棘射击
        _events.ScheduleEvent(1, 4s);
    }
};

struct npc_rift_thorn_flayer : public RiftLevel70SummonAI
{
    explicit npc_rift_thorn_flayer(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(1, 5s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != 1)
            return;

        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
            DoCast(target, SpellMindFlay); // 精神鞭笞
        _events.ScheduleEvent(1, 7s);
    }
};

struct boss_rift_laj : public BossAIBase
{
    explicit boss_rift_laj(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        _transformIndex = 0;
        BossAIBase::Reset();
        if (_tier)
        {
            me->SetDisplayId(LajTransforms[0].ModelId);
            CastIfConfigured(me, LajTransforms[0].SpellId, true);
        }
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventAllergicReaction, 5s);
        events.ScheduleEvent(EventTransform, 30s);
        events.ScheduleEvent(EventSummonPlants, 20s);
        if (_tier >= 2)
            events.ScheduleEvent(EventArcaneResonance, 13s);
        if (_tier >= 3)
            events.ScheduleEvent(EventArcaneDevastation, 27s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(3.5f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventAllergicReaction:
                CastIfConfigured(me->GetVictim(), SpellAllergicReaction);
                events.ScheduleEvent(EventAllergicReaction, 25s);
                break;
            case EventTransform:
            {
                me->RemoveAurasDueToSpell(LajTransforms[_transformIndex].SpellId);
                uint8 nextIndex = urand(0, LajTransforms.size() - 2);
                if (nextIndex >= _transformIndex)
                    ++nextIndex;
                _transformIndex = nextIndex;
                me->SetDisplayId(LajTransforms[_transformIndex].ModelId);
                CastIfConfigured(me, LajTransforms[_transformIndex].SpellId, true);
                events.ScheduleEvent(EventTransform, 35s);
                break;
            }
            case EventSummonPlants:
                CastIfConfigured(me, SpellTeleportSelf);
                me->SetReactState(REACT_PASSIVE);
                me->GetMotionMaster()->Clear();
                events.ScheduleEvent(EventSummonPlantsComplete, 2500ms);
                events.ScheduleEvent(EventSummonPlants, 30s);
                break;
            case EventSummonPlantsComplete:
                me->TextEmote(LajSummonText);
                for (uint32 i = 0; i < 2 * _tier; ++i)
                {
                    uint32 entry = i % 2 ? RiftEntryThornFlayer : RiftEntryThornLasher;
                    SummonTieredCreature(entry, me->GetRandomNearPosition(7.0f), 0.45f, 0.65f);
                }
                me->SetReactState(REACT_AGGRESSIVE);
                me->ResumeChasingVictim();
                break;
            case EventArcaneResonance: // T2新增：生态船奥术共鸣
                CastIfConfigured(SelectRandomPlayer(), SpellArcaneResonance, true);
                events.ScheduleEvent(EventArcaneResonance, _tier == 3 ? 14s : 18s);
                break;
            case EventArcaneDevastation: // T3新增：生态船奥术毁灭
                CastIfConfigured(SelectRandomPlayer(), SpellArcaneDevastation, true);
                events.ScheduleEvent(EventArcaneDevastation, 22s);
                break;
            default:
                break;
        }
    }

private:
    uint8 _transformIndex = 0;
};

void AddSC_boss_rift_laj()
{
    RegisterCreatureAI(boss_rift_laj);
    RegisterCreatureAI(npc_rift_thorn_lasher);
    RegisterCreatureAI(npc_rift_thorn_flayer);
}

} // namespace HeroicDungeonRift
