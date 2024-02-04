#include "PlayerTransmog.h"
#include "ItemTemplate.h"
#include "DatabaseEnv.h" 
#include "Configuration/Config.h"



//void PlayerTransmog::InitConfig()
//{
//    RandRangeattrMaxPct = sConfigMgr->GetOption<float>("RandomEnchants.RandRangeattrMaxPct", 0.7f);
//    MaxAttrChance = sConfigMgr->GetOption<float>("RandomEnchants.MaxAttrChance", 40.0f);
//    ReRandChance = sConfigMgr->GetOption<float>("RandomEnchants.ReRandChance", 80.0f);
//}

void PlayerTransmog::InitData()
{
    QueryResult result = CharacterDatabase.Query("SELECT account_id,modelid,modelname,ccflag,quality FROM mod_player_transmog ");
    if (!result) {
        IsEnable = false;
        return;
    };
    ModelDataStore.clear();
    CollectionDataStore.clear();
    do
    {
        Field* fields = result->Fetch();
        ModelData mdata = {};
        uint32 account_id = fields[0].Get<uint32>();
        mdata.modelid = fields[1].Get<uint32>();
        mdata.modelname = fields[2].Get<std::string>();
        mdata.ccflag = fields[3].Get<uint32>();
        mdata.quality = fields[4].Get<uint32>();
        //账号分组
        auto mits = ModelDataStore.find(account_id);
        if (mits != ModelDataStore.end()) {
            //品质组
            auto qgroup = &mits->second;
            auto list_it = qgroup->find(mdata.quality);
            if (list_it != qgroup->end()) {
                (*list_it).second.push_back(mdata);
            }
            else
            {
                ModelDataList list = { mdata };
                qgroup->operator[](mdata.quality) = list;
            }
        }
        else {
            QualityGroupMap qgmap = {};
            ModelDataList list = { mdata };
            qgmap[mdata.quality] = list;
            ModelDataStore[account_id] = qgmap;
        }
        if (mdata.ccflag == 1) {
            if (CollectionDataStore.find(account_id) == CollectionDataStore.end())
                CollectionDataStore[account_id] = CcList();
            CollectionDataStore[account_id].push_back(mdata); 
        }
    } while (result->NextRow());
}
bool PlayerTransmog::CastTransmogBot(Unit* target, int modelid)
{
    target->RemoveAurasByType(SPELL_AURA_TRANSFORM);
    CreatureDisplayInfoEntry const* minfo = sCreatureDisplayInfoStore.AssertEntry(modelid);
    if (!minfo) return false;
    CreatureDisplayInfoEntry const* pminfo = sCreatureDisplayInfoStore.AssertEntry(target->GetNativeDisplayId());

    if (target->CastSpell(target, 100004, false) == SPELL_CAST_OK) {
        target->SetDisplayId(modelid);

        if (minfo->scale > (pminfo->scale * 1.1)) {
            float scale = pminfo->scale / (minfo->scale);
            target->SetObjectScale(scale);
        }
    }
    return true;
}
bool PlayerTransmog::CastTransmog(Player* player, int modelid)
{
    player->RemoveAurasByType(SPELL_AURA_TRANSFORM);
    CreatureDisplayInfoEntry const* minfo = sCreatureDisplayInfoStore.AssertEntry(modelid);
    if (!minfo) return false;
    CreatureDisplayInfoEntry const* pminfo = sCreatureDisplayInfoStore.AssertEntry(player->GetNativeDisplayId()); 
 
    if (player->CastSpell(player->ToUnit(), 100004, false) == SPELL_CAST_OK) {
        player->SetDisplayId(modelid);

        if (minfo->scale > (pminfo->scale * 1.1)) {
            float scale = pminfo->scale  / (minfo->scale); 
            player->SetObjectScale(scale);
        } 
    } 
    return true;
}

ModelData* PlayerTransmog::AddModelData(uint32 account_id, Creature* const creature)
{
    const CreatureTemplate* ctemp = creature->GetCreatureTemplate();
   
    uint32 modelid = creature->GetNativeDisplayId();
    CreatureDisplayInfoEntry const* minfo = sCreatureDisplayInfoStore.AssertEntry(modelid);
    if (!minfo) return nullptr;
    return AddModelDataById(account_id, ctemp->Name, minfo->Displayid, ctemp->rank == 4 ? 2 : ctemp->rank);
}

