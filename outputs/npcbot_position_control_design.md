# NPCBot 走位控制功能设计方案

## 1. 文档定位

本文是 AzerothCore WotLK with NPCBots 的**实施设计**，不是已经提交的代码补丁。目标是在尽量不改变现有战斗 AI 优先级的前提下，增加以下临时走位控制：

1. `mass`：非坦克、非守卫 Bot 集合到主人身边并继续跟随。
2. `mass ranged`：仅远程和治疗职责 Bot 集合到主人身边并继续跟随。
3. `unmass`：取消集合约束，恢复现有跟随和战斗走位。
4. `spread`：战斗中尽量与其他 Bot 保持指定间距，默认 5 码。
5. `spread off`：取消分散约束。

本方案不修改数据库，不新增持久化字段，不改变 `BotMgr::GetBotFollowDist()` 的全局语义，也不配置或构建项目。

---

## 2. 已确认的现有调用链

### 2.1 命令入口

文件：

```text
src/server/game/AI/NpcBots/botcommands.cpp
```

`.npcbot command` 子命令表位于约 639 行，已有：

```cpp
{ "standstill", HandleNpcBotCommandStandstillCommand, ... },
{ "stopfully",  HandleNpcBotCommandStopfullyCommand,  ... },
{ "follow",     npcbotCommandFollowCommandTable       },
```

建议把新命令注册到同一个子表。第一版可复用：

```cpp
rbac::RBAC_PERM_COMMAND_NPCBOT_COMMAND_MISC
```

这样不需要修改 `RBAC.h` 和权限数据库。后续如果服务器管理员需要单独授权，再新增专用权限。

### 2.2 普通跟随位置

文件：

```text
src/server/game/AI/NpcBots/bot_ai.cpp
```

函数：

```cpp
void bot_ai::_calculatePos(Unit const* followUnit, Position& pos, float* speed) const;
```

现有函数按坦克、远程、DPS 和其他职责计算角度、距离、碰撞点和速度。集合模式应在函数计算普通职责槽位之前调用控制器：

```cpp
if (master->GetBotMgr()->GetBotPositionControl()->TryGetMassPosition(*this, followUnit, pos, speed))
    return;
```

只有控制器确认当前 Bot 是集合参与者时才返回 `true`；坦克、守卫、死亡、载具或跨地图等情况必须返回 `false`，继续走原有逻辑。

### 2.3 战斗追击入口

函数：

```cpp
void bot_ai::GetInPosition(bool force, Unit* newtarget, Position* mypos);
```

现有入口在开头处理 `BOT_COMMAND_STAY`、战斗位置开关、控制效果和目标有效性；之后可能调用 `CalculateAttackPos()`、`BotMovement(BOT_MOVE_POINT, ...)` 或近战 `BOT_MOVE_CHASE`。

集合模式不能像 `BOT_COMMAND_STAY` 那样在函数最前面直接 `return`，因为这样会跳过底部的攻击状态更新，也无法保证跟随移动中的主人。正确做法是：

1. 在确定 `newtarget` 后判断是否为敌对目标。
2. 如果当前 Bot 是集合参与者且当前位置不能攻击该目标，跳过本次敌方移动/追击。
3. 保留函数后面的攻击状态维护，使 Bot 仍可在当前位置攻击、施法、治疗。

伪代码：

```cpp
bool holdMassPosition =
    master->GetBotMgr()->GetBotPositionControl()->ShouldHoldMassPosition(*this, newtarget);

if (!holdMassPosition)
{
    // 原有 AdjustTankingPosition、CalculateAttackPos、BotMovement 逻辑
}

// 保留原有 Attack(newtarget, ...) 等战斗状态处理
```

不能把集合约束简单实现为 `SetBotCommandState(BOT_COMMAND_STAY)`，因为该状态会移除 `BOT_COMMAND_FOLLOW` 并停止移动。

### 2.4 远程攻击位置和 AoE

函数：

```cpp
void bot_ai::CalculateAttackPos(Unit* target, Position& pos, bool& force) const;
```

当前逻辑已经处理：

