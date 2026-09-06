-- 自定义小宠物生物：白虎
-- 生物 entry：101000
-- 显示模型：DisplayID 100051（Creature\siberiantigergod\siberiantigergod.mdx，西伯利亚虎神）
-- 用途：制作小宠物（非战斗宠物）。后续新建召唤法术时，法术效果选 SUMMON_PET（效果ID 56），SummonEntry 填 101000 即可。
-- 执行数据库：acore_world

-- 幂等清理：先删后插（重复执行无副作用）
DELETE FROM `creature_template` WHERE `entry` = 101000;
DELETE FROM `creature_template_model` WHERE `CreatureID` = 101000;
DELETE FROM `creature_model_info` WHERE `DisplayID` = 100051;

-- 生物模板：type=10 非战斗宠物，faction=35 对所有人友善，1 级，无掉落、无偷窃、无剥皮
INSERT INTO `creature_template`
(`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `name`,
 `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `speed_walk`,
 `speed_run`, `speed_swim`, `speed_flight`, `detection_range`, `rank`, `dmgschool`, `DamageModifier`,
 `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`,
 `dynamicflags`, `family`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `PetSpellDataId`, `VehicleId`,
 `mingold`, `maxgold`, `AIName`, `MovementType`, `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`,
 `ExperienceModifier`, `RacialLeader`, `movementId`, `RegenHealth`, `CreatureImmunitiesId`, `flags_extra`, `ScriptName`,
 `VerifiedBuild`)
VALUES
(101000, 0, 0, 0, 0, 0, '白虎',
 '小宠物', '', 0, 1, 1, 0, 35, 0, 1.11111,
 1.14286, 1, 1, 20, 0, 0, 1,
 2000, 2000, 1, 1, 1, 0, 2048,
 0, 0, 10, 0, 0, 0, 0, 0, 0,
 0, 0, '', 0, 1, 1, 1, 1,
 1, 0, 0, 1, 0, 2, '',
 0);

