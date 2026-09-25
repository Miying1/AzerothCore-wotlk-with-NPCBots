/*
 * 世界BOSS：伊利丹·怒风（黑暗神庙复刻，83级）
 *
 * 复刻黑暗神庙·伊利丹的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（Outland/BlackTemple/boss_illidan.cpp）：
 *  - 省去副本实例环境与剧情相关 NPC（阿卡玛、玛维、暗影牢笼、笼子陷阱、开场/结局动画、任务）；
 *  - 省去飞行阶段的「投掷战刃」机制（不再召唤阿兹诺斯之刃，着陆条件相应简化）；
 *  - 保留核心战斗：火焰碰撞、吸取灵魂、寄生暗影魔、飞行阶段（火球/眼棱/召唤烈焰）、
 *    痛苦烈焰、恶魔形态（暗影冲击/烈焰爆发/召唤暗影魔）、狂暴。
 * 技能伤害统一由 WorldBossGuardAI 基类（world_boss_guard.cpp）缩放。
 * 该BOSS用于世界地图随机刷新（普通生物，非召唤物/无 owner），无固定房间坐标（飞行阶段原地升空）。
 */

#include "Player.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

enum IllidanSpells
{
    // 通用
    SPELL_BERSERK               = 45078, // 狂暴
    SPELL_DUAL_WIELD            = 42459, // 双持
    SPELL_CLEAR_ALL_DEBUFFS     = 34098, // 清除所有减益

    // 一阶段 / 着陆阶段
    SPELL_FLAME_CRASH           = 40832, // 火焰碰撞（AOE）
    SPELL_DRAW_SOUL             = 40904, // 吸取灵魂（正面 AOE + 治疗，原版脚本处理治疗）
    SPELL_PARASITIC_SHADOWFIEND = 41917, // 寄生暗影魔（DOT）

    // 飞行阶段
    SPELL_FIREBALL              = 40598, // 火球
    SPELL_EYE_BLAST             = 39908, // 眼棱（引导 AOE）

    // 着陆阶段
    SPELL_AGONIZING_FLAMES      = 40932, // 痛苦烈焰（AOE + DOT）

    // 恶魔形态
    SPELL_DEMON_TRANSFORM_1     = 40511, // 恶魔变身（原版脚本链式触发）
    SPELL_DEMON_TRANSFORM_2     = 40398,
    SPELL_DEMON_TRANSFORM_3     = 40510,
    SPELL_DEMON_FORM            = 40506,
    SPELL_SHADOW_BLAST          = 41078, // 暗影冲击
    SPELL_FLAME_BURST           = 41126, // 烈焰爆发（原版脚本触发效果）
};

enum IllidanEvents
{
    EVENT_BERSERK = 1,
};

enum IllidanPhases
{
    PHASE_INITIAL = 0,
    PHASE_FLYING  = 1,
    PHASE_LANDING = 2,
    PHASE_DEMON   = 3,
};

enum IllidanEquipment
{
    EQUIPMENT_UNARMED = 0, // 空手
    EQUIPMENT_GLAIVES = 1, // 埃辛诺斯战刃
};

// 寄生暗影魔（召唤物）：近战，但不会攻击带有寄生 DOT 的目标。
struct npc_world_boss_illidan_parasitic_shadowfiend : public ScriptedAI
{
    npc_world_boss_illidan_parasitic_shadowfiend(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetCorpseDelay(2);
        me->SetReactState(REACT_DEFENSIVE);

        // 野外化：SetInCombatWithZone 在野外地图不生效，切换主动后由 UpdateVictim 搜索附近敌对目标进战。
        scheduler.Schedule(2400ms, [this](TaskContext)
        {
            me->SetReactState(REACT_AGGRESSIVE);
        });
    }

    bool CanAIAttack(Unit const* target) const override
    {
        // 不攻击携带寄生暗影魔 DOT 的目标（该目标由暗影魔吞噬）。
        return !target->HasAura(SPELL_PARASITIC_SHADOWFIEND);
    }

    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

// 阿兹诺斯烈焰（召唤物）：近战火元素，周期性烈焰冲击与冲锋。
struct npc_world_boss_illidan_flame : public WorldBossSummonAI
{
    npc_world_boss_illidan_flame(Creature* creature) : WorldBossSummonAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetCorpseDelay(2);
        me->SetReactState(REACT_DEFENSIVE);

        scheduler.Schedule(2s, [this](TaskContext)
        {
            me->SetReactState(REACT_AGGRESSIVE);
        });

        // 烈焰冲击（40631）
        scheduler.Schedule(6s, [this](TaskContext context)
        {
            SetLastCastSpellId(40631); // 预置：瞬发直伤不触发 OnSpellStart，首跳需手动记录来源
            DoCastVictim(40631);
            context.Repeat(8s);
        });

