/* =====================================================================================
   账号银行 / 银行背包 —— 风险巡检 SQL（只读，不写任何数据）
   -------------------------------------------------------------------------------------
   用途：
     · 未部署修复版 worldserver 时，找出「下次加载就会被删掉」的脏数据（A/B/C）
     · 排查玩家报「物品不见了」（D）
     每段都先打印总览计数，再打印明细；计数为 0 就说明该项无风险。

   背景（老程序的删除路径，与物品是否“唯一”无关）：
     · 子物品的宿主背包找不到  -> 子物品映射被删除
     · 顶层物品/背包加载失败（位置非法、背包槽未购买）-> 该物品映射被删除

   运行：
     mysql -uroot -p --default-character-set=utf8mb4 < 账号银行巡检.sql
   （角色库/世界库名不同的话，改下面的 USE 与 SQL 里的 acore_world 前缀）

   修复语句见同目录：账号银行丢件恢复.sql
   ===================================================================================== */

USE acore_characters;

-- =====================================================================================
-- 1. 采集四类风险到临时表（只读）
-- =====================================================================================
-- A. 落在「银行物品槽 39~66」的容器：加载时不会被识别为容器，包内物品会失去宿主
DROP TEMPORARY TABLE IF EXISTS pat_a;
CREATE TEMPORARY TABLE pat_a AS
SELECT 'A-个人银行' AS risk, c.name AS char_name, NULL AS account_id, ci.slot AS slot,
       ci.item AS item_guid, ii.itemEntry, it.name AS item_name, it.ContainerSlots,
       (SELECT COUNT(*) FROM character_inventory x WHERE x.bag = ci.item) AS inner_items,
       '把背包挪到背包槽 67~73（需已购买有空位），否则包内物品会丢' AS action
FROM character_inventory ci
JOIN item_instance ii             ON ii.guid = ci.item
JOIN acore_world.item_template it ON it.entry = ii.itemEntry AND it.class = 1
JOIN characters c                 ON c.guid = ci.guid
WHERE ci.bag = 0 AND ci.slot BETWEEN 39 AND 66
UNION ALL
SELECT 'A-账号银行', NULL, abi.account_id, abi.slot, abi.item, ii.itemEntry, it.name, it.ContainerSlots,
       (SELECT COUNT(*) FROM account_bank_item x WHERE x.bag = abi.item),
       '把背包挪到 67~73（需 account_bank_slots.slot_count 有空位），否则包内物品会丢'
FROM account_bank_item abi
JOIN item_instance ii             ON ii.guid = abi.item
JOIN acore_world.item_template it ON it.entry = ii.itemEntry AND it.class = 1
WHERE abi.bag = 0 AND abi.slot BETWEEN 39 AND 66;

-- B. 子物品的宿主背包已不存在（Bag GUID 在对应表里没有行）-> 子物品映射会被删
DROP TEMPORARY TABLE IF EXISTS pat_b;
CREATE TEMPORARY TABLE pat_b AS
SELECT 'B-账号银行' AS risk, NULL AS char_name, c.account_id AS account_id,
       c.bag AS host_bag, c.slot AS inner_slot, c.item AS item_guid,
       ii.itemEntry, it.name AS item_name,
       bi.itemEntry AS host_bag_entry, bti.name AS host_bag_name, bi.owner_guid AS host_bag_owner,
       CASE WHEN bi.guid IS NULL
            THEN '宿主背包 item_instance 行已丢失：需从旧备份恢复背包，或把该物品改为顶层槽位'
            ELSE '宿主背包仍在库中（可能落在物品槽/已移出账号银行）：修正映射（放回包内或改顶层槽）'
       END AS action
