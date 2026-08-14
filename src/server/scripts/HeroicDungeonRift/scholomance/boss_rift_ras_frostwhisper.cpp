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
// 通灵学院 - 莱斯·霜语（Ras Frostwhisper）
enum Events : uint32
{
    EventFrostboltVolley = 1, // 连发寒冰箭（Spell 8398，T1原版）
    EventChillNova,           // 冰冻新星（Spell 18099，T1原版）
    EventFreeze,              // 冰冻术（Spell 18763，T2新增）
    EventFear                 // 恐惧（Spell 12096，T3新增）
};

enum Spells : uint32
{
    SpellFrostArmor = 18100,      // 霜甲术（T1原版，开战自施）
    SpellFrostboltVolley = 8398,  // 连发寒冰箭（T1原版）
    SpellChillNova = 18099,       // 冰冻新星（T1原版）
    SpellFreeze = 18763,          // 冰冻术（T2新增，混合BP：仅覆写BP0周期伤害）
    SpellFear = 12096             // 恐惧（T3新增）
};

constexpr int32 FreezeTier1DamagePerTick = 1800;

constexpr char const* RasFrostwhisperYell = "这不可能！！";
constexpr uint32 RasFrostwhisperSound = 6371;
}

struct boss_rift_ras_frostwhisper : public BossAIBase
{
    explicit boss_rift_ras_frostwhisper(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        CastIfConfigured(me, SpellFrostArmor, true);
        events.ScheduleEvent(EventFrostboltVolley, Milliseconds(10000));
        events.ScheduleEvent(EventChillNova, Milliseconds(14000));
        if (_tier >= 2)
            events.ScheduleEvent(EventFreeze, 8s);
        if (_tier >= 3)
            events.ScheduleEvent(EventFear, 18s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->Yell(RasFrostwhisperYell, LANG_UNIVERSAL);
        me->PlayDirectSound(RasFrostwhisperSound);
        BossAIBase::JustDied(nullptr);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); AddInterruptImmuneSpell(SpellFear); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventFrostboltVolley:
                CastIfConfigured(me, SpellFrostboltVolley);
                events.ScheduleEvent(EventFrostboltVolley, Milliseconds(14000));
                break;
            case EventChillNova:
                CastIfConfigured(me, SpellChillNova);
                events.ScheduleEvent(EventChillNova, Milliseconds(18000));
                break;
            case EventFreeze: // T2新增：冰冻术，点名随机目标，瞬发；混合BP仅覆写BP0周期伤害
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellFreeze, SPELLVALUE_BASE_POINT0,
                    FreezeTier1DamagePerTick, true);
                events.ScheduleEvent(EventFreeze, _tier == 3 ? 14s : 17s);
                break;
            case EventFear: // T3新增：恐惧，点名随机目标，免疫打断
                CastIfConfigured(SelectRandomPlayer(), SpellFear);
                events.ScheduleEvent(EventFear, 20s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_ras_frostwhisper()
{
    RegisterCreatureAI(boss_rift_ras_frostwhisper);
}

} // namespace HeroicDungeonRift
