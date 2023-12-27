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

class worldboss_dalingzhu : public CreatureScript
{
public:
    worldboss_dalingzhu() : CreatureScript("worldboss_dalingzhu") { }

    struct worldboss_dalingzhuAI : public ScriptedAI
    {
        worldboss_dalingzhuAI(Creature* creature) : ScriptedAI(creature) {}

        void Reset() override
        {
            _scheduler.CancelAll();
            _inBerserk = false;
        }

        void JustRespawned() override
        {
            me->Yell(std::string("我又回来了!"), LANG_UNIVERSAL, NULL);
        }
        void EnterEvadeMode(EvadeReason why= EVADE_REASON_OTHER) override
        {
            Reset();
            ScriptedAI::EnterEvadeMode();
        }
        void JustEngagedWith(Unit* /*who*/) override
        {
            if (urand(0, 2)) {
                me->Yell(std::string("这次你们别想在逃!"), LANG_UNIVERSAL, NULL);
            }
            _scheduler.Schedule(4s, [this](TaskContext context)
                {
                    std::list<Unit*> targets;
                    SelectTargetList(targets, 4, SelectTargetMethod::Random, 0, 50.0f, false,false);
                    if (!targets.empty())
                        for (std::list<Unit*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                            me->CastSpell(*itr, SPELL_SHADOW_VOLLEY, true);
                    context.Repeat(2s, 3s);
                }).Schedule(15s, [this](TaskContext context)
                    {
                        me->CastSpell((Unit*)nullptr, SPELL_CLEAVE, false);
                        context.Repeat(30s);
                    }).Schedule(13s, 18s, [this](TaskContext context)
                            {
                                me->CastSpell((Unit*)nullptr, SPELL_THUNDERCLAP, false);
                                context.Repeat(20s, 30s);
                            })/*.Schedule(5s, [this](TaskContext context)
                                {
                                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50.0f, true))
                                    {
                                        DoCast(target, SPELL_BOOM);
                                    }
                                    context.Repeat(17s, 20s);
                                })*/.Schedule(20s, [this](TaskContext context)
                                    {
                                        me->CastSpell((Unit*)nullptr, SPELL_MARK_OF_KAZZAK_DAMAGE, false);
                                        context.Repeat(20s);
                                    }).Schedule(15s, [this](TaskContext context)
                                        {
                                            if (Unit* target = SelectTarget(SelectTargetMethod::MaxThreat, 0, 0.0f, false))
                                            {
                                                DoCast(target, SPELL_TOT);
                                            }
                                            context.Repeat(15s);
                                        }).Schedule(3s, [this](TaskContext  context)
                                            {
                                                if (HealthBelowPct(25)) {
                                                    if (!_inBerserk)
                                                    {
                                                        DoCastSelf(SPELL_BERSERK);
                                                        _inBerserk = true;
                                                    }
                                                }
                                                context.Repeat(3s);
                                            });
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER && urand(0, 2))
            {
                me->Yell(std::string("哈~又一个!"), LANG_UNIVERSAL, NULL);
            }
        }

        void JustDied(Unit* /*killer*/) override
        {
            me->Yell(std::string("我...还会..回来的.."), LANG_UNIVERSAL, NULL);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim() || !CheckInDistance())
                return;

            _scheduler.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            DoMeleeAttackIfReady();
        }
        bool CheckInDistance()
        {
            if (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > 170.0f)
            {
                EnterEvadeMode();
                return false;
            }
            Unit* v = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true);
            if (!v)
            {
                EnterEvadeMode();
                return false;
            }
            return true;
        }
    private:
        TaskScheduler _scheduler;
        bool _inBerserk;
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
