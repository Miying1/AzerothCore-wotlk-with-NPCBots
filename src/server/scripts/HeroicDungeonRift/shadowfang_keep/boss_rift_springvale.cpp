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
// 影牙城堡 - 指挥官斯普林瓦尔（Commander Springvale）
enum Events : uint32
{
    EventHammerOfJustice = 1, // 制裁之锤（原版/T1基础，眩晕当前目标）
    EventHolyLight,           // 圣光术（原版/T1基础，低血量自疗）
    EventConsecration,        // 奉献（T2新增）
    EventTier3Skill           // 愤怒之锤（T3新增）
};

enum Spells : uint32
{
    SpellHammerOfJustice = 5588, // 制裁之锤（原版/T1基础）
    SpellHolyLight = 1026,       // 圣光术（原版/T1基础）
    SpellDivineShield = 33581,   // 圣盾术（原版/T1基础，低血量无敌）
    SpellConsecration = 26573,   // 奉献（T2新增）
    SpellHammerOfWrath = 24275   // 愤怒之锤（T3新增）
};

constexpr int32 ConsecrationTier1DamagePerTick = 2000;
constexpr int32 HammerOfWrathTier1DirectDamage = 4500;

// 喊话（中文，对应原版 creature_text）
constexpr char const* SpringvaleAggroText = "城堡里有入侵者！准备战斗！";
constexpr char const* SpringvaleDeathText = "我们的警戒是永恒的……";
constexpr uint32 SpringvaleAggroSound = 48826;
constexpr uint32 SpringvaleDeathSound = 48829;
}

struct boss_rift_springvale : public BossAIBase
{
    explicit boss_rift_springvale(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _divineShieldUsed = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(SpringvaleAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(SpringvaleAggroSound);

        events.ScheduleEvent(EventHammerOfJustice, Milliseconds(5000));
        events.ScheduleEvent(EventHolyLight, Milliseconds(8000));
        if (_tier >= 2)
            events.ScheduleEvent(EventConsecration, 12s); // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 16s);   // T3新增
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        // 原版机制：血量低于30%时开启圣盾术无敌（一次性）
        if (_divineShieldUsed || !me->HealthBelowPctDamaged(30, damage))
            return;

        _divineShieldUsed = true;
        CastIfConfigured(me, SpellDivineShield);
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->Yell(SpringvaleDeathText, LANG_UNIVERSAL);
        me->PlayDirectSound(SpringvaleDeathSound);
        BossAIBase::JustDied(nullptr);
    }

    void ConfigureTier() override { }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventHammerOfJustice:
                CastIfConfigured(me->GetVictim(), SpellHammerOfJustice);
                events.ScheduleEvent(EventHammerOfJustice, Milliseconds(16000));
                break;
            case EventHolyLight:
                if (me->HealthBelowPct(60))
                    CastIfConfigured(me, SpellHolyLight);
                events.ScheduleEvent(EventHolyLight, Milliseconds(10000));
                break;
            case EventConsecration: // T2新增：瞬发
                CastFinalRaidDamageSpell(me, SpellConsecration, SPELLVALUE_BASE_POINT0,
                    ConsecrationTier1DamagePerTick, true);
                events.ScheduleEvent(EventConsecration, _tier == 3 ? 14s : 18s);
                break;
            case EventTier3Skill: // T3新增：点名单体，选随机目标，瞬发
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellHammerOfWrath, SPELLVALUE_BASE_POINT0,
                    HammerOfWrathTier1DirectDamage, true);
                events.ScheduleEvent(EventTier3Skill, 12s);
                break;
            default:
                break;
        }
    }

private:
    bool _divineShieldUsed = false;
};

void AddSC_boss_rift_springvale()
{
    RegisterCreatureAI(boss_rift_springvale);
}

} // namespace HeroicDungeonRift
