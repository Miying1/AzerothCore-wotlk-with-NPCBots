/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CellImpl.h"
#include "CreatureScript.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ObjectMgr.h"
#include "PassiveAI.h"
#include "Random.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "zulaman.h"

enum Yells
{
    SAY_AGGRO                   = 0,
    SAY_FIRE_BOMBS              = 1,
    SAY_SUMMON_HATCHER          = 2,
    SAY_ALL_EGGS                = 3,
    SAY_BERSERK                 = 4,
    SAY_SLAY                    = 5,
    SAY_DEATH                   = 6,
    SAY_EVENT_STRANGERS         = 7,
    SAY_EVENT_FRIENDS           = 8
};

enum Spells
{
    // Jan'alai
    SPELL_FLAME_BREATH          = 43140,//对施法者正面锥形区域内的敌人造成18001 to 20000点火焰伤害。
    SPELL_FIRE_WALL             = 43113,
    SPELL_ENRAGE                = 44779,
    SPELL_SUMMON_PLAYERS_DUMMY  = 43096,
    SPELL_SUMMON_PLAYERS        = 43097,
    SPELL_TELE_TO_CENTER        = 43098, // coord
    SPELL_HATCH_ALL             = 43144,
    SPELL_BERSERK               = 45078,

    // Fire Bob Spells
    SPELL_FIRE_BOMB_CHANNEL     = 42621, // last forever
    SPELL_FIRE_BOMB_THROW       = 42628, // throw visual
    SPELL_FIRE_BOMB_DUMMY       = 42629, // bomb visual
    SPELL_FIRE_BOMB_DAMAGE      = 42630,

    // Hatcher Spells
    SPELL_HATCH_EGG_ALL         = 42471,
    SPELL_HATCH_EGG_SINGULAR    = 43734,
    SPELL_SUMMON_HATCHLING      = 42493,

    // Hatchling Spells
    SPELL_FLAMEBUFFET           = 43299
};

enum Creatures
{
    NPC_AMANI_HATCHER           = 23818,
    NPC_EGG                     = 23817,
    NPC_FIRE_BOMB               = 23920
};

const int area_dx = 44;
const int area_dy = 51;

const Position janalainPos = {-33.93f, 1149.27f, 19.0f, 0.0f};

const Position fireWallCoords[4] =
{
    {-10.13f, 1149.27f, 19, 3.1415f},
    {-33.93f, 1123.90f, 19, 0.5f * 3.1415f},
    {-54.80f, 1150.08f, 19, 0.0f},
    {-33.93f, 1175.68f, 19, 1.5f * 3.1415f}
};

// 孵化者路径点数量（最后一点即蛋巢，到达后才会开始孵化）
constexpr uint32 hatcherWaypointCount = 5;

// 重置战斗时搜索龙鹰蛋的范围：必须覆盖整个平台，否则 Boss 在场地一侧脱战时，
// 另一侧远处的蛋不会被复活（蛋的重生时间长达 7200 秒，等于永远不重置）
constexpr float eggResetSearchRange = 250.0f;

// 孵化者只孵化自己身边的蛋（本侧蛋巢），避免隔着整个平台把对面蛋巢的蛋也孵了
constexpr float hatcherHatchRange = 30.0f;

const Position hatcherway[2][hatcherWaypointCount] =
{
    {
        {-87.46f, 1170.09f, 6.0f, 0.0f},
        {-74.41f, 1154.75f, 6.0f, 0.0f},
        {-52.74f, 1153.32f, 19.0f, 0.0f},
        {-33.37f, 1172.46f, 19.0f, 0.0f},
        {-33.09f, 1203.87f, 19.0f, 0.0f}
    },
    {
        {-86.57f, 1132.85f, 6.0f, 0.0f},
        {-73.94f, 1146.00f, 6.0f, 0.0f},
        {-52.29f, 1146.51f, 19.0f, 0.0f},
        {-33.57f, 1125.72f, 19.0f, 0.0f},
        {-34.29f, 1095.22f, 19.0f, 0.0f}
    }
};

enum Misc
{
    MAX_BOMB_COUNT              = 40,
    GROUP_ENRAGE                = 1,
    GROUP_HATCHING              = 2,
    DATA_ALL_EGGS_HATCHED       = 0
};

struct boss_janalai : public BossAI
{
    boss_janalai(Creature* creature) : BossAI(creature, DATA_JANALAI)
    {    }

