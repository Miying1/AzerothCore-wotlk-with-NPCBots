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
    EventShadowBolt = 1,  // 暗影箭（Spell 17393，T1原版）
    EventCleave,          // 顺劈斩（Spell 15284，T1原版）
    EventMortalStrike,    // 致死打击（Spell 15708，T1原版）
    EventRaiseDead,       // 亡者复生（T2新增，代码直接召唤2只骷髅，无独立施法ID）
    EventTier3Skill       // 死亡缠绕（Spell 6789，T3新增）
};

enum Spells : uint32
{
    SpellShadowBolt = 17393,    // 暗影箭（T1原版）
    SpellCleave = 15284,        // 顺劈斩（T1原版）
    SpellMortalStrike = 15708,  // 致死打击（T1原版）
    SpellUnholyAura = 17467,    // 邪恶光环（T1原版，开战自施）
    SpellDeathCoil = 6789       // 死亡缠绕（T3新增，覆写BP0直接伤害）
};

constexpr int32 DeathCoilTier1DirectDamage = 4500;

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

        events.ScheduleEvent(EventShadowBolt, Milliseconds(6000));
        events.ScheduleEvent(EventCleave, Milliseconds(8000));
        events.ScheduleEvent(EventMortalStrike, Milliseconds(10000));
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
                events.ScheduleEvent(EventShadowBolt, Milliseconds(5000));
                break;
            case EventCleave:
                CastIfConfigured(me->GetVictim(), SpellCleave);
                events.ScheduleEvent(EventCleave, Milliseconds(8000));
                break;
            case EventMortalStrike:
                CastIfConfigured(me->GetVictim(), SpellMortalStrike);
                events.ScheduleEvent(EventMortalStrike, Milliseconds(11000));
                break;
            case EventRaiseDead: // T2新增：代码直接召唤2只骷髅，无独立施法ID
                for (uint32 i = 0; i < SkeletonSummonCount; ++i)
                    SummonTieredCreature(RiftEntryMindlessSkeleton, me->GetRandomNearPosition(6.0f), 0.5f, 0.6f);
                events.ScheduleEvent(EventRaiseDead, _tier == 3 ? 16s : 20s);
                break;
            case EventTier3Skill: // T3新增：死亡缠绕，瞬发
                CastFinalRaidDamageSpell(me->GetVictim(), SpellDeathCoil, SPELLVALUE_BASE_POINT0,
                    DeathCoilTier1DirectDamage, true);
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
