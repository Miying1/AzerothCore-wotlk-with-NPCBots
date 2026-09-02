# NPCBot 风筝路线规划方案

## 1. 目标与约束

设计一个“风筝指定目标”系统，仅在以下条件同时满足时生效：

```cpp
target->GetVictim() == me
```

也就是说，只有指定目标当前正在攻击该 BOT 本身时，BOT 才进入风筝状态；目标转而攻击其他单位时，风筝逻辑暂停或退出，恢复普通战斗逻辑。

风筝路线不能只考虑“远离目标”，还必须同时满足：

- 目标处于远程攻击范围内；
- BOT 的移动路径可通行；
- 终点和路径不经过危险区域；
- 不把目标带入坦克、治疗者或远程队伍；
- 不超过主人、团队或战斗区域的允许范围；
- 遇到墙体、障碍物或不可达区域时能够绕行；
- 路线不会频繁切换，避免抖动、原地转圈和左右反复横跳；
- 无安全路线时能够降级为普通站位或向坦克方向移动。

核心原则：

```text
安全 > 不带离团队 > 路径可持续 > 保持射程 > 距离目标
```

---

## 2. 与现有代码的关系

当前 NPCBot 已有一部分可复用能力：

| 能力 | 现有实现 |
|---|---|
| 危险区域收集 | `bot_ai::CalculateAoeSpots()` |
| 单点危险判断 | `bot_ai::IsWithinAoERadius()` |
| 环形安全点生成 | `bot_ai::CalculateAoeSafeSpots()` |
| 碰撞位置计算 | `Unit::GetFirstCollisionPosition()` |
| 视线检查 | `Unit::IsWithinLOS()`、`Unit::IsWithinLOSInMap()` |
| 团队分散惩罚 | `BotPositionControl::GetSpreadPenalty()` |
| 攻击站位调整 | `BotPositionControl::TryImproveSpreadPosition()` |
| 移动执行 | `bot_ai::BotMovement()` |
| 普通战斗站位 | `bot_ai::CalculateAttackPos()`、`bot_ai::GetInPosition()` |

相关文件：

```text
src/server/game/AI/NpcBots/bot_ai.h
src/server/game/AI/NpcBots/bot_ai.cpp
src/server/game/AI/NpcBots/botpositioncontrol.h
src/server/game/AI/NpcBots/botpositioncontrol.cpp
src/server/game/AI/NpcBots/Hazards/NPCBotHazardMgr.h
```

不建议直接重写 `CalculateAttackPos()`。它主要用于普通远程攻击站位，风筝需要保存路线状态、检测路径阻塞、预测目标追击路线，因此建议新增独立的风筝路线规划逻辑，并在 `GetInPosition()` 的普通远程站位逻辑之前调用。

---

## 3. 风筝状态设计

建议为每个 BOT 增加独立的风筝状态。目标只保存 `ObjectGuid`，不要长期保存 `Unit*`。

```cpp
struct KiteState
{
    ObjectGuid targetGuid;
    Position destination;
    Position previousDestination;

    float direction = 0.0f;
    float preferredSide = 1.0f;

    uint32 repathTimer = 0;
    uint32 stuckTimer = 0;
    uint8 failedPlanCount = 0;

    bool active = false;
    bool forcedRepath = false;
};
```

也可以不使用结构体，直接拆分为 `bot_ai` 成员变量。

### 3.1 建议接口

```cpp
void SetKiteTarget(Unit const* target);
void ClearKiteTarget();
Unit* GetKiteTarget() const;
bool ShouldKiteTarget(Unit const* target) const;
bool UpdateKiteMovement(Unit* target, uint32 diff);
```

路线规划相关接口：

```cpp
bool BuildKiteRoute(Unit const* target, std::vector<Position>& route) const;
void GenerateKiteCandidates(Unit const* target, std::vector<Position>& candidates) const;
bool IsKitePositionValid(Unit const* target, Position const& position) const;
bool IsKitePathValid(Unit const* target, Position const& start, Position const& end) const;
bool IsKitePathSafeForGroup(Unit const* target, Position const& start, Position const& end) const;
float EvaluateKitePosition(Unit const* target, Position const& position) const;
bool RecoverFromKiteObstacle(Unit const* target, std::vector<Position>& route);
```

