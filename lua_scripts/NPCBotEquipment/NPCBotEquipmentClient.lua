local AIO = AIO or require("AIO")
if AIO.AddAddon() then
    return
end

local NAMESPACE = "NPCBotEquipment"
local handlers = AIO.AddHandlers(NAMESPACE, {})

local COLUMNS = 4
local ITEM_SIZE = 38
local SPACING = 4
local PADDING = 8
local MAX_VISIBLE_ROWS = 5
local PANEL_WIDTH = PADDING * 2 + ITEM_SIZE * COLUMNS + SPACING * (COLUMNS - 1)
local SLOT_SIZE = 42
local SLOT_COLUMNS = 6
local SLOT_NAMES = {
    "主手", "副手", "远程", "头部", "肩部", "胸部",
    "腰部", "腿部", "脚部", "护腕", "手套", "披风",
    "衬衣", "戒指1", "戒指2", "饰品1", "饰品2", "项链"
}
local QUALITY_COLORS = {
    [0] = { 0.62, 0.62, 0.62 },
    [1] = { 1.00, 1.00, 1.00 },
    [2] = { 0.12, 1.00, 0.00 },
    [3] = { 0.00, 0.44, 0.87 },
    [4] = { 0.64, 0.21, 0.93 },
    [5] = { 1.00, 0.50, 0.00 },
    [6] = { 0.90, 0.80, 0.50 },
    [7] = { 0.90, 0.80, 0.50 }
}

local UI = {
    requestSerial = 0,
    currentBot = nil,
    snapshot = nil,
    candidateButtons = {},
    equipPending = false,
    requestDeadline = nil,
    mutationDeadline = nil
}
_G.NPCBotEquipmentUI = UI

local function NextRequestId()
    UI.requestSerial = UI.requestSerial + 1
    if UI.requestSerial > 2147483647 then
        UI.requestSerial = 1
    end
    return UI.requestSerial
end

local function NormalizeIcon(icon)
    if not icon or icon == "" then
        return "Interface\\Icons\\INV_Misc_QuestionMark"
    end
    if string.find(string.lower(icon), "interface\\", 1, true) == 1 then
        return icon
    end
    return "Interface\\Icons\\" .. icon
end

local function ParseCreatureGuid(unit)
    local guid = unit and UnitGUID(unit)
    if type(guid) ~= "string" or string.sub(guid, 1, 6) ~= "0xF130" or string.len(guid) < 18 then
        return nil
    end

    local entry = tonumber(string.sub(guid, 7, 12), 16)
    local guidLow = tonumber(string.sub(guid, 13, 18), 16)
    if not entry or not guidLow or entry <= 0 or guidLow <= 0 then
        return nil
    end

    return entry, tostring(guidLow)
end

local function CreateBackdrop(frame)
    frame:SetBackdrop({
        bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true,
        tileSize = 16,
        edgeSize = 16,
        insets = { left = 4, right = 4, top = 4, bottom = 4 }
    })
    frame:SetBackdropColor(0.05, 0.05, 0.07, 0.96)
end

