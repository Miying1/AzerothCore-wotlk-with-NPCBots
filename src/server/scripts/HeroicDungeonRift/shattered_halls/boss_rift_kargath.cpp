/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "ScriptMgr.h"

namespace HeroicDungeonRift
{
namespace
{
// 破碎大厅 - 酋长卡加斯·刃拳（Warchief Kargath Bladefist）
enum BossEvents : uint32
{
    EventBladeDanceStart = 1, // 剑刃之舞（T1基础）
    EventBladeDanceStrike,
    EventSummonWave,          // 传送门援军（T1基础）
    EventRestoreAssassins,    // 刺客死亡20秒后补充（T1基础）
    EventCleave,              // 顺劈斩（T2新增）
    EventUppercut             // 上钩拳（T3新增）
};

enum AddEvents : uint32
{
    EventAddPrimary = 1,
    EventAddSecondary,
    EventAddTertiary
};

enum Spells : uint32
{
    SpellBladeDanceDamage = 30739,
    SpellBladeDanceCharge = 30751,
    SpellBackstab = 30992,
    SpellWoundPoison = 36974,
    SpellBloodthirst = 35949,
    SpellEnrage = 30485,
    SpellAddCleave = 15496,
    SpellAddUppercut = 30471,
    SpellShoot = 22907,
    SpellIncendiaryShot = 30481,
    SpellScatterShot = 23601,
    SpellBossCleave = 15496,
    SpellBossUppercut = 30471
};

enum RiftEntries : uint32
{
    RiftEntryShatteredHandAssassin = 102034,
    RiftEntryShatteredHandHeathen = 102035,
    RiftEntryShatteredHandReaver = 102036,
    RiftEntryShatteredHandSharpshooter = 102037
};

char const* const AggroTexts[] =
{
    "我们才是真正的部落！唯一的部落！",
    "我要把你碎尸万段！",
    "我这“刃拳”的名号可不是白来的！"
};

char const* const SlayTexts[] =
{
    "为了真正的部落而战！",
    "我是唯一的酋长！"
};

constexpr char const* DeathText = "真正的部落……会获胜的。";
}

struct npc_rift_shattered_hand_assassin : public RiftLevel70SummonAI
{
    explicit npc_rift_shattered_hand_assassin(Creature* creature) : RiftLevel70SummonAI(creature) { }

protected:
    void ScheduleAbilities() override
    {
        // 原版援军属于T1基础机制，所有Tier均保持原版SAI窗口。
        _events.ScheduleEvent(EventAddPrimary, Milliseconds(urand(4500, 6500)));
        _events.ScheduleEvent(EventAddSecondary, Milliseconds(urand(8000, 11000)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventAddPrimary:
                DoCastVictim(SpellBackstab);
                _events.ScheduleEvent(EventAddPrimary, Milliseconds(urand(4500, 6500)));
                break;
            case EventAddSecondary:
                DoCastVictim(SpellWoundPoison);
                _events.ScheduleEvent(EventAddSecondary, Milliseconds(urand(22000, 25000)));
                break;
            default:
                break;
        }
    }
};

struct npc_rift_shattered_hand_heathen : public RiftLevel70SummonAI
{
    explicit npc_rift_shattered_hand_heathen(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!_enraged && me->HealthBelowPctDamaged(30, damage))
        {
            _enraged = true;
            DoCast(me, SpellEnrage, true);
        }
    }

protected:
    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventAddPrimary, Milliseconds(urand(7300, 18300)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventAddPrimary)
            return;

        DoCastVictim(SpellBloodthirst);
        _events.ScheduleEvent(EventAddPrimary, Milliseconds(urand(15550, 26450)));
    }

private:
    bool _enraged = false;
};

struct npc_rift_shattered_hand_reaver : public RiftLevel70SummonAI
{
    explicit npc_rift_shattered_hand_reaver(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/,
        SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!_enraged && me->HealthBelowPctDamaged(25, damage))
        {
            _enraged = true;
            DoCast(me, SpellEnrage, true);
        }
    }

protected:
    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventAddPrimary, Milliseconds(urand(7300, 14250)));
        _events.ScheduleEvent(EventAddSecondary, Milliseconds(urand(12150, 30400)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventAddPrimary:
                DoCastVictim(SpellAddCleave);
                _events.ScheduleEvent(EventAddPrimary, Milliseconds(urand(950, 14550)));
                break;
            case EventAddSecondary:
                DoCastVictim(SpellAddUppercut);
                _events.ScheduleEvent(EventAddSecondary, Milliseconds(urand(15800, 30700)));
                break;
            default:
                break;
        }
    }

private:
    bool _enraged = false;
};

struct npc_rift_shattered_hand_sharpshooter : public RiftLevel70SummonAI
{
    explicit npc_rift_shattered_hand_sharpshooter(Creature* creature) : RiftLevel70SummonAI(creature) { }

protected:
    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventAddPrimary, Milliseconds(urand(1200, 6100)));
        _events.ScheduleEvent(EventAddSecondary, Milliseconds(urand(12150, 30400)));
        _events.ScheduleEvent(EventAddTertiary, Milliseconds(urand(13350, 21000)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventAddPrimary:
                DoCastVictim(SpellShoot);
                _events.ScheduleEvent(EventAddPrimary, Milliseconds(urand(3100, 5600)));
                break;
            case EventAddSecondary:
                DoCastVictim(SpellIncendiaryShot);
                _events.ScheduleEvent(EventAddSecondary, Milliseconds(urand(12150, 30400)));
                break;
            case EventAddTertiary:
                DoCastVictim(SpellScatterShot);
                _events.ScheduleEvent(EventAddTertiary, Milliseconds(urand(20700, 39250)));
                break;
            default:
                break;
        }
    }
};