---

## 4. 风筝生效条件

`ShouldKiteTarget()` 至少检查以下条件：

```text
风筝目标 GUID 有效
目标存在且存活
BOT 存活
当前目标就是指定目标
目标当前攻击 BOT 本身
BOT 是远程职责
BOT 不处于控制状态
BOT 没有施法或特殊移动限制
BOT 与目标在同一地图和阶段
BOT 没有 STAY、FULLSTOP 等禁止移动状态
```

核心判断：

```cpp
bool bot_ai::ShouldKiteTarget(Unit const* target) const
{
    if (!target || !target->IsAlive() || !me->IsAlive())
        return false;

    if (_kiteTargetGuid.IsEmpty() || target->GetGUID() != _kiteTargetGuid)
        return false;

    if (target->GetVictim() != me)
        return false;

    if (!HasRole(BOT_ROLE_RANGED))
        return false;

    if (CCed(me, true) || IsCasting())
        return false;

    if (!me->IsInMap(target))
        return false;

    if (HasBotCommandState(BOT_COMMAND_STAY | BOT_COMMAND_FULLSTOP))
        return false;

    return true;
}
```

不要用以下条件替代 `GetVictim()` 判断：

```cpp
target->getAttackers().contains(me)
```

攻击者列表只能说明 BOT 在目标的攻击者或仇恨关系中，不能说明目标当前正在攻击 BOT。需求要求的是：

```cpp
target->GetVictim() == me
```

---

## 5. 风筝距离模型

不能使用单一距离阈值，否则 BOT 会在阈值附近频繁启停。应使用带迟滞的距离区间。

远程 BOT 可以采用以下初始参数：

```text
强制逃离距离：16 码
安全最小距离：18 码
理想风筝距离：22 码
最大攻击距离：26 码
路线最大距离：30 码
```

实际值应根据职业、技能射程和 `GetSpellAttackRange()` 动态计算。

建议行为：

| 当前距离 | 行为 |
|---|---|
| 小于 16 码 | 强制远离目标，优先选择安全逃生点 |
| 16～20 码 | 继续沿安全路线拉开距离 |
| 20～26 码 | 保持路线并进行远程攻击 |
| 26～30 码 | 停止继续远离，选择切向或朝团队方向调整 |
| 大于 30 码 | 向目标或团队方向回收，防止脱离战斗范围 |

目标距离应结合双方碰撞体积和攻击范围计算，不应只使用 `GetDistance()` 的裸值。

---

## 6. 候选点生成

风筝点应围绕目标生成环形候选点，而不是每次只选择目标反方向的一个点。

### 6.1 环形候选点

建议生成多层距离、多个角度的候选点：

```text
距离：18、22、26、30 码
角度：每 15° 一个点，共 24 个方向
```

伪代码：

```cpp
for (float distance : { 18.0f, 22.0f, 26.0f, 30.0f })
{
    for (uint8 index = 0; index < 24; ++index)
    {
        float angle = Position::NormalizeOrientation(
            baseAngle + float(index) * float(M_PI) / 12.0f);

        Position candidate = target->GetFirstCollisionPosition(distance, angle);
        candidates.push_back(candidate);
    }
}
```

### 6.2 候选方向优先级

候选方向不应固定。建议按以下优先级生成：

1. 当前风筝方向；
2. 当前方向的小角度修正；
3. 目标左侧切向方向；
4. 目标右侧切向方向；
5. 使目标接近坦克的方向；
6. 朝主人或团队安全区域的方向；
7. 目标反方向的逃生路线。

`preferredSide` 用于记录当前选择的左侧或右侧，避免每次重新规划时左右切换。

---

## 7. 候选点硬过滤

候选点生成后，必须先过滤，不合格点不能参与评分。

### 7.1 终点有效性

候选点必须满足：

```text
地图坐标有效
地面高度合理
不是墙体内部
不是悬崖或不可行走地面
不是危险区域
BOT 可以到达
```

可以复用：

