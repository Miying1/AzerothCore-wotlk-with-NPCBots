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
#include "TaskScheduler.h"
#include "Tokenize.h"
#include "Unit.h"
#include "ChallengeDifficulty.h"

ChallengeDifficulty* ChallengeDifficulty::instance()
{
    static ChallengeDifficulty instance;
    return &instance;
}

void ChallengeDifficulty::LoadMapDifficultySettings()
{
    if (!sChallengeDiff->IsEnabled)
    {
        return;
    }

    sChallengeDiff->MythicmodeAI.clear();
    sChallengeDiff->DisallowedBuffs.clear();
    sChallengeDiff->DiffLevelData.clear();
    sChallengeDiff->ZoneChallengeSpellData.clear();
    sChallengeDiff->ZoneChallengeSpellGroupData.clear();
    sChallengeDiff->PlayerLevelData.clear();
    //挑战难度等级
    if (QueryResult result = WorldDatabase.Query("SELECT difflevel,enhance,global_spell_num FROM zone_difficulty_level"))
    {
        do
        {
            uint32 difflevel = (*result)[0].Get<uint32>();
            ZoneDifficultyLevel level;
            level.difflevel = difflevel;
            level.enhance = (*result)[1].Get<uint32>();
            level.global_spell_num = (*result)[2].Get<uint32>();
            sChallengeDiff->DiffLevelData[difflevel] = level;

        } while (result->NextRow());
    }
    //法术集合
    if (QueryResult result = WorldDatabase.Query("SELECT spell_id,chance,delay,cooldown,triggered_cast FROM zone_difficulty_spells"))
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
    //法术组合
    if (QueryResult result = WorldDatabase.Query("SELECT gid,spell_id1,spell_id2,spell_id3 FROM zone_difficulty_spell_group"))
    {
        do
        {
            uint32 gid = (*result)[0].Get<uint32>(); 
            ZoneChallengeSpellGroup group = {};
            group.spellIds[0]= (*result)[1].Get<uint32>();
            group.spellIds[1] = (*result)[2].Get<uint32>();
            group.spellIds[2] = (*result)[3].Get<uint32>(); 
            sChallengeDiff->ZoneChallengeSpellGroupData[gid] = group;

        } while (result->NextRow());
    }
    //玩家等级
    if (QueryResult result = CharacterDatabase.Query("SELECT player_guid,challenge_level FROM zone_diffculty_playerlevel"))
    {
        do
        {
            uint32 player_guid = (*result)[0].Get<uint32>(); 
            sChallengeDiff->PlayerLevelData[player_guid] = (*result)[1].Get<uint32>();;

        } while (result->NextRow());
    }
    if (QueryResult result = WorldDatabase.Query("SELECT * FROM zone_difficulty_disallowed_buffs"))
    {
        do
        {
            std::vector<uint32> debuffs;
            uint32 mapId;
            if ((*result)[2].Get<bool>())
            {
                std::string spellString = (*result)[1].Get<std::string>();
                std::vector<std::string_view> tokens = Acore::Tokenize(spellString, ' ', false);

                mapId = (*result)[0].Get<uint32>();
                for (auto token : tokens)
                {
                    if (token.empty())
                    {
                        continue;
                    }

                    uint32 spell;
                    if ((spell = Acore::StringTo<uint32>(token).value()))
                    {
                        debuffs.push_back(spell);
                    }
                    else
                    {
                        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Disabling buffs for spell '{}' is invalid, skipped.", spell);
                    }
                }
                sChallengeDiff->DisallowedBuffs[mapId] = debuffs;
            }
        } while (result->NextRow());
    }


    if (QueryResult result = WorldDatabase.Query("SELECT * FROM zone_difficulty_mythicmode_ai"))
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
                else
                {
                    LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Unknown type for `Target`: {} in zone_difficulty_mythicmode_ai", data.Target);
                }
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: New creature with entry: {} has exception for hp: {}", creatureEntry, hpModifier);
            }
        } while (result->NextRow());
    }
    else
    {
        LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: Query failed: SELECT * FROM zone_difficulty_mythicmode_ai");
    }

}

/**
 *  @brief Loads the MythicmodeInstanceData from the database. Fetch from zone_difficulty_instance_saves.
 *
 *  `InstanceID` INT NOT NULL DEFAULT 0,
 *  `MythicmodeOn` TINYINT NOT NULL DEFAULT 0,
 *
 *  Exclude data not in the IDs stored in GetInstanceIDs() and delete
 *  zone_difficulty_instance_saves for instances that no longer exist.
 */
void ChallengeDifficulty::LoadMythicmodeInstanceData()
{
    std::vector<bool> instanceIDs = sMapMgr->GetInstanceIDs();

    if (QueryResult result = CharacterDatabase.Query("SELECT InstanceID,level,enhance,spell_id1,spell_id2,spell_id3 FROM zone_difficulty_instance_saves"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 InstanceId = fields[0].Get<uint32>();

            if (instanceIDs[InstanceId])
            {
                ZoneChallengeData cdata;
                cdata.level = fields[1].Get<uint32>();
                cdata.enhance = fields[2].Get<uint32>();
                cdata.apply_spell[0] = fields[3].Get<uint32>();
                cdata.apply_spell[1] = fields[4].Get<uint32>();
                cdata.apply_spell[2] = fields[5].Get<uint32>();
                sChallengeDiff->ChallengeInstanceData[InstanceId] = cdata;
            }
            else
            {
                CharacterDatabase.Execute("DELETE FROM zone_difficulty_instance_saves WHERE InstanceID = {}", InstanceId);
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
    cdata.enhance = (*levelinfo).second.enhance;
    cdata.apply_spell = { 0, 0, 0 };
    GetRandomGlobalSpell((*levelinfo).second.global_spell_num, &cdata.apply_spell); 
   
    ChallengeInstanceData[inst_id] = cdata; 
    insScript->CheckChallengeMode();
    CharacterDatabase.Execute("REPLACE INTO zone_difficulty_instance_saves (InstanceID, level,enhance,spell_id1,spell_id2,spell_id3) VALUES ({}, {}, {}, {}, {}, {})", inst_id, cdata.level, cdata.enhance, cdata.apply_spell[0], cdata.apply_spell[1], cdata.apply_spell[2]);
    return true;
}

bool ChallengeDifficulty::CloseChallenge(uint32 inst_id, Player* player)
{
    auto insScript = player->GetInstanceScript();
    if (!insScript) return false;
    ChallengeInstanceData.erase(inst_id); 
    insScript->CheckChallengeMode();
    CharacterDatabase.Execute("DELETE FROM zone_difficulty_instance_saves WHERE InstanceID = {}", inst_id);
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


