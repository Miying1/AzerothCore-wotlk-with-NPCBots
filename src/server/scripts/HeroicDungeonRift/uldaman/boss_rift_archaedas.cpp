/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"

#include <algorithm>
#include <array>
#include <limits>
#include <span>

namespace HeroicDungeonRift
{
namespace
{
// 裂隙版以动态召唤的四类石像守卫替代原版阶段中预置并唤醒的场景单位，避免依赖副本原生布置。
enum BossEvents : uint32
{
    EventGroundTremor = 1, // 大地震颤（原版/T1基础）
    EventSupportWave       // 塑石者与看守者支援波（全Tier；裂隙版原版阶段替代机制）
};

enum GuardEvents : uint32
{
    EventGuardianWhirlwind = 1, // 旋风斩（全Tier裂隙地灵守护者技能）
    EventWarderTrample,          // 践踏（全Tier裂隙宝库守卫技能）
    EventHallshaperHeal,         // 重铸（全Tier裂隙地灵塑石者技能）
    EventAwaken                  // 石像苏醒定时器（复刻原版4秒苏醒动画）
};

enum Spells : uint32
{
    SpellGroundTremor = 6524,          // 大地震颤（原版/T1基础）
    SpellGuardianWhirlwind = 17207,    // 旋风斩（全Tier裂隙守护者技能）
    SpellWarderTrample = 5568,         // 践踏（全Tier裂隙宝库守卫技能）
    SpellHallshaperHealVisual = 10260, // 重铸（全Tier裂隙塑石者治疗视觉）
    SpellStoned = 10255,               // 石化（原版石像沉睡状态：被动+免疫）
    SpellAwakenVisual = 10254          // 石像苏醒视觉（原版唤醒法术的视觉特效）
};

class EarthenGuardAI : public ScriptedAI
{
public:
    explicit EarthenGuardAI(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _events.Reset();
        _tier = 1;
        _damagePermille = 1000;
        _awakening = false;
        ScheduleAbilities();
    }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        // 复刻原版奥达曼石像唤醒动画：召唤物先以石化状态刷出（被动+免疫，像雕像一样站立），
        // 播放苏醒视觉(10254)，4秒后由EventAwaken移除石化并进入战斗。
        _awakening = true;
        _events.Reset();
        _events.ScheduleEvent(EventAwaken, 4s);
        me->CastSpell(me, SpellStoned, true);
        me->CastSpell(me, SpellAwakenVisual, true);
    }

    void SetData(uint32 type, uint32 data) override
    {
        if (type == RiftDataTier)
        {
            _tier = uint8(std::clamp<uint32>(data, 1, MaxTier));
            _events.Reset();
            // 石化唤醒流程尚未结束，仅保留苏醒定时器；技能在苏醒后才开始计时。
            if (_awakening)
                _events.ScheduleEvent(EventAwaken, 4s);
            else
                ScheduleAbilities();
        }
        else if (type == RiftDataDamagePermille)
            _damagePermille = std::max<uint32>(1, data) * 15;
    }

    void DamageDealt(Unit* /*victim*/, uint32& damage, DamageEffectType damageType, SpellSchoolMask /*damageSchoolMask*/) override
    {
        if (damageType != DIRECT_DAMAGE)
        {
            uint64 scaledDamage = uint64(damage) * _damagePermille / 1000;
            damage = uint32(std::min<uint64>(scaledDamage, std::numeric_limits<uint32>::max()));
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        // 苏醒流程：石化期间单位处于被动+免疫，不进入战斗，仅等待苏醒定时器到期。
        while (uint32 eventId = _events.ExecuteEvent())
        {
            if (eventId == EventAwaken)
            {
                _awakening = false;
                // 移除石化并恢复可攻击（spell_uldaman_stoned_aura 的移除回调也会恢复REACT_AGGRESSIVE），
                // 这里显式再兜底一次，与BossAIBase::Reset的清理保持一致。
                me->RemoveAurasDueToSpell(SpellStoned);
                me->SetStandState(UNIT_STAND_STATE_STAND);
                me->SetReactState(REACT_AGGRESSIVE);
                me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC |
                    UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE_2 | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE);
                me->RemoveUnitFlag2(UNIT_FLAG2_FEIGN_DEATH | UNIT_FLAG2_HIDE_BODY);

                // 苏醒后开始技能计时并进入战斗（复刻原版"Remove Aura → Set In Combat With Zone"）。
                ScheduleAbilities();
                me->SetInCombatWithZone();

                // 兜底锁定目标：优先Boss当前目标，确保石像苏醒后立即进攻。
                if (Unit* victim = me->GetVictim())
                    AttackStart(victim);
                else if (TempSummon* summon = me->ToTempSummon())
                    if (Unit* owner = summon->GetSummonerUnit())
                        if (Unit* bossVictim = owner->GetVictim())
                            AttackStart(bossVictim);
            }
            else
                ExecuteAbility(eventId);
        }

        // 石化中不进行常规战斗逻辑。
        if (_awakening)
            return;

        if (!UpdateVictim())
            return;

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        DoMeleeAttackIfReady();
    }

protected:
    virtual void ScheduleAbilities() { }
    virtual void ExecuteAbility(uint32 /*eventId*/) { }

