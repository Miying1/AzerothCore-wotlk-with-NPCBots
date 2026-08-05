# 合并提交 `1df390c80` 数据库冲突审查报告

## 1. 审查对象与最终结论

- 当前分支：`merge_test2_from_npcbots`
- 目标合并提交：`1df390c80587e810884d8c59dd10566c95e21775`
- 提交说明：`Merge test2 custom features into npcbots_3.3.5 base`
- 第一父提交（NPCBots 3.3.5 新基线）：`5c882898a53adf41445a2a35a4357f3f09304e6e`
- 第二父提交（test2）：`c331574c1df26b9b871a55dd491fd4883edc3be3`
- 本次数据库审查范围：目标合并相对第一父提交带入或修改的 SQL，以及这些 SQL 与当前本地数据库、模块 C++ 代码、AzerothCore 更新器之间的兼容性。

### 最终判断

**本次合并带入的数据库内容存在多个已确认的严重冲突，不具备直接自动更新、部署或发布条件。**

即使先修复代码审查中已发现的编译错误，数据库层仍至少存在以下发布阻断问题：

1. 标准模块目录中的 **25 个 SQL 文件会被当前 C++ 更新器自动发现**；本地数据库没有相应 `MODULE` 更新历史，因而会被视为未执行的新更新。
2. `mod-zone-difficulty` 会在 `spell_custom_attr.100008` 处发生主键冲突，并留下部分应用状态；修正该错误后，后续还会删除正式物品 `item_template.62000`（当前为 `Five of Stones`）并替换为“挑战印记”。
3. `mod-raidleader-reawrd` 会删除本地正确表和数据，重建为缺少代码必需列 `is25raid` 的错误 schema。
4. `mod-ah-bot` 会删除并重建两张已有业务表，清除本地 3 行拍卖行配置和 4608 行禁用物品配置。
5. `mod-congrats-on-level` 会清空本地 7 行定制奖励并替换为脚本默认值。
6. `mod-individual-xp`、`mod-transmog` 使用当前 `command` 表不存在的旧列 `security`，会在前序删除或文本覆盖后失败。
7. `mod-transmog` 的 NPC SQL还使用当前 `creature_template` 不存在的旧列 `faction`、`rank`，其 `spell_dbc` 插入又对应完全不同的旧宽表结构。
8. `mod-zone-difficulty`、`mod-reward-shop`、`mod-weapon-visual` 均存在 SQL schema 与当前 C++ 代码不一致的问题；在已有库上可能被 `CREATE TABLE IF NOT EXISTS` 静默掩盖，在全新库上则会创建代码无法使用的表。
9. 更新器调用 MySQL CLI 时没有 `--force`；SQL 发生首个错误后停止，但前面的 DDL/DML 通常已经提交，且该 SQL 不会登记为成功更新。下次启动还会再次尝试，形成可重复的部分破坏。
10. 另有 5 个模块 SQL 位于非标准顶层 `sql/...`，当前 C++ 更新器无法自动发现；其中 `mod-skip-dk-starting-area` 的 legacy 路径也配置错误，数据库绑定可能根本未安装。

**结论：当前本地库不应直接运行启用了全部合并模块的 worldserver/dbimport 自动更新器。** 在完成备份、SQL 修正、显式迁移、更新历史规划，并在数据库克隆上验证之前，不应执行这些 SQL。

> 本次审查仅执行 `SELECT` 和 `information_schema` 查询；没有执行任何数据库写操作，也没有修改产品源码。

---

## 2. 审查方法、环境与边界

### 2.1 使用的只读方法

1. 对比合并提交与第一父提交，盘点新增和修改的 SQL 文件。
2. 静态提取 SQL 中的 `CREATE TABLE`、`DROP TABLE`、`ALTER TABLE`、`DELETE`、`INSERT`、`REPLACE` 及固定 ID。
3. 查询本地 MySQL 的实际表结构、主键、现有固定 ID、模块数据量和更新历史。
4. 对照模块 C++ 中实际执行的 `SELECT`、`INSERT`、`REPLACE`、`DELETE` 语句。
5. 阅读 `UpdateFetcher.cpp`、`DBUpdater.cpp` 和 dbimport 默认配置，确认模块 SQL 的发现、排序、执行和失败行为。
6. 区分以下两类结论：
   - **当前首次执行的实际可达错误**：按当前 SQL 顺序和当前本地 schema，更新会在哪里首先停止；
   - **前序错误修正后仍存在的潜伏破坏**：当前可能因更早错误暂未执行，但一旦修正或绕过前序错误就会发生。

