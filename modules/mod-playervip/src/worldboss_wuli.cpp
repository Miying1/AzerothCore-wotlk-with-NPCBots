/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"



enum Spells
{
    SPELL_SHADOW_VOLLEY = 71818,//向目标射出一股黑色血箭，对目标及其周围6码内的盟友造成9250 to 10750点伤害。
    SPELL_CLEAVE = 63757,//对附近的所有敌人造成4250 to 5750点自然伤害，并使其攻击速度降低20%。
    SPELL_THUNDERCLAP = 64652,//践踏地面，将周围半径50码范围内的敌人打飞，并造成12000点冰霜伤害。。
    SPELL_TOT = 62130,//对一个敌人造成200%的武器伤害，并使其防御技能降低200点，持续15 seconds。
    SPELL_MARK_OF_KAZZAK_DAMAGE = 73796,//向前方释放出强大的能量波，对你面前半径20码范围内的正面锥形区域中的所有敌方目标造成150%的武器伤害。
    SPELL_ENRAGE = 63766,//以巨大的手臂发动横扫，产生一道冲击波，对其行进路上的所有敌人造成8788 to 10212点伤害。
    SPELL_BOOM = 64234,//向目标灌注黑暗的能量，使其在9 seconds后爆炸并拖拽附近的盟友。
    SPELL_VOID_BOLT = 62414,//对施法者面前半径10码范围的一个锥形区域内的敌人造成37000 to 43000点物理伤害。
    SPELL_HANBEN_PAOTAI = 72705,//召唤一道冰霜攻击其行进路线上的敌人。
    SPELL_BERSERK = 32965
};

/*
 * 事件表：战斗流程全部由 EventMap 事件驱动（使用核心 BOSS 模块 WorldBossAI 自带的事件表）。
 * 每个技能一个独立事件，各自独立续期；某个技能这一轮施放失败只会丢掉它自己，
 * 不会像原先的 TaskScheduler 串行写法那样连带影响其它技能。
 */
enum Events
{
    EVENT_SHADOW_VOLLEY  = 1,//暗影箭雨：随机 4 个 60 码内的目标
    EVENT_CLEAVE         = 2,//顺劈斩
    EVENT_THUNDERCLAP    = 3,//雷霆践踏
    EVENT_MARK_OF_KAZZAK = 4,//卡扎克的印记·伤害
    EVENT_TOT            = 5,//撕裂（致死打击）
};

/*
 * 脱战距离：本 BOSS 自己在 AI 内实现（LEASH_RANGE，150 码），与其它自定义世界BOSS
 * （src/server/scripts/Custom/WorldBoss 的 WORLD_BOSS_LEASH_RANGE）保持一致，不依赖核心配置
 * CreatureLeashRadius —— 那是全服通用值（各服务器取值不同，默认只有 30 码），而本 BOSS 带
 * CREATURE_TYPE_FLAG_BOSS_MOB，核心的回家距离判定对它恒定生效，靠配置放宽会牵连其它生物。
 */
constexpr float LEASH_RANGE = 150.0f;

class worldboss_dalingzhu : public CreatureScript
{
public:
    worldboss_dalingzhu() : CreatureScript("worldboss_dalingzhu") { }

    struct worldboss_dalingzhuAI : public WorldBossAI
    {
        worldboss_dalingzhuAI(Creature* creature) : WorldBossAI(creature)
        {
            // 本BOSS的所有技能都不允许被玩家打断：免疫一切“打断施法”效果
            // （脚踢/拳击/反制/法术封锁等，以及它们附带的法术系锁定）。
            // 63000 若已在 creature_template 的免疫模块里勾选 MECHANIC 26(INTERRUPT)，
            // 这里就是代码层兜底，保证缺配置时也不会被白打断。
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, true);
        }

        void Reset() override
        {
            WorldBossAI::Reset();//基类内部为 events.Reset() + 清理召唤物
            _inBerserk = false;
        }

