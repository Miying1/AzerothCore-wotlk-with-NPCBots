# NPCBots 技能、天赋与雕文应用机制重构分析

> 范围：AzerothCore WotLK 3.3.5a 集成版 NPCBots。分析现有技能选择、专精、天赋与雕文效果的实现，并给出按 Bot 等级模拟玩家逐步获得技能、分配天赋点、解锁并装备雕文的落地方案。本文为设计文档，不含代码修改。

> **核查状态（2026-10-14）**：本文第 2、3 节的事实性论断已逐条对照本地源码与 DBC 验证，验证方法与结论见第 2.3 节；核查中发现 7 处需在设计阶段一并处理的问题，见第 2.4 节与第 12 节。
>
> **实施原则（2026-10-14 补充，用户明确）**：天赋与雕文的应用**必须同时重构职业脚本中的相关代码，不允许新层与旧硬编码并行**。硬编码不只集中在 `ApplyClassPassives()`，而是分散在 3 类载体、约 160 个覆写函数中（见 4.2.1）；其中数值修正类的残留会**直接双重累加**，不会互相覆盖。因此本文所有阶段划分都以“同批次删除对应硬编码”为前置条件。

## 1. 结论摘要

1. 当前 NPCBots 没有完整的玩家式天赋树分配与可配置雕文系统。Bot 的“专精”只是一个选择值（`characters_npcbot.spec`）；技能可用性由各职业 AI 按等级和专精构建；部分雕文效果直接写在职业 AI 行为/数值中。
2. 采用“服务器规则数据 + 默认构筑 + 运行时按等级渐进应用”的混合方案：规则（天赋合法性、法术链、雕文属性）以服务器数据为准，构筑只记录“选什么”，运行时保留 Bot 自有的效果应用层。
3. 需要为每个受支持专精准备**默认构筑**（目标天赋 rank 顺序 + 雕文选择），但**不需要复制整棵天赋树**——树结构来自服务器 DBC。
4. 天赋点数：等级基准 = `等级 - 9`（10 级起每级 1 点，与玩家一致）；**Bot 不套用 71 点上限，构筑点数也不设上限**，生效预算 = 等级基准 + **额外天赋点**（由外部机制提供，本方案只预留接口）。预算是**截断阈值而非拒绝条件**。雕文槽按本项目规则在 **20 级后全部开放**。Bot 自动执行这些操作，但必须保留独立、可查询、可重建的状态。
5. 第二阶段允许玩家为自己雇佣的 Bot 设置构筑，采用**三张表按职责分离**（第 6 节）：默认构筑放 world 库；玩家预设按 `(owner, bot_class, spec, preset_index)` 支持每系多套；Bot 实际生效的构筑**单独建表存完整快照**。新雇佣 Bot 一律用系统默认构筑。玩家的**“应用”与“保存为预设”是两个互相独立的操作**：应用（包括在界面上改几项后直接点应用）只写生效快照、立即生效、**不落预设库**；保存为预设只写预设库、**不改变任何 Bot**。客户端天赋树 UI 由 `mod-ale` 实现，不在本方案内。
6. 分阶段实施：先清点硬编码规则并保证行为等价，再引入构筑与渐进应用，随后落地玩家预设与钩子，最后接客户端 UI。不把 Creature 塞入 Player 的天赋/雕文流程。
7. **本次重构是“替换”而非“新增”**：天赋/雕文效果目前以硬编码形式分散在职业脚本的 3 类载体中——`ApplyClassPassives()` 的 `RefreshAura()` 调用、~160 个 `ApplyClass*` 数值/时长/冷却修正覆写、以及 AI 决策分支。新效果层能正确提供某项效果后，**必须同批次删除对应的旧硬编码**，否则会出现“光环互相覆盖”或“数值双重累加”（见 4.2.1、4.2.2）。
8. 核查中定位到 7 项须在实施前处理的具体问题：解雇清理有两条路径、各职业被动使用 `master->GetLevel()` 而非 `me->GetLevel()`、天赋点预算须实现为"等级基准 + 额外天赋点"的可加形式且**不作上限校验**、`RefreshAura()` 不支持叠层、**硬编码载体分散且数值类残留会双重累加**、**“应用”与“保存为预设”须拆为两个独立操作**、**升级补点行为须按档位区分**（手动档不自动补点）。详见 2.4 与第 12 节。
9. **升级时是否自动应用新点数由“档位”决定**（用户明确，见 6.4）：系统默认构筑与玩家预设属**自动档**，Bot 升级时自动按新预算向后补齐；玩家在界面上直接改点并应用后进入**手动档**，此后升级**不再自动补点**，未分配的点数由玩家自己配置，或通过“恢复默认”“使用预设”两条操作回到自动档。**降级时两种档位都会被截断**（预算是硬上限）。

## 2. 当前实现盘点

### 2.1 主要代码位置

| 领域 | 位置 | 当前职责 |
|---|---|---|
| Bot 通用初始化 | `src/server/game/AI/NpcBots/bot_ai.cpp` | `SetStats(bool force)` 在 `bot_ai.cpp:2396`；等级变更时于 `bot_ai.cpp:2448-2449` 依次调用 `InitSpells()` → `ApplyClassPassives()`（注释明确 `//this must stay before class passives`）。`InitSpellMap()`（`bot_ai.cpp:7474`）按等级沿法术 rank 链选最高可用等级 |
| 专精持久化 | `src/server/game/AI/NpcBots/botdatamgr.h`（枚举 `NPCBOT_UPDATE_SPEC`，`botdatamgr.h:76`）、`botdatamgr.cpp` | `characters_npcbot.spec` 保存专精；`bot_ai::SetSpec()`（`bot_ai.cpp:16031`）在 `activate=true` 时写库并重建 |
| 职业技能规则 | `src/server/game/AI/NpcBots/bot_<class>_ai.cpp` | 19 个职业/AI 类各自 override `InitSpells()`、`ApplyClassPassives()` 与约 10–16 个 `ApplyClass*` 修正函数；等级门槛与专精条件**以硬编码形式散落在这三类载体中**，见 4.2.1 |
| 玩家天赋/雕文参考 | `src/server/game/Entities/Player/Player.cpp`、`Player.h` | `Player::InitGlyphsForLevel()`（`Player.cpp:13789`）按等级位掩码解锁雕文槽；`Player.h:1804-1806` 提供 `SetGlyphSlot` / `GetGlyphSlot` / `SetGlyph` |

### 2.2 已观察到的差异

- Bot 自维护 `_spells`（`bot_ai.h:873`，类型 `BotSpellMap = std::unordered_map<uint32 /*firstrankspellid*/, BotSpell>`），不等同于玩家 SpellMap；等级变化时会重选 rank，但这不表示它逐级“学习”并保存了玩家技能。
- `spec` 是专精/角色策略标识（`botcommon.h:340-361`，取值 1–30，`BOT_SPEC_DEFAULT = 31`），不等同于完整的三系 Talent rank 分配；技能门槛与天赋条件目前编码在职业 AI 内部。
- 职业 AI 中有大量 `//Glyph of ...` 注释与对应行为，且已有 53 个 `GLYPH_*` spell 枚举常量（如 `bot_warrior_ai.cpp:128 GLYPH_HEROIC_STRIKE = 58357`），但仍**未发现** Bot 雕文槽、雕文 ID 持久化或通用装备/撤销流程。
- **雕文效果存在两类载体，且数量级不同**：一类是 `ApplyClassPassives()` 中以 `RefreshAura(GLYPH_*, 等级条件 ? 1 : 0)` 施加的**光环类**（对应那 53 个常量）；另一类是**直接写在 `ApplyClass*` 数值/时长/冷却修正里的注释 + 魔法数字**（如 `bot_warrior_ai.cpp:1268 //Glypg of Victory Rush: 30% additional critical chance` 后紧跟 `crit_chance += 30.f;`），共约 178 处雕文注释，**这部分连常量都没有**。后者才是工作量大头，且不接入任何撤销机制。
- 天赋效果同样跨载体：既出现在 `ApplyClassPassives()` 的 `RefreshAura()` 里，也出现在 `ApplyClass*` 的数值修正里（如 `Poleaxe Specialization` 同时以 `pctbonus *= 1.025f`（`bot_warrior_ai.cpp:1220`）与 `crit_chance += 5.f`（`:1250`）两处表达）。
- 不能直接套用玩家 API：玩家流程依赖 `Player` 的 TalentMap、SpellMap、槽位、数据库与网络更新。Bot 是 Creature/AI，应复用规则与校验思想，而非把 Player 成员函数当通用服务。

**含义**：当前系统只能做到“某等级可用某技能”，无法解释“哪一级学会、花了哪一点天赋、装备了哪个雕文、切专精如何撤销”。重构要把这三类状态分开建模。

### 2.3 事实核查记录

以下论断已对照本地源码与 DBC 逐条验证，作为后续设计的事实基线。

| # | 论断 | 验证方式 | 结论 |
|---|---|---|---|
| 1 | Bot 无天赋树分配、无雕文槽位 | 全目录检索 `glyph`：仅命中注释、`GLYPH_*` spell 常量、以及 `botcommands.cpp:4543` 一处 `spell->m_glyphIndex = 0`（与 Bot 雕文无关） | **成立**。不存在 Bot 侧雕文槽字段、雕文表或装备流程 |
| 2 | 雕文效果已硬编码进职业 AI | 统计 `GLYPH_*` 常量定义 **53** 个；`RefreshAura()` 在 10 个玩家职业文件中共 **425** 处调用；`//...Glyph...` 形式的效果注释共 **178** 处，其中多数位于 `ApplyClass*` 数值修正函数内 | **成立**。且实际规模远大于“若干常量 + RefreshAura”的原始描述，是阶段 A 的主要清点对象（见下条基线） |
| 3 | `ApplyClassPassives()` 是“应用被动法术”的可用先例 | 读取 `bot_warrior_ai.cpp:2036-2085`：`RefreshAura(spellId, level条件 ? 1 : 0)`；`RefreshAura()` 实现内部先 `RemoveAurasDueToSpell(spellId)` 再按 count `AddAura()` | **成立**，且已内建 apply/remove 对称语义，可直接作为效果层基础 |
| 4 | 玩家雕文槽按 15/30/50/70/80 分段 | `Player.cpp:13800-13809`：`level>=15→0x03`、`>=30→0x08`、`>=50→0x04`、`>=70→0x10`、`>=80→0x20`，写入 `PLAYER_GLYPHS_ENABLED` | **成立**，与原规则完全一致；本项目改为 20 级全开是**有意偏离**，非笔误 |
| 5 | 天赋 rank 可用 `TalentEntry->RankID[rank-1]` 反查 | `DBCStructure.h` `TalentEntry`：`TalentID(0)`、`TalentTab(1)`、`Row(2)`、`Col(3)`、`RankID[5](4-8)`、`DependsOn(13)`、`DependsOnRank(16)`、`addToSpellBook(19)`；`MAX_TALENT_RANK = 5` | **成立**。反查路径与前置/层级字段均存在，校验所需数据齐备 |
| 6 | 雕文效果技能可由 `GlyphProperties.dbc` 的 `SpellId` 反查 | `DBCStructure.h` `GlyphPropertiesEntry{Id, SpellId, TypeFlags}`；`DBCfmt.h` 格式串 `"niix"`；实测 362 条 × 4 字段 | **成立**，与原方案一致 |
| 7 | 雕文槽类型校验需要 `GlyphSlot` 数据 | `GlyphSlotEntry{Id, TypeFlags, Order}`，格式串 `"nii"`，实测 10 条；玩家侧校验见 `PlayerStorage.cpp:5929-5939` 与 `SpellEffects.cpp:4609-4611`：`TypeFlags == TypeFlags` 才合法 | **成立**，且已实测确认 `TypeFlags` 0=大雕文 / 1=小雕文（见 2.5.2） |
| 8 | `spec` 在 10 级以下固定返回默认 | `bot_ai::GetSpec()`：`return me->GetLevel() < 10 ? uint8(BOT_SPEC_DEFAULT) : _spec;` | **成立** |
| 9 | 专精切换经 `ACTIVATE_SPEC` 落地 | `SwitchSpec()`（`bot_ai.cpp:16067`）设 `_newspec` 并 `CastSpell(me, ACTIVATE_SPEC)`（`botspell.h:21`，ID 63645）；`bot_ai.cpp:17210-17211` 在法术落地判定 `curInfo->Id == ACTIVATE_SPEC` 后调用 `SetSpec(_newspec)` | **成立** |
| 10 | `SetSpec(activate=true)` 重建技能/被动/治疗 | `bot_ai.cpp:16031-16060`：写库 → `UnsummonAll` → `removeShapeshiftForm` → `RemoveAllAurasExceptType` → `ApplyItemsSpells` → `ApplyRacials` → `InitSpells` → `ApplyClassPassives` → `InitHeals` → 清空三系能量 | **成立**。这是新增构筑应用逻辑时应挂载的既有重建序列 |
| 11 | 解雇清理可放在 `NPCBOT_UPDATE_OWNER` 分支 | `botdatamgr.cpp:2963` 单 entry 分支与 `:3159` 批量分支；批量分支内已有 `CHAR_DEL_NPCBOT_TRANSMOG_ALL` 清理，且 `:3136 NPCBOT_UPDATE_ERASE` 会 `DELETE FROM characters_npcbot WHERE entry = ?` | **需修正**，见 2.4-① |
| 12 | Lua 接口经 `mod-ale` `RegisterFunctions()` 注册 | `modules/mod-ale/src/LuaEngine/LuaFunctions.cpp:1898 void RegisterFunctions(ALE* E)` 存在 | **成立** |
| 13 | 等级变化会触发 Bot 重算 | `Player::GiveLevel` 内 `sScriptMgr->OnPlayerLevelChanged(this, oldLevel)` 之后调用 `_botMgr->SetBotsShouldUpdateStats()`（`Player.cpp:2686-2688`）→ `botmgr.cpp:1477` → `SetShouldUpdateStats()` → `UpdateAI` 中 `SetStats(false)` | **成立**，见 2.4-③ |
| 14 | 硬编码载体远不止 `ApplyClassPassives()` | 统计 10 个玩家职业文件：`ApplyClass*` 覆写 **160** 个函数、`RefreshAura()` **425** 处调用、`GLYPH_*` 常量 **53** 个、含等级门槛判断的行 **1172** 处 | **成立**，见 2.4-⑤ 与 4.2.1。这是“两套并行”风险的根源 |
| 15 | 天赋与雕文的效果也走数值修正路径 | `bot_warrior_ai.cpp:1207-1275`：Poleaxe Spec（`pctbonus *= 1.025f` / `crit_chance += 5.f`）、Incite、Improved Overpower、Critical Block、Sword and Board、**Glyph of Victory Rush**（`crit_chance += 30.f`）全部为累加/累乘 | **成立**，属路径 2。此类残留**不会互相覆盖，而是双重累加**，见 4.2.2 |

### 2.4 核查中发现的问题（须在设计阶段处理）

**① 解雇清理的挂载点需要区分“解雇”与“删档”两条路径**

原方案（6.6 与 6.3）写的是“在现有 `NPCBOT_UPDATE_OWNER` 分支执行删除，与已有的 transmog 清理放在同一处”。实际代码里这是**两个语义不同的位置**：