ModelData* PlayerTransmog::AddModelDataById(uint32 account_id, std::string name, uint32 modelid,uint32 quality)
{
    if (GetModelDataById(account_id, modelid)) {
        return nullptr;
    }
    ModelData mdata = {};
    mdata.modelid = modelid;
    mdata.modelname = name;
    mdata.ccflag = 0;
    mdata.quality = quality;
    QualityGroupMap* qgroup = GetAccountQualityGroupMap(account_id);
    if (qgroup->find(mdata.quality) == qgroup->end()) {
        qgroup->emplace(mdata.quality, ModelDataList()); 
    }
    if(qgroup->operator[](mdata.quality).size()>=30){
      return nullptr;
    }
    qgroup->operator[](mdata.quality).push_back(mdata);
    CharacterDatabase.Query("insert into mod_player_transmog(account_id,modelid,modelname,ccflag,quality)values({},{},'{}',{},{})", account_id, mdata.modelid, mdata.modelname, mdata.ccflag, mdata.quality); 
    return  &qgroup->operator[](mdata.quality).back();
}

QualityGroupMap* PlayerTransmog::GetAccountQualityGroupMap(uint32 account_id)
{
    if (ModelDataStore.find(account_id) == ModelDataStore.end()) {
        ModelDataStore.emplace(account_id, QualityGroupMap());
    }
    return &ModelDataStore[account_id];
}

ModelData* PlayerTransmog::GetModelDataById(uint32 account_id, uint32 modelId)
{
    QualityGroupMap* qgroup = GetAccountQualityGroupMap(account_id);
    for (auto its = qgroup->begin(); its != qgroup->end(); ++its)
    {
        auto datalist = &(*its).second;
        if (datalist->empty()) continue;
        for (auto data_its = datalist->begin(); data_its != datalist->end(); ++data_its)
        {
            if ((*data_its).modelid == modelId)
                return &(*data_its);
        }
    }
    return nullptr;
}

uint8 PlayerTransmog::SetCcFlag(uint32 account_id, int modelid, int flag)
{
    auto data = GetModelDataById(account_id, modelid);
    if (!data) return 0;
    data->ccflag = flag;
    auto cc_its = CollectionDataStore.find(account_id);
    if (cc_its == CollectionDataStore.end()) {
        CollectionDataStore[account_id] = CcList();
    }
    if (flag == 1) {
        if(CollectionDataStore[account_id].size()>=15){
          return ERROR_CCMax;
        }
        CollectionDataStore[account_id].push_back(*data);
    }
    else
    {
        CcList* cclist = &CollectionDataStore[account_id];
        for (auto it = cclist->begin(); it != cclist->end(); ++it)
        {
            if ((*it).modelid == modelid) {
                cclist->erase(it);
                break;
            }
        }
    }
    CharacterDatabase.Query("update mod_player_transmog set ccflag={} WHERE  account_id={} and modelid={} ", flag, account_id, modelid);
    return 1;
}

bool PlayerTransmog::DeleteCcModel(uint32 account_id, int modelid)
{
    auto data = GetModelDataById(account_id, modelid);
    if (!data) return false;
    QualityGroupMap* qgroup = GetAccountQualityGroupMap(account_id);
    ModelDataList* datalist = &qgroup->operator[](data->quality);
    for (auto data_its = datalist->begin(); data_its != datalist->end(); ++data_its)
    {
        if ((*data_its).modelid == modelid) {
            datalist->erase(data_its);
        }
    }
    if (CollectionDataStore.find(account_id) != CollectionDataStore.end()) {
        CcList* cclist = &CollectionDataStore[account_id];
        for (auto it = cclist->begin(); it != cclist->end(); ++it)
        {
            if ((*it).modelid == modelid) {
                cclist->erase(it);
                break;
            }
        }
    }
    CharacterDatabase.Query("delete from mod_player_transmog where account_id={} and modelid={} ", account_id, modelid);
    return true;
}

std::string PlayerTransmog::GetModelNameText(ModelData* data)
{
    std::ostringstream str;
    str << "|c";
    switch (data->quality)
    {
    case 0:     str << "ff1eff00"; break;  //GREEN
    case 1:     str << "ff0070dd"; break;  //BLUE
    case 2:     str << "ffa335ee"; break;  //PURPLE
    case 3:     str << "ffd74800"; break;  //ORANGE 
    default:    str << "ff000000"; break;  //UNK BLACK
    }
    str << data->modelname << "|r";
    return str.str();
}





