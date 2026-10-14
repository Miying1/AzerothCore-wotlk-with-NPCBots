# NPCBots 技能、天赋与雕文应用机制重构分析

> 范围：AzerothCore WotLK 3.3.5a 集成版 NPCBots。分析现有技能选择、专精、天赋与雕文效果的实现，并给出按 Bot 等级模拟玩家逐步获得技能、分配天赋点、解锁并装备雕文的落地方案。本文为设计文档，不含代码修改。

## 1. 结论摘要

1. 当前 NPCBots 没有完整的玩家式天赋树分配与可配置雕文系统。Bot 的“专精”只是一个选择值（`characters_npcbot.spec`）；技能可用性由各职业 AI 按等级和专精构建；部分雕文效果直接写在职业 AI 行为/数值中。
2. 采用“服务器规则数据 + 默认构筑 + 运行时按等级渐进应用”的混合方案：规则（天赋合法性、法术链、雕文属性）以服务器数据为准，构筑只记录“选什么”，运行时保留 Bot 自有的效果应用层。
3. 需要为每个受支持专精准备**默认构筑**（目标天赋 rank 顺序 + 雕文选择），但**不需要复制整棵天赋树**——树结构来自服务器 DBC。
4. 玩家侧基础时序：10 级起每级 1 点天赋，点数 = `等级 - 9`；雕文槽按本项目规则在 **20 级后全部开放**。Bot 自动执行这些操作，但必须保留独立、可查询、可重建的状态。
5. 第二阶段允许玩家为自己雇佣的 Bot 定制构筑，采用**四张表按职责分离**（第 6 节）：默认构筑放 world 库；玩家预设按 `(owner, bot_class, spec, preset_index)` 支持每系多套；Bot 实际生效的构筑**单独建表存完整快照**。客户端天赋树 UI 由 `mod-ale` 实现，不在本方案内。
6. 分阶段实施：先清点硬编码规则并保证行为等价，再引入构筑与渐进应用，随后落地玩家预设与钩子，最后接客户端 UI。不把 Creature 塞入 Player 的天赋/雕文流程。

## 2. 当前实现盘点

### 2.1 主要代码位置

| 领域 | 位置 | 当前职责 |
|---|---|---|
| Bot 通用初始化 | `src/server/game/AI/NpcBots/bot_ai.cpp` | `SetStats()` 调用 `InitSpells()`；`InitSpellMap()` 按等级沿法术 rank 链选最高可用等级；初始化专精、被动与技能表 |
| 专精持久化 | `src/server/game/AI/NpcBots/botdatamgr.h`、`botdatamgr.cpp` | `characters_npcbot.spec` 保存专精；`SetSpec()` 更新专精并触发技能/被动重建 |
| 职业技能规则 | `src/server/game/AI/NpcBots/bot_<class>_ai.cpp` | 各职业覆盖 `InitSpells()`，按等级、专精增删 AI 可用技能；不少天赋技能通过条件直接加入 |
| 玩家天赋/雕文参考 | `src/server/game/Entities/Player/Player.cpp`、`Player.h` | Talent rank、可用点数、天赋技能/光环处理、雕文槽位与校验、专精切换流程 |

### 2.2 已观察到的差异

- Bot 自维护 `_spells` 等 AI 技能表，不等同于玩家 SpellMap；等级变化时会重选 rank，但这不表示它逐级“学习”并保存了玩家技能。
- `spec` 是专精/角色策略标识，不等同于完整的三系 Talent rank 分配；技能门槛与天赋条件目前编码在职业 AI 内部。
- 职业 AI 中有 `Glyph of ...` 注释与对应行为，但未发现 Bot 雕文槽、雕文 ID、按专精持久化或通用装备/撤销流程；现有效果更接近“固定内置的雕文特性”。
- 不能直接套用玩家 API：玩家流程依赖 `Player` 的 TalentMap、SpellMap、槽位、数据库与网络更新。Bot 是 Creature/AI，应复用规则与校验思想，而非把 Player 成员函数当通用服务。

