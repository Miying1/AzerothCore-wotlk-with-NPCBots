-- Transmogrification 插件 简体中文本地化文件
local L = LibStub("AceLocale-3.0"):NewLocale("Transmogrification", "zhCN")
if not L then return end

-- 幻化窗口
L["Transmogrify"] = "幻化"
L["Collected Item Appearances"] = "已收集的物品外观"
L["Filter Item Appearance"] = "筛选物品外观"
L["No item equipped in this slot."] = "此栏位没有装备任何物品。"
L["Page %s"] = "第 %s 页"
L["Show Cloak"] = "显示披风"
L["Show Helm"] = "显示头盔"
L["You must have an item equipped in this slot to hide its appearance."] = "你必须在此栏位装备物品才能隐藏其外观。"
L["You must have an item equipped in this slot to restore its appearance."] = "你必须在此栏位装备物品才能恢复其外观。"

-- 插件功能
L["You must "] = "你必须 "
L["/reload"] = "/reload"
L[" the interface for this change to take effect."] = " 界面才能使此更改生效。"
L["Querying the server for collected transmogrification appearances..."] = "正在向服务器查询已收集的幻化外观……"
L["No transmogrification appearances could be located for this account. If you believe this is an error, please contact a Game Master."] = "无法找到此账号的幻化外观。如果你认为这是错误，请联系游戏管理员。"
L["Your transmogrification appearance collection has been successfully synchronized!"] = "你的幻化外观收藏已成功同步！"
L["You have collected "] = "你已收集 "
L[" transmogrification appearances."] = " 个幻化外观。"
L["It is recommended that you "] = "建议你"
L[" your interface to finalize any changes, otherwise the "] = " 界面以完成任何更改，否则“"
L[" tooltip line may not function correctly."] = "”提示行可能无法正常工作。"
L["Would you like to reload the interface?"] = "你希望重载界面吗？"
L["Yes"] = "是"
L["No"] = "否"

-- 提示文本
L["New Appearance"] = "新外观"
L["Click to preview this item."] = "点击预览此物品。"
L["Hidden Appearance"] = "已隐藏外观"
L["Restore Item Appearance"] = "恢复物品外观"
L["Hide Item"] = "隐藏物品"
L["Restore All Item Appearances"] = "恢复所有物品外观"
L["Hide All Items"] = "隐藏所有物品"
L["Toggle Character Cloak Display"] = "切换角色披风显示"
L["This checkbox provides the same function as\nticking or unticking the \"Show Cloak\" checkbox\nin the interface options menu. It will have no\neffect on the transmogrify preview window."] = "此复选框与界面选项菜单中\n勾选或取消勾选“显示披风”复选框\n的功能相同。对幻化预览窗口\n没有任何影响。"
L["Toggle Character Helm Display"] = "切换角色头盔显示"
L["This checkbox provides the same function as\nticking or unticking the \"Show Helm\" checkbox\nin the interface options menu. It will have no\neffect on the transmogrify preview window."] = "此复选框与界面选项菜单中\n勾选或取消勾选“显示头盔”复选框\n的功能相同。对幻化预览窗口\n没有任何影响。"
L["No appearances to apply."] = "没有可应用的外观。"

-- 文本（十六进制）颜色代码
L["00ccff"] = true -- 高亮文本
L["f194f7"] = true -- 新外观提示
L["00ff00"] = true -- 预览物品
L["b2b2b2"] = true -- 搜索筛选提示
L["ff4040"] = true -- 未装备物品警告

-- 幻化窗口选项
L["Transmogrification Window Options"] = "幻化窗口选项"
L["Transmogrification Window Scale"] = "幻化窗口缩放"
L["Determines the scale of the Transmogrification window."] = "决定幻化窗口的缩放比例。"
L["Transmogrification Window Opacity"] = "幻化窗口透明度"
L["Determines the opacity of the Transmogrification window."] = "决定幻化窗口的透明度。"
L["Transmogrification Window Lock"] = "锁定幻化窗口"
L["Locks the position of the Transmogrification window."] = "锁定幻化窗口的位置。"

-- 显示选项
L["Display Options"] = "显示选项"
L["Display New Appearance Tooltip"] = "显示“新外观”提示"
L["Toggles the display of the "] = "切换“"
L[" tooltip line."] = "”提示行的显示。"
L["Display Collection Messages"] = "显示收集消息"
L["Toggles the display of the new appearance system message when collecting a new transmogrification appearance."] = "切换在收集到新的幻化外观时显示“新外观”系统消息。"

-- 收藏管理
L["Collection Management"] = "收藏管理"
L["Sync Collection"] = "同步收藏"
L["Creates a local list of collected transmogrification appearances. The collected transmogrification appearances list is used to display the "] = "创建已收集幻化外观的本地列表。该列表用于显示“"
L[" tooltip."] = "”提示。"
L["This button provides the same function as using the "] = "此按钮与使用“"
L["/transmog sync"] = "/transmog sync"
L[" command."] = "”命令的功能相同。"

-- 下面的文本必须与本语言在服务器 Eluna 脚本中配置的 LOOT_ITEM_LOCALE 完全一致，
-- 否则自动将新外观添加到本地 CollectedAppearances 表格的功能将无法正常工作。
--
-- 除非你了解自己在做什么，或者正在翻译一种语言，否则请勿编辑下面这一行。
L["has been added to your appearance collection."] = "已被添加到你的外观收藏中。"
