local UI = _G.NPCBotEquipmentUI
if not UI then
    return
end

local AIO = AIO or require("AIO")
local NAMESPACE = UI.namespace or "NPCBotEquipment"
local handlers = UI.handlers or AIO.AddHandlers(NAMESPACE, {})

local SPEC_NAMES = {
    [1] = "武器", [2] = "狂怒", [3] = "防护", [4] = "神圣", [5] = "防护", [6] = "惩戒",
    [7] = "野兽控制", [8] = "射击", [9] = "生存", [10] = "刺杀", [11] = "战斗", [12] = "敏锐",
    [13] = "戒律", [14] = "神圣", [15] = "暗影", [16] = "鲜血", [17] = "冰霜", [18] = "邪恶",
    [19] = "元素", [20] = "增强", [21] = "恢复", [22] = "奥术", [23] = "火焰", [24] = "冰霜",
    [25] = "痛苦", [26] = "恶魔学识", [27] = "毁灭", [28] = "平衡", [29] = "野性战斗",
    [30] = "恢复", [31] = "未选择"
}
local CATEGORY_NAMES = {
    MELEE_PHYSICAL = "物理近战", RANGED_PHYSICAL = "物理远程", HEALING = "治疗", RANGED_SPELL = "法系远程"
}
local DEFENSE_STATS = {
    { "生命值", "maxHealth", "integer" }, { "护甲", "armor", "integer" }, { "防御", "defense", "integer" },
    { "躲闪", "dodge", "percent" }, { "招架", "parry", "percent" }, { "格挡", "block", "percent" },
    { "格挡值", "blockValue", "integer" }
}
local CATEGORY_STATS = {
    MELEE_PHYSICAL = {
        { "攻强", "attackPower", "integer" }, { "伤害", "damage", "damage" },
        { "秒伤", "damagePerSecond", "decimal" }, { "攻速", "attackSpeed", "seconds" },
        { "命中", "hit", "percent" }, { "爆击", "crit", "percent" }, { "急速", "haste", "percent" },
        { "精准", "expertise", "integer" }, { "护甲穿透", "armorPenetration", "percent" }
    },
    RANGED_PHYSICAL = {
        { "远程攻强", "attackPower", "integer" }, { "伤害", "damage", "damage" },
        { "秒伤", "damagePerSecond", "decimal" }, { "攻速", "attackSpeed", "seconds" },
        { "命中", "hit", "percent" }, { "爆击", "crit", "percent" }, { "急速", "haste", "percent" },
        { "护甲穿透", "armorPenetration", "percent" }
    },
    HEALING = {
        { "治疗加成", "healingPower", "integer" }, { "法术强度", "spellPower", "integer" },
        { "爆击", "crit", "percent" }, { "急速", "haste", "percent" }, { "法力值", "maxMana", "integer" },
        { "施法时回蓝", "manaRegenCasting", "decimal" }, { "非施法回蓝", "manaRegenNotCasting", "decimal" },
        { "法术穿透", "spellPenetration", "integer" }
    },
    RANGED_SPELL = {
        { "法术强度", "spellPower", "integer" }, { "命中", "hit", "percent" }, { "爆击", "crit", "percent" },
        { "急速", "haste", "percent" }, { "法术穿透", "spellPenetration", "integer" }, { "法力值", "maxMana", "integer" },
        { "施法时回蓝", "manaRegenCasting", "decimal" }, { "非施法回蓝", "manaRegenNotCasting", "decimal" }
    }
}

local function FormatAttributeValue(data, stat)
    if stat[3] == "damage" then
        return string.format("%.0f - %.0f", tonumber(data.minDamage) or 0, tonumber(data.maxDamage) or 0)
    end
    local value = tonumber(data[stat[2]]) or 0
    if stat[3] == "percent" then
        return string.format("%.2f%%", value)
    elseif stat[3] == "decimal" then
        return string.format("%.1f", value)
    elseif stat[3] == "seconds" then
        return string.format("%.2f 秒", value)
    end
    return string.format("%d", math.floor(value + 0.5))
end

local function CreateStatRow(parent, row)
    local line = CreateFrame("Frame", nil, parent)
    line:SetHeight(26)
    line:SetPoint("TOPLEFT", parent, "TOPLEFT", 6, -30 - (row - 1) * 26)
    line:SetPoint("TOPRIGHT", parent, "TOPRIGHT", -6, -30 - (row - 1) * 26)
    local background = line:CreateTexture(nil, "BACKGROUND")
    background:SetTexture("Interface\\Buttons\\WHITE8X8")
    background:SetAllPoints(line)
    background:SetVertexColor(row % 2 == 0 and 0.09 or 0.13, 0.075, 0.035, 0.62)
    local label = line:CreateFontString(nil, "OVERLAY", "GameFontNormalSmall")
    label:SetPoint("LEFT", line, "LEFT", 8, 0)
    label:SetTextColor(0.90, 0.78, 0.54)
    line.label = label
    local value = line:CreateFontString(nil, "OVERLAY", "GameFontHighlightSmall")
    value:SetPoint("RIGHT", line, "RIGHT", -8, 0)
    value:SetTextColor(1, 1, 1)
    line.value = value
    return line
end

