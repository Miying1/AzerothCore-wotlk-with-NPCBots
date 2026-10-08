/* =====================================================================================
   账号银行 / 银行加载 bug —— 丢失物品恢复脚本（依据服务器日志的 item / bag / slot）
   -------------------------------------------------------------------------------------
   数据来源（三份日志，共 43 件物品，另有宿主背包 312332）：
     ① 量子风（角色 172）：447641(深渊口袋,顶层槽67 失败 reason 17)
                        + 背包 312332 内 15 件（槽 1~9、30~35）
     ② 奈奥尤（角色  92）：369042(深渊口袋,顶层槽67 失败 reason 17)
                        + 背包 369042 内 1 件（槽 0）
     ③ 奥德彪（角色   2）：445804(深渊口袋,顶层槽68 失败 reason 17)
                        + 背包 445804 内 24 件（槽 0~19、27~30）
    注：445804 落在 68 号背包槽，说明当时 67 号已被别的背包占用；脚本会自动挑空闲背包槽。

   目标库：acore_characters（世界库按需改 `acore_world`.item_template）
   数据库：需要 MySQL 8.0（用到递归 CTE + 窗口函数）

   执行前置（缺一不可）：
     1) 备份：mysqldump -uroot -p acore_characters > chars_before_recover.sql
     2) 停 worldserver，或至少让日志里的角色（量子风 / 奈奥尤 / 奥德彪）离线
        否则运行中的服务端会用内存库存覆盖本脚本写入的行
     3) 部署「修复版」worldserver 后再上线，否则旧二进制会再删一次：
        · 加载期间跳过唯一数量校验（不再误判 reason 17）
        · 加载失败不再删除数据库映射（不再不可逆丢件）
        · 背包跨越个人/账号银行边界时同步迁移包内物品映射（不再产生孤儿映射）
        · 银行物品槽 39~66 拒收非空背包（不再产生“落在物品槽里的背包”）

   脚本会自动判断每件物品该落回哪个银行：
     item_instance.owner_guid = 0         -> 账号银行（account_bank_item，owner 必须为 0）
     item_instance.owner_guid = 角色 guid  -> 该角色个人银行（character_inventory）
   并且宿主背包仍在背包槽（背包 19~22 / 银行 67~73）时优先按原槽位放回包内；
   否则退化为银行顶层物品槽（39~66）自动分配（会失去“装在包里”的形态，物品不丢）。

   用法：
     mysql -uroot -p --default-character-set=utf8mb4 < 账号银行丢件恢复.sql
     第 0~3 段只读+临时表（3.7/3.8 先核对计划），第 4 段落库，第 5 段验证/回滚，第 6 段可选清理。
   ===================================================================================== */

-- 角色库名（若你的库不叫 acore_characters，改这一行；下方 item_template 用的 acore_world 同理）
USE acore_characters;

-- =====================================================================================
-- 0. 参数
-- =====================================================================================
SET @name1 = '量子风';
SET @char1 = (SELECT guid    FROM characters WHERE name = @name1 LIMIT 1);
SET @acc1  = (SELECT account FROM characters WHERE guid = @char1);
SET @name2 = '奈奥尤';
SET @char2 = (SELECT guid    FROM characters WHERE name = @name2 LIMIT 1);
SET @acc2  = (SELECT account FROM characters WHERE guid = @char2);
SET @name3 = '奥德彪';
SET @char3 = (SELECT guid    FROM characters WHERE name = @name3 LIMIT 1);
SET @acc3  = (SELECT account FROM characters WHERE guid = @char3);

SELECT @name1 AS name1, @char1 AS char_guid1, @acc1 AS account1,
       @name2 AS name2, @char2 AS char_guid2, @acc2 AS account2,
       @name3 AS name3, @char3 AS char_guid3, @acc3 AS account3;

-- 三行必须都是 1，否则对应角色名写错（或该角色不在本库），改对再继续
SELECT (@char1 IS NOT NULL) AS ok_name1, (@char2 IS NOT NULL) AS ok_name2, (@char3 IS NOT NULL) AS ok_name3;

