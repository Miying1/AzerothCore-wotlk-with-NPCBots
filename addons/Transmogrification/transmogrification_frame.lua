-- 初始化 Ace3 库。
local addonName, addon = ...
local L = LibStub("AceLocale-3.0"):GetLocale("Transmogrification")

-- 建立 AIO 客户端协议。
local AIO = AIO or require("AIO")
if AIO.AddAddon() then
	return
end

-- 建立 AIO 处理函数表。
local TransmogrificationHandler = AIO.AddHandlers("TransmogrificationServer", {})

-- 定义幻化装备栏位引用。
PLAYER_VISIBLE_ITEM_1_ENTRYID  = 283 -- 头部
PLAYER_VISIBLE_ITEM_3_ENTRYID  = 287 -- 肩膀
PLAYER_VISIBLE_ITEM_4_ENTRYID  = 289 -- 衬衫
PLAYER_VISIBLE_ITEM_5_ENTRYID  = 291 -- 胸甲
PLAYER_VISIBLE_ITEM_6_ENTRYID  = 293 -- 腰带
PLAYER_VISIBLE_ITEM_7_ENTRYID  = 295 -- 腿部
PLAYER_VISIBLE_ITEM_8_ENTRYID  = 297 -- 脚部
PLAYER_VISIBLE_ITEM_9_ENTRYID  = 299 -- 手腕
PLAYER_VISIBLE_ITEM_10_ENTRYID = 301 -- 手套
PLAYER_VISIBLE_ITEM_15_ENTRYID = 311 -- 背部
PLAYER_VISIBLE_ITEM_16_ENTRYID = 313 -- 主手
PLAYER_VISIBLE_ITEM_17_ENTRYID = 315 -- 副手
PLAYER_VISIBLE_ITEM_18_ENTRYID = 317 -- 远程
PLAYER_VISIBLE_ITEM_19_ENTRYID = 319 -- 战袍

transmogrificationEquipmentSlotMap = {
	[PLAYER_VISIBLE_ITEM_1_ENTRYID]  = "Head",
	[PLAYER_VISIBLE_ITEM_3_ENTRYID]  = "Shoulder",
	[PLAYER_VISIBLE_ITEM_4_ENTRYID]  = "Shirt",
	[PLAYER_VISIBLE_ITEM_5_ENTRYID]  = "Chest",
	[PLAYER_VISIBLE_ITEM_6_ENTRYID]  = "Waist",
	[PLAYER_VISIBLE_ITEM_7_ENTRYID]  = "Legs",
	[PLAYER_VISIBLE_ITEM_8_ENTRYID]  = "Feet",
	[PLAYER_VISIBLE_ITEM_9_ENTRYID]  = "Wrist",
	[PLAYER_VISIBLE_ITEM_10_ENTRYID] = "Hands",
	[PLAYER_VISIBLE_ITEM_15_ENTRYID] = "Back",
	[PLAYER_VISIBLE_ITEM_16_ENTRYID] = "MainHand",
	[PLAYER_VISIBLE_ITEM_17_ENTRYID] = "SecondaryHand",
	[PLAYER_VISIBLE_ITEM_18_ENTRYID] = "Ranged",
	[PLAYER_VISIBLE_ITEM_19_ENTRYID] = "Tabard"
}

local equipmentSlotIDs = {
	Head = PLAYER_VISIBLE_ITEM_1_ENTRYID,
	Shoulder = PLAYER_VISIBLE_ITEM_3_ENTRYID,
	Shirt = PLAYER_VISIBLE_ITEM_4_ENTRYID,
	Chest = PLAYER_VISIBLE_ITEM_5_ENTRYID,
	Waist = PLAYER_VISIBLE_ITEM_6_ENTRYID,
	Legs = PLAYER_VISIBLE_ITEM_7_ENTRYID,
	Feet = PLAYER_VISIBLE_ITEM_8_ENTRYID,
	Wrist = PLAYER_VISIBLE_ITEM_9_ENTRYID,
	Hands = PLAYER_VISIBLE_ITEM_10_ENTRYID,
	Back = PLAYER_VISIBLE_ITEM_15_ENTRYID,
	MainHand = PLAYER_VISIBLE_ITEM_16_ENTRYID,
	SecondaryHand = PLAYER_VISIBLE_ITEM_17_ENTRYID,
	Ranged = PLAYER_VISIBLE_ITEM_18_ENTRYID,
	Tabard = PLAYER_VISIBLE_ITEM_19_ENTRYID,
}

local characterEquipmentSlotNames = {
	"CharacterHeadSlot",
	"CharacterShoulderSlot",
	"CharacterBackSlot",
	"CharacterChestSlot",
	"CharacterShirtSlot",
	"CharacterTabardSlot",
	"CharacterWristSlot",
	"CharacterHandsSlot",
	"CharacterWaistSlot",
	"CharacterLegsSlot",
	"CharacterFeetSlot",
	"CharacterMainHandSlot",
	"CharacterSecondaryHandSlot",
	"CharacterRangedSlot"
}

local equipmentSlotIcons = {
	"Head",
	"",			-- 颈部
	"Shoulder",
	"Shirt",
	"Chest",	-- 胸甲
	"Waist",
	"Legs",
	"Feet",
	"Wrists",
	"Hands",
	"",			-- 戒指 1
	"",			-- 戒指 2
	"",			-- 饰品 1
	"",			-- 饰品 2
	"Chest",	-- 长袍
	"MainHand",
	"SecondaryHand",
	"Ranged",
	"Tabard"
}

-- 定义幻化窗口变量。
local itemButtons = {}
local isInputHovered = false
local isTooltipHooked = false
local CurrentItemSlot = PLAYER_VISIBLE_ITEM_1_ENTRYID
local currentPage = 1
local currentSearchText = ""
local listRequestSerial = 0
local latestListRequestSerial = 0
local applyRequestSerial = 0
local activeApplyRequestSerial = 0
local pendingApplyCount = 0
local pendingApplySlots = {}
local applyTimeoutRemaining = 0
local equipmentChangeSerial = 0
local equipmentChangeEventRegistered = false
local collectionMonitorRegistered = false
local currentSlotTooltip = nil
originalTransmogrificationIDs = originalTransmogrificationIDs or {}
previewTransmogrificationIDs = {}
currentTransmogrificationIDs = {}

for k, v in pairs(originalTransmogrificationIDs) do
	currentTransmogrificationIDs[k] = v
end

-- 缓存全局函数以提升性能。
local GetItemIcon, SetItemButtonTexture, PlaySound, CreateFrame, GameTooltip = GetItemIcon, SetItemButtonTexture, PlaySound, CreateFrame, GameTooltip

-- 定义辅助函数。
function CalculateInverseSlot(slot)
	local inverseSlot = (slot - 281) / 2
	return inverseSlot;
end

function TableSetHelper(list)
	local set = {}
	for _, l in ipairs(list) do set[l] = true end
	return set
end

-- 返回装备栏位映射条目。
function GetEquipmentSlot(displaySlot)
	local slotMapping = {
		[PLAYER_VISIBLE_ITEM_1_ENTRYID]  = 1,  -- 头部
		[PLAYER_VISIBLE_ITEM_3_ENTRYID]  = 3,  -- 肩膀
		[PLAYER_VISIBLE_ITEM_4_ENTRYID]  = 4,  -- 衬衫
		[PLAYER_VISIBLE_ITEM_5_ENTRYID]  = 5,  -- 胸甲
		[PLAYER_VISIBLE_ITEM_6_ENTRYID]  = 6,  -- 腰带
		[PLAYER_VISIBLE_ITEM_7_ENTRYID]  = 7,  -- 腿部
		[PLAYER_VISIBLE_ITEM_8_ENTRYID]  = 8,  -- 脚部
		[PLAYER_VISIBLE_ITEM_9_ENTRYID]  = 9,  -- 手腕
		[PLAYER_VISIBLE_ITEM_10_ENTRYID] = 10, -- 手套
		[PLAYER_VISIBLE_ITEM_15_ENTRYID] = 15, -- 背部
		[PLAYER_VISIBLE_ITEM_16_ENTRYID] = 16, -- 主手
		[PLAYER_VISIBLE_ITEM_17_ENTRYID] = 17, -- 副手
		[PLAYER_VISIBLE_ITEM_18_ENTRYID] = 18, -- 远程
		[PLAYER_VISIBLE_ITEM_19_ENTRYID] = 19, -- 战袍
	}
	return slotMapping[displaySlot] or CalculateInverseSlot(displaySlot)
end

