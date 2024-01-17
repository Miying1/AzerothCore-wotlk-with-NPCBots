/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Config.h"
#include "Chat.h"
#include "GameTime.h"
#include "ItemTemplate.h"
#include "MapMgr.h"
#include "Pet.h"
#include "Player.h"
#include "PoolMgr.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "Unit.h"
#include "ChallengeDifficulty.h"
#include "botmgr.h"

ChallengeDifficulty* ChallengeDifficulty::instance()
{
    static ChallengeDifficulty instance; 
    return &instance;
}

 
void ChallengeDifficulty::LoadIntiData()
{
    sChallengeDiff->ZoneChallengeSpellData.clear();
    sChallengeDiff->ZoneChallengeSpellGroupData.clear();
    sChallengeDiff->PlayerLevelData.clear();
    sChallengeDiff->MythicmodeAI.clear();
    sChallengeDiff->DiffLevelData.clear();
    sChallengeDiff->BaseEnhanceMapData.clear();
    std::vector<bool> instanceIDs = sMapMgr->GetInstanceIDs();

    if (QueryResult result = CharacterDatabase.Query("SELECT InstanceID,level,enhance_damage,enhance_hp,kill_boss,spell_id1,spell_id2,spell_id3,is_complete FROM zone_difficulty_instance_saves"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 InstanceId = fields[0].Get<uint32>();

            if (instanceIDs[InstanceId])
            {
                ZoneChallengeData cdata;
                cdata.level = fields[1].Get<uint32>();
                cdata.enhance_damage = fields[2].Get<uint32>();
                cdata.enhance_hp = fields[3].Get<uint32>();
                cdata.kill_boss= fields[4].Get<uint32>();
                cdata.apply_spell[0] = fields[5].Get<uint32>();
                cdata.apply_spell[1] = fields[6].Get<uint32>();
                cdata.apply_spell[2] = fields[7].Get<uint32>();
                cdata.is_complete = fields[8].Get<bool>();
                sChallengeDiff->ChallengeInstanceData[InstanceId] = cdata;
            }
            else
            {
                CharacterDatabase.Execute("DELETE FROM zone_difficulty_instance_saves WHERE InstanceID = {}", InstanceId);
            }
        } while (result->NextRow());
    }
    else
    {
        LOG_ERROR("module", "zone_difficulty_instance_saves: Query error");
    }
    //每日激活地图
    if (QueryResult result = CharacterDatabase.Query("select mdayticker,active_mapid1,active_mapid2,active_mapid3,active_mapid4  from zone_diffculty_activemap"))
    {
        DayActiveMaps = {};
        DayActiveMaps.mdayticker = (*result)[0].Get<uint32>();
        DayActiveMaps.active_mapid[0] = (*result)[1].Get<uint32>();
        DayActiveMaps.active_mapid[1] = (*result)[2].Get<uint32>();
        DayActiveMaps.active_mapid[2] = (*result)[3].Get<uint32>();
        DayActiveMaps.active_mapid[3] = (*result)[4].Get<uint32>(); 
    }
    else
    {
        LOG_ERROR("module", "zone_diffculty_activemap: Query error");
    }
    //挑战难度等级
    if (QueryResult result = WorldDatabase.Query("SELECT difflevel,enhance,diff_player,global_spell_num,boss_score,award1,award2,award3  FROM zone_difficulty_level"))
    {
        do
        {
            uint32 difflevel = (*result)[0].Get<uint32>();
            ZoneDifficultyLevel level;
            level.difflevel = difflevel;
            level.enhance = (*result)[1].Get<uint32>();
            level.diff_player = (*result)[2].Get<uint32>();
            level.global_spell_num = (*result)[3].Get<uint32>();
            level.boss_score = (*result)[4].Get<uint32>();
            level.award1 = (*result)[5].Get<uint32>();
            level.award2 = (*result)[6].Get<uint32>();
            level.award3 = (*result)[7].Get<uint32>();
            sChallengeDiff->DiffLevelData[difflevel] = level;

        } while (result->NextRow());
    }
    else
    {
        LOG_ERROR("module", "zone_difficulty_level: Query error");
    } 
    //地图基础增强
    if (QueryResult result = WorldDatabase.Query("SELECT mapid,base_hp_pct,base_damage_pct,time_limit,boss_count,lastboss,descp  FROM zone_difficulty_mapbase where status=1"))
    {
        do
        {
            ZoneChallengeBaseEnhance baseEn = {};
            uint32 mapid = (*result)[0].Get<uint32>();
            baseEn.base_hp_pct = (*result)[1].Get<int>();
            baseEn.base_damage_pct = (*result)[2].Get<int>();
            baseEn.time_limit = (*result)[3].Get<uint32>();
            baseEn.boss_count = (*result)[4].Get<uint32>();
            baseEn.lastboss = (*result)[5].Get<uint32>();
            baseEn.descp = (*result)[6].Get<std::string>();
            BaseEnhanceMapData[mapid] = baseEn; 

        } while (result->NextRow());
    }
    else
    {
        LOG_ERROR("module", "zone_difficulty_mapbase: Query error");
    }
    //法术集合
    if (QueryResult result = WorldDatabase.Query("SELECT spell_id,chance,delay,cooldown,triggered_cast  FROM zone_difficulty_spells"))
    {
        do
        {
            uint32 spell_id = (*result)[0].Get<uint32>();
            ZoneChallengeSpell data;
            data.spell_id = spell_id;
            data.chance = (*result)[1].Get<uint8>();;
            data.delay = (*result)[2].Get<Milliseconds>();;
            data.cooldown = (*result)[3].Get<Milliseconds>();;
            data.triggered_cast = (*result)[4].Get<bool>();;
            sChallengeDiff->ZoneChallengeSpellData[spell_id] = data;

        } while (result->NextRow()); 
    }
    else
    {
        LOG_ERROR("module", "zone_difficulty_spells: Query error");
    }
    //法术组合
    if (QueryResult result = WorldDatabase.Query("SELECT gid,spell_id1,spell_id2,spell_id3  FROM zone_difficulty_spell_group"))
    {
        do
        {
            uint32 gid = (*result)[0].Get<uint32>();
            ZoneChallengeSpellGroup group = {};
            group.spellIds[0] = (*result)[1].Get<uint32>();
            group.spellIds[1] = (*result)[2].Get<uint32>();
            group.spellIds[2] = (*result)[3].Get<uint32>();
            sChallengeDiff->ZoneChallengeSpellGroupData[gid] = group;

        } while (result->NextRow()); 
    }
    else
    {
        LOG_ERROR("module", "zone_difficulty_spells: Query error");
    }
    //玩家等级
    if (QueryResult result = CharacterDatabase.Query("SELECT player_guid,challenge_level  FROM zone_diffculty_playerlevel"))
    {
        do
        {
            uint32 player_guid = (*result)[0].Get<uint32>();
            sChallengeDiff->PlayerLevelData[player_guid] = (*result)[1].Get<uint32>();

        } while (result->NextRow()); 
    }
     

    if (QueryResult result = WorldDatabase.Query("SELECT *  FROM zone_difficulty_mythicmode_ai"))
    {
        do
        {
            bool enabled = (*result)[12].Get<bool>();

            if (enabled)
            {
                uint32 creatureEntry = (*result)[0].Get<uint32>();
                ZoneDifficultyHAI data;
                data.Chance = (*result)[1].Get<uint8>();
                data.Spell = (*result)[2].Get<uint32>();
                data.Spellbp0 = (*result)[3].Get<int32>();
                data.Spellbp1 = (*result)[4].Get<int32>();
                data.Spellbp2 = (*result)[5].Get<int32>();
                data.Target = (*result)[6].Get<uint8>();
                data.TargetArg = (*result)[7].Get<int8>();
                data.TargetArg2 = (*result)[8].Get<uint8>();
                data.Delay = (*result)[9].Get<Milliseconds>();
                data.Cooldown = (*result)[10].Get<Milliseconds>();
                data.Repetitions = (*result)[11].Get<uint8>();
                data.TriggeredCast = (*result)[13].Get<bool>();

                if (data.Chance != 0 && data.Spell != 0 && ((data.Target >= 1 && data.Target <= 6) || data.Target == 18))
                {
                    sChallengeDiff->MythicmodeAI[creatureEntry].push_back(data);
                    LOG_INFO("module", "MOD-ZONE-DIFFICULTY: New AI for entry {} with spell {}", creatureEntry, data.Spell);
                }
                
            }
        } while (result->NextRow());
    }
   

}



