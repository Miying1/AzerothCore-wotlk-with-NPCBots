#include "bot_mgr_service.h"

#include "Bag.h"
#include "Creature.h"
#include "CreatureData.h"
#include "GameTime.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "SharedDefines.h"
#include "World.h"
#include "WorldSession.h"
#include "bot_ai.h"
#include "botconfig.h"
#include "botdatamgr.h"
#include "botgearscore.h"
#include "botmgr.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <list>
#include <mutex>
#include <ranges>
#include <sstream>
#include <unordered_map>

namespace
{
constexpr std::size_t MAX_CANDIDATES = 128;
constexpr uint32 CANDIDATE_REQUEST_INTERVAL_MS = 250;
constexpr uint32 OPERATION_REQUEST_INTERVAL_MS = 350;
constexpr uint32 RATE_LIMIT_ENTRY_TTL_MS = 60000;

struct RequestRateState
{
    uint32 candidateRequestAt = 0;
    uint32 operationRequestAt = 0;
    uint32 touchedAt = 0;
};

std::mutex requestRateMutex;
std::unordered_map<ObjectGuid::LowType, RequestRateState> requestRates;

enum class RequestKind : uint8
{
    Candidate,
    Operation
};

uint64 HashValue(uint64 hash, uint64 value)
{
    constexpr uint64 FNV_PRIME = 1099511628211ULL;

    for (uint8 index = 0; index != sizeof(value); ++index)
    {
        hash ^= uint8(value & 0xFF);
        hash *= FNV_PRIME;
        value >>= 8;
    }

    return hash;
}

std::string ToHex(uint64 value)
{
    std::array<char, 16> buffer{};
    auto const [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, 16);
    if (error != std::errc{})
        return {};

    return std::string(buffer.data(), end);
}

bool IsPlayerAvailable(Player const* player)
{
    return player && player->IsInWorld() && player->GetSession() && !player->GetSession()->isLogingOut();
}

bool IsRateLimited(Player const* player, RequestKind kind)
{
    if (!player)
        return true;

    ObjectGuid::LowType const playerGuidLow = player->GetGUID().GetCounter();
    uint32 const now = uint32(GameTime::GetGameTimeMS().count());
    uint32 const interval = kind == RequestKind::Candidate ? CANDIDATE_REQUEST_INTERVAL_MS : OPERATION_REQUEST_INTERVAL_MS;

    std::lock_guard lock(requestRateMutex);

    for (auto iterator = requestRates.begin(); iterator != requestRates.end();)
    {
        if (now - iterator->second.touchedAt > RATE_LIMIT_ENTRY_TTL_MS)
            iterator = requestRates.erase(iterator);
        else
            ++iterator;
    }

    RequestRateState& state = requestRates[playerGuidLow];
    uint32& lastRequestAt = kind == RequestKind::Candidate ? state.candidateRequestAt : state.operationRequestAt;
    state.touchedAt = now;

    if (lastRequestAt && now - lastRequestAt < interval)
        return true;

    lastRequestAt = now;
    return false;
}

Creature* FindVisibleBot(Player* player, uint32 botEntry, ObjectGuid::LowType botGuidLow)
{
    if (!player || !botEntry || !botGuidLow)
        return nullptr;

    // 主人的 BotMap 是最准确的快速路径；其他玩家只能读取当前视野范围内的 NPCBot。
    if (player->GetBotMgr())
    {
        if (BotMap const* bots = player->GetBotMgr()->GetBotMap())
        {
            auto const iterator = std::ranges::find_if(*bots, [botEntry, botGuidLow](BotMap::value_type const& entry)
            {
                Creature const* bot = entry.second;
                return bot && bot->GetEntry() == botEntry && entry.first.GetCounter() == botGuidLow &&
                    bot->GetGUID().GetCounter() == botGuidLow;
            });
            if (iterator != bots->end())
                return iterator->second;
        }
    }

    std::list<Creature*> visibleBots;
    player->GetCreatureListWithEntryInGrid(visibleBots, botEntry, player->GetSightRange());
    auto const iterator = std::ranges::find_if(visibleBots, [player, botGuidLow](Creature const* bot)
    {
        return bot && bot->GetGUID().GetCounter() == botGuidLow && player->CanSeeOrDetect(bot);
    });
    return iterator != visibleBots.end() ? *iterator : nullptr;
}

bool CanManageBot(Player const* player, bot_ai const* ai)
{
    return player && ai && ai->GetBotOwnerGuid() == player->GetGUID().GetCounter() && ai->GetBotOwner() == player;
}

BotEquipmentUiResult ValidateBotView(
    Player* player,
    uint32 botEntry,
    ObjectGuid::LowType botGuidLow,
    Creature*& bot,
    bot_ai*& ai)
{
    bot = nullptr;
    ai = nullptr;

    if (!IsPlayerAvailable(player) || !botEntry || !botGuidLow)
        return BotEquipmentUiResult::InvalidRequest;

    bot = FindVisibleBot(player, botEntry, botGuidLow);
    if (!bot || !bot->IsNPCBot() || !bot->IsInWorld() || bot->IsTempBot() || bot->IsSummon())
        return BotEquipmentUiResult::BotNotFound;

    ai = bot->GetBotAI();
    if (!ai || ai->IsTempBot() || ai->IsWanderer())
        return BotEquipmentUiResult::BotNotFound;

    return BotEquipmentUiResult::Ok;
}

BotEquipmentUiResult ValidateManagementOperation(
    Player* player,
    uint32 botEntry,
    ObjectGuid::LowType botGuidLow,
    Creature*& bot,
    bot_ai*& ai)
{
    BotEquipmentUiResult const result = ValidateBotView(player, botEntry, botGuidLow, bot, ai);
    if (result != BotEquipmentUiResult::Ok)
        return result;
    if (!CanManageBot(player, ai))
        return BotEquipmentUiResult::NoPermission;
    return player->GetBotMgr() ? BotEquipmentUiResult::Ok : BotEquipmentUiResult::InternalError;
}

BotEquipmentUiResult ValidateOperation(
    Player* player,
    uint32 botEntry,
    ObjectGuid::LowType botGuidLow,
    uint8 slot,
    Creature*& bot,
    bot_ai*& ai)
{
    if (slot >= BOT_INVENTORY_SIZE)
        return BotEquipmentUiResult::InvalidSlot;

    BotEquipmentUiResult const result = ValidateManagementOperation(player, botEntry, botGuidLow, bot, ai);
    if (result != BotEquipmentUiResult::Ok)
        return result;

    if (player->IsInCombat() || bot->IsInCombat())
        return BotEquipmentUiResult::BusyInCombat;

    return BotEquipmentUiResult::Ok;
}

uint32 GetSupportedManagementRoles(bot_ai const* ai)
{
    // 与现有 NPCBot 职责 Gossip 保持一致：除治疗外的四项主职责均可切换；治疗仅治疗职业可用。
    uint32 roles = BOT_ROLE_TANK | BOT_ROLE_TANK_OFF | BOT_ROLE_DPS | BOT_ROLE_RANGED;
    if (BotDataMgr::IsHealingClass(ai->GetBotClass()))
        roles |= BOT_ROLE_HEAL;
    return roles;
}

void BuildManagementSnapshot(Player const* player, Creature const* bot, bot_ai const* ai, BotManagementSnapshot& snapshot)
{
    snapshot = {};
    snapshot.botEntry = bot->GetEntry();
    snapshot.botGuidLow = bot->GetGUID().GetCounter();
    snapshot.canManage = CanManageBot(player, ai);
    snapshot.supportedRoles = GetSupportedManagementRoles(ai);
    snapshot.roles = ai->GetBotRoles() & BOT_ROLE_MASK_MAIN;
    snapshot.healThresholdSupported = ai->GetBotClass() != BOT_CLASS_SPHYNX &&
        BotDataMgr::IsHealingClass(ai->GetBotClass());
    snapshot.healHealthThreshold = ai->GetHealHpPctThreshold();
    snapshot.engageDelayMs = std::max(
        player->GetBotMgr()->GetEngageDelayDPS(), player->GetBotMgr()->GetEngageDelayHeal());
    snapshot.attackAngleMode = player->GetBotMgr()->GetBotAttackAngleMode();
    snapshot.combatPositioning = ai->GetAllowCombatPositioning();
}

bool IsAllowedInventoryPosition(uint8 bag, uint8 slot)
{
    if (bag == INVENTORY_SLOT_BAG_0)
        return slot >= INVENTORY_SLOT_ITEM_START && slot < INVENTORY_SLOT_ITEM_END;

    return bag >= INVENTORY_SLOT_BAG_START && bag < INVENTORY_SLOT_BAG_END;
}

template <typename Visitor>
void VisitPlayerInventoryItems(Player* player, Visitor&& visitor)
{
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot != INVENTORY_SLOT_ITEM_END; ++slot)
    {
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            visitor(item, INVENTORY_SLOT_BAG_0, slot);
    }

    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot != INVENTORY_SLOT_BAG_END; ++bagSlot)
    {
        Bag const* bag = player->GetBagByPos(bagSlot);
        if (!bag)
            continue;

        for (uint32 slot = 0; slot != bag->GetBagSize(); ++slot)
        {
            if (Item* item = player->GetItemByPos(bagSlot, uint8(slot)))
                visitor(item, bagSlot, uint8(slot));
        }
    }
}

