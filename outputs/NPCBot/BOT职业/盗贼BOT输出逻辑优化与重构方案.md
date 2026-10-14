# 盗贼 BOT 三系输出逻辑优化与重构方案

> 适用仓库：`AzerothCore-wotlk-with-NPCBots`
> 分析对象：`src/server/game/AI/NpcBots/bot_rogue_ai.cpp`（2066 行）
> 参考玩法基准：icy-veins WotLK Classic Rogue
> 技能数据来源：`outputs/Spell.csv`

**文档结构**

1. 代码定位与整体架构
2. 三系输出流程现状
3. 常规 PVE 输出逻辑优化方案
4. 代码重构方案
5. 验证方式
6. 结论
7. **补充：打断 / 必须控制 / 紧急生存**（抢占型行为，优先级高于输出）

**设计约束（来自评审反馈）**

- 打断**不做硬打断**：团队中可能有其他 BOT 打断同一目标，需协同去重；
- 紧急生存**只用"血量 + 是否被怪作为目标"**，不做掉血速率等复杂计算。

**v2 修订说明（本次审核修正）**：以下 6 处已按代码实况修正，落地前请优先阅读对应章节。

| # | 修正点 | 所在章节 |
|---|---|---|
| 1 | 打断协同改为**双层检测**（`HasQueuedAction` 抓"已排队" + `GetCurrentSpell` 抓"已读条"），并覆盖 channeled 法术——原稿只查 `GetCurrentSpell` 无法覆盖 150~900ms 排队窗口 | 7.1 |
| 2 | 天赋描述表 schema 改为 **`[minLvl, maxLvl)` 区间**（含"任意专精"哨兵值），原稿单 `lvl` 字段无法表达 `SEAL_FATE1~5` 的等级区间 | 4.4 |
| 3 | 明确 Vanish / Cloak 的**归属方案（A：并入生存段）**，消除与 `UpdateAI` 层 `CheckVanish`/`CheckCloakOfShadows` 的双重触发 | 4.3 / 7.4 |
| 4 | "团队 CC 目标"改为复用现有 **`IsPointedNoDPSTarget()`（raid marker）**，原稿"可由 gossip 设置"与现状不符 | 7.2 |
| 5 | Envenom 门控补充**具体 API**：`GetAura(DEADLY_POISON_PROC_1, me->GetGUID())->GetStackAmount()`，并强调**按施法者过滤** | 3.1 |
| 6 | 目标从"可测试性"改为"**行为可复现性**"，验证依赖 DPS 对比 + 场景回归（与单文件约束一致） | 4.5 / 五 |

---

## 一、代码定位与整体架构

| 项目 | 位置 |
|---|---|
| 盗贼 BOT AI | `src/server/game/AI/NpcBots/bot_rogue_ai.cpp` |
| 基类 | `bot_ai`（`bot_ai.h` / `bot_ai.cpp`） |
| 专精枚举 | `botcommon.h:337-339` — `BOT_SPEC_ROGUE_ASSASINATION=10` / `BOT_SPEC_ROGUE_COMBAT=11` / `BOT_SPEC_ROGUE_SUBTLETY=12` |
| 技能 ID 分区 | `RogueBaseSpells`(`:23`)、`RoguePassives`(`:83`)、`RogueSpecial`(`:137`) |
| 技能列表注册 | `Rogue_spells_damage`(`:171`)、`Rogue_spells_cc`(`:174`)、`Rogue_spells_support`(`:175`) |

### 1.1 决策入口结构

真正的"决策"只有 4 个函数：

```
UpdateAI(diff)                     :278  主循环 / 状态收集 / 防御检查
  ├─ DoNonCombatActions(diff)      :754  涂毒（唯一与天赋相关的"物品"逻辑）
  ├─ Counter(diff)                 :258  打断（Kick / Gouge / Blind）
  ├─ CheckSprint                   :981  疾跑
  ├─ CheckCloakOfShadows           :838  斗篷
  ├─ CheckVanish                   :822  消失
  ├─ CheckBlind                    :897  致盲
  ├─ CheckPreparation              :914  预备
  ├─ CheckTricksOfTheTrade         :943  嫁祸诀窍
  └─ Attack(diff)                  :336  ★ 全部输出决策集中于此（约 400 行）
```

**核心结论：当前实现是"单回合线性 if 瀑布"。**

`Attack()` 内约 40 个 `if (...) { if (doCast(...)) return; }` 依次串联，没有优先级表、没有场景分流、没有评分机制。单体 / 多目标 / 爆发 / 常规 / 保命逻辑全部混在同一条顺序执行链中，优先级完全由**代码书写顺序**隐含表达。

---

## 二、三系输出流程现状

### 2.1 天赋（专精）判定方式

专精通过 `GetSpec()` 读取。在 `InitSpells()`(`:1845`) 与 `ApplyClassPassives()`(`:1912`) 中，按「等级 + 专精」用 `InitSpellMap` / `RemoveSpell` 手动注册技能，并按等级段用 `RefreshAura` 模拟天赋被动：

```cpp
// InitSpells() :1884-1898
lvl >= 30 && isAssa ? InitSpellMap(COLD_BLOOD_1) : RemoveSpell(COLD_BLOOD_1);
lvl >= 50 && isAssa ? InitSpellMap(MUTILATE_1) : RemoveSpell(MUTILATE_1);
lvl >= 60 && isAssa ? InitSpellMap(HUNGER_FOR_BLOOD_1) : RemoveSpell(HUNGER_FOR_BLOOD_1);

lvl >= 20 && isComb ? InitSpellMap(RIPOSTE_1) : RemoveSpell(RIPOSTE_1);
lvl >= 30 && isComb ? InitSpellMap(BLADE_FLURRY_1) : RemoveSpell(BLADE_FLURRY_1);
lvl >= 40 && isComb ? InitSpellMap(ADRENALINE_RUSH_1) : RemoveSpell(ADRENALINE_RUSH_1);
lvl >= 60 && isComb ? InitSpellMap(KILLING_SPREE_1) : RemoveSpell(KILLING_SPREE_1);

lvl >= 20 && isSubt ? InitSpellMap(GHOSTLY_STRIKE_1) : RemoveSpell(GHOSTLY_STRIKE_1);
lvl >= 30 && isSubt ? InitSpellMap(HEMORRHAGE_1) : RemoveSpell(HEMORRHAGE_1);
lvl >= 30 && isSubt ? InitSpellMap(PREPARATION_1) : RemoveSpell(PREPARATION_1);
lvl >= 40 && isSubt ? InitSpellMap(PREMEDITATION_1) : RemoveSpell(PREMEDITATION_1);
lvl >= 50 && isSubt ? InitSpellMap(SHADOWSTEP_1) : RemoveSpell(SHADOWSTEP_1);
lvl >= 60 && isSubt ? InitSpellMap(SHADOW_DANCE_1) : RemoveSpell(SHADOW_DANCE_1);
```

