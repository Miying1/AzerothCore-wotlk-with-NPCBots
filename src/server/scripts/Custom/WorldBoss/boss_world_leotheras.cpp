/*
 * 世界BOSS：盲眼者莱欧瑟拉斯（Leotheras the Blind，毒蛇神殿复刻，83级）
 *
 * 复刻毒蛇神殿·盲眼者莱欧瑟拉斯的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（Outland/CoilfangReservoir/SerpentShrine/boss_leotheras_the_blind.cpp）：
 *  - 移除开战前的灰心缚法者（formation）机制，直接进入战斗；
 *  - 保留精灵形态（旋风斩）/ 恶魔形态（混沌冲击 + 疯狂低语）两阶段循环，
 *    以及 15% 血量时的最终形态（下跪召唤莱欧瑟拉斯之影）；
 *  - 内心的恶灵（120513）与莱欧瑟拉斯之影（120512）为自定义召唤物（继承 WorldBossSummonAI），
 *    使其伤害经 WorldBossSummonAI::DamageDealt 统一缩放；
 *  - 旋风斩（spell_leotheras_whirlwind）/ 混沌冲击（spell_leotheras_chaos_blast）复用原版法术脚本；
 *  - 疯狂低语改用自定义法术脚本（spell_world_boss_leotheras_insidious_whisper[_aura]），
 *    以替代依赖副本实例的原版脚本（原版通过 InstanceScript 获取 BOSS 施放疯狂吞噬）。
 * 技能伤害统一由 WorldBossGuardAI 基类（world_boss_guard.cpp）缩放。
 * 该BOSS用于世界地图随机刷新（临时召唤），无固定房间坐标。
 */

#include "GridNotifiers.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

enum LeotherasSpells
{
    SPELL_WHIRLWIND          = 37640, // 旋风斩（触发 37641 造成伤害）
    SPELL_CHAOS_BLAST        = 37674, // 混沌冲击（DUMMY，触发 37675）
    SPELL_INSIDIOUS_WHISPER  = 37676, // 疯狂低语（对最多 5 个非当前目标施放）
    SPELL_DUAL_WIELD         = 42459, // 双持
    SPELL_BERSERK            = 26662, // 狂暴（10 分钟）
    SPELL_METAMORPHOSIS      = 37673, // 变形（进入恶魔形态外观）
    SPELL_CONSUMING_MADNESS  = 37749, // 疯狂吞噬（惩罚：魅惑）
    SPELL_CLEAR_CONSUMING_MADNESS = 37750, // 清除疯狂吞噬（脱战清场：秒杀仍被魅惑的玩家）
    SPELL_SUMMON_INNER_DEMON = 37735, // 召唤内心的恶灵（原版法术，召唤原版内心的恶灵 21857）
    SPELL_SHADOW_BOLT        = 39309, // 暗影箭（内心的恶灵）
};

enum LeotherasSays
{
    SAY_AGGRO           = 0, // 终于，我的放逐结束了！
    SAY_SWITCH_TO_DEMON = 1, // 滚开，渺小的精灵！现在由我掌控！
    SAY_INNER_DEMONS    = 2, // 我们都有自己的心魔……
    SAY_DEMON_SLAY      = 3, // 我无人能敌！/ 去死吧，凡人！/ 是的……就是这样！
    SAY_NIGHTELF_SLAY   = 4, // 杀，杀！/ 没错，就是这样！/ 现在谁是主宰？
    SAY_FINAL_FORM      = 5, // 不……不！你们都干了些什么？！
    SAY_DEATH           = 6, // 我终于解脱了……
};

enum LeotherasEvents
{
    EVENT_WHIRLWIND         = 1, // 旋风斩（精灵形态）
    EVENT_SWITCH_TO_DEMON   = 2, // 切换恶魔形态
    EVENT_SWITCH_TO_ELF     = 3, // 切换精灵形态
    EVENT_INSIDIOUS_WHISPER = 4, // 疯狂低语（恶魔形态）
    EVENT_MOVE_IN_RANGE     = 5, // 恶魔形态保持 40 码距离
    EVENT_BERSERK           = 6, // 狂暴（10 分钟）
    EVENT_FINAL_FORM        = 7, // 最终形态：召唤莱欧瑟拉斯之影（下跪后 4 秒）
    EVENT_RESUME_COMBAT     = 8, // 最终形态：恢复战斗（下跪后 6 秒）
};

