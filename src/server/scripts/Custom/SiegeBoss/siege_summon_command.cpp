/*
 * 攻城BOSS：通用召唤命令
 *
 * 功能：
 *   1. 按「生物 ID + 数量」召唤怪物，召唤位置以玩家为中心 15 码内随机散布，
 *      并做「视野可达（LOS）+ 碰撞（地面有效）」双重校验，避免刷新到墙内/虚空。
 *   2. 内置多组预设怪物组合，使用命令时只需传一个标识即可一键召唤整组。
 *
 * 命令：
 *   .mob <entry> [count]      按 ID 召唤（数量可省略，默认 1）
 *   .mob preset <id>          按预设标识召唤整组
 *   .mob preset list          列出所有预设组合
 *
 * 召唤物说明：
 *   使用 WorldObject::SummonCreature（不传 SummonProperties）创建「纯 TempSummon」
 *   （UNIT_MASK_SUMMON，非 Guardian/Minion），因此：
 *     - 拥有完整仇恨列表（可被嘲讽、按仇恨转火）
 *     - 走 creature_template.AIName / ScriptName（可挂 C++ 脚本）
 *   阵营由 creature_template.faction 决定（攻城爪牙 = 16，与变身后真人 BOSS 同阵营友善）。
 */

#include "Chat.h"
#include "CommandScript.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "TemporarySummon.h"
#include <vector>

using namespace Acore::ChatCommands;

// ======================= 可调参数 =======================

// 召唤散布半径（码）
constexpr float MOB_SPAWN_RADIUS = 15.0f;

// 召唤物存活时长（秒）；配合 TEMPSUMMON_CORPSE_DESPAWN：死亡立即消失，超时也消失
constexpr uint32 MOB_DESPAWN_SECONDS = 300;

// 单次召唤数量上限（防误刷）
constexpr uint32 MOB_MAX_COUNT = 50;

// 单个怪物定位的最大随机尝试次数（超过则兜底用玩家脚下）
constexpr uint32 MOB_POSITION_MAX_ATTEMPTS = 20;

// ======================= 预设组合 =======================

// 预设组合里的单个怪物
struct PresetMob
{
    uint32 entry;   // 生物 ID
    uint32 count;   // 数量
};

// 预设组合
struct MobPreset
{
    uint32 id;                 // 预设标识（命令传入）
    char const* name;          // 预设名称（提示用）
    std::vector<PresetMob> mobs;   // 怪物列表
};

// 内置预设组合（按需增删改，标识 id 需唯一）
std::vector<MobPreset> const SiegeMobPresets =
{
    { 1, "攻城爪牙·基础组", { { 910101, 2 }, { 910102, 1 } } },
    { 2, "攻城爪牙·满编组", { { 910101, 3 }, { 910102, 2 }, { 910103, 1 } } },
    { 3, "攻城爪牙·精英组", { { 910103, 2 }, { 910102, 2 } } },
    { 4, "攻城爪牙·射手组", { { 910102, 3 } } },
};

// 按标识查找预设组合
MobPreset const* FindPreset(uint32 presetId)
{
    for (auto const& preset : SiegeMobPresets)
        if (preset.id == presetId)
            return &preset;

    return nullptr;
}

// ======================= 召唤位置 =======================

// 在玩家周围 MOB_SPAWN_RADIUS 码内寻找合法召唤点：
//   - 地面有效（排除墙内 / 虚空）
//   - 视野可达（玩家能看到该点，排除墙后）
// 找不到时兜底返回玩家脚下，保证一定能召出来。
void FindSafeSpawnPosition(Player* player, Position& outPos)
{
    for (uint32 i = 0; i < MOB_POSITION_MAX_ATTEMPTS; ++i)
    {
        Position pos = player->GetRandomNearPosition(MOB_SPAWN_RADIUS);

        // 修正 Z 到合法地面高度，同时取地面高度判断是否有效
        float groundZ = INVALID_HEIGHT;
        player->UpdateAllowedPositionZ(pos.m_positionX, pos.m_positionY, pos.m_positionZ, &groundZ);

        // 无有效地面（墙内 / 虚空）→ 跳过
        if (groundZ <= INVALID_HEIGHT)
            continue;

        // 视野被墙阻挡 → 跳过
        if (!player->IsWithinLOS(pos.m_positionX, pos.m_positionY, pos.m_positionZ))
            continue;

        outPos = pos;
        return;
    }

    // 兜底：用玩家脚下
    outPos = player->GetPosition();
}

