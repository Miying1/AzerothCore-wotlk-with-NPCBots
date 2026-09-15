/*
 * 世界BOSS公共定义（自定义临时召唤内容）
 *
 * 本文件定义自定义世界BOSS（83级，强度对齐 10 人冰冠堡垒）的 entry 常量。
 * 这些BOSS用于在世界地图上随机刷新（临时召唤），没有副本实例环境。
 *
 * 技能伤害缩放由 world_boss_guard.cpp 的基类（WorldBossGuardAI / WorldBossSummonAI）
 * 在 DamageDealt 里按施法者身份实现，天然隔离原版内容，避免污染复用同一法术的原版 70 级技能。
 */

#ifndef CUSTOM_WORLD_BOSS_COMMON_H
#define CUSTOM_WORLD_BOSS_COMMON_H

#include "Define.h"

// ---- 奥（Al'ar，风暴要塞复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_ALAR             = 120100; // 奥
constexpr uint32 NPC_WORLD_BOSS_ALAR_EMBER       = 120500; // 奥的余烬
constexpr uint32 NPC_WORLD_BOSS_ALAR_FLAME_PATCH = 120501; // 烈焰之痕

// ---- 伊利丹（Illidan，黑暗神庙复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_ILLIDAN                     = 120101; // 伊利丹·怒风
constexpr uint32 NPC_WORLD_BOSS_ILLIDAN_PARASITIC_SHADOWFIEND = 120502; // 寄生暗影魔
constexpr uint32 NPC_WORLD_BOSS_ILLIDAN_BLADE_OF_AZZINOTH      = 120503; // 阿兹诺斯之刃
constexpr uint32 NPC_WORLD_BOSS_ILLIDAN_FLAME_OF_AZZINOTH      = 120504; // 阿兹诺斯烈焰
constexpr uint32 NPC_WORLD_BOSS_ILLIDAN_SHADOW_DEMON           = 120505; // 暗影魔

// ---- 拉格纳罗斯（Ragnaros，熔火之心复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_RAGNAROS              = 120102; // 拉格纳罗斯（炎魔之王）
constexpr uint32 NPC_WORLD_BOSS_RAGNAROS_SON_OF_FLAME = 120506; // 火焰之子

// ---- 勒什雷尔（Broodlord Lashlayer，黑翼之巢复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_BROODLORD = 120103; // 勒什雷尔（龙类将军）

// ---- 战争守卫沙尔图拉（Battleguard Sartura，安其拉神殿复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_SARTURA       = 120104; // 战争守卫沙尔图拉
constexpr uint32 NPC_WORLD_BOSS_SARTURA_GUARD = 120507; // 沙尔图拉的皇家守卫

// ---- 库林纳克斯（Kurinnaxx，安其拉废墟复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_KURINNAXX = 120105; // 库林纳克斯

// ---- 苏普雷姆斯（Supremus，黑暗神庙复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_SUPREMUS               = 120106; // 苏普雷姆斯
constexpr uint32 NPC_WORLD_BOSS_SUPREMUS_PUNCH_STALKER = 120509; // 苏普雷姆斯的熔岩拳隐形巡者
constexpr uint32 NPC_WORLD_BOSS_SUPREMUS_VOLCANO       = 120510; // 苏普雷姆斯火山

// ---- 古尔图格·血沸（Gurtogg Bloodboil，黑暗神庙复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_BLOODBOIL_GEYSER = 120511; // 古尔图格·血沸的邪能间歇泉

// ---- 古尔图格·血沸（Gurtogg Bloodboil，黑暗神庙复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_BLOODBOIL = 120107; // 古尔图格·血沸

// ---- 盲眼者莱欧瑟拉斯（Leotheras the Blind，毒蛇神殿复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_LEOTHERAS        = 120108; // 盲眼者莱欧瑟拉斯
constexpr uint32 NPC_WORLD_BOSS_LEOTHERAS_SHADOW = 120512; // 莱欧瑟拉斯之影
constexpr uint32 NPC_WORLD_BOSS_INNER_DEMON      = 120513; // 内心的恶灵

// ---- 莫洛格里·踏潮者（Morogrim Tidewalker，毒蛇神殿复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_MOROGRIM      = 120109; // 莫洛格里·踏潮者
constexpr uint32 NPC_WORLD_BOSS_MURLOC        = 120514; // 潮行者潜伏者
constexpr uint32 NPC_WORLD_BOSS_WATER_GLOBULE = 120515; // 水晶体

// ---- 阿克蒙德（Archimonde，海加尔山之战复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_ARCHIMONDE                  = 120110; // 阿克蒙德（污染者）
constexpr uint32 NPC_WORLD_BOSS_ARCHIMONDE_DOOMFIRE         = 120516; // 阿克蒙德的毁灭之火
constexpr uint32 NPC_WORLD_BOSS_ARCHIMONDE_DOOMFIRE_SPIRIT  = 120517; // 阿克蒙德的毁灭之火灵魂

// ---- 阿兹加洛（Azgalor，海加尔山之战复刻，83级） ----
constexpr uint32 NPC_WORLD_BOSS_AZGALOR = 120111; // 阿兹加洛

// 判断 entry 是否为自定义世界BOSS本体（不含召唤物）。
// 归属锁定与脱战距离只对本体生效，召唤物跟随本体仇恨，无需单独处理。
constexpr bool IsWorldBossBodyEntry(uint32 entry)
{
    return entry == NPC_WORLD_BOSS_ALAR || entry == NPC_WORLD_BOSS_ILLIDAN
        || entry == NPC_WORLD_BOSS_RAGNAROS || entry == NPC_WORLD_BOSS_BROODLORD
        || entry == NPC_WORLD_BOSS_SARTURA || entry == NPC_WORLD_BOSS_KURINNAXX
        || entry == NPC_WORLD_BOSS_SUPREMUS || entry == NPC_WORLD_BOSS_BLOODBOIL
        || entry == NPC_WORLD_BOSS_LEOTHERAS || entry == NPC_WORLD_BOSS_MOROGRIM
        || entry == NPC_WORLD_BOSS_ARCHIMONDE || entry == NPC_WORLD_BOSS_AZGALOR;
}

// 世界BOSS脱战距离（码）：进入战斗后移动超过该距离即脱战，防止风筝拉脱。
constexpr float WORLD_BOSS_LEASH_RANGE = 150.0f;

#endif // CUSTOM_WORLD_BOSS_COMMON_H