### 2.2 数据库环境

- MySQL：`8.0.46`
- 连接用户：`root@localhost`
- 数据库：
  - `acore_auth`：17 张表
  - `acore_characters`：296 张表
  - `acore_world`：448 张表
- Core revision：`cc2dcf282c2c`
- DB version：`SFDB 548.Release.25.000`

### 2.3 限制

- 没有在数据库克隆中真实执行这些 SQL，因此未产生写入副作用。
- 本报告根据 SQL 顺序、MySQL DDL/DML 行为、现有 schema 和更新器代码推导实际失败点；结论足以判定发布阻断，但正式修复后仍应在克隆数据库上执行验证。
- 本地数据库显然包含历史手工导入或自定义数据库包内容，不能仅凭 `updates` 表完整还原其来源。

---

## 3. 合并带入的 SQL 总览

目标提交相对第一父提交涉及 **31 个 SQL 文件**：

- 修改 core archive SQL：1 个；
- 新增模块 SQL：30 个。

### 3.1 当前 C++ 更新器可自动发现：25 个

当前 `UpdateFetcher.cpp` 会针对已编译模块扫描：

```text
modules/<module>/data/sql/<目录名包含目标数据库名>/.../*.sql
```

自动发现的模块 SQL数量如下：

| 模块 | 自动发现 SQL 数 | 主要数据库 |
|---|---:|---|
| `mod-ah-bot` | 1 | world |
| `mod-congrats-on-level` | 2 | world |
| `mod-individual-xp` | 3 | characters/world |
| `mod-playerchallenge-modes` | 1 | world |
| `mod-raidleader-reawrd` | 1 | world |
| `mod-transmog` | 3 | characters/world |
| `mod-weapon-visual` | 2 | characters/world |
| `mod-zone-difficulty` | 12 | characters/world |
| **合计** | **25** |  |

### 3.2 当前 C++ 更新器不能自动发现：5 个

| 模块 | 文件 | 原因 |
|---|---|---|
| `mod-npc-gambler` | `modules/mod-npc-gambler/sql/world/npc_gambler.sql` | 不在 `data/sql` 下 |
| `mod-random-enchants` | `modules/mod-random-enchants/sql/world/npc.sql` | 不在 `data/sql` 下 |
| `mod-reward-shop` | `modules/mod-reward-shop/sql/chars-base/reward_shop.sql` | 不在 `data/sql` 下 |
| `mod-reward-shop` | `modules/mod-reward-shop/sql/world/npc.sql` | 不在 `data/sql` 下 |
| `mod-skip-dk-starting-area` | `modules/mod-skip-dk-starting-area/sql/world/Skip_DK_Script.sql` | 不在 `data/sql` 下，且 legacy 注册路径错误 |

这些 SQL 不会被当前 C++ updater 自动执行，但手工导入、外部 SQL assembler 或旧 Bash 流程仍可能触发其中的 schema 问题。

### 3.3 修改的 core archive SQL：1 个

- `data/sql/archive/db_world/2023_11_12_07.sql`

该文件已在本地 `updates` 中以 `RELEASED` 状态登记，数据库记录的 SHA1 与当前文件一致；默认 `Updates.ArchivedRedundancy = 0`，因此当前更新器不会重跑它。该修改本身不构成本次本地自动更新的直接阻断，但修改已归档 SQL 仍不利于历史可追溯性。

---

## 4. 更新器行为与为什么会发生部分应用

### 4.1 模块 SQL按已编译模块自动加入

`UpdateFetcher.cpp` 使用编译期模块列表，在每个模块的 `data/sql` 下寻找目录名包含 `world`、`characters` 或 `auth` 的目录，并将其中 SQL 统一标记为 `MODULE`。

因此，模块的 `conf.sh.dist` 或 `include.sh` 路径错误 **不会阻止标准 `data/sql/db-*` 文件被当前 C++ updater 发现**。这修正了仅根据 legacy Bash 配置判断 SQL 不会执行的片面结论。

### 4.2 本地没有此次模块 SQL的成功历史

本地三个数据库的 `updates` 表均未发现此次新增 SQL 的 `MODULE` 记录。更新器只按 SQL **文件名**匹配历史，而非完整路径。

结果是：只要对应模块被编译启用且 `Updates.AllowedModules` 允许，25 个标准模块 SQL会被视为新更新。

