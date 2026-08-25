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
    std::string icon;
    uint8 quality = 0;
    uint32 itemLevel = 0;
    uint32 gearScore = 0;
    uint32 count = 0;
    uint32 durability = 0;
    uint32 maxDurability = 0;
};

struct BotEquipmentCandidate
{
    ObjectGuid::LowType itemGuidLow = 0;
    uint32 itemEntry = 0;
    std::string itemLink;
    std::string icon;
    uint8 quality = 0;
    uint32 itemLevel = 0;
    uint32 gearScore = 0;
    uint32 count = 0;
    uint32 durability = 0;
    uint32 maxDurability = 0;
    uint8 bag = 0;
    uint8 bagSlot = 0;
};

struct BotEquipmentSnapshot
{
    uint32 botEntry = 0;
    ObjectGuid::LowType botGuidLow = 0;
    std::string revision;
    std::array<BotEquipmentSlotSnapshot, BOT_INVENTORY_SIZE> slots{};
};

struct BotEquipmentCandidatesResult
{
    BotEquipmentSnapshot snapshot;
    std::vector<BotEquipmentCandidate> candidates;
    bool truncated = false;
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
    static std::string_view GetResultMessage(BotEquipmentUiResult result);

private:
    static std::string CalculateRevision(bot_ai const* ai);
    static void BuildSnapshot(
        Player const* player,
        Creature const* bot,
        bot_ai const* ai,
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
