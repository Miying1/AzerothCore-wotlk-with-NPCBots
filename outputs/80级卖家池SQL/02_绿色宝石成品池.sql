-- 80级内容：绿色颜色宝石成品池
-- 说明：绿色宝石按 Forest Emerald / Eye of Zul 的颜色筛选；只取 ItemLevel=80、单颗成品(stackable=1)，兼容两者在数据库中的不同品质值。
-- 池ID：802

DELETE FROM `ah_seller_pool_items` WHERE `pool_id` = 802;
DELETE FROM `ah_seller_pool` WHERE `id` = 802;

INSERT INTO `ah_seller_pool`
  (`id`, `pool_type`, `enabled`, `buyout_price_gold`, `bid_price_gold`,
   `price_up_pct`, `price_down_pct`, `price_step_pct`, `max_count`,
   `restock_interval`, `restock_count`, `duration_hours`, `stack_count`, `comment`)
VALUES
  (802, 2, 1, 8.00, 6.00, 15.00, 15.00, 3.00, 50, 300, 10, 12, 1,
   '80级绿色颜色宝石成品：Forest Emerald / Eye of Zul');

INSERT INTO `ah_seller_pool_items`
  (`pool_id`, `item_id`, `buyout_price_gold`, `bid_price_gold`)
SELECT 802, `entry`, 0.00, 0.00
FROM `item_template`
WHERE `class` = 3
  AND `ItemLevel` = 80
  AND `Quality` IN (2, 3)
  AND `stackable` = 1
  AND `bonding` = 0
  AND `RequiredSkill` = 0
  AND (`name` LIKE '%Forest Emerald' OR `name` LIKE '%Eye of Zul');
