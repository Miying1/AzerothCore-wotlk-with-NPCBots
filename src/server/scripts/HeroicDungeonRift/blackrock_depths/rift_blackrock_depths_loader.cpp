/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

namespace HeroicDungeonRift
{
void AddSC_boss_rift_roccor();
void AddSC_boss_rift_incendius();
void AddSC_boss_rift_baelgar();
void AddSC_boss_rift_argelmach();
void AddSC_boss_rift_flamelash();
void AddSC_boss_rift_the_seven();
void AddSC_boss_rift_thaurissan();
void AddSC_npc_rift_voidwalker_minion();
void AddSC_npc_rift_baelgar_spawn();
}

void AddSC_rift_blackrock_depths()
{
    HeroicDungeonRift::AddSC_boss_rift_roccor();
    HeroicDungeonRift::AddSC_boss_rift_incendius();
    HeroicDungeonRift::AddSC_boss_rift_baelgar();
    HeroicDungeonRift::AddSC_boss_rift_argelmach();
    HeroicDungeonRift::AddSC_boss_rift_flamelash();
    HeroicDungeonRift::AddSC_boss_rift_the_seven();
    HeroicDungeonRift::AddSC_boss_rift_thaurissan();
    HeroicDungeonRift::AddSC_npc_rift_voidwalker_minion();
    HeroicDungeonRift::AddSC_npc_rift_baelgar_spawn();
}