```cpp
IsWithinAoERadius(candidate)
me->IsWithinLOS(candidate.GetPositionX(), candidate.GetPositionY(), candidate.GetPositionZ())
target->IsWithinLOS(candidate.GetPositionX(), candidate.GetPositionY(), candidate.GetPositionZ())
```

注意：LOS 只能说明视线可见，不等于地面路径可通行。

### 7.2 目标射程与视线

远程 BOT 到候选点后，必须仍能攻击目标：

```text
目标与候选点距离不能超过技能射程
目标到候选点必须有视线
候选点不能让 BOT 跑到墙后
```

如果候选点虽然安全，但会失去目标视线，应淘汰。

### 7.3 主人和战斗范围

候选点不能超过允许的主人距离：

```text
有主人 BOT：受 BotFollowDist 和战斗站位上限约束
自由 BOT：受地图可见距离和追击范围约束
```

建议将主人距离作为硬限制，而不是普通评分项；否则 BOT 可能为了风筝持续远离队伍。

---

## 8. 移动路径检测

只检查终点是不够的。BOT 从当前位置到候选点的移动线段也必须进行检查。

建议将路径按 1～2 码采样：

```text
当前位置
    ↓
1/4 路程
    ↓
1/2 路程
    ↓
3/4 路程
    ↓
候选终点
```

每个采样点都检查：

```text
是否在危险区域
是否越过地图边界
是否仍在有效地面
是否超过主人距离
是否穿过团队成员安全半径
```

建议封装：

```cpp
bool IsKitePathValid(Unit const* target, Position const& start, Position const& end) const;
```

### 8.1 碰撞检测原则

优先级如下：

1. 使用引擎正式寻路结果；
2. 使用碰撞检测和路径采样组合判断；
3. 使用 `GetFirstCollisionPosition()` 生成绕行点；
4. 不要仅依赖 `IsWithinLOS()`。

原因：

```text
LOS 只表示视线不被阻挡
LOS 不代表两点之间地面连通
LOS 不能可靠识别悬崖、深坑和不可行走地面
```

---

## 9. 危险区域处理

现有危险区域通过 `CalculateAoeSpots()` 收集，并由 `IsWithinAoERadius()` 判断单个位置。

风筝系统需要同时判断：

```text
候选终点是否危险
BOT 移动路径是否穿过危险区域
目标追击路径是否把目标带入危险区域
危险区域是否正在扩大或移动
```

### 9.1 危险区域分级

#### 绝对禁止区

例如持续伤害地板、即将爆炸区域、毒池和火圈。

- 候选终点在其中：淘汰；
- BOT 路径经过其中：淘汰；
- 目标追击路径经过其中：淘汰或设置极高惩罚。

#### 高惩罚区

例如危险区域边缘或即将扩大的区域。

- 尽量不进入；
- 没有其他路线时才允许短暂穿越；
- 不允许作为最终停留点。

#### 可接受区

只有在所有安全路线都不可用时，才允许短时间经过。

危险半径应加入安全边距：

```text
有效危险半径 = 机制危险半径
              + BOT 碰撞半径
              + 移动误差缓冲
```

建议额外增加 1～2 码的移动缓冲。

---

## 10. 防止把目标带离人群

这是风筝路线与普通远程站位的关键区别。

不能只检查 BOT 候选点到主人的距离，还要预测目标会沿着追击方向移动到哪里。

假设：

```text
E = 目标当前位置
B = BOT 当前位置
P = BOT 候选风筝点
```

目标后续大概率会沿着：

```text
E → P
```

方向追击 BOT。因此需要检查 `E → P` 这条目标追击路线。

### 10.1 团队约束

检查目标追击路线是否：

- 穿过坦克；
- 穿过治疗者；
- 穿过远程 BOT 集合区；
- 穿过其他敌人；
- 进入首领危险区域；
- 离开战斗区域或主人过远。

建议封装：

```cpp
bool IsKitePathSafeForGroup(Unit const* target, Position const& start, Position const& end) const;
```

### 10.2 团队安全半径

可按角色设置不同安全半径：

```text
坦克：3～5 码
近战 BOT：5～8 码
远程 BOT：8～12 码
治疗者：8～12 码
```