- 主人距离上限；
- 目标正面规避；
- 视线；
- `IsWithinAoERadius()`；
- `CalculateAoeSafeSpots()`；
- 被目标攻击时向坦克或主人靠拢；
- 载具位置计算。

因此分散功能不能覆盖或替换这些条件。推荐只对已经通过硬约束的候选点增加软评分：

```cpp
float score = master->GetBotMgr()->GetBotPositionControl()->GetSpreadPenalty(*this, candidate);
```

现有 `safespots` 选择中，保持“最近安全点”为第一优先级；只有安全性和可攻击性相同或接近时，才用分散评分作为次级排序。

### 2.5 主更新循环

在 `bot_ai.cpp` 约 18436 行，当前 AI 会周期性计算战斗位置；在约 18574 行，如果存在主人或其他跟随对象，则调用：

```cpp
_calculatePos(mmover, movepos, &speed);
```

然后通过：

```cpp
SetBotCommandState(BOT_COMMAND_FOLLOW, true, &movepos, &speed);
```

执行移动。集合模式复用这个分支，就能让 Bot 在战斗中保持战斗逻辑，同时跟随主人更新集合锚点。

需要注意，现有这段分支通常由“无当前战斗走位目标”或其他跟随条件触发。为满足“战斗中不能攻击时仍继续集合”，实现阶段应在该分支前增加一个专门判断：

```cpp
bool massFollow =
    !IAmFree() &&
    master->GetBotMgr()->GetBotPositionControl()->ShouldFollowMass(*this);

if (massFollow)
    mmover = master;
```

然后仍然复用 `_calculatePos()` 和 `SetBotCommandState(BOT_COMMAND_FOLLOW, ...)`。该分支不能无条件覆盖副本强制移动、AoE 逃生、载具控制或坦克位置。控制器应提供 `ShouldFollowMass()`，只对集合参与者且当前没有更高优先级移动任务的 Bot 返回 `true`。
---

## 3. 推荐架构：`BotMgr` 独占的独立控制器

### 3.1 新增文件

```text
src/server/game/AI/NpcBots/botpositioncontrol.h
src/server/game/AI/NpcBots/botpositioncontrol.cpp
```

`src/server/game/CMakeLists.txt` 使用：

```cmake
CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE_SOURCES ...)
```

因此新文件放在 `src/server/game` 子目录下会被递归收录，不需要单独修改 CMake 文件。

### 3.2 为什么不直接扩展 BOT_COMMAND_STAY

已有命令语义明确：

- `BOT_COMMAND_STAY`：原地守卫，停止跟随移动；
- `BOT_COMMAND_FULLSTOP`：停止活动、停止攻击和施法；
- `BOT_COMMAND_INACTION`：只跟随，不做其他动作；
- `BOT_COMMAND_FOLLOW`：恢复战斗跟随。

集合模式是“跟随主人但限制敌方追击”，与以上任一命令都不等价。因此新增状态不放进 `botcommon.h` 的通用命令位，而由 `BotPositionControl` 单独管理。

### 3.3 状态保存方式

推荐让 `BotMgr` 持有一个 `std::unique_ptr<BotPositionControl>`，控制器对象只属于当前主人。这样状态天然随 `BotMgr` 生命周期销毁，不需要全局主人注册表，不会发生不同主人之间的状态串扰，也不需要为全局容器加锁。

`botmgr.h` 只需要前置声明控制器并增加访问器和成员：

```cpp
class BotPositionControl;

BotPositionControl* GetBotPositionControl() const { return _positionControl.get(); }

std::unique_ptr<BotPositionControl> _positionControl;
```

`botmgr.cpp` 在构造函数初始化列表创建控制器，在析构前自动释放。新控制器构造时只接收主人 GUID 或 `BotMgr` 引用，不长期保存裸 `Player*`。

状态结构建议如下：

```cpp
struct BotMassSlot
{
    ObjectGuid botGuid;
    float angle = 0.0f;
    float radius = 0.0f;
    uint32 slotSeed = 0;
};

struct BotPositionState
{
    BotMassMode massMode = BotMassMode::None;
    float massRadius = 4.0f;
    float spreadDistance = 5.0f;
    std::unordered_map<ObjectGuid, BotMassSlot> massSlots;
};
```

