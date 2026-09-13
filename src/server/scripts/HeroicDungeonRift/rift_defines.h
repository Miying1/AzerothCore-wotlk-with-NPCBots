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
// 区域随机入口：三档各一个虚空门生物模板，运行时在区域刷新点上随机生成。
// 视觉取自卡拉赞虚空幽龙（Netherspite）的三个陷阱传送门：
//   T1 = 17367 Nether Portal - Serenity（绿/平静），门体特效光环 30490
//   T2 = 17368 Nether Portal - Dominance（蓝/统御），门体特效光环 30491
//   T3 = 17369 Nether Portal - Perseverence（红/坚韧），门体特效光环 30487
constexpr uint32 EntranceEntryTier1 = 100510; // 绿
constexpr uint32 EntranceEntryTier2 = 100511; // 蓝
constexpr uint32 EntranceEntryTier3 = 100512; // 红
constexpr uint32 EntranceAuraTier1 = 30490;
constexpr uint32 EntranceAuraTier2 = 30491;
constexpr uint32 EntranceAuraTier3 = 30487;
// 玩家进入裂隙后，入口停止提供对话并保留一段时间再移除。
constexpr uint32 EntrancePurgeGraceMilliseconds = 5 * IN_MILLISECONDS;
// 常规补充间隔：入口数量不低于最小值时，每隔该时间向最大值补齐一次。
constexpr uint32 EntranceRefillIntervalMilliseconds = 5 * MINUTE * IN_MILLISECONDS;
// 时间表评估间隔：开启窗口内每秒一次（保证到点即刷），关闭窗口时放宽以节流。
constexpr uint32 SchedulePollIntervalOpenMilliseconds = 1 * IN_MILLISECONDS;
constexpr uint32 SchedulePollIntervalClosedMilliseconds = 5 * IN_MILLISECONDS;
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
constexpr uint32 BossIdNetherkurse = 48;
constexpr uint32 BossIdKargath = 49;
constexpr uint32 BossIdBroggok = 50;
constexpr uint32 BossIdKelidan = 51;
constexpr uint32 BossIdOmor = 52;
constexpr uint32 BossIdKalithresh = 53;
constexpr uint32 BossIdMuselEk = 54;
constexpr uint32 BossIdBlackStalker = 55;
constexpr uint32 BossIdMennu = 56;
constexpr uint32 BossIdQuagmirran = 57;
constexpr uint32 BossIdDalliah = 58;
constexpr uint32 BossIdSoccothrates = 59;
constexpr uint32 BossIdLaj = 60;
constexpr uint32 BossIdWarpSplinter = 61;
constexpr uint32 BossIdPathaleon = 62;
constexpr uint32 BossIdMurmur = 63;
constexpr uint32 BossIdIkiss = 64;
constexpr uint32 BossIdShaffar = 65;
constexpr uint32 BossIdShirrak = 66;
constexpr uint32 BossIdMaladaar = 67;
constexpr uint32 BossIdKaelthas = 68;
constexpr uint32 BossIdVazruden = 69;

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
constexpr uint32 SourceEntrySevenHateRel = 9034;
constexpr uint32 SourceEntrySevenAngerRel = 9035;
constexpr uint32 SourceEntrySevenVileRel = 9036;
constexpr uint32 SourceEntrySevenGloomRel = 9037;
constexpr uint32 SourceEntrySevenSeethRel = 9038;
constexpr uint32 SourceEntryTheSeven = 9039; // 七贤最终成员末日之链 Doom'rel
constexpr uint32 SourceEntrySevenDopeRel = 9040;
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
constexpr uint32 SourceEntryDathrohan = 10812; // 巴纳扎尔原版的人形态Boss
constexpr uint32 SourceEntryBalnazzar = 10813;
constexpr uint32 SourceEntryAnastari = 10436;
constexpr uint32 SourceEntryRivendare = 10440;
constexpr uint32 SourceEntryNetherkurse = 16807;
constexpr uint32 SourceEntryKargath = 16808;
constexpr uint32 SourceEntryBroggok = 17380;
constexpr uint32 SourceEntryKelidan = 17377;
constexpr uint32 SourceEntryOmor = 17308;
constexpr uint32 SourceEntryKalithresh = 17798;
constexpr uint32 SourceEntryMuselEk = 17826;
constexpr uint32 SourceEntryBlackStalker = 17882;
constexpr uint32 SourceEntryMennu = 17941;
constexpr uint32 SourceEntryQuagmirran = 17942;
constexpr uint32 SourceEntryDalliah = 20885;
constexpr uint32 SourceEntrySoccothrates = 20886;
constexpr uint32 SourceEntryLaj = 17980;
constexpr uint32 SourceEntryWarpSplinter = 17977;
constexpr uint32 SourceEntryPathaleon = 19220;
constexpr uint32 SourceEntryMurmur = 18708;
constexpr uint32 SourceEntryIkiss = 18473;
constexpr uint32 SourceEntryShaffar = 18344;
constexpr uint32 SourceEntryShirrak = 18371;
constexpr uint32 SourceEntryMaladaar = 18373;
constexpr uint32 SourceEntryKaelthas = 24664;
// 裂隙主实体直接使用原版实际战斗Boss维斯路登；17307仅为原版遭遇控制器且无静态战斗实体。
constexpr uint32 SourceEntryVazruden = 17537;

