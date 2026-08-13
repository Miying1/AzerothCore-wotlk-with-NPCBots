/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_smolderweb();
void AddSC_boss_rift_wyrmthalak();
void AddSC_npc_rift_spirestone_warlord();
void AddSC_npc_rift_smolderthorn_berserker();
void AddSC_npc_rift_spire_spiderling();
}

void AddSC_rift_blackrock_spire()
{
    HeroicDungeonRift::AddSC_boss_rift_smolderweb();
    HeroicDungeonRift::AddSC_boss_rift_wyrmthalak();
    HeroicDungeonRift::AddSC_npc_rift_spirestone_warlord();
    HeroicDungeonRift::AddSC_npc_rift_smolderthorn_berserker();
    HeroicDungeonRift::AddSC_npc_rift_spire_spiderling();
}
