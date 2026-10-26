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

#include "GameTime.h"
#include "Group.h"
#include "InstanceMapScript.h"
#include "InstanceScript.h"
#include "MapMgr.h"
#include "Transport.h"
#include "WorldStateDefines.h"
//npcbot
#include "botmgr.h"
//end npcbot
#include "halls_of_reflection.h"

class UtherBatteredHiltEvent : public BasicEvent
{
public:
    UtherBatteredHiltEvent(Creature& owner, uint8 eventId) : _owner(owner), _eventId(eventId) { }

    bool Execute(uint64 /*eventTime*/, uint32 /*updateTime*/) override
    {
        switch (_eventId)
        {
            case 1:
                _owner.UpdatePosition(5300.53f, 1987.80f, 707.70f, 3.89f, true);
                _owner.StopMovingOnCurrentPos();
                _owner.GetMotionMaster()->Clear();
                _owner.SetVisible(true);
                _owner.NearTeleportTo(5300.53f, 1987.80f, 707.70f, 3.89f);
                _owner.m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(_owner, 2), 1s);
                break;
            case 2:
                _owner.AI()->Talk(SAY_BATTERED_HILT_HALT);
                break;
            case 3:
                _owner.CastSpell((Unit*)nullptr, SPELL_SUMMON_EVIL_QUEL, true);
                _owner.AI()->Talk(SAY_BATTERED_HILT_REALIZE);
                if (InstanceScript* inst = _owner.GetInstanceScript())
                    inst->SetData(DATA_BATTERED_HILT, 4);
                if (Creature* quel = _owner.FindNearestCreature(NPC_QUEL_DELAR, 50))
                    quel->AI()->Talk(EMOTE_QUEL_SPAWN);
                _owner.m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(_owner, 4), 3500ms);
                break;
            case 4:
                _owner.SetWalk(false);
                _owner.GetMotionMaster()->MovePoint(0, 5337.53f, 1981.21f, 709.32f);
                _owner.m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(_owner, 5), 6s);
                break;
            case 5:
                _owner.SetFacingTo(2.82f);
                _owner.SetStandState(UNIT_STAND_STATE_KNEEL);
                break;
            case 6:
                if (InstanceScript* inst = _owner.GetInstanceScript())
                    inst->SetData(DATA_BATTERED_HILT, 6);
                _owner.m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(_owner, 7), 2s);
                break;
            case 7:
                if (InstanceScript* inst = _owner.GetInstanceScript())
                    inst->SetData(DATA_BATTERED_HILT, 7);
                if (Creature* quel = _owner.FindNearestCreature(NPC_QUEL_DELAR, 50))
                    quel->AI()->Talk(EMOTE_QUEL_PREPARE);
                _owner.m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(_owner, 8), 4s);
                break;
            case 8:
                _owner.SetReactState(REACT_AGGRESSIVE);
                _owner.SetImmuneToAll(false);
                if (InstanceScript* inst = _owner.GetInstanceScript())
                    inst->SetData(DATA_BATTERED_HILT, 8);
                break;
            case 9:
                _owner.AI()->Talk(SAY_BATTERED_HILT_OUTRO1);
                _owner.m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(_owner, _eventId + 1), 11s);
                break;
            case 10:
                _owner.AI()->Talk(SAY_BATTERED_HILT_OUTRO2);
                _owner.m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(_owner, _eventId + 1), 7500ms);
                break;
            case 11:
                _owner.AI()->Talk(SAY_BATTERED_HILT_OUTRO3);
                _owner.m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(_owner, _eventId + 1), 8s);
                break;
            case 12:
                _owner.AI()->Talk(SAY_BATTERED_HILT_OUTRO4);
                _owner.m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(_owner, _eventId + 1), 5s);
                break;
            case 13:
                _owner.CastSpell((Unit*)nullptr, SPELL_UTHER_HOLY_LIGHT_VISUAL, true);
                _owner.m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(_owner, _eventId + 1), 3s);
                break;
            case 14:
            {
                Position homePos = _owner.GetHomePosition();
                _owner.SetReactState(REACT_PASSIVE);
                _owner.SetImmuneToAll(true);
                _owner.SetVisible(false);
                _owner.UpdatePosition(homePos.GetPositionX(), homePos.GetPositionY(), homePos.GetPositionZ(), homePos.GetOrientation(), true);
                _owner.StopMovingOnCurrentPos();
                _owner.GetMotionMaster()->Clear();
                if (InstanceScript* inst = _owner.GetInstanceScript())
                    inst->SetData(DATA_BATTERED_HILT, 9);
                break;
            }
        }
        return true;
    }

private:
    Creature& _owner;
    uint8 _eventId;
};

ObjectData const creatureData[] =
{
    { NPC_FALRIC,                      DATA_FALRIC                      },
    { NPC_MARWYN,                      DATA_MARWYN                      },
    { NPC_SYLVANAS_PART1,              NPC_SYLVANAS_PART1               },
    { NPC_DARK_RANGER_LORALEN,         NPC_DARK_RANGER_LORALEN          },
    { NPC_UTHER,                       NPC_UTHER                        },
    { NPC_LICH_KING_EVENT,             NPC_LICH_KING_EVENT              },
    { NPC_FROSTSWORN_GENERAL,          NPC_FROSTSWORN_GENERAL           },
    { NPC_LICH_KING_BOSS,              NPC_LICH_KING_BOSS               },
    { NPC_SYLVANAS_PART2,              NPC_SYLVANAS_PART2               },
    { NPC_ALTAR_BUNNY,                 NPC_ALTAR_BUNNY                  },
    { NPC_QUEL_DELAR,                  NPC_QUEL_DELAR                   },
    { NPC_HIGH_CAPTAIN_JUSTIN_BARLETT, DATA_SHIP_CAPTAIN                },
    { NPC_SKY_REAVER_KORM_BLACKSKAR,   DATA_SHIP_CAPTAIN                },
    { 0,                               0                                }
};

ObjectData const gameObjectData[] =
{
    { GO_FROSTMOURNE,        GO_FROSTMOURNE        },
    { GO_FROSTMOURNE_ALTAR,  GO_FROSTMOURNE_ALTAR  },
    { GO_FRONT_DOOR,         GO_FRONT_DOOR         },
    { GO_ARTHAS_DOOR,        GO_ARTHAS_DOOR        },
    { GO_CAVE_IN,            GO_CAVE_IN            },
    { GO_DOOR_BEFORE_THRONE, GO_DOOR_BEFORE_THRONE },
    { GO_DOOR_AFTER_THRONE,  GO_DOOR_AFTER_THRONE  },
    { GO_ICE_WALL,           GO_ICE_WALL           },
    { 0,                     0                     }
};

class instance_halls_of_reflection : public InstanceScript
{
public:
    instance_halls_of_reflection(Map* map) : InstanceScript(map)
    {
        SetHeaders(DataHeader);
        SetBossNumber(MAX_ENCOUNTER);
        SetPersistentDataCount(PERSISTENT_DATA_COUNT);
        LoadObjectData(creatureData, gameObjectData);
    }