**含义**：当前系统只能做到“某等级可用某技能”，无法解释“哪一级学会、花了哪一点天赋、装备了哪个雕文、切专精如何撤销”。重构要把这三类状态分开建模。

## 3. 玩家式应用规则

### 3.1 天赋点数

```text
等级 < 10 ：0 点
等级 >= 10：可用总点数 = 等级 - 9
80 级     ：71 点
```

目标天赋必须满足 Talent.dbc 的职业/树归属、最大 rank、层级点数要求与前置天赋要求——仅校验总点数不够。逐级应用时每级最多新增 1 点，结果须在该等级预算内。Bot 若发生等级跳跃（如 20 直升 60），应补算 21–60 级的全部升级事件或等价逐点回放，不能在最终等级一次性忽略顺序校验。

### 3.2 技能训练

玩家达到技能需求等级后访问训练师学习。模拟无需还原金币、距离与点击，但须保留清晰的学习语义：

- 等级/前置条件以服务器 `trainer_spell` 与 Spell 数据为准，不用“每两级学一次”这类经验概括替代实际数据。
- 升级时计算新增可学技能并记录为可用；同一技能链只激活合适的最高 rank，低 rank 不重复进入主动技能选择。
- 种族、职业、武器/姿态/形态、专精与任务等限制需区分。首期可只覆盖 NPCBot 职业技能与当前 AI 实际支持的技能集。
- 技能循环仍由现有 AI 负责“何时施放”；学习系统负责“是否拥有、当前 rank、是否因专精/天赋/雕文启用”。

### 3.3 雕文槽与装备

本项目对 Bot 采用简化规则：**达到 20 级后大/小雕文槽全部开放**。

| 槽位 | Bot 解锁等级 | 数量 |
|---|---:|---:|
| 大雕文槽 1/2/3 | 20 | 3 |
| 小雕文槽 1/2/3 | 20 | 3 |

（玩家侧原规则为 15/30/50/70/80 分段，本项目不沿用。）

解锁槽位不等于自动拥有或装备雕文。Bot 可把“获得可用雕文集合”与“自动装备默认雕文”视为系统代办，但状态模型须区分：槽位已解锁、候选合法、某雕文已装备、效果已应用。切换专精或构筑时须先撤销旧雕文效果再应用新方案，避免光环重复或残留修正。

用集中判定函数取代散落的等级判断：

```cpp
// 20 级前 0 个，20 级起全部 6 个
uint8 GetUnlockedGlyphSlots(uint8 botLevel)
{
    return botLevel >= BOT_GLYPH_UNLOCK_LEVEL ? BOT_GLYPH_MAX_SLOTS : 0;
}
```

`BOT_GLYPH_UNLOCK_LEVEL = 20`、`BOT_GLYPH_MAX_SLOTS = 6` 作为集中常量，避免魔法数字散落到职业 AI。

## 4. 推荐目标架构

### 4.1 分层

```text
服务器规则层（Talent / Spell / Glyph / Trainer 数据）
          ↓ 查询、校验、解析
构筑层（默认构筑 / 玩家预设，按职业 + 专精 + 预设序号）
          ↓ 等级条件与加点顺序
Bot 生效状态（该 entry 该专精的实际天赋与雕文快照）
          ↓ 派生效果
效果应用层（被动、光环、数值修正、技能启用）
          ↓
职业 AI 战斗决策（保留现有施法循环）
```

职责边界：

- **规则层**：判断技能 rank、天赋合法性、点数与前置、雕文类型/槽位/等级限制。
- **构筑层**：定义“这个专精加点怎么加”，提供默认构筑与玩家预设。
- **生效状态层**：记录该 Bot 该专精实际应用了什么。必须可重建、可审计、幂等。
- **效果层**：集中管理 Talent/Glyph 带来的被动法术、光环与数值修正，要求 apply/remove 对称。
- **AI 层**：只关心当前有效技能与战斗决策，不重复编码天赋规则。

### 4.2 构筑数据

每个构筑至少包含：

```text
bot_class
spec
plan_version
talent_data   -- 有序 talentId:rank 序列
glyph_data    -- slot:glyphId 列表
```

