-- ============================================================
-- 将物品 109999（神器材料）、49908（源生萨隆邪铁）加入各团本 BOSS 独立掉落
-- 掉落概率/数量按团本难度区分（数量均为 1）：
--
-- ICC（冰冠堡垒）全部 BOSS（含对应战利品宝箱）：
--   10人普通：10% 掉落 1
--   10人英雄：50% 掉落 1
--   25人普通：50% 掉落 1
--   25人英雄：100% 掉落 1
--
-- TOC（十字军的试炼）：
--   10人普通：尾王（阿努巴拉克）50% 掉落 1
--   10人英雄：尾王 80% 掉落 1，其他 BOSS 30% 掉落 1
--   25人普通：尾王 100% 掉落 2，其他 BOSS 50% 掉落 1
--   25人英雄：尾王 100% 掉落 4，其他 BOSS 100% 掉落 1
--
-- 卡拉赞：
--   神器材料：玛克扎尔王子 50%、夜之魇 80%，各掉落 1
--   源生萨隆邪铁：玛克扎尔王子 50%、夜之魇 20%，各掉落 1
--
-- 祖阿曼：
--   神器材料：祖尔金 50% 掉落 1
--   源生萨隆邪铁：祖尔金 50% 掉落 1
--
-- 难度条目说明：
--   生物掉落：基础条目 = 10人普通，difficulty_entry_1 = 25人普通，
--             difficulty_entry_2 = 10人英雄，difficulty_entry_3 = 25人英雄
--   宝箱掉落：按 gameobject_template.data0 指向的掉落表，对应难度宝箱条目
-- ============================================================

-- ============================================================
-- 一、ICC（冰冠堡垒）
-- ============================================================

-- 马洛加尔领主（Lord Marrowgar）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36612, 37957, 37958, 37959) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36612, 109999, 0, 10, 0, 1, 0, 1, 1, '马洛加尔领主（10人普通） - 神器材料'),
(37957, 109999, 0, 50, 0, 1, 0, 1, 1, '马洛加尔领主（25人普通） - 神器材料'),
(37958, 109999, 0, 50, 0, 1, 0, 1, 1, '马洛加尔领主（10人英雄） - 神器材料'),
(37959, 109999, 0, 100, 0, 1, 0, 1, 1, '马洛加尔领主（25人英雄） - 神器材料');

-- 亡语者女士（Lady Deathwhisper）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36855, 38106, 38296, 38297) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36855, 109999, 0, 10, 0, 1, 0, 1, 1, '亡语者女士（10人普通） - 神器材料'),
(38106, 109999, 0, 50, 0, 1, 0, 1, 1, '亡语者女士（25人普通） - 神器材料'),
(38296, 109999, 0, 50, 0, 1, 0, 1, 1, '亡语者女士（10人英雄） - 神器材料'),
(38297, 109999, 0, 100, 0, 1, 0, 1, 1, '亡语者女士（25人英雄） - 神器材料');

-- 腐面（Festergut）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36626, 37504, 37505, 37506) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36626, 109999, 0, 10, 0, 1, 0, 1, 1, '腐面（10人普通） - 神器材料'),
(37504, 109999, 0, 50, 0, 1, 0, 1, 1, '腐面（25人普通） - 神器材料'),
(37505, 109999, 0, 50, 0, 1, 0, 1, 1, '腐面（10人英雄） - 神器材料'),
(37506, 109999, 0, 100, 0, 1, 0, 1, 1, '腐面（25人英雄） - 神器材料');

-- 烂肠（Rotface）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36627, 38390, 38549, 38550) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36627, 109999, 0, 10, 0, 1, 0, 1, 1, '烂肠（10人普通） - 神器材料'),
(38390, 109999, 0, 50, 0, 1, 0, 1, 1, '烂肠（25人普通） - 神器材料'),
(38549, 109999, 0, 50, 0, 1, 0, 1, 1, '烂肠（10人英雄） - 神器材料'),
(38550, 109999, 0, 100, 0, 1, 0, 1, 1, '烂肠（25人英雄） - 神器材料');

-- 普崔塞德教授（Professor Putricide）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36678, 38431, 38585, 38586) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36678, 109999, 0, 10, 0, 1, 0, 1, 1, '普崔塞德教授（10人普通） - 神器材料'),
(38431, 109999, 0, 50, 0, 1, 0, 1, 1, '普崔塞德教授（25人普通） - 神器材料'),
(38585, 109999, 0, 50, 0, 1, 0, 1, 1, '普崔塞德教授（10人英雄） - 神器材料'),
(38586, 109999, 0, 100, 0, 1, 0, 1, 1, '普崔塞德教授（25人英雄） - 神器材料');

