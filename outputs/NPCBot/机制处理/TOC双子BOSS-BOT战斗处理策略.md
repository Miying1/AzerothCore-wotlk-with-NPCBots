# TOC 双子 BOSS（Fjola Lightbane / Eydis Darkbane）—— NPCBot 战斗处理策略

> 适用地图：Trial of the Crusader（map 649）第 4 号首领
> 脚本实现：`src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/boss_twin_valkyr.cpp`
> BOT 框架：`src/server/game/AI/NpcBots/`（含 `Hazards/` 定制模块）
>
> **当前状态：设计阶段。** 本文档只做方案说明，未修改任何源码，也未写入 SQL：
> `data/sql/updates/pending_db_world/` 无新增文件，`outputs/NPCBot/机制处理/危险区域识别.sql` 保持原样未改动。
> 文中第 6 节的 SQL 仅供评审，落地时再按 `pending_db_world` 的 `rev_*.sql` 规范新建文件。

---

## 0. 结论摘要

| 场景 | BOT 应当的行为 | 现状 | 落地阶段 |
|---|---|---|---|
| 开战瞬间 | 按团队颜色比例自动分配光 / 暗精华，并自己施法获得精华 | 无法获得（精华只能玩家点 NPC gossip 取得） | P1 |
| 选择攻击目标 | 暗精华 → 打 **Fjola Lightbane(34497)**；光精华 → 打 **Eydis Darkbane(34496)** | 混打、随机 | P1 |
| 目标有 Twin Pact 护盾 | 换另一只 boss | 继续打有盾目标，DPS 被 175000 吸收 | P1 |
| 场上出现 Concentrated Light/Dark 球 | 不打球；异色球视为 20 码危险区绕行 | 可能被当作普通敌人攻击 | P0 / P1 |
| 站位 | 光精华贴光之精华 NPC(34568) 一侧，暗精华贴暗之精华 NPC(34567) 一侧 | 无差别绕 boss | P1 |
| 同色球 | **BOT 无法撞球**（引爆判定只认真实玩家），只能被动吸收玩家引爆的同色球 | 不处理 | 需改脚本 |
| 坦克 | 两个坦克分别拉光 / 暗各一只，精华取所坦 boss 同色（吸收 Twin Spike） | 无区分 | P1 |
| Touch of Light/Darkness | 精华常驻即可免疫，无需额外动作 | 常驻后自动生效 | P1 |
| Vortex | 同色精华减伤，无需额外动作 | — | — |
| 狂暴（6 分钟英雄 / 10 分钟普通） | 不处理 | — | — |

**核心矛盾**：这套双子的全部机制都建立在"精华（Essence）"上，而精华**只能由玩家点击场地两侧的精华 NPC 通过 gossip 获得**（`boss_twin_valkyr.cpp:569` `npc_essence_of_twin::OnGossipHello`）。BOT 作为 Creature AI 不会、也无法走 gossip 流程 —— **因此 BOT 侧必须自己 `CastSpell(self, essenceId)` 代劳**，这是所有后续策略的前置条件，不能靠数据库配置解决。

---

## 1. 战斗机制拆解（以本仓库脚本为准）

| 机制 | 脚本位置 | 周期 / 数值 | 对 BOT 的影响 |
|---|---|---|---|
| 共享血量 | `:265 UpdateSharedHealth` | 以 Lightbane 为基准每 tick 同步 | 打哪一只都会掉血，分兵不会导致血量失衡 |
| 精华获取 | `:569` gossip；`:574-675` | 点光门 → 移除暗精华 → 挂光精华；点暗门反之 | **BOT 拿不到，必须代施** |
| 精华光环 | `:733 spell_valkyr_essence_aura` | SCHOOL_ABSORB（吸收同色伤害）+ DUMMY 计数 | 同色球伤害被吸收并转层数 |
| 球召唤 | `:309-338` | 开战 10~15s 起，每 8s 一波 4 个(非终极波)/6 个(25人)；每 ~39s 一次大波 24/36 个 | 从中心 47 码半径生成，向场内游走 |
| 球引爆 | `:866 spell_valkyr_ball_periodic_dummy_aura` | 与**真实玩家**距离 ≤ 2.75 码即引爆（BOT 无法触发） | 引爆后 1.5s 消失，留下一发范围伤害 |
| 球伤害 | `Unleashed Light 65795` / `Unleashed Dark 65808` | SCHOOL_DAMAGE，半径索引 29 | 同色精华吸收 → 每 1000 吸收量 1 层 Powering Up |
| Powering Up | 67590(10N)/67602(25N)/67603(10H)/67604(25H) | 100 层 → Empowered | Empowered Light 65748 / Dark 65724：对**相反** aurastate 目标 +100% 伤害 |
| Empowered 对应关系 | `boss_eydisAI` 设 aurastate 19，`boss_fjolaAI` 设 aurastate 22（`:531` / `:557`）；65748 misc=19，65724 misc=22 | — | **暗精华强化(65724) → 打光 boss(22)；光精华强化(65748) → 打暗 boss(19)** |
| Twin Spike | `:339-342` | 7~10s 一次，打当前坦克；附 aura 87（受伤 +20%） | 坦克承伤；同色精华可吸收 spike 伤害 |
| Touch of Light/Darkness | `:343-410` | 10~25s；对无对应精华的玩家造成周期伤害 | 精华常驻即免疫 |
| 特殊技能循环 | `:411-463` | 每 45s 随机一种，SpecialMask 保证四种都出过后重置 | 见下 |
| ├ Light/Dark Vortex | 66046 / 66058 | boss 自身 aura 23（周期触发） | 同色精华减伤，无位移要求 |
| └ Light/Dark Twin Pact | 65858 / 65874（盾）+ 65876 / 65875 | 175000 全学派吸收 + 机制免疫，仅加给其中一只 | **打有盾目标 = 白打** |
| 狂暴 | `:299-308` | 6min（英雄）/ 10min（普通） | 不处理 |

