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
enum Events : uint32
{
    EventBattleShout = 1, // 战斗怒吼（原版/T1基础）
    EventThunderclap // 雷霆一击（原版/T1基础）
};

enum Spells : uint32
{
    SpellBattleShout = 9128, // 战斗怒吼（原版/T1基础）：对自身施放
    SpellThunderclap = 15548 // 雷霆一击（原版/T1基础）：使用裂隙伤害校准施放
};
// 裂隙伤害校准：雷霆一击以2500点覆盖基础点0，后续仍统一应用Tier伤害倍率。
constexpr int32 ThunderclapRaidDamage = 2500;
}

struct boss_rift_ramtusk : public BossAIBase
{
    explicit boss_rift_ramtusk(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventBattleShout, 2s);
        events.ScheduleEvent(EventThunderclap, 4s);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventBattleShout:
                CastIfConfigured(me, SpellBattleShout);
                events.ScheduleEvent(EventBattleShout, Milliseconds(30000));
                break;
            case EventThunderclap:
                CastRaidTunedSpell(me, SpellThunderclap, ThunderclapRaidDamage);
                events.ScheduleEvent(EventThunderclap, Milliseconds(13000));
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_ramtusk()
{
    RegisterCreatureAI(boss_rift_ramtusk);
}

} // namespace HeroicDungeonRift