local function CreateEquipmentFrame()
    local frame = CreateFrame("Frame", "NPCBotEquipmentFrame", UIParent)
    frame:SetWidth(304)
    frame:SetHeight(232)
    frame:SetPoint("CENTER", UIParent, "CENTER", -170, 40)
    frame:SetFrameStrata("DIALOG")
    frame:SetMovable(true)
    frame:EnableMouse(true)
    frame:RegisterForDrag("LeftButton")
    frame:SetScript("OnDragStart", frame.StartMoving)
    frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
    CreateBackdrop(frame)
    frame:Hide()

    local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
    title:SetPoint("TOP", frame, "TOP", 0, -13)
    title:SetText("NPCBot 装备")
    frame.title = title

    local close = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
    close:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -2, -2)

    local status = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    status:SetPoint("BOTTOMLEFT", frame, "BOTTOMLEFT", 14, 11)
    status:SetPoint("BOTTOMRIGHT", frame, "BOTTOMRIGHT", -14, 11)
    status:SetJustifyH("LEFT")
    status:SetText("右键装备槽查看背包候选")
    frame.status = status

    frame.slots = {}
    for slot = 0, 17 do
        local button = CreateFrame("Button", nil, frame)
        button:SetWidth(SLOT_SIZE)
        button:SetHeight(SLOT_SIZE)
        button:RegisterForClicks("LeftButtonUp", "RightButtonUp")
        button.botSlot = slot

        local column = math.mod(slot, SLOT_COLUMNS)
        local row = math.floor(slot / SLOT_COLUMNS)
        button:SetPoint("TOPLEFT", frame, "TOPLEFT", 15 + column * 46, -43 - row * 48)

        local border = button:CreateTexture(nil, "BACKGROUND")
        border:SetTexture("Interface\\Buttons\\UI-Quickslot2")
        border:SetAllPoints(button)
        button.border = border

        local icon = button:CreateTexture(nil, "ARTWORK")
        icon:SetPoint("TOPLEFT", button, "TOPLEFT", 3, -3)
        icon:SetPoint("BOTTOMRIGHT", button, "BOTTOMRIGHT", -3, 3)
        icon:SetTexture("Interface\\Icons\\INV_Misc_QuestionMark")
        icon:SetVertexColor(0.35, 0.35, 0.35)
        button.icon = icon

        local label = button:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
        label:SetPoint("BOTTOM", button, "BOTTOM", 0, 2)
        label:SetText(SLOT_NAMES[slot + 1])
        button.label = label

        button:SetScript("OnEnter", function(self)
            local slotData = UI.snapshot and UI.snapshot.slots and UI.snapshot.slots[self.botSlot + 1]
            GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
            if slotData and slotData.occupied and slotData.link and slotData.link ~= "" then
                GameTooltip:SetHyperlink(slotData.link)
            else
                GameTooltip:SetText(SLOT_NAMES[self.botSlot + 1], 1, 0.82, 0)
                GameTooltip:AddLine("该槽位当前为空", 0.8, 0.8, 0.8)
            end
            GameTooltip:Show()
        end)
        button:SetScript("OnLeave", function()
            GameTooltip:Hide()
        end)
        button:SetScript("OnClick", function(self, mouseButton)
            if mouseButton == "RightButton" then
                UI:ToggleCandidates(self.botSlot, self)
            end
        end)

        frame.slots[slot + 1] = button
    end

    frame:SetScript("OnHide", function()
        UI:CloseCandidatePanel()
    end)

    table.insert(UISpecialFrames, frame:GetName())
    return frame
end

local function CreateCandidatePanel()
    local panel = CreateFrame("Frame", "NPCBotEquipmentCandidatePanel", UIParent)
    panel:SetWidth(PANEL_WIDTH)
    panel:SetHeight(PADDING * 2 + ITEM_SIZE)
    panel:SetFrameStrata("FULLSCREEN_DIALOG")
    CreateBackdrop(panel)
    panel:Hide()

    local status = panel:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    status:SetPoint("CENTER", panel, "CENTER", 0, 0)
    status:SetText("正在筛选背包装备...")
    panel.status = status

    local scrollFrame = CreateFrame("ScrollFrame", nil, panel, "UIPanelScrollFrameTemplate")
    scrollFrame:SetPoint("TOPLEFT", panel, "TOPLEFT", 4, -4)
    scrollFrame:SetPoint("BOTTOMRIGHT", panel, "BOTTOMRIGHT", -24, 4)
    panel.scrollFrame = scrollFrame

    local content = CreateFrame("Frame", nil, scrollFrame)
    content:SetWidth(PANEL_WIDTH - 28)
    content:SetHeight(ITEM_SIZE + PADDING * 2)
    scrollFrame:SetScrollChild(content)
    panel.content = content

    panel:SetScript("OnHide", function(self)
        self.requestId = nil
        self.botSlot = nil
        self.botGuidLow = nil
        self.anchorButton = nil
        UI.requestDeadline = nil
    end)

    return panel
end

function UI:EnsureFrames()
    if not self.frame then
        self.frame = CreateEquipmentFrame()
    end
    if not self.candidatePanel then
        self.candidatePanel = CreateCandidatePanel()
    end
end

function UI:ShowError(message)
    self:EnsureFrames()
    self.frame.status:SetText(message or "操作失败")
    UIErrorsFrame:AddMessage(message or "操作失败", 1, 0.2, 0.2, 1)
end

