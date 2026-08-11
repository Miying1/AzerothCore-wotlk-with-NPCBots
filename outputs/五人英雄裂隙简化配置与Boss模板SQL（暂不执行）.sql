-- 五人英雄裂隙简化配置与Boss模板SQL（暂不执行）
-- 适用：AzerothCore WotLK 3.3.5a + NPCBots
--
-- 本文件只供审核和后续生成使用，当前不执行、不导入数据库。
-- 设计约束：
-- 1. 数据库只保存两张静态Boss配置表；实例运行状态全部保存在服务器内存。
-- 2. T1/T2/T3的新Entry共用同一个C++脚本：boss_rift_common。
-- 3. 统一出口门只使用一个GameObject Entry：100500。
-- 4. 出口门为实例内动态GameObject，返回点不写入数据库。
-- 5. 新Boss Entry从100000开始分配；本文件不修改Spell.dbc。

-- ============================================================================
-- 配置表1：Boss基础配置
-- 每行一个可随机选择的Boss。
-- ============================================================================
CREATE TABLE IF NOT EXISTS `heroic_dungeon_rift_boss` (
  `boss_id` INT UNSIGNED NOT NULL COMMENT '自定义Boss配置ID，也是运行态Boss分派键',
  `map_name` VARCHAR(64) NOT NULL COMMENT '副本名称',
  `map_id` SMALLINT UNSIGNED NOT NULL COMMENT 'AzerothCore副本地图ID',
  `player_entry_x` FLOAT NOT NULL DEFAULT 0 COMMENT '玩家唯一传入点X',
  `player_entry_y` FLOAT NOT NULL DEFAULT 0 COMMENT '玩家唯一传入点Y',
  `player_entry_z` FLOAT NOT NULL DEFAULT 0 COMMENT '玩家唯一传入点Z',
  `player_entry_o` FLOAT NOT NULL DEFAULT 0 COMMENT '玩家传入朝向',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '1=进入随机池，0=禁用',
  `remark` VARCHAR(255) DEFAULT NULL COMMENT '坐标来源和维护备注',
  PRIMARY KEY (`boss_id`),
  KEY `idx_rift_boss_enabled_map` (`enabled`, `map_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- 配置表2：Boss的Tier、Entry、生成点和强度
-- 每个boss_id通常配置tier=1、2、3三行。
-- 玩家传入点在此表保存最终生效值，允许某一Tier覆盖表1默认值。
-- ============================================================================
CREATE TABLE IF NOT EXISTS `heroic_dungeon_rift_boss_tier` (
  `boss_id` INT UNSIGNED NOT NULL COMMENT '关联heroic_dungeon_rift_boss.boss_id',
  `entry_id` INT UNSIGNED NOT NULL COMMENT '新creature_template.entry，从100000开始',
  `boss_spawn_x` FLOAT NOT NULL DEFAULT 0 COMMENT 'Boss生成点X，采用现有副本Boss坐标',
  `boss_spawn_y` FLOAT NOT NULL DEFAULT 0 COMMENT 'Boss生成点Y',
  `boss_spawn_z` FLOAT NOT NULL DEFAULT 0 COMMENT 'Boss生成点Z',
  `boss_spawn_o` FLOAT NOT NULL DEFAULT 0 COMMENT 'Boss生成朝向',
  `tier` TINYINT UNSIGNED NOT NULL COMMENT '1=T1，2=T2，3=T3',
  `health_multiplier` DECIMAL(8,4) NOT NULL DEFAULT 1.0000 COMMENT '血量倍率',
  `damage_multiplier` DECIMAL(8,4) NOT NULL DEFAULT 1.0000 COMMENT '伤害倍率',
  `player_entry_x` FLOAT NOT NULL DEFAULT 0 COMMENT '本Tier最终玩家传入点X',
  `player_entry_y` FLOAT NOT NULL DEFAULT 0 COMMENT '本Tier最终玩家传入点Y',
  `player_entry_z` FLOAT NOT NULL DEFAULT 0 COMMENT '本Tier最终玩家传入点Z',
  `player_entry_o` FLOAT NOT NULL DEFAULT 0 COMMENT '本Tier最终玩家传入朝向',
  PRIMARY KEY (`boss_id`, `tier`),
  UNIQUE KEY `uk_rift_boss_tier_entry` (`entry_id`),
  KEY `idx_rift_boss_tier_enabled` (`tier`),
  CONSTRAINT `chk_rift_boss_tier_value` CHECK (`tier` BETWEEN 1 AND 3),
  CONSTRAINT `chk_rift_boss_tier_health` CHECK (`health_multiplier` > 0),
  CONSTRAINT `chk_rift_boss_tier_damage` CHECK (`damage_multiplier` > 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- 运行时设计（不建表）
-- ============================================================================
-- 以下数据只存在服务器内存的HeroicDungeonRiftRunComponent：
-- runToken、boss_id、tier、entry_id、map_id、instance_id、bossGuid、exitPortalGuid、
-- groupGuid、leaderGuid、initiatorGuid、Boss状态、成员进入/离开状态，
-- 以及仅由入口对话者进入前位置生成的一份sharedReturnLocation。
-- 不记录其他队员各自的原位置；所有通过出口门离开的玩家都返回同一sharedReturnLocation。
-- 服务器重启不恢复半场运行；实例结束、超时或清理时释放整个上下文。

-- ============================================================================
-- 统一出口传送门（只交付配置说明，不执行）
-- ============================================================================
-- 固定 gameobject_entry：100500
-- gameobject_template 的 ScriptName：rift_exit_portal
-- 该Entry在所有副本、所有Tier共用。
-- 门由服务器在本场玩家传入点附近动态生成，不写入gameobject表。
-- GameObjectScript只提供固定选项“离开...”，读取本场内存中的唯一sharedReturnLocation。
-- 所有通过门离开的真实玩家都返回入口对话者进入前的位置。
-- 不在gameobject_template或gameobject表保存具体返回坐标。
/*
INSERT INTO `gameobject_template`
  (`entry`, `type`, `displayId`, `name`, `size`, `ScriptName`)
VALUES
  (100500, <已确认的传送门类型>, <已确认的客户端displayId>, '五人英雄裂隙出口', 1.0, 'rift_exit_portal');
*/

-- ============================================================================
-- 示例配置（注释状态；等待用户提供实际Boss传入点后填写）
-- ============================================================================
/*
INSERT INTO `heroic_dungeon_rift_boss`
  (`boss_id`, `map_name`, `map_id`,
   `player_entry_x`, `player_entry_y`, `player_entry_z`, `player_entry_o`,
   `enabled`, `remark`)
VALUES
  (1, '死亡矿井', 36, -14.58, -385.25, 61.91, 0.78, 1, '等待确认玩家传入点');

INSERT INTO `heroic_dungeon_rift_boss_tier`
  (`boss_id`, `entry_id`,
   `boss_spawn_x`, `boss_spawn_y`, `boss_spawn_z`, `boss_spawn_o`,
   `tier`, `health_multiplier`, `damage_multiplier`,
   `player_entry_x`, `player_entry_y`, `player_entry_z`, `player_entry_o`)
VALUES
  (1, 100000,  -11.20, -329.50, 60.65, 0.00, 1, 1.0000, 1.0000, -14.58, -385.25, 61.91, 0.78),
  (1, 100001,  -11.20, -329.50, 60.65, 0.00, 2, 1.8000, 1.4000, -14.58, -385.25, 61.91, 0.78),
  (1, 100002,  -11.20, -329.50, 60.65, 0.00, 3, 3.5000, 2.0000, -14.58, -385.25, 61.91, 0.78);
*/

-- ============================================================================
-- 新生物模板生成模板（注释状态；只生成、不执行）
-- ============================================================================
-- 每个源Boss需要运营者填写source_entry，并为T1/T2/T3分别生成Entry。
-- 三档Entry全部设置同一个ScriptName：boss_rift_common。
-- 不把T1/T2/T3写入difficulty_entry_1/2/3。
/*
SET @source_entry := 639;
SET @new_entry := 100000;
SET @tier := 1;
SET @health_multiplier := 1.0000;
SET @damage_multiplier := 1.0000;

INSERT INTO `creature_template` (
  `entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`,
  `KillCredit1`, `KillCredit2`, `name`, `subname`, `IconName`, `gossip_menu_id`,
  `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `speed_walk`, `speed_run`,
  `speed_swim`, `speed_flight`, `detection_range`, `rank`, `dmgschool`,
  `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`,
  `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`, `dynamicflags`,
  `family`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`,
  `mingold`, `maxgold`, `AIName`, `MovementType`, `HoverHeight`,
  `HealthModifier`, `ManaModifier`, `ArmorModifier`, `ExperienceModifier`,
  `RacialLeader`, `movementId`, `RegenHealth`, `CreatureImmunitiesId`,
  `flags_extra`, `ScriptName`, `VerifiedBuild`
)
SELECT
  @new_entry, 0, 0, 0,
  `KillCredit1`, `KillCredit2`, CONCAT(`name`, ' [Rift T', @tier, ']'), `subname`, `IconName`, 0,
  `minlevel`, `maxlevel`, `exp`, `faction`, 0, `speed_walk`, `speed_run`,
  `speed_swim`, `speed_flight`, `detection_range`, `rank`, `dmgschool`,
  `DamageModifier` * @damage_multiplier, `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`,
  `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`, `dynamicflags`,
  `family`, `type`, `type_flags`, @new_entry, 0, `skinloot`,
  0, 0, '', 0, `HoverHeight`,
  `HealthModifier` * @health_multiplier, `ManaModifier`, `ArmorModifier`, `ExperienceModifier`,
  `RacialLeader`, 0, `RegenHealth`, `CreatureImmunitiesId`,
  `flags_extra`, 'boss_rift_common', `VerifiedBuild`
FROM `creature_template`
WHERE `entry` = @source_entry
  AND NOT EXISTS (SELECT 1 FROM `creature_template` WHERE `entry` = @new_entry);

INSERT INTO `creature_template_model`
  (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
SELECT @new_entry, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`
FROM `creature_template_model`
WHERE `CreatureID` = @source_entry;

INSERT INTO `creature_template_spell`
  (`CreatureID`, `Index`, `Spell`, `VerifiedBuild`)
SELECT @new_entry, `Index`, `Spell`, `VerifiedBuild`
FROM `creature_template_spell`
WHERE `CreatureID` = @source_entry;

INSERT INTO `creature_template_resistance`
  (`CreatureID`, `School`, `Resistance`, `VerifiedBuild`)
SELECT @new_entry, `School`, `Resistance`, `VerifiedBuild`
FROM `creature_template_resistance`
WHERE `CreatureID` = @source_entry;

INSERT INTO `creature_template_addon`
  (`entry`, `path_id`, `mount`, `bytes1`, `bytes2`, `emote`, `visibilityDistanceType`, `auras`)
SELECT @new_entry, 0, `mount`, `bytes1`, `bytes2`, `emote`, `visibilityDistanceType`, NULL
FROM `creature_template_addon`
WHERE `entry` = @source_entry;
*/

-- T2/T3重复上述生成块，替换@new_entry、@tier、@health_multiplier和@damage_multiplier。
-- 正式执行前必须为每个新Entry检查：entry未占用、源Entry存在、附属表无重复行、
-- boss_rift_common已在C++中注册、对应boss_id+tier配置存在。
