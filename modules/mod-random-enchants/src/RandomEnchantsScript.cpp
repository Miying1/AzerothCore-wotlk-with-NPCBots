/*
* Converted from the original LUA script to a module for Azerothcore(Sunwell) :D
*/
#include "ScriptMgr.h"
#include "Player.h"
#include "Configuration/Config.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "TnEchants.h"

 

class RandomEnchantsPlayer : public PlayerScript {
public:
    RandomEnchantsPlayer() : PlayerScript("RandomEnchantsPlayer") { }
     
    void OnPlayerLootItem(Player* player, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/) override {
        if (tnEchants->EnchantOnLoot)
            RollPossibleEnchant(player, item);
    }

    void OnPlayerCreateItem(Player* player, Item* item, uint32 /*count*/) override {
        if (tnEchants->EnchantOnCreate)
            RollPossibleEnchant(player, item);
    }

    void OnPlayerGroupRollRewardItem(Player* player, Item* item, uint32 /*count*/, RollVote /*voteType*/, Roll* /*roll*/) override {
        if (tnEchants->EnchantOnGroupRoll)
            RollPossibleEnchant(player, item);
    }

    // 配置已由 TnEchants::InitConfig() 缓存，这里只读内存；
    // 只有真正附魔成功时才落库，避免每次拾取都整角色存档。
    void RollPossibleEnchant(Player* player, Item* item) {
        // Check global enable option
        if (!tnEchants->ModuleEnable) {
            return;
        }
        if (!item) return; 
        if (rand_chance() > tnEchants->LootEnchantChance) return;

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
        if (tnEchants->RandomEnchEffect(player, item))
        {
            // 附魔只改 item_instance 的 enchantments 字段，单件落库即可；
            // 避免为一次附魔做整角色存档（登出/周期存档本就会保存，这里只为消除崩溃窗口）。
            CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
            item->SaveToDB(trans);
            CharacterDatabase.CommitTransaction(trans);
        }
    }
     
};

void AddRandomEnchantsScripts() {
    new RandomEnchantsPlayer();
}


