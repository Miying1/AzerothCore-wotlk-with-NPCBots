#ifndef BOT_MGR_SERVICE_H
#define BOT_MGR_SERVICE_H

#include "botcommon.h"

#include <array>
#include <string>
#include <string_view>
#include <vector>

class bot_ai;
class Creature;
class Item;
class Player;

struct EquipmentInfo;

enum class BotEquipmentUiResult : uint8
{
    Ok = 0,
    InvalidRequest,
    RateLimited,
    BotNotFound,
    DifferentMap,
    BotDead,
    NoPermission,
    InvalidSlot,
    BusyInCombat,
    ItemNotFound,
    ItemMoved,
    ItemMismatch,
    StaleEquipment,
    CantEquip,
    ItemConflict,
    NoBagSpace,
    NoBankSpace,
    InternalError
};

struct BotEquipmentSlotSnapshot
{
    uint8 slot = 0;
    bool occupied = false;
    ObjectGuid::LowType itemGuidLow = 0;
    uint32 itemEntry = 0;
    std::string itemLink;
    uint8 quality = 0;
    uint32 itemLevel = 0;
    uint32 gearScore = 0;
};

struct BotEquipmentCandidate
{
    ObjectGuid::LowType itemGuidLow = 0;
    uint32 itemEntry = 0;
    std::string itemLink;
    uint8 quality = 0;
    uint32 itemLevel = 0;
    uint32 gearScore = 0;
    uint8 bag = 0;
    uint8 bagSlot = 0;
};

struct BotEquipmentSnapshot
{
    uint32 botEntry = 0;
    ObjectGuid::LowType botGuidLow = 0;
    bool canManage = false;
    std::string ownerName;
    std::string revision;
    uint32 totalGearScore = 0;
    std::array<BotEquipmentSlotSnapshot, BOT_INVENTORY_SIZE> slots{};
};

struct BotEquipmentCandidatesResult
{
    BotEquipmentSnapshot snapshot;
    std::vector<BotEquipmentCandidate> candidates;
    bool truncated = false;
};

// 属性页只返回界面实际展示的数据；右侧属性由当前天赋归类决定。
struct BotAttributeSnapshot
{
    uint32 botEntry = 0;
    ObjectGuid::LowType botGuidLow = 0;
    bool canManage = false;
    uint8 spec = BOT_SPEC_DEFAULT;
    std::string category;

    uint32 maxHealth = 0;
    uint32 armor = 0;
    uint32 defense = 0;
    float dodge = 0.0f;
    float parry = 0.0f;
    float block = 0.0f;
    uint32 blockValue = 0;

    int32 attackPower = 0;
    float minDamage = 0.0f;
    float maxDamage = 0.0f;
    float damagePerSecond = 0.0f;
    float attackSpeed = 0.0f;
    float hit = 0.0f;
    float crit = 0.0f;
    int32 haste = 0;
    uint32 expertise = 0;
    float armorPenetration = 0.0f;

    uint32 spellPower = 0;
    int32 healingPower = 0;
    uint32 spellPenetration = 0;
    uint32 maxMana = 0;
    float manaRegenCasting = 0.0f;
    float manaRegenNotCasting = 0.0f;
};

// 管理页同时包含单 Bot 设置和主人 BotMgr 的队伍级设置。
struct BotManagementSnapshot
{
    uint32 botEntry = 0;
    ObjectGuid::LowType botGuidLow = 0;
    bool canManage = false;
    uint32 supportedRoles = BOT_ROLE_NONE;
    uint32 roles = BOT_ROLE_NONE;
    bool healThresholdSupported = false;
    uint8 healHealthThreshold = 95;
    uint32 engageDelayMs = 0;
    uint8 attackAngleMode = 1;
    bool combatPositioning = true;
};

class AC_GAME_API bot_mgr_service
{
public:
    static BotEquipmentUiResult GetSnapshot(
        Player* player,
        uint32 botEntry,
        ObjectGuid::LowType botGuidLow,
        BotEquipmentSnapshot& snapshot);

    static BotEquipmentUiResult GetCandidates(
        Player* player,
        uint32 botEntry,
        ObjectGuid::LowType botGuidLow,
        uint8 slot,
        BotEquipmentCandidatesResult& result);

    static BotEquipmentUiResult GetAttributes(
        Player* player,
        uint32 botEntry,
        ObjectGuid::LowType botGuidLow,
        BotAttributeSnapshot& snapshot);

    static BotEquipmentUiResult GetManagement(
        Player* player,
        uint32 botEntry,
        ObjectGuid::LowType botGuidLow,
        BotManagementSnapshot& snapshot);

    static BotEquipmentUiResult UpdateManagement(
        Player* player,
        uint32 botEntry,
        ObjectGuid::LowType botGuidLow,
        uint32 roles,
        uint32 healHealthThreshold,
        uint32 engageDelayMs,
        uint32 attackAngleMode,
        bool combatPositioning,
        BotManagementSnapshot& snapshot);

    static BotEquipmentUiResult EquipFromInventory(
        Player* player,
        uint32 botEntry,
        ObjectGuid::LowType botGuidLow,
        uint8 slot,
        ObjectGuid::LowType itemGuidLow,
        uint32 expectedItemEntry,
        std::string_view expectedRevision,
        bool storeReplacedToBank,
        BotEquipmentSnapshot& snapshot);

    static BotEquipmentUiResult Unequip(
        Player* player,
        uint32 botEntry,
        ObjectGuid::LowType botGuidLow,
        uint8 slot,
        std::string_view expectedRevision,
        bool storeToBank,
        BotEquipmentSnapshot& snapshot);

    static std::string_view GetResultCode(BotEquipmentUiResult result);

private:
    static std::string CalculateRevision(bot_ai const* ai);
    static uint32 CalculateTotalGearScore(Creature const* bot, bot_ai const* ai);
    static void BuildSnapshot(
        Player const* player,
        Creature const* bot,
        bot_ai const* ai,
        BotEquipmentSnapshot& snapshot);
    static void BuildOperationSnapshot(
        Player const* player,
        Creature const* bot,
        bot_ai const* ai,
        uint8 changedSlot,
        BotEquipmentSnapshot& snapshot);
    static bool IsStandardBotEquipment(EquipmentInfo const* equipmentInfo, Item const* item);

    static void FillItemValues(
        Player const* player,
        Creature const* bot,
        bot_ai const* ai,
        Item const* item,
        uint8 slot,
        BotEquipmentSlotSnapshot& output);

    static void FillItemValues(
        Player const* player,
        Creature const* bot,
        bot_ai const* ai,
        Item const* item,
        uint8 slot,
        uint8 bag,
        uint8 bagSlot,
        BotEquipmentCandidate& output);

    static bool IsCandidate(
        Player const* player,
        bot_ai const* ai,
        EquipmentInfo const* equipmentInfo,
        Item const* item,
        uint8 slot,
        uint8 bag,
        uint8 bagSlot);
};

#endif
