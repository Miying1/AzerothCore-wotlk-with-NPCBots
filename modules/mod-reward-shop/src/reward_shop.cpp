/*

Database Actions:

1 = item
2 = gold

每个兑换码最多支持三组 action。

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
#include <algorithm>
#include <utility>
#include <vector>

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

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player *player, Creature *creature, uint32 /* sender */, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus(); 
        if (action == 1)
            return true;

        CloseGossipMenuFor(player);
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
        QueryResult result = CharacterDatabase.Query("SELECT action, action_data, quantity, action2, action_data2, quantity2, action3, action_data3, quantity3, status, isonly FROM reward_shop WHERE code = '{}'", rewardcode.c_str());

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
        uint32 actions[3] = { fields[0].Get<uint32>(), fields[3].Get<uint32>(), fields[6].Get<uint32>() };
        uint32 actionData[3] = { fields[1].Get<uint32>(), fields[4].Get<uint32>(), fields[7].Get<uint32>() };
        uint32 quantities[3] = { fields[2].Get<uint32>(), fields[5].Get<uint32>(), fields[8].Get<uint32>() };
        uint32 status = fields[9].Get<uint32>();
        uint32 isonly = fields[10].Get<uint32>();
        if (status == 1)
        {
            player->PlayDirectSound(9638); // No
            creature->Whisper("兑换码已被使用!", LANG_UNIVERSAL, player);
            creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return false;
        }
        if (isonly == 1)
        {
            uint32 uniqueRewardData = actionData[0];
            if (!uniqueRewardData)
                uniqueRewardData = actionData[1] ? actionData[1] : actionData[2];

            QueryResult check_only = CharacterDatabase.Query("SELECT 1 FROM reward_shop WHERE action_data = {} and PlayerGUID={} and status=1", uniqueRewardData, playerguid.GetCounter());
            if (check_only)
            {
                player->PlayDirectSound(9638); // No
                creature->Whisper("你已领取过这类奖励了!", LANG_UNIVERSAL, player);
                creature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);
                SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
                return false;
            }
        }

        std::vector<std::pair<uint32, uint32>> itemRewards;
        uint32 totalGold = 0;
        bool hasReward = false;
        for (uint32 index = 0; index < 3; ++index)
        {
            if (!actions[index])
                continue;

            if (actions[index] == 1)
            {
                if (!actionData[index] || !quantities[index])
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("无法发送奖励，配置的物品数据无效!");
                    ChatHandler(player->GetSession()).SetSentErrorMessage(true);
                    return false;
                }

                auto item = std::find_if(itemRewards.begin(), itemRewards.end(), [itemId = actionData[index]](auto const& reward)
                {
                    return reward.first == itemId;
                });
                if (item == itemRewards.end())
                    itemRewards.emplace_back(actionData[index], quantities[index]);
                else
                    item->second += quantities[index];

                hasReward = true;
            }
            else if (actions[index] == 2)
            {
                if (!actionData[index])
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("无法发送奖励，配置的金币数据无效!");
                    ChatHandler(player->GetSession()).SetSentErrorMessage(true);
                    return false;
                }

                totalGold += actionData[index];
                hasReward = true;
            }
            else
            {
                ChatHandler(player->GetSession()).PSendSysMessage("无法发送奖励，配置的 action 无效!");
                ChatHandler(player->GetSession()).SetSentErrorMessage(true);
                return false;
            }
        }

        if (!hasReward)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("无法发送奖励，未配置有效的 action!");
            ChatHandler(player->GetSession()).SetSentErrorMessage(true);
            return false;
        }

        std::vector<std::pair<std::pair<uint32, uint32>, ItemPosCountVec>> itemDestinations;
        ItemPosCountVec reservedDestinations;
        for (auto const& item : itemRewards)
        {
            uint32 noSpaceForCount = 0;
            ItemPosCountVec destinations;
            std::size_t reservedCount = reservedDestinations.size();
            if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, reservedDestinations, item.first, item.second, &noSpaceForCount) != EQUIP_ERR_OK)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("无法发送奖励，你的背包满了或你已拥有相同的唯一物品!");
                ChatHandler(player->GetSession()).SetSentErrorMessage(true);
                return false;
            }

            destinations.assign(reservedDestinations.begin() + reservedCount, reservedDestinations.end());
            itemDestinations.emplace_back(item, std::move(destinations));
        }

        CharacterDatabase.DirectExecute("UPDATE reward_shop SET status = 1, PlayerGUID = '{}' WHERE code = '{}' AND status = 0", playerguid.GetCounter(), rewardcode);
        QueryResult redeemed = CharacterDatabase.Query("SELECT 1 FROM reward_shop WHERE code = '{}' AND status = 1 AND PlayerGUID = '{}'", rewardcode, playerguid.GetCounter());
        if (!redeemed)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("兑换代码状态更新失败，请稍后重试!");
            ChatHandler(player->GetSession()).SetSentErrorMessage(true);
            return false;
        }

        for (auto const& item : itemDestinations)
            player->StoreNewItem(item.second, item.first.first, true);

        if (totalGold)
        {
            player->ModifyMoney(totalGold * 10000);
            ChatHandler(player->GetSession()).PSendSysMessage("成功发送G币: [%u G]", totalGold);
        }
        CloseGossipMenuFor(player);
        return true;
    }
     
};

void AddRewardShopScripts()
{
    new reward_shop();
}
