# NPCBot 佣兵幻形功能设计方案

> 版本：v2.0
> 日期：2026-09-20
> 目标：玩家用「哈哈镜」收集的模型给雇佣的 BOT 变形（幻形），并持久化到数据库；BOT 在线期间保持、下线/解雇时恢复原形。

---

## 一、需求概述

1. 玩家可用自己在「哈哈镜」中收集的模型（记录在 `mod_player_transmog`）给 BOT 变形，并保持持久。
2. 在哈哈镜 `TransmogItemScript` 中新增菜单项 **「佣兵幻形」**。
3. 点击后下级菜单列出当前雇佣的 BOT（`[entry_id]BOT名称`）。
4. 点击某个 BOT 后的下级菜单，与首级菜单分类一致：普通 / 精英 / 稀有 / 史诗幻象。
5. 选中分类后列出该分类的幻象名称，点击幻象名称给 BOT 变形；**必须控制变形后模型缩放，保持与 BOT 原本模型体积一致**。
6. 将 BOT 变形数据持久化到新表 `mod_player_bot_transmog`（核心字段：`character_id`、`bot_entry`、模型 id、模型名称），服务启动时全局初始化。
7. 应用场景：
   - ① 角色登录 → 过滤该角色数据 → BOT `setowner` 时应用幻形；
   - ② 玩家在线期间幻形效果任何时候都保持（死亡/复活、被其它变形覆盖均不变）；
   - ③ 玩家下线、BOT 恢复自由时恢复原形（**保留**数据库记录，供下次上线重新应用）；
   - ④ 玩家解雇 BOT 时恢复原形，并**清理** `mod_player_bot_transmog` 表数据。

> 现有收集模型按 `account_id` 分组（账号级），而 BOT 归属是**角色级**（`character_id`），故新表用 `character_id` 作键。

---

## 二、幻形机制：写原生显示 ID（不走任何光环）

### 2.1 核心原理

核心里**恢复模型的路径，最终都回落到 `SetDisplayId(GetNativeDisplayId())`**（即在无其它变形/形态光环时恢复为原生显示 ID）：

| 恢复模型路径 | 位置 | 恢复目标 |
| --- | --- | --- |
| `Unit::RestoreDisplayId()`（无 transform/shapeshift/clone 光环时回 native） | `Unit.cpp:13954`，fallback 语句在 `14014` | `SetDisplayId(GetNativeDisplayId())` |
| `Unit::DeMorph()` | `Unit.cpp:4831` | `SetDisplayId(GetNativeDisplayId())` |
| `BotMgr::_reviveBot()`（BOT 复活） | `botmgr.cpp:439` | `SetDisplayId(GetNativeDisplayId(), ...)` |
| 变形（transform）光环失效 | `SpellAuraEffects.cpp:2935` | `RestoreDisplayId()` → 无光环时回 native |

因此，只要把幻形模型直接写入 `UNIT_FIELD_NATIVEDISPLAYID`（原生显示 ID），上述所有路径都会自动恢复成幻形模型。实现核心就两行：

```cpp
bot->SetNativeDisplayId(modelId);   // 写原生显示 ID（Unit.h:2031）
bot->SetDisplayId(modelId, scale);  // SetDisplayId 内部会 SetObjectScale(scale)
```

由此保证：

- **死亡/复活不变** ✓（复活 `_reviveBot` 恢复的是 native display id）
- **被其它变形覆盖再失效不变** ✓（`RestoreDisplayId()` 兜底回 native）
- **无需任何光环标记** ✓（从根上规避「手动 SetDisplayId + 光环」方案的失效 bug）

### 2.2 关键前提：BOT 单模型

每个 BOT 的 creature_template（entry）只对应**一个生物/一个模型**。因此 `GetFirstValidModel()` / `GetRandomValidModel()` / `ChooseDisplayId()` 对 BOT 结果一致；未幻形时 `GetNativeDisplayId()` 等于模板唯一模型（幻形后等于幻形模型）。「是否已幻形」的判断（`GetNativeDisplayId() != 模板模型`）可靠、无随机歧义。

> BOT 初始 native display id 在加载时由 `Creature.cpp:551` 设置（`SetNativeDisplayId(model.CreatureDisplayID)`）。

---

## 三、数据表设计

新建表 `mod_player_bot_transmog`，归属 **acore_characters** 库。

