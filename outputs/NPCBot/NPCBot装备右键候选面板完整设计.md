# NPCBot 装备右键候选面板完整设计

## 1. 文档目的

本文定义 AzerothCore WotLK with NPCBots 项目中，基于 `mod-ale` 与 `Rochet2/AIO` 实现 NPCBot 仿玩家装备界面的“右键槽位换装”方案。

目标交互如下：

1. 玩家打开 NPCBot 装备框。
2. 玩家右键任意 NPCBot 装备槽。
3. 客户端向服务器请求该 Bot、该槽位的可装备候选物品。
4. 服务器扫描玩家主背包和已装备背包中的全部物品，并调用 NPCBot 现有装备规则过滤。
5. 客户端将“卸下”固定放在候选列表第一项；当前槽位有装备时可点击，用于卸下该槽位装备。
6. 其他候选物品在槽位右侧显示，每行固定 4 件，超过 4 件自动换行。
7. 鼠标指向装备候选时显示原生 `GameTooltip`。
8. 左键“卸下”或其他候选物品后，服务器再次校验并执行操作。
9. 服务器返回最新完整装备快照，客户端以服务器状态刷新全部 18 个槽位。

本文是技术设计，不包含实际 C++、Lua 或 SQL 修改，也未执行构建。本轮不新增独立 C++ Script 文件；Bot 相关服务统一规划在 `bot_mgr_service.h/.cpp`，以后其他 Bot 功能继续扩展到该服务中。

---

## 2. 核心工程结论

### 2.1 不在 Lua 中重写装备规则

NPCBot 已在 `bot_ai::_canEquip()` 中实现完整装备约束，包括：

- Bot 等级与职业限制；
- 武器、护甲和 InventoryType 匹配；
- 主手、副手、远程槽规则；
- 双持和双手武器组合冲突；
- 戒指、饰品双槽逻辑；
- 原始装备与实际装备处理；
- Item 实例随机属性参与判断；
- 装备替换和关联槽卸装。

因此客户端和服务器 Lua 只负责请求、显示和编排，最终候选判断必须调用现有 `_canEquip()`，最终换装必须调用现有 `_equip()`。

### 2.2 候选物品必须以 Item GUID 标识

Item Entry 只表示模板，不能唯一标识玩家背包中的物品实例。相同 Entry 可能具有不同的：

- 附魔；
- 宝石；
- 随机属性；
- 耐久度；
- 绑定状态；
- 所在背包位置。

所有候选项和换装请求都必须携带 Item GUID Low；Item Entry 只用于一致性校验和展示。

### 2.3 客户端候选列表不是权限依据

候选响应只能作为短时显示快照。收到点击请求后，服务器必须重新验证：

- Bot 仍存在且仍归当前玩家管理；
- 共享所有者仍具有装备管理权限；
- 槽位仍合法；
- Item GUID 仍对应同一物品；
- 物品仍属于当前玩家；
- 物品仍在允许的背包区域；
- 物品不在交易中；
- 当前装备状态未过期；
- `_canEquip()` 仍返回可装备；
- `_equip()` 最终执行成功。

### 2.4 同 Entry 的实例不去重

现有 Gossip 对带随机属性的物品存在按 Entry 去重行为：

```cpp
(pItem->GetItemRandomPropertyId() == 0 ||
    !idsList.contains(pItem->GetEntry()))
```

这不适合可视化候选面板，因为同 Entry 的不同实例可能有不同附魔、宝石或随机属性。新界面应逐 Item GUID 展示，不按 Entry 粗暴去重。

如果后续确实需要压缩候选数量，只能按“完整实例指纹”去重，例如：

```text
Entry + ItemLink + Durability + BindingState
```

第一版不去重，确保玩家能选择准确实例。

---

## 3. 已核实的现有源码基础

### 3.1 18 个 NPCBot 装备槽

文件：`src/server/game/AI/NpcBots/botcommon.h`

| Bot 槽位 | 枚举 | 中文名称 |
|---:|---|---|
| 0 | `BOT_SLOT_MAINHAND` | 主手 |
| 1 | `BOT_SLOT_OFFHAND` | 副手 |
| 2 | `BOT_SLOT_RANGED` | 远程 |
| 3 | `BOT_SLOT_HEAD` | 头部 |
| 4 | `BOT_SLOT_SHOULDERS` | 肩部 |
| 5 | `BOT_SLOT_CHEST` | 胸部 |
| 6 | `BOT_SLOT_WAIST` | 腰部 |
| 7 | `BOT_SLOT_LEGS` | 腿部 |
| 8 | `BOT_SLOT_FEET` | 脚部 |
| 9 | `BOT_SLOT_WRIST` | 护腕 |
| 10 | `BOT_SLOT_HANDS` | 手套 |
| 11 | `BOT_SLOT_BACK` | 披风 |
| 12 | `BOT_SLOT_BODY` | 衬衣 |
| 13 | `BOT_SLOT_FINGER1` | 戒指 1 |
| 14 | `BOT_SLOT_FINGER2` | 戒指 2 |
| 15 | `BOT_SLOT_TRINKET1` | 饰品 1 |
| 16 | `BOT_SLOT_TRINKET2` | 饰品 2 |
| 17 | `BOT_SLOT_NECK` | 项链 |

边界值为 `BOT_INVENTORY_SIZE`，所有外部输入必须验证：

```cpp
if (slot >= BOT_INVENTORY_SIZE)
    return InvalidSlot;
```

### 3.2 当前 Gossip 候选筛选

文件：`src/server/game/AI/NpcBots/bot_ai.cpp`

`GOSSIP_SENDER_EQUIPMENT_SHOW` 已按槽位扫描主背包和装备包，并使用：

```cpp
_canEquip(pItem->GetTemplate(), slot, true, pItem)
```

其中 `ignoreItemLevel = true` 表示手动换装列表不因装备评分较低而隐藏候选。新 UI 应保持这个手动选择语义。

现有扫描范围：

```cpp
for (uint8 i = INVENTORY_SLOT_ITEM_START;
    i != INVENTORY_SLOT_ITEM_END;
    ++i)
{
    try_put_item(INVENTORY_SLOT_BAG_0, i);
}

for (uint8 i = INVENTORY_SLOT_BAG_START;
    i != INVENTORY_SLOT_BAG_END;
    ++i)
{
    if (Bag const* bag = player->GetBagByPos(i))
    {
        for (uint32 j = 0; j != bag->GetBagSize(); ++j)
            try_put_item(i, j);
    }
}
```

第一版候选面板应保持相同扫描边界：

- 包含主背包；
- 包含玩家已装备的普通背包；
- 不包含银行；
- 不包含装备银行；
- 不包含邮件、交易栏和拍卖行；
- 不包含玩家当前已穿戴装备。

### 3.3 当前装备执行

现有 Gossip 使用 Item GUID Low 重新定位物品，然后调用：

```cpp
_equip(slot, item, player->GetGUID(), false)
```

新 UI 的点击请求也必须走同一核心逻辑，但应将查找、校验、执行和结果构造放入一个 C++ 原子服务方法，避免 Lua 分多步持有过期对象。

### 3.4 当前核心方法是私有方法

文件：`src/server/game/AI/NpcBots/bot_ai.h`

```cpp
bool _canEquip(
    ItemTemplate const* newProto,
    uint8 slot,
    bool ignoreItemLevel,
    Item const* newItem = nullptr,
    bool ignore_combine = false) const;

[[nodiscard]] BotEquipResult _unequip(
    uint8 slot,
    ObjectGuid receiver,
    bool store_to_bank,
    bool on_equip_from_bank = false);

[[nodiscard]] BotEquipResult _equip(
    uint8 slot,
    Item* newItem,
    ObjectGuid receiver,
    bool store_to_bank,
    bool from_bank = false);
```

