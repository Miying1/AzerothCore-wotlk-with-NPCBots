# BossBot 小队方案

> 将 NPCBot 的职业战斗逻辑复用为**多个 BOSS 组成的小队**：
> 相互协作（互奶 / Buff / 集火 / 不内讧，与玩家 Bot 小队一致），
> 固定装备外形 + 指定职业 botai + 单独配置血量/伤害缩放，
> 可在大世界或任意副本中**临时召唤**出来攻击玩家，或按固定路线游荡，**不依赖 creature 表固定刷新点**。

---

## 1. 需求概述

| 需求 | 说明 |
|---|---|
| 多个 BOSS 组队 | 多个 creature entry 组成小队（如战士 + 牧师 + 法师） |
| 相互协作 | 与玩家 Bot 小队一致：治疗互相奶、Buff 互加、集火、不内讧 |
| 装备外形固定 | 不随机生成，外观固定 |
| 使用不同职业 botai | 复用 `warrior_bot` / `priest_bot` / `mage_bot` 等完整战斗逻辑 |
| 伤害/血量单独配置 | 每个 BossBot 单独设缩放倍率 |
| 临时召唤 | 大世界 / 任意副本动态生成，不固定刷新点 |
| 可选固定路线游荡 | 可配置按 waypoint 巡逻 |

---

## 2. 方案总览

```
┌──────────────────────────────────────────────────────────────┐
│ 数据层（SQL）                                                 │
│  自定义 faction（对玩家敌对 + 同阵营友好）← 协作的核心         │
│  creature_template             : ScriptName + flags_extra     │
│  creature_template_npcbot_extras: entry + class + race        │
│  creature_equip_template       : 固定武器外观                  │
│  characters_npcbot            : owner=0, faction, 缩放        │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│ 核心引擎（最小侵入，共 4 个文件）                              │
│  ① BotMiscValues 枚举：新增 3 个 miscvalue key               │
│  ② bot_ai 新增 IsBossBot()/GetBossBotHpMod()/GetBossBotDmgMod│
│  ③ InitEquips()：BossBot 强制走固定装备分支                   │
│  ④ _OnHealthUpdate()/ApplyBotDamageMultiplier*()：叠加缩放    │
│  ⑤ botcommands：.npcbot delete free 跳过 BossBot（防误删）    │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│ 触发层（PvpBotTeam 模块，全新代码，不动核心）                  │
│  临时召唤小队 → 可选 waypoint 巡逻 → 击杀/重生                │
└──────────────────────────────────────────────────────────────┘
```

---

## 3. 核心机制（已核实）

### 3.1 职业 botai 挂载 —— `creature_template.ScriptName`

职业 AI 通过 `CreatureScript` 注册，`ScriptName` 决定挂载哪个职业 AI：

```cpp
// bot_warrior_ai.cpp
struct warrior_bot : public CreatureScript
{
    warrior_bot() : CreatureScript("warrior_bot") { }   // ScriptName = "warrior_bot"
    CreatureAI* GetAI(Creature* creature) const override { return new warrior_botAI(creature); }
};
```

`FactorySelector::SelectAI()` 根据 `creature_template.ScriptName` 返回对应职业 AI。

各职业 ScriptName：`warrior_bot` / `paladin_bot` / `hunter_bot` / `rogue_bot` / `priest_bot` / `death_knight_bot` / `shaman_bot` / `mage_bot` / `warlock_bot` / `druid_bot`。

### 3.2 协作机制（本方案的关键）—— `IsInBotParty()` 与阵营

BossBot 是 free bot（`IAmFree()` 为 true），队友判定在 `bot_ai::IsInBotParty()`（`bot_ai.cpp:3618`）：

```cpp
bool bot_ai::IsInBotParty(Unit const* unit) const
{
    if (IAmFree())
    {
        // ⚠️ 关键陷阱：中立敌对(16) 的 bot 之间直接判定为「非队友」！
        if (me->GetFaction() == FACTION_TEMPLATE_NEUTRAL_HOSTILE || unit->GetFaction() == FACTION_TEMPLATE_NEUTRAL_HOSTILE)
            return false;
        // ...
        return (unit->GetFaction() == me->GetFaction() ||          // ① 同阵营即队友
                (me->GetBotGroup() && me->GetBotGroup()->IsMember(unit->GetGUID()))); // ② 同 Group
    }
    // ...
}
```

同理，`CanBotAttack()`（`bot_ai.cpp:3796`）：

```cpp
// 同阵营(非中立敌对)不互相攻击
if ((target->GetFaction() == 35 || target->GetFaction() == me->GetFaction()) &&
    me->GetFaction() != FACTION_TEMPLATE_NEUTRAL_HOSTILE)
    return false;
```

**结论（必须牢记）**：

1. 多个 BossBot 只要**使用同一个阵营模板（faction），且该 faction 不是 16（中立敌对）**，就会互相判定为队友：
   - 治疗会互相奶（`IsInBotParty` 通过）；
   - Buff 会互加；
   - 不会互相攻击（`CanBotAttack` 同阵营拦截）。
