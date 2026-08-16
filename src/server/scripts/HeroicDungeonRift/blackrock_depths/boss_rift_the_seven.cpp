/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"

namespace HeroicDungeonRift
{
namespace
{
// 黑石深渊 - 七贤（The Seven）：末日之链 Doom'rel 为遭遇控制实体，其余6名成员为裂隙专用Boss。
// 保留原版开战保存、30秒依次激活和全灭完成流程；各成员伤害按Boss倍率计算，总血量共享一份Boss预算。

enum BossEvents : uint32
{
    EventShadowBoltVolley = 1, // 暗影箭雨（原版/T1基础）
    EventImmolate,             // 献祭（原版/T1基础）
    EventCurseOfWeakness,      // 虚弱诅咒（T2新增，点名）
    EventTier3Skill,           // 暗影箭（T3新增，瞬发）
    EventActivateMember        // 按原版间隔激活下一名七贤
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
constexpr std::chrono::seconds MemberActivationInterval = 30s;
// 七贤每个成员使用单Boss血量预算的1/7×1.1，七名成员总血量约为单Boss的1.1倍。
constexpr float MemberHealthCoefficient = 1.1f / 7.0f;
constexpr float SevenBossMeleeDamageCoefficient = 35.0f / 12.0f;
constexpr uint32 TombOfSevenInstanceData = 4;
constexpr uint32 SevenFriendlyFaction = 35;
constexpr uint32 SevenHostileFaction = 754;
constexpr int32 ActionStartEncounter = 1;
constexpr int32 ActionMemberDied = 2;

// 喊话（中文，对应原版 creature_text，由主Boss末日之链发出）
constexpr char const* TheSevenAggroText = "你们挑战了七贤，现在你们死定了！";
constexpr uint32 TheSevenAggroSound = 4894;
}

class RiftSevenMemberAI : public RiftSummonAI
{
public:
    explicit RiftSevenMemberAI(Creature* creature) : RiftSummonAI(creature) { }

    void Reset() override
    {
        RiftSummonAI::Reset();
        _activated = false;
        me->SetReactState(REACT_PASSIVE);
        me->SetFaction(SevenFriendlyFaction);
        me->SetImmuneToPC(true);
        me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
    }

    void IsSummonedBy(WorldObject* /*summoner*/) override { }

    void SetData(uint32 type, uint32 data) override
    {
        RiftSummonAI::SetData(type, data);
        if (type == RiftDataActivate && data && !_activated)
            Activate();
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (!_activated)
            return;

        me->RemoveAllAuras();
        me->GetThreatMgr().ClearAllThreat();
        me->CombatStop(true);
        me->SetLootRecipient(nullptr);
        if (me->IsAlive())
            me->GetMotionMaster()->MoveTargetedHome();
    }

protected:
    void Activate()
    {
        _activated = true;
        me->SetFaction(SevenHostileFaction);
        me->SetImmuneToPC(false);
        me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        me->SetReactState(REACT_AGGRESSIVE);
        _events.Reset();
        ScheduleAbilities();

        if (Player* target = me->SelectNearestPlayer(130.0f))
        {
            AttackStart(target);
            DoZoneInCombat();
        }
    }

private:
    bool _activated = false;
};

// 仇恨者 Hate'rel：暗影箭 + 打击
struct npc_rift_seven_haterel : public RiftSevenMemberAI
{
    explicit npc_rift_seven_haterel(Creature* creature) : RiftSevenMemberAI(creature) { }

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
struct npc_rift_seven_angerrel : public RiftSevenMemberAI
{
    explicit npc_rift_seven_angerrel(Creature* creature) : RiftSevenMemberAI(creature) { }

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
struct npc_rift_seven_vilerel : public RiftSevenMemberAI
{
    explicit npc_rift_seven_vilerel(Creature* creature) : RiftSevenMemberAI(creature) { }

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
struct npc_rift_seven_gloomrel : public RiftSevenMemberAI
{
    explicit npc_rift_seven_gloomrel(Creature* creature) : RiftSevenMemberAI(creature) { }
};

// 沸腾者 Seeth'rel：霜甲术 + 寒冰箭 + 冰锥术 + 冰霜新星 + 防护冰霜结界
struct npc_rift_seven_seethrel : public RiftSevenMemberAI
{
    explicit npc_rift_seven_seethrel(Creature* creature) : RiftSevenMemberAI(creature) { }

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
struct npc_rift_seven_doperel : public RiftSevenMemberAI
{
    explicit npc_rift_seven_doperel(Creature* creature) : RiftSevenMemberAI(creature) { }

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
        _encounterStarted = false;
        _activeMemberIndex = 0;
        _deadMemberCount = 0;
        _doomrelActive = false;
        _memberGuids.clear();

        if (_tier >= 1 && _tier <= MaxTier)
        {
            // 末日之链与原版一致：初始为友方交互单位，由玩家对话启动整个七贤事件。
            me->SetFaction(SevenFriendlyFaction);
            me->SetImmuneToPC(true);
            me->SetReactState(REACT_PASSIVE);
            me->SetNpcFlag(UNIT_NPC_FLAG_GOSSIP);
            ScaleControllerHealth();
            SummonMembers();
        }
    }

    void sGossipHello(Player* player) override
    {
        if (_encounterStarted)
            return;

        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "我们准备好了，开始吧。", GOSSIP_SENDER_MAIN,
            GOSSIP_ACTION_INFO_DEF + 1);
        SendGossipMenuFor(player, player->GetGossipTextId(me), me->GetGUID());
    }

    void sGossipSelect(Player* player, uint32 sender, uint32 action) override
    {
        if (_encounterStarted || sender != GOSSIP_SENDER_MAIN || action != GOSSIP_ACTION_INFO_DEF + 1)
            return;

        CloseGossipMenuFor(player);
        DoAction(ActionStartEncounter);
    }