-- 返回装备栏位对应的物品 ID。
function GetItemIDForEquipmentSlot(slotName)
	local equipmentSlotName = equipmentSlotIDs[slotName]
	if equipmentSlotName then
		local equipmentSlot = GetEquipmentSlot(equipmentSlotName)
		if equipmentSlot then
			return GetInventoryItemID("player", equipmentSlot)
		end
	end
	return nil
end

-- 更新物品图标纹理。
function SetItemButtonTexture(button, texture)
	if (not button) then
		return
	end

	if (button.Icon or button.icon or (button:GetName() ~= nil and _G[button:GetName()] ~= nil and _G[button:GetName().."IconTexture"] ~= nil)) then
		local icon = button.Icon or button.icon or _G[button:GetName().."IconTexture"];
		if (texture) then
			icon:Show();
			_G[button:GetName().."IconTexture"]:SetTexture(texture);
		else
			icon:Hide();
		end
	end
end

-- 更新装备栏位纹理。
function UpdateSlotTexture(slotName, isTransmogrificationFrame, useTransmogrificationPreview)
	local slotFrame
	
	-- 判断当前更新的是幻化窗口还是角色信息窗口。
	if isTransmogrificationFrame then
		slotFrame = _G["TransmogCharacter" .. slotName .. "Slot"]
	else
		slotFrame = _G["Character" .. slotName .. "Slot"]
	end
	
	if not slotFrame then return end
	
	-- 从按钮获取装备图标纹理。
	local iconTexture = slotFrame.Icon or slotFrame.icon or _G[slotFrame:GetName().."IconTexture"]
	if not iconTexture then return end
	
	-- 确定使用哪个 ID 表。
	local transmogrificationTable = useTransmogrificationPreview and previewTransmogrificationIDs or currentTransmogrificationIDs
	local transmogrificationID = transmogrificationTable[slotName]
	
	-- 检查该栏位是否装备了物品。
	local slotID = equipmentSlotIDs[slotName]
	local equipSlot = GetEquipmentSlot(slotID)
	local hasItem = equipSlot and GetInventoryItemID("player", equipSlot) ~= nil
	
	-- 如果该栏位未装备物品，则强制清除任何幻化外观。
	if not hasItem then
		-- 没有装备意味着不应显示任何幻化，将纹理重置为正常。
		SetItemButtonTexture(slotFrame, slotFrame.backgroundTextureName or "")
		iconTexture:SetDesaturated(false)
		return
	end
	
	-- 如果该栏位装备了物品，则继续处理幻化。
	-- 恢复原外观（transmogrificationID = -1）视为未幻化，显示原始装备图标，避免对 -1 调用 GetItemIcon 导致图标消失。
	if transmogrificationID ~= nil and transmogrificationID ~= 0 and transmogrificationID ~= -1 then
		-- 物品已幻化为其他外观，显示幻化装备图标。
		SetItemButtonTexture(slotFrame, GetItemIcon(transmogrificationID))
		iconTexture:SetDesaturated(false)
	elseif transmogrificationID == 0 then
		-- 物品外观已被隐藏，对原始物品图标进行去饱和处理。
		local originalTexture = GetInventoryItemTexture("player", equipSlot)
		if originalTexture then
			SetItemButtonTexture(slotFrame, originalTexture)
			iconTexture:SetDesaturated(true)
		else
			-- 如果因某些原因无法找到物品图标，则回退到空栏位纹理。
			SetItemButtonTexture(slotFrame, slotFrame.backgroundTextureName)
			iconTexture:SetDesaturated(false)
		end
	else
		-- 物品未被幻化，显示原始装备图标。
		local itemTexture = GetInventoryItemTexture("player", equipSlot)
		if itemTexture then
			SetItemButtonTexture(slotFrame, itemTexture)
			iconTexture:SetDesaturated(false)
		else
			-- 如果因某些原因无法找到物品图标，则回退到空栏位纹理。
			SetItemButtonTexture(slotFrame, slotFrame.backgroundTextureName)
			iconTexture:SetDesaturated(false)
		end
	end
end

-- 更新所有装备图标。
function UpdateAllSlotTextures(useTransmogrificationPreview)
	for slotName, _ in pairs(equipmentSlotIDs) do
		-- 更新角色信息窗口中的物品图标。
		UpdateSlotTexture(slotName, false, false)
		
		-- 更新幻化窗口中的物品图标。
		UpdateSlotTexture(slotName, true, useTransmogrificationPreview)
	end
	
	-- 更新纸娃娃窗口（若可见）以显示新的物品图标。
	if PaperDollFrame:IsShown() then
		PaperDollFrame_UpdateStats()
	end
end

-- 卸下物品时清除该栏位的幻化。
function TransmogrificationHandler.ClearSlotTransmogrification(player, slot)
	equipmentChangeSerial = equipmentChangeSerial + 1
	pendingApplyCount = 0
	-- 从栏位条目 ID 映射表中获取通用栏位名称。
	local slotName = transmogrificationEquipmentSlotMap[tonumber(slot)]

	-- 如果找到通用栏位名称，则从客户端表中清除它。
	if slotName then
		currentTransmogrificationIDs[slotName] = nil
		originalTransmogrificationIDs[slotName] = nil
		previewTransmogrificationIDs[slotName] = nil

		-- 更新所有装备图标。
		UpdateAllSlotTextures(false)
	end
end

function OnClickItemTransmogrificationButton(btn, buttonType)
	if pendingApplyCount > 0 then
		return
	end
	PlaySound("igMainMenuOptionCheckBoxOn", "sfx")
	local itemID = btn:GetID()
	local textureName = GetItemIcon(itemID)
	local slotName = transmogrificationEquipmentSlotMap[CurrentItemSlot]

	-- 判断装备栏位中是否有物品。
	local equipSlot = GetEquipmentSlot(equipmentSlotIDs[slotName])
	local hasItem = equipSlot and GetInventoryItemID("player", equipSlot) ~= nil

	if not hasItem then
		return
	end

	-- 更新该栏位的幻化预览。
	previewTransmogrificationIDs[slotName] = itemID



	-- 使用新的物品幻化预览更新玩家模型。
	LoadTransmogrificationsFromCurrentIDs(true)

	-- 更新幻化窗口中的物品图标。
	UpdateSlotTexture(slotName, true, true)
end

function TransmogrificationHandler.SetTransmogItemIDClient(player, slot, id, realItemID)
	local part = transmogrificationEquipmentSlotMap[tonumber(slot)]
	if part then
		if id == -1 then
			currentTransmogrificationIDs[part] = nil
			originalTransmogrificationIDs[part] = nil
		elseif id == 0 then
			currentTransmogrificationIDs[part] = 0
			originalTransmogrificationIDs[part] = 0
		else
			currentTransmogrificationIDs[part] = id
			originalTransmogrificationIDs[part] = id
		end
		previewTransmogrificationIDs[part] = nil
	end

	UpdateAllSlotTextures()
	LoadTransmogrificationsFromCurrentIDs(false)
end

function TransmogrificationHandler.TransmogrificationFrame(player)
	OnClickTransmogButton(nil)
	TransmogrificationFrame:Show()
end

function TransmogrificationHandler.ApplyTransmogResult(player, slot, success, appliedItemID, realItemID, requestSerial)

	if requestSerial ~= activeApplyRequestSerial then
		return
	end

	local numericSlot = tonumber(slot)
	if not pendingApplySlots[numericSlot] then
		return
	end
	pendingApplySlots[numericSlot] = nil
	pendingApplyCount = math.max(0, pendingApplyCount - 1)
	local part = transmogrificationEquipmentSlotMap[numericSlot]
	if part and success then
		if appliedItemID == -1 then
			currentTransmogrificationIDs[part] = nil
			originalTransmogrificationIDs[part] = nil
		else
			currentTransmogrificationIDs[part] = appliedItemID
			originalTransmogrificationIDs[part] = appliedItemID
		end
		previewTransmogrificationIDs[part] = nil
	end

	if pendingApplyCount == 0 then
		LoadTransmogrificationsFromCurrentIDs(false)
	end
end

-- 接收并保存已收集幻化外观的本地列表，用于显示“新外观”提示行。
function TransmogrificationHandler.ReceiveCollectedAppearances(player, collectedAppearances, uniqueAppearancesCount)
	-- 清空已收集幻化外观表。
	wipe(CollectedAppearances)

	-- 服务端返回代表外观的物品 ID，本地严格以服务端列表为准。
	for _, itemID in ipairs(collectedAppearances or {}) do
		table.insert(CollectedAppearances, itemID)
	end
	
	local collectedAppearancesCount = uniqueAppearancesCount or 0

	if collectedAppearancesCount == 0 then
		DEFAULT_CHAT_FRAME:AddMessage("|cffffff00" .. L["No transmogrification appearances could be located for this account. If you believe this is an error, please contact a Game Master."])
	else
		-- 本地已收集外观列表已以服务端返回为准完成同步，无需额外提示。
	end
