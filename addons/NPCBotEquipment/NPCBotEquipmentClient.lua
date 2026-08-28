local AIO = AIO or require("AIO")

local NAMESPACE = "NPCBotEquipment"
local handlers = AIO.AddHandlers(NAMESPACE, {})

local RESULT_MESSAGES = {
    OK = "操作成功",
    INVALID_REQUEST = "请求参数无效",
    RATE_LIMITED = "操作频繁，请稍后重试",
    BOT_NOT_FOUND = "Bot 不在世界中或当前不可管理",
    DIFFERENT_MAP = "Bot 与你不在同一地图，无法管理装备",
    BOT_DEAD = "Bot 已死亡，无法管理装备",
    NO_PERMISSION = "无权管理该 Bot",
    INVALID_SLOT = "Bot 装备槽无效",
    BUSY_IN_COMBAT = "战斗中不能管理 Bot 装备",
    ITEM_NOT_FOUND = "物品不存在或已离开背包",
    ITEM_MOVED = "物品状态或背包位置已变化",
    ITEM_MISMATCH = "物品实例与请求不匹配",
    STALE_EQUIPMENT = "Bot 装备状态已变化，请重新选择",
    CANT_EQUIP = "该 Bot 无法装备此物品",
    ITEM_CONFLICT = "物品与当前主副手装备冲突",
    NO_BAG_SPACE = "背包空间不足",
    NO_BANK_SPACE = "Bot 装备银行空间不足",
    INTERNAL_ERROR = "Bot 装备服务内部错误"
}

-- 主窗口宽高（像素）
-- 宽度 = 左右栏距边框各 3px + 左右栏各 56px + 模型栏 252px + 三栏之间各 4px 间距
local FRAME_WIDTH = 378
local FRAME_HEIGHT = 460
-- 模型区域宽度（像素），左右装备栏与模型栏间距保留 4px
local MODEL_INSET_WIDTH = 252
-- 左右装备栏宽度（像素）
local SIDE_INSET_WIDTH = 56
-- 左右装备栏距离主窗体边框的间距（像素）
local SIDE_INSET_MARGIN = 3
-- 左右装备栏与模型栏之间的间距（像素）
local SIDE_MODEL_GAP = 4
-- 快照缓存：有效期（秒，3 天）与最大缓存条目数
local SNAPSHOT_CACHE_TTL = 3 * 24 * 60 * 60
local SNAPSHOT_CACHE_MAX = 32
-- 快照缓存数据结构的版本号，结构变化时递增以触发客户端缓存失效重建
local SNAPSHOT_CACHE_VERSION = 3
-- 装备槽图标尺寸（像素，42×42）
local SLOT_SIZE = 42
-- 装备槽之间的间距（像素）
local SLOT_GAP = 3
-- 装备槽在左右装备栏内水平居中，左右栏距离主窗体顶部一致。
-- 左右槽位直接相对主窗体 frame 定位（不再包裹左右栏面板）。
-- 槽位起始位置的垂直偏移（相对主窗体顶部，负值向下）。
-- 与模型栏顶部边框（modelInset 的 y=-34）保持一致，使左右槽位顶部与模型栏顶边水平对齐。
local SLOT_TOP = -38
-- 左右槽位相对主窗体边缘的水平偏移：栏宽 56、槽宽 42，居中后两侧各留 7px，
-- 再加上栏距主窗体边框 3px，故偏移 = 3 + (56 - 42) / 2 = 10。
-- local SLOT_SIDE_X = SIDE_INSET_MARGIN + (SIDE_INSET_WIDTH - SLOT_SIZE) / 2
local SLOT_SIDE_X = 14
-- 底部武器槽（主手/副手/远程）水平起始 X 与垂直 Y 偏移
local WEAPON_SLOT_X = 180
local WEAPON_SLOT_Y = -374
-- 单次点击旋转按钮时模型的旋转步长（弧度）
local MODEL_ROTATION_STEP = 0.15
-- 长按旋转按钮时的持续旋转速度（弧度/秒）
local MODEL_ROTATION_HOLD_SPEED = 1.5
-- 候选装备面板最多同时显示的可见行数
local MAX_VISIBLE_ROWS = 5
-- 候选装备图标尺寸（像素）
local CANDIDATE_SIZE = 38
-- 候选装备图标之间的列间距（水平方向，像素）
local CANDIDATE_GAP = 3
-- 候选装备每行的行高增量：行间距比列间距多 6px，使每行占高增加 6px
local CANDIDATE_ROW_GAP = CANDIDATE_GAP-2
-- 候选装备面板的内边距（像素）：顶部/左右内边距
local CANDIDATE_PADDING = 5
-- 候选装备面板的底部内边距（像素）：比顶部更大，让装备与底部边框留出更多间距
local CANDIDATE_BOTTOM_PADDING = 14
-- 候选装备面板每行显示的列数
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
-- 空槽默认边框颜色（偏灰）
local EMPTY_SLOT_BORDER = { 0.40, 0.40, 0.40 }
-- 设置装备槽边框颜色：装备按品质色显示，空槽用灰色。
-- 边框由 UI-Quickslot2 纹理承担（已 SetDesaturated 去金色），通过 SetVertexColor 设置品质色。
local function SetSlotBorderColor(button, quality)
    local color = (quality ~= nil and QUALITY_COLORS[quality]) or EMPTY_SLOT_BORDER
    if not color then
        color = EMPTY_SLOT_BORDER
    end
    if button.border then
        button.border:SetVertexColor(color[1], color[2], color[3])
    end
    button.currentBorderColor = color
