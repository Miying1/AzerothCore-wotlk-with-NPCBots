 
#include "Player.h"
#include "Config.h"
#include "ScriptMgr.h" 
#include "Item.h" 
#include "Chat.h"
#include "ItemTemplate.h"  

 
struct ItemEnchants
{
    uint32    item_guid;
    uint32     attr_group;
    uint32     oid;
    uint32     max_oid;
    uint32     refm_count;
};
struct EnchantsEffect
{
    uint32    enchantID;
    uint32     attr_group;
    uint32     oid;
};
typedef std::list<EnchantsEffect> EnchEffectGroup;
class TnEchants
{
  private:
    bool IsEnable=true;

  public:
    static TnEchants* instance(){
       static TnEchants instance;
       return &instance;
    }
    //随机值范围上限%
    float RandRangeattrMaxPct;
    //满级属性概率(1-100)
    float MaxAttrChance;
    float ReRandChance;
    std::unordered_map<uint32, ItemEnchants> ItemEnchantsStore; 
    std::map<uint8,EnchEffectGroup> EnchEffectStore;

    void InitData();
    void InitConfig();
    //随机附魔
    bool RandomEnchEffect(Player* player,Item* const item);
    bool SaveEnchEffect(uint32 itemid, const EnchantsEffect* itemEnch,uint32 isnew);

    //附魔强化  res flag:0 异常  1 失败 2成功
    uint32 UpEnchLevel(Player* player, Item* item);
    //判断物品是不是泰坦FM物品
    bool VerifyItemIsTn(uint32 item_guid);
    ItemEnchants* GetItemEnchants(uint32 item_guid);
    float GetUpEnchLevelChance(ItemEnchants* itemEnch) const;
};
 

#define tnEchants TnEchants::instance()

