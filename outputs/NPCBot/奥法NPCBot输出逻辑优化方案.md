# 奥法 NPCBot 输出逻辑优化方案

> 目标文件：`src/server/game/AI/NpcBots/bot_mage_ai.cpp`
> 天赋分支：`BOT_SPEC_MAGE_ARCANE`（`botcommon.h:349`）
> 分析方式：只读源码审查 + 机制推演；本方案未修改任何代码，未做游戏内实测。

---

## 1. 背景与目标

法师 NPCBot 默认天赋为奥术（`bot_mgr_service.cpp:234`）。当前奥术输出分支存在若干机制性缺陷，导致 DPS 明显偏低。本方案基于 WLK 3.3.5a 奥法 PVE 常规输出循环（4+1）、泄蓝/续航双阶段模型，给出可直接落地的优化实现。

目标：

- 让飞弹速射（Missile Barrage）proc 得到 100% 利用；
- 正确使用奥术弹幕（Arcane Barrage）作为消层与移动施法手段；
- 引入"泄蓝 → 唤醒 → 再泄蓝"的完整 PVE 循环；
- 消除现有死代码与缓存过期问题。

---

## 2. 现状代码定位

| 位置 | 内容 |
|---|---|
| `bot_mage_ai.cpp:25-77` | `MageBaseSpells` 技能 ID 枚举 |
| `bot_mage_ai.cpp:79-138` | `MagePassives` 天赋被动枚举 |
| `bot_mage_ai.cpp:139-156` | `MageSpecial` 特殊 buff 枚举 |
| `bot_mage_ai.cpp:158-166` | 伤害/控制/辅助法术清单 |
| `bot_mage_ai.cpp:265-320` | `DoNonCombatActions`（护甲、造宝石、魔法抑制） |
| `bot_mage_ai.cpp:337-392` | `UpdateAI` 每帧主流程 |
| `bot_mage_ai.cpp:394-668` | `DoNormalAttack` 输出主体 |
| `bot_mage_ai.cpp:435-450` | 奥术强化 / 气定神闲（no-GCD 爆发段） |
| `bot_mage_ai.cpp:616-644` | **奥术专精主轮转分支（本方案核心修改点）** |
| `bot_mage_ai.cpp:702-727` | `CheckPots`（唤醒、法力宝石、药水） |
| `bot_mage_ai.cpp:1583-1585` | `arcaneBlastStack` 层数缓存更新 |
| `bot_mage_ai.cpp:1712-1760` | `InitSpells()` 法术等级映射 |

当前奥术分支的实际行为：

```text
1) 奥术强化(12042)期间 且 蓝>30%  → 奥术冲击
2) 蓝<15% 且 奥冲≥2层 / 蓝<30% 且 奥冲≥3层 / 奥冲≥4层且有飞弹速射 → 奥术飞弹
3) 否则 → 奥术冲击
```

---

## 3. 现有问题清单

### P0（直接大幅拉低 DPS）

1. **完全缺失奥术弹幕（Arcane Barrage, 44425）**。全仓库搜不到该 ID，未在 `InitSpells()` 中注册。弹幕是 WLK 奥法清层与移动施法的核心技能，缺失导致层数只能靠奥术飞弹清、移动战完全断档。
2. **飞弹速射（buff 44401）几乎被浪费**。`bot_mage_ai.cpp:626` 要求 `arcaneBlastStack >= 4 && HasAura(MISSILE_BARRAGE_BUFF)` 才施放飞弹，即必须叠满 4 层才消费 proc。正确做法是 **proc 一触发（任意层数）立即施放奥术飞弹**（瞬发且免费）。
3. **满层判定死代码**。奥术充能 debuff（`ARCANE_BLAST_DEBUFF = 36032`）**硬上限 4 层**，`GetStackAmount()` 永远 ≤ 4，任何 `>= 6` 之类的判定恒为假，属于无效逻辑。
4. **奥术冲击层数无清层机制**。`bot_mage_ai.cpp:639` 只要蓝够就一直奥冲；4 层时奥冲蓝耗为 800% 基础值（`:1133-1136` 的 `-1.75f * stack`），会迅速打空蓝。

### P1（收益损失 / 逻辑瑕疵）

