/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Config.h"
#include "Chat.h" 
#include "ItemTemplate.h"
#include "MapMgr.h" 
#include "Player.h" 
#include "ScriptedCreature.h"
#include "ScriptMgr.h"    
#include "Unit.h" 

struct RaidLeaderReawrd
{
  uint32 bossid;
  uint8 player_count1;
  uint32 reawrd1;
  uint8 player_count2;
  uint32 reawrd2;
};

std::vector<RaidLearderReawrd> LeaderReawrdData;

class mod_raidleader_reawrd_worldscript : public WorldScript
{
public:
    mod_raidleader_reawrd_worldscript() : WorldScript("mod_raidleader_reawrd_worldscript") { }
 
    void OnStartup() override
    {  
        if (QueryResult result = WorldDatabase.Query("SELECT bossid,player_count1,reawrd1,player_count2,reawrd2 FROM mod_raidleader_reawrd where active=1"))
        {
            do
            {
                RaidLeaderReawrd data={};
                data.bossid = (*result)[0].Get<uint32>(); 
                data.player_count1 = (*result)[1].Get<uint8>();
                data.reawrd1 = (*result)[2].Get<uint32>();
                data.player_count2 = (*result)[3].Get<uint8>();
                data.reawrd2 = (*result)[4].Get<uint32>(); 
                LeaderReawrdData.push_back(data);

            } while (result->NextRow());
        }
    }
};

class mod_raidleader_reawrd_globalscript : public GlobalScript
{
public:
    mod_raidleader_reawrd_globalscript() : GlobalScript("mod_raidleader_reawrd_globalscript") { }
    void OnBeforeSetBossState(uint32 id, EncounterState newState, EncounterState oldState, Map* instance) override
    {  
       if (!instance->IsRaid() ||  newState!=DONE)
       {
           return;
       }
       InstanceScript* insScript= instance->ToInstanceMap()->GetInstanceScript();
       auto boss=insScript->GetCreature(id);
       if(!boss || boss->IsAlive()) {
          LOG_INFO("module", "BOSS:{} ({}) 状态完成，但还活着",boss->GetName(),boss->GetEntry());
          return;
       }
       for (std::list<uint32>::const_iterator itr = LeaderReawrdData.begin(); itr != LeaderReawrdData.end(); ++itr)
        {
            if((*itr)->bossid != boss->GetEntry()) 
              continue; 
            Map::PlayerList const& playerList = map->GetPlayers(); 
            uint32 plcount=playerList.getSize();
            uint32 reawrd_loot=0;
            if((*itr)->player_count2>0 && plcount>=(*itr)->player_count2){
                  reawrd_loot=(*itr)->reawrd2;
            }else if((*itr)->player_count1>0 && plcount>=(*itr)->player_count1){
                  reawrd_loot=(*itr)->reawrd1;
            }
            if(reawrd_loot==0) break; 
            for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
            {
                Player* player = i->GetSource();
                if (Group* group = player->GetGroup()) {
                    Player* leader=group->GetLeader();
                    if(leader->IsInWorld()){
                        if (!leader->AddItem(reawrd_loot, 1)) {
                          leader->SendItemRetrievalMail(reawrd_loot, 1);
                        }  
                    }else{
                        leader->SendItemRetrievalMail(reawrd_loot, 1);
                    }
                    break;
                } 
            }  
            break;
        }
    }
    // void OnAfterUpdateEncounterState(Map* map, EncounterCreditType /*type*/, uint32 /*creditEntry*/, Unit* source, Difficulty /*difficulty_fixed*/, DungeonEncounterList const* /*encounters*/, uint32  dungeonCompleted, bool /*updated*/) override
    // {

    //     if (!map->IsRaid() || !source || !source->ToCreature())
    //     { 
    //         return;
    //     } 
    //     for (std::list<uint32>::const_iterator itr = LeaderReawrdData.begin(); itr != LeaderReawrdData.end(); ++itr)
    //     {
    //         if((*itr)->bossid!= source->GetEntry()) 
    //           continue; 
    //         Map::PlayerList const& playerList = map->GetPlayers(); 
    //         uint32 plcount=playerList.getSize();
    //         uint32 reawrd_loot=0;
    //         if((*itr)->player_count2>0 && plcount>=(*itr)->player_count2){
    //               reawrd_loot=(*itr)->reawrd2;
    //         }else if((*itr)->player_count1>0 && plcount>=(*itr)->player_count1){
    //               reawrd_loot=(*itr)->reawrd1;
    //         }
    //         if(reawrd_loot==0) break; 
    //         for (Map::PlayerList::const_iterator i = playerList.begin(); i != playerList.end(); ++i)
    //         {
    //             Player* player = i->GetSource();
    //             if (Group* group = player->GetGroup()) {
    //                 Player* leader=group->GetLeader();
    //                 if(leader->IsInWorld()){
    //                     if (!leader->AddItem(reawrd_loot, 1)) {
    //                       leader->SendItemRetrievalMail(reawrd_loot, 1);
    //                     }  
    //                 }else{
    //                     leader->SendItemRetrievalMail(reawrd_loot, 1);
    //                 }
    //                 break;
    //             } 
    //         }  
    //         break;
    //     }
    // }

};


// Add all scripts in one
void AddModRaidLeaderReawrdScripts()
{
    new mod_raidleader_reawrd_worldscript();
    new mod_raidleader_reawrd_globalscript(); 
}
