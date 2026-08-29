--[[
1.插件载入时，默认显示，为了方便记忆，显示/隐藏插件，由 [/netherbot show] & [/netherbot hide] 改为 [/nb s] & [/nb h]（原作者是：玉素奴香）
2.增加一些 NPCBots 命令
3.增加施法功能，NPCBots 中各个职业的有些法术无法直接对机器人施放，做成了按钮，快捷键是小键盘，或者是CTRL+小键盘（注意：快捷键会覆盖系统设置的快捷键），目前仅死亡骑士未实现，后期会添加，术士没有无法对机器人释放的技能，因此施法条是空的
4.对“生成机器人”按钮加入了 Enter 键盘事件监听，键盘 Enter 可以直接触发点击事件
5.区分普通玩家功能菜单和 GM 玩家功能菜单，GM 菜单中只有 GM 权限的账号能使用
6.增加了一个“生成老鸨”的功能按钮，用来生成一个提供雇佣机器人服务的管家（NPCBots 自带功能）
7.删除了“信息”按钮功能（感觉没啥用）
8.toc 中保留了原作者的信息：NetherstormX & 玉素奴香
9.大家在使用中有什么建议或者意见，提出来，本人虚心接受
10.插件是针对 NPCBots-V20230629 版本制作出来的，太老的版本，就不建议用了
]]

-- 获取玩家职业
-- playerClassId 不一定能获取到，用 playerClassFilename 判断最保险
local playerClassName, playerClassFilename, playerClassId = UnitClass("player")

-- 职业 ClassFilename
local CLASS_FILENAME = {
    WARRIOR = "WARRIOR", -- 战士
    PALADIN = "PALADIN", -- 圣骑士
    HUNTER = "HUNTER", -- 猎人
    ROGUE = "ROGUE", -- 盗贼
    PRIEST = "PRIEST", -- 牧师
    DEATH_KNIGHT = "DEATHKNIGHT", -- 死亡骑士
    SHAMAN = "SHAMAN", -- 萨满
    MAGE = "MAGE", -- 法师
    WARLOCK = "WARLOCK", -- 术士
    DRUID = "DRUID" -- 德鲁伊
}

-- 法术表
-- 每个职业每个法术的ID，法术等级从高到低降序排序：{法术等级10的ID, 法术等级9的ID, 法术等级8的ID...}
local SPELL_TABLE = {
    -- 战士
    WARRIOR = {
        J_J = {
            NAME = "警戒",
            ICON = "Interface\\Icons\\Ability_Warrior_Vigilance",
            IDS = { 50720 }
        },
        Y_H = {
            NAME = "援护",
            ICON = "Interface\\Icons\\Ability_Warrior_VictoryRush",
            IDS = { 3411 }
        }
    },
    -- 圣骑士
    PALADIN = {
        B_H_Z_S = {
            NAME = "保护之手",
            ICON = "Interface\\Icons\\Spell_Holy_SealOfProtection",
            IDS = { 10278, 5599, 1022 }
        },
        Z_J_Z_S = {
            NAME = "拯救之手",
            ICON = "Interface\\Icons\\Spell_Holy_SealOfSalvation",
            IDS = { 1038 }
        },
        X_S_Z_S = {
            NAME = "牺牲之手",
            ICON = "Interface\\Icons\\Spell_Holy_SealOfSacrifice",
            IDS = { 6940 }
        },
        Z_Y_F_Y = {
            NAME = "正义防御",
            ICON = "Interface\\Icons\\INV_Shoulder_37",
            IDS = { 31789 }
        },
        S_G_D_B = {
            NAME = "圣光道标",
            ICON = "Interface\\Icons\\Ability_Paladin_BeaconofLight",
            IDS = { 53563 }
        },
        Q_X_L_L_Z_F = {
            NAME = "强效力量祝福",
            ICON = "Interface\\Icons\\Spell_Holy_GreaterBlessingofKings",
            IDS = { 48934, 48933, 27141, 25916, 25782 }
        },
        Q_X_Z_H_Z_F = {
            NAME = "强效智慧祝福",
            ICON = "Interface\\Icons\\Spell_Holy_GreaterBlessingofWisdom",
            IDS = { 48938, 48937, 27143, 25918, 25894 }
        },
        Q_X_W_Z_Z_F = {
            NAME = "强效王者祝福",
            ICON = "Interface\\Icons\\Spell_Magic_GreaterBlessingofKings",
            IDS = { 25898 }
        },
        Q_X_B_H_Z_F = {
            NAME = "强效庇护祝福",
            ICON = "Interface\\Icons\\Spell_Holy_GreaterBlessingofSanctuary",
            IDS = { 25899 }
        },
        J_S = {
            NAME = "救赎",
            ICON = "Interface\\Icons\\Spell_Holy_Resurrection",
            IDS = { 48950, 48949, 20773, 20772, 10324, 10322, 7328 }
        },
        S_S_G_S = {
            NAME = "圣神干涉",
            ICON = "Interface\\Icons\\Spell_Nature_TimeStop",
            IDS = { 19752 }
        }
    },
    -- 猎人
    HUNTER = {
        W_D = {
            NAME = "误导",
            ICON = "Interface\\Icons\\Ability_Hunter_Misdirection",
            IDS = { 34477 }
        }
    },
    -- 盗贼
    ROGUE = {
        J_H_J_Q = {
            NAME = "嫁祸诀窍",
            ICON = "Interface\\Icons\\Ability_Rogue_TricksOftheTrade",
            IDS = { 57934 }
        }
    },
    -- 牧师
    PRIEST = {
        Y_H_D_Y = {
            NAME = "愈合祷言",
            ICON = "Interface\\Icons\\Spell_Holy_PrayerOfMendingtga",
            IDS = { 48113, 48112, 33076 }
        },
        P_F_S = {
            NAME = "漂浮术",
            ICON = "Interface\\Icons\\Spell_Holy_LayOnHands",
            IDS = { 1706 }
        },
        F_H_S = {
            NAME = "复活术",
            ICON = "Interface\\Icons\\Spell_Holy_Resurrection",
            IDS = { 48171, 25435, 20770, 10881, 10880, 2010, 2006 }
        }
    },
    -- 死亡骑士
    DEATH_KNIGHT = {

    },
    -- 萨满
    SHAMAN = {
        X_Z_Z_H = {
            NAME = "先祖之魂",
            ICON = "Interface\\Icons\\Spell_Nature_Regenerate",
            IDS = { 49277, 25590, 20777, 20776, 20610, 20609, 2008 }
        }
    },
    -- 法师
    MAGE = {
        H_L_S = {
            NAME = "缓落术",
            ICON = "Interface\\Icons\\Spell_Magic_FeatherFall",
            IDS = { 130 }
        },
        M_F_Y_Z = {
            NAME = "魔法抑制",
            ICON = "Interface\\Icons\\Spell_Nature_AbolishMagic",
            IDS = { 43015, 33944, 10174, 10173, 8451, 8450, 604 }
        },
        M_F_Z_X = {
            NAME = "魔法增效",
            ICON = "Interface\\Icons\\Spell_Holy_FlashHeal",
            IDS = { 43017, 33946, 27130, 10170, 10169, 8455, 1008 }
        }
    },
    -- 术士
    WARLOCK = {

    },
    -- 德鲁伊
    DRUID = {
        F_S = {
            NAME = "复生",
            ICON = "Interface\\Icons\\Spell_Nature_Reincarnation",
            IDS = { 48477, 26994, 20748, 20747, 20742, 20739, 20484 }
        },
        Q_S_H_S = {
            NAME = "起死回生",
            ICON = "Interface\\Icons\\Ability_Druid_LunarGuidance",
            IDS = { 50763, 50764, 50765, 50766, 50767, 50768, 50769 }
        }
    }
}

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 函数定义开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- 通过 16 位的 GUID 获取生物模板 entry
-- creature_template 表的 entry 字段值
local function GetCreatureTemplateEntry(guid)
    if guid then
        -- local knownTypes = {[0]="player", [3]="NPC", [4]="pet", [5]="vehicle"};
        -- 335-12340 版本暂时只发现 [0] [3] [5] 类型
        -- [0] 是玩家类型（无 entry）
        local creatureTemplateType = tonumber(guid:sub(5, 5), 16)
        if creatureTemplateType == 3 or creatureTemplateType == 5 then
            -- 生物模板的 entry
            local creatureTemplateEntry = tonumber(guid:sub(8, 12), 16)
            if creatureTemplateEntry then
                return creatureTemplateEntry
            end
        end
    end
end
-- 校验参数是否大于 0，只支持数字和字符串类型
-- 字符串类型会转换后再判断，如果无法转换，返回 false
local function ValidateIsGtZero(param)
    if param then
        local paramType = type(param)
        if paramType == "string" then
            if param ~= "" then
                -- 如果转换时 nil 溢出（例如："2323224a"），则返回 -1
                local t = tonumber(param) or -1
                if t and t > 0 then
                    return true
                end
            end
        elseif paramType == "number" then
            if param > 0 then
                return true
            end
        end
    end
end
-- 跟随玩家，并主动攻击进入攻击范围，且有仇恨的敌人
local function CommandNPCBotCommandFollow_Player()
    SendChatMessage(".npcbot command follow", "SAY")
end
-- 跟随玩家，并在活跃与非活跃之间切换
-- 跟随（活跃）：跟随玩家，并主动攻击进入攻击范围，且有仇恨的敌人
-- 跟随（非活跃）：所有机器人在跟随玩家时，不会采取任何行动
local function CommandNPCBotCommandFollowOnly_Player()
    SendChatMessage(".npcbot command follow only", "SAY")
end
-- 停在原地，不会采取任何行动
local function CommandNPCBotCommandStopFully_Player()
    SendChatMessage(".npcbot command stopfully", "SAY")
end
-- 停在原地，会攻击进入攻击范围，且有仇恨的敌人（炮台模式）
local function CommandNPCBotCommandStandstill_Player()
    SendChatMessage(".npcbot command standstill", "SAY")
end
-- 开启/关闭对话，关闭后，鼠标放在机器人身上，不会显示对话图标，打怪时关闭对话，可以防止误点
local function CommandNPCBotCommandNoGossip_Player()
    SendChatMessage(".npcbot command nogossip", "SAY")
end
-- 机器人在走/跑之间切换
local function CommandNPCBotCommandWalk_Player()
    SendChatMessage(".npcbot command walk", "SAY")
end
-- 使机器人暂时下线，他们将从地图上传送出去，直到被允许回来，不能在战斗中使用
local function CommandNPCBotHide_Player()
    SendChatMessage(".npcbot hide", "SAY")
end
-- 将下线的机器人，召唤回来，不能在战斗中使用
local function CommandNPCBotShow_Player()
    SendChatMessage(".npcbot show", "SAY")
end
-- 杀死机器人，可以用来解决，有时在副本中，即使脱离了战斗，机器人仍然处于战斗中，导致无法对话，无法复活玩家的 BUG
local function CommandNPCBotKill_Player()
    SendChatMessage(".npcbot kill", "SAY")
end
-- 查看机器人属性
local function CommandNPCBotAttr_Player()
    DoEmote("BONK")
end
-- 查看机器人装备
local function CommandNPCBotEquip_Player()
  SendChatMessage(".npcbot command equip", "SAY")
end
-- 机器人跟随距离（0 - 100）
local function CommandNPCBotDistance_Player(param)
    SendChatMessage(".npcbot distance " .. param, "SAY")
end
-- 机器人远程攻击距离（支持数字类型以及两个字符串类型的参数）
-- 数字：0 - 50
-- "short"：最小远程攻击距离
-- "long"：最大远程攻击距离
local function CommandNPCBotDistanceAttack_Player(param)
    SendChatMessage(".npcbot distance attack " .. param, "SAY")
end
-- 强制机器人直接移动到玩家的位置，死后可用
local function CommandNPCBotRecall_Player()
    SendChatMessage(".npcbot recall teleport", "SAY")
end
-- 分散机器人站位
local function CommandNPCBotSpread_Player()
    SendChatMessage(".npcbot command spread", "SAY")
end
-- 取消分散站位
local function CommandNPCBotSpreadOff_Player()
    SendChatMessage(".npcbot command spread off", "SAY")
end
-- 必须选中玩家，显示玩家拥有的机器人的各种状态下的数量
local function CommandNPCBotInfo_Player()
    SendChatMessage(".npcbot info", "SAY")
end
-- 对机器人使用法术
-- 根据法术等级从高到低的顺序，依次校验玩家是否已经学会该法术，优先对机器人使用高等级法术
-- 否则就算未学会这个法术，也能对机器人使用（不平衡）
local function CommandNPCBotUseOnBotSpell_Player(spellIds)
    for index, spellId in pairs(spellIds) do
        -- 校验玩家是否已经学会了这个法术
        local isSpellKnown = IsSpellKnown(spellId)
        if isSpellKnown then
            SendChatMessage(".npcbot useonbot spell " .. spellId, "SAY")
            return
        end
    end
    ChatFrame1:AddMessage("|cffFFFF00你还没有学会这个法术！")
end
-- 对机器人使用物品，物品ID
local function CommandNPCBotUseOnBotItem_Player(itemId)
    SendChatMessage(".npcbot useonbot item " .. itemId, "SAY")