-- 鲜血王子议会（Blood Prince Council）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (37970, 38401, 38784, 38785) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(37970, 109999, 0, 10, 0, 1, 0, 1, 1, '鲜血王子议会（10人普通） - 神器材料'),
(38401, 109999, 0, 50, 0, 1, 0, 1, 1, '鲜血王子议会（25人普通） - 神器材料'),
(38784, 109999, 0, 50, 0, 1, 0, 1, 1, '鲜血王子议会（10人英雄） - 神器材料'),
(38785, 109999, 0, 100, 0, 1, 0, 1, 1, '鲜血王子议会（25人英雄） - 神器材料');

-- 鲜血女王兰娜瑟尔（Blood-Queen Lana'thel）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (37955, 38434, 38435, 38436) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(37955, 109999, 0, 10, 0, 1, 0, 1, 1, '鲜血女王兰娜瑟尔（10人普通） - 神器材料'),
(38434, 109999, 0, 50, 0, 1, 0, 1, 1, '鲜血女王兰娜瑟尔（25人普通） - 神器材料'),
(38435, 109999, 0, 50, 0, 1, 0, 1, 1, '鲜血女王兰娜瑟尔（10人英雄） - 神器材料'),
(38436, 109999, 0, 100, 0, 1, 0, 1, 1, '鲜血女王兰娜瑟尔（25人英雄） - 神器材料');

-- 辛达苟萨（Sindragosa）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36853, 38265, 38266, 38267) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36853, 109999, 0, 10, 0, 1, 0, 1, 1, '辛达苟萨（10人普通） - 神器材料'),
(38265, 109999, 0, 50, 0, 1, 0, 1, 1, '辛达苟萨（25人普通） - 神器材料'),
(38266, 109999, 0, 50, 0, 1, 0, 1, 1, '辛达苟萨（10人英雄） - 神器材料'),
(38267, 109999, 0, 100, 0, 1, 0, 1, 1, '辛达苟萨（25人英雄） - 神器材料');

-- 巫妖王（The Lich King）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36597, 39166, 39167, 39168) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36597, 109999, 0, 100, 0, 1, 0, 1, 1, '巫妖王（10人普通） - 神器材料'),
(39166, 109999, 0, 100, 0, 1, 0, 2, 2, '巫妖王（25人普通） - 神器材料'),
(39167, 109999, 0, 100, 0, 1, 0, 1, 1, '巫妖王（10人英雄） - 神器材料'),
(39168, 109999, 0, 100, 0, 1, 0, 4, 4, '巫妖王（25人英雄） - 神器材料');

-- 火炮战军械库（Gunship Armory）
DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (28045, 28057, 28072, 28090) AND `Item` = 109999;
INSERT INTO `gameobject_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(28045, 109999, 0, 10, 0, 1, 0, 1, 1, '火炮战军械库（10人普通） - 神器材料'),
(28072, 109999, 0, 50, 0, 1, 0, 1, 1, '火炮战军械库（25人普通） - 神器材料'),
(28057, 109999, 0, 50, 0, 1, 0, 1, 1, '火炮战军械库（10人英雄） - 神器材料'),
(28090, 109999, 0, 100, 0, 1, 0, 1, 1, '火炮战军械库（25人英雄） - 神器材料');

-- 死亡使者萨鲁法尔宝箱（Deathbringer's Cache）
DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (28046, 28058, 28074, 28088) AND `Item` = 109999;
INSERT INTO `gameobject_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(28046, 109999, 0, 10, 0, 1, 0, 1, 1, '死亡使者萨鲁法尔宝箱（10人普通） - 神器材料'),
(28074, 109999, 0, 50, 0, 1, 0, 1, 1, '死亡使者萨鲁法尔宝箱（25人普通） - 神器材料'),
(28058, 109999, 0, 50, 0, 1, 0, 1, 1, '死亡使者萨鲁法尔宝箱（10人英雄） - 神器材料'),
(28088, 109999, 0, 100, 0, 1, 0, 1, 1, '死亡使者萨鲁法尔宝箱（25人英雄） - 神器材料');

