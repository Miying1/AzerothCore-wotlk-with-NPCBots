/*
 * 随机装备礼包（物品 60401 / 60402）使用脚本 —— mod-playervip 模块
 *
 * 功能：
 *   玩家使用礼包后打开 gossip 菜单，自选分类与部位，随机发放一件指定装等的可装备物品。
 *   按礼包用途拆成两个独立脚本，分别绑定不同的 item_template.ScriptName：
 *
 *   1. RandomEquipGiftArmorOtherItem —— 护甲 + 其他
 *        一级菜单：护甲 / 其他
 *        护甲：二级为布甲/皮甲/锁甲/板甲，三级为装备部位，选中部位后发放
 *        其他：二级即部位（颈部/背部/手指/盾牌/副手物品），选完直接发放
 *
 *   2. RandomEquipGiftWeaponItem —— 武器
 *        只有一层菜单，直接列出武器子类（单手斧/双手斧/弓/…），选完直接发放
 *
 * 原理：
 *   - 目标装等写在物品 description 的方括号里，例如 [230]，脚本会解析该数值。
 *   - 候选池一次性扫描内存中的 item_template，按装等缓存，避免每次点击都全表遍历。
 *   - 物品通过 item_template.ScriptName 绑定脚本；
 *     spellid_1 = 18282（无效果触发法术）使客户端把礼包当作「可使用」物品，
 *     OnUse 返回 true 会阻止该法术施放，因此礼包不会被默认消耗。
 */

#include "Chat.h"
#include "Common.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "StringFormat.h"

#include <algorithm>
#include <array>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
// ==================== 分类与名称 ====================

// 一级分类
enum GiftTopCategory : uint32
{
    GIFT_TOP_ARMOR  = 0, // 护甲：二级为布甲/皮甲/锁甲/板甲，三级为部位
    GIFT_TOP_WEAPON = 1, // 武器：二级为武器子类，选完直接发放
    GIFT_TOP_OTHER  = 2, // 其他：二级即部位（颈部/背部/手指/盾牌/副手物品），选完直接发放
    GIFT_TOP_COUNT  = 3
};

// 一级分类中文名
constexpr std::array<char const*, GIFT_TOP_COUNT> TopCategoryNames =
{
    "护甲", "武器", "其他"
};

// 护甲+其他礼包对外暴露的一级分类
constexpr std::array<uint32, 2> ArmorOtherTopCategories =
{
    GIFT_TOP_ARMOR, GIFT_TOP_OTHER
};

// 护甲子类（布甲/皮甲/锁甲/板甲）
constexpr std::array<uint32, 4> ArmorGiftSubClasses =
{
    ITEM_SUBCLASS_ARMOR_CLOTH, ITEM_SUBCLASS_ARMOR_LEATHER, ITEM_SUBCLASS_ARMOR_MAIL, ITEM_SUBCLASS_ARMOR_PLATE
};

// 武器子类（按子类 ID 升序，与拍卖行武器分类一致）
constexpr std::array<uint32, 16> WeaponGiftSubClasses =
{
    ITEM_SUBCLASS_WEAPON_AXE, ITEM_SUBCLASS_WEAPON_AXE2, ITEM_SUBCLASS_WEAPON_BOW, ITEM_SUBCLASS_WEAPON_GUN,
    ITEM_SUBCLASS_WEAPON_MACE, ITEM_SUBCLASS_WEAPON_MACE2, ITEM_SUBCLASS_WEAPON_POLEARM, ITEM_SUBCLASS_WEAPON_SWORD,
    ITEM_SUBCLASS_WEAPON_SWORD2, ITEM_SUBCLASS_WEAPON_STAFF, ITEM_SUBCLASS_WEAPON_FIST, ITEM_SUBCLASS_WEAPON_DAGGER,
    ITEM_SUBCLASS_WEAPON_THROWN, ITEM_SUBCLASS_WEAPON_SPEAR, ITEM_SUBCLASS_WEAPON_CROSSBOW, ITEM_SUBCLASS_WEAPON_WAND
};

