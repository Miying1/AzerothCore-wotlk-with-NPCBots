#include "Chat.h"
#include "Config.h"
#include "CreatureData.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SharedDefines.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "StringFormat.h"
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

class AccountMounts : public PlayerScript
{
private:
    // 法术分类枚举
    enum class SpellCategory : uint8
    {
        NONE = 0,          // 既不是坐骑也不是小宠物
        MOUNT = 1,         // 坐骑
        COMPANION_PET = 2  // 小宠物（同伴宠物）
    };

    bool limitrace;                              // 是否限制种族/职业专属坐骑
    std::set<uint32> excludedSpellIds;           // 需要排除的法术ID集合

public:
    AccountMounts() : PlayerScript("AccountMounts", {
        PLAYERHOOK_ON_LOGIN
    })
    {
        // 从配置文件读取是否限制种族/职业专属坐骑
        limitrace = sConfigMgr->GetOption<bool>("Account.Mounts.LimitRace", false);

        // 从配置文件读取需要排除的法术ID列表（逗号分隔）
        std::string excludedSpellsStr = sConfigMgr->GetOption<std::string>("Account.Mounts.ExcludedSpellIDs", "");
        // 仅当配置不为 "0" 或空时才解析，表示指定了需要排除的法术
        if (excludedSpellsStr != "0" && !excludedSpellsStr.empty())
        {
            std::istringstream spellStream(excludedSpellsStr);
            std::string spellIdStr;
            while (std::getline(spellStream, spellIdStr, ','))
            {
                uint32 spellId = static_cast<uint32>(std::stoul(spellIdStr));
                if (spellId != 0) // 0 表示不排除任何法术，跳过
                    excludedSpellIds.insert(spellId);
            }
        }
    }

    /// 判断法术的分类（坐骑 / 小宠物 / 其他）
    /// 遍历法术的所有效果槽位：
    ///   - 存在 APPLY_AURA + MOUNTED 效果 => 坐骑
    ///   - 存在 SUMMON 效果且所召唤生物为小动物(CRITTER)或非战斗宠物(NON_COMBAT_PET) => 小宠物
    SpellCategory ClassifySpell(uint32 spellId)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
            return SpellCategory::NONE;

        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            SpellEffectInfo const& effect = spellInfo->Effects[i];

            // 判断是否为坐骑效果
            if (effect.Effect == SPELL_EFFECT_APPLY_AURA && effect.ApplyAuraName == SPELL_AURA_MOUNTED)
                return SpellCategory::MOUNT;

            // 判断是否为小宠物召唤效果
            if (effect.Effect == SPELL_EFFECT_SUMMON && effect.MiscValue > 0)
            {
                uint32 creatureEntry = static_cast<uint32>(effect.MiscValue);
                CreatureTemplate const* creatureTemplate = sObjectMgr->GetCreatureTemplate(creatureEntry);
                if (creatureTemplate &&
                    (creatureTemplate->type == CREATURE_TYPE_CRITTER ||
                     creatureTemplate->type == CREATURE_TYPE_NON_COMBAT_PET))
                    return SpellCategory::COMPANION_PET;
            }
        }

        return SpellCategory::NONE;
    }

    void OnPlayerLogin(Player* pPlayer)
    {
        if (!sConfigMgr->GetOption<bool>("Account.Mounts.Enable", true))
            return;

        if (sConfigMgr->GetOption<bool>("Account.Mounts.Announce", false))
            ChatHandler(pPlayer->GetSession()).SendSysMessage("This server is running the |cff4CFF00AccountMounts |rmodule.");

        uint32 accountId = pPlayer->GetSession()->GetAccountId();
        uint32 currentRace = pPlayer->getRace();
        bool limitRace = limitrace;

        // 捕获 WorldSession 而非 Player，避免 DB 响应返回前玩家下线导致悬空指针
        WorldSession* session = pPlayer->GetSession();

        // 1. 异步查询账号下所有角色及其种族
        std::string charactersSql = Acore::StringFormat(
            "SELECT `guid`, `race` FROM `characters` WHERE `account`={};", accountId);

        session->GetQueryProcessor().AddCallback(CharacterDatabase.AsyncQuery(charactersSql)
            .WithChainingCallback([this, session, currentRace, limitRace](QueryCallback& queryCallback, QueryResult charactersResult)
            {
                // 2. 根据种族限制过滤出需要同步的角色
                std::vector<uint32> guids;
                if (charactersResult)
                {
                    do
                    {
                        Field* fields = charactersResult->Fetch();
                        uint32 race = fields[1].Get<uint8>();

                        if (!limitRace || Player::TeamIdForRace(race) == Player::TeamIdForRace(currentRace))
                            guids.push_back(fields[0].Get<uint32>());

                    } while (charactersResult->NextRow());
                }

                if (guids.empty())
                    return;

                // 3. 构造 IN 子句，一次性查询所有角色的法术（消除 N+1 查询）
                std::string guidList;
                for (uint32 guid : guids)
                {
                    if (!guidList.empty())
                        guidList += ',';
                    guidList += std::to_string(guid);
                }

                std::string spellsSql = Acore::StringFormat(
                    "SELECT `spell` FROM `character_spell` WHERE `guid` IN ({});", guidList);
                queryCallback.SetNextQuery(CharacterDatabase.AsyncQuery(spellsSql));
            })
            .WithChainingCallback([this, session](QueryCallback& /*queryCallback*/, QueryResult spellsResult)
            {
                if (!spellsResult)
                    return;

                // 重新获取 Player，判空防止玩家已下线
                Player* player = session->GetPlayer();
                if (!player)
                    return;

                // 4. 用集合去重（多个角色可能共享相同的法术）
                std::unordered_set<uint32> spells;
                do
                {
                    spells.insert(spellsResult->Fetch()[0].Get<uint32>());
                } while (spellsResult->NextRow());

                // 5. 遍历去重后的法术，分类并学习
                for (uint32 spellId : spells)
                {
                    // 跳过排除列表中的法术
                    if (excludedSpellIds.find(spellId) != excludedSpellIds.end())
                        continue;

                    if (ClassifySpell(spellId) != SpellCategory::NONE)
                        player->learnSpell(spellId);
                }
            }));
    }
};

void AddAccountMountsScripts()
{
    new AccountMounts();
}
