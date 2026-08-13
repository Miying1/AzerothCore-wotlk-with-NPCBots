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
// 影牙城堡 - 大法师阿鲁高（Archmage Arugal）
enum Events : uint32
{
    EventVoidBolt = 1,       // 虚空箭（T1基础）
    EventThundershock,       // 雷霆震荡（T1基础）
    EventArugalsCurse,       // 阿鲁高的诅咒（T1基础，变羊+魅惑）
    EventShadowPort,         // 暗影传送（T1基础，随机传送到阳台）
    EventSummonVoidwalkers,  // 召唤虚空行者（T2新增）
    EventTier3Skill          // 暗影箭雨（T3新增）
};

enum Spells : uint32
{
    SpellVoidBolt = 7588,         // 虚空箭
    SpellThundershock = 7803,     // 雷霆震荡
    SpellArugalsCurse = 7621,     // 阿鲁高的诅咒
    SpellShadowPort1 = 7136,      // 暗影传送（阳台点1）
    SpellShadowPort2 = 7586,      // 暗影传送（阳台点2）
    SpellShadowPort3 = 7587,      // 暗影传送（阳台点3）
    SpellShadowBoltVolley = 20741 // 暗影箭雨
};

constexpr uint32 ShadowPortSpells[] = { SpellShadowPort1, SpellShadowPort2, SpellShadowPort3 };

constexpr uint32 VoidwalkerSummonCount = 4;
constexpr char const* ArugalAggroText = "你也要服侍我！";
constexpr char const* ArugalCurseText = "释放你的怒火！";
constexpr char const* ArugalKillText = "又倒下了一个！";
}

struct boss_rift_arugal : public BossAIBase
{
    explicit boss_rift_arugal(Creature* creature) : BossAIBase(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(ArugalAggroText, LANG_UNIVERSAL);

        ScheduleTieredEvent(EventVoidBolt, 3000, 2400, 1800);
        ScheduleTieredEvent(EventThundershock, 9000, 7000, 5500);
        ScheduleTieredEvent(EventArugalsCurse, 13000, 10000, 8000);
        events.ScheduleEvent(EventShadowPort, 18s); // 暗影传送：随机传送到阳台
        if (_tier >= 2)
            events.ScheduleEvent(EventSummonVoidwalkers, 15s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 11s);
    }

    void KilledUnit(Unit* victim) override
    {
        if (victim && victim->IsPlayer())
            me->Yell(ArugalKillText, LANG_UNIVERSAL, victim);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventVoidBolt:
                CastIfConfigured(me->GetVictim(), SpellVoidBolt);
                ScheduleTieredEvent(EventVoidBolt, 3200, 2600, 2000);
                break;
            case EventThundershock:
                CastIfConfigured(me, SpellThundershock);
                ScheduleTieredEvent(EventThundershock, 26000, 21000, 17000);
                break;
            case EventArugalsCurse:
                if (Unit* target = SelectRandomPlayer())
                {
                    CastIfConfigured(target, SpellArugalsCurse);
                    me->Yell(ArugalCurseText, LANG_UNIVERSAL, target);
                }
                ScheduleTieredEvent(EventArugalsCurse, 26000, 21000, 17000);
                break;
            case EventShadowPort: // 暗影传送：随机传送到阳台3点之一
                me->CastSpell(me, ShadowPortSpells[urand(0, 2)], true);
                events.ScheduleEvent(EventShadowPort, 20s);
                break;
            case EventSummonVoidwalkers:
                SummonVoidwalkers();
                events.ScheduleEvent(EventSummonVoidwalkers, _tier == 3 ? 30s : 45s);
                break;
            case EventTier3Skill: // T3新增：暗影箭雨，瞬发
                CastIfConfigured(me, SpellShadowBoltVolley, true);
                events.ScheduleEvent(EventTier3Skill, 18s);
                break;
            default:
                break;
        }
    }

private:
    void SummonVoidwalkers()
    {
        for (uint32 i = 0; i < VoidwalkerSummonCount; ++i)
            SummonTieredCreature(RiftEntryArugalVoidwalker, me->GetRandomNearPosition(6.0f), 0.6f, 0.8f);
    }
};

void AddSC_boss_rift_arugal()
{
    RegisterCreatureAI(boss_rift_arugal);
}

} // namespace HeroicDungeonRift