end
-- 命令机器人施放指定法术
-- botName: 机器人名称, spellName: 法术名(客户端语言), targetToken: 目标标记(可选, nil=self)
local function CommandNPCBotOrderCast_Player(botName, spellName, targetToken)
    if targetToken then
        SendChatMessage(".npcbot order cast " .. botName .. " " .. spellName .. " " .. targetToken, "SAY")
    else
        SendChatMessage(".npcbot order cast " .. botName .. " " .. spellName, "SAY")
    end
end

-- 嘲讽技能名对照表（客户端中文名，用于 .npcbot order cast）
-- 同时包含 spellId，用于通过 GetSpellInfo 获取技能图标
local TAUNT_SPELL_TABLE = {
    WARRIOR = { name = "嘲讽", spellId = 355 },
    PALADIN = { name = "清算之手", spellId = 62124 },
    DRUID = { name = "低吼", spellId = 6795 },
    DEATHKNIGHT = { name = "黑暗命令", spellId = 56222 }
}

-- 团队减伤/抬血技能表（用于控制面板 .npcbot order cast）
-- spellName: 客户端中文法术名, icon: 技能图标路径, targetToken: 法术施放目标(可选, nil=self)
local TEAM_DEFENSIVE_SPELLS = {
    WARRIOR = {
        { spellName = "警戒", icon = "Interface\\Icons\\Ability_Warrior_Vigilance" },
        { spellName = "破釜沉舟", icon = "Interface\\Icons\\Spell_Holy_AshesToAshes" },
        { spellName = "盾墙", icon = "Interface\\Icons\\Ability_Warrior_ShieldWall" },
        { spellName = "盾牌格挡", icon = "Interface\\Icons\\Ability_Defend" }
    },
    PALADIN = {
        { spellName = "圣盾术", icon = "Interface\\Icons\\Spell_Holy_DivineIntervention" },
        { spellName = "神圣牺牲", icon = "Interface\\Icons\\Spell_Holy_SealOfSacrifice" },
        { spellName = "保护之手", icon = "Interface\\Icons\\Spell_Holy_SealOfProtection"},
        { spellName = "牺牲之手", icon = "Interface\\Icons\\Spell_Holy_SealOfSacrifice"},
        { spellName = "圣疗术", icon = "Interface\\Icons\\Spell_Holy_LayOnHands" }, 
        { spellName = "圣光道标", icon = "Interface\\Icons\\Ability_Paladin_BeaconofLight" }
    },
    PRIEST = {
        { spellName = "神圣赞美诗", icon = "Interface\\Icons\\Spell_Holy_DivineHymn" },
        { spellName = "真言术：盾", icon = "Interface\\Icons\\Spell_Holy_PowerWordShield" }, 
        { spellName = "痛苦压制", icon = "Interface\\Icons\\Spell_Holy_PainSupression" },
        { spellName = "能量灌注", icon = "Interface\\Icons\\Spell_Holy_PowerInfusion" }
    },
    DRUID = {
        { spellName = "宁静", icon = "Interface\\Icons\\Spell_Nature_Tranquility" },
        { spellName = "激活", icon = "Interface\\Icons\\Spell_Nature_Lightning" },
        { spellName = "树皮术", icon = "Interface\\Icons\\Spell_Nature_StoneClawTotem" },
        { spellName = "复生", icon = "Interface\\Icons\\Spell_Nature_Reincarnation" }
    },
    SHAMAN = {
        { spellName = "嗜血", icon = "Interface\\Icons\\Spell_Nature_BloodLust" },
        { spellName = "治疗链", icon = "Interface\\Icons\\Spell_Nature_HealingWaveGreater" },
        { spellName = "法力之潮图腾", icon = "Interface\\Icons\\Spell_Nature_SlowingTotem" }, 
        { spellName = "治疗之泉图腾", icon = "Interface\\Icons\\inv_spear_04" },
        { spellName = "战栗图腾", icon = "Interface\\Icons\\Spell_Nature_TremorTotem" }
    },
    DEATHKNIGHT = {
        { spellName = "反魔法领域", icon = "Interface\\Icons\\Spell_DeathKnight_AntiMagicZone" },
        { spellName = "死亡之握", icon = "Interface\\Icons\\Spell_Deathknight_Strangulate" },
        { spellName = "冰封之韧", icon = "Interface\\Icons\\Spell_DeathKnight_IceBoundFortitude" }
    },
    MAGE = {
        { spellName = "变形术", icon = "Interface\\Icons\\Spell_Nature_Polymorph" },
        { spellName = "法术反制", icon = "Interface\\Icons\\Spell_Frost_IceShock" },
        { spellName = "冰霜新星", icon = "Interface\\Icons\\Spell_Frost_FrostNova" },
        { spellName = "法术吸取", icon = "Interface\\Icons\\Spell_Arcane_Arcane04" }
    },
    HUNTER = {
        { spellName = "误导", icon = "Interface\\Icons\\Ability_Hunter_Misdirection" },
        { spellName = "宁神射击", icon = "Interface\\Icons\\Spell_Nature_Drowsy" },
        { spellName = "冰冻陷阱", icon = "Interface\\Icons\\Spell_Frost_ChainsOfIce" }
    },
    ROGUE = {
        { spellName = "嫁祸诀窍", icon = "Interface\\Icons\\Ability_Rogue_TricksOftheTrade" },
        { spellName = "脚踢", icon = "Interface\\Icons\\Ability_Kick" },
        { spellName = "致盲", icon = "Interface\\Icons\\Spell_Shadow_MindSteal" }
    },
    WARLOCK = {
        { spellName = "灵魂石", icon = "Interface\\Icons\\Spell_Shadow_SoulGem" },
        { spellName = "恐惧", icon = "Interface\\Icons\\Spell_Shadow_Possession" },
        { spellName = "放逐术", icon = "Interface\\Icons\\Spell_Shadow_Cripple" }
    }
}

-- 列出所有机器人的ID、名字、等级、位置、活跃状态（active、free）信息
local function CommandNPCBotListSpawned_GM()
    SendChatMessage(".npcbot list spawned", "SAY")
end
-- 列出所有空闲（free）机器人的ID、名字、等级、位置、活跃状态（active、free）信息
local function CommandNPCBotListSpawnedFree_GM()
    SendChatMessage(".npcbot list spawned free", "SAY")
end
-- 复活机器人
local function CommandNPCBotRevive_GM()
    SendChatMessage(".npcbot revive", "SAY")
end
-- 免费招募一个选中的机器人（绕过购买），仅适用于无主的机器人
local function CommandNPCBotAdd_GM()
    SendChatMessage(".npcbot add", "SAY")
end
-- 解雇，通过此方式解除招募的机器人，会保留装备，并会回到原先招募的位置
local function CommandNPCBotRemove_GM()
    SendChatMessage(".npcbot remove", "SAY")
end
-- 将机器人移动到玩家当前的位置，只能移动无主机器人，支持选中目标和根据机器人模板 entry（creature_template.entry）两种方式
local function CommandNPCBotMove_GM(entry)
    if entry then
        SendChatMessage(".npcbot move " .. entry, "SAY")
    else
        SendChatMessage(".npcbot move", "SAY")
    end
end
-- 删除一个机器人，机器人的装备会回到背包
local function CommandNPCBotDelete_GM()
    SendChatMessage(".npcbot delete", "SAY")
end
-- 根据机器人模板 entry（creature_template.entry），永久删除一个机器人，机器人的装备会回到背包
local function CommandNPCBotDeleteId_GM(entry)
    SendChatMessage(".npcbot delete id " .. entry, "SAY")
end
-- 删除所有无主的机器人
local function CommandNPCBotDeleteFree_GM()
    SendChatMessage(".npcbot delete free", "SAY")
end
-- 根据职业编码，查询种族编码
local function CommandNPCBotLookup_GM(classId)
    SendChatMessage(".npcbot lookup " .. classId, "SAY")
end
-- 根据种族编码，生成机器人
local function CommandNPCBotSpawn_GM(entry)
    SendChatMessage(".npcbot spawn " .. entry, "GUILD")
end
-- 在玩家的位置上生成一个 NPC
local function CommandNPCAdd_GM(entry)
    SendChatMessage(".npc add " .. entry, "SAY")
end
-- 删除选中的 NPC，会校验选中的 NPC 的 entry 和参数 entry 是否一致
local function CommandNPCDelete_GM(entry, message)
    local targetEntry = GetCreatureTemplateEntry(UnitGUID("target"))
    if entry and targetEntry and entry == targetEntry then
        SendChatMessage(".npc delete", "SAY")
    else
        if message then
            ChatFrame1:AddMessage(message)
        else
            ChatFrame1:AddMessage("|cffFFFF00目标错误！")
        end
    end
end
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 函数定义结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 初始化标头菜单 frame 开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
local titleFrame = CreateFrame("Frame", "NetherBotTitleFrame", UIParent)
titleFrame:SetSize(170, 35)
titleFrame:SetPoint("RIGHT", UIParent, "RIGHT", -200, 0)
titleFrame:Show()
titleFrame:SetBackdrop({
    bgFile = "Interface/Buttons/WHITE8X8",
    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
titleFrame:SetBackdropColor(0.35, 0.14, 0.73, 0.25)
titleFrame:SetBackdropBorderColor(0.53, 0.07, 0.89, 1)
titleFrame:SetMovable(true)
titleFrame:EnableMouse(true)
titleFrame:SetScript("OnMouseDown", function(self, button)
    if button == "LeftButton" then
        self:StartMoving()
    end
end)
titleFrame:SetScript("OnMouseUp", function(self, button)
    if button == "LeftButton" then
        self:StopMovingOrSizing()
    end
end)
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 初始化标头菜单 frame 结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 初始化主菜单 frame 开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
local mainFrame = CreateFrame("Frame", "NetherBotMainFrame", UIParent)
mainFrame:SetSize(170, 260)
mainFrame:SetPoint("TOP", titleFrame, "BOTTOM", 0, 0)
mainFrame:Show()
mainFrame:SetBackdrop({
    bgFile = "Interface/Buttons/WHITE8X8",
    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
mainFrame:SetBackdropColor(0.35, 0.14, 0.73, 0.25)
mainFrame:SetBackdropBorderColor(0.53, 0.07, 0.89, 1)
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 初始化主菜单 frame 结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 初始化管理菜单 frame 开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
local gameMasterFrame = CreateFrame("Frame", "NetherBotGameMasterFrame", UIParent)
gameMasterFrame:SetSize(136, 230)
gameMasterFrame:SetPoint("RIGHT", mainFrame, "LEFT", 0, 0)
gameMasterFrame:SetBackdrop({ bgFile = "Interface/Tooltips/UI-Tooltip-Background",
                         edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
                         tile = true, tileSize = 16, edgeSize = 16,
                         insets = { left = 4, right = 4, top = 4, bottom = 4 } })
gameMasterFrame:SetBackdropColor(1, 0, 0, 0.2) -- Set the background color to red and transparency to 20%.
gameMasterFrame:SetBackdropBorderColor(0, 1, 0, 1)
gameMasterFrame:Hide() -- hide the admin frame by default
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 初始化管理菜单 frame 结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 初始化查找菜单 frame 开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
local lookupFrame = CreateFrame("Frame", "NetherBotLookupFrame", UIParent)
lookupFrame:SetSize(200, 310)
lookupFrame:SetPoint("RIGHT", gameMasterFrame, "LEFT", -20, 0)
lookupFrame:SetBackdrop({
    bgFile = "Interface/Tooltips/UI-Tooltip-Background",
    edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
lookupFrame:SetBackdropColor(0, 0, 1, 0.3)
lookupFrame:SetBackdropBorderColor(0, 0, 1, 1)
lookupFrame:Hide()
-- Make the frame movable
lookupFrame:SetMovable(true)
lookupFrame:EnableMouse(true)
lookupFrame:SetScript("OnMouseDown", function(self, button)
    if button == "LeftButton" then
        self:StartMoving()
    end
end)
lookupFrame:SetScript("OnMouseUp", function(self, button)
    if button == "LeftButton" then
        self:StopMovingOrSizing()
    end
end)
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 初始化查找菜单 frame 结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 初始化施法菜单 frame 开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
local castSpellFrame = CreateFrame("Frame", "NetherBotCastSpellFrame", UIParent)
castSpellFrame:SetSize(50, 300)
castSpellFrame:SetPoint("LEFT", mainFrame, "RIGHT", 0, 0)
castSpellFrame:SetBackdrop({ bgFile = "Interface/Tooltips/UI-Tooltip-Background",
                             edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
                             tile = true, tileSize = 16, edgeSize = 16,
                             insets = { left = 4, right = 4, top = 4, bottom = 4 } })
castSpellFrame:SetBackdropColor(1, 0, 0, 0.2) -- Set the background color to red and transparency to 20%.
castSpellFrame:SetBackdropBorderColor(0, 1, 0, 1)
castSpellFrame:Hide()
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 初始化施法菜单 frame 结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 标头菜单开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
local title = titleFrame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
title:SetPoint("TOP", titleFrame, "TOP", 0, -10)
title:SetText("NetherBot")

-- Create the "reload" button
local reloadButton = CreateFrame("Button", "NetherBotReloadButton", titleFrame, "UIPanelButtonTemplate")
reloadButton:SetSize(21, 20)
reloadButton:SetPoint("TOPLEFT", titleFrame, "TOPLEFT", 20, -7)
reloadButton:SetText("|cff00C957RL")
reloadButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
reloadButton:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:SetText("重载插件")
    GameTooltip:Show()
end)
reloadButton:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)
reloadButton:SetScript("OnClick", function()
    StaticPopupDialogs["CONFIRM_RELOAD"] = {
        text = "确定要|cff00C957重载插件|r？",
        button1 = "|cff00C957是",
        button2 = "否",
        timeout = 0,
        whileDead = true,
        hideOnEscape = true,
        OnAccept = function()
            -- 重载插件
            ReloadUI()
        end
    }
    StaticPopup_Show("CONFIRM_RELOAD")
end)

local switchButton = CreateFrame("Button", "NetherBotSwitchButton", titleFrame, "UIPanelButtonTemplate")
switchButton:SetSize(21, 20)
switchButton:SetPoint("TOPRIGHT", titleFrame, "TOPRIGHT", -20, -7)
switchButton:SetText("|cff00C957▽")
switchButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
switchButton:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:SetText("展开/收起")
    GameTooltip:Show()
end)
switchButton:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)
switchButton:SetScript("OnClick", function()
    if mainFrame:IsShown() then
        mainFrame:Hide()
        gameMasterFrame:Hide()
        lookupFrame:Hide()
        castSpellFrame:Hide()
    else
        mainFrame:Show()
    end
end)
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 标头菜单结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 主菜单开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- Follow Button
local followButton = CreateFrame("Button", "NetherBotFollowButton", mainFrame, "ActionButtonTemplate")
followButton:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 20, -10)
followButton:SetSize(40, 40)
followButton:SetText("|cff69CCF0跟随\n模式")
followButton:SetNormalFontObject("GameFontNormal")
local followTexture = followButton:CreateTexture(nil, "BACKGROUND")
followTexture:SetTexture("Interface\\Icons\\Ability_Tracking")
followTexture:SetAllPoints()
followButton:SetNormalTexture(followTexture)
local followPushedTexture = followButton:CreateTexture(nil, "BACKGROUND")
followPushedTexture:SetTexture("Interface\\Icons\\spell_magic_polymorphrabbit")
followPushedTexture:SetAllPoints()
followButton:SetPushedTexture(followPushedTexture)
followButton:SetScript("OnClick", function()
    CommandNPCBotCommandFollow_Player()
end)

-- StandStill Button
local standstillButton = CreateFrame("Button", "NetherBotStandstillButton", mainFrame, "ActionButtonTemplate")
standstillButton:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 65, -10)
standstillButton:SetSize(40, 40)
standstillButton:SetText("|cff69CCF0炮台\n模式")
standstillButton:SetNormalFontObject("GameFontNormal")
local standstillTexture = standstillButton:CreateTexture(nil, "BACKGROUND")
standstillTexture:SetTexture("Interface\\Icons\\Ability_Vehicle_SiegeEngineCannon")
standstillTexture:SetAllPoints()
standstillButton:SetNormalTexture(standstillTexture)
local standstillPushedTexture = standstillButton:CreateTexture(nil, "BACKGROUND")
standstillPushedTexture:SetTexture("Interface\\Icons\\spell_magic_polymorphrabbit")
standstillPushedTexture:SetAllPoints()
standstillButton:SetPushedTexture(standstillPushedTexture)
standstillButton:SetScript("OnClick", function()
    CommandNPCBotCommandStandstill_Player()
end)

