/*
 * 世界BOSS：阿克蒙德（Archimonde，海加尔山之战复刻，83级）
 *
 * 复刻海加尔山之战·阿克蒙德（污染者）的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（Kalimdor/CavernsOfTime/BattleForMountHyjal/boss_archimonde.cpp）：
 *  - 保留核心战斗：气爆、军团之握（诅咒）、毁灭之火、死亡一指（无近战目标惩罚）、
 *    恐惧、灵魂充能（击杀玩家后按职业充能并释放对应灵魂）、10% 血量与 10 分钟狂暴；
 *  - 移除副本剧情专属：世界树吸取、智慧精灵、红天效果、永恒之井束缚等；
 *  - 毁灭之火（120516）为自定义召唤物（继承 WorldBossSummonAI），由召唤物自身携带
 *    31945 光环触发伤害链（31945->31943->31944 火焰直伤），伤害经 WorldBossSummonAI::DamageDealt 统一缩放；
 *  - 毁灭之火灵魂（120517）为移动引导者，复刻原版"转向 + 传送"的蔓延移动，使毁灭之火沿途留下火焰区域。
 * 技能伤害统一由 WorldBossGuardAI 基类（world_boss_guard.cpp）缩放。
 * 该BOSS用于世界地图随机刷新（临时召唤），无固定房间坐标。
 */

#include "ScriptedCreature.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
#include <vector>

enum ArchimondeSpells
{
    // 核心技能
    SPELL_AIR_BURST          = 32014, // 气爆（随机目标 3000 自然伤害 + 击飞）
    SPELL_GRIP_OF_THE_LEGION = 31972, // 军团之握（随机目标诅咒 DOT）
    SPELL_DOOMFIRE           = 31945, // 毁灭之火（召唤物携带的光环，触发火焰伤害链）
    SPELL_DOOMFIRE_SPAWN     = 32074, // 毁灭之火召唤（视觉）
    SPELL_FINGER_OF_DEATH    = 31984, // 死亡一指（无近战目标时惩罚，20000 暗影）
    SPELL_FEAR               = 31970, // 恐惧（AOE）

    // 灵魂充能（击杀玩家后按职业充能，2-10 秒后释放）
    SPELL_SOUL_CHARGE_RED     = 32052, // 灵魂充能·红（法师/牧师/术士）
    SPELL_SOUL_CHARGE_YELLOW  = 32045, // 灵魂充能·黄（DK/圣骑/盗贼/战士）
    SPELL_SOUL_CHARGE_GREEN   = 32051, // 灵魂充能·绿（德鲁伊/猎人/萨满）
    SPELL_UNLEASH_SOUL_RED    = 32053, // 灵魂释放·红（火焰 AOE）
    SPELL_UNLEASH_SOUL_YELLOW = 32054, // 灵魂释放·黄（物理 AOE）
    SPELL_UNLEASH_SOUL_GREEN  = 32057, // 灵魂释放·绿（自然 DOT）

    // 狂暴
    SPELL_HAND_OF_DEATH = 35354, // 死亡之手（10% 血量 / 10 分钟狂暴，99998 暗影秒杀）
};

enum ArchimondeSays
{
    SAY_AGGRO    = 0,
    SAY_SLAY     = 1,
    SAY_DOOMFIRE = 2,
    SAY_ENRAGE   = 3,
    SAY_DEATH    = 4,
};

enum ArchimondeEvents
{
    EVENT_AIR_BURST          = 1, // 气爆（初始 25-35 秒，之后 25-40 秒）
    EVENT_DOOMFIRE           = 2, // 毁灭之火（每 8 秒）
    EVENT_GRIP_OF_THE_LEGION = 3, // 军团之握（初始 25-35 秒，之后 5-25 秒）
    EVENT_FINGER_OF_DEATH    = 4, // 死亡一指检查（初始 5 秒，每 3.5 秒）
    EVENT_BERSERK            = 5, // 狂暴（10 分钟）
};

// 恐惧使用 scheduler 组，便于气爆成功时整体延迟 5 秒（复刻原版节奏）。
constexpr uint32 GROUP_FEAR = 1;

