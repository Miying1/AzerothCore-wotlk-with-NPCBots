/*
 * mod-ah-seller - 拍卖行卖家池模块
 * 核心逻辑：加载池子配置、定时补货、随机选品、定价（含需求浮动）、上架。
 */

#include "AhSeller.h"

#include "AuctionHouseMgr.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"

#include <algorithm>
#include <cmath>

std::vector<SellerPool> gSellerPools;
uint32 gAhSellerAccount = 0;
uint32 gAhSellerGuid = 0;
bool   gAhSellerEnabled = false;
bool   gAhSellerDebug = false;
std::unordered_map<uint32, int32> gAhSellerHeat;

// 金币转铜（1 金 = 10000 铜）
static uint64 GoldToCopper(double gold)
{
    if (gold <= 0.0)
        return 0;
    return (uint64)std::llround(gold * 10000.0);
}

bool SellerPool::Contains(uint32 itemId) const
{
    return std::find(Items.begin(), Items.end(), itemId) != Items.end();
}

// 判断物品是否命中类型池的筛选条件
static bool MatchTypePool(SellerPool const& pool, ItemTemplate const& proto)
{
    if (pool.ItemClass >= 0 && (int32)proto.Class != pool.ItemClass)
        return false;
    if (pool.ItemSubclass >= 0 && (int32)proto.SubClass != pool.ItemSubclass)
        return false;
    if (pool.MinItemLevel && proto.ItemLevel < pool.MinItemLevel)
        return false;
    if (pool.MaxItemLevel && proto.ItemLevel > pool.MaxItemLevel)
        return false;
    if (pool.MinRequiredLevel && proto.RequiredLevel < pool.MinRequiredLevel)
        return false;
    if (pool.MaxRequiredLevel && proto.RequiredLevel > pool.MaxRequiredLevel)
        return false;
    if (pool.BagFamily && (proto.BagFamily & pool.BagFamily) == 0)
        return false;
    if (pool.MinQuality && proto.Quality < pool.MinQuality)
        return false;
    if (pool.MaxQuality && proto.Quality > pool.MaxQuality)
        return false;
    return true;
}

