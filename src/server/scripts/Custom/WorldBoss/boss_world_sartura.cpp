/*
 * 世界BOSS：战争守卫沙尔图拉（Battleguard Sartura，安其拉神殿复刻，83级）
 *
 * 复刻安其拉神殿·战争守卫沙尔图拉的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（Kalimdor/TempleOfAhnQiraj/boss_sartura.cpp）：
 *  - 皇家守卫由沙尔图拉开战时召唤（原版在世界刷新时即存在）；
 *  - 保留核心战斗：旋风斩、破甲顺劈、激怒、狂暴，以及皇家守卫的旋风斩与击退。
 * 技能伤害统一由 WorldBossGuardAI / WorldBossSummonAI 基类（world_boss_guard.cpp）缩放。
 * 该BOSS用于世界地图随机刷新（普通生物，非召唤物/无 owner），无固定房间坐标。
 */

#include "ScriptedCreature.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

enum SarturaSpells
{
    // 战争守卫沙尔图拉
    SPELL_WHIRLWIND        = 26083, // 旋风斩：期间免疫眩晕，周期触发旋风斩伤害
    SPELL_ENRAGE           = 8269,  // 激怒
    SPELL_BERSERK          = 27680, // 狂暴
    SPELL_SUNDERING_CLEAVE = 25174, // 破甲顺劈
    // 沙尔图拉的皇家守卫
    SPELL_GUARD_WHIRLWIND = 26038, // 守卫旋风斩
    SPELL_GUARD_KNOCKBACK = 26027, // 守卫击退
};

enum SarturaEvents
{
    // 战争守卫沙尔图拉
    EVENT_SARTURA_WHIRLWIND        = 1,
    EVENT_SARTURA_WHIRLWIND_RANDOM = 2,
    EVENT_SARTURA_WHIRLWIND_END    = 3,
    EVENT_SARTURA_BERSERK          = 4,
    EVENT_SARTURA_SUNDERING_CLEAVE = 5,
    // 沙尔图拉的皇家守卫
    EVENT_GUARD_WHIRLWIND        = 6,
    EVENT_GUARD_WHIRLWIND_RANDOM = 7,
    EVENT_GUARD_WHIRLWIND_END    = 8,
    EVENT_GUARD_KNOCKBACK        = 9,
};

// 沙尔图拉的皇家守卫（召唤物）：近战巨人，旋风斩 + 击退。
struct npc_world_boss_sartura_guard : public WorldBossSummonAI
{
    npc_world_boss_sartura_guard(Creature* creature) : WorldBossSummonAI(creature) { }

    void IsSummonedBy(WorldObject*) override
    {
        me->SetCorpseDelay(2);
        // 野外化：SetInCombatWithZone 在野外地图不生效，REACT_AGGRESSIVE 下由 UpdateVictim 搜索敌对目标进战。
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void Reset() override
    {
        events.Reset();
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void JustEngagedWith(Unit*) override
    {
        events.ScheduleEvent(EVENT_GUARD_WHIRLWIND, 6s, 10s);
        events.ScheduleEvent(EVENT_GUARD_KNOCKBACK, 12s, 16s);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        while (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_GUARD_WHIRLWIND:
                    // 旋风斩：随机重置仇恨并周期触发旋风斩伤害
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                    {
                        me->GetThreatMgr().ResetAllThreat();
                        me->AddThreat(target, 1000.0f);
                    }
                    DoCastSelf(SPELL_GUARD_WHIRLWIND);
                    events.ScheduleEvent(EVENT_GUARD_WHIRLWIND_RANDOM, 2s, 7s);
                    events.ScheduleEvent(EVENT_GUARD_WHIRLWIND_END, 8s);
                    break;

                case EVENT_GUARD_WHIRLWIND_RANDOM:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                    {
                        me->GetThreatMgr().ResetAllThreat();
                        me->AddThreat(target, 1000.0f);
                    }
                    events.Repeat(2s, 7s);
                    break;

                case EVENT_GUARD_WHIRLWIND_END:
                    me->GetThreatMgr().ResetAllThreat();
                    me->SetReactState(REACT_AGGRESSIVE);
                    events.CancelEvent(EVENT_GUARD_WHIRLWIND_RANDOM);
                    events.ScheduleEvent(EVENT_GUARD_WHIRLWIND, 500ms, 9s);
                    break;

                case EVENT_GUARD_KNOCKBACK:
                    DoCastVictim(SPELL_GUARD_KNOCKBACK);
                    events.Repeat(21s, 37s);
                    break;
            }
        }

        DoMeleeAttackIfReady();
    }
};

struct boss_world_sartura : public WorldBossGuardAI
{
    boss_world_sartura(Creature* creature) : WorldBossGuardAI(creature) { }

