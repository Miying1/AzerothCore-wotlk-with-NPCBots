/*
 * 世界BOSS：古尔图格·血沸（Gurtogg Bloodboil，黑暗神庙复刻，83级）
 *
 * 复刻黑暗神庙·古尔图格·血沸的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（Outland/BlackTemple/boss_bloodboil.cpp）：
 *  - 保留核心战斗：血沸（对最近 5 个目标的 DOT）、邪酸吐息、击退、弧光粉碎、迷惑打击，
 *    以及每 90 秒一次的邪能狂怒（随机非坦克目标、转移仇恨、召唤邪能间歇泉）；
 *  - 邪能间歇泉（120511）为自定义召唤物（继承 WorldBossSummonAI），由召唤物自身施放间歇泉伤害，
 *    使其伤害经 WorldBossSummonAI::DamageDealt 统一缩放；
 *  - 血沸的目标筛选与击退减仇恨复用原版法术脚本（spell_gurtogg_bloodboil / spell_gurtogg_eject）。
 * 技能伤害统一由 WorldBossGuardAI 基类（world_boss_guard.cpp）缩放。
 * 该BOSS用于世界地图随机刷新（临时召唤），无固定房间坐标。
 */

#include "ScriptedCreature.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

enum BloodboilSpells
{
    // 核心技能
    SPELL_ACIDIC_WOUND       = 40484, // 酸性创伤（自身光环）
    SPELL_FEL_ACID_BREATH1   = 40508, // 邪酸吐息
    SPELL_FEL_ACID_BREATH2   = 40595, // 邪酸吐息（邪能狂怒期间）
    SPELL_ARCING_SMASH1      = 40457, // 弧光粉碎
    SPELL_ARCING_SMASH2      = 40599, // 弧光粉碎（邪能狂怒期间）
    SPELL_EJECT1             = 40486, // 击退
    SPELL_EJECT2             = 40597, // 击退（邪能狂怒期间）
    SPELL_BEWILDERING_STRIKE = 40491, // 迷惑打击
    SPELL_BLOODBOIL          = 42005, // 血沸（对最近的 5 个目标造成 DOT）
    SPELL_BERSERK            = 45078, // 狂暴
    SPELL_CHARGE             = 40602, // 冲锋（邪能狂怒期间）

    // 邪能狂怒相关
    SPELL_FEL_GEYSER_STUN   = 40591, // 邪能间歇泉眩晕（自身）
    SPELL_FEL_GEYSER_DAMAGE = 40593, // 邪能间歇泉伤害（由召唤物施放）
    SPELL_FEL_RAGE_SELF     = 40594, // 邪能狂怒（自身）
    SPELL_FEL_RAGE_TARGET   = 40604, // 邪能狂怒（目标）
    SPELL_FEL_RAGE_2        = 40616, // 邪能狂怒 2（目标）
    SPELL_FEL_RAGE_3        = 41625, // 邪能狂怒 3（目标）
    SPELL_FEL_RAGE_SIZE     = 46787, // 邪能狂怒体型（目标）
    SPELL_TAUNT_GURTOGG     = 40603, // 嘲讽血沸（目标施放）
    SPELL_INSIGNIFICANCE    = 40618, // 无足轻重（自身）
};

enum BloodboilSays
{
    SAY_AGGRO   = 0,
    SAY_SLAY    = 1,
    SAY_SPECIAL = 2,
    SAY_ENRAGE  = 3,
    SAY_DEATH   = 4,
};

enum BloodboilEvents
{
    EVENT_BLOODBOIL       = 1, // 血沸（每 10 秒）
    EVENT_FEL_ACID_BREATH = 2, // 邪酸吐息（初始 38 秒，每 30 秒）
    EVENT_EJECT           = 3, // 击退（初始 14 秒，每 20 秒）
    EVENT_ARCING_SMASH    = 4, // 弧光粉碎（初始 5 秒，每 15 秒）
    EVENT_FEL_RAGE        = 5, // 邪能狂怒（初始 1 分钟，每 90 秒）
    EVENT_BERSERK         = 6, // 狂暴（10 分钟）
};

// 迷惑打击使用 scheduler 组，便于邪能狂怒时整体延迟。
constexpr uint32 GROUP_DELAY = 1;

struct boss_world_bloodboil : public WorldBossGuardAI
{
    boss_world_bloodboil(Creature* creature) : WorldBossGuardAI(creature) { }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        DoCastSelf(SPELL_ACIDIC_WOUND, true);

        events.ScheduleEvent(EVENT_BLOODBOIL, 10s);
        events.ScheduleEvent(EVENT_FEL_ACID_BREATH, 38s);
        events.ScheduleEvent(EVENT_EJECT, 14s);
        events.ScheduleEvent(EVENT_ARCING_SMASH, 5s);
        events.ScheduleEvent(EVENT_FEL_RAGE, 1min);
        events.ScheduleEvent(EVENT_BERSERK, 10min);

