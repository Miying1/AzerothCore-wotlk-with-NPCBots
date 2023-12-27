/*
* Converted from the original LUA script to a module for Azerothcore(Sunwell) :D
*/
#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "Chat.h"


struct RandomEnchant
{
    uint32    EnchantID;
    uint32    AttrGroup;
    uint32    part;
    uint32    oid; 
};

class RandomEnchantsPlayer : public PlayerScript {
public:
    RandomEnchantsPlayer() : PlayerScript("RandomEnchantsPlayer") { }
     
    void OnLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnLoot", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
            RollPossibleEnchant(player, item);
    }

    void OnCreateItem(Player* player, Item* item, uint32 /*count*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnCreate", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
            RollPossibleEnchant(player, item);
    }
     

    void OnGroupRollRewardItem(Player* player, Item* item, uint32 /*count*/, RollVote /*voteType*/, Roll* /*roll*/) override {
        if (sConfigMgr->GetOption<bool>("RandomEnchants.OnGroupRoll", true) && sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true))
            RollPossibleEnchant(player, item);
    }

    void RollPossibleEnchant(Player* player, Item* item) {
        // Check global enable option
        if (!sConfigMgr->GetOption<bool>("RandomEnchants.Enable", true)) {
            return;
        }
        if (!item) return; 
        float enchantChance1 = sConfigMgr->GetOption<float>("RandomEnchants.EnchantChance1", 30.0f);
        if (rand_chance() > enchantChance1) return;

        uint32 Quality = item->GetTemplate()->Quality;
        uint32 Class = item->GetTemplate()->Class;
        uint32 itemtype = item->GetTemplate()->InventoryType;
        //LOG_ERROR("sql.sql", "{} (Quality: {} Class:{} InventoryType:{})", item->GetTemplate()->Name1.c_str(), Quality, Class, itemtype);
        if (
            (Quality != 4) /* eliminates enchanting anything that isn't a recognized quality */ 
            || (Class != 4) /* eliminates enchanting anything but weapons/armor */
            || item->GetTemplate()->ItemLevel<200
            || (itemtype !=12 && itemtype !=2)
            ) {
            return;
        } 
        std::string part = "";
        if (itemtype == 2) {//项链
            part = "(1,2)";
        }
        else
        {
            part = "(1)";
        }
        QueryResult qr = WorldDatabase.Query("SELECT enchantID,attr_group,oid FROM mod_item_enchantment_random WHERE  part in{} ORDER BY RAND() LIMIT 1", part);
        if (!qr) return;
        int slotRand= qr->Fetch()[0].Get<uint32>();
        int attr_group = qr->Fetch()[1].Get<uint32>();
        int oid = qr->Fetch()[2].Get<uint32>(); 
       
        if (sSpellItemEnchantmentStore.LookupEntry(slotRand)) { //Make sure enchantment id exists
            player->ApplyEnchantment(item, EnchantmentSlot(0), false);
            item->SetEnchantment(EnchantmentSlot(0), slotRand, 0, 0);
            player->ApplyEnchantment(item, EnchantmentSlot(0), true);
            SaveRandEnchantmentToDB(item->GetGUID().GetCounter(), oid, attr_group);
         
        }
        else
        {
            return;
        }
 
    }
    void SaveRandEnchantmentToDB(int itemguid,int oid,int attr_group) {
         WorldDatabase.Query("REPLACE into mod_item_enchantment_result(item_guid,attr_group,oid)values({},{},{});", itemguid, attr_group, oid );
    }
   
};

void AddRandomEnchantsScripts() {
    new RandomEnchantsPlayer();
}


