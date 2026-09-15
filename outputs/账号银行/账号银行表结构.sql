-- 账号银行扩展：新增账号维度物品映射表（镜像 character_inventory 的银行部分语义）
-- 个人银行保持不变，账号银行为纯增量功能
CREATE TABLE IF NOT EXISTS `account_bank_item` (
  `account_id` int unsigned NOT NULL DEFAULT 0 COMMENT '账号ID（来自 auth 库）',
  `bag` int unsigned NOT NULL DEFAULT 0 COMMENT '所在袋子：0=银行顶层槽位，否则为银行背包物品GUID',
  `slot` tinyint unsigned NOT NULL DEFAULT 0 COMMENT '槽位索引：39~74 为顶层，0~N 为背包内部',
  `item` int unsigned NOT NULL DEFAULT 0 COMMENT '物品GUID（item_instance.guid）',
  PRIMARY KEY (`item`),
  KEY `idx_account_bag_slot` (`account_id`,`bag`,`slot`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- 账号银行扩展：账号维度已购买背包槽数（同账号所有角色共享）
CREATE TABLE IF NOT EXISTS `account_bank_slots` (
  `account_id` int unsigned NOT NULL DEFAULT 0 COMMENT '账号ID（来自 auth 库）',
  `slot_count` tinyint unsigned NOT NULL DEFAULT 0 COMMENT '已购买的银行背包槽数（0~7）',
  PRIMARY KEY (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
