-- 英雄裂隙：父Aura子法术的T1基线缩放（脚本绑定汇总）
--
-- 通用背景：裂隙C++脚本只有被绑定到对应法术才会加载，且父Aura的BasePoint/倍率不会传递给
-- 引擎直接施放的子法术；因此这里对每个裂隙父/子法术单独建立 spell_script_names 绑定，
-- 由脚本内部判断施法者是否为裂隙Boss（原版Boss与普通生物保留DBC默认行为）。

-- 1) 玛拉顿 · 瑟莱德丝公主 灰尘力场
-- 21909 灰尘力场是挂在Boss身上的自我光环：每1秒通过 PERIODIC_TRIGGER_SPELL 触发子法术 21868
-- （以Boss为中心的范围伤害+击退）。父Aura的BasePoint不会传给子法术，因此必须把 21909 绑定到
-- spell_rift_theradras_dust_field。
DELETE FROM `spell_script_names` WHERE `spell_id` = 21909 AND `ScriptName` = 'spell_rift_theradras_dust_field';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(21909, 'spell_rift_theradras_dust_field');

-- 2) 黑石深渊 · 达格兰·索瑞森大帝 烈焰化身
-- 15636 烈焰化身的第三个效果是光环42（PROC_TRIGGER_SPELL）：Boss每次近战出手都会触发子法术
-- 15643「烈焰」。proc 由引擎直接施放，父Aura的BasePoint不会传给子法术，因此必须把 15636 绑定到
-- spell_rift_thaurissan_avatar_of_flame。
DELETE FROM `spell_script_names` WHERE `spell_id` = 15636 AND `ScriptName` = 'spell_rift_thaurissan_avatar_of_flame';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(15636, 'spell_rift_thaurissan_avatar_of_flame');

-- 3) 幽暗沼泽 · 黑色潜猎者 静电充能跳跃伤害
-- 31715 静电充能的第三个效果是光环23（PERIODIC_TRIGGER_SPELL）：每1秒触发子法术 31716
-- （对自己与周围盟友造成自然伤害，即"跳向邻近盟友"的那一份）。引擎在 SpellAuraEffects.cpp 中
-- 因 31716 不需要显式单位目标而改由"光环承受者"（被点名的玩家）施放该子法术，所以这份伤害
-- 既不过 BossAIBase::DamageDealt（拿不到Tier倍率），也不受核心的非玩家施法等级缩放，恒为DBC原值。
-- 这里把 31716 绑定到裂隙脚本，在命中时按父光环31715施法者（裂隙Boss）的Tier重写伤害。
DELETE FROM `spell_script_names` WHERE `spell_id` = 31716 AND `ScriptName` = 'spell_rift_black_stalker_static_charge';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(31716, 'spell_rift_black_stalker_static_charge');
