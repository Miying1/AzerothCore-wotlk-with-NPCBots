local AIO = AIO or require("AIO")

local NAMESPACE = "NPCBotEquipment"
local handlers = AIO.AddHandlers(NAMESPACE, {})

local FRAME_WIDTH = 780
local FRAME_HEIGHT = 600
local SNAPSHOT_CACHE_TTL = 3 * 24 * 60 * 60
local SNAPSHOT_CACHE_MAX = 32
local SNAPSHOT_CACHE_VERSION = 3
local SLOT_SIZE = 42
local SLOT_GAP = 4
local SIDE_SLOT_X = 28
local RIGHT_SLOT_X = 706
local SLOT_TOP = -88
local MAX_VISIBLE_ROWS = 5
local CANDIDATE_SIZE = 38
local CANDIDATE_GAP = 4
local CANDIDATE_PADDING = 8
local CANDIDATE_COLUMNS = 4
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
local SLOT_NAMES = {
    "主手", "副手", "远程", "头部", "肩部", "胸部", "腰部", "腿部", "脚部",
    "护腕", "手套", "披风", "衬衣", "戒指1", "戒指2", "饰品1", "饰品2", "项链"
}
-- 采用玩家 PaperDollFrame 的常见视觉顺序，数值仍对应 NPCBot 原始 18 槽。
-- 左侧为头部、项链、肩部、衬衣、胸部、腰部、腿部、脚部、护腕。
-- 右侧为手套、戒指、饰品、披风和武器。
local LEFT_DISPLAY_SLOTS = { 3, 17, 4, 12, 5, 6, 7, 8, 9 }
local RIGHT_DISPLAY_SLOTS = { 10, 13, 14, 15, 16, 11, 0, 1, 2 }

local SLOT_EMPTY_TEXTURES = {
    [0] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-MainHand",
    [1] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-SecondaryHand",
    [2] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Ranged",
    [3] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Head",
    [4] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Shoulder",
    [5] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Chest",
    [6] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Waist",
    [7] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Legs",
    [8] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Feet",
    [9] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Wrists",
    [10] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Hands",
    [11] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Back",
    [12] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Shirt",
    [13] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Finger",
    [14] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Finger",
    [15] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Trinket",
    [16] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Trinket",
    [17] = "Interface\\PaperDollInfoFrame\\UI-PaperDoll-Slot-Neck"
}

local function GetEmptySlotTexture(slot)
    return SLOT_EMPTY_TEXTURES[slot] or "Interface\\Icons\\INV_Misc_QuestionMark"
end

local UI = {
    requestSerial = 0,
    currentBot = nil,
    currentUnit = nil,
    snapshot = nil,
    snapshotCache = nil,
    snapshotCacheOrder = {},
    candidateButtons = {},
    equipPending = false,
    requestDeadline = nil,
    mutationDeadline = nil,
    activeTab = "装备"
}
UI.namespace = NAMESPACE
UI.handlers = handlers
_G.NPCBotEquipmentUI = UI

local function NextRequestId()
    UI.requestSerial = UI.requestSerial + 1
    if UI.requestSerial > 2147483647 then
        UI.requestSerial = 1
    end
    return UI.requestSerial
end
UI.NextRequestId = NextRequestId

local function SnapshotCacheKey(botEntry, botGuidLow)
    -- SavedVariables 为账号级；加入当前玩家 GUID，防止不同角色复用主人的管理权限缓存。
    local viewerGuid = UnitGUID and UnitGUID("player") or "unknown-player"
    return tostring(viewerGuid) .. ":" .. tostring(botEntry) .. ":" .. tostring(botGuidLow)
end
UI.SnapshotCacheKey = SnapshotCacheKey

local function GetEpochTime()
    if time then
        return time()
    end
    return 0
end

local function CompactSlotData(slotData, fallbackSlot)
    slotData = type(slotData) == "table" and slotData or {}
    local occupied = slotData.occupied == true
    return {
        slot = tonumber(slotData.slot) or fallbackSlot,
        occupied = occupied,
        itemGuid = occupied and tostring(slotData.itemGuid or "0") or "0",
        entry = occupied and (tonumber(slotData.entry) or 0) or 0,
        link = occupied and (slotData.link or "") or "",
        quality = occupied and (tonumber(slotData.quality) or 0) or 0,
        itemLevel = occupied and (tonumber(slotData.itemLevel) or 0) or 0,
        gearScore = occupied and (tonumber(slotData.gearScore) or 0) or 0
    }
end