2. **绝不能用 faction=16**：`FACTION_TEMPLATE_NEUTRAL_HOSTILE` 会被代码特殊处理，导致 BossBot 之间不协作、甚至内讧。

### 3.3 自定义阵营 —— 对玩家敌对 + 同阵营友好

`FactionTemplateEntry::IsFriendlyTo()` 保证 **`Faction`（Faction.dbc ID）相同的两个单位始终友好**：

```cpp
// DBCStructure.h:953
bool IsFriendlyTo(FactionTemplateEntry const& entry) const
{
    if (faction == entry.faction)   // 同 Faction 始终友好
        return true;
    ...
}
```

因此 BossBot 需要自定义一个阵营模板：
- `EnemyGroup` 含玩家 group（对玩家敌对 → 攻击玩家）；
- 同 `Faction` 友好（同阵营友好 → 小队协作）；
- 非 16（避开中立敌对特殊逻辑）。

### 3.4 装备分支 —— `InitEquips()`

```cpp
// bot_ai.cpp:15545
if (is_wanderer || me->IsSummon())   // ① 随机装备（游荡/召唤 bot）
{ ... GenerateWanderingBotItem ... }
else                                 // ② 固定装备（读 characters_npcbot.equips / creature_equip_template）
{ ... }
```

BossBot 必须走 ② 固定装备分支（见 4.3）。

### 3.5 缩放配置 —— `characters_npcbot.miscvalues`

`miscvalues` 是 per-bot 持久化键值（格式 `key:value key:value`），用于存 BossBot 标记与缩放，**改 SQL 即生效，无需重编译**。

### 3.6 临时召唤 —— `SummonCreature`

`LoadBotCreatureFromDB` 有 `ASSERT(map->GetInstanceId() == 0)` 限制，只能用于世界地图；临时召唤必须用 `SummonCreature`（生成 TempSummon），可在任意地图（含副本）生成，AI 同样按 `ScriptName` 正确挂载。

---

## 4. 核心代码改动（最小侵入，共 4 个文件）

> 所有改动均为「新增谓词 + 条件排除 + 数值末尾叠加」，不影响现有 Bot 行为（`IsBossBot()` 默认 false）。

### 4.1 `src/server/game/AI/NpcBots/botcommon.h` —— 新增 miscvalue key

```cpp
enum BotMiscValues : uint32
{
//SAVED
    BOTAI_MISC_ENCHANT_IS_AUTO_MH       = 1,
    BOTAI_MISC_ENCHANT_IS_AUTO_OH       = 2,
    BOTAI_MISC_ENCHANT_TIMER_MH         = 3,
    BOTAI_MISC_ENCHANT_TIMER_OH         = 4,
    BOTAI_MISC_ENCHANT_CURRENT_MH       = 5,
    BOTAI_MISC_ENCHANT_CURRENT_OH       = 6,
    BOTAI_MISC_PET_TYPE                 = 7,
    BOTAI_MISC_AURA_TYPE                = 8,
    // —— BossBot 专用（保存到 characters_npcbot.miscvalues）——
    BOTAI_MISC_BOSS_FLAG                = 9,    // 是否为 BossBot（1 = 是）
    BOTAI_MISC_BOSS_HP_MOD              = 10,   // 血量缩放百分比，100 = 1.0 倍
    BOTAI_MISC_BOSS_DMG_MOD             = 11,   // 伤害缩放百分比，100 = 1.0 倍
//INTERNAL
    BOTAI_MISC_ENCHANT_AVAILABLE_1      = 12,   // 注意：起点从 9 改为 12，避免与 9 冲突
    BOTAI_MISC_ENCHANT_AVAILABLE_2,
    // ... 其余保持不变（自动递增）...
    BOT_MISCVALUE_SAVED_FIRST = BOTAI_MISC_ENCHANT_IS_AUTO_MH,   // = 1
    BOT_MISCVALUE_SAVED_LAST = BOTAI_MISC_BOSS_DMG_MOD           // 由 8 改为 11
};
```

### 4.2 `src/server/game/AI/NpcBots/bot_ai.h` —— 新增 3 个辅助方法

在 `IAmFree()`（约 205 行）附近 public 区域新增：

```cpp
    // —— BossBot 相关 ——
    // 是否 BossBot（由 characters_npcbot.miscvalues 的 BOTAI_MISC_BOSS_FLAG 标记）
    bool IsBossBot() const
    {
        auto const it = _botData->miscvalues.find(BOTAI_MISC_BOSS_FLAG);
        return it != _botData->miscvalues.cend() && it->second != 0;
    }
    // 血量缩放系数（BOTAI_MISC_BOSS_HP_MOD，百分比），未配置时默认 1.0
    float GetBossBotHpMod() const
    {
        auto const it = _botData->miscvalues.find(BOTAI_MISC_BOSS_HP_MOD);
        return it != _botData->miscvalues.cend() ? float(it->second) / 100.0f : 1.0f;
    }
    // 伤害缩放系数（BOTAI_MISC_BOSS_DMG_MOD，百分比），未配置时默认 1.0
    float GetBossBotDmgMod() const
    {
        auto const it = _botData->miscvalues.find(BOTAI_MISC_BOSS_DMG_MOD);
        return it != _botData->miscvalues.cend() ? float(it->second) / 100.0f : 1.0f;
    }
```

