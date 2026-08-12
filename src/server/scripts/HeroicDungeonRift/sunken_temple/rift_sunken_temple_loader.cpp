/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_eranikus();
void AddSC_boss_rift_jammalan();
}

void AddSC_rift_sunken_temple()
{
    HeroicDungeonRift::AddSC_boss_rift_eranikus();
    HeroicDungeonRift::AddSC_boss_rift_jammalan();
}
