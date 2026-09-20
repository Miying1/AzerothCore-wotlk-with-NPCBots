/*
 * 佣兵幻形 BOT 列表菜单问候语（归属 acore_world 库）
 *
 * 60701 为模块自定义 npc_text ID（当前 npc_text 中空闲，与现有无效的 60700 相邻）。
 */

SET NAMES utf8mb4;

DELETE FROM `npc_text` WHERE `ID` = 60701;
INSERT INTO `npc_text` (`ID`, `text0_0`, `VerifiedBuild`) VALUES
(60701, '给佣兵幻形需要消耗 1 枚幸运币,会长久保持,直到你解雇他。请选择要幻形的佣兵：', -1);
