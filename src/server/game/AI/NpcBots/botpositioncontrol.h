#ifndef BOTPOSITIONCONTROL_H
#define BOTPOSITIONCONTROL_H

#include "Define.h"
#include "ObjectGuid.h"

#include <unordered_map>

class bot_ai;
class BotMgr;
class Creature;
class Unit;
struct Position;

enum class BotMassMode : uint8
{
    None,
    AllNonTank,
    RangedAndHeal
};

class AC_GAME_API BotPositionControl
{
public:
    explicit BotPositionControl(BotMgr& botMgr);

    uint32 EnableMass(BotMassMode mode, float radius);
    void DisableMass();
    bool IsMassEnabled() const { return _massMode != BotMassMode::None; }
    BotMassMode GetMassMode() const { return _massMode; }
    float GetMassRadius() const { return _massRadius; }

    bool ShouldFollowMass(Creature const& bot, bot_ai const& ai) const;
    bool CanUpdateMassPosition(Creature& bot, bot_ai const& ai) const;
    bool TryGetMassPosition(Creature const& bot, bot_ai const& ai, Unit const* followUnit, Position& pos, float* speed);
    bool ShouldHoldMassPosition(Creature const& bot, bot_ai const& ai, Unit const* target) const;

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
    };

    bool IsMassEligible(Creature const& bot, bot_ai const& ai) const;
    BotMassSlot& GetOrCreateMassSlot(Creature const& bot);
    uint32 CountMassEligible() const;

    BotMgr& _botMgr;
    BotMassMode _massMode = BotMassMode::None;
    float _massRadius = 4.0f;
    float _spreadDistance = 0.0f;
    std::unordered_map<ObjectGuid, BotMassSlot> _massSlots;
    std::unordered_map<ObjectGuid, BotMassDestination> _massDestinations;
};

#endif