5. **未纳入唤醒兜底的泄蓝循环**。奥法 PVE 核心是"蓝多时连续 4 层奥冲泄蓝 → 唤醒回蓝 → 再泄蓝"，现有逻辑完全没有体现。
6. **层数阈值写死**，缺少"满层后按蓝量继续赌 proc 若干次再消层"的动态策略。
7. **`arcaneBlastStack` 缓存会过期**。只在收到 debuff 时刷新（`:1583`），debuff 自然到期后旧值残留，导致后续判断错乱。
8. **气定神闲未与奥冲配合**。`:445-450` 仅 `Rand()<35` 施放 PoM，后续轮转里奥法不使用瞬发奥冲。
9. **硬编码与死代码**：`:620` 用 `me->HasAura(12042)` 而非 `ARCANE_POWER_1`；`:618` 注释写 "Arcane Missiles" 实际是奥冲；`:631-638` 整段注释掉的飞弹逻辑。
10. **法力宝石**：`manaGemCharges` 硬编码为 3（`:1335`），制造无冷却保护，2 分钟 CD 靠 `:1340` 手动覆盖。

---

## 4. 目标循环设计

### 4.1 标准 4+1 循环

- **叠层阶段**：连续奥术冲击，每次叠加 1 层"奥术充能"（最高 4 层），3/4 层伤害最高。
- **消层阶段**：满 4 层后用消层技能清除 debuff，重新开始循环。
- **消层技能优先级**：飞弹速射 proc 的奥术飞弹 > 无速射奥术飞弹 > 奥术弹幕（弹幕蓝耗最低，移动/省蓝时优先）。

### 4.2 动态双阶段

```text
【泄蓝/爆发阶段 Burn】
  触发：奥术强化 / 嗜血英勇 / 有唤醒兜底且蓝充足 / 蓝 > 80%
  行为：叠到 4 层后【不消层】，持续奥冲（4 层伤害最高）
        · proc 飞弹速射 → 打飞弹（顺带清层，然后重新叠）
        · 蓝打空 → 唤醒回蓝（→ 回到常规循环）

【常规阶段 Conserve】
  行为：4+1 —— 奥冲到 4 层
        · 有 proc → 奥术飞弹消层
        · 无 proc 且 蓝 > 60% → 再奥冲 1~2 次（随机额度）继续赌 proc
        · 额度用尽仍未触发 → 奥术弹幕消层
        · 移动 → 弹幕瞬发
        · 蓝紧 → 更早消层 + 必要时寒冰箭回蓝
```

### 4.3 满层后"随机赌 1~2 次"机制

```text
进入满 4 层（blastStack 首次 >= 4）时：
    overstackLimit = urand(1, 2)        // 本轮随机决定赌 1 或 2 次
    overstack      = 0                  // 已额外奥冲次数
层数掉回 < 4（消层/掉层）时：
    overstack = 0, overstackLimit = 0   // 归零，等下一轮满层时再随机
```

> 该随机额度只在进入满层时生成一次，避免每 tick 重随机导致行为抖动。

---

## 5. 最终决策表

```text
burn_phase = 奥术强化 || 嗜血 || 英勇
           || (唤醒可用 && 蓝 > 40%)
           || 蓝 > 80%

优先级（每 GCD 判定一次）：
1. 有飞弹速射 proc                          → 奥术飞弹      （任意层数，最高优先）
2. 层数 >= 4（debuff 硬上限即 4）：
   a. 移动中                                → 奥术弹幕（瞬发）
   b. burn_phase                            → 奥术冲击（保持 4 层泄蓝，不限次数）
   c. 蓝 > 60% 且 已赌次数 < 随机额度(1~2)  → 奥术冲击（继续赌 proc）
   d. 额度用尽 / 蓝 <= 60%                  → 奥术弹幕消层
                                                → 弹幕不可用则无速射奥术飞弹消层
                                                → 都不可用才继续奥冲
3. 移动中（未满层）                         → 奥术弹幕
4. 层数 < 4                                 → 奥术冲击（叠层）
5. 蓝 < 15% 且非 burn                       → 寒冰箭（省蓝回蓝）
```

---

## 6. 代码实现

### 6.1 替换奥术主轮转分支（`bot_mage_ai.cpp:616-644`）