local function CompactSnapshot(snapshot)
    if type(snapshot) ~= "table" or type(snapshot.slots) ~= "table" then
        return nil
    end
    local compact = {
        botEntry = snapshot.botEntry,
        botGuidLow = snapshot.botGuidLow,
        canManage = snapshot.canManage == true,
        revision = snapshot.revision or "",
        totalGearScore = tonumber(snapshot.totalGearScore) or 0,
        partial = false,
        slots = {}
    }
    for slot = 0, 17 do
        compact.slots[slot + 1] = CompactSlotData(snapshot.slots[slot + 1], slot)
    end
    return compact
end

function UI:InitializeSnapshotCache()
    if type(NPCBotEquipmentCacheDB) ~= "table" then
        NPCBotEquipmentCacheDB = {}
    end
    if NPCBotEquipmentCacheDB.version ~= SNAPSHOT_CACHE_VERSION then
        NPCBotEquipmentCacheDB.snapshots = {}
        NPCBotEquipmentCacheDB.version = SNAPSHOT_CACHE_VERSION
    elseif type(NPCBotEquipmentCacheDB.snapshots) ~= "table" then
        NPCBotEquipmentCacheDB.snapshots = {}
    end

    self.snapshotCache = NPCBotEquipmentCacheDB.snapshots
    self.snapshotCacheOrder = {}
    local now = GetEpochTime()
    for key, entry in pairs(self.snapshotCache) do
        if type(entry) ~= "table" or type(entry.snapshot) ~= "table" or type(entry.cachedAt) ~= "number" or
            now <= 0 or now - entry.cachedAt > SNAPSHOT_CACHE_TTL then
            self.snapshotCache[key] = nil
        else
            local compactSnapshot = CompactSnapshot(entry.snapshot)
            if compactSnapshot then
                entry.snapshot = compactSnapshot
                table.insert(self.snapshotCacheOrder, key)
            else
                self.snapshotCache[key] = nil
            end
        end
    end
    table.sort(self.snapshotCacheOrder, function(left, right)
        return self.snapshotCache[left].cachedAt < self.snapshotCache[right].cachedAt
    end)
    while table.getn(self.snapshotCacheOrder) > SNAPSHOT_CACHE_MAX do
        local oldestKey = table.remove(self.snapshotCacheOrder, 1)
        self.snapshotCache[oldestKey] = nil
    end
end

function UI:EnsureSnapshotCache()
    if not self.snapshotCache then
        self:InitializeSnapshotCache()
    end
end

function UI:DropSnapshotCache(botEntry, botGuidLow)
    self:EnsureSnapshotCache()
    local key = SnapshotCacheKey(botEntry, botGuidLow)
    self.snapshotCache[key] = nil
    for index = table.getn(self.snapshotCacheOrder), 1, -1 do
        if self.snapshotCacheOrder[index] == key then
            table.remove(self.snapshotCacheOrder, index)
        end
    end
end

function UI:TouchSnapshotCache(key)
    for index = table.getn(self.snapshotCacheOrder), 1, -1 do
        if self.snapshotCacheOrder[index] == key then
            table.remove(self.snapshotCacheOrder, index)
        end
    end
    table.insert(self.snapshotCacheOrder, key)
    while table.getn(self.snapshotCacheOrder) > SNAPSHOT_CACHE_MAX do
        local oldestKey = table.remove(self.snapshotCacheOrder, 1)
        self.snapshotCache[oldestKey] = nil
    end
end

function UI:GetCachedSnapshot(botEntry, botGuidLow)
    self:EnsureSnapshotCache()
    local key = SnapshotCacheKey(botEntry, botGuidLow)
    local entry = self.snapshotCache[key]
    if not entry then
        return nil
    end
    local now = GetEpochTime()
    if now <= 0 or now - entry.cachedAt > SNAPSHOT_CACHE_TTL then
        self:DropSnapshotCache(botEntry, botGuidLow)
        return nil
    end
    self:TouchSnapshotCache(key)
    return entry.snapshot
end

function UI:CacheSnapshot(snapshot)
    if not self.currentBot then
        return
    end
    local compactSnapshot = CompactSnapshot(snapshot)
    if not compactSnapshot then
        return
    end
    self:EnsureSnapshotCache()
    local key = SnapshotCacheKey(self.currentBot.entry, self.currentBot.guidLow)
    self.snapshotCache[key] = {
        snapshot = compactSnapshot,
        cachedAt = GetEpochTime()
    }
    self:TouchSnapshotCache(key)
end

function UI:RequestSnapshot()
    if not self.currentBot then
        return
    end
    local requestId = NextRequestId()
    self.snapshotRequestId = requestId
    AIO.Handle(NAMESPACE, "RequestSnapshot", {
        requestId = requestId,
        botEntry = self.currentBot.entry,
        botGuidLow = self.currentBot.guidLow
    })
