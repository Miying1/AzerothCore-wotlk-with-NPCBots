-- 初始化 Ace3 库。
local addonName, addon = ...
Transmogrification = LibStub("AceAddon-3.0"):NewAddon(addonName, "AceConsole-3.0")
local L = LibStub("AceLocale-3.0"):GetLocale("Transmogrification")

-- 应用幻化时的金币费用，单位为铜币；需与服务端脚本保持一致。
WEAPON_TRANSMOG_COST = 0
ARMOR_TRANSMOG_COST = 0

-- 声明插件默认选项。插件选项将全局保存。
local defaultTransmogrificationOptions = {
	global = {
		windowScale = 1.0,
		windowOpacity = 1.0,
		windowLock = false,
		displayNewAppearanceTooltip = true,
		displayCollectionMessages = true
	}
}

-- 禁用物品提示系统（若已禁用）以节省性能。
function Transmogrification:HookItemTooltip()
	if not self.db.global.displayNewAppearanceTooltip then return end
end

-- 挂载系统聊天消息函数。
function Transmogrification:HookChatFilter()
	ChatFrame_AddMessageEventFilter("CHAT_MSG_SYSTEM", function(self, event, msg)
		-- 如果玩家决定隐藏新的物品外观消息，则过滤外观收藏消息。
		if not Transmogrification.db.global.displayCollectionMessages and
			-- 确保新的物品外观消息与服务器发送的本地化文本一致。
			msg:find(L["has been added to your appearance collection."]) then
			return true
		end
		return false
	end)
end

-- 应用自定义幻化窗口样式。
function Transmogrification:ApplyWindowSettings()
	-- 设置幻化窗口缩放比例。
	if not TransmogrificationFrame then return end
	TransmogrificationFrame:SetScale(self.db.global.windowScale)
	
	-- 设置幻化窗口透明度。
	if TransmogrificationFrame.SetAlpha then
		TransmogrificationFrame:SetAlpha(self.db.global.windowOpacity)
	end
	
	-- 如果玩家决定锁定，则锁定幻化窗口。
	if self.db.global.windowLock then
		TransmogrificationFrame:SetMovable(false)
		TransmogrificationFrame:RegisterForDrag()
	else
		TransmogrificationFrame:SetMovable(true)
		TransmogrificationFrame:RegisterForDrag("LeftButton")
	end
end

-- 重载提示函数。
function Transmogrification:DisplayReloadPrompt()
	StaticPopupDialogs["TRANSMOGRIFICATION_RELOAD_PROMPT"] = {
		text = L["Would you like to reload the interface?"],
		button1 = L["Yes"],
		button2 = L["No"],
		OnAccept = function()
			ReloadUI()
		end,
		timeout = 0,
		whileDead = true,
		hideOnEscape = true,
		preferredIndex = 3,
	}
	StaticPopup_Show("TRANSMOGRIFICATION_RELOAD_PROMPT")
end

function Transmogrification:OnInitialize()
	-- 初始化幻化选项数据库与选项表。
	self.db = LibStub("AceDB-3.0"):New("TransmogrificationOptions", defaultTransmogrificationOptions)
	self:RegisterOptions()
	
	-- 注册聊天命令。
	self:RegisterChatCommand("tmog", "HandleSlashCommand")
	self:RegisterChatCommand("transmog", "HandleSlashCommand")
	self:RegisterChatCommand("transmogrify", "HandleSlashCommand")
	self:RegisterChatCommand("transmogrification", "HandleSlashCommand")
	
	-- 如果已收集外观表尚不存在，则进行初始化。
	if CollectedAppearances == nil then
		CollectedAppearances = {}
	end
end

function Transmogrification:OnEnable()
	-- 挂载物品提示系统，以便（启用时）显示“新外观”提示文本。
	self:HookItemTooltip()
	
	-- 挂载系统聊天消息函数，以便（启用时）隐藏新的物品外观系统消息。
	self:HookChatFilter()
	
	-- 应用自定义幻化窗口样式。
	self:ApplyWindowSettings()
end

function Transmogrification:HandleSlashCommand(input)
	-- 如果命令参数是“config(s)”、“option(s)”或“setting(s)”，则显示选项面板。
	if input:trim() == "config" or input:trim() == "configs" or input:trim() == "option" or input:trim() == "options" or input:trim() == "setting" or input:trim() == "settings" then
		InterfaceOptionsFrame_OpenToCategory(addonName)
	-- 如果命令参数是“sync”，则从服务器同步已收集外观到已收集外观表，并在完成后发送重载提示。
	elseif input:trim() == "sync" then
		DEFAULT_CHAT_FRAME:AddMessage("|cffffff00" .. L["Querying the server for collected transmogrification appearances..."] .. "\n")
		ChatFrame1EditBox:SetText(".transmog sync")
		ChatEdit_SendText(ChatFrame1EditBox, 1)
	else
		if TransmogrificationFrame and TransmogrificationFrame:IsShown() then
			TransmogrificationFrame:Hide()
		else
			if TransmogrificationFrame then
				TransmogrificationFrame:Show()
			end
		end
	end
end

-- 在界面插件窗口中注册选项窗口。
function Transmogrification:RegisterOptions()
	LibStub("AceConfig-3.0"):RegisterOptionsTable(addonName, GetTransmogrificationOptions)
	self.optionsFrame = LibStub("AceConfigDialog-3.0"):AddToBlizOptions(addonName, addonName)
end

-- 注册控制台变量选项数据库。
function Transmogrification:GetSettings()
	return self.db.global
end

-- 更新设置。
function Transmogrification:UpdateSetting(key, value)
	if self.db.global[key] ~= nil then
		self.db.global[key] = value
		self:ApplyWindowSettings()
	end
end