    void Reset() override
    {
        BossAI::Reset();
        ResetEggs();
        _isBombing = false;
        _isFlameBreathing = false;

        ScheduleHealthCheckEvent(35, [&]{
            Talk(SAY_ALL_EGGS);
            me->AttackStop();
            me->GetMotionMaster()->Clear();
            me->SetPosition(janalainPos);
            me->StopMovingOnCurrentPos();
            DoCastAOE(SPELL_HATCH_ALL);
        });

        ScheduleHealthCheckEvent(20, [&] {
            if (!me->HasAura(SPELL_ENRAGE))
                DoCastSelf(SPELL_ENRAGE, true);
            me->m_Events.CancelEventGroup(GROUP_ENRAGE);
        });

        me->m_Events.KillAllEvents(false);
        _sideHatched[0] = false;
        _sideHatched[1] = false;
    }

    void JustDied(Unit* killer) override
    {
        Talk(SAY_DEATH);
        BossAI::JustDied(killer);
    }

    void JustSummoned(Creature* summon) override
    {
        if (summon->GetEntry() == NPC_AMANI_HATCHLING)
        {
            // 雏龙破壳后跑向平台中央。注意第三个参数是 Z 坐标，原实现误把路径点的 Y 坐标当成了 Z，
            // 导致雏龙朝一千多码高的空中跑
            uint8 side = summon->GetPositionY() > 1150.0f ? 0 : 1;
            Position const& midPos = hatcherway[side][hatcherWaypointCount - 2];
            summon->GetMotionMaster()->MovePoint(0,
                midPos.GetPositionX() + irand(-2, 2),
                1150.0f + irand(-2, 2),
                midPos.GetPositionZ());
        }

        BossAI::JustSummoned(summon);
    }

