# NPCBots 技能、天赋与雕文应用机制重构方案

> **适用范围**：AzerothCore WotLK 3.3.5a 集成版 NPCBots（`modules/mod-npcbots`，本项目为内嵌集成分支）。
>
> **目标**：把 Bot 的“技能可用性由职业 AI 按等级/专精硬编码”升级为“**按 Bot 等级模拟玩家逐步获得技能、分配天赋点、解锁并装备雕文**”，并在此基础上支持玩家为自己雇佣的 Bot 自定义构筑。
>
> **本文性质**：设计文档。不含代码修改，但**明确要求改动现有职业脚本**（见 3.1）。
>
> **配套文件**：`NPCBots职业脚本硬编码效果台账.csv`（阶段 A 交付物，战士清点范本，见附录 B）；`NPCBot技能天赋雕文应用重构_核查过程存档.md`（本文所依据的源码/DBC 核查过程记录，含每个论断的验证方式）。

---

## 1. 目标与实施原则

### 1.1 要解决的问题

当前 NPCBots 只有“某等级可用某技能”这一层语义，无法解释以下任何一项：

- 这一级**学会了什么**，技能链当前停在哪个 rank；
- 天赋点**花在哪里**，为什么某个被动在这个等级存在；
- **装备了哪个雕文**，切专精时如何撤销它的效果。

根因是三件事**没有分开建模**：技能获得、天赋分配、雕文装备目前都以“等级/专精字面条件”的形式散落在职业 AI 里。

### 1.2 设计方案：三层分离

采用 **“服务器规则数据 + 默认构筑 + 运行时按等级渐进应用”** 的混合方案：

| 层 | 权威来源 | 内容 |
|---|---|---|
| **规则** | 服务器 DBC / DB | 天赋合法性、法术 rank 链、雕文类型与槽位 |
| **构筑** | 项目维护的默认构筑 + 玩家预设 | 只记录“选什么”（加点顺序、雕文选择），不含树结构 |
| **生效状态** | 每个 Bot 每个专精的快照 | 该 Bot 实际应用了什么，可查询、可重建、幂等 |

**不复制完整天赋树**——完整树在服务器 `Talent.dbc` 中，手工复制会造成数据重复与补丁漂移。需要项目维护的只有“加点顺序”与“雕文选择”这两项**选择**。

### 1.3 实施原则

以下四条是本文所有设计决策的前提，实施排期时不得违反。

**原则一：这是“替换”而非“新增”。**
天赋/雕文效果目前以硬编码形式分散在职业脚本的 **3 类载体**中。新效果层能正确提供某项效果后，**必须同批次删除对应的旧硬编码**，不允许新层与旧硬编码并行。

| 载体 | 代码形态 | 实测规模 |
|---|---|---|
| **A. 光环类** | `ApplyClassPassives()` 内的 `RefreshAura(spellId, 条件 ? 1 : 0)` | **425** 处调用（含 53 个 `GLYPH_*` 常量） |
| **B. 数值类** | `ApplyClass*` 修正函数内的 `crit_chance += N`、`pctbonus *= x`、`cooldown -= N`、`cost -= N` | **160** 个覆写函数，含 **178** 处雕文效果注释 |
| **C. 行为类** | AI 决策分支内的等级/专精条件 | 未逐一清点，散落在技能选择、`GetAIMiscValue()`、`ReduceCD()` 等处 |

（统计口径：10 个玩家职业文件 `bot_<class>_ai.cpp`。详见 2.4。）

**并行不得的原因是两类残留后果不同**：A 类会互相覆盖（难察觉但不超模），B/C 类会**双重累加**（直接超模，且不报错、不崩溃、无日志）。详见 4.3。

**原则二：所有等级判定以 `me->GetLevel()` 为唯一基准。**
现有 10 个职业文件的 `ApplyClassPassives()` 使用 `master->GetLevel()`（玩家主人等级），与 Bot 真实等级存在系统性偏差。新代码一律用 `me->GetLevel()`，既有代码在阶段 B 统一替换（见 8.2）。

**原则三：点预算是截断阈值，不是拒绝条件。**
Bot **不套用玩家的 71 点上限**，构筑声明的点数**也不设上限**。低等级 Bot 只应用前 `A(等级)` 点，其余保留为未生效——这是正常状态，不报错（见 5.1）。

**原则四：不把 Creature 塞进 Player 的天赋/雕文流程。**
玩家流程依赖 `Player` 的 TalentMap、SpellMap、槽位、数据库与网络更新。Bot 是 Creature/AI，应复用规则与校验**思想**，而不是把 `Player` 成员函数当通用服务。

### 1.4 玩家侧新增能力（第二阶段）

在第一阶段（Bot 自动按默认构筑渐进应用）之上，第二阶段允许玩家为自己雇佣的 Bot 设置构筑，采用**三张表按职责分离**（见 4）：

- 默认构筑放 **world 库**（只读内容，随服务器版本走）；
- 玩家预设按 `(owner, bot_class, spec, preset_index)` 支持**每系多套**；
- Bot 实际生效的构筑**单独建表存完整快照**。

两条必须记住的规则：

- **新雇佣的 Bot 一律用系统默认构筑**，不继承玩家任何偏好；只有玩家显式设置才改用自定义。
- **“应用”与“保存为预设”是两个互相独立的操作**：应用只写生效快照、立即生效、**不落预设库**；保存为预设只写预设库、**不改变任何 Bot**。

此外支持**构筑的导出与导入**（见 6.6）：导出一串紧凑的分享码（直接复用 `80.wowdb.cc` / `wowhead` 系模拟器的 URL 编码规则，因此玩家可粘贴到攻略站对照），导入时接受分享码或完整模拟器链接。**导入只填充编辑区，不写库**——入库仍需玩家显式执行“应用”或“保存为预设”，导入不构成绕过校验的后门。

### 1.5 实施阶段总览

```text
阶段 A  清点硬编码 + 建立行为基线（产出效果台账）
阶段 B  统一等级基准、集中技能来源
阶段 C  接入天赋分配（同批次删除天赋硬编码）
阶段 D  雕文配置化（同批次删除雕文硬编码）
阶段 D′ 职业脚本零残留复查（与台账对账）
阶段 E  专精切换、等级跳跃与持久化
阶段 F  玩家预设、三张表、服务端接口与 mod-ale 钩子
阶段 F′ 构筑导出/导入（分享码与模拟器链接，见 6.6）
阶段 G  客户端天赋树 UI（不在本方案内）
```

除阶段 G 外，每阶段的完成判据都包含“**对应硬编码已删除**”，而不只是“新功能可用”。

---

## 2. 现状基线

本节是事实核查记录，作为后续设计的依据。所有论断已对照本地源码与 DBC 验证。

### 2.1 主要代码位置

| 领域 | 位置 | 当前职责 |
|---|---|---|
| Bot 通用初始化 | `src/server/game/AI/NpcBots/bot_ai.cpp` | `SetStats(bool force)` 在 `bot_ai.cpp:2396`；等级变更时于 `:2448-2449` 依次调用 `InitSpells()` → `ApplyClassPassives()`（注释明确 `//this must stay before class passives`）。`InitSpellMap()`（`:7474`）按等级沿法术 rank 链选最高可用等级 |
| 专精持久化 | `botdatamgr.h:76`（枚举 `NPCBOT_UPDATE_SPEC`）、`botdatamgr.cpp` | `characters_npcbot.spec` 保存专精；`bot_ai::SetSpec()`（`bot_ai.cpp:16031`）在 `activate=true` 时写库并重建 |
| 职业技能规则 | `bot_<class>_ai.cpp` | 19 个职业/AI 类各自 override `InitSpells()`、`ApplyClassPassives()` 与约 10–16 个 `ApplyClass*` 修正函数；等级门槛与专精条件**以硬编码形式散落在这三类载体中** |
| 玩家天赋/雕文参考 | `Player.cpp` / `Player.h` | `Player::InitGlyphsForLevel()`（`Player.cpp:13789`）按等级位掩码解锁雕文槽；`Player.h:1804-1806` 提供 `SetGlyphSlot` / `GetGlyphSlot` / `SetGlyph` |

### 2.2 现状与玩家机制的差异

- Bot 自维护 `_spells`（`bot_ai.h:873`，`BotSpellMap = std::unordered_map<uint32 /*firstrankspellid*/, BotSpell>`），不等同于玩家 SpellMap；等级变化时会重选 rank，但这不表示它逐级“学习”并保存了玩家技能。
- `spec` 是专精标识（`botcommon.h:340-361`，取值 1–30，`BOT_SPEC_DEFAULT = 31`），**不等同于完整的三系 Talent rank 分配**。
- 已有 53 个 `GLYPH_*` spell 枚举常量（如 `bot_warrior_ai.cpp:128 GLYPH_HEROIC_STRIKE = 58357`），但**不存在** Bot 雕文槽字段、雕文 ID 持久化或通用装备/撤销流程。
- 天赋与雕文效果都不止一种载体，且**同一个效果会跨载体出现**：

| 效果（战士） | A 类 `ApplyClassPassives()` | B 类 `ApplyClass*()` |
|---|---|---|
| Poleaxe Specialization（天赋） | 不存在 | `:1215-1220` `pctbonus *= 1.025f` + `:1245-1250` `crit_chance += 5.f`（同一天赋拆两处） |
| Sword and Board（天赋） | 不存在 | `:1265-1266` `crit_chance += 15.f` |
| Critical Block（天赋） | `:2082` `RefreshAura(CRITICAL_BLOCK, isProt && level >= 50)` | `:1261-1263` `crit_chance += 15.f` |
| Glyph of Victory Rush（雕文） | 不存在 | `:1268-1270` `crit_chance += 30.f` |
| Glyph of Heroic Strike（雕文） | `:2085` `RefreshAura(GLYPH_HEROIC_STRIKE, level >= 15)` | 不存在 |

  同一个效果或只在 A、或只在 B、或**同时在 A 和 B** → 因此**清点单位是“效果”而不是“常量”**（见 8.1）。
- 不能直接套用玩家 API，理由见 1.3 原则四。

### 2.3 核查方法

| # | 论断 | 验证方式 | 结论 |
|---|---|---|---|
| 1 | Bot 无天赋树分配、无雕文槽位 | 全目录检索 `glyph`：仅命中注释、`GLYPH_*` 常量、`botcommands.cpp:4543` 一处 `spell->m_glyphIndex = 0`（与 Bot 雕文无关） | **成立**：不存在 Bot 侧雕文槽字段、雕文表或装备流程 |
| 2 | 雕文效果已硬编码进职业 AI | 统计 `GLYPH_*` 常量 **53** 个；`RefreshAura()` 在 10 个玩家职业文件共 **425** 处调用；`//...Glyph...` 注释 **178** 处（多数位于 `ApplyClass*` 数值修正内） | **成立**，且规模远大于“若干常量 + RefreshAura” | 
| 3 | `ApplyClassPassives()` 是“应用被动法术”的可用先例 | 读 `bot_warrior_ai.cpp:2036-2085`；`RefreshAura()` 内部先 `RemoveAurasDueToSpell` 再按 count `AddAura()` | **成立**，已内建 apply/remove 对称语义，可直接作为效果层基础 |
| 4 | 玩家雕文槽按 15/30/50/70/80 分段 | `Player.cpp:13800-13809`：`>=15→0x03`、`>=30→0x08`、`>=50→0x04`、`>=70→0x10`、`>=80→0x20`，写入 `PLAYER_GLYPHS_ENABLED` | **成立**；本项目改为 20 级全开属**有意偏离** |
| 5 | 天赋 rank 可用 `TalentEntry->RankID[rank-1]` 反查 | `DBCStructure.h` `TalentEntry`：`TalentID(0)`、`TalentTab(1)`、`Row(2)`、`Col(3)`、`RankID[5](4-8)`、`DependsOn(13)`、`DependsOnRank(16)`、`addToSpellBook(19)`；`MAX_TALENT_RANK = 5` | **成立**，校验所需数据齐备 |
| 6 | 雕文效果技能可由 `GlyphProperties.dbc` 的 `SpellId` 反查 | `GlyphPropertiesEntry{Id, SpellId, TypeFlags}`，格式串 `"niix"`，实测 362 条 × 4 字段 | **成立** |
| 7 | 雕文槽类型校验需要 `GlyphSlot` 数据 | `GlyphSlotEntry{Id, TypeFlags, Order}`，格式串 `"nii"`，实测 10 条；玩家侧校验见 `PlayerStorage.cpp:5929-5939`、`SpellEffects.cpp:4609-4611` | **成立**，`TypeFlags` 0=大 / 1=小（见 2.5.2） |
| 8 | `spec` 在 10 级以下固定返回默认 | `bot_ai::GetSpec()`：`return me->GetLevel() < 10 ? uint8(BOT_SPEC_DEFAULT) : _spec;` | **成立** |
| 9 | 专精切换经 `ACTIVATE_SPEC` 落地 | `SwitchSpec()`（`bot_ai.cpp:16067`）设 `_newspec` 并 `CastSpell(me, ACTIVATE_SPEC)`（`botspell.h:21`，ID 63645）；`:17210-17211` 在法术落地后调 `SetSpec(_newspec)` | **成立** |
| 10 | `SetSpec(activate=true)` 重建技能/被动/治疗 | `bot_ai.cpp:16031-16060`：写库 → `UnsummonAll` → `removeShapeshiftForm` → `RemoveAllAurasExceptType` → `ApplyItemsSpells` → `ApplyRacials` → `InitSpells` → `ApplyClassPassives` → `InitHeals` → 清空三系能量 | **成立**。这是新增构筑应用逻辑应挂载的既有重建序列 |
| 11 | 硬编码载体远不止 `ApplyClassPassives()` | 统计 10 个玩家职业文件：`ApplyClass*` 覆写 **160** 个、`RefreshAura()` **425** 处、`GLYPH_*` **53** 个、含等级门槛判断的行 **1172** 处 | **成立**。这是“两套并行”风险的根源 |
| 12 | 天赋与雕文效果也走数值修正路径 | `bot_warrior_ai.cpp:1207-1275`：Poleaxe Spec、Incite、Improved Overpower、Critical Block、Sword and Board、Glyph of Victory Rush 全部为累加/累乘 | **成立**。此类残留**不会互相覆盖，而是双重累加** |

### 2.4 载体规模实测

逐文件统计 10 个玩家职业文件（`bot_<class>_ai.cpp`）：

| 载体 | 代码形态 | 实测规模 | 对应效果路径 |
|---|---|---|---|
| **A. `ApplyClassPassives()` 内的 `RefreshAura()`** | `RefreshAura(spellId, 等级/专精条件 ? 1 : 0)` | **425** 处调用，含 **53** 个 `GLYPH_*` 常量 | 路径 1（光环） |
| **B. `ApplyClass*` 系列修正函数** | `crit_chance += N`、`pctbonus *= x`、`cooldown -= N`、`cost -= N`，配 `//Glyph of ...` / `//Xxx Specialization: ...` 注释 | **160** 个覆写函数；**178** 处雕文效果注释（多数**连常量都没有**） | 路径 2（数值） |
| **C. AI 决策分支** | `if (等级/专精条件)` 切换到另一套优先级或目标选择 | 未逐一清点 | 路径 3（行为） |

**含义：只重构 A 类等于只做了一半。** B、C 类残留会与新效果层并存，这正是“两套并行”。

**雕文的风险尤其高**：以战士为例，21 条雕文中仅 6 条是 A 类光环，**15 条只在 B 类存在**（`Glyph of Victory Rush` / Bloodrage / Sweeping Strikes 等）。若按“改造 `ApplyClassPassives()`”理解迁移范围，会漏掉约 **71%** 的雕文效果。