function UI:ApplyFullSnapshot(snapshot)
    if type(snapshot) ~= "table" or type(snapshot.slots) ~= "table" then
        return
    end

    self:EnsureFrames()
    self.snapshot = snapshot
    if self.currentBot then
        self.currentBot.equipmentRevision = snapshot.revision
    end

    for slot = 0, 17 do
        local button = self.frame.slots[slot + 1]
        local slotData = snapshot.slots[slot + 1]
        if slotData and slotData.occupied then
            button.icon:SetTexture(NormalizeIcon(slotData.icon))
            button.icon:SetVertexColor(1, 1, 1)
            local color = QUALITY_COLORS[slotData.quality] or QUALITY_COLORS[1]
            button.label:SetTextColor(color[1], color[2], color[3])
        else
            button.icon:SetTexture("Interface\\Icons\\INV_Misc_QuestionMark")
            button.icon:SetVertexColor(0.35, 0.35, 0.35)
            button.label:SetTextColor(0.8, 0.8, 0.8)
        end
    end

    self.frame.status:SetText("右键装备槽查看背包候选")
end

function UI:Open(botEntry, botGuidLow, displayName)
    self:EnsureFrames()
    self:CloseCandidatePanel()
    self.currentBot = {
        entry = botEntry,
        guidLow = tostring(botGuidLow),
        name = displayName or "NPCBot",
        equipmentRevision = ""
    }
    self.snapshot = nil
    self.frame.title:SetText((displayName or "NPCBot") .. " - 装备")
    self.frame.status:SetText("正在读取装备...")
    self.frame:Show()

    local requestId = NextRequestId()
    self.snapshotRequestId = requestId
    AIO.Handle(NAMESPACE, "RequestSnapshot", {
        requestId = requestId,
        botEntry = botEntry,
        botGuidLow = tostring(botGuidLow)
    })
end

function UI:OpenFromUnit(unit)
    local entry, guidLow = ParseCreatureGuid(unit)
    if not entry then
        self:ShowError("该单位不是可识别的 NPCBot")
        return
    end
    self:Open(entry, guidLow, UnitName(unit))
end

function UI:CloseCandidatePanel()
    if not self.candidatePanel then
        return
    end
    self.requestSerial = self.requestSerial + 1
    self.equipPending = false
    self.mutationDeadline = nil
    self.candidatePanel:Hide()
    for _, button in ipairs(self.candidateButtons) do
        button:Hide()
    end
end

function UI:AnchorCandidatePanel(slotButton)
    local panel = self.candidatePanel
    panel:ClearAllPoints()
    panel:SetPoint("TOPLEFT", slotButton, "TOPRIGHT", 8, 0)
    panel:Show()

    if panel:GetRight() and UIParent:GetRight() and panel:GetRight() > UIParent:GetRight() - 8 then
        panel:ClearAllPoints()
        panel:SetPoint("TOPRIGHT", slotButton, "TOPLEFT", -8, 0)
    end

    if panel:GetBottom() and panel:GetBottom() < 8 then
        panel:ClearAllPoints()
        panel:SetPoint("BOTTOMLEFT", slotButton, "BOTTOMRIGHT", 8, 0)
        if panel:GetRight() and UIParent:GetRight() and panel:GetRight() > UIParent:GetRight() - 8 then
            panel:ClearAllPoints()
            panel:SetPoint("BOTTOMRIGHT", slotButton, "BOTTOMLEFT", -8, 0)
        end
    end
end

function UI:ToggleCandidates(botSlot, slotButton)
    if not self.currentBot or not self.snapshot then
        self:ShowError("装备快照尚未就绪")
        return
    end

    self:EnsureFrames()
    local panel = self.candidatePanel
    if panel:IsShown() and panel.botSlot == botSlot then
        self:CloseCandidatePanel()
        return
    end

    for _, button in ipairs(self.candidateButtons) do
        button:Hide()
    end
    panel.content:Hide()
    panel.status:SetText("正在筛选背包装备...")
    panel.status:Show()
    panel:SetHeight(PADDING * 2 + ITEM_SIZE)

    local requestId = NextRequestId()
    panel.requestId = requestId
    panel.botSlot = botSlot
    panel.botGuidLow = self.currentBot.guidLow
    panel.equipmentRevision = self.snapshot.revision
    panel.anchorButton = slotButton
    self.requestDeadline = GetTime() + 5
    self:AnchorCandidatePanel(slotButton)

    AIO.Handle(NAMESPACE, "RequestCandidates", {
        requestId = requestId,
        botEntry = self.currentBot.entry,
        botGuidLow = self.currentBot.guidLow,
        botSlot = botSlot,
        equipmentRevision = self.snapshot.revision or ""
    })
