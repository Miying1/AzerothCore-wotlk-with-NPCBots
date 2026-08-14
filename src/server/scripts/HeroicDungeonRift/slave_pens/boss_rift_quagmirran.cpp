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
// 奴隶围栏 - 夸格米拉（Quagmirran）
enum QuagmirranEvents : uint32
{
    EventCleave = 1,         // 顺劈斩（T1基础）
    EventUppercut,           // 上钩拳（T1基础）
    EventAcidSpray,          // 酸液喷射（T1基础）
    EventPoisonBoltVolley,   // 毒箭之雨（T1基础）
    EventPoisonCloud,        // 毒云（T2新增）
    EventEnrage              // 激怒（T3新增）
};

enum Spells : uint32
{
    SpellAcidSpray = 38153,          // 酸液喷射
    SpellCleave = 40504,             // 顺劈斩
    SpellPoisonBoltVolley = 34780,   // 毒箭之雨
    SpellUppercut = 32055,           // 上钩拳
    SpellPoisonCloud = 49548,        // 3.3.5：达卡莱毒云；38152不在Spell_dbc中
    SpellEnrage = 34971              // 激怒
};

constexpr int32 PoisonCloudRaidDamagePerTick = 1800;
}

struct boss_rift_quagmirran : public BossAIBase
{
    explicit boss_rift_quagmirran(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        // T1 原版技能在所有 Tier 均保持原版首次施放窗口与 CD。
        events.ScheduleEvent(EventCleave, 9100ms);
        events.ScheduleEvent(EventUppercut, 20300ms);
        events.ScheduleEvent(EventAcidSpray, 25200ms);
        events.ScheduleEvent(EventPoisonBoltVolley, 31800ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventPoisonCloud, 12s);
        if (_tier >= 3)
            events.ScheduleEvent(EventEnrage, 18s);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventCleave:
                CastIfConfigured(me->GetVictim(), SpellCleave);
                events.ScheduleEvent(EventCleave, Milliseconds(urand(18800, 24800)));
                break;
            case EventUppercut:
                CastIfConfigured(me->GetVictim(), SpellUppercut);
                events.ScheduleEvent(EventUppercut, 21800ms);
                break;
            case EventAcidSpray:
                CastIfConfigured(SelectRandomPlayer(), SpellAcidSpray);
                events.ScheduleEvent(EventAcidSpray, 25s);
                break;
            case EventPoisonBoltVolley:
                CastIfConfigured(me, SpellPoisonBoltVolley);
                events.ScheduleEvent(EventPoisonBoltVolley, 24400ms);
                break;
            case EventPoisonCloud: // T2新增：随机目标毒云，强化站位压力
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellPoisonCloud, SPELLVALUE_BASE_POINT0,
                    PoisonCloudRaidDamagePerTick, true);
                events.ScheduleEvent(EventPoisonCloud, _tier == 3 ? 11s : 14s);
                break;
            case EventEnrage: // T3新增：周期激怒，强化近战威胁
                CastIfConfigured(me, SpellEnrage, true);
                events.ScheduleEvent(EventEnrage, 22s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_quagmirran()
{
    RegisterCreatureAI(boss_rift_quagmirran);
}

} // namespace HeroicDungeonRift
