# mod-ah-bot 模块功能与配置说明报告

> 分析对象：`modules/mod-ah-bot/`（AzerothCore 官方拍卖行机器人模块）
> 分析日期：2026-08-21
> 模块来源：github.com/azerothcore/mod-ah-bot（由 TrinityCore/MaNGOS 的 AuctionHouseBot 移植而来，作者 Ayase）

---

## 一、模块概览

mod-ah-bot 是一个**拍卖行机器人（Auction House Bot）**模块，用于在 AzerothCore 3.3.5a 私服的三大拍卖行（联盟、部落、中立）中自动挂卖物品和竞买物品，从而让冷清的服务器市场保持"有人气"。

核心能力由两个**彼此独立**的开关控制：

| 功能 | 配置开关 | 作用 |
|------|----------|------|
| 卖家（Seller） | `AuctionHouseBot.EnableSeller` | 按比例持续向拍卖行上架新物品，维持目标挂单量 |
| 买家（Buyer） | `AuctionHouseBot.EnableBuyer` | 定期对玩家挂出的物品进行竞价/一口价收购 |

- 卖家负责**制造供应**：从 `item_template` 中筛选出符合条件的物品，随机定价后挂到拍卖行。
- 买家负责**制造需求**：以"供应商价（vendor price）乘倍数"为心理价位，收购玩家挂出的低价物品。

二者可只开其一，也可同时开启，互不依赖。

### 版本与前置要求

- 需要 AzerothCore commit 至少 `9adba48` 之后的版本。
- 旧版本模块升级需执行一次表重命名：

```sql
ALTER TABLE `auctionhousebot` RENAME TO `mod_auctionhousebot`;
```

---

## 二、架构与运行机制

### 2.1 文件结构

```
modules/mod-ah-bot/
├── conf/
│   └── mod_ahbot.conf.dist          # 模块配置模板（复制为 mod_ahbot.conf 生效）
├── data/sql/db-world/
│   ├── mod_auctionhousebot.sql      # 主配置表 + 黑名单表 + 默认数据
│   ├── auctionhousebot_professionItems.sql  # 职业物品清单（供 ProfessionItems 过滤用）
│   └── z_filter_disabled_and_trash.sql      # 额外黑名单（按名称过滤 PTR/废品物品）
└── src/
    ├── ah_bot_loader.cpp            # 模块入口 Addmod_ah_botScripts()
    ├── AuctionHouseBot.cpp/.h       # 核心：Sell / Buy / Update / Commands
    ├── AuctionHouseBotConfig.cpp/.h # 配置加载、百分比换算、物品过滤分箱、市场价统计
    ├── AuctionHouseBotWorldScript.cpp/.h     # 世界钩子：启动时初始化、重载
    ├── AuctionHouseBotAuctionHouseScript.cpp/.h # 拍卖行钩子：成交/过期/上下架时统计
    ├── AuctionHouseBotMailScript.cpp/.h       # 邮件钩子：拦截发往 bot 的邮件
    ├── AuctionHouseBotCommon.h      # 常量、枚举（品质/职业/命令）
    └── cs_ah_bot.cpp                # GM 命令脚本（ahbotoptions）
```

### 2.2 脚本钩子（Hook）与职责

模块通过 3 类 `ScriptObject` 挂入核心：

| 脚本类 | 挂载的钩子 | 作用 |
|--------|-----------|------|
| `AHBot_WorldScript` | `OnBeforeConfigLoad` / `OnStartup` | 启动时读取 Account/GUID，初始化三个 AH 配置，创建 bot 实例；重载配置时重建 bot |
| `AHBot_AuctionHouseScript` | 成交/过期/被超价/添加/移除/更新等 8 个钩子 | 实时统计各 AH 各品质物品数量；成交与过期时更新"市场价"统计；拦截 bot 自身的成交/过期通知 |
| `AHBot_MailScript` | `OnBeforeMailDraftSendMailTo` | 拦截发往 bot 角色的拍卖邮件，直接删除物品不回发，避免堆积 |

### 2.3 运行流程

