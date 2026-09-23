local UI = _G.NPCBotEquipmentUI
if not UI then
    return
end

local AIO = AIO or require("AIO")
local NAMESPACE = UI.namespace or "NPCBotEquipment"
local handlers = UI.handlers or AIO.AddHandlers(NAMESPACE, {})

-- 复用 NPCBotEquipmentUtil.lua 统一提供的兼容层方法。
local SafeSetShown = UI.SafeSetShown
local SafeSetEnabled = UI.SafeSetEnabled
local SafeEditBoxSetEnabled = UI.SafeEditBoxSetEnabled

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

-- 天赋专精名称复用属性页的同一份编号表（NPCBotEquipmentUtil.lua 中的 UI.SPEC_NAMES）。
local SPEC_NAMES = UI.SPEC_NAMES or {}

-- 可切换天赋的最低 Bot 等级（与 Bot Gossip / 服务端校验保持一致）。
local TALENT_MIN_LEVEL = 10

-- 切换天赋的结果提示：状态类失败单独给出天赋相关文案，其余沿用通用文案。
local TALENT_RESULT_MESSAGES = {
    BUSY_IN_COMBAT = "战斗中或处于施法、受控状态，无法切换天赋",
    NO_PERMISSION = "只有该 Bot 的真正主人才能切换天赋",
    INVALID_REQUEST = "该 Bot 当前无法切换天赋（需 10 级以上常规职业）",
    RATE_LIMITED = "切换过于频繁，请稍后重试"
}

-- 天赋切换由服务端施放 ACTIVATE_SPEC 落地（约 5 秒），服务端返回时会带上 specPending。
-- 此时列表里的专精只是"切换目标"，需等施法结束后重新拉取管理数据才能拿到真正生效的专精。
local TALENT_SWITCH_DELAY_SECONDS = 5
local TALENT_SWITCH_REFRESH_DELAY = TALENT_SWITCH_DELAY_SECONDS + 1
local TALENT_SWITCH_PENDING_TEXT = ("天赋切换中，约 %d 秒后生效"):format(TALENT_SWITCH_DELAY_SECONDS)

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
    local behavior = CreateSection(panel, "战斗设置", -94, 170)

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

    -- 天赋：按职业列出可切换的专精（选项由服务端下发），默认选中当前专精，点击后直接切换。
    local talent = CreateSection(panel, "天赋", -274, 72)
    panel.talentSection = talent
    panel.specRadios = {}
    for index = 1, 3 do
        local radio = CreateRadio(talent, "")
        radio:SetPoint("TOPLEFT", talent, "TOPLEFT", 16 + (index - 1) * 112, -40)
        panel.specRadios[index] = radio
        radio:SetScript("OnClick", function(self)
            if panel.rendering or not self.specValue then
                return
            end
            UI:SubmitTalentChange(self.specValue)
        end)
    end
    -- 首次渲染前先隐藏整栏，避免出现没有选项的空天赋栏。
    SafeSetShown(talent, false)

    local status = panel:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    status:SetPoint("TOP", talent, "BOTTOM", 0, -6)
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
    -- 切换天赋的等待刷新时间：换 Bot / 重置管理模块时一并清除。
    self.talentRefreshAt = nil
end

