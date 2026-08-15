/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "ScriptMgr.h"

#include <algorithm>
#include <limits>

namespace HeroicDungeonRift
{
namespace
{
enum BossEvents : uint32
{
    EventPoisonCloud = 1, // 毒云（原版/T1基础）
    EventFrenziedRage,    // 狂暴怒气（原版/T1基础）
    EventSnapjawWave,     // 阿库麦尔钳嘴龟波（T2新增；T3提高数量与频率）
    EventServantWave      // 阿库麦尔仆从波（T3新增）
};

enum AddEvents : uint32
{
    EventSnapjawAbility = 1, // 毁灭（T2/T3钳嘴龟技能）
    EventServantFrostbolt,   // 寒冰箭（T3仆从技能）
    EventServantFrostNova    // 冰霜新星（T3仆从技能）
};

enum Spells : uint32
{
    SpellPoisonCloud = 3815,       // 毒云（原版/T1基础）
    SpellFrenziedRage = 3490,      // 狂暴怒气（原版/T1基础）
    SpellSnapjawAbility = 8391,    // 毁灭（T2/T3钳嘴龟技能）
    SpellServantFrostbolt = 15043, // 寒冰箭（T3仆从技能）
    SpellServantFrostNova = 865    // 冰霜新星（T3仆从技能）
};

class AkumaiAddAI : public RiftSummonAI
{
public:
    explicit AkumaiAddAI(Creature* creature) : RiftSummonAI(creature) { }

protected:
    void ScheduleAbilities() override = 0;
    void ExecuteAbility(uint32 eventId) override = 0;
};
}

struct npc_rift_akumai_snapjaw : public AkumaiAddAI
{
    explicit npc_rift_akumai_snapjaw(Creature* creature) : AkumaiAddAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventSnapjawAbility, Milliseconds(7000));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventSnapjawAbility)
            return;

        DoCastVictim(SpellSnapjawAbility);
        _events.ScheduleEvent(EventSnapjawAbility, Milliseconds(10000));
    }
};

struct npc_rift_akumai_servant : public AkumaiAddAI
{
    explicit npc_rift_akumai_servant(Creature* creature) : AkumaiAddAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventServantFrostbolt, Milliseconds(4000));
        _events.ScheduleEvent(EventServantFrostNova, Milliseconds(9000));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventServantFrostbolt:
                DoCastVictim(SpellServantFrostbolt);
                _events.ScheduleEvent(EventServantFrostbolt, Milliseconds(5500));
                break;
            case EventServantFrostNova:
                DoCast(me, SpellServantFrostNova);
                _events.ScheduleEvent(EventServantFrostNova, Milliseconds(14000));
                break;
            default:
                break;
        }
    }
};

struct boss_rift_akumai : public BossAIBase
{
    explicit boss_rift_akumai(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _snapjawPhaseStarted = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventPoisonCloud, Milliseconds(9000));
        events.ScheduleEvent(EventFrenziedRage, Milliseconds(18000));
        if (_tier >= 2)
            events.ScheduleEvent(EventSnapjawWave, 20s);
        if (_tier >= 3)
            events.ScheduleEvent(EventServantWave, 14s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        // T2/T3在65%生命值开启钳嘴龟阶段并立即出波；此前排定的周期事件仅等待阶段解锁。
        if (_tier >= 2 && !_snapjawPhaseStarted && me->HealthBelowPctDamaged(65, damage))
        {
            _snapjawPhaseStarted = true;
            SummonWave(RiftEntryAkumaiSnapjaw, _tier == 3 ? 3 : 2, 0.55f, 0.65f);
            events.RescheduleEvent(EventSnapjawWave, _tier == 3 ? 16s : 21s);
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        DespawnRiftSummons();
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventPoisonCloud:
                CastIfConfigured(me->GetVictim(), SpellPoisonCloud);
                events.ScheduleEvent(EventPoisonCloud, Milliseconds(12000));
                break;
            case EventFrenziedRage:
                CastIfConfigured(me, SpellFrenziedRage);
                events.ScheduleEvent(EventFrenziedRage, Milliseconds(26000));
                break;
            case EventSnapjawWave:
                if (_snapjawPhaseStarted)
                    SummonWave(RiftEntryAkumaiSnapjaw, _tier == 3 ? 2 : 1, 0.55f, 0.65f);
                events.ScheduleEvent(EventSnapjawWave, _tier == 3 ? 18s : 24s);
                break;
            case EventServantWave:
                SummonWave(RiftEntryAkumaiServant, 1, 0.6f, 0.8f);
                events.ScheduleEvent(EventServantWave, 20s);
                break;
            default:
                break;
        }
    }

private:
    uint32 CountAlive(uint32 entry) const
    {
        uint32 count = 0;
        for (ObjectGuid const& guid : _riftSummons)
            if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                if (summon->IsAlive() && summon->GetEntry() == entry)
                    ++count;
        return count;
    }

    void SummonWave(uint32 entry, uint32 amount, float healthCoefficient, float damageCoefficient)
    {
        // 存活上限按类型独立计算：钳嘴龟T2为3、T3为5；T3仆从为3。
        uint32 cap = entry == RiftEntryAkumaiSnapjaw ? (_tier == 2 ? 3 : 5) : 3;
        uint32 alive = CountAlive(entry);
        for (uint32 i = alive; i < cap && amount > 0; ++i, --amount)
        {
            Position position = me->GetRandomNearPosition(8.0f);
            if (Creature* summon = SummonTieredCreature(entry, position, healthCoefficient, damageCoefficient))
                if (Unit* victim = me->GetVictim())
                    summon->AI()->AttackStart(victim);
        }
    }

    bool _snapjawPhaseStarted = false;
};

void AddSC_boss_rift_akumai()
{
    RegisterCreatureAI(boss_rift_akumai);
    RegisterCreatureAI(npc_rift_akumai_snapjaw);
    RegisterCreatureAI(npc_rift_akumai_servant);
}

} // namespace HeroicDungeonRift