```cpp
if (GetSpec() == BOT_SPEC_MAGE_ARCANE)
{
    // 奥术充能：实时读取（上限 4 层）
    Aura* const blastDebuff = me->GetAura(ARCANE_BLAST_DEBUFF);
    uint8 const blastStack = blastDebuff ? blastDebuff->GetStackAmount() : 0;

    // 层数未满：重置本轮"赌飞弹速射"状态
    if (blastStack < 4)
    {
        arcaneBlastOverstack = 0;
        arcaneBlastOverstackLimit = 0;       // 0 = 本轮尚未随机额度
    }

    bool const has_barrage_proc = me->HasAura(MISSILE_BARRAGE_BUFF);   // 飞弹速射
    bool const in_burst = me->HasAura(ARCANE_POWER_1) || me->HasAura(BLOODLUST_1) || me->HasAura(HEROISM_1);
    bool const evo_ready = IsSpellReady(EVOCATION_1, diff, false);      // 唤醒可用（续航后盾）
    bool const moving = me->isMoving();
    bool const burn_phase = in_burst || (evo_ready && manaPct > 40) || manaPct > 80;

    bool const can_barrage = GetSpell(ARCANE_BARRAGE_1) && IsSpellReady(ARCANE_BARRAGE_1, diff) &&
        can_do_arcane && dist < CalcSpellMaxRange(ARCANE_BARRAGE_1);
    bool const can_missiles = IsSpellReady(ARCANEMISSILES_1, diff) &&
        can_do_arcane && dist < CalcSpellMaxRange(ARCANEMISSILES_1);
    bool const can_blast = IsSpellReady(ARCANE_BLAST_1, diff) &&
        can_do_arcane && dist < CalcSpellMaxRange(ARCANE_BLAST_1);

    if (GetSpell(ARCANE_BLAST_1))
    {
        // 1. 飞弹速射 proc：无论层数立即打（瞬发增伤 + 消层）
        if (has_barrage_proc && can_missiles && doCast(mytar, GetSpell(ARCANEMISSILES_1)))
        {
            arcaneBlastOverstack = 0;
            return;
        }

        // 2. 满 4 层（debuff 硬上限为 4，判定用 >= 4）
        if (blastStack >= 4)
        {
            // 本轮随机额度：赌 1~2 次额外奥冲
            if (arcaneBlastOverstackLimit == 0)
                arcaneBlastOverstackLimit = urand<uint8>(1, 2);

            // 2a. 移动中：弹幕瞬发消层
            if (moving && can_barrage && doCast(mytar, GetSpell(ARCANE_BARRAGE_1)))
            {
                arcaneBlastOverstack = 0;
                return;
            }
            // 2b. 泄蓝模式：保持 4 层持续奥冲（不限次数），蓝尽交给唤醒
            if (burn_phase)
            {
                arcaneBlastOverstack = 0;   // 泄蓝结束回常规后可重新获得额度
                if (can_blast && doCast(mytar, GetSpell(ARCANE_BLAST_1)))
                    return;
            }
            // 2c. 常规且蓝 > 60%：按本轮随机额度继续奥冲赌飞弹速射
            if (manaPct > 60 && arcaneBlastOverstack < arcaneBlastOverstackLimit && can_blast)
            {
                if (doCast(mytar, GetSpell(ARCANE_BLAST_1)))
                {
                    ++arcaneBlastOverstack;
                    return;
                }
            }
            // 2d. 额度用尽仍未触发：奥术弹幕消层（蓝耗最低）
            if (can_barrage && doCast(mytar, GetSpell(ARCANE_BARRAGE_1)))
            {
                arcaneBlastOverstack = 0;
                return;
            }
            // 2e. 无弹幕：无速射奥术飞弹消层
            if (can_missiles && doCast(mytar, GetSpell(ARCANEMISSILES_1)))
            {
                arcaneBlastOverstack = 0;
                return;
            }
            // 2f. 兜底：只能继续奥冲
            if (can_blast && doCast(mytar, GetSpell(ARCANE_BLAST_1)))
                return;
        }

        // 3. 移动中（未满层）：弹幕瞬发
        if (moving && can_barrage && doCast(mytar, GetSpell(ARCANE_BARRAGE_1)))
            return;

        // 4. 叠层：奥术冲击
        if (can_blast && doCast(mytar, GetSpell(ARCANE_BLAST_1)))
            return;
    }
    else if (can_missiles && doCast(mytar, GetSpell(ARCANEMISSILES_1))) // 低级无奥冲
        return;

    // 5. 极低蓝且非泄蓝期：寒冰箭省蓝回蓝
    if (!burn_phase && manaPct < 15 && IsSpellReady(FROSTBOLT_1, diff) && can_do_frost &&
        dist < CalcSpellMaxRange(FROSTBOLT_1) && doCast(mytar, GetSpell(FROSTBOLT_1)))
        return;
}
```