-- =====================================================================================
-- 1. 备份（回滚依据：物品原 owner_guid；稍后还会备份本次的位置计划）
-- =====================================================================================
DROP TABLE IF EXISTS bk_rec_item_instance;
CREATE TABLE bk_rec_item_instance AS
SELECT * FROM item_instance
WHERE guid IN (
    -- ① 量子风
    447641,139526,143165,111193,130249,129639,129632,129859,130082,129949,
    326517,383334,110202,383001,107476,111959,
    -- ② 奈奥尤
    369042,204365,
    -- ③ 奥德彪
    445804,388496,353391,302456,302813,113556,302876,94584,275407,276969,
    350686,352186,352223,334055,350644,17631,20088,385258,350618,429688,
    432917,385840,408159,257298,432746,
    -- 宿主背包
    312332);

SELECT COUNT(*) AS backup_item_rows FROM bk_rec_item_instance;   -- 期望 43（+1 若 312332 还在库里）

-- =====================================================================================
-- 2. 体检
-- =====================================================================================
-- 2.1 日志数据表（物品 -> 原容器/槽位）
DROP TEMPORARY TABLE IF EXISTS tmp_rec_map;
CREATE TEMPORARY TABLE tmp_rec_map (
    item_guid INT UNSIGNED     NOT NULL,
    bag_guid  INT UNSIGNED     NOT NULL,   -- 0 = 银行顶层槽位
    slot      TINYINT UNSIGNED NOT NULL,
    char_name VARCHAR(64)      NOT NULL,
    PRIMARY KEY (item_guid)
);
INSERT INTO tmp_rec_map VALUES
    -- ① 量子风（角色 low guid 172）；447641 = 61004 深渊口袋
    (447641,      0, 67, '量子风'),
    (139526, 312332,  1, '量子风'),   -- 60100 泰坦符文
    (143165, 312332,  2, '量子风'),   -- 62011 挑战奖章
    (111193, 312332,  3, '量子风'),   -- 43102 冰冻宝珠
    (130249, 312332,  4, '量子风'),   -- 37704 生命结晶
    (129639, 312332,  5, '量子风'),   -- 37701 土之结晶
    (129632, 312332,  6, '量子风'),   -- 37703 暗影结晶
    (129859, 312332,  7, '量子风'),   -- 37700 空气结晶
    (130082, 312332,  8, '量子风'),   -- 37705 水之结晶
    (129949, 312332,  9, '量子风'),   -- 37702 火焰结晶
    (326517, 312332, 30, '量子风'),   -- 60702 圣灵珠
    (383334, 312332, 31, '量子风'),   -- 60705 幻影珠
    (110202, 312332, 32, '量子风'),   -- 60703 觅影珠
    (383001, 312332, 33, '量子风'),   -- 60703 觅影珠
    (107476, 312332, 34, '量子风'),   -- 60704 魅影珠
    (111959, 312332, 35, '量子风'),   -- 60705 幻影珠
    -- ② 奈奥尤（角色 low guid 92）
    (369042,      0, 67, '奈奥尤'),   -- 61004 深渊口袋
    (204365, 369042,  0, '奈奥尤'),   -- 49908 源生萨隆邪铁
    -- ③ 奥德彪（角色 low guid 2）；445804 = 61004 深渊口袋（顶层槽 68）
    (445804,      0, 68, '奥德彪'),
    (388496, 445804,  0, '奥德彪'),   -- 50266 上古冰熊之皮
    (353391, 445804,  1, '奥德彪'),   -- 99931 炽焰驭天战锤
    (302456, 445804,  2, '奥德彪'),   -- 50293 黑色背叛肩甲
    (302813, 445804,  3, '奥德彪'),   -- 50264 碎羽护腕
    (113556, 445804,  4, '奥德彪'),   -- 41667 狂怒角斗士的龙皮护腿
    (302876, 445804,  5, '奥德彪'),   -- 50319 未开刃的寒冰剃刀
    (94584,  445804,  6, '奥德彪'),   -- 99905 命轮手斧
    (275407, 445804,  7, '奥德彪'),   -- 50291 碎魂者
    (276969, 445804,  8, '奥德彪'),   -- 50291 碎魂者
    (350686, 445804,  9, '奥德彪'),   -- 99846 赤霞·披肩
    (352186, 445804, 10, '奥德彪'),   -- 99742 惊涛护手
    (352223, 445804, 11, '奥德彪'),   -- 99915 轮回破界猎枪
    (334055, 445804, 12, '奥德彪'),   -- 39720 衰退护腿
    (350644, 445804, 13, '奥德彪'),   -- 99711 血月守护头冠
    (17631,  445804, 14, '奥德彪'),   -- 47508 阿雷达尔的战星之锤
    (20088,  445804, 15, '奥德彪'),   -- 49827 食尸鬼切割者
    (385258, 445804, 16, '奥德彪'),   -- 99471 晶歌森林玄铁护腿
    (350618, 445804, 17, '奥德彪'),   -- 99887 神恩之大盾
    (429688, 445804, 18, '奥德彪'),   -- 99953 裂云大剑
    (432917, 445804, 19, '奥德彪'),   -- 40258 谋划护符
    (385840, 445804, 27, '奥德彪'),   -- 99521 幽暗城月刃战盔
    (408159, 445804, 28, '奥德彪'),   -- 99722 龙骨之护甲
    (257298, 445804, 29, '奥德彪'),   -- 49118 光明汽酒咒符
    (432746, 445804, 30, '奥德彪');   -- 40256 死亡之钟