-- 瓦莉瑟瑞娅·梦行者宝箱（Cache of the Dreamwalker）
DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (28052, 28064, 28082, 28096) AND `Item` = 109999;
INSERT INTO `gameobject_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(28052, 109999, 0, 10, 0, 1, 0, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（10人普通） - 神器材料'),
(28082, 109999, 0, 50, 0, 1, 0, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（25人普通） - 神器材料'),
(28064, 109999, 0, 50, 0, 1, 0, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（10人英雄） - 神器材料'),
(28096, 109999, 0, 100, 0, 1, 0, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（25人英雄） - 神器材料');

-- ============================================================
-- 二、TOC（十字军的试炼）
-- ============================================================

-- 阿努巴拉克（Anub'arak）——尾王
DELETE FROM `creature_loot_template` WHERE `Entry` IN (34564, 34566, 35615, 35616) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(34564, 109999, 0, 50, 0, 1, 0, 1, 1, '阿努巴拉克（10人普通） - 神器材料'),
(34566, 109999, 0, 100, 0, 1, 0, 1, 1, '阿努巴拉克（25人普通） - 神器材料'),
(35615, 109999, 0, 80, 0, 1, 0, 1, 1, '阿努巴拉克（10人英雄） - 神器材料'),
(35616, 109999, 0, 100, 0, 1, 0, 4, 4, '阿努巴拉克（25人英雄） - 神器材料');

-- 冰吼（Icehowl）——诺森德野兽
DELETE FROM `creature_loot_template` WHERE `Entry` IN (35447, 35448, 35449) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(35447, 109999, 0, 50, 0, 1, 0, 1, 1, '冰吼·诺森德野兽（25人普通） - 神器材料'),
(35448, 109999, 0, 30, 0, 1, 0, 1, 1, '冰吼·诺森德野兽（10人英雄） - 神器材料'),
(35449, 109999, 0, 100, 0, 1, 0, 1, 1, '冰吼·诺森德野兽（25人英雄） - 神器材料');

-- 加拉克苏斯大王（Lord Jaraxxus）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (35216, 35268, 35269) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(35216, 109999, 0, 50, 0, 1, 0, 1, 1, '加拉克苏斯大王（25人普通） - 神器材料'),
(35268, 109999, 0, 30, 0, 1, 0, 1, 1, '加拉克苏斯大王（10人英雄） - 神器材料'),
(35269, 109999, 0, 100, 0, 1, 0, 1, 1, '加拉克苏斯大王（25人英雄） - 神器材料');

-- 瓦格里双子（Val'kyr Twins）——掉落挂靠与“凯旋纹章”一致，保证每次只掉 1 份
--   （10人英雄挂菲奥拉 35351；25人普通/英雄挂艾迪丝 35347/35349）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (35347, 35349, 35351) AND `Item` = 109999;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(35347, 109999, 0, 50, 0, 1, 0, 1, 1, '瓦格里双子（25人普通） - 神器材料'),
(35351, 109999, 0, 30, 0, 1, 0, 1, 1, '瓦格里双子（10人英雄） - 神器材料'),
(35349, 109999, 0, 100, 0, 1, 0, 1, 1, '瓦格里双子（25人英雄） - 神器材料');

-- 阵营冠军宝箱（Champions' Cache）
DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (27503, 27335, 27356) AND `Item` = 109999;
INSERT INTO `gameobject_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(27503, 109999, 0, 50, 0, 1, 0, 1, 1, '阵营冠军宝箱（25人普通） - 神器材料'),
(27335, 109999, 0, 30, 0, 1, 0, 1, 1, '阵营冠军宝箱（10人英雄） - 神器材料'),
(27356, 109999, 0, 100, 0, 1, 0, 1, 1, '阵营冠军宝箱（25人英雄） - 神器材料');

-- ============================================================
-- 三、卡拉赞
-- ============================================================

-- 玛克扎尔王子（Prince Malchezaar）
DELETE FROM `creature_loot_template` WHERE `Entry` = 15690 AND `Item` IN (109999, 49908);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(15690, 109999, 0, 50, 0, 1, 0, 1, 1, '玛克扎尔王子 - 神器材料'),
(15690, 49908, 0, 70, 0, 1, 0, 1, 1, '玛克扎尔王子 - 源生萨隆邪铁');

-- 夜之魇（Nightbane）
DELETE FROM `creature_loot_template` WHERE `Entry` = 17225 AND `Item` IN (109999, 49908);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(17225, 109999, 0, 100, 0, 1, 0, 1, 1, '夜之魇 - 神器材料'),
(17225, 49908, 0, 100, 0, 1, 0, 1, 1, '夜之魇 - 源生萨隆邪铁');

-- ============================================================
-- 四、祖阿曼
-- ============================================================

-- 祖尔金（Zul'jin）
DELETE FROM `creature_loot_template` WHERE `Entry` = 23863 AND `Item` IN (109999, 49908);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(23863, 109999, 0, 50, 0, 1, 0, 1, 1, '祖尔金 - 神器材料'),
(23863, 49908, 0, 80, 0, 1, 0, 1, 1, '祖尔金 - 源生萨隆邪铁');

-- ============================================================
-- 新增掉落：源力核心（63101）、源力碎片（63100）
-- 源力核心：爆率为“神器材料（109999）”的一半，数量与其一致
-- 全部区域：源力核心与源力碎片同组（GroupId = 5），互斥掉落；
--           源力碎片爆率 = 100 - 源力核心，数量为 1
-- ============================================================