        // 冲锋（42003）
        scheduler.Schedule(10s, [this](TaskContext context)
        {
            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
            {
                SetLastCastSpellId(42003); // 预置：同上
                DoCast(target, 42003, true);
            }
            context.Repeat(12s);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

// 暗影魔（召唤物）：近战，周期性寻找目标（原版脚本触发吞噬灵魂）。
struct npc_world_boss_illidan_shadow_demon : public ScriptedAI
{
    npc_world_boss_illidan_shadow_demon(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetCorpseDelay(2);
        me->SetReactState(REACT_AGGRESSIVE);

        scheduler.Schedule(2s, [this](TaskContext context)
        {
            DoCastVictim(41082); // Find Target（原版 spell_illidan_found_target 处理吞噬）
            context.Repeat(2s);
        });
    }

    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

// 寄生暗影魔 DOT（41917）：DOT 移除时在世界BOSS伊利丹目标位置召唤寄生暗影魔。
// 与原版脚本共存：原版依赖实例（世界BOSS无实例时自动跳过），本脚本按 entry 守卫只对自定义世界BOSS生效。
class spell_world_boss_illidan_parasitic_shadowfiend_aura : public AuraScript
{
    PrepareAuraScript(spell_world_boss_illidan_parasitic_shadowfiend_aura);

    void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        if (!caster || !IsWorldBossBodyEntry(caster->GetEntry()))
            return;

        if (Creature* boss = caster->ToCreature())
            boss->SummonCreature(NPC_WORLD_BOSS_ILLIDAN_PARASITIC_SHADOWFIEND, *GetTarget(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30 * IN_MILLISECONDS);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_world_boss_illidan_parasitic_shadowfiend_aura::HandleEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
    }
};

struct boss_world_illidan : public WorldBossGuardAI
{
    boss_world_illidan(Creature* creature) : WorldBossGuardAI(creature) { }

    void Reset() override
    {
        WorldBossGuardAI::Reset();

        _phase = PHASE_INITIAL;
        _hasFlown = false;
        _isDemon = false;
        _dying = false;

        me->SetReactState(REACT_AGGRESSIVE);
        me->SetCombatMovement(true);
        me->SetDisableGravity(false);
        me->SetHover(false);
        me->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);

        DoCastSelf(SPELL_DUAL_WIELD, true);
        me->LoadEquipment(EQUIPMENT_GLAIVES, true);
    }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);

        SchedulePhase(PHASE_INITIAL);
        events.ScheduleEvent(EVENT_BERSERK, 25min);
    }

    void JustDied(Unit* killer) override
    {
        WorldBossGuardAI::JustDied(killer);
        me->SetDisableGravity(false);
        me->SetHover(false);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damagetype*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_dying)
            return;

        // 65% 触发飞行阶段（仅一次，且仅在非飞行/非恶魔状态）
        if (!_hasFlown && !_isDemon && _phase != PHASE_FLYING && me->HealthBelowPctDamaged(65, damage))
        {
            _hasFlown = true;
            EnterFlyingPhase();
            return;
        }

        // 30% 触发恶魔形态（仅一次）
        if (!_isDemon && _phase != PHASE_FLYING && me->HealthBelowPctDamaged(30, damage))
            EnterDemonPhase();
    }

    void SchedulePhase(uint8 phase)
    {
        scheduler.CancelAll();

        switch (phase)
        {
            case PHASE_INITIAL:
            case PHASE_LANDING:
                // 火焰碰撞
                scheduler.Schedule(26s, [this](TaskContext context)
                {
                    DoCastVictim(SPELL_FLAME_CRASH);
                    context.Repeat(30s);
                });
                // 吸取灵魂
                scheduler.Schedule(32s, [this](TaskContext context)
                {
                    DoCastVictim(SPELL_DRAW_SOUL);
                    context.Repeat(34s);
                });
                // 寄生暗影魔（对随机玩家 DOT）
                scheduler.Schedule(25s, [this](TaskContext context)
                {
                    DoCastRandomTarget(SPELL_PARASITIC_SHADOWFIEND, 0, 100.0f, true);
                    context.Repeat(30s);
                });

                if (phase == PHASE_LANDING)
                {
                    // 痛苦烈焰（自身 AOE，原版为 DoCastSelf）
                    scheduler.Schedule(24s, [this](TaskContext context)
                    {
                        DoCastSelf(SPELL_AGONIZING_FLAMES);
                        context.Repeat(26s);
                    });
                    // 着陆 60 秒后进入恶魔形态
                    scheduler.Schedule(60s, [this](TaskContext) { EnterDemonPhase(); });
                }
                break;

            case PHASE_DEMON:
                // 暗影冲击
                scheduler.Schedule(4s, [this](TaskContext context)
                {
                    DoCastVictim(SPELL_SHADOW_BLAST);
                    context.Repeat(4s);
                });
                // 烈焰爆发
                scheduler.Schedule(7s, [this](TaskContext context)
                {
                    DoCastSelf(SPELL_FLAME_BURST);
                    context.Repeat(19500ms);
                });
                // 召唤暗影魔
                scheduler.Schedule(30s, [this](TaskContext context)
                {
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                        me->SummonCreature(NPC_WORLD_BOSS_ILLIDAN_SHADOW_DEMON, *target, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30 * IN_MILLISECONDS);
                    context.Repeat(100s);
                });
                // 恶魔形态 60 秒后变回
                scheduler.Schedule(60s, [this](TaskContext) { ExitDemonPhase(); });
                break;
        }
    }

