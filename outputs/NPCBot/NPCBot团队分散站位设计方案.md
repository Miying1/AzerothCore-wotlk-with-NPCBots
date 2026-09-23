# NPCBot 团队级分散站位方案（已落地：团队 BOT 遍历）

> **方案状态**：**已落地实施**，采用 **团队 BOT 遍历**（详见 §4）。
> **邻居口径**：只把**团队内已入组的 BOT**作为分散参考，未入组的 BOT 一律不参与；组不存在时降级为本 BotMgr 的 BOT（详见 §5.3、§7）。入组是**逐 BOT 的显式操作**（新雇 BOT 不会自动入组），详见 §3.5.4。
> **接口口径**：对外只有 `CollectSpreadNeighbors`（收集）+ `GetSpreadPenaltyFromNeighbors`（算术）两个公开入口，由调用方控制邻居快照的复用粒度；原单点查询入口 `GetSpreadPenalty` 已删除（详见 §5.1、§5.2）。
> **性能口径**：两个调用点都做「收集一次、循环内复用」，圈内判定改用平方距离免开方（详见 §5.5、§6.5）。
> **适用范围**：仅「分散（spread）」模式，不涉及「集合（mass）」模式。
> **本文性质**：设计文档 + 实施对照，§1~§9 与 §12 已按**最终落地实现**同步。
> **文档结构**：§1~§9 与 §12 为**本次实施**内容；§11 为**后续可选**的容量增强方案（不在本次范围）。

---

## 1. 问题背景

玩家通过 `.npcbot command spread [2-20|off]` 开启战斗分散模式后，BOT 会在攻击目标周围寻找一个彼此间距不小于给定距离的落点，避免全体贴在一起吃 AOE。

改造前的分散惩罚函数只遍历**发起者自己**的 `BotMgr`（下列代码为改造前实现）：

```cpp
float BotPositionControl::GetSpreadPenalty(Creature const& bot, Position const& candidate) const
{
    if (!IsSpreadEnabled() || !bot.IsInCombat())
        return 0.0f;

    float penalty = 0.0f;
    for (auto const& [_, other] : *_botMgr.GetBotMap())
    {
        ...
    }
    ...
}
```

而 `_botMgr.GetBotMap()` 只包含该玩家名下的 BOT（`botmgr.h` 中 `BotMap _bots`，每个 `Player` 一个 `BotMgr`）。团队中其他玩家的 BOT 属于各自的 `BotMgr`，永远不会进入这个循环。

### 现状后果

| 场景 | 现状行为 |
| --- | --- |
| 单人带 N 个 BOT | 分散正常，BOT 之间保持设定间距 |
| 组队（A、B 各带 4 个 BOT） | A 的 4 个 BOT 内部分散、B 的 4 个 BOT 内部分散，**两组之间完全可能重叠站位** |
| 25 人团队 | 全团 BOT 形成 25 个互不相干的独立分散圈，整体看起来仍然扎堆 |

本质问题：**分散惩罚的作用域是「玩家私有」的，而不是「团队」的。**

---

## 2. 目标与范围

### 2.1 目标

- 玩家开启 `spread` 后，其 BOT 的分散计算把**同一团队内已入组的 BOT**（含其他玩家携带的 BOT）一并纳入间距约束；未入组的 BOT 不作为规避目标。
- 单人 / 无团队场景行为与现状**完全一致**。
- 不引入新线程、不引入全局调度器、不改变 `spread` 命令的语义与参数。

### 2.2 非目标

- 不修改 `mass`（集合站位）机制。`mass` 的角色过滤与槽位分配是另一套逻辑（`IsMassEligible`、`GetOrCreateMassSlot`），本次不动。
- 不处理「野外的非队友 BOT」与「敌对玩家 BOT」。它们不属于团队，语义上不应被本方站位逻辑规避。
- 不把玩家本体（非 BOT）纳入规避目标。
- 不实现团队级集中调度（详见 §2.3）。

### 2.3 计算模型（本方案确定的口径）

本方案是**分布式独立贪心**，不是团队全局求解。这一点必须在实施前明确：

| 维度 | 本方案的口径 |
| --- | --- |
| 计算主体 | 每个 BOT **各自独立**计算，共同步数等于团队内已入组 BOT 总数，没有唯一协调者 |
| 触发方 | 每个 BOT 由自己的 `bot_ai::CalculateAttackPos` 触发，调用的是**自己主人**的 `BotPositionControl` |
| 决策结果 | 只改写自己这一个 BOT 的落点，不触碰其他单位 |
| 团队 BOT 遍历的作用 | **仅用于读取**组内其他 BOT 的当前位置作为惩罚项，不产生任何协调动作 |
| 本质模型 | 迭代松弛 / 坐标下降式贪心；观察其他 BOT 的上一轮位置，独立取局部最优 |

由该模型带来的固有性质（属于设计取舍，非缺陷）：

- 不保证全局最优，结果与团队内的执行顺序相关；
- 窄空间下可能出现互相推让的僵局或小幅振荡（缓解手段见 §10）；
- 阈值不共享：A 用自己的 `_spreadDistance`，B 用自己的（见 §5.6）。

> 若将来要改为「团队全局统一分配落点」，需要引入协调者 + 全局位置快照 + 串行化令牌，属于另一次设计，不在本方案内。

---

## 3. 现状链路梳理

### 3.1 分散模式的开关

`BotPositionControl` 中与开关相关的成员：

| 方法 | 位置 | 说明 |
| --- | --- | --- |
| `EnableSpread(float)` | `botpositioncontrol.cpp:302` | 校验 2~20 区间，内部先 `DisableMass()`，再设置 `_spreadDistance` |
| `DisableSpread()` | `botpositioncontrol.h:47` | `_spreadDistance = 0.0f` |
| `IsSpreadEnabled()` | `botpositioncontrol.h:48` | `_spreadDistance > 0.0f` |

`_spreadDistance` 是 **per-BotMgr** 的，即每个玩家可以有自己的分散距离（A 设 5，B 设 8）。

命令入口在 `botcommands.cpp:4826 HandleNpcBotCommandSpreadCommand`，取的是 `owner->GetBotMgr()->GetBotPositionControl()`，即只操作自己的控制器。

### 3.2 惩罚函数（改造前形态）

改造前只有一个单点查询入口 `GetSpreadPenalty`（落地后已删除，最终形态见 §5.1、§5.4）：

```cpp
float BotPositionControl::GetSpreadPenalty(Creature const& bot, Position const& candidate) const
```

惩罚构成：

1. **间距惩罚**：对每个邻居 `other`，若 `dist(other, candidate) < _spreadDistance`，累加 `deficit²`（`deficit = _spreadDistance - distance`）。平方形式使越近的邻居权重越大。
2. **位移惩罚**：`bot` 当前位置到候选点的距离 × `0.05`，轻微抑制「为了分散跑太远」。

### 3.3 两个调用点

**调用点 A：攻击位优化**

```377:419:src/server/game/AI/NpcBots/botpositioncontrol.cpp
bool BotPositionControl::TryImproveSpreadPosition(Creature const& bot, bot_ai const& ai, Unit const& target,
    float maxOwnerDistance, float attackDistance, Position& position) const
```

逻辑：以当前落点为基准，围绕目标在 ±(1~3)×15° 的 6 个候选角度上试探，取惩罚最低者（要求至少改善 `0.25f`，抑制抖动）。其中坦克被排除（`ai.HasRole(BOT_ROLE_TANK) || ai.IsTank()`）。

唯一调用点：

```6018:6023:src/server/game/AI/NpcBots/bot_ai.cpp
    pos.Relocate(ppos);
    if (!IAmFree())
    {
        float maxOwnerDistance = followdist > collision_dist_max ? float(collision_dist_max) : followdist < 20 ? 20.0f : float(followdist);
        master->GetBotMgr()->GetBotPositionControl()->TryImproveSpreadPosition(*me, *this, *target, maxOwnerDistance, dist, pos);
    }
```

即 `bot_ai::CalculateAttackPos`（攻击位计算）中调用一次，内部会对 7 个位置点各算一次惩罚。

**调用点 B：AOE 安全点选择**（改造前）

```5936:5946:src/server/game/AI/NpcBots/bot_ai.cpp
        for (Position const& safepos : safespots)
        {
            float currentDistance = me->GetExactDist2d(safepos);
            float spreadPenalty = positionControl ? positionControl->GetSpreadPenalty(*me, safepos) : 0.0f;
            ...
        }
```

改造前对每个候选安全点调用一次 `GetSpreadPenalty`，也就是**每个候选点都重新收集一次邻居**。此处 `positionControl` 为 `IAmFree() ? nullptr : ...`，即自由 BOT 不参与。

**两条路径互斥**：`CalculateAttackPos` 在 `safespots` 非空时直接 `return`（`bot_ai.cpp:5963`），因此单次调用只会走其中一条——存在 AOE 危险区时走调用点 B，没有 AOE 才会走到末尾的调用点 A。

