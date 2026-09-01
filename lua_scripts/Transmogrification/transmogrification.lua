-- ╔════════════════════════════════════════════════════════════════════════╗
-- ║           由 DanielTheDeveloper 编写的 Eluna 幻化系统脚本           ║
-- ╚════════════════════════════════════════════════════════════════════════╝
--
--                        ╔══════════════════════════╗
-- ╔══════════════════════║      幻化系统设置      ║══════════════════════╗
-- ║                      ╚══════════════════════════╝                      ║
-- ║ 首次装备物品时，自动将该物品的幻化外观添加到玩家的账号级幻化收藏中。 ║
-- ║                                                                        ║
-- ║ 建议启用此选项。                                                        ║
-- ╟────────────────────────────────────────────────────────────────────────╢
      local ADD_NEWLY_EQUIPPED_ITEMS_TO_THE_TRANSMOG_LIST = true          --║
-- ╟────────────────────────────────────────────────────────────────────────╢
-- ║                                                                        ║
-- ║ 首次拾取物品时，自动将该物品的幻化外观添加到玩家的账号级幻化收藏中。   ║
-- ║                                                                        ║
-- ║ 建议禁用此选项，因为它可能有助于在拍卖行中形成更健康的幻化经济。       ║
-- ╟────────────────────────────────────────────────────────────────────────╢
      local ADD_NEWLY_LOOTED_ITEMS_TO_THE_TRANSMOG_LIST = true           --║
-- ╟────────────────────────────────────────────────────────────────────────╢
-- ║                                                                        ║
-- ║ 完成任务时，无论玩家实际选择了哪件任务奖励，自动将所有适用的任务奖励物品 ║
-- ║ 的幻化外观添加到玩家的账号级幻化收藏中。                               ║
-- ║                                                                        ║
-- ║ 建议启用此选项，因为它避免了在潜在的幻化外观与角色实用装备之间做选择的困 ║
-- ║ 境。                                                                   ║
-- ╟────────────────────────────────────────────────────────────────────────╢
      local ADD_QUEST_REWARD_ITEMS_TO_THE_TRANSMOG_LIST = true            --║
-- ╟────────────────────────────────────────────────────────────────────────╢
-- ║                                                                        ║
-- ║ 将护甲幻化外观限制为使用相同材质的物品。例如，启用此选项后，布甲胸部装备 ║
-- ║ 只能幻化为其他布甲胸部装备的外观。                                     ║
-- ║ 禁用此选项后，布甲胸部装备可以幻化为布甲、皮甲、锁甲或板甲胸部装备的外观 ║
-- ║。                                                                       ║
-- ║                                                                        ║
-- ║ 建议启用此选项，因为这样可以保持职业特色。                             ║
-- ╟────────────────────────────────────────────────────────────────────────╢
      local RESTRICT_ARMOR_TRANSMOG_TO_SIMILAR_MATERIALS = false           --║
-- ╟────────────────────────────────────────────────────────────────────────╢
-- ║                                                                        ║
-- ║ 将武器幻化外观限制为相同类型的武器。例如，启用此选项后，双手剑只能幻化为 ║
-- ║ 其他双手剑的外观。禁用此选项后，双手剑可以幻化为单手剑、法杖、长柄武器 ║
-- ║、鱼竿等武器的外观。                                                     ║
-- ║                                                                        ║
-- ║ 建议启用此选项，因为这样可以保持职业特色。                             ║
-- ╟────────────────────────────────────────────────────────────────────────╢
      local RESTRICT_WEAPON_TRANSMOG_TO_SIMILAR_WEAPONS = false            --║
-- ╟────────────────────────────────────────────────────────────────────────╢
-- ║ 应用幻化时的金币费用，单位为铜币。                                    ║
-- ║ 武器和护甲分别配置，设置为 0 表示不收取费用。                          ║
-- ╟────────────────────────────────────────────────────────────────────────╢
      local WEAPON_TRANSMOG_COST = 500000                                     --║
      local ARMOR_TRANSMOG_COST = 250000                                      --║
-- ╚════════════════════════════════════════════════════════════════════════╝

local AIO = AIO or require("AIO")
local TransmogrificationHandler = AIO.AddHandlers("TransmogrificationServer", {})

local SLOTS = 6
local CALC = 281

local TRANSMOG_ERROR = {
	NONE = 0,
	INVALID_REQUEST = 1,
	NO_EQUIPMENT = 2,
	APPEARANCE_NOT_COLLECTED = 3,
	INCOMPATIBLE_ITEM = 4,
	INSUFFICIENT_MONEY = 5,
	OUTDATED_REQUEST = 6,
}

local PLAYER_VISIBLE_ITEM_1_ENTRYID = 283  -- Head
local PLAYER_VISIBLE_ITEM_3_ENTRYID = 287  -- Shoulder
local PLAYER_VISIBLE_ITEM_4_ENTRYID = 289  -- Shirt
local PLAYER_VISIBLE_ITEM_5_ENTRYID = 291  -- Chest
local PLAYER_VISIBLE_ITEM_6_ENTRYID = 293  -- Waist
local PLAYER_VISIBLE_ITEM_7_ENTRYID = 295  -- Legs
local PLAYER_VISIBLE_ITEM_8_ENTRYID = 297  -- Feet
local PLAYER_VISIBLE_ITEM_9_ENTRYID = 299  -- Wrist
local PLAYER_VISIBLE_ITEM_10_ENTRYID = 301 -- Hands
local PLAYER_VISIBLE_ITEM_15_ENTRYID = 311 -- Back
local PLAYER_VISIBLE_ITEM_16_ENTRYID = 313 -- Main
local PLAYER_VISIBLE_ITEM_17_ENTRYID = 315 -- Off
local PLAYER_VISIBLE_ITEM_18_ENTRYID = 317 -- Ranged
local PLAYER_VISIBLE_ITEM_19_ENTRYID = 319 -- Tabard
local UNUSABLE_INVENTORY_TYPES = {[2] = true, [11] = true, [12] = true, [18] = true, [24] = true, [27] = true, [28] = true}