    void Initialize() override
    {
        std::fill(std::begin(_trashActive), std::end(_trashActive), false);
        std::fill(std::begin(_trashGUID), std::end(_trashGUID), ObjectGuid::Empty);
        std::fill(std::begin(_trashSpawnId), std::end(_trashSpawnId), ObjectGuid::LowType(0));
        _trashCounter = 0;
        std::memset(&_chosenComposition, 0, sizeof(_chosenComposition));
        _waveNumber = 0;
        _nextWaveTimer = 0;
        _playerCheckTimer = 5000;
        _waveResumeTimer = 0;
        _waveResumeStep = 0;
        _falricPhaseComplete = false;
        _remainingTrashKills = 0;
        _waveTrashCount = 0;
        _waveActivateCursor = 0;
        std::fill(std::begin(_waveTrashGUID), std::end(_waveTrashGUID), ObjectGuid::Empty);
        std::fill(std::begin(_trashWaveNumber), std::end(_trashWaveNumber), uint8(0));
        _isLichKingFightActive = false;
        _batteredHiltStatus = 0;
        _outroTimer = 0;
        _outroStep = 0;
        _transport = nullptr;
    }

    bool IsEncounterInProgress() const override
    {
        return (instance->HavePlayers() && _waveNumber) || _isLichKingFightActive;
    }