不建议简单把三个方法整体改成 public。推荐增加专用服务类，并仅将该服务声明为 `bot_ai` 的 friend，使权限校验集中在一个入口。

### 3.5 ItemLink 可以支持原生 Tooltip

文件：`modules/mod-ale/src/LuaEngine/methods/ItemMethods.h`

`Item:GetItemLink(locale)` 已生成包含以下字段的完整超链接：

- Item Entry；
- 永久附魔；
- 三个宝石槽附魔；
- 额外附魔；
- 随机属性 ID；
- 随机后缀缩放值；
- 等级上下文。

客户端可直接调用：

```lua
GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
GameTooltip:SetHyperlink(self.itemData.link)
GameTooltip:Show()
```

因此当前装备槽和候选物品都可以获得接近原生背包物品的 Tooltip 效果。

---

## 4. 总体架构

```text
NPCBot 装备槽右键
        |
        v
AIO 客户端 RequestCandidates
        |
        v
AIO 服务端 Lua Handler
        |
        v
ALE PlayerMethods 窄接口
        |
        v
bot_mgr_service
  - 解析并限定当前玩家的 Bot
  - 验证所有者/共享权限
  - 扫描主背包和装备包
  - 调用 bot_ai::_canEquip()
  - 构造候选快照
  - 预留其他 Bot 相关服务
        |
        v
AIO CandidatesResult
        |
        v
客户端 4 列悬浮候选面板
        |
      左键物品
        |
        v
AIO 服务端 EquipCandidate
        |
        v
bot_mgr_service 原子重验证
  - 重新找 Bot
  - 重新找 Item GUID
  - 检查背包位置和所有权
  - 检查装备状态指纹
  - 再次调用 _canEquip()
  - 调用 _equip()
        |
        v
EquipResult + 完整 18 槽快照
        |
        v
客户端覆盖刷新并关闭候选面板
```

职责边界：

| 层 | 负责 | 不负责 |
|---|---|---|
| 客户端 Lua | 右键检测、布局、Tooltip、加载状态、点击请求、过期响应丢弃 | 装备权限、装备合法性、物品所有权 |
| AIO | 客户端脚本下发、消息序列化、分片、handler 分发 | NPCBot 业务规则 |
| 服务端 Lua | 参数规整、调用 C++ 窄接口、构造 AIO 响应 | 遍历底层 Bot 装备结构、复制 `_canEquip()` |
| ALE C++ 桥接 | 把 Lua 请求转换为类型安全的 C++ 调用和 Lua table | 绕过 NPCBot 核心规则 |
| `bot_mgr_service` | 权限、扫描、实例校验、竞态校验、调用核心装备函数，并承载未来 Bot 相关服务 | UI 展示 |
| `bot_ai` | 最终装备规则和装备状态变更 | 网络协议和客户端布局 |

---

## 5. 用户交互设计

### 5.1 打开候选面板

右键装备槽时：

1. 如果当前已有候选面板，先使其失效并关闭。
2. 客户端生成递增 `requestId`。
3. 记录当前 `botKey`、`botSlot` 和装备快照中的 `equipmentRevision`。
4. 在目标槽位右侧显示加载面板。
5. 发送 `RequestCandidates`。
6. 收到响应后，仅当 `requestId`、`botKey`、`botSlot` 均与当前请求一致时更新面板。
7. 旧响应直接丢弃，禁止覆盖新槽位面板。

### 5.2 右键行为

- 右键当前已打开的同一槽位：关闭面板。
- 右键其他槽位：立即切换加载状态并请求新槽位。
- 空槽位同样允许右键请求候选。
- 右键槽位不会直接卸装。 

### 5.3 固定第一项“卸下”

“卸下”不是服务器背包候选物品，而是客户端候选面板的固定操作项，始终占据列表第一个位置（第一行第一列）。因此服务器返回的 `candidates` 数组不包含它，客户端渲染前先插入一个特殊项：

```lua
{
    kind = "UNEQUIP",
    text = "卸下",
    enabled = currentSlotItem ~= nil
}
```

规则：

- 当前槽位有装备：显示可用的“卸下”，左键发送 `Unequip` 请求。
- 当前槽位为空：仍显示“卸下”，但置灰并禁止点击，保证第一项位置固定。
- “卸下”不占用服务器候选数量上限，但占用客户端布局中的一个格子；因此第一个真实物品从第一行第二列开始。
- “卸下”按钮不显示 Item Tooltip；可显示简短提示“卸下当前装备”。
- 右键“卸下”不执行操作，避免与槽位右键关闭/切换语义冲突。
- 卸下成功后关闭面板，并使用服务器返回的完整 18 槽快照刷新界面。

### 5.4 候选物品点击

- 左键装备候选：请求装备该 Item GUID。
- 右键装备候选：第一版不执行装备，避免与槽位右键语义混淆。
- 点击后所有候选按钮进入禁用状态，直到成功、失败或超时。
- 装备成功：关闭面板，刷新完整装备快照。
- 装备失败：保留面板并显示错误；若错误表示状态已过期，则自动重新请求候选。

### 5.5 面板关闭条件

以下任一事件关闭并使当前请求失效：

- 再次右键同一槽位；
- 切换至其他槽位；
- NPCBot 装备窗口关闭；
- 装备窗口切换到另一 Bot；
- 按 Esc；
- 点击面板外部；
- Bot 离队、消失或服务端返回无权限；
- 换装成功；
- 玩家登出或重载 UI。

### 5.6 空列表和异常状态

| 状态 | 面板表现 |
|---|---|
| 正在请求 | 旋转或闪烁加载图标，文字“正在筛选背包装备...” |
| 无真实候选 | 仍显示固定“卸下”；若当前槽位为空则置灰 |
| 请求超时 | 文字“请求超时”，提供“重试”按钮 |
| 无权限 |  不响应操作 |
| 战斗中 | 不响应操作 |
| 状态过期 | 显示“装备状态已变化，正在刷新...”，自动重请求 |

---

## 6. 候选面板布局

### 6.1 固定 4 列

每行固定 4 件：

```lua
local column = (index - 1) % 4
local row = math.floor((index - 1) / 4)
```

按钮定位：

```lua
button:SetPoint(
    "TOPLEFT",
    content,
    "TOPLEFT",
    padding + column * (itemSize + spacing),
    -padding - row * (itemSize + spacing)
)
```

推荐参数：

```lua
local COLUMNS = 4
local ITEM_SIZE = 38
local SPACING = 4
local PADDING = 8
local MAX_VISIBLE_ROWS = 5
```

固定宽度：

```lua
local panelWidth =
    PADDING * 2 +
    ITEM_SIZE * COLUMNS +
    SPACING * (COLUMNS - 1)
```

总行数：

```lua
local displayCount = candidateCount + 1 -- 第一项固定为“卸下”
local rows = math.max(1, math.ceil(displayCount / COLUMNS))
```

内容高度：

```lua
local contentHeight =
    PADDING * 2 +
    rows * ITEM_SIZE +
    math.max(0, rows - 1) * SPACING
```

可见高度最多 5 行；“卸下”占用第一格，因此当真实服务器候选超过 19 件时启用 `ScrollFrame`。即使滚动，每个逻辑行仍固定 4 件，不改为分页或单列列表。

### 6.2 槽位右侧悬浮

默认锚点：

```lua
panel:SetPoint("TOPLEFT", slotButton, "TOPRIGHT", 8, 0)
```

在 `panel:Show()` 后检查屏幕边界：

```lua
if panel:GetRight() and UIParent:GetRight() and
    panel:GetRight() > UIParent:GetRight() then
    panel:ClearAllPoints()
    panel:SetPoint("TOPRIGHT", slotButton, "TOPLEFT", -8, 0)
end
```

如果面板底部越界，应上移或将底部锚定到屏幕底部安全区。第一版推荐：

- 水平方向优先右侧，越界时翻到左侧；
- 垂直方向限制最大 5 行，通过滚动避免高度过大；
- 最终对 `GetBottom()` 做最小边距修正。

