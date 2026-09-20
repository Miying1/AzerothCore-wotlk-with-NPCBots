#include "Chat.h"
#include "Config.h" 
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include <PlayerTransmog.h>
#include <ScriptedGossip.h>
#include "Spell.h"
#include "SpellAuraEffects.h"
#include <SpellScript.h>
#include "bot_ai.h"
#include "botmgr.h"
#include "botdatamgr.h"
#include "ObjectAccessor.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"

class TransmogItem_WorldScript : public WorldScript
{
public:
    TransmogItem_WorldScript() : WorldScript("TransmogItem_WorldScript") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {

    }
    void OnStartup() override {
        pTransmog->InitData();
    }
};

enum TransmogItemEnum
{
    GOSSIP_SENDER_BACK_HOME = 1,
    GOSSIP_SENDER_PT = 2,
    GOSSIP_SENDER_JY = 3,
    GOSSIP_SENDER_XY = 4,
    GOSSIP_SENDER_BOSS = 5,
    GOSSIP_SENDER_MODEL_INFO = 100,
    GOSSIP_SENDER_FLAG_OK = 2000,
    GOSSIP_SENDER_FLAG_NONE = 2100,
    GOSSIP_SENDER_DEL = 3000,
    GOSSIP_SENDER_USE = 4000,

    // 佣兵幻形菜单
    GOSSIP_SENDER_BOT_MAIN     = 5000,   // 点击「佣兵幻形」入口
    GOSSIP_SENDER_BOT_SELECT   = 5100,   // 选中某个 BOT（action = bot_entry）
    GOSSIP_SENDER_BOT_CATEGORY = 5200,   // 选中分类（action = quality）
    GOSSIP_SENDER_BOT_MODEL    = 5300,   // 选中幻象（action = model_id）
    GOSSIP_SENDER_BOT_BACK     = 5400    // 返回上一级

};

// 佣兵幻形：消耗品与问候语文本 ID
constexpr uint32 BOT_TRANSMOG_COIN_ENTRY     = 63000;  // 幸运币 item entry（每次幻形消耗 1 枚）
constexpr uint32 BOT_TRANSMOG_GOSSIP_TEXT_ID = 60701;  // 佣兵幻形 BOT 列表菜单问候语 npc_text ID

// 多级状态携带：哈哈镜是无状态 gossip，用模块级瞬态缓存记录当前选中 BOT
std::unordered_map<ObjectGuid, uint32> BotTransmogSelectedEntry; // player guid -> 当前选中 bot_entry

class TransmogItemScript : public ItemScript
{
private:
    int textId = 60700;
public:
    TransmogItemScript() : ItemScript("TransmogItemScript") { }

