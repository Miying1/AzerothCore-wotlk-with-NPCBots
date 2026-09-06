DELETE FROM `item_template` WHERE `entry` = 70000;
INSERT INTO `item_template`
    (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`,
     `ItemLevel`, `RequiredLevel`, `maxcount`, `stackable`,
     `spellid_1`, `description`, `ScriptName`,bonding)
VALUES
    (70000, 15, 4, '刷金卷轴', 2616, 5,
     80, 80, 10, 0,
     18282, '使用后金币拾取额外增加 15%,最高可叠加150%，账号通用。',
     'PlayerVipGoldBonusItem',1);

SET @ITEM_ENTRY := 70001;
DELETE FROM `item_template` where `entry` >= @ITEM_ENTRY AND `entry` < @ITEM_ENTRY + 16;
-- 玩家权益消耗品
INSERT INTO `item_template` (`entry`, `class`, `subclass`, `name`, `displayid`, `Quality`, `Flags`, `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `stackable`, `description`, `spellid_1`, `spelltrigger_1`, `ScriptName`)
VALUES
(@ITEM_ENTRY, 0, 0, '[10PT冰冠堡垒]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:631|10PT。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+1, 0, 0, '[10H冰冠堡垒]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:631|10H。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+2, 0, 0, '[25PT冰冠堡垒]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:631|25PT。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+3, 0, 0, '[25H冰冠堡垒]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:631|25H。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+4, 0, 0, '[10PT奥杜尔]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:603|10PT。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+5, 0, 0, '[25PT奥杜尔]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:603|25PT。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+6, 0, 0, '[10PT十字军试炼]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:649|10PT。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+7, 0, 0, '[10H十字军试炼]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:649|10H。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+8, 0, 0, '[25PT十字军试]炼重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:649|25PT。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+9, 0, 0, '[25H十字军试炼]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:649|25H。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+10, 0, 0, '[10PT祖阿曼]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:568|10PT。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+11, 0, 0, '[10PT卡拉赞]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:532|10PT。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+12, 0, 0, '[5H灵魂洪炉]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:632|5H。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+13, 0, 0, '[5H萨隆矿坑]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:658|5H。', 18282, 0, 'PlayerVipResetInstanceItem'),
(@ITEM_ENTRY+14, 0, 0, '[5H映像大厅]重置券', 634, 5, 0, 1, 0, 0, 0, -1, -1, 80, 80, 20, '使用后重置副本CD，MAP:668|5H。', 18282, 0, 'PlayerVipResetInstanceItem');
