/*
 * 装备掉落随机附魔（按分组：法系 / 物理 / 防御 / 治疗）
 *
 * 分组数据存于自定义表 mod_random_enchant（entry, ench, attr, attr_type, tier, chance），
 * 不走原版 item_enchantment_template，装备模板也无需绑定 RandomProperty。
 *
 * 流程：
 *   1. 世界启动时（OnLoadCustomDatabaseTable）把 mod_random_enchant 加载进内存；
 *   2. 获得装备时（拾取 / 制造 / 团队 roll，时机同 RandomEnchantsScript.cpp）按主属性推断分组；
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
#include "RandomEnchantCommon.h" // IsEquipmentSlot
#include "ScriptMgr.h"     // WorldScript / MiscScript
#include "Timer.h"         // getMSTime / GetMSTimeDiffToNow

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    // 分组入口 ID（对应 mod_random_enchant.entry）
    constexpr uint32 GROUP_CASTER   = 90001; // 法系
    constexpr uint32 GROUP_PHYSICAL = 90101; // 物理
    constexpr uint32 GROUP_DEFENSE  = 90201; // 防御
    constexpr uint32 GROUP_HEALER   = 90301; // 治疗
    constexpr uint32 GROUP_NONE     = 0;

    // 配置缓存：仅在启动 / 配置重载时读取一次，触发时直接读内存，不再每次查配置。
    struct RandomEnchantConfig
    {
        bool   onLoot        = true;   // RandomEnchants.OnLoot（拾取）
        bool   onCreate      = true;   // RandomEnchants.OnCreate（制造）
        bool   onGroupRoll   = true;   // RandomEnchants.OnGroupRoll（团队 roll）
        bool   enable        = true;   // RandomEnchants.Custom.Enable
        float  chance        = 50.0f;  // RandomEnchants.Custom.Chance
        uint32 minItemLevel  = 220;    // RandomEnchants.Custom.MinItemLevel
        uint32 weightMinLevel = 220;   // RandomEnchants.Custom.WeightMinLevel
        uint32 weightMaxLevel = 284;   // RandomEnchants.Custom.WeightMaxLevel
    };

    RandomEnchantConfig g_config;

    // 启动 / .reload config 时调用，一次性把配置读入缓存。
    void LoadRandomEnchantConfig()
    {
        g_config.onLoot         = sConfigMgr->GetOption<bool>("RandomEnchants.OnLoot", true);
        g_config.onCreate       = sConfigMgr->GetOption<bool>("RandomEnchants.OnCreate", true);
        g_config.onGroupRoll    = sConfigMgr->GetOption<bool>("RandomEnchants.OnGroupRoll", true);
        g_config.enable         = sConfigMgr->GetOption<bool>("RandomEnchants.Custom.Enable", true);
        g_config.chance         = sConfigMgr->GetOption<float>("RandomEnchants.Custom.Chance", 50.0f);
        g_config.minItemLevel   = sConfigMgr->GetOption<uint32>("RandomEnchants.Custom.MinItemLevel", 220);
        g_config.weightMinLevel = sConfigMgr->GetOption<uint32>("RandomEnchants.Custom.WeightMinLevel", 220);
        g_config.weightMaxLevel = sConfigMgr->GetOption<uint32>("RandomEnchants.Custom.WeightMaxLevel", 284);
    }

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
            "SELECT entry, ench, attr, attr_type, tier, chance FROM mod_random_enchant ORDER BY entry, attr, tier");
        if (!result)
        {
            LOG_WARN("server.loading", ">> Loaded 0 custom random enchant definitions. DB table `mod_random_enchant` is empty.");
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
        uint32 wMin = g_config.weightMinLevel;
        uint32 wMax = g_config.weightMaxLevel;
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
    // 判断顺序：
    //   1) 附带防御评级（防御/躲闪/招架/格挡）→ 防御；
    //   2) 附带力量 / 敏捷 / 攻强 → 物理；此时即使同时带智力也算物理
    //      （猎人、增强萨的锁甲都是"敏捷 + 智力"，不能因为带智力就判为法系）；
    //   3) 附带智力 / 法伤 → 法系或治疗（回蓝/精神为治疗，命中/穿透为法系）。
    // 盾牌不单独归防御，按自身属性判定：防御盾（带防御评级）→ 防御，
    // 治疗盾（智力 + 法伤 + 回蓝/精神）→ 治疗，法伤盾（智力 + 法伤 + 命中）→ 法系。
    uint32 GetRandomEnchantGroup(ItemTemplate const* proto)
    {
        bool hasDefense  = false; // 防御类评级
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

        // 1) 防御（坦克）优先：带防御类评级即坦克装（盾牌与是否有防御评级无关）。
        if (hasDefense)
            return GROUP_DEFENSE;

        // 2) 物理：力量 / 敏捷 / 攻强；同时带智力也算物理（猎人 / 增强萨锁甲）。
        if (hasPhysical)
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

// 世界脚本：启动时加载自定义分组表，并在配置（重）加载时刷新配置缓存
class custom_random_enchant_loader : public WorldScript
{
public:
    custom_random_enchant_loader()
        : WorldScript("custom_random_enchant_loader", { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_LOAD_CUSTOM_DATABASE_TABLE }) { }

    // 启动与 .reload config 时各触发一次，配置只在这里读。
    void OnAfterConfigLoad(bool /*reload*/) override
    {
        LoadRandomEnchantConfig();
    }

    void OnLoadCustomDatabaseTable() override
    {
        LoadCustomRandomEnchantTable();
    }
};