    bool OnUse(Player* player, Item* item, const SpellCastTargets&) override
    {
        if (player->IsInCombat())
            return false;
        HelleGossip(player, item);
        return true;
    }
    void HelleGossip(Player* player, Item* item) {
        player->PlayerTalkClass->ClearMenus();
        uint16 account_id = player->GetSession()->GetAccountId();
        if (pTransmog->CollectionDataStore.find(account_id) != pTransmog->CollectionDataStore.end()) {
            CcList collectionDatas = pTransmog->CollectionDataStore[account_id];
            for (auto its = collectionDatas.begin(); its != collectionDatas.end(); ++its)
            { 
               AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, pTransmog->GetModelNameText(&(*its)), GOSSIP_SENDER_USE, (*its).modelid); 
            }
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "普通幻象", GOSSIP_SENDER_PT, 0);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "精英幻象", GOSSIP_SENDER_JY, 1);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "稀有幻象", GOSSIP_SENDER_XY, 2);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "史诗幻象", GOSSIP_SENDER_BOSS, 3);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "佣兵幻形", GOSSIP_SENDER_BOT_MAIN, 0);
        SendGossipMenuFor(player, textId, item->GetGUID());
    }

    void  OnGossipSelect(Player* player, Item* item, uint32  sender, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();
        uint16 account_id = player->GetSession()->GetAccountId();
        uint32 modelId = action;
        Unit* target = player->GetSelectedUnit();

        switch (sender)
        {
        case GOSSIP_SENDER_BACK_HOME:
            HelleGossip(player, item);
            return;
        case GOSSIP_SENDER_PT:
        case GOSSIP_SENDER_JY:
        case GOSSIP_SENDER_XY:
        case GOSSIP_SENDER_BOSS:
            SendModelNameList(player, account_id, sender, action);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_BACK_HOME, 0);
            break;
        case GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_PT:
        case GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_JY:
        case GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_XY:
        case GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_BOSS:
            SendModelInfo(player, account_id, sender, action);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", sender - GOSSIP_SENDER_MODEL_INFO, sender - GOSSIP_SENDER_MODEL_INFO - 2);
            break;
        case GOSSIP_SENDER_FLAG_OK + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_PT:
        case GOSSIP_SENDER_FLAG_OK + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_JY:
        case GOSSIP_SENDER_FLAG_OK + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_XY:
        case GOSSIP_SENDER_FLAG_OK + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_BOSS:  
            if (pTransmog->SetCcFlag(account_id, action, 1) == ERROR_CCMax) {
                ChatHandler(player->GetSession()).SendSysMessage("你的收藏列表已经满了,最多15个收藏!");
            }
            OnGossipSelect(player, item, sender - GOSSIP_SENDER_FLAG_OK, action);
            break;
        case GOSSIP_SENDER_FLAG_NONE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_PT:
        case GOSSIP_SENDER_FLAG_NONE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_JY:
        case GOSSIP_SENDER_FLAG_NONE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_XY:
        case GOSSIP_SENDER_FLAG_NONE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_BOSS:
            pTransmog->SetCcFlag(account_id, action, 0);
            OnGossipSelect(player, item, sender - GOSSIP_SENDER_FLAG_NONE, action);
            break;
            //删除
        case GOSSIP_SENDER_DEL + GOSSIP_SENDER_PT:
        case GOSSIP_SENDER_DEL + GOSSIP_SENDER_JY:
        case GOSSIP_SENDER_DEL + GOSSIP_SENDER_XY:
        case GOSSIP_SENDER_DEL + GOSSIP_SENDER_BOSS:
            pTransmog->DeleteCcModel(account_id, action);
            OnGossipSelect(player, item, sender - GOSSIP_SENDER_DEL, action);
            break;
        case GOSSIP_SENDER_USE://变身
            CloseGossipMenuFor(player); 
            // if (target && target->IsNPCBot() && target->ToCreature()->GetBotAI()->GetBotOwner()==player) {
            //    pTransmog->CastTransmogBot(target, action);
            //    return;
            // } 
            pTransmog->CastTransmog(player, action); 
            return;
        case GOSSIP_SENDER_USE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_PT:
        case GOSSIP_SENDER_USE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_JY:
        case GOSSIP_SENDER_USE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_XY:
        case GOSSIP_SENDER_USE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_BOSS:
            pTransmog->CastTransmog(player, action);
            OnGossipSelect(player, item, sender - GOSSIP_SENDER_USE, action);
            break;
        //==================== 佣兵幻形 ====================
        case GOSSIP_SENDER_BOT_MAIN:
            ShowBotList(player, item);
            return;
        case GOSSIP_SENDER_BOT_SELECT:
            BotTransmogSelectedEntry[player->GetGUID()] = action;   // 记住选中的 bot_entry
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "普通幻象", GOSSIP_SENDER_BOT_CATEGORY, 0);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "精英幻象", GOSSIP_SENDER_BOT_CATEGORY, 1);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "稀有幻象", GOSSIP_SENDER_BOT_CATEGORY, 2);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "史诗幻象", GOSSIP_SENDER_BOT_CATEGORY, 3);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_BOT_MAIN, 0);
            SendGossipMenuFor(player, textId, item->GetGUID());
            return;
        case GOSSIP_SENDER_BOT_CATEGORY:
            ShowBotModelList(player, item, account_id, action);
            return;
        case GOSSIP_SENDER_BOT_MODEL:
            ApplyBotTransmog(player, action);
            return;
        }

        SendGossipMenuFor(player, textId, item->GetGUID());
    }

    bool SendModelNameList(Player* player, uint32 account_id, uint32 sender, uint32 action) {

        QualityGroupMap* qGroupData = pTransmog->GetAccountQualityGroupMap(account_id);
        if (!qGroupData) return false;
        QualityGroupMap::iterator its = qGroupData->find(action);
        if (its != qGroupData->end() && !(*its).second.empty()) {
            ModelDataList* list = &(*its).second;
            uint8 count=0;
            for (auto it = list->begin(); it != list->end(); ++it)
            {
                if(count>=30){
                   ChatHandler(player->GetSession()).SendNotification("你的该类幻象已超出30个,需删除部分幻象,保持30个幻象内!"); 
                  return true;
                }
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, pTransmog->GetModelNameText(&(*it)), GOSSIP_SENDER_MODEL_INFO + sender, (*it).modelid);
                ++count;
            }
        }
        return true;
    }
    void SendModelInfo(Player* player, uint32 account_id, uint32 sender, uint32 modelId) {
        ModelData* mData = pTransmog->GetModelDataById(account_id, modelId);
        if (mData) {
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "变身", GOSSIP_SENDER_USE + sender, modelId);
            if (mData->ccflag == 0)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, "收藏", GOSSIP_SENDER_FLAG_OK + sender, modelId);
            else
            {
                AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "取消收藏", GOSSIP_SENDER_FLAG_NONE + sender, modelId);
            }
            std::ostringstream str;
            str << "是否确定删除 " << pTransmog->GetModelNameText(mData) << " 幻象?";
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "删除", GOSSIP_SENDER_DEL + (sender - GOSSIP_SENDER_MODEL_INFO), modelId, str.str(), 0, false);
        }
    }

    //==================== 佣兵幻形菜单 ====================
    // 列出当前雇佣的 BOT
    void ShowBotList(Player* player, Item* item)
    {
        player->PlayerTalkClass->ClearMenus();
        BotMap const* bots = player->GetBotMgr()->GetBotMap();
        for (auto const& [_, bot] : *bots)
        {
            if (!bot) continue;
            std::ostringstream str;
            str << "[" << bot->GetEntry() << "]" << bot->GetName();
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, str.str(), GOSSIP_SENDER_BOT_SELECT, bot->GetEntry());
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_BACK_HOME, 0);
        // 用专属问候语 textId，说明「幻形需消耗 1 枚幸运币」
        SendGossipMenuFor(player, BOT_TRANSMOG_GOSSIP_TEXT_ID, item->GetGUID());
    }

    // 列出所选分类下的幻象（选中即弹确认框）
    void ShowBotModelList(Player* player, Item* item, uint32 account_id, uint32 quality)
    {
        uint32 bot_entry = BotTransmogSelectedEntry[player->GetGUID()];
        QualityGroupMap* qg = pTransmog->GetAccountQualityGroupMap(account_id);
        auto it = qg->find(quality);
        if (it != qg->end())
        {
            for (auto& m : it->second)
            {
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, pTransmog->GetModelNameText(&m),
                                 GOSSIP_SENDER_BOT_MODEL, m.modelid,
                                 "给佣兵幻形需要消耗 1 枚幸运币，是否确定？", 0, false);
            }
        }
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回...", GOSSIP_SENDER_BOT_SELECT, bot_entry);
        SendGossipMenuFor(player, textId, item->GetGUID());
    }

    // 应用幻形：校验 BOT 在场、幸运币充足，成功后消耗并持久化
    void ApplyBotTransmog(Player* player, uint32 model_id)
    {
        uint32 bot_entry  = BotTransmogSelectedEntry[player->GetGUID()];
        uint16 account_id = player->GetSession()->GetAccountId();

        Creature* bot = nullptr;
        for (auto const& [_, b] : *player->GetBotMgr()->GetBotMap())
        {
            if (b && b->GetEntry() == bot_entry)
            {
                bot = b;
                break;
            }
        }

        if (!bot || !bot->IsInWorld() || !bot->IsAlive())
        {
            ChatHandler(player->GetSession()).SendSysMessage("该 BOT 当前不在场，无法幻形。");
        }
        else if (!player->HasItemCount(BOT_TRANSMOG_COIN_ENTRY, 1))
        {
            ChatHandler(player->GetSession()).SendSysMessage("幸运币不足，无法给佣兵幻形。");
        }
        else if (pTransmog->CastTransmogBot(bot, model_id))
        {
            player->DestroyItemCount(BOT_TRANSMOG_COIN_ENTRY, 1, true);   // 幻形成功后消耗 1 枚幸运币
            ModelData* m = pTransmog->GetModelDataById(account_id, model_id);
            std::string name = m ? m->modelname : std::to_string(model_id);
            pTransmog->SetBotTransmog(player->GetGUID().GetCounter(), bot_entry, model_id, name); // 持久化
            ChatHandler(player->GetSession()).SendSysMessage("BOT 幻形成功。");
        }
        else
        {
            ChatHandler(player->GetSession()).SendSysMessage("幻形失败。");
        }
        CloseGossipMenuFor(player);
    }
};
 