-- TODO: Add further language support.
local localeMessages = {
	LOOT_ITEM_LOCALE = {
		[0] = " has been added to your appearance collection.", -- enUS/enGB
		[3] = " wurde deiner Transmog-Sammlung hinzugefügt.", -- deDE
		[4] = "已被添加到你的外观收藏中。", -- zhCN
	}
}

-- 账号级收藏缓存和角色幻化状态缓存。
local transmogCollectionCache = {}
local characterTransmogCache = {}
local characterTransmogLoaded = {}
local latestApplyRequestSerial = {}

local DISPLAY_SLOTS = {
	[PLAYER_VISIBLE_ITEM_1_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_3_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_4_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_5_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_6_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_7_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_8_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_9_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_10_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_15_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_16_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_17_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_18_ENTRYID] = true,
	[PLAYER_VISIBLE_ITEM_19_ENTRYID] = true,
}

local SLOT_INVENTORY_TYPES = {
	[PLAYER_VISIBLE_ITEM_1_ENTRYID] = { 1 },
	[PLAYER_VISIBLE_ITEM_3_ENTRYID] = { 3 },
	[PLAYER_VISIBLE_ITEM_4_ENTRYID] = { 4 },
	[PLAYER_VISIBLE_ITEM_5_ENTRYID] = { 5, 20 },
	[PLAYER_VISIBLE_ITEM_6_ENTRYID] = { 6 },
	[PLAYER_VISIBLE_ITEM_7_ENTRYID] = { 7 },
	[PLAYER_VISIBLE_ITEM_8_ENTRYID] = { 8 },
	[PLAYER_VISIBLE_ITEM_9_ENTRYID] = { 9 },
	[PLAYER_VISIBLE_ITEM_10_ENTRYID] = { 10 },
	[PLAYER_VISIBLE_ITEM_15_ENTRYID] = { 16 },
	[PLAYER_VISIBLE_ITEM_16_ENTRYID] = { 13, 17, 21 },
	[PLAYER_VISIBLE_ITEM_17_ENTRYID] = { 13, 17, 22, 23, 14 },
	[PLAYER_VISIBLE_ITEM_18_ENTRYID] = { 15, 25, 26 },
	[PLAYER_VISIBLE_ITEM_19_ENTRYID] = { 19 },
}

local function IsAllowedInventoryType(slot, inventoryType)
	for _, allowedType in ipairs(SLOT_INVENTORY_TYPES[slot] or {}) do
		if allowedType == inventoryType then
			return true
		end
	end
	return false
end

local function GetTransmogCost(equippedItem)
	if not equippedItem then
		return 0
	end
	if equippedItem:GetClass() == 2 then
		return WEAPON_TRANSMOG_COST
	elseif equippedItem:GetClass() == 4 then
		return ARMOR_TRANSMOG_COST
	end
	return 0
end

local GetTransmogCollectionCache
local GetCharacterTransmogCache

local function ValidateTransmogItem(player, itemID, slot)
	local numericSlot = tonumber(slot)
	local numericItemID = tonumber(itemID)
	if not numericSlot or not DISPLAY_SLOTS[numericSlot] or not numericItemID or numericItemID <= 0 then
		return nil, TRANSMOG_ERROR.INVALID_REQUEST
	end

	local state = GetCharacterTransmogCache(player)[numericSlot]
	local appearance = GetTransmogCollectionCache(player).appearances
	local collectedAppearance = nil
	local itemTemplate = GetItemTemplate(numericItemID)
	if not itemTemplate then
		return nil, TRANSMOG_ERROR.INVALID_REQUEST
	end

	local displayID = itemTemplate:GetDisplayId()
	for _, collected in pairs(appearance) do
		if collected.displayID == displayID then
			collectedAppearance = collected
			break
		end
	end
	if not collectedAppearance then
		return nil, TRANSMOG_ERROR.APPEARANCE_NOT_COLLECTED
	end
	if not state then
		return nil, TRANSMOG_ERROR.INVALID_REQUEST
	end

	local equipmentSlot = GetEquipmentSlot(numericSlot)
	local equippedItem = player:GetEquippedItemBySlot(equipmentSlot)
	if not equippedItem then
		return nil, TRANSMOG_ERROR.NO_EQUIPMENT
	end

	local equippedTemplate = equippedItem:GetItemTemplate()
	if not equippedTemplate then
		return nil, TRANSMOG_ERROR.NO_EQUIPMENT
	end
	local equippedClass = equippedTemplate:GetClass()
	local equippedInventoryType = equippedTemplate:GetInventoryType()
	local equippedSubType = equippedTemplate:GetSubClass()
	local itemClass = itemTemplate:GetClass()
	local inventoryType = itemTemplate:GetInventoryType()
	local itemSubType = itemTemplate:GetSubClass()
	if (itemClass ~= 2 and itemClass ~= 4) or itemClass ~= equippedClass then
		return nil, TRANSMOG_ERROR.INCOMPATIBLE_ITEM
	end
	if not IsAllowedInventoryType(numericSlot, inventoryType) then
		return nil, TRANSMOG_ERROR.INCOMPATIBLE_ITEM
	end
	if not IsAllowedInventoryType(numericSlot, equippedInventoryType) then
		return nil, TRANSMOG_ERROR.INCOMPATIBLE_ITEM
	end
	if itemClass == 4 and RESTRICT_ARMOR_TRANSMOG_TO_SIMILAR_MATERIALS and itemSubType ~= equippedSubType then
		return nil, TRANSMOG_ERROR.INCOMPATIBLE_ITEM
	end
	if itemClass == 2 and RESTRICT_WEAPON_TRANSMOG_TO_SIMILAR_WEAPONS and itemSubType ~= equippedSubType then
		return nil, TRANSMOG_ERROR.INCOMPATIBLE_ITEM
	end

	return state, equippedItem, itemTemplate, GetTransmogCost(equippedItem)