```sql
-- modules/mod-player-transmog/data/mod_player_bot_transmog.sql
DROP TABLE IF EXISTS `mod_player_bot_transmog`;
CREATE TABLE `mod_player_bot_transmog` (
  `character_id` INT UNSIGNED NOT NULL COMMENT '角色GUID低32位',
  `bot_entry`    INT UNSIGNED NOT NULL COMMENT 'BOT的creature entry',
  `model_id`     INT UNSIGNED NOT NULL DEFAULT '0' COMMENT '模型DisplayId',
  `model_name`   VARCHAR(255)  NOT NULL DEFAULT '' COMMENT '模型名称',
  PRIMARY KEY (`character_id`, `bot_entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='佣兵幻形数据';
```

| 字段 | 来源 | 说明 |
| --- | --- | --- |
| `character_id` | `player->GetGUID().GetCounter()` | 角色 guid 低 32 位（与 `characters_npcbot.owner` 一致） |
| `bot_entry` | `me->GetEntry()` | BOT 的 creature entry |
| `model_id` | 所选幻象的 `modelid` | 引用 `mod_player_transmog.modelid` |
| `model_name` | 所选幻象名称 | 冗余存储，便于展示 |

> `(character_id, bot_entry)` 联合主键：同一角色对同一 BOT 只有一份幻形，再次选择为覆盖更新。`model_id` 不做外键，幻象被玩家删除时由应用逻辑兜底（跳过或恢复原形）。

#### 问候语文本（world 库）

佣兵幻形 BOT 列表菜单的问候语说明写入 `npc_text`（供 5.3 的 `SendGossipMenuFor` 使用）：

```sql
-- modules/mod-player-transmog/data/mod_player_bot_transmog_gossip.sql（acore_world 库）
DELETE FROM `npc_text` WHERE `ID` = 60701;
INSERT INTO `npc_text` (`ID`, `text0_0`, `VerifiedBuild`) VALUES
(60701, '给佣兵幻形需要消耗 1 枚幸运币。请选择要幻形的佣兵：', -1);
```

> `60701` 为模块自定义 `npc_text` ID（当前 `npc_text` 中空闲，与现有无效的 `60700` 相邻）。

---

## 四、全局初始化（服务启动时）

在 `PlayerTransmog::InitData()`（由 `TransmogItem_WorldScript::OnStartup` 调用）中追加加载 `mod_player_bot_transmog`。

新增内存结构（`PlayerTransmog.h`）：

```cpp
struct BotTransmogData {
    uint32 bot_entry{};
    uint32 model_id{};
    std::string model_name;
};

class PlayerTransmog {
    // ...现有成员...
    std::map<uint32, std::map<uint32, BotTransmogData>> BotTransmogStore; // character_id -> bot_entry -> 数据
    void LoadBotTransmogData();
    BotTransmogData* GetBotTransmog(uint32 characterId, uint32 botEntry);
    void SetBotTransmog(uint32 characterId, uint32 botEntry, uint32 modelId, std::string const& modelName);
    void RemoveBotTransmog(uint32 characterId, uint32 botEntry);
};
```

加载实现（`PlayerTransmog.cpp`）：

```cpp
void PlayerTransmog::LoadBotTransmogData()
{
    BotTransmogStore.clear();
    QueryResult result = CharacterDatabase.Query(
        "SELECT character_id, bot_entry, model_id, model_name FROM mod_player_bot_transmog");
    if (!result) return;
    do {
        Field* f = result->Fetch();
        BotTransmogData d;
        uint32 cid = f[0].Get<uint32>();
        d.bot_entry  = f[1].Get<uint32>();
        d.model_id   = f[2].Get<uint32>();
        d.model_name = f[3].Get<std::string>();
        BotTransmogStore[cid][d.bot_entry] = d;
    } while (result->NextRow());
}
```

---

## 五、哈哈镜菜单设计（佣兵幻形）

### 5.1 菜单枚举

```cpp
enum TransmogItemEnum {
    // ...现有...
    GOSSIP_SENDER_USE = 4000,
    // 新增：佣兵幻形
    GOSSIP_SENDER_BOT_MAIN     = 5000,   // 点击「佣兵幻形」入口
    GOSSIP_SENDER_BOT_SELECT   = 5100,   // 选中某个 BOT（action = bot_entry）
    GOSSIP_SENDER_BOT_CATEGORY = 5200,   // 选中分类（action = quality）
    GOSSIP_SENDER_BOT_MODEL    = 5300,   // 选中幻象（action = model_id）
    GOSSIP_SENDER_BOT_BACK     = 5400    // 返回上一级
};

