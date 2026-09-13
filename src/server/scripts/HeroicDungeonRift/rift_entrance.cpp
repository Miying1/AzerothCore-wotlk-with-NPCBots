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
#include "CreatureScript.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "StringFormat.h"

namespace HeroicDungeonRift
{
namespace
{
void SendEntranceError(Player* player, std::string const& error)
{
    if (player && player->GetSession())
        ChatHandler(player->GetSession()).SendSysMessage(error);
}
}

// 开放区域中随机刷新的裂隙入口：采用卡拉赞虚空幽龙的三色虚空门生物。
// 点击弹出“进入 Tn 裂隙”，玩家成功进入裂隙后本入口立即失效，并在宽限期结束后移除。
class npc_rift_portal : public CreatureScript
{
public:
    npc_rift_portal() : CreatureScript("npc_rift_portal") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        RiftSpawnManager& spawns = RiftSpawnManager::Instance();
        uint8 tier = spawns.GetEntranceTier(creature);
        if (!tier || spawns.IsEntranceConsumed(creature))
        {
            SendEntranceError(player, "这道裂隙入口已经失效了。");
            return true;
        }

        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, Acore::StringFormat("进入 T{} 裂隙", uint32(tier)),
            GOSSIP_SENDER_MAIN, GossipEnterRiftEntrance);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        CloseGossipMenuFor(player);
        if (sender != GOSSIP_SENDER_MAIN || action != GossipEnterRiftEntrance)
            return true;

        RiftSpawnManager& spawns = RiftSpawnManager::Instance();
        uint8 tier = spawns.GetEntranceTier(creature);
        if (!tier || spawns.IsEntranceConsumed(creature))
        {
            SendEntranceError(player, "这道裂隙入口已经失效了。");
            return true;
        }

        std::string error;
        if (!RunManager::Instance().StartRun(player, tier, error))
        {
            // 进入失败不消耗入口，玩家可以调整队伍后重试。
            SendEntranceError(player, error);
            return true;
        }

        spawns.ConsumeEntrance(creature);
        return true;
    }
};

void AddSC_rift_entrance()
{
    new npc_rift_portal();
}

} // namespace HeroicDungeonRift