```
世界启动 (OnStartup)
   └─ 读 AuctionHouseBot.Account / .GUID → 确定 gBotsId（bot 角色集合）
   └─ 三个 AHBConfig(联盟=2 / 部落=6 / 中立=7) 各自 Initialize：
        ├─ InitializeFromFile()  从 mod_ahbot.conf 读开关/过滤项
        ├─ InitializeFromSql()   从 mod_auctionhousebot 表读配额/价格/买家参数
        └─ InitializeBins()      遍历 item_template，逐项过滤后装入 14 个品质分箱
   └─ PopulateBots() 为每个 bot 角色建 AuctionHouseBot 实例

拍卖行每次 Update 钩子 (OnBeforeAuctionHouseMgrUpdate)
   └─ 对每个 bot 执行 Update()：
        ├─ Sell()  每个阵营：按配额补货（ItemsPerCycle 限流）
        └─ Buy()   按 buyerbiddinginterval 分钟间隔、每次 buyerbidsperinterval 笔竞拍
```

### 2.4 物品"分箱"机制

`InitializeBins()` 会把通过全部过滤条件的物品，按 **品质 × 是否贸易商品** 分成 14 个集合（bin）：

| 品质 | 贸易商品（Trade Goods） | 普通物品（Items） |
|------|:---:|:---:|
| 灰色（Poor） | GreyTradeGoodsBin | GreyItemsBin |
| 白色（Normal） | WhiteTradeGoodsBin | WhiteItemsBin |
| 绿色（Uncommon） | GreenTradeGoodsBin | GreenItemsBin |
| 蓝色（Rare） | BlueTradeGoodsBin | BlueItemsBin |
| 紫色（Epic） | PurpleTradeGoodsBin | PurpleItemsBin |
| 橙色（Legendary） | OrangeTradeGoodsBin | OrangeItemsBin |
| 黄色（Artifact） | YellowTradeGoodsBin | YellowItemsBin |

卖家每次补货时，会按"灰→白→绿→蓝→紫→橙→黄"的稀有度顺序，从对应 bin 中随机抽取物品上架，直到本周期配额用完或触发循环上限（`AUCTION_HOUSE_BOT_LOOP_BREAKER = 32`）。

---

## 三、安装与启用

1. 把模块目录放到 `modules/` 下（本项目已放置）。
2. 将 SQL 手动导入 **acore_world** 数据库：
   - `data/sql/db-world/mod_auctionhousebot.sql`（必需，建表 + 默认数据）
   - `data/sql/db-world/auctionhousebot_professionItems.sql`（启用 `ProfessionItems` 过滤时才需要）
   - `data/sql/db-world/z_filter_disabled_and_trash.sql`（可选，追加按名称过滤的废品黑名单）
3. 重新运行 cmake 并重新编译。
4. 在 `conf/` 下把 `mod_ahbot.conf.dist` 复制为 `mod_ahbot.conf`，配置 Account / GUID。
5. **必须指定一个真实存在的账号与角色**（见下文"常见问题"）。

关键说明（来自 README 与源码）：

- 使用的账号**不需要任何安全等级**，普通玩家账号即可。
- 若只填 `Account`（GUID=0），该账号下**所有角色**都会参与买卖；若同时填 `GUID`，则只用该角色。
- bot 角色**不建议玩家本人登录使用**——用它浏览拍卖行可能出现"正在搜索物品……"卡死的现象。

---

## 四、配置文件说明（`conf/mod_ahbot.conf`）

配置文件分「全局设置」与「过滤器 Part 1~4」四部分。以下逐项列出，默认值取自 `mod_ahbot.conf.dist`。

### 4.1 全局设置（AUCTION HOUSE BOT SETTINGS）