### 2.5 等级基准偏差

`bot_ai::SetStats()`（`bot_ai.cpp:2394-2403`）计算 Bot 实际等级：

```cpp
uint8 mylevel = std::min<uint8>(master->GetLevel(), DEFAULT_MAX_LEVEL);
// ... 自由 Bot 走独立的 _baseLevel / _killsCount 逻辑
else
    mylevel += BotDataMgr::GetLevelBonusForBotRank(me->GetCreatureTemplate()->rank);
mylevel = std::min<uint8>(mylevel, DEFAULT_MAX_LEVEL + 3);
mylevel = std::max<uint8>(mylevel, BotDataMgr::GetMinLevelForBotClass(_botclass));
```

即雇佣 Bot 的等级 = `master 等级 + 精英等级加成`（受职业最低等级、`MAX_LEVEL+3` 上限约束），**且自由 Bot 完全不受 master 等级影响**。

而 **10 个玩家职业文件**的 `ApplyClassPassives()` 首行均写作 `uint8 level = master->GetLevel();`：

| 文件 | 行 | 文件 | 行 |
|---|---:|---|---:|
| `bot_warrior_ai.cpp` | 2038 | `bot_paladin_ai.cpp` | 2349 |
| `bot_death_knight_ai.cpp` | 1905 | `bot_priest_ai.cpp` | 2215 |
| `bot_druid_ai.cpp` | 2979 | `bot_rogue_ai.cpp` | 2383 |
| `bot_hunter_ai.cpp` | 2387 | `bot_shaman_ai.cpp` | 2636 |
| `bot_mage_ai.cpp` | 1867 | `bot_warlock_ai.cpp` | 2115 |

另有 `bot_ai.cpp:7721`（`_OnHealthUpdate()` 小兵血量）与 `:7775` 两处同类用法。`master` 是 `Player* master`（`bot_ai.h:633`），即**玩家主人**，不是 Bot 本身。

**后果**：自由 Bot 的 `master->GetLevel()` 与 Bot 真实等级脱节；雇佣场景下恒偏低一个 rank 加成。这直接冲击本方案的核心判据——“按 Bot 等级渐进应用”必须以 `me->GetLevel()` 为唯一基准（原则二）。

### 2.6 等级变化的入口

现有链路是 `Player::GiveLevel` → `OnPlayerLevelChanged`（`Player.cpp:2686`）→ `SetBotsShouldUpdateStats()`（`botmgr.cpp:1477`）→ `SetShouldUpdateStats()` → `UpdateAI` 中 `SetStats(false)`。

但 Bot 等级在以下路径同样会变，且**不经过**该回调：

- 自由 Bot 击杀累积：`_killsCount` 提升导致 `mylevel` 上升（`bot_ai.cpp:17052`）；
- 招募 / 重新召唤：`botmgr.cpp:549` 直接 `SetShouldUpdateStats()`；
- 装备变更、角色变更：`bot_ai.cpp:14405`、`:14424`、`:15738`；
- GM 命令与脚本直接改等级。

**因此统一入口 `ReconcileBotProgression(bot)` 必须挂在已有的 `SetStats()` 重建序列内部**，而不是新增一个独立的等级事件回调——只有 `SetStats()` 被上述全部路径覆盖。

### 2.7 点数上限

`mylevel = std::min<uint8>(mylevel, DEFAULT_MAX_LEVEL + 3);`（`bot_ai.cpp:2436`）允许 Bot 最高 **83 级**。

原有“等级 - 9，上限 71”的推断不成立：Bot 等级可达 83，且按本项目规则点数**不套用玩家 71 点上限**。故点预算实现为“等级基准 + 额外天赋点”的可加形式（见 5.1）。

### 2.8 解雇与删档是两条路径

原有描述“在 `NPCBOT_UPDATE_OWNER` 分支执行清理”不完整——实际是**两个语义不同的位置**，都需要删除生效快照：

| 路径 | 位置 | 语义 | 为何必须清理 |
|---|---|---|---|
| `NPCBOT_UPDATE_OWNER`（批量） | `botdatamgr.cpp:3159` | 把某玩家**所有** Bot 的 `owner` 置 0 | 解雇后 Bot 变自由 Bot，下次雇佣应回到系统默认 |
| `NPCBOT_UPDATE_ERASE`（单 entry） | `botdatamgr.cpp:3136` | **彻底删除**该 Bot 的 `characters_npcbot` 行（调用点 `botcommands.cpp:3654`、`:3706`） | 不删会留下**永久孤儿行** |

批量分支的语句要写成 `WHERE entry IN (SELECT entry FROM characters_npcbot WHERE owner = ?)` 形式以复用现有 transmog 清理（`CHAR_DEL_NPCBOT_TRANSMOG_ALL`）的模式，且**必须在 `UPDATE owner` 之前求值**，否则子查询结果已为空。

### 2.9 DBC 与数据源

客户端 DBC 目录：`G:/wow/dbc/`（与 `E:/WoW Spell Editor/DBC_335_wotlk/` 内容一致）。字段语义以 `src/server/shared/DataStores/DBCfmt.h` 的格式串为准。

> **读取注意**：WDBC 文件头固定 **20 字节**（magic `WDBC` + recordCount/fieldCount/recordSize/stringBlockSize 各 uint32）。按 24 字节读会整体错位一格、ID 列看似全 0。后续工具脚本须注意。

| 数据 | 规模（已实测） | 格式串 | 结构要点 |
|---|---|---|---|
| `Talent.dbc` | 892 条 × 23 字段 × 92 字节 | `niiiiiiiixxxxixxixxixxx` | `TalentID(0)`、`TalentTab(1)`、`Row(2)`、`Col(3)`、`RankID[0..4](4-8)`、`DependsOn(13)`、`DependsOnRank(16)`、`addToSpellBook(19)`；TalentID 实测范围 23–2285 |
| `TalentTab.dbc` | 33 条 × 24 字段 × 96 字节 | `nxxxxxxxxxxxxxxxxxxxiiix` | `TalentTabID(0)`、`ClassMask(20)`、`petTalentMask(21)`、`tabpage(22)` |
| `GlyphProperties.dbc` | 362 条 × 4 字段 × 16 字节 | `niix` | `Id(0)`、`SpellId(1)`、`TypeFlags(2)`、`spellIconID(3)` |
| `GlyphSlot.dbc` | 10 条 × 3 字段 × 12 字节 | `nii` | `Id(0)`、`TypeFlags(1)`、`Order(2)` |
| 法术 rank 链 | `outputs/Spell.csv` | — | `InitSpellMap()` 通过 `SpellInfo::GetNextRankSpell()` 沿链上升 |

**服务端确实加载这些表**（`DBCStores.cpp:370-371` 的 `LOAD_DBC(sTalentStore, ...)` / `sTalentTabStore`；`:307-308` 的 `sGlyphPropertiesStore` / `sGlyphSlotStore`）。运行时读取 DBC 做校验可行，**无需自建天赋树数据**。

#### 2.9.1 天赋树归属映射

`TalentTab.dbc` 字段 20 的 `ClassMask` 实测取值恰为 `1 << (classID - 1)`，覆盖全部 10 个可选职业：

| ClassMask | bit | classID | 职业 | 该职业 3 个 TalentTab（tabpage 0/1/2） |
|---:|---:|---:|---|---|
| 1 | 0 | 1 | Warrior | 161 / 164 / 163 |
| 2 | 1 | 2 | Paladin | 382 / 383 / 381 |
| 4 | 2 | 3 | Hunter | 361 / 363 / 362 |
| 8 | 3 | 4 | Rogue | 182 / 181 / 183 |
| 16 | 4 | 5 | Priest | 201 / 202 / 203 |
| 32 | 5 | 6 | Death Knight | 398 / 399 / 400 |
| 64 | 6 | 7 | Shaman | 261 / 263 / 262 |
| 128 | 7 | 8 | Mage | 81 / 41 / 61 |
| 256 | 8 | 9 | Warlock | 302 / 303 / 301 |
| 1024 | 10 | 11 | Druid | 283 / 281 / 282 |
| 0 | — | — | 非职业系（409/410/411，宠物/特殊） | 应排除 |

关键点：

- **`BOT_CLASS_*` 与职业 ID 恒等**：`botcommon.h:222-232` 定义为 `BOT_CLASS_WARRIOR = CLASS_WARRIOR`（=1）… `BOT_CLASS_DRUID = CLASS_DRUID`（=11）。因此 10 个真实职业值**直接就是 classID**，不需要映射表。
- **两套 mask 约定不要混用**（`bot_ai.h:144-145`）：
  - `GetBotClassMask0()` = `1ul << (_botclass - 1)` —— 与 `TalentTab.ClassMask`、`Player::GetClassMask()` 同约定，**天赋树校验用这个**；
  - `GetBotClassMask1()` = `1ul << _botclass` —— 与模块自有 `BOT_CLASS_MASK_*` 同约定，用于 Bot 内部分组。
- **`BOT_CLASS_BM` 及之后 9 个自定义职业没有天赋树**（`BOT_CLASS_EX_START = BOT_CLASS_BM`，无显式赋值、从 12 起递增）。这些职业不得进入构筑流程，否则 `ClassMask` 校验必然失败。

#### 2.9.2 雕文槽与大小雕文判定

`GlyphSlot.dbc` 全部 10 条（`Id`, `TypeFlags`, `Order`）：

```
(1, 4, 0)   (2, 2, 0)   (3, 3, 0)   (4, 0, 0)      <- Order=0，非玩家槽位，忽略
(21, 0, 1)  (22, 1, 2)  (23, 1, 3)  (24, 0, 4)  (25, 1, 5)  (26, 0, 6)
```

可用槽位即 `Order != 0` 的 6 条，与 `Player::InitGlyphsForLevel()`（`Player.cpp:13791-13794`，`SetGlyphSlot(gs->Order - 1, gs->Id)`）完全一致：

| 槽位索引 | `Order` | `Id` | `TypeFlags` | 类型 |
|---:|---:|---:|---:|---|
| 0 | 1 | 21 | 0 | **大雕文** |
| 1 | 2 | 22 | 1 | **小雕文** |
| 2 | 3 | 23 | 1 | **小雕文** |
| 3 | 4 | 24 | 0 | **大雕文** |
| 4 | 5 | 25 | 1 | **小雕文** |
| 5 | 6 | 26 | 0 | **大雕文** |

即 **3 大（索引 0/3/5）+ 3 小（索引 1/2/4）**，与 `BOT_GLYPH_MAX_SLOTS = 6` 一致。

**`TypeFlags` 语义**：`GlyphProperties.TypeFlags` 实测分布为 `{0: 284, 1: 73, 2: 1, 64: 1, 128: 2, 162: 1}`。经已知雕文交叉验证：

| 雕文 | `SpellId` | `GlyphProperties.TypeFlags` | 实际类型 |
|---|---:|---:|---|
| Glyph of Heroic Strike | 58357 | 0 | 大雕文 |
| Glyph of Revenge | 58388 | 0 | 大雕文 |
| Glyph of Chains of Ice | 58620 | 0 | 大雕文 |
| Glyph of Disease | 63334 | 0 | 大雕文 |
| Glyph of Charge | 58097 | 1 | 小雕文 |

**结论：`TypeFlags = 0` 表示大雕文，`1` 表示小雕文**（284 : 73 ≈ 3.9:1，与“大雕文远多于小雕文”的实情吻合）。

两个可直接落地的性质：

1. 匹配规则就是 `GlyphProperties.TypeFlags == GlyphSlot.TypeFlags`（`PlayerStorage.cpp:5932`、`SpellEffects.cpp:4611`），Bot 侧照抄即可，**不要自造大/小雕文枚举**；
2. 由于是等值比较，`TypeFlags` 为 2/64/128/162 的异常行**永远不会匹配任何槽位**（槽位只有 0 和 1），天然被排除，无需黑名单。

> 以上数值实测自 `G:/wow/dbc/`。若更换客户端版本需重新确认，尤其 `ClassMask` 与 `GlyphSlot.TypeFlags`。

---

## 3. 目标架构

### 3.1 分层

```text
服务器规则层（Talent / Spell / Glyph / Trainer 数据）
          ↓ 查询、校验、解析
构筑来源（系统默认构筑 / 玩家预设，按职业 + 专精 [+ 预设序号]）
          ↓ 等级条件与加点顺序
Bot 生效状态（该 entry 该专精的实际天赋与雕文快照）
          ↓ 派生效果
┌──────────────────────────────────────────────────┐
│ 效果应用层：光环、数值修正、技能启用、行为开关      │
│   路径 1 DBC 光环 / 路径 2 数值修正 / 路径 3 行为   │
│   —— 取代职业脚本中现有的全部同类硬编码 ——          │
└──────────────────────────────────────────────────┘
          ↓ 查询（不再读等级/专精字面量）
职业 AI：施法循环与技能优先级保留，效果差异改为查生效构筑
```

职责边界：

- **规则层**：判断技能 rank、天赋合法性、点数与前置、雕文类型/槽位/等级限制。
- **构筑来源**：默认构筑开箱即用；玩家预设仅在玩家显式应用到某 Bot 时参与。
- **生效状态层**：记录该 Bot 该专精实际应用了什么。必须可重建、可审计、幂等。
- **效果层**：集中管理 Talent/Glyph 带来的被动法术、光环与数值修正，要求 apply/remove 对称。
- **AI 层**：只关心当前有效技能与战斗决策。**天赋/雕文带来的数值与行为差异一律通过查询生效构筑获得，不再内嵌等级判断**；现有施法循环与技能优先级保留，但其中以等级/专精字面条件表达的天赋效果须改为构筑查询。

> **本方案会改动职业脚本本身**（`bot_<class>_ai.cpp`），不是只在其上叠加一个新层。这一层关系必须在实施排期时就明确，否则会退回“两套并行”。

### 3.2 效果应用的三条路径

天赋/雕文的效果技能由数据反查得到，但“如何应用”需按效果性质分三条路径，**默认走第 1 条**：

| 路径 | 适用 | 做法 | 代价 |
|---|---|---|---|
| **1. DBC 光环（默认）** | 纯被动光环、直接授予技能 | 反查 spellId 后由 Bot 施放/应用该法术 | 无 |
| **2. 代码数值修正** | 光环在 Bot 上不生效，或效果依赖 Bot 自有属性计算 | 在效果层按条件和数值修正 | 数值被复制进代码，需与 DBC 同步 |
| **3. AI 行为改动** | 效果是决策而非数值（如改变技能优先级） | 在职业 AI 中按条件分支 | 需逐项确认 |

判定顺序：

```text
1. 能挂 DBC 光环 → 路径 1
2. 光环挂不上或不生效，或效果依赖 NpcBotStats 等自有计算 → 路径 2
3. 效果是 AI 行为而非数值 → 路径 3
```

约束：

