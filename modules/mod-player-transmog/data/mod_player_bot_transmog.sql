/*
 * 佣兵幻形数据表（归属 acore_characters 库）
 *
 * 角色用哈哈镜中收集的模型给雇佣的 BOT 变形后持久化在此表。
 * 核心字段：character_id（角色GUID低32位）、bot_entry（BOT的creature entry）、
 *          model_id（模型DisplayId）、model_name（模型名称）。
 *
 * (character_id, bot_entry) 联合主键：同一角色对同一 BOT 只有一份幻形，再次选择为覆盖更新。
 */

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

DROP TABLE IF EXISTS `mod_player_bot_transmog`;
CREATE TABLE `mod_player_bot_transmog`  (
  `character_id` INT UNSIGNED NOT NULL COMMENT '角色GUID低32位',
  `bot_entry`    INT UNSIGNED NOT NULL COMMENT 'BOT的creature entry',
  `model_id`     INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '模型DisplayId',
  `model_name`   VARCHAR(255)  NOT NULL DEFAULT '' COMMENT '模型名称',
  PRIMARY KEY (`character_id`, `bot_entry`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic COMMENT = '佣兵幻形数据';

SET FOREIGN_KEY_CHECKS = 1;
