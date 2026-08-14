-- 60级五人英雄裂隙SQL（暂不执行）
-- 适用：AzerothCore WotLK 3.3.5a + NPCBots
--
-- 重要：本文件仅用于安装60级经典旧世裂隙内容；当前不要执行。
-- 数据范围（16个首批Boss + 31个经典扩展Boss，共47个60级Boss）：
--   首批：死亡矿井、诺莫瑞根、剃刀高地、剃刀沼泽、血色修道院、黑暗深渊、奥达曼、沉没的神庙；
--   经典扩展：哀嚎洞穴、影牙城堡、祖尔法拉克、玛拉顿、黑石深渊、黑石塔下层、厄运之槌、通灵学院、斯坦索姆。
-- 经典Boss传入坐标来自 outputs/boss传入点.xls。
-- 入口NPC Entry：100000；60级Boss模板连续使用100001-100141。
-- 60级裂隙专用召唤物连续使用102000-102033。
-- T1/T2/T3出口门：100500-100502，displayId：7148/8196/8197。
-- 所有Boss和召唤物均使用新Entry与裂隙C++脚本，不修改原版Boss、原版召唤物、SmartAI或Spell.dbc。

SET @RIFT_ENTRY_NPC_ENTRY := 100000;
SET @RIFT_ENTRY_NPC_SOURCE_ENTRY := 8379;
SET @RIFT_BOSS_ID_BASE := 1;
SET @RIFT_BOSS_ENTRY_BASE := 100001;
SET @RIFT_SUMMON_ENTRY_BASE := 102000;
SET @RIFT_EXIT_PORTAL_ENTRY_BASE := 100500;
SET @RIFT_EXIT_PORTAL_SOURCE_ENTRY := 181229;

SET @RIFT_T1_HEALTH := 1.0000;
SET @RIFT_T1_DAMAGE := 1.0000;
SET @RIFT_T2_HEALTH := 1.5000;
SET @RIFT_T2_DAMAGE := 1.4000;
SET @RIFT_T3_HEALTH := 3.0000;
SET @RIFT_T3_DAMAGE := 2.0000;

-- 主Boss 血量按职业分档（HealthModifier 固定值，血量=basehp2(83)×HealthModifier）：
--   战士/圣骑士（unit_class 1/2）：HealthModifier=129 → 血量≈180万；
--   法系（unit_class 8）       ：HealthModifier=115 → 血量≈128万（47个源Boss中仅秘法师杜安为 class 8）。
SET @RIFT_BOSS_HM_MELEE := 129;
SET @RIFT_BOSS_HM_CASTER := 115;

-- creature_template基础近战伤害系数；与运行时Tier伤害倍率分开配置。
SET @RIFT_BOSS_DAMAGE_MODIFIER := 35.0000;
SET @RIFT_SUMMON_DAMAGE_MODIFIER := 12.0000;

-- 主Boss只保留与可战斗状态无关的源 unit_flags；清除所有不可攻击、免疫、假死和不可选中标志。
SET @RIFT_BOSS_UNIT_FLAGS_KEEP_MASK := 2113862781;
-- 动态召唤物保留源模板常规状态，但同样清除不可攻击、免疫玩家、免疫NPC和不可选中标志。
SET @RIFT_SUMMON_UNIT_FLAGS_KEEP_MASK := 4261412093;