end
local SLOT_NAMES = {
    "主", "副", "远", "头", "肩", "胸", "腰", "腿", "脚",
    "护", "手", "披", "衬", "戒1", "戒2", "饰1", "饰2", "项"
}
-- 采用玩家 PaperDollFrame 的标准纸娃娃布局（中间为模型），数值对应 NPCBot 原始 18 槽。
-- 左侧槽位：头、项、肩、披、胸、衬；腕槽单独放在第 8 行，与右侧饰2对齐。
-- 右侧 8 槽：手、腰、腿、脚、戒1、戒2、饰1、饰2。
-- 模型底部 3 槽：主手、副手、远程（横向排列，不显示槽位文字）。
local LEFT_DISPLAY_SLOTS = { 3, 17, 4, 11, 5, 12 }
local RIGHT_DISPLAY_SLOTS = { 10, 6, 7, 8, 13, 14, 15, 16 }
local BOTTOM_DISPLAY_SLOTS = { 0, 1, 2 }

local SLOT_EMPTY_TEXTURES = {
    [0] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-MainHand",
    [1] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-SecondaryHand",
    [2] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Ranged",
    [3] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Head",
    [4] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Shoulder",
    [5] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Chest",
    [6] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Waist",
    [7] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Legs",
    [8] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Feet",
    [9] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Wrists",
    [10] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Hands",
    [11] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Back",
    [12] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Shirt",
    [13] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Finger",
    [14] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Finger",
    [15] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Trinket",
    [16] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Trinket",
    [17] = "Interface\\PaperDoll\\UI-PaperDoll-Slot-Neck"
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
        ownerName = snapshot.ownerName or "",
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

-- NPCBot 是 creature，客户端 UnitClass 的第一个返回值（本地化职业名）不可靠（可能返回单位名称），
-- 第二个返回值（英文 class token，如 "WARRIOR"）才是可靠的职业标识，据此映射本地化职业名。
local CLASS_LOCALIZED_NAMES = {
    WARRIOR = "战士",
    PALADIN = "圣骑士",
    HUNTER = "猎人",
    ROGUE = "盗贼",
    PRIEST = "牧师",
    DEATHKNIGHT = "死亡骑士",
    SHAMAN = "萨满",
    MAGE = "法师",
    WARLOCK = "术士",
    DRUID = "德鲁伊"
}

local function GetUnitDisplayInfo(unit)
    local name = unit and UnitName(unit)
    local classToken
    if unit and UnitClass then
        classToken = select(2, UnitClass(unit))
    end

    if type(name) ~= "string" or name == "" then
        name = "NPCBot"
    end

    local className = classToken and CLASS_LOCALIZED_NAMES[classToken] or "NPCBot"
    local classColor = classToken and RAID_CLASS_COLORS and RAID_CLASS_COLORS[classToken]
    if classColor then
        className = string.format(
            "|cff%02x%02x%02x%s|r",
            math.floor(classColor.r * 255 + 0.5),
            math.floor(classColor.g * 255 + 0.5),
            math.floor(classColor.b * 255 + 0.5),
            className)
    end
    return name, className, classToken
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
    -- 仅在单位变化时重新加载模型；同一单位重复调用（如快照刷新）不重置模型，保留用户的旋转角度。
    local guid = unit and UnitGUID and UnitGUID(unit) or "none"
    if UI.modelKey == guid then
        return
    end
    UI.modelKey = guid
    if unit and model.SetUnit then
        pcall(model.SetUnit, model, unit)
    end
    if model.SetPortraitZoom then
        model:SetPortraitZoom(0.55)
    end
    if model.SetPosition then
        model:SetPosition(0, 0, 0)
    end
    UI.modelRotation = 0
    if model.SetRotation then
        pcall(model.SetRotation, model, 0)
    end
end

local function ApplyModelRotation(delta)
    local model = UI.frame and UI.frame.model
    if not model then
        return
    end
    UI.modelRotation = (UI.modelRotation or 0) + delta
    if model.SetRotation then
        pcall(model.SetRotation, model, UI.modelRotation)
    elseif model.SetFacing then
        pcall(model.SetFacing, model, UI.modelRotation)
    end
end
UI.ApplyModelRotation = ApplyModelRotation
local function CreateSlotButton(frame, slot, side, index)
    local button = CreateFrame("Button", nil, frame)
    button:SetWidth(SLOT_SIZE)
    button:SetHeight(SLOT_SIZE)
    button:RegisterForClicks("LeftButtonUp", "RightButtonUp")
    button:SetFrameLevel(frame:GetFrameLevel() + 10)
    button.botSlot = slot
    button.side = side

    if side == "left" then
        -- 左栏：相对主窗体左上角定位，水平居中于左栏区域，垂直从 SLOT_TOP 向下按索引排列。
        button:SetPoint("TOPLEFT", frame, "TOPLEFT", SLOT_SIDE_X, SLOT_TOP - index * (SLOT_SIZE + SLOT_GAP))
    elseif side == "right" then
        -- 右栏：相对主窗体右上角定位，与左栏对称，垂直偏移一致保证左右栏顶部对齐。
        button:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -SLOT_SIDE_X, SLOT_TOP - index * (SLOT_SIZE + SLOT_GAP))
    else
        button:SetPoint("TOPLEFT", frame, "TOPLEFT", WEAPON_SLOT_X + index * (SLOT_SIZE + SLOT_GAP), WEAPON_SLOT_Y)
    end

    local slotPanel = CreateFrame("Frame", nil, frame)
    slotPanel:SetWidth(SLOT_SIZE + 8)
    slotPanel:SetHeight(SLOT_SIZE + 8)
    slotPanel:SetPoint("CENTER", button, "CENTER")
    slotPanel:SetFrameLevel(frame:GetFrameLevel() + 7)
    slotPanel:SetBackdrop({
        bgFile = "Interface\\Tooltips\\UI-Tooltip-Background",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true,
        tileSize = 8,
        edgeSize = 8,
        insets = { left = 1, right = 1, top = 1, bottom = 1 }
    })
    slotPanel:SetBackdropColor(0.02, 0.02, 0.015, 0.88)
    slotPanel:SetBackdropBorderColor(0.34, 0.24, 0.12, 1)
    button.slotPanel = slotPanel

    -- 装备槽边框：使用 UI-Quickslot2 边框纹理。
    -- 该纹理本身为金色，SetVertexColor 为乘法调制，金色纹理乘品质色仍是金色调；
    -- 因此先用 SetDesaturated 去饱和（变灰），再乘品质色即可正确显示对应颜色。
    local border = button:CreateTexture(nil, "BACKGROUND")
    border:SetTexture("Interface\\Buttons\\UI-Quickslot2")
    border:SetAllPoints(button)
    -- SetDesaturated 去金色：部分客户端版本可能不支持该方法，先判断存在性避免报错
    if border.SetDesaturated then
        border:SetDesaturated(true)
    end
    border:SetVertexColor(EMPTY_SLOT_BORDER[1], EMPTY_SLOT_BORDER[2], EMPTY_SLOT_BORDER[3])
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

    -- 空槽轮廓纹理（与角色装备栏空槽一致，始终显示在物品图标之下）
    local emptyTexture = button:CreateTexture(nil, "BORDER")
    emptyTexture:SetPoint("TOPLEFT", button, "TOPLEFT", 3, -3)
    emptyTexture:SetPoint("BOTTOMRIGHT", button, "BOTTOMRIGHT", -3, 3)
    emptyTexture:SetTexture(GetEmptySlotTexture(slot))
    button.emptyTexture = emptyTexture

    local icon = button:CreateTexture(nil, "ARTWORK")
    icon:SetPoint("TOPLEFT", button, "TOPLEFT", 3, -3)
    icon:SetPoint("BOTTOMRIGHT", button, "BOTTOMRIGHT", -3, 3)
    icon:Hide()
    button.icon = icon

    -- 空槽名称统一显示在槽位中央；装备图标出现后由快照刷新逻辑隐藏。
    local label = button:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    label:SetPoint("CENTER", button, "CENTER", 0, 0)
    label:SetJustifyH("CENTER")
    label:SetJustifyV("MIDDLE")
    label:SetText(SLOT_NAMES[slot + 1])
    label:SetTextColor(0.82, 0.72, 0.52)
    label:SetShadowColor(0, 0, 0, 1)
    label:SetShadowOffset(1, -1)
    label:Show()
    button.label = label

    local itemLevel = button:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    itemLevel:SetPoint("BOTTOM", button, "BOTTOM", 0, 4)
    itemLevel:SetJustifyH("CENTER")
    itemLevel:SetTextColor(1, 0.82, 0)
    itemLevel:SetShadowColor(0, 0, 0, 1)
    itemLevel:SetShadowOffset(1, -1)
    -- 槽内装等字号比默认槽位字体再小一号
    local itemLevelFont, itemLevelSize, itemLevelFlags = itemLevel:GetFont()
    if itemLevelFont then
        itemLevel:SetFont(itemLevelFont, itemLevelSize - 1, itemLevelFlags)
    end
    button.itemLevel = itemLevel

    button:SetScript("OnEnter", function(self)
        local slotData = UI.snapshot and UI.snapshot.slots and UI.snapshot.slots[self.botSlot + 1]
        -- 悬停时边框高亮为金色
        self.border:SetVertexColor(0.95, 0.75, 0.28)
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
        -- 恢复为该槽位当前品质对应的边框颜色（或空槽灰色）
        local color = self.currentBorderColor or EMPTY_SLOT_BORDER
        self.border:SetVertexColor(color[1], color[2], color[3])
        GameTooltip:Hide()
    end)
    button:SetScript("OnClick", function(self, mouseButton)
        if mouseButton == "RightButton" and UI.activeTab == "装备" then
            if UI.currentBot and UI.currentBot.canManage then
                UI:ToggleCandidates(self.botSlot, self)
            end
        elseif mouseButton == "LeftButton" then
            local panel = UI.candidatePanel
            if panel and panel:IsShown() and panel.botSlot ~= self.botSlot then
                UI:CloseCandidatePanel()
            end
        end
    end)
    return button