Item* FindInventoryItem(Player* player, ObjectGuid::LowType itemGuidLow)
{
    Item* found = nullptr;
    VisitPlayerInventoryItems(player, [itemGuidLow, &found](Item* item, uint8, uint8)
    {
        if (!found && item->GetGUID().GetCounter() == itemGuidLow)
            found = item;
    });
    return found;
}

uint32 GetGearScore(bot_ai const* ai, Creature const* bot, ItemTemplate const* itemTemplate, uint8 slot)
{
    if (!ai || !bot || !itemTemplate)
        return 0;

    float const score = CalculateItemGearScore(
        itemTemplate,
        bot->GetEntry(),
        bot->GetLevel(),
        ai->GetBotClass(),
        ai->GetSpec(),
        slot);

    if (score <= 0.0f)
        return 0;
    if (score >= float(std::numeric_limits<uint32>::max()))
        return std::numeric_limits<uint32>::max();
    return uint32(score);
}

std::string_view GetAttributeCategory(bot_ai const* ai)
{
    uint8 const spec = ai->GetSpec();
    switch (spec)
    {
        case BOT_SPEC_PALADIN_HOLY:
        case BOT_SPEC_PRIEST_DISCIPLINE:
        case BOT_SPEC_PRIEST_HOLY:
        case BOT_SPEC_SHAMAN_RESTORATION:
        case BOT_SPEC_DRUID_RESTORATION:
            return "HEALING";
        case BOT_SPEC_HUNTER_BEASTMASTERY:
        case BOT_SPEC_HUNTER_MARKSMANSHIP:
        case BOT_SPEC_HUNTER_SURVIVAL:
            return "RANGED_PHYSICAL";
        case BOT_SPEC_PRIEST_SHADOW:
        case BOT_SPEC_SHAMAN_ELEMENTAL:
        case BOT_SPEC_MAGE_ARCANE:
        case BOT_SPEC_MAGE_FIRE:
        case BOT_SPEC_MAGE_FROST:
        case BOT_SPEC_WARLOCK_AFFLICTION:
        case BOT_SPEC_WARLOCK_DEMONOLOGY:
        case BOT_SPEC_WARLOCK_DESTRUCTION:
        case BOT_SPEC_DRUID_BALANCE:
            return "RANGED_SPELL";
        default:
            break;
    }

    // 低等级 Bot 可能尚无有效天赋，此时按职责和职业决定主要属性页。
    if (ai->HasRole(BOT_ROLE_HEAL))
        return "HEALING";
    if (ai->GetBotClass() == BOT_CLASS_HUNTER || ai->GetBotClass() == BOT_CLASS_DARK_RANGER)
        return "RANGED_PHYSICAL";
    if (ai->HasRole(BOT_ROLE_RANGED) || ai->GetBotClass() == BOT_CLASS_MAGE ||
        ai->GetBotClass() == BOT_CLASS_WARLOCK || ai->GetBotClass() == BOT_CLASS_ARCHMAGE ||
        ai->GetBotClass() == BOT_CLASS_NECROMANCER || ai->GetBotClass() == BOT_CLASS_SEA_WITCH)
    {
        return "RANGED_SPELL";
    }
    return "MELEE_PHYSICAL";
}

