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
    EventStrike = 1,     // 打击（T1基础）
    EventShieldSlam,     // 盾牌猛击（T1基础）
    EventCleave,         // 顺劈斩（T1基础）
    EventShieldCharge,   // 盾牌冲锋（T2新增）
    EventShieldBash      // 盾击（T3新增）
};

enum Spells : uint32
{
    SpellStrike = 15580,      // 打击
    SpellShieldSlam = 15655,  // 盾牌猛击
    SpellCleave = 20691,      // 顺劈斩
    SpellShieldCharge = 15749,// 盾牌冲锋
    SpellShieldBash = 11972,  // 盾击
    SpellFrenzy = 8269        // 狂乱
};
}

struct boss_rift_moldar : public BossAIBase
{
    explicit boss_rift_moldar(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventStrike, 5000, 4000, 3200);
        ScheduleTieredEvent(EventShieldSlam, 9000, 7000, 5500);
        ScheduleTieredEvent(EventCleave, 12000, 9500, 7500);
        if (_tier >= 2)
            events.ScheduleEvent(EventShieldCharge, 8s);
        if (_tier >= 3)
            events.ScheduleEvent(EventShieldBash, 11s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
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
                ScheduleTieredEvent(EventStrike, 7000, 5500, 4500);
                break;
            case EventShieldSlam:
                CastIfConfigured(me->GetVictim(), SpellShieldSlam);
                ScheduleTieredEvent(EventShieldSlam, 11000, 9000, 7200);
                break;
            case EventCleave:
                CastIfConfigured(me->GetVictim(), SpellCleave);
                ScheduleTieredEvent(EventCleave, 10000, 8000, 6500);
                break;
            case EventShieldCharge: // T2新增：盾牌冲锋，选随机目标
                CastIfConfigured(SelectRandomPlayer(), SpellShieldCharge, true);
                events.ScheduleEvent(EventShieldCharge, _tier == 3 ? 12s : 15s);
                break;
            case EventShieldBash: // T3新增：盾击，冲锋带打断，优先打断正在读条的目标
                CastIfConfigured(SelectCastingPlayer(), SpellShieldBash, true);
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