// 精灵形态：排定旋风斩与切恶魔形态
struct boss_world_leotheras : public WorldBossGuardAI
{
    boss_world_leotheras(Creature* creature) : WorldBossGuardAI(creature) { }

    void Reset() override
    {
        WorldBossGuardAI::Reset();
        _isDemonForm = false;
        _finalForm = false;
        DoCastSelf(SPELL_DUAL_WIELD, true);
        DoCastSelf(SPELL_CLEAR_CONSUMING_MADNESS, true); // 脱战时清除残留的疯狂吞噬（秒杀仍被魅惑的玩家）
    }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        _isDemonForm = false;
        _finalForm = false;

        events.ScheduleEvent(EVENT_BERSERK, 10min);
        ScheduleElfPhase();
    }

    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        summons.Despawn(summon);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim->IsPlayer())
            Talk(_isDemonForm ? SAY_DEMON_SLAY : SAY_NIGHTELF_SLAY);
    }

    void JustDied(Unit* killer) override
    {
        WorldBossGuardAI::JustDied(killer);
        Talk(SAY_DEATH);
    }

    // 恶魔形态使用远程攻击（40 码），精灵形态使用近战
    void AttackStart(Unit* who) override
    {
        if (_isDemonForm)
            AttackStartCaster(who, 40.0f);
        else
            ScriptedAI::AttackStart(who);
    }

    // 精灵形态阶段：旋风斩 + 切恶魔
    void ScheduleElfPhase()
    {
        DoResetThreatList();
        me->InterruptNonMeleeSpells(false);
        events.ScheduleEvent(EVENT_WHIRLWIND, 25s);
        events.ScheduleEvent(EVENT_SWITCH_TO_DEMON, 60s);
    }

    // 恶魔形态阶段：变形 + 保持距离 + 疯狂低语 + 切回精灵
    void ScheduleDemonPhase()
    {
        DoResetThreatList();
        _isDemonForm = true;
        me->InterruptNonMeleeSpells(false);
        me->LoadEquipment(0, true); // 恶魔形态卸下武器（视觉）
        DoCastSelf(SPELL_METAMORPHOSIS, true);

        events.ScheduleEvent(EVENT_MOVE_IN_RANGE, 1s);
        events.ScheduleEvent(EVENT_INSIDIOUS_WHISPER, 24s);
        events.ScheduleEvent(EVENT_SWITCH_TO_ELF, 60s);
    }

    // 恶魔形态：与目标保持 40 码距离（超出则逼近，否则原地）
    void MoveToTargetIfOutOfRange(Unit* target)
    {
        if (!me->IsWithinDistInMap(target, 40.0f))
        {
            me->GetMotionMaster()->MoveChase(target, 40.0f, 0);
            me->AddThreat(target, 0.0f);
        }
        else
            me->GetMotionMaster()->Clear();
    }

    // 切回精灵形态
    void TransformToElf()
    {
        _isDemonForm = false;
        me->RemoveAurasDueToSpell(SPELL_METAMORPHOSIS);
        me->LoadEquipment();
        me->InterruptNonMeleeSpells(false);
        if (me->GetVictim())
            me->ResumeChasingVictim();

        ScheduleElfPhase();
    }

    // 15% 血量触发最终形态：下跪召唤莱欧瑟拉斯之影
    void EnterFinalForm()
    {
        _finalForm = true;
        events.CancelEvent(EVENT_WHIRLWIND);
        events.CancelEvent(EVENT_SWITCH_TO_DEMON);
        events.CancelEvent(EVENT_SWITCH_TO_ELF);
        events.CancelEvent(EVENT_INSIDIOUS_WHISPER);
        events.CancelEvent(EVENT_MOVE_IN_RANGE);

        me->RemoveAurasDueToSpell(SPELL_WHIRLWIND);
        me->RemoveAurasDueToSpell(SPELL_METAMORPHOSIS);
        me->LoadEquipment();
        _isDemonForm = false;

        me->SetUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
        me->AttackStop();
        me->GetMotionMaster()->Clear();
        me->StopMoving();
        me->SetReactState(REACT_PASSIVE);
        me->SetStandState(UNIT_STAND_STATE_KNEEL);
        Talk(SAY_FINAL_FORM);

        events.ScheduleEvent(EVENT_FINAL_FORM, 4s);
        events.ScheduleEvent(EVENT_RESUME_COMBAT, 6s);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_WHIRLWIND:
                DoCastSelf(SPELL_WHIRLWIND);
                events.Repeat(30s);
                break;

            case EVENT_SWITCH_TO_DEMON:
                Talk(SAY_SWITCH_TO_DEMON);
                ScheduleDemonPhase();
                break;

            case EVENT_SWITCH_TO_ELF:
                TransformToElf();
                break;

            case EVENT_INSIDIOUS_WHISPER:
                Talk(SAY_INNER_DEMONS);
                me->CastCustomSpell(SPELL_INSIDIOUS_WHISPER, SPELLVALUE_MAX_TARGETS, 5, me, false);
                break;

            case EVENT_MOVE_IN_RANGE:
                // 恶魔形态保持在目标 40 码内（超出则逼近）
                MoveToTargetIfOutOfRange(me->GetVictim());
                events.Repeat(1s);
                break;

            case EVENT_BERSERK:
                DoCastSelf(SPELL_BERSERK, true);
                break;

            case EVENT_FINAL_FORM:
                // 在 BOSS 身边召唤莱欧瑟拉斯之影（15% 血量）
                me->SummonCreature(NPC_WORLD_BOSS_LEOTHERAS_SHADOW, me->GetPosition(), TEMPSUMMON_DEAD_DESPAWN, 0s);
                break;

            case EVENT_RESUME_COMBAT:
                DoResetThreatList();
                me->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->SetReactState(REACT_AGGRESSIVE);
                if (Unit* victim = me->GetVictim())
                    me->GetMotionMaster()->MoveChase(victim);
                ScheduleElfPhase();
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

        // 15% 血量触发最终形态（仅一次）
        if (!_finalForm && me->HealthBelowPct(15))
            EnterFinalForm();

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
            ExecuteEvent(eventId);

        // 最终形态下跪期间不攻击
        if (_finalForm && me->GetReactState() == REACT_PASSIVE)
            return;

        if (_isDemonForm)
        {
            // 恶魔形态：远程混沌冲击（每 2 秒），无近战
            if (me->isAttackReady(BASE_ATTACK))
            {
                if (DoCastVictim(SPELL_CHAOS_BLAST) != SPELL_CAST_OK)
                    DoMeleeAttackIfReady();
                else
                    me->setAttackTimer(BASE_ATTACK, 2000);
            }
        }
        else
        {
            DoMeleeAttackIfReady();
        }
    }