    void OnCreatureCreate(Creature* creature) override
    {
        InstanceScript::OnCreatureCreate(creature);

        switch (creature->GetEntry())
        {
            case NPC_SYLVANAS_PART1:
                creature->SetVisible(false);
                creature->SetSpeed(MOVE_RUN, 1.1);
                if (GetTeamIdInInstance() == TEAM_ALLIANCE)
                    creature->UpdateEntry(NPC_JAINA_PART1);
                creature->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                creature->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                break;
            case NPC_DARK_RANGER_LORALEN:
                creature->SetVisible(false);
                if (GetTeamIdInInstance() == TEAM_ALLIANCE)
                    creature->UpdateEntry(NPC_ARCHMAGE_KORELN);
                break;
            case NPC_UTHER:
                creature->SetVisible(false);
                creature->SetReactState(REACT_PASSIVE);
                break;
            case NPC_LICH_KING_EVENT:
                creature->SetVisible(false);
                creature->SetReactState(REACT_PASSIVE);
                creature->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                break;
            case NPC_FALRIC:
                creature->SetVisible(false);
                creature->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                break;
            case NPC_MARWYN:
                creature->SetVisible(false);
                creature->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                break;
            case NPC_WAVE_MERCENARY:
            case NPC_WAVE_FOOTMAN:
            case NPC_WAVE_RIFLEMAN:
            case NPC_WAVE_PRIEST:
            case NPC_WAVE_MAGE:
                if (GetBossState(DATA_MARWYN) != DONE && !creature->IsAlive())
                    creature->Respawn();
                creature->SetVisible(false);
                // 登记表按刷怪点维护：核心重建出来的新对象（GUID 会变）在这里重新绑定槽位
                RegisterTrash(creature);
                break;
            case NPC_FROSTSWORN_GENERAL:
                if (GetBossState(DATA_MARWYN) != DONE)
                {
                    creature->SetVisible(false);
                    creature->SetReactState(REACT_PASSIVE);
                }
                break;
            case NPC_SPIRITUAL_REFLECTION:
                for (uint8 i = 0; i < MAX_SPIRITUAL_REFLECTIONS; ++i)
                    if (!_spiritualReflectionGUID[i])
                    {
                        _spiritualReflectionGUID[i] = creature->GetGUID();
                        break;
                    }
                creature->SetVisible(false);
                break;
            case NPC_LICH_KING_BOSS:
                if (!GetPersistentData(PERSISTENT_DATA_FROSTSWORN_GENERAL))
                    creature->SetVisible(false);
                if (!GetPersistentData(PERSISTENT_DATA_LK_INTRO))
                {
                    creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_ATTACK2HTIGHT);
                    creature->CastSpell(creature, SPELL_SOUL_REAPER, true);
                }
                else if (GetBossState(DATA_LICH_KING) != DONE)
                    creature->AddAura(GetTeamIdInInstance() == TEAM_ALLIANCE ? SPELL_JAINA_ICE_PRISON : SPELL_SYLVANAS_DARK_BINDING, creature);
                else
                    creature->SetVisible(false);

                creature->SetHealth((creature->GetMaxHealth() * 3) / 4);
                creature->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                break;
            case NPC_SYLVANAS_PART2:
                if (!creature->IsAlive())
                    creature->Respawn();
                creature->SetWalk(false);
                creature->SetSheath(SHEATH_STATE_MELEE);
                if (GetTeamIdInInstance() == TEAM_ALLIANCE)
                    creature->UpdateEntry(NPC_JAINA_PART2);
                creature->SetHealth(creature->GetMaxHealth() / 20);
                if (!GetPersistentData(PERSISTENT_DATA_FROSTSWORN_GENERAL))
                    creature->SetVisible(false);
                if (!GetPersistentData(PERSISTENT_DATA_LK_INTRO))
                {
                    creature->SetSheath(SHEATH_STATE_MELEE);
                    creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, GetTeamIdInInstance() == TEAM_ALLIANCE ? EMOTE_ONESHOT_ATTACK2HTIGHT : EMOTE_ONESHOT_ATTACK1H);
                    creature->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP | UNIT_NPC_FLAG_QUESTGIVER);
                    creature->CastSpell(creature, GetTeamIdInInstance() == TEAM_ALLIANCE ? SPELL_JAINA_ICE_BARRIER : SPELL_SYLVANAS_CLOAK_OF_DARKNESS, true);
                }
                else if (GetBossState(DATA_LICH_KING) != DONE)
                {
                    creature->RemoveNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);
                    creature->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                    creature->UpdatePosition(LeaderEscapePos, true);
                    creature->StopMovingOnCurrentPos();
                }
                else
                {
                    creature->UpdatePosition(PathWaypoints[PATH_WP_COUNT - 1], true);
                    creature->StopMovingOnCurrentPos();
                }
                creature->SetSheath(SHEATH_STATE_MELEE);
                creature->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                break;
            case NPC_ICE_WALL_TARGET:
                if (creature->GetPositionX() > 5525.0f)
                    _iceWallTargetGUID[0] = creature->GetGUID();
                else if (creature->GetPositionX() > 5475.0f)
                    _iceWallTargetGUID[1] = creature->GetGUID();
                else if (creature->GetPositionX() > 5400.0f)
                    _iceWallTargetGUID[2] = creature->GetGUID();
                else
                    _iceWallTargetGUID[3] = creature->GetGUID();
                break;
            case NPC_QUEL_DELAR:
                creature->SetReactState(REACT_PASSIVE);
                break;
        }
    }

    // 判断玩家所在队伍中是否还有存活的 BOT。
    // BOT 必须与当前副本在同一地图：未跟随进本、已被传送出去的 BOT 不能算“还有活着的”，
    // 否则副本内明明已经团灭，却因为副本外残留的 BOT 永远不肯重置波次
    bool HasAliveBotsInInstance(Player* player)
    {
        auto hasAliveBot = [this](Player* owner) -> bool
        {
            if (!owner || !owner->HaveBot())
                return false;

            for (auto const& it : *owner->GetBotMgr()->GetBotMap())
            {
                Creature* bot = it.second;
                if (bot && bot->IsInWorld() && bot->FindMap() == instance && bot->IsAlive())
                    return true;
            }
            return false;
        };

        if (hasAliveBot(player))
            return true;

        // 单人玩家没有队伍，上面的检查即可覆盖
        if (Group* group = player->GetGroup())
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!member || member == player || member->FindMap() != instance)
                    continue;
                if (hasAliveBot(member))
                    return true;
            }

        return false;
    }

    void OnPlayerLeave(Player* player) override
    {
        InstanceScript::OnPlayerLeave(player);

        // 最后一名玩家离开副本后波次无法继续推进（Update 会因副本内没有玩家而停止更新），
        // 而且灵魂被传出副本时团灭判定也永远等不到。无论该玩家与其 BOT 是否存活，
        // 都在这里复位波次，避免下次进入时计数接着涨却不再刷新小怪。
        // 注意：不能用 GetPlayersCountExceptGMs()，本项目的 npcbot 补丁会把 BOT 计入该值，
        // 单人带 BOT 离开时人数永远大于 1，波次将永远不会复位，所以这里只统计真实玩家
        uint32 players = 0;
        for (auto const& ref : instance->GetPlayers())
        {
            Player* member = ref.GetSource();
            if (member && !member->IsGameMaster())
                ++players;
        }

        if (_waveNumber && players <= 1)
            HandleWaveWipe();
    }

    void OnGameObjectCreate(GameObject* go) override
    {
        switch (go->GetEntry())
        {
            case GO_FROSTMOURNE:
                HandleGameObject(ObjectGuid::Empty, false, go);
                if (GetPersistentData(PERSISTENT_DATA_INTRO))
                    go->SetPhaseMask(2, true);
                break;
            case GO_FRONT_DOOR:
                HandleGameObject(ObjectGuid::Empty, true, go);
                break;
            case GO_ARTHAS_DOOR:
                HandleGameObject(ObjectGuid::Empty, GetBossState(DATA_MARWYN) == DONE, go);
                break;
        }

        InstanceScript::OnGameObjectCreate(go);
    }

    bool SetBossState(uint32 type, EncounterState state) override
    {
        if (!InstanceScript::SetBossState(type, state))
            return false;

        switch (type)
        {
            case DATA_FALRIC:
                // 法瑞克被击杀即视为第一阶段完成：此后即使还没进入第 6 波就灭团，
                // 重来时也会跳过第 1-4 波和法瑞克本波，避免卡在第 5 波不出 Boss
                if (state == DONE)
                    _falricPhaseComplete = true;
                if (_waveNumber)
                {
                    if (state == NOT_STARTED)
                        HandleWaveWipe();
                    else if (state == DONE)
                        _nextWaveTimer = 60000;
                }
                break;
            case DATA_MARWYN:
                if (_waveNumber)
                {
                    if (state == NOT_STARTED)
                        HandleWaveWipe();
                    else if (state == DONE)
                    {
                        HandleGameObject(GO_FRONT_DOOR, true);
                        HandleGameObject(GO_ARTHAS_DOOR, true);
                        if (Creature* general = GetCreature(NPC_FROSTSWORN_GENERAL))
                        {
                            general->SetVisible(true);
                            general->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                            general->SetReactState(REACT_AGGRESSIVE);
                            general->HandleEmoteCommand(EMOTE_ONESHOT_EMERGE);
                        }
                        _waveNumber = 0;
                        _remainingTrashKills = 0;
                        _waveTrashCount = 0;
                        _nextWaveTimer = 0;
                        DoUpdateWorldState(WORLD_STATE_HALLS_OF_REFLECTION_WAVES_ENABLED, 0);

                        instance->DoForAllPlayers([](Player* player)
                        {
                            player->CastSpell(player, player->GetTeamId() == TEAM_ALLIANCE ? SPELL_HOR_START_QUEST_ALLY : SPELL_HOR_START_QUEST_HORDE, true);
                        });
                    }
                }
                break;
            case DATA_LICH_KING:
                if (state == DONE)
                {
                    if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                        leader->setActive(false);
                    if (Creature* lichKing = GetCreature(NPC_LICH_KING_BOSS))
                    {
                        lichKing->GetThreatMgr().ClearAllThreat();
                        lichKing->CombatStop(true);
                        lichKing->setActive(false);
                    }
                    _isLichKingFightActive = false;
                    _outroStep = 1;
                    _outroTimer = 0;
                }
                break;
        }

        return true;
    }

    void SetData(uint32 type, uint32 data) override
    {
        switch (type)
        {
            case DATA_INTRO:
                StorePersistentData(PERSISTENT_DATA_INTRO, 1);
                StartNextWave();
                break;
            case ACTION_SHOW_TRASH:
                RandomizeCompositionsAndShow();
                break;
            case DATA_FROSTSWORN_GENERAL:
                StorePersistentData(PERSISTENT_DATA_FROSTSWORN_GENERAL, 1);
                if (data == DONE)
                {
                    if (Creature* lichKing = GetCreature(NPC_LICH_KING_BOSS))
                        lichKing->SetVisible(true);
                    if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                        leader->SetVisible(true);
                }
                break;
            case ACTION_SPIRITUAL_REFLECTIONS_COPY:
            {
                uint8 reflectionIdx = 0;
                instance->DoForAllPlayers([&](Player* player)
                {
                    if (reflectionIdx >= MAX_SPIRITUAL_REFLECTIONS || !player->IsAlive() || player->IsGameMaster())
                        return;
                    Creature* reflection = instance->GetCreature(_spiritualReflectionGUID[reflectionIdx++]);
                    if (!reflection)
                        return;
                    if (!reflection->IsAlive())
                        reflection->Respawn();
                    reflection->SetCanFly(true);
                    reflection->SetDisableGravity(true);
                    reflection->SetVisible(true);

                    Item* weapon = player->GetWeaponForAttack(BASE_ATTACK);
                    reflection->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID, weapon ? weapon->GetEntry() : 0);
                    weapon = player->GetWeaponForAttack(OFF_ATTACK);
                    reflection->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, weapon ? weapon->GetEntry() : 0);
                    weapon = player->GetWeaponForAttack(RANGED_ATTACK);
                    reflection->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 2, weapon ? weapon->GetEntry() : 0);
                    player->CastSpell(reflection, SPELL_HOR_CLONE, true);
                    player->CastSpell(reflection, SPELL_HOR_CLONE_NAME, true);
                });
                break;
            }
            case ACTION_SPIRITUAL_REFLECTIONS_ACTIVATE:
            {
                Creature* general = GetCreature(NPC_FROSTSWORN_GENERAL);
                if (!general)
                    break;
                for (uint8 i = 0; i < MAX_SPIRITUAL_REFLECTIONS; ++i)
                    if (Creature* reflection = instance->GetCreature(_spiritualReflectionGUID[i]))
                        if (reflection->IsVisible())
                        {
                            reflection->SetInCombatWithZone();
                            reflection->SetCanFly(false);
                            reflection->SetDisableGravity(false);
                            reflection->GetMotionMaster()->MoveJump(general->GetPositionX(), general->GetPositionY(), general->GetPositionZ(), 20.0f, 10.0f);
                        }
                break;
            }
            case ACTION_SPIRITUAL_REFLECTIONS_HIDE:
                for (uint8 i = 0; i < MAX_SPIRITUAL_REFLECTIONS; ++i)
                    if (Creature* reflection = instance->GetCreature(_spiritualReflectionGUID[i]))
                        reflection->AI()->EnterEvadeMode();
                break;
            case DATA_LK_INTRO:
                StorePersistentData(PERSISTENT_DATA_LK_INTRO, 1);
                if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                    leader->AI()->DoAction(ACTION_START_INTRO);
                break;
            case ACTION_START_LK_FIGHT:
                SetBossState(DATA_LICH_KING, IN_PROGRESS);
                _isLichKingFightActive = true;
                DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_RETREATING_TIMED_EVENT);
                DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_RETREATING_TIMED_EVENT);
                break;
            case ACTION_STOP_LK_FIGHT:
                if (!_isLichKingFightActive)
                    break;
                if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                {
                    if (!leader->IsAlive())
                    {
                        leader->Respawn();
                        if (GetTeamIdInInstance() == TEAM_ALLIANCE)
                            leader->UpdateEntry(NPC_JAINA_PART2);
                    }
                    leader->GetThreatMgr().ClearAllThreat();
                    leader->CombatStop(true);
                    leader->InterruptNonMeleeSpells(true);
                    leader->GetMotionMaster()->Clear();
                    leader->GetMotionMaster()->MoveIdle();
                    leader->UpdatePosition(LeaderEscapePos, true);
                    leader->StopMovingOnCurrentPos();
                    leader->RemoveNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);
                    leader->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                    leader->SetHealth(leader->GetMaxHealth() / 20);
                    leader->AI()->Reset();
                    leader->setActive(false);
                    leader->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                    leader->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                }
                if (Creature* lichKing = GetCreature(NPC_LICH_KING_BOSS))
                {
                    lichKing->GetThreatMgr().ClearAllThreat();
                    lichKing->CombatStop(true);
                    lichKing->InterruptNonMeleeSpells(true);
                    lichKing->GetMotionMaster()->Clear();
                    lichKing->GetMotionMaster()->MoveIdle();
                    lichKing->UpdatePosition(lichKing->GetHomePosition(), true);
                    lichKing->StopMovingOnCurrentPos();
                    lichKing->RemoveAllAuras();
                    lichKing->AddAura(GetTeamIdInInstance() == TEAM_ALLIANCE ? SPELL_JAINA_ICE_PRISON : SPELL_SYLVANAS_DARK_BINDING, lichKing);
                    lichKing->AI()->Reset();
                    lichKing->setActive(false);
                    lichKing->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                    lichKing->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                    lichKing->SetSpeed(MOVE_RUN, lichKing->GetCreatureTemplate()->speed_run);
                }
                _isLichKingFightActive = false;
                SetBossState(DATA_LICH_KING, FAIL);
                _outroTimer = 0;
                _outroStep = 0;
                [[fallthrough]];
            case ACTION_DELETE_ICE_WALL:
                HandleGameObject(GO_ICE_WALL, true);
                break;
            case DATA_BATTERED_HILT:
            {
                if (GetPersistentData(PERSISTENT_DATA_BATTERED_HILT))
                    return;

                switch (data)
                {
                    case 1:
                        StorePersistentData(PERSISTENT_DATA_BATTERED_HILT, 1);
                        break;
                    case 2:
                        if (_batteredHiltStatus)
                            break;
                        _batteredHiltStatus |= BHSF_STARTED;
                        if (Creature* bunny = GetCreature(NPC_ALTAR_BUNNY))
                            bunny->CastSpell(bunny, SPELL_FROSTMOURNE_ALTAR_GLOW, true);
                        if (Creature* uther = GetCreature(NPC_UTHER))
                            uther->m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(*uther, 1), 3s);
                        break;
                    case 3:
                        if (!(_batteredHiltStatus & BHSF_STARTED) || (_batteredHiltStatus & BHSF_THROWN))
                            break;
                        _batteredHiltStatus |= BHSF_THROWN;
                        if (Creature* uther = GetCreature(NPC_UTHER))
                            uther->m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(*uther, 3), 5500ms);
                        break;
                    case 4:
                        if (Creature* quelDelar = GetCreature(NPC_QUEL_DELAR))
                        {
                            quelDelar->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
                            quelDelar->SetSpeed(MOVE_RUN, 2.5f);
                        }
                        break;
                    case 5:
                        if (Creature* uther = GetCreature(NPC_UTHER))
                            uther->m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(*uther, 6), 3s);
                        break;
                    case 6:
                        if (Creature* quelDelar = GetCreature(NPC_QUEL_DELAR))
                        {
                            quelDelar->SetSpeed(MOVE_RUN, quelDelar->GetCreatureTemplate()->speed_run);
                            quelDelar->GetMotionMaster()->MoveLand(0, quelDelar->GetPositionX(), quelDelar->GetPositionY(), 707.70f, 7.0f);
                        }
                        break;
                    case 7:
                        if (Creature* quelDelar = GetCreature(NPC_QUEL_DELAR))
                        {
                            quelDelar->SetReactState(REACT_AGGRESSIVE);
                            quelDelar->SetImmuneToAll(false);
                            quelDelar->RemoveAurasDueToSpell(SPELL_QUEL_DELAR_HATRED);
                        }
                        break;
                    case 8:
                        if (Creature* quelDelar = GetCreature(NPC_QUEL_DELAR))
                            quelDelar->SetInCombatWithZone();
                        break;
                    case 9:
                        StorePersistentData(PERSISTENT_DATA_BATTERED_HILT, 1);
                        _batteredHiltStatus |= BHSF_FINISHED;
                        break;
                }
                return;
            }
        }
    }

    uint32 GetData(uint32 type) const override
    {
        switch (type)
        {
            case DATA_WAVE_NUMBER:
                return static_cast<uint32>(_waveNumber);
            case DATA_BATTERED_HILT:
                return _batteredHiltStatus;
        }

        return 0;
    }

    ObjectGuid GetGuidData(uint32 type) const override
    {
        switch (type)
        {
            case NPC_ICE_WALL_TARGET:
            case NPC_ICE_WALL_TARGET + 1:
            case NPC_ICE_WALL_TARGET + 2:
            case NPC_ICE_WALL_TARGET + 3:
                return _iceWallTargetGUID[type - NPC_ICE_WALL_TARGET];
            default:
                return GetObjectGuid(type);
        }
    }

    void OnUnitDeath(Unit* unit) override
    {
        // 只统计“本波激活”的小怪：旧波次残留的怪在下一波被杀时不能再减计数，
        // 否则本波的击杀数永远凑不齐，只能等 150 秒超时硬推波次
        if (_waveNumber && _waveNumber % 5 && _remainingTrashKills)
        {
            for (uint8 i = 0; i < _waveTrashCount; ++i)
                if (_waveTrashGUID[i] == unit->GetGUID())
                {
                    _waveTrashGUID[i].Clear();
                    if ((--_remainingTrashKills) == 0 && _nextWaveTimer > 5000)
                        _nextWaveTimer = 5000;
                    break;
                }
        }

        if (unit->GetEntry() == NPC_QUEL_DELAR)
            if (Creature* uther = GetCreature(NPC_UTHER))
            {
                uther->SetStandState(UNIT_STAND_STATE_STAND);
                uther->SetWalk(false);
                uther->GetMotionMaster()->MovePoint(0, 5313.92f, 1989.36f, 707.70f);
                uther->m_Events.AddEventAtOffset(new UtherBatteredHiltEvent(*uther, 9), 7s);
            }
    }

    // 重新生成 8 个波次的小怪组合（不改变当前波次进度）
    void GenerateCompositions()
    {
        uint8 r1 = urand(0, 1);
        uint8 r2 = urand(2, 3);
        for (uint8 i = 0; i < MAX_SPIRITUAL_REFLECTIONS; ++i)
        {
            _chosenComposition[0][i] = allowedCompositions[r1][i];
            _chosenComposition[1][i] = allowedCompositions[r1 == 0 ? 1 : 0][i];
            _chosenComposition[2][i] = allowedCompositions[r2][i];
            _chosenComposition[3][i] = allowedCompositions[r2 == 2 ? 3 : 2][i];
        }
        bool left[4] = {true, true, true, true};
        for (uint8 k = 4; k > 0; --k)
        {
            uint8 r = urand(0, k - 1);
            uint8 ur = 0;
            for (uint8 j = 0; j < 4; ++j)
            {
                if (!left[j])
                    continue;
                if (ur == r)
                {
                    left[j] = false;
                    for (uint8 i = 0; i < MAX_SPIRITUAL_REFLECTIONS; ++i)
                        _chosenComposition[8 - k][i] = allowedCompositions[j + 4][i];
                    break;
                }
                ++ur;
            }
        }
    }

    // 波次小怪按刷怪点（spawnId）登记。默认的非兼容重生模式下，Creature::Respawn
    // 会销毁对象、再由 Map::ProcessRespawns 重建，重建后 GUID 会变；如果只缓存 GUID，
    // 登记表就会随着重生逐只失效，最终表现为波次计数还在推进却一只小怪都刷不出来
    void RegisterTrash(Creature* creature)
    {
        ObjectGuid::LowType const spawnId = creature->GetSpawnId();

        uint8 index = NUM_OF_TRASH;
        if (spawnId)
            for (uint8 i = 0; i < _trashCounter; ++i)
                if (_trashSpawnId[i] == spawnId)
                {
                    index = i;
                    break;
                }

        if (index == NUM_OF_TRASH)
        {
            if (_trashCounter >= NUM_OF_TRASH)
                return;

            index = _trashCounter++;
            _trashSpawnId[index] = spawnId;
            _trashWaveNumber[index] = 0;
            _trashActive[index] = false;
        }
        else
        {
            // 同一刷怪点的对象被重建：本波击杀统计里的旧 GUID 一并换成新的，
            // 否则这一波永远凑不齐击杀数，只能等 150 秒超时硬推下一波
            ObjectGuid const oldGuid = _trashGUID[index];
            if (oldGuid)
                for (uint8 i = 0; i < _waveTrashCount; ++i)
                    if (_waveTrashGUID[i] == oldGuid)
                        _waveTrashGUID[i] = creature->GetGUID();
        }

        _trashGUID[index] = creature->GetGUID();
    }

    // 回收已死亡的旧波次槽位：这些怪已经无法参战，继续占着槽位会逐波耗尽可用小怪，
    // 最终表现为波次计数还在涨却一只小怪都刷不出来
    void RecoverDeadTrash()
    {
        for (uint8 i = 0; i < NUM_OF_TRASH; ++i)
        {
            // 0xFF 表示该位置是“跳过的老一波”被主动击杀的，不能复活复用
            if (!_trashActive[i] || _trashWaveNumber[i] == _waveNumber || _trashWaveNumber[i] == 0xFF)
                continue;
            Creature* c = instance->GetCreature(_trashGUID[i]);
            if (c && !c->IsAlive())
                _trashActive[i] = false;
        }
    }

    // 查找一只尚未被波次系统占用的小怪，返回在 _trashGUID 中的下标，找不到返回 -1
    int8 FindFreeTrash(uint32 entry)
    {
        int8 deadFallback = -1;     // entry 匹配但已经死亡
        int8 aliveFallback = -1;    // entry 不匹配但还活着
        int8 pendingFallback = -1;  // 对象已被回收，需要交给核心重新生成

        for (uint8 i = 0; i < NUM_OF_TRASH; ++i)
        {
            if (_trashActive[i])
                continue;

            Creature* c = instance->GetCreature(_trashGUID[i]);
            if (!c)
            {
                // 对象被回收（例如尸体腐烂后重生时间为 24 小时），但刷怪点还在，
                // 激活时会按刷怪点把重生时间拉到当前，让核心重新生成
                if (pendingFallback < 0 && _trashSpawnId[i])
                    pendingFallback = int8(i);
                continue;
            }

            bool const alive = c->IsAlive();
            if (c->GetEntry() == entry)
            {
                // 优先选活着的小怪；尸体只在没有别的选择时才启用（激活时会复活）
                if (alive)
                    return int8(i);
                if (deadFallback < 0)
                    deadFallback = int8(i);
            }
            else if (alive && aliveFallback < 0)
                aliveFallback = int8(i);
        }

        // 正常情况下每个波次的组合与实际刷怪数量严格匹配；匹配不到时复用任意一只
        // 未占用的怪，避免整波激活失败、波次计数空转
        if (deadFallback >= 0)
            return deadFallback;
        return aliveFallback >= 0 ? aliveFallback : pendingFallback;
    }

    // 当前波次应刷出的小怪数量；Boss 波（5、10）返回 0
    uint8 GetWaveTrashNum() const
    {
        if (!_waveNumber || !(_waveNumber % 5))
            return 0;
        if (_waveNumber <= 2)
            return 3;
        if (_waveNumber <= 4)
            return 4;
        return 5;
    }

    // 当前波次在 _chosenComposition 中的组合下标
    uint8 GetWaveCompositionIndex() const
    {
        return _waveNumber - (_waveNumber > 5 ? 2 : 1);
    }

    // 激活当前波次中尚未刷出的小怪，_waveActivateCursor 记录补刷进度；返回本次激活数量
    uint8 ActivateWaveTrash()
    {
        uint8 const numToActivate = GetWaveTrashNum();
        if (!numToActivate)
            return 0;

        uint8 const compositionIndex = GetWaveCompositionIndex();
        // 组合在灭团时会被清空，如果恢复流程没有重新生成（例如重新进入副本），
        // 这里补生成一次，否则整波都选不到小怪，只会空转波次计数
        if (!_chosenComposition[compositionIndex][0])
            GenerateCompositions();

        RecoverDeadTrash();

        uint8 activated = 0;
        while (_waveActivateCursor < numToActivate)
        {
            // 尸体同样会被复活后重新激活，只有全部槽位都被占用才会选不到
            int8 const index = FindFreeTrash(_chosenComposition[compositionIndex][_waveActivateCursor]);
            // 暂时没有可用槽位时保留游标，稍后原地补刷，避免漏刷的小怪被波次直接跳过
            if (index < 0 || !ActivateTrash(uint8(index)))
                break;
            ++activated;
            ++_waveActivateCursor;
        }
        return activated;
    }

    // 补刷当前波次漏掉的小怪。不能推进波次，否则会跳波并与下一波小怪混在一起
    void ResumeWaveTrash()
    {
        uint8 const numToActivate = GetWaveTrashNum();
        _remainingTrashKills += ActivateWaveTrash();
        _nextWaveTimer = (_waveActivateCursor < numToActivate) ? 5000 : 150000;
    }

    // 让指定槽位的小怪真正出现在场上并进入战斗
    void SpawnTrash(uint8 index)
    {
        Creature* c = instance->GetCreature(_trashGUID[index]);
        if (!c)
            return;

        // 灭团或超时后小怪的存活与可见状态可能已经错乱（例如还处于隐藏状态），
        // 这里强制显示并解除不可选中，否则整波会“选中了怪但不刷新”
        c->RemoveUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
        c->SetImmuneToAll(false);
        c->SetVisible(true);

        // 击杀统计只认这只小怪当前的 GUID（对象重建后 GUID 会变）
        if (_waveTrashCount < 5)
        {
            bool tracked = false;
            for (uint8 i = 0; i < _waveTrashCount; ++i)
                if (_waveTrashGUID[i] == c->GetGUID())
                {
                    tracked = true;
                    break;
                }
            if (!tracked)
                _waveTrashGUID[_waveTrashCount++] = c->GetGUID();
        }

        c->AI()->DoAction(1);
    }

    // 占用一只小怪作为当前波次的战斗力
    bool ActivateTrash(uint8 index)
    {
        if (index >= NUM_OF_TRASH || _trashActive[index])
            return false;

        Creature* c = instance->GetCreature(_trashGUID[index]);

        _trashActive[index] = true;
        _trashWaveNumber[index] = _waveNumber;

        if (c && c->IsAlive())
        {
            SpawnTrash(index);
            return true;
        }

        // 兼容重生模式下 Respawn 是原地复活，立刻恢复上场状态；
        // 非兼容模式下（默认）对象会被销毁并交给核心重建，
        // 重建出来的新对象由 ReconcileWaveTrash 恢复上场状态
        if (c)
        {
            c->Respawn(true);
            if (c->IsAlive())
                SpawnTrash(index);
            return true;
        }

        // 对象已经被回收：把刷怪点的重生时间拉到当前，让核心重新生成一只
        if (_trashSpawnId[index])
        {
            time_t now = GameTime::GetGameTime().count();
            instance->SaveCreatureRespawnTime(_trashSpawnId[index], now);
            return true;
        }

        _trashActive[index] = false;
        _trashWaveNumber[index] = 0;
        return false;
    }

    // 核对当前波次已上场的小怪：对象被核心重建后（GUID 变化、带着隐藏和不可选中标记）
    // 需要重新上场，否则会出现“波次在打却看不到怪”的情况
    void ReconcileWaveTrash()
    {
        if (!_waveNumber || !(_waveNumber % 5))
            return;

        for (uint8 i = 0; i < NUM_OF_TRASH; ++i)
        {
            if (!_trashActive[i] || _trashWaveNumber[i] != _waveNumber)
                continue;

            Creature* c = instance->GetCreature(_trashGUID[i]);
            if (!c || !c->IsAlive() || c->IsVisible())
                continue;

            SpawnTrash(i);
        }
    }

    // 把本波尚未消耗的小怪以灵魂形态显示出来
    void ShowAllTrash()
    {
        for (uint8 i = 0; i < NUM_OF_TRASH; ++i)
            if (!_trashActive[i])
                if (Creature* c = instance->GetCreature(_trashGUID[i]))
                {
                    if (!c->IsAlive())
                        c->Respawn(true);
                    c->SetVisible(true);
                    c->CastSpell(c, SPELL_WELL_OF_SOULS_VISUAL, false);
                }
    }

    void RandomizeCompositionsAndShow()
    {
        GenerateCompositions();
        RecoverDeadTrash();

        // 副本重新加载后运行时标记会被清零，这里以持久化的 Boss 状态为准，
        // 否则老一已经死了还会重刷 1-4 波的小怪
        if (!_falricPhaseComplete && GetBossState(DATA_FALRIC) == DONE)
            _falricPhaseComplete = true;

        if (_falricPhaseComplete && _waveNumber < 4)
        {
            // 这些被“跳过消耗”的小怪同样会走 Unit::Kill，不能让它们误减当前波次的击杀计数
            _remainingTrashKills = 0;
            _waveTrashCount = 0;
            _waveActivateCursor = 0;

            for (; _waveNumber < 4; ++_waveNumber)
            {
                uint8 numToActivate = (_waveNumber <= 1) ? 3 : 4;

                for (uint8 i = 0; i < numToActivate; ++i)
                {
                    int8 index = FindFreeTrash(_chosenComposition[_waveNumber][i]);
                    if (index < 0)
                        continue;

                    Creature* c = instance->GetCreature(_trashGUID[index]);
                    if (!c)
                        continue;

                    // 标记为“跳过阶段已消耗”，不再参与后续死亡槽位回收
                    _trashActive[index] = true;
                    _trashWaveNumber[index] = 0xFF;
                    // 已经是尸体的无需再消耗一次
                    if (c->IsAlive())
                        Unit::Kill(c, c);
                }
            }
            _waveNumber = 5;
        }

        ShowAllTrash();
    }

    void StartNextWave()
    {
        if (_waveNumber >= 10)
            return;

        ++_waveNumber;
        // 每一波都从头开始刷怪，_waveActivateCursor 表示本波已处理到组合中的第几只
        _waveActivateCursor = 0;
        if (_waveNumber >= 6)
            _falricPhaseComplete = true;

        DoUpdateWorldState(WORLD_STATE_HALLS_OF_REFLECTION_WAVES_ENABLED, 1);
        DoUpdateWorldState(WORLD_STATE_HALLS_OF_REFLECTION_WAVE_COUNT, _waveNumber);
        HandleGameObject(GO_FRONT_DOOR, false);

        for (uint8 i = 0; i < NUM_OF_TRASH; ++i)
            if (_trashActive[i])
                if (Creature* c = instance->GetCreature(_trashGUID[i]))
                    if (c->IsAlive() && !c->IsInCombat())
                        c->AI()->DoAction(1);

        if (_waveNumber == 5 || _waveNumber == 10)
        {
            _nextWaveTimer = 0;
            // Boss 波不再需要统计小怪击杀
            _remainingTrashKills = 0;
            _waveTrashCount = 0;
            if (_waveNumber == 5)
            {
                // 法瑞克已阵亡或对象已被回收时不能停在这里，否则永远不出下一波（Boss），
                // 这里直接推进到第 6 波
                Creature* falric = GetCreature(DATA_FALRIC);
                if (falric && falric->IsAlive())
                    falric->AI()->DoAction(1);
                else
                    _nextWaveTimer = 1;
            }
            else
            {
                if (Creature* marwyn = GetCreature(DATA_MARWYN))
                    if (marwyn->IsAlive())
                        marwyn->AI()->DoAction(1);
            }
        }
        else
        {
            uint8 const numToActivate = GetWaveTrashNum();

            _waveTrashCount = 0;
            // 击杀计数按本波重新开始：上一波超时硬推时残留的击杀数不能带入本波，
            // 否则本波永远凑不齐，只能每波都等 150 秒超时
            _remainingTrashKills = ActivateWaveTrash();

            if (!_remainingTrashKills)
            {
                // 一只都没刷出来说明状态异常：保持当前波次并在 5 秒后原地重试，
                // 避免“波次计数一直在涨但永远不出小怪”
                _nextWaveTimer = 5000;
                return;
            }

            // 还有小怪没刷出来时 5 秒后继续补刷，补齐后进入 150 秒击杀等待
            _nextWaveTimer = (_waveActivateCursor < numToActivate) ? 5000 : 150000;
        }
    }

    void HandleWaveWipe()
    {
        if (!_waveNumber)
            return;

        // 波次状态是从运行时变量重建的（例如再次进入副本后 Initialize 会把标记清零），
        // 这里以已持久化的 Boss 状态为准：法瑞克已死就说明第一阶段已完成
        if (GetBossState(DATA_FALRIC) == DONE)
            _falricPhaseComplete = true;

        DoUpdateWorldState(WORLD_STATE_HALLS_OF_REFLECTION_WAVES_ENABLED, 0);
        DoUpdateWorldState(WORLD_STATE_HALLS_OF_REFLECTION_WAVE_COUNT, 0);
        HandleGameObject(GO_FRONT_DOOR, true);

        _waveNumber = 0;
        _nextWaveTimer = 0;
        std::memset(&_chosenComposition, 0, sizeof(_chosenComposition));
        std::fill(std::begin(_trashWaveNumber), std::end(_trashWaveNumber), uint8(0));

        // 所有小怪一律复位（不只本波激活的）：灭团/超时后小怪的存活状态可能与
        // _trashActive 不同步，只复活 active 的怪会留下尸体，下一波选中它时不会刷新
        for (uint8 i = 0; i < NUM_OF_TRASH; ++i)
            if (Creature* c = instance->GetCreature(_trashGUID[i]))
            {
                c->GetThreatMgr().ClearAllThreat();
                c->CombatStop(true);
                c->InterruptNonMeleeSpells(true);
                c->SetUnitFlag(UNIT_FLAG_NOT_SELECTABLE);
                c->SetImmuneToAll(true);
                if (!c->IsAlive())
                    c->Respawn(true);
                c->UpdatePosition(c->GetHomePosition(), true);
                c->StopMovingOnCurrentPos();
                c->SetVisible(false);
            }
        std::fill(std::begin(_trashActive), std::end(_trashActive), false);
        if (Creature* falric = GetCreature(DATA_FALRIC))
            falric->AI()->EnterEvadeMode();
        if (Creature* marwyn = GetCreature(DATA_MARWYN))
            marwyn->AI()->EnterEvadeMode();

        _waveResumeTimer = 5000;
        _waveResumeStep = 2;
        _remainingTrashKills = 0;
        _waveTrashCount = 0;
        _waveActivateCursor = 0;
    }

    void Update(uint32 diff) override
    {
        if (!instance->HavePlayers())
            return;

        if (_playerCheckTimer <= diff)
        {
            _playerCheckTimer = 5000;
            ReconcileWaveTrash();
            if ((GetPersistentData(PERSISTENT_DATA_INTRO)) && GetBossState(DATA_MARWYN) != DONE)
            {
                Map::PlayerList const& pl = instance->GetPlayers();
                if (_waveNumber || _nextWaveTimer)
                {
                    bool allDead = true;
                    bool outOfRange = false;
                    for (auto itr = pl.begin(); itr != pl.end(); ++itr)
                    {
                        Player* p = itr->GetSource();
                        if (!p || p->IsGameMaster())
                            continue;
                        if (p->IsAlive())
                            allDead = false;
                        else if (HasAliveBotsInInstance(p))
                        {
                            // 只有玩家与其队伍中同在本副本的 BOT 全部死亡才算团灭；
                            // 副本外的 BOT 不计入，BOT 仍存活时波次继续
                            allDead = false;
                        }
                        if (p->GetExactDist2d(&CenterPos) > MAX_DIST_FROM_CENTER_IN_COMBAT)
                        {
                            outOfRange = true;
                            break;
                        }
                    }
                    if (allDead || outOfRange)
                        HandleWaveWipe();
                }
                else if (!_waveResumeTimer)
                {
                    bool allInRange = (instance->GetPlayersCountExceptGMs() > 0);
                    for (auto itr = pl.begin(); itr != pl.end(); ++itr)
                    {
                        Player* p = itr->GetSource();
                        if (!p || p->IsGameMaster())
                            continue;
                        if (p->GetExactDist2d(&CenterPos) > MAX_DIST_FROM_CENTER_TO_START || !p->IsAlive())
                        {
                            allInRange = false;
                            break;
                        }
                    }
                    if (allInRange)
                    {
                        _waveResumeTimer = 1;
                        _waveResumeStep = 0;
                    }
                }
            }
        }
        else
            _playerCheckTimer -= diff;

        if (_nextWaveTimer)
        {
            if (_nextWaveTimer <= diff)
            {
                _nextWaveTimer = 0;
                // 本波还有小怪没刷出来时只能原地补刷，不能推进波次
                if (_waveNumber && _waveNumber % 5 && _waveActivateCursor < GetWaveTrashNum())
                    ResumeWaveTrash();
                else
                    StartNextWave();
            }
            else
                _nextWaveTimer -= diff;
        }

        if (_waveResumeTimer)
        {
            if (_waveResumeTimer <= diff)
            {
                switch (_waveResumeStep)
                {
                    case 0:
                        if (Creature* falric = GetCreature(DATA_FALRIC))
                        {
                            falric->UpdatePosition(5274.9f, 2039.2f, 709.319f, 5.4619f, true);
                            falric->StopMovingOnCurrentPos();
                            if (falric->IsAlive())
                            {
                                falric->SetVisible(true);
                                falric->GetMotionMaster()->MovePoint(0, FalricMovePos);
                                if (Aura* a = falric->AddAura(SPELL_SHADOWMOURNE_VISUAL, falric))
                                    a->SetDuration(8000);
                            }
                            // 已阵亡的法瑞克（灭团发生在第一阶段完成之后）不再显示
                            else
                                falric->SetVisible(false);
                        }
                        if (Creature* marwyn = GetCreature(DATA_MARWYN))
                        {
                            marwyn->UpdatePosition(5343.77f, 1973.86f, 709.319f, 2.35173f, true);
                            marwyn->StopMovingOnCurrentPos();
                            marwyn->SetVisible(true);
                            if (marwyn->IsAlive())
                            {
                                marwyn->GetMotionMaster()->MovePoint(0, MarwynMovePos);
                                if (Aura* a = marwyn->AddAura(SPELL_SHADOWMOURNE_VISUAL, marwyn))
                                    a->SetDuration(8000);
                            }
                            marwyn->AI()->Talk(EMOTE_MARWYN_INTRO_SPIRIT);
                        }
                        ++_waveResumeStep;
                        _waveResumeTimer = 7500;
                        break;
                    case 1:
                        if (Creature* falric = GetCreature(DATA_FALRIC))
                        {
                            if (falric->IsAlive())
                                falric->AI()->Talk(SAY_FALRIC_INTRO_2);
                            else if (Creature* marwyn = GetCreature(DATA_MARWYN))
                                marwyn->AI()->Talk(SAY_MARWYN_WIPE_AFTER_FALRIC);
                        }
                        SetData(ACTION_SHOW_TRASH, 1);
                        _waveResumeStep = 0;
                        _waveResumeTimer = 0;
                        _nextWaveTimer = 7000;
                        break;
                    default:
                        for (uint8 i = 0; i < NUM_OF_TRASH; ++i)
                            if (Creature* c = instance->GetCreature(_trashGUID[i]))
                                c->SetVisible(false);
                        if (Creature* falric = GetCreature(DATA_FALRIC))
                            falric->SetVisible(false);
                        if (Creature* marwyn = GetCreature(DATA_MARWYN))
                            marwyn->SetVisible(false);
                        _waveResumeStep = 0;
                        _waveResumeTimer = 0;
                        break;
                }
            }
            else
                _waveResumeTimer -= diff;
        }

        if (_outroStep)
        {
            if (_outroTimer <= diff)
                UpdateOutro();
            else
                _outroTimer -= diff;
        }
    }