end

local function CreateTab(frame, text, index)
    local button = CreateFrame("Button", nil, frame)
    button:SetWidth(104)
    button:SetHeight(30) 
    button:SetPoint("BOTTOMLEFT", frame, "BOTTOMLEFT", 83 + (index - 1) * 107, 98)

    -- 底图：经典纸娃娃选项卡纹理
    local background = button:CreateTexture(nil, "BACKGROUND")
    background:SetTexture("Interface\\PaperDollInfoFrame\\UI-Character-Tab")
    background:SetTexCoord(0, 0.5, 0, 0.75)
    background:SetAllPoints(button)
    background:SetVertexColor(0.72, 0.72, 0.72)
    button.background = background

    -- 选中态底图：高亮纹理段
    local active = button:CreateTexture(nil, "ARTWORK")
    active:SetTexture("Interface\\PaperDollInfoFrame\\UI-Character-Tab")
    active:SetTexCoord(0.5, 1, 0, 0.75)
    active:SetAllPoints(button)
    active:Hide()
    button.active = active

    -- 悬停高亮：叠加金色柔光
    local hover = button:CreateTexture(nil, "HIGHLIGHT")
    hover:SetTexture("Interface\\Buttons\\ButtonHilight-Square")
    hover:SetBlendMode("ADD")
    hover:SetAllPoints(button)
    hover:SetVertexColor(0.55, 0.40, 0.15, 0.75)
    hover:Hide()
    button.hover = hover

    -- 选中态顶部亮条：醒目金色，横贯选项卡上沿
    local activeTop = button:CreateTexture(nil, "OVERLAY")
    activeTop:SetTexture("Interface\\Common\\UI-TooltipDivider-Transparent")
    activeTop:SetPoint("TOPLEFT", button, "TOPLEFT", 2, -1)
    activeTop:SetPoint("TOPRIGHT", button, "TOPRIGHT", -2, -1)
    activeTop:SetHeight(3)
    activeTop:SetVertexColor(1, 0.82, 0.35, 1)
    activeTop:Hide()
    button.activeTop = activeTop

    -- 选中态底部亮条
    local activeBottom = button:CreateTexture(nil, "OVERLAY")
    activeBottom:SetTexture("Interface\\Common\\UI-TooltipDivider-Transparent")
    activeBottom:SetPoint("BOTTOMLEFT", button, "BOTTOMLEFT", 2, 2)
    activeBottom:SetPoint("BOTTOMRIGHT", button, "BOTTOMRIGHT", -2, 2)
    activeBottom:SetHeight(2)
    activeBottom:SetVertexColor(1, 0.82, 0.35, 0.9)
    activeBottom:Hide()
    button.activeBottom = activeBottom

    local label = button:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    label:SetPoint("CENTER", button, "CENTER", 0, 1)
    label:SetText(text)
    label:SetTextColor(0.62, 0.46, 0.24)
    label:SetShadowColor(0, 0, 0, 1)
    label:SetShadowOffset(1, -1)
    button.label = label
    button.tabName = text

    -- 刷新选项卡视觉状态（选中 / 悬停 / 普通）
    local function RefreshVisual(self)
        local selected = self.tabName == UI.activeTab
        self.active:SetShown(selected)
        self.activeTop:SetShown(selected)
        self.activeBottom:SetShown(selected)
        self.background:SetVertexColor(selected and 1 or 0.72, selected and 1 or 0.72, selected and 1 or 0.72)
        self.hover:Hide()
        self.label:SetTextColor(selected and 1 or 0.62, selected and 0.86 or 0.46, selected and 0.42 or 0.24)
    end
    button.RefreshVisual = RefreshVisual

    button:SetScript("OnEnter", function(self)
        if self.tabName ~= UI.activeTab then
            self.label:SetTextColor(1, 0.82, 0.35)
            self.hover:Show()
        end
    end)
    button:SetScript("OnLeave", function(self)
        self.hover:Hide()
        if self.tabName ~= UI.activeTab then
            self.label:SetTextColor(0.62, 0.46, 0.24)
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
    frame:SetFrameStrata("FULLSCREEN_DIALOG")
    frame:SetFrameLevel(20)
    frame:SetToplevel(true)
    frame:SetMovable(true)
    frame:EnableMouse(true)
    frame:RegisterForDrag("LeftButton")
    frame:SetScript("OnDragStart", frame.StartMoving)
    frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
    frame:EnableKeyboard(false)
    frame:Hide()

    local title = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    title:SetPoint("TOP", frame, "TOP", 0, -8)
    title:SetJustifyH("CENTER")
    title:SetText("NPCBot")
    title:SetTextColor(1, 0.82, 0.35)
    frame.title = title

    local subtitle = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    subtitle:SetPoint("TOP", title, "BOTTOM", 0, -2)
    subtitle:SetText("")
    subtitle:SetTextColor(0.72, 0.65, 0.52)
    frame.subtitle = subtitle

    -- 注意：UIPanelDialogTemplate 已内置 $parentCloseButton（右上角关闭按钮），无需再手动创建，避免出现两个关闭按钮。

    local titleDivider = frame:CreateTexture(nil, "ARTWORK")
    titleDivider:SetTexture("Interface\\Common\\UI-TooltipDivider-Transparent")
    titleDivider:SetPoint("TOPLEFT", frame, "TOPLEFT", 42, -28)
    titleDivider:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -42, -28)
    titleDivider:SetHeight(2)
    titleDivider:SetVertexColor(0.65, 0.46, 0.22, 0.85)
    frame.titleDivider = titleDivider

    -- 仅保留模型栏面板；左右装备栏不再用面板包裹，槽位直接相对主窗体定位。
    local modelInset = CreateInset(frame, "TOPLEFT", frame, "TOPLEFT",
        SIDE_INSET_MARGIN + SIDE_INSET_WIDTH + SIDE_MODEL_GAP, -34, MODEL_INSET_WIDTH, 368)
    frame.modelInset = modelInset

    local model = CreateFrame("PlayerModel", nil, modelInset)
    model:SetPoint("TOPLEFT", modelInset, "TOPLEFT", 6, -6)
    model:SetPoint("BOTTOMRIGHT", modelInset, "BOTTOMRIGHT", -6, 6)
    model:EnableMouse(true)
    frame.model = model

    -- local leftCaption = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    -- leftCaption:SetPoint("TOP", leftInset, "TOP", 0, -10)
    -- leftCaption:SetText("装备")
    -- leftCaption:SetTextColor(0.75, 0.58, 0.30)
    -- frame.leftCaption = leftCaption

    -- local rightCaption = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    -- rightCaption:SetPoint("TOP", rightInset, "TOP", 0, -10)
    -- rightCaption:SetText("装备")
    -- rightCaption:SetTextColor(0.75, 0.58, 0.30)
    -- frame.rightCaption = rightCaption

    local totalGearScore = modelInset:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    totalGearScore:SetPoint("TOPRIGHT", modelInset, "TOPRIGHT", -10, -12)
    totalGearScore:SetJustifyH("RIGHT")
    totalGearScore:SetText("GS:0")
    totalGearScore:SetTextColor(1, 0.82, 0)
    totalGearScore:SetShadowColor(0, 0, 0, 1)
    totalGearScore:SetShadowOffset(1, -1)
    frame.totalGearScore = totalGearScore

    local function CreateRotateButton(isLeft)
        local button = CreateFrame("Button", nil, modelInset)
        button:SetWidth(24)
        button:SetHeight(24)
        button:SetFrameLevel(modelInset:GetFrameLevel() + 2)
        button:SetPoint("TOP", modelInset, "TOP", isLeft and -42 or 42, -10)
        button:SetNormalTexture(isLeft and "Interface\\Buttons\\UI-SpellbookIcon-PrevPage-Up"
            or "Interface\\Buttons\\UI-SpellbookIcon-NextPage-Up")
        button:SetHighlightTexture("Interface\\Buttons\\UI-Common-MouseHilight")
        button:SetPushedTexture(isLeft and "Interface\\Buttons\\UI-SpellbookIcon-PrevPage-Down"
            or "Interface\\Buttons\\UI-SpellbookIcon-NextPage-Down")
        -- 长按持续旋转：按下时先旋转一步并开启旋转，松开或按钮隐藏时停止。
        local direction = isLeft and 1 or -1
        button:SetScript("OnMouseDown", function(self)
            self.rotating = true
            ApplyModelRotation(direction * MODEL_ROTATION_STEP)
        end)
        button:SetScript("OnMouseUp", function(self)
            self.rotating = false
        end)
        button:SetScript("OnHide", function(self)
            self.rotating = false
        end)
        button:SetScript("OnUpdate", function(self, elapsed)
            if self.rotating then
                ApplyModelRotation(direction * MODEL_ROTATION_HOLD_SPEED * elapsed)
            end
        end)
        return button
    end
    frame.rotateLeft = CreateRotateButton(true)
    frame.rotateRight = CreateRotateButton(false)

    local function OnModelMouseDown(self, mouseButton)
        if mouseButton == "LeftButton" then
            UI.rotatingModel = true
            local scale = UIParent:GetEffectiveScale()
            local x = GetCursorPosition()
            UI.rotateStartX = x / scale
        end
    end
    local function OnModelMouseUp()
        UI.rotatingModel = false
    end
    local function OnModelUpdate(self, elapsed)
        if not UI.rotatingModel then
            return
        end
        local scale = UIParent:GetEffectiveScale()
        local x = GetCursorPosition()
        local cursorX = x / scale
        local deltaX = cursorX - (UI.rotateStartX or cursorX)
        if deltaX ~= 0 then
            UI.rotateStartX = cursorX
            ApplyModelRotation(-deltaX * 0.01)
        end
    end
    model:SetScript("OnMouseDown", OnModelMouseDown)
    model:SetScript("OnMouseUp", OnModelMouseUp)
    model:SetScript("OnUpdate", OnModelUpdate)

    frame.slots = {}
    for row, slot in ipairs(LEFT_DISPLAY_SLOTS) do
        frame.slots[slot + 1] = CreateSlotButton(frame, slot, "left", row - 1)
    end
    frame.slots[9 + 1] = CreateSlotButton(frame, 9, "left", 7)
    for row, slot in ipairs(RIGHT_DISPLAY_SLOTS) do
        frame.slots[slot + 1] = CreateSlotButton(frame, slot, "right", row - 1)
    end
    for index, slot in ipairs(BOTTOM_DISPLAY_SLOTS) do
        local button = CreateSlotButton(frame, slot, "bottom", index - 1)
        button:SetParent(modelInset)
        button:ClearAllPoints()
        button:SetPoint("BOTTOM", modelInset, "BOTTOM", (index - 2) * (SLOT_SIZE + SLOT_GAP), 10)
        button:SetFrameLevel(modelInset:GetFrameLevel() + 10)
        button.slotPanel:SetParent(modelInset)
        button.slotPanel:ClearAllPoints()
        button.slotPanel:SetPoint("CENTER", button, "CENTER")
        button.slotPanel:SetFrameLevel(modelInset:GetFrameLevel() + 7)
        frame.slots[slot + 1] = button
    end

    frame.status = frame:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    frame.status:SetPoint("BOTTOMLEFT", frame, "BOTTOMLEFT", 12, 34)
    frame.status:SetPoint("BOTTOMRIGHT", frame, "BOTTOMRIGHT", -12, 34)
    frame.status:SetJustifyH("CENTER")
    frame.status:SetText("")
    frame.status:SetTextColor(0.72, 0.65, 0.52)

    local divider = frame:CreateTexture(nil, "ARTWORK")
    divider:SetTexture("Interface\\Common\\UI-TooltipDivider-Transparent")
    divider:SetPoint("BOTTOMLEFT", frame, "BOTTOMLEFT", 12, 16)
    divider:SetPoint("BOTTOMRIGHT", frame, "BOTTOMRIGHT", -12, 16)
    divider:SetHeight(2)
    divider:SetVertexColor(0.62, 0.43, 0.20, 0.8)
    frame.divider = divider

    frame.tabs = {
        CreateTab(frame, "装备", 1),
        CreateTab(frame, "属性", 2),
        CreateTab(frame, "管理", 3)
    }
    for _, tab in ipairs(frame.tabs) do
        tab:SetFrameLevel(frame:GetFrameLevel() + 12)
    end

    frame:SetScript("OnHide", function()
        UI:CloseCandidatePanel()
    end)
    frame:HookScript("OnMouseDown", function()
        if UI.candidatePanel and UI.candidatePanel:IsShown() then
            UI:CloseCandidatePanel()
        end
    end)
    table.insert(UISpecialFrames, frame:GetName())
    return frame