-- ============================================================
-- 一、ICC（冰冠堡垒）——源力核心 / 源力碎片（同组 5，互斥）
-- ============================================================

-- 马洛加尔领主（Lord Marrowgar）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36612, 37957, 37958, 37959) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36612, 63101, 0, 5, 0, 1, 5, 1, 1, '马洛加尔领主（10人普通） - 源力核心'),
(36612, 63100, 0, 95, 0, 1, 5, 1, 1, '马洛加尔领主（10人普通） - 源力碎片'),
(37957, 63101, 0, 25, 0, 1, 5, 1, 1, '马洛加尔领主（25人普通） - 源力核心'),
(37957, 63100, 0, 75, 0, 1, 5, 1, 1, '马洛加尔领主（25人普通） - 源力碎片'),
(37958, 63101, 0, 25, 0, 1, 5, 1, 1, '马洛加尔领主（10人英雄） - 源力核心'),
(37958, 63100, 0, 75, 0, 1, 5, 1, 1, '马洛加尔领主（10人英雄） - 源力碎片'),
(37959, 63101, 0, 50, 0, 1, 5, 1, 1, '马洛加尔领主（25人英雄） - 源力核心'),
(37959, 63100, 0, 50, 0, 1, 5, 1, 1, '马洛加尔领主（25人英雄） - 源力碎片');

-- 亡语者女士（Lady Deathwhisper）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36855, 38106, 38296, 38297) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36855, 63101, 0, 5, 0, 1, 5, 1, 1, '亡语者女士（10人普通） - 源力核心'),
(36855, 63100, 0, 95, 0, 1, 5, 1, 1, '亡语者女士（10人普通） - 源力碎片'),
(38106, 63101, 0, 25, 0, 1, 5, 1, 1, '亡语者女士（25人普通） - 源力核心'),
(38106, 63100, 0, 75, 0, 1, 5, 1, 1, '亡语者女士（25人普通） - 源力碎片'),
(38296, 63101, 0, 25, 0, 1, 5, 1, 1, '亡语者女士（10人英雄） - 源力核心'),
(38296, 63100, 0, 75, 0, 1, 5, 1, 1, '亡语者女士（10人英雄） - 源力碎片'),
(38297, 63101, 0, 50, 0, 1, 5, 1, 1, '亡语者女士（25人英雄） - 源力核心'),
(38297, 63100, 0, 50, 0, 1, 5, 1, 1, '亡语者女士（25人英雄） - 源力碎片');

-- 腐面（Festergut）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36626, 37504, 37505, 37506) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36626, 63101, 0, 5, 0, 1, 5, 1, 1, '腐面（10人普通） - 源力核心'),
(36626, 63100, 0, 95, 0, 1, 5, 1, 1, '腐面（10人普通） - 源力碎片'),
(37504, 63101, 0, 25, 0, 1, 5, 1, 1, '腐面（25人普通） - 源力核心'),
(37504, 63100, 0, 75, 0, 1, 5, 1, 1, '腐面（25人普通） - 源力碎片'),
(37505, 63101, 0, 25, 0, 1, 5, 1, 1, '腐面（10人英雄） - 源力核心'),
(37505, 63100, 0, 75, 0, 1, 5, 1, 1, '腐面（10人英雄） - 源力碎片'),
(37506, 63101, 0, 50, 0, 1, 5, 1, 1, '腐面（25人英雄） - 源力核心'),
(37506, 63100, 0, 50, 0, 1, 5, 1, 1, '腐面（25人英雄） - 源力碎片');

-- 烂肠（Rotface）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36627, 38390, 38549, 38550) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36627, 63101, 0, 5, 0, 1, 5, 1, 1, '烂肠（10人普通） - 源力核心'),
(36627, 63100, 0, 95, 0, 1, 5, 1, 1, '烂肠（10人普通） - 源力碎片'),
(38390, 63101, 0, 25, 0, 1, 5, 1, 1, '烂肠（25人普通） - 源力核心'),
(38390, 63100, 0, 75, 0, 1, 5, 1, 1, '烂肠（25人普通） - 源力碎片'),
(38549, 63101, 0, 25, 0, 1, 5, 1, 1, '烂肠（10人英雄） - 源力核心'),
(38549, 63100, 0, 75, 0, 1, 5, 1, 1, '烂肠（10人英雄） - 源力碎片'),
(38550, 63101, 0, 50, 0, 1, 5, 1, 1, '烂肠（25人英雄） - 源力核心'),
(38550, 63100, 0, 50, 0, 1, 5, 1, 1, '烂肠（25人英雄） - 源力碎片');

