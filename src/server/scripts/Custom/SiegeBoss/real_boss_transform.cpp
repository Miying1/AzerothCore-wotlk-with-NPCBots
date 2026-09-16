/*
 * 真人BOSS：化身技能 + 参与掉落脚本（按角色 GUID 配置）
 *
 * 玩法：
 *   玩家释放技能 93001「化身攻城BOSS」——该技能在 DBC 里【只负责加免疫光环 BUFF（免控）】，
 *   模型 / 放大 / 血量 / 伤害 / 掉落全部由本脚本按「施法者 GUID」配置并在代码中直接设置。
 *
 * 结构：
 *   1. 内置配置表 g_RealBossConfigs：GUID → { 模型, 放大, 血量, 伤害, 掉落 }
 *   2. SpellScript  spell_real_boss_transform：挂 93001，OnCast 判定 GUID 并修改属性
 *   3. UnitScript   RealBossCombatTracker：OnDamage 记录所有参与战斗的玩家
 *   4. PlayerScript RealBossDeathLootPlayerScript：OnPlayerJustDied 恢复变身 + 给所有参与者发战利品
 *
 * 掉落：
 *   真人BOSS 被围攻时，战利品发放给【所有参与战斗的玩家】，而非仅最后击杀者。
 *   参与判定：对 BOSS 造成过伤害的玩家（OnDamage 记录），BOSS 死亡时统一发放。
 *
 * 部署：
 *   1. 重新 CMake configure + 编译 worldserver（本文件已由 custom_script_loader.cpp 注册）。
 *   2. 数据库挂载法术脚本（spell_script_names）：
 *        DELETE FROM spell_script_names WHERE spell_id = 93001;
 *        INSERT INTO spell_script_names (spell_id, ScriptName) VALUES (93001, 'spell_real_boss_transform');
 *   3. 技能 DBC：93001 的 Effect_1 设为 DUMMY(3)（挂本脚本），
 *      免控光环仍由 DBC 光环链实现（93011 → 93004 → 93005/93006/93007）。
 *
 * 说明：
 *   - GUID 是角色的唯一标识（characters 表 guid，游戏内 .guid 可查）。
 *   - 模型 / 放大 / 血量 / 伤害均为「直接设置」，死亡复活、重新登录后还原（非持久化）。
 *   - 战利品按「被杀者 GUID」配置，发放给所有参与战斗的玩家。
 */

#include "CharacterDatabase.h"
#include "Chat.h"
#include "Item.h"
#include "Mail.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerScript.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "StringFormat.h"
#include "UnitScript.h"
#include <unordered_map>
#include <unordered_set>

// ======================= 内置配置 =======================

// 掉落条目：{ 物品 ID, 数量 }
struct RealBossLootEntry
{
    uint32 itemId;
    uint32 count;
};

// 单个真人BOSS 的完整配置
struct RealBossConfig
{
    uint32 guid;        // 角色 GUID
    uint32 displayId;   // 变身模型 display id
    float  scale;       // 放大倍数
    uint32 maxHealth;   // 变身血量（固定值）
    float  meleeMin;    // 平砍最小伤害
    float  meleeMax;    // 平砍最大伤害
    uint32 lootMoney;   // 掉落金币（单位：铜，1 金 = 10000 铜）
    std::vector<RealBossLootEntry> loot;  // 掉落物品
};

// 真人BOSS 配置表：不同 GUID 对应不同的模型 / 伤害 / 血量 / 掉落
static std::vector<RealBossConfig> const g_RealBossConfigs =
{
    // ===== 真人BOSS 1 =====
    {
        123456,                        // GUID（请替换为实际角色 GUID）
        100000,                        // 模型：愤怒融合体
        2.0f,                          // 放大 2 倍
        500000,                        // 血量 50 万
        9000.0f,                       // 平砍最小伤害
        11000.0f,                      // 平砍最大伤害
        0,                             // 金币（0 = 不发）
        { { 40001, 1 }, { 40002, 3 } } // 掉落物品
    },

    // ===== 真人BOSS 2 =====
    {
        789012,                        // GUID（请替换为实际角色 GUID）
        100001,                        // 模型（请替换为实际 display id）
        1.5f,                          // 放大 1.5 倍
        300000,                        // 血量 30 万
        7000.0f,                       // 平砍最小伤害
        9000.0f,                       // 平砍最大伤害
        100000,                        // 金币 10 金
        { { 40003, 1 } }               // 掉落物品
    },
};

