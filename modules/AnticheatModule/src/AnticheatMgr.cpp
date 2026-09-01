/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AnticheatMgr.h"
#include "MapMgr.h"
#include "Player.h"
#include "WorldSessionMgr.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "Configuration/Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include <cmath>

#define CLIMB_SLOPE 1.9f

AnticheatMgr::AnticheatMgr()
{
}

AnticheatMgr::~AnticheatMgr()
{
	m_Players.clear();
}

void AnticheatMgr::JumpHackDetection(Player* player, MovementInfo /* movementInfo */, uint32 opcode)
{
	if (!sConfigMgr->GetOption<bool>("Anticheat.DetectJumpHack", true))
		return;

	uint32 key = player->GetGUID().GetCounter();
	auto itr = m_Players.find(key);
	if (itr == m_Players.end())
		return;

	AnticheatData const& data = itr->second;
	if (data.GetLastOpcode() == MSG_MOVE_JUMP && opcode == MSG_MOVE_JUMP)
	{
		BuildReport(player, JUMP_HACK_REPORT);
		LOG_INFO("modules.anticheat", "AnticheatMgr:: Jump-Hack detected player {} ({})", player->GetName(), player->GetGUID().GetCounter());
	}
}

void AnticheatMgr::WalkOnWaterHackDetection(Player* player, MovementInfo  movementInfo)
{
	if (!sConfigMgr->GetOption<bool>("Anticheat.DetectWaterWalkHack", true))
		return;

	uint32 key = player->GetGUID().GetCounter();
	auto itr = m_Players.find(key);
	if (itr == m_Players.end())
		return;

	AnticheatData const& data = itr->second;
	/* Thanks to @LilleCarl */
	if (!data.GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_WATERWALKING) && !movementInfo.HasMovementFlag(MOVEMENTFLAG_WATERWALKING))
		return;

	// if we are a ghost we can walk on water
	if (!player->IsAlive())
		return;

	if (player->HasAuraType(SPELL_AURA_FEATHER_FALL) ||
		player->HasAuraType(SPELL_AURA_SAFE_FALL) ||
		player->HasAuraType(SPELL_AURA_WATER_WALK))
		return;
	if (sConfigMgr->GetOption<bool>("Anticheat.KickPlayerWaterWalkHack", false))
	{
		/* cheap hack for now, look at "applyfortargets" later*/
		/*player->AddAura(SPELL_AURA_WATER_WALK, player);
		player->RemoveAura(SPELL_AURA_WATER_WALK);*/
		//cba to double check this, just adding a kick option
		player->GetSession()->KickPlayer(true);
		LOG_INFO("modules.anticheat", "AnticheatMgr:: Walk on Water - Hack detected and counteracted by kicking player {} ({})", player->GetName(), player->GetGUID().GetCounter());
	}
	else {
		LOG_INFO("modules.anticheat", "AnticheatMgr:: Walk on Water - Hack detected player {} ({})", player->GetName(), player->GetGUID().GetCounter());
	}
	BuildReport(player, WALK_WATER_HACK_REPORT);

}

void AnticheatMgr::FlyHackDetection(Player* player, MovementInfo  movementInfo)
{
    if (!sConfigMgr->GetOption<bool>("Anticheat.DetectFlyHack", true))
        return;

    uint32 key = player->GetGUID().GetCounter();
    if (player->HasAuraType(SPELL_AURA_FLY) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) || player->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED))//overkill but wth
        return;
	/*Thanks to @LilleCarl for info to check extra flag*/
	bool stricterChecks = true;
	if (sConfigMgr->GetOption<bool>("Anticheat.StricterFlyHackCheck", false))
		stricterChecks = !(movementInfo.HasMovementFlag(MOVEMENTFLAG_ASCENDING) && !player->IsInWater());
	if (!movementInfo.HasMovementFlag(MOVEMENTFLAG_CAN_FLY) && !movementInfo.HasMovementFlag(MOVEMENTFLAG_FLYING) && stricterChecks)
		return;
	if (sConfigMgr->GetOption<bool>("Anticheat.KickPlayerFlyHack", false))
	{
		LOG_INFO("modules.anticheat", "AnticheatMgr:: Fly-Hack detected and counteracted by kicking player {} ({})", player->GetName(), player->GetGUID().GetCounter());
		player->GetSession()->KickPlayer(true);
	}else
		LOG_INFO("modules.anticheat", "AnticheatMgr:: Fly-Hack detected player {} ({})", player->GetName(), player->GetGUID().GetCounter());
    BuildReport(player,FLY_HACK_REPORT);
}