- **优先路径 1**，因为它以 `Spell.dbc` 为权威，参数、持续时间、叠加规则不必手写，改 DBC 自动生效，与玩家行为一致。Bot 侧 `ApplyClassPassives()` 已是“应用被动法术”的可用先例，且 `RefreshAura()`（`bot_ai.cpp`）**已内建 apply/remove 对称语义**：

  ```cpp
  void bot_ai::RefreshAura(uint32 spellId, int8 count, Unit* target) const
  {
      if (count < 0 || count > 1) { /* 越界报错并返回 */ }
      if (!spellId) { /* 报错并返回 */ }
      SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
      if (!spellInfo) { /* 报错并返回 */ }
      spellInfo = spellInfo->TryGetSpellInfoOverride(me);
      if (!target) target = me;
      target->RemoveAurasDueToSpell(spellId);   // 先撤销
      if (count) target->AddAura(spellInfo, MAX_EFFECT_MASK, target);  // 再按需施加
  }
  ```

  即 `RefreshAura(id, 0)` 等价于“移除该效果”、`RefreshAura(id, 1)` 等价于“施加该效果”，天然幂等（重复调用不叠加）。**效果层的 apply/remove 应直接复用该函数**，不要另写 `AddAura`/`RemoveAura` 配对逻辑。

- **`RefreshAura()` 的 `count` 只接受 0/1，不支持叠层。** 现有实现用**不同 spell ID 表达不同 rank 并互斥刷新**（`bot_warrior_ai.cpp:2043-2045` 的 `DEEP_WOUNDS_1/2/3`），而非对同一 spell 叠 count。天赋 rank 关联的效果若采用该模式，`talent_data` 的 rank 变化需映射到不同 spell ID，**不能假定“施加一次 spell 就能覆盖所有 rank”**。

- **路径 2 等于把 DBC 数值抄进 C++**，存在“改了 DBC 但 Bot 仍用旧值”的漂移风险。仅在前两条前提不成立时使用。**职业脚本中现有的 B 类硬编码本质上就是路径 2 的裸写版本**——迁移要做的是把它从“写死的数字”升级为“从生效构筑反查得到的值”，而不是在保留它的基础上再新增一套。

- **无论走哪条路径，都必须标明该项对应哪个 Talent ID / Glyph ID / Spell ID**，作为可追溯锚点，否则后续调平衡时无从定位需同步之处。

### 3.3 构筑数据

每个构筑至少包含：

```text
bot_class
spec
plan_version
talent_data   -- 有序 talentId:rank 序列
glyph_data    -- slot:glyphId 列表
```

用 `talentId + rank` 而非只存天赋法术 ID：规则读取时再解析该 rank 对应 spell，并能报告 rank 缺失。加点顺序须显式保存，因为相同最终分配可能存在多个合法逐级路径。构筑带版本号，升级后的新版本在 Bot 重建或管理员明确刷新时迁移，不要每次启动无条件重置已有状态。

### 3.4 不复制完整天赋树

**不需要手工准备整棵树**——完整树在服务器 DBC 中，手工复制会造成数据重复、补丁漂移与前置不一致。

**需要准备默认构筑**：每个受支持专精应有完整的目标 Talent rank 分配与雕文默认选择。建议工具读取服务器 Talent 数据校验构筑：rank 合法、职业/树匹配、各层级前置满足、**序列每个前缀都是合法部分分配**。**不校验“总点数是否超预算”**——构筑点数不设上限，预算只决定低等级时截断多少。

> **“不设上限”是校验规则，不是撰写指引**（易误读，特此澄清）：它说的是**系统不会因构筑点数超过 `A(等级)` 而拒绝**；**不代表默认构筑就应当按远超预算的点数去写**。默认构筑是**给人看、供玩家对照攻略**的静态数据，应表达“这个专精该怎么点”，因此**按玩家 71 点规则（主系 51 + 副系 20）构建**。按满配写会造成两个实际危害：①截断只切尾，被切掉的恰是副系核心输出点（而非低价值填充），取舍逻辑反转；②玩家无法用游戏内天赋模拟器核对点数。参见盗贼实施项的第 0 节第 5 条。

即“**树由服务器权威维护，构筑由项目维护**”。

---

## 4. 数据模型

### 4.1 三张表

| 表 | 库 | 主键 | 职责 |
|---|---|---|---|
| `npcbot_default_talent_plan` | **world** | `(bot_class, spec)` | 服务端静态默认构筑与雕文 |
| `characters_npcbot_talent_plan` | characters | `(owner, bot_class, spec, preset_index)` | 玩家预设，一系多套 |
| `characters_npcbot_applied_plan` | characters | `(entry, spec)` | Bot 某系当前实际生效的构筑快照 |

**SQL 位置**：本项目对模块自定义表使用 `data/sql/custom/db_world/`、`data/sql/custom/db_characters/`（已先例：`2024_10_17_00_characters_npcbot_settings.sql`、`2024_11_13_00_characters_npcbot_gear_set.sql`），采用 `SET FOREIGN_KEY_CHECKS=0; DROP TABLE IF EXISTS ...; CREATE TABLE ...;` 的幂等风格。按库分别为：默认构筑 → `custom/db_world/`；玩家预设与生效快照 → `custom/db_characters/`。

### 4.2 表 1：服务端默认构筑（world 库）