### 1.1 精华 Spell ID 按难度映射

来源：`boss_twin_valkyr.cpp:151-186`（`DoAction(-1)` 的清理列表）与 `:585-660`（gossip 切换）。

| 难度 | Dark Essence | Light Essence |
|---|---|---|
| 10N（difficulty 0） | 65684 | 65686 |
| 25N（difficulty 1） | 67176 | 67222 |
| 10H（difficulty 2） | 67177 | 67223 |
| 25H（difficulty 3） | 67178 | 67224 |

> ⚠️ 已知脚本疑点：`OnGossipHello` 的光门分支在 `:621` 固定 `CastSpell(player, 65686)`，未按难度取 67222/67223/67224；25 人本玩家身上的光精华实际由 spell diff 或 `DoAction(-1)` 的清理列表间接反映。**BOT 代施时必须按上表取难度对应 ID，不要照抄 `:621` 的固定值。**

---

## 2. BOT 现状能力与缺口

| 能力 | 现状依据 | 缺口 |
|---|---|---|
| 决策入口 | 各职业 `UpdateAI` → `bot_ai::GlobalUpdate`（`bot_ai.cpp:19114`），已有 `UpdateTankSwap(diff)` 挂载点（`:19234`） | 需要一个同构的 `UpdateTwinValkyrTactic(diff)` 钩子 |
| 选目标 | `bot_ai::_getTargets()`（`:4135`），map 特判分支在 `:4204-4379`（BWL / ICC 已有先例） | 无 649 双子分支 |
| 免疫换目标 | `CanBotAttack()` 内 `:3931` 只处理"整系免疫" | Twin Pact 是 **absorb 不是 immune**，BOT 不会自动换目标 |
| 攻击排除 | `IsPointedNoDPSTarget()`（`:1354`，依赖团队标记图标） | 球体不是标记目标，BOT 可能把 34628/34630 当敌人打 |
| 危险区域 | `CalculateAoeSpots()`（`:5294`），649 块在 `:5535-5580`（酸喉毒池、戈莫克火圈） | 无球体；且无法表达"同色球安全、异色球危险"这种**相对于 BOT 自身**的判定 |
| DB 危险区 | `npcbot_creature_hazard`（`Hazards/NPCBotHazardMgr.h:22`，加载 `Hazards/NPCBotHazardMgr.cpp:119`），过滤条件 `required_aura_spell_id` 查的是**生物身上**的光环（`:63-79`） | 无法按 BOT 自身光环区分同色 / 异色球 |
| 走位 | `BotMovement()`(`:1109`)、`CalculateAttackPos()`(`:5908`)、`BotPositionControl` 集合/分散 | 无双子专用站位锚点 |
| 精华 | 无 | 完全缺失（最大缺口） |
| 自我施法能力 | 已有大量 `me->CastSpell(me, ..., true)` 先例（`:6709`、`:16357`、`:19111`） | 能力具备，只差战术逻辑 |

**结论**：DB 层能兜住的只有"球体范围伤害"这一项；颜色分配、精华代施、目标重定向、站位都必须落到 `bot_ai.cpp` 的 649 战术分支里 —— 这与 ICC Marrowgar/Sindragosa/LichKing 的处理范式完全一致（`:4225-4378`）。