void AnticheatMgr::TeleportPlaneHackDetection(Player* player, MovementInfo movementInfo)
{
	if (!sConfigMgr->GetOption<bool>("Anticheat.DetectTelePlaneHack", true))
		return;

	uint32 key = player->GetGUID().GetCounter();
	auto itr = m_Players.find(key);
	if (itr == m_Players.end())
		return;

	AnticheatData const& data = itr->second;
	if (data.GetLastMovementInfo().pos.GetPositionZ() != 0 ||
		movementInfo.pos.GetPositionZ() != 0)
		return;

	if (movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING))
		return;

	// DEAD_FALLING was deprecated
	//if (player->getDeathState() == DEAD_FALLING)
	//    return;
	float x, y, z;
	player->GetPosition(x, y, z);
	float ground_Z = player->GetMap()->GetHeight(x, y, z);
	if (ground_Z == INVALID_HEIGHT)
		return;

	float z_diff = fabs(ground_Z - z);

	// we are not really walking there
	if (z_diff > 1.0f)
	{
		LOG_INFO("modules.anticheat", "AnticheatMgr:: Teleport To Plane - Hack detected player {} ({})", player->GetName(), player->GetGUID().GetCounter());
		BuildReport(player, TELEPORT_PLANE_HACK_REPORT);
	}
}

void AnticheatMgr::StartHackDetection(Player* player, MovementInfo movementInfo, uint32 opcode)
{
	if (!sConfigMgr->GetOption<bool>("Anticheat.Enabled", 0) || player->IsGameMaster())
		return;

	uint32 key = player->GetGUID().GetCounter();
	auto itr = m_Players.find(key);
	if (itr == m_Players.end())
		return;

	AnticheatData& data = itr->second;
	uint32 mapId = player->GetMapId();

	if (!data.HasLastMovement())
	{
		data.SetLastMovementInfo(movementInfo);
		data.SetLastOpcode(opcode);
		data.SetLastMapId(mapId);
		data.SetHasLastMovement(true);
		return;
	}

	bool resetBaseline = mapId != data.GetLastMapId() || player->IsInFlight() || player->GetTransport() || player->GetVehicle();
	if (resetBaseline)
	{
		data.SetLastMovementInfo(movementInfo);
		data.SetLastOpcode(opcode);
		data.SetLastMapId(mapId);
		return;
	}

	SpeedHackDetection(player, movementInfo);
	FlyHackDetection(player, movementInfo);
	WalkOnWaterHackDetection(player, movementInfo);
	JumpHackDetection(player, movementInfo, opcode);
	TeleportPlaneHackDetection(player, movementInfo);
	ClimbHackDetection(player, movementInfo, opcode);

	data.SetLastMovementInfo(movementInfo);
	data.SetLastOpcode(opcode);
	data.SetLastMapId(mapId);
	data.SetHasLastMovement(true);
}

// basic detection
void AnticheatMgr::ClimbHackDetection(Player *player, MovementInfo movementInfo, uint32 opcode)
{
	if (!sConfigMgr->GetOption<bool>("Anticheat.DetectClimbHack", false))
		return;

	uint32 key = player->GetGUID().GetCounter();
	auto itr = m_Players.find(key);
	if (itr == m_Players.end())
		return;

	AnticheatData const& data = itr->second;
	if (opcode != MSG_MOVE_HEARTBEAT ||
		data.GetLastOpcode() != MSG_MOVE_HEARTBEAT)
		return;

	// in this case we don't care if they are "legal" flags, they are handled in another parts of the Anticheat Manager.
	if (player->IsInWater() ||
		player->IsFlying() ||
		player->IsFalling())
		return;

	Position const& playerPos = data.GetLastMovementInfo().pos;
	float deltaZ = fabs(playerPos.GetPositionZ() - movementInfo.pos.GetPositionZ());
	float deltaXY = movementInfo.pos.GetExactDist2d(&playerPos);
	if (deltaXY < 0.01f)
		return;

	float slope = deltaZ / deltaXY;
	if (slope > CLIMB_SLOPE)
	{
		LOG_INFO("modules.anticheat", "AnticheatMgr:: Climb-Hack detected player {} ({})", player->GetName(), player->GetGUID().GetCounter());
		BuildReport(player, CLIMB_HACK_REPORT);
	}
}