### 4.3 `src/server/game/AI/NpcBots/bot_ai.cpp` —— `InitEquips()` 固定装备分支

随机装备分支条件排除 BossBot（`bot_ai.cpp:15545`）：

```cpp
    const bool is_wanderer = IsWanderer();
    // BossBot：强制走固定装备分支（外形固定），不走随机生成
    if (!IsBossBot() && (is_wanderer || me->IsSummon()))
    {
        // ... 原有随机装备逻辑不变 ...
    }
    else
    {
        // ... 原有固定装备逻辑不变（BossBot 与普通 bot 共用）...
    }
```

### 4.4 `src/server/game/AI/NpcBots/bot_ai.cpp` —— 血量缩放

`_OnHealthUpdate()`（约 7357 行），`bonuspct` 处理（约 7398 行）之后插入：

```cpp
    // BossBot：在基础属性公式上叠加按 Bot 单独的血量缩放
    if (IsBossBot())
        m_totalhp = uint32(float(m_totalhp) * GetBossBotHpMod());
```

### 4.4b `src/server/game/AI/NpcBots/bot_ai.cpp` —— 法力兜底（审查新增，必须）

`_OnManaUpdate()`（约 7408 行），`m_basemana = intValue * intMult + 20.f`（约 7450 行）之后插入：

```cpp
    // BossBot：固定装备分支下无盔甲智力，法力会掉到 ~20（放不出技能）。
    // 用「职业基础法力」兜底，保证法系 BossBot 能正常施法。
    if (IsBossBot())
        m_basemana = std::max<float>(m_basemana, classinfo.basemana);
```

> 原理：`_OnManaUpdate` 里 `m_basemana = 装备智力 × 15 + 20`，法力几乎完全由装备智力决定；
> 而 BossBot 走固定装备分支（只有 `creature_equip_template` 武器外观，无盔甲智力），
> 若不兜底，法系 BossBot（法师/牧师）法力 ≈ 20，放一个技能即空蓝。

### 4.5 `src/server/game/AI/NpcBots/bot_ai.cpp` —— 伤害缩放（3 处）

三个伤害结算入口（约 7908 / 7915 / 7922 行），`ApplyClass*` 之后追加：

```cpp
void bot_ai::ApplyBotDamageMultiplierMelee(uint32& damage, CalcDamageInfo& damageinfo) const
{
    damage *= BotCfg::GetBotDamageModByClass(GetBotClass());
    damage *= BotCfg::GetBotDamageModByLevel(me->GetLevel());
    ApplyClassDamageMultiplierMelee(damage, damageinfo);
    // BossBot：按 Bot 单独的伤害缩放
    if (IsBossBot())
        damage = uint32(float(damage) * GetBossBotDmgMod());
}

void bot_ai::ApplyBotDamageMultiplierMelee(int32& damage, SpellNonMeleeDamage& damageinfo,
    SpellInfo const* spellInfo, WeaponAttackType attackType, bool iscrit) const
{
    damage *= BotCfg::GetBotDamageModByClass(GetBotClass());
    damage *= BotCfg::GetBotDamageModByLevel(me->GetLevel());
    ApplyClassDamageMultiplierMeleeSpell(damage, damageinfo, spellInfo, attackType, iscrit);
    // BossBot：按 Bot 单独的伤害缩放
    if (IsBossBot())
        damage = int32(float(damage) * GetBossBotDmgMod());
}

void bot_ai::ApplyBotDamageMultiplierSpell(int32& damage, SpellNonMeleeDamage& damageinfo,
    SpellInfo const* spellInfo, WeaponAttackType attackType, bool iscrit) const
{
    damage *= BotCfg::GetBotDamageModByClass(GetBotClass());
    damage *= BotCfg::GetBotDamageModByLevel(me->GetLevel());
    ApplyClassDamageMultiplierSpell(damage, damageinfo, spellInfo, attackType, iscrit);
    // BossBot：按 Bot 单独的伤害缩放
    if (IsBossBot())
        damage = int32(float(damage) * GetBossBotDmgMod());
}
```

### 4.6 `src/server/game/AI/NpcBots/bot_ai.cpp` —— `ResetAllMiscValues()` 补 case

```cpp
        switch (miscval)
        {
            // ... 原有 case ...
            case BOTAI_MISC_BOSS_FLAG:
            case BOTAI_MISC_BOSS_HP_MOD:
            case BOTAI_MISC_BOSS_DMG_MOD:
                // BossBot 标记与缩放由数据库配置决定，重置时保持不变
                break;
            default:
                BOT_LOG_ERROR("npcbots", "ResetMiscValues: unknown saved miscvalue {} ...", miscval, ...);
                SetAIMiscValue(miscval, uint32(0));
                break;
        }
```

### 4.7 `src/server/game/AI/NpcBots/botcommands.cpp` —— `.npcbot delete free` 跳过 BossBot

`HandleNpcBotDeleteFreeCommand`（约 3698 行），跳过带 BOSS_FLAG 的 free bot，防止 GM 批量清理自由 bot 时误删 BossBot：