struct boss_world_archimonde : public WorldBossGuardAI
{
    boss_world_archimonde(Creature* creature) : WorldBossGuardAI(creature) { }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);

        events.ScheduleEvent(EVENT_AIR_BURST, 25s, 35s);
        events.ScheduleEvent(EVENT_DOOMFIRE, 8s);
        events.ScheduleEvent(EVENT_GRIP_OF_THE_LEGION, 25s, 35s);
        events.ScheduleEvent(EVENT_FINGER_OF_DEATH, 5s);
        events.ScheduleEvent(EVENT_BERSERK, 10min);

        // 恐惧：初始 40 秒，之后每 42 秒（气爆成功时整体延迟 5 秒）。
        scheduler.Schedule(40s, [this](TaskContext context)
        {
            context.SetGroup(GROUP_FEAR);
            DoCastAOE(SPELL_FEAR);
            context.Repeat(42s);
        });
    }

    void JustSummoned(Creature* summon) override
    {
        // 毁灭之火（120516）与毁灭之火灵魂（120517）由召唤物自身处理行为。
        summons.Summon(summon);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        summons.Despawn(summon);
    }

    void KilledUnit(Unit* victim) override
    {
        Talk(SAY_SLAY);

        // 灵魂充能：击杀玩家后按职业叠加对应充能印记，2-10 秒后释放对应灵魂。
        if (Player* player = victim->ToPlayer())
        {
            uint32 soulChargeSpell = 0;
            switch (player->getClass())
            {
                case CLASS_MAGE:
                case CLASS_PRIEST:
                case CLASS_WARLOCK:
                    soulChargeSpell = SPELL_SOUL_CHARGE_RED;
                    break;
                case CLASS_DEATH_KNIGHT:
                case CLASS_PALADIN:
                case CLASS_ROGUE:
                case CLASS_WARRIOR:
                    soulChargeSpell = SPELL_SOUL_CHARGE_YELLOW;
                    break;
                case CLASS_DRUID:
                case CLASS_HUNTER:
                case CLASS_SHAMAN:
                    soulChargeSpell = SPELL_SOUL_CHARGE_GREEN;
                    break;
                default:
                    break;
            }

            if (soulChargeSpell)
            {
                DoCastSelf(soulChargeSpell, true);
                scheduler.Schedule(2s, 10s, [this](TaskContext)
                {
                    UnleashSoulCharge();
                });
            }
        }
    }

    void JustDied(Unit* killer) override
    {
        WorldBossGuardAI::JustDied(killer);
        Talk(SAY_DEATH);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType, SpellSchoolMask) override
    {
        // 10% 血量软狂暴：秒杀当前目标（原版阿克蒙德的死亡之手惩罚）。
        if (!_enraged && me->HealthBelowPct(10))
        {
            _enraged = true;
            Talk(SAY_ENRAGE);
            me->InterruptNonMeleeSpells(false);
            DoCastVictim(SPELL_HAND_OF_DEATH);
        }
    }

    // 召唤毁灭之火：在自身周围随机角度处召唤移动引导者与毁灭之火本体，本体跟随引导者蔓延。
    void DoCastDoomFire()
    {
        Talk(SAY_DOOMFIRE);

        float x, y, z;
        me->GetClosePoint(x, y, z, me->GetObjectSize(), 15.0f, frand(0.0f, 6.2831853f)); // 2π 随机角度

        if (Creature* doomfireSpirit = me->SummonCreature(NPC_WORLD_BOSS_ARCHIMONDE_DOOMFIRE_SPIRIT, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 27s))
        {
            if (Creature* doomfire = me->SummonCreature(NPC_WORLD_BOSS_ARCHIMONDE_DOOMFIRE, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 27s))
            {
                doomfire->GetMotionMaster()->MoveFollow(doomfireSpirit, 0.0f, 0.0f);
            }
        }
    }

    // 释放灵魂充能：随机选择一个已叠加的充能印记，移除并释放对应灵魂。
    void UnleashSoulCharge()
    {
        me->InterruptNonMeleeSpells(false);

        static std::array<std::pair<uint32, uint32>, 3> const chargeAurasAndSpells =
        {{
            { SPELL_SOUL_CHARGE_RED,    SPELL_UNLEASH_SOUL_RED    },
            { SPELL_SOUL_CHARGE_YELLOW, SPELL_UNLEASH_SOUL_YELLOW },
            { SPELL_SOUL_CHARGE_GREEN,  SPELL_UNLEASH_SOUL_GREEN  },
        }};

        std::vector<uint32> availableAuras;
        std::vector<uint32> availableSpells;
        for (auto const& [aura, spell] : chargeAurasAndSpells)
        {
            if (me->HasAura(aura))
            {
                availableAuras.push_back(aura);
                availableSpells.push_back(spell);
            }
        }

        if (availableAuras.empty())
            return;

        // 掷硬币翻转释放顺序，模拟原版的不确定性。
        if (urand(0, 1))
        {
            std::reverse(availableAuras.begin(), availableAuras.end());
            std::reverse(availableSpells.begin(), availableSpells.end());
        }

        me->RemoveAuraFromStack(availableAuras.front());
        DoCastVictim(availableSpells.front());
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_AIR_BURST:
                // 气爆命中随机非坦克目标，成功时延迟恐惧 5 秒（复刻原版节奏）。
                if (DoCastRandomTarget(SPELL_AIR_BURST, 1) == SPELL_CAST_OK)
                    scheduler.DelayGroup(GROUP_FEAR, 5s);
                events.Repeat(25s, 40s);
                break;

            case EVENT_DOOMFIRE:
                DoCastDoomFire();
                events.Repeat(8s);
                break;

            case EVENT_GRIP_OF_THE_LEGION:
                DoCastRandomTarget(SPELL_GRIP_OF_THE_LEGION);
                events.Repeat(5s, 25s);
                break;

            case EVENT_FINGER_OF_DEATH:
                // 无近战目标时施放死亡一指惩罚（复刻原版"远程风筝惩罚"）。
                if (!me->GetVictim() || !me->GetVictim()->IsWithinMeleeRange(me))
                    DoCastRandomTarget(SPELL_FINGER_OF_DEATH);
                events.Repeat(3500ms);
                break;

            case EVENT_BERSERK:
                Talk(SAY_ENRAGE);
                me->InterruptNonMeleeSpells(false);
                DoCastVictim(SPELL_HAND_OF_DEATH);
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

private:
    bool _enraged = false; // 是否已触发 10% 血量软狂暴
};

