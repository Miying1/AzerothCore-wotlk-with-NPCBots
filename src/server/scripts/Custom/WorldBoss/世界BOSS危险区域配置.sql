-- ============================================================================
-- 世界BOSS 生物型危险区域配置
-- 目标表：npcbot_creature_hazard（NPCBot 生物型危险区域，详见
--   outputs/NPCBot/机制处理/危险区域配置说明.md）
--
-- 说明：
--   世界BOSS 复刻副本首领，其中部分召唤物为「固定生物周期造成范围伤害」的
--   地板型危险区域，NPCBot 依赖本表识别并自动避让。
--
--   map_id = 0：世界BOSS 在世界地图随机刷新，无固定地图，故全地图通用。
--   damage_spell_id 非 0 时，系统会读取该法术的伤害效果半径；
--   radius 为半径下限：与法术半径取较大值（法术半径取不到时即用 radius）。
--   若希望危险区比法术伤害范围更大，直接调大 radius 即可生效。
-- ============================================================================

-- 确保表存在（表结构来自危险区域配置说明.md）
CREATE TABLE IF NOT EXISTS `npcbot_creature_hazard` (
  `map_id` SMALLINT UNSIGNED NOT NULL COMMENT '地图ID，0表示所有地图',
  `creature_entry` INT UNSIGNED NOT NULL COMMENT '危险区域生物Entry',
  `radius` FLOAT UNSIGNED NOT NULL DEFAULT 0 COMMENT '数据库配置的危险半径（回退值）',
  `damage_spell_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '伤害法术ID，非0时优先读取法术效果半径',
  `safety_distance` FLOAT UNSIGNED NOT NULL DEFAULT 0 COMMENT '危险半径外的额外安全距离',
  `deactivation_delay_ms` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '危险源消失后继续保留危险区域的时间（毫秒）',
  `comment` VARCHAR(255) NOT NULL DEFAULT '' COMMENT '配置说明',
  PRIMARY KEY (`map_id`, `creature_entry`),
  KEY `idx_creature_entry` (`creature_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='NPCBot生物型危险区域配置';

-- ============================================================================
-- 危险区域配置
-- ============================================================================

-- 幂等：重复导入本文件时先清掉本项目配置的条目，避免主键冲突导致后续语句中断
DELETE FROM `npcbot_creature_hazard`
WHERE `creature_entry` IN (120501, 120509, 120510, 120516, 23336);

-- 1. 奥（120100）：烈焰之痕 120501
--    俯冲轰炸后留下的地面火焰，周期触发 35380 -> 35383 火焰伤害（DBC 2188-2812）。
INSERT INTO `npcbot_creature_hazard`
    (`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
    (0, 120501, 10, 35383, 2, 2000, '世界BOSS-奥：烈焰之痕（火焰地板）');

-- 2. 苏普雷姆斯（120106）：熔岩拳隐形巡者 120509
--    施放 40980 -> 40253（区域光环，半径约 8 码）-> 40265 熔岩烈焰伤害，持续地面火焰。
INSERT INTO `npcbot_creature_hazard`
    (`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
    (0, 120509, 8, 40265, 2, 2000, '世界BOSS-苏普雷姆斯：熔岩拳隐形巡者（熔岩烈焰地板）');

-- 3. 苏普雷姆斯（120106）：火山 120510
--    施放 40117 -> 42055 -> 42052 火山间歇泉伤害，固定火山持续喷发地板。
INSERT INTO `npcbot_creature_hazard`
    (`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
    (0, 120510, 12, 42052, 2, 2000, '世界BOSS-苏普雷姆斯：火山（间歇泉地板）');

-- 4. 阿克蒙德（120110）：毁灭之火 120516
--    施放 31945 -> 31943（区域光环，半径约 8 码）-> 31944 火焰伤害，持续地面火焰。
INSERT INTO `npcbot_creature_hazard`
    (`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
    (0, 120516, 8, 31944, 2, 2000, '世界BOSS-阿克蒙德：毁灭之火（火焰地板）');

-- 5. 伊利丹（120101）：烈焰碰撞 23336（原版生物）
--    40832（烈焰碰撞）除直伤外还召唤 23336，该生物经 creature_template_addon 自带光环 40836，
--    每 2 秒触发 40841 火焰伤害，形成落点地面火焰；伤害以生物当前位置为圆心，属生物型危险区域。
--    注：23336 为原版生物，黑暗神庙原版伊利丹同样使用，故 map_id = 0 全地图生效。
INSERT INTO `npcbot_creature_hazard`
    (`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
    (0, 23336, 6, 40841, 2, 2000, '世界BOSS-伊利丹：烈焰碰撞（火焰地板，原版生物 23336）');