end

local function GetEntryIcon(entry)
    if entry and entry > 0 and GetItemIcon then
        local icon = GetItemIcon(entry)
        if icon and icon ~= "" then
            return icon
        end
    end
    return "Interface\\Icons\\INV_Misc_QuestionMark"
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

local function GetUnitDisplayInfo(unit)
    local name = unit and UnitName(unit)
    local className
    local classToken
    if unit and UnitClass then
        className, classToken = UnitClass(unit)
    end

    if type(name) ~= "string" or name == "" then
        name = "NPCBot"
    end
    if type(className) ~= "string" or className == "" then
        className = "NPCBot"
    end

    local classColor = classToken and RAID_CLASS_COLORS and RAID_CLASS_COLORS[classToken]
    if classColor then
        className = string.format(
            "|cff%02x%02x%02x%s|r",
            math.floor(classColor.r * 255 + 0.5),
            math.floor(classColor.g * 255 + 0.5),
            math.floor(classColor.b * 255 + 0.5),
            className)
    end
    return name, className
end

local function CreateBackdrop(frame)
    frame:SetBackdrop({
        bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background-Dark",
        edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
        tile = true,
        tileSize = 32,
        edgeSize = 32,
        insets = { left = 11, right = 11, top = 11, bottom = 11 }
    })
    frame:SetBackdropColor(0.08, 0.06, 0.04, 0.98)
    frame:SetBackdropBorderColor(0.75, 0.55, 0.25, 1)
end

local function SetModel(model, unit)
    if unit and model.SetUnit then
        pcall(model.SetUnit, model, unit)
    end
    if model.SetPortraitZoom then
        model:SetPortraitZoom(0.55)
    end
    if model.SetPosition then
        model:SetPosition(0, 0, 0)
    end
    if model.SetFacing then
        model:SetFacing(0)
    end
end
local function CreateSlotButton(frame, slot, side, row)
    local button = CreateFrame("Button", nil, frame)
    button:SetWidth(SLOT_SIZE)
    button:SetHeight(SLOT_SIZE)
    button:RegisterForClicks("LeftButtonUp", "RightButtonUp")
    button:SetFrameLevel(frame:GetFrameLevel() + 4)
    button.botSlot = slot

    local x = side == "left" and SIDE_SLOT_X or RIGHT_SLOT_X
    button:SetPoint("TOPLEFT", frame, "TOPLEFT", x, SLOT_TOP - row * (SLOT_SIZE + SLOT_GAP))

    local slotPanel = CreateFrame("Frame", nil, frame)
    slotPanel:SetWidth(SLOT_SIZE + 12)
    slotPanel:SetHeight(SLOT_SIZE + 12)
    slotPanel:SetPoint("CENTER", button, "CENTER")
    slotPanel:SetFrameLevel(frame:GetFrameLevel() + 1)
    slotPanel:SetBackdrop({
        bgFile = "Interface\\Tooltips\\UI-Tooltip-Background",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true,
        tileSize = 8,
        edgeSize = 8,
        insets = { left = 2, right = 2, top = 2, bottom = 2 }
    })
    slotPanel:SetBackdropColor(0.02, 0.02, 0.015, 0.88)
    slotPanel:SetBackdropBorderColor(0.34, 0.24, 0.12, 1)
    button.slotPanel = slotPanel

    local border = button:CreateTexture(nil, "BACKGROUND")
    border:SetTexture("Interface\\Buttons\\UI-Quickslot2")
    border:SetAllPoints(button)
    border:SetVertexColor(0.82, 0.67, 0.42)
    button.border = border

    local highlight = button:CreateTexture(nil, "HIGHLIGHT")
    highlight:SetTexture("Interface\\Buttons\\ButtonHilight-Square")
    highlight:SetBlendMode("ADD")
    highlight:SetAllPoints(button)
    button.highlight = highlight

    local pushed = button:CreateTexture(nil, "PUSHED")
    pushed:SetTexture("Interface\\Buttons\\UI-Quickslot-Depress")
    pushed:SetAllPoints(button)
    button.pushed = pushed

    local icon = button:CreateTexture(nil, "ARTWORK")
    icon:SetPoint("TOPLEFT", button, "TOPLEFT", 3, -3)
    icon:SetPoint("BOTTOMRIGHT", button, "BOTTOMRIGHT", -3, 3)
    icon:SetTexture("Interface\\Icons\\INV_Misc_QuestionMark")
    icon:SetVertexColor(0.24, 0.24, 0.24)
    button.icon = icon

    local label = button:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    if side == "left" then
        label:SetPoint("LEFT", button, "RIGHT", 9, 0)
        label:SetJustifyH("LEFT")
    else
        label:SetPoint("RIGHT", button, "LEFT", -9, 0)
        label:SetJustifyH("RIGHT")
    end
    label:SetText(SLOT_NAMES[slot + 1])
    label:SetTextColor(0.82, 0.72, 0.52)
    button.label = label

    local itemLevel = button:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    itemLevel:SetPoint("BOTTOM", button, "BOTTOM", 0, 4)
    itemLevel:SetJustifyH("CENTER")
    itemLevel:SetTextColor(1, 0.82, 0)
    itemLevel:SetShadowColor(0, 0, 0, 1)
    itemLevel:SetShadowOffset(1, -1)
    button.itemLevel = itemLevel

    button:SetScript("OnEnter", function(self)
        local slotData = UI.snapshot and UI.snapshot.slots and UI.snapshot.slots[self.botSlot + 1]
        self.slotPanel:SetBackdropBorderColor(0.95, 0.75, 0.28, 1)
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        if slotData and slotData.occupied and slotData.link and slotData.link ~= "" then
            GameTooltip:SetHyperlink(slotData.link)
        else
            GameTooltip:SetText(SLOT_NAMES[self.botSlot + 1], 1, 0.82, 0)
            GameTooltip:AddLine("该槽位当前为空", 0.8, 0.8, 0.8)
            if UI.currentBot and UI.currentBot.canManage then
                GameTooltip:AddLine("右键选择背包中的装备", 0.65, 0.65, 0.65)
            end
        end
        GameTooltip:Show()
    end)
    button:SetScript("OnLeave", function(self)
        self.slotPanel:SetBackdropBorderColor(0.34, 0.24, 0.12, 1)
        GameTooltip:Hide()
    end)
    button:SetScript("OnClick", function(self, mouseButton)
        if mouseButton == "RightButton" and UI.activeTab == "装备" then
            if UI.currentBot and UI.currentBot.canManage then
                UI:ToggleCandidates(self.botSlot, self)
            end
        end
    end)
    return button
