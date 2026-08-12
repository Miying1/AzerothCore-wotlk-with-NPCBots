/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or any later version.
 */

#ifndef HEROIC_DUNGEON_RIFT_DEFINES_H
#define HEROIC_DUNGEON_RIFT_DEFINES_H

#include "Common.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "ScriptedGossip.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class Creature;
class GameObject;
class Map;
class Player;

namespace HeroicDungeonRift
{
constexpr uint8 MaxTier = 3;
constexpr uint8 MaxCombatSlots = 5;
constexpr uint32 ExitPortalEntryTier1 = 100500;
constexpr uint32 ExitPortalEntryTier2 = 100501;
constexpr uint32 ExitPortalEntryTier3 = 100502;
constexpr uint32 RunTimeoutMilliseconds = 2 * HOUR * IN_MILLISECONDS;
constexpr uint32 RollbackGraceMilliseconds = 30 * IN_MILLISECONDS;
constexpr uint32 CreatureSummonLifetimeMilliseconds = 2 * HOUR * IN_MILLISECONDS;
constexpr uint32 GameObjectSummonLifetimeSeconds = 2 * HOUR;

constexpr uint32 BossIdVanCleef = 1;
constexpr uint32 BossIdSneed = 2;
constexpr uint32 BossIdCookie = 3;
constexpr uint32 BossIdElectrocutioner = 4;
constexpr uint32 BossIdThermaplugg = 5;
constexpr uint32 BossIdAmnennar = 6;
constexpr uint32 BossIdGlutton = 7;
constexpr uint32 BossIdRamtusk = 8;
constexpr uint32 BossIdCharlga = 9;
constexpr uint32 BossIdMograineWhitemane = 10;
constexpr uint32 BossIdHerod = 11;
constexpr uint32 BossIdKelris = 12;
constexpr uint32 BossIdAkumai = 13;
constexpr uint32 BossIdArchaedas = 14;
constexpr uint32 BossIdEranikus = 15;
constexpr uint32 BossIdJammalan = 16;

constexpr uint32 SourceEntryVanCleef = 639;
constexpr uint32 SourceEntrySneed = 642;
constexpr uint32 SourceEntryCookie = 645;
constexpr uint32 SourceEntryElectrocutioner = 6235;
constexpr uint32 SourceEntryThermaplugg = 7800;
constexpr uint32 SourceEntryAmnennar = 7358;
constexpr uint32 SourceEntryGlutton = 8567;
constexpr uint32 SourceEntryRamtusk = 4420;
constexpr uint32 SourceEntryCharlga = 4421;
constexpr uint32 SourceEntryMograine = 3976;
constexpr uint32 SourceEntryWhitemane = 3977;
constexpr uint32 SourceEntryHerod = 3975;
constexpr uint32 SourceEntryKelris = 4832;
constexpr uint32 SourceEntryAkumai = 4829;
constexpr uint32 SourceEntryArchaedas = 2748;
constexpr uint32 SourceEntryEranikus = 5709;
constexpr uint32 SourceEntryJammalan = 5710;

constexpr uint32 RiftEntryWhitemane = 100100;
constexpr uint32 RiftEntryWalkingBomb = 100101;
constexpr uint32 RiftEntryFrostSpectre = 100102;
constexpr uint32 RiftEntryScarletTrainee = 100103;
constexpr uint32 RiftEntryEarthgrabTotem = 100104;
constexpr uint32 RiftEntryEarthenGuardian = 100105;
constexpr uint32 RiftEntryVaultWarder = 100106;
constexpr uint32 RiftEntryEarthenHallshaper = 100107;
constexpr uint32 RiftEntryEarthenCustodian = 100108;
constexpr uint32 RiftEntryAkumaiSnapjaw = 100109;
constexpr uint32 RiftEntryAkumaiServant = 100110;

constexpr uint32 RiftDataTier = 1;
constexpr uint32 RiftDataDamagePermille = 2;

enum GossipActions : uint32
{
    GossipEnterTier1 = GOSSIP_ACTION_INFO_DEF + 1,
    GossipEnterTier2 = GOSSIP_ACTION_INFO_DEF + 2,
    GossipEnterTier3 = GOSSIP_ACTION_INFO_DEF + 3,
    GossipExit = GOSSIP_ACTION_INFO_DEF + 10
};

enum class EncounterState : uint8
{
    Preparing,
    Active,
    Completed,
    Failed,
    Cleaning
};

struct BossConfig
{
    uint32 BossId = 0;
    std::string MapName;
    uint32 MapId = 0;
    Position DefaultPlayerEntry;
    bool Enabled = false;
    std::string Remark;
};

struct TierConfig
{
    uint32 BossId = 0;
    uint32 EntryId = 0;
    Position BossSpawn;
    uint8 Tier = 0;
    float HealthMultiplier = 1.0f;
    float DamageMultiplier = 1.0f;
    Position PlayerEntry;
};

struct MemberState
{
    ObjectGuid PlayerGuid;
    bool Entered = false;
    bool Exited = false;
    bool TeleportIssued = false;
};

struct RunComponent
{
    uint64 RunToken = 0;
    ObjectGuid InitiatorGuid;
    ObjectGuid GroupGuid;
    ObjectGuid LeaderGuid;
    uint32 BossId = 0;
    uint8 Tier = 0;
    uint32 EntryId = 0;
    uint32 MapId = 0;
    uint32 InstanceId = 0;
    Position BossSpawn;
    Position PlayerEntry;
    WorldLocation SharedReturnLocation;
    uint32 SharedReturnInstanceId = 0;
    uint8 SharedReturnDifficulty = 0;
    ObjectGuid BossGuid;
    ObjectGuid ExitPortalGuid;
    std::vector<MemberState> Members;
    EncounterState State = EncounterState::Preparing;
    uint32 ElapsedMilliseconds = 0;
    bool SpawnInitialized = false;
};

class ConfigStore
{
public:
    static ConfigStore& Instance();

