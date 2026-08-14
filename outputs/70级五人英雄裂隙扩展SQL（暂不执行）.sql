-- 70级五人英雄裂隙扩展SQL（暂不执行）
-- 适用：已经安装60级裂隙基础内容，仅追加燃烧的远征22个Boss。
-- ID与完整SQL使用相同全局基数：boss_id 48-69；Boss Entry 100142-100207；召唤物 Entry 102034-102058。
-- 玩家传入点来自 outputs/英雄裂隙boss传入点_整合.xlsx 的“燃烧的远征(70级)”工作表；已回填基础表和Tier表并启用。当前不要执行。

SET @RIFT_BOSS_ID_BASE := 1;
SET @RIFT_BOSS_ENTRY_BASE := 100001;
SET @RIFT_SUMMON_ENTRY_BASE := 102000;
SET @RIFT_LEVEL70_BOSS_ID_OFFSET := 47;
SET @RIFT_LEVEL70_BOSS_ENTRY_OFFSET := 141;
SET @RIFT_LEVEL70_SUMMON_ENTRY_OFFSET := 34;

SET @RIFT_T1_HEALTH := 1.0000;
SET @RIFT_T1_DAMAGE := 1.0000;
SET @RIFT_T2_HEALTH := 1.5000;
SET @RIFT_T2_DAMAGE := 1.4000;
SET @RIFT_T3_HEALTH := 3.0000;
SET @RIFT_T3_DAMAGE := 2.0000;

SET @RIFT_BOSS_HM_MELEE := 129;
SET @RIFT_BOSS_HM_CASTER := 115;
SET @RIFT_BOSS_DAMAGE_MODIFIER := 35.0000;
SET @RIFT_SUMMON_DAMAGE_MODIFIER := 12.0000;
SET @RIFT_BOSS_UNIT_FLAGS_KEEP_MASK := 2113862781;
SET @RIFT_SUMMON_UNIT_FLAGS_KEEP_MASK := 4261412093;

-- 本扩展SQL默认 `heroic_dungeon_rift_boss` 与 `heroic_dungeon_rift_boss_tier` 已由60级基础SQL创建，
-- 不在此重复创建或修改正式配置表结构。

