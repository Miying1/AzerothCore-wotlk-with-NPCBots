-- =============================================================
-- mod-ah-seller 拍卖行卖家池模块
-- 库：acore_world
-- 说明：
--   · 类型池（pool_type=1）：按 class/subclass/等级/品质筛一类物品，全池统一价
--   · Entry 池（pool_type=2）：按 entry 指定一组物品，每个 entry 可单独设价
--   · 每池独立配置：最多挂架数、补货间隔、单次补货量、上架时长、堆叠数、上下浮比例
--   · 不分阵营，统一挂中立拍卖行
-- =============================================================

DROP TABLE IF EXISTS `ah_seller_pool`;
CREATE TABLE `ah_seller_pool` (
  `id`                INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pool_type`         TINYINT      NOT NULL DEFAULT '1'   COMMENT '1=类型池 2=Entry池',
  `enabled`           TINYINT      NOT NULL DEFAULT '1'   COMMENT '1=启用 0=停用',
  -- 类型池筛选（pool_type=1 生效）
  `item_class`        TINYINT      NOT NULL DEFAULT '-1'  COMMENT '物品大类(ItemClass)，-1=不限',
  `item_subclass`     INT          NOT NULL DEFAULT '-1'  COMMENT '物品子类(ItemSubClass)，-1=不限',
  `min_item_level`    INT          NOT NULL DEFAULT '0'   COMMENT '物品等级下限，0=不限',
  `max_item_level`    INT          NOT NULL DEFAULT '0'   COMMENT '物品等级上限，0=不限',
  `min_quality`       TINYINT      NOT NULL DEFAULT '0'   COMMENT '品质下限(0灰..6黄)，0=不限',
  `max_quality`       TINYINT      NOT NULL DEFAULT '0'   COMMENT '品质上限，0=不限',
  -- 定价（金币/每件）
  `buyout_price_gold` DECIMAL(12,2) NOT NULL DEFAULT '0.00' COMMENT '统一一口价；0=用物品SellPrice(为0则忽略该物品)',
  `bid_price_gold`    DECIMAL(12,2) NOT NULL DEFAULT '0.00' COMMENT '起拍价；0=按一口价×80%',
  -- 浮动控制
  `price_up_pct`      DECIMAL(5,2)  NOT NULL DEFAULT '0.00' COMMENT '卖得好最多上浮%；0=不浮动(不记录出售记录)',
  `price_down_pct`    DECIMAL(5,2)  NOT NULL DEFAULT '0.00' COMMENT '卖得差最多下浮%',
  `price_step_pct`    DECIMAL(5,2)  NOT NULL DEFAULT '3.00' COMMENT '每次成交/过期调整的步进%',
  -- 上架策略
  `max_count`         INT UNSIGNED NOT NULL DEFAULT '0'   COMMENT '同时最多挂架堆数，0=不限制',
  `restock_interval`  INT UNSIGNED NOT NULL DEFAULT '600' COMMENT '补货间隔(秒)',
  `restock_count`     INT UNSIGNED NOT NULL DEFAULT '1'   COMMENT '单次补货数量(受max_count封顶)',
  `duration_hours`    INT UNSIGNED NOT NULL DEFAULT '12'  COMMENT '上架持续小时数',
  `stack_count`       INT UNSIGNED NOT NULL DEFAULT '0'   COMMENT '每堆数量；0=不堆叠(1件/堆)，不支持堆叠的物品忽略',
  `comment`           VARCHAR(100) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 存量数据兜底：将仍在使用默认 5% 步进的池子统一调整为 3%
UPDATE `ah_seller_pool` SET `price_step_pct` = 3.00 WHERE `price_step_pct` = 5.00;

DROP TABLE IF EXISTS `ah_seller_pool_items`;
CREATE TABLE `ah_seller_pool_items` (
  `id`                INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `pool_id`           INT UNSIGNED NOT NULL,
  `item_id`           INT UNSIGNED NOT NULL             COMMENT '物品entry',
  `buyout_price_gold` DECIMAL(12,2) NOT NULL DEFAULT '0.00' COMMENT '该entry单独一口价；0=用池子默认价',
  `bid_price_gold`    DECIMAL(12,2) NOT NULL DEFAULT '0.00' COMMENT '该entry单独起拍价；0=用池子默认价',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_pool_item` (`pool_id`, `item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =============================================================
-- 示例（按需取消注释 / 修改）
-- =============================================================

-- ① 类型池：80级蓝/紫宝石，统一一口价 25 金，最多 50 堆，每 10 分钟补 5 件，挂 12 小时，浮动±20%
-- INSERT INTO ah_seller_pool
--   (pool_type, item_class, min_item_level, max_item_level, min_quality, max_quality,
--    buyout_price_gold, price_up_pct, price_down_pct, max_count, restock_interval, restock_count, duration_hours, comment)
-- VALUES (1, 3, 80, 80, 3, 4, 25.00, 20.00, 20.00, 50, 600, 5, 12, '80级蓝紫宝石');

-- ② 类型池：附魔卷轴（class=0 消耗品, subclass=6 物品强化），统一一口价 15 金，最多 100 堆，堆叠 20
-- INSERT INTO ah_seller_pool
--   (pool_type, item_class, item_subclass, buyout_price_gold, max_count, restock_interval, restock_count, stack_count, comment)
-- VALUES (1, 0, 6, 15.00, 100, 600, 10, 20, '附魔卷轴');

-- ③ Entry 池：指定 3 件物品，各配不同价
-- INSERT INTO ah_seller_pool (pool_type, max_count, restock_interval, restock_count, comment)
-- VALUES (2, 30, 300, 3, '自定义装备组');
--
-- INSERT INTO ah_seller_pool_items (pool_id, item_id, buyout_price_gold) VALUES
-- (3, 93003, 10.00),
-- (3, 100001, 50.00),
-- (3, 100002, 80.00);

-- =============================================================
-- 预核实查询
-- =============================================================

-- 宝石的 class/subclass/等级分布
-- SELECT class, subclass, quality, itemlevel, requiredlevel, COUNT(*) AS cnt
-- FROM item_template WHERE class = 3
-- GROUP BY class, subclass, quality, itemlevel, requiredlevel ORDER BY itemlevel DESC;

-- 附魔卷轴的 class/subclass 分布
-- SELECT class, subclass, itemlevel, requiredlevel, COUNT(*) AS cnt
-- FROM item_template WHERE class = 0 AND subclass = 6
-- GROUP BY class, subclass, itemlevel, requiredlevel ORDER BY itemlevel DESC;