**模型缺陷**

天赋被动并非"按等级解锁"，而是"按专精固定假想的单一 build"：

- 刺杀系固定假设 Seal Fate 5/5、Ruthlessness、Relentless Strikes、Murder 2/2 等；
- 战斗系固定假设 Combat Potency 5/5、Hack and Slash 5/5、Unfair Advantage 等；
- 天赋数值全部硬编码在 `SpellHitTarget()`(`:1444`) 与 `OnClassSpellGo()`(`:1334`)，例如：
  - Ruthlessness 触发率写死 `urand(1,100) <= 60`（`:1512`）
  - Relentless Strikes 写死 `irand(1,100) <= 20 * comboPoints`（`:1353`）
  - Seal Fate 在 `SpellHitTarget` 里通过 `SEAL_FATE_EFFECT`(`:1460`) 加 CP

**直接后果**

1. 无法支持混搭 build（例如 41/5/25 之类的非标准分配）；
2. 天赋点未分配时仍给满被动（`ApplyClassPassives` 中 `MURDER = isAssa && level >= 35` 无条件给满 2/2）；
3. 天赋数值散落在多处，调整一处需全文搜索。

### 2.2 刺杀（Assassination / Mutilate）流程

```
涂毒: MH = Instant Poison, OH = Deadly Poison          (:783 / :788)   ← 完全固定，不看装备/等级段
起手: Cheap Shot / Garrote / Ambush / Backstab          (:644)
循环:
  Hunger for Blood 保 buff（需目标流血）                 (:367)
  Slice and Dice（Rand < 110，必定施放）                 (:418)
  Rupture 补流血（需 hasHunger 才允许）                  (:579)
  Envenom（4+ CP，需目标带 Deadly Poison 特效）          (:613)
  Mutilate 攒 CP                                        (:728)
爆发: Cold Blood（> 3 CP 时按概率）                       (:623)、Vanish、Shadowstep
保命: Evasion / Cloak of Shadows / Vanish / Sprint
```

**问题**

- Envenom 判定极其绕（`:613-620`），依赖 `0x800000` flag 与 `energy >= 110` 硬编码；
- 刺杀核心机制 **Cut to the Chase** 在 `SpellHitTarget`(`:1684`) 用固定时长 `21000 + 3000 + 12000` 硬刷 SnD，而非"重算 5CP SnD 时长"，随天赋 / 雕文变化即失真；
- 未检查目标身上 Deadly Poison 的**层数**（只检查 aura effect flag），Envenom 收益判定不准确。

### 2.3 战斗（Combat）流程

```
涂毒: MH = Wound Poison, OH = Instant / Crippling     ← 与刺杀不同，但同样是硬编码树
起手: Cheap Shot / Garrote / Ambush / Backstab
循环:
  Blade Flurry（Rand < 50，需有溅射目标）    (:409)
  Slice and Dice（Rand < 110）               (:418)
  Adrenaline Rush（energy < 50）             (:565)
  Killing Spree（Rand < (70 - energy)）      (:383)   ← 概率随能量漂移，随机性大
  Sinister Strike（主 CP 生成）              (:720)
爆发: Blade Flurry + Adrenaline Rush 双开（但无配对时机控制）
```

### 2.4 敏锐（Subtlety）流程

```
涂毒: MH = Wound Poison, OH = Instant / Crippling   ← 与战斗共用同一棵树，无差异
循环:
  Shadow Dance（能量 / AR 条件）    (:632)
  Shadowstep                        (:439)
  Premeditation                     (:375)
  Hemorrhage（主 CP 生成，非匕首）  (:713)
  Backstab / Ambush（匕首）         (:653)
  Waylay 由 SpellHitTarget 补       (:1694)
```

### 2.5 现存缺陷清单

| # | 缺陷 | 位置 | 影响 |
|---|---|---|---|
| 1 | 优先级靠代码顺序隐含，且顺序混乱 | `Attack()` 全局 | `Dismantle` / `Blind` / `Gouge` 排在核心输出之前，PVE 浪费 GCD 与能量 |
| 2 | SnD 无条件刷新 | `:418` `Rand() < 110` | 剩余 20s 也照刷，白白浪费 1 CP |
| 3 | Finisher 在 CP > 0 即判定 | `:599` | "先到先得"而非计划性爆发，与 SnD / Rupture 生命周期脱节 |
| 4 | Envenom / Eviscerate 混合体 | `:613` | 用 `envescerate` 变量切换，两个技能的条件互相污染，可读性差 |
| 5 | 爆发技能概率化 | `:383`、`:565`、`:623` | Killing Spree / AR 可能长时间不放；无"集火 / 斩杀"配对 |
| 6 | 多目标判定极粗 | `:683-695` | `GetNearbyTargetsList(targets, 7, 1)` 后需 `size() > 2` 才用 FoK |
| 7 | 能量不足时整轮阻断 | `:692` | 能量 < FoK 消耗时直接 `return`，阻断后续所有单体循环 |
| 8 | 无 Expose Armor 支持 | `:1853`（注释掉）、`:26`（标 NYI） | 团队缺破甲时盗贼不能补，团队 DPS 损失 |
| 9 | 无 Shiv、无目标毒药层数检查 | `:199`（注释） | 无法维持目标身上的毒，对刺杀影响大 |
| 10 | 多目标时 Finisher 目标不切换 | `:682-695` | FoK 仅用于攒 CP，无 AOE 清 CP 策略 |
| 11 | 涂毒策略静态 | `:782-791` | PVP 无致残 / 脑麻痹逻辑；PVE 无 Wound（减治疗）时机控制 |
| 12 | `Rand()` 散布全文 | 20+ 处 | 行为不可复现、不可测试，用随机数掩盖优先级不清 |

> **对第 12 项的说明**：`Rand()` 需要分两类看待，不能一刀切全删。
> - **掩盖决策缺失型**（应去随机、改确定性）：`:383` Killing Spree、`:565` Adrenaline Rush、`:623` Cold Blood、`:684` Fan of Knives —— 这些位置的随机本质是"没想清楚什么时候该放"；
> - **刻意行为抖动型**（可保留，但收敛到统一入口）：`:352` 择机进潜行、`:740` BreakCC 脱身、`Counter()` 内的抖动 —— 属于"避免所有 bot 动作完全同步"的合理设计。
> 重构后只删除第一类，第二类统一走 `RogueRoll(chancePct)` 单一入口。