- `botdatamgr.cpp:3159 NPCBOT_UPDATE_OWNER`（批量）：把某玩家的**所有** Bot 的 `owner` 置 0。这里用 `CHAR_DEL_NPCBOT_TRANSMOG_ALL`（`WHERE entry IN (SELECT entry FROM characters_npcbot WHERE owner = ?)`）清理，属于“批量解雇”。
- `botdatamgr.cpp:3136 NPCBOT_UPDATE_ERASE`（单 entry）：**彻底删除**该 Bot 的 `characters_npcbot` 行，调用点在 `botcommands.cpp:3654` 与 `:3706`。

二者都需要删除生效快照，理由不同：批量解雇后 Bot 变自由 Bot，下次雇佣应回到系统默认（删快照即可）；删档则必须连带删除，否则留下永久孤儿行。**实现时应两处都加**，且批量分支的语句要写成 `WHERE entry IN (SELECT entry FROM characters_npcbot WHERE owner = ?)` 形式以复用现有模式（必须在 `UPDATE owner` 之前求值，否则子查询结果已为空）。

**② `master->GetLevel()` 与 Bot 实际等级不一致，等级判定基准须统一**

核查发现 **10 个玩家职业文件**的 `ApplyClassPassives()` 首行均写作 `uint8 level = master->GetLevel();`：

| 文件 | 行 | 文件 | 行 |
|---|---|---|---|
| `bot_warrior_ai.cpp` | 2038 | `bot_paladin_ai.cpp` | 2349 |
| `bot_death_knight_ai.cpp` | 1905 | `bot_priest_ai.cpp` | 2215 |
| `bot_druid_ai.cpp` | 2979 | `bot_rogue_ai.cpp` | 2383 |
| `bot_hunter_ai.cpp` | 2387 | `bot_shaman_ai.cpp` | 2636 |
| `bot_mage_ai.cpp` | 1867 | `bot_warlock_ai.cpp` | 2115 |

另有 `bot_ai.cpp:7721`（`_OnHealthUpdate()` 小兵血量）与 `:7775` 两处同类用法，影响面较小但基准同样应以 Bot 为准。而 `master` 是 `Player* master`（`bot_ai.h:633`），即**玩家主人**，不是 Bot 本身。

这与 Bot 实际等级存在系统性偏差。`bot_ai::SetStats()`（`bot_ai.cpp:2394-2403`）中：

```cpp
uint8 mylevel = std::min<uint8>(master->GetLevel(), DEFAULT_MAX_LEVEL);
// ... 自由 Bot 走独立的 _baseLevel / _killsCount 逻辑
else
    mylevel += BotDataMgr::GetLevelBonusForBotRank(me->GetCreatureTemplate()->rank);
mylevel = std::min<uint8>(mylevel, DEFAULT_MAX_LEVEL + 3);
mylevel = std::max<uint8>(mylevel, BotDataMgr::GetMinLevelForBotClass(_botclass));
```

即雇佣 Bot 的等级是 `master 等级 + 精英等级加成`（并受职业最低等级、`MAX_LEVEL+3` 上限约束），**并且自由 Bot 完全不受 master 等级影响**。

后果：自由 Bot（`IAmFree()`）的 `master` 可能为空或语义不同时，`master->GetLevel()` 与 Bot 真实等级脱节；雇佣场景下则恒偏低一个 rank 加成。这直接冲击本方案的核心判据——“按 Bot 等级渐进应用”必须以 **`me->GetLevel()`** 为唯一基准。

> **设计决定**：本方案的所有等级判定（天赋点数、雕文槽、技能 rank、构筑截断）一律以 `me->GetLevel()` 为准，并在阶段 B 中把上述 10 处 `ApplyClassPassives()` 内的 `master->GetLevel()` 统一改为 `me->GetLevel()`。此项**属于行为变更**，会改变部分 Bot 的被动获取时间点，须单独提交并做回归对比（见第 7 节阶段 B 与第 12 节）。

**③ “等级变化”不止一条入口，不能只依赖自然升级回调**

现有链路是 `Player::GiveLevel` → `OnPlayerLevelChanged` → `SetBotsShouldUpdateStats()`。但 Bot 等级在以下路径同样会变，且**不经过**该回调：

- 自由 Bot 击杀累积：`_killsCount` 提升导致 `mylevel` 上升，仅置 `shouldUpdateStats = true`（`bot_ai.cpp:17052`）。
- 招募/重新召唤：`botmgr.cpp:549` 直接 `SetShouldUpdateStats()`。
- 装备变更、角色变更：`bot_ai.cpp:14405`、`:14424`、`:15738`。
- GM 命令与脚本直接改等级。

因此方案要求的统一入口 `ReconcileBotProgression(bot)` 必须挂在**已有的 `SetStats()` 重建序列内部**（而非新增一个独立的等级回调），才能覆盖全部路径。这也意味着第 5.1 节的“初始化或等级变化”流程实际执行点是 `SetStats()` 的 `if (force)` 块。

**④ Bot 等级可达 83，且天赋点数不以 71 点封顶**

`mylevel = std::min<uint8>(mylevel, DEFAULT_MAX_LEVEL + 3);`（`bot_ai.cpp:2436`）允许 Bot 最高 83 级。

**本项目规则（用户明确）**：Bot 的天赋点数**不套用玩家的 71 点上限**，**构筑声明的点数本身也不设上限**。点数由"等级基准"与"额外天赋点"两部分相加：

```text
等级基准   = max(0, 等级 - 9)       与玩家速率一致，每级 1 点
额外天赋点 = 由外部机制提供          超出玩家规则的部分
生效预算 A(等级) = 等级基准 + 额外天赋点     无 71 硬上限
```

因此：

- 81–83 级按基准得 72–74 点，**合法**，不做钳制。
- 点预算**可能**高于 71，因此**不应以"总点数超过 71"作为校验失败条件**。
- **关键区分**：生效预算 `A(等级)` 是**截断阈值**（该等级当前能应用多少点），**不是拒绝条件**。构筑声明 150 点而当前只允许 31 点是**正常状态**，按序列截取前 31 点应用即可，不报错（见 5.1、6.4）。
- 校验真正的**拒绝条件**只有结构性非法：`talentId` 不存在、职业/树不匹配（`TalentTab.ClassMask`）、超天赋自身 `maxRank`、层级门槛未满足、前置天赋未满足、序列的某个前缀在任何等级都无法构成合法部分分配。

**设计决定**：基准与额外点分离，额外来源作为**外部集成点**保留接口，本方案不设计其实现：

```cpp
// 基准：与玩家速率一致（10 级起每级 1 点），本方案唯一内置的点数来源
uint32 GetBaseTalentPointBudget(uint8 botLevel)
{
    return botLevel < 10 ? 0 : uint32(botLevel) - 9;
}
// 额外天赋点：由外部机制提供（来源、发放时机与上限均不在本方案范围内）
// 本方案只需保证 A(等级) 是可加的、且不设 71 上限
uint32 GetBonusTalentPoints(uint8 botLevel, /* 预留：来源标识 */);
// 生效预算（截断阈值，非拒绝条件）；无 71 硬上限
uint32 GetTalentPointBudget(uint8 botLevel)
{
    return GetBaseTalentPointBudget(botLevel) + GetBonusTalentPoints(botLevel);
}
```

**注意**：额外天赋点**不能**用于突破天赋自身的 `maxRank`、层级解锁条件或前置要求——否则会绕过 Talent.dbc 规则。它只抬高"当前可应用点数"这一项。即"允许在合法范围内投得更多"，不是"允许投出非法分配"。

若后期需要限制单套构筑的规模（防止畸形数据），应以配置项（如 `BotTalentPointBudgetCap`，默认 0 = 不限）表达，而不是在代码里写死 71；该上限触发的是**拒绝**，与 `A(等级)` 的截断语义不同。

**⑤ 天赋/雕文效果分散在 3 类硬编码载体中，重构必须整体覆盖**

这是本次核查中**影响面最大**的发现，也是“不许两套并行”这条原则的直接依据。逐文件统计 10 个玩家职业文件（`bot_<class>_ai.cpp`）后得到：

| 载体 | 代码形态 | 实测规模 | 对应路径 |
|---|---|---|---|
| **A. `ApplyClassPassives()` 内的 `RefreshAura()`** | `RefreshAura(spellId, 等级/专精条件 ? 1 : 0)` | **425** 处调用，其中含 **53** 个 `GLYPH_*` 常量 | 路径 1（光环） |
| **B. `ApplyClass*` 系列修正函数** | `crit_chance += N`、`pctbonus *= x`、`cooldown -= N`、`cost -= N` 等，配 `//Glyph of ...` / `//Xxx Specialization: ...` 注释 | **160** 个覆写函数；**178** 处雕文效果注释（这部分多数**连常量都没有**） | 路径 2（数值） |
| **C. AI 决策分支** | `if (等级/专精条件)` 切换到另一套优先级或目标选择 | 未逐一清点，散落在技能选择、`GetAIMiscValue()`、`ReduceCD()` 等处 | 路径 3（行为） |

**含义：只重构 A 类等于只做了一半。** B、C 类残留会与新效果层并存，这正是“两套并行”。

关键特征是**同一个效果会跨载体出现**。以战士为例（`bot_warrior_ai.cpp`）：

| 效果 | A 类（`ApplyClassPassives`） | B 类（`ApplyClass*`） |
|---|---|---|
| Poleaxe Specialization（天赋） | **不存在** | `:1215-1220` `pctbonus *= 1.025f`（暴击伤害）+ `:1245-1250` `crit_chance += 5.f`（暴击率）——同一天赋拆成两处 |
| Sword and Board（天赋） | 不存在 | `:1265-1266` `if (lvl >= 55 && baseId == DEVASTATE_1) crit_chance += 15.f` |
| Critical Block（天赋） | `:2082` `RefreshAura(CRITICAL_BLOCK, isProt && level >= 50)` | `:1261-1263` `crit_chance += 15.f`（对 Shield Slam） |
| Glyph of Victory Rush（雕文） | 不存在 | `:1268-1270` `if (lvl >= 15 && baseId == VICTORY_RUSH_1) crit_chance += 30.f` |
| Glyph of Heroic Strike（雕文） | `:2085` `RefreshAura(GLYPH_HEROIC_STRIKE, level >= 15)` | 不存在 |

同一个效果或只在 A、或只在 B、或**同时在 A 和 B**。因此**阶段 A 的清点单位不是“`GLYPH_*` 常量”，而是“效果”**：每个效果登记它在 3 类载体中的全部出现位置（可能 0–3 处），形成一份“效果 → 出现位置 → 目标路径”的台账。只有登记完整，才能保证删除时不漏、不误删。

> **设计决定**：重构范围明确包含职业脚本本身。每个效果的迁移以“**先建立新提供方式 → 验证等价 → 删除全部旧实现 → 回归对比**”为固定顺序执行，四步在同一批次内完成。禁止“先加新层、旧硬编码以后再清”的分期做法。

### 2.5 DBC 与数据源事实（本地方案基线）

客户端 DBC 目录：`G:/wow/dbc/`（与 `E:/WoW Spell Editor/DBC_335_wotlk/` 内容一致，同为 141/5813/82085/3925 字节等）；字段语义以 `src/server/shared/DataStores/DBCfmt.h` 的格式串为准。

| 数据 | 规模（已实测） | 格式串 | 结构要点 |
|---|---|---|---|
| `Talent.dbc` | 892 条 × 23 字段 × 92 字节 | `niiiiiiiixxxxixxixxixxx` | `TalentID(0)`、`TalentTab(1)`、`Row(2)`、`Col(3)`、`RankID[0..4](4-8)`、`DependsOn(13)`、`DependsOnRank(16)`、`addToSpellBook(19)`；TalentID 实测范围 23–2285 |
| `TalentTab.dbc` | 33 条 × 24 字段 × 96 字节 | `nxxxxxxxxxxxxxxxxxxxiiix` | `TalentTabID(0)`、`ClassMask(20)`、`petTalentMask(21)`、`tabpage(22)` |
| `GlyphProperties.dbc` | 362 条 × 4 字段 × 16 字节 | `niix` | `Id(0)`、`SpellId(1)`、`TypeFlags(2)`、`spellIconID(3)` |
| `GlyphSlot.dbc` | 10 条 × 3 字段 × 12 字节 | `nii` | `Id(0)`、`TypeFlags(1)`、`Order(2)` |
| 法术 rank 链 | `outputs/Spell.csv`（见 AGENTS.md） | — | `InitSpellMap()` 通过 `SpellInfo::GetNextRankSpell()` 沿链上升 |

**服务端确实加载这些表**（`DBCStores.cpp:370-371`）：`LOAD_DBC(sTalentStore, "Talent.dbc", ...)`、`LOAD_DBC(sTalentTabStore, "TalentTab.dbc", ...)`、以及 `sGlyphPropertiesStore` / `sGlyphSlotStore`（`:307-308`）。因此运行时读取 DBC 做校验是可行的，无需自建天赋树数据。

#### 2.5.1 天赋树归属：`ClassMask` 与 Bot 职业枚举的映射（已实测确认）

`TalentTab.dbc` 字段 20 的 `ClassMask` 实测取值恰为 `1 << (classID - 1)`，覆盖全部 10 个可选职业（classID 1–9、11）：

| ClassMask | bit | classID | 职业 | 该职业的 3 个 TalentTab（按 tabpage 0/1/2） |
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

- **`BOT_CLASS_*` 与职业 ID 是恒等的**：`botcommon.h:222-232` 定义为 `BOT_CLASS_WARRIOR = CLASS_WARRIOR`（=1）… `BOT_CLASS_DRUID = CLASS_DRUID`（=11）。因此 `BOT_CLASS_*` 的 10 个真实职业值**直接就是 classID**，不需要额外映射表。
- **注意两套 mask 约定**，写代码时不要混用（`bot_ai.h:144-145`）：
  - `GetBotClassMask0()` = `1ul << (_botclass - 1)` —— 与 `TalentTab.ClassMask`、`Player::GetClassMask()` 同约定，**天赋树校验用这个**。
  - `GetBotClassMask1()` = `1ul << _botclass` —— 与模块自有 `BOT_CLASS_MASK_*` 常量同约定，用于 Bot 内部分组。
- `BOT_CLASS_BM` 及之后的 9 个自定义职业（无显式赋值，从 12 起递增）**没有天赋树**，`BOT_CLASS_EX_START = BOT_CLASS_BM`。这些职业不得进入天赋构筑流程，否则 `ClassMask` 校验必然失败。

#### 2.5.2 雕文槽与大小雕文判定（已实测确认）

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

即 3 大（索引 0/3/5）+ 3 小（索引 1/2/4），总计 6 槽，与 `BOT_GLYPH_MAX_SLOTS = 6` 一致。

**`TypeFlags` 语义**：`GlyphProperties.TypeFlags` 实测分布为 `{0: 284, 1: 73, 2: 1, 64: 1, 128: 2, 162: 1}`。经已知雕文交叉验证：

| 雕文 | `SpellId` | `GlyphProperties.TypeFlags` | 实际类型 |
|---|---:|---:|---|
| Glyph of Heroic Strike | 58357 | 0 | 大雕文 |
| Glyph of Revenge | 58388 | 0 | 大雕文 |
| Glyph of Chains of Ice | 58620 | 0 | 大雕文 |
| Glyph of Disease | 63334 | 0 | 大雕文 |
| Glyph of Charge | 58097 | 1 | 小雕文 |

