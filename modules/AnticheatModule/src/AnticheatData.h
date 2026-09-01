#ifndef SC_ACDATA_H
#define SC_ACDATA_H

#include "Common.h"
#include "Object.h"

class Field;

constexpr uint8 MAX_REPORT_TYPES = 6;

enum ReportTypes : uint8
{
    SPEED_HACK_REPORT = 0,
    FLY_HACK_REPORT,
    WALK_WATER_HACK_REPORT,
    JUMP_HACK_REPORT,
    TELEPORT_PLANE_HACK_REPORT,
    CLIMB_HACK_REPORT
};

class AnticheatData
{
public:
    AnticheatData();

    uint32 GetLastOpcode() const;
    void SetLastOpcode(uint32 opcode);

    MovementInfo const& GetLastMovementInfo() const;
    void SetLastMovementInfo(MovementInfo const& moveInfo);
    void SetPosition(float x, float y, float z, float o);

    uint32 GetLastMapId() const;
    void SetLastMapId(uint32 mapId);

    bool HasLastMovement() const;
    void SetHasLastMovement(bool value);

    uint32 GetTotalReports() const;
    void SetTotalReports(uint32 totalReports);

    uint32 GetTypeReports(uint8 type) const;
    void SetTypeReports(uint8 type, uint32 amount);

    float GetAverage() const;
    void SetAverage(float average);

    uint32 GetCreationTime() const;
    void SetCreationTime(uint32 creationTime);

    void SetTempReports(uint32 amount, uint8 type);
    uint32 GetTempReports(uint8 type) const;

    void SetTempReportsTimer(uint32 time, uint8 type);
    uint32 GetTempReportsTimer(uint8 type) const;

    void SetDailyReportState(bool value);
    bool GetDailyReportState() const;

    void LoadPersistentReports(Field* fields);
    void ResetPersistentReports();
    void ResetTemporaryReports();


private:
    uint32 lastOpcode = 0;
    MovementInfo lastMovementInfo;
    uint32 lastMapId = MAPID_INVALID;
    bool hasLastMovement = false;
    uint32 totalReports = 0;
    uint32 typeReports[MAX_REPORT_TYPES] = {};
    float average = 0.0f;
    uint32 creationTime = 0;
    uint32 tempReports[MAX_REPORT_TYPES] = {};
    uint32 tempReportsTimer[MAX_REPORT_TYPES] = {};
    bool hasDailyReport = false;
};

#endif