    void DamageDealt(Unit* target, uint32& damage, DamageEffectType /*damagetype*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_isFlameBreathing)
        {
            if (!me->HasInArc(M_PI / 6, target))
                damage = 0;
        }
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);
        //schedule abilities
        ScheduleTimedEvent(30s, [&]{
            StartBombing();
        }, 20s, 40s);

        scheduler.Schedule(10s, GROUP_HATCHING, [this](TaskContext context)
        {
            if (_sideHatched[0] && _sideHatched[1])
                return;

            Talk(SAY_SUMMON_HATCHER);

            if (_sideHatched[0] && !_sideHatched[1])
            {
                me->SummonCreature(NPC_AMANI_HATCHER, hatcherway[1][0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                me->SummonCreature(NPC_AMANI_HATCHER, hatcherway[1][0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
            }
            else if (!_sideHatched[0] && _sideHatched[1])
            {
                me->SummonCreature(NPC_AMANI_HATCHER, hatcherway[0][0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                me->SummonCreature(NPC_AMANI_HATCHER, hatcherway[0][0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
            }
            else
            {
                me->SummonCreature(NPC_AMANI_HATCHER, hatcherway[0][0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
                me->SummonCreature(NPC_AMANI_HATCHER, hatcherway[1][0], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10000);
            }

            context.Repeat(90s);
        });

        ScheduleTimedEvent(8s, [&]{
            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
            {
                me->AttackStop();
                me->GetMotionMaster()->Clear();
                DoCast(target, SPELL_FLAME_BREATH);
                me->StopMoving();
                _isFlameBreathing = true;
                // placeholder time idk yet
                scheduler.Schedule(2s, [this](TaskContext)
                {
                    _isFlameBreathing = false;
                });
            }
        }, 8s);

        me->m_Events.AddEventAtOffset([&] {
            DoCastSelf(SPELL_ENRAGE, true);
        }, 5min, 5min, GROUP_ENRAGE);

        me->m_Events.AddEventAtOffset([&] {
            Talk(SAY_BERSERK);
            DoCastSelf(SPELL_BERSERK);
        }, 10min);
    }

    void SetData(uint32 index, uint32 data) override
    {
        if (index == DATA_ALL_EGGS_HATCHED)
            _sideHatched[data] = true;
    }

    // 重置所有龙鹰蛋：先清掉残留的孵化者与雏龙，再复活平台上所有的蛋
    void ResetEggs()
    {
        summons.DespawnEntry(NPC_AMANI_HATCHER);
        summons.DespawnEntry(NPC_AMANI_HATCHLING);

        // 半径必须覆盖整个平台：南北两个蛋巢相距约 137 码，而搜索圆心是 Boss 脱战时的位置，
        // 半径太小（例如 100 码）就会漏掉对面蛋巢的蛋
        std::list<Creature*> eggList;
        me->GetCreaturesWithEntryInRange(eggList, eggResetSearchRange, NPC_EGG);
        for (Creature* egg : eggList)
            if (!egg->IsAlive())
                egg->Respawn(true);

        // 兜底：蛋对象已不在场（例如所在格子被卸载）时，清掉刷新计时并立即重建，
        // 否则要等 creature 表里的 7200 秒才会重新刷出来，等同于"永远不重置"
        for (auto const& [spawnId, data] : sObjectMgr->GetAllCreatureData())
        {
            if (data.id != NPC_EGG || uint32(data.mapid) != me->GetMap()->GetId())
                continue;

            if (janalainPos.GetExactDist2d(data.posX, data.posY) > eggResetSearchRange)
                continue;

            if (!me->GetMap()->GetCreature(ObjectGuid::Create<HighGuid::Unit>(NPC_EGG, spawnId)))
            {
                me->GetMap()->RemoveCreatureRespawnTime(spawnId);
                me->GetMap()->ProcessCreatureRespawn(spawnId);
            }
        }
    }

    void FireWall()
    {
        for (uint8 i = 0; i < 4; ++i)
        {
            uint8 wallNum = i == 0 || i == 2 ? 3 : 2;

            for (uint8 j = 0; j < wallNum; j++)
            {
                Creature* wall = wallNum == 3
                        ? me->SummonCreature(NPC_FIRE_BOMB, fireWallCoords[i].GetPositionX(), fireWallCoords[i].GetPositionY() + 5 * (j - 1), fireWallCoords[i].GetPositionZ(), fireWallCoords[i].GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 15000)
                        : me->SummonCreature(NPC_FIRE_BOMB, fireWallCoords[i].GetPositionX() - 2 + 4 * j, fireWallCoords[i].GetPositionY(), fireWallCoords[i].GetPositionZ(), fireWallCoords[i].GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 15000);

                if (wall)
                    wall->AI()->DoCastSelf(SPELL_FIRE_WALL, true);
            }
        }
    }

    void SpawnBombs()
    {
        float dx, dy;
        for (int i = 0; i < MAX_BOMB_COUNT; ++i)
        {
            dx = float(irand(-area_dx / 2, area_dx / 2));
            dy = float(irand(-area_dy / 2, area_dy / 2));
            DoSpawnCreature(NPC_FIRE_BOMB, dx, dy, 0, 0, TEMPSUMMON_TIMED_DESPAWN, 15000);
        }
    }

    void Boom()
    {
        summons.DoForAllSummons([&](WorldObject* summon) {
            if (summon->GetEntry() == NPC_FIRE_BOMB)
            {
                if (Creature* bomb = summon->ToCreature())
                {
                    bomb->AI()->DoCastSelf(SPELL_FIRE_BOMB_DAMAGE, true);
                    bomb->RemoveAllAuras();
                }
            }
        });
    }

    void StartBombing()
    {
        Talk(SAY_FIRE_BOMBS);
        me->AttackStop();
        me->GetMotionMaster()->Clear();
        me->SetPosition(janalainPos);
        me->StopMovingOnCurrentPos();
        DoCastSelf(SPELL_FIRE_BOMB_CHANNEL);

        FireWall();
        SpawnBombs();
        _isBombing = true;

        DoCastSelf(SPELL_TELE_TO_CENTER);
        DoCastAOE(SPELL_SUMMON_PLAYERS_DUMMY, true);

        //DoCast(Temp, SPELL_SUMMON_PLAYERS, true) // core bug, spell does not work if too far
        ThrowBombs();

        scheduler.Schedule(11s, [this](TaskContext)
        {
            Boom();
            _isBombing = false;

            me->RemoveAurasDueToSpell(SPELL_FIRE_BOMB_CHANNEL);
        });
    }

    void ThrowBombs()
    {
        std::chrono::milliseconds bombTimer = 100ms;

        summons.DoForAllSummons([this, &bombTimer](WorldObject* summon) {
            if (summon->GetEntry() == NPC_FIRE_BOMB)
            {
                if (Creature* bomb = summon->ToCreature())
                {
                    bomb->m_Events.AddEventAtOffset([this, bomb] {
                        bomb->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
                        DoCast(bomb, SPELL_FIRE_BOMB_THROW, true);
                        bomb->SetUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
                    }, bombTimer);
                }

                bombTimer += 100ms;
            }
        });
    }

    bool CheckEvadeIfOutOfCombatArea() const override
    {
        return me->GetPositionZ() <= 12.0f;
    }
private:
    bool _isBombing;
    bool _isFlameBreathing;
    bool _sideHatched[2];
};

struct npc_janalai_hatcher : public ScriptedAI
{
    npc_janalai_hatcher(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        ScriptedAI::Reset();
        scheduler.CancelAll();
        _side = (me->GetPositionY() < 1150.0f) ? 1 : 0;
        _isHatching = false;
        _sidesCleared = 0;
        me->GetMotionMaster()->Clear();
        MoveToWaypoint(0);
    }

    void MovementInform(uint32 type, uint32 pointId) override
    {
        // 只处理路径点移动；孵化过程中不再移动
        if (type != POINT_MOTION_TYPE || _isHatching)
            return;

        // 用目标点编号推进路径，避免重复的移动回调导致跳点（跳点会让孵化者在半路就开始孵化）
        if (pointId + 1 < hatcherWaypointCount)
            MoveToWaypoint(pointId + 1);
        else
            StartHatching();
    }

    void MoveToWaypoint(uint32 pointId)
    {
        scheduler.Schedule(100ms, [this, pointId](TaskContext)
        {
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MovePoint(pointId, hatcherway[_side][pointId]);
        });
    }

    // 只有真正走到蛋巢（路径最后一点）才会开始孵化；重复或过期的移动回调不会让孵化在半路开始
    void StartHatching()
    {
        if (_isHatching)
            return;

        Position const& nestPos = hatcherway[_side][hatcherWaypointCount - 1];
        if (me->GetDistance2d(nestPos.GetPositionX(), nestPos.GetPositionY()) > hatcherHatchRange)
        {
            // 还没走到蛋巢，继续走过去
            MoveToWaypoint(hatcherWaypointCount - 1);
            return;
        }

        _isHatching = true;

        scheduler.Schedule(1500ms, [this](TaskContext context)
        {
            if (HatchNearbyEgg())
                context.Repeat(5s);
            else
                SwitchToOtherNest();
        });
    }

    // 孵化自己身边最近的一颗蛋：直接让蛋施放召唤雏龙的法术
    //（与数据库中蛋的 SmartAI 行为一致：召唤雏龙并由法术效果摧毁蛋本身）
    bool HatchNearbyEgg()
    {
        Creature* egg = me->FindNearestCreature(NPC_EGG, hatcherHatchRange);
        if (!egg)
            return false;

        egg->CastSpell(egg, SPELL_SUMMON_HATCHLING, true);
        return true;
    }

    // 本侧蛋巢已孵完：通报 Boss 后换到另一侧继续；两侧都孵完就离场（避免两只孵化者来回跑）
    void SwitchToOtherNest()
    {
        if (WorldObject* summoner = GetSummoner())
            if (Creature* janalai = summoner->ToCreature())
                janalai->AI()->SetData(DATA_ALL_EGGS_HATCHED, _side);

        if (++_sidesCleared >= 2)
        {
            me->DespawnOrUnsummon(3s);
            return;
        }

        _side = _side ? 0 : 1;
        _isHatching = false;
        MoveToWaypoint(hatcherWaypointCount - 2);
    }

    void UpdateAI(uint32 diff) override
    {
        scheduler.Update(diff);
    }

    void JustEngagedWith(Unit* /*who*/) override { }
    void AttackStart(Unit* /*who*/) override { }
    void MoveInLineOfSight(Unit* /*who*/) override { }

private:
    uint8 _side;
    uint8 _sidesCleared;
    bool _isHatching;
};

class spell_summon_all_players_dummy: public SpellScript
{
    PrepareSpellScript(spell_summon_all_players_dummy);

    bool Validate(SpellInfo const* /*spell*/) override
    {
        return ValidateSpellInfo({ SPELL_SUMMON_PLAYERS });
    }

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        Position pos = GetCaster()->GetPosition();
        targets.remove_if([&, pos](WorldObject* target) -> bool
        {
            return target->IsWithinBox(pos, 22.0f, 28.0f, 28.0f);
        });
    }

    void OnHit(SpellEffIndex /*effIndex*/)
    {
        GetCaster()->CastSpell(GetHitUnit(), SPELL_SUMMON_PLAYERS, true);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_summon_all_players_dummy::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnEffectHitTarget += SpellEffectFn(spell_summon_all_players_dummy::OnHit, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

void AddSC_boss_janalai()
{
    RegisterZulAmanCreatureAI(boss_janalai);
    RegisterZulAmanCreatureAI(npc_janalai_hatcher);
    RegisterSpellScript(spell_summon_all_players_dummy);
}
