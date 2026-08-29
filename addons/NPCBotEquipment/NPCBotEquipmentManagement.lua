local UI = _G.NPCBotEquipmentUI
if not UI then
    return
end

local AIO = AIO or require("AIO")
local NAMESPACE = UI.namespace or "NPCBotEquipment"
local handlers = UI.handlers or AIO.AddHandlers(NAMESPACE, {})

local ROLE_OPTIONS = {
    { value = 1, label = "主坦" },
    { value = 2, label = "副坦" },
    { value = 4, label = "输出" },
    { value = 8, label = "治疗" },
    { value = 16, label = "远程" }
}

-- 职责位：1 主坦克、2 副坦克、4 输出、8 治疗、16 远程。
-- 最终显示仍会与服务端 supportedRoles 取交集，客户端只负责进一步收窄。
local CLASS_ROLE_MASKS = {
    WARRIOR = 1 + 2 + 4,
    PALADIN = 1 + 2 + 4 + 8+16,
    HUNTER = 4 + 16,
    ROGUE = 4,
    PRIEST = 4 + 8 + 16,
    DEATHKNIGHT = 1 + 2 + 4,
    SHAMAN = 4 + 8 + 16,
    MAGE = 4 + 16,
    WARLOCK = 4 + 16,
    DRUID = 1 + 2 + 4 + 8 + 16
}

local function HasRole(mask, role)
    mask = tonumber(mask) or 0
    return (math.floor(mask / role) % 2) == 1
end

local function CreateSection(parent, title, y, height)
    local inset = UI.CreateInset(parent, "TOPLEFT", parent, "TOPLEFT", 0, y, 358, height)
    local heading = inset:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    heading:SetPoint("TOPLEFT", inset, "TOPLEFT", 16, -12)
    heading:SetText(title)
    heading:SetTextColor(0.95, 0.75, 0.28)
    return inset
end

local function CreateCheck(parent, label)
    local check = CreateFrame("CheckButton", nil, parent, "UICheckButtonTemplate")
    check:SetWidth(24)
    check:SetHeight(24)
    local text = check:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    text:SetPoint("LEFT", check, "RIGHT", 2, 0)
    text:SetText(label)
    check.label = text
    return check
end

local function CreateRadio(parent, label)
    local radio = CreateFrame("CheckButton", nil, parent, "UIRadioButtonTemplate")
    radio:SetWidth(20)
    radio:SetHeight(20)
    local text = radio:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    text:SetPoint("LEFT", radio, "RIGHT", 3, 0)
    text:SetText(label)
    radio.label = text
    return radio
end

-- 兼容层：当前运行环境的 EditBox 缺少 Frame 基类方法 Disable/Enable/SetShown，
-- 这里用存在的方法做等价降级，避免 "attempt to call method ... (a nil value)"。
local function SafeEditBoxSetShown(edit, shown)
    if edit.SetShown then
        edit:SetShown(shown)
    elseif shown then
        edit:Show()
    else
        edit:Hide()
    end
end

local function SafeEditBoxSetEnabled(edit, enabled)
    if edit.Enable and edit.Disable then
        if enabled then
            edit:Enable()
        else
            edit:Disable()
        end
    end
    -- 无论是否真正禁用，都用文字颜色区分可用/禁用状态。
    if edit.SetTextColor then
        edit:SetTextColor(enabled and 1 or 0.45, enabled and 1 or 0.45, enabled and 1 or 0.45)
    end
end

local function CreateEditBox(parent, width)
    -- 参考 NetherBot 的输入框写法：不用模板，用 SetBackdrop 手动绘制背景边框。
    local edit = CreateFrame("EditBox", nil, parent)
    edit:SetSize(width, 24)
    edit:SetFontObject("GameFontHighlight")
    edit:SetBackdrop({
        bgFile = "Interface\\Tooltips\\UI-Tooltip-Background",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true, tileSize = 16, edgeSize = 16,
        insets = { left = 4, right = 4, top = 4, bottom = 4 }
    })
    edit:SetBackdropColor(0, 0, 0, 0.85)
    edit:SetBackdropBorderColor(0.4, 0.4, 0.6, 1)
    edit:SetTextInsets(6, 6, 4, 4)
    edit:SetMultiLine(false)
    edit:SetAutoFocus(false)
    return edit
end

