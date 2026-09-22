-- WotLK 3.3.5 / 250 装等 PVE 装备：修复 6 件力量板甲缺失的数值
-- 修复依据：使用相同等级(250)、相同部位、相同类型(力量输出板甲 subclass=4)的正常装备对应字段值
-- 缺失字段：armor(护甲值)、MaxDurability(耐久度)、RequiredDisenchantSkill(分解所需技能)、DisenchantID(分解掉落)
-- 参考装备：100915(头) 100919(肩) 100923(胸) 100931(腿) 100942(手)

UPDATE `item_template` SET `armor`=2046, `MaxDurability`=100, `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=100914; -- 破阵面罩(头部)
UPDATE `item_template` SET `armor`=1889, `MaxDurability`=100, `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=100918; -- 战争领主肩卫(肩部)
UPDATE `item_template` SET `armor`=1889, `MaxDurability`=100, `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=100920; -- 银河回响肩甲(肩部)
UPDATE `item_template` SET `armor`=2519, `MaxDurability`=165, `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=100922; -- 奥妮克希亚之胸甲(胸部)
UPDATE `item_template` SET `armor`=2204, `MaxDurability`=120, `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=100932; -- 沙塔斯圣光腿甲(腿部)
UPDATE `item_template` SET `armor`=1573, `MaxDurability`=55,  `RequiredDisenchantSkill`=375, `DisenchantID`=68 WHERE `entry`=100941; -- 炽焰·护手(手部)