void AhSellerLoadPools()
{
    gSellerPools.clear();

    // 读取池子定义
    QueryResult poolResult = WorldDatabase.Query(
        "SELECT id, pool_type, enabled, item_class, item_subclass, "
        "min_item_level, max_item_level, min_required_level, max_required_level, bag_family, min_quality, max_quality, "
        "buyout_price_gold, bid_price_gold, price_up_pct, price_down_pct, price_step_pct, "
        "max_count, restock_interval, restock_count, duration_hours, stack_count "
        "FROM mod_ah_seller_pool ORDER BY id");

    if (!poolResult)
    {
        LOG_WARN("module", "AhSeller: 未找到 mod_ah_seller_pool 表或表为空，模块不生效");
        return;
    }

    do
    {
        Field* f = poolResult->Fetch();

        SellerPool pool;
        pool.Id              = f[0].Get<uint32>();
        pool.PoolType        = f[1].Get<uint8>();
        pool.Enabled         = f[2].Get<uint8>() != 0;
        pool.ItemClass       = f[3].Get<int32>();
        pool.ItemSubclass    = f[4].Get<int32>();
        pool.MinItemLevel      = f[5].Get<uint32>();
        pool.MaxItemLevel      = f[6].Get<uint32>();
        pool.MinRequiredLevel  = f[7].Get<uint32>();
        pool.MaxRequiredLevel  = f[8].Get<uint32>();
        pool.BagFamily         = f[9].Get<uint32>();
        pool.MinQuality        = f[10].Get<uint32>();
        pool.MaxQuality        = f[11].Get<uint32>();
        pool.BuyoutPrice       = GoldToCopper(f[12].Get<double>());
        pool.BidPrice          = GoldToCopper(f[13].Get<double>());
        pool.PriceUpPct        = f[14].Get<double>();
        pool.PriceDownPct      = f[15].Get<double>();
        pool.PriceStepPct      = f[16].Get<double>();
        pool.MaxCount          = f[17].Get<uint32>();
        pool.RestockInterval   = f[18].Get<uint32>();
        pool.RestockCount      = f[19].Get<uint32>();
        pool.DurationHours     = f[20].Get<uint32>();
        pool.StackCount        = f[21].Get<uint32>();

        if (pool.DurationHours == 0)
            pool.DurationHours = 12;   // 默认 12 小时

        gSellerPools.push_back(pool);
    } while (poolResult->NextRow());

    // 读取 Entry 池物品（单独价）
    QueryResult itemResult = WorldDatabase.Query(
        "SELECT pool_id, item_id, buyout_price_gold, bid_price_gold FROM mod_ah_seller_pool_items");

    if (itemResult)
    {
        do
        {
            Field* f = itemResult->Fetch();
            uint32 poolId = f[0].Get<uint32>();

            for (SellerPool& pool : gSellerPools)
            {
                if (pool.Id != poolId)
                    continue;

                SellerPoolItem item;
                item.ItemId      = f[1].Get<uint32>();
                item.BuyoutPrice = GoldToCopper(f[2].Get<double>());
                item.BidPrice    = GoldToCopper(f[3].Get<double>());
                pool.ItemOverrides[item.ItemId] = item;
                break;
            }
        } while (itemResult->NextRow());
    }

    // 构建每个池子的命中 entry 列表
    ItemTemplateContainer const* store = sObjectMgr->GetItemTemplateStore();

    for (SellerPool& pool : gSellerPools)
    {
        if (!pool.Enabled)
            continue;

        if (pool.PoolType == 2)
        {
            // Entry 池：命中列表 = 物品表里的 entry
            for (auto const& kv : pool.ItemOverrides)
                pool.Items.push_back(kv.first);
        }
        else
        {
            // 类型池：遍历 item_template 筛选
            for (ItemTemplateContainer::const_iterator itr = store->begin(); itr != store->end(); ++itr)
            {
                ItemTemplate const& proto = itr->second;

                if (!MatchTypePool(pool, proto))
                    continue;

                // 可定价检查：池子有统一价，或物品 SellPrice > 0
                if (pool.BuyoutPrice == 0 && proto.SellPrice == 0)
                    continue;

                pool.Items.push_back(proto.ItemId);
            }
        }

        if (gAhSellerDebug)
            LOG_INFO("module", "AhSeller: 池子 {} (type={}) 命中 {} 件物品", pool.Id, (uint32)pool.PoolType, (uint32)pool.Items.size());
    }

    // 初始化当前存量（遍历中立 AH 现有 bot 挂单）
    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId::Neutral);
    if (auctionHouse)
    {
        for (AuctionHouseObject::AuctionEntryMap::const_iterator itr = auctionHouse->GetAuctionsBegin();
             itr != auctionHouse->GetAuctionsEnd(); ++itr)
        {
            AuctionEntry* entry = itr->second;
            if (entry->owner.GetCounter() != gAhSellerGuid)
                continue;
            AhSellerOnAuctionAdd(entry->item_template);
        }
    }

    LOG_INFO("server.loading", "AhSeller: 已加载 {} 个池子", (uint32)gSellerPools.size());
}

// 解析某物品的最终基础一口价（铜），返回 0 表示无法定价（忽略）
static uint64 ResolveBuyout(SellerPool const& pool, ItemTemplate const& proto, uint32 itemId)
{
    uint64 buyout = 0;

    if (pool.PoolType == 2)
    {
        auto it = pool.ItemOverrides.find(itemId);
        if (it != pool.ItemOverrides.end() && it->second.BuyoutPrice > 0)
            buyout = it->second.BuyoutPrice;
    }

    if (buyout == 0)
        buyout = pool.BuyoutPrice;
    if (buyout == 0)
        buyout = proto.SellPrice;

    return buyout;
}