// “其他”分类的部位
constexpr std::array<uint32, 5> OtherGiftInvTypes =
{
    INVTYPE_NECK, INVTYPE_CLOAK, INVTYPE_FINGER, INVTYPE_SHIELD, INVTYPE_HOLDABLE
};

// 武器装备子类中文名（下标须与 ITEM_SUBCLASS_WEAPON_* 枚举一一对应：
// 9=obsolete、11=exotic、12=exotic2 均无实际用途，13 才是拳套）
constexpr std::array<char const*, MAX_ITEM_SUBCLASS_WEAPON> WeaponSubClassNames =
{
    "单手斧", "双手斧", "弓", "枪械", "单手锤", "双手锤", "长柄武器", "单手剑", "双手剑", "废弃",
    "法杖", "废弃", "废弃", "拳套", "杂项", "匕首", "投掷武器", "长矛", "弩", "魔杖", "钓鱼竿"
};

// 护甲子类中文名
constexpr std::array<char const*, MAX_ITEM_SUBCLASS_ARMOR> ArmorSubClassNames =
{
    "杂项", "布甲", "皮甲", "锁甲", "板甲", "小型盾牌", "盾牌", "圣契", "神像", "图腾", "魔印"
};

// 装备部位中文名
constexpr std::array<char const*, MAX_INVTYPE> InvTypeNames =
{
    "未装备", "头部", "颈部", "肩部", "衬衣", "胸部", "腰部", "腿部", "脚部", "手腕",
    "手部", "手指", "饰品", "单手", "盾牌", "远程", "背部", "双手", "背包", "战袍",
    "长袍", "主手", "副手", "副手物品", "弹药", "投掷", "远程(右)", "箭袋", "圣物"
};

// 抽取候选（一件可装备的物品）
struct GiftCandidate
{
    uint32 Entry = 0;    // 物品 entry
    uint32 ItemClass = 0; // 物品大类
    uint32 SubClass = 0;  // 物品子类
    uint32 InvType = 0;   // 装备部位（长袍已归一化为胸部）
};

// 表示“该维度不参与过滤”
constexpr uint32 GiftAny = 0xFFFFFFFF;

// gossip 菜单状态编码
constexpr uint32 GossipSenderRoot = GOSSIP_SENDER_MAIN;                  // 一级分类菜单
constexpr uint32 GossipSenderSubClassBase = 100;                         // 二级分类菜单（100 + 一级分类）
constexpr uint32 GossipSenderSlotBase = 1000;                            // 护甲部位菜单（1000 + 护甲子类）
constexpr uint32 GossipSenderWeaponMenu = GossipSenderSubClassBase + GIFT_TOP_WEAPON; // 武器子类菜单（武器礼包唯一一层）
constexpr uint32 GossipActionBase = GOSSIP_ACTION_INFO_DEF;              // 选项动作基准（+ 选项取值）
constexpr uint32 GossipActionBack = GOSSIP_ACTION_INFO_DEF + 90;         // 返回上一级
constexpr uint32 GossipActionClose = GOSSIP_ACTION_INFO_DEF + 91;        // 关闭

// ==================== 基础工具 ====================

