/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_lethtendris();
void AddSC_boss_rift_alzzin();
void AddSC_boss_rift_immolthar();
void AddSC_boss_rift_tortheldrin();
void AddSC_boss_rift_moldar();
void AddSC_boss_rift_gordok();
void AddSC_npc_rift_alzzin_minion();
}

void AddSC_rift_dire_maul()
{
    HeroicDungeonRift::AddSC_boss_rift_lethtendris();
    HeroicDungeonRift::AddSC_boss_rift_alzzin();
    HeroicDungeonRift::AddSC_boss_rift_immolthar();
    HeroicDungeonRift::AddSC_boss_rift_tortheldrin();
    HeroicDungeonRift::AddSC_boss_rift_moldar();
    HeroicDungeonRift::AddSC_boss_rift_gordok();
    HeroicDungeonRift::AddSC_npc_rift_alzzin_minion();
}
