-- 英雄裂隙Boss掉落更新SQL（独立执行）
-- 适用范围：60级 + 70级 全部英雄裂隙Boss（T1 / T2 / T3）。
--
-- 前置依赖：
--   1. 基础SQL已建立 heroic_dungeon_rift_boss_tier 映射表（entry_id -> tier），
--      且 60级 / 70级 裂隙Boss模板（creature_template）已生成、lootid 已指向自身 Entry；
--   2. 装备Reference池已建立：230装等=914000、240装等=924000、250装等=934000；
--   3. 物品已存在：49908（源生萨隆邪铁）、60002（随机坐骑礼包）、109999（神器材料）、63000（幸运币）。
--
-- 掉落规则：
--   - GroupId=0 为独立掉落槽，按 Chance 单独掷骰；
--   - 同一 GroupId 为一组，组内按 Chance 加权、总概率=100% 时必出 1 件；
--   - Reference 为装备池引用，MinCount=MaxCount=1 表示从池内取 1 件。
--
-- T1：装备 70%（230装等）；源生萨隆邪铁 5%。
-- T2：装备组 80%/230 + 20%/240；掉落组（源生萨隆邪铁 15% + 随机坐骑礼包 1% + 幸运币 10%）。
-- T3：装备组 90%/240 + 10%/250；材料组 10%神器材料 + 90%源生萨隆邪铁；坐骑/幸运币组 50%随机坐骑礼包 + 50%幸运币。

SET @RIFT_LOOT_ITEM_SARONITE := 49908;    -- 源生萨隆邪铁
SET @RIFT_LOOT_ITEM_MOUNT_GIFT := 60002;  -- 随机坐骑礼包
SET @RIFT_LOOT_ITEM_ARTIFACT := 109999;   -- 神器材料
SET @RIFT_LOOT_ITEM_LUCKY_COIN := 63000;  -- 幸运币
SET @RIFT_LOOT_REF_230 := 914000;         -- 230装等装备池
SET @RIFT_LOOT_REF_240 := 924000;         -- 240装等装备池
SET @RIFT_LOOT_REF_250 := 934000;         -- 250装等装备池

-- ============================================================================
-- 1. 清理所有裂隙Boss的旧掉落
-- ============================================================================
DELETE FROM `creature_loot_template`
WHERE `Entry` IN (SELECT `entry_id` FROM `heroic_dungeon_rift_boss_tier`);

-- ============================================================================
-- 2. T1 Boss 掉落
--    装备 70% / 230装等 / 1；源生萨隆邪铁 5% / 1。
-- ============================================================================
INSERT INTO `creature_loot_template`
(`Entry`,`Item`,`Reference`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`)
SELECT `entry_id`,0,@RIFT_LOOT_REF_230,70,0,1,0,1,1,'裂隙 T1 - 230装等装备 70%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=1
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_SARONITE,0,5,0,1,0,1,1,'裂隙 T1 - 源生萨隆邪铁 5%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=1;

-- ============================================================================
-- 3. T2 Boss 掉落
--    装备组 80%/230 + 20%/240 / 1；
--    掉落组（源生萨隆邪铁 15% + 随机坐骑礼包 1% + 幸运币 10%）/ 1。
-- ============================================================================
INSERT INTO `creature_loot_template`
(`Entry`,`Item`,`Reference`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`)
SELECT `entry_id`,0,@RIFT_LOOT_REF_230,80,0,1,1,1,1,'裂隙 T2 - 装备组 80% 230装等'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2
UNION ALL
SELECT `entry_id`,0,@RIFT_LOOT_REF_240,20,0,1,1,1,1,'裂隙 T2 - 装备组 20% 240装等'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_SARONITE,0,25,0,1,2,1,1,'裂隙 T2 - 掉落组 源生萨隆邪铁 25%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_MOUNT_GIFT,0,10,0,1,2,1,1,'裂隙 T2 - 掉落组 随机坐骑礼包 10%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_LUCKY_COIN,0,10,0,1,2,1,1,'裂隙 T2 - 掉落组 幸运币 10%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=2;

-- ============================================================================
-- 4. T3 Boss 掉落
--    装备组 90%/240 + 10%/250 / 1；
--    材料组 10%神器材料 + 90%源生萨隆邪铁 / 1；
--    坐骑/幸运币组 50%随机坐骑礼包 + 50%幸运币 / 1。
-- ============================================================================
INSERT INTO `creature_loot_template`
(`Entry`,`Item`,`Reference`,`Chance`,`QuestRequired`,`LootMode`,`GroupId`,`MinCount`,`MaxCount`,`Comment`)
SELECT `entry_id`,0,@RIFT_LOOT_REF_240,90,0,1,1,1,1,'裂隙 T3 - 装备组 90% 240装等'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,0,@RIFT_LOOT_REF_250,10,0,1,1,1,1,'裂隙 T3 - 装备组 10% 250装等'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_ARTIFACT,0,10,0,1,2,1,1,'裂隙 T3 - 材料组 10% 神器材料'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_SARONITE,0,90,0,1,2,1,1,'裂隙 T3 - 材料组 90% 源生萨隆邪铁'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_MOUNT_GIFT,0,50,0,1,3,1,1,'裂隙 T3 - 坐骑/幸运币组 随机坐骑礼包 50%'
FROM `heroic_dungeon_rift_boss_tier` WHERE `tier`=3
UNION ALL
SELECT `entry_id`,@RIFT_LOOT_ITEM_LUCKY_COIN,0,50,0,1,3,1,1,'裂隙 T3 - 坐骑/幸运币组 幸运币 50%'
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