**结论：`TypeFlags = 0` 表示大雕文，`1` 表示小雕文**（284 : 73 ≈ 3.9:1，与"大雕文远多于小雕文"的实际情况吻合）。其余零星取值（2 / 64 / 128 / 162 各 1–2 条）为遗留/特殊行。

**两个可直接落地的性质**：

1. 匹配规则就是 `GlyphProperties.TypeFlags == GlyphSlot.TypeFlags`（`PlayerStorage.cpp:5932`、`SpellEffects.cpp:4611`），Bot 侧照抄即可，**不要自造大/小雕文枚举**。
2. 由于该等值比较，那些 `TypeFlags` 为 2/64/128/162 的异常行**永远不会匹配任何槽位（槽位只有 0 和 1）**，天然被排除，无需额外黑名单。

> 以上数值均实测自 `G:/wow/dbc/`。若后续更换客户端版本，应重新跑一遍确认（尤其 `ClassMask` 与 `GlyphSlot` 的 `TypeFlags`）。

## 3. 玩家式应用规则

### 3.1 天赋点数

**Bot 不套用玩家的 71 点上限，构筑声明的点数也不设上限。** 点数规则只有一条基准，加一个外部集成点：

```text
等级 < 10 ：0 点（此阶段不加点，与玩家一致）

等级基准     = 等级 - 9            与玩家速率一致，每级 1 点
额外天赋点   = 由外部机制提供      超出玩家规则的部分，本方案不设计其来源

生效预算 A(等级) = 等级基准 + 额外天赋点
```

**`A(等级)` 是截断阈值，不是拒绝条件。** 构筑可以声明远超当前预算的点数（如 80 级满配 120 点），低等级 Bot 只应用其中前 `A(等级)` 点，其余保留为"未生效"（见 5.1、6.4）。这正是"不设上限"的落地方式。

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

> **Bot 超出 71 点的部分来自"额外天赋点"**，即外部机制在基准之上追加。本方案只需保证预算可加、不设上限，并为额外来源预留接口，无需在此定义它从哪来。

实现：基准与额外点分离，额外来源作为**外部集成点**：

```cpp
// 基准：与玩家速率一致（10 级起每级 1 点）
uint32 GetBaseTalentPointBudget(uint8 botLevel)
{
    return botLevel < 10 ? 0 : uint32(botLevel) - 9;
}
// 额外天赋点：由外部机制提供；来源、发放时机与上限均不在本方案范围内
uint32 GetBonusTalentPoints(uint8 botLevel /*, 预留来源标识 */);
// 生效预算 = 基准 + 额外；无 71 硬上限
uint32 GetTalentPointBudget(uint8 botLevel)
{
    return GetBaseTalentPointBudget(botLevel) + GetBonusTalentPoints(botLevel);
}
```

**点数规则的边界**：额外天赋点只抬高"当前可应用点数"，**不得**用于突破天赋自身的 `maxRank`、层级解锁门槛或前置天赋要求。即它是"在合法范围内可以投得更多"，而不是"允许投出非法分配"。若后期需要限制单套构筑规模以防畸形数据，用配置项表达（如 `BotTalentPointBudgetCap`，默认 0 = 不限），该上限触发**拒绝**，与 `A(等级)` 的截断语义不同。

**渐进节奏**：等级基准部分等价于"**每级 1 点**"，逐级回放时每级最多消费 1 点。额外天赋点不经过等级节奏，在应用时直接计入可用预算，可与等级所得混合投出，但仍须满足天赋合法性。

目标天赋必须满足 Talent.dbc 的职业/树归属、最大 rank、层级点数要求与前置天赋要求。具体到本地数据结构（见 2.5）：`TalentEntry::TalentTab` 经 `TalentTabEntry::ClassMask` 校验职业归属（**实测约定为 `1 << (classID - 1)`，且 `BOT_CLASS_*` 的 10 个真实职业值直接等于 classID**，见 2.5.1，无需额外映射表）；`Row` 决定层级（第 R 层要求该树已投入 `5 × R` 点）；`DependsOn` / `DependsOnRank` 为前置校验；`RankID[0..4]` 为 rank 上限。

**适用职业范围**：只有 10 个真实职业（`BOT_CLASS_WARRIOR` … `BOT_CLASS_DRUID`，值 1–11）有天赋树；`BOT_CLASS_BM` 及之后的 9 个自定义职业（从 12 起）**没有 TalentTab**，不得进入构筑流程。

Bot 若发生等级跳跃（如 20 直升 60），应补算 21–60 级的全部升级事件或等价逐点回放（每级 1 点），不能在最终等级一次性忽略顺序校验——否则无法发现"序列某个前缀在该等级不合法"这类配置错误。

### 3.2 技能训练

玩家达到技能需求等级后访问训练师学习。模拟无需还原金币、距离与点击，但须保留清晰的学习语义：

- 等级/前置条件以服务器 `trainer_spell` 与 Spell 数据为准，不用“每两级学一次”这类经验概括替代实际数据。
- 升级时计算新增可学技能并记录为可用；同一技能链只激活合适的最高 rank，低 rank 不重复进入主动技能选择。现有 `InitSpellMap()`（`bot_ai.cpp:7474`）已实现该 rank 链选择（`info = info->GetNextRankSpell()` 循环），重构应保留这段逻辑，只把“是否可用”与“何时学习”分离。
- 种族、职业、武器/姿态/形态、专精与任务等限制需区分。首期可只覆盖 NPCBot 职业技能与当前 AI 实际支持的技能集。
- 技能循环仍由现有 AI 负责“何时施放”；学习系统负责“是否拥有、当前 rank、是否因专精/天赋/雕文启用”。
- 注意 `_spells` 的键是**首级法术 ID**（`bot_ai.h:866`：`BotSpellMap = unordered_map<uint32 /*firstrankspellid*/, BotSpell>`），值为当前 rank 的 `spellId` 与 `enabled`。来源标签（训练师/天赋/专精/雕文）须以该首级 ID 为键挂接，才能与既有 `HasSpell()` / `GetSpell()` / `IsSpellReady()` 查询路径兼容。

### 3.3 雕文槽与装备

本项目对 Bot 采用简化规则：**达到 20 级后大/小雕文槽全部开放**。

| 槽位 | Bot 解锁等级 | 数量 |
|---|---:|---:|
| 大雕文槽 1/2/3 | 20 | 3 |
| 小雕文槽 1/2/3 | 20 | 3 |

（玩家侧原规则经源码核实为 15/30/50/70/80 分段位掩码，见 `Player.cpp:13800-13809` 与 2.3-④；本项目改为 20 级全开，是**主动偏离**，非笔误。）

解锁槽位不等于自动拥有或装备雕文。Bot 可把“获得可用雕文集合”与“自动装备默认雕文”视为系统代办，但状态模型须区分：槽位已解锁、候选合法、某雕文已装备、效果已应用。切换专精或构筑时须先撤销旧雕文效果再应用新方案，避免光环重复或残留修正。

雕文合法性判定应对齐玩家侧既有实现，而非自造规则。以下均为本地 DBC 实测结果（详见 2.5.2）：

- **槽位集合**：只有 `GlyphSlot.Order != 0` 的 6 条是玩家槽位，`Id` 为 21/22/23/24/25/26，槽位索引 = `Order - 1`（与 `Player.cpp:13791-13794` 一致）。其余 4 条（`Order = 0`）不参与。
- **类型匹配**：`GlyphProperties.TypeFlags == GlyphSlot.TypeFlags`（`PlayerStorage.cpp:5932`、`SpellEffects.cpp:4611`）。
- **`TypeFlags` 取值**：`0` = 大雕文，`1` = 小雕文。槽位分布为索引 0/3/5 是大雕文槽（`Id` 21/24/26），索引 1/2/4 是小雕文槽（`Id` 22/23/25），恰为 3 大 + 3 小。
- **效果法术**：`GlyphProperties.SpellId`（由 `glyphId` 反查）。
- **异常行自动排除**：`GlyphProperties.TypeFlags` 存在零星非 0/1 取值（实测 2 / 64 / 128 / 162 各 1–2 条），因槽位 `TypeFlags` 只有 0 和 1，等值比较天然过滤，无需黑名单。

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

**装备雕文 ≠ 效果生效。** 本节只解决“哪些雕文能装、装在哪”。装好之后的实际作用方式（光环 / 数值 / 行为），以及**如何替换职业脚本中已有的同名硬编码**，见 4.2 与 4.2.1。特别注意：现有雕文效果中有相当一部分（178 处注释中的绝大部分）根本不走光环，而是直接以 `crit_chance += N` / `cooldown -= N` 这类形式写在 `ApplyClass*` 函数内，这部分在迁移中必须逐项删除，否则会与新的雕文层双重累加（4.2.2）。

## 4. 推荐目标架构

### 4.1 分层

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
- **AI 层**：只关心当前有效技能与战斗决策，**天赋/雕文带来的数值与行为差异一律通过查询生效构筑获得，不再内嵌等级判断**。现有施法循环与技能优先级保留，但其中以等级/专精字面条件表达的天赋效果须改为构筑查询（见 4.2.1 的 C 类）。

> **本方案会改动职业脚本本身**（`bot_<class>_ai.cpp`），不是只在其上叠加一个新层。这一层关系必须在实施排期时就明确，否则会退回“两套并行”。

### 4.2 效果应用的三条路径

天赋/雕文的效果技能由数据反查得到（见 6.2），但“如何应用”需按效果性质分三条路径，**默认走第 1 条**：

| 路径 | 适用 | 做法 | 代价 |
|---|---|---|---|
| **1. DBC 光环（默认）** | 纯被动光环、直接授予技能 | 反查 spellId 后由 Bot 施放/应用该法术 | 无 |
| **2. 代码数值修正** | 光环在 Bot 上不生效，或效果依赖 Bot 自有属性计算 | 在效果层按条件和数值修正 | 数值被复制进代码，需与 DBC 同步 |
| **3. AI 行为改动** | 效果是决策而非数值（如改变技能优先级） | 在职业 AI 中按条件分支 | 属阶段 D 重点，需逐项确认 |

判定顺序：

```text
1. 能挂 DBC 光环 → 路径 1
2. 光环挂不上或不生效，或效果依赖 NpcBotStats 等自有计算 → 路径 2
3. 效果是 AI 行为而非数值 → 路径 3
```

约束：

- **优先路径 1**，因为它以 `Spell.dbc` 为权威，参数、持续时间、叠加规则不必手写，改 DBC 自动生效，与玩家行为一致。Bot 侧 `ApplyClassPassives()` 已是“应用被动法术”的可用先例，且 `RefreshAura()`（`bot_ai.cpp`）本身**已内建 apply/remove 对称语义**：

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

  即 `RefreshAura(id, 0)` 等价于“移除该效果”、`RefreshAura(id, 1)` 等价于“施加该效果”，天然幂等（重复调用不会叠加）。**效果层的 apply/remove 应直接复用该函数**，不要另写一套 `AddAura`/`RemoveAura` 配对逻辑。

  但须注意其 `count` 只接受 0/1（**不支持叠层**）。若某个天赋/雕文效果需要按 rank 叠层（如 `DEEP_WOUNDS_1/2/3` 三个不同 spell 分别代表 1/2/3 点），现有实现是**用不同 spell ID 表达不同层数并互斥刷新**（`bot_warrior_ai.cpp:2043-2045`），而非对同一 spell 叠 count。天赋 rank 关联的效果若采用该模式，`talent_data` 的 rank 变化需映射到不同的 spell ID，这一点在效果层设计时必须处理，不能假定“施加一次 spell 就能覆盖所有 rank”。

- **路径 2 等于把 DBC 数值抄进 C++**，存在“改了 DBC 但 Bot 仍用旧值”的漂移风险（见第 10 节）。仅在前两条前提不成立时使用。**注意：职业脚本中现有的 B 类硬编码（`ApplyClass*` 覆写里的数值字面量）本质上就是路径 2 的裸写版本**——迁移要做的是把它从“写死的数字”升级为“从生效构筑反查得到的值”，而不是在保留它的基础上再新增一套（见 4.2.1）。
- **无论走哪条路径，都必须标明该项对应哪个 Talent ID / Glyph ID / Spell ID**，作为可追溯锚点，否则后续调平衡时无从定位需同步之处。

### 4.2.1 与现有硬编码的关系：这是“替换”而非“新增”

**`ApplyClassPassives()` 只是载体之一。** 职业脚本中与天赋/雕文相关的硬编码分 3 类（实测规模见 2.4-⑤）：

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

> 上述代码块中的 `HasGlyphEquipped()` / `HasTalentRank()` / `GetGlyphEffectValue()` 仅为**形态示意**，用于说明“条件与数值都改由构筑提供”这一结构变化；具体接口名与签名在第 6.6 节统一确定。

**迁移的固定四步**（每项效果独立执行，四步在同批次内完成）：

1. **登记**：在台账中记录该效果在 3 类载体中的全部出现位置。
2. **建立**：让效果层能按生效构筑正确提供该效果（光环 / 数值 / 行为）。
3. **验证等价**：同一等级、专精、构筑下新旧行为对比一致，差异须有明确理由。
4. **删除**：删除该效果的全部旧硬编码语句，并复查无残留。

> **禁止“先加新层、旧硬编码以后再清”的分期做法。** 这与常规重构“先并行再切换”的经验相反，原因是 B 类残留不会互相覆盖、而是**双重累加**（见 4.2.2）：一旦并行就立即超模，且不报错、不崩溃，只表现为数值偏高，事后极难定位是哪些项在叠加。

**已有的中间态先例**：`bot_rogue_ai.cpp:194-296` 已把散落的 `RefreshAura(..., level/spec 条件)` 收敛为数据表 `RogueTalentDef RogueTalents[]`（`{spec, minLvl, maxLvl, auraId}`），`ApplyClassPassives()` 只遍历该表（`:2387-2389`），不再逐行写条件。这是**半程**，后续应继续推进：

| 盗贼当前做法 | 目标做法 |
|---|---|
| `uint8 minLvl / maxLvl` 等级半开区间 | 换成 `talentId:rank`，由生效构筑提供 |
| 天赋与雕文混在同一张表（如 `{ROGUE_ANY_SPEC, 15, 0, GLYPH_BACKSTAB}`） | 拆为天赋表与雕文表，雕文项由槽位状态驱动 |
| 仍用 `master->GetLevel()`（`:2383`） | 改 `me->GetLevel()`（见 2.4-②） |
| 只覆盖 A 类载体 | 把该职业的 B/C 类一并纳入台账 |

值得注意：`maxLvl` 半开区间（如 `SEAL_FATE1` 在 `[35,37)`、`SEAL_FATE2` 在 `[37,39)`）本质上是用**等级区间近似模拟天赋 rank**。改用 `talentId:rank` 后，这类区间判断可以整体消失，`GLYPH_BACKSTAB` 混在天赋表里的问题也随之解决。

**阶段 A 的核心产出是一份台账**，而不是“`GLYPH_*` 常量清单”：

