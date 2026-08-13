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
// 祖尔法拉克 - 加兹瑞拉（Gahz'rilla）
enum Events : uint32
{
    EventIcicle = 1,      // 冰柱（T1基础）
    EventFreezeSolid,     // 冰霜凝固（T1基础）
    EventSlam,            // 加兹瑞拉猛击（T1基础）
    EventTier2Skill,      // 痛击（T2新增）
    EventTier3Skill       // 冰霜新星（T3新增）
};

enum Spells : uint32
{
    SpellIcicle = 11131,        // 冰柱
    SpellFreezeSolid = 11836,   // 冰霜凝固
    SpellGahzrillaSlam = 11902, // 加兹瑞拉猛击
    SpellThrash = 3391,         // 痛击
    SpellFrostNova = 12674      // 冰霜新星
};
}

struct boss_rift_gahzrilla : public BossAIBase
{
    explicit boss_rift_gahzrilla(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventIcicle, 5000, 4000, 3200);
        ScheduleTieredEvent(EventFreezeSolid, 16000, 13000, 10500);
        ScheduleTieredEvent(EventSlam, 13000, 10500, 8500);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 9s);  // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 20s); // T3新增
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventIcicle:
                CastIfConfigured(SelectRandomPlayer(), SpellIcicle);
                ScheduleTieredEvent(EventIcicle, 10000, 8000, 6500);
                break;
            case EventFreezeSolid:
                CastIfConfigured(SelectRandomPlayer(), SpellFreezeSolid);
                ScheduleTieredEvent(EventFreezeSolid, 20000, 16000, 13000);
                break;
            case EventSlam:
                CastIfConfigured(me, SpellGahzrillaSlam);
                ScheduleTieredEvent(EventSlam, 30000, 24000, 19000);
                break;
            case EventTier2Skill: // T2新增：痛击，瞬发
                CastIfConfigured(me, SpellThrash, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 12s : 15s);
                break;
            case EventTier3Skill: // T3新增：冰霜新星，瞬发
                CastIfConfigured(me, SpellFrostNova, true);
                events.ScheduleEvent(EventTier3Skill, 22s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_gahzrilla()
{
    RegisterCreatureAI(boss_rift_gahzrilla);
}

} // namespace HeroicDungeonRift