// 阿克蒙德的毁灭之火（召唤物）：携带 31945 光环触发火焰伤害链（31945->31943->31944 直伤），
// 伤害经本 AI 的 DamageDealt 缩放；本体跟随毁灭之火灵魂移动，无主动行为。
struct npc_world_boss_archimonde_doomfire : public WorldBossSummonAI
{
    npc_world_boss_archimonde_doomfire(Creature* creature) : WorldBossSummonAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        // 31945 为触发链入口（周期触发 31943->31944 火焰直伤），
        // 预置施法记录以覆盖触发伤害时施法记录仍停留在 31945 的时序。
        SetLastCastSpellId(SPELL_DOOMFIRE);
        DoCastSelf(SPELL_DOOMFIRE_SPAWN, true);
        DoCastSelf(SPELL_DOOMFIRE, true);
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        // 触发型单位：无主动行为，跟随移动由召唤者通过 MoveFollow 控制，伤害由光环触发链提供。
    }
};

// 阿克蒙德的毁灭之火灵魂（召唤物）：移动引导者，复刻原版"转向 + 传送"的蔓延移动，
// 使毁灭之火沿途留下火焰区域。无伤害，故继承 ScriptedAI。
struct npc_world_boss_archimonde_doomfire_spirit : public ScriptedAI
{
    npc_world_boss_archimonde_doomfire_spirit(Creature* creature) : ScriptedAI(creature) { }

    static constexpr float TURN_CONSTANT = 0.785402f; // 45 度转向

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        // 生成时原地转向并前移 1 码。
        scheduler.Schedule(10ms, [this](TaskContext)
        {
            TryTeleportInDirection(1.0f, 3.1415927f, 1.0f, true); // π 原地转向
        });

        // 每 1.6 秒随机转向并前移，形成蔓延轨迹。
        scheduler.Schedule(1600ms, [this](TaskContext context)
        {
            float angle = irand(-1, 1) * TURN_CONSTANT;
            TryTeleportInDirection(8.0f, angle, 2.0f, false);
            context.Repeat(1600ms);
        });
    }

    void TryTeleportInDirection(float dist, float angle, float step, bool alwaysTurn)
    {
        Position pos;
        while (dist >= 0)
        {
            pos = me->WorldObject::GetFirstCollisionPosition(dist, angle);
            if (std::fabs(dist - me->GetExactDist2d(pos)) < 0.001f)
                break;
            dist -= step;
        }

        if (dist || alwaysTurn)
            me->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), Position::NormalizeOrientation(me->GetOrientation() + angle));
        else
            me->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), me->GetOrientation());
    }

    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);
    }
};

void AddSC_boss_world_archimonde()
{
    RegisterCreatureAI(boss_world_archimonde);
    RegisterCreatureAI(npc_world_boss_archimonde_doomfire);
    RegisterCreatureAI(npc_world_boss_archimonde_doomfire_spirit);
}
