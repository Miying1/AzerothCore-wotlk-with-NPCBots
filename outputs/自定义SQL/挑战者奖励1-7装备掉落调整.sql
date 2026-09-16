-- ============================================================
-- 挑战者奖励袋 1-7 装备掉落调整
--
-- 将各奖励袋的「装备掉落」引用（item_loot_template 中的 Reference 行）
-- 替换为新的装备池（reference_loot_template id）。
--
-- 新方案（PT=普通版 / H=英雄版）：
--   袋1 : PT=62105(100%×1)         H=62106(100%×1)
--   袋2 : PT=62105(100%×1)         H=62106(100%×2)
--   袋3 : PT=62106(70%×1)          H=62107(80%×1)
--   袋4 : PT=914000(60%×1)         H=62108(70%×1)
--   袋5 : PT=62109+62110(25%各1)   H=62101+62102(50%各1)
--   袋6 : PT=62111+62112(30%各1)   H=934000(60%×1)
--   袋7 : PT=62112(12%×1)          H=62113(60%×1)
--
-- 说明：
--   1. 仅替换装备 Reference 行（Reference > 0，GroupId = 0），
--      货币/材料行（纹章、挑战值、挑战奖章、泰坦符文/精华等）保持不变。
--   2. 袋8（62115）不在本次调整范围内，保持原掉落。
--   3. 引用的装备池 62105~62113 需已存在于 reference_loot_template。
-- ============================================================

-- ============================================================
-- 袋1
-- ============================================================
-- 普通版（62100）
DELETE FROM `item_loot_template` WHERE `Entry` = 62100 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62100, 0, 62105, 100, 0, 1, 0, 1, 1, '挑战者奖励1·普通 - 装备池62105');

-- 英雄版（62101）
DELETE FROM `item_loot_template` WHERE `Entry` = 62101 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62101, 0, 62106, 100, 0, 1, 0, 1, 1, '挑战者奖励1·英雄 - 装备池62106');

-- ============================================================
-- 袋2
-- ============================================================
-- 普通版（62102）
DELETE FROM `item_loot_template` WHERE `Entry` = 62102 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62102, 0, 62105, 100, 0, 1, 0, 1, 1, '挑战者奖励2·普通 - 装备池62105');

-- 英雄版（62103）
DELETE FROM `item_loot_template` WHERE `Entry` = 62103 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62103, 0, 62106, 100, 0, 1, 0, 2, 2, '挑战者奖励2·英雄 - 装备池62106');

-- ============================================================
-- 袋3
-- ============================================================
-- 普通版（62104）
DELETE FROM `item_loot_template` WHERE `Entry` = 62104 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62104, 0, 62106, 70, 0, 1, 0, 1, 1, '挑战者奖励3·普通 - 装备池62106');

-- 英雄版（62105）
DELETE FROM `item_loot_template` WHERE `Entry` = 62105 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62105, 0, 62107, 80, 0, 1, 0, 1, 1, '挑战者奖励3·英雄 - 装备池62107');

-- ============================================================
-- 袋4
-- ============================================================
-- 普通版（62106）
DELETE FROM `item_loot_template` WHERE `Entry` = 62106 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62106, 0, 914000, 60, 0, 1, 0, 1, 1, '挑战者奖励4·普通 - 装备池914000');

-- 英雄版（62107）
DELETE FROM `item_loot_template` WHERE `Entry` = 62107 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62107, 0, 62108, 70, 0, 1, 0, 1, 1, '挑战者奖励4·英雄 - 装备池62108');

-- ============================================================
-- 袋5
-- ============================================================
-- 普通版（62108）
DELETE FROM `item_loot_template` WHERE `Entry` = 62108 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62108, 0, 62109, 25, 0, 1, 0, 1, 1, '挑战者奖励5·普通 - 装备池62109'),
(62108, 1, 62110, 25, 0, 1, 0, 1, 1, '挑战者奖励5·普通 - 装备池62110');

-- 英雄版（62109）
DELETE FROM `item_loot_template` WHERE `Entry` = 62109 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62109, 0, 62101, 50, 0, 1, 0, 1, 1, '挑战者奖励5·英雄 - 装备池62101(联盟)'),
(62109, 1, 62102, 50, 0, 1, 0, 1, 1, '挑战者奖励5·英雄 - 装备池62102(部落)');

-- ============================================================
-- 袋6
-- ============================================================
-- 普通版（62110）
DELETE FROM `item_loot_template` WHERE `Entry` = 62110 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62110, 0, 62111, 30, 0, 1, 0, 1, 1, '挑战者奖励6·普通 - 装备池62111'),
(62110, 1, 62112, 30, 0, 1, 0, 1, 1, '挑战者奖励6·普通 - 装备池62112');

-- 英雄版（62111）
DELETE FROM `item_loot_template` WHERE `Entry` = 62111 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62111, 0, 934000, 60, 0, 1, 0, 1, 1, '挑战者奖励6·英雄 - 装备池934000');

-- ============================================================
-- 袋7
-- ============================================================
-- 普通版（62112）
DELETE FROM `item_loot_template` WHERE `Entry` = 62112 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62112, 0, 62112, 12, 0, 1, 0, 1, 1, '挑战者奖励7·普通 - 装备池62112');

-- 英雄版（62113）
DELETE FROM `item_loot_template` WHERE `Entry` = 62113 AND `Reference` > 0;
INSERT INTO `item_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(62113, 0, 62113, 60, 0, 1, 0, 1, 1, '挑战者奖励7·英雄 - 装备池62113');