function UI:SetManagementControlsEnabled(enabled)
    local panel = self.frame and self.frame.managementPanel
    if not panel then
        return
    end
    for _, check in ipairs(panel.roleChecks) do
        local supported = check.roleSupported ~= false
        SafeSetEnabled(check, enabled and supported)
        check.label:SetTextColor(enabled and supported and 1 or 0.45, enabled and supported and 1 or 0.45, enabled and supported and 1 or 0.45)
    end
    for _, radio in ipairs(panel.angleRadios) do
        SafeSetEnabled(radio, enabled)
    end
    for _, radio in ipairs(panel.combatPositioningRadios) do
        SafeSetEnabled(radio, enabled)
    end
    for _, radio in ipairs(panel.specRadios) do
        SafeSetEnabled(radio, enabled and panel.talentSupported == true)
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
        SafeSetShown(check, check.roleSupported)
        if check.roleSupported then
            check:ClearAllPoints()
            check:SetPoint("TOPLEFT", check:GetParent(), "TOPLEFT", 16 + visibleRoleIndex * 64, -36)
            visibleRoleIndex = visibleRoleIndex + 1
        end
    end

    panel.healThresholdSupported = management.healThresholdSupported == true
    SafeSetShown(panel.healThresholdLabel, panel.healThresholdSupported)
    SafeSetShown(panel.healThresholdEdit, panel.healThresholdSupported)
    SafeSetShown(panel.healThresholdUnit, panel.healThresholdSupported)
    panel.healThresholdEdit:SetText(tostring(tonumber(management.healHealthThreshold) or 95))
    panel.healThresholdUnit:SetText("%（1-100，整数）")
    -- 有坦克职责（主坦或副坦）时隐藏进战延迟和攻击角度，坦克无需这两项设置。
    local hasTankRole = HasRole(roles, 1) or HasRole(roles, 2)
    SafeSetShown(panel.engageDelayLabel, not hasTankRole)
    SafeSetShown(panel.engageDelayEdit, not hasTankRole)
    SafeSetShown(panel.engageDelayUnit, not hasTankRole)
    panel.engageDelayEdit:SetText(string.format("%.3g", (tonumber(management.engageDelayMs) or 0) / 1000))
    SafeSetShown(panel.angleLabel, not hasTankRole)
    for _, radio in ipairs(panel.angleRadios) do
        SafeSetShown(radio, not hasTankRole)
        radio:SetChecked(radio.angleMode == tonumber(management.attackAngleMode))
    end
    -- 服务端返回的 combatPositioning 为数字：-1 = 跟随主人，0 = 禁用，1 = 启用。
    -- 提交值（单选按钮 positioningMode）：0 = 跟随主人，1 = 禁用，2 = 启用。
    -- 映射关系：服务端值 + 1 = 单选按钮值（-1→0、0→1、1→2）。
    local combatPositioningMode = (tonumber(management.combatPositioning) or -1) + 1
    for _, radio in ipairs(panel.combatPositioningRadios) do
        radio:SetChecked(radio.positioningMode == combatPositioningMode)
    end

    -- 天赋：服务端给定可切换专精编号，默认绑定当前专精，点击即切换。
    -- 客户端本地校验：未满 10 级的 Bot 不显示天赋切换功能（等级未知时以服务端结果为准，
    -- 服务端同样会按等级与职业复核）。
    local botLevel = tonumber(self.currentBot and self.currentBot.level)
    panel.talentSupported = management.specSwitchSupported == true and
        (not botLevel or botLevel >= TALENT_MIN_LEVEL)
    SafeSetShown(panel.talentSection, panel.talentSupported)
    local specOptions = type(management.specOptions) == "table" and management.specOptions or {}
    local currentSpec = tonumber(management.spec)
    for index, radio in ipairs(panel.specRadios) do
        local spec = panel.talentSupported and tonumber(specOptions[index]) or nil
        radio.specValue = spec
        if spec then
            radio.label:SetText(SPEC_NAMES[spec] or ("专精 " .. spec))
            radio:SetChecked(spec == currentSpec)
            radio:Show()
        else
            radio:Hide()
        end
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
    self.frame.managementPanel.status:SetText("正在保存设置...")
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
    self.frame.managementPanel.status:SetText("正在读取设置...")
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
        UI.frame.managementPanel.status:SetText(UI:GetResultMessage(response, "读取设置失败"))
        UI.frame.managementPanel.status:SetTextColor(1, 0.25, 0.25)
        UI:SetManagementControlsEnabled(false)
        return
    end
    UI:RenderManagement(response.management)
    UI.frame.managementPanel.status:SetText("设置已同步")
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
        UI.frame.managementPanel.status:SetText(UI:GetResultMessage(response, "保存设置失败"))
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
    UI.frame.managementPanel.status:SetText("设置已保存")
    UI.frame.managementPanel.status:SetTextColor(0.35, 0.85, 0.35)
end

