/*
 * 可驯服生物召唤卷轴（物品 69000）使用脚本 —— mod-playervip 模块
 *
 * 功能：
 *   猎人玩家在野外大世界使用「可驯服生物召唤卷轴」时，从物品描述文本中解析出
 *   生物 entry（格式：[entry]，例如 "使用后召唤一个可供驯服的生物[94000]..."），
 *   在其身边临时召唤一个可供驯服的野生生物，成功后消耗一个卷轴。
 *
 * 设计：
 *   - 召唤的生物为临时召唤物（TempSummon，不写库、无拥有者），
 *     猎人可直接对其施放「驯服野兽」（法术 1515）。
 *   - 生物 ID 从 item_template.Description 的 [entry] 中动态解析，
 *     因此同一卷轴模板可通过修改 description 指向不同生物。
 *   - 物品通过 item_template.ScriptName = 'item_tameable_summon' 绑定本脚本；
 *     物品 spellid_1 = 18282（无效果触发法术）使客户端把物品当作「可使用」。
 *   - 物品 SQL 通过 AllowableClass = 4 限定仅猎人可用；脚本内再次校验兜底。
 */

#include "Chat.h"
#include "Common.h"
#include "ItemScript.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"

#include <string>

// 从描述文本中解析 [entry] 里的生物 ID，成功返回 true 并写入 entry
static bool ParseCreatureEntryFromDescription(std::string const& description, uint32& entry)
{
    size_t const left = description.find('[');
    if (left == std::string::npos)
        return false;

    size_t const right = description.find(']', left + 1);
    if (right == std::string::npos || right <= left + 1)
        return false;

    std::string const number = description.substr(left + 1, right - left - 1);
    try
    {
        entry = static_cast<uint32>(std::stoul(number));
    }
    catch (...)
    {
        return false;
    }

    return entry != 0;
}

// 可驯服生物召唤卷轴使用脚本
class item_tameable_summon : public ItemScript
{
public:
    item_tameable_summon() : ItemScript("item_tameable_summon") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        // 兜底校验：仅限猎人使用
        if (!player->IsClass(CLASS_HUNTER))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("只有猎人才可以使用该物品。");
            return true; // 阻止默认施法，不消耗物品
        }

        // 只能在大世界中使用（排除副本、战场、竞技场）
        if (player->GetMap()->IsDungeon() || player->GetMap()->IsBattlegroundOrArena())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("该物品只能在野外大世界中使用。");
            return true;
        }

        // 从物品描述中解析出生物 ID
        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("物品数据异常，无法使用。");
            return true;
        }

        uint32 creatureEntry = 0;
        if (!ParseCreatureEntryFromDescription(proto->Description, creatureEntry))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("物品描述中缺少有效的生物 ID，无法使用。");
            return true;
        }

        // 校验生物模板存在且为可驯服类型
        CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(creatureEntry);
        if (!creatureTemplate)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("召唤目标（ID: {}）不存在，请联系管理员。", creatureEntry);
            return true;
        }

        if (!creatureTemplate->IsTameable(true))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("召唤目标（ID: {}）不可驯服，请检查生物模板配置。", creatureEntry);
            return true;
        }

        // 在玩家周围 5 码内随机位置临时召唤野生生物（不写库、无拥有者，猎人可直接驯服）
        Position const summonPos = player->GetRandomPoint(*player, 5.0f);
        if (!player->SummonCreature(creatureEntry, summonPos, TEMPSUMMON_CORPSE_DESPAWN, 600))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("召唤失败，请稍后再试。");
            return true;
        }

        // 消耗一个卷轴
        player->DestroyItemCount(item->GetEntry(), 1, true);

        ChatHandler(player->GetSession()).PSendSysMessage("已在身边召唤出可驯服生物，请尽快驯服！");

        return true; // 已自行处理，阻止默认施法（spellid_1 18282）
    }
};

void AddSC_item_tameable_summon()
{
    new item_tameable_summon();
}
