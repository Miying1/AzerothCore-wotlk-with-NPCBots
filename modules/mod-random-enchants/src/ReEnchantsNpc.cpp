/*

Database Actions:

1 = item
2 = gold
3 = name change
4 = faction change
5 = race change

script made by talamortis

*/

#include "Configuration/Config.h"
#include "Player.h"
#include "Creature.h"
#include "AccountMgr.h"
#include "ScriptMgr.h"
#include "Define.h"
#include "GossipDef.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Chat.h"

struct ItemEnchants
{
    uint32    item_guid;
    int32     attr_group;
    int32     oid;
    int32     refm_count;
};
enum FMITEM
{
    TTFUWEN    =60100, //符文
    TTJINGHUA  =60101 //精华
};
std::unordered_map<int, ItemEnchants> ItemEnchantsStore;
float rolleh_chance;
float rolltx_chance;
class ReEnchants_WorldScript : public WorldScript
{
public:
    ReEnchants_WorldScript() : WorldScript("ReEnchants_WorldScript") { }
 
    void OnAfterConfigLoad(bool /*reload*/) override
    {
        rolleh_chance = sConfigMgr->GetOption<float>("RandomEnchants.rolleh_chance", 70.0f);;
        rolltx_chance = sConfigMgr->GetOption<float>("RandomEnchants.rolltx_chance", 70.0f);;
    }
};


