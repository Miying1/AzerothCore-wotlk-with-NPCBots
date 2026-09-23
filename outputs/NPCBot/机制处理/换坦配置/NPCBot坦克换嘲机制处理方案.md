# NPCBot 坦克换嘲机制处理方案

> 面向落地实现的详细设计。所有函数名、类名、技能 ID、加载点均与当前代码库对齐，可直接照此实施。

## 1. 目标

为 NPCBot 增加可由 World 数据库配置的坦克换嘲机制：

- 当 BOSS 当前攻击目标身上的换嘲 Aura 达到指定层数时，由正在攻击该 BOSS 的另一名坦克嘲讽 BOSS，实现自动换嘲。

换嘲只依赖「当前 BOT 正在攻击的 BOSS + BOSS 当前攻击目标 + Aura 层数」这一组关系判断，不要求 BOSS 当前目标正在攻击 BOSS，不引入地图扫描，不硬编码到职业 AI 的既有嘲讽逻辑中。

---

## 2. 总体设计

复用现有 `NPCBotHazardMgr` 的单例 + 加载 + 查询模式，新增独立管理器 `NPCBotTankSwapMgr`：

| 模块 | 说明 |
|---|---|
| `NPCBotTankSwapMgr` | 单例，负责从 `npcbot_tank_swap` 表加载规则、按 `(map_id, boss_entry)` 查询 |
| `bot_ai::UpdateTankSwap()` | 每帧由 `GlobalUpdate` 调用，判断当前 BOT / BOSS / BOSS 目标关系并触发换嘲 |
| `bot_ai::CanCastConfiguredTaunt()` | 准入判断：嘲讽技能是否可用（未禁用、未在 CD） |
| `bot_ai::IsConfiguredTankSwapBoss()` | 拦截普通嘲讽逻辑，防止绕过配置机制 |
| `bot_ai::CastConfiguredTaunt()` | 虚接口，由各职业 AI 实现具体嘲讽技能 |

关键差异点：换嘲机制**直接使用 `me->GetVictim()` 定位 BOSS**，无需像危险区域那样做 `Cell::VisitObjects` 扫描——因为换嘲只发生在「当前正在攻击的 BOSS」身上。

---

## 3. 数据库配置表

### 3.1 建表 SQL

新增 World 数据库专用表 `npcbot_tank_swap`：

```sql
CREATE TABLE IF NOT EXISTS `npcbot_tank_swap` (
  `map_id` SMALLINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '地图ID，0表示所有地图',
  `boss_entry` INT UNSIGNED NOT NULL COMMENT 'BOSS生物Entry',
  `spell_id` INT UNSIGNED NOT NULL COMMENT 'BOSS施加的换嘲Aura或技能ID',
  `aura_stacks` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '触发换嘲所需BUFF层数',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '是否启用',
  `comment` VARCHAR(255) NOT NULL DEFAULT '' COMMENT '配置说明',
  PRIMARY KEY (`map_id`, `boss_entry`, `spell_id`),
  KEY `idx_boss_entry` (`boss_entry`),
  KEY `idx_spell_id` (`spell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='NPCBot坦克换嘲配置';