end

-- 收到新外观系统消息时，将新外观添加到本地物品列表。
-- 我们利用系统消息来自然遵循服务器关于何时将新外观添加到玩家收藏的选项。
-- 也就是说，如果系统消息字符串与 server_transmog.lua 中的字符串不一致，此函数将失效。
TransmogrificationHandler.ReceiveMatchingAppearances = function(player, originalItemID, matchingItems)
	AIO.Handle("TransmogrificationServer", "SendCollectedTransmogItemIDs")
end

local function AddNewAppearanceToLocalList()
	if collectionMonitorRegistered then
		return
	end
	collectionMonitorRegistered = true
	local chatMonitor = CreateFrame("Frame")
	chatMonitor:RegisterEvent("CHAT_MSG_SYSTEM")

	chatMonitor:SetScript("OnEvent", function(self, event, msg)
		if event == "CHAT_MSG_SYSTEM" and string.find(msg, L["has been added to your appearance collection."]) then

			-- 然后使用链接模式从系统消息中提取物品。
			local itemLink = string.match(msg, "|Hitem:(%d+):[^|]+|h|c%x+%[[^%]]+%]|r|h|r")

			if itemLink then
				local itemID = tonumber(itemLink)

				if itemID then
					AIO.Handle("TransmogrificationServer", "GetItemsWithSameAppearance", itemID)
				end
			end
		end
	end)
end

-- 判断是否应向玩家显示新外观系统消息。
-- 这不决定物品是否添加到本地列表，只决定玩家是否应看到系统消息。
local function collectionMessageFilter(self, event, msg)
	if not Transmogrification.db.global.displayCollectionMessages and
		msg:find(L["has been added to your appearance collection."]) then
		return true -- 隐藏系统消息。
	end
	return false -- 显示系统消息。
end

function LoadTransmogrificationsFromCurrentIDs(useTransmogrificationPreview)
	TransmogrificationModelFrame:SetUnit("player")

	-- 确定使用哪个 ID 表。
	local transmogrificationTable = useTransmogrificationPreview and previewTransmogrificationIDs or currentTransmogrificationIDs

	-- 卸下模型装备，稍后将在函数下方更新外观。
	TransmogrificationModelFrame:Undress()

	-- 为没有幻化的装备栏位应用装备。
	for slotName, slotID in pairs(equipmentSlotIDs) do
		local transmogrificationID = transmogrificationTable[slotName]

		-- 如果没有幻化外观或物品已恢复，则显示原始物品。
		if transmogrificationID == nil or transmogrificationID == -1 then
			local itemID = GetItemIDForEquipmentSlot(slotName)
			if itemID then
				TransmogrificationModelFrame:TryOn(itemID)
			end
		end
	end

	-- 为已幻化的物品应用幻化外观。
	for slotName, transmogrificationID in pairs(transmogrificationTable) do
		if transmogrificationID and transmogrificationID ~= 0 and transmogrificationID ~= -1 then
			TransmogrificationModelFrame:TryOn(transmogrificationID)
		end
	end

	-- 更新所有装备图标。
	UpdateAllSlotTextures(useTransmogrificationPreview)

	-- 根据当前预览同步“应用幻化”按钮的可点击状态与金币花费显示。
	UpdateTransmogApplyState()
end

function OnClickRestoreAllButton(btn)
	if pendingApplyCount > 0 then
		return
	end
	PlaySound("Glyph_MajorCreate", "sfx")
	for slotName, _ in pairs(equipmentSlotIDs) do
		previewTransmogrificationIDs[slotName] = -1
	end

	-- 刷新玩家模型与幻化窗口中的装备图标（恢复后显示原始外观图标）。
	LoadTransmogrificationsFromCurrentIDs(true)
	for slotName, _ in pairs(equipmentSlotIDs) do
		UpdateSlotTexture(slotName, true, true)
	end
end

function OnClickHideAllButton(btn)
	if pendingApplyCount > 0 then
		return
	end
	PlaySound("Glyph_MinorDestroy", "sfx")
	for slotName, _ in pairs(equipmentSlotIDs) do
		previewTransmogrificationIDs[slotName] = 0
	end
	
	-- 刷新玩家模型。
	LoadTransmogrificationsFromCurrentIDs(true)
end

-- 注册装备变更事件。
local function RegisterEquipmentChangeEvent()
	if equipmentChangeEventRegistered then
		return
	end
	equipmentChangeEventRegistered = true
	local eventFrame = CreateFrame("Frame")
	eventFrame:RegisterEvent("PLAYER_EQUIPMENT_CHANGED")

	eventFrame:SetScript("OnUpdate", function(self, elapsed)
		if applyTimeoutRemaining > 0 then
			applyTimeoutRemaining = applyTimeoutRemaining - elapsed
			if applyTimeoutRemaining <= 0 then
				applyRequestSerial = applyRequestSerial + 1
				activeApplyRequestSerial = applyRequestSerial
				pendingApplyCount = 0
				wipe(pendingApplySlots)
				LoadTransmogrificationsFromCurrentIDs(true)
			end
		end
	end)

	eventFrame:SetScript("OnEvent", function(self, event, slot)
		if event == "PLAYER_EQUIPMENT_CHANGED" then
			applyRequestSerial = applyRequestSerial + 1
			activeApplyRequestSerial = applyRequestSerial
			pendingApplyCount = 0
			wipe(pendingApplySlots)
			applyTimeoutRemaining = 0
			-- 装备变更时，通知服务器。
			equipmentChangeSerial = equipmentChangeSerial + 1
			wipe(previewTransmogrificationIDs)
			AIO.Handle("TransmogrificationServer", "OnUnequipItem")

			-- 更新所有装备图标。
			UpdateAllSlotTextures(false)

			-- 如果幻化窗口已打开，也一并更新。
			if TransmogrificationFrame:IsShown() then
				local currentSlotName = transmogrificationEquipmentSlotMap[CurrentItemSlot]
				local currentEquipSlot = GetEquipmentSlot(equipmentSlotIDs[currentSlotName])
				local hasItem = currentEquipSlot and GetInventoryItemID("player", currentEquipSlot) ~= nil

				-- 如果所选栏位未装备物品，则显示无装备警告。
				if not hasItem then
					TransmogWarningText:SetText("|cff" .. L["ff4040"] .. L["No item equipped in this slot."])
					TransmogWarningFrame:Show()
				else
					TransmogWarningFrame:Hide()
					-- 向服务器请求当前数据，以保持幻化窗口最新。
					if CurrentItemSlot then
						listRequestSerial = listRequestSerial + 1
						latestListRequestSerial = listRequestSerial
						if currentSearchText ~= "" then
							AIO.Handle("TransmogrificationServer", "SetSearchCurrentSlotItemIDs", CurrentItemSlot, currentPage, currentSearchText, latestListRequestSerial)
						else
							AIO.Handle("TransmogrificationServer", "SetCurrentSlotItemIDs", CurrentItemSlot, currentPage, latestListRequestSerial)
						end
					end
				end

				-- 刷新玩家模型。
				LoadTransmogrificationsFromCurrentIDs(true)
			end
		end
	end)
end

-- 定义提示函数。
local function OnEnterItemToolTip(btn)
	local itemID = btn:GetID()
	GameTooltip:SetOwner(btn, "ANCHOR_RIGHT")
	GameTooltip:SetHyperlink("item:"..itemID..":0:0:0:0:0:0:0")
	
	local slotName = transmogrificationEquipmentSlotMap[CurrentItemSlot]
	local equipSlot = GetEquipmentSlot(equipmentSlotIDs[slotName])
	local hasItem = equipSlot and GetInventoryItemID("player", equipSlot) ~= nil
	
	if hasItem then
		GameTooltip:AddLine("\n|cff" .. L["00ff00"] .. L["Click to preview this item."])
	end
	
	GameTooltip:Show()
end

function TransmogItemSlotButton_OnEnter(self)
	self:RegisterEvent("MODIFIER_STATE_CHANGED")
	GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
	local slotName = self:GetName():gsub("Transmog", ""):gsub("Character", ""):gsub("Slot", "")
	local transmogID = currentTransmogrificationIDs[slotName] or originalTransmogrificationIDs[slotName]
	
	local slotID = equipmentSlotIDs[slotName]
	local equipSlot = GetEquipmentSlot(slotID)
	local hasItem = equipSlot and GetInventoryItemID("player", equipSlot) ~= nil
	
	if hasItem then
		if transmogID == 0 then
			GameTooltip:SetInventoryItem("player", self:GetID())
			GameTooltip:AddLine("\n|cff" .. L["ff4040"] .. L["Hidden Appearance"])
		elseif transmogID then
			GameTooltip:SetHyperlink("item:"..transmogID..":0:0:0:0:0:0:0")
		else
			GameTooltip:SetInventoryItem("player", self:GetID())
		end
	else
		GameTooltip:SetInventoryItem("player", self:GetID())
	end
	
	GameTooltip:Show()
	CursorUpdate(self)
