#include "AnticheatData.h"
#include "DatabaseEnv.h"

AnticheatData::AnticheatData() = default;

uint32 AnticheatData::GetLastOpcode() const
{
    return lastOpcode;
}

void AnticheatData::SetLastOpcode(uint32 opcode)
{
    lastOpcode = opcode;
}

MovementInfo const& AnticheatData::GetLastMovementInfo() const
{
    return lastMovementInfo;
}

void AnticheatData::SetLastMovementInfo(MovementInfo const& moveInfo)
{
    lastMovementInfo = moveInfo;
}

void AnticheatData::SetPosition(float x, float y, float z, float o)
{
    lastMovementInfo.pos = { x, y, z, o };
    lastMovementInfo.time = 0;
}

uint32 AnticheatData::GetLastMapId() const
{
    return lastMapId;
}

void AnticheatData::SetLastMapId(uint32 mapId)
{
    lastMapId = mapId;
}

bool AnticheatData::HasLastMovement() const
{
    return hasLastMovement;
}

void AnticheatData::SetHasLastMovement(bool value)
{
    hasLastMovement = value;
}

uint32 AnticheatData::GetTotalReports() const
{
    return totalReports;
}

void AnticheatData::SetTotalReports(uint32 value)
{
    totalReports = value;
}

uint32 AnticheatData::GetTypeReports(uint8 type) const
{
    return type < MAX_REPORT_TYPES ? typeReports[type] : 0;
}

void AnticheatData::SetTypeReports(uint8 type, uint32 amount)
{
    if (type < MAX_REPORT_TYPES)
        typeReports[type] = amount;
}

float AnticheatData::GetAverage() const
{
    return average;
}

void AnticheatData::SetAverage(float value)
{
    average = value;
}

uint32 AnticheatData::GetCreationTime() const
{
    return creationTime;
}

void AnticheatData::SetCreationTime(uint32 value)
{
    creationTime = value;
}

void AnticheatData::SetTempReports(uint32 amount, uint8 type)
{
    if (type < MAX_REPORT_TYPES)
        tempReports[type] = amount;
}

uint32 AnticheatData::GetTempReports(uint8 type) const
{
    return type < MAX_REPORT_TYPES ? tempReports[type] : 0;
}

void AnticheatData::SetTempReportsTimer(uint32 time, uint8 type)
{
    if (type < MAX_REPORT_TYPES)
        tempReportsTimer[type] = time;
}

uint32 AnticheatData::GetTempReportsTimer(uint8 type) const
{
    return type < MAX_REPORT_TYPES ? tempReportsTimer[type] : 0;
}

void AnticheatData::SetDailyReportState(bool value)
{
    hasDailyReport = value;
}

bool AnticheatData::GetDailyReportState() const
{
    return hasDailyReport;
}

void AnticheatData::LoadPersistentReports(Field* fields)
{
    creationTime = fields[1].GetUInt32();
    average = fields[2].GetFloat();
    totalReports = fields[3].GetUInt32();
    typeReports[SPEED_HACK_REPORT] = fields[4].GetUInt32();
    typeReports[FLY_HACK_REPORT] = fields[5].GetUInt32();
    typeReports[JUMP_HACK_REPORT] = fields[6].GetUInt32();
    typeReports[WALK_WATER_HACK_REPORT] = fields[7].GetUInt32();
    typeReports[TELEPORT_PLANE_HACK_REPORT] = fields[8].GetUInt32();
    typeReports[CLIMB_HACK_REPORT] = fields[9].GetUInt32();
}

void AnticheatData::ResetPersistentReports()
{
    totalReports = 0;
    average = 0.0f;
    creationTime = 0;
    for (uint8 i = 0; i < MAX_REPORT_TYPES; ++i)
        typeReports[i] = 0;
}

void AnticheatData::ResetTemporaryReports()
{
    lastOpcode = 0;
    for (uint8 i = 0; i < MAX_REPORT_TYPES; ++i)
    {
        tempReports[i] = 0;
        tempReportsTimer[i] = 0;
    }
}