void AnticheatMgr::SpeedHackDetection(Player* player, MovementInfo movementInfo)
{
	if (!sConfigMgr->GetOption<bool>("Anticheat.DetectSpeedHack", true))
		return;

	uint32 key = player->GetGUID().GetCounter();
	auto itr = m_Players.find(key);
	if (itr == m_Players.end())
		return;

	AnticheatData const& data = itr->second;

	// We also must check the map because the movementFlag can be modified by the client.
	// If we just check the flag, they could always add that flag and always skip the speed hacking detection.
	// 369 == DEEPRUN TRAM
	if (data.GetLastMovementInfo().HasMovementFlag(MOVEMENTFLAG_ONTRANSPORT) && player->GetMapId() == 369)
		return;
	float distance2D = movementInfo.pos.GetExactDist2d(&data.GetLastMovementInfo().pos);
	uint8 moveType = 0;

	// we need to know HOW is the player moving
	// TO-DO: Should we check the incoming movement flags?
	if (player->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING))
		moveType = MOVE_SWIM;
	else if (player->IsFlying())
		moveType = MOVE_FLIGHT;
	else if (player->HasUnitMovementFlag(MOVEMENTFLAG_WALKING))
		moveType = MOVE_WALK;
	else
		moveType = MOVE_RUN;

	// how many yards the player can do in one sec.
	float speedRate = player->GetSpeed(UnitMoveType(moveType));
	if (movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING) && movementInfo.jump.xyspeed > speedRate)
		speedRate = movementInfo.jump.xyspeed;

	uint32 timeDiff = getMSTimeDiff(data.GetLastMovementInfo().time, movementInfo.time);
	if (timeDiff == 0 || timeDiff > 5000)
		return;

	float measuredSpeed = distance2D * 1000.0f / float(timeDiff);
	float tolerance = sConfigMgr->GetOption<float>("Anticheat.SpeedTolerance", 1.10f);
	if (measuredSpeed > speedRate * tolerance)
	{
		BuildReport(player, SPEED_HACK_REPORT);
		LOG_INFO("modules.anticheat", "AnticheatMgr:: Speed-Hack detected player {} ({})", player->GetName(), player->GetGUID().GetCounter());
	}
}


void AnticheatMgr::HandlePlayerLogin(Player* player)
{
	uint32 key = player->GetGUID().GetCounter();
	AnticheatData& data = m_Players[key];
	++m_SessionGenerations[key];
	m_PendingSaveData.erase(key);
	data.SetPosition(player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation());
	data.SetLastMapId(player->GetMapId());
	data.SetHasLastMovement(false);
	data.ResetTemporaryReports();

	QueryResult status = CharacterDatabase.Query("SELECT guid,creation_time,average,total_reports,speed_reports,fly_reports,jump_reports,waterwalk_reports,teleportplane_reports,climb_reports FROM players_reports_status WHERE guid={};", key);
	if (status)
		data.LoadPersistentReports(status->Fetch());
	else
		data.ResetPersistentReports();

	QueryResult daily = CharacterDatabase.Query("SELECT guid FROM daily_players_reports WHERE guid={};", key);
	data.SetDailyReportState(bool(daily));
	m_LogoutGenerations.erase(key);
	QueuePlayerDataSave(key);
}

void AnticheatMgr::HandlePlayerLogout(Player* player)
{
	uint32 key = player->GetGUID().GetCounter();
	auto itr = m_Players.find(key);
	if (itr == m_Players.end())
		return;

	m_LogoutGenerations[key] = m_SessionGenerations[key];
	m_PendingSaveData[key] = { itr->second, false, true, false, m_SessionGenerations[key], m_ReportOperationGeneration };
	m_SaveQueue.erase(key);
	m_DailySaveQueue.erase(key);
}

void AnticheatMgr::QueuePlayerDataSave(uint32 lowGuid)
{
    if (m_Players.find(lowGuid) != m_Players.end())
        m_SaveQueue.insert(lowGuid);
}

void AnticheatMgr::QueueDailyReportSave(uint32 lowGuid)
{
    if (m_Players.find(lowGuid) != m_Players.end())
        m_DailySaveQueue.insert(lowGuid);
}

void AnticheatMgr::ProcessSaveCallbacks()
{
    m_SaveCallbacks.ProcessReadyCallbacks();
}

