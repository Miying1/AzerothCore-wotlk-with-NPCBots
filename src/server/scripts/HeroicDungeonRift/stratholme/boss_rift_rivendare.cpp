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
// 斯坦索姆 - 瑞文戴尔男爵（Baron Rivendare）
enum Events : uint32
{
    EventShadowBolt = 1,  // 暗影箭（T1基础）
    EventCleave,          // 顺劈斩（T1基础）
    EventMortalStrike,    // 致死打击（T1基础）
    EventRaiseDead,       // 亡者复生召唤骷髅（T2新增）
    EventTier3Skill       // 死亡缠绕（T3新增）
};

enum Spells : uint32
{
    SpellShadowBolt = 17393,    // 暗影箭
    SpellCleave = 15284,        // 顺劈斩
    SpellMortalStrike = 15708,  // 致死打击
    SpellUnholyAura = 17467,    // 邪恶光环
    SpellDeathCoil = 6789       // 死亡缠绕
};

// 原版另有45分钟限时、释放囚徒与拉姆斯登Slaughter事件，裂隙单Boss房未启用。
constexpr uint32 SkeletonSummonCount = 2;
constexpr char const* RivendareAggroText = "入侵者！又是银色黎明的走狗……";
constexpr uint32 RivendareAggroSound = 11812;
}

struct boss_rift_rivendare : public BossAIBase
{
    explicit boss_rift_rivendare(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(RivendareAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(RivendareAggroSound);
        CastIfConfigured(me, SpellUnholyAura, true);

        ScheduleTieredEvent(EventShadowBolt, 6000, 4800, 3800);
        ScheduleTieredEvent(EventCleave, 8000, 6500, 5200);
        ScheduleTieredEvent(EventMortalStrike, 10000, 8000, 6500);
        if (_tier >= 2)
            events.ScheduleEvent(EventRaiseDead, 14s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 12s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventShadowBolt:
                CastIfConfigured(me->GetVictim(), SpellShadowBolt);
                ScheduleTieredEvent(EventShadowBolt, 5000, 4000, 3200);
                break;
            case EventCleave:
                CastIfConfigured(me->GetVictim(), SpellCleave);
                ScheduleTieredEvent(EventCleave, 8000, 6500, 5200);
                break;
            case EventMortalStrike:
                CastIfConfigured(me->GetVictim(), SpellMortalStrike);
                ScheduleTieredEvent(EventMortalStrike, 11000, 9000, 7200);
                break;
            case EventRaiseDead:
                for (uint32 i = 0; i < SkeletonSummonCount; ++i)
                    SummonTieredCreature(RiftEntryMindlessSkeleton, me->GetRandomNearPosition(6.0f), 0.5f, 0.6f);
                events.ScheduleEvent(EventRaiseDead, _tier == 3 ? 16s : 20s);
                break;
            case EventTier3Skill: // T3新增：死亡缠绕，瞬发
                CastIfConfigured(me->GetVictim(), SpellDeathCoil, true);
                events.ScheduleEvent(EventTier3Skill, 15s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_rivendare()
{
    RegisterCreatureAI(boss_rift_rivendare);
}

} // namespace HeroicDungeonRift