| 字段 | 说明 |
|---|---|
| 效果名称 | 如 `Poleaxe Specialization`、`Glyph of Victory Rush` |
| 来源类型 | Talent / Glyph / 套装 / 技能自带（后两类不在本次迁移范围） |
| Talent ID 或 Glyph ID | 天赋项附目标 rank；雕文项附大/小类型与槽位 |
| 出现位置 | 全部命中位置，格式 `文件:行号`；可能 0–3 处（每类载体至多一处） |
| 目标路径 | 1 / 2 / 3（见 4.2） |
| 迁移后提供方式 | 光环 spell ID / 数值来源与系数 / 行为分支位置 |
| 状态 | 待迁移 → 已建立 → 已验证 → **已删除旧实现** |

最后两列是防“两套并行”的硬检查点：**一行只有在“已删除旧实现”勾选后才算完成**。

### 4.2.2 两类残留的后果不同，必须分别对待

“两套并行”的后果取决于载体类型，二者差别很大，直接决定迁移策略：

| | A 类（光环） | B/C 类（数值 / 行为） |
|---|---|---|
| 冲突机制 | `RefreshAura()` 内部先 `RemoveAurasDueToSpell` 再 `AddAura()`，**后调用者覆盖前者** | `crit_chance += N`、`pctbonus *= x` **累加/累乘，两处都生效** |
| 并行后果 | 效果**互相打架**：时有时无、在高等级莫名消失 | **双重累加**：直接超模 |
| 可察觉性 | 低——表现为“效果不稳定”，难归因 | 中——表现为“数值偏高”，但不知偏高在哪几项 |
| 是否报错 | 否 | 否 |
| 迁移要求 | 可以短暂并行（结果不稳定但不超模），但仍须尽快删除 | **绝不能并行**，必须先删后用 |

因此迁移的严格程度按载体区分：

- **A 类**：允许“先建后拆”，但两者之间的窗口必须尽可能短，且该窗口内不得发布。
- **B/C 类**：**先删除旧实现、再由新层提供**（或在同一提交内完成替换），不允许任何时刻两处同时生效。

> 这条区分也解释了为什么“逐项迁移 + 每项独立验证”比“整体重写”更可执行：B 类共 178 处雕文注释 + 大量天赋数值，一次性重写的回归面无法控制；而逐项迁移时，每项只有 1–3 个出现位置，删除是否完整可以精确核对。

### 4.3 构筑数据

每个构筑至少包含：

```text
bot_class
spec
plan_version
talent_data   -- 有序 talentId:rank 序列
glyph_data    -- slot:glyphId 列表
```

用 `talentId + rank` 而非只存天赋法术 ID：规则读取时再解析该 rank 对应 spell，并能报告 rank 缺失。加点顺序须显式保存，因为相同最终分配可能存在多个合法逐级路径。构筑带版本号，升级后的新版本在 Bot 重建或管理员明确刷新时迁移，不要每次启动无条件重置已有状态。

### 4.4 不复制完整天赋树

**不需要手工准备整棵树**——完整树在服务器 DBC 中，手工复制会造成数据重复、补丁漂移与前置不一致。

**需要准备默认构筑**：每个受支持专精应有完整的目标 Talent rank 分配与雕文默认选择。建议工具读取服务器 Talent 数据校验构筑：rank 合法、职业/树匹配、各层级前置满足、序列每个前缀都是合法部分分配。**不校验"总点数是否超预算"**——构筑点数不设上限，预算只决定低等级时截断多少（见 3.1）。即“树由服务器权威维护，构筑由项目维护”。

## 5. 按等级渐进应用

### 5.1 初始化或等级变化

**实际执行点**是 `bot_ai::SetStats(bool force)` 的 `if (force)` 块（`bot_ai.cpp:2438-2452`），而不是新增一个独立的等级事件回调。原因见 2.4-③：Bot 等级变化有多条路径（自然升级回调、自由 Bot 击杀累积、招募重召、装备变更、GM 命令），只有 `SetStats()` 被它们全部覆盖。现有序列：

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
2. 计算生效天赋点预算 `GetTalentPointBudget(level)` = 等级基准（`level - 9`）+ 额外天赋点，**不设 71 硬上限**（见 3.1）。该值是**截断阈值**：构筑点数超出它不报错，只应用前 `A(level)` 点；雕文槽按 20 级规则一次性全开。
3. 校验生效状态与等级、构筑是否一致；缺失或损坏则进入可重复的重建流程。**复用判据按档分化**（见 6.3）：自动档（`plan_source = 0/1`）为 `built_at_level == me->GetLevel()` **且** `applied_points == A(level)`；手动档（`= 2`）只看 `applied_points <= A(level)`。
4. 对缺失的等级逐级回放——**必须在 `InitSpells()` 之前完成天赋/雕文决策**，因为 `InitSpells()` 与 `ApplyClassPassives()` 需要读取“哪些技能因天赋/雕文而启用”，且既有注释已规定 `InitSpells()` 必须先于被动：
   - 更新职业技能学习状态与最高可用 rank。
   - **自动档**（系统默认 / 玩家预设）：等级 >= 10 时从加点序列取当前合法的下一点，每级最多消费 1 点，直到用满 `A(level)`。
   - **手动档**（玩家直编）：**不回放、不补点**。多出的预算保持未分配，等玩家自己配置（见 6.4 的“三条出口”）。
   - 额外天赋点不按等级节奏，在应用起点即计入可用预算（自动档会因此自动多点若干项）。
   - 激活新天赋 rank 赋予的技能/被动/光环。
   - 等级达到 20 时一次性装备全部 6 个雕文槽。
5. 结果构建完成后一次性刷新属性、被动、AI 技能表与需重算的冷却/资源，减少中间态闪烁。复用既有的 `InitSpells() → ApplyClassPassives() → InitHeals()` 重建序列，不要另建一套。
6. 保存生效状态；重复执行同一等级初始化不得重复叠加光环、重复消费点数或重复插入技能。

> **降级是唯一会被自动截断的情形**（见 6.4 的非对称规则）：预算变小后，两种档位都必须把 `applied_points` 压到 `A(level)`，否则会生效超出预算的点数。

**等级来源统一**：各职业 `ApplyClassPassives()` 的实现中有 **10 个玩家职业文件**使用 `master->GetLevel()`（玩家主人等级，精确清单见 2.4-②），与 Bot 真实等级存在系统偏差。本方案要求所有新判定以 `me->GetLevel()` 为准，并在阶段 B 把既有 `master->GetLevel()` 统一替换为 `me->GetLevel()`，作为独立子任务验证。

**性能**：`SetStats()` 在战斗中也可能被触发（如装备变更），因此构筑应用必须走快照复用快路径（复用判据见 6.3，命中即返回），避免每次重跑天赋校验。**注意手动档的复用判据刻意放宽**（不看等级），正是为了让高频触发的 `SetStats()` 不产生无意义重建。

### 5.2 构筑不合法时的回退

**先区分两类"不合法"，处理方式不同：**

| 类型 | 例子 | 处理 |
|---|---|---|
| **超出当前预算**（非错误） | 80 级设计的 120 点构筑用在 40 级 Bot 上 | **截断**：只应用前 `A(level)` 点，其余标记未生效，不报错。**自动档**（`plan_source = 0/1`）在升级后会自动补齐；**手动档**（`= 2`）保留差额为“未分配点数”，交由玩家处置（见 6.4） |
| **结构性非法**（配置错误） | 超 `maxRank`、层级未解锁、前置未满足、职业树不匹配、`talentId` 不存在 | **拒绝**：按下列流程处理，不静默越级分配 |

结构性非法的回退流程：

1. 记录诊断：职业、专精、等级、Talent ID、rank、缺失条件。
2. 跳过该点并尝试同级下一个合法候选；候选耗尽则保留未花点数。
3. 开发/测试环境可将配置错误升级为错误日志或启动校验失败；生产运行时避免因此导致 Bot 无法生成。
4. 不私自选择与专精无关的天赋作为回退，否则会掩盖配置缺陷。

## 6. 第二阶段：玩家自定义构筑与持久化

### 6.1 需求与边界

- 玩家可为自己雇佣的 Bot 设置每系天赋构筑。
- 预设**绑定玩家角色 + Bot 职业**（`owner + bot_class + spec`）：玩家换同职业的 Bot 时可用同一套预设。
- Bot 当前实际生效的天赋/雕文**绑定 `entry`**；被解雇（`owner` 归零、`IAmFree()`）后清空并恢复默认。
- **新雇佣的 Bot 一律使用系统默认构筑**；只有玩家**显式设置**后才改用自定义预设。不存在“继承玩家某套偏好”这种隐式行为。
- 玩家操作分**两种，互相独立**（详见 6.4）：**应用**——把界面上当前的编辑内容直接覆盖该 Bot 的生效构筑，**不写预设库**，属于临场调整；**保存为预设**——只把内容存入预设库，**不改变任何 Bot**。UI 上对应两个独立按钮，可单独执行也可组合执行。
- **玩家可以把 Bot 恢复为系统默认**（`ApplyBotDefaultPlan`）或**改用某套已存预设**（`ApplyBotTalentPreset`）。这两条是把 Bot 从“手动档”拉回“自动档”的出口（见 6.4），属于正常的玩家操作，不是仅供内部使用的兜底逻辑。早期版本写“没有独立的‘重置为默认’操作”，此处已修正。
- **升级时是否自动应用新点数取决于当前档位**：系统默认与玩家预设（自动档）会按新预算自动补齐；玩家直编（手动档）**不自动补点**，多出的点数保持未分配，由玩家自己配置，或改用上一条的两种出口。详见 6.4。
- 客户端天赋树 UI 由 `mod-ale`（AzerothCore Lua Engine）实现，不在本方案内。服务端负责数据模型、校验、应用/撤销、查询接口与事件钩子。
- 不模拟经济/社交：不要求洗点费用或雕文物品来源，但设置需显式动作（如 UI 提交），不自动洗点。

### 6.2 数据模型：三张表

| 表 | 库 | 主键 | 职责 |
|---|---|---|---|
| `npcbot_default_talent_plan` | **world** | `(bot_class, spec)` | 服务端静态默认构筑与雕文 |
| `characters_npcbot_talent_plan` | characters | `(owner, bot_class, spec, preset_index)` | 玩家预设，一系多套 |
| `characters_npcbot_applied_plan` | characters | `(entry, spec)` | Bot 某系当前实际生效的构筑快照 |

#### 表 1：服务端默认构筑（world 库）

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

#### 表 2：玩家预设（一系多套）

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

#### 表 3：Bot 生效构筑快照

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
  `updated_at`     BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`entry`, `spec`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

**`talent_data` 为什么存完整序列而不是截断结果**（与早期设计不同，此处已修正）：

因为**“玩家直编”的构筑（`plan_source = 2`）没有第二份副本**——它不写预设库，快照就是唯一存储。若只存截断结果，40 级时应用的 120 点方案会被截成 31 点落库，于是：

- Bot 升到 60 级时，玩家想在**自己原有方案**的基础上继续分配那 20 点也**无从下手**——尾部 89 点的具体内容已经丢了，只能从头重配；
- 手动档下升级**不会自动补点**（见 6.4），但“保留完整方案”正是让玩家能一键沿用自己的思路继续分配的前提。

这两点合起来决定了：快照必须保存**完整序列**，另用 `applied_points` 记录生效到第几项。注意“存完整序列”并不等于“自动补齐”——是否补齐由档位决定，与存储方式无关。

因此统一为：**快照存完整序列，`applied_points` 标记生效到第几项**。实际生效的天赋 = `talent_data` 的前 `applied_points` 项。这样任何来源都能在等级变化后按新预算重新截断、自动向后补齐。`plan_total_points` 是对完整序列求和的缓存，便于 UI 与 SQL 直查。

`plan_source` 有三档，其区别是本方案的关键之一：

| 取值 | 含义 | `preset_ref` | 完整构筑在哪 | 升级时 |
|---|---|---|---|---|
| `0` | 系统默认构筑（新雇佣、未设置，或玩家预设已失效后回落） | 恒 0 | 表 1 `npcbot_default_talent_plan` | **自动补齐** |
| `1` | 玩家**保存并应用**的预设 | 有效，指向该系第几套 | 表 2 该预设；**预设被删除则来源失效**，重置为 0 并回默认 | **自动补齐**（按该预设） |
| `2` | 玩家在 UI 上**直接编辑后点应用**，未保存为预设 | 恒 0 | **快照自身**（没有第二份副本） | **不自动补点**，记为未分配 |

后两类合称**自动档**（`0`/`1`），第 `2` 类为**手动档**；分档的用途与完整规则见 6.4。

`preset_ref` **仅供 UI 溯源显示（“当前来自防护系第 2 套”），不参与任何逻辑判断**；`plan_source != 1` 时必须为 0，否则 UI 会误报来源。

`0` 与 `2` 在“是否受预设库变动影响”上截然不同：前者随服务器默认构筑更新而重建；后者是玩家的一次性临场决定，不受任何预设增删影响——也因此**解雇后即丢失**（见 6.4）。

`plan_total_points` / `applied_points` 是**无上限模型的必要冗余**：构筑点数可以远超当前预算，`talent_data` 存完整序列而实际生效的只是它的前缀，所以需要 `applied_points` 明确“生效到第几项”，`plan_total_points` 让 UI 直接显示“已生效 31 / 共 120（等级不足）”，也便于 GM 排查“为什么这个 Bot 只有一半天赋”。二者均可由 `talent_data` 重算，属于缓存字段。

> **读取约定**：生效天赋 = `talent_data` 的**前 `applied_points` 项**。效果层与 UI 一律按这个前缀切，不要假定“`talent_data` 本身就是生效内容”。
>
> `applied_points` 的取值方式**按档不同**（见 6.4）：**自动档**（`plan_source = 0/1`）= `min(plan_total_points, A(等级))`；**手动档**（`= 2`）= 玩家最后一次确认的点数，**可以小于 `A(等级)`**，差额即“未分配点数”（`A(等级) - applied_points`）。
>
> **雕文不参与截断**：雕文槽只有“20 级前 0 槽 / 20 级起 6 槽”两态，因此 `glyph_data` 直接是“玩家选择的方案”，没有前缀概念。20 级以下的 Bot 会持有 `glyph_data` 但不生效，UI 应显示“达到 20 级后生效”；跨越 20 级时 `built_at_level` 变化已触发重建，不需要额外的失效标记。

**为什么单独建表而不加列到 `characters_npcbot`：**

1. `characters_npcbot` 是身份/归属表（owner、roles、spec、equips、miscvalues），读归属信息非常频繁，塞入两大段 LONGTEXT 会拖重每次读取。
2. 生效构筑**按专精**区分。加列只能存当前一份，切换专精后切回就丢；单独表按 `(entry, spec)` 可存多行。
3. 生命周期不同：归属信息长期驻留，生效构筑是派生态、会被频繁重建。分开后解雇清理只需删生效表。
4. 与模块既有惯例一致：`characters_npcbot_transmog`、`characters_npcbot_gear_set`、`characters_npcbot_settings` 都是旁挂表。

**为什么存快照而不引用预设：**

- **预设会被删**。只存引用则删预设后 Bot 失去合法来源，需额外兜底；存快照则删预设不影响运行中的 Bot。
- **预设会被改**。“编辑预设是否立即影响在跑的 Bot”在引用模型下必须回答且易混淆；快照模型下要生效就显式重算，行为明确。
- **等级截断是常态**。因为构筑点数不设上限，任何等级低于"配满所需等级"的 Bot 都只会生效一部分点；**实际生效的天赋本就是构筑的子集**，引用模型无法表达，快照天然表达（配合 `plan_total_points` / `applied_points` 显示差额）。
- **诊断友好**。直接查看这份数据即可，不必反查“当时引用的是哪套预设的哪个版本”。

