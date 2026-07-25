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

class reward_shop : public CreatureScript
{
public:
    reward_shop() : CreatureScript("reward_shop") {}

    bool failedcode;

    bool OnGossipHello(Player *player, Creature *creature)
    {
        if (player->IsInCombat())
            return false;

        if (!sConfigMgr->GetOption<bool>("RewardShopEnable", 0))
            return false;

        
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我想兑换我的代码.", GOSSIP_SENDER_MAIN, 1, "", 0, true);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "离开...", GOSSIP_SENDER_MAIN, 3);

        if (sConfigMgr->GetOption<bool>("AllowGM", 1) && player->IsGameMaster())
        {
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "[GM] 我想生成一个代码.", GOSSIP_SENDER_MAIN, 4);
        }

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player *player, Creature *creature, uint32 /* sender */, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus(); 
        uint32 rnd1 = urand(10000, 90000);
        uint32 rnd2 = urand(10000, 90000); 

        std::string CreatedBy = player->GetName();
        std::ostringstream randomcode;
        

        switch (action)
        { 
        case 3:
            CloseGossipMenuFor(player);
            break;
        case 4:
            player->PlayerTalkClass->ClearMenus();
            AddGossipItemFor(player, GOSSIP_ICON_CHAT,std::string("生成名称更改代码.").c_str(), GOSSIP_SENDER_MAIN, 6);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "生成阵营改变代码.", GOSSIP_SENDER_MAIN, 7);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "生成种族改变代码.", GOSSIP_SENDER_MAIN, 8);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 6:
            randomcode << "N" << rnd1 << "-" << rnd2;
            CharacterDatabase.Query("INSERT INTO `reward_shop` (`action`, `action_data`, `quantity`, `code`, `status`,isonly, `PlayerGUID`,  `CreatedBy`) VALUES(3, 0, 0, '{}', 0, 0, 0, '{}')", randomcode.str().c_str(), CreatedBy.c_str());
            ChatHandler(player->GetSession()).PSendSysMessage("Code was successfully created your code is %s", randomcode.str().c_str());
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 7:
            randomcode << "F" << rnd1 << "-" << rnd2;
            CharacterDatabase.Query("INSERT INTO `reward_shop` (`action`, `action_data`, `quantity`, `code`, `status`, isonly,`PlayerGUID`, `CreatedBy`) VALUES(4, 0, 0, '{}', 0, 0, 0, '{}')", randomcode.str().c_str(), CreatedBy.c_str());
            ChatHandler(player->GetSession()).PSendSysMessage("Code was successfully created your code is %s", randomcode.str().c_str());
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        case 8:
            randomcode << "R" << rnd1 << "-" << rnd2;
            CharacterDatabase.Query("INSERT INTO `reward_shop` (`action`, `action_data`, `quantity`, `code`, `status`, isonly,`PlayerGUID`, `CreatedBy`) VALUES(5, 0, 0, '{}', 0, 0, 0, '{}')", randomcode.str().c_str(), CreatedBy.c_str());
            ChatHandler(player->GetSession()).PSendSysMessage("Code was successfully created your code is %s", randomcode.str().c_str());
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            break;
        }
        return true;
    }

    bool OnGossipSelectCode(Player *player, Creature *creature, uint32 /* sender */, uint32, const char *code)
    {
        ObjectGuid playerguid = player->GetGUID(); 
        std::string rewardcode = code; 

        std::size_t found = rewardcode.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-");

        if (found != std::string::npos)
            return false;

        // check for code
        QueryResult result = CharacterDatabase.Query("SELECT action, action_data, quantity, status,isonly FROM reward_shop WHERE code = '{}'", rewardcode.c_str());

        if (!result)
        {
            player->PlayDirectSound(9638); // No
            creature->Whisper("兑换代码无效!", LANG_UNIVERSAL, player);
            creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return false;
        }
      
        //std::transform(rewardcode.begin(), rewardcode.end(), rewardcode.begin(), ::toupper);

        
        Field *fields = result->Fetch();
        uint32 action = fields[0].Get<uint32>();
        uint32 action_data = fields[1].Get<uint32>();
        uint32 quantity = fields[2].Get<uint32>();
        uint32 status = fields[3].Get<int32>();
        uint32 isonly = fields[4].Get<int32>();
        if (status == 1)
        {
            player->PlayDirectSound(9638); // No
            creature->Whisper("兑换码已被使用!", LANG_UNIVERSAL, player);
            creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return false;
        }
        if(isonly==1){
            QueryResult check_only = CharacterDatabase.Query("SELECT 1 FROM reward_shop WHERE action_data = {} and PlayerGUID={} and status=1", action_data,playerguid.GetCounter());
            if(check_only){
                player->PlayDirectSound(9638); // No
                creature->Whisper("你已领取过这类奖励了!", LANG_UNIVERSAL, player);
                creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
                return false;
            }
        }
        int count = 1;
        uint32 noSpaceForCount = 0;
        ItemPosCountVec dest;
        InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, action_data, quantity, &noSpaceForCount);  
        switch (action)
        {

        case 1: /* Item */
            if (msg != EQUIP_ERR_OK)
                count -= noSpaceForCount;

            if (count == 0 || dest.empty())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("无法发送奖励，你的背包满了或你已拥有相同的唯一物品!");
                ChatHandler(player->GetSession()).SetSentErrorMessage(true);
                return false;
            } 
            if (count > 0 && action_data)
            {
                player->AddItem(action_data, quantity);
            }
            break;
        case 2: /* Gold */
            player->ModifyMoney(action_data * 10000);
            ChatHandler(player->GetSession()).PSendSysMessage("成功发送G币: [%u G]", action_data);
            break;
        case 3: /* Name Change */
            player->SetAtLoginFlag(AT_LOGIN_RENAME);
            ChatHandler(player->GetSession()).PSendSysMessage("CHAT OUTPUT: 请返回角色登录界面以修改角色名称.");
            break;
        case 4: /* Faction Change */
            player->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
            ChatHandler(player->GetSession()).PSendSysMessage("CHAT OUTPUT: 请返回角色登录界面选择阵营.");
            break;
        case 5: /* Race Change */
            player->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);
            ChatHandler(player->GetSession()).PSendSysMessage("CHAT OUTPUT: 请返回角色登录界面修改种族.");
            break;
        } 

        CharacterDatabase.Query("UPDATE reward_shop SET status = 1, PlayerGUID = '{}' WHERE code = '{}'", playerguid.GetCounter(), rewardcode);
        CloseGossipMenuFor(player);
        return true;
    }
     
};

void AddRewardShopScripts()
{
    new reward_shop();
}
