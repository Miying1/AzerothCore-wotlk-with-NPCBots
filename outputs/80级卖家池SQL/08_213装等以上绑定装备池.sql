-- 80级内容：ItemLevel>=213 且装备绑定（bonding=1）的武器/护甲池
-- 池ID：808
-- 价格按物品等级分档写入 Entry 覆盖价，避免所有高低等级装备同价。

DELETE FROM `ah_seller_pool_items` WHERE `pool_id` = 808;
DELETE FROM `ah_seller_pool` WHERE `id` = 808;

INSERT INTO `ah_seller_pool`
  (`id`, `pool_type`, `enabled`, `buyout_price_gold`, `bid_price_gold`,
   `price_up_pct`, `price_down_pct`, `price_step_pct`, `max_count`,
   `restock_interval`, `restock_count`, `duration_hours`, `stack_count`, `comment`)
VALUES
  (808, 2, 1, 0.00, 0.00, 15.00, 15.00, 3.00, 40, 900, 2, 12, 1,
   '80级 ItemLevel>=213 且装备绑定的武器/护甲；Entry价格按装等分档');

INSERT INTO `ah_seller_pool_items`
  (`pool_id`, `item_id`, `buyout_price_gold`, `bid_price_gold`)
SELECT
  808,
  `entry`,
  CASE
    WHEN `ItemLevel` >= 284 THEN 20000.00
    WHEN `ItemLevel` >= 277 THEN 12000.00
    WHEN `ItemLevel` >= 264 THEN 8000.00
    WHEN `ItemLevel` >= 258 THEN 6000.00
    WHEN `ItemLevel` >= 252 THEN 4500.00
    WHEN `ItemLevel` >= 245 THEN 3000.00
    WHEN `ItemLevel` >= 232 THEN 1800.00
    ELSE 1000.00
  END,
  0.00
FROM `item_template`
WHERE `class` IN (2, 4)
  AND `ItemLevel` >= 213
  AND `bonding` = 1
  AND `stackable` = 1
  AND `name` NOT LIKE '%DO NOT USE%'
  AND `name` NOT LIKE 'QA %'
  AND `name` NOT LIKE '%Test%';
