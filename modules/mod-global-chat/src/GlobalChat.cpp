/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Channel.h"
#include "Chat.h"
#include "Common.h"
#include "Config.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"

#include <unordered_map>

const char* CLASS_ICON;

#define FACTION_SPECIFIC 0

using namespace Acore::ChatCommands;

/* Config Variables */
struct GCConfig
{
    bool Enabled;
    bool Announce;
};

GCConfig GC_Config;

class GlobalChat_Config : public WorldScript
{
public:
    GlobalChat_Config() : WorldScript("GlobalChat_Config", {
        WORLDHOOK_ON_BEFORE_CONFIG_LOAD
    }) {}

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload)
        {
          GC_Config.Enabled = sConfigMgr->GetOption<bool>("GlobalChat.Enable", true);
          GC_Config.Announce = sConfigMgr->GetOption<bool>("GlobalChat.Announce", true);
        }
    }
};

/* STRUCTURE FOR GlobalChat map */
struct ChatElements
{
    uint8 chat = (GC_Config.Enabled) ? 0 : 1; // CHAT DISABLED BY DEFAULT
};

/* UNORDERED MAP FOR STORING IF CHAT IS ENABLED OR DISABLED */
std::unordered_map<uint32, ChatElements>GlobalChat;

std::string GetNameLink(Player* player)
{
    std::string name = player->GetName();
    std::string color;
    switch (player->getClass())
    {
    case CLASS_DEATH_KNIGHT:
        color = "|cffC41F3B";
        CLASS_ICON = "|TInterface\\icons\\Spell_Deathknight_ClassIcon:15|t|r";
        break;

    case CLASS_DRUID:
        color = "|cffFF7D0A";
        CLASS_ICON = "|TInterface\\icons\\Ability_Druid_Maul:15|t|r";
        break;

    case CLASS_HUNTER:
        color = "|cffABD473";
        CLASS_ICON = "|TInterface\\icons\\INV_Weapon_Bow_07:15|t|r";
        break;

    case CLASS_MAGE:
        color = "|cff69CCF0";
        CLASS_ICON = "|TInterface\\icons\\INV_Staff_13:15|t|r";
        break;

    case CLASS_PALADIN:
        color = "|cffF58CBA";
        CLASS_ICON = "|TInterface\\icons\\INV_Hammer_01:15|t|r";
        break;

    case CLASS_PRIEST:
        color = "|cffFFFFFF";
        CLASS_ICON = "|TInterface\\icons\\INV_Staff_30:15|t|r";
        break;

    case CLASS_ROGUE:
        color = "|cffFFF569";
        CLASS_ICON = "|TInterface\\icons\\INV_ThrowingKnife_04:15|t|r";
        break;

    case CLASS_SHAMAN:
        color = "|cff0070DE";
        CLASS_ICON = "|TInterface\\icons\\Spell_Nature_BloodLust:15|t|r";
        break;

    case CLASS_WARLOCK:
        color = "|cff9482C9";
        CLASS_ICON = "|TInterface\\icons\\Spell_Nature_FaerieFire:15|t|r";
        break;

    case CLASS_WARRIOR:
        color = "|cffC79C6E";
        CLASS_ICON = "|TInterface\\icons\\INV_Sword_27.png:15|t|r";
        break;

    }
    return "|Hplayer:" + name + "|h|cffFFFFFF[" + color + name + "|cffFFFFFF]|h|r";
}