float GetAttackSpeedSeconds(Creature const* bot, WeaponAttackType attackType)
{
    // 与 bot_ai 现有属性显示保持一致，读取已经应用攻击速度修正后的字段值。
    return bot->GetFloatValue(static_cast<uint16>(UNIT_FIELD_BASEATTACKTIME) + attackType) / 1000.0f;
}

float GetDamagePerSecond(float minimum, float maximum, float attackSpeed)
{
    return attackSpeed > 0.0f ? ((minimum + maximum) * 0.5f) / attackSpeed : 0.0f;
}

}

void bot_mgr_service::FillItemValues(
    Player const* player,
    Creature const* bot,
    bot_ai const* ai,
    Item const* item,
    uint8 slot,
    BotEquipmentSlotSnapshot& output)
{
    output.slot = slot;
    if (!item)
        return;

    ItemTemplate const* itemTemplate = item->GetTemplate();
    std::ostringstream itemLink;
    ai->_AddItemLink(player, item, itemLink, false);

    output.occupied = true;
    output.itemGuidLow = item->GetGUID().GetCounter();
    output.itemEntry = item->GetEntry();
    output.itemLink = itemLink.str();
    output.quality = itemTemplate ? itemTemplate->Quality : 0;
    output.itemLevel = itemTemplate ? itemTemplate->ItemLevel : 0;
    output.gearScore = GetGearScore(ai, bot, itemTemplate, slot);
}

void bot_mgr_service::FillItemValues(
    Player const* player,
    Creature const* bot,
    bot_ai const* ai,
    Item const* item,
    uint8 slot,
    uint8 bag,
    uint8 bagSlot,
    BotEquipmentCandidate& output)
{
    ItemTemplate const* itemTemplate = item->GetTemplate();
    std::ostringstream itemLink;
    ai->_AddItemLink(player, item, itemLink, false);

    output.itemGuidLow = item->GetGUID().GetCounter();
    output.itemEntry = item->GetEntry();
    output.itemLink = itemLink.str();
    output.quality = itemTemplate ? itemTemplate->Quality : 0;
    output.itemLevel = itemTemplate ? itemTemplate->ItemLevel : 0;
    output.gearScore = GetGearScore(ai, bot, itemTemplate, slot);
    output.bag = bag;
    output.bagSlot = bagSlot;
}