```cpp
static bool HandleNpcBotDeleteFreeCommand(ChatHandler* handler)
{
    uint32 count = 0;
    for (uint32 creature_id : BotDataMgr::GetExistingNPCBotIds())
        if (NpcBotData const* botData = BotDataMgr::SelectNpcBotData(creature_id))
            if (botData->owner == 0)
            {
                // 跳过 BossBot：miscvalues 里带 BOSS_FLAG（key=9）的 free bot 是 Boss，不是可清理的游荡/自由 bot
                if (botData->miscvalues.contains(BOTAI_MISC_BOSS_FLAG))
                    continue;
                if (HandleNpcBotDeleteByIdCommand(handler, creature_id))
                    ++count;
            }

    handler->PSendSysMessage("{} 个自由 npcbot 已删除", count);
    return true;
}
```

> `botData->miscvalues` 为 `std::map<uint32, uint32>`，项目为 C++20，`contains()` 可用。
> 可选：`.npcbot list free`（约 4048 行）若想彻底隐藏 BossBot，可在 `copy_if` lambda 里加 `&& !botData->miscvalues.contains(BOTAI_MISC_BOSS_FLAG)`。

---

## 5. 数据层 SQL

> 按 AGENTS.md 规范：`creature_*`、`faction*` 写入 `data/sql/updates/pending_db_world/`，
> `characters_npcbot*` 写入 `data/sql/updates/pending_db_characters/`。

### 5.1 `pending_db_world` —— 自定义阵营（协作核心）

```sql
-- ============ 自定义阵营：对玩家敌对 + 同阵营友好 + 非 16 ============
-- ① faction_dbc（Faction.dbc）：声望阵营定义（BossBot 不涉及玩家声望，核心字段即可）
DELETE FROM `faction_dbc` WHERE `ID` = 91000;
INSERT INTO `faction_dbc` (`ID`, `ReputationIndex`, `Name_Lang_zhCN`, `Name_Lang_Mask`)
VALUES (91000, 91000, 'BossBot 小队', 1);

-- ② factiontemplate_dbc（FactionTemplate.dbc）：阵营模板
--    FactionGroup=0 : ourMask 为 0（不属于联盟/部落/怪物 group）
--    EnemyGroup=1   : hostileMask=1（对「玩家 group」敌对 → 攻击玩家）
--    Friend_1=91000 : 对 faction 91000 友好（同阵营友好 → 小队协作）
DELETE FROM `factiontemplate_dbc` WHERE `ID` = 91000;
INSERT INTO `factiontemplate_dbc`
    (`ID`, `Faction`, `Flags`, `FactionGroup`, `FriendGroup`, `EnemyGroup`,
     `Enemies_1`, `Enemies_2`, `Enemies_3`, `Enemies_4`,
     `Friend_1`, `Friend_2`, `Friend_3`, `Friend_4`)
VALUES
    (91000, 91000, 0, 0, 0, 1, 0, 0, 0, 0, 91000, 0, 0, 0);
```

> 说明：`factiontemplate_dbc` 是 base 表，这里**只新增高 ID（91000）的自定义行**，不修改任何现有行（符合 AGENTS.md「base 表不可变」的约束，updates 允许追加新数据）。
> 若不想自定义，也可复用现成阵营（如 ID=21，`Faction=20`，`EnemyGroup=1` 对玩家敌对、同 faction 友好），但自定义更可控。

### 5.2 `pending_db_world` —— 生物模板（以 3 个 BossBot 为例）

```sql
-- ============ creature_template ============
-- ScriptName 决定职业 AI；flags_extra 含 NPCBot 标志；faction 占位（运行时被覆盖为 91000）
DELETE FROM `creature_template` WHERE `entry` IN (900001, 900002, 900003);
INSERT INTO `creature_template`
    (`entry`, `name`, `subname`, `minlevel`, `maxlevel`, `faction`, `npcflag`,
     `rank`, `dmgschool`, `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`,
     `unit_class`, `unit_flags`, `unit_flags2`, `type`, `type_flags`,
     `HealthModifier`, `ManaModifier`, `ArmorModifier`, `AIName`, `MovementType`,
     `HoverHeight`, `flags_extra`, `ScriptName`, `VerifiedBuild`)
VALUES
    (900001, '血色剑士',  'BossBot 小队', 80, 80, 35, 0, 1, 0, 1, 2000, 2000, 1, 32832, 2048, 7, 4096, 1, 1, 1, '', 0, 1, 68157552, 'warrior_bot', -1),
    (900002, '血色牧师',  'BossBot 小队', 80, 80, 35, 0, 1, 0, 1, 2000, 2000, 2, 32832, 2048, 7, 4096, 1, 1, 1, '', 0, 1, 68157552, 'priest_bot',  -1),
    (900003, '血色法师',  'BossBot 小队', 80, 80, 35, 0, 1, 0, 1, 2000, 2000, 8, 32832, 2048, 7, 4096, 1, 1, 1, '', 0, 1, 68157552, 'mage_bot',    -1);

-- ============ 声明为 NPCBot 并指定职业 ============
-- class：BOT_CLASS_WARRIOR=1, BOT_CLASS_PRIEST=5, BOT_CLASS_MAGE=8
DELETE FROM `creature_template_npcbot_extras` WHERE `entry` IN (900001, 900002, 900003);
INSERT INTO `creature_template_npcbot_extras` (`entry`, `class`, `race`) VALUES
    (900001, 1, 1),   -- 战士，人类
    (900002, 5, 1),   -- 牧师，人类
    (900003, 8, 1);   -- 法师，人类

-- ============ 固定武器外观（creature_equip_template）============
DELETE FROM `creature_equip_template` WHERE `CreatureID` IN (900001, 900002, 900003);
INSERT INTO `creature_equip_template` (`CreatureID`, `ID`, `ItemID1`, `ItemID2`, `ItemID3`) VALUES
    (900001, 1, 19364, 0, 0),   -- 战士：亚什坎德里·大军的黄刀（双手剑外观）
    (900002, 1, 46014, 0, 0),   -- 牧师：圣光之锤（法杖外观）
    (900003, 1, 45620, 0, 0);   -- 法师：影风之杖（法杖外观）
```

