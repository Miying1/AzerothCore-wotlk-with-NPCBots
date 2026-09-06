/*
 * 白虎（生物 101000）对话脚本
 * 功能：
 *   1. 欢迎语（窗口正文）：多条问候文本随机显示（npc_text 表 ID 101000~101023，由 data/小宠物生物_101000_白虎.sql 维护）
 *   2. 宝物商店：选择后打开售卖窗口
 *   3. 航班：选择后打开飞行点地图
 *   4. 我的金币倍率：选择后由 NPC 悄悄话告知玩家真实倍率（100% + VIP 金币加成）
 * 配套 SQL：data/小宠物生物_101000_白虎.sql（需在 acore_world 库执行）
 */

#include "Chat.h"
#include "Creature.h"
#include "DBCStores.h"
#include "GossipDef.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "SharedDefines.h"
#include "StringFormat.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
constexpr uint32 NPC_BAIHU_ENTRY = 101000; // 白虎生物入口

// ===== 对话文本（硬编码于脚本头部）=====  
constexpr char const* TXT_SHOP = "宝物商店";                     // 宝物商店选项
constexpr char const* TXT_FLIGHT = "航班";                       // 航班选项
constexpr char const* TXT_GOLD_RATE = "我的金币倍率";             // 悄悄话告知选项

// 悄悄话模板：{} 为真实金币倍率（基础 100% + 玩家 VIP 金币加成，取自 PlayerVipBenefits）
constexpr char const* TXT_GOLD_RATE_WHISPER = "你的金币倍率为:|cffffd100{}%|r";

// ===== 欢迎语正文（npc_text 表）=====
constexpr uint32 TEXT_ID_BASE = 101000;          // 与生物入口一致，避开官方文本 ID 段
// 欢迎语文本条数（ID 101000 ~ 101023，由 data/小宠物生物_101000_白虎.sql 维护；
// SQL 中增删文本时需同步修改此值）
constexpr uint32 NPC_WELCOME_TEXT_COUNT = 23;

// ===== 菜单动作 ID =====
constexpr uint32 ACTION_SHOP = GOSSIP_ACTION_INFO_DEF + 1;      // 宝物商店
constexpr uint32 ACTION_FLIGHT = GOSSIP_ACTION_INFO_DEF + 2;    // 航班
constexpr uint32 ACTION_GOLD_RATE = GOSSIP_ACTION_INFO_DEF + 3; // 我的金币倍率

// 航班功能使用条件校验：返回空字符串表示允许使用，否则返回拒绝原因
// 条件：仅限大世界（大陆地图，不含副本/团队/战场/竞技场），且玩家存活、不在战斗中
std::string GetFlightDenyReason(Player* player)
{
    if (!player->GetMap()->GetEntry()->IsContinent())
        return "航班仅在大世界中可以使用。";
    if (!player->IsAlive())
        return "你已死亡，无法使用航班。";
    if (player->IsInCombat())
        return "你正处于战斗中，无法使用航班。";
    return {};
}

// 航班：打开飞行点地图（不要求 NPC 位于飞行点附近）
// 复刻 WorldSession::SendTaxiMenu，但 NPC 附近无飞行节点时回退为玩家已解锁的节点； 
void SendTaxiMapFor(Player* player, Creature* creature)
{
    // 优先取 NPC 附近的节点（标准行为）
    uint32 curloc = sObjectMgr->GetNearestTaxiNode(*creature, player->GetTeamId(true));

    // 回退 1：取玩家在同地图已解锁的节点
    for (uint32 i = 1; i < sTaxiNodesStore.GetNumRows() && !curloc; ++i)
    {
        TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(i);
        if (!node || node->map_id != player->GetMapId())
            continue;
        if (player->m_taxi.IsTaximaskNodeKnown(node->ID))
            curloc = node->ID;
    }
    // 回退 2：取玩家在任意地图已解锁的节点
    for (uint32 i = 1; i < sTaxiNodesStore.GetNumRows() && !curloc; ++i)
    {
        TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(i);
        if (node && player->m_taxi.IsTaximaskNodeKnown(node->ID))
            curloc = node->ID;
    }

    // 玩家尚未解锁任何飞行点，无法打开航班地图
    if (!curloc)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("你还没有解锁任何飞行点，无法打开航班地图。");
        return;
    }

    WorldPacket data(SMSG_SHOWTAXINODES, 4 + 8 + 4 + 8 * 4);
    data << uint32(1);
    data << creature->GetGUID();
    data << uint32(curloc);
    player->m_taxi.AppendTaximaskTo(data, player->isTaxiCheater());
    player->GetSession()->SendPacket(&data);
}
}

class NpcBaihuGossip : public CreatureScript
{
public:
    NpcBaihuGossip() : CreatureScript("npc_baihu_gossip") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->GetEntry() != NPC_BAIHU_ENTRY)
            return false;

        // 宝物商店（打开售卖窗口）
        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, TXT_SHOP, GOSSIP_SENDER_MAIN, ACTION_SHOP);
        // 航班（打开飞行点地图）：仅在满足使用条件（大世界 + 存活 + 非战斗）时显示
        if (GetFlightDenyReason(player).empty())
            AddGossipItemFor(player, GOSSIP_ICON_TAXI, TXT_FLIGHT, GOSSIP_SENDER_MAIN, ACTION_FLIGHT);
        // 我的金币倍率（NPC 悄悄话告知）
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, TXT_GOLD_RATE, GOSSIP_SENDER_MAIN, ACTION_GOLD_RATE);

        // 正文欢迎语：70% 概率取第一条，30% 概率随机取一条
        uint32 textId = TEXT_ID_BASE;
        if (!roll_chance_i(70))
            textId += urand(1, NPC_WELCOME_TEXT_COUNT - 1);
        SendGossipMenuFor(player, textId, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        switch (action)
        {
        case ACTION_SHOP: // 宝物商店：打开售卖窗口（商品列表见 npc_vendor 表）
            CloseGossipMenuFor(player);
            player->GetSession()->SendListInventory(creature->GetGUID());
            break;
        case ACTION_FLIGHT: // 航班：打开飞行点地图（NPC 不在飞行点附近时自动回退到已解锁节点）
        {
            // 防御性校验：仅限大世界，且玩家存活、不在战斗中
            std::string denyReason = GetFlightDenyReason(player);
            if (!denyReason.empty())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("{}", denyReason);
                CloseGossipMenuFor(player);
                break;
            }
            CloseGossipMenuFor(player);
            SendTaxiMapFor(player, creature);
            break;
        }
        case ACTION_GOLD_RATE: // 我的金币倍率：由 NPC 悄悄话告知真实倍率
        {
            uint32 goldRate = 100 + player->GetVipBenefits().gold_loot_bonus;
            creature->Whisper(Acore::StringFormat(TXT_GOLD_RATE_WHISPER, goldRate), LANG_UNIVERSAL, player);
            CloseGossipMenuFor(player);
            break;
        }
        default: // 未知选项：直接关闭
            CloseGossipMenuFor(player);
            break;
        }

        return true;
    }
};

void AddNpcBaihuGossipScripts()
{
    new NpcBaihuGossip();
}