-- StopFully Button
local stopFullyButton = CreateFrame("Button", "NetherBotStopFullyButton", mainFrame, "ActionButtonTemplate")
stopFullyButton:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 110, -10)
stopFullyButton:SetSize(40, 40)
stopFullyButton:SetText("|cff69CCF0呆立\n模式")
stopFullyButton:SetNormalFontObject("GameFontNormal")
local stopFullyTexture = stopFullyButton:CreateTexture(nil, "BACKGROUND")
stopFullyTexture:SetTexture("Interface\\Icons\\ABILITY_SEAL")
stopFullyTexture:SetAllPoints()
stopFullyButton:SetNormalTexture(stopFullyTexture)
local stopFullyPushedTexture = stopFullyButton:CreateTexture(nil, "BACKGROUND")
stopFullyPushedTexture:SetTexture("Interface\\Icons\\spell_magic_polymorphrabbit")
stopFullyPushedTexture:SetAllPoints()
stopFullyButton:SetPushedTexture(stopFullyPushedTexture)
stopFullyButton:SetScript("OnClick", function()
    CommandNPCBotCommandStopFully_Player()
end)

-- 集合按钮通用样式：细边框 + 蓝色背景
local function CreateMassButton(name, text, x, y, onClickFunc)
    local btn = CreateFrame("Button", name, mainFrame)
    btn:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", x, y)
    btn:SetSize(40, 40)
    btn:SetText(text)
    btn:SetNormalFontObject("GameFontNormal")
    -- 细边框 + 深红色半透明背景
    btn:SetBackdrop({
        bgFile = "Interface\\Tooltips\\UI-Tooltip-Background",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true, tileSize = 16, edgeSize = 8,
        insets = { left = 2, right = 2, top = 2, bottom = 2 }
    })
    btn:SetBackdropColor(0.1, 0.2, 0.55, 0.9)
    btn:SetBackdropBorderColor(0.15, 0.3, 0.7, 1.0)
    -- 悬停/按下颜色变化
    btn:SetScript("OnEnter", function()
        btn:SetBackdropColor(0.2, 0.35, 0.75, 1.0)
        btn:SetBackdropBorderColor(0.3, 0.5, 0.9, 1.0)
    end)
    btn:SetScript("OnLeave", function()
        btn:SetBackdropColor(0.1, 0.2, 0.55, 0.9)
        btn:SetBackdropBorderColor(0.15, 0.3, 0.7, 1.0)
    end)
    btn:SetScript("OnMouseDown", function()
        btn:SetBackdropColor(0.05, 0.1, 0.35, 1.0)
    end)
    btn:SetScript("OnMouseUp", function()
        btn:SetBackdropColor(0.2, 0.35, 0.75, 1.0)
    end)
    btn:SetScript("OnClick", onClickFunc)
    return btn
end

-- 集合按钮（无目标=集合所有 Bot，有目标=集合选中的 Bot）
CreateMassButton("NetherBotMassButton", "|cff69CCF0集合", 20, -60, function()
    SendChatMessage(".npcbot command mass", "SAY")
end)

-- 集合远程按钮（仅远程和治疗职责 Bot 集合）
CreateMassButton("NetherBotMassRangedButton", "|cff69CCF0集合\n远程", 65, -60, function()
    SendChatMessage(".npcbot command mass ranged", "SAY")
end)

-- 取消集合按钮
CreateMassButton("NetherBotUnmassButton", "|cff69CCF0取消\n集合", 110, -60, function()
    SendChatMessage(".npcbot command unmass", "SAY")
end)


-- 分散按钮（切换模式：分散 / 取消分散）
local spreadButton = CreateFrame("Button", "NetherBotSpreadButton", mainFrame, "ActionButtonTemplate")
spreadButton:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 20, -110)
spreadButton:SetSize(40, 40)
spreadButton:SetText("|cff69CCF0分散")
spreadButton:SetNormalFontObject("GameFontNormal")
-- 记录当前是否处于分散模式
spreadButton.isSpreadMode = false
local spreadTexture = spreadButton:CreateTexture(nil, "BACKGROUND")
spreadTexture:SetTexture("Interface\\Icons\\Spell_Frost_IceStorm")
spreadTexture:SetAllPoints()
spreadButton:SetNormalTexture(spreadTexture)
local spreadPushedTexture = spreadButton:CreateTexture(nil, "BACKGROUND")
spreadPushedTexture:SetTexture("Interface\\Icons\\spell_magic_polymorphrabbit")
spreadPushedTexture:SetAllPoints()
spreadButton:SetPushedTexture(spreadPushedTexture)
spreadButton:SetScript("OnClick", function()
    if spreadButton.isSpreadMode then
        -- 当前是分散模式，点击取消分散
        CommandNPCBotSpreadOff_Player()
        spreadButton.isSpreadMode = false
        spreadButton:SetText("|cff69CCF0分散")
        spreadTexture:SetTexture("Interface\\Icons\\Spell_Frost_IceStorm")
    else
        -- 当前不是分散模式，点击开启分散
        CommandNPCBotSpread_Player()
        spreadButton.isSpreadMode = true
        spreadButton:SetText("|cffff2222取消")
        spreadTexture:SetTexture("Interface\\Icons\\spell_shaman_earthquake")
    end
end)

local reviveButton = CreateFrame("Button", "NetherBotRecallButton", mainFrame, "ActionButtonTemplate")
reviveButton:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 65, -110)
reviveButton:SetSize(40, 40)
reviveButton:SetText("|cff69CCF0传送")
reviveButton:SetNormalFontObject("GameFontNormal")
local reviveTexture = reviveButton:CreateTexture(nil, "BACKGROUND")
reviveTexture:SetTexture("Interface\\Icons\\Ability_Hunter_BeastTraining")
reviveTexture:SetAllPoints()
reviveButton:SetNormalTexture(reviveTexture)
local revivePushedTexture = reviveButton:CreateTexture(nil, "BACKGROUND")
revivePushedTexture:SetTexture("Interface\\Icons\\spell_magic_polymorphrabbit")
revivePushedTexture:SetAllPoints()
reviveButton:SetPushedTexture(revivePushedTexture)
reviveButton:SetScript("OnClick", function()
    CommandNPCBotRecall_Player()
end)

-- 上线/下线 切换按钮（通过文字显示当前状态）
local toggleButton = CreateFrame("Button", "NetherBotToggleButton", mainFrame, "ActionButtonTemplate")
toggleButton:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 110, -110)
toggleButton:SetSize(40, 40)
toggleButton:SetNormalFontObject("GameFontNormal")
local toggleTexture = toggleButton:CreateTexture(nil, "BACKGROUND")
toggleTexture:SetTexture("Interface\\Icons\\Ability_Hunter_BeastCall")
toggleTexture:SetAllPoints()
toggleButton:SetNormalTexture(toggleTexture)
local togglePushedTexture = toggleButton:CreateTexture(nil, "BACKGROUND")
togglePushedTexture:SetTexture("Interface\\Icons\\spell_magic_polymorphrabbit")
togglePushedTexture:SetAllPoints()
toggleButton:SetPushedTexture(togglePushedTexture)

-- 记录当前是否已上线（true=已上线，显示"下线"；false=未上线，显示"上线"）
toggleButton.isOnline = true
toggleButton:SetText("|cff69CCF0下线")
toggleTexture:SetTexture("Interface\\Icons\\Spell_Nature_SpiritWolf")

toggleButton:SetScript("OnClick", function()
    if toggleButton.isOnline then
        -- 当前已上线，点击则下线
        CommandNPCBotHide_Player()
        toggleButton.isOnline = false
        toggleButton:SetText("|cff69CCF0上线")
        toggleTexture:SetTexture("Interface\\Icons\\Ability_Hunter_BeastCall")
    else
        -- 当前未上线，点击则上线
        CommandNPCBotShow_Player()
        toggleButton.isOnline = true
        toggleButton:SetText("|cff69CCF0下线")
        toggleTexture:SetTexture("Interface\\Icons\\Spell_Nature_SpiritWolf")
    end
end)

 
--查看属性（已移除，按钮位由"远攻"占用）
 

--查看装备（已移除，按钮位由"近攻"占用）
  
--杀死
local killButton = CreateFrame("Button", "NetherBotKillButton", mainFrame, "ActionButtonTemplate")
killButton:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 110, -160)
killButton:SetSize(40, 25)
killButton:SetText("|cff00FF96杀死")
killButton:SetNormalFontObject("GameFontNormal")
local killTexture = killButton:CreateTexture(nil, "BACKGROUND")
killTexture:SetTexture("Interface\\Icons\\Ability_Hunter_RapidKilling")
killTexture:SetAllPoints()
killButton:SetNormalTexture(killTexture)
local killPushedTexture = killButton:CreateTexture(nil, "BACKGROUND")
killPushedTexture:SetTexture("Interface\\Icons\\spell_magic_polymorphrabbit")
killPushedTexture:SetAllPoints()
killButton:SetPushedTexture(killPushedTexture)
killButton:SetScript("OnClick", function()
    CommandNPCBotKill_Player()
end)

 
 
 
--远攻（移入原"属性"按钮位置，大小与属性按钮一致）
local longButton = CreateFrame("Button", "NetherBotLongButton", mainFrame, "ActionButtonTemplate")
longButton:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 20, -160)
longButton:SetSize(40, 25)
longButton:SetText("|cff00FF96远攻")
longButton:SetNormalFontObject("GameFontNormal")
local reviveTexture = longButton:CreateTexture(nil, "BACKGROUND")
reviveTexture:SetTexture("Interface\\Icons\\spell_magic_lesserinvisibilty")
reviveTexture:SetAllPoints()
longButton:SetNormalTexture(reviveTexture)
local revivePushedTexture = longButton:CreateTexture(nil, "BACKGROUND")
revivePushedTexture:SetTexture("Interface\\Icons\\spell_magic_lesserinvisibilty")
revivePushedTexture:SetAllPoints()
longButton:SetPushedTexture(revivePushedTexture)
longButton:SetScript("OnClick", function()
  CommandNPCBotDistanceAttack_Player("long")
end)
 

