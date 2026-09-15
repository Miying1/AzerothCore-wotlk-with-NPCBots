/*
 * 世界BOSS：奥（Al'ar，风暴要塞复刻，83级）
 *
 * 复刻风暴要塞·奥的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（Outland/TempestKeep/Eye/boss_alar.cpp）：
 *  - 省去副本实例环境（无 DATA_ALAR、无平台路径点移动、无任务交互）；
 *  - 保留核心战斗：火焰盛宴、火焰羽刺、召唤余烬、假死重生、熔化护甲、冲锋、烈焰之痕、俯冲轰炸、狂暴。
 * 技能伤害统一由 WorldBossGuardAI 基类（world_boss_guard.cpp）缩放。
 * 该BOSS用于世界地图随机刷新（临时召唤），无固定房间坐标。
 */

#include "Player.h"
#include "ScriptedCreature.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

enum AlarSpells
{
    // 核心技能
    SPELL_BERSERK           = 45078, // 狂暴
    SPELL_FLAME_QUILLS      = 34229, // 火焰羽刺（复用原版 spell_alar_flame_quills 发射羽刺导弹）
    SPELL_FLAME_BUFFET      = 34121, // 火焰盛宴（近战无目标时 AOE）
    SPELL_MELT_ARMOR        = 35410, // 熔化护甲
    SPELL_CHARGE            = 35412, // 冲锋
    SPELL_DIVE_BOMB         = 35181, // 俯冲轰炸
    SPELL_REBIRTH           = 34342, // 重生（进入二阶段）
    SPELL_CLEAR_ALL_DEBUFFS = 34098, // 清除所有减益

    // 召唤物
    SPELL_EMBER_BIRTH         = 35177, // 余烬出生（视觉）
    SPELL_FLAME_PATCH_PERIODIC = 35380, // 烈焰之痕周期触发光环（每 1 秒触发一次烈焰伤害 35383）
};

enum AlarEvents
{
    EVENT_FLAME_BUFFET = 1,
    EVENT_FLAME_QUILLS = 2,
    EVENT_SUMMON_EMBER = 3,
    EVENT_REBIRTH      = 4,
    EVENT_MELT_ARMOR   = 5,
    // 注意：不能直接叫 EVENT_CHARGE，SharedDefines.h 的 EventId 枚举里已有同名枚举数（值 1003），会重定义冲突。
    EVENT_ALAR_CHARGE  = 6,
    EVENT_FLAME_PATCH  = 7,
    EVENT_DIVE_BOMB    = 8,
    EVENT_BERSERK      = 9,
};

enum AlarPhases
{
    PHASE_ONE = 1,
    PHASE_TWO = 2,
};