    void EnterFlyingPhase()
    {
        _phase = PHASE_FLYING;
        scheduler.CancelAll();

        me->InterruptNonMeleeSpells(true);
        me->SetReactState(REACT_PASSIVE);
        me->GetMotionMaster()->Clear();
        me->StopMovingOnCurrentPos();
        me->SetUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
        DoCastSelf(SPELL_CLEAR_ALL_DEBUFFS, true);

        // 原地升空
        me->SetDisableGravity(true);
        me->SetHover(true);

        // 延迟后召唤烈焰 + 卸下武器
        scheduler.Schedule(4s, [this](TaskContext)
        {
            me->LoadEquipment(EQUIPMENT_UNARMED, true);
            SummonFlames();
        });

        // 火球（空中持续）
        scheduler.Schedule(5s, [this](TaskContext context)
        {
            if (DoCastRandomTarget(SPELL_FIREBALL, 0, 100.0f, true) == SPELL_CAST_OK)
                context.Repeat(3s);
        });

        // 眼棱（空中周期）
        scheduler.Schedule(20s, [this](TaskContext context)
        {
            me->InterruptNonMeleeSpells(false);
            if (DoCastRandomTarget(SPELL_EYE_BLAST, 0, 100.0f, true) == SPELL_CAST_OK)
                context.Repeat(20s);
        });

        // 着陆检查：阿兹诺斯烈焰被摧毁后着陆
        scheduler.Schedule(10s, [this](TaskContext context)
        {
            summons.RemoveNotExisting();
            if (!summons.HasEntry(NPC_WORLD_BOSS_ILLIDAN_FLAME_OF_AZZINOTH))
                LandIllidan();
            else
                context.Repeat(3s);
        });

        // 超时强制着陆
        scheduler.Schedule(90s, [this](TaskContext)
        {
            if (_phase == PHASE_FLYING)
                LandIllidan();
        });
    }

    void SummonFlames()
    {
        // 召唤 1 只阿兹诺斯烈焰（正前方）
        Position pos = me->GetNearPosition(18.0f, 0.0f);
        me->SummonCreature(NPC_WORLD_BOSS_ILLIDAN_FLAME_OF_AZZINOTH, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 2 * MINUTE * IN_MILLISECONDS);
    }

    void LandIllidan()
    {
        _phase = PHASE_LANDING;
        scheduler.CancelAll();

        me->LoadEquipment(EQUIPMENT_GLAIVES, true);
        me->SetDisableGravity(false);
        me->SetHover(false);
        me->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
        me->SetReactState(REACT_AGGRESSIVE);

        DoResetThreatList();
        SchedulePhase(PHASE_LANDING);
    }

    void EnterDemonPhase()
    {
        _phase = PHASE_DEMON;
        _isDemon = true;
        scheduler.CancelAll();

        me->InterruptNonMeleeSpells(true);
        me->SetCombatMovement(false);
        me->SetReactState(REACT_AGGRESSIVE);
        DoCastSelf(SPELL_DEMON_TRANSFORM_1, true);

        SchedulePhase(PHASE_DEMON);
    }

    void ExitDemonPhase()
    {
        _phase = PHASE_LANDING;
        _isDemon = false;
        scheduler.CancelAll();

        me->RemoveAurasDueToSpell(SPELL_DEMON_TRANSFORM_1);
        me->RemoveAurasDueToSpell(SPELL_DEMON_TRANSFORM_2);
        me->RemoveAurasDueToSpell(SPELL_DEMON_TRANSFORM_3);
        me->RemoveAurasDueToSpell(SPELL_DEMON_FORM);
        me->LoadEquipment(EQUIPMENT_GLAIVES, true);
        me->SetCombatMovement(true);
        me->SetReactState(REACT_AGGRESSIVE);

        DoResetThreatList();
        SchedulePhase(PHASE_LANDING);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        if (eventId == EVENT_BERSERK)
            DoCastSelf(SPELL_BERSERK, true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (CheckLeash())
            return;

        scheduler.Update(diff);

        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
            ExecuteEvent(eventId);

        // 飞行阶段不近战
        if (_phase != PHASE_FLYING)
            DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
    bool _hasFlown;
    bool _isDemon;
    bool _dying;
};

void AddSC_boss_world_illidan()
{
    RegisterCreatureAI(boss_world_illidan);
    RegisterCreatureAI(npc_world_boss_illidan_parasitic_shadowfiend);
    RegisterCreatureAI(npc_world_boss_illidan_flame);
    RegisterCreatureAI(npc_world_boss_illidan_shadow_demon);
    RegisterSpellScript(spell_world_boss_illidan_parasitic_shadowfiend_aura);
}
