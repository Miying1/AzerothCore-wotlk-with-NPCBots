/*
 * 随机坐骑礼包（物品 60002）使用脚本 —— mod-playervip 模块
 *
 * 功能：
 *   玩家使用「随机坐骑礼包」时，从数据库表 custom_mount_gift_spells 读取该礼包
 *   可开出的坐骑法术 ID、名称及权重（chance），过滤掉玩家已学习的，按权重随机
 *   学习一个未学习坐骑并消耗礼包。
 *
 * 原理：
 *   - 坐骑法术 = 坐骑物品的 spellid_2（spelltrigger_2 = 6 即 LEARN_SPELL_ID）。
 *   - 玩家是否已学习某坐骑，用 Player::HasSpell(坐骑法术ID) 判断。
 *   - 爆率由 custom_mount_gift_spells.chance 控制：值越大越容易开出。
 *     最终概率 = 该坐骑 chance / 所有未学习坐骑 chance 之和。
 *   - 坐骑名称优先用表里的 name 字段（中文），为空时回退到 spell_dbc。
 *   - 物品通过 item_template.ScriptName = 'item_mount_gift_box' 绑定本脚本；
 *     物品 spellid_1 = 18282（无效果触发法术）使客户端把礼包当作「可使用」物品。
 */

#include "Chat.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "ItemScript.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <string>
#include <vector>

// 随机坐骑礼包物品 ID
enum
{
    ITEM_MOUNT_GIFT_BOX = 60002
};

// 单个未学习坐骑候选：法术 ID + 名称 + 权重
struct MountCandidate
{
    uint32 spellId;
    uint32 chance;
    std::string name;
};

// 随机坐骑礼包使用脚本
class item_mount_gift_box : public ItemScript
{
public:
    item_mount_gift_box() : ItemScript("item_mount_gift_box") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        // 读取该礼包可开出的坐骑法术 ID、名称及权重
        QueryResult result = WorldDatabase.Query(
            "SELECT `spellId`, `name`, `chance` FROM `custom_mount_gift_spells` WHERE `entry` = {}", ITEM_MOUNT_GIFT_BOX);
        if (!result)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("坐骑礼包数据未配置，请联系管理员。");
            return true; // 阻止默认使用，不消耗礼包
        }

        // 收集玩家尚未学习且权重 > 0 的坐骑，并累加总权重
        std::vector<MountCandidate> candidates;
        uint32 totalWeight = 0;
        do
        {
            Field* fields = result->Fetch();
            uint32 spellId    = fields[0].Get<uint32>();
            std::string name  = fields[1].Get<std::string>();
            uint32 chance     = fields[2].Get<uint32>();
            if (!player->HasSpell(spellId) && chance > 0)
            {
                candidates.push_back({ spellId, chance, name });
                totalWeight += chance;
            }
        } while (result->NextRow());

        // 全部坐骑都已学习（或无可用候选），阻止使用并提示
        if (candidates.empty())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("你已经学会了礼包内的全部坐骑，无法再使用。");
            return true; // 阻止默认使用，不消耗礼包
        }

        // 按权重加权随机：roll 落在 [0, totalWeight)，按累计权重区间命中
        uint32 roll = urand(0u, totalWeight - 1);
        MountCandidate const* chosen = &candidates.back();
        uint32 acc = 0;
        for (MountCandidate const& candidate : candidates)
        {
            acc += candidate.chance;
            if (roll < acc)
            {
                chosen = &candidate;
                break;
            }
        }

        // 学习该坐骑技能
        player->learnSpell(chosen->spellId);

        // 坐骑名称：优先用表里的 name，为空则回退 spell_dbc（中文优先）
        std::string mountName = chosen->name;
        if (mountName.empty())
        {
            mountName = "未知坐骑";
            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(chosen->spellId))
            {
                char const* name = spellInfo->SpellName[LOCALE_zhCN];
                if (!name)
                    name = spellInfo->SpellName[DEFAULT_LOCALE];
                if (name)
                    mountName = name;
            }
        }

        // 消耗一个礼包
        player->DestroyItemCount(item->GetEntry(), 1, true);

        ChatHandler(player->GetSession()).PSendSysMessage("你学会了新坐骑「{}」！", mountName);

        return true; // 已自行处理，阻止默认施法（spellid_1 18282）
    }
};

void AddSC_item_mount_gift_box()
{
    new item_mount_gift_box();
}
