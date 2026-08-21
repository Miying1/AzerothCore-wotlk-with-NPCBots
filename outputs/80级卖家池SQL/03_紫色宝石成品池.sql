-- 80级内容：紫色颜色宝石成品池
-- 说明：紫色宝石按 Dreadstone / Ametrine 成品名筛选；排除原石(stackable>1)、珠宝专业限制物品(RequiredSkill>0)和绑定物品。
-- 池ID：803

DELETE FROM `ah_seller_pool_items` WHERE `pool_id` = 803;
DELETE FROM `ah_seller_pool` WHERE `id` = 803;

INSERT INTO `ah_seller_pool`
  (`id`, `pool_type`, `enabled`, `buyout_price_gold`, `bid_price_gold`,
   `price_up_pct`, `price_down_pct`, `price_step_pct`, `max_count`,
   `restock_interval`, `restock_count`, `duration_hours`, `stack_count`, `comment`)
VALUES
  (803, 2, 1, 45.00, 36.00, 20.00, 20.00, 3.00, 60, 300, 10, 12, 1,
   '80级紫色颜色宝石成品：Dreadstone / Ametrine；排除珠宝专业限制宝石');

INSERT INTO `ah_seller_pool_items`
  (`pool_id`, `item_id`, `buyout_price_gold`, `bid_price_gold`)
SELECT 803, `entry`, 0.00, 0.00
FROM `item_template`
WHERE `class` = 3
  AND `ItemLevel` = 80
  AND `Quality` = 4
  AND `stackable` = 1
  AND `bonding` = 0
  AND `RequiredSkill` = 0
  AND (`name` LIKE '%Dreadstone' OR `name` LIKE '%Ametrine');
