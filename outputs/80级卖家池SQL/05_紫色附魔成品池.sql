-- 80级内容：紫色品质附魔卷轴成品池
-- 池ID：805
-- 当前数据库中没有 Quality=4 且名称为 Scroll of Enchant 的记录，因此默认禁用，避免空池持续补货。
-- 若后续新增紫色品质附魔卷轴，可按下方查询结果确认后再启用。

DELETE FROM `ah_seller_pool_items` WHERE `pool_id` = 805;
DELETE FROM `ah_seller_pool` WHERE `id` = 805;

INSERT INTO `ah_seller_pool`
  (`id`, `pool_type`, `enabled`, `buyout_price_gold`, `bid_price_gold`,
   `price_up_pct`, `price_down_pct`, `price_step_pct`, `max_count`,
   `restock_interval`, `restock_count`, `duration_hours`, `stack_count`, `comment`)
VALUES
  (805, 2, 0, 120.00, 96.00, 20.00, 20.00, 3.00, 20, 900, 2, 12, 5,
   '紫色品质附魔卷轴：当前数据库无符合条件成品，默认禁用');

-- 预核查：
-- SELECT entry, name, ItemLevel, Quality, stackable, bonding
-- FROM item_template
-- WHERE class=0 AND subclass=6 AND Quality=4 AND name LIKE 'Scroll of Enchant%';
