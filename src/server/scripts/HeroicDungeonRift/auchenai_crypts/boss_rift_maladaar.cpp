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
// 奥金顿：奥金尼地穴 - 大主教玛拉达尔（源 Entry 18373；裂隙 Entry 100199-100201）
constexpr uint32 RiftEntryStolenSoul = 102054; // 源 Entry 18441
constexpr uint32 RiftEntryMartyredAvatar = 102055; // 源 Entry 18478
constexpr int32 ShadowWordPainRaidDamagePerTick = 2000;
constexpr int32 MindFlayRaidDamagePerTick = 2000;

enum BossEvents : uint32
{
    EventRibbonOfSouls = 1, // 灵魂之带（T1基础）
    EventSoulScream,        // 灵魂尖啸（T1基础）
    EventStolenSoul,        // 偷取灵魂（T1基础）
    EventShadowWordPain,    // 暗言术：痛（T2新增）
    EventMindFlay           // 精神鞭笞（T3新增）
};

enum SummonEvents : uint32
{
    EventSoulAbility = 1,
    EventAvatarMortalStrike,
    EventAvatarSunderArmor,
    EventAvatarBladestorm
};

enum Spells : uint32
{
    SpellRibbonOfSouls = 32422,
    SpellSoulScream = 32421,
    SpellStolenSoul = 32346,
    SpellStolenSoulVisual = 32395,
    SpellMoonfire = 37328,
    SpellFireball = 37329,
    SpellMindFlaySoul = 37330,
    SpellHemorrhage = 37331,
    SpellFrostShock = 37332,
    SpellCurseOfAgony = 37334,
    SpellMortalStrikeSoul = 37335,
    SpellFreezingTrap = 37368,
    SpellHammerOfJustice = 37369,
    SpellPlagueStrike = 58839,
    SpellAvatarPhaseIn = 33422,
    SpellAvatarMortalStrike = 16856,
    SpellAvatarSunderArmor = 16145,
    SpellShadowWordPain = 72318, // 3.3.5：映像大厅暗言术：痛
    SpellMindFlay = 57941,       // 3.3.5：安卡赫特精神鞭笞
    SpellBladestorm = 63784      // 3.3.5：冠军的试炼剑刃风暴
};

constexpr char const* AggroTexts[] =
{
    "你们将以生命偿还！",
    "现在已经没有回头路了！",
    "为你们的罪过赎罪吧！"
};
constexpr char const* SoulTexts[] = { "让你的心智蒙上阴影。", "凝视你灵魂中的黑暗。" };
constexpr char const* SummonText = "起来吧，我倒下的兄弟们！现身战斗！";
constexpr char const* SlayTexts[] = { "这些墙壁将成为你的坟墓！", "现在你将永远……留在这里。" };
constexpr char const* DeathText = "这里……才是我的归宿。";
}

struct npc_rift_stolen_soul : public RiftLevel70SummonAI
{
    explicit npc_rift_stolen_soul(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void Reset() override
    {
        RemoveStolenSoulAura();
        RiftLevel70SummonAI::Reset();
    }

    void SetGUID(ObjectGuid const& guid, int32 /*id*/) override { _targetGuid = guid; }

    ObjectGuid GetGUID(int32 /*id*/ = 0) const override { return _targetGuid; }

    void DoAction(int32 playerClass) override
    {
        _playerClass = uint8(playerClass);
        _events.Reset();
        ScheduleAbilities();
    }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventSoulAbility, 1s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventSoulAbility)
            return;

        uint32 spellId = SpellMortalStrikeSoul;
        uint32 tier1Delay = 6000;
        switch (_playerClass)
        {
            case CLASS_PALADIN: spellId = SpellHammerOfJustice; tier1Delay = 6000; break;
            case CLASS_HUNTER: spellId = SpellFreezingTrap; tier1Delay = 20000; break;
            case CLASS_ROGUE: spellId = SpellHemorrhage; tier1Delay = 10000; break;
            case CLASS_PRIEST: spellId = SpellMindFlaySoul; tier1Delay = 5000; break;
            case CLASS_SHAMAN: spellId = SpellFrostShock; tier1Delay = 8000; break;
            case CLASS_MAGE: spellId = SpellFireball; tier1Delay = 5000; break;
            case CLASS_WARLOCK: spellId = SpellCurseOfAgony; tier1Delay = 20000; break;
            case CLASS_DRUID: spellId = SpellMoonfire; tier1Delay = 10000; break;
            case CLASS_DEATH_KNIGHT: spellId = SpellPlagueStrike; tier1Delay = 6000; break;
            default: break;
        }
        DoCastVictim(spellId);
        _events.ScheduleEvent(EventSoulAbility, Milliseconds(tier1Delay));
    }

    void JustDied(Unit* /*killer*/) override
    {
        RemoveStolenSoulAura();
    }

private:
    void RemoveStolenSoulAura()
    {
        if (Unit* target = ObjectAccessor::GetUnit(*me, _targetGuid))
            if (TempSummon* summon = me->ToTempSummon())
                target->RemoveAurasDueToSpell(SpellStolenSoul, summon->GetSummonerGUID());
    }

    ObjectGuid _targetGuid;
    uint8 _playerClass = CLASS_WARRIOR;
};

