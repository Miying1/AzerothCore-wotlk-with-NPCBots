# 世界BOSS系统设计说明

> 自定义世界BOSS框架的统一战斗机制、技能伤害缩放与数据库规范。

## 一、概述

本框架用于将副本首领复刻为 83 级世界BOSS，强度对齐 10人奥杜尔，
在世界地图上随机刷新（普通生物，非召唤物/无 owner），无副本实例环境。

所有自定义世界BOSS共享一套统一战斗机制，由继承 `WorldBossAI` 的基类 `WorldBossGuardAI` 实现，
召唤物继承 `WorldBossSummonAI`，从而避免为每个 BOSS 重复编写公共逻辑。

## 二、文件结构

| 文件 | 说明 |
|------|------|
| `world_boss_common.h` | 公共常量：entry 定义、本体判断、脱战距离 |
| `world_boss_guard.h` | 基类声明：`WorldBossGuardAI` / `WorldBossSummonAI`、缩放表结构 |
| `world_boss_guard.cpp` | 基类实现：归属锁定、脱战距离、技能缩放、reaction 守卫 |
| `boss_world_<名称>.cpp` | 单个 BOSS 的战斗脚本（如 `boss_world_alar.cpp`） |
| `世界BOSS设计与实现.md` | 本文档 |
| `世界BOSS数据库.sql` | 数据库内容（生物模板、装备模板、光环、法术脚本绑定） |
| `世界BOSS危险区域配置.sql` | NPCBot 生物型危险区域配置（地板型召唤物避让） |

## 三、类继承关系

```
WorldBossAI (ScriptedCreature.h)
   └── WorldBossGuardAI   ← BOSS 本体基类（锁定 + 脱战距离 + 技能缩放）
ScriptedAI
   └── WorldBossSummonAI  ← 召唤物基类（仅技能缩放）
```

- BOSS 本体脚本继承 `WorldBossGuardAI`。
- 需要技能缩放的召唤物继承 `WorldBossSummonAI`。
- 纯近战、无需缩放的召唤物可直接继承 `ScriptedAI`。

## 四、核心机制

### 1. 归属锁定

- BOSS 本体进入战斗（`JustEngagedWith`）时，记录进入战斗时的位置，并锁定开怪玩家及其所在队伍/团队。
- 开怪者通过 `GetCharmerOrOwnerPlayerOrPlayerItself()` 追溯，覆盖宠物 / NPCBot 开怪场景。
- 脱战（`Reset`）或死亡（`JustDied`）时清除锁定状态。

### 2. 脱战距离

- 战斗中每帧检查（子类在 `UpdateAI` 开头调用 `CheckLeash()`），
  移动超过 `WORLD_BOSS_LEASH_RANGE`（150 码）即触发 `EnterEvadeMode(EVADE_REASON_BOUNDARY)` 脱战，
  防止风筝拉脱。
- **与核心回家距离判定的关系（重要）**：`Creature::CanCreatureAttack` 末段会检查「与刷新点/复位点的距离 ≤
  `CreatureLeashRadius`（默认 30 码）」，且对 `isWorldBoss()`（`type_flags` 含 `CREATURE_TYPE_FLAG_BOSS_MOB`）
  的单位**取消了「近期受伤即可离开刷新点」的豁免** —— 一旦被拉开 30 码，全部仇恨引用会同时被判为
  offline，`SelectVictim` 随即以 `EVADE_REASON_OTHER` 兜底脱战（配合 `HARD_RESET` 就是“打着打着BOSS消失”）。
  `BOSS_MOB` 位**刻意保留**（它提供“??”等级显示与免疫击退），因此不改数据，而在代码侧化解：
  `WorldBossGuardAI::CheckLeash()` 在锁定期间做两件事 ——
  ① 把 `IDLE` 运动生成器换成默认 idle：核心的距离基准优先取 `IDLE` 槽的 `GetResetPosition()`，
     而 `MovementType=1` 的 `RandomMovementGenerator` 返回的是漫游目标点/初始点（≈刷新点）而非
     `m_homePosition`；换成 `IdleMovementGenerator`（未重写该方法，基类返回 `false`）后判定才回落到 `m_homePosition`；
  ② 让 `m_homePosition` 跟随 BOSS 自身，使核心的距离判定恒为 0。
  本框架的 `WORLD_BOSS_LEASH_RANGE`（150 码）因此成为唯一脱战距离；
  进入战斗时保存真实出生点（`_savedHomePosition`），脱战/退场前（`EnterEvadeMode` → `RestoreCoreHome()`）还原，
  以免影响“走回家”的目标点；`IDLE` 槽则由核心 `MotionMaster::InitDefault()` 在脱战后自动重建。

