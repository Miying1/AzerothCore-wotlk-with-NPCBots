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
    ActionBeginWhitemaneEncounter = 1,
    ActionResetWhitemaneEncounter
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
// 入场移动兜底参数：出生点到莫格莱尼身边需要穿过大门并跨越多级台阶，若路径被地形或门碰撞
// 卡住、或 MovementInform 因移动被重置而丢失，战斗将永远停在"只有喊话没有现身"的状态。
// 因此持续无实质进展达到阈值时直接传送进场，保证怀特迈恩必然出现并进入战斗。
constexpr float EntranceReachedDistance = 2.0f;   // 视为已到达的半径（码）
constexpr float EntranceProgressThreshold = 0.5f; // 每轮检查至少应缩短的距离（码）
constexpr uint32 EntranceProgressCheckMs = 500;   // 进展检查间隔（毫秒）
constexpr uint32 EntranceStallTeleportMs = 3000;  // 无实质进展超过该时长则传送进场（毫秒）
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
        _lastCastSpellId = 0;
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
            _damagePermille = std::max<uint32>(1, data);
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
        if (action == ActionResetWhitemaneEncounter)
        {
            ResetToDormantState();
            return;
        }

        if (action != ActionBeginWhitemaneEncounter || _phase != WhitemaneDormant)
            return;

        Creature* mograine = ObjectAccessor::GetCreature(*me, _mograineGuid);
        if (!mograine || !mograine->IsAlive())
        {
            LOG_ERROR("scripts", "Rift Whitemane {} cannot start: Mograine {} is unavailable.",
                me->GetGUID().ToString(), _mograineGuid.ToString());
            return;
        }

        me->SetVisible(true);
        if (GameObject* door = me->FindNearestGameObject(WhitemaneDoorEntry, WhitemaneDoorSearchRange))
            door->SetGoState(GO_STATE_ACTIVE_ALTERNATIVE);

        YellWithSound(WhitemaneIntroText, SoundWhitemaneIntro);
        _phase = WhitemaneMovingToMograine;
        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        me->SetInCombatWithZone();
        Position const approachPosition = GetApproachPosition(*mograine);
        me->GetMotionMaster()->MovePoint(PointWhitemaneEntrance, approachPosition);
        _lastEntranceDistance = me->GetDistance(approachPosition);
        _entranceProgressTimer = 0;
        _entranceStallMilliseconds = 0;
    }

    void MovementInform(uint32 type, uint32 pointId) override
    {
        if (type != POINT_MOTION_TYPE)
            return;

        if (pointId == PointWhitemaneEntrance)
            EnterCombatPhaseOne();
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
        ScaleRiftSummonSpellDamage(_lastCastSpellId, damage, damageType, _damagePermille, 1);
    }

    void OnSpellCast(SpellInfo const* spell) override
    {
        if (spell)
            _lastCastSpellId = spell->Id;
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

        // 入场移动兜底：路径卡住或MovementInform丢失时传送进场，避免只喊话不出人。
        if (!UpdateWhitemaneEntrance(diff))
            return;

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (uint32 eventId = _events.ExecuteEvent())
        {
            if (eventId == EventWhitemaneResurrectMograine)
            {
                if (Creature* mograine = ObjectAccessor::GetCreature(*me, _mograineGuid))
                    DoCast(mograine, SpellScarletResurrection);
                else
                    me->KillSelf();
            }
            else if (eventId == EventWhitemaneResurrectionYell)
            {
                if (_phase == WhitemaneResurrecting || _phase == WhitemaneCombatPhaseTwo)
                    YellWithSound(WhitemaneResurrectText, SoundWhitemaneResurrect);
            }
            else if (UpdateVictim())
            {
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
        }

        if (UpdateVictim())
            DoMeleeAttackIfReady();
    }

private:
    // 怀特迈恩走到莫格莱尼身边后进入第一战斗阶段；由 MovementInform、接近判定或传送兜底触发。
    void EnterCombatPhaseOne()
    {
        if (_phase != WhitemaneMovingToMograine)
            return;

        _phase = WhitemaneCombatPhaseOne;
        me->SetInCombatWithZone();
        ResumeCombat();
    }

    // 入场移动兜底：只要还在 WhitemaneMovingToMograine 阶段就持续跟踪到目标的距离。
    // 到达目标附近但 MovementInform 未触发时直接进入战斗；持续无实质进展则传送进场。
    bool UpdateWhitemaneEntrance(uint32 diff)
    {
        if (_phase != WhitemaneMovingToMograine)
            return true;

        Creature* mograine = ObjectAccessor::GetCreature(*me, _mograineGuid);
        if (!mograine || !mograine->IsAlive())
        {
            me->KillSelf();
            return false;
        }

        Position const approachPosition = GetApproachPosition(*mograine);
        float const distanceToTarget = me->GetDistance(approachPosition);

        if (distanceToTarget <= EntranceReachedDistance)
        {
            EnterCombatPhaseOne();
            return false;
        }

        _entranceProgressTimer += diff;
        if (_entranceProgressTimer < EntranceProgressCheckMs)
            return true;

        _entranceProgressTimer = 0;
        if (distanceToTarget < _lastEntranceDistance - EntranceProgressThreshold)
        {
            _lastEntranceDistance = distanceToTarget;
            _entranceStallMilliseconds = 0;
        }
        else
        {
            _entranceStallMilliseconds += EntranceProgressCheckMs;
            if (_entranceStallMilliseconds >= EntranceStallTeleportMs)
            {
                LOG_WARN("scripts", "Rift Whitemane {} is stuck approaching Mograine {} (remaining {:.1f} yd), teleporting into position.",
                    me->GetGUID().ToString(), mograine->GetGUID().ToString(), distanceToTarget);
                me->GetMotionMaster()->Clear(false);
                me->NearTeleportTo(approachPosition.GetPositionX(), approachPosition.GetPositionY(),
                    approachPosition.GetPositionZ(), approachPosition.GetOrientation());
                EnterCombatPhaseOne();
                return false;
            }
        }
        return true;
    }

    void ResetToDormantState()
    {
        _events.Reset();
        _resurrectionStarted = false;
        _phase = WhitemaneDormant;
        _combatTargetGuid.Clear();
        me->SetVisible(true);
        me->CombatStop(true);
        DoResetThreatList();
        me->GetMotionMaster()->Clear(false);
        me->NearTeleportTo(WhitemaneSpawnPosition.GetPositionX(), WhitemaneSpawnPosition.GetPositionY(),
            WhitemaneSpawnPosition.GetPositionZ(), WhitemaneSpawnPosition.GetOrientation());
        me->SetStandState(UNIT_STAND_STATE_STAND);
        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        me->SetFullHealth();

        if (GameObject* door = me->FindNearestGameObject(WhitemaneDoorEntry, WhitemaneDoorSearchRange))
            door->SetGoState(GO_STATE_READY);
    }

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

    SpellCastResult DoCast(Unit* target, uint32 spellId, bool triggered = false)
    {
        return CastRiftTunedSpell(me, target, spellId, triggered, &_lastCastSpellId);
    }

    SpellCastResult DoCastVictim(uint32 spellId, bool triggered = false)
    {
        return DoCast(me->GetVictim(), spellId, triggered);
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
    uint32 _lastCastSpellId = 0;
    bool _resurrectionStarted = false;
    WhitemanePhase _phase = WhitemaneDormant;
    float _lastEntranceDistance = 0.0f;      // 上一次检查时到莫格莱尼身边目标的距离
    uint32 _entranceProgressTimer = 0;       // 距下一次进展检查的计时
    uint32 _entranceStallMilliseconds = 0;   // 已持续无实质进展的时长
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
        _combatTargetGuid.Clear();
        _fakeDeath = false;
        _resurrected = false;
        PrepareWhitemane();
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
        me->RemoveAurasDueToSpell(SpellRetributionAura);
        me->SetStandState(UNIT_STAND_STATE_DEAD);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);

        Creature* whitemane = ObjectAccessor::GetCreature(*me, _whitemaneGuid);
        if (!whitemane || !whitemane->IsAlive())
        {
            LOG_ERROR("scripts", "Rift Mograine {} cannot enter fake death: prepared Whitemane {} is unavailable.",
                me->GetGUID().ToString(), _whitemaneGuid.ToString());
            EnterEvadeMode(EVADE_REASON_SEQUENCE_BREAK);
            return;
        }

        whitemane->AI()->SetGUID(me->GetGUID(), GuidMograine);
        whitemane->AI()->SetGUID(_combatTargetGuid, GuidCombatTarget);
        whitemane->AI()->DoAction(ActionBeginWhitemaneEncounter);
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

    void SummonedCreatureDespawn(Creature* summon) override
    {
        if (summon && summon->GetGUID() == _whitemaneGuid)
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

    void PrepareWhitemane()
    {
        if (_whitemaneGuid)
            if (Creature* existing = ObjectAccessor::GetCreature(*me, _whitemaneGuid))
            {
                existing->AI()->DoAction(ActionResetWhitemaneEncounter);
                existing->AI()->SetGUID(me->GetGUID(), GuidMograine);
                return;
            }

        // 怀特迈恩在莫格莱尼假死前必须保持休眠（不可攻击、被动、不进入战斗），
        // 故保留被动状态；入口启动时由 DoAction 打开大门并让她入场。
        Creature* whitemane = SummonTieredCreature(RiftEntryWhitemane, WhitemaneSpawnPosition, 0.8f, 0.8f,
            TEMPSUMMON_CORPSE_TIMED_DESPAWN, CreatureSummonLifetimeMilliseconds, true);
        if (!whitemane)
        {
            LOG_ERROR("scripts", "Rift Mograine {} failed to prepare Whitemane at {}.",
                me->GetGUID().ToString(), WhitemaneSpawnPosition.GetPositionX());
            return;
        }

        _whitemaneGuid = whitemane->GetGUID();
        whitemane->AI()->SetGUID(me->GetGUID(), GuidMograine);
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