    void Load();
    BossConfig const* GetBoss(uint32 bossId) const;
    TierConfig const* GetTier(uint32 bossId, uint8 tier) const;
    TierConfig const* SelectRandomTier(uint8 tier) const;
    uint32 GetBossIdByEntry(uint32 entryId) const;

private:
    std::map<uint32, BossConfig> _bosses;
    std::map<std::pair<uint32, uint8>, TierConfig> _tiers;
    std::map<uint32, uint32> _entryToBoss;
};

class RunManager
{
public:
    static RunManager& Instance();

    bool StartRun(Player* initiator, uint8 tier, std::string& error);
    bool ExitRun(Player* player, GameObject* portal, std::string& error);
    void OnPlayerEnterMap(Map* map, Player* player);
    void OnPlayerLeaveMap(Map* map, Player* player);
    void OnMapUpdate(Map* map, uint32 diff);
    void OnDestroyInstance(Map* map);
    void Clear();

    std::shared_ptr<RunComponent> FindByInstance(uint32 mapId, uint32 instanceId) const;
    std::shared_ptr<RunComponent> FindByPlayer(ObjectGuid playerGuid) const;
    std::shared_ptr<RunComponent> FindByCreature(Creature const* creature) const;

private:
    using RunKey = std::pair<uint32, uint32>;

    RunManager() = default;
    bool ValidateAndCollectMembers(Player* initiator, std::vector<Player*>& members, std::string& error) const;
    Position MakePlayerDestination(Map const* map, Position const& center, Position const& boss, uint8 index, uint8 count) const;
    bool InitializeRunObjects(Map* map, std::shared_ptr<RunComponent> const& run);
    bool TeleportToSharedReturn(Player* player, RunComponent const& run) const;
    void RemoveOriginalBoss(Map* map, uint32 bossId) const;
    void RollbackRun(std::shared_ptr<RunComponent> const& run);
    void CleanupRun(RunKey const& key, bool unbindPlayers);
    MemberState* FindMember(RunComponent& run, ObjectGuid playerGuid) const;

    std::map<RunKey, std::shared_ptr<RunComponent>> _runs;
    std::map<ObjectGuid, std::weak_ptr<RunComponent>> _playerRuns;
    std::map<ObjectGuid, std::weak_ptr<RunComponent>> _pendingPlayers;
    uint64 _nextRunToken = 1;
};

class BossAIBase;

uint32 GetExitPortalEntryForTier(uint8 tier);
bool IsExitPortalEntry(uint32 entry);
uint8 GetTierForCreature(Creature const* creature);
TierConfig const* GetTierConfigForCreature(Creature const* creature);
void ApplyTierStats(Creature* creature, TierConfig const& config, uint32 baseHealth);

} // namespace HeroicDungeonRift

#endif
