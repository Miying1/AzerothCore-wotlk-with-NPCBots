-- =====================================================================
-- 【恢复 / 开放】撤销 03_限制鲜血烘炉映像大厅萨隆矿坑_英雄模式.sql 的全部改动
--   * 放在 outputs/进度控制/ 下：不属于 data/sql 的任何自动更新目录
--   * 手动执行：mysql -uroot -p acore_world < 04_恢复鲜血烘炉映像大厅萨隆矿坑_英雄模式.sql
--   * 所有语句幂等，可重复执行
-- 恢复值均按 2026-09-16 对 acore_world 的核对结果填写
-- =====================================================================

-- ---------- 一、恢复三个副本的英雄(H)模式 ----------
-- 按主键(sourceType,entry)删除，不依赖 comment 文本，避免因连接编码导致匹配不上
DELETE FROM `disables` WHERE `sourceType` = 2 AND `entry` IN (542, 658, 668);

-- 恢复英雄模式(difficulty=1)的准入等级（原值已核对）：
--   542 鲜血烘炉   英雄 min_level = 70
--   658 萨隆矿坑   英雄 min_level = 80
--   668 映像大厅   英雄 min_level = 80
UPDATE `dungeon_access_template` SET `min_level` = 70 WHERE `map_id` = 542 AND `difficulty` = 1 AND `min_level` = 81;
UPDATE `dungeon_access_template` SET `min_level` = 80 WHERE `map_id` IN (658, 668) AND `difficulty` = 1 AND `min_level` = 81;