-- ============================================================================
-- 1. 静态配置表；运行状态仅保存在服务器内存
-- ============================================================================
CREATE TABLE IF NOT EXISTS `heroic_dungeon_rift_boss` (
  `boss_id` INT UNSIGNED NOT NULL,
  `map_name` VARCHAR(64) NOT NULL,
  `map_id` SMALLINT UNSIGNED NOT NULL,
  `dungeon_version` TINYINT UNSIGNED NOT NULL DEFAULT 60,
  `player_entry_x` FLOAT DEFAULT NULL,
  `player_entry_y` FLOAT DEFAULT NULL,
  `player_entry_z` FLOAT DEFAULT NULL,
  `player_entry_o` FLOAT DEFAULT NULL,
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1,
  `remark` VARCHAR(255) DEFAULT NULL,
  PRIMARY KEY (`boss_id`),
  KEY `idx_rift_boss_enabled_map` (`enabled`, `map_id`),
  CONSTRAINT `chk_rift_boss_dungeon_version` CHECK (`dungeon_version` IN (60, 70))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 兼容此前已经创建的旧表：已有经典数据默认标记为60级，基础玩家传入点改为可空。
-- MySQL 8.0不支持 ADD COLUMN IF NOT EXISTS，改为根据 information_schema 条件执行迁移。
SET @RIFT_HAS_DUNGEON_VERSION := (
  SELECT COUNT(*)
  FROM `information_schema`.`COLUMNS`
  WHERE `TABLE_SCHEMA` = DATABASE()
    AND `TABLE_NAME` = 'heroic_dungeon_rift_boss'
    AND `COLUMN_NAME` = 'dungeon_version'
);
SET @RIFT_ADD_DUNGEON_VERSION_SQL := IF(
  @RIFT_HAS_DUNGEON_VERSION = 0,
  'ALTER TABLE `heroic_dungeon_rift_boss` ADD COLUMN `dungeon_version` TINYINT UNSIGNED NOT NULL DEFAULT 60 AFTER `map_id`',
  'SELECT 1'
);
PREPARE `rift_add_dungeon_version_stmt` FROM @RIFT_ADD_DUNGEON_VERSION_SQL;
EXECUTE `rift_add_dungeon_version_stmt`;
DEALLOCATE PREPARE `rift_add_dungeon_version_stmt`;
ALTER TABLE `heroic_dungeon_rift_boss`
  MODIFY COLUMN `player_entry_x` FLOAT DEFAULT NULL,
  MODIFY COLUMN `player_entry_y` FLOAT DEFAULT NULL,
  MODIFY COLUMN `player_entry_z` FLOAT DEFAULT NULL,
  MODIFY COLUMN `player_entry_o` FLOAT DEFAULT NULL;

CREATE TABLE IF NOT EXISTS `heroic_dungeon_rift_boss_tier` (
  `boss_id` INT UNSIGNED NOT NULL,
  `entry_id` INT UNSIGNED NOT NULL,
  `boss_spawn_x` FLOAT NOT NULL,
  `boss_spawn_y` FLOAT NOT NULL,
  `boss_spawn_z` FLOAT NOT NULL,
  `boss_spawn_o` FLOAT NOT NULL,
  `tier` TINYINT UNSIGNED NOT NULL,
  `health_multiplier` DECIMAL(8,4) NOT NULL DEFAULT 1.0000,
  `damage_multiplier` DECIMAL(8,4) NOT NULL DEFAULT 1.0000,
  `player_entry_x` FLOAT DEFAULT NULL,
  `player_entry_y` FLOAT DEFAULT NULL,
  `player_entry_z` FLOAT DEFAULT NULL,
  `player_entry_o` FLOAT DEFAULT NULL,
  PRIMARY KEY (`boss_id`, `tier`),
  UNIQUE KEY `uk_rift_boss_tier_entry` (`entry_id`),
  KEY `idx_rift_boss_tier_tier` (`tier`),
  CONSTRAINT `chk_rift_boss_tier_value` CHECK (`tier` BETWEEN 1 AND 3),
  CONSTRAINT `chk_rift_boss_tier_health` CHECK (`health_multiplier` > 0),
  CONSTRAINT `chk_rift_boss_tier_damage` CHECK (`damage_multiplier` > 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- ============================================================================
-- 2. 本次完整Boss映射
-- ============================================================================
DROP TEMPORARY TABLE IF EXISTS `_rift_boss_map`;
CREATE TEMPORARY TABLE `_rift_boss_map` (
  `boss_id` INT UNSIGNED NOT NULL,
  `source_entry` INT UNSIGNED NOT NULL,
  `new_entry` INT UNSIGNED NOT NULL,
  `tier` TINYINT UNSIGNED NOT NULL,
  `script_name` VARCHAR(64) NOT NULL,
  `map_name` VARCHAR(64) NOT NULL,
  `map_id` SMALLINT UNSIGNED NOT NULL,
  `boss_x` FLOAT NOT NULL, `boss_y` FLOAT NOT NULL, `boss_z` FLOAT NOT NULL, `boss_o` FLOAT NOT NULL,
  `player_x` FLOAT DEFAULT NULL, `player_y` FLOAT DEFAULT NULL, `player_z` FLOAT DEFAULT NULL, `player_o` FLOAT DEFAULT NULL,
  `remark` VARCHAR(255) NOT NULL,
  PRIMARY KEY (`boss_id`, `tier`),
  UNIQUE KEY (`new_entry`)
);

INSERT INTO `_rift_boss_map` VALUES
(@RIFT_BOSS_ID_BASE + 0,639,@RIFT_BOSS_ENTRY_BASE + 0,1,'boss_rift_vancleef','死亡矿井',36,-87.369,-819.895,39.3004,0,6.5,-761.146,9.63,3.9,'艾德温·范克里夫；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 0,639,@RIFT_BOSS_ENTRY_BASE + 1,2,'boss_rift_vancleef','死亡矿井',36,-87.369,-819.895,39.3004,0,6.5,-761.146,9.63,3.9,'艾德温·范克里夫；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 0,639,@RIFT_BOSS_ENTRY_BASE + 2,3,'boss_rift_vancleef','死亡矿井',36,-87.369,-819.895,39.3004,0,6.5,-761.146,9.63,3.9,'艾德温·范克里夫；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 1,642,@RIFT_BOSS_ENTRY_BASE + 3,1,'boss_rift_sneed','死亡矿井',36,-289.453,-513.009,49.6785,3.78367,-309,-520,49.4,0.343,'斯尼德的伐木机；沿用建议传入点'),
(@RIFT_BOSS_ID_BASE + 1,642,@RIFT_BOSS_ENTRY_BASE + 4,2,'boss_rift_sneed','死亡矿井',36,-289.453,-513.009,49.6785,3.78367,-309,-520,49.4,0.343,'斯尼德的伐木机；沿用建议传入点'),
(@RIFT_BOSS_ID_BASE + 1,642,@RIFT_BOSS_ENTRY_BASE + 5,3,'boss_rift_sneed','死亡矿井',36,-289.453,-513.009,49.6785,3.78367,-309,-520,49.4,0.343,'斯尼德的伐木机；沿用建议传入点'),
(@RIFT_BOSS_ID_BASE + 2,645,@RIFT_BOSS_ENTRY_BASE + 6,1,'boss_rift_cookie','死亡矿井',36,-67.5844,-853.749,17.075,4.92527,-67.6,-838,17.1,4.713,'曲奇；沿用建议传入点'),
(@RIFT_BOSS_ID_BASE + 2,645,@RIFT_BOSS_ENTRY_BASE + 7,2,'boss_rift_cookie','死亡矿井',36,-67.5844,-853.749,17.075,4.92527,-67.6,-838,17.1,4.713,'曲奇；沿用建议传入点'),
(@RIFT_BOSS_ID_BASE + 2,645,@RIFT_BOSS_ENTRY_BASE + 8,3,'boss_rift_cookie','死亡矿井',36,-67.5844,-853.749,17.075,4.92527,-67.6,-838,17.1,4.713,'曲奇；沿用建议传入点'),
(@RIFT_BOSS_ID_BASE + 3,6235,@RIFT_BOSS_ENTRY_BASE + 9,1,'boss_rift_electrocutioner','诺莫瑞根',90,-552.048,502.902,-216.727,5.75959,-530.513,598.183,-230.601,4.56,'电刑器6000型；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 3,6235,@RIFT_BOSS_ENTRY_BASE + 10,2,'boss_rift_electrocutioner','诺莫瑞根',90,-552.048,502.902,-216.727,5.75959,-530.513,598.183,-230.601,4.56,'电刑器6000型；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 3,6235,@RIFT_BOSS_ENTRY_BASE + 11,3,'boss_rift_electrocutioner','诺莫瑞根',90,-552.048,502.902,-216.727,5.75959,-530.513,598.183,-230.601,4.56,'电刑器6000型；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 4,7800,@RIFT_BOSS_ENTRY_BASE + 12,1,'boss_rift_thermaplugg','诺莫瑞根',90,-531.324,670.159,-325.185,2.9147,-642.258,708.885,-327.059,5.8,'机械师瑟玛普拉格；裂隙专用步行炸弹'),
(@RIFT_BOSS_ID_BASE + 4,7800,@RIFT_BOSS_ENTRY_BASE + 13,2,'boss_rift_thermaplugg','诺莫瑞根',90,-531.324,670.159,-325.185,2.9147,-642.258,708.885,-327.059,5.8,'机械师瑟玛普拉格；裂隙专用步行炸弹'),
(@RIFT_BOSS_ID_BASE + 4,7800,@RIFT_BOSS_ENTRY_BASE + 14,3,'boss_rift_thermaplugg','诺莫瑞根',90,-531.324,670.159,-325.185,2.9147,-642.258,708.885,-327.059,5.8,'机械师瑟玛普拉格；裂隙专用步行炸弹'),
(@RIFT_BOSS_ID_BASE + 5,7358,@RIFT_BOSS_ENTRY_BASE + 15,1,'boss_rift_amnennar','剃刀高地',129,2403.37,960.93,55.1437,2.33874,2428.069,1011.859,50.341,3.96,'寒冰之王亚门纳尔；裂隙专用冰霜亡魂'),
(@RIFT_BOSS_ID_BASE + 5,7358,@RIFT_BOSS_ENTRY_BASE + 16,2,'boss_rift_amnennar','剃刀高地',129,2403.37,960.93,55.1437,2.33874,2428.069,1011.859,50.341,3.96,'寒冰之王亚门纳尔；裂隙专用冰霜亡魂'),
(@RIFT_BOSS_ID_BASE + 5,7358,@RIFT_BOSS_ENTRY_BASE + 17,3,'boss_rift_amnennar','剃刀高地',129,2403.37,960.93,55.1437,2.33874,2428.069,1011.859,50.341,3.96,'寒冰之王亚门纳尔；裂隙专用冰霜亡魂'),
(@RIFT_BOSS_ID_BASE + 6,8567,@RIFT_BOSS_ENTRY_BASE + 18,1,'boss_rift_glutton','剃刀高地',129,2468.71,1006.83,23.7573,6.11095,2451.85,860.159,37.885,1.19,'暴食者；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 6,8567,@RIFT_BOSS_ENTRY_BASE + 19,2,'boss_rift_glutton','剃刀高地',129,2468.71,1006.83,23.7573,6.11095,2451.85,860.159,37.885,1.19,'暴食者；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 6,8567,@RIFT_BOSS_ENTRY_BASE + 20,3,'boss_rift_glutton','剃刀高地',129,2468.71,1006.83,23.7573,6.11095,2451.85,860.159,37.885,1.19,'暴食者；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 7,4420,@RIFT_BOSS_ENTRY_BASE + 21,1,'boss_rift_ramtusk','剃刀沼泽',47,2203.13,1640.05,85.9022,4.7822,2194.96,1601.139,79.832,1.05,'主宰拉姆塔斯；不拉原版小怪'),
(@RIFT_BOSS_ID_BASE + 7,4420,@RIFT_BOSS_ENTRY_BASE + 22,2,'boss_rift_ramtusk','剃刀沼泽',47,2203.13,1640.05,85.9022,4.7822,2194.96,1601.139,79.832,1.05,'主宰拉姆塔斯；不拉原版小怪'),
(@RIFT_BOSS_ID_BASE + 7,4420,@RIFT_BOSS_ENTRY_BASE + 23,3,'boss_rift_ramtusk','剃刀沼泽',47,2203.13,1640.05,85.9022,4.7822,2194.96,1601.139,79.832,1.05,'主宰拉姆塔斯；不拉原版小怪'),
(@RIFT_BOSS_ID_BASE + 8,4421,@RIFT_BOSS_ENTRY_BASE + 24,1,'boss_rift_charlga','剃刀沼泽',47,2190.4,1864.29,79.1389,1.37881,2188.083,1912.97,70.221,5.04,'卡尔加·刺肋；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 8,4421,@RIFT_BOSS_ENTRY_BASE + 25,2,'boss_rift_charlga','剃刀沼泽',47,2190.4,1864.29,79.1389,1.37881,2188.083,1912.97,70.221,5.04,'卡尔加·刺肋；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 8,4421,@RIFT_BOSS_ENTRY_BASE + 26,3,'boss_rift_charlga','剃刀沼泽',47,2190.4,1864.29,79.1389,1.37881,2188.083,1912.97,70.221,5.04,'卡尔加·刺肋；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 9,3976,@RIFT_BOSS_ENTRY_BASE + 27,1,'boss_rift_mograine','血色大教堂',189,1153.87,1398.39,32.6112,6.12611,1054.209,1399.082,27.299,0,'莫格莱尼/怀特迈恩双Boss遭遇'),
(@RIFT_BOSS_ID_BASE + 9,3976,@RIFT_BOSS_ENTRY_BASE + 28,2,'boss_rift_mograine','血色大教堂',189,1153.87,1398.39,32.6112,6.12611,1054.209,1399.082,27.299,0,'莫格莱尼/怀特迈恩双Boss遭遇'),
(@RIFT_BOSS_ID_BASE + 9,3976,@RIFT_BOSS_ENTRY_BASE + 29,3,'boss_rift_mograine','血色大教堂',189,1153.87,1398.39,32.6112,6.12611,1054.209,1399.082,27.299,0,'莫格莱尼/怀特迈恩双Boss遭遇'),
(@RIFT_BOSS_ID_BASE + 10,3975,@RIFT_BOSS_ENTRY_BASE + 30,1,'boss_rift_herod','血色军械库',189,1965.09,-431.607,6.26076,2.98451,1925.698,-408.605,18.007,4.68,'赫洛德；裂隙专用血色预备兵'),
(@RIFT_BOSS_ID_BASE + 10,3975,@RIFT_BOSS_ENTRY_BASE + 31,2,'boss_rift_herod','血色军械库',189,1965.09,-431.607,6.26076,2.98451,1925.698,-408.605,18.007,4.68,'赫洛德；裂隙专用血色预备兵'),
(@RIFT_BOSS_ID_BASE + 10,3975,@RIFT_BOSS_ENTRY_BASE + 32,3,'boss_rift_herod','血色军械库',189,1965.09,-431.607,6.26076,2.98451,1925.698,-408.605,18.007,4.68,'赫洛德；裂隙专用血色预备兵'),
(@RIFT_BOSS_ID_BASE + 11,4832,@RIFT_BOSS_ENTRY_BASE + 33,1,'boss_rift_kelris','黑暗深渊',48,-818.832,-155.576,-25.7923,4.74729,-814.908,-90.937,-25.766,4.65,'暮光领主克尔里斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 11,4832,@RIFT_BOSS_ENTRY_BASE + 34,2,'boss_rift_kelris','黑暗深渊',48,-818.832,-155.576,-25.7923,4.74729,-814.908,-90.937,-25.766,4.65,'暮光领主克尔里斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 11,4832,@RIFT_BOSS_ENTRY_BASE + 35,3,'boss_rift_kelris','黑暗深渊',48,-818.832,-155.576,-25.7923,4.74729,-814.908,-90.937,-25.766,4.65,'暮光领主克尔里斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 12,4829,@RIFT_BOSS_ENTRY_BASE + 36,1,'boss_rift_akumai','黑暗深渊',48,-848.446,-453.865,-33.8922,1.5708,-812.852,-219.654,-25.875,4.31,'阿库麦尔；专用小怪不干扰四火计数'),
(@RIFT_BOSS_ID_BASE + 12,4829,@RIFT_BOSS_ENTRY_BASE + 37,2,'boss_rift_akumai','黑暗深渊',48,-848.446,-453.865,-33.8922,1.5708,-812.852,-219.654,-25.875,4.31,'阿库麦尔；专用小怪不干扰四火计数'),
(@RIFT_BOSS_ID_BASE + 12,4829,@RIFT_BOSS_ENTRY_BASE + 38,3,'boss_rift_akumai','黑暗深渊',48,-848.446,-453.865,-33.8922,1.5708,-812.852,-219.654,-25.875,4.31,'阿库麦尔；专用小怪不干扰四火计数'),
(@RIFT_BOSS_ID_BASE + 13,2748,@RIFT_BOSS_ENTRY_BASE + 39,1,'boss_rift_archaedas','奥达曼',70,104.338,272.31,-51.6959,3.64774,75.78,213.714,-54.974,1.96,'阿扎达斯；裂隙专用四类石像守卫'),
(@RIFT_BOSS_ID_BASE + 13,2748,@RIFT_BOSS_ENTRY_BASE + 40,2,'boss_rift_archaedas','奥达曼',70,104.338,272.31,-51.6959,3.64774,75.78,213.714,-54.974,1.96,'阿扎达斯；裂隙专用四类石像守卫'),
(@RIFT_BOSS_ID_BASE + 13,2748,@RIFT_BOSS_ENTRY_BASE + 41,3,'boss_rift_archaedas','奥达曼',70,104.338,272.31,-51.6959,3.64774,75.78,213.714,-54.974,1.96,'阿扎达斯；裂隙专用四类石像守卫'),
(@RIFT_BOSS_ID_BASE + 14,5709,@RIFT_BOSS_ENTRY_BASE + 42,1,'boss_rift_eranikus','沉没的神庙',109,-658.379,-35.7623,-90.8352,1.57592,-659.882,61.247,-90.833,4.66,'伊兰尼库斯的阴影；不拉副本原生龙类'),
(@RIFT_BOSS_ID_BASE + 14,5709,@RIFT_BOSS_ENTRY_BASE + 43,2,'boss_rift_eranikus','沉没的神庙',109,-658.379,-35.7623,-90.8352,1.57592,-659.882,61.247,-90.833,4.66,'伊兰尼库斯的阴影；不拉副本原生龙类'),
(@RIFT_BOSS_ID_BASE + 14,5709,@RIFT_BOSS_ENTRY_BASE + 44,3,'boss_rift_eranikus','沉没的神庙',109,-658.379,-35.7623,-90.8352,1.57592,-659.882,61.247,-90.833,4.66,'伊兰尼库斯的阴影；不拉副本原生龙类'),
(@RIFT_BOSS_ID_BASE + 15,5710,@RIFT_BOSS_ENTRY_BASE + 45,1,'boss_rift_jammalan','沉没的神庙',109,-425.894,-86.0747,-88.224,3.11157,-493.84,-85.272,-90.827,6.28,'预言者迦玛兰；裂隙专用地缚图腾'),
(@RIFT_BOSS_ID_BASE + 15,5710,@RIFT_BOSS_ENTRY_BASE + 46,2,'boss_rift_jammalan','沉没的神庙',109,-425.894,-86.0747,-88.224,3.11157,-493.84,-85.272,-90.827,6.28,'预言者迦玛兰；裂隙专用地缚图腾'),
(@RIFT_BOSS_ID_BASE + 15,5710,@RIFT_BOSS_ENTRY_BASE + 47,3,'boss_rift_jammalan','沉没的神庙',109,-425.894,-86.0747,-88.224,3.11157,-493.84,-85.272,-90.827,6.28,'预言者迦玛兰；裂隙专用地缚图腾'),
(@RIFT_BOSS_ID_BASE + 16,5775,@RIFT_BOSS_ENTRY_BASE + 48,1,'boss_rift_verdan','哀嚎洞穴',43,-81.8554,32.2565,-30.9939,0,-138.47,-4.46,-28.17,0.48,'永生者沃尔丹；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 16,5775,@RIFT_BOSS_ENTRY_BASE + 49,2,'boss_rift_verdan','哀嚎洞穴',43,-81.8554,32.2565,-30.9939,0,-138.47,-4.46,-28.17,0.48,'永生者沃尔丹；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 16,5775,@RIFT_BOSS_ENTRY_BASE + 50,3,'boss_rift_verdan','哀嚎洞穴',43,-81.8554,32.2565,-30.9939,0,-138.47,-4.46,-28.17,0.48,'永生者沃尔丹；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 17,3654,@RIFT_BOSS_ENTRY_BASE + 51,1,'boss_rift_mutanus','哀嚎洞穴',43,151.27,252.26,-102.82,0,115.407,240.016,-94.021,3.08,'吞噬者穆坦努斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 17,3654,@RIFT_BOSS_ENTRY_BASE + 52,2,'boss_rift_mutanus','哀嚎洞穴',43,151.27,252.26,-102.82,0,115.407,240.016,-94.021,3.08,'吞噬者穆坦努斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 17,3654,@RIFT_BOSS_ENTRY_BASE + 53,3,'boss_rift_mutanus','哀嚎洞穴',43,151.27,252.26,-102.82,0,115.407,240.016,-94.021,3.08,'吞噬者穆坦努斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 18,4278,@RIFT_BOSS_ENTRY_BASE + 54,1,'boss_rift_springvale','影牙城堡',33,-222.592,2259.44,102.839,0,-262.51,2246.29,100.89,0.28,'指挥官斯普林瓦尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 18,4278,@RIFT_BOSS_ENTRY_BASE + 55,2,'boss_rift_springvale','影牙城堡',33,-222.592,2259.44,102.839,0,-262.51,2246.29,100.89,0.28,'指挥官斯普林瓦尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 18,4278,@RIFT_BOSS_ENTRY_BASE + 56,3,'boss_rift_springvale','影牙城堡',33,-222.592,2259.44,102.839,0,-262.51,2246.29,100.89,0.28,'指挥官斯普林瓦尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 19,4275,@RIFT_BOSS_ENTRY_BASE + 57,1,'boss_rift_arugal','影牙城堡',33,-76.7541,2152.41,155.792,0,-134.17,2167.25,155.68,5.90,'大法师阿鲁高；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 19,4275,@RIFT_BOSS_ENTRY_BASE + 58,2,'boss_rift_arugal','影牙城堡',33,-76.7541,2152.41,155.792,0,-134.17,2167.25,155.68,5.90,'大法师阿鲁高；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 19,4275,@RIFT_BOSS_ENTRY_BASE + 59,3,'boss_rift_arugal','影牙城堡',33,-76.7541,2152.41,155.792,0,-134.17,2167.25,155.68,5.90,'大法师阿鲁高；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 20,6487,@RIFT_BOSS_ENTRY_BASE + 60,1,'boss_rift_doan','血色图书馆',189,148.32,-428.692,18.4864,0,187.66,-429.1,18.53,3.01,'奥法师杜安；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 20,6487,@RIFT_BOSS_ENTRY_BASE + 61,2,'boss_rift_doan','血色图书馆',189,148.32,-428.692,18.4864,0,187.66,-429.1,18.53,3.01,'奥法师杜安；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 20,6487,@RIFT_BOSS_ENTRY_BASE + 62,3,'boss_rift_doan','血色图书馆',189,148.32,-428.692,18.4864,0,187.66,-429.1,18.53,3.01,'奥法师杜安；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 21,7273,@RIFT_BOSS_ENTRY_BASE + 63,1,'boss_rift_gahzrilla','祖尔法拉克',209,1659.43,1180.5,1.05,0,1660.29,1142.86,8.88,1.35,'加兹瑞拉；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 21,7273,@RIFT_BOSS_ENTRY_BASE + 64,2,'boss_rift_gahzrilla','祖尔法拉克',209,1659.43,1180.5,1.05,0,1660.29,1142.86,8.88,1.35,'加兹瑞拉；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 21,7273,@RIFT_BOSS_ENTRY_BASE + 65,3,'boss_rift_gahzrilla','祖尔法拉克',209,1659.43,1180.5,1.05,0,1660.29,1142.86,8.88,1.35,'加兹瑞拉；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 22,7267,@RIFT_BOSS_ENTRY_BASE + 66,1,'boss_rift_ukorz','祖尔法拉克',209,1727.48,1017.35,54.9102,0,1730.28,1015.17,54.91,0.90,'乌克兹·沙顶/卢兹鲁双Boss；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 22,7267,@RIFT_BOSS_ENTRY_BASE + 67,2,'boss_rift_ukorz','祖尔法拉克',209,1727.48,1017.35,54.9102,0,1730.28,1015.17,54.91,0.90,'乌克兹·沙顶/卢兹鲁双Boss；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 22,7267,@RIFT_BOSS_ENTRY_BASE + 68,3,'boss_rift_ukorz','祖尔法拉克',209,1727.48,1017.35,54.9102,0,1730.28,1015.17,54.91,0.90,'乌克兹·沙顶/卢兹鲁双Boss；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 23,12236,@RIFT_BOSS_ENTRY_BASE + 69,1,'boss_rift_vyletongue','玛拉顿',349,748.874,-219.647,-47.6926,0,692.71,-219.9,-47.28,6.26,'维利塔恩；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 23,12236,@RIFT_BOSS_ENTRY_BASE + 70,2,'boss_rift_vyletongue','玛拉顿',349,748.874,-219.647,-47.6926,0,692.71,-219.9,-47.28,6.26,'维利塔恩；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 23,12236,@RIFT_BOSS_ENTRY_BASE + 71,3,'boss_rift_vyletongue','玛拉顿',349,748.874,-219.647,-47.6926,0,692.71,-219.9,-47.28,6.26,'维利塔恩；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 24,12225,@RIFT_BOSS_ENTRY_BASE + 72,1,'boss_rift_celebras','玛拉顿',349,726.106,77.9764,-86.5913,0,796.76,62.35,-86.86,2.75,'被诅咒的塞雷布拉斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 24,12225,@RIFT_BOSS_ENTRY_BASE + 73,2,'boss_rift_celebras','玛拉顿',349,726.106,77.9764,-86.5913,0,796.76,62.35,-86.86,2.75,'被诅咒的塞雷布拉斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 24,12225,@RIFT_BOSS_ENTRY_BASE + 74,3,'boss_rift_celebras','玛拉顿',349,726.106,77.9764,-86.5913,0,796.76,62.35,-86.86,2.75,'被诅咒的塞雷布拉斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 25,12201,@RIFT_BOSS_ENTRY_BASE + 75,1,'boss_rift_theradras','玛拉顿',349,27.8981,83.1932,-124.483,0,29.17,4.66,-127.46,1.53,'瑟莱德丝公主；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 25,12201,@RIFT_BOSS_ENTRY_BASE + 76,2,'boss_rift_theradras','玛拉顿',349,27.8981,83.1932,-124.483,0,29.17,4.66,-127.46,1.53,'瑟莱德丝公主；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 25,12201,@RIFT_BOSS_ENTRY_BASE + 77,3,'boss_rift_theradras','玛拉顿',349,27.8981,83.1932,-124.483,0,29.17,4.66,-127.46,1.53,'瑟莱德丝公主；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 26,9025,@RIFT_BOSS_ENTRY_BASE + 78,1,'boss_rift_roccor','黑石深渊',230,615.522,-267.397,-83.5907,0,632.02,-144.26,-70.74,2.50,'洛考尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 26,9025,@RIFT_BOSS_ENTRY_BASE + 79,2,'boss_rift_roccor','黑石深渊',230,615.522,-267.397,-83.5907,0,632.02,-144.26,-70.74,2.50,'洛考尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 26,9025,@RIFT_BOSS_ENTRY_BASE + 80,3,'boss_rift_roccor','黑石深渊',230,615.522,-267.397,-83.5907,0,632.02,-144.26,-70.74,2.50,'洛考尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 27,9017,@RIFT_BOSS_ENTRY_BASE + 81,1,'boss_rift_incendius','黑石深渊',230,893.546,-267.056,-71.9002,0,854.42,-233.45,-71.76,5.55,'伊森迪奥斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 27,9017,@RIFT_BOSS_ENTRY_BASE + 82,2,'boss_rift_incendius','黑石深渊',230,893.546,-267.056,-71.9002,0,854.42,-233.45,-71.76,5.55,'伊森迪奥斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 27,9017,@RIFT_BOSS_ENTRY_BASE + 83,3,'boss_rift_incendius','黑石深渊',230,893.546,-267.056,-71.9002,0,854.42,-233.45,-71.76,5.55,'伊森迪奥斯；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 28,9016,@RIFT_BOSS_ENTRY_BASE + 84,1,'boss_rift_baelgar','黑石深渊',230,702.416,184.462,-71.988,0,683.17,135.67,-73.22,1.20,'贝尔加；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 28,9016,@RIFT_BOSS_ENTRY_BASE + 85,2,'boss_rift_baelgar','黑石深渊',230,702.416,184.462,-71.988,0,683.17,135.67,-73.22,1.20,'贝尔加；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 28,9016,@RIFT_BOSS_ENTRY_BASE + 86,3,'boss_rift_baelgar','黑石深渊',230,702.416,184.462,-71.988,0,683.17,135.67,-73.22,1.20,'贝尔加；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 29,8983,@RIFT_BOSS_ENTRY_BASE + 87,1,'boss_rift_argelmach','黑石深渊',230,846.801,16.2806,-53.6395,0,801.99,22.66,-53.66,6.12,'傀儡统帅阿格曼奇；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 29,8983,@RIFT_BOSS_ENTRY_BASE + 88,2,'boss_rift_argelmach','黑石深渊',230,846.801,16.2806,-53.6395,0,801.99,22.66,-53.66,6.12,'傀儡统帅阿格曼奇；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 29,8983,@RIFT_BOSS_ENTRY_BASE + 89,3,'boss_rift_argelmach','黑石深渊',230,846.801,16.2806,-53.6395,0,801.99,22.66,-53.66,6.12,'傀儡统帅阿格曼奇；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 30,9156,@RIFT_BOSS_ENTRY_BASE + 90,1,'boss_rift_flamelash','黑石深渊',230,1009.75,-239.017,-61.3038,0,982.96,-212.1,-61.79,5.50,'弗莱拉斯大使；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 30,9156,@RIFT_BOSS_ENTRY_BASE + 91,2,'boss_rift_flamelash','黑石深渊',230,1009.75,-239.017,-61.3038,0,982.96,-212.1,-61.79,5.50,'弗莱拉斯大使；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 30,9156,@RIFT_BOSS_ENTRY_BASE + 92,3,'boss_rift_flamelash','黑石深渊',230,1009.75,-239.017,-61.3038,0,982.96,-212.1,-61.79,5.50,'弗莱拉斯大使；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 31,9039,@RIFT_BOSS_ENTRY_BASE + 93,1,'boss_rift_the_seven','黑石深渊',230,1215.05,-220.656,-85.5903,0,1223.09,-196.92,-85.69,5.30,'七贤(末日之链主Boss+6名成员)；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 31,9039,@RIFT_BOSS_ENTRY_BASE + 94,2,'boss_rift_the_seven','黑石深渊',230,1215.05,-220.656,-85.5903,0,1223.09,-196.92,-85.69,5.30,'七贤(末日之链主Boss+6名成员)；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 31,9039,@RIFT_BOSS_ENTRY_BASE + 95,3,'boss_rift_the_seven','黑石深渊',230,1215.05,-220.656,-85.5903,0,1223.09,-196.92,-85.69,5.30,'七贤(末日之链主Boss+6名成员)；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 32,9019,@RIFT_BOSS_ENTRY_BASE + 96,1,'boss_rift_thaurissan','黑石深渊',230,1380.18,-831.645,-87.59,0,1380.34,-756.31,-92.72,4.73,'达格兰·索瑞森大帝；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 32,9019,@RIFT_BOSS_ENTRY_BASE + 97,2,'boss_rift_thaurissan','黑石深渊',230,1380.18,-831.645,-87.59,0,1380.34,-756.31,-92.72,4.73,'达格兰·索瑞森大帝；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 32,9019,@RIFT_BOSS_ENTRY_BASE + 98,3,'boss_rift_thaurissan','黑石深渊',230,1380.18,-831.645,-87.59,0,1380.34,-756.31,-92.72,4.73,'达格兰·索瑞森大帝；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 33,10596,@RIFT_BOSS_ENTRY_BASE + 99,1,'boss_rift_smolderweb','黑石塔下层',229,-135.51,-565.85,10.17,0,-107.05,-520.27,10.85,4.20,'烟网蛛后；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 33,10596,@RIFT_BOSS_ENTRY_BASE + 100,2,'boss_rift_smolderweb','黑石塔下层',229,-135.51,-565.85,10.17,0,-107.05,-520.27,10.85,4.20,'烟网蛛后；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 33,10596,@RIFT_BOSS_ENTRY_BASE + 101,3,'boss_rift_smolderweb','黑石塔下层',229,-135.51,-565.85,10.17,0,-107.05,-520.27,10.85,4.20,'烟网蛛后；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 34,9568,@RIFT_BOSS_ENTRY_BASE + 102,1,'boss_rift_wyrmthalak','黑石塔下层',229,-22.6325,-486.186,90.7531,0,-65.37,-486.17,90.67,6.19,'维姆萨拉克；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 34,9568,@RIFT_BOSS_ENTRY_BASE + 103,2,'boss_rift_wyrmthalak','黑石塔下层',229,-22.6325,-486.186,90.7531,0,-65.37,-486.17,90.67,6.19,'维姆萨拉克；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 34,9568,@RIFT_BOSS_ENTRY_BASE + 104,3,'boss_rift_wyrmthalak','黑石塔下层',229,-22.6325,-486.186,90.7531,0,-65.37,-486.17,90.67,6.19,'维姆萨拉克；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 35,14327,@RIFT_BOSS_ENTRY_BASE + 105,1,'boss_rift_lethtendris','厄运之槌东区',429,-5.4506,-441.126,16.4179,0,-13.72,-453.08,16.4,1.67,'蕾瑟塔蒂丝；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 35,14327,@RIFT_BOSS_ENTRY_BASE + 106,2,'boss_rift_lethtendris','厄运之槌东区',429,-5.4506,-441.126,16.4179,0,-13.72,-453.08,16.4,1.67,'蕾瑟塔蒂丝；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 35,14327,@RIFT_BOSS_ENTRY_BASE + 107,3,'boss_rift_lethtendris','厄运之槌东区',429,-5.4506,-441.126,16.4179,0,-13.72,-453.08,16.4,1.67,'蕾瑟塔蒂丝；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 36,11492,@RIFT_BOSS_ENTRY_BASE + 108,1,'boss_rift_alzzin','厄运之槌东区',429,274.844,-427.251,-119.962,0,260.45,-503.11,-119.12,1.50,'奥兹恩；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 36,11492,@RIFT_BOSS_ENTRY_BASE + 109,2,'boss_rift_alzzin','厄运之槌东区',429,274.844,-427.251,-119.962,0,260.45,-503.11,-119.12,1.50,'奥兹恩；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 36,11492,@RIFT_BOSS_ENTRY_BASE + 110,3,'boss_rift_alzzin','厄运之槌东区',429,274.844,-427.251,-119.962,0,260.45,-503.11,-119.12,1.50,'奥兹恩；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 37,11496,@RIFT_BOSS_ENTRY_BASE + 111,1,'boss_rift_immolthar','厄运之槌西区',429,-38.0807,812.44,-29.4525,0,-93.86,761.22,-26.02,0.74,'伊莫塔尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 37,11496,@RIFT_BOSS_ENTRY_BASE + 112,2,'boss_rift_immolthar','厄运之槌西区',429,-38.0807,812.44,-29.4525,0,-93.86,761.22,-26.02,0.74,'伊莫塔尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 37,11496,@RIFT_BOSS_ENTRY_BASE + 113,3,'boss_rift_immolthar','厄运之槌西区',429,-38.0807,812.44,-29.4525,0,-93.86,761.22,-26.02,0.74,'伊莫塔尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 38,11486,@RIFT_BOSS_ENTRY_BASE + 114,1,'boss_rift_tortheldrin','厄运之槌西区',429,132.626,625.913,-48.3836,0,171.16,586.69,-48.47,2.31,'托塞德林王子；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 38,11486,@RIFT_BOSS_ENTRY_BASE + 115,2,'boss_rift_tortheldrin','厄运之槌西区',429,132.626,625.913,-48.3836,0,171.16,586.69,-48.47,2.31,'托塞德林王子；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 38,11486,@RIFT_BOSS_ENTRY_BASE + 116,3,'boss_rift_tortheldrin','厄运之槌西区',429,132.626,625.913,-48.3836,0,171.16,586.69,-48.47,2.31,'托塞德林王子；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 39,14326,@RIFT_BOSS_ENTRY_BASE + 117,1,'boss_rift_moldar','厄运之槌北区',429,410.711,-3.1504,-24.558,0,355.24,0.47,-24.64,6.19,'卫兵摩尔达；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 39,14326,@RIFT_BOSS_ENTRY_BASE + 118,2,'boss_rift_moldar','厄运之槌北区',429,410.711,-3.1504,-24.558,0,355.24,0.47,-24.64,6.19,'卫兵摩尔达；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 39,14326,@RIFT_BOSS_ENTRY_BASE + 119,3,'boss_rift_moldar','厄运之槌北区',429,410.711,-3.1504,-24.558,0,355.24,0.47,-24.64,6.19,'卫兵摩尔达；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 40,11501,@RIFT_BOSS_ENTRY_BASE + 120,1,'boss_rift_gordok','厄运之槌北区',429,828.074,480.751,37.318,0,833.99,489.54,37.4,3.21,'戈多克大王/观察者克鲁什双Boss；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 40,11501,@RIFT_BOSS_ENTRY_BASE + 121,2,'boss_rift_gordok','厄运之槌北区',429,828.074,480.751,37.318,0,833.99,489.54,37.4,3.21,'戈多克大王/观察者克鲁什双Boss；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 40,11501,@RIFT_BOSS_ENTRY_BASE + 122,3,'boss_rift_gordok','厄运之槌北区',429,828.074,480.751,37.318,0,833.99,489.54,37.4,3.21,'戈多克大王/观察者克鲁什双Boss；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 41,10508,@RIFT_BOSS_ENTRY_BASE + 123,1,'boss_rift_ras_frostwhisper','通灵学院',289,-25.1079,141.284,83.9083,0,23.89,142.94,83.55,3.15,'莱斯·霜语；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 41,10508,@RIFT_BOSS_ENTRY_BASE + 124,2,'boss_rift_ras_frostwhisper','通灵学院',289,-25.1079,141.284,83.9083,0,23.89,142.94,83.55,3.15,'莱斯·霜语；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 41,10508,@RIFT_BOSS_ENTRY_BASE + 125,3,'boss_rift_ras_frostwhisper','通灵学院',289,-25.1079,141.284,83.9083,0,23.89,142.94,83.55,3.15,'莱斯·霜语；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 42,10507,@RIFT_BOSS_ENTRY_BASE + 126,1,'boss_rift_ravenian','通灵学院',289,103.305,-1.6775,75.2183,0,148.8,0.04,75.4,3.12,'拉文尼亚；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 42,10507,@RIFT_BOSS_ENTRY_BASE + 127,2,'boss_rift_ravenian','通灵学院',289,103.305,-1.6775,75.2183,0,148.8,0.04,75.4,3.12,'拉文尼亚；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 42,10507,@RIFT_BOSS_ENTRY_BASE + 128,3,'boss_rift_ravenian','通灵学院',289,103.305,-1.6775,75.2183,0,148.8,0.04,75.4,3.12,'拉文尼亚；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 43,1853,@RIFT_BOSS_ENTRY_BASE + 129,1,'boss_rift_gandling','通灵学院',289,180.771,-5.4286,75.5702,0,178.87,23.59,88.89,4.74,'黑暗院长加丁；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 43,1853,@RIFT_BOSS_ENTRY_BASE + 130,2,'boss_rift_gandling','通灵学院',289,180.771,-5.4286,75.5702,0,178.87,23.59,88.89,4.74,'黑暗院长加丁；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 43,1853,@RIFT_BOSS_ENTRY_BASE + 131,3,'boss_rift_gandling','通灵学院',289,180.771,-5.4286,75.5702,0,178.87,23.59,88.89,4.74,'黑暗院长加丁；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 44,10813,@RIFT_BOSS_ENTRY_BASE + 132,1,'boss_rift_balnazzar','斯坦索姆正门',329,3415.84,-3044.54,136.814,0,3434.92,-3071.78,136.54,2.19,'巴纳扎尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 44,10813,@RIFT_BOSS_ENTRY_BASE + 133,2,'boss_rift_balnazzar','斯坦索姆正门',329,3415.84,-3044.54,136.814,0,3434.92,-3071.78,136.54,2.19,'巴纳扎尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 44,10813,@RIFT_BOSS_ENTRY_BASE + 134,3,'boss_rift_balnazzar','斯坦索姆正门',329,3415.84,-3044.54,136.814,0,3434.92,-3071.78,136.54,2.19,'巴纳扎尔；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 45,10436,@RIFT_BOSS_ENTRY_BASE + 135,1,'boss_rift_anastari','斯坦索姆后门',329,3855.34,-3715.75,148.175,0,3836.7,-3655.99,145.29,5.08,'安娜丝塔丽男爵夫人；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 45,10436,@RIFT_BOSS_ENTRY_BASE + 136,2,'boss_rift_anastari','斯坦索姆后门',329,3855.34,-3715.75,148.175,0,3836.7,-3655.99,145.29,5.08,'安娜丝塔丽男爵夫人；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 45,10436,@RIFT_BOSS_ENTRY_BASE + 137,3,'boss_rift_anastari','斯坦索姆后门',329,3855.34,-3715.75,148.175,0,3836.7,-3655.99,145.29,5.08,'安娜丝塔丽男爵夫人；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 46,10440,@RIFT_BOSS_ENTRY_BASE + 138,1,'boss_rift_rivendare','斯坦索姆后门',329,4035.83,-3336.31,115.144,0,4033.2,-3402.78,115.26,1.64,'瑞文戴尔男爵；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 46,10440,@RIFT_BOSS_ENTRY_BASE + 139,2,'boss_rift_rivendare','斯坦索姆后门',329,4035.83,-3336.31,115.144,0,4033.2,-3402.78,115.26,1.64,'瑞文戴尔男爵；传入点来自表格'),
(@RIFT_BOSS_ID_BASE + 46,10440,@RIFT_BOSS_ENTRY_BASE + 140,3,'boss_rift_rivendare','斯坦索姆后门',329,4035.83,-3336.31,115.144,0,4033.2,-3402.78,115.26,1.64,'瑞文戴尔男爵；传入点来自表格');

-- ============================================================================
-- 3. 裂隙专用动态同伴和召唤物映射
-- ============================================================================
DROP TEMPORARY TABLE IF EXISTS `_rift_summon_map`;
CREATE TEMPORARY TABLE `_rift_summon_map` (
  `source_entry` INT UNSIGNED NOT NULL,
  `new_entry` INT UNSIGNED NOT NULL,
  `boss_source_entry` INT UNSIGNED NOT NULL,
  `script_name` VARCHAR(64) NOT NULL,
  `name_suffix` VARCHAR(64) NOT NULL,
  PRIMARY KEY (`new_entry`)
);

INSERT INTO `_rift_summon_map` (`source_entry`,`new_entry`,`boss_source_entry`,`script_name`,`name_suffix`) VALUES
(3977,@RIFT_SUMMON_ENTRY_BASE + 0,3976,'npc_rift_whitemane','裂隙怀特迈恩'),
(7915,@RIFT_SUMMON_ENTRY_BASE + 1,7800,'npc_rift_walking_bomb','裂隙步行炸弹'),
(8585,@RIFT_SUMMON_ENTRY_BASE + 2,7358,'npc_rift_frost_spectre','裂隙冰霜亡魂'),
(6575,@RIFT_SUMMON_ENTRY_BASE + 3,3975,'npc_rift_scarlet_trainee','裂隙血色预备兵'),
(6066,@RIFT_SUMMON_ENTRY_BASE + 4,5710,'npc_rift_earthgrab_totem','裂隙地缚图腾'),
(7076,@RIFT_SUMMON_ENTRY_BASE + 5,2748,'npc_rift_earthen_guardian','裂隙地灵守护者'),
(10120,@RIFT_SUMMON_ENTRY_BASE + 6,2748,'npc_rift_vault_warder','裂隙宝库守卫'),
(7077,@RIFT_SUMMON_ENTRY_BASE + 7,2748,'npc_rift_earthen_hallshaper','裂隙地灵塑石者'),
(7309,@RIFT_SUMMON_ENTRY_BASE + 8,2748,'npc_rift_earthen_custodian','裂隙地灵看守者'),
(4825,@RIFT_SUMMON_ENTRY_BASE + 9,4829,'npc_rift_akumai_snapjaw','裂隙阿库麦尔钳嘴龟'),
(4978,@RIFT_SUMMON_ENTRY_BASE + 10,4829,'npc_rift_akumai_servant','裂隙阿库麦尔仆从'),
(7797,@RIFT_SUMMON_ENTRY_BASE + 11,7267,'npc_rift_ruuzlu','裂隙卢兹鲁'),
(14324,@RIFT_SUMMON_ENTRY_BASE + 12,11501,'npc_rift_chorsh','裂隙观察者克鲁什'),
(8929,@RIFT_SUMMON_ENTRY_BASE + 13,9019,'npc_rift_moira','裂隙茉艾拉公主'),
(11598,@RIFT_SUMMON_ENTRY_BASE + 14,1853,'npc_rift_risen_guardian','裂隙复生的守卫'),
(9178,@RIFT_SUMMON_ENTRY_BASE + 15,9156,'npc_rift_burning_spirit','裂隙燃烧之灵'),
(9216,@RIFT_SUMMON_ENTRY_BASE + 16,9568,'npc_rift_spirestone_warlord','裂隙尖石军阀'),
(9268,@RIFT_SUMMON_ENTRY_BASE + 17,9568,'npc_rift_smolderthorn_berserker','裂隙燃棘狂战士'),
(4627,@RIFT_SUMMON_ENTRY_BASE + 18,4275,'npc_rift_arugal_voidwalker','裂隙阿鲁高的虚空行者'),
(8996,@RIFT_SUMMON_ENTRY_BASE + 19,4275,'npc_rift_voidwalker_minion','裂隙虚空行者仆从'),
(10375,@RIFT_SUMMON_ENTRY_BASE + 20,10596,'npc_rift_spire_spiderling','裂隙尖塔小蜘蛛'),
(11197,@RIFT_SUMMON_ENTRY_BASE + 21,10440,'npc_rift_mindless_skeleton','裂隙无脑骷髅'),
(11460,@RIFT_SUMMON_ENTRY_BASE + 22,11492,'npc_rift_alzzin_minion','裂隙奥兹恩仆从'),
(9436,@RIFT_SUMMON_ENTRY_BASE + 23,9016,'npc_rift_baelgar_spawn','裂隙贝尔加幼体'),
(9034,@RIFT_SUMMON_ENTRY_BASE + 24,9039,'npc_rift_seven_haterel','裂隙七贤·仇恨者'),
(9035,@RIFT_SUMMON_ENTRY_BASE + 25,9039,'npc_rift_seven_angerrel','裂隙七贤·愤怒者'),
(9036,@RIFT_SUMMON_ENTRY_BASE + 26,9039,'npc_rift_seven_vilerel','裂隙七贤·邪恶者'),
(9037,@RIFT_SUMMON_ENTRY_BASE + 27,9039,'npc_rift_seven_gloomrel','裂隙七贤·忧郁者'),
(9038,@RIFT_SUMMON_ENTRY_BASE + 28,9039,'npc_rift_seven_seethrel','裂隙七贤·沸腾者'),
(9040,@RIFT_SUMMON_ENTRY_BASE + 29,9039,'npc_rift_seven_doperel','裂隙七贤·愚昧者'),
(8900,@RIFT_SUMMON_ENTRY_BASE + 30,8983,'npc_rift_argelmach_arcanasmith','裂隙末日熔炉奥术铁匠'),
(8906,@RIFT_SUMMON_ENTRY_BASE + 31,8983,'npc_rift_argelmach_golem','裂隙怒削魔像'),
(8907,@RIFT_SUMMON_ENTRY_BASE + 32,8983,'npc_rift_argelmach_wrath_hammer','裂隙怒火之锤构造体'),
(8920,@RIFT_SUMMON_ENTRY_BASE + 33,8983,'npc_rift_argelmach_technician','裂隙武器技师');

DROP TEMPORARY TABLE IF EXISTS `_rift_template_map`;
CREATE TEMPORARY TABLE `_rift_template_map` AS
SELECT `source_entry`, `new_entry`, `script_name`, CONCAT('Rift T', `tier`) AS `name_suffix`, 1 AS `is_boss`, 0 AS `boss_source_entry`
FROM `_rift_boss_map`
UNION ALL
SELECT `source_entry`, `new_entry`, `script_name`, `name_suffix`, 0, `boss_source_entry`
FROM `_rift_summon_map`;

-- ============================================================================
-- 4. 生成60级Boss与召唤物模板
-- ============================================================================
DELETE FROM `creature_template_model` WHERE `CreatureID` IN (SELECT `new_entry` FROM `_rift_template_map`);
DELETE FROM `creature_template_spell` WHERE `CreatureID` IN (SELECT `new_entry` FROM `_rift_template_map`);
DELETE FROM `creature_template_resistance` WHERE `CreatureID` IN (SELECT `new_entry` FROM `_rift_template_map`);
DELETE FROM `creature_template_addon` WHERE `entry` IN (SELECT `new_entry` FROM `_rift_template_map`);
DELETE FROM `creature_equip_template` WHERE `CreatureID` IN (SELECT `new_entry` FROM `_rift_template_map`);
DELETE FROM `creature_template_movement` WHERE `CreatureId` IN (SELECT `new_entry` FROM `_rift_template_map`);
DELETE FROM `creature_template` WHERE `entry` IN (SELECT `new_entry` FROM `_rift_template_map`);

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
  m.new_entry,0,0,0,0,0,CONCAT(ct.name,' [',m.name_suffix,']'),ct.subname,ct.IconName,0,
  IF(m.is_boss=1,83,82),IF(m.is_boss=1,83,82),2,ct.faction,0,ct.speed_walk,ct.speed_run,ct.speed_swim,ct.speed_flight,ct.detection_range,
  IF(m.is_boss=1,1,ct.rank),ct.dmgschool,
  IF(m.is_boss=1,@RIFT_BOSS_DAMAGE_MODIFIER,@RIFT_SUMMON_DAMAGE_MODIFIER),
  ct.BaseAttackTime,ct.RangeAttackTime,ct.BaseVariance,ct.RangeVariance,ct.unit_class,
  (ct.unit_flags & IF(m.is_boss=1,@RIFT_BOSS_UNIT_FLAGS_KEEP_MASK,@RIFT_SUMMON_UNIT_FLAGS_KEEP_MASK)),
  IF(m.is_boss=1,0,ct.unit_flags2),0,ct.family,ct.type,ct.type_flags,
  0,0,0,0,0,0,0,'',0,ct.HoverHeight,
  (IF(m.is_boss=1,
      IF(ct.unit_class IN (1,2), @RIFT_BOSS_HM_MELEE, @RIFT_BOSS_HM_CASTER),
      (bossDst.basehp2 * IF(bossCt.unit_class IN (1,2), @RIFT_BOSS_HM_MELEE, @RIFT_BOSS_HM_CASTER))
        * (CASE ct.exp WHEN 0 THEN srcStats.basehp0 WHEN 1 THEN srcStats.basehp1 ELSE srcStats.basehp2 END) * ct.HealthModifier
        / (CASE bossCt.exp WHEN 0 THEN bossSrc.basehp0 WHEN 1 THEN bossSrc.basehp1 ELSE bossSrc.basehp2 END) / bossCt.HealthModifier
        / dstStats.basehp2)),
  ct.ManaModifier,ct.ArmorModifier,ct.ExperienceModifier,0,0,ct.RegenHealth,ct.CreatureImmunitiesId,
  IF(m.is_boss=1,0,ct.flags_extra),m.script_name,ct.VerifiedBuild
FROM `_rift_template_map` m
JOIN `creature_template` ct ON ct.entry=m.source_entry
JOIN `creature_classlevelstats` srcStats ON srcStats.level=ct.minlevel AND srcStats.`class`=ct.unit_class
JOIN `creature_classlevelstats` dstStats ON dstStats.level=IF(m.is_boss=1,83,82) AND dstStats.`class`=ct.unit_class
LEFT JOIN `creature_template` bossCt ON bossCt.entry=m.boss_source_entry
LEFT JOIN `creature_classlevelstats` bossSrc ON bossSrc.level=bossCt.minlevel AND bossSrc.`class`=bossCt.unit_class
LEFT JOIN `creature_classlevelstats` bossDst ON bossDst.level=83 AND bossDst.`class`=bossCt.unit_class;

INSERT INTO `creature_template_model`
SELECT m.new_entry,src.Idx,src.CreatureDisplayID,src.DisplayScale,src.Probability,src.VerifiedBuild
FROM `creature_template_model` src JOIN `_rift_template_map` m ON m.source_entry=src.CreatureID;

INSERT INTO `creature_template_spell`
SELECT m.new_entry,src.`Index`,src.Spell,src.VerifiedBuild
FROM `creature_template_spell` src JOIN `_rift_template_map` m ON m.source_entry=src.CreatureID;

INSERT INTO `creature_template_resistance`
SELECT m.new_entry,src.School,src.Resistance,src.VerifiedBuild
FROM `creature_template_resistance` src JOIN `_rift_template_map` m ON m.source_entry=src.CreatureID;

-- 主Boss保留常驻战斗光环，但清除 bytes1 低字节中的睡眠、死亡等站姿，确保生成时为站立状态。
INSERT INTO `creature_template_addon`
SELECT m.new_entry,0,src.mount,IF(m.is_boss=1,(src.bytes1 & 4294967040),src.bytes1),
       src.bytes2,src.emote,src.visibilityDistanceType,src.auras
FROM `creature_template_addon` src JOIN `_rift_template_map` m ON m.source_entry=src.entry;

INSERT INTO `creature_equip_template`
SELECT m.new_entry,src.ID,src.ItemID1,src.ItemID2,src.ItemID3,src.VerifiedBuild
FROM `creature_equip_template` src JOIN `_rift_template_map` m ON m.source_entry=src.CreatureID;

-- 主Boss使用默认可追击移动，不继承源副本可能存在的 Rooted 或特殊移动限制。
INSERT INTO `creature_template_movement`
SELECT m.new_entry,src.Ground,src.Swim,src.Flight,src.Rooted,src.Chase,src.Random,src.InteractionPauseTimer
FROM `creature_template_movement` src JOIN `_rift_template_map` m ON m.source_entry=src.CreatureId
WHERE m.is_boss=0;

-- ============================================================================
-- 5. 写入60级静态Boss与Tier配置
-- ============================================================================
DELETE FROM `heroic_dungeon_rift_boss_tier`
WHERE `boss_id` BETWEEN @RIFT_BOSS_ID_BASE + 0 AND @RIFT_BOSS_ID_BASE + 46;
DELETE FROM `heroic_dungeon_rift_boss`
WHERE `boss_id` BETWEEN @RIFT_BOSS_ID_BASE + 0 AND @RIFT_BOSS_ID_BASE + 46;

INSERT INTO `heroic_dungeon_rift_boss`
(`boss_id`,`map_name`,`map_id`,`dungeon_version`,`player_entry_x`,`player_entry_y`,`player_entry_z`,`player_entry_o`,`enabled`,`remark`)
SELECT boss_id,MAX(map_name),MAX(map_id),60,MAX(player_x),MAX(player_y),MAX(player_z),MAX(player_o),1,MAX(remark)
FROM `_rift_boss_map` GROUP BY boss_id;

INSERT INTO `heroic_dungeon_rift_boss_tier`
(`boss_id`,`entry_id`,`boss_spawn_x`,`boss_spawn_y`,`boss_spawn_z`,`boss_spawn_o`,`tier`,
 `health_multiplier`,`damage_multiplier`,`player_entry_x`,`player_entry_y`,`player_entry_z`,`player_entry_o`)
SELECT boss_id,new_entry,boss_x,boss_y,boss_z,boss_o,tier,
  CASE tier WHEN 1 THEN @RIFT_T1_HEALTH WHEN 2 THEN @RIFT_T2_HEALTH ELSE @RIFT_T3_HEALTH END,
  CASE tier WHEN 1 THEN @RIFT_T1_DAMAGE WHEN 2 THEN @RIFT_T2_DAMAGE ELSE @RIFT_T3_DAMAGE END,
  player_x,player_y,player_z,player_o
FROM `_rift_boss_map`;

-- ============================================================================
-- 6. 三档出口门
-- ============================================================================
DELETE FROM `gameobject_template`
WHERE `entry` IN (@RIFT_EXIT_PORTAL_ENTRY_BASE + 0,@RIFT_EXIT_PORTAL_ENTRY_BASE + 1,@RIFT_EXIT_PORTAL_ENTRY_BASE + 2);
INSERT INTO `gameobject_template` (
  `entry`,`type`,`displayId`,`name`,`IconName`,`castBarCaption`,`unk1`,`size`,
  `Data0`,`Data1`,`Data2`,`Data3`,`Data4`,`Data5`,`Data6`,`Data7`,`Data8`,`Data9`,`Data10`,`Data11`,
  `Data12`,`Data13`,`Data14`,`Data15`,`Data16`,`Data17`,`Data18`,`Data19`,`Data20`,`Data21`,`Data22`,`Data23`,
  `AIName`,`ScriptName`,`VerifiedBuild`)
SELECT tier.entry,src.type,tier.display_id,CONCAT('五人英雄裂隙 T',tier.tier,' 出口'),'','','',src.size,
  src.Data0,src.Data1,src.Data2,src.Data3,src.Data4,src.Data5,src.Data6,src.Data7,src.Data8,src.Data9,src.Data10,src.Data11,
  src.Data12,src.Data13,src.Data14,src.Data15,src.Data16,src.Data17,src.Data18,src.Data19,src.Data20,src.Data21,src.Data22,src.Data23,
  '','rift_exit_portal',src.VerifiedBuild
FROM `gameobject_template` src
CROSS JOIN (
  SELECT 1 tier,@RIFT_EXIT_PORTAL_ENTRY_BASE + 0 entry,7148 display_id
  UNION ALL SELECT 2,@RIFT_EXIT_PORTAL_ENTRY_BASE + 1,8196
  UNION ALL SELECT 3,@RIFT_EXIT_PORTAL_ENTRY_BASE + 2,8197
) tier
WHERE src.entry=@RIFT_EXIT_PORTAL_SOURCE_ENTRY;

-- ============================================================================
-- 7. 入口NPC模板（世界生成坐标仍待用户确认）
-- ============================================================================
-- 入口Entry可能覆盖早期审核草案中的Boss模板，因此先清理全部模板附属数据。
DELETE FROM `creature_template_model` WHERE `CreatureID`=@RIFT_ENTRY_NPC_ENTRY;
DELETE FROM `creature_template_spell` WHERE `CreatureID`=@RIFT_ENTRY_NPC_ENTRY;
DELETE FROM `creature_template_resistance` WHERE `CreatureID`=@RIFT_ENTRY_NPC_ENTRY;
DELETE FROM `creature_template_addon` WHERE `entry`=@RIFT_ENTRY_NPC_ENTRY;
DELETE FROM `creature_equip_template` WHERE `CreatureID`=@RIFT_ENTRY_NPC_ENTRY;
DELETE FROM `creature_template_movement` WHERE `CreatureId`=@RIFT_ENTRY_NPC_ENTRY;
DELETE FROM `creature_template` WHERE `entry`=@RIFT_ENTRY_NPC_ENTRY;
INSERT INTO `creature_template` (
  `entry`,`difficulty_entry_1`,`difficulty_entry_2`,`difficulty_entry_3`,`KillCredit1`,`KillCredit2`,
  `name`,`subname`,`IconName`,`gossip_menu_id`,`minlevel`,`maxlevel`,`exp`,`faction`,`npcflag`,`speed_walk`,`speed_run`,
  `speed_swim`,`speed_flight`,`detection_range`,`rank`,`dmgschool`,`DamageModifier`,`BaseAttackTime`,`RangeAttackTime`,
  `BaseVariance`,`RangeVariance`,`unit_class`,`unit_flags`,`unit_flags2`,`dynamicflags`,`family`,`type`,`type_flags`,
  `lootid`,`pickpocketloot`,`skinloot`,`PetSpellDataId`,`VehicleId`,`mingold`,`maxgold`,`AIName`,`MovementType`,
  `HoverHeight`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,`ExperienceModifier`,`RacialLeader`,`movementId`,`RegenHealth`,
  `CreatureImmunitiesId`,`flags_extra`,`ScriptName`,`VerifiedBuild`)
SELECT @RIFT_ENTRY_NPC_ENTRY,0,0,0,0,0,'五人英雄裂隙引导者','选择裂隙难度',src.IconName,0,
  src.minlevel,src.maxlevel,src.exp,src.faction,1,src.speed_walk,src.speed_run,src.speed_swim,src.speed_flight,
  src.detection_range,0,src.dmgschool,1,src.BaseAttackTime,src.RangeAttackTime,src.BaseVariance,src.RangeVariance,
  src.unit_class,src.unit_flags,src.unit_flags2,0,src.family,src.type,src.type_flags,0,0,0,0,0,0,0,'',0,
  src.HoverHeight,1,1,1,1,0,0,1,0,src.flags_extra,'npc_rift_entry',src.VerifiedBuild
FROM `creature_template` src WHERE src.entry=@RIFT_ENTRY_NPC_SOURCE_ENTRY;
INSERT INTO `creature_template_model`
SELECT @RIFT_ENTRY_NPC_ENTRY,src.Idx,src.CreatureDisplayID,src.DisplayScale,src.Probability,src.VerifiedBuild
FROM `creature_template_model` src WHERE src.CreatureID=@RIFT_ENTRY_NPC_SOURCE_ENTRY;

-- ============================================================================
-- 8. 裂隙专用法术脚本绑定
-- ============================================================================
-- 父Aura触发子法术时，父法术BasePoint不会自动传给子法术；以下脚本只在裂隙Boss施放时接管伤害。
DELETE FROM `spell_script_names`
WHERE (`spell_id`=15790 AND `ScriptName`='spell_rift_doan_arcane_missiles')
   OR (`spell_id`=16247 AND `ScriptName`='spell_rift_lethtendris_curse_of_thorns');
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(15790,'spell_rift_doan_arcane_missiles'),
(16247,'spell_rift_lethtendris_curse_of_thorns');

-- ============================================================================
-- 9. 审核查询
-- ============================================================================
SELECT `entry`,`name`,`minlevel`,`maxlevel`,`unit_flags`,`unit_flags2`,`flags_extra`,
       `AIName`,`ScriptName`,`HealthModifier`,`DamageModifier`
FROM `creature_template`
WHERE (`entry` = @RIFT_ENTRY_NPC_ENTRY)
   OR (`entry` BETWEEN @RIFT_BOSS_ENTRY_BASE + 0 AND @RIFT_BOSS_ENTRY_BASE + 140)
   OR (`entry` BETWEEN @RIFT_SUMMON_ENTRY_BASE + 0 AND @RIFT_SUMMON_ENTRY_BASE + 33)
ORDER BY `entry`;
SELECT * FROM `heroic_dungeon_rift_boss` ORDER BY `boss_id`;
SELECT * FROM `heroic_dungeon_rift_boss_tier` ORDER BY `boss_id`,`tier`;
SELECT `entry`,`type`,`displayId`,`name`,`ScriptName` FROM `gameobject_template`
WHERE `entry` IN (@RIFT_EXIT_PORTAL_ENTRY_BASE + 0,@RIFT_EXIT_PORTAL_ENTRY_BASE + 1,@RIFT_EXIT_PORTAL_ENTRY_BASE + 2)
ORDER BY `entry`;

DROP TEMPORARY TABLE IF EXISTS `_rift_template_map`;
DROP TEMPORARY TABLE IF EXISTS `_rift_summon_map`;
DROP TEMPORARY TABLE IF EXISTS `_rift_boss_map`;
