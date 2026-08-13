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
// 黑石深渊 - 贝尔加（Bael'Gar）
enum Events : uint32
{
    EventMagmaSplash = 1, // 熔岩喷溅（T1基础）
    EventTier2Skill,      // 火焰冲击（T2新增）
    EventTier3Skill       // 痛击（T3新增）
};

enum Spells : uint32
{
    SpellMagmaSplash = 13880, // 熔岩喷溅
    SpellFireBlast = 13342,   // 火焰冲击
    SpellThrash = 3391        // 痛击
};
}

struct boss_rift_baelgar : public BossAIBase
{
    explicit boss_rift_baelgar(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventMagmaSplash, 9000, 7000, 5500);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 7s);  // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 15s); // T3新增
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        // 原版机制：血量每下降20%召唤贝尔加幼体（82%/62%/42%/22%）
        if (!me->HealthBelowPctDamaged(_nextSpawnThreshold, damage))
            return;

        SummonTieredCreature(RiftEntryBaelGarSpawn, me->GetRandomNearPosition(6.0f), 0.5f, 0.7f);
        SummonTieredCreature(RiftEntryBaelGarSpawn, me->GetRandomNearPosition(6.0f), 0.5f, 0.7f);
        _nextSpawnThreshold -= 20;
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventMagmaSplash:
                CastIfConfigured(me->GetVictim(), SpellMagmaSplash);
                ScheduleTieredEvent(EventMagmaSplash, 12000, 9500, 7500);
                break;
            case EventTier2Skill: // T2新增：火焰冲击，读条不可打断
                CastIfConfigured(me->GetVictim(), SpellFireBlast, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 6s : 8s);
                break;
            case EventTier3Skill: // T3新增：痛击，读条不可打断
                CastIfConfigured(me, SpellThrash, true);
                events.ScheduleEvent(EventTier3Skill, 14s);
                break;
            default:
                break;
        }
    }

private:
    int32 _nextSpawnThreshold = 80;
};

void AddSC_boss_rift_baelgar()
{
    RegisterCreatureAI(boss_rift_baelgar);
}

} // namespace HeroicDungeonRift