### 5.3 `pending_db_characters` —— Bot 实例数据（含缩放 + 阵营）

```sql
-- owner=0（无主 free bot），faction=91000（自定义阵营：对玩家敌对 + 同阵营协作）
-- roles：BOT_ROLE_TANK=1, BOT_ROLE_DPS=4, BOT_ROLE_HEAL=8, BOT_ROLE_RANGED=16
-- equips 留 0：BossBot 走固定装备分支（creature_equip_template 提供武器外观）
-- miscvalues：9:1（BossBot 标记）、10:N（血量缩放）、11:N（伤害缩放）

DELETE FROM `characters_npcbot` WHERE `entry` IN (900001, 900002, 900003);
INSERT INTO `characters_npcbot`
    (`entry`, `owner`, `roles`, `spec`, `faction`, `hire_time`, `shared_owners`,
     `equipMhEx`, `equipOhEx`, `equipRhEx`, `equipHead`, `equipShoulders`, `equipChest`,
     `equipWaist`, `equipLegs`, `equipFeet`, `equipWrist`, `equipHands`, `equipBack`,
     `equipBody`, `equipFinger1`, `equipFinger2`, `equipTrinket1`, `equipTrinket2`,
     `equipNeck`, `spells_disabled`, `miscvalues`)
VALUES
    -- 战士：坦克，血量 ×3.0，伤害 ×1.5
    (900001, 0, 1, 3, 91000, UNIX_TIMESTAMP(), '',
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', '9:1 10:300 11:150'),
    -- 牧师：治疗，血量 ×2.0，伤害 ×1.0
    (900002, 0, 8, 5, 91000, UNIX_TIMESTAMP(), '',
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', '9:1 10:200 11:100'),
    -- 法师：输出，血量 ×1.5，伤害 ×2.0
    (900003, 0, 4, 1, 91000, UNIX_TIMESTAMP(), '',
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', '9:1 10:150 11:200');
```

> 三个 BossBot 用**同一个 faction=91000**，因此 `IsInBotParty()` 判定为队友 → 牧师会奶战士、法师会集火、互相加 Buff、不内讧，完全复刻玩家 Bot 小队协作。

---

## 6. `PvpBotTeam` 模块（临时召唤小队 + 可选游荡）

在 `src/server/scripts/Custom/PvpBotTeam/` 新建，注册到 `custom_script_loader.cpp`。

### 6.1 文件结构

```
PvpBotTeam/
├── BossBot方案.md            # 本文档
├── pvp_bot_team.h            # 常量与接口
└── pvp_bot_team.cpp          # 召唤 / 游荡 / 触发逻辑
```

### 6.2 临时召唤整支小队

```cpp
// pvp_bot_team.h
#pragma once
#include "Creature.h"
#include "Map.h"

namespace PvpBotTeam
{
    // BossBot 小队的 entry（与数据库一致）
    inline constexpr uint32 BOSS_ENTRIES[] = { 900001, 900002, 900003 };  // 战士/牧师/法师

    // 召唤一支 BossBot 小队到指定地图坐标（可在大世界/副本）
    void SummonBossTeam(Map* map, Position const& center, uint32 durationMs = 0);
}
```

```cpp
// pvp_bot_team.cpp
#include "pvp_bot_team.h"
#include "Log.h"

// 临时召唤一支 BossBot 小队（围绕中心点三角站位，间距约 5 码）
void PvpBotTeam::SummonBossTeam(Map* map, Position const& center, uint32 durationMs)
{
    static constexpr float offsets[][2] = { {0.f, 0.f}, {5.f, 0.f}, {-2.5f, 4.3f} };
    for (uint32 i = 0; i < 3; ++i)
    {
        Position pos = center;
        pos.m_positionX += offsets[i][0];
        pos.m_positionY += offsets[i][1];

        // TempSummon：可在任意地图（含副本），AI 按 ScriptName 挂载职业 botai
        Creature* boss = map->SummonCreature(BOSS_ENTRIES[i], pos, nullptr, durationMs, nullptr, 0, 0);
        if (!boss)
            LOG_ERROR("pvp_bot_team", "无法召唤 BossBot entry={}", BOSS_ENTRIES[i]);
    }
}
```

