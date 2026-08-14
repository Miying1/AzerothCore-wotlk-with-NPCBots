/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_omor();
void AddSC_boss_rift_vazruden();
}

void AddSC_rift_hellfire_ramparts()
{
    HeroicDungeonRift::AddSC_boss_rift_omor();
    HeroicDungeonRift::AddSC_boss_rift_vazruden();
}
