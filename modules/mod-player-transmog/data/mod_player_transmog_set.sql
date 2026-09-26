/*
 * 玩家变形缩放设置表（归属 acore_characters 库）
 *
 * 记录"玩家自身变形"的缩放系数，只对该角色自己的变形生效，不影响佣兵幻形。
 * 核心字段：character_id（角色GUID低32位）、scale_factor（缩放系数，0.5 - 1.5，默认 1）。
 *
 * character_id 主键：一个角色只有一份设置，重复设置为覆盖更新。
 */

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

DROP TABLE IF EXISTS `mod_player_transmog_set`;
CREATE TABLE `mod_player_transmog_set`  (
  `character_id` INT UNSIGNED NOT NULL COMMENT '角色GUID低32位',
  `scale_factor` FLOAT        NOT NULL DEFAULT 1 COMMENT '玩家变形缩放系数(0.5-1.5,默认1)',
  PRIMARY KEY (`character_id`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic COMMENT = '玩家变形缩放设置';

SET FOREIGN_KEY_CHECKS = 1;