end

local function CreateTab(frame, text, index)
    local button = CreateFrame("Button", nil, frame)
    button:SetWidth(116)
    button:SetHeight(32)
    button:SetPoint("BOTTOMLEFT", frame, "BOTTOMLEFT", 202 + (index - 1) * 119, 12)

    local background = button:CreateTexture(nil, "BACKGROUND")
    background:SetTexture("Interface\\PaperDollInfoFrame\\UI-Character-Tab")
    background:SetTexCoord(0, 0.5, 0, 0.75)
    background:SetAllPoints(button)
    button.background = background

    local active = button:CreateTexture(nil, "ARTWORK")
    active:SetTexture("Interface\\PaperDollInfoFrame\\UI-Character-Tab")
    active:SetTexCoord(0.5, 1, 0, 0.75)
    active:SetAllPoints(button)
    active:Hide()
    button.active = active

    local label = button:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    label:SetPoint("CENTER", button, "CENTER", 0, 1)
    label:SetText(text)
    label:SetTextColor(0.75, 0.58, 0.30)
    button.label = label
    button.tabName = text

    button:SetScript("OnEnter", function(self)
        if self.tabName ~= UI.activeTab then
            self.label:SetTextColor(1, 0.82, 0.35)
        end
    end)
    button:SetScript("OnLeave", function(self)
        if self.tabName ~= UI.activeTab then
            self.label:SetTextColor(0.75, 0.58, 0.30)
        end
    end)
    button:SetScript("OnClick", function(self)
        UI:SelectTab(self.tabName)
    end)
    return button
end

local function CreateInset(frame, point, relative, relativePoint, x, y, width, height)
    local inset = CreateFrame("Frame", nil, frame)
    inset:SetPoint(point, relative, relativePoint, x, y)
    inset:SetWidth(width)
    inset:SetHeight(height)
    inset:SetFrameLevel(frame:GetFrameLevel() + 1)
    inset:SetBackdrop({
        bgFile = "Interface\\Tooltips\\UI-Tooltip-Background",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true,
        tileSize = 16,
        edgeSize = 12,
        insets = { left = 3, right = 3, top = 3, bottom = 3 }
    })
    inset:SetBackdropColor(0.015, 0.012, 0.008, 0.72)
    inset:SetBackdropBorderColor(0.33, 0.23, 0.12, 1)
    return inset
end
UI.CreateInset = CreateInset

