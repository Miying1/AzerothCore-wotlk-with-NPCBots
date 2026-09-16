# 真人BOSS设计方案

> 账号角色学会「化身技能」后变身为攻城 BOSS（模型放大 + 高血高伤 + 免控 + 敌对全目标），
> 被击杀时所有参与战斗的玩家均获得个人掉落。

## 产物文件

| 文件 | 作用 |
|------|------|
| `真人BOSS技能_Spell.csv` | 技能本体（DBC 格式，232 列） |
| `真人BOSS技能SQL（暂不执行）.sql` | 召唤物 `creature_template` + 固定伤害锁 `spell_bonus_data` |
| `真人BOSS化身脚本SQL.sql` | 挂载 93001 → `spell_real_boss_transform` |
| `real_boss_transform.cpp` | 化身 + 掉落脚本（按 GUID 配置） |
| `siege_summon_command.cpp` | 爪牙召唤命令（`.mob <entry> [count]` / `.mob preset <id>`） |

## 技能链

> 技能 DBC 只负责免控光环；模型 / 放大 / 血量 / 伤害 / 掉落全部由 `spell_real_boss_transform` 按 GUID 直接设置。

```
93001 化身攻城BOSS（主技能，学给玩家）
 ├─ Effect1 → DUMMY（挂 C++ 脚本）
 └─ Effect2 → 触发 93011 破甲威势
93011 破甲威势（Aura 123，无视目标护甲）→ 触发 93004
93004 免控入口 → 触发 93005 / 93006 / 93007（免控）
93009 召唤攻城爪牙（独立技能）
93010 攻城炮击（独立技能，固定伤害）
```

## 技能清单

| 技能 ID | 名称 | 关键列 |
|---------|------|--------|
| 93001 | 化身攻城BOSS | `Effect_1=3`（DUMMY 挂脚本）；`Effect_2=64`（触发 93011） |
| 93004 | 免控入口 | 触发 93005 / 93006 / 93007 |
| 93005~07 | 免控一/二/三 | `Aura 77`（MECHANIC_IMMUNITY），机制值写在 `EffectMiscValue` |
| 93009 | 召唤攻城爪牙 | `Effect 28`，`MiscValue 910101/2/3`，`MiscValueB 41` |
| 93010 | 攻城炮击 | `Effect 2`（火焰），固定 8000 |
| 93011 | 破甲威势 | `Aura 123`（物理 `MiscValue 1`），`BasePoints -100000`；**须设「死亡保留」** |

> 免控机制值：12 昏迷 / 5 恐惧 / 7 定身 / 17 变羊 / 1 魅惑 / 2 困惑 / 11 减速 / 13 冰冻 / 30 闷棍。

## 伤害要点

- **平砍**：脚本 `SetBaseWeaponDamage` 直接写固定值（`g_RealBossConfigs[].meleeMin/Max`）。
- **技能固定伤害**：DBC `EffectBonusMultiplier` 保持 0 + `spell_bonus_data` 显式全 0，锁定不吃法伤/攻强；
  数值写在 `EffectBasePoints`（`EffectDieSides=1` 时实际值 = `EffectBasePoints + 1`，要 8000 填 7999）。
- **无视护甲**：由 93011 破甲光环（`Aura 123` + 物理 + 大负值）实现，平砍与物理技能一并生效；纯法术技能（如 93010 火焰）天然不吃护甲。

## 召唤物

- 技能 `93009`：`SummonProperties=41`（守卫者），**无仇恨列表**，自动攻击召唤者的敌对目标。
- `.mob` 命令：创建**纯 `TempSummon`**（`UNIT_MASK_SUMMON`），有仇恨列表、可被嘲讽。
- 两者 `faction=16`，与变身后的 BOSS 同阵营。

## 参与掉落（C++）

玩家角色没有 `lootid`，掉落由脚本直接发放给**所有参与战斗的玩家**（而非最后击杀者）。

流程（`real_boss_transform.cpp`）：

1. `UnitScript::OnDamage` 记录对 BOSS 造成伤害的玩家 → `g_RealBossCombatants`；
2. `PlayerScript::OnPlayerJustDied`：按变身标志光环 `REAL_BOSS_TRANSFORM_AURA_SPELL`（93011）校验形态 → 移除光环 → 恢复阵营/旗标/模型/缩放 → 遍历参与者发物品（背包满改邮件）与金币；
3. `PlayerScript::OnPlayerLogout`：清理参与记录（丢弃其作为 BOSS 的记录 + 从其它 BOSS 的参与者集合中移除自身）。

**注意**：`93011` 必须设置 `SPELL_ATTR3_ALLOW_AURA_WHILE_DEAD`（死亡保留），否则死亡时被引擎 `RemoveAllAurasOnDeath` 提前移除，形态判断失效、掉落无法发放。

## 实施步骤

1. 把 `真人BOSS技能_Spell.csv` 导入 / 追加到 `Spell.dbc`；
2. 执行 `真人BOSS技能SQL（暂不执行）.sql`（召唤物 + 固定伤害锁）；
3. 重新编译 worldserver（`real_boss_transform.cpp` 已注册到 `custom_script_loader.cpp`）；
4. 执行 `真人BOSS化身脚本SQL.sql`（挂载 93001 → `spell_real_boss_transform`）；
5. 在 `g_RealBossConfigs` 填入各真人BOSS 的 GUID 与模型/伤害/血量/掉落；
6. `.learn 93001`、`.learn 93009`、`.learn 93010`。

## 可调参数

| 参数 | 位置 |
|------|------|
| 模型 / 放大 / 血量 / 平砍 / 掉落 / 金币 | `g_RealBossConfigs[]`（按 GUID 独立配置） |
| 敌对阵营 | `REAL_BOSS_FACTION`（公共，当前 16） |
| 变身标志光环 | `REAL_BOSS_TRANSFORM_AURA_SPELL`（当前 93011） |
| 免控范围 | 93005 / 93006 / 93007 |
| 召唤物 | 93009 + `creature_template` |

## 注意事项

1. 变身期间 `SetFaction(16)` 全敌对，无法自疗 / 被同阵营队友治疗；需自疗可另加回血光环。
2. `93011` 须设「死亡保留」属性（见「参与掉落」）。
3. `displayId`（当前示例 `100000`）与召唤物 `CreatureDisplayID` 均为示例值，需按需替换。
