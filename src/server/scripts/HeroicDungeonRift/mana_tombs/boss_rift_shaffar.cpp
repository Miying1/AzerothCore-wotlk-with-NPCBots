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
// 奥金顿：法力陵墓 - 节点亲王沙法尔（源 Entry 18344；裂隙 Entry 100193-100195）
constexpr uint32 RiftEntryEtherealBeacon = 102051;    // 源 Entry 18431
constexpr uint32 RiftEntryEtherealApprentice = 102052; // 源 Entry 18430

enum BossEvents : uint32
{
    EventElementalBolt = 1, // 火球/寒冰箭（T1基础）
    EventFrostNova,         // 冰霜新星后闪现（T1基础）
    EventBlink,
    EventSummonBeacon,      // 召唤虚灵信标（T1基础）
    EventArcaneBarrage,     // 奥术弹幕（T2新增）
    EventArcaneBlast        // 奥术冲击（T3新增）
};

enum BeaconEvents : uint32
{
    EventBeaconBolt = 1,
    EventBeaconTransform
};

constexpr int32 ActionTransformBeacon = 1;

enum ApprenticeEvents : uint32
{
    EventApprenticeFireball = 1,
    EventApprenticeFrostbolt
};

enum Spells : uint32
{
    SpellBlink = 34605,
    SpellFrostbolt = 32364,
    SpellFireball = 32363,
    SpellFrostNova = 32365,
    SpellBeaconVisual = 32368,
    SpellBeaconArcaneBolt = 15254,
    SpellApprenticeFireball = 32369,
    SpellApprenticeFrostbolt = 32370,
    SpellArcaneBarrage = 58456, // 3.3.5：紫罗兰监狱奥术弹幕
    SpellArcaneBlast = 58462    // 3.3.5：紫罗兰监狱奥术冲击
};

constexpr int32 ArcaneBarrageRaidDamage = 4500;
constexpr int32 ArcaneBlastRaidDamage = 5000;

constexpr char const* AggroTexts[] =
{
    "看来我们还没有正式介绍过。",
    "一场史诗般的战斗，多么令人兴奋！",
    "我一直渴望一次精彩的冒险！"
};
constexpr char const* SummonText = "我有些非常迷人的东西要给你们看。";
constexpr char const* SlayTexts[] = { "这很……有趣。", "现在，我们该分道扬镳了。" };
constexpr char const* DeathText = "我必须向你们……告别。";
}

struct npc_rift_ethereal_apprentice : public RiftLevel70SummonAI
{
    explicit npc_rift_ethereal_apprentice(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventApprenticeFireball, 1s);
        _events.ScheduleEvent(EventApprenticeFrostbolt, 3500ms);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventApprenticeFireball:
                DoCastVictim(SpellApprenticeFireball);
                _events.ScheduleEvent(EventApprenticeFireball, 5s);
                break;
            case EventApprenticeFrostbolt:
                DoCastVictim(SpellApprenticeFrostbolt);
                _events.ScheduleEvent(EventApprenticeFrostbolt, 5s);
                break;
            default:
                break;
        }
    }
};

struct npc_rift_ethereal_beacon : public RiftLevel70SummonAI
{
    explicit npc_rift_ethereal_beacon(Creature* creature) : RiftLevel70SummonAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        RiftLevel70SummonAI::IsSummonedBy(summoner);
        DoCast(me, SpellBeaconVisual, true);
    }

    void ScheduleAbilities() override
    {
        _events.ScheduleEvent(EventBeaconBolt, 500ms);
        _events.ScheduleEvent(EventBeaconTransform, 20s);
    }

    void ExecuteAbility(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventBeaconBolt:
                DoCastVictim(SpellBeaconArcaneBolt);
                _events.ScheduleEvent(EventBeaconBolt, 2500ms);
                break;
            case EventBeaconTransform:
                if (TempSummon* summon = me->ToTempSummon())
                    if (Creature* boss = summon->GetSummonerCreatureBase())
                    {
                        boss->AI()->SetGUID(me->GetGUID());
                        boss->AI()->DoAction(ActionTransformBeacon);
                    }
                break;
            default:
                break;
        }
    }
};

