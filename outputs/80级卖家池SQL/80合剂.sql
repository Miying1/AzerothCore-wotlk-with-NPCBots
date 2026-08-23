DELETE FROM `mod_ah_seller_pool_items` WHERE `pool_id` = 805;
DELETE FROM `mod_ah_seller_pool` WHERE `id` = 805;

INSERT INTO `mod_ah_seller_pool`
  (`id`, `pool_type`, `enabled`, `buyout_price_gold`, `bid_price_gold`,
   `price_up_pct`, `price_down_pct`, `price_step_pct`, `max_count`,
   `restock_interval`, `restock_count`, `duration_hours`, `stack_count`, `comment`)
VALUES
  (805, 2, 1, 150.00, 0.00, 10.00, 30.00, 3.00, 50, 1200, 5, 12, 1,
   '80级合剂');

INSERT INTO `mod_ah_seller_pool_items`
  (`pool_id`, `item_id`, `buyout_price_gold`, `bid_price_gold`)
SELECT 805, `entry`, 0.00, 0.00
FROM `item_template` 
WHERE `class` = 0 and subclass=3   
  AND `RequiredLevel` >= 75 and ItemLevel=80
  AND `Quality` =1