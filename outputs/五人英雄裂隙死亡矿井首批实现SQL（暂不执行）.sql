-- 已废弃：五人英雄裂隙死亡矿井首批实现SQL
--
-- 本文件已被全部副本整合SQL取代，不要执行，也不要与整合SQL同时导入。
-- 当前唯一应审核的文件：
-- outputs/五人英雄裂隙全部副本整合SQL（暂不执行）.sql
--
-- 以下内容仅保留为历史实现记录。
-- 1. Boss Entry 100000-100008按已确认规划生成。
-- 2. 裂隙Boss固定为83级、exp=2，并使用早期80级10人本近战基线DamageModifier=35；Tier伤害倍率仅由C++脚本应用。
-- 3. 为避免等级提升顺带把生命放大约29倍，HealthModifier按20级源模板基础生命反向折算；现有Tier生命倍率仍仅由C++脚本应用。
-- 4. 三个Tier Entry分别绑定同一个Boss脚本，不使用boss_rift_common。
-- 5. 玩家传入点使用按当前静态刷怪分布筛选的建议值；尚未经过地图高度、碰撞、巡逻和仇恨范围实测，导入前必须进服验证。
-- 6. 入口NPC Entry固定为100009，来源模板为8379（模型7591）；世界生成map和坐标尚未确认，因此本文件不写creature生成行。
-- 7. T1/T2/T3分别使用出口门Entry 100500/100501/100502和模型7148/8196/8197；三者共用rift_exit_portal脚本。

SET @RIFT_EXIT_PORTAL_T1_ENTRY := 100500;
SET @RIFT_EXIT_PORTAL_T2_ENTRY := 100501;
SET @RIFT_EXIT_PORTAL_T3_ENTRY := 100502;
SET @RIFT_EXIT_PORTAL_SOURCE_ENTRY := 181229; -- 只复制交互类型、Data字段和尺寸，displayId由Tier显式覆盖

SET @RIFT_ENTRY_NPC_ENTRY := 100009;
SET @RIFT_ENTRY_NPC_SOURCE_ENTRY := 8379; -- Archmage Xylem，当前基线模型7591

-- 建议传入点：只基于当前静态刷怪坐标筛选，导入前必须进服实测高度、碰撞、巡逻与仇恨范围。
SET @VANCLEEF_PLAYER_X := -101.500;
SET @VANCLEEF_PLAYER_Y := -819.900;
SET @VANCLEEF_PLAYER_Z := 39.300;
SET @VANCLEEF_PLAYER_O := 0.000;
SET @SNEED_PLAYER_X := -309.000;
SET @SNEED_PLAYER_Y := -520.000;
SET @SNEED_PLAYER_Z := 49.400;
SET @SNEED_PLAYER_O := 0.343;
SET @COOKIE_PLAYER_X := -67.600;
SET @COOKIE_PLAYER_Y := -838.000;
SET @COOKIE_PLAYER_Z := 17.100;
SET @COOKIE_PLAYER_O := 4.713;

-- Tier倍率可在执行前统一调整；SQL模板本身不乘这些倍率。
SET @RIFT_T1_HEALTH := 1.0000;
SET @RIFT_T1_DAMAGE := 1.0000;
SET @RIFT_T2_HEALTH := 1.8000;
SET @RIFT_T2_DAMAGE := 1.4000;
SET @RIFT_T3_HEALTH := 3.5000;
SET @RIFT_T3_DAMAGE := 2.0000;

