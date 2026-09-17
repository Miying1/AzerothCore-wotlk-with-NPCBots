-- ============================================================================
-- 可驯服宠物生成模板（变量控制版，可重复执行）
-- ============================================================================
-- 使用方法：
--   1. 只改下方「变量区」的变量
--   2. 整段执行即可生成一个生物（同 entry 重复执行会自动删除重建，幂等）
--   3. 要生成多个生物：复制本文件 / 修改变量后再次执行
--
-- 默认生成规则：
--   · 等级固定 79 级
--   · 阵营 faction = 188（中立黄名、可被玩家攻击，被打会反击不主动打人）
--   · type_flags = 1（普通可驯服，已去除灵魂兽等异域标志，任何猎人都能驯服）
--   · type = 1（野兽）、family 由变量指定（决定独有技能与天赋树）
--
-- 家族编号对照（@family）：
--   1狼 2猫 3蜘蛛 4熊 5野猪 6鳄鱼 7食腐鸟 8螃蟹 9猩猩
--   11迅猛龙 12陆行鸟 20蝎子 21乌龟 24蝙蝠 25鬣狗 26猛禽 27风蛇
--   30龙鹰 31虚空鳐 32掠夺者 33孢子蝙蝠 34跃迁追猎者 35蛇 37飞蛾 44黄蜂
--   （异域：38奇美拉 39魔暴龙 41其拉虫 42沙虫 43犀牛 45熔岩犬 46灵魂兽）
-- ============================================================================

-- ----------------------------------------------------------------------------
-- 【变量区】每次生成只需改这里
-- ----------------------------------------------------------------------------
SET @entry  := 94000;            -- 生物 entry（必须唯一，重复执行会覆盖）
SET @name   := '玄牛幼崽';       -- 生物名称
SET @model  := 100002;            -- 模型 ID（CreatureDisplayID，对应 creature_template_model）
SET @scale  := 0.7;              -- 模型缩放值（1.0 = 原始大小，可 0.5~2.0）
SET @family := 1;                -- 宠物家族编号（决定独有技能与天赋树，见上方对照）
SET @hp     := 5550;             -- 驯服后宠物血量（79级基准）：标准 5050 ｜ 狼系高血 7208
SET @bounding := 1;            -- 模型边界半径（选中/交互范围），模型不在 creature_model_info 时用于补记录
SET @combat   := 1.3;            -- 近战距离，模型不在 creature_model_info 时用于补记录

-- ----------------------------------------------------------------------------
-- 【自动推导区】@petspelldata 按 @family 自动取同族可驯服生物的现成值，一般不用改
--   （想手动指定：改成 SET @petspelldata := 0; 或某个同族生物的 ID）
-- ----------------------------------------------------------------------------
SET @petspelldata := COALESCE(
    (SELECT PetSpellDataId FROM creature_template
      WHERE family = @family AND type = 1 AND (type_flags & 1) = 1
      ORDER BY entry LIMIT 1), 0);

-- ----------------------------------------------------------------------------
-- 1. 生物主模板 creature_template
-- ----------------------------------------------------------------------------
DELETE FROM `creature_template` WHERE `entry` = @entry;
INSERT INTO `creature_template`
(`entry`,`difficulty_entry_1`,`difficulty_entry_2`,`difficulty_entry_3`,`KillCredit1`,`KillCredit2`,
 `name`,`subname`,`IconName`,`gossip_menu_id`,`minlevel`,`maxlevel`,`exp`,`faction`,`npcflag`,
 `speed_walk`,`speed_run`,`speed_swim`,`speed_flight`,`detection_range`,`rank`,`dmgschool`,`DamageModifier`,
 `BaseAttackTime`,`RangeAttackTime`,`BaseVariance`,`RangeVariance`,`unit_class`,`unit_flags`,`unit_flags2`,
 `dynamicflags`,`family`,`type`,`type_flags`,`lootid`,`pickpocketloot`,`skinloot`,`PetSpellDataId`,`VehicleId`,
 `mingold`,`maxgold`,`AIName`,`MovementType`,`HoverHeight`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,
 `ExperienceModifier`,`RacialLeader`,`movementId`,`RegenHealth`,`CreatureImmunitiesId`,`flags_extra`,`ScriptName`,`VerifiedBuild`)
VALUES
(@entry,0,0,0,0,0,@name,'','',0,79,79,2,188,0,1,1.14286,1,1,20,0,0,1,2000,2000,1,1,1,0,2048,0,
 @family,1,1,0,0,0,@petspelldata,0,0,0,'',0,1,1,1,1,1,0,0,1,0,0,'',12340);

-- ----------------------------------------------------------------------------
-- 2. 模型 creature_template_model
-- ----------------------------------------------------------------------------
DELETE FROM `creature_template_model` WHERE `CreatureID` = @entry;
INSERT INTO `creature_template_model` (`CreatureID`,`Idx`,`CreatureDisplayID`,`DisplayScale`,`Probability`,`VerifiedBuild`)
VALUES (@entry,0,@model,@scale,1,12340);

-- ----------------------------------------------------------------------------
-- 3. 模型附加信息 creature_model_info（模型不在表里时自动补一条，已存在则跳过）
--    作用：决定野生状态下的选中/交互范围(BoundingRadius)与近战距离(CombatReach)
--    注意：只影响"野生生物"（.npc add 后未驯服时）；驯服成宠物后会强制用固定值
--          （bounding=1.0、combat=1.5），故此处缺失不影响宠物本身，仅影响野外行为
-- ----------------------------------------------------------------------------
INSERT INTO `creature_model_info` (`DisplayID`,`BoundingRadius`,`CombatReach`,`Gender`,`DisplayID_Other_Gender`,`VerifiedBuild`)
SELECT @model, @bounding, @combat, 2, 0, 12340
WHERE NOT EXISTS (SELECT 1 FROM `creature_model_info` WHERE `DisplayID` = @model);

-- ----------------------------------------------------------------------------
-- 4. 宠物等级属性 pet_levelstats（79 级基准，决定驯服后宠物血量）
--    · @hp 才是驯服后宠物的真实血量来源（creature_template.HealthModifier 对宠物无效）
--    · 官方 79 级参考值：物理系(猫/蟹等) 5050 ｜ 狼系高血 7208
--    · 熊/风蛇/灵魂兽等 family 官方无 pet_levelstats 数据，按上面两档自定义即可
--    · mana 对猎人宠物无影响（猎人宠物用集中值 Focus）
-- ----------------------------------------------------------------------------
DELETE FROM `pet_levelstats` WHERE `creature_entry` = @entry;
INSERT INTO `pet_levelstats` (`creature_entry`,`level`,`hp`,`mana`,`str`,`agi`,`sta`,`inte`,`spi`,`armor`,`min_dmg`,`max_dmg`)
VALUES (@entry,79,@hp,1,189,155,351,69,113,9485,0,0);

-- ----------------------------------------------------------------------------
-- 验证查询（可选）：检查生成的生物是否符合驯服三要素
-- ----------------------------------------------------------------------------
SELECT `entry`,`name`,`minlevel`,`maxlevel`,`faction`,`family`,`type`,`type_flags`,`PetSpellDataId`,`HealthModifier`
FROM `creature_template` WHERE `entry` = @entry;