---

## 三、常规 PVE 输出逻辑优化方案

按 icy-veins WotLK Classic 的优先级范式，给出「单体常规 → 爆发 → 多目标」的目标逻辑。

### 3.1 刺杀（Assassination / Mutilate）

**毒药**：MH = Instant Poison IX；OH = Deadly Poison IX（5 层后视情况转 Wound）。

需新增"目标毒层检测"——Envenom 收益完全依赖 Deadly Poison 层数。

**单体优先级（常规）**

1. 保持 **Hunger for Blood**（需目标流血；无流血时先补流血）
2. 保持 **Slice and Dice**（剩余 < 4s 且 CP ≥ 1 时刷新，5 CP 优先）
3. 保持 **Rupture**（剩余 < 4s 且 CP ≥ 4，且预计可完整跳完）
4. **Envenom**（CP ≥ 4，且目标 Deadly Poison 层数 ≥ 4）
5. **Mutilate**（CP < 4，能量充足）
6. **Cold Blood** → 紧接 **Envenom**（CB 只配 Envenom，且仅 5 CP）
7. **Fan of Knives**（多目标时）

**门控修正**：Envenom 必须检查 `目标 Deadly Poison 层数 >= 4`，而非当前的 aura effect flag 检查。

**实现要点（必须明确，否则会卡住）**：Deadly Poison 是叠层的 `SPELL_AURA_PERIODIC_DAMAGE`，现有 `GetAuraEffect(...)` **拿不到层数**：

```cpp
// 现状（:617-619）：只判断 aura effect flag，无法得知层数
mytar->GetAuraEffect(SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE, SPELLFAMILY_ROGUE, 0x10000, 0x0, 0x0, me->GetGUID())
```

正确做法是按 **caster GUID 过滤** 后读叠层数：

```cpp
// 必须先按施法者过滤，否则会把其他 bot / 玩家上的毒算进自己的 Envenom 收益
uint8 stacks = 0;
if (Aura const* deadly = mytar->GetAura(DEADLY_POISON_PROC_1, me->GetGUID()))
    stacks = deadly->GetStackAmount();

bool envenomReady = (stacks >= 4);
```

> 注意两点：
> 1. `DEADLY_POISON_PROC_1`（2818）是 DBC 里的中毒触发 spell，不是涂毒用的 `DEADLY_POISON_1`（2823）；
> 2. 副手涂 Deadly 才有稳定叠层，与 4.6 的涂毒方案表联动。

### 3.2 战斗（Combat）

**毒药**：MH = Instant Poison IX；OH = Instant Poison IX（副手提速后可考虑 Deadly）；Wound 视减治疗需求。

**单体优先级（常规）**

1. 保持 **Slice and Dice**（刷新阈值 4s，5 CP 优先）
2. 保持 **Rupture**（仅当目标存活 ≥ 5 跳时）
3. **Eviscerate**（CP = 5，或斩杀阶段 CP ≥ 3）
4. **Sinister Strike**（CP < 5，能量 ≥ SS 消耗 + 15 余量）
5. **Riposte**（触发时，优先级高于 SS）
6. **Killing Spree**（爆发，仅在"CP ≥ 4 且 Blade Flurry 已在存续期"时前置）

**爆发（Burst）**

```
Blade Flurry + Adrenaline Rush 同时开（AR 在能量 < 25 时开）
  → 内层连续 Sinister Strike 刷 CP
  → AR 期间 Eviscerate 只用 CP = 5
  → Killing Spree 在 Blade Flurry 存续期内施放（斩杀期 / 团队增伤期）
```

> **与 4.3 主流程的一致性**：`TryBurstAbilities` 虽排在主流程第 4 段（早于维持段），但其内部**自带门控**——只在"CP 已攒够（≥ 4）"或"双技能 CD 对齐"时才消耗本回合；否则直接返回 `false`，让位给维持段。**爆发不是无条件前置**，避免与 3.x 各系"循环优先"的清单表述冲突。

### 3.3 敏锐（Subtlety）

**毒药**：MH = Instant Poison；OH = Deadly / Wound。

**单体优先级（常规）**

1. 保持 **Slice and Dice**
2. 维持 **Hemorrhage** debuff（团队物理增益）
3. **Backstab**（匕首，CP < 4，站背后）
4. **Eviscerate**（CP = 5）
5. **Premeditation** → **Shadowstep** 起手
6. **Shadow Dance** 爆发窗口

**爆发**：`Shadow Dance` + `Preparation`（重置 CD）+ `Shadowstep`。Shadow Dance 持续内优先 **Ambush**（潜行技），而非 Hemorrhage。

### 3.4 多目标（3+ 目标）统一逻辑

```
1. 保持 SnD（优先级不变）
2. Blade Flurry 开启（战斗系）
3. Fan of Knives（能量 >= 消耗，用于攒 CP）——目标数 >= 3 时替代单体攒 CP
4. 能量充裕时：Fan of Knives 攒到 4-5 CP → 对主要目标 Eviscerate / Envenom
5. 目标 >= 5 时：只用 Fan of Knives + SnD，不再刷 Rupture
```

**关键修复**：`:692` 分支在能量不足时 `return` 会阻断后续所有技能，应改为"跳过 FoK、继续单体循环"。

---

## 四、代码重构方案

### 4.1 目标架构：引入"输出优先级表"替代 if 瀑布

不修改基类，仅在 `rogue_botAI` 内引入三个概念：

```
1. RogueSnapshot   —— 一次性只读快照，本轮所有决策共享
2. 分段决策函数     —— 每段职责单一，返回 bool 表示已消耗本回合
3. CombatScenario  —— 分流器：SOLO / MULTI(3+) / BURST / EXECUTE / DEFENSIVE / EMERGENCY
```

### 4.2 一次性快照（消除重复计算）

原实现中 `energy` / `comboPoints` / `dist` / 各种 `GetAuraEffect` 在每条 `if` 里重复计算；新建快照结构统一收集：

