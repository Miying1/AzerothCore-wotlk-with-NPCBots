/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_muselek();
void AddSC_boss_rift_black_stalker();
}

void AddSC_rift_underbog()
{
    HeroicDungeonRift::AddSC_boss_rift_muselek();
    HeroicDungeonRift::AddSC_boss_rift_black_stalker();
}
