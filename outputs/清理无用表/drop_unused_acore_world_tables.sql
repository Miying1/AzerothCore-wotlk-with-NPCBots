-- ============================================================================
-- acore_world 无用表清理脚本
-- 生成日期: 2026-09-18
-- 目标库  : acore_world
--
-- 说明:
--   以下 93 张表均为"从客户端 DBC 批量导入的转储副本"(不带 _dbc 后缀)，
--   核心与现有模块均不查询，可安全删除。核心实际读取的是对应的 *_dbc 表
--   (例如 Spell.dbc -> spell_dbc)，与本脚本删除的表无关。
--
-- 执行前务必先备份:
--   mysqldump -uroot -p acore_world > acore_world_backup.sql
--
-- 不要删除(项目在用，但建表脚本不在 data/sql 中):
--   creature_template_npcbot_appearance / _disabled_items / _extras / _wander_nodes,
--   creature_template_outfits,
--   mod_item_enchantment_random, mod_item_enchantment_result,
--   zone_difficulty_level, zone_difficulty_mapbase, zone_difficulty_spells,
--   zone_difficulty_spell_group
-- ============================================================================

SET FOREIGN_KEY_CHECKS = 0;

-- 成就 / 区域 / 阵营 / 角色 / 聊天
DROP TABLE IF EXISTS `achievement`;
DROP TABLE IF EXISTS `achievement_category`;
DROP TABLE IF EXISTS `achievement_criteria`;
DROP TABLE IF EXISTS `areagroup`;
DROP TABLE IF EXISTS `areatable`;
DROP TABLE IF EXISTS `charstartoutfit`;
DROP TABLE IF EXISTS `chartitles`;
DROP TABLE IF EXISTS `chatchannels`;
DROP TABLE IF EXISTS `chrclasses`;
DROP TABLE IF EXISTS `chrraces`;
DROP TABLE IF EXISTS `faction`;
DROP TABLE IF EXISTS `factiongroup`;
DROP TABLE IF EXISTS `factiontemplate`;

-- 物品 / 货币 / 地牢 / 专业技能
DROP TABLE IF EXISTS `currencycategory`;
DROP TABLE IF EXISTS `currencytypes`;
DROP TABLE IF EXISTS `dungeonencounter`;
DROP TABLE IF EXISTS `item`;
DROP TABLE IF EXISTS `itemclass`;
DROP TABLE IF EXISTS `itemdisplayinfo`;
DROP TABLE IF EXISTS `itemextendedcost`;
DROP TABLE IF EXISTS `itemset`;
DROP TABLE IF EXISTS `itemsubclass`;
DROP TABLE IF EXISTS `gemproperties`;
DROP TABLE IF EXISTS `lfgdungeongroup`;
DROP TABLE IF EXISTS `lfgdungeons`;

-- 法术相关
DROP TABLE IF EXISTS `spell`;
DROP TABLE IF EXISTS `spellcasttimes`;
DROP TABLE IF EXISTS `spellcategory`;
DROP TABLE IF EXISTS `spelldescriptionvariables`;
DROP TABLE IF EXISTS `spelldifficulty`;
DROP TABLE IF EXISTS `spelldispeltype`;
DROP TABLE IF EXISTS `spellduration`;
DROP TABLE IF EXISTS `spellfocusobject`;
DROP TABLE IF EXISTS `spellicon`;
DROP TABLE IF EXISTS `spellitemenchantment`;
DROP TABLE IF EXISTS `spellmechanic`;
DROP TABLE IF EXISTS `spellmissile`;
DROP TABLE IF EXISTS `spellmissilemotion`;
DROP TABLE IF EXISTS `spellradius`;
DROP TABLE IF EXISTS `spellrange`;
DROP TABLE IF EXISTS `spellrunecost`;
DROP TABLE IF EXISTS `spellshapeshiftform`;
DROP TABLE IF EXISTS `spellvisual`;
DROP TABLE IF EXISTS `spellvisualeffectname`;
DROP TABLE IF EXISTS `spellvisualkit`;
DROP TABLE IF EXISTS `spellvisualkitareamodel`;
DROP TABLE IF EXISTS `spellvisualkitmodelattach`;
DROP TABLE IF EXISTS `spellvisualprecasttransitions`;
DROP TABLE IF EXISTS `overridespelldata`;

-- 光照 / 场景 / 影片 / 音乐 / 界面
DROP TABLE IF EXISTS `light`;
DROP TABLE IF EXISTS `lightfloatband`;
DROP TABLE IF EXISTS `lightintband`;
DROP TABLE IF EXISTS `lightparams`;
DROP TABLE IF EXISTS `lightskybox`;
DROP TABLE IF EXISTS `cinematiccamera`;
DROP TABLE IF EXISTS `cinematicsequences`;
DROP TABLE IF EXISTS `movie`;
DROP TABLE IF EXISTS `moviefiledata`;
DROP TABLE IF EXISTS `movievariation`;
DROP TABLE IF EXISTS `zonemusic`;
DROP TABLE IF EXISTS `worldstateui`;
DROP TABLE IF EXISTS `loadingscreens`;
DROP TABLE IF EXISTS `animationdata`;
DROP TABLE IF EXISTS `screeneffect`;
DROP TABLE IF EXISTS `filedata`;

-- 生物 / 模型 / 地图
DROP TABLE IF EXISTS `creaturedisplayinfo`;
DROP TABLE IF EXISTS `creaturedisplayinfoextra`;
DROP TABLE IF EXISTS `creaturemodeldata`;
DROP TABLE IF EXISTS `creaturesounddata`;
DROP TABLE IF EXISTS `gameobjectdisplayinfo`;
DROP TABLE IF EXISTS `wmoareatable`;
DROP TABLE IF EXISTS `map`;
DROP TABLE IF EXISTS `mapdifficulty`;

-- 其它
DROP TABLE IF EXISTS `gametips`;
DROP TABLE IF EXISTS `holidays`;
DROP TABLE IF EXISTS `holidaynames`;
DROP TABLE IF EXISTS `holidaydescriptions`;
DROP TABLE IF EXISTS `lock`;
DROP TABLE IF EXISTS `locktype`;
DROP TABLE IF EXISTS `questsort`;
DROP TABLE IF EXISTS `skillline`;
DROP TABLE IF EXISTS `skilllineability`;
DROP TABLE IF EXISTS `skillraceclassinfo`;
DROP TABLE IF EXISTS `soundentries`;
DROP TABLE IF EXISTS `stationery`;
DROP TABLE IF EXISTS `talent`;
DROP TABLE IF EXISTS `talenttab`;
DROP TABLE IF EXISTS `taxinodes`;
DROP TABLE IF EXISTS `taxipath`;
DROP TABLE IF EXISTS `taxipathnode`;
DROP TABLE IF EXISTS `totemcategory`;
DROP TABLE IF EXISTS `vehicle`;
DROP TABLE IF EXISTS `vehicleseat`;

SET FOREIGN_KEY_CHECKS = 1;