class ReEnchantsNpc : public CreatureScript
{
public:
    ReEnchantsNpc() : CreatureScript("npc_re_enchants") {}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (player->IsInCombat())
            return false;
      
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我想重新附魔首饰。", GOSSIP_SENDER_MAIN, 1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我想重铸首饰的力量。", GOSSIP_SENDER_MAIN + 1, 2);
        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "我需要泰坦能量", GOSSIP_SENDER_MAIN+4, 1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "请介绍下泰坦附魔。", GOSSIP_SENDER_MAIN+5, 1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "离开...", GOSSIP_SENDER_MAIN + 2, 3);
        SendGossipMenuFor(player, textId, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32  sender, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
         
        std::set<uint32> itemList ;
        int result = 0;
        switch (sender)
        {
        case 3:
            CloseGossipMenuFor(player);
            break;
        case 4:
            OnGossipHello(player, creature);
            break;
        case 5:
            CloseGossipMenuFor(player);
            player->GetSession()->SendListInventory(creature->GetGUID());
            break;
        case 6:
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_MAIN + 3, 3);
            SendGossipMenuFor(player, textId + 6, creature->GetGUID());
            break;
        case 1://随机FM
            if (!VerifyReqItem(player, TTFUWEN)) {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_MAIN + 3, 3);
                SendGossipMenuFor(player, textId + 1, creature->GetGUID());
                break;
            }
            GetItemList(player, 100,&itemList);
            if (itemList.empty()) {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_MAIN + 3, 3);
                SendGossipMenuFor(player, textId + 3, creature->GetGUID());
                break;
            }
            SendGossipMenuFor(player, textId + 4, creature->GetGUID());
            break;
        case 2://精炼
            if (!VerifyReqItem(player, TTJINGHUA)) {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_MAIN + 3, 3);
                SendGossipMenuFor(player, textId + 2, creature->GetGUID());
                break;
            }
            GetItemList(player, 200, &itemList);
            if (itemList.empty()) {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_MAIN + 3, 3);
                SendGossipMenuFor(player, textId + 3, creature->GetGUID());
                break;
            } 
            SendGossipMenuFor(player, textId + 4, creature->GetGUID()); 
            break;

        case 100:
            if (!VerifyReqItem(player, TTFUWEN)) {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_MAIN + 3, 3);
                SendGossipMenuFor(player, textId + 2, creature->GetGUID());
                break;
            }
            result = RollPossibleEnchant(player, action);
            if (result == 0) {
                ChatHandler(player->GetSession()).PSendSysMessage("附魔失败！");
                CloseGossipMenuFor(player);
            }
            else
            {
                std::string msg = result == 1 ? "泰坦符文能量消散了！" : "泰坦符文能量汇聚于你的装备之上！";
                ChatHandler(player->GetSession()).PSendSysMessage(msg.c_str());
                player->DestroyItemCount(TTFUWEN, 1, true);
                return OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 1); 
            }
            break;
        case 200:
            if (!VerifyReqItem(player, TTJINGHUA)) {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_MAIN + 3, 3);
                SendGossipMenuFor(player, textId + 2, creature->GetGUID());
                break;
            }
            result = RollTXEnchant(player, action);
            if (result == 0) {
                ChatHandler(player->GetSession()).PSendSysMessage("附魔失败！");
                CloseGossipMenuFor(player);
            }
            else
            {
                std::string msg = result == 1 ? "泰坦精华能量消散了！" : "泰坦精华能量汇聚于你的装备之上！";
                ChatHandler(player->GetSession()).PSendSysMessage(msg.c_str());
                player->DestroyItemCount(TTJINGHUA, 1, true);
                return OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN+1, 1);
            }
            break;
        }
        return true;
    }
    int RollPossibleEnchant(Player* player, uint32 item_guid) {
         auto guid=ObjectGuid::Create<HighGuid::Item>(item_guid);
         Item* item = player->GetItemByGuid(guid);
        if (!item) return 0;
        if (rand_chance() > rolleh_chance) return 1;
        uint32 itemtype = item->GetTemplate()->InventoryType;
      
        std::string part = "";
        if (itemtype == 2) {//项链
            part = "(1,2)";
        }
        else
        {
            part = "(1)";
        }
        QueryResult qr = WorldDatabase.Query("SELECT enchantID,attr_group,oid FROM mod_item_enchantment_random WHERE  part in{} ORDER BY RAND() LIMIT 1", part);
        if (!qr) return 0;
        int slotRand = qr->Fetch()[0].Get<uint32>();
        int attr_group = qr->Fetch()[1].Get<uint32>();
        int oid = qr->Fetch()[2].Get<uint32>();
      
        if (sSpellItemEnchantmentStore.LookupEntry(slotRand)) { //Make sure enchantment id exists
            player->ApplyEnchantment(item, EnchantmentSlot(0), false);
            item->SetEnchantment(EnchantmentSlot(0), slotRand, 0, 0);
            player->ApplyEnchantment(item, EnchantmentSlot(0), true);
            SaveRandEnchantmentToDB(item->GetGUID().GetCounter(), oid, attr_group);
            auto its = ItemEnchantsStore.find(item_guid);
            if (its != ItemEnchantsStore.end()) {
                auto itemEh = its->second;
                ItemEnchantsStore[item_guid].attr_group = attr_group;
                ItemEnchantsStore[item_guid].oid = oid;
                ItemEnchantsStore[item_guid].refm_count = 0;
            }
            else
            {
                ItemEnchants newitemEh{};
                newitemEh.item_guid = item_guid;
                newitemEh.attr_group = attr_group;
                newitemEh.oid = oid;
                newitemEh.refm_count =0;
                ItemEnchantsStore[item_guid] = newitemEh;
            }
            return 2;
        }
        else
        {
            return 0;
        }

    }
    void SaveRandEnchantmentToDB(int itemguid, int oid, int attr_group) {
        WorldDatabase.Query("REPLACE into mod_item_enchantment_result(item_guid,attr_group,oid)values({},{},{});", itemguid, attr_group, oid);
    }
    int RollTXEnchant(Player* player, uint32 item_guid) {
        auto guid = ObjectGuid::Create<HighGuid::Item>(item_guid);
        Item* item = player->GetItemByGuid(guid);
        if (!item) return 0;
        if (rand_chance() > rolleh_chance) return 1;
        uint32 itemtype = item->GetTemplate()->InventoryType;
        auto its = ItemEnchantsStore.find(item_guid);
        if (its == ItemEnchantsStore.end()) return 0;
        auto itemEnchants = &its->second;
        
        QueryResult qr = WorldDatabase.Query("SELECT enchantID FROM mod_item_enchantment_random WHERE  attr_group={} ORDER BY RAND() LIMIT 1", itemEnchants->attr_group);
        if (!qr) return 0;
        int slotRand = qr->Fetch()[0].Get<uint32>(); 

        if (sSpellItemEnchantmentStore.LookupEntry(slotRand)) { //Make sure enchantment id exists
            player->ApplyEnchantment(item, EnchantmentSlot(0), false);
            item->SetEnchantment(EnchantmentSlot(0), slotRand, 0, 0);
            player->ApplyEnchantment(item, EnchantmentSlot(0), true);
            QueryResult qr = WorldDatabase.Query("update mod_item_enchantment_result set refm_count=refm_count+1 where item_guid={}", itemEnchants->item_guid);
            ItemEnchantsStore[item_guid].refm_count = itemEnchants->refm_count + 1;
            return 2;
        }
        else
        {
            return 0;
        }

    }

    void GetItemList(Player* player, int sender, std::set<uint32>* itemList) { 
        //s2.1.2: other bags
        for (uint8 i = INVENTORY_SLOT_BAG_START; i != INVENTORY_SLOT_BAG_END; ++i)
        {
            if (Bag const* pBag = player->GetBagByPos(i))
            {
                for (uint32 j = 0; j != pBag->GetBagSize(); ++j)
                {
                    if (Item const* pItem = player->GetItemByPos(i, j))
                    {
                        uint32 itemtype = pItem->GetTemplate()->InventoryType;
                        if (pItem->GetTemplate()->Class != 4
                            || pItem->GetTemplate()->Quality != 4
                            || pItem->GetTemplate()->ItemLevel < 200
                            || (itemtype != 12 && itemtype != 2))
                            continue;
                        int itemid = pItem->GetGUID().GetCounter();
                        if (!GetItemEnchants(itemid)) continue;

                        std::ostringstream name;
                        _AddItemLink(player, pItem, name, true);
                        name << " [" << pItem->GetTemplate()->ItemLevel << "]";
                        AddGossipItemFor(player, GOSSIP_ICON_CHAT, name.str().c_str(), sender, itemid);
                        itemList->insert(itemid);
                    }
                }
            }
        } 
    }
    bool VerifyReqItem(Player* player, int hasreqItem) {
        auto item = player->GetItemByEntry(hasreqItem);
        if (item) return true;
        return false;
        // other bags
        /*for (uint8 i = INVENTORY_SLOT_BAG_START; i != INVENTORY_SLOT_BAG_END; ++i)
        {
            if (Bag const* pBag = player->GetBagByPos(i))
            {
                for (uint32 j = 0; j != pBag->GetBagSize(); ++j)
                {
                    if (Item const* pItem = player->GetItemByPos(i, j))
                    {
                        uint32 itemtype = pItem->GetTemplate()->InventoryType;
                        if (pItem->GetEntry() == hasreqItem)
                            return true;
                    }
                }
            }
        }
        return false;*/
    }
    bool GetItemEnchants(int item_guid) {
        auto its = ItemEnchantsStore.find(item_guid);
        if (its != ItemEnchantsStore.end()) return true;

        QueryResult qr = WorldDatabase.Query("SELECT attr_group,oid,refm_count FROM mod_item_enchantment_result WHERE  item_guid={} ", item_guid);
        if (!qr) return false;
        ItemEnchants itemEnchants{};
        itemEnchants.item_guid = item_guid;
        itemEnchants.attr_group = qr->Fetch()[0].Get<uint32>();
        itemEnchants.oid = qr->Fetch()[1].Get<uint32>();
        itemEnchants.refm_count = qr->Fetch()[2].Get<uint32>();
        ItemEnchantsStore[item_guid] = itemEnchants;
        return true;
    }
    void _AddItemLink(Player const* forPlayer, Item const* item, std::ostringstream& str, bool addIcon)
    {
        ItemTemplate const* proto = item->GetTemplate();
        //ItemRandomSuffixEntry const* item_rand = sItemRandomSuffixStore.LookupEntry(abs(item->GetItemRandomPropertyId()));
        uint32 g1 = 0, g2 = 0, g3 = 0;
        //uint32 bpoints = 0;
        std::string name = proto->Name1;
        std::string suffix = "";

        //icon
        if (addIcon)
        {
            ItemDisplayInfoEntry const* itemDisplayEntry = sItemDisplayInfoStore.LookupEntry(item->GetTemplate()->DisplayInfoID);
            if (itemDisplayEntry)
                str << "|TInterface\\Icons\\" << itemDisplayEntry->inventoryIcon << ":16|t";
        }

        //color
        str << "|c";
        switch (proto->Quality)
        {
        case ITEM_QUALITY_POOR:     str << "ff9d9d9d"; break;  //GREY
        case ITEM_QUALITY_NORMAL:   str << "ffffffff"; break;  //WHITE
        case ITEM_QUALITY_UNCOMMON: str << "ff1eff00"; break;  //GREEN
        case ITEM_QUALITY_RARE:     str << "ff0070dd"; break;  //BLUE
        case ITEM_QUALITY_EPIC:     str << "ffa335ee"; break;  //PURPLE
        case ITEM_QUALITY_LEGENDARY:str << "ffff8000"; break;  //ORANGE
        case ITEM_QUALITY_ARTIFACT: str << "ffe6cc80"; break;  //LIGHT YELLOW
        case ITEM_QUALITY_HEIRLOOM: str << "ffe6cc80"; break;  //LIGHT YELLOW
        default:                    str << "ff000000"; break;  //UNK BLACK
        }
        str << "|Hitem:" << proto->ItemId << ':';

        //permanent enchantment
        str << item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT) << ':';
        //gems 3
        for (uint32 slot = SOCK_ENCHANTMENT_SLOT; slot != SOCK_ENCHANTMENT_SLOT + MAX_ITEM_PROTO_SOCKETS; ++slot)
        {
            uint32 eId = item->GetEnchantmentId(EnchantmentSlot(slot));

            switch (slot - SOCK_ENCHANTMENT_SLOT)
            {
            case 0: g1 = eId;   break;
            case 1: g2 = eId;   break;
            case 2: g3 = eId;   break;
            }
        }
        str << g1 << ':' << g2 << ':' << g3 << ':';
        //always zero
        str << 0 << ':';
        //random property
        str << item->GetItemRandomPropertyId() << ':';
        str << item->GetItemSuffixFactor() << ':';

         

        //name
        _LocalizeItem(forPlayer, name, item->GetEntry());

        str << "|h[" << name;
        if (suffix.length() > 0)
            str << ' ' << suffix;
        str << "]|h|r";

        //quantity
        if (item->GetCount() > 1)
            str << "x" << item->GetCount() << ' ';

        //TC_LOG_ERROR("entities.player", "bot_ai::_AddItemLink(): %s", str.str().c_str());
    }
    //Localization
    void _LocalizeItem(Player const* forPlayer, std::string& itemName, uint32 entry) 
    {
        uint32 loc = forPlayer->GetSession()->GetSessionDbLocaleIndex();
        std::wstring wnamepart;

        ItemLocale const* itemInfo = sObjectMgr->GetItemLocale(entry);
        if (!itemInfo)
            return;

        if (itemInfo->Name.size() > loc && !itemInfo->Name[loc].empty())
        {
            const std::string name = itemInfo->Name[loc];
            if (Utf8FitTo(name, wnamepart))
                itemName = name;
        }
    }

private:
    int textId = 60001;
};

void AddReEnchantsNpcScripts()
{
    new ReEnchants_WorldScript();
    new ReEnchantsNpc();
}
