/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_balnazzar();
void AddSC_boss_rift_anastari();
void AddSC_boss_rift_rivendare();
void AddSC_npc_rift_mindless_skeleton();
}

void AddSC_rift_stratholme()
{
    HeroicDungeonRift::AddSC_boss_rift_balnazzar();
    HeroicDungeonRift::AddSC_boss_rift_anastari();
    HeroicDungeonRift::AddSC_boss_rift_rivendare();
    HeroicDungeonRift::AddSC_npc_rift_mindless_skeleton();
}