constexpr uint32 RiftEntryWhitemane = 102000;
constexpr uint32 RiftEntryWalkingBomb = 102001;
constexpr uint32 RiftEntryFrostSpectre = 102002;
constexpr uint32 RiftEntryScarletTrainee = 102003;
constexpr uint32 RiftEntryEarthgrabTotem = 102004;
constexpr uint32 RiftEntryEarthenGuardian = 102005;
constexpr uint32 RiftEntryVaultWarder = 102006;
constexpr uint32 RiftEntryEarthenHallshaper = 102007;
constexpr uint32 RiftEntryEarthenCustodian = 102008;
constexpr uint32 RiftEntryAkumaiSnapjaw = 102009;
constexpr uint32 RiftEntryAkumaiServant = 102010;
constexpr uint32 RiftEntryRuuzlu = 102011;
constexpr uint32 RiftEntryChoRush = 102012;
constexpr uint32 RiftEntryMoira = 102013;
constexpr uint32 RiftEntryRisenGuardian = 102014;
constexpr uint32 RiftEntryBurningSpirit = 102015;
constexpr uint32 RiftEntrySpirestoneWarlord = 102016;
constexpr uint32 RiftEntrySmolderthornBerserker = 102017;
constexpr uint32 RiftEntryArugalVoidwalker = 102018;
constexpr uint32 RiftEntryVoidwalkerMinion = 102019;
constexpr uint32 RiftEntrySpireSpiderling = 102020;
constexpr uint32 RiftEntryMindlessSkeleton = 102021;
constexpr uint32 RiftEntryAlzzinMinion = 102022;
constexpr uint32 RiftEntryBaelGarSpawn = 102023;
constexpr uint32 RiftEntrySevenHateRel = 102024;   // 七贤·仇恨者 Hate'rel
constexpr uint32 RiftEntrySevenAngerRel = 102025;  // 七贤·愤怒者 Anger'rel
constexpr uint32 RiftEntrySevenVileRel = 102026;   // 七贤·邪恶者 Vile'rel
constexpr uint32 RiftEntrySevenGloomRel = 102027;  // 七贤·忧郁者 Gloom'rel
constexpr uint32 RiftEntrySevenSeethRel = 102028;  // 七贤·沸腾者 Seeth'rel
constexpr uint32 RiftEntrySevenDopeRel = 102029;   // 七贤·愚昧者 Dope'rel
constexpr uint32 RiftEntryArcanasmith = 102030;    // 阿格曼奇·末日熔炉奥术铁匠 Doomforge Arcanasmith
constexpr uint32 RiftEntryRagereaverGolem = 102031; // 阿格曼奇·怒削魔像 Ragereaver Golem
constexpr uint32 RiftEntryWrathHammer = 102032;    // 阿格曼奇·怒火之锤构造体 Wrath Hammer Construct
constexpr uint32 RiftEntryWeaponTechnician = 102033; // 阿格曼奇·武器技师 Weapon Technician

