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
// 裂隙开战同时召唤其余6名成员，替代原副本依次激活；各成员按原版/T1基础技能战斗。

enum BossEvents : uint32
{
    EventShadowBoltVolley = 1, // 暗影箭雨（原版/T1基础）
    EventImmolate,             // 献祭（原版/T1基础）
    EventCurseOfWeakness,      // 虚弱诅咒（T2新增，点名）
    EventTier3Skill            // 暗影箭（T3新增，瞬发）
};

// 以下均为原版同伴/T1基础技能；各成员使用独立EventMap。
enum HaterelEvents : uint32
{
    EventHateShadowBolt = 1, // 仇恨者：暗影箭
    EventHateStrike          // 仇恨者：打击
};

enum AngerrelEvents : uint32
{
    EventAngerStrike = 1,  // 愤怒者：打击
    EventAngerSunderArmor, // 愤怒者：破甲攻击
    EventAngerShieldBlock, // 愤怒者：盾牌格挡
    EventAngerShieldWall   // 愤怒者：盾墙
};

enum VilerelEvents : uint32
{
    EventVileMindBlast = 1,   // 邪恶者：心灵震爆
    EventVilePowerWordShield, // 邪恶者：真言术：盾
    EventVilePrayerOfHealing, // 邪恶者：治疗祷言
    EventVileHeal             // 邪恶者：治疗术
};

enum SeethrelEvents : uint32
{
    EventSeethFrostbolt = 1, // 沸腾者：寒冰箭
    EventSeethConeOfCold,    // 沸腾者：冰锥术
    EventSeethFrostNova,     // 沸腾者：冰霜新星
    EventSeethFrostWard      // 沸腾者：防护冰霜结界
};

enum DoperelEvents : uint32
{
    EventDopeSinisterStrike = 1, // 愚昧者：影袭
    EventDopeRupture,            // 愚昧者：割裂
    EventDopeGouge,              // 愚昧者：凿击
    EventDopeEvasion,            // 愚昧者：闪避
    EventDopeBackstab            // 愚昧者：背刺
};

enum Spells : uint32
{
    // 主Boss 末日之链 Doom'rel
    SpellShadowBoltVolley = 15245, // 暗影箭雨（原版/T1基础）
    SpellImmolate = 12742,         // 献祭（原版/T1基础）
    SpellCurseOfWeakness = 12493,  // 虚弱诅咒（T2新增）
    SpellDemonArmor = 13787,       // 魔甲术（原版/T1基础）
    SpellShadowBolt = 20791,       // 暗影箭（T3新增）
    // 仇恨者 Hate'rel（原版同伴/T1基础）
    SpellHateRelShadowBolt = 15232, // 暗影箭
    SpellStrike = 15580,            // 打击
    // 愤怒者 Anger'rel（原版同伴/T1基础）
    SpellSunderArmor = 11971,   // 破甲攻击
    SpellShieldBlock = 12169,   // 盾牌格挡
    SpellShieldWall = 15062,    // 盾墙
    // 邪恶者 Vile'rel（原版同伴/T1基础）
    SpellMindBlast = 15587,       // 心灵震爆
    SpellPowerWordShield = 11974, // 真言术：盾
    SpellPrayerOfHealing = 15585, // 治疗祷言
    SpellHeal = 15586,            // 治疗术
    // 沸腾者 Seeth'rel（原版同伴/T1基础）
    SpellFrostArmor = 12544, // 霜甲术
    SpellFrostbolt = 12675,  // 寒冰箭
    SpellConeOfCold = 15244, // 冰锥术
    SpellFrostNova = 12674,  // 冰霜新星
    SpellFrostWard = 15044,  // 防护冰霜结界
    // 愚昧者 Dope'rel（原版同伴/T1基础）
    SpellSinisterStrike = 15581, // 影袭
    SpellRupture = 15583,        // 割裂
    SpellGouge = 12540,          // 凿击
    SpellEvasion = 15087,        // 闪避
    SpellBackstab = 15582        // 背刺
};

constexpr int32 ShadowBoltTier1DirectDamage = 4500;

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
        _events.ScheduleEvent(EventHateShadowBolt, 3000ms);
        _events.ScheduleEvent(EventHateStrike, 6000ms);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventHateShadowBolt:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SpellHateRelShadowBolt);
                _events.ScheduleEvent(EventHateShadowBolt, 3500ms);
                break;
            case EventHateStrike:
                DoCastVictim(SpellStrike);
                _events.ScheduleEvent(EventHateStrike, 7000ms);
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
        _events.ScheduleEvent(EventAngerStrike, 5000ms);
        _events.ScheduleEvent(EventAngerSunderArmor, 8000ms);
        _events.ScheduleEvent(EventAngerShieldBlock, 12000ms);
        _events.ScheduleEvent(EventAngerShieldWall, 20000ms);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventAngerStrike:
                DoCastVictim(SpellStrike);
                _events.ScheduleEvent(EventAngerStrike, 6000ms);
                break;
            case EventAngerSunderArmor:
                DoCastVictim(SpellSunderArmor);
                _events.ScheduleEvent(EventAngerSunderArmor, 9000ms);
                break;
            case EventAngerShieldBlock:
                DoCast(me, SpellShieldBlock);
                _events.ScheduleEvent(EventAngerShieldBlock, 14000ms);
                break;
            case EventAngerShieldWall:
                DoCast(me, SpellShieldWall);
                _events.ScheduleEvent(EventAngerShieldWall, 24000ms);
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
        _events.ScheduleEvent(EventVileMindBlast, 4000ms);
        _events.ScheduleEvent(EventVilePowerWordShield, 15000ms);
        _events.ScheduleEvent(EventVilePrayerOfHealing, 18000ms);
        _events.ScheduleEvent(EventVileHeal, 9000ms);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventVileMindBlast:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 30.0f, true))
                    DoCast(target, SpellMindBlast);
                _events.ScheduleEvent(EventVileMindBlast, 5000ms);
                break;
            case EventVilePowerWordShield:
                DoCast(me, SpellPowerWordShield);
                _events.ScheduleEvent(EventVilePowerWordShield, 17000ms);
                break;
            case EventVilePrayerOfHealing:
                DoCast(me, SpellPrayerOfHealing);
                _events.ScheduleEvent(EventVilePrayerOfHealing, 20000ms);
                break;
            case EventVileHeal:
                if (me->HealthBelowPct(50))
                    DoCast(me, SpellHeal);
                _events.ScheduleEvent(EventVileHeal, 10000ms);
                break;
            default:
                break;
        }
    }
};

