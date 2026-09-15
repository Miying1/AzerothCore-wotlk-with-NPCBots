-- 放开种族/职业限制
-- 为缺失的 (种族, 职业) 组合补齐出生点、初始物品、动作条
--
-- 复用策略：
--   1. 出生点复用同种族已有组合的坐标
--   2. 初始物品复用同职业已有组合的传家宝配置
--   3. 动作条复用同职业模板，并将模板种族技能替换为目标种族技能
--   4. playercreateinfo_skills / playercreateinfo_spell_custom /
--      playercreateinfo_cast_spell 已通过 racemask/classmask 通配覆盖，无需新增

-- ---------- playercreateinfo 出生点 ----------

DELETE FROM `playercreateinfo` WHERE `race` = 1 AND `class` = 3;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (1, 3, 0, 12, -8949.95, -132.493, 83.5312, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 1 AND `class` = 7;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (1, 7, 0, 12, -8949.95, -132.493, 83.5312, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 1 AND `class` = 11;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (1, 11, 0, 12, -8949.95, -132.493, 83.5312, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 2 AND `class` = 2;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (2, 2, 1, 14, -618.518, -4251.67, 38.718, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 2 AND `class` = 5;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (2, 5, 1, 14, -618.518, -4251.67, 38.718, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 2 AND `class` = 8;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (2, 8, 1, 14, -618.518, -4251.67, 38.718, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 2 AND `class` = 11;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (2, 11, 1, 14, -618.518, -4251.67, 38.718, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 3 AND `class` = 7;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (3, 7, 0, 1, -6240.32, 331.033, 382.758, 6.17716);

DELETE FROM `playercreateinfo` WHERE `race` = 3 AND `class` = 8;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (3, 8, 0, 1, -6240.32, 331.033, 382.758, 6.17716);

DELETE FROM `playercreateinfo` WHERE `race` = 3 AND `class` = 9;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (3, 9, 0, 1, -6240.32, 331.033, 382.758, 6.17716);

DELETE FROM `playercreateinfo` WHERE `race` = 3 AND `class` = 11;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (3, 11, 0, 1, -6240.32, 331.033, 382.758, 6.17716);

DELETE FROM `playercreateinfo` WHERE `race` = 4 AND `class` = 2;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (4, 2, 1, 141, 10311.3, 832.463, 1326.41, 5.69632);

DELETE FROM `playercreateinfo` WHERE `race` = 4 AND `class` = 7;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (4, 7, 1, 141, 10311.3, 832.463, 1326.41, 5.69632);

DELETE FROM `playercreateinfo` WHERE `race` = 4 AND `class` = 8;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (4, 8, 1, 141, 10311.3, 832.463, 1326.41, 5.69632);

DELETE FROM `playercreateinfo` WHERE `race` = 4 AND `class` = 9;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (4, 9, 1, 141, 10311.3, 832.463, 1326.41, 5.69632);

DELETE FROM `playercreateinfo` WHERE `race` = 5 AND `class` = 2;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (5, 2, 0, 85, 1676.71, 1678.31, 121.67, 2.70526);

DELETE FROM `playercreateinfo` WHERE `race` = 5 AND `class` = 3;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (5, 3, 0, 85, 1676.71, 1678.31, 121.67, 2.70526);

DELETE FROM `playercreateinfo` WHERE `race` = 5 AND `class` = 7;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (5, 7, 0, 85, 1676.71, 1678.31, 121.67, 2.70526);

DELETE FROM `playercreateinfo` WHERE `race` = 5 AND `class` = 11;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (5, 11, 0, 85, 1676.71, 1678.31, 121.67, 2.70526);

DELETE FROM `playercreateinfo` WHERE `race` = 6 AND `class` = 2;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (6, 2, 1, 215, -2917.58, -257.98, 52.9968, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 6 AND `class` = 4;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (6, 4, 1, 215, -2917.58, -257.98, 52.9968, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 6 AND `class` = 5;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (6, 5, 1, 215, -2917.58, -257.98, 52.9968, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 6 AND `class` = 8;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (6, 8, 1, 215, -2917.58, -257.98, 52.9968, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 6 AND `class` = 9;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (6, 9, 1, 215, -2917.58, -257.98, 52.9968, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 7 AND `class` = 2;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (7, 2, 0, 1, -6240.32, 331.033, 382.758, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 7 AND `class` = 3;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (7, 3, 0, 1, -6240.32, 331.033, 382.758, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 7 AND `class` = 5;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (7, 5, 0, 1, -6240.32, 331.033, 382.758, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 7 AND `class` = 7;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (7, 7, 0, 1, -6240.32, 331.033, 382.758, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 7 AND `class` = 11;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (7, 11, 0, 1, -6240.32, 331.033, 382.758, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 8 AND `class` = 2;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (8, 2, 1, 14, -618.518, -4251.67, 38.718, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 8 AND `class` = 9;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (8, 9, 1, 14, -618.518, -4251.67, 38.718, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 8 AND `class` = 11;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (8, 11, 1, 14, -618.518, -4251.67, 38.718, 0);

DELETE FROM `playercreateinfo` WHERE `race` = 10 AND `class` = 1;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (10, 1, 530, 3431, 10349.6, -6357.29, 33.4026, 5.31605);

DELETE FROM `playercreateinfo` WHERE `race` = 10 AND `class` = 7;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (10, 7, 530, 3431, 10349.6, -6357.29, 33.4026, 5.31605);

DELETE FROM `playercreateinfo` WHERE `race` = 10 AND `class` = 11;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (10, 11, 530, 3431, 10349.6, -6357.29, 33.4026, 5.31605);

DELETE FROM `playercreateinfo` WHERE `race` = 11 AND `class` = 4;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (11, 4, 530, 3526, -3961.64, -13931.2, 100.615, 2.08364);

DELETE FROM `playercreateinfo` WHERE `race` = 11 AND `class` = 9;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (11, 9, 530, 3526, -3961.64, -13931.2, 100.615, 2.08364);

DELETE FROM `playercreateinfo` WHERE `race` = 11 AND `class` = 11;
INSERT INTO `playercreateinfo` (`race`, `class`, `map`, `zone`, `position_x`, `position_y`, `position_z`, `orientation`) VALUES (11, 11, 530, 3526, -3961.64, -13931.2, 100.615, 2.08364);

-- ---------- playercreateinfo_item 初始物品(传家宝) ----------

DELETE FROM `playercreateinfo_item` WHERE `race` = 1 AND `class` = 3;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 3, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 3, 38880, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 3, 42944, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 3, 42945, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 3, 42946, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 3, 42950, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 3, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 3, 48677, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 1 AND `class` = 7;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 7, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 7, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 7, 38880, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 7, 42945, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 7, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 7, 42950, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 7, 42951, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 7, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 7, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 7, 48677, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 7, 48683, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 1 AND `class` = 11;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 11, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 11, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 11, 38896, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 11, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 11, 42952, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 11, 42984, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 11, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 11, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 11, 48687, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 11, 48689, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (1, 11, 48718, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 2 AND `class` = 2;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 2, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 2, 38873, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 2, 42943, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 2, 42949, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 2, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 2, 44092, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 2, 48685, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 2 AND `class` = 5;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 5, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 5, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 5, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 5, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 5, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 5, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 2 AND `class` = 8;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 8, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 8, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 8, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 8, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 8, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 8, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 2 AND `class` = 11;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 11, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 11, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 11, 38896, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 11, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 11, 42952, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 11, 42984, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 11, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 11, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 11, 48687, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 11, 48689, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (2, 11, 48718, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 3 AND `class` = 7;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 7, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 7, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 7, 38880, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 7, 42945, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 7, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 7, 42950, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 7, 42951, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 7, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 7, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 7, 48677, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 7, 48683, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 3 AND `class` = 8;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 8, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 8, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 8, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 8, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 8, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 8, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 3 AND `class` = 9;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 9, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 9, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 9, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 9, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 9, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 9, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 3 AND `class` = 11;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 11, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 11, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 11, 38896, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 11, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 11, 42952, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 11, 42984, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 11, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 11, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 11, 48687, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 11, 48689, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (3, 11, 48718, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 4 AND `class` = 2;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 2, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 2, 38873, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 2, 42943, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 2, 42949, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 2, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 2, 44092, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 2, 48685, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 4 AND `class` = 7;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 7, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 7, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 7, 38880, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 7, 42945, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 7, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 7, 42950, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 7, 42951, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 7, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 7, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 7, 48677, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 7, 48683, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 4 AND `class` = 8;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 8, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 8, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 8, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 8, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 8, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 8, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 4 AND `class` = 9;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 9, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 9, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 9, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 9, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 9, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (4, 9, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 5 AND `class` = 2;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 2, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 2, 38873, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 2, 42943, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 2, 42949, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 2, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 2, 44092, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 2, 48685, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 5 AND `class` = 3;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 3, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 3, 38880, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 3, 42944, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 3, 42945, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 3, 42946, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 3, 42950, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 3, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 3, 48677, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 5 AND `class` = 7;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 7, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 7, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 7, 38880, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 7, 42945, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 7, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 7, 42950, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 7, 42951, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 7, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 7, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 7, 48677, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 7, 48683, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 5 AND `class` = 11;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 11, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 11, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 11, 38896, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 11, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 11, 42952, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 11, 42984, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 11, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 11, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 11, 48687, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 11, 48689, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (5, 11, 48718, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 6 AND `class` = 2;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 2, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 2, 38873, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 2, 42943, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 2, 42949, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 2, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 2, 44092, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 2, 48685, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 6 AND `class` = 4;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 4, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 4, 38873, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 4, 38880, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 4, 42944, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 4, 42945, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 4, 42946, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 4, 42952, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 4, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 4, 48689, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 6 AND `class` = 5;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 5, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 5, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 5, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 5, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 5, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 5, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 6 AND `class` = 8;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 8, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 8, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 8, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 8, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 8, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 8, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 6 AND `class` = 9;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 9, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 9, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 9, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 9, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 9, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (6, 9, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 7 AND `class` = 2;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 2, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 2, 38873, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 2, 42943, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 2, 42949, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 2, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 2, 44092, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 2, 48685, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 7 AND `class` = 3;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 3, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 3, 38880, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 3, 42944, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 3, 42945, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 3, 42946, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 3, 42950, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 3, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 3, 48677, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 7 AND `class` = 5;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 5, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 5, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 5, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 5, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 5, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 5, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 7 AND `class` = 7;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 7, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 7, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 7, 38880, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 7, 42945, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 7, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 7, 42950, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 7, 42951, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 7, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 7, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 7, 48677, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 7, 48683, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 7 AND `class` = 11;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 11, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 11, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 11, 38896, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 11, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 11, 42952, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 11, 42984, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 11, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 11, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 11, 48687, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 11, 48689, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (7, 11, 48718, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 8 AND `class` = 2;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 2, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 2, 38873, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 2, 42943, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 2, 42949, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 2, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 2, 44092, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 2, 48685, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 8 AND `class` = 9;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 9, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 9, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 9, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 9, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 9, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 9, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 8 AND `class` = 11;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 11, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 11, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 11, 38896, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 11, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 11, 42952, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 11, 42984, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 11, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 11, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 11, 48687, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 11, 48689, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (8, 11, 48718, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 10 AND `class` = 1;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 1, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 1, 38873, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 1, 42943, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 1, 42946, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 1, 42949, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 1, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 1, 44092, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 1, 48685, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 10 AND `class` = 7;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 7, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 7, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 7, 38880, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 7, 42945, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 7, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 7, 42950, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 7, 42951, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 7, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 7, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 7, 48677, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 7, 48683, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 10 AND `class` = 11;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 11, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 11, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 11, 38896, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 11, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 11, 42952, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 11, 42984, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 11, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 11, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 11, 48687, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 11, 48689, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (10, 11, 48718, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 11 AND `class` = 4;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 4, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 4, 38873, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 4, 38880, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 4, 42944, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 4, 42945, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 4, 42946, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 4, 42952, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 4, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 4, 48689, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 11 AND `class` = 9;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 9, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 9, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 9, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 9, 42985, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 9, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 9, 48691, 1);

DELETE FROM `playercreateinfo_item` WHERE `race` = 11 AND `class` = 11;
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 11, 21843, 4);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 11, 38877, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 11, 38896, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 11, 42947, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 11, 42952, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 11, 42984, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 11, 42991, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 11, 42992, 2);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 11, 48687, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 11, 48689, 1);
INSERT INTO `playercreateinfo_item` (`race`, `class`, `itemid`, `amount`) VALUES (11, 11, 48718, 1);

-- ---------- playercreateinfo_action 动作条 ----------

DELETE FROM `playercreateinfo_action` WHERE `race` = 1 AND `class` = 3;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 3, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 3, 1, 2973, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 3, 2, 75, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 3, 3, 59752, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 1 AND `class` = 7;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 7, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 7, 1, 403, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 7, 2, 331, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 7, 3, 59752, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 1 AND `class` = 11;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 11, 0, 5176, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 11, 1, 5185, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 11, 2, 59752, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 11, 72, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (1, 11, 96, 6603, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 2 AND `class` = 2;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 2, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 2, 1, 21084, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 2, 2, 635, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 2, 9, 20572, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 2 AND `class` = 5;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 5, 0, 585, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 5, 1, 2050, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 5, 9, 33702, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 2 AND `class` = 8;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 8, 0, 133, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 8, 1, 168, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 8, 9, 33702, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 2 AND `class` = 11;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 11, 0, 5176, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 11, 1, 5185, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 11, 2, 20572, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 11, 72, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (2, 11, 96, 6603, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 3 AND `class` = 7;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 7, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 7, 1, 403, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 7, 2, 331, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 7, 3, 20594, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 7, 4, 2481, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 3 AND `class` = 8;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 8, 0, 133, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 8, 1, 168, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 8, 9, 20594, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 8, 10, 2481, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 3 AND `class` = 9;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 9, 0, 686, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 9, 1, 687, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 9, 9, 20594, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 9, 10, 2481, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 3 AND `class` = 11;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 11, 0, 5176, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 11, 1, 5185, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 11, 2, 20594, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 11, 72, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 11, 74, 2481, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (3, 11, 96, 6603, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 4 AND `class` = 2;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 2, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 2, 1, 21084, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 2, 2, 635, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 2, 9, 58984, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 4 AND `class` = 7;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 7, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 7, 1, 403, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 7, 2, 331, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 7, 3, 58984, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 4 AND `class` = 8;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 8, 0, 133, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 8, 1, 168, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 8, 9, 58984, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 4 AND `class` = 9;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 9, 0, 686, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 9, 1, 687, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (4, 9, 9, 58984, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 5 AND `class` = 2;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 2, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 2, 1, 21084, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 2, 2, 635, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 2, 9, 20577, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 5 AND `class` = 3;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 3, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 3, 1, 2973, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 3, 2, 75, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 3, 3, 20577, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 5 AND `class` = 7;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 7, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 7, 1, 403, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 7, 2, 331, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 7, 3, 20577, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 5 AND `class` = 11;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 11, 0, 5176, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 11, 1, 5185, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 11, 2, 20577, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 11, 72, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (5, 11, 96, 6603, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 6 AND `class` = 2;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 2, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 2, 1, 21084, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 2, 2, 635, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 2, 9, 20549, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 6 AND `class` = 4;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 4, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 4, 1, 1752, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 4, 2, 2098, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 4, 3, 2764, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 4, 10, 20549, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 6 AND `class` = 5;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 5, 0, 585, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 5, 1, 2050, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 5, 9, 20549, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 6 AND `class` = 8;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 8, 0, 133, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 8, 1, 168, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 8, 9, 20549, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 6 AND `class` = 9;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 9, 0, 686, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 9, 1, 687, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (6, 9, 9, 20549, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 7 AND `class` = 2;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 2, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 2, 1, 21084, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 2, 2, 635, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 2, 9, 20589, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 7 AND `class` = 3;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 3, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 3, 1, 2973, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 3, 2, 75, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 3, 3, 20589, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 7 AND `class` = 5;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 5, 0, 585, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 5, 1, 2050, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 5, 9, 20589, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 7 AND `class` = 7;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 7, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 7, 1, 403, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 7, 2, 331, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 7, 3, 20589, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 7 AND `class` = 11;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 11, 0, 5176, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 11, 1, 5185, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 11, 2, 20589, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 11, 72, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (7, 11, 96, 6603, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 8 AND `class` = 2;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 2, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 2, 1, 21084, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 2, 2, 635, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 2, 9, 26297, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 8 AND `class` = 9;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 9, 0, 686, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 9, 1, 687, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 9, 9, 26297, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 8 AND `class` = 11;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 11, 0, 5176, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 11, 1, 5185, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 11, 2, 26297, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 11, 72, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (8, 11, 96, 6603, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 10 AND `class` = 1;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 1, 72, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 1, 73, 78, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 1, 82, 28730, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 1, 84, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 1, 96, 6603, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 10 AND `class` = 7;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 7, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 7, 1, 403, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 7, 2, 331, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 7, 3, 28730, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 10 AND `class` = 11;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 11, 0, 5176, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 11, 1, 5185, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 11, 2, 28730, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 11, 72, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (10, 11, 96, 6603, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 11 AND `class` = 4;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 4, 0, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 4, 1, 1752, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 4, 2, 2098, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 4, 3, 2764, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 4, 10, 28880, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 11 AND `class` = 9;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 9, 0, 686, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 9, 1, 687, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 9, 9, 59544, 0);

DELETE FROM `playercreateinfo_action` WHERE `race` = 11 AND `class` = 11;
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 11, 0, 5176, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 11, 1, 5185, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 11, 2, 28880, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 11, 72, 6603, 0);
INSERT INTO `playercreateinfo_action` (`race`, `class`, `button`, `action`, `type`) VALUES (11, 11, 96, 6603, 0);