// 佣兵幻形：消耗品与问候语文本 ID
constexpr uint32 BOT_TRANSMOG_COIN_ENTRY     = 63000;  // 幸运币 item entry（每次幻形消耗 1 枚）
constexpr uint32 BOT_TRANSMOG_GOSSIP_TEXT_ID = 60701;  // 佣兵幻形 BOT 列表菜单问候语 npc_text ID
```

**多级状态携带**：哈哈镜是无状态 gossip，从「选 BOT → 分类 → 幻象 → 应用」需同时记住 `bot_entry` 与 `model_id`，用模块级瞬态缓存记录当前选中 BOT：

```cpp
std::unordered_map<ObjectGuid, uint32> BotTransmogSelectedEntry; // player guid -> 当前选中 bot_entry
```

### 5.2 首级菜单加入「佣兵幻形」

在 `HelleGossip` 末尾追加：

```cpp
AddGossipItemFor(player, GOSSIP_ICON_TALK, "佣兵幻形", GOSSIP_SENDER_BOT_MAIN, 0);
```

### 5.3 下级菜单：列出雇佣的 BOT

```cpp
void ShowBotList(Player* player, Item* item)
{
    player->PlayerTalkClass->ClearMenus();
    BotMap const* bots = player->GetBotMgr()->GetBotMap();
    for (auto const& [guid, bot] : *bots)
    {
        if (!bot) continue;
        std::ostringstream str;
        str << "[" << bot->GetEntry() << "]" << bot->GetName();
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, str.str(), GOSSIP_SENDER_BOT_SELECT, bot->GetEntry());
    }
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_BACK_HOME, 0);
    // 用专属问候语 textId，说明「幻形需消耗 1 枚幸运币」
    SendGossipMenuFor(player, BOT_TRANSMOG_GOSSIP_TEXT_ID, item->GetGUID());
}
```

### 5.4 下级菜单：分类（与首级一致）

```cpp
// case GOSSIP_SENDER_BOT_SELECT：
BotTransmogSelectedEntry[player->GetGUID()] = action;   // 记住 bot_entry
player->PlayerTalkClass->ClearMenus();
AddGossipItemFor(player, GOSSIP_ICON_CHAT, "普通幻象", GOSSIP_SENDER_BOT_CATEGORY, 0);
AddGossipItemFor(player, GOSSIP_ICON_CHAT, "精英幻象", GOSSIP_SENDER_BOT_CATEGORY, 1);
AddGossipItemFor(player, GOSSIP_ICON_CHAT, "稀有幻象", GOSSIP_SENDER_BOT_CATEGORY, 2);
AddGossipItemFor(player, GOSSIP_ICON_CHAT, "史诗幻象", GOSSIP_SENDER_BOT_CATEGORY, 3);
AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_BOT_MAIN, 0);
SendGossipMenuFor(player, textId, item->GetGUID());
```

### 5.5 下级菜单：列出分类幻象

```cpp
// case GOSSIP_SENDER_BOT_CATEGORY：
uint32 bot_entry = BotTransmogSelectedEntry[player->GetGUID()];
uint16 account_id = player->GetSession()->GetAccountId();
QualityGroupMap* qg = pTransmog->GetAccountQualityGroupMap(account_id);
auto it = qg->find(action);
if (it != qg->end())
    for (auto& m : it->second)
        // 选中幻象即弹确认框（popupText），点「确定」后才触发 GOSSIP_SENDER_BOT_MODEL
        AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, pTransmog->GetModelNameText(&m),
                         GOSSIP_SENDER_BOT_MODEL, m.modelid,
                         "给佣兵幻形需要消耗 1 枚幸运币，是否确定？", 0, false);
AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_BOT_SELECT, bot_entry);
SendGossipMenuFor(player, textId, item->GetGUID());
```

### 5.6 应用幻形

```cpp
// case GOSSIP_SENDER_BOT_MODEL：（仅在 5.5 确认框点「确定」后触发）
uint32 bot_entry  = BotTransmogSelectedEntry[player->GetGUID()];
uint32 model_id   = action;
uint16 account_id = player->GetSession()->GetAccountId();

Creature* bot = nullptr;
for (auto const& [guid, b] : *player->GetBotMgr()->GetBotMap())
    if (b && b->GetEntry() == bot_entry) { bot = b; break; }

