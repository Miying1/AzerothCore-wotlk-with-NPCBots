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
    EventShout = 1,    // 怒吼/恐惧（原版/T1基础）
    EventCleave,       // 顺劈斩（原版/T1基础）
    EventKnockAway,    // 击退（原版/T1基础）
    EventBlastWave,    // 冲击波（原版/T1基础）
    EventTier3Skill    // 雷霆一击（T3新增）
};

enum Spells : uint32
{
    SpellShout = 23511,      // 怒吼（原版/T1基础，恐惧）
    SpellCleave = 20691,     // 顺劈斩（原版/T1基础）
    SpellKnockAway = 20686,  // 击退（原版/T1基础）
    SpellBlastWave = 11130,  // 冲击波（原版/T1基础）
    SpellThunderclap = 15588 // 雷霆一击（T3新增）
};

constexpr int32 ThunderclapTier1DirectDamage = 3500;

constexpr char const* WyrmthalakHelpText = "你们竟敢挑战龙喉氏族？你们的尸骨将铺满这座大厅！";
}

struct boss_rift_wyrmthalak : public BossAIBase
{
    explicit boss_rift_wyrmthalak(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _reinforcementsSummoned = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventShout, 4000ms);
        events.ScheduleEvent(EventCleave, 7000ms);
        events.ScheduleEvent(EventKnockAway, 13000ms);
        events.ScheduleEvent(EventBlastWave, 20000ms);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 10s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        // 原版/T1阶段召唤：血量低于51%时同时召唤2名援军。
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
                events.ScheduleEvent(EventShout, 10000ms);
                break;
            case EventCleave:
                CastIfConfigured(me->GetVictim(), SpellCleave);
                events.ScheduleEvent(EventCleave, 7000ms);
                break;
            case EventKnockAway:
                CastIfConfigured(me->GetVictim(), SpellKnockAway);
                events.ScheduleEvent(EventKnockAway, 14000ms);
                break;
            case EventBlastWave:
                CastIfConfigured(me, SpellBlastWave);
                events.ScheduleEvent(EventBlastWave, 20000ms);
                break;
            case EventTier3Skill: // T3新增：雷霆一击，瞬发
                CastFinalRaidDamageSpell(me, SpellThunderclap, SPELLVALUE_BASE_POINT0,
                    ThunderclapTier1DirectDamage, true);
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
