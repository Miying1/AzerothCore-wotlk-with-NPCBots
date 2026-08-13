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
// 斯坦索姆 - 巴纳扎尔（Balnazzar）
enum Events : uint32
{
    EventMindBlast = 1,    // 心灵震爆（T1基础）
    EventShadowShock,      // 暗影震击（T1基础）
    EventSleep,            // 催眠术（T1基础）
    EventPsychicScream,    // 心灵尖啸（T2新增）
    EventTier3Skill        // 暗影箭雨（T3新增）
};

enum Spells : uint32
{
    SpellMindBlast = 17287,       // 心灵震爆
    SpellShadowShock = 17399,     // 暗影震击
    SpellSleep = 12098,           // 催眠术
    SpellPsychicScream = 13704,   // 心灵尖啸
    SpellShadowBoltVolley = 20741 // 暗影箭雨
};

constexpr char const* BalnazzarAggroText = "你们这些蠢货以为能这么轻易打败我？见识一下纳斯雷兹姆真正的力量吧！";
constexpr char const* BalnazzarDeathText = "该死的凡人！我所有的复仇计划，所有的仇恨……我会复仇的……";
constexpr uint32 BalnazzarAggroSound = 6447;
constexpr uint32 BalnazzarDeathSound = 6442;
}

struct boss_rift_balnazzar : public BossAIBase
{
    explicit boss_rift_balnazzar(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(BalnazzarAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(BalnazzarAggroSound);

        ScheduleTieredEvent(EventMindBlast, 5000, 4000, 3200);
        ScheduleTieredEvent(EventShadowShock, 8000, 6500, 5200);
        ScheduleTieredEvent(EventSleep, 12000, 9500, 7500);
        if (_tier >= 2)
            events.ScheduleEvent(EventPsychicScream, 16s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 20s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->Yell(BalnazzarDeathText, LANG_UNIVERSAL);
        me->PlayDirectSound(BalnazzarDeathSound);
        BossAIBase::JustDied(nullptr);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventMindBlast:
                CastIfConfigured(me->GetVictim(), SpellMindBlast);
                ScheduleTieredEvent(EventMindBlast, 6000, 4800, 3800);
                break;
            case EventShadowShock:
                CastIfConfigured(me->GetVictim(), SpellShadowShock);
                ScheduleTieredEvent(EventShadowShock, 9000, 7000, 5500);
                break;
            case EventSleep:
                CastIfConfigured(SelectRandomPlayer(), SpellSleep);
                ScheduleTieredEvent(EventSleep, 15000, 12000, 9500);
                break;
            case EventPsychicScream: // T2新增：心灵尖啸，读条不可打断
                CastIfConfigured(me, SpellPsychicScream, true);
                events.ScheduleEvent(EventPsychicScream, _tier == 3 ? 18s : 22s);
                break;
            case EventTier3Skill: // T3新增：暗影箭雨，读条不可打断
                CastIfConfigured(me, SpellShadowBoltVolley, true);
                events.ScheduleEvent(EventTier3Skill, 24s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_balnazzar()
{
    RegisterCreatureAI(boss_rift_balnazzar);
}

} // namespace HeroicDungeonRift
