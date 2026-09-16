-- =====================================================================
-- 【更新 / 限制】鲜血烘炉(542) + 萨隆矿坑(658) + 映像大厅(668) 仅限制英雄(H)模式
--                普通模式保持不变，可正常进入
-- 数据已比对本机 acore_world 实际数据（2026-09-16），非 base 默认值。
-- 配套恢复脚本：outputs/进度控制/04_恢复鲜血烘炉映像大厅萨隆矿坑_英雄模式.sql
-- =====================================================================

-- ---------- 一、禁用三个副本的英雄(H)模式 ----------
-- 三个本均为 5 人地下城(dungeon)，英雄难度对应 DungeonStatusFlag：
--   DUNGEON_STATUSFLAG_NORMAL = 0x01（普通）
--   DUNGEON_STATUSFLAG_HEROIC  = 0x02（英雄）
-- 故 flags = 2 只命中英雄难度，普通难度不受影响。
-- 命中 DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP)：
--   * 直接进本：玩家选英雄难度时提示"副本已关闭"
--   * 随机地下城(LFG)：LFGMgr::IsDungeonDisabled -> 英雄难度从可排列表中移除
DELETE FROM `disables` WHERE `sourceType` = 2 AND `entry` IN (542, 658, 668);
INSERT INTO `disables` (`sourceType`, `entry`, `flags`, `params_0`, `params_1`, `comment`) VALUES
(2, 542, 2, '', '', 'The Blood Furnace 英雄模式暂时关闭'),
(2, 658, 2, '', '', 'Pit of Saron 英雄模式暂时关闭'),
(2, 668, 2, '', '', 'Halls of Reflection 英雄模式暂时关闭');

-- ---------- 二、备用闸门：英雄模式准入等级抬到 81（满级 80）----------
-- 只更新英雄模式(difficulty=1)的行，普通模式(difficulty=0)不受影响
-- 本服 Instance.IgnoreLevel = 0，由 Player::Satisfy 生效
UPDATE `dungeon_access_template` SET `min_level` = 81 WHERE `map_id` IN (542, 658, 668) AND `difficulty` = 1 AND `min_level` BETWEEN 1 AND 80;
