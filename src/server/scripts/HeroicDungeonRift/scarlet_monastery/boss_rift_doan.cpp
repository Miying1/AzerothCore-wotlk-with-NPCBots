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
// 血色图书馆 - 奥法师杜安（Arcanist Doan）
enum Events : uint32
{
    EventArcaneExplosion = 1, // 奥术爆炸（T1基础，自身范围）
    EventSilence,             // 沉默（T1基础，自身范围）
    EventPolymorph,           // 变形术（T1基础，随机玩家）
    EventArcaneBubble,        // 奥术气泡（T1基础，血量<50%无敌）
    EventDetonation,          // 引爆（T1基础，气泡后范围火焰伤害）
    EventTier2Skill,          // 奥术飞弹（T2新增）
    EventTier3Skill           // 烈焰风暴（T3新增）
};

enum Spells : uint32
{
    SpellArcaneExplosion = 9433,  // 奥术爆炸
    SpellSilence = 8988,          // 沉默
    SpellPolymorph = 13323,       // 变形术
    SpellArcaneBubble = 9438,     // 奥术气泡
    SpellDetonation = 9435,       // 引爆
    SpellArcaneMissiles = 15790,  // 奥术飞弹
    SpellFlamestrike = 12468      // 烈焰风暴
};

// 喊话（中文，对应原版 creature_text）
constexpr char const* DoanAggroText = "你们不能玷污这些奥秘！";
constexpr char const* DoanDetonateText = "在正义之火中燃烧吧！";
constexpr uint32 DoanAggroSound = 5842;
constexpr uint32 DoanDetonateSound = 5843;
}

struct boss_rift_doan : public BossAIBase
{
    explicit boss_rift_doan(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(DoanAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(DoanAggroSound);

        ScheduleTieredEvent(EventArcaneExplosion, 5000, 4000, 3200);
        ScheduleTieredEvent(EventSilence, 9000, 7500, 6000);
        ScheduleTieredEvent(EventPolymorph, 7000, 5500, 4500);
        if (_tier >= 2)
            events.ScheduleEvent(EventTier2Skill, 11s); // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 16s); // T3新增
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        // 原版机制：血量低于50%时开启奥术气泡无敌并准备引爆
        if (_bubbleTriggered || !me->HealthBelowPctDamaged(50, damage))
            return;

        _bubbleTriggered = true;
        me->Yell(DoanDetonateText, LANG_UNIVERSAL);
        me->PlayDirectSound(DoanDetonateSound);
        CastIfConfigured(me, SpellArcaneBubble);
        events.ScheduleEvent(EventDetonation, 6s);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventArcaneExplosion:
                CastIfConfigured(me, SpellArcaneExplosion);
                ScheduleTieredEvent(EventArcaneExplosion, 8000, 6500, 5200);
                break;
            case EventSilence:
                CastIfConfigured(me, SpellSilence);
                ScheduleTieredEvent(EventSilence, 20000, 16000, 13000);
                break;
            case EventPolymorph:
                CastIfConfigured(SelectRandomPlayer(), SpellPolymorph);
                ScheduleTieredEvent(EventPolymorph, 20000, 16000, 13000);
                break;
            case EventArcaneBubble:
                break;
            case EventDetonation:
                CastRaidTunedSpell(me, SpellDetonation, 4000);
                break;
            case EventTier2Skill: // T2新增：奥术飞弹，读条不可打断
                CastIfConfigured(me->GetVictim(), SpellArcaneMissiles, true);
                events.ScheduleEvent(EventTier2Skill, _tier == 3 ? 8s : 10s);
                break;
            case EventTier3Skill: // T3新增：烈焰风暴，点名随机目标，读条不可打断
                CastIfConfigured(SelectRandomPlayer(), SpellFlamestrike, true);
                events.ScheduleEvent(EventTier3Skill, 22s);
                break;
            default:
                break;
        }
    }

private:
    bool _bubbleTriggered = false;
};

void AddSC_boss_rift_doan()
{
    RegisterCreatureAI(boss_rift_doan);
}

} // namespace HeroicDungeonRift
