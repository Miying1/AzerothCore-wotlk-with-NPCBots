#include "TnEchants.h"
#include "ItemTemplate.h"
#include "DatabaseEnv.h" 
#include "Configuration/Config.h"


 
void TnEchants::InitConfig()
{
    RandRangeattrMaxPct = sConfigMgr->GetOption<float>("RandomEnchants.RandRangeattrMaxPct", 0.7f);
    MaxAttrChance = sConfigMgr->GetOption<float>("RandomEnchants.MaxAttrChance", 40.0f);
    ReRandChance = sConfigMgr->GetOption<float>("RandomEnchants.ReRandChance", 80.0f);
}

void TnEchants::InitData()
{
       QueryResult delres = WorldDatabase.Query("delete from mod_item_enchantment_result where item_guid not in (select guid from acore_characters.item_instance)");
       if(delres){
          LOG_INFO("module", "随机附魔废弃装备删除:{} 个",(*delres)[0].Get<uint32>());
       }
       QueryResult result = WorldDatabase.Query("SELECT enchantID,attr_group,oid FROM mod_item_enchantment_random where active=1 order by attr_group,oid ");
        if (!result) {
            IsEnable=false;
            return ;
        }; 
        EnchEffectStore.clear();
        do
        {
            Field* fields = result->Fetch();
            uint32 enchantID = fields[0].Get<uint32>();
            uint32 attr_group = fields[1].Get<uint32>();
            uint32 oid = fields[2].Get<uint32>();
            EnchantsEffect effect={enchantID,attr_group,oid};
            auto its = EnchEffectStore.find(attr_group);
            if(its!=EnchEffectStore.end()){
                EnchEffectGroup* group = &its->second;
                group->push_back(effect);
            }else{
              EnchEffectGroup group;
              group.push_back(effect);
              EnchEffectStore[attr_group]=group;
            } 
        } while (result->NextRow()); 
        IsEnable=EnchEffectStore.size()>0;
 
}

bool TnEchants::RandomEnchEffect(Player* player,Item* const item){
    if (!IsEnable || EnchEffectStore.size() < 1) return false;
    uint32 group_rand=urand(1,EnchEffectStore.size())-1;
    if(group_rand<0) return false; 
    auto efs_its=EnchEffectStore.begin();
    std::advance(efs_its, group_rand);  
    const EnchEffectGroup* group =&efs_its->second;
    if (!group || group->size() < 2) return false;
    uint32 maxrand = static_cast<uint32>(std::floor(group->size() * RandRangeattrMaxPct));
    if (maxrand <= 1) return false;
    uint32 eff_rand=urand(1, maxrand)-1;
    if(eff_rand<0) return false; 
    auto group_its=group->begin();
    std::advance(group_its, eff_rand);  
    const EnchantsEffect* effect = &(*group_its);
    if(!&effect) return false;
    if (sSpellItemEnchantmentStore.LookupEntry(effect->enchantID)) { //Make sure enchantment id exists
            player->ApplyEnchantment(item, EnchantmentSlot(0), false);
            item->SetEnchantment(EnchantmentSlot(0), effect->enchantID, 0, 0);
            player->ApplyEnchantment(item, EnchantmentSlot(0), true);
            SaveEnchEffect(item->GetGUID().GetCounter(), effect, 1); 
            return true;
    }
    else
    {
        return false;
    }
}
bool TnEchants::SaveEnchEffect(uint32 itemid, const EnchantsEffect* enchEffect,uint32 isnew){
 
      ItemEnchants* itemEnch = nullptr;
      if (ItemEnchantsStore.find(itemid) != ItemEnchantsStore.end()) {
          ItemEnchantsStore[itemid].attr_group = enchEffect->attr_group;
          ItemEnchantsStore[itemid].max_oid=EnchEffectStore[enchEffect->attr_group].size();
          ItemEnchantsStore[itemid].oid = enchEffect->oid;
          if(isnew>0)
              ItemEnchantsStore[itemid].refm_count =0;
          else{
             ItemEnchantsStore[itemid].refm_count += 1;
          }
          itemEnch = &ItemEnchantsStore[itemid];
      }
      else
      {
          ItemEnchants newitemEh{};
          newitemEh.item_guid = itemid;
          newitemEh.attr_group = enchEffect->attr_group;
          newitemEh.max_oid=EnchEffectStore[enchEffect->attr_group].size();
          newitemEh.oid = enchEffect->oid;
          newitemEh.refm_count =0;
          ItemEnchantsStore[itemid] = newitemEh;
          itemEnch =&newitemEh;
      }
      WorldDatabase.Query("REPLACE into mod_item_enchantment_result(item_guid,attr_group,oid,refm_count)values({},{},{},{});", itemid, itemEnch->attr_group, itemEnch->oid, itemEnch->refm_count );
      return true;
}