#### 表 1 与表 2 的关系

`npcbot_default_talent_plan`（表 1）提供开箱即用的默认构筑；`characters_npcbot_talent_plan`（表 2）是玩家自己保存的预设库，**只在玩家显式应用某个已存预设时才被读取**。表 2 存多套是为了让玩家复用（例如把“团本坦”套同样应用到另一个 Bot），而不是让新 Bot 自动继承。

**表 2 不是“应用”的必经之路。** 玩家在 UI 上直接改点后点应用（`plan_source = 2`）**完全不经过表 2**，构筑内容只存在于表 3 的快照里。表 2 仅在玩家主动执行“保存为预设”或“应用某个已存预设”时才参与（见 6.4）。

#### `talent_data` 存什么

存天赋点分配明细，每项为三元组，序列化后按加点顺序排列：

| 含义 | 取值来源 | 说明 |
|---|---|---|
| `talentId` | Talent.dbc 的 TalentID（第 1 列） | **必须是天赋本身 ID，不是它给予的法术 ID** |
| `rank` | 已投入点数 1..maxRank | 与 `TalentEntry->RankID[rank-1]` 对应，可反查实际生效法术 |
| 顺序 | 加点先后 | 保证任意等级可按序截断重放 |

```
204:5 205:3 206:1 1122:5 1123:2
```

**不存**：天赋树结构（来自 Talent.dbc）、天赋法术 ID（可由 talentId+rank 反查）、普通训练师技能、雕文（独立字段 `glyph_data`）。

**同一字段在三张表里语义一致**（都存完整序列），差别只在表 3 多记一个生效前缀：

| 表 | `talent_data` 内容 | 点数 |
|---|---|---|
| 表 1 / 表 2（构筑来源） | **完整**加点序列 | **不设上限**，可远超 71 |
| 表 3（生效快照） | **完整**加点序列（与其来源一致） | 序列本身不设上限；**其中前 `applied_points` 项生效** |

统一为“完整序列”有两个好处：读取方不必为不同表写不同解析逻辑；快照不会因为截断而丢失尾部点数（对没有第二份副本的 `plan_source = 2` 是必需的）。

截断**只切尾、不重排**——因为序列本身保证每个前缀都是合法的部分分配（见 3.1），所以"取前 K 项"必然合法。这也是"必须有序"的实际用途。

两条硬约束：

- **必须存 talentId 而非 spellId**。同一天赋不同 rank 对应不同 spell（`RankID[]`），存 spell 无法回溯点数、校验前置与层级、正确洗点。
- **必须有序**。最终分配相同但路径可能不同；只有有序序列才能在低等级截断出合法子集并保证重建结果可复现。

#### `glyph_data` 存什么

存已装备的雕文，每项为 `slot:glyphId`：

| 含义 | 取值来源 | 说明 |
|---|---|---|
| `slot` | 0–5 | 与 `GlyphSlotEntry::Order - 1` 对齐（`Player.cpp:13794`）。**槽位 0/3/5 是大雕文槽，1/2/4 是小雕文槽**（实测，见 2.5.2） |
| `glyphId` | `GlyphProperties.dbc` 的 `Id` | **存雕文 ID 而非效果法术 ID**，效果法术由 `SpellId` 反查 |

```
# 槽位索引:glyphId     槽位类型：0/3/5=大雕文(TypeFlags 0)，1/2/4=小雕文(TypeFlags 1)
# 下列 ID 均已实测核对类型（示例混用职业仅为演示槽位↔类型对应关系）
0:52084  1:57856  2:57858  3:54810  4:57862  5:58357
```

槽位合法性校验必须包含 `GlyphProperties.TypeFlags == GlyphSlot.TypeFlags`（即大雕文只能进 0/3/5、小雕文只能进 1/2/4），与玩家侧实现一致（见 2.5.2、3.3）。**不存**雕文效果法术 ID（由 `SpellId` 反查）。

#### 效果技能从哪来（不需要手工准备）

天赋与雕文带来的**技能由服务器数据反查**，不需要为每个 rank、每个雕文单独维护技能清单：

| 数据 | 来源 | 说明 |
|---|---|---|
| 天赋树结构（节点/层级/前置/最大 rank） | `Talent.dbc` | 直接读，勿复制 |
| **天赋效果技能** | `Talent.dbc` 的 `RankID[]` | `spellId = TalentEntry->RankID[rank - 1]` |
| **雕文效果技能** | `GlyphProperties.dbc` 的 `SpellId` | `spellId = GlyphPropertiesEntry->SpellId` |
| 加点顺序 + 雕文选择 | 构筑表 | **唯一需要提前准备的内容** |

因此 `talent_data` 存 `talentId + rank` 就够——反查天然得到“该等级该天赋恰好应给哪个法术”，无需为“1/3 点”和“3/3 点”分别准备技能 ID；雕文同理存 glyphId 而非 spellId。

需要提前准备的只有两样：**加点顺序**（有序，理由见上）与**雕文选择**（从可用雕文里挑哪几个）。这两项是“选择”，不是“技能清单”。

### 6.3 读取与回退顺序

Bot 加载/重建时，先看快照可否复用；判据**按档分化**（自动档看等级与预算，手动档只看是否超预算），不可复用再按来源决定：

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
  - 自动档：`built_at_level == 当前等级` **且** `applied_points == A(等级)`，两者都满足才复用（理由见下条）。
  - 手动档：只要 **`applied_points <= A(等级)`** 就复用。**等级上升不触发重建**——因为手动档不补点，重建也改变不了任何东西，白白重跑一遍；只有降级（`applied_points > A(等级)`）才需要截断。
- **手动档下 `applied_points != A(等级)` 是正常状态**（未分配点数），不能当作“快照失效”的信号。若沿用自动档的判据，每次升级都会触发一次无意义的重建。
- **只有 `plan_source = 1` 才去读玩家预设表**；`= 2` 的构筑内容就在快照自身，且**不会因为等级变化而丢失尚未生效的尾部点数**（因为存的是完整序列）。
- **`built_at_level` 在手动档下只用于审计/显示**（记录玩家最后一次应用的等级），不参与重建决策。
- **先查快照，命中即复用**，避免每次加载重跑整套解析。
- **`spec` 决定读哪一系**：同一 Bot 不同专精各有一行快照，当前是防护系就读防护系。
- **自动档的复用判据不能只看等级**：`A(L) = (L-9) + 额外天赋点`，额外天赋点可能在等级不变的情况下变化（外部机制追加）。故需 `built_at_level == 当前等级` **且** `applied_points == 当前 A(等级)`。若只比等级，追加额外点数后快照会被错误复用，表现为“外部加了点数但 Bot 没有任何变化”。
- **解雇删除快照行**：`DELETE FROM characters_npcbot_applied_plan WHERE entry = ?`，下次雇佣回到系统默认。
- **解雇有两条路径，都要处理**（见 2.4-①）：`NPCBOT_UPDATE_OWNER`（`botdatamgr.cpp:3159`，批量置 0）与 `NPCBOT_UPDATE_ERASE`（`botdatamgr.cpp:3136`，删 `characters_npcbot` 行）。前者用 `WHERE entry IN (SELECT entry FROM characters_npcbot WHERE owner = ?)` 形式，且**必须在 `UPDATE owner` 之前执行**，否则子查询结果已为空。

### 6.4 设置与覆盖的语义

**“应用”与“保存为预设”是两个独立操作，不是一步。** 这是本节的核心，也是早期设计里写错的地方（原写法把“应用”等同于“存预设并应用”）。

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

- **“应用”是临场调整**：玩家点开界面、改几项、点应用，改动直接落到该 Bot。这份构筑**不写预设库**，快照是它的唯一副本，因此重建时按快照自身重新截断（见 6.3 第 3 步）。
- **“保存为预设”是单独操作**：把界面上当前的内容写入预设库并编号 `preset_index`，**不自动应用到任何 Bot**；保存完仍需（可选地）对某个 Bot 执行一次应用。
- **一次应用 = 一次“洗点 + 逐级重分配”**：撤销该 Bot 当前构筑的 Talent/Glyph 效果 → 校验 → 按当前 `A(等级)` 截断 → 重放 → 覆盖生效快照。
- **`applied_points` 的确定规则**：`applied_points = min(提交序列的点数, A(当前等级))`。提交序列的点数**由玩家自己决定**，不足预算也合法。这一条同时覆盖两种情况：
  - 40 级提交一份按 80 级配满的 120 点方案 → `min(120, 31) = 31`，未分配 0；
  - 40 级只点了 15 点就应用 → `min(15, 31) = 15`，未分配 16（**手动档**下这是正常状态，见下一小节）。
  两种都进**手动档**（`plan_source = 2`），因为提交内容来自玩家直编。
- **`plan_source` 由操作方式决定，不由内容决定**：即使玩家应用的恰好与某套预设内容完全相同，只要他是通过“应用”提交的，`plan_source` 就是 2、`preset_ref` 就是 0；只有走 `ApplyBotTalentPreset` 才是 1。**不要反查内容去匹配预设**——那样“来源”就不再反映玩家的真实意图，UI 溯源会撒谎。
- **`plan_source = 2` 的构筑在解雇后丢失**：没有第二份副本（见 6.3 第 3 步）。玩家若要保留须另存为预设；UI 建议在“应用”成功后提示可“保存为预设”。
- **保存的预设与应用的构筑都是完整方案，不因等级截断而裁剪**：玩家在 40 级环境下编辑时可以看到/提交超过 31 点的完整方案（例如按 80 级配满 120 点），**应用与保存走同一份校验逻辑，都接受完整方案**。系统只在实际生效时按 `A(当前等级)` 截取前若干点，UI 侧标注“已生效 X / 共 Y，剩余需提升等级或增加额外天赋点”。这样玩家不必为了低等级 Bot 而重做一份降级方案。
- **超出预算不是错误，不拒绝**：`A(等级)` 只决定应用多少点（见 5.2）。真正的拒绝条件只有结构性非法（超 `maxRank`、层级未解锁、前置未满足、职业树不匹配）。
- 结构性非法时**整体拒绝并返回错误码，绝不部分应用**，也不改动该 Bot 现有快照——避免留下"一半新方案一半旧方案"的中间态。
- **同一套预设可应用于多个 Bot**：对另一个同职业 Bot 执行设置并选择同一套预设即可，两者各自持有独立快照。
- **删除预设只影响 `plan_source = 1` 的 Bot**：那些 Bot 的生效值不受影响（在快照里），仅把 `plan_source` / `preset_ref` 重置为 `0`，下次因等级变化等原因重建时回到系统默认构筑。`plan_source = 0` 与 `= 2` 的 Bot 与预设库无关，删预设对它们没有任何影响。
- 操作需满足安全约束：Bot 不在战斗中、不属于进行中的实例关键流程、无待落地的 `_newspec`。可参照 `SwitchSpec()` 的非阻塞判定习惯。

#### 升级时是否自动应用新点数：自动档与手动档

**用户明确规则（2026-10-14）**：Bot 30 级时玩家修改了「按默认天赋配置」的天赋后，后续 Bot **升级不再自动应用新点数**；多出的点数交给玩家自己配置，或由玩家主动选择「恢复默认」/「使用预设」。

据此把 `plan_source` 分成两档：

| 档 | `plan_source` | 来源性质 | 升级时行为 |
|---|---|---|---|
| **自动档** | `0` 系统默认 / `1` 玩家预设 | 来源本身是一份**完整方案** | **自动**按新预算向后补齐，新点数按来源序列投入 |
| **手动档** | `2` 玩家直编 | 玩家手改过的**手稿** | **不自动补点**；多出的点数保持**未分配**，等玩家决定 |

分界依据很自然：默认构筑与预设都是“配满的完整方案”，玩家选用它们就意味着接受“按它配满”；而玩家手工调整过之后，系统不应擅自替他往后点，否则会覆盖他的意图。

**未分配点数** = `A(当前等级) - applied_points`（自动档恒为 0；手动档可 > 0）。它不是错误状态，而是“等玩家处置”的正常状态。

手动档下 `applied_points` 的语义因此变为“**玩家确认生效的点数**”，不再等于预算：

- 30 级预算 21 点，玩家只点了 15 点就应用 → `applied_points = 15`，未分配 6 点（天赋本来就可以点不满，合法）。
- 升到 31 级、预算变 22 → `applied_points` **仍是 15**，未分配变为 7 点。

**三条出口**——未分配点数由玩家决定怎么处理：

| 玩家想做什么 | 操作 | 结果 |
|---|---|---|
| 自己配置多余点数 | 打开界面（提示“X 点未分配”）→ 分配 → `ApplyBotPlan` | 仍为手动档 `2`，`applied_points` 更新 |
| **恢复默认** | `ApplyBotDefaultPlan(bot, spec)` | 回到自动档 `0`，此后升级**自动**应用新点数 |
| **使用预设** | `ApplyBotTalentPreset(bot, spec, presetIndex)` | 转为自动档 `1`，此后按该预设**自动**应用新点数 |

即：**只要回到自动档（默认或预设），自动应用就恢复**；停留在手动档则一直由玩家掌握。

> **`ApplyBotDefaultPlan` 是玩家可主动执行的正常操作**。早期版本写“没有独立的‘重置为默认’操作”，此处已修正——它正是本规则给出的三条出口之一。

**手动档下“存完整序列”的用途**：UI 可以提供“按当前方案补满剩余 X 点”的快捷操作，它等价于把 `applied_points` 提升到 `min(plan_total_points, A(等级))` 后再调一次 `ApplyBotPlan`。**这是玩家主动触发的操作，不是自动行为**——玩家也可以只补一部分，或完全不补。这正是 6.2 选择“存完整序列”而非“存截断结果”的另一个理由。

**非对称规则：升级不自动补，但降级必须截断。**

| 等级变化 | 自动档（`0`/`1`） | 手动档（`2`） |
|---|---|---|
| **升级**（预算变大） | 自动向后补齐到 `A(等级)` | **保持 `applied_points` 不变**，差额记为未分配 |
| **降级 / GM 改小**（预算变小） | 截断到 `A(等级)` | **同样必须截断到 `A(等级)`** |

手动档在降级时不能例外：若 `applied_points > A(等级)`，Bot 就会生效超出预算的点数，违反点预算的硬约束。降级截断会削减玩家此前的分配（预算不够，无法避免），UI 应提示“等级下降导致 X 点被撤销”。

**复用判据必须按档分化**（6.3 的第 2 步据此改写）：手动档下 `applied_points != A(等级)` 是**正常状态**，不能再作为“需要重建”的信号。

### 6.5 与现有专精机制的关系

现有实现中，`spec` 持久化在 `characters_npcbot.spec`，切换由 `bot_ai::SwitchSpec()` 施放 `ACTIVATE_SPEC`，在 `OnBotSpellGo()` 中落地为 `SetSpec(_newspec)` 并重建技能、被动与治疗；`GetSpec()` 在 10 级以下固定返回 `BOT_SPEC_DEFAULT`。