/**
 *  @brief Sends a whisper to all members of the player's raid in the same instance as the creature.
 *
 *  @param message The message which should be sent to the <Player>.
 *  @param creature The creature who sends the whisper.
 *  @param player The object of the player, whose whole group should receive the message.
 */
void ChallengeDifficulty::SendWhisperToRaid(std::string message, Creature* creature, Player* player)
{
    if (Map* map = creature->GetMap())
    {
        map->DoForAllPlayers([&, player, creature](Player* mapPlayer) {
            if (creature && player)
            {
                if (mapPlayer->IsInSameGroupWith(player))
                {
                    creature->Whisper(message, LANG_UNIVERSAL, mapPlayer);
                }
            }
            });
    }
}

bool ChallengeDifficulty::HasChallengMode(uint32 inst_id)
{
    if (ChallengeInstanceData.find(inst_id) == ChallengeInstanceData.end())
        return false;
    return true;
}

bool ChallengeDifficulty::OpenChallenge(uint32 inst_id, uint32 level, Player* player)
{
    if (ChallengeInstanceData.find(inst_id) != ChallengeInstanceData.end())
        return false;
    auto insScript = player->GetInstanceScript();
    if (!insScript) return false;
     
    auto levelinfo = DiffLevelData.find(level);
    if (levelinfo == DiffLevelData.end()) return false; 
    ZoneChallengeData cdata;
    cdata.level = level;
    cdata.enhance_damage = (*levelinfo).second.enhance;
    cdata.enhance_hp = (*levelinfo).second.enhance;
    cdata.residue_time = 0;
    cdata.kill_boss = 0; 
    if (BaseEnhanceMapData.find(player->GetMapId()) != BaseEnhanceMapData.end()) {
        cdata.enhance_damage = cdata.enhance_damage+ BaseEnhanceMapData[player->GetMapId()].base_damage_pct;
        cdata.enhance_hp = cdata.enhance_hp +BaseEnhanceMapData[player->GetMapId()].base_hp_pct;
        cdata.residue_time = BaseEnhanceMapData[player->GetMapId()].time_limit;
    }
    cdata.apply_spell = { 0, 0, 0 };
    GetRandomGlobalSpell((*levelinfo).second.global_spell_num, &cdata.apply_spell); 
   
    ChallengeInstanceData[inst_id] = cdata;  
    insScript->SetCMode(true);
    insScript->RefreshChallengeBuff();
    insScript->SetTimeLimitMinute(cdata.residue_time);
    CharacterDatabase.Execute("REPLACE INTO zone_difficulty_instance_saves (InstanceID, level,enhance_damage,enhance_hp,residue_time,spell_id1,spell_id2,spell_id3) VALUES ({}, {},{},{}, {}, {}, {}, {})", inst_id, cdata.level, cdata.enhance_damage,cdata.enhance_hp, cdata.residue_time,cdata.apply_spell[0], cdata.apply_spell[1], cdata.apply_spell[2]);
    return true;
}