end

function TransmogrifyToolTip(btn)
	GameTooltip:SetOwner(btn, "ANCHOR_RIGHT")
	GameTooltip:AddLine("|cffffffff" .. L["Transmogrify"])
	GameTooltip:Show()
end

function RestoreItemToolTip(btn)
	GameTooltip:SetOwner(btn, "ANCHOR_RIGHT")
	GameTooltip:AddLine("|cffffffff" .. L["Restore Item Appearance"])
	GameTooltip:Show()
end

function HideItemToolTip(btn)
	GameTooltip:SetOwner(btn, "ANCHOR_RIGHT")
	GameTooltip:AddLine("|cffffffff" .. L["Hide Item"])
	GameTooltip:Show()
end

function RestoreAllItemsToolTip(btn)
	GameTooltip:SetOwner(btn, "ANCHOR_RIGHT")
	GameTooltip:AddLine("|cffffffff" .. L["Restore All Item Appearances"])
	GameTooltip:Show()
end

function HideAllItemsToolTip(btn)
	GameTooltip:SetOwner(btn, "ANCHOR_RIGHT")
	GameTooltip:AddLine("|cffffffff" .. L["Hide All Items"])
	GameTooltip:Show()
end

-- function ShowCloakToolTip(btn)
-- 	GameTooltip:SetOwner(btn, "ANCHOR_RIGHT")
-- 	GameTooltip:AddLine("|cffffffff" .. L["Toggle Character Cloak Display"])
-- 	GameTooltip:AddLine("|cffffd200" .. L["This checkbox provides the same function as\nticking or unticking the \"Show Cloak\" checkbox\nin the interface options menu. It will have no\neffect on the transmogrify preview window."])
-- 	GameTooltip:Show()
-- end

-- function ShowHelmToolTip(btn)
-- 	GameTooltip:SetOwner(btn, "ANCHOR_RIGHT")
-- 	GameTooltip:AddLine("|cffffffff" .. L["Toggle Character Helm Display"])
-- 	GameTooltip:AddLine("|cffffd200" .. L["This checkbox provides the same function as\nticking or unticking the \"Show Helm\" checkbox\nin the interface options menu. It will have no\neffect on the transmogrify preview window."])
-- 	GameTooltip:Show()
-- end

-- 将铜币数值格式化为“金/银/铜”字符串（3.3.5 客户端无 GetMoneyString，需自行实现）。
local function FormatMoney(copper)
	copper = math.max(0, math.floor(tonumber(copper) or 0))
	local gold = math.floor(copper / 10000)
	local silver = math.floor((copper % 10000) / 100)
	local copperRem = copper % 100

	local parts = {}
	if gold > 0 then
		table.insert(parts, "|cffffd700" .. gold .. "|r|cffffffff金|r")
	end
	if silver > 0 then
		table.insert(parts, "|cffc0c0c0" .. silver .. "|r|cffffffff银|r")
	end
	if copperRem > 0 or #parts == 0 then
		table.insert(parts, "|cffb87333" .. copperRem .. "|r|cffffffff铜|r")
	end
	return table.concat(parts, " ")
end

-- 判断是否还有待应用的幻化改动。
-- 逻辑与“应用幻化”处理函数保持一致：仅在对应栏位已装备物品、且预览外观与当前外观不同时才算作改动。
function HasTransmogChanges()
	for slotName, entryID in pairs(equipmentSlotIDs) do
		local transmogID = previewTransmogrificationIDs[slotName]
		local currentID = currentTransmogrificationIDs[slotName]

		-- 预览为 nil 表示没有待应用的改动（应用成功后预览会被清空，不应视为改动）。
		local equipSlot = GetEquipmentSlot(entryID)
		local hasItem = equipSlot and GetInventoryItemID("player", equipSlot) ~= nil
		local equippedItemID = hasItem and GetInventoryItemID("player", equipSlot) or nil
		-- 请求的外观若等于当前已应用的幻化，或等于当前装备实物自身的外观，均视为无改动（不改变显示）。
		if hasItem and transmogID ~= nil and transmogID ~= currentID and transmogID ~= equippedItemID then
			return true
		end
	end
	return false
end

local function GetTransmogrificationCost()
	local totalCost = 0

	for slotName, entryID in pairs(equipmentSlotIDs) do
		local transmogID = previewTransmogrificationIDs[slotName]
		local currentID = currentTransmogrificationIDs[slotName]

		-- 预览为 nil 表示没有待应用的改动，不计费。
		local equipSlot = GetEquipmentSlot(entryID)
		local hasItem = equipSlot and GetInventoryItemID("player", equipSlot) ~= nil
		local equippedItemID = hasItem and GetInventoryItemID("player", equipSlot) or nil
		-- 仅对“应用一个具体外观”计费。恢复原外观（transmogID = -1）属于取消幻化的特殊操作，
		-- 不收取金币，与服务端保持一致；若请求外观等于当前已应用幻化或装备实物自身外观，亦不计费。
		if hasItem and transmogID and transmogID ~= -1 and transmogID ~= currentID and transmogID ~= equippedItemID then
			local itemID = equippedItemID
			-- 3.3.5 客户端无 GetItemInfoInstant / 数值化 itemClass，
			-- 改用 GetItemInfo 返回的 invType（非本地化常量）区分武器与护甲。
			local isWeapon = false
			local _, _, _, _, _, _, _, _, invType = GetItemInfo(itemID)
			if invType then
				isWeapon = invType == "INVTYPE_WEAPON" or invType == "INVTYPE_WEAPONMAINHAND"
					or invType == "INVTYPE_WEAPONOFFHAND" or invType == "INVTYPE_2HWEAPON"
					or invType == "INVTYPE_RANGED" or invType == "INVTYPE_RANGEDRIGHT" or invType == "INVTYPE_THROWN"
			else
				-- GetItemInfo 未命中缓存时，按栏位类型回退判断（副手可能含盾牌，保守按护甲计）。
				isWeapon = (slotName == "MainHand" or slotName == "Ranged")
			end

			if isWeapon then
				totalCost = totalCost + WEAPON_TRANSMOG_COST
			else
				totalCost = totalCost + ARMOR_TRANSMOG_COST
			end
		end
	end

	return math.max(0, tonumber(totalCost) or 0)
end

-- 根据是否存在待应用改动，更新“应用幻化”按钮的可点击状态，并刷新所需金币显示。
function UpdateTransmogApplyState()
	local hasChanges = HasTransmogChanges()

	if _G["SaveButton"] then
		if hasChanges then
			_G["SaveButton"]:Enable()
		else
			_G["SaveButton"]:Disable()
		end
	end

	if _G["TransmogCostText"] then
		if hasChanges then
			local totalCost = GetTransmogrificationCost()
			if totalCost > 0 then
				local text = "|cffffd200" .. L["Cost"] .. "：|r" .. FormatMoney(totalCost)
				_G["TransmogCostText"]:SetText(text)
			else
				_G["TransmogCostText"]:SetText("")
			end
		else
			_G["TransmogCostText"]:SetText("")
		end
	end
end

function TransmogrificationToolTip(btn)
	GameTooltip:SetOwner(btn, "ANCHOR_RIGHT")

	-- 判断是否有待应用的幻化外观更改。
	local hasChanges = false
	for slotName, transmogID in pairs(previewTransmogrificationIDs) do
		if transmogID ~= currentTransmogrificationIDs[slotName] then
			hasChanges = true
			break
		end
	end

	GameTooltip:AddLine("|cffffffff" .. L["Transmogrify"])
	if hasChanges then
		local totalCost = GetTransmogrificationCost()
		if totalCost > 0 then
			GameTooltip:AddLine("|cffffd200需要消耗：|r" .. FormatMoney(totalCost))
		end
	else
		GameTooltip:AddLine("|cff808080" .. L["No appearances to apply."])
	end

	GameTooltip:Show()
end