-- ============================================================================
-- 1. 两张静态配置表；运行状态不建表
-- ============================================================================
CREATE TABLE IF NOT EXISTS `heroic_dungeon_rift_boss` (
  `boss_id` INT UNSIGNED NOT NULL COMMENT '裂隙Boss配置ID',
  `map_name` VARCHAR(64) NOT NULL COMMENT '副本名称',
  `map_id` SMALLINT UNSIGNED NOT NULL COMMENT '副本地图ID',
  `player_entry_x` FLOAT NOT NULL COMMENT '默认玩家传入点X',
  `player_entry_y` FLOAT NOT NULL COMMENT '默认玩家传入点Y',
  `player_entry_z` FLOAT NOT NULL COMMENT '默认玩家传入点Z',
  `player_entry_o` FLOAT NOT NULL COMMENT '默认玩家传入朝向',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '1=进入随机池，0=禁用',
  `remark` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`boss_id`),
  KEY `idx_rift_boss_enabled_map` (`enabled`, `map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

CREATE TABLE IF NOT EXISTS `heroic_dungeon_rift_boss_tier` (
  `boss_id` INT UNSIGNED NOT NULL,
  `entry_id` INT UNSIGNED NOT NULL COMMENT '各Tier的新Boss Entry',
  `boss_spawn_x` FLOAT NOT NULL,
  `boss_spawn_y` FLOAT NOT NULL,
  `boss_spawn_z` FLOAT NOT NULL,
  `boss_spawn_o` FLOAT NOT NULL,
  `tier` TINYINT UNSIGNED NOT NULL,
  `health_multiplier` DECIMAL(8,4) NOT NULL DEFAULT 1.0000,
  `damage_multiplier` DECIMAL(8,4) NOT NULL DEFAULT 1.0000,
  `player_entry_x` FLOAT NOT NULL,
  `player_entry_y` FLOAT NOT NULL,
  `player_entry_z` FLOAT NOT NULL,
  `player_entry_o` FLOAT NOT NULL,
  PRIMARY KEY (`boss_id`, `tier`),
  UNIQUE KEY `uk_rift_boss_tier_entry` (`entry_id`),
  KEY `idx_rift_boss_tier_tier` (`tier`),
  CONSTRAINT `chk_rift_boss_tier_value` CHECK (`tier` BETWEEN 1 AND 3),
  CONSTRAINT `chk_rift_boss_tier_health` CHECK (`health_multiplier` > 0),
  CONSTRAINT `chk_rift_boss_tier_damage` CHECK (`damage_multiplier` > 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- 2. 九个Boss模板：三个Boss × T1/T2/T3
-- ============================================================================
-- 强度基线：level 83 / exp 2 / DamageModifier 35.0。
-- creature_classlevelstats中战士类level 20 basehp0=484、level 83 basehp2=13945，
-- 因此HealthModifier使用 484 * 源HealthModifier / 13945，保持升级前模板基础生命不变。
-- 固定/周期法术伤害由对应Boss脚本以CastCustomSpell局部覆盖，不修改全服Spell.dbc。
-- 映射：
-- 639 范克里夫 -> 100000/100001/100002 -> boss_rift_vancleef
-- 642 斯尼德的伐木机 -> 100003/100004/100005 -> boss_rift_sneed
-- 645 曲奇 -> 100006/100007/100008 -> boss_rift_cookie

DELETE FROM `creature_template_model` WHERE `CreatureID` BETWEEN 100000 AND 100008;
DELETE FROM `creature_template_spell` WHERE `CreatureID` BETWEEN 100000 AND 100008;
DELETE FROM `creature_template_resistance` WHERE `CreatureID` BETWEEN 100000 AND 100008;
DELETE FROM `creature_template_addon` WHERE `entry` BETWEEN 100000 AND 100008;
DELETE FROM `creature_equip_template` WHERE `CreatureID` BETWEEN 100000 AND 100008;
DELETE FROM `creature_template_movement` WHERE `CreatureId` BETWEEN 100000 AND 100008;
DELETE FROM `creature_template` WHERE `entry` BETWEEN 100000 AND 100008;

INSERT INTO `creature_template` (
  `entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`,
  `KillCredit1`, `KillCredit2`, `name`, `subname`, `IconName`, `gossip_menu_id`,
  `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `speed_walk`, `speed_run`,
  `speed_swim`, `speed_flight`, `detection_range`, `rank`, `dmgschool`,
  `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`,
  `unit_class`, `unit_flags`, `unit_flags2`, `dynamicflags`, `family`, `type`, `type_flags`,
  `lootid`, `pickpocketloot`, `skinloot`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`,
  `AIName`, `MovementType`, `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`,
  `ExperienceModifier`, `RacialLeader`, `movementId`, `RegenHealth`, `CreatureImmunitiesId`,
  `flags_extra`, `ScriptName`, `VerifiedBuild`
)
SELECT
  m.new_entry, 0, 0, 0,
  ct.KillCredit1, ct.KillCredit2, CONCAT(ct.name, ' [Rift T', m.tier, ']'), ct.subname, ct.IconName, 0,
  83, 83, 2, ct.faction, 0, ct.speed_walk, ct.speed_run,
  ct.speed_swim, ct.speed_flight, ct.detection_range, ct.rank, ct.dmgschool,
  35.0000, ct.BaseAttackTime, ct.RangeAttackTime, ct.BaseVariance, ct.RangeVariance,
  ct.unit_class, ct.unit_flags, ct.unit_flags2, ct.dynamicflags, ct.family, ct.type, ct.type_flags,
  0, 0, 0, 0, 0, 0, 0,
  '', 0, ct.HoverHeight, (484.0000 * ct.HealthModifier / 13945.0000), ct.ManaModifier, ct.ArmorModifier,
  ct.ExperienceModifier, ct.RacialLeader, 0, ct.RegenHealth, ct.CreatureImmunitiesId,
  ct.flags_extra, m.script_name, ct.VerifiedBuild
FROM `creature_template` ct
JOIN (
  SELECT 639 source_entry, 100000 new_entry, 1 tier, 'boss_rift_vancleef' script_name
  UNION ALL SELECT 639, 100001, 2, 'boss_rift_vancleef'
  UNION ALL SELECT 639, 100002, 3, 'boss_rift_vancleef'
  UNION ALL SELECT 642, 100003, 1, 'boss_rift_sneed'
  UNION ALL SELECT 642, 100004, 2, 'boss_rift_sneed'
  UNION ALL SELECT 642, 100005, 3, 'boss_rift_sneed'
  UNION ALL SELECT 645, 100006, 1, 'boss_rift_cookie'
  UNION ALL SELECT 645, 100007, 2, 'boss_rift_cookie'
  UNION ALL SELECT 645, 100008, 3, 'boss_rift_cookie'
) m ON m.source_entry = ct.entry;

-- 复制客户端模型。
INSERT INTO `creature_template_model`
  (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
SELECT m.new_entry, src.Idx, src.CreatureDisplayID, src.DisplayScale, src.Probability, src.VerifiedBuild
FROM `creature_template_model` src
JOIN (
  SELECT 639 source_entry, 100000 new_entry UNION ALL SELECT 639,100001 UNION ALL SELECT 639,100002
  UNION ALL SELECT 642,100003 UNION ALL SELECT 642,100004 UNION ALL SELECT 642,100005
  UNION ALL SELECT 645,100006 UNION ALL SELECT 645,100007 UNION ALL SELECT 645,100008
) m ON m.source_entry = src.CreatureID;

-- 复制源Boss法术清单，仅用于模板资源完整性；具体施法时机由对应C++ Boss脚本控制。
INSERT INTO `creature_template_spell` (`CreatureID`, `Index`, `Spell`, `VerifiedBuild`)
SELECT m.new_entry, src.`Index`, src.Spell, src.VerifiedBuild
FROM `creature_template_spell` src
JOIN (
  SELECT 639 source_entry, 100000 new_entry UNION ALL SELECT 639,100001 UNION ALL SELECT 639,100002
  UNION ALL SELECT 642,100003 UNION ALL SELECT 642,100004 UNION ALL SELECT 642,100005
  UNION ALL SELECT 645,100006 UNION ALL SELECT 645,100007 UNION ALL SELECT 645,100008
) m ON m.source_entry = src.CreatureID;

INSERT INTO `creature_template_resistance` (`CreatureID`, `School`, `Resistance`, `VerifiedBuild`)
SELECT m.new_entry, src.School, src.Resistance, src.VerifiedBuild
FROM `creature_template_resistance` src
JOIN (
  SELECT 639 source_entry, 100000 new_entry UNION ALL SELECT 639,100001 UNION ALL SELECT 639,100002
  UNION ALL SELECT 642,100003 UNION ALL SELECT 642,100004 UNION ALL SELECT 642,100005
  UNION ALL SELECT 645,100006 UNION ALL SELECT 645,100007 UNION ALL SELECT 645,100008
) m ON m.source_entry = src.CreatureID;

INSERT INTO `creature_template_addon`
  (`entry`, `path_id`, `mount`, `bytes1`, `bytes2`, `emote`, `visibilityDistanceType`, `auras`)
SELECT m.new_entry, 0, src.mount, src.bytes1, src.bytes2, src.emote, src.visibilityDistanceType, src.auras
FROM `creature_template_addon` src
JOIN (
  SELECT 639 source_entry, 100000 new_entry UNION ALL SELECT 639,100001 UNION ALL SELECT 639,100002
  UNION ALL SELECT 642,100003 UNION ALL SELECT 642,100004 UNION ALL SELECT 642,100005
  UNION ALL SELECT 645,100006 UNION ALL SELECT 645,100007 UNION ALL SELECT 645,100008
) m ON m.source_entry = src.entry;

INSERT INTO `creature_equip_template`
  (`CreatureID`, `ID`, `ItemID1`, `ItemID2`, `ItemID3`, `VerifiedBuild`)
SELECT m.new_entry, src.ID, src.ItemID1, src.ItemID2, src.ItemID3, src.VerifiedBuild
FROM `creature_equip_template` src
JOIN (
  SELECT 639 source_entry, 100000 new_entry UNION ALL SELECT 639,100001 UNION ALL SELECT 639,100002
  UNION ALL SELECT 642,100003 UNION ALL SELECT 642,100004 UNION ALL SELECT 642,100005
  UNION ALL SELECT 645,100006 UNION ALL SELECT 645,100007 UNION ALL SELECT 645,100008
) m ON m.source_entry = src.CreatureID;

INSERT INTO `creature_template_movement`
  (`CreatureId`, `Ground`, `Swim`, `Flight`, `Rooted`, `Chase`, `Random`, `InteractionPauseTimer`)
SELECT m.new_entry, src.Ground, src.Swim, src.Flight, src.Rooted, src.Chase, src.Random, src.InteractionPauseTimer
FROM `creature_template_movement` src
JOIN (
  SELECT 639 source_entry, 100000 new_entry UNION ALL SELECT 639,100001 UNION ALL SELECT 639,100002
  UNION ALL SELECT 642,100003 UNION ALL SELECT 642,100004 UNION ALL SELECT 642,100005
  UNION ALL SELECT 645,100006 UNION ALL SELECT 645,100007 UNION ALL SELECT 645,100008
) m ON m.source_entry = src.CreatureId;

-- ============================================================================
-- 3. 三档出口传送门模板
-- ============================================================================
-- 核心支持GameObject::SetDisplayId在对象入图后重建客户端显示和碰撞模型，但部分交互边界仍取模板displayId。
-- 为确保创建时显示、服务器碰撞和交互边界完全一致，本实现使用三个模板Entry，不做运行时换模。
DELETE FROM `gameobject_template`
WHERE `entry` IN (@RIFT_EXIT_PORTAL_T1_ENTRY, @RIFT_EXIT_PORTAL_T2_ENTRY, @RIFT_EXIT_PORTAL_T3_ENTRY);
INSERT INTO `gameobject_template` (
  `entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `size`,
  `Data0`, `Data1`, `Data2`, `Data3`, `Data4`, `Data5`, `Data6`, `Data7`,
  `Data8`, `Data9`, `Data10`, `Data11`, `Data12`, `Data13`, `Data14`, `Data15`,
  `Data16`, `Data17`, `Data18`, `Data19`, `Data20`, `Data21`, `Data22`, `Data23`,
  `AIName`, `ScriptName`, `VerifiedBuild`
)
SELECT
  tier.entry, src.type, tier.display_id, CONCAT('五人英雄裂隙 T', tier.tier, ' 出口'), '', '', '', src.size,
  src.Data0, src.Data1, src.Data2, src.Data3, src.Data4, src.Data5, src.Data6, src.Data7,
  src.Data8, src.Data9, src.Data10, src.Data11, src.Data12, src.Data13, src.Data14, src.Data15,
  src.Data16, src.Data17, src.Data18, src.Data19, src.Data20, src.Data21, src.Data22, src.Data23,
  '', 'rift_exit_portal', src.VerifiedBuild
