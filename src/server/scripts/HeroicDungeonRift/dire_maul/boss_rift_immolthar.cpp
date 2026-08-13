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
    EventInfectedBite = 1, // 感染撕咬（T1基础）
    EventTrample,          // 践踏（T1基础）
    EventPortal,           // 伊莫塔尔传送门（T1基础）
    EventEye,              // 伊莫塔尔之眼（T2新增）
    EventTier3Skill        // 暗影箭（T3新增）
};

enum Spells : uint32
{
    SpellInfectedBite = 16128,       // 感染撕咬
    SpellTrample = 5568,             // 践踏
    SpellPortalOfImmolthar = 22950,  // 伊莫塔尔传送门
    SpellEyeOfImmolthar = 22899,     // 伊莫塔尔之眼
    SpellFrenzy = 8269,              // 狂乱
    SpellShadowBolt = 20791          // 暗影箭
};

// 原版由5座水晶塔全关后才可攻击，裂隙已去除该前置，直接可攻击。
constexpr char const* ImmoltharFrenzyText = "陷入了狂暴！";
constexpr uint32 ImmoltharFrenzySound = 38630;
}

struct boss_rift_immolthar : public BossAIBase
{
    explicit boss_rift_immolthar(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        ScheduleTieredEvent(EventInfectedBite, 7000, 5500, 4500);
        ScheduleTieredEvent(EventTrample, 11000, 9000, 7200);
        ScheduleTieredEvent(EventPortal, 14000, 11000, 9000);
        if (_tier >= 2)
            events.ScheduleEvent(EventEye, 16s);       // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 9s); // T3新增
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
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
                ScheduleTieredEvent(EventInfectedBite, 12000, 9500, 7500);
                break;
            case EventTrample:
                CastIfConfigured(me, SpellTrample);
                ScheduleTieredEvent(EventTrample, 14000, 11000, 9000);
                break;
            case EventPortal:
                CastIfConfigured(SelectRandomPlayer(), SpellPortalOfImmolthar);
                ScheduleTieredEvent(EventPortal, 18000, 14500, 11500);
                break;
            case EventEye: // T2新增：伊莫塔尔之眼，读条不可打断
                CastIfConfigured(me, SpellEyeOfImmolthar, true);
                events.ScheduleEvent(EventEye, _tier == 3 ? 16s : 20s);
                break;
            case EventTier3Skill: // T3新增：暗影箭，读条不可打断
                CastIfConfigured(me->GetVictim(), SpellShadowBolt, true);
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