constexpr uint32 RiftDataTier = 1;
constexpr uint32 RiftDataDamagePermille = 2;
constexpr uint32 RiftDataActivate = 3;

enum GossipActions : uint32
{
    GossipEnterTier1 = GOSSIP_ACTION_INFO_DEF + 1,
    GossipEnterTier2 = GOSSIP_ACTION_INFO_DEF + 2,
    GossipEnterTier3 = GOSSIP_ACTION_INFO_DEF + 3,
    GossipSpecifyTier1 = GOSSIP_ACTION_INFO_DEF + 4,
    GossipSpecifyTier2 = GOSSIP_ACTION_INFO_DEF + 5,
    GossipSpecifyTier3 = GOSSIP_ACTION_INFO_DEF + 6,
    GossipExit = GOSSIP_ACTION_INFO_DEF + 10,
    GossipEnterRiftEntrance = GOSSIP_ACTION_INFO_DEF + 20
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
    uint8 DungeonVersion = 60;
    Position DefaultPlayerEntry;
    bool HasDefaultPlayerEntry = false;
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
    uint8 InstanceDifficulty = 0; // 裂隙实例实际难度（与MapMgr::CreateMap创建的实例难度一致）
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
    bool StartRun(Player* initiator, uint8 tier, uint32 bossId, std::string& error);
    bool ExitRun(Player* player, GameObject* portal, std::string& error);
    void OnPlayerEnterMap(Map* map, Player* player);
    void OnPlayerLeaveMap(Map* map, Player* player);
    void OnPlayerResurrect(Player* player);
    void OnMapUpdate(Map* map, uint32 diff);
    void OnDestroyInstance(Map* map);
    void Clear();

    std::shared_ptr<RunComponent> FindByInstance(uint32 mapId, uint32 instanceId) const;
    std::shared_ptr<RunComponent> FindByPlayer(ObjectGuid playerGuid) const;
    std::shared_ptr<RunComponent> FindByCreature(Creature const* creature) const;

private:
    using RunKey = std::pair<uint32, uint32>;

    RunManager() = default;
    bool StartRun(Player* initiator, TierConfig const& tierConfig, std::vector<Player*> const& members,
        std::string& error);
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

// 区域配置：一行代表一个允许刷新裂隙入口的开放区域。
struct RiftSpawnRegion
{
    uint32 RegionId = 0;
    std::string RegionName; // 用于全服通知显示的区域名称
    uint32 MapId = 0;
    uint32 AreaId = 0;
    Position Center;
    uint8 TierMinCounts[MaxTier] = { 0, 0, 0 }; // 依次为 T1/T2/T3 的最小数量：低于该值时立即补齐
    uint8 TierMaxCounts[MaxTier] = { 0, 0, 0 }; // 依次为 T1/T2/T3 的最大数量：常规补充的上限
    bool Enabled = false;
    std::string Remark;
    std::vector<uint32> PointIds; // 该区域全部可用刷新点
};

// 刷新点：区域内的一个固定候选刷新坐标。
struct RiftSpawnPoint
{
    uint32 PointId = 0;
    uint32 RegionId = 0;
    Position Pos;
    bool Enabled = false;
    std::string Remark;
};

// 每周开启时段：一行代表一个允许刷新入口的时间窗口，可配置多行取并集。
// WeekDay: 0=周日, 1=周一, ... 6=周六；Start/EndMinuteOfDay 为当日分钟数 [0, 1440)。
// 当 EndMinuteOfDay <= StartMinuteOfDay 时视为跨零点窗口（延续到次日）。
struct RiftScheduleWindow
{
    uint32 ScheduleId = 0;
    uint8 WeekDay = 0;
    uint16 StartMinuteOfDay = 0;
    uint16 EndMinuteOfDay = 0;
    bool Enabled = false;
    std::string Remark;
};

// 区域入口刷新管理器：在开放区域内的固定点位上随机生成 T1/T2/T3 虚空门生物，
// 维持每档目标数量；入口被玩家使用后停止对话，并在宽限期结束后移除。
class RiftSpawnManager
{
public:
    static RiftSpawnManager& Instance();

