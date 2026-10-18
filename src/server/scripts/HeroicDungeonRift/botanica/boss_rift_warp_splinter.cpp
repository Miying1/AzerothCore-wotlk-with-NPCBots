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
    EventThunderclap,     // 雷霆一击（T1基础）
    EventSummonSaplings   // 召唤树苗（T1基础，T2起召唤后立刻引导宁静，T3树苗额外获得魔爆术）
};

enum Spells : uint32
{
    SpellThunderclap = 36706,
    SpellArcaneVolley = 36705,
    SpellTranquility = 34550,
    SpellSaplingHeal = 34742,
    SpellArcaneExplosion = 54211 // T3：树苗的魔爆术（瞬发，以自身为中心半径8码）
};

// 树苗自身的事件（与Boss的Events互不影响）。
enum SaplingEvents : uint32
{
    SaplingEventHeal = 1,
    SaplingEventArcaneExplosion
};

enum Entries : uint32
{
    RiftEntrySapling = 102049 // source 19949
};

// 宁静(34550)是2秒一跳的范围治疗光环，DBC原值仅924~1075/跳，对82级Boss血量几乎无感；
// 这里把每跳治疗量提升到约2倍DBC上限的固定基线。
constexpr int32 TranquilityHealPerTick = 3000;

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
        _events.ScheduleEvent(SaplingEventHeal, 8s);

        // T3新增：树苗额外获得魔爆术，冷却4~8秒随机。
        if (_tier >= 3)
            _events.ScheduleEvent(SaplingEventArcaneExplosion, Milliseconds(urand(4000, 8000)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case SaplingEventHeal:
                if (TempSummon* summon = me->ToTempSummon())
                    if (Unit* owner = summon->GetSummonerUnit())
                        if (owner->IsAlive())
                            DoCast(owner, SpellSaplingHeal);
                _events.ScheduleEvent(SaplingEventHeal, 10s);
                break;
            case SaplingEventArcaneExplosion:
                DoCast(me, SpellArcaneExplosion);
                _events.ScheduleEvent(SaplingEventArcaneExplosion, Milliseconds(urand(4000, 8000)));
                break;
            default:
                break;
        }
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
        events.ScheduleEvent(EventThunderclap, 15s);
        events.ScheduleEvent(EventSummonSaplings, 20s);
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

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (!summon || summon->GetEntry() != RiftEntrySapling)
            return;

        if (HasLivingSaplings())
            return;

        // 最后一棵树苗阵亡：宁静正在引导时由Boss自己打断读条。
        // 宁静是自引导法术（SPELL_ATTR1_IS_SELF_CHANNELED），InterruptSpell 走 cancel(false)，
        // 会连带移除该次引导产生的治疗光环。
        SpellInfo const* interrupted = nullptr;
        for (CurrentSpellTypes spellType : { CURRENT_GENERIC_SPELL, CURRENT_CHANNELED_SPELL })
            if (Spell* spell = me->GetCurrentSpell(spellType))
                if (spell->GetSpellInfo()->Id == SpellTranquility)
                {
                    interrupted = spell->GetSpellInfo();
                    me->InterruptSpell(spellType);
                }

        // 主动打断不会走 OnSpellCast/OnSpellFailed，这里补一次解除打断免疫，避免免疫状态残留。
        RemoveInterruptImmunity(interrupted);
    }

    void ConfigureTier() override
    {
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
            case EventThunderclap:
                CastIfConfigured(me, SpellThunderclap);
                events.ScheduleEvent(EventThunderclap, 30s);
                break;
            case EventSummonSaplings:
            {
                me->Yell(WarpSplinterSummonText, LANG_UNIVERSAL);
                me->PlayDirectSound(WarpSplinterSummonSound);
                // 树苗血量系数由 0.32 提高到 0.64（血量翻倍），伤害系数保持 0.5 不变。
                uint32 summonCount = 5 + _tier;
                for (uint32 i = 0; i < summonCount; ++i)
                    SummonTieredCreature(RiftEntrySapling, me->GetRandomNearPosition(8.0f), 0.64f, 0.5f);

                // T2起：召唤完成后立刻引导宁静（ConfigureTier 已注册打断免疫，玩家无法打断）；
                // 小怪全部阵亡时由 SummonedCreatureDies 打断这次引导。
                // 每跳治疗量用自定义基线覆盖DBC原值。
                if (_tier >= 2 && HasLivingSaplings())
                    CastRaidTunedSpell(me, SpellTranquility, TranquilityHealPerTick);

                // 召唤节奏：T3 40秒一波，T2 50秒一波，T1 保持40秒。
                events.ScheduleEvent(EventSummonSaplings, _tier == 2 ? 50s : 40s);
                break;
            }
            default:
                break;
        }
    }

private:
    // 场上是否还有存活的树苗（召唤物列表由 BossAIBase::SummonTieredCreature 维护）。
    bool HasLivingSaplings() const
    {
        for (ObjectGuid const& guid : _riftSummons)
            if (Creature* sapling = ObjectAccessor::GetCreature(*me, guid))
                if (sapling->IsAlive() && sapling->GetEntry() == RiftEntrySapling)
                    return true;

        return false;
    }
};

void AddSC_boss_rift_warp_splinter()
{
    RegisterCreatureAI(boss_rift_warp_splinter);
    RegisterCreatureAI(npc_rift_warp_sapling);
}

} // namespace HeroicDungeonRift