end

local function CreateCandidatePanel()
    local panel = CreateFrame("Frame", "NPCBotEquipmentCandidatePanel", UIParent)
    panel:SetWidth(CANDIDATE_PADDING * 2 + CANDIDATE_SIZE * CANDIDATE_COLUMNS +
        CANDIDATE_GAP * (CANDIDATE_COLUMNS - 1) + 24)
    panel:SetHeight(CANDIDATE_PADDING + CANDIDATE_BOTTOM_PADDING + CANDIDATE_SIZE)
    -- 使用 TOOLTIP 层级（高于主框架的 FULLSCREEN_DIALOG），确保候选列表不被遮挡
    panel:SetFrameStrata("TOOLTIP")
    panel:SetFrameLevel(100)
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
    content:SetHeight(CANDIDATE_SIZE + CANDIDATE_PADDING + CANDIDATE_BOTTOM_PADDING)
    scrollFrame:SetScrollChild(content)
    panel.content = content

    panel:SetScript("OnHide", function(self)
        self.requestId = nil
        self.botSlot = nil
        self.botGuidLow = nil
        self.anchorButton = nil
        UI.requestDeadline = nil
    end)

    -- 点击外部关闭层：覆盖主框架以外的区域，点击任意其他位置即关闭候选面板
    local clickCatcher = CreateFrame("Frame", "NPCBotEquipmentClickCatcher", UIParent)
    clickCatcher:SetFrameStrata("FULLSCREEN_DIALOG")
    clickCatcher:SetFrameLevel(10)
    clickCatcher:SetAllPoints(UIParent)
    clickCatcher:EnableMouse(true)
    clickCatcher:Hide()
    clickCatcher:SetScript("OnMouseDown", function()
        UI:CloseCandidatePanel()
    end)
    panel.clickCatcher = clickCatcher

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

