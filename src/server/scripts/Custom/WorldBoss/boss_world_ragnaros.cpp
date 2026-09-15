/*
 * 世界BOSS：拉格纳罗斯（炎魔之王，熔火之心复刻，83级）
 *
 * 复刻熔火之心·拉格纳罗斯的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（EasternKingdoms/BlackrockMountain/MoltenCore/boss_ragnaros.cpp）：
 *  - 省去下潜/浮现阶段（原版 50% 血量下潜召唤火焰之子，击杀全部后浮现）；
 *  - 保留召唤火焰之子（改为周期性召唤，不再依赖下潜触发）；
 *  - 保留核心战斗：拉格纳罗斯之怒、拉格纳罗斯之手、岩浆爆裂、拉格纳罗斯之力、熔岩喷发。
 * 技能伤害统一由 WorldBossGuardAI 基类（world_boss_guard.cpp）缩放。
 * 该BOSS用于世界地图随机刷新（临时召唤），无固定房间坐标，熔岩喷发陷阱在自身周围随机位置生成。
 */

#include "GameObject.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

#include <deque>

enum RagnarosSpells
{
    // 核心技能
    SPELL_HAND_OF_RAGNAROS  = 19780, // 拉格纳罗斯之手：强化下一次近战并击退（自身 BUFF）
    SPELL_WRATH_OF_RAGNAROS = 20566, // 拉格纳罗斯之怒：击退当前目标
    SPELL_MAGMA_BLAST       = 20565, // 岩浆爆裂：近战无目标时的远程回退
    SPELL_MIGHT_OF_RAGNAROS = 21154, // 拉格纳罗斯之力：随机法力职业击退
    SPELL_LAVA_BURST_TRAP   = 21158, // 熔岩喷发陷阱：AOE 火焰伤害

    // 游戏对象
    GO_LAVA_BURST           = 178088, // 熔岩喷发陷阱（拉格纳罗斯召唤的熔岩喷口）
};

enum RagnarosEvents
{
    EVENT_WRATH_OF_RAGNAROS  = 1,
    EVENT_HAND_OF_RAGNAROS   = 2,
    EVENT_MAGMA_BLAST        = 3,
    EVENT_MIGHT_OF_RAGNAROS  = 4,
    EVENT_LAVA_BURST         = 5,
    EVENT_LAVA_BURST_TRIGGER = 6,
    EVENT_SUMMON_SONS        = 7,
};

