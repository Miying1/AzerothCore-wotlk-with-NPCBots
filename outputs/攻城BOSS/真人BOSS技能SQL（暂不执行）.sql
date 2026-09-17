-- =====================================================================
-- 真人BOSS：数据库表部分 SQL（暂不执行，先人工审阅）
-- ---------------------------------------------------------------------
-- 说明：
--   1. 技能本体（93000~93005）的 DBC 定义已改由独立 CSV 承载：
--        「真人BOSS技能_Spell.csv」（232 列，表头与 G:\wow\dbc_csv\Spell.csv 一致）
--      因此本文件【不再重复定义技能】，只保留无法放进 DBC 的数据库表部分。
--   2. 本文件包含：
--        - creature_template / creature_template_model ：召唤物（由 .mob 命令召唤）
--   3. 个人掉落需要 C++ 脚本配合（见 src/server/scripts/Custom/SiegeBoss/real_boss_transform.cpp）。
--   4. 召唤物模型请按需替换 creature_template_model 里的 CreatureDisplayID。
-- 导入方式（手动）：mysql -u<用户> -p<密码> acore_world < "真人BOSS技能SQL（暂不执行）.sql"
-- =====================================================================

-- 【重要】若之前执行过"含 spell_dbc 的旧版本 SQL"，请先执行下面这行清理，
-- 否则数据库里的 spell_dbc 记录会覆盖 DBC 文件中的同名技能。
-- DELETE FROM `spell_dbc` WHERE `ID` IN (93000,93001,93002,93003,93004,93005);

-- ---------------------------------------------------------------------
-- 一、召唤物定义（creature_template + creature_template_model）
-- ---------------------------------------------------------------------
DELETE FROM `creature_template` WHERE `entry` IN (910101,910102,910103);
DELETE FROM `creature_template_model` WHERE `CreatureID` IN (910101,910102,910103);

-- 召唤物基础属性（80 级，faction=16 与 BOSS 同阵营，攻击其他玩家；由 .mob 命令召唤）
INSERT INTO `creature_template`
(`entry`,`difficulty_entry_1`,`difficulty_entry_2`,`difficulty_entry_3`,`KillCredit1`,`KillCredit2`,`name`,`subname`,`IconName`,`gossip_menu_id`,`minlevel`,`maxlevel`,`exp`,`faction`,`npcflag`,`speed_walk`,`speed_run`,`speed_swim`,`speed_flight`,`detection_range`,`rank`,`dmgschool`,`DamageModifier`,`BaseAttackTime`,`RangeAttackTime`,`BaseVariance`,`RangeVariance`,`unit_class`,`unit_flags`,`unit_flags2`,`dynamicflags`,`family`,`type`,`type_flags`,`lootid`,`pickpocketloot`,`skinloot`,`PetSpellDataId`,`VehicleId`,`mingold`,`maxgold`,`AIName`,`MovementType`,`HoverHeight`,`HealthModifier`,`ManaModifier`,`ArmorModifier`,`ExperienceModifier`,`RacialLeader`,`movementId`,`RegenHealth`,`CreatureImmunitiesId`,`flags_extra`,`ScriptName`,`VerifiedBuild`) VALUES
-- 910101 蛮兵（近战）
(910101,0,0,0,0,0,'攻城爪牙·蛮兵',NULL,NULL,0,80,80,2,16,0,1,1.14286,1,1,20,1,0,40,2000,2000,1,1,1,0,2048,0,0,6,1,0,0,0,0,0,0,0,'',0,1,5,1,1,1,0,0,1,0,0,'',12340),
-- 910102 射手（远程）
(910102,0,0,0,0,0,'攻城爪牙·射手',NULL,NULL,0,80,80,2,16,0,1,1.14286,1,1,20,1,0,35,2000,2000,1,1,1,0,2048,0,0,6,1,0,0,0,0,0,0,0,'',0,1,4,1,1,1,0,0,1,0,0,'',12340),
-- 910103 精英（大怪）
(910103,0,0,0,0,0,'攻城爪牙·精英',NULL,NULL,0,80,80,2,16,0,1,1.14286,1,1,20,2,0,60,2000,2000,1,1,1,0,2048,0,0,6,1,0,0,0,0,0,0,0,'',0,1,10,1,1,1,0,0,1,0,0,'',12340);

-- 召唤物模型（CreatureDisplayID 请替换为目标生物的显示 ID）
INSERT INTO `creature_template_model` (`CreatureID`,`Idx`,`CreatureDisplayID`,`DisplayScale`,`Probability`) VALUES
(910101,0,10699,1.0,1.0),  -- 蛮兵：食尸鬼模型（示例，可替换）
(910102,0,9786, 1.0,1.0),  -- 射手：骷髅模型（示例，可替换）
(910103,0,1281, 1.0,1.0);  -- 精英：憎恶模型（示例，可替换）

-- ---------------------------------------------------------------------
-- 二、召唤物 AI（可选）：用 SmartAI 让爪牙主动放技能
-- ---------------------------------------------------------------------
-- 机制：召唤物的 AI 由 creature_template.AIName / ScriptName 决定
--       （Guardian 不是 Pet，不会被强制成 PetAI，可正常使用 SmartAI）。
--   · AIName = 'SmartAI'  → 读 smart_scripts 表（推荐，纯 SQL）
--   · ScriptName 有值      → 走注册的 C++ CreatureScript
--   · AIName 为空          → 默认 AI，只会普攻（当前的爪牙就是这种）
--
-- 目标类型注意：爪牙由 .mob 命令创建（纯 TempSummon，有完整仇恨列表），因此
--   ✅ 用 1（自身）、2（当前目标，me->GetVictim()）、5（仇恨随机）等常规目标类型均可
--
-- 启用步骤：
--   1) UPDATE `creature_template` SET `AIName` = 'SmartAI' WHERE `entry` IN (910101,910102,910103);
--   2) 按下面模板为每个爪牙插入 smart_scripts
--
-- 模板（近战爪牙：战斗中每 8~12 秒对当前目标放一次技能）：
-- DELETE FROM `smart_scripts` WHERE `entryorguid` = 910101 AND `source_type` = 0;
-- INSERT INTO `smart_scripts`
-- (`entryorguid`,`source_type`,`id`,`link`,`event_type`,`event_phase_mask`,`event_chance`,`event_flags`,
--  `event_param1`,`event_param2`,`event_param3`,`event_param4`,`event_param5`,`event_param6`,
--  `action_type`,`action_param1`,`action_param2`,`action_param3`,`action_param4`,`action_param5`,`action_param6`,
--  `target_type`,`target_param1`,`target_param2`,`target_param3`,`target_param4`,`target_x`,`target_y`,`target_z`,`target_o`,`comment`) VALUES
-- (910101,0,0,0,0,0,100,0, 3000,5000,8000,12000,0,0, 11,<技能ID>,0,0,0,0,0, 2,0,0,0,0,0,0,0,0, '攻城爪牙·蛮兵 - 战斗中施放技能');
