#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "LootScript.h"
#include "InstanceSaveMgr.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "LootMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
#include "Chat.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

namespace
{
constexpr uint32 GoldLootBonusIncrement = 15;
constexpr uint32 MaxGoldLootBonus = 150;

class PlayerVipBenefitsScript : public PlayerScript
{
public:
    PlayerVipBenefitsScript() : PlayerScript("PlayerVipBenefitsScript") { }

    void OnPlayerLogin(Player* player) override
    {
        uint32 accountId = player->GetSession()->GetAccountId();
        QueryResult result = LoginDatabase.Query("SELECT `vip_level`, `gold_loot_bonus`, `skill_max_count` FROM `account_vip` WHERE `account_id` = {}", accountId);
        if (result)
        {
            Field* fields = result->Fetch();
            PlayerVipBenefits& benefits = player->GetVipBenefits();
            benefits.vip_level = fields[0].Get<uint32>();
            benefits.gold_loot_bonus = std::min(fields[1].Get<uint32>(), MaxGoldLootBonus);
            benefits.skill_max_count = fields[2].Get<uint32>();
        }

        player->RecalculatePrimaryProfessionPoints();
        LoginDatabase.Execute("INSERT INTO `account_vip` (`account_id`) VALUES ({}) ON DUPLICATE KEY UPDATE `account_id` = `account_id`", accountId);
    }

    void OnPlayerBeforeLootMoney(Player* player, Loot* loot) override
    {
        if (!loot || !loot->gold)
            return;

        uint32 bonus = player->GetVipBenefits().gold_loot_bonus;
        if (bonus)
            loot->gold = uint32(uint64(loot->gold) * (100 + bonus) / 100);
    }
};

bool TryParseInstanceInfo(std::string const& description, uint32& mapId, Difficulty& difficulty, std::string& modeName)
{
    constexpr std::string_view mapMarker = "MAP:";
    size_t markerPosition = description.find(mapMarker);
    if (markerPosition == std::string::npos)
        return false;

    markerPosition += mapMarker.size();
    if (markerPosition == description.size() || description[markerPosition] < '0' || description[markerPosition] > '9')
        return false;

    mapId = 0;
    while (markerPosition < description.size() && description[markerPosition] >= '0' && description[markerPosition] <= '9')
    {
        mapId = mapId * 10 + uint32(description[markerPosition] - '0');
        ++markerPosition;
    }

    if (markerPosition + 1 >= description.size() || description[markerPosition] != '|')
        return false;

    std::string_view mode(description.data() + markerPosition + 1, description.size() - markerPosition - 1);
    if (mode.starts_with("10PT"))
    {
        difficulty = RAID_DIFFICULTY_10MAN_NORMAL;
        modeName = "10人普通";
    }
    else if (mode.starts_with("10H"))
    {
        difficulty = RAID_DIFFICULTY_10MAN_HEROIC;
        modeName = "10人英雄";
    }
    else if (mode.starts_with("25PT"))
    {
        difficulty = RAID_DIFFICULTY_25MAN_NORMAL;
        modeName = "25人普通";
    }
    else if (mode.starts_with("25H"))
    {
        difficulty = RAID_DIFFICULTY_25MAN_HEROIC;
        modeName = "25人英雄";
    }
    else if (mode.starts_with("5H"))
    {
        difficulty = DUNGEON_DIFFICULTY_HEROIC;
        modeName = "5人英雄";
    }
    else
        return false;

    return true;
}

class PlayerVipGoldBonusItem : public ItemScript
{
public:
    PlayerVipGoldBonusItem() : ItemScript("PlayerVipGoldBonusItem") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (!player || !item)
            return false;

        if (!player->IsAlive() )
        {
            ChatHandler(player->GetSession()).PSendSysMessage("当前状态无法使用该物品。");
            return true;
        }

        uint32 currentBonus = player->GetVipBenefits().gold_loot_bonus;
        if (currentBonus >= MaxGoldLootBonus)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("你的金币拾取倍率加成已经达到上限。");
            return true;
        }

        uint32 newBonus = std::min(currentBonus + GoldLootBonusIncrement, MaxGoldLootBonus);
        player->GetVipBenefits().gold_loot_bonus = newBonus;
        uint32 accountId = player->GetSession()->GetAccountId();
        LoginDatabase.Execute("UPDATE `account_vip` SET `gold_loot_bonus` = {} WHERE `account_id` = {}", newBonus, accountId);
        player->DestroyItemCount(item->GetEntry(), 1, true);
        ChatHandler(player->GetSession()).PSendSysMessage("金币拾取额外加成已增加 {}%，当前额外加成：{}%（账号通用）。", GoldLootBonusIncrement, newBonus);
        return true;
    }

};

class PlayerVipResetInstanceItem : public ItemScript
{
public:
    PlayerVipResetInstanceItem() : ItemScript("PlayerVipResetInstanceItem") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        if (!player || !item || !item->GetTemplate())
            return false;

        if (player->IsInCombat() || !player->IsAlive() || (player->GetMap() && player->GetMap()->GetEntry()->IsDungeon()))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("当前状态无法使用该物品。");
            return true;
        }

        uint32 mapId = 0;
        Difficulty difficulty;
        std::string modeName;
        if (!TryParseInstanceInfo(item->GetTemplate()->Description, mapId, difficulty, modeName))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("物品描述格式错误，模式必须为 10PT、10H、25PT、25H 或 5H，例如 MAP:631|10PT。");
            return true;
        }

        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        if (!mapEntry || !mapEntry->IsDungeon())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("该物品指定的地图不是有效的地下城或团队副本。");
            return true;
        }

        // PlayerGetBoundInstance 返回非 null 表示玩家当前已绑定（有副本 CD）
        bool bound = sInstanceSaveMgr->PlayerGetBoundInstance(player->GetGUID(), mapId, difficulty) != nullptr;
        if (!bound)
        {
            ChatHandler(player->GetSession()).PSendSysMessage("你当前没有该副本的 CD，无法使用此物品。");
            return true;
        }

        sInstanceSaveMgr->PlayerUnbindInstance(player->GetGUID(), mapId, difficulty, true, player);
        player->DestroyItemCount(item->GetEntry(), 1, true);
        ChatHandler(player->GetSession()).PSendSysMessage("{}[{}] 副本已重置。", mapEntry->name[sWorld->GetDefaultDbcLocale()],  modeName);
        return true;
    }
};
}

void AddPlayerVipBenefitsScripts()
{
    new PlayerVipBenefitsScript();
    new PlayerVipGoldBonusItem();
    new PlayerVipResetInstanceItem();
}
