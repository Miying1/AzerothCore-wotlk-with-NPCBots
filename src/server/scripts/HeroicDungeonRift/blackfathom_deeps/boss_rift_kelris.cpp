/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "Random.h"
#include "ScriptMgr.h"

namespace HeroicDungeonRift
{
namespace
{
enum Events : uint32
{
    EventSleep = 1,           // 催眠术（原版/T1基础）
    EventMindBlast,           // 心灵震爆（原版/T1基础）
    EventShadowWordPain,      // 暗言术：痛（T2新增）
    EventShadowBoltVolley     // 暗影箭雨（T3新增）
};

enum Spells : uint32
{
    SpellResetVisual = 8734,       // 黑暗深渊引导（原版/T1基础；重置视觉）
    SpellSleep = 8399,             // 催眠术（原版/T1基础）
    SpellMindBlast = 15587,        // 心灵震爆（原版/T1基础）
    SpellShadowWordPain = 10892,   // 暗言术：痛（T2新增；18秒DoT，每3秒一跳）
    SpellShadowBoltVolley = 20741  // 暗影箭雨（T3新增；以自身为中心的范围暗影伤害，覆写BP0）
};

// 暗影箭雨的T1基线只能按Boss给出：同一Spell ID(20741)被巴纳扎尔(4500)、阿纳斯提(5500)、
// 阿鲁高(表中5800)复用，而rift_spell_damage.h按Spell ID索引，放不下每Boss不同的基线。
// 暗言术：痛(10892)仅本Boss使用，基线直接放在rift_spell_damage.h里，施法走CastIfConfigured。
constexpr int32 ShadowBoltVolleyTier1DirectDamage = 4500; // 与同副本巴纳扎尔T3箭雨保持一致

// 原版催眠术目标为SMART_TARGET_HOSTILE_RANDOM_NOT_TOP：30码内随机、排除当前坦克。
constexpr float SleepTargetRange = 30.0f;
}

struct boss_rift_kelris : public BossAIBase
{
    explicit boss_rift_kelris(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        if (_tierConfig)
            CastIfConfigured(me, SpellResetVisual, true);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        // 计时取自原版SmartAI（creature entry 4832）：催眠术首次5–11秒、循环16–23.5秒；心灵震爆首次1–4秒、循环5–8秒。
        events.ScheduleEvent(EventSleep, Milliseconds(urand(5000, 11000)));
        events.ScheduleEvent(EventMindBlast, Milliseconds(urand(1000, 4000)));
        // T2/T3新增技能的首次施放与催眠术、心灵震爆错峰，避免同一时刻叠加。
        if (_tier >= 2)
            events.ScheduleEvent(EventShadowWordPain, 9s);
        if (_tier >= 3)
            events.ScheduleEvent(EventShadowBoltVolley, 15s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        DespawnRiftSummons();
    }

    void ConfigureTier() override
    {
        SetRaidSpellDamageMultiplier(15.0f);
        // 催眠术与心灵震爆都是可打断读条，按需求让本Boss的读条技能免疫打断。
        AddInterruptImmuneSpell(SpellSleep);
        AddInterruptImmuneSpell(SpellMindBlast);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventSleep:
                // 原版SMART_TARGET_HOSTILE_RANDOM_NOT_TOP：30码内随机目标，且排除当前坦克（playerOnly=0，可命中BOT/宠物）。
                CastIfConfigured(SelectTarget(SelectTargetMethod::Random, 0, SleepTargetRange, false, false), SpellSleep);
                events.ScheduleEvent(EventSleep, Milliseconds(urand(16000, 23500)));
                break;
            case EventMindBlast:
                CastIfConfigured(me->GetVictim(), SpellMindBlast);
                events.ScheduleEvent(EventMindBlast, Milliseconds(urand(5000, 8000)));
                break;
            case EventShadowWordPain: // T2新增：暗言术：痛，随机目标，但避开已被催眠的目标
            {
                // aura参数传负值表示“目标不能持有该光环”，防止DoT落在正在被催眠的人身上。
                Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, false, true, -(int32)SpellSleep);
                // DoT每跳基线已在rift_spell_damage.h中调谐，这里按Spell ID直接施放。
                CastIfConfigured(target, SpellShadowWordPain, true);
                // DoT持续18秒：T2用15秒刷新，T3缩短到12秒，保证全程覆盖。
                events.ScheduleEvent(EventShadowWordPain, _tier == 3 ? 12s : 15s);
                break;
            }
            case EventShadowBoltVolley: // T3新增：暗影箭雨，瞬发，以自身为中心命中范围内所有敌人
                CastFinalRaidDamageSpell(me, SpellShadowBoltVolley, SPELLVALUE_BASE_POINT0,
                    ShadowBoltVolleyTier1DirectDamage, true);
                events.ScheduleEvent(EventShadowBoltVolley, 20s);
                break;
            default:
                break;
        }
    }
};

void AddSC_boss_rift_kelris()
{
    RegisterCreatureAI(boss_rift_kelris);
}

} // namespace HeroicDungeonRift