> **对原「关键结论」的修正**：原方案认为「只要把 `GetSpreadPenalty` 的邻居集合改成团队 BOT，两个调用点无需改动即可全部生效」。邻居来源部分成立，但**调用点 B 最终仍需改动**：它的候选点最多 200 个，沿用「每候选点收集一次」会把组内 BOT 遍历重复 200 次。因此接口被拆成「收集 + 算术」两个公开入口（§5.1），调用点 B 改为「收集一次 + 循环内复用」（§5.5）。

### 3.4 团队 BOT 遍历能力：接口与既有先例

| 需求 | 现有接口 |
| --- | --- |
| 取玩家团队 | `Group const* Player::GetGroup() const`（`Player.h:2540`） |
| 遍历组内 BOT | `GroupBotReference const* Group::GetFirstBotMember() const`（`Group.h:213`），`ref->next()` / `ref->GetSource()` |
| （降级用）遍历成员 | `GroupReference const* Group::GetFirstMember() const`（`Group.h:263`）、`BotMgr* Player::GetBotMgr() const`（`Player.h:2723`）、`BotMap const* BotMgr::GetBotMap() const`（`botmgr.h:121`） |

代码库中已有大量「遍历团队成员」的既有范式（如 `LootMgr.cpp:571`、`QuestHandler.cpp:540`），写法统一：

```cpp
for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
{
    Player* member = itr->GetSource();
    if (!member)
        continue;
    ...
}
```

#### 3.4.1 更强依据：NPCBot 自身已在直接遍历组内 BOT

**「遍历 `Group::GetFirstBotMember()`」在本项目中是既有成熟范式**，且多数就写作 `GroupBotReference const*` + `ref->GetSource()` 的 const 形式：

```2208:2212:src/server/game/AI/NpcBots/botmgr.cpp
        for (GroupBotReference const* ref = group->GetFirstBotMember(); ref != nullptr; ref = ref->next())
        {
            if (Creature* cr = ref->GetSource())
                group_members.push_back(cr);
        }
```

```1949:1954:src/server/game/AI/NpcBots/botdatamgr.cpp
    for (GroupReference const* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        if (Player const* gplayer = itr->GetSource())
            collect_existing_roles(gplayer->GetGUID());
    for (GroupBotReference const* itr = group->GetFirstBotMember(); itr != nullptr; itr = itr->next())
        if (Creature const* gbot = itr->GetSource())
            collect_existing_roles(gbot->GetGUID());
```

同族用法还有 `Unit.cpp:8713`（雷达/AOE 判定取组内 BOT）、`BattlegroundQueue.cpp:196`（组队队列带 BOT）、`LFGScripts.cpp:136`（名单查询）、`Group.cpp:2565`（成员计数）。

这段既有代码对本方案的意义：

1. **架构可行性已被验证**：直接遍历组内 BOT 在本项目中是**已被接受的成熟做法**，不是本方案引入的新风险。`CollectSpreadNeighbors` 只是把同样的遍历用于读取坐标。
2. **`const` 写法模板现成**：`GroupBotReference const*` + `GetSource()` 的完整 const 写法在 `botmgr.cpp` / `botdatamgr.cpp` 中已固定成型，可直接照抄，避免 `const` 重载选错。
3. **语义天然匹配**：`GetFirstBotMember()` 只返回**已入组**的 BOT，正好是本方案要的口径（§5.3），无需再对「是否入组」做二次过滤。
4. **调用路径与频次无关**：它本身就位于 `CalculateAttackPos` 附近的既有路径上，本方案在该路径上新增一次组内 BOT 遍历**不会改变任何调用频次**（频率依据见 §6.1）。

#### 3.4.2 相邻但未采用的写法：团队成员 → 各自 `BotMgr`

同一条执行路径上还存在另一种团队遍历写法——`bot_ai::CalculateAttackPos` 内为「被集火的远程 BOT 寻找坦克掩护目标」时的实现（`bot_ai.cpp:5975~5991`）：

```5975:5991:src/server/game/AI/NpcBots/bot_ai.cpp
        if (Group const* gr = master->GetGroup())
        {
            for (GroupReference const* itr = gr->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player const* pl = itr->GetSource();
                if (!pl || !pl->IsInMap(me) || pl->GetDistance(me) > VISIBILITY_DISTANCE_NORMAL)
                    continue;
                if (pl->IsAlive() && !pl->HasUnitState(UNIT_STATE_ISOLATED) && IsTank(pl))
                    safetyTargets.push_back(pl);
                if (!pl->HaveBot())
                    continue;
                for (auto const& [_, c] : *pl->GetBotMgr()->GetBotMap())
                {
                    if (c && c->IsInWorld() && me->GetMap() == c->FindMap() && c->IsAlive() && !c->HasUnitState(UNIT_STATE_ISOLATED) && IsTank(c) && c->GetBotAI()->HasRole(BOT_ROLE_DPS))
                        safetyTargets.push_back(c);
                }
            }
        }
```

该写法能拿到玩家**全部** BOT（含未入组的），但**本方案不采用**，因为需求限定为「只考虑团队中的 BOT」；排除理由见 §3.5.3，备选记录见 §4.2。

> 结论：本方案在「遍历团队 + 读取 BOT」这一层**没有任何新机制**，纯粹是把既有遍历范式复制到分散惩罚计算中。

### 3.5 Group 的成员结构：小队与团队的口径

#### 3.5.1 小队与团队不作区分

`Player::GetGroup()` 返回统一的 `Group*`，小队与团队是**同一个类**，只靠 `GroupType` 标记区分：

```85:96:src/server/game/Groups/Group.h
enum GroupType
{
    GROUPTYPE_NORMAL         = 0x00,
    GROUPTYPE_BG             = 0x01,
    GROUPTYPE_RAID           = 0x02,
    GROUPTYPE_BGRAID         = GROUPTYPE_BG | GROUPTYPE_RAID, // mask
    ...
};
```

```44:46:src/server/game/Groups/Group.h
#define MAXGROUPSIZE 5
#define MAXRAIDSIZE 40
#define MAX_RAID_SUBGROUPS MAXRAIDSIZE/MAXGROUPSIZE
```

```2815:2818:src/server/game/Groups/Group.cpp
bool Group::IsFull() const
{
    return isRaidGroup() ? (m_memberSlots.size() >= MAXRAIDSIZE) : (m_memberSlots.size() >= MAXGROUPSIZE);
}
```

**本方案不做小队/团队区分**：`GetFirstBotMember()` 对两者通用，小队只是成员更少（≤5），同一份 `CollectSpreadNeighbors` 自动覆盖两种形态，无需读 `isRaidGroup()`。另外小队满员时 `BotMgr::AddBotToGroup` 会自动 `ConvertToRaid()`（§3.5.3），因此「单人多 BOT」不会卡在小队的 5 人上限上。

#### 3.5.2 关键事实：Group 有两条彼此独立的成员链表

NPCBots 对 `Group` 做了扩展，成员分为**玩家链表**与 **BOT 链表**：

```367:372:src/server/game/Groups/Group.h
    MemberSlotList      m_memberSlots;
    GroupRefMgr     m_memberMgr;
    //npcbot
    GroupBotRefManager  m_botMemberMgr;
    //end npcbot
```

| 接口 | 底层类型 | 返回内容 |
| --- | --- | --- |
| `GetFirstMember()` | `GroupReference : Reference<Group, **Player**>`（`GroupReference.h:26`） | **只含玩家** |
| `GetFirstBotMember()` | `GroupBotReference : Reference<Group, **Creature**>`（`GroupReference.h:45`），底层容器 `RefMgr<Group, Creature>`（`GroupRefMgr.h:38`） | **只含已入组的 BOT** |
| `m_memberSlots` | 槽位列表 | 玩家 + BOT **混装**，供 `IsMember` / `GetMembersCount` / `IsFull` / subgroup 计数使用 |

BOT 是通过 `SetBotGroup` → `LinkBotMember` 挂到 `m_botMemberMgr` 上的：

```548:549:src/server/game/Groups/Group.cpp
    else //if player is not in group, then call set group
        creature->SetBotGroup(this, subGroup);
```

```3047:3051:src/server/game/Groups/Group.cpp
//npcbot
void Group::LinkBotMember(GroupBotReference* bRef)
{
    m_botMemberMgr.insertFirst(bRef);
}
```

同时 `Group::AddMember(Creature*)` 会把 BOT 一并塞进 `m_memberSlots`（这正是 BOT 占用团队人数的原因）：

```533:539:src/server/game/Groups/Group.cpp
    MemberSlot member;
    member.guid      = creature->GetGUID();
    member.name      = creature->GetName();
    member.group     = subGroup;
    member.flags     = 0;
    member.roles     = 0;
    m_memberSlots.push_back(member);
```

#### 3.5.3 由该结构得出的两点结论

**结论一：两条链表必须分清、不能互相替代。** 遍历 `GetFirstMember()` 得到的 `GroupReference::GetSource()` 类型上就是 `Player*`，BOT 在另一条链表上，不存在「把 BOT 当玩家去取 `BotMgr`」的类型混淆风险；但反过来，玩家链表也**不包含** BOT，取组内 BOT 必须走 `GetFirstBotMember()`。