uint32 bot_mgr_service::CalculateTotalGearScore(Creature const* bot, bot_ai const* ai)
{
    uint64 total = 0;
    for (uint8 slot = 0; slot != BOT_INVENTORY_SIZE; ++slot)
    {
        Item const* item = ai->GetEquips(slot);
        if (!item)
            continue;

        total += GetGearScore(ai, bot, item->GetTemplate(), slot);
        if (total >= std::numeric_limits<uint32>::max())
            return std::numeric_limits<uint32>::max();
    }
    return uint32(total);
}

std::string bot_mgr_service::CalculateRevision(bot_ai const* ai)
{
    constexpr uint64 FNV_OFFSET_BASIS = 14695981039346656037ULL;
    uint64 hash = FNV_OFFSET_BASIS;

    for (uint8 slot = 0; slot != BOT_INVENTORY_SIZE; ++slot)
    {
        hash = HashValue(hash, slot);

        Item const* item = ai->GetEquips(slot);
        if (!item)
        {
            hash = HashValue(hash, 0);
            continue;
        }

        hash = HashValue(hash, item->GetGUID().GetCounter());
        hash = HashValue(hash, item->GetEntry());
        hash = HashValue(hash, uint32(item->GetItemRandomPropertyId()));
        hash = HashValue(hash, item->GetItemSuffixFactor());
        hash = HashValue(hash, item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT));

        for (uint32 enchantmentSlot = SOCK_ENCHANTMENT_SLOT;
            enchantmentSlot != SOCK_ENCHANTMENT_SLOT + MAX_ITEM_PROTO_SOCKETS;
            ++enchantmentSlot)
        {
            hash = HashValue(hash, item->GetEnchantmentId(EnchantmentSlot(enchantmentSlot)));
        }
    }

    return ToHex(hash);
}

void bot_mgr_service::BuildSnapshot(Player const* player, Creature const* bot, bot_ai const* ai, BotEquipmentSnapshot& snapshot)
{
    snapshot = {};
    snapshot.botEntry = bot->GetEntry();
    snapshot.botGuidLow = bot->GetGUID().GetCounter();
    snapshot.canManage = CanManageBot(player, ai);
    snapshot.revision = CalculateRevision(ai);
    snapshot.totalGearScore = CalculateTotalGearScore(bot, ai);

    for (uint8 slot = 0; slot != BOT_INVENTORY_SIZE; ++slot)
        FillItemValues(player, bot, ai, ai->GetEquips(slot), slot, snapshot.slots[slot]);
}

void bot_mgr_service::BuildOperationSnapshot(
    Player const* player,
    Creature const* bot,
    bot_ai const* ai,
    uint8 changedSlot,
    BotEquipmentSnapshot& snapshot)
{
    snapshot = {};
    snapshot.botEntry = bot->GetEntry();
    snapshot.botGuidLow = bot->GetGUID().GetCounter();
    snapshot.canManage = CanManageBot(player, ai);
    snapshot.revision = CalculateRevision(ai);
    snapshot.totalGearScore = CalculateTotalGearScore(bot, ai);

    if (changedSlot == BOT_SLOT_MAINHAND || changedSlot == BOT_SLOT_OFFHAND)
    {
        FillItemValues(player, bot, ai, ai->GetEquips(BOT_SLOT_MAINHAND), BOT_SLOT_MAINHAND, snapshot.slots[BOT_SLOT_MAINHAND]);
        FillItemValues(player, bot, ai, ai->GetEquips(BOT_SLOT_OFFHAND), BOT_SLOT_OFFHAND, snapshot.slots[BOT_SLOT_OFFHAND]);
        return;
    }

    FillItemValues(player, bot, ai, ai->GetEquips(changedSlot), changedSlot, snapshot.slots[changedSlot]);
}

bool bot_mgr_service::IsStandardBotEquipment(EquipmentInfo const* equipmentInfo, Item const* item)
{
    if (!equipmentInfo || !item)
        return false;

    return std::ranges::any_of(equipmentInfo->ItemEntry, [item](uint32 itemEntry)
    {
        return itemEntry == item->GetEntry();
    });
}

bool bot_mgr_service::IsCandidate(
    Player const* player,
    bot_ai const* ai,
    EquipmentInfo const* equipmentInfo,
    Item const* item,
    uint8 slot,
    uint8 bag,
    uint8 bagSlot)
{
    if (!item || item->GetState() == ITEM_REMOVED || item->GetOwnerGUID() != player->GetGUID())
        return false;
    if (!IsAllowedInventoryPosition(bag, bagSlot) || item->GetBagSlot() != bag || item->GetSlot() != bagSlot)
        return false;
    if (item->IsInTrade() || IsStandardBotEquipment(equipmentInfo, item))
        return false;

    ItemTemplate const* itemTemplate = item->GetTemplate();
    return itemTemplate && ai->_canEquip(itemTemplate, slot, true, item);
}