local function CreateManagementPanel(frame)
    local panel = CreateFrame("Frame", nil, frame)
    panel:SetPoint("TOPLEFT", frame, "TOPLEFT", 10, -44)
    panel:SetPoint("BOTTOMRIGHT", frame, "BOTTOMRIGHT", -10, 48)
    panel:SetFrameLevel(frame:GetFrameLevel() + 3)
    panel:Hide()

    local roles = CreateSection(panel, "职责", 0, 82)
    panel.roleChecks = {}
    for index, option in ipairs(ROLE_OPTIONS) do
        local check = CreateCheck(roles, option.label)
        check:SetPoint("TOPLEFT", roles, "TOPLEFT", 16 + (index - 1) * 70, -35)
        check.roleValue = option.value
        panel.roleChecks[index] = check
        check:SetScript("OnClick", function(self)
            if panel.rendering then
                return
            end
            panel.rendering = true
            if self.roleValue == 2 and self:GetChecked() then
                panel.roleChecks[1]:SetChecked(true)
            elseif self.roleValue == 1 and not self:GetChecked() then
                panel.roleChecks[2]:SetChecked(false)
            end
            panel.rendering = false
            UI:SubmitManagementChanges()
        end)
    end
    local behavior = CreateSection(panel, "战斗设置", -94, 178)

    local healLabel = behavior:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    healLabel:SetPoint("TOPLEFT", behavior, "TOPLEFT", 16, -42)
    healLabel:SetText("治疗生命阈值")
    panel.healThresholdLabel = healLabel
    local healEdit = CreateEditBox(behavior, 72)
    healEdit:SetPoint("LEFT", healLabel, "RIGHT", 14, 0)
    panel.healThresholdEdit = healEdit
    local healUnit = behavior:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
    healUnit:SetPoint("LEFT", healEdit, "RIGHT", 8, 0)
    healUnit:SetText("%（1-100，整数）")
    panel.healThresholdUnit = healUnit

    local delayLabel = behavior:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    delayLabel:SetPoint("TOPLEFT", behavior, "TOPLEFT", 16, -76)
    delayLabel:SetText("进战延迟")
    panel.engageDelayLabel = delayLabel
    local delayEdit = CreateEditBox(behavior, 72)
    delayEdit:SetPoint("LEFT", delayLabel, "RIGHT", 14, 0)
    panel.engageDelayEdit = delayEdit
    local delayUnit = behavior:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
    delayUnit:SetPoint("LEFT", delayEdit, "RIGHT", 8, 0)
    delayUnit:SetText("秒（0-10）")
    panel.engageDelayUnit = delayUnit

    local angleLabel = behavior:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    angleLabel:SetPoint("TOPLEFT", behavior, "TOPLEFT", 16, -110)
    angleLabel:SetText("攻击角度")
    panel.angleLabel = angleLabel
    local normal = CreateRadio(behavior, "普通")
    normal:SetPoint("LEFT", angleLabel, "RIGHT", 24, 0)
    normal.angleMode = 1
    local avoid = CreateRadio(behavior, "避开正面 AOE")
    avoid:SetPoint("LEFT", normal, "RIGHT", 84, 0)
    avoid.angleMode = 2
    panel.angleRadios = { normal, avoid }
    for _, radio in ipairs(panel.angleRadios) do
        radio:SetScript("OnClick", function(self)
            if panel.rendering then
                return
            end
            panel.rendering = true
            for _, other in ipairs(panel.angleRadios) do
                other:SetChecked(other == self)
            end
            panel.rendering = false
            UI:SubmitManagementChanges()
        end)
    end

    local positioningLabel = behavior:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    positioningLabel:SetPoint("TOPLEFT", behavior, "TOPLEFT", 16, -144)
    positioningLabel:SetText("战斗走位")
    local positioningFollow = CreateRadio(behavior, "禁用")
    positioningFollow:SetPoint("LEFT", positioningLabel, "RIGHT", 24, 0)
    positioningFollow.positioningMode = 0
    local positioningDisable = CreateRadio(behavior, "跟随主人")
    positioningDisable:SetPoint("LEFT", positioningFollow, "RIGHT", 56, 0)
    positioningDisable.positioningMode = 1
    local positioningEnable = CreateRadio(behavior, "启用")
    positioningEnable:SetPoint("LEFT", positioningDisable, "RIGHT", 70, 0)
    positioningEnable.positioningMode = 2
    panel.combatPositioningRadios = { positioningFollow, positioningDisable, positioningEnable }
    for _, radio in ipairs(panel.combatPositioningRadios) do
        radio:SetScript("OnClick", function(self)
            if panel.rendering then
                return
            end
            panel.rendering = true
            for _, other in ipairs(panel.combatPositioningRadios) do
                other:SetChecked(other == self)
            end
            panel.rendering = false
            UI:SubmitManagementChanges()
        end)
    end

    local status = panel:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    status:SetPoint("TOP", behavior, "BOTTOM", 0, -8)
    status:SetText("")
    status:SetTextColor(0.72, 0.65, 0.52)
    panel.status = status

    local function CommitEdit(self)
        UI.managementCommitQueued = true
        self:ClearFocus()
    end
    local function CancelEdit(self)
        panel.rendering = true
        self:ClearFocus()
        panel.rendering = false
        UI:RenderManagement(UI.management)
    end
    local function QueueCommit()
        if not panel.rendering then
            UI.managementCommitQueued = true
        end
    end
    healEdit:SetScript("OnEnterPressed", CommitEdit)
    delayEdit:SetScript("OnEnterPressed", CommitEdit)
    healEdit:SetScript("OnEscapePressed", CancelEdit)
    delayEdit:SetScript("OnEscapePressed", CancelEdit)
    healEdit:SetScript("OnEditFocusLost", QueueCommit)
    delayEdit:SetScript("OnEditFocusLost", QueueCommit)

    return panel
