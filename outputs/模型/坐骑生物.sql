-- 自定义坐骑生物配置
-- 所有生物模板均参考生物33904（Rusted Proto-Drake）创建。
-- 执行数据库：acore_world

DROP TEMPORARY TABLE IF EXISTS `tmp_mount_template`;
CREATE TEMPORARY TABLE `tmp_mount_template` AS
SELECT
    `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`,
    `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`,
    `speed_walk`, `speed_run`, `speed_swim`, `speed_flight`, `detection_range`, `rank`, `dmgschool`,
    `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`, `unit_class`,
    `unit_flags`, `unit_flags2`, `dynamicflags`, `family`, `type`, `type_flags`, `lootid`, `pickpocketloot`,
    `skinloot`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `HoverHeight`,
    `HealthModifier`, `ManaModifier`, `ArmorModifier`, `ExperienceModifier`, `RacialLeader`, `movementId`,
    `RegenHealth`, `CreatureImmunitiesId`, `flags_extra`, `ScriptName`, `VerifiedBuild`
FROM `creature_template`
WHERE `entry` = 33904;

DROP TEMPORARY TABLE IF EXISTS `tmp_mounts`;
CREATE TEMPORARY TABLE `tmp_mounts` (`entry` INT UNSIGNED, `name` CHAR(100), `model_id` INT UNSIGNED);
INSERT INTO `tmp_mounts` (`entry`, `name`, `model_id`) VALUES
(91006, '翔龙坐骑', 100007),
(91007, '星界灵龙坐骑', 100012),
(91008, '火鹰坐骑', 100013),
(91009, '祥云坐骑', 100014),
(91010, '苍穹龙坐骑', 100015),
(91011, '深岩之洲龙坐骑', 100016),
(91012, '影猎豹坐骑', 100020),
(91013, '苏拉玛坐骑', 100021),
(91014, '圣光机甲', 100025),
(91015, '邪能小格隆坐骑', 100026),
(91016, '虚空龙坐骑', 100029),
(91017, '三角龙坐骑', 100047),
(91018, '骷髅迅猛龙坐骑', 100048),
(91019, '拉格纳罗斯坐骑', 100063);

DELETE FROM `creature_template_model`
WHERE `CreatureID` BETWEEN 91006 AND 91019;

DELETE FROM `creature_template`
WHERE `entry` BETWEEN 91006 AND 91019;

INSERT INTO `creature_template`
(`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `name`,
 `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `speed_walk`,
 `speed_run`, `speed_swim`, `speed_flight`, `detection_range`, `rank`, `dmgschool`, `DamageModifier`,
 `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`,
 `dynamicflags`, `family`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `PetSpellDataId`, `VehicleId`,
 `mingold`, `maxgold`, `AIName`, `MovementType`, `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`,
 `ExperienceModifier`, `RacialLeader`, `movementId`, `RegenHealth`, `CreatureImmunitiesId`, `flags_extra`, `ScriptName`,
 `VerifiedBuild`)
SELECT
    `m`.`entry`, `t`.`difficulty_entry_1`, `t`.`difficulty_entry_2`, `t`.`difficulty_entry_3`, `t`.`KillCredit1`,
    `t`.`KillCredit2`, `m`.`name`, `t`.`subname`, `t`.`IconName`, `t`.`gossip_menu_id`, `t`.`minlevel`, `t`.`maxlevel`,
    `t`.`exp`, `t`.`faction`, `t`.`npcflag`, `t`.`speed_walk`, `t`.`speed_run`, `t`.`speed_swim`, `t`.`speed_flight`,
    `t`.`detection_range`, `t`.`rank`, `t`.`dmgschool`, `t`.`DamageModifier`, `t`.`BaseAttackTime`, `t`.`RangeAttackTime`,
    `t`.`BaseVariance`, `t`.`RangeVariance`, `t`.`unit_class`, `t`.`unit_flags`, `t`.`unit_flags2`, `t`.`dynamicflags`,
    `t`.`family`, `t`.`type`, `t`.`type_flags`, `t`.`lootid`, `t`.`pickpocketloot`, `t`.`skinloot`, `t`.`PetSpellDataId`,
    `t`.`VehicleId`, `t`.`mingold`, `t`.`maxgold`, `t`.`AIName`, `t`.`MovementType`, `t`.`HoverHeight`, `t`.`HealthModifier`,
    `t`.`ManaModifier`, `t`.`ArmorModifier`, `t`.`ExperienceModifier`, `t`.`RacialLeader`, `t`.`movementId`, `t`.`RegenHealth`,
    `t`.`CreatureImmunitiesId`, `t`.`flags_extra`, `t`.`ScriptName`, `t`.`VerifiedBuild`
FROM `tmp_mounts` `m`
CROSS JOIN `tmp_mount_template` `t`;

INSERT INTO `creature_template_model`
(`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`, `VerifiedBuild`)
SELECT `entry`, 0, `model_id`, 1, 1, 0
FROM `tmp_mounts`;

DROP TEMPORARY TABLE IF EXISTS `tmp_mounts`;
DROP TEMPORARY TABLE IF EXISTS `tmp_mount_template`;