class cs_global_chat : public CommandScript
{
public:
    cs_global_chat() : CommandScript("cs_global_chat") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable gcCommandTable =
        {
            { "on",  HandleGlobalChatOnCommand,  SEC_PLAYER, Console::Yes },
            { "off", HandleGlobalChatOffCommand, SEC_PLAYER, Console::Yes },
            { "",    HandleGlobalChatCommand,    SEC_PLAYER, Console::Yes }
        };
        static ChatCommandTable HandleGlobalChatCommandTable =
        {
            { "chat", gcCommandTable }
        };
        return HandleGlobalChatCommandTable;

    }

    static bool HandleGlobalChatOnCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint64 guid = player->GetGUID().GetCounter();

        if (!GC_Config.Enabled)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("世界聊天系统已关闭。|r");
            return true;
        }

        if (GlobalChat[guid].chat)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("世界聊天已经显示。|r");
            return true;
        }

        GlobalChat[guid].chat = 1;

        ChatHandler(player->GetSession()).PSendSysMessage("世界聊天现在已显示。|r");

        return true;
    };

    static bool HandleGlobalChatOffCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint64 guid = player->GetGUID().GetCounter();

        if (!GC_Config.Enabled)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("世界聊天系统已关闭。|r");
            return true;
        }

        if (!GlobalChat[guid].chat)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("世界聊天已经隐藏。|r");
            return true;
        }

        GlobalChat[guid].chat = 0;

        ChatHandler(player->GetSession()).PSendSysMessage("世界聊天现在已隐藏。|r");

        return true;
    };

    static bool HandleGlobalChatCommand(ChatHandler* handler, std::string args)
    {
        if (!handler->GetSession()->GetPlayer())
            return false;
        std::string temp = args;

        if (temp.find_first_not_of(' ') == std::string::npos)
            return false;

        std::string msg = "";
        Player* player = handler->GetSession()->GetPlayer();

        switch (player->GetSession()->GetSecurity())
        {
            // Player
        case SEC_PLAYER:
            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                msg += "|cffABD473[世] ";
                // msg += "|cff0000ff|TInterface\\pvpframe\\pvp-currency-alliance:17|t|r ";
                msg += GetNameLink(player);
                msg += " |cfffaeb00";
            }
            else
            {
                msg += "|cffABD473[世] ";
                // msg += "|cffff0000|TInterface\\pvpframe\\pvp-currency-horde:17|t|r ";
                msg += GetNameLink(player);
                msg += " |cfffaeb00";
            }
            break;

            // Moderator
        case SEC_MODERATOR:
            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                msg += "|cffABD473[世] ";
                msg += "|TINTERFACE/CHATFRAME/UI-CHATICON-BLIZZ:15|t";
                msg += GetNameLink(player);
                msg += " |cfffaeb00";
            }
            else
            {
                msg += "|cffABD473[世] ";
                msg += "|TINTERFACE/CHATFRAME/UI-CHATICON-BLIZZ:15|t";
                msg += GetNameLink(player);
                msg += " |cfffaeb00";
            }
            break;

            // GM
        case SEC_GAMEMASTER:
            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                msg += "|cffABD473[世] ";
                msg += "|TINTERFACE/CHATFRAME/UI-CHATICON-BLIZZ:15|t";
                msg += GetNameLink(player);
                msg += " |cfffaeb00";
            }
            else
            {
                msg += "|cffABD473[世] ";
                msg += "|TINTERFACE/CHATFRAME/UI-CHATICON-BLIZZ:15|t";
                msg += GetNameLink(player);
                msg += " |cfffaeb00";
            }
            break;

            // Admin
        case SEC_ADMINISTRATOR:
            // adding SEC_CONSOLE here to fix build warning
            // not sure if this needs to be handled separately
        case SEC_CONSOLE:
            if (player->GetTeamId() == TEAM_ALLIANCE)
            {
                msg += "|cffABD473[世] ";
                msg += "|TINTERFACE/CHATFRAME/UI-CHATICON-BLIZZ:15|t";
                msg += GetNameLink(player);
                msg += " |cfffaeb00";
            }
            else
            {
                msg += "|cffABD473[世] ";
                msg += "|TINTERFACE/CHATFRAME/UI-CHATICON-BLIZZ:15|t";
                msg += GetNameLink(player);
                msg += " |cfffaeb00";
            }
            break;

        }

        if (!GC_Config.Enabled)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("世界聊天系统已关闭。|r");
            return false;
        }

        if (!player->CanSpeak())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("你被禁言期间不能使用世界聊天！|r");
            return false;
        }

        if (!GlobalChat[player->GetGUID().GetCounter()].chat)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("世界聊天已隐藏。（.chat off）|r");
            return false;
        }

        msg += args;
        WorldSessionMgr::SessionMap sessions = sWorldSessionMgr->GetAllSessions();
        for (WorldSessionMgr::SessionMap::iterator itr = sessions.begin(); itr != sessions.end(); ++itr)
        {
            if (!itr->second)
                continue;

            Player* receiver = itr->second->GetPlayer();
            if (!receiver || !receiver->IsInWorld())
                continue;

            if (!GlobalChat[receiver->GetGUID().GetCounter()].chat)
                continue;

            // if (FACTION_SPECIFIC && receiver->GetTeamId() != player->GetTeamId())
            //     continue;
            sWorldSessionMgr->SendServerMessage(SERVER_MSG_STRING, msg.c_str(), receiver);
        }

        return true;
    }
};

class GlobalChat_Announce : public PlayerScript
{
public:

    GlobalChat_Announce() : PlayerScript("GlobalChat_Announce", {
        PLAYERHOOK_ON_LOGIN
    }) {}

    void OnPlayerLogin(Player* player) override
    {
        // Announce Module
        if (GC_Config.Enabled && GC_Config.Announce)
            ChatHandler(player->GetSession()).SendSysMessage("|cff4CFF00Azerothcore 世界聊天|r 使用 .chat 命令进行世界聊天");
    }
};

void AddSC_global_chat()
{
    new GlobalChat_Config();
    new cs_global_chat();
    new GlobalChat_Announce();
}
