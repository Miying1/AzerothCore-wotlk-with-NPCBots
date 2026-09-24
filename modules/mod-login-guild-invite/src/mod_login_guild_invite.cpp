/*
 * 升级自动公会邀请
 *
 * 角色升级到指定等级（默认 35 级）时，若尚未加入任何公会，则由系统向其发送加入「指定公会」的邀请。
 * 由于角色等级只会上升、不会下降，因此每个角色只会经过一次目标等级，
 * 天然保证每个角色仅触发一次，无需额外缓存已邀请数据。
 * 目标公会通过模块配置（mod_login_guild_invite.conf）中的 LoginGuildInvite.* 项指定：
 *   LoginGuildInvite.Enable    - 是否启用该功能（默认关闭）
 *   LoginGuildInvite.GuildId   - 目标公会 ID（优先使用；为 0 时改用公会名匹配）
 *   LoginGuildInvite.GuildName - 目标公会名称
 *   LoginGuildInvite.Level     - 触发邀请的等级（角色升级到该等级时触发，默认 35）
 * 玩家在客户端确认（CMSG_GUILD_ACCEPT）后才会真正加入公会；拒绝则维持现状。
 */

#include "CharacterCache.h"
#include "Config.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "GuildPackets.h"
#include "Log.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include <mutex>

namespace
{
    // 目标公会的解析结果缓存：配置在运行期不会变化，因此只在首次使用时解析一次，
    // 配置有误时也仅在首次解析时打印一次警告，避免每次登录/升级都刷日志。
    // 该缓存同样会被主线程与 map 更新线程并发访问，所以读写要加锁。
    // 这里缓存的是公会 ID 而不是 Guild*，因为公会可能在运行期被解散，缓存指针会悬空。
    struct TargetGuildCache
    {
        bool resolved = false;  // 是否已经完成首次解析
        uint32 guildId = 0;     // 0 表示未配置或找不到目标公会，功能不生效
    };

    TargetGuildCache targetGuildCache;
    std::mutex targetGuildCacheMutex;

    // 依据配置解析目标公会 ID：优先 GuildId，其次 GuildName。
    // 返回 0 表示未配置或找不到目标公会，调用方应跳过邀请。
    uint32 ResolveTargetGuildId()
    {
        // 首次解析在锁内完成，保证并发场景下也只解析一次、只警告一次；
        // 解析完成后后续调用直接命中缓存，不再进入该分支。
        std::lock_guard<std::mutex> lock(targetGuildCacheMutex);

        if (targetGuildCache.resolved)
            return targetGuildCache.guildId;

        uint32 guildId = sConfigMgr->GetOption<uint32>("LoginGuildInvite.GuildId", 0);
        if (guildId)
        {
            if (!sGuildMgr->GetGuildById(guildId))
            {
                LOG_WARN("module.login_guild_invite", "LoginGuildInvite.GuildId = {} 未找到对应公会，功能不生效", guildId);
                guildId = 0;
            }
        }
        else
        {
            std::string guildName = sConfigMgr->GetOption<std::string>("LoginGuildInvite.GuildName", "");
            if (guildName.empty())
                LOG_WARN("module.login_guild_invite", "LoginGuildInvite.Enable 已开启，但未配置 GuildId/GuildName，功能不生效");
            else if (Guild* guild = sGuildMgr->GetGuildByName(guildName))
                guildId = guild->GetId();
            else
                LOG_WARN("module.login_guild_invite", "LoginGuildInvite.GuildName = {} 未找到对应公会，功能不生效", guildName);
        }

        targetGuildCache.resolved = true;
        targetGuildCache.guildId = guildId;
        return guildId;
    }
}

// 角色升级到目标等级时，尝试向其发送一次加入指定公会的邀请
class LoginGuildInvitePlayerScript : public PlayerScript
{
public:
    LoginGuildInvitePlayerScript()
        : PlayerScript("LoginGuildInvitePlayerScript",
            { PLAYERHOOK_ON_LEVEL_CHANGED }) { }

    // 仅在升级回调中触发：角色升级到目标等级（默认 35 级）时发送邀请。
    // 由于等级只升不降，每个角色只会经过一次目标等级，天然保证只触发一次。
    void OnPlayerLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        TryInvite(player);
    }

private:
    // 校验角色是否满足邀请条件（刚升级到目标等级、未入会、无待处理邀请），满足则发送入会邀请
    static void TryInvite(Player* player)
    {
        if (!sConfigMgr->GetOption<bool>("LoginGuildInvite.Enable", false))
            return;

        // 只在角色刚升级到目标等级时触发（此处 player->GetLevel() 为升级后的新等级）
        if (player->GetLevel() != sConfigMgr->GetOption<uint32>("LoginGuildInvite.Level", 35))
            return;

        // 已加入公会的角色无需邀请
        if (player->GetGuildId() != 0)
            return;

        // 已存在等待玩家处理的公会邀请时不要重复发送
        if (player->GetGuildIdInvited() != 0)
            return;

        uint32 guildId = ResolveTargetGuildId();
        if (!guildId)
            return;

        // 缓存中只保存了公会 ID，公会可能在运行期被解散，此处按需解析最新对象
        Guild* guild = sGuildMgr->GetGuildById(guildId);
        if (!guild)
            return;

        // 邀请弹窗中显示的发起人，优先使用公会会长名，取不到时退化为公会名
        std::string inviterName = guild->GetName();
        sCharacterCache->GetCharacterNameByGuid(guild->GetLeaderGUID(), inviterName);

        WorldPackets::Guild::GuildInvite invite;
        invite.InviterName = inviterName;
        invite.GuildName = guild->GetName();

        // 客户端接受邀请时会依据此处记录的邀请公会 ID 定位公会，因此必须先设置再发包
        player->SetGuildIdInvited(guildId);
        player->SendDirectMessage(invite.Write());

        LOG_INFO("module.login_guild_invite", "已向角色 {} 发送加入公会 '[{}]' 的邀请",
            player->GetName(), guild->GetName());
    }
};

void Addmod_login_guild_inviteScripts()
{
    new LoginGuildInvitePlayerScript();
}
