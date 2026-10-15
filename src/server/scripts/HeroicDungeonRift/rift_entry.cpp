/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or any later version.
 */

#include "rift_defines.h"

#include "Chat.h"
#include "Creature.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "StringConvert.h"
#include "StringFormat.h"

namespace HeroicDungeonRift
{
namespace
{
constexpr uint32 RiftFragmentItem = 60106;
constexpr uint64 RiftTier1GoldCost = 100 * GOLD;

uint32 GetTierFragmentCost(uint8 tier)
{
    return tier == 2 ? 1 : 3;
}

void SendError(Player* player, std::string const& error)
{
    if (player && player->GetSession())
        ChatHandler(player->GetSession()).SendSysMessage(error);
}

void SendTierConfirmation(Player* player, Creature* creature, uint8 tier)
{
    uint32 action = GossipConfirmTier1 + tier - 1;
    ClearGossipMenuFor(player);
    if (tier == 1)
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "确认进入 T1（消耗 100 金币）", GOSSIP_SENDER_MAIN, action);
    else
    {
        uint32 cost = GetTierFragmentCost(tier);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT,
            Acore::StringFormat("确认进入 T{}（消耗 {} 个|c{:08x}[裂隙碎片]|r）",
                uint32(tier), cost, ItemQualityColors[ITEM_QUALITY_EPIC]),
            GOSSIP_SENDER_MAIN, action);
    }
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "取消", GOSSIP_SENDER_MAIN, GossipExit);
    SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
}
}

class npc_rift_entry : public CreatureScript
{
public:
    npc_rift_entry() : CreatureScript("npc_rift_entry") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "进入 T1", GOSSIP_SENDER_MAIN, GossipEnterTier1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "进入 T2", GOSSIP_SENDER_MAIN, GossipEnterTier2);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "进入 T3", GOSSIP_SENDER_MAIN, GossipEnterTier3);
        if (player->IsGameMaster())
        {
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "指定T1", GOSSIP_SENDER_MAIN, GossipSpecifyTier1,
                "请输入 boss_id", 0, true);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "指定T2", GOSSIP_SENDER_MAIN, GossipSpecifyTier2,
                "请输入 boss_id", 0, true);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "指定T3", GOSSIP_SENDER_MAIN, GossipSpecifyTier3,
                "请输入 boss_id", 0, true);
        }
        
        SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (sender != GOSSIP_SENDER_MAIN)
        {
            CloseGossipMenuFor(player);
            return true;
        }

        if (action == GossipExit)
        {
            CloseGossipMenuFor(player);
            return true;
        }

        if (action >= GossipEnterTier1 && action <= GossipEnterTier3)
        {
            uint8 tier = uint8(action - GossipEnterTier1 + 1);
            SendTierConfirmation(player, creature, tier);
            return true;
        }

        if (action >= GossipConfirmTier1 && action <= GossipConfirmTier3)
        {
            CloseGossipMenuFor(player);
            uint8 tier = uint8(action - GossipConfirmTier1 + 1);
            uint32 fragmentCost = tier == 1 ? 0 : GetTierFragmentCost(tier);
            if (tier == 1)
            {
                if (player->GetMoney() < RiftTier1GoldCost)
                {
                    SendError(player, "进入 T1 需要 100 金币。");
                    return true;
                }
            }
            else if (!player->HasItemCount(RiftFragmentItem, fragmentCost))
            {
                SendError(player, Acore::StringFormat("进入 T{} 需要 {} 个裂隙碎片。",
                    uint32(tier), fragmentCost));
                return true;
            }

            std::string error;
            if (RunManager::Instance().StartRun(player, tier, error))
            {
                if (tier == 1)
                    player->ModifyMoney(-int32(RiftTier1GoldCost));
                else
                    player->DestroyItemCount(RiftFragmentItem, fragmentCost, true);
            }
            else
                SendError(player, error);
            return true;
        }

        CloseGossipMenuFor(player);
        if (action >= GossipSpecifyTier1 && action <= GossipSpecifyTier3)
            SendError(player, "请输入有效的boss_id。");
        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* /*creature*/, uint32 sender, uint32 action,
        char const* code) override
    {
        CloseGossipMenuFor(player);
        if (sender != GOSSIP_SENDER_MAIN || action < GossipSpecifyTier1 || action > GossipSpecifyTier3)
            return true;

        auto bossId = code ? Acore::StringTo<uint32>(code) : std::nullopt;
        if (!bossId || !*bossId)
        {
            SendError(player, "请输入有效的boss_id。");
            return true;
        }

        uint8 tier = uint8(action - GossipSpecifyTier1 + 1);
        std::string error;
        if (!RunManager::Instance().StartRun(player, tier, *bossId, error))
            SendError(player, error);
        return true;
    }
};

void AddSC_rift_entry()
{
    new npc_rift_entry();
}

} // namespace HeroicDungeonRift
