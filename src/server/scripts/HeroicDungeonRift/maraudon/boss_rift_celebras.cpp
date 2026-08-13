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
// 玛拉顿 - 被诅咒的塞雷布拉斯（Celebras the Cursed）
enum Events : uint32
{
    EventWrath = 1,            // 愤怒（T1基础）
    EventEntanglingRoots,      // 纠缠根须（T1基础）
    EventTwistedTranquility,   // 扭曲宁静（T2新增）
    EventTier3Skill            // 堕落自然之力（T3新增）
};

enum Spells : uint32
{
    SpellWrath = 21807,             // 愤怒
    SpellEntanglingRoots = 12747,   // 纠缠根须
    SpellTwistedTranquility = 21793,// 扭曲宁静
    SpellCorruptForcesOfNature = 21968 // 堕落自然之力
};
}

struct boss_rift_celebras : public BossAIBase
{
    explicit boss_rift_celebras(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventWrath, 2500, 2000, 1600);
        ScheduleTieredEvent(EventEntanglingRoots, 8000, 6500, 5200);
        if (_tier >= 2)
            events.ScheduleEvent(EventTwistedTranquility, 12s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 14s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventWrath:
                CastIfConfigured(me->GetVictim(), SpellWrath);
                ScheduleTieredEvent(EventWrath, 2800, 2200, 1800);
                break;
            case EventEntanglingRoots:
                CastIfConfigured(SelectRandomPlayer(), SpellEntanglingRoots);
                ScheduleTieredEvent(EventEntanglingRoots, 15000, 12000, 9500);
                break;
            case EventTwistedTranquility: // T2新增：扭曲宁静，瞬发
                CastIfConfigured(me, SpellTwistedTranquility, true);
                events.ScheduleEvent(EventTwistedTranquility, _tier == 3 ? 30s : 38s);
                break;
            case EventTier3Skill: // T3新增：堕落自然之力，瞬发
                CastIfConfigured(me, SpellCorruptForcesOfNature, true);
                events.ScheduleEvent(EventTier3Skill, 30s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_celebras()
{
    RegisterCreatureAI(boss_rift_celebras);
}

} // namespace HeroicDungeonRift