void AnticheatMgr::FlushSaveQueue()
{
    ProcessSaveCallbacks();
    if (m_SaveInFlight)
        return;
    if (m_SaveQueue.empty() && m_DailySaveQueue.empty() && m_PendingSaveData.empty() && m_DeleteReportQueue.empty() && !m_DeleteAllReportsQueued && !m_DailyReportsResetQueued)
        return;

    auto snapshot = std::make_shared<SaveSnapshot>();
    for (uint32 lowGuid : m_SaveQueue)
    {
        auto itr = m_Players.find(lowGuid);
        if (itr != m_Players.end())
        {
            SaveEntry& entry = (*snapshot)[lowGuid];
            entry.data = itr->second;
            entry.sessionGeneration = m_SessionGenerations[lowGuid];
            entry.operationGeneration = m_ReportOperationGeneration;
        }
    }
    for (uint32 lowGuid : m_DailySaveQueue)
    {
        auto itr = m_Players.find(lowGuid);
        if (itr != m_Players.end())
        {
            SaveEntry& entry = (*snapshot)[lowGuid];
            entry.data = itr->second;
            entry.saveDailyReport = true;
            entry.sessionGeneration = m_SessionGenerations[lowGuid];
            entry.operationGeneration = m_ReportOperationGeneration;
        }
    }
    for (auto const& entry : m_PendingSaveData)
    {
        if (snapshot->find(entry.first) == snapshot->end())
            (*snapshot)[entry.first] = entry.second;
    }
    for (auto& entry : *snapshot)
        entry.second.operationGeneration = m_ReportOperationGeneration;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    auto appendReport = [&trans](std::string_view tableName, uint32 lowGuid, AnticheatData const& data)
    {
        trans->Append("REPLACE INTO {} (guid,average,total_reports,speed_reports,fly_reports,jump_reports,waterwalk_reports,teleportplane_reports,climb_reports,creation_time) VALUES ({},{},{},{},{},{},{},{},{},{});", tableName, lowGuid, data.GetAverage(), data.GetTotalReports(), data.GetTypeReports(SPEED_HACK_REPORT), data.GetTypeReports(FLY_HACK_REPORT), data.GetTypeReports(JUMP_HACK_REPORT), data.GetTypeReports(WALK_WATER_HACK_REPORT), data.GetTypeReports(TELEPORT_PLANE_HACK_REPORT), data.GetTypeReports(CLIMB_HACK_REPORT), data.GetCreationTime());
    };

    if (m_DeleteAllReportsQueued)
    {
        trans->Append("DELETE FROM players_reports_status;");
        trans->Append("DELETE FROM daily_players_reports;");
    }
    else
    {
        for (uint32 lowGuid : m_DeleteReportQueue)
        {
            trans->Append("DELETE FROM players_reports_status WHERE guid = {}", lowGuid);
            trans->Append("DELETE FROM daily_players_reports WHERE guid = {}", lowGuid);
            (*snapshot)[lowGuid].deleteReports = true;
            (*snapshot)[lowGuid].operationGeneration = m_ReportOperationGeneration;
        }
    }
    if (m_DailyReportsResetQueued)
    {
        trans->Append("DELETE FROM daily_players_reports;");
        for (auto& entry : *snapshot)
            entry.second.saveDailyReport = false;
    }

    bool dailyReportsReset = m_DailyReportsResetQueued;
    bool deleteAllReports = m_DeleteAllReportsQueued;
    for (auto const& entry : *snapshot)
    {
        if (deleteAllReports || entry.second.deleteReports)
            continue;

        appendReport("players_reports_status", entry.first, entry.second.data);
        if (entry.second.saveDailyReport)
            appendReport("daily_players_reports", entry.first, entry.second.data);
    }
    uint64 operationGeneration = ++m_ReportOperationGeneration;
    m_SaveInFlight = true;
    m_SaveCallbacks.AddCallback(CharacterDatabase.AsyncCommitTransaction(trans)).AfterComplete([this, snapshot, dailyReportsReset, deleteAllReports, operationGeneration](bool success)
    {
        OnSaveComplete(snapshot, success, dailyReportsReset, deleteAllReports, operationGeneration);
    });
    m_SaveQueue.clear();
    m_DailySaveQueue.clear();
    m_DeleteReportQueue.clear();
    m_DeleteAllReportsQueued = false;
    m_DailyReportsResetQueued = false;
}