if (!bot || !bot->IsInWorld() || !bot->IsAlive()) {
    ChatHandler(player->GetSession()).SendSysMessage("该 BOT 当前不在场，无法幻形。");
} else if (!player->HasItemCount(BOT_TRANSMOG_COIN_ENTRY, 1)) {
    ChatHandler(player->GetSession()).SendSysMessage("幸运币不足，无法给佣兵幻形。");
} else if (pTransmog->CastTransmogBot(bot, model_id)) {
    player->DestroyItemCount(BOT_TRANSMOG_COIN_ENTRY, 1, true);   // 幻形成功后消耗 1 枚幸运币
    ModelData* m = pTransmog->GetModelDataById(account_id, model_id);
    std::string name = m ? m->modelname : std::to_string(model_id);
    pTransmog->SetBotTransmog(player->GetGUID().GetCounter(), bot_entry, model_id, name); // 持久化
    ChatHandler(player->GetSession()).SendSysMessage("BOT 幻形成功。");
} else {
    ChatHandler(player->GetSession()).SendSysMessage("幻形失败。");
}
CloseGossipMenuFor(player);
```

---

## 六、模型大小缩放控制

### 6.1 缩放公式

模型最终体积 ≈ `ObjectScale × 模型尺寸系数`，尺寸系数取 `GeoBox(Z 高度) × CreatureModelData.Scale × CreatureDisplayInfo.scale`
（详见 6.3 ①）。

```
若 模板模型 DisplayScale ≤ 0 → 计算失败（返回 false）：调用方回退模板 scale / 放弃幻形（避免算出 0 缩放）

基础缩放 base = 模板模型 DisplayScale × 原模型尺寸系数 / 幻形模型尺寸系数   // 高度对齐，等于 BOT 原高度
高度比 ratio  = 幻形模型尺寸系数 / 原模型尺寸系数

ratio > 2.0（目标高出 100% 以上）→ 缩放 = base × 1.2
ratio > 1.5（目标高出 50% 以上）  → 缩放 = base × 1.1
其余                              → 缩放 = base

加成只增不减，故最终缩放恒 ≥ base（幻形后的高度不会小于 BOT 原本高度）
```

> 计算统一收敛到核心的 `Creature::CalculateBotTransmogScale()`，`GetNativeObjectScale()` 与
> `CastTransmogBot()` 两处共用，避免口径漂移（此前两处曾分别为 1.1 / 1.2）。

### 6.2 `CastTransmogBot` 实现

```cpp
bool PlayerTransmog::CastTransmogBot(Creature* bot, uint32 modelId)
{
    if (!bot || !bot->IsNPCBot()) return false;

    // 1) 校验幻形模型（用 LookupEntry，避免无效 DisplayId 触发断言崩溃）
    CreatureDisplayInfoEntry const* minfo = sCreatureDisplayInfoStore.LookupEntry(modelId);
    if (!minfo) return false;

    // 2) 取 BOT 模板唯一模型作为高度基准（BOT 单模型，entry 唯一对应一个生物）
    CreatureModel const* tmpl = bot->GetCreatureTemplate()->GetFirstValidModel();
    if (!tmpl) return false;

    // 3) 计算缩放：与 Creature::GetNativeObjectScale() 共用同一实现（6.1 公式）
    float scale = 0.f;
    if (!Creature::CalculateBotTransmogScale(tmpl->CreatureDisplayID, modelId, tmpl->DisplayScale, scale))
        return false;                       // 幻形模型数据异常，放弃幻形

    // 4) 直接写原生显示 ID + 显示 ID + 缩放（无任何光环）
    bot->SetNativeDisplayId(modelId);
    bot->SetDisplayId(modelId, scale);

    return true;
}
```

### 6.3 缩放持久需要改动的两处核心

「写原生显示 ID」解决了**模型**持久；但 `Creature::GetNativeObjectScale()` 读的是 creature_template 而非 native display id，因此**缩放**还需两处小改：

#### ① `Creature::GetNativeObjectScale()`（Creature.cpp）

幻形缩放计算抽出为 `Creature::CalculateBotTransmogScale()`（静态，见 6.1 公式），本函数直接复用：

```cpp
bool Creature::CalculateBotTransmogScale(uint32 srcDisplayId, uint32 dstDisplayId, float srcDisplayScale, float& outScale)
{
    if (srcDisplayScale <= 0.f) return false;   // 模板 scale 异常，放弃计算（否则会算出 0 缩放）

    // 尺寸系数 = GeoBox(Z 高度) × CreatureModelData.Scale × CreatureDisplayInfo.scale
    // （取 Z 高度而非三轴最大值：伊利丹等模型 X/Y 包围盒含武器与张臂姿态，会把模型缩得过小）
    float srcSize = BotTransmogDisplaySizeFactor(srcDisplayId);
    float dstSize = BotTransmogDisplaySizeFactor(dstDisplayId);
    // ...拿不到包围盒时退化为 ModelScale × DisplayInfo.scale...

    // 1) 高度归一
    float baseScale = srcDisplayScale * srcSize / dstSize;
    // 2) 目标远高于 BOT 原模型时补体量：高出 100% 以上 +20%，高出 50% 以上 +10%
    float scale = baseScale;
    float ratio = dstSize / srcSize;
    if (ratio > 2.0f)      scale *= 1.2f;
    else if (ratio > 1.5f) scale *= 1.1f;
    // 3) 加成只增不减，最终缩放恒 ≥ base
    outScale = scale;
    return true;
}