class TransmogCcJewel_ALLScript : public ItemScript
{
public:
    TransmogCcJewel_ALLScript() : ItemScript("TransmogCcJewel_ALLScript") { }

    bool OnUse(Player* player, Item* item, const SpellCastTargets&) override
    {
        ChatHandler ch(player->GetSession());
        Unit* target = player->GetSelectedUnit();
        if (!target) {
            ch.SendNotification("我还没有目标!"); 
            return false;
        }
        Creature* creature = target->ToCreature();
        if (!creature || creature->IsNPCBot()) {
            ch.SendSysMessage("这是一个无效的目标");
            //target->Whisper("这是一个无效的目标", LANG_UNIVERSAL, player, true);
            return false;
        }
        auto ctemp = creature->GetCreatureTemplate();
        if (ctemp->type == 8 || ctemp->type == 10 || ctemp->type == 12) {
            ch.SendSysMessage("这是一个无效的目标");
            //target->Whisper("这是一个无效的目标", LANG_UNIVERSAL, player, true);
            return false;
        }
        //ctemp->type_flags|
        if ((ctemp->rank == 3 || creature->isWorldBoss()) && creature->HealthAbovePct(40)) {
            ch.SendSysMessage("这个目标他太强大了，你需要先削弱他！");
            //target->Whisper("这是一个目标他太强大了，你需要先削弱他！", LANG_UNIVERSAL, player, true);
            return false;
        }

        uint16 account_id = player->GetSession()->GetAccountId();
        if (auto data = pTransmog->AddModelData(account_id, creature)) {
            std::ostringstream msg;
            msg << "[" << pTransmog->GetModelNameText(data) << "]幻象已成功收集到了你的哈哈镜中。";
            ch.SendSysMessage(msg.str());
            //target->Whisper(msg.str(), LANG_UNIVERSAL, player, true);
            player->DestroyItemCount(item->GetEntry(), 1, true);
        }
        else
        {
            ch.SendSysMessage("失败了,你可能已经拥有了这个幻象！或该类幻象已满30个");
            //target->Whisper("哦！No!失败了,Why??", LANG_UNIVERSAL, player, true);
            return false;
        }
        return true;
    }
};
//圣灵
class TransmogCcJewel_BossScript : public ItemScript
{
public:
    TransmogCcJewel_BossScript() : ItemScript("TransmogCcJewel_BossScript") { }

