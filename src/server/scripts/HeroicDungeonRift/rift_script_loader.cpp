/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_rift_entry();
void AddSC_rift_exit_portal();
void AddSC_rift_hooks();
}

void AddSC_rift_deadmines();
void AddSC_rift_gnomeregan();
void AddSC_rift_razorfen_downs();
void AddSC_rift_razorfen_kraul();
void AddSC_rift_scarlet_monastery();
void AddSC_rift_blackfathom_deeps();
void AddSC_rift_uldaman();
void AddSC_rift_sunken_temple();

void AddHeroicDungeonRiftScripts()
{
    HeroicDungeonRift::AddSC_rift_entry();
    HeroicDungeonRift::AddSC_rift_exit_portal();
    HeroicDungeonRift::AddSC_rift_hooks();
    AddSC_rift_deadmines();
    AddSC_rift_gnomeregan();
    AddSC_rift_razorfen_downs();
    AddSC_rift_razorfen_kraul();
    AddSC_rift_scarlet_monastery();
    AddSC_rift_blackfathom_deeps();
    AddSC_rift_uldaman();
    AddSC_rift_sunken_temple();
}