float Creature::GetNativeObjectScale() const
{
    // BOT 幻形：原生显示 ID 已被改成幻形模型 → 返回高度归一后的 scale
    if (IsNPCBot())
    {
        CreatureModel const* tmpl = GetCreatureTemplate()->GetFirstValidModel();
        if (tmpl && GetNativeDisplayId() != tmpl->CreatureDisplayID)
        {
            float scale = 0.f;
            if (CalculateBotTransmogScale(tmpl->CreatureDisplayID, GetNativeDisplayId(), tmpl->DisplayScale, scale))
                return scale;
        }
    }

    return ObjectMgr::ChooseDisplayId(GetCreatureTemplate())->DisplayScale;
}
```

#### ② `BotMgr::_reviveBot()`（botmgr.cpp:439）

```cpp
// 改前：
bot->SetDisplayId(bot->GetNativeDisplayId(), bot->GetCreatureTemplate()->GetFirstValidModel()->DisplayScale);
// 改后：
bot->SetDisplayId(bot->GetNativeDisplayId(), bot->GetNativeObjectScale());
```

> - 非幻形 BOT：`GetNativeObjectScale()` 仍返回模板 scale，行为不变。
> - `Unit::RecalculateObjectScale()` 无需改动：幻形 BOT 不挂 100004 光环，走 `else` 分支 `GetNativeObjectScale() + CalculatePct(1.0f, scaleAuras)`，`GetNativeObjectScale()` 已返回归一 scale。
> - 玩家幻形仍走 100004 光环，不受影响。

---

## 七、生命周期钩子设计（推荐路径：新增核心钩子）

NPCBots 生命周期在核心（非模块），模块无法被核心直接回调，故新增 **`sScriptMgr` 钩子** 解耦：在 `UnitScript` 新增 `OnBotSetOwner` / `OnBotReset`，核心 `bot_ai.cpp` 触发，模块注册处理。

### 7.1 关键生命周期事实

```cpp
enum BotAIResetType {
    BOTAI_RESET_INIT = 0x01, BOTAI_RESET_DISMISS = 0x02,
    BOTAI_RESET_UNBIND = 0x04, BOTAI_RESET_LOGOUT = 0x08, BOTAI_RESET_FORCERECALL = 0x10,
    BOTAI_RESET_MASK_RESET_MASTER = (INIT|DISMISS|UNBIND|LOGOUT)
};
```

- **下线（LOGOUT）**：`_botData->owner` **不变**，BOT 仅临时自由 → 只恢复原形，**保留** DB 记录。
- **解雇（DISMISS）**：`botmgr.cpp:857-859` 把 `_botData->owner` 重置为 0 → 恢复原形并**清理** DB 记录。

### 7.2 核心改动（4 个文件）

#### ① `Scripting/ScriptDefines/UnitScript.h`

```cpp
enum UnitHook {
    // ...现有...
    UNITHOOK_ON_BOT_SET_OWNER,   // 新增
    UNITHOOK_ON_BOT_RESET,       // 新增
    UNITHOOK_END
};

// UnitScript 内新增虚函数
virtual void OnBotSetOwner(Unit* /*bot*/, Player* /*owner*/) { }
virtual void OnBotReset(Unit* /*bot*/, uint8 /*resetType*/) { }
```

#### ② `Scripting/ScriptMgr.h`

```cpp
void OnBotSetOwner(Unit* bot, Player* owner);
void OnBotReset(Unit* bot, uint8 resetType);
```

#### ③ `Scripting/ScriptDefines/UnitScript.cpp`

```cpp
void ScriptMgr::OnBotSetOwner(Unit* bot, Player* owner)
{
    CALL_ENABLED_HOOKS(UnitScript, UNITHOOK_ON_BOT_SET_OWNER, script->OnBotSetOwner(bot, owner));
}
void ScriptMgr::OnBotReset(Unit* bot, uint8 resetType)
{
    CALL_ENABLED_HOOKS(UnitScript, UNITHOOK_ON_BOT_RESET, script->OnBotReset(bot, resetType));
}
```

#### ④ `AI/NpcBots/bot_ai.cpp`（触发点）

`SetBotOwner` 成功后（`bot_ai.cpp:418` 附近）：

```cpp
master = newowner;
_checkOwershipTimer = BotCfg::GetOwnershipExpireTime() ? CalculateOwnershipCheckTime() : 0;