---

## 3. 策略设计

### 3.1 颜色（光 / 暗）分配

**目标**：团队光暗比例 ≈ 1:1，理由有二 ——
1. Touch of Light/Darkness 只打"没有对应精华"的人，比例失衡会让一侧被点名过密；
2. Vortex / 球是全场铺开的，两侧都要有人分摊。

**规则**：

1. **真人优先**。开战时先扫描团队里已有精华的真人（读 4 个难度 ID 的光/暗光环），统计 `lightCount` / `darkCount`。
2. **BOT 补位**。BOT 全体先按 `ObjectGuid` 升序排序（稳定顺序），依次分配给当前人数较少的一侧，直到两侧持平后交替分配。
3. **结果缓存**。分配结果写入 `_valkyrColor`，只在"进入战斗 / 离开战斗 / 换图"时重置，避免 BOT 颜色每 tick 抖动（抖动会导致精华反复切换、层数清零）。
4. **坦克例外**。坦克的精华颜色**不参与补位**，固定取"所坦 boss 的同色"（理由见 3.6）。
5. **真人强制覆盖**。若真人对某个 BOT 使用了"集火/指定目标"图标（`IsPointedDPSTarget`），该 BOT 直接跟随被指定目标，颜色策略让位。

### 3.2 精华代施（关键路径）

```
每隔 1000ms 检查一次（仅 map 649 且自身在战斗中）：
    期望精华 = 按难度 + 自身颜色查表
    若身上已有期望精华 → 返回
    若身上有相反精华 → 先 RemoveAura（与玩家点门一致）
    CastSpell(self, 期望精华, true)
```

注意点：

- **必须周期重试**：`DoAction(-1)`（`:143`）在战斗结束会清空全员精华，重开战斗需要重新挂。
- **不要每 tick 施法**：精华是 100% 稳定的光环，用 1s 节流即可，避免无谓的 `CastSpell` 开销。
- **失败兜底**：若连续 N 次施法后 `HasAura` 仍为 false（极可能是 aura 对 Creature 有隐藏限制），标记该场战斗禁用精华策略，退化为"目标重定向 + 站位 + 躲异色球"，并在 `BOT_LOG_DEBUG("npcbots.toc", ...)` 中留痕。这是本方案唯一的真实风险点，必须在实现后第一时间实测。

### 3.3 目标分配

| BOT 角色 | 精华颜色 | 攻击目标 | 依据 |
|---|---|---|---|
| 近战 / 远程 DPS | 暗 | Fjola Lightbane（34497，光） | Empowered Dark(65724) 的 aurastate 是 22 = Fjola，+100% 只对光 boss 生效 |
| 近战 / 远程 DPS | 光 | Eydis Darkbane（34496，暗） | Empowered Light(65748) 的 aurastate 是 19 = Eydis |
| 坦克 1（坦克序号 0） | 光 | Fjola Lightbane | 同色精华吸收 Twin Spike 的光明伤害 |
| 坦克 2（坦克序号 1） | 暗 | Eydis Darkbane | 同上 |
| 治疗 | 保留分配色 | 不主动打，跟随站位 | — |

**优先级覆盖**（从高到低）：

1. 真人指定目标（`IsPointedDPSTarget`）
2. 目标有 Twin Pact 盾且另一只无盾 → 打另一只
3. 角色 + 颜色默认目标（上表）
4. 目标死亡 → 打另一只

> 关于"分兵"：本脚本 `UpdateSharedHealth` 是单向复制（`:265-280`），两只 boss 的伤害都会累积，分兵不会导致血量偏离。分兵的真实收益只有一个——**避开 Twin Pact 护盾**，所以第 2 条覆盖优先级比"按颜色分兵"更重要。

**禁止目标**：`34628 Concentrated Darkness` / `34630 Concentrated Light`。实现方式是在 `CanBotAttack()`（`:3828`）或 `_getTargets()` 的 649 分支里直接排除这两个 entry（仅在双子战斗中生效，避免影响其它场景）。

### 3.4 站位

| 角色 | 锚点 | 半径 |
|---|---|---|
| 光精华 DPS / 治疗 | 最近的 `34568 Light Essence` NPC | 6~10 码（按 BOT 序号做扇形错开） |
| 暗精华 DPS / 治疗 | 最近的 `34567 Dark Essence` NPC | 6~10 码 |
| 坦克 | boss 脚下（不参与侧站位） | — |

**为什么用精华 NPC 当锚点而不是硬编码坐标**：