-- 普崔塞德教授（Professor Putricide）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36678, 38431, 38585, 38586) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36678, 63101, 0, 5, 0, 1, 5, 1, 1, '普崔塞德教授（10人普通） - 源力核心'),
(36678, 63100, 0, 95, 0, 1, 5, 1, 1, '普崔塞德教授（10人普通） - 源力碎片'),
(38431, 63101, 0, 25, 0, 1, 5, 1, 1, '普崔塞德教授（25人普通） - 源力核心'),
(38431, 63100, 0, 75, 0, 1, 5, 1, 1, '普崔塞德教授（25人普通） - 源力碎片'),
(38585, 63101, 0, 25, 0, 1, 5, 1, 1, '普崔塞德教授（10人英雄） - 源力核心'),
(38585, 63100, 0, 75, 0, 1, 5, 1, 1, '普崔塞德教授（10人英雄） - 源力碎片'),
(38586, 63101, 0, 50, 0, 1, 5, 1, 1, '普崔塞德教授（25人英雄） - 源力核心'),
(38586, 63100, 0, 50, 0, 1, 5, 1, 1, '普崔塞德教授（25人英雄） - 源力碎片');

-- 鲜血王子议会（Blood Prince Council）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (37970, 38401, 38784, 38785) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(37970, 63101, 0, 5, 0, 1, 5, 1, 1, '鲜血王子议会（10人普通） - 源力核心'),
(37970, 63100, 0, 95, 0, 1, 5, 1, 1, '鲜血王子议会（10人普通） - 源力碎片'),
(38401, 63101, 0, 25, 0, 1, 5, 1, 1, '鲜血王子议会（25人普通） - 源力核心'),
(38401, 63100, 0, 75, 0, 1, 5, 1, 1, '鲜血王子议会（25人普通） - 源力碎片'),
(38784, 63101, 0, 25, 0, 1, 5, 1, 1, '鲜血王子议会（10人英雄） - 源力核心'),
(38784, 63100, 0, 75, 0, 1, 5, 1, 1, '鲜血王子议会（10人英雄） - 源力碎片'),
(38785, 63101, 0, 50, 0, 1, 5, 1, 1, '鲜血王子议会（25人英雄） - 源力核心'),
(38785, 63100, 0, 50, 0, 1, 5, 1, 1, '鲜血王子议会（25人英雄） - 源力碎片');

-- 鲜血女王兰娜瑟尔（Blood-Queen Lana'thel）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (37955, 38434, 38435, 38436) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(37955, 63101, 0, 5, 0, 1, 5, 1, 1, '鲜血女王兰娜瑟尔（10人普通） - 源力核心'),
(37955, 63100, 0, 95, 0, 1, 5, 1, 1, '鲜血女王兰娜瑟尔（10人普通） - 源力碎片'),
(38434, 63101, 0, 25, 0, 1, 5, 1, 1, '鲜血女王兰娜瑟尔（25人普通） - 源力核心'),
(38434, 63100, 0, 75, 0, 1, 5, 1, 1, '鲜血女王兰娜瑟尔（25人普通） - 源力碎片'),
(38435, 63101, 0, 25, 0, 1, 5, 1, 1, '鲜血女王兰娜瑟尔（10人英雄） - 源力核心'),
(38435, 63100, 0, 75, 0, 1, 5, 1, 1, '鲜血女王兰娜瑟尔（10人英雄） - 源力碎片'),
(38436, 63101, 0, 50, 0, 1, 5, 1, 1, '鲜血女王兰娜瑟尔（25人英雄） - 源力核心'),
(38436, 63100, 0, 50, 0, 1, 5, 1, 1, '鲜血女王兰娜瑟尔（25人英雄） - 源力碎片');

-- 辛达苟萨（Sindragosa）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36853, 38265, 38266, 38267) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36853, 63101, 0, 5, 0, 1, 5, 1, 1, '辛达苟萨（10人普通） - 源力核心'),
(36853, 63100, 0, 95, 0, 1, 5, 1, 1, '辛达苟萨（10人普通） - 源力碎片'),
(38265, 63101, 0, 25, 0, 1, 5, 1, 1, '辛达苟萨（25人普通） - 源力核心'),
(38265, 63100, 0, 75, 0, 1, 5, 1, 1, '辛达苟萨（25人普通） - 源力碎片'),
(38266, 63101, 0, 25, 0, 1, 5, 1, 1, '辛达苟萨（10人英雄） - 源力核心'),
(38266, 63100, 0, 75, 0, 1, 5, 1, 1, '辛达苟萨（10人英雄） - 源力碎片'),
(38267, 63101, 0, 50, 0, 1, 5, 1, 1, '辛达苟萨（25人英雄） - 源力核心'),
(38267, 63100, 0, 50, 0, 1, 5, 1, 1, '辛达苟萨（25人英雄） - 源力碎片');

