# 饰品特效转附魔卷轴（Trinket Enchant Scrolls）

把饰品（`InventoryType = 12`）上的 **proc 特效**提取出来做成**附魔卷轴**：
右键使用卷轴 → 弹出附魔窗口 → 选择一把武器 → 把该特效**永久附魔到武器 0 槽位**。

另外提供一个**道具「饰品特效提取器」**（60105，消耗品）：右键使用 → 列出背包中带可提取特效的饰品 →
选中一件 → 自动生成对应的附魔卷轴并销毁该饰品，同时消耗提取器本身。

## 目录结构

```
outputs/饰品武器特效转附魔/
├── generate.py                             # 生成脚本（唯一入口，读库 + Spell.csv）
├── README.md                               # 本文件
├── 可用饰品特效清单.csv                     # 生成物：特效清单（人工核对用）
├── dbc/                                    # 客户端 DBC CSV（WDBX 导入用）
│   ├── TrinketEnchant_SpellItemEnchantment.dbc.csv   # 生成物
│   ├── TrinketEnchant_Spell.dbc.csv                  # 生成物
│   ├── TrinketEnchant_Item.dbc.csv                   # 生成物：附魔卷轴 63200~63277
│   └── RandomEnchants_Items_Item.dbc.csv             # 手写：本模块道具 60103/60104/60105
└── 饰品武器特效转附魔方案.md                # 需求/技术方案

modules/mod-random-enchants/
├── src/TrinketEssence.cpp                  # 生成物之外的运行时脚本：道具「饰品特效提取器」
├── src/EnchantTransfer.cpp                 # 参考实现：属性提取器 / 属性卷轴
├── sql/world/trinket_enchant_scrolls.sql   # 生成物：卷轴 + proc 数据 + 映射表（DBC 数据不写库）
└── sql/world/trinket_essence.sql           # 道具「饰品特效提取器」本体
```

## 数据来源与筛选范围

| 项 | 取值 |
| --- | --- |
| 物品来源 | 本地库 `acore_world.item_template` |
| 筛选条件 | `ItemLevel >= 200` 且 `InventoryType = 12`（饰品栏） |
| 运行时过滤 | 与生成口径一致：先按 `ItemLevel >= 200`（`MIN_TRINKET_ITEM_LEVEL`）过滤，再查映射表；低于装等的饰品直接跳过（见 `src/TrinketEssence.cpp`） |
| 特效类型 | `spelltrigger_1..5 = 1`（装备被动）或 `2`（命中触发） |
| 真 proc 判定 | `Spell.csv` 中该法术 `ProcTypeMask > 0`；`spelltrigger = 2` 由物品/附魔的 PPM 机制驱动，不受此约束 |
| 排除 | `ProcTypeMask = 0` 的纯被动属性光环；`Spell.csv` 中找不到定义的法术 |
| 法术定义 | `<仓库根>/outputs/Spell.csv`（中文放在 `Name_Lang_deDE` 列） |

被排除的特效（纯属性光环等）由 `generate.py` 在运行时逐条打印，不单独落盘；
需要留档的话把输出重定向到文件即可：`python generate.py > 生成日志.txt`。

## 数据链路

```
饰品 spelltrigger=1 (装备被动 proc 光环)
   └─ SpellItemEnchantment (Effect=3 EQUIP_SPELL, EffectArg=原法术 ID)
        └─ Spell (Effect=53 ENCHANT_ITEM, EffectMiscValue=附魔 ID)
             └─ item_template 卷轴 (class=0/subclass=8, spellid_1=法术, spelltrigger_1=0)
```

- **EQUIP_SPELL**（`Effect=3`）：装备武器时由 `Player::ApplyItemEquipSpell` 把 `EffectArg` 指向的原 proc 光环施加给玩家，触发逻辑复用原法术，**不需要** `spell_enchant_proc_data`。
- **COMBAT_SPELL**（`Effect=1`）：命中时由 `Player::CastItemCombatSpell` 按 `spell_enchant_proc_data` 的 PPM 触发。本批（饰品）没有命中触发特效，故该表为空。
- **0 槽位**：`Spell::EffectEnchantItemPerm` 把 `EffectMiscValue` 指向的附魔写入 `Item::PERM_ENCHANTMENT_SLOT`。