### 6.3 候选按钮视觉

每个按钮包含：

- 物品图标；
- 原生品质边框颜色；
- 物品数量角标，仅堆叠物品需要；
- 物品等级角标，可选；
- 当前按下、悬停和禁用状态；
- 不同实例即使同 Entry 也分别显示。

推荐排序：

1. ItemLevel 降序；
2. GearScore 降序；
3. Quality 降序；
4. Bag、BagSlot 升序，保证相同数据下顺序稳定。

排序仅影响显示，不改变服务器可装备判断。

### 6.4 点击外部关闭

可创建仅在候选面板打开时显示的透明 dismiss layer：

- 覆盖 `UIParent`；
- FrameLevel 低于候选面板、高于普通装备窗口背景；
- 点击后只执行 `CloseCandidatePanel()`；
- 候选按钮和 Bot 装备槽按钮位于其上层；
- 面板关闭后立即隐藏 dismiss layer，避免影响其他 UI。

若该层与原生界面层级发生冲突，可降级为：Esc、装备框关闭、槽位切换和换装完成时关闭，不应为“点击外部关闭”破坏原生 UI 操作。

---

## 7. 候选物品数据结构

服务器返回的 `candidates` 只包含真实背包物品；客户端绘制时必须在数组最前面插入固定的 `UNEQUIP` 操作项。服务器返回的单个真实候选项：

```lua
{
    itemGuid = "123456",
    entry = 50730,
    link = "|cffa335ee|Hitem:50730:...|h[物品名]|h|r",
    icon = "Interface\\Icons\\INV_Sword_01",
    quality = 4,
    itemLevel = 277,
    gearScore = 512,
    count = 1,
    durability = 100,
    maxDurability = 100,
    bag = 1,
    bagSlot = 7
}
```

字段说明：

| 字段 | 类型 | 用途 | 是否可信操作依据 |
|---|---|---|---|
| `itemGuid` | string | 唯一 Item 实例身份 | 服务器必须重新查找 |
| `entry` | uint32 | 图标、缓存和一致性校验 | 不能单独用于装备 |
| `link` | string | 原生 Tooltip 和聊天链接 | 仅显示 |
| `icon` | string | 候选按钮图标 | 仅显示 |
| `quality` | uint8 | 品质边框 | 仅显示 |
| `itemLevel` | uint32 | 排序和角标 | 仅显示 |
| `gearScore` | uint32 | 排序和比较 | 仅显示 |
| `count` | uint32 | 堆叠数量 | 仅显示 |
| `durability` | uint32 | Tooltip 补充 | 仅显示 |
| `maxDurability` | uint32 | Tooltip 补充 | 仅显示 |
| `bag` | uint8 | 来源提示和调试 | 不能作为唯一定位 |
| `bagSlot` | uint8 | 来源提示和调试 | 不能作为唯一定位 |

`itemGuid` 使用字符串传输，避免 Lua number、序列化实现或未来 GUID 宽度变化产生精度和符号问题。

图标可由服务器通过 `ItemDisplayInfoEntry::inventoryIcon` 构造。若未取得图标，客户端再使用 Item Entry 或 ItemLink 查询本地物品缓存。

---

## 8. Bot 身份与装备版本

### 8.1 不直接信任客户端 UnitGUID

客户端右键队伍头像打开装备框后，服务器返回一个 Bot 身份对象：

```lua
botKey = {
    entry = 70001,
    guidLow = "34567"
}
```

后续请求必须同时携带 `entry` 和 `guidLow`。C++ 服务不能仅凭客户端数据构造任意 Creature GUID，而应只在当前玩家的 `BotMgr::GetBotMap()` 中查找同时匹配 Entry 和 GUID Low 的活动 Bot。

查找逻辑示意：

```cpp
Creature* FindManagedBot(
    Player* player,
    uint32 botEntry,
    ObjectGuid::LowType botGuidLow)
{
    for (auto const& [guid, bot] : *player->GetBotMgr()->GetBotMap())
    {
        if (bot &&
            bot->GetEntry() == botEntry &&
            guid.GetCounter() == botGuidLow)
        {
            return bot;
        }
    }

    return nullptr;
}
```

共享所有者的实际 Bot 获取路径若不在当前玩家的 BotMap 内，应复用现有共享所有者查找方法，但最终仍须调用 `HasOwner(playerGuidLow)` 并检查共享装备配置。

### 8.2 equipmentRevision 采用状态指纹

当前 NPCBot 核心没有覆盖所有装备入口的单调版本号。若只在 AIO 换装时自增，玩家通过 Gossip、自动装备或其他入口更改装备后会漏记。

因此第一版将 `equipmentRevision` 定义为服务器即时计算的不可解释状态指纹，而不是数据库字段或简单计数器。本方案明确采用状态指纹，不新增装备版本字段、不依赖自增计数器，也不要求在每个装备入口维护额外版本状态。

指纹至少包含 18 个槽位的：

```text
slot + itemGuidLow + itemEntry + randomPropertyId
```

可进一步包含永久附魔和三个宝石槽附魔。结果编码为十六进制或十进制字符串。

用途：

- 请求候选时返回当前指纹；
- 点击候选时携带该指纹；
- 执行前重新计算；
- 不一致则拒绝旧操作并要求刷新。

指纹只用于竞态检测，不用于权限判断，也不需要持久化到数据库。

---

## 9. AIO 消息协议

统一命名空间：

```lua
NPCBotEquipment
```

### 9.1 客户端请求候选

```lua
AIO.Handle(
    "NPCBotEquipment",
    "RequestCandidates",
    {
        requestId = 41,
        botEntry = 70001,
        botGuidLow = "34567",
        botSlot = 3,
        equipmentRevision = "a102ff09"
    }
)
```

`equipmentRevision` 可以为空；为空时服务器仍返回候选，但响应必须包含服务器当前值。

### 9.2 服务器返回候选

```lua
AIO.Handle(
    player,
    "NPCBotEquipment",
    "CandidatesResult",
    {
        requestId = 41,
        ok = true,
        code = "OK",
        botEntry = 70001,
        botGuidLow = "34567",
        botSlot = 3,
        equipmentRevision = "a102ff09",
        candidates = {
            -- CandidateItem[]
        }
    }
)
```

失败响应：

```lua
{
    requestId = 41,
    ok = false,
    code = "NO_PERMISSION",
    message = "无权管理该 NPCBot 的装备",
    botSlot = 3
}
```

客户端以 `code` 决定行为，`message` 只用于显示，不能反向参与逻辑判断。

### 9.3 客户端请求装备候选物品

```lua
AIO.Handle(
    "NPCBotEquipment",
    "EquipCandidate",
    {
        requestId = 42,
        botEntry = 70001,
        botGuidLow = "34567",
        botSlot = 3,
        itemGuid = "123456",
        expectedItemEntry = 50730,
        expectedEquipmentRevision = "a102ff09",
        storeReplacedToBank = false
    }
)
```

`storeReplacedToBank` 第一版建议固定为 `false`，即遵循现有 Gossip 普通换装行为，将被替换物品交给玩家背包。以后可在面板设置中开放装备银行选项。

### 9.4 服务器返回换装或卸下结果

成功（装备和卸下共用同一结果字段，消息名分别为 `EquipResult`、`UnequipResult`）：

```lua
AIO.Handle(
    player,
    "NPCBotEquipment",
    "EquipResult", -- 卸下时使用 "UnequipResult"
    {
        requestId = 42,
        ok = true,
        code = "OK",
        botEntry = 70001,
        botGuidLow = "34567",
        changedSlot = 3,
        operation = "EQUIP", -- 卸下时为 "UNEQUIP"
        equipmentRevision = "b81c240e",
        snapshot = {
            -- 完整 18 槽装备快照
        }
    }
)
```

失败：