FROM `gameobject_template` src
CROSS JOIN (
  SELECT 1 tier, @RIFT_EXIT_PORTAL_T1_ENTRY entry, 7148 display_id
  UNION ALL SELECT 2, @RIFT_EXIT_PORTAL_T2_ENTRY, 8196
  UNION ALL SELECT 3, @RIFT_EXIT_PORTAL_T3_ENTRY, 8197
) tier
WHERE src.entry = @RIFT_EXIT_PORTAL_SOURCE_ENTRY
  AND tier.entry <> 0;

-- 三档出口门都由C++按本场Tier在玩家传入点动态召唤，不插入gameobject世界生成表。

-- ============================================================================
-- 4. 入口NPC模板（Entry 100009；不含世界生成坐标）
-- ============================================================================
DELETE FROM `creature_template_model` WHERE `CreatureID` = @RIFT_ENTRY_NPC_ENTRY;
DELETE FROM `creature_template` WHERE `entry` = @RIFT_ENTRY_NPC_ENTRY;

INSERT INTO `creature_template` (
  `entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`,
  `KillCredit1`, `KillCredit2`, `name`, `subname`, `IconName`, `gossip_menu_id`,
  `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `speed_walk`, `speed_run`,
  `speed_swim`, `speed_flight`, `detection_range`, `rank`, `dmgschool`,
  `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`,
  `unit_class`, `unit_flags`, `unit_flags2`, `dynamicflags`, `family`, `type`, `type_flags`,
  `lootid`, `pickpocketloot`, `skinloot`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`,
  `AIName`, `MovementType`, `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`,
  `ExperienceModifier`, `RacialLeader`, `movementId`, `RegenHealth`, `CreatureImmunitiesId`,
  `flags_extra`, `ScriptName`, `VerifiedBuild`
)
SELECT
  @RIFT_ENTRY_NPC_ENTRY, 0, 0, 0,
  0, 0, '五人英雄裂隙引导者', '选择裂隙难度', src.IconName, 0,
  src.minlevel, src.maxlevel, src.exp, src.faction, 1, src.speed_walk, src.speed_run,
  src.speed_swim, src.speed_flight, src.detection_range, 0, src.dmgschool,
  1, src.BaseAttackTime, src.RangeAttackTime, src.BaseVariance, src.RangeVariance,
  src.unit_class, src.unit_flags, src.unit_flags2, 0, src.family, src.type, src.type_flags,
  0, 0, 0, 0, 0, 0, 0,
  '', 0, src.HoverHeight, 1, 1, 1,
  1, 0, 0, 1, 0,
  src.flags_extra, 'npc_rift_entry', src.VerifiedBuild
