/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 */

#include "../rift_boss_base.h"

#include "Creature.h"
#include "GameObject.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"

namespace HeroicDungeonRift
{
namespace
{
// 通灵学院 - 黑暗院长加丁（Darkmaster Gandling）
// 原版机制：周期性用暗影传送门随机传送一名玩家到6个房间之一，关该房门并在房间召唤3只复生的守卫。
enum Events : uint32
{
    EventArcaneMissiles = 1, // 奥术飞弹（T1基础）
    EventCurseOfDarkmaster,  // 黑暗主宰诅咒（T1基础）
    EventShadowShield,       // 暗影之盾（T1基础）
    EventShadowPortal,       // 暗影传送门（T1基础，传送玩家到房间）
    EventSummonGuardians,    // 召唤复生的守卫（T2新增）
    EventTier3Skill          // 暗影箭（T3新增）
};

enum Spells : uint32
{
    SpellArcaneMissiles = 15790,  // 奥术飞弹
    SpellCurseOfDarkmaster = 18702, // 黑暗主宰诅咒
    SpellShadowShield = 12040,    // 暗影之盾
    SpellShadowPortal = 17950,    // 暗影传送门
    SpellShadowBolt = 20791       // 暗影箭
};

// 6房间传送法术（顺序对应 GandlingGateIds 与 GandlingSummonPos）
constexpr uint32 GandlingPortalSpells[6] =
{
    17944, // 下北·暗影金库
    17946, // 下东·巴罗夫家族墓室
    17948, // 下南·拉文尼亚之墓
    17863, // 上北·秘密大厅
    17939, // 上东·受诅咒者大厅
    17943  // 上南·女巫会
};

// 6房间对应房门
constexpr uint32 GandlingGateIds[6] =
{
    177371, // 下北
    177373, // 下东
    177372, // 下南
    177376, // 上北
    177377, // 上东
    177375  // 上南
};

// 6房间守卫召唤位置（每房间3只，与GandlingPortalSpells/GandlingGateIds顺序一致）
Position const GandlingSummonPos[6][3] =
{
    // 下北·暗影金库
    { { 245.3716f, 0.628038f, 72.73877f, 0.01745329f }, { 240.9920f, 3.405653f, 72.73877f, 6.143559f }, { 240.9543f, -3.182943f, 72.73877f, 0.2268928f } },
    // 下东·巴罗夫家族墓室
    { { 181.8245f, -42.58117f, 75.4812f, 4.660029f }, { 177.7456f, -42.74745f, 75.4812f, 4.886922f }, { 185.6157f, -42.91200f, 75.4812f, 4.45059f } },
    // 下南·拉文尼亚之墓
    { { 136.362f, 6.221f, 75.40f, 3.14f }, { 130.79f, -0.91f, 75.40f, 3.14f }, { 136.362f, -8.221f, 75.40f, 3.14f } },
    // 上北·秘密大厅
    { { 230.80f, 0.138f, 85.23f, 0.0f }, { 241.23f, -6.979f, 85.23f, 0.0f }, { 246.65f, 4.227f, 84.85f, 0.0f } },
    // 上东·受诅咒者大厅
    { { 177.9624f, -68.23893f, 84.95197f, 3.228859f }, { 183.7705f, -61.43489f, 84.92424f, 5.148721f }, { 184.7035f, -77.74805f, 84.92424f, 4.660029f } },
    // 上南·女巫会
    { { 111.7203f, -1.105035f, 85.45985f, 3.961897f }, { 118.0079f, 6.430664f, 85.31169f, 2.408554f }, { 120.0276f, -7.496636f, 85.31169f, 2.984513f } }
};

constexpr uint32 RisenGuardianSummonCount = 3;
constexpr char const* GandlingAggroText = "现在开始上课！";
constexpr uint32 GandlingAggroSound = 7145;
}

struct boss_rift_gandling : public BossAIBase
{
    explicit boss_rift_gandling(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _currentRoom = 6; // 无有效房间
        OpenAllGates();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(GandlingAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(GandlingAggroSound);

        ScheduleTieredEvent(EventArcaneMissiles, 6000, 4800, 3800);
        ScheduleTieredEvent(EventCurseOfDarkmaster, 12000, 9500, 7500);
        ScheduleTieredEvent(EventShadowShield, 15000, 12000, 9500);
        events.ScheduleEvent(EventShadowPortal, 20s); // 原版周期传送
        if (_tier >= 2)
            events.ScheduleEvent(EventSummonGuardians, 18s);
        if (_tier >= 3)
            events.ScheduleEvent(EventTier3Skill, 9s);
    }

    void JustDied(Unit* /*killer*/) override
    {
        OpenAllGates();
        BossAIBase::JustDied(nullptr);
    }

    void SpellHitTarget(Unit* target, SpellInfo const* spell) override
    {
        if (!target || !spell || spell->Id != SpellShadowPortal)
            return;

        uint32 room = _currentRoom;
        if (room >= 6)
            return;

        // 关该房间门 + 在该房间召唤3只复生的守卫 + 传送目标
        SetGate(room, false);
        SpawnMobsInRoom(room);
        me->CastSpell(target, GandlingPortalSpells[room], true);

        // 被传者若为主目标则降威胁、转火次目标
        if (Unit* victim = me->GetVictim())
            if (target->GetGUID() == victim->GetGUID())
                if (Unit* newTarget = SelectTarget(SelectTargetMethod::MaxThreat, 0, 200.0f, false))
                    me->AddThreat(newTarget, 1000000.0f);
    }

    void ConfigureTier() override { SetRaidSpellDamageMultiplier(15.0f); }

    void ExecuteRiftEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EventArcaneMissiles:
                CastIfConfigured(me->GetVictim(), SpellArcaneMissiles);
                ScheduleTieredEvent(EventArcaneMissiles, 7000, 5500, 4500);
                break;
            case EventCurseOfDarkmaster:
                CastIfConfigured(SelectRandomPlayer(), SpellCurseOfDarkmaster);
                ScheduleTieredEvent(EventCurseOfDarkmaster, 20000, 16000, 13000);
                break;
            case EventShadowShield:
                CastIfConfigured(me, SpellShadowShield);
                ScheduleTieredEvent(EventShadowShield, 25000, 20000, 16000);
                break;
            case EventShadowPortal:
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 15.0f, true))
                {
                    _currentRoom = urand(0, 5);
                    me->CastSpell(target, SpellShadowPortal, false);
                }
                events.ScheduleEvent(EventShadowPortal, 25s);
                break;
            case EventSummonGuardians: // T2新增
                for (uint32 i = 0; i < RisenGuardianSummonCount; ++i)
                    SummonTieredCreature(RiftEntryRisenGuardian, me->GetRandomNearPosition(8.0f), 0.6f, 0.7f);
                events.ScheduleEvent(EventSummonGuardians, _tier == 3 ? 24s : 30s);
                break;
            case EventTier3Skill: // T3新增：暗影箭，顺发
                CastIfConfigured(me->GetVictim(), SpellShadowBolt, true);
                events.ScheduleEvent(EventTier3Skill, 5s);
                break;
            default:
                break;
        }
    }

private:
    void SetGate(uint8 room, bool open)
    {
        if (room >= 6)
            return;
        if (GameObject* gate = me->FindNearestGameObject(GandlingGateIds[room], 200.0f))
            gate->SetGoState(open ? GO_STATE_ACTIVE : GO_STATE_READY);
    }

    void OpenAllGates()
    {
        for (uint8 i = 0; i < 6; ++i)
            SetGate(i, true);
    }

    void SpawnMobsInRoom(uint32 room)
    {
        if (room >= 6)
            return;
        for (uint32 i = 0; i < RisenGuardianSummonCount; ++i)
            SummonTieredCreature(RiftEntryRisenGuardian, GandlingSummonPos[room][i], 0.6f, 0.7f);
    }

    uint32 _currentRoom = 6;
};

void AddSC_boss_rift_gandling()
{
    RegisterCreatureAI(boss_rift_gandling);
}

} // namespace HeroicDungeonRift