local function CreateAttributesPanel(frame)
    local panel = CreateFrame("Frame", nil, frame)
    panel:SetPoint("TOPLEFT", frame, "TOPLEFT", 10, -64)
    panel:SetPoint("BOTTOMRIGHT", frame, "BOTTOMRIGHT", -10, 48)
    panel:SetFrameLevel(frame:GetFrameLevel() + 3)
    panel:Hide()
    local createInset = UI.CreateInset
    local specInset = createInset(panel, "TOPLEFT", panel, "TOPLEFT", 0, 0, 416, 48)
    local specValue = specInset:CreateFontString(nil, "OVERLAY", "GameFontHighlight")
    specValue:SetPoint("CENTER", specInset, "CENTER", 0, 0)
    local specFont, specFontSize, specFontFlags = specValue:GetFont()
    if specFont then
        specValue:SetFont(specFont, (specFontSize or 12) + 2, specFontFlags)
    end
    specValue:SetText("正在读取...")
    specValue:SetTextColor(1, 0.82, 0.35)
    panel.specValue = specValue
    local left = createInset(panel, "TOPLEFT", specInset, "BOTTOMLEFT", 0, -6, 202, 290)
    local right = createInset(panel, "TOPRIGHT", specInset, "BOTTOMRIGHT", 0, -6, 202, 290)
    local leftTitle = left:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    leftTitle:SetPoint("TOP", left, "TOP", 0, -14)
    leftTitle:SetText("防御属性")
    leftTitle:SetTextColor(0.95, 0.75, 0.28)
    local rightTitle = right:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    rightTitle:SetPoint("TOP", right, "TOP", 0, -14)
    rightTitle:SetText("主要属性")
    rightTitle:SetTextColor(0.95, 0.75, 0.28)
    panel.rightTitle = rightTitle
    panel.leftRows, panel.rightRows = {}, {}
    for row = 1, 9 do
        panel.leftRows[row] = CreateStatRow(left, row)
        panel.rightRows[row] = CreateStatRow(right, row)
    end
    return panel
end

function UI:ResetAttributesModule()
    self.attributes = nil
    self.attributesBotKey = nil
    self.attributesLoading = false
    self.attributesDeadline = nil
    self.attributesRequestId = nil
end

function UI:InvalidateAttributesModule()
    self.attributes = nil
    self.attributesBotKey = nil
end

function UI:ShowAttributesTab()
    self:EnsureFrames()
    if not self.currentBot then
        return
    end
    if not self.frame.attributesPanel then
        self.frame.attributesPanel = CreateAttributesPanel(self.frame)
    end
    self.frame.attributesPanel:Show()
    -- 属性数据不缓存；每次切换到属性页都重新向服务端请求最新快照。
    self.attributes = nil
    self.attributesBotKey = nil
    self.attributesLoading = false
    self.attributesDeadline = nil
    self.frame.attributesPanel.specValue:SetText("正在读取...")
    local requestId = UI.NextRequestId()
    self.attributesRequestId = requestId
    self.attributesLoading = true
    self.attributesDeadline = GetTime() + 5
    self.frame.attributesPanel.specValue:SetText("正在读取...")
    AIO.Handle(NAMESPACE, "RequestAttributes", {
        requestId = requestId, botEntry = self.currentBot.entry, botGuidLow = self.currentBot.guidLow
    })
end

function UI:RenderAttributes(attributes)
    if type(attributes) ~= "table" then return end
    self:EnsureFrames()
    local showPanel = self.activeTab == "属性"
    self.frame.attributesPanel:SetShown(showPanel)
    self.attributes = attributes
    self.attributesBotKey = UI.SnapshotCacheKey(attributes.botEntry, attributes.botGuidLow)
    if self.currentBot then
        self.currentBot.canManage = attributes.canManage == true
        self:UpdatePermissionUi()
    end
    local panel = self.frame.attributesPanel
    local specName = SPEC_NAMES[tonumber(attributes.spec)] or "未知"
    local categoryName = CATEGORY_NAMES[attributes.category] or "主要属性"
    panel.specValue:SetText(specName)
    panel.rightTitle:SetText(categoryName)
    local renderRows = function(rows, definitions)
        for index, row in ipairs(rows) do
            local definition = definitions and definitions[index]
            if definition then
                row.label:SetText(definition[1])
                row.value:SetText(FormatAttributeValue(attributes, definition))
                row:Show()
            else
                row:Hide()
            end
        end
    end
    renderRows(panel.leftRows, DEFENSE_STATS)
    renderRows(panel.rightRows, CATEGORY_STATS[attributes.category] or {})
end

function handlers.AttributesResult(player, response)
    if type(response) ~= "table" or response.requestId ~= UI.attributesRequestId then return end
    if not UI.currentBot or response.botGuidLow ~= UI.currentBot.guidLow then return end
    UI.attributesLoading = false
    UI.attributesDeadline = nil
    if not response.ok or type(response.attributes) ~= "table" then
        UI.frame.attributesPanel.specValue:SetText("读取失败")
        return
    end
    UI:RenderAttributes(response.attributes)
end

function UI:UpdateAttributesModule(now)
    if self.attributesDeadline and now >= self.attributesDeadline then
        self.attributesDeadline = nil
        self.attributesLoading = false
        if self.frame and self.frame.attributesPanel and self.frame.attributesPanel:IsShown() then
            self.frame.attributesPanel.specValue:SetText("读取超时")
        end
    end
end
