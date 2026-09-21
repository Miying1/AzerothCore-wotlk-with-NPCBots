-- ============================================================================
-- 五人英雄裂隙：区域随机入口点刷新系统（暂不执行）
-- ----------------------------------------------------------------------------
-- 本脚本只新增以下内容，不修改任何既有裂隙数据：
--   1) 区域入口刷新主表 `heroic_dungeon_rift_spawn_region`（含每档 min/max 数量）
--   2) 区域入口刷新点子表 `heroic_dungeon_rift_spawn_point`
--   3) 每周开启时段表 `heroic_dungeon_rift_schedule`（按区域，含 region_id）
--   4) 三档入口生物模板 100510/100511/100512（ScriptName=npc_rift_portal）
-- 另附一份示例区域（暴风峭壁，map_id=571，area_id=67），默认 enabled=0。
--
-- 运行机制（对应 src/server/scripts/HeroicDungeonRift/rift_spawn.cpp）：
--   - 入口外观取自卡拉赞虚空幽龙（Netherspite）的三个陷阱传送门，模板字段为显式字面量：
--       T1 = 绿色（Serenity / 平静）   门体特效光环 30490
--       T2 = 蓝色（Dominance / 统御） 门体特效光环 30491
--       T3 = 红色（Perseverence / 坚韧）门体特效光环 30487
--   - 服务器按区域配置在各刷新点里随机挑选点位生成入口，每个点位同一时刻最多一个入口；
--   - 每档数量有 min/max 两个阈值（“有效入口”指生物仍在内存中的入口，幽灵记录不计入）：
--       * 有效入口数 低于 min  → 立即补齐到 min；一次补齐不成功（点位不足）则退避 30 秒再试；
--       * 其余时候            → 每 5 分钟向 max 补齐一次；
--   - 每个区域按各自的时间表独立开关（时间表是区域级的，不是全服统一的）：
--     窗口开启瞬间该区域直接按 max 刷满；窗口关闭那一刻清空该区域已刷新的全部入口
--     （只在状态切换时清理一次，其余时间不重复清理）。
--     时间表每秒评估一次；若所有启用区域都处于关闭状态，则放宽到每 5 秒一次（节流）。
--     开启前 10 分钟起依次播报“还剩10/5/1分钟开启”；开启与结束各连发两条相同通知
--     （“已开启”/“已关闭”）；通知文案使用该区域的 region_name。
--     某区域没有“启用”的窗口时视为该区域未配置时间表，该区域全天可刷新；
--   - 玩家点击入口选择“进入 Tn 裂隙”，成功进入后该入口立即失效，并在 5 秒后移除；
--   - 运行态只存内存，不写入数据库。
-- 执行前请确认：本次只执行本文件，不要与旧的裂隙 SQL 重复叠加。
-- ============================================================================