-- 挂载物品提示系统，以便（启用时）显示“新外观”提示文本。
local function HookItemTooltip()
	local settings = Transmogrification:GetSettings()
	if not settings.displayNewAppearanceTooltip then return end

	if isTooltipHooked then return end

	local originalSetItem = GameTooltip:GetScript("OnTooltipSetItem")

	GameTooltip:SetScript("OnTooltipSetItem", function(self, ...)
		if originalSetItem then
			originalSetItem(self, ...)
		end

		local _, link = self:GetItem()
		if not link then return end

		local id = select(3, strfind(link, "^|%x+|Hitem:(%-?%d+):(%d+):(%d+):(%d+):(%d+):(%d+):(%-?%d+):(%-?%d+)"))
		if not id then return end

		id = tonumber(id)
		if not id then return end

		local _, _, _, _, _, _, _, _, itemEquipSlot = GetItemInfo(id)

		-- 如果物品不可幻化或外观已被收集，则跳过应用“新外观”提示行。
		if IsEquippableItem(id) and itemEquipSlot and itemEquipSlot ~= "INVTYPE_AMMO" and
		itemEquipSlot ~= "INVTYPE_NECK" and itemEquipSlot ~= "INVTYPE_FINGER" and
		itemEquipSlot ~= "INVTYPE_TRINKET" and itemEquipSlot ~= "INVTYPE_BAG" and
		itemEquipSlot ~= "INVTYPE_QUIVER" and not tContains(CollectedAppearances, id) then
			self:AddLine("|cff" .. L["f194f7"] .. L["New Appearance"])
		end
	end)
	
	isTooltipHooked = true
end

function OnLeaveHideToolTip(btn)
	GameTooltip:Hide()
end

-- 搜索函数
function EnterSearchInput()
	isInputHovered = true
end

function LeaveSearchInput()
	isInputHovered = false
end

function SetSearchInputFocus()
	if ( isInputHovered ) then
		ItemSearchInput:SetText("")
		ItemSearchInput:SetFocus()
	end
end

function SetSearchTab()
	if pendingApplyCount > 0 then
		return
	end
	PlaySound("igSpellBookSpellIconPickup", "sfx")
	currentPage = 1
	currentSearchText = ItemSearchInput:GetText()
	listRequestSerial = listRequestSerial + 1
	latestListRequestSerial = listRequestSerial
	TransmogPaginationText:SetText(string.format(L["Page %s"], currentPage))
	AIO.Handle("TransmogrificationServer", "SetSearchCurrentSlotItemIDs", CurrentItemSlot, currentPage, currentSearchText, latestListRequestSerial)
	ItemSearchInput:ClearFocus()
end

-- 定义装备栏位名称。
characterEquipmentSlotNames = TableSetHelper(characterEquipmentSlotNames)

-- 登录时应用玩家幻化。
local function OnEvent(self, event)
	AIO.Handle("TransmogrificationServer", "LoadPlayer")
end

AIO.AddSavedVarChar("originalTransmogrificationIDs")

local function OnEventEnterWorldReloadTransmogIDs(self, event)
	if ( event == "PLAYER_ENTERING_WORLD") then
		AIO.Handle("TransmogrificationServer", "SetTransmogItemIDs")
		if CollectedAppearances == nil then
			CollectedAppearances = {}
		end
		HookItemTooltip()
		AddNewAppearanceToLocalList()
	else
		AIO.Handle("TransmogrificationServer", "OnUnequipItem")
		UpdateAllSlotTextures()
		if ( TransmogrificationFrame:IsShown() ) then
			LoadTransmogrificationsFromCurrentIDs()
		end
	end
end

-- 为插件功能注册事件框架。
local f = CreateFrame("Frame")
f:RegisterEvent("PLAYER_ENTERING_WORLD")
f:SetScript("OnEvent", OnEvent)

-- 为新外观系统消息过滤器注册事件框架。
ChatFrame_AddMessageEventFilter("CHAT_MSG_SYSTEM", collectionMessageFilter)

-- 定义窗口函数。
function OnClickTransmogButton(self)
	PlaySound("AchievementMenuOpen", "sfx")

	-- 清空并初始化预览幻化表。这确保预览窗口始终保持最新。
	wipe(previewTransmogrificationIDs)
	for slot, transmogID in pairs(currentTransmogrificationIDs) do
		previewTransmogrificationIDs[slot] = transmogID
	end

	-- 在预览窗口中显示玩家当前的外观。
	TransmogrificationModelFrame:SetUnit("player")
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_1_ENTRYID

	-- 设置界面状态变量。
	characterTransmogTab:SetChecked(true)
	isInputHovered = false
	currentPage = 1
	currentSearchText = ""
	listRequestSerial = listRequestSerial + 1
	latestListRequestSerial = listRequestSerial
	TransmogPaginationText:SetText(string.format(L["Page %s"], currentPage))

	-- 更新所有装备图标。
	UpdateAllSlotTextures(true)

	-- 初始化界面状态。
	for slot, value in pairs(equipmentSlotIDs) do
		_G["TransmogCharacter"..slot.."Slot"].toastTexture:SetTexture("Interface\\AddOns\\Transmogrification\\assets\\Transmog-Overlay-Toast")
		_G["TransmogCharacter"..slot.."Slot"].restoreButton:Hide()
		_G["TransmogCharacter"..slot.."Slot"].hideButton:Hide()
	end

	-- 设置当前激活的装备栏位。
	local slotName = transmogrificationEquipmentSlotMap[CurrentItemSlot]
	_G["TransmogCharacter"..slotName.."Slot"].toastTexture:SetTexture("Interface\\AddOns\\Transmogrification\\assets\\Transmog-Overlay-Selected")
	_G["TransmogCharacter"..slotName.."Slot"].restoreButton:Show()
	_G["TransmogCharacter"..slotName.."Slot"].hideButton:Show()

	-- 更多界面状态初始化。
	ItemSearchInput:SetText("|cff" .. L["b2b2b2"] .. L["Filter Item Appearance"] .. "|r")

	-- 直接向服务器请求物品。
	AIO.Handle("TransmogrificationServer", "SetCurrentSlotItemIDs", CurrentItemSlot, 1, latestListRequestSerial)

	-- 使用新的物品幻化预览更新玩家模型。
	LoadTransmogrificationsFromCurrentIDs(true)
end

function PaperDollFrame_OnShow(self)
	PaperDollFrame_SetLevel();
	PaperDollFrame_SetResistances();
	PaperDollFrame_UpdateStats();
	if ( UnitHasRelicSlot("player") ) then
		CharacterAmmoSlot:Hide();
	else
		CharacterAmmoSlot:Show();
	end
	if ( not PlayerTitlePickerScrollFrame.titles ) then
		PlayerTitleFrame_UpdateTitles();
	end

	if ( TransmogrificationFrame:IsShown() ) then
		characterTransmogTab:SetChecked(true)
		else
		characterTransmogTab:SetChecked(false)
	end

	LoadTransmogrificationsFromCurrentIDs()
end

function OnClickApplyAllowTransmogrifications(btn)
	if pendingApplyCount > 0 then
		return
	end
	pendingApplyCount = 0
	wipe(pendingApplySlots)
	applyTimeoutRemaining = 0
	applyRequestSerial = applyRequestSerial + 1
	activeApplyRequestSerial = applyRequestSerial
	PlaySound("Distract Impact", "sfx")

	-- 在服务器层面应用幻化，最终状态仅由服务器回调确认。
	for slotName, entryID in pairs(equipmentSlotIDs) do
		local transmogID = previewTransmogrificationIDs[slotName]
		local currentID = currentTransmogrificationIDs[slotName]
		local requestedID = transmogID
		if requestedID == nil and currentID ~= nil then
			requestedID = -1
		end

		local equipSlot = GetEquipmentSlot(entryID)
		local hasItem = equipSlot and GetInventoryItemID("player", equipSlot) ~= nil
		if hasItem and requestedID ~= nil and requestedID ~= currentID then
			pendingApplyCount = pendingApplyCount + 1
			pendingApplySlots[entryID] = true

			AIO.Handle("TransmogrificationServer", "EquipTransmogItem", requestedID, entryID, activeApplyRequestSerial)
		end
	end

	if pendingApplyCount == 0 then

		LoadTransmogrificationsFromCurrentIDs(true)
	else

		applyTimeoutRemaining = 10
	end
end