    EventMap _events;
    uint8 _tier = 1;
    uint32 _damagePermille = 1000;
    bool _awakening = false;
};

// 原版奥达曼石像守卫的固定刷点坐标（creature 表，map 70）。
// 裂隙版按这些原版位置刷出召唤物，并在该位置播放石像苏醒动画。
// 各类守卫存活上限均不超过其刷点数，刷点足够一一对应。
std::span<Position const> GetGuardSpawns(uint32 entry)
{
    static std::array<Position, 4> const vaultWarderSpawns = {{
        { 48.5035f, 251.146f, -52.1147f, 0.558505f },
        { 51.9748f, 243.765f, -52.1147f, 0.593412f },
        { 117.541f, 244.48f, -52.1213f, 2.09439f },
        { 90.9648f, 301.704f, -52.1213f, 5.28835f },
    }};

    static std::array<Position, 6> const guardianSpawns = {{
        { 106.737f, 290.618f, -51.6997f, 4.59022f },
        { 89.731f, 282.814f, -51.6997f, 5.67232f },
        { 102.634f, 255.188f, -51.6997f, 1.48353f },
        { 88.0912f, 265.328f, -51.6997f, 0.436332f },
        { 120.997f, 280.152f, -51.6997f, 3.61283f },
        { 119.465f, 263.298f, -51.6997f, 2.56563f },
    }};

    static std::array<Position, 12> const hallshaperSpawns = {{
        { 125.958f, 296.112f, -52.1213f, 3.9619f },
        { 83.3431f, 249.266f, -52.1213f, 0.837758f },
        { 93.7888f, 243.029f, -52.1213f, 1.23918f },
        { 115.438f, 302.439f, -52.1213f, 4.34587f },
        { 103.067f, 304.25f, -52.1213f, 4.74729f },
        { 127.731f, 251.6f, -52.1213f, 2.40855f },
        { 134.081f, 262.03f, -52.1213f, 2.80998f },
        { 74.8823f, 283.399f, -52.1213f, 5.91667f },
        { 105.984f, 241.057f, -52.1213f, 1.64061f },
        { 73.0399f, 271.531f, -52.1213f, 0.017453f },
        { 81.1552f, 293.988f, -52.1213f, 5.51524f },
        { 135.904f, 274.323f, -52.1213f, 3.21141f },
    }};

    static std::array<Position, 32> const custodianSpawns = {{
        { 132.846f, 259.723f, -52.1213f, 2.60054f },
        { 79.827f, 292.044f, -52.1213f, 5.84685f },
        { 119.714f, 299.935f, -52.1213f, 4.18879f },
        { 73.6493f, 276.098f, -52.1213f, 6.0912f },
        { 96.1878f, 242.548f, -52.1213f, 1.36136f },
        { 85.3935f, 247.961f, -52.1213f, 0.959931f },
        { 87.3029f, 246.851f, -52.1213f, 1.06465f },
        { 107.897f, 303.66f, -52.1213f, 4.57276f },
        { 110.552f, 303.294f, -52.1213f, 4.57276f },
        { 78.5479f, 289.994f, -52.1213f, 5.70723f },
        { 77.1684f, 287.766f, -52.1213f, 5.81195f },
        { 75.9659f, 285.748f, -52.1213f, 5.65487f },
        { 74.2951f, 280.898f, -52.1213f, 6.0912f },
        { 73.9196f, 278.483f, -52.1213f, 6.12611f },
        { 73.2958f, 273.634f, -52.1213f, 6.00393f },
        { 130.37f, 255.511f, -52.1213f, 2.58309f },
        { 131.642f, 257.691f, -52.1213f, 2.61799f },
        { 91.5691f, 244.263f, -52.1213f, 1.0472f },
        { 98.6404f, 242.097f, -52.1213f, 1.36136f },
        { 101.175f, 241.734f, -52.1213f, 1.41372f },
        { 134.815f, 264.562f, -52.1213f, 2.9147f },
        { 135.212f, 267.042f, -52.1213f, 2.98451f },
        { 135.559f, 269.673f, -52.1213f, 2.98451f },
        { 135.927f, 272.091f, -52.1213f, 3.01942f },
        { 123.935f, 297.48f, -52.1213f, 4.15388f },
        { 117.584f, 301.153f, -52.1213f, 4.24115f },
        { 103.572f, 241.414f, -52.1213f, 1.36136f },
        { 121.925f, 298.689f, -52.1213f, 4.15388f },
        { 112.992f, 302.948f, -52.1213f, 4.50295f },
        { 105.498f, 304.0f, -52.1213f, 4.59022f },
        { 129.047f, 253.491f, -52.1213f, 2.53073f },
        { 89.4681f, 245.446f, -52.1213f, 1.06465f },
    }};

    switch (entry)
    {
        case RiftEntryVaultWarder:
            return vaultWarderSpawns;
        case RiftEntryEarthenGuardian:
            return guardianSpawns;
        case RiftEntryEarthenHallshaper:
            return hallshaperSpawns;
        case RiftEntryEarthenCustodian:
            return custodianSpawns;
        default:
            return {};
    }
}
}