-- 2.2 每件物品是否还在、是否已有位置（state = 待恢复 的才会被处理）
SELECT m.char_name, m.item_guid, m.bag_guid AS orig_bag, m.slot AS orig_slot,
       ii.itemEntry, it.name, ii.owner_guid, ii.count,
       (ci.item IS NOT NULL)      AS in_char_inv,
       (ab.item IS NOT NULL)      AS in_acc_bank,
       (mi.item_guid IS NOT NULL) AS in_mail,
       CASE WHEN ii.guid IS NULL THEN '物品已不存在(需从 bk / 旧备份找回)'
            WHEN ci.item IS NOT NULL OR ab.item IS NOT NULL OR mi.item_guid IS NOT NULL THEN '已有位置(会跳过)'
            ELSE '待恢复' END AS state
FROM tmp_rec_map m
LEFT JOIN item_instance ii             ON ii.guid = m.item_guid
LEFT JOIN acore_world.item_template it ON it.entry = ii.itemEntry
LEFT JOIN character_inventory ci       ON ci.item = m.item_guid
LEFT JOIN account_bank_item   ab       ON ab.item = m.item_guid
LEFT JOIN mail_items          mi       ON mi.item_guid = m.item_guid
ORDER BY m.char_name, m.bag_guid, m.slot;

-- 2.3 宿主背包现在在哪 / 有没有空位（只有落在背包槽 19~22、67~73 才能继续当容器）
SELECT b.guid AS bag_guid, ii.itemEntry, it.name, it.ContainerSlots,
       (SELECT COUNT(*) FROM character_inventory x WHERE x.bag = b.guid) AS used_in_char_inv,
       (SELECT COUNT(*) FROM account_bank_item   x WHERE x.bag = b.guid) AS used_in_acc_bank,
       bi.guid  AS ci_guid,  bi.bag  AS ci_bag,  bi.slot  AS ci_slot,
       abi.account_id AS abi_account, abi.bag AS abi_bag, abi.slot AS abi_slot
FROM (SELECT DISTINCT bag_guid AS guid FROM tmp_rec_map WHERE bag_guid <> 0) b
JOIN item_instance ii                  ON ii.guid = b.guid
LEFT JOIN acore_world.item_template it ON it.entry = ii.itemEntry
LEFT JOIN character_inventory bi       ON bi.item = b.guid
LEFT JOIN account_bank_item   abi      ON abi.item = b.guid
ORDER BY b.guid;