控制器只保存 GUID、模式、数字参数和槽位，不保存长期 `Player*`、`Creature*` 或 `Unit*`。每次调用时通过当前 `BotMgr`、`GetBotMap()` 和 `bot_ai` 获取有效对象。

AzerothCore 世界线程的 Bot AI 更新通常在单线程上下文执行，第一版不需要引入锁。若未来命令从异步线程触发，再单独增加线程边界处理。

### 3.4 推荐公开接口

头文件可提供以下接口；具体命名可以在实现时微调：

```cpp
enum class BotMassMode : uint8
{
    None,
    AllNonTank,
    RangedAndHeal
};

class BotPositionControl
{
public:
    explicit BotPositionControl(BotMgr& botMgr);

    bool EnableMass(BotMassMode mode, float radius);
    void DisableMass();
    bool IsMassEnabled() const;
    bool ShouldFollowMass(bot_ai const& ai) const;

    bool EnableSpread(float distance);
    void DisableSpread();
    float GetSpreadDistance() const;

    bool TryGetMassPosition(
        bot_ai const& ai, Unit const* followUnit, Position& pos, float* speed);

    bool ShouldHoldMassPosition(
        bot_ai const& ai, Unit const* target) const;

    float GetSpreadPenalty(
        bot_ai const& ai, Position const& candidate) const;

    void ForgetBot(ObjectGuid botGuid);
};
```

命令处理只负责参数解析和调用主人 `BotMgr` 的控制器；走位控制器不依赖聊天对象。

---

## 4. 命令设计

### 4.1 推荐语法

```text
.npcbot command mass
.npcbot command mass all [radius]
.npcbot command mass ranged [radius]
.npcbot command unmass
.npcbot command spread [distance]
.npcbot command spread off
```

语义：

| 命令 | 行为 |
|---|---|
| `mass` | 默认启用 `all` 模式，半径默认 4 码 |
| `mass all` | 所有符合条件的非坦克 Bot 集合 |
| `mass ranged` | 仅远程或治疗 Bot 集合 |
| `unmass` | 清除集合模式和全部集合槽位 |
| `spread` | 启用战斗分散，距离默认 5 码 |
| `spread 7` | 设定 7 码分散距离 |
| `spread off` | 只关闭分散，不影响集合模式 |

参数校验建议：

- 集合半径限制为 `1.0f <= radius <= 4.0f`；
- 分散距离限制为 `2.0f <= distance <= 20.0f`；
- 拒绝 NaN、负数、非数字和多余参数；
- 无 Bot 时沿用已有命令风格，发送用法并返回失败；
- 重复启用同一模式时刷新半径，但保留已有槽位，避免 Bot 瞬间抖动；
- 从 `all` 切换到 `ranged` 时重新筛选参与者并清理不再参与者的槽位。

### 4.2 “守卫状态”的定义

当前 `standstill` 明确调用：

```cpp
SetBotCommandState(BOT_COMMAND_STAY);
```

因此第一版将“守卫状态”定义为：

```cpp
HasBotCommandState(BOT_COMMAND_STAY)
```

此外，为避免破坏玩家明确下达的停止命令，集合和分散控制均不接管以下状态的 Bot：

```cpp
BOT_COMMAND_FULLSTOP |
BOT_COMMAND_INACTION
```

但这三种状态语义不同：

- `STAY`：站在原地守卫；
- `FULLSTOP`：停止活动；
- `INACTION`：跟随但不战斗。

控制器只读取它们，不自动清除它们。用户使用 `follow` 或其他原有命令恢复后，控制器在下一次计算时重新接管。

### 4.3 坦克筛选

推荐的“坦克职责”判定为：

```cpp
bool isTank = ai.HasRole(BOT_ROLE_TANK) || ai.IsTank();
```

