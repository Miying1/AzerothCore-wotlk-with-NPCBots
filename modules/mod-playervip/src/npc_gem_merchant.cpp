/*
 * 宝石商人（生物 101010）对话脚本 —— mod-playervip 模块
 *
 * 功能：
 *   1. 宝石商店：直接打开 NPC 售卖窗口（商品见 npc_vendor 表）
 *   2. 剥离源力宝石：消耗金币，把玩家背包中物品上镶嵌的「源力宝石」（entry 63102~63110，
 *      多彩宝石）剥离下来返还到背包，物品上其他插槽的宝石不受影响
 *
 * 菜单结构：
 *   主菜单：宝石商店 / 剥离源力宝石
 *   剥离子菜单：正文（问候语）提示后，列出玩家背包中所有已镶嵌源力宝石的装备；
 *      点击装备弹出收取金币的确认框（1000 金币），确认后执行剥离
 *
 * 原理：
 *   - 宝石在服务端以「附魔」（SpellItemEnchantment）形式存储于物品的宝石插槽：
 *     SOCK_ENCHANTMENT_SLOT / _2 / _3（原生三插槽）与 PRISMATIC_ENCHANTMENT_SLOT（额外棱彩插槽，
 *     如腰带打孔）；BONUS_ENCHANTMENT_SLOT 是插槽奖励，不是宝石，需跳过。
 *   - 源力宝石是多彩宝石（meta gem），只会镶嵌在多彩插槽（SocketColor == SOCKET_COLOR_META）中；
 *     通过附魔的 GemID 字段反查宝石物品 entry，仅剥离 entry 在 63102~63110 之间、且位于
 *     多彩插槽中的宝石，其余插槽的宝石保持不动。
 *   - 剥离范围仅限玩家背包（主背包物品 + 身上背包），不涉及已装备栏、银行/公会银行。
 *   - 物品在背包内未装备，宝石附魔属性未生效，剥离时直接 ClearEnchantment 清空插槽即可。
 *
 * 配套 SQL：data/宝石商人_101010.sql（需在 acore_world 库执行）
 */

#include "Bag.h"
#include "Chat.h"
#include "DBCStores.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "SharedDefines.h"
#include "StringFormat.h"
#include "WorldSession.h"

#include <vector>

