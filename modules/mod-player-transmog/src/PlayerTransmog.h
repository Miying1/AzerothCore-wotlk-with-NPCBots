 
#include "Player.h"
#include "Config.h"
#include "ScriptMgr.h"  
#include "Chat.h"   
#include "ObjectGuid.h"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>
#include <utility>

enum ResultStatus {
    ERROR_CCMax = 2,
    ERROR_ADDMax = 2
};

struct ModelData
{
    //uint32          account_id;
    uint32          modelid;
    std::string     modelname;
    uint32          ccflag;
    uint32          quality;

};
typedef std::list<ModelData> ModelDataList;
//品级组
typedef std::map<uint32, ModelDataList> QualityGroupMap;
//收藏集合
typedef std::vector<ModelData> CcList;

//佣兵幻形数据（角色级）
struct BotTransmogData
{
    uint32      bot_entry{};
    uint32      model_id{};
    std::string model_name;
};

//玩家幻形状态（角色级）：记录当前幻形模型与缩放，供被其他变形覆盖后恢复
struct PlayerTransmogState
{
    uint32      modelid{};
    float       scale{ 1.0f };
};
class PlayerTransmog
{
  private:
    bool IsEnable=true;
    // 佣兵幻形数据：character_id -> bot_entry -> 数据（跨 worker 线程访问，需锁保护）
    std::map<uint32, std::map<uint32, BotTransmogData>> BotTransmogStore;
    // 佣兵幻形数据锁：保护 BotTransmogStore 的跨线程并发访问
    mutable std::mutex _botTransmogMutex;
    // 玩家幻形状态：player guid -> 当前幻形模型（主世界线程访问，无需加锁）
    std::unordered_map<ObjectGuid, PlayerTransmogState> PlayerTransmogStore;

  public:
    static PlayerTransmog* instance(){
       static PlayerTransmog instance;
       return &instance;
    }
    std::map<uint32, QualityGroupMap> ModelDataStore;
    std::map<uint32, CcList> CollectionDataStore;
    void InitData();
    bool CastTransmogBot(Creature* bot, uint32 modelId);

    //变身
    bool CastTransmog(Player* player,int modelid);
    ModelData* AddModelData(uint32 account_id, Creature* const creature);
    ModelData* AddModelDataById(uint32 account_id, std::string name, uint32 modelid, uint32 quality);
    QualityGroupMap* GetAccountQualityGroupMap(uint32 account_id); 
    ModelData* GetModelDataById(uint32 account_id,uint32 modelId);
    //设置喜欢标记
    uint8 SetCcFlag(uint32 account_id, int modelid,int flag);
    //删除收集的模型
    bool DeleteCcModel(uint32 account_id, int modelid);

    std::string GetModelNameText(ModelData* const data);

    //佣兵幻形：数据读写与恢复
    void LoadBotTransmogData();
    std::optional<BotTransmogData> GetBotTransmog(uint32 characterId, uint32 botEntry);
    void SetBotTransmog(uint32 characterId, uint32 botEntry, uint32 modelId, std::string const& modelName);
    void RemoveBotTransmog(uint32 characterId, uint32 botEntry);
    void RestoreBotTransmog(Creature* bot);

    //玩家幻形状态读写：记录/读取/清除当前幻形（用于被其他变形覆盖后恢复）
    void SetPlayerTransmog(Player* player, uint32 modelid, float scale);
    std::optional<PlayerTransmogState> GetPlayerTransmog(Player* player) const;
    void ClearPlayerTransmog(Player* player);
    // 在锁内拷贝某角色全部幻形条目（entry -> model_id），供巡检在锁外使用
    std::vector<std::pair<uint32, uint32>> GetBotTransmogEntries(uint32 characterId) const;
    
};
 
//变形操作
#define pTransmog PlayerTransmog::instance()