- **副本地图没有该限制（与 `BOSS_MOB` 无关）**：`Creature::CanCreatureAttack` 在
  `if (GetMap()->IsDungeon())` 处直接 `return true`，副本/团队副本内的非玩家控制单位
  **完全不做**可见距离与回家距离判定；`isWorldBoss()` 的「近期受伤豁免」也只存在于非副本分支，
  所以开放世界里的 `BOSS_MOB` 单位反而更严格。本框架的 150 码脱战规则是脚本层实现，副本内外都生效。

### 3. 技能伤害缩放

参考裂隙 BOSS（`HeroicDungeonRift::BossAIBase`）的实现，在 AI 的 `DamageDealt` 回调中按倍率表统一放大：

- **直接伤害**：`OnSpellCast` / `OnSpellStart` 记录 `_lastCastSpellId`，按 `direct` 倍率放大。
  该记录在本侧取不到倍率时，回退用施法者当前正在结算的法术（`GetCurrentSpell`）——
  瞬发法术不触发 `OnSpellStart`，且 `OnSpellCast` 在伤害结算**之后**才更新记录，
  否则会错用上一个法术的倍率（例：邪酸吐息 `40595` 结算时记录仍停在血沸 `42005`）。
- **DOT 周期伤害**：扫描目标身上 `SPELL_AURA_PERIODIC_DAMAGE` 光环（施法者为 BOSS、命中缩放表
  **且法术学派与本次结算一致**），按 `periodic` 倍率放大。
  同一目标身上同时存在多条缩放表内的 DOT 时（如血沸 `42005` 为物理、邪酸吐息 `40508/40595` 为火焰），
  学派过滤可避免取到先生效的那条并套用错误倍率。
- **近战（`DIRECT_DAMAGE`）不缩放**。
- 缩放结果用 `std::numeric_limits<uint32>::max()` 封顶，防止溢出。

缩放只对继承基类的 AI 生效，**天然隔离原版内容**，无需依赖全局 entry 范围守卫，
不会影响复用同一法术的原版技能。

#### 触发型 / 召唤物伤害的缩放

很多复刻首领的伤害由「召唤物施放的触发型子法术」造成（如地面火焰、间歇泉、熔岩烈焰），
这些伤害的施法者（attacker）是召唤物而非 BOSS 本体。`DamageDealt` 按施法者分发，
因此**这类伤害必须由继承 `WorldBossSummonAI` 的召唤物承载**才能被缩放，分两种情形：

1. **光环触发的子法术**（`光环 A → 周期触发 → 直伤 B`）：召唤物在 `IsSummonedBy` 中
   `DoCastSelf(A)`，伤害 `B` 会经过召唤物的 `DamageDealt`。由于 `OnSpellCast` 在伤害结算
   **之后**才更新 `_lastCastSpellId`，触发链的**首跳**伤害识别的仍是上一环 `A`，
   故把 `A`、`B` 一并加入缩放表（`A` 只作为时序占位，本身无伤害）。
   例：熔岩烈焰 `40980→40253→40265`、火山间歇泉 `40117→42055→42052`、
   烈焰之痕 `35380→35383`、皇家守卫旋风斩 `26038→26686`。

2. **召唤物直接施放的瞬发直伤**（无触发链）：瞬发法术不触发 `OnSpellStart`，
   且 `OnSpellCast` 在伤害结算之后，仅施放一次时 `_lastCastSpellId` 恒为 0 导致漏缩放。
   此时在施放前调用 `SetLastCastSpellId(spellId)` 预置记录即可。
   例：邪能间歇泉 `40593`、阿兹诺斯烈焰 `40631` / `42003`。

> **复刻时不要复用原版 `NullCreatureAI` 召唤物**（其 `DamageDealt` 不缩放），
> 应新建自定义 entry 继承 `WorldBossSummonAI`，让触发型伤害经召唤物 AI 统一缩放。

### 4. 非开怪队伍无法攻击

WoW 引擎中单位阵营对所有观察者一致，无法做到「不同玩家看到不同阵营」，
因此通过全局 `UnitScript::IfNormalReaction`（`world_boss_reaction_guard`）实现：

- BOSS 锁定后，非开怪队伍的玩家对 BOSS 双向返回 `REP_NEUTRAL`（显示中立、无法攻击）。
- 开怪者本人及同队伍/团队成员仍正常敌对。
- 该机制必须在 `Unit::GetReactionTo` 层面实现，AI 基类无法覆盖，故保留此轻量全局 hook。

## 五、技能伤害缩放配置

倍率表定义于 `world_boss_guard.cpp` 的 `BuildWorldBossSpellScaling()`，结构如下：

| 字段 | 说明 |
|------|------|
| 法术 ID | 需放大的主动施放法术 |
| `direct` | 直接伤害倍率 |
| `periodic` | DOT 周期伤害倍率（无 DOT 则留空） |

示例（奥）：

