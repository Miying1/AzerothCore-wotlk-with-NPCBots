-- 80级内容：蓝色品质附魔卷轴成品池
-- 说明：取 ItemLevel>=70 的正式 Scroll of Enchant 成品；排除非卷轴腿部附魔和腰带扣等其它强化物品。
-- 池ID：804

DELETE FROM `ah_seller_pool_items` WHERE `pool_id` = 804;
DELETE FROM `ah_seller_pool` WHERE `id` = 804;

INSERT INTO `ah_seller_pool`
  (`id`, `pool_type`, `enabled`, `buyout_price_gold`, `bid_price_gold`,
   `price_up_pct`, `price_down_pct`, `price_step_pct`, `max_count`,
   `restock_interval`, `restock_count`, `duration_hours`, `stack_count`, `comment`)
VALUES
  (804, 2, 1, 60.00, 48.00, 20.00, 20.00, 3.00, 50, 600, 5, 12, 5,
   '80级蓝色品质附魔卷轴成品');

INSERT INTO `ah_seller_pool_items`
  (`pool_id`, `item_id`, `buyout_price_gold`, `bid_price_gold`)
SELECT 804, `entry`, 0.00, 0.00
FROM `item_template`
WHERE `class` = 0
  AND `subclass` = 6
  AND `Quality` = 3
  AND `ItemLevel` >= 70
  AND `stackable` > 1
  AND `bonding` = 0
  AND `name` LIKE 'Scroll of Enchant%';