用 `talentId + rank` 而非只存天赋法术 ID：规则读取时再解析该 rank 对应 spell，并能报告 rank 缺失。加点顺序须显式保存，因为相同最终分配可能存在多个合法逐级路径。构筑带版本号，升级后的新版本在 Bot 重建或管理员明确刷新时迁移，不要每次启动无条件重置已有状态。

### 4.3 不复制完整天赋树

**不需要手工准备整棵树**——完整树在服务器 DBC 中，手工复制会造成数据重复、补丁漂移与前置不一致。

**需要准备默认构筑**：每个受支持专精应有完整的目标 Talent rank 分配与雕文默认选择。建议工具读取服务器 Talent 数据校验构筑：总点数不超预算、rank 合法、职业/树匹配、各等级节点前置满足。即“树由服务器权威维护，构筑由项目维护”。

## 5. 按等级渐进应用

### 5.1 初始化或等级变化

1. 读取 Bot 职业、等级、当前专精、owner、构筑版本与已有生效状态。
2. 计算可用天赋点数上限 `max(0, level - 9)`；雕文槽按 20 级规则一次性全开。
3. 校验生效状态与等级、构筑是否一致；缺失或损坏则进入可重复的重建流程。
4. 对缺失的等级逐级回放：
   - 更新职业技能学习状态与最高可用 rank。
   - 等级 >= 10 时从加点序列取当前合法的下一点，每级最多消费 1 点。
   - 激活新天赋 rank 赋予的技能/被动/光环。
   - 等级达到 20 时一次性装备全部 6 个雕文槽。
5. 结果构建完成后一次性刷新属性、被动、AI 技能表与需重算的冷却/资源，减少中间态闪烁。
6. 保存生效状态；重复执行同一等级初始化不得重复叠加光环、重复消费点数或重复插入技能。

### 5.2 构筑不合法时的回退

严格校验，不静默越级分配：

1. 记录诊断：职业、专精、等级、Talent ID、rank、缺失条件。
2. 跳过该点并尝试同级下一个合法候选；候选耗尽则保留未花点数。
3. 开发/测试环境可将配置错误升级为错误日志或启动校验失败；生产运行时避免因此导致 Bot 无法生成。
4. 不私自选择与专精无关的天赋作为回退，否则会掩盖配置缺陷。

## 6. 第二阶段：玩家自定义构筑与持久化

### 6.1 需求与边界

- 玩家可修改**自己雇佣的** Bot 的每系天赋构筑。
- 预设**绑定玩家角色 + Bot 职业**（`owner + bot_class + spec`）：玩家换同职业的 Bot 时拉取同一套预设，三个战士 Bot 共用同一套战士预设。
- Bot 当前实际生效的天赋/雕文**绑定 `entry`**；Bot 被解雇（`owner` 归零、`IAmFree()`）后清空并恢复默认。
- 客户端天赋树 UI 由 `mod-ale`（AzerothCore Lua Engine）实现，不在本方案内。服务端负责数据模型、校验、应用/撤销、查询接口与事件钩子。
- 不模拟经济/社交：不要求洗点费用或雕文物品来源，但**重新分配需显式动作**（如 UI 提交），而非自动洗点。

### 6.2 数据模型：四张表

| 表 | 库 | 主键 | 职责 |
|---|---|---|---|
| `npcbot_default_talent_plan` | **world** | `(bot_class, spec)` | 服务端静态默认构筑与雕文 |
| `characters_npcbot_talent_plan` | characters | `(owner, bot_class, spec, preset_index)` | 玩家预设，一系多套 |
| `characters_npcbot_talent_preset_active` | characters | `(owner, bot_class, spec)` | 玩家为每一系指定的默认预设 |
| `characters_npcbot_applied_plan` | characters | `(entry, spec)` | Bot 某系当前实际生效的构筑快照 |

#### 表 1：服务端默认构筑（world 库）

