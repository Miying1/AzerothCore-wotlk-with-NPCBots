/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "GameObject.h"
#include "MotionMaster.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string_view>

namespace HeroicDungeonRift
{
namespace
{
// 原版/T1组合战：莫格莱尼首次致死时假死，怀特迈恩入场；她半血沉睡全场并复活莫格莱尼，随后双人作战。
enum MograineEvents : uint32
{
    EventMograineCrusaderStrike = 1, // 十字军打击（原版/T1基础）
    EventMograineHammerOfJustice,    // 制裁之锤（原版/T1基础）
    EventMograineResurrectionYell    // 复活台词（原版/T1流程）
};

enum WhitemaneEvents : uint32
{
    EventWhitemaneSmite = 1,        // 神圣惩击（原版/T1基础）
    EventWhitemaneShield,           // 真言术：盾（原版/T1基础）
    EventWhitemaneHeal,             // 治疗术（原版/T1复活后阶段）
    EventWhitemaneDominateMind,     // 统御意志（原版/T1复活后阶段）
    EventWhitemaneResurrectMograine,// 血色复活（原版/T1组合战流程）
    EventWhitemaneResurrectionYell  // 复活台词（原版/T1流程）
};

enum Spells : uint32
{
    SpellRetributionAura = 8990,     // 惩戒光环（原版/T1基础）
    SpellCrusaderStrike = 14518,     // 十字军打击（原版/T1基础）
    SpellHammerOfJustice = 5589,     // 制裁之锤（原版/T1基础）
    SpellSmite = 9481,               // 神圣惩击（原版/T1基础）
    SpellPowerWordShield = 22187,    // 真言术：盾（原版/T1基础）
    SpellHeal = 12039,               // 治疗术（原版/T1复活后阶段）
    SpellDominateMind = 14515,       // 统御意志（原版/T1复活后阶段）
    SpellDeepSleep = 9256,           // 深度睡眠（原版/T1复活流程）
    SpellScarletResurrection = 9232, // 血色复活（原版/T1复活流程）
    SpellLayOnHands = 9257           // 圣疗术（原版/T1；莫格莱尼复活后施放）
};

enum Actions : int32
{
    ActionBeginWhitemaneEncounter = 1
};

enum WhitemaneMovementPoints : uint32
{
    PointWhitemaneEntrance = 1,
    PointWhitemaneResurrection
};

enum WhitemanePhase : uint8
{
    WhitemaneDormant,
    WhitemaneMovingToMograine,
    WhitemaneCombatPhaseOne,
    WhitemaneMovingForResurrection,
    WhitemaneResurrecting,
    WhitemaneCombatPhaseTwo
};

Position const WhitemaneSpawnPosition = { 1202.13f, 1399.07f, 29.0931f, 3.12414f };
constexpr float MograineApproachDistance = 3.0f;
constexpr uint32 WhitemaneDoorEntry = 104600;
constexpr float WhitemaneDoorSearchRange = 100.0f;

constexpr std::string_view MograineAggroText = "异教徒，净化他们！";
constexpr std::string_view MograineKillText = "无能！";
constexpr std::string_view MograineResurrectedText = "为你而战，我的女士！";
constexpr std::string_view WhitemaneIntroText = "莫格莱尼倒下了？你们要为此付出代价！复活吧，我的勇士！复活吧！";
constexpr std::string_view WhitemaneKillText = "圣光会审判你！";
constexpr std::string_view WhitemaneResurrectText = "复活吧，我的勇士！";

constexpr uint32 SoundMograineAggro = 5835;
constexpr uint32 SoundMograineKill = 5836;
constexpr uint32 SoundMograineResurrected = 5837;
constexpr uint32 SoundWhitemaneIntro = 5838;
constexpr uint32 SoundWhitemaneKill = 5839;
constexpr uint32 SoundWhitemaneResurrect = 5840;

enum GuidData : int32
{
    GuidMograine = 1,
    GuidCombatTarget
};
}

struct npc_rift_whitemane : public ScriptedAI
{
    explicit npc_rift_whitemane(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _mograineGuid.Clear();
        _combatTargetGuid.Clear();
        _tier = 1;
        _damagePermille = 1000;
        _resurrectionStarted = false;
        _phase = WhitemaneDormant;
        me->SetStandState(UNIT_STAND_STATE_STAND);
        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
    }