```lua
{
    requestId = 42,
    ok = false,
    code = "STALE_EQUIPMENT",
    message = "NPCBot 装备状态已变化，请重新选择",
    refreshRequired = true
}
```

成功后必须返回完整 18 槽快照，禁止客户端只替换被点击槽位并推断其他槽不变。主副手关联逻辑可能同时改变多个槽位。

### 9.5 requestId 规则

- 客户端会话内单调递增即可。
- 每次候选请求、装备请求分别生成新 ID。
- 客户端只处理当前活跃请求对应的响应。
- `requestId` 不是安全令牌，不承担权限职责。

---

## 10. 后续 C++ 服务层设计（本轮暂不实现）

### 10.1 推荐新增文件

```text
src/server/game/AI/NpcBots/bot_mgr_service.h
src/server/game/AI/NpcBots/bot_mgr_service.cpp
```

命名采用 `bot_mgr_service`，而不是只面向装备的服务名。该文件作为 Bot 业务服务的统一承载点：本次放入装备候选、卸下和换装服务；以后其他 Bot 相关功能（例如 Bot 状态查询、交互权限、技能/外观管理等）继续在此服务中按模块划分扩展，避免为每个小功能散落创建独立服务文件。

本轮只完成设计，不创建上述 C++ 文件，也不接入 CMake。未来真正实现时，若该目录当前 CMake 使用显式文件列表，再同步加入构建列表；若使用目录自动收集则无需额外修改。

### 10.2 服务结果结构

```cpp
enum class BotEquipmentUiResult : uint8
{
    Ok = 0,
    InvalidRequest,
    RateLimited,
    BotNotFound,
    NoPermission,
    InvalidSlot,
    BusyInCombat,
    ItemNotFound,
    ItemMoved,
    ItemMismatch,
    StaleEquipment,
    CantEquip,
    ItemConflict,
    NoBagSpace,
    NoBankSpace,
    InternalError
};

struct BotEquipmentCandidate
{
    ObjectGuid::LowType itemGuidLow = 0;
    uint32 itemEntry = 0;
    std::string itemLink;
    std::string icon;
    uint8 quality = 0;
    uint32 itemLevel = 0;
    uint32 gearScore = 0;
    uint32 count = 0;
    uint32 durability = 0;
    uint32 maxDurability = 0;
    uint8 bag = 0;
    uint8 bagSlot = 0;
};

struct BotEquipmentSnapshot
{
    uint32 botEntry = 0;
    ObjectGuid::LowType botGuidLow = 0;
    std::string revision;
    std::array<BotEquipmentSlotSnapshot, BOT_INVENTORY_SIZE> slots;
};
```

### 10.3 服务接口

```cpp
class bot_mgr_service
{
public:
    static BotEquipmentUiResult GetCandidates(
        Player* player,
        uint32 botEntry,
        ObjectGuid::LowType botGuidLow,
        uint8 slot,
        std::vector<BotEquipmentCandidate>& candidates,
        BotEquipmentSnapshot& snapshot);

    static BotEquipmentUiResult EquipFromInventory(
        Player* player,
        uint32 botEntry,
        ObjectGuid::LowType botGuidLow,
        uint8 slot,
        ObjectGuid::LowType itemGuidLow,
        uint32 expectedItemEntry,
        std::string_view expectedRevision,
        bool storeReplacedToBank,
        BotEquipmentSnapshot& snapshot);

    static BotEquipmentUiResult Unequip(
        Player* player,
        uint32 botEntry,
        ObjectGuid::LowType botGuidLow,
        uint8 slot,
        std::string_view expectedRevision,
        bool storeToBank,
        BotEquipmentSnapshot& snapshot);
};
```

`Unequip()` 是本次第一版 UI 的正式操作，不是仅供未来预留：客户端固定显示“卸下”第一项，并在槽位有装备时调用该接口。服务类同时保留继续承载其他 Bot 相关服务的扩展空间。

### 10.4 后续服务层访问核心装备方法

本轮暂不创建或实现 C++ Script/服务文件；这里只记录未来接入点。届时在 `bot_ai` 中只增加：

```cpp
friend class bot_mgr_service;
```

这样：

- `_canEquip()`、`_equip()`、`_unequip()` 继续保持私有；
- 只有 `bot_mgr_service` 能够进入；
- ALE 不能绕过服务层直接操作任意 Bot；
- 权限、范围和竞态校验集中维护。

若项目维护者不接受 friend，可增加命名明确的窄包装器，但包装器仍应要求经过服务层，不能直接暴露给 Lua。

---

## 11. 服务器候选扫描算法

### 11.1 前置校验

`GetCandidates()` 顺序：

1. 验证 `player != nullptr` 且在线。
2. 检查请求限流。
3. 验证 `slot < BOT_INVENTORY_SIZE`。
4. 只在当前玩家可管理的 Bot 集合中查找 Entry 与 GUID Low 同时匹配的 Bot。
5. 验证对象是 NPCBot，且不是临时 Bot、召唤物或 wanderer。
6. 获取 `bot_ai`。
7. 验证主所有者或共享所有者身份。
8. 若为共享所有者，验证 `SHARED_OWNER_OPTION_MASK_EQUIPMENT`。
9. 建议拒绝玩家或 Bot 正在战斗的换装管理请求。
10. 计算当前完整装备状态指纹。

### 11.2 单件候选判断

```cpp
bool IsCandidate(
    Player* player,
    bot_ai* ai,
    EquipmentInfo const* equipmentInfo,
    Item const* item,
    uint8 slot)
{
    if (!item || item->GetOwnerGUID() != player->GetGUID())
        return false;

    if (!Player::IsInventoryPos(item->GetBagSlot(), item->GetSlot()))
        return false;

    if (item->IsInTrade())
        return false;

    if (std::ranges::any_of(
        equipmentInfo->ItemEntry,
        [item](uint32 entry)
        {
            return entry == item->GetEntry();
        }))
    {
        return false;
    }

    return ai->_canEquip(
        item->GetTemplate(),
        slot,
        true,
        item);
}
```

是否排除 `equipmentInfo->ItemEntry` 应与现有 Gossip 保持一致，第一版建议保留。若后续确认这是 Gossip 展示限制而非装备规则，再单独调整并进行回归测试。

### 11.3 遍历背包

抽取公共遍历器，候选扫描和点击后的 Item GUID 查找共用，避免两套范围不一致：

```cpp
template <typename Visitor>
void VisitPlayerInventoryItems(Player* player, Visitor&& visitor)
{
    for (uint8 slot = INVENTORY_SLOT_ITEM_START;
        slot != INVENTORY_SLOT_ITEM_END;
        ++slot)
    {
        if (Item* item = player->GetItemByPos(
            INVENTORY_SLOT_BAG_0,
            slot))
        {
            visitor(item, INVENTORY_SLOT_BAG_0, slot);
        }
    }

    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START;
        bagSlot != INVENTORY_SLOT_BAG_END;
        ++bagSlot)
    {
        if (Bag* bag = player->GetBagByPos(bagSlot))
        {
            for (uint32 slot = 0; slot != bag->GetBagSize(); ++slot)
            {
                if (Item* item = player->GetItemByPos(bagSlot, slot))
                    visitor(item, bagSlot, uint8(slot));
            }
        }
    }
}
```

注意：实现时应遵循当前代码的 const 语义和项目格式，不应照抄伪代码中的类型细节而忽略实际签名。

### 11.4 候选数量

WotLK 常规随身背包总容量有限，第一版可以返回全部候选。建议仍设置协议保护上限，例如 128 件：

- 正常玩家随身背包不会超过该数量；
- 防止未来扩展背包或异常数据造成过大消息；
- 达到上限时返回 `truncated = true`，客户端显示“候选过多，仅显示前 128 件”。

候选在服务器排序后再截断，优先保留 ItemLevel 和 GearScore 较高的物品。

---

