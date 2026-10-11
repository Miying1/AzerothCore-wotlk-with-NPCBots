/*
 * 随机属性萃取与转移 —— mod-random-enchants 模块
 *
 * 说明：本功能与模块的「泰坦附魔」无关，是独立的随机属性转移功能。
 *   直接读写装备自带的随机属性（ITEM_FIELD_RANDOM_PROPERTIES_ID，即
 *   RandomProperty / RandomSuffix），不依赖 tnEchants / mod_item_enchantment_result。
 *
 * 功能：
 *   1. 属性萃取仪（60103，非消耗）：右键使用，选择一件「有随机属性」的装备，
 *      将其随机属性萃取为「属性卷轴」，并销毁原装备。
 *   2. 属性卷轴（60104，消耗）：右键使用，选择一件「无随机属性」的装备，
 *      将卷轴上的随机属性注入目标装备，并消耗卷轴。
 *
 * 菜单：
 *   - 装备名按品质着色（ItemQualityColors），且菜单项不显示图标；
 *   - 分页显示，每页 14 条，超出时给出「上一页 / 下一页」（页码带在按钮与 action 里）；
 *   - 选中装备时客户端弹出确认框，玩家确认后才会真正执行萃取 / 注入。
 *
 * 装等过滤（优先按装等）：萃取仪与属性卷轴共用同一门槛，只有
 *   ItemLevel > RandomEnchants.Custom.MinItemLevel（默认 220）的装备才会出现在菜单里，
 *   阈值与随机附魔（item_random_enchant.cpp）口径一致 —— 萃取仪只能取出本模块随机附魔
 *   覆盖到的那批装备，低装等的原生随机属性装备既不参与萃取、也不能作为注入目标。
 *
 * 可操作的装备部位：与随机附魔（item_random_enchant.cpp）一致，覆盖全部装备槽
 *   （头/颈/肩/背/胸/袍/腰/腿/脚/腕/手/戒指/饰品/主手/副手/双手/远程/投掷/盾/
 *   副手物品/圣物），见 RandomEnchantCommon.h 的 IsEquipmentSlot()。
 *
 * 可选范围：玩家主背包（16 格）与已装备的包内物品，不含已装备的装备。
 *
 * 持久化：卷轴的 randomPropertyId 由 item_instance 表保存，可跨登录长久保留。
 */

#include "Chat.h"
#include "Common.h"
#include "Configuration/Config.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "RandomEnchantCommon.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "SharedDefines.h"
#include "StringFormat.h"

#include <algorithm>
#include <string>
#include <vector>

// GOSSIP_SENDER_MAIN 由 ScriptedGossip.h 提供（= 1），此处不再重复定义
enum
{
    ITEM_EXTRACTOR = 60103,   // 属性萃取仪（非消耗）
    ITEM_SCROLL    = 60104,   // 属性卷轴（消耗，携带随机属性）

    ITEMS_PER_PAGE = 14,      // 每页菜单条数

    // 菜单项 sender：装备项与翻页项分开，避免翻页动作与装备 GUID 撞车
    SENDER_ITEM_LIST = GOSSIP_SENDER_MAIN + 100,  // 装备项：action = 装备 guid 低 32 位
    SENDER_PAGE_NAV  = GOSSIP_SENDER_MAIN + 200,  // 翻页项：action = 目标页码（从 1 起）

    // 客户端对超出范围的图标编号不绘制图标，用它实现「菜单项无图标」
    GOSSIP_ICON_NONE = 255,
};

class EnchantTransferItem : public ItemScript
{
public:
    EnchantTransferItem() : ItemScript("EnchantTransferItem") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        uint32 entry = item->GetEntry();
        if (entry != ITEM_EXTRACTOR && entry != ITEM_SCROLL)
            return false;

