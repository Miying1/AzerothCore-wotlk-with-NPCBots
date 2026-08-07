#include "botpositioncontrol.h"

#include "bot_ai.h"
#include "botcommon.h"
#include "botmgr.h"
#include "Creature.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Unit.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
constexpr float MIN_MASS_RADIUS = 1.0f;
constexpr float MAX_MASS_RADIUS = 4.0f;
constexpr float MIN_SPREAD_DISTANCE = 2.0f;
constexpr float MAX_SPREAD_DISTANCE = 20.0f;
constexpr float GOLDEN_ANGLE = 2.39996323f;

float GetStableUnitFloat(uint64 seed)
{
    seed ^= seed >> 33;
    seed *= 0xff51afd7ed558ccdULL;
    seed ^= seed >> 33;
    seed *= 0xc4ceb9fe1a85ec53ULL;
    seed ^= seed >> 33;
    return float(seed & 0x00FFFFFFu) / float(0x01000000u);
}
}

BotPositionControl::BotPositionControl(BotMgr& botMgr) : _botMgr(botMgr) { }

uint32 BotPositionControl::EnableMass(BotMassMode mode, float radius)
{
    _massMode = mode;
    _massRadius = std::clamp(radius, MIN_MASS_RADIUS, MAX_MASS_RADIUS);

    for (auto itr = _massSlots.begin(); itr != _massSlots.end();)
    {
        Creature* bot = _botMgr.GetBot(itr->first);
        bot_ai* ai = bot ? bot->GetBotAI() : nullptr;
        if (!bot || !ai || !IsMassEligible(*bot, *ai))
        {
            _massDestinations.erase(itr->first);
            itr = _massSlots.erase(itr);
        }
        else
        {
            itr->second.radius = std::min(itr->second.radius, _massRadius);
            ++itr;
        }
    }

    return CountMassEligible();
}

void BotPositionControl::DisableMass()
{
    _massMode = BotMassMode::None;
    _massSlots.clear();
    _massDestinations.clear();
}

bool BotPositionControl::ShouldFollowMass(Creature const& bot, bot_ai const& ai) const
{
    if (!IsMassEligible(bot, ai))
        return false;
    if (ai.IsDuringTeleport() || bot.GetVehicle() || bot.IsFalling() || bot.GetMap()->IsBattlegroundOrArena())
        return false;
    if (bot_ai::CCed(&bot, true))
        return false;
    if (!ai.GetAoeSpots().empty() && ai.IsBotPositionWithinAoE(bot.GetPosition()))
        return false;

    return true;
}

bool BotPositionControl::CanUpdateMassPosition(Creature& bot, bot_ai const& ai) const
{
    if (!ShouldFollowMass(bot, ai))
        return false;

    switch (bot.GetMotionMaster()->GetCurrentMovementGeneratorType())
    {
        case IDLE_MOTION_TYPE:
        case CHASE_MOTION_TYPE:
            return true;
        case POINT_MOTION_TYPE:
        {
            auto itr = _massDestinations.find(bot.GetGUID());
            if (itr == _massDestinations.end())
                return false;

            Position destination;
            if (!bot.GetMotionMaster()->GetDestination(destination.m_positionX, destination.m_positionY, destination.m_positionZ))
                return false;

            Position recorded(itr->second.x, itr->second.y, itr->second.z);
            return destination.GetExactDist(recorded) < 1.0f;
        }
        default:
            return false;
    }
}

