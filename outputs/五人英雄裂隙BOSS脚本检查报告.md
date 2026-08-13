# 五人英雄裂隙 BOSS 实现脚本检查报告

> 检查范围：`src/server/scripts/HeroicDungeonRift/` 下全部 47 个 `boss_rift_*.cpp` 脚本 + 整合 SQL
> 法术数据来源：`outputs/Spell_dbc.csv`；对比基线：本地 `acore_world` 中纳克萨玛斯 10 人 BOSS 数据
> 检查日期：2026-08-13

## 结论速览

| # | 检查项 | 结论 |
|---|---|---|
| 1 | T2/T3 新增读条技能的免疫打断 | ❌ 实现方式与"读条不可打断"的意图不符 |
| 2 | 技能强度缩放（T1 = 纳克萨玛斯 10 人平均） | ✅ 近战基线正确；法术伤害用 ×15 启发式，需实测 |
| 3 | T1 血量基线（= 纳克萨玛斯 10 人平均的 6/10） | ❌ 严重不达标，差约 34～794 倍 |

---

## 1. T2/T3 新增读条技能的免疫打断 ❌

### 现状

所有 T2/T3 新增技能均通过 `CastIfConfigured(目标, 技能, true)` 释放，即 `triggered=true`。
核心链路 `UnitAI::DoCast → Unit::CastSpell(..., true) → Spell::prepare` 中：

```cpp
// Spell.cpp:3620
m_casttime = HasTriggeredCastFlag(TRIGGERED_CAST_DIRECTLY) ? 0 : m_spellInfo->CalcCastTime(m_caster, this);
```

`triggered=true` 会把 `m_casttime` 直接置 0，**法术变成瞬发，客户端根本不显示读条**。

### 问题

代码注释写的是"读条不可打断"，但实际效果是"瞬发、无读条"。二者语义不同：

- 期望（读条不可打断）：玩家**能看到读条**，但 Kick/拳击等打断技能**无效**。
- 现状（triggered=true）：**没有读条**，打断自然无从谈起。

"免疫打断"是"瞬发"的副作用被附带实现的，而不是"在保留读条的前提下禁止打断"。凡是有读条时间（`CastingTimeIndex > 1`）的新增技能，其读条都被抹掉了。

### 受影响的读条技能清单（`CastingTimeIndex > 1`，现均被 forced 瞬发）

| 技能 | ID | 读条 | 出现位置 |
|---|---|---|---|
| 闪电箭 | 15801 | 4000ms | 阿格曼奇 T2 |
| 毒液喷吐 | 15664 | 500ms | 烟网蛛后 T2 |
| 荆棘诅咒 | 16247 | 2000ms | 蕾瑟塔蒂丝 T2 |
| 冰冻术 | 18763 | 6000ms | 莱斯·霜语 T2 |
| 烈焰风暴 | 12468 | 4000ms | 弗莱拉斯 T3 / 杜安 T3 |
| 暗影箭 | 20791 | 4000ms | 索瑞森 T3 / 七贤 T3 / 伊莫塔尔 T3 / 加丁 T3 |
| 恐惧 | 12096 | 6000ms | 莱斯·霜语 T3 |
| 支配心灵 | 14515 | 2000ms | 安娜丝塔丽 T3 |
| 纠缠根须 | 12747 | 6000ms | 穆坦努斯 T3 / 沃尔丹 T3 |

其余标注"读条不可打断"的新增技能（痛击 3391、旋风斩 15589、致死打击 16856、雷霆一击 15588、破甲 15572 等）在 DBC 里本就是**瞬发**（`CastingTimeIndex=1`），`triggered=true` 无害，但"读条不可打断"的注释同样是错误描述——它们从未有读条。

### 修复建议

- 若意图确为"读条 + 不可打断"：应改为 `triggered=false` 恢复读条，同时对该次施法施加打断免疫。可选方案：
  1. 优先选用 DBC `InterruptFlags` 不含 `SPELL_INTERRUPT_FLAG_INTERRUPT(0x08)` 的法术（本次列出的读条技能其 `InterruptFlags` 均含 0x08，会吃 Kick）；
  2. 或在该技能施法期间临时 `me->ApplySpellImmune` 屏蔽 `SPELL_EFFECT_INTERRUPT_CAST`；
  3. 或明确接受"可被打断"作为机制，删除误导性注释。
- 若意图其实是"瞬发、不可打断"：当前代码正确，但需把注释从"读条不可打断"改为"瞬发"。

---

## 2. 技能强度缩放（T1 = 纳克萨玛斯 10 人平均）✅（部分）

### 近战基线 ✅

整合 SQL 对主 Boss 设 `DamageModifier=35.0`，`damage_multiplier` T1/T2/T3 = 1.0/1.4/2.0。

实测 `acore_world` 中纳克萨玛斯 10 人全部 18 个 Boss 的 `DamageModifier` **全部为 35.0**：

