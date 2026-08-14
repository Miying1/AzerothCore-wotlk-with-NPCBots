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
// 禁魔监狱 - 末日预言者达尔莉安（source 20885；裂隙 Entry 100172-100174）
enum Events : uint32
{
    EventGiftOfTheDoomsayer = 1, // 末日预言者的礼物（T1基础）
    EventWhirlwind,             // 旋风斩（T1基础）
    EventHeal,                  // 治疗术（T1基础，旋风斩后施放）
    EventShadowWave,            // 暗影波（T1英雄机制）
    EventShadowNova,            // 暗影新星（T2新增）
    EventPsychicScream          // 心灵尖啸（T3新增）
};

enum Spells : uint32
{
    SpellGiftOfTheDoomsayer = 36173,
    SpellWhirlwind = 36142,
    SpellHeal = 36144,
    SpellShadowWave = 39016,
    SpellShadowNova = 36127,
    SpellPsychicScream = 34322
};

constexpr int32 ShadowNovaRaidDamage = 5000;

// 原版中文喊话与语音。
constexpr char const* DalliahAggroText = "激怒我可不是个好主意！";
constexpr char const* DalliahSlayText = "完全不是我的对手，和其他的家伙一样。";
constexpr char const* DalliahWhirlwindText = "旋风斩！";
constexpr char const* DalliahHealText = "感觉好多了。";
constexpr char const* DalliahDeathText = "我可真的生气了。";
constexpr uint32 DalliahAggroSound = 11086;
constexpr uint32 DalliahSlaySound = 11087;
constexpr uint32 DalliahWhirlwindSound = 11089;
constexpr uint32 DalliahHealSound = 11091;
constexpr uint32 DalliahDeathSound = 11093;
}

struct boss_rift_dalliah : public BossAIBase
{
    explicit boss_rift_dalliah(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(DalliahAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(DalliahAggroSound);

        events.ScheduleEvent(EventGiftOfTheDoomsayer, Milliseconds(urand(8000, 12000)));
        events.ScheduleEvent(EventWhirlwind, Milliseconds(urand(20000, 30000)));
        events.ScheduleEvent(EventShadowWave, Milliseconds(urand(11000, 30000)));
        if (_tier >= 2)
            events.ScheduleEvent(EventShadowNova, 15s);
        if (_tier >= 3)
            events.ScheduleEvent(EventPsychicScream, 23s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
        {
            me->Yell(DalliahSlayText, LANG_UNIVERSAL, victim);
            me->PlayDirectSound(DalliahSlaySound, victim->ToPlayer());
        }
    }

    void JustDied(Unit* killer) override
    {
        BossAIBase::JustDied(killer);
        me->Yell(DalliahDeathText, LANG_UNIVERSAL);
        me->PlayDirectSound(DalliahDeathSound);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(2.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventGiftOfTheDoomsayer:
                CastIfConfigured(me->GetVictim(), SpellGiftOfTheDoomsayer);
                events.ScheduleEvent(EventGiftOfTheDoomsayer, Milliseconds(urand(17000, 35000)));
                break;
            case EventWhirlwind:
                me->Yell(DalliahWhirlwindText, LANG_UNIVERSAL);
                me->PlayDirectSound(DalliahWhirlwindSound);
                CastIfConfigured(me, SpellWhirlwind);
                events.ScheduleEvent(EventHeal, 7s);
                events.ScheduleEvent(EventWhirlwind, Milliseconds(urand(20000, 30000)));
                break;
            case EventHeal:
                me->Yell(DalliahHealText, LANG_UNIVERSAL);
                me->PlayDirectSound(DalliahHealSound);
                CastIfConfigured(me, SpellHeal);
                break;
            case EventShadowWave:
                CastIfConfigured(me->GetVictim(), SpellShadowWave);
                events.ScheduleEvent(EventShadowWave, 30s);
                break;
            case EventShadowNova: // T2新增：禁魔监狱暗影新星
                CastFinalRaidDamageSpell(me, SpellShadowNova, SPELLVALUE_BASE_POINT0,
                    ShadowNovaRaidDamage, true);
                events.ScheduleEvent(EventShadowNova, _tier == 3 ? 14s : 18s);
                break;
            case EventPsychicScream: // T3新增：风暴要塞心灵尖啸
                CastIfConfigured(me, SpellPsychicScream, true);
                events.ScheduleEvent(EventPsychicScream, 24s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_dalliah()
{
    RegisterCreatureAI(boss_rift_dalliah);
}

} // namespace HeroicDungeonRift
