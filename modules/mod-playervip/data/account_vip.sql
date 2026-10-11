-- 账号权益表
CREATE TABLE IF NOT EXISTS `account_vip` (
  `account_id` int unsigned NOT NULL,
  `vip_level` int unsigned NOT NULL DEFAULT '0',
  `gold_loot_bonus` smallint unsigned NOT NULL DEFAULT '0',
  `skill_max_count` int unsigned NOT NULL DEFAULT '4',
  `extra_botcount` int unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

ALTER TABLE `account_vip`
  ADD COLUMN IF NOT EXISTS `extra_botcount` int unsigned NOT NULL DEFAULT '0' AFTER `skill_max_count`;
