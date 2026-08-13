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
// 厄运之槌西区 - 托塞德林王子（Prince Tortheldrin）
enum Events : uint32
{
    EventArcaneBlast = 1, // 奥术冲击（T1基础）
    EventWhirlwind,       // 旋风斩（T1基础）
    EventCounterspell,    // 法术反制（T2新增）
    EventTier3Skill       // 冰霜新星（T3新增）
};

enum Spells : uint32
{
    SpellDualWield = 674,       // 双武器
    SpellThrash = 3417,         // 痛击
    SpellArcaneBlast = 22920,   // 奥术冲击
    SpellWhirlwind = 13736,     // 旋风斩
    SpellCounterspell = 20537,  // 法术反制
    SpellFrostNova = 12674      // 冰霜新星
};
}

struct boss_rift_tortheldrin : public BossAIBase
{
    explicit boss_rift_tortheldrin(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        CastIfConfigured(me, SpellDualWield, true);
        CastIfConfigured(me, SpellThrash, true);

        ScheduleTieredEvent(EventArcaneBlast, 6000, 4800, 3800);
        ScheduleTieredEvent(EventWhirlwind, 11000, 9000, 7200);
        if (_tier >= 2)
            events.ScheduleEvent(EventCounterspell, 10s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 14s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventArcaneBlast:
                CastIfConfigured(me->GetVictim(), SpellArcaneBlast);
                ScheduleTieredEvent(EventArcaneBlast, 13000, 10500, 8500);
                break;
            case EventWhirlwind:
                CastIfConfigured(me, SpellWhirlwind);
                ScheduleTieredEvent(EventWhirlwind, 13000, 10500, 8500);
                break;
            case EventCounterspell: // T2新增：法术反制，冲锋带打断，优先打断正在读条的目标
                CastIfConfigured(SelectCastingPlayer(), SpellCounterspell, true);
                events.ScheduleEvent(EventCounterspell, _tier == 3 ? 10s : 12s);
                break;
            case EventTier3Skill: // T3新增：冰霜新星，读条不可打断
                CastIfConfigured(me, SpellFrostNova, true);
                events.ScheduleEvent(EventTier3Skill, 18s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_tortheldrin()
{
    RegisterCreatureAI(boss_rift_tortheldrin);
}

} // namespace HeroicDungeonRift