```sql
CREATE TABLE `npcbot_default_talent_plan` (
  `bot_class`    TINYINT UNSIGNED NOT NULL,    -- BOT_CLASS_*
  `spec`         TINYINT UNSIGNED NOT NULL,    -- BOT_SPEC_*
  `plan_version` INT UNSIGNED NOT NULL DEFAULT 1,
  `preset_name`  VARCHAR(32) DEFAULT NULL,     -- 如 'Default'
  `talent_data`  LONGTEXT NOT NULL,            -- 有序 talentId:rank 序列
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
  `talent_data`  LONGTEXT NOT NULL,            -- 有序 talentId:rank 序列
  `glyph_data`   LONGTEXT DEFAULT NULL,        -- slot:glyphId 列表
  `created_at`   BIGINT UNSIGNED NOT NULL DEFAULT 0,
  `updated_at`   BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`owner`, `bot_class`, `spec`, `preset_index`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

#### 表 3：玩家默认预设指针

```sql
CREATE TABLE `characters_npcbot_talent_preset_active` (
  `owner`         INT UNSIGNED NOT NULL,
  `bot_class`     TINYINT UNSIGNED NOT NULL,
  `spec`          TINYINT UNSIGNED NOT NULL,
  `active_preset` TINYINT UNSIGNED NOT NULL DEFAULT 1,  -- 指向 preset_index
  `updated_at`    BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`owner`, `bot_class`, `spec`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

#### 表 4：Bot 生效构筑快照

```sql
CREATE TABLE `characters_npcbot_applied_plan` (
  `entry`          INT UNSIGNED NOT NULL,                 -- Bot creature entry
  `spec`           TINYINT UNSIGNED NOT NULL,             -- 该行对应哪一系
  `plan_source`    TINYINT UNSIGNED NOT NULL DEFAULT 0,   -- 0=系统默认, 1=玩家预设
  `preset_ref`     TINYINT UNSIGNED NOT NULL DEFAULT 0,   -- 来源预设序号，仅供 UI 溯源
  `is_override`    TINYINT UNSIGNED NOT NULL DEFAULT 0,   -- 1=玩家单独指定，不跟随默认指针
  `talent_data`    LONGTEXT NOT NULL,                     -- 已按等级截断的实际生效序列
  `glyph_data`     LONGTEXT DEFAULT NULL,                 -- 实际已装备雕文
  `built_at_level` TINYINT UNSIGNED NOT NULL DEFAULT 1,   -- 生成时 Bot 等级
  `updated_at`     BIGINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`entry`, `spec`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
```

**为什么单独建表而不加列到 `characters_npcbot`：**

1. `characters_npcbot` 是身份/归属表（owner、roles、spec、equips、miscvalues），读归属信息非常频繁，塞入两大段 LONGTEXT 会拖重每次读取。
2. 生效构筑**按专精**区分。加列只能存当前一份，切换专精后切回就丢；单独表按 `(entry, spec)` 可存多行。
3. 生命周期不同：归属信息长期驻留，生效构筑是派生态、会被频繁重建。分开后解雇清理只需删生效表。
4. 与模块既有惯例一致：`characters_npcbot_transmog`、`characters_npcbot_gear_set`、`characters_npcbot_settings` 都是旁挂表。

**为什么存快照而不引用预设：**

- **预设会被删**。只存引用则删预设后 Bot 失去合法来源，需额外兜底；存快照则删预设不影响运行中的 Bot。
- **预设会被改**。“编辑预设是否立即影响在跑的 Bot”在引用模型下必须回答且易混淆；快照模型下要生效就显式重算，行为明确。
- **等级截断**。80 级预设用在 40 级 Bot 上只生效前 31 点，**实际生效的天赋本就是预设的子集**，引用模型无法表达，快照天然表达。
- **诊断友好**。直接查看这份数据即可，不必反查“当时引用的是哪套预设的哪个版本”。

因此 `preset_ref` 仅作 UI 溯源（“来自防护系第 2 套”），**不参与任何逻辑判断**。

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

两条硬约束：

- **必须存 talentId 而非 spellId**。同一天赋不同 rank 对应不同 spell（`RankID[]`），存 spell 无法回溯点数、校验前置与层级、正确洗点。
- **必须有序**。最终分配相同但路径可能不同；只有有序序列才能在低等级截断出合法子集并保证重建结果可复现。

### 6.3 读取与回退顺序

Bot 加载/重建时，先看快照可否复用，不可复用再定位预设，先命中者生效：

```text
1. 取 bot 当前专精 spec（bot_ai::GetSpec()）
2. applied_plan 中 (entry, spec) 存在且 built_at_level == 当前等级
      是 → 直接复用该快照（结束）
3. 取 owner；owner == 0（自由 Bot）→ 跳到第 6 步
4. 定位预设序号：is_override = 1 用 preset_ref；
                 否则读玩家默认指针 active_preset（无记录视为 1）
5. 按 (owner, bot_class, spec, preset_index) 查玩家预设
      命中 → 校验 → 按等级截断 → 写入 applied_plan（plan_source=1）
6. 未命中或校验失败 → 用 npcbot_default_talent_plan 生成快照（plan_source=0，记诊断）
7. 默认构筑也缺失 → 用当前职业 AI 既有行为基线（兼容旧数据）
```

要点：

- **先查快照，命中且等级未变直接复用**，避免每次加载重跑整套解析。
- **`spec` 决定读哪一系**：同一玩家的同一职业可三系预设并存，当前是防护系就读防护系。
- **预设序号决定读哪一套**：允许同一系多套并指定某 Bot 用哪套。
- **`built_at_level`**：等级变化时可用点数变了，既有快照需重新截断，比较等级即可判断是否复用。
- **解雇删除快照行**：`DELETE FROM characters_npcbot_applied_plan WHERE entry = ?`。

### 6.4 重新分配的语义

- 保存预设视为一次“洗点 + 逐级重分配”：**撤销旧构筑的 Talent/Glyph 效果 → 写入玩家预设表 → 按新预设在该 Bot 当前等级内重放 → 写入生效快照**。
- 重放只用当前等级允许的点数；预设按 80 级设计而 Bot 只有 40 级时，只应用前 31 点，UI 侧体现“未完全生效”。
- 不允许点数超支强制应用。校验失败即整体拒绝并返回错误码，绝不部分应用。
- **改默认指针**后，所有处于跟随状态（`is_override = 0`）的 Bot 需重算。
- **单独指定某 Bot 用哪套**走同一流程，并置 `is_override = 1`。
- **删除预设**：正在使用它的 Bot 不受影响（生效值在快照里），仅 `preset_ref` 溯源失效。下次重建时才退回默认指针或系统默认构筑。
- 操作需满足安全约束：Bot 不在战斗中、不属于进行中的实例关键流程、无待落地的 `_newspec`。可参照 `SwitchSpec()` 的非阻塞判定习惯。

### 6.5 与现有专精机制的关系

现有实现中，`spec` 持久化在 `characters_npcbot.spec`，切换由 `bot_ai::SwitchSpec()` 施放 `ACTIVATE_SPEC`，在 `OnBotSpellGo()` 中落地为 `SetSpec(_newspec)` 并重建技能、被动与治疗；`GetSpec()` 在 10 级以下固定返回 `BOT_SPEC_DEFAULT`。

第二阶段不改变这套语义，只在其上叠加构筑：

- `spec` 仍是“当前专精”，是 AI 行为分支依据，也是生效快照的分区键。
- 构筑表示“该专精下天赋点与雕文怎么分配”，是技能/被动/雕文效果的来源。
- 玩家改构筑**不等于**换专精；换专精时按新专精重走读取流程。
- 同一 Bot 在不同专精下**各有一行生效快照**，切回原专精时快照有效即可复用；预设按 `(owner, bot_class, spec, preset_index)` 分条存储，三系互不覆盖。
- 10 级以下保留默认行为：使用默认构筑，不加载玩家预设。
- 解雇清空该 entry 的生效快照；预设按 owner + 职业存储，**不随解雇丢失**。

### 6.6 服务端接口

三处职责不同，查询不要合并：**默认构筑**（world 静态数据）、**玩家预设**（owner + bot_class + spec + preset）、**生效快照**（entry + spec）。

| 能力 | 建议形态 | 用途 |
|---|---|---|
| 查询天赋树 | `GetBotTalentTreeForClassSpec(botClass, spec)` 返回节点 + 层级 + 前置 + 最大 rank | 客户端渲染；数据来自服务器 Talent 数据 |
| 查询默认构筑 | `GetDefaultBotTalentPlan(botClass, spec)` | UI 对比“系统默认”，也是无预设时的回退来源 |
| 列举预设 | `ListBotTalentPresets(owner, botClass, spec)` | UI 列出该系已存哪几套 |
| 查询单个预设 | `GetBotTalentPreset(owner, botClass, spec, presetIndex)` | UI 回显某套内容 |
| 查询默认指针 | `GetActivePreset(owner, botClass, spec)` | 读取玩家默认预设 |
| 校验方案 | `ValidateBotTalentPlan(botClass, spec, level, plan)` 返回错误码与原因 | 提交前预校验；服务端提交时再校验一次 |
| 保存预设 | `SaveBotTalentPreset(owner, botClass, spec, presetIndex, plan, name)` | 新增/覆盖一套预设 |
| 删除预设 | `DeleteBotTalentPreset(owner, botClass, spec, presetIndex)` | 仅删预设；运行中的 Bot 不受影响 |
| 设置默认预设 | `SetActivePreset(owner, botClass, spec, presetIndex)` | 重算所有跟随状态的 Bot |
| 指定某 Bot 用哪套 | `SetBotAppliedPreset(bot, presetIndex)` | 让单个 Bot 偏离玩家默认，置 `is_override = 1` |
| 查询生效快照 | `GetBotAppliedTalentPlan(bot, spec)` | 返回实际生效的来源与 `preset_ref` |
| 重算生效快照 | `ReconcileBotProgression(bot)` | 等级变化、雇佣、专精切换、解雇、显式刷新时调用 |
| 查询雕文 | `GetBotGlyphSlots(level)` / `GetBotAppliedGlyphPlan(bot, spec)` | 返回槽位与已装备雕文 |
| 变更事件 | 默认构筑/预设/指针/快照变更后广播回调 | 供脚本、命令、UI 刷新，避免轮询 |

关键语义：

- **保存预设后影响相关已雇佣 Bot**：遍历该 owner 名下 `bot_class` 匹配的 Bot，判断其当前 `spec` 与来源是否受影响，逐个 `ReconcileBotProgression`。
- **改默认指针只影响跟随者**：不干扰 `is_override = 1` 的 Bot。
- **删除预设不打断运行中的 Bot**：仅将相关 `preset_ref` 置 0。
- **解雇删除生效快照**：在现有 `NPCBOT_UPDATE_OWNER` 分支（`botdatamgr.cpp`）执行删除，与已有的 transmog 清理放在同一处。

实现位置：

- 数据访问与校验放 `BotDataMgr`（已承担 `NpcBotData` 加载/更新）；默认构筑在启动阶段随静态数据载入内存。
- 应用/撤销编排放 `bot_ai`（已负责 `InitSpells()`/`ApplyClassPassives()`/`InitHeals()` 的重建序列）。
- Lua 接口走 `mod-ale` 已有注册机制：在 `RegisterFunctions()`（`modules/mod-ale/src/LuaEngine/LuaFunctions.cpp`）中按 `ALE*::Register` / `SetMethods` 模式注册为独立函数表，不让 Lua 直接读数据库。
- 也可按现有模块钩子风格暴露给 C++ 脚本（参考 `src/server/game/Scripting/ScriptDefines/*.h`），使同一套接口既能被 Lua 调用，也能被命令/脚本调用。
- SQL 位置：默认构筑走 `data/sql/updates/pending_db_world/`；其余三张走 `data/sql/updates/pending_db_characters/`。若沿用 `data/sql/custom/db_*` 约定，应保持幂等（`DROP TABLE IF EXISTS` + `CREATE TABLE`）风格。

### 6.7 权限与安全

- 只有 Bot 的 **owner（或 `shared_owners` 中的玩家）** 可提交对应构筑；须校验归属，不能仅凭 entry 写入。
- 预设按 owner 存储，同一玩家的某职业某系预设对其实全局共享；不同玩家互不可见。
- **预设数量需上限**（如每系最多 5–10 套），`preset_index` 范围固定并校验。
- 数据体量需上限（条数、字符串长度、rank 范围），防止畸形数据导致解析开销或越界。
- 服务端**必须**完整校验，不信任客户端传入的点数与前置关系。
- 并发提交同一主键时以最后一次成功写入为准，保证内存与数据库一致；失败保持旧状态。
- 多 shared owner 场景下，预设与指针以**实际 owner** 为准；建议先限制为 owner 本人可改。

## 7. 技能、天赋、雕文效果的迁移

### 阶段 A：清点与行为基线

- 分类各职业 `InitSpells()` 中每个技能来源：训练师技能、基础技能、天赋技能、专精限制、形态/武器条件、内部辅助 spell。
- 将职业 AI 内的 Glyph 注释逐项关联到 GlyphProperties/GlyphSlot 与真实效果，标记“已有等价硬编码效果/尚未实现/依赖战斗情境”。
- 为关键等级与专精建立 AI 技能可用表快照，作为回归基线。

### 阶段 B：集中技能等级与来源

- 保留 `InitSpellMap()` 的 rank 链选择能力，将“是否可用”与“何时学习”分离。
- 将各职业硬编码等级门槛迁移至职业技能/天赋映射，行为保持一致。
- 用来源标签防止多来源重复添加同一 spell；移除天赋/专精时只撤销对应来源，不误删职业基础技能。

### 阶段 C：接入天赋分配

- 增加构筑与生效状态（内存 + 持久化），通过统一校验层解析 TalentEntry、等级、层级、前置 Talent、职业树与 rank。
- 将当前按 spec 直接授予的天赋技能逐步改为 Talent rank 派生，复用现有被动/技能应用函数。
- 兼容旧数据：旧 Bot 按原 spec 匹配默认构筑，首次加载一次性迁移；不改变原有专精 ID 语义。

### 阶段 D：雕文配置化

- 为每个专精定义大/小雕文候选优先级；按 20 级规则装备。
- 新增统一 Glyph apply/remove 生命周期，区分直接光环、技能替换、数值修正与 AI 行为策略。
- 将现有硬编码 Glyph 效果标记为“已由集中雕文模块负责”后逐类删除，配回归测试，不一次性删除未确认覆盖的逻辑。
- 对不适用或对 Bot 无意义的雕文，明确标为不支持/忽略，而非假装已装备生效。

### 阶段 E：专精切换、等级跳跃与持久化

- 专精切换时先停用旧构筑的 Talent/Glyph 效果，再装载新构筑并完整重算技能与被动。
- Bot 等级可能被 GM/招募/脚本直接修改；所有入口应调用统一 `ReconcileBotProgression(bot)`，不依赖自然升级回调。

## 8. 测试与验收

### 8.1 单元/数据验证

- 天赋点公式边界：1、9、10、11、79、80 级；满级 71 点。
- 构筑校验：错误职业、错误树、超最大 rank、超点数、层级未解锁、前置不满足、缺少 Talent/Spell 数据。
- 技能 rank：等级门槛前后、rank 链断裂、多来源重复、专精移除后仍由其他来源有效。
- 雕文：19/20 级边界（19 级 0 槽、20 级 6 槽）、槽类型不匹配、重复雕文策略、专精切换的效果撤销/重加。
- 幂等性：连续初始化两次结果与一次相同；低级加载、升级、直升高等级、降级/GM 改级均有定义。

### 8.2 集成回归

每职业至少一个代表性专精，覆盖等级 9/10/15/19/20/30/40/50/70/80；对坦克、治疗、远程、近战各至少一条 AI 战斗场景验证：

- 技能表中只有等级与构筑允许的技能及最高 rank。
- 天赋被动/技能与点数记录一致，无超点或非法前置。
- 雕文槽与已装备雕文一致，效果只应用一次；20 级前后槽位数量正确切换。
- 现有战斗 AI 仍按预期选技能，未因初始化顺序变更失去技能或保留旧专精效果。
- 旧数据库 Bot 首次加载可迁移，失败有可诊断日志且不阻断服务。
- 预设的加载、校验、应用与回退默认构筑的完整链路。
- 多预设解析：同系多套并存、单独指定与跟随默认两条路径、删预设后运行中 Bot 继续用快照而重建时回落、改专精后读到新系预设。
- 生效快照：等级提升后 `built_at_level` 触发重建并重新截断；解雇后快照行被删除；`preset_ref` 失效不导致异常。

## 9. 分阶段落地

| 阶段 | 内容 | 产出 |
|---|---|---|
| 1 | 清点各职业硬编码门槛与雕文效果，建立行为基线 | 分类清单 + 技能可用表快照 |
| 2 | 集中技能来源与等级判定，拆分 `InitSpellMap()` 职责 | 行为等价重构，无功能变化 |
| 3 | 引入构筑/生效状态与规则校验层，落地默认构筑 | 各专精默认构筑可按等级渐进应用 |
| 4 | 雕文配置化 + 20 级全开槽位 | 统一 glyph apply/remove 生命周期 |
| 5 | 四张表 + 校验/应用/重置接口 + 钩子 | 服务端能力完备（UI 待接入） |
| 6 | 接入 `mod-ale` 客户端 UI（独立工作项） | 玩家可操作的界面 |

阶段 1–4 让 Bot 更像玩家，阶段 5 让玩家能改 Bot。阶段 5 不依赖 UI，可先用 GM 命令/脚本验证。

## 10. 风险与取舍

- **重复效果**：Talent/Glyph 集中应用后，原职业 AI 里的硬编码同类效果必须同步移除，否则双重加成。迁移以“逐类替换 + 回归对比”为原则。
- **顺序敏感**：天赋被动、形态技能、雕文替换技能对应用顺序敏感，固定“天赋被动 → 专精技能 → 雕文”顺序并测试。
- **数据漂移**：预设可能针对旧构筑/旧补丁。`plan_version` + 校验失败回退默认构筑可缓解。
- **兼容性**：存量 `characters_npcbot.spec` 数据必须继续可用；预设表为空时行为与现状完全一致。
- **性能**：天赋树查询与校验在提交/进入时做，不每 tick 做；应用中只读缓存后的结果。
- **客户端耦合**：UI 由 `mod-ale` 实现，服务端接口需稳定且自描述（返回错误码而非仅布尔）。

## 11. 参考资料与使用边界

- [80.wowdb.cc WotLK 天赋模拟器](https://80.wowdb.cc/?talent)：天赋树浏览/人工构筑参考；不能替代服务器运行时 Talent 数据校验。
- [Icy-Veins WotLK Classic 职业指南](https://www.icy-veins.com/wotlk-classic/)：包含特定玩法的构筑、天赋解释与雕文建议。推荐方案不是唯一答案，且可能针对特定装备阶段或团队配置；默认构筑应注明定位与版本。
- [Icy-Veins 平衡德鲁伊构筑示例](https://www.icy-veins.com/wotlk-classic/balance-druid-dps-pve-spec-builds-talents-glyphs)：区分常规 PvE 与套装阶段构筑，说明不宜只维护“职业 → 唯一永久天赋”而忽略定位/场景。
- [Icy-Veins 法师练级示例](https://www.icy-veins.com/wotlk-classic/frost-single-target-mage-leveling-guide)：说明练级构筑按等级变化，并列出玩家侧雕文槽的等级节奏（本项目 Bot 改为 20 级全开）。
- 具体技能/训练等级、Talent 前置、雕文 ID 与效果，以本项目当前客户端版本 DBC/数据库及代码中的实际数据为准。外部攻略用于定义推荐构筑，不是服务器规则权威来源。
