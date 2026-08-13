/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_springvale();
void AddSC_boss_rift_arugal();
void AddSC_npc_rift_arugal_voidwalker();
}

void AddSC_rift_shadowfang_keep()
{
    HeroicDungeonRift::AddSC_boss_rift_springvale();
    HeroicDungeonRift::AddSC_boss_rift_arugal();
    HeroicDungeonRift::AddSC_npc_rift_arugal_voidwalker();
}