function UI:GetResultMessage(response, fallback)
    return response and RESULT_MESSAGES[response.code] or fallback or "操作失败"
end

function UI:ShowError(message)
    DEFAULT_CHAT_FRAME:AddMessage(message or "操作失败", 1, 1, 0)
end

function UI:ValidateCurrentBotRecord()
    if not self.currentBot or not self.currentBot.entry or not self.currentBot.guidLow then
        return false, "NPCBot 装备数据已失效，装备界面已关闭"
    end
    return true
end

function UI:CloseForInvalidBot(message)
    self:CloseCandidatePanel()
    if self.frame then
        self.frame:Hide()
    end
    self.currentBot = nil
    self.currentUnit = nil
    self.snapshot = nil
    if message and message ~= "" then
        self:ShowError(message)
    end
end

function UI:ShouldCloseForResponse(response)
    return response and (response.code == "BOT_NOT_FOUND" or response.code == "DIFFERENT_MAP")
end

function UI:SetEquipmentContentShown(shown)
    self.frame.modelInset:SetShown(shown)
    self.frame.totalGearScore:SetShown(shown)
    self.frame.rotateLeft:SetShown(shown)
    self.frame.rotateRight:SetShown(shown)
    for _, button in ipairs(self.frame.slots) do
        button:SetShown(shown)
        button.slotPanel:SetShown(shown)
    end
