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
    EventFireBlast = 1,         // 火焰冲击（T1基础）
    EventSummonBurningSpirits,  // 召唤燃烧之灵（T2新增）
    EventTier3Skill             // 烈焰风暴（T3新增）
};

enum Spells : uint32
{
    SpellFireBlast = 13342,    // 火焰冲击
    SpellFireNova = 11970,     // 火焰新星（燃烧之灵死亡爆炸）
    SpellFlamestrike = 12468   // 烈焰风暴
};

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

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(FlamelashAggroText, LANG_UNIVERSAL);

        ScheduleTieredEvent(EventFireBlast, 4000, 3200, 2500);
        events.ScheduleEvent(EventSummonBurningSpirits, _tier == 1 ? 10s : (_tier == 2 ? 8s : 6s));
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
                ScheduleTieredEvent(EventFireBlast, 7000, 5500, 4500);
                break;
            case EventSummonBurningSpirits:
                PruneBurningSpirits();
                for (uint32 i = _burningSpirits.size(); i < _tier + BurningSpiritSummonCount - 1; ++i)
                    if (Creature* spirit = SummonTieredCreature(RiftEntryBurningSpirit, me->GetRandomNearPosition(8.0f), 0.5f, 0.7f))
                        _burningSpirits.push_back(spirit->GetGUID());
                events.ScheduleEvent(EventSummonBurningSpirits, _tier == 1 ? 13s : (_tier == 2 ? 11s : 9s));
                break;
            case EventTier3Skill: // T3新增：烈焰风暴，点名随机目标，顺发
                CastIfConfigured(SelectRandomPlayer(), SpellFlamestrike, true);
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