        // 萃取仪 → 列出有随机属性的装备；卷轴 → 列出无随机属性的装备
        ListItems(player, item, entry == ITEM_EXTRACTOR, 1);
        return true;
    }

    void OnGossipSelect(Player* player, Item* item, uint32 sender, uint32 action) override
    {
        uint32 entry = item->GetEntry();
        if (entry != ITEM_EXTRACTOR && entry != ITEM_SCROLL)
            return;

        // 翻页：action 里存的是目标页码
        if (sender == SENDER_PAGE_NAV)
        {
            ListItems(player, item, entry == ITEM_EXTRACTOR, action);
            return;
        }

        // 选择装备：确认框已由客户端弹出，玩家确认后才会走到这里
        if (sender == SENDER_ITEM_LIST)
        {
            // action 存的是目标装备的 guid 低 32 位
            Item* target = player->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(action));
            if (!target)
            {
                CloseGossipMenuFor(player);
                return;
            }

            if (entry == ITEM_EXTRACTOR)
                DoExtract(player, target);
            else
                DoInject(player, item, target);
            return;
        }

        CloseGossipMenuFor(player);
    }

private:
    // 一条菜单项：装备 guid 低 32 位（action）+ 已按品质着色的装备名
    struct MenuEntry
    {
        uint32      guid;
        std::string name;
    };

    // 萃取仪与属性卷轴共用的装等门槛：与随机附魔（RandomEnchants.Custom.MinItemLevel）同一口径，
    // 保证「能被萃取」与「能被注入」的装备就是「本模块随机附魔覆盖」的那批装备
    static uint32 GetMinItemLevel()
    {
        return sConfigMgr->GetOption<uint32>("RandomEnchants.Custom.MinItemLevel", 220);
    }

    void DoExtract(Player* player, Item* target)
    {
        // 优先按装等过滤：菜单已过滤，这里再校验一次，防止伪造 action 绕过
        if (target->GetTemplate()->ItemLevel <= GetMinItemLevel())
        {
            CloseGossipMenuFor(player);
            ChatHandler(player->GetSession()).PSendSysMessage("该装备装等不高于 {}，无法萃取随机属性。", GetMinItemLevel());
            return;
        }

        int32 rp = target->GetItemRandomPropertyId();
        if (!rp)
        {
            CloseGossipMenuFor(player);
            return;
        }

        ItemPosCountVec dest;
        if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, ITEM_SCROLL, 1) != EQUIP_ERR_OK)
        {
            CloseGossipMenuFor(player);
            ChatHandler(player->GetSession()).PSendSysMessage("背包已满，无法生成卷轴。");
            return;
        }

        // 销毁源装备，生成携带随机属性的卷轴（randomPropertyId 会持久化入库）
        player->DestroyItem(target->GetBagSlot(), target->GetSlot(), true);
        player->StoreNewItem(dest, ITEM_SCROLL, true, rp);

        CloseGossipMenuFor(player);
        ChatHandler(player->GetSession()).PSendSysMessage("属性词条已提取为卷轴，原装备被销毁。");
    }

    void DoInject(Player* player, Item* scroll, Item* target)
    {
        // 与萃取仪同一装等门槛：菜单已过滤，这里再校验一次，防止伪造 action 绕过
        if (target->GetTemplate()->ItemLevel <= GetMinItemLevel())
        {
            CloseGossipMenuFor(player);
            ChatHandler(player->GetSession()).PSendSysMessage("该装备装等不高于 {}，无法注入随机属性。", GetMinItemLevel());
            return;
        }

        int32 rp = scroll->GetItemRandomPropertyId();
        if (!rp)
        {
            CloseGossipMenuFor(player);
            return;
        }

        // 把卷轴随机属性写入目标装备（SetItemRandomProperties 内部会写附魔槽并标记变更）
        target->SetItemRandomProperties(rp);

        // 消耗卷轴
        player->DestroyItem(scroll->GetBagSlot(), scroll->GetSlot(), true);

        CloseGossipMenuFor(player);
        ChatHandler(player->GetSession()).PSendSysMessage("属性词条已注入目标装备。");
    }

    // 按物品品质着色（|cAARRGGBB…|r），用于菜单显示
    static std::string ColoredItemName(ItemTemplate const* proto)
    {
        uint32 quality = proto->Quality < MAX_ITEM_QUALITY ? proto->Quality : uint32(ITEM_QUALITY_POOR);
        return Acore::StringFormat("|c{:08x}{}|r", ItemQualityColors[quality], proto->Name1);
    }

    // withRandomProp=true 收集「有随机属性」的装备（萃取仪），false 收集「无随机属性」的装备（卷轴）；
    // 两条路径都先按装等过滤
    static void CollectItems(Player* player, bool withRandomProp, std::vector<MenuEntry>& out)
    {
        // 萃取仪与卷轴共用装等门槛，只读一次配置
        uint32 const minItemLevel = GetMinItemLevel();

        auto addItem = [&](Item* pItem)
        {
            if (!pItem)
                return;

            ItemTemplate const* proto = pItem->GetTemplate();

            // 优先按装等过滤：装等不高于阈值的装备既不参与萃取、也不能作为注入目标
            if (proto->ItemLevel <= minItemLevel)
                return;

            // 装备部位判定与随机附魔（item_random_enchant.cpp）保持一致
            if (!IsEquipmentSlot(proto->InventoryType))
                return;

            bool has = pItem->GetItemRandomPropertyId() != 0;
            if (has != withRandomProp)
                return;

            out.push_back({ pItem->GetGUID().GetCounter(), ColoredItemName(proto) });
        };

        // 扫描玩家主背包（16 格）
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot != INVENTORY_SLOT_ITEM_END; ++slot)
            addItem(player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));

        // 扫描已装备的包内物品
        for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot != INVENTORY_SLOT_BAG_END; ++bagSlot)
            if (Bag* bag = player->GetBagByPos(bagSlot))
                for (uint32 slot = 0; slot != bag->GetBagSize(); ++slot)
                    addItem(player->GetItemByPos(bagSlot, slot));
    }

    // 选中装备时客户端弹出的确认框文本
    static std::string ConfirmText(uint32 sourceEntry, std::string const& itemName)
    {
        if (sourceEntry == ITEM_EXTRACTOR)
            return Acore::StringFormat("确定萃取 {} 的随机属性吗？原装备将被销毁。", itemName);

        return Acore::StringFormat("确定把卷轴上的随机属性注入 {} 吗？卷轴将被消耗。", itemName);
    }

    // 列出可操作的装备（分页，每页 ITEMS_PER_PAGE 条）
    void ListItems(Player* player, Item* sourceItem, bool withRandomProp, uint32 page) const
    {
        player->PlayerTalkClass->ClearMenus();

        std::vector<MenuEntry> entries;
        CollectItems(player, withRandomProp, entries);

        uint32 const total = uint32(entries.size());
        uint32 pages = (total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
        if (!pages)
            pages = 1;

        // 页码越界时夹到有效范围（客户端回传的 action 不可信）
        page = std::min(std::max(page, uint32(1)), pages);

        if (!total)
        {
            AddGossipItemFor(player, GOSSIP_ICON_NONE,
                             withRandomProp
                                 ? Acore::StringFormat("（背包里没有装等高于 {} 且带随机属性的装备）", GetMinItemLevel())
                                 : Acore::StringFormat("（背包里没有装等高于 {} 的可注入装备）", GetMinItemLevel()),
                             SENDER_PAGE_NAV, page);
        }
        else
        {
            uint32 const first = (page - 1) * ITEMS_PER_PAGE;
            uint32 const last = std::min(first + ITEMS_PER_PAGE, total);

            for (uint32 i = first; i < last; ++i)
                AddGossipItemFor(player, GOSSIP_ICON_NONE, entries[i].name, SENDER_ITEM_LIST, entries[i].guid,
                                 ConfirmText(sourceItem->GetEntry(), entries[i].name), 0, false);
        }

        // 翻页（页码同时写在按钮文字与 action 里）
        if (page > 1)
            AddGossipItemFor(player, GOSSIP_ICON_NONE, Acore::StringFormat("上一页（{}/{}）", page - 1, pages),
                             SENDER_PAGE_NAV, page - 1);
        if (page < pages)
            AddGossipItemFor(player, GOSSIP_ICON_NONE, Acore::StringFormat("下一页（{}/{}）", page + 1, pages),
                             SENDER_PAGE_NAV, page + 1);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, sourceItem->GetGUID());
    }
};

void AddEnchantTransferScripts()
{
    new EnchantTransferItem();
}
