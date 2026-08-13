/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_vyletongue();
void AddSC_boss_rift_celebras();
void AddSC_boss_rift_theradras();
}

void AddSC_rift_maraudon()
{
    HeroicDungeonRift::AddSC_boss_rift_vyletongue();
    HeroicDungeonRift::AddSC_boss_rift_celebras();
    HeroicDungeonRift::AddSC_boss_rift_theradras();
}