bool BotPositionControl::TryGetMassPosition(Creature const& bot, bot_ai const& ai, Unit const* followUnit, Position& pos, float* speed)
{
    Player const* owner = _botMgr.GetOwner();
    if (!owner || followUnit != owner || !ShouldFollowMass(bot, ai))
        return false;
    if (!owner->IsAlive() || !owner->IsInWorld() || !bot.IsInMap(owner) || !bot.InSamePhase(owner))
        return false;

    BotMassSlot& slot = GetOrCreateMassSlot(bot);
    Unit const* mover = owner->GetVehicle() ? owner->GetVehicleBase() : owner;
    float angle = Position::NormalizeOrientation(slot.angle);
    Position massPosition = mover->GetFirstCollisionPosition(slot.radius, angle);

    if (!owner->IsWithinLOS(massPosition.GetPositionX(), massPosition.GetPositionY(), massPosition.GetPositionZ()) || ai.IsBotPositionWithinAoE(massPosition))
    {
        massPosition = mover->GetFirstCollisionPosition(std::max(0.5f, slot.radius * 0.5f), angle);
        if (!owner->IsWithinLOS(massPosition.GetPositionX(), massPosition.GetPositionY(), massPosition.GetPositionZ()) || ai.IsBotPositionWithinAoE(massPosition))
            return false;
    }

    if (bot.GetPositionZ() < massPosition.GetPositionZ())
        massPosition.m_positionZ += 0.5f;

    if (speed)
    {
        Unit const* botMover = bot.GetVehicle() ? bot.GetVehicleBase() : &bot;
        float positionDistance = botMover->GetDistance(massPosition);
        if (owner->IsWalking() || ai.HasBotCommandState(BOT_COMMAND_WALK))
        {
            float baseWalkSpeed = botMover->GetSpeed(MOVE_WALK);
            *speed = baseWalkSpeed;
            if (!ai.HasBotCommandState(BOT_COMMAND_WALK) && positionDistance > 10.0f && botMover->GetDistance(owner) > 10.0f)
                *speed = botMover->GetSpeed(MOVE_RUN);
            else if (positionDistance > 7.5f)
                *speed = baseWalkSpeed * 1.15f;
        }
        else
        {
            float baseRunSpeed = botMover->GetSpeed(MOVE_RUN);
            if (positionDistance > 50.0f)
                *speed = baseRunSpeed * 2.0f;
            else if (positionDistance > 30.0f)
                *speed = baseRunSpeed * 1.5f;
            else if (positionDistance > 10.0f)
                *speed = baseRunSpeed * 1.25f;
        }
    }

    pos.Relocate(massPosition);
    _massDestinations[bot.GetGUID()] = { pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ() };
    return true;
}

bool BotPositionControl::ShouldHoldMassPosition(Creature const& bot, bot_ai const& ai, Unit const* target) const
{
    if (!target || !ShouldFollowMass(bot, ai))
        return false;
    if (!bot.IsValidAttackTarget(target))
        return false;

    bool inLineOfSight = bot.IsWithinLOSInMap(target, VMAP::ModelIgnoreFlags::M2, LINEOFSIGHT_ALL_CHECKS);
    if (ai.HasRole(BOT_ROLE_RANGED) || ai.HasRole(BOT_ROLE_HEAL))
    {
        float attackRange = ai.GetMassAttackRange();
        if (attackRange <= 0.0f)
            return !inLineOfSight || !bot.IsWithinMeleeRange(target);

        return !inLineOfSight || bot.GetDistance(target) > attackRange;
    }

    return !inLineOfSight || !bot.IsWithinMeleeRange(target);
}

bool BotPositionControl::EnableSpread(float distance)
{
    if (!std::isfinite(distance) || distance < MIN_SPREAD_DISTANCE || distance > MAX_SPREAD_DISTANCE)
        return false;

    _spreadDistance = distance;
    return true;
}

float BotPositionControl::GetSpreadPenalty(Creature const& bot, Position const& candidate) const
{
    if (!IsSpreadEnabled() || !bot.IsInCombat())
        return 0.0f;

    float penalty = 0.0f;
    for (auto const& [_, other] : *_botMgr.GetBotMap())
    {
        if (!other || other == &bot || !other->IsInWorld() || !other->IsAlive())
            continue;
        if (!bot.IsInMap(other) || !bot.InSamePhase(other))
            continue;

        float distance = other->GetExactDist2d(candidate);
        if (distance < _spreadDistance)
        {
            float deficit = _spreadDistance - distance;
            penalty += deficit * deficit;
        }
    }

    float moveDistance = bot.GetExactDist2d(candidate);
    penalty += moveDistance * 0.05f;
    return penalty;
}