-- 2.4 三个账号的银行背包槽购买情况（顶层背包要放回 67~73 的前提）
SELECT @acc1 AS account_id, COALESCE((SELECT slot_count FROM account_bank_slots WHERE account_id=@acc1),0) AS acc_bank_bag_slots,
       (SELECT COUNT(*) FROM account_bank_item WHERE account_id=@acc1 AND bag=0 AND slot BETWEEN 67 AND 73) AS acc_bank_bag_used
UNION ALL
SELECT @acc2, COALESCE((SELECT slot_count FROM account_bank_slots WHERE account_id=@acc2),0),
       (SELECT COUNT(*) FROM account_bank_item WHERE account_id=@acc2 AND bag=0 AND slot BETWEEN 67 AND 73)
UNION ALL
SELECT @acc3, COALESCE((SELECT slot_count FROM account_bank_slots WHERE account_id=@acc3),0),
       (SELECT COUNT(*) FROM account_bank_item WHERE account_id=@acc3 AND bag=0 AND slot BETWEEN 67 AND 73);

SELECT c.name, c.guid, c.account, c.level, c.bankSlots AS personal_bank_bag_slots,
       (SELECT COUNT(*) FROM character_inventory x WHERE x.guid=c.guid AND x.bag=0 AND x.slot BETWEEN 67 AND 73) AS personal_bank_bag_used
FROM characters c WHERE c.guid IN (@char1, @char2, @char3);

-- =====================================================================================
-- 3. 生成恢复计划（只建临时表，不动业务表）
--    优先级：① 宿主背包仍在背包槽 -> 按原槽位放回包内
--            ② 顶层容器 -> 背包槽（已购买且空闲）优先，其次物品槽
--            ③ 其余 -> 银行顶层物品槽 39~66
-- =====================================================================================
-- 3.1 待恢复清单：解析目标银行(1=账号银行/2=个人银行) + 排除已有位置/已删的物品
DROP TEMPORARY TABLE IF EXISTS tmp_rec_pending;
CREATE TEMPORARY TABLE tmp_rec_pending (
    item_guid    INT UNSIGNED     NOT NULL,
    target_type  TINYINT UNSIGNED NOT NULL,
    target_id    INT UNSIGNED     NOT NULL,
    bag_guid     INT UNSIGNED     NOT NULL,
    slot         TINYINT UNSIGNED NOT NULL,
    is_container TINYINT UNSIGNED NOT NULL,
    PRIMARY KEY (item_guid)
);
INSERT INTO tmp_rec_pending (item_guid, target_type, target_id, bag_guid, slot, is_container)
SELECT m.item_guid,
       CASE WHEN ii.owner_guid = 0 THEN 1 ELSE 2 END,
       CASE WHEN ii.owner_guid = 0 THEN co.account ELSE ii.owner_guid END,
       m.bag_guid, m.slot,
       CASE WHEN it.class = 1 THEN 1 ELSE 0 END
FROM tmp_rec_map m
JOIN item_instance ii                  ON ii.guid = m.item_guid
LEFT JOIN acore_world.item_template it ON it.entry = ii.itemEntry
LEFT JOIN characters co                ON co.name = m.char_name
WHERE NOT EXISTS (SELECT 1 FROM character_inventory x WHERE x.item = m.item_guid)
  AND NOT EXISTS (SELECT 1 FROM account_bank_item   x WHERE x.item = m.item_guid)
  AND NOT EXISTS (SELECT 1 FROM mail_items          x WHERE x.item_guid = m.item_guid);

SELECT target_type, target_id, COUNT(*) AS pending_count FROM tmp_rec_pending GROUP BY target_type, target_id;

-- 3.2 目标去重（MySQL 不允许同一语句内两次引用同一个临时表，故单独建表）
DROP TEMPORARY TABLE IF EXISTS tmp_rec_targets;
CREATE TEMPORARY TABLE tmp_rec_targets (
    target_type TINYINT UNSIGNED NOT NULL, target_id INT UNSIGNED NOT NULL,
    PRIMARY KEY (target_type, target_id)
);
INSERT INTO tmp_rec_targets (target_type, target_id)
SELECT DISTINCT target_type, target_id FROM tmp_rec_pending;