end

function UI:ResetManagementModule()
    self.management = nil
    self.managementBotKey = nil
    self.managementLoading = false
    self.managementPending = false
    self.managementDeadline = nil
    self.managementRequestId = nil
    self.managementUpdateRequestId = nil
    self.managementCommitQueued = false
end

function UI:SetManagementControlsEnabled(enabled)
    local panel = self.frame and self.frame.managementPanel
    if not panel then
        return
    end
    for _, check in ipairs(panel.roleChecks) do
        local supported = check.roleSupported ~= false
        check:SetEnabled(enabled and supported)
        check.label:SetTextColor(enabled and supported and 1 or 0.45, enabled and supported and 1 or 0.45, enabled and supported and 1 or 0.45)
    end
    for _, radio in ipairs(panel.angleRadios) do
        radio:SetEnabled(enabled)
    end
    for _, radio in ipairs(panel.combatPositioningRadios) do
        radio:SetEnabled(enabled)
    end
    if enabled and panel.healThresholdSupported then
        SafeEditBoxSetEnabled(panel.healThresholdEdit, true)
    else
        SafeEditBoxSetEnabled(panel.healThresholdEdit, false)
    end
    SafeEditBoxSetEnabled(panel.engageDelayEdit, enabled)
end

function UI:RenderManagement(management)
    if type(management) ~= "table" or not self.frame or not self.frame.managementPanel then
        return
    end
    local panel = self.frame.managementPanel
    panel.rendering = true
    self.management = management
    self.managementBotKey = UI.SnapshotCacheKey(management.botEntry, management.botGuidLow)

    local supportedRoles = tonumber(management.supportedRoles) or 0
    local classToken = self.currentBot and self.currentBot.classToken
    local classRoleMask = CLASS_ROLE_MASKS[classToken]
    local roles = tonumber(management.roles) or 0
    local visibleRoleIndex = 0
    for _, check in ipairs(panel.roleChecks) do
        local classSupportsRole = not classRoleMask or HasRole(classRoleMask, check.roleValue)
        check.roleSupported = classSupportsRole and HasRole(supportedRoles, check.roleValue)
        check:SetChecked(check.roleSupported and HasRole(roles, check.roleValue))
        check:SetShown(check.roleSupported)
        if check.roleSupported then
            check:ClearAllPoints()
            check:SetPoint("TOPLEFT", check:GetParent(), "TOPLEFT", 16 + visibleRoleIndex * 64, -36)
            visibleRoleIndex = visibleRoleIndex + 1
        end
    end

    panel.healThresholdSupported = management.healThresholdSupported == true
    panel.healThresholdLabel:SetShown(panel.healThresholdSupported)
    SafeEditBoxSetShown(panel.healThresholdEdit, panel.healThresholdSupported)
    panel.healThresholdUnit:SetShown(panel.healThresholdSupported)
    panel.healThresholdEdit:SetText(tostring(tonumber(management.healHealthThreshold) or 95))
    panel.healThresholdUnit:SetText("%（1-100，整数）")
    -- 有坦克职责（主坦或副坦）时隐藏进战延迟和攻击角度，坦克无需这两项设置。
    local hasTankRole = HasRole(roles, 1) or HasRole(roles, 2)
    panel.engageDelayLabel:SetShown(not hasTankRole)
    SafeEditBoxSetShown(panel.engageDelayEdit, not hasTankRole)
    panel.engageDelayUnit:SetShown(not hasTankRole)
    panel.engageDelayEdit:SetText(string.format("%.3g", (tonumber(management.engageDelayMs) or 0) / 1000))
    panel.angleLabel:SetShown(not hasTankRole)
    for _, radio in ipairs(panel.angleRadios) do
        radio:SetShown(not hasTankRole)
        radio:SetChecked(radio.angleMode == tonumber(management.attackAngleMode))
    end
    -- 服务端返回的 combatPositioning 为数字：-1 = 跟随主人，0 = 禁用，1 = 启用。
    -- 提交值（单选按钮 positioningMode）：0 = 跟随主人，1 = 禁用，2 = 启用。
    -- 映射关系：服务端值 + 1 = 单选按钮值（-1→0、0→1、1→2）。
    local combatPositioningMode = (tonumber(management.combatPositioning) or -1) + 1
    for _, radio in ipairs(panel.combatPositioningRadios) do
        radio:SetChecked(radio.positioningMode == combatPositioningMode)
    end
    panel.rendering = false
    self:SetManagementControlsEnabled(not self.managementPending)