### 4.3 默认配置扩大了风险范围

`dbimport.conf.dist` 默认配置为：

```ini
Updates.EnableDatabases = 7
Updates.AllowedModules = "all"
Updates.Redundancy = 1
Updates.ArchivedRedundancy = 0
```

即默认启用全部三个数据库更新，并允许全部已编译模块 SQL。

### 4.4 首错停止，但不会回滚已执行语句

`DBUpdater.cpp` 调用外部 MySQL CLI，没有传入 `--force`。CLI 返回非零后，更新器抛出 `UpdateException` 并停止。

这意味着：

1. SQL 从文件开头顺序执行；
2. 遇到第一个错误即停止；
3. 之前执行的 `DROP`、`DELETE`、`CREATE`、`INSERT` 通常已经提交；
4. 该文件不会被写入成功的 `updates` 记录；
5. 下次启动会再次尝试同一文件；
6. 若 SQL 非幂等或前半段有删除操作，重复尝试可能进一步破坏状态。

这不是“失败即安全退出”，而是**可留下半迁移数据库的失败模式**。

---

## 5. 阻断级数据库冲突

## B-DB-01 `mod-zone-difficulty` 固定 spell ID 首次执行即冲突

- 文件：`modules/mod-zone-difficulty/data/sql/db-world/zone_difficulty_mythicmode_creatures.sql`
- 严重度：**阻断**

脚本包含：

```sql
DELETE FROM spell_custom_attr WHERE spell_id = 100007;
INSERT INTO spell_custom_attr VALUES (100008, 4194304);
```

本地 `spell_custom_attr.100008` 已存在，而脚本删除的是 `100007`。因此当前本地库首次执行的可达顺序为：

1. 删除并创建 `creature_template.61000`；
2. 写入相应 NPC 文本；
3. 删除不存在或无关的 `spell_custom_attr.100007`；
4. 插入 `100008` 时发生主键冲突；
5. MySQL CLI 停止；
6. 文件不登记为成功更新。

**影响：** 数据库已经写入前半段 NPC 数据，但模块更新失败；下次启动会再次尝试。

同一文件还存在：

```sql
DELETE FROM spell_custom_attr WHERE spell_id = 100009;
INSERT INTO spell_custom_attr VALUES (1000010, 4194304);
```

`1000010` 极可能是将 `100010` 多写了一个零。当前库中 `100010` 已存在，而 `1000010` 是不同 ID。该错误会造成逻辑绑定漂移，即使不触发主键冲突也可能无法作用于预期法术。

## B-DB-02 修正 spell 错误后会覆盖正式物品 `62000`

- 同一文件：`zone_difficulty_mythicmode_creatures.sql`
- 严重度：**阻断 / 数据破坏**

脚本后部包含：

```sql
DELETE FROM item_template WHERE entry = 62000;
INSERT INTO item_template (...) VALUES (62000, ..., '挑战印记', ...);
```

本地当前 `item_template.62000` 为：

| 字段 | 当前值 |
|---|---|
| `name` | `Five of Stones` |
| `class` | `12` |
| `displayid` | `81250` |
| `Quality` | `3` |
| `ItemLevel` | `85` |

当前首次执行会先在 `spell_custom_attr.100008` 处停止，因此暂时到不了物品覆盖位置。但这不等于安全：一旦修正、删除、忽略或绕过前面的 spell 冲突，脚本会删除该正式物品并替换为“挑战印记”。

**必须为自定义物品分配不冲突的新 ID，并迁移所有代码、掉落、奖励和文本引用，不能直接执行现有 SQL。**

## B-DB-03 `zone_difficulty_instance_saves` 存在三套互不兼容模型

- SQL：`modules/mod-zone-difficulty/data/sql/db-characters/zone_difficulty_char_tables.sql`
- 代码：
  - `modules/mod-zone-difficulty/src/ChallengeDifficulty.cpp`
  - `modules/mod-zone-difficulty/src/mod_zone_difficulty_handler.cpp`
- 严重度：**阻断**

合并 SQL 创建三列表：

```text
InstanceID
MythicmodeOn
MythicmodePossible
```

本地实际表是十列表：

```text
InstanceID, level, enhance_damage, enhance_hp, residue_time,
kill_boss, spell_id1, spell_id2, spell_id3, is_complete
```

当前活跃挑战代码 `ChallengeDifficulty.cpp` 明确查询和写入十列模型，例如：