运行时（`TrinketEssence.cpp`）：

```
使用「饰品特效提取器」→ 扫描背包中的 InventoryType=12 饰品
   → 先按装等过滤：ItemLevel < 200 直接跳过（连法术槽都不扫）
   → 通过装等后再逐个读 item_template.spellid_1..5
   → 用 mod_trinket_essence_map（对照字典）判定能否提取：
        法术 ID 在字典里 = 可提取（会出现在菜单里）；不在 = 不可提取（不显示）
   → 只取第一个命中的特效，生成 1 张对应附魔卷轴
   → 成功后销毁原饰品（并按 CONSUME_ESSENCE 决定是否消耗提取器）
```

## ID 分配

| 对象 | 起始 ID | 区块 | 表 |
| --- | ---: | ---: | --- |
| 附魔 | 30000 | 30000 ~ 30099 | `spellitemenchantment_dbc.ID` |
| 附魔法术 | 94500 | 94500 ~ 94599 | `spell_dbc.ID` |
| 附魔卷轴 | 63200 | 63200 ~ 63299 | `item_template.entry` |
| proc 数据 | = 附魔 ID | — | `spell_enchant_proc_data.entry` |
| 饰品特效提取器 | 60105 | — | `item_template.entry` |

附魔/法术/卷轴每类预留 100 个 ID，当前占用 78 个（`30000~30077` / `94500~94577` / `63200~63277`）。

## 运行时映射表 `mod_trinket_essence_map`

由生成脚本重建，`src/TrinketEssence.cpp` 在世界启动时（`WorldScript::OnStartup`）一次性读进内存。
运行时只 `SELECT spell_id, scroll_item, name` 三列，其余列仅供人工核对、不参与逻辑：

| 字段 | 含义 |
| --- | --- |
| `spell_id` | 饰品上的原特效法术 ID（主键） |
| `enchant_id` | `spellitemenchantment_dbc.ID` |
| `enchant_spell_id` | 附魔法术 `spell_dbc.ID` |
| `scroll_item` | 附魔卷轴 `item_template.entry`（生成给玩家的物品） |
| `name` | 附魔显示名（紫色 `\|cffa335ee魄·…\|r`，与附魔窗口一致） |
| `src_item` / `src_spell_name` | 来源饰品 entry / 原特效法术名（便于人工核对） |

## 道具（提取器 / 卷轴）的 Item.dbc 行

`dbc/RandomEnchants_Items_Item.dbc.csv` 是**手写维护**的文件（不由 `generate.py` 产出），
用于把本模块的自定义道具补进客户端 `Item.dbc`：

| ID | 名称 | ClassID | SubclassID | Sound_Override_Subclassid | Material | DisplayInfoID | InventoryType | SheatheType |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 60103 | 属性提取器 | 0 | 0 | -1 | 0 | 40557 | 0 | 0 |
| 60104 | 属性卷轴 | 0 | 0 | -1 | 0 | 11161 | 0 | 0 |
| 60105 | 饰品特效提取器 | 0 | 0 | -1 | 0 | 48007 | 0 | 0 |

取值依据 `modules/mod-random-enchants/sql/world/` 下的 `item_template` 定义
（`enchant_transfer.sql` / `trinket_essence.sql`）；SQL 里未显式给出的列用表默认值：
`SoundOverrideSubclass = -1`、`Material = 0`、`InventoryType = 0`、`sheath = 0`。

> 改了这些道具的 `class`/`subclass`/`displayid`/`InventoryType` 后，**必须同步改本 CSV 并重新导入**
> 客户端 `Item.dbc`，否则客户端看到的还是旧图标/旧类型。

## 关键字段口径

### `spellitemenchantment_dbc`

