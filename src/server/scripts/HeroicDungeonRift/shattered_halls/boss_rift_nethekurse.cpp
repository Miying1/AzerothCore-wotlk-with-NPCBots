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
// 破碎大厅 - 高阶术士奈瑟库斯（Grand Warlock Nethekurse）
enum Events : uint32
{
    EventDeathCoil = 1,       // 死亡缠绕（T1基础）
    EventShadowFissure,       // 暗影裂隙（T1基础）
    EventShadowCleave,        // 暗影顺劈（T1基础）
    EventDarkSpin,            // 黑暗旋转阶段开始（T1，25%生命阶段）
    EventDarkSpinTick,        // 黑暗旋转周期打击（每1秒）
    EventShadowBoltVolley,    // 暗影箭雨（T2新增）
    EventCorruption           // 腐蚀术（T3新增）
};

enum Spells : uint32
{
    SpellDeathCoil = 30500,
    SpellShadowFissure = 30496,
    SpellShadowCleave = 30495,
    SpellShadowBolt = 30505,           // 黑暗旋转触发的暗影箭（spell_tsh_shadow_bolt）
    SpellDarkSpinPulse = 30508,        // 黑暗旋转的旋风武器打击
    SpellShadowBoltVolley = 28599,
    SpellCorruption = 30938
};

char const* const AggroTexts[] =
{
    "你想要我们一起上？很好！",
    "谢谢你们帮我省去不少麻烦。现在，该我来找找乐子了！"
};

char const* const SlayTexts[] =
{
    "我已经有些厌倦了！",
    "来吧，让我看看你们真正的实力！",
    "跟苦工打交道都比这有趣！",
    "你败了。",
    "啊，死吧！"
};

constexpr int32 ShadowBoltVolleyRaidDamage = 4500;
constexpr int32 CorruptionRaidDamagePerTick = 2000;

constexpr char const* DeathText = "真……丢人。";
}

struct boss_rift_nethekurse : public BossAIBase
{
    explicit boss_rift_nethekurse(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _darkSpinStarted = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(AggroTexts[urand(0, 1)], LANG_UNIVERSAL);
        // T1 原版技能在所有 Tier 均保持原版首次施放窗口与 CD。
        events.ScheduleEvent(EventDeathCoil, Milliseconds(urand(12150, 19850)));
        events.ScheduleEvent(EventShadowFissure, Milliseconds(urand(8100, 17300)));
        events.ScheduleEvent(EventShadowCleave, Milliseconds(urand(10950, 21850)));

        if (_tier >= 2)
            events.ScheduleEvent(EventShadowBoltVolley, 16s);
        if (_tier >= 3)
            events.ScheduleEvent(EventCorruption, 10s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_darkSpinStarted || !me->HealthBelowPctDamaged(25, damage))
            return;

        _darkSpinStarted = true;
        events.ScheduleEvent(EventDarkSpin, 1ms);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
            me->Yell(SlayTexts[urand(0, 4)], LANG_UNIVERSAL);
    }

    void JustDied(Unit* killer) override
    {
        me->Yell(DeathText, LANG_UNIVERSAL);
        BossAIBase::JustDied(killer);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (uint32 eventId = events.ExecuteEvent())
            ExecuteRiftEvent(eventId);

        // 原版进入黑暗旋转阶段后停止普通攻击，但继续施放既有法术。
        if (!_darkSpinStarted)
            DoMeleeAttackIfReady();
    }

protected:
    void ConfigureTier() override
    {
        // DBC 30500/28599 基础伤害约 1260/1618-1881；2.5 倍后约 3.1k/4.0-4.7k，符合 83 级团队首领单次伤害。
        SetRaidSpellDamageMultiplier(2.5f);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventDeathCoil:
                if (me->HealthBelowPct(90))
                    CastIfConfigured(SelectRandomPlayer(30.0f), SpellDeathCoil, true);
                events.ScheduleEvent(EventDeathCoil, Milliseconds(urand(12150, 19850)));
                break;
            case EventShadowFissure:
                CastIfConfigured(SelectRandomPlayer(60.0f), SpellShadowFissure, true);
                events.ScheduleEvent(EventShadowFissure, Milliseconds(urand(8450, 9450)));
                break;
            case EventShadowCleave:
                CastIfConfigured(me->GetVictim(), SpellShadowCleave);
                events.ScheduleEvent(EventShadowCleave, Milliseconds(urand(1200, 23900)));
                break;
            case EventDarkSpin:
                // 原版由 30502 黑暗旋转光环每 1 秒自动触发 30505 暗影箭（随机玩家）与 30508 旋风武器打击。
                // 裂隙改为显式周期事件，使 30505 通过调谐施放（读取 rift_spell_damage 中 30505 的 T1 基线）。
                events.ScheduleEvent(EventDarkSpinTick, 1s);
                break;
            case EventDarkSpinTick:
                if (Unit* target = SelectRandomPlayer(100.0f))
                    CastIfConfigured(target, SpellShadowBolt, true);
                CastIfConfigured(me, SpellDarkSpinPulse, true);
                events.ScheduleEvent(EventDarkSpinTick, 1s);
                break;
            case EventShadowBoltVolley: // T2新增：与暗影裂隙至少错开 3 秒，避免两次全队高伤重合
                if (events.GetTimeUntilEvent(EventShadowFissure) < 3s)
                {
                    events.ScheduleEvent(EventShadowBoltVolley, 4s);
                    break;
                }
                CastFinalRaidDamageSpell(me, SpellShadowBoltVolley, SPELLVALUE_BASE_POINT0,
                    ShadowBoltVolleyRaidDamage);
                events.ScheduleEvent(EventShadowBoltVolley, 20s);
                break;
            case EventCorruption: // T3新增：避开暗影裂隙及暗影箭雨的爆发窗口
                if (events.GetTimeUntilEvent(EventShadowFissure) < 3s ||
                    events.GetTimeUntilEvent(EventShadowBoltVolley) < 3s)
                {
                    events.ScheduleEvent(EventCorruption, 5s);
                    break;
                }
                CastFinalRaidDamageSpell(me, SpellCorruption, SPELLVALUE_BASE_POINT0,
                    CorruptionRaidDamagePerTick);
                events.ScheduleEvent(EventCorruption, 24s);
                break;
            default:
                break;
        }
    }

private:
    bool _darkSpinStarted = false;
};

void AddSC_boss_rift_nethekurse()
{
    RegisterCreatureAI(boss_rift_nethekurse);
}

} // namespace HeroicDungeonRift
