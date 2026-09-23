# NPCBot 战斗走位与技能处理机制全面分析及 BOSS 战优化方案

> 分析对象：AzerothCore WotLK（3.3.5a）NPCBots 模块
> 源码目录：`src/server/game/AI/NpcBots/`
> 分析方式：静态源码分析（未修改源码、未配置/执行构建）
> 版本基准：Trickerer NPCBot 系统 + 本项目自定义扩展（含 Hazards 危险区管理、BotPositionControl 集合/散开、载具策略等）

---

## 目录

1. [结论摘要](#1-结论摘要)
2. [分析范围与代码结构](#2-分析范围与代码结构)
3. [职责模型与走位全景](#3-职责模型与走位全景)
4. [战斗走位策略详解（按职责）](#4-战斗走位策略详解按职责)
5. [技能处理机制详解](#5-技能处理机制详解)
6. [BOSS 战适应性现状评估](#6-boss-战适应性现状评估)
7. [BOSS 战策略优化意见](#7-boss-战策略优化意见)
8. [新增智能功能设计](#8-新增智能功能设计)
9. [实施优先级与风险](#9-实施优先级与风险)
10. [附录：关键常量速查](#10-附录关键常量速查)

---

## 1. 结论摘要

当前 NPCBot 的 BOSS 战能力**不是一个统一的"Boss 战术引擎"**，而由以下层次叠加构成：

1. **通用角色 AI**：坦克 / 治疗 / 近战 DPS / 远程 DPS 各自执行职业技能优先级链。
2. **通用移动与危险区系统**：队形跟随、追击、绕背、躲避正面 AOE、坦克调整朝向、AOE 安全点搜索、绕行点规划、集合/散开控制。
3. **团队管理逻辑**：进场延迟、主坦/副坦、团队图标选怪、仇恨门槛、换担（taunt swap）。
4. **公共战术能力**：打断、反制、驱散、净化、控制、治疗、减伤、复活。
5. **少量 BOSS 硬编码**：骨刺、冰墓、瓦格里、污染之核、普特雷塞德软泥坑、腐面绿水等机制的目标切换或物件交互。
6. **玩家手动控制**：`sendto` / `stay` 等命令兜底处理 AI 无法稳定完成的定点站位。

**结论**：系统能较好处理「坦克拉住、近战绕背、远程分布、躲部分地面技能、优先打断」这类战斗；但对需要**严格阶段状态机、同步集合/分散、路径规划、打断轮次、换担层数、精确站位槽**的 BOSS，自动化程度仍不足。

**核心短板（详见第 6、7 章）**：
- 坦克走位只有「把攻击者聚到正面」，**没有「背对人群 / 靠墙 / 定点拉」的 BOSS 级站位意识**；
- 技能循环是**线性 if-else 优先级链**，缺少优先级队列与"本场战斗状态"记忆，阶段切换依赖硬编码；
- 打断/嘲讽等是**各自为战**，没有团队级轮换协调（打断轮次、减伤链、换担队列）；
- 危险区识别高度依赖**动态物体检测 + 地图硬编码 + DB 规则**，对新 BOSS 地面技能的覆盖是打补丁式，缺乏通用性。

---

## 2. 分析范围与代码结构

### 2.1 核心文件清单

| 文件 | 职责 | 规模 |
|------|------|------|
| `bot_ai.cpp` / `bot_ai.h` | 基类：调度基础设施（GCD/冷却/动作队列/走位/搜索/治疗/嘲讽/驱散） | 约 95 万字节 |
| `botcommon.h` | 枚举与常量（角色、移动、动作、AIMisc） | — |
| `bot_GridNotifiers.h` | Grid 搜索谓词（读条检测、驱散目标、远距嘲讽等） | 约 5.4 万字节 |
| `botpositioncontrol.cpp` / `.h` | 集合（Mass）/散开（Spread）模式 | 439 行 |
| `Hazards/NPCBotHazardMgr.cpp` / `.h` | DB 配置的生物危险区规则管理 | 269 行 |
| `botspell.cpp` / `.h` | 技能相关工具 | — |
| `bpet_ai.cpp` / `bpet_*.cpp` | 宠物 AI | — |
| `bot_<职业>_ai.cpp`（20 个） | 各职业技能优先级循环与专属战术 | 数千行/职业 |

### 2.2 架构分层

```
BotMgr::Update（每帧）
  └─ bot_ai::GlobalUpdate(diff)
       ├─ ReduceCD(diff)                         // 冷却递减（各职业可 override）
       ├─ _processQueuedActions()                // 动作队列（打断/开怪）
       ├─ 无效施法主动打断                        // 目标死亡/免疫/满血等
       ├─ GetInPosition() / 走位决策
       └─ CommonTimers(diff)                     // SpellTimers/GCD/各计时器
  └─ 职业 UpdateAI(diff)                          // 各 bot_*_ai.cpp
       ├─ CheckRacials / BuffAndHealGroup / CureGroup
       ├─ CheckAttackTarget()                    // 目标选择（基类）
       └─ Attack() / DoNormalAttack()            // 技能 if-else 优先级链
            └─ doCast() → CheckBotCast() / IsSpellReady()
```

**核心设计思想**：技能循环是**「按优先级降序排列的线性 if-else 块 + 命中即 doCast 且 return」**，不存在显式优先级队列。GCD 用 `GC_Timer`，冷却用 `BotSpell.cooldown`（随 `SpellTimers` 每 tick 递减）。

---

## 3. 职责模型与走位全景

### 3.1 角色（Role）枚举

`botcommon.h` 中 `BotRoles`（uint32 位掩码）：

| 角色 | 常量 | 说明 |
|------|------|------|
| 主坦 | `BOT_ROLE_TANK` | 主坦克 |
| 副坦 | `BOT_ROLE_TANK_OFF` | 副坦克（换担/接小怪） |
| 输出 | `BOT_ROLE_DPS` | DPS |
| 治疗 | `BOT_ROLE_HEAL` | 治疗 |
| 远程 | `BOT_ROLE_RANGED` | 远程定位标记 |

`BOT_ROLE_MASK_MAIN = TANK | TANK_OFF | DPS | HEAL | RANGED`。

关键判定函数：
- `IsTank()`：对 bot 看 `HasRole(BOT_ROLE_TANK)`；对玩家看 LFG 标记 / 组队 MAINTANK / 职业天赋（带盾防战、防骑、熊德、带威胁光环的血 DK）。
- `IsOffTank()`：判定 `BOT_ROLE_TANK_OFF`。
- `IsMelee()` = 非远程 + 有 DPS/TANK 角色。
- `IsRanged()` = 有 `BOT_ROLE_RANGED` 或载具远程覆写。

### 3.2 移动类型枚举

`BotMovementType`（botcommon.h）：

| 常量 | 含义 |
|------|------|
| `BOT_MOVE_POINT` | `MovePoint` 精确移动到坐标（规避网格边界"鬼墙"阻挡寻路） |
| `BOT_MOVE_CHASE` | `MoveChase` 追击目标 |
| `BOT_MOVE_JUMP` | `MoveJump` 跳跃（22 码/s 初速） |

攻击距离/角度模式（`BotAttackRange` / `BotAttackAngle`）：
- `BOT_ATTACK_RANGE_SHORT` / `LONG` / `EXACT`（主人指定精确码数）。
- `BOT_ATTACK_ANGLE_NORMAL` / `AVOID_FRONTAL_AOE`（规避正面 AOE 站位）。

### 3.3 职责走位全景表

| 职责 | 走位行为 | 关键函数 |
|------|---------|---------|
| **主坦 TANK** | 正面迎敌，后退平移把多个近战攻击者聚到正面（利用正面招架/格挡）；无"拉背对人群/墙角"逻辑 | `AdjustTankingPosition()` |
| **副坦 OFF-TANK** | 无独立走位分支，走 DPS 分支；换担靠嘲讽逻辑 | `CanTauntDistantTarget()` |
| **近战 DPS** | `MoveChase` 追击；主坦在场时以 0.40π~0.60π 大角散开；盗贼/猫德额外绕后 | `GetInPosition`、`CalculateAttackPos`、`MoveBehind` |
| **远程 DPS** | 保持 `GetSpellAttackRange()-5` 码；规避正面 AOE 侧后站位（±0.62π）；被点名跑向坦克；无 AOE 才移动 | `CalculateAttackPos` |
| **治疗 HEAL** | 非战斗贴主人 0.5 码；集合模式下受集合半径约束 | `_calculatePos` |
| **通用 AoE 规避** | 动态物体 + HazardMgr + 地图硬编码三层检测；安全点 + 绕行点 | `CalculateAoeSpots`、`TryGetAoeDetourPoint` |

---

## 4. 战斗走位策略详解（按职责）

### 4.1 攻击位置计算 `CalculateAttackPos()`

这是「不同职责 bot 在 BOSS 战如何站位」的核心函数，逻辑流程：

**A. 基础参数**：`dist = (EXACT ? exactRange : GetSpellAttackRange(LONG) - 5)`（远程默认留 5 码余量）。

**B. BOSS 冲锋修正**：目标带 `MOVEMENTFLAG_FORWARD` 且 bot 在其 2/3π 弧度内 → `dist = min(dist+4, 30)`，拉远防冲锋。

**C. 规避正面 AOE**：`angleMode == AVOID_FRONTAL_AOE` 且远程且目标非玩家控制且（主人是坦克或主人离目标 <2.5 码）→ 角度偏移 ±0.62π（约 112°），远程站到 BOSS 侧后方。

**D. 随机抖动**：
- `clockwise` 由 entry 奇偶决定；
- `angleDelta1` = 主人是坦克而我不是时 `rand(0.40π, 0.60π)`（**主坦在场时其他近战大幅散开**），否则 `rand(0, 0.15π)`；
- `angleDelta2` = `rand(0, 0.08π)`。

**E. 载具分支**：按座位分配角度，飞行目标用 `GetNearPoint`，检测 AoE/过远调整 Z ±8 码或角度。

**F. 原地安全检查**：当前位满足「不过远 + 有 LOS + 不在 AoE + 可攻击」→ 直接返回，避免无谓移动。

**G. 安全点集生成**：`CalculateAoeSafeSpots(target, followdist)` + 碰撞探测循环（最多 5 次）。

**H. 挑选最优安全点**：遍历安全点，用 `GetSpreadPenalty()` 加"散开惩罚"；优先 `closestAttackPos`（最近且能攻击），否则 `closestPos`，均无则原地 `force=true`。

**I. 无安全点但被 AoE 威胁**：移动到主人身边。

**J. 被仇恨远程保护**：**远程 bot 被 BOSS 点名（`target->GetVictim()==me`）时跑向坦克/主人身边**，贴近距离 1.5 码（坦克在目标正面时 0.5 码修正，否则 -1.5）。

**K. 兜底 + 散开优化**：`TryImproveSpreadPosition()` 微调，无 LOS 则 `force=true`。

### 4.2 到位主控 `GetInPosition()`

决策顺序：
1. **早期退出**：`STAY` 指令 / 非自由且禁止战斗站位 / 被控制 / 跳跃下落 / 目标不在战 / 正在移动 / 施法中 / 魔杖射击。
2. **集合走位**：`ShouldHoldMassPosition()` 为真则原地只攻击。
3. **坦克优先**：`AdjustTankingPosition()` 成功则 return。
4. **EXACT 距离 ==0**：目标身旁随机 0.5~1.5 码停留。
5. **远程/有 AoE 威胁**：`CalculateAttackPos()` + `BOT_MOVE_POINT`；玩家目标防逃保护（距玩家 < 6+随机 时不再移）。
6. **纯近战**：`BOT_MOVE_CHASE`。

### 4.3 坦克站位 `AdjustTankingPosition()`

**目的（源码注释）**："Bots 无法从背后招架/躲闪，所以尽量把敌人聚拢到正面"。

算法要点：
1. 触发：战斗中、非施法、非载具、非跳跃/控制、`Rand() < 10 + 20*IsDungeon()`（副本更高）、无 UNMOVING。
2. **必须同时被 ≥2 个近战攻击者攻击**才调整。
3. 统计"在近战范围且不在自己正面 π 弧度内"的攻击者数，为 0 则不动。
4. 计算新位置：以自身朝向为基准，`moveDist = -max(CombatReach, 3.0)` 码向后平移；循环 6 次探测，角度偏移 `(i+1)*0.31π`，选第一个有 LOS 且不在 AoE 的点。

**关键结论**：此函数做的是**「背对后方攻击者、把敌人聚到正面」**，**没有「把 BOSS 拉背对冲人群 / 拉墙角」的显式逻辑**——那是玩家坦克基于房间几何的战术，当前 bot 不具备。

### 4.4 近战绕后 `MoveBehind()`

目的：绕到目标背后（避免顺劈/躲闪/招架，用于盗贼/野德）。

- 冷却 `_moveBehindTimer`（urand 1000~4000ms）降低抖动。
- 盗贼/猫德：目标打我/被控制/玩家目标时绕后；其他近战：目标不打我时才绕后。
- 几何：`myangle = 目标朝向 + π`（正后方），`mydist = GetCombatReach()`；不在 AoE 则移动。

### 4.5 远程施法距离 `GetSpellAttackRange()`

基类：短距离 15 码、长距离 23 码，部分职业 override。集合模式取 `GetMassAttackRange()` 精确距离。

### 4.6 AoE 规避三层机制

**第一层：动态物体检测**（`CalculateAoeSpots`）
- `NearbyHostileAoEDynobjectCheck`（60 码）收集敌对动态物体，筛选 `IsPeriodicDynObjAOEDamage()`，radius = 物体半径 + 尺寸 + `CombatReach*1.2`。

**第二层：DB 危险规则**（`NPCBotHazardMgr`）
- 从 `npcbot_creature_hazard` 表加载规则（MapId / CreatureEntry / DamageSpellId / Radius / SafetyDistance / DeactivationDelayMs / RequiredAuraSpellId）。
- 扫描 60 码内匹配规则的活体生物，`finalRadius = Radius + SafetyDistance + CombatReach*1.2`。
- **已消失生物的危险区在 DeactivationDelayMs 内仍保留**——用于规避"地面残留"。

**第三层：地图硬编码**（`CalculateAoeSpots` 内大量特例）
- 熔火之心 Hot Coal、AQ 沙坑/变异虫爆炸、鲜血熔炉炸弹、永恒之眼 Static Field、祖阿曼火弹、ICC 软泥坑 + 腐面绿水、十字军试炼酸喉毒池 + 戈莫克火弹等。

**安全点与绕行**：
- `CalculateAoeSafeSpots`：目标为中心，径向 8 环 × 25 角度 = 200 候选点，排除过远/在 AoE 内的点。
- `TryGetAoeDetourPoint`：直线穿越危险圆时，在圆边界采样 16 个绕行点，选"起点→候选→目标"总长最短者，避免"直穿-躲避-再直穿"死循环。

### 4.7 集合 / 散开模式（BotPositionControl）

| 模式 | 常量 | 行为 |
|------|------|------|
| 集合 | `MIN/MAX_MASS_RADIUS = 1.0~4.0` 码 | 非坦 bot 聚合到主人身边（分担/规避机制），黄金角分配槽位 |
| 散开 | `MIN/MAX_SPREAD_DISTANCE = 2.0~20.0` 码 | 保持间距避免重叠吃 AoE，`deficit²` 惩罚 + 移动距离 ×0.05 |

`IsMassEligible`：排除自由/游荡/临时 bot、STAY/FULLSTOP/INACTION、坦克；`RangedAndHeal` 模式只聚远程+治疗。

---

## 5. 技能处理机制详解

### 5.1 技能调度三要素

| 要素 | 实现 |
|------|------|
| **GCD** | `GC_Timer`，`doCast` 中按施法时间与急速计算，`clamp(1000ms, 1500ms)` |
| **冷却** | `BotSpell { spellId, cooldown, enabled }`，`SpellTimers` 每 tick 递减；`ReduceCD()` 供职业动态修正 |
| **就绪判定** | `IsSpellReady(baseSpell, diff, checkGCD)`：GCD + enabled + cooldown 三重判定；未注册技能视为就绪 |

### 5.2 施法入口 `doCast()` 前置检查

- 目标无效/跨地图/`IsCasting()` → 放弃；
- 取 aura rank（按目标等级选最高可用等级）；
- 目标已有更强同类 aura 则不覆盖；
- 非触发 + 无 LOS 豁免 + 敌对 → 无视线放弃；
- `CastInterruptionCheck` 校验"打断/沉默是否真能打断目标读条"；
- **移动打断判定**：有读条 + 移动 + 非触发 → 停下施法或放弃；
- 形态冲突则移除形态；
- GCD 计算。

`CheckBotCast()` 是更严格的"资格验证"（`NO_CAST` / `NO_CAST_LONG` / 法力不足 / 冷却未好 / 离主人过远 / 缴械 / 免疫 / aurastate / 职业特殊状态）。

### 5.3 动作队列机制（打断/开怪的即时响应）

`BotActionTypes` 仅两种：

| 动作 | 用途 |
|------|------|
| `BOT_ACTION_SPELLCAST` | 排队的施法（常为打断/反制） |
| `BOT_ACTION_PULL` | 开怪动作 |

- 用 `std::set<BotAction>`（按 `_exec_point` 有序），`GetFirstActionInQueue()` 返回最早应执行的动作。
- 队列容量：玩家指令 3、内部动作 5。
- `_processQueuedActions` 对打断类做精细时间窗口：目标读条剩余 > `cast_time + 800ms` 则等待（过早打断浪费）；目标已停读条则取消（自由 bot 有 50% 概率留到下 tick 防 juke）。

**结论**：动作队列主要用于**打断/反制**和**开怪**两类"即时响应"，是排队的；普通战斗技能是即时的 if-else 链。

### 5.4 目标选择 `CheckAttackTarget()` / `_getTargets()`

目标优先级（`_getTargets`）：
1. 当前 victim 有嘲讽光环 → 强制锁死；
2. 强制攻击目标 `_forcedAttackTargetGuid`（玩家标记）；
3. 动作队列 PULL 目标（DPS 被要求开怪）；
4. 主人被魅惑 → 转打主人；
5. 副本专属机制目标（骨刺、冰墓、暗影陷阱等硬编码）。

非 T + 团本世界 BOSS + 目标已有人拉 + 仇恨 < `min(50000, victim 血量/2)` → 不打（防抢怪）。

### 5.5 Team 图标战术（Pointed Target）

玩家通过团队目标图标（骷髅/叉/方块等）给 bot 下达战术指令，`IsPointedTarget()` 检查目标 GUID 是否命中团队框架图标。
- `IsPointedTankingTarget` / `IsPointedOffTankingTarget` / `IsPointedDPSTarget` / `IsPointedRangedDPSTarget` / `IsPointedNoDPSTarget` / `IsPointedHealTarget`。
- 通过配置 `NpcBot.HealTargetIconMask` 等加载，**默认均 0（未配置则图标战术不生效）**。

### 5.6 治疗决策 `BuffAndHealGroup()` / `HealTarget()`

优先级注释：`1) 治疗玩家 2) buff 玩家 3) 治疗 bot 4) buff bot`。

核心阈值：
- 治疗选人距离 40 码、buff 距离 30 码；
- 治疗血量阈值 `GetHealHpPctThreshold()`（默认 **95**，可调）；
- 治疗候选**随机选一个**（非精确最低血量优先）。

**前瞻性治疗（关键亮点）**：各治疗职业用 `_heals[]`（动态计算实际治疗量）+ `hppctps`（每秒血量变化率）+ `xppct`（2.0~2.5 秒后预判血量百分比）做**过量治疗抑制**——`if (xppct >= 95 && hp >= 25 && !pointed) return false`，避免奶满溢出。

各治疗职业差异化：
- **牧师**：Guardian Spirit（濒死）、Pain Suppression（hp 25~55 且骤降）、Penance（hp≤80）、Greater Heal / Flash Heal（`xphploss > _heals[]` 才用）。
- **圣骑士**：Divine Plea 期间保护治疗、Lay on Hands（hp≤20 濒死）、Holy Shock（无 GCD）、Holy Light + Aura Mastery。
- **德鲁伊**：形态管理 + HoT（回春/愈合/迅捷治愈/百花）。

### 5.7 打断机制

两段式：
1. **`FindCastingTarget` + `EnqueueCounterSpellAction`**（队列反制）：打断/沉默类（束缚亡灵、制裁、变羊、恐惧）。
2. **即时打断**（非队列）：DK/战士/盗贼脚踢类，条件 `mytar->IsNonMeleeSpellCast(false,false,true)`，如战士 Shield Bash（需 CanBlock + 防姿）/ Pummel（狂暴姿）。

`CastingUnitCheck::CastInterruptionCheck` 用 `PreventionType` / `SPELL_INTERRUPT_FLAG_INTERRUPT` 校验"该读条确实可被此类打断"。

**主动打断自身无意义施法**：每 tick 检查当前读条，目标死亡/反射/免疫/无 LOS/控制无效/治疗目标已满血 → `InterruptSpell`。

### 5.8 嘲讽（Taunt）/ 换担机制

`CanTauntTarget` 换担阈值：
- 受害者不是 T → 直接抢；
- 我是 T 且血量 >67% 且（受害者 T 濒死 <30% / 副 T 换副 T / 主 T 接回）→ 嘲讽。

`CanTauntDistantTarget`：自己被盯着打时，副 T（或队里没副 T 的主 T）主动远距嘲讽接怪；Lv40+ boss 不主动换。

`FindDistantTauntTarget`：搜索"队友被攻击需 T 接走的怪"，`FarTauntUnitCheck` 做 T 换 T 校验 + 按职业豁免免疫（战士 355 / 骑士 62124 / 德鲁伊 6795）。

---

## 6. BOSS 战适应性现状评估

按真实魔兽世界团本 BOSS 战策略维度评估当前能力：

| 机制类别 | 当前支持度 | 说明 |
|---------|-----------|------|
| 换担（嘲讽轮换） | 中 | 有换担阈值，但无"层数累积"机制，靠血量阈值粗略判断 |
| 打断轮次 | 低 | 各自为战，无轮次协调，可能多个 bot 同时打断浪费 |
| 集合 / 分散 | 中 | 有 Mass/Spread 模式，但需玩家手动触发，无机制感知自动切换 |
| 减伤链（盾墙/痛苦压制等） | 低 | 单体自救，无团队减伤轮次编排 |
| 地面技能规避 | 中 | 三层检测，但对未知/新技能靠打补丁，通用性弱 |
| 正面 AOE / 顺劈规避 | 中 | 近战有绕后，远程有侧后站位，但无"扇形精确"判定 |
| 点名机制（跑脱/集火） | 低 | 有被点名跑向坦克，但无"Debuff 点名集火/分散"通用逻辑 |
| 阶段状态机 | 低 | 依赖硬编码，无通用阶段记忆 |
| 精确站位槽 | 无 | 无固定 slot 站位，仅黄金角近似 |
| 载具战 | 高 | 有完整 DoVehicleStrats（奥杜尔/十字军载具） |

---

## 7. BOSS 战策略优化意见

> 以下优化均遵循本项目「低侵入、可维护、兼容现有行为」的约定。按改动成本从低到高排列。

### 优化 1：坦克「背对人群 / 靠墙」站位意识（高价值，中成本）

**问题**：`AdjustTankingPosition()` 只"把敌人聚到正面"，不感知房间几何。

**方案**：在 `AdjustTankingPosition()` 中引入"人群方向感知"——
- 计算团队重心位置（非坦克成员的平均坐标），判定 BOSS 当前朝向与"人群方向"的夹角；
- 若 BOSS 正面扇区（如 ±45°）覆盖了人群，则优先选择"背对人群、面向 BOSS"的移动目标点；
- 靠墙加分：`GetFirstCollisionPosition` 检测后方墙体，将坦克引导到"背靠墙"位置，减少 BOSS 被击退/位移导致的乱向；
- **保持低侵入**：仅在 `IsInHeroicOrRaid()` 且目标为 Boss（`IsWorldBoss` / 副本首领）时启用，普通小怪仍用现有逻辑。

### 优化 2：打断轮次协调（高价值，中成本）

**问题**：多个 bot 有打断技能时会同时打断，浪费 GCD 且导致后续读条无人打断。

**方案**：引入团队级"打断令牌"——
- 在 BotMgr 中维护一个"下一个打断者"标记，`EnqueueCounterSpellAction` 前先查询是否已有其他 bot 在同一目标上排队了打断；
- 已有人排队则跳过，仅当目标读条进入"高危窗口"（如剩余时间 < 400ms 且无人打断）时才允许本 bot 补断；
- 打断优先级：近战短 CD 打断 > 长 CD 反制 > 控制类。

### 优化 3：减伤链编排（高价值，中成本）

**问题**：减伤技能（盾墙/冰封之韧/痛苦压制/Guardian Spirit）各自触发，不协作，可能重叠浪费。

**方案**：为坦克建立"减伤需求信号"（高额伤害预警 / 换担瞬间 / 仇恨剧变），减伤类技能释放前检查团队里是否已有减伤生效，避免叠加；对治疗的保护技能（痛苦压制、Guardian Spirit）建立"目标濒死 + 无他人保护"才释放的互斥。

### 优化 4：换担「按层数 / 按 Debuff」而非纯血量（高价值，低侵入）

**问题**：`CanTauntTarget` 靠血量阈值判断换担，对"叠 N 层 debuff 必须换担"的 BOSS 不准。

**方案**：`CanTauntTarget` / `CanTauntDistantTarget` 增加一个**可配置的"换担触发器"**——检测当前 T 身上是否存在配置中指定的可叠加 debuff（如 `npcbot_taunt_swap_debuff` 表指定 SpellId + 层数阈值），存在且达阈值则触发换担。无配置时回退到现有血量逻辑，完全向后兼容。

### 优化 5：危险区识别通用化（中价值，中成本）

**问题**：危险区靠"动态物体 + 地图硬编码 + DB 规则"三层，新 BOSS 地面技能要打补丁。

**方案**：扩展 `npcbot_creature_hazard` 的规则能力——
- 支持"来源是周期性地面技能（`SPELL_EFFECT_PERSISTENT_AREA_AURA` + `PERIODIC_DAMAGE`）"的**自动识别**，不必为每个 BOSS 手写；
- 支持"扇形正面危险区"规则（目前只有圆形），配合优化 7 的扇形判定；
- 支持"点选目标后延迟出现的地面圈"（如地面红圈预警）的提示型规避。

### 优化 6：阶段状态机的轻量落地（高价值，高成本，分期）

**问题**：BOSS 阶段切换靠硬编码，无通用状态记忆。

**方案（分期）**：
- **一期（低侵入）**：为 bot_ai 增加一个 `boss_phase` 轻量值，由「BOSS 血量百分比 + 配置的阶段阈值」驱动，仅用于**可选的行为切换钩子**（如阶段 2 强制远程靠拢、阶段 3 禁止近战）。不影响现有逻辑。
- **二期**：抽象出 `BotEncounterScript` 接口（类似 InstanceScript 但面向 bot），允许为特定 BOSS 注册战术脚本，覆盖默认走位/技能。

### 优化 7：扇形 AOE 精确判定（中价值，中成本）

**问题**：现有一律圆形规避，正面锥形/扇形技能（龙息、顺劈）只能靠"绕后"粗处理。

**方案**：危险区模型增加"扇形"类型（角度 + 半径 + 朝向），`IsWithinAoERadius` 支持扇形包含判定；远程"规避正面 AOE"从固定 ±0.62π 改为按扇形真实角度动态偏移。

---

## 8. 新增智能功能设计

> 以下为**可落地的增强功能**，按价值/可行性排序，均标注改动文件与兼容性。

### 功能 A：可配置换担规则表（推荐优先级最高）

- **新增表**：`npcbot_taunt_swap_rule`（MapId / BossEntry / TauntDebuffSpellId / SwapStackThreshold / SwapHpThreshold）。
- **改动**：`CanTauntTarget()` / `CanTauntDistantTarget()` 优先读取规则；无规则回退血量逻辑。
- **价值**：解决"叠层换担"类 BOSS（如纳克萨玛斯帕奇维克仇恨打击、奥杜尔钢铁议会等）的自动换担，且不破坏现有行为。

### 功能 B：团队打断轮次管理器

- **改动**：`bot_mgr_service` 增加打断令牌状态；`EnqueueCounterSpellAction` 增查重。
- **价值**：避免多 bot 同时打断的空转，提升关键读条（如治疗/灭团技能）的打断成功率。

### 功能 C：治疗濒死保护互斥

- **改动**：`BuffAndHealGroup()` 中保护技能（Guardian Spirit / Pain Suppression / Lay on Hands）加"目标已有人保护则跳过"检查。
- **价值**：避免减伤/保护技能重叠浪费，显著提升连招高压期的存活率。

### 功能 D：扇形/预警型危险区模型

- **改动**：`NPCBotHazardMgr` 的 `BotCreatureHazardRule` 增加 `HazardShape`（圆/扇形）+ `Angle` + `PreviewDelayMs`；`IsWithinAoERadius` 支持扇形包含。
- **价值**：精确规避正面锥形技能，支持地面红圈预警型技能的提前规避。

### 功能 E：阶段行为钩子（轻量状态机）

- **改动**：bot_ai 增加 `_bossPhase` 值 + `OnBossPhaseChange(uint8)` 虚函数；由血量阈值驱动。
- **价值**：为后续 BOSS 战术脚本提供挂载点，一期仅做可选行为切换，零破坏。

### 功能 F：固定站位槽（Slot-based 站位）

- **改动**：`BotPositionControl` 增加"战术槽位"概念，为特定 BOSS 预定义站位槽（如"近战分散点位""远程扇形点位""坦克定点"），bot 按角色分配到槽。
- **价值**：覆盖"精确站位"类 BOSS（如冰冠堡垒的梦魇之龙、奥杜尔零灯），弥补黄金角近似的不足。

### 功能 G：载具战策略扩展（已有基础，补充完善）

- **现状**：`DoVehicleStrats` / `DoSkytalonVehicleStrats` 等已支持多类载具。
- **补充**：为自定义副本新增载具时，复用 `ChooseVehicleForEncounter` + `HasVehicleRoleOverride`，避免重复实现。

---

## 9. 实施优先级与风险

| 优先级 | 优化/功能 | 改动量 | 兼容性风险 | 建议 |
|-------|----------|-------|-----------|------|
| P0（立即） | 功能 C：治疗保护互斥 | 小 | 极低 | 收益立竿见影 |
| P0 | 优化 4 + 功能 A：换担规则表 | 中 | 低（回退兼容） | 直击换担短板 |
| P1 | 功能 B：打断轮次 | 中 | 低 | 提升关键打断 |
| P1 | 优化 1：坦克背人群/靠墙 | 中 | 中（需 BOSS 判定） | 团本体验提升 |
| P1 | 功能 D：扇形危险区 | 中 | 中 | 正面技能规避 |
| P2 | 优化 5：危险区通用化 | 中 | 中 | 减少打补丁 |
| P2 | 功能 E：阶段行为钩子 | 中 | 低（纯增量） | 为战术脚本铺路 |
| P3 | 优化 2/6/7 完整落地 | 大 | 高 | 建议分期、专项验证 |
| P3 | 功能 F：固定站位槽 | 大 | 中 | 仅高端团本需要 |

> **通用风险提示**：
> - 走位/站位类改动会影响 **BotPositionControl 的集合/散开状态机**，需回归测试集合模式与 `sendto`/`stay` 指令；
> - 换担/打断/减伤类改动涉及仇恨与 GCD 时序，需在团本实测验证不会导致"抢仇恨"或"打断空转"；
> - 所有新增 DB 表/字段应放入 `data/sql/updates/pending_db_world/`，遵循"DELETE + INSERT 幂等"约定；
> - 坦克背人群/靠墙需严格限定在副本 Boss 场景，避免影响野外/小怪拉怪体验。

---

## 10. 附录：关键常量速查

### 10.1 走位相关

| 常量/参数 | 值 | 位置 |
|-----------|-----|------|
| 远程默认攻击距离（短/长） | 15 / 23 码 | `GetSpellAttackRange` |
| 远程攻击距离余量 | -5 码 | `CalculateAttackPos` |
| 主坦在场近战散开角 | 0.40π ~ 0.60π | `CalculateAttackPos` |
| 规避正面 AOE 偏移角 | ±0.62π（约 112°） | `CalculateAttackPos` |
| BOSS 冲锋拉远距离 | min(dist+4, 30) 码 | `CalculateAttackPos` |
| 坦克聚怪后退距离 | -max(CombatReach, 3.0) 码 | `AdjustTankingPosition` |
| 坦克聚怪探测次数/步进 | 6 次 × 0.31π | `AdjustTankingPosition` |
| 近战绕后冷却 | urand(1000, 4000)ms | `MoveBehind` |
| 集合模式半径 | 1.0 ~ 4.0 码 | `BotPositionControl` |
| 散开模式距离 | 2.0 ~ 20.0 码 | `BotPositionControl` |
| AoE 安全点候选数 | 200（8 环 × 25 角） | `CalculateAoeSafeSpots` |
| AoE 绕行点采样数 | 16 个 | `TryGetAoeDetourPoint` |

### 10.2 技能与治疗相关

| 常量/参数 | 值 | 位置 |
|-----------|-----|------|
| GCD 范围 | 1000 ~ 1500ms | `doCast` |
| 治疗选人距离 | 40 码 | `BuffAndHealGroup` |
| Buff 距离 | 30 码 | `BuffAndHealGroup` |
| 治疗血量阈值（默认） | 95% | `_healHpPctThreshold` |
| 反制延迟（自由 bot） | urand(150, 900)ms | `EnqueueCounterSpellAction` |
| 反制时间窗口扩展 | 800ms | `BOT_ACTION_COUNTERCAST_TIME_WINDOW_EXTENSION_MS` |
| 动作队列容量 | 指令 3 / 自动 5 | `botcommon.h` |
| 换担血量阈值 | 主 T >67% 且副 T <30% | `CanTauntTarget` |

### 10.3 危险区规则字段（`npcbot_creature_hazard`）

`MapId` / `CreatureEntry` / `DamageSpellId` / `Radius` / `SafetyDistance` / `DeactivationDelayMs` / `RequiredAuraSpellId`。

危险区最终半径 = `Radius + SafetyDistance + CombatReach * 1.2`。

---

## 附：与既有文档的关系

本模块 `outputs/NPCBot/` 下已有《NPCBot首领战走位与战术逻辑分析.md》（走位专项）、《NPCBot走位控制功能设计方案.md》（集合/散开控制）。本文在其基础上：
- **补齐了技能处理机制维度**（GCD/冷却/优先级链/动作队列/治疗/打断/嘲讽/驱散），这是既有文档未系统覆盖的部分；
- **把「竞争策略分析」落到可实施的优化方案与新功能设计**（第 7、8、9 章），而非仅静态描述现状。

建议后续将本文第 7、8 章的优化项拆成独立的设计文档与 SQL/代码迁移实现。