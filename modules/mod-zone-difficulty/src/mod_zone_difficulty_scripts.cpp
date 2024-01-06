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

 
 
class mod_zone_difficulty_worldscript : public WorldScript
{
public:
    mod_zone_difficulty_worldscript() : WorldScript("mod_zone_difficulty_worldscript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sChallengeDiff->IsEnabled = sConfigMgr->GetOption<bool>("ModZoneDifficulty.Enable", false);
        sChallengeDiff->IsDebugInfoEnabled = sConfigMgr->GetOption<bool>("ModZoneDifficulty.DebugInfo", false); 
       
        sChallengeDiff->LoadMapDifficultySettings();
    }

    void OnStartup() override
    {
        sChallengeDiff->LoadMythicmodeInstanceData(); 
    }
};

class mod_zone_difficulty_globalscript : public GlobalScript
{
public:
    mod_zone_difficulty_globalscript() : GlobalScript("mod_zone_difficulty_globalscript") { }
     
    void OnInstanceIdRemoved(uint32 instanceId) override
    {
        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnInstanceIdRemoved: instanceId = {}", instanceId);
        if (sChallengeDiff->ChallengeInstanceData.find(instanceId) != sChallengeDiff->ChallengeInstanceData.end())
        {
            sChallengeDiff->ChallengeInstanceData.erase(instanceId);
        }

        CharacterDatabase.Execute("DELETE FROM zone_difficulty_instance_saves WHERE InstanceID = {};", instanceId);
    } 
};
 

class mod_zone_difficulty_dungeonmaster : public CreatureScript
{
public:
    mod_zone_difficulty_dungeonmaster() : CreatureScript("mod_zone_difficulty_dungeonmaster") { }

    struct mod_zone_difficulty_dungeonmasterAI : public ScriptedAI
    {
        mod_zone_difficulty_dungeonmasterAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _scheduler.CancelAll();
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: mod_zone_difficulty_dungeonmasterAI: Reset happens.");
            if (me->GetMap() && me->GetMap()->IsHeroic())
            {
                if (!sChallengeDiff->IsEnabled)
                {
                    return;
                }
                 
                _scheduler.Schedule(6s, [this](TaskContext /*context*/)
                    {
                        me->Yell("如果你想开启挑战， 请尽快和我交谈，冒险者!", LANG_UNIVERSAL);
                    });
                _scheduler.Schedule(55s, [this](TaskContext /*context*/)
                    {
                        me->Yell("再见冒险者!", LANG_UNIVERSAL);
                    });
                /*_scheduler.Schedule(60s, [this](TaskContext context)
                    {
                        me->DespawnOrUnsummon();
                    });  */
            }
            else
            {
                me->DespawnOrUnsummon();
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new mod_zone_difficulty_dungeonmasterAI(creature);
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        uint32 instanceId = player->GetMap()->GetInstanceId();
       
        if (action == 99)
        {
            player->PlayerTalkClass->ClearMenus(); 
            
            for (auto& _levelits : sChallengeDiff->DiffLevelData)
            {
                std::ostringstream str;
                str << "开启 " << _levelits.first << "级挑战";
                AddGossipItemFor(player, GOSSIP_ICON_BATTLE, str.str(), _levelits.first, 100); 
            } 
            SendGossipMenuFor(player, 61002, creature);
        }
        else if (action == 100)
        {
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Try turn on");
            bool canTurnOn = true;
            uint32 playerLevel = sChallengeDiff->PlayerLevelData[player->GetGUID().GetCounter()];
           /* if (playerLevel + 1 < sender) {
                canTurnOn = false;
                creature->Whisper("冒险者，你的挑战等级还不足以开启这项挑战！", LANG_UNIVERSAL, player);
                return OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 99);
            }*/
            // Forbid turning Mythicmode on ...
            // ...if a single encounter was completed on normal mode
            if (sChallengeDiff->ChallengeInstanceData.find(instanceId) != sChallengeDiff->ChallengeInstanceData.end())
            {
                if (player->GetInstanceScript()->GetBossState(0) == DONE)
                {
                    //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Mythicmode is not Possible for instanceId {}", instanceId);
                    canTurnOn = false;
                    creature->Whisper("对不起，冒险者，因为你已经完成了一部分，你已经不能在开启挑战了！", LANG_UNIVERSAL, player);
                }
            }
            // ... if there is an encounter in progress
            if (player->GetInstanceScript()->IsEncounterInProgress())
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: IsEncounterInProgress");
                canTurnOn = false;
                creature->Whisper("现在正有一场战斗进行中，不能开启挑战！", LANG_UNIVERSAL, player);
            } 
            if (canTurnOn)
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Turn on Mythicmode for id {}", instanceId);
                sChallengeDiff->OpenChallenge(instanceId, sender, player); 
                sChallengeDiff->SendWhisperToRaid("开始冒险吧，我已经为你们开启了挑战！", creature, player);
            }

