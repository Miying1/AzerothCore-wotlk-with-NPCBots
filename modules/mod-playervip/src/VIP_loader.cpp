/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 * Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
 */

void AddPlayerItemScripts();
void Addworldboss_list();
void AddPlayerNoFlyInInstanceScript();
void AddPlayerVipBenefitsScripts();
void Addmod_playervipScripts()
{
    AddPlayerItemScripts();
    Addworldboss_list();
    AddPlayerNoFlyInInstanceScript();
    AddPlayerVipBenefitsScripts();
}