bool ChallengeDifficulty::CloseChallenge(Map* instance)
{
    auto insScript = instance->IsDungeon() ? instance->ToInstanceMap()->GetInstanceScript() : nullptr;
    if (!insScript) return false;
    if (HasChallengMode(instance->GetInstanceId()))
        ChallengeInstanceData.erase(instance->GetInstanceId());
    insScript->SetCMode(false);
    insScript->RefreshChallengeBuff();
    insScript->SetTimeLimitMinute(0); 
    CharacterDatabase.Execute("DELETE FROM zone_difficulty_instance_saves WHERE InstanceID = {}", instance->GetInstanceId());
    return true;
}

void ChallengeDifficulty::GetRandomGlobalSpell(uint8 count, std::array<uint32, 3>* apply_spell)
{
    if (ZoneChallengeSpellGroupData.size() < 1) return;
    uint32 rand = urand(1, ZoneChallengeSpellGroupData.size()) - 1;
    if (rand < 0) return ;
    auto sd_its = ZoneChallengeSpellGroupData.begin();
    std::advance(sd_its, rand);
    auto group = &(*sd_its).second;
    for (size_t i = 0; i < count; i++)
    {
        (*apply_spell)[i] =group->spellIds[i]; 
    }
}
void ChallengeDifficulty::SetPlayerChallengeLevel(Map* map)
{
    if (!map || !HasChallengMode(map->GetInstanceId()))
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: No object for map in AddMythicmodeScore.");
        return;
    }
    auto cdata = &ChallengeInstanceData[map->GetInstanceId()];
    //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Called AddMythicmodeScore for map id: {} and type: {}", map->GetId(), type);
    Map::PlayerList const& PlayerList = map->GetPlayers();
    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
    {
        uint32 playerid = i->GetSource()->GetGUID().GetCounter();
        if (PlayerLevelData[playerid] >= cdata->level) continue;
        PlayerLevelData[playerid]= PlayerLevelData[playerid]+1;
        CharacterDatabase.Execute("REPLACE INTO zone_diffculty_playerlevel (player_guid,challenge_level) VALUES ({}, {})", playerid,PlayerLevelData[playerid]);
        ChatHandler(i->GetSource()->GetSession()).SendSysMessage("你的挑战等级提升了: %i", PlayerLevelData[playerid]);
    }
}
void ChallengeDifficulty::AddBossScore(Map* map)
{
    if (!IsSendLoot)
        return;
    if (!map || !HasChallengMode(map->GetInstanceId()))
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: No object for map in AddMythicmodeScore.");
        return;
    } 
    auto cdata = &ChallengeInstanceData[map->GetInstanceId()];
    //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Called AddMythicmodeScore for map id: {} and type: {}", map->GetId(), type);
    Map::PlayerList const& PlayerList = map->GetPlayers();
    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
    {
        Player* player = i->GetSource();
        player->AddItem(SCORE_CURRENCY, DiffLevelData[cdata->level].boss_score); 
        //ChatHandler(player->GetSession()).PSendSysMessage("你获得了挑战值: %i", DiffLevelData[cdata->level].boss_score);
        
    }
}