function OnClickHideCurrentTransmogSlot(btn)
	local slotName = transmogrificationEquipmentSlotMap[CurrentItemSlot]
	local equipSlot = GetEquipmentSlot(equipmentSlotIDs[slotName])
	local hasItem = equipSlot and GetInventoryItemID("player", equipSlot) ~= nil

	-- 如果玩家已卸下他们试图隐藏的物品，则显示“警告”对话框。
	if not hasItem then
		StaticPopupDialogs["NO_ITEM_TO_HIDE_EQUIPPED_DIALOG"] = {
			text = L["You must have an item equipped in this slot to hide its appearance."],
			button1 = OKAY,
			timeout = 0,
			whileDead = true,
			hideOnEscape = true,
			preferredIndex = 3,
		}
		StaticPopup_Show("NO_ITEM_TO_HIDE_EQUIPPED_DIALOG")
		return
	end

	PlaySound("ArcaneMissileImpacts", "sfx")

	-- 在预览窗口中（临时）隐藏该物品。
	previewTransmogrificationIDs[slotName] = 0

	-- 使用新的物品幻化预览更新玩家模型。
	LoadTransmogrificationsFromCurrentIDs(true)

	-- 更新幻化窗口中的物品图标。
	UpdateSlotTexture(slotName, true, true)
end

function OnClickRestoreCurrentTransmogSlot(btn)
	local slotName = transmogrificationEquipmentSlotMap[CurrentItemSlot]
	local equipSlot = GetEquipmentSlot(equipmentSlotIDs[slotName])
	local hasItem = equipSlot and GetInventoryItemID("player", equipSlot) ~= nil

	if not hasItem then
		-- 如果玩家已卸下他们试图恢复的物品，则显示“警告”对话框。
		StaticPopupDialogs["NO_ITEM_TO_RESTORE_EQUIPPED_DIALOG"] = {
			text = L["You must have an item equipped in this slot to restore its appearance."],
			button1 = OKAY,
			timeout = 0,
			whileDead = true,
			hideOnEscape = true,
			preferredIndex = 3,
		}
		StaticPopup_Show("NO_ITEM_TO_RESTORE_EQUIPPED_DIALOG")
		return
	end

	PlaySound("Glyph_MinorCreate", "sfx")

	-- 在预览窗口中（临时）恢复该物品。
	previewTransmogrificationIDs[slotName] = -1

	-- 使用新的物品幻化预览更新玩家模型。
	LoadTransmogrificationsFromCurrentIDs(true)

	-- 更新幻化窗口中的物品图标。
	UpdateSlotTexture(slotName, true, true)
end

function TransmogModelMouseRotation(modelFrame)
	local rotationArea = CreateFrame("Frame", modelFrame:GetName().."RotationArea", modelFrame)
	rotationArea:SetSize(160, 280)
	rotationArea:SetPoint("CENTER", 0, 0)

	rotationArea:EnableMouse(true)
	modelFrame.isMouseRotating = false
	modelFrame.lastCursorX = 0

	rotationArea:SetScript("OnMouseDown", function(frame, button)
		if button == "LeftButton" then
			modelFrame.isMouseRotating = true
			modelFrame.lastCursorX = GetCursorPosition()
			if not _G["TransmogMouseCapture"] then
				local captureFrame = CreateFrame("Frame", "TransmogMouseCapture", UIParent)
				captureFrame:SetFrameStrata("TOOLTIP")
				captureFrame:SetAllPoints(UIParent)
				captureFrame:EnableMouse(true)
				captureFrame:Hide()
				captureFrame:SetScript("OnMouseUp", function(captureFrame, button)
					if button == "LeftButton" and modelFrame.isMouseRotating then
						modelFrame.isMouseRotating = false
						modelFrame:SetScript("OnUpdate", nil)
						captureFrame:Hide()
					end
				end)
			end

			TransmogMouseCapture:Show()

			modelFrame:SetScript("OnUpdate", function()
				if modelFrame.isMouseRotating then
					local currentX = GetCursorPosition()
					-- 控制鼠标旋转速度。
					local diff = (currentX - modelFrame.lastCursorX) * 0.02
					modelFrame:SetFacing(modelFrame:GetFacing() + diff)
					modelFrame.lastCursorX = currentX
				end
			end)
		end
	end)

	rotationArea:SetScript("OnMouseUp", function(frame, button)
		if button == "LeftButton" and modelFrame.isMouseRotating then
			modelFrame.isMouseRotating = false
			modelFrame:SetScript("OnUpdate", nil)
			if _G["TransmogMouseCapture"] then
				TransmogMouseCapture:Hide()
			end
		end
	end)

	modelFrame:HookScript("OnHide", function(frame)
		if modelFrame.isMouseRotating then
			modelFrame.isMouseRotating = false
			modelFrame:SetScript("OnUpdate", nil)
			if _G["TransmogMouseCapture"] then
				TransmogMouseCapture:Hide()
			end
		end
	end)

	rotationArea:SetScript("OnLeave", function(frame)
		GameTooltip:Hide()
	end)

	modelFrame.rotationArea = rotationArea
end

-- 设置幻化窗口的当前标签页。
function SetTab()
	if pendingApplyCount > 0 then
		return
	end
	if (ItemSearchInput:GetText() ~= "" and ItemSearchInput:GetText() ~= "|cff" .. L["b2b2b2"] .. L["Filter Item Appearance"] .. "|r") then
		SetSearchTab()
		return;
	end

	PlaySound("igSpellBookSpellIconPickup", "sfx")
	currentPage = 1
	currentSearchText = ""
	listRequestSerial = listRequestSerial + 1
	latestListRequestSerial = listRequestSerial
	TransmogPaginationText:SetText(string.format(L["Page %s"], currentPage))

	-- 检查该栏位是否装备了物品。
	local slotName = transmogrificationEquipmentSlotMap[CurrentItemSlot]
	local equipSlot = GetEquipmentSlot(equipmentSlotIDs[slotName])
	local hasItem = equipSlot and GetInventoryItemID("player", equipSlot) ~= nil

	-- 刷新幻化窗口。
	for slot, value in pairs(equipmentSlotIDs) do
		_G["TransmogCharacter"..slot.."Slot"].toastTexture:SetTexture("Interface\\AddOns\\Transmogrification\\assets\\Transmog-Overlay-Toast")
		_G["TransmogCharacter"..slot.."Slot"].restoreButton:Hide()
		_G["TransmogCharacter"..slot.."Slot"].hideButton:Hide()
	end

	-- 设置当前激活的装备栏位。
	_G["TransmogCharacter"..slotName.."Slot"].toastTexture:SetTexture("Interface\\AddOns\\Transmogrification\\assets\\Transmog-Overlay-Selected")

	-- 如果玩家正在查看空装备栏位，则显示警告消息。
	if not hasItem then
		TransmogWarningText:SetText("|cff" .. L["ff4040"] .. L["No item equipped in this slot."])
		TransmogWarningFrame:Show()
	else
		TransmogWarningFrame:Hide()

		-- 如果装备栏位不为空，则显示恢复/隐藏按钮。
		_G["TransmogCharacter"..slotName.."Slot"].restoreButton:Show()
		_G["TransmogCharacter"..slotName.."Slot"].hideButton:Show()
	end

	-- 向服务器查询可显示的适用物品外观。
	listRequestSerial = listRequestSerial + 1
	latestListRequestSerial = listRequestSerial
	AIO.Handle("TransmogrificationServer", "SetCurrentSlotItemIDs", CurrentItemSlot, currentPage, latestListRequestSerial)
end


function OnClickNextPage(btn)
	if pendingApplyCount > 0 then
		return
	end
	PlaySound("igAbiliityPageTurn", "sfx")
	currentPage = currentPage + 1
	listRequestSerial = listRequestSerial + 1
	latestListRequestSerial = listRequestSerial
	if currentSearchText ~= "" then
		AIO.Handle("TransmogrificationServer", "SetSearchCurrentSlotItemIDs", CurrentItemSlot, currentPage, currentSearchText, latestListRequestSerial)
	else
		AIO.Handle("TransmogrificationServer", "SetCurrentSlotItemIDs", CurrentItemSlot, currentPage, latestListRequestSerial)
	end
end

function OnClickPrevPage(btn)
	if pendingApplyCount > 0 then
		return
	end
	PlaySound("igAbiliityPageTurn", "sfx")
	if ( currentPage == 1 ) then
		return;
	end
	currentPage = currentPage - 1
	listRequestSerial = listRequestSerial + 1
	latestListRequestSerial = listRequestSerial
	if currentSearchText ~= "" then
		AIO.Handle("TransmogrificationServer", "SetSearchCurrentSlotItemIDs", CurrentItemSlot, currentPage, currentSearchText, latestListRequestSerial)
	else
		AIO.Handle("TransmogrificationServer", "SetCurrentSlotItemIDs", CurrentItemSlot, currentPage, latestListRequestSerial)
	end
end

function OnClickHeadTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_1_ENTRYID
	SetTab()
end

function OnClickShoulderTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_3_ENTRYID
	SetTab()
end

function OnClickShirtTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_4_ENTRYID
	SetTab()