**结论二：邻居来源取 `GetFirstBotMember()`（组内已入组的 BOT）。** 这是「只考虑团队中的 BOT」的直接实现，语义边界清晰：

1. 它只覆盖**已加入团队**的 BOT；入组是**逐 BOT 的显式操作**，且 `BOT_ROLE_PARTY` **不是** BOT 的默认角色（`BotDataMgr::DefaultRolesForClass`，`botdatamgr.cpp:3645-3666`，只给出 `DPS/RANGED`），入组的完整机制见 §3.5.4；
2. BOT 与玩家**混装**在同一条 `m_memberSlots` 里，受 `MAXRAIDSIZE = 40` 约束，因此「团队内已入组 BOT 数」的上界是 `40 − 玩家数`；
3. 普通小队（`MAXGROUPSIZE = 5`）满员时会自动 `ConvertToRaid()`（`botmgr.cpp:1015-1021`），所以「单人多 BOT」不会因 5 人上限而漏 BOT。

**已知代价（本次规则明确接受）**：

- 从未入组的 BOT（从未点过 bot gossip 的「加入队伍」，因而未携带 `BOT_ROLE_PARTY`，见 §3.5.4），以及 40 个槽位被占满后无法入组的 BOT，**不再被任何人规避**，也不再规避别人；
- 极端情况下（40 人满编纯玩家团）组内没有 BOT，间距惩罚项为空，`spread` 只剩位移项 —— 相当于失效。数值影响见 §6.2，边界见 §7，风险见 §10。

#### 3.5.4 入组是逐 BOT 的显式操作（决定邻居集合的实际大小）

**关键事实：`BOT_ROLE_PARTY` 不是 BOT 的默认角色。** `BotDataMgr::DefaultRolesForClass`（`botdatamgr.cpp:3645-3666`）只给出 `BOT_ROLE_DPS` / `BOT_ROLE_RANGED`；`botdatamgr.cpp:758`（`broles_mask = first_role | BOT_ROLE_PARTY | BOT_ROLE_DPS;`）只属于**副本自动补 BOT** 这一条生成路径，不适用于普通雇佣的 BOT。雇佣 BOT 的角色取自 `_botData->roles`（`bot_ai.cpp:15693` 的 `InitRoles`），默认同样不含该角色。

该角色唯一的授予点是 `BotMgr::AddBotToGroup` 自身 —— 它对每只成功入组的 BOT 强制补角色并落库：

```1034:1039:src/server/game/AI/NpcBots/botmgr.cpp
    if (gr->AddMember(bot))
    {
        if (!bot->GetBotAI()->HasRole(BOT_ROLE_PARTY))
            bot->GetBotAI()->ToggleRole(BOT_ROLE_PARTY, true);

        return true;
    }
```

`ToggleRole` 会把角色写回 `characters_npcbot.roles`（`bot_ai.cpp:15419`），因此「入组」是**逐 BOT 的显式操作**，且一旦入过组会被持久记录。触发入组的入口只有两处：

| 入口 | 位置 | 说明 |
| --- | --- | --- |
| bot gossip「创建队伍 / 加入队伍 / 加入队伍（全部 BOT）」 | 菜单 `bot_ai.cpp:8373-8385` → 处理 `bot_ai.cpp:11146-11163` | 玩家主动操作；「全部 BOT」选项会遍历 `BotMgr::GetBotMap()` 逐只调用 `AddBotToGroup`，因而**每只都会拿到该角色** |
| `CheckAuras` 自动补组 | `bot_ai.cpp:19152-19157` | 仅对**已带** `BOT_ROLE_PARTY` 的 BOT 生效（条件为已有该角色且不在组内），不会给缺角色的 BOT 补角色 |

由此得出三点，直接决定 `GetFirstBotMember()` 的实际返回：

1. **全新雇佣的 BOT 不会自动入组**：`BotMgr::AddBot` 只在 BOT 已有 `BOT_ROLE_PARTY` 时才入组（`botmgr.cpp:998-999`），而雇佣 BOT 默认不含该角色 ⇒ 既不会自动建组、也不会自动入组；之后 `CheckAuras` 的自动补组前提不满足，也不会替它补上（除非玩家再点一次 bot gossip）。
2. **点过「加入队伍（全部 BOT）」后，参与的每只 BOT 都带上了该角色**，之后登录/重进游戏都会被 `CheckAuras` 自动补回组内 ⇒ 此时「有 Group」且组内就是自己的全部 BOT，邻居与旧实现等价；若玩家**没有** Group，则走降级分支（§5.3），同样与旧实现等价。
3. **「移出队伍」不会摘掉该角色**（`RemoveBotFromGroup` 反而会重赋，`botmgr.cpp:1064-1065`），下一 tick 就被自动加回 ⇒ 「部分入组」只能由「从未入组」造成，不能靠手动移除维持。

**必然推论（本次口径接受）**：玩家处于某个 `Group` 中、但自家 BOT 从未入组时，`owner->GetGroup()` 非空 ⇒ 走组分支 ⇒ 邻居集合为空 ⇒ 间距项为空，**连自家 BOT 之间的分散也失效**（详见 §7、§8、§10）。触发路径：只点了「创建队伍（单个 BOT）」而没再用「全部 BOT」；或入组之后才新雇 BOT。恢复方式：对任意一只 BOT 点一次「加入队伍 / 加入队伍（全部 BOT）」。

**对文档的修正**：旧稿以「单人玩家通常已经存在一个 `Group`」论证「单人场景行为不变」，依据不成立（见上第 1 点）。正确依据是：**无 `Group` 时走显式降级分支**（`owner->GetGroup()` 为空 ⇒ 邻居回退为本 `BotMgr` 的 BOT，§5.3）；有 `Group` 且自家 BOT 已入组时，`GetFirstBotMember()` 拿到的就是自己的 BOT。两种情况都与旧实现一致。

---

## 4. 方案确定：团队 BOT 遍历

### 4.1 确定结论

采用 **团队 BOT 遍历**：以 `owner->GetGroup()` 上 `GetFirstBotMember()` 给出的**已入组 BOT** 作为分散参考集合；无团队时降级为本 `BotMgr` 的 BOT。

判定依据：

1. 需求限定「只考虑团队中的 BOT，不考虑不在组的 BOT」，`GetFirstBotMember()` 与该口径一一对应，语义无歧义；
2. 单人 / 无团队时行为零变化（无组走降级分支；有组且自家 BOT 已入组时，邻居与之等价，见 §3.5.4），回归风险最低；
3. 邻居来源的替换收敛在 `BotPositionControl` 内部；为消除 safespot 分支的重复收集，最终把「收集」与「算术」拆成两个公开入口（§5.1），调用点改为快照复用（§5.5）；
4. 邻居规模上界明确（组内 BOT 数 ≤ 39，见 §6.2），性能可估算、可控（§6）；
5. 项目内已有大量同类既有先例（§3.4.1），无新增架构风险；
6. 所需依赖（`Group.h`）在本项目既有代码路径中已经存在，不构成新增依赖负担；
7. 只需遍历一条 BOT 链表，比「遍历玩家成员再逐个访问 `BotMgr`」更省，也不需要 `IsMember` 之类的二次判定。

### 4.2 已排除的备选（仅作决策记录）

| 备选 | 邻居来源 | 排除原因 |
| --- | --- | --- |
| 团队成员 → 各自 `BotMgr` 的全部 BOT | 遍历 `GetFirstMember()`，逐个访问 `GetBotMgr()->GetBotMap()` | 会把**未入组**的 BOT 也当作规避目标，超出「只考虑团队中的 BOT」的口径；且需跨 BotMgr 遍历（写法见 §3.4.2） |
| 同地图全部 BOT | 遍历 `Map` 上的 creature 并判定 `bot_ai` | 会把路人 / 敌对 / 无关 BOT 当作规避目标，语义错误；且需全图遍历，成本高 |
| 全局 BOT 表 + 距离过滤 | `BotDataMgr` 全局表 | 不依赖团队关系，语义过宽；需跨控制器访问全局容器，锁与生命周期风险更大 |

> 三者都被否掉的核心原因是**语义**，而非实现难度：分散站位应当只规避「团队里已入组的自己人」。

---

## 5. 详细设计

### 5.1 对外接口（最终形态）

对外只保留两个公开入口（`botpositioncontrol.h:56`、`:58`）：

```cpp
// 收集分散参考单位：团队中已入组的 BOT（不含未入组的 BOT）；无团队时退化为本 BotMgr 的 BOT
void CollectSpreadNeighbors(Creature const& bot, std::vector<Creature const*>& neighbors) const;

// 基于预收集的邻居快照计算分散惩罚（不校验是否启用分散，由调用方按需保证）
float GetSpreadPenaltyFromNeighbors(Creature const& bot,
    std::vector<Creature const*> const& neighbors, Position const& candidate) const;
```

改造前的单点查询入口 `GetSpreadPenalty` **已删除**。它内部就是「收集 + 算术」的组合，而两个调用点都需要在多个候选点之间复用同一份邻居集合（§5.5），保留它只会留下一个树内无人调用、且诱导「每个候选点重新收集一次」的入口。删除后：

