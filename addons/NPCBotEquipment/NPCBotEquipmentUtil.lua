-- ============================================================================
-- 兼容层：不同客户端 / 补丁环境下部分 API 可能缺失（SetShown、SetDesaturated、
-- SetEnabled 等），或字体信息获取不到。统一做存在性判断与等价降级，避免
-- "attempt to call method 'xxx' (a nil value)" 导致界面卡在加载态。
-- 本文件最先加载，创建 _G.NPCBotEquipmentUI 并向其注册共享的 Safe* 辅助方法，
-- 供属性页、管理页等其余文件复用（单一来源，避免各文件重复实现）。
-- ============================================================================

local UI = _G.NPCBotEquipmentUI or {}
_G.NPCBotEquipmentUI = UI

local function ResolveDefaultFont()
    if type(STANDARD_TEXT_FONT) == "string" and STANDARD_TEXT_FONT ~= "" then
        return STANDARD_TEXT_FONT
    end
    if GameFontNormal and GameFontNormal.GetFont then
        local font = GameFontNormal:GetFont()
        if type(font) == "string" and font ~= "" then
            return font
        end
    end
    return "Fonts\\FRIZQT__.TTF"
end

local DEFAULT_FONT = ResolveDefaultFont()
local DEFAULT_FONT_SIZE = 12
local DEFAULT_FONT_FLAGS = "OUTLINE"

-- 显示/隐藏：优先 SetShown，缺失时回退到 Show/Hide。
local function SafeSetShown(widget, shown)
    if not widget then
        return
    end
    if widget.SetShown then
        widget:SetShown(shown)
    elseif shown then
        widget:Show()
    else
        widget:Hide()
    end
end

-- 去饱和：缺失时静默忽略（仅影响图标灰度显示，不影响功能）。
local function SafeSetDesaturated(texture, desaturated)
    if texture and texture.SetDesaturated then
        texture:SetDesaturated(desaturated)
    end
end

-- 在现有字体基础上调整字号（sizeOffset 为相对增量，可正可负）。
-- 字体路径 / 字号 / 标志任一获取不到时使用默认值，避免跨客户端报错。
local function SafeAdjustFontSize(fontString, sizeOffset)
    if not fontString or not fontString.GetFont or not fontString.SetFont then
        return
    end
    local font, size, flags = fontString:GetFont()
    if type(font) ~= "string" or font == "" then
        font = DEFAULT_FONT
    end
    if type(size) ~= "number" or size <= 0 then
        size = DEFAULT_FONT_SIZE
    end
    if type(flags) ~= "string" then
        flags = DEFAULT_FONT_FLAGS
    end
    fontString:SetFont(font, math.max(1, size + (sizeOffset or 0)), flags)
end

-- 启用/禁用：优先 SetEnabled，缺失时回退到 Enable/Disable（部分版本的控件只有后者）。
local function SafeSetEnabled(widget, enabled)
    if not widget then
        return
    end
    if widget.SetEnabled then
        widget:SetEnabled(enabled)
    elseif enabled then
        if widget.Enable then
            widget:Enable()
        end
    else
        if widget.Disable then
            widget:Disable()
        end
    end
end

-- EditBox 专用：部分版本的 EditBox 只有 Enable/Disable 而没有 SetEnabled，
-- 且无论能否真正禁用都用文字颜色区分可用 / 禁用状态。
local function SafeEditBoxSetEnabled(edit, enabled)
    if edit.Enable and edit.Disable then
        if enabled then
            edit:Enable()
        else
            edit:Disable()
        end
    end
    if edit.SetTextColor then
        edit:SetTextColor(enabled and 1 or 0.45, enabled and 1 or 0.45, enabled and 1 or 0.45)
    end
end

UI.SafeSetShown = SafeSetShown
UI.SafeSetDesaturated = SafeSetDesaturated
UI.SafeAdjustFontSize = SafeAdjustFontSize
UI.SafeSetEnabled = SafeSetEnabled
UI.SafeEditBoxSetEnabled = SafeEditBoxSetEnabled
UI.DEFAULT_FONT = DEFAULT_FONT