`BOT_ROLE_TANK_OFF` 不默认全部排除，因为副坦克可能当前承担 DPS；但如果 `IsTank()` 表明它正在承担坦克职责，则排除。这样兼顾用户的“排除坦克”和副坦克的实际战斗状态。

集合参与条件：

```cpp
eligible =
    bot != nullptr &&
    bot->IsInWorld() &&
    bot->IsAlive() &&
    !ai.IsWanderer() &&
    !ai.IsTempBot() &&
    !isTank &&
    !ai.HasBotCommandState(BOT_COMMAND_STAY | BOT_COMMAND_FULLSTOP | BOT_COMMAND_INACTION);
```

是否排除临时 Bot 可以作为实现开关；推荐第一版排除，避免副本临时 Bot 与主人状态切换产生意外耦合。

---

## 5. 集合槽位算法

### 5.1 稳定随机，而不是每帧随机

如果每次调用 `_calculatePos()` 都重新随机，Bot 会不断抖动。启用集合时为每个符合条件的 Bot 分配一次稳定槽位：

1. 使用主人 GUID、Bot GUID 和集合模式生成稳定 seed；
2. 用 `urand` 或项目随机辅助函数在启用/新增 Bot 时生成角度和半径；
3. 将结果保存到 `BotMassSlot`；
4. 只有模式切换、槽位失效或 Bot 数量明显变化时才重排。

槽位应满足：

```text
0 < radius <= massRadius
```

默认 `massRadius = 4.0`。建议半径分布优先使用 `sqrt(U) * radius`，使平面面积上更均匀，而不是把 Bot 全部堆在中心。

### 5.2 解决多个 Bot 重叠

单纯随机仍可能重叠。启用或补槽位时，为候选 Bot 生成 8 到 16 个候选点，按照以下分数选最低者：

```text
score =
    与现有集合 Bot 的重叠惩罚
  + 与主人碰撞的惩罚
  + 超出可行地面/视线的惩罚
  + 与上一次槽位距离变化的惩罚
```

这不是严格的几何间距保证；Bot 数量较多时，4 码半径内不可能让所有 Bot 都保持 5 码互距。设计目标是“随机分散、不重叠、稳定”，而不是物理上不可满足的严格约束。

### 5.3 位置合法性

计算候选点时复用核心已有能力：

- `GetFirstCollisionPosition()`；
- `GetNearPoint()`；
- `UpdateAllowedPositionZ()`；
- `IsWithinLOS()`；
- 地图、相位、载具和飞行状态检查。

如果主人周围 4 码全部不可行：

1. 优先选择最近的合法点；
2. 必要时将半径临时收缩到 1 码以上；
3. 如果没有合法点，返回 `false`，交给原有 `_calculatePos()`；
4. 不传送、不穿墙、不强制覆盖副本脚本的位置。

### 5.4 主人移动时的刷新

每次调用控制器时：

- 主人未移动且当前点合法：直接返回缓存槽位；
- 主人移动超过约 1 码，或主人朝向变化明显：重新把缓存局部坐标投影到主人当前位置；
- 目标点失去 LOS、地图变化或被障碍物阻挡：只重算该 Bot 的槽位；
- 不因每个服务器 tick 全量随机重排。

集合 Bot 使用普通的 `SetBotCommandState(BOT_COMMAND_FOLLOW, true, &movepos, &speed)` 继续移动，不直接调用新的移动 API。

---

## 6. 集合状态下的战斗行为

### 6.1 保留战斗逻辑

集合控制器只限制“移动位置”，不限制：

- 目标选择；
- 自动攻击；
- 法术施放；
- 治疗、驱散和辅助；
- 仇恨、战斗重置和死亡处理。

如果目标位于当前攻击/施法有效距离和视线内，Bot 正常攻击或施法；如果当前位置无法攻击目标，则不为了追击目标离开主人集合半径。

### 6.2 不能攻击时的处理

建议 `ShouldHoldMassPosition()` 只在以下条件同时满足时返回 `true`：

```text
集合模式参与者
+ 目标是敌方目标
+ Bot 不处于坦克/强制副本走位逻辑
+ 当前点不能满足原有 CanBotAttack/攻击距离/视线条件
```