```cpp
// 一次性快照：本轮决策共享的只读上下文
// 设计原则：只存"单次读取/单步判断"即可得到的字段，不做跨 tick 累计统计
struct RogueSnapshot
{
    Unit*  target;
    float  dist;
    int32  energy, maxEnergy;
    uint8  comboPoints;
    bool   stealthed, shadowDance, inCombat;
    DiminishingLevels stunDrv;

    // Buff / Debuff 生命周期
    bool   hasHfB;
    bool   hasSnD;      uint32 snDRemainMs;
    bool   hasRupture;  uint32 ruptureRemainMs;
    uint8  targetDeadlyStacks;   // 按 me->GetGUID() 过滤后的叠层数，见 3.1

    // 多目标分流依据
    uint32 enemiesIn7y;

    // 生存判定（极简：只用两个直接可读的输入）
    float  selfHealthPct;    // GetHealthPCT(me)，沿用 bot_ai 静态函数
    bool   hasAttackers;     // !me->getAttackers().empty()

    // 单步条件推导（一次读取 + 一次比较，无跨 tick 状态）
    bool   targetBleeding;   // 目标带 AURA_STATE_BLEEDING
    bool   isExecutePhase;   // 目标血量 < 35%
};
```

> **字段取值说明**
> - `hasAttackers` 语义 = "**当前是否有单位正在攻击我**"（`getAttackers()` 返回攻击者列表），
>   与"怪把我作为目标"(`GetTarget() == me`) 不完全等价，实现时统一用前者，勿混用；
> - `selfHealthPct` 用 `GetHealthPCT(me)`（现有代码惯例），不要用 `me->GetHealthPct()` 混写。

> **刻意不包含的字段**（避免复杂计算 / 跨 tick 统计）：
> - `incomingDpsRank`（掉血速率分级）—— 需要跨 tick 采样统计，不做；
> - `attackersCount`（攻击者数量用于动态阈值）—— 只需知道"有无"，用 `hasAttackers`；
> - `reservedEnergy`（预留能量）—— 不做能量预留机制。

### 4.3 分段决策函数

除 6 个输出分段外，新增 3 个"抢占段"（打断 / 必须控制 / 紧急生存），详见 **第七节**：

```cpp
// —— 抢占段（优先级高于输出）——
bool TryEmergencySurvival();                       // 紧急生存：血量 + 有无怪，两条件
bool TryMustControl();                             // 必须控制：团队 CC / 灭团技 / 逃跑目标
bool TryDefensiveUtilities();                      // 常规生存：Evasion / Cloak / Vanish

// —— 输出段 ——
bool TryBurstAbilities(RogueSnapshot const&);      // 爆发段（自带 CP / CD 门控，见 3.2）
bool TryMaintainBuffs(RogueSnapshot const&);       // SnD / Rupture / HfB / Hemo 维持段
bool TryMultiTarget(RogueSnapshot const&);         // 多目标段（仅 3+ 敌人）
bool TryFinishers(RogueSnapshot const&);           // 清 CP 段
bool TryBuilders(RogueSnapshot const&);            // 攒 CP 段
```

**关于现有 `UpdateAI` 层 Check 函数的归属（必须明确，否则会双重触发）**

现状 Vanish / Cloak / Sprint 的判定**不在 `Attack()` 内**，而在 `UpdateAI()` 层且早于 `Attack()` 执行：

```cpp
// bot_rogue_ai.cpp:316-318
CheckSprint(diff);
CheckCloakOfShadows(diff);
CheckVanish(diff);
```

因此本节的重构**必须二选一，不能两处并存**（否则同一 tick 会有两条独立阈值链）：

| 方案 | 做法 | 取舍 |
|---|---|---|
| **A（推荐）合并** | 删除 `CheckVanish` / `CheckCloakOfShadows`，把其逻辑并入 `TryEmergencySurvival` + `TryDefensiveUtilities`，统一在 `Attack()` 内按 7.3 固定阈值判定 | 阈值单点维护，优先级最清晰；但 `CheckVanish` 原有一条"无目标也可逃"路径（`Attack()` 无 `mytar` 时会 `return`，见 `:339`），需在 `UpdateAI` 层保留一个极简兜底调用 |
| **B 保留分层** | `CheckVanish` / `CheckCloakOfShadows` 留在 `UpdateAI` 层（即**先于** `TryEmergencySurvival`），`TryEmergencySurvival` 只处理 Evasion 等 | 改动量小；但"紧急生存最先判断"（7.4）在实现上不成立，文档表述需改为"UpdateAI 层生存检查 → Attack 内输出段" |

**本方案采用 A**：`TryEmergencySurvival` 承担 Cloak → Vanish → Evasion 的固定分档（7.3），`Attack()` 无目标时的 Vanish 兜底保留在 `UpdateAI` 层（沿用现有 `CheckVanish` 的 `me->getAttackers().empty()` 前置判断）。

主流程改造为（**注意顺序：生存与控制先于爆发**）：

```cpp
void Attack(uint32 diff)
{
    RogueSnapshot const snap = BuildSnapshot(mytar, diff);

    // 1) 紧急生存：最高优先级
    if (TryEmergencySurvival())       return;

    // 2) 必须控制：团队 CC / 灭团技 / 逃跑目标
    if (TryMustControl())             return;

    // 3) 常规生存 / 机动（非紧急）
    if (TryDefensiveUtilities())      return;

    // 4) 爆发窗口
    if (TryBurstAbilities(snap))      return;

    // 5) Buff 维持（SnD / Rupture / HfB）
    if (TryMaintainBuffs(snap))       return;

    // 6) 多目标（3+）
    if (snap.enemiesIn7y >= 3 && TryMultiTarget(snap)) return;

    // 7) 清 CP
    if (TryFinishers(snap))           return;

    // 8) 攒 CP
    if (TryBuilders(snap))            return;
}
```

> **打断（`Counter()`）不在此列**：它在 `UpdateAI()` 层、`Attack()` 之前独立执行，
> 且需要"团队协同"判断，详见 **第七节 7.1**。

**收益**

- 优先级由函数调用顺序**显式表达**，改动只影响单段；
- `RogueSnapshot` 消除全文 30+ 次重复的 `GetAuraEffect` 调用；
- `Attack()` 从 400 行 if 瀑布拆为 8 个职责单一函数；
- 生存与控制被显式提到爆发之前，修正了原实现"爆发优先于保命"的错误。

### 4.4 天赋系统重构：从"硬编码单 build"到"天赋描述表"

**schema 必须支持"等级区间"，不能只用一个最低等级字段。**

现有 `ApplyClassPassives()` 里大量条目是**区间**语义（同一天赋按等级给不同 rank），单个 `lvl` 字段表达不了：

