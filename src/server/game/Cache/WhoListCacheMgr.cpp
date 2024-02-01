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

#include "WhoListCacheMgr.h"
#include "GuildMgr.h"
#include "ObjectAccessor.h"
#include "World.h"
#include "Config.h"

WhoListCacheMgr* WhoListCacheMgr::instance()
{
    static WhoListCacheMgr instance;
    return &instance;
}

void WhoListCacheMgr::Update()
{
    // clear current list
    _whoListStorage.clear();
    _whoListStorage.reserve(sWorld->GetPlayerCount() + 1);

    for (auto const& [guid, player] : ObjectAccessor::GetPlayers())
    {
        if (!player->FindMap() || player->GetSession()->PlayerLoading())
            continue;

        std::string playerName = player->GetName();
        std::wstring widePlayerName;

        if (!Utf8toWStr(playerName, widePlayerName))
            continue;

        wstrToLower(widePlayerName);

        std::string guildName = sGuildMgr->GetGuildNameById(player->GetGuildId());
        std::wstring wideGuildName;

        if (!Utf8toWStr(guildName, wideGuildName))
            continue;

        wstrToLower(wideGuildName);

        _whoListStorage.emplace_back(player->GetGUID(), player->GetTeamId(), player->GetSession()->GetSecurity(), player->GetLevel(),
            player->getClass(), player->getRace(),
            (player->IsSpectator() ? 4395 /*Dalaran*/ : player->GetZoneId()), player->getGender(), player->IsVisible(),
            widePlayerName, wideGuildName, playerName, guildName);
        
    }
    if(sConfigMgr->GetOption<uint32>("WhoListOnlineBot", 0)){
        AddOnlineBot(sWorld->GetPlayerCount());
    }

}
void WhoListCacheMgr::AddOnlineBot(uint32 count)
{
    count= count<10?10:count;
    QueryResult result = CharacterDatabase.Query("select guid,level,name,race,class,gender,zone from characters where online=0 and level>20 order by rand()  limit {}", count);
    if (result) {
        do
        {
            Field* fields = result->Fetch();  
            uint32 pid=fields[0].Get<uint32>();
            ObjectGuid playerguid = ObjectGuid(HighGuid::Player, 0, pid);
            uint32 level=fields[1].Get<uint32>();
            std::string name=fields[2].Get<std::string>();
            std::wstring widePlayerName;
            if (!Utf8toWStr(name, widePlayerName))
                continue;
            std::string guildName = sGuildMgr->GetGuildNameById(pid);
            std::wstring wideGuildName; 
            if (!Utf8toWStr(guildName, wideGuildName))
                continue;
            uint8 race=fields[3].Get<uint8>();
            uint8 classid=fields[4].Get<uint8>();
            uint8 gender=fields[5].Get<uint8>();
            uint32 zoneid=fields[6].Get<uint32>();
            _whoListStorage.emplace_back(playerguid, TeamId::TEAM_HORDE, AccountTypes::SEC_PLAYER, level,classid, race,
             zoneid, gender, true,  widePlayerName, wideGuildName, name, guildName);
        } while (result->NextRow());
       
    }
}