    void Reset() override
    {
        WorldBossGuardAI::Reset();
        _enraged = false;
        _berserked = false;
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);

        events.ScheduleEvent(EVENT_SARTURA_WHIRLWIND, 12s, 22s);
        events.ScheduleEvent(EVENT_SARTURA_BERSERK, 10min);
        events.ScheduleEvent(EVENT_SARTURA_SUNDERING_CLEAVE, 2400ms, 3s);

        SummonGuards();
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, SpellSchoolMask) override
    {
        if (!_enraged && HealthBelowPct(20))
        {
            DoCastSelf(SPELL_ENRAGE);
            _enraged = true;
        }
    }

    // 召唤 3 个皇家守卫（原版在世界刷新时即存在，世界BOSS改为开战召唤）。
    void SummonGuards()
    {
        for (uint8 i = 0; i < 3; ++i)
        {
            Position pos = me->GetNearPosition(frand(3.0f, 6.0f), frand(0.0f, 6.2831853f));
            me->SummonCreature(NPC_WORLD_BOSS_SARTURA_GUARD, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30 * IN_MILLISECONDS);
        }
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_SARTURA_WHIRLWIND:
                // 旋风斩：随机重置仇恨并施放旋风斩（期间免疫眩晕）
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                {
                    me->GetThreatMgr().ResetAllThreat();
                    me->AddThreat(target, 1000.0f);
                }
                DoCastSelf(SPELL_WHIRLWIND);
                events.ScheduleEvent(EVENT_SARTURA_WHIRLWIND_RANDOM, 2s, 7s);
                events.ScheduleEvent(EVENT_SARTURA_WHIRLWIND_END, 15s);
                break;

            case EVENT_SARTURA_WHIRLWIND_RANDOM:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                {
                    me->GetThreatMgr().ResetAllThreat();
                    me->AddThreat(target, 1000.0f);
                }
                events.Repeat(2s, 7s);
                break;

            case EVENT_SARTURA_WHIRLWIND_END:
                me->GetThreatMgr().ResetAllThreat();
                me->SetReactState(REACT_AGGRESSIVE);
                events.CancelEvent(EVENT_SARTURA_WHIRLWIND_RANDOM);
                events.ScheduleEvent(EVENT_SARTURA_WHIRLWIND, 5s, 11s);
                break;

            case EVENT_SARTURA_BERSERK:
                if (!_berserked)
                {
                    DoCastSelf(SPELL_BERSERK, true);
                    _berserked = true;
                }
                break;

            case EVENT_SARTURA_SUNDERING_CLEAVE:
                // 破甲顺劈：旋风斩（被动）期间暂停
                if (me->HasReactState(REACT_PASSIVE))
                {
                    Milliseconds whirlwindTimer = events.GetTimeUntilEvent(EVENT_SARTURA_WHIRLWIND_END);
                    events.RescheduleEvent(EVENT_SARTURA_SUNDERING_CLEAVE, whirlwindTimer + 500ms);
                }
                else
                {
                    DoCastVictim(SPELL_SUNDERING_CLEAVE, false);
                    events.RescheduleEvent(EVENT_SARTURA_SUNDERING_CLEAVE, 2400ms, 3s);
                }
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
    bool _enraged = false;
    bool _berserked = false;
};

void AddSC_boss_world_sartura()
{
    RegisterCreatureAI(boss_world_sartura);
    RegisterCreatureAI(npc_world_boss_sartura_guard);
}
