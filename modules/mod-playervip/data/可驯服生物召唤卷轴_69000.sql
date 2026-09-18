-- ============================================================================
-- 可驯服生物召唤卷轴（物品 69000）
-- ============================================================================
-- 说明：
--   · 仅限猎人使用（AllowableClass = 4，即 CLASS_HUNTER 位掩码）
--   · 使用后在玩家身边临时召唤一个野生生物供猎人驯服
--   · 生物 ID 取自 description 中的 [entry]（当前为 94000，可自行修改）
--   · 图标使用书本样式（displayid = 1317）
--   · 使用脚本由 mod-playervip 模块的 item_tameable_summon.cpp 实现
--   · 如需调整等级：改 RequiredLevel / ItemLevel 即可
-- ============================================================================

DELETE FROM `item_template` WHERE `entry` = 69000;
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`,
     `ItemLevel`, `RequiredLevel`, `stackable`, `description`, `spellid_1`, `spelltrigger_1`, `ScriptName`)
VALUES
    (69000, 0, 0, '召唤:玄牛幼崽', 1317, 5,
     0, 1, 0, 0, 0, 4, -1,
     80, 80, 1, '使用后召唤一个可供驯服的生物[94000],注意小心召唤生物被佣兵打死!', 18282, 0, 'item_tameable_summon');

DELETE FROM `item_template` WHERE `entry` = 69001;
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`,
     `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`,
     `ItemLevel`, `RequiredLevel`, `stackable`, `description`, `spellid_1`, `spelltrigger_1`, `ScriptName`)
VALUES
    (69001, 0, 0, '召唤:白虎幼崽', 1317, 5,
     0, 1, 0, 0, 0, 4, -1,
     80, 80, 1, '使用后召唤一个可供驯服的生物[94001],注意小心召唤生物被佣兵打死!', 18282, 0, 'item_tameable_summon');

 