-- 巫妖王（The Lich King）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (36597, 39166, 39167, 39168) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(36597, 63101, 0, 50, 0, 1, 5, 1, 1, '巫妖王（10人普通） - 源力核心'),
(36597, 63100, 0, 50, 0, 1, 5, 1, 1, '巫妖王（10人普通） - 源力碎片'),
(39166, 63101, 0, 50, 0, 1, 5, 2, 2, '巫妖王（25人普通） - 源力核心'),
(39166, 63100, 0, 50, 0, 1, 5, 1, 1, '巫妖王（25人普通） - 源力碎片'),
(39167, 63101, 0, 50, 0, 1, 5, 1, 1, '巫妖王（10人英雄） - 源力核心'),
(39167, 63100, 0, 50, 0, 1, 5, 1, 1, '巫妖王（10人英雄） - 源力碎片'),
(39168, 63101, 0, 50, 0, 1, 5, 4, 4, '巫妖王（25人英雄） - 源力核心'),
(39168, 63100, 0, 50, 0, 1, 5, 1, 1, '巫妖王（25人英雄） - 源力碎片');

-- 火炮战军械库（Gunship Armory）
DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (28045, 28057, 28072, 28090) AND `Item` IN (63100, 63101);
INSERT INTO `gameobject_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(28045, 63101, 0, 5, 0, 1, 5, 1, 1, '火炮战军械库（10人普通） - 源力核心'),
(28045, 63100, 0, 95, 0, 1, 5, 1, 1, '火炮战军械库（10人普通） - 源力碎片'),
(28072, 63101, 0, 25, 0, 1, 5, 1, 1, '火炮战军械库（25人普通） - 源力核心'),
(28072, 63100, 0, 75, 0, 1, 5, 1, 1, '火炮战军械库（25人普通） - 源力碎片'),
(28057, 63101, 0, 25, 0, 1, 5, 1, 1, '火炮战军械库（10人英雄） - 源力核心'),
(28057, 63100, 0, 75, 0, 1, 5, 1, 1, '火炮战军械库（10人英雄） - 源力碎片'),
(28090, 63101, 0, 50, 0, 1, 5, 1, 1, '火炮战军械库（25人英雄） - 源力核心'),
(28090, 63100, 0, 50, 0, 1, 5, 1, 1, '火炮战军械库（25人英雄） - 源力碎片');

-- 死亡使者萨鲁法尔宝箱（Deathbringer's Cache）
DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (28046, 28058, 28074, 28088) AND `Item` IN (63100, 63101);
INSERT INTO `gameobject_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(28046, 63101, 0, 5, 0, 1, 5, 1, 1, '死亡使者萨鲁法尔宝箱（10人普通） - 源力核心'),
(28046, 63100, 0, 95, 0, 1, 5, 1, 1, '死亡使者萨鲁法尔宝箱（10人普通） - 源力碎片'),
(28074, 63101, 0, 25, 0, 1, 5, 1, 1, '死亡使者萨鲁法尔宝箱（25人普通） - 源力核心'),
(28074, 63100, 0, 75, 0, 1, 5, 1, 1, '死亡使者萨鲁法尔宝箱（25人普通） - 源力碎片'),
(28058, 63101, 0, 25, 0, 1, 5, 1, 1, '死亡使者萨鲁法尔宝箱（10人英雄） - 源力核心'),
(28058, 63100, 0, 75, 0, 1, 5, 1, 1, '死亡使者萨鲁法尔宝箱（10人英雄） - 源力碎片'),
(28088, 63101, 0, 50, 0, 1, 5, 1, 1, '死亡使者萨鲁法尔宝箱（25人英雄） - 源力核心'),
(28088, 63100, 0, 50, 0, 1, 5, 1, 1, '死亡使者萨鲁法尔宝箱（25人英雄） - 源力碎片');