void AnticheatMgr::OnSaveComplete(std::shared_ptr<SaveSnapshot> snapshot, bool success, bool dailyReportsReset, bool deleteAllReports, uint64 operationGeneration)
{
    m_SaveInFlight = false;
    if (!success)
    {
        if (dailyReportsReset)
            m_DailyReportsResetQueued = true;
        if (deleteAllReports)
            ++m_ReportOperationGeneration;
        if (deleteAllReports)
            m_DeleteAllReportsQueued = true;
        for (auto const& entry : *snapshot)
        {
            if (entry.second.deleteReports)
                m_DeleteReportQueue.insert(entry.first);
            else if (entry.second.sessionGeneration == m_SessionGenerations[entry.first])
                m_PendingSaveData[entry.first] = entry.second;
        }
        return;
    }

    if (dailyReportsReset)
    {
        for (auto& entry : m_Players)
        {
            entry.second.SetDailyReportState(false);
        }
    }

    for (auto const& entry : *snapshot)
    {
        if (!entry.second.logoutSnapshot)
            continue;

        auto pending = m_PendingSaveData.find(entry.first);
        if (pending != m_PendingSaveData.end() && pending->second.sessionGeneration == entry.second.sessionGeneration)
            m_PendingSaveData.erase(pending);
        auto logout = m_LogoutGenerations.find(entry.first);
        if (logout != m_LogoutGenerations.end() && logout->second == entry.second.sessionGeneration &&
            m_SessionGenerations[entry.first] == entry.second.sessionGeneration &&
            m_Players.find(entry.first) != m_Players.end())
        {
            m_Players.erase(entry.first);
            m_LogoutGenerations.erase(logout);
        }
    }
}

uint32 AnticheatMgr::GetTotalReports(uint32 lowGUID)
{
    auto itr = m_Players.find(lowGUID);
    return itr != m_Players.end() ? itr->second.GetTotalReports() : 0;
}

float AnticheatMgr::GetAverage(uint32 lowGUID)
{
    auto itr = m_Players.find(lowGUID);
    return itr != m_Players.end() ? itr->second.GetAverage() : 0.0f;
}

uint32 AnticheatMgr::GetTypeReports(uint32 lowGUID, uint8 type)
{
    auto itr = m_Players.find(lowGUID);
    return itr != m_Players.end() ? itr->second.GetTypeReports(type) : 0;
}

bool AnticheatMgr::MustCheckTempReports(uint8 type)
{
	if (type == JUMP_HACK_REPORT)
		return false;

	return true;
}

void AnticheatMgr::BuildReport(Player* player, uint8 reportType)
{
	uint32 key = player->GetGUID().GetCounter();
	auto itr = m_Players.find(key);
	if (itr == m_Players.end())
		return;

	AnticheatData& data = itr->second;
	if (MustCheckTempReports(reportType))
	{
		uint32 actualTime = getMSTime();

		if (!data.GetTempReportsTimer(reportType))
			data.SetTempReportsTimer(actualTime, reportType);

		if (getMSTimeDiff(data.GetTempReportsTimer(reportType), actualTime) < 3000)
		{
			data.SetTempReports(data.GetTempReports(reportType) + 1, reportType);

			if (data.GetTempReports(reportType) < 3)
				return;
		}
		else
		{
			data.SetTempReportsTimer(actualTime, reportType);
			data.SetTempReports(1, reportType);
			return;
		}
		data.SetTempReports(0, reportType);
		data.SetTempReportsTimer(0, reportType);
	}

	// generating creationTime for average calculation
	if (!data.GetTotalReports())
		data.SetCreationTime(getMSTime());

	// increasing total_reports
	data.SetTotalReports(data.GetTotalReports() + 1);
	// increasing specific cheat report
	data.SetTypeReports(reportType, data.GetTypeReports(reportType) + 1);

	// diff time for average calculation
	uint32 diffTime = getMSTimeDiff(data.GetCreationTime(), getMSTime()) / IN_MILLISECONDS;

	if (diffTime > 0)
	{
		// Average == Reports per second
		float average = float(data.GetTotalReports()) / float(diffTime);
		data.SetAverage(average);
	}

	if (data.GetTotalReports() >= uint32(sConfigMgr->GetOption<int>("Anticheat.MaxReportsForDailyReport", 70)))
	{
		if (!data.GetDailyReportState())
		{
			QueueDailyReportSave(key);
			data.SetDailyReportState(true);
		}
	}

	QueuePlayerDataSave(key);

	if (data.GetTotalReports() >= uint32(sConfigMgr->GetOption<int>("Anticheat.ReportsForIngameWarnings", 70)) &&
		data.GetTotalReports() == uint32(sConfigMgr->GetOption<int>("Anticheat.ReportsForIngameWarnings", 70)))
	{
		// display warning at the center of the screen, hacky way?
		std::string str = "";
		str = "|cFFFFFC00[Playername:|cFF00FFFF[|cFF60FF00" + std::string(player->GetName().c_str()) + "|cFF00FFFF] Possible cheater!";
		WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
		data << str;
		sWorldSessionMgr->SendGlobalGMMessage(&data);
	}
}

