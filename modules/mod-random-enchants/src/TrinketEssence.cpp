/*
 * 饰品特效提取器 —— mod-random-enchants 模块
 *
 * 说明：本功能与模块的「泰坦附魔」无关，是独立的「饰品特效转附魔卷轴」功能的运行时部分，
 *   与 EnchantTransfer.cpp（属性萃取/转移）同一套路：右键使用道具 → 列出可选目标 →
 *   玩家在 gossip 菜单里选中 → 执行转换。所有 ID 与对应关系全部来自数据库，代码不写死清单。
 *
 * 功能：
 *   饰品特效提取器（60105，消耗品）：右键使用，列出背包中「带可提取特效」的饰品装备；
 *   选中一件饰品后，依次读它的 5 个法术槽（item_template.spellid_1..5），
 *   用数据库对照字典（mod_trinket_essence_map）判定能否提取，**只取第一个命中的特效**，
 *   生成对应的附魔卷轴（只生成一张），并销毁原饰品与提取器本身。
 *
 * 判定与过滤：两段过滤，先装等后查表 ——
 *   1) 装等：ItemLevel < MIN_TRINKET_ITEM_LEVEL（200）的饰品直接排除，与生成字典时的
 *      MIN_ITEM_LEVEL 口径一致；低于阈值就不必再逐个扫法术槽。
 *   2) 查表：法术 ID 在 mod_trinket_essence_map 里 = 该饰品可提取，
 *      不在字典里 = 不可提取（列表里不显示）。字典本身在生成时已按
 *      「spelltrigger=1 且 ProcTypeMask>0」或「spelltrigger=2」筛过，运行时不再重复判断触发类型。
 *
 * 数据来源（由 outputs/饰品武器特效转附魔/generate.py 生成）：
 *   mod_trinket_essence_map   特效法术 ID -> 附魔卷轴 entry / 显示名（运行时只加载这三列：
 *                             spell_id、scroll_item、name；其余列仅供人工核对）
 *   spellitemenchantment_dbc  附魔定义（Effect=3 EQUIP_SPELL，EffectArg=原特效法术）
 *   spell_dbc                 附魔法术（Effect=53 ENCHANT_ITEM，EffectMiscValue=附魔 ID）
 *   item_template             附魔卷轴（class=0/subclass=8，spellid_1=附魔法术）
 *
 * 提取范围：ItemLevel >= 200 的饰品（InventoryType=12），且该特效法术在映射表中存在
 *   （装等与查表两段过滤，见上）。
 * 可操作范围：玩家主背包（16 格）与已装备的包内物品，不含已装备的饰品。
 */

#include "Chat.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "SharedDefines.h"

#include <sstream>
#include <string>
#include <unordered_map>

// GOSSIP_SENDER_MAIN 由 ScriptedGossip.h 提供（= 1），此处不再重复定义
enum
{
    ITEM_TRINKET_ESSENCE = 60105,   // 饰品特效提取器
};

// 是否消耗提取器本身（true = 提取成功后消失）
static bool const CONSUME_ESSENCE = true;

// 可提取的饰品最低装等（与 outputs/饰品武器特效转附魔/generate.py 的 MIN_ITEM_LEVEL 保持一致）
static uint32 const MIN_TRINKET_ITEM_LEVEL = 200;

namespace
{
    // 一条「特效法术 -> 附魔卷轴」的对应关系（只保留运行时真正用到的字段）
    struct EssenceScroll
    {
        uint32      scrollItem = 0;        // item_template.entry（附魔卷轴）
        std::string name;                  // 显示名
    };

    // key = 饰品上的原特效法术 ID
    std::unordered_map<uint32, EssenceScroll> g_EssenceScrolls;

    void LoadEssenceScrolls()
    {
        g_EssenceScrolls.clear();

        QueryResult result = WorldDatabase.Query(
            "SELECT spell_id, scroll_item, name FROM mod_trinket_essence_map");
        if (!result)
        {
            LOG_WARN("module", "饰品特效提取器：`mod_trinket_essence_map` 为空，特效提取功能不可用。");
            return;
        }

        do
        {
            Field* fields = result->Fetch();

            uint32 spellId = fields[0].Get<uint32>();

            EssenceScroll entry;
            entry.scrollItem = fields[1].Get<uint32>();
            entry.name       = fields[2].Get<std::string>();

            g_EssenceScrolls[spellId] = entry;
        } while (result->NextRow());

        LOG_INFO("module", "饰品特效提取器：已加载 {} 条「特效法术 -> 附魔卷轴」映射。", g_EssenceScrolls.size());
    }
}

// 世界启动时把映射表读进内存（映射表随生成的 SQL 一起入库）
class TrinketEssenceWorldScript : public WorldScript
{
public:
    TrinketEssenceWorldScript() : WorldScript("TrinketEssenceWorldScript") { }

    void OnStartup() override
    {
        LoadEssenceScrolls();
    }
};

class TrinketEssenceItem : public ItemScript
{
public:
    TrinketEssenceItem() : ItemScript("TrinketEssenceItem") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (item->GetEntry() != ITEM_TRINKET_ESSENCE)
            return false;