end

local function LoadTransmogCollectionCache(player)
	local accountGUID = player:GetAccountId()
	local cache = { appearances = {}, list = {} }
	local result = AuthDBQuery("SELECT unlocked_item_id, inventory_type, inventory_subtype, display_id, item_name FROM account_transmog WHERE account_id = " .. accountGUID .. " ORDER BY unlocked_item_id;")
	if result then
		for _ = 1, result:GetRowCount() do
			local row = result:GetRow()
			local displayID = tonumber(row["display_id"])
			local itemTemplate = GetItemTemplate(tonumber(row["unlocked_item_id"]))
			local itemQuality = itemTemplate and itemTemplate:GetQuality()
			if displayID and itemQuality and itemQuality >= 2 and not cache.appearances[displayID] then
				local appearance = {
					itemID = tonumber(row["unlocked_item_id"]),
					inventoryType = tonumber(row["inventory_type"]),
					inventorySubType = tonumber(row["inventory_subtype"]),
					displayID = displayID,
					itemName = row["item_name"] or "",
				}
				cache.appearances[displayID] = appearance
				table.insert(cache.list, appearance)
			end
			result:NextRow()
		end
	end
	transmogCollectionCache[accountGUID] = cache
	return cache
end

GetTransmogCollectionCache = function(player)
	local accountGUID = player:GetAccountId()
	return transmogCollectionCache[accountGUID] or LoadTransmogCollectionCache(player)
end

GetCharacterTransmogCache = function(player)
	local playerGUID = player:GetGUIDLow()
	if not characterTransmogCache[playerGUID] then
		characterTransmogCache[playerGUID] = {}
		for _, slot in ipairs({
			PLAYER_VISIBLE_ITEM_1_ENTRYID, PLAYER_VISIBLE_ITEM_3_ENTRYID, PLAYER_VISIBLE_ITEM_4_ENTRYID,
			PLAYER_VISIBLE_ITEM_5_ENTRYID, PLAYER_VISIBLE_ITEM_6_ENTRYID, PLAYER_VISIBLE_ITEM_7_ENTRYID,
			PLAYER_VISIBLE_ITEM_8_ENTRYID, PLAYER_VISIBLE_ITEM_9_ENTRYID, PLAYER_VISIBLE_ITEM_10_ENTRYID,
			PLAYER_VISIBLE_ITEM_15_ENTRYID, PLAYER_VISIBLE_ITEM_16_ENTRYID, PLAYER_VISIBLE_ITEM_17_ENTRYID,
			PLAYER_VISIBLE_ITEM_18_ENTRYID, PLAYER_VISIBLE_ITEM_19_ENTRYID
		}) do
			characterTransmogCache[playerGUID][slot] = { item = nil, realItem = nil }
		end
	end
	return characterTransmogCache[playerGUID]
end

local function SaveCharacterTransmog(player, slot, item, realItem)
	local playerGUID = player:GetGUIDLow()
	local numericSlot = tonumber(slot)
	if not DISPLAY_SLOTS[numericSlot] then
		return
	end
	local values = item and tostring(item) or "NULL"
	local realValues = realItem and tostring(realItem) or "NULL"
	CharDBQuery("INSERT INTO character_transmog (player_guid, slot, item, real_item) VALUES (" .. playerGUID .. ", " .. numericSlot .. ", " .. values .. ", " .. realValues .. ") ON DUPLICATE KEY UPDATE item = VALUES(item), real_item = VALUES(real_item);")
end

local function LoadCharacterTransmogCache(player)
	local playerGUID = player:GetGUIDLow()
	local cache = GetCharacterTransmogCache(player)
	if characterTransmogLoaded[playerGUID] then
		return cache
	end
	for _, state in pairs(cache) do
		state.item = nil
		state.realItem = nil
	end
	local result = CharDBQuery("SELECT item, real_item, slot FROM character_transmog WHERE player_guid = " .. playerGUID .. ";")
	if result then
		for _ = 1, result:GetRowCount() do
			local row = result:GetRow()
			local slot = tonumber(row["slot"])
			if cache[slot] then
				cache[slot].item = tonumber(row["item"])
				cache[slot].realItem = tonumber(row["real_item"])
			end
			result:NextRow()
		end
	end
	characterTransmogLoaded[playerGUID] = true
	return cache
end

local function GetLocalizedMessage(messageID, locale, ...)
	 local message = localeMessages[messageID][locale] or localeMessages[messageID][0]
	 if select("#", ...) > 0 then
		  return string.format(message, ...)
	 end
	 return message
end

