/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace HeroicDungeonRift
{
namespace
{
// 魔导师平台 - 炽热的凯尔萨斯（source Entry 24664；裂隙 Entry 100202/100203/100204）。
// T1 保留原版两阶段：火焰阶段 -> 重力流逝阶段。
constexpr uint32 RiftEntryPhoenix = 102056;
constexpr uint32 RiftEntryArcaneSphere = 102057;
constexpr int32 ArcaneDisruptionRaidDamage = 4000;

enum Events : uint32
{
    EventFireball = 1,
    EventFlamestrike,
    EventPhoenix,
    EventPyroblast,
    EventGravityLapse,
    EventGravityLapsePlayers,
    EventGravityLapseKnockup,
    EventGravityLapseFlight,
    EventGravityLapseEnd,
    EventArcaneDisruption,
    EventMindControl
};

enum Spells : uint32
{
    SpellFireball = 44189,
    SpellFlamestrikeSummon = 44192,
    SpellPhoenixVisual = 44194,
    SpellShockBarrier = 46165,
    SpellPyroblast = 36819,
    SpellArcaneDisruption = 36834,
    SpellMindControl = 36797,
    SpellTeleportCenter = 44218,
    SpellGravityLapseInitial = 44224,
    SpellGravityLapsePlayer = 44219,
    SpellGravityLapseDot = 44226,
    SpellGravityLapseFly = 44227,
    SpellGravityLapseChannel = 44251,
    SpellPowerFeedback = 44233
};

constexpr char const* KaelthasAggroText = "别拿那种眼神看着我！我知道你们在想些什么，但风暴要塞的失败早就过去了。";
constexpr char const* KaelthasPhoenixText = "复仇之炎，燃烧吧！";
constexpr char const* KaelthasFlamestrikeText = "Felomin Ashal！";
constexpr char const* KaelthasGravityText = "我要让你们的世界……彻底颠覆……";
constexpr char const* KaelthasTiredText = "主人，赐予我力量。";
constexpr char const* KaelthasRecastGravityText = "不会让你们好过的。";
constexpr char const* KaelthasDeathText = "我的死根本算不了什么！主人一定会消灭你们的！你们会溺毙在自己的鲜血中！这个世界将会熊熊燃烧！啊！";
constexpr uint32 KaelthasAggroSound = 12413;
constexpr uint32 KaelthasPhoenixSound = 12415;
constexpr uint32 KaelthasFlamestrikeSound = 12417;
constexpr uint32 KaelthasGravitySound = 12418;
constexpr uint32 KaelthasTiredSound = 12419;
constexpr uint32 KaelthasRecastGravitySound = 12420;
constexpr uint32 KaelthasDeathSound = 12421;
}

struct npc_rift_kaelthas_phoenix : public RiftLevel70SummonAI
{
    explicit npc_rift_kaelthas_phoenix(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(1, 3s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId == 1)
        {
            if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 40.0f, true))
                DoCast(target, SpellFireball);
            _events.ScheduleEvent(1, 7s);
        }
    }
};