    void SetData(uint32 type, uint32 data) override
    {
        if (type == RiftDataTier)
            _tier = uint8(std::clamp<uint32>(data, 1, MaxTier));
        else if (type == RiftDataDamagePermille)
            _damagePermille = std::max<uint32>(1, data) * 15;
    }

    void SetGUID(ObjectGuid const& guid, int32 type) override
    {
        if (type == GuidMograine)
            _mograineGuid = guid;
        else if (type == GuidCombatTarget)
            _combatTargetGuid = guid;
    }

    void DoAction(int32 action) override
    {
        if (action != ActionBeginWhitemaneEncounter || _phase != WhitemaneDormant)
            return;

        Creature* mograine = ObjectAccessor::GetCreature(*me, _mograineGuid);
        if (!mograine || !mograine->IsAlive())
            return;

        if (GameObject* door = me->FindNearestGameObject(WhitemaneDoorEntry, WhitemaneDoorSearchRange))
            door->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);

        YellWithSound(WhitemaneIntroText, SoundWhitemaneIntro);
        _phase = WhitemaneMovingToMograine;
        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        me->GetMotionMaster()->MovePoint(PointWhitemaneEntrance, GetApproachPosition(*mograine));
    }

    void MovementInform(uint32 type, uint32 pointId) override
    {
        if (type != POINT_MOTION_TYPE)
            return;

        if (pointId == PointWhitemaneEntrance && _phase == WhitemaneMovingToMograine)
        {
            _phase = WhitemaneCombatPhaseOne;
            me->SetInCombatWithZone();
            ResumeCombat();
        }
        else if (pointId == PointWhitemaneResurrection && _phase == WhitemaneMovingForResurrection)
        {
            _phase = WhitemaneResurrecting;
            _events.ScheduleEvent(EventWhitemaneResurrectMograine, 4500ms);
            _events.ScheduleEvent(EventWhitemaneResurrectionYell, 1900ms);
        }
    }

    void SpellHitTarget(Unit* target, SpellInfo const* spell) override
    {
        if (_phase != WhitemaneResurrecting || !target || !spell || spell->Id != SpellScarletResurrection ||
            target->GetGUID() != _mograineGuid)
            return;

        _phase = WhitemaneCombatPhaseTwo;
        ResumeCombat();
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_resurrectionStarted || !me->HealthBelowPctDamaged(50, damage))
            return;

        _resurrectionStarted = true;
        if (damage >= me->GetHealth())
            damage = me->GetHealth() - 1;

        if (Unit* victim = me->GetVictim())
            _combatTargetGuid = victim->GetGUID();

        _events.Reset();
        me->AttackStop();
        me->GetMotionMaster()->Clear(false);
        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        DoCast(me, SpellDeepSleep);

