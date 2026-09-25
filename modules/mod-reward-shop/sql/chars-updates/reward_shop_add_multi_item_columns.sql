-- 为 reward_shop 表增加三组 action 及其奖励数据字段
ALTER TABLE `reward_shop`
  ADD COLUMN `action2` int(11) NOT NULL DEFAULT '0' AFTER `quantity`,
  ADD COLUMN `action_data2` int(11) NOT NULL DEFAULT '0' AFTER `action2`,
  ADD COLUMN `quantity2` int(11) NOT NULL DEFAULT '0' AFTER `action_data2`,
  ADD COLUMN `action3` int(11) NOT NULL DEFAULT '0' AFTER `quantity2`,
  ADD COLUMN `action_data3` int(11) NOT NULL DEFAULT '0' AFTER `action3`,
  ADD COLUMN `quantity3` int(11) NOT NULL DEFAULT '0' AFTER `action_data3`;