-- 3.3 顶层容器 -> 银行背包槽（个人银行看 characters.bankSlots，账号银行看 account_bank_slots.slot_count）
DROP TEMPORARY TABLE IF EXISTS tmp_plan_container;
CREATE TEMPORARY TABLE tmp_plan_container (
    target_type TINYINT UNSIGNED NOT NULL, target_id INT UNSIGNED NOT NULL,
    slot TINYINT UNSIGNED NOT NULL, item_guid INT UNSIGNED NOT NULL,
    PRIMARY KEY (item_guid)
);
INSERT INTO tmp_plan_container (target_type, target_id, slot, item_guid)
WITH RECURSIVE bagslots AS (SELECT 67 AS s UNION ALL SELECT s+1 FROM bagslots WHERE s < 73),
pool AS (
    SELECT t.target_type, t.target_id, b.s,
           ROW_NUMBER() OVER (PARTITION BY t.target_type, t.target_id ORDER BY b.s) AS rn
    FROM tmp_rec_targets t
    JOIN bagslots b
    WHERE ((t.target_type = 1 AND (b.s - 67) < COALESCE((SELECT slot_count FROM account_bank_slots s WHERE s.account_id = t.target_id), 0))
        OR (t.target_type = 2 AND (b.s - 67) < COALESCE((SELECT bankSlots FROM characters c WHERE c.guid = t.target_id), 0)))
      AND NOT EXISTS (SELECT 1 FROM account_bank_item a WHERE t.target_type = 1 AND a.account_id = t.target_id AND a.bag = 0 AND a.slot = b.s)
      AND NOT EXISTS (SELECT 1 FROM character_inventory c WHERE t.target_type = 2 AND c.guid = t.target_id AND c.bag = 0 AND c.slot = b.s)
),
need AS (
    SELECT p.target_type, p.target_id, p.item_guid,
           ROW_NUMBER() OVER (PARTITION BY p.target_type, p.target_id ORDER BY p.item_guid) AS rn
    FROM tmp_rec_pending p
    WHERE p.is_container = 1
)
SELECT n.target_type, n.target_id, f.s, n.item_guid
FROM need n JOIN pool f ON f.target_type = n.target_type AND f.target_id = n.target_id AND f.rn = n.rn;

-- 3.4 宿主背包可用 -> 按日志原槽位放回包内（槽位必须在该包容量内且未被占用）
DROP TEMPORARY TABLE IF EXISTS tmp_plan_host;
CREATE TEMPORARY TABLE tmp_plan_host (
    target_type TINYINT UNSIGNED NOT NULL, target_id INT UNSIGNED NOT NULL,
    bag INT UNSIGNED NOT NULL, slot TINYINT UNSIGNED NOT NULL, item_guid INT UNSIGNED NOT NULL,
    PRIMARY KEY (item_guid)
);
-- 3.4a 宿主背包在账号银行背包槽
INSERT IGNORE INTO tmp_plan_host (target_type, target_id, bag, slot, item_guid)
SELECT 1, ab.account_id, p.bag_guid, p.slot, p.item_guid
FROM tmp_rec_pending p
JOIN account_bank_item ab          ON ab.item = p.bag_guid AND ab.bag = 0 AND ab.slot BETWEEN 67 AND 73
JOIN item_instance bi              ON bi.guid = p.bag_guid
JOIN acore_world.item_template bit ON bit.entry = bi.itemEntry AND bit.ContainerSlots > p.slot
WHERE p.bag_guid <> 0
  AND NOT EXISTS (SELECT 1 FROM account_bank_item x
                  WHERE x.account_id = ab.account_id AND x.bag = p.bag_guid AND x.slot = p.slot);

-- 3.4b 宿主背包在角色背包(19~22)/个人银行背包槽(67~73)
INSERT IGNORE INTO tmp_plan_host (target_type, target_id, bag, slot, item_guid)
SELECT 2, ci.guid, p.bag_guid, p.slot, p.item_guid
FROM tmp_rec_pending p
JOIN character_inventory ci        ON ci.item = p.bag_guid AND ci.bag = 0
                                      AND (ci.slot BETWEEN 19 AND 22 OR ci.slot BETWEEN 67 AND 73)
