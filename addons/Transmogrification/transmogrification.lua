-- 初始化 Ace3 库。
local addonName, addon = ...
Transmogrification = LibStub("AceAddon-3.0"):NewAddon(addonName, "AceConsole-3.0")
local L = LibStub("AceLocale-3.0"):GetLocale("Transmogrification")

-- 应用幻化时的金币费用，单位为铜币；需与服务端脚本保持一致。
WEAPON_TRANSMOG_COST = 500000
ARMOR_TRANSMOG_COST = 250000

-- 声明插件默认选项。插件选项将全局保存。
local defaultTransmogrificationOptions = {
	global = {
		windowScale = 1.0,
		windowOpacity = 1.0,
		windowLock = false
	}
}

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
 
function Transmogrification:OnInitialize()
	-- 初始化幻化选项数据库与选项表。
	self.db = LibStub("AceDB-3.0"):New("TransmogrificationOptions", defaultTransmogrificationOptions)
	self:RegisterOptions()
	end

function Transmogrification:OnEnable()

		
	-- 应用自定义幻化窗口样式。
	self:ApplyWindowSettings()
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