function Transmog_CalculateSlot(slot)
	if (slot == 0) then
		slot = 1
	elseif (slot >= 2) then
		slot = slot + 1
	end
	return CALC + (slot * 2);
end

function Transmog_CalculateSlotReverse(slot)
	local reverseSlot = (slot - CALC) / 2
	if (reverseSlot == 1) then
		return 0;
	end
	return reverseSlot;
end

function Transmog_OnCharacterCreate(event, player)
	GetCharacterTransmogCache(player)
end

function Transmog_OnCharacterDelete(event, guid)
	CharDBQuery("DELETE FROM character_transmog WHERE player_guid = " .. guid)
	characterTransmogCache[guid] = nil
end

function Transmog_OnCharacterLogin(event, player)
	LoadTransmogCollectionCache(player)
	LoadCharacterTransmogCache(player)
end

function Transmog_OnLogout(event, player)
	local accountGUID = player:GetAccountId()
	local playerGUID = player:GetGUIDLow()
	transmogCollectionCache[accountGUID] = nil
	characterTransmogCache[playerGUID] = nil
	characterTransmogLoaded[playerGUID] = nil
end

function TransmogrificationHandler.LootItemLocale(player, item, count, locale)
	local accountGUID = player:GetAccountId()
	local itemID
	local itemTemplate

	if type(item) == "number" then
		itemID = item
		itemTemplate = GetItemTemplate(itemID)
	else
		itemTemplate = item:GetItemTemplate()
		if not itemTemplate then
			return
		end
		itemID = itemTemplate:GetItemId()
	end
	if not itemTemplate then
		return
	end

	local inventoryType = itemTemplate:GetInventoryType()
	local inventorySubType = itemTemplate:GetSubClass()
	local class = itemTemplate:GetClass()
	local itemQuality = itemTemplate:GetQuality()

	-- 灰色和白色物品不加入账号幻化收藏。
	if itemQuality == nil or itemQuality <= 1 then
		return
	end

	if (class == 2 or class == 4) and not UNUSABLE_INVENTORY_TYPES[inventoryType] then
		local collectionCache = GetTransmogCollectionCache(player)
		local displayID = itemTemplate:GetDisplayId()
		local displayExists = collectionCache.appearances[displayID] ~= nil
		
		if not displayExists then
			local itemName = itemTemplate:GetName()
			local locItemName = itemTemplate:GetName(locale)
			local itemQuality = itemTemplate:GetQuality()
			
			itemName = itemName:gsub("'", "''")
			AuthDBQuery("INSERT IGNORE INTO `account_transmog` (`account_id`, `unlocked_item_id`, `inventory_type`, `inventory_subtype`,`display_id`, `item_name`) VALUES (" .. accountGUID .. ", " .. itemID .. ", " .. inventoryType .. ", " .. inventorySubType .. ", " .. displayID .. ", '" .. itemName .. "');")
			local appearance = {
				itemID = itemID,
				inventoryType = inventoryType,
				inventorySubType = inventorySubType,
				displayID = displayID,
				itemName = itemName,
			}
			collectionCache.appearances[displayID] = appearance
			table.insert(collectionCache.list, appearance)

			if locItemName == nil then
				locItemName = itemTemplate:GetName(0)
			end
			
			local qualityColors = {
				[0] = "|cff9d9d9d", -- Poor
				[1] = "|cffffffff", -- Common
				[2] = "|cff1eff00", -- Uncommon
				[3] = "|cff0070dd", -- Rare
				[4] = "|cffa335ee", -- Epic
				[5] = "|cffff8000", -- Legendary
				[6] = "|cffe6cc80"  -- Heirloom
			}
			
			local colorCode = qualityColors[itemQuality] or "|cffffffff" -- Default to white if quality not found
			
			local itemLink = "|Hitem:" .. itemID .. ":0:0:0:0:0:0:0:0|h" .. colorCode .. "[" .. locItemName .. "]|r|h|r"
			player:SendBroadcastMessage(itemLink .. GetLocalizedMessage("LOOT_ITEM_LOCALE", locale))
		end
	end
end

function GetDisplaySlotForEquipmentSlot(equipSlot)
	local slotMapping = {
		[0] = PLAYER_VISIBLE_ITEM_1_ENTRYID,    -- Head
		[2] = PLAYER_VISIBLE_ITEM_3_ENTRYID,    -- Shoulders
		[3] = PLAYER_VISIBLE_ITEM_4_ENTRYID,    -- Shirt
		[4] = PLAYER_VISIBLE_ITEM_5_ENTRYID,    -- Chest
		[5] = PLAYER_VISIBLE_ITEM_6_ENTRYID,    -- Waist
		[6] = PLAYER_VISIBLE_ITEM_7_ENTRYID,    -- Legs
		[7] = PLAYER_VISIBLE_ITEM_8_ENTRYID,    -- Feet
		[8] = PLAYER_VISIBLE_ITEM_9_ENTRYID,    -- Wrists
		[9] = PLAYER_VISIBLE_ITEM_10_ENTRYID,   -- Hands
		[14] = PLAYER_VISIBLE_ITEM_15_ENTRYID,  -- Back
		[15] = PLAYER_VISIBLE_ITEM_16_ENTRYID,  -- Main Hand
		[16] = PLAYER_VISIBLE_ITEM_17_ENTRYID,  -- Off Hand
		[17] = PLAYER_VISIBLE_ITEM_18_ENTRYID,  -- Ranged
		[18] = PLAYER_VISIBLE_ITEM_19_ENTRYID   -- Tabard
	}
	
	return slotMapping[equipSlot]
