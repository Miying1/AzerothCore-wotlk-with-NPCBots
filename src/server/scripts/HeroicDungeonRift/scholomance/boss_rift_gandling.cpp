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
// T1原版暗影传送门：随机传送一名玩家到6个房间之一，关门并在该传送房召唤3只复生的守卫。
// T2额外守卫：直接在Boss场内召唤3只，不属于暗影传送门房间守卫。
enum Events : uint32
{
    EventArcaneMissiles = 1, // 奥术飞弹（Spell 15790，T1原版）
    EventCurseOfDarkmaster,  // 黑暗主宰诅咒（Spell 18702，T1原版）
    EventShadowShield,       // 暗影之盾（Spell 12040，T1原版）
    EventShadowPortal,       // 暗影传送门（Spell 17950，T1原版，传送房机制）
    EventSummonGuardians,    // 场内额外守卫（T2新增，无独立施法ID）
    EventTier3Skill          // 暗影箭（Spell 20791，T3新增）
};

enum Spells : uint32
{
    SpellArcaneMissiles = 15790,  // 奥术飞弹（T1原版）
    SpellCurseOfDarkmaster = 18702, // 黑暗主宰诅咒（T1原版）
    SpellShadowShield = 12040,    // 暗影之盾（T1原版）
    SpellShadowPortal = 17950,    // 暗影传送门（T1原版；命中后关门、召唤房间守卫并传送）
    SpellShadowBolt = 20791       // 暗影箭（T3新增，覆写BP0直接伤害）
};

constexpr int32 ShadowBoltTier1DirectDamage = 4500;

// T1原版6房间定向传送法术（顺序对应 GandlingGateIds 与 GandlingSummonPos）
constexpr uint32 GandlingPortalSpells[6] =
{
    17944, // 下北·暗影金库（T1原版）
    17946, // 下东·巴罗夫家族墓室（T1原版）
    17948, // 下南·拉文尼亚之墓（T1原版）
    17863, // 上北·秘密大厅（T1原版）
    17939, // 上东·受诅咒者大厅（T1原版）
    17943  // 上南·女巫会（T1原版）
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

// 传送房开门检查间隔：被传送玩家清完房内守卫后需尽快开门，
// 但不必每帧轮询（每次检查都要遍历6个房间的守卫GUID）。
constexpr uint32 TeleportRoomCheckInterval = 500;
}

struct boss_rift_gandling : public BossAIBase
{
    explicit boss_rift_gandling(Creature* creature) : BossAIBase(creature) { }