namespace
{
BotEquipmentUiResult MapEquipResult(BotEquipResult result)
{
    switch (result)
    {
        case BotEquipResult::BOT_EQUIP_RESULT_OK:
            return BotEquipmentUiResult::Ok;
        case BotEquipResult::BOT_EQUIP_RESULT_FAIL_NO_BAG_SPACE:
            return BotEquipmentUiResult::NoBagSpace;
        case BotEquipResult::BOT_EQUIP_RESULT_FAIL_NO_BANK_SPACE:
            return BotEquipmentUiResult::NoBankSpace;
        case BotEquipResult::BOT_EQUIP_RESULT_FAIL_NO_ITEM:
            return BotEquipmentUiResult::ItemMoved;
        case BotEquipResult::BOT_EQUIP_RESULT_FAIL_SAME_ID:
        case BotEquipResult::BOT_EQUIP_RESULT_FAIL_CANT_EQUIP:
            return BotEquipmentUiResult::CantEquip;
        case BotEquipResult::BOT_EQUIP_RESULT_FAIL_ITEM_CONFLICT:
        case BotEquipResult::BOT_EQUIP_RESULT_FAIL_LINKED_UNEQUIP_FAILED:
        case BotEquipResult::BOT_EQUIP_RESULT_FAIL_LINKED_RESET_FAILED:
            return BotEquipmentUiResult::ItemConflict;
        case BotEquipResult::BOT_EQUIP_RESULT_FAIL_NO_RECEIVER:
        case BotEquipResult::BOT_EQUIP_RESULT_FAIL_INVALID_RECEIVER:
            return BotEquipmentUiResult::NoPermission;
        case BotEquipResult::BOT_EQUIP_RESULT_FAIL_WANDERER:
            return BotEquipmentUiResult::BotNotFound;
        default:
            return BotEquipmentUiResult::InternalError;
    }
}
}

BotEquipmentUiResult bot_mgr_service::GetSnapshot(
    Player* player,
    uint32 botEntry,
    ObjectGuid::LowType botGuidLow,
    BotEquipmentSnapshot& snapshot)
{
    snapshot = {};

    Creature* bot = nullptr;
    bot_ai* ai = nullptr;
    BotEquipmentUiResult const result = ValidateBotView(player, botEntry, botGuidLow, bot, ai);
    if (result != BotEquipmentUiResult::Ok)
        return result;

    BuildSnapshot(player, bot, ai, snapshot);
    return BotEquipmentUiResult::Ok;
}

BotEquipmentUiResult bot_mgr_service::GetAttributes(
    Player* player,
    uint32 botEntry,
    ObjectGuid::LowType botGuidLow,
    BotAttributeSnapshot& snapshot)
{
    snapshot = {};

    Creature* bot = nullptr;
    bot_ai* ai = nullptr;
    BotEquipmentUiResult const result = ValidateBotView(player, botEntry, botGuidLow, bot, ai);
    if (result != BotEquipmentUiResult::Ok)
        return result;

    snapshot.botEntry = bot->GetEntry();
    snapshot.botGuidLow = bot->GetGUID().GetCounter();
    snapshot.canManage = CanManageBot(player, ai);
    snapshot.spec = ai->GetSpec();
    snapshot.category = GetAttributeCategory(ai);

    snapshot.maxHealth = bot->GetMaxHealth();
    snapshot.armor = uint32(bot->GetArmor());
    snapshot.defense = uint32(bot->GetDefenseSkillValue());
    snapshot.dodge = ai->GetBotDodgeChance();
    snapshot.parry = ai->CanParry() ? ai->GetBotParryChance() : 0.0f;
    snapshot.block = ai->CanBlock() ? ai->GetBotBlockChance() : 0.0f;
    snapshot.blockValue = ai->CanBlock() ? ai->GetShieldBlockValue() : 0;

    WeaponAttackType const attackType = snapshot.category == "RANGED_PHYSICAL" ? RANGED_ATTACK : BASE_ATTACK;
    snapshot.attackPower = int32(bot->GetTotalAttackPowerValue(attackType));
    if (attackType == RANGED_ATTACK)
    {
        snapshot.minDamage = bot->GetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE);
        snapshot.maxDamage = bot->GetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE);
    }
    else
    {
        snapshot.minDamage = bot->GetFloatValue(UNIT_FIELD_MINDAMAGE);
        snapshot.maxDamage = bot->GetFloatValue(UNIT_FIELD_MAXDAMAGE);
    }
    snapshot.attackSpeed = GetAttackSpeedSeconds(bot, attackType);
    snapshot.damagePerSecond = GetDamagePerSecond(snapshot.minDamage, snapshot.maxDamage, snapshot.attackSpeed);
    snapshot.hit = -ai->GetBotMissChance();
    snapshot.crit = ai->GetBotCritChance();
    snapshot.haste = ai->GetHaste();
    snapshot.expertise = ai->GetBotExpertise() +
        uint32(std::max<int32>(0, bot->GetTotalAuraModifier(SPELL_AURA_MOD_EXPERTISE)));
    snapshot.armorPenetration = bot->GetCreatureArmorPenetrationCoef();

    snapshot.spellPower = ai->GetBotSpellPower();
    snapshot.healingPower = std::max<int32>(0, bot->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_MAGIC));
    snapshot.spellPenetration = ai->GetBotSpellPenetration() + uint32(std::abs(
        bot->GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_TARGET_RESISTANCE, SPELL_SCHOOL_MASK_MAGIC)));
    snapshot.maxMana = bot->GetMaxPower(POWER_MANA);
    if (snapshot.maxMana > 1)
    {
        snapshot.manaRegenCasting = bot->GetFloatValue(UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER) *
            sWorld->getRate(RATE_POWER_MANA) * 5.0f;
        snapshot.manaRegenNotCasting = bot->GetFloatValue(UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER) *
            sWorld->getRate(RATE_POWER_MANA) * 5.0f;
    }

    return BotEquipmentUiResult::Ok;
}

