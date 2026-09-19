/*
 * 世界BOSS：勒什雷尔（Broodlord Lashlayer，黑翼之巢复刻，83级）
 *
 * 复刻黑翼之巢·勒什雷尔（龙类将军）的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（EasternKingdoms/BlackrockMountain/BlackwingLair/boss_broodlord_lashlayer.cpp）：
 *  - 省去抑制装置（Suppression Device）房间机制与相关游戏对象脚本；
 *  - 保留核心战斗：顺劈斩、冲击波、致死打击、击退。
 * 技能伤害统一由 WorldBossGuardAI 基类（world_boss_guard.cpp）缩放。
 * 该BOSS用于世界地图随机刷新（普通生物，非召唤物/无 owner），无固定房间坐标。
 */

#include "ScriptedCreature.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

enum BroodlordSpells
{
    // 核心技能
    SPELL_CLEAVE        = 26350, // 顺劈斩：正面近战顺劈
    SPELL_BLAST_WAVE    = 23331, // 冲击波：AOE 火焰伤害并减速
    SPELL_MORTAL_STRIKE = 24573, // 致死打击：降低治疗效果
    SPELL_KNOCK_AWAY    = 25778, // 击退：击退当前目标并降低仇恨
};

enum BroodlordEvents
{
    EVENT_CLEAVE        = 1,
    EVENT_BLAST_WAVE    = 2,
    EVENT_MORTAL_STRIKE = 3,
    EVENT_KNOCK_AWAY    = 4,
};

struct boss_world_broodlord : public WorldBossGuardAI
{
    boss_world_broodlord(Creature* creature) : WorldBossGuardAI(creature) { }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);

        events.ScheduleEvent(EVENT_CLEAVE, 8s);
        events.ScheduleEvent(EVENT_BLAST_WAVE, 12s);
        events.ScheduleEvent(EVENT_MORTAL_STRIKE, 20s);
        events.ScheduleEvent(EVENT_KNOCK_AWAY, 30s);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_CLEAVE:
                // 顺劈斩：正面近战顺劈
                DoCastVictim(SPELL_CLEAVE);
                events.Repeat(7s);
                break;

            case EVENT_BLAST_WAVE:
                // 冲击波：AOE 火焰伤害并减速
                DoCastVictim(SPELL_BLAST_WAVE);
                events.Repeat(20s, 35s);
                break;

            case EVENT_MORTAL_STRIKE:
                // 致死打击：降低治疗效果
                DoCastVictim(SPELL_MORTAL_STRIKE);
                events.Repeat(25s, 35s);
                break;

            case EVENT_KNOCK_AWAY:
                // 击退：击退当前目标并降低 50% 仇恨
                DoCastVictim(SPELL_KNOCK_AWAY);
                if (Unit* victim = me->GetVictim())
                    if (DoGetThreat(victim))
                        DoModifyThreatByPercent(victim, -50);
                events.Repeat(15s, 30s);
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
};

void AddSC_boss_world_broodlord()
{
    RegisterCreatureAI(boss_world_broodlord);
}