end

function Transmog_OnEquipItem(event, player, item, bag, slot)
	local itemTemplate = item:GetItemTemplate()
	if not itemTemplate then
		return
	end
	local itemID = itemTemplate:GetItemId()
	local locale = player:GetDbLocaleIndex()
	
	if ADD_NEWLY_EQUIPPED_ITEMS_TO_THE_TRANSMOG_LIST then
		TransmogrificationHandler.LootItemLocale(player, itemID, 1, locale)
	end
	
	local class = itemTemplate:GetClass()
	local inventoryType = itemTemplate:GetInventoryType()
	
	if (class == 2 or class == 4) and not UNUSABLE_INVENTORY_TYPES[inventoryType] then
		local playerGUID = player:GetGUIDLow()
		local displaySlot = GetDisplaySlotForEquipmentSlot(slot)

		if displaySlot then
			local state = GetCharacterTransmogCache(player)[displaySlot]
			if state then
				state.realItem = itemID
				if state.item then
					SaveCharacterTransmog(player, displaySlot, state.item, itemID)
				end
			end
		end
	end
end

function Transmog_OnLootItem(event, player, item, count)
	if not item or not item:IsSoulBound() or item:CanBeTraded() then
		return
	end

	local locale = player:GetDbLocaleIndex()
	TransmogrificationHandler.LootItemLocale(player, item, 1, locale)
end

function Transmog_OnQuestComplete(event, player, quest)
	local questID = quest:GetId()
	local locale = player:GetDbLocaleIndex()
	
	local questRewardsQuery = WorldDBQuery("SELECT RewardItem1, RewardItem2, RewardItem3, RewardItem4, RewardChoiceItemID1, RewardChoiceItemID2, RewardChoiceItemID3, RewardChoiceItemID4, RewardChoiceItemID5, RewardChoiceItemID6 FROM quest_template WHERE ID = " .. questID .. ";")
	
	if not questRewardsQuery then
		return
	end
	
	for i = 0, 9 do
		local itemID = questRewardsQuery:GetUInt32(i)
		if itemID and itemID > 0 then
			TransmogrificationHandler.LootItemLocale(player, itemID, 1, locale)
		end
	end
end

function GetEquipmentSlot(displaySlot)
	-- Map from display/visual slots to actual equipment slots
	local slotMapping = {
		[PLAYER_VISIBLE_ITEM_1_ENTRYID] = 0,     -- Head (0 in equipment slot system)
		[PLAYER_VISIBLE_ITEM_3_ENTRYID] = 2,     -- Shoulders
		[PLAYER_VISIBLE_ITEM_4_ENTRYID] = 3,     -- Shirt
		[PLAYER_VISIBLE_ITEM_5_ENTRYID] = 4,     -- Chest
		[PLAYER_VISIBLE_ITEM_6_ENTRYID] = 5,     -- Waist
		[PLAYER_VISIBLE_ITEM_7_ENTRYID] = 6,     -- Legs
		[PLAYER_VISIBLE_ITEM_8_ENTRYID] = 7,     -- Feet
		[PLAYER_VISIBLE_ITEM_9_ENTRYID] = 8,     -- Wrists
		[PLAYER_VISIBLE_ITEM_10_ENTRYID] = 9,    -- Hands
		[PLAYER_VISIBLE_ITEM_15_ENTRYID] = 14,   -- Back/Cloak
		[PLAYER_VISIBLE_ITEM_16_ENTRYID] = 15,   -- Main Hand
		[PLAYER_VISIBLE_ITEM_17_ENTRYID] = 16,   -- Off Hand
		[PLAYER_VISIBLE_ITEM_18_ENTRYID] = 17,   -- Ranged
		[PLAYER_VISIBLE_ITEM_19_ENTRYID] = 18,   -- Tabard
	}
	
	return slotMapping[displaySlot] or Transmog_CalculateSlotReverse(displaySlot)
end

-- TODO: add lua/c++ function for unequip!!
function TransmogrificationHandler.OnUnequipItem(player)
	-- 检查所有显示槽位，清理已经卸下装备的幻化。
	local slots = {
		PLAYER_VISIBLE_ITEM_1_ENTRYID,  -- Head
		PLAYER_VISIBLE_ITEM_3_ENTRYID,  -- Shoulder
		PLAYER_VISIBLE_ITEM_4_ENTRYID,  -- Shirt
		PLAYER_VISIBLE_ITEM_5_ENTRYID,  -- Chest
		PLAYER_VISIBLE_ITEM_6_ENTRYID,  -- Waist
		PLAYER_VISIBLE_ITEM_7_ENTRYID,  -- Legs
		PLAYER_VISIBLE_ITEM_8_ENTRYID,  -- Feet
		PLAYER_VISIBLE_ITEM_9_ENTRYID,  -- Wrist
		PLAYER_VISIBLE_ITEM_10_ENTRYID, -- Hands
		PLAYER_VISIBLE_ITEM_15_ENTRYID, -- Back
		PLAYER_VISIBLE_ITEM_16_ENTRYID, -- Main
		PLAYER_VISIBLE_ITEM_17_ENTRYID, -- Off
		PLAYER_VISIBLE_ITEM_18_ENTRYID, -- Ranged
		PLAYER_VISIBLE_ITEM_19_ENTRYID  -- Tabard
	}
	
	for _, slot in ipairs(slots) do
		-- Get the corresponding equipment slot
		local equipmentSlot = GetEquipmentSlot(slot)
		
		-- Check if this slot has an item equipped
		local currentItem = player:GetEquippedItemBySlot(equipmentSlot)
		
		-- If the slot is empty but we have a transmog value, we need to clear it
		if not currentItem then
			local state = GetCharacterTransmogCache(player)[slot]
			if state and state.item then
				state.item = nil
				state.realItem = nil
				SaveCharacterTransmog(player, slot, nil, nil)
				player:SetUInt32Value(tonumber(slot), 0)
				AIO.Handle(player, "TransmogrificationServer", "ClearSlotTransmogrification", slot)
			end
		end
	end