目标追击路径穿过这些区域时，应更换另一侧候选点。

### 10.3 坦克方向奖励

如果目标当前没有攻击坦克，优先选择能够让目标逐渐接近坦克、但不会穿过团队的路线。

评分中可加入：

```text
目标更接近坦克：奖励
目标远离坦克：惩罚
目标经过治疗者或远程队伍：高惩罚
目标经过其他敌人：高惩罚
```

不能简单地让 BOT 直接跑到坦克身边，否则目标可能穿过整个远程队伍。

---

## 11. 候选点评分

通过硬过滤的候选点，再进行综合评分。建议分数越低越优先。

```text
Score(P) =
    w1 * 距离偏差
  + w2 * 危险区域惩罚
  + w3 * 路径复杂度惩罚
  + w4 * 带离团队惩罚
  + w5 * 主人距离惩罚
  + w6 * 与其他 BOT 过近惩罚
  + w7 * 方向改变惩罚
  + w8 * 目标脱离坦克惩罚
```

### 11.1 各项优先级

```text
不可达：淘汰
终点在危险区：淘汰
路径穿过危险区：淘汰或极高惩罚
目标追击路线穿过人群：淘汰或极高惩罚
超出主人距离：淘汰
没有目标视线：淘汰
距离不理想：中等惩罚
候选点距离当前点太远：中等惩罚
改变当前方向：中等惩罚
靠近其他 BOT：轻到中等惩罚
```

不要把“距离目标最远”设置为最高优先级，否则 BOT 可能为了拉开距离穿过危险区或团队。

---

## 12. 撞墙与路径失败恢复

不能在撞墙后简单地反向，否则容易形成：

```text
向左撞墙 → 向右撞墙 → 向左撞墙
```

### 12.1 卡住检测

满足以下条件时认为路线被阻塞：

```text
BOT 正在移动
目标点仍未到达
500～800 毫秒内实际移动距离小于约 0.5 码
当前移动方向没有明显变化
```

触发后进入 `KITE_RECOVERING` 状态并重新规划。

### 12.2 绕行方向探测

以当前移动方向为中心，尝试：

```text
左 30°、右 30°
左 60°、右 60°
左 90°、右 90°
```

每个方向测试多个距离：

```text
6 码、12 码、18 码
```

每个点都使用相同的：

```text
终点有效性检查
路径碰撞检查
危险区检查
团队约束检查
目标视线检查
```

### 12.3 沿墙滑行

如果当前方向被挡，但一侧可以通行，应生成沿墙的短路线：

```text
当前位置
    → 墙边安全点
    → 墙体侧后方点
    → 目标环形风筝点
```

不要一次性强行生成远处终点，因为狭窄区域中的长直线路径更容易失败。

### 12.4 连续失败降级

建议处理：

```text
第一次失败：换另一侧
第二次失败：缩短目标距离并尝试绕墙
第三次失败：退出风筝，恢复普通远程站位
```

如果目标距离过近且没有安全路线，可以向坦克方向移动，或交给队伍中的近战 BOT 处理。

---

## 13. 风筝状态机

建议采用以下状态：

```text
KITE_INACTIVE
    ↓ 目标当前攻击 BOT
KITE_PLANNING
    ↓ 找到安全路线
KITE_MOVING
    ↓ 到达路线点
KITE_HOLDING
    ↓ 目标距离、危险区或团队位置变化
KITE_PLANNING
    ↓ 路径阻塞
KITE_RECOVERING
    ↓ 连续失败
KITE_FALLBACK
```

### 13.1 状态说明

#### `KITE_INACTIVE`

未满足风筝条件，执行普通战斗逻辑。

#### `KITE_PLANNING`

生成候选点、过滤并评分，构建新的路线。

#### `KITE_MOVING`

沿缓存路线移动，不因每个更新周期都重新计算而抖动。

#### `KITE_HOLDING`

当前距离合适，BOT 保持位置进行远程攻击，同时继续监测目标、危险区和路线状态。

#### `KITE_RECOVERING`

检测到撞墙、路线不可达或危险区扩展，尝试侧向绕行。