template <size_t N>
bool Contains(std::array<uint32, N> const& values, uint32 value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

// 归一化部位：长袍与胸部同属胸甲位置，合并统计
uint32 NormalizeInvType(uint32 invType)
{
    return invType == INVTYPE_ROBE ? uint32(INVTYPE_CHEST) : invType;
}

// 非装备大类（容器/任务物品/钥匙）不参与随机
bool IsNonEquipClass(uint32 itemClass)
{
    switch (itemClass)
    {
        case ITEM_CLASS_CONTAINER:
        case ITEM_CLASS_QUEST:
        case ITEM_CLASS_KEY:
            return true;
        default:
            return false;
    }
}

std::string GetSubClassName(uint32 itemClass, uint32 subClass)
{
    if (itemClass == ITEM_CLASS_WEAPON && subClass < WeaponSubClassNames.size())
        return WeaponSubClassNames[subClass];
    if (itemClass == ITEM_CLASS_ARMOR && subClass < ArmorSubClassNames.size())
        return ArmorSubClassNames[subClass];
    return Acore::StringFormat("子类{}", subClass);
}

std::string GetInvTypeName(uint32 invType)
{
    if (invType < InvTypeNames.size())
        return InvTypeNames[invType];
    return Acore::StringFormat("部位{}", invType);
}

// 是否属于抽取范围：只收“护甲四系 / 武器各系 / 其他五个部位”，其余全部忽略
bool IsGiftCandidate(GiftCandidate const& candidate)
{
    if (candidate.ItemClass == ITEM_CLASS_ARMOR)
    {
        // “其他”的五个部位优先判定，保证三个一级分类互不重叠
        if (Contains(OtherGiftInvTypes, candidate.InvType))
            return true;
        return Contains(ArmorGiftSubClasses, candidate.SubClass);
    }

    if (candidate.ItemClass == ITEM_CLASS_WEAPON)
        return Contains(WeaponGiftSubClasses, candidate.SubClass);

    return false;
}

// 取候选所属的一级分类
uint32 GetTopCategory(GiftCandidate const& candidate)
{
    if (candidate.ItemClass != ITEM_CLASS_ARMOR)
        return GIFT_TOP_WEAPON;

    return Contains(OtherGiftInvTypes, candidate.InvType) ? GIFT_TOP_OTHER : GIFT_TOP_ARMOR;
}

// ==================== 候选池构建与缓存 ====================

// 扫描内存中的物品模板，构建指定装等的候选池
std::vector<GiftCandidate> BuildGiftCandidates(uint32 targetItemLevel)
{
    std::vector<GiftCandidate> candidates;
    ItemTemplateContainer const* templates = sObjectMgr->GetItemTemplateStore();
    for (auto const& [entry, proto] : *templates)
    {
        if (proto.ItemLevel != targetItemLevel)
            continue;

        // 只要可装备物品（排除未装备、背包、弹药袋），并限定部位下标上界，
        // 避免异常数据在按部位统计时越界
        if (proto.InventoryType >= MAX_INVTYPE || proto.InventoryType == INVTYPE_NON_EQUIP
            || proto.InventoryType == INVTYPE_BAG || proto.InventoryType == INVTYPE_QUIVER)
            continue;

        // 排除非装备大类、已废弃物品、传家宝
        if (IsNonEquipClass(proto.Class) || proto.HasFlag(ITEM_FLAG_DEPRECATED)
            || proto.Quality == ITEM_QUALITY_HEIRLOOM)
            continue;

        GiftCandidate candidate;
        candidate.Entry = entry;
        candidate.ItemClass = proto.Class;
        candidate.SubClass = proto.SubClass;
        candidate.InvType = NormalizeInvType(proto.InventoryType);
        if (IsGiftCandidate(candidate))
            candidates.push_back(candidate);
    }
    return candidates;
}

// item_template 在运行期不会变化，按装等缓存候选池，避免每次 gossip 点击都全表扫描
std::mutex GiftCandidateCacheMutex;
std::unordered_map<uint32, std::vector<GiftCandidate>> GiftCandidateCache;
size_t GiftCandidateCacheTemplateCount = 0;

// 取指定装等的候选池（命中缓存时直接返回副本，未命中则构建后写入缓存）
std::vector<GiftCandidate> GetGiftCandidates(uint32 targetItemLevel)
{
    std::lock_guard<std::mutex> lock(GiftCandidateCacheMutex);

    ItemTemplateContainer const* templates = sObjectMgr->GetItemTemplateStore();
    // 物品模板数量发生变化（如重载过模板表）时整体失效，避免继续返回过期数据
    if (templates->size() != GiftCandidateCacheTemplateCount)
    {
        GiftCandidateCache.clear();
        GiftCandidateCacheTemplateCount = templates->size();
    }

    auto it = GiftCandidateCache.find(targetItemLevel);
    if (it == GiftCandidateCache.end())
        it = GiftCandidateCache.emplace(targetItemLevel, BuildGiftCandidates(targetItemLevel)).first;

    return it->second;
}

// ==================== 数量统计 ====================

// 统计一级分类下的候选数量
size_t CountTopCategory(std::vector<GiftCandidate> const& candidates, uint32 topCategory)
{
    return size_t(std::count_if(candidates.begin(), candidates.end(), [topCategory](GiftCandidate const& candidate)
    {
        return GetTopCategory(candidate) == topCategory;
    }));
}

// 统计指定大类 + 子类的候选数量
size_t CountSubClass(std::vector<GiftCandidate> const& candidates, uint32 itemClass, uint32 subClass)
{
    auto matches = [itemClass, subClass](GiftCandidate const& candidate)
    {
        return candidate.ItemClass == itemClass && candidate.SubClass == subClass;
    };
    return size_t(std::count_if(candidates.begin(), candidates.end(), matches));
}

// 统计“其他”分类下指定部位的候选数量
size_t CountInvType(std::vector<GiftCandidate> const& candidates, uint32 invType)
{
    return size_t(std::count_if(candidates.begin(), candidates.end(), [invType](GiftCandidate const& candidate)
    {
        return GetTopCategory(candidate) == GIFT_TOP_OTHER && candidate.InvType == invType;
    }));
}

// 统计护甲一级分类下指定子类的候选数量
// 必须带上「一级分类为护甲」的条件：披风等部位在 item_template 里也是 class=4、subclass=1(布甲)，
// 但归属“其他”分类，若不带该条件会导致二级菜单数量与三级各部位之和不等
size_t CountArmorSubClass(std::vector<GiftCandidate> const& candidates, uint32 subClass)
{
    return size_t(std::count_if(candidates.begin(), candidates.end(), [subClass](GiftCandidate const& candidate)
    {
        return GetTopCategory(candidate) == GIFT_TOP_ARMOR && candidate.SubClass == subClass;
    }));
}

// 组装带数量的菜单项文本，例如“布甲 (12)”
std::string MakeMenuText(std::string const& name, size_t count)
{
    return Acore::StringFormat("{} ({})", name, count);
}

// ==================== 菜单 ====================

// 一级分类菜单（只列出本礼包允许的分类）
template <size_t N>
void SendTopCategoryMenu(Player* player, Item* item, std::vector<GiftCandidate> const& candidates,
    std::array<uint32, N> const& topCategories)
{
    ClearGossipMenuFor(player);

    for (uint32 topCategory : topCategories)
    {
        size_t count = CountTopCategory(candidates, topCategory);
        if (!count)
            continue;

        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, MakeMenuText(TopCategoryNames[topCategory], count),
            GossipSenderRoot, GossipActionBase + topCategory);
    }

    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "关闭", GossipSenderRoot, GossipActionClose);
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, item->GetGUID());
}

