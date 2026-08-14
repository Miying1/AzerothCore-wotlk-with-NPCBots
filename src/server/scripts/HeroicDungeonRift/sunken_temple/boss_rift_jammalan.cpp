/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace HeroicDungeonRift
{
namespace
{
enum BossEvents : uint32
{
    EventEarthgrabTotem = 1, // 召唤地缚图腾（原版/T1基础；T3并存上限提高）
    EventFlamestrike,        // 烈焰风暴（原版/T1基础）
    EventHealingWave,        // 治疗波（原版/T1基础）
    EventHex                 // 迦玛兰的妖术（原版/T1基础）
};

enum TotemEvents : uint32
{
    EventEarthgrab = 1 // 陷地（原版/T1图腾技能）
};

enum Spells : uint32
{
    SpellEarthgrab = 8377,       // 陷地（原版/T1图腾技能）
    SpellFlamestrike = 12468,    // 烈焰风暴（原版/T1基础）
    SpellHealingWave = 12492,    // 治疗波（原版/T1基础）
    SpellHexAura = 12479,        // 迦玛兰的妖术：入口光环（原版/T1基础）
    SpellHexTransform = 12480,   // 迦玛兰的妖术：变形（由入口光环触发）
    SpellHexCharm = 12483,       // 迦玛兰的妖术：魅惑（由入口光环触发）
    SpellGreenChanneling = 13540 // 绿色引导（原版/T1重置视觉）
};
}

struct npc_rift_earthgrab_totem : public ScriptedAI
{
    explicit npc_rift_earthgrab_totem(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _tier = 1;
        me->SetReactState(REACT_PASSIVE);
        _events.ScheduleEvent(EventEarthgrab, 1s);
    }

    void SetData(uint32 type, uint32 data) override
    {
        if (type == RiftDataTier)
        {
            _tier = uint8(std::clamp<uint32>(data, 1, MaxTier));
            _events.RescheduleEvent(EventEarthgrab, 1s);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);
        if (_events.ExecuteEvent() == EventEarthgrab)
        {
            DoCast(me, SpellEarthgrab);
            _events.ScheduleEvent(EventEarthgrab, 15s);
        }
    }

private:
    EventMap _events;
    uint8 _tier = 1;
};

struct boss_rift_jammalan : public BossAIBase
{
    explicit boss_rift_jammalan(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        ClearHexAuras();
        BossAIBase::Reset();
        _lowHealthTriggered = false;
        if (_tierConfig)
            CastIfConfigured(me, SpellGreenChanneling, true);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventEarthgrabTotem, 7000ms);
        events.ScheduleEvent(EventFlamestrike, 5000ms);
        events.ScheduleEvent(EventHealingWave, 9000ms);
        events.ScheduleEvent(EventHex, 12000ms);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (!_lowHealthTriggered && me->HealthBelowPctDamaged(10, damage))
        {
            _lowHealthTriggered = true;
            CastIfConfigured(me, SpellHealingWave);
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        ClearHexAuras();
        DespawnRiftSummons();
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventEarthgrabTotem:
                SummonTotem();
                events.ScheduleEvent(EventEarthgrabTotem, 30000ms);
                break;
            case EventFlamestrike:
                CastIfConfigured(SelectRandomPlayer(30.0f), SpellFlamestrike);
                events.ScheduleEvent(EventFlamestrike, 16000ms);
                break;
            case EventHealingWave:
                if (me->HealthBelowPct(85))
                    CastIfConfigured(me, SpellHealingWave);
                events.ScheduleEvent(EventHealingWave, 11000ms);
                break;
            case EventHex:
                CastIfConfigured(SelectRandomPlayer(30.0f), SpellHexAura);
                events.ScheduleEvent(EventHex, 40000ms);
                break;
            default:
                break;
        }
    }

private:
    uint32 CountTotems() const
    {
        uint32 count = 0;
        for (ObjectGuid const& guid : _riftSummons)
            if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                if (summon->IsAlive() && summon->GetEntry() == RiftEntryEarthgrabTotem)
                    ++count;
        return count;
    }

    void SummonTotem()
    {
        // 地缚图腾存活上限：T1/T2为1，T3为2；达到上限时本次召唤不补充。
        uint32 cap = _tier == 3 ? 2 : 1;
        if (CountTotems() >= cap)
            return;
        SummonTieredCreature(RiftEntryEarthgrabTotem, me->GetRandomNearPosition(7.0f), 0.2f, 1.0f,
            TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 35 * IN_MILLISECONDS);
    }

    // 重置与死亡时清除附近玩家整条妖术链，避免入口、变形或魅惑控制残留。
    void ClearHexAuras()
    {
        Map::PlayerList const& players = me->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* player = itr->GetSource();
            if (!player || !me->IsWithinDistInMap(player, 120.0f))
                continue;
            player->RemoveAurasDueToSpell(SpellHexAura);
            player->RemoveAurasDueToSpell(SpellHexTransform);
            player->RemoveAurasDueToSpell(SpellHexCharm);
        }
    }

    bool _lowHealthTriggered = false;
};

void AddSC_boss_rift_jammalan()
{
    RegisterCreatureAI(boss_rift_jammalan);
    RegisterCreatureAI(npc_rift_earthgrab_totem);
}

} // namespace HeroicDungeonRift