end

function Transmog_Load(player)
	local cache = LoadCharacterTransmogCache(player)
	for slot, state in pairs(cache) do
		local equipmentSlot = GetEquipmentSlot(tonumber(slot))
		local equippedItem = player:GetEquippedItemBySlot(equipmentSlot)
		local actualItemID = equippedItem and equippedItem:GetItemTemplate():GetItemId() or 0
		player:SetUInt32Value(tonumber(slot), state.item or actualItemID)
	end
end

function Transmog_OnLogin(event, player)
	-- Apply transmog on login
	-- Transmog_Load(player)
	--local item = player:GetEquippedItemBySlot(4)
	--print(item:GetName())
end

function TransmogrificationHandler.LoadPlayer(player)
	Transmog_Load(player)
	player:SetUInt32Value(147, 1) -- use unit padding
end

function TransmogrificationHandler.EquipTransmogItem(player, item, slot, requestSerial)
	local numericSlot = tonumber(slot)
	local numericItem = tonumber(item)
	local numericRequestSerial = tonumber(requestSerial) or 0
	local playerGUID = player:GetGUIDLow()
	local latestRequestSerial = latestApplyRequestSerial[playerGUID] or 0
	if numericRequestSerial < latestRequestSerial then
		AIO.Handle(player, "TransmogrificationServer", "ApplyTransmogResult", numericSlot, false, -1, 0, numericRequestSerial, TRANSMOG_ERROR.OUTDATED_REQUEST)
		return
	end
	latestApplyRequestSerial[playerGUID] = numericRequestSerial
	local state = numericSlot and GetCharacterTransmogCache(player)[numericSlot]
	local equippedItem = numericSlot and player:GetEquippedItemBySlot(GetEquipmentSlot(numericSlot))
	local oldItemID = state and state.realItem or nil
	if equippedItem then
		local equippedTemplate = equippedItem:GetItemTemplate()
		if equippedTemplate then
			oldItemID = equippedTemplate:GetItemId()
		end
	end

	local function SendResult(success, appliedItemID, errorCode)
		AIO.Handle(player, "TransmogrificationServer", "ApplyTransmogResult", numericSlot, success, appliedItemID, oldItemID or 0, requestSerial, errorCode or TRANSMOG_ERROR.NONE)
	end

	if not numericSlot or not numericItem or not DISPLAY_SLOTS[numericSlot] then
		SendResult(false, -1, TRANSMOG_ERROR.INVALID_REQUEST)
		return
	end
	if not state or not equippedItem then
		SendResult(false, -1, TRANSMOG_ERROR.NO_EQUIPMENT)
		return
	end

	if numericItem == -1 then
		state.item = nil
		state.realItem = oldItemID
		SaveCharacterTransmog(player, numericSlot, nil, oldItemID)
		player:SetUInt32Value(numericSlot, oldItemID or 0)
		SendResult(true, -1)
		return
	end

	if numericItem == 0 then
		state.item = 0
		state.realItem = oldItemID
		SaveCharacterTransmog(player, numericSlot, 0, oldItemID)
		player:SetUInt32Value(numericSlot, 0)
		SendResult(true, 0)
		return
	end

	-- Eluna 单线程，handler 执行期间装备不可能被换掉，无需再比对"装备是否还是同一件"。
	local validatedState, validationError, _, transmogCost = ValidateTransmogItem(player, numericItem, numericSlot)
	if not validatedState then
		SendResult(false, -1, validationError or TRANSMOG_ERROR.INVALID_REQUEST)
		return
	end

	-- 请求的幻化目标与当前已应用的模型（或当前装备实物自身的模型）完全一致时，
	-- 属于无视觉变化的操作，无需扣费（避免重复累计）。
	if numericItem > 0 and (state.item == numericItem or (oldItemID and oldItemID == numericItem)) then
		SendResult(true, numericItem)
		return
	end

	if transmogCost > 0 then
		if player:GetCoinage() < transmogCost then
			SendResult(false, -1, TRANSMOG_ERROR.INSUFFICIENT_MONEY)
			return
		end
		player:ModifyMoney(-transmogCost)
	end

	state.item = numericItem
	state.realItem = oldItemID
	SaveCharacterTransmog(player, numericSlot, numericItem, oldItemID)
	player:SetUInt32Value(numericSlot, numericItem)
	SendResult(true, numericItem)
end

function TransmogrificationHandler.EquipAllTransmogItems(player, transmogPreview, requestSerial)
	if not transmogPreview then
		return
	end

	for slot, item in pairs(transmogPreview) do
		TransmogrificationHandler.EquipTransmogItem(player, item, slot, requestSerial)
	end
end