- 精华 NPC 由 boss 脚本在构造时召唤在场地两侧（`:533-536 / :559-562`，`Locs[LOC_DARKESS_1/2]`、`Locs[LOC_LIGHTESS_1/2]`），位置即"官方分边点"；
- BOT 侧只需 `AllCreaturesOfEntryInRange` 搜索最近的一个，**不需要跨脚本共享坐标**，也不会因实例变化失效；
- 贴边站位天然减少异色球的命中面积，同时更靠近同色球的飞行轨迹（为 P2 吃球做准备）。

**与现有站位系统的关系**：优先级应低于 `BotPositionControl::EnableMass`（真人手动集合）与 `sendto` 战术点，高于默认的"绕 boss 站位"。实现上在 `CalculateAttackPos()`（`:5908`）末尾对双子场景做一次锚点修正即可。

### 3.5 球（Concentrated Light / Dark）处理

| 球颜色 | 与 BOT 精华关系 | 处理 |
|---|---|---|
| 同色 | 相同 | 理论上无威胁（被精华吸收并加 Powering Up 层数），但 **BOT 撞不爆球**，详见下方结论 |
| 异色 | 相反 | **危险**：被真人引爆后 20 码内造成大量伤害。加入危险区域，绕行 + 圈内禁读条 |
| 无精华（兜底） | — | 两种球都视为危险 |

**关键结论（已核对代码，BOT 不能撞球）**：

球的引爆判定只有一处 —— `boss_twin_valkyr.cpp:875-889` 的 `spell_valkyr_ball_periodic_dummy_aura::HandleEffectPeriodic`：

```cpp
if (Player* player = creature->SelectNearestPlayer(2.75f))
    if (creature->GetExactDist2d(player) <= 2.75f)
        // ... 引爆
```

`SelectNearestPlayer` → `WorldObject::SelectNearestPlayer`（`Object.cpp:2651`）→ `NearestPlayerInObjectRangeCheck`（`GridNotifiers.h:1430`）的 `operator()(Player* u)`，配合 `PlayerLastSearcher`（`GridNotifiersImpl.h:523`）只遍历 `PlayerMapType`（真实玩家列表）。NPCBot 是 Creature，类型上就不可能进入该搜索；本项目 npcbot 补丁也没有修改这个 checker（补丁只作用于 `DefaultTargetSelector` 一类 AI 选目标逻辑）。球体自身 AI（`:722-729`）只做 `MoveToNextPoint`，没有任何碰撞检测。

因此：

- BOT **即便贴到球上，球也不会爆**，既拿不到 Powering Up，也不会被自己引爆；
- BOT 也**不会因为靠近异色球而自爆** —— 异色球的伤害只来自"真人引爆后的 AoE"，所以躲异色球的意义是"别站在真人即将引爆的球上"，而不是"别碰球"；
- BOT 唯一获得 Powering Up 的途径是被动受益：真人在附近引爆同色球，BOT 处于爆炸范围内且有同色精华时，伤害被 SCHOOL_ABSORB 吸收并转层数。这条链路不可控，不能作为战术依赖。

**P0（纯 DB，立即生效）**：把两种球都配进 `npcbot_creature_hazard`，让 BOT 一律绕行。代价是永远拿不到 Empowered，但先保命、先不炸团。

**P1（代码）**：在 `CalculateAoeSpots()` 的 649 块里按 `_valkyrColor` 过滤 —— 只把异色球 `emplace_back` 进 `spots`，同色球跳过。这一步做完之后，`npcbot_creature_hazard` 里球体的那条配置应当删除，否则 DB 规则会把同色球也重新塞回危险区（DB 规则走的是 `sNPCBotHazardMgr->CollectCreatureHazards`，在硬编码之后执行，`bot_ai.cpp:5320`）。

**P2（主动吃球）—— 当前脚本下不可实现，必须先改 BOSS 脚本**：BOT 侧导航（同色球 15 码内、非坦克、不在其它 AoE 内、用 `BotMovement(BOT_MOVE_POINT, &pos)` 靠近）改完也不会触发引爆，纯属白跑。若确定要做，改动点在 `boss_twin_valkyr.cpp:875-889`，把引爆判定从"只认真实玩家"扩展为"玩家优先，其次 NPCBot"：

```cpp
// 原：if (Player* player = creature->SelectNearestPlayer(2.75f))
// 改：没有真实玩家在 2.75 码内时，允许 NPCBot 触发引爆
Unit* trigger = creature->SelectNearestPlayer(2.75f);
if (!trigger)
{
    Acore::AnyUnitInObjectRangeCheck check(creature, 2.75f);
    Acore::UnitLastSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(creature, trigger, check);
    Cell::VisitObjects(creature, searcher, 2.75f);
    if (trigger && !trigger->IsNPCBot())
        trigger = nullptr;
}
if (trigger && creature->GetExactDist2d(trigger) <= 2.75f)
    // ... 引爆
```