```sql
CREATE TABLE `npcbot_default_talent_plan` (
  `bot_class`    TINYINT UNSIGNED NOT NULL,    -- BOT_CLASS_*
  `spec`         TINYINT UNSIGNED NOT NULL,    -- BOT_SPEC_*
  `plan_version` INT UNSIGNED NOT NULL DEFAULT 1,
  `preset_name`  VARCHAR(32) DEFAULT NULL,     -- 如 'Default'
  `talent_data`  LONGTEXT NOT NULL,            -- **完整**有序 talentId:rank 序列（不按等级裁剪）
  `glyph_data`   LONGTEXT DEFAULT NULL,        -- slot:glyphId 列表
  `updated_at`   BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`bot_class`, `spec`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

放 world 而非 characters：默认构筑是**只读内容**，随服务器版本走，与 `creature_template`、`smart_scripts` 同类。放 characters 会导致每个角色库各存一份、GM 调整需逐库同步。启动时载入内存，运行期只读。

### 4.3 表 2：玩家预设（一系多套）

```sql
CREATE TABLE `characters_npcbot_talent_plan` (
  `owner`        INT UNSIGNED NOT NULL,        -- 玩家角色 guid
  `bot_class`    TINYINT UNSIGNED NOT NULL,    -- BOT_CLASS_*
  `spec`         TINYINT UNSIGNED NOT NULL,    -- BOT_SPEC_*
  `preset_index` TINYINT UNSIGNED NOT NULL,    -- 该职业该系第几套，从 1 开始
  `preset_name`  VARCHAR(32) DEFAULT NULL,     -- 玩家自定义名称（可选，仅供 UI）
  `plan_version` INT UNSIGNED NOT NULL DEFAULT 1,
  `talent_data`  LONGTEXT NOT NULL,            -- **完整**有序 talentId:rank 序列（不按等级裁剪）
  `glyph_data`   LONGTEXT DEFAULT NULL,        -- slot:glyphId 列表
  `created_at`   BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `updated_at`   BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`owner`, `bot_class`, `spec`, `preset_index`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

### 4.4 表 3：Bot 生效构筑快照

```sql
CREATE TABLE `characters_npcbot_applied_plan` (
  `entry`             INT UNSIGNED NOT NULL,                 -- Bot creature entry
  `spec`              TINYINT UNSIGNED NOT NULL,             -- 该行对应哪一系
  `plan_source`       TINYINT UNSIGNED NOT NULL DEFAULT 0,   -- 0=系统默认, 1=玩家预设, 2=玩家直编
  `preset_ref`        TINYINT UNSIGNED NOT NULL DEFAULT 0,   -- 来源预设序号；仅 plan_source=1 时有效，其余恒为 0
  `talent_data`       LONGTEXT NOT NULL,                     -- **完整**加点序列（未截断，与来源一致）
  `glyph_data`        LONGTEXT DEFAULT NULL,                 -- 玩家选择的雕文方案（6 槽）；<20 级时不生效，无需截断列
  `plan_total_points` SMALLINT UNSIGNED NOT NULL DEFAULT 0,  -- 完整序列的总点数（缓存，可由 talent_data 重算）
  `applied_points`    SMALLINT UNSIGNED NOT NULL DEFAULT 0,  -- 生效前缀长度：自动档 = min(总点数, A(等级))；手动档 = 玩家确认的点数
  `built_at_level`    TINYINT UNSIGNED NOT NULL DEFAULT 1,   -- 生成/最后一次应用时的 Bot 等级（手动档下仅作审计与显示）
  `updated_at`        BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`entry`, `spec`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

#### `plan_source` 三档

| 取值 | 含义 | `preset_ref` | 完整构筑在哪 | 升级时 |
|---|---|---|---|---|
| `0` | 系统默认构筑（新雇佣、未设置，或玩家预设已失效后回落） | 恒 0 | 表 1 | **自动补齐** |
| `1` | 玩家**保存并应用**的预设 | 有效，指向该系第几套 | 表 2 该预设；**预设被删除则来源失效**，重置为 0 并回默认 | **自动补齐**（按该预设） |
| `2` | 玩家在 UI 上**直接编辑后点应用**，未保存为预设 | 恒 0 | **快照自身**（没有第二份副本） | **不自动补点**，记为未分配 |

`0` 与 `1` 合称**自动档**，`2` 为**手动档**。分档的用途与完整规则见 5.3。

`preset_ref` **仅供 UI 溯源显示**（“当前来自防护系第 2 套”），**不参与任何逻辑判断**；`plan_source != 1` 时必须为 0，否则 UI 会误报来源。

`0` 与 `2` 在“是否受预设库变动影响”上截然不同：前者随服务器默认构筑更新而重建；后者是玩家的一次性临场决定，不受任何预设增删影响——也因此**解雇后即丢失**。

#### `talent_data` 为什么存完整序列

因为**“玩家直编”的构筑（`plan_source = 2`）没有第二份副本**——它不写预设库，快照就是唯一存储。若只存截断结果，40 级时应用的 120 点方案会被截成 31 点落库，于是：

- Bot 升到 60 级时，玩家想在**自己原有方案**的基础上继续分配那 20 点也**无从下手**——尾部 89 点的具体内容已经丢了，只能从头重配；
- 手动档下升级**不会自动补点**（见 5.3），但“保留完整方案”正是让玩家能一键沿用自己的思路继续分配的前提。

因此统一为：**快照存完整序列，`applied_points` 标记生效到第几项**。

> **读取约定**：生效天赋 = `talent_data` 的**前 `applied_points` 项**。效果层与 UI 一律按这个前缀切，**不要假定“`talent_data` 本身就是生效内容”**。

注意“存完整序列”**不等于**“自动补齐”——是否补齐由档位决定，与存储方式无关。

`plan_total_points` / `applied_points` 是**无上限模型的必要冗余**：构筑点数可以远超当前预算，`talent_data` 存完整序列而实际生效的只是它的前缀，所以需要 `applied_points` 明确“生效到第几项”，`plan_total_points` 让 UI 直接显示“已生效 31 / 共 120（等级不足）”，也便于 GM 排查“为什么这个 Bot 只有一半天赋”。二者均可由 `talent_data` 重算，属缓存字段。

**雕文不参与截断**：雕文槽只有“20 级前 0 槽 / 20 级起 6 槽”两态，因此 `glyph_data` 直接是“玩家选择的方案”，没有前缀概念。20 级以下的 Bot 会持有 `glyph_data` 但不生效，UI 应显示“达到 20 级后生效”；跨越 20 级时 `built_at_level` 变化已触发重建，不需要额外的失效标记。

#### 为什么单独建表而不加列到 `characters_npcbot`

1. `characters_npcbot` 是身份/归属表（owner、roles、spec、equips、miscvalues），读归属信息非常频繁，塞入两大段 LONGTEXT 会拖重每次读取。
2. 生效构筑**按专精**区分。加列只能存当前一份，切换专精后切回就丢；单独表按 `(entry, spec)` 可存多行。
3. 生命周期不同：归属信息长期驻留，生效构筑是派生态、会被频繁重建。分开后解雇清理只需删生效表。
4. 与模块既有惯例一致：`characters_npcbot_transmog`、`characters_npcbot_gear_set`、`characters_npcbot_settings` 都是旁挂表。

#### 为什么存快照而不引用预设

- **预设会被删**。只存引用则删预设后 Bot 失去合法来源，需额外兜底；存快照则删预设不影响运行中的 Bot。
- **预设会被改**。“编辑预设是否立即影响在跑的 Bot”在引用模型下必须回答且易混淆；快照模型下要生效就显式重算，行为明确。
- **等级截断是常态**。因为构筑点数不设上限，任何等级低于“配满所需等级”的 Bot 都只会生效一部分点；**实际生效的天赋本就是构筑的子集**，引用模型无法表达，快照天然表达（配合 `plan_total_points` / `applied_points` 显示差额）。
- **诊断友好**。直接查看这份数据即可，不必反查“当时引用的是哪套预设的哪个版本”。

#### 表 1 与表 2 的关系

表 1 提供开箱即用的默认构筑；表 2 是玩家自己保存的预设库，**只在玩家显式应用某个已存预设时才被读取**。表 2 存多套是为了让玩家复用（例如把“团本坦”套同样应用到另一个 Bot），而不是让新 Bot 自动继承。

**表 2 不是“应用”的必经之路。** 玩家在 UI 上直接改点后点应用（`plan_source = 2`）**完全不经过表 2**，构筑内容只存在于表 3 的快照里。

### 4.5 `talent_data` 存什么

存天赋点分配明细，每项为三元组，序列化后按加点顺序排列：

| 含义 | 取值来源 | 说明 |
|---|---|---|
| `talentId` | `Talent.dbc` 的 `TalentID`（第 1 列） | **必须是天赋本身 ID，不是它给予的法术 ID** |
| `rank` | 已投入点数 1..maxRank | 与 `TalentEntry->RankID[rank-1]` 对应，可反查实际生效法术 |
| 顺序 | 加点先后 | 保证任意等级可按序截断重放 |

```
204:5 205:3 206:1 1122:5 1123:2
```

**不存**：天赋树结构（来自 `Talent.dbc`）、天赋法术 ID（可由 `talentId+rank` 反查）、普通训练师技能、雕文（独立字段 `glyph_data`）。

**同一字段在三张表里语义一致**（都存完整序列），差别只在表 3 多记一个生效前缀：

| 表 | `talent_data` 内容 | 点数 |
|---|---|---|
| 表 1 / 表 2（构筑来源） | **完整**加点序列 | **不设上限**，可远超 71 |
| 表 3（生效快照） | **完整**加点序列（与其来源一致） | 序列本身不设上限；**其中前 `applied_points` 项生效** |

统一为“完整序列”有两个好处：读取方不必为不同表写不同解析逻辑；快照不会因截断而丢失尾部点数（对没有第二份副本的 `plan_source = 2` 是必需的）。

截断**只切尾、不重排**——因为序列本身保证每个前缀都是合法的部分分配，所以“取前 K 项”必然合法。**这也是“必须有序”的实际用途。**

两条硬约束：

- **必须存 `talentId` 而非 `spellId`**。同一天赋不同 rank 对应不同 spell（`RankID[]`），存 spell 无法回溯点数、校验前置与层级、正确洗点。
- **必须有序**。最终分配相同但路径可能不同；只有有序序列才能在低等级截断出合法子集并保证重建结果可复现。

### 4.6 `glyph_data` 存什么

存已装备的雕文，每项为 `slot:glyphId`：

| 含义 | 取值来源 | 说明 |
|---|---|---|
| `slot` | 0–5 | 与 `GlyphSlotEntry::Order - 1` 对齐（`Player.cpp:13794`）。**槽位 0/3/5 是大雕文槽，1/2/4 是小雕文槽** |
| `glyphId` | `GlyphProperties.dbc` 的 `Id` | **存雕文 ID 而非效果法术 ID**，效果法术由 `SpellId` 反查 |

```
# 槽位索引:glyphId     槽位类型：0/3/5=大雕文(TypeFlags 0)，1/2/4=小雕文(TypeFlags 1)
# 下列 ID 均已实测核对类型（示例混用职业仅为演示槽位↔类型对应关系）
0:52084  1:57856  2:57858  3:54810  4:57862  5:58357
```

槽位合法性校验必须包含 `GlyphProperties.TypeFlags == GlyphSlot.TypeFlags`（即大雕文只能进 0/3/5、小雕文只能进 1/2/4），与玩家侧实现一致。**不存**雕文效果法术 ID（由 `SpellId` 反查）。

### 4.7 效果技能从哪来（不需要手工准备）

天赋与雕文带来的**技能由服务器数据反查**，不需要为每个 rank、每个雕文单独维护技能清单：

| 数据 | 来源 | 说明 |
|---|---|---|
| 天赋树结构（节点/层级/前置/最大 rank） | `Talent.dbc` | 直接读，勿复制 |
| **天赋效果技能** | `Talent.dbc` 的 `RankID[]` | `spellId = TalentEntry->RankID[rank - 1]` |
| **雕文效果技能** | `GlyphProperties.dbc` 的 `SpellId` | `spellId = GlyphPropertiesEntry->SpellId` |
| 加点顺序 + 雕文选择 | 构筑表 | **唯一需要提前准备的内容** |

因此 `talent_data` 存 `talentId + rank` 就够——反查天然得到“该等级该天赋恰好应给哪个法术”，无需为“1/3 点”和“3/3 点”分别准备技能 ID；雕文同理存 `glyphId` 而非 `spellId`。

需要提前准备的只有两样：**加点顺序**（有序，理由见 4.5）与**雕文选择**。这两项是“选择”，不是“技能清单”。

---

## 5. 运行时行为

### 5.1 天赋点数规则

**Bot 不套用玩家的 71 点上限，构筑声明的点数也不设上限。**

```text
等级 < 10 ：0 点（此阶段不加点，与玩家一致）

等级基准     = 等级 - 9            与玩家速率一致，每级 1 点
额外天赋点   = 由外部机制提供      超出玩家规则的部分，本方案不设计其来源

生效预算 A(等级) = 等级基准 + 额外天赋点
```

**`A(等级)` 是截断阈值，不是拒绝条件。** 构筑可以声明远超当前预算的点数（如 80 级满配 120 点），低等级 Bot 只应用其中前 `A(等级)` 点，其余保留为“未生效”。这正是“不设上限”的落地方式。

对照表（仅等级基准，额外天赋点按 0 计）：

| 等级 | 等级基准 | 说明 |
|---:|---:|---|
| 9 | 0 | 未开点 |
| 10 | 1 | 起始 |
| 20 | 11 | 同时雕文槽全开 |
| 40 | 31 | |
| 60 | 51 | |
| 80 | 71 | **非上限**，仅是该等级的基准值 |
| 83 | 74 | Bot 等级上限为 `DEFAULT_MAX_LEVEL + 3 = 83` |

实现——基准与额外点分离，额外来源作为**外部集成点**：

```cpp
// 基准：与玩家速率一致（10 级起每级 1 点），本方案唯一内置的点数来源
uint32 GetBaseTalentPointBudget(uint8 botLevel)
{
    return botLevel < 10 ? 0 : uint32(botLevel) - 9;
}
// 额外天赋点：由外部机制提供；来源、发放时机与上限均不在本方案范围内
uint32 GetBonusTalentPoints(uint8 botLevel /*, 预留来源标识 */);
// 生效预算（截断阈值，非拒绝条件）；无 71 硬上限
uint32 GetTalentPointBudget(uint8 botLevel)
{
    return GetBaseTalentPointBudget(botLevel) + GetBonusTalentPoints(botLevel);
}
```

**点数规则的边界**：额外天赋点只抬高“当前可应用点数”，**不得**用于突破天赋自身的 `maxRank`、层级解锁门槛或前置天赋要求。即它是“在合法范围内可以投得更多”，而不是“允许投出非法分配”。若后期需要限制单套构筑规模以防畸形数据，用配置项表达（如 `BotTalentPointBudgetCap`，默认 0 = 不限），该上限触发**拒绝**，与 `A(等级)` 的截断语义不同。

**渐进节奏**：等级基准部分等价于“**每级 1 点**”，逐级回放时每级最多消费 1 点。额外天赋点不经过等级节奏，在应用时直接计入可用预算，可与等级所得混合投出，但仍须满足天赋合法性。

**天赋合法性**：目标天赋必须满足 `Talent.dbc` 的职业/树归属、最大 rank、层级点数要求与前置天赋要求。具体到本地数据结构（见 2.9）：

- `TalentEntry::TalentTab` 经 `TalentTabEntry::ClassMask` 校验职业归属（实测约定为 `1 << (classID - 1)`，且 `BOT_CLASS_*` 的 10 个真实职业值直接等于 classID，**无需额外映射表**）；
- `Row` 决定层级（第 R 层要求该树已投入 `5 × R` 点）；
- `DependsOn` / `DependsOnRank` 为前置校验；
- `RankID[0..4]` 为 rank 上限。

**适用职业范围**：只有 10 个真实职业（`BOT_CLASS_WARRIOR` … `BOT_CLASS_DRUID`，值 1–11）有天赋树；`BOT_CLASS_BM` 及之后的 9 个自定义职业（从 12 起）**没有 TalentTab**，不得进入构筑流程。

**等级跳跃**：Bot 若发生等级跳跃（如 20 直升 60），应补算 21–60 级的全部升级事件或等价逐点回放（每级 1 点），不能在最终等级一次性忽略顺序校验——否则无法发现“序列某个前缀在该等级不合法”这类配置错误。**逐点回放是校验手段（验证序列每个前缀都合法），不是运行时应用流程**：应用时直接取有序序列的前 `A(等级)` 项即可，"取前缀"天然合法，无需模拟过程。校验对象是玩家手编构筑（可能有畸形数据）；系统默认构筑在入库前已完成前缀校验，应用时直接截断。

### 5.2 技能训练

玩家达到技能需求等级后访问训练师学习。模拟无需还原金币、距离与点击，但须保留清晰的学习语义：

- 等级/前置条件以服务器 `trainer_spell` 与 Spell 数据为准，不用“每两级学一次”这类经验概括替代实际数据。
- 升级时计算新增可学技能并记录为可用；**同一技能链只激活合适的最高 rank**，低 rank 不重复进入主动技能选择。现有 `InitSpellMap()`（`bot_ai.cpp:7474`）已实现该 rank 链选择（`info = info->GetNextRankSpell()` 循环），重构应保留这段逻辑，只把“是否可用”与“何时学习”分离。
- 种族、职业、武器/姿态/形态、专精与任务等限制需区分。首期可只覆盖 NPCBot 职业技能与当前 AI 实际支持的技能集。
- 技能循环仍由现有 AI 负责“何时施放”；学习系统负责“是否拥有、当前 rank、是否因专精/天赋/雕文启用”。
- 注意 `_spells` 的键是**首级法术 ID**（`bot_ai.h:866`）。来源标签（训练师/天赋/专精/雕文）须以该首级 ID 为键挂接，才能与既有 `HasSpell()` / `GetSpell()` / `IsSpellReady()` 查询路径兼容。

### 5.3 雕文槽与装备

本项目对 Bot 采用简化规则：**达到 20 级后大/小雕文槽全部开放**。

| 槽位 | Bot 解锁等级 | 数量 |
|---|---:|---:|
| 大雕文槽 1/2/3 | 20 | 3 |
| 小雕文槽 1/2/3 | 20 | 3 |

（玩家侧原规则经源码核实为 15/30/50/70/80 分段位掩码，见 `Player.cpp:13800-13809`；本项目改为 20 级全开，是**主动偏离**，非笔误。）

解锁槽位**不等于**自动拥有或装备雕文。Bot 可把“获得可用雕文集合”与“自动装备默认雕文”视为系统代办，但状态模型须区分：槽位已解锁、候选合法、某雕文已装备、效果已应用。切换专精或构筑时须**先撤销旧雕文效果再应用新方案**，避免光环重复或残留修正。

雕文合法性判定应对齐玩家侧既有实现，而非自造规则：

- **槽位集合**：只有 `GlyphSlot.Order != 0` 的 6 条是玩家槽位，`Id` 为 21–26，槽位索引 = `Order - 1`。其余 4 条（`Order = 0`）不参与。
- **类型匹配**：`GlyphProperties.TypeFlags == GlyphSlot.TypeFlags`（`PlayerStorage.cpp:5932`、`SpellEffects.cpp:4611`）。
- **`TypeFlags` 取值**：`0` = 大雕文，`1` = 小雕文。槽位索引 0/3/5 是大雕文槽（`Id` 21/24/26），1/2/4 是小雕文槽（`Id` 22/23/25）。
- **效果法术**：`GlyphProperties.SpellId`（由 `glyphId` 反查）。
- **异常行自动排除**：`TypeFlags` 存在零星非 0/1 取值（2 / 64 / 128 / 162 各 1–2 条），因槽位 `TypeFlags` 只有 0 和 1，等值比较天然过滤，**无需黑名单**。

用集中判定函数取代散落的等级判断：

```cpp
// 20 级前 0 个，20 级起全部 6 个
uint8 GetUnlockedGlyphSlots(uint8 botLevel)
{
    return botLevel >= BOT_GLYPH_UNLOCK_LEVEL ? BOT_GLYPH_MAX_SLOTS : 0;
}

// 槽位类型：与实际 GlyphSlot 表对齐（不要自造枚举）
enum BotGlyphSlotType : uint8
{
    BOT_GLYPH_TYPE_MAJOR = 0,   // 大雕文，对应槽位索引 0/3/5
    BOT_GLYPH_TYPE_MINOR = 1,   // 小雕文，对应槽位索引 1/2/4
};
```

`BOT_GLYPH_UNLOCK_LEVEL = 20`、`BOT_GLYPH_MAX_SLOTS = 6`（对齐 `MAX_GLYPH_SLOT_INDEX`，`SharedDefines.h:671`）作为集中常量，避免魔法数字散落到职业 AI。

> **装备雕文 ≠ 效果生效。** 本节只解决“哪些雕文能装、装在哪”。装好之后的实际作用方式（光环 / 数值 / 行为），以及**如何替换职业脚本中已有的同名硬编码**，见 3.2 与第 7 章。特别注意：现有雕文效果中有相当一部分（178 处注释中的绝大部分）**根本不走光环**，而是直接以 `crit_chance += N` / `cooldown -= N` 这类形式写在 `ApplyClass*` 函数内，这部分在迁移中必须逐项删除，否则会与新的雕文层双重累加。

### 5.4 应用流程与执行点

**实际执行点**是 `bot_ai::SetStats(bool force)` 的 `if (force)` 块（`bot_ai.cpp:2438-2452`），而不是新增一个独立的等级事件回调。原因见 2.6：Bot 等级变化有多条路径（自然升级回调、自由 Bot 击杀累积、招募重召、装备变更、GM 命令），只有 `SetStats()` 被它们全部覆盖。现有序列：

```cpp
if (me->GetLevel() != mylevel) { ... me->SetLevel(mylevel); force = true; }
if (force)
{
    InitPowers();
    InitSpells(); //this must stay before class passives
    ApplyClassPassives();
    // ... 等级属性写入
}
```

方案在该序列内插入构筑应用步骤，形成：

1. 读取 Bot 职业、**`me->GetLevel()`**、当前专精、owner、构筑版本与已有生效状态。
2. 计算生效天赋点预算 `GetTalentPointBudget(level)` = 等级基准 + 额外天赋点，**不设 71 硬上限**。该值是**截断阈值**：构筑点数超出它不报错，只应用前 `A(level)` 点；雕文槽按 20 级规则一次性全开。
3. 校验生效状态与等级、构筑是否一致；缺失或损坏则进入可重复的重建流程。**复用判据按档分化**（见 5.6）：自动档为 `built_at_level == me->GetLevel()` **且** `applied_points == A(level)`；手动档只看 `applied_points <= A(level)`。
4. 对缺失的等级逐级回放——**必须在 `InitSpells()` 之前完成天赋/雕文决策**，因为 `InitSpells()` 与 `ApplyClassPassives()` 需要读取“哪些技能因天赋/雕文而启用”，且既有注释已规定 `InitSpells()` 必须先于被动：
   - 更新职业技能学习状态与最高可用 rank。
   - **自动档**（系统默认 / 玩家预设）：等级 >= 10 时从加点序列取当前合法的下一点，每级最多消费 1 点，直到用满 `A(level)`。
   - **手动档**（玩家直编）：**不回放、不补点**。多出的预算保持未分配，等玩家自己配置。
   - 额外天赋点不按等级节奏，在应用起点即计入可用预算（自动档会因此自动多点若干项）。
   - 激活新天赋 rank 赋予的技能/被动/光环。
   - 等级达到 20 时一次性装备全部 6 个雕文槽。
5. 结果构建完成后一次性刷新属性、被动、AI 技能表与需重算的冷却/资源，减少中间态闪烁。复用既有的 `InitSpells() → ApplyClassPassives() → InitHeals()` 重建序列，**不要另建一套**。
6. 保存生效状态；重复执行同一等级初始化不得重复叠加光环、重复消费点数或重复插入技能。

> **降级是唯一会被自动截断的情形**（见 5.5 的非对称规则）：预算变小后，两种档位都必须把 `applied_points` 压到 `A(level)`，否则会生效超出预算的点数。

**等级来源统一**：所有新判定以 `me->GetLevel()` 为准；既有 10 处 `master->GetLevel()`（见 2.5）在阶段 B 统一替换，作为独立子任务验证。

**性能**：`SetStats()` 在战斗中也可能被触发（如装备变更），因此构筑应用必须走**快照复用快路径**（命中即返回），避免每次重跑天赋校验。手动档的复用判据刻意放宽（不看等级），正是为了让高频触发的 `SetStats()` 不产生无意义重建。

### 5.5 档位规则：升级时是否自动补点

Bot 30 级时玩家修改了按默认配置的天赋后，**后续升级不再自动应用新点数**；多出的点数交给玩家自己配置，或由玩家主动选择“恢复默认”/“使用预设”。

据此把 `plan_source` 分成两档：

| 档 | `plan_source` | 来源性质 | 升级时行为 |
|---|---|---|---|
| **自动档** | `0` 系统默认 / `1` 玩家预设 | 来源本身是一份**完整方案** | **自动**按新预算向后补齐，新点数按来源序列投入 |
| **手动档** | `2` 玩家直编 | 玩家手改过的**手稿** | **不自动补点**；多出的点数保持**未分配**，等玩家决定 |

分界依据：默认构筑与预设都是“配满的完整方案”，玩家选用它们就意味着接受“按它配满”；而玩家手工调整过之后，系统不应擅自替他往后点，否则会覆盖他的意图。

**未分配点数** = `A(当前等级) - applied_points`（自动档恒为 0；手动档可 > 0）。它不是错误状态，而是“等玩家处置”的正常状态。

手动档下 `applied_points` 的语义因此变为“**玩家确认生效的点数**”，不再等于预算：

- 30 级预算 21 点，玩家只点了 15 点就应用 → `applied_points = 15`，未分配 6 点（天赋本来就可以点不满，合法）。
- 升到 31 级、预算变 22 → `applied_points` **仍是 15**，未分配变为 7 点。

**`applied_points` 的确定规则**（所有写入路径通用）：`applied_points = min(提交序列的点数, A(当前等级))`。提交序列的点数**由玩家自己决定**，不足预算也合法。这一条同时覆盖两种情况：

- 40 级提交一份按 80 级配满的 120 点方案 → `min(120, 31) = 31`，未分配 0；
- 40 级只点了 15 点就应用 → `min(15, 31) = 15`，未分配 16。

两者都进**手动档**（`plan_source = 2`），因为提交内容来自玩家直编。

**三条出口**——未分配点数由玩家决定怎么处理：

| 玩家想做什么 | 操作 | 结果 |
|---|---|---|
| 自己配置多余点数 | 打开界面（提示“X 点未分配”）→ 分配 → `ApplyBotPlan` | 仍为手动档 `2`，`applied_points` 更新 |
| **恢复默认** | `ApplyBotDefaultPlan(bot, spec)` | 回到自动档 `0`，此后升级**自动**应用新点数 |
| **使用预设** | `ApplyBotTalentPreset(bot, spec, presetIndex)` | 转为自动档 `1`，此后按该预设**自动**应用新点数 |

即：**只要回到自动档（默认或预设），自动应用就恢复**；停留在手动档则一直由玩家掌握。

**手动档下“存完整序列”的用途**：UI 可提供“按当前方案补满剩余 X 点”的快捷操作，等价于把 `applied_points` 提升到 `min(plan_total_points, A(等级))` 后再调一次 `ApplyBotPlan`。**这是玩家主动触发的操作，不是自动行为**——玩家也可以只补一部分，或完全不补。

**非对称规则：升级不自动补，但降级必须截断。**

| 等级变化 | 自动档（`0`/`1`） | 手动档（`2`） |
|---|---|---|
| **升级**（预算变大） | 自动向后补齐到 `A(等级)` | **保持 `applied_points` 不变**，差额记为未分配 |
| **降级 / GM 改小**（预算变小） | 截断到 `A(等级)` | **同样必须截断到 `A(等级)`** |

手动档在降级时不能例外：若 `applied_points > A(等级)`，Bot 就会生效超出预算的点数，违反点预算的硬约束。降级截断会削减玩家此前的分配（预算不够，无法避免），UI 应提示“等级下降导致 X 点被撤销”。

### 5.6 读取与回退顺序

Bot 加载/重建时，先看快照可否复用；判据**按档分化**，不可复用再按来源决定：

```text
1. 取 bot 当前专精 spec（bot_ai::GetSpec()）
2. 查 (entry, spec) 快照；不存在 → 跳到第 5 步

   ── 自动档（plan_source = 0 系统默认 / 1 玩家预设）──
3. built_at_level == 当前等级 且 applied_points == A(等级)
      是 → 直接复用该快照（结束）
      否 → 需要重建：按来源重读**完整序列**，自动补齐/截断到 A(等级)
            来源 0 → 表1 npcbot_default_talent_plan
            来源 1 → 表2 该预设；未命中或校验失败 → 记诊断，重置 plan_source = 0，转第 5 步

   ── 手动档（plan_source = 2 玩家直编）──
4. applied_points <= A(等级)
      是 → 直接复用（结束）。**等级变化不触发补点**，差额即“未分配点数”
      否（降级，预算变小）→ 把 applied_points 压到 A(等级) 后复用（结束）
      两种情况都不回默认、也不自动补点

   ── 无快照或来源失效：生成 ──
5. 用 npcbot_default_talent_plan 生成快照（plan_source = 0，进入自动档）
6. 默认构筑也缺失 → 用当前职业 AI 既有行为基线（兼容旧数据）
```

要点：

- **新雇佣或未设置的 Bot 一律用系统默认构筑**，不继承玩家任何偏好。
- **自动档与手动档的复用判据不同**，这是本规则的关键：
  - 自动档：`built_at_level == 当前等级` **且** `applied_points == A(等级)`，两者都满足才复用。
  - 手动档：只要 **`applied_points <= A(等级)`** 就复用。**等级上升不触发重建**——因为手动档不补点，重建也改变不了任何东西，白白重跑一遍；只有降级（`applied_points > A(等级)`）才需要截断。
- **手动档下 `applied_points != A(等级)` 是正常状态**（未分配点数），不能当作“快照失效”的信号。若沿用自动档的判据，每次升级都会触发一次无意义的重建。
- **只有 `plan_source = 1` 才去读玩家预设表**；`= 2` 的构筑内容就在快照自身，且**不会因为等级变化而丢失尚未生效的尾部点数**（因为存的是完整序列）。
- **`built_at_level` 在手动档下只用于审计/显示**（记录玩家最后一次应用的等级），**不参与重建决策**。
- **`spec` 决定读哪一系**：同一 Bot 不同专精各有一行快照，当前是防护系就读防护系。
- **自动档的复用判据不能只看等级**：`A(L) = (L-9) + 额外天赋点`，额外天赋点可能在等级不变的情况下变化（外部机制追加）。故需 `built_at_level == 当前等级` **且** `applied_points == 当前 A(等级)`。若只比等级，追加额外点数后快照会被错误复用，表现为“外部加了点数但 Bot 没有任何变化”。
- **解雇删除快照行**：`DELETE FROM characters_npcbot_applied_plan WHERE entry = ?`，下次雇佣回到系统默认。两条解雇路径都要处理（见 2.8）。

### 5.7 构筑不合法时的回退

先区分两类“不合法”，处理方式不同：

| 类型 | 例子 | 处理 |
|---|---|---|
| **超出当前预算**（非错误） | 80 级设计的 120 点构筑用在 40 级 Bot 上 | **截断**：只应用前 `A(level)` 点，其余标记未生效，不报错。**自动档**在升级后自动补齐；**手动档**保留差额为“未分配点数”，交由玩家处置 |
| **结构性非法**（配置错误） | 超 `maxRank`、层级未解锁、前置未满足、职业树不匹配、`talentId` 不存在 | **拒绝**：按下列流程处理，不静默越级分配 |

结构性非法的回退流程：

1. 记录诊断：职业、专精、等级、Talent ID、rank、缺失条件。
2. 跳过该点并尝试同级下一个合法候选；候选耗尽则保留未花点数。
3. 开发/测试环境可将配置错误升级为错误日志或启动校验失败；生产运行时避免因此导致 Bot 无法生成。
4. 不私自选择与专精无关的天赋作为回退，否则会掩盖配置缺陷。

---

## 6. 玩家自定义构筑（第二阶段）

### 6.1 需求与边界

- 玩家可为自己雇佣的 Bot 设置每系天赋构筑。
- 预设**绑定玩家角色 + Bot 职业**（`owner + bot_class + spec`）：玩家换同职业的 Bot 时可用同一套预设。
- Bot 当前实际生效的天赋/雕文**绑定 `entry`**；被解雇（`owner` 归零、`IAmFree()`）后清空并恢复默认。
- **新雇佣的 Bot 一律使用系统默认构筑**；只有玩家**显式设置**后才改用自定义预设。不存在“继承玩家某套偏好”这种隐式行为。
- 玩家操作分**两种，互相独立**：**应用**——把界面上当前的编辑内容直接覆盖该 Bot 的生效构筑，**不写预设库**，属于临场调整；**保存为预设**——只把内容存入预设库，**不改变任何 Bot**。UI 上对应两个独立按钮，可单独执行也可组合执行。
- **玩家可以把 Bot 恢复为系统默认**（`ApplyBotDefaultPlan`）或**改用某套已存预设**（`ApplyBotTalentPreset`）。这两条是把 Bot 从“手动档”拉回“自动档”的出口，属于**正常的玩家操作**，不是仅供内部使用的兜底逻辑。
- **玩家可以导出构筑为分享码（或模拟器链接）并导入别人的分享码**，见 6.6。导出/导入是**纯数据转换、不写库**；导入只填充编辑区，仍需玩家显式“应用”或“保存为预设”。
- **升级时是否自动应用新点数取决于当前档位**（见 5.5）。
- 客户端天赋树 UI 由 `mod-ale`（AzerothCore Lua Engine）实现，**不在本方案内**。服务端负责数据模型、校验、应用/撤销、查询接口与事件钩子。
- 不模拟经济/社交：不要求洗点费用或雕文物品来源，但设置需显式动作（如 UI 提交），不自动洗点。

### 6.2 应用与保存预设

**“应用”与“保存为预设”是两个独立操作，不是一步。**

| 操作 | 作用范围 | 写哪张表 | 对 Bot 的影响 |
|---|---|---|---|
| **应用** | 仅当前这一个 Bot | 只写 `characters_npcbot_applied_plan` | 立即生效；`plan_source = 2`、`preset_ref = 0` |
| **保存为预设** | 玩家的预设库 | 只写 `characters_npcbot_talent_plan` | **不改变任何 Bot 的生效状态** |

典型用法是两者组合（改几项 → 点应用看效果 → 满意后另存为预设），但服务端把它们当作两次独立调用：

```text
ApplyBotPlan(bot, spec, talentData, glyphData)                          -- 应用：只影响这一个 Bot
SaveBotTalentPreset(owner, botClass, spec, presetIndex, talentData, glyphData, name)  -- 存预设：不影响任何 Bot
```

四种组合都合法，UI 上应是两个独立按钮：

| 玩家操作 | 该 Bot | 预设库 |
|---|---|---|
| 只应用不保存 | 生效（`plan_source = 2`） | 无变化 |
| 只保存不应用 | **完全不变** | 多一套预设 |
| 先应用再保存 | 生效（`plan_source = 2`） | 多一套预设 |
| 保存后对 Bot 应用该预设（`ApplyBotTalentPreset`） | 生效（`plan_source = 1`） | 多一套预设 |

要点：

- **“应用”是临场调整**：玩家点开界面、改几项、点应用，改动直接落到该 Bot。这份构筑**不写预设库**，快照是它的唯一副本，因此重建时按快照自身重新截断（见 5.6 第 3 步）。
- **“保存为预设”是单独操作**：把界面上当前的内容写入预设库并编号 `preset_index`，**不自动应用到任何 Bot**；保存完仍需（可选地）对某个 Bot 执行一次应用。
- **一次应用 = 一次“洗点 + 逐级重分配”**：撤销该 Bot 当前构筑的 Talent/Glyph 效果 → 校验 → `applied_points = min(提交点数, A(等级))` → 重放 → 覆盖生效快照。
- **`plan_source` 由操作方式决定，不由内容决定**：即使玩家应用的恰好与某套预设内容完全相同，只要他是通过“应用”提交的，`plan_source` 就是 2、`preset_ref` 就是 0；只有走 `ApplyBotTalentPreset` 才是 1。**不要反查内容去匹配预设**——那样“来源”就不再反映玩家的真实意图，UI 溯源会撒谎。
- **`plan_source = 2` 的构筑在解雇后丢失**：没有第二份副本。玩家若要保留须另存为预设；UI 建议在“应用”成功后提示可“保存为预设”。
- **保存的预设与应用的构筑都是完整方案，不因等级截断而裁剪**：玩家在 40 级环境下编辑时可以看到/提交超过 31 点的完整方案（例如按 80 级配满 120 点），**应用与保存走同一份校验逻辑，都接受完整方案**。系统只在实际生效时按 `A(当前等级)` 截取前若干点，UI 侧标注“已生效 X / 共 Y，剩余需提升等级或增加额外天赋点”。这样玩家不必为了低等级 Bot 而重做一份降级方案。
- **超出预算不是错误，不拒绝**：`A(等级)` 只决定应用多少点。真正的拒绝条件只有结构性非法。
- 结构性非法时**整体拒绝并返回错误码，绝不部分应用**，也不改动该 Bot 现有快照——避免留下“一半新方案一半旧方案”的中间态。
- **同一套预设可应用于多个 Bot**：对另一个同职业 Bot 执行设置并选择同一套预设即可，两者各自持有独立快照。
- **删除预设只影响 `plan_source = 1` 的 Bot**：那些 Bot 的生效值不受影响（在快照里），仅把 `plan_source` / `preset_ref` 重置为 `0`，下次重建时回到系统默认构筑。`plan_source = 0` 与 `= 2` 的 Bot 与预设库无关。
- 操作需满足安全约束：Bot 不在战斗中、不属于进行中的实例关键流程、无待落地的 `_newspec`。可参照 `SwitchSpec()` 的非阻塞判定习惯。

### 6.3 与专精机制的关系

现有实现中，`spec` 持久化在 `characters_npcbot.spec`，切换由 `bot_ai::SwitchSpec()` 施放 `ACTIVATE_SPEC`，在 `OnBotSpellGo()` 中落地为 `SetSpec(_newspec)` 并重建技能、被动与治疗；`GetSpec()` 在 10 级以下固定返回 `BOT_SPEC_DEFAULT`。

第二阶段不改变这套语义，只在其上叠加构筑：

- `spec` 仍是“当前专精”，是 AI 行为分支依据，也是生效快照的分区键。
- 构筑表示“该专精下天赋点与雕文怎么分配”，是技能/被动/雕文效果的来源。
- 玩家改构筑**不等于**换专精；换专精后该系若尚未设置过，回到系统默认构筑，玩家可再通过 UI 设置。
- 同一 Bot 在不同专精下**各有一行生效快照**，三系互不覆盖。
- 10 级以下保留默认行为：使用默认构筑，不加载玩家预设。
- 解雇清空该 entry 的生效快照；玩家预设按 `owner + 职业` 存储，**不随解雇丢失**（便于复用到新雇佣的同职业 Bot）。

### 6.4 服务端接口

三处职责不同，查询不要合并：**默认构筑**（world 静态数据）、**玩家预设**（owner + bot_class + spec + preset）、**生效快照**（entry + spec）。

**写入路径也是两条，互相独立**：**应用**只写生效快照、立即影响该 Bot；**保存为预设**只写预设库、不影响任何 Bot。**不要把两者做成一个“保存并应用”的复合接口。**

| 能力 | 建议形态 | 用途 |
|---|---|---|
| 查询天赋树 | `GetBotTalentTreeForClassSpec(botClass, spec)` 返回节点 + 层级 + 前置 + 最大 rank | 客户端渲染；数据来自服务器 Talent 数据 |
| 查询默认构筑 | `GetDefaultBotTalentPlan(botClass, spec)` | UI 展示/对比“系统默认”，也是未设置时的来源 |
| 列举预设 | `ListBotTalentPresets(owner, botClass, spec)` | UI 列出该系已存哪几套，供玩家选择应用 |
| 查询单个预设 | `GetBotTalentPreset(owner, botClass, spec, presetIndex)` | UI 回显某套内容 |
| 校验方案 | `ValidateBotTalentPlan(botClass, spec, plan)` 返回错误码与原因 | 只校验**结构性非法**（职业树/`maxRank`/层级/前置/`talentId` 存在性）。**点数超预算与超 71 都不是失败条件**，预算只用于截断 |
| 查询点预算 | `GetTalentPointBudget(bot)` / `GetBaseTalentPointBudget(level)` / `GetBonusTalentPoints(bot)` | UI 显示“当前可用 / 已用 / 构筑总数”；额外来源为外部集成点 |
| **载入编辑起点** | `GetBotPlanForEditing(bot, spec)` | UI 打开界面时取“当前生效构筑的**完整序列**”作为编辑内容；无快照时回退系统默认。同时返回 `plan_source`（显示“当前来自：系统默认 / 预设 N / 手动调整”）与 **`unassigned_points`** |
| **应用（UI 主入口）** | `ApplyBotPlan(bot, spec, talentData, glyphData)` | **玩家在界面上改完点“应用”所走的唯一入口**：校验 → `applied_points = min(提交点数, A(等级))` → 重放 → 覆盖生效快照（`plan_source = 2`、`preset_ref = 0`）。**不写预设库**。**应用后即进入手动档：此后升级不再自动补点** |
| 应用（仅改雕文） | `ApplyBotGlyphPlan(bot, spec, glyphData)` | 只换雕文不改天赋的便捷入口；内部委托 `ApplyBotPlan`，天赋沿用当前生效的完整序列 |
| 保存预设 | `SaveBotTalentPreset(owner, botClass, spec, presetIndex, talentData, glyphData, name)` | **独立操作**：新增/覆盖一套预设，写**完整**构筑、不做等级裁剪、**不应用到任何 Bot** |
| 删除预设 | `DeleteBotTalentPreset(owner, botClass, spec, presetIndex)` | 仅删预设；**只对 `plan_source = 1` 的 Bot** 重置 `plan_source`/`preset_ref` 为 0 |
| 应用预设到某 Bot | `ApplyBotTalentPreset(bot, spec, presetIndex)` | 把已保存的预设应用到该 Bot：`plan_source = 1`、`preset_ref = presetIndex`，按当前预算截断。**转为自动档：此后升级按该预设自动应用新点数** |
| 应用系统默认到某 Bot | `ApplyBotDefaultPlan(bot, spec)` | 让该 Bot 该系回到系统默认构筑（`plan_source = 0`）→ **回到自动档，之后升级自动补点**。这是玩家的正常操作之一 |
| 查询未分配点数 | `GetBotPlanForEditing` / `GetBotAppliedTalentPlan` 返回 `unassigned_points = A(等级) - applied_points` | 自动档恒为 0；手动档 > 0 时表示“玩家还有 X 点没分配”，UI 应提示可走三条出口 |
| 查询生效快照 | `GetBotAppliedTalentPlan(bot, spec)` | 返回来源（`plan_source` / `preset_ref`）、完整序列、`applied_points` / `plan_total_points` 与 `unassigned_points`；**生效部分 = 前 `applied_points` 项** |
| **查询效果状态（职业脚本用）** | `HasBotTalentRank(bot, talentId)` / `HasBotGlyphEquipped(bot, glyphId, slotType)` / `GetBotGlyphEffectValue(bot, glyphId, 效果类型)` | **职业脚本改造后访问天赋/雕文状态的唯一入口**：第 7 章三类载体的替换都调用它，避免各职业文件自行解析构筑数据 |
| 重算生效快照 | `ReconcileBotProgression(bot)` | 等级变化、雇佣、专精切换、解雇、显式刷新时调用 |
| 查询雕文 | `GetBotGlyphSlots(level)` / `GetBotAppliedGlyphPlan(bot, spec)` | 返回槽位与已装备雕文 |
| 变更事件 | 预设/快照变更后广播回调 | 供脚本、命令、UI 刷新，避免轮询 |

关键语义：

- **让 Bot 用上自定义构筑有两个入口，语义不同**：`ApplyBotPlan`（UI 直接编辑后点“应用”，`plan_source = 2`，不落预设库）与 `ApplyBotTalentPreset`（应用已保存的预设，`plan_source = 1`）。两者都作用于单个 Bot、都按当前预算截断、都可被 `ApplyBotDefaultPlan` 覆盖回默认。
- **应用与保存预设互不调用**：`ApplyBotPlan` 只写生效快照，`SaveBotTalentPreset` 只写预设库。两者接受的都是**完整**构筑、都不受当前等级预算裁剪。UI 上必须是两个独立按钮。
- **不要提供“保存并应用”的复合接口**：一旦合并，就无法表达“只应用不保存”这一最常用的临场调整语义，而 UI 已经天然是两次动作。
- **自动档与手动档的升级行为不同**：`plan_source = 0/1` 升级时**自动**按新预算补齐；`= 2` **不自动补点**，多出的点数记为 `unassigned_points`。两者的**降级**都会被自动截断。任何新增的写入路径都必须先判定这一点，**不能一律按“自动补齐”实现**。
- **预算只截断、不拒绝**：任何写入路径都不因“点数超过当前预算”而失败。
- **删除预设不打断运行中的 Bot**：仅对 `plan_source = 1` 者重置 `plan_source`/`preset_ref`，下次重建才回到系统默认；`plan_source = 0/2` 的 Bot 与预设库无关，不受影响。
- **解雇删除生效快照**：在 `botdatamgr.cpp` 的 `NPCBOT_UPDATE_OWNER`（:3159）与 `NPCBOT_UPDATE_ERASE`（:3136）两处都执行删除。前者与已有的 `CHAR_DEL_NPCBOT_TRANSMOG_ALL` 清理放在同一事务，注意子查询须在 `UPDATE owner` 之前求值（见 2.8）。
- **职业脚本不再自行判断等级与专精**：三类载体迁移后，一律通过上表的“查询效果状态”接口获取天赋 rank 与雕文装备情况。这些接口是只读的、可 O(1) 命中的快路径，允许在战斗中被高频调用（`ApplyClass*` 修正函数每次伤害结算都会执行）。

**实现位置**：

- 数据访问与校验放 `BotDataMgr`（已承担 `NpcBotData` 加载/更新）；默认构筑在启动阶段随静态数据载入内存。
- 应用/撤销编排放 `bot_ai`（已负责 `InitSpells()`/`ApplyClassPassives()`/`InitHeals()` 的重建序列，见 `bot_ai.cpp:16050-16052`）。**统一入口 `ReconcileBotProgression()` 挂在 `SetStats()` 的 `if (force)` 块内**，而不是新增独立的等级回调（见 2.6）。
- Lua 接口走 `mod-ale` 已有注册机制：在 `RegisterFunctions()`（`modules/mod-ale/src/LuaEngine/LuaFunctions.cpp:1898`）中按 `ALE*::Register` / `SetMethods` 模式注册为独立函数表，**不让 Lua 直接读数据库**。
- 也可按现有模块钩子风格暴露给 C++ 脚本（参考 `src/server/game/Scripting/ScriptDefines/*.h`，等级变化钩子为 `PlayerScript::OnPlayerLevelChanged`，`PlayerScript.h:261`），使同一套接口既能被 Lua 调用，也能被命令/脚本调用。

### 6.5 权限与安全

- **所有写入路径都须校验归属**：`ApplyBotPlan`（直编应用）、`ApplyBotGlyphPlan`、`ApplyBotTalentPreset`、`ApplyBotDefaultPlan` 都须确认调用者是该 Bot 的 **owner（或 `shared_owners` 中的玩家）**，不能仅凭 entry 写入。保存/删除预设则校验 `owner` 本人（预设按 owner 存储）。**直编应用同样要校验**——它是能立即改变 Bot 战斗数值的入口，不能因为“没入库”就放宽。
- 预设按 owner 存储，同一玩家的某职业某系预设对其实全局共享，可复用到多个同职业 Bot；不同玩家互不可见。
- **预设数量需上限**（如每系最多 5–10 套），`preset_index` 范围固定并校验。
- 数据体量需上限（条数、字符串长度、rank 范围），防止畸形数据导致解析开销或越界。
- 服务端**必须**完整校验，不信任客户端传入的点数与前置关系。
- 并发提交同一主键时以最后一次成功写入为准，保证内存与数据库一致；失败保持旧状态。
- 多 shared owner 场景下，预设以**实际 owner** 为准；建议先限制为 owner 本人可改。

### 6.6 构筑的导出与导入（分享码）

#### 6.6.1 目的

玩家在 UI 上配好一套天赋+雕文后，应当能**导出一串紧凑的分享码**（发群、贴论坛、抄给朋友），也能**把别人的分享码粘回来直接应用**。同时提供 `80.wowdb.cc` / `wowhead` 模拟器链接，便于与攻略站对照。

**关键取舍：分享码不自造格式，直接复用 WotLK 模拟器已有的 URL 编码规则。** 理由见 6.6.2。

#### 6.6.2 为什么复用模拟器编码

| 方案 | 评价 |
|---|---|
| 自造 JSON / Base64 | ✗ 玩家拿去模拟器（`80.wowdb.cc`、wowhead、evowow）**打不开**，无法对照攻略；也没有现成的第三方校验工具 |
| 直接复用模拟器 URL 编码 | ✓ 玩家可原样粘贴到模拟器核对；编码算法公开且已被多方独立实现；本方案只需实现**解码 + 校验**，编码端已有现成实现（见 6.6.8） |
| 只给 JSON 导出 | ✗ 过长且不可读，分享体验差；但**仍保留**作为调试/备份格式（见 6.6.6） |

选定的编码规则与 `80.wowdb.cc`、`wowhead`、`wotlk.evowow.com` 同源（均出自同一份 `TalentCalc.js`），因此**同一串码在这些站点通用**，不存在绑定单一站点的问题。

#### 6.6.3 编码格式

```text
https://80.wowdb.cc/?talent#<build>[:<glyphs>]
```

| 段 | 内容 |
|---|---|
| `<build>` | 职业前缀 + 三棵树依次编码 |
| `:` | 天赋段与雕文段的分隔符（**必须**，见 6.6.5） |
| `<glyphs>` | 大雕文组 + 小雕文组依次编码 |

**天赋段编码**：

1. 编码表 `0zMcmVokRsaqbdrfwihuGINALpTjnyxtgevElBCDFHJKOPQSUWXYZ123456789`（62 字符）。
2. 职业前缀 = `ENCODING[oldId * 3]`，`oldId` 映射：德鲁伊 0 / 猎人 1 / 法师 2 / 圣骑士 3 / 牧师 4 / **盗贼 5** / 萨满 6 / 术士 7 / 战士 8 / 死骑 9。
3. 每棵树：把各天赋 rank（含 0）按**槽位顺序**拼成数字串（每个天赋占 1 位）→ 去掉尾部 `0` → 若长度为奇数则补 `0`（记录补齐后位数 `i`）→ 每 2 位一组 `(d0,d1)` 编码为 `ENCODING[d0*6 + d1]`。
4. 树尾：若 `i < 该树槽位总数`，追加树分隔符 `Z`。
5. 三棵树依次拼接，整个 `<build>` 末尾去掉尾部 `Z`。

**槽位顺序** = `Talent.dbc` 中按 `Row` 升序、同 `Row` 内 `Col` 升序。这是**解码的关键**：码里只有"第 N 个天赋点了几点"，没有 talentId，必须靠槽位表反查。

**雕文段编码**：

1. 按 `_glyphOrder = [1, 2]` 分组（1 = 大雕文、2 = 小雕文），组内按槽位顺序。
2. 每个雕文贡献 1 个字符 `ENCODING[glyphIndex]`（`glyphIndex` 为雕文在站点 `g_glyphs` 表中的既定索引，非物品 ID）。
3. 每组按槽位容量（大 3 / 小 3）判断是否补 `Z`；雕文段末尾去掉尾部 `Z`。

**雕文槽位**：`0/3/5` = 大雕文，`1/2/4` = 小雕文。

#### 6.6.4 两种导出形态

UI 提供两个动作，语义不同：

| 动作 | 输出 | 用途 |
|---|---|---|
| **导出分享码** | `f0ecojgoVboIuVo0xV0hZxb:TIpVzc`（纯码，不含 URL） | 短、可贴任意聊天/论坛；导入时直接粘码 |
| **导出模拟器链接** | `https://80.wowdb.cc/?talent#f0ecojgoVboIuVo0xV0hZxb:TIpVzc` | 可点击、可在攻略站对照；导入时**也接受**（自动剥离前缀） |

导入端**两种都接受**，解析时按以下顺序容错：

```text
1. 若含 '#'，取 '#' 之后的部分
2. 若形如 '?talent#...' 的完整 URL，先取 hash
3. 剩余部分按 ':' 拆分 → build / glyphs 两段
4. 若 ':' 前还有非编码表字符（如站点域名残留），丢弃到最后一个非编码字符之后
```

**必须支持"粘贴模拟器链接"这一最常见的导入路径**——玩家从攻略站复制链接是主要来源。

#### 6.6.5 实现时必须遵守的三条约束

这三条是编码规则的**硬约束**，实现时踩过坑（详见姊妹文档 `NPCBot盗贼默认构筑与脚本改造台账.md` 3.3 节）：

1. **雕文段必须用 `:` 分隔，不能直接尾接在天赋段后面。** 站点的 hash 解析是 `location.hash.substr(1).split(':')` → `pounds[0]` = build、`pounds[1]` = glyphs。直接拼接会让解析器把雕文字符当成天赋数据吃掉，表现为"树点数丢失 / hash 被截断"。
2. **树尾判据 `i < slot_count` 比较的是"补齐后的数字位数"，不是编码字符数。** 写成字符数会多输出一个 `Z`。
3. **空树也必须输出 `Z`**（因 `0 < slot_count`）。写成"空串直接 return"会让后续树错位。

#### 6.6.6 导入的校验与失败处理

**解码出的是"槽位 → rank"向量，不是 talentId 序列**，因此导入必须经过两层转换与校验：

```text
分享码
  → 解码为 槽位向量 (per-tree ranks) + 雕文槽位列表
  → 【转换】按槽位表反查 talentId / glyphId
  → 【校验】调用 ValidateBotTalentPlan()（见 6.4）
  → 【入库】走与 UI 提交完全相同的写入路径
```

要点：

- **校验逻辑必须与 UI 提交共用**。导入不是特权路径，`ApplyBotPlan` / `SaveBotTalentPreset` 接受什么，导入就接受什么，**不要写第二套校验**。
- **职业不匹配直接拒绝**，并明确报错。分享码的职业前缀决定 classId，若与目标 Bot 职业不同则拒绝（不做"尽力适配"）。
- **槽位数不匹配按结构性非法处理**。客户端 DBC 版本不一致时槽位数可能变化，此时槽位→talentId 的反查会错位，必须拒绝而不是硬套。**这是导入路径最需要防御的点。**
- **雕文类型不匹配即拒绝**（大雕文放进小雕文槽），校验规则照抄玩家侧 `GlyphProperties.TypeFlags == GlyphSlot.TypeFlags`。
- **未知雕文索引 / 未知职业前缀 / 编码表外字符** 一律拒绝并返回可读原因，不静默忽略。
- **超预算不是失败条件**（与 6.4 一致）：导入的构筑可以是完整方案，按 `A(当前等级)` 截断生效，UI 标注"已生效 X / 共 Y"。
- **导入后不自动入库为预设**：与"应用"/"保存为预设"的既有语义保持一致——导入先填充**编辑区**，玩家再决定走哪个动作。即：**导入 ≠ 应用 ≠ 保存**，三者是三个独立步骤。
- 长度上限：分享码长度设上限（如 512 字符）防畸形输入。

#### 6.6.7 调试格式（可选保留）

除分享码外，建议保留一个**可读的调试/备份格式**，用于排障与版本迁移：

```text
潜行者/刺杀 71点
天赋: 0:5 3:3 5:3 ...        # 树:槽位:rank
雕文: 0:45768 3:45761 ...     # 槽位:物品ID
```

该格式**不用于玩家间分享**（太长），只在日志、导入失败诊断、跨版本备份时使用。**解析器必须与分享码共用同一套校验与写入路径**，不得另建特权入口。

#### 6.6.8 服务端接口

导出与导入分列，均与 6.4 的写入路径复用：

| 能力 | 建议形态 | 用途 |
|---|---|---|
| **导出分享码** | `ExportBotPlanCode(botClass, spec, talentData, glyphData)` → `string` | 把当前编辑区/生效构筑编码为分享码（不含 URL） |
| **导出模拟器链接** | `ExportBotPlanUrl(botClass, spec, talentData, glyphData)` → `string` | 拼上 `https://80.wowdb.cc/?talent#` 前缀 |
| **导入分享码** | `ImportBotPlanCode(code)` → `{botClass, spec, talentData, glyphData}` 或错误码 | **只做解码 + 槽位反查**，不写库。结果交给 UI 填充编辑区 |
| **导入并校验** | `ImportBotPlanCode(code, expectBotClass)` | 额外校验职业匹配 + 槽位数匹配，返回校验结果 |

关键语义：

- **导出/导入是纯函数式转换**，不触碰数据库，也不产生副作用。`ImportBotPlanCode` 返回的只是数据，**入库仍需 UI 显式调用 `ApplyBotPlan` / `SaveBotTalentPreset`**。这样"导入"就不会成为一个绕过校验的后门。
- **编码/解码实现在服务端**，不放在 Lua/UI 层。原因：解码需要**槽位表**（`Talent.dbc` 的 `Row/Col` 顺序）与**雕文索引表**，这些是服务端已经加载的 DBC 数据；放客户端会导致两套槽位表漂移。UI 只负责显示与剪贴板。
- **槽位表与雕文索引表须与站点版本一致**。这两张表是解码的"密钥"，客户端 DBC 更换后必须重新确认。建议在启动时校验一次（如断言盗贼三树槽位数 = 27/28/28），不一致则**禁用导入功能并记警告**，而不是带病解码。
- 已有现成的编码/解码参考实现可直接对照：`tools/talent_url.py`（Python，支持全部 10 个职业，含 829 个天赋与 345 条雕文的槽位/索引表），配套数据 `tools/talent_data.json` 与 `tools/glyph_data.json`。**该工具可作为服务端实现的对照基准与回归测试数据源**——同一份构筑，工具生成的码与 C++ 生成的码必须逐字符相同。
- 分享码内**不含 owner / entry 信息**，因此天然可跨玩家、跨 Bot 分享，不存在越权读取问题；但**导入后的写入仍需通过 6.5 的归属校验**。

#### 6.6.9 与消息/聊天的关系

若后续要支持"在游戏内聊天频道直接分享码"：

- 分享码是纯 ASCII（编码表 62 字符 + `:` + `Z`），**不含空格与引号**，可直接作为普通聊天文本发送，无需转义。
- 反之，从聊天中提取分享码需要**锚定格式**（如 `[NPCBOT:xxxxx]`），避免误匹配普通文本。建议定义一个明确的包裹标记，而不是靠"看起来像"来识别。
- 这部分属**可选增强**，不在第二阶段必需范围内。

---

## 7. 效果迁移

**迁移范围明确包含职业脚本本身。** 本章规定如何把现有硬编码逐项替换为构筑驱动，是原则一的具体执行方法。

### 7.1 三类载体的替换形态

| 载体 | 现状形态 | 替换后的形态 |
|---|---|---|
| **A. 光环类**：`ApplyClassPassives()` 内的 `RefreshAura(spellId, 条件 ? 1 : 0)` | 触发条件是等级/专精字面判断 | 条件改为“构筑中该 talent 的 rank / 该 glyph 是否装备”，`RefreshAura` 调用本身保留 |
| **B. 数值类**：`ApplyClass*` 修正函数内的 `crit_chance += N`、`pctbonus *= x`、`cooldown -= N`、`cost -= N` | 数值是写死的魔法数字，仅靠 `//Glyph of ...` 注释标识来源 | 数值改为从“生效构筑 + DBC/配置”反查得到，**并删除原语句** |
| **C. 行为类**：AI 决策分支内的等级/专精条件 | 直接 `if (等级条件)` 切换技能优先级或目标选择 | 改为查询“该 talent/glyph 是否在生效构筑中” |

A 类现状示例：

```cpp
// bot_warrior_ai.cpp:2043-2045 —— 天赋 Deep Wounds 按 rank 用不同 spell
RefreshAura(DEEP_WOUNDS_3, (isArms || isFury) && level >= 24 ? 1 : 0);
RefreshAura(DEEP_WOUNDS_2, (isArms || isFury) && level >= 23 && level < 24 ? 1 : 0);
RefreshAura(DEEP_WOUNDS_1, (isArms || isFury) && level >= 22 && level < 23 ? 1 : 0);
// bot_warrior_ai.cpp:2085-2090 —— 雕文直接以等级门槛硬编码
RefreshAura(GLYPH_HEROIC_STRIKE, level >= 15 ? 1 : 0);
RefreshAura(GLYPH_REVENGE,       level >= 15 ? 1 : 0);
RefreshAura(GLYPH_EXECUTION,     level >= 24 ? 1 : 0);
RefreshAura(GLYPH_BLOCKING,      level >= 40 ? 1 : 0);
```

B 类现状示例——**同一文件里，这个效果在 A 类中完全不存在，只活在数值修正里，连常量都没有**：

```cpp
// bot_warrior_ai.cpp:1268-1270   （注释原文拼写即为 "Glypg"，非本文笔误）
//Glypg of Victory Rush: 30% additional critical chance for Victory Rush
if (lvl >= 15 && baseId == VICTORY_RUSH_1)
    crit_chance += 30.f;
```

替换后的形态——三类载体统一走同一个构筑查询入口：

```cpp
// A 类：条件来自构筑，不再写等级
RefreshAura(GLYPH_HEROIC_STRIKE, HasGlyphEquipped(GLYPH_SLOT_MAJOR_1) ? 1 : 0);

// B 类：数值来自构筑反查，函数内不再有等级/专精字面量
if (HasGlyphEquipped(GLYPH_VICTORY_RUSH))
    crit_chance += GetGlyphEffectValue(GLYPH_VICTORY_RUSH, EFFECT_CRIT_BONUS);

// C 类：行为分支改为构筑判定
if (HasTalentRank(TALENT_SWORD_AND_BOARD))
    /* 按构筑调整技能优先级 */;
```

> 上述代码块中的 `HasGlyphEquipped()` / `HasTalentRank()` / `GetGlyphEffectValue()` 是**形态示意**，用于说明“条件与数值都改由构筑提供”这一结构变化；具体接口名与签名见 6.4。

### 7.2 迁移的固定四步

每项效果独立执行，**四步在同一批次内完成**：

1. **登记**：在台账中记录该效果在 3 类载体中的全部出现位置。
2. **建立**：让效果层能按生效构筑正确提供该效果（光环 / 数值 / 行为）。
3. **验证等价**：同一等级、专精、构筑下新旧行为对比一致，差异须有明确理由。
4. **删除**：删除该效果的全部旧硬编码语句，并复查无残留。

> **禁止“先加新层、旧硬编码以后再清”的分期做法。** 这与常规重构“先并行再切换”的经验相反，原因是 B 类残留不会互相覆盖、而是**双重累加**（见 7.3）：一旦并行就立即超模，且不报错、不崩溃，只表现为数值偏高，事后极难定位是哪些项在叠加。

### 7.3 两类残留的后果不同

并行后果取决于载体类型，二者差别很大，直接决定迁移策略：

| | A 类（光环） | B/C 类（数值 / 行为） |
|---|---|---|
| 冲突机制 | `RefreshAura()` 内部先 `RemoveAurasDueToSpell` 再 `AddAura()`，**后调用者覆盖前者** | `crit_chance += N`、`pctbonus *= x` **累加/累乘，两处都生效** |
| 并行后果 | 效果**互相打架**：时有时无、在高等级莫名消失 | **双重累加**：直接超模 |
| 可察觉性 | 低——表现为“效果不稳定”，难归因 | 中——表现为“数值偏高”，但不知偏高在哪几项 |
| 是否报错 | 否 | 否 |
| 迁移要求 | 可以短暂并行（结果不稳定但不超模），但仍须尽快删除 | **绝不能并行**，必须先删后用 |

因此迁移的严格程度按载体区分：

- **A 类**：允许“先建后拆”，但两者之间的窗口必须尽可能短，且该窗口内**不得发布**。
- **B/C 类**：**先删除旧实现、再由新层提供**（或在同一提交内完成替换），不允许任何时刻两处同时生效。

> 这条区分也解释了为什么“逐项迁移 + 每项独立验证”比“整体重写”更可执行：B 类共 178 处雕文注释 + 大量天赋数值，一次性重写的回归面无法控制；而逐项迁移时，每项只有 1–3 个出现位置，删除是否完整可以精确核对。

### 7.4 已有的中间态先例

`bot_rogue_ai.cpp:194-296` 已把散落的 `RefreshAura(..., level/spec 条件)` 收敛为数据表 `RogueTalentDef RogueTalents[]`（`{spec, minLvl, maxLvl, auraId}`），`ApplyClassPassives()` 只遍历该表（`:2387-2389`），不再逐行写条件。这是**半程**，后续应继续推进：

| 盗贼当前做法 | 目标做法 |
|---|---|
| `uint8 minLvl / maxLvl` 等级半开区间 | 换成 `talentId:rank`，由生效构筑提供 |
| 天赋与雕文混在同一张表（如 `{ROGUE_ANY_SPEC, 15, 0, GLYPH_BACKSTAB}`） | 拆为天赋表与雕文表，雕文项由槽位状态驱动 |
| 仍用 `master->GetLevel()`（`:2383`） | 改 `me->GetLevel()`（见 2.5） |
| 只覆盖 A 类载体 | 把该职业的 B/C 类一并纳入台账 |

值得注意：`maxLvl` 半开区间（如 `SEAL_FATE1` 在 `[35,37)`、`SEAL_FATE2` 在 `[37,39)`）本质上是用**等级区间近似模拟天赋 rank**。改用 `talentId:rank` 后，这类区间判断可以整体消失，`GLYPH_BACKSTAB` 混在天赋表里的问题也随之解决。

### 7.5 效果台账

**阶段 A 的核心产出是一份台账**，而不是“`GLYPH_*` 常量清单”。

| 字段 | 说明 |
|---|---|
| 效果名称 | 如 `Poleaxe Specialization`、`Glyph of Victory Rush` |
| 来源类型 | Talent / Glyph / 套装 / 技能自带（后两类不在本次迁移范围） |
| Talent ID 或 Glyph ID | 天赋项附目标 rank；雕文项附大/小类型与槽位 |
| 出现位置 | 全部命中位置，格式 `文件:行号`；可能 0–3 处（每类载体至多一处） |
| 目标路径 | 1 / 2 / 3（见 3.2） |
| 迁移后提供方式 | 光环 spell ID / 数值来源与系数 / 行为分支位置 |
| 状态 | 待迁移 → 已建立 → 已验证 → **已删除旧实现** |

**最后两列是防“两套并行”的硬检查点：一行只有在“已删除旧实现”勾选后才算完成。**

**非迁移范围必须登记白名单**（套装效果、技能自带效果、Bot 平衡性改动），否则第 8.1 节的零残留复查会把它们误判为漏删。

战士职业的完整清点结果见附录 B。

---

## 8. 测试与验收

### 8.1 单元 / 数据验证

- **点预算边界**：等级 1、9、10、11、79、80、81、82、83 的等级基准值分别为 0、0、1、2、70、71、72、73、74，**均属合法，不钳制**；`A(等级)` 单调不减且 10 级以下恒为 0。
- **预算只截断不拒绝**：声明 120 点的构筑用在 40 级 Bot 上**不报错**，只生效前 `A(40)` 点，`applied_points` 与 `plan_total_points` 正确记录差额；**自动档**升级后快照重建并向后追加点数，**手动档**升级后不追加。
- **额外天赋点集成**：额外为 0 时行为等于纯基准；注入非零额外点后预算正确抬高，且**不突破任何天赋的 `maxRank`/层级/前置**；额外点变化（等级不变）也能触发重建（复用判据含 `applied_points`）；配置上限 `BotTalentPointBudgetCap` 生效时按上限拒绝。
- **构筑校验**：错误职业、错误树、超最大 rank、层级未解锁、前置不满足、缺少 Talent/Spell 数据 → 拒绝。**“超生效预算”不在此列**（属截断）。
- **序列前缀合法性**：构造一个“总点数合法但中间某前缀非法”的构筑（如先点第 4 层再补够前置），应被拒绝。
- **技能 rank**：等级门槛前后、rank 链断裂、多来源重复、专精移除后仍由其他来源有效。
- **雕文**：19/20 级边界（19 级 0 槽、20 级 6 槽）、槽类型不匹配、重复雕文策略、专精切换的效果撤销/重加。
- **等级来源**：自由 Bot（`IAmFree()`）与带精英加成的雇佣 Bot，验证 `master->GetLevel()` → `me->GetLevel()` 切换后被动获取时间点符合预期。
- **幂等性**：连续初始化两次结果与一次相同；低级加载、升级、直升高等级、降级/GM 改级均有定义。
- **旧硬编码零残留检查（可自动执行）**：对已迁移的职业文件做静态检索，断言不含属于迁移范围的等级/专精字面条件。按载体分别建立检查规则：
  - `ApplyClassPassives()` 函数体内不应再出现 `level >=` / `lvl >=` / `GetSpec() ==` 形式的天赋/雕文条件；
  - `ApplyClass*` 修正函数体内不应再出现被台账标记为天赋/雕文来源的数值字面量（`crit_chance +=`、`pctbonus *=`、`cooldown -=` 等）；
  - 命中项必须能在台账的“非迁移范围”白名单（套装、技能自带、毒药/附魔）中找到对应条目，否则视为漏删。
- **台账一致性**：阶段 A 的基线计数（160 / 425 / 53 / 178）与复查后的剩余计数之差，应等于台账中状态为“已删除旧实现”的条目所覆盖的出现位置数。
- **应用与保存预设必须正交**：只调 `ApplyBotPlan` → 预设库行数不变、该 Bot 快照变化且 `plan_source = 2`；只调 `SaveBotTalentPreset` → 预设库 +1、**所有 Bot 的快照与 `plan_source` 均不变**。四种组合各验一次。
- **`plan_source` 由操作方式决定，不由内容决定**：把某套预设的完整内容原样通过 `ApplyBotPlan` 应用，`plan_source` 必须是 2 而非 1（不得反查内容去匹配预设）。
- **手动档升级不自动补点**：40 级 Bot 应用 120 点直编构筑（生效 31 点）→ 快照应记录完整 120 点 → 升到 60 级 → **`applied_points` 保持 31 不变**、`unassigned_points` 变为 20，Bot 未因此获得新天赋。
- **档位三条出口**：同一 Bot 依次验证——手改应用后升级不补点；`ApplyBotDefaultPlan` 之后升级**自动**补齐；`ApplyBotTalentPreset` 之后升级**按该预设自动**补齐；再次 `ApplyBotPlan` 手改后又回到不自动。
- **手动档降级必须截断**：60 级手动档 Bot（`applied_points = 51`）被 GM 降到 40 级 → `applied_points` 压到 31、`unassigned_points` 为 0，且未出现超预算生效。
- **手动档升级不触发重建**（验证复用判据按档分化）：手动档 Bot 每次升级不应产生快照重建（用计数或日志断言），否则每次升级都会白跑一遍解析。
- **手动档“一键补满”是玩家操作而非自动行为**：显式调用一次 `ApplyBotPlan`（把 `applied_points` 提升到 `min(plan_total_points, A(等级))`）后才补满；未调用时点数保持不变。
- **导出/导入往返一致（编码正确性的核心断言）**：对每个职业每系，`ImportBotPlanCode(ExportBotPlanCode(plan)) == plan`，逐字段比对天赋 rank 与雕文槽位。**分享码必须包含雕文段**（用 6.6.5 的 `:` 分隔），验证往返后雕文不丢失、后续树点数不错位。
- **与参考实现逐字符一致**：同一构筑下，C++ 导出的分享码与 `tools/talent_url.py` 的输出**必须完全相同**（该工具已支持全部 10 个职业）。这是防止槽位表漂移的关键回归——建议把工具输出的若干基准码（如盗贼三系）作为测试常量固化。
- **导入接受两种输入形态**：纯分享码与完整模拟器链接（含 `?talent#` 前缀）解析结果必须一致。
- **导入不做隐式写入**：`ImportBotPlanCode` 调用后断言预设库行数与所有 Bot 快照**均无变化**；只有显式 `ApplyBotPlan` / `SaveBotTalentPreset` 才写库。
- **导入的拒绝路径**：职业前缀与目标 Bot 职业不匹配、槽位数与本地 DBC 不一致、雕文类型与槽位不匹配、未知职业前缀、含编码表外字符、超长输入 —— 逐一断言被拒绝且返回可读原因，**不静默忽略、不部分应用**。
- **导入复用既有校验**：构造一个结构性非法的分享码（如超 `maxRank`、层级未解锁），断言导入路径与 UI 提交路径**返回同一错误码**（不得出现两套判定不一致）。
- **槽位表自检**：启动时断言盗贼三树槽位数 = 27/28/28（及其余职业），不匹配时导入功能应禁用并记录警告，而不是带病解码。

### 8.2 集成回归

每职业至少一个代表性专精，覆盖等级 **9 / 10 / 15 / 19 / 20 / 30 / 40 / 50 / 70 / 80**；对坦克、治疗、远程、近战各至少一条 AI 战斗场景验证：

- 技能表中只有等级与构筑允许的技能及最高 rank。
- 天赋被动/技能与点数记录一致，无非法前置；**自动档**生效点数等于 `min(构筑总点数, A(等级))`，**手动档**等于玩家确认的 `applied_points`（可小于预算，差额为未分配点数）。
- 雕文槽与已装备雕文一致，效果只应用一次；20 级前后槽位数量正确切换。
- 现有战斗 AI 仍按预期选技能，未因初始化顺序变更失去技能或保留旧专精效果。
- 旧数据库 Bot 首次加载可迁移，失败有可诊断日志且不阻断服务。
- 预设的加载、校验、应用与回退默认构筑的完整链路。
- **导出 → 粘贴到 80.wowdb.cc 模拟器 → 核对点数与雕文**：三系盗贼实测链接见 `NPCBot盗贼默认构筑与脚本改造台账.md` 3.3 节（刺杀 51/13/7、战斗 20/51/0、敏锐 19/0/52，均已验证往返一致）。此项为**人工验收**，用于确认编码规则与第三方站点实际兼容。
  > **注意**：通过 URL hash 打开模拟器后**不能立即读取页面数据**——站点用 1 秒轮询解析 hash，初始化未完成时读到的是中间态（会误判点数丢失）。核对应等页面标题出现正确点数，或直接调用页面的 `setWhBuild()` 验证编码本身。
- 多预设解析：同系多套并存、应用预设与使用系统默认两条路径、删预设后运行中 Bot 继续用快照而重建时回到默认构筑、改专精后未设置的系回到默认构筑。
- 生效快照：**自动档**下等级提升后 `built_at_level`/`applied_points` 变化触发重建并向后追加点数（**手动档**下不触发）；解雇后快照行被删除；`preset_ref` 失效不导致异常。
- 超预算构筑全链路（**自动档**）：保存 120 点预设 → 40 级 Bot 应用 → 生效 31 点且 UI 显示差额 → 升到 80 级 → **自动**补齐到 71 点（超出 71 的部分由额外天赋点提供）。同场景若改走**手动档**（`ApplyBotPlan` 直编），升级后 `applied_points` 应保持不变。
- **数值等价性回归（B 类迁移专项）**：对每项迁移的数值效果，在固定等级/专精/装备下比对迁移前后的最终结果（如 `ApplyClassSpellCritMultiplierAll()` 输出的 `crit_chance`、`GetBotArmorPenetrationCoef()` 的返回值）。**若迁移后数值高于迁移前，优先怀疑旧硬编码未删除（双重累加），而不是新层算错**——这是 7.3 给出的诊断顺序。
- **专精切换与效果撤销**：切换专精后旧构筑的 A/B/C 三类效果全部失效、新构筑全部生效；连续切换 3 次以上结果稳定（验证无残留光环、无数值累积）。
- **直编应用全链路**：`GetBotPlanForEditing` 载入当前生效的完整序列 → 改几项 → `ApplyBotPlan` → 立即生效、其他 Bot 不受影响、预设库无新增 → 解雇该 Bot 后快照被删、重新雇佣回到系统默认（直编内容如期丢失）。
- **档位行为全链路**：30 级 Bot 跑默认构筑（自动档，`A(30) = 21`）→ 玩家在界面改几项并应用（转手动档）→ 升到 35 级（`A(35) = 26`）→ **Bot 天赋未自动增加，UI 显示“未分配 5 点”** → 分别验证三条出口：手改补满、`ApplyBotDefaultPlan`（之后升级自动补）、`ApplyBotTalentPreset`（之后按预设自动补）。

---

## 9. 实施排期

| 阶段 | 内容 | 产出 | 完成判据 |
|---|---|---|---|
| **A** | 清点 3 类载体，建立行为基线 | **效果台账**（每效果：来源、ID、全部出现位置、目标路径、迁移后提供方式）+ 非迁移范围白名单 | 10 个职业文件全部清点完毕，各载体计数与基线（160 / 425 / 53 / 178）对账一致 |
| **B** | 统一等级基准、集中技能来源 | `master->GetLevel()` → `me->GetLevel()`（10 处 + `bot_ai.cpp:7721/7775`）；技能来源标签挂到 `_spells` 首级 ID | 逐职业“被动生效等级前后对比表”，差异项均有明确理由 |
| **C** | 接入天赋分配 | 天赋效果由构筑提供（路径 1/2/3 分流） | **同批次删除**全部 A 类与天赋相关的 B/C 类硬编码；台账中天赋项状态为“已删除旧实现” |
| **D** | 雕文配置化 | 雕文槽、装备、效果三态分离；效果由构筑提供 | **同批次删除**雕文相关硬编码。**注意：15/21 的战士雕文只在 B 类存在**，是漏删高发区 |
| **D′** | 职业脚本零残留复查 | 静态检索报告 + 与台账对账结果 | 8.1 的零残留检查全绿；所有命中项都能在非迁移白名单中找到对应条目 |
| **E** | 专精切换、等级跳跃与持久化 | `ReconcileBotProgression()` 挂入 `SetStats()`；解雇两条路径清理 | 8.2 的专精切换、等级跳跃用例通过；解雇后无孤儿行 |
| **F** | 玩家预设与接口 | 三张表 + 6.4 接口 + `mod-ale` 注册 | 8.1 的应用/保存正交、档位行为用例通过 |
| **G** | 客户端天赋树 UI（`mod-ale`） | — | **不在本方案内** |

**排期提示**：阶段 A 的规模由台账决定，而台账的完整度直接决定 C/D 的风险。**不建议为了压缩排期而跳过 A**——B 类共 178 处雕文注释分散在 160 个覆写函数中，没有台账就无法判断某次删除是否完整。

---

## 10. 风险与取舍

### 10.1 重复效果（本方案最主要的失败模式）

**B/C 类残留会双重累加，A 类残留会互相覆盖。** 具体机制与迁移严格度见 7.3。

数值类残留是最危险的：`crit_chance += 30.f` 出现两次就是 +60，不报错、不崩溃、无日志，只表现为“这个 Bot 伤害偏高”。因此：

- 每项效果的迁移必须**删除旧实现**才算完成，台账状态列强制这一点；
- 阶段 D′ 做零残留复查，与台账对账；
- 8.2 的数值等价性回归须在**迁移前先取基线值**，否则事后无法判断是否偏高。

### 10.2 DBC 数值漂移（路径 2 的固有代价）

路径 2 把 DBC 数值抄进 C++，存在“改了 `Spell.dbc` 但 Bot 仍用旧值”的风险。缓解：

- 优先走路径 1（光环），仅在前两条前提不成立时用路径 2；
- 每个路径 2 条目必须在台账中标注对应的 Talent/Glyph/Spell ID，作为同步锚点；
- 把可配置的数值放在集中位置（而非散落在各职业文件），便于审查。

### 10.3 等级基准变更的行为影响

把 10 处 `master->GetLevel()` 改为 `me->GetLevel()` 是**行为变更**，会改变部分 Bot 的被动获取时间点（雇佣 Bot 在带 rank 加成时会提前获得被动）。必须作为独立子任务提交，并做逐职业回归对比（阶段 B）。

### 10.4 `RefreshAura()` 不支持叠层

`count` 只接受 0/1。现有实现用不同 spell ID 表达不同 rank（`DEEP_WOUNDS_1/2/3`）。效果层设计**不能假定“施加一次 spell 就能覆盖所有 rank”**——rank 变化需映射到不同 spell ID。

### 10.5 无上限模型的数据体量

构筑点数不设上限，`talent_data` 可能很长。需要：

- 数据体量上限（条数、字符串长度、rank 范围）防畸形数据；
- `plan_total_points` 缓存以避免每次显示都重算；
- 解析结果的缓存，避免战斗中高频调用时重复解析。

### 10.6 不做的取舍

- **不复制完整天赋树**：树由服务器 DBC 权威维护，避免数据重复与补丁漂移。
- **不把 Creature 塞进 Player 流程**：复用规则与校验思想，而非 `Player` 成员函数。
- **不做“保存并应用”复合接口**：UI 天然是两次动作，合并会丢失“只应用不保存”语义。
- **不模拟经济/社交**：不要求洗点费用或雕文物品来源。

---

## 11. 参考资料

### 11.1 本地可用的权威数据源

| 数据 | 路径 | 用途 |
|---|---|---|
| 法术数据 | `outputs/Spell.csv` | 法术名、rank 链、描述（列错位：中文名 index 138、描述 172、光环描述 189；关键列 `DurationIndex` 38、`EffectAura_1/2/3` 93/94/95、`EffectTriggerSpell` 114-116） |
| 天赋 / 雕文 DBC | `G:/wow/dbc/`（=`E:/WoW Spell Editor/DBC_335_wotlk/`） | 天赋树结构、雕文属性与槽位 |
| 字段格式串 | `src/server/shared/DataStores/DBCfmt.h` | **权威字段顺序**，勿只凭 `DBCStructure.h` 结构体推断 |
| 雕文时长参考 | `G:/wow/dbc_csv/SpellDuration.csv` | `ID,Duration,PerLevel,MaxDuration`（毫秒） |

### 11.2 玩家侧参考实现（校验规则照抄来源）

| 主题 | 位置 |
|---|---|
| 雕文槽解锁 | `Player::InitGlyphsForLevel()`，`Player.cpp:13789`、`:13791-13794` |
| 雕文类型匹配 | `PlayerStorage.cpp:5929-5939`、`SpellEffects.cpp:4609-4611` |
| 天赋 rank 反查 | `bot_warrior_ai.cpp` / `bot_rogue_ai.cpp` 现有 Talent 定义 |

### 11.3 使用边界

- 本文是**设计文档**，不含代码修改。所有涉及源码的论断均已标注文件与行号，便于核对。
- 文中标注为“实测”的 DBC 数值取自本地 `G:/wow/dbc/`；更换客户端版本后需重新确认，尤其 `TalentTab.ClassMask` 与 `GlyphSlot.TypeFlags`。
- 本文**不定义**额外天赋点的来源机制、默认构筑的具体内容与目标点数——这三项属外部集成与内容决策，见 11.4。

### 11.4 待定的外部决策项

| 决策项 | 状态 | 本方案需保证的边界 |
|---|---|---|
| **额外天赋点的来源机制** | 外部集成，本方案只预留 `GetBonusTalentPoints()` 接口 | 预算可加、不设 71 上限、预算变化能触发快照重建 |
| **默认构筑的权威内容** | 未定。可参考 `outputs/NPCBot/BOT职业/` 现有职业文档（奥法、术士、牧师、盗贼）作为输入来源 | 表 1 的 `(bot_class, spec)` 逐行覆盖所有受支持专精 |
| **默认构筑的目标点数** | 未定（“单系满 + 副系关键点”还是“三系配满”） | 不设上限，故目标点数只影响“额外天赋点需补多少才能完全生效” |
| **哪些雕文对 Bot 无意义** | 未定，须在阶段 A 逐项判定并标注“不支持/忽略” | 不能默认全部可装备 |

---

## 附录 A：既有重建序列（供挂载参考）

`bot_ai::SetSpec(activate = true)`（`bot_ai.cpp:16031-16060`）的顺序，是新增构筑应用逻辑时应复用的既有链路：

```text
写库 → UnsummonAll → removeShapeshiftForm → RemoveAllAurasExceptType
     → ApplyItemsSpells → ApplyRacials
     → InitSpells → ApplyClassPassives → InitHeals
     → 清空三系能量
```

`SetStats(bool force)` 的 `if (force)` 块（`bot_ai.cpp:2438-2452`）：

```text
InitPowers → InitSpells → ApplyClassPassives → 等级属性写入
```

`InitSpells()` 必须先于 `ApplyClassPassives()`（代码注释明确规定），构筑应用步骤须插在 `InitSpells()` 之前完成天赋/雕文决策。

## 附录 B：效果台账

台账以“**效果**”为清点单位（原因见 2.2）：每个效果登记它在 3 类载体中的全部出现位置。

| 项 | 值 |
|---|---|
| 清点职业 | 战士（`bot_warrior_ai.cpp`），作为其余 9 职业的填写范本 |
| 数据行 | 90 条 |
| 迁移范围 | **84** 条（天赋 63 + 雕文 21） |
| 非迁移白名单 | **6** 条（套装 3：T10 4P / T8 4P / T9 2P；技能自带 2：Recklessness / Bloodthirst-MS 怒气折扣；Bot 平衡 1：英勇打击 -25%） |
| 载体分布 | A 45 处 / B 50 处 / C 1 处，其中 **6 条跨 A+B** |
| 文件 | `NPCBots职业脚本硬编码效果台账.csv` |

样例行：

```
Critical Block,天赋,待查,Talent,A+B,bot_warrior_ai.cpp:2082 / :1261,1(光环)+2(数值),光环走构筑 + 盾牌猛击暴击加成走数值查询,待迁移(跨载体-重点)
Glyph of Victory Rush,雕文,待查,Glyph,B,bot_warrior_ai.cpp:1268-1270,2(数值),暴击率走槽位数值查询,待迁移(重点:仅B类存在)
```

### 从战士样本看出的三个规律

1. **雕文是漏删高发区**：21 条雕文中 15 条只在 B 类存在，按“改造 `ApplyClassPassives()`”理解范围会漏掉大部分。
2. **天赋常被拆散**：`Poleaxe Specialization` 同一效果拆在 B 类两处；`Critical Block` 跨 A+B 两类。
3. **存在必须保留的非迁移项**：套装、技能自带、Bot 平衡性改动都不是天赋/雕文效果，但同样以数值修正形式存在。**不做白名单，零残留复查会误删它们。**

### 其余职业的推进方式

按战士的表结构与字段填写；每完成一个职业，其基线计数（该职业的 `ApplyClass*` 覆写数、`RefreshAura()` 数、`GLYPH_*` 数、雕文注释数）须记入台账，作为阶段 D′ 对账的输入。