end

function UI:AcquireCandidateButton(index)
    local button = self.candidateButtons[index]
    if button then
        return button
    end

    button = CreateFrame("Button", nil, self.candidatePanel.content)
    button:SetWidth(ITEM_SIZE)
    button:SetHeight(ITEM_SIZE)
    button:RegisterForClicks("LeftButtonUp", "RightButtonUp")

    local border = button:CreateTexture(nil, "BACKGROUND")
    border:SetTexture("Interface\\Buttons\\UI-Quickslot2")
    border:SetAllPoints(button)
    button.border = border

    local icon = button:CreateTexture(nil, "ARTWORK")
    icon:SetPoint("TOPLEFT", button, "TOPLEFT", 3, -3)
    icon:SetPoint("BOTTOMRIGHT", button, "BOTTOMRIGHT", -3, 3)
    button.icon = icon

    local count = button:CreateFontString(nil, "OVERLAY", "NumberFontNormalSmall")
    count:SetPoint("BOTTOMRIGHT", button, "BOTTOMRIGHT", -3, 3)
    button.count = count

    local itemLevel = button:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    itemLevel:SetPoint("TOPLEFT", button, "TOPLEFT", 3, -3)
    button.itemLevel = itemLevel

    button:SetScript("OnEnter", function(self)
        if not self.itemData then
            return
        end
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        if self.itemData.kind == "UNEQUIP" then
            GameTooltip:SetText("卸下", 1, 0.82, 0)
            GameTooltip:AddLine("卸下当前槽位装备", 1, 1, 1)
        elseif self.itemData.link and self.itemData.link ~= "" then
            GameTooltip:SetHyperlink(self.itemData.link)
        end
        GameTooltip:Show()
    end)
    button:SetScript("OnLeave", function()
        GameTooltip:Hide()
    end)
    button:SetScript("OnClick", function(self, mouseButton)
        if mouseButton ~= "LeftButton" or not self.itemData or not self:IsEnabled() or UI.equipPending then
            return
        end
        if IsShiftKeyDown() and self.itemData.link and ChatEdit_InsertLink then
            ChatEdit_InsertLink(self.itemData.link)
            return
        end
        UI:ExecuteCandidate(self.itemData)
    end)

    self.candidateButtons[index] = button
    return button
end

function UI:SetCandidateButtonsEnabled(enabled)
    for _, button in ipairs(self.candidateButtons) do
        if button:IsShown() then
            local canEnable = enabled and (button.itemData.kind ~= "UNEQUIP" or button.itemData.enabled)
            button:SetEnabled(canEnable)
            button.icon:SetDesaturated(not canEnable)
        end
    end
end