## 12. 服务器原子换装流程

`EquipFromInventory()` 必须在一次 C++ 调用中完成：

```text
收到请求
  -> 校验频率和参数
  -> 在当前玩家可管理 Bot 集合中重新找 Bot
  -> 校验所有者/共享装备权限
  -> 检查玩家和 Bot 状态
  -> 校验 slot
  -> 计算当前 equipmentRevision
  -> 与 expectedRevision 比较
  -> 遍历允许的背包范围，按 Item GUID 查找 Item
  -> 校验 Item Entry 与 expectedItemEntry
  -> 校验 Item owner、位置、交易状态
  -> 再次调用 _canEquip(..., slot, true, item)
  -> 调用 _equip(slot, item, playerGuid, storeToBank)
  -> 映射 BotEquipResult
  -> 构造最新完整 18 槽快照
  -> 返回
```

关键要求：

- 不先通过 Lua 取得 `Item*` 再跨调用保存。
- 不仅根据旧 `bag + bagSlot` 查找，因为物品可能已移动。
- 不依赖旧候选列表白名单作为唯一条件。
- 世界线程内一次调用完成检查和执行，减少 TOCTOU 竞态窗口。
- `_equip()` 可能联动主副手或把旧装备退回玩家，因此成功后必须刷新全部槽位。

### 12.1 装备银行选项

第一版：

```text
storeReplacedToBank = false
```

保持与现有普通 Gossip 换装一致。

未来开放装备银行时：

- 只有 `BotCfg::IsGearBankEnabled()` 为 true 才允许；
- 预先检查装备银行容量；
- 映射 `BOT_EQUIP_RESULT_FAIL_NO_BANK_SPACE`；
- 客户端应明确显示“旧装备存入装备银行”，不能默默改变存储位置。

---

## 13. BotEquipResult 错误映射

现有 `BotEquipResult`：

| 核心结果 | UI code | 中文信息 |
|---|---|---|
| `BOT_EQUIP_RESULT_OK` | `OK` | 装备成功 |
| `FAIL_NO_BAG_SPACE` | `NO_BAG_SPACE` | 背包空间不足 |
| `FAIL_NO_BANK_SPACE` | `NO_BANK_SPACE` | 装备银行空间不足 |
| `FAIL_NO_RECEIVER` | `NO_RECEIVER` | 未找到装备接收者 |
| `FAIL_INVALID_RECEIVER` | `INVALID_RECEIVER` | 装备接收者无效 |
| `FAIL_NO_ITEM` | `ITEM_NOT_FOUND` | 物品已不存在或已移动 |
| `FAIL_SAME_ID` | `SAME_ITEM` | 该槽位已经使用相同物品 |
| `FAIL_WANDERER` | `INVALID_BOT_STATE` | 游荡 NPCBot 不能更换装备 |
| `FAIL_LINKED_UNEQUIP_FAILED` | `LINKED_UNEQUIP_FAILED` | 关联槽位卸装失败 |
| `FAIL_LINKED_RESET_FAILED` | `LINKED_RESET_FAILED` | 关联槽位重置失败 |
| `FAIL_CANT_EQUIP` | `CANT_EQUIP` | 该 NPCBot 不能装备此物品 |
| `FAIL_ITEM_CONFLICT` | `ITEM_CONFLICT` | 装备与当前主副手配置冲突 |

服务层自身还需补充：

| UI code | 触发场景 |
|---|---|
| `INVALID_REQUEST` | 参数缺失、类型或范围错误 |
| `RATE_LIMITED` | 请求过快 |
| `BOT_NOT_FOUND` | Bot 已离队、消失或身份不匹配 |
| `NO_PERMISSION` | 不是所有者或共享装备权限关闭 |
| `INVALID_SLOT` | 槽位不在 0–17 |
| `BUSY_IN_COMBAT` | 玩家或 Bot 正在战斗 |
| `ITEM_MOVED` | Item 不在允许的随身背包范围 |
| `ITEM_MISMATCH` | GUID 对应 Entry 与请求不一致 |
| `STALE_EQUIPMENT` | 装备状态指纹已改变 |
| `INTERNAL_ERROR` | 未分类异常，服务端记录详细日志 |

客户端不显示 C++ 枚举名，只显示稳定的中文提示。

---

## 14. ALE C++ 桥接设计

### 14.1 推荐 Player 方法（含固定“卸下”操作）

文件：`modules/mod-ale/src/LuaEngine/methods/PlayerMethods.h`

新增：

```lua
result = player:GetNPCBotEquipmentSnapshot(
    botEntry,
    botGuidLow)

result = player:GetNPCBotEquipCandidates(
    botEntry,
    botGuidLow,
    botSlot)

result = player:EquipNPCBotItemFromInventory(
    botEntry,
    botGuidLow,
    botSlot,
    itemGuidLow,
    expectedItemEntry,
    expectedEquipmentRevision,
    storeReplacedToBank)

result = player:UnequipNPCBotItem(
    botEntry,
    botGuidLow,
    botSlot,
    expectedEquipmentRevision,
    storeToBank)
```

这些方法应返回一个结构化 Lua table，而不是多个位置含义不清的返回值：

```lua
{
    ok = true,
    code = "OK",
    revision = "a102ff09",
    candidates = {},
    snapshot = {}
}
```

### 14.2 注册方法

文件：`modules/mod-ale/src/LuaEngine/LuaFunctions.cpp`

在 `PlayerMethods[]` 中注册：

```cpp
{ "GetNPCBotEquipmentSnapshot",
    &LuaPlayer::GetNPCBotEquipmentSnapshot },
{ "GetNPCBotEquipCandidates",
    &LuaPlayer::GetNPCBotEquipCandidates },
{ "EquipNPCBotItemFromInventory",
    &LuaPlayer::EquipNPCBotItemFromInventory },
{ "UnequipNPCBotItem",
    &LuaPlayer::UnequipNPCBotItem },
```

### 14.3 桥接层规则

- 只接受标量参数，不接受客户端构造的 Item userdata 或 Creature userdata。
- 将 GUID 字符串严格解析为 `ObjectGuid::LowType`，拒绝负数、溢出和非数字字符。
- 所有业务调用交给 `bot_mgr_service`。
- Lua table 构造只复制快照值，不向 Lua 暴露长生命周期 `Item*`、`Creature*` 或 `Player*`。
- 按项目约定，长期状态只保存 `ObjectGuid` 或值对象，不保存裸指针。

---

## 15. 服务端 AIO Handler

建议目录：

```text
lua_scripts/NPCBotEquipment/NPCBotEquipmentServer.lua
lua_scripts/NPCBotEquipment/NPCBotEquipmentClient.lua
```

实际路径以 `ALE.ScriptPath` 配置为准，默认是 `lua_scripts`。

服务端示意：

```lua
local AIO = AIO or require("AIO")
local handlers = AIO.AddHandlers("NPCBotEquipment", {})

function handlers.RequestCandidates(player, request)
    local result = player:GetNPCBotEquipCandidates(
        request.botEntry,
        request.botGuidLow,
        request.botSlot)

    result.requestId = request.requestId
    result.botEntry = request.botEntry
    result.botGuidLow = request.botGuidLow
    result.botSlot = request.botSlot

    AIO.Handle(
        player,
        "NPCBotEquipment",
        "CandidatesResult",
        result)
end

function handlers.EquipCandidate(player, request)
    local result = player:EquipNPCBotItemFromInventory(
        request.botEntry,
        request.botGuidLow,
        request.botSlot,
        request.itemGuid,
        request.expectedItemEntry,
        request.expectedEquipmentRevision,
        false)

    result.requestId = request.requestId

    AIO.Handle(
        player,
        "NPCBotEquipment",
        "EquipResult",
        result)
end

function handlers.Unequip(player, request)
    local result = player:UnequipNPCBotItem(
        request.botEntry,
        request.botGuidLow,
        request.botSlot,
        request.expectedEquipmentRevision,
        false)

    result.requestId = request.requestId

    AIO.Handle(
        player,
        "NPCBotEquipment",
        "UnequipResult",
        result)
end
```