| 字段 | 取值 |
| --- | --- |
| `Effect_1` | `3` EQUIP_SPELL（装备被动）/ `1` COMBAT_SPELL（命中触发） |
| `EffectArg_1` | 原 proc 法术 ID |
| `EffectPointsMin/Max_*` | 0（spell 类效果不需要点数） |
| `Name_Lang_deDE` | 紫色 + `魄·` + 饰品名，如 `\|cffa335ee魄·真实之镜\|r` |
| `Name_Lang_zhCN` | 同上（服务端按 `DBC.Locale` 取列，两列都写保险） |
| `Name_Lang_Mask` | 16712190 |
| `Charges` / `ItemVisual` / `Flags` / `Src_ItemID` / `Condition_Id` / `RequiredSkill*` / `MinLevel` | 0（永久附魔、无发光、无限制） |

### `spell_dbc`

以 `Spell.csv` 中真实武器附魔法术 **35793（附魔武器 - 猫鼬）** 为模板整行复制，仅覆盖：

| 字段 | 取值 |
| --- | --- |
| `ID` | 94500 起 |
| `EffectMiscValue_1` | 对应附魔 ID |
| `EquippedItemClass` | `2`（武器） |
| `EquippedItemSubclass` | `2090495` = 0~20 位除 9/11/12 外全部置位（单手/双手/主手/副手/远程/法杖/匕首等） |
| `EquippedItemInvTypes` | `0`（真实武器附魔均为 0，目标过滤靠子类掩码） |
| `BaseLevel` / `MaxLevel` | `0`（不限制目标装备等级/装等，避免 `SPELL_FAILED_LOWLEVEL` / `HIGHLEVEL`） |
| `Name_Lang_*` | `魄·<饰品名>`（中文写 deDE，enUS 留空） |
| `Description_Lang_*` | **直接沿用原特效法术的中文描述**（`Spell.csv` 的 `Description_Lang_deDE`），只写 deDE；描述里的裸 `$s1` 会按原法术数值在生成阶段展开（跨法术引用如 `$67714s1` 保持原样，由客户端解析） |
| `SpellIconID` / `ActiveIconID` | `241` / `0`（沿用模板法术 35793 的技能图标） |
| `CastingTimeIndex` | `16`（1.5 秒，配合卷轴的 `ITEM_FLAG_PLAYERCAST` 显示读条）。`1` = 瞬发（暴雪原版卷轴）、`14` = 3 秒（磨刀石那类），见 `SPELL_CASTING_TIME_INDEX` |
| 其余 | 全部沿用模板（`RangeIndex=1`、`InterruptFlags=15`、`SpellVisualID_1=3182` 等） |

### `item_template`（卷轴）

| 字段 | 取值 |
| --- | --- |
| `entry` | 63200 起 |
| `class` / `subclass` | `0` / `8`（消耗品-附魔） |
| `name` | `武器附魔·<饰品名>`（不带颜色码，紫色由 `Quality` 呈现） |
| `displayid` | 11161（附魔卷轴图标，对应 `ItemDisplayInfo.dbc`） |
| `Quality` | 4（紫色） |
| `Flags` | **`64`** = `ITEM_FLAG_PLAYERCAST`（物品法术由玩家施放；**缺了它客户端不会施法，也就没有读条**） |
| `stackable` | 1 |
| `Material` | 8 |
| `delay` | 0（与真实卷轴 33315 一致） |
| `spellid_1` / `spelltrigger_1` | 附魔法术 ID / `0`（使用） |
| `spellcharges_1` | **`-1`**（可消耗：施法成功后由 `Spell::TakeCastItem()` 销毁卷轴；**`0` 会导致永不消耗**） |
| `spellcooldown_1` / `spellcategorycooldown_1` | `-1`（用默认值） |
| 其余 | 数据库默认值 |

> 这三个字段（`Flags=64` + `spellcharges_1=-1` + 施法时间）是「右键使用有读条、附魔成功后卷轴消失」的必要条件，
> 对照参考：真实附魔卷轴 `33315 附魔武器死刑`（`Flags=64`、`spellcharges_1=-1`）与
> `2862 劣质磨刀石`（`Flags=64`、`CastingTimeIndex=14`）。
> 消耗判定见 `Spell::TakeCastItem()`：只有 `spellcharges_i != 0` 才会走「充能扣减 → 归零则销毁」。

### `spell_enchant_proc_data`

