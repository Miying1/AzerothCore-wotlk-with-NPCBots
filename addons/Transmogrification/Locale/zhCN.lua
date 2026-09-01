-- Transmogrification 插件 简体中文本地化文件
local L = LibStub("AceLocale-3.0"):NewLocale("Transmogrification", "zhCN")
if not L then return end

-- 幻化窗口
L["Transmogrify"] = "幻化"
L["Cost"] = "花费"
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
L["It is recommended that you "] = "建议你"
L[" your interface to finalize any changes, otherwise the "] = " 界面以完成任何更改，否则“"
L[" tooltip line may not function correctly."] = "”提示行可能无法正常工作。"
L["Would you like to reload the interface?"] = "你希望重载界面吗？"
L["Yes"] = "是"
L["No"] = "否"

-- 提示文本
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
