-- 为 reward_shop 表增加多物品发放所需的字段（action_data2/quantity2/action_data3/quantity3）
ALTER TABLE `reward_shop`
  ADD COLUMN `action_data2` int(11) NOT NULL DEFAULT '0' AFTER `quantity`,
  ADD COLUMN `quantity2` int(11) NOT NULL DEFAULT '0' AFTER `action_data2`,
  ADD COLUMN `action_data3` int(11) NOT NULL DEFAULT '0' AFTER `quantity2`,
  ADD COLUMN `quantity3` int(11) NOT NULL DEFAULT '0' AFTER `action_data3`;
