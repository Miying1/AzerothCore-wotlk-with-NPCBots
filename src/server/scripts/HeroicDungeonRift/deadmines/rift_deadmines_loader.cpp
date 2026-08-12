/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_vancleef();
void AddSC_boss_rift_sneed();
void AddSC_boss_rift_cookie();
}

void AddSC_rift_deadmines()
{
    HeroicDungeonRift::AddSC_boss_rift_vancleef();
    HeroicDungeonRift::AddSC_boss_rift_sneed();
    HeroicDungeonRift::AddSC_boss_rift_cookie();
}