| 法术 ID | 技能 | direct | periodic |
|---------|------|--------|----------|

> **法术 DBC 数据**：配置倍率表时可查阅 `Spell.csv` 核对法术 ID、伤害数值、持续时间与触发关系，
> 文件路径：`E:\workbuddy\AzerothCore-wotlk-with-NPCBots\outputs\Spell.csv`。

## 六、脚本注册

`custom_script_loader.cpp` 中通过 `AddSC_<名称>()` 注册脚本，常见注册方式：

- `RegisterCreatureAI(类名)` —— 生物 AI（BOSS 本体与召唤物）。
- `RegisterSpellScript(类名)` —— 需要自定义施法逻辑的 BOSS。
- `new 类名()` —— 全局 hook（如 reaction 守卫）。

## 七、数据库内容（`世界BOSS数据库.sql`）

### 导入方式

本目录不在 AzerothCore DB 更新器（`data/sql/updates/pending_db_world/`）的扫描路径内，
因此需**手动导入** `acore_world` 数据库：

```bash
mysql -u<用户> -p<密码> acore_world < "世界BOSS数据库.sql"
```

若希望随 DB 更新器自动执行，可将其复制到 `data/sql/updates/pending_db_world/` 并按
`rev_<时间戳>.sql` 命名。

### 内容概览

| 段落 | 表 | 内容 |
|------|-----|------|
| 1 | `creature_template` | 生物模板（BOSS 本体 + 召唤物） |
| 2 | `creature_equip_template` | BOSS 装备（如埃辛诺斯战刃） |
| 3 | `creature_template_addon` | BOSS 本体可见距离（`visibilityDistanceType = 1`，25 码）；召唤物附加光环由 AI 主动施放，不走此表 |
| 4 | `spell_script_names` | 需要自定义逻辑的法术脚本绑定 |
| 5 | `creature_template_model` | 全部生物的模型（沿用各副本原版模型 ID） |
| 6 | `creature_text` | BOSS 喊话（沿用原版 `BroadcastTextId`，客户端按语言本地化） |
| 7 | `npcbot_creature_hazard` | NPCBot 生物型危险区域配置（见 `世界BOSS危险区域配置.sql`） |

## 八、entry 与模型规范

### entry 分配

| entry 段 | 用途 |
|----------|------|
| `120100` 起 | BOSS 本体 |
| `120500` 起 | 召唤物 |

同一副本来源的 BOSS 及其召唤物建议连续编号，便于通过 `world_boss_common.h` 中的
本体判断函数统一识别。

### 模型规范

所有生物（BOSS 本体与召唤物）的模型**必须使用复刻对象（原版生物）的模型 ID**，
即 `CreatureDisplayID` 沿用原版 `creature_template_model` 中对应生物的值，保证与
WotLK 客户端资源一致，不自行另选模型。

### 命名规范

生物的 `name` 与 `subname`（`creature_template` 字段）**统一使用中文名**：
- `name` 使用中文名（如 `奥`、`伊利丹·怒风`）。
- `subname` 使用国服官方中文称号（如 `凤凰之神`、`背叛者`、`火焰之王`）。

### 元素免疫规范

世界 BOSS 及其召唤物**不得免疫元素伤害**（火焰 / 冰霜 / 自然 / 暗影 / 奥术），
即 `creature_template.CreatureImmunitiesId` 必须指向 `creature_immunities` 表中
`SchoolMask = 0`（无学校免疫）的条目，避免元素伤害职业（火法 / 冰法 / 暗牧等）被免疫废掉。

- 机制免疫（`MechanicsMask`：免控、免嘲讽、免击退等）**不受此限制**，可沿用原版。
- 复刻原版 BOSS 时，若原版免疫表含元素学校（`SchoolMask != 0`），需改用其
  `SchoolMask = 0` 的等价条目（保持 `MechanicsMask` 不变），仅去掉元素免疫。

## 九、扩展指南：新增一个世界BOSS

1. 在 `world_boss_common.h` 中声明新的 entry 常量，并加入本体判断列表。
2. 新建 `boss_world_<名称>.cpp`，继承 `WorldBossGuardAI`，实现 `JustEngagedWith` /
   `Reset` / `JustDied` 与战斗逻辑（`TaskScheduler` 编排技能循环）。
3. 在 `BuildWorldBossSpellScaling()` 中登记该 BOSS 需要放大的技能倍率。
4. 在 `custom_script_loader.cpp` 中声明并调用 `AddSC_boss_world_<名称>()`。
5. 在 `世界BOSS数据库.sql` 中新增对应 `creature_template` 及模型条目。
6. 召唤物继承 `WorldBossSummonAI`（需缩放）或 `ScriptedAI`（纯近战），
   并在 BOSS 脚本中通过 `TaskScheduler` 控制召唤时机与数量。

