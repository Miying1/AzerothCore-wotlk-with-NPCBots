/*
* Converted from the original LUA script to a module for Azerothcore(Sunwell) :D
*/
#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "Chat.h"
#include "TnEchants.h"

 

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
        float enchantChance1 = sConfigMgr->GetOption<float>("RandomEnchants.LootEnchantChance1", 30.0f);
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
        tnEchants->RandomEnchEffect(player, item);  
    }
     
};

void AddRandomEnchantsScripts() {
    new RandomEnchantsPlayer();
}


