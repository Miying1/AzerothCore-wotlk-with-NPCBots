/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "Config.h"
#include "Chat.h"
#include "GameTime.h"
#include "ItemTemplate.h"
#include "InstanceSaveMgr.h"
#include "Log.h"
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
#include "ObjectAccessor.h"
#include "ChallengeDifficulty.h"
#include "botmgr.h"

#include <algorithm>

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
    sChallengeDiff->DiffLevelData.clear();
    sChallengeDiff->BaseEnhanceMapData.clear();
    sChallengeDiff->ChallengeInstanceData.clear();
    std::vector<bool> instanceIDs = sMapMgr->GetInstanceIDs();

    std::vector<std::pair<uint32, ZoneChallengeData>> savedChallenges;
    if (QueryResult result = CharacterDatabase.Query("SELECT InstanceID,level,enhance_damage,enhance_hp,kill_boss,residue_time,spell_id1,spell_id2,spell_id3,is_complete,last_boss_killed,completed_encounters FROM zone_difficulty_instance_saves"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 InstanceId = fields[0].Get<uint32>();

            if (InstanceId < instanceIDs.size() && instanceIDs[InstanceId])
            {
                ZoneChallengeData cdata;
                cdata.level = fields[1].Get<uint32>();
                cdata.enhance_damage = fields[2].Get<uint32>();
                cdata.enhance_hp = fields[3].Get<uint32>();
                cdata.kill_boss = fields[4].Get<uint32>();
                cdata.residue_time = fields[5].Get<uint32>();
                cdata.apply_spell[0] = fields[6].Get<uint32>();
                cdata.apply_spell[1] = fields[7].Get<uint32>();
                cdata.apply_spell[2] = fields[8].Get<uint32>();
                cdata.is_complete = fields[9].Get<bool>();
                cdata.last_boss_killed = fields[10].Get<bool>();
                std::string completedEncounters = fields[11].Get<std::string>();
                for (std::string_view encounter : Acore::Tokenize(completedEncounters, ',', true))
                    cdata.completed_encounters.insert(Acore::StringTo<uint32>(encounter).value_or(0));
                cdata.completed_encounters.erase(0);
                savedChallenges.emplace_back(InstanceId, std::move(cdata));
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

    for (auto& [instanceId, challengeData] : savedChallenges)
    {
        InstanceSave* instanceSave = sInstanceSaveMgr->GetInstanceSave(instanceId);
        if (!instanceSave || !IsChallengeMap(instanceSave->GetMapId()))
        {
            CharacterDatabase.Execute("DELETE FROM zone_difficulty_instance_saves WHERE InstanceID = {}", instanceId);
            continue;
        }
        ChallengeInstanceData[instanceId] = std::move(challengeData);
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
    return IsEnabled && ChallengeInstanceData.find(inst_id) != ChallengeInstanceData.end();
}

bool ChallengeDifficulty::OpenChallenge(uint32 inst_id, uint32 level, Player* player)
{
    if (!IsEnabled || !player || !player->GetMap() || inst_id == 0 || inst_id != player->GetMap()->GetInstanceId()
        || !player->GetMap()->IsDungeon() || !player->GetMap()->IsHeroic() || player->GetMap()->IsRaid()
        || !IsChallengeMap(player->GetMapId()) || ChallengeInstanceData.find(inst_id) != ChallengeInstanceData.end())
        return false;

    auto insScript = player->GetInstanceScript();
    if (!insScript || insScript->IsEncounterInProgress())
        return false;

    for (uint32 bossId = 0; bossId < insScript->GetEncounterCount(); ++bossId)
        if (insScript->GetBossState(bossId) == DONE)
            return false;

    auto levelinfo = DiffLevelData.find(level);
    if (levelinfo == DiffLevelData.end())
        return false;
    ZoneChallengeData cdata;
    cdata.level = level;
    cdata.enhance_damage = levelinfo->second.enhance;
    cdata.enhance_hp = levelinfo->second.enhance;
    cdata.residue_time = 0;
    cdata.kill_boss = 0;
    auto baseEnhance = BaseEnhanceMapData.find(player->GetMapId());
    if (baseEnhance != BaseEnhanceMapData.end())
    {
        // base_damage_pct / base_hp_pct 可能为负数，必须用有符号类型累加，
        // 避免 uint32 与负数相加发生无符号溢出，从而写入超出列范围的超大值
        int32 damage = static_cast<int32>(cdata.enhance_damage) + baseEnhance->second.base_damage_pct;
        int32 hp = static_cast<int32>(cdata.enhance_hp) + baseEnhance->second.base_hp_pct;
        cdata.enhance_damage = damage > 0 ? static_cast<uint32>(damage) : 0;
        cdata.enhance_hp = hp > 0 ? static_cast<uint32>(hp) : 0;
        cdata.residue_time = baseEnhance->second.time_limit;
    }
    cdata.apply_spell = { 0, 0, 0 };
    GetRandomGlobalSpell(levelinfo->second.global_spell_num, &cdata.apply_spell);

    ChallengeInstanceData[inst_id] = cdata;
    SaveChallengeData(inst_id);
    insScript->SetCMode(true);
    insScript->RefreshChallengeBuff();
    insScript->SetTimeLimitMinute(cdata.residue_time);
    return true;
}

bool ChallengeDifficulty::CloseChallenge(Map* instance)
{
    if (!instance || !instance->IsDungeon() || !instance->IsHeroic() || instance->IsRaid()
        || !HasChallengMode(instance->GetInstanceId()))
        return false;

    auto insScript = instance->ToInstanceMap()->GetInstanceScript();
    if (!insScript || insScript->IsEncounterInProgress())
        return false;

    ChallengeInstanceData.erase(instance->GetInstanceId());
    insScript->SetCMode(false);
    insScript->RefreshChallengeBuff();
    insScript->SetTimeLimitMinute(0);
    CharacterDatabase.Execute("DELETE FROM zone_difficulty_instance_saves WHERE InstanceID = {}", instance->GetInstanceId());
    return true;
}

void ChallengeDifficulty::GetRandomGlobalSpell(uint8 count, std::array<uint32, 3>* apply_spell)
{
    if (!apply_spell || ZoneChallengeSpellGroupData.empty())
        return;

    uint32 const randomIndex = urand(0, static_cast<uint32>(ZoneChallengeSpellGroupData.size() - 1));
    auto spellGroup = ZoneChallengeSpellGroupData.begin();
    std::advance(spellGroup, randomIndex);
    size_t const spellCount = std::min<size_t>(count, apply_spell->size());
    for (size_t i = 0; i < spellCount; ++i)
        (*apply_spell)[i] = spellGroup->second.spellIds[i];
}

bool ChallengeDifficulty::IsChallengeMap(uint32 mapId) const
{
    return BaseEnhanceMapData.find(mapId) != BaseEnhanceMapData.end();
}

void ChallengeDifficulty::TrackParticipant(Map* map, Player* player)
{
    if (!map || !player)
        return;

    auto data = ChallengeInstanceData.find(map->GetInstanceId());
    if (data != ChallengeInstanceData.end())
        data->second.participant_guids.insert(player->GetGUID());
}

void ChallengeDifficulty::SaveChallengeData(uint32 instanceId)
{
    auto data = ChallengeInstanceData.find(instanceId);
    if (data == ChallengeInstanceData.end())
        return;

    std::string completedEncounters;
    for (uint32 encounter : data->second.completed_encounters)
    {
        if (!completedEncounters.empty())
            completedEncounters += ',';
        completedEncounters += std::to_string(encounter);
    }

    CharacterDatabase.EscapeString(completedEncounters);
    CharacterDatabase.Execute("REPLACE INTO zone_difficulty_instance_saves (InstanceID, level, enhance_damage, enhance_hp, kill_boss, residue_time, spell_id1, spell_id2, spell_id3, is_complete, last_boss_killed, completed_encounters) VALUES ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, '{}')",
        instanceId, data->second.level, data->second.enhance_damage, data->second.enhance_hp, data->second.kill_boss,
        data->second.residue_time, data->second.apply_spell[0], data->second.apply_spell[1], data->second.apply_spell[2],
        data->second.is_complete, data->second.last_boss_killed, completedEncounters);
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
    for (ObjectGuid const& guid : cdata->participant_guids)
    {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player || player->GetMap() != map)
            continue;

        uint32 playerid = guid.GetCounter();
        if (PlayerLevelData[playerid] >= cdata->level)
            continue;
        PlayerLevelData[playerid] = cdata->level;
        CharacterDatabase.Execute("REPLACE INTO zone_diffculty_playerlevel (player_guid,challenge_level) VALUES ({}, {})", playerid, PlayerLevelData[playerid]);
        ChatHandler(player->GetSession()).SendSysMessage("你的挑战等级提升了,当前: " + std::to_string(PlayerLevelData[playerid]));
    }
}
void ChallengeDifficulty::AddBossScore(Map* map)
{
    if (!map || !HasChallengMode(map->GetInstanceId()))
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: No object for map in AddMythicmodeScore.");
        return;
    }
    auto cdata = &ChallengeInstanceData[map->GetInstanceId()];
    auto levelData = DiffLevelData.find(cdata->level);
    if (levelData == DiffLevelData.end())
        return;
    //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Called AddMythicmodeScore for map id: {} and type: {}", map->GetId(), type);
    std::vector<Player*> eligiblePlayers;
    Map::PlayerList const& PlayerList = map->GetPlayers();
    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
    {
        Player* player = i->GetSource();
        if (!player || player->IsGameMaster())
            continue;
        eligiblePlayers.push_back(player);
    }

    for (Player* player : eligiblePlayers)
    {
        TrackParticipant(map, player);
        if (!IsSendLoot)
            continue;
        player->AddItem(SCORE_CURRENCY, levelData->second.boss_score);
        //ChatHandler(player->GetSession()).PSendSysMessage("你获得了挑战值: %i", DiffLevelData[cdata->level].boss_score);
    }
}

void ChallengeDifficulty::SendChallengLoot(Map* map)
{
    if (!IsSendLoot)
    {
        if (map && HasChallengMode(map->GetInstanceId()))
        {
            ChallengeInstanceData[map->GetInstanceId()].is_complete = true;
            SaveChallengeData(map->GetInstanceId());
        }
        return;
    }
    if (!map || !HasChallengMode(map->GetInstanceId()))
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: No object for map in ChallengMode.");
        return;
    }
    auto insScript = map->IsDungeon() ? map->ToInstanceMap()->GetInstanceScript() : nullptr;
    if (!insScript)
        return;
    uint32 instId = map->GetInstanceId();
    auto data = ChallengeInstanceData.find(instId);
    if (data == ChallengeInstanceData.end())
        return;
    auto cdata = &data->second;
    auto levelData = DiffLevelData.find(cdata->level);
    if (levelData == DiffLevelData.end())
        return;

    uint32 sendloot = levelData->second.award2;
    std::string notice = "恭喜你，已完成 %i 级挑战！";
    //限时完成
    if (insScript->GetTimeLimitMinute() > 0)
    { 
        sendloot = levelData->second.award1;
        notice = "恭喜你，已限时完成 %i 级挑战！";
    }
    cdata->is_complete = true;
    SaveChallengeData(instId);
    for (ObjectGuid const& guid : cdata->participant_guids)
    {
        Player* player = ObjectAccessor::FindPlayer(guid);
        if (!player || player->GetMap() != map)
            continue;

        if (!player->AddItem(sendloot, 1))
            player->SendItemRetrievalMail(sendloot, 1);
        LOG_ERROR("module", "{} 限时完成 Level:{} killcount:{} mapid:{} dytime:{} .", player->GetName(), cdata->level,
            cdata->kill_boss, map->GetMapName(), insScript->GetTimeLimitMinute());
        ChatHandler(player->GetSession()).PSendSysMessage(notice.c_str(), cdata->level);
    }
}
void ChallengeDifficulty::RemoveChallengeAure(Unit* creature) {
    if (!creature || !creature->IsInWorld()) return;
    RemoveChallengeAureBuff(creature);
    if (creature->IsPlayer()) {

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
void ChallengeDifficulty::RemoveChallengeAureBuff(Unit* unit)
{
    if (!unit)
        return;

    Unit::AuraMap const& vAuras = unit->GetOwnedAuras();
    if (vAuras.empty()) return;
   // LOG_ERROR("module", "RemoveChallengeAureBuff: {}",unit->GetName());
    std::list<uint32> removeList;
    for (auto& itr : vAuras)
    {
        bool const isChallengeAffix = std::any_of(ZoneChallengeSpellGroupData.begin(), ZoneChallengeSpellGroupData.end(), [&itr](auto const& group)
        {
            return std::find(group.second.spellIds.begin(), group.second.spellIds.end(), itr.first) != group.second.spellIds.end();
        });
        if (itr.first == SPELL_ENHANCE_CREATURE || itr.first == SPELL_DEFF_PLAYER || itr.first == 90010 || isChallengeAffix)
            removeList.push_back(itr.first);
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
void ChallengeDifficulty::ApplyChallengeAure(Unit* creature, uint32 instanceId)
{
    if (!IsEnabled || !creature || !creature->IsInWorld())
        return;

    auto challenge = ChallengeInstanceData.find(instanceId);
    if (challenge == ChallengeInstanceData.end())
        return;

    bool isplayer = creature->IsPlayer() || creature->IsNPCBot();
    uint32 spellid = isplayer ? SPELL_DEFF_PLAYER : SPELL_ENHANCE_CREATURE;
    if (creature->GetAura(spellid))
        return;
    auto ench = &challenge->second;
    CustomSpellValues values;
    if (isplayer) {
        auto levelData = DiffLevelData.find(ench->level);
        if (levelData == DiffLevelData.end())
            return;
        uint32 diff_player = levelData->second.diff_player;
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
    auto instanceData = ChallengeInstanceData.find(instanceId);
    if (instanceData == ChallengeInstanceData.end())
        return;

    for (uint8 i = 0; i < instanceData->second.apply_spell.size(); ++i)
    {
        uint32 spellid = instanceData->second.apply_spell[i];
        auto spellData = ZoneChallengeSpellData.find(spellid);
        if (spellid == 0 || spellData == ZoneChallengeSpellData.end())
            continue;

        if (urand(1, 100) <= spellData->second.chance) {
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
    if (local_time->tm_hour != 8) return;
    uint32 dayticke=  (local_time->tm_mon + 1)*100 + local_time->tm_mday ;
    if (DayActiveMaps.mdayticker == dayticke) return;
    if (BaseEnhanceMapData.empty())
        return;

    DayActiveMaps.mdayticker = dayticke;
    uint32 const randomIndex = urand(0, static_cast<uint32>(BaseEnhanceMapData.size() - 1));
    auto map_its = BaseEnhanceMapData.begin();
    std::advance(map_its, randomIndex);
    DayActiveMaps.active_mapid.fill(0);
    for (size_t countMap = 0; countMap < DayActiveMaps.active_mapid.size(); ++countMap)
    {
        DayActiveMaps.active_mapid[countMap] = map_its->first;
        ++map_its;
        if (map_its == BaseEnhanceMapData.end())
            map_its = BaseEnhanceMapData.begin();
    }
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
