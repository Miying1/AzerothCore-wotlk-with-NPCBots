/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or any later version.
 */

#include "rift_defines.h"

#include "Chat.h"
#include "Creature.h"
#include "GameObject.h"
#include "Group.h"
#include "InstanceSaveMgr.h"
#include "Log.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace HeroicDungeonRift
{
namespace
{
uint32 GetSourceEntry(uint32 bossId)
{
    switch (bossId)
    {
        case BossIdVanCleef:
            return SourceEntryVanCleef;
        case BossIdSneed:
            return SourceEntrySneed;
        case BossIdCookie:
            return SourceEntryCookie;
        case BossIdElectrocutioner:
            return SourceEntryElectrocutioner;
        case BossIdThermaplugg:
            return SourceEntryThermaplugg;
        case BossIdAmnennar:
            return SourceEntryAmnennar;
        case BossIdGlutton:
            return SourceEntryGlutton;
        case BossIdRamtusk:
            return SourceEntryRamtusk;
        case BossIdCharlga:
            return SourceEntryCharlga;
        case BossIdMograineWhitemane:
            return SourceEntryMograine;
        case BossIdHerod:
            return SourceEntryHerod;
        case BossIdKelris:
            return SourceEntryKelris;
        case BossIdAkumai:
            return SourceEntryAkumai;
        case BossIdArchaedas:
            return SourceEntryArchaedas;
        case BossIdEranikus:
            return SourceEntryEranikus;
        case BossIdJammalan:
            return SourceEntryJammalan;
        case BossIdVerdan:
            return SourceEntryVerdan;
        case BossIdMutanus:
            return SourceEntryMutanus;
        case BossIdSpringvale:
            return SourceEntrySpringvale;
        case BossIdArugal:
            return SourceEntryArugal;
        case BossIdDoan:
            return SourceEntryDoan;
        case BossIdGahzrilla:
            return SourceEntryGahzrilla;
        case BossIdUkorz:
            return SourceEntryUkorz;
        case BossIdVyletongue:
            return SourceEntryVyletongue;
        case BossIdCelebras:
            return SourceEntryCelebras;
        case BossIdTheradras:
            return SourceEntryTheradras;
        case BossIdRoccor:
            return SourceEntryRoccor;
        case BossIdIncendius:
            return SourceEntryIncendius;
        case BossIdBaelGar:
            return SourceEntryBaelGar;
        case BossIdArgelmach:
            return SourceEntryArgelmach;
        case BossIdFlamelash:
            return SourceEntryFlamelash;
        case BossIdTheSeven:
            return SourceEntryTheSeven;
        case BossIdThaurissan:
            return SourceEntryThaurissan;
        case BossIdSmolderweb:
            return SourceEntrySmolderweb;
        case BossIdWyrmthalak:
            return SourceEntryWyrmthalak;
        case BossIdLethtendris:
            return SourceEntryLethtendris;
        case BossIdAlzzin:
            return SourceEntryAlzzin;
        case BossIdImmolthar:
            return SourceEntryImmolthar;
        case BossIdTortheldrin:
            return SourceEntryTortheldrin;
        case BossIdMoldar:
            return SourceEntryMoldar;
        case BossIdGordok:
            return SourceEntryGordok;
        case BossIdRasFrostwhisper:
            return SourceEntryRasFrostwhisper;
        case BossIdRavenian:
            return SourceEntryRavenian;
        case BossIdGandling:
            return SourceEntryGandling;
        case BossIdBalnazzar:
            return SourceEntryBalnazzar;
        case BossIdAnastari:
            return SourceEntryAnastari;
        case BossIdRivendare:
            return SourceEntryRivendare;
        default:
            return 0;
    }
}

void SendMessage(Player* player, std::string const& message)
{
    if (player && player->GetSession())
        ChatHandler(player->GetSession()).SendSysMessage(message);
}
} // namespace

uint32 GetExitPortalEntryForTier(uint8 tier)
{
    switch (tier)
    {
        case 1:
            return ExitPortalEntryTier1;
        case 2:
            return ExitPortalEntryTier2;
        case 3:
            return ExitPortalEntryTier3;
        default:
            return 0;
    }
}

bool IsExitPortalEntry(uint32 entry)
{
    return entry == ExitPortalEntryTier1 || entry == ExitPortalEntryTier2 || entry == ExitPortalEntryTier3;
}

RunManager& RunManager::Instance()
{
    static RunManager instance;
    return instance;
}

bool RunManager::ValidateAndCollectMembers(Player* initiator, std::vector<Player*>& members, std::string& error) const
{
    if (!initiator || initiator->IsBeingTeleported() || initiator->IsInCombat())
    {
        error = "当前状态无法进入五人英雄裂隙。";
        return false;
    }

    Group* group = initiator->GetGroup();
    if (group)
    {
        if (!group->IsLeader(initiator->GetGUID()))
        {
            error = "只有队长可以发起五人英雄裂隙。";
            return false;
        }

        if (group->isRaidGroup() || group->isBGGroup() || group->isBFGroup() || group->isLFGGroup())
        {
            error = "五人英雄裂隙只支持普通小队。";
            return false;
        }

        if (group->GetMembersCount() > MaxCombatSlots)
        {
            error = "队伍战斗席位不能超过5个。";
            return false;
        }

        for (GroupReference* reference = group->GetFirstMember(); reference; reference = reference->next())
        {
            Player* member = reference->GetSource();
            if (!member || !member->IsInWorld())
            {
                error = "所有真实玩家队员必须在线。";
                return false;
            }

            if (member->IsInCombat() || member->IsBeingTeleported())
            {
                error = "有队员正在战斗或传送，暂时无法进入。";
                return false;
            }

            if (member->GetMapId() != initiator->GetMapId() || !member->IsWithinDistInMap(initiator, 40.0f))
            {
                error = "所有真实玩家队员必须在入口附近40码内。";
                return false;
            }

            members.push_back(member);
        }
    }
    else
        members.push_back(initiator);

    if (members.empty() || members.size() > MaxCombatSlots)
    {
        error = "有效真实玩家数量必须为1至5人。";
        return false;
    }

    for (Player* member : members)
    {
        if (FindByPlayer(member->GetGUID()))
        {
            error = "队伍中已有成员处于五人英雄裂隙运行中。";
            return false;
        }
    }

    return true;
}

Position RunManager::MakePlayerDestination(Map const* map, Position const& center, Position const& boss, uint8 index, uint8 count) const
{
    float angle = count > 0 ? float(2.0 * M_PI * index / count) : 0.0f;
    angle += frand(-0.35f, 0.35f);
    float radius = frand(1.0f, 2.0f);
    float x = center.GetPositionX() + std::cos(angle) * radius;
    float y = center.GetPositionY() + std::sin(angle) * radius;
    float z = center.GetPositionZ();

    if (map)
    {
        float groundZ = map->GetHeight(x, y, center.GetPositionZ() + 2.0f, true, 5.0f);
        if (groundZ > INVALID_HEIGHT)
            z = groundZ;
    }

    Position result;
    result.Relocate(x, y, z, std::atan2(boss.GetPositionY() - y, boss.GetPositionX() - x));
    return result;
}

bool RunManager::StartRun(Player* initiator, uint8 tier, std::string& error)
{
    if (tier < 1 || tier > MaxTier)
    {
        error = "无效的裂隙难度。";
        return false;
    }

    std::vector<Player*> members;
    if (!ValidateAndCollectMembers(initiator, members, error))
        return false;

    TierConfig const* tierConfig = ConfigStore::Instance().SelectRandomTier(tier);
    if (!tierConfig)
    {
        error = "当前没有可用的该难度Boss配置。";
        return false;
    }

    BossConfig const* bossConfig = ConfigStore::Instance().GetBoss(tierConfig->BossId);
    if (!bossConfig)
    {
        error = "随机到的Boss基础配置不存在。";
        return false;
    }

    Player* leader = initiator;
    if (Group* group = initiator->GetGroup())
        leader = ObjectAccessor::FindConnectedPlayer(group->GetLeaderGUID());

    if (!leader)
    {
        error = "无法找到在线队长。";
        return false;
    }

    Difficulty difficulty = DUNGEON_DIFFICULTY_NORMAL;
    for (Player* member : members)
    {
        InstancePlayerBind* bind = sInstanceSaveMgr->PlayerGetBoundInstance(member->GetGUID(), bossConfig->MapId, difficulty);
        if (bind && bind->perm)
        {
            error = "有队员已永久绑定目标副本，无法为本场创建独立裂隙实例。";
            return false;
        }
    }

    for (Player* member : members)
    {
        if (sInstanceSaveMgr->PlayerGetBoundInstance(member->GetGUID(), bossConfig->MapId, difficulty))
            sInstanceSaveMgr->PlayerUnbindInstance(member->GetGUID(), bossConfig->MapId, difficulty, true, member);
    }

    Map* targetMap = sMapMgr->CreateMap(bossConfig->MapId, leader);
    if (!targetMap || !targetMap->IsDungeon())
    {
        error = "无法创建目标副本实例。";
        return false;
    }

    if (sInstanceSaveMgr->PlayerGetDestinationInstanceId(leader, bossConfig->MapId, difficulty) != 0)
    {
        error = "目标副本仍存在未清理的队长路由，拒绝复用旧实例。";
        return false;
    }

    std::shared_ptr<RunComponent> run = std::make_shared<RunComponent>();
    run->RunToken = _nextRunToken++;
    run->InitiatorGuid = initiator->GetGUID();
    run->GroupGuid = initiator->GetGroup() ? initiator->GetGroup()->GetGUID() : ObjectGuid::Empty;
    run->LeaderGuid = leader->GetGUID();
    run->BossId = tierConfig->BossId;
    run->Tier = tierConfig->Tier;
    run->EntryId = tierConfig->EntryId;
    run->MapId = bossConfig->MapId;
    run->InstanceId = targetMap->GetInstanceId();
    run->BossSpawn = tierConfig->BossSpawn;
    run->PlayerEntry = tierConfig->PlayerEntry;
    run->SharedReturnLocation = WorldLocation(initiator->GetMapId(), initiator->GetPositionX(), initiator->GetPositionY(), initiator->GetPositionZ(), initiator->GetOrientation());
    run->SharedReturnInstanceId = initiator->GetInstanceId();
    run->SharedReturnDifficulty = uint8(initiator->GetMap()->GetDifficulty());

    InstanceSave* instanceSave = sInstanceSaveMgr->GetInstanceSave(run->InstanceId);
    if (!instanceSave || instanceSave->GetMapId() != run->MapId || instanceSave->GetDifficulty() != difficulty)
    {
        error = "新裂隙实例缺少InstanceSave，无法建立整队路由。";
        return false;
    }

    for (Player* member : members)
    {
        sInstanceSaveMgr->PlayerCreateBoundInstancesMaps(member->GetGUID());
        sInstanceSaveMgr->PlayerBindToInstance(member->GetGUID(), instanceSave, false, member);
    }
    for (Player* member : members)
    {
        MemberState state;
        state.PlayerGuid = member->GetGUID();
        run->Members.push_back(state);
        _pendingPlayers[member->GetGUID()] = run;
    }

    _runs[RunKey(run->MapId, run->InstanceId)] = run;

    uint8 index = 0;
    for (Player* member : members)
    {
        Position destination = MakePlayerDestination(targetMap, run->PlayerEntry, run->BossSpawn, index++, members.size());
        MemberState* memberState = FindMember(*run, member->GetGUID());
        memberState->TeleportIssued = member->TeleportTo(run->MapId, destination.GetPositionX(), destination.GetPositionY(),
            destination.GetPositionZ(), destination.GetOrientation(), 0, nullptr, true);

        if (!memberState->TeleportIssued)
        {
            error = "队伍成员传送启动失败，本场已进入回滚流程。";
            run->State = EncounterState::Failed;

            bool anyTeleportIssued = false;
            for (MemberState& state : run->Members)
            {
                anyTeleportIssued = anyTeleportIssued || state.TeleportIssued;
                if (!state.TeleportIssued)
                    state.Exited = true;
            }
            if (!anyTeleportIssued)
                CleanupRun(RunKey(run->MapId, run->InstanceId), true);
            return false;
        }
    }

    LOG_INFO("scripts", "Five-player heroic rift run {} started: boss {}, tier {}, map {}, instance {}.",
        run->RunToken, run->BossId, run->Tier, run->MapId, run->InstanceId);
    return true;
}

MemberState* RunManager::FindMember(RunComponent& run, ObjectGuid playerGuid) const
{
    auto itr = std::find_if(run.Members.begin(), run.Members.end(), [playerGuid](MemberState const& member)
    {
        return member.PlayerGuid == playerGuid;
    });
    return itr == run.Members.end() ? nullptr : &*itr;
}

void RunManager::RemoveOriginalBoss(Map* map, uint32 bossId) const
{
    uint32 sourceEntry = GetSourceEntry(bossId);
    if (!map || !sourceEntry)
        return;

    std::vector<Creature*> originals;
    for (auto const& pair : map->GetCreatureBySpawnIdStore())
    {
        Creature* creature = pair.second;
        if (creature && creature->GetEntry() == sourceEntry)
            originals.push_back(creature);
    }

    for (Creature* creature : originals)
        creature->DespawnOrUnsummon(0ms, Hours(2));

    if (bossId == BossIdMograineWhitemane)
    {
        originals.clear();
        for (auto const& pair : map->GetCreatureBySpawnIdStore())
        {
            Creature* creature = pair.second;
            if (creature && creature->GetEntry() == SourceEntryWhitemane)
                originals.push_back(creature);
        }

        for (Creature* creature : originals)
            creature->DespawnOrUnsummon(0ms, Hours(2));
    }
}

bool RunManager::InitializeRunObjects(Map* map, std::shared_ptr<RunComponent> const& run)
{
    if (!map || !run || run->SpawnInitialized)
        return run && run->SpawnInitialized;

    Player* summoner = nullptr;
    for (MemberState const& member : run->Members)
    {
        Player* candidate = ObjectAccessor::FindConnectedPlayer(member.PlayerGuid);
        if (candidate && candidate->GetMap() == map)
        {
            summoner = candidate;
            break;
        }
    }
    if (!summoner)
        return false;

    RemoveOriginalBoss(map, run->BossId);

    Creature* boss = summoner->SummonCreature(run->EntryId, run->BossSpawn, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, CreatureSummonLifetimeMilliseconds);
    if (!boss)
        return false;

    uint32 exitPortalEntry = GetExitPortalEntryForTier(run->Tier);
    if (!exitPortalEntry)
    {
        boss->DespawnOrUnsummon();
        return false;
    }

    GameObject* portal = summoner->SummonGameObject(exitPortalEntry,
        run->PlayerEntry.GetPositionX(), run->PlayerEntry.GetPositionY(), run->PlayerEntry.GetPositionZ(), run->PlayerEntry.GetOrientation(),
        0.0f, 0.0f, 0.0f, 1.0f, GameObjectSummonLifetimeSeconds);
    if (!portal)
    {
        boss->DespawnOrUnsummon();
        return false;
    }

    run->BossGuid = boss->GetGUID();
    run->ExitPortalGuid = portal->GetGUID();
    run->State = EncounterState::Active;
    run->SpawnInitialized = true;
    return true;
}

void RunManager::OnPlayerEnterMap(Map* map, Player* player)
{
    if (!map || !player)
        return;

    auto pendingItr = _pendingPlayers.find(player->GetGUID());
    if (pendingItr == _pendingPlayers.end())
        return;

    std::shared_ptr<RunComponent> run = pendingItr->second.lock();
    if (!run || run->MapId != map->GetId() || run->InstanceId != map->GetInstanceId())
        return;

    MemberState* member = FindMember(*run, player->GetGUID());
    if (!member)
        return;

    member->Entered = true;
    _playerRuns[player->GetGUID()] = run;
    _pendingPlayers.erase(pendingItr);

    if (run->State == EncounterState::Failed)
    {
        if (TeleportToSharedReturn(player, *run))
            member->Exited = true;
        return;
    }

    if (!run->SpawnInitialized)
    {
        if (!InitializeRunObjects(map, run))
        {
            SendMessage(player, "裂隙Boss或出口门生成失败，本场将回滚。");
            run->State = EncounterState::Failed;
        }
    }
}

void RunManager::OnPlayerLeaveMap(Map* map, Player* player)
{
    if (!map || !player)
        return;

    std::shared_ptr<RunComponent> run = FindByPlayer(player->GetGUID());
    if (!run || run->MapId != map->GetId() || run->InstanceId != map->GetInstanceId())
        return;

    // Leaving the map is not the same as permanently exiting the rift. In
    // particular, releasing spirit normally moves a ghost to an outdoor
    // graveyard. Keep the run association and temporary instance bind so the
    // player can run back into this exact instance. Exited is set only after a
    // successful portal/rollback transfer.
}

void RunManager::OnMapUpdate(Map* map, uint32 diff)
{
    if (!map || !map->IsDungeon())
        return;

    RunKey key(map->GetId(), map->GetInstanceId());
    auto itr = _runs.find(key);
    if (itr == _runs.end())
        return;

    std::shared_ptr<RunComponent> run = itr->second;
    run->ElapsedMilliseconds += diff;

    if (run->State == EncounterState::Failed)
    {
        RollbackRun(run);
        bool rollbackFinished = std::all_of(run->Members.begin(), run->Members.end(), [](MemberState const& member)
        {
            return member.Exited;
        });
        if (rollbackFinished || run->ElapsedMilliseconds >= RollbackGraceMilliseconds)
            CleanupRun(key, true);
        return;
    }

    bool allExited = std::all_of(run->Members.begin(), run->Members.end(), [](MemberState const& member)
    {
        return member.Exited;
    });

    if (run->ElapsedMilliseconds >= RunTimeoutMilliseconds)
    {
        RollbackRun(run);
        CleanupRun(key, true);
        return;
    }

    if (allExited)
        CleanupRun(key, true);
}

bool RunManager::TeleportToSharedReturn(Player* player, RunComponent const& run) const
{
    if (!player)
        return false;

    if (run.SharedReturnInstanceId)
    {
        InstanceSave* returnSave = sInstanceSaveMgr->GetInstanceSave(run.SharedReturnInstanceId);
        Difficulty returnDifficulty = Difficulty(run.SharedReturnDifficulty);
        if (!returnSave || returnSave->GetMapId() != run.SharedReturnLocation.GetMapId() || returnSave->GetDifficulty() != returnDifficulty)
            return false;

        InstancePlayerBind* existingBind = sInstanceSaveMgr->PlayerGetBoundInstance(player->GetGUID(), returnSave->GetMapId(), returnDifficulty);
        if (existingBind && existingBind->save != returnSave)
        {
            if (existingBind->perm)
                return false;
            sInstanceSaveMgr->PlayerUnbindInstance(player->GetGUID(), returnSave->GetMapId(), returnDifficulty, true, player);
        }

        sInstanceSaveMgr->PlayerCreateBoundInstancesMaps(player->GetGUID());
        sInstanceSaveMgr->PlayerBindToInstance(player->GetGUID(), returnSave, false, player);
    }

    bool forceInstanceTransfer = player->GetMapId() == run.SharedReturnLocation.GetMapId() && player->GetInstanceId() != run.SharedReturnInstanceId;
    return player->TeleportTo(run.SharedReturnLocation.GetMapId(), run.SharedReturnLocation.GetPositionX(),
        run.SharedReturnLocation.GetPositionY(), run.SharedReturnLocation.GetPositionZ(),
        run.SharedReturnLocation.GetOrientation(), 0, nullptr, forceInstanceTransfer);
}

void RunManager::RollbackRun(std::shared_ptr<RunComponent> const& run)
{
    if (!run)
        return;

    for (MemberState& member : run->Members)
    {
        if (member.Exited)
            continue;

        Player* player = ObjectAccessor::FindConnectedPlayer(member.PlayerGuid);
        if (!player)
        {
            member.Exited = true;
            continue;
        }

        if (player->GetMapId() == run->MapId && player->GetInstanceId() == run->InstanceId)
        {
            if (TeleportToSharedReturn(player, *run))
                member.Exited = true;
            continue;
        }

        if (!member.TeleportIssued || !player->IsBeingTeleported())
            member.Exited = true;
    }
}

void RunManager::CleanupRun(RunKey const& key, bool unbindPlayers)
{
    auto itr = _runs.find(key);
    if (itr == _runs.end())
        return;

    std::shared_ptr<RunComponent> run = itr->second;
    run->State = EncounterState::Cleaning;

    if (Map* map = sMapMgr->FindMap(run->MapId, run->InstanceId))
    {
        if (Creature* boss = map->GetCreature(run->BossGuid))
            boss->DespawnOrUnsummon();
        if (GameObject* portal = map->GetGameObject(run->ExitPortalGuid))
            portal->Delete();
    }

    for (MemberState const& member : run->Members)
    {
        _playerRuns.erase(member.PlayerGuid);
        _pendingPlayers.erase(member.PlayerGuid);
        if (unbindPlayers)
        {
            InstancePlayerBind* bind = sInstanceSaveMgr->PlayerGetBoundInstance(member.PlayerGuid, run->MapId, DUNGEON_DIFFICULTY_NORMAL);
            if (bind && bind->save && bind->save->GetInstanceId() == run->InstanceId && !bind->perm)
            {
                Player* player = ObjectAccessor::FindConnectedPlayer(member.PlayerGuid);
                sInstanceSaveMgr->PlayerUnbindInstance(member.PlayerGuid, run->MapId, DUNGEON_DIFFICULTY_NORMAL, true, player);
            }
        }
    }

    LOG_INFO("scripts", "Five-player heroic rift run {} cleaned: map {}, instance {}.", run->RunToken, run->MapId, run->InstanceId);
    _runs.erase(itr);
}

bool RunManager::ExitRun(Player* player, GameObject* portal, std::string& error)
{
    if (!player || !portal)
    {
        error = "无效的出口请求。";
        return false;
    }

    std::shared_ptr<RunComponent> run = FindByPlayer(player->GetGUID());
    if (!IsExitPortalEntry(portal->GetEntry()) || !run || run->ExitPortalGuid != portal->GetGUID() || run->MapId != player->GetMapId() || run->InstanceId != player->GetInstanceId())
    {
        error = "该出口不属于你的当前裂隙。";
        return false;
    }

    MemberState* member = FindMember(*run, player->GetGUID());
    if (!member || !member->Entered || member->Exited)
    {
        error = "你已经离开或不属于本场裂隙。";
        return false;
    }

    if (!TeleportToSharedReturn(player, *run))
    {
        error = "返回共享入口位置或原实例失败。";
        return false;
    }

    member->Exited = true;
    return true;
}

void RunManager::OnDestroyInstance(Map* map)
{
    if (map)
        CleanupRun(RunKey(map->GetId(), map->GetInstanceId()), true);
}

void RunManager::Clear()
{
    std::vector<RunKey> keys;
    for (auto const& pair : _runs)
        keys.push_back(pair.first);
    for (RunKey const& key : keys)
        CleanupRun(key, false);
}

std::shared_ptr<RunComponent> RunManager::FindByInstance(uint32 mapId, uint32 instanceId) const
{
    auto itr = _runs.find(RunKey(mapId, instanceId));
    return itr == _runs.end() ? nullptr : itr->second;
}

std::shared_ptr<RunComponent> RunManager::FindByPlayer(ObjectGuid playerGuid) const
{
    auto itr = _playerRuns.find(playerGuid);
    if (itr != _playerRuns.end())
        return itr->second.lock();

    auto pendingItr = _pendingPlayers.find(playerGuid);
    return pendingItr == _pendingPlayers.end() ? nullptr : pendingItr->second.lock();
}

std::shared_ptr<RunComponent> RunManager::FindByCreature(Creature const* creature) const
{
    if (!creature)
        return nullptr;

    std::shared_ptr<RunComponent> run = FindByInstance(creature->GetMapId(), creature->GetInstanceId());
    if (!run || (!run->BossGuid.IsEmpty() && run->BossGuid != creature->GetGUID()))
        return nullptr;
    return run;
}

uint8 GetTierForCreature(Creature const* creature)
{
    std::shared_ptr<RunComponent> run = RunManager::Instance().FindByCreature(creature);
    return run ? run->Tier : 0;
}

TierConfig const* GetTierConfigForCreature(Creature const* creature)
{
    if (!creature)
        return nullptr;

    uint32 bossId = ConfigStore::Instance().GetBossIdByEntry(creature->GetEntry());
    std::shared_ptr<RunComponent> run = RunManager::Instance().FindByCreature(creature);
    return run && bossId == run->BossId ? ConfigStore::Instance().GetTier(run->BossId, run->Tier) : nullptr;
}

void ApplyTierStats(Creature* creature, TierConfig const& config, uint32 baseHealth)
{
    if (!creature)
        return;

    uint64 scaledHealth = uint64(std::max<uint32>(1, baseHealth)) * config.HealthMultiplier;
    scaledHealth = std::min<uint64>(scaledHealth, std::numeric_limits<uint32>::max());
    creature->SetCreateHealth(uint32(scaledHealth));
    creature->SetMaxHealth(uint32(scaledHealth));
    creature->SetFullHealth();
}

} // namespace HeroicDungeonRift