### 6.2 新增常量

```cpp
// MageBaseSpells (bot_mage_ai.cpp:25-77)
ARCANE_BARRAGE_1 = 44425,   // 奥术弹幕

// MageSpecial (bot_mage_ai.cpp:139-156)
BLOODLUST_1 = 2825,         // 嗜血
HEROISM_1   = 32182,        // 英勇
```

- 将 `ARCANE_BARRAGE_1` 追加进 `Mage_spells_damage`（`:158-160`）。

### 6.3 新增成员与初始化

```cpp
// 私有成员区（bot_mage_ai.cpp:1894-1904 附近）
uint8 arcaneBlastOverstack;        // 满 4 层后已额外施放的奥冲次数
uint8 arcaneBlastOverstackLimit;   // 本轮随机额度（1~2，0 = 未生成）

// Reset() / 进入战斗（bot_mage_ai.cpp:1688-1690 附近）
arcaneBlastOverstack = 0;
arcaneBlastOverstackLimit = 0;
```

### 6.4 法术注册（`InitSpells()`，`bot_mage_ai.cpp:1712-1760`）

```cpp
/*Talent*/lvl >= 60 && isArca ? InitSpellMap(ARCANE_BARRAGE_1) : RemoveSpell(ARCANE_BARRAGE_1);
```

> 注意：`IsSpellReady()` 对**未注册**的法术 ID 会返回 `true`（`bot_ai.cpp:7554-7555`），因此必须显式 `InitSpellMap/RemoveSpell`，并配合 `GetSpell(...)` 判空，否则低等级会误判奥术弹幕可用。

### 6.5 依赖头文件

`bot_mage_ai.cpp:1-18` 未包含随机工具头，使用 `urand` 需补：

```cpp
#include "Random.h"
```

（若不便引入，可改用 `Rand() < 50 ? 1 : 2`；但 `Rand()` 每 tick 只生成一次随机值，多个技能概率会高度相关，不推荐。）

---

## 7. 资源管理配合（`CheckPots`，`bot_mage_ai.cpp:702-727`）

泄蓝循环依赖唤醒作为续航后盾，因此资源使用需同步调整：

```cpp
void CheckPots(uint32 diff)
{
    if (me->IsMounted() || IsCasting())
        return;

    if (IsPotionReady() && GetHealthPCT(me) < 50)
        DrinkPotion(false);

    // 唤醒：泄蓝循环的续航核心，蓝 < 35% 且环境安全即用
    if (IsSpellReady(EVOCATION_1, diff) && GetManaPCT(me) < 35 &&
        uint8(me->getAttackers().size()) < (shielded ? 3 : 1))
    {
        if (doCast(me, GetSpell(EVOCATION_1)))
            return;
    }
    // 法力宝石：唤醒不可用时的补充
    if (manaGemCharges > 0 && GetManaPCT(me) < 60 && IsSpellReady(MANA_GEM_1, diff, false))
    {
        if (doCast(me, GetSpell(MANA_GEM_1)))
            return;
    }
    // 法力药水
    if (IsPotionReady() && GetManaPCT(me) < 40)
        DrinkPotion(true);
}
```

资源使用优先级：**唤醒（蓝<35%）→ 法力宝石（蓝<60%）→ 法力药水（蓝<40%）**。

---

## 8. 爆发阶段调整（`bot_mage_ai.cpp:435-450`）

现有 AP 门槛过严（`Rand()<75`、目标血量条件），导致开场爆发经常延迟。建议：