end

function OnClickChestTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_5_ENTRYID
	SetTab()
end

function OnClickWaistTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_6_ENTRYID
	SetTab()
end

function OnClickLegsTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_7_ENTRYID
	SetTab()
end

function OnClickFeetTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_8_ENTRYID
	SetTab()
end

function OnClickWristTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_9_ENTRYID
	SetTab()
end

function OnClickHandsTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_10_ENTRYID
	SetTab()
end

function OnClickBackTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_15_ENTRYID
	SetTab()
end

function OnClickMainTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_16_ENTRYID
	SetTab()
end

function OnClickOffTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_17_ENTRYID
	SetTab()
end

function OnClickRangedTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_18_ENTRYID
	SetTab()
end

function OnClickTabardTab(btn)
	CurrentItemSlot = PLAYER_VISIBLE_ITEM_19_ENTRYID
	SetTab()
end

function OnHideTransmogrificationFrame(self)
	applyRequestSerial = applyRequestSerial + 1
	activeApplyRequestSerial = applyRequestSerial
	pendingApplyCount = 0
	wipe(pendingApplySlots)
	applyTimeoutRemaining = 0
	currentSearchText = ""
	currentPage = 1
	PlaySound("AchievementMenuClose", "sfx")

	-- 丢弃幻化预览更改。
	wipe(previewTransmogrificationIDs)

	-- 使用服务器返回的新信息刷新幻化预览。
	LoadTransmogrificationsFromCurrentIDs(false)
	characterTransmogTab:SetChecked(false)
end

-- 定义窗口布局。
function TransmogItemSlotButton_OnLoad(self)
	self:RegisterForClicks("LeftButtonUp", "RightButtonUp")
	local slotName = self:GetName():gsub("Transmog", "")
	local id, textureName, checkRelic = GetInventorySlotInfo(strsub(slotName,10))
	self:SetID(id)
	local texture = _G["Transmog"..slotName.."IconTexture"]
	texture:SetTexture(textureName)
	self.backgroundTextureName = textureName
	self.checkRelic = checkRelic
	self.UpdateTooltip = TransmogItemSlotButton_OnEnter
end

local function InitTabSlots()
	local lastSlot
	local firstInRowSlot
	
	for i = 1, 6, 1 do
		local itemChild
		if ( i == 1 ) then
			itemChild = CreateFrame("Frame", "ItemChild"..i, TransmogrificationFrame, "TransmogItemWrapperTemplate")
			itemChild:SetPoint("TOPLEFT", 400, -110)
			firstInRowSlot = itemChild
		else
			if ( i == 4 ) then
				itemChild = CreateFrame("Frame", "ItemChild"..i, firstInRowSlot, "TransmogItemWrapperTemplate")
				itemChild:SetPoint("RIGHT", 0, -200)
				firstInRowSlot = itemChild
			else
				itemChild = CreateFrame("Button", "ItemChild"..i, lastSlot, "TransmogItemWrapperTemplate")
				itemChild:SetPoint("RIGHT", 230, 0)
			end
		end

		local rightTopItemFrame = CreateFrame("Frame", "RightTopItemFrame"..i, itemChild)
		rightTopItemFrame:SetPoint("TOPRIGHT", -4, -4)
		rightTopItemFrame:SetSize(34, 142)
		
		local rightTopTexture = rightTopItemFrame:CreateTexture(nil, "Background")
		rightTopTexture:SetTexture(DressUpTexturePath().."2")
		rightTopTexture:SetAllPoints()
		
		local rightBottomItemFrame = CreateFrame("Frame", "RightBottomItemFrame"..i, itemChild)
		rightBottomItemFrame:SetPoint("BOTTOMRIGHT", -4, -18)
		rightBottomItemFrame:SetSize(34, 53)
		
		local rightBottomTexture = rightBottomItemFrame:CreateTexture(nil, "Background")
		rightBottomTexture:SetTexture(DressUpTexturePath().."4")
		rightBottomTexture:SetAllPoints()
		
		local leftTopItemFrame = CreateFrame("Frame", "LeftTopItemFrame"..i, itemChild)
		leftTopItemFrame:SetPoint("TOPLEFT", 4, -4)
		leftTopItemFrame:SetSize(109, 142)
		
		local leftTopTexture = leftTopItemFrame:CreateTexture(nil, "Background")
		leftTopTexture:SetTexture(DressUpTexturePath().."1")
		leftTopTexture:SetAllPoints()
		
		local leftBottomItemFrame = CreateFrame("Frame", "LeftBottomItemFrame"..i, itemChild)
		leftBottomItemFrame:SetPoint("BOTTOMLEFT", 4, -18)
		leftBottomItemFrame:SetSize(109, 53)
		
		local leftBottomTexture = leftBottomItemFrame:CreateTexture(nil, "Background")
		leftBottomTexture:SetTexture(DressUpTexturePath().."3")
		leftBottomTexture:SetAllPoints()
		
		local itemModel = CreateFrame("DressUpModel", "ItemModel"..i, itemChild)
		itemModel:SetPoint("CENTER", 0, 0)
		itemModel:SetSize(142, 172)
		itemModel:Hide()
		
		local itemButton = CreateFrame("Button", "ItemButton"..i, leftBottomItemFrame, "TransmogItemButtonTemplate")
		itemButton:SetPoint("BOTTOMLEFT", 6, 28)
		itemButton:SetScript("OnClick", OnClickItemTransmogrificationButton)
		itemButton:SetScript("OnEnter", OnEnterItemToolTip)
		itemButton:SetScript("OnLeave", OnLeaveHideToolTip)
		itemButton:RegisterForClicks("AnyUp");
		itemButton:Disable()
		lastSlot = itemChild
		itemChild.itemModel = itemModel
		itemChild.itemButton = itemButton
		table.insert(itemButtons, itemChild)
	end
end