```sql
SELECT InstanceID, level, enhance_damage, enhance_hp, kill_boss,
       spell_id1, spell_id2, spell_id3, is_complete
FROM zone_difficulty_instance_saves
```

模块中另一个旧 handler 又使用三列语义：

```sql
SELECT * FROM zone_difficulty_instance_saves
REPLACE INTO zone_difficulty_instance_saves (InstanceID, MythicmodeOn) ...
```

由此形成：

1. 合并 SQL 的三列模型；
2. 当前本地库和 `ChallengeDifficulty.cpp` 的十列模型；
3. 旧 handler 的 `MythicmodeOn` 模型。

由于 SQL 使用 `CREATE TABLE IF NOT EXISTS`：

- 在当前本地库上：不会报错，也不会修正 schema，错误被静默掩盖；
- 在全新数据库上：会创建三列表，`ChallengeDifficulty.cpp` 启动查询立即失败；
- 两套 C++ 实现共存也说明不同版本代码被混合。

## B-DB-04 zone difficulty 缺少当前挑战代码依赖的多张表

当前 `ChallengeDifficulty.cpp` 依赖但本次合并的 12 个 zone difficulty SQL 中未创建的表包括：

- `zone_diffculty_activemap`
- `zone_diffculty_playerlevel`
- `zone_difficulty_level`
- `zone_difficulty_mapbase`
- `zone_difficulty_spell_group`
- `zone_difficulty_spells`

这些表当前仅因本地数据库已有定制内容而可用。对全新数据库或标准 CI 数据库而言，模块 SQL不完整，不能独立部署。

## B-DB-05 `mod-raidleader-reawrd` 会删除正确表并创建错误 schema

- SQL：`modules/mod-raidleader-reawrd/data/sql/db-world/mod_raidleader_reawrd.sql`
- 严重度：**阻断 / 数据破坏**

脚本执行：

```sql
DROP TABLE IF EXISTS mod_raidleader_reawrd;
CREATE TABLE mod_raidleader_reawrd (...);
```

脚本新表缺少 `is25raid`，而模块 C++ 明确查询：

```sql
SELECT bossid, is25raid, player_count1, reawrd1, player_count2, reawrd2
FROM mod_raidleader_reawrd
WHERE active = 1
```

本地正确表：

- 包含 `is25raid`；
- 主键为 `(bossid, is25raid)`；
- 已有 4 行配置；
- 同一个 `bossid = 36627` 同时存在 10 人和 25 人记录。

执行脚本将：

1. 删除全部 4 行配置；
2. 删除 `is25raid`；
3. 将复合主键退化为单列 `bossid`；
4. 使模块启动查询报 `Unknown column 'is25raid'`。

## B-DB-06 `mod-ah-bot` 会清除本地拍卖行定制

- 文件：`modules/mod-ah-bot/data/sql/db-world/base/mod_auctionhousebot.sql`
- 严重度：**阻断 / 大规模数据覆盖**

脚本对以下表执行 `DROP TABLE IF EXISTS` 后重建：

- `mod_auctionhousebot`
- `mod_auctionhousebot_disabled_items`

本地现状：

- `mod_auctionhousebot`：3 行；
- `mod_auctionhousebot_disabled_items`：4608 行。

本地配置与脚本默认值不同：

- Alliance/Horde 本地 `minitems/maxitems = 0/0`，脚本会改为 `250/250`；
- Neutral 本地 `2200/2500`，脚本会改为 `250/250`；
- 本地禁用物品最大 ID 为 91010，脚本默认列表最大值约 56806，说明本地有额外定制。

该文件位于标准 `data/sql/db-world` 且无更新历史；模块启用后会被自动执行并清空本地定制。

## B-DB-07 `mod-transmog` 的 world SQL 与当前 world schema 不兼容

### 7.1 NPC SQL 首个 INSERT 即失败

- 文件：`modules/mod-transmog/data/sql/db-world/trasm_world_NPC.sql`
- 严重度：**阻断 / 部分应用**

脚本先执行：

```sql
DELETE FROM creature_template WHERE entry = 190010;
INSERT INTO creature_template (..., faction, ..., rank, ...) VALUES (...);
```

当前 `creature_template` 使用：

- `faction_A`、`faction_H`，不存在 `faction`；
- `npc_rank`，不存在 `rank`。

因此当前首次自动执行会：

1. 删除 `creature_template.190010`；
2. 在随后的 INSERT 处报未知列；
3. 停止，无法创建 190010、190011 和法术 2000100；
4. SQL 不登记为成功更新。

