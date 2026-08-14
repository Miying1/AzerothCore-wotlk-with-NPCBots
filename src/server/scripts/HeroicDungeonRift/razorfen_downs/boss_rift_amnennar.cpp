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
enum Events : uint32
{
    EventWrath = 1, // 愤怒（原版/T1基础）
    EventFrostbolt, // 寒冰箭（原版/T1基础）
    EventFrostNova // 冰霜新星（原版/T1基础）
};

enum Spells : uint32
{
    SpellFrostArmor = 12556, // 冰霜护甲（原版/T1基础）：重置时按配置对自身施放
    SpellWrath = 13009, // 愤怒（原版/T1基础）：对当前目标施放
    SpellFrostbolt = 15530, // 寒冰箭（原版/T1基础）：对当前目标施放
    SpellFrostNova = 15531 // 冰霜新星（原版/T1基础）：当前目标在10码内时对自身施放
};
}

struct npc_rift_frost_spectre : public ScriptedAI
{
    explicit npc_rift_frost_spectre(Creature* creature) : ScriptedAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        if (Unit* owner = summoner->ToUnit())
            if (Unit* victim = owner->GetVictim())
                AttackStart(victim);
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        if (!UpdateVictim())
            return;
        DoMeleeAttackIfReady();
    }
};

struct boss_rift_amnennar : public BossAIBase
{
    explicit boss_rift_amnennar(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _summonedAt55 = false;
        _summonedAt30 = false;
        if (_tierConfig)
            CastIfConfigured(me, SpellFrostArmor, true);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventWrath, Milliseconds(9000));
        events.ScheduleEvent(EventFrostbolt, Milliseconds(1500));
        events.ScheduleEvent(EventFrostNova, Milliseconds(8000));
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        // 原版/T1阶段召唤：生命值降至55%和30%时各触发一次，每阶段召唤Tier数量的冰霜幽灵。
        if (!_summonedAt55 && me->HealthBelowPctDamaged(55, damage))
        {
            _summonedAt55 = true;
            SummonSpectres(_tier);
        }
        if (!_summonedAt30 && me->HealthBelowPctDamaged(30, damage))
        {
            _summonedAt30 = true;
            SummonSpectres(_tier);
        }
    }

    void JustDied(Unit* /*killer*/) override { DespawnRiftSummons(); }

    // 裂隙伤害校准：原版法术的非直接伤害在通用Tier倍率外补偿15倍。
    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventWrath:
                CastIfConfigured(me->GetVictim(), SpellWrath);
                events.ScheduleEvent(EventWrath, Milliseconds(13000));
                break;
            case EventFrostbolt:
                CastIfConfigured(me->GetVictim(), SpellFrostbolt);
                events.ScheduleEvent(EventFrostbolt, Milliseconds(4000));
                break;
            case EventFrostNova:
                if (me->GetVictim() && me->IsWithinDistInMap(me->GetVictim(), 10.0f))
                    CastIfConfigured(me, SpellFrostNova);
                events.ScheduleEvent(EventFrostNova, Milliseconds(17000));
                break;
            default:
                break;
        }
    }

private:
    void SummonSpectres(uint32 count)
    {
        // 冰霜幽灵按Boss的Tier缩放，血量系数0.45、伤害系数0.65；Boss死亡时统一清理。
        // 两个阶段各召唤1/2/3只，因此未提前击杀时的理论存活上限为2/4/6只。
        for (uint32 i = 0; i < count; ++i)
        {
            Position position = me->GetRandomNearPosition(6.0f);
            if (Creature* summon = SummonTieredCreature(RiftEntryFrostSpectre, position, 0.45f, 0.65f))
                if (Unit* victim = me->GetVictim())
                    summon->AI()->AttackStart(victim);
        }
    }

    bool _summonedAt55 = false;
    bool _summonedAt30 = false;
};

void AddSC_boss_rift_amnennar()
{
    RegisterCreatureAI(boss_rift_amnennar);
    RegisterCreatureAI(npc_rift_frost_spectre);
}

} // namespace HeroicDungeonRift
