-- =====================================================================
-- 【更新 / 限制】鲜血烘炉(542) + 萨隆矿坑(658) + 映像大厅(668) 仅限制英雄(H)模式
--                普通模式保持不变，可正常进入
--                + 卡拉赞(532) 10 人副本整体关闭（该副本没有普通/英雄之分，只有一种难度）
-- 数据已比对本机 acore_world 实际数据（2026-09-16），非 base 默认值。
-- 配套恢复脚本：outputs/进度控制/04_恢复鲜血烘炉映像大厅萨隆矿坑_英雄模式.sql
--               其中第二部分为卡拉赞(532)的恢复语句，与本文件第三、四部分对应
-- =====================================================================

-- ---------- 一、禁用三个副本的英雄(H)模式 ----------
-- 三个本均为 5 人地下城(dungeon)，英雄难度对应 DungeonStatusFlag：
--   DUNGEON_STATUSFLAG_NORMAL = 0x01（普通）
--   DUNGEON_STATUSFLAG_HEROIC  = 0x02（英雄）
-- 故 flags = 2 只命中英雄难度，普通难度不受影响。
-- 命中 DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP)：
--   * 直接进本：玩家选英雄难度时提示"副本已关闭"
--   * 随机地下城(LFG)：LFGMgr::IsDungeonDisabled -> 英雄难度从可排列表中移除
DELETE FROM `disables` WHERE `sourceType` = 2 AND `entry` IN (632, 658, 668);
INSERT INTO `disables` (`sourceType`, `entry`, `flags`, `params_0`, `params_1`, `comment`) VALUES
(2, 632, 2, '', '', 'The Blood Furnace 英雄模式暂时关闭'),
(2, 658, 2, '', '', 'Pit of Saron 英雄模式暂时关闭'),
(2, 668, 2, '', '', 'Halls of Reflection 英雄模式暂时关闭');

-- ---------- 二、备用闸门：英雄模式准入等级抬到 81（满级 80）----------
-- 只更新英雄模式(difficulty=1)的行，普通模式(difficulty=0)不受影响
-- 本服 Instance.IgnoreLevel = 0，由 Player::Satisfy 生效
UPDATE `dungeon_access_template` SET `min_level` = 81 WHERE `map_id` IN (542, 658, 668) AND `difficulty` = 1 AND `min_level` BETWEEN 1 AND 80;

-- ---------- 三、关闭卡拉赞(532) 10 人团队副本 ----------
-- 卡拉赞是 TBC 时代的 10 人 raid，3.3.5 客户端的 MapDifficulty 里只有 difficulty = 0
-- 一行（MaxPlayers = 10），不存在 25 人/英雄难度记录，所以它没有"只关某个难度"的说法，
-- 要么整体开、要么整体关。
-- 关于 flags 取值的依据：
--   * 玩家即使把团队难度选成 25 人(RAID_DIFFICULTY_25MAN_NORMAL = 1)，进本时
--     GetDownscaledMapDifficultyData() 会因 532 没有难度 1 的记录把它降回 0
--     （源码注释：any non-normal mode for raids like tbc (only one mode)）
--   * 于是 DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP) 里 targetDifficulty = 0
--     -> 命中 DUNGEON_DIFFICULTY_NORMAL，即校验 DUNGEON_STATUSFLAG_NORMAL(0x01)
--   因此 flags = 1 就能覆盖卡拉赞的全部难度。
--   注意不能照抄 01 里的 flags = 15：DisableMgr::LoadDisables 会逐位校验"英雄"位在
--   MapDifficulty 里是否真有对应记录，532 没有难度 2/3 的记录，flags 含 0x04/0x08
--   时整条会被判为 "Disable flags for map 532 are invalid" 直接丢弃（等于没限制）；
--   0x02(25N) 同理，532 也没有难度 1 的记录。
-- 生效点（DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP)）：
--   * 直接进本：PlayerStorage.cpp Satisfy() 提示副本已关闭，返回 false
--   * 传送进本：Player::TeleportTo() 直接拒绝并回 TRANSFER_ABORT_MAP_NOT_ALLOWED
-- 按主键(sourceType,entry)先删后插，保证脚本可重复执行
DELETE FROM `disables` WHERE `sourceType` = 2 AND `entry` = 532;
INSERT INTO `disables` (`sourceType`, `entry`, `flags`, `params_0`, `params_1`, `comment`) VALUES
(2, 532, 1, '', '', 'Karazhan 暂时关闭');

-- ---------- 四、备用闸门：卡拉赞准入等级抬到 81（满级 80）----------
-- 卡拉赞只有 difficulty = 0 这一行，原值 min_level = 68（base 与 updates 均已核对，未被改过）
-- 本服 Instance.IgnoreLevel = 0，由 Player::Satisfy 生效
UPDATE `dungeon_access_template` SET `min_level` = 81 WHERE `map_id` = 532 AND `difficulty` = 0 AND `min_level` BETWEEN 1 AND 80;
