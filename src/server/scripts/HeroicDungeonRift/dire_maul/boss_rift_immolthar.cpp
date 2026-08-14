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
// 厄运之槌西区 - 伊莫塔尔（Immol'thar）
enum Events : uint32
{
    EventInfectedBite = 1, // 感染撕咬（Spell 16128，T1原版）
    EventTrample,          // 践踏（Spell 5568，T1原版）
    EventPortal,           // 伊莫塔尔传送门（Spell 22950，T1原版）
    EventEye,              // 伊莫塔尔之眼（Spell 22899，T2新增）
    EventTier3Skill        // 暗影箭（Spell 20791，T3新增）
};

enum Spells : uint32
{
    SpellInfectedBite = 16128,       // 感染撕咬（T1原版）
    SpellTrample = 5568,             // 践踏（T1原版）
    SpellPortalOfImmolthar = 22950,  // 伊莫塔尔传送门（T1原版）
    SpellEyeOfImmolthar = 22899,     // 伊莫塔尔之眼（T2新增）
    SpellFrenzy = 8269,              // 狂乱（原版40%生命值阶段）
    SpellShadowBolt = 20791          // 暗影箭（T3新增）
};

constexpr int32 ShadowBoltTier1DirectDamage = 4500;

// 原版由5座水晶塔全关后才可攻击，裂隙已去除该前置，直接可攻击。
constexpr char const* ImmoltharFrenzyText = "陷入了狂暴！";
constexpr uint32 ImmoltharFrenzySound = 38630;
}

struct boss_rift_immolthar : public BossAIBase
{
    explicit boss_rift_immolthar(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventInfectedBite, Milliseconds(7000));
        events.ScheduleEvent(EventTrample, Milliseconds(11000));
        events.ScheduleEvent(EventPortal, Milliseconds(14000));
        if (_tier >= 2)
            events.ScheduleEvent(EventEye, 16s);       // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 9s); // T3新增
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*type*/, SpellSchoolMask /*school*/) override
    {
        // 原版机制：血量低于40%时狂暴
        if (_frenzied || !me->HealthBelowPctDamaged(40, damage))
            return;

        _frenzied = true;
        me->TextEmote(ImmoltharFrenzyText, nullptr, false);
        me->PlayDirectSound(ImmoltharFrenzySound);
        CastIfConfigured(me, SpellFrenzy);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventInfectedBite:
                CastIfConfigured(me->GetVictim(), SpellInfectedBite);
                events.ScheduleEvent(EventInfectedBite, Milliseconds(12000));
                break;
            case EventTrample:
                CastIfConfigured(me, SpellTrample);
                events.ScheduleEvent(EventTrample, Milliseconds(14000));
                break;
            case EventPortal:
                CastIfConfigured(SelectRandomPlayer(), SpellPortalOfImmolthar);
                events.ScheduleEvent(EventPortal, Milliseconds(18000));
                break;
            case EventEye: // T2新增：伊莫塔尔之眼，瞬发
                CastIfConfigured(me, SpellEyeOfImmolthar, true);
                events.ScheduleEvent(EventEye, _tier == 3 ? 16s : 20s);
                break;
            case EventTier3Skill: // T3新增：暗影箭，瞬发；覆写BP0直接伤害
                CastFinalRaidDamageSpell(me->GetVictim(), SpellShadowBolt, SPELLVALUE_BASE_POINT0,
                    ShadowBoltTier1DirectDamage, true);
                events.ScheduleEvent(EventTier3Skill, 5s);
                break;
            default:
                break;
        }
    }

private:
    bool _frenzied = false;
};

void AddSC_boss_rift_immolthar()
{
    RegisterCreatureAI(boss_rift_immolthar);
}

} // namespace HeroicDungeonRift