function UI:RenderCandidates(candidates)
    local panel = self.candidatePanel
    local slotData = self.snapshot and self.snapshot.slots and self.snapshot.slots[panel.botSlot + 1]
    local displayItems = {
        {
            kind = "UNEQUIP",
            text = "卸下",
            enabled = slotData and slotData.occupied == true
        }
    }

    for _, itemData in ipairs(candidates or {}) do
        table.insert(displayItems, itemData)
    end

    for _, button in ipairs(self.candidateButtons) do
        button:Hide()
    end

    local rows = math.max(1, math.ceil(table.getn(displayItems) / COLUMNS))
    local contentHeight = PADDING * 2 + rows * ITEM_SIZE + math.max(0, rows - 1) * SPACING
    local visibleRows = math.min(rows, MAX_VISIBLE_ROWS)
    local visibleHeight = PADDING * 2 + visibleRows * ITEM_SIZE + math.max(0, visibleRows - 1) * SPACING
    panel.content:SetHeight(contentHeight)
    panel:SetHeight(visibleHeight)
    panel.scrollFrame:SetVerticalScroll(0)

    for index, itemData in ipairs(displayItems) do
        local button = self:AcquireCandidateButton(index)
        local column = math.mod(index - 1, COLUMNS)
        local row = math.floor((index - 1) / COLUMNS)
        button:ClearAllPoints()
        button:SetPoint("TOPLEFT", panel.content, "TOPLEFT", PADDING + column * (ITEM_SIZE + SPACING),
            -PADDING - row * (ITEM_SIZE + SPACING))
        button.itemData = itemData

        if itemData.kind == "UNEQUIP" then
            button.icon:SetTexture("Interface\\Buttons\\UI-GroupLoot-Pass-Up")
            button.count:SetText("")
            button.itemLevel:SetText("")
            button:SetEnabled(itemData.enabled)
            button.icon:SetDesaturated(not itemData.enabled)
        else
            button.icon:SetTexture(NormalizeIcon(itemData.icon))
            button.count:SetText(itemData.count and itemData.count > 1 and itemData.count or "")
            button.itemLevel:SetText(itemData.itemLevel and itemData.itemLevel > 0 and itemData.itemLevel or "")
            local color = QUALITY_COLORS[itemData.quality] or QUALITY_COLORS[1]
            button.itemLevel:SetTextColor(color[1], color[2], color[3])
            button:SetEnabled(true)
            button.icon:SetDesaturated(false)
        end
        button:Show()
    end

    panel.status:Hide()
    panel.content:Show()
    if panel.anchorButton then
        self:AnchorCandidatePanel(panel.anchorButton)
    end
end

function UI:ExecuteCandidate(itemData)
    local panel = self.candidatePanel
    if not self.currentBot or not panel:IsShown() then
        return
    end

    self.equipPending = true
    self:SetCandidateButtonsEnabled(false)
    local requestId = NextRequestId()
    self.mutationRequestId = requestId
    self.mutationDeadline = GetTime() + 5

    if itemData.kind == "UNEQUIP" then
        AIO.Handle(NAMESPACE, "Unequip", {
            requestId = requestId,
            botEntry = self.currentBot.entry,
            botGuidLow = self.currentBot.guidLow,
            botSlot = panel.botSlot,
            expectedEquipmentRevision = panel.equipmentRevision,
            storeToBank = false
        })
        return
    end

    AIO.Handle(NAMESPACE, "EquipCandidate", {
        requestId = requestId,
        botEntry = self.currentBot.entry,
        botGuidLow = self.currentBot.guidLow,
        botSlot = panel.botSlot,
        itemGuid = itemData.itemGuid,
        expectedItemEntry = itemData.entry,
        expectedEquipmentRevision = panel.equipmentRevision,
        storeReplacedToBank = false
    })
end

function UI:ReloadCurrentSlotCandidates()
    local panel = self.candidatePanel
    local slot = panel and panel.botSlot
    local anchor = panel and panel.anchorButton
    if slot and anchor then
        panel:Hide()
        self:ToggleCandidates(slot, anchor)
    end
end

function UI:HandleMutationResult(response)
    if type(response) ~= "table" or response.requestId ~= self.mutationRequestId then
        return
    end
    if not self.currentBot or response.botGuidLow ~= self.currentBot.guidLow then
        return
    end

    self.equipPending = false
    self.mutationDeadline = nil
    if response.ok then
        self:CloseCandidatePanel()
        self:ApplyFullSnapshot(response.snapshot)
        return
    end

    if response.snapshot and response.snapshot.slots then
        self:ApplyFullSnapshot(response.snapshot)
    end
    self:ShowError(response.message)
    if response.refreshRequired or response.code == "STALE_EQUIPMENT" or response.code == "ITEM_MOVED" or
        response.code == "ITEM_NOT_FOUND" then
        self:ReloadCurrentSlotCandidates()
    else
        self:SetCandidateButtonsEnabled(true)
    end
end

function handlers.SnapshotResult(response)
    if type(response) ~= "table" or response.requestId ~= UI.snapshotRequestId then
        return
    end
    if not UI.currentBot or response.botGuidLow ~= UI.currentBot.guidLow then
        return
    end

    if not response.ok then
        UI:ShowError(response.message)
        UI.frame:Hide()
        return
    end
    UI:ApplyFullSnapshot(response.snapshot)
end