    void Load();
    void Update(uint32 diff);
    void Clear();

    // 查询该生物是否为受管入口及其难度；非受管入口返回 0。
    uint8 GetEntranceTier(Creature const* creature) const;
    // 入口是否已被消耗（已使用或已不在管理中）。
    bool IsEntranceConsumed(Creature const* creature) const;
    // 标记入口已被使用，停止对话并在宽限期后移除。
    void ConsumeEntrance(Creature* creature);

private:
    struct EntranceEntry
    {
        uint64 Token = 0;
        ObjectGuid Guid;
        uint32 RegionId = 0;
        uint32 PointId = 0;
        uint32 MapId = 0;
        uint8 Tier = 0;
        bool Consumed = false;
        uint32 PurgeCountdown = 0;
    };

    RiftSpawnManager() = default;

    // 根据每周时间表切换开启/关闭状态，并在切换时刷满/清空入口与发送全服通知。
    void PollSchedule();
    bool EvaluateSchedule() const;
    bool IsWithinWindow(RiftScheduleWindow const& window, uint32 weekDay, uint32 minuteOfDay) const;
    // 距离下一次开启的剩余秒数（epoch）；没有可用窗口时返回 0。
    int64 ComputeNextOpenTime() const;
    // 开启前播报：还剩 10/5/1 分钟各一次。
    void UpdateOpenReminders(bool announce);
    std::string BuildRegionNames() const;
    bool HasEnabledRegion() const;
    // 清空指定区域已刷新的全部入口（含已消耗待移除的），并释放其点位。
    void RemoveRegionEntrances(uint32 regionId);
    // 清空所有存在入口的区域；关闭窗口时调用。
    void RemoveAllEntrances();
    void BroadcastNotice(std::string const& message) const;
    // 开启/结束连发两条相同通知。
    void BroadcastOpenCloseNotice(std::string const& message) const;

    // 是否存在任一启用区域的有效入口数低于最小数量（用于触发立即补充）。
    bool HasRegionBelowMin() const;

    void RefreshAll(bool fillToMax);
    void RefreshRegion(RiftSpawnRegion const& region, bool fillToMax);
    bool SpawnOne(RiftSpawnRegion const& region, Map* map, uint8 tier, std::vector<uint32>& freePoints);
    RiftSpawnPoint const* GetPoint(uint32 pointId) const;

    std::map<uint32, RiftSpawnRegion> _regions;
    std::map<uint32, RiftSpawnPoint> _points;
    std::map<ObjectGuid, EntranceEntry> _entrances;
    std::vector<RiftScheduleWindow> _schedule;
    bool _hasSchedule = false;
    bool _windowOpen = true;
    bool _windowInitialized = false;
    uint32 _refillTimer = EntranceRefillIntervalMilliseconds;
    uint32 _scheduleTimer = 0;     // 时间表评估计时器：避免每帧计算时间
    int64 _cachedNextOpenTime = 0; // 已播报过的下次开启时刻，用于重置倒计时阶段
    uint32 _openReminderStage = 0; // 0=未播报, 1=已播报10分钟, 2=已播报5分钟, 3=已播报1分钟
    uint64 _nextToken = 1;
};

class BossAIBase;

uint32 GetExitPortalEntryForTier(uint8 tier);
bool IsExitPortalEntry(uint32 entry);
uint32 GetEntranceEntryForTier(uint8 tier);
uint32 GetEntranceAuraForTier(uint8 tier);
uint8 GetTierForEntranceEntry(uint32 entry);
uint8 GetTierForCreature(Creature const* creature);
TierConfig const* GetTierConfigForCreature(Creature const* creature);
void ApplyTierStats(Creature* creature, TierConfig const& config, uint32 baseHealth);

} // namespace HeroicDungeonRift

#endif
