-- ============================================================================
-- 卡拉赞（地图 532）Boss 新装备掉落绑定
-- 目标：为卡拉赞 Boss 绑定 250/260/270 装等自定义装备掉落，并清除这些 Boss
--       原版的官方 T4 装备掉落（reference 池 34016~34023/34031/4113），
--       使 Boss 只掉自定义新装备。不改 C++ 机制。
--
-- 装等分组与掉落件数：
--   250 装等（池 934000）：
--     猎手阿图门·骑乘(16152)、莫罗斯(15687)、贞洁圣女(16457) 各掉 2 件；
--     歌剧院 4 单位各掉 1 件：老巫婆(18168)、大灰狼(17521)、罗密欧(17533)、朱丽叶(17534)。
--   260 装等（池 944000）：
--     馆长(15691)、泰雷斯蒂安(15688)、埃兰之影(16524)、虚空幽龙(15689) 各掉 2 件。
--   270 装等（池 954000）：
--     玛克扎尔王子(15690)、夜之魇(17225)、泰恩里斯·米尔克布拉德(28194) 各掉 2 件。
--
-- 依赖：先执行对应装备 SQL，建立 item_template 与 reference_loot_template 池
--       （250=934000、260=944000、270=954000；池内装备 GroupId=1、Chance=0，等概率选 1 件）。
-- 机制：creature_loot_template 中 Item=0 + Reference=池 + Chance=0 + GroupId=10 表示一个掉落槽；
--       掉落件数由 MinCount=MaxCount=N 控制（引擎要求 reference 的 MinCount 必须等于 MaxCount，
--       MaxCount 决定从池内抽取的件数，每件独立等概率）。
-- 注：GroupId 取 10，规避这些 Boss 原有掉落已占用的分组（GroupId 1~3）。
-- ============================================================================

SET @KZ_REF_250 := 934000;
SET @KZ_REF_260 := 944000;
SET @KZ_REF_270 := 954000;

DELETE FROM `creature_loot_template`
WHERE `Reference` IN (@KZ_REF_250, @KZ_REF_260, @KZ_REF_270)
  AND `Entry` IN (16152, 15687, 16457, 18168, 17521, 17533, 17534,
                  15691, 15688, 16524, 15689,
                  15690, 17225, 28194);

-- 清除原版 Reference 掉落：删除 10 个 Boss 挂载的官方 T4 装备池引用
-- （34016~34023/34031/4113），使这些 Boss 不再掉落官方 T4 装备，只掉自定义新装备。
-- 注：王子(15690)、馆长(15691)、罗密欧(17533)、朱丽叶(17534) 无 reference 掉落，
--     其官方装备为直接 Item 掉落，如需一并清除请另行处理。
DELETE FROM `creature_loot_template`
WHERE `Entry` IN (16152, 15687, 16457, 15688, 16524, 15689, 17225, 18168, 17521, 28194)
  AND `Reference` IN (34016, 34017, 34018, 34019, 34020, 34021, 34022, 34023, 34031, 4113);

INSERT INTO `creature_loot_template`
(`Entry`, `Item`, `Reference`, `Chance`, `QuestRequired`, `LootMode`, `GroupId`, `MinCount`, `MaxCount`, `Comment`)
SELECT s.entry, 0, s.reference_entry, 0, 0, 1, 10, s.qty, s.qty,
       CONCAT('卡拉赞 ', s.boss_name, ' - ', s.item_level, '装等装备池 - 掉落', s.qty, '件')
FROM (
    -- 250 装等：主线 Boss 各掉 2 件
    SELECT 16152 AS entry, '阿图门(骑乘)' AS boss_name, 250 AS item_level, @KZ_REF_250 AS reference_entry, 2 AS qty
    UNION ALL SELECT 15687, '莫罗斯', 250, @KZ_REF_250, 2
    UNION ALL SELECT 16457, '贞洁圣女', 250, @KZ_REF_250, 2
    -- 歌剧院 4 单位各掉 1 件
    UNION ALL SELECT 18168, '老巫婆(Oz)', 250, @KZ_REF_250, 1
    UNION ALL SELECT 17521, '大灰狼(小红帽)', 250, @KZ_REF_250, 1
    UNION ALL SELECT 17533, '罗密欧(RAJ)', 250, @KZ_REF_250, 1
    UNION ALL SELECT 17534, '朱丽叶(RAJ)', 250, @KZ_REF_250, 1
    -- 260 装等：4 Boss 各掉 2 件
    UNION ALL SELECT 15691, '馆长', 260, @KZ_REF_260, 2
    UNION ALL SELECT 15688, '泰雷斯蒂安', 260, @KZ_REF_260, 2
    UNION ALL SELECT 16524, '埃兰之影', 260, @KZ_REF_260, 2
    UNION ALL SELECT 15689, '虚空幽龙', 260, @KZ_REF_260, 2
    -- 270 装等：3 Boss 各掉 2 件
    UNION ALL SELECT 15690, '玛克扎尔王子', 270, @KZ_REF_270, 2
    UNION ALL SELECT 17225, '夜之魇', 270, @KZ_REF_270, 2
    UNION ALL SELECT 28194, '泰恩里斯', 270, @KZ_REF_270, 2
) s;

-- ============================================================================
-- 审核查询：确认 14 条掉落记录（10 主线 Boss + 歌剧院 4 单位），掉 2 件 10 条、掉 1 件 4 条
-- ============================================================================
SELECT `Entry`, `Item`, `Reference`, `Chance`, `GroupId`, `MinCount`, `MaxCount`, `Comment`
FROM `creature_loot_template`
WHERE `Reference` IN (@KZ_REF_250, @KZ_REF_260, @KZ_REF_270)
  AND `Entry` IN (16152, 15687, 16457, 18168, 17521, 17533, 17534,
                  15691, 15688, 16524, 15689,
                  15690, 17225, 28194)
ORDER BY `Entry`;