-- 瓦莉瑟瑞娅·梦行者宝箱（Cache of the Dreamwalker）
DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (28052, 28064, 28082, 28096) AND `Item` IN (63100, 63101);
INSERT INTO `gameobject_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(28052, 63101, 0, 5, 0, 1, 5, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（10人普通） - 源力核心'),
(28052, 63100, 0, 95, 0, 1, 5, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（10人普通） - 源力碎片'),
(28082, 63101, 0, 25, 0, 1, 5, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（25人普通） - 源力核心'),
(28082, 63100, 0, 75, 0, 1, 5, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（25人普通） - 源力碎片'),
(28064, 63101, 0, 25, 0, 1, 5, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（10人英雄） - 源力核心'),
(28064, 63100, 0, 75, 0, 1, 5, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（10人英雄） - 源力碎片'),
(28096, 63101, 0, 50, 0, 1, 5, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（25人英雄） - 源力核心'),
(28096, 63100, 0, 50, 0, 1, 5, 1, 1, '瓦莉瑟瑞娅·梦行者宝箱（25人英雄） - 源力碎片');

-- ============================================================
-- 二、TOC（十字军的试炼）——源力核心 / 源力碎片（同组 5，互斥）
-- ============================================================

-- 阿努巴拉克（Anub'arak）——尾王
DELETE FROM `creature_loot_template` WHERE `Entry` IN (34564, 34566, 35615, 35616) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(34564, 63101, 0, 25, 0, 1, 5, 1, 1, '阿努巴拉克（10人普通） - 源力核心'),
(34564, 63100, 0, 75, 0, 1, 5, 1, 1, '阿努巴拉克（10人普通） - 源力碎片'),
(34566, 63101, 0, 50, 0, 1, 5, 1, 1, '阿努巴拉克（25人普通） - 源力核心'),
(34566, 63100, 0, 50, 0, 1, 5, 1, 1, '阿努巴拉克（25人普通） - 源力碎片'),
(35615, 63101, 0, 40, 0, 1, 5, 1, 1, '阿努巴拉克（10人英雄） - 源力核心'),
(35615, 63100, 0, 60, 0, 1, 5, 1, 1, '阿努巴拉克（10人英雄） - 源力碎片'),
(35616, 63101, 0, 50, 0, 1, 5, 4, 2, '阿努巴拉克（25人英雄） - 源力核心'),
(35616, 63100, 0, 50, 0, 1, 5, 1, 5, '阿努巴拉克（25人英雄） - 源力碎片');

-- 冰吼（Icehowl）——诺森德野兽
DELETE FROM `creature_loot_template` WHERE `Entry` IN (35447, 35448, 35449) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(35447, 63101, 0, 25, 0, 1, 5, 1, 1, '冰吼·诺森德野兽（25人普通） - 源力核心'),
(35447, 63100, 0, 75, 0, 1, 5, 1, 1, '冰吼·诺森德野兽（25人普通） - 源力碎片'),
(35448, 63101, 0, 15, 0, 1, 5, 1, 1, '冰吼·诺森德野兽（10人英雄） - 源力核心'),
(35448, 63100, 0, 85, 0, 1, 5, 1, 1, '冰吼·诺森德野兽（10人英雄） - 源力碎片'),
(35449, 63101, 0, 50, 0, 1, 5, 1, 1, '冰吼·诺森德野兽（25人英雄） - 源力核心'),
(35449, 63100, 0, 50, 0, 1, 5, 1, 1, '冰吼·诺森德野兽（25人英雄） - 源力碎片');

-- 加拉克苏斯大王（Lord Jaraxxus）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (35216, 35268, 35269) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(35216, 63101, 0, 25, 0, 1, 5, 1, 1, '加拉克苏斯大王（25人普通） - 源力核心'),
(35216, 63100, 0, 75, 0, 1, 5, 1, 1, '加拉克苏斯大王（25人普通） - 源力碎片'),
(35268, 63101, 0, 15, 0, 1, 5, 1, 1, '加拉克苏斯大王（10人英雄） - 源力核心'),
(35268, 63100, 0, 85, 0, 1, 5, 1, 1, '加拉克苏斯大王（10人英雄） - 源力碎片'),
(35269, 63101, 0, 50, 0, 1, 5, 1, 1, '加拉克苏斯大王（25人英雄） - 源力核心'),
(35269, 63100, 0, 50, 0, 1, 5, 1, 1, '加拉克苏斯大王（25人英雄） - 源力碎片');

-- 瓦格里双子（Val'kyr Twins）
DELETE FROM `creature_loot_template` WHERE `Entry` IN (35347, 35349, 35351) AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(35347, 63101, 0, 25, 0, 1, 5, 1, 1, '瓦格里双子（25人普通） - 源力核心'),
(35347, 63100, 0, 75, 0, 1, 5, 1, 1, '瓦格里双子（25人普通） - 源力碎片'),
(35351, 63101, 0, 15, 0, 1, 5, 1, 1, '瓦格里双子（10人英雄） - 源力核心'),
(35351, 63100, 0, 85, 0, 1, 5, 1, 1, '瓦格里双子（10人英雄） - 源力碎片'),
(35349, 63101, 0, 50, 0, 1, 5, 1, 1, '瓦格里双子（25人英雄） - 源力核心'),
(35349, 63100, 0, 50, 0, 1, 5, 1, 2, '瓦格里双子（25人英雄） - 源力碎片');

-- 阵营冠军宝箱（Champions' Cache）
DELETE FROM `gameobject_loot_template` WHERE `Entry` IN (27503, 27335, 27356) AND `Item` IN (63100, 63101);
INSERT INTO `gameobject_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(27503, 63101, 0, 25, 0, 1, 5, 1, 1, '阵营冠军宝箱（25人普通） - 源力核心'),
(27503, 63100, 0, 75, 0, 1, 5, 1, 1, '阵营冠军宝箱（25人普通） - 源力碎片'),
(27335, 63101, 0, 15, 0, 1, 5, 1, 1, '阵营冠军宝箱（10人英雄） - 源力核心'),
(27335, 63100, 0, 85, 0, 1, 5, 1, 1, '阵营冠军宝箱（10人英雄） - 源力碎片'),
(27356, 63101, 0, 50, 0, 1, 5, 1, 1, '阵营冠军宝箱（25人英雄） - 源力核心'),
(27356, 63100, 0, 50, 0, 1, 5, 1, 1, '阵营冠军宝箱（25人英雄） - 源力碎片');