```cpp
// bot_rogue_ai.cpp:1925-1929 —— 典型区间结构
RefreshAura(SEAL_FATE5, isAssa && level >= 45 ? 1 : 0);
RefreshAura(SEAL_FATE4, isAssa && level >= 42 && level < 45 ? 1 : 0);
RefreshAura(SEAL_FATE3, isAssa && level >= 39 && level < 42 ? 1 : 0);
RefreshAura(SEAL_FATE2, isAssa && level >= 37 && level < 39 ? 1 : 0);
RefreshAura(SEAL_FATE1, isAssa && level >= 35 && level < 37 ? 1 : 0);
```

因此表结构改为 **`[minLvl, maxLvl)` 半开区间**（`maxLvl = 0` 表示无上限）：

```cpp
// 天赋描述表：(专精, 最低等级, 等级上限(0=无上限), 被动光环 id)
struct RogueTalentDef
{
    uint8  spec;
    uint8  minLvl;
    uint8  maxLvl;   // 0 => 无上限
    uint32 auraId;
};

static constexpr RogueTalentDef RogueTalents[] =
{
    { BOT_SPEC_ROGUE_ASSASINATION, 35,  0, MURDER },       // 2/2，35+ 恒定
    { BOT_SPEC_ROGUE_ASSASINATION, 45,  0, SEAL_FATE5 },   // 45+
    { BOT_SPEC_ROGUE_ASSASINATION, 42, 45, SEAL_FATE4 },   // [42,45)
    { BOT_SPEC_ROGUE_ASSASINATION, 39, 42, SEAL_FATE3 },   // [39,42)
    // ... 其余天赋
};
```

`ApplyClassPassives()` 改为遍历表：

```cpp
for (RogueTalentDef const& t : RogueTalents)
    RefreshAura(t.auraId, (t.spec == _spec && level >= t.minLvl && (!t.maxLvl || level < t.maxLvl)) ? 1 : 0);
```

> 注意：存在少数**非专精限定**的通用天赋（如 `REMORSELESS_ATTACKS` ≥10、`VIGOR` ≥20、`DUAL_WIELD_SPECIALIZATION` ≥10、`MASTER_OF_DECEPTION` ≥10），这些条目的 `spec` 需要一个"任意专精"的哨兵值（例如 `0xFF`），否则遍历时会漏掉。实现时需一并处理。

同时把天赋触发概率（Seal Fate / Ruthlessness / Relentless Strikes）从 `SpellHitTarget` 中的魔法数字（`:1512` 的 `60`、`:1353` 的 `20 * comboPoints`）提取为具名常量（如 `RUTHLESSNESS_PROC_PCT = 60`、`RELENTLESS_RS_PCT_PER_CP = 20`），为后续按天赋 rank 计算预留空间。

> 跨 tick 安全性：天赋表为编译期常量，安全；`master` 取等级的用法沿用 bot 基类既有模式。

### 4.5 消除随机性（提升行为可复现性）

全文 20+ 处 `Rand()` / `urand()` 散落在决策层（`:383`、`:565`、`:623`、`:684` 等），其中一类是用随机数掩盖优先级不清。重构后：

- 只保留**真随机点**（暴击、proc，属引擎职责）；
- **掩盖决策缺失型**随机（Killing Spree / AR / CB / FoK 的施放门控）改为**确定性条件**；
- **刻意行为抖动型**随机（`:352` 择机进潜行、`:740` BreakCC 等）保留，但统一走单一入口 `RogueRoll(chancePct)`，便于集中调参。

> **目标定位**：本节的目标是"**行为可复现、可回归**"，而非"可做单元测试"。
> 按 AGENTS.md，BOT AI 以单文件组织（4.8），决策函数依赖 `me` / `mytar` / `GetAuraEffect` 等实例状态，难以脱离运行环境单测；
> 因此验证依赖 **DPS 对比 + 固定场景回归**（见第五节），而非 `src/test/` 单测。此处不宣称"可测试性"。

### 4.6 涂毒逻辑重构（`DoNonCombatActions`）

将 `:782-791` 的嵌套三元表达式改为描述表，并区分 PVE / PVP：

```cpp
// 涂毒方案表
struct RoguePoisonPlan
{
    uint8  spec;
    uint8  lvlMin;
    uint32 mh;
    uint32 oh;
    bool   pvp;
};
```

新增 `IsPvpContext()` 判断，决定是否使用致残 / 脑麻痹毒。

**判定口径需收紧**："目标为玩家"在**野外 PvP**也成立，但野外遭遇通常不需要换致残毒（会牺牲 PVE 输出），若照此实现会导致涂毒在野外反复切换。建议采用**更窄的判定**：

```cpp
// 仅战场 / 竞技场视为 PvP 涂毒场景；
// 野外 PvP 仍按 PVE 方案涂毒，避免毒药在野外来回切换
bool IsPvpContext() const { return me->GetMap()->IsBattlegroundOrArena(); }
```

> 项目已有 `GetBG()` 等 BG 判定辅助，实现时优先复用，避免重复造判定。
> 本条属 P2 级，若时间不足可先只做 PVE 涂毒方案表，PVP 分支留空。

### 4.7 缺失能力补全（按优先级）

| 优先级 | 补全项 | 理由 |
|---|---|---|
| P0 | 修复 `:692` 能量不足 `return` 阻断 | 多目标时整轮空转 |
| P0 | SnD / Rupture / HfB 剩余时长门控 | 直接 DPS 提升，消除 CP 浪费 |
| P0 | Envenom 需目标 Deadly 层数 ≥ 4 | 刺杀核心，当前判定无效 |
| P0 | 打断团队协同（`IsInterruptCovered` 双层检测 + 去随机门控） | 多 BOT 重复打断浪费 CD（见 7.1） |
| P0 | 紧急生存固定阈值化（去 `10 * attackers` 公式 + 去 Rand） | 生存可靠性（见 7.3） |
| P1 | 实现 Expose Armor（破甲缺失时补） | 团队增益 |
| P1 | 爆发技能去概率化 + 配对（AR+BF、CB+Envenom） | 爆发质量 |
| P1 | 斩杀阶段（< 35%）降低 Eviscerate / Envenom 阈值 | 常规优化 |
| P1 | 必须控制独立成段（PVE 单体不占用 Kidney Shot / Gouge） | 减少 DPS 损失（见 7.2） |
| P2 | 实现 Sap（潜行起手闷棍） | 潜行职业常规开场，现为 NYI |
| P2 | Shiv 维持毒药 | 刺杀补充 |
| P2 | Cut to the Chase 时长重算（`:1684` 硬编码） | 天赋 / 雕文变化时失真 |
| P3 | 多目标 5+ 只用 FoK + SnD | AOE 玩法对齐 |