## 十、危险区域配置要求（NPCBot 避让）

### 目标

NPCBot 依赖 World 表 `npcbot_creature_hazard` 识别并自动避让「生物型危险区域」——
由 BOSS 召唤的生物以其当前位置为圆心、持续或周期性造成范围伤害的地板型区域。
此类伤害的施法者是召唤物而非 BOSS 本体，无法被现有 `DynamicObject` 扫描识别。

### 判定标准

**需要**配置的召唤物须同时满足：

1. 由 BOSS 技能召唤的**生物**（`creature`，非游戏对象）；
2. 该生物**在其位置周围持续或周期性造成范围伤害**（地板技能，典型为「触发光环 → 直伤 AOE」链）；
3. 伤害以生物当前位置为圆心，且持续一段时间。

> 危险生物不限于自定义召唤物：BOSS 技能召唤的**原版生物**同样适用
> （如伊利丹「烈焰碰撞」`40832` 召唤的原版 Flame Crash `23336`），直接填其原版 Entry 即可。
> 若该原版生物在其他内容中也被使用，`map_id` 填 `0` 会一并生效。

**不需要**配置：

- 纯近战召唤物（火焰之子、皇家守卫、奥的余烬等），走正常近战处理；
- 单体追踪型技能（如水晶体「冻结」），不构成范围危险；
- 游戏对象施放的地板（熔岩喷发陷阱、沙陷阱），走 `DynamicObject` / 游戏对象识别路径；
- **一次性瞬发 AOE**（如邪能间歇泉 `40593`）——伤害瞬间完成，NPCBot 无法提前避让，配置无收益。

### 配置表结构与字段规范

```sql
CREATE TABLE `npcbot_creature_hazard` (
  `map_id` SMALLINT UNSIGNED NOT NULL COMMENT '地图ID，0表示所有地图',
  `creature_entry` INT UNSIGNED NOT NULL COMMENT '危险区域生物Entry',
  `radius` FLOAT UNSIGNED NOT NULL DEFAULT 0 COMMENT '数据库配置的危险半径（回退值）',
  `damage_spell_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '伤害法术ID，非0时优先读取法术效果半径',
  `safety_distance` FLOAT UNSIGNED NOT NULL DEFAULT 0 COMMENT '危险半径外的额外安全距离',
  `deactivation_delay_ms` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '危险源消失后继续保留危险区域的时间（毫秒）',
  `comment` VARCHAR(255) NOT NULL DEFAULT '' COMMENT '配置说明',
  PRIMARY KEY (`map_id`, `creature_entry`),
  KEY `idx_creature_entry` (`creature_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='NPCBot生物型危险区域配置';
```

| 字段 | 规范 |
|------|------|
| `map_id` | 世界BOSS 在世界地图随机刷新，填 `0`（全地图通用） |
| `creature_entry` | 危险召唤物 entry（`120500` 段） |
| `damage_spell_id` | 触发链（`A→B`）填直伤法术 `B`，系统优先读其效果半径 |
| `radius` | 回退值；触发链直伤无半径时按地板实际大小填 |
| `safety_distance` | 额外安全距离，一般 `2` |
| `deactivation_delay_ms` | 持续地板填 `2000` |
| `comment` | `世界BOSS-<BOSS名>：<召唤物名>（<机制>）` |

### 半径读取优先级

```
基础半径 = max(radius, damage_spell_id 的有效伤害效果半径)
最终危险半径 = 基础半径 + safety_distance + BOT 体积补偿
```

即 `radius` 是**下限**而非回退值：法术半径取不到或比配置值小时用 `radius`；
若危险区需要比实际伤害范围更大（例如火山 `42052`），调大 `radius` 即可生效。

### 当前已配置的危险区域

| Entry | 召唤物 | BOSS | 伤害法术 | 回退半径 | 机制 |
|-------|--------|------|----------|----------|------|
| 120501 | 烈焰之痕 | 奥 | 35383 | 10 | 火焰地板（周期触发） |
| 120509 | 熔岩拳隐形巡者 | 苏普雷姆斯 | 40265 | 8 | 熔岩烈焰地板（区域光环） |
| 120510 | 火山 | 苏普雷姆斯 | 42052 | 8 | 间歇泉地板（周期触发） |
| 120516 | 毁灭之火 | 阿克蒙德 | 31944 | 8 | 火焰地板（区域光环） |
| 23336 | 烈焰碰撞（原版生物） | 伊利丹 | 40841 | 10 | 地面火焰（周期触发） |
| 23069 | 恶魔之火（原版生物） | 伊利丹 | 40030 | 8 | 眼棱召唤，40029 光环触发范围伤害 |

> 配置 SQL 见 `世界BOSS危险区域配置.sql`。新增 BOSS 时，若其召唤物符合判定标准，
> 需同步补充本表配置并更新上述清单。