--近攻（移入原"装备"按钮位置，大小与属性按钮一致）
local shortButton = CreateFrame("Button", "NetherBotshortButton", mainFrame, "ActionButtonTemplate")
shortButton:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 65, -160)
shortButton:SetSize(40, 25)
shortButton:SetText("|cff00FF96近攻")
shortButton:SetNormalFontObject("GameFontNormal")
local walkTexture = shortButton:CreateTexture(nil, "BACKGROUND")
walkTexture:SetTexture("Interface\\Icons\\spell_magic_lesserinvisibilty")
walkTexture:SetAllPoints()
shortButton:SetNormalTexture(walkTexture)
local walkPushedTexture = shortButton:CreateTexture(nil, "BACKGROUND")
walkPushedTexture:SetTexture("Interface\\Icons\\spell_magic_lesserinvisibilty")
walkPushedTexture:SetAllPoints()
shortButton:SetPushedTexture(walkPushedTexture)
shortButton:SetScript("OnClick", function()
  CommandNPCBotDistanceAttack_Player("short")
end)
  
--避免AOE
 

--Distance1 Button
local distance1Button = CreateFrame("Button", "NetherBotDistance1Button", mainFrame, "ActionButtonTemplate")
distance1Button:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 20, -190)
distance1Button:SetSize(40, 25)
distance1Button:SetText("|cff00FF9630码|r")
distance1Button:SetNormalFontObject("GameFontNormal")
local distance1Texture = distance1Button:CreateTexture(nil, "BACKGROUND")
distance1Texture:SetTexture("Interface\\Icons\\Inv_misc_punchcards_red")
distance1Texture:SetAllPoints()
distance1Button:SetNormalTexture(distance1Texture)
local distance1PushedTexture = distance1Button:CreateTexture(nil, "BACKGROUND")
distance1PushedTexture:SetTexture("Interface\\Icons\\Inv_misc_punchcards_red")
distance1PushedTexture:SetAllPoints()
distance1Button:SetPushedTexture(distance1PushedTexture)
distance1Button:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:SetText("跟随30码")
    GameTooltip:Show()
end)
distance1Button:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)
distance1Button:SetScript("OnClick", function()
    CommandNPCBotDistance_Player(30)
end)

--Distance2 Button
local distance2Button = CreateFrame("Button", "NetherBotDistance2Button", mainFrame, "ActionButtonTemplate")
distance2Button:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 65, -190)
distance2Button:SetSize(40, 25)
distance2Button:SetText("|cff00FF9650码|r")
distance2Button:SetNormalFontObject("GameFontNormal")
local distance2Texture = distance2Button:CreateTexture(nil, "BACKGROUND")
distance2Texture:SetTexture("Interface\\Icons\\Inv_misc_punchcards_red")
distance2Texture:SetAllPoints()
distance2Button:SetNormalTexture(distance2Texture)
local distance2PushedTexture = distance2Button:CreateTexture(nil, "BACKGROUND")
distance2PushedTexture:SetTexture("Interface\\Icons\\Inv_misc_punchcards_red")
distance2PushedTexture:SetAllPoints()
distance2Button:SetPushedTexture(distance2PushedTexture)
distance2Button:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:SetText("跟随50码")
    GameTooltip:Show()
end)
distance2Button:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)
distance2Button:SetScript("OnClick", function()
    CommandNPCBotDistance_Player(50)
end)

--Distance3 Button
local distance3Button = CreateFrame("Button", "NetherBotDistance3Button", mainFrame, "ActionButtonTemplate")
distance3Button:SetPoint("TOPLEFT", mainFrame, "TOPLEFT", 110, -190)
distance3Button:SetSize(40, 25)
distance3Button:SetText("|cff00FF9685码|r")
distance3Button:SetNormalFontObject("GameFontNormal")
local distance3Texture = distance3Button:CreateTexture(nil, "BACKGROUND")
distance3Texture:SetTexture("Interface\\Icons\\Inv_misc_punchcards_red")
distance3Texture:SetAllPoints()
distance3Button:SetNormalTexture(distance3Texture)
local distance3PushedTexture = distance3Button:CreateTexture(nil, "BACKGROUND")
distance3PushedTexture:SetTexture("Interface\\Icons\\Inv_misc_punchcards_red")
distance3PushedTexture:SetAllPoints()
distance3Button:SetPushedTexture(distance3PushedTexture)
distance3Button:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:SetText("跟随85码")
    GameTooltip:Show()
end)
distance3Button:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)
distance3Button:SetScript("OnClick", function()
    CommandNPCBotDistance_Player(85)
end)

local adminButton = CreateFrame("Button", "NetherBotAdminButton", mainFrame, "UIPanelButtonTemplate")
adminButton:SetSize(40, 22)
adminButton:SetPoint("BOTTOMLEFT", mainFrame, "BOTTOMLEFT", 15, 8)
adminButton:SetText("GM")
adminButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
adminButton:SetScript("OnClick", function()
    if gameMasterFrame:IsShown() then
        gameMasterFrame:Hide()
    else
        gameMasterFrame:Show()
    end
end)

local castSpellButton = CreateFrame("Button", "NetherBotCastSpellButton", mainFrame, "UIPanelButtonTemplate")
castSpellButton:SetSize(40, 22)
castSpellButton:SetPoint("BOTTOMRIGHT", mainFrame, "BOTTOMRIGHT", -15, 8)
castSpellButton:SetText("施法")
castSpellButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
castSpellButton:SetScript("OnClick", function()
    if castSpellFrame:IsShown() then
        castSpellFrame:Hide()
    else
        castSpellFrame:Show()
    end
end)
-- 控制面板按钮（底部居中）- 每次点击创建独立新窗口
local controlPanelButton = CreateFrame("Button", "NetherBotControlPanelButton", mainFrame, "UIPanelButtonTemplate")
controlPanelButton:SetSize(50, 22)
controlPanelButton:SetPoint("BOTTOM", mainFrame, "BOTTOM", 0, 8)
controlPanelButton:SetText("控制")
controlPanelButton:GetNormalTexture():SetVertexColor(1.00, 0.82, 0.10)
controlPanelButton:SetScript("OnClick", function()
    CreateControlPanelWindow()
end)
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 主菜单结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 管理菜单开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
local adminTitle = gameMasterFrame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
adminTitle:SetPoint("TOP", gameMasterFrame, "TOP", 0, -15)
adminTitle:SetText("GM 菜单")

-- Create Admin Buttons
local addButton = CreateFrame("Button", "NetherBotAddButton", gameMasterFrame, "UIPanelButtonTemplate")
addButton:SetSize(56, 22)
addButton:SetPoint("TOPLEFT", gameMasterFrame, "TOPLEFT", 10, -35)
addButton:SetText("雇佣")
addButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
addButton:SetScript("OnClick", function()
    CommandNPCBotAdd_GM()
end)

local removeButton = CreateFrame("Button", "NetherBotRemoveButton", gameMasterFrame, "UIPanelButtonTemplate")
removeButton:SetSize(56, 22)
removeButton:SetPoint("TOPLEFT", gameMasterFrame, "TOPLEFT", 70, -35)
removeButton:SetText("解雇")
removeButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
removeButton:SetScript("OnClick", function()
    CommandNPCBotRemove_GM()
end)

local recallButton = CreateFrame("Button", "NetherBotReviveButton", gameMasterFrame, "UIPanelButtonTemplate")
recallButton:SetSize(56, 22)
recallButton:SetPoint("TOPLEFT", gameMasterFrame, "TOPLEFT", 10, -65)
recallButton:SetText("复活")
recallButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
recallButton:SetScript("OnClick", function()
    CommandNPCBotRevive_GM()
end)

local moveButton = CreateFrame("Button", "NetherBotMoveButton", gameMasterFrame, "UIPanelButtonTemplate")
moveButton:SetSize(56, 22)
moveButton:SetPoint("TOPLEFT", gameMasterFrame, "TOPLEFT", 70, -65)
moveButton:SetText("移动")
moveButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
moveButton:SetScript("OnClick", function()
    local targetGUID = UnitGUID("target")
    if targetGUID then
        CommandNPCBotMove_GM()
    else
        StaticPopupDialogs["MOVE_NPC"] = {
            text = "输入NPC机器人ID，只能移动|cffFF0000无主|r的机器人",
            button1 = "确定",
            button2 = "取消",
            hasEditBox = true,
            timeout = 0,
            whileDead = true,
            hideOnEscape = true,
            OnAccept = function(self)
                local npc = self.editBox:GetText()
                local bool = ValidateIsGtZero(npc)
                if bool then
                    CommandNPCBotMove_GM(npc)
                else
                    ChatFrame1:AddMessage("|cffFFFF00无效输入！")
                end
            end
        }
        StaticPopup_Show("MOVE_NPC")
    end
end)

local addHireBotButton = CreateFrame("Button", "NetherBotAddHireBotButton", gameMasterFrame, "UIPanelButtonTemplate")
addHireBotButton:SetSize(56, 22)
addHireBotButton:SetPoint("TOPLEFT", gameMasterFrame, "TOPLEFT", 10, -95)
addHireBotButton:SetText("召唤老鸨")
addHireBotButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
addHireBotButton:SetScript("OnClick", function()
    -- 在玩家位置生成一个提供机器人招募服务的 NPC
    CommandNPCAdd_GM(70000)
end)

local deleteHireBotButton = CreateFrame("Button", "NetherBotDeleteHireBotButton", gameMasterFrame, "UIPanelButtonTemplate")
deleteHireBotButton:SetSize(56, 22)
deleteHireBotButton:SetPoint("TOPLEFT", gameMasterFrame, "TOPLEFT", 70, -95)
deleteHireBotButton:SetText("删除老鸨")
deleteHireBotButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
deleteHireBotButton:SetScript("OnClick", function()
    CommandNPCDelete_GM(70000, "|cffFFFF00目标错误！请选中名为『Lagretta』的老鸨...")
end)

local listAllButton = CreateFrame("Button", "NetherBotListAllButton", gameMasterFrame, "UIPanelButtonTemplate")
listAllButton:SetSize(56, 22)
listAllButton:SetPoint("TOPLEFT", gameMasterFrame, "TOPLEFT", 10, -125)
listAllButton:SetText("所有Bots")
listAllButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
listAllButton:SetScript("OnClick", function()
    CommandNPCBotListSpawned_GM()
end)

local listFreeButton = CreateFrame("Button", "NetherBotListFreeButton", gameMasterFrame, "UIPanelButtonTemplate")
listFreeButton:SetSize(56, 22)
listFreeButton:SetPoint("TOPLEFT", gameMasterFrame, "TOPLEFT", 70, -125)
listFreeButton:SetText("空闲Bots")
listFreeButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
listFreeButton:SetScript("OnClick", function()
    CommandNPCBotListSpawnedFree_GM()
end)

-- Create the "lookupButton" button
local lookupButton = CreateFrame("Button", "NetherBotLookupButton", gameMasterFrame, "UIPanelButtonTemplate")
lookupButton:SetSize(56, 22)
lookupButton:SetPoint("BOTTOMLEFT", gameMasterFrame, "BOTTOMLEFT", 10, 10)
lookupButton:SetText("|cff6A5ACD查找")
lookupButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
-- Handle the Lookup buttons click event
lookupButton:SetScript("OnClick", function()
    if lookupFrame:IsShown() then
        lookupFrame:Hide()
    else
        lookupFrame:Show()
    end
end)

local deleteButton = CreateFrame("Button", "NetherBotDeleteButton", gameMasterFrame, "UIPanelButtonTemplate")
deleteButton:SetSize(56, 22)
deleteButton:SetPoint("BOTTOMRIGHT", gameMasterFrame, "BOTTOMRIGHT", -10, 10)
deleteButton:SetText("|cffFF0000删除")
deleteButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
deleteButton:SetScript("OnClick", function()
    local targetGUID = UnitGUID("target")
    if targetGUID then
        StaticPopupDialogs["CONFIRM_DELETE"] = {
            text = "确定要|cffFF0000删除|r？",
            button1 = "|cffFF0000是",
            button2 = "否",
            timeout = 0,
            whileDead = true,
            hideOnEscape = true,
            OnAccept = function()
                CommandNPCBotDelete_GM()
            end
        }
        StaticPopup_Show("CONFIRM_DELETE")
    else
        StaticPopupDialogs["DELETE_NPC"] = {
            text = "输入NPC机器人ID:",
            button1 = "确定",
            button2 = "取消",
            hasEditBox = true,
            timeout = 0,
            whileDead = true,
            hideOnEscape = true,
            OnAccept = function(self)
                local npc = self.editBox:GetText()
                local bool = ValidateIsGtZero(npc)
                if bool then
                    StaticPopupDialogs["CONFIRM_DELETE"] = {
                        text = "确定要|cffFF0000删除|r？",
                        button1 = "|cffFF0000是",
                        button2 = "否",
                        timeout = 0,
                        whileDead = true,
                        hideOnEscape = true,
                        OnAccept = function()
                            CommandNPCBotDeleteId_GM(npc)
                        end
                    }
                    StaticPopup_Show("CONFIRM_DELETE")
                else
                    ChatFrame1:AddMessage("|cffFFFF00无效输入！")
                end
            end
        }
        StaticPopup_Show("DELETE_NPC")
    end
end)
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 管理菜单结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 查找菜单开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
local lookupTitle = lookupFrame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
lookupTitle:SetPoint("TOPLEFT", lookupFrame, "TOPLEFT", 10, -14)
lookupTitle:SetText("选择职业:")