代价与风险：

- BOT 引爆的球对**异色队友同样是 20 码范围伤害**，等于把"是否炸团"的决定权交给 AI，必须配合 `_valkyrColor` 与站位严格过滤，否则会出现 BOT 主动炸死真人的事故；
- 球的游走路径由 `EVENT_SUMMON_BALLS` 随机生成（`:320`），BOT 追击会偏离站位锚点，与 3.4 的分边站位冲突；
- 收益仅为 30% 概率的 `Surge of Speed` 与长期 Empowered，稳定性差。

结论：**默认不做**。若要做，建议加独立开关 `NpcBot.TwinValkyrTactic.EatOrb`（默认 false），且只在"团队成员全为 BOT + 自己主人"的场景下允许。

### 3.6 坦克与 Twin Spike

- Twin Spike（66075 光 / 66069 暗）只打当前坦克，伤害类型与施法 boss 同色，附带 aura 87（受伤 +20%）。
- 同色精华的 SCHOOL_ABSORB 会直接吸收 spike 伤害，**所以坦克的精华必须与所坦 boss 同色**，这是坦克不参与 3.1 补位的原因。
- 不需要换坦：两只 boss 各有独立威胁目标（`UpdateVictim`），Twin Spike 是单体短周期技能，aura 87 只有 +20%，不构成换嘲理由。**不要**为双子配置 `npcbot_tank_swap` 规则。
- 坦克职责分配：在 BOT 队伍内按 `ObjectGuid` 升序取坦克序号，0 → 光 boss，1 → 暗 boss。若只有 1 个坦克，则该坦克固定拉光 boss，暗 boss 交给"威胁最高的近战"（脚本 `SelectNearestTarget(200.f)` 会自动选中，无需干预）。

### 3.7 Vortex / Touch / 狂暴

| 机制 | 策略 |
|---|---|
| Light/Dark Vortex | 同色精华减伤，BOT 不做额外动作。若实测存在推拉位移，再补"保持原位"（`BOT_COMMAND_STAY` 式驻守） |
| Touch of Light/Darkness | 精华常驻即免疫，无额外动作。**注意不要**让治疗把 Touch 当成普通魔法 DOT 反复驱散（`CureGroup`，`bot_ai.cpp:1767`），否则治疗 GCD 会被吃光；若实测出现，在 `_getBotDispellableAuraList`（`:1913`）里把 Touch ID 加入黑名单 |
| 狂暴 | 不处理 |

---

## 4. 实现方案

### 4.1 常量与数据（`botcommon.h`）

```cpp
// ===== 十字军的试炼：瓦格里双子（map 649） =====
enum BotTwinValkyrEntries : uint32
{
    CREATURE_TOC_FJOLA_LIGHTBANE       = 34497, // 光 boss
    CREATURE_TOC_EYDIS_DARKBANE        = 34496, // 暗 boss
    CREATURE_TOC_LIGHT_ESSENCE_NPC     = 34568, // 光之精华（gossip NPC）
    CREATURE_TOC_DARK_ESSENCE_NPC      = 34567, // 暗之精华（gossip NPC）
    CREATURE_TOC_CONCENTRATED_LIGHT    = 34630, // 光球
    CREATURE_TOC_CONCENTRATED_DARK     = 34628, // 暗球
};

// 精华颜色
enum BotTwinValkyrColor : uint8
{
    BOT_VALKYR_COLOR_NONE  = 0,
    BOT_VALKYR_COLOR_LIGHT = 1,
    BOT_VALKYR_COLOR_DARK  = 2,
};

// 精华 spell：[难度][0 = 暗, 1 = 光]
// 难度顺序与 Player::GetDifficulty() 一致：0 = 10N / 1 = 25N / 2 = 10H / 3 = 25H
uint32 const VALKYR_ESSENCE_SPELL[4][2] =
{
    { 65684, 65686 },
    { 67176, 67222 },
    { 67177, 67223 },
    { 67178, 67224 },
};

uint32 const SPELL_TOC_LIGHT_SHIELD = 65858; // Twin Pact 光盾（175000 全学派吸收）
uint32 const SPELL_TOC_DARK_SHIELD  = 65874; // Twin Pact 暗盾
```

### 4.2 改动落点

