-- 幻形缩放系数：佣兵幻形表新增 scale_factor 列；新增玩家变形缩放设置表
ALTER TABLE `mod_player_bot_transmog`
  ADD COLUMN IF NOT EXISTS `scale_factor` FLOAT NOT NULL DEFAULT 1 AFTER `model_name`;

CREATE TABLE IF NOT EXISTS `mod_player_transmog_set` (
  `character_id` INT UNSIGNED NOT NULL COMMENT '角色GUID低32位',
  `scale_factor` FLOAT NOT NULL DEFAULT 1 COMMENT '玩家变形缩放系数(0.5-1.5,默认1)',
  PRIMARY KEY (`character_id`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic COMMENT = '玩家变形缩放设置';
