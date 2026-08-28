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

#include "botmgr.h"
#include "CreatureScript.h"
#include "GameTime.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "karazhan.h"

enum Emotes
{
    EMOTE_PHASE_BANISH      = 0,
    EMOTE_PHASE_PORTAL      = 1
};

enum Spells
{
    SPELL_NETHERBURN_AURA   = 30522,
    SPELL_VOIDZONE          = 37063,
    SPELL_NETHER_INFUSION   = 38688,
    SPELL_NETHERBREATH      = 38523,
    SPELL_BANISH_VISUAL     = 39833,
    SPELL_BANISH_ROOT       = 42716,
    SPELL_EMPOWERMENT       = 38549,
    SPELL_NETHERSPITE_ROAR  = 38684
};

enum Portals
{
    RED_PORTAL              = 0, // Perseverence
    GREEN_PORTAL            = 1, // Serenity
    BLUE_PORTAL             = 2  // Dominance
};

enum Groups
{
    PORTAL_PHASE            = 0,
    BANISH_PHASE            = 1
};

const float PortalCoord[3][3] =
{
    {-11195.353516f, -1613.237183f, 278.237258f}, // Left side
    {-11137.846680f, -1685.607422f, 278.239258f}, // Right side
    {-11094.493164f, -1591.969238f, 279.949188f}  // Back side
};

const uint32 PortalID[3] =     {17369, 17367, 17368};
const uint32 PortalVisual[3] = {30487, 30490, 30491};
const uint32 PortalBeam[3] =   {30465, 30464, 30463};
const uint32 PlayerBuff[3] =   {30421, 30422, 30423};
const uint32 NetherBuff[3] =   {30466, 30467, 30468};

struct boss_netherspite : public BossAI
{
    boss_netherspite(Creature* creature) : BossAI(creature, DATA_NETHERSPITE) {}

    bool IsBetween(WorldObject* u1, WorldObject* target, WorldObject* u2) // the in-line checker
    {
        if (!u1 || !u2 || !target)
            return false;

        float xn, yn, xp, yp, xh, yh;
        xn = u1->GetPositionX();
        yn = u1->GetPositionY();
        xp = u2->GetPositionX();
        yp = u2->GetPositionY();
        xh = target->GetPositionX();
        yh = target->GetPositionY();

        // check if target is between (not checking distance from the beam yet)
        if (dist(xn, yn, xh, yh) >= dist(xn, yn, xp, yp) || dist(xp, yp, xh, yh) >= dist(xn, yn, xp, yp))
            return false;
        // 检查目标到光束直线的垂直距离（连线判定宽度，单位：码）
        return (std::abs((xn - xp) * yh + (yp - yn) * xh - xn * yp + xp * yn) / dist(xn, yn, xp, yp) < 2.5f);
    }

    float dist(float xa, float ya, float xb, float yb) // auxiliary method for distance
    {
        return std::sqrt((xa - xb) * (xa - xb) + (ya - yb) * (ya - yb));
    }

    void Reset() override
    {
        BossAI::Reset();
        berserk = false;
        for (uint8 i = 0; i < 3; ++i)
            lastNetherBuffApplyTime[i] = 0; // 重置 BOSS 套 BUFF 计时器
        HandleDoors(true);
        DestroyPortals();
    }

    void SummonPortals()
    {
        uint8 r = rand() % 4;
        uint8 pos[3];
        pos[RED_PORTAL] = ((r % 2) ? (r > 1 ? 2 : 1) : 0);
        pos[GREEN_PORTAL] = ((r % 2) ? 0 : (r > 1 ? 2 : 1));
        pos[BLUE_PORTAL] = (r > 1 ? 1 : 2); // Blue Portal not on the left side (0)
        for (int i = 0; i < 3; ++i)
        {
            if (Creature* portal = me->SummonCreature(PortalID[i], PortalCoord[pos[i]][0], PortalCoord[pos[i]][1], PortalCoord[pos[i]][2], 0, TEMPSUMMON_TIMED_DESPAWN, 60000))
            {
                PortalGUID[i] = portal->GetGUID();
                portal->AddAura(PortalVisual[i], portal);
            }
        }
    }

