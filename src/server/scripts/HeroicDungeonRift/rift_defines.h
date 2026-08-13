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
constexpr uint32 BossIdVerdan = 17;
constexpr uint32 BossIdMutanus = 18;
constexpr uint32 BossIdSpringvale = 19;
constexpr uint32 BossIdArugal = 20;
constexpr uint32 BossIdDoan = 21;
constexpr uint32 BossIdGahzrilla = 22;
constexpr uint32 BossIdUkorz = 23;
constexpr uint32 BossIdVyletongue = 24;
constexpr uint32 BossIdCelebras = 25;
constexpr uint32 BossIdTheradras = 26;
constexpr uint32 BossIdRoccor = 27;
constexpr uint32 BossIdIncendius = 28;
constexpr uint32 BossIdBaelGar = 29;
constexpr uint32 BossIdArgelmach = 30;
constexpr uint32 BossIdFlamelash = 31;
constexpr uint32 BossIdTheSeven = 32;
constexpr uint32 BossIdThaurissan = 33;
constexpr uint32 BossIdSmolderweb = 34;
constexpr uint32 BossIdWyrmthalak = 35;
constexpr uint32 BossIdLethtendris = 36;
constexpr uint32 BossIdAlzzin = 37;
constexpr uint32 BossIdImmolthar = 38;
constexpr uint32 BossIdTortheldrin = 39;
constexpr uint32 BossIdMoldar = 40;
constexpr uint32 BossIdGordok = 41;
constexpr uint32 BossIdRasFrostwhisper = 42;
constexpr uint32 BossIdRavenian = 43;
constexpr uint32 BossIdGandling = 44;
constexpr uint32 BossIdBalnazzar = 45;
constexpr uint32 BossIdAnastari = 46;
constexpr uint32 BossIdRivendare = 47;

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
constexpr uint32 SourceEntryVerdan = 5775;
constexpr uint32 SourceEntryMutanus = 3654;
constexpr uint32 SourceEntrySpringvale = 4278;
constexpr uint32 SourceEntryArugal = 4275;
constexpr uint32 SourceEntryDoan = 6487;
constexpr uint32 SourceEntryGahzrilla = 7273;
constexpr uint32 SourceEntryUkorz = 7267;
constexpr uint32 SourceEntryVyletongue = 12236;
constexpr uint32 SourceEntryCelebras = 12225;
constexpr uint32 SourceEntryTheradras = 12201;
constexpr uint32 SourceEntryRoccor = 9025;
constexpr uint32 SourceEntryIncendius = 9017;
constexpr uint32 SourceEntryBaelGar = 9016;
constexpr uint32 SourceEntryArgelmach = 8983;
constexpr uint32 SourceEntryFlamelash = 9156;
constexpr uint32 SourceEntryTheSeven = 9039; // 七贤主Boss取末日之链(Doom'rel 9039)，其余6名成员为裂隙专用同伴
constexpr uint32 SourceEntryThaurissan = 9019;
constexpr uint32 SourceEntrySmolderweb = 10596;
constexpr uint32 SourceEntryWyrmthalak = 9568;
constexpr uint32 SourceEntryLethtendris = 14327;
constexpr uint32 SourceEntryAlzzin = 11492;
constexpr uint32 SourceEntryImmolthar = 11496;
constexpr uint32 SourceEntryTortheldrin = 11486;
constexpr uint32 SourceEntryMoldar = 14326;
constexpr uint32 SourceEntryGordok = 11501;
constexpr uint32 SourceEntryRasFrostwhisper = 10508;
constexpr uint32 SourceEntryRavenian = 10507;
constexpr uint32 SourceEntryGandling = 1853;
constexpr uint32 SourceEntryBalnazzar = 10813;
constexpr uint32 SourceEntryAnastari = 10436;
constexpr uint32 SourceEntryRivendare = 10440;

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
constexpr uint32 RiftEntryRuuzlu = 100300;
constexpr uint32 RiftEntryChoRush = 100301;
constexpr uint32 RiftEntryMoira = 100302;
constexpr uint32 RiftEntryRisenGuardian = 100303;
constexpr uint32 RiftEntryBurningSpirit = 100304;
constexpr uint32 RiftEntrySpirestoneWarlord = 100305;
constexpr uint32 RiftEntrySmolderthornBerserker = 100306;
constexpr uint32 RiftEntryArugalVoidwalker = 100307;
constexpr uint32 RiftEntryVoidwalkerMinion = 100308;
constexpr uint32 RiftEntrySpireSpiderling = 100309;
constexpr uint32 RiftEntryMindlessSkeleton = 100310;
constexpr uint32 RiftEntryAlzzinMinion = 100311;
constexpr uint32 RiftEntryBaelGarSpawn = 100312;
constexpr uint32 RiftEntrySevenHateRel = 100313;   // 七贤·仇恨者 Hate'rel
constexpr uint32 RiftEntrySevenAngerRel = 100314;  // 七贤·愤怒者 Anger'rel
constexpr uint32 RiftEntrySevenVileRel = 100315;   // 七贤·邪恶者 Vile'rel
constexpr uint32 RiftEntrySevenGloomRel = 100316;  // 七贤·忧郁者 Gloom'rel
constexpr uint32 RiftEntrySevenSeethRel = 100317;  // 七贤·沸腾者 Seeth'rel
constexpr uint32 RiftEntrySevenDopeRel = 100318;   // 七贤·愚昧者 Dope'rel
constexpr uint32 RiftEntryArcanasmith = 100319;    // 阿格曼奇·末日熔炉奥术铁匠 Doomforge Arcanasmith
constexpr uint32 RiftEntryRagereaverGolem = 100320; // 阿格曼奇·怒削魔像 Ragereaver Golem
constexpr uint32 RiftEntryWrathHammer = 100321;    // 阿格曼奇·怒火之锤构造体 Wrath Hammer Construct
constexpr uint32 RiftEntryWeaponTechnician = 100322; // 阿格曼奇·武器技师 Weapon Technician

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
