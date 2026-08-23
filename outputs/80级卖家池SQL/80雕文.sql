DELETE FROM `mod_ah_seller_pool_items` WHERE `pool_id` = 806;
DELETE FROM `mod_ah_seller_pool` WHERE `id` = 806;

INSERT INTO `mod_ah_seller_pool`
  (`id`, `pool_type`, `enabled`, `buyout_price_gold`, `bid_price_gold`,
   `price_up_pct`, `price_down_pct`, `price_step_pct`, `max_count`,
   `restock_interval`, `restock_count`, `duration_hours`, `stack_count`, `comment`)
VALUES
  (806, 2, 1, 150.00, 0.00, 30.00, 20.00, 5.00, 90, 600, 5, 12, 1,
   '80级雕文');

INSERT INTO `mod_ah_seller_pool_items`
  (`pool_id`, `item_id`, `buyout_price_gold`, `bid_price_gold`)
SELECT 806, `entry`, 0.00, 0.00
FROM `item_template` 
WHERE `class` = 16   
  AND `Quality` =1
	and spellid_1>0