JOIN item_instance bi              ON bi.guid = p.bag_guid
JOIN acore_world.item_template bit ON bit.entry = bi.itemEntry AND bit.ContainerSlots > p.slot
WHERE p.bag_guid <> 0
  AND NOT EXISTS (SELECT 1 FROM character_inventory x
                  WHERE x.guid = ci.guid AND x.bag = p.bag_guid AND x.slot = p.slot);

-- 3.4c 宿主背包刚被 3.3 放回银行背包槽（例如 369042、445804 这类刚丢了映射的顶层背包）
INSERT IGNORE INTO tmp_plan_host (target_type, target_id, bag, slot, item_guid)
SELECT c.target_type, c.target_id, p.bag_guid, p.slot, p.item_guid
FROM tmp_rec_pending p
JOIN tmp_plan_container c          ON c.item_guid = p.bag_guid
JOIN item_instance bi              ON bi.guid = p.bag_guid
JOIN acore_world.item_template bit ON bit.entry = bi.itemEntry AND bit.ContainerSlots > p.slot
WHERE p.bag_guid <> 0
  AND NOT EXISTS (SELECT 1 FROM account_bank_item x
                  WHERE c.target_type = 1 AND x.account_id = c.target_id AND x.bag = p.bag_guid AND x.slot = p.slot)
  AND NOT EXISTS (SELECT 1 FROM character_inventory x
                  WHERE c.target_type = 2 AND x.guid = c.target_id AND x.bag = p.bag_guid AND x.slot = p.slot);

-- 3.5 其余物品（含没抢到背包槽的容器、宿主背包不可用的包内物品）-> 银行顶层物品槽 39~66
DROP TEMPORARY TABLE IF EXISTS tmp_plan_item;
CREATE TEMPORARY TABLE tmp_plan_item (
    target_type TINYINT UNSIGNED NOT NULL, target_id INT UNSIGNED NOT NULL,
    slot TINYINT UNSIGNED NOT NULL, item_guid INT UNSIGNED NOT NULL,
    PRIMARY KEY (item_guid)
);
INSERT INTO tmp_plan_item (target_type, target_id, slot, item_guid)
WITH RECURSIVE slots AS (SELECT 39 AS s UNION ALL SELECT s+1 FROM slots WHERE s < 66),
pool AS (
    SELECT t.target_type, t.target_id, s.s,
           ROW_NUMBER() OVER (PARTITION BY t.target_type, t.target_id ORDER BY s.s) AS rn
    FROM tmp_rec_targets t
    JOIN slots s
    WHERE NOT EXISTS (SELECT 1 FROM account_bank_item a WHERE t.target_type = 1 AND a.account_id = t.target_id AND a.bag = 0 AND a.slot = s.s)
      AND NOT EXISTS (SELECT 1 FROM character_inventory c WHERE t.target_type = 2 AND c.guid = t.target_id AND c.bag = 0 AND c.slot = s.s)
),
need AS (
    SELECT p.target_type, p.target_id, p.item_guid,
           ROW_NUMBER() OVER (PARTITION BY p.target_type, p.target_id ORDER BY p.item_guid) AS rn
    FROM tmp_rec_pending p
    WHERE NOT EXISTS (SELECT 1 FROM tmp_plan_container x WHERE x.item_guid = p.item_guid)
      AND NOT EXISTS (SELECT 1 FROM tmp_plan_host      x WHERE x.item_guid = p.item_guid)
)
SELECT n.target_type, n.target_id, f.s, n.item_guid
FROM need n JOIN pool f ON f.target_type = n.target_type AND f.target_id = n.target_id AND f.rn = n.rn;