当前本地 190010 不存在，因此这次只读检查未发现现有 NPC 被删除的现实损失；但 SQL 的执行模型仍是危险的“先删后失败”。

### 7.2 `spell_dbc` 插入对应旧宽表结构

脚本后部试图向包含数百列的旧 `spell_dbc` 宽表写入 `Category`、`DispelType`、`Mechanic`、`Effect_*`、多语言名称等字段。

当前本地 `spell_dbc` 只有 25 列，采用拆分后的引用模型，主要包括：

```text
Id, Attributes, AttributesEx...AttributesEx10,
CastingTimeIndex, DurationIndex, RangeIndex, SchoolMask,
SpellAuraOptionsId, SpellCastingRequirementsId, SpellCategoriesId,
SpellClassOptionsId, SpellEquippedItemsId, SpellLevelsId,
SpellTargetRestrictionsId, SpellInterruptsId, Comment
```

因此，即使先修复 `creature_template` 的旧列名，后续 `spell_dbc` INSERT 仍会因大量不存在的列而失败。当前 `spell_dbc.Id = 2000100` 不存在。

### 7.3 文本 SQL使用旧 `command.security`

- 文件：`modules/mod-transmog/data/sql/db-world/trasm_world_texts.sql`

当前 `command` 表列为：

```text
name, permission, help
```

脚本使用：

```sql
INSERT INTO command (name, security, help) ...
```

执行顺序是先覆盖 NPC 文本和 `acore_string.11100–11116`，再删除 transmog 命令，最后在插入命令时失败。结果可能是：

- 文本已被覆盖；
- 原 transmog 命令已删除；
- 新命令未创建；
- SQL 未登记成功；
- 下次启动再次执行。

## B-DB-08 `mod-individual-xp` 命令 SQL先删除后失败

- 文件：`modules/mod-individual-xp/data/sql/db-world/base/mod_individual_xp_command.sql`
- 严重度：**阻断 / 部分应用**

该文件同样使用旧列：

```sql
INSERT INTO command (name, security, help) ...
```

当前 schema 使用 `permission`。脚本会先删除 `.xp` 相关命令，再在 INSERT 处失败，留下命令缺失的部分状态。

## B-DB-09 `mod-weapon-visual` SQL在全新库创建代码无法使用的表

- SQL：`modules/mod-weapon-visual/data/sql/db-characters/base/mod_weapon_visual_effect.sql`
- C++：`modules/mod-weapon-visual/src/VisualWeapon.cpp`
- 严重度：**阻断（全新库）/ 静默漂移（已有库）**

SQL 只创建：

```text
item_guid
enchant_visual_id
```

但 C++ 明确执行：

```sql
REPLACE INTO mod_weapon_visual_effect
(item_guid, enchant_visual_id, owner_guid) VALUES (...)

SELECT owner_guid, item_guid, enchant_visual_id
FROM mod_weapon_visual_effect
```

本地实际表有第三列 `owner_guid`，所以 `CREATE TABLE IF NOT EXISTS` 在当前库上不会报错。但在全新数据库上，SQL 会创建缺少 `owner_guid` 的两列表，模块首次写入或启动查询即失败。

## B-DB-10 `mod-reward-shop` 的 SQL、本地表和 C++ 三者不一致

- SQL：`modules/mod-reward-shop/sql/chars-base/reward_shop.sql`
- C++：`modules/mod-reward-shop/src/reward_shop.cpp`
- 严重度：**阻断（手工导入或全新库）**

SQL 期望九列模型，包括：

```text
id, action, action_data, quantity, code, status,
PlayerGUID, PlayerIP, CreatedBy
```

本地实际八列模型包括：

```text
code, action, action_data, quantity, isonly,
status, PlayerGUID, CreatedBy
```

模块 C++ 明确依赖 `isonly`：

```sql
SELECT action, action_data, quantity, status, isonly
FROM reward_shop WHERE code = ...
```

因此：

- 当前已有库上，`CREATE TABLE IF NOT EXISTS` 不迁移 schema；随后无列名九值 INSERT 会因列数不匹配失败；
- 全新库按 SQL 创建后没有 `isonly`，模块 C++ 运行失败；
- SQL 位于非标准顶层 `sql/chars-base`，当前 C++ updater 不会自动发现，但手工导入仍不安全。

---

## 6. 高风险数据覆盖与部署缺口

