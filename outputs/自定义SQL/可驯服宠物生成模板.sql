-- ============================================================================
-- 可驯服宠物生成模板（变量控制版，可重复执行）
-- ============================================================================
-- 使用方法：
--   1. 只改下方「变量区」的变量
--   2. 整段执行即可生成一个生物（同 entry 重复执行会自动删除重建，幂等）
--   3. 要生成多个生物：复制本文件 / 修改变量后再次执行
--      （批量补数据可参考 outputs/自定义SQL/可驯服宠物补pet_levelstats_94000-*.sql）
--
-- 默认生成规则：
--   · 等级由 @level 指定（默认 79）
--   · 阵营 faction 由 @faction 指定（默认 188 = 中立黄名、可被玩家攻击，被打会反击不主动打人）
--   · type_flags = 1（普通可驯服，不含异域标志，任何猎人都能驯服）
--   · type = 1（野兽）、family 由变量指定（决定独有技能与天赋树）
--
-- 家族编号对照（@family）：
--   1狼 2猫 3蜘蛛 4熊 5野猪 6鳄鱼 7食腐鸟 8螃蟹 9猩猩
--   11迅猛龙 12陆行鸟 20蝎子 21乌龟 24蝙蝠 25鬣狗 26猛禽 27风蛇
--   30龙鹰 31虚空鳐 32掠夺者 33孢子蝙蝠 34跃迁追猎者 35蛇 37飞蛾 44黄蜂
--   （异域：38奇美拉 39魔暴龙 41其拉虫 42沙虫 43犀牛 45熔岩犬 46灵魂兽
--     异域家族要把 type_flags 改为 65537（= 1 | 0x10000 TAMEABLE_EXOTIC），
--     否则会被当成普通宠物、任何猎人都能驯服）
-- ============================================================================

-- ----------------------------------------------------------------------------
-- 【变量区】每次生成只需改这里
-- ----------------------------------------------------------------------------
SET @entry    := 94000;          -- 生物 entry（必须唯一，重复执行会覆盖）
SET @name     := '玄牛幼崽';      -- 生物名称
SET @subname  := '';             -- 生物副标题（一般留空）
SET @model    := 100002;         -- 模型 ID（CreatureDisplayID，对应 creature_template_model）
SET @scale    := 0.7;            -- 模型缩放值（1.0 = 原始大小，可 0.5~2.0）
SET @family   := 1;              -- 宠物家族编号（决定独有技能与天赋树，见上方对照）
SET @level    := 79;             -- 生物等级（minlevel = maxlevel）
SET @faction  := 188;            -- 阵营 FactionTemplate（188 = 中立黄名）
SET @bounding := 1;              -- 模型边界半径（选中/交互范围），模型不在 creature_model_info 时用于补记录
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
--    关键字段：type = 1(野兽)、family = @family、type_flags = 1(可驯服)、exp = 2(WotLK)
--    说明：HealthModifier/ManaModifier 等对猎人宠物无效（见第 4 节）
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
(@entry,0,0,0,0,0,@name,@subname,'',0,@level,@level,2,@faction,0,1,1.14286,1,1,20,0,0,1,2000,2000,1,1,1,0,2048,0,
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
FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM `creature_model_info` WHERE `DisplayID` = @model);

-- ----------------------------------------------------------------------------
-- 4. 宠物等级属性 pet_levelstats（复制 entry = 1 的 1~80 级数据，不影响驯服后属性）
--    · LoadPetLevelInfo() 会校验 pet_levelstats 里每个 entry：
--      完全没有行 → 跳过（不报错）；有行但缺某等级 → 逐级填充并打印 ERROR；
--      缺 level = 1 → 致命错误 + exit(1)（"does not have pet stats data for Level 1!"）。
--      故这里直接复制一份完整的 1~80 级数据，既通过校验又不刷屏。
--    · 注意：猎人宠物查表时 entry 被强制替换为 1
--      （Pet.cpp: `uint32 creature_ID = (petType == HUNTER_PET) ? 1 : cinfo->Entry;`），
--      所以真正决定驯服后属性的永远是 `pet_levelstats` 里 entry = 1 的数据，
--      本段为自定义 entry 复制出来的行不会生效，仅用于通过完整性校验。
--    · 想改猎人宠物血量：改 entry = 1 的对应等级行（会影响全服所有猎人宠物），
--      或用脚本钩子 / 宠物光环缩放做定向调整。
--    · 若不想写任何行：直接注释掉整段即可（该 entry 不会被遍历，同样不报错）。
-- ----------------------------------------------------------------------------
DELETE FROM `pet_levelstats` WHERE `creature_entry` = @entry;
INSERT INTO `pet_levelstats` (`creature_entry`,`level`,`hp`,`mana`,`str`,`agi`,`sta`,`inte`,`spi`,`armor`,`min_dmg`,`max_dmg`)
SELECT @entry, `level`,`hp`,`mana`,`str`,`agi`,`sta`,`inte`,`spi`,`armor`,`min_dmg`,`max_dmg`
FROM `pet_levelstats` WHERE `creature_entry` = 1;

-- ----------------------------------------------------------------------------
-- 5. 验证查询（可选）：逐项核对生成结果
-- ----------------------------------------------------------------------------
SELECT
    ct.`entry`,
    ct.`name`,
    ct.`minlevel`,
    ct.`maxlevel`,
    ct.`faction`,
    ct.`family`,
    ct.`type`,
    ct.`type_flags`,
    ct.`PetSpellDataId`,
    (SELECT COUNT(*) FROM `creature_template_model` WHERE `CreatureID` = ct.`entry`)                       AS `模型行数_应1`,
    (SELECT COUNT(*) FROM `creature_model_info`     WHERE `DisplayID`  = @model)                           AS `model_info_应1`,
    (SELECT COUNT(*) FROM `pet_levelstats`          WHERE `creature_entry` = ct.`entry`)                   AS `pet_levelstats行数_应80`,
    CASE WHEN ct.`type` = 1 AND ct.`family` > 0 AND (ct.`type_flags` & 1) = 1
         THEN '可驯服 OK'
         ELSE '不可驯服：请检查 type=1 / family>0 / type_flags 含 1' END                                  AS `驯服校验`
FROM `creature_template` ct
WHERE ct.`entry` = @entry;