        void JustRespawned() override
        {
            me->Yell(std::string("我又回来了!"), LANG_UNIVERSAL, NULL);
        }

        void EnterEvadeMode(EvadeReason why = EVADE_REASON_OTHER) override
        {
            // 必须在基类的 MoveTargetedHome() 之前还原真实刷新点（CheckLeash 在战斗中会临时改写它），
            // 否则“走回家”的目标点会变成最后一个战斗位置
            RestoreCoreHome();

            Reset();
            ScriptedAI::EnterEvadeMode(why);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            // 进入战斗时记录脱战距离基准点，并保存核心真实刷新点（战斗中会被临时改写，见 CheckLeash）
            _leashHome = me->GetPosition();
            _coreHome = me->GetHomePosition();
            _hasCoreHome = true;

            // 不调用基类的 _JustEngagedWith()（它会随机挑一个目标 AttackStart），
            // 保持“只打进入战斗时的那名玩家（开怪者）”
            if (urand(0, 2))
                me->Yell(std::string("这次你们别想在逃!"), LANG_UNIVERSAL, NULL);

            // 技能入队：开局刻意错开（4/7/15/21/26 秒，彼此至少间隔 5 秒），
            // 保证开场不会有两个技能同一刻到期
            events.ScheduleEvent(EVENT_SHADOW_VOLLEY, 4s);
            events.ScheduleEvent(EVENT_TOT, 7s);
            events.ScheduleEvent(EVENT_CLEAVE, 15s);
            events.ScheduleEvent(EVENT_THUNDERCLAP, 21s);
            events.ScheduleEvent(EVENT_MARK_OF_KAZZAK, 26s);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER && urand(0, 2))
            {
                me->Yell(std::string("哈~又一个!"), LANG_UNIVERSAL, NULL);
            }
        }

        void JustDied(Unit* killer) override
        {
            WorldBossAI::JustDied(killer);//基类内部会停掉全部事件
            RestoreCoreHome();//战斗中改写过 home 基准，死亡后要还原，避免复活后“家”落在死亡点
            me->Yell(std::string("我...还会..回来的.."), LANG_UNIVERSAL, NULL);
        }

        // 事件派发：每个事件在这里独立续期
        void ExecuteEvent(uint32 eventId) override
        {
            switch (eventId)
            {
                case EVENT_SHADOW_VOLLEY:
                {
                    std::list<Unit*> targets;
                    SelectTargetList(targets, 4, SelectTargetMethod::Random, 0, 60.0f, false, false);
                    for (Unit* target : targets)
                        me->CastSpell(target, SPELL_SHADOW_VOLLEY, true);
                    events.Repeat(3s, 4s);
                    break;
                }
                case EVENT_CLEAVE:
                    // 顺劈斩：打在当前目标身上（与核心剧本、其它世界BOSS的 CLEAVE 写法一致）
                    // 周期取 31 秒（与撕裂 23、印记 29 互质，避免周期性撞在一刻）
                    DoCastVictim(SPELL_CLEAVE);
                    events.Repeat(31s);
                    break;
                case EVENT_THUNDERCLAP:
                    // 雷霆践踏：与冰冠堡垒同名法术（SPELL_STOMP）一致，打在当前目标身上
                    // 周期固定 53 秒（质数，与 23/29/31 互质；固定值才能保证不与其它技能同刻）
                    DoCastVictim(SPELL_THUNDERCLAP);
                    events.Repeat(53s);
                    break;
                case EVENT_MARK_OF_KAZZAK:
                    // 卡扎克的印记·伤害：自身正前方的锥形能量波，按 AOE 施放
                    // 周期取 29 秒（与顺劈 31、撕裂 23 互质）
                    DoCastAOE(SPELL_MARK_OF_KAZZAK_DAMAGE);
                    events.Repeat(29s);
                    break;
                case EVENT_TOT:
                    // 周期取 23 秒（与顺劈 31、印记 29、践踏 53 互质；原 20 秒会与顺劈每 60 秒撞一次）
                    if (Unit* target = SelectTarget(SelectTargetMethod::MaxThreat, 0, 0.0f, false))
                        DoCast(target, SPELL_TOT);
                    events.Repeat(23s);
                    break;
                default:
                    break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            // 脱战距离检测：超过 LEASH_RANGE（以进入战斗时的位置为基准）直接脱战
            if (CheckLeash())
                return;

            if (!UpdateVictim())
                return;

            // 事件只推进到期时间，不在这里执行；正在读条时直接返回，
            // 到期事件会顺延到读条结束后再取，不会被白白丢掉
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            // 每帧最多取出并执行一个到期事件：避免同一时刻到期的技能被连续施放
            // （第一个进入读条后，第二个会因“已有法术正在施放”而施放失败）
            if (uint32 eventId = events.ExecuteEvent())
                ExecuteEvent(eventId);

            // 25% 血量狂暴：逐帧判定，不进事件表（原来 3 秒一次的检查几乎与所有技能同刻，
            // 是撞车最多的事件）；触发式施放，不占读条，也不会和本帧的事件施法互相干扰
            if (!_inBerserk && HealthBelowPct(25))
            {
                DoCastSelf(SPELL_BERSERK, true);
                _inBerserk = true;
            }

            DoMeleeAttackIfReady();
        }

    private:
        // 还原核心回家基准：战斗中 CheckLeash() 会把 home 临时改写到本体所在位置
        void RestoreCoreHome()
        {
            if (!_hasCoreHome)
                return;

            me->SetHomePosition(_coreHome);
            _hasCoreHome = false;
        }

        // 脱战距离检测：超过 LEASH_RANGE 即脱战，返回 true 表示已触发脱战、调用方应立即返回。
        bool CheckLeash()
        {
            if (!_hasCoreHome)
                return false;

            // 化解核心的回家距离判定（CreatureLeashRadius，全服默认 30 码）：63000 带
            // CREATURE_TYPE_FLAG_BOSS_MOB，核心不对它启用「近期受伤即可离开刷新点」的豁免，
            // 该判定在开放世界恒定生效（只有副本分支才直接放行）。做法与其它自定义世界BOSS
            // （WorldBossGuardAI::CheckLeash）一致：让核心回家基准跟随本体，使该判定恒为真；
            // 真实刷新点在进入战斗时保存，脱战/死亡前由 RestoreCoreHome() 还原。
            // 核心的距离基准优先取 IDLE 运动生成器的复位点（Random/Waypoint 会返回漫游/路径点），
            // 返回 false（默认 idle）时才回落到 m_homePosition，故这里先把带复位点的 IDLE 槽换掉；
            // 63000 当前 MovementType=0，这一步不会触发，仅为移动类型变更时兜底。
            if (MovementGenerator* idleSlot = me->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE))
            {
                float rx, ry, rz;
                if (idleSlot->GetResetPosition(rx, ry, rz))
                    me->GetMotionMaster()->MoveIdle();
            }

            if (me->GetDistance(me->GetHomePosition()) > 1.0f)
                me->SetHomePosition(me->GetPosition());

            // 本 BOSS 自己的脱战距离：从进入战斗时的位置算起，超过 LEASH_RANGE 即脱战
            if (me->IsInCombat() && me->GetDistance(_leashHome) > LEASH_RANGE)
            {
                EnterEvadeMode(EVADE_REASON_BOUNDARY);
                return true;
            }

            return false;
        }

        bool _inBerserk = false;
        Position _leashHome;       // 进入战斗时的位置（脱战距离基准）
        Position _coreHome;        // 核心真实刷新点（战斗中会被临时改写，脱战/死亡前还原）
        bool _hasCoreHome = false; // 是否已保存上述真实刷新点
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new worldboss_dalingzhuAI(creature);
    }
};

void Addworldboss_list()
{
    new worldboss_dalingzhu();
}
