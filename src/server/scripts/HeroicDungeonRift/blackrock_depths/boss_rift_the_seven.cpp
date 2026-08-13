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
// 黑石深渊 - 七贤（The Seven）：末日之链 Doom'rel 为主Boss，其余6名成员为裂隙专用同伴。
// 原版7名成员依次激活后各自使用独立技能参与战斗，裂隙中改为开战即全部召唤、直接可攻击。

enum BossEvents : uint32
{
    EventShadowBoltVolley = 1, // 暗影箭雨（T1基础）
    EventImmolate,             // 献祭（T1基础）
    EventCurseOfWeakness,      // 虚弱诅咒（T2新增，点名）
    EventTier3Skill            // 暗影箭（T3新增，顺发）
};

// 各成员事件（每个成员的 EventMap 独立，事件ID可各自从1开始）
enum HaterelEvents : uint32 { EventHateShadowBolt = 1, EventHateStrike };
enum AngerrelEvents : uint32 { EventAngerStrike = 1, EventAngerSunderArmor, EventAngerShieldBlock, EventAngerShieldWall };
enum VilerelEvents : uint32 { EventVileMindBlast = 1, EventVilePowerWordShield, EventVilePrayerOfHealing, EventVileHeal };
enum SeethrelEvents : uint32 { EventSeethFrostbolt = 1, EventSeethConeOfCold, EventSeethFrostNova, EventSeethFrostWard };
enum DoperelEvents : uint32 { EventDopeSinisterStrike = 1, EventDopeRupture, EventDopeGouge, EventDopeEvasion, EventDopeBackstab };

enum Spells : uint32
{
    // 主Boss 末日之链 Doom'rel
    SpellShadowBoltVolley = 15245, // 暗影箭雨
    SpellImmolate = 12742,         // 献祭
    SpellCurseOfWeakness = 12493,  // 虚弱诅咒
    SpellDemonArmor = 13787,       // 魔甲术
    SpellShadowBolt = 20791,       // 暗影箭
    // 仇恨者 Hate'rel
    SpellHateRelShadowBolt = 15232, // 暗影箭
    SpellStrike = 15580,            // 打击
    // 愤怒者 Anger'rel
    SpellSunderArmor = 11971,   // 破甲攻击
    SpellShieldBlock = 12169,   // 盾牌格挡
    SpellShieldWall = 15062,    // 盾墙
    // 邪恶者 Vile'rel
    SpellMindBlast = 15587,       // 心灵震爆
    SpellPowerWordShield = 11974, // 真言术：盾
    SpellPrayerOfHealing = 15585, // 治疗祷言
    SpellHeal = 15586,            // 治疗术
    // 沸腾者 Seeth'rel
    SpellFrostArmor = 12544, // 霜甲术
    SpellFrostbolt = 12675,  // 寒冰箭
    SpellConeOfCold = 15244, // 冰锥术
    SpellFrostNova = 12674,  // 冰霜新星
    SpellFrostWard = 15044,  // 防护冰霜结界
    // 愚昧者 Dope'rel
    SpellSinisterStrike = 15581, // 影袭
    SpellRupture = 15583,        // 割裂
    SpellGouge = 12540,          // 凿击
    SpellEvasion = 15087,        // 闪避
    SpellBackstab = 15582        // 背刺
};

// 喊话（中文，对应原版 creature_text，由主Boss末日之链发出）
constexpr char const* TheSevenAggroText = "你们挑战了七贤，现在你们死定了！";
constexpr uint32 TheSevenAggroSound = 4894;
}

// 仇恨者 Hate'rel：暗影箭 + 打击
struct npc_rift_seven_haterel : public RiftSummonAI
{
    explicit npc_rift_seven_haterel(Creature* creature) : RiftSummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventHateShadowBolt, Milliseconds(TierDelay(3000, 2400, 1900)));
        _events.ScheduleEvent(EventHateStrike, Milliseconds(TierDelay(6000, 4800, 3800)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventHateShadowBolt:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SpellHateRelShadowBolt);
                _events.ScheduleEvent(EventHateShadowBolt, Milliseconds(TierDelay(3500, 2800, 2200)));
                break;
            case EventHateStrike:
                DoCastVictim(SpellStrike);
                _events.ScheduleEvent(EventHateStrike, Milliseconds(TierDelay(7000, 5500, 4500)));
                break;
            default:
                break;
        }
    }
};