// 二级分类菜单：护甲子类 / 武器子类 / 其他部位
// addBackButton 为 false 时（武器礼包作为唯一一层菜单）不显示「返回上一级」
void SendSubCategoryMenu(Player* player, Item* item, std::vector<GiftCandidate> const& candidates,
    uint32 topCategory, bool addBackButton)
{
    ClearGossipMenuFor(player);

    uint32 sender = GossipSenderSubClassBase + topCategory;
    switch (topCategory)
    {
        case GIFT_TOP_ARMOR:
            for (uint32 subClass : ArmorGiftSubClasses)
            {
                size_t count = CountArmorSubClass(candidates, subClass);
                if (!count)
                    continue;

                AddGossipItemFor(player, GOSSIP_ICON_VENDOR,
                    Acore::StringFormat("{} ({})", GetSubClassName(ITEM_CLASS_ARMOR, subClass), count),
                    sender, GossipActionBase + subClass);
            }
            break;
        case GIFT_TOP_WEAPON:
            for (uint32 subClass : WeaponGiftSubClasses)
            {
                size_t count = CountSubClass(candidates, ITEM_CLASS_WEAPON, subClass);
                if (!count)
                    continue;

                AddGossipItemFor(player, GOSSIP_ICON_VENDOR,
                    Acore::StringFormat("{} ({})", GetSubClassName(ITEM_CLASS_WEAPON, subClass), count),
                    sender, GossipActionBase + subClass);
            }
            break;
        case GIFT_TOP_OTHER:
            for (uint32 invType : OtherGiftInvTypes)
            {
                size_t count = CountInvType(candidates, invType);
                if (!count)
                    continue;

                AddGossipItemFor(player, GOSSIP_ICON_VENDOR,
                    Acore::StringFormat("{} ({})", GetInvTypeName(invType), count),
                    sender, GossipActionBase + invType);
            }
            break;
        default:
            break;
    }

    if (addBackButton)
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回上一级", sender, GossipActionBack);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "关闭", sender, GossipActionClose);
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, item->GetGUID());
}

