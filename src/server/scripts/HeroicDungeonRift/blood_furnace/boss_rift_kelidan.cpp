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
// 鲜血熔炉 - 击碎者克里丹（Kelidan the Breaker）
enum Events : uint32
{
    EventShadowBoltVolley = 1, // 暗影箭雨（T1基础）
    EventCorruption,           // 腐蚀术（T1基础）
    EventBurningNova,          // 燃烧新星预警（T1基础）
    EventFireNova,             // 火焰新星（T1基础）
    EventVortex,               // 火焰漩涡（T1英雄基础）
    EventShadowfury            // 暗影之怒（T3新增）
};

enum Spells : uint32
{
    SpellCorruption = 30938,
    SpellFireNova = 33132,
    SpellShadowBoltVolley = 28599,
    SpellBurningNova = 30940,
    SpellVortex = 37370,
    SpellShadowfury = 39082
};

constexpr int32 ShadowfuryRaidDamage = 3500;

char const* const KillTexts[] =
{
    "罪有应得！",
    "你的朋友很快就会去找你了！"
};

constexpr char const* WakeText = "谁竟敢打断……什么？你们都干了些什么？你们毁掉了一切！";
constexpr char const* NovaText = "靠近点！再近点……燃烧吧！";
constexpr char const* DeathText = "祝你们……好运。你们会需要好运气的。";
}

struct boss_rift_kelidan : public BossAIBase
{
    explicit boss_rift_kelidan(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        ApplyImmunities(true);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        // 原版五名导魔者前置已移除，克里丹可被直接攻击。
        me->Yell(WakeText, LANG_UNIVERSAL);
        // T1 原版技能在所有 Tier 均保持原版首次施放窗口与 CD。
        events.ScheduleEvent(EventShadowBoltVolley, 1s);
        events.ScheduleEvent(EventCorruption, 5s);
        events.ScheduleEvent(EventBurningNova, 15s);
        if (_tier >= 3)
            events.ScheduleEvent(EventShadowfury, 18s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer() && urand(0, 1))
            me->Yell(KillTexts[urand(0, 1)], LANG_UNIVERSAL);
    }

    void JustDied(Unit* killer) override
    {
        me->Yell(DeathText, LANG_UNIVERSAL);
        BossAIBase::JustDied(killer);
    }

protected:
    void ConfigureTier() override
    {
        // TBC 暗影箭雨与火焰新星基础伤害已较高，3 倍可维持合理团队爆发。
        SetRaidSpellDamageMultiplier(3.0f);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventShadowBoltVolley:
                CastIfConfigured(me, SpellShadowBoltVolley);
                events.ScheduleEvent(EventShadowBoltVolley, Milliseconds(urand(8000, 13000)));
                break;
            case EventCorruption:
                CastIfConfigured(me, SpellCorruption);
                events.ScheduleEvent(EventCorruption, Milliseconds(urand(30000, 50000)));
                break;
            case EventBurningNova:
                me->Yell(NovaText, LANG_UNIVERSAL);
                CastIfConfigured(me, SpellBurningNova, true);
                events.DelayEvents(6s);
                // 英雄裂隙 T1 即保留原版英雄难度的火焰漩涡。
                events.ScheduleEvent(EventVortex, 1ms);
                events.ScheduleEvent(EventFireNova, 5s);
                events.ScheduleEvent(EventBurningNova, Milliseconds(urand(25000, 32000)));
                break;
            case EventFireNova:
                CastIfConfigured(me, SpellFireNova, true);
                break;
            case EventVortex: // T1基础：原版英雄难度的火焰漩涡
                CastIfConfigured(me, SpellVortex);
                break;
            case EventShadowfury: // T3新增：暗影之怒
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellShadowfury, SPELLVALUE_BASE_POINT0,
                    ShadowfuryRaidDamage);
                events.ScheduleEvent(EventShadowfury, 22s);
                break;
            default:
                break;
        }
    }

private:
    void ApplyImmunities(bool apply)
    {
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_CHARM, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DISTRACT, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FEAR, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_ROOT, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SILENCE, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SLEEP, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SNARE, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_FREEZE, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_KNOCKOUT, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_BANISH, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SHACKLE, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_TURN, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_HORROR, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_DAZE, apply);
        me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_SAPPED, apply);
    }
};

void AddSC_boss_rift_kelidan()
{
    RegisterCreatureAI(boss_rift_kelidan);
}

} // namespace HeroicDungeonRift