-- 切换天赋：点击后立即单独提交一次请求，不与职责 / 战斗设置合并提交。
function UI:SubmitTalentChange(spec)
    local panel = self.frame and self.frame.managementPanel
    if not panel then
        return
    end
    if not self.currentBot or not self.currentBot.canManage or panel.talentSupported ~= true or
        self.managementPending or self.talentRefreshAt then
        -- 条件不满足时回退到服务端已知状态，避免单选按钮停留在被点中的状态。
        self:RenderManagement(self.management)
        if self.talentRefreshAt then
            -- 上一次切换仍在落地（服务端正在施放 ACTIVATE_SPEC），此时再提交只会被判定为施法中。
            panel.status:SetText(TALENT_SWITCH_PENDING_TEXT)
            panel.status:SetTextColor(0.72, 0.65, 0.52)
        end
        return
    end
    -- 与当前专精一致时无需请求。
    if tonumber(spec) == tonumber(self.management and self.management.spec) then
        return
    end

    -- 本地先判断玩家自身是否在战斗中：战斗中禁止切换天赋，避免发出无谓请求（服务端仍会复核）。
    if UnitAffectingCombat and UnitAffectingCombat("player") then
        self:RenderManagement(self.management)
        panel.status:SetText("战斗中无法切换天赋")
        panel.status:SetTextColor(1, 0.25, 0.25)
        return
    end

    panel.rendering = true
    for _, radio in ipairs(panel.specRadios) do
        radio:SetChecked(radio.specValue == spec)
    end
    panel.rendering = false

    local requestId = UI.NextRequestId()
    self.managementUpdateRequestId = requestId
    self.managementPending = true
    self.managementDeadline = GetTime() + 5
    panel.status:SetText("正在切换天赋...")
    panel.status:SetTextColor(0.72, 0.65, 0.52)
    self:SetManagementControlsEnabled(false)
    AIO.Handle(NAMESPACE, "SetTalent", {
        requestId = requestId,
        botEntry = self.currentBot.entry,
        botGuidLow = self.currentBot.guidLow,
        spec = spec
    })
end

-- 切换天赋失败的提示：状态类错误使用天赋专属文案，其余沿用通用文案。
function UI:GetTalentResultMessage(response, fallback)
    local code = type(response) == "table" and response.code or nil
    if code and TALENT_RESULT_MESSAGES[code] then
        return TALENT_RESULT_MESSAGES[code]
    end
    return self:GetResultMessage(response, fallback)
end

function handlers.TalentResult(player, response)
    if type(response) ~= "table" or response.requestId ~= UI.managementUpdateRequestId then
        return
    end
    if not UI.currentBot or response.botGuidLow ~= UI.currentBot.guidLow then
        return
    end
    UI.managementPending = false
    UI.managementDeadline = nil
    if not response.ok or type(response.management) ~= "table" then
        -- 请求被拒绝时不会进入等待刷新流程，避免一直挡住后续操作。
        UI.talentRefreshAt = nil
        UI.frame.managementPanel.status:SetText(UI:GetTalentResultMessage(response, "切换天赋失败"))
        UI.frame.managementPanel.status:SetTextColor(1, 0.25, 0.25)
        if UI.management then
            UI:RenderManagement(UI.management)
        else
            UI:SetManagementControlsEnabled(false)
        end
        return
    end

    -- 服务端只有施法被接受时才回传 specPending：此时 snapshot.spec 是切换目标，
    -- 成败一律以 response.ok 为准（不能用专精是否变化判断，切换要等施法结束才落地）。
    local pending = response.management.specPending == true
    UI:RenderManagement(response.management)
    if UI.InvalidateAttributesModule then
        UI:InvalidateAttributesModule()
    end
    -- 天赋切换可能连带更换装备（例如自动卸下副手），清掉缓存避免展示过期装备。
    UI:DropSnapshotCache(UI.currentBot.entry, UI.currentBot.guidLow)
    if UI.activeTab == "装备" then
        UI:RequestSnapshot()
    end
    if pending then
        UI.talentRefreshAt = GetTime() + TALENT_SWITCH_REFRESH_DELAY
        UI.frame.managementPanel.status:SetText(TALENT_SWITCH_PENDING_TEXT)
        UI.frame.managementPanel.status:SetTextColor(0.72, 0.65, 0.52)
    else
        UI.talentRefreshAt = nil
        UI.frame.managementPanel.status:SetText("天赋已切换")
        UI.frame.managementPanel.status:SetTextColor(0.35, 0.85, 0.35)
    end
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
            self.frame.managementPanel.status:SetText("设置请求超时")
            self.frame.managementPanel.status:SetTextColor(1, 0.25, 0.25)
            self:SetManagementControlsEnabled(true)
        end
    end
    -- 天赋切换的 ACTIVATE_SPEC 施法结束后重新拉取管理数据，
    -- 此时服务端返回的 spec 才是真正生效的专精（请求本身已在 TalentResult 里判过成败）。
    if self.talentRefreshAt and now >= self.talentRefreshAt then
        self.talentRefreshAt = nil
        self:RequestManagement()
    end
end
