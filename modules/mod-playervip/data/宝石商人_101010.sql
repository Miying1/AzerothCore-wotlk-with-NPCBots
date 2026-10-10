-- 自定义 NPC：宝石商人
-- 生物 entry：101010
-- 显示模型：DisplayID 19956（Ethereal 商人模型，与游戏内宝石/货币商人同款）
-- 功能：1. 宝石商店（打开 NPC 售卖窗口）；2. 剥离源力宝石（消耗 1000 金币，把装备上镶嵌的
--       源力宝石 entry 63102~63110 剥离下来返还背包，其他插槽宝石不动）
-- 脚本：npc_gem_merchant（modules/mod-playervip/src/npc_gem_merchant.cpp）
-- 执行数据库：acore_world

-- ============================================================
-- 生物模板（幂等：先删后插）
-- npcflag = 129（GOSSIP=1 + VENDOR=128）：GOSSIP 提供对话菜单，「宝石商店」的售卖窗口依赖 VENDOR 标志
-- unit_flags = 32768（UNIT_FLAG_NON_ATTACKABLE，不可攻击）；flags_extra = 2（平民，不会被误伤）
-- ============================================================
DELETE FROM `creature_template` WHERE `entry` = 101010;
DELETE FROM `creature_template_model` WHERE `CreatureID` = 101010;

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
(101010, 0, 0, 0, 0, 0, '宝石商人',
 '', '', 0, 80, 80, 0, 35, 129, 1,
 1.14286, 1, 1, 20, 0, 0, 1,
 2000, 2000, 1, 1, 1, 32768, 2048,
 0, 0, 7, 0, 0, 0, 0, 0, 0,
 0, 0, '', 0, 1, 1, 1, 1,
 1, 0, 0, 1, 0, 2, 'npc_gem_merchant',
 0);

-- 模型绑定：Ethereal 商人模型
INSERT INTO `creature_template_model`
(`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
VALUES
(101010, 0, 19956, 1, 1, 0);

-- ============================================================
-- 对话正文（窗口顶部显示，脚本通过 SendGossipMenuFor 引用）
-- 101030：主菜单正文；101031：剥离源力宝石子菜单正文（问候语）
-- 注意：剥离费用「1000 金币」需与脚本常量 GEM_STRIP_COST（1000 * GOLD）保持一致
-- ============================================================
DELETE FROM `npc_text` WHERE `ID` IN (101030, 101031);

INSERT INTO `npc_text` (`ID`, `text0_0`, `Probability0`) VALUES
(101030, '欢迎光临，我可以为你提供宝石商店服务，或帮你剥离装备上的源力宝石。', 1),
(101031, '消耗金币将镶嵌了源力宝石的装备放入背包中进行剥离宝石。', 1);

DELETE FROM `npc_vendor` WHERE (`entry` = 101010) AND (`item` IN (63102, 63103, 63104, 63105, 63106, 63107, 63108, 63109, 63110, 63100, 63101));
INSERT INTO `npc_vendor` (`entry`, `slot`, `item`, `maxcount`, `incrtime`, `ExtendedCost`, `VerifiedBuild`) VALUES
(101010, 0, 63102, 0, 0, 4126, 0),
(101010, 0, 63103, 0, 0, 4126, 0),
(101010, 0, 63104, 0, 0, 4126, 0),
(101010, 0, 63105, 0, 0, 4128, 0),
(101010, 0, 63106, 0, 0, 4129, 0),
(101010, 0, 63107, 0, 0, 4130, 0),
(101010, 0, 63108, 0, 0, 4131, 0),
(101010, 0, 63109, 0, 0, 4133, 0),
(101010, 0, 63110, 0, 0, 4135, 0),
(101010, 0, 63100, 0, 0, 0, 0),
(101010, 0, 63101, 0, 0, 4068, 0);