-- 3.6 合并成最终计划
DROP TEMPORARY TABLE IF EXISTS tmp_rec_plan;
CREATE TEMPORARY TABLE tmp_rec_plan (
    target_type TINYINT UNSIGNED NOT NULL, target_id INT UNSIGNED NOT NULL,
    bag INT UNSIGNED NOT NULL, slot TINYINT UNSIGNED NOT NULL, item_guid INT UNSIGNED NOT NULL,
    PRIMARY KEY (item_guid)
);
INSERT INTO tmp_rec_plan (target_type, target_id, bag, slot, item_guid)
SELECT target_type, target_id, bag, slot, item_guid FROM tmp_plan_host
UNION ALL
SELECT target_type, target_id, 0, slot, item_guid FROM tmp_plan_container
UNION ALL
SELECT target_type, target_id, 0, slot, item_guid FROM tmp_plan_item;

-- 3.7 计划总览（务必核对：条数、目标、槽位是否符合预期）
SELECT p.char_name, CASE WHEN p.target_type = 1 THEN '账号银行' ELSE '个人银行' END AS bank_name,
       p.target_id, p.bag, p.slot, p.item_guid, ii.itemEntry, it.name,
       CASE WHEN p.bag <> 0 THEN '放回背包内' ELSE '银行顶层槽' END AS place_kind
FROM (SELECT r.*, m.char_name, m.slot AS orig_slot
      FROM tmp_rec_plan r JOIN tmp_rec_map m ON m.item_guid = r.item_guid) p
LEFT JOIN item_instance ii             ON ii.guid = p.item_guid
LEFT JOIN acore_world.item_template it ON it.entry = ii.itemEntry
ORDER BY p.target_type, p.target_id, p.bag, p.slot;

-- 3.8 空间不足、没排上位置的物品（银行 39~66 只有 28 格 + 背包槽有限）
SELECT m.char_name, p.item_guid, ii.itemEntry, it.name,
       '未安排(目标银行空间不足，腾出位置后重跑第 3 段即可)' AS reason
FROM tmp_rec_pending p
JOIN tmp_rec_map m                     ON m.item_guid = p.item_guid
LEFT JOIN item_instance ii             ON ii.guid = p.item_guid
LEFT JOIN acore_world.item_template it ON it.entry = ii.itemEntry
WHERE NOT EXISTS (SELECT 1 FROM tmp_rec_plan x WHERE x.item_guid = p.item_guid);

-- =====================================================================================
-- 4. 落库（确认 3.7 无误后执行）
-- =====================================================================================
DROP TABLE IF EXISTS bk_rec_plan;
CREATE TABLE bk_rec_plan AS SELECT * FROM tmp_rec_plan;                 -- 本次写入的位置计划
DROP TABLE IF EXISTS bk_rec_owner;
CREATE TABLE bk_rec_owner AS                                            -- 变更前 owner_guid
SELECT ii.guid, ii.owner_guid FROM item_instance ii JOIN tmp_rec_plan p ON p.item_guid = ii.guid;

-- 4.1 账号银行目标：写 account_bank_item，owner_guid 置 0
INSERT INTO account_bank_item (account_id, bag, slot, item)
SELECT p.target_id, p.bag, p.slot, p.item_guid FROM tmp_rec_plan p WHERE p.target_type = 1;

UPDATE item_instance ii JOIN tmp_rec_plan p ON p.item_guid = ii.guid
SET ii.owner_guid = 0
WHERE p.target_type = 1;

-- 4.2 个人银行/角色库存目标：写 character_inventory，owner_guid 置为该角色
INSERT INTO character_inventory (guid, bag, slot, item)
SELECT p.target_id, p.bag, p.slot, p.item_guid FROM tmp_rec_plan p WHERE p.target_type = 2;

UPDATE item_instance ii JOIN tmp_rec_plan p ON p.item_guid = ii.guid
SET ii.owner_guid = p.target_id
WHERE p.target_type = 2;

