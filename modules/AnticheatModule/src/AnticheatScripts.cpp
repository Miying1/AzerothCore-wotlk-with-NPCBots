#include "Configuration/Config.h"
#include "AnticheatMgr.h"
#include "Object.h"
#include "AccountMgr.h"
#include "Log.h"
#include "GameTime.h"
#include "Utilities/Timer.h"

int64 resetTime = 0;
uint32 saveQueueTimer = 0;
class AnticheatPlayerScript : public PlayerScript
{
public:
	AnticheatPlayerScript()
		: PlayerScript("AnticheatPlayerScript", { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT })
	{
	}

	void OnPlayerLogout(Player* player) override
	{
		sAnticheatMgr->HandlePlayerLogout(player);
	}

	void OnPlayerLogin(Player* player) override
	{
		sAnticheatMgr->HandlePlayerLogin(player);
		if(sConfigMgr->GetOption<bool>("Anticheat.LoginMessage", true))
			ChatHandler(player->GetSession()).PSendSysMessage("This server is running an Anticheat Module.");
	}
};
class AnticheatWorldScript : public WorldScript
{
public:
	AnticheatWorldScript()
		: WorldScript("AnticheatWorldScript", { WORLDHOOK_ON_UPDATE, WORLDHOOK_ON_AFTER_CONFIG_LOAD })
	{
	}
	void OnUpdate(uint32 diff) override
	{
		if (!resetTime)
			UpdateReportResetTime();

		if (GameTime::GetGameTime().count() > resetTime)
		{
			LOG_INFO("modules.anticheat", "Anticheat: Resetting daily report states.");
			sAnticheatMgr->ResetDailyReportStates();
			UpdateReportResetTime();
			LOG_INFO("modules.anticheat", "Anticheat: Next daily report reset: {}", resetTime);
		}
		if (saveQueueTimer <= diff)
		{
			saveQueueTimer = 2000;
			sAnticheatMgr->FlushSaveQueue();
		}
		else
			saveQueueTimer -= diff;
	}
	void OnAfterConfigLoad(bool reload) override
	{
		LOG_INFO("modules.anticheat", "AnticheatModule Loaded.");
	}
	void UpdateReportResetTime()
	{
		resetTime = Acore::Time::GetNextTimeWithDayAndHour(-1, 6);
	}
};
class AnticheatMovementHandlerScript : public MovementHandlerScript
{
	public:
	AnticheatMovementHandlerScript()
		: MovementHandlerScript("AnticheatMovementHandlerScript", { MOVEMENTHOOK_ON_PLAYER_MOVE })
	{
	}
    void OnPlayerMove(Player* player, MovementInfo mi, uint32 opcode) override
    {
		if (!AccountMgr::IsGMAccount(player->GetSession()->GetSecurity()) || sConfigMgr->GetOption<bool>("Anticheat.EnabledOnGmAccounts", false))
			sAnticheatMgr->StartHackDetection(player, mi, opcode);
    }
};
void startAnticheatScripts()
{
	new AnticheatWorldScript();
	new AnticheatPlayerScript();
	new AnticheatMovementHandlerScript();
}