void ChallengeDifficulty::SendChallengLoot(Map* map)
{
    if (!IsSendLoot) return;
    if (!map || !HasChallengMode(map->GetInstanceId()))
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: No object for map in ChallengMode.");
        return;
    }
    auto insScript = map->IsDungeon() ? map->ToInstanceMap()->GetInstanceScript() : nullptr;
    if (!insScript) return ;
    uint32 instId = map->GetInstanceId();
    auto cdata = &ChallengeInstanceData[instId];
    uint32 sendloot = DiffLevelData[cdata->level].award2;
    std::string notice = "恭喜你，已完成 %i 级挑战！";
    //限时完成
    if (insScript->GetTimeLimitMinute() > 0) {
        LOG_ERROR("module", "限时完成 Level:{} killcount:{} mapid:{} dytime:{} .", ChallengeInstanceData[instId].level, ChallengeInstanceData[instId].kill_boss, map->GetMapName(), insScript->GetTimeLimitMinute());
        sendloot= DiffLevelData[cdata->level].award1;
        notice = "恭喜你，已限时完成 %i 级挑战！";
    }
    cdata->is_complete = true;
    CharacterDatabase.Execute("update zone_difficulty_instance_saves set is_complete=1 where InstanceID={} ", instId);
    Map::PlayerList const& PlayerList = map->GetPlayers();
    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
    {
        Player* player = i->GetSource();
        if (!player->AddItem(sendloot, 1)) {
            player->SendItemRetrievalMail(sendloot, 1);
        }
        ChatHandler(player->GetSession()).PSendSysMessage(notice.c_str(), cdata->level);
        
    } 
}
void ChallengeDifficulty::RemoveChallengeAure(Unit* creature) {
    if (!creature || !creature->IsInWorld()) return; 
    RemoveChallengeAureBuff(creature);
    if (creature->GetTypeId() == TYPEID_PLAYER) {

        if (creature->ToPlayer()->HaveBot())
        {
            for (auto const& bitr : *creature->ToPlayer()->GetBotMgr()->GetBotMap())
                if (bitr.second && bitr.second->IsInWorld())
                {
                    RemoveChallengeAureBuff(bitr.second);
                    if (Unit* botpet = bitr.second->GetBotsPet())
                    {
                        RemoveChallengeAureBuff(botpet);
                    }
                }
        } 
    }
   
}
void ChallengeDifficulty::RemoveChallengeAureBuff(Unit* unit) {
    if (!unit) return; 
    Unit::AuraMap const& vAuras = unit->GetOwnedAuras();
    if (vAuras.empty()) return;
   // LOG_ERROR("module", "RemoveChallengeAureBuff: {}",unit->GetName());
    std::list<uint32> removeList;
    for (auto& itr : vAuras)
    {
        if (itr.first > 100000 || itr.first==90010) {
            removeList.push_back(itr.first);
        }
    }
    /*for (Unit::AuraMap::const_iterator itr = vAuras.begin(); itr != vAuras.end(); ++itr)
    { 
        if (itr->first > 100000) {
            removeList.push_back(itr->first);
        }
    }*/
    if (removeList.empty()) return;
    for (std::list<uint32>::const_iterator itr = removeList.begin(); itr != removeList.end(); ++itr)
    {
        unit->RemoveAurasDueToSpell(*itr);
    }
}
void ChallengeDifficulty::ApplyChallengeAure(Unit* creature,uint32 instanceId) {
    if (!creature->IsInWorld()) return;
    bool isplayer = creature->GetTypeId() == TYPEID_PLAYER || creature->IsNPCBot();
    uint32 spellid = isplayer ? SPELL_DEFF_PLAYER : SPELL_ENHANCE_CREATURE;
    if (creature->GetAura(spellid)) return; 
    auto ench = &ChallengeInstanceData[instanceId];
    CustomSpellValues values;
    if (isplayer) {
        uint32 diff_player = DiffLevelData[ench->level].diff_player;
        if (diff_player == 0) return;
        values.AddSpellMod(SPELLVALUE_BASE_POINT0,-1- diff_player);
        values.AddSpellMod(SPELLVALUE_BASE_POINT1,-1- diff_player);
        creature->CastCustomSpell(spellid, values, creature);
        if (creature->IsNPCBot()) return;
        if (creature->ToPlayer()->HaveBot())
        {
            for (auto const& bitr : *creature->ToPlayer()->GetBotMgr()->GetBotMap())
                if (bitr.second && bitr.second->IsInWorld())
                {
                    bitr.second->CastCustomSpell(spellid, values, bitr.second);
                    if (Unit* botpet = bitr.second->GetBotsPet())
                        botpet->CastCustomSpell(spellid, values, botpet);
                }
        }
        return;
    }
    else
    {
        values.AddSpellMod(SPELLVALUE_BASE_POINT0, ench->enhance_hp);
        values.AddSpellMod(SPELLVALUE_BASE_POINT1, ench->enhance_damage);
        values.AddSpellMod(SPELLVALUE_BASE_POINT2, ench->enhance_damage);
        creature->CastCustomSpell(spellid, values, creature);
    }
    for (uint8 i = 0; i < 3; i++)
    {
        uint32 spellid = ChallengeInstanceData[instanceId].apply_spell[i];
        if (spellid <= 0) continue;
        //LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Spell:{} chance:{}", spellid, sChallengeDiff->ZoneChallengeSpellData[spellid].chance); 
        if (urand(1, 100) <= ZoneChallengeSpellData[spellid].chance) {
            creature->CastSpell(creature, spellid, true);
            // LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Spell:{} chance:{}", spellid, chance);
        }
    }
}