        // 迷惑打击：初始 28 秒，每 30 秒（邪能狂怒时整体延迟 30 秒）
        scheduler.Schedule(28s, [this](TaskContext context)
        {
            context.SetGroup(GROUP_DELAY);
            DoCastVictim(SPELL_BEWILDERING_STRIKE);
            context.Repeat(30s);
        });
    }

    void JustSummoned(Creature* summon) override
    {
        // 邪能间歇泉（120511）为自定义召唤物，在其 IsSummonedBy 中自行施放间歇泉伤害。
        summons.Summon(summon);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        summons.Despawn(summon);
    }

    void KilledUnit(Unit* /*victim*/) override
    {
        Talk(SAY_SLAY);
    }

    void JustDied(Unit* killer) override
    {
        WorldBossGuardAI::JustDied(killer);
        Talk(SAY_DEATH);
    }

    bool CanAIAttack(Unit const* who) const override
    {
        return !who->IsImmunedToDamage(SPELL_SCHOOL_MASK_ALL) && !who->HasUnitState(UNIT_STATE_CONFUSED);
    }

    // 邪能狂怒：随机非坦克玩家（withTank=false），转移仇恨并召唤邪能间歇泉。
    void CastFelRage()
    {
        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true, false))
        {
            me->RemoveAurasByType(SPELL_AURA_MOD_TAUNT);
            me->RemoveAurasDueToSpell(SPELL_ACIDIC_WOUND);
            DoCastSelf(SPELL_FEL_RAGE_SELF, true);
            DoCast(target, SPELL_FEL_RAGE_TARGET, true);
            DoCast(target, SPELL_FEL_RAGE_2, true);
            DoCast(target, SPELL_FEL_RAGE_3, true);
            DoCast(target, SPELL_FEL_RAGE_SIZE, true);
            target->CastSpell(me, SPELL_TAUNT_GURTOGG, true);
            // 在目标脚下召唤自定义邪能间歇泉，由其自身施放间歇泉伤害（经召唤物 AI 缩放）。
            me->SummonCreature(NPC_WORLD_BOSS_BLOODBOIL_GEYSER, *target, TEMPSUMMON_TIMED_DESPAWN, 30s);
            DoCastSelf(SPELL_FEL_GEYSER_STUN, true);
            DoCastSelf(SPELL_INSIGNIFICANCE, true);

            // 2 秒后冲锋当前目标
            scheduler.Schedule(2s, [this](TaskContext)
            {
                DoCastVictim(SPELL_CHARGE);
            });

            // 28 秒后重新施放酸性创伤
            scheduler.Schedule(28s, [this](TaskContext)
            {
                DoCastSelf(SPELL_ACIDIC_WOUND, true);
            });
        }
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_BLOODBOIL:
                if (!me->HasAura(SPELL_FEL_RAGE_SELF))
                    me->CastCustomSpell(SPELL_BLOODBOIL, SPELLVALUE_MAX_TARGETS, 5, me, false);
                events.Repeat(10s);
                break;

            case EVENT_FEL_ACID_BREATH:
                DoCastVictim(me->HasAura(SPELL_FEL_RAGE_SELF) ? SPELL_FEL_ACID_BREATH2 : SPELL_FEL_ACID_BREATH1);
                events.Repeat(30s);
                break;

            case EVENT_EJECT:
                DoCastVictim(me->HasAura(SPELL_FEL_RAGE_SELF) ? SPELL_EJECT2 : SPELL_EJECT1);
                events.Repeat(20s);
                break;

            case EVENT_ARCING_SMASH:
                DoCastVictim(me->HasAura(SPELL_FEL_RAGE_SELF) ? SPELL_ARCING_SMASH2 : SPELL_ARCING_SMASH1);
                events.Repeat(15s);
                break;

            case EVENT_FEL_RAGE:
                CastFelRage();
                events.Repeat(90s);
                scheduler.DelayGroup(GROUP_DELAY, 30s);
                break;

            case EVENT_BERSERK:
                Talk(SAY_ENRAGE);
                DoCastSelf(SPELL_BERSERK, true);
                break;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (CheckLeash())
            return;

        if (!UpdateVictim())
            return;

        events.Update(diff);
        scheduler.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
            ExecuteEvent(eventId);

        DoMeleeAttackIfReady();
    }
};

// 古尔图格·血沸的邪能间歇泉（召唤物）：自身施放间歇泉伤害（AOE + 击退），经本 AI 的 DamageDealt 缩放。
struct npc_world_boss_bloodboil_geyser : public WorldBossSummonAI
{
    npc_world_boss_bloodboil_geyser(Creature* creature) : WorldBossSummonAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        // 40593 为瞬发直伤（18 码 AOE + 击退），射程 0 码（仅自身）。
        // 预置施法记录：瞬发直伤不触发 OnSpellStart，且 OnSpellCast 在伤害结算之后才更新，
        // 若不预置，仅施放一次的间歇泉伤害会因 _lastCastSpellId 为 0 而漏缩放。
        SetLastCastSpellId(SPELL_FEL_GEYSER_DAMAGE);
        DoCastSelf(SPELL_FEL_GEYSER_DAMAGE, true);
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        // 触发型单位：无主动行为，间歇泉伤害施放一次后由召唤者控制生命周期。
    }
};

void AddSC_boss_world_bloodboil()
{
    RegisterCreatureAI(boss_world_bloodboil);
    RegisterCreatureAI(npc_world_boss_bloodboil_geyser);
}