-- =====================================================================================
-- 5. 验证与回滚
-- =====================================================================================
-- 5.1 验证：应与 3.7 完全一致
SELECT '账号银行' AS place, ab.account_id AS target_id, ab.bag, ab.slot, ab.item AS item_guid, ii.itemEntry
FROM account_bank_item ab JOIN item_instance ii ON ii.guid = ab.item
WHERE ab.item IN (SELECT item_guid FROM bk_rec_plan WHERE target_type = 1)
UNION ALL
SELECT '个人银行', ci.guid, ci.bag, ci.slot, ci.item, ii.itemEntry
FROM character_inventory ci JOIN item_instance ii ON ii.guid = ci.item
WHERE ci.item IN (SELECT item_guid FROM bk_rec_plan WHERE target_type = 2)
ORDER BY place, target_id, bag, slot;

-- 5.2 回滚（如需撤销，整段去掉注释执行）
-- DELETE ab FROM account_bank_item ab JOIN bk_rec_plan p ON p.item_guid = ab.item AND p.target_type = 1;
-- DELETE ci FROM character_inventory ci JOIN bk_rec_plan p ON p.item_guid = ci.item AND p.target_type = 2;
-- UPDATE item_instance ii JOIN bk_rec_owner o ON o.guid = ii.guid SET ii.owner_guid = o.owner_guid;

-- =====================================================================================
-- 6.（可选）存量脏数据：把“落在银行物品槽 39~66 的背包”挪回银行背包槽
--    动机：物品槽里的背包加载时不会被当作容器（IsBagPos 只认 19~22 / 67~73），
--          其包内物品会失去宿主。新版服务端已不再删除它们，但仍不可见。
--    注意：需要已购买且空闲的银行背包槽；本地库 77 例中仅 2 例可挪，
--          其余需先购买背包槽，或按 3.5 的思路把包内物品放到顶层物品槽。
-- =====================================================================================
-- 6.1 个人银行（预览，不写库）
DROP TEMPORARY TABLE IF EXISTS tmp_bag_move;
CREATE TEMPORARY TABLE tmp_bag_move AS
WITH free_slots AS (
    SELECT c.guid, s.s, ROW_NUMBER() OVER (PARTITION BY c.guid ORDER BY s.s) AS rn
    FROM characters c
    JOIN (SELECT 67 AS s UNION ALL SELECT 68 UNION ALL SELECT 69 UNION ALL SELECT 70
          UNION ALL SELECT 71 UNION ALL SELECT 72 UNION ALL SELECT 73) s
      ON (s.s - 67) < c.bankSlots
    WHERE NOT EXISTS (SELECT 1 FROM character_inventory x
                      WHERE x.guid = c.guid AND x.bag = 0 AND x.slot = s.s)
),
move_bags AS (
    SELECT ci.guid, ci.item, ROW_NUMBER() OVER (PARTITION BY ci.guid ORDER BY ci.slot) AS rn
    FROM character_inventory ci
    JOIN item_instance ii             ON ii.guid = ci.item
    JOIN acore_world.item_template it ON it.entry = ii.itemEntry AND it.class = 1
    WHERE ci.bag = 0 AND ci.slot BETWEEN 39 AND 66
)
SELECT b.guid, b.item, f.s AS new_slot
FROM move_bags b JOIN free_slots f ON f.guid = b.guid AND f.rn = b.rn;

SELECT COUNT(*) AS movable_bags FROM tmp_bag_move;      -- 可自动归位的背包数量
SELECT * FROM tmp_bag_move ORDER BY guid, new_slot;

-- 6.2 个人银行（确认后执行）
-- UPDATE character_inventory ci JOIN tmp_bag_move m ON m.item = ci.item SET ci.slot = m.new_slot;

-- 6.3 账号银行同类处理（把 account_bank_item 里 bag=0 且 slot 39~66 的容器挪到 67~73，
--     前提 account_bank_slots.slot_count 有富余）
-- SELECT abi.account_id, abi.slot AS cur_slot, abi.item, ii.itemEntry, it.name
-- FROM account_bank_item abi
-- JOIN item_instance ii             ON ii.guid = abi.item
-- JOIN acore_world.item_template it ON it.entry = ii.itemEntry AND it.class = 1
-- WHERE abi.bag = 0 AND abi.slot BETWEEN 39 AND 66;