-- Create the scrollFrame for the list
local lookupScrollFrame = CreateFrame("ScrollFrame", "NetherBotLookupScrollFrame", lookupFrame, "UIPanelScrollFrameTemplate")
lookupScrollFrame:SetPoint("TOPLEFT", lookupFrame, "TOPLEFT", 4, -25)
lookupScrollFrame:SetPoint("BOTTOMRIGHT", lookupFrame, "BOTTOMRIGHT", -4, 4)

-- Create the list frame
local lookupList = CreateFrame("Frame", "NetherBotLookupList", lookupScrollFrame)
lookupList:SetSize(lookupScrollFrame:GetWidth(), lookupScrollFrame:GetHeight())
lookupScrollFrame:SetScrollChild(lookupList)

-- Create the key-value store
local classTable = {
    ["|cffC69B6D战士"] = 1,
    ["|cffF58CBA圣骑士"] = 2,
    ["|cffAAD372猎人"] = 3,
    ["|cffFFF468盗贼"] = 4,
    ["|cffF0EBE0牧师"] = 5,
    ["|cffC41E3B死亡骑士"] = 6,
    ["|cff2359FF萨满"] = 7,
    ["|cff68CCEF法师"] = 8,
    ["|cff9382C9术士"] = 9,
    ["|cff00FF96------------------------------"] = 10,
    ["|cffFF7C0A德鲁伊"] = 11,
    ["|cffC69B6D------------------------------"] = 12,
    ["|cffF0EBE0War3黑曜石毁灭者"] = 13,
    ["|cff68CCEFWar3大魔导师"] = 14,
    ["|cffA330C9War3恐惧魔王"] = 15,
    ["|cff68CCEFWar3破法者"] = 16,
    ["|cffAAD372War3黑暗游侠"] = 17,
    ["|cff9382C9War3死灵法师"] = 18,
    ["|cff68CCEFWar3娜迦女海巫"] = 19,
    ["|cff009ABFWar3地穴领主"] = 20
}

-- Create the buttons for the list items
for key, value in pairs(classTable) do
    local button = CreateFrame("Button", "NetherBotLookupClassButton" .. value, lookupList, "UIPanelButtonTemplate")
    button:SetSize(180, 25)
    button:SetPoint("TOPLEFT", lookupList, "TOPLEFT", 10, -10 - (value - 1) * 30)
    button:SetText(key)
    button:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)

    -- Handle the button's click event
    button:SetScript("OnClick", function()
        CommandNPCBotLookup_GM(value)
        -- You can add your custom functionality here like running a command or doing some other action
    end)
end

-- Create the "hideLookup" button
local hideLookupButton = CreateFrame("Button", "NetherBotHideLookupButton", lookupFrame, "UIPanelButtonTemplate")
hideLookupButton:SetSize(21, 20)
hideLookupButton:SetPoint("TOPRIGHT", lookupFrame, "TOPRIGHT", -10, -8)
hideLookupButton:SetText("|cffFF0000X")
hideLookupButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
-- Handle Lookup buttons click event
hideLookupButton:SetScript("OnClick", function()
    lookupFrame:Hide()
end)

-- Create the spawnFrame
local spawnFrame = CreateFrame("Frame", "NetherBotSpawnFrame", lookupFrame)
spawnFrame:SetSize(200, 45)
spawnFrame:SetPoint("BOTTOM", lookupFrame, "BOTTOM", 0, -50)
spawnFrame:SetBackdrop({
    bgFile = "Interface/BUTTONS/WHITE8X8",
    edgeFile = "Interface/BUTTONS/WHITE8X8",
    edgeSize = 1,
    insets = { left = 0, right = 0, top = 0, bottom = 0 } })
spawnFrame:SetBackdropColor(0, 0, 1, 0.3)
spawnFrame:SetBackdropBorderColor(0, 0, 1, 1)

-- Create the "buttonSpawnBot" button
local spawnBotButton = CreateFrame("Button", "NetherBotSpawnBotButton", spawnFrame, "UIPanelButtonTemplate")
spawnBotButton:SetSize(80, 25)
spawnBotButton:SetPoint("TOPLEFT", spawnFrame, "TOPLEFT", 15, -10)
spawnBotButton:SetText("生成机器人")
spawnBotButton:GetNormalTexture():SetVertexColor(0.10, 1.00, 0.10)
-- Create the "classInput" input box
local classInput = CreateFrame("EditBox", "NetherBotClassInput", spawnFrame, "InputBoxTemplate")
classInput:SetSize(80, 25)
classInput:SetPoint("TOPLEFT", spawnFrame, "TOPLEFT", 105, -10)
classInput:SetAutoFocus(false)
-- Handle the buttons click event
spawnBotButton:SetScript("OnClick", function()
    local input = classInput:GetText()
    if input ~= "" then
        CommandNPCBotSpawn_GM(input)
        classInput:SetText("")
        classInput:ClearFocus()
    else
        ChatFrame1:AddMessage("|cffFFFF00请输入聊天框中查询出的机器人『ID』，例如：『70XXX』")
    end
end)
-- 回车触发点击事件
classInput:SetScript("OnEnterPressed", function()
    spawnBotButton:Click()
end)
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 查找菜单结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 施法菜单开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- 创建施法按钮自增序列函数，返回一个闭包
local function CreateSpellButtonCounter()
    -- 初始值为 0
    local count = 0
    -- 返回一个函数，每次调用该函数，count 加 1 并返回新值
    return function()
        count = count + 1
        return count
    end
end
-- 创建施法按钮自增计数器
local spellButtonCounter = CreateSpellButtonCounter()
-- 创建施法按钮
local function CreateSpellButton(offsetX, offsetY, table, pressKey)
    local count = spellButtonCounter()
    local spellButton = CreateFrame("Button", "NetherBotSpell" .. count .. "Button", castSpellFrame, "UIPanelButtonTemplate")
    spellButton:SetPoint("TOPLEFT", castSpellFrame, "TOPLEFT", offsetX, offsetY)
    spellButton:SetSize(30, 30)
    spellButton:SetNormalFontObject("GameFontNormal")
    local spellTexture = spellButton:CreateTexture(nil, "BACKGROUND")
    spellTexture:SetTexture(table.ICON)
    spellTexture:SetAllPoints()
    spellButton:SetNormalTexture(spellTexture)
    local spellPushedTexture = spellButton:CreateTexture(nil, "BACKGROUND")
    spellPushedTexture:SetTexture("Interface\\Icons\\spell_magic_polymorphrabbit")
    spellPushedTexture:SetAllPoints()
    spellButton:SetPushedTexture(spellPushedTexture)
    spellButton:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetText(table.NAME .. " [" .. pressKey .. "]")
        GameTooltip:Show()
    end)
    spellButton:SetScript("OnLeave", function()
        GameTooltip:Hide()
    end)
    spellButton:SetScript("OnClick", function()
        CommandNPCBotUseOnBotSpell_Player(table.IDS)
    end)
    -- 绑定按键，触发按钮点击事件，此设置会覆盖系统按键设置
    SetOverrideBindingClick(spellButton, true, pressKey, spellButton:GetName())
    return spellButton
end

-- >>>>>>>>>>>>>>>>>> 战士 <<<<<<<<<<<<<<<<<<<
if playerClassFilename == CLASS_FILENAME.WARRIOR then
    -- 重置施法菜单尺寸
    castSpellFrame:SetSize(50, 90)
    CreateSpellButton(10, -10, SPELL_TABLE.WARRIOR.J_J, "NUMPAD1")
    CreateSpellButton(10, -50, SPELL_TABLE.WARRIOR.Y_H, "NUMPAD2")
end
-- >>>>>>>>>>>>>>>>>> 圣骑士 <<<<<<<<<<<<<<<<<<<
if playerClassFilename == CLASS_FILENAME.PALADIN then
    -- 重置施法菜单尺寸
    castSpellFrame:SetSize(90, 210)
    CreateSpellButton(10, -10, SPELL_TABLE.PALADIN.B_H_Z_S, "NUMPAD1")
    CreateSpellButton(10, -50, SPELL_TABLE.PALADIN.Z_J_Z_S, "NUMPAD2")
    CreateSpellButton(10, -90, SPELL_TABLE.PALADIN.X_S_Z_S, "NUMPAD3")
    CreateSpellButton(10, -130, SPELL_TABLE.PALADIN.Z_Y_F_Y, "NUMPAD4")
    CreateSpellButton(10, -170, SPELL_TABLE.PALADIN.S_G_D_B, "NUMPAD5")
    CreateSpellButton(50, -10, SPELL_TABLE.PALADIN.Q_X_L_L_Z_F, "CTRL-NUMPAD1")
    CreateSpellButton(50, -50, SPELL_TABLE.PALADIN.Q_X_Z_H_Z_F, "CTRL-NUMPAD2")
    CreateSpellButton(50, -90, SPELL_TABLE.PALADIN.Q_X_W_Z_Z_F, "CTRL-NUMPAD3")
    CreateSpellButton(50, -130, SPELL_TABLE.PALADIN.Q_X_B_H_Z_F, "CTRL-NUMPAD4")
    CreateSpellButton(50, -170, SPELL_TABLE.PALADIN.J_S, "CTRL-NUMPAD5")
    -- 对机器人使用神圣干涉没啥用，机器人不会自己取消
    --CreateSpellButton(50, -210, SPELL_TABLE.PALADIN.S_S_G_S, "CTRL-NUMPAD6")
end
-- >>>>>>>>>>>>>>>>>> 猎人 <<<<<<<<<<<<<<<<<<<
if playerClassFilename == CLASS_FILENAME.HUNTER then
    -- 重置施法菜单尺寸
    castSpellFrame:SetSize(40, 40)
    CreateSpellButton(10, -10, SPELL_TABLE.HUNTER.W_D, "NUMPAD1")
end
-- >>>>>>>>>>>>>>>>>> 盗贼 <<<<<<<<<<<<<<<<<<<
if playerClassFilename == CLASS_FILENAME.ROGUE then
    -- 重置施法菜单尺寸
    castSpellFrame:SetSize(40, 40)
    CreateSpellButton(10, -10, SPELL_TABLE.ROGUE.J_H_J_Q, "NUMPAD1")
end
-- >>>>>>>>>>>>>>>>>> 牧师 <<<<<<<<<<<<<<<<<<<
if playerClassFilename == CLASS_FILENAME.PRIEST then
    -- 重置施法菜单尺寸
    castSpellFrame:SetSize(50, 130)
    CreateSpellButton(10, -10, SPELL_TABLE.PRIEST.Y_H_D_Y, "NUMPAD1")
    CreateSpellButton(10, -50, SPELL_TABLE.PRIEST.P_F_S, "NUMPAD2")
    CreateSpellButton(10, -90, SPELL_TABLE.PRIEST.F_H_S, "NUMPAD3")
end
-- >>>>>>>>>>>>>>>>>> 死亡骑士 <<<<<<<<<<<<<<<<<<<
if playerClassFilename == CLASS_FILENAME.DEATH_KNIGHT then

end
-- >>>>>>>>>>>>>>>>>> 萨满 <<<<<<<<<<<<<<<<<<<
if playerClassFilename == CLASS_FILENAME.SHAMAN then
    -- 重置施法菜单尺寸
    castSpellFrame:SetSize(40, 40)
    CreateSpellButton(10, -10, SPELL_TABLE.SHAMAN.X_Z_Z_H, "NUMPAD1")
end
-- >>>>>>>>>>>>>>>>>> 法师 <<<<<<<<<<<<<<<<<<<
if playerClassFilename == CLASS_FILENAME.MAGE then
    -- 重置施法菜单尺寸
    castSpellFrame:SetSize(50, 130)
    CreateSpellButton(10, -10, SPELL_TABLE.MAGE.H_L_S, "NUMPAD1")
    CreateSpellButton(10, -50, SPELL_TABLE.MAGE.M_F_Y_Z, "NUMPAD2")
    CreateSpellButton(10, -90, SPELL_TABLE.MAGE.M_F_Z_X, "NUMPAD3")
end
-- >>>>>>>>>>>>>>>>>> 术士 <<<<<<<<<<<<<<<<<<<
if playerClassFilename == CLASS_FILENAME.WARLOCK then

end
-- >>>>>>>>>>>>>>>>>> 德鲁伊 <<<<<<<<<<<<<<<<<<<
if playerClassFilename == CLASS_FILENAME.DRUID then
    -- 重置施法菜单尺寸
    castSpellFrame:SetSize(50, 90)
    CreateSpellButton(10, -10, SPELL_TABLE.DRUID.F_S, "NUMPAD1")
    CreateSpellButton(10, -50, SPELL_TABLE.DRUID.Q_S_H_S, "NUMPAD2")
