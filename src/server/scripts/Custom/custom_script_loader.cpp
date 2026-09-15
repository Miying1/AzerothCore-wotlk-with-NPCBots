/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This is where scripts' loading functions should be declared:
// void MyExampleScript()

// 世界BOSS（自定义临时召唤内容，83级，10 人奥杜尔强度）
void AddSC_boss_world_alar();
void AddSC_boss_world_illidan();
void AddSC_boss_world_ragnaros();
void AddSC_boss_world_broodlord();
void AddSC_boss_world_sartura();
void AddSC_boss_world_kurinnaxx();
void AddSC_boss_world_supremus();
void AddSC_boss_world_bloodboil();
void AddSC_boss_world_leotheras();
void AddSC_boss_world_morogrim();
void AddSC_world_boss_guard();

// The name of this function should match:
// void Add${NameOfDirectory}Scripts()
void AddCustomScripts()
{
    // 世界BOSS：统一战斗机制（归属锁定 + 脱战距离 + reaction 守卫）
    AddSC_world_boss_guard();
    // 世界BOSS：奥（Al'ar，风暴要塞复刻）
    AddSC_boss_world_alar();
    // 世界BOSS：伊利丹（Illidan，黑暗神庙复刻）
    AddSC_boss_world_illidan();
    // 世界BOSS：拉格纳罗斯（Ragnaros，熔火之心复刻）
    AddSC_boss_world_ragnaros();
    // 世界BOSS：勒什雷尔（Broodlord Lashlayer，黑翼之巢复刻）
    AddSC_boss_world_broodlord();
    // 世界BOSS：战争守卫沙尔图拉（Battleguard Sartura，安其拉神殿复刻）
    AddSC_boss_world_sartura();
    // 世界BOSS：库林纳克斯（Kurinnaxx，安其拉废墟复刻）
    AddSC_boss_world_kurinnaxx();
    // 世界BOSS：苏普雷姆斯（Supremus，黑暗神庙复刻）
    AddSC_boss_world_supremus();
    // 世界BOSS：古尔图格·血沸（Gurtogg Bloodboil，黑暗神庙复刻）
    AddSC_boss_world_bloodboil();
    // 世界BOSS：盲眼者莱欧瑟拉斯（Leotheras the Blind，毒蛇神殿复刻）
    AddSC_boss_world_leotheras();
    // 世界BOSS：莫洛格里·踏潮者（Morogrim Tidewalker，毒蛇神殿复刻）
    AddSC_boss_world_morogrim();
}