### 4.8 改动边界（守住最小改动原则）

- **不新建文件**：`bot_rogue_ai.cpp` 是既定组织方式，重构保持单文件；
- **不改 `bot_ai` 基类**：`RogueSnapshot` 为本职业局部结构；7.1 的协同检测**复用基类已有的公开只读接口**（`HasQueuedAction` / `IsActionNext`），无需给基类加接口；
- **不动 `spell_rogue.cpp`**：那是玩家法术脚本，与 BOT AI 无关；
- **不引入 DB / SmartAI**：BOT 输出逻辑是代码路径，不适合数据驱动；
- **可新增 `rogue_botAI` 私有成员**：如 7.1 的 `_counterCheckTimer`，在 `:2050-2058` 的 private 区声明并初始化，属单文件内改动，不违反上述边界。

### 4.9 代码规范要求

- 4 空格缩进（禁 tab）、Allman 大括号、`if (x)` 空格规范；
- `auto const&`（非 `const auto&`）、`Type const*`（非 `const Type*`）；
- 注释用中文；
- 字符串格式化用 `Acore::StringFormat("{}", arg)`；
- 随机数用 `urand` / `irand` / `rand_chance`（`src/common/Utilities/Random.h`），禁 `std::rand`；
- 日志用 `LOG_INFO("category.sub", "msg {}", arg)`。

---

## 五、验证方式

1. **静态检查**：`python apps/codestyle/codestyle-cpp.py`（缩进、Allman、`auto const&` 等强制项）；
2. **随机点核查**：确认**掩盖决策缺失型** `Rand()` 已清除（Killing Spree / AR / CB / FoK），
   `grep Rand` 结果只应剩 proc 与**行为抖动型**（统一走 `RogueRoll`）；
3. **DPS 对比**：使用 `botdpstracker`（`botdpstracker.h`）对比重构前后 DPS，三系 × 单体 / 3 目标两场景各跑一轮；
4. **回归场景**：刺杀单体、战斗单体、敏锐单体、战斗 3 目标、5 目标 AOE、斩杀阶段、PVP 场景；
5. **协同专项验证（新增，对应 7.1）**：同一队伍放入 **2~3 个带打断的 BOT**（盗贼 + 战士/法师），
   观察同一读条目标是否仍被**重复打断**——理想结果是"同一读条只被一次 Kick 命中"；
   同时确认 Kick 冷却中改用 Gouge、且引导法术（channeled）也能被正确覆盖检测；
6. **生存归属验证（对应 4.3/7.3）**：确认 Vanish / Cloak 在单 tick 内**只被一处触发**，
   无"UpdateAI 层与 Attack 层各放一次"的重复施放。

---

## 六、结论

当前盗贼 BOT 的 3 系输出是「等级 + 专精分支 + 单函数线性 if 瀑布 + 随机概率」的组合。优点是能跑通、代码集中；缺陷是：

1. **优先级隐含在代码顺序中**，无法独立演进；
2. **随机数掩盖决策缺失**，行为不可复现；
3. **三系共用一条混合判定链**，条件互相污染（尤其 `:613` 的 Envenom / Eviscerate 混合体）；
4. **天赋模型是固定单 build**，不支持混搭，数值散落多处。

优化的核心不是"加更多技能"，而是**把隐含的优先级显式化**：

- 引入 `RogueSnapshot`（一次性上下文）+ 8 个分段函数（紧急生存 / 必须控制 / 常规生存 / 爆发 / 维持 / 多目标 / 清 CP / 攒 CP）；
- 把 `Attack()` 从 400 行 if 瀑布拆为职责单一的函数；
- 把天赋与涂毒从硬编码树改为描述表。

如此，三系的输出优先级、爆发配对、多目标分流才能各自独立演进，同时保持**单文件、零基类改动**的约束。

---

## 七、补充：打断 / 必须控制 / 紧急生存

前六节聚焦"输出循环"。本节补齐三个**抢占型**行为——它们优先级高于输出，但现有实现要么缺失、要么粗糙、要么与输出逻辑缠绕。

### 7.1 打断（Interrupt）—— 不能"硬打断"，必须团队协同

**现状**（`Counter()`，`bot_rogue_ai.cpp:258`）：

```cpp
void Counter(uint32 diff)
{
    if (Rand() > 50 || me->HasAuraType(SPELL_AURA_MOD_STEALTH))
        return;                                    // ← 一半 tick 直接放弃判断
    ...
    if (IsSpellReady(KICK_1, diff, false) && Rand() && !HasQueuedSpellAction(KICK_1))
        if (EnqueueCounterSpellAction(u->GetGUID(), KICK_1, true))
```

**核心问题：多个 BOT（或带 Kick 的战/骑/德、带 Counterspell 的法）会重复抢同一个读条目标。**

当前每个 bot 独立判断，`EnqueueCounterSpellAction` 也只管自己排队，没有任何"是否已被队友覆盖"的检查 → 结果是 N 个 bot 同时踢一个读条，浪费全部 CD。

**设计原则：不抢，只补。**

```
1) 先判断"是否已被队友覆盖"
   - 只读遍历队伍中的其他单位
   - 若已有队友正在对同一目标施放打断技 → 放弃本次打断
2) 未被覆盖时才执行
   - 优先 Kick（本职，CD 10s）
   - Kick 冷却中才用 Gouge（且目标可被 gouge、在正面）
   - Blind 仅在"多目标 / 无人能打断"时兜底，不进 PVE 单体循环
3) 保留"是否值得打断"的门控
   - 治疗读条 / 长读条（≥ 阈值）→ 打断
   - 随机小怪的短读条 → 不打（省 CD）
```

**实现要点（关键修正）**

只读查询，**不引入全局状态**（遵守项目 `ObjectGuid` 约定，不跨 tick 持有指针）。

⚠️ **不能只查 `GetCurrentSpell`**。打断在本项目里是**排队执行**的：

```cpp
// bot_ai.cpp:17601-17608
bool bot_ai::EnqueueCounterSpellAction(ObjectGuid target_guid, uint32 base_spell, bool interrupt_self_cast)
{
    // 自由 bot 有 150~900ms 延迟（botcommon.h:667 BOT_ACTION_COUNTERSPELL_DELAY_RANGE）
    const auto delay_roll = IAmFree() ? Milliseconds(urand(...)) : 0ms;
```