void ChallengeDifficulty::CheckUpdateActiveMap()
{
    auto now = std::chrono::system_clock::now();  
    std::time_t now_time = std::chrono::system_clock::to_time_t(now); 
    // 转换为 tm 结构体以获取年月日  
    std::tm* local_time = std::localtime(&now_time);
    if (local_time->tm_hour != 7) return;
    uint32 dayticke=  (local_time->tm_mon + 1)*100 + local_time->tm_mday ;
    if (DayActiveMaps.mdayticker == dayticke) return;
    DayActiveMaps.mdayticker = dayticke;
    uint32 rand = urand(1, BaseEnhanceMapData.size()) - 1;
    if (rand < 0) return;
    auto map_its = BaseEnhanceMapData.begin();
    std::advance(map_its, rand);
    int countMap = 0;
    do {
        DayActiveMaps.active_mapid[countMap] = (*map_its).first;
        ++countMap;
        ++map_its;
        if (map_its == BaseEnhanceMapData.end()) {
            map_its = BaseEnhanceMapData.begin();
        }
    } while (countMap<=4);
    LOG_ERROR("module", " {} ActiveMap:{},{},{},{}", DayActiveMaps.mdayticker, DayActiveMaps.active_mapid[0], DayActiveMaps.active_mapid[1], DayActiveMaps.active_mapid[2], DayActiveMaps.active_mapid[3]);
    CharacterDatabase.Execute("update zone_diffculty_activemap set mdayticker={},active_mapid1={},active_mapid2={},active_mapid3={},active_mapid4={} where id=1", DayActiveMaps.mdayticker, DayActiveMaps.active_mapid[0], DayActiveMaps.active_mapid[1], DayActiveMaps.active_mapid[2], DayActiveMaps.active_mapid[3]);
}

bool ChallengeDifficulty::MapIsActive(uint32 mapId)
{
    for (uint8 i = 0; i < 4; i++)
    {
        if (DayActiveMaps.active_mapid[i] == mapId)
            return true;
    }
    return false;
}