## H-DB-01 `mod-congrats-on-level` 会覆盖本地定制奖励

- 文件：`modules/mod-congrats-on-level/data/sql/db-world/col_reward_items.sql`
- 严重度：**高**

脚本执行：

```sql
CREATE TABLE IF NOT EXISTS mod_congrats_on_level_items (...);
DELETE FROM mod_congrats_on_level_items;
INSERT INTO mod_congrats_on_level_items ...;
```

本地已有 7 行定制数据，脚本包含 8 行默认数据且内容明显不同。模块启用后自动更新会清空本地配置并替换为脚本默认值。

## H-DB-02 `acore_string` 固定区间会覆盖本地文本

涉及：

- `mod-congrats-on-level`：`acore_string.60000`
- `mod-individual-xp`：`acore_string.35411–35420`
- `mod-transmog`：`acore_string.11100–11116`

其中 individual XP 与 transmog 区间本地已存在，内容基本匹配模块默认值，但 SQL 使用 `DELETE + INSERT`，仍会覆盖本地翻译和定制。更新历史中无对应 `MODULE` 成功记录，因此不能依赖 updater 跳过。

## H-DB-03 zone difficulty 多张 world 表采用破坏性重建

以下 SQL包含 `DROP TABLE IF EXISTS`：

- `zone_difficulty_disallowed_buffs.sql`
- `zone_difficulty_info.sql`
- `zone_difficulty_mythicmode_ai.sql`
- `zone_difficulty_mythicmode_creatureoverrides.sql`
- `zone_difficulty_mythicmode_instance_data.sql`
- `zone_difficulty_mythicmode_rewards.sql`
- `zone_difficulty_spelloverrides.sql`

其中 `zone_difficulty_mythicmode_ai` 当前本地表为空且结构基本一致，当前执行未必立即丢失业务数据；但整体模式仍属于 base SQL 直接重建表，不适合无条件作为已有数据库的自动增量更新。

## H-DB-04 顶层旧 NPC SQL使用过期 `creature_template` 列

以下非自动发现 SQL均使用当前表不存在的 `faction` 和 `rank`：

- `modules/mod-npc-gambler/sql/world/npc_gambler.sql`
- `modules/mod-random-enchants/sql/world/npc.sql`
- `modules/mod-reward-shop/sql/world/npc.sql`

它们不会被当前 C++ updater 自动执行，但手工导入时会在先删除固定 NPC 后，因未知列而失败。

当前固定 NPC ID 601020、93000、92000 均未占用，所以本地当前没有现有 NPC 被覆盖；这只说明固定 ID 暂无冲突，不代表 SQL schema 兼容。

## H-DB-05 `mod-skip-dk-starting-area` SQL无法被当前可见机制自动安装

- 实际文件：`modules/mod-skip-dk-starting-area/sql/world/Skip_DK_Script.sql`
- legacy 配置注册：`sql/db-world/`

文件既不在当前 C++ updater 扫描的 `data/sql` 下，legacy 配置路径又与实际目录不一致。结果是跳过死亡骑士出生区所需数据库绑定可能根本没有安装。

## H-DB-06 `mod-raidleader-reawrd/include.sh` 引用不存在的配置文件

该模块 `include.sh` 无条件 source：

```bash
source $MOD_SKELETON_ROOT"/conf/conf.sh.dist"
```

但模块不存在对应文件。此问题主要影响 legacy Bash 工具链；标准 `data/sql` 仍会被 C++ updater 自动发现。

---

## 7. 当前未发现固定 ID 冲突或结构基本兼容的项目

以下项目当前未发现直接固定 ID 占用，但仍应在修复后的克隆库验证：

### 7.1 `mod-playerchallenge-modes`

- `gameobject_template.entry = 254605`：当前未占用；
- `gameobject.guid = 5530536–5530544`：当前未占用；
- 文件位于标准 `data/sql/db-world/base`，模块启用后可自动发现。

### 7.2 `mod-random-enchants` 固定物品和 NPC

- NPC `93000`：当前未占用；
- `npc_text.60001–60007`：当前未占用；
- `item_template.60100`、`60101`：当前未占用。

但该 SQL 不被 C++ updater 自动发现，且 NPC INSERT 使用旧列名，因此不能直接手工导入。

### 7.3 `mod-npc-gambler`

- NPC/NPC 文本 ID `601020`：当前未占用；
- 但 SQL 路径和旧 `creature_template` 列仍有问题。