从"排队"到真正 `doCast` 之间有 **150~900ms 窗口**，期间 `GetCurrentSpell()` 返回 `nullptr`——
"已排队未施放"的打断**完全不可见**。只查 `GetCurrentSpell` 无法达成"多 BOT 去重"的核心目标。

**正确做法：双层检测（已排队 + 已在读条）**

`bot_ai` 已有公开的、**目标感知**的只读接口可直接复用：

```cpp
// bot_ai.h:916-917
bool HasQueuedAction(BotActionTypes action_type, ObjectGuid guid_param, uint32 uparam, Optional<bool> bparam = std::nullopt) const;
bool IsActionNext(BotActionTypes action_type, uint32 param1 = 0, ObjectGuid guidparam1 = ObjectGuid::Empty) const;
```

```cpp
// 判断目标是否已被队友的打断技覆盖（只读遍历，不持有指针）
bool IsInterruptCovered(Unit const* target) const
{
    if (!target)
        return false;

    // 遍历队伍中其他 bot（只读，不缓存指针）
    // 队伍成员通过 master->GetGroup() 取玩家，再经 player->GetBotMgr()->GetBotMap() 取该玩家的 bot；
    // 也可用 master->GetBotMgr()->GetBotMap() 直接枚举本体 bot
    for (/* 每个其他 bot */)
    {
        bot_ai const* other = otherBot->GetBotAI();
        if (!other)
            continue;

        // 第 1 层：已"排队"的打断（覆盖 150~900ms 延迟窗口，这是 GetCurrentSpell 看不到的）
        if (other->HasQueuedAction(BotActionTypes::BOT_ACTION_SPELLCAST, target->GetGUID(), KICK_1) ||
            other->HasQueuedAction(BotActionTypes::BOT_ACTION_SPELLCAST, target->GetGUID(), GOUGE_1))
            return true;

        // 第 2 层：已在读条/引导的打断（兜底，覆盖非 bot 队友或已进入施法阶段的情况）
        for (uint8 i = CURRENT_FIRST_NON_MELEE_SPELL; i != CURRENT_AUTOREPEAT_SPELL; ++i)
        {
            Spell const* spell = otherBot->GetCurrentSpell(CurrentSpellTypes(i));
            if (!spell || spell->m_targets.GetUnitTargetGUID() != target->GetGUID())
                continue;
            if (spell->GetSpellInfo()->HasEffect(SPELL_EFFECT_INTERRUPT_CAST) ||
                spell->GetSpellInfo()->HasAura(SPELL_AURA_MOD_SILENCE))
                return true;
        }
    }

    return false;
}
```

> **两点必须注意**：
> 1. **必须遍历 `CURRENT_FIRST_NON_MELEE_SPELL` 到 `CURRENT_AUTOREPEAT_SPELL`**，不能只查 `CURRENT_GENERIC_SPELL`——
>    Kick 同样能打断**引导法术**（`CURRENT_CHANNELED_SPELL`），只查 GENERIC 会漏。
>    可参考 `bot_GridNotifiers.h:697` 的 `CastingUnitCheck::CastInterruptionCheck`，它就是正确的遍历范式。
> 2. `HasQueuedAction` 是 `bot_ai` 的**公开 const 成员**，可直接在其他 bot 的 AI 实例上调用，
>    **无需给基类加接口**（符合 4.8"不改基类"的约束）。

> 补充：目标被成功打断后会获得短暂的法术免疫 / 沉默类状态，`IsNonMeleeSpellCast` 会返回 false，天然抑制重复。
> 但两个 bot 在**同一 tick 同时决策**时仍会同时排队，所以上面的覆盖检查不可省；
> 且**只有"已排队"检测才能覆盖这一竞态**，`GetCurrentSpell` 单独使用是无效的。

**决策频率修正**

去掉 `Rand() > 50` 门控，改为按 `diff` 定频检查（每 100~200ms 一次），在检查内做确定性判断：

```cpp
void Counter(uint32 diff)
{
    static constexpr uint32 COUNTER_CHECK_INTERVAL = 150;
    _counterCheckTimer += diff;
    if (_counterCheckTimer < COUNTER_CHECK_INTERVAL)
        return;
    _counterCheckTimer = 0;

    // 确定性判断：值得打断 && 未被队友覆盖 && 技能可用
    ...
}
```

> **`_counterCheckTimer` 需新增为 `rogue_botAI` 私有成员**（`bot_rogue_ai.cpp:2050-2058` 的 private 区），
> 类型 `uint32`，初值 0。这与 4.8 的"不新建文件 / 不改基类"约束不冲突。

**优先级门控表**

| 读条类型 | 是否打断 | 手段 |
|---|---|---|
| 治疗 / 大 AoE / 召唤（致命） | 必打断 | Kick |
| 队友被针对的读条 | 打断 | 切目标 Kick |
| 普通长读条 | 打断 | Kick（CD 中则 Gouge） |
| 随机小怪短读条 | 不打 | — |
| 已被队友覆盖 | 不打 | — |

### 7.2 必须控制（Must-CC）

**现状**：控制技分散在多处，且与输出循环抢占优先级。

| 控制技 | 位置 | 问题 |
|---|---|---|
| Cheap Shot | `:644` | 仅 PVP 或"多 bot"启用，PVE 团队默认不用 |
| Kidney Shot | `:602` | 排在 Finisher 段第一位，`Rand() < 80`，**用 CP 换控制** |
| Gouge | `:392`、`:268` | 同时出现在输出段与打断段，职责混乱 |
| Blind | `:400`、`:897`、`:272` | 出现在三个地方，各自条件不同 |
| Sap | `:64` | **标记 NYI，完全未实现** |
| Dismantle | `:427` | 缴械，属"必须控制"的物理手段 |

**核心缺失**

1. **没有"必须控制"的场景识别**——不区分"灭团技需要 CC"与"随便控一下"；
2. **Kidney Shot 在 PVE 单体里用 CP 换控制**（`:602` `comboPoints >= 4`）是纯 DPS 损失，只有特定场景才该用；
3. **Gouge 在输出段**（`:392`）会打断自己的输出循环，PVE 单人打怪是负收益；
4. **Sap 完全未实现**——潜行起手闷棍是常规开场，缺失明显。

**优化目标**

```
"必须控制"独立成段，在输出之前，但仅在满足条件时触发：
  1. 团队标记的 CC 目标（目标图标 / raid marker）→ 优先 CC
  2. 目标即将施放灭团技 / 读条需锁 → Kidney Shot / Gouge / Blind
  3. 目标低血逃跑 → 定身 / 眩晕
  4. PVE 单体常规：Kidney Shot 不参与，Gouge 不参与
  5. 实现 Sap（潜行起手，仅 PVE 单人 / 指定目标）
```