local function CreateEquipmentFrame()
    local frame = CreateFrame("Frame", "NPCBotEquipmentFrame", UIParent, "UIPanelDialogTemplate")
    frame:SetWidth(FRAME_WIDTH)
    frame:SetHeight(FRAME_HEIGHT)
    frame:SetPoint("CENTER", UIParent, "CENTER", 0, 0)
    frame:SetFrameStrata("DIALOG")
    frame:SetToplevel(true)
    frame:SetMovable(true)
    frame:EnableMouse(true)
    frame:RegisterForDrag("LeftButton")
    frame:SetScript("OnDragStart", frame.StartMoving)
    frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
    frame:EnableKeyboard(false)
    frame:Hide()

    local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    title:SetPoint("TOP", frame, "TOP", 0, -14)
    title:SetText("NPCBot")
    title:SetTextColor(1, 0.82, 0.35)
    frame.title = title

    local subtitle = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    subtitle:SetPoint("TOP", title, "BOTTOM", 0, -4)
    subtitle:SetText("NPCBot 装备查看")
    subtitle:SetTextColor(0.72, 0.65, 0.52)
    frame.subtitle = subtitle

    local close = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
    close:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -5, -5)

    local titleDivider = frame:CreateTexture(nil, "ARTWORK")
    titleDivider:SetTexture("Interface\\Common\\UI-TooltipDivider-Transparent")
    titleDivider:SetPoint("TOPLEFT", frame, "TOPLEFT", 154, -48)
    titleDivider:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -154, -48)
    titleDivider:SetHeight(2)
    titleDivider:SetVertexColor(0.65, 0.46, 0.22, 0.85)
    frame.titleDivider = titleDivider

    local leftInset = CreateInset(frame, "TOPLEFT", frame, "TOPLEFT", 16, -74, 176, 432)
    local rightInset = CreateInset(frame, "TOPRIGHT", frame, "TOPRIGHT", -16, -74, 176, 432)
    local modelInset = CreateInset(frame, "TOPLEFT", frame, "TOPLEFT", 201, -74, 378, 432)
    frame.leftInset = leftInset
    frame.rightInset = rightInset
    frame.modelInset = modelInset

    local model = CreateFrame("PlayerModel", nil, modelInset)
    model:SetPoint("TOPLEFT", modelInset, "TOPLEFT", 8, -8)
    model:SetPoint("BOTTOMRIGHT", modelInset, "BOTTOMRIGHT", -8, 8)
    model:EnableMouse(false)
    frame.model = model

    local modelTop = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    modelTop:SetPoint("TOP", modelInset, "TOP", 0, -10)
    modelTop:SetText("角色预览")
    modelTop:SetTextColor(0.75, 0.58, 0.30)
    frame.modelTop = modelTop

    local modelHint = frame:CreateFontString(nil, "OVERLAY", "GameFontDisableSmall")
    modelHint:SetPoint("BOTTOM", modelInset, "BOTTOM", 0, 12)
    modelHint:SetText("NPCBot 当前模型")
    frame.modelHint = modelHint

    local leftCaption = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    leftCaption:SetPoint("TOP", leftInset, "TOP", 0, -10)
    leftCaption:SetText("装备")
    leftCaption:SetTextColor(0.75, 0.58, 0.30)
    frame.leftCaption = leftCaption

    local rightCaption = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    rightCaption:SetPoint("TOP", rightInset, "TOP", 0, -10)
    rightCaption:SetText("装备")
    rightCaption:SetTextColor(0.75, 0.58, 0.30)
    frame.rightCaption = rightCaption

    local totalGearScore = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    totalGearScore:SetPoint("BOTTOM", modelInset, "BOTTOM", 0, 150)
    totalGearScore:SetJustifyH("CENTER")
    totalGearScore:SetText("GS:0")
    totalGearScore:SetTextColor(1, 0.82, 0)
    totalGearScore:SetShadowColor(0, 0, 0, 1)
    totalGearScore:SetShadowOffset(1, -1)
    frame.totalGearScore = totalGearScore

    frame.slots = {}
    for row, slot in ipairs(LEFT_DISPLAY_SLOTS) do
        frame.slots[slot + 1] = CreateSlotButton(frame, slot, "left", row - 1)
    end
    for row, slot in ipairs(RIGHT_DISPLAY_SLOTS) do
        frame.slots[slot + 1] = CreateSlotButton(frame, slot, "right", row - 1)
    end

    frame.status = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    frame.status:SetPoint("BOTTOMLEFT", frame, "BOTTOMLEFT", 24, 54)
    frame.status:SetPoint("BOTTOMRIGHT", frame, "BOTTOMRIGHT", -24, 54)
    frame.status:SetJustifyH("CENTER")
    frame.status:SetText("右键装备槽查看背包候选")
    frame.status:SetTextColor(0.72, 0.65, 0.52)

    local divider = frame:CreateTexture(nil, "ARTWORK")
    divider:SetTexture("Interface\\Common\\UI-TooltipDivider-Transparent")
    divider:SetPoint("BOTTOMLEFT", frame, "BOTTOMLEFT", 24, 46)
    divider:SetPoint("BOTTOMRIGHT", frame, "BOTTOMRIGHT", -24, 46)
    divider:SetHeight(2)
    divider:SetVertexColor(0.62, 0.43, 0.20, 0.8)
    frame.divider = divider

    frame.tabs = {
        CreateTab(frame, "装备", 1),
        CreateTab(frame, "属性", 2),
        CreateTab(frame, "管理", 3)
    }

    frame:SetScript("OnHide", function()
        UI:CloseCandidatePanel()
    end)
    table.insert(UISpecialFrames, frame:GetName())
    return frame
