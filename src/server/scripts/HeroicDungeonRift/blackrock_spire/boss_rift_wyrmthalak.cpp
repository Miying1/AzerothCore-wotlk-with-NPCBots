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
// 黑石塔下层 - 维姆萨拉克（Overlord Wyrmthalak）
enum Events : uint32
{
    EventShout = 1,    // 怒吼/恐惧（T1基础）
    EventCleave,       // 顺劈斩（T1基础）
    EventKnockAway,    // 击退（T1基础）
    EventBlastWave,    // 冲击波（T1基础）
    EventTier3Skill    // 雷霆一击（T3新增）
};

enum Spells : uint32
{
    SpellShout = 23511,      // 怒吼（恐惧）
    SpellCleave = 20691,     // 顺劈斩
    SpellKnockAway = 20686,  // 击退
    SpellBlastWave = 11130,  // 冲击波
    SpellThunderclap = 15588 // 雷霆一击
};

constexpr char const* WyrmthalakHelpText = "你们竟敢挑战龙喉氏族？你们的尸骨将铺满这座大厅！";
}

struct boss_rift_wyrmthalak : public BossAIBase
{
    explicit boss_rift_wyrmthalak(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventShout, 4000, 3200, 2500);
        ScheduleTieredEvent(EventCleave, 7000, 5500, 4500);
        ScheduleTieredEvent(EventKnockAway, 13000, 10500, 8500);
        ScheduleTieredEvent(EventBlastWave, 20000, 16000, 13000);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 10s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_reinforcementsSummoned || !me->HealthBelowPctDamaged(51, damage))
            return;

        _reinforcementsSummoned = true;
        me->Yell(WyrmthalakHelpText, LANG_UNIVERSAL);
        SummonTieredCreature(RiftEntrySpirestoneWarlord, me->GetRandomNearPosition(6.0f), 0.7f, 0.8f);
        SummonTieredCreature(RiftEntrySmolderthornBerserker, me->GetRandomNearPosition(6.0f), 0.7f, 0.8f);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventShout:
                CastIfConfigured(me, SpellShout);
                ScheduleTieredEvent(EventShout, 10000, 8000, 6500);
                break;
            case EventCleave:
                CastIfConfigured(me->GetVictim(), SpellCleave);
                ScheduleTieredEvent(EventCleave, 7000, 5500, 4500);
                break;
            case EventKnockAway:
                CastIfConfigured(me->GetVictim(), SpellKnockAway);
                ScheduleTieredEvent(EventKnockAway, 14000, 11000, 9000);
                break;
            case EventBlastWave:
                CastIfConfigured(me, SpellBlastWave);
                ScheduleTieredEvent(EventBlastWave, 20000, 16000, 13000);
                break;
            case EventTier3Skill: // T3新增：雷霆一击，瞬发
                CastIfConfigured(me, SpellThunderclap, true);
                events.ScheduleEvent(EventTier3Skill, 16s);
                break;
            default:
                break;
        }
    }

private:
    bool _reinforcementsSummoned = false;
};

void AddSC_boss_rift_wyrmthalak()
{
    RegisterCreatureAI(boss_rift_wyrmthalak);
}

} // namespace HeroicDungeonRift
