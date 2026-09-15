/*
 * 世界BOSS：库林纳克斯（Kurinnaxx，安其拉废墟复刻，83级）
 *
 * 复刻安其拉废墟·库林纳克斯的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（Kalimdor/RuinsOfAhnQiraj/boss_kurinnaxx.cpp）：
 *  - 省去死亡时召唤安多罗夫与奥斯里安喊话等副本剧情；
 *  - 保留核心战斗：致死创伤、沙陷阱、横扫、痛击、激怒。
 * 沙陷阱参照拉格纳罗斯熔岩喷发，由库林纳克斯自行生成并定时引爆（伤害经游戏对象施放，不缩放）。
 * 该BOSS用于世界地图随机刷新（临时召唤），无固定房间坐标。
 */

#include "GameObject.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

#include <deque>

enum KurinnaxxSpells
{
    SPELL_MORTAL_WOUND = 25646, // 致死创伤：降低治疗效果（可叠加）
    SPELL_SAND_TRAP_DMG = 25656, // 沙陷阱伤害（自然伤害 + 减速 + 定身）
    SPELL_ENRAGE       = 26527, // 激怒
    SPELL_WIDE_SLASH   = 25814, // 横扫：正面锥形 AOE
    SPELL_THRASH       = 3391,  // 痛击：额外攻击
};

enum KurinnaxxGameObjects
{
    GO_SAND_TRAP = 180647, // 沙陷阱
};

enum KurinnaxxEvents
{
    EVENT_MORTAL_WOUND   = 1,
    EVENT_SAND_TRAP      = 2,
    EVENT_SAND_TRAP_TICK = 3,
    EVENT_WIDE_SLASH     = 4,
    EVENT_THRASH         = 5,
};

struct boss_world_kurinnaxx : public WorldBossGuardAI
{
    boss_world_kurinnaxx(Creature* creature) : WorldBossGuardAI(creature) { }

    void Reset() override
    {
        WorldBossGuardAI::Reset();
        _sandTraps.clear();
        _enraged = false;
    }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);

        events.ScheduleEvent(EVENT_MORTAL_WOUND, 8s, 10s);
        events.ScheduleEvent(EVENT_SAND_TRAP, 5s, 15s);
        events.ScheduleEvent(EVENT_WIDE_SLASH, 10s, 15s);
        events.ScheduleEvent(EVENT_THRASH, 16s);
    }

    void DamageTaken(Unit*, uint32&, DamageEffectType, SpellSchoolMask) override
    {
        if (!_enraged && HealthBelowPct(30))
        {
            DoCastSelf(SPELL_ENRAGE);
            _enraged = true;
        }
    }

    // 沙陷阱：在随机目标脚下生成，5 秒后引爆。
    void SpawnSandTrap()
    {
        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
        {
            Position pos = target->GetPosition();
            if (GameObject* go = me->SummonGameObject(GO_SAND_TRAP, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
                    pos.GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 15))
                _sandTraps.push_back({ go->GetGUID(), 5 });
        }
    }

    // 每秒递减陷阱计时，到时引爆。
    void TickSandTraps()
    {
        for (auto it = _sandTraps.begin(); it != _sandTraps.end();)
        {
            if (--it->remaining == 0)
            {
                if (GameObject* go = ObjectAccessor::GetGameObject(*me, it->guid))
                    go->CastSpell(nullptr, SPELL_SAND_TRAP_DMG);
                it = _sandTraps.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_MORTAL_WOUND:
                DoCastVictim(SPELL_MORTAL_WOUND);
                events.Repeat(8s, 10s);
                break;

            case EVENT_SAND_TRAP:
                SpawnSandTrap();
                events.Repeat(5s, 15s);
                break;

            case EVENT_SAND_TRAP_TICK:
                TickSandTraps();
                if (!_sandTraps.empty())
                    events.Repeat(1s);
                break;

            case EVENT_WIDE_SLASH:
                DoCastSelf(SPELL_WIDE_SLASH);
                events.Repeat(12s, 15s);
                break;

            case EVENT_THRASH:
                DoCastSelf(SPELL_THRASH);
                events.Repeat(16s);
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
    struct SandTrap
    {
        ObjectGuid guid;
        uint32 remaining; // 剩余触发时间（秒）
    };
    std::deque<SandTrap> _sandTraps; // 待引爆的沙陷阱
    bool _enraged = false;
};

void AddSC_boss_world_kurinnaxx()
{
    RegisterCreatureAI(boss_world_kurinnaxx);
}