// 忧郁者 Gloom'rel：原版同伴/T1基础，无战斗技能，仅普通近战
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
    }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventSeethFrostbolt, 3000ms);
        _events.ScheduleEvent(EventSeethConeOfCold, 10000ms);
        _events.ScheduleEvent(EventSeethFrostNova, 14000ms);
        _events.ScheduleEvent(EventSeethFrostWard, 18000ms);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventSeethFrostbolt:
                DoCastVictim(SpellFrostbolt);
                _events.ScheduleEvent(EventSeethFrostbolt, 3500ms);
                break;
            case EventSeethConeOfCold:
                DoCast(me, SpellConeOfCold);
                _events.ScheduleEvent(EventSeethConeOfCold, 12000ms);
                break;
            case EventSeethFrostNova:
                DoCast(me, SpellFrostNova);
                _events.ScheduleEvent(EventSeethFrostNova, 16000ms);
                break;
            case EventSeethFrostWard:
                DoCast(me, SpellFrostWard);
                _events.ScheduleEvent(EventSeethFrostWard, 20000ms);
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
        _events.ScheduleEvent(EventDopeSinisterStrike, 4000ms);
        _events.ScheduleEvent(EventDopeRupture, 9000ms);
        _events.ScheduleEvent(EventDopeGouge, 13000ms);
        _events.ScheduleEvent(EventDopeEvasion, 18000ms);
        _events.ScheduleEvent(EventDopeBackstab, 11000ms);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventDopeSinisterStrike:
                DoCastVictim(SpellSinisterStrike);
                _events.ScheduleEvent(EventDopeSinisterStrike, 5000ms);
                break;
            case EventDopeRupture:
                DoCastVictim(SpellRupture);
                _events.ScheduleEvent(EventDopeRupture, 10000ms);
                break;
            case EventDopeGouge:
                DoCastVictim(SpellGouge);
                _events.ScheduleEvent(EventDopeGouge, 15000ms);
                break;
            case EventDopeEvasion:
                DoCast(me, SpellEvasion);
                _events.ScheduleEvent(EventDopeEvasion, 22000ms);
                break;
            case EventDopeBackstab:
                DoCastVictim(SpellBackstab);
                _events.ScheduleEvent(EventDopeBackstab, 13000ms);
                break;
            default:
                break;
        }
    }
};

struct boss_rift_the_seven : public BossAIBase
{
    explicit boss_rift_the_seven(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _voidwalkersSummoned = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(TheSevenAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(TheSevenAggroSound);
        CastIfConfigured(me, SpellDemonArmor, true);

        // 裂隙开战同时召唤其余6名成员，替代原副本依次激活
        SummonMembers();

        events.ScheduleEvent(EventShadowBoltVolley, 10000ms);
        events.ScheduleEvent(EventImmolate, 7000ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventCurseOfWeakness, 12s); // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 9s);       // T3新增
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        // 原版/T1阶段召唤：血量低于51%时召唤虚空行者，数量随Tier增加
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
                events.ScheduleEvent(EventShadowBoltVolley, 14000ms);
                break;
            case EventImmolate:
                CastIfConfigured(SelectRandomPlayer(), SpellImmolate);
                events.ScheduleEvent(EventImmolate, 9000ms);
                break;
            case EventCurseOfWeakness: // T2新增：点名随机目标，瞬发
                CastIfConfigured(SelectRandomPlayer(), SpellCurseOfWeakness, true);
                events.ScheduleEvent(EventCurseOfWeakness, _tier == 3 ? 16s : 20s);
                break;
            case EventTier3Skill: // T3新增：暗影箭，瞬发
                CastFinalRaidDamageSpell(me->GetVictim(), SpellShadowBolt, SPELLVALUE_BASE_POINT0,
                    ShadowBoltTier1DirectDamage, true);
                events.ScheduleEvent(EventTier3Skill, 4s);
                break;
            default:
                break;
        }
    }

private:
    void SummonMembers()
    {
        // 裂隙同时召唤其余6名成员，替代原副本依次激活
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
