/*
 * 世界BOSS：阿兹加洛（Azgalor，海加尔山之战复刻，83级）
 *
 * 复刻海加尔山之战·阿兹加洛的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（Kalimdor/CavernsOfTime/BattleForMountHyjal/boss_azgalor.cpp）：
 *  - 保留核心战斗：顺劈斩、火雨、阿兹加洛的嚎叫（范围沉默）、末日诅咒、10 分钟狂暴；
 *  - 末日诅咒的目标筛选（排除主目标）复用原版法术脚本（spell_azgalor_doom，注册在 31347 上，
 *    不依赖副本环境）；末日诅咒的死亡由核心硬编码实现（SpellAuraEffects 中 31347 到期直接 Unit::Kill，
 *    原版的"末日降临 31348"在 DBC 中不存在，spell_azgalor_doom_aura 触发它会静默失败，故不影响）；
 *  - 移除副本剧情专属：开场巡逻路径、死亡后唤醒阿克蒙德的剧情联动。
 * 技能伤害统一由 WorldBossGuardAI 基类（world_boss_guard.cpp）缩放。
 * 该BOSS用于世界地图随机刷新（临时召唤），无固定房间坐标。
 */

#include "ScriptedCreature.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

enum AzgalorSpells
{
    SPELL_RAIN_OF_FIRE    = 31340, // 火雨（随机非近战目标，区域持续火焰 + 火雨 DOT）
    SPELL_DOOM            = 31347, // 末日诅咒（AOE 诅咒，排除主目标，到期由核心硬编码 Unit::Kill 杀死目标）
    SPELL_HOWL_OF_AZGALOR = 31344, // 阿兹加洛的嚎叫（AOE 沉默）
    SPELL_CLEAVE          = 31345, // 顺劈斩（物理近战）
    SPELL_BERSERK         = 26662, // 狂暴
};

enum AzgalorSays
{
    SAY_AGGRO  = 0,
    SAY_SLAY   = 1,
    SAY_DOOM   = 2,
    SAY_ENRAGE = 3,
    SAY_DEATH  = 4,
};

enum AzgalorEvents
{
    EVENT_CLEAVE       = 1, // 顺劈斩（初始 10-16 秒，之后 8-16 秒）
    EVENT_RAIN_OF_FIRE = 2, // 火雨（初始 20-25 秒，之后 12-35 秒）
    EVENT_HOWL         = 3, // 阿兹加洛的嚎叫（初始 30 秒，之后 18-20 秒）
    EVENT_DOOM         = 4, // 末日诅咒（初始 45-55 秒，之后 45-55 秒）
    EVENT_BERSERK      = 5, // 狂暴（10 分钟，之后每 5 分钟）
};

struct boss_world_azgalor : public WorldBossGuardAI
{
    boss_world_azgalor(Creature* creature) : WorldBossGuardAI(creature) { }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);

        events.ScheduleEvent(EVENT_CLEAVE, 10s, 16s);
        events.ScheduleEvent(EVENT_RAIN_OF_FIRE, 20s, 25s);
        events.ScheduleEvent(EVENT_HOWL, 30s);
        events.ScheduleEvent(EVENT_DOOM, 45s, 55s);
        events.ScheduleEvent(EVENT_BERSERK, 10min);
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

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_CLEAVE:
                DoCastVictim(SPELL_CLEAVE);
                events.Repeat(8s, 16s);
                break;

            case EVENT_RAIN_OF_FIRE:
                // 火雨：随机 40 码内目标（playerOnly=false，可砸 NPCBot）。
                DoCastRandomTarget(SPELL_RAIN_OF_FIRE, 0, 40.f, false);
                events.Repeat(12s, 35s);
                break;

            case EVENT_HOWL:
                DoCastAOE(SPELL_HOWL_OF_AZGALOR);
                events.Repeat(18s, 20s);
                break;

            case EVENT_DOOM:
                // 末日诅咒：AOE 施放（31347 的 SRC_AREA_ENEMY 目标，原版法术脚本排除主目标），
                // 到期由核心硬编码 Unit::Kill 杀死目标。
                Talk(SAY_DOOM);
                DoCastAOE(SPELL_DOOM);
                events.Repeat(45s, 55s);
                break;

            case EVENT_BERSERK:
                Talk(SAY_ENRAGE);
                DoCastSelf(SPELL_BERSERK);
                events.Repeat(5min);
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

void AddSC_boss_world_azgalor()
{
    RegisterCreatureAI(boss_world_azgalor);
}