    void UpdatePortals() // Here we handle the beams' behavior
    {
        for (int j = 0; j < 3; ++j) // j = color
        {
            if (Creature* portal = ObjectAccessor::GetCreature(*me, PortalGUID[j]))
            {
                // the one who's been cast upon before
                Unit* current = ObjectAccessor::GetUnit(*portal, BeamTarget[j]);
                // temporary store for the best suitable beam reciever
                Unit* target = me;

                if (Map* map = me->GetMap())
                {
                    Map::PlayerList const& players = map->GetPlayers();

                    // get the best suitable target
                    for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                    {
                        Player* p = i->GetSource();
                        if (p && p->IsAlive() // alive
                            && (!target || target->GetDistance2d(portal) > p->GetDistance2d(portal)) // closer than current best
                            && IsBetween(me, p, portal)) // on the beam
                            target = p;

                        // npcbot: 同样遍历该玩家拥有的 NPCbot，选择更合适的目标
                        if (p && p->HaveBot())
                        {
                            BotMap const* bmap = p->GetBotMgr()->GetBotMap();
                            for (BotMap::const_iterator citr = bmap->begin(); citr != bmap->end(); ++citr)
                            {
                                Creature* bot = citr->second;
                                if (bot && bot->IsAlive()
                                    && (!target || target->GetDistance2d(portal) > bot->GetDistance2d(portal))
                                    && IsBetween(me, bot, portal))
                                    target = bot;
                            }
                        }
                    }
                }
                // buff the target
                if (target->IsNPlayer())
                {
                    target->AddAura(PlayerBuff[j], target);
                }
                // BOSS 被连线：按 2 秒间隔给 BOSS 叠加/刷新 BUFF
                else if (GameTime::GetGameTimeMS().count() - lastNetherBuffApplyTime[j] >= 2000)
                {
                    target->AddAura(NetherBuff[j], target);
                    lastNetherBuffApplyTime[j] = GameTime::GetGameTimeMS().count();
                }
                // cast visual beam on the chosen target if switched
                // simple target switching isn't working -> using BeamerGUID to cast (workaround)
                if (!current || target != current)
                {
                    BeamTarget[j] = target->GetGUID();
                    // remove currently beaming portal
                    if (Creature* beamer = ObjectAccessor::GetCreature(*portal, BeamerGUID[j]))
                    {
                        beamer->CastSpell(target, PortalBeam[j], false);
                        beamer->DisappearAndDie();
                        BeamerGUID[j].Clear();
                    }
                    // create new one and start beaming on the target
                    if (Creature* beamer = portal->SummonCreature(PortalID[j], portal->GetPositionX(), portal->GetPositionY(), portal->GetPositionZ(), portal->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN, 60000))
                    {
                        beamer->CastSpell(target, PortalBeam[j], false);
                        BeamerGUID[j] = beamer->GetGUID();
                    }
                }
                // aggro target if Red Beam
                if (j == RED_PORTAL && me->GetVictim() != target && target->IsPlayer())
                {
                    me->GetThreatMgr().AddThreat(target, 100000.0f + DoGetThreat(me->GetVictim()));
                }
            }
        }
    }

    void SwitchToPortalPhase(bool aggro = false)
    {
        if (!aggro)
        {
            Talk(EMOTE_PHASE_PORTAL);
        }

        scheduler.CancelGroup(BANISH_PHASE);
        me->RemoveAurasDueToSpell(SPELL_BANISH_ROOT);
        me->RemoveAurasDueToSpell(SPELL_BANISH_VISUAL);
        SummonPortals();
        scheduler.Schedule(60s, [this](TaskContext /*context*/)
        {
            if (!me->IsNonMeleeSpellCast(false))
            {
                SwitchToBanishPhase();
                return;
            }
        }).Schedule(10s, PORTAL_PHASE, [this](TaskContext context)
        {
            UpdatePortals();
            context.Repeat(1s); // 光束刷新（目标选择/切换）间隔 1 秒
        }).Schedule(10s, PORTAL_PHASE, [this](TaskContext context)
        {
            DoCastSelf(SPELL_EMPOWERMENT);
            me->AddAura(SPELL_NETHERBURN_AURA, me);
            context.Repeat(90s);
        }).Schedule(15s, PORTAL_PHASE, [this](TaskContext context)
        {
            DoCastRandomTarget(SPELL_VOIDZONE, 0, 45.0f, true, true, false);
            context.Repeat(15s);
        });
    }

