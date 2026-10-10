-- 英雄裂隙Boss掉落更新SQL（独立执行）
-- 适用范围：60级 + 70级 全部英雄裂隙Boss（T1 / T2 / T3）。
 
SET @RIFT_LOOT_ITEM_SARONITE := 49908;    -- 源生萨隆邪铁
SET @RIFT_LOOT_ITEM_POWER_FRAGMENT := 63100; -- 源力碎片
SET @RIFT_LOOT_ITEM_POWER_CORE := 63101;    -- 源力核心
SET @RIFT_LOOT_ITEM_MOUNT_GIFT := 60002;  -- 随机坐骑礼包
SET @RIFT_LOOT_ITEM_ARTIFACT := 109999;   -- 神器材料
SET @RIFT_LOOT_ITEM_LUCKY_COIN := 63000;  -- 幸运币
SET @RIFT_LOOT_ITEM_CHALLENGE_MARK := 62000; -- 挑战印记
SET @RIFT_LOOT_REF_230 := 914000;         -- 230装等装备池
SET @RIFT_LOOT_REF_240 := 924000;         -- 240装等装备池
SET @RIFT_LOOT_REF_250 := 934000;         -- 250装等装备池

SET @RIFT_BOSS_ENTRY_BASE := 100100;      -- 与 60级基础SQL 的 @RIFT_BOSS_ENTRY_BASE 保持一致
SET @RIFT_BOSS_ENTRY_LAST := 100306;      -- 最后一个裂隙Boss entry（100100 + 206）

-- ============================================================================
-- 0. 修正裂隙Boss的 lootid
--    引擎只把「creature_template.lootid 指向的 entry」当作有效掉落宿主，
--    lootid 未指向自身时，写入 creature_loot_template 的掉落行不会被读取，
--    服务器启动会打印 ERROR：
--      Table 'creature_loot_template' Entry X isn't creature entry and
--      not referenced from loot, and thus useless.
--    故先把范围内所有 Boss 的 lootid 强制指回自身 Entry。
-- ============================================================================
UPDATE `creature_template`
SET `lootid` = `entry`
WHERE `entry` BETWEEN @RIFT_BOSS_ENTRY_BASE AND @RIFT_BOSS_ENTRY_LAST
  AND `lootid` <> `entry`;

-- ============================================================================
-- 1. 清理所有裂隙Boss的旧掉落
-- ============================================================================
DELETE FROM `creature_loot_template`
WHERE `Entry` IN (SELECT `entry_id` FROM `heroic_dungeon_rift_boss_tier`);

-- ============================================================================
-- 2. T1 Boss 掉落
--    装备 70% / 230装等 / 1；
--    材料组（源生萨隆邪铁 5% + 源力碎片 20%）/ 1。
-- ============================================================================
INSERT INTO `creature_loot_template`
(`Entry`,`Item`,`Reference`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`)
SELECT `entry_id`,0,@RIFT_LOOT_REF_230,70,0,1,0,1,1,'裂隙 T1 - 230装等装备 70%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=1
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_SARONITE,0,5,0,1,2,1,1,'裂隙 T1 - 材料组 源生萨隆邪铁 5%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=1
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_POWER_FRAGMENT,0,20,0,1,2,1,1,'裂隙 T1 - 材料组 源力碎片 20%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=1
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_CHALLENGE_MARK,0,100,0,1,0,2,2,'裂隙 T1 - 挑战印记 100% - 2个'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=1;

-- ============================================================================
-- 3. T2 Boss 掉落
--    装备组 80%/230 + 20%/240 / 1；
--    掉落组（源生萨隆邪铁 15% + 源力碎片 40% + 随机坐骑礼包 10% + 幸运币 10%）/ 1。
-- ============================================================================
INSERT INTO `creature_loot_template`
(`Entry`,`Item`,`Reference`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`)
SELECT `entry_id`,0,@RIFT_LOOT_REF_230,80,0,1,1,1,1,'裂隙 T2 - 装备组 80% 230装等'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2
UNION ALL
SELECT `entry_id`,0,@RIFT_LOOT_REF_240,20,0,1,1,1,1,'裂隙 T2 - 装备组 20% 240装等'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_SARONITE,0,15,0,1,2,1,1,'裂隙 T2 - 掉落组 源生萨隆邪铁 15%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_POWER_FRAGMENT,0,50,0,1,2,1,1,'裂隙 T2 - 掉落组 源力碎片 50%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_POWER_CORE,0,5,0,1,2,1,1,'裂隙 T3 - 材料组 源力核心 5%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_MOUNT_GIFT,0,10,0,1,2,1,1,'裂隙 T2 - 掉落组 随机坐骑礼包 10%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_LUCKY_COIN,0,10,0,1,2,1,1,'裂隙 T2 - 掉落组 幸运币 10%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_CHALLENGE_MARK,0,100,0,1,0,3,3,'裂隙 T2 - 挑战印记 100% - 3个'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2;

-- ============================================================================
-- 4. T3 Boss 掉落
--    装备组 85%/240 + 15%/250 / 1；
--    掉落组（神器材料 10% + 源生萨隆邪铁 35% + 源力碎片 35% + 源力核心 10% + 随机坐骑礼包 10%）/ 1；
--    幸运币 35% 独立掉落 / 1。
-- ============================================================================
INSERT INTO `creature_loot_template`
(`Entry`,`Item`,`Reference`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`)
SELECT `entry_id`,0,@RIFT_LOOT_REF_240,85,0,1,1,1,1,'裂隙 T3 - 装备组 85% 240装等'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,0,@RIFT_LOOT_REF_250,15,0,1,1,1,1,'裂隙 T3 - 装备组 15% 250装等'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_ARTIFACT,0,10,0,1,2,1,1,'裂隙 T3 - 材料组 10% 神器材料'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_SARONITE,0,35,0,1,2,1,1,'裂隙 T3 - 材料组 35% 源生萨隆邪铁'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_POWER_FRAGMENT,0,35,0,1,2,1,1,'裂隙 T3 - 掉落组 源力碎片 35%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_POWER_CORE,0,10,0,1,2,1,1,'裂隙 T3 - 材料组 源力核心 10%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_MOUNT_GIFT,0,10,0,1,2,1,1,'裂隙 T3 - 掉落组 随机坐骑礼包 10%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_LUCKY_COIN,0,35,0,1,0,1,1,'裂隙 T3 - 幸运币 35% 独立掉落'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_CHALLENGE_MARK,0,100,0,1,0,5,5,'裂隙 T3 - 挑战印记 100% - 5个'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3;

-- ============================================================================
-- 5. 掉落审核查询
-- ============================================================================
SELECT t.`tier`, COUNT(DISTINCT clt.`Entry`) AS boss_count, COUNT(*) AS loot_row_count
FROM `heroic_dungeon_rift_boss_tier` t
JOIN `creature_loot_template` clt ON clt.`Entry` = t.`entry_id`
GROUP BY t.`tier`
ORDER BY t.`tier`;

SELECT clt.`Entry`, t.`tier`, clt.`Item`, clt.`Reference`, clt.`Chance`,
       clt.`GroupId`, clt.`MinCount`, clt.`MaxCount`, clt.`Comment`
FROM `creature_loot_template` clt
JOIN `heroic_dungeon_rift_boss_tier` t ON t.`entry_id` = clt.`Entry`
ORDER BY clt.`Entry`, clt.`GroupId`, clt.`Chance` DESC;