服务端 Lua 仍要进行基础参数存在性检查，但不能把它当成安全边界；C++ 服务必须重复进行严格类型、范围、权限和状态校验。

---

## 16. 客户端 Lua 设计

### 16.1 装备槽绑定

```lua
slotButton:RegisterForClicks("LeftButtonUp", "RightButtonUp")

slotButton:SetScript("OnClick", function(self, mouseButton)
    if mouseButton == "RightButton" then
        NPCBotEquipmentUI:ToggleCandidates(self.botSlot, self)
        return
    end

    NPCBotEquipmentUI:HandlePrimarySlotClick(self)
end)
```

### 16.2 请求候选

```lua
function NPCBotEquipmentUI:ToggleCandidates(botSlot, slotButton)
    if self.candidatePanel:IsShown() and
        self.candidatePanel.botSlot == botSlot then
        self:CloseCandidatePanel()
        return
    end

    self.requestSerial = self.requestSerial + 1

    local requestId = self.requestSerial
    local bot = self.currentBot

    self:ShowCandidateLoading(slotButton, botSlot, requestId)

    AIO.Handle(
        "NPCBotEquipment",
        "RequestCandidates",
        {
            requestId = requestId,
            botEntry = bot.entry,
            botGuidLow = bot.guidLow,
            botSlot = botSlot,
            equipmentRevision = bot.equipmentRevision
        })
end
```

### 16.3 丢弃旧响应

```lua
function handlers.CandidatesResult(player, response)
    local panel = NPCBotEquipmentUI.candidatePanel

    if not panel:IsShown() then
        return
    end

    if response.requestId ~= panel.requestId or
        response.botSlot ~= panel.botSlot or
        response.botGuidLow ~= panel.botGuidLow then
        return
    end

    NPCBotEquipmentUI:ApplyCandidateResponse(response)
end
```

AIO 客户端 handler 的首个参数形式应以项目当前 AIO 版本为准；上例重点是响应关联规则。

### 16.4 渲染 4 列

```lua
function NPCBotEquipmentUI:RenderCandidates(candidates)
    self:ReleaseCandidateButtons()

    local displayItems = {
        {
            kind = "UNEQUIP",
            text = "卸下",
            enabled = self:IsCurrentSlotEquipped()
        }
    }

    for _, itemData in ipairs(candidates) do
        displayItems[#displayItems + 1] = itemData
    end

    for index, itemData in ipairs(displayItems) do
        local button = self:AcquireCandidateButton(index)
        local column = (index - 1) % 4
        local row = math.floor((index - 1) / 4)

        button:ClearAllPoints()
        button:SetPoint(
            "TOPLEFT",
            self.candidateContent,
            "TOPLEFT",
            8 + column * 42,
            -8 - row * 42)

        button.itemData = itemData
        button:SetEnabled(itemData.kind ~= "UNEQUIP" or itemData.enabled)
        self:RenderCandidateButton(button, itemData)
        button:Show()
    end

    self:UpdateCandidateScroll(#displayItems)
end
```

必须使用按钮池复用 Frame，不能每次右键都无限创建新按钮。

### 16.5 候选按钮 Tooltip

`UNEQUIP` 操作项不调用 `GameTooltip:SetHyperlink()`，只显示“卸下当前装备”的普通提示；真实物品继续使用原生 ItemLink Tooltip。

### 16.6 Tooltip

```lua
button:SetScript("OnEnter", function(self)
    if not self.itemData or not self.itemData.link then
        return
    end

    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:SetHyperlink(self.itemData.link)

    if self.itemData.maxDurability and
        self.itemData.maxDurability > 0 then
        GameTooltip:AddLine(
            string.format(
                "耐久度 %d / %d",
                self.itemData.durability or 0,
                self.itemData.maxDurability),
            1,
            1,
            1)
    end

    GameTooltip:Show()
end)

button:SetScript("OnLeave", function()
    GameTooltip:Hide()
end)
```

注意：如果客户端原生 Tooltip 已经显示耐久度，不应重复添加。实现时可检测实际 3.3.5a 客户端效果后决定是否保留补充行。

支持 Shift 装备比较：

- 继续使用原生 `GameTooltip:SetHyperlink()`；
- 在 Shift 状态变化时重新显示 Tooltip；
- 可调用客户端现有比较 Tooltip API；
- 不要自己拼装备属性文本。

### 16.7 点击装备或卸下

```lua
button:SetScript("OnClick", function(self, mouseButton)
    if mouseButton ~= "LeftButton" or
        not self.itemData or
        not self:IsEnabled() or
        NPCBotEquipmentUI.equipPending then
        return
    end

    NPCBotEquipmentUI.equipPending = true
    NPCBotEquipmentUI:SetCandidateButtonsEnabled(false)
    NPCBotEquipmentUI.requestSerial =
        NPCBotEquipmentUI.requestSerial + 1

    local requestId = NPCBotEquipmentUI.requestSerial
    local bot = NPCBotEquipmentUI.currentBot
    local panel = NPCBotEquipmentUI.candidatePanel
    local itemData = self.itemData

    if itemData.kind == "UNEQUIP" then
        AIO.Handle(
            "NPCBotEquipment",
            "Unequip",
            {
                requestId = requestId,
                botEntry = bot.entry,
                botGuidLow = bot.guidLow,
                botSlot = panel.botSlot,
                expectedEquipmentRevision =
                    panel.equipmentRevision,
                storeToBank = false
            })
        return
    end

    AIO.Handle(
        "NPCBotEquipment",
        "EquipCandidate",
        {
            requestId = requestId,
            botEntry = bot.entry,
            botGuidLow = bot.guidLow,
            botSlot = panel.botSlot,
            itemGuid = itemData.itemGuid,
            expectedItemEntry = itemData.entry,
            expectedEquipmentRevision =
                panel.equipmentRevision,
            storeReplacedToBank = false
        })
end)
```

### 16.8 处理装备和卸下结果

```lua
function handlers.UnequipResult(player, response)
    NPCBotEquipmentUI:HandleEquipmentMutationResult(response)
end

function handlers.EquipResult(player, response)
    NPCBotEquipmentUI:HandleEquipmentMutationResult(response)
end

function NPCBotEquipmentUI:HandleEquipmentMutationResult(response)
    NPCBotEquipmentUI.equipPending = false

    if response.ok then
        NPCBotEquipmentUI:CloseCandidatePanel()
        NPCBotEquipmentUI:ApplyFullSnapshot(response.snapshot)
        return
    end

    NPCBotEquipmentUI:ShowError(response.message)

    if response.refreshRequired or
        response.code == "STALE_EQUIPMENT" or
        response.code == "ITEM_MOVED" then
        NPCBotEquipmentUI:ReloadCurrentSlotCandidates()
        return
    end

    NPCBotEquipmentUI:SetCandidateButtonsEnabled(true)
end
```

---

## 17. 原生 Tooltip 与聊天链接

当前装备槽和候选物品统一保存完整 ItemLink：

```lua
button.itemData.link
```

### 17.1 基础 Tooltip

```lua
GameTooltip:SetHyperlink(itemLink)
```

可显示：

- 品质与物品名称；
- 装备位置和护甲/武器类型；
- 属性；
- 附魔；
- 宝石；
- 随机属性；
- 套装效果；
- 使用和装备效果；
- 职业与等级需求。

### 17.2 聊天链接

Shift 点击候选物品时，可调用原生聊天链接插入逻辑。该操作仅使用 ItemLink，不发送换装请求。

建议交互：

- 普通左键：装备；
- Shift + 左键：插入聊天链接，不装备；
- 鼠标悬停：Tooltip；
- 右键：保留，不执行操作。