- **短路校验上移到调用方**：原 `GetSpreadPenalty` 首行的 `!IsSpreadEnabled() || !bot.IsInCombat()` 改由调用方承担（`bot_ai.cpp:5938` 的 `useSpread`；`TryImproveSpreadPosition` 的前置校验），语义与删除前逐字一致；
- **候选点评估的粒度由调用方决定**：单点评估就收集一次算一次，批量评估就收集一次算 N 次。

### 5.2 拆分与可见性（最终接口签名）

两个方法都由 private 改为放在 **public** 区（`botpositioncontrol.h:56`、`:58`，签名同 §5.1）。改为公开的直接原因：safespot 分支位于 `bot_ai.cpp`，不公开就无法「收集一次、循环内复用」（§5.5）。

拆分理由：

- `CollectSpreadNeighbors` 负责「集合范围」这一**策略**。将来若要扩展为「含玩家本体」「含未入组的 BOT」或「含自由 BOT」，只改这里；
- `GetSpreadPenaltyFromNeighbors` 只负责**算术**，与原有公式逐字一致（另含 §5.4 的平方距离优化），便于对照验证。

### 5.3 邻居收集逻辑

```
CollectSpreadNeighbors(bot, neighbors):
    neighbors.clear()
    owner = _botMgr.GetOwner()
    if !owner: return

    // 局部闭包：把合格的 BOT 追加进 neighbors
    collectBot(other):
        other 为空              -> 跳过
        other == &bot           -> 跳过（排除自己）
        !other.IsInWorld()      -> 跳过
        !other.IsAlive()        -> 跳过
        !bot.IsInMap(other)     -> 跳过（跨地图）
        !bot.InSamePhase(other) -> 跳过（相位不一致）
        neighbors.push_back(other)

    group = owner.GetGroup()
    if group:
        for ref in group.GetFirstBotMember() .. ref.next():
            collectBot(ref.GetSource())    // 只取已入组的 BOT
        return

    // 无团队：降级为本 BotMgr 的 BOT，保持单人 / 无团队时与旧实现一致
    for other in _botMgr.GetBotMap():
        collectBot(other)
```

要点：

- **过滤条件与原实现完全一致**，只把「遍历单个 `BotMap`」换成「遍历一条组内 BOT 链表」，不引入新的语义；
- **只取已入组的 BOT**：`GetFirstBotMember()` 天然不含玩家、也不含未入组的 BOT（包括本 `BotMgr` 中未入组的），这正是本次口径；注意入组是逐 BOT 的显式操作（§3.5.4），因此「有 `Group`、但组内没有自家 BOT」时本分支会得到**空集**（间距项失效，属口径推论）；
- 无团队时**必须显式降级**到本 `BotMgr`，而不是返回空集：否则「确实无 `Group`」的玩家 `spread` 会完全失效（依据见 §7）；
- 使用 `Group::GetFirstBotMember()` 的 `const` 重载（返回 `GroupBotReference const*`），与 `Player const* owner` 匹配，写法照抄 §3.4.1 的既有代码；
- 不需要 `IsMember` 二次判定，也不需要访问玩家成员的 `BotMgr`（那套写法见 §3.4.2 与 §4.2）。

### 5.4 惩罚计算逻辑

与原公式等价，只把数据源换成快照，并用**平方距离**提前排除圈外邻居（`botpositioncontrol.cpp:347-366`）：

```
GetSpreadPenaltyFromNeighbors(bot, neighbors, candidate):
    penalty = 0
    spreadDistanceSq = _spreadDistance * _spreadDistance
    for other in neighbors:
        distanceSq = other.GetExactDist2dSq(candidate)      // 不开方
        if distanceSq < spreadDistanceSq:
            deficit = _spreadDistance - sqrt(distanceSq)
            penalty += deficit * deficit

    penalty += bot.GetExactDist2d(candidate) * 0.05f
    return penalty
```

- **等价性**：`sqrt(distanceSq) < r` ⟺ `distanceSq < r²`，因此只有真正落在圈内的邻居才需要开方（`Position.h:170` 的 `GetExactDist2d` 内部即 `std::sqrt`）；
- **不再做短校验**：`IsSpreadEnabled()` / `IsInCombat()` 由调用方保证（§5.1），本函数是纯算术。

### 5.5 邻居快照复用（性能关键，两个调用点都做）

邻居集合的规模与「候选点数」无关，但与「组内 BOT 数」成正比。两个调用点都要评估多个候选点，因此**都**采用「收集一次、循环内复用」：调用点 A 是 7 个候选点，调用点 B 最多 200 个。

**调用点 A：`TryImproveSpreadPosition`**（`botpositioncontrol.cpp:379-380`、`:381`、`:397`）

```
TryImproveSpreadPosition(...):
    前置校验（坦克排除、owner 有效性）保持不变
    neighbors = []; CollectSpreadNeighbors(bot, neighbors)   // 本轮只收集一次
    bestPenalty = GetSpreadPenaltyFromNeighbors(bot, neighbors, position)
    for 每个候选角度/步长:
        ...
        penalty = GetSpreadPenaltyFromNeighbors(bot, neighbors, candidate)   // 复用快照
```

**调用点 B：AOE 安全点择优**（`bot_ai.cpp:5935-5947`）

```
CalculateAttackPos(...):
    positionControl = IAmFree() ? nullptr : master.GetBotMgr().GetBotPositionControl()
    useSpread = positionControl && positionControl->IsSpreadEnabled() && me->IsInCombat()
    neighbors = {}
    if useSpread: positionControl->CollectSpreadNeighbors(*me, neighbors)   // 本轮只收集一次
    for safepos in safespots:                    // 候选点最多 200 个
        spreadPenalty = useSpread ? GetSpreadPenaltyFromNeighbors(*me, neighbors, safepos) : 0.0f
```

> 调用点 B 是本方案单次成本的主要来源：候选点可达 200，旧写法会把组内 BOT 遍历重复 200 次（同时产生 200 次 `std::vector` 分配）。快照化后，每次 `CalculateAttackPos` 的收集次数与分配次数都恒为 **1**。

约束说明：

- 快照类型为 `std::vector<Creature const*>`，生命周期**严格限定在单次调用内**（`TryImproveSpreadPosition` 或 `CalculateAttackPos`），不跨 tick、不跨函数保存，符合项目「不长期持有裸指针」的约定；
- 同一轮内的所有候选点共用一份快照：邻居坐标在该轮内不会被改写，因此结果与「每候选点各自收集一次」逐点等价（收集只是读取坐标）；
- `neighbors.clear()` 由 `CollectSpreadNeighbors` 负责，调用方可直接复用同一容器；
- 调用点 B 的短路语义不变：`useSpread` 为 false 时惩罚恒为 `0.0f`，与删除 `GetSpreadPenalty` 之前的返回值一致；
- 跨线程视角的既有假设见 §7「线程安全说明」。

### 5.6 距离阈值 `_spreadDistance` 的取值规则（已确定）

**规则：统一使用发起计算的这个 `BotMgr` 的 `_spreadDistance`。**

即 A 的 BOT 在做分散计算时，参考队友 B **已入组的** BOT 位置，但间距阈值取 A 设定的值，**不读取** B 的控制器状态。

理由：

1. 语义自洽——「我要求我的 BOT 之间、以及我的 BOT 与队友 BOT 之间，都保持我设定的间距」；
2. 实现简单，不产生跨 BotMgr 的状态耦合；
3. 即使队友没开 `spread`，我的 BOT 依然会主动避开他的 BOT（前提：这些 BOT 已入组），符合「避免扎堆」的直觉诉求。

**明确不采用**的备选语义：取双方阈值中的较大值。该做法需要读取 `member->GetBotMgr()->GetBotPositionControl()->GetSpreadDistance()`，会引入「对方配置影响我方站位」的隐式耦合。

### 5.7 依赖调整

| 文件 | 调整 |
| --- | --- |
| `botpositioncontrol.h` | 增加 `#include <vector>`（`:9`）；新增 `CollectSpreadNeighbors` / `GetSpreadPenaltyFromNeighbors` 两个 **public** 声明（`:56`、`:58-60`）；删除 `GetSpreadPenalty` 声明 |
| `botpositioncontrol.cpp` | 增加 `#include "Group.h"`（`:7`）；实现两个新方法（`CollectSpreadNeighbors` `:313-345`、`GetSpreadPenaltyFromNeighbors` `:347-366`）；删除 `GetSpreadPenalty` 定义；`TryImproveSpreadPosition` 改为快照复用（`:379-380`） |
| `bot_ai.cpp` | safespot 择优分支：短路校验上移 + 收集一次邻居快照 + 循环内复用（`:5935-5947`） |

`botpositioncontrol.cpp` 已经 include 了 `Player.h`、`Map.h`、`botmgr.h`，唯一缺的是 `Group.h`（提供 `Group::GetFirstBotMember()` 与 `GroupBotRefManager`）。`GroupBotReference` 的完整定义在 `GroupReference.h`，由本文件已 include 的 `bot_ai.h`（`bot_ai.h:11`）带入，无需额外 include。`bot_ai.cpp` 本身已 include `botpositioncontrol.h`（`bot_ai.cpp:15`），调用新接口不需要新增 include。该头文件在本项目其他 AI 代码路径中已被广泛使用，不构成新增依赖负担。