FROM `creature_template` src
WHERE src.entry = @RIFT_ENTRY_NPC_SOURCE_ENTRY
  AND @RIFT_ENTRY_NPC_ENTRY <> 0
  AND @RIFT_ENTRY_NPC_SOURCE_ENTRY <> 0;

INSERT INTO `creature_template_model`
  (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
SELECT @RIFT_ENTRY_NPC_ENTRY, src.Idx, src.CreatureDisplayID, src.DisplayScale, src.Probability, src.VerifiedBuild
FROM `creature_template_model` src
WHERE src.CreatureID = @RIFT_ENTRY_NPC_SOURCE_ENTRY
  AND @RIFT_ENTRY_NPC_ENTRY <> 0
  AND @RIFT_ENTRY_NPC_SOURCE_ENTRY <> 0;

-- 入口NPC的creature世界生成行必须在Entry、map和坐标确认后另行添加。

-- ============================================================================
-- 5. 静态Boss配置（当前为建议传入点，全部需要进服实测）
-- ============================================================================
INSERT INTO `heroic_dungeon_rift_boss`
  (`boss_id`, `map_name`, `map_id`, `player_entry_x`, `player_entry_y`, `player_entry_z`, `player_entry_o`, `enabled`, `remark`)
SELECT 1, '死亡矿井', 36, @VANCLEEF_PLAYER_X, @VANCLEEF_PLAYER_Y, @VANCLEEF_PLAYER_Z, @VANCLEEF_PLAYER_O, 1,
       '范克里夫；建议传入点，需实测船体跨层引怪、高度与碰撞'
WHERE @VANCLEEF_PLAYER_X IS NOT NULL AND @VANCLEEF_PLAYER_Y IS NOT NULL
  AND @VANCLEEF_PLAYER_Z IS NOT NULL AND @VANCLEEF_PLAYER_O IS NOT NULL
UNION ALL
SELECT 2, '死亡矿井', 36, @SNEED_PLAYER_X, @SNEED_PLAYER_Y, @SNEED_PLAYER_Z, @SNEED_PLAYER_O, 1,
       '斯尼德的伐木机；建议传入点，需实测周边小怪巡逻、仇恨与碰撞'
WHERE @SNEED_PLAYER_X IS NOT NULL AND @SNEED_PLAYER_Y IS NOT NULL
  AND @SNEED_PLAYER_Z IS NOT NULL AND @SNEED_PLAYER_O IS NOT NULL
UNION ALL
SELECT 3, '死亡矿井', 36, @COOKIE_PLAYER_X, @COOKIE_PLAYER_Y, @COOKIE_PLAYER_Z, @COOKIE_PLAYER_O, 1,
       '曲奇；建议传入点，需实测船舱区域高度、可达性与小怪仇恨'
WHERE @COOKIE_PLAYER_X IS NOT NULL AND @COOKIE_PLAYER_Y IS NOT NULL
  AND @COOKIE_PLAYER_Z IS NOT NULL AND @COOKIE_PLAYER_O IS NOT NULL
ON DUPLICATE KEY UPDATE
  `map_name` = VALUES(`map_name`), `map_id` = VALUES(`map_id`),
  `player_entry_x` = VALUES(`player_entry_x`), `player_entry_y` = VALUES(`player_entry_y`),
  `player_entry_z` = VALUES(`player_entry_z`), `player_entry_o` = VALUES(`player_entry_o`),
  `enabled` = VALUES(`enabled`), `remark` = VALUES(`remark`);

-- Boss生成点来自当前基线data/sql/base/db_world/creature.sql：
-- 范克里夫(639)：-87.369, -819.895, 39.3004, 0
-- 斯尼德的伐木机(642)：-289.453, -513.009, 49.6785, 3.78367
-- 曲奇(645)：-67.5844, -853.749, 17.075, 4.92527
INSERT INTO `heroic_dungeon_rift_boss_tier`
  (`boss_id`, `entry_id`, `boss_spawn_x`, `boss_spawn_y`, `boss_spawn_z`, `boss_spawn_o`,
   `tier`, `health_multiplier`, `damage_multiplier`,
   `player_entry_x`, `player_entry_y`, `player_entry_z`, `player_entry_o`)
SELECT 1, 100000, -87.369, -819.895, 39.3004, 0, 1, @RIFT_T1_HEALTH, @RIFT_T1_DAMAGE,
       @VANCLEEF_PLAYER_X, @VANCLEEF_PLAYER_Y, @VANCLEEF_PLAYER_Z, @VANCLEEF_PLAYER_O
WHERE EXISTS (SELECT 1 FROM `heroic_dungeon_rift_boss` WHERE `boss_id` = 1)
  AND @VANCLEEF_PLAYER_X IS NOT NULL AND @VANCLEEF_PLAYER_Y IS NOT NULL
  AND @VANCLEEF_PLAYER_Z IS NOT NULL AND @VANCLEEF_PLAYER_O IS NOT NULL
UNION ALL SELECT 1,100001,-87.369,-819.895,39.3004,0,2,@RIFT_T2_HEALTH,@RIFT_T2_DAMAGE,
       @VANCLEEF_PLAYER_X,@VANCLEEF_PLAYER_Y,@VANCLEEF_PLAYER_Z,@VANCLEEF_PLAYER_O
WHERE EXISTS (SELECT 1 FROM `heroic_dungeon_rift_boss` WHERE `boss_id` = 1)
  AND @VANCLEEF_PLAYER_X IS NOT NULL AND @VANCLEEF_PLAYER_Y IS NOT NULL
  AND @VANCLEEF_PLAYER_Z IS NOT NULL AND @VANCLEEF_PLAYER_O IS NOT NULL
UNION ALL SELECT 1,100002,-87.369,-819.895,39.3004,0,3,@RIFT_T3_HEALTH,@RIFT_T3_DAMAGE,
       @VANCLEEF_PLAYER_X,@VANCLEEF_PLAYER_Y,@VANCLEEF_PLAYER_Z,@VANCLEEF_PLAYER_O
WHERE EXISTS (SELECT 1 FROM `heroic_dungeon_rift_boss` WHERE `boss_id` = 1)
  AND @VANCLEEF_PLAYER_X IS NOT NULL AND @VANCLEEF_PLAYER_Y IS NOT NULL
  AND @VANCLEEF_PLAYER_Z IS NOT NULL AND @VANCLEEF_PLAYER_O IS NOT NULL
UNION ALL SELECT 2,100003,-289.453,-513.009,49.6785,3.78367,1,@RIFT_T1_HEALTH,@RIFT_T1_DAMAGE,
       @SNEED_PLAYER_X,@SNEED_PLAYER_Y,@SNEED_PLAYER_Z,@SNEED_PLAYER_O
WHERE EXISTS (SELECT 1 FROM `heroic_dungeon_rift_boss` WHERE `boss_id` = 2)
  AND @SNEED_PLAYER_X IS NOT NULL AND @SNEED_PLAYER_Y IS NOT NULL
  AND @SNEED_PLAYER_Z IS NOT NULL AND @SNEED_PLAYER_O IS NOT NULL
UNION ALL SELECT 2,100004,-289.453,-513.009,49.6785,3.78367,2,@RIFT_T2_HEALTH,@RIFT_T2_DAMAGE,
       @SNEED_PLAYER_X,@SNEED_PLAYER_Y,@SNEED_PLAYER_Z,@SNEED_PLAYER_O
WHERE EXISTS (SELECT 1 FROM `heroic_dungeon_rift_boss` WHERE `boss_id` = 2)
  AND @SNEED_PLAYER_X IS NOT NULL AND @SNEED_PLAYER_Y IS NOT NULL
  AND @SNEED_PLAYER_Z IS NOT NULL AND @SNEED_PLAYER_O IS NOT NULL
UNION ALL SELECT 2,100005,-289.453,-513.009,49.6785,3.78367,3,@RIFT_T3_HEALTH,@RIFT_T3_DAMAGE,
       @SNEED_PLAYER_X,@SNEED_PLAYER_Y,@SNEED_PLAYER_Z,@SNEED_PLAYER_O
WHERE EXISTS (SELECT 1 FROM `heroic_dungeon_rift_boss` WHERE `boss_id` = 2)
  AND @SNEED_PLAYER_X IS NOT NULL AND @SNEED_PLAYER_Y IS NOT NULL
  AND @SNEED_PLAYER_Z IS NOT NULL AND @SNEED_PLAYER_O IS NOT NULL
UNION ALL SELECT 3,100006,-67.5844,-853.749,17.075,4.92527,1,@RIFT_T1_HEALTH,@RIFT_T1_DAMAGE,
       @COOKIE_PLAYER_X,@COOKIE_PLAYER_Y,@COOKIE_PLAYER_Z,@COOKIE_PLAYER_O
WHERE EXISTS (SELECT 1 FROM `heroic_dungeon_rift_boss` WHERE `boss_id` = 3)
  AND @COOKIE_PLAYER_X IS NOT NULL AND @COOKIE_PLAYER_Y IS NOT NULL
  AND @COOKIE_PLAYER_Z IS NOT NULL AND @COOKIE_PLAYER_O IS NOT NULL
UNION ALL SELECT 3,100007,-67.5844,-853.749,17.075,4.92527,2,@RIFT_T2_HEALTH,@RIFT_T2_DAMAGE,
       @COOKIE_PLAYER_X,@COOKIE_PLAYER_Y,@COOKIE_PLAYER_Z,@COOKIE_PLAYER_O
WHERE EXISTS (SELECT 1 FROM `heroic_dungeon_rift_boss` WHERE `boss_id` = 3)
  AND @COOKIE_PLAYER_X IS NOT NULL AND @COOKIE_PLAYER_Y IS NOT NULL
  AND @COOKIE_PLAYER_Z IS NOT NULL AND @COOKIE_PLAYER_O IS NOT NULL
UNION ALL SELECT 3,100008,-67.5844,-853.749,17.075,4.92527,3,@RIFT_T3_HEALTH,@RIFT_T3_DAMAGE,
       @COOKIE_PLAYER_X,@COOKIE_PLAYER_Y,@COOKIE_PLAYER_Z,@COOKIE_PLAYER_O
WHERE EXISTS (SELECT 1 FROM `heroic_dungeon_rift_boss` WHERE `boss_id` = 3)
  AND @COOKIE_PLAYER_X IS NOT NULL AND @COOKIE_PLAYER_Y IS NOT NULL
  AND @COOKIE_PLAYER_Z IS NOT NULL AND @COOKIE_PLAYER_O IS NOT NULL
ON DUPLICATE KEY UPDATE
  `entry_id` = VALUES(`entry_id`),
  `boss_spawn_x` = VALUES(`boss_spawn_x`), `boss_spawn_y` = VALUES(`boss_spawn_y`),
  `boss_spawn_z` = VALUES(`boss_spawn_z`), `boss_spawn_o` = VALUES(`boss_spawn_o`),
  `health_multiplier` = VALUES(`health_multiplier`), `damage_multiplier` = VALUES(`damage_multiplier`),
  `player_entry_x` = VALUES(`player_entry_x`), `player_entry_y` = VALUES(`player_entry_y`),
  `player_entry_z` = VALUES(`player_entry_z`), `player_entry_o` = VALUES(`player_entry_o`);

-- ============================================================================
-- 6. 审核查询
-- ============================================================================
SELECT `entry`, `name`, `AIName`, `ScriptName`, `HealthModifier`, `DamageModifier`
FROM `creature_template` WHERE `entry` BETWEEN 100000 AND 100008 ORDER BY `entry`;
SELECT `CreatureID`, `CreatureDisplayID` FROM `creature_template_model`
WHERE `CreatureID` BETWEEN 100000 AND 100008 ORDER BY `CreatureID`, `Idx`;
SELECT `entry`, `type`, `displayId`, `name`, `ScriptName`
FROM `gameobject_template`
WHERE `entry` IN (@RIFT_EXIT_PORTAL_T1_ENTRY, @RIFT_EXIT_PORTAL_T2_ENTRY, @RIFT_EXIT_PORTAL_T3_ENTRY)
ORDER BY `entry`;
SELECT `entry`, `name`, `subname`, `npcflag`, `ScriptName`
FROM `creature_template` WHERE `entry` = @RIFT_ENTRY_NPC_ENTRY;
SELECT `CreatureID`, `CreatureDisplayID` FROM `creature_template_model`
WHERE `CreatureID` = @RIFT_ENTRY_NPC_ENTRY ORDER BY `Idx`;
SELECT * FROM `heroic_dungeon_rift_boss` ORDER BY `boss_id`;
SELECT * FROM `heroic_dungeon_rift_boss_tier` ORDER BY `boss_id`, `tier`;