function handlers.CandidatesResult(response)
    local panel = UI.candidatePanel
    if type(response) ~= "table" or not panel or not panel:IsShown() then
        return
    end
    if response.requestId ~= panel.requestId or response.botSlot ~= panel.botSlot or
        response.botGuidLow ~= panel.botGuidLow then
        return
    end

    UI.requestDeadline = nil
    if not response.ok then
        panel.content:Hide()
        panel.status:SetText(response.message or "候选请求失败")
        panel.status:Show()
        return
    end

    UI:ApplyFullSnapshot(response.snapshot)
    panel.equipmentRevision = response.equipmentRevision or response.snapshot.revision
    UI:RenderCandidates(response.candidates)
    if response.truncated then
        UI.frame.status:SetText("候选过多，仅显示前 128 件")
    end
end

function handlers.EquipResult(response)
    UI:HandleMutationResult(response)
end

function handlers.UnequipResult(response)
    UI:HandleMutationResult(response)
end

local updateFrame = CreateFrame("Frame")
updateFrame:SetScript("OnUpdate", function()
    local now = GetTime()
    if UI.requestDeadline and now >= UI.requestDeadline then
        UI.requestDeadline = nil
        if UI.candidatePanel and UI.candidatePanel:IsShown() then
            UI.candidatePanel.content:Hide()
            UI.candidatePanel.status:SetText("请求超时，请重新右键该槽位")
            UI.candidatePanel.status:Show()
        end
    end

    if UI.mutationDeadline and now >= UI.mutationDeadline then
        UI.mutationDeadline = nil
        UI.equipPending = false
        UI:SetCandidateButtonsEnabled(true)
        UI:ShowError("操作响应超时，正在核实装备状态")
        if UI.currentBot then
            local requestId = NextRequestId()
            UI.snapshotRequestId = requestId
            AIO.Handle(NAMESPACE, "RequestSnapshot", {
                requestId = requestId,
                botEntry = UI.currentBot.entry,
                botGuidLow = UI.currentBot.guidLow
            })
        end
    end
end)

local POPUP_VALUE = "NPCBOT_EQUIPMENT"
if UnitPopupButtons and UnitPopupMenus then
    UnitPopupButtons[POPUP_VALUE] = { text = "查看 NPCBot 装备", dist = 0 }
    local popupMenus = { "PARTY", "RAID_PLAYER", "RAID" }
    for _, menuName in ipairs(popupMenus) do
        local menu = UnitPopupMenus[menuName]
        if menu then
            local found = false
            for _, value in ipairs(menu) do
                if value == POPUP_VALUE then
                    found = true
                    break
                end
            end
            if not found then
                table.insert(menu, POPUP_VALUE)
            end
        end
    end

    hooksecurefunc("UnitPopup_ShowMenu", function(dropdownMenu)
        local menu = dropdownMenu or UIDROPDOWNMENU_INIT_MENU
        local unit = menu and menu.unit
        local entry = ParseCreatureGuid(unit)
        for level = 1, UIDROPDOWNMENU_MAXLEVELS do
            for index = 1, UIDROPDOWNMENU_MAXBUTTONS do
                local button = _G["DropDownList" .. level .. "Button" .. index]
                if button and button.value == POPUP_VALUE then
                    if entry then
                        button:Show()
                    else
                        button:Hide()
                    end
                end
            end
        end
    end)

    hooksecurefunc("UnitPopup_OnClick", function(button)
        button = button or _G.this
        if not button or button.value ~= POPUP_VALUE then
            return
        end
        local menu = UIDROPDOWNMENU_INIT_MENU
        if menu and menu.unit then
            UI:OpenFromUnit(menu.unit)
        end
    end)
end

SLASH_NPCBOTGEAR1 = "/nbgear"
SlashCmdList.NPCBOTGEAR = function(text)
    local entry, guidLow = string.match(text or "", "^%s*(%d+)%s+(%d+)%s*$")
    if entry and guidLow then
        UI:Open(tonumber(entry), guidLow, "NPCBot")
        return
    end

    if UnitExists("target") then
        UI:OpenFromUnit("target")
        return
    end

    DEFAULT_CHAT_FRAME:AddMessage("用法：选中 NPCBot 后输入 /nbgear，或 /nbgear <entry> <guidLow>")
end