sScriptMgr->OnBotSetOwner(me, newowner);   // ★新增：通知模块应用幻形
```

`ResetBotAI` 末尾（`bot_ai.cpp:611` 附近）：

```cpp
sScriptMgr->OnBotReset(me, resetType);     // ★新增：通知模块恢复原形/清理
```

> - `ResetBotAI` 在 `bot_ai` 构造函数（bot_ai.cpp:248）中会以 `BOTAI_RESET_INIT` 被调用，模块侧对 `BOTAI_RESET_INIT` 直接忽略（此时 `_botData->owner` 尚未归属玩家）。
> - **owner 清零时序（已确认）**：`RemoveBot` 中先执行 `ResetBotAI(resetType)`（botmgr.cpp:850），后执行 owner 清零 `UpdateNpcBotData(NPCBOT_UPDATE_OWNER, 0)`（botmgr.cpp:859）。因此钩子在 `ResetBotAI` 末尾触发时 `_botData->owner` 仍是原主人，模块侧可直接读取，无需缓存或改 `RemoveBot`。

### 7.3 模块侧处理（`TransmogBot_UnitScript`）

```cpp
class TransmogBot_UnitScript : public UnitScript
{
public:
    TransmogBot_UnitScript() : UnitScript("TransmogBot_UnitScript") { }

    // ① setowner 成功：应用幻形
    void OnBotSetOwner(Unit* unit, Player* owner) override
    {
        Creature* bot = unit->ToCreature();
        if (!bot || !owner) return;
        uint32 cid = owner->GetGUID().GetCounter();
        BotTransmogData* d = pTransmog->GetBotTransmog(cid, bot->GetEntry());
        if (d && d->model_id)
            pTransmog->CastTransmogBot(bot, d->model_id);
    }

    // ③④ 下线/解雇：恢复原形；解雇额外清理 DB
    void OnBotReset(Unit* unit, uint8 resetType) override
    {
        Creature* bot = unit->ToCreature();
        if (!bot) return;

        bool isDismiss = (resetType == BOTAI_RESET_DISMISS);
        bool isLogout  = (resetType == BOTAI_RESET_LOGOUT);
        if (!isDismiss && !isLogout) return;

        pTransmog->RestoreBotTransmog(bot);

        if (isDismiss) {
            // 此时 owner 尚未清零（清零发生在 ResetBotAI 返回之后的 botmgr.cpp:859），直接读取即可
            uint32 ownerLow = bot->GetBotAI() ? bot->GetBotAI()->GetBotData()->owner : 0;
            if (ownerLow)
                pTransmog->RemoveBotTransmog(ownerLow, bot->GetEntry());
        }
    }
};
```

### 7.4 在线期间保持（需求 ②）

「写原生显示 ID」后，死亡/复活、被其它变形覆盖都能自动恢复幻形。为覆盖极少数直接改 `UNIT_FIELD_DISPLAYID` 而未经过 `RestoreDisplayId` 的路径，增加低频兜底巡检（`WorldScript::OnUpdate`）：

```cpp
void OnUpdate(uint32 diff) override
{
    if (_checkTimer > diff) { _checkTimer -= diff; return; }
    _checkTimer = 1000;

    for (auto const& [cid, entryMap] : pTransmog->BotTransmogStore)
    {
        Player* pl = ObjectAccessor::FindPlayerByLowGUID(cid);
        if (!pl || !pl->GetBotMgr()) continue;
        for (auto const& [entry, d] : entryMap)
        {
            if (!d.model_id) continue;
            for (auto const& [guid, bot] : *pl->GetBotMgr()->GetBotMap())
            {
                if (bot && bot->GetEntry() == entry && bot->IsInWorld() && bot->IsAlive()
                    && (bot->GetDisplayId() != d.model_id || bot->GetNativeDisplayId() != d.model_id))
                    pTransmog->CastTransmogBot(bot, d.model_id);
            }
        }
    }
}
```

### 7.5 恢复原形（`RestoreBotTransmog`）

```cpp
void PlayerTransmog::RestoreBotTransmog(Creature* bot)
{
    if (!bot) return;

    // 恢复到模板唯一模型的显示 ID 与 scale（BOT 单模型，直接取模板）
    CreatureModel const* tmpl = bot->GetCreatureTemplate()->GetFirstValidModel();
    if (!tmpl) return;

    bot->SetNativeDisplayId(tmpl->CreatureDisplayID);
    bot->SetDisplayId(tmpl->CreatureDisplayID, tmpl->DisplayScale);
}
```

---

## 八、DB 读写（持久化）

```cpp
// 写入（覆盖更新，幂等）
void PlayerTransmog::SetBotTransmog(uint32 cid, uint32 botEntry, uint32 modelId, std::string const& modelName)
{
    BotTransmogData d{ botEntry, modelId, modelName };
    BotTransmogStore[cid][botEntry] = d;

    CharacterDatabase.DirectExecute(
        "INSERT INTO mod_player_bot_transmog (character_id, bot_entry, model_id, model_name) "
        "VALUES ({}, {}, {}, '{}') ON DUPLICATE KEY UPDATE model_id=VALUES(model_id), model_name=VALUES(model_name)",
        cid, botEntry, modelId, modelName);
}

