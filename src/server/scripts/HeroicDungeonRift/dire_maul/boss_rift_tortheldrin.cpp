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
    EventArcaneBlast = 1, // 奥术冲击（Spell 22920，T1原版）
    EventWhirlwind,       // 旋风斩（Spell 13736，T1原版）
    EventCounterspell,    // 法术反制（Spell 20537，T2新增）
    EventTier3Skill       // 冰霜新星（Spell 12674，T3新增）
};

enum Spells : uint32
{
    SpellDualWield = 674,       // 双武器（T1原版，开战自施）
    SpellThrash = 3417,         // 痛击（T1原版，开战自施）
    SpellArcaneBlast = 22920,   // 奥术冲击（T1原版）
    SpellWhirlwind = 13736,     // 旋风斩（T1原版）
    SpellCounterspell = 20537,  // 法术反制（T2新增）
    SpellFrostNova = 12674      // 冰霜新星（T3新增，覆写BP0直接伤害）
};

constexpr int32 FrostNovaTier1DirectDamage = 3500;
}

struct boss_rift_tortheldrin : public BossAIBase
{
    explicit boss_rift_tortheldrin(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        CastIfConfigured(me, SpellDualWield, true);
        CastIfConfigured(me, SpellThrash, true);

        events.ScheduleEvent(EventArcaneBlast, Milliseconds(6000));
        events.ScheduleEvent(EventWhirlwind, Milliseconds(11000));
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
                events.ScheduleEvent(EventArcaneBlast, Milliseconds(13000));
                break;
            case EventWhirlwind:
                CastIfConfigured(me, SpellWhirlwind);
                events.ScheduleEvent(EventWhirlwind, Milliseconds(13000));
                break;
            case EventCounterspell: // T2新增：法术反制，冲锋带打断，优先打断正在读条的目标
                CastIfConfigured(SelectCastingPlayer(), SpellCounterspell, true);
                events.ScheduleEvent(EventCounterspell, _tier == 3 ? 10s : 12s);
                break;
            case EventTier3Skill: // T3新增：冰霜新星，瞬发
                CastFinalRaidDamageSpell(me, SpellFrostNova, SPELLVALUE_BASE_POINT0,
                    FrostNovaTier1DirectDamage, true);
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