BotEquipmentUiResult bot_mgr_service::GetManagement(
    Player* player,
    uint32 botEntry,
    ObjectGuid::LowType botGuidLow,
    BotManagementSnapshot& snapshot)
{
    snapshot = {};

    Creature* bot = nullptr;
    bot_ai* ai = nullptr;
    BotEquipmentUiResult const result = ValidateManagementOperation(player, botEntry, botGuidLow, bot, ai);
    if (result != BotEquipmentUiResult::Ok)
        return result;

    BuildManagementSnapshot(player, bot, ai, snapshot);
    return BotEquipmentUiResult::Ok;
}

BotEquipmentUiResult bot_mgr_service::UpdateManagement(
    Player* player,
    uint32 botEntry,
    ObjectGuid::LowType botGuidLow,
    uint32 roles,
    uint32 healHealthThreshold,
    uint32 engageDelayMs,
    uint32 attackAngleMode,
    bool combatPositioning,
    BotManagementSnapshot& snapshot)
{
    snapshot = {};
    if ((roles & ~BOT_ROLE_MASK_MAIN) != 0 || healHealthThreshold < 1 || healHealthThreshold > 100 ||
        engageDelayMs > 10 * IN_MILLISECONDS ||
        (attackAngleMode != BOT_ATTACK_ANGLE_NORMAL && attackAngleMode != BOT_ATTACK_ANGLE_AVOID_FRONTAL_AOE))
    {
        return BotEquipmentUiResult::InvalidRequest;
    }

    if (IsRateLimited(player, RequestKind::Operation))
        return BotEquipmentUiResult::RateLimited;

    Creature* bot = nullptr;
    bot_ai* ai = nullptr;
    BotEquipmentUiResult const validation = ValidateManagementOperation(player, botEntry, botGuidLow, bot, ai);
    if (validation != BotEquipmentUiResult::Ok)
        return validation;

    uint32 const supportedRoles = GetSupportedManagementRoles(ai);
    if ((roles & ~supportedRoles) != 0)
        return BotEquipmentUiResult::InvalidRequest;
    if ((roles & (BOT_ROLE_TANK | BOT_ROLE_DPS | BOT_ROLE_HEAL)) == 0)
        return BotEquipmentUiResult::InvalidRequest;

    // 副坦克是主坦克的从属职责，与现有 ToggleRole 规则保持一致。
    if (roles & BOT_ROLE_TANK_OFF)
        roles |= BOT_ROLE_TANK;
    if (!(roles & BOT_ROLE_TANK))
        roles &= ~BOT_ROLE_TANK_OFF;

    auto setRole = [ai, roles](uint32 role)
    {
        if (ai->HasRole(role) != ((roles & role) != 0))
            ai->ToggleRole(role, true);
    };

    // 先处理主坦克，再单独收敛副坦克，避免 ToggleRole 的联动规则破坏最终状态。
    setRole(BOT_ROLE_TANK);
    setRole(BOT_ROLE_TANK_OFF);
    setRole(BOT_ROLE_DPS);
    setRole(BOT_ROLE_HEAL);
    setRole(BOT_ROLE_RANGED);

    if (BotDataMgr::IsHealingClass(ai->GetBotClass()) && ai->GetBotClass() != BOT_CLASS_SPHYNX)
        ai->SetHealHpPctThreshold(uint8(healHealthThreshold));

    // 管理页只有一个“进战延迟”，同时应用于攻击和治疗，确保纯治疗职责也生效。
    player->GetBotMgr()->SetEngageDelayDPS(engageDelayMs);
    player->GetBotMgr()->SetEngageDelayHeal(engageDelayMs);
    player->GetBotMgr()->SetBotAttackAngleMode(uint8(attackAngleMode));
    ai->SetCombatPositioningOverride(combatPositioning ? 1 : 0);
    player->SaveToDB(false, false);

    BuildManagementSnapshot(player, bot, ai, snapshot);
    return BotEquipmentUiResult::Ok;
}

