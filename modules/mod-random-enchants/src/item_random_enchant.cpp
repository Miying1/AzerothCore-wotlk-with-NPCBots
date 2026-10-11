/*
 * 装备掉落随机附魔（按分组：法系 / 物理 / 防御 / 治疗）
 *
 * 分组数据存于自定义表 custom_random_enchant（entry, ench, attr, attr_type, tier, chance），
 * 不走原版 item_enchantment_template，装备模板也无需绑定 RandomProperty。
 *
 * 流程：
 *   1. 世界启动时（OnLoadCustomDatabaseTable）把 custom_random_enchant 加载进内存；
 *   2. 物品创建时（OnItemCreate，覆盖掉落/任务/制造/购买）按主属性推断分组；
 *   3. 先等概率选属性，再在该属性内按装等动态权重（装等越高越偏大序号）挑一个组合；
 *   4. SetItemRandomProperties 把组合内的 SpellItemEnchantment 应用（附魔）到装备。
 *
 * 前提：被附魔装备的 item_template.RandomProperty / RandomSuffix 必须为 0。
 */

#include "Configuration/Config.h" // sConfigMgr
#include "DatabaseEnv.h"   // WorldDatabase
#include "Item.h"          // Item::SetItemRandomProperties
#include "ItemTemplate.h"  // ItemTemplate / ItemModType / InventoryType / ItemClass
#include "Log.h"           // LOG_INFO / LOG_WARN
#include "QueryResult.h"   // QueryResult / Field
#include "Random.h"        // rand_norm / roll_chance_f
#include "ScriptMgr.h"     // WorldScript / MiscScript
#include "Timer.h"         // getMSTime / GetMSTimeDiffToNow

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    // 分组入口 ID（对应 custom_random_enchant.entry）
    constexpr uint32 GROUP_CASTER   = 90001; // 法系
    constexpr uint32 GROUP_PHYSICAL = 90101; // 物理
    constexpr uint32 GROUP_DEFENSE  = 90201; // 防御
    constexpr uint32 GROUP_HEALER   = 90301; // 治疗
    constexpr uint32 GROUP_NONE     = 0;

    // 单个随机条目：ItemRandomProperties 组合 ID + 档位序号 + 权重
    struct RandomEnchEntry
    {
        uint32 ench;
        uint32 tier;
        float  chance;   // 条目权重（同属性内 tier 越大越小）
    };

    // 同一属性下的所有条目（数值从小到大）
    struct RandomEnchAttribute
    {
        std::string attr;      // 属性名
        uint32      attrType;  // ItemModType
        std::vector<RandomEnchEntry> entries;
    };

    // 分组数据：entry -> 属性列表
    std::unordered_map<uint32, std::vector<RandomEnchAttribute>> g_randomEnchGroups;

    // 启动时从自定义表加载分组数据
    void LoadCustomRandomEnchantTable()
    {
        uint32 oldMSTime = getMSTime();
        g_randomEnchGroups.clear();

        QueryResult result = WorldDatabase.Query(
            "SELECT entry, ench, attr, attr_type, tier, chance FROM custom_random_enchant ORDER BY entry, attr, tier");
        if (!result)
        {
            LOG_WARN("server.loading", ">> Loaded 0 custom random enchant definitions. DB table `custom_random_enchant` is empty.");
            return;
        }

        // 临时结构：entry -> (属性名 -> 属性下标)
        std::unordered_map<uint32, std::vector<RandomEnchAttribute>> groups;
        std::unordered_map<uint32, std::unordered_map<std::string, size_t>> attrIndex;

        uint32 count = 0;
        do
        {
            Field* fields = result->Fetch();
            uint32      entry    = fields[0].Get<uint32>();
            uint32      ench     = fields[1].Get<uint32>();
            std::string attr     = fields[2].Get<std::string>();
            uint32      attrType = fields[3].Get<uint32>();
            uint32      tier     = fields[4].Get<uint32>();
            float       chance   = fields[5].Get<float>();

            if (chance <= 0.0f)
            {
                ++count;
                continue;
            }

            std::vector<RandomEnchAttribute>& attrList = groups[entry];
            std::unordered_map<std::string, size_t>&  idxMap  = attrIndex[entry];

            size_t idx;
            auto found = idxMap.find(attr);
            if (found == idxMap.end())
            {
                idx = attrList.size();
                idxMap[attr] = idx;
                attrList.push_back({ attr, attrType, {} });
            }
            else
            {
                idx = found->second;
            }

            RandomEnchAttribute& ra = attrList[idx];
            ra.entries.push_back({ ench, tier, chance });

            ++count;
        } while (result->NextRow());

        g_randomEnchGroups = std::move(groups);

        LOG_INFO("server.loading", ">> Loaded {} custom random enchant definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    }

    // 两级随机：先等概率选属性，再在该属性内按装等动态权重选条目。
    // 装等越高，高数值（大 tier）档位权重越大。
    uint32 GetRandomEnchantFromGroup(uint32 entry, uint32 itemLevel)
    {
        auto it = g_randomEnchGroups.find(entry);
        if (it == g_randomEnchGroups.end() || it->second.empty())
            return 0;

        std::vector<RandomEnchAttribute> const& attrs = it->second;

        // 第一次 roll：等概率选一个属性。
        RandomEnchAttribute const& ra = attrs[urand(uint32(0), uint32(attrs.size() - 1))];

        if (ra.entries.empty())
            return 0;

        // 高数值偏好因子 p：0=低数值占优，1=高数值占优（随装等线性过渡）。
        uint32 wMin = sConfigMgr->GetOption<uint32>("RandomEnchants.Custom.WeightMinLevel", 220);
        uint32 wMax = sConfigMgr->GetOption<uint32>("RandomEnchants.Custom.WeightMaxLevel", 284);
        float p = 0.0f;
        if (wMax > wMin)
            p = std::clamp(float(itemLevel - wMin) / float(wMax - wMin), 0.0f, 1.0f);
        else if (itemLevel > wMin)
            p = 1.0f;

        // 条目动态权重：低装等偏好低数值（chance 为基准），高装等偏好高数值（tier）。
        auto weight = [p](RandomEnchEntry const& e) -> double
        {
            double lowBias  = double(e.chance); // 低装等基准权重（= 档位数 - tier + 1）
            double highBias = double(e.tier);   // 高数值偏好
            return (1.0 - double(p)) * lowBias + double(p) * highBias;
        };

        double totalWeight = 0.0;
        for (RandomEnchEntry const& e : ra.entries)
            totalWeight += weight(e);

        if (totalWeight <= 0.0)
            return 0;

        // 第二次 roll：属性内按动态权重选具体条目。
        double roll = rand_norm() * totalWeight; // [0, totalWeight)
        double acc  = 0.0;
        for (RandomEnchEntry const& e : ra.entries)
        {
            acc += weight(e);
            if (roll < acc)
                return e.ench;
        }

        return ra.entries.back().ench; // 浮点误差兜底
    }

    // 根据物品主属性推断分组。
    // 判断顺序：防御 > 物理 > 法系/治疗；法系与治疗再按命中/穿透/精神/回蓝细分。
    uint32 GetRandomEnchantGroup(ItemTemplate const* proto)
    {
        bool hasDefense  = false; // 防御类评级或盾牌
        bool hasPhysical = false; // 力量 / 敏捷 / 攻强
        bool hasCaster   = false; // 智力 / 法伤
        bool hasSpirit   = false; // 精神
        bool hasMp5      = false; // 每5秒法力回复
        bool hasSpellHit = false; // 法术命中 / 统一命中
        bool hasSpellPen = false; // 法术穿透

        for (uint32 i = 0; i < proto->StatsCount && i < MAX_ITEM_PROTO_STATS; ++i)
        {
            if (proto->ItemStat[i].ItemStatValue <= 0)
                continue;

            switch (proto->ItemStat[i].ItemStatType)
            {
                case ITEM_MOD_INTELLECT:
                case ITEM_MOD_SPELL_POWER:
                    hasCaster = true;
                    break;
                case ITEM_MOD_STRENGTH:
                case ITEM_MOD_AGILITY:
                case ITEM_MOD_ATTACK_POWER:
                    hasPhysical = true;
                    break;
                case ITEM_MOD_SPIRIT:
                    hasSpirit = true;
                    break;
                case ITEM_MOD_MANA_REGENERATION:
                    hasMp5 = true;
                    break;
                case ITEM_MOD_HIT_SPELL_RATING:
                case ITEM_MOD_HIT_RATING:
                    hasSpellHit = true;
                    break;
                case ITEM_MOD_SPELL_PENETRATION:
                    hasSpellPen = true;
                    break;
                case ITEM_MOD_DEFENSE_SKILL_RATING:
                case ITEM_MOD_DODGE_RATING:
                case ITEM_MOD_PARRY_RATING:
                case ITEM_MOD_BLOCK_RATING:
                case ITEM_MOD_BLOCK_VALUE:
                    hasDefense = true;
                    break;
                default:
                    break;
            }
        }

        // 盾牌归防御。
        if (proto->InventoryType == INVTYPE_SHIELD)
            hasDefense = true;

        // 1) 防御（坦克）优先。
        if (hasDefense)
            return GROUP_DEFENSE;

        // 2) 物理（力量 / 敏捷 / 攻强）。
        if (hasPhysical && !hasCaster)
            return GROUP_PHYSICAL;

        // 3) 法系 / 治疗：命中/穿透为法系输出，回蓝/精神为治疗。
        if (hasCaster)
        {
            if (hasMp5)
                return GROUP_HEALER;
            if (hasSpellHit || hasSpellPen)
                return GROUP_CASTER;
            if (hasSpirit)
                return GROUP_HEALER;
            return GROUP_CASTER; // 默认法系
        }

        // 无法归类（如无属性的纯白字武器），不附魔。
        return GROUP_NONE;
    }
}

// 世界脚本：启动时加载自定义分组表
class custom_random_enchant_loader : public WorldScript
{
public:
    custom_random_enchant_loader()
        : WorldScript("custom_random_enchant_loader", { WORLDHOOK_ON_LOAD_CUSTOM_DATABASE_TABLE }) { }

    void OnLoadCustomDatabaseTable() override
    {
        LoadCustomRandomEnchantTable();
    }
};

// 装备创建钩子：按分组随机附魔
class item_random_enchant : public MiscScript
{
public:
    item_random_enchant()
        : MiscScript("item_random_enchant", { MISCHOOK_ON_ITEM_CREATE }) { }

    void OnItemCreate(Item* item, ItemTemplate const* itemProto, Player const* /*owner*/) override
    {
        if (!item || !itemProto)
            return;

        // 模块总开关。
        if (!sConfigMgr->GetOption<bool>("RandomEnchants.Custom.Enable", true))
            return;

        // 只处理装备（武器 / 护甲）。
        if (itemProto->Class != ITEM_CLASS_WEAPON && itemProto->Class != ITEM_CLASS_ARMOR)
            return;

        // 保留原生随机属性：已配置 RandomProperty / RandomSuffix 的装备不覆盖。
        if (itemProto->RandomProperty || itemProto->RandomSuffix)
            return;

        // 装备等级需严格大于 MinItemLevel（默认 220）。
        uint32 minLevel = sConfigMgr->GetOption<uint32>("RandomEnchants.Custom.MinItemLevel", 220);
        if (itemProto->ItemLevel <= minLevel)
            return;

        // 触发概率（可配置）。
        if (!roll_chance_f(sConfigMgr->GetOption<float>("RandomEnchants.Custom.Chance", 100.0f)))
            return;

        uint32 groupEntry = GetRandomEnchantGroup(itemProto);
        if (groupEntry == GROUP_NONE)
            return;

        // 按分组 + 装备等级动态权重随机挑一个组合，应用到物品。
        if (uint32 randomPropId = GetRandomEnchantFromGroup(groupEntry, itemProto->ItemLevel))
            item->SetItemRandomProperties(int32(randomPropId));
    }
};

void AddSC_item_random_enchant()
{
    new custom_random_enchant_loader();
    new item_random_enchant();
}