### 6.3 可选：固定路线游荡（waypoint 巡逻）

BossBot 默认 `MovementType = IDLE`（站桩）。若要按固定路线巡逻，在生成后挂循环巡逻脚本：

```cpp
// 固定路线巡逻：按预设坐标点循环移动（战斗时由 botai 接管移动）
class bossbot_patrol_ai : public ScriptedAI
{
public:
    bossbot_patrol_ai(Creature* creature, std::vector<Position> const& path)
        : ScriptedAI(creature), _path(path), _idx(0) { }

    void UpdateAI(uint32 diff) override
    {
        if (me->IsInCombat())   // 战斗中不巡逻，交给 botai 战斗移动
            return;
        if (_moveTimer <= diff)
        {
            _moveTimer = 3000;
            me->GetMotionMaster()->MovePoint(_idx, _path[_idx]);
            _idx = (_idx + 1) % _path.size();
        }
        else
            _moveTimer -= diff;
    }

private:
    std::vector<Position> _path;   // 巡逻路线坐标点
    size_t _idx;                   // 当前目标点下标
    uint32 _moveTimer = 0;         // 移动间隔计时器
};
```

> 注意：BossBot 实际 AI 是 `bot_ai`，其移动由 `bot_ai::BotMovement()` 控制，巡逻需与战斗移动协调，具体在实现阶段联调。

### 6.4 触发方式（GM 命令，便于调试）

```cpp
// pvp_bot_team.cpp
class pvp_bot_team_command : public CommandScript
{
public:
    pvp_bot_team_command() : CommandScript("pvp_bot_team_command") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> commandTable =
        {
            { "bossbot", SEC_GAMEMASTER, false, &HandleBossBotCommand, "" }
        };
        return commandTable;
    }

    static bool HandleBossBotCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;
        PvpBotTeam::SummonBossTeam(player->GetMap(), player->GetPosition());
        handler->PSendSysMessage("BossBot 小队已召唤");
        return true;
    }
};

void AddSC_pvp_bot_team()
{
    new pvp_bot_team_command();
}
```

在 `custom_script_loader.cpp` 声明并调用：

```cpp
void AddSC_pvp_bot_team();
// ... 在 AddCustomScripts() 中
    AddSC_pvp_bot_team();
```

> 如需「区域/击杀前置 BOSS 触发」，把 `HandleBossBotCommand` 换成 `CreatureScript::MoveInLineOfSight` 或 `InstanceScript` 即可。

---

## 7. 使用与配置

### 7.1 调整单个 BossBot 缩放（改 SQL，无需重编译）

```sql
-- 把 900003（法师）血量改为 ×3.0，伤害改为 ×2.5
UPDATE `characters_npcbot`
SET `miscvalues` = '9:1 10:300 11:250'
WHERE `entry` = 900003;
```

- `9:1`   → 固定为 1，表示 BossBot；
- `10:N`  → 血量缩放 = `N / 100` 倍；
- `11:N`  → 伤害缩放 = `N / 100` 倍。

### 7.2 更换职业

改 `creature_template.ScriptName` + `creature_template_npcbot_extras.class`（两者必须一致）。

### 7.3 增减小队成员

新增一个 BossBot 只需：`creature_template` + `creature_template_npcbot_extras` + `creature_equip_template` + `characters_npcbot`（faction 用同一个 91000），并在 `PvpBotTeam::BOSS_ENTRIES` 里加 entry。

---

## 8. 注意事项与风险

1. **协作的关键是「同阵营 + 非 16」**：所有 BossBot 必须用同一个自定义 faction（91000），且**绝不能用 16（中立敌对）**，否则 `IsInBotParty()` 直接判定为非队友，小队不协作甚至内讧。
2. **`ScriptName` 与 `class` 必须一致**：`ScriptName="warrior_bot"` 配合 `class=1`（战士），否则 AI 与职业数据错位。
3. **`miscvalues` 的 key 冲突**：`BOTAI_MISC_ENCHANT_AVAILABLE_1` 必须显式改为 `12`（见 4.1）。
4. **`ResetAllMiscValues()` 补 case**：漏补会导致 Bot 重置时对 `9/10/11` 报错并清零。
5. **固定装备分支**：`creature_equip_template` 只提供 3 个武器槽外观（无盔甲）。若需完整盔甲外形，需在 `characters_npcbot.equips` 指定 item_instance，或用带盔甲的 `creature_template.modelid`。
6. **自定义阵营对「怪物」的态度**：本方案 `FriendGroup=0`，BossBot 不会对野怪友好（会攻击敌对野怪，这通常无害）；如需 BossBot 与野怪和平共处，可设 `FriendGroup=8`（怪物 group）。
7. **临时召唤的阵营/属性**：`SummonCreature` 生成的 TempSummon 会在 `bot_ai` 初始化时按 `characters_npcbot.faction` 覆盖阵营、按 `spec/level` 计算属性，召唤前需确保 `characters_npcbot` 记录存在。
8. **法系 BossBot 的法力兜底（4.4b）必须实现**：否则法系 BossBot（法师/牧师）法力 ≈ 20，放不出技能（详见第 10 节审查结论）。
9. **法系伤害偏低**：BossBot 走固定装备分支后法强 ≈ 0（法强来自装备法强 + 天赋转智力），法系技能伤害只剩 `base_damage`，需把 `dmg_mod`（`11:N`）调到比物理职业更大才能达到同等 BOSS 伤害（详见第 10 节）。
10. **编译**：核心改动在 `src/server/game/AI/NpcBots/`，改后需重新编译 worldserver。

