-- 随机装备礼包：使用后打开 gossip 菜单，自选分类与部位，随机发放一件指定装等的可装备物品
-- 目标装等写成方括号内的纯数字，例如 [230]，可放在 description 的任意位置，脚本会解析该数值
-- 脚本实现见 modules/mod-playervip/src/item_random_equip_gift.cpp：
--   RandomEquipGiftArmorOtherItem —— 护甲 + 其他：一级菜单（护甲 / 其他），护甲再选子类与部位
--   RandomEquipGiftWeaponItem     —— 武器：直接列出武器子类（二级分类），选完即发放
DELETE FROM `item_template` WHERE `entry` = 60401;
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`,
     `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `stackable`, `bonding`,
     `description`, `spellid_1`, `spelltrigger_1`, `ScriptName`)
VALUES
    (60401, 0, 0, '230护甲自选包', 19745, 4, 0, 1, 0, 0,
     0, -1, -1, 1, 80, 20, 1,
     '打开后可在菜单中自选分类与部位[230]',
     18282, 0, 'RandomEquipGiftArmorOtherItem');
 

DELETE FROM `item_template` WHERE `entry` = 60402;
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`,
     `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `stackable`, `bonding`,
     `description`, `spellid_1`, `spelltrigger_1`, `ScriptName`)
VALUES
    (60402, 0, 0, '230武器自选包', 19745, 4, 0, 1, 0, 0,
     0, -1, -1, 1, 80, 20, 1,
     '打开后可在菜单中自选分类[230]',
     18282, 0, 'RandomEquipGiftWeaponItem');



DELETE FROM `item_template` WHERE `entry` = 60403;
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`,
     `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `stackable`, `bonding`,
     `description`, `spellid_1`, `spelltrigger_1`, `ScriptName`)
VALUES
    (60403, 0, 0, '240护甲自选包', 19745, 4, 0, 1, 0, 0,
     0, -1, -1, 1, 80, 20, 1,
     '打开后可在菜单中自选分类与部位[240]',
     18282, 0, 'RandomEquipGiftArmorOtherItem');
 

DELETE FROM `item_template` WHERE `entry` = 60404;
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`,
     `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `stackable`, `bonding`,
     `description`, `spellid_1`, `spelltrigger_1`, `ScriptName`)
VALUES
    (60404, 0, 0, '240武器自选包', 19745, 4, 0, 1, 0, 0,
     0, -1, -1, 1, 80, 20, 1,
     '打开后可在菜单中自选分类[240]',
     18282, 0, 'RandomEquipGiftWeaponItem');