BotEquipmentUiResult bot_mgr_service::GetCandidates(
    Player* player,
    uint32 botEntry,
    ObjectGuid::LowType botGuidLow,
    uint8 slot,
    BotEquipmentCandidatesResult& result)
{
    result = {};

    if (IsRateLimited(player, RequestKind::Candidate))
        return BotEquipmentUiResult::RateLimited;

    Creature* bot = nullptr;
    bot_ai* ai = nullptr;
    BotEquipmentUiResult const validation = ValidateOperation(player, botEntry, botGuidLow, slot, bot, ai);
    if (validation != BotEquipmentUiResult::Ok)
        return validation;

    EquipmentInfo const* equipmentInfo = BotDataMgr::GetBotEquipmentInfo(botEntry);
    if (!equipmentInfo)
        return BotEquipmentUiResult::InternalError;

    VisitPlayerInventoryItems(player, [&](Item* item, uint8 bag, uint8 bagSlot)
    {
        if (!IsCandidate(player, ai, equipmentInfo, item, slot, bag, bagSlot))
            return;

        BotEquipmentCandidate candidate;
        FillItemValues(player, bot, ai, item, slot, bag, bagSlot, candidate);
        result.candidates.push_back(std::move(candidate));
    });

    std::ranges::sort(result.candidates, [](BotEquipmentCandidate const& left, BotEquipmentCandidate const& right)
    {
        if (left.itemLevel != right.itemLevel)
            return left.itemLevel > right.itemLevel;
        if (left.gearScore != right.gearScore)
            return left.gearScore > right.gearScore;
        if (left.quality != right.quality)
            return left.quality > right.quality;
        if (left.bag != right.bag)
            return left.bag < right.bag;
        return left.bagSlot < right.bagSlot;
    });

    if (result.candidates.size() > MAX_CANDIDATES)
    {
        result.candidates.resize(MAX_CANDIDATES);
        result.truncated = true;
    }

    BuildSnapshot(player, bot, ai, result.snapshot);
    return BotEquipmentUiResult::Ok;
}

BotEquipmentUiResult bot_mgr_service::EquipFromInventory(
    Player* player,
    uint32 botEntry,
    ObjectGuid::LowType botGuidLow,
    uint8 slot,
    ObjectGuid::LowType itemGuidLow,
    uint32 expectedItemEntry,
    std::string_view expectedRevision,
    bool storeReplacedToBank,
    BotEquipmentSnapshot& snapshot)
{
    snapshot = {};

    if (!itemGuidLow || !expectedItemEntry || expectedRevision.empty())
        return BotEquipmentUiResult::InvalidRequest;
    if (IsRateLimited(player, RequestKind::Operation))
        return BotEquipmentUiResult::RateLimited;
    if (storeReplacedToBank && !BotCfg::IsGearBankEnabled())
        return BotEquipmentUiResult::InvalidRequest;

    Creature* bot = nullptr;
    bot_ai* ai = nullptr;
    BotEquipmentUiResult const validation = ValidateOperation(player, botEntry, botGuidLow, slot, bot, ai);
    if (validation != BotEquipmentUiResult::Ok)
        return validation;

    if (CalculateRevision(ai) != expectedRevision)
    {
        BuildSnapshot(player, bot, ai, snapshot);
        return BotEquipmentUiResult::StaleEquipment;
    }

    if (!ai->GetBotOwner() || ai->GetBotOwner() != player)
        return BotEquipmentUiResult::NoPermission;

    Item* item = FindInventoryItem(player, itemGuidLow);
    if (!item)
        return BotEquipmentUiResult::ItemNotFound;
    if (item->GetEntry() != expectedItemEntry)
        return BotEquipmentUiResult::ItemMismatch;
    if (item->GetState() == ITEM_REMOVED || item->GetOwnerGUID() != player->GetGUID() || item->IsInTrade() ||
        !IsAllowedInventoryPosition(item->GetBagSlot(), item->GetSlot()))
    {
        return BotEquipmentUiResult::ItemMoved;
    }

    EquipmentInfo const* equipmentInfo = BotDataMgr::GetBotEquipmentInfo(botEntry);
    if (!equipmentInfo)
        return BotEquipmentUiResult::InternalError;
    if (IsStandardBotEquipment(equipmentInfo, item) || !ai->_canEquip(item->GetTemplate(), slot, true, item))
        return BotEquipmentUiResult::CantEquip;

    BotEquipmentUiResult const result = MapEquipResult(ai->_equip(slot, item, player->GetGUID(), storeReplacedToBank));
    if (result == BotEquipmentUiResult::Ok)
        BuildOperationSnapshot(player, bot, ai, slot, snapshot);
    else
        BuildSnapshot(player, bot, ai, snapshot);
    return result;
}