function TransmogrificationHandler.UnequipTransmogItem(player, slot)
	local numericSlot = tonumber(slot)
	local state = GetCharacterTransmogCache(player)[numericSlot]
	if not state then
		return
	end

	local equipmentSlot = GetEquipmentSlot(numericSlot)
	local currentItem = player:GetEquippedItemBySlot(equipmentSlot)
	local realItemID = 0
	if currentItem then
		local itemTemplate = currentItem:GetItemTemplate()
		realItemID = itemTemplate and itemTemplate:GetItemId() or 0
	end

	state.item = nil
	state.realItem = realItemID > 0 and realItemID or nil
	SaveCharacterTransmog(player, numericSlot, nil, state.realItem)
	player:SetUInt32Value(numericSlot, realItemID)
end

function TransmogrificationHandler.displayTransmog(player, spellid)
	AIO.Handle(player, "TransmogrificationServer", "TransmogrificationFrame")
	return false
end

function TransmogrificationHandler.Print(player, ...)
	print(...)
end

function TransmogrificationHandler.SetTransmogItemIDs(player)
	local cache = LoadCharacterTransmogCache(player)
	for slot in pairs(DISPLAY_SLOTS) do
		local state = cache[slot]
		local item = state and state.item or -1
		local realItem = state and state.realItem or 0
		AIO.Handle(player, "TransmogrificationServer", "SetTransmogItemIDClient", slot, item, realItem)
	end
end