FROM account_bank_item c
LEFT JOIN item_instance ii              ON ii.guid = c.item
LEFT JOIN acore_world.item_template it  ON it.entry = ii.itemEntry
LEFT JOIN item_instance bi              ON bi.guid = c.bag
LEFT JOIN acore_world.item_template bti ON bti.entry = bi.itemEntry
WHERE c.bag <> 0 AND NOT EXISTS (SELECT 1 FROM account_bank_item p WHERE p.item = c.bag)
UNION ALL
SELECT 'B-个人银行', ch.name, NULL,
       ci.bag, ci.slot, ci.item, ii.itemEntry, it.name,
       bi.itemEntry, bti.name, bi.owner_guid,
       CASE WHEN bi.guid IS NULL
            THEN '宿主背包 item_instance 行已丢失：需从旧备份恢复背包，或把该物品改为顶层槽位'
            ELSE '宿主背包不在角色库存里（可能已存进账号银行）：修正映射，否则登录时会被删'
       END
FROM character_inventory ci
JOIN characters ch                      ON ch.guid = ci.guid
LEFT JOIN item_instance ii              ON ii.guid = ci.item
LEFT JOIN acore_world.item_template it  ON it.entry = ii.itemEntry
LEFT JOIN item_instance bi              ON bi.guid = ci.bag
LEFT JOIN acore_world.item_template bti ON bti.entry = bi.itemEntry
WHERE ci.bag <> 0 AND NOT EXISTS (SELECT 1 FROM character_inventory p WHERE p.item = ci.bag);

-- C. 顶层背包槽（67~73）占用超出已购买数量 -> 加载时 MUST_PURCHASE_THAT_BAG_SLOT(34)，映射被删
DROP TEMPORARY TABLE IF EXISTS pat_c;
CREATE TEMPORARY TABLE pat_c AS
SELECT 'C-账号银行' AS risk, NULL AS char_name, abi.account_id AS account_id,
       abi.slot AS slot, abi.item AS item_guid, ii.itemEntry, it.name AS item_name,
       COALESCE(s.slot_count, 0) AS purchased_bag_slots,
       '补 account_bank_slots.slot_count，或把该物品挪到物品槽 39~66' AS action
FROM account_bank_item abi
JOIN item_instance ii                  ON ii.guid = abi.item
LEFT JOIN acore_world.item_template it ON it.entry = ii.itemEntry
LEFT JOIN account_bank_slots s         ON s.account_id = abi.account_id
WHERE abi.bag = 0 AND abi.slot BETWEEN 67 AND 73 AND (abi.slot - 67) >= COALESCE(s.slot_count, 0)
UNION ALL
SELECT 'C-个人银行', c.name, NULL,
       ci.slot, ci.item, ii.itemEntry, it.name, c.bankSlots,
       '让玩家购买背包槽，或把该物品挪到物品槽 39~66'
FROM character_inventory ci
JOIN characters c                      ON c.guid = ci.guid
JOIN item_instance ii                  ON ii.guid = ci.item
LEFT JOIN acore_world.item_template it ON it.entry = ii.itemEntry
WHERE ci.bag = 0 AND ci.slot BETWEEN 67 AND 73 AND (ci.slot - 67) >= c.bankSlots;

-- D. 已彻底没有位置映射、且归属角色仍然存在的物品（疑似历史丢件，可恢复）
--    （邮件/公会银行/拍卖/机器人装备与装备库/幻化引用等已排除，避免误报）
DROP TEMPORARY TABLE IF EXISTS pat_d;
CREATE TEMPORARY TABLE pat_d AS
SELECT ii.guid AS item_guid, ii.itemEntry, it.name AS item_name, ii.owner_guid,
       c.name AS owner_name, ii.count