#### `KITE_FALLBACK`

无法找到安全路线时，退出风筝并执行普通攻击站位、向坦克移动或其他职业逻辑。

---

## 14. 路线更新频率

不应每个 `UpdateAI()` 都重新规划。

建议：

```text
普通重规划间隔：300～500 ms
危险区变化：立即重规划
目标位置显著变化：立即重规划
团队位置显著变化：立即重规划
路线阻塞：立即重规划
目标不再攻击 BOT：立即退出风筝或暂停风筝
```

只有以下情况才强制重规划：

- 当前目标 GUID 变化；
- 目标距离变化超过 3～5 码；
- 当前目的地失效；
- 当前路径被阻塞；
- 危险区域发生新增、移动或扩大；
- 坦克或主人移动到新的战斗位置；
- BOT 即将超出战斗范围。

---

## 15. 接入 `GetInPosition()`

推荐在 `bot_ai::GetInPosition()` 中，在普通远程站位计算之前接入：

```cpp
void bot_ai::GetInPosition(bool force, Unit* newtarget, Position* mypos)
{
    // 保留现有前置检查

    if (!newtarget)
        newtarget = me->GetVictim();

    if (!newtarget)
        return;

    if (UpdateKiteMovement(newtarget, lastdiff))
        return;

    // 保留原有 CalculateAttackPos()、MoveChase() 等逻辑
}
```

执行流程：

```text
GetInPosition()
    ↓
检查是否为指定风筝目标
    ↓
检查 target->GetVictim() == me
    ↓
检查 BOT 是否为远程职责
    ↓
更新危险区信息
    ↓
复用当前路线或重新规划路线
    ↓
检查路径和团队约束
    ↓
执行移动
    ↓
继续使用原有远程攻击逻辑
```

风筝路线结果应拥有高于普通分散站位的优先级，避免 `TryImproveSpreadPosition()` 或普通 `CalculateAttackPos()` 覆盖风筝目的地。

---

## 16. 推荐实现顺序

建议分阶段实现，避免一次性修改过多公共战斗逻辑。

### 第一阶段：基础风筝

实现：

- 指定目标 GUID；
- `target->GetVictim() == me` 条件；
- 环形候选点；
- 目标距离控制；
- 终点碰撞和危险区判断；
- 缓存目的地，限制重规划频率。

### 第二阶段：路径安全

实现：

- 路径采样；
- 路径危险区检查；
- LOS 检查；
- 主人距离限制；
- 目标视线和攻击范围检查。

### 第三阶段：团队约束

实现：

- 目标追击路线预测；
- 团队成员安全半径；
- 坦克方向奖励；
- 穿过人群的路径淘汰；
- 与 `BotPositionControl` 的分散惩罚结合。

### 第四阶段：障碍恢复

实现：

- 卡住检测；
- 左右绕行探测；
- 沿墙滑行；
- 连续失败计数；
- 风筝失败降级。

### 第五阶段：优化

实现：

- 路线变化滞后；
- 危险区变化触发重规划；
- 目标运动方向预测；
- 根据职业射程动态调整距离；
- 日志和调试可视化。

---

## 17. 最终行为原则

最终的风筝系统应遵循以下原则：

```text
1. 只有指定目标当前攻击 BOT 时才风筝。
2. 不为了保持距离而进入危险区域。
3. 不为了躲危险区而穿过团队。
4. 不为了继续移动而把目标带离战斗区域。
5. 不只检查终点，必须检查 BOT 的移动路径。
6. 不只检查 BOT 路径，还要预测目标追击路径。
7. 目标点应优先保持当前风筝方向，减少路线抖动。
8. 撞墙后优先侧向绕行，不要立即反向横跳。
9. 连续找不到安全路线时必须降级，而不是无限尝试。
10. 风筝路线规划应独立于普通攻击站位逻辑。
```

理想效果是：

```text
BOT 围绕指定目标在安全区域内移动，
优先沿当前方向持续风筝，
遇到危险区或障碍物时选择侧向绕行，
同时限制目标不能穿过团队或离开战斗区域，
如果无法找到安全路线，则平滑降级为普通战斗行为。
```