    bool OnUse(Player* player, Item* item, const SpellCastTargets&) override
    {
        ChatHandler ch(player->GetSession());
        Unit* target = player->GetSelectedUnit();
        if (!target) {
            ch.SendNotification("我还没有目标!"); 
            return false;
        }
        Creature* creature = target->ToCreature();
        if (!creature || creature->IsNPCBot()) { 
            ch.SendNotification("这是一个无效的目标!");
            return false;
        }
        auto ctemp = creature->GetCreatureTemplate();
        if (ctemp->type == 8 || ctemp->type == 10 || ctemp->type == 12 || !(ctemp->rank == 3 || creature->isWorldBoss())) {
            ch.SendNotification("这是一个无效的目标!");
            return false;
        }
        //ctemp->type_flags|
        if ((ctemp->rank == 3 || creature->isWorldBoss()) && creature->HealthAbovePct(40)) {
            ch.SendSysMessage("这个目标他太强大了，你需要先削弱他！"); 
            return false;
        }

        uint16 account_id = player->GetSession()->GetAccountId();
        if (auto data = pTransmog->AddModelData(account_id, creature)) {
            std::ostringstream msg;
            msg << "[" << pTransmog->GetModelNameText(data) << "]幻象已成功收集到了你的哈哈镜中。";
            ch.SendSysMessage(msg.str()); 
            player->DestroyItemCount(item->GetEntry(), 1, true);
        }
        else
        {
            ch.SendSysMessage("失败了,你可能已经拥有了这个幻象！或该类幻象已满30个"); 
            return false;
        }
        return true;
    }
 
};
//魅影珠
class TransmogCcJewel_XYScript : public ItemScript
{
public:
    TransmogCcJewel_XYScript() : ItemScript("TransmogCcJewel_XYScript") { }