// 护甲部位菜单（选项后附带可用装备数量）
void SendSlotMenu(Player* player, Item* item, std::vector<GiftCandidate> const& candidates, uint32 subClass)
{
    ClearGossipMenuFor(player);

    std::array<size_t, MAX_INVTYPE> invTypeCount{};
    for (auto const& candidate : candidates)
    {
        // 双重保险：候选池已保证部位下标合法，这里再挡一次，避免数组越界写
        if (candidate.InvType >= MAX_INVTYPE)
            continue;
        if (GetTopCategory(candidate) == GIFT_TOP_ARMOR && candidate.SubClass == subClass)
            ++invTypeCount[candidate.InvType];
    }

    uint32 sender = GossipSenderSlotBase + subClass;
    for (uint32 invType = 0; invType < MAX_INVTYPE; ++invType)
    {
        if (!invTypeCount[invType])
            continue;

        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, MakeMenuText(GetInvTypeName(invType), invTypeCount[invType]),
            sender, GossipActionBase + invType);
    }

    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回上一级", sender, GossipActionBack);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "关闭", sender, GossipActionClose);
    SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, item->GetGUID());
}

// ==================== 发放 ====================

// 随机抽取一件并发放，成功发放后扣除一个礼包物品
void GiveRandomGift(Player* player, Item* item, std::vector<GiftCandidate> const& candidates,
    uint32 topCategory, uint32 subClass, uint32 invType)
{
    std::vector<uint32> entries;
    for (auto const& candidate : candidates)
    {
        if (GetTopCategory(candidate) != topCategory)
            continue;
        if (subClass != GiftAny && candidate.SubClass != subClass)
            continue;
        if (invType != GiftAny && candidate.InvType != invType)
            continue;

        entries.push_back(candidate.Entry);
    }

    if (entries.empty())
    {
        CloseGossipMenuFor(player);
        ChatHandler(player->GetSession()).PSendSysMessage("没有找到符合条件的装备。");
        return;
    }

    uint32 entry = entries[urand(0, uint32(entries.size()) - 1)];
    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(entry);

    // 背包空间不足时 AddItem 会自行提示，此处不扣除礼包，允许玩家重新选择
    if (!player->AddItem(entry, 1))
    {
        CloseGossipMenuFor(player);
        return;
    }

    CloseGossipMenuFor(player);
    player->DestroyItemCount(item->GetEntry(), 1, true);
    if (proto)
        ChatHandler(player->GetSession()).PSendSysMessage("你获得了：{}", proto->Name1);
}

