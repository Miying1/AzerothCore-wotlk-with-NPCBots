-- ============================================================================
-- 五人英雄裂隙：索拉查盆地（Sholazar Basin）1000 个入口刷新点（暂不执行）
-- ----------------------------------------------------------------------------
-- 区域：诺森德 · 索拉查盆地（map_id=571，area_id=3711，zoneId=3711）
-- 数据来源（均为原版游戏真实落地坐标，z 为实测地面高度）：
--   * `gameobject` 表 zoneId=3711 的 920 个物体坐标（精确属于索拉查盆地）；
--   * `creature` 表 map=571 且落在盆地范围（x 4707~6700，y 3498~6038）的 2282 个生物坐标；
--   两者合并约 3202 个候选坐标，随机抽取 1000 个。
-- 偏移规则：
--   * X/Y 在原坐标基础上做 ±5 码随机偏移：position_x + (RAND()*2-1)*5；
--   * Z（地面高度）与朝向 O 保持原值不变——±5 码范围内地面高差可忽略，仍为真实落地高度。
-- 编号：point_id 从 10001 连续编到 11000（region_id=2 专用区间，
--       与暴风峭壁 region_id=1 的 point_id 1~35 互不冲突）。
-- 说明：本脚本用 INSERT ... SELECT + RAND() 生成坐标，每次执行得到的随机偏移不同；
--       执行一次后坐标即固定写入表内；再次执行会先 DELETE 再重新随机生成。
-- 依赖：需先执行「裂隙入口点刷新SQL（暂不执行）.sql」建立表结构与三档入口生物模板。
-- 审核：执行后本文件末尾的 SELECT 会列出该区域点数与坐标范围供核对。
-- ============================================================================

-- ----------------------------------------------------------------------------
-- 1. 区域配置（region_id=2，索拉查盆地）
--    中心坐标取官方传送点 SholazarBasin（4857.14, 5529.11, -55.58）。
-- ----------------------------------------------------------------------------
DELETE FROM `heroic_dungeon_rift_spawn_region` WHERE `region_id`=2;
INSERT INTO `heroic_dungeon_rift_spawn_region`
  (`region_id`,`region_name`,`map_id`,`area_id`,`center_x`,`center_y`,`center_z`,
   `t1_min_count`,`t1_max_count`,`t2_min_count`,`t2_max_count`,`t3_min_count`,`t3_max_count`,`enabled`,`remark`)
VALUES
  (2,'索拉查盆地',571,3711,4857.14,5529.11,-55.58,10,20,5,10,2,5,1,
   '索拉查盆地：1000 个刷新点，坐标来自原版 creature/gameobject 并做 XY ±5 码偏移');

-- ----------------------------------------------------------------------------
-- 2. 1000 个刷新点（region_id=2，point_id 10001~11000）
-- ----------------------------------------------------------------------------
DELETE FROM `heroic_dungeon_rift_spawn_point` WHERE `region_id`=2;

INSERT INTO `heroic_dungeon_rift_spawn_point`
  (`point_id`,`region_id`,`x`,`y`,`z`,`o`,`enabled`,`remark`)
SELECT
  10000 + ROW_NUMBER() OVER (),
  2,
  ROUND(src.x, 2),
  ROUND(src.y, 2),
  ROUND(src.z, 2),
  ROUND(src.o, 4),
  1,
  '索拉查盆地 自动生成点'
FROM (
  SELECT x, y, z, o
  FROM (
    -- 生物坐标（按索拉查盆地范围筛选）
    SELECT
      position_x + (RAND()*2-1)*5 AS x,
      position_y + (RAND()*2-1)*5 AS y,
      position_z                AS z,
      orientation               AS o
    FROM `creature`
    WHERE `map`=571 AND `position_x` BETWEEN 4707 AND 6700 AND `position_y` BETWEEN 3498 AND 6038
    UNION ALL
    -- 物体坐标（zoneId=3711 精确属于索拉查盆地）
    SELECT
      position_x + (RAND()*2-1)*5 AS x,
      position_y + (RAND()*2-1)*5 AS y,
      position_z                AS z,
      orientation               AS o
    FROM `gameobject`
    WHERE `map`=571 AND `zoneId`=3711
  ) raw
  ORDER BY RAND()
  LIMIT 1000
) src;

-- ----------------------------------------------------------------------------
-- 3. 审核查询
-- ----------------------------------------------------------------------------
SELECT `region_id`,`region_name`,`map_id`,`area_id`,`enabled`
FROM `heroic_dungeon_rift_spawn_region` WHERE `region_id`=2;
SELECT COUNT(*) AS `point_count`
FROM `heroic_dungeon_rift_spawn_point` WHERE `region_id`=2;
SELECT
  ROUND(MIN(`x`),1) AS `min_x`, ROUND(MAX(`x`),1) AS `max_x`,
  ROUND(MIN(`y`),1) AS `min_y`, ROUND(MAX(`y`),1) AS `max_y`,
  ROUND(MIN(`z`),1) AS `min_z`, ROUND(MAX(`z`),1) AS `max_z`
FROM `heroic_dungeon_rift_spawn_point` WHERE `region_id`=2;