end
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 施法菜单结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 控制面板窗口开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- 每次点击"控制面板"按钮都会创建一个全新的独立窗口
-- 两种模式由 CreateControlPanelWindow 根据当前目标分派:
--   锁定模式: 有选中 NPCBot → CreateLockedControlPanel
--   动态模式: 无有效 NPCBot → CreateDynamicControlPanel

-- 偏移量计数器，避免多个窗口完全重叠
local controlPanelOffset = 0

-- NPCBot 模板 entry 区间：只有该区间内的目标才被认为是可控制的机器人
local NPCBOT_ENTRY_MIN = 70000
local NPCBOT_ENTRY_MAX = 80000

-- 判断目标的 entry 是否在 [70000, 80000] 区间内
local function IsNPCBotEntry(target)
    if not target or target == "" then return false end
    local guid = UnitGUID(target)
    if not guid then return false end
    -- 玩家 GUID 类型位为 0（无 entry），直接排除
    local unitType = tonumber(guid:sub(5, 5), 16)
    if unitType == 0 then return false end
    local entry = GetCreatureTemplateEntry(guid)
    if not entry then return false end
    return entry >= NPCBOT_ENTRY_MIN and entry <= NPCBOT_ENTRY_MAX
end

local function IsValidNPCBot(target)
    -- entry 必须在 [70000, 80000] 区间内
    if not IsNPCBotEntry(target) then return false end
    -- 必须有职业才能显示技能按钮
    local _, class = UnitClass(target)
    if not class then return false end
    return TEAM_DEFENSIVE_SPELLS[class] ~= nil
end

-- 创建通用窗口 Frame 基座（拖拽、关闭、背景）
local function CreateBaseControlFrame()
    local frame = CreateFrame("Frame", nil, UIParent)
    frame:SetSize(280, 80)
    controlPanelOffset = (controlPanelOffset + 30) % 300
    frame:SetPoint("CENTER", UIParent, "CENTER", controlPanelOffset, -controlPanelOffset)
    frame:SetBackdrop({
        bgFile = "Interface/Buttons/WHITE8X8",
        edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
        tile = true, tileSize = 16, edgeSize = 16,
        insets = { left = 4, right = 4, top = 4, bottom = 4 }
    })
    frame:SetBackdropColor(0.35, 0.14, 0.73, 0.30)
    frame:SetBackdropBorderColor(0.53, 0.07, 0.89, 1)
    frame:SetMovable(true)
    frame:EnableMouse(true)
    frame:RegisterForDrag("LeftButton")
    frame:SetScript("OnDragStart", function(self) self:StartMoving() end)
    frame:SetScript("OnDragStop", function(self) self:StopMovingOrSizing() end)

    local closeButton = CreateFrame("Button", nil, frame, "UIPanelCloseButton")
    closeButton:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -4, -4)
    closeButton:SetScript("OnClick", function() frame:Hide() end)

    return frame
end

-- 在窗口中按锚点参照填充团队技能按钮（图标显示 + 悬浮 tooltip）
local function FillTeamSpellButtons(frame, class, anchorRef, spellButtons, isLocked, lockedName)
    -- 清除旧按钮
    for _, btn in ipairs(spellButtons) do
        btn:Hide()
        btn:SetScript("OnClick", nil)
        btn:SetScript("OnEnter", nil)
        btn:SetScript("OnLeave", nil)
    end
    while #spellButtons > 0 do table.remove(spellButtons) end

    local spells = TEAM_DEFENSIVE_SPELLS[class]
    if not spells then
        frame:SetHeight(60)
        return
    end

    local BTN_SIZE = 32
    local GAP_X = 8
    local GAP_Y = 6
    local COLS = 3  -- 每行3个按钮
    local rows = math.ceil(#spells / COLS)
    -- 团队技能区域：标签占 18px + 6px 间距，每行按钮 32+6
    local HEADER_OFFSET = -24  -- 标签下方的起始位置
    -- 按钮总宽度，用于居中
    local totalWidth = COLS * BTN_SIZE + (COLS - 1) * GAP_X
    local startX = math.floor((130 - totalWidth) / 2)  -- 130 是 teamAnchor 宽度，居中
    if startX < 4 then startX = 4 end
    if isLocked then
        frame:SetHeight(130 + rows * (BTN_SIZE + GAP_Y))
    else
        -- 动态面板比锁定面板多一行"跟我"按钮，基础高度相应增加
        frame:SetHeight(130 + rows * (BTN_SIZE + GAP_Y))
    end

    for i, spellData in ipairs(spells) do
        local col = (i - 1) % COLS  -- 0,1,2
        local row = math.floor((i - 1) / COLS)

        local btn = CreateFrame("Button", nil, frame)
        btn:SetSize(BTN_SIZE, BTN_SIZE)
        local xOffset = startX + col * (BTN_SIZE + GAP_X)
        local yOffset = HEADER_OFFSET - row * (BTN_SIZE + GAP_Y)
        btn:SetPoint("TOPLEFT", anchorRef, "TOPLEFT", xOffset, yOffset)

        -- 技能图标（非模板按钮需要用 CreateTexture + SetTexture 方式设置贴图）
        local iconTexture = btn:CreateTexture(nil, "BACKGROUND")
        iconTexture:SetTexture(spellData.icon)
        iconTexture:SetAllPoints()
        btn:SetNormalTexture(iconTexture)
        btn:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Square", "ADD")

        -- 显式注册左右键点击，避免父 Frame 的 RegisterForDrag("LeftButton") 拦截左键
        btn:RegisterForClicks("LeftButtonUp", "RightButtonUp")

        -- 悬浮 tooltip 显示技能名
        btn:SetScript("OnEnter", function(self)
            GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
            GameTooltip:SetText(spellData.spellName, 1, 1, 1, 1)
            GameTooltip:Show()
        end)
        btn:SetScript("OnLeave", function(self)
            GameTooltip:Hide()
        end)

        btn:SetScript("OnClick", function(self, button)
            -- 优先使用按钮锁定的目标名，其次用面板记录的目标名，最后用当前目标
            local name = btn.lockedBotName or lockedName or UnitName("target")
            if not name then
                ChatFrame1:AddMessage("|cffFFFF00[NetherBot] 请先选择一个机器人目标！")
                return
            end
            -- 左键以自身为目标(me)，右键以当前目标为目标(mytarget)
            local targetToken = (button == "RightButton") and "mytarget" or "me"
            CommandNPCBotOrderCast_Player(name, spellData.spellName, targetToken)
        end)
        btn:Show()
        table.insert(spellButtons, btn)
    end
end

-- 创建"攻击"按钮（图标 Ability_MeleeDamage，点击在 攻击/取消 间切换）
-- 锁定模式与动态模式复用同一创建方法：
--   parent: 父框架
--   point/relativeTo/relativePoint/x/y: 锚定参数（与其余控制按钮同一行排列）
--   getTargetName: 目标名解析函数（锁定模式返回 botName，动态模式返回 recordedBotName）
local function CreateAttackButton(parent, point, relativeTo, relativePoint, x, y, getTargetName)
    local btn = CreateFrame("Button", nil, parent)
    btn:SetSize(40, 40)
    btn:SetPoint(point, relativeTo, relativePoint, x, y)
    -- 记录当前是否处于攻击模式
    btn.isAttackMode = false
    -- 图标：Ability_MeleeDamage（仿照锁定模式嘲讽按钮的图标方式）
    local attackTexture = btn:CreateTexture(nil, "ARTWORK")
    attackTexture:SetTexture("Interface\\Icons\\Ability_MeleeDamage")
    attackTexture:SetAllPoints()
    btn:SetNormalTexture(attackTexture)
    btn:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Square", "ADD")
    -- 细边框 + 黄色背景（普通模式）
    btn:SetBackdrop({
        bgFile = "Interface\\Tooltips\\UI-Tooltip-Background",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true, tileSize = 16, edgeSize = 8,
        insets = { left = 2, right = 2, top = 2, bottom = 2 }
    })
    btn:SetBackdropColor(0.45, 0.35, 0.05, 0.9)
    btn:SetBackdropBorderColor(0.6, 0.45, 0.1, 1.0)
    -- 悬停：显示提示并变色（根据当前模式动态切换）
    btn:SetScript("OnEnter", function()
        GameTooltip:SetOwner(btn, "ANCHOR_RIGHT")
        if btn.isAttackMode then
            GameTooltip:AddLine("取消持续攻击", 1, 1, 1)
        else
            GameTooltip:AddLine("持续攻击玩家当前目标", 1, 1, 1)
        end
        GameTooltip:Show()
        if btn.isAttackMode then
            btn:SetBackdropColor(0.6, 0.15, 0.15, 1.0)
            btn:SetBackdropBorderColor(0.75, 0.2, 0.2, 1.0)
        else
            btn:SetBackdropColor(0.6, 0.45, 0.1, 1.0)
            btn:SetBackdropBorderColor(0.75, 0.55, 0.15, 1.0)
        end
    end)
    btn:SetScript("OnLeave", function()
        GameTooltip:Hide()
        if btn.isAttackMode then
            btn:SetBackdropColor(0.45, 0.1, 0.1, 0.9)
            btn:SetBackdropBorderColor(0.6, 0.15, 0.15, 1.0)
        else
            btn:SetBackdropColor(0.45, 0.35, 0.05, 0.9)
            btn:SetBackdropBorderColor(0.6, 0.45, 0.1, 1.0)
        end
    end)
    btn:SetScript("OnMouseDown", function()
        if btn.isAttackMode then
            btn:SetBackdropColor(0.3, 0.05, 0.05, 1.0)
        else
            btn:SetBackdropColor(0.3, 0.22, 0.02, 1.0)
        end
    end)
    btn:SetScript("OnMouseUp", function()
        if btn.isAttackMode then
            btn:SetBackdropColor(0.6, 0.15, 0.15, 1.0)
        else
            btn:SetBackdropColor(0.6, 0.45, 0.1, 1.0)
        end
    end)
    btn:SetScript("OnClick", function()
        -- 没有有效目标时不响应
        local targetName = getTargetName()
        if not targetName then
            ChatFrame1:AddMessage("|cffFFFF00[NetherBot] 请先选择一个有效的 NPCBot 目标！")
            return
        end
        if btn.isAttackMode then
            -- 当前是攻击模式，点击取消攻击
            SendChatMessage(".npcbot command unattack " .. targetName, "SAY")
            btn.isAttackMode = false
            btn:GetNormalTexture():SetVertexColor(1, 1, 1)
            btn:SetBackdropColor(0.45, 0.35, 0.05, 0.9)
            btn:SetBackdropBorderColor(0.6, 0.45, 0.1, 1.0)
        else
            -- 当前不是攻击模式，点击开始攻击
            SendChatMessage(".npcbot command attack " .. targetName, "SAY")
            btn.isAttackMode = true
            btn:GetNormalTexture():SetVertexColor(1, 0.35, 0.35)
            btn:SetBackdropColor(0.45, 0.1, 0.1, 0.9)
            btn:SetBackdropBorderColor(0.6, 0.15, 0.15, 1.0)
        end
    end)
    return btn
end

-- ====== 锁定模式：有选中 NPCBot → 标题/控制按钮/技能一次性锁定 ======
function CreateLockedControlPanel(botName, botClass)
    local frame = CreateBaseControlFrame()
    frame:SetSize(180, 200)

    -- 顶部标题区域（占位锚点）
    local headerAnchor = CreateFrame("Frame", nil, frame)
    headerAnchor:SetSize(130, 22)
    headerAnchor:SetPoint("TOP", frame, "TOP", 0, -10)

    -- 标题
    local cpTitle = frame:CreateFontString(nil, "OVERLAY")
    cpTitle:SetFont("Fonts\\FRIZQT__.TTF", 14, "OUTLINE")
    cpTitle:SetPoint("CENTER", headerAnchor, "CENTER", 8, 0)
    cpTitle:SetText("|cffFFD700" .. botName)

    -- 职业图标（标题左侧）
    local classIcon = frame:CreateTexture(nil, "OVERLAY")
    classIcon:SetSize(22, 22)
    classIcon:SetPoint("RIGHT", cpTitle, "LEFT", -4, 0)
    -- 英文大写职业名 -> 图标库路径映射
    local classIcons = {
        ["WARRIOR"]     = "Interface\\Icons\\Ability_Warrior_OffensiveStance",
        ["PALADIN"]     = "Interface\\Icons\\Spell_Holy_HolyBolt",
        ["HUNTER"]      = "Interface\\Icons\\Ability_Hunter_BeastTraining",
        ["ROGUE"]       = "Interface\\Icons\\Ability_Rogue_Eviscerate",
        ["PRIEST"]      = "Interface\\Icons\\Spell_Holy_GuardianSpirit",
        ["DEATHKNIGHT"] = "Interface\\Icons\\Spell_Deathknight_ClassIcon",
        ["SHAMAN"]      = "Interface\\Icons\\Spell_Nature_BloodLust",
        ["MAGE"]        = "Interface\\Icons\\Spell_Holy_MagicalSentry",
        ["WARLOCK"]     = "Interface\\Icons\\Spell_Shadow_DeathCoil",
        ["DRUID"]       = "Interface\\Icons\\Ability_Druid_Maul",
    }
    local iconPath = classIcons[botClass] or classIcons["WARRIOR"]
    classIcon:SetTexture(iconPath)

    -- 控制按钮第一行：嘲讽（图标按钮） / 到达指定点
    local btnTaunt = CreateFrame("Button", nil, frame)
    btnTaunt:SetSize(40, 40)
    btnTaunt:SetPoint("TOPLEFT", headerAnchor, "BOTTOMLEFT", 0, -6)
    -- 通过 spellId 用 GetSpellInfo 获取嘲讽法术的真实图标
    local tauntData = TAUNT_SPELL_TABLE[botClass]
    local tauntIconPath = "Interface\\Icons\\Ability_Warrior_BattleShout"
    if tauntData and tauntData.spellId then
        local _, _, icon = GetSpellInfo(tauntData.spellId)
        if icon then
            tauntIconPath = icon
        end
    end
    local tauntTexture = btnTaunt:CreateTexture(nil, "BACKGROUND")
    tauntTexture:SetTexture(tauntIconPath)
    tauntTexture:SetAllPoints()
    btnTaunt:SetNormalTexture(tauntTexture)
    btnTaunt:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Square", "ADD")
    btnTaunt:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetText("嘲讽", 1, 1, 1, 1)
        GameTooltip:AddLine("嘲讽你的目标", 0.8, 0.8, 0.8, 1)
        GameTooltip:Show()
    end)
    btnTaunt:SetScript("OnLeave", function()
        GameTooltip:Hide()
    end)
    btnTaunt:SetScript("OnClick", function()
        local tauntTarget = UnitName("target")
        if not tauntTarget then
            ChatFrame1:AddMessage("|cffFFFF00[NetherBot] 请先选择一个要嘲讽的目标！")
            return
        end
        local tauntData = TAUNT_SPELL_TABLE[botClass]
        if not tauntData then
            ChatFrame1:AddMessage("|cffFFFF00[NetherBot] 该职业(" .. (botClass or "未知") .. ")没有嘲讽技能！")
            return
        end
        CommandNPCBotOrderCast_Player(botName, tauntData.name, 'mytarget')
    end)

    -- 攻击按钮（替换原"位移"按钮，复用动态面板同一创建方法，作用于锁定机器人 botName）
    local btnAttack = CreateAttackButton(frame, "LEFT", btnTaunt, "RIGHT", 8, 0, function() return botName end)
 

    local btnFollow = CreateFrame("Button", nil, frame)
    btnFollow:SetSize(40, 40)
    btnFollow:SetPoint("LEFT", btnAttack, "RIGHT", 8, 0)
    btnFollow:SetText("跟我")
    btnFollow:SetNormalFontObject("GameFontNormal")
    -- 记录当前是否处于集合模式
    btnFollow.isMassMode = false
    -- 细边框 + 黄色背景（普通模式）
    btnFollow:SetBackdrop({
        bgFile = "Interface\\Tooltips\\UI-Tooltip-Background",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true, tileSize = 16, edgeSize = 8,
        insets = { left = 2, right = 2, top = 2, bottom = 2 }
    })
    btnFollow:SetBackdropColor(0.45, 0.35, 0.05, 0.9)
    btnFollow:SetBackdropBorderColor(0.6, 0.45, 0.1, 1.0)
    -- 悬停/按下颜色变化（根据当前模式动态切换）
    btnFollow:SetScript("OnEnter", function()
        if btnFollow.isMassMode then
            btnFollow:SetBackdropColor(0.6, 0.15, 0.15, 1.0)
            btnFollow:SetBackdropBorderColor(0.75, 0.2, 0.2, 1.0)
        else
            btnFollow:SetBackdropColor(0.6, 0.45, 0.1, 1.0)
            btnFollow:SetBackdropBorderColor(0.75, 0.55, 0.15, 1.0)
        end
    end)
    btnFollow:SetScript("OnLeave", function()
        if btnFollow.isMassMode then
            btnFollow:SetBackdropColor(0.45, 0.1, 0.1, 0.9)
            btnFollow:SetBackdropBorderColor(0.6, 0.15, 0.15, 1.0)
        else
            btnFollow:SetBackdropColor(0.45, 0.35, 0.05, 0.9)
            btnFollow:SetBackdropBorderColor(0.6, 0.45, 0.1, 1.0)
        end
    end)
    btnFollow:SetScript("OnMouseDown", function()
        if btnFollow.isMassMode then
            btnFollow:SetBackdropColor(0.3, 0.05, 0.05, 1.0)
        else
            btnFollow:SetBackdropColor(0.3, 0.22, 0.02, 1.0)
        end
    end)
    btnFollow:SetScript("OnMouseUp", function()
        if btnFollow.isMassMode then
            btnFollow:SetBackdropColor(0.6, 0.15, 0.15, 1.0)
        else
            btnFollow:SetBackdropColor(0.6, 0.45, 0.1, 1.0)
        end
    end)
    btnFollow:SetScript("OnClick", function()
        if btnFollow.isMassMode then
            -- 当前是集合模式，点击取消集合
            SendChatMessage(".npcbot command unmass name " .. botName, "SAY")
            btnFollow.isMassMode = false
            btnFollow:SetText("跟我")
            btnFollow:SetBackdropColor(0.45, 0.35, 0.05, 0.9)
            btnFollow:SetBackdropBorderColor(0.6, 0.45, 0.1, 1.0)
        else
            -- 当前不是集合模式，点击开始集合
            SendChatMessage(".npcbot command mass name " .. botName, "SAY")
            btnFollow.isMassMode = true
            btnFollow:SetText("取消")
            btnFollow:SetBackdropColor(0.45, 0.1, 0.1, 0.9)
            btnFollow:SetBackdropBorderColor(0.6, 0.15, 0.15, 1.0)
        end
    end)

    -- 控制按钮第二行：跟我走位 / 靠近主T
    -- local btnFollowMe = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
    -- btnFollowMe:SetSize(120, 26)
    -- btnFollowMe:SetPoint("TOPLEFT", btnTaunt, "BOTTOMLEFT", 0, -6)
    -- btnFollowMe:SetText("跟我走位")
    -- local nt3 = btnFollowMe:GetNormalTexture()
    -- if nt3 then nt3:SetVertexColor(0.10, 0.90, 0.40) end
    -- btnFollowMe:SetScript("OnClick", function()
    --     SendChatMessage(".npcbot command follow", "SAY")
    -- end)

    -- local btnFollowTank = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
    -- btnFollowTank:SetSize(120, 26)
    -- btnFollowTank:SetPoint("TOPRIGHT", btnTaunt, "BOTTOMRIGHT", 0, -6)
    -- btnFollowTank:SetText("靠近主T")
    -- local nt4 = btnFollowTank:GetNormalTexture()
    -- if nt4 then nt4:SetVertexColor(0.10, 0.90, 0.40) end
    -- btnFollowTank:SetScript("OnClick", function()
    --     SendChatMessage(".npcbot command follow", "SAY")
    -- end)

    -- 团队技能区域锚点（用于后续技能按钮的相对定位）
    local teamAnchor = CreateFrame("Frame", nil, frame)
    teamAnchor:SetSize(130, 1)
    teamAnchor:SetPoint("TOPLEFT", btnTaunt, "BOTTOMLEFT", 0, -14)

    -- 团队技能标签
    local cpTeamLabel = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    cpTeamLabel:SetPoint("TOPLEFT", teamAnchor, "TOPLEFT", 0, 0)
    cpTeamLabel:SetText("左键: 自身/右键: 目标")
    cpTeamLabel:SetTextColor(1.00, 0.82, 0.10)

    -- 一次性填充团队技能
    local cpTeamSpellButtons = {}
    FillTeamSpellButtons(frame, botClass, teamAnchor, cpTeamSpellButtons, true, botName)

    frame:Show()
