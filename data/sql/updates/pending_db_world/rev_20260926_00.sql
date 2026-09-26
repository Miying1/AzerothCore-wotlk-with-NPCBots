-- 英雄裂隙：禁魔监狱 · 天怒预言者苏克拉底 父Aura子法术的T1基线缩放
-- 36051 邪能献祭 与 35769 邪火 都是父Aura，自身没有可由裂隙C++覆写的伤害基础点：
--   36051 -> 每3秒触发 35959，35769 -> 每1秒触发 35767。
-- 脚本只在裂隙生物身上接管（原版苏克拉底不受影响），因此需要把父Aura绑定到脚本名。
DELETE FROM `spell_script_names` WHERE `spell_id` IN (36051, 35769);
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(36051, 'spell_rift_soccothrates_fel_immolation'),
(35769, 'spell_rift_soccothrates_felfire');
