/*
 * 账号银行 NPC：提供「个人银行 / 账号银行」切换入口
 *
 * 玩家通过与银行 NPC（gossip 菜单项）交互，在个人银行（原版，角色私有）与
 * 账号银行（扩展，同账号所有角色共享）之间切换。切换逻辑由 Player::SwitchBankMode
 * 完成：保存当前银行 -> 软卸载 -> 切换模式 -> 加载目标银行 -> 刷新槽数显示。
 */

#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "WorldSession.h"

// 账号银行 NPC：提供「个人银行 / 账号银行」切换入口
class npc_account_bank : public CreatureScript
{
public:
    npc_account_bank() : CreatureScript("npc_account_bank") { }

    // 打开 gossip 菜单
    bool OnGossipHello(Player* player, Creature* creature) override
    {
        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "我想使用个人银行",
                         GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "我想使用账号银行",
                         GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    // 处理菜单选择
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (sender == GOSSIP_SENDER_MAIN)
        {
            if (action == GOSSIP_ACTION_INFO_DEF + 1)
                player->SwitchBankMode(BANK_MODE_PERSONAL);   // 个人银行
            else if (action == GOSSIP_ACTION_INFO_DEF + 2)
                player->SwitchBankMode(BANK_MODE_ACCOUNT);    // 账号银行
        }

        player->PlayerTalkClass->SendCloseGossip();
        player->GetSession()->SendShowBank(creature->GetGUID());
        return true;
    }
};

void AddSC_npc_account_bank()
{
    new npc_account_bank();
}
