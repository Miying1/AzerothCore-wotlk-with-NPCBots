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
// 厄运之槌北区 - 卫兵摩尔达（Guard Mol'dar）
enum Events : uint32
{
    EventStrike = 1,     // 打击（Spell 15580，T1原版）
    EventShieldSlam,     // 盾牌猛击（Spell 15655，T1原版）
    EventCleave,         // 顺劈斩（Spell 20691，T1原版）
    EventShieldCharge,   // 盾牌冲锋（Spell 15749，T2新增）
    EventShieldBash      // 盾击（Spell 11972，T3新增）
};

enum Spells : uint32
{
    SpellStrike = 15580,      // 打击（T1原版）
    SpellShieldSlam = 15655,  // 盾牌猛击（T1原版）
    SpellCleave = 20691,       // 顺劈斩（T1原版）
    SpellShieldCharge = 15749, // 盾牌冲锋（T2新增）
    SpellShieldBash = 11972,  // 盾击（T3新增，混合BP：BP0打断、仅覆写BP1伤害）
    SpellFrenzy = 8269        // 狂乱（原版50%生命值阶段）
};

constexpr int32 ShieldBashTier1DirectDamage = 3500;
}

struct boss_rift_moldar : public BossAIBase
{
    explicit boss_rift_moldar(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _frenzied = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventStrike, Milliseconds(5000));
        events.ScheduleEvent(EventShieldSlam, Milliseconds(9000));
        events.ScheduleEvent(EventCleave, Milliseconds(12000));
        if (_tier >= 2)
            events.ScheduleEvent(EventShieldCharge, 8s);
        if (_tier >= 3)
            events.ScheduleEvent(EventShieldBash, 11s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_frenzied || !me->HealthBelowPctDamaged(50, damage))
            return;

        _frenzied = true;
        CastIfConfigured(me, SpellFrenzy);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventStrike:
                CastIfConfigured(me->GetVictim(), SpellStrike);
                events.ScheduleEvent(EventStrike, Milliseconds(7000));
                break;
            case EventShieldSlam:
                CastIfConfigured(me->GetVictim(), SpellShieldSlam);
                events.ScheduleEvent(EventShieldSlam, Milliseconds(11000));
                break;
            case EventCleave:
                CastIfConfigured(me->GetVictim(), SpellCleave);
                events.ScheduleEvent(EventCleave, Milliseconds(10000));
                break;
            case EventShieldCharge: // T2新增：盾牌冲锋，选随机目标
                CastIfConfigured(SelectRandomPlayer(), SpellShieldCharge, true);
                events.ScheduleEvent(EventShieldCharge, _tier == 3 ? 12s : 15s);
                break;
            case EventShieldBash: // T3新增：混合BP，保留BP0打断、仅覆写BP1伤害；优先读条目标
                CastFinalRaidDamageSpell(SelectCastingPlayer(), SpellShieldBash, SPELLVALUE_BASE_POINT1,
                    ShieldBashTier1DirectDamage, true);
                events.ScheduleEvent(EventShieldBash, 12s);
                break;
            default:
                break;
        }
    }

private:
    bool _frenzied = false;
};

void AddSC_boss_rift_moldar()
{
    RegisterCreatureAI(boss_rift_moldar);
}

} // namespace HeroicDungeonRift
