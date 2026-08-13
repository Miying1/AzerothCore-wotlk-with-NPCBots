/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_verdan();
void AddSC_boss_rift_mutanus();
}

void AddSC_rift_wailing_caverns()
{
    HeroicDungeonRift::AddSC_boss_rift_verdan();
    HeroicDungeonRift::AddSC_boss_rift_mutanus();
}