-- 模型绑定：DisplayScale=0.5 将虎神模型缩小为小宠物体型，可按观感在 0.3~1.0 间微调（1.0 为原始大小）
INSERT INTO `creature_template_model`
(`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
VALUES
(101000, 0, 100051, 0.4, 1, 0);

-- 碰撞体积：为模型 100051 补充 creature_model_info，避免启动报错
-- No model data exist for `CreatureDisplayID` = 100051 listed by creature (Entry: 101000)
-- BoundingRadius / CombatReach 为小宠物体型的经验值，可据实际观感微调
INSERT INTO `creature_model_info`
(`DisplayID`, `BoundingRadius`, `CombatReach`, `Gender`, `DisplayID_Other_Gender`, `VerifiedBuild`)
VALUES
(100051, 0.4, 0.8, 2, 0, 0);

-- ============================================================
-- 对话功能配置：绑定脚本并开启所需 NPC 标志（幂等）
-- GOSSIP(1) + VENDOR(128) + FLIGHTMASTER(8192) = 8321
-- VENDOR 必需：宝物商店的售卖窗口依赖此标志
-- FLIGHTMASTER 必需：航班（飞行点地图）的乘坐校验依赖此标志
-- ============================================================
UPDATE `creature_template` SET `npcflag` = 12417, `ScriptName` = 'npc_baihu_gossip' WHERE `entry` = 101000;

-- ============================================================
-- 问候语正文（窗口顶部显示，替换默认问候文本 "Greetings $N"）
-- 背景设定：白虎为潘达利亚四天神之一，身处 WLK 3.3.5 时代，被玩家召唤而来；
--           内容方向：以未来见证者身份隐晦透露后续版本的剧情（天神预言体，不明说人名事件）
-- 多条文本随机显示：脚本按 urand 随机发送 ID（101000 ~ 101023），
-- 增删文本时需同步修改脚本常量 NPC_WELCOME_TEXT_COUNT（npc_baihu_gossip.cpp）
-- 真实金币倍率由对话选项「我的金币倍率」以 NPC 悄悄话形式告知
-- 幂等清理：先删后插（重复执行无副作用）
-- ============================================================
DELETE FROM `npc_text` WHERE `ID` BETWEEN 101000 AND 101022;

INSERT INTO `npc_text` (`ID`, `text0_0`, `Probability0`) VALUES
(101000,'|cff00ccff凡人,我在未来见过你!|r',1),
(101001,'|cff00ccff时间线交织,你我相遇并非偶然。|r',1),
(101002,'|cff00ccff迷雾散开之日,便是我的家园现世之时。|r',1),
(101003,'|cff00ccff雾霭深处,一名少年兽王正在磨砺他的怒火,那火比战鼓更响,比刀锋更利。|r',1),
(101004,'|cff00ccff我家园的大地之下,沉睡着由贪、嗔、痴凝成的暗影,莫要用贪婪去唤醒它们。|r',1),
(101005,'|cff00ccff玉珑盘于云巅,赤精燃于烈焰,砮皂立于厚土,它们与我,各自静候有缘之人。|r',1),
(101006,'|cff00ccff门扉会再度开启,门后似是故土,却是一条走岔了的时间线。|r',1),
(101007,'|cff00ccff绿色的天穹之下,那位以背叛为名的囚徒,将以守护之姿归来。|r',1),
(101008,'|cff00ccff命运的织机上有一根丝线,将于异邦的海岸断裂,狮子的王冠,会传给哭泣的雄狮。|r',1),
(101009,'|cff00ccff翡翠色的长梦正在腐朽,沉睡万年的女王醒来时,泪水会淹没梦境。|r',1),
(101010,'|cff00ccff红发的游侠终将执掌权柄,她的一支箭,能点燃最圣洁的古树。|r',1),
(101011,'|cff00ccff海渊之底,被遗忘者的低语,足以淹没整片海洋的心跳。|r',1),
(101012,'|cff00ccff冰封的王座终有碎裂之日,彼时,生者与亡者的界限将不再分明。|r',1),
(101013,'|cff00ccff你们脚下的世界并非凡土,她沉睡的心跳,终有被世人听见的一日。|r',1),
(101014,'|cff00ccff驾驭烈焰与惊涛的兽人,将放下权杖,俯身聆听大地受伤的呻吟。|r',1),
(101015,'|cff00ccff少年兽王的怒火平息之日,他的战盔会悬于高台,供往来的旅人警醒。|r',1),
(101016,'|cff00ccff黑翼的阴影遮蔽日月之时,大地的裂痕会吞没你们熟悉的城池。|r',1),
(101017,'|cff00ccff天空将被邪能染成病态的绿,泰坦的坟墓会向有胆识者敞开。|r',1),
(101018,'|cff00ccff泉水之城将陷入长夜,月影下的居民会以新的面貌走出迷障。|r',1),
(101019,'|cff00ccff王冠与王座几度易主,唯有权力的游戏,从未因血与火而落幕。|r',1),
(101020,'|cff00ccff冥界的看守者自以为执掌众生,却不知自己也不过是棋盘上的一枚子。|r',1),
(101021,'|cff00ccff冰原之上,新的龙裔将重新振翅,古老的誓言会随钟声归来。|r',1),
(101022,'|cff00ccff未来还会有人开启新的裂隙,也会有人缝补旧日的伤痕。|r',1);

DELETE FROM `item_template` WHERE `ID` = 91500 ;
INSERT INTO `acore_world`.`item_template` (`entry`, `class`, `subclass`, `SoundOverrideSubclass`, `name`, `displayid`, `Quality`, `Flags`, `FlagsExtra`, `BuyCount`, `BuyPrice`, `SellPrice`, `InventoryType`, `AllowableClass`, `AllowableRace`, `ItemLevel`, `RequiredLevel`, `RequiredSkill`, `RequiredSkillRank`, `requiredspell`, `requiredhonorrank`, `RequiredCityRank`, `RequiredReputationFaction`, `RequiredReputationRank`, `maxcount`, `stackable`, `ContainerSlots`, `stat_type1`, `stat_value1`, `stat_type2`, `stat_value2`, `stat_type3`, `stat_value3`, `stat_type4`, `stat_value4`, `stat_type5`, `stat_value5`, `stat_type6`, `stat_value6`, `stat_type7`, `stat_value7`, `stat_type8`, `stat_value8`, `stat_type9`, `stat_value9`, `stat_type10`, `stat_value10`, `ScalingStatDistribution`, `ScalingStatValue`, `dmg_min1`, `dmg_max1`, `dmg_type1`, `dmg_min2`, `dmg_max2`, `dmg_type2`, `armor`, `holy_res`, `fire_res`, `nature_res`, `frost_res`, `shadow_res`, `arcane_res`, `delay`, `ammo_type`, `RangedModRange`, `spellid_1`, `spelltrigger_1`, `spellcharges_1`, `spellppmRate_1`, `spellcooldown_1`, `spellcategory_1`, `spellcategorycooldown_1`, `spellid_2`, `spelltrigger_2`, `spellcharges_2`, `spellppmRate_2`, `spellcooldown_2`, `spellcategory_2`, `spellcategorycooldown_2`, `spellid_3`, `spelltrigger_3`, `spellcharges_3`, `spellppmRate_3`, `spellcooldown_3`, `spellcategory_3`, `spellcategorycooldown_3`, `spellid_4`, `spelltrigger_4`, `spellcharges_4`, `spellppmRate_4`, `spellcooldown_4`, `spellcategory_4`, `spellcategorycooldown_4`, `spellid_5`, `spelltrigger_5`, `spellcharges_5`, `spellppmRate_5`, `spellcooldown_5`, `spellcategory_5`, `spellcategorycooldown_5`, `bonding`, `description`, `PageText`, `LanguageID`, `PageMaterial`, `startquest`, `lockid`, `Material`, `sheath`, `RandomProperty`, `RandomSuffix`, `block`, `itemset`, `MaxDurability`, `area`, `Map`, `BagFamily`, `TotemCategory`, `socketColor_1`, `socketContent_1`, `socketColor_2`, `socketContent_2`, `socketColor_3`, `socketContent_3`, `socketBonus`, `GemProperties`, `RequiredDisenchantSkill`, `ArmorDamageModifier`, `duration`, `ItemLimitCategory`, `HolidayId`, `ScriptName`, `DisenchantID`, `FoodType`, `minMoneyLoot`, `maxMoneyLoot`, `flagsCustom`, `VerifiedBuild`) VALUES (91500, 15, 2, -1, '白虎天神', 62969, 3, 134250560, 0, 1, 0, 0, 0, -1, -1, 20, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 55884, 0, 0, 0, -1, 0, -1, 69541, 6, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, -1, 1, '', 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 4096, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, '', 0, 0, 0, 0, 0, 10505);