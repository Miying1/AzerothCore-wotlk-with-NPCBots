/*
 * 世界BOSS：苏普雷姆斯（Supremus，黑暗神庙复刻，83级）
 *
 * 复刻黑暗神庙·苏普雷姆斯的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（Outland/BlackTemple/boss_supremus.cpp）：
 *  - 保留核心战斗：憎恨打击阶段（熔岩拳 + 憎恨打击）与凝视阶段（自我减速 + 火山喷发 + 凝视冲锋）每 1 分钟交替；
 *  - 火山（120510）与熔岩拳隐形巡者（120509）均为自定义召唤物（继承 WorldBossSummonAI），
 *    由召唤物自身施放触发型法术，使熔岩烈焰 / 火山间歇泉伤害经 WorldBossSummonAI::DamageDealt 统一缩放。
 * 技能伤害统一由 WorldBossGuardAI / WorldBossSummonAI 基类（world_boss_guard.cpp）缩放。
 * 该BOSS用于世界地图随机刷新（普通生物，非召唤物/无 owner），无固定房间坐标。
 */

#include "ScriptedCreature.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

enum SupremusSpells
{
    // 核心技能
    SPELL_SNARE_SELF                = 41922, // 自我减速（凝视阶段）
    SPELL_MOLTEN_FLAME              = 40980, // 熔岩烈焰（隐形巡者施放，触发熔岩烈焰伤害）
    SPELL_HATEFUL_STRIKE            = 41926, // 憎恨打击：打击近战范围内血量最高的目标
    SPELL_VOLCANIC_ERUPTION_TRIGGER = 40117, // 火山喷发触发（火山自身施放，触发间歇泉伤害）
    SPELL_BERSERK                   = 26662, // 狂暴
    SPELL_CHARGE                    = 41581, // 冲锋（凝视目标）
};

enum SupremusEmotes
{
    EMOTE_NEW_TARGET   = 0, // 凝视新目标
    EMOTE_PUNCH_GROUND = 1, // 捶地
    EMOTE_GROUND_CRACK = 2, // 地面裂开
    EMOTE_BERSERK      = 3, // 狂暴
};

enum SupremusEvents
{
    EVENT_HATEFUL_STRIKE    = 1, // 憎恨打击（憎恨打击阶段）
    EVENT_MOLTEN_PUNCH      = 2, // 熔岩拳（全程，召唤隐形巡者）
    EVENT_VOLCANIC_ERUPTION = 3, // 火山喷发（凝视阶段）
    EVENT_FIXATE            = 4, // 凝视随机目标并冲锋（凝视阶段）
    EVENT_PHASE_CHANGE      = 5, // 阶段切换（每 1 分钟）
    EVENT_BERSERK           = 6, // 狂暴（15 分钟）
};

// 苏普雷姆斯本体：憎恨打击阶段与凝视阶段每 1 分钟交替。
struct boss_world_supremus : public WorldBossGuardAI
{
    boss_world_supremus(Creature* creature) : WorldBossGuardAI(creature) { }

