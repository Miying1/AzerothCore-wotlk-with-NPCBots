#ifndef BOTPOSITIONCONTROL_H
#define BOTPOSITIONCONTROL_H

#include "Define.h"
#include "ObjectGuid.h"

#include <unordered_map>
#include <unordered_set>

class bot_ai;
class BotMgr;
class Creature;
class Unit;
struct Position;

enum class BotMassMode : uint8
{
    None,
    AllNonTank,
    RangedAndHeal,
    SingleTarget     // single specified bot, no role filter
};

class AC_GAME_API BotPositionControl
{
public:
    explicit BotPositionControl(BotMgr& botMgr);

    uint32 EnableMass(BotMassMode mode, float radius);
    uint32 EnableMassForBot(ObjectGuid botGuid, float radius);
    void DisableMass();
    void RemoveMassForBot(ObjectGuid botGuid);
    bool IsMassEnabled() const { return _massMode != BotMassMode::None; }
    BotMassMode GetMassMode() const { return _massMode; }
    float GetMassRadius() const { return _massRadius; }

    bool ShouldFollowMass(Creature const& bot, bot_ai const& ai) const;
    bool CanUpdateMassPosition(Creature& bot, bot_ai const& ai) const;
    bool TryGetMassPosition(Creature const& bot, bot_ai const& ai, Unit const* followUnit, Position& pos, float* speed);
    bool ShouldHoldMassPosition(Creature const& bot, bot_ai const& ai, Unit const* target) const;
    // 该 bot 是否已跑出集合圈（距主人超过集合半径的 1.5 倍）。
    // 用于集合期间动态启用/撤销 BOT_COMMAND_NO_CAST_LONG（禁止读条类技能）。
    bool IsBotOutOfMassRange(Creature const& bot, bot_ai const& ai) const;

    bool EnableSpread(float distance);
    void DisableSpread() { _spreadDistance = 0.0f; }
    bool IsSpreadEnabled() const { return _spreadDistance > 0.0f; }
    float GetSpreadDistance() const { return _spreadDistance; }
    float GetSpreadPenalty(Creature const& bot, Position const& candidate) const;
    bool TryImproveSpreadPosition(Creature const& bot, bot_ai const& ai, Unit const& target,
        float maxOwnerDistance, float attackDistance, Position& position) const;

    void ForgetBot(ObjectGuid botGuid);

private:
    struct BotMassSlot
    {
        float angle = 0.0f;
        float radius = 0.0f;
    };

    struct BotMassDestination
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float ownerX = 0.0f;  // 记录目标点时主人的位置，用于检测主人移动
        float ownerY = 0.0f;
        float ownerZ = 0.0f;
    };

    bool IsMassEligible(Creature const& bot, bot_ai const& ai) const;
    BotMassSlot& GetOrCreateMassSlot(Creature const& bot);
    uint32 CountMassEligible() const;

    BotMgr& _botMgr;
    BotMassMode _massMode = BotMassMode::None;
    ObjectGuid _singleTargetGuid;
    float _massRadius = 4.0f;
    float _spreadDistance = 0.0f;
    std::unordered_map<ObjectGuid, BotMassSlot> _massSlots;
    std::unordered_map<ObjectGuid, BotMassDestination> _massDestinations;
    std::unordered_set<ObjectGuid> _extraMassGuids;  // 通过指定 Bot 单独加入集合的 Guid，不参与模式过滤
};

#endif
