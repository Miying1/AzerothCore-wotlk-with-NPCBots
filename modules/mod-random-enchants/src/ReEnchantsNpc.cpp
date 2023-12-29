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
#include "TnEchants.h"

 
enum FMITEM
{
    TTFUWEN = 60100, //符文
    TTJINGHUA = 60101 //精华
};

class ReEnchants_WorldScript : public WorldScript
{
public:
    ReEnchants_WorldScript() : WorldScript("ReEnchants_WorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        tnEchants->InitConfig();
    }
    void OnStartup() override {
        tnEchants->InitData();
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
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我想强化首饰的附魔力量。", GOSSIP_SENDER_MAIN + 1, 2);
        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "我需要泰坦能量", GOSSIP_SENDER_MAIN + 4, 1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "请介绍下泰坦附魔。", GOSSIP_SENDER_MAIN + 5, 1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "离开...", GOSSIP_SENDER_MAIN + 2, 3);
        SendGossipMenuFor(player, textId, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32  sender, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();

        std::set<uint32> itemList;
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
            GetItemList(player, 100, &itemList);
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
            SendGossipMenuFor(player, textId + 5, creature->GetGUID());
            break;

        case 100:
            if (!VerifyReqItem(player, TTFUWEN)) {
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_MAIN + 3, 3);
                SendGossipMenuFor(player, textId + 2, creature->GetGUID());
                break;
            }
            result = RollPossibleEnchant(player, action);
            if (result == 0) {
                ChatHandler(player->GetSession()).PSendSysMessage("附魔失败:%u", action);
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
            result = UpdateEnchant(player, action);
            if (result == 0) {
                ChatHandler(player->GetSession()).PSendSysMessage("附魔失败:%u",action);
                CloseGossipMenuFor(player);
            }
            else
            {
                std::string msg = result == 1 ? "泰坦精华能量消散了,但下次成功的机会更多了！" : result == 2 ? "泰坦精华能量汇聚于你的装备之上！":"当前装备的附魔等级已达到最大值.";
                ChatHandler(player->GetSession()).PSendSysMessage(msg.c_str());
                if(result!=3)
                    player->DestroyItemCount(TTJINGHUA, 1, true);
                return OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN + 1, 1);
            }
            break;
        }
        return true;
    }
    int RollPossibleEnchant(Player* player, uint32 item_guid) {
        auto guid = ObjectGuid::Create<HighGuid::Item>(item_guid);
        Item* item = player->GetItemByGuid(guid);
        if (!item) return 0;
        if (rand_chance() > tnEchants->ReRandChance) return 1;
        bool res = tnEchants->RandomEnchEffect(player, item);
        return res ? 2 : 1;
    }
    //升级FM值
    int UpdateEnchant(Player* player, uint32 item_guid) {
        auto guid = ObjectGuid::Create<HighGuid::Item>(item_guid);
        Item* item = player->GetItemByGuid(guid);
        if (!item) return 0;
        int res_flag= tnEchants->UpEnchLevel(player, item);
        return res_flag;
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
                        if (!tnEchants->VerifyItemIsTn(itemid)) continue;

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