struct npc_rift_kaelthas_arcane_sphere : public RiftLevel70SummonAI
{
    explicit npc_rift_kaelthas_arcane_sphere(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void ScheduleAbilities() override
    {
        me->SetReactState(REACT_AGGRESSIVE);
    }
};

struct boss_rift_kaelthas : public BossAIBase
{
    explicit boss_rift_kaelthas(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        RemoveGravityLapseFromPlayers();
        _phaseTwo = false;
        _gravityLapseActive = false;
        _gravityLapseCounter = 0;
        _gravityLapseCastCount = 0;
        BossAIBase::Reset();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(KaelthasAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(KaelthasAggroSound);

        events.ScheduleEvent(EventFireball, 0ms);
        events.ScheduleEvent(EventPhoenix, 15s);
        events.ScheduleEvent(EventFlamestrike, 22s);
        events.ScheduleEvent(EventPyroblast, 50s);
        if (_tier >= 2)
            events.ScheduleEvent(EventArcaneDisruption, 27s);
        if (_tier >= 3)
            events.ScheduleEvent(EventMindControl, 41s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!_phaseTwo && me->HealthBelowPctDamaged(50, damage))
        {
            _phaseTwo = true;
            _gravityLapseActive = false;
            events.Reset();
            me->CastStop();
            me->SetReactState(REACT_PASSIVE);
            me->AttackStop();
            me->StopMoving();
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveIdle();
            CastIfConfigured(me, SpellTeleportCenter, true);
            events.ScheduleEvent(EventGravityLapse, 5s);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!_phaseTwo)
        {
            BossAIBase::UpdateAI(diff);
            return;
        }

        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (uint32 eventId = events.ExecuteEvent())
            ExecuteRiftEvent(eventId);

        if (!_gravityLapseActive && UpdateVictim())
            DoMeleeAttackIfReady();
    }

    void JustDied(Unit* killer) override
    {
        RemoveGravityLapseFromPlayers();
        BossAIBase::JustDied(killer);
        me->Yell(KaelthasDeathText, LANG_UNIVERSAL);
        me->PlayDirectSound(KaelthasDeathSound);
    }

    void EnterEvadeMode(EvadeReason why) override
    {
        RemoveGravityLapseFromPlayers();
        BossAIBase::EnterEvadeMode(why);
    }

    void ConfigureTier() override
    {
        // 保持原版70级伤害基线；Tier伤害由配置表统一倍率承担，避免T2/T3对同一法术重复放大。
        SetRaidSpellDamageMultiplier(1.0f);
        AddInterruptImmuneSpell(SpellGravityLapseChannel);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventFireball:
                CastIfConfigured(me->GetVictim(), SpellFireball);
                events.ScheduleEvent(EventFireball, Milliseconds(urand(3000, 4500)));
                break;
            case EventFlamestrike:
                me->Yell(KaelthasFlamestrikeText, LANG_UNIVERSAL);
                me->PlayDirectSound(KaelthasFlamestrikeSound);
                CastIfConfigured(SelectRandomPlayer(), SpellFlamestrikeSummon);
                events.ScheduleEvent(EventFlamestrike, 25s);
                break;
            case EventPhoenix:
                me->Yell(KaelthasPhoenixText, LANG_UNIVERSAL);
                me->PlayDirectSound(KaelthasPhoenixSound);
                CastIfConfigured(me, SpellPhoenixVisual, true);
                SummonTieredCreature(RiftEntryPhoenix, me->GetRandomNearPosition(6.0f), 0.45f, 0.65f,
                    TEMPSUMMON_TIMED_DESPAWN, 30000);
                events.ScheduleEvent(EventPhoenix, 60s);
                break;
            case EventPyroblast:
                CastIfConfigured(me, SpellShockBarrier, true);
                CastIfConfigured(me->GetVictim(), SpellPyroblast);
                events.ScheduleEvent(EventPyroblast, 50s);
                break;
            case EventGravityLapse:
                StartGravityLapse();
                break;
            case EventGravityLapsePlayers:
                ApplyGravityLapseToPlayers(EventGravityLapsePlayers);
                events.ScheduleEvent(EventGravityLapseKnockup, 1s);
                break;
            case EventGravityLapseKnockup:
                ApplyGravityLapseToPlayers(EventGravityLapseKnockup);
                events.ScheduleEvent(EventGravityLapseFlight, 1s);
                break;
            case EventGravityLapseFlight:
                ApplyGravityLapseToPlayers(EventGravityLapseFlight);
                for (uint8 i = 0; i < (_tier >= 3 ? 4 : 3); ++i)
                    SummonTieredCreature(RiftEntryArcaneSphere, me->GetRandomNearPosition(5.0f), 0.3f, 0.45f,
                        TEMPSUMMON_TIMED_DESPAWN, 30000);
                CastIfConfigured(me, SpellGravityLapseChannel);
                events.ScheduleEvent(EventGravityLapseEnd, 30s);
                break;
            case EventGravityLapseEnd:
                ApplyGravityLapseToPlayers(EventGravityLapseEnd);
                DespawnRiftSummons();
                me->Yell(KaelthasTiredText, LANG_UNIVERSAL);
                me->PlayDirectSound(KaelthasTiredSound);
                CastIfConfigured(me, SpellPowerFeedback, true);
                _gravityLapseActive = false;
                me->SetReactState(REACT_AGGRESSIVE);
                events.ScheduleEvent(EventGravityLapse, 10s);
                break;
            case EventArcaneDisruption:
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellArcaneDisruption, SPELLVALUE_BASE_POINT0,
                    ArcaneDisruptionRaidDamage, true);
                events.ScheduleEvent(EventArcaneDisruption, _tier == 3 ? 22s : 28s);
                break;
            case EventMindControl:
                CastIfConfigured(SelectRandomPlayer(), SpellMindControl, true);
                events.ScheduleEvent(EventMindControl, 32s);
                break;
            default:
                break;
        }
    }

private:
    void StartGravityLapse()
    {
        if (!_phaseTwo || _gravityLapseActive)
            return;

        _gravityLapseActive = true;
        _gravityLapseCounter = 0;
        me->Yell(_gravityLapseCastCount++ == 0 ? KaelthasGravityText : KaelthasRecastGravityText, LANG_UNIVERSAL);
        me->PlayDirectSound(_gravityLapseCastCount == 1 ? KaelthasGravitySound : KaelthasRecastGravitySound);
        CastIfConfigured(me, SpellGravityLapseInitial);
        events.ScheduleEvent(EventGravityLapsePlayers, 2s);
    }

    void RemoveGravityLapseFromPlayers()
    {
        me->GetMap()->DoForAllPlayers([&](Player* player)
        {
            player->RemoveAurasDueToSpell(SpellGravityLapseFly);
            player->RemoveAurasDueToSpell(SpellGravityLapseDot);
        });
    }

    void ApplyGravityLapseToPlayers(uint32 action)
    {
        if (action == EventGravityLapseEnd)
        {
            RemoveGravityLapseFromPlayers();
            return;
        }

        _gravityLapseCounter = 0;
        me->GetMap()->DoForAllPlayers([&](Player* player)
        {
            if (player->IsGameMaster() || !player->IsAlive())
                return;

            if (action == EventGravityLapsePlayers)
                CastIfConfigured(player, SpellGravityLapsePlayer + _gravityLapseCounter, true);
            else if (action == EventGravityLapseKnockup)
                player->CastSpell(player, SpellGravityLapseDot, true, nullptr, nullptr, me->GetGUID());
            else if (action == EventGravityLapseFlight)
                player->CastSpell(player, SpellGravityLapseFly, true, nullptr, nullptr, me->GetGUID());
            ++_gravityLapseCounter;
        });
    }

    bool _phaseTwo = false;
    bool _gravityLapseActive = false;
    uint8 _gravityLapseCounter = 0;
    uint8 _gravityLapseCastCount = 0;
};

void AddSC_boss_rift_kaelthas()
{
    RegisterCreatureAI(boss_rift_kaelthas);
    RegisterCreatureAI(npc_rift_kaelthas_phoenix);
    RegisterCreatureAI(npc_rift_kaelthas_arcane_sphere);
}

} // namespace HeroicDungeonRift
