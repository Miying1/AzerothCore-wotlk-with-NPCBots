/*
 * 随机属性萃取与转移 —— mod-random-enchants 模块
 *
 * 说明：本功能与模块的「泰坦附魔」无关，是独立的随机属性转移功能。
 *   直接读写装备自带的随机属性（ITEM_FIELD_RANDOM_PROPERTIES_ID，即
 *   RandomProperty / RandomSuffix），不依赖 tnEchants / mod_item_enchantment_result。
 *
 * 功能：
 *   1. 属性萃取仪（60103，非消耗）：右键使用，选择一件「有随机属性」的装备，
 *      将其随机属性萃取为「属性卷轴」，并销毁原装备。
 *   2. 属性卷轴（60104，消耗）：右键使用，选择一件装备（不限是否已有随机属性，
 *      已有随机属性会被直接覆盖），将卷轴上的随机属性注入目标装备，并消耗卷轴。
 *
 * 菜单：
 *   - 装备名按品质着色并用 [] 括起（ItemQualityColors），菜单项统一用聊天气泡图标（GOSSIP_ICON_CHAT）；
 *     装备的图标用文本转义 |T 内联在名字前面（gossip 菜单项只支持固定图标，取不到物品图标）；
 *     确认框文案只用名字不带图标（弹框字体串未必解析 |T 转义）；
 *   - 属性卷轴为两级菜单：一级按装备部位分类（见 SlotCategory，只列当前背包里确有装备的部位，
 *     并附上该部位的装备数量），二级列出该部位的装备；
 *   - 萃取仪只有一级菜单：不分部位，直接列出全部「有随机属性」的装备；
 *   - 装备列表分页显示，每页 14 条，超出时给出「上一页 / 下一页」（页码带在按钮与 action 里）；
 *     卷轴二级菜单另有「返回部位分类」回到一级菜单；
 *   - 每级菜单末尾固定给出「关闭」项：既是手动退出入口，也保证任意一页至少有 2 项 ——
 *     客户端对「只有 1 项且图标不是聊天气泡」的菜单会自动选中唯一项并跳过窗口显示
 *     （FrameXML GossipFrame_OnEvent 的 GOSSIP_SHOW 分支），空列表只剩提示项时会变成死循环；
 *   - 选中装备时客户端弹出确认框，玩家确认后才会真正执行萃取 / 注入。
 *
 * 装等过滤（优先按装等）：萃取仪与属性卷轴共用同一门槛，只有
 *   ItemLevel > RandomEnchants.Custom.MinItemLevel（默认 220）的装备才会出现在菜单里，
 *   阈值与随机附魔（item_random_enchant.cpp）口径一致 —— 萃取仪只能取出本模块随机附魔
 *   覆盖到的那批装备，低装等的原生随机属性装备既不参与萃取、也不能作为注入目标。
 *
 * 可操作的装备部位：与随机附魔（item_random_enchant.cpp）一致，覆盖全部装备槽
 *   （头/颈/肩/背/胸/袍/腰/腿/脚/腕/手/戒指/饰品/主手/副手/双手/远程/投掷/盾/
 *   副手物品/圣物），见 RandomEnchantCommon.h 的 IsEquipmentSlot()。
 *
 * 可选范围：玩家主背包（16 格）与已装备的包内物品，不含已装备的装备。
 *
 * 持久化：卷轴的 randomPropertyId 由 item_instance 表保存，可跨登录长久保留。
 */

#include "Chat.h"
#include "Common.h"
#include "Configuration/Config.h"
#include "DBCStores.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "RandomEnchantCommon.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "SharedDefines.h"
#include "StringFormat.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

// GOSSIP_SENDER_MAIN 由 ScriptedGossip.h 提供（= 1），此处不再重复定义
enum
{
    ITEM_EXTRACTOR = 60103,   // 属性萃取仪（非消耗）
    ITEM_SCROLL    = 60104,   // 属性卷轴（消耗，携带随机属性）

    ITEMS_PER_PAGE = 14,      // 每页菜单条数