| 文件 | 位置 | 改动 |
|---|---|---|
| `botcommon.h` | entry / spell 常量区 | 4.1 的常量与颜色枚举 |
| `bot_ai.h` | 成员区、方法声明区 | `_valkyrColor`、`_valkyrTacticTimer`、`_valkyrEssenceTimer`、`_valkyrTankIndex`、`UpdateTwinValkyrTactic()`、`GetTwinValkyrColor()`、`RefreshValkyrEssence()`、`SelectTwinValkyrTarget()`、`IsTwinValkyrFight()` |
| `bot_ai.cpp` | `GlobalUpdate()` `:19234` 之后 | `UpdateTwinValkyrTactic(diff);` |
| `bot_ai.cpp` | `UpdateTankSwap()` 之后（约 `:2084`） | 四个新函数的实现（与换嘲模块并列，保持"BOSS 战术"集中在一处） |
| `bot_ai.cpp` | `_getTargets()` map 分支 `:4204` | 双子目标重定向 |
| `bot_ai.cpp` | `CanBotAttack()` `:3828` | 双子战斗中排除球体与已死亡的姐妹 boss |
| `bot_ai.cpp` | `CalculateAoeSpots()` 649 块 `:5535` | 按颜色区分同色 / 异色球 |
| `bot_ai.cpp` | `CalculateAttackPos()` `:5908` 末尾 | 站位锚点修正 |
| `botconfig.h/.cpp` | 配置项 | `NpcBot.TwinValkyrTactic.Enable`、`NpcBot.TwinValkyrTactic.EssenceOverride`、`NpcBot.TwinValkyrTactic.EatOrb` |
| `data/sql/updates/pending_db_world/` | 新增 `rev_*.sql` | P0 的球体危险区（P1 上线后删除） |

### 4.3 核心伪代码

```cpp
// 是否是双子战斗：545 必须在场且存活，且自身在战斗中
bool bot_ai::IsTwinValkyrFight() const
{
    if (me->GetMapId() != 649 || !me->IsInCombat())
        return false;
    // 复用范围搜索，不依赖实例脚本 GUID
    return FindCreatureInRange(CREATURE_TOC_FJOLA_LIGHTBANE, 200.f) ||
           FindCreatureInRange(CREATURE_TOC_EYDIS_DARKBANE, 200.f);
}

// 颜色分配：只在首次进入战斗时计算，结果缓存
uint8 bot_ai::GetTwinValkyrColor()
{
    if (_valkyrColor)
        return _valkyrColor;

    // 1) 统计真人已有颜色（4 个难度的光 / 暗精华都要认）
    uint32 lightCount = 0, darkCount = 0;
    if (Group* gr = master->GetGroup())
        for (GroupReference* itr = gr->GetFirstMember(); itr; itr = itr->next())
            if (Player* pl = itr->GetSource())
            {
                if (pl->HasAnyAuraOf(VALKYR_LIGHT_IDS)) ++lightCount;
                else if (pl->HasAnyAuraOf(VALKYR_DARK_IDS)) ++darkCount;
            }

    // 2) 坦克不参与补位：按坦克序号固定颜色
    if (IsTank())
        return _valkyrColor = (_valkyrTankIndex == 0) ? BOT_VALKYR_COLOR_LIGHT : BOT_VALKYR_COLOR_DARK;

    // 3) 其余 BOT 按 guid 排序后向人少的一侧补位（1:1 收敛）
    //    实现时把同一 master 名下所有在战斗中的 BOT 收集并按 guid 升序排序，
    //    逐个分配：当前哪边少就分到哪边，持平则交替。
    return _valkyrColor = PickValancedColor(lightCount, darkCount);
}

// 精华常驻：1s 节流
void bot_ai::RefreshValkyrEssence()
{
    uint8 diff = std::min<uint8>(uint8(me->GetMap()->GetDifficulty()), 3);
    uint32 want  = VALKYR_ESSENCE_SPELL[diff][_valkyrColor == BOT_VALKYR_COLOR_DARK ? 0 : 1];
    uint32 other = VALKYR_ESSENCE_SPELL[diff][_valkyrColor == BOT_VALKYR_COLOR_DARK ? 1 : 0];

    if (me->HasAura(want))
        return;

    // 与玩家点传送门的行为保持一致：先移除相反色，再挂目标色
    if (me->HasAura(other))
        me->RemoveAura(other);
    me->CastSpell(me, want, true);
}

// 战术主入口：由 GlobalUpdate 每帧调用，内部自行节流
void bot_ai::UpdateTwinValkyrTactic(uint32 diff)
{
    if (!BotCfg::IsTwinValkyrTacticEnabled())
        return;
    if (!me->IsAlive() || IAmFree())
        return;
    if (me->GetMapId() != 649)
    {
        _valkyrColor = BOT_VALKYR_COLOR_NONE; // 离开地图即重置
        return;
    }
    if (!IsTwinValkyrFight())
    {
        _valkyrColor = BOT_VALKYR_COLOR_NONE; // 战斗结束即重置，下次重开重新分配
        return;
    }

    GetTwinValkyrColor();          // 首次进战斗时计算并缓存

    if (_valkyrEssenceTimer <= diff)
    {
        _valkyrEssenceTimer = 1000; // 1s 重试，不使用每帧施法
        RefreshValkyrEssence();
    }
    else
        _valkyrEssenceTimer -= diff;
}

// 目标重定向
Creature* bot_ai::SelectTwinValkyrTarget()
{
    Creature* lightbane = FindCreatureInRange(CREATURE_TOC_FJOLA_LIGHTBANE, 200.f);
    Creature* darkbane  = FindCreatureInRange(CREATURE_TOC_EYDIS_DARKBANE, 200.f);

    if (!lightbane || !lightbane->IsAlive()) return darkbane;
    if (!darkbane  || !darkbane->IsAlive())  return lightbane;

    // Twin Pact 护盾：优先打没盾的一只（175000 全学派吸收，打有盾的等于白打）
    bool lightShielded = lightbane->HasAura(SPELL_TOC_LIGHT_SHIELD);
    bool darkShielded  = darkbane->HasAura(SPELL_TOC_DARK_SHIELD);
    if (lightShielded != darkShielded)
        return lightShielded ? darkbane : lightbane;

    // 坦克按职责，DPS 按"与精华相反色"
    if (IsTank())
        return (_valkyrTankIndex == 0) ? lightbane : darkbane;
    return (_valkyrColor == BOT_VALKYR_COLOR_DARK) ? lightbane : darkbane;
}
```

