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
// 哀嚎洞穴 - 吞噬者穆坦努斯（Mutanus the Devourer）
enum Events : uint32
{
    EventTerrify = 1,          // 恐吓（原版/T1基础）
    EventNaralexsNightmare,    // 纳拉雷克斯的梦魇（原版/T1基础）
    EventThundercrack,         // 雷霆震裂（原版/T1基础）
    EventTier2Skill,           // 痛击（T2新增）
    EventTier3Skill            // 纠缠根须（T3新增）
};

enum Spells : uint32
{
    SpellTerrify = 7399,          // 恐吓（原版/T1基础）
    SpellNaralexsNightmare = 7967,// 纳拉雷克斯的梦魇（原版/T1基础）
    SpellThundercrack = 8150,     // 雷霆震裂（原版/T1基础）
    SpellThrash = 3391,           // 痛击（T2新增）
    SpellEntanglingRoots = 12747  // 纠缠根须（T3新增，混合BP：仅覆写BP1周期伤害）
};

enum RaidTunedDamage : int32
{
    ThundercrackRaidDamage = 2000 // 雷霆震裂83级基础伤害
};
}

struct boss_rift_mutanus : public BossAIBase
{
    explicit boss_rift_mutanus(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventTerrify, Milliseconds(9000));
        events.ScheduleEvent(EventNaralexsNightmare, Milliseconds(14000));
        events.ScheduleEvent(EventThundercrack, Milliseconds(6000));
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 8s);  // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 16s); // T3新增
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); AddInterruptImmuneSpell(SpellEntanglingRoots); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventTerrify:
                CastIfConfigured(me->GetVictim(), SpellTerrify);
                events.ScheduleEvent(EventTerrify, Milliseconds(22000));
                break;
            case EventNaralexsNightmare:
                CastIfConfigured(SelectRandomPlayer(), SpellNaralexsNightmare);
                events.ScheduleEvent(EventNaralexsNightmare, Milliseconds(33000));
                break;
            case EventThundercrack:
                CastRaidTunedSpell(me, SpellThundercrack, ThundercrackRaidDamage);
                events.ScheduleEvent(EventThundercrack, Milliseconds(17000));
                break;
            case EventTier2Skill: // T2新增：痛击，瞬发
                CastIfConfigured(me, SpellThrash, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 12s : 15s);
                break;
            case EventTier3Skill: // T3新增：纠缠根须，点名随机目标，免疫打断
                // 混合BP：BP0为0不覆写，BP1的T1基线直接取自调谐表（12747）。
                CastIfConfigured(SelectRandomPlayer(), SpellEntanglingRoots);
                events.ScheduleEvent(EventTier3Skill, 18s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_mutanus()
{
    RegisterCreatureAI(boss_rift_mutanus);
}

} // namespace HeroicDungeonRift