| 配置项 | 默认 | 说明 |
|--------|:---:|------|
| `AuctionHouseBot.DEBUG` | 0 | 总调试输出开关 |
| `AuctionHouseBot.DEBUG_CONFIG` | 0 | 配置加载调试输出 |
| `AuctionHouseBot.DEBUG_FILTERS` | 0 | 过滤器调试输出 |
| `AuctionHouseBot.DEBUG_BUYER` | 0 | 买家调试输出 |
| `AuctionHouseBot.DEBUG_SELLER` | 0 | 卖家调试输出 |
| `AuctionHouseBot.TRACE_SELLER` | 0 | 记录每笔卖出明细 |
| `AuctionHouseBot.TRACE_BUYER` | 0 | 记录每笔买入明细 |
| `AuctionHouseBot.EnableSeller` | 0 | 开启卖家（挂卖物品） |
| `AuctionHouseBot.EnableBuyer` | 0 | 开启买家（收购物品） |
| `AuctionHouseBot.UseBuyPriceForSeller` | 0 | 卖家定价用 BuyPrice 还是 SellPrice；0=用 SellPrice |
| `AuctionHouseBot.UseBuyPriceForBuyer` | 0 | 买家心理价位用 BuyPrice 还是 SellPrice；0=用 SellPrice |
| `AuctionHouseBot.UseMarketPriceForSeller` | 0 | 卖家是否按"市场价"（历史成交均价）定价 |
| `AuctionHouseBot.MarketResetThreshold` | 25 | 同一物品累计多少次成交后重置市场价统计，使其快速适应行情；越小反应越快，越大越平滑 |
| `AuctionHouseBot.Account` | 0 | bot 所在账号 ID（`acore_auth.account.id`）；0=禁用 |
| `AuctionHouseBot.GUID` | 0 | bot 角色 GUID（`acore_characters.characters.guid`）；0=用该账号全部角色 |
| `AuctionHouseBot.ItemsPerCycle` | 200 | 每次补货周期最多上/下架物品数 |
| `AuctionHouseBot.ConsiderOnlyBotAuctions` | 0 | 计数时只统计 bot 自己的挂单（0=把玩家挂单也算进配额） |
| `AuctionHouseBot.DuplicatesCount` | 0 | 同一物品 bot 在市场上允许存在的最大重复堆叠数；0=不限 |
| `AuctionHouseBot.DivisibleStacks` | 0 | 按最大堆叠的约数拆分堆叠（如最大 20 → 5/10/15/20），而非随机 |
| `AuctionHouseBot.ElapsingTimeClass` | 1 | 挂单时长档位：0=长(1~3天)、1=中(1~24小时)、2=短(10~60分钟) |

### 4.2 过滤器 Part 1（物品来源 / 绑定类型 / 特殊物品）

| 配置项 | 默认 | 说明 |
|--------|:---:|------|
| `AuctionHouseBot.VendorItems` | 0 | 允许上架可在 NPC 购买的普通物品 |
| `AuctionHouseBot.VendorTradeGoods` | 0 | 允许上架可在 NPC 购买的贸易商品 |
| `AuctionHouseBot.LootItems` | 1 | 允许上架可掉落/钓鱼获得的普通物品 |
| `AuctionHouseBot.LootTradeGoods` | 1 | 允许上架可掉落/钓鱼获得的贸易商品 |
| `AuctionHouseBot.OtherItems` | 0 | 允许上架其它杂项普通物品 |
| `AuctionHouseBot.OtherTradeGoods` | 0 | 允许上架其它杂项贸易商品 |
| `AuctionHouseBot.ProfessionItems` | 0 | 允许上架职业必需物品（读 `auctionhousebot_professionItems` 表） |
| `AuctionHouseBot.No_Bind` | 1 | 允许"不绑定"物品 |
| `AuctionHouseBot.Bind_When_Picked_Up` | 0 | 允许"拾取绑定"物品 |
| `AuctionHouseBot.Bind_When_Equipped` | 1 | 允许"装备绑定"物品 |
| `AuctionHouseBot.Bind_When_Use` | 1 | 允许"使用绑定"物品 |
| `AuctionHouseBot.Bind_Quest_Item` | 0 | 允许"任务物品" |
| `AuctionHouseBot.DisablePermEnchant` | 0 | 禁用永久附魔物品 |
| `AuctionHouseBot.DisableConjured` | 0 | 禁用魔法制造（消耗品）物品 |
| `AuctionHouseBot.DisableGems` | 0 | 禁用宝石 |
| `AuctionHouseBot.DisableMoney` | 0 | 禁用作为货币的物品 |
| `AuctionHouseBot.DisableMoneyLoot` | 0 | 禁用携带金钱掉落物的物品 |
| `AuctionHouseBot.DisableLootable` | 0 | 禁用可被打开取物的容器类物品 |
| `AuctionHouseBot.DisableKeys` | 0 | 禁用钥匙类物品 |
| `AuctionHouseBot.DisableDuration` | 0 | 禁用带持续时间的物品 |
| `AuctionHouseBot.DisableBOP_Or_Quest_NoReqLevel` | 0 | 禁用"拾取绑定/任务物品且需求等级<物品等级"的装备（防止低级穿高级装） |

