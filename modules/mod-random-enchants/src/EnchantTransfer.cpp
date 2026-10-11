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
 * 可操作的装备部位：武器 / 护甲（头、颈、肩、胸、腕、手、腰、腿、脚、戒指、披风、
 *   盾牌、副手等），但不含饰品。
 *
 * 持久化：卷轴的 randomPropertyId 由 item_instance 表保存，可跨登录长久保留。
 */

#include "Chat.h"
#include "Common.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"

enum
{
    ITEM_EXTRACTOR     = 60103,   // 属性萃取仪（非消耗）
    ITEM_SCROLL        = 60104,   // 属性卷轴（消耗，携带随机属性）
    GOSSIP_SENDER_MAIN = 1,
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

        player->PlayerTalkClass->ClearMenus();
        // 萃取仪 → 列出有随机属性的装备；卷轴 → 列出无随机属性的装备
        ListItems(player, entry == ITEM_EXTRACTOR);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, item->GetGUID());
        return true;
    }

    void OnGossipSelect(Player* player, Item* item, uint32 /*sender*/, uint32 action) override
    {
        uint32 entry = item->GetEntry();
        if (entry != ITEM_EXTRACTOR && entry != ITEM_SCROLL)
            return;

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
    }

private:
    void DoExtract(Player* player, Item* target)
    {
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
        ChatHandler(player->GetSession()).PSendSysMessage("随机属性已提取为卷轴，原装备被销毁。");
    }

    void DoInject(Player* player, Item* scroll, Item* target)
    {
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
        ChatHandler(player->GetSession()).PSendSysMessage("卷轴随机属性已注入目标装备。");
    }

    // withRandomProp=true 列出「有随机属性」的装备，false 列出「无随机属性」的装备
    void ListItems(Player* player, bool withRandomProp)
    {
        uint8 itemCount = 0;
        auto addItem = [&](Item* pItem)
        {
            if (!pItem || itemCount >= GOSSIP_MAX_MENU_ITEMS)
                return;

            ItemTemplate const* proto = pItem->GetTemplate();
            // 只处理武器/护甲类装备，且排除饰品
            if ((proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR)
                || proto->InventoryType == INVTYPE_TRINKET)
                return;

            bool has = pItem->GetItemRandomPropertyId() != 0;
            if (has != withRandomProp)
                return;

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, proto->Name1, GOSSIP_SENDER_MAIN, pItem->GetGUID().GetCounter());
            ++itemCount;
        };

        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot != INVENTORY_SLOT_ITEM_END && itemCount < GOSSIP_MAX_MENU_ITEMS; ++slot)
            addItem(player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));

        for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot != INVENTORY_SLOT_BAG_END && itemCount < GOSSIP_MAX_MENU_ITEMS; ++bagSlot)
            if (Bag* bag = player->GetBagByPos(bagSlot))
                for (uint32 slot = 0; slot != bag->GetBagSize() && itemCount < GOSSIP_MAX_MENU_ITEMS; ++slot)
                    addItem(player->GetItemByPos(bagSlot, slot));
    }
};

void AddEnchantTransferScripts()
{
    new EnchantTransferItem();
}