// ======================= 召唤动作 =======================

// 在指定位置召唤一只怪物（纯 TempSummon）
bool SummonOneMob(Player* player, uint32 entry, Position const& pos)
{
    if (!sObjectMgr->GetCreatureTemplate(entry))
        return false;

    // 纯 TempSummon（不传 SummonProperties），有仇恨列表 + 可挂 ScriptName
    TempSummon* summon = player->SummonCreature(entry, pos, TEMPSUMMON_CORPSE_DESPAWN, MOB_DESPAWN_SECONDS);
    return summon != nullptr;
}

// ======================= 命令脚本 =======================

class mob_commandscript : public CommandScript
{
public:
    mob_commandscript() : CommandScript("mob_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable presetCommandTable =
        {
            { "list", HandleMobPresetListCommand, rbac::RBAC_PERM_COMMAND_NPC_ADD, Console::No },
            { "",     HandleMobPresetCommand,     rbac::RBAC_PERM_COMMAND_NPC_ADD, Console::No }
        };
        // 注意：空名字命令只能出现在「非顶层」的具名命令下，作为该命令的默认处理器。
        // 顶层命令必须全部有名字，否则 ChatCommandNode::LoadCommandsIntoMap 会直接断言崩溃。
        static ChatCommandTable mobCommandTable =
        {
            { "preset", presetCommandTable },
            { "",       HandleMobSummonCommand, rbac::RBAC_PERM_COMMAND_NPC_ADD, Console::No }
        };
        static ChatCommandTable commandTable =
        {
            { "mob", mobCommandTable }
        };
        return commandTable;
    }

    // .mob <entry> [count]
    static bool HandleMobSummonCommand(ChatHandler* handler, uint32 entry, Optional<uint32> count)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (!sObjectMgr->GetCreatureTemplate(entry))
        {
            handler->SendErrorMessage("生物 {} 不存在于 creature_template。", entry);
            return false;
        }

        uint32 num = count.value_or(1);
        if (num == 0 || num > MOB_MAX_COUNT)
        {
            handler->SendErrorMessage("数量需在 1 ~ {} 之间。", MOB_MAX_COUNT);
            return false;
        }

        uint32 spawned = 0;
        for (uint32 i = 0; i < num; ++i)
        {
            Position pos;
            FindSafeSpawnPosition(player, pos);
            if (SummonOneMob(player, entry, pos))
                ++spawned;
        }

        handler->PSendSysMessage("已召唤 {} 只生物（entry={}）。", spawned, entry);
        return true;
    }

    // .mob preset <id>
    static bool HandleMobPresetCommand(ChatHandler* handler, uint32 presetId)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        MobPreset const* preset = FindPreset(presetId);
        if (!preset)
        {
            handler->SendErrorMessage("预设 {} 不存在，用 .mob preset list 查看可用预设。", presetId);
            return false;
        }

        uint32 spawned = 0;
        for (auto const& mob : preset->mobs)
        {
            for (uint32 i = 0; i < mob.count; ++i)
            {
                Position pos;
                FindSafeSpawnPosition(player, pos);
                if (SummonOneMob(player, mob.entry, pos))
                    ++spawned;
            }
        }

        handler->PSendSysMessage("已按预设「{}」召唤 {} 只怪物。", preset->name, spawned);
        return true;
    }

    // .mob preset list
    static bool HandleMobPresetListCommand(ChatHandler* handler)
    {
        handler->PSendSysMessage("可用预设组合：");
        for (auto const& preset : SiegeMobPresets)
        {
            uint32 total = 0;
            for (auto const& mob : preset.mobs)
                total += mob.count;

            handler->PSendSysMessage("  [{}] {}（共 {} 只）", preset.id, preset.name, total);
        }
        return true;
    }
};

void AddSC_siege_summon_command()
{
    new mob_commandscript();
}