        if (Creature* mograine = ObjectAccessor::GetCreature(*me, _mograineGuid))
        {
            _phase = WhitemaneMovingForResurrection;
            me->GetMotionMaster()->MovePoint(PointWhitemaneResurrection, GetApproachPosition(*mograine));
        }
        else
            me->KillSelf();
    }

    void DamageDealt(Unit* /*victim*/, uint32& damage, DamageEffectType damageType,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (damageType == DIRECT_DAMAGE)
            return;

        uint64 scaledDamage = uint64(damage) * _damagePermille / 1000;
        damage = uint32(std::min<uint64>(scaledDamage, std::numeric_limits<uint32>::max()));
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
            YellWithSound(WhitemaneKillText, SoundWhitemaneKill, victim);
    }

    void JustDied(Unit* /*killer*/) override
    {
        _events.Reset();
        if (Creature* mograine = ObjectAccessor::GetCreature(*me, _mograineGuid))
            if (mograine->IsAlive())
                mograine->KillSelf();
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = _events.ExecuteEvent())
        {
            if (eventId == EventWhitemaneResurrectMograine)
            {
                if (Creature* mograine = ObjectAccessor::GetCreature(*me, _mograineGuid))
                    DoCast(mograine, SpellScarletResurrection);
                else
                    me->KillSelf();
                continue;
            }

            if (eventId == EventWhitemaneResurrectionYell)
            {
                if (_phase == WhitemaneResurrecting || _phase == WhitemaneCombatPhaseTwo)
                    YellWithSound(WhitemaneResurrectText, SoundWhitemaneResurrect);
                continue;
            }

            if (!UpdateVictim())
                continue;

            switch (eventId)
            {
                case EventWhitemaneSmite:
                    DoCastVictim(SpellSmite);
                    _events.ScheduleEvent(EventWhitemaneSmite, Milliseconds(6000));
                    break;
                case EventWhitemaneShield:
                    DoCast(me, SpellPowerWordShield);
                    _events.ScheduleEvent(EventWhitemaneShield, Milliseconds(18000));
                    break;
                case EventWhitemaneHeal:
                    if (me->HealthBelowPct(80))
                        DoCast(me, SpellHeal);
                    _events.ScheduleEvent(EventWhitemaneHeal, Milliseconds(12000));
                    break;
                case EventWhitemaneDominateMind:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                        DoCast(target, SpellDominateMind);
                    _events.ScheduleEvent(EventWhitemaneDominateMind, Milliseconds(24000));
                    break;
                default:
                    break;
            }
        }

        if (UpdateVictim())
            DoMeleeAttackIfReady();
    }

private:
    void YellWithSound(std::string_view text, uint32 soundId, WorldObject const* target = nullptr)
    {
        me->Yell(text, LANG_UNIVERSAL, target);
        me->PlayDirectSound(soundId);
    }

    void ScheduleCombatEvents()
    {
        _events.ScheduleEvent(EventWhitemaneSmite, Milliseconds(3000));
        _events.ScheduleEvent(EventWhitemaneShield, Milliseconds(9000));
        if (_phase == WhitemaneCombatPhaseTwo)
        {
            _events.ScheduleEvent(EventWhitemaneHeal, Milliseconds(7000));
            _events.ScheduleEvent(EventWhitemaneDominateMind, Milliseconds(14000));
        }
    }

    Position GetApproachPosition(Creature const& mograine) const
    {
        Position position = mograine.GetPosition();
        float angle = mograine.GetAbsoluteAngle(me);
        position.Relocate(
            mograine.GetPositionX() + MograineApproachDistance * std::cos(angle),
            mograine.GetPositionY() + MograineApproachDistance * std::sin(angle),
            mograine.GetPositionZ(),
            Position::NormalizeOrientation(angle + float(M_PI)));
        return position;
    }

    void ResumeCombat()
    {
        me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        me->SetReactState(REACT_AGGRESSIVE);
        ScheduleCombatEvents();

        Unit* target = ObjectAccessor::GetUnit(*me, _combatTargetGuid);
        if (!target || !target->IsAlive())
            target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true);
        if (target)
        {
            _combatTargetGuid = target->GetGUID();
            AttackStart(target);
        }
    }

    EventMap _events;
    ObjectGuid _mograineGuid;
    ObjectGuid _combatTargetGuid;
    uint8 _tier = 1;
    uint32 _damagePermille = 1000;
    bool _resurrectionStarted = false;
    WhitemanePhase _phase = WhitemaneDormant;
};

struct boss_rift_mograine : public BossAIBase
{
    explicit boss_rift_mograine(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        me->SetStandState(UNIT_STAND_STATE_STAND);
        me->SetReactState(REACT_AGGRESSIVE);
        me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        BossAIBase::Reset();
        _whitemaneGuid.Clear();
        _combatTargetGuid.Clear();
        _fakeDeath = false;
        _resurrected = false;
    }

    void JustEngagedWith(Unit* who) override
    {
        if (who)
            _combatTargetGuid = who->GetGUID();
        YellWithSound(MograineAggroText, SoundMograineAggro, who);
        CastIfConfigured(me, SpellRetributionAura, true);
        ScheduleCombatEvents();
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_fakeDeath || _resurrected || damage < me->GetHealth())
            return;

