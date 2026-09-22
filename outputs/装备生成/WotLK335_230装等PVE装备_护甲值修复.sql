-- WotLK 3.3.5 / 230 装等 PVE 装备：修复 7 件力量板甲缺失的数值
-- 修复依据：使用相同等级(230)、相同部位、相同类型(力量输出板甲 subclass=4)的正常装备对应字段值
-- 缺失字段：armor(护甲值)、MaxDurability(耐久度)、RequiredDisenchantSkill(分解所需技能)、DisenchantID(分解掉落)
-- 参考装备：99744(头) 99748(肩) 99664(腿) 99772(手)
-- 注：BuyPrice/SellPrice 不修复，因为 buy=0 是绑定装备(基准为绑定套装)的正常表现，同文件约 1/3 的正常护甲装备 buy 也为 0

UPDATE `item_template` SET `armor`=1914, `MaxDurability`=100, `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=99743; -- 降临战盔(头部)
UPDATE `item_template` SET `armor`=1914, `MaxDurability`=100, `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=99746; -- 先驱者之面甲(头部)
UPDATE `item_template` SET `armor`=1767, `MaxDurability`=100, `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=99747; -- 御风者肩卫(肩部)
UPDATE `item_template` SET `armor`=1767, `MaxDurability`=100, `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=99750; -- 霸主肩卫(肩部)
UPDATE `item_template` SET `armor`=2061, `MaxDurability`=120, `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=99760; -- 夜幕守护腿胄(腿部)
UPDATE `item_template` SET `armor`=2061, `MaxDurability`=120, `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=99762; -- 符文驭天腿胄(腿部)
UPDATE `item_template` SET `armor`=1471, `MaxDurability`=55,  `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=99771; -- 驭雷拳甲(手部)