// 装备获得钩子：按分组随机附魔。
// 附魔时机与 RandomEnchantsScript.cpp 保持一致：
//   拾取（OnPlayerLootItem）/ 制造（OnPlayerCreateItem）/ 团队 roll 奖励（OnPlayerGroupRollRewardItem），
//   分别由 RandomEnchants.OnLoot / RandomEnchants.OnCreate / RandomEnchants.OnGroupRoll 开关控制。
class item_random_enchant : public PlayerScript
{
public:
    item_random_enchant()
        : PlayerScript("item_random_enchant", { PLAYERHOOK_ON_LOOT_ITEM, PLAYERHOOK_ON_CREATE_ITEM, PLAYERHOOK_ON_GROUP_ROLL_REWARD_ITEM }) { }

    // 拾取物品后。
    void OnPlayerLootItem(Player* /*player*/, Item* item, uint32 /*count*/, ObjectGuid /*lootguid*/) override
    {
        if (g_config.onLoot)
            TryApplyRandomEnchant(item);
    }

    // 制造物品后（专业技能产出）。
    void OnPlayerCreateItem(Player* /*player*/, Item* item, uint32 /*count*/) override
    {
        if (g_config.onCreate)
            TryApplyRandomEnchant(item);
    }

    // 队伍 roll 获得奖励后。
    void OnPlayerGroupRollRewardItem(Player* /*player*/, Item* item, uint32 /*count*/, RollVote /*voteType*/, Roll* /*roll*/) override
    {
        if (g_config.onGroupRoll)
            TryApplyRandomEnchant(item);
    }

private:
    // 核心判定：过滤器 + 分组 + 随机组合，命中则附魔。
    void TryApplyRandomEnchant(Item* item)
    {
        if (!item)
            return;

        ItemTemplate const* itemProto = item->GetTemplate();
        if (!itemProto)
            return;

        // 模块总开关。
        if (!g_config.enable)
            return;

        // 所有装备部位都处理（按装备槽判断，而非物品大类）。
        if (!IsEquipmentSlot(itemProto->InventoryType))
            return;

        // 保留原生随机属性：已配置 RandomProperty / RandomSuffix 的装备不覆盖。
        if (itemProto->RandomProperty || itemProto->RandomSuffix)
            return;

        // 装备等级需严格大于 MinItemLevel（默认 220）。
        if (itemProto->ItemLevel <= g_config.minItemLevel)
            return;

        // 触发概率（可配置）。
        if (!roll_chance_f(g_config.chance))
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