// 奥的余烬（召唤物）：出生后短暂停顿再进入战斗，作为普通火元素近战攻击。
struct npc_world_boss_alar_ember : public ScriptedAI
{
    npc_world_boss_alar_ember(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        DoCastSelf(SPELL_EMBER_BIRTH, true);

        // 野外化：SetInCombatWithZone 在野外地图不生效（CreatureAI::DoZoneInCombat 对非副本直接返回），
        // 改为切换主动状态后由 UpdateVictim 搜索附近敌对目标进战。
        scheduler.Schedule(3s, [this](TaskContext)
        {
            me->SetReactState(REACT_AGGRESSIVE);
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

// 烈焰之痕（召唤物）：自身施放周期触发光环，烈焰伤害经本 AI 的 DamageDealt 缩放。
struct npc_world_boss_alar_flame_patch : public WorldBossSummonAI
{
    npc_world_boss_alar_flame_patch(Creature* creature) : WorldBossSummonAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        // 35380 为周期触发光环，施加在烈焰之痕自己身上，每 1 秒触发一次烈焰伤害（35383）。
        DoCastSelf(SPELL_FLAME_PATCH_PERIODIC, true);
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        // 触发型单位：无主动行为，烈焰伤害由触发链自动完成。
    }
};

struct boss_world_alar : public WorldBossGuardAI
{
    boss_world_alar(Creature* creature) : WorldBossGuardAI(creature)
    {
        // 野外化：开启追逐移动，让 BOSS 能正常追击近战玩家；飞行外观由 Reset 中的 SetHover 处理。
        me->SetCombatMovement(true);
    }

    void Reset() override
    {
        WorldBossGuardAI::Reset();

        _phase = PHASE_ONE;
        _hasPretendedToDie = false;
        _noMelee = false;

        me->SetModelVisible(true);
        me->SetDisplayId(me->GetNativeDisplayId());
        me->SetStandState(UNIT_STAND_STATE_STAND);
        me->SetReactState(REACT_AGGRESSIVE);
        me->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
        me->SetHover(true); // 野外化：低空悬浮，保留飞行外观但不影响近战判定
    }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);

        // 一阶段技能循环
        events.ScheduleEvent(EVENT_FLAME_BUFFET, 2s);
        events.ScheduleEvent(EVENT_FLAME_QUILLS, 15s);
        events.ScheduleEvent(EVENT_SUMMON_EMBER, 10s);
    }

    void JustDied(Unit* killer) override
    {
        WorldBossGuardAI::JustDied(killer);
        me->SetModelVisible(true);
        me->SetDisplayId(me->GetNativeDisplayId());
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damagetype*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        // 一阶段血量归零 -> 假死，随后自动重生进入二阶段。
        if (_phase == PHASE_ONE && !_hasPretendedToDie && damage >= me->GetHealth())
        {
            damage = me->GetHealth() - 1;
            _hasPretendedToDie = true;

            events.Reset();
            PretendToDie();
            events.ScheduleEvent(EVENT_REBIRTH, 6s); // 野外化：缩短假死窗口，避免长时间脱战
        }
    }

    void PretendToDie()
    {
        _noMelee = true;
        me->InterruptNonMeleeSpells(true);
        me->RemoveAllAuras();
        me->SetReactState(REACT_PASSIVE);
        me->GetMotionMaster()->MovementExpired(false);
        me->GetMotionMaster()->MoveIdle();
        me->SetStandState(UNIT_STAND_STATE_DEAD);
        // 野外化：保持可选中、模型可见，玩家不会丢失目标（6 秒后重生）
    }

    void SchedulePhaseTwo()
    {
        events.ScheduleEvent(EVENT_MELT_ARMOR, 57s);
        events.ScheduleEvent(EVENT_ALAR_CHARGE, 10s);
        events.ScheduleEvent(EVENT_FLAME_PATCH, 20s);
        events.ScheduleEvent(EVENT_DIVE_BOMB, 34s);
        events.ScheduleEvent(EVENT_BERSERK, 10min);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_FLAME_BUFFET:
                // 近战范围内无敌人时释放火焰盛宴
                if (!me->SelectNearestTarget(me->GetCombatReach()))
                    DoCastAOE(SPELL_FLAME_BUFFET);
                events.Repeat(2s);
                break;

            case EVENT_FLAME_QUILLS:
                DoCastSelf(SPELL_FLAME_QUILLS);
                events.Repeat(25s);
                break;

            case EVENT_SUMMON_EMBER:
                SpawnEmbers();
                events.Repeat(30s);
                break;

            case EVENT_REBIRTH:
                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->SetModelVisible(true);
                me->SetDisplayId(me->GetNativeDisplayId());
                me->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
                me->SetReactState(REACT_AGGRESSIVE);

                DoCastSelf(SPELL_CLEAR_ALL_DEBUFFS, true);
                DoCastSelf(SPELL_REBIRTH);
                me->SetHealth(me->GetMaxHealth());

                _noMelee = false;
                _phase = PHASE_TWO;

                if (Unit* victim = me->GetVictim())
                    me->Attack(victim, true);
                SchedulePhaseTwo();
                break;

            case EVENT_MELT_ARMOR:
                DoCastVictim(SPELL_MELT_ARMOR);
                events.Repeat(60s);
                break;

            case EVENT_ALAR_CHARGE:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50.0f, true))
                    DoCast(target, SPELL_CHARGE, true);
                events.Repeat(30s);
                break;

            case EVENT_FLAME_PATCH:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50.0f, true))
                    me->SummonCreature(NPC_WORLD_BOSS_ALAR_FLAME_PATCH, *target, TEMPSUMMON_TIMED_DESPAWN, 2 * MINUTE * IN_MILLISECONDS);
                events.Repeat(30s);
                break;

            case EVENT_DIVE_BOMB:
                DoDiveBomb();
                events.Repeat(57s);
                break;

            case EVENT_BERSERK:
                DoCastSelf(SPELL_BERSERK, true);
                break;
        }
    }

    void SpawnEmbers()
    {
        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50.0f, true))
            for (uint8 i = 0; i < 2; ++i)
                me->SummonCreature(NPC_WORLD_BOSS_ALAR_EMBER, *target, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30 * IN_MILLISECONDS);
    }

    void DoDiveBomb()
    {
        // 野外化：不再隐身消失，改为对随机目标俯冲轰炸，全程保持可选中、可近战
        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 90.0f, true))
            DoCast(target, SPELL_DIVE_BOMB, true);
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

        if (!_noMelee)
            DoMeleeAttackIfReady();
    }

private:
    uint8 _phase;
    bool _hasPretendedToDie;
    bool _noMelee;
};

void AddSC_boss_world_alar()
{
    RegisterCreatureAI(boss_world_alar);
    RegisterCreatureAI(npc_world_boss_alar_ember);
    RegisterCreatureAI(npc_world_boss_alar_flame_patch);
}
