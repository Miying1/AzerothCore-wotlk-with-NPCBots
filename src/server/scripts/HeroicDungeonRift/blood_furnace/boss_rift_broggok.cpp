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
// 鲜血熔炉 - 布洛戈克（Broggok）
enum Events : uint32
{
    EventSlimeSpray = 1,     // 粘液喷射（T1基础）
    EventPoisonBolt,         // 毒箭（T1基础）
    EventPoisonCloud,        // 毒云（T1基础）
    EventAcidBreath,         // 酸息术（T2新增）
    EventPoisonBoltVolley    // 毒箭之雨（T3新增）
};

enum Spells : uint32
{
    SpellSlimeSpray = 30913,
    SpellPoisonCloud = 30916,
    SpellPoisonBolt = 30917,
    SpellPoison = 30914,
    SpellAcidBreath = 34268,
    SpellPoisonBoltVolley = 28796
};

enum RiftEntries : uint32
{
    RiftEntryBroggokPoisonCloud = 102038
};

constexpr int32 AcidBreathTier1DirectDamage = 3000;
constexpr int32 AcidBreathTier1DamagePerTick = 1500;
constexpr int32 PoisonBoltVolleyRaidDirectDamage = 3375;
constexpr int32 PoisonBoltVolleyRaidDamagePerTick = 1720;

constexpr char const* AggroText = "入侵者来了……";
}

struct npc_rift_broggok_poison_cloud : public RiftLevel70SummonAI
{
    explicit npc_rift_broggok_poison_cloud(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void Reset() override
    {
        RiftLevel70SummonAI::Reset();
        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        DoCast(me, SpellPoison, true);
        me->DespawnOrUnsummon(20s);
    }
};

struct boss_rift_broggok : public BossAIBase
{
    explicit boss_rift_broggok(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        // 原版牢门/拉杆前置已移除，进入战斗立即启用完整技能组。
        me->Yell(AggroText, LANG_UNIVERSAL);
        // T1 原版技能在所有 Tier 均保持原版首次施放窗口与 CD。
        events.ScheduleEvent(EventSlimeSpray, 10s);
        events.ScheduleEvent(EventPoisonBolt, 5s);
        events.ScheduleEvent(EventPoisonCloud, 7s);
        if (_tier >= 2)
            events.ScheduleEvent(EventAcidBreath, 12s);
        if (_tier >= 3)
            events.ScheduleEvent(EventPoisonBoltVolley, 16s);
    }

protected:
    void ConfigureTier() override
    {
        // TBC 法术基础伤害约数百点，4 倍后适合作为 83 级团队持续毒伤。
        SetRaidSpellDamageMultiplier(4.0f);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventSlimeSpray:
                CastIfConfigured(me->GetVictim(), SpellSlimeSpray);
                events.ScheduleEvent(EventSlimeSpray, Milliseconds(urand(7000, 12000)));
                break;
            case EventPoisonBolt:
                CastIfConfigured(SelectRandomPlayer(), SpellPoisonBolt);
                events.ScheduleEvent(EventPoisonBolt, Milliseconds(urand(6000, 11000)));
                break;
            case EventPoisonCloud:
                // 原法术召唤的 17662 改为裂隙专用 Entry，并经统一召唤缩放。
                SummonTieredCreature(RiftEntryBroggokPoisonCloud, me->GetPosition(), 0.1f, 1.0f,
                    TEMPSUMMON_TIMED_DESPAWN, 20 * IN_MILLISECONDS);
                events.ScheduleEvent(EventPoisonCloud, 20s);
                break;
            case EventAcidBreath: // T2新增：正面酸息术；T1基准伤害由公共逻辑应用Tier倍率
                CastFinalRaidDamageSpell(me->GetVictim(), SpellAcidBreath,
                    AcidBreathTier1DamagePerTick, AcidBreathTier1DirectDamage);
                events.ScheduleEvent(EventAcidBreath, _tier == 2 ? 17s : 14s);
                break;
            case EventPoisonBoltVolley: // T3新增：毒箭之雨
                CastFinalRaidDamageSpell(me, SpellPoisonBoltVolley,
                    PoisonBoltVolleyRaidDirectDamage, PoisonBoltVolleyRaidDamagePerTick);
                events.ScheduleEvent(EventPoisonBoltVolley, 20s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_broggok()
{
    RegisterCreatureAI(boss_rift_broggok);
    RegisterCreatureAI(npc_rift_broggok_poison_cloud);
}

} // namespace HeroicDungeonRift
