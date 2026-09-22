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
(532, 16697, 8, 28865, 2, 1000, '卡拉赞：虚空幽龙的虚空领域');

-- 奥杜尔：米米尔隆硬模式 Flames Spread。
-- 34121 由火焰扩散机制生成，并由生物自身承载 64561 Flames Aura。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 603 AND `creature_entry` = 34121;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(603, 34121, 5.0, 64561, 1.5, 1000, '奥杜尔：米米尔隆硬模式火焰扩散，Flames Aura');

-- 奥杜尔：烈焰巨兽硬模式 Scorched Ground。
-- 33123 由 Boss 技能触发生成，并由生物自身施放 62548 Scorched Ground。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 603 AND `creature_entry` = 33123;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(603, 33123, 8.0, 62548, 1.5, 1000, '奥杜尔：烈焰巨兽硬模式灼热地面，Scorched Ground');

-- 冰冠堡垒：辛达苟萨 Icy Blast。
-- 38223 由 Boss 技能链生成，并由生物自身施放 71380 Icy Blast Area。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 631 AND `creature_entry` = 38223;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(631, 38223, 8.0, 71380, 1.5, 500, '冰冠堡垒：辛达苟萨寒冰冲击区域，Icy Blast Area');

-- 奥杜尔：科拉隆凝视之眼（左眼 33632 / 右眼 33802）。
-- 眼睛由 63342 召唤后 MoveChase 追人被点名玩家，向前方发射射线 63676/63702。
-- 射线为定向光束（目标 TARGET_DEST_CASTER_FRONT，半径 0），无法读取圆形半径，
-- 故用固定 radius 让 BOT 远离眼睛本体，危险圈随眼睛实时位置移动。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 603 AND `creature_entry` IN (33632, 33802);

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(603, 33632, 10.0, 0, 1.0, 500, '奥杜尔：科拉隆凝视之眼（左眼，射线追人）'),
(603, 33802, 10.0, 0, 1.0, 500, '奥杜尔：科拉隆凝视之眼（右眼，射线追人）');

-- 奥杜尔：芙蕾雅自然炸弹（34129）。
-- 芙蕾雅对随机玩家施放 64648 后，在目标位置召唤 34129，约 11 秒后施放 64587 范围爆炸伤害。
-- 生物固定于落点，属「定时爆炸地板」，BOT 应远离炸弹落点。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 603 AND `creature_entry` = 34129;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(603, 34129, 10.0, 64587, 1.0, 0, '奥杜尔：芙蕾雅自然炸弹（定时爆炸地板）');

-- 奥杜尔：奥尔加隆黑洞（32953）。
-- 由崩塌星死亡后生成，黑洞本体持续对周围施放 62169 周期伤害（EffectAura=周期伤害，无伤害半径），
-- 固定位置持续地板，BOT 应远离。damage_spell_id 读不到半径，故以固定 radius 兜底。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 603 AND `creature_entry` = 32953;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(603, 32953, 8.0, 62169, 1.0, 500, '奥杜尔：奥尔加隆黑洞（持续地板）');

-- 奥杜尔：奥尔加隆虚空带（34100）。
-- 由 64470 召唤的环境伤害 stalker，固定位置持续对范围内造成 28（环境伤害）。
-- 64070 半径由法术读取，radius 作为保底下限。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 603 AND `creature_entry` = 34100;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(603, 34100, 10.0, 64470, 1.0, 500, '奥杜尔：奥尔加隆虚空带（环境伤害地板）');

-- 奥杜尔：米米尔隆感应地雷（34362）。
-- 米米尔隆 P2 布设的地雷，靠近约 1.9 码触发 66351 范围爆炸，固定位置。
-- BOT 应远离地雷本体，避免踩踏引爆。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 603 AND `creature_entry` = 34362;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(603, 34362, 3.0, 0, 1.0, 500, '奥杜尔：米米尔隆感应地雷（踩踏引爆）');

-- 奥杜尔：烈焰巨兽本体（33113）。
-- 一号 Boss 为载具战，烈焰巨兽会追人碾压（SPELL_PURSUED 62374）并施放火焰喷射（62396）。
-- BOT 在载具战中步行，需远离烈焰巨兽本体，避免被追人碾压。
-- 载具本体为危险源，危险圈随其移动实时更新。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 603 AND `creature_entry` = 33113;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(603, 33113, 15.0, 0, 1.0, 500, '奥杜尔：烈焰巨兽本体（追人碾压，载具战）');

-- ============================================================================
-- 十字军试炼（Trial of the Crusader，map 649）
-- ============================================================================
-- 注：酸喉粘液池 35176、戈莫克火焰炸弹 34854 已在 bot_ai.cpp 的 CalculateAoeSpots
-- 硬编码逻辑中处理（map 649 块），无需在此配置。以下补充硬编码未覆盖的技能。

-- 加拉克苏斯大王：军团烈焰（34784）。
-- 加拉克苏斯点名玩家施放 66197，命中后 66200 在目标位置召唤军团烈焰生物 34784，
-- 该生物自带 66201 周期伤害光环，形成固定位置持续地面火焰。
-- 伤害法术 66201 无有效半径（EffectAura=周期触发），故用固定 radius。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 649 AND `creature_entry` = 34784;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(649, 34784, 8.0, 0, 1.0, 1000, 'TOC：加拉克苏斯军团烈焰（落地持续火焰）');

-- 加拉克苏斯大王：地狱火火山（34813）。
-- 加拉克苏斯施放 66258 召唤地狱火火山生物 34813（固定位置），
-- 火山周期性喷发地狱火火球造成范围伤害，属固定地板危险区。
-- 召唤法术 66258 本身有半径（RadiusIndex=18），火山伤害由生物周期触发。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 649 AND `creature_entry` = 34813;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(649, 34813, 10.0, 0, 1.0, 1000, 'TOC：加拉克苏斯地狱火火山（固定喷发地板）');

-- 阿努巴拉克：追击尖刺（34660）。
-- 钻地后 EVENT_SPELL_SUMMON_SPIKE 用 66169 召唤，尖刺 MoveChase 追击被点名真实玩家，
-- 每次追到时由 spell_pursuing_spikes_aura 周期施放 65919 穿刺（IMPALE，范围伤害）。
-- 尖刺本体为移动的威胁源，危险圈随其实时位置移动，BOT 应提前躲开追人路线及其穿刺范围。
-- 65919 为 SCHOOL_DAMAGE（有半径），damage_spell_id 优先读半径，读不到时用固定 radius 兜底。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 649 AND `creature_entry` = 34660;

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(649, 34660, 10.0, 0, 1.0, 0, 'TOC：阿努巴拉克追击尖刺（追人 + 穿刺，危险圈随尖刺移动）');