struct boss_rift_shaffar : public BossAIBase
{
    explicit boss_rift_shaffar(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(AggroTexts[urand(0, 2)], LANG_UNIVERSAL);
        events.ScheduleEvent(EventElementalBolt, 4s);
        events.ScheduleEvent(EventFrostNova, 15s);
        events.ScheduleEvent(EventSummonBeacon, 10s);
        if (_tier >= 2)
            events.ScheduleEvent(EventArcaneBarrage, 8s);
        if (_tier >= 3)
            events.ScheduleEvent(EventArcaneBlast, 17s);
    }

    void SetGUID(ObjectGuid const& guid, int32 /*id*/) override
    {
        _transformingBeaconGuid = guid;
    }

    void DoAction(int32 action) override
    {
        if (action != ActionTransformBeacon)
            return;

        if (Creature* beacon = ObjectAccessor::GetCreature(*me, _transformingBeaconGuid))
        {
            SummonTieredCreature(RiftEntryEtherealApprentice, beacon->GetPosition(), 0.55f, 0.65f);
            beacon->DespawnOrUnsummon();
        }
        _transformingBeaconGuid.Clear();
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
            me->Yell(SlayTexts[urand(0, 1)], LANG_UNIVERSAL, victim);
    }

    void JustDied(Unit* killer) override
    {
        me->Yell(DeathText, LANG_UNIVERSAL);
        BossAIBase::JustDied(killer);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(2.5f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventElementalBolt:
                CastIfConfigured(me->GetVictim(), urand(0, 1) ? SpellFrostbolt : SpellFireball);
                events.ScheduleEvent(EventElementalBolt, Milliseconds(urand(3000, 4000)));
                break;
            case EventFrostNova:
                CastIfConfigured(me, SpellFrostNova);
                events.ScheduleEvent(EventBlink, 1500ms);
                events.ScheduleEvent(EventFrostNova, Milliseconds(urand(16000, 23000)));
                break;
            case EventBlink:
                CastIfConfigured(me, SpellBlink);
                break;
            case EventSummonBeacon:
                if (!urand(0, 3))
                    me->Yell(SummonText, LANG_UNIVERSAL);
                SummonTieredCreature(RiftEntryEtherealBeacon, me->GetRandomNearPosition(5.0f), 0.35f, 0.45f,
                    TEMPSUMMON_TIMED_DESPAWN, 25 * IN_MILLISECONDS);
                events.ScheduleEvent(EventSummonBeacon, 10s);
                break;
            case EventArcaneBarrage: // T2新增：3.3.5奥术弹幕，固定15秒避免与奥术冲击追平
                CastFinalRaidDamageSpell(SelectRandomPlayer(), SpellArcaneBarrage, SPELLVALUE_BASE_POINT0,
                    ArcaneBarrageRaidDamage);
                events.ScheduleEvent(EventArcaneBarrage, 15s);
                break;
            case EventArcaneBlast: // T3新增：3.3.5奥术冲击，与奥术弹幕保持错峰
                CastFinalRaidDamageSpell(me->GetVictim(), SpellArcaneBlast, SPELLVALUE_BASE_POINT0,
                    ArcaneBlastRaidDamage);
                events.ScheduleEvent(EventArcaneBlast, 19s);
                break;
            default:
                break;
        }
    }

private:
    ObjectGuid _transformingBeaconGuid;
};

void AddSC_boss_rift_shaffar()
{
    RegisterCreatureAI(boss_rift_shaffar);
    RegisterCreatureAI(npc_rift_ethereal_beacon);
    RegisterCreatureAI(npc_rift_ethereal_apprentice);
}

} // namespace HeroicDungeonRift
