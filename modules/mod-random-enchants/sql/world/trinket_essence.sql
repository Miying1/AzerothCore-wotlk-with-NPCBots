-- ============================================================
-- 饰品特效提取器（道具 SQL）—— mod-random-enchants 模块
--   右键使用：列出背包中带可提取特效的饰品 → 选中一件 →
--   读取其特效法术 ID，查出对应的附魔卷轴并生成 → 销毁原饰品，并消耗提取器。
--   脚本实现见 modules/mod-random-enchants/src/TrinketEssence.cpp（ScriptName = TrinketEssenceItem）。
--
-- 【依赖】必须先执行同一目录下的 trinket_enchant_scrolls.sql，
--   其中包含附魔定义（spellitemenchantment_dbc）、附魔法术（spell_dbc）、
--   附魔卷轴（item_template）以及运行时映射表 mod_trinket_essence_map；
--   本文件只负责「饰品特效提取器」道具本体。
--   两者均由 outputs/饰品武器特效转附魔/generate.py 配套生成/维护。
-- ============================================================

DELETE FROM `item_template` WHERE `entry` = 60105;
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`,
     `ItemLevel`, `RequiredLevel`, `stackable`, `bonding`, `Material`, `delay`, `description`,
     `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellppmRate_1`, `spellcooldown_1`,
     `spellcategory_1`, `spellcategorycooldown_1`, `ScriptName`)
VALUES
    (60105, 0, 0, -1, '饰品特效提取器', 48007, 5, 0,
     1, 0, 0, 0, -1, -1,
     1, 80, 1, 1, 0, 1000, '右键使用：提取背包内一件饰品上的特效，将其转化为武器附魔卷轴。提取成功后，饰品与提取器都会被销毁。',
     18282, 0, 0, 0, -1,
     0, -1, 'TrinketEssenceItem');