BotEquipmentUiResult bot_mgr_service::Unequip(
    Player* player,
    uint32 botEntry,
    ObjectGuid::LowType botGuidLow,
    uint8 slot,
    std::string_view expectedRevision,
    bool storeToBank,
    BotEquipmentSnapshot& snapshot)
{
    snapshot = {};

    if (expectedRevision.empty())
        return BotEquipmentUiResult::InvalidRequest;
    if (IsRateLimited(player, RequestKind::Operation))
        return BotEquipmentUiResult::RateLimited;
    if (storeToBank && !BotCfg::IsGearBankEnabled())
        return BotEquipmentUiResult::InvalidRequest;

    Creature* bot = nullptr;
    bot_ai* ai = nullptr;
    BotEquipmentUiResult const validation = ValidateOperation(player, botEntry, botGuidLow, slot, bot, ai);
    if (validation != BotEquipmentUiResult::Ok)
        return validation;

    if (CalculateRevision(ai) != expectedRevision)
    {
        BuildSnapshot(player, bot, ai, snapshot);
        return BotEquipmentUiResult::StaleEquipment;
    }

    if (!ai->GetBotOwner() || ai->GetBotOwner() != player)
        return BotEquipmentUiResult::NoPermission;

    if (!ai->GetEquips(slot))
    {
        BuildSnapshot(player, bot, ai, snapshot);
        return BotEquipmentUiResult::ItemNotFound;
    }

    BotEquipmentUiResult const result = MapEquipResult(ai->_unequip(slot, player->GetGUID(), storeToBank));
    if (result == BotEquipmentUiResult::Ok && ai->GetBotOwner() && ai->GetBotOwner()->IsInWorld())
        ai->GetBotOwner()->SaveToDB(false, false);

    if (result == BotEquipmentUiResult::Ok)
        BuildOperationSnapshot(player, bot, ai, slot, snapshot);
    else
        BuildSnapshot(player, bot, ai, snapshot);
    return result;
}

std::string_view bot_mgr_service::GetResultCode(BotEquipmentUiResult result)
{
    switch (result)
    {
        case BotEquipmentUiResult::Ok: return "OK";
        case BotEquipmentUiResult::InvalidRequest: return "INVALID_REQUEST";
        case BotEquipmentUiResult::RateLimited: return "RATE_LIMITED";
        case BotEquipmentUiResult::BotNotFound: return "BOT_NOT_FOUND";
        case BotEquipmentUiResult::NoPermission: return "NO_PERMISSION";
        case BotEquipmentUiResult::InvalidSlot: return "INVALID_SLOT";
        case BotEquipmentUiResult::BusyInCombat: return "BUSY_IN_COMBAT";
        case BotEquipmentUiResult::ItemNotFound: return "ITEM_NOT_FOUND";
        case BotEquipmentUiResult::ItemMoved: return "ITEM_MOVED";
        case BotEquipmentUiResult::ItemMismatch: return "ITEM_MISMATCH";
        case BotEquipmentUiResult::StaleEquipment: return "STALE_EQUIPMENT";
        case BotEquipmentUiResult::CantEquip: return "CANT_EQUIP";
        case BotEquipmentUiResult::ItemConflict: return "ITEM_CONFLICT";
        case BotEquipmentUiResult::NoBagSpace: return "NO_BAG_SPACE";
        case BotEquipmentUiResult::NoBankSpace: return "NO_BANK_SPACE";
        case BotEquipmentUiResult::InternalError: return "INTERNAL_ERROR";
        default: return "INTERNAL_ERROR";
    }
}

std::string_view bot_mgr_service::GetResultMessage(BotEquipmentUiResult result)
{
    switch (result)
    {
        case BotEquipmentUiResult::Ok: return "操作成功";
        case BotEquipmentUiResult::InvalidRequest: return "请求参数无效";
        case BotEquipmentUiResult::RateLimited: return "操作过于频繁，请稍后重试";
        case BotEquipmentUiResult::BotNotFound: return "NPCBot 不存在或当前不可管理";
        case BotEquipmentUiResult::NoPermission: return "无权管理该 NPCBot";
        case BotEquipmentUiResult::InvalidSlot: return "NPCBot 装备槽无效";
        case BotEquipmentUiResult::BusyInCombat: return "战斗中不能管理 NPCBot 装备";
        case BotEquipmentUiResult::ItemNotFound: return "物品不存在或已离开背包";
        case BotEquipmentUiResult::ItemMoved: return "物品状态或背包位置已变化";
        case BotEquipmentUiResult::ItemMismatch: return "物品实例与请求不匹配";
        case BotEquipmentUiResult::StaleEquipment: return "NPCBot 装备状态已变化，请重新选择";
        case BotEquipmentUiResult::CantEquip: return "该 NPCBot 无法装备此物品";
        case BotEquipmentUiResult::ItemConflict: return "物品与当前主副手装备冲突";
        case BotEquipmentUiResult::NoBagSpace: return "背包空间不足";
        case BotEquipmentUiResult::NoBankSpace: return "NPCBot 装备银行空间不足";
        case BotEquipmentUiResult::InternalError: return "NPCBot 装备服务内部错误";
        default: return "NPCBot 装备服务内部错误";
    }
}
