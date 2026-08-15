/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "ScriptMgr.h"
#include "SpellScript.h"

namespace HeroicDungeonRift
{
namespace
{
// 厄运之槌东区 - 蕾瑟塔蒂丝（Lethtendris）
enum Events : uint32
{
    EventVoidBolt = 1,      // 虚空箭（Spell 22709，T1原版）
    EventShadowBoltVolley,  // 暗影箭雨（Spell 14887，T1原版）
    EventImmolate,          // 献祭（Spell 20787，T1原版）
    EventCurseOfThorns,     // 荆棘诅咒（Spell 16247，T2新增）
    EventCurseOfTongues     // 语言诅咒（Spell 13338，T3新增）
};

enum Spells : uint32
{
    SpellVoidBolt = 22709,               // 虚空箭（T1原版）
    SpellShadowBoltVolley = 14887,       // 暗影箭雨（T1原版）
    SpellImmolate = 20787,               // 献祭（T1原版）
    SpellCurseOfThorns = 16247,          // 荆棘诅咒（T2新增，父Aura）
    SpellCurseOfThornsDamage = 16248,    // 荆棘诅咒反伤法术（父Aura 16247触发）
    SpellCurseOfTongues = 13338          // 语言诅咒（T3新增）
};

constexpr int32 CurseOfThornsTier1DirectDamage = 3500;
}

class spell_rift_lethtendris_curse_of_thorns : public AuraScript
{
    PrepareAuraScript(spell_rift_lethtendris_curse_of_thorns);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SpellCurseOfThornsDamage });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        Creature* caster = GetCaster() ? GetCaster()->ToCreature() : nullptr;
        Unit* target = GetTarget();
        Unit* attacker = target == eventInfo.GetActor() ? eventInfo.GetActionTarget() : eventInfo.GetActor();
        if (!caster || !target || !attacker || GetTierForCreature(caster) < 2)
            return;

        PreventDefaultAction();
        int32 damage = CurseOfThornsTier1DirectDamage;
        if (Creature* damageCaster = target->ToCreature())
            damage = CompensateRiftCreatureLevelScaling(damageCaster, SpellCurseOfThornsDamage, EFFECT_0, damage);
        target->CastCustomSpell(SpellCurseOfThornsDamage, SPELLVALUE_BASE_POINT0, damage, attacker,
            TRIGGERED_FULL_MASK, nullptr, aurEff, caster->GetGUID());
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_rift_lethtendris_curse_of_thorns::HandleProc,
            EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
    }
};

struct boss_rift_lethtendris : public BossAIBase
{
    explicit boss_rift_lethtendris(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventVoidBolt, Milliseconds(3000));
        events.ScheduleEvent(EventShadowBoltVolley, Milliseconds(8000));
        events.ScheduleEvent(EventImmolate, Milliseconds(10000));
        if (_tier >= 2)
            events.ScheduleEvent(EventCurseOfThorns, 12s);
        if (_tier >= 3)
            events.ScheduleEvent(EventCurseOfTongues, 15s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventVoidBolt:
                CastIfConfigured(me->GetVictim(), SpellVoidBolt);
                events.ScheduleEvent(EventVoidBolt, Milliseconds(3200));
                break;
            case EventShadowBoltVolley:
                CastIfConfigured(me, SpellShadowBoltVolley);
                events.ScheduleEvent(EventShadowBoltVolley, Milliseconds(14000));
                break;
            case EventImmolate:
                CastIfConfigured(SelectRandomPlayer(), SpellImmolate);
                events.ScheduleEvent(EventImmolate, Milliseconds(18000));
                break;
            case EventCurseOfThorns: // T2新增：荆棘诅咒，瞬发；父Aura 16247触发反伤法术16248
                CastIfConfigured(me->GetVictim(), SpellCurseOfThorns, true);
                events.ScheduleEvent(EventCurseOfThorns, _tier == 3 ? 22s : 28s);
                break;
            case EventCurseOfTongues: // T3新增：语言诅咒，点名随机目标，瞬发
                CastIfConfigured(SelectRandomPlayer(), SpellCurseOfTongues, true);
                events.ScheduleEvent(EventCurseOfTongues, 24s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_lethtendris()
{
    RegisterCreatureAI(boss_rift_lethtendris);
    RegisterSpellScript(spell_rift_lethtendris_curse_of_thorns);
}

} // namespace HeroicDungeonRift