function TransmogrificationHandler.InitTab(player, newSlotItemIDs, responseSlot, page, hasMorePages, requestSerial)
	if requestSerial and requestSerial ~= latestListRequestSerial then
		return
	end
	if tonumber(responseSlot) ~= tonumber(CurrentItemSlot) then
		return
	end
	TransmogPaginationText:SetText(string.format(L["Page %s"], page))

	-- 判断该栏位是否为空。
	local slotName = transmogrificationEquipmentSlotMap[CurrentItemSlot]
	local equipSlot = GetEquipmentSlot(equipmentSlotIDs[slotName])
	local hasItem = equipSlot and GetInventoryItemID("player", equipSlot) ~= nil

	-- 如果栏位为空，则显示警告。
	if not hasItem then
		TransmogWarningText:SetText("|cff" .. L["ff4040"] .. L["No item equipped in this slot."])
		TransmogWarningFrame:Show()
	else
		TransmogWarningFrame:Hide()

		-- 如果栏位不为空，则显示恢复和隐藏按钮。
		_G["TransmogCharacter"..slotName.."Slot"].restoreButton:Show()
		_G["TransmogCharacter"..slotName.."Slot"].hideButton:Show()
	end

	-- 更新翻页按钮。
	if (hasMorePages) then
		RightButton:Enable()
	else
		RightButton:Disable()
	end

	if (page > 1) then
		LeftButton:Enable()
	else
		LeftButton:Disable()
	end

	-- 显示可能的幻化外观。
	if newSlotItemIDs and #newSlotItemIDs > 0 then
		for i, child in ipairs(itemButtons) do
			if (i > #newSlotItemIDs or newSlotItemIDs[i] == nil) then
				-- 如果正处于可用外观的最后一页，则隐藏空物品。
				child:SetID(0)
				child.itemButton:SetID(0)
				child.itemButton:Disable()
				child.itemModel:Hide()
				SetItemButtonTexture(child.itemButton, "Interface\\paperdoll\\UI-PaperDoll-Slot-" .. equipmentSlotIcons[CalculateInverseSlot(CurrentItemSlot)])
			else
				-- 如果找到适用外观，则显示物品。
				child:SetID(newSlotItemIDs[i])
				child.itemButton:SetID(newSlotItemIDs[i])
				local textureName = GetItemIcon(newSlotItemIDs[i])
				SetItemButtonTexture(child.itemButton, textureName)

				-- 允许点击物品以更改幻化外观。
				if hasItem then
					child.itemButton:Enable()
					child.itemButton:SetScript("OnClick", OnClickItemTransmogrificationButton)
				else
					-- 启用物品按钮以显示提示。
					child.itemButton:Enable()

					-- 由于装备栏位为空，移除点击事件。
					child.itemButton:SetScript("OnClick", function(self)
						PlaySound("igMainMenuOptionCheckBoxOff", "sfx")
					end)
				end

				child.itemModel:Show()
				child.itemModel:SetUnit("player")

				-- 如果正在查看适用的栏位，则旋转玩家模型。
				if (CurrentItemSlot == PLAYER_VISIBLE_ITEM_15_ENTRYID) then -- 披风
					child.itemModel:SetRotation(10, false)
				elseif (CurrentItemSlot == PLAYER_VISIBLE_ITEM_16_ENTRYID) then -- 主手
					child.itemModel:SetRotation(1, false)
				else
					child.itemModel:SetRotation(0, false)
				end
				child.itemModel:Undress()
				child.itemModel:TryOn(newSlotItemIDs[i])

				local _, playerRace = UnitRace("player")
				playerRace = string.upper(playerRace)
				local playerSex = UnitSex("player")
				local isFemale = (playerSex == 3)

				-- 根据种族更改预览模型的位置和缩放，确保它们与窗口对齐。
				if playerRace == "HUMAN" then
					if isFemale then
						child.itemModel:SetPoint("CENTER", 4, -1)
						child.itemModel:SetSize(169, 169)
					else
						child.itemModel:SetPoint("CENTER", 4, 2)
						child.itemModel:SetSize(180, 180)
					end
				elseif playerRace == "DWARF" then
					if isFemale then
						child.itemModel:SetPoint("CENTER", 0, 6)
						child.itemModel:SetSize(165, 165)
					else
						child.itemModel:SetPoint("CENTER", 6, -10)
						child.itemModel:SetSize(170, 170)
					end
				elseif playerRace == "NIGHTELF" then
					if isFemale then
						child.itemModel:SetPoint("CENTER", 3, -9)
						child.itemModel:SetSize(181, 181)
					else
						child.itemModel:SetPoint("CENTER", 2, -5)
						child.itemModel:SetSize(190, 190)
					end
				elseif playerRace == "GNOME" then
					if isFemale then
						child.itemModel:SetPoint("CENTER", 3, -8)
						child.itemModel:SetSize(133, 133)
					else
						child.itemModel:SetPoint("CENTER", 4, -4)
						child.itemModel:SetSize(140, 140)
					end
				elseif playerRace == "DRAENEI" then
					if isFemale then
						child.itemModel:SetPoint("CENTER", 10, 3)
						child.itemModel:SetSize(185, 185)
					else
						child.itemModel:SetPoint("CENTER", 6, 2)
						child.itemModel:SetSize(165, 165)
					end
				elseif playerRace == "ORC" then
					if isFemale then
						child.itemModel:SetPoint("CENTER", -3, -5)
						child.itemModel:SetSize(175, 175)
					else
						child.itemModel:SetPoint("CENTER", 2, -4)
						child.itemModel:SetSize(165, 165)
					end
				elseif playerRace == "UNDEAD" or playerRace == "SCOURGE" then
					if isFemale then
						child.itemModel:SetPoint("CENTER", 3, 0)
						child.itemModel:SetSize(188, 188)
					else
						child.itemModel:SetPoint("CENTER", 1, -6)
						child.itemModel:SetSize(175, 175)
					end
				elseif playerRace == "TAUREN" then
					if isFemale then
						child.itemModel:SetPoint("CENTER", 2, -1)
						child.itemModel:SetSize(180, 180)
					else
						child.itemModel:SetPoint("CENTER", 1, -6)
						child.itemModel:SetSize(220, 220)
					end
				elseif playerRace == "TROLL" then
					if isFemale then
						child.itemModel:SetPoint("CENTER", -6, 2)
						child.itemModel:SetSize(180, 180)
					else
						child.itemModel:SetPoint("CENTER", -2, 2)
						child.itemModel:SetSize(170, 170)
					end
				else -- 血精灵作为回退。
					if isFemale then
						child.itemModel:SetPoint("CENTER", 2, -2)
						child.itemModel:SetSize(180, 180)
					else
						child.itemModel:SetPoint("CENTER", -2, -4)
						child.itemModel:SetSize(190, 190)
					end
				end
			end
		end
	else
		-- 回退行为是清空所有适用的栏位。
		for i, child in ipairs(itemButtons) do
			child:SetID(0)
			child.itemButton:SetID(0)
			child.itemButton:Disable()
			child.itemModel:Hide()
			SetItemButtonTexture(child.itemButton, "Interface\\paperdoll\\UI-PaperDoll-Slot-" .. equipmentSlotIcons[CalculateInverseSlot(CurrentItemSlot)])
		end
	end
end

function OnTransmogrificationFrameLoad(self)
	Title:SetText(L["Transmogrify"])
	Subtitle:SetText(L["Collected Item Appearances"])
	TransmogPaginationText:SetText(string.format(L["Page %s"], 1))

	-- 加载幻化窗口时默认隐藏警告文本。
	TransmogWarningFrame:Hide()
	RegisterEquipmentChangeEvent()

	-- 初始化预览幻化 ID 表。
	for slot, transmogID in pairs(currentTransmogrificationIDs) do
		previewTransmogrificationIDs[slot] = transmogID
	end

	ItemSearchInput:SetText("|cff" .. L["b2b2b2"] .. L["Filter Item Appearance"] .. "|r")
	ItemSearchInput:SetScript("OnEnterPressed", SetSearchTab)

	InitTabSlots()

	-- 在角色信息窗口上创建标签按钮。
	characterTransmogTab = CreateFrame("CheckButton", "CharacterFrameTab6", CharacterFrame, "SpellBookSkillLineTabTemplate")
	characterTransmogTab:SetSize(32, 32);
	characterTransmogTab:SetPoint("TOPRIGHT", CharacterFrame, "TOPRIGHT", 0, -48)
	characterTransmogTab:Show()
	innerCharacterTransmogTab = characterTransmogTab:CreateTexture("Item", "ARTWORK")
	innerCharacterTransmogTab:SetTexture("Interface\\AddOns\\Transmogrification\\assets\\Transmog-Icon")
	innerCharacterTransmogTab:SetAllPoints()
	innerCharacterTransmogTab:Show()
	characterTransmogTab:SetScript("OnEnter", TransmogrifyToolTip)
	characterTransmogTab:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Square", "ADD")
	characterTransmogTab:SetScript("OnClick", function(self) if ( TransmogrificationFrame:IsShown() ) then TransmogrificationFrame:Hide() return; end TransmogrificationFrame:Show() end)
	TransmogCloseButton:SetScript("OnClick", function(self) if ( TransmogrificationFrame:IsShown() ) then TransmogrificationFrame:Hide() return; end TransmogrificationFrame:Show() end)

	PaperDollFrame:SetScript("OnShow", PaperDollFrame_OnShow)

	-- 保存幻化窗口的位置。
	AIO.SavePosition(TransmogrificationFrame)

	-- 幻化窗口初始化时应用设置。
	if Transmogrification and Transmogrification.db then
		local settings = Transmogrification:GetSettings()
		TransmogrificationFrame:SetScale(settings.windowScale)
		TransmogrificationFrame:SetAlpha(settings.windowOpacity)

		if settings.windowLock then
			TransmogrificationFrame:SetMovable(false)
			TransmogrificationFrame:RegisterForDrag()
		else
			TransmogrificationFrame:SetMovable(true)
			TransmogrificationFrame:RegisterForDrag("LeftButton")
		end
	end

	_G["TransmogrificationFrame"] = TransmogrificationFrame
	tinsert(UISpecialFrames, TransmogrificationFrame:GetName())
	TransmogrificationFrame:RegisterEvent("PLAYER_ENTERING_WORLD")
	TransmogrificationFrame:RegisterEvent("UNIT_MODEL_CHANGED")
	TransmogrificationFrame:SetScript("OnEvent", OnEventEnterWorldReloadTransmogIDs)

	SetItemButtonTexture(_G["SaveButton"], "Interface\\AddOns\\Transmogrification\\assets\\Transmog-Icon")

	-- 创建“应用幻化”按钮左侧显示所需金币的文本。
	-- 作为 SaveButton 的子元素创建，使其绘制层级高于按钮自带的装饰纹理（192x96 的 SaveTexture），
	-- 避免被遮挡；并定位在按钮左侧。
	local costText = _G["SaveButton"]:CreateFontString("TransmogCostText", "OVERLAY", "GameFontNormal")
	costText:SetSize(220, 20)
	costText:SetPoint("RIGHT", _G["SaveButton"], "LEFT", -10, 0)
	costText:SetJustifyH("RIGHT")
	costText:Show()

	TransmogModelMouseRotation(TransmogrificationModelFrame)

	-- 更新所有装备图标。
	UpdateAllSlotTextures(false)
end
