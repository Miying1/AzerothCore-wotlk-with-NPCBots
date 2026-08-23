-- =============================================================
-- mod-ah-bot 上架规则表（卖家"规则模式"· 存量上限版）
-- 库：acore_world
-- 说明：
--   表为空  => 卖家保持原行为（黑名单 + 全局过滤器）；
--   表非空  => 卖家只上架命中任一条规则的物品，并支持：
--              · 指定一口价/起拍价（金币/每件）
--              · 每类物品的存量上限 max_count（挂单堆数），防止某类被无限上架
-- 依赖：需配合 C++ 改造（见《mod-ah-bot上架规则改造设计方案.md》）
-- =============================================================

DROP TABLE IF EXISTS `mod_auctionhousebot_seller_rules`;
CREATE TABLE `mod_auctionhousebot_seller_rules` (
  `id`                 INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `item_id`            INT UNSIGNED NOT NULL DEFAULT '0'     COMMENT '精确物品entry，非0=精确匹配(忽略其余筛选条件)',
  `item_class`         TINYINT      NOT NULL DEFAULT '-1'    COMMENT '物品大类(ItemClass)，-1=不限',
  `item_subclass`      INT          NOT NULL DEFAULT '-1'    COMMENT '物品子类(ItemSubClass)，-1=不限',
  `min_item_level`     INT          NOT NULL DEFAULT '0'     COMMENT '物品等级下限，0=不限',
  `max_item_level`     INT          NOT NULL DEFAULT '0'     COMMENT '物品等级上限，0=不限',
  `min_required_level` INT          NOT NULL DEFAULT '0'     COMMENT '需求等级下限，0=不限',
  `max_required_level` INT          NOT NULL DEFAULT '0'     COMMENT '需求等级上限，0=不限',
  `min_quality`        TINYINT      NOT NULL DEFAULT '0'     COMMENT '品质下限(0灰..6黄)，0=不限',
  `max_quality`        TINYINT      NOT NULL DEFAULT '0'     COMMENT '品质上限，0=不限',
  `buyout_price_gold`  DECIMAL(12,2) NOT NULL DEFAULT '0.00' COMMENT '一口价(金币/每件)，0=用默认公式',
  `bid_price_gold`     DECIMAL(12,2) NOT NULL DEFAULT '0.00' COMMENT '起拍价(金币/每件)，0=按一口价×默认起拍比例',
  `max_count`          INT UNSIGNED NOT NULL DEFAULT '0'     COMMENT '该类物品在该AH的存量上限(挂单堆数)，0=不设上限',
  `comment`            VARCHAR(100) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =============================================================
-- 示例规则（按需取消注释 / 修改 entry、价格与上限）
-- =============================================================

-- ① 精确物品：一口价 10 金、起拍 8 金，最多 20 堆
-- INSERT INTO mod_auctionhousebot_seller_rules
--   (item_id, buyout_price_gold, bid_price_gold, max_count, comment)
-- VALUES (93003, 10.00, 8.00, 20, '指位技能');

-- ② 80级蓝/紫宝石：一口价 25 金，最多 50 堆
-- INSERT INTO mod_auctionhousebot_seller_rules
--   (item_class, min_item_level, max_item_level, min_quality, max_quality,
--    buyout_price_gold, max_count, comment)
-- VALUES (3, 80, 80, 3, 4, 25.00, 50, '80级蓝紫宝石');

-- ③ 附魔卷轴（任意等级）：默认定价，最多 100 堆
-- INSERT INTO mod_auctionhousebot_seller_rules
--   (item_class, item_subclass, max_count, comment)
-- VALUES (0, 6, 100, '附魔卷轴');

-- ④ 80级附魔卷轴：一口价 15 金，最多 80 堆
-- INSERT INTO mod_auctionhousebot_seller_rules
--   (item_class, item_subclass, min_item_level, max_item_level, buyout_price_gold, max_count, comment)
-- VALUES (0, 6, 80, 80, 15.00, 80, '80级附魔卷轴');

-- =============================================================
-- 预核实查询（改动前先确认数据范围与价格）
-- =============================================================

-- 宝石的 class/subclass/等级分布
-- SELECT class, subclass, quality, itemlevel, requiredlevel, COUNT(*) AS cnt
-- FROM item_template WHERE class = 3
-- GROUP BY class, subclass, quality, itemlevel, requiredlevel ORDER BY itemlevel DESC;

-- 附魔卷轴的 class/subclass 分布
-- SELECT class, subclass, itemlevel, requiredlevel, COUNT(*) AS cnt
-- FROM item_template WHERE class = 0 AND subclass = 6
-- GROUP BY class, subclass, itemlevel, requiredlevel ORDER BY itemlevel DESC;