---

## 9. 验证清单

- [ ] `botcommon.h` 枚举：新增 3 个 key，`BOTAI_MISC_ENCHANT_AVAILABLE_1 = 12`，`BOT_MISCVALUE_SAVED_LAST = 11`；
- [ ] `bot_ai.h`：`IsBossBot()` / `GetBossBotHpMod()` / `GetBossBotDmgMod()`；
- [ ] `bot_ai.cpp`：`InitEquips()` 排除 BossBot + 血量缩放 + **法力兜底** + 3 处伤害缩放 + `ResetAllMiscValues()` 补 case；
- [ ] `botcommands.cpp`：`.npcbot delete free` 跳过 BossBot（`miscvalues.contains(BOTAI_MISC_BOSS_FLAG)`）；
- [ ] SQL：自定义 faction（两表）、`creature_template`、`creature_template_npcbot_extras`、`creature_equip_template`、`characters_npcbot`（owner=0/faction=91000/miscvalues）；
- [ ] `PvpBotTeam` 模块：`SummonBossTeam` / 触发脚本 / 注册；
- [ ] 编译通过、无 lint 错误；
- [ ] 实测：`.bossbot summon` 能在大世界与副本中召唤 BossBot 小队；
- [ ] 实测：BossBot 小队能主动攻击玩家与玩家 Bot；
- [ ] 实测：**牧师会奶受伤的战士/法师，BossBot 之间不内讧**（协作生效）；
- [ ] 实测：武器外形固定，血量/伤害按各自 `miscvalues` 缩放生效；
- [ ] 实测：**法系 BossBot 法力值正常（≈职业基础法力），能持续施法**；
- [ ] 回归：普通雇佣/对话/组队流程不受影响（`IsBossBot()` 默认 false）。

---

## 10. 审查结论（针对本次审核）

> 本方案重新审核后，针对「是否影响原 bot 流程」和「技能缩放是否合理」两个问题，结论如下。

### 10.1 是否影响原本 bot 流程（雇佣 / 对话 / 组队）—— **不影响**

所有改动通过 `IsBossBot()` 谓词隔离，普通 bot 的 `_botData->miscvalues` 里没有 `9`（BOSS_FLAG），故 `IsBossBot()` 恒为 false：

| 改动点 | 对普通 bot 的影响 |
|---|---|
| `InitEquips()` 条件 `!IsBossBot() && (is_wanderer \|\| me->IsSummon())` | 等价于原条件，行为不变 |
| 血量 / 伤害 / 法力缩放 `if (IsBossBot())` | false，不执行 |
| 枚举 `BOTAI_MISC_ENCHANT_AVAILABLE_1` 9→12 | 该 key 属 INTERNAL 区（不持久化），代码均通过枚举名引用，安全 |
| `BOT_MISCVALUE_SAVED_LAST` 8→11 | 现有 bot 的 `miscvalues` 只有 1~8，新增 9~11 不冲突 |
| `ResetAllMiscValues()` 补 case 9/10/11 | 仅在 `BOTAI_RESET_DISMISS`（玩家解散雇佣 bot）触发；BossBot 是 free bot 永不 DISMISS，且补 case 后普通 bot 也不会误报 |

**结论**：雇佣、对话、组队、游荡等既有流程均不受影响。

### 10.2 技能缩放是否合理 —— **基本合理，但发现并已修正一个致命问题**

关键结论（已核实代码）：

1. **基础属性来自 `PlayerLevelInfo`（80 级职业裸体属性），与装备无关**（`SetStats()` 2366 行 `SetCreateStat(info.stats[i])`）。
2. **总属性 = 装备属性 × (1+天赋%) + 基础属性**（`_getTotalBotStat()` 14619~14623 行 `fval = 装备 × (1+fpct) + GetTotalStatValue(fstat)`）。
3. **血量 = 职业基础血量（`classinfo.basehealth`）+ 装备耐力**，`_OnHealthUpdate` 里 `m_totalhp` 显式包含 base，因此 BossBot 无盔甲也有足够基础血量。
4. **法力 = 装备智力 × 15 + 20**（`_OnManaUpdate` 7450 行 `m_basemana = intValue * intMult + 20`），**不包含** `classinfo.basemana` —— 这就是问题所在。

**发现的问题与修正**：

| 问题 | 影响 | 状态 |
|---|---|---|
| **法力归零**：法系 BossBot 无盔甲智力 → 法力 ≈ 20 | 法师/牧师放一个技能即空蓝 | ✅ 已加 4.4b 兜底（`m_basemana = max(m_basemana, classinfo.basemana)`） |
| **法强归零**：法强 = 装备法强 + 天赋转智力 → ≈ 0 | 法系技能伤害只剩 `base_damage`，缺法强加成 | ⚠️ 靠 `dmg_mod` 弥补，但法系需更大系数（见下） |
| 物理职业（战士/盗贼等） | 攻强 = 基础力量 + 武器 DPS，够用 | ✅ 无需处理 |