end

local function CreateCandidatePanel()
    local panel = CreateFrame("Frame", "NPCBotEquipmentCandidatePanel", UIParent)
    panel:SetWidth(CANDIDATE_PADDING * 2 + CANDIDATE_SIZE * CANDIDATE_COLUMNS +
        CANDIDATE_GAP * (CANDIDATE_COLUMNS - 1) + 24)
    panel:SetHeight(CANDIDATE_PADDING * 2 + CANDIDATE_SIZE)
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
    content:SetWidth(panel:GetWidth() - 28)
    content:SetHeight(CANDIDATE_SIZE + CANDIDATE_PADDING * 2)
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

function UI:SetEquipmentContentShown(shown)
    self.frame.leftInset:SetShown(shown)
    self.frame.rightInset:SetShown(shown)
    self.frame.modelInset:SetShown(shown)
    self.frame.modelTop:SetShown(shown)
    self.frame.modelHint:SetShown(shown)
    self.frame.totalGearScore:SetShown(shown)
    self.frame.leftCaption:SetShown(shown)
    self.frame.rightCaption:SetShown(shown)
    for _, button in ipairs(self.frame.slots) do
        button:SetShown(shown)
        button.slotPanel:SetShown(shown)
    end
end

function UI:UpdatePermissionUi()
    self:EnsureFrames()
    local canManage = self.currentBot and self.currentBot.canManage == true
    local firstX = canManage and 202 or 261
    for index, tab in ipairs(self.frame.tabs) do
        tab:ClearAllPoints()
        tab:SetPoint("BOTTOMLEFT", self.frame, "BOTTOMLEFT", firstX + (index - 1) * 119, 12)
    end
    self.frame.tabs[3]:SetShown(canManage)
    if not canManage and self.activeTab == "管理" then
        self.activeTab = "装备"
    end
end

function UI:SelectTab(tabName, preserveCandidatePanel)
    self:EnsureFrames()
    if tabName == "管理" and not (self.currentBot and self.currentBot.canManage) then
        tabName = "装备"
    end
    self.activeTab = tabName
    self:UpdatePermissionUi()
    for _, tab in ipairs(self.frame.tabs) do
        local selected = tab.tabName == tabName
        tab.active:SetShown(selected)
        tab.label:SetTextColor(selected and 1 or 0.75, selected and 0.82 or 0.75, selected and 0.25 or 0.75)
    end

    if not preserveCandidatePanel then
        self:CloseCandidatePanel()
    end
    if self.frame.attributesPanel then
        self.frame.attributesPanel:Hide()
    end
    if self.frame.managementPanel then
        self.frame.managementPanel:Hide()
    end

    if tabName == "装备" then
        self:SetEquipmentContentShown(true)
        self.frame.status:SetText(self.currentBot and self.currentBot.canManage and "右键装备槽查看背包候选" or "")
    elseif tabName == "属性" then
        self:SetEquipmentContentShown(false)
        if self.ShowAttributesTab then
            self:ShowAttributesTab()
        end
    else
        self:SetEquipmentContentShown(false)
        if self.ShowManagementTab then
            self:ShowManagementTab()
        end
    end
end