private:
    bool _isDemonForm = false; // 是否处于恶魔形态
    bool _finalForm = false;   // 是否已进入最终形态
};

// 内心的恶灵（召唤物）：只对召唤者（玩家）可见/可伤害，施放暗影箭
struct npc_world_boss_inner_demon : public WorldBossSummonAI
{
    npc_world_boss_inner_demon(Creature* creature) : WorldBossSummonAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        // 内心的恶灵只攻击召唤它的玩家
        if (summoner)
        {
            if (Unit* unit = summoner->ToUnit())
            {
                me->AddThreat(unit, 1000000.0f);
                AttackStart(unit);
            }
        }

        // 4 秒后开始施放暗影箭，之后每 6 秒一次
        scheduler.Schedule(4s, [this](TaskContext context)
        {
            DoCastVictim(SPELL_SHADOW_BOLT);
            context.Repeat(6s);
        });
    }

    void JustDied(Unit* /*killer*/) override
    {
        // 内心的恶灵死亡时移除召唤者的疯狂低语，避免误触发疯狂吞噬
        if (Unit* summoner = ObjectAccessor::GetUnit(*me, me->GetSummonerGUID()))
            summoner->RemoveAurasDueToSpell(SPELL_INSIDIOUS_WHISPER);
    }

    bool CanBeSeen(Player const* player) override
    {
        return player && player->GetGUID() == me->GetSummonerGUID();
    }

    bool CanAIAttack(Unit const* who) const override
    {
        return who->GetGUID() == me->GetSummonerGUID();
    }

    bool CanReceiveDamage(Unit* attacker)
    {
        return attacker && attacker->GetGUID() == me->GetSummonerGUID();
    }

    void OnCalculateMeleeDamageReceived(uint32& damage, Unit* attacker) override
    {
        if (!CanReceiveDamage(attacker))
            damage = 0;
    }

    void OnCalculateSpellDamageReceived(int32& damage, Unit* attacker) override
    {
        if (!CanReceiveDamage(attacker))
            damage = 0;
    }

    void OnCalculatePeriodicTickReceived(uint32& damage, Unit* attacker) override
    {
        if (!CanReceiveDamage(attacker))
            damage = 0;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        scheduler.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        DoMeleeAttackIfReady();
    }
};