### 4.3 过滤器 Part 2（按职业屏蔽）

以下均为布尔开关，`1` 表示禁止该职业专属物品上架，默认全部 `0`（允许）：

`DisableWarriorItems` / `DisablePaladinItems` / `DisableHunterItems` / `DisableRogueItems` / `DisablePriestItems` / `DisableDKItems` / `DisableShamanItems` / `DisableMageItems` / `DisableWarlockItems` / `DisableUnusedClassItems` / `DisableDruidItems`。

### 4.4 过滤器 Part 3（按等级 / Entry / 需求等级 / 需求技能屏蔽）

以下均默认 `0`（Off）。区分普通物品（Items）与贸易商品（TGs）两套：

| 配置项 | 含义 |
|--------|------|
| `DisableItemsBelowLevel` / `DisableItemsAboveLevel` | 按物品等级（ItemLevel）上下限屏蔽普通物品 |
| `DisableTGsBelowLevel` / `DisableTGsAboveLevel` | 按物品等级上下限屏蔽贸易商品 |
| `DisableItemsBelowGUID` / `DisableItemsAboveGUID` | 按物品 Entry 上下限屏蔽普通物品 |
| `DisableTGsBelowGUID` / `DisableTGsAboveGUID` | 按物品 Entry 上下限屏蔽贸易商品 |
| `DisableItemsBelowReqLevel` / `DisableItemsAboveReqLevel` | 按需求等级（RequiredLevel）上下限屏蔽普通物品 |
| `DisableTGsBelowReqLevel` / `DisableTGsAboveReqLevel` | 按需求等级上下限屏蔽贸易商品 |
| `DisableItemsBelowReqSkillRank` / `DisableItemsAboveReqSkillRank` | 按需求技能点数上下限屏蔽普通物品 |
| `DisableTGsBelowReqSkillRank` / `DisableTGsAboveReqSkillRank` | 按需求技能点数上下限屏蔽贸易商品 |

### 4.5 过滤器 Part 4（白名单）

| 配置项 | 默认 | 说明 |
|--------|:---:|------|
| `AuctionHouseBot.SellerWhiteList` | `""` | 卖家白名单，填 item_template 的 entry，逗号分隔（如 `"1, 2, 3"`）。**非空时绕过黑名单，只卖白名单内物品** |

> 注意：`AuctionHouseBot.DisabledItems` 旧配置项已废弃，黑名单改由数据库表 `mod_auctionhousebot_disabled_items` 维护。

---

## 五、数据库表结构（acore_world）

### 5.1 `mod_auctionhousebot` —— 每个拍卖行一条配置

主键 `auctionhouse`（AH 的 mapID）：**2=联盟、6=部落、7=中立**。

| 字段 | 默认 | 说明 |
|------|:---:|------|
| `auctionhouse` | — | 拍卖行 mapID（2/6/7），主键 |
| `name` | — | 拍卖行名称（文本） |
| `minitems` | 0 | 想维持的**最低**挂单数；0=与 maxitems 相同 |
| `maxitems` | 0 | 想维持的**最高**挂单数（达到后卖家停止补货） |
| `percentgreytradegoods` ~ `percentyellowtradegoods` | 见下 | 7 档品质的**贸易商品**上架占比（%） |
| `percentgreyitems` ~ `percentyellowitems` | 见下 | 7 档品质的**普通物品**上架占比（%） |
| `minprice{color}` / `maxprice{color}` | 见下 | 卖家一口价相对基础价的随机百分比区间 |
| `minbidprice{color}` / `maxbidprice{color}` | 见下 | 起拍价占一口价的随机百分比区间 |
| `maxstack{color}` | 0 | 该品质物品最大堆叠数；0=按物品自身允许的最大堆叠 |
| `buyerprice{color}` | 见下 | 买家收购价 = 供应商价 × 此倍数 |
| `buyerbiddinginterval` | 1 | 买家每隔多少**分钟**在对应 AH 竞价一次 |
| `buyerbidsperinterval` | 1 | 每个竞价周期最多投多少笔 |