`CalculateAoeSpots()` 649 块新增：

```cpp
// 瓦格里双子：球体（Concentrated Light / Dark）
// 同色球会被自身精华吸收，不构成威胁；异色球接触即爆炸，按 20 码危险区绕行
if (Creature* lightbane = /* 双子在场判定 */)
{
    auto ballCheck = [this](Unit const* u) {
        uint32 entry = u->GetEntry();
        if (entry != CREATURE_TOC_CONCENTRATED_LIGHT && entry != CREATURE_TOC_CONCENTRATED_DARK)
            return false;
        bool isLightBall = (entry == CREATURE_TOC_CONCENTRATED_LIGHT);
        // 无精华兜底：两种球都躲
        if (_valkyrColor == BOT_VALKYR_COLOR_NONE)
            return true;
        return isLightBall != (_valkyrColor == BOT_VALKYR_COLOR_LIGHT); // 只保留异色球
    };

    std::list<Creature*> ballList;
    Bcore::CreatureListSearcher ballSearcher(unit, ballList, ballCheck);
    Cell::VisitObjects(unit, ballSearcher, 60.f);

    for (Creature const* c : ballList)
        spots.emplace_back(*c, 20.0f + DEFAULT_COMBAT_REACH * 2.0f);
}
```

---

## 5. 分期

### P0 —— 零代码兜底

把两种球配进 `npcbot_creature_hazard`（SQL 见第 6 节，**设计稿，暂不执行**）。BOT 会绕开所有球，不会炸团，但也吃不到 Empowered。**P1 上线后必须删除这两行**，否则同色球仍会被 DB 规则塞回危险区。

### P1 —— 最小可用战术（推荐一次做完，约 250~300 行）

1. 常量 + 配置开关
2. `UpdateTwinValkyrTactic` 钩子 + 精华代施
3. 颜色分配（含真人补位、坦克例外）
4. 目标重定向（含 Twin Pact 护盾判断）
5. 排除打卡球
6. 站位锚点（贴精华 NPC 一侧）
7. 从 `npcbot_creature_hazard` 删除 P0 的两行

### P2 —— 增强

1. 同色球主动吃（**需先改 `boss_twin_valkyr.cpp` 的引爆判定，否则 BOT 撞不爆球**；默认关闭）
2. 吃到 Empowered 后强制打相反 boss（当前逻辑已天然一致，仅需日志确认）
3. 战斗中途真人改色时的动态再平衡（每 10s 检查一次比例，偏差 > 2 时重新分配）
4. Touch 驱散黑名单

---

## 6. SQL 配置（P0，设计稿，未应用）

> 以下 SQL 只是设计说明，**没有**写入 `data/sql/updates/pending_db_world/`，也没有改动
> `outputs/NPCBot/机制处理/危险区域识别.sql`（该文件保持原样）。待方案评审通过、进入实现阶段后，
> 再按约定用 `create_sql.sh` 生成 `rev_<timestamp>.sql` 落到 `pending_db_world`。