    bool OnUse(Player* player, Item* item, const SpellCastTargets&) override
    {
        ChatHandler ch(player->GetSession());
        Unit* target = player->GetSelectedUnit();
        if (!target) {
            ch.SendNotification("我还没有目标!");
            return false;
        }
        Creature* creature = target->ToCreature();
        if (!creature || creature->IsNPCBot()) {
            ch.SendNotification("这是一个无效的目标!");
            return false;
        }
        auto ctemp = creature->GetCreatureTemplate();

        if (ctemp->type == 8 || ctemp->type == 10 || ctemp->type == 12 || !(ctemp->rank == 2 || ctemp->rank == 4)) {
            ch.SendNotification("这是一个无效的目标!");
            return false;
        } 
        uint16 account_id = player->GetSession()->GetAccountId();
        if (auto data = pTransmog->AddModelData(account_id, creature)) {
            std::ostringstream msg;
            msg << "[" << pTransmog->GetModelNameText(data) << "]幻象已成功收集到了你的哈哈镜中。";
            ch.SendSysMessage(msg.str());
            player->DestroyItemCount(item->GetEntry(), 1, true);
        }
        else
        {
            ch.SendSysMessage("失败了,你可能已经拥有了这个幻象！或该类幻象已满30个");
            return false;
        }
        return true;
    }

};

//觅影珠
class TransmogCcJewel_JYScript : public ItemScript
{
public:
    TransmogCcJewel_JYScript() : ItemScript("TransmogCcJewel_JYScript") { }

    bool OnUse(Player* player, Item* item, const SpellCastTargets&) override
    {
        ChatHandler ch(player->GetSession());
        Unit* target = player->GetSelectedUnit();
        if (!target) {
            ch.SendNotification("我还没有目标!");
            return false;
        }
        Creature* creature = target->ToCreature();
        if (!creature || creature->IsNPCBot()) {
            ch.SendNotification("这是一个无效的目标!");
            return false;
        }
        auto ctemp = creature->GetCreatureTemplate();

        if (ctemp->type == 8 || ctemp->type == 10 || ctemp->type == 12 || ctemp->rank != 1) {
            ch.SendNotification("这是一个无效的目标!");
            return false;
        }
        uint16 account_id = player->GetSession()->GetAccountId();
        if (auto data = pTransmog->AddModelData(account_id, creature)) {
            std::ostringstream msg;
            msg << "[" << pTransmog->GetModelNameText(data) << "]幻象已成功收集到了你的哈哈镜中。";
            ch.SendSysMessage(msg.str());
            player->DestroyItemCount(item->GetEntry(), 1, true);
        }
        else
        {
            ch.SendSysMessage("失败了,你可能已经拥有了这个幻象！或该类幻象已满30个");
            return false;
        }
        return true;
    }

};

