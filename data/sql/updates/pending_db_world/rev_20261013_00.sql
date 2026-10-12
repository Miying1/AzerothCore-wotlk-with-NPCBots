-- 修复任务 8736「噩梦显现」：伊兰尼库斯刷完两波梦魇魅灵后卡住，既不能被攻击也不攻击
--
-- 原因：伊兰尼库斯（15491）到达 3 号路径点后被 SMART_ACTION_ESCORT_PAUSE 暂停 130 秒，
--       而两波幻象 + 台词链（文本5/6/7）实际需要约 144 秒。
--       护送暂停的恢复条件里有一条 !IsInCombat()：只要他在此期间被玩家或 NPCBot 攻击而进入战斗，
--       护送就永远不会恢复，他会一直停在 3 号路径点上：
--         * 召唤时加的 UNIT_FLAG_IMMUNE_TO_PC|UNIT_FLAG_IMMUNE_TO_NPC（768）没有被移除 -> 不可攻击
--         * ESCORT_START 的 reactState 参数为 0（REACT_PASSIVE），而改为主动的两个脚本都在 4 号路径点 -> 不攻击
--
-- 处理：在第二波结束（文本 7 播放完毕）时无条件把伊兰尼库斯切入战斗阶段，
--       不再依赖护送暂停能否恢复；同时停止护送，避免他继续走到 5 号路径点自杀并判定任务失败。
DELETE FROM `smart_scripts` WHERE `entryorguid` = 15491 AND `source_type` = 0 AND `id` IN (42, 43, 44, 45, 46);
INSERT INTO `smart_scripts` (`entryorguid`, `source_type`, `id`, `link`, `event_type`, `event_phase_mask`, `event_chance`, `event_flags`, `event_param1`, `event_param2`, `event_param3`, `event_param4`, `event_param5`, `event_param6`, `action_type`, `action_param1`, `action_param2`, `action_param3`, `action_param4`, `action_param5`, `action_param6`, `target_type`, `target_param1`, `target_param2`, `target_param3`, `target_param4`, `target_x`, `target_y`, `target_z`, `target_o`, `comment`) VALUES
(15491, 0, 42, 0, 52, 0, 100, 0, 7, 15491, 0, 0, 0, 0, 2, 14, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Eranikus, Tyrant of the Dream - On Text 7 Over - Set Faction 14'),
(15491, 0, 43, 0, 52, 0, 100, 0, 7, 15491, 0, 0, 0, 0, 19, 768, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Eranikus, Tyrant of the Dream - On Text 7 Over - Remove Flags Immune To Players & Immune To NPC'),
(15491, 0, 44, 0, 52, 0, 100, 0, 7, 15491, 0, 0, 0, 0, 8, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Eranikus, Tyrant of the Dream - On Text 7 Over - Set Reactstate Aggressive'),
(15491, 0, 45, 0, 52, 0, 100, 0, 7, 15491, 0, 0, 0, 0, 55, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 'Eranikus, Tyrant of the Dream - On Text 7 Over - Stop Escort'),
(15491, 0, 46, 0, 52, 0, 100, 0, 7, 15491, 0, 0, 0, 0, 49, 0, 0, 0, 0, 0, 0, 18, 40, 0, 0, 0, 0, 0, 0, 0, 'Eranikus, Tyrant of the Dream - On Text 7 Over - Start Attacking');

-- 附带修复：这条「进入战斗时喊话（文本 8：打败了我的手下？来跟我打打看吧，凡人！）」的阶段掩码被误写成 1，
-- 而事件阶段默认是 0，SmartScript::IsInPhase 对 0 直接返回 false，所以它从来不会触发。
-- 同一份脚本链里其余 39 条阶段掩码都是 0，这里统一改成 0。
UPDATE `smart_scripts` SET `event_phase_mask` = 0 WHERE `entryorguid` = 15491 AND `source_type` = 0 AND `id` = 12;
