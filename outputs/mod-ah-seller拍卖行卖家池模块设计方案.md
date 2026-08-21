# mod-ah-seller 拍卖行卖家池模块（最终设计）

> 已按最终需求落地实现为独立模块 `modules/mod-ah-seller/`（重做，不基于 mod-ah-bot）。
> 本文档与代码、SQL 保持一致。

## 一、需求落地对照

| 需求 | 实现 |
|------|------|
| ① 按规则指定某类型物品，统一金额 | **类型池** `pool_type=1`：按 class/subclass/等级/品质筛一类，全池统一 `buyout_price_gold` |
| ② 按 entry 指定一组物品，单独金额 | **Entry 池** `pool_type=2`：物品在 `ah_seller_pool_items`，每个 entry 单独价 |
| 每池最多挂架数量 | `max_count`（堆数），补货时 `当前存量 < max_count` 才补 |
| 池内随机上架 | 从池子命中 entry 列表 `urand` 随机选一件 |
| 补货间隔 | `restock_interval`（秒） |
| 补货数量（≤最多架） | `restock_count`，实际被 `max_count - 当前存量` 封顶 |
| 上架持续时长（默认12h） | `duration_hours`，默认 12 |
| 卖多上浮 ≤30%、卖少下浮 ≤30% | `price_up_pct` / `price_down_pct`（默认 0=不浮动），heat 机制 |
| 每池单独配上下浮 | 上述字段在每池独立 |
| 不分阵营 | 统一挂**中立拍卖行**（`AuctionHouseId::Neutral=7`） |
| 重启不保留浮动 | heat 存内存 `std::unordered_map`，重启清零 |
| 不设价=用物品 SellPrice，0 忽略 | `buyout_price_gold=0` 时取 `item_template.SellPrice`；仍为 0 则跳过该物品 |
| 浮动 0 = 不记录、不调价 | 池子 `up_pct/down_pct` 均为 0 时，成交/过期不更新 heat，定价固定 |
| 每池可配堆叠数量 | `stack_count`；不支持堆叠（MaxStackCount=1）的物品自动 1 件/堆 |

## 二、数据模型（acore_world）

### `ah_seller_pool`（池子）

| 字段 | 默认 | 说明 |
|------|:---:|------|
| `id` | 自增 | 主键 |
| `pool_type` | 1 | 1=类型池 2=Entry 池 |
| `enabled` | 1 | 启用开关 |
| `item_class` / `item_subclass` | -1 | 类型池筛选（-1=不限） |
| `min/max_item_level` | 0 | 物品等级范围 |
| `min/max_quality` | 0 | 品质范围（0灰..6黄） |
| `buyout_price_gold` | 0.00 | 统一一口价（金币/每件）；0=用 SellPrice |
| `bid_price_gold` | 0.00 | 起拍价；0=一口价×80% |
| `price_up_pct` / `price_down_pct` | 0.00 | 上下浮上限；0=不浮动 |
| `price_step_pct` | 5.00 | 每次成交/过期调整步进 |
| `max_count` | 0 | 最多挂架堆数；0=不限制 |
| `restock_interval` | 600 | 补货间隔（秒） |
| `restock_count` | 1 | 单次补货量 |
| `duration_hours` | 12 | 上架时长 |
| `stack_count` | 0 | 每堆数量；0=1件/堆 |

### `ah_seller_pool_items`（Entry 池物品）

| 字段 | 说明 |
|------|------|
| `pool_id` | 所属池子 |
| `item_id` | 物品 entry |
| `buyout_price_gold` / `bid_price_gold` | 该 entry 单独价；0=用池子默认 |

## 三、代码结构（已实现）

```
modules/mod-ah-seller/
├── conf/mod_ah_seller.conf.dist        # AhSeller.Enable/Account/GUID/Debug
├── data/sql/db-world/mod_ah_seller.sql # 建表 + 示例
└── src/
    ├── AhSeller.h                      # SellerPool / SellerPoolItem 结构 + 全局状态
    ├── AhSeller.cpp                    # 加载池子、补货调度、定价、浮动、上架
    └── AhSellerScripts.cpp             # WorldScript / AuctionHouseScript / MailScript + 入口
```

- **入口**：`Addmod_ah_sellerScripts()`（模块目录 `mod-ah-seller` 自动被 CMake 发现，`MODULES=static` 静态编译）。
- **钩子**：
  - `WorldScript::OnBeforeConfigLoad` → 读 conf；`OnStartup` → `AhSellerLoadPools()`。
  - `AuctionHouseScript::OnBeforeAuctionHouseMgrUpdate` → `AhSellerRestockTick()`（每池时间戳比较补货）。
  - `OnAuctionAdd/Remove` → 维护每池 `CurrentCount`（只统计 bot 挂单）。
  - `OnAuctionSuccessful/Expire` → 维护 `heat`（成交+1 / 过期-1，仅浮动开启的池子）。
  - `MailScript` → 拦截发往 bot 的成交邮件。

## 四、关键逻辑

**定价优先级**（每件一口价）：

```
Entry 池：物品单独价  >  池子统一价  >  物品 SellPrice
类型池：        池子统一价  >  物品 SellPrice
（最终为 0 → 跳过该物品）
```

**浮动**：`heat[itemId]` 成交+1、过期-1；定价 = 基础价 × (1 + clamp(heat×step, -down, +up))。仅当池子 `up_pct/down_pct` 非 0 才参与。

**堆叠**：`stackCount = min(stack_count, item.MaxStackCount)`，`stack_count=0` 时 1 件/堆。

**金币换算**：内部铜 = 金币 × 10000（`COPPER=1, SILVER=100, GOLD=10000`）。

## 五、启用步骤

1. 导入 `mod_ah_seller.sql` 到 acore_world，配置池子。
2. 复制 `mod_ah_seller.conf.dist` 为 `mod_ah_seller.conf`，填 `Account`/`GUID`（bot 角色，需真实存在），`Enable=1`。
3. 重新 cmake + 编译 worldserver。
4. 观察日志 `AhSeller: 已加载 N 个池子`，AH 中按池子补货。

---

*与代码同步。*