**关于「技能缩放不合理」的澄清**：

- 普通 bot 的法术伤害 = `base_damage + 法强 × 系数`，其中法强来自**装备**；BossBot 无装备法强，伤害只剩 `base_damage`。
- 伤害缩放 `damage × dmg_mod` 是**线性缩放**，作用在结算后的最终伤害上，数学上不会破坏技能内部平衡；
- 但不同技能的 `base_damage` 占比不同，**统一的 `dmg_mod` 无法让每个技能都精确复刻「装备齐全 bot」的伤害曲线**。
- 这其实是**可接受的**：BOSS 的技能伤害本就该是「固定数值 × 明确倍率」，而非依赖装备。实际调参时，法系（法师/牧师）的 `11:N` 需要比物理（战士）大（例如物理 150、法系 250~300），因为法系缺了法强这部分加成。

### 10.3 BossBot 加入 `characters_npcbot` 是否会被正常 Bot 功能统计到 —— **会，2 处需防范**

已核实各功能的数据源与过滤条件（`LoadNpcBots` / `GetExistingNPCBotIds` / `GetExistingNPCBots` / botgiver / botcommands / botdump）：

**关键事实**：
- `IsTempBot()` 判断的是 `GetOriginalEntry() == BOT_ENTRY_MIRROR_IMAGE_BM`（兽王猎「镜像」特殊 entry），**与 TempSummon 无关**。因此 BossBot（TempSummon）**不是** `IsTempBot()`。
- BossBot 会被 `bot_ai` 构造里的 `RegisterBot(me)` 登记进 `_existingBots`（因为 `!IsTempBot()` 成立）。
- BossBot 是 `SummonCreature` 生成的 TempSummon → `IsSummon() == true`。

**会被统计到（有风险）**：

| 功能 | 数据源 | 是否命中 BossBot | 风险 |
|---|---|---|---|
| `LoadNpcBots` 加载 | `characters_npcbot` 全表 | ✅ 加载进 `_botsData` | 无（召唤必需） |
| `.npcbot delete free`（botcommands.cpp 3701） | `GetExistingNPCBotIds()`（遍历 `_botsData`） | ✅ `owner==0` 被删 | **❌ 严重：会误删 BossBot** |
| `.npcbot list free`（botcommands.cpp 4048） | `GetExistingNPCBots()`（`_existingBots`） | ✅ 召唤后被列出 | 中（暴露 entry） |
| botdump 导出（botdump.cpp 724） | `GetExistingNPCBotIds()` | ✅ 被导出 | 低（备份） |
| `next_bot_id` 计算（botdatamgr.cpp 303） | `GetExistingNPCBotIds()` | ✅ 影响自动 entry 起点 | 低 |

**不会被统计到（天然隔离）**：

| 功能 | 原因 |
|---|---|
| 雇佣列表（botgiver.cpp 122/212） | BossBot 是 TempSummon → `IsSummon()==true` → 被 `continue` 跳过 |
| 玩家 Bot 管理器（`BotMgr`） | 只管理 `owner=玩家guid` 的 bot |
| `GetOwnedBotsCount` / `GetAccountBotsCount` / `GetNpcBotCountByIp` | 按 owner/account/IP 过滤，BossBot `owner=0` 不匹配 |

**必须追加的防范改动（第 4 个文件）**：

`src/server/game/AI/NpcBots/botcommands.cpp` 的 `HandleNpcBotDeleteFreeCommand`（约 3698 行），跳过 BossBot：

```cpp
static bool HandleNpcBotDeleteFreeCommand(ChatHandler* handler)
{
    uint32 count = 0;
    for (uint32 creature_id : BotDataMgr::GetExistingNPCBotIds())
        if (NpcBotData const* botData = BotDataMgr::SelectNpcBotData(creature_id))
            if (botData->owner == 0)
            {
                // 跳过 BossBot：miscvalues 里带 BOSS_FLAG（key=9）的 free bot 不是游荡/可雇佣 bot
                if (botData->miscvalues.contains(BOTAI_MISC_BOSS_FLAG))
                    continue;
                if (HandleNpcBotDeleteByIdCommand(handler, creature_id))
                    ++count;
            }

    handler->PSendSysMessage("{} 个自由 npcbot 已删除", count);
    return true;
}
```

> 同理，`.npcbot list free`（约 4048 行）若想彻底隐藏 BossBot，可在 `copy_if` 的 lambda 里加 `&& !botData->miscvalues.contains(BOTAI_MISC_BOSS_FLAG)`（可选，非必须）。
>
> 结论：BossBot 以 `owner=0` 存表，能复用 free bot 的加载/召唤链路；但「按 owner==0 批量操作」的 GM 命令（`.npcbot delete free`）会把它当普通 free bot 误删，必须用 `BOTAI_MISC_BOSS_FLAG` 谓词隔离。