end

function UI:UpdatePermissionUi()
    self:EnsureFrames()
    local canManage = self.currentBot and self.currentBot.canManage == true
    local tabCount = canManage and 3 or 2
    local totalWidth = tabCount * 104 + (tabCount - 1) * 3
    local firstX = (FRAME_WIDTH - totalWidth) / 2
    for index, tab in ipairs(self.frame.tabs) do
        tab:ClearAllPoints()
        tab:SetPoint("BOTTOMLEFT", self.frame, "BOTTOMLEFT", firstX + (index - 1) * 107, 15)
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
        if tab.RefreshVisual then
            tab:RefreshVisual()
        end
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
        self.frame.status:SetText("")
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

function UI:ResetEquipmentSlotDisplays()
    if not self.frame or not self.frame.slots then
        return
    end

    for _, button in ipairs(self.frame.slots) do
        button.icon:Hide()
        button.emptyTexture:Show()
        button.label:SetTextColor(0.82, 0.72, 0.52)
        button.label:Show()
        button.itemLevel:SetText("")
        SetSlotBorderColor(button, nil)
    end
    self.frame.totalGearScore:SetText("GS:0")
end

function UI:UpdateOwnerSubtitle(snapshot)
    if not self.frame then
        return
    end
    local ownerName = snapshot and snapshot.ownerName
    if snapshot and not snapshot.canManage and ownerName and ownerName ~= "" then
        self.frame.subtitle:SetText("主人：" .. ownerName)
    else
        self.frame.subtitle:SetText("")
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
    self:UpdateOwnerSubtitle(snapshot)
    SetModel(self.frame.model, self.currentUnit)

    for slot = 0, 17 do
        local button = self.frame.slots[slot + 1]
        local slotData = snapshot.slots[slot + 1]
        if slotData and slotData.occupied then
            button.icon:SetTexture(GetEntryIcon(slotData.entry))
            button.icon:SetVertexColor(1, 1, 1)
            button.icon:Show()
            button.emptyTexture:Hide()
            button.label:Hide()
            button.itemLevel:SetText(slotData.itemLevel and slotData.itemLevel > 0 and slotData.itemLevel or "")
            -- 装备等级文本按装备品质色显示
            local levelColor = QUALITY_COLORS[slotData.quality] or QUALITY_COLORS[1]
            button.itemLevel:SetTextColor(levelColor[1], levelColor[2], levelColor[3])
            -- 边框按装备品质色显示
            SetSlotBorderColor(button, slotData.quality)
        else
            button.icon:Hide()
            button.emptyTexture:Show()
            button.label:SetTextColor(0.82, 0.72, 0.52)
            button.label:Show()
            button.itemLevel:SetText("")
            -- 空槽恢复灰色边框
            SetSlotBorderColor(button, nil)
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
    local name, className, classToken = GetUnitDisplayInfo(unit)
    self.currentBot = {
        entry = botEntry,
        guidLow = tostring(botGuidLow),
        name = name,
        className = className,
        classToken = classToken,
        canManage = false,
        equipmentRevision = ""
    }
    self.currentUnit = unit
    self.snapshot = nil
    self:ResetEquipmentSlotDisplays()
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
    if entry <= 70000 or entry >= 80000 then
        self:ShowError("该单位不在允许查看的 NPCBot Entry 范围内")
        return
    end
    if UnitIsDeadOrGhost(unit) then
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
    if self.candidatePanel.clickCatcher then
        self.candidatePanel.clickCatcher:Hide()
    end
    for _, button in ipairs(self.candidateButtons) do
        button:Hide()
    end
