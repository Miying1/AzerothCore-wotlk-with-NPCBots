/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or any later version.
 */

#include "rift_defines.h"

#include "AllMapScript.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldScript.h"

namespace HeroicDungeonRift
{
class rift_world_script : public WorldScript
{
public:
    rift_world_script() : WorldScript("rift_world_script") { }

    void OnBeforeWorldInitialized() override
    {
        ConfigStore::Instance().Load();
    }

    void OnShutdown() override
    {
        RunManager::Instance().Clear();
    }
};

class rift_map_script : public AllMapScript
{
public:
    rift_map_script() : AllMapScript("rift_map_script") { }

    void OnPlayerEnterAll(Map* map, Player* player) override
    {
        RunManager::Instance().OnPlayerEnterMap(map, player);
    }

    void OnPlayerLeaveAll(Map* map, Player* player) override
    {
        RunManager::Instance().OnPlayerLeaveMap(map, player);
    }

    void OnDestroyInstance(MapInstanced* /*mapInstanced*/, Map* map) override
    {
        RunManager::Instance().OnDestroyInstance(map);
    }

    void OnMapUpdate(Map* map, uint32 diff) override
    {
        RunManager::Instance().OnMapUpdate(map, diff);
    }
};

void AddSC_rift_hooks()
{
    new rift_world_script();
    new rift_map_script();
}

} // namespace HeroicDungeonRift