    void DestroyPortals()
    {
        for (int i = 0; i < 3; ++i)
        {
            if (Creature* portal = ObjectAccessor::GetCreature(*me, PortalGUID[i]))
            {
                portal->DisappearAndDie();
            }
            if (Creature* portal = ObjectAccessor::GetCreature(*me, BeamerGUID[i]))
            {
                portal->DisappearAndDie();
            }

            PortalGUID[i].Clear();
            BeamTarget[i].Clear();
        }
    }

    void SwitchToBanishPhase()
    {
        Talk(EMOTE_PHASE_BANISH);
        scheduler.CancelGroup(PORTAL_PHASE);
        me->RemoveAurasDueToSpell(SPELL_EMPOWERMENT);
        me->RemoveAurasDueToSpell(SPELL_NETHERBURN_AURA);
        DoCastSelf(SPELL_BANISH_VISUAL, true);
        DoCastSelf(SPELL_BANISH_ROOT, true);

        DestroyPortals();

        scheduler.Schedule(30s, [this](TaskContext)
        {
            SwitchToPortalPhase();
            DoResetThreatList();
            return;
        }).Schedule(10s, BANISH_PHASE, [this](TaskContext context)
        {
            DoCastRandomTarget(SPELL_NETHERBREATH, 0, 40.0f, true);
            context.Repeat(5s, 7s);
        });

        for (uint8 i = 0; i < 3; ++i)
        {
            me->RemoveAurasDueToSpell(NetherBuff[i]);
        }
    }

    void HandleDoors(bool open)
    {
        if (GameObject* door = ObjectAccessor::GetGameObject(*me, instance->GetGuidData(DATA_GO_MASSIVE_DOOR)))
        {
            door->SetGoState(open ? GO_STATE_ACTIVE : GO_STATE_READY);
        }
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);
        HandleDoors(false);
        SwitchToPortalPhase(true);
        DoZoneInCombat();
        scheduler.Schedule(9min, [this](TaskContext /*context*/)
        {
            if (!berserk)
            {
                DoCastSelf(SPELL_NETHER_INFUSION);
                DoCastAOE(SPELL_NETHERSPITE_ROAR);
                berserk = true;
            }
        });
    }

    void JustDied(Unit* killer) override
    {
        BossAI::JustDied(killer);
        HandleDoors(true);
        DestroyPortals();
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        scheduler.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        DoMeleeAttackIfReady();
    }

private:
    bool berserk;
    uint32 lastNetherBuffApplyTime[3] = {0, 0, 0}; // 各色光束给 BOSS 套 BUFF 的上次时间（毫秒），用于 2 秒间隔节流
    ObjectGuid PortalGUID[3]; // guid's of portals
    ObjectGuid BeamerGUID[3]; // guid's of auxiliary beaming portals
    ObjectGuid BeamTarget[3]; // guid's of portals' current targets
};

class spell_nether_portal_perseverence : public AuraScript
{
    PrepareAuraScript(spell_nether_portal_perseverence);

    void HandleApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        const_cast<AuraEffect*>(aurEff)->SetAmount(aurEff->GetAmount() + 5000);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_nether_portal_perseverence::HandleApply, EFFECT_2, SPELL_AURA_MOD_INCREASE_HEALTH, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
    }
};

void AddSC_boss_netherspite()
{
    RegisterKarazhanCreatureAI(boss_netherspite);
    RegisterSpellScript(spell_nether_portal_perseverence);
}