### 7.4 `mod-transmog` characters 表

本地以下三张表已存在，结构与合并 SQL基本一致：

- `custom_transmogrification`
- `custom_transmogrification_sets`
- `custom_unlocked_appearances`

这不消除其 world SQL 的确定性错误。

### 7.5 archive SQL

`2023_11_12_07.sql` 的当前文件哈希与本地 `updates` 记录一致，默认不会重跑。未发现足以单独证明由本次合并造成的当前数据冲突。

---

## 8. 典型部分应用场景

## 场景 A：zone difficulty world SQL

按当前本地状态：

1. 创建/覆盖 NPC `61000` 和文本；
2. 删除 `spell_custom_attr.100007`；
3. 插入已存在的 `100008`；
4. 主键冲突，更新停止；
5. NPC 前半段保留；
6. 文件无成功记录；
7. 下次启动再次尝试；
8. 若修正 spell 错误，后续将覆盖物品 `62000`。

## 场景 B：individual XP 命令 SQL

1. 删除旧 `.xp` 命令；
2. 使用不存在的 `security` 列插入；
3. INSERT 失败；
4. 命令保持已删除状态；
5. SQL 无成功记录。

## 场景 C：transmog NPC SQL

1. 删除 `creature_template.190010`；
2. 使用不存在的 `faction`、`rank` 列插入；
3. INSERT 失败；
4. 第二个 NPC和法术不再执行；
5. 修复 NPC 列后，旧宽表 `spell_dbc` INSERT 仍会失败。

## 场景 D：transmog 文本/命令 SQL

1. 删除并重插 NPC 文本、locale 和 `acore_string`；
2. 删除 transmog 命令；
3. 使用不存在的 `command.security` 插入；
4. INSERT 失败；
5. 文本已覆盖，命令却缺失。

## 场景 E：raid leader reward

1. `DROP TABLE` 删除正确表和 4 行数据；
2. 创建缺少 `is25raid` 的错误表；
3. SQL本身可能成功并写入更新历史；
4. worldserver 模块启动查询才失败。

该场景比“SQL执行时报错”更危险，因为 updater 可能认为更新成功。

---

## 9. 冲突根因分析

此次数据库问题不是单一 ID 碰撞，而是多种版本错配叠加：

1. **base SQL 被作为增量更新执行**：大量 `DROP TABLE`、全表 `DELETE`、固定 ID `DELETE + INSERT` 不适合直接用于已有业务库。
2. **模块代码和 SQL来自不同版本**：zone difficulty、reward shop、weapon visual、raid leader reward 均有明确证据。
3. **数据库 core schema 与模块 SQL年代不一致**：`command.security`、`creature_template.faction/rank`、旧宽表 `spell_dbc` 均属于旧 schema。
4. **本地数据库已手工部署模块，但 `updates` 无 MODULE 历史**：更新器无法知道这些表和数据已存在，会把 SQL 当作首次安装。
5. **`CREATE TABLE IF NOT EXISTS` 掩盖不兼容**：表存在时 SQL“成功”，但没有完成必要迁移；表不存在时又可能创建错误结构。
6. **SQL发现机制不统一**：一部分模块放在标准 `data/sql` 自动执行，另一部分仍依赖未接入或路径错误的 legacy 机制。
7. **更新文件只按文件名登记**：未来若不同模块出现同名 SQL，还可能触发重复文件名致命错误或历史误匹配。

---

## 10. 发布前必须完成的修复建议

本报告不执行修复，建议按以下顺序处理。

### 10.1 立即阻断自动更新

在完成修复前，不要用当前代码对生产或唯一开发数据库运行启用了全部模块的 worldserver/dbimport。可在隔离配置中临时限制模块更新范围，但这只能避免执行，不能替代 SQL 修复。

### 10.2 先建立可恢复基线

1. 对 `acore_auth`、`acore_characters`、`acore_world` 做一致性备份；
2. 创建数据库克隆；
3. 在克隆上记录所有模块表的 schema、行数和关键配置校验值；
4. 禁止直接在唯一数据库上试错。

### 10.3 将 base SQL 与 migration SQL 分离

每个模块应至少区分：

- 全新安装 base SQL；
- 已有 test2 数据库升级 migration SQL；
- 可重复运行的幂等内容更新。

不得继续用 `DROP TABLE` 重建已有业务表作为普通 MODULE 自动更新。

### 10.4 修正明确的固定 ID 和拼写错误

至少包括：

