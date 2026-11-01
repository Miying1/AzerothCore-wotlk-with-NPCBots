-- 新增三个不可重复的纹章兑换天赋点任务（发布/交付 NPC：28160 大法师潘塔鲁斯，达拉然）
--
-- 62022「凯旋天赋点+1」：上交 凯旋纹章(47241) x30 -> 天赋点+1(70103)
-- 62023「凯旋天赋点+2」：上交 凯旋纹章(47241) x60 -> 天赋点+2(70104)
-- 62024「寒冰天赋点」  ：上交 冰霜纹章(49426) x60 -> 天赋点+2(70104)
--
-- 说明：
--   * QuestType = 2（常规任务；Method = 0 会被核心当作 auto-complete，
--     不校验也不扣除上交物品），因此必须为 2。
--   * “不可重复”由 quest_template_addon.SpecialFlags = 0 保证（不含
--     QUEST_SPECIAL_FLAGS_REPEATABLE = 1，任务只能完成一次）。
--   * 奖励物品 70103「天赋点+1」/ 70104「天赋点+2」已由
--     rev_20261101_00.sql 与 modules/mod-playervip 建立（PlayerVipTalentPointItem）。
--   * quest_template 按项目规范不使用 DELETE，改用 REPLACE INTO 保证幂等。

-- 1) 任务模板
REPLACE INTO `quest_template`
    (`ID`, `QuestType`, `QuestLevel`, `MinLevel`, `QuestSortID`, `QuestInfoID`, `Flags`,
     `RewardXPDifficulty`, `RewardMoney`, `RequiredItemId1`, `RequiredItemCount1`,
     `RewardItem1`, `RewardAmount1`, `ObjectiveText1`,
     `LogTitle`, `LogDescription`, `QuestDescription`, `QuestCompletionLog`, `VerifiedBuild`)
VALUES
(62022, 2, 80, 80, 0, 0, 8,
 0, 0, 47241, 30,
 70103, 1, '凯旋纹章',
 '凯旋天赋点+1',
 '把 30 枚凯旋纹章交给达拉然的大法师潘塔鲁斯，换取 1 点天赋点。',
 '凯旋纹章在你手里越积越多，却换不来真正属于你的力量。$B$B交给我 30 枚，我便为你凝聚一枚天赋结晶，永久增加 1 点天赋点。',
 '返回大法师潘塔鲁斯处。',
 12340),
(62023, 2, 80, 80, 0, 0, 8,
 0, 0, 47241, 60,
 70104, 1, '凯旋纹章',
 '凯旋天赋点+2',
 '把 60 枚凯旋纹章交给达拉然的大法师潘塔鲁斯，换取 2 点天赋点。',
 '60 枚凯旋纹章，换 2 点天赋，比零散兑换更划算。$B$B水晶的成形需要足够的纹章，你只管准备好材料。',
 '返回大法师潘塔鲁斯处。',
 12340),
(62024, 2, 80, 80, 0, 0, 8,
 0, 0, 49426, 60,
 70104, 1, '冰霜纹章',
 '寒冰天赋点',
 '把 60 枚冰霜纹章交给达拉然的大法师潘塔鲁斯，换取 2 点天赋点。',
 '冰霜纹章中残留着巫妖王的寒意，正好用来凝练天赋结晶。$B$B给我 60 枚，我就把这份力量交到你手上。',
 '返回大法师潘塔鲁斯处。',
 12340);

-- 2) 任务附加数据：SpecialFlags = 0 -> 不可重复
DELETE FROM `quest_template_addon` WHERE `ID` IN (62022, 62023, 62024);
INSERT INTO `quest_template_addon`
    (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`,
     `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`,
     `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`,
     `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`)
VALUES
(62022, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(62023, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(62024, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

-- 3) 任务发布者 / 交付者：28160 大法师潘塔鲁斯
DELETE FROM `creature_queststarter` WHERE `id` = 28160 AND `quest` IN (62022, 62023, 62024);
INSERT INTO `creature_queststarter` (`id`, `quest`) VALUES
(28160, 62022),
(28160, 62023),
(28160, 62024);

DELETE FROM `creature_questender` WHERE `id` = 28160 AND `quest` IN (62022, 62023, 62024);
INSERT INTO `creature_questender` (`id`, `quest`) VALUES
(28160, 62022),
(28160, 62023),
(28160, 62024);
