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
#include "SpellScript.h"
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
        sChallengeDiff->IsSendLoot = sConfigMgr->GetOption<bool>("ModZoneDifficulty.IsSendLoot", true); 

    }

    void OnStartup() override
    { 
        sChallengeDiff->LoadIntiData();
    }
};

class mod_zone_difficulty_globalscript : public GlobalScript
{
public:
    mod_zone_difficulty_globalscript() : GlobalScript("mod_zone_difficulty_globalscript") { }

    //void OnBeforeSetBossState(uint32 id, EncounterState newState, EncounterState oldState, Map* instance) override
    //{

    //    LOG_ERROR("module", "MOD-ZONE-DIFFICULTY: EncounterState {}" , newState);
    //    uint32 instanceId = instance->GetInstanceId();
    //    if (!instance->IsHeroic() || !sChallengeDiff->HasChallengMode(instanceId))
    //    {
    //        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnBeforeSetBossState: Instance not handled because there is no Mythicmode loot data for map id: {}", instance->GetId());
    //        return;
    //    }
    //    if (oldState != IN_PROGRESS && newState == IN_PROGRESS)
    //    { 
    //        sChallengeDiff->EncountersInProgress[instanceId] = GameTime::GetGameTime().count();
    //       
    //    }
    //    else if (oldState == IN_PROGRESS && newState == DONE)
    //    { 
    //        LOG_ERROR("module", "BOSS State DONE :{}.");
    //       /* if (sChallengeDiff->EncountersInProgress.find(instanceId) != sChallengeDiff->EncountersInProgress.end() && sChallengeDiff->EncountersInProgress[instanceId] != 0)
    //        {
    //            sChallengeDiff->AddBossScore(instance);
    //        }*/ 
    //    }
    //}

