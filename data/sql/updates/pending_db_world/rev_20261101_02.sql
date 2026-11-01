-- 清理脚本：供已执行过旧版 rev_20261101_01.sql（含任务 62022）的数据库使用
--
-- 旧版内容：62022「凯旋天赋点+1」30 凯旋纹章 -> 70103
--           62023「凯旋天赋点+2」60 凯旋纹章 -> 70104
-- 现要求：删除 62022，62023 奖励改为 70103（+1 天赋点）
--
-- 注：这里必须对 quest_template 使用 DELETE（否则 62022 会残留在任务列表里），
--     是本项目的有意例外；正常情况下 quest_template 只做 REPLACE/UPDATE。
--
-- 1) 删除任务 62022 及其关联数据
DELETE FROM `quest_template` WHERE `ID` = 62022;

DELETE FROM `quest_template_addon` WHERE `ID` = 62022;

DELETE FROM `creature_queststarter` WHERE `id` = 28160 AND `quest` = 62022;

DELETE FROM `creature_questender` WHERE `id` = 28160 AND `quest` = 62022;

-- 2) 62023 奖励由 70104(+2 天赋点) 改为 70103(+1 天赋点)，同步标题与文本
UPDATE `quest_template` SET
    `RewardItem1` = 70103,
    `RewardAmount1` = 1,
    `LogTitle` = '凯旋天赋点+1',
    `LogDescription` = '把 60 枚凯旋纹章交给达拉然的大法师潘塔鲁斯，换取 1 点天赋点。',
    `QuestDescription` = '60 枚凯旋纹章，换 1 点天赋。$B$B水晶的成形需要足够的纹章，你只管准备好材料。'
WHERE `ID` = 62023;
