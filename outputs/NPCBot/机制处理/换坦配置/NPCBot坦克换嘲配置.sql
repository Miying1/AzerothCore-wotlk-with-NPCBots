CREATE TABLE IF NOT EXISTS `npcbot_tank_swap` (
  `map_id` SMALLINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '地图ID，0表示所有地图',
  `boss_entry` INT UNSIGNED NOT NULL COMMENT 'BOSS生物Entry（难度结构副本填难度Entry）',
  `spell_id` INT UNSIGNED NOT NULL COMMENT 'BOSS施加在坦克身上的换嘲Debuff光环ID（随难度映射）',
  `aura_stacks` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '触发换嘲所需BUFF层数',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '是否启用',
  `comment` VARCHAR(255) NOT NULL DEFAULT '' COMMENT '配置说明',
  PRIMARY KEY (`map_id`, `boss_entry`, `spell_id`),
  KEY `idx_boss_entry` (`boss_entry`),
  KEY `idx_spell_id` (`spell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='NPCBot坦克换嘲配置';

-- =============================================================================
-- 奥杜尔（map 603，10/25 人按 entry 区分，各自无难度映射）
-- =============================================================================
DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 603 AND `boss_entry` = 32930 AND `spell_id` = 63355;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(603, 32930, 63355, 1, 1, '奥杜尔：柯洛刚恩(10) 压碎护甲 降护甲-21% 1层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 603 AND `boss_entry` = 33909 AND `spell_id` = 64002;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(603, 33909, 64002, 2, 1, '奥杜尔：柯洛刚恩(25) 压碎护甲 降护甲-26%可叠加 2层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 603 AND `boss_entry` = 32871 AND `spell_id` = 64392;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(603, 32871, 64392, 1, 1, '奥杜尔：观察者艾尔加隆(10) 相位冲击 暗影易伤+99% 1层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 603 AND `boss_entry` = 33070 AND `spell_id` = 64679;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(603, 33070, 64679, 1, 1, '奥杜尔：观察者艾尔加隆(25) 相位冲击 暗影易伤+99% 1层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 603 AND `boss_entry` = 33288 AND `spell_id` = 63612;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(603, 33288, 63612, 2, 1, '奥杜尔：尤格萨伦(10) P1 闪电烙印 自然易伤+39% 2层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 603 AND `boss_entry` = 33955 AND `spell_id` = 63673;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(603, 33955, 63673, 2, 1, '奥杜尔：尤格萨伦(25) P1 闪电烙印 自然易伤+39% 2层换嘲');

-- =============================================================================
-- 冰冠堡垒（map 631，4 难度结构：10N/25N/10H/25H）
-- =============================================================================
-- 巫妖王 灵魂收割：debuff 随难度映射 69409/73797/73798/73799
DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 36597 AND `spell_id` = 69409;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 36597, 69409, 1, 1, '冰冠堡垒：巫妖王(10N) 灵魂收割 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 39166 AND `spell_id` = 73797;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 39166, 73797, 1, 1, '冰冠堡垒：巫妖王(25N) 灵魂收割 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 39167 AND `spell_id` = 73798;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 39167, 73798, 1, 1, '冰冠堡垒：巫妖王(10H) 灵魂收割 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 39168 AND `spell_id` = 73799;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 39168, 73799, 1, 1, '冰冠堡垒：巫妖王(25H) 灵魂收割 命中即换嘲');

-- 萨鲁法尔 符文之血：72410 各难度共用
DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 37813 AND `spell_id` = 72410;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 37813, 72410, 1, 1, '冰冠堡垒：萨鲁法尔(10N) 符文之血 吸血标签 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 38402 AND `spell_id` = 72410;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 38402, 72410, 1, 1, '冰冠堡垒：萨鲁法尔(25N) 符文之血 吸血标签 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 38582 AND `spell_id` = 72410;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 38582, 72410, 1, 1, '冰冠堡垒：萨鲁法尔(10H) 符文之血 吸血标签 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 38583 AND `spell_id` = 72410;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 38583, 72410, 1, 1, '冰冠堡垒：萨鲁法尔(25H) 符文之血 吸血标签 命中即换嘲');

-- 辛德拉苟莎 冰霜吐息：debuff 随难度映射 69649/71056/71057/71058
DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 36853 AND `spell_id` = 69649;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 36853, 69649, 1, 1, '冰冠堡垒：辛德拉苟莎(10N) 冰霜吐息 降攻速 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 38265 AND `spell_id` = 71056;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 38265, 71056, 1, 1, '冰冠堡垒：辛德拉苟莎(25N) 冰霜吐息 降攻速 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 38266 AND `spell_id` = 71057;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 38266, 71057, 1, 1, '冰冠堡垒：辛德拉苟莎(10H) 冰霜吐息 降攻速 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 38267 AND `spell_id` = 71058;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 38267, 71058, 1, 1, '冰冠堡垒：辛德拉苟莎(25H) 冰霜吐息 降攻速 命中即换嘲');

-- 脓肠 胃胀气：debuff 随难度映射 72219/72551/72552/72553
DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 36626 AND `spell_id` = 72219;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 36626, 72219, 9, 1, '冰冠堡垒：脓肠(10N) 胃胀气 持续100秒 10层爆炸 9层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 37504 AND `spell_id` = 72551;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 37504, 72551, 9, 1, '冰冠堡垒：脓肠(25N) 胃胀气 持续100秒 10层爆炸 9层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 37505 AND `spell_id` = 72552;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 37505, 72552, 9, 1, '冰冠堡垒：脓肠(10H) 胃胀气 持续100秒 10层爆炸 9层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 37506 AND `spell_id` = 72553;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 37506, 72553, 9, 1, '冰冠堡垒：脓肠(25H) 胃胀气 持续100秒 10层爆炸 9层换嘲');

-- 教授 畸变瘟疫：debuff 随难度映射 72451/72463/72671/72672
DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 36678 AND `spell_id` = 72451;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 36678, 72451, 2, 1, '冰冠堡垒：普崔希德教授(10N) 畸变瘟疫 全团DOT每层x3 2层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 38431 AND `spell_id` = 72463;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 38431, 72463, 2, 1, '冰冠堡垒：普崔希德教授(25N) 畸变瘟疫 全团DOT每层x3 2层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 38585 AND `spell_id` = 72671;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 38585, 72671, 2, 1, '冰冠堡垒：普崔希德教授(10H) 畸变瘟疫 全团DOT每层x3 2层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 631 AND `boss_entry` = 38586 AND `spell_id` = 72672;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(631, 38586, 72672, 2, 1, '冰冠堡垒：普崔希德教授(25H) 畸变瘟疫 全团DOT每层x3 2层换嘲');

-- =============================================================================
-- 卡拉赞（map 532，10 人唯一难度）
-- =============================================================================
DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 532 AND `boss_entry` = 15690 AND `spell_id` = 30901;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(532, 15690, 30901, 3, 1, '卡拉赞：莫克札王子 破甲攻击 降护甲可叠5层 3层换嘲');

-- =============================================================================
-- 十字军的试炼（map 649，4 难度结构：10N/25N/10H/25H）
-- =============================================================================
-- 戈莫克 穿刺：debuff 随难度映射 66331/67477/67478/67479
DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 649 AND `boss_entry` = 34796 AND `spell_id` = 66331;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(649, 34796, 66331, 3, 1, '十字军试炼：戈莫克(10N) 穿刺 流血DOT 3层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 649 AND `boss_entry` = 35438 AND `spell_id` = 67477;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(649, 35438, 67477, 3, 1, '十字军试炼：戈莫克(25N) 穿刺 流血DOT 3层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 649 AND `boss_entry` = 35439 AND `spell_id` = 67478;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(649, 35439, 67478, 3, 1, '十字军试炼：戈莫克(10H) 穿刺 流血DOT 3层换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 649 AND `boss_entry` = 35440 AND `spell_id` = 67479;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(649, 35440, 67479, 3, 1, '十字军试炼：戈莫克(25H) 穿刺 流血DOT 3层换嘲');

-- 阿努巴拉克 寒冰打击：66012 各难度共用
DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 649 AND `boss_entry` = 34564 AND `spell_id` = 66012;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(649, 34564, 66012, 1, 1, '十字军试炼：阿努巴拉克(10N) 寒冰打击 昏迷冻结坦 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 649 AND `boss_entry` = 34566 AND `spell_id` = 66012;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(649, 34566, 66012, 1, 1, '十字军试炼：阿努巴拉克(25N) 寒冰打击 昏迷冻结坦 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 649 AND `boss_entry` = 35615 AND `spell_id` = 66012;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(649, 35615, 66012, 1, 1, '十字军试炼：阿努巴拉克(10H) 寒冰打击 昏迷冻结坦 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 649 AND `boss_entry` = 35616 AND `spell_id` = 66012;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(649, 35616, 66012, 1, 1, '十字军试炼：阿努巴拉克(25H) 寒冰打击 昏迷冻结坦 命中即换嘲');

-- =============================================================================
-- 祖阿曼（map 568，10 人唯一难度）
-- =============================================================================
DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 568 AND `boss_entry` = 23576 AND `spell_id` = 42389;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(568, 23576, 42389, 1, 1, '祖阿曼：纳罗拉克 裂伤 流血易伤+100% 命中即换嘲');

DELETE FROM `npcbot_tank_swap` WHERE `map_id` = 568 AND `boss_entry` = 23576 AND `spell_id` = 42397;
INSERT INTO `npcbot_tank_swap` (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`) VALUES
(568, 23576, 42397, 1, 1, '祖阿曼：纳罗拉克 撕裂 流血DOT 命中即换嘲');
