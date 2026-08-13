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
    EventThorns = 1,  // 荆棘术（T1基础）
    EventEnervate,    // 弱化（T1基础）
    EventWither,      // 寒冬（T1基础）
    EventMangle,      // 裂伤（T2新增）
    EventViciousBite, // 恶毒之咬（T3新增）
    EventDisarm       // 缴械（T3新增）
};

enum Spells : uint32
{
    SpellThorns = 22128,       // 荆棘术
    SpellEnervate = 22661,     // 弱化
    SpellWither = 22662,       // 寒冬
    SpellMangle = 22689,       // 裂伤
    SpellViciousBite = 19319,  // 恶毒之咬
    SpellDisarm = 22691        // 缴械
};
}

struct boss_rift_alzzin : public BossAIBase
{
    explicit boss_rift_alzzin(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventThorns, 8000, 6500, 5200);
        ScheduleTieredEvent(EventEnervate, 12000, 9500, 7500);
        ScheduleTieredEvent(EventWither, 10000, 8000, 6500);
        if (_tier >= 2)
            events.ScheduleEvent(EventMangle, 7s);
        if (_tier >= 3)
        {
            events.ScheduleEvent(EventViciousBite, 11s);
            events.ScheduleEvent(EventDisarm, 16s);
        }
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
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
                ScheduleTieredEvent(EventThorns, 40000, 32000, 26000);
                break;
            case EventEnervate:
                CastIfConfigured(SelectRandomPlayer(), SpellEnervate);
                ScheduleTieredEvent(EventEnervate, 16000, 13000, 10500);
                break;
            case EventWither:
                CastIfConfigured(SelectRandomPlayer(), SpellWither);
                ScheduleTieredEvent(EventWither, 14000, 11000, 9000);
                break;
            case EventMangle: // T2新增：裂伤，瞬发
                CastIfConfigured(me->GetVictim(), SpellMangle, true);
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
