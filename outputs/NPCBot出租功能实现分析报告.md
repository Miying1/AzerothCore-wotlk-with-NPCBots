# NPCBot 机器人出租功能实现分析报告

## 一、系统概述

NPCBot 出租系统由 Trickerer 开发，**直接集成在核心源码** `src/server/game/AI/NpcBots/` 中，不是独立模块。系统包含两个费用维度：

| 费用类型 | 配置项 | 默认值 | 说明 |
|---------|--------|--------|------|
| 雇佣费 (Hire) | `NpcBot.Cost.Hire` | 1000000 (100金) | 一次性支付，雇佣时扣除 |
| 租金 (Rent) | `NpcBot.Cost.Rent` | 0 (关闭) | 按小时计费，每10分钟扣除一次 |

**关键常量** (`botcommon.h`):
- `RENT_TIMER = 3600000` — 租金周期：1小时
- `RENT_COLLECT_TIMER = 600000` — 收租间隔：10分钟

---

## 二、核心文件与职责

| 文件 | 职责 |
|------|------|
| `botconfig.h / .cpp` | 配置加载、费用计算 (`GetNpcBotCostHire/Rent`) |
| `botgiver.cpp` | Bot Giver NPC 的 gossip 交互（雇佣入口） |
| `botmgr.cpp` | `AddBot()` — 雇佣核心逻辑、费用扣除 |
| `bot_ai.cpp` | `SetBotOwner()` — 设置主人；租金定时收取逻辑 |
| `bot_ai.h` | `_rentTimer` 成员声明 |
| `botdatamgr.h / .cpp` | `NpcBotData` 结构体、数据库读写 |
| `botcommon.h` | 租金相关常量 |
| `worldserver.conf.dist` | 配置项文档与默认值 |

---

## 三、雇佣流程详解

### 3.1 通过 Bot Giver NPC 雇佣 (`botgiver.cpp`)

```
玩家 → 与 Bot Giver NPC 对话
  → HIRE: 检查数量上限（玩家/账号/IP/职业）
    → HIRE_CLASS: 选择职业，显示可用 bot 列表 + 费用
      → HIRE_ENTRY: 选择具体 bot
        → bot_ai::OnGossipSelect(GOSSIP_SENDER_HIRE)
          → SetBotOwner(player)
            → BotMgr::AddBot(bot)  ← 扣除雇佣费
```

### 3.2 直接与 Bot 对话雇佣 (`bot_ai.cpp` ~L7822)

玩家也可以直接右键点击自由 bot 进行雇佣，走相同的费用检查和 `SetBotOwner` 路径。

### 3.3 雇佣时的费用扣除 (`botmgr.cpp` L926-940)

```cpp
if (!owned)
{
    uint32 cost = BotCfg::GetNpcBotCostHire(_owner->GetLevel(), bot->GetBotClass());
    if (!_owner->HasEnoughMoney(cost))
        return BOT_ADD_CANT_AFFORD;
    _owner->ModifyMoney(-(int32(cost)));  // 扣除雇佣费
}
```

### 3.4 费用计算公式 (`botconfig.cpp` L1371-1408)

`_normalizedCostForLevel(cost_base, bot_class, level)`:

| 等级区间 | 计算公式 (以 cost_base=1000000 为例) |
|---------|--------------------------------------|
| 1-9 | cost_base / 2000 = 5银 |
| 10-19 | cost_base / 100 = 1金 |
| 20-29 | cost_base / 20 = 5金 |
| 30-39 | cost_base / 5 = 20金 |
| 40+ | cost_base × (level - level%10) / 80，线性递增到100金 |

**职业倍率**：
- 标准职业 (战/骑/猎/贼/牧/死骑/萨/法/术/德)：1x
- BM/Archmage/Spellbreaker/Necromancer：**2x**
- Sphynx/Dreadlord/DarkRanger/SeaWitch/CryptLord：**5x**

---

## 四、租金收取机制详解

### 4.1 租金计时器累积 (`bot_ai.cpp` L18694-18695)

```cpp
// 在 UpdateAI 中，每个 tick 累积
if (BotCfg::GetNpcBotCostRent(me->GetLevel(), GetBotClass()) && me->IsInWorld()
    && !HasBotCommandState(BOT_COMMAND_UNBIND) && !IAmFree())
    _rentTimer += diff;
```

### 4.2 租金扣除 (`bot_ai.cpp` L17837-17855)

每 10 分钟（`RENT_COLLECT_TIMER`）执行一次：

