-- 自定义坐骑学习物品
-- 参考物品：item_template.entry = 91004
-- 执行数据库：acore_world

DROP TEMPORARY TABLE IF EXISTS `tmp_mount_item_template`;
CREATE TEMPORARY TABLE `tmp_mount_item_template` AS
SELECT *
FROM `item_template`
WHERE `entry` = 91004;

DELETE FROM `item_template`
WHERE `entry` IN (91006, 91007, 91008, 91009, 91010, 91011, 91012, 91013, 91014, 91015, 91016, 91017, 91018, 91019);

UPDATE `tmp_mount_item_template`
SET `entry` = 91006,
    `name` = '翔龙坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91006,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91007,
    `name` = '星界灵龙坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91007,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91008,
    `name` = '火鹰坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91008,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91009,
    `name` = '祥云坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91009,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91010,
    `name` = '苍穹龙坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91010,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91011,
    `name` = '深岩之洲龙坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91011,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91012,
    `name` = '影猎豹坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91012,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91013,
    `name` = '苏拉玛坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91013,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91014,
    `name` = '圣光机甲',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91014,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91015,
    `name` = '邪能小格隆坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91015,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91016,
    `name` = '虚空龙坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91016,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91017,
    `name` = '三角龙坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91017,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91018,
    `name` = '骷髅迅猛龙坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91018,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

UPDATE `tmp_mount_item_template`
SET `entry` = 91019,
    `name` = '拉格纳罗斯坐骑',
    `Quality` = 4,
    `displayid` = 3426,
    `spellcharges_1` = -1,
    `spellid_2` = 91019,
    `description` = '教你学会如何召唤这个坐骑。飞行速度320%。';
INSERT INTO `item_template`
SELECT * FROM `tmp_mount_item_template`;

DROP TEMPORARY TABLE IF EXISTS `tmp_mount_item_template`;