**不需要改动的部分**：`botcommands.cpp` 的命令处理、`botmgr.h/.cpp`、`mass` 相关的全部代码。

---

## 6. 性能

### 6.1 触发频率（分母）

站位计算挂在 `CheckAttackState()` 中，该函数**每次 AI 更新都会调用**：

```2453:2457:src/server/game/AI/NpcBots/bot_ai.cpp
    if (HasBotCommandState(BOT_COMMAND_FULLSTOP))
        return false;

    if (!HasBotCommandState(BOT_COMMAND_INACTION))
        CheckAttackState();
```

但内部的站位重算被计时器门控：

```19140:19142:src/server/game/AI/NpcBots/bot_ai.cpp
    if (checkAurasTimer <= lastdiff)
    {
        checkAurasTimer += uint32(_rand + _rand + (IAmFree() ? 1000 : 40 * (1 + master->GetNpcBotsCount())));
```

配合 `_rand` 的取值范围：

```293:297:src/server/game/AI/NpcBots/bot_ai.cpp
void bot_ai::GenerateRand()
{
    _rand = urand(0, IAmFree() ? 100 : 100 + (master->GetNpcBotsCount() - 1) * 2);
}
```

带 8 个 BOT 时，`_rand ∈ [0, 114]`，计时器增量 ≈ `0..228 + 40×9 = 360..588`(ms)，即**每个 BOT 约每 0.4~0.6 秒**才做一次 `CalculateAttackPos`。

另外 `TryImproveSpreadPosition` 位于 `CalculateAttackPos` 的末尾兜底分支，前面若命中「当前位置已安全」或「AOE 安全点存在」会提前 return，实际到达率还低于上述频率。

> 结论：**分母不是 tick 频率**，改造只影响单次成本，不影响触发节奏。

### 6.2 单次成本与前后对比

| 环节 | 现状（改造前） | 本方案（已优化） |
| --- | --- | --- |
| 邻居来源 | 自己的 `BotMap`（B 项） | 组内已入组的 BOT（N 项） |
| 收集/过滤 | 每候选点 1 次自身 map 遍历 | **每次调用恒为 1 次**组内 BOT 遍历（两个调用点都做了快照复用，§5.5） |
| 距离运算 | 候选点数 × (B−1) | 候选点数 × (N−1) |
| 开方次数 | 候选点数 × (B−1) | 仅「落在 `_spreadDistance` 内」的邻居（§5.4 平方距离预筛，正常分散时接近 0） |

候选点数的量级（**两条路径互斥，单次调用只会走其中一条**，见 §3.3）：

| 调用路径 | 候选点数 | 进入条件 |
| --- | --- | --- |
| `TryImproveSpreadPosition` | 7（1 + 2×3） | 有目标且**不存在** AOE 危险区 |
| safespot 择优 | ≤ 200（8 半径 × 25 角度） | 存在 AOE 危险区（`aoespots` 非空） |

因此单次 `CalculateAttackPos` 的距离运算上界是 `max(7, 200) × (N−1) = 200 × (N−1)`，且不再叠加重复收集与重复分配。

记 M = 团队玩家数、B = 每人 BOT 数。组内槽位上限 `MAXRAIDSIZE = 40` 由玩家与 BOT **混装**，因此：

\[ N_{\max} = \max(0,\ 40 - M) \]

邻居数上界 = `N_max − 1`（若自己在组内）。

数值估算（每人 8 个 BOT；下表按候选点数 7 的 `TryImproveSpreadPosition` 路径折算）：

| 场景 | 邻居数（现状） | 组内 BOT 上界 | 邻居数（本方案） | 单轮距离运算（现状） | 单轮距离运算（本方案） | 倍数 |
| --- | --- | --- | --- | --- | --- | --- |
| 单人 8 BOT（M=1） | 7 | min(1×8, 40−1) = 8 | 7 | 49 | 49 | **1×（不变）** |
| 5 人团 × 8（M=5） | 7 | min(5×8, 40−5) = 35 | ≤34 | 49 | ≤238 | ≤4.9× |
| 25 人团 × 8（M=25） | 7 | min(25×8, 40−25) = 15 | ≤14 | 49 | ≤98 | ≤2× |
| 40 人团 × 8（M=40） | 7 | min(40×8, 40−40) = 0 | 0 | 49 | 0 | **功能失效** |

> **反直觉但必须注意的结论**：团队越大，可入组的 BOT 越少。40 个槽位被玩家占满后 `AddBotToGroup` 返回 `false`，这些 BOT 既不参与规避也不再被规避。因此本方案的实际开销**低于**按 `M × B` 估算的旧口径，代价是规避覆盖度下降（见 §7、§10）。

> 口径澄清：邻居集合**完全由组内 BOT 链表决定**（§5.3），与「团队成员 → `BotMgr` 全部 BOT」的备选数法不同，因此上界是 `40 − M` 而非 `M × B`。
>
> 另需注意：某一玩家的 BOT 能否入组取决于入组顺序，**某个玩家的部分 BOT 可能在组外**；此时这些 BOT 既不会被别人规避，也不去规避别人（本次口径的必然结果）。
>
> 若走 safespot 分支（候选点 ≤ 200），把上表最后一列按 `200 / 7` 折算：以 5 人团（N ≤ 34）为例约 `200 × 33 ≈ 6,600` 次平方距离运算，无重复收集、无开方。这是本方案单次成本的实际上界，且只在存在 AOE 危险区时才会发生。

### 6.3 三个决定性前提

1. **未开启 `spread` 的玩家零开销**。短路校验已上移到调用方（`bot_ai.cpp:5938` 的 `useSpread`；`TryImproveSpreadPosition` 的前置校验），条件不成立时连收集都不会发生，惩罚恒为 `0.0f`。
2. **队友不承担开销**。队友的 BOT 只是「被读取」，不触发任何额外计算；其自身计算仍按原逻辑（或按队友自己的 `spread` 设置）执行。
3. **触发频率不因邻居变多而升高**。频率只由 §6.1 的计时器决定，与邻居数无关。

### 6.4 已知的额外成本来源

- **cache locality 变差**。改造前邻居都在自己 `BotMgr` 内，本方案的邻居来自不同玩家的 BOT，`Creature` 对象在堆上分散，取坐标更易 cache miss。实际单次开销会高于纯「邻居数倍数」。
- **多团并发线性叠加**。上表是单团数字，同时开战的团数会直接相乘。
- **safespot 分支的候选点数**：收集已快照化（只做 1 次），但距离运算仍是 `S × N`（S ≤ 200）。这是本方案单次成本上界的主要来源，且只在存在 AOE 危险区时触发。
- **组内 BOT 遍历自身开销**：`GetFirstBotMember()` 是链表遍历（O(N)），无哈希查找，量级与一次距离运算同阶；快照化后每次调用只做 1 次，可忽略。

### 6.5 本方案包含 / 不包含的优化

**已包含（本次落地）**：

| 优化 | 说明 | 效果 |
| --- | --- | --- |
| 两个调用点的邻居快照复用 | `TryImproveSpreadPosition`（7 候选点）与 safespot 择优（≤200 候选点）都改为「收集一次、循环内复用」 | 每次调用的组内 BOT 遍历次数由「候选点数」降为恒为 **1**，`std::vector` 分配由「候选点数」降为 **1** |
| 平方距离预筛 | 圈内判定改用 `GetExactDist2dSq`，只有圈内邻居才 `sqrt` | 开方次数由「候选点数 × 邻居数」降为「仅圈内邻居」（正常分散时接近 0） |
| 删除 `GetSpreadPenalty` | 单点查询入口已无调用者，留着会诱导「每候选点重新收集一次」的写法 | 代码面无死入口，接口收敛为「收集 + 算术」（§5.1） |

**未做（明确不做或后续按实测决定）**：

| 未做项 | 说明 | 结论 |
| --- | --- | --- |
| 收集时按粗距离剪枝 | 要保证不漏邻居，阈值必须 ≥ `_spreadDistance + 2 × maxOwnerDistance`（≈ 70~80 码，而 AOE 危险区本身也只有 60 码） | 剪不掉多少，收益近零，不做 |
| safespot 循环「两遍化」（先按距离选，再算惩罚） | 1 码容差是**流式**比较（`minPosDistance` 随循环单调收紧，结果依赖遍历顺序），两遍化会改变比较顺序 | 会引入行为漂移，明确不做 |
| 每 tick 级缓存团队 BOT 列表 | 只能省掉「收集」这一项，而该项已被快照化压到 1 次；无法降低 `S × N` 的距离运算 | 收益有限、需引入失效逻辑，不做 |
| 空间网格分组 | 需要同时降低 `S × N` | 当前上界（200 候选 × ≤39 邻居）可接受，不做 |

---

## 7. 边界与安全