-- ============================================================
-- 三、卡拉赞——源力核心 / 源力碎片（同组 5，互斥）
-- ============================================================

-- 玛克扎尔王子（Prince Malchezaar）
DELETE FROM `creature_loot_template` WHERE `Entry` = 15690 AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(15690, 63101, 0, 25, 0, 1, 5, 1, 1, '玛克扎尔王子 - 源力核心'),
(15690, 63100, 0, 75, 0, 1, 5, 1, 1, '玛克扎尔王子 - 源力碎片');

-- 夜之魇（Nightbane）
DELETE FROM `creature_loot_template` WHERE `Entry` = 17225 AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(17225, 63101, 0, 50, 0, 1, 5, 1, 1, '夜之魇 - 源力核心'),
(17225, 63100, 0, 50, 0, 1, 5, 1, 1, '夜之魇 - 源力碎片');

-- ============================================================
-- 四、祖阿曼——源力核心 / 源力碎片（同组 5，互斥）
-- ============================================================

-- 祖尔金（Zul'jin）
DELETE FROM `creature_loot_template` WHERE `Entry` = 23863 AND `Item` IN (63100, 63101);
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(23863, 63101, 0, 25, 0, 1, 5, 1, 1, '祖尔金 - 源力核心'),
(23863, 63100, 0, 75, 0, 1, 5, 1, 1, '祖尔金 - 源力碎片');

-- ============================================================
-- 五、卡拉赞其他 BOSS——源力碎片（独立掉落 40%）
-- ============================================================

DELETE FROM `creature_loot_template` WHERE `Entry` IN (16152, 15687, 16457, 17521, 17533, 17534, 18168, 15691, 15688, 16524, 15689) AND `Item` = 63100;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(16152, 63100, 0, 60, 0, 1, 0, 1, 1, '猎人阿图门 - 源力碎片'),
(15687, 63100, 0, 60, 0, 1, 0, 1, 1, '莫罗斯 - 源力碎片'),
(16457, 63100, 0, 60, 0, 1, 0, 1, 1, '贞洁圣女 - 源力碎片'),
(17521, 63100, 0, 60, 0, 1, 0, 1, 1, '大野狼（歌剧院·小红帽） - 源力碎片'),
(17533, 63100, 0, 60, 0, 1, 0, 1, 1, '罗密欧（歌剧院） - 源力碎片'),
(18168, 63100, 0, 60, 0, 1, 0, 1, 1, '老巫婆（歌剧院·绿野仙踪） - 源力碎片'),
(15691, 63100, 0, 60, 0, 1, 0, 1, 1, '馆长 - 源力碎片'),
(15688, 63100, 0, 60, 0, 1, 0, 1, 1, '特雷斯坦·邪蹄 - 源力碎片'),
(16524, 63100, 0, 60, 0, 1, 0, 1, 1, '埃兰之影 - 源力碎片'),
(15689, 63100, 0, 60, 0, 1, 0, 1, 1, '虚空幽龙 - 源力碎片');

-- ============================================================
-- 六、祖阿曼其他 BOSS——源力碎片（独立掉落 25%）
-- ============================================================

DELETE FROM `creature_loot_template` WHERE `Entry` IN (23574, 23576, 23577, 23578, 24239) AND `Item` = 63100;
INSERT INTO `creature_loot_template` (`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`) VALUES
(23574, 63100, 0, 40, 0, 1, 0, 1, 1, '阿基尔森 - 源力碎片'),
(23576, 63100, 0, 40, 0, 1, 0, 1, 1, '纳洛拉克 - 源力碎片'),
(23577, 63100, 0, 40, 0, 1, 0, 1, 1, '哈拉齐 - 源力碎片'),
(23578, 63100, 0, 40, 0, 1, 0, 1, 1, '加亚莱 - 源力碎片'),
(24239, 63100, 0, 40, 0, 1, 0, 1, 1, '妖术领主玛拉卡斯 - 源力碎片');
