/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"

namespace HeroicDungeonRift
{
namespace
{
// 奥金顿：奥金尼地穴 - 死亡观察者希尔拉克（源 Entry 18371；裂隙 Entry 100196-100198）
constexpr uint32 RiftEntryFocusFire = 102053; // 源 Entry 18374

enum Events : uint32
{
    EventInhibitMagic = 1, // 抑制魔法（T1基础）
    EventAttractMagic,     // 吸引魔法（T1基础）
    EventCarnivorousBite,  // 食肉撕咬（T1基础）
    EventFocusFire,        // 聚焦火焰（T1基础）
    EventFocusBlast,
    EventShadowVolley,     // 暗影箭雨（T2新增）
    EventShadowNova        // 暗影新星（T3新增）
};

enum Spells : uint32
{
    SpellInhibitMagic = 32264,
    SpellAttractMagic = 32265,
    SpellCarnivorousBite = 36383,
    SpellFieryBlast = 32302,
    SpellFocusFireVisual = 32286,
    SpellShadowVolley = 57942, // 3.3.5：安卡赫特暗影箭雨
    SpellShadowNova = 59358    // 3.3.5：英雄难度暗影新星，单次基础伤害3700
};

constexpr int32 ShadowVolleyRaidDamage = 4500;
constexpr int32 ShadowNovaRaidDamage = 5500;
}

struct npc_rift_focus_fire : public RiftLevel70SummonAI
{
    explicit npc_rift_focus_fire(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->AttackStop();
        DoCast(me, SpellFocusFireVisual, true);
    }
};

struct boss_rift_shirrak : public BossAIBase
{
    explicit boss_rift_shirrak(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        me->SetControlled(false, UNIT_STATE_ROOT);
        _focusGuid.Clear();
        _focusBlasts = 0;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventInhibitMagic, 1ms);
        events.ScheduleEvent(EventCarnivorousBite, 10s);
        events.ScheduleEvent(EventAttractMagic, 28s);
        events.ScheduleEvent(EventFocusFire, 17s);
        if (_tier >= 2)
            events.ScheduleEvent(EventShadowVolley, 9s);
        if (_tier >= 3)
            events.ScheduleEvent(EventShadowNova, 16s);
    }

    void JustDied(Unit* killer) override
    {
        RemoveInhibitMagic();
        BossAIBase::JustDied(killer);
    }

    void EnterEvadeMode(EvadeReason why) override
    {
        RemoveInhibitMagic();
        me->SetControlled(false, UNIT_STATE_ROOT);
        ScriptedAI::EnterEvadeMode(why);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(2.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventInhibitMagic:
                RefreshInhibitMagic();
                events.ScheduleEvent(EventInhibitMagic, 3s);
                break;
            case EventAttractMagic:
                CastIfConfigured(me, SpellAttractMagic);
                events.RescheduleEvent(EventCarnivorousBite, 1500ms);
                events.ScheduleEvent(EventAttractMagic, 30s);
                break;
            case EventCarnivorousBite:
                CastIfConfigured(me, SpellCarnivorousBite);
                events.ScheduleEvent(EventCarnivorousBite, 10s);
                break;
            case EventFocusFire:
                StartFocusFire();
                events.ScheduleEvent(EventFocusFire, Milliseconds(urand(15000, 20000)));
                break;
            case EventFocusBlast:
                FireAtFocus();
                break;
            case EventShadowVolley: // T2新增：3.3.5暗影箭雨，固定13秒保持施法窗口
                CastFinalRaidDamageSpell(me, SpellShadowVolley, SPELLVALUE_BASE_POINT0,
                    ShadowVolleyRaidDamage);
                events.ScheduleEvent(EventShadowVolley, 13s);
                break;
            case EventShadowNova: // T3新增：3.3.5暗影新星，以19秒周期持续错开箭雨
                CastFinalRaidDamageSpell(me, SpellShadowNova, SPELLVALUE_BASE_POINT0,
                    ShadowNovaRaidDamage);
                events.ScheduleEvent(EventShadowNova, 19s);
                break;
            default:
                break;
        }
    }

private:
    static uint8 GetInhibitStacks(float distance)
    {
        if (distance < 15.0f)
            return 4;
        if (distance < 25.0f)
            return 3;
        if (distance < 35.0f)
            return 2;
        return 1;
    }

    void RefreshInhibitMagic()
    {
        for (auto const& playerReference : me->GetMap()->GetPlayers())
        {
            Player* player = playerReference.GetSource();
            if (!player)
                continue;

            float distance = me->GetDistance(player);
            if (player->IsAlive() && distance < 45.0f)
            {
                Aura* aura = player->GetAura(SpellInhibitMagic);
                if (!aura)
                    aura = me->AddAura(SpellInhibitMagic, player);
                else
                    aura->RefreshDuration();
                if (aura)
                    aura->SetStackAmount(GetInhibitStacks(distance));
            }
            else
                player->RemoveAurasDueToSpell(SpellInhibitMagic);
        }
    }

    void RemoveInhibitMagic()
    {
        for (auto const& playerReference : me->GetMap()->GetPlayers())
            if (Player* player = playerReference.GetSource())
                player->RemoveAurasDueToSpell(SpellInhibitMagic);
    }

    void StartFocusFire()
    {
        Unit* target = SelectRandomPlayer(60.0f);
        if (!target)
            return;

        if (Creature* focus = SummonTieredCreature(RiftEntryFocusFire, target->GetPosition(), 0.2f, 0.4f,
            TEMPSUMMON_TIMED_DESPAWN, 7 * IN_MILLISECONDS))
        {
            _focusGuid = focus->GetGUID();
            _focusBlasts = 3;
            me->TextEmote("希尔拉克将火焰聚焦在一名敌人身上！", target, false);
            me->SetControlled(true, UNIT_STATE_ROOT);
            events.ScheduleEvent(EventFocusBlast, 3s);
        }
    }

    void FireAtFocus()
    {
        if (Creature* focus = ObjectAccessor::GetCreature(*me, _focusGuid))
            focus->CastSpell(focus, SpellFieryBlast, false);

        if (--_focusBlasts)
            events.ScheduleEvent(EventFocusBlast, 500ms);
        else
        {
            me->SetControlled(false, UNIT_STATE_ROOT);
            _focusGuid.Clear();
        }
    }

    ObjectGuid _focusGuid;
    uint8 _focusBlasts = 0;
};

void AddSC_boss_rift_shirrak()
{
    RegisterCreatureAI(boss_rift_shirrak);
    RegisterCreatureAI(npc_rift_focus_fire);
}

} // namespace HeroicDungeonRift