### 17.3 客户端缓存

若 `GetItemInfo()` 暂时取不到图标或名称：

- 优先使用服务器下发的 `icon` 和 `link`；
- Tooltip 仍调用 `SetHyperlink()` 触发客户端查询；
- 可监听 `GET_ITEM_INFO_RECEIVED` 后重绘缺失图标；
- 不因单个物品缓存未命中而丢弃候选。

---

## 18. 安全、竞态与限流

### 18.1 必须防护的竞态

候选列表返回后、玩家点击前，物品可能：

- 从一个背包格移动到另一个格；
- 被出售、摧毁或邮寄；
- 放入银行；
- 放入交易栏；
- 已装备给另一个 Bot；
- 所有权发生变化；
- Bot 已离队或更换所有者；
- Bot 当前主副手状态发生变化；
- 共享装备权限被关闭。

解决方式不是锁住客户端列表，而是点击时完整重新验证。

### 18.2 限流建议

按玩家 GUID 保存短期令牌桶：

| 请求 | 建议限制 |
|---|---|
| `RequestCandidates` | 最快每 250 ms 一次，突发最多 4 次 |
| `EquipCandidate` | 最快每 350 ms 一次，突发最多 2 次 |
| `Unequip` | 最快每 350 ms 一次，突发最多 2 次 |
| 装备快照请求 | 最快每 250 ms 一次 |

限流状态只保存在内存中，玩家登出时清理。日志不要记录完整 ItemLink，以免刷日志；记录玩家 GUID、Bot Entry、槽位、Item GUID 和错误码即可。

### 18.3 输入验证

- `requestId`：非负且在协议允许范围内；
- `botEntry`：非零 uint32；
- `botGuidLow`：严格十进制字符串；
- `botSlot`：0–17；
- `itemGuid`：严格十进制字符串；
- `expectedItemEntry`：非零 uint32；
- `expectedEquipmentRevision`：长度受限，只允许约定字符；
- `storeReplacedToBank`：严格 boolean，第一版服务端可忽略客户端值并固定 false。

### 18.4 客户端超时

建议 5 秒超时：

- 超时只解除客户端禁用状态；
- 不能假定服务器操作失败；
- 若装备或卸下请求超时，客户端立即请求完整快照确认最终状态；
- 不自动重复发送同一个装备请求，避免网络延迟下重复操作。

---

## 19. 与 NPCBot 头像“查看”菜单的衔接

NPCBot 已作为 Creature 成员加入服务器 Group，用户实测在队伍框架右键 Bot 时可见“移出队伍”且能正常移除。因此“查看”菜单应优先接入实际队伍成员菜单上下文：

```lua
UnitPopupMenus["PARTY"]
UnitPopupMenus["RAID_PLAYER"]
UnitPopupMenus["RAID"]
```

具体 `which` 应通过客户端诊断确认：

```lua
hooksecurefunc(
    "UnitPopup_ShowMenu",
    function(dropdownMenu, which, unit, name, userData)
        print(which, unit, UnitGUID(unit))
    end)
```

点击“查看”后：

1. 客户端发送被点击 unit 的标识。
2. 服务器确认该对象是当前玩家可查看的 NPCBot。
3. 服务器返回 `botKey` 和完整装备快照。
4. 客户端打开装备框。
5. 右键装备槽再进入本文定义的候选流程。

普通玩家头像不能显示 NPCBot 专用“查看”。是否为 NPCBot 必须由服务器标记或已验证缓存决定，不能只根据名字或 UnitGUID 外观猜测。

---

## 20. 实现文件清单

### 20.1 NPCBot 核心

| 文件 | 修改 |
|---|---|
| `src/server/game/AI/NpcBots/bot_ai.h` | 添加 `friend class bot_mgr_service;` 或受控窄包装器 |
| `src/server/game/AI/NpcBots/bot_mgr_service.h` | 规划 Bot 服务结果、候选、快照结构和统一服务接口；本轮不创建 |
| `src/server/game/AI/NpcBots/bot_mgr_service.cpp` | 规划 Bot 查找、权限、背包遍历、候选筛选、卸下、原子换装和状态指纹；本轮不创建 |
| NPCBot 对应 CMake 文件 | 若使用显式源文件列表则注册新文件 |

### 20.2 mod-ale

| 文件 | 修改 |
|---|---|
| `modules/mod-ale/src/LuaEngine/methods/PlayerMethods.h` | 新增 4 个 NPCBot 装备 Player 方法，包括固定“卸下”操作 |
| `modules/mod-ale/src/LuaEngine/LuaFunctions.cpp` | 注册新增 Player 方法 |
| mod-ale include/CMake 配置 | 视实际依赖补 NPCBot 服务头文件引用 |

### 20.3 AIO Lua

| 文件 | 修改 |
|---|---|
| `lua_scripts/NPCBotEquipment/NPCBotEquipmentServer.lua` | 服务端 handler、参数规整、调用 ALE 方法、响应 |
| `lua_scripts/NPCBotEquipment/NPCBotEquipmentClient.lua` | 装备框、右键槽位、候选面板、Tooltip、装备请求 |
| AIO 启动脚本 | 加载 NPCBotEquipment 服务端和客户端脚本 |

### 20.4 SQL

本功能不需要新增 SQL 表。装备状态继续使用 NPCBot 现有持久化逻辑。除非后续增加玩家 UI 偏好持久化，否则不应为候选面板单独创建数据库表。

---

## 21. 分阶段实施计划

### 阶段 1：服务设计与只读快照（暂不创建 C++ 文件）

1. 设计 `bot_mgr_service`，暂不创建 C++ 文件或接入构建。
2. 明确安全 Bot 查找和权限判断接口。
3. 明确完整 18 槽快照结构。
4. 明确状态指纹计算和比较。
5. 明确主背包和装备包遍历器。
6. 明确候选筛选并逐 Item GUID 返回。
7. 本阶段暂不创建 C++ 文件，不接入构建，也不实现换装。

验收：服务器 Lua 可以请求某 Bot 某槽位候选，并打印正确数量与 Item GUID。

### 阶段 2：ALE 桥接

1. 新增 PlayerMethods。
2. 注册方法。
3. 严格解析 GUID 字符串。
4. 将 C++ 结果转换为 Lua table。
5. 验证 Lua 层拿不到裸 `Item*` 或任意 Bot 操作入口。

验收：服务端 AIO handler 可获取候选和快照。

### 阶段 3：客户端候选面板

1. 装备槽响应右键。
2. 显示加载状态。
3. 实现 4 列布局。
4. 超过 5 行启用滚动。
5. 实现边缘翻转。
6. 实现按钮池。
7. 实现原生 Tooltip。
8. 实现旧响应丢弃和关闭规则。

验收：不同槽位显示不同候选；同 Entry 不同实例分别显示；超过 4 件正确换行。

### 阶段 4：服务层原子换装（后续实现）

1. 在 `bot_mgr_service` 中实现 `EquipFromInventory()` 和 `Unequip()`。
2. 加入 Item GUID、Entry、owner、位置和交易状态复核（卸下不需要 Item GUID，但仍校验槽位状态）。
3. 加入装备状态指纹校验。
4. 复用 `_canEquip()`、`_equip()` 和 `_unequip()`。
5. 映射全部错误码。
6. 装备或卸下成功后返回完整快照。

验收：装备、卸下和主副手联动均正确，旧物品去向与现有 Gossip 一致。

### 阶段 5：安全和体验收尾

1. 加入限流。
2. 加入超时后的快照确认。
3. 接入头像右键“查看”。
4. 增加 Shift 聊天链接和装备比较。
5. 实机检查不同 UI 缩放和分辨率。
6. 运行 C++ 与 Lua 相关格式检查。

---

## 22. 测试矩阵

### 22.1 固定“卸下”操作

