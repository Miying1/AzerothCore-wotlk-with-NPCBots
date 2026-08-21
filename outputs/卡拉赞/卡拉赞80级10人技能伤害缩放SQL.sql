-- 卡拉赞（地图 532）升级为 80 级 10 人：Boss/召唤物/精英怪技能伤害缩放
-- 依据《卡拉赞80级10人技能伤害C++调整实现方案》§4/§5
-- 将缩放脚本拆分为两个独立脚本名，分别绑定到对应法术：
--   spell_karazhan_direct_scale   直接伤害缩放（SpellScript::OnHit），绑定 direct > 0 的法术
--   spell_karazhan_periodic_scale  DOT 每跳缩放（AuraScript::OnEffectApply），绑定 periodic > 0 的法术
-- 同时具备直伤与 DOT 的法术需同时绑定两个脚本名。
-- 拆分的意义：避免纯直伤法术加载 AuraScript 时，因 DBC 无 SPELL_AURA_PERIODIC_DAMAGE 效果而
-- 触发 "did not match dbc effect data" 加载警告。
-- 幂等：DELETE 按 ScriptName 定向，重复执行无副作用；不影响其它脚本（如 spell_nightbane_fireball_barrage）
-- 无污染：C++ 侧已加地图守卫（GetMapId()==532），非卡拉赞施法者（如裂隙复用的 29901 酸性之牙）不受影响

DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_karazhan_direct_scale','spell_karazhan_periodic_scale');

-- 直接伤害缩放（61 个 direct > 0 的法术）
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(18812,'spell_karazhan_direct_scale'),
(29293,'spell_karazhan_direct_scale'),
(29298,'spell_karazhan_direct_scale'),
(29300,'spell_karazhan_direct_scale'),
(29317,'spell_karazhan_direct_scale'),
(29386,'spell_karazhan_direct_scale'),
(29425,'spell_karazhan_direct_scale'),
(29477,'spell_karazhan_direct_scale'),
(29487,'spell_karazhan_direct_scale'),
(29492,'spell_karazhan_direct_scale'),
(29522,'spell_karazhan_direct_scale'),
(29563,'spell_karazhan_direct_scale'),
(29575,'spell_karazhan_direct_scale'),
(29576,'spell_karazhan_direct_scale'),
(29586,'spell_karazhan_direct_scale'),
(29609,'spell_karazhan_direct_scale'),
(29618,'spell_karazhan_direct_scale'),
(29666,'spell_karazhan_direct_scale'),
(29673,'spell_karazhan_direct_scale'),
(29675,'spell_karazhan_direct_scale'),
(29676,'spell_karazhan_direct_scale'),
(29677,'spell_karazhan_direct_scale'),
(29680,'spell_karazhan_direct_scale'),
(29684,'spell_karazhan_direct_scale'),
(29711,'spell_karazhan_direct_scale'),
(29712,'spell_karazhan_direct_scale'),
(29717,'spell_karazhan_direct_scale'),
(29765,'spell_karazhan_direct_scale'),
(29885,'spell_karazhan_direct_scale'),
(29904,'spell_karazhan_direct_scale'),
(29919,'spell_karazhan_direct_scale'),
(29922,'spell_karazhan_direct_scale'),
(29925,'spell_karazhan_direct_scale'),
(29928,'spell_karazhan_direct_scale'),
(29939,'spell_karazhan_direct_scale'),
(29949,'spell_karazhan_direct_scale'),
(29953,'spell_karazhan_direct_scale'),
(29954,'spell_karazhan_direct_scale'),
(29956,'spell_karazhan_direct_scale'),
(29973,'spell_karazhan_direct_scale'),
(29978,'spell_karazhan_direct_scale'),
(30050,'spell_karazhan_direct_scale'),
(30055,'spell_karazhan_direct_scale'),
(30128,'spell_karazhan_direct_scale'),
(30180,'spell_karazhan_direct_scale'),
(30210,'spell_karazhan_direct_scale'),
(30282,'spell_karazhan_direct_scale'),
(30358,'spell_karazhan_direct_scale'),
(30383,'spell_karazhan_direct_scale'),
(30815,'spell_karazhan_direct_scale'),
(30852,'spell_karazhan_direct_scale'),
(30860,'spell_karazhan_direct_scale'),
(30890,'spell_karazhan_direct_scale'),
(31012,'spell_karazhan_direct_scale'),
(31041,'spell_karazhan_direct_scale'),
(32337,'spell_karazhan_direct_scale'),
(32445,'spell_karazhan_direct_scale'),
(37057,'spell_karazhan_direct_scale'),
(37078,'spell_karazhan_direct_scale'),
(37161,'spell_karazhan_direct_scale'),
(38524,'spell_karazhan_direct_scale');

-- DOT 周期伤害缩放（24 个 periodic > 0 的法术；其中 10 个同时具备直伤，需与 direct 脚本并存）
INSERT INTO `spell_script_names` (`spell_id`,`ScriptName`) VALUES
(24212,'spell_karazhan_periodic_scale'),
(29293,'spell_karazhan_periodic_scale'),
(29491,'spell_karazhan_periodic_scale'),
(29522,'spell_karazhan_periodic_scale'),
(29563,'spell_karazhan_periodic_scale'),
(29570,'spell_karazhan_periodic_scale'),
(29578,'spell_karazhan_periodic_scale'),
(29670,'spell_karazhan_periodic_scale'),
(29675,'spell_karazhan_periodic_scale'),
(29901,'spell_karazhan_periodic_scale'),
(29906,'spell_karazhan_periodic_scale'),
(29922,'spell_karazhan_periodic_scale'),
(29925,'spell_karazhan_periodic_scale'),
(29928,'spell_karazhan_periodic_scale'),
(29930,'spell_karazhan_periodic_scale'),
(29935,'spell_karazhan_periodic_scale'),
(29951,'spell_karazhan_periodic_scale'),
(29964,'spell_karazhan_periodic_scale'),
(30129,'spell_karazhan_periodic_scale'),
(30210,'spell_karazhan_periodic_scale'),
(30854,'spell_karazhan_periodic_scale'),
(30890,'spell_karazhan_periodic_scale'),
(31041,'spell_karazhan_periodic_scale'),
(37066,'spell_karazhan_periodic_scale');