struct npc_rift_martyred_avatar : public RiftLevel70SummonAI
{
    explicit npc_rift_martyred_avatar(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        RiftLevel70SummonAI::IsSummonedBy(summoner);
        DoCast(me, SpellAvatarPhaseIn, true);
    }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventAvatarMortalStrike, 8500ms);
        _events.ScheduleEvent(EventAvatarSunderArmor, 6500ms);
        if (_tier >= 3)
            _events.ScheduleEvent(EventAvatarBladestorm, 12s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventAvatarMortalStrike:
                DoCastVictim(SpellAvatarMortalStrike);
                _events.ScheduleEvent(EventAvatarMortalStrike, 14s);
                break;
            case EventAvatarSunderArmor:
                DoCastVictim(SpellAvatarSunderArmor);
                _events.ScheduleEvent(EventAvatarSunderArmor, 10s);
                break;
            case EventAvatarBladestorm:
                DoCast(me, SpellBladestorm);
                _events.ScheduleEvent(EventAvatarBladestorm, 22s);
                break;
            default:
                break;
        }
    }
};

struct boss_rift_maladaar : public BossAIBase
{
    explicit boss_rift_maladaar(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _avatarSummoned = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(AggroTexts[urand(0, 2)], LANG_UNIVERSAL);
        events.ScheduleEvent(EventRibbonOfSouls, 5s);
        events.ScheduleEvent(EventSoulScream, 15s);
        events.ScheduleEvent(EventStolenSoul, 25s);
        if (_tier >= 2)
            events.ScheduleEvent(EventShadowWordPain, 9s);
        if (_tier >= 3)
            events.ScheduleEvent(EventMindFlay, 17s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (_avatarSummoned || !me->HealthBelowPctDamaged(25, damage))
            return;

        _avatarSummoned = true;
        me->Yell(SummonText, LANG_UNIVERSAL);
        SummonTieredCreature(RiftEntryMartyredAvatar, me->GetRandomNearPosition(3.0f), 1.0f, 0.9f,
            TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer() && urand(0, 1))
            me->Yell(SlayTexts[urand(0, 1)], LANG_UNIVERSAL, victim);
    }

    void JustDied(Unit* killer) override
    {
        me->Yell(DeathText, LANG_UNIVERSAL);
        BossAIBase::JustDied(killer);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        if (!summon || summon->GetEntry() != RiftEntryStolenSoul)
            return;

        if (Unit* target = ObjectAccessor::GetUnit(*me, summon->AI()->GetGUID()))
            target->RemoveAurasDueToSpell(SpellStolenSoul, me->GetGUID());
    }

    void ConfigureTier() override
    {
        SetRaidSpellDamageMultiplier(2.5f);
        AddInterruptImmuneSpell(SpellMindFlay);
    }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventRibbonOfSouls:
                CastIfConfigured(SelectRandomPlayer(), SpellRibbonOfSouls);
                events.ScheduleEvent(EventRibbonOfSouls, Milliseconds(urand(10000, 20000)));
                break;
            case EventSoulScream:
                CastIfConfigured(me, SpellSoulScream);
                events.ScheduleEvent(EventSoulScream, Milliseconds(urand(15000, 25000)));
                break;
            case EventStolenSoul:
                SummonStolenSoul();
                events.ScheduleEvent(EventStolenSoul, Milliseconds(urand(25000, 30000)));
                break;
            case EventShadowWordPain: // T2新增：3.3.5暗言术：痛，固定14秒避免Tier 3追平鞭笞
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellShadowWordPain, SPELLVALUE_BASE_POINT0,
                    ShadowWordPainRaidDamagePerTick);
                events.ScheduleEvent(EventShadowWordPain, 14s);
                break;
            case EventMindFlay: // T3新增：3.3.5精神鞭笞，以19秒周期错开暗言术：痛
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellMindFlay, SPELLVALUE_BASE_POINT0,
                    MindFlayRaidDamagePerTick);
                events.ScheduleEvent(EventMindFlay, 19s);
                break;
            default:
                break;
        }
    }

private:
    void SummonStolenSoul()
    {
        Unit* target = SelectRandomPlayer();
        if (!target)
            return;

        me->Yell(SoulTexts[urand(0, 1)], LANG_UNIVERSAL, target);
        CastIfConfigured(target, SpellStolenSoul);
        if (Creature* soul = SummonTieredCreature(RiftEntryStolenSoul, me->GetRandomNearPosition(2.0f), 0.55f, 0.65f,
            TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10 * IN_MILLISECONDS))
        {
            soul->CastSpell(soul, SpellStolenSoulVisual, false);
            soul->SetDisplayId(target->GetDisplayId());
            soul->AI()->SetGUID(target->GetGUID());
            if (Player* player = target->ToPlayer())
                soul->AI()->DoAction(player->getClass());
            soul->AI()->AttackStart(target);
        }
    }

    bool _avatarSummoned = false;
};

void AddSC_boss_rift_maladaar()
{
    RegisterCreatureAI(boss_rift_maladaar);
    RegisterCreatureAI(npc_rift_stolen_soul);
    RegisterCreatureAI(npc_rift_martyred_avatar);
}

} // namespace HeroicDungeonRift