-- ============================================================================
-- 1. 70级Boss映射
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
(@RIFT_BOSS_ID_BASE + 47,16807,@RIFT_BOSS_ENTRY_BASE + 141,1,'boss_rift_nethekurse','破碎大厅',540,172.663,289.61,-8.11541,6.01108,157.76,246.38,-13.20,0.98,'高阶术士奈瑟库斯；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 47,16807,@RIFT_BOSS_ENTRY_BASE + 142,2,'boss_rift_nethekurse','破碎大厅',540,172.663,289.61,-8.11541,6.01108,157.76,246.38,-13.20,0.98,'高阶术士奈瑟库斯；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 47,16807,@RIFT_BOSS_ENTRY_BASE + 143,3,'boss_rift_nethekurse','破碎大厅',540,172.663,289.61,-8.11541,6.01108,157.76,246.38,-13.20,0.98,'高阶术士奈瑟库斯；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 48,16808,@RIFT_BOSS_ENTRY_BASE + 144,1,'boss_rift_kargath','破碎大厅',540,231.25,-83.6449,5.02341,6.26573,303.65,-85.39,1.93,3.06,'酋长卡加斯·刃拳；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 48,16808,@RIFT_BOSS_ENTRY_BASE + 145,2,'boss_rift_kargath','破碎大厅',540,231.25,-83.6449,5.02341,6.26573,303.65,-85.39,1.93,3.06,'酋长卡加斯·刃拳；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 48,16808,@RIFT_BOSS_ENTRY_BASE + 146,3,'boss_rift_kargath','破碎大厅',540,231.25,-83.6449,5.02341,6.26573,303.65,-85.39,1.93,3.06,'酋长卡加斯·刃拳；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 49,17380,@RIFT_BOSS_ENTRY_BASE + 147,1,'boss_rift_broggok','鲜血熔炉',542,455.336,-1.82919,9.6299,1.43117,455.65,65.71,9.61,4.66,'布洛戈克；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 49,17380,@RIFT_BOSS_ENTRY_BASE + 148,2,'boss_rift_broggok','鲜血熔炉',542,455.336,-1.82919,9.6299,1.43117,455.65,65.71,9.61,4.66,'布洛戈克；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 49,17380,@RIFT_BOSS_ENTRY_BASE + 149,3,'boss_rift_broggok','鲜血熔炉',542,455.336,-1.82919,9.6299,1.43117,455.65,65.71,9.61,4.66,'布洛戈克；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 50,17377,@RIFT_BOSS_ENTRY_BASE + 150,1,'boss_rift_kelidan','鲜血熔炉',542,326.503,-86.0028,-24.577,3.59538,326.82,-151.94,-25.49,1.53,'击碎者克里丹；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 50,17377,@RIFT_BOSS_ENTRY_BASE + 151,2,'boss_rift_kelidan','鲜血熔炉',542,326.503,-86.0028,-24.577,3.59538,326.82,-151.94,-25.49,1.53,'击碎者克里丹；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 50,17377,@RIFT_BOSS_ENTRY_BASE + 152,3,'boss_rift_kelidan','鲜血熔炉',542,326.503,-86.0028,-24.577,3.59538,326.82,-151.94,-25.49,1.53,'击碎者克里丹；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 51,17308,@RIFT_BOSS_ENTRY_BASE + 153,1,'boss_rift_omor','地狱火城墙',543,-1122.34,1718.41,89.4315,3.75246,-1183.01,1686.34,90.96,0.48,'无疤者奥摩尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 51,17308,@RIFT_BOSS_ENTRY_BASE + 154,2,'boss_rift_omor','地狱火城墙',543,-1122.34,1718.41,89.4315,3.75246,-1183.01,1686.34,90.96,0.48,'无疤者奥摩尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 51,17308,@RIFT_BOSS_ENTRY_BASE + 155,3,'boss_rift_omor','地狱火城墙',543,-1122.34,1718.41,89.4315,3.75246,-1183.01,1686.34,90.96,0.48,'无疤者奥摩尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 52,17798,@RIFT_BOSS_ENTRY_BASE + 156,1,'boss_rift_kalithresh','蒸汽地窟',545,-95.4188,-552.031,8.27027,1.44862,-95.66,-485.22,8.20,4.72,'督军卡利瑟里斯；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 52,17798,@RIFT_BOSS_ENTRY_BASE + 157,2,'boss_rift_kalithresh','蒸汽地窟',545,-95.4188,-552.031,8.27027,1.44862,-95.66,-485.22,8.20,4.72,'督军卡利瑟里斯；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 52,17798,@RIFT_BOSS_ENTRY_BASE + 158,3,'boss_rift_kalithresh','蒸汽地窟',545,-95.4188,-552.031,8.27027,1.44862,-95.66,-485.22,8.20,4.72,'督军卡利瑟里斯；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 53,17826,@RIFT_BOSS_ENTRY_BASE + 159,1,'boss_rift_muselek','幽暗沼泽',546,288.582,-121.831,29.7033,6.07879,241.33,-155.12,27.01,0.61,'沼地领主穆塞雷克；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 53,17826,@RIFT_BOSS_ENTRY_BASE + 160,2,'boss_rift_muselek','幽暗沼泽',546,288.582,-121.831,29.7033,6.07879,241.33,-155.12,27.01,0.61,'沼地领主穆塞雷克；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 53,17826,@RIFT_BOSS_ENTRY_BASE + 161,3,'boss_rift_muselek','幽暗沼泽',546,288.582,-121.831,29.7033,6.07879,241.33,-155.12,27.01,0.61,'沼地领主穆塞雷克；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 54,17882,@RIFT_BOSS_ENTRY_BASE + 162,1,'boss_rift_black_stalker','幽暗沼泽',546,143.432,9.12584,27.6458,4.2237,210.83,-1.16,27.56,2.69,'黑色阔步者；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 54,17882,@RIFT_BOSS_ENTRY_BASE + 163,2,'boss_rift_black_stalker','幽暗沼泽',546,143.432,9.12584,27.6458,4.2237,210.83,-1.16,27.56,2.69,'黑色阔步者；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 54,17882,@RIFT_BOSS_ENTRY_BASE + 164,3,'boss_rift_black_stalker','幽暗沼泽',546,143.432,9.12584,27.6458,4.2237,210.83,-1.16,27.56,2.69,'黑色阔步者；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 55,17941,@RIFT_BOSS_ENTRY_BASE + 165,1,'boss_rift_mennu','奴隶围栏',547,49.4763,-380.219,3.03558,3.17405,81.16,-328.18,3.04,5.11,'背叛者门努；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 55,17941,@RIFT_BOSS_ENTRY_BASE + 166,2,'boss_rift_mennu','奴隶围栏',547,49.4763,-380.219,3.03558,3.17405,81.16,-328.18,3.04,5.11,'背叛者门努；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 55,17941,@RIFT_BOSS_ENTRY_BASE + 167,3,'boss_rift_mennu','奴隶围栏',547,49.4763,-380.219,3.03558,3.17405,81.16,-328.18,3.04,5.11,'背叛者门努；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 56,17942,@RIFT_BOSS_ENTRY_BASE + 168,1,'boss_rift_quagmirran','奴隶围栏',547,-281.096,-667.138,9.40212,5.84685,-149.10,-741.73,37.89,2.51,'夸格米拉；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 56,17942,@RIFT_BOSS_ENTRY_BASE + 169,2,'boss_rift_quagmirran','奴隶围栏',547,-281.096,-667.138,9.40212,5.84685,-149.10,-741.73,37.89,2.51,'夸格米拉；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 56,17942,@RIFT_BOSS_ENTRY_BASE + 170,3,'boss_rift_quagmirran','奴隶围栏',547,-281.096,-667.138,9.40212,5.84685,-149.10,-741.73,37.89,2.51,'夸格米拉；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 57,20885,@RIFT_BOSS_ENTRY_BASE + 171,1,'boss_rift_dalliah','禁魔监狱',552,137.234,128.506,22.5245,1.01229,148.05,146.99,20.90,6.26,'末日预言者达尔莉安；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 57,20885,@RIFT_BOSS_ENTRY_BASE + 172,2,'boss_rift_dalliah','禁魔监狱',552,137.234,128.506,22.5245,1.01229,148.05,146.99,20.90,6.26,'末日预言者达尔莉安；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 57,20885,@RIFT_BOSS_ENTRY_BASE + 173,3,'boss_rift_dalliah','禁魔监狱',552,137.234,128.506,22.5245,1.01229,148.05,146.99,20.90,6.26,'末日预言者达尔莉安；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 58,20886,@RIFT_BOSS_ENTRY_BASE + 174,1,'boss_rift_soccothrates','禁魔监狱',552,136.2,168.31,22.5245,5.23599,122.10,192.72,22.44,4.67,'天怒预言者苏克拉底；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 58,20886,@RIFT_BOSS_ENTRY_BASE + 175,2,'boss_rift_soccothrates','禁魔监狱',552,136.2,168.31,22.5245,5.23599,122.10,192.72,22.44,4.67,'天怒预言者苏克拉底；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 58,20886,@RIFT_BOSS_ENTRY_BASE + 176,3,'boss_rift_soccothrates','禁魔监狱',552,136.2,168.31,22.5245,5.23599,122.10,192.72,22.44,4.67,'天怒预言者苏克拉底；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 59,17980,@RIFT_BOSS_ENTRY_BASE + 177,1,'boss_rift_laj','生态船',553,-204.125,391.249,-11.1943,0.0174533,-164.17,441.88,-17.82,4.28,'拉伊；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 59,17980,@RIFT_BOSS_ENTRY_BASE + 178,2,'boss_rift_laj','生态船',553,-204.125,391.249,-11.1943,0.0174533,-164.17,441.88,-17.82,4.28,'拉伊；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 59,17980,@RIFT_BOSS_ENTRY_BASE + 179,3,'boss_rift_laj','生态船',553,-204.125,391.249,-11.1943,0.0174533,-164.17,441.88,-17.82,4.28,'拉伊；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 60,17977,@RIFT_BOSS_ENTRY_BASE + 180,1,'boss_rift_warp_splinter','生态船',553,63.8407,391.882,-27.8938,3.21141,-3.62,391.49,-27.96,0,'迁跃扭木；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 60,17977,@RIFT_BOSS_ENTRY_BASE + 181,2,'boss_rift_warp_splinter','生态船',553,63.8407,391.882,-27.8938,3.21141,-3.62,391.49,-27.96,0,'迁跃扭木；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 60,17977,@RIFT_BOSS_ENTRY_BASE + 182,3,'boss_rift_warp_splinter','生态船',553,63.8407,391.882,-27.8938,3.21141,-3.62,391.49,-27.96,0,'迁跃扭木；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 61,19220,@RIFT_BOSS_ENTRY_BASE + 183,1,'boss_rift_pathaleon','能源舰',554,139.542,149.319,25.659,4.59022,138.46,82.39,26.37,1.51,'计算者帕萨雷恩；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 61,19220,@RIFT_BOSS_ENTRY_BASE + 184,2,'boss_rift_pathaleon','能源舰',554,139.542,149.319,25.659,4.59022,138.46,82.39,26.37,1.51,'计算者帕萨雷恩；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 61,19220,@RIFT_BOSS_ENTRY_BASE + 185,3,'boss_rift_pathaleon','能源舰',554,139.542,149.319,25.659,4.59022,138.46,82.39,26.37,1.51,'计算者帕萨雷恩；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 62,18708,@RIFT_BOSS_ENTRY_BASE + 186,1,'boss_rift_murmur','暗影迷宫',555,-157.895,-497.322,15.8651,1.5625,-157.90,-497.32,15.87,1.56,'摩摩尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 62,18708,@RIFT_BOSS_ENTRY_BASE + 187,2,'boss_rift_murmur','暗影迷宫',555,-157.895,-497.322,15.8651,1.5625,-157.90,-497.32,15.87,1.56,'摩摩尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 62,18708,@RIFT_BOSS_ENTRY_BASE + 188,3,'boss_rift_murmur','暗影迷宫',555,-157.895,-497.322,15.8651,1.5625,-157.90,-497.32,15.87,1.56,'摩摩尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 63,18473,@RIFT_BOSS_ENTRY_BASE + 189,1,'boss_rift_ikiss','塞泰克大厅',556,44.7227,286.96,25.1521,3.97935,-37.84,286.96,26.73,6.25,'利爪之王艾吉斯；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 63,18473,@RIFT_BOSS_ENTRY_BASE + 190,2,'boss_rift_ikiss','塞泰克大厅',556,44.7227,286.96,25.1521,3.97935,-37.84,286.96,26.73,6.25,'利爪之王艾吉斯；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 63,18473,@RIFT_BOSS_ENTRY_BASE + 191,3,'boss_rift_ikiss','塞泰克大厅',556,44.7227,286.96,25.1521,3.97935,-37.84,286.96,26.73,6.25,'利爪之王艾吉斯；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 64,18344,@RIFT_BOSS_ENTRY_BASE + 192,1,'boss_rift_shaffar','法力陵墓',557,-184.366,9.33347,16.8174,2.94961,-228.38,46.85,21.10,5.32,'节点亲王沙法尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 64,18344,@RIFT_BOSS_ENTRY_BASE + 193,2,'boss_rift_shaffar','法力陵墓',557,-184.366,9.33347,16.8174,2.94961,-228.38,46.85,21.10,5.32,'节点亲王沙法尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 64,18344,@RIFT_BOSS_ENTRY_BASE + 194,3,'boss_rift_shaffar','法力陵墓',557,-184.366,9.33347,16.8174,2.94961,-228.38,46.85,21.10,5.32,'节点亲王沙法尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 65,18371,@RIFT_BOSS_ENTRY_BASE + 195,1,'boss_rift_shirrak','奥金尼地穴',558,-50.9133,-163.133,26.3687,0.0185413,39.09,-163.50,14.80,3.13,'死亡观察者希尔拉克；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 65,18371,@RIFT_BOSS_ENTRY_BASE + 196,2,'boss_rift_shirrak','奥金尼地穴',558,-50.9133,-163.133,26.3687,0.0185413,39.09,-163.50,14.80,3.13,'死亡观察者希尔拉克；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 65,18371,@RIFT_BOSS_ENTRY_BASE + 197,3,'boss_rift_shirrak','奥金尼地穴',558,-50.9133,-163.133,26.3687,0.0185413,39.09,-163.50,14.80,3.13,'死亡观察者希尔拉克；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 66,18373,@RIFT_BOSS_ENTRY_BASE + 198,1,'boss_rift_maladaar','奥金尼地穴',558,68.1311,-387.821,26.5891,3.18791,24.40,-425.30,30.71,0.77,'大主教玛拉达尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 66,18373,@RIFT_BOSS_ENTRY_BASE + 199,2,'boss_rift_maladaar','奥金尼地穴',558,68.1311,-387.821,26.5891,3.18791,24.40,-425.30,30.71,0.77,'大主教玛拉达尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 66,18373,@RIFT_BOSS_ENTRY_BASE + 200,3,'boss_rift_maladaar','奥金尼地穴',558,68.1311,-387.821,26.5891,3.18791,24.40,-425.30,30.71,0.77,'大主教玛拉达尔；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 67,24664,@RIFT_BOSS_ENTRY_BASE + 201,1,'boss_rift_kaelthas','魔导师平台',585,148.549,186.981,-16.6441,4.74729,149.56,120.97,-14.38,1.54,'凯尔萨斯·逐日者；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 67,24664,@RIFT_BOSS_ENTRY_BASE + 202,2,'boss_rift_kaelthas','魔导师平台',585,148.549,186.981,-16.6441,4.74729,149.56,120.97,-14.38,1.54,'凯尔萨斯·逐日者；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 67,24664,@RIFT_BOSS_ENTRY_BASE + 203,3,'boss_rift_kaelthas','魔导师平台',585,148.549,186.981,-16.6441,4.74729,149.56,120.97,-14.38,1.54,'凯尔萨斯·逐日者；玩家传入点来自整合表格'),
(@RIFT_BOSS_ID_BASE + 68,17537,@RIFT_BOSS_ENTRY_BASE + 204,1,'boss_rift_vazruden','地狱火城墙',543,-1406.5,1746.5,81.2,5.46,-1183.01,1686.34,90.96,0.48,'维斯路登/纳杉脚本召唤遭遇；玩家传入点沿用整合表格传令官瓦兹德行'),
(@RIFT_BOSS_ID_BASE + 68,17537,@RIFT_BOSS_ENTRY_BASE + 205,2,'boss_rift_vazruden','地狱火城墙',543,-1406.5,1746.5,81.2,5.46,-1183.01,1686.34,90.96,0.48,'维斯路登/纳杉脚本召唤遭遇；玩家传入点沿用整合表格传令官瓦兹德行'),
(@RIFT_BOSS_ID_BASE + 68,17537,@RIFT_BOSS_ENTRY_BASE + 206,3,'boss_rift_vazruden','地狱火城墙',543,-1406.5,1746.5,81.2,5.46,-1183.01,1686.34,90.96,0.48,'维斯路登/纳杉脚本召唤遭遇；玩家传入点沿用整合表格传令官瓦兹德行');