bool BotPositionControl::TryImproveSpreadPosition(Creature const& bot, bot_ai const& ai, Unit const& target,
    float maxOwnerDistance, float attackDistance, Position& position) const
{
    if (!IsSpreadEnabled() || !bot.IsInCombat() || ai.HasRole(BOT_ROLE_TANK) || ai.IsTank())
        return false;

    Player const* owner = _botMgr.GetOwner();
    if (!owner || !bot.IsInMap(owner) || !bot.InSamePhase(owner))
        return false;

    Position bestPosition = position;
    float bestPenalty = GetSpreadPenalty(bot, position);
    float baseAngle = target.GetAbsoluteAngle(position.GetPositionX(), position.GetPositionY());
    for (int8 direction : std::array<int8, 2>{ -1, 1 })
    {
        for (uint8 step = 1; step <= 3; ++step)
        {
            float angle = Position::NormalizeOrientation(baseAngle + direction * step * float(M_PI) / 12.0f);
            Position candidate = target.GetFirstCollisionPosition(attackDistance, Position::NormalizeOrientation(angle - target.GetOrientation()));
            if (owner->GetDistance(candidate) > maxOwnerDistance || ai.IsBotPositionWithinAoE(candidate))
                continue;
            if (!target.IsWithinLOS(candidate.GetPositionX(), candidate.GetPositionY(), candidate.GetPositionZ()))
                continue;
            if (ai.HasRole(BOT_ROLE_RANGED) ? target.GetDistance(candidate) - bot.GetCombatReach() > attackDistance :
                !bot.IsWithinMeleeRangeAt(candidate, &target))
                continue;

            float penalty = GetSpreadPenalty(bot, candidate);
            if (penalty + 0.25f < bestPenalty)
            {
                bestPosition.Relocate(candidate);
                bestPenalty = penalty;
            }
        }
    }

    if (bestPosition.GetExactDist2d(position) < 0.1f)
        return false;

    position.Relocate(bestPosition);
    return true;
}

void BotPositionControl::ForgetBot(ObjectGuid botGuid)
{
    _massSlots.erase(botGuid);
    _massDestinations.erase(botGuid);
}

bool BotPositionControl::IsMassEligible(Creature const& bot, bot_ai const& ai) const
{
    if (_massMode == BotMassMode::None || !bot.IsInWorld() || !bot.IsAlive())
        return false;
    if (ai.IAmFree() || ai.IsWanderer() || ai.IsTempBot())
        return false;
    if (ai.HasRole(BOT_ROLE_TANK) || ai.IsTank())
        return false;
    if (ai.HasBotCommandState(BOT_COMMAND_STAY | BOT_COMMAND_FULLSTOP | BOT_COMMAND_INACTION))
        return false;
    if (_massMode == BotMassMode::RangedAndHeal && !ai.HasRole(BOT_ROLE_RANGED | BOT_ROLE_HEAL))
        return false;

    return ai.GetBotOwner() == _botMgr.GetOwner();
}

BotPositionControl::BotMassSlot& BotPositionControl::GetOrCreateMassSlot(Creature const& bot)
{
    auto [itr, inserted] = _massSlots.try_emplace(bot.GetGUID());
    if (!inserted)
        return itr->second;

    uint64 seed = bot.GetGUID().GetRawValue() ^ (_botMgr.GetOwner()->GetGUID().GetRawValue() << 1);
    seed ^= uint64(_massMode) << 57;
    float angleJitter = GetStableUnitFloat(seed) * 0.75f;
    float radiusValue = GetStableUnitFloat(seed ^ 0x9e3779b97f4a7c15ULL);

    itr->second.angle = Position::NormalizeOrientation(float(_massSlots.size() - 1u) * GOLDEN_ANGLE + angleJitter);
    itr->second.radius = std::max(0.5f, std::sqrt(radiusValue) * _massRadius);
    return itr->second;
}

uint32 BotPositionControl::CountMassEligible() const
{
    uint32 count = 0;
    for (auto const& [_, bot] : *_botMgr.GetBotMap())
    {
        bot_ai const* ai = bot ? bot->GetBotAI() : nullptr;
        if (bot && ai && IsMassEligible(*bot, *ai))
            ++count;
    }
    return count;
}
