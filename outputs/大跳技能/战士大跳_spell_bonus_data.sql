-- 战士大跳(90021)落地伤害的 AP 加成
DELETE FROM `spell_bonus_data` WHERE `entry` = 90021;
INSERT INTO `spell_bonus_data` (`entry`, `direct_bonus`, `dot_bonus`, `ap_bonus`, `ap_dot_bonus`, `comments`) VALUES
(90021, 0, 0, 0.5, 0, 'Warrior - Heroic Leap damage (custom)');