// 上架一件物品到中立 AH；返回是否成功
static bool SellOne(Player* bot, SellerPool& pool)
{
    if (pool.Items.empty())
        return false;

    uint32 itemId = pool.Items[urand(0, (uint32)pool.Items.size() - 1)];

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
    if (!proto)
    {
        LOG_WARN("module", "AhSeller: 池子 {} 内物品 {} 未在 item_template 中找到，已跳过", pool.Id, itemId);
        return false;
    }

    // 基础一口价
    uint64 baseBuyout = ResolveBuyout(pool, *proto, itemId);
    if (baseBuyout == 0)
        return false;   // SellPrice=0 且未指定价 → 忽略

    // 需求浮动比例（一口价与显式起拍价使用同一比例）
    double pct = 0.0;
    if (pool.PriceUpPct > 0.0 || pool.PriceDownPct > 0.0)
    {
        int32 heat = 0;
        auto hit = gAhSellerHeat.find(itemId);
        if (hit != gAhSellerHeat.end())
            heat = hit->second;

        pct = heat * pool.PriceStepPct;
        if (pct > pool.PriceUpPct)
            pct = pool.PriceUpPct;
        if (pct < -pool.PriceDownPct)
            pct = -pool.PriceDownPct;
    }

    // 一口价（含需求浮动）
    uint64 buyout = (uint64)((double)baseBuyout * (100.0 + pct) / 100.0);

    // 起拍价：显式配置的起拍价应用相同浮动比例；未配置则默认一口价的 80%
    uint64 bid = 0;
    if (pool.PoolType == 2)
    {
        auto it = pool.ItemOverrides.find(itemId);
        if (it != pool.ItemOverrides.end() && it->second.BidPrice > 0)
            bid = it->second.BidPrice;
    }
    if (bid == 0)
        bid = pool.BidPrice;
    if (bid == 0)
        bid = buyout * 80 / 100;
    else
        bid = (uint64)((double)bid * (100.0 + pct) / 100.0);

    // 起拍价不高于一口价
    if (bid > buyout)
        bid = buyout;

    // 创建物品
    Item* item = Item::CreateItem(itemId, 1, bot);
    if (!item)
        return false;

    item->AddToUpdateQueueOf(bot);

    uint32 randomPropertyId = Item::GenerateItemRandomPropertyId(itemId);
    if (randomPropertyId != 0)
        item->SetItemRandomProperties(randomPropertyId);

    // 堆叠数量
    uint32 stackCount = 1;
    if (pool.StackCount > 1 && item->GetMaxStackCount() > 1)
        stackCount = std::min(pool.StackCount, item->GetMaxStackCount());
    item->SetCount(stackCount);

    // 单价 × 堆叠数，超出 MAX_MONEY_AMOUNT 会截断 uint32，上架前直接跳过
    uint64 bidTotal = bid * stackCount;
    uint64 buyoutTotal = buyout * stackCount;
    if (bidTotal > MAX_MONEY_AMOUNT || buyoutTotal > MAX_MONEY_AMOUNT)
    {
        LOG_WARN("module", "AhSeller: 池子 {} 物品 {} x{} 价格超出上限(bid={} buyout={})，已跳过上架",
                 pool.Id, itemId, stackCount, bidTotal, buyoutTotal);
        item->RemoveFromUpdateQueueOf(bot);
        delete item;
        return false;
    }

    // 时长与押金
    uint32 durationSeconds = pool.DurationHours * 3600;
    AuctionHouseEntry const* ahEntry = sAuctionMgr->GetAuctionHouseEntryFromHouse(AuctionHouseId::Neutral);
    AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsMapByHouseId(AuctionHouseId::Neutral);
    if (!ahEntry || !auctionHouse)
    {
        item->RemoveFromUpdateQueueOf(bot);
        delete item;
        return false;
    }

    uint32 deposit = sAuctionMgr->GetAuctionDeposit(ahEntry, durationSeconds, item, stackCount);

    // 上架
    auto trans = CharacterDatabase.BeginTransaction();

    AuctionEntry* auctionEntry = new AuctionEntry();
    auctionEntry->Id                = sObjectMgr->GenerateAuctionID();
    auctionEntry->houseId           = AuctionHouseId::Neutral;
    auctionEntry->item_guid         = item->GetGUID();
    auctionEntry->item_template     = item->GetEntry();
    auctionEntry->itemCount         = item->GetCount();
    auctionEntry->owner             = bot->GetGUID();
    auctionEntry->startbid          = (uint32)bidTotal;
    auctionEntry->buyout            = (uint32)buyoutTotal;
    auctionEntry->bid               = 0;
    auctionEntry->deposit           = deposit;
    auctionEntry->expire_time       = (time_t)durationSeconds + time(nullptr);
    auctionEntry->auctionHouseEntry = ahEntry;

    item->SaveToDB(trans);
    item->RemoveFromUpdateQueueOf(bot);
    sAuctionMgr->AddAItem(item);
    auctionHouse->AddAuction(auctionEntry);
    auctionEntry->SaveToDB(trans);

    CharacterDatabase.CommitTransaction(trans);

    if (gAhSellerDebug)
        LOG_INFO("module", "AhSeller: 上架池 {} 物品 {} x{} bid={} buyout={}",
                 pool.Id, itemId, stackCount, auctionEntry->startbid, auctionEntry->buyout);

    return true;
}