**"团队标记的 CC 目标"应复用现有机制，不要新造 gossip 接口**

项目**已有**基于**团队目标图标（raid marker）**的 CC 目标机制，不是 gossip：

```cpp
// bot_ai.cpp:1388-1391
bool bot_ai::IsPointedNoDPSTarget(Unit const* target) const
{
    return IsPointedTarget(target, BotCfg::GetNoDPSTargetIconFlags());
}
```

- 图标掩码由配置项决定（`botconfig.cpp:1083-1086 GetNoDPSTargetIconFlags()`）；
- 该判定**已被** `bot_GridNotifiers.h` 的 `CastingUnitCheck` 与 `NearbyHostileUnitCheck` 使用，
  用于让 bot"不要打被标记为 CC 的目标"（`bot_GridNotifiers.h:678`、`:855`）。

因此 `TryMustControl` 的 CC 目标识别**直接调用 `IsPointedNoDPSTarget(mytar)`** 即可，
既零成本、又与现有团队协作机制一致，无需新增 gossip 交互。文档此前"可由 gossip 设置"的表述有误，已修正。

### 7.3 紧急生存（Emergency Survival）—— 只用"血量 + 有无怪"，不做复杂计算

**现状**：生存技分散在 `Attack()` 中后段与独立 Check 函数里，触发条件参差不齐。

| 技能 | 位置 | 当前条件 | 评估 |
|---|---|---|---|
| Evasion | `:458` | `Rand() < 65`，血量 < `65 + 10*攻击者数` | 阈值用了动态公式，应固定 |
| Cloak of Shadows | `CheckCloakOfShadows` `:838` | debuff / 读条统计，`count > 1` | ✅ 简单计数，可保留 |
| Vanish | `CheckVanish` `:822`、`:510` | 血量 < 30%，或 PVP 逃命 | 条件合理 |
| Sprint | `:449`、`CheckSprint` `:981` | 逃生 / 追人 | 偏机动，非生存 |
| Thistle Tea | `:541` | 能量低 | 属续航 |
| 药水 | `:295` | 血量 < 50% | ✅ 简单直接 |
| BreakCC | `:738` | 被控时 Vanish / Sprint，`Rand() < 25` | 设计合理，随机门控可去 |

**设计约束（重要）**：**只用"血量 + 是否被怪作为目标"，不做复杂计算。**

**判定输入（仅两个）**

```
1. 自身血量百分比     me->GetHealthPct()
2. 是否被怪作为目标   !me->getAttackers().empty()
```

**固定分档（不搞公式）**

| 情况 | 判定 | 动作 |
|---|---|---|
| 危险（紧急） | 血量 < 35% **且** 有怪在打我 | Cloak → Vanish → Evasion |
| 危险（无怪） | 血量 < 35% 且无怪打我 | 只喝药，不放保命技 |
| 警戒 | 血量 < 55% **且** 有怪在打我 | Evasion |
| 安全 | 血量 ≥ 55% | 不处理 |

**必须去掉的复杂度**（当前代码里已存在）

```cpp
// :458 Evasion —— 阈值随攻击者数量线性增长，属于"复杂计算"，改为固定阈值
GetHealthPCT(me) < 65 + 10 * me->getAttackers().size();

// :459、:565 等多处同样用 getAttackers().size() 做动态阈值
```

改为：

```cpp
// 固定阈值，只判断"有无攻击者"
if (GetHealthPCT(me) < 55.f && !me->getAttackers().empty())
    doCast(me, GetSpell(EVASION_1));
```

**刻意不做的事**

- ❌ 掉血速率分级（需跨 tick 采样统计）
- ❌ 被集火人数用于动态阈值（只判断"有无"）
- ❌ 能量预留机制
- ❌ 基于治疗者存活的推断

**保留现有已写得好的部分**

- `CheckCloakOfShadows`（`:838`）的 debuff / 读条计数（`count > 1`），本身已是简单计数
  （逻辑并入 `TryEmergencySurvival` 时**原样保留该计数判定**，不要简化掉）；
- 喝药水（`:295` 血量 < 50%）；
- `BreakCC`（`:738`）被控脱身。

### 7.4 优先级总览（修正后）

抢占段优先级从高到低（**采用 4.3 的 A 方案：Vanish/Cloak 并入紧急/常规生存段**）：

```
0. UpdateAI 层：Counter() 打断（团队协同）    —— Attack() 之前独立执行
1. 紧急生存（血量 < 35% 且有怪）            —— Attack() 第 1 段（Cloak → Vanish → Evasion）
2. 必须控制（灭团技 / 团队 CC / 逃跑）        —— Attack() 第 2 段
3. 常规生存（血量 < 55% 且有怪）             —— Attack() 第 3 段（Evasion 等）
4. 爆发窗口（自带 CP / CD 门控）              —— Attack() 第 4 段
5. 输出循环（维持 / 多目标 / 清 CP / 攒 CP）   —— Attack() 第 5~8 段
```

> **两处与旧稿不同的关键修正**：
> 1. **打断 `Counter()` 仍在 `UpdateAI` 层、先于 `Attack()`**（它需要队列协同，不放进 `Attack()` 分段）；
> 2. **Vanish / Cloak 从 `UpdateAI` 层的 `CheckVanish` / `CheckCloakOfShadows` 并入 `Attack()` 的生存段**（A 方案），
>    消除双重触发；仅保留 `CheckVanish` 中"无目标也可逃"的兜底在 `UpdateAI` 层。

### 7.5 现有实现 vs 本方案

| 能力 | 现有代码 | 本方案 |
|---|---|---|
| 打断 | ✅ 有 `Counter()`，但双层 `Rand` 门控，**无团队覆盖检查** | 去掉随机门控改定频；新增 `IsInterruptCovered()` **双层检测**（`HasQueuedAction` 抓已排队 + `GetCurrentSpell` 抓已读条，覆盖 channeled）；保留"是否值得打断"门控 |
| 必须控制 | ⚠️ 有 Kidney Shot / Gouge / Blind，但混在输出段中间，Sap 未实现 | 独立成段；PVE 单体不占用 Kidney Shot / Gouge；实现 Sap |
| 紧急生存 | ⚠️ 有 Evasion / Cloak / Vanish，但靠血量 + 随机 + 动态公式触发 | 固定阈值；仅用"血量 + 有无怪"；去掉一切推导计算 |
