-- ============================================================================
-- acore_characters 无用表清理脚本
-- 生成日期: 2026-09-18
-- 目标库  : acore_characters
--
-- 说明:
--   以下 3 张表属于已废弃的旧版幻化模块 (Rochet2 mod-transmog)，
--   全仓库 (src / modules / data/sql / outputs / lua_scripts / addons)
--   均无任何引用，项目也没有任何地方创建它们。
--   仓库中该模块仅剩一个未解压、未编译的压缩包 modules/mod-transmog.rar。
--
-- !!! 重要: 这三张表含玩家历史数据 !!!
--   custom_transmogrification          907 行
--   custom_transmogrification_sets       6 行
--   custom_unlocked_appearances     236365 行  <-- 已解锁外观
--   若将来可能重新启用该旧幻化模块，请勿执行本脚本。
--
-- 执行前务必先备份:
--   mysqldump -uroot -p acore_characters > acore_characters_backup.sql
--
-- 不要删除(项目在用，但建表脚本不在 data/sql 中):
--   characters_npcbot / _gear_storage / _group_member / _stats / _transmog,
--   view_player_botcount,
--   zone_diffculty_activemap, zone_diffculty_playerlevel, zone_difficulty_instance_saves,
--   mod_player_transmog,   (mod-player-transmog C++ 模块)
--   character_transmog     (lua_scripts/Transmogrification Eluna 脚本)
-- ============================================================================

SET FOREIGN_KEY_CHECKS = 0;

-- 旧版幻化模块 (mod-transmog) 遗留表
DROP TABLE IF EXISTS `custom_transmogrification`;
DROP TABLE IF EXISTS `custom_transmogrification_sets`;
DROP TABLE IF EXISTS `custom_unlocked_appearances`;

SET FOREIGN_KEY_CHECKS = 1;