```cpp
// ARCANE POWER：PVE 中只要战斗中、蓝 > 50%、目标为 boss/精英/多目标即可开
if (IsSpellReady(ARCANE_POWER_1, diff, false) && me->IsInCombat() && manaPct > 50 &&
    (mytar->IsPlayer() ||
    (mytar->IsCreature() && mytar->ToCreature()->GetCreatureTemplate()->rank != CREATURE_ELITE_NORMAL) ||
    b_attackers.size() > 1) &&
    !me->GetAuraEffect(SPELL_AURA_ADD_PCT_MODIFIER, SPELLFAMILY_MAGE, 0x0, 0x20, 0x0))  // 与 PoM 互斥
{
    if (doCast(me, GetSpell(ARCANE_POWER_1)))
        return;
}
```

- 移除 `Rand() < 75`（AP 是 2 分钟 CD 的核心爆发，不应随机丢弃）；
- 修正 `me->HasAura(12042)` 硬编码 → `ARCANE_POWER_1`；
- 气定神闲施放后，在轮转中插入"PoM 存在时优先瞬发奥术冲击"。

---

## 9. 连带清理

1. **层数缓存实时化**：删除成员 `arcaneBlastStack`、`:1583-1585` 的更新、`:1689` 的初始化，统一改为实时 `me->GetAura(ARCANE_BLAST_DEBUFF)`，彻底消除"层数缓存过期"问题。
2. **删除死代码**：移除 `:631-638` 注释块，修正 `:618` 的错误注释。
3. **法力宝石**：制造加冷却保护，或还原宝石真实充能数（`:1330-1335`）。

---

## 10. 参数总表

| 参数 | 建议值 | 含义 |
|---|---|---|
| 层数上限 | 4（硬上限） | `>= 4` 即为满层 |
| 满层赌 proc 额度 | `urand(1, 2)` 随机 | 每轮满层随机赌 1~2 次奥冲 |
| 额外奥冲蓝量门槛 | 60% | 仅蓝 > 60% 才继续赌 |
| 泄蓝退出蓝量 | 40%（无唤醒） | 有唤醒兜底才能压到 40% 以下继续泄 |
| 泄蓝触发蓝量 | > 80% / 有唤醒且 > 40% / 爆发期 | 进入 burn_phase |
| 唤醒触发蓝量 | 35% | 蓝低于此且安全即引导 |
| 宝石触发蓝量 | 60% | 唤醒不可用时补充 |
| 药水触发蓝量 | 40% | 法力药水 |
| 寒冰箭省蓝 | 15% | 非泄蓝期极低蓝回蓝 |

---

## 11. 验证与风险

| 项 | 说明 |
|---|---|
| 编译 / lint | 改动后执行 `python apps/codestyle/codestyle-cpp.py`，CI 需通过 `-Werror` |
| 引导法术 | 奥术飞弹为引导法术，需确认 `IsCasting()`（`:383`）能锁住引导期间的重复施法；移动中引导会被打断，必要时为 proc 飞弹补 `!me->isMoving()` |
| 等级门槛 | 奥术弹幕 60 级，需在 `InitSpells` 判级，避免低等级 `GetSpell` 返回 0 |
| 数值验证 | 木桩/副本记录 DPS、4 层保持率、proc 消耗率（应 ≈100%）、唤醒次数/战斗、蓝量曲线是否呈"锯齿状"（泄蓝→唤醒→再泄） |
| 回归 | 改动封闭在 `GetSpec() == BOT_SPEC_MAGE_ARCANE` 块 + 新增常量内，火/冰分支不受影响 |

---

## 12. 实施步骤建议

1. 新增常量（`ARCANE_BARRAGE_1` / `BLOODLUST_1` / `HEROISM_1`）与成员（`arcaneBlastOverstack` / `arcaneBlastOverstackLimit`）；
2. `InitSpells()` 注册奥术弹幕，补充 `#include "Random.h"`；
3. 重写奥术主轮转分支（`616-644`）；
4. 层数实时化，删除 `arcaneBlastStack` 缓存；
5. 调整爆发段（`435-450`）与 `CheckPots`（`702-727`）；
6. 清理死代码（`631-638`）；
7. 编译 + lint + 木桩实测调参。
