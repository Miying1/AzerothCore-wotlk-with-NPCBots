# NPCBot 首领机制处理方案

## 1. 目标

为 NPCBot 增加可由 World 数据库配置的首领战斗机制，支持：

1. 坦克换嘲；
2. 点名后离开人群；
3. 配置点名 BUFF 剩余多少时间时才开始离开人群。

机制处理应复用现有 NPCBot 的危险区域、移动、站位和职业技能逻辑，避免把副本机制硬编码到各个职业 AI 中。

---

## 2. 机制类型

```text
1 = 坦克换嘲
2 = 点名离开人群
```

建议在代码中定义：

```cpp
enum NPCBotBossMechanicType : uint8
{
    NPCBOT_BOSS_MECHANIC_TANK_SWAP = 1,
    NPCBOT_BOSS_MECHANIC_SPREAD = 2
};
```

---

## 3. 数据库配置表

建议新增 World 数据库表：

```sql
CREATE TABLE `npcbot_boss_mechanic` (
    `map_id` SMALLINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '地图ID，0表示所有地图',
    `mechanic_type` TINYINT UNSIGNED NOT NULL COMMENT '机制类型：1坦克换嘲，2点名离群',
    `boss_entry` INT UNSIGNED NOT NULL COMMENT 'BOSS生物Entry',
    `spell_id` INT UNSIGNED NOT NULL COMMENT 'BOSS施加的Aura或技能ID',
    `aura_stacks` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '触发所需BUFF层数',
    `distance` FLOAT UNSIGNED NOT NULL DEFAULT 0 COMMENT '点名离开人群的距离，单位码',
    `start_before_expire_ms` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'BUFF剩余多少毫秒时开始处理',
    `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '是否启用',
    `comment` VARCHAR(255) NOT NULL DEFAULT '' COMMENT '配置说明',
    PRIMARY KEY (`map_id`, `mechanic_type`, `boss_entry`, `spell_id`),
    KEY `idx_boss_entry` (`boss_entry`),
    KEY `idx_spell_id` (`spell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='NPCBot首领机制配置';
```

### 3.1 字段说明

| 字段 | 说明 |
|---|---|
| `map_id` | 地图 ID；`0` 表示所有地图通用 |
| `mechanic_type` | `1` 为坦克换嘲，`2` 为点名离群 |
| `boss_entry` | BOSS 的 Creature Entry，不使用实例 GUID |
| `spell_id` | BOSS 施加的 Aura 或技能 ID |
| `aura_stacks` | 触发机制所需的 BUFF 层数 |
| `distance` | 点名 BOT 与人群中心保持的距离，单位为码 |
| `start_before_expire_ms` | BUFF 剩余时间阈值，单位为毫秒 |
| `enabled` | 是否启用该配置 |
| `comment` | 中文配置说明 |

`boss_entry` 应使用 Creature Entry，而不是数据库中的唯一 ID 或运行时 GUID。实例中的 BOSS GUID 每次可能不同，Entry 才适合用于通用配置。

---

## 4. 点名剩余时间规则

点名机制采用：

> 当点名 BUFF 剩余时间小于等于配置值时，BOT 才开始离开人群。

例如：

```text
start_before_expire_ms = 5000
```

表示 BUFF 剩余 5 秒或更少时开始离开人群。

| BUFF 剩余时间 | 是否开始离群 |
|---:|---|
| 20 秒 | 否 |
| 10 秒 | 否 |
| 5 秒 | 是 |
| 3 秒 | 是 |
| 0 秒或 BUFF 消失 | 停止离群并准备归位 |

判断逻辑：

```cpp
Aura const* aura = me->GetAura(rule.SpellId);
if (!aura || aura->GetStackAmount() < rule.AuraStacks)
    return;

if (rule.StartBeforeExpireMs > 0 &&
    aura->GetDuration() > rule.StartBeforeExpireMs)
    return;

HandleBossSpread(rule, boss);
```

说明：

- `start_before_expire_ms = 0` 表示不启用剩余时间限制，满足层数后立即处理；
- `start_before_expire_ms = 5000` 表示剩余 5 秒以内处理；
- 该字段只对点名机制生效；
- 坦克换嘲只根据 BUFF 层数判断，不使用该字段。

---

## 5. 配置示例

### 5.1 坦克两层换嘲

本示例表示：T1 和 T2 都在攻击该 BOSS，BOSS 当前攻击 T1；当 T1 身上的换嘲 BUFF 达到 2 层时，由 T2 嘲讽 BOSS。

```sql
DELETE FROM `npcbot_boss_mechanic`
WHERE `map_id` = 531
  AND `mechanic_type` = 1
  AND `boss_entry` = 41413
  AND `spell_id` = 21414;

INSERT INTO `npcbot_boss_mechanic`
    (`map_id`, `mechanic_type`, `boss_entry`, `spell_id`,
     `aura_stacks`, `distance`, `start_before_expire_ms`, `enabled`, `comment`)
VALUES
    (531, 1, 41413, 21414, 2, 0, 0, 1, '坦克两层换嘲');
```

含义：

```text
地图 531
BOSS Entry 41413
当前BOT坦克正在攻击该BOSS
该BOT坦克身上的 Aura 21414 达到 2 层
其他可用坦克接管BOSS
```

坦克换嘲的触发必须绑定到 BOT 当前攻击目标：

```cpp
if (me->GetVictim() != boss)
    return;
```

也就是说，不能只因为附近存在匹配 Entry 的 BOSS，或因为其他单位身上存在相同 Aura，就触发换嘲。只有当前 BOT 坦克的 `GetVictim()` 正是该配置 BOSS 时，才允许继续检查 Aura 层数和执行换嘲。

### 5.2 点名剩余 5 秒时离开人群

```sql
DELETE FROM `npcbot_boss_mechanic`
WHERE `map_id` = 531
  AND `mechanic_type` = 2
  AND `boss_entry` = 41413
  AND `spell_id` = 21414;

INSERT INTO `npcbot_boss_mechanic`
    (`map_id`, `mechanic_type`, `boss_entry`, `spell_id`,
     `aura_stacks`, `distance`, `start_before_expire_ms`, `enabled`, `comment`)
VALUES
    (531, 2, 41413, 21414, 1, 15, 5000, 1, '点名BUFF剩余5秒时离开人群');
```

含义：

```text
地图 531
BOSS Entry 41413
BOT 身上的 Aura 21414 达到 1 层
Aura 剩余时间小于等于 5 秒
BOT 移动到距离人群中心约 15 码的位置
```

---

## 6. 内存数据结构

建议扩展现有 `NPCBotHazardMgr`，增加首领机制规则结构：

```cpp
struct BotBossMechanicRule
{
    uint32 MapId;
    uint8 MechanicType;
    uint32 BossEntry;
    uint32 SpellId;
    uint8 AuraStacks;
    float Distance;
    uint32 StartBeforeExpireMs;
    bool Enabled;
};
```

建议提供查询接口：

```cpp
std::vector<BotBossMechanicRule> const* GetBossMechanics(
    uint32 mapId,
    uint32 bossEntry) const;
```

规则优先级：

1. 优先使用指定地图的配置；
2. 没有指定地图配置时，使用 `map_id = 0` 的全局配置；
3. `enabled = 0` 的配置不参与运行时处理。

加载时应校验：

- 地图 ID 是否有效；
- BOSS Entry 是否存在；
- Spell ID 是否存在；
- `mechanic_type` 是否为支持的类型；
- `aura_stacks` 是否大于 0；
- 点名机制的 `distance` 是否大于 0；
- `start_before_expire_ms` 是否合理。

单条配置错误时跳过该配置，不阻止服务器启动。

---

## 7. 运行时处理

建议在 `bot_ai::GlobalUpdate(uint32 diff)` 中调用统一入口：

```cpp
void bot_ai::UpdateBossMechanics(uint32 diff);
```

使用独立计时器，建议每约 500 毫秒检查一次：

```cpp
uint32 _bossMechanicTimer{};
```

不要复用已有 Aura 检查或危险区域计时器，避免多个功能相互影响。

### 7.1 BOSS 查找条件

每次运行时根据当前地图重新查找 BOSS，不保存长期有效的裸指针。BOSS 应满足：

- 与 BOT 位于同一地图；
- `GetEntry() == rule.BossEntry`；
- BOSS 存活；
- BOSS 处于战斗状态；
- 与 BOT 处于相同相位；
- 在合理扫描范围内。

如果 BOSS 已离开视野或战斗结束，应清理对应机制状态。

---

## 8. 坦克换嘲处理

### 8.1 双坦克触发条件

换嘲机制必须按照当前 BOSS 目标和两个坦克的攻击目标进行判断，不能只检查某一个 BOT 是否正在攻击 BOSS。

假设：

```text
T1 = 当前被 BOSS 攻击的主坦克
T2 = 准备接管 BOSS 的副坦克
```

只有同时满足以下条件，T2 才能嘲讽：

1. T1 和 T2 都存活并处于战斗状态；
2. T1 和 T2 都具有坦克职责；
3. T1 当前攻击目标是配置 BOSS；
4. T2 当前攻击目标也是同一个配置 BOSS；
5. BOSS 当前目标是 T1；
6. T1 正在被该 BOSS 攻击；
7. T1 身上的换嘲 Aura 层数达到 `aura_stacks`；
8. T2 不是 BOSS 当前目标；
9. T2 可以移动并施放嘲讽技能；
10. T2 不在换嘲冷却时间内。

这里的 T1、T2 是根据当前战斗关系确定的临时身份，不强制要求数据库角色标记为“主坦”或“副坦”：

```text
T1 = BOSS 当前目标，并且正在攻击该 BOSS 的坦克
T2 = 正在攻击同一个 BOSS、但不是 BOSS 当前目标的另一个坦克
```

因此，两个 BOT 都被标记为主坦时，只要上述攻击关系成立，也允许 T2 执行配置换嘲。一个主坦和一个副坦时，同样优先按照实际目标关系确定 T1/T2，而不是单独依赖 `IsOffTank()`。

对应关系：

```cpp
bool validTankSwap =
    t1 && t2 &&
    t1->IsAlive() && t2->IsAlive() &&
    IsTank(t1) && IsTank(t2) &&
    t1->GetVictim() == boss &&
    t2->GetVictim() == boss &&
    boss->GetVictim() == t1 &&
    t1->HasAura(rule.SpellId) &&
    t1->GetAura(rule.SpellId)->GetStackAmount() >= rule.AuraStacks;
```

只要以下任一条件成立，就不能触发换嘲：

- T1 没有攻击该配置 BOSS；
- T2 没有攻击同一个 BOSS；
- BOSS 当前目标不是 T1；
- T1 没有正在被 BOSS 攻击；
- 换嘲 Aura 在 T2 身上而不是 T1 身上；
- Aura 层数未达到配置值。

因此，T1 被 BOSS 攻击且达到层数时，**不由 T1 执行换嘲**；由正在攻击同一个 BOSS、且未被 BOSS 攻击的 T2 执行嘲讽。

### 8.2 与当前普通嘲讽逻辑的关系

当前代码中的 `bot_ai::CanTauntTarget()` 已支持通用救场嘲讽，主要包括：

- 非坦克目标的救场嘲讽；
- 被攻击坦克生命值低于约 30% 时的救场嘲讽；
- 主坦/副坦标记和目标标记驱动的嘲讽；
- `Rand() < 50` 的随机嘲讽尝试。

当前代码中的 `bot_ai::CanTauntDistantTarget()` 还支持远程救场嘲讽，但对高等级副本 BOSS 和世界 BOSS 已有排除条件。各职业 AI 还会分别调用普通嘲讽技能，例如战士、圣骑士、德鲁伊、死亡骑士和虫族坦克。

如果 BOSS 配置了 `mechanic_type = 1`，新机制必须优先于这些普通嘲讽逻辑：

1. 禁止 `CanTauntTarget()` 按普通救场、生命值或主副坦标记提前嘲讽该 BOSS；
2. 禁止 `CanTauntDistantTarget()` 绕过配置机制嘲讽该 BOSS；
3. 仅在严格的 T1/T2/BOSS 关系和 Aura 层数条件满足时，由 T2 执行配置换嘲；
4. 没有配置换嘲机制的普通目标，继续使用现有嘲讽逻辑。

建议公共 AI 增加配置判断：

```cpp
bool bot_ai::IsConfiguredTankSwapBoss(Unit const* target) const;
```

普通嘲讽入口应在进入现有条件判断前拦截配置 BOSS。配置机制执行成功后，应设置短暂冷却或记录本次触发状态，避免普通逻辑和配置逻辑在同一更新周期重复施法。

### 8.3 换嘲目标选择

T2 不应通过“附近任意可用坦克”动态选择。建议由当前队伍坦克中筛选：

1. 排除 T1；
2. 必须正在攻击同一个 BOSS；
3. 必须存活且具有坦克职责；
4. 必须不是 BOSS 当前目标；
5. 距离 BOSS 在嘲讽技能有效范围内；
6. 多个候选时优先副坦克，再选择距离 BOSS 最近者。

换嘲成功后设置约 2 至 3 秒冷却，并记录当前 BOSS GUID、Spell ID 和上次处理层数，防止每次更新重复施放。

### 8.4 职业技能执行

公共 AI 只负责判断是否需要换嘲，实际技能由职业 AI 执行：

- 战士使用战士嘲讽技能；
- 圣骑士使用圣骑士嘲讽技能；
- 德鲁伊使用 `Growl`；
- 死亡骑士使用 `Dark Command`。

建议增加职业虚拟接口：

```cpp
virtual bool CastConfiguredTaunt(Unit* boss);
```

这样可以避免在公共 `bot_ai` 中硬编码各职业技能 ID。

---

## 9. 点名离开人群处理

### 9.1 触发条件

1. BOT 存活并处于战斗状态；
2. BOT 身上存在配置的 Aura；
3. Aura 层数达到 `aura_stacks`；
4. Aura 剩余时间小于等于 `start_before_expire_ms`；
5. `distance > 0`；
6. BOT 默认不是坦克；
7. BOT 尚未处于有效离群位置。

如果需要允许坦克离群，建议后续增加独立字段 `spread_include_tank`，第一版默认禁止坦克执行点名离群。

### 9.2 人群中心

收集当前 NPCBot 队伍中满足以下条件的成员：

- 存活；
- 与 BOT 同地图；
- 与 BOT 同相位；
- 不是当前点名 BOT；
- 距离 BOT 不超过合理范围。

对有效成员坐标取平均值，得到人群中心：

```text
中心X = 所有成员X坐标之和 / 成员数量
中心Y = 所有成员Y坐标之和 / 成员数量
中心Z = 所有成员Z坐标之和 / 成员数量
```

如果没有有效队友，不执行离群移动。

### 9.3 离群位置

从人群中心指向点名 BOT，按方向计算目标位置：

```text
方向 = BOT当前位置 - 人群中心
目标位置 = 人群中心 + 归一化方向 * distance
```

例如：

```text
人群中心距离 BOT 为 8 码
配置 distance 为 15 码
BOT 继续沿远离人群方向移动，直到距离达到约 15 码
```

目标位置必须经过：

- 地图高度修正；
- 寻路检查；
- 现有危险区域检查；
- BOSS 战斗距离检查。

移动应复用现有 `BotMovement()` 和 `GetAoeSpots()` 逻辑，避免新增一套移动系统。

---

## 10. 点名结束后的归位

当 Aura 消失或不再满足层数条件时：

1. 停止离群状态；
2. 延迟约 1 秒，避免 Aura 刷新造成频繁进出；
3. 重新调用普通战斗站位逻辑；
4. 如果当前存在危险区域，优先选择安全位置；
5. 清理点名状态。

建议保存以下状态：

```cpp
bool _bossMechanicSpreadActive{};
ObjectGuid _bossMechanicBossGuid;
uint32 _bossMechanicSpellId{};
uint32 _bossMechanicReturnTimer{};
```

不建议始终返回点名开始前的旧坐标，因为 BOSS 可能已经移动，原坐标可能已经不再适合当前战斗站位。

---

## 11. 机制优先级

同一时间有多个状态时，建议使用以下优先级：

1. 死亡、传送和控制状态；
2. 致命危险区域；
3. 点名离开人群；
4. 坦克换嘲；
5. 普通战斗站位；
6. 点名结束后的归位。

点名处理中必须暂时阻止普通 `GetInPosition()` 将 BOT 拉回人群，否则 BOT 会不断在离群和归位之间来回移动。

---

## 12. 推荐代码落点

| 功能 | 建议位置 |
|---|---|
| 配置结构和加载 | `src/server/game/AI/NpcBots/Hazards/NPCBotHazardMgr.h`、`NPCBotHazardMgr.cpp` |
| 机制查询 | `NPCBotHazardMgr` |
| 通用机制检查 | `bot_ai::GlobalUpdate()` 或其统一更新入口 |
| 坦克职责判断 | 现有 `bot_ai::IsTank()` |
| BOSS 当前目标判断 | `Creature::GetVictim()` |
| 点名 Aura 判断 | `Unit::GetAura()`、`Aura::GetStackAmount()`、`Aura::GetDuration()` |
| 离群移动 | 现有 `BotMovement()` |
| 安全位置判断 | `GetAoeSpots()` 和现有危险区域逻辑 |
| 职业嘲讽 | 各职业 NPCBot AI |
| 数据库更新 | `data/sql/updates/pending_db_world/` |

---

## 13. 测试要点

### 数据库加载

- 正常配置能够加载；
- 不存在的 BOSS Entry 被跳过；
- 不存在的 Spell ID 被跳过并记录日志；
- `enabled = 0` 的配置不生效；
- 地图专用配置优先于全局配置；
- `start_before_expire_ms = 0` 时点名立即处理。

### 点名剩余时间

- BUFF 剩余 20 秒时不离群；
- BUFF 剩余 10 秒时不离群；
- BUFF 剩余 5 秒时开始离群；
- BUFF 剩余 3 秒时保持离群；
- BUFF 消失后停止离群并归位；
- Aura 层数不足时不触发。

### 坦克换嘲

- T1 和 T2 没有同时攻击同一个配置 BOSS 时不换嘲；
- BOSS 当前目标不是 T1 时不换嘲；
- T1 没有正在被 BOSS 攻击时不换嘲；
- 换嘲 Aura 不在 T1 身上时不换嘲；
- Aura 层数未达到阈值时不换嘲；
- 达到阈值后由 T2 只触发一次；
- 嘲讽成功后 BOSS 目标发生变化；
- 没有可用副坦克时不重复刷嘲讽；
- 换嘲冷却期间不会重复处理。

### 回归测试

- 没有配置的 BOSS 不影响 NPCBot 正常战斗；
- 普通战斗站位不受影响；
- 现有危险区域规避不受影响；
- 点名 BOT 不会被普通站位逻辑拉回人群；
- 多个 BOSS 或多个点名 BOT 同时存在时状态互不干扰。

---

## 14. 最终规则示例

```text
地图ID：531
BOSS Entry：41413
点名技能：21414
触发层数：1
离群距离：15码
BUFF剩余时间阈值：5000毫秒
```

最终行为：

```text
BOT 获得 Aura 21414，但剩余时间大于 5 秒：继续正常站位
BOT 获得 Aura 21414，剩余时间小于等于 5 秒：移动到距离人群中心约 15 码的位置
Aura 消失：延迟归位，并重新执行普通战斗站位
```