```

### 3.2 字段说明

| 字段 | 类型 | 说明 |
|---|---|---|
| `map_id` | SMALLINT UNSIGNED | 地图 ID；`0` 表示所有地图通用 |
| `boss_entry` | INT UNSIGNED | BOSS 的 Creature Entry，不使用实例 GUID |
| `spell_id` | INT UNSIGNED | BOSS 施加的换嘲 Aura（Debuff）ID |
| `aura_stacks` | TINYINT UNSIGNED | 触发换嘲所需的最小 BUFF 层数 |
| `enabled` | TINYINT UNSIGNED | `1` 启用、`0` 禁用（加载时跳过） |
| `comment` | VARCHAR(255) | 中文配置说明 |

`boss_entry` 使用 Creature Entry 而非 GUID：实例中的 BOSS GUID 每次可能不同，Entry 才适合通用配置。

### 3.3 数据库更新文件

按 `AGENTS.md` 规范，新增到 `data/sql/updates/pending_db_world/`，命名遵循现有 `rev_<时间戳>.sql` 约定，例如：

```text
data/sql/updates/pending_db_world/rev_20260923_00.sql
```

文件内容（建表 + 可选示例，示例保留 DELETE 幂等语义）：

```sql
-- NPCBot 坦克换嘲机制配置表
CREATE TABLE IF NOT EXISTS `npcbot_tank_swap` (
  `map_id` SMALLINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '地图ID，0表示所有地图',
  `boss_entry` INT UNSIGNED NOT NULL COMMENT 'BOSS生物Entry',
  `spell_id` INT UNSIGNED NOT NULL COMMENT 'BOSS施加的换嘲Aura或技能ID',
  `aura_stacks` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '触发换嘲所需BUFF层数',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '是否启用',
  `comment` VARCHAR(255) NOT NULL DEFAULT '' COMMENT '配置说明',
  PRIMARY KEY (`map_id`, `boss_entry`, `spell_id`),
  KEY `idx_boss_entry` (`boss_entry`),
  KEY `idx_spell_id` (`spell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='NPCBot坦克换嘲配置';
```

---

## 4. 配置示例

### 4.1 坦克两层换嘲

一名坦克 BOT（T2）正在攻击该 BOSS，BOSS 当前攻击目标（可为任意单位，不要求是坦克或 BOT）身上带有换嘲 Aura `21414`；当该目标身上的 Aura 达到 2 层时，由 T2 嘲讽 BOSS：

```sql
DELETE FROM `npcbot_tank_swap`
WHERE `map_id` = 531
  AND `boss_entry` = 41413
  AND `spell_id` = 21414;

INSERT INTO `npcbot_tank_swap`
    (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`)
VALUES
    (531, 41413, 21414, 2, 1, '坦克两层换嘲');
```

含义：

```text
地图 531
BOSS Entry 41413
BOSS 当前攻击目标身上的 Aura 21414 达到 2 层
由正在攻击该 BOSS 的坦克 BOT 嘲讽 BOSS
```

---

## 5. 数据管理器 `NPCBotTankSwapMgr`

### 5.1 头文件

新建 `src/server/game/AI/NpcBots/Hazards/NPCBotTankSwapMgr.h`：

```cpp
/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef NPCBOT_TANK_SWAP_MGR_H
#define NPCBOT_TANK_SWAP_MGR_H

#include "../botcommon.h"

#include <unordered_map>
#include <vector>

struct BotTankSwapRule
{
    uint32 MapId;
    uint32 BossEntry;
    uint32 SpellId;
    uint8 AuraStacks;
};

class NPCBotTankSwapMgr
{
public:
    static NPCBotTankSwapMgr* instance();

    void LoadFromDB();

    // 返回指定地图下该 BOSS 的全部换嘲规则（无配置返回 nullptr）
    std::vector<BotTankSwapRule> const* GetRules(uint32 mapId, uint32 bossEntry) const;
    bool HasRule(uint32 mapId, uint32 bossEntry) const;

private:
    // key: bossEntry -> 该 BOSS 的换嘲规则列表（允许一个 BOSS 配置多个换嘲 Aura）
    using TankSwapRulesByBoss = std::unordered_map<uint32, std::vector<BotTankSwapRule>>;

    NPCBotTankSwapMgr() = default;

    std::unordered_map<uint32, TankSwapRulesByBoss> _rulesByMap; // key: mapId
    TankSwapRulesByBoss _globalRules;                            // mapId == 0 的全局配置
};

#define sNPCBotTankSwapMgr NPCBotTankSwapMgr::instance()

#endif
```

### 5.2 实现文件

新建 `src/server/game/AI/NpcBots/Hazards/NPCBotTankSwapMgr.cpp`：

```cpp
/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "NPCBotTankSwapMgr.h"

#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"

NPCBotTankSwapMgr* NPCBotTankSwapMgr::instance()
{
    static NPCBotTankSwapMgr instance;
    return &instance;
}

void NPCBotTankSwapMgr::LoadFromDB()
{
    uint32 oldMSTime = getMSTime();
    uint32 loadedCount = 0;
    uint32 skippedCount = 0;
    std::unordered_map<uint32, TankSwapRulesByBoss> rulesByMap;
    TankSwapRulesByBoss globalRules;

    QueryResult result = WorldDatabase.Query(
        "SELECT map_id, boss_entry, spell_id, aura_stacks, enabled FROM npcbot_tank_swap");
    if (!result)
    {
        _rulesByMap.clear();
        _globalRules.clear();
        LOG_INFO("server.loading", ">> Loaded 0 NPCBot tank swap definitions. DB table `npcbot_tank_swap` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        uint32 mapId = fields[0].Get<uint16>();
        uint32 bossEntry = fields[1].Get<uint32>();
        uint32 spellId = fields[2].Get<uint32>();
        uint8 auraStacks = fields[3].Get<uint8>();
        bool enabled = fields[4].Get<uint8>() != 0;

        // enabled = 0 的配置不参与运行时处理
        if (!enabled)
            continue;

        // 校验：地图有效 + BOSS Entry 存在
        if ((mapId && !sMapStore.LookupEntry(mapId)) || !sObjectMgr->GetCreatureTemplate(bossEntry))
        {
            ++skippedCount;
            LOG_ERROR("sql.sql", "Table `npcbot_tank_swap` has invalid rule for map {} and boss {}", mapId, bossEntry);
            continue;
        }

        // 校验：换嘲 Aura 必须存在
        if (!sSpellMgr->GetSpellInfo(spellId))
        {
            ++skippedCount;
            LOG_ERROR("sql.sql", "Table `npcbot_tank_swap` references missing spell {} for map {} and boss {}", spellId, mapId, bossEntry);
            continue;
        }

        // 校验：层数必须大于 0
        if (auraStacks == 0)
        {
            ++skippedCount;
            LOG_ERROR("sql.sql", "Table `npcbot_tank_swap` has invalid aura_stacks for map {} and boss {}", mapId, bossEntry);
            continue;
        }

        BotTankSwapRule rule{ mapId, bossEntry, spellId, auraStacks };
        if (mapId)
            rulesByMap[mapId][bossEntry].push_back(rule);
        else
            globalRules[bossEntry].push_back(rule);
        ++loadedCount;
    } while (result->NextRow());

    _rulesByMap = std::move(rulesByMap);
    _globalRules = std::move(globalRules);

    LOG_INFO("server.loading", ">> Loaded {} NPCBot tank swap definitions in {} ms ({} skipped)",
        loadedCount, GetMSTimeDiffToNow(oldMSTime), skippedCount);
}

std::vector<BotTankSwapRule> const* NPCBotTankSwapMgr::GetRules(uint32 mapId, uint32 bossEntry) const
{
    // 优先地图专用配置
    if (auto mapItr = _rulesByMap.find(mapId); mapItr != _rulesByMap.end())
        if (auto bossItr = mapItr->second.find(bossEntry); bossItr != mapItr->second.end())
            return &bossItr->second;

    // 回退到全局配置
    if (auto bossItr = _globalRules.find(bossEntry); bossItr != _globalRules.end())
        return &bossItr->second;

    return nullptr;
}

bool NPCBotTankSwapMgr::HasRule(uint32 mapId, uint32 bossEntry) const
{
    return GetRules(mapId, bossEntry) != nullptr;
}
```

---

## 6. `bot_ai` 运行时集成

### 6.1 头文件修改

`src/server/game/AI/NpcBots/bot_ai.h`：

1. 顶部 include 增加（紧随现有 `#include "Hazards/NPCBotHazardMgr.h"`）：

```cpp
#include "Hazards/NPCBotHazardMgr.h"
#include "Hazards/NPCBotTankSwapMgr.h"
```

2. 在公开方法区（`CanTauntDistantTarget` 声明附近）新增：

```cpp
    bool UpdateTankSwap(uint32 diff);
    bool IsConfiguredTankSwapBoss(Unit const* target) const;
    virtual bool CanCastConfiguredTaunt(uint32 /*diff*/) const { return false; }
    virtual bool CastConfiguredTaunt(Unit* /*boss*/, uint32 /*diff*/) { return false; }
```

3. 在成员变量区（`_creatureHazardStates` 声明附近，`bot_ai.h:730`）新增换嘲状态：

```cpp
    AoeSpotsVec _aoeSpots;
    NPCBotCreatureHazardStateMap _creatureHazardStates;

    // 坦克换嘲机制状态
    uint32 _tankSwapTimer{};       // 换嘲节流计时器（800ms）
```

### 6.2 调用点

`bot_ai::GlobalUpdate(uint32 diff)` 中，在 `ReduceCD(diff);`（`bot_ai.cpp:18414`）之后插入：

```cpp
    ReduceCD(diff);

    // 坦克换嘲机制（内部自行判断存活/战斗/坦克职责/节流）
    UpdateTankSwap(diff);
```

### 6.3 `UpdateTankSwap` 实现

`bot_ai.cpp` 新增：

```cpp
// 坦克换嘲机制统一入口，由 GlobalUpdate 每帧调用
bool bot_ai::UpdateTankSwap(uint32 diff)
{
    // 800ms 节流，避免每帧扫描（置于最前，先于一切判断）
    if (_tankSwapTimer > diff)
    {
        _tankSwapTimer -= diff;
        return false;
    }
    _tankSwapTimer = 800;

    // 准入条件：本 BOT 必须是坦克，且嘲讽技能可用（未禁用、未在 CD）
    if (!IsTank() || !CanCastConfiguredTaunt(diff))
        return false;

    // 仅存活、在战斗中的 BOT 参与，自由 BOT 不参与
    if (!me->IsAlive() || !me->IsInCombat() || IAmFree())
        return false;

    // T2 正在攻击的必须是配置了换嘲机制的 BOSS（无需地图扫描）
    Unit* boss = me->GetVictim();
    if (!boss || !boss->IsCreature())
        return false;

    // 检查 BOSS 当前攻击目标；目标是自己或不存在时跳过（先于规则查询，尽早短路）
    Unit* t1 = boss->GetVictim();
    if (!t1 || t1 == me)
        return false;

    std::vector<BotTankSwapRule> const* rules = sNPCBotTankSwapMgr->GetRules(me->GetMapId(), boss->ToCreature()->GetCreatureTemplate()->Entry);
    if (!rules || rules->empty())
        return false;

    // 遍历该 BOSS 的全部换嘲规则，BOSS 目标身上的换嘲 Aura 达到层数即触发
    for (BotTankSwapRule const& rule : *rules)
    {
        Aura const* aura = t1->GetAura(rule.SpellId);
        if (!aura || aura->GetStackAmount() < rule.AuraStacks)
            continue;

        // 由职业 AI 施放对应嘲讽技能。
        // 嘲讽技能自身有 CD（IsSpellReady 已拦），且换嘲成功后 BOSS 目标切到本 BOT（t1 == me 跳过），
        // 天然避免重复施法，无需额外冷却。
        if (CastConfiguredTaunt(boss, diff))
            return true;

        break;
    }

    return false;
}
```

### 6.4 触发条件对照

`UpdateTankSwap` 内蕴含的完整条件（与设计目标一一对应）：

| # | 条件 | 代码位置 |
|---|---|---|
| 1 | 节流周期到达（800ms，最先判断） | `_tankSwapTimer` |
| 2 | 准入：本 BOT 是坦克 | `!IsTank()` |
| 3 | 准入：嘲讽技能可用（未禁用、未在 CD） | `!CanCastConfiguredTaunt(diff)` |
| 4 | 本 BOT 存活、在战斗中、非自由 BOT | `!me->IsAlive() \|\| !me->IsInCombat() \|\| IAmFree()` |
| 5 | 本 BOT 当前攻击目标是 BOSS | `me->GetVictim()` + `boss->IsCreature()` |
| 6 | BOSS 当前目标存在且不是本 BOT（目标是自己则跳过） | `t1 && t1 != me` |
| 7 | 本 BOT 当前攻击目标是配置了换嘲机制的 BOSS | `GetRules` + `!rules->empty()` |
| 8 | BOSS 目标身上的换嘲 Aura 达到层数 | `aura->GetStackAmount() >= rule.AuraStacks` |

---

## 7. 拦截普通嘲讽逻辑

### 7.1 现有普通嘲讽逻辑

当前 `bot_ai::CanTauntTarget()`（`bot_ai.cpp:2030`）支持：

- 非坦克目标的救场嘲讽；
- 被攻击坦克生命值低于约 30% 时的救场嘲讽；
- 主坦/副坦标记和目标标记驱动的嘲讽；
- `Rand() < 50` 的随机嘲讽。

`bot_ai::CanTauntDistantTarget()`（`bot_ai.cpp:2043`）支持远程救场嘲讽，但对高等级副本 BOSS 和世界 BOSS 已有排除条件。

各职业 AI 在 `UpdateAI` 中分别调用嘲讽技能：

| 职业 | 类名 | 嘲讽技能 | 技能 ID |
|---|---|---|---|
| 战士 | `warrior_botAI` | Taunt | `355` |
| 圣骑士 | `paladin_botAI` | Hand of Reckoning | `62124` |
| 德鲁伊 | `bot_druid_ai` | Growl | `6795` |
| 死亡骑士 | `death_knight_botAI` | Dark Command | `56222` |
| 虫族 | `crypt_lord_botAI` | Taunt | `54794` |

### 7.2 拦截实现

配置了换嘲机制的 BOSS 必须禁止走上述普通嘲讽逻辑。新增：

```cpp
// 目标是否为配置了坦克换嘲机制的 BOSS
bool bot_ai::IsConfiguredTankSwapBoss(Unit const* target) const
{
    if (!target || !target->IsCreature())
        return false;
    return sNPCBotTankSwapMgr->HasRule(me->GetMapId(), target->ToCreature()->GetCreatureTemplate()->Entry);
}
```

在 `CanTauntTarget` 与 `CanTauntDistantTarget` 函数体**开头**插入拦截：

```cpp
bool bot_ai::CanTauntTarget(Unit const* target, float dist) const
{
    // 配置了坦克换嘲机制的 BOSS 禁止走普通救场/标记嘲讽，改由 UpdateTankSwap 统一处理
    if (IsConfiguredTankSwapBoss(target))
        return false;

    Unit const* u = target->GetVictim();
    // ... 现有逻辑保持不变 ...
}

bool bot_ai::CanTauntDistantTarget(Unit const* target) const
{
    if (IsConfiguredTankSwapBoss(target))
        return false;

    Unit const* u = target->GetVictim();
    // ... 现有逻辑保持不变 ...
}
```

> 补充：圣骑士的 `Righteous Defense`（正义防御 `31789`）与战士的 `Challenging Shout`（挑战怒吼 `1161`）不走 `CanTauntTarget`/`CanTauntDistantTarget`，若需完全禁止普通逻辑干扰，可在对应职业 AI 调用处增加 `IsConfiguredTankSwapBoss(mytar)` 判断。第一版可只拦单目标嘲讽，群体/保护嘲讽作为后续加固项。

---

## 8. 职业技能执行

### 8.1 虚接口

`bot_ai` 声明两个虚函数，各职业 AI override：

```cpp
// 准入判断：嘲讽技能是否可用（未禁用、未在 CD）
virtual bool CanCastConfiguredTaunt(uint32 /*diff*/) const { return false; }

// 执行嘲讽：施放对应职业的嘲讽技能
virtual bool CastConfiguredTaunt(Unit* /*boss*/, uint32 /*diff*/) { return false; }
```

### 8.2 各职业实现

**战士** `bot_warrior_ai.cpp`（`warrior_botAI`）：

```cpp
bool CanCastConfiguredTaunt(uint32 diff) const override
{
    return IsSpellReady(TAUNT_1, diff, false);
}

bool CastConfiguredTaunt(Unit* boss, uint32 diff) override
{
    if (!boss || !CanCastConfiguredTaunt(diff))
        return false;

    // 需要防御姿态；无法切换姿态时放弃
    if (!_inStance(2) && stancetimer > diff)
        return false;

    return doCast(boss, GetSpell(TAUNT_1));
}
```

**圣骑士** `bot_paladin_ai.cpp`（`paladin_botAI`）：

```cpp
bool CanCastConfiguredTaunt(uint32 diff) const override
{
    return IsSpellReady(HAND_OF_RECKONING_1, diff, false);
}

bool CastConfiguredTaunt(Unit* boss, uint32 diff) override
{
    if (!boss || !CanCastConfiguredTaunt(diff))
        return false;

    return doCast(boss, GetSpell(HAND_OF_RECKONING_1));
}
```

**德鲁伊** `bot_druid_ai.cpp`（`bot_druid_ai`）：

```cpp
bool CanCastConfiguredTaunt(uint32 diff) const override
{
    return IsSpellReady(GROWL_1, diff, false);
}

bool CastConfiguredTaunt(Unit* boss, uint32 diff) override
{
    if (!boss || !CanCastConfiguredTaunt(diff))
        return false;

    return doCast(boss, GetSpell(GROWL_1));
}
```

**死亡骑士** `bot_death_knight_ai.cpp`（`death_knight_botAI`）：

```cpp
bool CanCastConfiguredTaunt(uint32 diff) const override
{
    return IsSpellReady(DARK_COMMAND_1, diff, false);
}

bool CastConfiguredTaunt(Unit* boss, uint32 diff) override
{
    if (!boss || !CanCastConfiguredTaunt(diff))
        return false;

    return doCast(boss, GetSpell(DARK_COMMAND_1));
}
```

**虫族（可选）** `bot_crypt_lord_ai.cpp`（`crypt_lord_botAI`）：

```cpp
bool CanCastConfiguredTaunt(uint32 diff) const override
{
    return IsSpellReady(TAUNT_1, diff, false);
}

bool CastConfiguredTaunt(Unit* boss, uint32 diff) override
{
    if (!boss || !CanCastConfiguredTaunt(diff))
        return false;

    return doCast(boss, GetSpell(TAUNT_1));
}
```

> `IsSpellReady(技能ID, diff, false)` 与 `doCast(目标, GetSpell(技能ID))` 均为 `bot_ai` 现有接口，与各职业既有嘲讽逻辑用法一致。

---

## 9. 各职业修改清单

| 文件 | 修改内容 |
|---|---|
| `bot_warrior_ai.cpp` | `warrior_botAI` 中 override `CanCastConfiguredTaunt` 与 `CastConfiguredTaunt` |
| `bot_paladin_ai.cpp` | `paladin_botAI` 中 override `CanCastConfiguredTaunt` 与 `CastConfiguredTaunt` |
| `bot_druid_ai.cpp` | `bot_druid_ai` 中 override `CanCastConfiguredTaunt` 与 `CastConfiguredTaunt` |
| `bot_death_knight_ai.cpp` | `death_knight_botAI` 中 override `CanCastConfiguredTaunt` 与 `CastConfiguredTaunt` |
| `bot_crypt_lord_ai.cpp` | （可选）`crypt_lord_botAI` 中 override `CanCastConfiguredTaunt` 与 `CastConfiguredTaunt` |

---

## 10. 机制优先级

同一时间有多个状态时，建议使用以下优先级：

1. 死亡、传送和控制状态；
2. 致命危险区域（`GetAoeSpots` 规避）；
3. 坦克换嘲（`UpdateTankSwap`）；
4. 普通战斗站位（`GetInPosition`）。

换嘲不涉及移动，优先级低于致命危险区域规避；由 `GlobalUpdate` 统一调用，与危险区域规避（`CalculateAoeSpots`）互不阻塞。

---

## 11. 推荐代码落点

| 功能 | 文件 | 位置 |
|---|---|---|
| 规则结构 `BotTankSwapRule` | `Hazards/NPCBotTankSwapMgr.h`（新建） | 顶部 |
| 管理器类 | `Hazards/NPCBotTankSwapMgr.h/.cpp`（新建） | 整文件 |
| 加载调用 | `botmgr.cpp` | `sNPCBotHazardMgr->LoadFromDB();`（`botmgr.cpp:76`）之后 |
| `UpdateTankSwap` | `bot_ai.cpp` | 新增，`GlobalUpdate` 内调用 |
| `IsConfiguredTankSwapBoss` | `bot_ai.cpp` | 新增，`CanTauntTarget` 附近 |
| 拦截普通嘲讽 | `bot_ai.cpp` | `CanTauntTarget`（`:2030`）、`CanTauntDistantTarget`（`:2043`）开头 |
| `CanCastConfiguredTaunt` / `CastConfiguredTaunt` 虚接口 | `bot_ai.h` | `CanTauntDistantTarget` 声明附近 |
| 换嘲状态成员 | `bot_ai.h` | `_creatureHazardStates`（`:730`）之后 |
| 数据库更新 | `data/sql/updates/pending_db_world/` | 新建 `rev_*.sql` |

### 11.1 加载点代码

`src/server/game/AI/NpcBots/botmgr.cpp:76` 当前：

```cpp
    sNPCBotHazardMgr->LoadFromDB();
    BotDataMgr::LoadNpcBots();
```

改为：

```cpp
    sNPCBotHazardMgr->LoadFromDB();
    sNPCBotTankSwapMgr->LoadFromDB();
    BotDataMgr::LoadNpcBots();
```

并在 `botmgr.cpp` 顶部 include 增加 `"Hazards/NPCBotTankSwapMgr.h"`。

---

## 12. 测试要点

### 12.1 数据库加载

- 正常配置能够加载；
- 不存在的 BOSS Entry 被跳过并记录日志；
- 不存在的 Spell ID 被跳过并记录日志；
- `enabled = 0` 的配置不参与处理；
- `aura_stacks = 0` 的配置被跳过；
- 地图专用配置优先于全局配置（`map_id = 0`）。

### 12.2 坦克换嘲

- 本 BOT 不是坦克时不换嘲；
- 嘲讽技能被禁用时不换嘲；
- 嘲讽技能在 CD 中时不换嘲；
- 本 BOT 没有攻击配置 BOSS 时不换嘲；
- BOSS 没有当前目标时不换嘲；
- BOSS 当前目标是本 BOT 自己时跳过（不换嘲）；
- BOSS 目标身上的换嘲 Aura 层数未达到阈值时不换嘲；
- 达到阈值后只触发一次；
- 嘲讽成功后 BOSS 目标发生变化；
- 没有配置的 BOSS 不影响正常嘲讽。

### 12.3 回归测试

- 没有配置的 BOSS 不影响 NPCBot 正常战斗；
- 现有普通嘲讽（救场、低血量、主副坦标记）对配置 BOSS 被正确拦截；
- 现有危险区域规避不受影响；
- 多个 BOSS 同时存在时状态互不干扰。

---

## 13. 实施步骤清单

1. 新建 SQL 更新文件，创建 `npcbot_tank_swap` 表；
2. 新建 `NPCBotTankSwapMgr.h/.cpp`，实现加载与查询；
3. `botmgr.cpp` 中注册 `sNPCBotTankSwapMgr->LoadFromDB()`；
4. `bot_ai.h` 增加 include、成员变量、三个方法声明；
5. `bot_ai.cpp` 实现 `UpdateTankSwap`、`IsConfiguredTankSwapBoss`，并在 `CanTauntTarget`/`CanTauntDistantTarget` 开头拦截；
6. 各职业 AI override `CanCastConfiguredTaunt` 与 `CastConfiguredTaunt`；
7. 按 12 节测试要点自测，运行 `apps/codestyle/codestyle-cpp.py` 与 `apps/codestyle/codestyle-sql.py` 校验代码风格。