    void DoAction(int32 action) override
    {
        if (action == ActionStartEncounter)
        {
            if (_encounterStarted || _memberGuids.size() != 6)
                return;

            _encounterStarted = true;
            if (InstanceScript* instance = me->GetInstanceScript())
                instance->SetData(TombOfSevenInstanceData, IN_PROGRESS);
            me->RemoveNpcFlag(UNIT_NPC_FLAG_GOSSIP);
            me->Yell(TheSevenAggroText, LANG_UNIVERSAL);
            me->PlayDirectSound(TheSevenAggroSound);
            // 原版 TombOfSevenEvent 首次触发前等待30秒。
            events.ScheduleEvent(EventActivateMember, MemberActivationInterval);
        }
        else if (action == ActionMemberDied)
        {
            if (!_encounterStarted || _deadMemberCount >= 6)
                return;

            ++_deadMemberCount;
            if (_deadMemberCount >= 6)
                ActivateDoomrel();
            else if (_encounterStarted && _activeMemberIndex < _memberGuids.size())
                events.ScheduleEvent(EventActivateMember, MemberActivationInterval);
        }
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (!_doomrelActive)
            return;

        CastIfConfigured(me, SpellDemonArmor, true);
        events.ScheduleEvent(EventShadowBoltVolley, 10000ms);
        events.ScheduleEvent(EventImmolate, 7000ms);
        if (_tier >= 2)
            events.ScheduleEvent(EventCurseOfWeakness, 12s); // T2新增
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 9s);       // T3新增
    }

    void JustDied(Unit* killer) override
    {
        if (InstanceScript* instance = me->GetInstanceScript())
            instance->SetData(TombOfSevenInstanceData, DONE);
        BossAIBase::JustDied(killer);
    }

    void EnterEvadeMode(EvadeReason /*why*/) override
    {
        if (!_encounterStarted)
            return;

        events.Reset();
        me->RemoveAllAuras();
        me->GetThreatMgr().ClearAllThreat();
        me->CombatStop(true);
        me->SetLootRecipient(nullptr);
        if (me->IsAlive())
            me->GetMotionMaster()->MoveTargetedHome();
    }

    void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
    {
        if (summon && IsSevenMemberEntry(summon->GetEntry()))
            DoAction(ActionMemberDied);
    }

    void UpdateAI(uint32 diff) override
    {
        // 控制实体在轮到末日之链前没有仇恨目标，但依次激活计时仍必须继续推进。
        if (!_doomrelActive)
        {
            events.Update(diff);
            if (uint32 eventId = events.ExecuteEvent())
                ExecuteRiftEvent(eventId);
            return;
        }

        BossAIBase::UpdateAI(diff);
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
            case EventActivateMember:
                ActivateNextMember();
                break;
            default:
                break;
        }
    }

private:
    static bool IsSevenMemberEntry(uint32 entry)
    {
        switch (entry)
        {
            case RiftEntrySevenHateRel:
            case RiftEntrySevenAngerRel:
            case RiftEntrySevenVileRel:
            case RiftEntrySevenGloomRel:
            case RiftEntrySevenSeethRel:
            case RiftEntrySevenDopeRel:
                return true;
            default:
                return false;
        }
    }

    void ScaleControllerHealth()
    {
        uint64 scaledHealth = uint64(me->GetMaxHealth()) * MemberHealthCoefficient;
        scaledHealth = std::max<uint64>(1, std::min<uint64>(scaledHealth, std::numeric_limits<uint32>::max()));
        me->SetCreateHealth(uint32(scaledHealth));
        me->SetMaxHealth(uint32(scaledHealth));
        me->SetFullHealth();
    }

    void SummonMembers()
    {
        // 激活顺序与原版实例一致：Anger'rel、Seeth'rel、Dope'rel、Gloom'rel、Vile'rel、Hate'rel，最后Doom'rel。
        for (uint32 entry : { RiftEntrySevenAngerRel, RiftEntrySevenSeethRel, RiftEntrySevenDopeRel,
            RiftEntrySevenGloomRel, RiftEntrySevenVileRel, RiftEntrySevenHateRel })
        {
            if (Creature* member = SummonTieredCreature(entry, me->GetRandomNearPosition(5.0f),
                MemberHealthCoefficient, SevenBossMeleeDamageCoefficient,
                TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS, true))
                _memberGuids.push_back(member->GetGUID());
        }
    }

    void ActivateNextMember()
    {
        if (_activeMemberIndex >= _memberGuids.size())
            return;

        if (Creature* member = ObjectAccessor::GetCreature(*me, _memberGuids[_activeMemberIndex]))
            member->AI()->SetData(RiftDataActivate, 1);
        ++_activeMemberIndex;

        if (_activeMemberIndex < _memberGuids.size())
            events.ScheduleEvent(EventActivateMember, MemberActivationInterval);
    }

    void ActivateDoomrel()
    {
        if (_doomrelActive)
            return;

        _doomrelActive = true;
        events.Reset();
        me->SetFaction(SevenHostileFaction);
        me->SetImmuneToPC(false);
        me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        me->SetReactState(REACT_AGGRESSIVE);

        if (Player* target = me->SelectNearestPlayer(130.0f))
        {
            AttackStart(target);
            DoZoneInCombat();
        }
    }

    bool _voidwalkersSummoned = false;
    bool _encounterStarted = false;
    uint8 _activeMemberIndex = 0;
    uint8 _deadMemberCount = 0;
    bool _doomrelActive = false;
    std::vector<ObjectGuid> _memberGuids;
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