第二阶段不改变这套语义，只在其上叠加构筑：

- `spec` 仍是“当前专精”，是 AI 行为分支依据，也是生效快照的分区键。
- 构筑表示“该专精下天赋点与雕文怎么分配”，是技能/被动/雕文效果的来源。
- 玩家改构筑**不等于**换专精；换专精后该系若尚未设置过，回到系统默认构筑，玩家可再通过 UI 设置。
- 同一 Bot 在不同专精下**各有一行生效快照**，三系互不覆盖。
- 10 级以下保留默认行为：使用默认构筑，不加载玩家预设。
- 解雇清空该 entry 的生效快照；玩家预设按 owner + 职业存储，**不随解雇丢失**（便于复用到新雇佣的同职业 Bot）。

### 6.6 服务端接口

三处职责不同，查询不要合并：**默认构筑**（world 静态数据）、**玩家预设**（owner + bot_class + spec + preset）、**生效快照**（entry + spec）。

**写入路径也是两条，互相独立**（见 6.4）：**应用**只写生效快照、立即影响该 Bot；**保存为预设**只写预设库、不影响任何 Bot。不要把两者做成一个“保存并应用”的复合接口。

| 能力 | 建议形态 | 用途 |
|---|---|---|
| 查询天赋树 | `GetBotTalentTreeForClassSpec(botClass, spec)` 返回节点 + 层级 + 前置 + 最大 rank | 客户端渲染；数据来自服务器 Talent 数据 |
| 查询默认构筑 | `GetDefaultBotTalentPlan(botClass, spec)` | UI 展示/对比“系统默认”，也是未设置时的来源 |
| 列举预设 | `ListBotTalentPresets(owner, botClass, spec)` | UI 列出该系已存哪几套，供玩家选择应用 |
| 查询单个预设 | `GetBotTalentPreset(owner, botClass, spec, presetIndex)` | UI 回显某套内容 |
| 校验方案 | `ValidateBotTalentPlan(botClass, spec, plan)` 返回错误码与原因 | 只校验**结构性非法**（职业树/`maxRank`/层级/前置/`talentId` 存在性）。**点数超预算与超 71 都不是失败条件**，预算只用于截断 |
| 查询点预算 | `GetTalentPointBudget(bot)` / `GetBaseTalentPointBudget(level)` / `GetBonusTalentPoints(bot)` | UI 显示"当前可用 / 已用 / 构筑总数"；额外来源为外部集成点 |
| **载入编辑起点** | `GetBotPlanForEditing(bot, spec)` | UI 打开界面时取“当前生效构筑的**完整序列**”作为编辑内容；无快照时回退系统默认。同时返回 `plan_source`（显示“当前来自：系统默认 / 预设 N / 手动调整”）与 **`unassigned_points`**（手动档下的未分配点数，供 UI 提示“有 X 点未分配”） |
| **应用（UI 主入口）** | `ApplyBotPlan(bot, spec, talentData, glyphData)` | **玩家在界面上改完点“应用”所走的唯一入口**：校验 → `applied_points = min(提交点数, A(等级))` → 重放 → 覆盖生效快照（`plan_source = 2`、`preset_ref = 0`）。**不写预设库**，属临场调整。**应用后即进入手动档：此后升级不再自动补点**（见 6.4） |
| 应用（仅改雕文） | `ApplyBotGlyphPlan(bot, spec, glyphData)` | 只换雕文不改天赋的便捷入口；内部委托 `ApplyBotPlan`，天赋沿用当前生效的完整序列 |
| 保存预设 | `SaveBotTalentPreset(owner, botClass, spec, presetIndex, talentData, glyphData, name)` | **独立操作**：新增/覆盖一套预设，写**完整**构筑、不做等级裁剪、**不应用到任何 Bot**（见 6.4） |
| 删除预设 | `DeleteBotTalentPreset(owner, botClass, spec, presetIndex)` | 仅删预设；**只对 `plan_source = 1` 的 Bot** 重置 `plan_source`/`preset_ref` 为 0 |
| 应用预设到某 Bot | `ApplyBotTalentPreset(bot, spec, presetIndex)` | 把已保存的预设应用到该 Bot：`plan_source = 1`、`preset_ref = presetIndex`，按当前预算截断。**转为自动档：此后升级按该预设自动应用新点数** |
| 应用系统默认到某 Bot | `ApplyBotDefaultPlan(bot, spec)` | 让该 Bot 该系回到系统默认构筑（`plan_source = 0`）→ **回到自动档，之后升级自动补点**。这是玩家的正常操作之一（6.4 的三条出口） |
| 查询未分配点数 | `GetBotPlanForEditing` / `GetBotAppliedTalentPlan` 返回 `unassigned_points = A(等级) - applied_points` | 自动档恒为 0；手动档 > 0 时表示“玩家还有 X 点没分配”，UI 应提示可从 6.4 的三条出口中选一条 |
| 查询生效快照 | `GetBotAppliedTalentPlan(bot, spec)` | 返回来源（`plan_source` / `preset_ref`）、完整序列、`applied_points` / `plan_total_points` 与 `unassigned_points`；**生效部分 = 前 `applied_points` 项** |
| **查询效果状态（职业脚本用）** | `HasBotTalentRank(bot, talentId)` / `HasBotGlyphEquipped(bot, glyphId, slotType)` / `GetBotGlyphEffectValue(bot, glyphId, 效果类型)` | **职业脚本改造后访问天赋/雕文状态的唯一入口**：4.2.1 的 A/B/C 三类替换都调用它，避免各职业文件自行解析构筑数据 |
| 重算生效快照 | `ReconcileBotProgression(bot)` | 等级变化、雇佣、专精切换、解雇、显式刷新时调用 |
| 查询雕文 | `GetBotGlyphSlots(level)` / `GetBotAppliedGlyphPlan(bot, spec)` | 返回槽位与已装备雕文 |
| 变更事件 | 预设/快照变更后广播回调 | 供脚本、命令、UI 刷新，避免轮询 |

关键语义：

- **让 Bot 用上自定义构筑有两个入口，语义不同**：`ApplyBotPlan`（UI 直接编辑后点“应用”，`plan_source = 2`，不落预设库）与 `ApplyBotTalentPreset`（应用已保存的预设，`plan_source = 1`）。两者都作用于单个 Bot、都按当前预算截断、都可被 `ApplyBotDefaultPlan` 覆盖回默认。
- **应用与保存预设互不调用**：`ApplyBotPlan` 只写生效快照，`SaveBotTalentPreset` 只写预设库。两者接受的都是**完整**构筑、都不受当前等级预算裁剪。UI 上必须是两个独立按钮（组合方式见 6.4 的表）。
- **不要提供“保存并应用”的复合接口**：一旦合并，就无法表达“只应用不保存”这一最常用的临场调整语义，而 UI 已经天然是两次动作。
- **自动档与手动档的升级行为不同**（见 6.4）：`plan_source = 0/1` 升级时**自动**按新预算补齐；`= 2` **不自动补点**，多出的点数记为 `unassigned_points`。两者的**降级**都会被自动截断。任何新增的写入路径都必须先判定这一点，不能一律按“自动补齐”实现。
- **预算只截断、不拒绝**：任何写入路径都不因"点数超过当前预算"而失败（见 5.2）。
- **删除预设不打断运行中的 Bot**：仅对 `plan_source = 1` 者重置 `plan_source`/`preset_ref`，下次重建才回到系统默认；`plan_source = 0/2` 的 Bot 与预设库无关，不受影响（见 6.4）。
- **解雇删除生效快照**：在 `botdatamgr.cpp` 的 `NPCBOT_UPDATE_OWNER`（:3159）与 `NPCBOT_UPDATE_ERASE`（:3136）两处都执行删除。前者与已有的 `CHAR_DEL_NPCBOT_TRANSMOG_ALL` 清理放在同一事务，注意子查询须在 `UPDATE owner` 之前求值（见 2.4-①）。
- **职业脚本不再自行判断等级与专精**：A/B/C 三类载体迁移后，一律通过上表的“查询效果状态”接口获取天赋 rank 与雕文装备情况。这些接口是只读的、可 O(1) 命中的快路径，允许在战斗中被高频调用（`ApplyClass*` 修正函数每次伤害结算都会执行）。

实现位置：

- 数据访问与校验放 `BotDataMgr`（已承担 `NpcBotData` 加载/更新）；默认构筑在启动阶段随静态数据载入内存。
- 应用/撤销编排放 `bot_ai`（已负责 `InitSpells()`/`ApplyClassPassives()`/`InitHeals()` 的重建序列，见 `bot_ai.cpp:16050-16052`）。**统一入口 `ReconcileBotProgression()` 应挂在 `SetStats()` 的 `if (force)` 块内**，而不是新增独立的等级回调——因为 Bot 等级变化有多条不经过玩家升级回调的路径（见 2.4-③）。
- Lua 接口走 `mod-ale` 已有注册机制：在 `RegisterFunctions()`（`modules/mod-ale/src/LuaEngine/LuaFunctions.cpp:1898`）中按 `ALE*::Register` / `SetMethods` 模式注册为独立函数表，不让 Lua 直接读数据库。
- 也可按现有模块钩子风格暴露给 C++ 脚本（参考 `src/server/game/Scripting/ScriptDefines/*.h`，等级变化钩子为 `PlayerScript::OnPlayerLevelChanged`，`PlayerScript.h:261`），使同一套接口既能被 Lua 调用，也能被命令/脚本调用。
- **SQL 位置**：本项目对模块自定义表使用 `data/sql/custom/db_world/`、`data/sql/custom/db_characters/`（已有先例：`2024_10_17_00_characters_npcbot_settings.sql`、`2024_11_13_00_characters_npcbot_gear_set.sql`），采用 `SET FOREIGN_KEY_CHECKS=0; DROP TABLE IF EXISTS ...; CREATE TABLE ...;` 的幂等风格。`pending_db_*` 用于通过官方 updater 分发的修订，模块自有表走 `custom/` 更符合既有惯例。三张表按库分别为：默认构筑 → `custom/db_world/`；玩家预设与生效快照 → `custom/db_characters/`。

### 6.7 权限与安全

- **所有写入路径都须校验归属**：`ApplyBotPlan`（直编应用）、`ApplyBotGlyphPlan`、`ApplyBotTalentPreset`、`ApplyBotDefaultPlan` 都须确认调用者是该 Bot 的 **owner（或 `shared_owners` 中的玩家）**，不能仅凭 entry 写入。保存/删除预设则校验 `owner` 本人（预设按 owner 存储）。**直编应用同样要校验**——它是能立即改变 Bot 战斗数值的入口，不能因为“没入库”就放宽。
- 预设按 owner 存储，同一玩家的某职业某系预设对其实全局共享，可复用到多个同职业 Bot；不同玩家互不可见。
- **预设数量需上限**（如每系最多 5–10 套），`preset_index` 范围固定并校验。
- 数据体量需上限（条数、字符串长度、rank 范围），防止畸形数据导致解析开销或越界。
- 服务端**必须**完整校验，不信任客户端传入的点数与前置关系。
- 并发提交同一主键时以最后一次成功写入为准，保证内存与数据库一致；失败保持旧状态。
- 多 shared owner 场景下，预设以**实际 owner** 为准；建议先限制为 owner 本人可改。

## 7. 技能、天赋、雕文效果的迁移

### 阶段 A：清点与行为基线

**产出物是台账，不是常量清单。** 清点单位是“效果”，每个效果登记它在 3 类载体中的全部出现位置（字段见 4.2.1 末）。

- 建立**效果台账**：逐项登记 `效果名 / 来源类型 / Talent ID 或 Glyph ID / 出现位置(文件:行号) / 目标路径 / 迁移后提供方式 / 状态`。规模基线（10 个职业文件实测）：`ApplyClass*` 覆写 **160** 个函数、`RefreshAura()` **425** 处调用、`GLYPH_*` 常量 **53** 个、`//...Glyph...` 效果注释 **178** 处。
- 按载体分三张清单，便于分批执行：**A 类清单**（`ApplyClassPassives` 内的 `RefreshAura` 条件）、**B 类清单**（`ApplyClass*` 内的数值/时长/冷却硬编码）、**C 类清单**（AI 决策分支内的等级/专精条件）。
- 分类各职业 `InitSpells()` 中每个技能来源：训练师技能、基础技能、天赋技能、专精限制、形态/武器条件、内部辅助 spell。
- 逐项标注**非本次迁移范围**的项（套装加成如 `WARRIOR_T10_PROT_4P`、技能自带效果如 `RECKLESSNESS_1`、毒药/附魔等），避免误删。战士的 `ApplyClassSpellCritMultiplierAll`（`bot_warrior_ai.cpp:1226-1275`）是典型样本：其中同时混有天赋、雕文、套装与技能自带四类来源。
- 为关键等级与专精建立 AI 技能可用表 + 效果快照，作为回归基线（含 `GetBotArmorPenetrationCoef()` 等数值函数的返回值）。

### 阶段 B：集中技能等级与来源

本阶段只处理**技能**维度，不动天赋/雕文效果。

- 保留 `InitSpellMap()` 的 rank 链选择能力，将“是否可用”与“何时学习”分离。
- 将各职业硬编码的**技能**等级门槛迁移至职业技能映射，行为保持一致。
- **统一等级基准**：把各职业 `ApplyClassPassives()` 中的 `master->GetLevel()` 改为 `me->GetLevel()`（见 2.4-②）。此项须单独提交并做被动获取时间点的前后对比。
- 用来源标签防止多来源重复添加同一 spell；移除天赋/专精时只撤销对应来源，不误删职业基础技能。来源标签挂 `_spells` 的首级法术 ID 键（见 3.2）。

### 阶段 C：接入天赋分配（含删除天赋硬编码）

本阶段的完成条件**包含“天赋类硬编码已全部删除”**，不只是“新层已能提供效果”。

- 增加构筑与生效状态（内存 + 持久化），通过统一校验层解析 TalentEntry、等级、层级、前置 Talent、职业树与 rank。
- 将当前按 spec 直接授予的天赋技能逐步改为 Talent rank 派生。
- **按台账逐项迁移天赋效果，每项走完 4.2.1 的四步**（登记 → 建立 → 验证等价 → 删除）：
  - 删除 A 类天赋项：如 `DEEP_WOUNDS_1/2/3`、`TASTE_FOR_BLOOD1/2/3`、`FLURRY1-5` 的等级区间条件（`bot_warrior_ai.cpp:2043-2073`）。
  - 删除 B 类天赋数值项：如 `Poleaxe Specialization`（`:1215-1220`、`:1245-1250`）、`Sword and Board`（`:1265-1266`）、`Critical Block`（`:1261-1263`）、`Improved Overpower`（`:1258-1259`）、`Incite`（`:1253-1256`）。
  - 删除 C 类天赋行为分支。
- 兼容旧数据：旧 Bot 按原 spec 匹配默认构筑，首次加载一次性迁移；不改变原有专精 ID 语义。

### 阶段 D：雕文配置化（含删除雕文硬编码）

雕文的清理量在数量上大于天赋（B 类 178 处注释中绝大多数是雕文），是本阶段的主体工作。

