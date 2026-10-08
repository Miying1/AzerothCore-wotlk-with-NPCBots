#include "PlayerTransmog.h"
#include "Creature.h"
#include "ItemTemplate.h"
#include "DatabaseEnv.h" 
#include "Configuration/Config.h"
#include "Log.h"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>



//void PlayerTransmog::InitConfig()
//{
//    RandRangeattrMaxPct = sConfigMgr->GetOption<float>("RandomEnchants.RandRangeattrMaxPct", 0.7f);
//    MaxAttrChance = sConfigMgr->GetOption<float>("RandomEnchants.MaxAttrChance", 40.0f);
//    ReRandChance = sConfigMgr->GetOption<float>("RandomEnchants.ReRandChance", 80.0f);
//}

// 去掉首尾空白（客户端输入框可能带上空格）
static std::string TrimString(std::string const& s)
{
    std::size_t begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return std::string();

    std::size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

bool ParseTransmogScaleFactor(std::string const& text, float& outFactor)
{
    std::string s = TrimString(text);
    if (s.empty())
        return false;

    // 只接受纯数字（可含小数点），不接受 "1.25倍" 这类带多余字符的输入
    errno = 0;
    char* end = nullptr;
    double value = std::strtod(s.c_str(), &end);
    // isfinite 用来挡 "nan"/"inf"：strtod 能成功解析它们，且 NaN 经过 clamp 后仍是 NaN
    if (errno == ERANGE || !end || *end != '\0' || !std::isfinite(value))
        return false;

    // 夹取到合法区间，超范围不视为失败（按边界处理，方便玩家直接填 3 表示最大）
    outFactor = std::clamp(static_cast<float>(value), TRANSMOG_SCALE_FACTOR_MIN, TRANSMOG_SCALE_FACTOR_MAX);
    return true;
}

std::string FormatTransmogScaleFactor(float factor)
{
    std::ostringstream str;
    str << std::fixed << std::setprecision(2)
        << std::clamp(factor, TRANSMOG_SCALE_FACTOR_MIN, TRANSMOG_SCALE_FACTOR_MAX) << " 倍";
    return str.str();
}

std::string MakeTransmogScaleHint(float currentFactor)
{
    std::ostringstream str;
    str << "请输入缩放系数(0.5-1.5),当前值:" << std::fixed << std::setprecision(2)
        << std::clamp(currentFactor, TRANSMOG_SCALE_FACTOR_MIN, TRANSMOG_SCALE_FACTOR_MAX);
    return str.str();
}

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

    // 追加加载佣兵幻形数据（角色级）
    LoadBotTransmogData();
    // 追加加载玩家变形缩放设置（角色级）
    LoadPlayerTransmogSetData();
}
bool PlayerTransmog::CastTransmogBot(Creature* bot, uint32 modelId, float scaleFactor /*= TRANSMOG_SCALE_FACTOR_DEFAULT*/)
{
    if (!bot || !bot->IsNPCBot()) return false;

    // 1) 校验幻形模型（用 LookupEntry，避免无效 DisplayId 触发断言崩溃）
    CreatureDisplayInfoEntry const* minfo = sCreatureDisplayInfoStore.LookupEntry(modelId);
    if (!minfo) return false;

    // 2) 取 BOT 模板唯一模型作为高度基准（BOT 单模型，entry 唯一对应一个生物）
    CreatureModel const* tmpl = bot->GetCreatureTemplate()->GetFirstValidModel();
    if (!tmpl) return false;

    // 3) 计算缩放（与 Creature::GetNativeObjectScale() 共用同一实现，保证复活/重生后口径一致）：
    //    命中 DisplayId 硬编码表时直接取表内基准缩放；否则先按高度归一到 BOT 原高度
    //    （缩放后高度不小于原本高度），再视目标模型高出幅度做阶梯加成
    //    （高出 50% 以上 +10%，高出 100% 以上 +20%）
    float scale = 0.f;
    if (!Creature::CalculateBotTransmogScale(tmpl->CreatureDisplayID, modelId, tmpl->DisplayScale, scale))
        return false;                         // 幻形模型数据异常，放弃幻形

    // 3.1) 乘上玩家为该佣兵设置的最终缩放系数（0.5 - 1.5），并把系数写进 BOT，
    //      让核心 GetNativeObjectScale() 在复活/光环重算时保持同一口径
    float factor = std::clamp(scaleFactor, TRANSMOG_SCALE_FACTOR_MIN, TRANSMOG_SCALE_FACTOR_MAX);
    scale *= factor;
    bot->SetBotTransmogScaleFactor(factor);

    // 4) 关键：需要展示角色外观的 BOT 条目在 creature_outfits 里，核心会打上
    //    UNIT_FLAG2_MIRROR_IMAGE（ObjectMgr.cpp:9799 "Needed so client requests mirror packet"）。
    //    客户端据此向服务端请求 SMSG_MIRRORIMAGE_DATA，并用**BOT 自己的**种族/肤色/脸/发型
    //    + 装备外观去渲染当前的 displayId —— 于是幻形目标模型被 BOT 的角色贴图覆盖（纯白/错贴图）。
    //    幻形期间清除该标记，让客户端按普通生物走 CreatureDisplayInfo 的 TextureVariation 渲染；
    //    恢复原形时由 RestoreBotTransmog() 依据模板还原该标记。
    bot->RemoveUnitFlag2(UNIT_FLAG2_MIRROR_IMAGE);

    // 5) 直接写原生显示 ID + 显示 ID + 缩放（不挂任何光环，从根上保证死亡/复活/被覆盖后仍恢复幻形）
    bot->SetNativeDisplayId(modelId);
    bot->SetDisplayId(modelId, scale);

    return true;
}
bool PlayerTransmog::CastTransmog(Player* player, int modelid)
{
    player->RemoveAurasByType(SPELL_AURA_TRANSFORM);
    CreatureDisplayInfoEntry const* minfo = sCreatureDisplayInfoStore.AssertEntry(modelid);
    if (!minfo) return false;

    // 与佣兵幻形共用同一套缩放算法（Creature::CalculateBotTransmogScale）：
    //   命中 DisplayId 硬编码表时取表内基准缩放，否则走高度归一（幻形后高度 = 玩家原高度）+ 大模型阶梯加成；
    //   玩家自己的缩放系数（0.5 - 1.5，见下）叠乘在这个基准之上，硬编码命中的模型同样受该系数影响。
    //   基准取玩家自己的种族模型（displayId_m/f，见 Player::InitDisplayIds），玩家的 object scale
    //   默认是 1.0，故 srcDisplayScale 传 1.0。
    float scale = 1.0f;
    if (!Creature::CalculateBotTransmogScale(player->GetNativeDisplayId(), static_cast<uint32>(modelid), 1.0f, scale))
        scale = 1.0f;                          // 幻形模型数据异常时退回不缩放

    // 乘上该玩家自己设置的变形缩放系数（0.5 - 1.5，持久化在 mod_player_transmog_set）
    scale *= GetPlayerTransmogScaleFactor(player->GetGUID().GetCounter());

    // 先记录本次幻形（模型 + 缩放），供 100004 光环在「被其他变形覆盖后恢复」时还原为所选幻形
    SetPlayerTransmog(player, static_cast<uint32>(modelid), scale);

    if (player->CastSpell(player->ToUnit(), 100004, false) == SPELL_CAST_OK) {
        player->SetDisplayId(modelid, scale);
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

//==================== 佣兵幻形：数据读写与恢复 ====================

void PlayerTransmog::LoadBotTransmogData()
{
    std::lock_guard<std::mutex> lock(_botTransmogMutex);
    BotTransmogStore.clear();
    QueryResult result = CharacterDatabase.Query(
        "SELECT character_id, bot_entry, model_id, model_name, scale_factor FROM mod_player_bot_transmog");
    if (!result) return;
    do
    {
        Field* f = result->Fetch();
        BotTransmogData d;
        uint32 cid = f[0].Get<uint32>();
        d.bot_entry    = f[1].Get<uint32>();
        d.model_id     = f[2].Get<uint32>();
        d.model_name   = f[3].Get<std::string>();
        d.scale_factor = std::clamp(f[4].Get<float>(), TRANSMOG_SCALE_FACTOR_MIN, TRANSMOG_SCALE_FACTOR_MAX);
        BotTransmogStore[cid][d.bot_entry] = d;
    } while (result->NextRow());
}

void PlayerTransmog::LoadPlayerTransmogSetData()
{
    std::lock_guard<std::mutex> lock(_playerScaleMutex);
    PlayerScaleStore.clear();
    QueryResult result = CharacterDatabase.Query(
        "SELECT character_id, scale_factor FROM mod_player_transmog_set");
    if (!result) return;
    do
    {
        Field* f = result->Fetch();
        PlayerScaleStore[f[0].Get<uint32>()] =
            std::clamp(f[1].Get<float>(), TRANSMOG_SCALE_FACTOR_MIN, TRANSMOG_SCALE_FACTOR_MAX);
    } while (result->NextRow());
}

float PlayerTransmog::GetPlayerTransmogScaleFactor(uint32 characterId) const
{
    std::lock_guard<std::mutex> lock(_playerScaleMutex);
    auto it = PlayerScaleStore.find(characterId);
    return it != PlayerScaleStore.end() ? it->second : TRANSMOG_SCALE_FACTOR_DEFAULT;
}

void PlayerTransmog::SetPlayerTransmogScaleFactor(uint32 characterId, float scaleFactor)
{
    float factor = std::clamp(scaleFactor, TRANSMOG_SCALE_FACTOR_MIN, TRANSMOG_SCALE_FACTOR_MAX);
    {
        std::lock_guard<std::mutex> lock(_playerScaleMutex);
        PlayerScaleStore[characterId] = factor;
    }

    // DB 写入放到锁外执行（异步，避免阻塞主线程）
    CharacterDatabase.AsyncQuery(Acore::StringFormat(
        "INSERT INTO mod_player_transmog_set (character_id, scale_factor) VALUES ({}, {}) "
        "ON DUPLICATE KEY UPDATE scale_factor=VALUES(scale_factor)", characterId, factor));
}

std::optional<BotTransmogData> PlayerTransmog::GetBotTransmog(uint32 characterId, uint32 botEntry)
{
    std::lock_guard<std::mutex> lock(_botTransmogMutex);
    auto cit = BotTransmogStore.find(characterId);
    if (cit == BotTransmogStore.end()) return std::nullopt;
    auto it = cit->second.find(botEntry);
    if (it == cit->second.end()) return std::nullopt;
    return it->second;   // 返回副本，避免锁释放后指针悬垂
}

void PlayerTransmog::SetBotTransmog(uint32 cid, uint32 botEntry, uint32 modelId, std::string const& modelName,
                                    float scaleFactor /*= TRANSMOG_SCALE_FACTOR_DEFAULT*/)
{
    float factor = std::clamp(scaleFactor, TRANSMOG_SCALE_FACTOR_MIN, TRANSMOG_SCALE_FACTOR_MAX);
    {
        std::lock_guard<std::mutex> lock(_botTransmogMutex);
        BotTransmogData d{ botEntry, modelId, modelName, factor };
        BotTransmogStore[cid][botEntry] = d;
    }

    // DB 写入较慢，放到锁外执行，避免长时间持有锁
    std::string escapedName = modelName;
    CharacterDatabase.EscapeString(escapedName);
    std::string sql = Acore::StringFormat(
        "INSERT INTO mod_player_bot_transmog (character_id, bot_entry, model_id, model_name, scale_factor) "
        "VALUES ({}, {}, {}, '{}', {}) ON DUPLICATE KEY UPDATE model_id=VALUES(model_id), "
        "model_name=VALUES(model_name), scale_factor=VALUES(scale_factor)",
        cid, botEntry, modelId, escapedName, factor);
    CharacterDatabase.AsyncQuery(sql);
}

void PlayerTransmog::SetBotTransmogScale(uint32 cid, uint32 botEntry, float scaleFactor)
{
    float factor = std::clamp(scaleFactor, TRANSMOG_SCALE_FACTOR_MIN, TRANSMOG_SCALE_FACTOR_MAX);
    {
        std::lock_guard<std::mutex> lock(_botTransmogMutex);
        // 只改系数，模型保持不动（调用方保证该佣兵已有幻形记录）
        BotTransmogData& d = BotTransmogStore[cid][botEntry];
        d.bot_entry    = botEntry;
        d.scale_factor = factor;
    }

    // 只改系数：直接用 UPDATE 改已有记录。
    // 该佣兵必然已有幻形记录（菜单在没有记录时不给输入框），故不再考虑「没有记录」时的插入，
    // 也就不会出现 INSERT ... ON DUPLICATE KEY UPDATE 占位行在唯一键不命中时另插一行的问题；
    // UPDATE 也不依赖 (character_id, bot_entry) 唯一键，只要记录存在就一定能改到。
    std::string const sql = Acore::StringFormat(
        "UPDATE mod_player_bot_transmog SET scale_factor = {} WHERE character_id = {} AND bot_entry = {}",
        factor, cid, botEntry);
    CharacterDatabase.AsyncQuery(sql);
}

float PlayerTransmog::GetBotTransmogScaleFactor(uint32 characterId, uint32 botEntry) const
{
    std::lock_guard<std::mutex> lock(_botTransmogMutex);
    auto cit = BotTransmogStore.find(characterId);
    if (cit == BotTransmogStore.end())
        return TRANSMOG_SCALE_FACTOR_DEFAULT;

    auto it = cit->second.find(botEntry);
    return it != cit->second.end() ? it->second.scale_factor : TRANSMOG_SCALE_FACTOR_DEFAULT;
}

void PlayerTransmog::RemoveBotTransmog(uint32 cid, uint32 botEntry)
{
    {
        std::lock_guard<std::mutex> lock(_botTransmogMutex);
        auto cit = BotTransmogStore.find(cid);
        if (cit != BotTransmogStore.end())
        {
            cit->second.erase(botEntry);
            if (cit->second.empty())
                BotTransmogStore.erase(cit);
        }
    }

    // DB 删除放到锁外执行
    std::string sql = Acore::StringFormat(
        "DELETE FROM mod_player_bot_transmog WHERE character_id={} AND bot_entry={}", cid, botEntry);
    CharacterDatabase.AsyncQuery(sql);
}

std::vector<BotTransmogData> PlayerTransmog::GetBotTransmogEntries(uint32 characterId) const
{
    std::lock_guard<std::mutex> lock(_botTransmogMutex);
    std::vector<BotTransmogData> entries;
    auto cit = BotTransmogStore.find(characterId);
    if (cit != BotTransmogStore.end())
    {
        for (auto const& [entry, d] : cit->second)
            if (d.model_id)
                entries.push_back(d);
    }
    return entries;
}

void PlayerTransmog::RestoreBotTransmog(Creature* bot)
{
    if (!bot) return;

    // 恢复到模板唯一模型的显示 ID 与 scale（BOT 单模型，直接取模板）
    CreatureModel const* tmpl = bot->GetCreatureTemplate()->GetFirstValidModel();
    if (!tmpl) return;

    // 恢复原形时清掉玩家自定义缩放系数，避免残留影响后续幻形计算
    bot->SetBotTransmogScaleFactor(TRANSMOG_SCALE_FACTOR_DEFAULT);

    bot->SetNativeDisplayId(tmpl->CreatureDisplayID);
    bot->SetDisplayId(tmpl->CreatureDisplayID, tmpl->DisplayScale);

    // 还原"角色外观（镜像）"标记：登记在 creature_outfits 的 BOT 需要在客户端显示角色外观+装备
    if (bot->GetCreatureTemplate()->unit_flags2 & UNIT_FLAG2_MIRROR_IMAGE)
        bot->SetUnitFlag2(UNIT_FLAG2_MIRROR_IMAGE);
}

bool PlayerTransmog::IsBotTransmogApplied(Creature* bot, uint32 modelId) const
{
    if (!bot)
        return false;

    if (bot->GetDisplayId() != modelId || bot->GetNativeDisplayId() != modelId)
        return false;

    // 幻形期间必须清掉 UNIT_FLAG2_MIRROR_IMAGE，否则客户端仍按角色（镜像）管线渲染
    if (bot->HasUnitFlag2(UNIT_FLAG2_MIRROR_IMAGE))
        return false;

    // 缩放也必须与核心算出的"原生缩放"（归一缩放 × 玩家系数）一致：
    // 某些只恢复显示 ID、不带缩放的重置路径（如战斗中被变形术变形、变形术失效时的
    // RestoreDisplayId）会把 ObjectScale 冲掉。只校验显示 ID 会漏判，导致幻形模型
    // 还在、大小却回到未缩放状态，巡检也无法把它纠正回来。
    float const nativeScale = bot->GetNativeObjectScale();
    if (nativeScale > 0.f && std::fabs(bot->GetObjectScale() - nativeScale) > 0.001f)
        return false;

    return true;
}

void PlayerTransmog::SetPlayerTransmog(Player* player, uint32 modelid, float scale)
{
    if (!player) return;
    PlayerTransmogState state{ modelid, scale };
    PlayerTransmogStore[player->GetGUID()] = state;
}

std::optional<PlayerTransmogState> PlayerTransmog::GetPlayerTransmog(Player* player) const
{
    if (!player) return std::nullopt;
    auto it = PlayerTransmogStore.find(player->GetGUID());
    if (it == PlayerTransmogStore.end()) return std::nullopt;
    return it->second;   // 返回副本，避免引用悬垂
}

void PlayerTransmog::ClearPlayerTransmog(Player* player)
{
    if (!player) return;
    PlayerTransmogStore.erase(player->GetGUID());
}



