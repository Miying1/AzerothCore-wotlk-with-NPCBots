DROP TABLE IF EXISTS `players_reports_status`;

CREATE TABLE `players_reports_status` (
  `guid` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '玩家低 GUID，作为每名玩家的唯一标识和主键',
  `creation_time` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '首次产生正式反作弊报告时记录的毫秒计时值，用于计算报告平均速率',
  `average` float NOT NULL DEFAULT '0' COMMENT '累计正式报告平均速率，实际表示报告数量除以累计时间（报告/秒）',
  `total_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '累计正式反作弊报告总数',
  `speed_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '速度作弊报告数量',
  `fly_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '飞行作弊报告数量',
  `jump_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '跳跃作弊报告数量',
  `waterwalk_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '水上行走作弊报告数量',
  `teleportplane_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '平面传送作弊报告数量',
  `climb_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '攀爬作弊报告数量',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='玩家长期累计反作弊报告统计';

DROP TABLE IF EXISTS `daily_players_reports`;
CREATE TABLE `daily_players_reports` (
  `guid` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '玩家低 GUID，作为每名玩家的唯一标识和主键',
  `creation_time` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '达到每日报告阈值时保存的首次正式报告时间快照',
  `average` float NOT NULL DEFAULT '0' COMMENT '达到每日报告阈值时保存的累计报告平均速率快照',
  `total_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '达到每日报告阈值时保存的累计正式报告总数快照',
  `speed_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '达到每日报告阈值时保存的速度作弊报告数快照',
  `fly_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '达到每日报告阈值时保存的飞行作弊报告数快照',
  `jump_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '达到每日报告阈值时保存的跳跃作弊报告数快照',
  `waterwalk_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '达到每日报告阈值时保存的水上行走作弊报告数快照',
  `teleportplane_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '达到每日报告阈值时保存的平面传送作弊报告数快照',
  `climb_reports` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '达到每日报告阈值时保存的攀爬作弊报告数快照',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='达到每日报告阈值的玩家状态及统计快照';