struct npc_rift_earthen_guardian : public EarthenGuardAI
{
    explicit npc_rift_earthen_guardian(Creature* creature) : EarthenGuardAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventGuardianWhirlwind, 9s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventGuardianWhirlwind)
            return;

        DoCast(me, SpellGuardianWhirlwind);
        _events.ScheduleEvent(EventGuardianWhirlwind, 13s);
    }
};

struct npc_rift_vault_warder : public EarthenGuardAI
{
    explicit npc_rift_vault_warder(Creature* creature) : EarthenGuardAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventWarderTrample, 8s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventWarderTrample)
            return;

        DoCast(me, SpellWarderTrample);
        _events.ScheduleEvent(EventWarderTrample, 12s);
    }
};

struct npc_rift_earthen_hallshaper : public EarthenGuardAI
{
    explicit npc_rift_earthen_hallshaper(Creature* creature) : EarthenGuardAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventHallshaperHeal, 9s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        if (eventId != EventHallshaperHeal)
            return;

        if (TempSummon* summon = me->ToTempSummon())
        {
            if (Unit* owner = summon->GetSummonerUnit())
            {
                if (owner->IsAlive() && owner->IsInCombat() && owner->GetDistance(me) <= 60.0f)
                {
                    uint32 healAmount = owner->CountPctFromMaxHealth(_tier == 1 ? 2 : (_tier == 2 ? 3 : 4));
                    if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SpellHallshaperHealVisual))
                    {
                        HealInfo healInfo(me, owner, healAmount, spellInfo, spellInfo->GetSchoolMask());
                        me->HealBySpell(healInfo);
                    }
                    else
                        Unit::DealHeal(me, owner, healAmount);
                }
            }
        }

        _events.ScheduleEvent(EventHallshaperHeal, 12s);
    }
};

struct npc_rift_earthen_custodian : public EarthenGuardAI
{
    explicit npc_rift_earthen_custodian(Creature* creature) : EarthenGuardAI(creature) { }
};

