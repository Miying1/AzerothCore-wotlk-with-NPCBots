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
    EventChainBolt = 1, // 连锁闪电（原版/T1基础）
    EventMegavolt, // 百万伏特（原版/T1基础）
    EventShock // 电击（原版/T1基础）
};

enum Spells : uint32
{
    SpellShock = 11084, // 电击（原版/T1基础）：随机选择20码内玩家施放
    SpellMegavolt = 11082, // 百万伏特（原版/T1基础）：对当前目标施放
    SpellChainBolt = 11085 // 连锁闪电（原版/T1基础）：对当前目标施放
};
}

struct boss_rift_electrocutioner : public BossAIBase
{
    explicit boss_rift_electrocutioner(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventChainBolt, 3s);
        events.ScheduleEvent(EventMegavolt, 10s);
        events.ScheduleEvent(EventShock, 17s);
    }

    // 裂隙伤害校准：原版法术的非直接伤害在通用Tier倍率外补偿15倍。
    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventChainBolt:
                CastIfConfigured(me->GetVictim(), SpellChainBolt);
                events.ScheduleEvent(EventChainBolt, Milliseconds(21000));
                break;
            case EventMegavolt:
                CastIfConfigured(me->GetVictim(), SpellMegavolt);
                events.ScheduleEvent(EventMegavolt, Milliseconds(21000));
                break;
            case EventShock:
                CastIfConfigured(SelectRandomPlayer(20.0f), SpellShock);
                events.ScheduleEvent(EventShock, Milliseconds(21000));
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_electrocutioner()
{
    RegisterCreatureAI(boss_rift_electrocutioner);
}

} // namespace HeroicDungeonRift