| 场景 | 预期行为 | 依据 |
| --- | --- | --- |
| 玩家无团队（无 `Group`） | 邻居 = 本 `BotMgr` 的 BOT，行为与现状**完全一致** | `owner->GetGroup()` 为空时走**降级分支**（§5.3） |
| 单人带 BOT、BOT 未入组（新雇 BOT 的常态） | 邻居 = 本 `BotMgr` 的 BOT，与旧实现等价 | 无 `Group` ⇒ 降级分支；新雇 BOT 默认无 `BOT_ROLE_PARTY`，不会自动建组/入组（§3.5.4） |
| 单人带 BOT、BOT 已入组（点过「加入队伍（全部 BOT）」） | 邻居 = 组内自己的 BOT，与旧实现等价 | `Group::GetFirstBotMember()`（§3.5.4） |
| 有团队 | 邻居 = 组内已入组的全部 BOT（不含自己） | `Group::GetFirstBotMember()` |
| 队友未携带 BOT | 无贡献 | 链表为空 |
| 某 BOT 未入组（从未点过「加入队伍」/ 40 槽位占满） | **不作为规避目标**，也不参与规避 | 本次口径限定为「团队中的 BOT」，见 §3.5.3、§3.5.4 |
| 有团队、但自家 BOT 全部未入组 | 邻居集合为空 ⇒ 间距项为空，**自家 BOT 之间也不再分散** | 本次口径的必然推论（§3.5.4）；点一次「加入队伍（全部 BOT）」即恢复 |
| 队友玩家本体 | 不作为规避目标 | 遍历的是 BOT 链表，天然不含玩家 |
| 队友在不同地图/副本 | 该队友的 BOT 不参与规避 | `bot.IsInMap(other)` 过滤 |
| 队友相位不同 | 同上 | `bot.InSamePhase(other)` 过滤 |
| 组内 BOT 引用为空（`GetSource()` 为 `nullptr`） | 跳过 | `collectBot` 内的空指针保护 |
| 邻居 BOT 已死亡 | 不作为规避目标 | `other->IsAlive()` 过滤（与原逻辑一致） |
| 邻居 BOT 被移除 | 快照只在本 tick 内使用，指针不跨调用保存 | §5.5 生命周期约束；跨线程移除的既有假设见下方「线程安全说明」 |
| 自己不在战斗 | 惩罚恒为 `0.0f` | 原 `GetSpreadPenalty` 的 `bot.IsInCombat()` 校验已上移到调用方（§5.1） |
| 本 BOT 是坦克 | 不参与分散位优化 | 原有 `TryImproveSpreadPosition` 前置校验保留（注意：坦克仍可能作为**邻居**被其他 BOT 规避，符合预期） |
| 自由 BOT（`IAmFree`） | 不参与分散计算（调用点已用 `positionControl` 判空拦掉） | 现状保持 |
| 40 人满编纯玩家团 | 组内无 BOT，间距项为空，`spread` 实际失效 | 组容量上限，见 §6.2 |

### 线程安全说明

- **读取方式**：主路径遍历 `Group::m_botMemberMgr`（`RefMgr` 链表），降级分支遍历 `BotMgr::_bots`；两者都只做**只读遍历**，快照只在单次调用内使用，不跨线程传递指针。
- **降级分支是受保护的**：AI tick 期间 `BotMgr::Update`（`botmgr.cpp:203`）已持有 `_botsMutex`（递归锁），同线程再进入降级分支的遍历是安全的；`_botsIterateDepth`（`botmgr.h:295`）保证遍历过程中不会就地 `erase`（改为延迟移除）。
- **实际执行线程的澄清**：AI tick 由 `Player::Update` → `_botMgr->Update(diff)`（`PlayerUpdates.cpp:433`）驱动，运行在**地图更新线程**，并非世界主线程；而 BOT 移除 / 登出（`RemoveAllBotsFromGroup` 等）可能发生在主线程。
- **已知假设（非本次新增）**：`Group::m_botMemberMgr` 本身没有锁保护，因此「地图线程只读遍历组内 BOT 链表」依赖与既有用法相同的假设（`bot_ai.cpp:2208`、`botdatamgr.cpp:1952`、`Group.cpp:2117` 都是同样的读法）。本方案不新增加锁，邻居集合上界 ≤ 39，量与既有用法同阶。若将来要强化，应给 `Group` 增加加锁的 BOT 快照接口，属于另一项改动。

---

## 8. 预期效果

| 场景 | 现状（改造前） | 本方案 |
| --- | --- | --- |
| A、B 各带 4 远程且都已入组，A 开 spread 5 | 两组各自分散，组间可能重叠 | A 的 4 个 BOT 会避开 B 的 4 个 BOT，组内 8 个 BOT 统一分散 |
| A 开 spread 8，B 未开（B 的 BOT 已入组） | B 的 BOT 不受约束，A 的 BOT 只顾自己 | A 的 BOT 主动避开 B 的 BOT（阈值 8）；B 的 BOT 不动 |
| B 的 BOT 未入组（从未点过「加入队伍」/ 40 槽位占满） | 会被 A 的 BOT 规避 | **不再**被规避，也不参与规避（本次口径） |
| 单人 4 BOT（BOT 未入组） | 正常 | **完全不变**（无 `Group`，走降级分支） |
| 单人 4 BOT（BOT 已入组） | 正常 | **完全不变**（邻居 = 组内自己的 BOT，与旧口径等价） |
| A 有队、但 A 的 BOT 全部未入组 | A 的 BOT 之间正常分散 | **间距项为空**，连自家 BOT 之间也不再分散；点一次「加入队伍（全部 BOT）」即恢复（§3.5.4） |
| 无团队（无 `Group`） | 正常 | **完全不变**（走降级分支） |

---

## 9. 测试要点

1. **单人回归**：单玩家带 4 远程，`spread 5`，分别在「BOT 未入组」（无 `Group`，走降级分支）与「BOT 已入组」两种状态下确认 BOT 间距与改造前一致（两种状态的邻居集合都与旧口径等价，不应有任何行为差异）。
2. **双人团队重叠场景**：两人各带 4 远程（确认 8 个 BOT 均已入组），站位初始刻意重叠，开 `spread` 后确认两组 BOT 相互拉开。
3. **仅一方开启**：A 开 `spread`、B 不开（B 的 BOT 已入组），确认 A 的 BOT 会避开 B 的 BOT，且 B 的 BOT 行为不变。
4. **阈值差异**：A 设 3、B 设 15，确认 A 的 BOT 使用 3 作为阈值（验证 §5.6 规则）。
5. **未入组 BOT 不参与（本次口径核心）**：构造「BOT 未入组」场景（只点「创建队伍（单个 BOT）」而不用「加入队伍（全部 BOT）」，或把团队填到 40 槽位满），确认未入组 BOT **既不规避别人，也不被别人规避**；同一场景下再确认「有队但自家 BOT 全部未入组 ⇒ 邻居为空 ⇒ 间距项失效」，点一次「加入队伍（全部 BOT）」后恢复 —— 这是本口径的必然推论（§3.5.4）。
6. **无 `Group` 降级**：让玩家处于确实无 `Group` 的状态，确认 `spread` 仍按本 `BotMgr` 生效，与改造前一致；并确认「有组 / 无组」切换时不出现异常或振荡。
7. **异常与边界**：队友死亡、队友下线、队友切地图、队友切相位、团灭、组内 BOT 引用为空（`GetSource()` 为 `nullptr`）—— 均不应崩溃，且行为退化合理。
8. **性能观测**：25 人团（每人 8 BOT，组内 BOT 受 40 槽位限制）与 40 人满编团、密集 AOE 场景下观察帧耗时，重点看 `CalculateAttackPos` 的 **safespot 分支**（候选点最多 200，是单次成本上界；已做快照复用 + 平方距离预筛）。40 人满编纯玩家团组内无 BOT，应观察到零额外开销。
9. **抖动检查**：邻居变化后重点观察 BOT 是否左右来回跑动；现有 `0.25f` 最小改善阈值 + 快照复用是主要抑制手段。
10. **等价性与快照复用验证**：临时在 `CollectSpreadNeighbors` 内加计数日志，确认每次 `CalculateAttackPos` 只收集 **1** 次——无 AOE 时（走 `TryImproveSpreadPosition`）为 1 次，有 AOE 时（走 safespot 分支）也是 1 次，而不是 ≤ 200 次；同时确认删除 `GetSpreadPenalty` 前后（对照改造前的构建）BOT 落点一致。

---

## 10. 风险与明确不做的事项

### 风险