- 为每个专精定义大/小雕文候选优先级；按 20 级规则装备。
- 新增统一 Glyph apply/remove 生命周期，按 4.2 的三条路径分类处理：优先 DBC 光环；挂不上或依赖 Bot 自有计算时用代码数值修正；改 AI 决策的走行为分支。
- **逐项删除旧的雕文硬编码**，两类分别处理：
  - A 类：`RefreshAura(GLYPH_*, level >= N ? 1 : 0)` 形式（`bot_warrior_ai.cpp:2085-2090` 等）。
  - B 类：`ApplyClass*` 内以注释 + 魔法数字表达的雕文效果，如 `Glyph of Victory Rush`（`bot_warrior_ai.cpp:1268-1270` `crit_chance += 30.f`）、DK 的 `Glyph of Frost Strike` 符文能量消耗、德鲁伊的 `Glyph of Starfall` 冷却缩减等。**这类残留会双重累加，必须先删后用（4.2.2）**。
- 每项须标注对应的 Glyph ID / Spell ID（见 4.2 约束），并更新台账状态至“已删除旧实现”。
- 对不适用或对 Bot 无意义的雕文，明确标为不支持/忽略，而非假装已装备生效。

### 阶段 D′：职业脚本零残留复查

阶段 C/D 完成后执行一次全量复查（这是“不许两套并行”的验收动作，不是可选步骤）：

- 在已迁移的职业文件中检索：`ApplyClassPassives()` 内不得再有等级/专精字面条件；`ApplyClass*` 内不得再有属于天赋/雕文来源的数值字面量。
- 允许保留的仅是阶段 A 明确标注的**非迁移范围**项（套装、技能自带、毒药/附魔等），且须保留注释说明为何保留。
- 复查结果与台账状态对账：台账中不存在状态停在“已验证”的行（必须推进到“已删除旧实现”）。
- 记录基线对比：阶段 A 的计数（160 / 425 / 53 / 178）应下降到仅剩非迁移范围项的数量，差额与台账条数一致。

### 阶段 E：专精切换、等级跳跃与持久化

- 专精切换时先停用旧构筑的 Talent/Glyph 效果，再装载新构筑并完整重算技能与被动。挂载点即 `SetSpec(activate=true)`（`bot_ai.cpp:16031`）的既有重建序列。
- Bot 等级可能被 GM/招募/脚本直接修改；所有入口应调用统一 `ReconcileBotProgression(bot)`，不依赖自然升级回调。**实现方式是把它挂进 `SetStats()` 的 `if (force)` 块**（见 2.4-③ 与 5.1）。
- 自由 Bot 击杀累积升级（`bot_ai.cpp:17052`）与招募重召（`botmgr.cpp:549`）这两条路径须重点验证，它们不经过 `Player::OnPlayerLevelChanged`。

## 8. 测试与验收

### 8.1 单元/数据验证

- 点预算边界：等级 1、9、10、11、79、80、81、82、83 的等级基准值分别为 0、0、1、2、70、71、72、73、74，**均属合法，不钳制**；`A(等级)` 单调不减且 10 级以下恒为 0（见 3.1）。
- 预算只截断不拒绝：声明 120 点的构筑用在 40 级 Bot 上**不报错**，只生效前 `A(40)` 点，`applied_points` 与 `plan_total_points` 正确记录差额；**自动档**升级后快照重建并向后追加点数，**手动档**升级后不追加（见 6.4）。
- 额外天赋点集成：额外为 0 时行为等于纯基准；注入非零额外点后预算正确抬高，且**不突破任何天赋的 `maxRank`/层级/前置**；额外点变化（等级不变）也能触发重建（复用判据含 `applied_points`，见 6.3）；配置上限 `BotTalentPointBudgetCap` 生效时按上限拒绝。
- 构筑校验：错误职业、错误树、超最大 rank、层级未解锁、前置不满足、缺少 Talent/Spell 数据 → 拒绝。**"超生效预算"不在此列**（属截断）。
- 序列前缀合法性：构造一个"总点数合法但中间某前缀非法"的构筑（如先点第 4 层再补够前置），应被拒绝。
- 技能 rank：等级门槛前后、rank 链断裂、多来源重复、专精移除后仍由其他来源有效。
- 雕文：19/20 级边界（19 级 0 槽、20 级 6 槽）、槽类型不匹配、重复雕文策略、专精切换的效果撤销/重加。
- **等级来源**：自由 Bot（`IAmFree()`）与带精英加成的雇佣 Bot，验证 `master->GetLevel()` → `me->GetLevel()` 切换后被动获取时间点符合预期（见 2.4-②）。
- 幂等性：连续初始化两次结果与一次相同；低级加载、升级、直升高等级、降级/GM 改级均有定义。
- **旧硬编码零残留检查（可自动执行）**：对已迁移的职业文件做静态检索，断言不含属于迁移范围的等级/专精字面条件。建议按载体分别建立检查规则，例如
  - `ApplyClassPassives()` 函数体内不应再出现 `level >=` / `lvl >=` / `GetSpec() ==` 形式的天赋/雕文条件；
  - `ApplyClass*` 修正函数体内不应再出现被台账标记为天赋/雕文来源的数值字面量（`crit_chance +=`、`pctbonus *=`、`cooldown -=` 等）；
  - 命中项必须能在台账的“非迁移范围”白名单（套装、技能自带、毒药/附魔）中找到对应条目，否则视为漏删。
- **台账一致性**：阶段 A 的基线计数（160 / 425 / 53 / 178）与复查后的剩余计数之差，应等于台账中状态为“已删除旧实现”的条目所覆盖的出现位置数。
- **应用与保存预设必须正交**（见 6.4）：只调 `ApplyBotPlan` → 预设库行数不变、该 Bot 快照变化且 `plan_source = 2`；只调 `SaveBotTalentPreset` → 预设库 +1、**所有 Bot 的快照与 `plan_source` 均不变**。四种组合各验一次。
- **`plan_source` 由操作方式决定，不由内容决定**：把某套预设的完整内容原样通过 `ApplyBotPlan` 应用，`plan_source` 必须是 2 而非 1（不得反查内容去匹配预设）。
- **手动档升级不自动补点**（用户明确规则，见 6.4）：40 级 Bot 应用 120 点直编构筑（生效 31 点）→ 快照应记录完整 120 点 → 升到 60 级 → **`applied_points` 保持 31 不变**、`unassigned_points` 变为 20，Bot 未因此获得新天赋。
- **档位三条出口**：同一 Bot 依次验证——手改应用后升级不补点；`ApplyBotDefaultPlan` 之后升级**自动**补齐；`ApplyBotTalentPreset` 之后升级**按该预设自动**补齐；再次 `ApplyBotPlan` 手改后又回到不自动。
- **手动档降级必须截断**（非对称规则）：60 级手动档 Bot（`applied_points = 51`）被 GM 降到 40 级 → `applied_points` 压到 31、`unassigned_points` 为 0，且未出现超预算生效。
- **手动档升级不触发重建**（验证复用判据按档分化）：手动档 Bot 每次升级不应产生快照重建（用计数或日志断言），否则每次升级都会白跑一遍解析。
- **手动档“一键补满”是玩家操作而非自动行为**：显式调用一次 `ApplyBotPlan`（把 `applied_points` 提升到 `min(plan_total_points, A(等级))`）后才补满；未调用时点数保持不变。

### 8.2 集成回归

每职业至少一个代表性专精，覆盖等级 9/10/15/19/20/30/40/50/70/80；对坦克、治疗、远程、近战各至少一条 AI 战斗场景验证：

- 技能表中只有等级与构筑允许的技能及最高 rank。
- 天赋被动/技能与点数记录一致，无非法前置；**自动档**生效点数等于 `min(构筑总点数, A(等级))`，**手动档**等于玩家确认的 `applied_points`（可小于预算，差额为未分配点数，见 6.4）。
- 雕文槽与已装备雕文一致，效果只应用一次；20 级前后槽位数量正确切换。
- 现有战斗 AI 仍按预期选技能，未因初始化顺序变更失去技能或保留旧专精效果。
- 旧数据库 Bot 首次加载可迁移，失败有可诊断日志且不阻断服务。
- 预设的加载、校验、应用与回退默认构筑的完整链路。
- 多预设解析：同系多套并存、应用预设与使用系统默认两条路径、删预设后运行中 Bot 继续用快照而重建时回到默认构筑、改专精后未设置的系回到默认构筑。
- 生效快照：**自动档**下等级提升后 `built_at_level`/`applied_points` 变化触发重建并向后追加点数（**手动档**下等级提升不触发重建，见 6.3）；解雇后快照行被删除；`preset_ref` 失效不导致异常。
- 超预算构筑全链路（**自动档**，即应用预设或默认构筑）：保存 120 点预设 → 40 级 Bot 应用 → 生效 31 点且 UI 显示差额 → 升到 80 级 → **自动**补齐到 71 点（超出 71 的部分由额外天赋点提供，见 3.1）。同场景若改走**手动档**（`ApplyBotPlan` 直编），升级后 `applied_points` 应保持不变。
- **数值等价性回归（B 类迁移专项）**：对每项迁移的数值效果，在固定等级/专精/装备下比对迁移前后的最终结果（如 `ApplyClassSpellCritMultiplierAll()` 输出的 `crit_chance`、`GetBotArmorPenetrationCoef()` 的返回值）。**若迁移后数值高于迁移前，优先怀疑旧硬编码未删除（双重累加），而不是新层算错**——这是 4.2.2 给出的诊断顺序。
- **专精切换与效果撤销**：切换专精后旧构筑的 A/B/C 三类效果全部失效、新构筑全部生效；连续切换 3 次以上结果稳定（验证无残留光环、无数值累积）。
- **直编应用全链路**：`GetBotPlanForEditing` 载入当前生效的完整序列 → 改几项 → `ApplyBotPlan` → 立即生效、其他 Bot 不受影响、预设库无新增 → 解雇该 Bot 后快照被删、重新雇佣回到系统默认（直编内容如期丢失）。
- **档位行为全链路**（对应 6.4 的用户规则）：30 级 Bot 跑默认构筑（自动档，`A(30) = 21`）→ 玩家在界面改几项并应用（转手动档）→ 升到 35 级（`A(35) = 26`）→ **Bot 天赋未自动增加，UI 显示“未分配 5 点”** → 分别验证三条出口：手改补满、`ApplyBotDefaultPlan`（之后升级自动补）、`ApplyBotTalentPreset`（之后按预设自动补）。

## 9. 分阶段落地

| 阶段 | 内容 | 产出 |
|---|---|---|
| A | 建立**效果台账**（不是常量清单）：160 个 `ApplyClass*` 覆写 / 425 处 `RefreshAura` / 53 个 `GLYPH_*` / 178 处雕文注释，逐效果登记 3 类载体中的出现位置与目标路径；区分非迁移范围项 | 效果台账 + A/B/C 三张分载体清单 + 技能可用表与效果快照基线 |
| B | 集中**技能**来源与等级判定，拆分 `InitSpellMap()` 职责；统一等级基准为 `me->GetLevel()` | 行为等价重构（等级基准变更需单独验证） |
| C | 引入构筑/生效状态与规则校验层，落地默认构筑；`ReconcileBotProgression()` 挂入 `SetStats()`；**逐项删除天赋类 A/B/C 硬编码** | 各专精默认构筑可按等级渐进应用，**且天赋硬编码已清零** |
| D | 雕文配置化 + 20 级全开槽位；**逐项删除雕文类 A 类（53 常量）与 B 类（178 注释中绝大部分）硬编码** | 统一 glyph apply/remove 生命周期（复用 `RefreshAura`），**雕文硬编码已清零** |
| D′ | **零残留复查**：检索已迁移文件中残留的等级/专精字面条件，与台账对账，核对计数下降幅度 | 无“两套并行”的可验证证据 |
| E | 三张表 + 校验/应用接口 + 钩子 + 解雇清理（两条路径） | 服务端能力完备（UI 待接入） |
| F | 接入 `mod-ale` 客户端 UI（独立工作项） | 玩家可操作的界面 |

阶段 A–D 让 Bot 更像玩家（**并把职业脚本里已有的硬编码替换掉**），阶段 E 让玩家能改 Bot。阶段 E 不依赖 UI，可先用 GM 命令/脚本验证。

> **排期提示**：C 与 D 的实际工作量主要在“删除并验证”而非“新增”，原因是 B 类硬编码多达数百处且每项都需要单独确认等价性。不要把 C/D 按“写新代码”的量级估时。

## 10. 风险与取舍

- **重复效果**（最高风险，且有两种截然不同的后果，见 4.2.2）：Talent/Glyph 集中应用后，原职业脚本里的硬编码同类效果必须同步移除，否则——
  - **A 类（光环）**：`RefreshAura()` 自身先 `RemoveAurasDueToSpell` 再 `AddAura`，同一 spell 被两处控制时**不会叠加，而是互相打架**（后调用者覆盖前者），表现为“效果时有时无 / 在高等级莫名消失”这类难查问题。
  - **B 类（数值修正）**：`crit_chance += N`、`pctbonus *= x` 是累加/累乘，两处都生效 → **直接双重累加、超模**。规模为 160 个 `ApplyClass*` 覆写、178 处雕文效果注释（多数连常量都没有，只能靠注释识别）、以及散布在同批函数中的天赋数值。
  - **B 类是本方案最主要的失败模式**：不报错、不崩溃、不产生日志，只表现为数值偏高；事后无法从现象反推是哪几项在叠加。因此 **B/C 类禁止并行，必须先删后用**。
  - 走 4.2 路径 2 的项还容易与 DBC 漂移（改了 DBC 但 Bot 仍用旧值），须标注 Talent/Glyph/Spell ID 以便同步。
- **顺序敏感**：天赋被动、形态技能、雕文替换技能对应用顺序敏感。现有序列已被注释锁定为 `InitSpells()` **必须先于** `ApplyClassPassives()`（`bot_ai.cpp:2448`），新建的应用逻辑不得打乱；固定顺序为“构筑决策 → `InitSpells()` → 天赋被动 → 专精技能 → 雕文”。
- **rank 与 spell 的多对一映射**：天赋 rank 变化在现有实现中以**不同 spell ID 互斥刷新**表达（如 `DEEP_WOUNDS_1/2/3`），而非同 spell 叠 `count`（`RefreshAura` 只接受 0/1）。效果层设计不能假定“一个 talent 对应一个固定 spell”，须按 rank 选择 spell 并撤销其余 rank 的 spell，否则会出现多个 rank 的光环同时存在。
- **数据漂移**：预设可能针对旧构筑/旧补丁。`plan_version` + 校验失败回退默认构筑可缓解。
- **兼容性**：存量 `characters_npcbot.spec` 数据必须继续可用；预设表为空时行为与现状完全一致。
- **等级基准变更的回归面**：`master->GetLevel()` → `me->GetLevel()` 会影响 10 个玩家职业文件的被动获取时间点（精确清单见 2.4-②），且自由 Bot 与雇佣 Bot 的表现差异不同。须按职业逐个建立“被动生效等级前后对比表”。
- **性能**：天赋树查询与校验在提交/进入时做，不每 tick 做；应用中只读缓存后的结果。由于 `SetStats()` 会在战斗中经装备变更等路径触发，快照复用判据（`built_at_level`）必须走 O(1) 快路径。
- **客户端耦合**：UI 由 `mod-ale` 实现，服务端接口需稳定且自描述（返回错误码而非仅布尔）。

## 11. 参考资料与使用边界