```cpp
uint32 rent_cost = BotCfg::GetNpcBotCostRent(master->GetLevel(), GetBotClass());
if (_rentTimer >= RENT_COLLECT_TIMER && rent_cost && ...)
{
    uint32 rent_money = 0;
    while (_rentTimer >= RENT_COLLECT_TIMER)
    {
        // 按比例计算：rent_cost × (600秒 / 3600秒) = rent_cost / 6
        rent_money += uint32(uint64(rent_cost) * (RENT_COLLECT_TIMER / 1000) / (RENT_TIMER / 1000));
        _rentTimer -= RENT_COLLECT_TIMER;
    }
    rent_money = std::max<uint32>(rent_money, 1);  // 至少1铜

    if (!master->HasEnoughMoney(rent_money))
    {
        // 玩家付不起租金 → 移除 bot
        master->GetBotMgr()->RemoveBot(me->GetGUID(), BOT_REMOVE_UNAFFORD);
        return false;
    }
    master->ModifyMoney(-int32(rent_money));  // 扣除租金
}
```

### 4.3 租金重置

- Bot 被解除拥有权时：`_rentTimer = 0`（`ResetBotAI` 中 `BOTAI_RESET_MASK_ABANDON_MASTER`）
- Bot 处于 `BOT_COMMAND_UNBIND` 状态时不累积租金

---

## 五、数据库结构

### `characters_npcbot` 表（acore_characters 库）

从 `botdatamgr.cpp` L1027-1033 的查询可知字段：

```
entry, owner, roles, spec, faction, hire_time (UNIX_TIMESTAMP),
shared_owners, equipMhEx, equipOhEx, equipRhEx, equipHead, equipShoulders,
equipChest, equipWaist, equipLegs, equipFeet, equipWrist, equipHands,
equipBack, equipBody, equipFinger1, equipFinger2, equipTrinket1, equipTrinket2,
equipNeck, spells_disabled, miscvalues
```

### `NpcBotData` 内存结构 (`botdatamgr.h` L87-110)

```cpp
struct NpcBotData
{
    uint32 owner;           // 玩家 GUID low
    uint64 hire_time;       // 雇佣时间戳
    uint32 roles;           // 角色定位 (坦克/治疗/DPS)
    uint32 faction;
    uint8 spec;
    std::array<uint32, BOT_INVENTORY_SIZE> equips;
    DisabledSpellsContainer disabled_spells;
    MiscValuesContainer miscvalues;      // map<uint32, uint32> 任意键值对
    SharedOwnersContainer shared_owners; // set<uint32>
};
```

### Owner 更新 (`botdatamgr.cpp` L2955-2967)

```cpp
case NPCBOT_UPDATE_OWNER:
    itr->second.owner = *(uint32*)(data);
    itr->second.hire_time = itr->second.owner ? uint64(std::time(0)) : 1ULL;
    // SQL: UPDATE characters_npcbot SET owner = ?, hire_time = FROM_UNIXTIME(?) WHERE entry = ?
```

---

## 六、限制机制

雇佣时进行以下检查（`botgiver.cpp` + `botmgr.cpp`）：

| 检查项 | 配置项 | 默认值 | 说明 |
|--------|--------|--------|------|
| 玩家最大 bot 数 | `NpcBot.MaxBots` | 39/级档 | 按等级分9档 (0-9,10-19,...,80+) |
| 每职业最大数 | `NpcBot.MaxBotsPerClass` | 0 (无限) | 每种职业最多雇佣数量 |
| 账号最大 bot 数 | `NpcBot.MaxBotsPerAccount` | 0 (无限) | 整个账号上限 |
| IP 最大 bot 数 | `NpcBot.IpMaxBots` | 8 | 同一IP地址上限 (GM豁免) |
| 所有权过期 | `NpcBot.OwnershipExpireTime` | 0 (关闭) | 秒，过期自动解除拥有权 |

---

## 七、当前系统的关键局限

**当前系统的费用配置是完全全局化的：**

1. `NpcBot.Cost.Hire` 和 `NpcBot.Cost.Rent` 是**全局单一值**，对所有 bot 统一生效
2. `_normalizedCostForLevel()` 仅按**等级区间**和**职业**区分费用，不区分个体
3. **不存在**任何 per-bot（按 entry）的费用覆盖机制
4. **不存在**"禁止某个 bot 被出租"的标记

---

## 八、实现 per-bot 出租控制与自定义租金的方案分析

### 方案 A：扩展 `characters_npcbot` 表（推荐）

**原理**：在 `characters_npcbot` 表中新增 per-bot 配置字段。

**数据库变更**：
```sql
ALTER TABLE characters_npcbot
  ADD COLUMN rentable TINYINT(1) NOT NULL DEFAULT 1 COMMENT '是否可出租',
  ADD COLUMN cost_hire_override INT(10) UNSIGNED DEFAULT NULL COMMENT '自定义雇佣费(铜), NULL=使用全局',
  ADD COLUMN cost_rent_override INT(10) UNSIGNED DEFAULT NULL COMMENT '自定义租金(铜), NULL=使用全局';
```

**代码改动点**：

1. **`botdatamgr.h` — `NpcBotData` 结构体**：新增 `rentable`、`costHireOverride`、`costRentOverride` 字段

2. **`botdatamgr.cpp` — `LoadNpcBots()`**：SQL 查询增加新字段读取

3. **`botdatamgr.cpp` — `AddNpcBotData()`**：INSERT 语句增加新字段

