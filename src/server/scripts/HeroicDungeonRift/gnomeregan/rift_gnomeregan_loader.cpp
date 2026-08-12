/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_electrocutioner();
void AddSC_boss_rift_thermaplugg();
void AddSC_npc_rift_walking_bomb();
}

void AddSC_rift_gnomeregan()
{
    HeroicDungeonRift::AddSC_boss_rift_electrocutioner();
    HeroicDungeonRift::AddSC_boss_rift_thermaplugg();
    HeroicDungeonRift::AddSC_npc_rift_walking_bomb();
}
