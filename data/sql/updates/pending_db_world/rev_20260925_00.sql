-- NPCBot 生物型危险区域：修复表结构缺列 + 奥杜尔锋鳞地面蓝焰（噬体烈焰）
-- 注：本文件只做增量变更，保证已建库与全新部署都能得到正确结果

-- 1) 表结构兼容：代码 NPCBotHazardMgr::LoadFromDB 的 SELECT 已引用 required_aura_spell_id，
--    旧结构缺该列会导致整表查询失败，所有生物型危险区域（虚空领域、冰柱等）一并失效。
--    仅当列不存在时执行 ALTER，避免已有该列的数据库重复加列报错。
SET @column_exists = (SELECT 1 FROM `INFORMATION_SCHEMA`.`COLUMNS` WHERE `TABLE_SCHEMA` = DATABASE() AND `TABLE_NAME` = 'npcbot_creature_hazard' AND `COLUMN_NAME` = 'required_aura_spell_id' LIMIT 1);
SET @sql = IF(@column_exists IS NULL, 'ALTER TABLE `npcbot_creature_hazard` ADD COLUMN `required_aura_spell_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT ''需同时存在的技能光环ID，非0时仅当生物身上存在该光环才视为危险源'' AFTER `deactivation_delay_ms`', 'SELECT 1');
PREPARE stmt FROM @sql;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

-- 2) 奥杜尔：锋鳞（Razorscale）第一阶段地面蓝焰「噬体烈焰」
--    Boss 空中阶段对随机玩家施放 63236，在落点召唤生物 34188（10人）/34189（25人）；
--    该生物经 creature_template_addon 常驻光环 64709（25人解析为 64734），
--    64709/64734 为 PERIODIC_TRIGGER_SPELL（2 秒），触发 64704/64733
--    以生物为中心、半径索引 8 的范围火焰伤害，全程不产生 DynamicObject，无法被现有扫描识别，
--    故按生物型危险区域配置：damage_spell_id 优先读取伤害法术半径，radius 作为兜底下限，
--    safety_distance 额外留出 2 码余量。生物常驻即危险，required_aura_spell_id 保持 0。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 603 AND `creature_entry` IN (34188, 34189);

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `required_aura_spell_id`, `comment`)
VALUES
(603, 34188, 8.0, 64704, 2.0, 1000, 0, '奥杜尔：锋鳞噬体烈焰地面蓝焰（10人）'),
(603, 34189, 8.0, 64733, 2.0, 1000, 0, '奥杜尔：锋鳞噬体烈焰地面蓝焰（25人）');