// 按 GUID 查找真人BOSS 配置，找不到返回 nullptr
static RealBossConfig const* FindRealBossConfig(uint32 guid)
{
    for (RealBossConfig const& cfg : g_RealBossConfigs)
        if (cfg.guid == guid)
            return &cfg;

    return nullptr;
}

// 判断单位是否为真人BOSS 并返回其配置。
// 先 O(1) 旗标快速过滤（真人BOSS 变身时移除了 PLAYER_CONTROLLED，普通玩家必有），
// 再查配置表精确确认——用于 OnDamage 等高频钩子，避免对每次伤害都线性查表。
static RealBossConfig const* GetRealBossConfig(Unit* unit)
{
    if (!unit || !unit->IsPlayer() || unit->HasUnitFlag(UNIT_FLAG_PLAYER_CONTROLLED))
        return nullptr;

    return FindRealBossConfig(unit->GetGUID().GetCounter());
}

// ======================= 公共设置（所有真人BOSS 共用） =======================

// 阵营 16 = 怪物（全敌对）：所有真人BOSS 共用的公共设置，不进配置表
static constexpr uint32 REAL_BOSS_FACTION = 16;

// 变身标志光环：主技能 93001 触发链施加的光环（攻城BOSS·破甲威势 93011），
// 变身期间持续存在，作为「是否处于化身BOSS 形态」的判断依据。
// 注意：该光环必须设置「死亡保留」属性（DBC AttributesEx3 的 SPELL_ATTR3_ALLOW_AURA_WHILE_DEAD），
//       否则玩家死亡时会被引擎 RemoveAllAurasOnDeath 提前移除，导致 OnPlayerJustDied 无法据此判断。
static constexpr uint32 REAL_BOSS_TRANSFORM_AURA_SPELL = 93011;

// 背包满时改发邮件（附件为战利品）
static void SendLootByMail(Player* receiver, RealBossLootEntry const& entry, std::string_view bossName)
{
    Item* item = Item::CreateItem(entry.itemId, entry.count, receiver);
    if (!item)
        return;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    MailDraft draft("BOSS战利品",
        Acore::StringFormat("你在参与击杀BOSS「{}」时背包已满，附件为战利品。", bossName));
    draft.AddItem(item);
    draft.SendMailTo(trans, MailReceiver(receiver), MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_DEFAULT));
    CharacterDatabase.CommitTransaction(trans);
}

// ======================= 参与战斗追踪 =======================

// 真人BOSS GUID → 参与攻击的玩家 GUID 集合（BOSS 死亡后统一发放并清除）
static std::unordered_map<uint32, std::unordered_set<uint32>> g_RealBossCombatants;

// ======================= 化身法术脚本 =======================

class spell_real_boss_transform : public SpellScript
{
    PrepareSpellScript(spell_real_boss_transform);

    void HandleTransform()
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->IsPlayer())
            return;

        Player* player = caster->ToPlayer();

        // 判定施法者 GUID：仅配置表中的真人BOSS 可触发
        RealBossConfig const* cfg = FindRealBossConfig(player->GetGUID().GetCounter());
        if (!cfg)
            return;

        // 已处于该 BOSS 形态则跳过，避免重复施放叠加属性
        if (player->GetDisplayId() == cfg->displayId)
            return;

        // 1. 模型
        player->SetDisplayId(cfg->displayId);
        // 2. 放大（同步更新碰撞体 / 战斗范围）
        player->SetObjectScale(cfg->scale);
        // 3. 血量
        player->SetMaxHealth(cfg->maxHealth);
        player->SetFullHealth();
        // 4. 伤害（平砍固定伤害）
        player->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, cfg->meleeMin);
        player->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, cfg->meleeMax);
        player->UpdateDamagePhysical(BASE_ATTACK);
        // 5. 阵营（公共设置：16 = 全敌对，移除玩家可控旗标使其可被攻击）
        player->SetFaction(REAL_BOSS_FACTION);
        player->RemoveUnitFlag(UNIT_FLAG_PLAYER_CONTROLLED);
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_real_boss_transform::HandleTransform);
    }
};

// ======================= 参与战斗追踪脚本 =======================

class RealBossCombatTracker : public UnitScript
{
public:
    RealBossCombatTracker() : UnitScript("RealBossCombatTracker", true, { UNITHOOK_ON_DAMAGE })
    {
    }

