/*
 * 白虎（生物 101000）对话脚本
 * 功能：
 *   1. 欢迎语（窗口正文）：多条问候文本随机显示（npc_text 表 ID 101000~101023，由 data/小宠物生物_101000_白虎.sql 维护）
 *   2. 宝物商店：选择后打开售卖窗口
 *   3. 航班：选择后打开飞行点地图
 *   4. 我的金币倍率：选择后由 NPC 悄悄话告知玩家真实倍率（100% + VIP 金币加成）
 *   5. 幻化：选择后打开 Lua 幻化界面
 * 配套 SQL：data/小宠物生物_101000_白虎.sql（需在 acore_world 库执行）
 */

#include "Chat.h"
#include "Creature.h"
#include "DBCStores.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "SharedDefines.h"
#include "StringFormat.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <array>
#include <unordered_map>

namespace
{
constexpr uint32 NPC_BAIHU_ENTRY = 101000; // 白虎生物入口

// ===== 对话文本（硬编码于脚本头部）=====  
constexpr char const* TXT_SHOP = "宝物商店";                     // 宝物商店选项
constexpr char const* TXT_FLIGHT = "航班";                       // 航班选项
constexpr char const* TXT_GOLD_RATE = "我的金币倍率";             // 悄悄话告知选项
constexpr char const* TXT_TRANSMOGRIFICATION = "幻化";            // 打开幻化界面

// 悄悄话模板：{} 为真实金币倍率（基础 100% + 玩家 VIP 金币加成，取自 PlayerVipBenefits）
constexpr char const* TXT_GOLD_RATE_WHISPER = "你的金币倍率为:{}%";

// ===== 欢迎语正文（npc_text 表）=====
constexpr uint32 TEXT_ID_BASE = 101000;          // 与生物入口一致，避开官方文本 ID 段
// 欢迎语文本条数（ID 101000 ~ 101023，由 data/小宠物生物_101000_白虎.sql 维护；
// SQL 中增删文本时需同步修改此值）
constexpr uint32 NPC_WELCOME_TEXT_COUNT = 23;

// ===== 菜单动作 ID =====
constexpr uint32 ACTION_SHOP = GOSSIP_ACTION_INFO_DEF + 1;                 // 宝物商店
constexpr uint32 ACTION_FLIGHT = GOSSIP_ACTION_INFO_DEF + 2;               // 航班
constexpr uint32 ACTION_GOLD_RATE = GOSSIP_ACTION_INFO_DEF + 3;            // 我的金币倍率
constexpr uint32 ACTION_TRANSMOGRIFICATION = GOSSIP_ACTION_INFO_DEF + 4;   // 幻化
constexpr uint32 ACTION_OPEN_NODE_MENU = GOSSIP_ACTION_INFO_DEF + 10;      // 节点传送（打开节点槽位菜单）
constexpr uint32 ACTION_NODE_SLOT_BASE = GOSSIP_ACTION_INFO_DEF + 20;      // 节点槽位（+槽位索引：打开该节点的操作菜单）
constexpr uint32 ACTION_NODE_TELEPORT_BASE = GOSSIP_ACTION_INFO_DEF + 30;  // 传送（+槽位索引）
constexpr uint32 ACTION_NODE_SET_BASE = GOSSIP_ACTION_INFO_DEF + 40;       // 记录位置（+槽位索引）

// ===== 传送节点功能（节点传送）=====
constexpr uint32 NPC_TELEPORT_NODE_ENTRY = 101001;   // 传送节点 gameobject 入口（data SQL 提供）
constexpr uint32 NODE_TEXT_ID = 101023;              // 节点子菜单问候语 ID（npc_text，data SQL 提供）
constexpr uint32 NODE_DURATION_SECONDS = 600; // 节点存活时长：2 小时（秒，SummonGameObject 以秒计）
constexpr uint32 SPELL_TELEPORT_VISUAL = 35517;      // 传送视觉法术

constexpr char const* TXT_TELEPORT_NODE = "节点传送";         // 主菜单项（节点传送）
constexpr char const* TXT_NODE_SLOT = "节点{}";               // 槽位标题（未记录，{} = 1/2/3）
constexpr char const* TXT_NODE_SLOT_RECORDED = "节点{}(已记录)"; // 槽位标题（已记录）
constexpr char const* TXT_NODE_SET = "记录位置";              // 节点操作菜单：记录当前位置
constexpr char const* TXT_NODE_TP = "传送";                   // 节点操作菜单：传送到该节点

// 航班功能使用条件校验：返回空字符串表示允许使用，否则返回拒绝原因
// 条件：仅限大世界（大陆地图，不含副本/团队/战场/竞技场），且玩家存活、不在战斗中
std::string GetFlightDenyReason(Player* player)
{
    if (!player->GetMap()->GetEntry()->IsContinent())
        return "航班仅在大世界中可以使用。";
    if (!player->IsAlive())
        return "你已死亡，无法使用航班。";
    if (player->IsInCombat())
        return "你正处于战斗中，无法使用航班。";
    return {};
}

// 航班：打开飞行点地图（不要求 NPC 位于飞行点附近）
// 复刻 WorldSession::SendTaxiMenu，但 NPC 附近无飞行节点时回退为玩家已解锁的节点； 
void SendTaxiMapFor(Player* player, Creature* creature)
{
    // 优先取 NPC 附近的节点（标准行为）
    uint32 curloc = sObjectMgr->GetNearestTaxiNode(*creature, player->GetTeamId(true));

    // 回退 1：取玩家在同地图已解锁的节点
    for (uint32 i = 1; i < sTaxiNodesStore.GetNumRows() && !curloc; ++i)
    {
        TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(i);
        if (!node || node->map_id != player->GetMapId())
            continue;
        if (player->m_taxi.IsTaximaskNodeKnown(node->ID))
            curloc = node->ID;
    }
    // 回退 2：取玩家在任意地图已解锁的节点
    for (uint32 i = 1; i < sTaxiNodesStore.GetNumRows() && !curloc; ++i)
    {
        TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(i);
        if (node && player->m_taxi.IsTaximaskNodeKnown(node->ID))
            curloc = node->ID;
    }

    // 玩家尚未解锁任何飞行点，无法打开航班地图
    if (!curloc)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("你还没有解锁任何飞行点，无法打开航班地图。");
        return;
    }