end

function UI:BuildManagementRequest()
    local panel = self.frame and self.frame.managementPanel
    if not panel or not self.currentBot then
        return nil, "管理界面尚未就绪"
    end

    local roles = 0
    for _, check in ipairs(panel.roleChecks) do
        if check.roleSupported and check:GetChecked() then
            roles = roles + check.roleValue
        end
    end

    if not HasRole(roles, 1) and not HasRole(roles, 4) and not HasRole(roles, 8) then
        return nil, "至少选择主坦克、伤害输出或治疗中的一项"
    end

    local threshold
    if panel.healThresholdSupported then
        local thresholdText = panel.healThresholdEdit:GetText() or ""
        threshold = tonumber(thresholdText)
        if not threshold or threshold ~= math.floor(threshold) or threshold < 1 or threshold > 100 then
            return nil, "治疗生命阈值必须是 1-100 的整数"
        end
    else
        threshold = tonumber(self.management and self.management.healHealthThreshold) or 95
    end

    local hasTankRole = HasRole(roles, 1) or HasRole(roles, 2)
    local delayMs = 0
    if not hasTankRole then
        local delaySeconds = tonumber(panel.engageDelayEdit:GetText() or "")
        if not delaySeconds or delaySeconds < 0 or delaySeconds > 10 then
            return nil, "进战延迟必须在 0-10 秒之间"
        end
        delayMs = math.floor(delaySeconds * 1000 + 0.5)
    end

    local angleMode = 1
    if not hasTankRole then
        for _, radio in ipairs(panel.angleRadios) do
            if radio:GetChecked() then
                angleMode = radio.angleMode
                break
            end
        end
    end

    -- 战斗走位三态：0 = 跟随主人，1 = 禁用，2 = 启用。
    local combatPositioning = 0
    for _, radio in ipairs(panel.combatPositioningRadios) do
        if radio:GetChecked() then
            combatPositioning = radio.positioningMode
            break
        end
    end

    return {
        botEntry = self.currentBot.entry,
        botGuidLow = self.currentBot.guidLow,
        roles = roles,
        healHealthThreshold = threshold,
        engageDelayMs = delayMs,
        attackAngleMode = angleMode,
        combatPositioning = combatPositioning
    }
end