private:
    void UpdateOutro()
    {
        switch (_outroStep)
        {
            case 1:
                if (Creature* lk = GetCreature(NPC_LICH_KING_BOSS))
                {
                    lk->UpdatePosition(PathWaypoints[PATH_WP_COUNT - 2], true);
                    lk->StopMovingOnCurrentPos();
                    lk->RemoveAllAuras();
                    lk->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_CUSTOM_SPELL_02);
                    if (!lk->IsVisible())
                        lk->SetVisible(true);
                    if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                    {
                        leader->UpdatePosition(PathWaypoints[PATH_WP_COUNT - 1], true);
                        leader->StopMovingOnCurrentPos();
                        leader->RemoveAllAuras();
                        leader->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_READY1H);
                        if (!leader->IsVisible())
                            leader->SetVisible(true);
                        lk->CastSpell(leader, SPELL_HARVEST_SOUL, false);
                    }
                }
                ++_outroStep;
                _outroTimer = 500;
                break;
            case 2:
            {
                uint32 entry = GetTeamIdInInstance() == TEAM_ALLIANCE ? GO_THE_SKYBREAKER : GO_ORGRIMS_HAMMER;
                _transport = sTransportMgr->CreateTransport(entry, 0, instance);
                ++_outroStep;
                _outroTimer = GetTeamIdInInstance() == TEAM_ALLIANCE ? 10000 : 10500;
                break;
            }
            case 3:
                if (_transport)
                    _transport->EnableMovement(false);
                if (Creature* captain = GetCreature(DATA_SHIP_CAPTAIN))
                    captain->AI()->Talk(GetTeamIdInInstance() == TEAM_ALLIANCE ? SAY_FIRE_ALLY : SAY_FIRE_HORDE);
                if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                {
                    leader->RemoveAllAuras();
                    leader->CastSpell(leader, SPELL_GUNSHIP_CANNON_FIRE_PERIODIC, true);
                }
                if (Creature* lichKing = GetCreature(NPC_LICH_KING_BOSS))
                {
                    lichKing->InterruptNonMeleeSpells(true);
                    lichKing->RemoveAllAuras();
                }
                ++_outroStep;
                _outroTimer = 5000;
                break;
            case 4:
                HandleGameObject(GO_CAVE_IN, false);
                ++_outroStep;
                _outroTimer = 3000;
                break;
            case 5:
                if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                    leader->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                if (Creature* lichKing = GetCreature(NPC_LICH_KING_BOSS))
                {
                    lichKing->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);
                    lichKing->GetThreatMgr().ClearAllThreat();
                    lichKing->CombatStop(true);
                    lichKing->InterruptNonMeleeSpells(true);
                    lichKing->SetVisible(false);
                }
                if (instance->IsHeroic())
                    instance->ToInstanceMap()->PermBindAllPlayers();
                if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                    leader->CastSpell(leader, SPELL_ACHIEVEMENT_CHECK, true);
                ++_outroStep;
                _outroTimer = 1000;
                break;
            case 6:
                if (_transport)
                    _transport->EnableMovement(true);
                ++_outroStep;
                _outroTimer = 3500;
                break;
            case 7:
                if (_transport)
                    _transport->EnableMovement(false);
                if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                {
                    uint8 index = GetTeamIdInInstance() == TEAM_ALLIANCE ? 0 : 1;
                    for (uint8 i = 0; i < 3; ++i)
                        if (StairsPos[index][i].GetPositionX())
                            if (GameObject* go = leader->SummonGameObject(GetTeamIdInInstance() == TEAM_ALLIANCE ? GO_STAIRS_ALLIANCE : GO_STAIRS_HORDE, StairsPos[index][i].GetPositionX(), StairsPos[index][i].GetPositionY(), StairsPos[index][i].GetPositionZ(), StairsPos[index][i].GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 86400, false))
                                go->SetGameObjectFlag(GO_FLAG_INTERACT_COND | GO_FLAG_NOT_SELECTABLE);
                }
                ++_outroStep;
                _outroTimer = 1000;
                break;
            case 8:
                if (Creature* captain = GetCreature(DATA_SHIP_CAPTAIN))
                    captain->AI()->Talk(GetTeamIdInInstance() == TEAM_ALLIANCE ? SAY_ONBOARD_ALLY : SAY_ONBOARD_HORDE);
                if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                {
                    leader->AddUnitMovementFlag(MOVEMENTFLAG_WALKING);
                    leader->GetMotionMaster()->MovePoint(0, WalkCaveInPos);
                    leader->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
                }
                ++_outroStep;
                _outroTimer = 6000;
                break;
            case 9:
                if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                    leader->AI()->Talk(GetTeamIdInInstance() == TEAM_ALLIANCE ? SAY_JAINA_FINAL_1 : SAY_SYLVANA_FINAL);
                HandleGameObject(GO_CAVE_IN, true);
                ++_outroStep;
                _outroTimer = 11000;
                break;
            case 10:
                ++_outroStep;
                _outroTimer = 0;
                instance->DoForAllPlayers([](Player* player)
                {
                    player->KilledMonsterCredit(NPC_WRATH_OF_THE_LICH_KING_CREDIT);
                });
                if (GetTeamIdInInstance() == TEAM_ALLIANCE)
                    if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                    {
                        leader->AI()->Talk(SAY_JAINA_FINAL_2);
                        _outroTimer = 10000;
                    }
                break;
            case 11:
                if (Creature* leader = GetCreature(NPC_SYLVANAS_PART2))
                    leader->SetNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);
                ++_outroStep;
                _outroTimer = 300 * 1000;
                break;
            case 12:
                _outroStep = 0;
                _outroTimer = 0;
                if (_transport)
                    _transport->setActive(false);
                break;
        }
    }

    // GUIDs not handled by ObjectData (arrays with position-based assignment)
    ObjectGuid _spiritualReflectionGUID[MAX_SPIRITUAL_REFLECTIONS];
    ObjectGuid _iceWallTargetGUID[MAX_ICE_WALL_TARGETS];
    ObjectGuid _trashGUID[NUM_OF_TRASH];
    ObjectGuid::LowType _trashSpawnId[NUM_OF_TRASH]{}; // 刷怪点 ID，对象被核心重建后据此重新绑定

    // Runtime state (not persisted)
    uint32 _batteredHiltStatus{};

    // Wave system
    bool _trashActive[NUM_OF_TRASH]{};
    uint8 _trashCounter{};
    uint32 _chosenComposition[8][5]{};
    uint8 _waveNumber{};
    uint32 _nextWaveTimer{};
    uint16 _playerCheckTimer{};
    uint16 _waveResumeTimer{};
    uint8 _waveResumeStep{};
    bool _falricPhaseComplete{};
    uint8 _remainingTrashKills{};           // 本波剩余待击杀的小怪数量
    ObjectGuid _waveTrashGUID[5];           // 本波激活的小怪，用于精确统计击杀
    uint8 _waveTrashCount{};
    uint8 _waveActivateCursor{};            // 本波已处理到组合中的第几只（补刷进度）
    uint8 _trashWaveNumber[NUM_OF_TRASH]{}; // 每只小怪最后一次被激活的波次（0xFF 表示跳过阶段消耗）

    // Lich King fight
    bool _isLichKingFightActive{};
    uint32 _outroTimer{};
    uint8 _outroStep{};
    MotionTransport* _transport{};
};

void AddSC_instance_halls_of_reflection()
{
    RegisterInstanceScript(instance_halls_of_reflection, MAP_HALLS_OF_REFLECTION);
}
