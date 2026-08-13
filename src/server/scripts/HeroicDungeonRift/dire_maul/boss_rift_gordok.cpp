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
// 厄运之槌北区 - 戈多克大王（King Gordok）+ 观察者克鲁什（Cho'Rush the Observer，双Boss同伴）
enum GordokEvents : uint32
{
    EventMortalStrike = 1,   // 致死打击（T1基础）
    EventSunderArmor,        // 破甲攻击（T1基础）
    EventWarStomp,           // 战争践踏（T1基础）
    EventBerserkerCharge,    // 狂暴冲锋（T2新增）
    EventTier3Skill          // 雷霆一击（T3新增）
};

enum ChoRushEvents : uint32
{
    EventLightningBolt = 1,  // 闪电箭
    EventChainLightning,     // 闪电链
    EventMindBlast,          // 心灵震爆
    EventPsychicScream,      // 心灵尖啸
    EventHeal                // 治疗术
};

enum Spells : uint32
{
    SpellMortalStrike = 15708,    // 致死打击
    SpellSunderArmor = 15572,     // 破甲攻击
    SpellWarStomp = 16727,        // 战争践踏
    SpellBerserkerCharge = 22886, // 狂暴冲锋
    SpellThunderclap = 15588,     // 雷霆一击
    SpellLightningBolt = 15801,   // 闪电箭
    SpellChainLightning = 16006,  // 闪电链
    SpellMindBlast = 15587,       // 心灵震爆
    SpellPsychicScream = 22884,   // 心灵尖啸
    SpellHeal = 38209             // 治疗术
};

constexpr char const* GordokAggroText = "没人能挑战戈多克！";
}

struct npc_rift_chorsh : public RiftSummonAI // 裂隙观察者克鲁什
{
    explicit npc_rift_chorsh(Creature* creature) : RiftSummonAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EventLightningBolt, Milliseconds(TierDelay(2500, 2000, 1600)));
        _events.ScheduleEvent(EventChainLightning, Milliseconds(TierDelay(10000, 8000, 6500)));
        _events.ScheduleEvent(EventMindBlast, Milliseconds(TierDelay(7000, 5500, 4500)));
        _events.ScheduleEvent(EventPsychicScream, Milliseconds(TierDelay(16000, 13000, 10500)));
        _events.ScheduleEvent(EventHeal, Milliseconds(TierDelay(9000, 7500, 6000)));
    }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventLightningBolt, Milliseconds(TierDelay(2500, 2000, 1600)));
        _events.ScheduleEvent(EventChainLightning, Milliseconds(TierDelay(10000, 8000, 6500)));
        _events.ScheduleEvent(EventMindBlast, Milliseconds(TierDelay(7000, 5500, 4500)));
        _events.ScheduleEvent(EventPsychicScream, Milliseconds(TierDelay(16000, 13000, 10500)));
        _events.ScheduleEvent(EventHeal, Milliseconds(TierDelay(9000, 7500, 6000)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventLightningBolt:
                DoCastVictim(SpellLightningBolt);
                _events.ScheduleEvent(EventLightningBolt, Milliseconds(TierDelay(3000, 2400, 1900)));
                break;
            case EventChainLightning:
                DoCastVictim(SpellChainLightning);
                _events.ScheduleEvent(EventChainLightning, Milliseconds(TierDelay(12000, 9500, 7500)));
                break;
            case EventMindBlast:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SpellMindBlast);
                _events.ScheduleEvent(EventMindBlast, Milliseconds(TierDelay(9000, 7000, 5500)));
                break;
            case EventPsychicScream:
                DoCast(me, SpellPsychicScream);
                _events.ScheduleEvent(EventPsychicScream, Milliseconds(TierDelay(18000, 14500, 11500)));
                break;
            case EventHeal:
                if (Unit* target = me)
                {
                    if (TempSummon* summon = me->ToTempSummon())
                        if (Unit* owner = summon->GetSummonerUnit())
                            if (owner->IsAlive() && owner->HealthBelowPct(80))
                                target = owner;
                    if (target->HealthBelowPct(80))
                        DoCast(target, SpellHeal);
                }
                _events.ScheduleEvent(EventHeal, Milliseconds(TierDelay(10000, 8000, 6500)));
                break;
            default:
                break;
        }
    }
};

struct boss_rift_gordok : public BossAIBase
{
    explicit boss_rift_gordok(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(GordokAggroText, LANG_UNIVERSAL);
        SummonTieredCreature(RiftEntryChoRush, me->GetRandomNearPosition(4.0f), 0.8f, 0.8f);

        ScheduleTieredEvent(EventMortalStrike, 6000, 4800, 3800);
        ScheduleTieredEvent(EventSunderArmor, 4000, 3200, 2500);
        ScheduleTieredEvent(EventWarStomp, 15000, 12000, 9500);
        if (_tier >= 2)
            events.ScheduleEvent(EventBerserkerCharge, 8s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 12s);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventMortalStrike:
                CastIfConfigured(me->GetVictim(), SpellMortalStrike);
                ScheduleTieredEvent(EventMortalStrike, 12000, 9500, 7500);
                break;
            case EventSunderArmor:
                CastIfConfigured(me->GetVictim(), SpellSunderArmor);
                ScheduleTieredEvent(EventSunderArmor, 8000, 6500, 5200);
                break;
            case EventWarStomp:
                CastIfConfigured(me, SpellWarStomp);
                ScheduleTieredEvent(EventWarStomp, 19000, 15500, 12500);
                break;
            case EventBerserkerCharge: // T2新增：冲锋，选随机目标
                CastIfConfigured(SelectRandomPlayer(), SpellBerserkerCharge, true);
                events.ScheduleEvent(EventBerserkerCharge, _tier == 3 ? 12s : 15s);
                break;
            case EventTier3Skill: // T3新增：雷霆一击，瞬发
                CastIfConfigured(me, SpellThunderclap, true);
                events.ScheduleEvent(EventTier3Skill, 14s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_gordok()
{
    RegisterCreatureAI(boss_rift_gordok);
    RegisterCreatureAI(npc_rift_chorsh);
}

} // namespace HeroicDungeonRift