**默认百分比占比**（两列之和必须 = 100，否则改动不会被接受）：

| 品质 | 贸易商品占比 | 普通物品占比 |
|------|:---:|:---:|
| 灰 | 0 | 0 |
| 白 | 27 | 10 |
| 绿 | 12 | 30 |
| 蓝 | 10 | 8 |
| 紫 | 1 | 2 |
| 橙 | 0 | 0 |
| 黄 | 0 | 0 |

**默认价格区间（卖家一口价，相对基础价的百分比）**：

| 品质 | min~max | 品质 | min~max |
|------|:---:|------|:---:|
| 灰 | 100 ~ 150 | 紫 | 2250 ~ 4550 |
| 白 | 150 ~ 250 | 橙 | 3250 ~ 5550 |
| 绿 | 800 ~ 1400 | 黄 | 5250 ~ 6550 |
| 蓝 | 1250 ~ 1750 | | |

**默认起拍价区间（占一口价的百分比，`minbidprice`~`maxbidprice`）**：灰/白 70~100，其余 80~100。

**默认买家收购倍数（`buyerprice`，相对供应商价）**：灰 1、白 3、绿 5、蓝 12、紫 15、橙 20、黄 22。

**默认 maxstack**：灰 0、白 0、绿 3、蓝 2、紫 1、橙 1、黄 1（贸易商品与普通物品同表共用字段）。

### 5.2 `mod_auctionhousebot_disabled_items` —— 卖家黑名单

| 字段 | 说明 |
|------|------|
| `item` | 物品 entry，主键。卖家不会上架此表内物品 |

默认 SQL 已预置数千条 PTR/Beta/废弃物品（`z_filter_disabled_and_trash.sql` 会按名称规则追加更多，如名称含 `OLD`、`NPC`、`QA`、`deprecated`、`test` 等）。

### 5.3 `auctionhousebot_professionItems` —— 职业物品清单

| 字段 | 说明 |
|------|------|
| `Entry` | 物品 entry。当配置 `ProfessionItems = 1` 时，此表物品会被纳入可卖 loot 集合 |

---

## 六、GM 命令说明（`ahbotoptions`）

权限等级：`SEC_GAMEMASTER`，控制台与游戏内均可用。AH id 固定为 2/6/7。

| 命令 | 语法 | 说明 |
|------|------|------|
| `buyer` | `ahbotoptions buyer 0\|1` | 全局开/关买家（不区分 AH） |
| `seller` | `ahbotoptions seller 0\|1` | 全局开/关卖家 |
| `usemarketprice` | `ahbotoptions usemarketprice 0\|1` | 全局开/关按市场价定价 |
| `ahexpire` | `ahbotoptions ahexpire <ahID>` | 立即让指定 AH 内 bot 的挂单全部过期下架 |
| `minitems` | `ahbotoptions minitems <ahID> <n>` | 设最低挂单数 |
| `maxitems` | `ahbotoptions maxitems <ahID> <n>` | 设最高挂单数 |
| `percentages` | `ahbotoptions percentages <ahID> <14个数>` | 设 14 档占比（合计须=100） |
| `minprice` | `ahbotoptions minprice <ahID> <color> <p>` | 设某品质一口价下限（%） |
| `maxprice` | `ahbotoptions maxprice <ahID> <color> <p>` | 设某品质一口价上限（%） |
| `minbidprice` | `ahbotoptions minbidprice <ahID> <color> <p>` | 设某品质起拍价下限（1~100） |
| `maxbidprice` | `ahbotoptions maxbidprice <ahID> <color> <p>` | 设某品质起拍价上限（1~100） |
| `maxstack` | `ahbotoptions maxstack <ahID> <color> <v>` | 设某品质最大堆叠 |
| `buyerprice` | `ahbotoptions buyerprice <ahID> <color> <v>` | 设某品质买家收购倍数 |
| `bidinterval` | `ahbotoptions bidinterval <ahID> <min>` | 设竞价周期（分钟） |
| `bidsperinterval` | `ahbotoptions bidsperinterval <ahID> <n>` | 设每周期竞价笔数 |

`<color>` 取值：`grey / white / green / blue / purple / orange / yellow`。

---

## 七、定价与买卖算法详解

### 7.1 卖家定价（`AuctionHouseBot::Sell`）