在 `GetInPosition()` 中，返回 `true` 后只跳过以下动作：

- `CalculateAttackPos()` 生成敌方追击点；
- `BOT_MOVE_CHASE`；
- 向敌方目标的 `BOT_MOVE_POINT`。

仍然执行：

- 现有攻击状态更新；
- 允许在当前点进行的攻击；
- 法术、治疗和其他 AI 逻辑；
- 后续主人跟随分支。

这样才能实现“打得到就打，打不到不追，但继续跟主人移动”。

### 6.3 高优先级例外

集合约束不能阻止以下高优先级行为：

1. 传送、载具、死亡和复活流程；
2. 副本脚本明确强制的移动；
3. 躲避 AoE 和危险区域；
4. 脱离控制、脱离卡死和必要的脱离目标正面；
5. 坦克/副坦克正在承担威胁时的战斗位置。

第一版若无法可靠识别副本脚本强制移动，应采用“控制器只提供建议点，原有强制移动可覆盖”的规则，而不是在所有 `BotMovement` 调用上做全局拦截。

---

## 7. 战斗分散算法

### 7.1 分散是软约束

分散模式不能简单地把所有 Bot 强行推到彼此 5 码外，因为这可能破坏：

- AoE 安全点；
- LOS；
- 目标正面规避；
- 施法距离；
- 坦克站位；
- 副本机制位置。

优先级固定为：

```text
传送/载具/死亡/控制
> 副本强制位置
> AoE 安全
> 坦克和威胁位置
> 目标正面规避
> 攻击/施法距离和 LOS
> 分散软约束
> 默认随机位置
```

### 7.2 候选点评分

对于 `CalculateAttackPos()` 已生成的候选点，使用：

```text
spreadPenalty = 0

如果 candidate 与其他活动 Bot 的距离 < desiredDistance：
    spreadPenalty += (desiredDistance - actualDistance)^2

如果 candidate 与主人或坦克重叠：
    spreadPenalty += 更高惩罚

如果 candidate 远离当前点过多：
    spreadPenalty += 小幅移动成本
```

在候选点选择时使用：

```text
最终分数 = 原有位置质量分数 + spreadPenalty
```

但以下条件仍是硬过滤，不能通过分数抵消：

```text
IsWithinAoERadius(candidate) == true
candidate 不在 LOS
candidate 超出主人允许距离
candidate 无法攻击或施法
```

### 7.3 对现有代码的最小改法

不建议重写 `CalculateAttackPos()`。建议分三步：

1. 把当前函数中“生成候选点并立即 return”的局部逻辑提取为少量候选点；
2. 每个候选点先执行原有 `toofaraway`、LOS、AoE 和 `canattack` 检查；
3. 通过 `BotPositionControl::GetSpreadPenalty()` 进行次级比较。

对于 `CalculateAoeSafeSpots()` 的结果，继续优先选择安全且可攻击的点；只在多个点同样安全时使用分散惩罚。对于现有的 `closestPos`/`closestAttackPos`，可把比较条件扩展为“距离相同范围内优先选择分散分数更低的点”。

### 7.4 分散对象

默认只统计同一主人、同一地图、同一相位、在世界中且存活的 NPCBot。不要在每个 Bot 的分散评分中遍历全地图单位。

可以缓存本次主人 Bot 集合快照，按一次战斗位置更新批次复用；第一版即使每个 Bot 遍历主人 Bot 数量，复杂度也只是 `O(n^2)`，对常见 5 到 10 个 Bot 可接受。若未来 Bot 数量增大，再加入刷新周期或空间网格。

---

## 8. 最小代码侵入清单

### 8.1 新增文件

```text
src/server/game/AI/NpcBots/botpositioncontrol.h
src/server/game/AI/NpcBots/botpositioncontrol.cpp
```

主要逻辑全部放入这两个文件。

### 8.2 `botcommands.cpp`

预计改动：