            CloseGossipMenuFor(player);
        }
        else if (action == 101)
        {
            if (player->GetInstanceScript()->IsEncounterInProgress())
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: IsEncounterInProgress");
                creature->Whisper("现在正有一场战斗进行中，不能结束挑战.", LANG_UNIVERSAL, player); 
            }
            if (sChallengeDiff->HasChallengMode(instanceId))
            {
                //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: Turn off Mythicmode for id {}", instanceId);
                sChallengeDiff->CloseChallenge(instanceId,player); 
                sChallengeDiff->SendWhisperToRaid("现在已经变为了正常模式了，再见!", creature, player); 
                creature->DespawnOrUnsummon(2000);
            }
            CloseGossipMenuFor(player);
        }
         

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnGossipHelloChromie");
        Group* group = player->GetGroup();
        if (group && group->IsLfgRandomInstance() && !player->GetMap()->IsRaid())
        {
            creature->Whisper("对不起，冒险者!你不能在随机地下城中开启挑战！", LANG_UNIVERSAL, player);
            return true;
        }
        if (!group)
        {
            creature->Whisper("我现在不能帮你开启挑战, 这太过危险，你可以召集你的朋友组队一起来挑战。", LANG_UNIVERSAL, player);
            return true;
        }

        uint32 npcText = 61000;
        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnGossipHello Has Group");
        if ((group && group->IsLeader(player->GetGUID())))
        {
            uint32 instanceId = player->GetMap()->GetInstanceId();
            LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: OnGossipHello Is Leader");
            if (!sChallengeDiff->HasChallengMode(instanceId))
            {
                npcText = 61000;
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我想开启挑战模式", GOSSIP_SENDER_MAIN, 99);
                if (sChallengeDiff->PlayerLevelData[player->GetGUID().GetCounter()]) {
                    std::ostringstream str;
                    str << "你当前的挑战等级为：" << sChallengeDiff->PlayerLevelData[player->GetGUID().GetCounter()] << "级";
                    creature->Whisper(str.str(), LANG_UNIVERSAL, player);
                }
              
            }
            else
            {
                auto data= sChallengeDiff->ChallengeInstanceData.find(instanceId);
                 LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: IsOpened level:{} spell1:{}", (*data).second.level, (*data).second.apply_spell[0]);
                npcText = 61001;
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我想关闭挑战模式", GOSSIP_SENDER_MAIN, 101); 
            }  
        }
        else
        {
            creature->Whisper("需要让你们队长来做出这个决定。当挑战开启时，你会收到通知的。", LANG_UNIVERSAL, player);
        }
        SendGossipMenuFor(player, npcText, creature);
        return true;
    }
};
 
// Add all scripts in one
void AddModZoneDifficultyScripts()
{ 
    new mod_zone_difficulty_worldscript();
    new mod_zone_difficulty_globalscript(); 
    new mod_zone_difficulty_dungeonmaster(); 
}
