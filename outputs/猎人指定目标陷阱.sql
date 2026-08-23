-- 玩家选中合法敌对目标时，将指定猎人陷阱释放到目标脚下；否则保持原有脚下释放方式
--60192：冰冻之箭 12409
DELETE FROM `spell_script_names`
WHERE `ScriptName` = 'spell_hun_targeted_trap';

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(-13795, 'spell_hun_targeted_trap'), -- 所有等级的献祭陷阱 max:49056
(-1499, 'spell_hun_targeted_trap'),  -- 所有等级的冰冻陷阱 max:14311
(13809, 'spell_hun_targeted_trap'),  -- 冰霜陷阱（仅一个等级）
(-13813, 'spell_hun_targeted_trap'), -- 所有等级的爆炸陷阱 max:49067
(34600, 'spell_hun_targeted_trap');  -- 毒蛇陷阱（仅一个等级）
