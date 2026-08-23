#include "ScriptMgr.h"
#include "Configuration/Config.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "Player.h"
#include "Object.h"
#include "DataMap.h"

using namespace Acore::ChatCommands;

/*
原作者：Talamortis（Azerothcore 项目）
感谢 Rochet 的协助
*/

struct IndividualXpModule
{
    bool Enabled, AnnounceModule, AnnounceRatesOnLogin;
    float MaxRate, DefaultRate;
};

IndividualXpModule individualXp;

class IndividualXPConf : public WorldScript
{
public:
    IndividualXPConf() : WorldScript("IndividualXPConf") {}

    void OnBeforeConfigLoad(bool /*reload*/) override
    {
        individualXp.Enabled = sConfigMgr->GetOption<bool>("IndividualXp.Enabled", true);
        individualXp.AnnounceModule = sConfigMgr->GetOption<bool>("IndividualXp.Announce", true);
        individualXp.AnnounceRatesOnLogin = sConfigMgr->GetOption<bool>("IndividualXp.AnnounceRatesOnLogin", true);
        individualXp.MaxRate = sConfigMgr->GetOption<float>("IndividualXp.MaxXPRate", 10.0f);
        individualXp.DefaultRate = sConfigMgr->GetOption<float>("IndividualXp.DefaultXPRate", 1.0f);
    }
};

class PlayerXpRate : public DataMap::Base
{
public:
    PlayerXpRate() {}
    PlayerXpRate(float XPRate) : XPRate(XPRate) {}
    float XPRate = 1.0f;
};

class IndividualXP : public PlayerScript
{
public:
    IndividualXP() : PlayerScript("IndividualXP") {}

    void OnPlayerLogin(Player* player) override
    {
        QueryResult result = CharacterDatabase.Query("SELECT `XPRate` FROM `individualxp` WHERE `CharacterGUID`='{}'", player->GetGUID().GetCounter());

        if (!result)
        {
            player->CustomData.GetDefault<PlayerXpRate>("IndividualXP")->XPRate = individualXp.DefaultRate;
        }
        else
        {
            Field* fields = result->Fetch();
            player->CustomData.Set("IndividualXP", new PlayerXpRate(fields[0].Get<float>()));
        }

        if (individualXp.Enabled)
        {
            // 公告模块信息
            if (individualXp.AnnounceModule)
            {
                ChatHandler(player->GetSession()).PSendSysMessage("本服务器正在运行 |cff4CFF00IndividualXpRate |r模块。");
            }

            // 公告经验倍率
            if (individualXp.AnnounceRatesOnLogin)
            {
                if (player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN))
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("[XP] 你的经验获取当前已关闭，使用 .xp enable 可重新开启。");
                }
                else
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("[XP] 你当前的经验倍率为 {}。", player->CustomData.GetDefault<PlayerXpRate>("IndividualXP")->XPRate);
                    ChatHandler(player->GetSession()).PSendSysMessage("[XP] 最大倍率限制为 {}。", individualXp.MaxRate);
                }
            }
        }
    }

    void OnPlayerLogout(Player* player) override
    {
        if (PlayerXpRate* data = player->CustomData.Get<PlayerXpRate>("IndividualXP"))
        {
            CharacterDatabase.DirectExecute("REPLACE INTO `individualxp` (`CharacterGUID`, `XPRate`) VALUES ('{}', '{}');", player->GetGUID().GetCounter(), data->XPRate);
        }
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/) override
    {
        if (individualXp.Enabled)
        {
            if (PlayerXpRate* data = player->CustomData.Get<PlayerXpRate>("IndividualXP"))
            {
                amount = static_cast<uint32>(std::round(static_cast<float>(amount) * data->XPRate));
            }
        }
    }
};

class IndividualXPCommand : public CommandScript
{
public:
    IndividualXPCommand() : CommandScript("IndividualXPCommand") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable IndividualXPCommandTable =
        {
            { "enable",  HandleEnableCommand, SEC_PLAYER, Console::No },
            { "disable",  HandleDisableCommand, SEC_PLAYER, Console::No },
            { "view",  HandleViewCommand, SEC_PLAYER, Console::No },
            { "set",  HandleSetCommand, SEC_PLAYER, Console::No },
            { "default",  HandleDefaultCommand, SEC_PLAYER, Console::No }
        };