function UI:ApplyFullSnapshot(snapshot, preserveCandidatePanel)
    if type(snapshot) ~= "table" or type(snapshot.slots) ~= "table" then
        return
    end

    self:EnsureFrames()
    self.snapshot = snapshot
    if self.currentBot then
        self.currentBot.canManage = snapshot.canManage == true
        if snapshot.revision then
            self.currentBot.equipmentRevision = snapshot.revision
        end
    end
    self:CacheSnapshot(snapshot)
    self:UpdatePermissionUi()
    local name = self.currentBot and self.currentBot.name or "NPCBot"
    local className = self.currentBot and self.currentBot.className or "NPCBot"
    self.frame.title:SetText(name .. "[" .. className .. "]")
    SetModel(self.frame.model, self.currentUnit)

    for slot = 0, 17 do
        local button = self.frame.slots[slot + 1]
        local slotData = snapshot.slots[slot + 1]
        if slotData and slotData.occupied then
            button.icon:SetTexture(GetEntryIcon(slotData.entry))
            button.icon:SetVertexColor(1, 1, 1)
            local color = QUALITY_COLORS[slotData.quality] or QUALITY_COLORS[1]
            button.label:SetTextColor(color[1], color[2], color[3])
            button.itemLevel:SetText(slotData.itemLevel and slotData.itemLevel > 0 and slotData.itemLevel or "")
        else
            button.icon:SetTexture(GetEmptySlotTexture(slot))
            button.icon:SetVertexColor(1, 1, 1)
            button.label:SetTextColor(0.82, 0.72, 0.52)
            button.itemLevel:SetText("")
        end
    end
    self.frame.totalGearScore:SetText("GS:" .. tostring(tonumber(snapshot.totalGearScore) or 0))
    self:SelectTab(self.activeTab, preserveCandidatePanel == true)
end

function UI:ApplySnapshotUpdate(update)
    if type(update) ~= "table" or type(update.slots) ~= "table" then
        return false
    end
    if not update.partial then
        self:ApplyFullSnapshot(update, false)
        return true
    end
    if not self.snapshot or type(self.snapshot.slots) ~= "table" then
        self:RequestSnapshot()
        return false
    end

    for _, slotData in pairs(update.slots) do
        local slot = type(slotData) == "table" and tonumber(slotData.slot)
        if slot and slot >= 0 and slot <= 17 then
            self.snapshot.slots[slot + 1] = slotData
        end
    end
    self.snapshot.botEntry = update.botEntry or self.snapshot.botEntry
    self.snapshot.botGuidLow = update.botGuidLow or self.snapshot.botGuidLow
    self.snapshot.canManage = update.canManage == true
    self.snapshot.revision = update.revision or self.snapshot.revision
    self.snapshot.totalGearScore = tonumber(update.totalGearScore) or self.snapshot.totalGearScore or 0
    self.snapshot.partial = false
    self:ApplyFullSnapshot(self.snapshot, false)
    return true
end

function UI:Open(botEntry, botGuidLow, unit)
    self:EnsureFrames()
    self:CloseCandidatePanel()
    local name, className = GetUnitDisplayInfo(unit)
    self.currentBot = {
        entry = botEntry,
        guidLow = tostring(botGuidLow),
        name = name,
        className = className,
        canManage = false,
        equipmentRevision = ""
    }
    self.currentUnit = unit
    self.snapshot = nil
    if self.ResetAttributesModule then
        self:ResetAttributesModule()
    end
    if self.ResetManagementModule then
        self:ResetManagementModule()
    end
    self.activeTab = "装备"
    self.frame.title:SetText(name .. "[" .. className .. "]")
    self.frame.subtitle:SetText("正在读取装备与模型...")
    self.frame.status:SetText("正在读取装备...")
    self.frame:Show()
    self:SelectTab("装备")

    local cachedSnapshot = self:GetCachedSnapshot(botEntry, botGuidLow)
    if cachedSnapshot then
        self.frame.subtitle:SetText("NPCBot 装备查看（客户端缓存）")
        self.frame.status:SetText("已使用客户端缓存；右键槽位时将校验最新状态")
        self:ApplyFullSnapshot(cachedSnapshot, false)
        -- 缓存只用于立即展示；仍向服务端校验最新装备、revision 与当前查看权限。
        self:RequestSnapshot()
        return
    end

    self.frame.subtitle:SetText("正在读取装备与模型...")
    self.frame.status:SetText("正在读取装备...")
    self:RequestSnapshot()
end

