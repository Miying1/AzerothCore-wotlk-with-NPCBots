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
// 禁魔监狱 - 天怒预言者苏克拉底（Wrath-Scryer Soccothrates）
enum Events : uint32
{
    EventFelfireShock = 1, // 魔火震击（T1基础）
    EventKnockAway,       // 击退（T1基础）
    EventCharge,          // 冲锋（T1基础，击退后施放）
    EventFelfireTrail,    // 魔火路径（T1基础）
    EventShadowPower,     // 暗影能量（T2新增）
    EventShadowfury       // 暗影之怒（T3新增）
};

enum Spells : uint32
{
    SpellFelImmolation = 36051,
    SpellFelfireShock = 35759,
    SpellKnockAway = 36512,
    SpellFelfire = 35769,
    SpellCharge = 35754,
    SpellShadowPower = 35322,
    SpellShadowfury = 39082
};

constexpr int32 ShadowfuryRaidDamage = 3500;

// 原版中文喊话与语音；裂隙版本去掉战前对话，生成后即可直接攻击。
constexpr char const* SoccothratesAggroText = "终于有个发泄怒气的目标了！";
constexpr char const* SoccothratesSlayText = "啊，真令人满足。";
constexpr char const* SoccothratesKnockAwayText = "看招！";
constexpr char const* SoccothratesDeathText = "我就知道……这是唯一的结局。";
constexpr uint32 SoccothratesAggroSound = 11238;
constexpr uint32 SoccothratesSlaySound = 11239;
constexpr uint32 SoccothratesKnockAwaySound = 11241;
constexpr uint32 SoccothratesDeathSound = 11243;
}

struct boss_rift_soccothrates : public BossAIBase
{
    explicit boss_rift_soccothrates(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(SoccothratesAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(SoccothratesAggroSound);

        events.ScheduleEvent(EventFelfireShock, Milliseconds(urand(8500, 22000)));
        events.ScheduleEvent(EventKnockAway, Milliseconds(urand(30000, 35000)));
        if (_tier >= 2)
            events.ScheduleEvent(EventShadowPower, 11s);
        if (_tier >= 3)
            events.ScheduleEvent(EventShadowfury, 18s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
        {
            me->Yell(SoccothratesSlayText, LANG_UNIVERSAL, victim);
            me->PlayDirectSound(SoccothratesSlaySound, victim->ToPlayer());
        }
    }

    void JustDied(Unit* killer) override
    {
        BossAIBase::JustDied(killer);
        me->Yell(SoccothratesDeathText, LANG_UNIVERSAL);
        me->PlayDirectSound(SoccothratesDeathSound);
    }

    void ConfigureTier() override
    {
        SetRaidSpellDamageMultiplier(2.5f);
        CastIfConfigured(me, SpellFelImmolation, true);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventFelfireShock:
                CastIfConfigured(me->GetVictim(), SpellFelfireShock);
                events.ScheduleEvent(EventFelfireShock, Milliseconds(urand(8500, 22000)));
                break;
            case EventKnockAway:
                me->Yell(SoccothratesKnockAwayText, LANG_UNIVERSAL);
                me->PlayDirectSound(SoccothratesKnockAwaySound);
                me->HandleEmoteCommand(EMOTE_ONESHOT_POINT);
                CastIfConfigured(me, SpellKnockAway);
                events.ScheduleEvent(EventCharge, 4600ms);
                events.ScheduleEvent(EventKnockAway, Milliseconds(urand(20000, 35000)));
                break;
            case EventCharge:
                CastIfConfigured(SelectRandomPlayer(), SpellCharge);
                events.ScheduleEvent(EventFelfireTrail, 300ms);
                break;
            case EventFelfireTrail:
                CastIfConfigured(me, SpellFelfire, true);
                if (++_felfireCount < 7)
                    events.ScheduleEvent(EventFelfireTrail, 300ms);
                else
                    _felfireCount = 0;
                break;
            case EventShadowPower: // T2新增：能源舰暗影能量
                CastIfConfigured(me, SpellShadowPower, true);
                events.ScheduleEvent(EventShadowPower, _tier == 3 ? 22s : 28s);
                break;
            case EventShadowfury: // T3新增：当前分支外域暗影之怒
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellShadowfury, SPELLVALUE_BASE_POINT0,
                    ShadowfuryRaidDamage, true);
                events.ScheduleEvent(EventShadowfury, 20s);
                break;
            default:
                break;
        }
    }

private:
    uint8 _felfireCount = 0;
};

void AddSC_boss_rift_soccothrates()
{
    RegisterCreatureAI(boss_rift_soccothrates);
}

} // namespace HeroicDungeonRift
