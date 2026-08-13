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
// 黑石深渊 - 达格兰·索瑞森大帝（Emperor Dagran Thaurissan）+ 茉艾拉公主（Princess Moira，同伴）
enum BossEvents : uint32
{
    EventHandOfThaurissan = 1, // 索瑞森之手（T1基础）
    EventAvatarOfFlame,        // 烈焰化身（T1基础）
    EventTier2Skill,           // 火焰冲击（T2新增）
    EventTier3Skill            // 暗影箭（T3新增）
};

enum MoiraEvents : uint32
{
    EventMoiraHeal = 1, // 治疗术
    EventMoiraRenew,    // 恢复
    EventMoiraMindBlast // 心灵震爆
};

enum Spells : uint32
{
    SpellHandOfThaurissan = 17492, // 索瑞森之手
    SpellAvatarOfFlame = 15636,    // 烈焰化身
    SpellFireBlast = 13342,        // 火焰冲击
    SpellShadowBolt = 20791,       // 暗影箭
    SpellMoiraHeal = 15586,        // 治疗术
    SpellMoiraRenew = 8362,        // 恢复
    SpellMoiraMindBlast = 15587    // 心灵震爆
};

// 喊话（中文，对应原版 creature_text）
constexpr char const* ThaurissanCrushText = "我要把你们碾成碎片！";
constexpr char const* ThaurissanKillText = "向国王致敬，宝贝！";
constexpr uint32 ThaurissanCrushSound = 5457;
constexpr uint32 ThaurissanKillSound = 5431;
}

struct npc_rift_moira : public RiftSummonAI // 裂隙茉艾拉公主
{
    explicit npc_rift_moira(Creature* creature) : RiftSummonAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        _events.ScheduleEvent(EventMoiraHeal, Milliseconds(TierDelay(8000, 6500, 5000)));
        _events.ScheduleEvent(EventMoiraRenew, Milliseconds(TierDelay(6000, 5000, 4000)));
        _events.ScheduleEvent(EventMoiraMindBlast, Milliseconds(TierDelay(12000, 9500, 7500)));
    }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventMoiraHeal, Milliseconds(TierDelay(8000, 6500, 5000)));
        _events.ScheduleEvent(EventMoiraRenew, Milliseconds(TierDelay(6000, 5000, 4000)));
        _events.ScheduleEvent(EventMoiraMindBlast, Milliseconds(TierDelay(12000, 9500, 7500)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventMoiraHeal:
            {
                Unit* target = me;
                if (TempSummon* summon = me->ToTempSummon())
                    if (Unit* owner = summon->GetSummonerUnit())
                        if (owner->IsAlive() && owner->HealthBelowPct(90))
                            target = owner;
                DoCast(target, SpellMoiraHeal);
                _events.ScheduleEvent(EventMoiraHeal, Milliseconds(TierDelay(9000, 7000, 5500)));
                break;
            }
            case EventMoiraRenew:
                if (TempSummon* summon = me->ToTempSummon())
                    if (Unit* owner = summon->GetSummonerUnit())
                        if (owner->IsAlive())
                            DoCast(owner, SpellMoiraRenew);
                _events.ScheduleEvent(EventMoiraRenew, Milliseconds(TierDelay(7000, 5500, 4500)));
                break;
            case EventMoiraMindBlast:
                if (Unit* victim = me->GetVictim())
                    DoCast(victim, SpellMoiraMindBlast);
                _events.ScheduleEvent(EventMoiraMindBlast, Milliseconds(TierDelay(14000, 11000, 9000)));
                break;
            default:
                break;
        }
    }
};

struct boss_rift_thaurissan : public BossAIBase
{
    explicit boss_rift_thaurissan(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(ThaurissanCrushText, LANG_UNIVERSAL);
        me->PlayDirectSound(ThaurissanCrushSound);

        SummonTieredCreature(RiftEntryMoira, me->GetRandomNearPosition(4.0f), 0.7f, 0.7f);
        ScheduleTieredEvent(EventHandOfThaurissan, 5000, 4000, 3200);
        ScheduleTieredEvent(EventAvatarOfFlame, 11000, 9000, 7200);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 9s);  // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 14s); // T3新增
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
        {
            me->Yell(ThaurissanKillText, LANG_UNIVERSAL, victim);
            me->PlayDirectSound(ThaurissanKillSound, victim->ToPlayer());
        }
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventHandOfThaurissan:
                CastIfConfigured(SelectRandomPlayer(), SpellHandOfThaurissan);
                ScheduleTieredEvent(EventHandOfThaurissan, 6000, 4800, 3800);
                break;
            case EventAvatarOfFlame:
                CastIfConfigured(me, SpellAvatarOfFlame);
                ScheduleTieredEvent(EventAvatarOfFlame, 25000, 20000, 16000);
                break;
            case EventTier2Skill: // T2新增：火焰冲击，读条不可打断
                CastIfConfigured(me->GetVictim(), SpellFireBlast, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 5s : 7s);
                break;
            case EventTier3Skill: // T3新增：暗影箭，读条不可打断
                CastIfConfigured(me->GetVictim(), SpellShadowBolt, true);
                events.ScheduleEvent(EventTier3Skill, 8s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_thaurissan()
{
    RegisterCreatureAI(boss_rift_thaurissan);
    RegisterCreatureAI(npc_rift_moira);
}

} // namespace HeroicDungeonRift