-- ============================================================================
-- 2. 裂隙专用动态同伴和召唤物映射
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
(17695,@RIFT_SUMMON_ENTRY_BASE + 34,16808,'npc_rift_shattered_hand_assassin','裂隙破碎之手刺客'),
(17621,@RIFT_SUMMON_ENTRY_BASE + 35,16808,'npc_rift_shattered_hand_heathen','裂隙希顿护卫'),
(17623,@RIFT_SUMMON_ENTRY_BASE + 36,16808,'npc_rift_shattered_hand_reaver','裂隙劫夺者护卫'),
(17622,@RIFT_SUMMON_ENTRY_BASE + 37,16808,'npc_rift_shattered_hand_sharpshooter','裂隙神射手护卫'),
(17662,@RIFT_SUMMON_ENTRY_BASE + 38,17380,'npc_rift_broggok_poison_cloud','裂隙布洛克毒云'),
(17540,@RIFT_SUMMON_ENTRY_BASE + 39,17308,'npc_rift_fiendish_hound','裂隙残忍的军犬'),
(17954,@RIFT_SUMMON_ENTRY_BASE + 40,17798,'npc_rift_naga_distiller','裂隙纳迦蒸馏器'),
(17827,@RIFT_SUMMON_ENTRY_BASE + 41,17826,'npc_rift_claw','裂隙裂爪'),
(22299,@RIFT_SUMMON_ENTRY_BASE + 42,17882,'npc_rift_spore_strider','裂隙孢子行者'),
(20208,@RIFT_SUMMON_ENTRY_BASE + 43,17941,'npc_rift_mennu_healing_ward','裂隙门努治疗图腾'),
(18176,@RIFT_SUMMON_ENTRY_BASE + 44,17941,'npc_rift_mennu_earthgrab_totem','裂隙腐蚀地缚图腾'),
(18177,@RIFT_SUMMON_ENTRY_BASE + 45,17941,'npc_rift_mennu_stoneskin_totem','裂隙腐蚀石肤图腾'),
(18179,@RIFT_SUMMON_ENTRY_BASE + 46,17941,'npc_rift_mennu_nova_totem','裂隙堕落新星图腾'),
(19919,@RIFT_SUMMON_ENTRY_BASE + 47,17980,'npc_rift_thorn_lasher','裂隙荆棘鞭笞者'),
(19920,@RIFT_SUMMON_ENTRY_BASE + 48,17980,'npc_rift_thorn_flayer','裂隙荆棘撕掠者'),
(19949,@RIFT_SUMMON_ENTRY_BASE + 49,17977,'npc_rift_warp_sapling','裂隙树苗'),
(21062,@RIFT_SUMMON_ENTRY_BASE + 50,19220,'npc_rift_nether_wraith','裂隙虚空怨灵'),
(18431,@RIFT_SUMMON_ENTRY_BASE + 51,18344,'npc_rift_ethereal_beacon','裂隙以太信标'),
(18430,@RIFT_SUMMON_ENTRY_BASE + 52,18344,'npc_rift_ethereal_apprentice','裂隙以太学徒'),
(18374,@RIFT_SUMMON_ENTRY_BASE + 53,18371,'npc_rift_focus_fire','裂隙专注之火'),
(18441,@RIFT_SUMMON_ENTRY_BASE + 54,18373,'npc_rift_stolen_soul','裂隙偷取的灵魂'),
(18478,@RIFT_SUMMON_ENTRY_BASE + 55,18373,'npc_rift_martyred_avatar','裂隙马丁瑞德的化身'),
(24674,@RIFT_SUMMON_ENTRY_BASE + 56,24664,'npc_rift_kaelthas_phoenix','裂隙凤凰'),
(24708,@RIFT_SUMMON_ENTRY_BASE + 57,24664,'npc_rift_kaelthas_arcane_sphere','裂隙秘法之球'),
(17536,@RIFT_SUMMON_ENTRY_BASE + 58,17537,'npc_rift_nazan','裂隙纳杉');

