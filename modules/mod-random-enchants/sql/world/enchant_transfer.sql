-- 随机属性萃取与转移 —— 道具（独立功能，与泰坦附魔无关）
-- 脚本实现见 modules/mod-random-enchants/src/EnchantTransfer.cpp：
--   EnchantTransferItem —— 同时绑定下面两个道具：
--     60103 属性萃取仪：萃取一件装备的随机属性，生成属性卷轴（原装备销毁）
--     60104 属性卷轴  ：将卷轴随机属性注入到一件装备（卷轴消耗）
-- 卷轴携带的随机属性由 item_instance 的 randomPropertyId 字段持久化，可跨登录长久保留。
-- 可操作的装备部位：武器 / 护甲（头、颈、肩、胸、腕、手、腰、腿、脚、戒指、披风、盾牌、副手等），不含饰品。

DELETE FROM `item_template` WHERE `entry` IN (60103, 60104);
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`,
     `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `stackable`, `bonding`,
     `description`, `spellid_1`, `spelltrigger_1`, `ScriptName`)
VALUES
    (60103, 0, 0, '属性提取器', 40557, 5, 0, 1, 0, 0,
     0, -1, -1, 1, 80, 1, 1,
     '右键使用：提取一件装备的蓝色属性词条，生成属性卷轴（原装备和提取器将被销毁）。',
     18282, 0, 'EnchantTransferItem'),
    (60104, 0, 0, '属性卷轴', 11161, 4, 0, 1, 0, 0,
     0, -1, -1, 1, 80, 1, 1,
     '右键使用：将卷轴上的属性词条注入到一件装备上，若目标装备已有词条将被覆盖。',
     18282, 0, 'EnchantTransferItem');
