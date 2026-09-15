/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or any later version.
 */

#include "rift_defines.h"

#include "Creature.h"
#include "DatabaseEnv.h"
#include "IWorld.h"
#include "Log.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectMgr.h"
#include "Random.h"
#include "SharedDefines.h"
#include "StringFormat.h"
#include "TemporarySummon.h"
#include "Timer.h"
#include "WorldSessionMgr.h"

#include <algorithm>
#include <ctime>
#include <set>

namespace HeroicDungeonRift
{
namespace
{
// 开启前倒计时提醒的触发点（剩余秒数）。
constexpr uint32 RiftOpenReminder10Minutes = 10 * MINUTE;
constexpr uint32 RiftOpenReminder5Minutes = 5 * MINUTE;
constexpr uint32 RiftOpenReminder1Minute = 1 * MINUTE;
// 开启/结束通知的连发次数。
constexpr uint32 RiftOpenCloseNoticeRepeat = 2;
}

uint32 GetEntranceEntryForTier(uint8 tier)
{
    switch (tier)
    {
        case 1:
            return EntranceEntryTier1;
        case 2:
            return EntranceEntryTier2;
        case 3:
            return EntranceEntryTier3;
        default:
            return 0;
    }
}

uint8 GetTierForEntranceEntry(uint32 entry)
{
    switch (entry)
    {
        case EntranceEntryTier1:
            return 1;
        case EntranceEntryTier2:
            return 2;
        case EntranceEntryTier3:
            return 3;
        default:
            return 0;
    }
}

uint32 GetEntranceAuraForTier(uint8 tier)
{
    switch (tier)
    {
        case 1:
            return EntranceAuraTier1;
        case 2:
            return EntranceAuraTier2;
        case 3:
            return EntranceAuraTier3;
        default:
            return 0;
    }
}

RiftSpawnManager& RiftSpawnManager::Instance()
{
    static RiftSpawnManager instance;
    return instance;
}

void RiftSpawnManager::Load()
{
    _regions.clear();
    _points.clear();
    _entrances.clear();
    // 取两个评估间隔中的较大者，保证首个 Update 立即评估一次各区域的时间表。
    _scheduleTimer = SchedulePollIntervalClosedMilliseconds;

    QueryResult regionResult = WorldDatabase.Query(
        "SELECT region_id, region_name, map_id, area_id, center_x, center_y, center_z, "
        "t1_min_count, t1_max_count, t2_min_count, t2_max_count, t3_min_count, t3_max_count, enabled, remark "
        "FROM heroic_dungeon_rift_spawn_region");

    if (!regionResult)
    {
        LOG_WARN("server.loading", ">> Loaded 0 five-player heroic rift spawn regions. Table `heroic_dungeon_rift_spawn_region` is empty or missing.");
        return;
    }

    do
    {
        Field* fields = regionResult->Fetch();
        RiftSpawnRegion region;
        region.RegionId = fields[0].Get<uint32>();
        region.RegionName = fields[1].IsNull() ? std::string() : fields[1].Get<std::string>();
        region.MapId = fields[2].Get<uint16>();
        region.AreaId = fields[3].Get<uint32>();
        region.Center.Relocate(fields[4].Get<float>(), fields[5].Get<float>(), fields[6].Get<float>(), 0.0f);
        region.TierMinCounts[0] = fields[7].Get<uint8>();
        region.TierMaxCounts[0] = fields[8].Get<uint8>();
        region.TierMinCounts[1] = fields[9].Get<uint8>();
        region.TierMaxCounts[1] = fields[10].Get<uint8>();
        region.TierMinCounts[2] = fields[11].Get<uint8>();
        region.TierMaxCounts[2] = fields[12].Get<uint8>();
        region.Enabled = fields[13].Get<uint8>() != 0;
        region.Remark = fields[14].IsNull() ? std::string() : fields[14].Get<std::string>();

        if (!region.RegionId || !region.MapId)
        {
            LOG_ERROR("sql.sql", "Five-player heroic rift spawn region {} is missing region_id/map_id and was ignored.", region.RegionId);
            continue;
        }

        for (uint8 tier = 0; tier < MaxTier; ++tier)
        {
            if (region.TierMinCounts[tier] > region.TierMaxCounts[tier])
            {
                LOG_ERROR("sql.sql", "Five-player heroic rift spawn region {} T{} min count {} exceeds max count {}; min was clamped.",
                    region.RegionId, uint32(tier + 1), uint32(region.TierMinCounts[tier]), uint32(region.TierMaxCounts[tier]));
                region.TierMinCounts[tier] = region.TierMaxCounts[tier];
            }
        }

        _regions[region.RegionId] = std::move(region);
    } while (regionResult->NextRow());

    uint32 loadedPoints = 0;
    QueryResult pointResult = WorldDatabase.Query(
        "SELECT point_id, region_id, x, y, z, o, enabled, remark FROM heroic_dungeon_rift_spawn_point");

    if (!pointResult)
    {
        LOG_WARN("server.loading", ">> Loaded 0 five-player heroic rift spawn points. Table `heroic_dungeon_rift_spawn_point` is empty or missing.");
    }
    else
    {
        do
        {
            Field* fields = pointResult->Fetch();
            RiftSpawnPoint point;
            point.PointId = fields[0].Get<uint32>();
            point.RegionId = fields[1].Get<uint32>();
            point.Pos.Relocate(fields[2].Get<float>(), fields[3].Get<float>(), fields[4].Get<float>(), fields[5].Get<float>());
            point.Enabled = fields[6].Get<uint8>() != 0;
            point.Remark = fields[7].IsNull() ? std::string() : fields[7].Get<std::string>();

            auto regionItr = _regions.find(point.RegionId);
            if (regionItr == _regions.end())
            {
                LOG_ERROR("sql.sql", "Five-player heroic rift spawn point {} references unknown region {} and was ignored.", point.PointId, point.RegionId);
                continue;
            }

            // 未启用的点位静默跳过，便于运营先行铺点、逐个启用。
            if (!point.PointId || !point.Enabled)
                continue;

            if (!MapMgr::IsValidMapCoord(regionItr->second.MapId, point.Pos))
            {
                LOG_ERROR("sql.sql", "Five-player heroic rift spawn point {} has invalid coordinates for map {} and was ignored.",
                    point.PointId, regionItr->second.MapId);
                continue;
            }

            uint32 pointId = point.PointId;
            regionItr->second.PointIds.push_back(pointId);
            _points[pointId] = std::move(point);
            ++loadedPoints;
        } while (pointResult->NextRow());
    }

    // 每周开启时段（按区域）。某区域没有启用窗口时视为未配置时间表，该区域全天可刷新。
    uint32 loadedWindows = 0;
    QueryResult scheduleResult = WorldDatabase.Query(
        "SELECT schedule_id, region_id, week_day, start_hour, start_minute, end_hour, end_minute, enabled, remark "
        "FROM heroic_dungeon_rift_schedule");

    if (!scheduleResult)
    {
        LOG_WARN("server.loading", ">> Loaded 0 five-player heroic rift schedule windows. Table `heroic_dungeon_rift_schedule` is empty or missing; every region stays open all week.");
    }
    else
    {
        do
        {
            Field* fields = scheduleResult->Fetch();
            RiftScheduleWindow window;
            window.ScheduleId = fields[0].Get<uint32>();
            window.RegionId = fields[1].Get<uint32>();
            // week_day = -1 表示每天（0~6）都生效，其余取值必须落在 0~6。
            window.WeekDay = fields[2].Get<int8>();
            uint8 startHour = fields[3].Get<uint8>();
            uint8 startMinute = fields[4].Get<uint8>();
            uint8 endHour = fields[5].Get<uint8>();
            uint8 endMinute = fields[6].Get<uint8>();
            window.Enabled = fields[7].Get<uint8>() != 0;
            window.Remark = fields[8].IsNull() ? std::string() : fields[8].Get<std::string>();

            if (!window.ScheduleId || window.WeekDay < RiftWeekDayEveryDay || window.WeekDay > 6 ||
                startHour > 23 || endHour > 23 || startMinute > 59 || endMinute > 59)
            {
                LOG_ERROR("sql.sql", "Five-player heroic rift schedule {} has invalid weekday/time and was ignored.", window.ScheduleId);
                continue;
            }

            auto regionItr = _regions.find(window.RegionId);
            if (regionItr == _regions.end())
            {
                LOG_ERROR("sql.sql", "Five-player heroic rift schedule {} references unknown region {} and was ignored.",
                    window.ScheduleId, window.RegionId);
                continue;
            }

            // 未启用的窗口静默跳过，便于运营先铺时段再逐个启用。
            if (!window.Enabled)
                continue;

            window.StartMinuteOfDay = uint16(startHour) * 60 + startMinute;
            window.EndMinuteOfDay = uint16(endHour) * 60 + endMinute;
            regionItr->second.Windows.push_back(std::move(window));
            ++loadedWindows;
        } while (scheduleResult->NextRow());
    }

    for (uint8 tier = 1; tier <= MaxTier; ++tier)
    {
        uint32 entry = GetEntranceEntryForTier(tier);
        if (!sObjectMgr->GetCreatureTemplate(entry))
            LOG_ERROR("sql.sql", "Five-player heroic rift entrance tier {} creature entry {} is missing from `creature_template`.", tier, entry);
    }

    for (auto const& pair : _regions)
        if (pair.second.Enabled && pair.second.PointIds.empty())
            LOG_WARN("server.loading", "Five-player heroic rift spawn region {} is enabled but has no usable spawn point.", pair.first);

    LOG_INFO("server.loading", ">> Loaded {} five-player heroic rift spawn regions, {} spawn points and {} schedule windows.",
        _regions.size(), loadedPoints, loadedWindows);
}

RiftSpawnPoint const* RiftSpawnManager::GetPoint(uint32 pointId) const
{
    auto itr = _points.find(pointId);
    return itr == _points.end() ? nullptr : &itr->second;
}

bool RiftSpawnManager::IsWithinWindow(RiftScheduleWindow const& window, uint32 weekDay, uint32 minuteOfDay) const
{
    if (window.StartMinuteOfDay == window.EndMinuteOfDay)
        return false;

    // week_day = -1 表示每天（0~6）都生效。
    bool const everyDay = window.WeekDay == RiftWeekDayEveryDay;

    if (window.StartMinuteOfDay < window.EndMinuteOfDay)
        return (everyDay || weekDay == uint32(window.WeekDay)) &&
            minuteOfDay >= window.StartMinuteOfDay && minuteOfDay < window.EndMinuteOfDay;

    // 跨零点窗口：当天 start 之后，或次日 end 之前。
    uint32 nextDay = everyDay ? weekDay : (uint32(window.WeekDay) + 1) % 7;
    return ((everyDay || weekDay == uint32(window.WeekDay)) && minuteOfDay >= window.StartMinuteOfDay) ||
        ((everyDay || weekDay == nextDay) && minuteOfDay < window.EndMinuteOfDay);
}

bool RiftSpawnManager::EvaluateSchedule(RiftSpawnRegion const& region) const
{
    // 未配置任何启用窗口：该区域全天可刷新。
    if (region.Windows.empty())
        return true;

    std::tm local = Acore::Time::TimeBreakdown();
    uint32 weekDay = uint32(local.tm_wday);
    uint32 minuteOfDay = uint32(local.tm_hour) * 60 + uint32(local.tm_min);

    for (RiftScheduleWindow const& window : region.Windows)
        if (IsWithinWindow(window, weekDay, minuteOfDay))
            return true;

    return false;
}

std::string RiftSpawnManager::GetRegionDisplayName(RiftSpawnRegion const& region) const
{
    return region.RegionName.empty()
        ? Acore::StringFormat("区域{}", region.RegionId)
        : region.RegionName;
}

bool RiftSpawnManager::IsAnyRegionOpen() const
{
    for (auto const& pair : _regions)
        if (pair.second.Enabled && pair.second.WindowInitialized && pair.second.WindowOpen)
            return true;

    return false;
}

int64 RiftSpawnManager::ComputeNextOpenTime(RiftSpawnRegion const& region) const
{
    if (region.Windows.empty())
        return 0;

    time_t now = std::time(nullptr);
    std::tm local = Acore::Time::TimeBreakdown(now);
    uint32 currentWeekDay = uint32(local.tm_wday);
    uint32 currentMinuteOfDay = uint32(local.tm_hour) * 60 + uint32(local.tm_min);

    int64 best = 0;
    for (RiftScheduleWindow const& window : region.Windows)
    {
        if (window.StartMinuteOfDay == window.EndMinuteOfDay)
            continue;

        uint32 daysAhead = 0;
        if (window.WeekDay == RiftWeekDayEveryDay)
        {
            // 每天生效：今天尚未到点则今天开启，否则顺延到明天。
            daysAhead = window.StartMinuteOfDay <= currentMinuteOfDay ? 1 : 0;
        }
        else
        {
            daysAhead = (uint32(window.WeekDay) + 7 - currentWeekDay) % 7;
            if (daysAhead == 0 && window.StartMinuteOfDay <= currentMinuteOfDay)
                daysAhead = 7;
        }

        std::tm target = local;
        target.tm_hour = int(window.StartMinuteOfDay / 60);
        target.tm_min = int(window.StartMinuteOfDay % 60);
        target.tm_sec = 0;
        target.tm_mday += int(daysAhead);

        time_t candidate = std::mktime(&target);
        if (candidate <= 0)
            continue;

        if (best == 0 || int64(candidate) < best)
            best = int64(candidate);
    }

    return best;
}

void RiftSpawnManager::UpdateRegionOpenReminders(RiftSpawnRegion& region)
{
    // 未配置时间表的区域全天可刷新，没有“开启倒计时”。
    if (region.Windows.empty())
        return;

    int64 nextOpen = ComputeNextOpenTime(region);
    if (nextOpen != region.CachedNextOpenTime)
    {
        region.CachedNextOpenTime = nextOpen;
        region.OpenReminderStage = 0;
    }

    if (nextOpen <= 0)
        return;

    int64 remaining = nextOpen - int64(std::time(nullptr));
    if (remaining <= 0)
        return;

    uint32 stage = 0;
    if (remaining <= int64(RiftOpenReminder10Minutes))
        stage = 1;
    if (remaining <= int64(RiftOpenReminder5Minutes))
        stage = 2;
    if (remaining <= int64(RiftOpenReminder1Minute))
        stage = 3;

    if (stage == 0 || stage <= region.OpenReminderStage)
        return;

    region.OpenReminderStage = stage;

    std::string const name = GetRegionDisplayName(region);
    switch (stage)
    {
        case 1:
            BroadcastNotice(Acore::StringFormat("【英雄裂隙】{} 的裂隙入口将在 10 分钟后开启。", name));
            break;
        case 2:
            BroadcastNotice(Acore::StringFormat("【英雄裂隙】{} 的裂隙入口将在 5 分钟后开启。", name));
            break;
        case 3:
            BroadcastNotice(Acore::StringFormat("【英雄裂隙】{} 的裂隙入口将在 1 分钟后开启。", name));
            break;
        default:
            break;
    }
}

void RiftSpawnManager::PollSchedules()
{
    // 每个区域按各自的时间表独立开关，互不影响。
    for (auto& pair : _regions)
        PollRegionSchedule(pair.second);
}

void RiftSpawnManager::PollRegionSchedule(RiftSpawnRegion& region)
{
    if (!region.Enabled)
        return;

    bool const open = EvaluateSchedule(region);
    // 未配置时间表的区域全天可刷新，没有开启/关闭概念，不做广播。
    bool const announce = !region.Windows.empty();
    std::string const name = GetRegionDisplayName(region);

    if (!region.WindowInitialized)
    {
        region.WindowInitialized = true;
        region.WindowOpen = open;
        region.RefillTimer = 0;
        region.RetryCountdown = 0;
        if (open)
        {
            // 启动时若该区域已处于开启窗口，直接按最大数量刷满。
            RefreshRegion(region, true);
            if (announce)
                BroadcastOpenCloseNotice(Acore::StringFormat("【英雄裂隙】{} 的裂隙入口已开启！", name));
        }
        else
        {
            UpdateRegionOpenReminders(region);
        }
        return;
    }

    if (open != region.WindowOpen)
    {
        region.WindowOpen = open;
        region.RefillTimer = 0;
        region.RetryCountdown = 0;

        if (open)
        {
            RefreshRegion(region, true);
            if (announce)
                BroadcastOpenCloseNotice(Acore::StringFormat("【英雄裂隙】{} 的裂隙入口已开启！", name));
        }
        else
        {
            // 仅在关闭这一刻清空该区域已刷新的入口，之后不再重复清理。
            RemoveRegionEntrances(region.RegionId);
            if (announce)
                BroadcastOpenCloseNotice(Acore::StringFormat("【英雄裂隙】{} 的裂隙入口已关闭。", name));
        }
    }

    if (!open)
        UpdateRegionOpenReminders(region);
}

void RiftSpawnManager::BroadcastNotice(std::string const& message) const
{
    sWorldSessionMgr->SendServerMessage(SERVER_MSG_STRING, message);
}

void RiftSpawnManager::BroadcastOpenCloseNotice(std::string const& message) const
{
    // 开启/结束连发两条相同通知，确保玩家不会错过。
    for (uint32 i = 0; i < RiftOpenCloseNoticeRepeat; ++i)
        BroadcastNotice(message);
}

void RiftSpawnManager::RemoveRegionEntrances(uint32 regionId)
{
    // 关闭某个区域：移除该区域已刷新的全部入口实体并删除记录，已消耗待移除的一并清理，释放点位。
    for (auto itr = _entrances.begin(); itr != _entrances.end();)
    {
        if (itr->second.RegionId != regionId)
        {
            ++itr;
            continue;
        }

        if (Map* map = sMapMgr->FindMap(itr->second.MapId, 0))
            if (Creature* creature = map->GetCreature(itr->second.Guid))
                creature->DespawnOrUnsummon();

        LOG_DEBUG("scripts", "Five-player heroic rift entrance T{} removed with region {}: point {}, guid {}.",
            uint32(itr->second.Tier), regionId, itr->second.PointId, itr->second.Guid.ToString());

        itr = _entrances.erase(itr);
    }
}

bool RiftSpawnManager::IsRegionBelowMin(RiftSpawnRegion const& region) const
{
    // 只统计「生物仍然存在」的入口：网格卸载、被击杀或外部移除留下的幽灵记录不能算数，
    // 否则该区域的 min 会被虚高的计数满足，导致迟迟不补刷。
    // 地图不在内存时视为全部缺失（下方计数全为 0），交由 RefreshRegion 重建后再补齐。
    Map* map = sMapMgr->FindMap(region.MapId, 0);

    uint32 activeByTier[MaxTier] = { 0, 0, 0 };
    if (map)
    {
        for (auto const& entrance : _entrances)
        {
            if (entrance.second.RegionId != region.RegionId || entrance.second.Consumed)
                continue;

            if (!map->GetCreature(entrance.second.Guid))
                continue;

            if (entrance.second.Tier >= 1 && entrance.second.Tier <= MaxTier)
                ++activeByTier[entrance.second.Tier - 1];
        }
    }

    for (uint8 tier = 1; tier <= MaxTier; ++tier)
    {
        uint32 minCount = std::min<uint32>(region.TierMinCounts[tier - 1], region.TierMaxCounts[tier - 1]);
        if (activeByTier[tier - 1] < minCount)
            return true;
    }

    return false;
}

bool RiftSpawnManager::RefreshRegion(RiftSpawnRegion const& region, bool fillToMax)
{
    Map* map = sMapMgr->CreateBaseMap(region.MapId);
    if (!map || map->Instanceable())
    {
        LOG_ERROR("scripts", "Five-player heroic rift spawn region {} targets non-world map {} and cannot spawn entrances.",
            region.RegionId, region.MapId);
        return false;
    }

    // 统计本区域已占用的点位，并清理已经消失的入口记录（例如网格卸载导致的丢失）。
    std::set<uint32> occupied;
    uint32 activeByTier[MaxTier] = { 0, 0, 0 };
    for (auto itr = _entrances.begin(); itr != _entrances.end();)
    {
        if (itr->second.RegionId != region.RegionId)
        {
            ++itr;
            continue;
        }

        if (!map->GetCreature(itr->second.Guid))
        {
            itr = _entrances.erase(itr);
            continue;
        }

        occupied.insert(itr->second.PointId);
        if (!itr->second.Consumed && itr->second.Tier >= 1 && itr->second.Tier <= MaxTier)
            ++activeByTier[itr->second.Tier - 1];
        ++itr;
    }

    std::vector<uint32> freePoints;
    freePoints.reserve(region.PointIds.size());
    for (uint32 pointId : region.PointIds)
        if (occupied.find(pointId) == occupied.end())
            freePoints.push_back(pointId);

    bool complete = true;
    for (uint8 tier = 1; tier <= MaxTier; ++tier)
    {
        uint32 target = std::min<uint32>(fillToMax ? region.TierMaxCounts[tier - 1] : region.TierMinCounts[tier - 1],
            region.TierMaxCounts[tier - 1]);
        for (uint32 count = activeByTier[tier - 1]; count < target; ++count)
        {
            if (!SpawnOne(region, map, tier, freePoints))
            {
                // 点位耗尽等原因导致该档位未能补齐，交由调用方退避后再试。
                complete = false;
                break;
            }
        }
    }

    return complete;
}

bool RiftSpawnManager::SpawnOne(RiftSpawnRegion const& region, Map* map, uint8 tier, std::vector<uint32>& freePoints)
{
    if (freePoints.empty())
        return false;

    uint32 entry = GetEntranceEntryForTier(tier);
    if (!entry)
        return false;

    uint32 index = urand(0, uint32(freePoints.size()) - 1);
    uint32 pointId = freePoints[index];
    freePoints[index] = freePoints.back();
    freePoints.pop_back();

    RiftSpawnPoint const* point = GetPoint(pointId);
    if (!point)
        return false;

    // 常驻临时生物：只由刷新管理器在被使用后主动移除，运行态不写入数据库。
    TempSummon* summon = map->SummonCreature(entry, point->Pos, nullptr, 0);
    if (!summon)
        return false;

    Creature* creature = summon;
    // 虚空门生物默认是敌对触发体，这里改为友好、被动，并开放对话标记以便点击。
    creature->SetFaction(35);
    creature->SetReactState(REACT_PASSIVE);
    creature->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
    if (uint32 aura = GetEntranceAuraForTier(tier))
        creature->AddAura(aura, creature);

    EntranceEntry record;
    record.Token = _nextToken++;
    record.Guid = creature->GetGUID();
    record.RegionId = region.RegionId;
    record.PointId = pointId;
    record.MapId = region.MapId;
    record.Tier = tier;
    _entrances[record.Guid] = record;

    LOG_DEBUG("scripts", "Five-player heroic rift entrance T{} spawned: region {}, point {}, guid {}.",
        uint32(tier), region.RegionId, pointId, record.Guid.ToString());
    return true;
}

void RiftSpawnManager::Update(uint32 diff)
{
    // 时间表评估不再每帧执行：有区域处于开启窗口时每秒一次（到点即刷），
    // 全部区域关闭时放宽到每 5 秒一次（节流）。
    _scheduleTimer += diff;
    uint32 const pollInterval = IsAnyRegionOpen()
        ? SchedulePollIntervalOpenMilliseconds
        : SchedulePollIntervalClosedMilliseconds;
    if (_scheduleTimer >= pollInterval)
    {
        _scheduleTimer = 0;
        PollSchedules();
    }

    // 被使用过的入口：保持 5 秒后移除，并释放其点位（与所在区域是否开启无关）。
    for (auto itr = _entrances.begin(); itr != _entrances.end();)
    {
        EntranceEntry& entry = itr->second;
        if (!entry.Consumed)
        {
            ++itr;
            continue;
        }

        entry.PurgeCountdown = entry.PurgeCountdown > diff ? entry.PurgeCountdown - diff : 0;
        if (entry.PurgeCountdown)
        {
            ++itr;
            continue;
        }

        if (Map* map = sMapMgr->FindMap(entry.MapId, 0))
            if (Creature* creature = map->GetCreature(entry.Guid))
                creature->DespawnOrUnsummon();

        itr = _entrances.erase(itr);
    }

    // 逐区域维持数量：低于 min 尝试立即补齐到 min，补齐失败则退避重试；
    // 其余情况每 5 分钟向 max 补齐一次。
    for (auto& pair : _regions)
    {
        RiftSpawnRegion& region = pair.second;
        if (!region.Enabled || !region.WindowOpen)
            continue;

        if (region.RetryCountdown)
            region.RetryCountdown = region.RetryCountdown > diff ? region.RetryCountdown - diff : 0;

        if (IsRegionBelowMin(region))
        {
            region.RefillTimer = 0;
            // 退避期内不再重试，避免点位不足时每个世界帧反复尝试。
            if (region.RetryCountdown == 0 && !RefreshRegion(region, false))
                region.RetryCountdown = EntranceRefillRetryIntervalMilliseconds;
            continue;
        }

        region.RefillTimer += diff;
        if (region.RefillTimer >= EntranceRefillIntervalMilliseconds)
        {
            region.RefillTimer = 0;
            RefreshRegion(region, true);
        }
    }
}

void RiftSpawnManager::Clear()
{
    // 关闭时地图会统一卸载，这里只清理内存记录，避免在关闭流程中操作实体。
    _entrances.clear();
    _regions.clear();
    _points.clear();
    _scheduleTimer = 0;
}

uint8 RiftSpawnManager::GetEntranceTier(Creature const* creature) const
{
    if (!creature)
        return 0;

    auto itr = _entrances.find(creature->GetGUID());
    if (itr == _entrances.end() || itr->second.Tier != GetTierForEntranceEntry(creature->GetEntry()))
        return 0;

    return itr->second.Tier;
}

bool RiftSpawnManager::IsEntranceConsumed(Creature const* creature) const
{
    if (!creature)
        return true;

    auto itr = _entrances.find(creature->GetGUID());
    return itr == _entrances.end() || itr->second.Consumed;
}

void RiftSpawnManager::ConsumeEntrance(Creature* creature)
{
    if (!creature)
        return;

    auto itr = _entrances.find(creature->GetGUID());
    if (itr == _entrances.end() || itr->second.Consumed)
        return;

    itr->second.Consumed = true;
    itr->second.PurgeCountdown = EntrancePurgeGraceMilliseconds;

    // 立刻停止交互：移除对话标记后客户端不再提供“进入裂隙”选项。
    creature->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);

    LOG_DEBUG("scripts", "Five-player heroic rift entrance T{} consumed: region {}, point {}, guid {}.",
        uint32(itr->second.Tier), itr->second.RegionId, itr->second.PointId, itr->second.Guid.ToString());
}

} // namespace HeroicDungeonRift