    WorldPacket data(SMSG_SHOWTAXINODES, 4 + 8 + 4 + 8 * 4);
    data << uint32(1);
    data << creature->GetGUID();
    data << uint32(curloc);
    player->m_taxi.AppendTaximaskTo(data, player->isTaxiCheater());
    player->GetSession()->SendPacket(&data);
}

constexpr uint32 MAX_NODE_SLOTS = 3;   // 传送节点槽位数量

// 玩家已设置的单个传送节点信息（仅在线会话期间有效，下线即清除）
struct TeleportNodeInfo
{
    bool valid = false;       // 该槽位是否已记录位置
    ObjectGuid nodeGUID;      // 召唤的视觉标记 GO 的 GUID，用于重新记录时销毁旧标记
    uint32 mapId = 0;         // 设置节点时所处的地图 ID
    uint32 instanceId = 0;    // 设置节点时所处的副本实例 ID（野外地图为 0）
    float x = 0.0f;           // 节点坐标（传送时直接使用）
    float y = 0.0f;
    float z = 0.0f;
    float o = 0.0f;           // 节点朝向
};

// 按玩家 GUID 各自保存 3 个独立槽位（ObjectGuid 已提供 std::hash 特化，可作 map 键）
std::unordered_map<ObjectGuid, std::array<TeleportNodeInfo, MAX_NODE_SLOTS>> g_nodeStorage;

// 判断指定槽位是否已记录位置（玩家从未设置过节点视为未记录）
bool IsNodeSlotRecorded(Player* player, uint32 slot)
{
    auto itr = g_nodeStorage.find(player->GetGUID());
    return itr != g_nodeStorage.end() && itr->second[slot].valid;
}