// 删除
void PlayerTransmog::RemoveBotTransmog(uint32 cid, uint32 botEntry)
{
    BotTransmogStore[cid].erase(botEntry);
    if (BotTransmogStore[cid].empty())
        BotTransmogStore.erase(cid);

    CharacterDatabase.DirectExecute(
        "DELETE FROM mod_player_bot_transmog WHERE character_id={} AND bot_entry={}", cid, botEntry);
}
```

> 模型名称含中文/特殊字符时需转义（`CharacterDatabase.EscapeString(modelName)`）。

---

## 九、实现清单（文件级）

### 核心（src/server/game）

| 文件 | 改动 |
| --- | --- |
| `Scripting/ScriptDefines/UnitScript.h` | 新增 `UNITHOOK_ON_BOT_SET_OWNER`、`UNITHOOK_ON_BOT_RESET`；新增两个虚函数 |
| `Scripting/ScriptMgr.h` | 新增 `OnBotSetOwner`、`OnBotReset` 声明 |
| `Scripting/ScriptDefines/UnitScript.cpp` | 新增两个分发器实现 |
| `AI/NpcBots/bot_ai.cpp` | `SetBotOwner` 成功处触发 `OnBotSetOwner`；`ResetBotAI` 末尾触发 `OnBotReset` |
| `Entities/Creature/Creature.cpp` | `GetNativeObjectScale()` 增加 BOT 幻形归一分支（6.3 ①） |
| `AI/NpcBots/botmgr.cpp` | `_reviveBot()` 复活 scale 参数改为 `GetNativeObjectScale()`（6.3 ②） |

### 模块（modules/mod-player-transmog）

| 文件 | 改动 |
| --- | --- |
| `src/PlayerTransmog.h` | 新增 `BotTransmogData`、`BotTransmogStore`、`BotTransmogSelectedEntry` 与 4 个方法声明 |
| `src/PlayerTransmog.cpp` | 实现 `LoadBotTransmogData`、`GetBotTransmog`、`SetBotTransmog`、`RemoveBotTransmog`、`RestoreBotTransmog`；修正 `CastTransmogBot`；`InitData` 追加加载 |
| `src/TmItemScript.cpp` | 新增 `GOSSIP_SENDER_BOT_*` 枚举、`ShowBotList`、四分类/幻象子菜单；幻形消耗幸运币（63000）；新增 `TransmogBot_UnitScript`、`TransmogBot_PlayerScript`、`TransmogBot_WorldScript`；注册到 `AddSC_TransmogItemScript` |

### SQL

| 文件 | 内容 |
| --- | --- |
| `modules/mod-player-transmog/data/mod_player_bot_transmog.sql` | 建表 `mod_player_bot_transmog`（character 库） |
| `modules/mod-player-transmog/data/mod_player_bot_transmog_gossip.sql` | 新增 `npc_text` 60701 问候语（world 库） |

---

## 十、关键代码骨架（模块侧脚本注册）

```cpp
// TmItemScript.cpp 追加
class TransmogBot_PlayerScript : public PlayerScript
{
public:
    TransmogBot_PlayerScript() : PlayerScript("TransmogBot_PlayerScript") { }

    // ① 登录：数据已在内存全局加载，实际应用在 OnBotSetOwner 中完成，此处无需额外处理
    void OnPlayerLogin(Player* player) override { }