-- ============================================================================
-- 1. 区域入口刷新主表
-- ============================================================================
CREATE TABLE IF NOT EXISTS `heroic_dungeon_rift_spawn_region` (
  `region_id` INT UNSIGNED NOT NULL COMMENT '区域配置ID',
  `region_name` VARCHAR(64) NOT NULL DEFAULT '' COMMENT '区域名称（用于全服通知）',
  `map_id` SMALLINT UNSIGNED NOT NULL COMMENT '地图ID',
  `area_id` INT UNSIGNED NOT NULL COMMENT '区域(Zone/Area)ID',
  `center_x` FLOAT NOT NULL DEFAULT 0 COMMENT '区域参考中心X（仅用于定位与运维核对）',
  `center_y` FLOAT NOT NULL DEFAULT 0 COMMENT '区域参考中心Y（仅用于定位与运维核对）',
  `center_z` FLOAT NOT NULL DEFAULT 0 COMMENT '区域参考中心Z（仅用于定位与运维核对）',
  `t1_min_count` TINYINT UNSIGNED NOT NULL DEFAULT 10 COMMENT 'T1最小数量：低于该值立即补齐',
  `t1_max_count` TINYINT UNSIGNED NOT NULL DEFAULT 20 COMMENT 'T1最大数量：常规补充上限',
  `t2_min_count` TINYINT UNSIGNED NOT NULL DEFAULT 5 COMMENT 'T2最小数量：低于该值立即补齐',
  `t2_max_count` TINYINT UNSIGNED NOT NULL DEFAULT 10 COMMENT 'T2最大数量：常规补充上限',
  `t3_min_count` TINYINT UNSIGNED NOT NULL DEFAULT 2 COMMENT 'T3最小数量：低于该值立即补齐',
  `t3_max_count` TINYINT UNSIGNED NOT NULL DEFAULT 5 COMMENT 'T3最大数量：常规补充上限',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '是否启用该区域的入口刷新',
  `remark` VARCHAR(255) NULL DEFAULT NULL COMMENT '备注',
  PRIMARY KEY (`region_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='五人英雄裂隙区域入口刷新配置';

-- ============================================================================
-- 2. 区域入口刷新点子表
-- ============================================================================
CREATE TABLE IF NOT EXISTS `heroic_dungeon_rift_spawn_point` (
  `point_id` INT UNSIGNED NOT NULL COMMENT '刷新点ID',
  `region_id` INT UNSIGNED NOT NULL COMMENT '所属区域配置ID（对应主表 region_id）',
  `x` FLOAT NOT NULL COMMENT 'X',
  `y` FLOAT NOT NULL COMMENT 'Y',
  `z` FLOAT NOT NULL COMMENT 'Z',
  `o` FLOAT NOT NULL DEFAULT 0 COMMENT '朝向',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '是否启用该点位',
  `remark` VARCHAR(255) NULL DEFAULT NULL COMMENT '备注',
  PRIMARY KEY (`point_id`),
  KEY `idx_rift_spawn_point_region` (`region_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='五人英雄裂隙区域入口刷新点';

-- ============================================================================
-- 3. 每周开启时段表（按区域）
--    region_id：所属区域，对应主表 `heroic_dungeon_rift_spawn_region`.`region_id`
--    week_day：-1=每天（0~6 全部生效），0=周日，1=周一，2=周二，3=周三，4=周四，5=周五，6=周六
--    start/end 为当日时间；当 end <= start 时视为跨零点窗口（延续到次日）。
--    同一区域可配置多行取并集，各区域的时间表互相独立；
--    某个区域没有任何 enabled=1 的行时，该区域全天可刷新（不参与开启/关闭通知）。
--    若该表此前已按 `week_day` TINYINT UNSIGNED 建过，需先改列类型以支持 -1：
--    ALTER TABLE `heroic_dungeon_rift_schedule` MODIFY `week_day`
--      TINYINT NOT NULL COMMENT '星期：-1=每天(0~6),0=周日,1=周一..6=周六';
-- ============================================================================
CREATE TABLE IF NOT EXISTS `heroic_dungeon_rift_schedule` (
  `schedule_id` INT UNSIGNED NOT NULL COMMENT '时段ID',
  `region_id` INT UNSIGNED NOT NULL COMMENT '所属区域配置ID（对应主表 region_id）',
  `week_day` TINYINT NOT NULL COMMENT '星期：-1=每天(0~6),0=周日,1=周一..6=周六',
  `start_hour` TINYINT UNSIGNED NOT NULL COMMENT '开始小时 [0,23]',
  `start_minute` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '开始分钟 [0,59]',
  `end_hour` TINYINT UNSIGNED NOT NULL COMMENT '结束小时 [0,23]',
  `end_minute` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '结束分钟 [0,59]',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '是否启用该窗口',
  `remark` VARCHAR(255) NULL DEFAULT NULL COMMENT '备注',
  PRIMARY KEY (`schedule_id`),
  KEY `idx_rift_schedule_region` (`region_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='五人英雄裂隙每周开启时段（按区域）';

-- ============================================================================
-- 4. 三档入口生物模板（外观复用虚空幽龙三色虚空门）
--    三个模板全部使用显式字面量，不再从 17367/17368/17369 复制字段，
--    避免把虚空幽龙（Netherspite）专用的非默认值带进来，例如：
--      unit_flags=33554496（含 UNIT_FLAG_NOT_SELECTABLE 0x02000000）、
--      flags_extra=128（CREATURE_FLAG_EXTRA_TRIGGER，触发器）、
--      HealthModifier=0.007、CreatureImmunitiesId=128 等。
--    关键取值：faction=35（友好，玩家无法攻击）、npcflag=1（可对话）、
--             unit_flags/unit_flags2/dynamicflags=0（可选可点）、
--             type_flags/flags_extra=0（非触发器、不隐藏、不免疫）、
--             family/VehicleId/PetSpellDataId=0（非宠物、非载具）、
--             type=10（NOT_SPECIFIED，非生物物件）、AIName=''（无 AI，原地不动）、
--             ScriptName=npc_rift_portal。
--    交互硬性要求（否则右键完全没反应、客户端连对话光标都不给）：
--      * unit_flags 绝不能带 UNIT_FLAG_NOT_SELECTABLE(0x02000000)；
--      * flags_extra 绝不能带 CREATURE_FLAG_EXTRA_TRIGGER(0x80)，否则核心会在
--        Creature::Create 里强制把生物追加为不可选，且对玩家不可见。
--    因此下面把所有可能影响“可选/可点/可交互”的标志列显式写成 0，不依赖表默认值。
--    模型与缩放：@RIFT_ENTRANCE_MODEL_T1/T2/T3 分别是三档的 displayID（当前同为 25683
--    元素裂隙 / 28452 Elemental Rift）；@RIFT_ENTRANCE_SCALE 是模型放大倍数，默认 1。
--    远程辨识度主要靠“放大倍数 + visibilityDistanceType=3”。
--    注意最终尺寸 = CreatureDisplayInfo.CreatureModelScale × 本变量：
--      25683 自带 CreatureModelScale=3（16946 是 2），所以这里给 1 已经比原来(2×1.3)略大，
--      再往上加会成倍放大，配合 visibilityDistanceType=3 通常够用了。
--    三档颜色：与模型无关，由 C++ 生成时附加的门体光环决定
--    （T1=30490 绿，T2=30491 蓝，T3=30487 红），所以三档用同一个 displayID 照样分色。
--    已核对客户端 CreatureDisplayInfo.dbc：25683 属于 ModelID 2411，其 7 个 display
--    (18402/18996/20011/20841/25683/27964/31069) 的 TextureVariation_1~3 全为空、
--    ParticleColorID 全为 0，只是 CreatureModelScale 不同，即“同一皮肤、没有颜色变体”；
--    传送门主模型族 ModelID 1731（11686/16946/19595/25206/17612... 数十个 display）
--    与 ModelID 1271（9510/23422）也同样没有皮肤变体。原版卡拉赞三色虚空门本就是靠
--    光环/法术视觉分色、不是靠模型皮肤，所以这里也只能靠光环分色。
--    若要三档在远处更好区分，可给三档各设不同 @RIFT_ENTRANCE_SCALE（尺寸差），
--    或直接换成形状本身不同的模型（如 T1=25206 虚空漩涡 / T2=18877 时光裂隙 / T3=23422 邪能门）。
--    备选外观：下面这些都是库里现成的传送门类生物模型，可直接填进上述变量；
--    进服可用 `.morph target <displayID>` 在已刷出的入口上即时预览（门体光环会一起叠加，
--    正好能看出“该模型 + 该档颜色”的实际观感），`.morph reset` 还原，不用改库也不用重启。
--      25683  元素裂隙（28452 Elemental Rift，当前默认）
--      16946  虚空门拱门（19224 Void Portal / 20663，体量中等）
--      11686  黑暗之门（18625 Dark Portal Dummy，体量最大、最显眼）
--       9510  恶魔/虚空传送门漩涡（9707 焦痕传送门、14081 恶魔传送门、15141 疯狂之门、
--              16420 暗影之门、17265 恶魔火传送门、24961 圣所裂隙）
--      18877  时光裂隙（17838 Time Rift，青铜色漩涡）
--      25206  混沌裂隙（26918 Chaotic Rift / 30522，虚空漩涡）
--      23422  邪能火传送门（25603 Felfire Portal，绿色邪能）
--      23719  沙塔斯传送门（26255 Shattrath Portal）
--      18783  酋长之门（17611 Warchief's Portal）
--      19595  太阳井/鸦神传送门（25156 / 23046）
-- ============================================================================
SET @RIFT_ENTRANCE_ENTRY_BASE := 100510;
SET @RIFT_ENTRANCE_MODEL_T1 := 25683;
SET @RIFT_ENTRANCE_MODEL_T2 := 25683;
SET @RIFT_ENTRANCE_MODEL_T3 := 25683;
SET @RIFT_ENTRANCE_SCALE := 0.35;

DELETE FROM `creature_template`
WHERE `entry` IN (@RIFT_ENTRANCE_ENTRY_BASE + 0,@RIFT_ENTRANCE_ENTRY_BASE + 1,@RIFT_ENTRANCE_ENTRY_BASE + 2);
INSERT INTO `creature_template` (
  `entry`,`name`,`subname`,`exp`,`faction`,`npcflag`,`unit_class`,
  `unit_flags`,`unit_flags2`,`dynamicflags`,`type_flags`,`flags_extra`,
  `family`,`VehicleId`,`PetSpellDataId`,
  `type`,`BaseAttackTime`,`RangeAttackTime`,`AIName`,`MovementType`,
  `HealthModifier`,`CreatureImmunitiesId`,`ScriptName`,`VerifiedBuild`)
VALUES
  (@RIFT_ENTRANCE_ENTRY_BASE + 0,'T1裂隙','虚空之门',1,35,1,1,0,0,0,0,0,0,0,0,10,2000,2000,'',0,1,0,'npc_rift_portal',12340),
  (@RIFT_ENTRANCE_ENTRY_BASE + 1,'T2裂隙','虚空之门',1,35,1,1,0,0,0,0,0,0,0,0,10,2000,2000,'',0,1,0,'npc_rift_portal',12340),
  (@RIFT_ENTRANCE_ENTRY_BASE + 2,'T3裂隙','虚空之门',1,35,1,1,0,0,0,0,0,0,0,0,10,2000,2000,'',0,1,0,'npc_rift_portal',12340);

DELETE FROM `creature_template_model`
WHERE `CreatureID` IN (@RIFT_ENTRANCE_ENTRY_BASE + 0,@RIFT_ENTRANCE_ENTRY_BASE + 1,@RIFT_ENTRANCE_ENTRY_BASE + 2);
INSERT INTO `creature_template_model` (`CreatureID`,`Idx`,`CreatureDisplayID`,`DisplayScale`,`Probability`,`VerifiedBuild`)
VALUES
  (@RIFT_ENTRANCE_ENTRY_BASE + 0,0,@RIFT_ENTRANCE_MODEL_T1,@RIFT_ENTRANCE_SCALE,1,12340),
  (@RIFT_ENTRANCE_ENTRY_BASE + 1,0,@RIFT_ENTRANCE_MODEL_T2,@RIFT_ENTRANCE_SCALE,1,12340),
  (@RIFT_ENTRANCE_ENTRY_BASE + 2,0,@RIFT_ENTRANCE_MODEL_T3,@RIFT_ENTRANCE_SCALE,1,12340);

-- visibilityDistanceType=3（Large）：入口在较远处也能被看到，便于玩家寻找。
DELETE FROM `creature_template_addon`
WHERE `entry` IN (@RIFT_ENTRANCE_ENTRY_BASE + 0,@RIFT_ENTRANCE_ENTRY_BASE + 1,@RIFT_ENTRANCE_ENTRY_BASE + 2);
INSERT INTO `creature_template_addon` (`entry`,`path_id`,`mount`,`bytes1`,`bytes2`,`emote`,`visibilityDistanceType`,`auras`)
VALUES
  (@RIFT_ENTRANCE_ENTRY_BASE + 0,0,0,0,0,0,3,NULL),
  (@RIFT_ENTRANCE_ENTRY_BASE + 1,0,0,0,0,0,3,NULL),
  (@RIFT_ENTRANCE_ENTRY_BASE + 2,0,0,0,0,0,3,NULL);

-- ============================================================================
-- 5. 示例区域：暴风峭壁（The Storm Peaks）
--    数量区间：T1 10~20，T2 5~10，T3 2~5（最大合计 35，因此该区域至少需要 35 个可用点位）。
--    坐标未实地测量前保持 enabled=0，避免入口生成在无效位置。
-- ============================================================================
DELETE FROM `heroic_dungeon_rift_spawn_point` WHERE `region_id`=1;
DELETE FROM `heroic_dungeon_rift_spawn_region` WHERE `region_id`=1;
INSERT INTO `heroic_dungeon_rift_spawn_region`
  (`region_id`,`region_name`,`map_id`,`area_id`,`center_x`,`center_y`,`center_z`,
   `t1_min_count`,`t1_max_count`,`t2_min_count`,`t2_max_count`,`t3_min_count`,`t3_max_count`,`enabled`,`remark`)
VALUES
  (1,'暴风峭壁',571,67,0,0,0,10,20,5,10,2,5,0,'暴风峭壁示例：填入实测坐标并设为 enabled=1 后生效');

-- ----------------------------------------------------------------------------
-- 刷新点录入模板（取消注释并替换为实测坐标；每个点一行，point_id 全局唯一）。
-- 所有点位都属于 region_id=1，因此 x/y/z 必须落在 map_id=571 的有效范围内。
-- 建议每个点都先以 enabled=0 录入，进服实测地面高度、碰撞与可达性后再逐个启用。
-- ----------------------------------------------------------------------------
-- DELETE FROM `heroic_dungeon_rift_spawn_point` WHERE `region_id`=1;
-- INSERT INTO `heroic_dungeon_rift_spawn_point`
--   (`point_id`,`region_id`,`x`,`y`,`z`,`o`,`enabled`,`remark`)
-- VALUES
--   (1,1,0,0,0,0,0,'暴风峭壁 点位1 待实测'),
--   (2,1,0,0,0,0,0,'暴风峭壁 点位2 待实测');
--   ... 依次补足至少 35 个可用点位后，把主表 enabled 置 1。

-- ============================================================================
-- 6. 示例每周开启时段（默认 enabled=0，确认后再启用）
--    启用任意一行后，该区域只在窗口内刷新；窗口开启瞬间按最大数量刷满并全服通知。
--    示例1：区域1 每周三 20:00 - 22:00
--    示例2：区域1 每周六 14:00 - 次日 02:00（跨零点）
--    示例3：区域1 每天 20:00 - 22:00（week_day = -1）
-- ============================================================================
DELETE FROM `heroic_dungeon_rift_schedule` WHERE `schedule_id` IN (1,2,3);
INSERT INTO `heroic_dungeon_rift_schedule`
  (`schedule_id`,`region_id`,`week_day`,`start_hour`,`start_minute`,`end_hour`,`end_minute`,`enabled`,`remark`)
VALUES
  (1,1,3,20,0,22,0,0,'示例：区域1 每周三 20:00-22:00 开启'),
  (2,1,6,14,0,2,0,0,'示例：区域1 每周六 14:00 至次日 02:00 开启（跨零点）'),
  (3,1,-1,20,0,22,0,0,'示例：区域1 每天 20:00-22:00 开启（-1=每天）');

-- ============================================================================
-- 7. 审核查询
-- ============================================================================
-- 可选/可交互审核：unit_flags、unit_flags2、dynamicflags、type_flags、flags_extra、
-- family、VehicleId、PetSpellDataId 必须全部为 0，faction=35，npcflag=1，ScriptName=npc_rift_portal。
SELECT `entry`,`name`,`subname`,`faction`,`npcflag`,`unit_class`,
       `unit_flags`,`unit_flags2`,`dynamicflags`,`type_flags`,`flags_extra`,
       `family`,`VehicleId`,`PetSpellDataId`,`type`,`ScriptName`
FROM `creature_template`
WHERE `entry` IN (@RIFT_ENTRANCE_ENTRY_BASE + 0,@RIFT_ENTRANCE_ENTRY_BASE + 1,@RIFT_ENTRANCE_ENTRY_BASE + 2)
ORDER BY `entry`;
SELECT `CreatureID`,`Idx`,`CreatureDisplayID` FROM `creature_template_model`
WHERE `CreatureID` IN (@RIFT_ENTRANCE_ENTRY_BASE + 0,@RIFT_ENTRANCE_ENTRY_BASE + 1,@RIFT_ENTRANCE_ENTRY_BASE + 2)
ORDER BY `CreatureID`,`Idx`;
SELECT * FROM `heroic_dungeon_rift_spawn_region` ORDER BY `region_id`;
SELECT * FROM `heroic_dungeon_rift_spawn_point` ORDER BY `region_id`,`point_id`;
SELECT * FROM `heroic_dungeon_rift_schedule` ORDER BY `schedule_id`;