end

-- ====== 动态模式：无有效 NPCBot → 仅显示团队技能区域，随目标切换实时刷新 ======
function CreateDynamicControlPanel()
    local frame = CreateBaseControlFrame()
    frame:SetSize(180, 130)

    -- 顶部标题区域（占位锚点）
    local headerAnchor = CreateFrame("Frame", nil, frame)
    headerAnchor:SetSize(150, 22)
    headerAnchor:SetPoint("TOPLEFT", frame, "TOPLEFT", 12, -10)

    -- 标题
    local cpTitle = frame:CreateFontString(nil, "OVERLAY")
    cpTitle:SetFont("Fonts\\FRIZQT__.TTF", 14, "OUTLINE")
    cpTitle:SetPoint("LEFT", headerAnchor, "LEFT", 0, 0)
    cpTitle:SetText("|cffFFD700<请选择>")

    -- 职业图标（固定在标题区最左侧，名称显示在图标右侧）
    local classIcon = frame:CreateTexture(nil, "OVERLAY")
    classIcon:SetSize(22, 22)
    classIcon:SetPoint("LEFT", headerAnchor, "LEFT", 0, 0)
    -- 英文大写职业名 -> 图标库路径映射
    local classIcons = {
        ["WARRIOR"]     = "Interface\\Icons\\Ability_Warrior_OffensiveStance",
        ["PALADIN"]     = "Interface\\Icons\\Spell_Holy_HolyBolt",
        ["HUNTER"]      = "Interface\\Icons\\Ability_Hunter_BeastTraining",
        ["ROGUE"]       = "Interface\\Icons\\Ability_Rogue_Eviscerate",
        ["PRIEST"]      = "Interface\\Icons\\Spell_Holy_GuardianSpirit",
        ["DEATHKNIGHT"] = "Interface\\Icons\\Spell_Deathknight_ClassIcon",
        ["SHAMAN"]      = "Interface\\Icons\\Spell_Nature_BloodLust",
        ["MAGE"]        = "Interface\\Icons\\Spell_Holy_MagicalSentry",
        ["WARLOCK"]     = "Interface\\Icons\\Spell_Shadow_DeathCoil",
        ["DRUID"]       = "Interface\\Icons\\Ability_Druid_Maul",
    }
    -- 无有效目标时默认隐藏
    classIcon:Hide()

    -- 锁定状态
    local isDynamicLocked = false
    local lockedTargetName = nil  -- 锁定时保存的目标名
    local recordedBotName = nil   -- 动态模式记录的有效机器人目标名（切到区间内目标时更新）
    local cpTeamSpellButtons = {}
    local DynamicRefresh  -- 前向声明

    -- 攻击按钮（作用于面板记录的目标名，攻击/取消切换）
    local btnAttack = CreateAttackButton(frame, "TOPLEFT", headerAnchor, "BOTTOMLEFT", 0, -6, function() return recordedBotName end)

    -- 跟我按钮（照搬锁定模式功能，作用于面板记录的目标名）
    local btnFollow = CreateFrame("Button", nil, frame)
    btnFollow:SetSize(40, 40)
    btnFollow:SetPoint("LEFT", btnAttack, "RIGHT", 8, 0)
    btnFollow:SetText("跟我")
    btnFollow:SetNormalFontObject("GameFontNormal")
    -- 记录当前是否处于集合模式
    btnFollow.isMassMode = false
    -- 细边框 + 黄色背景（普通模式）
    btnFollow:SetBackdrop({
        bgFile = "Interface\\Tooltips\\UI-Tooltip-Background",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true, tileSize = 16, edgeSize = 8,
        insets = { left = 2, right = 2, top = 2, bottom = 2 }
    })
    btnFollow:SetBackdropColor(0.45, 0.35, 0.05, 0.9)
    btnFollow:SetBackdropBorderColor(0.6, 0.45, 0.1, 1.0)
    -- 悬停/按下颜色变化（根据当前模式动态切换）
    btnFollow:SetScript("OnEnter", function()
        if btnFollow.isMassMode then
            btnFollow:SetBackdropColor(0.6, 0.15, 0.15, 1.0)
            btnFollow:SetBackdropBorderColor(0.75, 0.2, 0.2, 1.0)
        else
            btnFollow:SetBackdropColor(0.6, 0.45, 0.1, 1.0)
            btnFollow:SetBackdropBorderColor(0.75, 0.55, 0.15, 1.0)
        end
    end)
    btnFollow:SetScript("OnLeave", function()
        if btnFollow.isMassMode then
            btnFollow:SetBackdropColor(0.45, 0.1, 0.1, 0.9)
            btnFollow:SetBackdropBorderColor(0.6, 0.15, 0.15, 1.0)
        else
            btnFollow:SetBackdropColor(0.45, 0.35, 0.05, 0.9)
            btnFollow:SetBackdropBorderColor(0.6, 0.45, 0.1, 1.0)
        end
    end)
    btnFollow:SetScript("OnMouseDown", function()
        if btnFollow.isMassMode then
            btnFollow:SetBackdropColor(0.3, 0.05, 0.05, 1.0)
        else
            btnFollow:SetBackdropColor(0.3, 0.22, 0.02, 1.0)
        end
    end)
    btnFollow:SetScript("OnMouseUp", function()
        if btnFollow.isMassMode then
            btnFollow:SetBackdropColor(0.6, 0.15, 0.15, 1.0)
        else
            btnFollow:SetBackdropColor(0.6, 0.45, 0.1, 1.0)
        end
    end)
    btnFollow:SetScript("OnClick", function()
        -- 没有记录的有效机器人目标时不响应
        if not recordedBotName then
            ChatFrame1:AddMessage("|cffFFFF00[NetherBot] 请先选择一个有效的 NPCBot 目标！")
            return
        end
        if btnFollow.isMassMode then
            -- 当前是集合模式，点击取消集合
            SendChatMessage(".npcbot command unmass name " .. recordedBotName, "SAY")
            btnFollow.isMassMode = false
            btnFollow:SetText("跟我")
            btnFollow:SetBackdropColor(0.45, 0.35, 0.05, 0.9)
            btnFollow:SetBackdropBorderColor(0.6, 0.45, 0.1, 1.0)
        else
            -- 当前不是集合模式，点击开始集合
            SendChatMessage(".npcbot command mass name " .. recordedBotName, "SAY")
            btnFollow.isMassMode = true
            btnFollow:SetText("取消")
            btnFollow:SetBackdropColor(0.45, 0.1, 0.1, 0.9)
            btnFollow:SetBackdropBorderColor(0.6, 0.15, 0.15, 1.0)
        end
    end)

    -- 团队技能区域锚点（提前创建，供锁/解按钮的 OnClick 使用）
    local teamAnchor = CreateFrame("Frame", nil, frame)
    teamAnchor:SetSize(156, 1)
    teamAnchor:SetPoint("TOPLEFT", btnAttack, "BOTTOMLEFT", 0, -14)

    -- 锁/解按钮（标题右侧，距关闭按钮左侧）
    local btnLockToggle = CreateFrame("Button", nil, frame, "UIPanelButtonTemplate")
    btnLockToggle:SetSize(32, 20)
    btnLockToggle:SetPoint("TOPRIGHT", frame, "TOPRIGHT", -32, -10)
    btnLockToggle:SetText("锁")
    btnLockToggle:GetNormalTexture():SetVertexColor(0.90, 0.60, 0.20)
    btnLockToggle:SetScript("OnClick", function()
        -- 只有选中 entry 在 [70000, 80000] 区间内的目标才允许锁定
        if not isDynamicLocked and not IsNPCBotEntry("target") then
            ChatFrame1:AddMessage("|cffFFFF00[NetherBot] 请先选择一个有效的 NPCBot 目标！")
            return
        end
        isDynamicLocked = not isDynamicLocked
        if isDynamicLocked then
            -- 锁定时使用面板记录的目标名，并给所有现有技能按钮设置锁定名
            lockedTargetName = recordedBotName or UnitName("target")
            for _, btn in ipairs(cpTeamSpellButtons) do
                btn.lockedBotName = lockedTargetName
            end
            if lockedTargetName then
                cpTitle:SetText(" " .. lockedTargetName .. " (锁定)")
            else
                cpTitle:SetText("<无目标>")
            end
            btnLockToggle:SetText("解")
            btnLockToggle:GetNormalTexture():SetVertexColor(0.20, 0.80, 0.30)
        else
            -- 解锁时清空锁定名，并清除按钮的锁定名
            lockedTargetName = nil
            for _, btn in ipairs(cpTeamSpellButtons) do
                btn.lockedBotName = nil
            end
            btnLockToggle:SetText("锁")
            btnLockToggle:GetNormalTexture():SetVertexColor(0.90, 0.60, 0.20)
            DynamicRefresh()
        end
    end)

    -- 团队技能标签
    local cpTeamLabel = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    cpTeamLabel:SetPoint("TOPLEFT", teamAnchor, "TOPLEFT", 0, 0)
    cpTeamLabel:SetText("技能")
    cpTeamLabel:SetTextColor(1.00, 0.82, 0.10)

    -- 刷新：根据当前目标职业重建技能按钮，并更新标题
    DynamicRefresh = function()
        -- 锁定状态下不刷新（按钮和标题已在锁定时设置）
        if isDynamicLocked then return end

        -- 只有选中 entry 在 [70000, 80000] 区间内的目标才以新目标刷新动态面板
        -- 区间外的目标（普通玩家、普通 NPC、无目标）保持上一个目标的面板不变
        if not IsNPCBotEntry("target") then
            return
        end

        -- 记录当前有效机器人目标名，面板后续操作（技能点击）都使用它，
        -- 直到切换到下一个区间内目标才更新
        local targetName = UnitName("target")
        if not targetName then
            return
        end
        recordedBotName = targetName
        -- 更新标题为记录的目标名
        cpTitle:SetText(" " .. recordedBotName)

        -- 更新职业图标（有图标时名称贴在图标右侧，无图标时名称回到最左侧）
        local _, cls = UnitClass("target")
        if cls and classIcons[cls] then
            classIcon:SetTexture(classIcons[cls])
            classIcon:Show()
            cpTitle:ClearAllPoints()
            cpTitle:SetPoint("LEFT", classIcon, "RIGHT", 4, 0)
        else
            classIcon:Hide()
            cpTitle:ClearAllPoints()
            cpTitle:SetPoint("LEFT", headerAnchor, "LEFT", 0, 0)
        end
        -- 切换到新机器人时重置"攻击"状态
        btnAttack.isAttackMode = false
        btnAttack:GetNormalTexture():SetVertexColor(1, 1, 1)
        btnAttack:SetBackdropColor(0.45, 0.35, 0.05, 0.9)
        btnAttack:SetBackdropBorderColor(0.6, 0.45, 0.1, 1.0)
        -- 切换到新机器人时重置"跟我"集合状态
        btnFollow.isMassMode = false
        btnFollow:SetText("跟我")
        btnFollow:SetBackdropColor(0.45, 0.35, 0.05, 0.9)
        btnFollow:SetBackdropBorderColor(0.6, 0.45, 0.1, 1.0)

        if not cls or not TEAM_DEFENSIVE_SPELLS[cls] then
            for _, btn in ipairs(cpTeamSpellButtons) do
                btn:Hide()
                btn:SetScript("OnClick", nil)
            end
            while #cpTeamSpellButtons > 0 do table.remove(cpTeamSpellButtons) end
            frame:SetHeight(150)
            return
        end
        -- 非锁定时使用记录的目标名（而非玩家当前 target），保证面板指向的目标不被切换干扰
        FillTeamSpellButtons(frame, cls, teamAnchor, cpTeamSpellButtons, false, recordedBotName)
    end

    -- 监听目标切换：仅在非锁定状态下刷新
    local eventFrame = CreateFrame("Frame", nil, frame)
    eventFrame:RegisterEvent("PLAYER_TARGET_CHANGED")
    eventFrame:SetScript("OnEvent", function()
        if frame:IsShown() and not isDynamicLocked then
            DynamicRefresh()
        end
    end)

    -- 初始立即刷新一次
    DynamicRefresh()

    frame:Show()
