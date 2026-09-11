-- 禁止装备团队光环 92000(星光之拥)/92003(月影光环) 多来源叠加：同组光环互斥，只保留最高值
-- 92000 与 92003 分别放入不同组，保证"法术爆击光环"与"物理爆击光环"可以共存
DELETE FROM `spell_group` WHERE `id` IN (920000, 920003) OR `spell_id` IN (92000, 92003);
INSERT INTO `spell_group` (`id`,`spell_id`) VALUES (920000, 92000), (920003, 92003);

DELETE FROM `spell_group_stack_rules` WHERE `group_id` IN (920000, 920003);
INSERT INTO `spell_group_stack_rules` (`group_id`,`stack_rule`, `description`) VALUES
(920000, 4, '星光之拥(92000)装备团队光环 禁止叠加 只取最高 SPELL_GROUP_STACK_RULE_EXCLUSIVE_HIGHEST'),
(920003, 4, '月影光环(92003)装备团队光环 禁止叠加 只取最高 SPELL_GROUP_STACK_RULE_EXCLUSIVE_HIGHEST');