| 纳克萨玛斯 10 人 BOSS | DamageModifier |
|---|---|
| 18 个 Boss（科尔苏加德/萨菲隆/憎恨者/缝补者/泰迪斯……四骑士） | 35.0（全部一致） |

因此 T1 近战基线 = 35 × 1.0 = 35，**与纳克萨玛斯 10 人平均值完全一致**，符合要求。
T2 = 49（×1.4）、T3 = 70（×2.0），相对基线缩放合理（T3 已达纳克萨玛斯 10 人 2 倍，对 5 人队伍偏激进，属难度设计取舍）。

### 法术伤害 ⚠️ 用 ×15 启发式，需实测

多数 Boss 在 `ConfigureTier()` 里 `SetRaidSpellDamageMultiplier(15.0f)`，`DamageDealt` 对非直接伤害再乘一次 Tier 倍率，即法术伤害 ≈ 原始 × 15 × (1.0/1.4/2.0)。这是把低级副本法术的原始伤害抬到 83 级区间的启发式系数，外加少量固定/周期伤害用 `CastCustomSpell(SPELLVALUE_BASE_POINT0)` 局部覆盖。

该 ×15 系数**并非由纳克萨玛斯法术伤害基准反推得出**，是否真正落到"纳克萨玛斯 10 人平均法术强度"需进服实测对比（例如对比纳克萨玛斯 10 人 BOSS 单发法术对 80 级玩家的期望伤害）。

---

## 3. T1 血量基线（= 纳克萨玛斯 10 人平均的 6/10）❌ 严重不达标

### 目标值

纳克萨玛斯 10 人 18 个 Boss 的实际血量（`basehp2(83) × HealthModifier`）：

| 指标 | 数值 |
|---|---|
| 最高（憎恨者 16011） | 6,693,600 |
| 最低（四骑士 16063–16065） | 780,920 |
| **平均** | **2,864,923（约 286 万）** |
| **平均的 6/10** | **1,718,954（约 172 万）** |

（若剔除四骑士与高希 5 个低血子 Boss，主 Boss 平均值约 366 万，6/10 约 220 万。）

### 现状

整合 SQL 用以下公式反向折算 `HealthModifier`，使"等级升到 83 后模板基础生命与原低级源 Boss 一致"：

```sql
新 HealthModifier = 源 basehp × 源 HealthModifier ÷ basehp2(83)
-- 因此 83 级模板实际生命 = 源 basehp × 源 HealthModifier = 源 Boss 原始生命
```

再乘 `health_multiplier` T1/T2/T3 = 1.0/1.8/3.5。

实测源 Boss 原始生命范围：

| 源 Boss | 原始生命（≈ 当前 T1 生命） |
|---|---|
| 指挥官斯普林瓦尔 4278 | 2,165 |
| 范克里夫 639 / 斯尼德 642 / 穆坦努斯 3654 | 3,872 |
| 黑暗院长加丁 1853（最高） | 50,300 |

即当前 T1 Boss 生命落在 **2,165～50,300**，而目标为 **约 172 万**，**差距约 34～794 倍**。即使 T3（×3.5）也只到 13,535～176,050，仍远低于目标。

### 根因

设计文档 6.3 节明确写了"这使升级前后的模板基础生命保持一致……最终生命绝对值仍需单独进服实测和调优"，即**血量从未被抬到 83 级团队 BOSS 量级**，只是保持源 Boss 原值。这与"T1 = 纳克萨玛斯 10 人平均的 6/10"的要求直接冲突。

### 修复建议

T1 血量应统一落到约 172 万。建议放弃"反向折算保持源生命"的公式，改为按目标值正向设置 `HealthModifier`：

```sql
新 HealthModifier = 目标T1生命(约 1,718,954) ÷ basehp2(83, 对应unit_class)
-- 例如战士类：1,718,954 ÷ 13945 ≈ 123
```

再由 `health_multiplier` 1.0/1.8/3.5 得到 T2≈309 万、T3≈602 万。这样 47 个 Boss 的 T1 才有统一基线，而不是各自保留千级源生命。

---

## 附：本次核查依据

- 法术读条/打断标志：`outputs/Spell_dbc.csv`（列 `CastingTimeIndex`、`InterruptFlags`）。
- 纳克萨玛斯 10 人基线：`acore_world.creature_template` 中 18 个纳克萨玛斯 Boss（entry 15928/15931/15932/15936/15952/15953/15954/15956/15989/15990/16011/16028/16060/16061/16062/16063/16064/16065）。
- 源 Boss 原始生命：`creature_template` JOIN `creature_classlevelstats`（`basehp0` 按 exp=0 取）。
- 打断机制：`Unit::InterruptSpell` / `Spell::EffectInterruptCast`（`PreventionType==SILENCE` 且 `InterruptFlags & 0x08` 才可被 Kick 打断）；`triggered=true` 经 `UnitAI::DoCast` 使 `m_casttime=0`。