end

function UI:AnchorCandidatePanel(slotButton)
    local panel = self.candidatePanel
    panel:ClearAllPoints()
    -- 按槽位所在栏决定候选框默认展开方向：左栏向左、右栏向右、底部武器栏向下。
    local side = slotButton.side or "right"
    if side == "left" then
        panel:SetPoint("TOPRIGHT", slotButton, "TOPLEFT", -8, 0)
    elseif side == "bottom" then
        panel:SetPoint("TOPLEFT", slotButton, "BOTTOMLEFT", 0, -8)
    else
        panel:SetPoint("TOPLEFT", slotButton, "TOPRIGHT", 8, 0)
    end
    panel:Show()
    if panel.clickCatcher then
        panel.clickCatcher:Show()
    end

    -- 越界兜底：右侧溢出屏幕则翻到槽位左侧。
    if panel:GetRight() and UIParent:GetRight() and panel:GetRight() > UIParent:GetRight() - 8 then
        panel:ClearAllPoints()
        panel:SetPoint("TOPRIGHT", slotButton, "TOPLEFT", -8, 0)
    end
    -- 越界兜底：底部溢出屏幕则翻到槽位上方。
    if panel:GetBottom() and panel:GetBottom() < 8 then
        panel:ClearAllPoints()
        panel:SetPoint("BOTTOMLEFT", slotButton, "TOPLEFT", 0, 8)
    end
