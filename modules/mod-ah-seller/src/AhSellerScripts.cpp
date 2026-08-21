/*
 * mod-ah-seller - 拍卖行卖家池模块
 * 脚本钩子：世界启动加载配置、AH 更新触发补货、成交/过期维护浮动与存量、拦截 bot 邮件。
 */

#include "AhSeller.h"

#include "AuctionHouseMgr.h"
#include "Config.h"
#include "Log.h"
#include "Mail.h"
#include "Player.h"
#include "ScriptMgr.h"

// =============================================================================
// 世界脚本：读配置 + 加载池子
// =============================================================================

class AhSellerWorldScript : public WorldScript
{
public:
    AhSellerWorldScript() : WorldScript("AhSellerWorldScript", {
        WORLDHOOK_ON_BEFORE_CONFIG_LOAD,
        WORLDHOOK_ON_STARTUP
    }) { }

    void OnBeforeConfigLoad(bool reload) override
    {
        if (reload)
            return;

        gAhSellerEnabled = sConfigMgr->GetOption<bool>("AhSeller.Enable", false);
        gAhSellerAccount = sConfigMgr->GetOption<uint32>("AhSeller.Account", 0);
        gAhSellerGuid    = sConfigMgr->GetOption<uint32>("AhSeller.GUID", 0);
        gAhSellerDebug   = sConfigMgr->GetOption<bool>("AhSeller.Debug", false);
    }

    void OnStartup() override
    {
        if (!gAhSellerEnabled)
            return;

        if (gAhSellerAccount == 0 || gAhSellerGuid == 0)
        {
            LOG_ERROR("server.loading", "AhSeller: Account/GUID 未配置，模块禁用");
            gAhSellerEnabled = false;
            return;
        }

        AhSellerLoadPools();
    }
};

// =============================================================================
// 拍卖行脚本：补货调度、存量、浮动
// =============================================================================

class AhSellerAuctionHouseScript : public AuctionHouseScript
{
public:
    AhSellerAuctionHouseScript() : AuctionHouseScript("AhSellerAuctionHouseScript", {
        AUCTIONHOUSEHOOK_ON_AUCTION_ADD,
        AUCTIONHOUSEHOOK_ON_AUCTION_REMOVE,
        AUCTIONHOUSEHOOK_ON_AUCTION_SUCCESSFUL,
        AUCTIONHOUSEHOOK_ON_AUCTION_EXPIRE,
        AUCTIONHOUSEHOOK_ON_BEFORE_AUCTIONHOUSEMGR_UPDATE
    }) { }

    void OnBeforeAuctionHouseMgrUpdate() override
    {
        if (!gAhSellerEnabled)
            return;
        AhSellerRestockTick();
    }

    void OnAuctionAdd(AuctionHouseObject*, AuctionEntry* auction) override
    {
        if (!gAhSellerEnabled || !auction)
            return;
        if (auction->owner.GetCounter() != gAhSellerGuid)
            return;
        AhSellerOnAuctionAdd(auction->item_template);
    }

    void OnAuctionRemove(AuctionHouseObject*, AuctionEntry* auction) override
    {
        if (!gAhSellerEnabled || !auction)
            return;
        if (auction->owner.GetCounter() != gAhSellerGuid)
            return;
        AhSellerOnAuctionRemove(auction->item_template);
    }

    void OnAuctionSuccessful(AuctionHouseObject*, AuctionEntry* auction) override
    {
        if (!gAhSellerEnabled || !auction)
            return;
        if (auction->owner.GetCounter() != gAhSellerGuid)
            return;
        AhSellerUpdateHeat(auction->item_template, +1);   // 卖出 → 上浮
    }

    void OnAuctionExpire(AuctionHouseObject*, AuctionEntry* auction) override
    {
        if (!gAhSellerEnabled || !auction)
            return;
        if (auction->owner.GetCounter() != gAhSellerGuid)
            return;
        AhSellerUpdateHeat(auction->item_template, -1);   // 过期未卖出 → 下浮
    }
};

// =============================================================================
// 邮件脚本：拦截发往 bot 的成交邮件，避免堆积
// =============================================================================

class AhSellerMailScript : public MailScript
{
public:
    AhSellerMailScript() : MailScript("AhSellerMailScript", {
        MAILHOOK_ON_BEFORE_MAIL_DRAFT_SEND_MAIL_TO
    }) { }

    void OnBeforeMailDraftSendMailTo(MailDraft*, MailReceiver const& receiver, MailSender const& sender,
                                     MailCheckMask&, uint32&, uint32&, bool& deleteMailItemsFromDB, bool& sendMail) override
    {
        if (receiver.GetPlayerGUIDLow() == gAhSellerGuid)
        {
            if (sender.GetMailMessageType() == MAIL_AUCTION)
                deleteMailItemsFromDB = true;
            sendMail = false;
        }
    }
};

// =============================================================================
// 模块入口
// =============================================================================

void Addmod_ah_sellerScripts()
{
    new AhSellerWorldScript();
    new AhSellerAuctionHouseScript();
    new AhSellerMailScript();
}