- 为“挑战印记”重新分配不与 `62000` 冲突的自定义 item ID；
- 核对 `100008`、`100010`、`100014` 的真实目标法术；
- 修正疑似错误 ID `1000010`；
- 更新所有 C++、奖励表、掉落表和文本引用。

### 10.5 为 schema 漂移编写显式迁移

优先处理：

1. `zone_difficulty_instance_saves`：确定唯一权威模型，删除或隔离旧 handler，编写从现有十列表到目标模型的迁移；
2. `mod_raidleader_reawrd`：保留 `is25raid` 和复合主键，迁移而不是 DROP；
3. `reward_shop`：统一 SQL、本地表和 C++ 的 `isonly`、`PlayerIP`、主键设计；
4. `mod_weapon_visual_effect`：base SQL补充代码必需的 `owner_guid`；
5. transmog：重写 `creature_template`、`spell_dbc` 和 `command` SQL以适配当前 core schema；
6. individual XP：使用当前 `permission` 模型。

### 10.6 统一模块 SQL布局

- 将需要 C++ updater 自动管理的文件放入标准 `modules/<module>/data/sql/db-*`；
- 或明确声明必须手工安装，并提供可靠、版本化的安装流程；
- 修正 `mod-skip-dk-starting-area` 和 `mod-raidleader-reawrd` 的 legacy 配置问题；
- 避免同名 SQL 文件跨模块冲突。

### 10.7 规划更新历史

由于本地已有大量模块数据但没有 `MODULE` 历史，不能简单手工向 `updates` 插入“已执行”记录来跳过全部文件。应先逐文件判断：

- 已完整等价应用；
- 部分应用；
- schema 不同但业务数据有效；
- 尚未应用；
- 不应再执行。

只有在克隆库验证并计算正确哈希后，才能设计更新历史迁移方案。

---

## 11. 修复后验证清单

### 11.1 静态验证

- 所有 SQL列名均存在于目标数据库版本；
- INSERT 列数与值数一致；
- 固定 ID 不占用正式数据；
- SQL 与 C++ 查询列完全一致；
- 全新安装和已有库升级使用不同路径；
- 所有自动发现 SQL文件名全局唯一。

### 11.2 克隆数据库执行验证

1. 从当前三库创建克隆；
2. 用与 worldserver 相同配置运行 updater；
3. 要求所有 SQL成功且只执行一次；
4. 再运行第二次，要求零变更、零错误；
5. 比较更新前后表结构、行数和关键数据；
6. 验证 `updates` 中 MODULE 记录、哈希和状态；
7. 故意中断一个测试 SQL，确认恢复流程可用。

### 11.3 启动验证

至少检查日志中不存在：

- unknown column/table；
- duplicate primary key；
- module SQL failed；
- reward shop、zone difficulty、weapon visual、raid leader reward 加载失败；
- command 注册缺失；
- spell 或 creature template 加载错误。

### 11.4 游戏内回归

- 挑战模式开启、保存、重载、完成和奖励；
- 10/25 人团长奖励区分；
- 拍卖行机器人三阵营配置及禁用列表；
- 等级奖励是否保持本地定制；
- transmog NPC、便携 NPC、命令和召唤法术；
- individual XP 命令；
- weapon visual 跨重启加载及 owner 隔离；
- reward shop 兑换、一次性限制和状态更新；
- DK 出生区跳过脚本是否实际绑定。

---

## 12. 最终结论

- **本次合并的数据库更新是否存在冲突：是，而且有多个确定性严重冲突。**
- **当前本地数据库能否直接运行全部模块自动更新：不能。**
- **是否只存在“可能风险”：否。** 已确认主键冲突、旧列名错误、SQL/C++ schema 错配、确定性数据覆盖和 SQL发现缺口。
- **是否会影响原 test2 功能：会。** 挑战模式、团长奖励、拍卖行机器人、等级奖励、幻化、individual XP、武器视觉、兑换商店和 DK 跳过出生区均可能受到影响。
- **更新失败是否会自动回滚：不会。** 当前执行模型会保留首错之前的已提交修改，并在下次启动重复尝试。
- **是否建议发布：不建议。** 代码和数据库两层均未达到可编译、可迁移、可发布状态。

综合代码审查报告与本报告，`1df390c80` 不应作为生产或正式测试环境的可用合并基线。应先修复代码编译阻断，再完成数据库 migration 设计，并在克隆数据库和游戏内进行完整回归。
