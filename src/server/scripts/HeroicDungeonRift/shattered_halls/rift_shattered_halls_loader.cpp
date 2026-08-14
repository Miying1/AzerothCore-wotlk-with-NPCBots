/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_nethekurse();
void AddSC_boss_rift_kargath();
}

void AddSC_rift_shattered_halls()
{
    HeroicDungeonRift::AddSC_boss_rift_nethekurse();
    HeroicDungeonRift::AddSC_boss_rift_kargath();
}