    // ③ 下线：BOT 恢复自由由核心 ResetBotAI(LOGOUT) -> OnBotReset 处理，这里仅清理瞬态缓存
    void OnPlayerLogout(Player* player) override
    {
        BotTransmogSelectedEntry.erase(player->GetGUID());
    }
};

void AddSC_TransmogItemScript()
{
    // ...现有注册...
    new TransmogBot_UnitScript();
    new TransmogBot_PlayerScript();
    new TransmogBot_WorldScript();   // 兜底巡检
}
```

---

## 十一、注意事项与边界

1. **`AssertEntry` → `LookupEntry`**：`CastTransmogBot` 中 `sCreatureDisplayInfoStore.AssertEntry` 在 id 无效时会 assert 崩溃，务必改用 `LookupEntry`。
2. **模型被玩家删除**：幻象删除后 BOT 幻形记录仍在；应用时若 `LookupEntry(model_id)` 为空，应跳过应用并（可选）清理孤儿记录。
3. **BOT 不在场**：点幻象时 BOT 可能已死/已传送，菜单点击处需校验 `IsInWorld() && IsAlive()`。
4. **解雇 owner 读取顺序（已确认）**：`RemoveBot` 中 `ResetBotAI(DISMISS)`（botmgr.cpp:850）先执行，owner 清零 `UpdateNpcBotData(NPCBOT_UPDATE_OWNER, 0)`（botmgr.cpp:859）后执行；且 `bot_ai::_botData` 与 `BotDataMgr::_botsData[entry]` 是同一份 `NpcBotData` 对象（`botdatamgr.cpp:2967` 才改 `owner=0`）。因此钩子在 `ResetBotAI` 末尾触发时 `_botData->owner` 仍是原主人，模块侧直接 `GetBotData()->owner` 读取即可，无需缓存或额外传参。
5. **线程/DB 写入**：`RemoveBot` 注释（botmgr.cpp:794）标明其「主线程（命令行/登出）与地图线程（遍历更新）都可能调用」，故 `OnBotReset` 可能在地图线程触发。此时 `DirectExecute` 的同步 DB 写线程安全性需确认；稳妥做法是写入统一走 `CharacterDatabase.AsyncQuery` 或排队到主线程执行。
6. **共享 BOT**：本方案仅针对「主 owner」的幻形；共享 BOT 被解雇（`UNBIND`）时 `resetType` 为 `UNBIND`，本方案不清理，符合「仅主 owner 解雇才清理」语义。
7. **缩放归一**：统一采用 `模板DisplayScale / 幻形模型scale` 保证体积一致；如产品要求「小模型不放大」，`CastTransmogBot` 中仅在 `minfo->scale > tmpl->DisplayScale` 时缩小即可。
8. **原生显示 ID 副作用**：幻形后 `GetNativeDisplayId()` 不再等于模板模型。`bot_ai.cpp:18586` 的 bounding radius 重算条件 `GetDisplayId() == GetNativeDisplayId()` 会恒真（按归一 scale 重算，符合预期）；`bot_necromancer_ai.cpp:757`、`bot_crypt_lord_ai.cpp:827` 的 `_isUsableCorpse` 判断均带 `!c->IsNPCBot()` 守卫，BOT 尸体被显式排除，幻形对这两处**无影响**。
9. **中文注释**：所有新增代码注释、文档、日志均使用中文；日志统一 `LOG_INFO("module.transmog", "...")` 风格。
10. **幸运币消耗**：幻象选项带 `popupText` 确认框，点「确定」后才进入 `GOSSIP_SENDER_BOT_MODEL` 分支；`CastTransmogBot` 成功后调用 `player->DestroyItemCount(63000, 1, true)` 扣 1 枚，检查用 `player->HasItemCount(63000, 1)`。物品不足时仅提示、不幻形、不扣币；点「取消」不触发任何逻辑。

---

## 附：菜单交互流程序列

```
玩家使用哈哈镜
 └─ 首级菜单
      ├─ [收藏幻象列表]（原有）
      ├─ 普通/精英/稀有/史诗幻象（原有）
      └─ 【佣兵幻形】(GOSSIP_SENDER_BOT_MAIN)
           └─ BOT 列表: [entry]名称 ×N (GOSSIP_SENDER_BOT_SELECT, action=bot_entry)
                └─ 四分类 (GOSSIP_SENDER_BOT_CATEGORY, action=quality)  ← 记录选中 bot_entry
                     └─ 幻象列表 (GOSSIP_SENDER_BOT_MODEL, action=model_id)
                          └─ 应用: CastTransmogBot + SetBotTransmog(持久化) → 关闭菜单
```
