-- 新增两个不可重复的挑战天赋兑换任务（发布/交付 NPC：61001 野牛 · 地下城挑战大师，达拉然）
--
-- 62020「挑战天赋+1」：上交 1 枚挑战奖章(62011) -> 天赋点+1(70103)
-- 62021「挑战天赋+2」：上交 5 枚挑战奖章(62011) -> 天赋点+2(70104)
--
-- 说明：
--   * QuestType = 2（普通任务；核心把非 0 的 Method 当作需要检查目标的常规任务，
--     设为 0 会被当成 auto-complete 而不校验上交物品），
--     Flags = 8（QUEST_FLAGS_SHARABLE，可共享给队友）；
--     “不可重复”由 quest_template_addon.SpecialFlags = 0 保证（不含
--     QUEST_SPECIAL_FLAGS_REPEATABLE = 1，任务只能完成一次）。
--   * 均为“上交物品”型任务：RequiredItemId1 / RequiredItemCount1 即交付条件，
--     玩家持有足够挑战奖章时任务即判定为可交付，交付时扣除对应数量。
--   * 奖励物品 70103/70104 是「天赋点+1 / 天赋点+2」，由 modules/mod-playervip 的
--     PlayerVipTalentPointItem 脚本处理（永久增加额外天赋点，上限 15 点）。
--     本文件末尾补齐了这两件物品的定义，与 modules/mod-playervip/data/player_vip_items.sql
--     完全一致，重复执行无副作用。
--   * quest_template / item_template 按项目规范不使用 DELETE，改用 REPLACE INTO 保证幂等。
--   * NPC 61001 的自定义 gossip 菜单里，任务列表通过「我想升级挑战者衬衫」这一项打开
--     （mod-zone-difficulty 的 mod_zone_npc_tzds::OnGossipHello），新任务会一并出现在其中。

-- 1) 任务模板
REPLACE INTO `quest_template`
    (`ID`, `QuestType`, `QuestLevel`, `MinLevel`, `QuestSortID`, `QuestInfoID`, `Flags`,
     `RewardXPDifficulty`, `RewardMoney`, `RequiredItemId1`, `RequiredItemCount1`,
     `RewardItem1`, `RewardAmount1`, `ObjectiveText1`,
     `LogTitle`, `LogDescription`, `QuestDescription`, `QuestCompletionLog`, `VerifiedBuild`)
VALUES
(62020, 2, 80, 80, 0, 0, 8,
 0, 0, 62011, 1,
 70103, 1, '挑战奖章',
 '挑战天赋+1',
 '把 1 枚挑战奖章交给达拉然的挑战大师野牛，换取 1 点天赋点。',
 '冒险者，你在挑战模式里赢来的挑战奖章，可以换来更实在的东西。$B$B这枚结晶能永久增加你的天赋点数，不过得用 1 枚挑战奖章来换。$B$B想清楚自己要走的路，再决定吧。',
 '返回挑战大师野牛处。',
 12340),
(62021, 2, 80, 80, 0, 0, 8,
 0, 0, 62011, 5,
 70104, 1, '挑战奖章',
 '挑战天赋+2',
 '把 5 枚挑战奖章交给达拉然的挑战大师野牛，换取 2 点天赋点。',
 '冒险者。$B$B5 枚挑战奖章，我就给你一枚更大的结晶，直接加 2 点天赋。$B$B当然，账你自己算得清。',
 '返回挑战大师野牛处。',
 12340);

-- 2) 任务附加数据：SpecialFlags = 0 -> 不可重复
DELETE FROM `quest_template_addon` WHERE `ID` IN (62020, 62021);
INSERT INTO `quest_template_addon`
    (`ID`, `MaxLevel`, `AllowableClasses`, `SourceSpellID`, `PrevQuestID`, `NextQuestID`,
     `ExclusiveGroup`, `BreadcrumbForQuestId`, `RewardMailTemplateID`, `RewardMailDelay`,
     `RequiredSkillID`, `RequiredSkillPoints`, `RequiredMinRepFaction`, `RequiredMaxRepFaction`,
     `RequiredMinRepValue`, `RequiredMaxRepValue`, `ProvidedItemCount`, `SpecialFlags`)
VALUES
(62020, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
(62021, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

-- 3) 任务发布者 / 交付者：61001 野牛
DELETE FROM `creature_queststarter` WHERE `id` = 61001 AND `quest` IN (62020, 62021);
INSERT INTO `creature_queststarter` (`id`, `quest`) VALUES
(61001, 62020),
(61001, 62021);

DELETE FROM `creature_questender` WHERE `id` = 61001 AND `quest` IN (62020, 62021);
INSERT INTO `creature_questender` (`id`, `quest`) VALUES
(61001, 62020),
(61001, 62021);

 