仅命中触发（`spelltrigger = 2`）的附魔需要：`entry = 附魔 ID`，`PPMChance = 原物品 spellppmRate`，
`customChance` / `procEx` / `attributeMask` 均为 0。饰品批次无此类特效，当前为空。

## DBC 数据的落库方式

生成脚本里 `WRITE_DBC_TABLES = False`（默认）：**附魔定义与附魔法术只输出客户端 DBC CSV，不写数据库**。

- 生成的 SQL 里对该区间只保留两条 `DELETE`（清掉早期版本写进 `spellitemenchantment_dbc` / `spell_dbc` 的覆盖行）——
  这两张表里的行会**覆盖客户端 DBC**，残留会导致你改了 DBC 也不生效；
- 实际数据由 `dbc/` 下两份 CSV 用 WDBX 导入客户端 `SpellItemEnchantment.dbc` / `Spell.dbc`（服务端从 DBC 目录读同一份文件）；
- 若你的服务端**不使用**客户端 DBC 目录（只有客户端打补丁），把 `WRITE_DBC_TABLES` 改为 `True` 再重跑，
  即可额外生成两张表的 INSERT（服务端 `LoadFromDB` 覆盖生效）。

## 名称从哪里来

| 显示位置 | 数据来源 | 当前取值 |
| --- | --- | --- |
| 背包/聊天/物品提示里的**卷轴名** | `item_template.name`（随物品查询包下发，Item.dbc 里没有名字字段） | `武器附魔·<饰品名>`，紫色由 `Quality = 4` 着色 |
| **附魔窗口**里的附魔名 | 客户端 `SpellItemEnchantment.dbc` 的 `Name_Lang_deDE` | `\|cffa335ee魄·<饰品名>\|r` |
| 附魔法术名（日志、错误提示） | `Spell.dbc` / `spell_dbc` 的 `Name_Lang_deDE` | `魄·<饰品名>` |

> 两个前缀是分开的：`generate.py` 里 `SCROLL_NAME_PREFIX = "武器附魔·"`（只作用于卷轴物品名）、
> `ENCHANT_NAME_PREFIX = "魄·"`（作用于附魔名与附魔法术名，附魔名额外套 `NAME_COLOR`）。
> 改完重跑生成 + 重新导入 DBC 即可。

**DBC 本地化列口径**（与 `outputs/自定义混沌宝石/混沌宝石_Spell.dbc.csv` 保持一致）：

| 列 | 写法 |
| --- | --- |
| `*_Lang_deDE` | **中文**（名称、描述；本项目客户端的中文都放这一列，其余语言列留空） |
| `*_Lang_enUS` | 英文；原 proc 法术在 `Spell.csv` 里都没有英文名，所以**留空**（不会把中文塞进 enUS） |
| `*_Lang_zhCN` 等 | 留空（SQL 里写 NULL） |
| `*_Lang_Mask` | 沿用模板法术 35793 的值（16712190；`AuraDescription_Lang_Mask` 为 16712188） |

`AuraDescription_*` 保持为空：附魔法术是 `Effect=53 ENCHANT_ITEM`，本身没有光环，
参考文件之所以填了那是因为它的法术是纯被动光环。

## 为什么每个特效都要一个卷轴物品

附魔 ID 写死在附魔法术的 `EffectMiscValue_1` 上，法术 ID 又写死在物品的 `spellid_1` 上，
这条链是**静态模板**，运行时无法替换。所以 N 个特效必然对应：

```
N 个 SpellItemEnchantment（附魔，客户端 DBC）
N 个 Spell（附魔法术，客户端 DBC）
N 个 item_template 卷轴（各自 spellid_1 指向自己的附魔法术）
```

不需要手工做，`generate.py` 一键生成；客户端只需为每个卷轴在 `Item.dbc` 里加一行（也由脚本输出）。

如果不想堆这么多物品，另一条路是「一个通用卷轴 + C++ 直接写附魔」（跳过 `Effect=53` 与附魔窗口）：
物品数可以压到 1 个，但客户端 DBC 里**仍然**必须为每个附魔准备 `SpellItemEnchantment` 条目，
而且会失去原生的附魔窗口/施法表现。当前「饰品特效提取器」的玩法即接近这一思路（提取时动态发卷轴）。

