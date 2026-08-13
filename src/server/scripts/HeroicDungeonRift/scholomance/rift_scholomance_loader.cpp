/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_ras_frostwhisper();
void AddSC_boss_rift_ravenian();
void AddSC_boss_rift_gandling();
void AddSC_npc_rift_risen_guardian();
}

void AddSC_rift_scholomance()
{
    HeroicDungeonRift::AddSC_boss_rift_ras_frostwhisper();
    HeroicDungeonRift::AddSC_boss_rift_ravenian();
    HeroicDungeonRift::AddSC_boss_rift_gandling();
    HeroicDungeonRift::AddSC_npc_rift_risen_guardian();
}