    // 菜单项 sender：装备项 / 翻页项 / 部位分类项 / 返回项 / 关闭项互相分开，避免动作与装备 GUID 撞车。
    // 装备项与翻页项的 sender 再各自带上部位分类编号（BASE + 部位分类，见 SlotCategory），
    // 这样二级菜单翻页时才知道自己列的是哪个部位。
    SENDER_ITEM_LIST_BASE = GOSSIP_SENDER_MAIN + 100,  // 「+ 部位」：action = 装备 guid 低 32 位
    SENDER_PAGE_NAV_BASE  = GOSSIP_SENDER_MAIN + 200,  // 「+ 部位」：action = 目标页码（从 1 起）
    SENDER_SLOT_MENU      = GOSSIP_SENDER_MAIN + 300,  // 一级部位分类菜单：action = 部位分类编号
    SENDER_BACK           = GOSSIP_SENDER_MAIN + 400,  // 「返回部位分类」：回到一级菜单
    SENDER_CLOSE          = GOSSIP_SENDER_MAIN + 500,  // 「关闭」：走 OnGossipSelect 兜底分支关闭对话
};

namespace
{
// 装备部位分类：把 InventoryType 归并成玩家视角的部位
//（长袍并入胸部；单手/双手/主手/副手武器并入武器；远程/远程(右)/投掷并入远程/投掷）
enum SlotCategory : uint32
{
    SLOT_HEAD = 0,      // 头部
    SLOT_NECK,          // 颈部
    SLOT_SHOULDERS,     // 肩部
    SLOT_CLOAK,         // 背部（披风）
    SLOT_CHEST,         // 胸部（含长袍）
    SLOT_WAIST,         // 腰部
    SLOT_LEGS,          // 腿部
    SLOT_FEET,          // 脚部
    SLOT_WRISTS,        // 手腕
    SLOT_HANDS,         // 手部
    SLOT_FINGER,        // 手指
    SLOT_TRINKET,       // 饰品
    SLOT_WEAPON,        // 武器（单手 / 双手 / 主手 / 副手）
    SLOT_SHIELD,        // 盾牌
    SLOT_HOLDABLE,      // 副手物品
    SLOT_RELIC,         // 圣物
    SLOT_RANGED,        // 远程 / 投掷

    SLOT_COUNT,              // 部位分类总数（本身不是有效分类）
    SLOT_ALL = SLOT_COUNT,   // 不分部位（萃取仪一次列出全部装备）
};

// 部位分类中文名（下标与上面的枚举一一对应）
constexpr std::array<char const*, SLOT_COUNT> SlotNames =
{
    "头部", "颈部", "肩部", "背部", "胸部", "腰部", "腿部", "脚部", "手腕", "手部",
    "手指", "饰品", "武器", "盾牌", "副手物品", "圣物", "远程/投掷"
};

// InventoryType → 部位分类；非装备部位返回 SLOT_COUNT
uint32 GetSlotCategory(uint32 invType)
{
    switch (invType)
    {
        case INVTYPE_HEAD:           return SLOT_HEAD;
        case INVTYPE_NECK:           return SLOT_NECK;
        case INVTYPE_SHOULDERS:      return SLOT_SHOULDERS;
        case INVTYPE_CLOAK:          return SLOT_CLOAK;
        // 长袍是胸甲的另一种版型，归到同一个部位
        case INVTYPE_CHEST:
        case INVTYPE_ROBE:           return SLOT_CHEST;
        case INVTYPE_WAIST:          return SLOT_WAIST;
        case INVTYPE_LEGS:           return SLOT_LEGS;
        case INVTYPE_FEET:           return SLOT_FEET;
        case INVTYPE_WRISTS:         return SLOT_WRISTS;
        case INVTYPE_HANDS:          return SLOT_HANDS;
        case INVTYPE_FINGER:         return SLOT_FINGER;
        case INVTYPE_TRINKET:        return SLOT_TRINKET;
        // 单手 / 双手 / 主手 / 副手武器归到同一个部位
        case INVTYPE_WEAPON:
        case INVTYPE_2HWEAPON:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_WEAPONOFFHAND:  return SLOT_WEAPON;
        case INVTYPE_SHIELD:         return SLOT_SHIELD;
        case INVTYPE_HOLDABLE:       return SLOT_HOLDABLE;
        case INVTYPE_RELIC:          return SLOT_RELIC;
        // 远程 / 远程(右) / 投掷归到同一个部位
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_THROWN:         return SLOT_RANGED;
        default:                     return SLOT_COUNT;
    }
}
} // namespace

class EnchantTransferItem : public ItemScript
{
public:
    EnchantTransferItem() : ItemScript("EnchantTransferItem") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        uint32 entry = item->GetEntry();
        if (entry != ITEM_EXTRACTOR && entry != ITEM_SCROLL)
            return false;

        if (entry == ITEM_SCROLL)
            ListSlots(player, item);                 // 属性卷轴：一级菜单按装备部位分类
        else
            ListItems(player, item, true, SLOT_ALL, 1); // 萃取仪：不分部位，直接列有随机属性的装备