//幻影珠
class TransmogCcJewel_PTScript : public ItemScript
{
public:
    TransmogCcJewel_PTScript() : ItemScript("TransmogCcJewel_PTScript") { }

    bool OnUse(Player* player, Item* item, const SpellCastTargets&) override
    {
        ChatHandler ch(player->GetSession());
        Unit* target = player->GetSelectedUnit();
        if (!target) {
            ch.SendNotification("我还没有目标!");
            return false;
        }
        Creature* creature = target->ToCreature();
        if (!creature || creature->IsNPCBot()) {
            ch.SendNotification("这是一个无效的目标!");
            return false;
        }
        auto ctemp = creature->GetCreatureTemplate();

        if (ctemp->type == 8 || ctemp->type == 10 || ctemp->type == 12 || ctemp->rank != 0) {
            ch.SendNotification("这是一个无效的目标!");
            return false;
        }
        uint16 account_id = player->GetSession()->GetAccountId();
        if (auto data = pTransmog->AddModelData(account_id, creature)) {
            std::ostringstream msg;
            msg << "[" << pTransmog->GetModelNameText(data) << "]幻象已成功收集到了你的哈哈镜中。";
            ch.SendSysMessage(msg.str());
            player->DestroyItemCount(item->GetEntry(), 1, true);
        }
        else
        {
            ch.SendSysMessage("失败了,你可能已经拥有了这个幻象！或该类幻象已满30个");
            return false;
        }
        return true;
    }

};
//使用物品收藏变身模型
class PlayerTransmog_ItemScript : public ItemScript
{
public:
    PlayerTransmog_ItemScript() : ItemScript("PlayerTransmog_ItemScript") { }

    bool OnUse(Player* player, Item* item, const SpellCastTargets&) override
    {
        ChatHandler ch(player->GetSession()); 
        uint16 account_id = player->GetSession()->GetAccountId();
        ItemTemplate const* proto = item->GetTemplate();
        std::string name = proto->Name1;
        uint32 modelid= static_cast<uint32>(std::stoll(proto->Description));
        if (auto data= pTransmog->AddModelDataById(account_id, name, modelid,3)) {
            std::ostringstream msg;
            msg << "[" << pTransmog->GetModelNameText(data) << "]幻象已成功收集到了你的哈哈镜中。";
            ch.SendSysMessage(msg.str());
            player->DestroyItemCount(item->GetEntry(), 1, true);
        }
        else
        {
            ch.SendSysMessage("失败了,你可能已经拥有了这个幻象！或该类幻象已满30个");
            return false;
        }
        return true;
    }

};

class spell_player_transmog : public SpellScriptLoader
{
public:
    spell_player_transmog() : SpellScriptLoader("spell_player_transmog") { }

    class spell_player_transmog_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_player_transmog_AuraScript);
        //变形光环移除时候，恢复scale
        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (target) {
                target->SetObjectScale(1);
            }
        }

        //变形光环重新应用时（含被其他变形覆盖后由 RestoreDisplayId 恢复），还原为玩家选择的幻形模型
        void OnAfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (!target || !target->IsPlayer())
                return;
            if (std::optional<PlayerTransmogState> state = pTransmog->GetPlayerTransmog(target->ToPlayer()))
                target->SetDisplayId(state->modelid, state->scale);
        }

        void Register() override
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_player_transmog_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
            AfterEffectApply += AuraEffectApplyFn(spell_player_transmog_AuraScript::OnAfterApply, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK);
        }
    };
    AuraScript* GetAuraScript() const override
    {
        return new spell_player_transmog_AuraScript();
    }
};

//==================== 佣兵幻形：生命周期钩子 ====================

// 核心 NPCBot 生命周期（setowner / reset）通知模块应用/恢复幻形
class TransmogBot_UnitScript : public UnitScript
{
public:
    TransmogBot_UnitScript() : UnitScript("TransmogBot_UnitScript") { }