## 使用方法

```bash
# 仅打印将要生成的特效清单（不写文件）
python "outputs/饰品武器特效转附魔/generate.py" --list

# 生成全部产物
python "outputs/饰品武器特效转附魔/generate.py"
```

脚本顶部「运行环境配置」区可调整：

- `MYSQL_EXE` / `DB_USER` / `DB_PASS` / `DB_NAME`：本地 MySQL 客户端与库（默认 `E:\wlk80Server\mysql8.0\bin\mysql.exe`，`root` / `wow@80` / `acore_world`）
- `SPELL_CSV`：`Spell.csv` 路径
- ID 分配 / 命名与显示 / 附魔法术模板 / 卷轴物品模板：区间、前缀、颜色、模板法术 ID、武器子类掩码等
- 输出文件被 Excel 占用时，会自动改写为 `<同名>.new`，不会中断生成

## 部署步骤

1. 执行两份服务端 SQL（均幂等，可重复执行；只写 `item_template` / `spell_enchant_proc_data` / 映射表，
   以及清理 `spellitemenchantment_dbc`、`spell_dbc` 的旧覆盖行）：
   ```bash
   mysql -uroot -p acore_world < modules/mod-random-enchants/sql/world/trinket_enchant_scrolls.sql
   mysql -uroot -p acore_world < modules/mod-random-enchants/sql/world/trinket_essence.sql
   ```
2. 用 WDBX 编辑器把 `dbc/` 下的 CSV 导入客户端对应的 DBC，打成 MPQ 补丁下发：
   - `TrinketEnchant_SpellItemEnchantment.dbc.csv` → `SpellItemEnchantment.dbc`
   - `TrinketEnchant_Spell.dbc.csv` → `Spell.dbc`
   - `TrinketEnchant_Item.dbc.csv`（卷轴 63200~63277）+ `RandomEnchants_Items_Item.dbc.csv`（道具 60103/60104/60105）
     → `Item.dbc`（同一张表，两批合并导入）

   **服务端也要能读到同一份 DBC**（一般服务端 dbc 目录指向客户端 `Data`，或把改好的 DBC 拷到服务端 dbc 目录）。
3. **重新配置并编译 worldserver**（新增了 `src/TrinketEssence.cpp`，必须重新 CMake 生成工程再编译）。
4. 重启 worldserver：附魔/法术走客户端 DBC，`TrinketEssence` 在 `OnStartup` 加载映射表（日志会打印加载条数）。
5. 发放道具：`additem <角色> 60105`（饰品特效提取器）、`63200` 起（单个附魔卷轴）。

## 注意事项

1. **原法术依赖**：附魔的 `EffectArg_1` 直接引用原 proc 法术，不复制定义；原法术被删改则附魔失效。
2. **DBC 一致性**：附魔/附魔法术只存在于 DBC 里，服务端必须能读到打过补丁的那一份 DBC
   （dbc 目录指向客户端 `Data`，或把改好的 DBC 拷到服务端 dbc 目录），否则施放卷轴会报
   `unknown spell 945xx is casted, ignoring`；同理不要把这两张表的旧覆盖行留在库里。
3. **平衡性**：饰品 proc 附到武器上会显著改变强度，需自行评估。
4. **`spell_enchant_proc_data` 与 `EQUIP_SPELL` 无关**：只有 `COMBAT_SPELL` 才需要它。
5. 生成脚本会 `DELETE` 整个 ID 区块（30000~30099 / 94500~94599 / 63200~63299）并清空
   `mod_trinket_essence_map` 后重新插入，请勿在这些区块内手工添加其它内容。
6. 「饰品特效提取器」为**消耗品**（`TrinketEssence.cpp` 里 `CONSUME_ESSENCE = true`）：
   只有在**提取成功**（卷轴已生成）后才消失；若该饰品无可用特效或背包空间不足，则不会消耗。
   改成 `false` 即可变为可重复使用的工具。
7. **道具的 Item.dbc 行**：`dbc/RandomEnchants_Items_Item.dbc.csv` 是手写的，
   `generate.py` 不会覆盖它；改了道具的 `displayid`/`class` 等字段别忘了同步这个 CSV。
