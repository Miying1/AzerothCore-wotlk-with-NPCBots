/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or any later version.
 */

#include "rift_defines.h"

#include "Chat.h"
#include "GameObject.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"

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

class rift_exit_portal : public GameObjectScript
{
public:
    rift_exit_portal() : GameObjectScript("rift_exit_portal") { }

    bool OnGossipHello(Player* player, GameObject* portal) override
    {
        std::shared_ptr<RunComponent> run = RunManager::Instance().FindByPlayer(player->GetGUID());
        if (!IsExitPortalEntry(portal->GetEntry()) || !run || run->ExitPortalGuid != portal->GetGUID())
        {
            SendError(player, "该传送门不属于你的当前裂隙。");
            return true;
        }

        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "离开...", GOSSIP_SENDER_MAIN, GossipExit);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, portal->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, GameObject* portal, uint32 sender, uint32 action) override
    {
        CloseGossipMenuFor(player);
        if (sender != GOSSIP_SENDER_MAIN || action != GossipExit)
            return true;

        std::string error;
        if (!RunManager::Instance().ExitRun(player, portal, error))
            SendError(player, error);
        return true;
    }
};

void AddSC_rift_exit_portal()
{
    new rift_exit_portal();
}

} // namespace HeroicDungeonRift
