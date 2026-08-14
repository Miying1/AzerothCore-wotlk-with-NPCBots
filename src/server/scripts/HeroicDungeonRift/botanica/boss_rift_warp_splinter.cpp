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
// 生态船 - 迁跃扭木（Warp Splinter）
enum Events : uint32
{
    EventArcaneVolley = 1, // 奥术箭雨（T1基础）
    EventWarStomp,        // 战争践踏（T1基础）
    EventSummonSaplings,  // 召唤树苗（T1基础）
    EventTranquility,     // 宁静（T2新增）
    EventPlantSeedlings   // 植物幼苗（T3新增）
};

enum Spells : uint32
{
    SpellWarStomp = 34716,
    SpellArcaneVolley = 36705,
    SpellTranquility = 34550,
    SpellPlantSeedlings = 34761,
    SpellSaplingHeal = 34742
};

enum Entries : uint32
{
    RiftEntrySapling = 102049 // source 19949
};

constexpr char const* WarpSplinterAggroText = "谁敢闯入这座圣殿？";
constexpr char const* WarpSplinterSlayText = "你去死吧！等等，这不……不，不……你去死吧！";
constexpr char const* WarpSplinterSummonText = "孩子们，来帮助我！";
constexpr char const* WarpSplinterDeathText = "非常……困惑。我不属于……这里。";
constexpr uint32 WarpSplinterAggroSound = 11230;
constexpr uint32 WarpSplinterSlaySound = 11231;
constexpr uint32 WarpSplinterSummonSound = 11233;
constexpr uint32 WarpSplinterDeathSound = 11235;
}

struct npc_rift_warp_sapling : public RiftLevel70SummonAI
{
    explicit npc_rift_warp_sapling(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(1, 8s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != 1)
            return;

        if (TempSummon* summon = me->ToTempSummon())
            if (Unit* owner = summon->GetSummonerUnit())
                if (owner->IsAlive())
                    DoCast(owner, SpellSaplingHeal);
        _events.ScheduleEvent(1, 10s);
    }
};

struct boss_rift_warp_splinter : public BossAIBase
{
    explicit boss_rift_warp_splinter(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(WarpSplinterAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(WarpSplinterAggroSound);

        events.ScheduleEvent(EventArcaneVolley, 8s);
        events.ScheduleEvent(EventWarStomp, 15s);
        events.ScheduleEvent(EventSummonSaplings, 20s);
        if (_tier >= 2)
            events.ScheduleEvent(EventTranquility, 27s);
        if (_tier >= 3)
            events.ScheduleEvent(EventPlantSeedlings, 13s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
        {
            me->Yell(WarpSplinterSlayText, LANG_UNIVERSAL, victim);
            me->PlayDirectSound(WarpSplinterSlaySound, victim->ToPlayer());
        }
    }

    void JustDied(Unit* killer) override
    {
        BossAIBase::JustDied(killer);
        me->Yell(WarpSplinterDeathText, LANG_UNIVERSAL);
        me->PlayDirectSound(WarpSplinterDeathSound);
    }

    void ConfigureTier() override
    {
        SetRaidSpellDamageMultiplier(2.0f);
        AddInterruptImmuneSpell(SpellTranquility);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventArcaneVolley:
                CastIfConfigured(me, SpellArcaneVolley);
                events.ScheduleEvent(EventArcaneVolley, 20s);
                break;
            case EventWarStomp:
                CastIfConfigured(me, SpellWarStomp);
                events.ScheduleEvent(EventWarStomp, 30s);
                break;
            case EventSummonSaplings:
            {
                me->Yell(WarpSplinterSummonText, LANG_UNIVERSAL);
                me->PlayDirectSound(WarpSplinterSummonSound);
                uint32 summonCount = 5 + _tier;
                for (uint32 i = 0; i < summonCount; ++i)
                    SummonTieredCreature(RiftEntrySapling, me->GetRandomNearPosition(8.0f), 0.32f, 0.5f);
                events.ScheduleEvent(EventSummonSaplings, 40s);
                break;
            }
            case EventTranquility: // T2新增：生态船宁静
                CastIfConfigured(me, SpellTranquility);
                events.ScheduleEvent(EventTranquility, _tier == 3 ? 30s : 38s);
                break;
            case EventPlantSeedlings: // T3新增：生态船植物幼苗
                CastIfConfigured(SelectRandomPlayer(), SpellPlantSeedlings, true);
                events.ScheduleEvent(EventPlantSeedlings, 18s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_warp_splinter()
{
    RegisterCreatureAI(boss_rift_warp_splinter);
    RegisterCreatureAI(npc_rift_warp_sapling);
}

} // namespace HeroicDungeonRift
