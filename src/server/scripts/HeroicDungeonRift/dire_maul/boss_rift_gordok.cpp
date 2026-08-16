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
// 厄运之槌北区 - 戈多克大王（King Gordok）
// 观察者克鲁什（Cho'Rush the Observer）是原版同伴，不是Tier新增守卫；裂隙开战时随Boss召唤。
enum GordokEvents : uint32
{
    EventMortalStrike = 1,   // 致死打击（Spell 15708，T1原版）
    EventSunderArmor,        // 破甲攻击（Spell 15572，T1原版）
    EventWarStomp,           // 战争践踏（Spell 16727，T1原版）
    EventBerserkerCharge,    // 狂暴冲锋（Spell 22886，T2新增）
    EventTier3Skill          // 雷霆一击（Spell 15588，T3新增）
};

enum ChoRushEvents : uint32
{
    EventLightningBolt = 1,  // 闪电箭（Spell 15801，原版同伴）
    EventChainLightning,     // 闪电链（Spell 16006，原版同伴）
    EventMindBlast,          // 心灵震爆（Spell 15587，原版同伴）
    EventPsychicScream,      // 心灵尖啸（Spell 22884，原版同伴）
    EventHeal                // 治疗术（Spell 38209，原版同伴）
};

enum Spells : uint32
{
    SpellMortalStrike = 15708,    // 致死打击（T1原版）
    SpellSunderArmor = 15572,     // 破甲攻击（T1原版）
    SpellWarStomp = 16727,        // 战争践踏（T1原版）
    SpellBerserkerCharge = 22886, // 狂暴冲锋（T2新增）
    SpellThunderclap = 15588,     // 雷霆一击（T3新增，覆写BP0直接伤害）
    SpellLightningBolt = 15801,   // 闪电箭（原版同伴Cho'Rush）
    SpellChainLightning = 16006,  // 闪电链（原版同伴Cho'Rush）
    SpellMindBlast = 15587,       // 心灵震爆（原版同伴Cho'Rush）
    SpellPsychicScream = 22884,   // 心灵尖啸（原版同伴Cho'Rush）
    SpellHeal = 38209             // 治疗术（原版同伴Cho'Rush）
};

constexpr int32 ThunderclapTier1DirectDamage = 3500;

constexpr char const* GordokAggroText = "没人能挑战戈多克！";
}

struct npc_rift_chorsh : public RiftSummonAI // 原版同伴观察者克鲁什
{
    explicit npc_rift_chorsh(Creature* creature) : RiftSummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventLightningBolt, Milliseconds(2500));
        _events.ScheduleEvent(EventChainLightning, Milliseconds(10000));
        _events.ScheduleEvent(EventMindBlast, Milliseconds(7000));
        _events.ScheduleEvent(EventPsychicScream, Milliseconds(16000));
        _events.ScheduleEvent(EventHeal, Milliseconds(9000));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventLightningBolt:
                DoCastVictim(SpellLightningBolt);
                _events.ScheduleEvent(EventLightningBolt, Milliseconds(3000));
                break;
            case EventChainLightning:
                DoCastVictim(SpellChainLightning);
                _events.ScheduleEvent(EventChainLightning, Milliseconds(12000));
                break;
            case EventMindBlast:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SpellMindBlast);
                _events.ScheduleEvent(EventMindBlast, Milliseconds(9000));
                break;
            case EventPsychicScream:
                DoCast(me, SpellPsychicScream);
                _events.ScheduleEvent(EventPsychicScream, Milliseconds(18000));
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
                _events.ScheduleEvent(EventHeal, Milliseconds(10000));
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

        events.ScheduleEvent(EventMortalStrike, Milliseconds(6000));
        events.ScheduleEvent(EventSunderArmor, Milliseconds(4000));
        events.ScheduleEvent(EventWarStomp, Milliseconds(15000));
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
                events.ScheduleEvent(EventMortalStrike, Milliseconds(12000));
                break;
            case EventSunderArmor:
                CastIfConfigured(me->GetVictim(), SpellSunderArmor);
                events.ScheduleEvent(EventSunderArmor, Milliseconds(8000));
                break;
            case EventWarStomp:
                CastIfConfigured(me, SpellWarStomp);
                events.ScheduleEvent(EventWarStomp, Milliseconds(19000));
                break;
            case EventBerserkerCharge: // T2新增：冲锋，选随机目标
                CastIfConfigured(SelectRandomPlayer(), SpellBerserkerCharge, true);
                events.ScheduleEvent(EventBerserkerCharge, _tier == 3 ? 12s : 15s);
                break;
            case EventTier3Skill: // T3新增：雷霆一击，瞬发
                CastFinalRaidDamageSpell(me, SpellThunderclap, SPELLVALUE_BASE_POINT0,
                    ThunderclapTier1DirectDamage, true);
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