| 场景 | 预期 |
|---|---|
| 有装备槽右键 | 第一格显示可点击“卸下” |
| 空装备槽右键 | 第一格仍显示“卸下”，但置灰不可点击 |
| 点击“卸下” | 发送 `Unequip`，不携带背包 Item GUID |
| 卸下成功 | 关闭候选面板，完整 18 槽快照显示为空槽 |
| 卸下时装备状态已变化 | 返回 `STALE_EQUIPMENT`，不执行旧卸装 |
| “卸下”悬停 | 显示普通提示，不调用 ItemLink Tooltip |

### 22.2 槽位与职业

| 场景 | 预期 |
|---|---|
| 战士主手请求 | 显示符合现有 `_canEquip()` 的武器 |
| 法师板甲头部 | 不显示 |
| 猎人远程槽 | 显示可用弓、弩或枪，遵循现有规则 |
| 低等级 Bot 请求高等级装备 | 不显示 |
| 萨满低等级副手武器 | 按现有等级和专精规则过滤 |
| 戒指 1/戒指 2 | 两个槽分别请求，均按目标槽判断 |
| 饰品 1/饰品 2 | 两个槽分别请求，均按目标槽判断 |
| 双手武器与副手冲突 | 候选和点击时均正确处理 |

### 22.3 物品实例

| 场景 | 预期 |
|---|---|
| 两件相同 Entry、不同附魔 | 显示两个候选，Tooltip 各自正确 |
| 两件相同 Entry、不同宝石 | 显示两个候选 |
| 两件相同 Entry、不同随机属性 | 显示两个候选 |
| 候选返回后移动格子 | 仍可按 GUID 找到并装备 |
| 候选返回后放入银行 | 点击失败并刷新候选 |
| 候选返回后出售或摧毁 | 点击返回 `ITEM_NOT_FOUND` |
| 候选返回后放入交易栏 | 点击被拒绝 |
| 伪造 Entry 但 GUID 正确 | 返回 `ITEM_MISMATCH` |
| 伪造其他角色 Item GUID | 返回 `ITEM_NOT_FOUND` 或 `ITEM_MOVED`，不得装备 |

### 22.4 UI 布局

| 真实服务器候选数 | 预期布局（含固定“卸下”） |
|---:|---|
| 0 | 仅显示第一行第一列“卸下”（空槽时置灰） |
| 1 | “卸下”第一列，真实物品第一行第二列 |
| 3 | 一行四列，最后一格为第 3 件真实物品 |
| 4 | 第二行第一列开始显示第 4 件真实物品 |
| 7 | 两行四列 |
| 19 | 五行，刚好填满 20 个显示格 |
| 20+ | 五行可见区域，启用滚动 |
| 槽位靠屏幕右边 | 面板翻转到槽位左边 |
| 低分辨率/UI 缩放增大 | 面板不超出屏幕底部 |

### 22.5 竞态与消息

| 场景 | 预期 |
|---|---|
| 快速右键槽位 A 后右键槽位 B | A 的迟到响应不能覆盖 B |
| 连续双击候选 | 只发送一次有效装备请求 |
| 装备或卸下请求超时但服务器成功 | 后续完整快照确认真实状态 |
| Gossip 同时改变 Bot 装备 | 旧 `equipmentRevision` 被拒绝 |
| Bot 离队后点击旧候选 | `BOT_NOT_FOUND`，关闭面板 |
| 共享权限关闭后点击 | `NO_PERMISSION` |
| 请求刷屏 | `RATE_LIMITED`，服务器稳定 |

### 22.6 Tooltip

| 场景 | 预期 |
|---|---|
| 普通装备 | 显示原生属性 Tooltip |
| 附魔装备 | 显示附魔 |
| 镶嵌装备 | 显示宝石 |
| 随机属性装备 | 显示正确后缀或属性 |
| 套装物品 | 显示套装信息 |
| Shift 比较 | 可与玩家当前装备比较 |
| Shift 点击 | 插入聊天链接，不发装备请求 |

---

## 23. 验收标准

功能完成必须同时满足：

1. NPCBot 18 个装备槽均可右键请求候选。
2. 候选面板第一项固定为“卸下”；槽位有装备时可执行，空槽位时置灰。
3. 候选由服务器扫描玩家主背包和全部已装备背包产生。
4. 每个真实候选都通过现有 `_canEquip()`，Lua 不复制装备规则。
5. 相同 Entry 的不同 Item 实例按 GUID 分别展示。
6. 包含固定“卸下”后仍固定每行 4 件，第 4 个真实候选进入第二行第一列。
7. 超过最大可见行数时可滚动，面板不超出屏幕。
8. 默认显示在槽位右侧，右侧越界时翻转到左侧。
9. 当前装备槽和真实候选物品使用原生 `GameTooltip`；“卸下”使用普通提示。
10. 点击真实候选时按 Item GUID 重新查找，不使用 Entry 直接装备；点击“卸下”时按 Bot 槽位执行卸装。
11. 服务器重新验证 Bot 权限、物品所有权、背包位置、交易状态、装备状态和 `_canEquip()`；卸装也校验状态指纹。
12. 最终调用现有 `_equip()` 或 `_unequip()`，不另写装备状态修改逻辑。
13. 装备或卸下成功后返回完整 18 槽快照并覆盖客户端状态。
14. 主副手等关联槽发生变化时客户端显示正确。
15. 迟到响应不会覆盖当前面板。
16. 请求有频率限制，重复点击不会重复装备或卸下。
17. Bot 离队、物品移动、权限变化和状态过期均有明确错误反馈。
18. 不新增不必要的数据库表，不绕过 NPCBot 现有持久化。

---

## 24. 最终推荐决策

### 24.1 第一版应做

- 右键槽位请求服务器候选；
- 固定 4 列，最多 5 行可见，超出滚动；
- 每个 Item GUID 单独展示，不按 Entry 去重；
- `ignoreItemLevel = true`，允许玩家手动选择较低评分装备；
- 原生 ItemLink Tooltip；
- C++ `bot_mgr_service` 统一权限和原子事务，并作为未来 Bot 服务扩展入口；
- `equipmentRevision` 只使用即时状态指纹，不新增版本字段；
- 候选列表第一项固定为“卸下”，服务端真实候选从第二个显示格开始。
- 使用装备状态指纹处理 Gossip/AIO 多入口竞态；
- 普通换装固定 `storeReplacedToBank = false`；
- 装备或卸下成功后刷新完整装备快照。

### 24.2 第一版不应做

- 不在 Lua 复制 `_canEquip()`；
- 不仅按 Item Entry 装备；
- 不信任客户端 bag/slot；
- 不把 `_canEquip()`、`_equip()` 全部公开给任意模块；
- 不缓存长期 `Item*`、`Creature*`；
- 不为候选面板新增数据库表；
- 不因实现“点击外部关闭”而使用会破坏原生 UI 的高层透明遮罩；
- 不自动重发超时的装备请求。

### 24.3 后续增强

在第一版稳定后再增加：

- 候选物品与当前 Bot 装备的属性差值；
- 装备银行来源和旧装备存入装备银行选项；
- 搜索、品质过滤和物品等级过滤；
- 自动选择戒指/饰品较合适的双槽；
- 候选排序方式切换；
- 装备预览或换装确认；
- 拖放与右键候选双入口共用同一服务层。

---

## 25. 结论

该方案的关键不是在客户端模拟一套装备判定，而是把现有 NPCBot Gossip 装备路径改造成可复用、可审计的 C++ 服务入口：服务器用 `_canEquip()` 筛选，用 Item GUID 标识实例，用 `_equip()` 完成最终变更；AIO 只传输快照和操作意图；客户端负责 4 列悬浮面板与原生 Tooltip。

这样可以在保持现有 NPCBot 装备行为兼容的前提下，实现更接近玩家角色面板的换装体验，同时避免 Item Entry 歧义、过期候选、共享权限绕过和主副手状态不同步等问题。
