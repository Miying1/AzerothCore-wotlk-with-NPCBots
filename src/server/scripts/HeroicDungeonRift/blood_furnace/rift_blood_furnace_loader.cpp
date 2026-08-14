/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_broggok();
void AddSC_boss_rift_kelidan();
}

void AddSC_rift_blood_furnace()
{
    HeroicDungeonRift::AddSC_boss_rift_broggok();
    HeroicDungeonRift::AddSC_boss_rift_kelidan();
}
