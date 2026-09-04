-- NPCBot 生物型危险区域配置：80级副本 Boss 召唤生物圆形危险区域
-- 依据：outputs/NPCBot/机制处理/危险区域配置说明.md
-- 范围：仅配置由 Boss 技能召唤、并由召唤生物承载伤害的 Creature。
-- 半径规则：damage_spell_id 非 0 时由核心优先通过 SpellInfo::CalcRadius() 读取；radius 仅作读取失败时的回退值。

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

-- 奥杜尔：米米尔隆硬模式 Flames Spread。
-- 34121 由火焰扩散机制生成，并由生物自身承载 64561 Flames Aura。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 603 AND `creature_entry` = 34121;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(603, 34121, 5.0, 64561, 1.5, 3000, '奥杜尔：米米尔隆硬模式火焰扩散，Flames Aura');

-- 奥杜尔：烈焰巨兽硬模式 Scorched Ground。
-- 33123 由 Boss 技能触发生成，并由生物自身施放 62548 Scorched Ground。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 603 AND `creature_entry` = 33123;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(603, 33123, 8.0, 62548, 1.5, 3000, '奥杜尔：烈焰巨兽硬模式灼热地面，Scorched Ground');

-- 冰冠堡垒：辛达苟萨 Icy Blast。
-- 38223 由 Boss 技能链生成，并由生物自身施放 71380 Icy Blast Area。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 631 AND `creature_entry` = 38223;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(631, 38223, 8.0, 71380, 1.5, 3000, '冰冠堡垒：辛达苟萨寒冰冲击区域，Icy Blast Area');