// 火焰之子（召唤物）：近战火元素，出生后即进入战斗。
struct npc_world_boss_ragnaros_son_of_flame : public ScriptedAI
{
    npc_world_boss_ragnaros_son_of_flame(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetCorpseDelay(2);
        // 野外化：SetInCombatWithZone 在野外地图不生效，REACT_AGGRESSIVE 下由 UpdateVictim 搜索敌对目标进战。
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

struct boss_world_ragnaros : public WorldBossGuardAI
{
    boss_world_ragnaros(Creature* creature) : WorldBossGuardAI(creature) { }

    void Reset() override
    {
        WorldBossGuardAI::Reset();

        _lavaBurstGUIDs.clear();
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);

        events.ScheduleEvent(EVENT_WRATH_OF_RAGNAROS, 30s);
        events.ScheduleEvent(EVENT_HAND_OF_RAGNAROS, 25s);
        events.ScheduleEvent(EVENT_LAVA_BURST, 10s);
        events.ScheduleEvent(EVENT_MIGHT_OF_RAGNAROS, 11s);
        events.ScheduleEvent(EVENT_MAGMA_BLAST, 4s);
        events.ScheduleEvent(EVENT_SUMMON_SONS, 15s);
    }

    void JustDied(Unit* killer) override
    {
        WorldBossGuardAI::JustDied(killer);
        _lavaBurstGUIDs.clear();
    }

    // 当前目标是否在近战范围内（威胁系统会优先选择近战目标，
    // 因此当前目标不在近战范围意味着近战范围内没有任何目标）。
    bool IsVictimWithinMeleeRange() const
    {
        return me->GetVictim() && me->IsWithinMeleeRange(me->GetVictim());
    }

    // 召唤火焰之子（省去下潜，改为周期性召唤 2 个近战火元素）。
    void SummonSonsOfFlame()
    {
        for (uint8 i = 0; i < 2; ++i)
        {
            Position pos = me->GetNearPosition(frand(6.0f, 10.0f), frand(0.0f, 6.2831853f));
            me->SummonCreature(NPC_WORLD_BOSS_RAGNAROS_SON_OF_FLAME, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30s);
        }
    }

    // 熔岩喷发：在自身周围随机位置生成 3 个陷阱。
    void SpawnLavaBurstTraps()
    {
        for (uint8 i = 0; i < 3; ++i)
        {
            Position pos = me->GetNearPosition(frand(5.0f, 15.0f), frand(0.0f, 6.2831853f));
            if (GameObject* go = me->SummonGameObject(GO_LAVA_BURST, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(),
                    pos.GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 15))
                _lavaBurstGUIDs.push_back(go->GetGUID());
        }

        if (!_lavaBurstGUIDs.empty())
            events.ScheduleEvent(EVENT_LAVA_BURST_TRIGGER, 1s);
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_WRATH_OF_RAGNAROS:
                // 拉格纳罗斯之怒：击退当前目标
                DoCastVictim(SPELL_WRATH_OF_RAGNAROS);
                events.Repeat(25s);
                break;

            case EVENT_HAND_OF_RAGNAROS:
                // 拉格纳罗斯之手：强化下一次近战攻击并击退
                DoCastSelf(SPELL_HAND_OF_RAGNAROS);
                events.Repeat(20s);
                break;

            case EVENT_MAGMA_BLAST:
                // 岩浆爆裂：近战范围内无人时对当前目标远程攻击
                if (!IsVictimWithinMeleeRange())
                    if (Unit* victim = me->GetVictim())
                        DoCast(victim, SPELL_MAGMA_BLAST);
                events.Repeat(4s);
                break;

            case EVENT_MIGHT_OF_RAGNAROS:
                // 拉格纳罗斯之力：随机选择一名法力职业击退
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, [](Unit const* t)
                {
                    return t->IsPlayer() && t->getPowerType() == POWER_MANA;
                }))
                    DoCast(target, SPELL_MIGHT_OF_RAGNAROS);
                events.Repeat(11s, 30s);
                break;

            case EVENT_LAVA_BURST:
                // 熔岩喷发：生成陷阱
                SpawnLavaBurstTraps();
                break;

            case EVENT_LAVA_BURST_TRIGGER:
                // 依次引爆陷阱（每个间隔 1 秒）
                if (!_lavaBurstGUIDs.empty())
                {
                    ObjectGuid guid = _lavaBurstGUIDs.front();
                    _lavaBurstGUIDs.pop_front();

                    if (GameObject* go = ObjectAccessor::GetGameObject(*me, guid))
                    {
                        go->CastSpell(nullptr, SPELL_LAVA_BURST_TRAP);
                        go->SendCustomAnim(0);
                    }

                    if (!_lavaBurstGUIDs.empty())
                        events.Repeat(1s);
                    else
                        events.ScheduleEvent(EVENT_LAVA_BURST, 10s);
                }
                break;

            case EVENT_SUMMON_SONS:
                // 召唤火焰之子（省去下潜，改为周期性召唤）
                SummonSonsOfFlame();
                events.Repeat(45s);
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
    std::deque<ObjectGuid> _lavaBurstGUIDs; // 待引爆的熔岩喷发陷阱
};

void AddSC_boss_world_ragnaros()
{
    RegisterCreatureAI(boss_world_ragnaros);
    RegisterCreatureAI(npc_world_boss_ragnaros_son_of_flame);
}
