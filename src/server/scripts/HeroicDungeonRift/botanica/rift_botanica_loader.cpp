/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_laj();
void AddSC_boss_rift_warp_splinter();
}

void AddSC_rift_botanica()
{
    HeroicDungeonRift::AddSC_boss_rift_laj();
    HeroicDungeonRift::AddSC_boss_rift_warp_splinter();
}