    // ① setowner 成功：应用幻形
    void OnBotSetOwner(Unit* unit, Player* owner) override
    {
        Creature* bot = unit->ToCreature();
        if (!bot || !owner) return;
        uint32 cid = owner->GetGUID().GetCounter();
        auto d = pTransmog->GetBotTransmog(cid, bot->GetEntry());
        if (d && d->model_id)
            pTransmog->CastTransmogBot(bot, d->model_id);
    }

    // ③④ 下线/解雇：恢复原形；解雇额外清理 DB
    void OnBotReset(Unit* unit, uint8 resetType) override
    {
        Creature* bot = unit->ToCreature();
        if (!bot) return;

        // resetType 是位掩码，核心存在组合值（如 LOGOUT | DISMISS），必须用位运算判断
        bool isDismiss = (resetType & BOTAI_RESET_DISMISS) != 0;
        bool isLogout  = (resetType & BOTAI_RESET_LOGOUT)  != 0;
        if (!isDismiss && !isLogout) return;

        pTransmog->RestoreBotTransmog(bot);

        if (isDismiss)
        {
            // 此时 owner 尚未清零（清零发生在 ResetBotAI 返回之后），直接读取即可
            uint32 ownerLow = (bot->GetBotAI() && bot->GetBotAI()->GetBotData()) ? bot->GetBotAI()->GetBotData()->owner : 0;
            if (ownerLow)
                pTransmog->RemoveBotTransmog(ownerLow, bot->GetEntry());
        }
    }
};

// 玩家下线清理瞬态缓存
class TransmogBot_PlayerScript : public PlayerScript
{
public:
    TransmogBot_PlayerScript() : PlayerScript("TransmogBot_PlayerScript") { }

    void OnPlayerLogout(Player* player) override
    {
        BotTransmogSelectedEntry.erase(player->GetGUID());
        pTransmog->ClearPlayerTransmog(player);
    }
};

// 在线期间低频兜底巡检，覆盖极少数直接改 DISPLAYID 而未走 RestoreDisplayId 的路径
class TransmogBot_WorldScript : public WorldScript
{
private:
    uint32 _checkTimer = 30000;   // 30 秒巡检周期

public:
    TransmogBot_WorldScript() : WorldScript("TransmogBot_WorldScript") { }

    void OnUpdate(uint32 diff) override
    {
        if (_checkTimer > diff)
        {
            _checkTimer -= diff;
            return;
        }
        _checkTimer = 30000;   // 30 秒

        // 只遍历在线玩家，避免扫描全部（含离线角色）的幻形记录
        for (auto const& [_, session] : sWorldSessionMgr->GetAllSessions())
        {
            Player* pl = session->GetPlayer();
            if (!pl || !pl->GetBotMgr()) continue;

            uint32 cid = pl->GetGUID().GetCounter();
            auto entries = pTransmog->GetBotTransmogEntries(cid);

            for (auto const& [entry, model_id] : entries)
            {
                for (auto const& [_, bot] : *pl->GetBotMgr()->GetBotMap())
                {
                    if (bot && bot->GetEntry() == entry && bot->IsInWorld() && bot->IsAlive()
                        && (bot->GetDisplayId() != model_id || bot->GetNativeDisplayId() != model_id))
                        pTransmog->CastTransmogBot(bot, model_id);
                }
            }
        }
    }
};

void AddSC_TransmogItemScript()
{
    new TransmogItem_WorldScript();
    new TransmogItemScript();
    new TransmogCcJewel_ALLScript();
    new TransmogCcJewel_BossScript();
    new TransmogCcJewel_XYScript();
    new TransmogCcJewel_JYScript();
    new TransmogCcJewel_PTScript();
    new PlayerTransmog_ItemScript();
    new spell_player_transmog();
    new TransmogBot_UnitScript();
    new TransmogBot_PlayerScript();
    new TransmogBot_WorldScript();
}
