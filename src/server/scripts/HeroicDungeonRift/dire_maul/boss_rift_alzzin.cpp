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
// 厄运之槌东区 - 奥兹恩（Alzzin the Wildshaper）
enum Events : uint32
{
    EventThorns = 1,  // 荆棘术（Spell 22128，T1原版）
    EventEnervate,    // 弱化（Spell 22661，T1原版）
    EventWither,      // 寒冬（Spell 22662，T1原版）
    EventMangle,      // 裂伤（Spell 22689，T2新增）
    EventViciousBite, // 恶毒之咬（Spell 19319，T3新增）
    EventDisarm       // 缴械（Spell 22691，T3新增）
};

enum Spells : uint32
{
    SpellThorns = 22128,       // 荆棘术（T1原版）
    SpellEnervate = 22661,     // 弱化（T1原版）
    SpellWither = 22662,       // 寒冬（T1原版）
    SpellMangle = 22689,       // 裂伤（T2新增，混合BP：仅覆写BP1周期伤害）
    SpellViciousBite = 19319,  // 恶毒之咬（T3新增）
    SpellDisarm = 22691        // 缴械（T3新增）
};

constexpr int32 MangleTier1DamagePerTick = 1800;
}

struct boss_rift_alzzin : public BossAIBase
{
    explicit boss_rift_alzzin(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventThorns, Milliseconds(8000));
        events.ScheduleEvent(EventEnervate, Milliseconds(12000));
        events.ScheduleEvent(EventWither, Milliseconds(10000));
        if (_tier >= 2)
            events.ScheduleEvent(EventMangle, 7s);
        if (_tier >= 3)
        {
            events.ScheduleEvent(EventViciousBite, 11s);
            events.ScheduleEvent(EventDisarm, 16s);
        }
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*type*/, SpellSchoolMask /*school*/) override
    {
        // 50%生命值阶段：仅触发一次，按Tier在Boss附近直接召唤2/3/4只小怪，无独立施法ID。
        if (_minionsSummoned || !me->HealthBelowPctDamaged(50, damage))
            return;

        _minionsSummoned = true;
        for (uint32 i = 0; i < _tier + 1; ++i)
            SummonTieredCreature(RiftEntryAlzzinMinion, me->GetRandomNearPosition(6.0f), 0.6f, 0.7f);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventThorns:
                CastIfConfigured(me, SpellThorns);
                events.ScheduleEvent(EventThorns, Milliseconds(40000));
                break;
            case EventEnervate:
                CastIfConfigured(SelectRandomPlayer(), SpellEnervate);
                events.ScheduleEvent(EventEnervate, Milliseconds(16000));
                break;
            case EventWither:
                CastIfConfigured(SelectRandomPlayer(), SpellWither);
                events.ScheduleEvent(EventWither, Milliseconds(14000));
                break;
            case EventMangle: // T2新增：裂伤，瞬发；混合BP仅覆写BP1周期伤害
                CastFinalRaidDamageSpell(me->GetVictim(), SpellMangle, SPELLVALUE_BASE_POINT1,
                    MangleTier1DamagePerTick, true);
                events.ScheduleEvent(EventMangle, _tier == 3 ? 10s : 12s);
                break;
            case EventViciousBite: // T3新增：恶毒之咬，瞬发
                CastIfConfigured(me->GetVictim(), SpellViciousBite, true);
                events.ScheduleEvent(EventViciousBite, 12s);
                break;
            case EventDisarm: // T3新增：缴械，瞬发
                CastIfConfigured(me->GetVictim(), SpellDisarm, true);
                events.ScheduleEvent(EventDisarm, 15s);
                break;
            default:
                break;
        }
    }

private:
    bool _minionsSummoned = false;
};

void AddSC_boss_rift_alzzin()
{
    RegisterCreatureAI(boss_rift_alzzin);
}

} // namespace HeroicDungeonRift
