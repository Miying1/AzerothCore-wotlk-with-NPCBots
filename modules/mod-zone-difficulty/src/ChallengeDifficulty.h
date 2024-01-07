#ifndef DEF_CAD_H
#define DEF_CAD_H

#include "Player.h"
#include "Config.h"
#include "InstanceScript.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"


enum MyEnum
{
    SCORE_CURRENCY  = 62000,
    SPELL_ENHANCE_CREATURE  =100006,//怪物增强
    SPELL_DEFF_PLAYER = 100012,//玩家压制
};

//挑战等级
struct ZoneDifficultyLevel
{
    uint32 difflevel;
    uint32 enhance=0;
    uint32 diff_player = 0;
    uint32 global_spell_num;
    uint32 boss_score;
};
//实例开启的挑战信息
struct ZoneChallengeData
{
    uint32 level=0;
    uint32 enhance_damage;
    uint32 enhance_hp;
    std::array<uint32,3> apply_spell; 
};
//全局法术信息
struct ZoneChallengeSpell
{
    uint32 spell_id;
    uint8 chance; 
    Milliseconds delay;
    Milliseconds cooldown;
    bool triggered_cast; 
};
//实例基础提升值
struct ZoneChallengeBaseEnhance
{
    uint32 base_hp_pct;
    uint32 base_damage_pct;
};
struct ZoneChallengeSpellGroup
{
    std::array<uint32, 3> spellIds;
};

struct ZoneDifficultyHAI
{
    uint8 Chance;
    uint32 Spell;
    int32 Spellbp0;
    int32 Spellbp1;
    int32 Spellbp2;
    uint8 Target;
    int8 TargetArg;
    uint8 TargetArg2;
    Milliseconds Delay;
    Milliseconds Cooldown;
    uint8 Repetitions;
    bool TriggeredCast;
};


class ChallengeDifficulty
{
public:
    static ChallengeDifficulty* instance();

    void LoadMapDifficultySettings(); 
    void LoadMythicmodeInstanceData();

    void SendWhisperToRaid(std::string message, Creature* creature, Player* player);
    //是否已开启挑战模式
    bool HasChallengMode(uint32 inst_id);
    //开启挑战
    bool OpenChallenge(uint32 inst_id, uint32 level, Player* player);
    bool CloseChallenge(uint32 inst_id, Player* player);
    //随机获取一个全局法术
    void GetRandomGlobalSpell(uint8 count, std::array<uint32, 3>* apply_spell);
    void SetPlayerChallengeLevel(Map* map); 
    void AddBossScore(Map* map);
    void SendChallengLoot(Map* map);

    void RemoveChallengeAure(Unit* creature);

    void RemoveChallengeAureBuff(Unit* unit);

    void ApplyChallengeAure(Unit* creature, uint32 instanceId);

    bool IsEnabled{ false };
    bool IsDebugInfoEnabled{ false };
    std::map<uint32, uint32> EncountersInProgress;
 
    typedef std::map<uint32, ZoneChallengeBaseEnhance > ZoneChallengeBaseEnhanceMap;
    //地图的基础增强
    ZoneChallengeBaseEnhanceMap  BaseEnhanceMapData;
    typedef std::map<uint32, ZoneDifficultyLevel > ZoneDifficultyLevelData;
    //挑战等级集
    ZoneDifficultyLevelData DiffLevelData; 
    typedef std::map<uint32, ZoneChallengeData> ZoneChallengeDataInstMap;
    //已开启的挑战副本
    ZoneChallengeDataInstMap ChallengeInstanceData;
    typedef std::map<uint32, ZoneChallengeSpell> ZoneChallengeSpellMap;
    //全局法术集
    ZoneChallengeSpellMap ZoneChallengeSpellData;
    //法术组合
    std::map<uint32, ZoneChallengeSpellGroup> ZoneChallengeSpellGroupData;
     
    typedef std::map<uint32, std::vector<ZoneDifficultyHAI> > ZoneDifficultyHAIMap;
    ZoneDifficultyHAIMap MythicmodeAI;
    typedef std::map<uint32, std::map<uint32, std::map<uint32, bool> > > ZoneDifficultyEncounterLogMap;
    ZoneDifficultyEncounterLogMap Logs;
    typedef std::map<uint32, uint32 > ZoneDifficultyPlayerLevelMap;
    //玩家挑战等级
    ZoneDifficultyPlayerLevelMap PlayerLevelData;
};

#define sChallengeDiff ChallengeDifficulty::instance()

#endif