```sql
-- ============================================================================
-- 十字军试炼：瓦格里双子 —— 能量球（Concentrated Light / Dark）
-- ============================================================================
-- 球体被 npc_concentrated_ballAI 驱动，与玩家距离 <= 2.75 码时引爆，
-- 施放 65795（光）/ 65808（暗）的范围伤害（半径索引 29）。
-- 同色精华会吸收该伤害，异色则全额承伤；P0 阶段无法区分同色，统一按危险源处理。
-- 注意：P1 实现"按精华颜色区分同色/异色"后，必须删除这两行配置。
DELETE FROM `npcbot_creature_hazard`
WHERE `map_id` = 649 AND `creature_entry` IN (34628, 34630);

INSERT INTO `npcbot_creature_hazard`
(`map_id`, `creature_entry`, `radius`, `damage_spell_id`, `safety_distance`, `deactivation_delay_ms`, `comment`)
VALUES
(649, 34630, 20.0, 65795, 2.0, 0, 'TOC双子：Concentrated Light 光球（引爆范围伤害，P1 后按精华颜色区分）'),
(649, 34628, 20.0, 65808, 2.0, 0, 'TOC双子：Concentrated Darkness 暗球（引爆范围伤害，P1 后按精华颜色区分）');
```

> `damage_spell_id` 非 0 时，`NPCBotHazardMgr::CollectCreatureHazards` 会优先按该 spell 的实际半径计算（`Hazards/NPCBotHazardMgr.cpp:214` 起），`radius` 仅作兜底。若实测爆炸半径与 20 码差异较大，直接改 `radius` 或调整 spell 半径索引的影响。

---

## 7. 验证与观测

| 验证项 | 方法 |
|---|---|
| BOT 能否挂上精华 | `.npcbot` 选中 BOT → 用 GM 命令查看光环；或在 `RefreshValkyrEssence` 施法后打 `BOT_LOG_DEBUG("npcbots.toc", ...)` 观察连续失败次数 |
| 颜色是否 1:1 | 战斗开始 5s 后统计两侧 BOT 数量，写入 DEBUG 日志 |
| 是否正确选目标 | 观察 BOT 是否出现"暗精华打光 boss"的分兵；Twin Pact 出现时有盾目标被立刻放弃 |
| 是否打球 | 战斗日志中 BOT 不应出现对 34628/34630 的攻击记录 |
| 是否躲异色球 | 与同色球对比，异色球引爆时 BOT 应在 20 码外 |

建议在 `#ifdef DEBUG` 或 `BotCfg` 里加一个 `.npcbot toc` 调试命令，打印当前颜色、精华 ID、当前目标与站位锚点，便于线上排查。

---

## 8. 风险与边界

1. **精华对 Creature 是否生效（最高风险）**。精华是玩家向的 gossip 奖励，理论上对任意 Unit 的 SCHOOL_ABSORB 都应生效，但本项目未经实测。**实现第一步就是用 GM 命令给一个 BOT 手动加 65686，确认以下三点**：光环是否挂上、吃到光球后 Powering Up 是否叠层、Touch 是否减免。若受限，退化为"无精华模式"（仅目标重定向 + 站位 + 躲球），仍能打通，只是拿不到 Empowered 增伤。
2. **25 人难度精华 ID**。`OnGossipHello` 的光门固定 cast 65686（`boss_twin_valkyr.cpp:621`），与 `DoAction(-1)` 的清理列表（67176/67222 等）不一致。BOT 代施按 4.1 的表取难度对应 ID；若实测 25 人下光精华挂不上，回头检查该处脚本是否本就有 bug。
3. **颜色抖动**。颜色必须缓存到"战斗结束"再清，绝不能每 tick 重算 —— 反复切换会清空 Powering Up 层数。
4. **真人干扰**。真人中途点门改色会打破 1:1 比例；P1 阶段不做动态再平衡，接受偏差。
5. **`DoAction(-1)` 清空**。战斗结束会重清精华（`:187-192`），下一场必须重新挂 —— 这也是"周期重试"而非"一次性施法"的原因。
6. **与换坦系统的冲突**。不要给双子配 `npcbot_tank_swap` 规则：Twin Spike 不需要换嘲，配了反而会导致两个坦克互相嘲来嘲去、把 boss 拉离预定站位。
7. **球体的危险区域与吃球互斥**。只要 DB 里还留着球体规则，同色球也会被绕开，P2 的吃球永远不会触发。