// 从物品描述中解析目标装等，[230] 可出现在描述的任意位置
bool ParseTargetItemLevel(std::string const& description, uint32& itemLevel)
{
    size_t open = description.find('[');
    while (open != std::string::npos)
    {
        size_t close = description.find(']', open + 1);
        if (close == std::string::npos)
            break;

        if (close > open + 1)
        {
            // 方括号内必须全部是数字才算装等标记
            uint32 value = 0;
            bool digitsOnly = true;
            for (size_t i = open + 1; i < close; ++i)
            {
                char c = description[i];
                if (c < '0' || c > '9')
                {
                    digitsOnly = false;
                    break;
                }

                value = value * 10 + uint32(c - '0');
                if (value > 100000) // 防止描述中出现异常数值
                {
                    digitsOnly = false;
                    break;
                }
            }

            if (digitsOnly && value)
            {
                itemLevel = value;
                return true;
            }
        }

        // 该方括号不是装等标记，继续往后找
        open = description.find('[', close + 1);
    }

    return false;
}

// ==================== 菜单前的公共校验 ====================

// 使用礼包时的公共校验；失败时已给出提示，调用方直接阻断本次使用（不消耗礼包）
bool PrepareGiftUse(Player* player, Item* item, std::vector<GiftCandidate>& candidates)
{
    if (!player || !item || !item->GetTemplate())
        return false;

    if (player->IsInCombat() || player->IsInFlight()
        || (player->GetMap() && player->GetMap()->IsBattlegroundOrArena()))
    {
        ChatHandler(player->GetSession()).PSendSysMessage("你现在还不能使用它!");
        return false;
    }

    uint32 targetItemLevel = 0;
    if (!ParseTargetItemLevel(item->GetTemplate()->Description, targetItemLevel))
    {
        ChatHandler(player->GetSession()).PSendSysMessage("物品描述中没有找到目标装等，请写成 [230] 的格式。");
        return false;
    }

    candidates = GetGiftCandidates(targetItemLevel);
    if (candidates.empty())
    {
        ChatHandler(player->GetSession()).PSendSysMessage("没有找到 {} 装等的可装备物品。", targetItemLevel);
        return false;
    }

    return true;
}

// 选择菜单项时的公共准备；失败时关闭菜单并返回 false
bool PrepareGiftSelect(Player* player, Item* item, std::vector<GiftCandidate>& candidates)
{
    uint32 targetItemLevel = 0;
    if (!ParseTargetItemLevel(item->GetTemplate()->Description, targetItemLevel))
    {
        CloseGossipMenuFor(player);
        return false;
    }

    candidates = GetGiftCandidates(targetItemLevel);
    if (candidates.empty())
    {
        CloseGossipMenuFor(player);
        return false;
    }

    return true;
}

