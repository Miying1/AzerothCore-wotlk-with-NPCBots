-- 幻化插件选项表。
local addonName, addon = ...
local L = LibStub("AceLocale-3.0"):GetLocale("Transmogrification")

-- 获取选项表的函数。
function GetTransmogrificationOptions()
	local options = {
		name = addonName,
		handler = Transmogrification,
		type = "group",
		args = {
			transmogrificationWindow = {
				name = L["Transmogrification Window Options"],
				order = 1,
				type = "header",
			},
			windowScale = {
				name = "|cffffffff" .. L["Transmogrification Window Scale"],
				desc = "|cffffd200" .. L["Determines the scale of the Transmogrification window."],
				order = 2,
				type = "range",
				min = 0.2,
				max = 4,
				softMin = 0.5,
				softMax = 2,
				step = 0.05,
				width = "double",
				get = function() return Transmogrification:GetSettings().windowScale end,
				set = function(_, value) Transmogrification:UpdateSetting("windowScale", value) end,
			},
			windowOpacity = {
				name = "|cffffffff" .. L["Transmogrification Window Opacity"],
				desc = "|cffffd200" .. L["Determines the opacity of the Transmogrification window."],
				order = 3,
				type = "range",
				min = 0.1,
				max = 1,
				softMin = 0.2,
				softMax = 1,
				step = 0.05,
				width = "double",
				get = function() return Transmogrification:GetSettings().windowOpacity end,
				set = function(_, value) Transmogrification:UpdateSetting("windowOpacity", value) end,
			},
			windowLock = {
				name = "|cffffffff" .. L["Transmogrification Window Lock"],
				desc = "|cffffd200" .. L["Locks the position of the Transmogrification window."],
				order = 4,
				type = "toggle",
				width = "double",
				get = function() return Transmogrification:GetSettings().windowLock end,
				set = function(_, value) Transmogrification:UpdateSetting("windowLock", value) end,
			},
			spacer1 = {
				name = " ",
				desc = " ",
				order = 4,
				type = "description",
				fontSize = "large",
				width = "full",
			},
			spacer2 = {
				name = " ",
				desc = " ",
				order = 5,
				type = "description",
				fontSize = "medium",
				width = "full",
			},
			displayOptions = {
				name = L["Display Options"],
				order = 10,
				type = "header",
			},
			spacer3 = {
				name = " ",
				desc = " ",
				order = 13,
				type = "description",
				fontSize = "large",
				width = "full",
			},
			spacer4 = {
				name = " ",
				desc = " ",
				order = 14,
				type = "description",
				fontSize = "medium",
				width = "full",
			},
		},
	}
	return options
end