void AnticheatMgr::AnticheatGlobalCommand(ChatHandler* handler)
{
	// MySQL will sort all for us, anyway this is not the best way we must only save the anticheat data not whole player's data!.
	ObjectAccessor::SaveAllPlayers();

	QueryResult resultDB = CharacterDatabase.Query("SELECT guid,average,total_reports FROM players_reports_status WHERE total_reports != 0 ORDER BY average ASC LIMIT 3;");
	if (!resultDB)
	{
		handler->PSendSysMessage("No players found.");
		return;
	}
	else
	{
		handler->SendSysMessage("=============================");
		handler->PSendSysMessage("Players with the lowest averages:");
		do
		{
			Field *fieldsDB = resultDB->Fetch();

			uint32 guid = fieldsDB[0].Get<uint32>();
			float average = fieldsDB[1].Get<float>();
			uint32 total_reports = fieldsDB[2].Get<uint32>();

			if (Player* player = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(guid)))
				handler->PSendSysMessage("Player: %s Average: %f Total Reports: %u", player->GetName().c_str(), average, total_reports);

			} while (resultDB->NextRow());
	}

	resultDB = CharacterDatabase.Query("SELECT guid,average,total_reports FROM players_reports_status WHERE total_reports != 0 ORDER BY total_reports DESC LIMIT 3;");

	// this should never happen
	if (!resultDB)
	{
		handler->PSendSysMessage("No players found.");
		return;
	}
	else
	{
		handler->PSendSysMessage("=============================");
		handler->PSendSysMessage("Players with the more reports:");
		do
		{
			Field *fieldsDB = resultDB->Fetch();

			uint32 guid = fieldsDB[0].Get<uint32>();
			float average = fieldsDB[1].Get<float>();
			uint32 total_reports = fieldsDB[2].Get<uint32>();

			if (Player* player = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(guid)))
				handler->PSendSysMessage("Player: %s Total Reports: %u Average: %f", player->GetName().c_str(), total_reports, average);

		} while (resultDB->NextRow());
	}
}

void AnticheatMgr::AnticheatDeleteCommand(uint32 guid)
{
	if (!guid)
	{
		for (AnticheatPlayersDataMap::iterator it = m_Players.begin(); it != m_Players.end(); ++it)
		{
			(*it).second.SetTotalReports(0);
			(*it).second.SetAverage(0);
			(*it).second.SetCreationTime(0);
			for (uint8 i = 0; i < MAX_REPORT_TYPES; i++)
			{
				(*it).second.SetTempReports(0, i);
				(*it).second.SetTempReportsTimer(0, i);
				(*it).second.SetTypeReports(i, 0);
			}
		}
		m_Players.clear();
		m_PendingSaveData.clear();
		m_SaveQueue.clear();
		m_DailySaveQueue.clear();
		m_DeleteReportQueue.clear();
		m_DailySaveQueue.clear();
		m_LogoutGenerations.clear();
		++m_ReportOperationGeneration;
		m_DeleteAllReportsQueued = true;
		m_DailyReportsResetQueued = false;
		FlushSaveQueue();
	}
	else
	{
		auto itr = m_Players.find(guid);
		if (itr == m_Players.end())
			return;

		AnticheatData& data = itr->second;
		data.SetTotalReports(0);
		data.SetAverage(0);
		data.SetCreationTime(0);
		data.SetDailyReportState(false);
		for (uint8 i = 0; i < MAX_REPORT_TYPES; i++)
		{
			data.SetTempReports(0, i);
			data.SetTempReportsTimer(0, i);
			data.SetTypeReports(i, 0);
		}
		m_PendingSaveData.erase(guid);
		m_SaveQueue.erase(guid);
		m_DailySaveQueue.erase(guid);
		m_DeleteReportQueue.insert(guid);
		m_LogoutGenerations.erase(guid);
		++m_SessionGenerations[guid];
		++m_ReportOperationGeneration;
		FlushSaveQueue();
	}
}

void AnticheatMgr::ResetDailyReportStates()
{
    for (auto& entry : m_Players)
        QueuePlayerDataSave(entry.first);

    m_DailyReportsResetQueued = true;
    FlushSaveQueue();
}