4. **`botconfig.h / .cpp`**：`GetNpcBotCostHire()` 和 `GetNpcBotCostRent()` 增加 `uint32 entry` 参数，先查 per-bot 覆盖值，无覆盖则回退全局值
   ```cpp
   static uint32 GetNpcBotCostHire(uint8 level, uint8 botclass, uint32 entry = 0);
   static uint32 GetNpcBotCostRent(uint8 level, uint8 botclass, uint32 entry = 0);
   ```

5. **`botgiver.cpp`**：在遍历可用 bot 时，跳过 `rentable == 0` 的 bot

6. **`botmgr.cpp` — `AddBot()`**：传入 entry 获取 per-bot 费用

7. **`bot_ai.cpp`**：
   - 雇佣 gossip 检查 `rentable`
   - 租金收取使用 per-bot rent 值
   - `_rentTimer` 累积条件改为检查 per-bot rent

8. **`botcommands.cpp`**：新增 GM 命令 `.npcbot rentable <entry> <0|1>` 和 `.npcbot cost <entry> <hire> <rent>`

**优点**：数据持久化、支持运行时修改、架构清晰
**缺点**：需改动较多文件（约8个）、需数据库迁移

### 方案 B：利用现有 `miscvalues` 字段（低侵入）

**原理**：`NpcBotData.miscvalues` 是 `map<uint32, uint32>`，已支持任意键值对存储，无需改表结构。

**预留键**：
- Key `100` = `rentable` (0/1)
- Key `101` = `cost_hire_override`
- Key `102` = `cost_rent_override`

**代码改动**：
- `BotCfg::GetNpcBotCostHire/Rent()` 无法直接访问 `NpcBotData`（静态方法），需改为通过 `bot_ai` 或 `BotDataMgr` 查询
- 在 `bot_ai.cpp` 中封装 `GetCostHire()` / `GetCostRent()` 实例方法，先查 miscvalues 再回退全局

**优点**：零数据库迁移、改动量小
**缺点**：不够直观、键名魔法数字、miscvalues 已有其他用途需确认无冲突

### 方案 C：World DB 的 `creature_template_npcbot` 表

**原理**：在 world 库的 bot 模板表中添加配置字段，使费用与 bot 模板绑定而非运行时状态。

**适用场景**：如果费用应与 creature entry 绑定（所有同名 bot 费用一致），而非每个个体独立。

**优点**：world DB 管理方便、GM 可通过 SQL 批量调整
**缺点**：无法对同一 entry 的不同个体设置不同费用（但通常也不需要）

### 方案推荐

| 需求场景 | 推荐方案 |
|---------|---------|
| 需要对每个 bot 个体独立控制 | 方案 A |
| 快速实现、接受魔法数字 | 方案 B |
| 按 creature entry 批量控制即可 | 方案 C |

**综合推荐方案 A**，因为它最符合项目的工程规范，数据持久化清晰，且与现有 `owner`/`roles`/`spec` 等字段的模式一致。

---

## 九、方案 A 的关键代码改动示意

### 9.1 NpcBotData 扩展

```cpp
// botdatamgr.h
struct NpcBotData
{
    // ... 现有字段 ...
    bool rentable = true;
    Optional<uint32> costHireOverride;
    Optional<uint32> costRentOverride;
};
```

### 9.2 费用查询入口

```cpp
// bot_ai.h / bot_ai.cpp
uint32 bot_ai::GetCostHire(uint8 level) const
{
    if (_botData->costHireOverride)
        return *_botData->costHireOverride;
    return BotCfg::GetNpcBotCostHire(level, _botclass);
}

uint32 bot_ai::GetCostRent(uint8 level) const
{
    if (!_botData->rentable)
        return 0;  // 不可出租则租金为0（也不可被雇佣）
    if (_botData->costRentOverride)
        return *_botData->costRentOverride;
    return BotCfg::GetNpcBotCostRent(level, _botclass);
}
```

### 9.3 雇佣可用性检查

```cpp
// botgiver.cpp - 遍历可用 bot 时
if (!ai->IsRentable())
    continue;  // 跳过不可出租的 bot
```

### 9.4 GM 命令

```
.npcbot rentable <entry> <0|1>     — 设置是否可出租
.npcbot costhire <entry> <copper>  — 设置自定义雇佣费
.npcbot costrent <entry> <copper>  — 设置自定义租金
```

---

## 十、总结

当前 NPCBot 出租系统是一个**全局配置驱动**的完整实现，涵盖雇佣费一次性扣除和租金定时收取两大机制。系统的费用计算基于等级区间和职业倍率，但不支持 per-bot 级别的费用定制或出租控制。

要实现 per-bot 的出租控制和自定义租金，推荐通过**扩展 `characters_npcbot` 表**的方式，在 `NpcBotData` 中增加 `rentable`、`cost_hire_override`、`cost_rent_override` 字段，并在费用查询入口、bot giver 遍历、AddBot 扣费、租金收取等环节进行适配。改动涉及约8个源文件，均为增量修改，不破坏现有行为。
