 
#include "Player.h"
#include "Config.h"
#include "ScriptMgr.h"  
#include "Chat.h"   
#include <unordered_map>
#include <vector>

enum ResultStatus{
  ERROR_CCMax=2,
  ERROR_ADDMax=2
}

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
class PlayerTransmog
{
  private:
    bool IsEnable=true;

  public:
    static PlayerTransmog* instance(){
       static PlayerTransmog instance;
       return &instance;
    }
    std::map<uint32, QualityGroupMap> ModelDataStore;
    std::map<uint32, CcList> CollectionDataStore;
    void InitData();
    bool CastTransmogBot(Unit* target, int modelid);

    //变身
    bool CastTransmog(Player* player,int modelid);
    bool AddModelData(uint32 account_id, Creature* const creature);
    QualityGroupMap* GetAccountQualityGroupMap(uint32 account_id); 
    ModelData* GetModelDataById(uint32 account_id,uint32 modelId);
    //设置喜欢标记
    uint8 SetCcFlag(uint32 account_id, int modelid,int flag);
    //删除收集的模型
    bool DeleteCcModel(uint32 account_id, int modelid);

    std::string GetModelNameText(ModelData* const data);
    
};
 
//变形操作
#define pTransmog PlayerTransmog::instance()

