/*
 * 随机附魔公共判定 —— mod-random-enchants 模块
 *
 * 供 item_random_enchant.cpp（随机附魔）与 EnchantTransfer.cpp（属性萃取/转移）共用，
 * 保证两处处理的装备部位范围始终一致。
 */

#ifndef MOD_RANDOM_ENCHANTS_COMMON_H
#define MOD_RANDOM_ENCHANTS_COMMON_H

#include "Define.h"
#include "ItemTemplate.h"

// 是否属于可装备部位：覆盖全部装备槽（头/颈/肩/背/胸/袍/腰/腿/脚/腕/手/
// 戒指/饰品/主手/副手/双手/远程/投掷/盾/副手物品/圣物），
// 排除不可装备（NON_EQUIP）、容器（BAG）、衬衫（BODY）、战袍（TABARD）、
// 弹药（AMMO）、箭袋（QUIVER）等非装备槽。
inline bool IsEquipmentSlot(uint32 invType)
{
    switch (invType)
    {
        case INVTYPE_HEAD:
        case INVTYPE_NECK:
        case INVTYPE_SHOULDERS:
        case INVTYPE_CHEST:
        case INVTYPE_WAIST:
        case INVTYPE_LEGS:
        case INVTYPE_FEET:
        case INVTYPE_WRISTS:
        case INVTYPE_HANDS:
        case INVTYPE_FINGER:
        case INVTYPE_TRINKET:
        case INVTYPE_WEAPON:
        case INVTYPE_SHIELD:
        case INVTYPE_RANGED:
        case INVTYPE_CLOAK:
        case INVTYPE_2HWEAPON:
        case INVTYPE_ROBE:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_HOLDABLE:
        case INVTYPE_THROWN:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_RELIC:
            return true;
        default:
            return false;
    }
}

#endif // MOD_RANDOM_ENCHANTS_COMMON_H
