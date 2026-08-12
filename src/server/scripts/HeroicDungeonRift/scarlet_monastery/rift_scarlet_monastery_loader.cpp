/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_mograine_whitemane();
void AddSC_boss_rift_herod();
}

void AddSC_rift_scarlet_monastery()
{
    HeroicDungeonRift::AddSC_boss_rift_mograine_whitemane();
    HeroicDungeonRift::AddSC_boss_rift_herod();
}