        return true;
    }

    void OnGossipSelect(Player* player, Item* item, uint32 sender, uint32 action) override
    {
        uint32 entry = item->GetEntry();
        if (entry != ITEM_EXTRACTOR && entry != ITEM_SCROLL)
            return;

        bool const forExtraction = (entry == ITEM_EXTRACTOR);

        // 一级菜单（属性卷轴）：选中某个部位 → 列出该部位的装备
        if (sender == SENDER_SLOT_MENU)
        {
            ListItems(player, item, forExtraction, action, 1);
            return;
        }

        // 「返回部位分类」：回到一级菜单（属性卷轴）
        if (sender == SENDER_BACK)
        {
            ListSlots(player, item);
            return;
        }

        // 翻页：sender 携带部位分类，action 里存的是目标页码
        if (sender >= SENDER_PAGE_NAV_BASE && sender <= SENDER_PAGE_NAV_BASE + SLOT_ALL)
        {
            ListItems(player, item, forExtraction, sender - SENDER_PAGE_NAV_BASE, action);
            return;
        }

        // 选择装备：sender 携带部位分类，action 存的是目标装备的 guid 低 32 位；
        // 确认框已由客户端弹出，玩家确认后才会走到这里
        if (sender >= SENDER_ITEM_LIST_BASE && sender <= SENDER_ITEM_LIST_BASE + SLOT_ALL)
        {
            Item* target = player->GetItemByGuid(ObjectGuid::Create<HighGuid::Item>(action));
            if (!target)
            {
                CloseGossipMenuFor(player);
                return;
            }

            if (forExtraction)
                DoExtract(player, target);
            else
                DoInject(player, item, target);
            return;
        }

        // 「关闭」项（SENDER_CLOSE）或未知 sender：直接关闭对话
        CloseGossipMenuFor(player);
    }