// 每次 AH 管理器更新时调用，处理各池补货
void AhSellerRestockTick()
{
    time_t now = time(nullptr);

    // 是否有池子需要补货
    bool any = false;
    for (SellerPool& pool : gSellerPools)
    {
        if (!pool.Enabled || pool.RestockInterval == 0)
            continue;
        if ((now - pool.LastRestock) < (time_t)pool.RestockInterval)
            continue;
        if (pool.MaxCount > 0 && pool.CurrentCount >= pool.MaxCount)
            continue;
        any = true;
    }
    if (!any)
        return;

    // 创建虚拟 bot 玩家
    std::string accountName = "AhSeller" + std::to_string(gAhSellerAccount);
    WorldSession session(gAhSellerAccount, std::move(accountName), 0, nullptr, SEC_PLAYER,
                         sWorld->getIntConfig(CONFIG_EXPANSION), 0, LOCALE_enUS, 0, false, false, 0);
    Player bot(&session);
    bot.Initialize(gAhSellerGuid);
    ObjectAccessor::AddObject(&bot);

    for (SellerPool& pool : gSellerPools)
    {
        if (!pool.Enabled || pool.RestockInterval == 0)
            continue;
        if ((now - pool.LastRestock) < (time_t)pool.RestockInterval)
            continue;

        uint32 canSell = pool.RestockCount;
        if (pool.MaxCount > 0)
        {
            uint32 free = (pool.CurrentCount < pool.MaxCount) ? (pool.MaxCount - pool.CurrentCount) : 0;
            // 首次补货（尚未补过货）直接补满到 max_count；之后每次按 restock_count 递增
            if (pool.LastRestock == 0)
                canSell = free;
            else
                canSell = std::min(canSell, free);
        }

        if (canSell == 0)
        {
            pool.LastRestock = now;   // 已满，更新时间避免空转
            continue;
        }

        for (uint32 i = 0; i < canSell; ++i)
        {
            if (!SellOne(&bot, pool))
                break;
        }

        pool.LastRestock = now;
    }

    ObjectAccessor::RemoveObject(&bot);
}

// 成交/过期时更新热度（仅当物品属于“浮动开启”的池子才记录）
void AhSellerUpdateHeat(uint32 itemId, int32 delta)
{
    for (SellerPool const& pool : gSellerPools)
    {
        if (!pool.Enabled)
            continue;
        if (pool.PriceUpPct <= 0.0 && pool.PriceDownPct <= 0.0)
            continue;   // 浮动关闭的池子不记录

        if (pool.Contains(itemId))
        {
            gAhSellerHeat[itemId] += delta;
            return;
        }
    }
}

// 上下架时维护当前存量
void AhSellerOnAuctionAdd(uint32 itemId)
{
    for (SellerPool& pool : gSellerPools)
        if (pool.Enabled && pool.Contains(itemId))
            ++pool.CurrentCount;
}

void AhSellerOnAuctionRemove(uint32 itemId)
{
    for (SellerPool& pool : gSellerPools)
        if (pool.Enabled && pool.Contains(itemId) && pool.CurrentCount > 0)
            --pool.CurrentCount;
}
