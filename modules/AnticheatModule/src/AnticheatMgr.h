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

#ifndef SC_ACMGR_H
#define SC_ACMGR_H

//#include <ace/Singleton.h>
#include "Common.h"
#include "SharedDefines.h"
#include "ScriptMgr.h"
#include "AnticheatData.h"
#include "Chat.h"
#include "Database/Transaction.h"
#include "Utilities/AsyncCallbackProcessor.h"

class Player;

enum DetectionTypes
{
    SPEED_HACK_DETECTION            = 1,
    FLY_HACK_DETECTION              = 2,
    WALK_WATER_HACK_DETECTION       = 4,
    JUMP_HACK_DETECTION             = 8,
    TELEPORT_PLANE_HACK_DETECTION   = 16,
    CLIMB_HACK_DETECTION            = 32
};

// GUIDLow is the key.
typedef std::map<uint32, AnticheatData> AnticheatPlayersDataMap;

class AnticheatMgr
{
//    friend class ACE_Singleton<AnticheatMgr, ACE_Null_Mutex>;
    AnticheatMgr();
    ~AnticheatMgr();

    public:
    static AnticheatMgr* instance()
        {
           static AnticheatMgr* instance = new AnticheatMgr();
           return instance;
        }

        void StartHackDetection(Player* player, MovementInfo movementInfo, uint32 opcode);
        void QueuePlayerDataSave(uint32 lowGuid);
        void QueueDailyReportSave(uint32 lowGuid);
        void FlushSaveQueue();
        void ProcessSaveCallbacks();
        void ResetDailyReportStates();
        struct SaveEntry
        {
            AnticheatData data;
            bool saveDailyReport = false;
            bool logoutSnapshot = false;
            bool deleteReports = false;
            uint32 sessionGeneration = 0;
            uint64 operationGeneration = 0;
        };

        using SaveSnapshot = std::map<uint32, SaveEntry>;

        void OnSaveComplete(std::shared_ptr<SaveSnapshot> snapshot, bool success, bool dailyReportsReset, bool deleteAllReports, uint64 operationGeneration);

        void HandlePlayerLogin(Player* player);
        void HandlePlayerLogout(Player* player);

        uint32 GetTotalReports(uint32 lowGUID);
        float GetAverage(uint32 lowGUID);
        uint32 GetTypeReports(uint32 lowGUID, uint8 type);

        void AnticheatGlobalCommand(ChatHandler* handler);
        void AnticheatDeleteCommand(uint32 guid);

    private:
        void SpeedHackDetection(Player* player, MovementInfo movementInfo);
        void FlyHackDetection(Player* player, MovementInfo movementInfo);
        void WalkOnWaterHackDetection(Player* player, MovementInfo movementInfo);
        void JumpHackDetection(Player* player, MovementInfo movementInfo,uint32 opcode);
        void TeleportPlaneHackDetection(Player* player, MovementInfo);
        void ClimbHackDetection(Player* player,MovementInfo movementInfo,uint32 opcode);

        void BuildReport(Player* player,uint8 reportType);

        bool MustCheckTempReports(uint8 type);

        AnticheatPlayersDataMap m_Players;
        std::set<uint32> m_SaveQueue;
        std::set<uint32> m_DailySaveQueue;
        std::set<uint32> m_DeleteReportQueue;
        bool m_DeleteAllReportsQueued = false;
        bool m_DailyReportsResetQueued = false;
        std::map<uint32, SaveEntry> m_PendingSaveData;
        std::map<uint32, uint32> m_SessionGenerations;
        std::map<uint32, uint32> m_LogoutGenerations;
        AsyncCallbackProcessor<TransactionCallback> m_SaveCallbacks;
        bool m_SaveInFlight = false;
        uint64 m_ReportOperationGeneration = 0;
};

#define sAnticheatMgr AnticheatMgr::instance()

#endif