// ==================== 物品脚本一：护甲 + 其他 ====================
class RandomEquipGiftArmorOtherItem : public ItemScript
{
public:
    RandomEquipGiftArmorOtherItem() : ItemScript("RandomEquipGiftArmorOtherItem") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const&) override
    {
        std::vector<GiftCandidate> candidates;
        if (!PrepareGiftUse(player, item, candidates))
            return true;

        SendTopCategoryMenu(player, item, candidates, ArmorOtherTopCategories);
        return true;
    }

    void OnGossipSelect(Player* player, Item* item, uint32 sender, uint32 action) override
    {
        if (!player || !item || !item->GetTemplate())
            return;

        if (action == GossipActionClose)
        {
            CloseGossipMenuFor(player);
            return;
        }

        std::vector<GiftCandidate> candidates;
        if (!PrepareGiftSelect(player, item, candidates))
            return;

        // 一级分类菜单（护甲 / 其他）
        if (sender == GossipSenderRoot)
        {
            if (action < GossipActionBase || action >= GossipActionBase + GIFT_TOP_COUNT)
            {
                CloseGossipMenuFor(player);
                return;
            }

            uint32 topCategory = action - GossipActionBase;
            if (!Contains(ArmorOtherTopCategories, topCategory))
            {
                CloseGossipMenuFor(player);
                return;
            }

            SendSubCategoryMenu(player, item, candidates, topCategory, true);
            return;
        }

        // 二级分类菜单（护甲子类 / 其他部位）
        if (sender >= GossipSenderSubClassBase && sender < GossipSenderSlotBase)
        {
            uint32 topCategory = sender - GossipSenderSubClassBase;
            if (!Contains(ArmorOtherTopCategories, topCategory))
            {
                CloseGossipMenuFor(player);
                return;
            }

            if (action == GossipActionBack)
            {
                SendTopCategoryMenu(player, item, candidates, ArmorOtherTopCategories);
                return;
            }

            if (action < GossipActionBase || action >= GossipActionBack)
                return;

            uint32 value = action - GossipActionBase;
            if (topCategory == GIFT_TOP_ARMOR)
            {
                // 护甲：还需再选一层部位
                if (Contains(ArmorGiftSubClasses, value))
                    SendSlotMenu(player, item, candidates, value);
                else
                    CloseGossipMenuFor(player);
            }
            else
            {
                // 其他：二级即部位，选完直接发放
                if (Contains(OtherGiftInvTypes, value))
                    GiveRandomGift(player, item, candidates, GIFT_TOP_OTHER, GiftAny, value);
                else
                    CloseGossipMenuFor(player);
            }
            return;
        }

        // 三级菜单（护甲部位）
        if (sender >= GossipSenderSlotBase)
        {
            uint32 subClass = sender - GossipSenderSlotBase;
            if (!Contains(ArmorGiftSubClasses, subClass))
            {
                CloseGossipMenuFor(player);
                return;
            }

            if (action == GossipActionBack)
            {
                SendSubCategoryMenu(player, item, candidates, GIFT_TOP_ARMOR, true);
                return;
            }

            if (action < GossipActionBase || action >= GossipActionBack)
                return;

            uint32 invType = action - GossipActionBase;
            if (invType >= MAX_INVTYPE)
                return;

            GiveRandomGift(player, item, candidates, GIFT_TOP_ARMOR, subClass, invType);
            return;
        }

        CloseGossipMenuFor(player);
    }
};

// ==================== 物品脚本二：武器 ====================
class RandomEquipGiftWeaponItem : public ItemScript
{
public:
    RandomEquipGiftWeaponItem() : ItemScript("RandomEquipGiftWeaponItem") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const&) override
    {
        std::vector<GiftCandidate> candidates;
        if (!PrepareGiftUse(player, item, candidates))
            return true;

        // 武器礼包没有一级菜单，直接展示二级（武器子类）
        SendSubCategoryMenu(player, item, candidates, GIFT_TOP_WEAPON, false);
        return true;
    }

    void OnGossipSelect(Player* player, Item* item, uint32 sender, uint32 action) override
    {
        if (!player || !item || !item->GetTemplate())
            return;

        if (action == GossipActionClose)
        {
            CloseGossipMenuFor(player);
            return;
        }

        std::vector<GiftCandidate> candidates;
        if (!PrepareGiftSelect(player, item, candidates))
            return;

        // 唯一一层菜单：选中武器子类即直接发放
        if (sender == GossipSenderWeaponMenu)
        {
            if (action < GossipActionBase || action >= GossipActionBack)
                return;

            uint32 subClass = action - GossipActionBase;
            if (Contains(WeaponGiftSubClasses, subClass))
                GiveRandomGift(player, item, candidates, GIFT_TOP_WEAPON, subClass, GiftAny);
            else
                CloseGossipMenuFor(player);
            return;
        }

        CloseGossipMenuFor(player);
    }
};
}

void AddSC_random_equip_gift()
{
    new RandomEquipGiftArmorOtherItem();
    new RandomEquipGiftWeaponItem();
}