// 在玩家当前位置记录坐标/地图/实例到指定槽位，并召唤一个持续 2 小时的视觉标记 GO
void DoSetTeleportNode(Player* player, uint32 slot)
{
    if (!player->IsAlive() || player->IsInCombat() || player->IsInFlight())
    {
        ChatHandler(player->GetSession()).PSendSysMessage("你已死亡、战斗中或飞行中，无法记录位置。");
        return;
    }

    Map* map = player->GetMap();
    if (!map)
        return;

    auto& info = g_nodeStorage[player->GetGUID()][slot];

    // 若该槽位已记录，先销毁旧的视觉标记 GO（若仍存在于当前地图上）
    if (info.valid)
    {
        if (GameObject* oldNode = map->GetGameObject(info.nodeGUID))
        {
            oldNode->SetRespawnTime(0);
            oldNode->Delete();
        }
    }

    // 记录坐标、地图、实例（野外实例 ID 为 0）
    info.mapId = map->GetId();
    info.instanceId = map->Instanceable() ? map->GetInstanceId() : 0;
    info.x = player->GetPositionX();
    info.y = player->GetPositionY();
    info.z = player->GetPositionZ();
    info.o = player->GetOrientation();
    info.valid = false; // 召唤成功后再置为 true

    // 召唤视觉标记 GO（地图级召唤，不绑定玩家，玩家死亡/离开/下线均不销毁）
    // 名称由 gameobject_template.name 固定为「传送节点」，无法运行时动态指定
    GameObject* node = map->SummonGameObject(NPC_TELEPORT_NODE_ENTRY, info.x, info.y, info.z, info.o,
        0.0f, 0.0f, 0.0f, 0.0f, NODE_DURATION_SECONDS);
    if (!node)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("无法在此处建立传送节点。");
        return;
    }

    info.nodeGUID = node->GetGUID();
    info.valid = true;

    ChatHandler(player->GetSession()).PSendSysMessage("已记录节点{}的位置，2 小时后自动消失。", slot + 1);
}

// 按记录的坐标传送指定槽位：野外位置允许跨地图传送，副本位置必须匹配地图和实例
void DoTeleportToNode(Player* player, uint32 slot)
{
    if (!player->IsAlive() || player->IsInCombat())
    {
        ChatHandler(player->GetSession()).PSendSysMessage("你已死亡或处于战斗中，无法传送。");
        return;
    }

    auto itr = g_nodeStorage.find(player->GetGUID());
    if (itr == g_nodeStorage.end() || !itr->second[slot].valid)
    {
        ChatHandler(player->GetSession()).PSendSysMessage("节点{}还没有记录位置。", slot + 1);
        return;
    }

    TeleportNodeInfo const& info = itr->second[slot];
    Map* curMap = player->GetMap();
    if (!curMap)
        return;

    // 记录点位于副本内时，必须与玩家当前所处的地图和副本实例一致
    if (info.instanceId != 0)
    {
        if (curMap->GetId() != info.mapId)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("传送节点位于副本地图中，无法跨地图传送。");
            return;
        }

        if (curMap->GetInstanceId() != info.instanceId)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("传送节点位于另一个副本实例中，无法传送。");
            return;
        }
    }

    // 先播放传送视觉法术，再按坐标传送；非副本位置支持跨地图传送
    player->CastSpell(player, SPELL_TELEPORT_VISUAL, true);
    player->TeleportTo(info.mapId, info.x, info.y, info.z, info.o);
}

// 打开指定槽位的操作菜单（传送 / 记录位置）
void OpenNodeSlotMenu(Player* player, Creature* creature, uint32 slot)
{
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, TXT_NODE_TP, GOSSIP_SENDER_MAIN, ACTION_NODE_TELEPORT_BASE + slot);
    AddGossipItemFor(player, GOSSIP_ICON_CHAT, TXT_NODE_SET, GOSSIP_SENDER_MAIN, ACTION_NODE_SET_BASE + slot);
    SendGossipMenuFor(player, NODE_TEXT_ID, creature->GetGUID());
}
}

class NpcBaihuGossip : public CreatureScript
{
public:
    NpcBaihuGossip() : CreatureScript("npc_baihu_gossip") { }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        if (creature->GetEntry() != NPC_BAIHU_ENTRY)
            return false;