struct boss_rift_archaedas : public BossAIBase
{
    explicit boss_rift_archaedas(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _guardianWaveTriggered = false;
        _warderWaveTriggered = false;
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        events.ScheduleEvent(EventGroundTremor, 11000ms);
        events.ScheduleEvent(EventSupportWave, 24s);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellSchoolMask /*damageSchoolMask*/) override
    {
        // 原版阶段守卫改为裂隙专用召唤：70%地灵守护者、40%宝库守卫，不激活预置单位。
        if (!_guardianWaveTriggered && me->HealthBelowPctDamaged(70, damage))
        {
            _guardianWaveTriggered = true;
            SummonGuards(RiftEntryEarthenGuardian, _tier, 0.65f, 0.7f);
        }

        if (!_warderWaveTriggered && me->HealthBelowPctDamaged(40, damage))
        {
            _warderWaveTriggered = true;
            SummonGuards(RiftEntryVaultWarder, _tier == 1 ? 1 : _tier - 1, 0.75f, 0.8f);
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        DespawnRiftSummons();
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventGroundTremor:
                CastIfConfigured(me, SpellGroundTremor);
                events.ScheduleEvent(EventGroundTremor, 14000ms);
                break;
            case EventSupportWave:
                SummonGuards(RiftEntryEarthenHallshaper, 1, 0.55f, 0.7f);
                SummonGuards(RiftEntryEarthenCustodian, _tier == 3 ? 2 : 1, 0.7f, 0.8f);
                events.ScheduleEvent(EventSupportWave, 30s);
                break;
            default:
                break;
        }
    }

private:
    // 各类守卫T1/T2/T3存活上限：守护者1/2/3、宝库守卫1/1/2、塑石者1/2/3、看守者2/3/4。
    uint32 GetEntryCap(uint32 entry) const
    {
        switch (entry)
        {
            case RiftEntryEarthenGuardian:
                return _tier == 1 ? 1 : (_tier == 2 ? 2 : 3);
            case RiftEntryVaultWarder:
                return _tier == 1 ? 1 : (_tier == 2 ? 1 : 2);
            case RiftEntryEarthenHallshaper:
                return _tier == 1 ? 1 : (_tier == 2 ? 2 : 3);
            case RiftEntryEarthenCustodian:
                return _tier == 1 ? 2 : (_tier == 2 ? 3 : 4);
            default:
                return 0;
        }
    }

    uint32 CountAlive(uint32 entry) const
    {
        uint32 count = 0;
        for (ObjectGuid const& guid : _riftSummons)
            if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                if (summon->IsAlive() && summon->GetEntry() == entry)
                    ++count;
        return count;
    }

    Position SelectGuardSpawn(uint32 entry) const
    {
        std::span<Position const> spawns = GetGuardSpawns(entry);
        if (spawns.empty())
            return me->GetRandomNearPosition(10.0f);

        // 优先选未被存活的同类守卫占用的原版刷点，保证石像分散在原版位置苏醒。
        for (Position const& spawn : spawns)
        {
            bool occupied = false;
            for (ObjectGuid const& guid : _riftSummons)
            {
                if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
                    if (summon->IsAlive() && summon->GetEntry() == entry && summon->GetExactDist(&spawn) < 2.0f)
                    {
                        occupied = true;
                        break;
                    }
            }
            if (!occupied)
                return spawn;
        }

        // 刷点全被占用时，随机取一个原版刷点。
        return spawns[urand(0, uint32(spawns.size()) - 1)];
    }

    void SummonGuards(uint32 entry, uint32 amount, float healthCoefficient, float damageCoefficient)
    {
        uint32 cap = GetEntryCap(entry);
        uint32 alive = CountAlive(entry);
        for (uint32 i = alive; i < cap && amount > 0; ++i, --amount)
        {
            // 复刻原版：石像在原版刷点位置刷出，并走石化唤醒动画（preserveStonedState=true），
            // 苏醒后由守卫AI自行进入战斗，此处不再立即AttackStart。
            Position position = SelectGuardSpawn(entry);
            SummonTieredCreature(entry, position, healthCoefficient, damageCoefficient,
                TEMPSUMMON_CORPSE_TIMED_DESPAWN, 10 * IN_MILLISECONDS, true);
        }
    }

    bool _guardianWaveTriggered = false;
    bool _warderWaveTriggered = false;
};

void AddSC_boss_rift_archaedas()
{
    RegisterCreatureAI(boss_rift_archaedas);
    RegisterCreatureAI(npc_rift_earthen_guardian);
    RegisterCreatureAI(npc_rift_vault_warder);
    RegisterCreatureAI(npc_rift_earthen_hallshaper);
    RegisterCreatureAI(npc_rift_earthen_custodian);
}

} // namespace HeroicDungeonRift
