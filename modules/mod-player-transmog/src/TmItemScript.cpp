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
    GOSSIP_SENDER_USE = 4000

};

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
            pTransmog->SetCcFlag(account_id, action, 1);
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
            if (player->IsVip() && target && target->IsNPCBot() && target->ToCreature()->GetBotAI()->GetBotOwner()==player) {
                pTransmog->CastTransmogBot(target, action);
                return;
            } 
            pTransmog->CastTransmog(player, action); 
            return;
        case GOSSIP_SENDER_USE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_PT:
        case GOSSIP_SENDER_USE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_JY:
        case GOSSIP_SENDER_USE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_XY:
        case GOSSIP_SENDER_USE + GOSSIP_SENDER_MODEL_INFO + GOSSIP_SENDER_BOSS:
            pTransmog->CastTransmog(player, action);
            OnGossipSelect(player, item, sender - GOSSIP_SENDER_USE, action);
            break;
        }

        SendGossipMenuFor(player, textId, item->GetGUID());
    }

    bool SendModelNameList(Player* player, uint32 account_id, uint32 sender, uint32 action) {

        QualityGroupMap* qGroupData = pTransmog->GetAccountQualityGroupMap(account_id);
        if (!qGroupData) return false;
        QualityGroupMap::iterator its = qGroupData->find(action);
        if (its != qGroupData->end() && !(*its).second.empty()) {
            ModelDataList* list = &(*its).second;
            for (auto it = list->begin(); it != list->end(); ++it)
            {
                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_2, pTransmog->GetModelNameText(&(*it)), GOSSIP_SENDER_MODEL_INFO + sender, (*it).modelid);
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
            player->GetSession()->SendNotification("我还没有目标!"); 
            return false;
        }
        Creature* creature = target->ToCreature();
        if (!creature) {
            ch.SendSysMessage("这是一个无效的目标");
            //target->Whisper("这是一个无效的目标", LANG_UNIVERSAL, player, true);
            return false;
        }
        auto ctemp = creature->GetCreatureTemplate();
        if (ctemp->type == 8 || ctemp->type == 10) {
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
        if (pTransmog->AddModelData(account_id, creature)) {
            std::ostringstream msg;
            msg << "成功了！" << ctemp->Name << " 幻象已成功收集到了你的哈哈镜中。";
            ch.SendSysMessage(msg.str());
            //target->Whisper(msg.str(), LANG_UNIVERSAL, player, true);
            player->DestroyItemCount(item->GetEntry(), 1, true);
        }
        else
        {
            ch.SendSysMessage("哦！No!失败了,你可能已经拥有了这个幻象！");
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
            player->GetSession()->SendNotification("我还没有目标!");
            return false;
        }
        Creature* creature = target->ToCreature();
        if (!creature) { 
            player->GetSession()->SendNotification("这是一个无效的目标!");
            return false;
        }
        auto ctemp = creature->GetCreatureTemplate();
        if (ctemp->type == 8 || ctemp->type == 10 || !(ctemp->rank == 3 || creature->isWorldBoss())) {
            player->GetSession()->SendNotification("这是一个无效的目标!");
            return false;
        }
        //ctemp->type_flags|
        if ((ctemp->rank == 3 || creature->isWorldBoss()) && creature->HealthAbovePct(40)) {
            ch.SendSysMessage("这个目标他太强大了，你需要先削弱他！"); 
            return false;
        }

        uint16 account_id = player->GetSession()->GetAccountId();
        if (pTransmog->AddModelData(account_id, creature)) {
            std::ostringstream msg;
            msg << "成功了！" << ctemp->Name << " 幻象已成功收集到了你的哈哈镜中。";
            ch.SendSysMessage(msg.str()); 
            player->DestroyItemCount(item->GetEntry(), 1, true);
        }
        else
        {
            ch.SendSysMessage("哦！No!失败了,你可能已经拥有了这个幻象！"); 
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
            player->GetSession()->SendNotification("我还没有目标!");
            return false;
        }
        Creature* creature = target->ToCreature();
        if (!creature) {
            player->GetSession()->SendNotification("这是一个无效的目标!");
            return false;
        }
        auto ctemp = creature->GetCreatureTemplate();

        if (ctemp->type == 8 || ctemp->type == 10 || !(ctemp->rank == 2 || ctemp->rank == 4)) {
            player->GetSession()->SendNotification("这是一个无效的目标!");
            return false;
        } 
        uint16 account_id = player->GetSession()->GetAccountId();
        if (pTransmog->AddModelData(account_id, creature)) {
            std::ostringstream msg;
            msg << "成功了！" << ctemp->Name << " 幻象已成功收集到了你的哈哈镜中。";
            ch.SendSysMessage(msg.str());
            player->DestroyItemCount(item->GetEntry(), 1, true);
        }
        else
        {
            ch.SendSysMessage("哦！No!失败了,你可能已经拥有了这个幻象！");
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
            player->GetSession()->SendNotification("我还没有目标!");
            return false;
        }
        Creature* creature = target->ToCreature();
        if (!creature) {
            player->GetSession()->SendNotification("这是一个无效的目标!");
            return false;
        }
        auto ctemp = creature->GetCreatureTemplate();

        if (ctemp->type == 8 || ctemp->type == 10 || ctemp->rank != 1) {
            player->GetSession()->SendNotification("这是一个无效的目标!");
            return false;
        }
        uint16 account_id = player->GetSession()->GetAccountId();
        if (pTransmog->AddModelData(account_id, creature)) {
            std::ostringstream msg;
            msg << "成功了！" << ctemp->Name << " 幻象已成功收集到了你的哈哈镜中。";
            ch.SendSysMessage(msg.str());
            player->DestroyItemCount(item->GetEntry(), 1, true);
        }
        else
        {
            ch.SendSysMessage("哦！No!失败了,你可能已经拥有了这个幻象！");
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
            player->GetSession()->SendNotification("我还没有目标!");
            return false;
        }
        Creature* creature = target->ToCreature();
        if (!creature) {
            player->GetSession()->SendNotification("这是一个无效的目标!");
            return false;
        }
        auto ctemp = creature->GetCreatureTemplate();

        if (ctemp->type == 8 || ctemp->type == 10 || ctemp->rank != 0) {
            player->GetSession()->SendNotification("这是一个无效的目标!");
            return false;
        }
        uint16 account_id = player->GetSession()->GetAccountId();
        if (pTransmog->AddModelData(account_id, creature)) {
            std::ostringstream msg;
            msg << "成功了！" << ctemp->Name << " 幻象已成功收集到了你的哈哈镜中。";
            ch.SendSysMessage(msg.str());
            player->DestroyItemCount(item->GetEntry(), 1, true);
        }
        else
        {
            ch.SendSysMessage("哦！No!失败了,你可能已经拥有了这个幻象！");
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
        void Register() override
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_player_transmog_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_TRANSFORM, AURA_EFFECT_HANDLE_REAL);
        }
    };
    AuraScript* GetAuraScript() const override
    {
        return new spell_player_transmog_AuraScript();
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
    new spell_player_transmog();
}
