-- =====================================================================
-- 【更新 / 关闭】冰冠堡垒 ICC(631) + 红玉圣殿 RS(724) 暂时关闭
--                随机日常纹章下调一档（数量不变）
-- 数据已比对本机 acore_world 实际数据（2026-09-13），非 base 默认值。
-- 配套恢复脚本：outputs/进度控制/02_恢复ICC与红玉圣殿_随机日常纹章还原.sql
-- =====================================================================

-- ---------- 一、关闭冰冠堡垒(631) 与 红玉圣殿(724) ----------
-- flags = 15 = 10N(0x01) | 25N(0x02) | 10H(0x04) | 25H(0x08)，覆盖全部难度
-- 命中 DisableMgr::IsDisabledFor(DISABLE_TYPE_MAP)，进本时提示"副本已关闭"
DELETE FROM `disables` WHERE `sourceType` = 2 AND `entry` IN (631, 724);
INSERT INTO `disables` (`sourceType`, `entry`, `flags`, `params_0`, `params_1`, `comment`) VALUES
(2, 631, 15, '', '', 'Icecrown Citadel 暂时关闭'),
(2, 724, 15, '', '', 'The Ruby Sanctum 暂时关闭');

-- 备用闸门：准入等级抬到 81（满级 80）。本服 Instance.IgnoreLevel = 0，由 Player::Satisfy 生效
UPDATE `dungeon_access_template` SET `min_level` = 81 WHERE `map_id` IN (631, 724) AND `min_level` BETWEEN 1 AND 80;

-- ---------- 二、随机日常纹章降级（只换物品，数量不变）----------
-- 随机日常奖励由 lfg_dungeon_rewards 驱动(LFGMgr::FinishDungeon -> Player::RewardQuest)：
--   dungeonId 262 随机英雄(80) -> firstQuest 24788 / otherQuest 24789
--   dungeonId 261 随机普通(80) -> firstQuest 24790 / otherQuest 24791
-- 本服现状（已核对）：
--   24788  49426 冰霜纹章 x12 (槽1) -> 47241 凯旋纹章 x12
--   24789  47241 凯旋纹章 x8  (槽2) -> 45624 征服纹章 x8
--   24790  47241 凯旋纹章 x15 (槽1) -> 45624 征服纹章 x15
--   24791  47241 凯旋纹章 x5  (槽1) -> 45624 征服纹章 x5
-- 精准 ID 匹配，逐条显式设置物品与数量（数量保持不变）
UPDATE `quest_template` SET `RewardItem1` = 47241, `RewardAmount1` = 12 WHERE `ID` = 24788;
UPDATE `quest_template` SET `RewardItem2` = 45624, `RewardAmount2` = 8  WHERE `ID` = 24789;
UPDATE `quest_template` SET `RewardItem1` = 45624, `RewardAmount1` = 15 WHERE `ID` = 24790;
UPDATE `quest_template` SET `RewardItem1` = 45624, `RewardAmount1` = 5  WHERE `ID` = 24791;

-- ---------- 三、寒冰纹章(49426)不再掉落 ----------
-- 命中 LootMgr::AllowedForPlayer，该物品会被从所有掉落中过滤掉：
-- creature_loot(47) / item_loot(13) / gameobject_loot(12) 等来源一并失效
DELETE FROM `disables` WHERE `sourceType` = 10 AND `entry` = 49426;
INSERT INTO `disables` (`sourceType`, `entry`, `flags`, `params_0`, `params_1`, `comment`) VALUES
(10, 49426, 0, '', '', 'Emblem of Frost 暂时不掉落');

-- ---------- 四、其余任务奖励纹章降级（20 个任务）----------
-- 冰霜纹章(49426) -> 凯旋纹章(47241)，凯旋纹章(47241) -> 征服纹章(45624)，数量不变
-- 只降奖励、不禁任务，避免破坏地下城准入链（24499/24511/24710/24712 是萨隆矿坑/映像大厅的前置）
-- 精准 ID 匹配，逐组显式设置物品与数量（数量保持不变）
UPDATE `quest_template` SET `RewardItem1` = 47241, `RewardAmount1` = 2  WHERE `ID` IN (24499, 24500, 24511, 24710, 24712, 24802);
UPDATE `quest_template` SET `RewardItem1` = 47241, `RewardAmount1` = 5  WHERE `ID` IN (24579, 24580, 24581, 24582, 24583, 24584, 24585, 24586, 24587, 24588, 24589, 24590, 26034);
UPDATE `quest_template` SET `RewardItem1` = 47241, `RewardAmount1` = 10 WHERE `ID` = 65000;
UPDATE `quest_template` SET `RewardItem2` = 45624, `RewardAmount2` = 5  WHERE `ID` IN (24579, 24580, 24581, 24582, 24583, 24584, 24585, 24586, 24587, 24588, 24589, 24590);

-- ---------- 五、关闭寒冰纹章军需官的商品 ----------
-- 37941 博学者亚兰 / 37942 秘法师乌弗瑞 / 38858 『关闭者』古德曼（共 390 条商品）
-- conditions 源类型 23 = NPC_VENDOR；条件 27 = CONDITION_LEVEL，"等级 == 0" 恒不成立 => 商品隐藏
-- 该条件在 SendListInventory 与 BuyItemFromVendor 两处都会生效
DELETE FROM `conditions` WHERE `SourceTypeOrReferenceId` = 23 AND `SourceGroup` IN (37941, 37942, 38858);
INSERT INTO `conditions`
(`SourceTypeOrReferenceId`, `SourceGroup`, `SourceEntry`, `SourceId`, `ElseGroup`, `ConditionTypeOrReference`, `ConditionTarget`, `ConditionValue1`, `ConditionValue2`, `ConditionValue3`, `NegativeCondition`, `ErrorType`, `ErrorTextId`, `ScriptName`, `Comment`)
SELECT DISTINCT 23, `entry`, `item`, 0, 0, 27, 0, 0, 0, 0, 0, 0, 0, '', 'Emblem of Frost 军需官暂时关闭'
FROM `npc_vendor`
WHERE `entry` IN (37941, 37942, 38858);