| 风险 | 说明 | 缓解 |
| --- | --- | --- |
| **组容量导致的规避缺口** | 40 槽位占满后 BOT 无法入组，这些 BOT 互不规避；40 人满编纯玩家团时间距项为空，`spread` 完全失效 | 属于本次「只考虑团队中的 BOT」口径的必然结果（§3.5.3）；若要覆盖需改用 §4.2 的备选，或提高组容量 |
| **有队但自家 BOT 未入组** | 玩家处于队伍中、但自家 BOT 从未入组（只点了「创建队伍（单个 BOT）」；或入组之后才新雇 BOT）⇒ 邻居集合为空，**连自家 BOT 之间的分散也失效** | 本次口径的必然推论（§3.5.4）；对任意一只 BOT 点「加入队伍（全部 BOT）」即可恢复（会补上并持久化 `BOT_ROLE_PARTY`） |
| 站位抖动 | 邻居变化后，候选点最优解可能频繁切换 | 保留 `0.25f` 最小改善阈值；快照保证同一轮内排名稳定 |
| 「互相避让」无解 | 空间严重不足时（楼梯、窄门），所有 BOT 都想跑开 | 现有 `maxOwnerDistance` 约束 + LOS 校验会限制落点范围，退化为「尽量散」而非「一定散开」 |
| 性能 | 多团并发 + 密集 AOE | 邻居上界仅 `40 − M`（§6.2）；两个调用点都已快照复用 + 平方距离预筛（§5.5、§6.5），单次调用的邻居收集恒为 1 次；必要时按 §6.5 的「未做项」继续评估 |
| 顺序相关性 | 独立贪心导致结果依赖 tick 内执行顺序 | 属于 §2.3 已确认的模型取舍，不追求消除 |

### 明确不做（本次范围外）

1. **把队友玩家本体纳入规避目标**：需求限定为「BOT」。扩展点已收敛在 `CollectSpreadNeighbors` 内部，将来追加即可。
2. **把敌对/中立 BOT 纳入**：属于另一套语义，需单独命令。
3. **团队级统一分散配置**：由队长设置一次、全团生效，替代当前 per-BotMgr 的 `_spreadDistance`。
4. **`mass` 模式的团队化**：同源问题（`IsMassEligible` 中的 `ai.GetBotOwner() == _botMgr.GetOwner()` 同样限定单玩家），但涉及槽位角度分配的团队级重排，需单独设计。
5. **团队全局统一分配落点**：即 §2.3 所述的另一套模型。
6. **容量增强（半径分层 + 黄金角均分）**：解决同半径圆周容量不足问题，见 §11。
7. **把未入组的 BOT 纳入规避目标**：包括团队成员 `BotMgr` 中未入组的 BOT。本次口径明确限定为「团队中已入组的 BOT」（§5.3），如将来要放宽，扩展点同样收敛在 `CollectSpreadNeighbors` 内部。

---

## 11. 容量增强方案（后续可选，不在本次实施范围）

> **状态**：本节的方案**不参与本次实施**。本次只落地 §4 确定的「团队 BOT 遍历」。
> **理由**：团队 BOT 遍历是纯「改变参考集合」，不改变落点生成方式；而容量增强会改变落点分布本身，影响面更大，需要单独评估与测试。

### 11.1 要解决的问题

现有分散只有**一个自由度（角度）**，半径被锁死为 `attackDistance`：

```393:399:src/server/game/AI/NpcBots/botpositioncontrol.cpp
    float baseAngle = target.GetAbsoluteAngle(position.GetPositionX(), position.GetPositionY());
    for (int8 direction : std::array<int8, 2>{ -1, 1 })
    {
        for (uint8 step = 1; step <= 3; ++step)
        {
            float angle = Position::NormalizeOrientation(baseAngle + direction * step * float(M_PI) / 12.0f);
            Position candidate = target.GetFirstCollisionPosition(attackDistance, Position::NormalizeOrientation(angle - target.GetOrientation()));
```

而 `attackDistance` 由**职业 + 玩家级 rangeMode** 决定，与 BOT 个体无关：

```5800:5800:src/server/game/AI/NpcBots/bot_ai.cpp
    float dist = (rangeMode == BOT_ATTACK_RANGE_EXACT) ? exactRange : GetSpellAttackRange(rangeMode == BOT_ATTACK_RANGE_LONG) - 5.f;
```

于是同一玩家的同职业远程 BOT 落在**同一个圆周**上。同半径圆周的容量存在硬上限：

\[ \Delta\theta = 2\arcsin\!\left(\frac{D}{2r}\right), \qquad N_{\max}(r) = \left\lfloor \frac{2\pi}{\Delta\theta} \right\rfloor \]

| 圆周半径 r | 出处（`rangeMode` 为短距时） | D=5 容量 | D=10 容量 |
| --- | --- | --- | --- |
| 10 | 默认职业 `15-5` | 12 | 6 |
| 15 | 法师 / 术士 / 牧师 / 萨满 `20-5` | 18 | 9 |
| 18 | 默认职业远距 `23-5` | 22 | 11 |
| 20 | 猎人 `25-5` | 25 | 12 |

超过 `N_max` 后，**任何算法都无法在这个圆周上满足间距要求** —— 这就是 §4 团队 BOT 遍历只能「缓解」而不能「根治」扎堆的原因。此外，§5.3 的邻居口径还受团队组容量约束（§6.2）：团队人数越多，能入组并参与分散的 BOT 反而越少。

### 11.2 方案总览（三级处理）

```
对同一目标的每个分散参与者：
  1. 按攻击半径分桶            → 得到若干「同半径组」
  2. 每组计算容量 N_max(r)，判定是否超容
       ├─ 未超容 → 单环：环内用黄金角均分角度
       └─ 超容   → 半径分层：拆成 K 个同心环，各环独立做黄金角均分
  3. 输出（目标角度, 目标半径）作为粗分配结果
```

核心思路：**把「一维角度容量」扩展成「二维环带容量」**，容量近似从 `N_max(r)` 提升到 `Σ N_max(r_i)`。

### 11.3 半径环带的定义

每个 BOT 有一个可用半径区间 `[r_min, r_max]`，而不是单一半径：

| 端点 | 含义 | 取值 |
| --- | --- | --- |
| `r_max` | 沿用现有 `attackDistance`（`dist`，见 `bot_ai.cpp:5800`） | 保持不变，作为最外环 |
| `r_min` | 仍能攻击到目标的最小距离 | 远程：`max(近战范围 + 2, r_max × 0.4)` 起，按职业射程实测；近战：近战范围（`GetCombatReach()` + 基准范围） |

约束：`r_min` 不能低于「还能造成伤害」的下限，否则分层会牺牲 DPS 换取间距。这一点必须显式校验，而不是无条件向内收缩。

### 11.4 分桶与容量判定

```
AssignSpreadSlots(self, candidates, target):
    遍历 candidates，按「半径」归桶（容差 RING_TOLERANCE，建议 1.0 码）
    同一桶内以 ObjectGuid 升序作为稳定序号（见 §11.7）

    对每个桶（含自己所在桶）：
        r       = 桶的半径
        N       = 桶内成员数
        N_max   = floor(2π / (2 * asin(D / (2r))))      // D = _spreadDistance
        K       = ceil(N / N_max)                        // 需要的环数

        if K == 1:  单环模式，半径 = r
        else:       半径分层（§11.5）
```

注意 `K` 是按**自己的桶**计算的，因此同一桶内所有 BOT 会得到相同的 `K`，保证分配一致。

### 11.5 半径分层规则

`K` 个环在 `[r_min, r_max]` 上等分：

```
r_i = r_max - i * (r_max - r_min) / (K - 1)      // i = 0..K-1，i=0 为最外环
```

各环半径不同 ⇒ 各环容量不同（内环容量更小），因此需要**迭代修正**：

```
初始:  K = ceil(N / N_max(r_max))
循环:
    计算各环容量 N_max(r_i)
    若 Σ N_max(r_i) < N:  K += 1，重算
    否则: 结束
```

**分层收益示例**（法师，`r_max = 15`，`r_min = 8`，`D = 5`）：

| 场景 | 单环容量 | 分层结果 | 总容量 |
| --- | --- | --- | --- |
| 10 个法师 | 18 | K=1，单环 r=15 | 18（够用） |
| 24 个法师 | 18 | K=2：r=15(18) + r=8(9) | 27（够用） |
| 40 个法师 | 18 | K=3：r=15(18) + r=11.5(14) + r=8(9) | 41（够用） |

即：**通过向内分层，同职业 BOT 的容量从 18 提升到 41**。

### 11.6 层内角度分配：复用 mass 的黄金角思路

`mass` 模式已经在做同类事情：

```455:469:src/server/game/AI/NpcBots/botpositioncontrol.cpp
BotPositionControl::BotMassSlot& BotPositionControl::GetOrCreateMassSlot(Creature const& bot)
{
    auto [itr, inserted] = _massSlots.try_emplace(bot.GetGUID());
    if (!inserted)
        return itr->second;

    uint64 seed = bot.GetGUID().GetRawValue() ^ (_botMgr.GetOwner()->GetGUID().GetRawValue() << 1);
    seed ^= uint64(_massMode) << 57;
    float angleJitter = GetStableUnitFloat(seed) * 0.75f;
    float radiusValue = GetStableUnitFloat(seed ^ 0x9e3779b97f4a7c15ULL);

    itr->second.angle = Position::NormalizeOrientation(float(_massSlots.size() - 1u) * GOLDEN_ANGLE + angleJitter);
    itr->second.radius = std::max(0.5f, std::sqrt(radiusValue) * _massRadius);
    return itr->second;
}
```

