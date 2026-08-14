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
// 原版先以达索汉形态作战，40%生命值进入巴纳扎尔阶段；裂隙脚本直接使用巴纳扎尔形态。
enum Events : uint32
{
    EventMindBlast = 1,    // 心灵震爆（Spell 17287，T1原版）
    EventShadowShock,      // 暗影震击（Spell 17399，T1原版）
    EventSleep,            // 催眠术（Spell 12098，T1原版）
    EventPsychicScream,    // 心灵尖啸（Spell 13704，T2新增）
    EventTier3Skill        // 暗影箭雨（Spell 20741，T3新增）
};

enum Spells : uint32
{
    SpellMindBlast = 17287,       // 心灵震爆（T1原版）
    SpellShadowShock = 17399,     // 暗影震击（T1原版）
    SpellSleep = 12098,           // 催眠术（T1原版）
    SpellPsychicScream = 13704,   // 心灵尖啸（T2新增）
    SpellShadowBoltVolley = 20741 // 暗影箭雨（T3新增，覆写BP0直接伤害）
};

constexpr int32 ShadowBoltVolleyTier1DirectDamage = 4500;

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

        events.ScheduleEvent(EventMindBlast, Milliseconds(5000));
        events.ScheduleEvent(EventShadowShock, Milliseconds(8000));
        events.ScheduleEvent(EventSleep, Milliseconds(12000));
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
                events.ScheduleEvent(EventMindBlast, Milliseconds(6000));
                break;
            case EventShadowShock:
                CastIfConfigured(me->GetVictim(), SpellShadowShock);
                events.ScheduleEvent(EventShadowShock, Milliseconds(9000));
                break;
            case EventSleep:
                CastIfConfigured(SelectRandomPlayer(), SpellSleep);
                events.ScheduleEvent(EventSleep, Milliseconds(15000));
                break;
            case EventPsychicScream: // T2新增：心灵尖啸，瞬发
                CastIfConfigured(me, SpellPsychicScream, true);
                events.ScheduleEvent(EventPsychicScream, _tier == 3 ? 18s : 22s);
                break;
            case EventTier3Skill: // T3新增：暗影箭雨，瞬发
                CastFinalRaidDamageSpell(me, SpellShadowBoltVolley, SPELLVALUE_BASE_POINT0,
                    ShadowBoltVolleyTier1DirectDamage, true);
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