1. 增加 `#include "botpositioncontrol.h"`；
2. 增加 `HandleNpcBotCommandMassCommand()`；
3. 增加 `HandleNpcBotCommandUnmassCommand()`；
4. 增加 `HandleNpcBotCommandSpreadCommand()`；
5. 在 `npcbotCommandCommandTable` 添加三条注册项；
6. 使用 `RBAC_PERM_COMMAND_NPCBOT_COMMAND_MISC`。

不修改已有 `standstill`、`stopfully` 和 `follow` 的行为。

### 8.3 `bot_ai.cpp`

预计改动点：

1. 增加 `#include "botpositioncontrol.h"`；
2. `_calculatePos()` 开头尝试取得集合位置；
3. `GetInPosition()` 中在敌方移动分支周围增加集合抑制判断，不在函数最前面无条件返回；
4. 主更新循环中在集合参与者无法攻击目标时显式维持主人跟随；
5. `CalculateAttackPos()` 的候选点选择增加分散次级评分。

不修改 `botcommon.h`，不增加新的 `BOT_COMMAND_*` 位。

### 8.4 `botmgr.h/.cpp`

如果采用本设计推荐的 `BotMgr` 独占控制器，预计只需：

1. `botmgr.h` 增加 `class BotPositionControl;` 前置声明；
2. 增加 `GetBotPositionControl()` 访问器；
3. 增加 `std::unique_ptr<BotPositionControl> _positionControl;` 成员；
4. `botmgr.cpp` 构造函数初始化控制器。

由于 `BotMgr` 销毁时会自动销毁控制器，主人状态不需要全局清理函数。`RemoveBot()` 的每个实际 `_bots.erase()` 路径前调用：

```cpp
_positionControl->ForgetBot(guid);
```

如果控制器查询发现槽位对应 Bot 已不在 `GetBotMap()`，也应惰性删除。

### 8.5 不需要修改的文件

第一版不需要修改：

- `botcommon.h`；
- `RBAC.h`；
- SQL 文件；
- 数据库结构；
- 脚本加载器；
- `src/server/game/CMakeLists.txt`。

CMake 已经递归收集 `src/server/game` 下的源文件。
---

## 9. 生命周期和异常边界

### 9.1 主人登出

`BotPositionControl` 由 `BotMgr` 独占持有。主人登出并销毁 `BotMgr` 时，模式和槽位随控制器一起释放，不访问已失效的主人裸指针。

### 9.2 Bot 移除

现有 `BotMgr::RemoveBot()` 有延迟删除、召唤 Bot 早退和普通删除路径。每个从 `_bots` 移除的路径都必须调用 `ForgetBot(ownerGuid, botGuid)`。如果控制器查询发现槽位对应 Bot 已不在 `GetBotMap()`，也应惰性删除。

### 9.3 地图、相位和载具

以下情况不强行应用集合点：

- Bot 和主人不在同一地图；
- 相位不同；
- Bot 进入载具且乘客位置由载具控制；
- Bot 正在传送；
- Bot 死亡、被控制或跳跃坠落。

这些情况返回 `false`，继续使用现有流程。

### 9.4 与原有命令交互

| 原有命令 | 集合/分散控制处理 |
|---|---|
| `standstill` | Bot 成为守卫，控制器跳过，不清除 `STAY` |
| `stopfully` | Bot 停止活动，控制器跳过，不清除 `FULLSTOP` |
| `follow` | 原有命令恢复战斗跟随；集合模式仍可对符合条件的 Bot 生效 |
| `follow only` | `INACTION` Bot 不被集合/分散逻辑接管 |
| `unmass` | 只清除集合，不改变 `STAY/FULLSTOP/INACTION` |
| `spread off` | 只清除分散，不改变集合 |

---

## 10. 测试方案

### 10.1 命令测试

1. 无 Bot 执行 `mass`、`spread`，确认返回用法和错误；
2. 执行 `mass`，确认默认等价 `mass all`；
3. 执行 `mass ranged`，确认近战 DPS 不参与，远程和治疗参与；
4. 执行 `mass 0`、`mass 5`、`spread 1`、`spread 21`，确认参数被拒绝；
5. 重复执行 `mass`，确认不会反复重排槽位；
6. `unmass` 后确认普通职责槽位恢复；
7. `spread off` 不影响集合模式。