        // 宝物商店对所有玩家开放
        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, TXT_SHOP, GOSSIP_SENDER_MAIN, ACTION_SHOP);

        // 小宠物的其他功能仅对宠物主人开放
        if (creature->GetCharmerOrOwnerPlayerOrPlayerItself() == player)
        {
            // 节点传送（打开子级对话菜单，可设置节点 / 传送到节点）
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, TXT_TELEPORT_NODE, GOSSIP_SENDER_MAIN, ACTION_OPEN_NODE_MENU);
            // 航班（打开飞行点地图）：仅在满足使用条件（大世界 + 存活 + 非战斗）时显示
            if (GetFlightDenyReason(player).empty())
                AddGossipItemFor(player, GOSSIP_ICON_TAXI, TXT_FLIGHT, GOSSIP_SENDER_MAIN, ACTION_FLIGHT);
            // 幻化（打开 Lua 幻化界面）
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, TXT_TRANSMOGRIFICATION, GOSSIP_SENDER_MAIN, ACTION_TRANSMOGRIFICATION);
            // 我的金币倍率（NPC 悄悄话告知）
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, TXT_GOLD_RATE, GOSSIP_SENDER_MAIN, ACTION_GOLD_RATE);
        }

        // 正文欢迎语：70% 概率取第一条，30% 概率随机取一条
        uint32 textId = TEXT_ID_BASE;
        if (!roll_chance_i(70))
            textId += urand(1, NPC_WELCOME_TEXT_COUNT - 1);
        SendGossipMenuFor(player, textId, creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
    {
        player->PlayerTalkClass->ClearMenus();

        // 除商店外的选项仅允许宠物主人执行，防止通过伪造动作绕过菜单显示限制
        if (action != ACTION_SHOP && creature->GetCharmerOrOwnerPlayerOrPlayerItself() != player)
        {
            CloseGossipMenuFor(player);
            return true;
        }

        // 节点槽位菜单：点击「节点1/2/3」→ 打开该节点的操作菜单
        if (action >= ACTION_NODE_SLOT_BASE && action < ACTION_NODE_SLOT_BASE + MAX_NODE_SLOTS)
        {
            OpenNodeSlotMenu(player, creature, action - ACTION_NODE_SLOT_BASE);
            return true;
        }
        // 节点操作：记录位置
        if (action >= ACTION_NODE_SET_BASE && action < ACTION_NODE_SET_BASE + MAX_NODE_SLOTS)
        {
            DoSetTeleportNode(player, action - ACTION_NODE_SET_BASE);
            CloseGossipMenuFor(player);
            return true;
        }
        // 节点操作：传送
        if (action >= ACTION_NODE_TELEPORT_BASE && action < ACTION_NODE_TELEPORT_BASE + MAX_NODE_SLOTS)
        {
            DoTeleportToNode(player, action - ACTION_NODE_TELEPORT_BASE);
            CloseGossipMenuFor(player);
            return true;
        }

        switch (action)
        {
        case ACTION_SHOP: // 宝物商店：打开售卖窗口（商品列表见 npc_vendor 表）
            CloseGossipMenuFor(player);
            player->GetSession()->SendListInventory(creature->GetGUID());
            break;
        case ACTION_FLIGHT: // 航班：打开飞行点地图（NPC 不在飞行点附近时自动回退到已解锁节点）
        {
            // 防御性校验：仅限大世界，且玩家存活、不在战斗中
            std::string denyReason = GetFlightDenyReason(player);
            if (!denyReason.empty())
            {
                ChatHandler(player->GetSession()).PSendSysMessage("{}", denyReason);
                CloseGossipMenuFor(player);
                break;
            }
            CloseGossipMenuFor(player);
            SendTaxiMapFor(player, creature);
            break;
        }
        case ACTION_GOLD_RATE: // 我的金币倍率：由 NPC 悄悄话告知真实倍率
        {
            uint32 goldRate = 100 + player->GetVipBenefits().gold_loot_bonus;
            ChatHandler(player->GetSession()).PSendSysMessage("{}", Acore::StringFormat(TXT_GOLD_RATE_WHISPER, goldRate));
            CloseGossipMenuFor(player);
            break;
        }
        case ACTION_TRANSMOGRIFICATION: // 幻化：由 Lua 脚本处理客户端界面
            CloseGossipMenuFor(player);
            break;
        case ACTION_OPEN_NODE_MENU: // 节点传送：打开 3 个独立节点槽位
        {
            for (uint32 slot = 0; slot < MAX_NODE_SLOTS; ++slot)
            {
                std::string label = IsNodeSlotRecorded(player, slot)
                    ? Acore::StringFormat(TXT_NODE_SLOT_RECORDED, slot + 1)
                    : Acore::StringFormat(TXT_NODE_SLOT, slot + 1);
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, label, GOSSIP_SENDER_MAIN, ACTION_NODE_SLOT_BASE + slot);
            }
            SendGossipMenuFor(player, NODE_TEXT_ID, creature->GetGUID());
            break;
        }
        default: // 未知选项：直接关闭
            CloseGossipMenuFor(player);
            break;
        }

        return true;
    }
};

// 下线时清空该玩家保存的传送节点，避免节点数据残留
class NpcBaihuNodePlayerScript : public PlayerScript
{
public:
    NpcBaihuNodePlayerScript() : PlayerScript("npc_baihu_node_player") { }

    void OnPlayerLogout(Player* player) override
    {
        g_nodeStorage.erase(player->GetGUID());
    }
};

void AddNpcBaihuGossipScripts()
{
    new NpcBaihuGossip();
    new NpcBaihuNodePlayerScript();
}
