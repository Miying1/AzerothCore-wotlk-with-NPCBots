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
// 奥金顿：塞泰克大厅 - 利爪之王艾吉斯（Talon King Ikiss，源 Entry 18473；裂隙 Entry 100190-100192）
enum Events : uint32
{
    EventArcaneVolley = 1,  // 奥术箭雨（T1基础）
    EventPolymorph,         // 变形术（T1基础）
    EventArcaneExplosion,   // 闪现后的奥术爆炸（T1阶段）
    EventResumeCombat,      // 奥术爆炸后恢复攻击
    EventSlow,              // 减速（T1英雄基础）
    EventArcaneBarrage,     // 奥术弹幕（T2新增）
    EventManaDetonation     // 法力爆炸（T3新增）
};

enum Spells : uint32
{
    SpellBlinkTeleport = 38203,
    SpellManaShield = 38151,
    SpellArcaneBubble = 9438,
    SpellSlow = 35032,
    SpellPolymorph = 38245,
    SpellArcaneVolley = 35059,
    SpellArcaneExplosion = 38197,
    SpellArcaneBarrage = 58456, // 3.3.5：紫罗兰监狱奥术弹幕
    SpellManaDetonation = 60182 // 3.3.5：紫罗兰监狱法力爆炸
};

constexpr char const* AggroTexts[] =
{
    "你们要向艾吉斯开战？啊——嘎！",
    "艾吉斯要切开你们，漂亮地——嘎嘎——切开！",
    "嘎——啦——嘎！你们逃不掉！"
};
constexpr char const* SlayTexts[] =
{
    "你死了——嘎！离我的宝物远点！",
    "嗯……"
};
constexpr int32 ArcaneVolleyRaidDamage = 3500;
constexpr int32 ArcaneExplosionRaidDamage = 8500;
constexpr int32 ArcaneBarrageRaidDamage = 4500;
constexpr int32 ManaDetonationRaidDamage = 5000;

constexpr char const* DeathText = "艾吉斯不会——嘎，嘎——死……";
constexpr char const* ArcaneExplosionText = "艾吉斯开始引导奥术能量……";
}

struct boss_rift_ikiss : public BossAIBase
{
    explicit boss_rift_ikiss(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _nextExplosionPct = 80;
        _manaShielded = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(AggroTexts[urand(0, 2)], LANG_UNIVERSAL);
        events.ScheduleEvent(EventArcaneVolley, 5s);
        events.ScheduleEvent(EventPolymorph, 8s);
        events.ScheduleEvent(EventSlow, Milliseconds(urand(15000, 25000)));
        if (_tier >= 2)
            events.ScheduleEvent(EventArcaneBarrage, 13s);
        if (_tier >= 3)
            events.ScheduleEvent(EventManaDetonation, 20s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!_manaShielded && me->HealthBelowPctDamaged(20, damage))
        {
            _manaShielded = true;
            CastIfConfigured(me, SpellManaShield, true);
        }

        if (_nextExplosionPct && me->HealthBelowPctDamaged(_nextExplosionPct, damage))
        {
            if (_tier >= 2)
                events.RescheduleEvent(EventArcaneBarrage, 8s);
            if (_tier >= 3)
                events.RescheduleEvent(EventManaDetonation, 12s);

            if (_nextExplosionPct == 80)
                _nextExplosionPct = 50;
            else if (_nextExplosionPct == 50)
                _nextExplosionPct = 25;
            else
                _nextExplosionPct = 0;
            StartArcaneExplosion();
        }
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer() && urand(0, 1))
            me->Yell(SlayTexts[urand(0, 1)], LANG_UNIVERSAL, victim);
    }

    void JustDied(Unit* killer) override
    {
        me->Yell(DeathText, LANG_UNIVERSAL);
        BossAIBase::JustDied(killer);
    }

    void ConfigureTier() override
    {
        // TBC 奥术箭雨/魔爆术分别在事件中覆盖为83级五人团本伤害，WotLK新增技能保留DBC原值。
        SetRaidSpellDamageMultiplier(1.0f);
        AddInterruptImmuneSpell(SpellArcaneExplosion);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventArcaneVolley:
                CastRaidTunedSpell(me, SpellArcaneVolley, ArcaneVolleyRaidDamage);
                events.ScheduleEvent(EventArcaneVolley, Milliseconds(urand(7000, 12000)));
                break;
            case EventPolymorph:
                CastIfConfigured(SelectTarget(SelectTargetMethod::Random, 1, 100.0f, true), SpellPolymorph);
                events.ScheduleEvent(EventPolymorph, Milliseconds(urand(15000, 17500)));
                break;
            case EventArcaneExplosion:
                CastRaidTunedSpell(me, SpellArcaneExplosion, ArcaneExplosionRaidDamage);
                events.ScheduleEvent(EventResumeCombat, 6s);
                break;
            case EventResumeCombat:
                me->SetReactState(REACT_AGGRESSIVE);
                me->GetThreatMgr().ResetAllThreat();
                break;
            case EventSlow:
                CastIfConfigured(me, SpellSlow);
                events.ScheduleEvent(EventSlow, Milliseconds(urand(15000, 30000)));
                break;
            case EventArcaneBarrage: // T2新增：低伤点名，避免与法力爆炸同秒
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellArcaneBarrage, SPELLVALUE_BASE_POINT0,
                    ArcaneBarrageRaidDamage);
                events.ScheduleEvent(EventArcaneBarrage, 17s);
                break;
            case EventManaDetonation: // T3新增：中等范围爆发，与奥术弹幕保持7秒错峰
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellManaDetonation, SPELLVALUE_BASE_POINT0,
                    ManaDetonationRaidDamage);
                events.ScheduleEvent(EventManaDetonation, 24s);
                break;
            default:
                break;
        }
    }

private:
    void StartArcaneExplosion()
    {
        me->InterruptNonMeleeSpells(false);
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();
        if (Unit* target = SelectRandomPlayer())
            CastIfConfigured(target, SpellBlinkTeleport, true);
        CastIfConfigured(me, SpellArcaneBubble, true);
        me->TextEmote(ArcaneExplosionText, nullptr, false);
        events.ScheduleEvent(EventArcaneExplosion, 1s);
    }

    uint8 _nextExplosionPct = 80;
    bool _manaShielded = false;
};

void AddSC_boss_rift_ikiss()
{
    RegisterCreatureAI(boss_rift_ikiss);
}

} // namespace HeroicDungeonRift