private:
    // 一条菜单项：装备 guid 低 32 位（action）+ 所属部位分类 + 图标文本转义 + 已按品质着色的装备名
    // （图标与名字分开存：菜单项显示「图标 + 名字」，确认框只显示名字，避免弹框不解析 |T 转义）
    struct MenuEntry
    {
        uint32      guid;
        uint32      slot;
        std::string icon;
        std::string name;
    };

    // 萃取仪与属性卷轴共用的装等门槛：与随机附魔（RandomEnchants.Custom.MinItemLevel）同一口径，
    // 保证「能被萃取」与「能被注入」的装备就是「本模块随机附魔覆盖」的那批装备
    static uint32 GetMinItemLevel()
    {
        return sConfigMgr->GetOption<uint32>("RandomEnchants.Custom.MinItemLevel", 220);
    }

    void DoExtract(Player* player, Item* target)
    {
        // 优先按装等过滤：菜单已过滤，这里再校验一次，防止伪造 action 绕过
        if (target->GetTemplate()->ItemLevel <= GetMinItemLevel())
        {
            CloseGossipMenuFor(player);
            ChatHandler(player->GetSession()).PSendSysMessage("该装备装等不高于 {}，无法萃取随机属性。", GetMinItemLevel());
            return;
        }

        int32 rp = target->GetItemRandomPropertyId();
        if (!rp)
        {
            CloseGossipMenuFor(player);
            return;
        }

        ItemPosCountVec dest;
        if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, ITEM_SCROLL, 1) != EQUIP_ERR_OK)
        {
            CloseGossipMenuFor(player);
            ChatHandler(player->GetSession()).PSendSysMessage("背包已满，无法生成卷轴。");
            return;
        }

        // 销毁源装备，生成携带随机属性的卷轴（randomPropertyId 会持久化入库）
        player->DestroyItem(target->GetBagSlot(), target->GetSlot(), true);
        player->StoreNewItem(dest, ITEM_SCROLL, true, rp);

        CloseGossipMenuFor(player);
        ChatHandler(player->GetSession()).PSendSysMessage("属性词条已提取为卷轴，原装备被销毁。");
    }

    void DoInject(Player* player, Item* scroll, Item* target)
    {
        // 与萃取仪同一装等门槛：菜单已过滤，这里再校验一次，防止伪造 action 绕过
        if (target->GetTemplate()->ItemLevel <= GetMinItemLevel())
        {
            CloseGossipMenuFor(player);
            ChatHandler(player->GetSession()).PSendSysMessage("该装备装等不高于 {}，无法注入随机属性。", GetMinItemLevel());
            return;
        }

        int32 rp = scroll->GetItemRandomPropertyId();
        if (!rp)
        {
            CloseGossipMenuFor(player);
            return;
        }

        // 把卷轴随机属性写入目标装备（SetItemRandomProperties 内部会写附魔槽并标记变更，
        // 目标装备若已有随机属性会被直接覆盖）
        target->SetItemRandomProperties(rp);

        // 消耗卷轴
        player->DestroyItem(scroll->GetBagSlot(), scroll->GetSlot(), true);

        CloseGossipMenuFor(player);
        ChatHandler(player->GetSession()).PSendSysMessage("属性词条已注入目标装备。");
    }

    // 物品图标：gossip 菜单项本身只支持固定的几个对话图标，没法直接放物品图标，
    // 所以用文本转义 |T 把图标内联进菜单文字里（与 ReEnchantsNpc.cpp 的写法一致），
    // 图标名取自 ItemDisplayInfo.dbc 的 inventoryIcon（如 INV_Sword_04）
    static std::string ItemIcon(ItemTemplate const* proto)
    {
        ItemDisplayInfoEntry const* display = sItemDisplayInfoStore.LookupEntry(proto->DisplayInfoID);
        if (!display || !display->inventoryIcon || !*display->inventoryIcon)
            return "";

        return Acore::StringFormat("|TInterface\\Icons\\{}:16|t", display->inventoryIcon);
    }

    // 按物品品质着色（|cAARRGGBB…|r），名字用 [] 括起，用于菜单显示
    static std::string ColoredItemName(ItemTemplate const* proto)
    {
        uint32 quality = proto->Quality < MAX_ITEM_QUALITY ? proto->Quality : uint32(ITEM_QUALITY_POOR);
        return Acore::StringFormat("|c{:08x}[{}]|r", ItemQualityColors[quality], proto->Name1);
    }

    // forExtraction=true 收集「有随机属性」的装备（萃取仪）；
    // false 收集可注入的装备（卷轴，不按「是否已有随机属性」过滤，允许覆盖）；
    // slot=SLOT_ALL 表示不分部位，否则只收集该部位的装备；两条路径都先按装等过滤
    static void CollectItems(Player* player, bool forExtraction, uint32 slot, std::vector<MenuEntry>& out)
    {
        // 萃取仪与卷轴共用装等门槛，只读一次配置
        uint32 const minItemLevel = GetMinItemLevel();

        auto addItem = [&](Item* pItem)
        {
            if (!pItem)
                return;

            ItemTemplate const* proto = pItem->GetTemplate();

            // 优先按装等过滤：装等不高于阈值的装备既不参与萃取、也不能作为注入目标
            if (proto->ItemLevel <= minItemLevel)
                return;

            // 部位过滤：SLOT_ALL 时用公共判定（与随机附魔 item_random_enchant.cpp 一致），
            // 指定部位时按归并后的部位分类匹配
            uint32 const category = GetSlotCategory(proto->InventoryType);
            if (slot == SLOT_ALL)
            {
                if (!IsEquipmentSlot(proto->InventoryType))
                    return;
            }
            else if (category != slot)
                return;

            // 萃取仪只列「已有随机属性」的装备；卷轴侧不再过滤已有随机属性的装备，
            // 已有随机属性的装备同样出现在注入列表里，确认后由 SetItemRandomProperties 直接覆盖
            if (forExtraction && pItem->GetItemRandomPropertyId() == 0)
                return;

            out.push_back({ pItem->GetGUID().GetCounter(), category, ItemIcon(proto), ColoredItemName(proto) });
        };

        // 扫描玩家主背包（16 格）
        for (uint8 invSlot = INVENTORY_SLOT_ITEM_START; invSlot != INVENTORY_SLOT_ITEM_END; ++invSlot)
            addItem(player->GetItemByPos(INVENTORY_SLOT_BAG_0, invSlot));

        // 扫描已装备的包内物品
        for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot != INVENTORY_SLOT_BAG_END; ++bagSlot)
            if (Bag* bag = player->GetBagByPos(bagSlot))
                for (uint32 bagItemSlot = 0; bagItemSlot != bag->GetBagSize(); ++bagItemSlot)
                    addItem(player->GetItemByPos(bagSlot, bagItemSlot));
    }

    // 选中装备时客户端弹出的确认框文本
    static std::string ConfirmText(uint32 sourceEntry, std::string const& itemName)
    {
        if (sourceEntry == ITEM_EXTRACTOR)
            return Acore::StringFormat("确定萃取 {} 的随机属性吗？原装备将被销毁。", itemName);

        return Acore::StringFormat("确定把卷轴上的随机属性注入 {} 吗？卷轴将被消耗。", itemName);
    }

    // 属性卷轴一级菜单：按装备部位分类，只列当前背包里确有装备的部位（附该部位装备数量）
    void ListSlots(Player* player, Item* sourceItem) const
    {
        player->PlayerTalkClass->ClearMenus();

        std::vector<MenuEntry> entries;
        CollectItems(player, false, SLOT_ALL, entries);

        std::array<uint32, SLOT_COUNT> counts{};
        for (MenuEntry const& e : entries)
            if (e.slot < SLOT_COUNT)
                ++counts[e.slot];

        uint32 categories = 0;
        for (uint32 slot = 0; slot < SLOT_COUNT; ++slot)
        {
            if (!counts[slot])
                continue;

            AddGossipItemFor(player, GOSSIP_ICON_CHAT, Acore::StringFormat("{} ({})", SlotNames[slot], counts[slot]),
                             SENDER_SLOT_MENU, slot);
            ++categories;
        }

        // 一件可用装备都没有时给提示项（该项与下面的「关闭」都能退出对话）
        if (!categories)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                             Acore::StringFormat("（背包里没有装等高于 {} 的可注入装备）", GetMinItemLevel()),
                             SENDER_CLOSE, 0);

        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "关闭", SENDER_CLOSE, 0);
        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, sourceItem->GetGUID());
    }

    // 装备列表（分页，每页 ITEMS_PER_PAGE 条）；slot=SLOT_ALL 表示不分部位（萃取仪）
    void ListItems(Player* player, Item* sourceItem, bool forExtraction, uint32 slot, uint32 page) const
    {
        player->PlayerTalkClass->ClearMenus();

        std::vector<MenuEntry> entries;
        CollectItems(player, forExtraction, slot, entries);

        uint32 const total = uint32(entries.size());
        uint32 pages = (total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
        if (!pages)
            pages = 1;

        // 页码越界时夹到有效范围（客户端回传的 action 不可信）
        page = std::min(std::max(page, uint32(1)), pages);

        // 装备项与翻页项都带上部位分类，翻页时才不会串到别的部位去
        uint32 const senderItem = SENDER_ITEM_LIST_BASE + slot;
        uint32 const senderNav  = SENDER_PAGE_NAV_BASE + slot;

        if (!total)
        {
            // 空列表：萃取仪在背包里没有目标时出现；卷轴二级菜单只在背包中途变化（或伪造 action）时出现
            AddGossipItemFor(player, GOSSIP_ICON_CHAT,
                             forExtraction
                                 ? Acore::StringFormat("（背包里没有装等高于 {} 且带随机属性的装备）", GetMinItemLevel())
                                 : Acore::StringFormat("（{}没有装等高于 {} 的可用装备）",
                                                       slot < SLOT_COUNT ? SlotNames[slot] : "背包里", GetMinItemLevel()),
                             senderNav, page);
        }
        else
        {
            uint32 const first = (page - 1) * ITEMS_PER_PAGE;
            uint32 const last = std::min(first + ITEMS_PER_PAGE, total);

            for (uint32 i = first; i < last; ++i)
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, entries[i].icon + entries[i].name, senderItem, entries[i].guid,
                                 ConfirmText(sourceItem->GetEntry(), entries[i].name), 0, false);
        }

        // 翻页（页码同时写在按钮文字与 action 里）
        if (page > 1)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, Acore::StringFormat("上一页（{}/{}）", page - 1, pages),
                             senderNav, page - 1);
        if (page < pages)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, Acore::StringFormat("下一页（{}/{}）", page + 1, pages),
                             senderNav, page + 1);

        // 属性卷轴是两级菜单，给出「返回部位分类」；萃取仪只有一级菜单，不给
        if (!forExtraction)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回部位分类", SENDER_BACK, 0);

        // 末尾固定补「关闭」项：既是手动退出入口，也保证任意一页至少有 2 项 ——
        // 客户端对「只有 1 项且图标不是聊天气泡」的菜单会自动选中唯一项并跳过窗口显示
        // （FrameXML GossipFrame_OnEvent 的 GOSSIP_SHOW 分支）；当前用的是聊天气泡图标不会触发，
        // 保留该项可在图标改动后继续兜底。
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "关闭", SENDER_CLOSE, 0);

        SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, sourceItem->GetGUID());
    }
};

void AddEnchantTransferScripts()
{
    new EnchantTransferItem();
}