// 愤怒者 Anger'rel：打击 + 破甲攻击 + 盾牌格挡 + 盾墙
struct npc_rift_seven_angerrel : public RiftSummonAI
{
    explicit npc_rift_seven_angerrel(Creature* creature) : RiftSummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventAngerStrike, Milliseconds(TierDelay(5000, 4000, 3200)));
        _events.ScheduleEvent(EventAngerSunderArmor, Milliseconds(TierDelay(8000, 6500, 5200)));
        _events.ScheduleEvent(EventAngerShieldBlock, Milliseconds(TierDelay(12000, 9500, 7500)));
        _events.ScheduleEvent(EventAngerShieldWall, Milliseconds(TierDelay(20000, 16000, 13000)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventAngerStrike:
                DoCastVictim(SpellStrike);
                _events.ScheduleEvent(EventAngerStrike, Milliseconds(TierDelay(6000, 4800, 3800)));
                break;
            case EventAngerSunderArmor:
                DoCastVictim(SpellSunderArmor);
                _events.ScheduleEvent(EventAngerSunderArmor, Milliseconds(TierDelay(9000, 7000, 5500)));
                break;
            case EventAngerShieldBlock:
                DoCast(me, SpellShieldBlock);
                _events.ScheduleEvent(EventAngerShieldBlock, Milliseconds(TierDelay(14000, 11000, 9000)));
                break;
            case EventAngerShieldWall:
                DoCast(me, SpellShieldWall);
                _events.ScheduleEvent(EventAngerShieldWall, Milliseconds(TierDelay(24000, 19000, 15500)));
                break;
            default:
                break;
        }
    }
};

// 邪恶者 Vile'rel：心灵震爆 + 真言术：盾 + 治疗祷言 + 治疗术(<50%)
struct npc_rift_seven_vilerel : public RiftSummonAI
{
    explicit npc_rift_seven_vilerel(Creature* creature) : RiftSummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventVileMindBlast, Milliseconds(TierDelay(4000, 3200, 2500)));
        _events.ScheduleEvent(EventVilePowerWordShield, Milliseconds(TierDelay(15000, 12000, 9500)));
        _events.ScheduleEvent(EventVilePrayerOfHealing, Milliseconds(TierDelay(18000, 14500, 11500)));
        _events.ScheduleEvent(EventVileHeal, Milliseconds(TierDelay(9000, 7000, 5500)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventVileMindBlast:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SpellMindBlast);
                _events.ScheduleEvent(EventVileMindBlast, Milliseconds(TierDelay(5000, 4000, 3200)));
                break;
            case EventVilePowerWordShield:
                DoCast(me, SpellPowerWordShield);
                _events.ScheduleEvent(EventVilePowerWordShield, Milliseconds(TierDelay(17000, 13500, 10500)));
                break;
            case EventVilePrayerOfHealing:
                DoCast(me, SpellPrayerOfHealing);
                _events.ScheduleEvent(EventVilePrayerOfHealing, Milliseconds(TierDelay(20000, 16000, 13000)));
                break;
            case EventVileHeal:
                if (me->HealthBelowPct(50))
                    DoCast(me, SpellHeal);
                _events.ScheduleEvent(EventVileHeal, Milliseconds(TierDelay(10000, 8000, 6500)));
                break;
            default:
                break;
        }
    }
};

// 忧郁者 Gloom'rel：原版无战斗技能，普通近战
struct npc_rift_seven_gloomrel : public RiftSummonAI
{
    explicit npc_rift_seven_gloomrel(Creature* creature) : RiftSummonAI(creature) { }
};

// 沸腾者 Seeth'rel：霜甲术 + 寒冰箭 + 冰锥术 + 冰霜新星 + 防护冰霜结界
struct npc_rift_seven_seethrel : public RiftSummonAI
{
    explicit npc_rift_seven_seethrel(Creature* creature) : RiftSummonAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        DoCast(me, SpellFrostArmor, true);
        ScheduleAbilities();
    }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventSeethFrostbolt, Milliseconds(TierDelay(3000, 2400, 1900)));
        _events.ScheduleEvent(EventSeethConeOfCold, Milliseconds(TierDelay(10000, 8000, 6500)));
        _events.ScheduleEvent(EventSeethFrostNova, Milliseconds(TierDelay(14000, 11000, 9000)));
        _events.ScheduleEvent(EventSeethFrostWard, Milliseconds(TierDelay(18000, 14500, 11500)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventSeethFrostbolt:
                DoCastVictim(SpellFrostbolt);
                _events.ScheduleEvent(EventSeethFrostbolt, Milliseconds(TierDelay(3500, 2800, 2200)));
                break;
            case EventSeethConeOfCold:
                DoCast(me, SpellConeOfCold);
                _events.ScheduleEvent(EventSeethConeOfCold, Milliseconds(TierDelay(12000, 9500, 7500)));
                break;
            case EventSeethFrostNova:
                DoCast(me, SpellFrostNova);
                _events.ScheduleEvent(EventSeethFrostNova, Milliseconds(TierDelay(16000, 13000, 10500)));
                break;
            case EventSeethFrostWard:
                DoCast(me, SpellFrostWard);
                _events.ScheduleEvent(EventSeethFrostWard, Milliseconds(TierDelay(20000, 16000, 13000)));
                break;
            default:
                break;
        }
    }
};