namespace
{
constexpr uint32 NPC_GEM_MERCHANT_ENTRY = 101010;   // 宝石商人生物入口

// 每次剥离源力宝石的费用（铜）：1000 金币
constexpr uint32 GEM_STRIP_COST = 1000 * GOLD;

// 源力宝石 item entry 范围（含两端）
constexpr uint32 GEM_SOURCE_MIN = 63102;
constexpr uint32 GEM_SOURCE_MAX = 63110;

// 剥离子菜单的 sender，用于在 OnGossipSelect 中与主菜单区分
constexpr uint32 SENDER_STRIP_MENU = 100;

// 动作 ID
constexpr uint32 ACTION_SHOP = GOSSIP_ACTION_INFO_DEF + 1;           // 宝石商店：打开售卖窗口
constexpr uint32 ACTION_OPEN_STRIP_MENU = GOSSIP_ACTION_INFO_DEF + 2; // 剥离源力宝石：打开子菜单
constexpr uint32 ACTION_STRIP_ITEM_BASE = GOSSIP_ACTION_INFO_DEF + 10; // 选择装备（低位编码 bag+slot，见 EncodeItemAction）
constexpr uint32 ACTION_CLOSE = GOSSIP_ACTION_INFO_DEF + 400;         // 关闭（避开装备动作编码区间 [DEF+10, DEF+330)）
constexpr uint32 ACTION_BACK = GOSSIP_ACTION_INFO_DEF + 401;          // 返回上一级

// 主菜单 / 子菜单正文（npc_text 表，由 data/宝石商人_101010.sql 维护）
constexpr uint32 TEXT_ID_MAIN = 101030;       // 主菜单正文
constexpr uint32 TEXT_ID_STRIP_MENU = 101031; // 剥离子菜单正文（问候语）

// 判断宝石 entry 是否属于源力宝石（63102~63110）
bool IsSourceGem(uint32 gemEntry)
{
    return gemEntry >= GEM_SOURCE_MIN && gemEntry <= GEM_SOURCE_MAX;
}

// 取指定插槽中宝石的物品 entry，失败返回 false
bool GetGemEntryFromSlot(Item* item, EnchantmentSlot slot, uint32& gemEntry)
{
    gemEntry = 0;
    uint32 enchantId = item->GetEnchantmentId(slot);
    if (!enchantId)
        return false;

    SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(enchantId);
    if (!enchant || !enchant->GemID)
        return false;

    gemEntry = enchant->GemID;
    return true;
}

// 收集物品上所有已镶嵌「源力宝石」的附魔插槽编号
// 源力宝石是多彩宝石（meta gem），只会镶嵌在多彩插槽（SocketColor == SOCKET_COLOR_META）中，
// 因此只遍历原生三插槽（2/3/4）并校验插槽颜色；额外棱彩插槽（6）与插槽奖励（5）均不涉及
void CollectSourceGemSlots(Item* item, std::vector<EnchantmentSlot>& slots)
{
    slots.clear();
    ItemTemplate const* proto = item->GetTemplate();

    for (uint32 e = SOCK_ENCHANTMENT_SLOT; e < SOCK_ENCHANTMENT_SLOT + MAX_GEM_SOCKETS; ++e)
    {
        // 仅处理多彩插槽
        if (proto->Socket[e - SOCK_ENCHANTMENT_SLOT].Color != SOCKET_COLOR_META)
            continue;

        uint32 gemEntry = 0;
        if (GetGemEntryFromSlot(item, EnchantmentSlot(e), gemEntry) && IsSourceGem(gemEntry))
            slots.push_back(EnchantmentSlot(e));
    }
}

// 将物品位置 (bag, slot) 编码进 gossip 动作低位，便于 OnGossipSelect 反查
// bagIndex：0 = 主背包物品栏，1~4 = 身上 4 个背包；slot 均小于 64
uint32 EncodeItemAction(uint8 bag, uint8 slot)
{
    uint32 bagIndex = (bag == INVENTORY_SLOT_BAG_0) ? 0 : (bag - INVENTORY_SLOT_BAG_START + 1);
    return ACTION_STRIP_ITEM_BASE + (bagIndex * 64) + slot;
}

// 从 gossip 动作反解物品位置
void DecodeItemAction(uint32 action, uint8& bag, uint8& slot)
{
    uint32 offset = action - ACTION_STRIP_ITEM_BASE;
    uint32 bagIndex = offset / 64;
    slot = uint8(offset % 64);
    bag = uint8((bagIndex == 0) ? INVENTORY_SLOT_BAG_0 : (INVENTORY_SLOT_BAG_START + bagIndex - 1));
}

// 主菜单：宝石商店 / 剥离源力宝石
void SendMainMenu(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);
    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "宝石商店", GOSSIP_SENDER_MAIN, ACTION_SHOP);
    AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG, "剥离源力宝石", GOSSIP_SENDER_MAIN, ACTION_OPEN_STRIP_MENU);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "关闭", GOSSIP_SENDER_MAIN, ACTION_CLOSE);
    SendGossipMenuFor(player, TEXT_ID_MAIN, creature->GetGUID());
}

// 剥离子菜单：列出玩家背包（主背包物品 + 身上背包，不含已装备栏）中所有已镶嵌源力宝石的装备
void SendStripMenu(Player* player, Creature* creature)
{
    ClearGossipMenuFor(player);

    bool hasAny = false;

    // 遍历指定容器（bag）内 [from, to) 的物品，命中源力宝石则加入菜单
    auto addItemsInContainer = [&](uint8 bag, uint8 from, uint8 to)
    {
        for (uint8 slot = from; slot < to; ++slot)
        {
            Item* item = player->GetItemByPos(bag, slot);
            if (!item)
                continue;

            std::vector<EnchantmentSlot> gemSlots;
            CollectSourceGemSlots(item, gemSlots);
            if (gemSlots.empty())
                continue;

            hasAny = true;
            // 带确认框（boxMoney）的菜单项：客户端会弹出「是否花费 1000 金币」的确认框，
            // 玩家确认后才会以相同的 sender/action 回调 OnGossipSelect 执行剥离
            // 装备名按物品品质着色（ItemQualityColors，SharedDefines.h）
            AddGossipItemFor(player, GOSSIP_ICON_MONEY_BAG,
                Acore::StringFormat("|c{:08x}[{}]|r（{} 颗）", ItemQualityColors[item->GetTemplate()->Quality], item->GetTemplate()->Name1, gemSlots.size()),
                SENDER_STRIP_MENU, EncodeItemAction(bag, slot),
                Acore::StringFormat("确定要花费 {} 金币剥离该装备上的源力宝石吗？", GEM_STRIP_COST / GOLD),
                GEM_STRIP_COST, false);
        }
    };

    // 主背包物品栏
    addItemsInContainer(INVENTORY_SLOT_BAG_0, INVENTORY_SLOT_ITEM_START, INVENTORY_SLOT_ITEM_END);
    // 身上 4 个背包
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
        if (Bag* bagPtr = player->GetBagByPos(bag))
            addItemsInContainer(bag, 0, uint8(bagPtr->GetBagSize()));

    if (!hasAny)
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "（背包中没有镶嵌源力宝石的装备）", SENDER_STRIP_MENU, ACTION_CLOSE);

    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "返回", SENDER_STRIP_MENU, ACTION_BACK);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, "关闭", SENDER_STRIP_MENU, ACTION_CLOSE);
    SendGossipMenuFor(player, TEXT_ID_STRIP_MENU, creature->GetGUID());
}