FROM item_instance ii
JOIN characters c                      ON c.guid = ii.owner_guid
LEFT JOIN acore_world.item_template it ON it.entry = ii.itemEntry
LEFT JOIN character_inventory ci       ON ci.item = ii.guid
LEFT JOIN account_bank_item   ab       ON ab.item = ii.guid
LEFT JOIN mail_items          mi       ON mi.item_guid = ii.guid
LEFT JOIN guild_bank_item     gbi      ON gbi.item_guid = ii.guid
LEFT JOIN auctionhouse        ah       ON ah.itemguid = ii.guid
LEFT JOIN characters_npcbot_gear_storage gs ON gs.item_guid = ii.guid
LEFT JOIN character_gifts     cg       ON cg.item_guid = ii.guid
LEFT JOIN item_refund_instance ri      ON ri.item_guid = ii.guid
LEFT JOIN item_soulbound_trade_data st ON st.itemGuid = ii.guid
LEFT JOIN mod_weapon_visual_effect  wv ON wv.item_guid = ii.guid
WHERE ci.item IS NULL AND ab.item IS NULL AND mi.item_guid IS NULL
  AND gbi.item_guid IS NULL AND ah.itemguid IS NULL AND gs.item_guid IS NULL
  AND cg.item_guid IS NULL AND ri.item_guid IS NULL AND st.itemGuid IS NULL
  AND wv.item_guid IS NULL
  AND NOT EXISTS (SELECT 1 FROM characters_npcbot b
                  WHERE ii.guid IN (b.equipMhEx, b.equipOhEx, b.equipRhEx, b.equipHead, b.equipShoulders,
                                    b.equipChest, b.equipWaist, b.equipLegs, b.equipFeet, b.equipWrist,
                                    b.equipHands, b.equipBack, b.equipBody, b.equipFinger1, b.equipFinger2,
                                    b.equipTrinket1, b.equipTrinket2, b.equipNeck));

-- =====================================================================================
-- 2. 总览：一眼看风险规模（正常应为 0）
-- =====================================================================================
SELECT 'A 背包落在银行物品槽(会丢包内物品)' AS risk, COUNT(*) AS rows_cnt, '先修，参见 账号银行丢件恢复.sql 第6段' AS note FROM pat_a
UNION ALL
SELECT 'B 子物品宿主背包不存在(子物品会被删)', COUNT(*), '修映射：放回包内或改顶层槽' FROM pat_b
UNION ALL
SELECT 'C 背包槽超出已购买数量(物品会被删)', COUNT(*), '补背包槽或挪到物品槽' FROM pat_c
UNION ALL
SELECT 'D 无任何位置映射的疑似丢件', COUNT(*), '人工确认后用恢复脚本处理' FROM pat_d;

-- 归属为空 / 角色已删除的无映射物品（历史垃圾，通常无需处理，仅统计）
SELECT 'D2 无归属或角色已删的无映射物品(仅统计)' AS risk, COUNT(*) AS rows_cnt
FROM item_instance ii
LEFT JOIN characters c                 ON c.guid = ii.owner_guid
LEFT JOIN character_inventory ci       ON ci.item = ii.guid
LEFT JOIN account_bank_item   ab       ON ab.item = ii.guid
LEFT JOIN mail_items          mi       ON mi.item_guid = ii.guid
LEFT JOIN guild_bank_item     gbi      ON gbi.item_guid = ii.guid
LEFT JOIN auctionhouse        ah       ON ah.itemguid = ii.guid
WHERE c.guid IS NULL
  AND ci.item IS NULL AND ab.item IS NULL AND mi.item_guid IS NULL
  AND gbi.item_guid IS NULL AND ah.itemguid IS NULL;

-- =====================================================================================
-- 3. 明细
-- =====================================================================================
-- 3.1 A 类明细（背包内容为空时危害较小，但仍应归位）
SELECT * FROM pat_a ORDER BY risk, account_id, char_name, slot;

-- 3.2 B 类明细（重点：inner_slot 与宿主背包信息）
SELECT * FROM pat_b ORDER BY risk, account_id, char_name, host_bag, inner_slot;

-- 3.3 C 类明细
SELECT * FROM pat_c ORDER BY risk, account_id, char_name, slot;

-- 3.4 D 类明细（只列前 200 条；总数见总览）
SELECT * FROM pat_d ORDER BY owner_guid, item_guid LIMIT 200;
