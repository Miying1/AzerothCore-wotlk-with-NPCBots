-- =====================================================================
-- 真人BOSS：化身技能脚本挂载（spell_script_names）
-- ---------------------------------------------------------------------
-- 作用：让 93000「化身攻城BOSS」走 C++ 法术脚本 spell_real_boss_transform，
--       脚本内按「施法者 GUID」判定真人BOSS，并修改血量/伤害/模型/放大。
-- 导入（手动）：mysql -u<用户> -p<密码> acore_world < "真人BOSS化身脚本SQL.sql"
-- =====================================================================

DELETE FROM `spell_script_names` WHERE `spell_id` = 93000;
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(93000, 'spell_real_boss_transform');
