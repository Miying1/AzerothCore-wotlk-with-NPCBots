 
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

// 幻形最终缩放系数：玩家在输入框里自定义，作用在计算出的归一缩放之上
//   < 1 缩小，> 1 放大；取值夹取在 [MIN, MAX]，默认 DEFAULT
constexpr float TRANSMOG_SCALE_FACTOR_MIN     = 0.5f;
constexpr float TRANSMOG_SCALE_FACTOR_MAX     = 1.5f;
constexpr float TRANSMOG_SCALE_FACTOR_DEFAULT = 1.0f;

// 解析玩家在输入框里填写的缩放系数：合法则夹取到 [MIN, MAX] 并写入 outFactor；
// 空串 / 非数字 / 带多余字符一律返回 false（调用方提示重填）
bool ParseTransmogScaleFactor(std::string const& text, float& outFactor);

// 把系数格式化成菜单展示用的文本（保留 2 位小数，如 "1.25 倍"）
std::string FormatTransmogScaleFactor(float factor);

// 生成缩放输入框的提示文本："请输入缩放系数(0.5-1.5),当前值:1.00"
std::string MakeTransmogScaleHint(float currentFactor);

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
    // 玩家为该佣兵单独设置的最终缩放系数（0.5 - 1.5，默认 1.0）
    float       scale_factor{ TRANSMOG_SCALE_FACTOR_DEFAULT };
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
    // 玩家变形缩放设置：character_id -> 缩放系数（DB 写入在锁外，读取可能来自巡检线程，需加锁）
    std::map<uint32, float> PlayerScaleStore;
    mutable std::mutex _playerScaleMutex;

  public:
    static PlayerTransmog* instance(){
       static PlayerTransmog instance;
       return &instance;
    }
    std::map<uint32, QualityGroupMap> ModelDataStore;
    std::map<uint32, CcList> CollectionDataStore;
    void InitData();
    // scaleFactor：玩家自定义的最终缩放系数（0.5 - 1.5），乘在归一缩放之上
    bool CastTransmogBot(Creature* bot, uint32 modelId, float scaleFactor = TRANSMOG_SCALE_FACTOR_DEFAULT);

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
    void SetBotTransmog(uint32 characterId, uint32 botEntry, uint32 modelId, std::string const& modelName,
                        float scaleFactor = TRANSMOG_SCALE_FACTOR_DEFAULT);
    void RemoveBotTransmog(uint32 characterId, uint32 botEntry);
    void RestoreBotTransmog(Creature* bot);
    // 只更新某个佣兵的缩放系数：不影响已选模型；该佣兵还没有记录时插入一条空模型记录
    void SetBotTransmogScale(uint32 characterId, uint32 botEntry, float scaleFactor);
    // 读取某个佣兵的缩放系数（无记录时返回默认 1.0）
    float GetBotTransmogScaleFactor(uint32 characterId, uint32 botEntry) const;
    // 判断 BOT 当前是否已正确套用幻形（显示ID + 非角色模型时的 race 字节），供兜底巡检使用
    bool IsBotTransmogApplied(Creature* bot, uint32 modelId) const;

    //玩家幻形状态读写：记录/读取/清除当前幻形（用于被其他变形覆盖后恢复）
    void SetPlayerTransmog(Player* player, uint32 modelid, float scale);
    std::optional<PlayerTransmogState> GetPlayerTransmog(Player* player) const;
    void ClearPlayerTransmog(Player* player);

    // 玩家变形缩放系数（角色级，持久化在 mod_player_transmog_set）
    void LoadPlayerTransmogSetData();
    float GetPlayerTransmogScaleFactor(uint32 characterId) const;
    void SetPlayerTransmogScaleFactor(uint32 characterId, float scaleFactor);

    // 在锁内拷贝某角色全部幻形条目（含模型 id 与缩放系数），供巡检在锁外使用
    std::vector<BotTransmogData> GetBotTransmogEntries(uint32 characterId) const;
    
};
 
//变形操作
#define pTransmog PlayerTransmog::instance()