        damage = me->GetHealth() - 1;
        if (Unit* victim = me->GetVictim())
            _combatTargetGuid = victim->GetGUID();

        _fakeDeath = true;
        events.Reset();
        me->AttackStop();
        DoResetThreatList();
        me->SetReactState(REACT_PASSIVE);
        me->SetStandState(UNIT_STAND_STATE_DEAD);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        SummonWhitemane();
    }

    void SpellHit(Unit* caster, SpellInfo const* spell) override
    {
        if (!_fakeDeath || !caster || !spell || spell->Id != SpellScarletResurrection ||
            caster->GetGUID() != _whitemaneGuid)
            return;

        _fakeDeath = false;
        _resurrected = true;
        me->SetStandState(UNIT_STAND_STATE_STAND);
        me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        me->SetReactState(REACT_AGGRESSIVE);
        me->SetFullHealth();
        CastIfConfigured(caster, SpellLayOnHands, true);
        CastIfConfigured(me, SpellRetributionAura, true);
        events.ScheduleEvent(EventMograineResurrectionYell, 1s);
        ScheduleCombatEvents();

        Unit* target = ObjectAccessor::GetUnit(*me, _combatTargetGuid);
        if ((!target || !target->IsAlive()) && caster->GetVictim())
            target = caster->GetVictim();
        if (target && target->IsAlive())
        {
            _combatTargetGuid = target->GetGUID();
            AttackStart(target);
        }
        else
            me->SetInCombatWithZone();
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
            YellWithSound(MograineKillText, SoundMograineKill, victim);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (Creature* whitemane = ObjectAccessor::GetCreature(*me, _whitemaneGuid))
            if (whitemane->IsAlive())
                whitemane->KillSelf();
        BossAIBase::JustDied(nullptr);
        _whitemaneGuid.Clear();
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventMograineResurrectionYell:
                YellWithSound(MograineResurrectedText, SoundMograineResurrected);
                break;
            case EventMograineCrusaderStrike:
                CastIfConfigured(me->GetVictim(), SpellCrusaderStrike);
                events.ScheduleEvent(EventMograineCrusaderStrike, Milliseconds(7000));
                break;
            case EventMograineHammerOfJustice:
                CastIfConfigured(SelectRandomPlayer(), SpellHammerOfJustice);
                events.ScheduleEvent(EventMograineHammerOfJustice, Milliseconds(15000));
                break;
            default:
                break;
        }
    }

private:
    void YellWithSound(std::string_view text, uint32 soundId, WorldObject const* target = nullptr)
    {
        me->Yell(text, LANG_UNIVERSAL, target);
        me->PlayDirectSound(soundId);
    }

    void ScheduleCombatEvents()
    {
        events.ScheduleEvent(EventMograineCrusaderStrike, Milliseconds(5000));
        events.ScheduleEvent(EventMograineHammerOfJustice, Milliseconds(9000));
    }

    void SummonWhitemane()
    {
        Creature* whitemane = SummonTieredCreature(RiftEntryWhitemane, WhitemaneSpawnPosition, 0.8f, 0.8f,
            TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS);
        if (!whitemane)
        {
            me->KillSelf();
            return;
        }

        _whitemaneGuid = whitemane->GetGUID();
        whitemane->AI()->SetGUID(me->GetGUID(), GuidMograine);
        whitemane->AI()->SetGUID(_combatTargetGuid, GuidCombatTarget);
        whitemane->AI()->DoAction(ActionBeginWhitemaneEncounter);
    }

    ObjectGuid _whitemaneGuid;
    ObjectGuid _combatTargetGuid;
    bool _fakeDeath = false;
    bool _resurrected = false;
};

void AddSC_boss_rift_mograine_whitemane()
{
    RegisterCreatureAI(boss_rift_mograine);
    RegisterCreatureAI(npc_rift_whitemane);
}

} // namespace HeroicDungeonRift