- [80.wowdb.cc WotLK 天赋模拟器](https://80.wowdb.cc/?talent)：天赋树浏览/人工构筑参考；不能替代服务器运行时 Talent 数据校验。
- [Icy-Veins WotLK Classic 职业指南](https://www.icy-veins.com/wotlk-classic/)：包含特定玩法的构筑、天赋解释与雕文建议。推荐方案不是唯一答案，且可能针对特定装备阶段或团队配置；默认构筑应注明定位与版本。
- [Icy-Veins 平衡德鲁伊构筑示例](https://www.icy-veins.com/wotlk-classic/balance-druid-dps-pve-spec-builds-talents-glyphs)：区分常规 PvE 与套装阶段构筑，说明不宜只维护“职业 → 唯一永久天赋”而忽略定位/场景。
- [Icy-Veins 法师练级示例](https://www.icy-veins.com/wotlk-classic/frost-single-target-mage-leveling-guide)：说明练级构筑按等级变化，并列出玩家侧雕文槽的等级节奏（本项目 Bot 改为 20 级全开）。
- 具体技能/训练等级、Talent 前置、雕文 ID 与效果，以本项目当前客户端版本 DBC/数据库及代码中的实际数据为准。外部攻略用于定义推荐构筑，不是服务器规则权威来源。

### 本地可用的权威数据源

| 数据 | 路径 | 用途 |
|---|---|---|
| Talent.dbc / TalentTab.dbc / GlyphProperties.dbc / GlyphSlot.dbc | `G:/wow/dbc/`（与 `E:/WoW Spell Editor/DBC_335_wotlk/` 内容一致） | 天赋树结构、职业归属、雕文效果与槽位校验；实测数据见 2.5.1 / 2.5.2 |
| 法术名与 rank 链 | `G:/wow/dbc_csv/Spell.csv`（中文名在 index 138，见项目长期约定）；`outputs/Spell.csv` | 法术名、效果字段 |
| **字段语义权威** | `src/server/shared/DataStores/DBCStructure.h` + **`DBCfmt.h`** | 结构体定义**与加载格式串**。判断字段顺序须看 `DBCfmt.h`（如 `TalentEntryfmt = "niiiiiiiixxxxixxixxixxx"`、`GlyphSlotfmt = "nii"`），不要只推断结构体 |
| 玩家侧参考实现 | `Player.cpp:13789 InitGlyphsForLevel()`、`PlayerStorage.cpp:5923-5943`、`SpellEffects.cpp:4601-4615` | 槽位解锁、雕文校验、类型匹配的既有正确写法 |
| 服务端加载点 | `DBCStores.cpp:307-308`（Glyph）、`:370-371`（Talent/TalentTab） | 确认表已被加载，可直接 `sTalentStore` / `sGlyphSlotStore` 查询 |

> 读 DBC 的注意事项：文件头固定 **20 字节**（`WDBC` + recordCount/fieldCount/recordSize/stringBlockSize，各 uint32）。按 24 字节读取会整体错位，ID 列会看似全为 0——本项目早期判读即因此出错（见 12.4 备注）。

## 12. 核查结论与遗留待定项

### 12.1 核查结论

本文档第 2 节原先的定性论断**全部成立**，且核查后确认若干处规模比原描述更大或更具体：

1. Bot 侧确实没有天赋树分配与雕文槽位。硬编码规模经逐文件统计为：10 个玩家职业文件中共 **160** 个 `ApplyClass*` 覆写、**425** 处 `RefreshAura()` 调用、**53** 个 `GLYPH_*` 常量、**178** 处雕文效果注释，且**分布在 3 类载体中**（见 2.4-⑤）。阶段 A 的清点范围须按此规模规划，且清点单位是“**效果**”而非常量。
2. `RefreshAura()` 已内建 apply/remove 对称与幂等语义，可直接作为效果层基础，**不需要另写一套光环管理**。
3. 玩家侧雕文槽规则（15/30/50/70/80 位掩码）与文档描述完全一致；本项目 20 级全开是有意偏离。
4. 天赋反查路径 `TalentEntry->RankID[rank-1]`、雕文反查路径 `GlyphPropertiesEntry->SpellId` 均在本地 DBC 中得到结构与数据双重验证。
5. **服务端确实加载 Talent.dbc / TalentTab.dbc / GlyphProperties.dbc / GlyphSlot.dbc**（`DBCStores.cpp:307-308、370-371`），运行时读取可行，无需自建天赋树静态数据。
6. **两项关键映射已实测确定**：`TalentTab.ClassMask = 1 << (classID - 1)` 且 `BOT_CLASS_*` 真实职业值等于 classID（2.5.1）；雕文 `TypeFlags` 0=大雕文 / 1=小雕文，槽位 0/3/5 为大、1/2/4 为小（2.5.2）。
7. **本方案必须重构职业脚本本身，不能只在新层里实现**。天赋与雕文效果分散在 3 类载体中，且**同一效果可能跨载体出现**（战士 `Critical Block` 同时在 `ApplyClassPassives()` 与 `ApplyClassSpellCritMultiplierAll()` 出现；`Poleaxe Specialization` 只在后者出现且拆成两处）。只做新效果层而不清旧硬编码，在数值类载体上会立即双重累加（见 4.2.2），且不会报错。

### 12.2 须在实施前确定的问题

| # | 问题 | 影响 | 建议 |
|---|---|---|---|
| 1 | 解雇清理挂载点原描述不准确（`NPCBOT_UPDATE_OWNER` 与 `NPCBOT_UPDATE_ERASE` 是两处） | 遗漏任一处会留孤儿快照行或阻止回到默认构筑 | 两处都加，且批量分支子查询须在 `UPDATE owner` 前求值（2.4-①） |
| 2 | `ApplyClassPassives()` 使用 `master->GetLevel()` 而非 `me->GetLevel()`（**10 个玩家职业文件**，另 `bot_ai.cpp:7721/7775`） | 直接冲击“按 Bot 等级渐进应用”的核心判据 | 阶段 B 统一基准，独立提交 + 逐职业回归对比（2.4-②，精确清单见 8 条） |
| 3 | **Bot 天赋点不套用玩家 71 点上限**；`等级 - 9` 只是等级基准，其上还有**额外天赋点**（外部机制提供，本方案只预留接口） | 若按玩家规则钳到 71 或把"超预算"当失败条件，会错误拒绝合法构筑 | 点预算实现为"等级基准 + 额外天赋点"的可加函数，不写死 71；预算只作**截断阈值**（见 3.1、2.4-④） |
| 4 | `RefreshAura()` 的 `count` 仅支持 0/1，不支持叠层；现有天赋按 rank 用不同 spell 互斥表达 | 效果层若假定“一个 talent 一个 spell”会残留多 rank 光环 | 效果层按 rank 选择 spell 并撤销其余 rank（第 10 节） |
| 5 | 统一入口的挂载点原描述为“不依赖自然升级回调”，但未指明落到何处 | 自由 Bot 击杀累积、招募重召等路径可能漏算 | 挂进 `SetStats()` 的 `if (force)` 块（2.4-③、5.1） |
| 6 | SQL 目录原写 `pending_db_*`，与项目模块自定义表惯例不符 | 表可能被官方 updater 流程意外处理或与其他模块不一致 | 走 `data/sql/custom/db_world/` 与 `data/sql/custom/db_characters/`（6.6） |
| 7 | **天赋/雕文效果分散在 3 类硬编码载体中**，且同一效果可能跨载体出现（2.4-⑤、4.2.1） | 只重构 `ApplyClassPassives()` 会造成“两套并行”：B 类数值残留**双重累加且不报错**，C 类行为分支与新层冲突 | 阶段 A 以“效果”为单位建台账、登记全部出现位置；B/C 类**先删后用**（4.2.2）；阶段 D′ 做零残留复查、与台账对账 |
| 8 | 等级基准误用范围原写“12 个职业文件”，实际为 **10 个玩家职业文件**（另 `bot_ai.cpp:7721/7775`） | 影响面估计偏差会打乱阶段 B 的回归排期 | 按 2.4-② 的精确清单逐个职业做“被动生效等级前后对比表” |
| 9 | **原设计把“应用”与“保存为预设”合并为一次操作**，缺少“只应用不保存”的入口（用户指出） | 无法表达最常用的“改几项试一下”场景；且 `plan_source` 只有两档、表 3 只存截断结果，导致玩家手改的方案在等级提升后**无从继续分配**（尾部内容已丢） | ①“应用”与“保存为预设”拆成两个独立接口（6.4/6.6）；②`plan_source` 增加 `2 = 玩家直编`；③表 3 改存**完整序列** + `applied_points` 作为生效前缀 |
| 10 | **原设计默认“升级即按预算自动补齐”，未区分玩家是否手改过**（用户明确） | 玩家 30 级手改构筑后，升级会被系统擅自继续加点、覆盖其意图；且手动档下 `applied_points != A(等级)` 是常态，沿用原复用判据会导致**每次升级都触发一次无意义的重建** | ①`plan_source` 分**自动档**（`0`/`1`，升级自动补齐）与**手动档**（`2`，不自动补点）；②新增 `unassigned_points = A(等级) - applied_points`；③**复用判据按档分化**（6.3）；④`ApplyBotDefaultPlan` 提升为玩家的正常操作（三条出口之一）；⑤**降级时两档都必须截断**（非对称规则） |

### 12.3 尚未验证、需进一步确认的事项

- **额外天赋点的来源机制**：本方案**只预留接口**（`GetBonusTalentPoints()`），不定义其供给方式（GM 配置、道具、精英加成、任务奖励等）、发放时机与是否设总量上限。这些属于外部集成，需在接入时另行设计；本方案需保证的只有"预算可加、不设 71 上限、预算变化能触发快照重建"。
- **默认构筑的权威内容**：本文只确定“需要为每个受支持专精准备目标 talent rank 顺序与雕文选择”，未确定具体内容。现有 4 份职业重构文档（`outputs/NPCBot/BOT职业/`：奥法、术士、牧师、盗贼）已包含各系天赋与逻辑设计，应作为默认构筑的输入来源而非另起一套。
- **默认构筑的目标点数**：既然不设上限，需明确系统默认构筑按多少点设计（例如"单系满 + 副系关键点"还是"三系配满"）。这决定额外天赋点需要补多少才能完全生效，属内容决策。
- **哪些雕文对 Bot 无意义**：需在阶段 A 逐项判定并标注“不支持/忽略”，不能默认全部可装备。

### 12.4 已解决的原待确认项

以下两项原列于 12.3，已通过实测本地客户端 DBC 确认，结论见 2.5.1 / 2.5.2：

| 原问题 | 结论 |
|---|---|
| `TalentTabEntry::ClassMask` 与 Bot 职业枚举的映射 | `ClassMask = 1 << (classID - 1)`（实测取值恰为 1/2/4/8/16/32/64/128/256/1024）；且 `BOT_CLASS_*` 的 10 个真实职业值**直接等于 classID**，无需映射表。30 个职业天赋系的 TalentTabID 已列表（2.5.1）。注意 `GetBotClassMask0()` 与 `GetBotClassMask1()` 是两套不同约定 |
| `GlyphSlot.dbc` 的 `TypeFlags` 具体含义 | `0` = 大雕文、`1` = 小雕文（经 10 个已知雕文中英文名交叉验证）。玩家 6 槽为 `Id` 21–26 / `Order` 1–6，其中槽位索引 0/3/5 为大、1/2/4 为小；`GlyphProperties` 中零星非 0/1 取值因等值比较天然被排除（2.5.2） |

> 实测方法备注：WDBC 文件头为 **20 字节**（magic `WDBC` + recordCount/fieldCount/recordSize/stringBlockSize 共 4 个 uint32）。按 24 字节读取会整体错位一格，导致 ID 列看似为 0——这是本项目早期把邻域 CSV/DBC 判读错误的原因之一，后续工具脚本须注意。

## 13. 附录：效果台账（阶段 A 交付物）

### 13.1 台账文件与方法

台账文件：`outputs/NPCBot/NPCBots职业脚本硬编码效果台账.csv`，字段定义见 4.2.1。

已按 4.2.1 的方法完成**战士职业**（`bot_warrior_ai.cpp`）的完整清点，作为其余 9 个玩家职业的填写范本。清点规则：

- **以“效果”为单位**，不以 `GLYPH_*` 常量为单位。一个效果在几类载体中出现，就登记一行、多个出现位置。
- 同一效果的不同数值表现合并为一行（如 `Poleaxe Specialization` 的暴击率与暴击伤害）。
- **非迁移范围的项也要登记**（套装、技能自带、Bot 平衡），否则零残留复查时会误判为漏删。

### 13.2 战士清点结果

| 维度 | 结果 |
|---|---|
| 效果条数 | **90** |
| 迁移范围内（天赋 63 + 雕文 21） | **84** |
| 非迁移范围（套装 3 + 技能自带 2 + 项目自定义 1） | **6** |
| 载体分布 | A 类涉及 45 处、B 类涉及 50 处、C 类 1 处（其中 6 条同时属于 A+B） |
| 跨载体效果 | `Improved Hamstring`、`Juggernaught`、`Bloodsurge`、`Critical Block`、`Glyph of Revenge` |
| 同效果多处 | `Poleaxe Specialization`（3 处）、`Unrelenting Assault`（2 处） |

### 13.3 从战士样本看出的三个规律

**① B 类多于 A 类**（50 处 vs 45 处），且 B 类的单处成本更高：A 类只需替换条件，B 类要确认数值来源、等价性与 DBC 漂移风险。

**② 绝大多数雕文效果只存在于 B 类，A 类里根本没有。** 战士 21 条雕文中，只有 6 条（`Heroic Strike` / `Revenge` / `Execution` / `Blocking` / `Vigilance` / `Devastate`）以 `RefreshAura(GLYPH_*, ...)` 形式出现；其余 15 条——`Victory Rush`、`Bloodrage`、`Sweeping Strikes`、`Resonating Power`、`Shockwave`、`Bladestorm`、`Spell Reflection`、`Rapid Charge`、`Last Stand`、`Whirlwind`、`Charge`、`Sunder Armor`、`Cleaving`、`Bloodthirst`、`Enduring Victory`——**只以 `ApplyClass*` 内的数值字面量形式存在**。

> 这意味着：如果按“改造 `ApplyClassPassives()` 里的雕文”来理解本方案，会漏掉 **15/21 ≈ 71%** 的雕文效果。这正是“必须重构职业脚本、不能两套并行”在数量上的依据。

**③ 非迁移范围的项与迁移项混在同一批函数里。** 战士样本中套装（T10 4P、T8 4P、T9 2P）、技能自带（`Recklessness`、`Bloodthirst`/`Mortal Strike` 怒气折扣）、Bot 平衡（英勇打击 -25%）共 6 条，全部位于与天赋/雕文相同的 `ApplyClass*` 函数内。不预先登记白名单，“零残留复查”就会误删。

### 13.4 其余职业的推进方式

- 沿用同一 CSV 字段追加行；`出现位置` 列必须写 `文件:行号`，供阶段 D′ 反向检索。
- **按职业分批提交**（一个职业一个批次），每批完成后立即做该职业的零残留复查，不要攒到最后统一复查。
- 战士批次可作为方法验证批：它的载体分布最全（A/B/C 三类齐备），跨载体与同效果多处的样本也都有。
