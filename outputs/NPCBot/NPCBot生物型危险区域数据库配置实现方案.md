# NPCBot 生物型危险区域数据库配置实现方案（简化版）

## 1. 目标

为 NPCBot 增加由 World 数据库配置的“生物型危险区域”识别功能，解决部分地板技能由固定生物周期施法、无法被现有 `DynamicObject` 扫描识别的问题。

典型案例：

```text
37063（Void Zone）
    -> 召唤 Creature 16697
    -> 16697 周期施放 28865（Consumption）
    -> 28865 对周围目标造成伤害
```

本方案只处理以生物当前位置为中心的圆形危险区域，不引入条件子表、复杂形状、角色策略、难度掩码或召唤者判断。

---

## 2. 数据库配置

### 2.1 表结构

建议新增 World 数据库表：

```sql
CREATE TABLE `npcbot_creature_hazard` (
  `map_id` SMALLINT UNSIGNED NOT NULL COMMENT '地图ID，0表示所有地图',
  `creature_entry` INT UNSIGNED NOT NULL COMMENT '危险区域生物Entry',
  `radius` FLOAT UNSIGNED NOT NULL DEFAULT 0 COMMENT '数据库配置的危险半径',
  `damage_spell_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '伤害法术ID，非0时优先读取法术效果半径',
  `safety_distance` FLOAT UNSIGNED NOT NULL DEFAULT 0 COMMENT '危险半径外的额外安全距离',
  `deactivation_delay_ms` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '危险源消失后继续保留危险区域的时间（毫秒）',
  `comment` VARCHAR(255) NOT NULL DEFAULT '' COMMENT '配置说明',
  PRIMARY KEY (`map_id`, `creature_entry`),
  KEY `idx_creature_entry` (`creature_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='NPCBot生物型危险区域配置';
```

### 2.2 核心配置字段

| 字段 | 说明 |
|---|---|
| `map_id` | 危险生物所在地图；`0` 表示全地图通用 |
| `creature_entry` | 危险区域生物的 Creature Entry |
| `radius` | 固定危险半径，也作为法术半径读取失败时的回退值 |
| `damage_spell_id` | 实际伤害法术 ID；非 `0` 时优先读取该法术的效果半径 |
| `safety_distance` | 在基础危险半径之外额外增加的安全距离 |
| `deactivation_delay_ms` | 生物消失或暂时未被扫描到后，继续保留该危险区域的时间 |
| `comment` | 中文说明，不参与运行时判断 |

### 2.3 唯一键约束

使用：

```text
map_id + creature_entry
```

作为主键，表示同一地图中的同一个生物 Entry 只有一条危险区域配置。

如果后续确实出现同一地图、同一 Entry 需要多套规则的情况，再增加独立 `id`，第一版不提前设计。

---

## 3. 半径计算规则

### 3.1 优先级

最终危险半径按以下顺序计算：

```text
1. damage_spell_id != 0 且成功取得法术伤害效果半径：
   基础半径 = 法术伤害效果半径

2. damage_spell_id == 0 或法术半径读取失败：
   基础半径 = radius

3. 最终危险半径：
   基础半径 + safety_distance + BOT体积补偿
```

公式：

```text
FinalRadius = SpellRadius 或 ConfigRadius
            + SafetyDistance
            + BotCombatReachPadding
```

现有 NPCBot 逻辑已经会对危险区域增加战斗距离补偿，建议继续沿用一致的补偿方式：

```cpp
DEFAULT_COMBAT_REACH * 1.2f
```

或者根据当前检测单位/载具的实际 `CombatReach` 计算。

### 3.2 法术半径读取

加载配置时通过：

```cpp
sSpellMgr->GetSpellInfo(damageSpellId)
```

取得 `SpellInfo`，然后遍历法术效果，优先选择具有伤害语义且 `CalcRadius() > 0` 的效果。

建议识别的直接伤害效果至少包括：

```text
SPELL_EFFECT_SCHOOL_DAMAGE
SPELL_EFFECT_ENVIRONMENTAL_DAMAGE
SPELL_EFFECT_HEALTH_LEECH
SPELL_EFFECT_POWER_BURN
```

对于通过 Aura 周期伤害的法术，还应识别：

```text
SPELL_AURA_PERIODIC_DAMAGE
SPELL_AURA_PERIODIC_DAMAGE_PERCENT
SPELL_AURA_PERIODIC_LEECH
SPELL_AURA_POWER_BURN
```

如果多个伤害效果均有有效半径，取最大值，避免危险范围被低估。

### 3.3 回退规则

出现以下情况时使用数据库 `radius`：

- `damage_spell_id = 0`；
- 法术不存在；
- 法术不存在有效伤害效果；
- 伤害效果的 `CalcRadius()` 小于或等于 `0`。

如果法术半径读取失败且 `radius <= 0`，该配置无有效危险半径，应跳过并记录错误日志。

配置了 `damage_spell_id` 时，数据库 `radius` 不是额外叠加值，只是回退值。

---

## 4. 内存数据结构

建议新增简化规则结构：

```cpp
struct BotCreatureHazardRule
{
    uint32 mapId;
    uint32 creatureEntry;
    uint32 damageSpellId;
    float radius;
    float safetyDistance;
    uint32 deactivationDelayMs;
};
```

其中 `radius` 在加载阶段直接编译为最终基础半径：

```text
有效法术半径或数据库固定半径
```

这样运行时不需要重复查询或解析 `SpellInfo`。

### 4.1 规则索引

按地图和生物 Entry 建立索引：

```cpp
using BotCreatureHazardRules =
    std::unordered_map<uint32, std::unordered_map<uint32, BotCreatureHazardRule>>;
```

结构含义：

```text
MapId
    -> CreatureEntry
        -> HazardRule
```

`map_id = 0` 的配置保存在全局规则集合中。

地图专用配置和全局配置同时匹配时，优先使用地图专用配置。

---

## 5. 运行时危险状态

`DeactivationDelayMs` 要求危险源消失后仍能保留其最后位置，因此不能只在每次扫描时直接生成临时危险点。

建议维护运行时状态：

```cpp
struct BotCreatureHazardState
{
    Position position;
    float radius;
    uint32 deactivationDelayMs;
    uint32 remainingDelayMs;
    bool seenThisUpdate;
};
```

状态键使用：

```text
Creature ObjectGuid
```

如果需要避免热重载或 Entry 复用造成混淆，也可以使用：

```text
Creature ObjectGuid + map_id + creature_entry
```

### 5.1 状态更新规则

每次扫描前：

```text
将所有状态的 seenThisUpdate 设为 false
```

扫描到匹配生物时：

```text
1. 新建或更新该 GUID 的状态
2. 更新当前位置
3. 更新最终危险半径
4. remainingDelayMs = deactivationDelayMs
5. seenThisUpdate = true
```

扫描结束后：

```text
如果 seenThisUpdate == false：
    remainingDelayMs 按 diff 递减

如果 remainingDelayMs > 0：
    继续使用最后一次位置作为危险区域

如果 remainingDelayMs == 0：
    删除状态
```

当 `deactivation_delay_ms = 0` 时，危险生物不再被扫描到后立即删除危险区域。

该延迟主要用于：

- 防止网格扫描偶发遗漏；
- 防止危险区域在更新间隔中闪烁；
- 生物刚消失但地面伤害仍短暂存在的技能；
- 避免 BOT 在边缘来回移动。

---

## 6. 数据加载

建议新增：

```text
NPCBotHazardMgr
```

其职责仅包括：

1. 从 `npcbot_creature_hazard` 读取配置；
2. 校验 `MapId`、Creature Entry 和 Spell ID；
3. 提前计算法术伤害半径；
4. 建立 `MapId + CreatureEntry` 内存索引；
5. 向运行时扫描提供只读规则。

建议在 `SpellInfo` 和 `CreatureTemplate` 已加载后执行：

```text
NPCBotHazardMgr::LoadFromDB()
```

可以在 `BotMgr::Initialize()` 的 NPCBot 数据加载阶段接入，但必须确认此时：

- `sSpellMgr` 已完成 SpellInfo 加载；
- `sObjectMgr` 已完成 CreatureTemplate 加载。

### 6.1 查询语句

```sql
SELECT
  `map_id`,
  `creature_entry`,
  `radius`,
  `damage_spell_id`,
  `safety_distance`,
  `deactivation_delay_ms`
FROM `npcbot_creature_hazard`;
```

`comment` 不需要载入运行时缓存。

### 6.2 加载校验

每条配置至少检查：

1. `map_id` 是否有效，`0` 除外；
2. `creature_entry` 是否存在；
3. `damage_spell_id` 非 `0` 时法术是否存在；
4. `radius`、`safety_distance` 是否为非负值；
5. 法术半径读取失败时 `radius` 是否大于 `0`；
6. 最终基础半径是否大于 `0`。

单条配置错误时跳过该条，不阻止服务器启动。

日志应包含：

```text
MapId
CreatureEntry
DamageSpellId
错误原因
```

加载完成后输出：

```text
成功加载数量
跳过数量
涉及地图数量
```

---

## 7. 运行时扫描

在 `bot_ai::CalculateAoeSpots()` 中增加：

```text
CollectConfiguredCreatureHazards
```

处理流程：

```text
1. 根据当前 MapId 取得该地图规则和全局规则
2. 得到需要关注的 Creature Entry 集合
3. 单次扫描附近 Creature
4. 按 Creature Entry 查找规则
5. 更新对应危险源 GUID 的运行时状态
6. 将有效状态转换为 AoeSpotsVec
```

### 7.1 默认检查条件

匹配生物必须满足：

- 指针有效；
- 已加入世界；
- 存活；
- 与检测单位处于同一地图；
- 与检测单位处于同一相位；
- 位于统一扫描范围内。

第一版不检查：

- 是否敌对；
- 是否可选中；
- 是否可攻击；
- 是否被动；
- 是否为召唤物；
- 当前是否正在施放配置法术。

原因是数据库中明确配置的 `map_id + creature_entry` 已经代表该生物在该地图中应被视为危险源。增加额外动态条件会提高配置和运行时复杂度，并可能导致周期施法间隔中危险区消失。

### 7.2 扫描距离

数据库不再单独配置扫描距离。第一版统一使用现有危险区域扫描范围：

```text
60码
```

若危险半径和安全距离之和大于 `60`，加载时应记录警告。后续可以根据当前地图规则中的最大最终半径动态扩大扫描范围。

### 7.3 单次扫描

不能为每条配置分别调用一次 `Cell::VisitObjects()`。

正确方式是：

```text
当前地图全部危险 Creature Entry
    ↓
一次 Cell::VisitObjects()
    ↓
扫描谓词按 Entry 过滤
```

这样配置数量增加时不会线性增加网格扫描次数。

---

## 8. 与现有 AoeSpotsVec 的兼容

第一版继续使用：

```cpp
using AoeSpotsVec = std::vector<std::pair<Position, float>>;
```

每个有效的生物危险状态加入：

```text
生物当前位置或最后一次有效位置
最终危险半径
```

因此无需修改现有：

- `IsWithinAoERadius()`；
- `CalculateAoeSafeSpots()`；
- `CalculateAttackPos()`；
- `GetInPosition()`；
- BOT 移动控制逻辑。

本方案仅支持圆形危险区域，不处理 Z 轴独立高度、圆环、锥形或直线。

---

## 9. 卡拉赞虚空领域配置示例

配置目标：

```text
地图：532
危险生物：16697
实际伤害法术：28865
固定回退半径：8码
额外安全距离：2码
消失延迟：3000ms
```

SQL：

```sql
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 532 AND `creature_entry` = 16697;

INSERT INTO `npcbot_creature_hazard`
(
  `map_id`,
  `creature_entry`,
  `radius`,
  `damage_spell_id`,
  `safety_distance`,
  `deactivation_delay_ms`,
  `comment`
)
VALUES
(
  532,
  16697,
  8.0,
  28865,
  2.0,
  3000,
  '卡拉赞：虚空幽龙的虚空领域'
);
```

实际运行时：

```text
如果 28865 可以取得有效伤害半径：
    最终半径 = 28865法术半径 + 2码 + BOT体积补偿

如果 28865 无法取得有效伤害半径：
    最终半径 = 8码 + 2码 + BOT体积补偿
```

`37063` 是召唤法术，不用于计算危险半径；`28865` 才是实际范围伤害法术。

---

## 10. 热重载

建议增加：

```text
.npcbot reload hazards
```

重载时：

1. 先将数据库内容加载到临时规则容器；
2. 完成全部校验和半径计算；
3. 成功后替换正式规则缓存；
4. 清理不再匹配新规则的运行时危险状态；
5. 输出加载统计。

如果数据库查询失败，应保留旧缓存，不能先清空现有规则。

热重载不是第一版必须功能，可以先只支持服务器启动加载。

---

## 11. 实施步骤

### 第一阶段

1. 在 `pending_db_world` 新增建表和 `16697` 配置 SQL；
2. 新增 `NPCBotHazardMgr`；
3. 加载并校验六个核心配置字段；
4. 配置法术 ID 时提前解析其伤害效果半径；
5. 按 `MapId + CreatureEntry` 建立内存索引；
6. 在 `CalculateAoeSpots()` 中执行一次生物扫描；
7. 将匹配结果加入现有 `AoeSpotsVec`；
8. 实现 `DeactivationDelayMs` 运行时状态；
9. 测试 `16697 + 28865`。

### 第二阶段（可选）

1. 增加 `.npcbot reload hazards`；
2. 将现有硬编码生物型危险区域迁移到数据库；
3. 根据最大危险半径动态计算扫描距离；
4. 增加调试日志或 GM 调试显示。

---

## 12. 测试要点

### 配置加载

- 正常配置成功加载；
- 不存在的 Creature Entry 被跳过；
- 不存在的 Spell ID 使用固定半径并记录警告；
- 法术无有效半径时使用固定半径；
- 法术和固定半径均无效时跳过配置；
- 地图专用配置优先于 `map_id = 0` 的全局配置。

### 运行行为

- `16697` 出现后被识别；
- BOT 在 `28865` 范围加安全距离内主动离开；
- BOT 在危险区外不产生多余移动；
- `16697` 消失后危险位置保留 `3000ms`；
- 延迟结束后危险状态被清除；
- 多个 `16697` 同时存在时分别生成危险区域；
- 生物 GUID 状态在地图切换、战斗结束和重载后正确清理。

### 回归验证

- 现有 `DynamicObject` 地板技能识别不受影响；
- 现有副本硬编码危险区域不受影响；
- 同一玩家的多个 BOT 继续共享危险扫描结果；
- 没有数据库配置的普通生物不会被误判。

---

## 13. 最终设计结论

第一版数据库只保留以下核心配置：

```text
map_id
creature_entry
radius
damage_spell_id
safety_distance
deactivation_delay_ms
```

危险半径规则为：

```text
配置了有效 damage_spell_id：优先使用法术伤害效果半径
否则：使用数据库 radius
最后：增加 safety_distance 和 BOT 体积补偿
```

运行时按 `MapId + CreatureEntry` 识别危险生物，并用 Creature GUID 保存最后位置，实现 `DeactivationDelayMs`。

该方案保持现有圆形 `AoeSpotsVec` 和 BOT 规避逻辑不变，开发范围小、配置直观，足以覆盖 `16697 + 28865` 以及大多数固定生物型地板技能。
