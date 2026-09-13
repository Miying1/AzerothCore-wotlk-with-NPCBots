-- =====================================================================
-- 【恢复 / 开放】撤销 01_关闭ICC与红玉圣殿_随机日常纹章下调.sql 的全部改动
--   * 放在 outputs/进度控制/ 下：不属于 data/sql 的任何自动更新目录
--     （pending_db_world、custom/db_world 都会被 worldserver 自动执行，恢复脚本不能放那里）
--   * 手动执行：mysql -uroot -p acore_world < 02_恢复ICC与红玉圣殿_随机日常纹章还原.sql
--   * 所有语句幂等，可重复执行
-- 恢复值均按 2026-09-13 对 acore_world 的核对结果填写
-- =====================================================================

-- ---------- 一、恢复冰冠堡垒(631) 与 红玉圣殿(724) ----------
-- 按主键(sourceType,entry)删除，不依赖 comment 文本，避免因连接编码导致匹配不上
DELETE FROM `disables` WHERE `sourceType` = 2 AND `entry` IN (631, 724);
UPDATE `dungeon_access_template` SET `min_level` = 80 WHERE `map_id` IN (631, 724) AND `min_level` = 81;

-- ---------- 二、恢复随机日常纹章 ----------
UPDATE `quest_template` SET `RewardItem1` = 49426, `RewardAmount1` = 12 WHERE `ID` = 24788;
UPDATE `quest_template` SET `RewardItem2` = 47241, `RewardAmount2` = 8  WHERE `ID` = 24789;
UPDATE `quest_template` SET `RewardItem1` = 47241, `RewardAmount1` = 15 WHERE `ID` = 24790;
UPDATE `quest_template` SET `RewardItem1` = 47241, `RewardAmount1` = 5  WHERE `ID` = 24791;

-- ---------- 三、恢复寒冰纹章掉落 ----------
DELETE FROM `disables` WHERE `sourceType` = 10 AND `entry` = 49426;

-- ---------- 四、恢复被剥离的寒冰纹章任务奖励（20 个，原值已核对）----------
-- 槽位2 的 47241 当时未被改动，无需恢复
UPDATE `quest_template` SET `RewardItem1` = 49426, `RewardAmount1` = 2  WHERE `ID` IN (24499, 24500, 24511, 24710, 24712, 24802);
UPDATE `quest_template` SET `RewardItem1` = 49426, `RewardAmount1` = 5  WHERE `ID` BETWEEN 24579 AND 24590;
UPDATE `quest_template` SET `RewardItem1` = 49426, `RewardAmount1` = 5  WHERE `ID` = 26034;
UPDATE `quest_template` SET `RewardItem1` = 49426, `RewardAmount1` = 10 WHERE `ID` = 65000;

-- ---------- 五、恢复寒冰纹章军需官 ----------
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 23 AND `SourceGroup` IN (37941, 37942, 38858);