    void Reset() override
    {
        BossAIBase::Reset();
        _currentRoom = 6; // 无有效房间
        ClearAllTeleportRoomTracking();
        OpenAllGates();
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell(GandlingAggroText, LANG_UNIVERSAL);
        me->PlayDirectSound(GandlingAggroSound);

        // 开战前确保六扇房门全开：房门在world库中的默认状态是关闭的，
        // 若Reset时因房间网格尚未加载而开门失败，之后被传送进房的玩家就没有出口。
        OpenAllGates();

        events.ScheduleEvent(EventArcaneMissiles, Milliseconds(6000));
        events.ScheduleEvent(EventCurseOfDarkmaster, Milliseconds(12000));
        events.ScheduleEvent(EventShadowShield, Milliseconds(15000));
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

    void UpdateAI(uint32 diff) override
    {
        // 开门检查必须独立于战斗状态：被传送玩家死亡或全员脱战后Boss不再有仇恨目标，
        // 基类的UpdateVictim会提前返回，若把开门交给EventMap，房门将永远不会重新打开。
        if (_teleportRoomCheckTimer <= diff)
        {
            _teleportRoomCheckTimer = TeleportRoomCheckInterval;
            OpenClearedTeleportRooms();
        }
        else
            _teleportRoomCheckTimer -= diff;

        BossAIBase::UpdateAI(diff);
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
        _roomTeleportedPlayerGuids[room] = target->GetGUID();
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
                events.ScheduleEvent(EventArcaneMissiles, Milliseconds(7000));
                break;
            case EventCurseOfDarkmaster:
                CastIfConfigured(SelectRandomPlayer(), SpellCurseOfDarkmaster);
                events.ScheduleEvent(EventCurseOfDarkmaster, Milliseconds(20000));
                break;
            case EventShadowShield:
                CastIfConfigured(me, SpellShadowShield);
                events.ScheduleEvent(EventShadowShield, Milliseconds(25000));
                break;
            case EventShadowPortal:
            {
                // 暗影传送门只传送真实玩家：本分支的SelectTarget(playerOnly=true)把NPCBot也算作玩家
                // （DefaultTargetSelector放宽到IsNPCBot），而SelectPlayerTarget在范围内没有真实玩家时
                // 仍会回退到BOT，因此这里再显式排除一次；没有真实玩家时本次不传送。
                Unit* target = SelectPlayerTarget(SelectTargetMethod::Random, 0, 15.0f);
                if (target && !target->IsNPCBot())
                {
                    _currentRoom = urand(0, 5);
                    me->CastSpell(target, SpellShadowPortal, false);
                }
                events.ScheduleEvent(EventShadowPortal, 25s);
                break;
            }
            case EventSummonGuardians: // T2新增：Boss场内额外召唤3只守卫，区别于T1传送房守卫
                for (uint32 i = 0; i < RisenGuardianSummonCount; ++i)
                    SummonTieredCreature(RiftEntryRisenGuardian, me->GetRandomNearPosition(8.0f), 0.6f, 0.7f);
                events.ScheduleEvent(EventSummonGuardians, _tier == 3 ? 24s : 30s);
                break;
            case EventTier3Skill: // T3新增：暗影箭，瞬发；覆写BP0直接伤害
                CastFinalRaidDamageSpell(me->GetVictim(), SpellShadowBolt, SPELLVALUE_BASE_POINT0,
                    ShadowBoltTier1DirectDamage, true);
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
        {
            // 记录本房守卫：守卫全部死亡或消失后据此重新开门，否则被传送玩家会被关在房内。
            if (Creature* guardian = SummonTieredCreature(RiftEntryRisenGuardian, GandlingSummonPos[room][i], 0.6f, 0.7f))
                _roomGuardianGuids[room][i] = guardian->GetGUID();
        }
    }

    // 房间守卫全部死亡或消失后开门放人，对应原版SummonedCreatureDespawn里的开门处理。
    // 被传送进房的玩家死亡或已离开副本时同样开门，对应原版由守卫KilledUnit触发的开门，
    // 否则玩家以灵魂状态跑尸时会被关在房内、队友也无法进房清掉剩下的守卫。
    void OpenClearedTeleportRooms()
    {
        for (uint8 room = 0; room < 6; ++room)
        {
            if (!IsTeleportRoomInUse(room))
                continue;

            if (HasAliveRoomGuardian(room) && IsTeleportedPlayerTrapped(room))
                continue;

            SetGate(room, true);
            ClearTeleportRoomTracking(room);
        }
    }

    // 只有被传送过玩家的房间才由本AI接管房门，避免误开别人操作的房门。
    bool IsTeleportRoomInUse(uint8 room) const
    {
        if (!_roomTeleportedPlayerGuids[room].IsEmpty())
            return true;

        for (uint32 i = 0; i < RisenGuardianSummonCount; ++i)
            if (!_roomGuardianGuids[room][i].IsEmpty())
                return true;
        return false;
    }

    bool HasAliveRoomGuardian(uint8 room) const
    {
        for (uint32 i = 0; i < RisenGuardianSummonCount; ++i)
        {
            ObjectGuid const& guardianGuid = _roomGuardianGuids[room][i];
            if (guardianGuid.IsEmpty())
                continue;

            if (Creature* guardian = ObjectAccessor::GetCreature(*me, guardianGuid))
                if (guardian->IsAlive())
                    return true;
        }
        return false;
    }

    // 玩家死亡或已离开本次副本时不再有被困风险，房内守卫交给后续进房的人清掉。
    bool IsTeleportedPlayerTrapped(uint8 room) const
    {
        Player* player = ObjectAccessor::FindPlayer(_roomTeleportedPlayerGuids[room]);
        if (!player || !player->IsAlive())
            return false;
        return player->GetMapId() == me->GetMapId() && player->GetInstanceId() == me->GetInstanceId();
    }

    void ClearAllTeleportRoomTracking()
    {
        for (uint8 room = 0; room < 6; ++room)
            ClearTeleportRoomTracking(room);
    }

    void ClearTeleportRoomTracking(uint8 room)
    {
        for (uint32 i = 0; i < RisenGuardianSummonCount; ++i)
            _roomGuardianGuids[room][i].Clear();
        _roomTeleportedPlayerGuids[room].Clear();
    }

    uint32 _currentRoom = 6;
    // 每个传送房当前登记的守卫GUID，用于判断该房间是否已清空。
    ObjectGuid _roomGuardianGuids[6][RisenGuardianSummonCount];
    // 每个传送房最近一次被传送进去的玩家，用于在玩家死亡后开门。
    ObjectGuid _roomTeleportedPlayerGuids[6];
    uint32 _teleportRoomCheckTimer = TeleportRoomCheckInterval;
};

void AddSC_boss_rift_gandling()
{
    RegisterCreatureAI(boss_rift_gandling);
}

} // namespace HeroicDungeonRift
