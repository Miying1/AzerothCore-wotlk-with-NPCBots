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
// 黑石深渊 - 弗莱拉斯大使（Ambassador Flamelash）
enum BossEvents : uint32
{
    EventFireBlast = 1,         // 火焰冲击（原版/T1基础）
    EventSummonBurningSpirits,  // 召唤燃烧之灵（原版/T1基础；并存上限T1/T2/T3为4/5/6只）
    EventTier3Skill             // 烈焰风暴（T3新增）
};

enum Spells : uint32
{
    SpellFireBlast = 13342,    // 火焰冲击（原版/T1基础）
    SpellFireNova = 11970,     // 火焰新星（裂隙版/T1改造：燃烧之灵死亡时爆炸，非原版行为）
    SpellFlamestrike = 12468   // 烈焰风暴（T3新增，混合BP0直伤/BP1周期）
};

constexpr int32 FlamestrikeTier1DirectDamage = 4500;
constexpr int32 FlamestrikeTier1DamagePerTick = 1800;
constexpr uint32 BurningSpiritSummonCount = 4;
constexpr char const* FlamelashAggroText = "你们的恐怖统治结束了！面对你们的末日吧，凡人！";
}

struct npc_rift_burning_spirit : public RiftSummonAI // 裂隙燃烧之灵
{
    explicit npc_rift_burning_spirit(Creature* creature) : RiftSummonAI(creature) { }

    void JustDied(Unit* /*killer*/) override
    {
        DoCast(me, SpellFireNova, true);
        me->DespawnOrUnsummon(1s);
    }
};

struct boss_rift_flamelash : public BossAIBase
{
    explicit boss_rift_flamelash(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _burningSpirits.clear();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(FlamelashAggroText, LANG_UNIVERSAL);

        events.ScheduleEvent(EventFireBlast, 4000ms);
        events.ScheduleEvent(EventSummonBurningSpirits, 10s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 16s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventFireBlast:
                CastIfConfigured(me->GetVictim(), SpellFireBlast);
                events.ScheduleEvent(EventFireBlast, 7000ms);
                break;
            case EventSummonBurningSpirits: // 原版/T1基础；全Tier保持原版频率，仅提高并存数量
                PruneBurningSpirits();
                for (uint32 i = _burningSpirits.size(); i < _tier + BurningSpiritSummonCount - 1; ++i)
                    if (Creature* spirit = SummonTieredCreature(RiftEntryBurningSpirit, me->GetRandomNearPosition(8.0f), 0.5f, 0.7f))
                        _burningSpirits.push_back(spirit->GetGUID());
                events.ScheduleEvent(EventSummonBurningSpirits, 13s);
                break;
            case EventTier3Skill: // T3新增：烈焰风暴，点名随机目标；混合BP0直伤/BP1周期，瞬发
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellFlamestrike,
                    FlamestrikeTier1DirectDamage, FlamestrikeTier1DamagePerTick, true);
                events.ScheduleEvent(EventTier3Skill, 20s);
                break;
            default:
                break;
        }
    }

private:
    void PruneBurningSpirits()
    {
        _burningSpirits.erase(std::remove_if(_burningSpirits.begin(), _burningSpirits.end(), [this](ObjectGuid const& guid)
        {
            Creature* spirit = ObjectAccessor::GetCreature(*me, guid);
            return !spirit || !spirit->IsAlive();
        }), _burningSpirits.end());
    }

    std::vector<ObjectGuid> _burningSpirits;
};

void AddSC_boss_rift_flamelash()
{
    RegisterCreatureAI(boss_rift_flamelash);
    RegisterCreatureAI(npc_rift_burning_spirit);
}

} // namespace HeroicDungeonRift