ItemEnchants* TnEchants::GetItemEnchants(uint32 item_guid){
  auto its = ItemEnchantsStore.find(item_guid);
  if (its != ItemEnchantsStore.end()) {
    return &its->second;
  }
  return nullptr;
}
 
 
bool TnEchants::VerifyItemIsTn(uint32 item_guid){
        auto itech = GetItemEnchants(item_guid);
        if (itech) return true; 
        QueryResult qr = WorldDatabase.Query("SELECT attr_group,oid,refm_count FROM mod_item_enchantment_result WHERE  item_guid={} ", item_guid);
        if (!qr) return false;
        ItemEnchants itemEnchants{};
        itemEnchants.item_guid = item_guid;
        itemEnchants.attr_group = qr->Fetch()[0].Get<uint32>();
        itemEnchants.oid = qr->Fetch()[1].Get<uint32>();
        itemEnchants.max_oid =EnchEffectStore[itemEnchants.attr_group].size();
        itemEnchants.refm_count = qr->Fetch()[2].Get<uint32>();
        ItemEnchantsStore[item_guid] = itemEnchants;
        return true;
}

float TnEchants::GetUpEnchLevelChance(ItemEnchants* itemEnch)const
{ 
    uint32 centre=std::ceil(itemEnch->max_oid/2.0);
    if(itemEnch->oid <= centre)//中间之前的等级100%概率
        return 100.0;
    float onelevelChance=std::ceil((100.0-MaxAttrChance)/(itemEnch->max_oid-centre));
    float res_chence= std::ceil(85.0 - (itemEnch->oid-centre)*onelevelChance);
    if(itemEnch->refm_count>0){
        res_chence += (itemEnch->refm_count * 5);
    }
    return  std::max(res_chence,15.0f);
}

uint32 TnEchants::UpEnchLevel(Player* player, Item* item){
    if (!IsEnable) return 0;
      uint32 itemid=item->GetGUID().GetCounter();
      ItemEnchants* itemEnch=GetItemEnchants(itemid);
      if(!itemEnch) return 0;
      std::list<EnchantsEffect>& attr_group = EnchEffectStore[itemEnch->attr_group];
      if (attr_group.back().oid <= itemEnch->oid) return 3;
      float chance=GetUpEnchLevelChance(itemEnch);
      uint32 curOid=itemEnch->oid +1;
      float roll=rand_chance();
      if (roll > chance) { 
        itemEnch->refm_count++; 
        WorldDatabase.Query("update mod_item_enchantment_result set refm_count=refm_count+1 WHERE  item_guid={} ", itemid);
        return 1;//几率失败
      }
      if(roll<=chance*0.2){ //连升两级几率
        curOid+=1;
      }
      
      if(attr_group.empty()) return 0;
      curOid=std::min(curOid, attr_group.back().oid);
      auto it = attr_group.begin(); 
      const EnchantsEffect* effect=nullptr;
      while (it != attr_group.end()) { 
          if(it->oid ==curOid){
              effect = &(*it);  
              break; 
          }
          ++it;
      }
      if(!effect) return 0;
      if (sSpellItemEnchantmentStore.LookupEntry(effect->enchantID)) { //Make sure enchantment id exists
          player->ApplyEnchantment(item, EnchantmentSlot(0), false);
          item->SetEnchantment(EnchantmentSlot(0), effect->enchantID, 0, 0);
          player->ApplyEnchantment(item, EnchantmentSlot(0), true);
          SaveEnchEffect(item->GetGUID().GetCounter(), effect, 1); 
          return 2;//成功
      }
      else
      {
          return 0;
      }
      
}
