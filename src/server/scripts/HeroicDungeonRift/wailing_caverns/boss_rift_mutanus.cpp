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
    EventTerrify = 1,          // 恐吓（T1基础）
    EventNaralexsNightmare,    // 纳拉雷克斯的梦魇（T1基础）
    EventThundercrack,         // 雷霆震裂（T1基础）
    EventTier2Skill,           // 痛击（T2新增）
    EventTier3Skill            // 纠缠根须（T3新增）
};

enum Spells : uint32
{
    SpellTerrify = 7399,          // 恐吓
    SpellNaralexsNightmare = 7967,// 纳拉雷克斯的梦魇
    SpellThundercrack = 8150,     // 雷霆震裂
    SpellThrash = 3391,           // 痛击
    SpellEntanglingRoots = 12747  // 纠缠根须
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
        ScheduleTieredEvent(EventTerrify, 9000, 7000, 5500);
        ScheduleTieredEvent(EventNaralexsNightmare, 14000, 10500, 8000);
        ScheduleTieredEvent(EventThundercrack, 6000, 5000, 4000);
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
                ScheduleTieredEvent(EventTerrify, 22000, 17500, 14000);
                break;
            case EventNaralexsNightmare:
                CastIfConfigured(SelectRandomPlayer(), SpellNaralexsNightmare);
                ScheduleTieredEvent(EventNaralexsNightmare, 33000, 26000, 21000);
                break;
            case EventThundercrack:
                CastRaidTunedSpell(me, SpellThundercrack, ThundercrackRaidDamage);
                ScheduleTieredEvent(EventThundercrack, 17000, 14000, 11000);
                break;
            case EventTier2Skill: // T2新增：痛击，瞬发
                CastIfConfigured(me, SpellThrash, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 12s : 15s);
                break;
            case EventTier3Skill: // T3新增：纠缠根须，点名随机目标，免疫打断
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
