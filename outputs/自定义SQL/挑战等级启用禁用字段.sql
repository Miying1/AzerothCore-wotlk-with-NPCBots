-- =============================================================
-- 给 zone_difficulty_level 表增加 status 字段
-- 用于控制每一个挑战等级的启用 / 禁用
--   1 = 启用（默认，原有等级全部启用）
--   0 = 禁用（禁用后该等级不会在副本 NPC 中显示，也无法开启）
-- 执行后重启世界服务（或执行 .reload config）即可生效
-- =============================================================

ALTER TABLE `zone_difficulty_level`
    ADD COLUMN `status` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '启用状态 1=启用 0=禁用' AFTER `award3`;

-- 示例：禁用某个挑战等级（例如禁用 difflevel=5）
-- UPDATE `zone_difficulty_level` SET `status` = 0 WHERE `difflevel` = 5;