function TransmogrificationHandler.SetCurrentSlotItemIDs(player, slot, page, requestSerial)
    slot = tonumber(slot)
    page = math.max(1, tonumber(page) or 1)
    local collectionCache = GetTransmogCollectionCache(player)

    -- Define inventory type mapping
    local inventoryTypesMapping = {
        [PLAYER_VISIBLE_ITEM_1_ENTRYID] = "= 1",
        [PLAYER_VISIBLE_ITEM_3_ENTRYID] = "= 3",
        [PLAYER_VISIBLE_ITEM_4_ENTRYID] = "= 4",
        [PLAYER_VISIBLE_ITEM_5_ENTRYID] = "IN (5, 20)",
        [PLAYER_VISIBLE_ITEM_6_ENTRYID] = "= 6",
        [PLAYER_VISIBLE_ITEM_7_ENTRYID] = "= 7",
        [PLAYER_VISIBLE_ITEM_8_ENTRYID] = "= 8",
        [PLAYER_VISIBLE_ITEM_9_ENTRYID] = "= 9",
        [PLAYER_VISIBLE_ITEM_10_ENTRYID] = "= 10",
        [PLAYER_VISIBLE_ITEM_15_ENTRYID] = "= 16",
        [PLAYER_VISIBLE_ITEM_16_ENTRYID] = "IN (13, 17, 21)",
        [PLAYER_VISIBLE_ITEM_17_ENTRYID] = "IN (13, 17, 22, 23, 14)",
        [PLAYER_VISIBLE_ITEM_18_ENTRYID] = "IN (15, 25, 26)",
        [PLAYER_VISIBLE_ITEM_19_ENTRYID] = "= 19"
    }

    -- Get the inventory type for the given slot
    local inventoryTypes = inventoryTypesMapping[slot]
    if not inventoryTypes then
        return -- Slot not valid, exit early
    end
    
    local equipmentSlot = GetEquipmentSlot(slot)
    local currentItem = player:GetEquippedItemBySlot(equipmentSlot)
    local equippedItemType = nil
    local equippedItemSubType = nil
    
    if currentItem then
        equippedItemType = currentItem:GetClass()
        equippedItemSubType = currentItem:GetSubClass()
    end

    -- Calculate page offset for pagination
    local pageOffset = (page > 1) and (SLOTS * (page - 1)) or 0
    

    local filteredItems = {}
    for _, appearance in ipairs(collectionCache.list) do
        local matchesSlot = false
        for inventoryType in string.gmatch(inventoryTypes, "%d+") do
            if appearance.inventoryType == tonumber(inventoryType) then
                matchesSlot = true
                break
            end
        end
        local matchesRestrictions = true
        if equippedItemType == 4 and RESTRICT_ARMOR_TRANSMOG_TO_SIMILAR_MATERIALS then
            matchesRestrictions = appearance.inventorySubType == equippedItemSubType
        elseif equippedItemType == 2 and RESTRICT_WEAPON_TRANSMOG_TO_SIMILAR_WEAPONS then
            matchesRestrictions = appearance.inventorySubType == equippedItemSubType
        end
        if matchesSlot and matchesRestrictions then
            table.insert(filteredItems, appearance.itemID)
        end
    end

    local currentSlotItemIDs = {}
    for index = pageOffset + 1, math.min(pageOffset + SLOTS, #filteredItems) do
        table.insert(currentSlotItemIDs, filteredItems[index])
    end
    local hasMorePages = #filteredItems > pageOffset + SLOTS

    -- Return the result to the player
    AIO.Handle(player, "TransmogrificationServer", "InitTab", currentSlotItemIDs, slot, page, hasMorePages, requestSerial)
    end

    function TransmogrificationHandler.SetSearchCurrentSlotItemIDs(player, slot, page, search, requestSerial)
	slot = tonumber(slot)
	page = math.max(1, tonumber(page) or 1)
	if search == nil or search == '' then
		return TransmogrificationHandler.SetCurrentSlotItemIDs(player, slot, page, requestSerial)
	end

	local collectionCache = GetTransmogCollectionCache(player)
	local normalizedSearch = string.lower(tostring(search))

	-- 定义部位对应的装备类型。

	-- Define slot-to-inventory type mapping
	local inventoryTypesMapping = {
		[PLAYER_VISIBLE_ITEM_1_ENTRYID] = "= 1",
		[PLAYER_VISIBLE_ITEM_3_ENTRYID] = "= 3",
		[PLAYER_VISIBLE_ITEM_4_ENTRYID] = "= 4",
		[PLAYER_VISIBLE_ITEM_5_ENTRYID] = "IN (5, 20)",
		[PLAYER_VISIBLE_ITEM_6_ENTRYID] = "= 6",
		[PLAYER_VISIBLE_ITEM_7_ENTRYID] = "= 7",
		[PLAYER_VISIBLE_ITEM_8_ENTRYID] = "= 8",
		[PLAYER_VISIBLE_ITEM_9_ENTRYID] = "= 9",
		[PLAYER_VISIBLE_ITEM_10_ENTRYID] = "= 10",
		[PLAYER_VISIBLE_ITEM_15_ENTRYID] = "= 16",
		[PLAYER_VISIBLE_ITEM_16_ENTRYID] = "IN (13, 17, 21)",
		[PLAYER_VISIBLE_ITEM_17_ENTRYID] = "IN (13, 17, 22, 23, 14)",
		[PLAYER_VISIBLE_ITEM_18_ENTRYID] = "IN (15, 25, 26)",
		[PLAYER_VISIBLE_ITEM_19_ENTRYID] = "= 19"
	}

	-- Get inventory type for the given slot
	local inventoryTypes = inventoryTypesMapping[slot]
	if not inventoryTypes then
		return -- Slot not valid
	end
	
	local equipmentSlot = GetEquipmentSlot(slot)
	local currentItem = player:GetEquippedItemBySlot(equipmentSlot)
	local equippedItemType = nil
	local equippedItemSubType = nil
	
	if currentItem then
		equippedItemType = currentItem:GetClass()
		equippedItemSubType = currentItem:GetSubClass()
	end

	-- Calculate page offset
	local pageOffset = (page > 1) and (SLOTS * (page - 1)) or 0

	local matchesRestrictions = function(appearance)
		if equippedItemType == 4 and RESTRICT_ARMOR_TRANSMOG_TO_SIMILAR_MATERIALS then
			return appearance.inventorySubType == equippedItemSubType
		elseif equippedItemType == 2 and RESTRICT_WEAPON_TRANSMOG_TO_SIMILAR_WEAPONS then
			return appearance.inventorySubType == equippedItemSubType
		end
		return true
	end
	local filteredItems = {}
	for _, appearance in ipairs(collectionCache.list) do
		local matchesSlot = false
		for inventoryType in string.gmatch(inventoryTypes, "%d+") do
			if appearance.inventoryType == tonumber(inventoryType) then
				matchesSlot = true
				break
			end
		end
		local searchableText = string.lower(tostring(appearance.itemName or ""))
		local matchesSearch = string.find(searchableText, normalizedSearch, 1, true) or string.find(tostring(appearance.displayID), normalizedSearch, 1, true)
		if matchesSlot and matchesSearch and matchesRestrictions(appearance) then
			table.insert(filteredItems, appearance.itemID)
		end
	end
	local currentSlotItemIDs = {}
	for index = pageOffset + 1, math.min(pageOffset + SLOTS, #filteredItems) do
		table.insert(currentSlotItemIDs, filteredItems[index])
	end
	AIO.Handle(player, "TransmogrificationServer", "InitTab", currentSlotItemIDs, slot, page, #filteredItems > pageOffset + SLOTS, requestSerial)
end

function TransmogrificationHandler.SetEquipmentTransmogInfo(player, slot, currentSlotTooltip)
	local state = GetCharacterTransmogCache(player)[tonumber(slot)]
	if state and state.item then
		AIO.Handle(player, "TransmogrificationServer", "SetEquipmentTransmogInfoClient", currentSlotTooltip)
	end
end

function TransmogrificationHandler.GetItemsWithSameAppearance(player, itemID)
	local itemTemplate = GetItemTemplate(tonumber(itemID))
	if not itemTemplate then
		return
	end

	local displayID = itemTemplate:GetDisplayId()
	local matchingItems = {}
	local collectionCache = GetTransmogCollectionCache(player)
	for appearanceID, appearance in pairs(collectionCache.appearances) do
		if appearanceID == displayID then
			table.insert(matchingItems, appearance.itemID)
		end
	end
	AIO.Handle(player, "TransmogrificationServer", "ReceiveMatchingAppearances", itemID, matchingItems)
end

RegisterPlayerEvent(1, Transmog_OnCharacterCreate)
RegisterPlayerEvent(2, Transmog_OnCharacterDelete)
RegisterPlayerEvent(3, Transmog_OnCharacterLogin)
RegisterPlayerEvent(4, Transmog_OnLogout)

RegisterPlayerEvent(29, Transmog_OnEquipItem)

RegisterPlayerEvent(30, function(event, player, bag, slot) 
    if bag == 255 then
        TransmogrificationHandler.OnUnequipItem(player)
    end
end)

if ADD_NEWLY_LOOTED_ITEMS_TO_THE_TRANSMOG_LIST then
	RegisterPlayerEvent(32, Transmog_OnLootItem)
	RegisterPlayerEvent(51, Transmog_OnLootItem)
	RegisterPlayerEvent(52, Transmog_OnLootItem)
	RegisterPlayerEvent(53, Transmog_OnLootItem)
	RegisterPlayerEvent(56, Transmog_OnLootItem)
end

if ADD_QUEST_REWARD_ITEMS_TO_THE_TRANSMOG_LIST then
	RegisterPlayerEvent(54, Transmog_OnQuestComplete)
end

print("[Eluna] Transmog System loaded successfully.")