        player->PlayerTalkClass->ClearMenus();
        ListTrinkets(player);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, item->GetGUID());
        return true;
    }

    void OnGossipSelect(Player* player, Item* item, uint32 /*sender*/, uint32 action) override
    {
        if (item->GetEntry() != ITEM_TRINKET_ESSENCE)
            return;

        // action 存的是目标饰品的 guid 低 32 位
        Item* trinket = player->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(action));
        if (!trinket)
        {
            CloseGossipMenuFor(player);
            return;
        }

        Extract(player, item, trinket);
    }

private:
    // 用数据库对照字典判定该饰品能否提取，并取出第一个命中的特效（一件饰品只提取一个）
    bool FindScroll(ItemTemplate const* proto, EssenceScroll& out) const
    {
        // 优先按装等过滤：低于阈值的饰品即使共用了同一特效法术也不提取，顺带省掉逐槽查表
        if (proto->ItemLevel < MIN_TRINKET_ITEM_LEVEL)
            return false;

        for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            uint32 spellId = proto->Spells[i].SpellId;
            if (!spellId)
                continue;

            // 只在字典里的法术 ID 才算「可提取」，其余（含纯属性光环、使用类法术）一律跳过
            auto itr = g_EssenceScrolls.find(spellId);
            if (itr == g_EssenceScrolls.end())
                continue;

            out = itr->second;
            return true;
        }
        return false;
    }

    // 按物品品阶上色（|cAARRGGBB…|r），用于 gossip 菜单显示
    std::string ColoredItemName(ItemTemplate const* proto) const
    {
        uint32 quality = proto->Quality < MAX_ITEM_QUALITY ? proto->Quality : uint32(ITEM_QUALITY_POOR);

        std::ostringstream out;
        out << "|c" << std::hex << ItemQualityColors[quality] << std::dec << proto->Name1 << "|r";
        return out.str();
    }

    // 列出背包中「按对照字典可提取特效」的饰品装备
    void ListTrinkets(Player* player) const
    {
        uint8 itemCount = 0;
        auto addItem = [&](Item* pItem)
        {
            if (!pItem || itemCount >= GOSSIP_MAX_MENU_ITEMS)
                return;

            ItemTemplate const* proto = pItem->GetTemplate();
            if (proto->InventoryType != INVTYPE_TRINKET)
                return;

            EssenceScroll scroll;
            if (!FindScroll(proto, scroll))
                return;

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, ColoredItemName(proto), GOSSIP_SENDER_MAIN,
                             pItem->GetGUID().GetCounter());
            ++itemCount;
        };

        // 扫描玩家主背包（16 格）
        for (uint8 slot = INVENTORY_SLOT_ITEM_START;
             slot != INVENTORY_SLOT_ITEM_END && itemCount < GOSSIP_MAX_MENU_ITEMS; ++slot)
            addItem(player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));

        // 扫描已装备的包内物品
        for (uint8 bagSlot = INVENTORY_SLOT_BAG_START;
             bagSlot != INVENTORY_SLOT_BAG_END && itemCount < GOSSIP_MAX_MENU_ITEMS; ++bagSlot)
            if (Bag* bag = player->GetBagByPos(bagSlot))
                for (uint32 slot = 0; slot != bag->GetBagSize() && itemCount < GOSSIP_MAX_MENU_ITEMS; ++slot)
                    addItem(player->GetItemByPos(bagSlot, slot));
    }

    void Extract(Player* player, Item* essence, Item* trinket)
    {
        ChatHandler handler(player->GetSession());

        ItemTemplate const* proto = trinket->GetTemplate();
        if (proto->InventoryType != INVTYPE_TRINKET)
        {
            CloseGossipMenuFor(player);
            return;
        }

        EssenceScroll scroll;
        if (!FindScroll(proto, scroll))
        {
            CloseGossipMenuFor(player);
            handler.PSendSysMessage("该饰品上没有可提取的特效。");
            return;
        }

        // 卷轴要在销毁饰品/提取器之前先占好位置，所以按当前剩余空格校验
        if (player->GetFreeInventorySpace() < 1)
        {
            CloseGossipMenuFor(player);
            handler.PSendSysMessage("背包空间不足，至少需要 1 个空格。");
            return;
        }

        // 先确认卷轴能放下，再销毁饰品/提取器，避免出现「道具没了、卷轴没生成」
        ItemPosCountVec dest;
        InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, scroll.scrollItem, 1);
        if (msg != EQUIP_ERR_OK)
        {
            player->SendEquipError(msg, nullptr, nullptr, scroll.scrollItem);
            CloseGossipMenuFor(player);
            return;
        }

        // 销毁原饰品
        player->DestroyItem(trinket->GetBagSlot(), trinket->GetSlot(), true);

        // 生成对应的附魔卷轴（一件饰品只生成一张）
        player->StoreNewItem(dest, scroll.scrollItem, true);
        handler.PSendSysMessage("已提取特效：{}。", scroll.name);

        // 消耗提取器（在卷轴已成功生成之后）
        if (CONSUME_ESSENCE)
            player->DestroyItem(essence->GetBagSlot(), essence->GetSlot(), true);

        CloseGossipMenuFor(player);
        handler.PSendSysMessage("原饰品{}已被销毁，并生成 1 张附魔卷轴。",
                                CONSUME_ESSENCE ? "与提取器" : "");
    }
};

void AddTrinketEssenceScripts()
{
    new TrinketEssenceWorldScript();
    new TrinketEssenceItem();
}