end

function UI:ToggleCandidates(botSlot, slotButton)
    local valid, message = self:ValidateCurrentBotRecord()
    if not valid then
        self:CloseForInvalidBot(message)
        return
    end
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
    panel:SetHeight(CANDIDATE_PADDING + CANDIDATE_BOTTOM_PADDING + CANDIDATE_SIZE)

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
            GameTooltip:AddLine("卸下当前装备", 1, 1, 1)
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
    local contentHeight = CANDIDATE_PADDING + CANDIDATE_BOTTOM_PADDING + rows * CANDIDATE_SIZE +
        math.max(0, rows - 1) * CANDIDATE_ROW_GAP
    local visibleRows = math.min(rows, MAX_VISIBLE_ROWS)
    local visibleHeight = CANDIDATE_PADDING + CANDIDATE_BOTTOM_PADDING + visibleRows * CANDIDATE_SIZE +
        math.max(0, visibleRows - 1) * CANDIDATE_ROW_GAP
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
            -CANDIDATE_PADDING - row * (CANDIDATE_SIZE + CANDIDATE_ROW_GAP))
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

    local valid, message = self:ValidateCurrentBotRecord()
    if not valid then
        self:CloseForInvalidBot(message)
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

    if self:ShouldCloseForResponse(response) then
        self:CloseForInvalidBot(UI:GetResultMessage(response))
        return
    end
    if response.snapshot and response.snapshot.slots then
        self:ApplyFullSnapshot(response.snapshot)
    end
    self:ShowError(UI:GetResultMessage(response))
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
        if UI:ShouldCloseForResponse(response) then
            UI:CloseForInvalidBot(UI:GetResultMessage(response))
        else
            UI:ShowError(UI:GetResultMessage(response))
            UI.frame:Hide()
        end
        return
    end
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
        if UI:ShouldCloseForResponse(response) then
            UI:CloseForInvalidBot(UI:GetResultMessage(response))
        else
            UI:CloseCandidatePanel()
            UI:ShowError(UI:GetResultMessage(response) or "候选请求失败")
        end
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
            UI:CloseCandidatePanel()
            UI:ShowError("请求超时，请重新右键该槽位")
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
    UnitPopupButtons[POPUP_VALUE] = { text = "查看装备", dist = 0 }
    -- 组队/团队状态下右键队伍成员时，菜单走的是 PARTY / RAID_PLAYER 分支，而非 TARGET，
    -- 因此必须将“查看装备”同时注册到多个菜单，否则组队状态下不会出现该菜单项。
    -- 按钮统一插入到各菜单的倒数第二个位置（即“取消”之前）。
    local function RegisterPopupValue(menuName)
        local menu = UnitPopupMenus[menuName]
        if not menu then
            return
        end
        for _, value in ipairs(menu) do
            if value == POPUP_VALUE then
                return -- 已注册过，避免重复
            end
        end
        if #menu > 0 then
            table.insert(menu, #menu, POPUP_VALUE) -- 插入到倒数第二个位置
        else
            table.insert(menu, POPUP_VALUE)
        end
    end

    RegisterPopupValue("TARGET")
    RegisterPopupValue("PARTY")
    RegisterPopupValue("RAID_PLAYER")
    RegisterPopupValue("PLAYER")

    hooksecurefunc("UnitPopup_ShowMenu", function(dropdownMenu, which, unit)
        -- UnitPopup_ShowMenu(dropdownMenu, which, unit, name, userData)
        -- 后置钩子执行时，unit 即本次右键菜单对应的单位，直接用第三个参数更可靠
        unit = unit or (UIDROPDOWNMENU_INIT_MENU and UIDROPDOWNMENU_INIT_MENU.unit)
        local entry = ParseCreatureGuid(unit)
        local canViewEquipment = entry and entry > 70000 and entry < 80000
        local shown = false -- 多个菜单拼接时只保留一个“查看装备”，其余隐藏去重
        for level = 1, UIDROPDOWNMENU_MAXLEVELS do
            for index = 1, UIDROPDOWNMENU_MAXBUTTONS do
                local button = _G["DropDownList" .. level .. "Button" .. index]
                if button and button.value == POPUP_VALUE then
                    if canViewEquipment and not shown then
                        button:Show()
                        shown = true
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

    DEFAULT_CHAT_FRAME:AddMessage("用法：选中 NPCBot 后输入 /nbgear，或 /nbgear <entry> <guidLow>", 1, 1, 0)
end