可直接复用的三个要点：

1. **黄金角均分**：`GOLDEN_ANGLE = 2.39996323f`（≈137.5°）。序号 × 黄金角得到的角度序列在圆上分布最均匀，且不会像 `2π/N` 那样在成员数变化时整体错位。
2. **稳定抖动**：`GetStableUnitFloat(seed)` 由 GUID 派生，同一 BOT 每次得到相同抖动值，避免「每次计算都换角度」导致抖动。
3. **半径也用稳定随机**：`sqrt(radiusValue) * radius` 让半径分布在环带上面积均匀，而不是挤在环的边缘。

差异点（必须替换的部分）：

| 项 | `mass` 的做法 | 本方案的做法 |
| --- | --- | --- |
| 序号来源 | `_massSlots.size() - 1`（插入顺序） | **按 `ObjectGuid` 排序后的稳定序号**（见 §11.7） |
| 半径 | `sqrt(rand) * _massRadius`（单环内随机） | `r_i`（分层后的确定性环半径）+ 微小抖动 |
| 作用域 | 单 BotMgr | 全团队 |

`mass` 可以用插入顺序，是因为它在**单个 BotMgr 内串行分配**，顺序天然稳定；跨玩家场景没有这种保证，必须换成确定性排序。

### 11.7 分布式一致性：确定性排序（关键设计点）

本方案沿用 §2.3 的分布式模型 —— **没有协调者，每个 BOT 各自计算**。因此必须让每个 BOT 独立算出**完全相同**的分配结果，否则会出现「两个 BOT 抢同一个角度」。

做法：用**确定性排序**替代「插入顺序」：

```
1. 构造候选集合 = 自己的团队级邻居（§5.3 的 CollectSpreadNeighbors，即组内已入组 BOT）+ 自己
2. 全部按 ObjectGuid 升序排序
3. 分桶 / 分层 / 环内序号，全部基于排序后的稳定位置
```

只要「候选集合 + 目标 + `_spreadDistance`」不变，任何 BOT 在任意时刻、任意玩家侧计算，都会得到同一套 `(angle, radius)` 分配。

> 注意这**不违反** §2.3「不实现团队级集中调度」的约束：不是某个 leader 统一分配后下发，而是每个 BOT 用同样的确定性算法独立推出同样的结论。

### 11.8 数据结构与缓存

```cpp
// 分散槽位：与 mass 的 BotMassSlot 对应
struct BotSpreadSlot
{
    float angle = 0.0f;      // 目标角度（相对目标的绝对朝向）
    float radius = 0.0f;     // 目标半径（分层后所在环）
    uint8 ringIndex = 0;     // 环带序号，0 = 最外环
};

std::unordered_map<ObjectGuid, BotSpreadSlot> _spreadSlots;
uint64 _spreadParamsHash = 0;   // 分配参数指纹，用于判断是否需要重建
```

**失效与重建**：

| 触发条件 | 说明 |
| --- | --- |
| 目标 GUID 变化 | 换目标后环带需重新分配 |
| 参与成员变化 | 有 BOT 加入 / 离开 / 死亡 / 跨地图 |
| `_spreadDistance` 变化 | 容量 `N_max` 依赖 `D` |
| 团队组成变化 | 影响候选集合 |
| 定时兜底 | 建议 1s 量级强制重建，避免遗漏失效路径 |

实现上用**参数指纹**（目标 GUID + 成员数 + 成员 GUID 哈希 + `D`）比对，指纹一致则直接复用 `_spreadSlots`，避免每帧重算。

### 11.9 与现有贪心的关系：两级结构

新增的分配结果是**粗分配**，现有 `TryImproveSpreadPosition` 是**细修正**，两者串联而非替换：

```
CalculateAttackPos(...):
    ...
    1. 粗分配：AssignSpreadSlots 得到 (targetAngle, targetRadius)
    2. 取初始落点：target.GetFirstCollisionPosition(targetRadius, targetAngle)
    3. 细修正：跑现有 6 点贪心（±15°/±30°/±45°），处理
       LOS / AOE / maxOwnerDistance 等硬约束
    4. 若粗分配落点被约束否决 → 退化为现有行为，不劣化
```

这样做的收益：

- 正常情况下由粗分配给出**全局均匀**的初始布局，贪心只做局部修正，收敛更快；
- 粗分配不可达时（窄楼梯、AOE 区、主人太远）自动退化到现状，**不会比现在更差**；
- 与 `mass` 模式的设计哲学一致（先按槽位落点，再局部修正）。

### 11.10 每个 BOT 的计算成本仍是 O(N)

一个容易过度设计的地方：**不需要真的做排序**。

每个 BOT 只需算出「自己在所属环内的序号」和「自己属于哪个环」，可以通过**单次遍历**统计得到：

```
遍历每个候选 other:
    if other.radius != self.radius:  更新桶计数（用于算 K）
    else if other.guid < self.guid:  selfIndex++      // 自己在桶内的序号
                                    ringBase = 该桶已分配的前缀环容量
```

即：`O(N)` 一次扫描即可确定 `(ringIndex, indexInRing)`，与现在 §5.4 的惩罚计算同量级，**不引入排序的 O(N log N) 开销**。

### 11.11 与本次方案的关系

| 项 | 本次（§4 团队 BOT 遍历） | 本节（容量增强） |
| --- | --- | --- |
| 邻居范围 | 团队内已入组 BOT | 同左（复用 §5.3） |
| 落点生成 | 不变（现有 6 点贪心） | 新增粗分配 + 保留贪心 |
| 需要新增状态 | 否（仅临时快照） | 是（`_spreadSlots` + 指纹） |
| 失效处理 | 无 | 指纹 + 定时重建 |
| 影响面 | 小（单文件） | 中（新增分配与缓存逻辑） |
| 结论 | **本次实施** | **后续按需，单独评估** |

---

## 12. 改动清单（实施对照）

| 文件 | 位置 | 改动类型 | 落地状态（行号为落地后现状） |
| --- | --- | --- | --- |
| `botpositioncontrol.h` | 顶部 include | 新增 `#include <vector>` | 已完成（`:9`） |
| `botpositioncontrol.h` | public 区 | 新增 `CollectSpreadNeighbors`、`GetSpreadPenaltyFromNeighbors` 声明 | 已完成（`:56`、`:58-60`） |
| `botpositioncontrol.h` | public 区 | 删除 `GetSpreadPenalty` 声明 | 已完成 |
| `botpositioncontrol.cpp` | 顶部 include | 新增 `#include "Group.h"` | 已完成（`:7`） |
| `botpositioncontrol.cpp` | 新增实现 | `CollectSpreadNeighbors`（`:313-345`）、`GetSpreadPenaltyFromNeighbors`（`:347-366`，含平方距离预筛） | 已完成 |
| `botpositioncontrol.cpp` | `GetSpreadPenalty` | 删除整个定义（逻辑拆入上面两个方法） | 已完成 |
| `botpositioncontrol.cpp` | `TryImproveSpreadPosition` | 循环前收集一次邻居快照，循环内复用 | 已完成（`:379-380`） |
| `bot_ai.cpp` | `CalculateAttackPos` 的 safespot 择优分支 | 短路校验上移 + 收集一次邻居快照 + 循环内复用 | 已完成（`:5935-5947`） |

**不需要改动（已确认未改）**：`botcommands.cpp`、`botmgr.h/.cpp`、`mass` 相关全部逻辑。

落地后的口径汇总：

| 项 | 改造前 | 落地后 |
| --- | --- | --- |
| 邻居来源 | 本 `BotMgr::GetBotMap()`（自己名下全部 BOT） | 有团队：`Group::GetFirstBotMember()`（组内已入组 BOT）；无团队：降级为本 `BotMgr` |
| 未入组 BOT | 会被规避 | 不参与（入组是逐 BOT 的显式操作，§3.5.4；有队但自家 BOT 全部未入组时邻居集合为空 ⇒ 间距项失效） |
| 玩家本体 / 路人 BOT / 敌对 BOT | 不参与 | 不参与（不变） |
| 距离阈值 | 本 `BotMgr` 的 `_spreadDistance` | 不变（§5.6） |
| 对外接口 | `GetSpreadPenalty`（单点查询，内部自行收集） | `CollectSpreadNeighbors` 收集 + `GetSpreadPenaltyFromNeighbors` 算术（**公开**，由调用方控制复用粒度）；`GetSpreadPenalty` 已删除 |
| 短路校验（未开 spread / 不在战斗） | 在 `GetSpreadPenalty` 首行 | 上移到调用方（`bot_ai.cpp:5938` 与 `TryImproveSpreadPosition` 前置校验） |
| 邻居收集次数 | 每个候选点 1 次 | 每次 `CalculateAttackPos` 恒为 1 次（§5.5） |
| 圈内判定 | `GetExactDist2d`（每次比较都开方） | `GetExactDist2dSq` + 仅圈内邻居 `sqrt`（§5.4） |

实际工作量：约 80 行代码，集中在 `botpositioncontrol.h`、`botpositioncontrol.cpp` 与 `bot_ai.cpp` 的 safespot 分支。