DROP TEMPORARY TABLE IF EXISTS `_rift_template_map`;
CREATE TEMPORARY TABLE `_rift_template_map` AS
SELECT `source_entry`, `new_entry`, `script_name`, CONCAT('Rift T', `tier`) AS `name_suffix`, 1 AS `is_boss`, 0 AS `boss_source_entry`
FROM `_rift_boss_map`
UNION ALL
SELECT `source_entry`, `new_entry`, `script_name`, `name_suffix`, 0, `boss_source_entry`
FROM `_rift_summon_map`;

-- ============================================================================
-- 3. 生成70级Boss与召唤物模板
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
-- 4. 写入70级Boss与Tier配置
-- ============================================================================
DELETE FROM `heroic_dungeon_rift_boss_tier`
WHERE `boss_id` BETWEEN @RIFT_BOSS_ID_BASE + 47 AND @RIFT_BOSS_ID_BASE + 68;
DELETE FROM `heroic_dungeon_rift_boss`
WHERE `boss_id` BETWEEN @RIFT_BOSS_ID_BASE + 47 AND @RIFT_BOSS_ID_BASE + 68;

INSERT INTO `heroic_dungeon_rift_boss`
(`boss_id`,`map_name`,`map_id`,`dungeon_version`,`player_entry_x`,`player_entry_y`,`player_entry_z`,`player_entry_o`,`enabled`,`remark`)
SELECT boss_id,MAX(map_name),MAX(map_id),70,MAX(player_x),MAX(player_y),MAX(player_z),MAX(player_o),1,MAX(remark)
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
-- 5. 70级内容审核查询
-- ============================================================================
SELECT `entry`,`name`,`minlevel`,`maxlevel`,`unit_flags`,`unit_flags2`,`flags_extra`,
       `AIName`,`ScriptName`,`HealthModifier`,`DamageModifier`
FROM `creature_template`
WHERE (`entry` BETWEEN @RIFT_BOSS_ENTRY_BASE + 141 AND @RIFT_BOSS_ENTRY_BASE + 206)
   OR (`entry` BETWEEN @RIFT_SUMMON_ENTRY_BASE + 34 AND @RIFT_SUMMON_ENTRY_BASE + 58)
ORDER BY `entry`;
SELECT * FROM `heroic_dungeon_rift_boss`
WHERE `boss_id` BETWEEN @RIFT_BOSS_ID_BASE + 47 AND @RIFT_BOSS_ID_BASE + 68
ORDER BY `boss_id`;
SELECT * FROM `heroic_dungeon_rift_boss_tier`
WHERE `boss_id` BETWEEN @RIFT_BOSS_ID_BASE + 47 AND @RIFT_BOSS_ID_BASE + 68
ORDER BY `boss_id`,`tier`;

DROP TEMPORARY TABLE IF EXISTS `_rift_template_map`;
DROP TEMPORARY TABLE IF EXISTS `_rift_summon_map`;
DROP TEMPORARY TABLE IF EXISTS `_rift_boss_map`;
