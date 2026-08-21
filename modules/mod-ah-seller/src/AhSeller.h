/*
 * mod-ah-seller - 拍卖行卖家池模块
 * 按池子上架物品：类型池（按分类/等级/品质筛一类，统一价）与 Entry 池（指定 entry，可单独价）。
 * 每池独立配置：最多挂架数、补货间隔、单次补货量、上架时长、堆叠数、上下浮比例。
 */

#ifndef AH_SELLER_H
#define AH_SELLER_H

#include "Common.h"

#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

class ItemTemplate;
class Player;

// Entry 池中单个物品的可选覆盖价（铜）
struct SellerPoolItem
{
    uint32 ItemId = 0;
    uint64 BuyoutPrice = 0;   // 0=用池子默认价
    uint64 BidPrice = 0;      // 0=用池子默认价
};

struct SellerPool
{
    uint32 Id = 0;
    uint8  PoolType = 1;      // 1=类型池 2=Entry 池
    bool   Enabled = true;

    // 类型池筛选条件（pool_type=1 生效）
    int32  ItemClass = -1;    // -1=不限
    int32  ItemSubclass = -1; // -1=不限
    uint32 MinItemLevel = 0;
    uint32 MaxItemLevel = 0;
    uint32 MinQuality = 0;
    uint32 MaxQuality = 0;

    // 定价（铜，每件单价）
    uint64 BuyoutPrice = 0;   // 0=使用物品 SellPrice
    uint64 BidPrice = 0;      // 0=按一口价×80%

    // 浮动控制（百分比）
    double PriceUpPct = 0.0;    // 卖得好最多上浮；0=不浮动
    double PriceDownPct = 0.0;  // 卖得差最多下浮
    double PriceStepPct = 3.0;  // 每次成交/过期调整的步进

    // 上架策略
    uint32 MaxCount = 0;         // 同时最多挂架堆数，0=不限制
    uint32 RestockInterval = 600;// 补货间隔（秒）
    uint32 RestockCount = 1;     // 单次补货数量（受 MaxCount 封顶）
    uint32 DurationHours = 12;   // 上架持续小时数
    uint32 StackCount = 0;       // 每堆数量，0=不堆叠（1件/堆）

    // 运行时状态
    std::vector<uint32> Items;                                  // 命中 entry 列表（随机选品用）
    std::unordered_map<uint32, SellerPoolItem> ItemOverrides;   // Entry 池：entry -> 单独价
    uint32 CurrentCount = 0;                                    // 当前 bot 挂单堆数
    time_t LastRestock = 0;                                     // 上次补货时间

    bool Contains(uint32 itemId) const;
};

// 全局状态
extern std::vector<SellerPool> gSellerPools;
extern uint32 gAhSellerAccount;
extern uint32 gAhSellerGuid;
extern bool   gAhSellerEnabled;
extern bool   gAhSellerDebug;
extern std::unordered_map<uint32, int32> gAhSellerHeat;   // itemId -> 热度（成交+1 过期-1）

void AhSellerLoadPools();
void AhSellerRestockTick();
void AhSellerUpdateHeat(uint32 itemId, int32 delta);
void AhSellerOnAuctionAdd(uint32 itemId);
void AhSellerOnAuctionRemove(uint32 itemId);

#endif // AH_SELLER_H