    // 记录攻击真人BOSS 的玩家（用于 BOSS 死亡时给所有参与者发战利品）
    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!attacker)
            return;

        // 仅真人BOSS（受害者）：快速过滤 + 精确确认
        RealBossConfig const* cfg = GetRealBossConfig(victim);
        if (!cfg)
            return;

        // 归属到真正的玩家（兼容宠物 / 被控制单位），排除 BOSS 自身
        Player* attackerPlayer = attacker->GetCharmerOrOwnerPlayerOrPlayerItself();
        if (!attackerPlayer || attackerPlayer == victim)
            return;

        g_RealBossCombatants[cfg->guid].insert(attackerPlayer->GetGUID().GetCounter());
    }
};

// ======================= 掉落发放脚本 =======================

class RealBossDeathLootPlayerScript : public PlayerScript
{
public:
    RealBossDeathLootPlayerScript() : PlayerScript("RealBossDeathLootPlayerScript",
        { PLAYERHOOK_ON_PLAYER_JUST_DIED, PLAYERHOOK_ON_LOGOUT })
    {
    }

    // 玩家登出：清理参与记录（该玩家作为真人BOSS 的记录 + 从其它 BOSS 的参与者集合中移除自身）
    void OnPlayerLogout(Player* player) override
    {
        if (!player)
            return;

        uint32 guid = player->GetGUID().GetCounter();

        // 1) 作为真人BOSS 登出：BOSS 已消失，丢弃其所有参与者记录
        g_RealBossCombatants.erase(guid);
    }

    // 真人BOSS 死亡：恢复变身 + 给所有参与战斗的玩家发战利品
    void OnPlayerJustDied(Player* player) override
    {
        if (!player)
            return;

        // 仅处理处于「化身BOSS」形态的玩家（通过变身标志光环判断），普通 PvP 死亡不触发
        if (!player->HasAura(REAL_BOSS_TRANSFORM_AURA_SPELL))
            return;

        RealBossConfig const* cfg = FindRealBossConfig(player->GetGUID().GetCounter());
        if (!cfg)
            return;

        // 移除变身标志光环，解除剩余的光环链（免控 / 破甲等）
        player->RemoveAurasDueToSpell(REAL_BOSS_TRANSFORM_AURA_SPELL);

        // 全服广播：真人BOSS 被击杀
        ChatHandler(nullptr).SendWorldText("|cFFFFD700【系统】|r BOSS |cFFFF4500「{}」|r 已被勇士们合力击杀！", player->GetName());

        // 恢复阵营与玩家可控旗标（公共设置）
        player->RestoreFaction();
        player->SetUnitFlag(UNIT_FLAG_PLAYER_CONTROLLED);

        // 恢复模型与缩放
        if (player->GetDisplayId() == cfg->displayId)
            player->RestoreDisplayId();
        player->SetObjectScale(1.0f);

        // 发放战利品给所有参与战斗的玩家（而非仅最后击杀者）
        auto it = g_RealBossCombatants.find(cfg->guid);
        if (it != g_RealBossCombatants.end())
        {
            for (uint32 guid : it->second)
            {
                // 仅在线玩家可发放：离线（已登出）返回空则跳过；
                // 死亡状态（含释放灵魂）的在线玩家仍正常发放。
                Player* combatant = ObjectAccessor::FindPlayerByLowGUID(guid);
                if (!combatant)
                    continue;

                // 发放物品：背包有空间直接发，满了改发邮件
                for (RealBossLootEntry const& entry : cfg->loot)
                {
                    if (!entry.itemId || !entry.count)
                        continue;

                    ItemPosCountVec dest;
                    if (combatant->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, entry.itemId, entry.count) == EQUIP_ERR_OK)
                        combatant->AddItem(entry.itemId, entry.count);
                    else
                        SendLootByMail(combatant, entry, player->GetName());
                }

                // 发放金币
                if (cfg->lootMoney)
                    combatant->ModifyMoney(cfg->lootMoney);

                // 参与提示
                // ChatHandler(combatant->GetSession()).PSendSysMessage("你参与了击杀BOSS「{}」，获得了个人战利品！", player->GetName());
            }

            // 清理参与者记录
            g_RealBossCombatants.erase(it);
        }
    }
};

void AddSC_real_boss_transform()
{
    RegisterSpellScript(spell_real_boss_transform);
    new RealBossCombatTracker();
    new RealBossDeathLootPlayerScript();
}
