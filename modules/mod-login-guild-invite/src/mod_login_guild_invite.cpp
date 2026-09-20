/*
 * 上线自动公会邀请
 *
 * 角色登录进入世界后，若尚未加入任何公会，则由系统向其发送加入「指定公会」的邀请；
 * 若登录时等级未达门槛，则在之后升级达到门槛时补发邀请。
 * 目标公会通过模块配置（mod_login_guild_invite.conf）中的 LoginGuildInvite.* 项指定：
 *   LoginGuildInvite.Enable    - 是否启用该功能（默认关闭）
 *   LoginGuildInvite.GuildId   - 目标公会 ID（优先使用；为 0 时改用公会名匹配）
 *   LoginGuildInvite.GuildName - 目标公会名称
 *   LoginGuildInvite.MinLevel  - 触发邀请所需的最低角色等级（低于该等级不邀请）
 *   LoginGuildInvite.MaxLevel  - 可被邀请的最高角色等级（高于该等级不邀请；0 表示不设上限）
 * 玩家在客户端确认（CMSG_GUILD_ACCEPT）后才会真正加入公会；拒绝则维持现状。
 * 同一角色每次在线期间最多只邀请一次：玩家拒绝后，本次在线期间的升级等触发不会再重复弹出邀请，
 * 直到其重新上线。
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
#include <unordered_set>

namespace
{
    // 本次在线期间已经发送过邀请的角色，用于保证同一角色每次上线最多只被邀请一次。
    // 该集合为所有玩家共享，而触发时机分散在不同线程：登录/登出在世界主线程，
    // 升级回调则可能在 map 更新线程（MapUpdate.Threads > 1 时多个 map 线程并存），
    // 因此对集合的读写必须加锁，否则会出现数据竞争。
    // 注意锁只保护这个共享容器；SetGuildIdInvited / SendDirectMessage 属于当前 player，
    // 同一角色不会被两个线程同时处理，因此无需纳入锁的范围。
    std::unordered_set<ObjectGuid::LowType> invitedPlayersThisSession;
    std::mutex invitedPlayersThisSessionMutex;

    // 该角色在本次在线期间是否已经收到过邀请
    bool IsInvitedThisSession(ObjectGuid::LowType guidLow)
    {
        std::lock_guard<std::mutex> lock(invitedPlayersThisSessionMutex);
        return invitedPlayersThisSession.contains(guidLow);
    }

    // 标记该角色在本次在线期间已收到邀请
    void MarkInvitedThisSession(ObjectGuid::LowType guidLow)
    {
        std::lock_guard<std::mutex> lock(invitedPlayersThisSessionMutex);
        invitedPlayersThisSession.insert(guidLow);
    }

    // 清除该角色的本次在线标记（上线时重置、下线时回收）
    void ResetInvitedThisSession(ObjectGuid::LowType guidLow)
    {
        std::lock_guard<std::mutex> lock(invitedPlayersThisSessionMutex);
        invitedPlayersThisSession.erase(guidLow);
    }

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

// 无公会角色上线时、以及本次在线期间升级时，尝试向其发送一次加入指定公会的邀请
class LoginGuildInvitePlayerScript : public PlayerScript
{
public:
    LoginGuildInvitePlayerScript()
        : PlayerScript("LoginGuildInvitePlayerScript",
            { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LEVEL_CHANGED, PLAYERHOOK_ON_LOGOUT }) { }

    void OnPlayerLogin(Player* player) override
    {
        // 每次上线重置标记，使本次在线期间可以重新收到一次邀请
        ResetInvitedThisSession(player->GetGUID().GetCounter());
        TryInvite(player);
    }

    // 升级后重新校验：登录时等级未达标的角色，升级达标后可在此刻收到邀请。
    // 本次在线期间已经邀请过的角色会被 TryInvite 直接拦截，不会重复弹出。
    void OnPlayerLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        TryInvite(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        ResetInvitedThisSession(player->GetGUID().GetCounter());
    }

private:
    // 校验角色是否满足邀请条件（本次在线未邀请过、未入会、无待处理邀请、等级在区间内），满足则发送入会邀请
    static void TryInvite(Player* player)
    {
        if (!sConfigMgr->GetOption<bool>("LoginGuildInvite.Enable", false))
            return;

        // 本次在线期间已经邀请过（含玩家已拒绝的情况），不再重复打扰
        if (IsInvitedThisSession(player->GetGUID().GetCounter()))
            return;

        // 已加入公会的角色无需邀请
        if (player->GetGuildId() != 0)
            return;

        // 已存在等待玩家处理的公会邀请时不要重复发送
        if (player->GetGuildIdInvited() != 0)
            return;

        // 等级限制：低于 MinLevel 或高于 MaxLevel（MaxLevel 为 0 时表示不设上限）的角色不被邀请
        if (player->GetLevel() < sConfigMgr->GetOption<uint32>("LoginGuildInvite.MinLevel", 1))
            return;

        uint32 maxLevel = sConfigMgr->GetOption<uint32>("LoginGuildInvite.MaxLevel", 0);
        if (maxLevel != 0 && player->GetLevel() > maxLevel)
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
        MarkInvitedThisSession(player->GetGUID().GetCounter());
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