1. **基础价确定**：
   - 若 `UseMarketPriceForSeller=1` 且存在该物品历史成交价 → `basePrice = GetItemPrice(itemID)`（市场均价）；
   - 否则按 `UseBuyPriceForSeller` 取 `BuyPrice` 或 `SellPrice`。
2. **一口价**：`buyout = basePrice × urand(minPrice[品质], maxPrice[品质]) / 100`
   即基础价乘一个品质相关的随机百分比（如绿色默认 800%~1400%）。
3. **起拍价**：`bid = buyout × urand(minBidPrice[品质], maxBidPrice[品质]) / 100`（默认 70%~100%）。
4. **堆叠**：按 `maxstack[品质]` 与 `DivisibleStacks` 决定单堆数量。
5. **挂单时长**：按 `ElapsingTimeClass` 随机（见 4.1）。
6. **最终挂单**：`startbid = bid × stackCount`，`buyout = buyout × stackCount`，押金由核心 `GetAuctionDeposit` 计算。

### 7.2 买家出价（`AuctionHouseBot::Buy`）

1. 从数据库取"非 bot 拥有、且 bot 未出过价"的挂单。
2. **心理价位上限**：
   `maximumBid = basePrice(BuyPrice 或 SellPrice) × itemCount × buyerprice[品质]`
3. 弹药/通用/货币/永久附魔四类物品 `maximumBid=0`，直接跳过（不收购）。
4. 若 `currentPrice > maximumBid` 跳过；否则：
   `bidPrice = currentPrice + (maximumBid - currentPrice) × urand(1,100)/100`，
   并保证 ≥ `currentPrice + 最低加价`，且 ≤ `maximumBid`。
5. **出价 vs 一口价**：
   - 若 `bidPrice < buyout`（或 `buyout=0`）→ 正常竞价；
   - 否则 → 直接一口价买断，走成交流程（发成交邮件、删除物品、触发市场价统计）。

### 7.3 市场价机制（可选增强）

- 每当拍卖**成交**（`OnAuctionSuccessful`）或**过期**（`OnAuctionExpire`），`UpdateItemStats` 记录该物品的**每单位成交价**并累计均值。
- 当同一物品累计成交次数超过 `MarketResetThreshold ± 随机扰动(±9)` 时，重置统计以让价格快速适应行情；否则用历史均价平滑波动。
- 卖家若开启 `UseMarketPriceForSeller`，将以这个滚动均价作为基础价，形成随市场供需波动的价格。

---

## 八、常见问题与注意事项

1. **Account / GUID 必须指向真实存在的账号与角色**。若 `Account=0` 且 `GUID=0`，启动时直接报错禁用；若 Account 指向不存在的账号或该账号无角色，模块同样无法启动。配置后建议核对：
   - `acore_auth.account` 中该 `Account` 存在；
   - `acore_characters.characters` 中存在 `account = <Account>` 且 `guid = <GUID>`（若指定了 GUID）的角色。
2. **占比之和必须为 100**。`percentages` 命令或直接改 `mod_auctionhousebot` 表时，14 档占比合计若不为 100，修改会被拒绝并回退默认值。
3. **黑名单/白名单二选一**。若 `SellerWhiteList` 非空，则黑名单表被绕过，仅卖白名单物品。若黑名单表与白名单同时为空，模块会主动清空所有分箱并**关闭卖家**（避免无筛选乱卖）。
4. **bot 角色勿登录使用**。用它浏览拍卖行可能出现界面卡死。
5. **`ConsiderOnlyBotAuctions` 的含义**：开启后配额计数只统计 bot 自己的挂单（玩家挂单不占配额），适合玩家活跃时保持"背景库存"；关闭则需要相应调大 `maxitems`，否则玩家挂单会挤占配额。
6. **卖家只会在 `maxitems` 未满时补货**，且每周期最多补 `ItemsPerCycle` 件；买家按 `buyerbiddinginterval` 分钟间隔执行一次，每次最多 `buyerbidsperinterval` 笔。
7. **中立 AH 始终参与**，联盟/部落 AH 在 `ALLOW_TWO_SIDE_INTERACTION_AUCTION` 开启（跨阵营拍卖）时跳过，仅中立生效。

---

*报告完。*