// 执行剥离：直接清空该背包物品上的源力宝石插槽，剥离的宝石返还背包
void DoStripSourceGems(Player* player, uint8 bag, uint8 slot)
{
    Item* item = player->GetItemByPos(bag, slot);
    if (!item || item->IsEquipped())
        return; // 仅处理背包内未装备的物品，防止伪造 action 误清已穿戴装备

    std::vector<EnchantmentSlot> gemSlots;
    CollectSourceGemSlots(item, gemSlots);
    if (gemSlots.empty())
    {
        ChatHandler(player->GetSession()).PSendSysMessage("该装备上没有可剥离的源力宝石。");
        return;
    }

    // 收集要返还的源力宝石 entry（保持顺序，用于背包校验与返还）
    std::vector<uint32> gemEntries;
    for (EnchantmentSlot s : gemSlots)
    {
        uint32 gemEntry = 0;
        if (GetGemEntryFromSlot(item, s, gemEntry))
            gemEntries.push_back(gemEntry);
    }
    if (gemEntries.empty())
        return;

    // 金币校验
    if (!player->HasEnoughMoney(GEM_STRIP_COST))
    {
        ChatHandler(player->GetSession()).PSendSysMessage("金币不足，剥离源力宝石需要 {} 金币。", GEM_STRIP_COST / GOLD);
        return;
    }

    // 背包空间校验（每颗返还宝石占一个位置，逐个校验）
    for (uint32 gemEntry : gemEntries)
    {
        ItemPosCountVec dest;
        if (player->CanStoreNewItem(INVENTORY_SLOT_BAG_0, NULL_SLOT, dest, gemEntry, 1) != EQUIP_ERR_OK)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("背包空间不足，无法剥离源力宝石。");
            return;
        }
    }

    // 物品在背包内未装备，宝石附魔属性未生效，直接清空源力宝石所在插槽即可
    for (EnchantmentSlot s : gemSlots)
        item->ClearEnchantment(s);

    // 扣费并返还源力宝石
    player->ModifyMoney(-int32(GEM_STRIP_COST));
    for (uint32 gemEntry : gemEntries)
        player->AddItem(gemEntry, 1);

    ChatHandler(player->GetSession()).PSendSysMessage("你从 [{}] 上剥离了 {} 颗源力宝石。",
        item->GetTemplate()->Name1, gemEntries.size());
}
}

class NpcGemMerchant : public CreatureScript
{
public:
    NpcGemMerchant() : CreatureScript("npc_gem_merchant") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->GetEntry() != NPC_GEM_MERCHANT_ENTRY)
            return false;

        SendMainMenu(player, creature);
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        // 主菜单
        if (sender == GOSSIP_SENDER_MAIN)
        {
            switch (action)
            {
            case ACTION_SHOP: // 宝石商店：打开 NPC 售卖窗口
                CloseGossipMenuFor(player);
                player->GetSession()->SendListInventory(creature->GetGUID());
                return true;
            case ACTION_OPEN_STRIP_MENU: // 剥离源力宝石：进入子菜单
                SendStripMenu(player, creature);
                return true;
            case ACTION_CLOSE:
            default:
                CloseGossipMenuFor(player);
                return true;
            }
        }

        // 剥离子菜单
        if (sender == SENDER_STRIP_MENU)
        {
            if (action == ACTION_BACK)
            {
                SendMainMenu(player, creature);
                return true;
            }
            if (action == ACTION_CLOSE)
            {
                CloseGossipMenuFor(player);
                return true;
            }

            // 选择装备（确认框已由客户端弹出，玩家确认后才走到这里）
            if (action >= ACTION_STRIP_ITEM_BASE && action < ACTION_STRIP_ITEM_BASE + 5 * 64)
            {
                uint8 bag;
                uint8 slot;
                DecodeItemAction(action, bag, slot);
                DoStripSourceGems(player, bag, slot);
                // 剥离后刷新子菜单，避免继续展示已无源力宝石的装备
                SendStripMenu(player, creature);
                return true;
            }

            CloseGossipMenuFor(player);
            return true;
        }

        CloseGossipMenuFor(player);
        return true;
    }
};

void AddNpcGemMerchantScripts()
{
    new NpcGemMerchant();
}