### 10.2 角色筛选测试

覆盖：

- 主坦克；
- 副坦克但当前未坦克；
- 当前通过 `IsTank()` 承担威胁的副坦克；
- 远程 DPS；
- 治疗；
- 近战 DPS；
- `BOT_COMMAND_STAY`；
- `BOT_COMMAND_FULLSTOP`；
- `BOT_COMMAND_INACTION`；
- 临时 Bot 和游荡 Bot。

### 10.3 集合战斗测试

1. 主人在城内移动，集合 Bot 保持 4 码内且不抖动；
2. 主人移动时 Bot 正在施法，确认不会被集合刷新打断；
3. Bot 能从当前集合点攻击目标时，正常攻击；
4. Bot 不能攻击目标时，不追到目标身边；
5. 目标离开后，Bot 继续集合跟随；
6. 坦克和 `STAY` Bot 不被集合点覆盖；
7. Bot 进入 AoE 时，AoE 规避优先于集合约束；
8. `unmass` 后远程、近战和治疗恢复原有站位。

### 10.4 分散测试

1. 默认 `spread` 距离为 5 码；
2. 设置 7 码后，候选位置尽量拉开；
3. AoE 安全点优先于分散距离；
4. 正面规避优先于分散距离；
5. 目标距离不足时，仍保持可攻击/可施法；
6. 坦克站位不被分散强行改变；
7. 关闭分散后恢复原有候选点选择；
8. 5 到 10 个 Bot 连续战斗时不出现明显位置抖动。

### 10.5 生命周期测试

1. 集合启用后解散 Bot；
2. 集合启用后主人登出；
3. 集合启用后 Bot 传送、副本切换和重新绑定；
4. Bot 延迟删除期间执行 `unmass`；
5. Bot 重新加入后重新分配槽位；
6. 多主人各自启用不同模式，确保状态不串号。

本阶段只输出设计，不执行构建和测试。实现阶段应先运行针对新增控制器的单元测试或最小编译检查，再进行完整构建。

---

## 11. 分阶段实施建议

### 阶段一：集合模式

只实现：

- 新控制器；
- `mass`、`unmass`；
- `_calculatePos()` 集合点替换；
- 主更新循环中战斗集合跟随条件；
- `GetInPosition()` 抑制敌方追击；
- 生命周期清理。

先不加入 `spread`，降低调试变量。

### 阶段二：分散模式

实现：

- 候选点结构化；
- 分散惩罚；
- AoE/LOS/攻击距离硬过滤保持不变；
- `spread` 命令和参数校验。

### 阶段三：专项兼容

针对载具、ICC 等副本机制、飞行单位、跨主人 Bot 和临时 Bot 增加专项测试。只有确认不破坏现有战斗脚本后，才考虑把更多位置策略纳入控制器。

---

## 12. 最终推荐

推荐采用以下实现边界：

```text
新文件承载状态、筛选、槽位、集合抑制和分散评分
        ↓
BotMgr 独占控制器，隔离主人状态和生命周期
        ↓
botcommands.cpp 只负责命令解析
bot_ai.cpp 增加 4 类轻量调用
botmgr.h/.cpp 增加成员、访问器和 Bot 移除清理
CMake、SQL、RBAC 和 botcommon.h 不改
```

核心原则是：

1. 集合是对普通跟随锚点的替换，不是新的战斗命令状态；
2. `BotMgr` 持有控制器，状态按主人隔离并随生命周期释放；
3. “不能攻击就不追”必须跳过敌方移动，但不能跳过正常战斗逻辑；
4. 战斗中必须显式维持主人集合跟随，不能只抑制追击后停在原地；
5. 分散只能是软约束，AoE、LOS、正面、坦克和副本机制永远优先；
6. 槽位必须稳定缓存，不能每帧随机；
7. 取消命令后不恢复旧快照，而是直接重新进入现有默认策略，避免污染原有 Bot 状态。