        static ChatCommandTable IndividualXPBaseTable =
        {
            { "xp",  IndividualXPCommandTable }
        };

        return IndividualXPBaseTable;
    }

    static bool HandleViewCommand(ChatHandler* handler)
    {
        if (!individualXp.Enabled)
        {
            handler->PSendSysMessage("[XP] Individual XP 模块已被停用。");
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();

        if (!player)
            return false;

        if (player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN))
        {
            handler->PSendSysMessage("[XP] 你的经验获取当前已关闭，使用 .xp enable 可重新开启。");
            handler->SetSentErrorMessage(true);
            return false;
        }
        else
        {
            ChatHandler(handler->GetSession()).PSendSysMessage("[XP] 你当前的经验倍率为 {}。", player->CustomData.GetDefault<PlayerXpRate>("IndividualXP")->XPRate);
        }
        return true;
    }

    static bool HandleSetCommand(ChatHandler* handler, float rate)
    {
        if (!individualXp.Enabled)
        {
            handler->PSendSysMessage("[XP] Individual XP 模块已被停用。");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!rate)
            return false;

        Player* player = handler->GetSession()->GetPlayer();

        if (!player)
            return false;

        if (player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN))
        {
            handler->PSendSysMessage("[XP] 你的经验获取当前已关闭，使用 .xp enable 可重新开启。");
            handler->SetSentErrorMessage(true);
            return false;
        }
        else
        {
            if (rate > individualXp.MaxRate)
            {
                handler->PSendSysMessage("[XP] 最大倍率限制为 {}。", individualXp.MaxRate);
                handler->SetSentErrorMessage(true);
                return false;
            }

            if (rate < 0.1f)
            {
                handler->PSendSysMessage("[XP] 最小倍率限制为 1。");
                handler->SetSentErrorMessage(true);
                return false;
            }

            player->CustomData.GetDefault<PlayerXpRate>("IndividualXP")->XPRate = rate;
            ChatHandler(handler->GetSession()).PSendSysMessage("[XP] 你已将经验倍率更新为 {}。", rate);
            return true;
        }
    }

    static bool HandleDisableCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        if (!player)
            return false;

        if (!player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN))
        {
            // 关闭经验获取，但不修改当前倍率数值
            player->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
            ChatHandler(handler->GetSession()).PSendSysMessage("[XP] 你已关闭经验获取。");
            return true;
        }
        else
        {
            ChatHandler(handler->GetSession()).PSendSysMessage("[XP] 你的经验获取当前已关闭，使用 .xp enable 可重新开启。");
            return false;
        }
    }

    static bool HandleEnableCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();

        if (!player)
            return false;

        if (player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN))
        {
            player->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
            ChatHandler(handler->GetSession()).PSendSysMessage("[XP] 你已开启经验获取。");
        }
        else
        {
            ChatHandler(handler->GetSession()).PSendSysMessage("[XP] 你的经验获取当前已关闭，使用 .xp enable 可重新开启。");
        }

        return true;
    }

    static bool HandleDefaultCommand(ChatHandler* handler)
    {
        if (!individualXp.Enabled)
        {
            handler->PSendSysMessage("[XP] Individual XP 模块已被停用。");
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();

        if (!player)
            return false;

        if (player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN))
        {
            handler->PSendSysMessage("[XP] 你的经验获取当前已关闭，使用 .xp enable 可重新开启。");
            handler->SetSentErrorMessage(true);
            return false;
        }
        else
        {
            player->CustomData.GetDefault<PlayerXpRate>("IndividualXP")->XPRate = individualXp.DefaultRate;
            ChatHandler(handler->GetSession()).PSendSysMessage("[XP] 你已将经验倍率恢复为默认值 {}。", individualXp.DefaultRate);
            return true;
        }
    }
};

void AddIndividualXPScripts()
{
    new IndividualXPConf();
    new IndividualXP();
    new IndividualXPCommand();
}