// 愚昧者 Dope'rel：影袭 + 割裂 + 凿击 + 闪避 + 背刺
struct npc_rift_seven_doperel : public RiftSummonAI
{
    explicit npc_rift_seven_doperel(Creature* creature) : RiftSummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventDopeSinisterStrike, Milliseconds(TierDelay(4000, 3200, 2500)));
        _events.ScheduleEvent(EventDopeRupture, Milliseconds(TierDelay(9000, 7000, 5500)));
        _events.ScheduleEvent(EventDopeGouge, Milliseconds(TierDelay(13000, 10500, 8500)));
        _events.ScheduleEvent(EventDopeEvasion, Milliseconds(TierDelay(18000, 14500, 11500)));
        _events.ScheduleEvent(EventDopeBackstab, Milliseconds(TierDelay(11000, 9000, 7200)));
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventDopeSinisterStrike:
                DoCastVictim(SpellSinisterStrike);
                _events.ScheduleEvent(EventDopeSinisterStrike, Milliseconds(TierDelay(5000, 4000, 3200)));
                break;
            case EventDopeRupture:
                DoCastVictim(SpellRupture);
                _events.ScheduleEvent(EventDopeRupture, Milliseconds(TierDelay(10000, 8000, 6500)));
                break;
            case EventDopeGouge:
                DoCastVictim(SpellGouge);
                _events.ScheduleEvent(EventDopeGouge, Milliseconds(TierDelay(15000, 12000, 9500)));
                break;
            case EventDopeEvasion:
                DoCast(me, SpellEvasion);
                _events.ScheduleEvent(EventDopeEvasion, Milliseconds(TierDelay(22000, 17500, 14000)));
                break;
            case EventDopeBackstab:
                DoCastVictim(SpellBackstab);
                _events.ScheduleEvent(EventDopeBackstab, Milliseconds(TierDelay(13000, 10500, 8500)));
                break;
            default:
                break;
        }
    }
};

struct boss_rift_the_seven : public BossAIBase
{
    explicit boss_rift_the_seven(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(TheSevenAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(TheSevenAggroSound);
        CastIfConfigured(me, SpellDemonArmor, true);

        // 原版：进战后其余6名成员加入战斗
        SummonMembers();

        ScheduleTieredEvent(EventShadowBoltVolley, 10000, 8000, 6500);
        ScheduleTieredEvent(EventImmolate, 7000, 5500, 4500);
        if (_tier >= 2)
            events.ScheduleEvent(EventCurseOfWeakness, 12s); // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 9s);       // T3新增
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        // 原版机制：血量低于51%时召唤虚空行者
        if (_voidwalkersSummoned || !me->HealthBelowPctDamaged(51, damage))
            return;

        _voidwalkersSummoned = true;
        for (uint32 i = 0; i < _tier + 1; ++i)
            SummonTieredCreature(RiftEntryVoidwalkerMinion, me->GetRandomNearPosition(6.0f), 0.6f, 0.8f);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventShadowBoltVolley:
                CastIfConfigured(me, SpellShadowBoltVolley);
                ScheduleTieredEvent(EventShadowBoltVolley, 14000, 11000, 9000);
                break;
            case EventImmolate:
                CastIfConfigured(SelectRandomPlayer(), SpellImmolate);
                ScheduleTieredEvent(EventImmolate, 9000, 7000, 5500);
                break;
            case EventCurseOfWeakness: // T2新增：点名随机目标，瞬发
                CastIfConfigured(SelectRandomPlayer(), SpellCurseOfWeakness, true);
                events.ScheduleEvent(EventCurseOfWeakness, _tier == 3 ? 16s : 20s);
                break;
            case EventTier3Skill: // T3新增：顺发
                CastIfConfigured(me->GetVictim(), SpellShadowBolt, true);
                events.ScheduleEvent(EventTier3Skill, 4s);
                break;
            default:
                break;
        }
    }

private:
    void SummonMembers()
    {
        // 原版七贤其余6名成员依次加入战斗
        SummonTieredCreature(RiftEntrySevenHateRel, me->GetRandomNearPosition(5.0f), 0.7f, 0.7f);
        SummonTieredCreature(RiftEntrySevenAngerRel, me->GetRandomNearPosition(5.0f), 0.7f, 0.7f);
        SummonTieredCreature(RiftEntrySevenVileRel, me->GetRandomNearPosition(5.0f), 0.7f, 0.7f);
        SummonTieredCreature(RiftEntrySevenGloomRel, me->GetRandomNearPosition(5.0f), 0.7f, 0.7f);
        SummonTieredCreature(RiftEntrySevenSeethRel, me->GetRandomNearPosition(5.0f), 0.7f, 0.7f);
        SummonTieredCreature(RiftEntrySevenDopeRel, me->GetRandomNearPosition(5.0f), 0.7f, 0.7f);
    }

    bool _voidwalkersSummoned = false;
};

void AddSC_boss_rift_the_seven()
{
    RegisterCreatureAI(boss_rift_the_seven);
    RegisterCreatureAI(npc_rift_seven_haterel);
    RegisterCreatureAI(npc_rift_seven_angerrel);
    RegisterCreatureAI(npc_rift_seven_vilerel);
    RegisterCreatureAI(npc_rift_seven_gloomrel);
    RegisterCreatureAI(npc_rift_seven_seethrel);
    RegisterCreatureAI(npc_rift_seven_doperel);
}

} // namespace HeroicDungeonRift
