/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_gahzrilla();
void AddSC_boss_rift_ukorz();
}

void AddSC_rift_zulfarrak()
{
    HeroicDungeonRift::AddSC_boss_rift_gahzrilla();
    HeroicDungeonRift::AddSC_boss_rift_ukorz();
}