function UI:OpenFromUnit(unit)
    local entry, guidLow = ParseCreatureGuid(unit)
    if not entry then
        self:ShowError("该单位不是可识别的 NPCBot")
        return
    end
    self:Open(entry, guidLow, unit)
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
    if slotButton:GetLeft() and slotButton:GetLeft() < UIParent:GetLeft() + 300 then
        panel:SetPoint("TOPLEFT", slotButton, "TOPRIGHT", 8, 0)
    else
        panel:SetPoint("TOPRIGHT", slotButton, "TOPLEFT", -8, 0)
    end
    panel:Show()

    if panel:GetRight() and UIParent:GetRight() and panel:GetRight() > UIParent:GetRight() - 8 then
        panel:ClearAllPoints()
        panel:SetPoint("TOPRIGHT", slotButton, "TOPLEFT", -8, 0)
    end
    if panel:GetBottom() and panel:GetBottom() < 8 then
        panel:ClearAllPoints()
        panel:SetPoint("BOTTOMLEFT", slotButton, "BOTTOMRIGHT", 8, 0)
    end
end

function UI:ToggleCandidates(botSlot, slotButton)
    if not self.currentBot or not self.snapshot then
        self:ShowError("装备快照尚未就绪")
        return
    end
    if not self.currentBot.canManage then
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
    panel:SetHeight(CANDIDATE_PADDING * 2 + CANDIDATE_SIZE)

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
    button:SetWidth(CANDIDATE_SIZE)
    button:SetHeight(CANDIDATE_SIZE)
    button:RegisterForClicks("LeftButtonUp", "RightButtonUp")

    local border = button:CreateTexture(nil, "BACKGROUND")
    border:SetTexture("Interface\\Buttons\\UI-Quickslot2")
    border:SetAllPoints(button)
    button.border = border

    local icon = button:CreateTexture(nil, "ARTWORK")
    icon:SetPoint("TOPLEFT", button, "TOPLEFT", 3, -3)
    icon:SetPoint("BOTTOMRIGHT", button, "BOTTOMRIGHT", -3, 3)
    button.icon = icon

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

    local rows = math.max(1, math.ceil(table.getn(displayItems) / CANDIDATE_COLUMNS))
    local contentHeight = CANDIDATE_PADDING * 2 + rows * CANDIDATE_SIZE +
        math.max(0, rows - 1) * CANDIDATE_GAP
    local visibleRows = math.min(rows, MAX_VISIBLE_ROWS)
    local visibleHeight = CANDIDATE_PADDING * 2 + visibleRows * CANDIDATE_SIZE +
        math.max(0, visibleRows - 1) * CANDIDATE_GAP
    panel.content:SetHeight(contentHeight)
    panel:SetHeight(visibleHeight)
    panel.scrollFrame:SetVerticalScroll(0)

    for index, itemData in ipairs(displayItems) do
        local button = self:AcquireCandidateButton(index)
        local column = (index - 1) % CANDIDATE_COLUMNS
        local row = math.floor((index - 1) / CANDIDATE_COLUMNS)
        button:ClearAllPoints()
        button:SetPoint("TOPLEFT", panel.content, "TOPLEFT",
            CANDIDATE_PADDING + column * (CANDIDATE_SIZE + CANDIDATE_GAP),
            -CANDIDATE_PADDING - row * (CANDIDATE_SIZE + CANDIDATE_GAP))
        button.itemData = itemData

        if itemData.kind == "UNEQUIP" then
            button.icon:SetTexture("Interface\\Buttons\\UI-GroupLoot-Pass-Up")
            button.itemLevel:SetText("")
            button:SetEnabled(itemData.enabled)
            button.icon:SetDesaturated(not itemData.enabled)
        else
            button.icon:SetTexture(GetEntryIcon(itemData.entry))
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
        if self.InvalidateAttributesModule then
            self:InvalidateAttributesModule()
        end
        self:ApplySnapshotUpdate(response.snapshot)
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

function handlers.SnapshotResult(player, response)
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
    UI.frame.subtitle:SetText("NPCBot 装备查看")
    UI:ApplyFullSnapshot(response.snapshot)
end

function handlers.CandidatesResult(player, response)
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

    UI:ApplyFullSnapshot(response.snapshot, true)
    panel.equipmentRevision = response.equipmentRevision or response.snapshot.revision
    UI:RenderCandidates(response.candidates)
    if response.truncated then
        UI.frame.status:SetText("候选过多，仅显示前 128 件")
    end
end

function handlers.EquipResult(player, response)
    UI:HandleMutationResult(response)
end

function handlers.UnequipResult(player, response)
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

    if UI.UpdateAttributesModule then
        UI:UpdateAttributesModule(now)
    end
    if UI.UpdateManagementModule then
        UI:UpdateManagementModule(now)
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
        UI:Open(tonumber(entry), guidLow, nil)
        return
    end

    if UnitExists("target") then
        UI:OpenFromUnit("target")
        return
    end

    DEFAULT_CHAT_FRAME:AddMessage("用法：选中 NPCBot 后输入 /nbgear，或 /nbgear <entry> <guidLow>")
end