end

-- 入口：根据当前目标判定模式并创建对应窗口
function CreateControlPanelWindow()
    -- 选中了目标，但该目标的 entry 不在 [70000, 80000] 区间内
    -- （普通玩家、普通 NPC、区间外 NPC）→ 点"控制"无任何反应，不弹窗
    -- 注：没有目标时仍允许创建动态窗口，以便后续选中目标时刷新
    if UnitExists("target") and not IsNPCBotEntry("target") then
        return
    end

    if IsValidNPCBot("target") then
        local botName = UnitName("target")
        local _, botClass = UnitClass("target")
        CreateLockedControlPanel(botName, botClass)
    else
        CreateDynamicControlPanel()
    end
end

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 控制面板窗口结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 自定义插件显示/隐藏命令开始 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
-- 固定写法：[SLASH_] + [名称] + [数字]，[名称]可以使用下划线
-- SlashCmdList 集合新增元素时，必须使用[名称]
SLASH_NETHER_BOT_CMD1 = '/netherbot'
SLASH_NETHER_BOT_CMD2 = '/nb'
SlashCmdList['NETHER_BOT_CMD'] = function(msg)
    if msg == "show" or msg == "s" then
        titleFrame:Show()
        mainFrame:Show()
    elseif msg == "hide" or msg == "h" then
        titleFrame:Hide()
        mainFrame:Hide()
        gameMasterFrame:Hide()
        lookupFrame:Hide()
        castSpellFrame:Hide()
    end
end
-- >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 自定义插件显示/隐藏命令结束 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

-- ==================== GPS 坐标复制功能 ====================
-- 玩家输入 .gps 命令后，自动解析输出的 XYZ 坐标并准备好复制

-- 剪贴板弹窗框架
local GPSClipboardFrame = CreateFrame("Frame", "NetherBotGPSClipboard", UIParent)
GPSClipboardFrame:SetSize(280, 100)
GPSClipboardFrame:SetPoint("CENTER", UIParent, "CENTER", 0, 0)
GPSClipboardFrame:SetFrameStrata("DIALOG")
GPSClipboardFrame:SetBackdrop({
    bgFile = "Interface\\Tooltips\\UI-Tooltip-Background",
    edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
GPSClipboardFrame:SetBackdropColor(0.08, 0.08, 0.15, 0.95)
GPSClipboardFrame:SetBackdropBorderColor(0.5, 0.5, 0.7, 1.0)
GPSClipboardFrame:SetMovable(true)
GPSClipboardFrame:EnableMouse(true)
GPSClipboardFrame:RegisterForDrag("LeftButton")
GPSClipboardFrame:SetScript("OnDragStart", function(self) self:StartMoving() end)
GPSClipboardFrame:SetScript("OnDragStop", function(self) self:StopMovingOrSizing() end)
GPSClipboardFrame:Hide()

-- 标题
local gpsTitle = GPSClipboardFrame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
gpsTitle:SetPoint("TOP", GPSClipboardFrame, "TOP", 0, -12)
gpsTitle:SetText("GPS 坐标")
gpsTitle:SetTextColor(1, 0.82, 0.1, 1)

-- 关闭按钮
local gpsCloseBtn = CreateFrame("Button", nil, GPSClipboardFrame, "UIPanelCloseButton")
gpsCloseBtn:SetPoint("TOPRIGHT", GPSClipboardFrame, "TOPRIGHT", -4, -4)
gpsCloseBtn:SetScript("OnClick", function() GPSClipboardFrame:Hide() end)

-- 坐标输入框（用于复制，用户按 Ctrl+C 即可）
local gpsEditBox = CreateFrame("EditBox", nil, GPSClipboardFrame)
gpsEditBox:SetSize(240, 24)
gpsEditBox:SetPoint("TOP", gpsTitle, "BOTTOM", 0, -6)
gpsEditBox:SetFontObject("GameFontHighlight")
gpsEditBox:SetBackdrop({
    bgFile = "Interface\\Tooltips\\UI-Tooltip-Background",
    edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
    tile = true, tileSize = 16, edgeSize = 16,
    insets = { left = 4, right = 4, top = 4, bottom = 4 }
})
gpsEditBox:SetBackdropColor(0, 0, 0, 0.85)
gpsEditBox:SetBackdropBorderColor(0.4, 0.4, 0.6, 1)
gpsEditBox:SetTextInsets(6, 6, 4, 4)
gpsEditBox:SetMultiLine(false)
gpsEditBox:SetScript("OnEscapePressed", function() GPSClipboardFrame:Hide() end)
gpsEditBox:SetScript("OnEnterPressed", function() GPSClipboardFrame:Hide() end)

-- 提示文字
local gpsHint = GPSClipboardFrame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
gpsHint:SetPoint("TOP", gpsEditBox, "BOTTOM", 0, -6)
gpsHint:SetText("按 Ctrl+C 复制坐标")
gpsHint:SetTextColor(0.6, 0.85, 0.6, 1)

-- 自动关闭计时（10 秒）
local gpsAutoDismissTime = 0
GPSClipboardFrame:SetScript("OnUpdate", function(self, elapsed)
    if self:IsShown() then
        gpsAutoDismissTime = gpsAutoDismissTime + elapsed
        if gpsAutoDismissTime >= 10 then
            self:Hide()
            gpsAutoDismissTime = 0
        end
    else
        gpsAutoDismissTime = 0
    end
end)

-- 显示 GPS 坐标到剪贴板弹窗
local function ShowGPSClipboard(x, y, z)
    local coordText = string.format("%.2f,%.2f,%.2f", x, y, z)
    gpsEditBox:SetText(coordText)
    GPSClipboardFrame:Show()
    gpsEditBox:SetFocus()
    gpsEditBox:HighlightText()
    gpsAutoDismissTime = 0  -- 重置计时器
end

-- 监听系统消息，自动解析 .gps 命令输出中的 XYZ 坐标
local gpsParseFrame = CreateFrame("Frame")
gpsParseFrame:RegisterEvent("CHAT_MSG_SYSTEM")
gpsParseFrame:SetScript("OnEvent", function(_, _, msg)
    -- .gps 命令输出格式: X: -8833.38 Y: 628.62 Z: 94.29 ...
    -- 正则提取三个坐标值
    local x, y, z = string.match(msg, "X:%s*([%-%d%.]+)%s+Y:%s*([%-%d%.]+)%s+Z:%s*([%-%d%.]+)")
    if x and y and z then
        ShowGPSClipboard(tonumber(x), tonumber(y), tonumber(z))
    end
end)