function UI:SubmitManagementChanges()
    if self.managementPending or not self.currentBot or not self.currentBot.canManage then
        return
    end
    local request, errorMessage = self:BuildManagementRequest()
    if not request then
        self.frame.managementPanel.status:SetText(errorMessage)
        self.frame.managementPanel.status:SetTextColor(1, 0.25, 0.25)
        self:RenderManagement(self.management)
        return
    end

    local current = self.management
    -- 服务端返回的 combatPositioning 为 -1/0/1，提交值为 0/1/2，二者相差 +1，据此比较是否变化。
    local currentCombatPositioning = (tonumber(current and current.combatPositioning) or -1) + 1
    if current and request.roles == tonumber(current.roles) and
        request.healHealthThreshold == tonumber(current.healHealthThreshold) and
        request.engageDelayMs == tonumber(current.engageDelayMs) and
        request.attackAngleMode == tonumber(current.attackAngleMode) and
        request.combatPositioning == currentCombatPositioning then
        return
    end

    request.requestId = UI.NextRequestId()
    self.managementUpdateRequestId = request.requestId
    self.managementPending = true
    self.managementDeadline = GetTime() + 5
    self.frame.managementPanel.status:SetText("正在保存管理设置...")
    self.frame.managementPanel.status:SetTextColor(0.72, 0.65, 0.52)
    self:SetManagementControlsEnabled(false)
    AIO.Handle(NAMESPACE, "UpdateManagement", request)
end

function UI:RequestManagement()
    if self.managementLoading or self.managementPending or not self.currentBot or not self.currentBot.canManage then
        return
    end
    local requestId = UI.NextRequestId()
    self.managementRequestId = requestId
    self.managementLoading = true
    self.managementDeadline = GetTime() + 5
    self.frame.managementPanel.status:SetText("正在读取管理设置...")
    self.frame.managementPanel.status:SetTextColor(0.72, 0.65, 0.52)
    self:SetManagementControlsEnabled(false)
    AIO.Handle(NAMESPACE, "RequestManagement", {
        requestId = requestId,
        botEntry = self.currentBot.entry,
        botGuidLow = self.currentBot.guidLow
    })
end

function UI:ShowManagementTab()
    self:EnsureFrames()
    if not self.currentBot or not self.currentBot.canManage then
        return
    end
    if not self.frame.managementPanel then
        self.frame.managementPanel = CreateManagementPanel(self.frame)
    end
    self.frame.managementPanel:Show()

    -- 管理数据不缓存；每次切换到管理页都重新向服务端请求最新设置。
    self.management = nil
    self.managementBotKey = nil
    self.managementLoading = false
    self.managementDeadline = nil
    self:RequestManagement()
end

function handlers.ManagementResult(player, response)
    if type(response) ~= "table" or response.requestId ~= UI.managementRequestId then
        return
    end
    if not UI.currentBot or response.botGuidLow ~= UI.currentBot.guidLow then
        return
    end
    UI.managementLoading = false
    UI.managementDeadline = nil
    if not response.ok or type(response.management) ~= "table" then
        UI.frame.managementPanel.status:SetText(UI:GetResultMessage(response, "读取管理设置失败"))
        UI.frame.managementPanel.status:SetTextColor(1, 0.25, 0.25)
        UI:SetManagementControlsEnabled(false)
        return
    end
    UI:RenderManagement(response.management)
    UI.frame.managementPanel.status:SetText("管理设置已同步")
    UI.frame.managementPanel.status:SetTextColor(0.35, 0.85, 0.35)
end

function handlers.ManagementUpdateResult(player, response)
    if type(response) ~= "table" or response.requestId ~= UI.managementUpdateRequestId then
        return
    end
    if not UI.currentBot or response.botGuidLow ~= UI.currentBot.guidLow then
        return
    end
    UI.managementPending = false
    UI.managementDeadline = nil
    if not response.ok or type(response.management) ~= "table" then
        UI.frame.managementPanel.status:SetText(UI:GetResultMessage(response, "保存管理设置失败"))
        UI.frame.managementPanel.status:SetTextColor(1, 0.25, 0.25)
        if UI.management then
            UI:RenderManagement(UI.management)
        else
            UI:SetManagementControlsEnabled(false)
        end
        return
    end
    UI:RenderManagement(response.management)
    if UI.InvalidateAttributesModule then
        UI:InvalidateAttributesModule()
    end
    UI.frame.managementPanel.status:SetText("管理设置已保存")
    UI.frame.managementPanel.status:SetTextColor(0.35, 0.85, 0.35)
end

function UI:UpdateManagementModule(now)
    if self.managementCommitQueued then
        self.managementCommitQueued = false
        self:SubmitManagementChanges()
    end
    if self.managementDeadline and now >= self.managementDeadline then
        self.managementDeadline = nil
        self.managementLoading = false
        self.managementPending = false
        if self.frame and self.frame.managementPanel then
            self.frame.managementPanel.status:SetText("管理设置请求超时")
            self.frame.managementPanel.status:SetTextColor(1, 0.25, 0.25)
            self:SetManagementControlsEnabled(true)
        end
    end
end
