/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or any later version.
 */

#include "rift_defines.h"

#include "DatabaseEnv.h"
#include "Log.h"
#include "MapMgr.h"
#include "ObjectMgr.h"

#include <algorithm>

namespace HeroicDungeonRift
{
ConfigStore& ConfigStore::Instance()
{
    static ConfigStore instance;
    return instance;
}

void ConfigStore::Load()
{
    _bosses.clear();
    _tiers.clear();
    _entryToBoss.clear();

    QueryResult bossResult = WorldDatabase.Query(
        "SELECT boss_id, map_name, map_id, player_entry_x, player_entry_y, player_entry_z, "
        "player_entry_o, enabled, remark FROM heroic_dungeon_rift_boss");

    if (!bossResult)
    {
        LOG_WARN("server.loading", ">> Loaded 0 five-player heroic rift bosses. Table `heroic_dungeon_rift_boss` is empty or missing.");
        return;
    }

    do
    {
        Field* fields = bossResult->Fetch();
        BossConfig config;
        config.BossId = fields[0].Get<uint32>();
        config.MapName = fields[1].Get<std::string>();
        config.MapId = fields[2].Get<uint16>();
        config.DefaultPlayerEntry.Relocate(fields[3].Get<float>(), fields[4].Get<float>(), fields[5].Get<float>(), fields[6].Get<float>());
        config.Enabled = fields[7].Get<uint8>() != 0;
        config.Remark = fields[8].IsNull() ? std::string() : fields[8].Get<std::string>();

        if (!config.BossId || !config.MapId || !MapMgr::IsValidMapCoord(config.MapId, config.DefaultPlayerEntry))
        {
            LOG_ERROR("sql.sql", "Five-player heroic rift boss {} has invalid map or player entry coordinates and was ignored.", config.BossId);
            continue;
        }

        _bosses[config.BossId] = std::move(config);
    } while (bossResult->NextRow());

    QueryResult tierResult = WorldDatabase.Query(
        "SELECT boss_id, entry_id, boss_spawn_x, boss_spawn_y, boss_spawn_z, boss_spawn_o, tier, "
        "health_multiplier, damage_multiplier, player_entry_x, player_entry_y, player_entry_z, player_entry_o "
        "FROM heroic_dungeon_rift_boss_tier");

    if (!tierResult)
    {
        LOG_WARN("server.loading", ">> Loaded 0 five-player heroic rift tiers. Table `heroic_dungeon_rift_boss_tier` is empty or missing.");
        return;
    }

    uint32 loadedTiers = 0;
    do
    {
        Field* fields = tierResult->Fetch();
        TierConfig config;
        config.BossId = fields[0].Get<uint32>();
        config.EntryId = fields[1].Get<uint32>();
        config.BossSpawn.Relocate(fields[2].Get<float>(), fields[3].Get<float>(), fields[4].Get<float>(), fields[5].Get<float>());
        config.Tier = fields[6].Get<uint8>();
        config.HealthMultiplier = fields[7].Get<float>();
        config.DamageMultiplier = fields[8].Get<float>();
        config.PlayerEntry.Relocate(fields[9].Get<float>(), fields[10].Get<float>(), fields[11].Get<float>(), fields[12].Get<float>());

        BossConfig const* boss = GetBoss(config.BossId);
        if (!boss || !boss->Enabled || config.Tier < 1 || config.Tier > MaxTier || !config.EntryId ||
            config.HealthMultiplier <= 0.0f || config.DamageMultiplier <= 0.0f)
        {
            LOG_ERROR("sql.sql", "Five-player heroic rift tier boss_id {}, tier {} has invalid keys or multipliers and was ignored.", config.BossId, config.Tier);
            continue;
        }

        if (!sObjectMgr->GetCreatureTemplate(config.EntryId))
        {
            LOG_ERROR("sql.sql", "Five-player heroic rift tier boss_id {}, tier {} references missing creature_template entry {} and was ignored.", config.BossId, config.Tier, config.EntryId);
            continue;
        }

        if (!MapMgr::IsValidMapCoord(boss->MapId, config.BossSpawn) || !MapMgr::IsValidMapCoord(boss->MapId, config.PlayerEntry))
        {
            LOG_ERROR("sql.sql", "Five-player heroic rift tier boss_id {}, tier {} has invalid coordinates and was ignored.", config.BossId, config.Tier);
            continue;
        }

        std::pair<uint32, uint8> key(config.BossId, config.Tier);
        if (_tiers.find(key) != _tiers.end() || _entryToBoss.find(config.EntryId) != _entryToBoss.end())
        {
            LOG_ERROR("sql.sql", "Five-player heroic rift tier boss_id {}, tier {} or entry {} is duplicated and was ignored.", config.BossId, config.Tier, config.EntryId);
            continue;
        }

        _entryToBoss[config.EntryId] = config.BossId;
        _tiers[key] = config;
        ++loadedTiers;
    } while (tierResult->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} five-player heroic rift bosses and {} tier rows.", _bosses.size(), loadedTiers);
}

BossConfig const* ConfigStore::GetBoss(uint32 bossId) const
{
    auto itr = _bosses.find(bossId);
    return itr == _bosses.end() ? nullptr : &itr->second;
}

TierConfig const* ConfigStore::GetTier(uint32 bossId, uint8 tier) const
{
    auto itr = _tiers.find(std::make_pair(bossId, tier));
    return itr == _tiers.end() ? nullptr : &itr->second;
}

TierConfig const* ConfigStore::SelectRandomTier(uint8 tier) const
{
    std::map<uint32, std::vector<TierConfig const*>> candidatesByMap;
    for (auto const& pair : _tiers)
    {
        TierConfig const& config = pair.second;
        BossConfig const* boss = GetBoss(config.BossId);
        if (config.Tier == tier && boss && boss->Enabled)
            candidatesByMap[boss->MapId].push_back(&config);
    }

    if (candidatesByMap.empty())
        return nullptr;

    std::vector<uint32> mapIds;
    mapIds.reserve(candidatesByMap.size());
    for (auto const& pair : candidatesByMap)
        mapIds.push_back(pair.first);

    uint32 selectedMapId = mapIds[urand(0, mapIds.size() - 1)];
    std::vector<TierConfig const*> const& mapCandidates = candidatesByMap.at(selectedMapId);
    return mapCandidates[urand(0, mapCandidates.size() - 1)];
}

uint32 ConfigStore::GetBossIdByEntry(uint32 entryId) const
{
    auto itr = _entryToBoss.find(entryId);
    return itr == _entryToBoss.end() ? 0 : itr->second;
}

} // namespace HeroicDungeonRift