struct boss_rift_kargath : public BossAIBase
{
    explicit boss_rift_kargath(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _bladeDancing = false;
        _danceStrikesRemaining = 0;
        _waveType = 0;
        _assassinCount = 0;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(AggroTexts[urand(0, 2)], LANG_UNIVERSAL);

        // 原版开战时四名刺客加入战斗，去除固定房间坐标以适配裂隙地图。
        _assassinCount = 4;
        for (uint32 i = 0; i < _assassinCount; ++i)
            SummonTieredCreature(RiftEntryShatteredHandAssassin, me->GetRandomNearPosition(9.0f), 0.35f, 0.55f);

        events.ScheduleEvent(EventBladeDanceStart, 30s);
        events.ScheduleEvent(EventSummonWave, 20600ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventCleave, 12s);
        if (_tier >= 3)
            events.ScheduleEvent(EventUppercut, 22s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
            me->Yell(SlayTexts[urand(0, 1)], LANG_UNIVERSAL);
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (summon && summon->GetEntry() == RiftEntryShatteredHandAssassin)
            events.ScheduleEvent(EventRestoreAssassins, 20s);
    }

    void JustDied(Unit* killer) override
    {
        me->Yell(DeathText, LANG_UNIVERSAL);
        BossAIBase::JustDied(killer);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);
        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
            ExecuteRiftEvent(eventId);

        if (!_bladeDancing)
            DoMeleeAttackIfReady();
    }

protected:
    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventBladeDanceStart:
                _bladeDancing = true;
                me->SetReactState(REACT_PASSIVE);
                me->AttackStop();
                // 原版首次选点后还会追加8次移动选点，合计9次刃舞命中。
                _danceStrikesRemaining = 9;
                events.ScheduleEvent(EventBladeDanceStrike, 1ms);
                break;
            case EventBladeDanceStrike:
                if (Unit* target = SelectRandomPlayer(60.0f))
                {
                    CastIfConfigured(target, SpellBladeDanceCharge, true);
                    // 武器伤害由SpellBladeDanceDamage的独立WeaponDamageMultiplier调谐。
                    CastIfConfigured(target, SpellBladeDanceDamage, true);
                }

                if (--_danceStrikesRemaining)
                    events.ScheduleEvent(EventBladeDanceStrike, 600ms);
                else
                {
                    _bladeDancing = false;
                    me->SetReactState(REACT_AGGRESSIVE);
                    if (Unit* victim = me->GetVictim())
                        AttackStart(victim);
                    events.ScheduleEvent(EventBladeDanceStart, Milliseconds(urand(32850, 41350)));
                }
                break;
            case EventSummonWave:
                SummonPortalWave();
                // 原版传送门固定每 20.6 秒召唤一波，不按 Tier 加速。
                events.ScheduleEvent(EventSummonWave, 20600ms);
                break;
            case EventRestoreAssassins:
                SummonMissingAssassin();
                break;
            case EventCleave: // T2新增：剑刃之舞前后不追加近战爆发
                if (_bladeDancing || events.GetTimeUntilEvent(EventBladeDanceStart) < 5s)
                {
                    events.ScheduleEvent(EventCleave, 7s);
                    break;
                }
                CastIfConfigured(me->GetVictim(), SpellBossCleave);
                events.ScheduleEvent(EventCleave, 14s);
                break;
            case EventUppercut: // T3新增：与剑刃之舞和顺劈斩错峰
                if (_bladeDancing || events.GetTimeUntilEvent(EventBladeDanceStart) < 5s ||
                    events.GetTimeUntilEvent(EventCleave) < 3s)
                {
                    events.ScheduleEvent(EventUppercut, 7s);
                    break;
                }
                CastIfConfigured(me->GetVictim(), SpellBossUppercut);
                events.ScheduleEvent(EventUppercut, 18s);
                break;
            default:
                break;
        }
    }

private:
    void SummonMissingAssassin()
    {
        uint32 aliveCount = 0;
        for (ObjectGuid const& guid : _riftSummons)
            if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                if (summon->IsAlive() && summon->GetEntry() == RiftEntryShatteredHandAssassin)
                    ++aliveCount;

        if (aliveCount < _assassinCount)
            SummonTieredCreature(RiftEntryShatteredHandAssassin, me->GetRandomNearPosition(9.0f),
                0.35f, 0.55f);
    }

    void SummonPortalWave()
    {
        uint32 const entries[] =
        {
            RiftEntryShatteredHandHeathen,
            RiftEntryShatteredHandReaver,
            RiftEntryShatteredHandSharpshooter
        };

        // 原版每个传送门周期只生成一名援军；Tier 仅通过统一属性缩放提高援军强度。
        SummonTieredCreature(entries[_waveType], me->GetRandomNearPosition(12.0f), 0.45f, 0.65f);
        _waveType = (_waveType + 1) % 3;
    }

    bool _bladeDancing = false;
    uint8 _danceStrikesRemaining = 0;
    uint8 _waveType = 0;
    uint8 _assassinCount = 0;
};

void AddSC_boss_rift_kargath()
{
    RegisterCreatureAI(boss_rift_kargath);
    RegisterCreatureAI(npc_rift_shattered_hand_assassin);
    RegisterCreatureAI(npc_rift_shattered_hand_heathen);
    RegisterCreatureAI(npc_rift_shattered_hand_reaver);
    RegisterCreatureAI(npc_rift_shattered_hand_sharpshooter);
}

} // namespace HeroicDungeonRift