// 莱欧瑟拉斯之影（最终形态召唤物）：15% 血量，近战 + 混沌冲击
struct npc_world_boss_leotheras_shadow : public WorldBossSummonAI
{
    npc_world_boss_leotheras_shadow(Creature* creature) : WorldBossSummonAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        // 阴影只有 15% 血量（对应莱欧瑟拉斯最终形态的血量）
        me->SetHealth(me->CountPctFromMaxHealth(15));

        if (summoner)
        {
            if (Unit* unit = summoner->ToUnit())
            {
                if (Unit* victim = unit->GetVictim())
                    AttackStart(victim);
                else
                    AttackStart(unit);
            }
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        // 近战 + 混沌冲击（每 2 秒）
        if (me->isAttackReady(BASE_ATTACK))
        {
            if (DoCastVictim(SPELL_CHAOS_BLAST) != SPELL_CAST_OK)
                DoMeleeAttackIfReady();
            else
                me->setAttackTimer(BASE_ATTACK, 2000);
        }
    }
};

// 疯狂低语（37676）：目标筛选复用原版逻辑（排除当前目标）
class spell_world_boss_leotheras_insidious_whisper : public SpellScript
{
    PrepareSpellScript(spell_world_boss_leotheras_insidious_whisper);

    void FilterTargets(std::list<WorldObject*>& unitList)
    {
        if (Unit* victim = GetCaster()->GetVictim())
            unitList.remove_if(Acore::ObjectGUIDCheck(victim->GetGUID(), true));
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_world_boss_leotheras_insidious_whisper::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
    }
};

// 疯狂低语（37676）光环：施加时召唤内心的恶灵，非默认移除时施放疯狂吞噬
class spell_world_boss_leotheras_insidious_whisper_aura : public AuraScript
{
    PrepareAuraScript(spell_world_boss_leotheras_insidious_whisper_aura);

    void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* owner = GetUnitOwner();
        if (!owner || !owner->IsPlayer())
            return;

        Unit* caster = GetCaster();
        if (!caster)
            return;

        // 世界BOSS：直接召唤自定义内心的恶灵（120513），伤害经 WorldBossSummonAI 统一缩放
        // 原版毒蛇神殿：施放 37735 召唤原版内心的恶灵（21857），保持原版副本行为不变
        if (caster->GetEntry() == NPC_WORLD_BOSS_LEOTHERAS)
            owner->SummonCreature(NPC_WORLD_BOSS_INNER_DEMON, *owner, TEMPSUMMON_TIMED_DESPAWN, 20s);
        else
            owner->CastSpell(owner, SPELL_SUMMON_INNER_DEMON, true);
    }

    void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        // 疯狂低语被非默认方式移除（玩家未及时击杀内心的恶灵）时，施放疯狂吞噬惩罚
        if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEFAULT)
            return;

        Unit* caster = GetCaster();
        Unit* target = GetUnitOwner();
        if (!caster || !target)
            return;

        // 惩罚：施放疯狂吞噬（魅惑）。GetCaster() 即莱欧瑟拉斯本体（世界BOSS 或原版均适用）
        caster->CastSpell(target, SPELL_CONSUMING_MADNESS, true);
    }

    void Register() override
    {
        AfterEffectApply += AuraEffectApplyFn(spell_world_boss_leotheras_insidious_whisper_aura::HandleEffectApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_world_boss_leotheras_insidious_whisper_aura::HandleEffectRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_boss_world_leotheras()
{
    RegisterCreatureAI(boss_world_leotheras);
    RegisterCreatureAI(npc_world_boss_inner_demon);
    RegisterCreatureAI(npc_world_boss_leotheras_shadow);
    RegisterSpellAndAuraScriptPair(spell_world_boss_leotheras_insidious_whisper, spell_world_boss_leotheras_insidious_whisper_aura);
}
