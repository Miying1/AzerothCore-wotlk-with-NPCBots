CREATE TABLE IF NOT EXISTS `npcbot_creature_hazard` (
  `map_id` SMALLINT UNSIGNED NOT NULL COMMENT '地图ID，0表示所有地图',
  `creature_entry` INT UNSIGNED NOT NULL COMMENT '危险区域生物Entry',
  `radius` FLOAT UNSIGNED NOT NULL DEFAULT 0 COMMENT '固定危险半径及法术半径读取失败时的回退值',
  `damage_spell_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '伤害法术ID，非0时优先读取法术伤害半径',
  `safety_distance` FLOAT UNSIGNED NOT NULL DEFAULT 0 COMMENT '额外安全距离',
  `deactivation_delay_ms` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '危险源消失后继续保留的时间（毫秒）',
  `comment` VARCHAR(255) NOT NULL DEFAULT '' COMMENT '配置说明',
  PRIMARY KEY (`map_id`, `creature_entry`),
  KEY `idx_creature_entry` (`creature_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='NPCBot生物型危险区域配置';

DELETE FROM `npcbot_creature_hazard` WHERE `map_id` = 532 AND `creature_entry` = 16697;
INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(532, 16697, 8, 28865, 2, 3000, '卡拉赞：虚空幽龙的虚空领域');