    void Reset() override
    {
        WorldBossGuardAI::Reset();
        _isGazePhase = false;
        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);
    }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);

        // 狂暴：15 分钟后一次性触发
        events.ScheduleEvent(EVENT_BERSERK, 15min);

        // 熔岩拳：全程持续，初始 20 秒
        events.ScheduleEvent(EVENT_MOLTEN_PUNCH, 20s);

        // 初始进入憎恨打击阶段
        EnterHatefulStrikePhase();
    }

    void JustSummoned(Creature* summon) override
    {
        // 火山与熔岩拳巡者均为自定义召唤物，各自在 IsSummonedBy 中施放触发型法术。
        summons.Summon(summon);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        summons.Despawn(summon);
    }

    // 进入憎恨打击阶段：近战 + 熔岩拳 + 憎恨打击。
    void EnterHatefulStrikePhase()
    {
        _isGazePhase = false;

        // 刚从凝视阶段切换过来时播放捶地动画
        if (me->HasAura(SPELL_SNARE_SELF))
            Talk(EMOTE_PUNCH_GROUND);

        me->RemoveAurasDueToSpell(SPELL_SNARE_SELF);
        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, false);

        events.CancelEvent(EVENT_VOLCANIC_ERUPTION);
        events.CancelEvent(EVENT_FIXATE);
        events.ScheduleEvent(EVENT_HATEFUL_STRIKE, 8s, 15s);
        events.ScheduleEvent(EVENT_PHASE_CHANGE, 60s);
    }

    // 进入凝视阶段：自我减速 + 火山喷发 + 凝视随机目标。
    void EnterGazePhase()
    {
        _isGazePhase = true;
        DoCastSelf(SPELL_SNARE_SELF, true);
        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
        me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, true);

        events.CancelEvent(EVENT_HATEFUL_STRIKE);
        events.ScheduleEvent(EVENT_VOLCANIC_ERUPTION, 5s);
        events.ScheduleEvent(EVENT_FIXATE, 0s);
        events.ScheduleEvent(EVENT_PHASE_CHANGE, 60s);
    }

    // 熔岩拳：在自身附近召唤隐形巡者，由其施放熔岩烈焰。
    void CastMoltenPunch()
    {
        Position pos = me->GetNearPosition(frand(0.0f, 18.0f), frand(0.0f, 6.2831853f));
        me->SummonCreature(NPC_WORLD_BOSS_SUPREMUS_PUNCH_STALKER, pos, TEMPSUMMON_TIMED_DESPAWN, 30 * IN_MILLISECONDS);
    }

    // 憎恨打击目标：近战范围内血量最高的目标。
    Unit* FindHatefulStrikeTarget()
    {
        Unit* target = nullptr;
        for (ThreatReference const* ref : me->GetThreatMgr().GetUnsortedThreatList())
        {
            if (Unit* unit = ref->GetVictim())
            {
                if (me->IsWithinMeleeRange(unit))
                    if (!target || unit->GetHealth() > target->GetHealth())
                        target = unit;
            }
        }
        return target;
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_HATEFUL_STRIKE:
                if (Unit* target = FindHatefulStrikeTarget())
                    DoCast(target, SPELL_HATEFUL_STRIKE);
                events.Repeat(1500ms, 15s);
                break;

            case EVENT_MOLTEN_PUNCH:
                CastMoltenPunch();
                events.Repeat(15s, 20s);
                break;

            case EVENT_VOLCANIC_ERUPTION:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                {
                    // 在随机目标脚下召唤自定义火山，由其自身施放火山喷发触发（间歇泉伤害经召唤物 AI 缩放）。
                    me->SummonCreature(NPC_WORLD_BOSS_SUPREMUS_VOLCANO, target->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 30 * IN_MILLISECONDS);
                    Talk(EMOTE_GROUND_CRACK);
                }
                events.Repeat(10s, 18s);
                break;

            case EVENT_FIXATE:
                // 凝视只点名真实玩家（不包含 NPCBot）；仇恨列表内没有真实玩家时
                // SelectPlayerTarget 会回退为普通选取，避免点名落空（仇恨已被清空）导致无目标。
                if (Unit* target = SelectPlayerTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                {
                    DoResetThreatList();
                    me->AddThreat(target, 5000000.0f);
                    Talk(EMOTE_NEW_TARGET);
                    if (target->IsWithinDist(me, 40.0f))
                        DoCast(target, SPELL_CHARGE);
                }
                events.Repeat(10s);
                break;

            case EVENT_PHASE_CHANGE:
                DoResetThreatList();
                if (_isGazePhase)
                    EnterHatefulStrikePhase();
                else
                    EnterGazePhase();
                break;

            case EVENT_BERSERK:
                DoCastSelf(SPELL_BERSERK, true);
                Talk(EMOTE_BERSERK);
                events.Reset();
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

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
            ExecuteEvent(eventId);

        DoMeleeAttackIfReady();
    }

private:
    bool _isGazePhase = false; // 是否处于凝视阶段
};

// 苏普雷姆斯的熔岩拳隐形巡者（召唤物）：施放熔岩烈焰并短暂跟随随机目标。
struct npc_world_boss_supremus_punch_stalker : public WorldBossSummonAI
{
    npc_world_boss_supremus_punch_stalker(Creature* creature) : WorldBossSummonAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        DoCastSelf(SPELL_MOLTEN_FLAME, true);

        // 触发型单位没有自己的仇恨，从召唤者那里选取跟随目标。
        if (Creature* supremus = summoner->ToCreature())
            if (Unit* target = supremus->AI()->SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                me->GetMotionMaster()->MoveFollow(target, 0.0f, 0.0f);

        scheduler.Schedule(6s, 10s, [this](TaskContext)
        {
            me->GetMotionMaster()->MoveIdle();
        });
    }

    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);
    }
};

// 苏普雷姆斯火山（召唤物）：自身施放火山喷发触发，间歇泉伤害经本 AI 的 DamageDealt 缩放。
struct npc_world_boss_supremus_volcano : public WorldBossSummonAI
{
    npc_world_boss_supremus_volcano(Creature* creature) : WorldBossSummonAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        // 40117 为仅自身（Self Only）法术，施加在火山自己身上，周期触发间歇泉伤害（42055->42052）。
        DoCastSelf(SPELL_VOLCANIC_ERUPTION_TRIGGER, true);
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        // 触发型单位：无主动行为，间歇泉伤害由触发链自动完成。
    }
};

void AddSC_boss_world_supremus()
{
    RegisterCreatureAI(boss_world_supremus);
    RegisterCreatureAI(npc_world_boss_supremus_punch_stalker);
    RegisterCreatureAI(npc_world_boss_supremus_volcano);
}
