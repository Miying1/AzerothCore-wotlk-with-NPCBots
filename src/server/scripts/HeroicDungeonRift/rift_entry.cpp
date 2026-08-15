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

namespace HeroicDungeonRift
{
namespace
{
void SendError(Player* player, std::string const& error)
{
    if (player && player->GetSession())
        ChatHandler(player->GetSession()).SendSysMessage(error);
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

    bool OnGossipSelect(Player* player, Creature* /*creature*/, uint32 sender, uint32 action) override
    {
        CloseGossipMenuFor(player);
        if (sender != GOSSIP_SENDER_MAIN)
            return true;

        if (action >= GossipSpecifyTier1 && action <= GossipSpecifyTier3)
        {
            SendError(player, "请输入有效的boss_id。");
            return true;
        }

        if (action < GossipEnterTier1 || action > GossipEnterTier3)
            return true;

        uint8 tier = uint8(action - GossipEnterTier1 + 1);
        std::string error;
        if (!RunManager::Instance().StartRun(player, tier, error))
            SendError(player, error);
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