    void OnAfterUpdateEncounterState(Map* map, EncounterCreditType /*type*/, uint32 /*creditEntry*/, Unit* source, Difficulty /*difficulty_fixed*/, DungeonEncounterList const* /*encounters*/, uint32  dungeonCompleted, bool /*updated*/) override
    {

        if (!source || !source->ToCreature())
        {
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: source is a nullptr in OnAfterUpdateEncounterState");
            return;
        }
        uint32 instId = map->GetInstanceId();
        if (sChallengeDiff->HasChallengMode(instId))
        {
            uint32 mapid = map->GetId();
            auto instanceScript = map->ToInstanceMap()->GetInstanceScript();
            if (sChallengeDiff->ChallengeInstanceData[instId].is_complete) return;
            sChallengeDiff->AddBossScore(map);
            sChallengeDiff->ChallengeInstanceData[instId].kill_boss++;
            CharacterDatabase.Execute("update zone_difficulty_instance_saves set kill_boss={},residue_time={} where InstanceID={} ", sChallengeDiff->ChallengeInstanceData[instId].kill_boss, instanceScript->GetTimeLimitMinute(), instId);
           // LOG_ERROR("module", "BOSS:{}({}) killcount:{} map:{} .", source->GetName(), source->GetEntry(), sChallengeDiff->ChallengeInstanceData[instId].kill_boss, map->GetMapName());
            uint32 lastboss = sChallengeDiff->BaseEnhanceMapData[mapid].lastboss;
            bool iskilledfinsh = sChallengeDiff->ChallengeInstanceData[instId].kill_boss >= sChallengeDiff->BaseEnhanceMapData[mapid].boss_count;
            if (lastboss == source->GetEntry())
                sChallengeDiff->ChallengeInstanceData[instId].last_boss_killed = true;
            if ((lastboss == 0 && iskilledfinsh)
                || (sChallengeDiff->ChallengeInstanceData[instId].last_boss_killed && iskilledfinsh)) {
                sChallengeDiff->SetPlayerChallengeLevel(map);
                sChallengeDiff->SendChallengLoot(map); //完成 
                //sChallengeDiff->CloseChallenge(map);
            }

        }
    }
    void OnInstanceIdRemoved(uint32 instanceId) override
    {
        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnInstanceIdRemoved: instanceId = {}", instanceId);
        if (sChallengeDiff->ChallengeInstanceData.find(instanceId) != sChallengeDiff->ChallengeInstanceData.end())
        {
            sChallengeDiff->ChallengeInstanceData.erase(instanceId);
            CharacterDatabase.Execute("DELETE FROM zone_difficulty_instance_saves WHERE InstanceID = {};", instanceId);
        }
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
            if (!sChallengeDiff->MapIsActive(me->GetMap()->GetId())) {
                me->SetVisible(false);
                return;
            }
            else
            {
                if (sChallengeDiff->HasChallengMode(me->GetMap()->GetInstanceId())) {
                    me->SetVisible(false);
                    return;
                }
                me->SetVisible(true);
            }
          
            //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: mod_zone_difficulty_dungeonmasterAI: Reset happens.");
            if (me->GetMap() && me->GetMap()->IsHeroic())
            {
                if (!sChallengeDiff->IsEnabled)
                {
                    return;
                }
                me->SetVisible(true);
                _scheduler.Schedule(4s, [this](TaskContext /*context*/)
                    {
                        me->Yell("如果你想开启挑战， 请尽快和我交谈，冒险者!", LANG_UNIVERSAL);
                    });
                _scheduler.Schedule(85s, [this](TaskContext /*context*/)
                    {
                       /* if (sChallengeDiff->HasChallengMode(me->GetInstanceId())) {
                            _scheduler.CancelAll();
                            return;
                        }*/
                        me->Yell("再见冒险者!", LANG_UNIVERSAL);
                    });
                _scheduler.Schedule(90s, [this](TaskContext context)
                    {
                        //me->DespawnOrUnsummon();
                        me->SetVisible(false);
                    });
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
            if (!player->IsGameMaster() && playerLevel + 1 < sender) {
                canTurnOn = false;
                creature->Whisper("冒险者，你的挑战等级还不足以开启这项挑战！", LANG_UNIVERSAL, player);
                return OnGossipSelect(player, creature, GOSSIP_SENDER_MAIN, 99);
            }
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
                std::ostringstream str;
                str << "我已经为你们开启了" << sender << "级挑战, 通关需击杀"+ std::to_string(sChallengeDiff ->BaseEnhanceMapData[player->GetMap()->GetId()].boss_count) << "个BOSS!";
                sChallengeDiff->SendWhisperToRaid(str.str(), creature, player);
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
                sChallengeDiff->CloseChallenge(player->GetMap());
                sChallengeDiff->SendWhisperToRaid("现在已经变为了正常模式了，再见!", creature, player);
                //creature->DespawnOrUnsummon(2000);
                creature->SetVisible(false);
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
        if (!player->IsGameMaster() && !group)
        {
            creature->Whisper("我现在不能帮你开启挑战, 这太过危险，你可以召集你的朋友组队一起来挑战。", LANG_UNIVERSAL, player);
            return true;
        }

        uint32 npcText = 61000;
        //LOG_INFO("module", "MOD-ZONE-DIFFICULTY: OnGossipHello Has Group");
        if (player->IsGameMaster() || (group && group->IsLeader(player->GetGUID())))
        {
            uint32 instanceId = player->GetMap()->GetInstanceId();
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
                auto data = sChallengeDiff->ChallengeInstanceData.find(instanceId);
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

class mod_zone_npc_tzds : public CreatureScript
{
public:
    mod_zone_npc_tzds() : CreatureScript("mod_zone_npc_tzds") { }

    struct mod_zone_npc_tzdsAI : public ScriptedAI
    {
        mod_zone_npc_tzdsAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _scheduler.CancelAll();
            _scheduler.Schedule(300s, [this](TaskContext context)
                {
                    sChallengeDiff->CheckUpdateActiveMap();
                    context.Repeat();
                });
            _scheduler.Schedule(120s, [this](TaskContext context)
                {
                    std::ostringstream str;
                    str << "冒险者!今日挑战地图有: ";
                    if (sChallengeDiff->DayActiveMaps.active_mapid[0] > 0)
                        str << sChallengeDiff->BaseEnhanceMapData[sChallengeDiff->DayActiveMaps.active_mapid[0]].descp << ", ";
                    if (sChallengeDiff->DayActiveMaps.active_mapid[1] > 0)
                        str << sChallengeDiff->BaseEnhanceMapData[sChallengeDiff->DayActiveMaps.active_mapid[1]].descp << ", ";
                    if (sChallengeDiff->DayActiveMaps.active_mapid[2] > 0)
                        str << sChallengeDiff->BaseEnhanceMapData[sChallengeDiff->DayActiveMaps.active_mapid[2]].descp << ", ";
                    if (sChallengeDiff->DayActiveMaps.active_mapid[3] > 0)
                        str << sChallengeDiff->BaseEnhanceMapData[sChallengeDiff->DayActiveMaps.active_mapid[3]].descp;
                    me->Yell(str.str(), LANG_UNIVERSAL);
                    context.Repeat(150s);
                });

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
        return new mod_zone_npc_tzdsAI(creature);
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
       
        if (action == 1) {
            auto id = sChallengeDiff->DayActiveMaps.active_mapid[0];  
            std::ostringstream str;
            str << "今日挑战地图有:";
            if (sChallengeDiff->DayActiveMaps.active_mapid[0] > 0)
                str << sChallengeDiff->BaseEnhanceMapData[sChallengeDiff->DayActiveMaps.active_mapid[0]].descp << ", ";
            if (sChallengeDiff->DayActiveMaps.active_mapid[1] > 0)
                str << sChallengeDiff->BaseEnhanceMapData[sChallengeDiff->DayActiveMaps.active_mapid[1]].descp << ", ";
            if (sChallengeDiff->DayActiveMaps.active_mapid[2] > 0)
                str << sChallengeDiff->BaseEnhanceMapData[sChallengeDiff->DayActiveMaps.active_mapid[2]].descp << ", ";
            if (sChallengeDiff->DayActiveMaps.active_mapid[3] > 0)
                str << sChallengeDiff->BaseEnhanceMapData[sChallengeDiff->DayActiveMaps.active_mapid[3]].descp;
            creature->Whisper(str.str(), LANG_UNIVERSAL, player);
            CloseGossipMenuFor(player);
        }
        else if(action==2)
        {
           player->PlayerTalkClass->ClearMenus();
           player->PrepareQuestMenu(creature->GetGUID());  
           SendGossipMenuFor(player, 68, creature);
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "今天的挑战副本有哪些？", GOSSIP_SENDER_MAIN, 1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我想升级挑战着衬衫。", GOSSIP_SENDER_MAIN, 2);
        SendGossipMenuFor(player, 68, creature);
        return true;
    }
};
class TMItemStrong_Script : public ItemScript
{
public:
    TMItemStrong_Script() : ItemScript("TMItemStrong_Script") { } 
    bool OnUse(Player* player, Item* item, const SpellCastTargets&) override
    {
        ChatHandler ch(player->GetSession());
        Unit* target = player->GetSelectedUnit();
        if (!target) {
            player->GetSession()->SendNotification("我还没有目标!");
            return false;
        }
        if (target->GetTypeId()!= TYPEID_PLAYER && !target->IsNPCBot()) {
            player->GetSession()->SendNotification("这是一个无效的目标!");
            return false;
        }
        uint32 instid= player->GetMap()-> GetInstanceId();
        if (!sChallengeDiff->HasChallengMode(instid)) {
            ch.SendSysMessage("你必须在挑战模式中使用该物品！");
            return false;
        }
        if (player->CastSpell(target, 90010, true) == SPELL_CAST_OK) {
            //player->DestroyItemCount(item->GetEntry(), 1, true);
            return true;
        }
        return false;
    }

};
class spell_kuangbaonuhou_aura : public SpellScriptLoader
{
public:
    spell_kuangbaonuhou_aura() : SpellScriptLoader("spell_kuangbaonuhou_aura") { }

    class spell_kuangbaonuhou_aura_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_kuangbaonuhou_aura_AuraScript);

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            Unit* caster = GetCaster();
            if (!caster) return false;
            if (caster->HealthBelowPct(25)) {
                return true;
            }
            else
            {
                iscasted = false;
                return false;
            }
        }

        void HandleProc(AuraEffect const*  /*aurEff*/, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            if (iscasted) return;
            Unit* caster = GetCaster();
            if (caster->CastSpell(caster, 100014, false))
                iscasted = true;
            return;
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_kuangbaonuhou_aura_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_kuangbaonuhou_aura_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    private:
        bool iscasted = false;
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_kuangbaonuhou_aura_AuraScript();
    }
};

class spell_challenge_periodic_trigger : public SpellScriptLoader
{
public:
    spell_challenge_periodic_trigger() : SpellScriptLoader("spell_challenge_periodic_trigger") { }

    class spell_challenge_periodic_trigger_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_challenge_periodic_trigger_AuraScript);

        void HandleTriggerSpell(AuraEffect const* aurEff)
        {
            PreventDefaultAction();
            Unit* caster = GetCaster();
            if (!caster)
                return;
            auto target = caster->GetAI()->SelectTarget(SelectTargetMethod::Random, 0, 30.0f);
            if (!target || !target->IsAlive() || !target->IsInWorld()) return;
            //caster->GetThreatMgr().GetThreatList
            uint32 triggerSpell = GetSpellInfo()->Effects[aurEff->GetEffIndex()].TriggerSpell;
            SpellInfo const* spell = sSpellMgr->AssertSpellInfo(triggerSpell);
            if (!spell) return;
            caster->CastSpell(target, triggerSpell, true);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_challenge_periodic_trigger_AuraScript::HandleTriggerSpell, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);

        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_challenge_periodic_trigger_AuraScript();
    }
};
// Add all scripts in one
void AddModZoneDifficultyScripts()
{
    new mod_zone_difficulty_worldscript();
    new mod_zone_difficulty_globalscript();
    new mod_zone_difficulty_dungeonmaster();
    new mod_zone_npc_tzds();
    new spell_kuangbaonuhou_aura();
    new spell_challenge_periodic_trigger();
    new TMItemStrong_Script();
}
