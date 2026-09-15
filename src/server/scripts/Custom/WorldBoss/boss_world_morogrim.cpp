/*
 * 世界BOSS：莫洛格里·踏潮者（Morogrim Tidewalker，毒蛇神殿复刻，83级）
 *
 * 复刻毒蛇神殿·莫洛格里·踏潮者的核心战斗逻辑，强度对齐 10 人奥杜尔（Ulduar 10N）。
 * 相比原版（Outland/CoilfangReservoir/SerpentShrine/boss_morogrim_tidewalker.cpp）：
 *  - 保留核心战斗：潮汐波（坦克正面 AOE）、水之墓（对 4 个非坦克目标的 DOT）、
 *    地震 + 鱼人召唤（全团 AOE 后召唤 11 只潮行者潜伏者）、25% 血量以下改为召唤水晶体；
 *  - 潮行者潜伏者（120514）与水晶体（120515）为自定义召唤物（继承 WorldBossSummonAI），
 *    其中水晶体的冻结伤害经 WorldBossSummonAI::DamageDealt 统一缩放；
 *  - 水之墓复用原版法术脚本（spell_morogrim_tidewalker_watery_grave）。
 * 技能伤害统一由 WorldBossGuardAI 基类（world_boss_guard.cpp）缩放。
 * 该BOSS用于世界地图随机刷新（临时召唤），无固定房间坐标。
 */

#include "ScriptedCreature.h"
#include "TaskScheduler.h"

#include "world_boss_guard.h"

enum MorogrimSpells
{
    SPELL_TIDAL_WAVE   = 37730, // 潮汐波（坦克正面 AOE）
    SPELL_WATERY_GRAVE = 38028, // 水之墓（DUMMY，脚本对 4 个目标施放 38023/38024/38025/37850）
    SPELL_EARTHQUAKE   = 37764, // 地震（全团 AOE）
    SPELL_FREEZE       = 37871, // 冻结（水晶体，冰霜伤害 + 定身）
};

enum MorogrimSays
{
    SAY_AGGRO              = 0, // 深渊的洪流将吞噬你们！
    SAY_SUMMON             = 1, // 以潮汐之名！/ 消灭他们，我的奴仆们！
    SAY_SUMMON_BUBBLE      = 2, // 你们无处可躲！/ 很快就要结束了！
    SAY_SLAY               = 3, // 结束了！/ 挣扎只会让你更痛苦。/ 只有强者才能生存。
    SAY_DEATH              = 4, // 巨大的洋流……艾森……
    EMOTE_WATERY_GRAVE     = 5, // 水之墓（表情）
    EMOTE_EARTHQUAKE       = 6, // 地震（表情）
    EMOTE_WATERY_GLOBULES  = 7, // 水晶体（表情）
};

enum MorogrimEvents
{
    EVENT_TIDAL_WAVE     = 1, // 潮汐波（每 20 秒）
    EVENT_WATERY_GRAVE   = 2, // 水之墓 / 水晶体（每 25 秒）
    EVENT_EARTHQUAKE     = 3, // 地震（每 45-60 秒）
    EVENT_SUMMON_MURLOCS = 4, // 召唤鱼人（地震后 8 秒）
};

struct boss_world_morogrim : public WorldBossGuardAI
{
    boss_world_morogrim(Creature* creature) : WorldBossGuardAI(creature) { }

    void JustEngagedWith(Unit* who) override
    {
        WorldBossGuardAI::JustEngagedWith(who);
        Talk(SAY_AGGRO);

        events.ScheduleEvent(EVENT_TIDAL_WAVE, 10s);
        events.ScheduleEvent(EVENT_WATERY_GRAVE, 20s);
        events.ScheduleEvent(EVENT_EARTHQUAKE, 40s);
    }

    void JustSummoned(Creature* summon) override
    {
        summons.Summon(summon);
    }

    void SummonedCreatureDespawn(Creature* summon) override
    {
        summons.Despawn(summon);
    }

    void KilledUnit(Unit* /*victim*/) override
    {
        Talk(SAY_SLAY);
    }

    void JustDied(Unit* killer) override
    {
        WorldBossGuardAI::JustDied(killer);
        Talk(SAY_DEATH);
    }

    // 召唤 11 只潮行者潜伏者（环绕 BOSS 分布）
    void SummonMurlocs()
    {
        constexpr float TAU = 6.2831853f; // 2π
        for (uint8 i = 0; i < 11; ++i)
        {
            Position pos = me->GetNearPosition(6.0f, TAU * float(i) / 11.0f);
            me->SummonCreature(NPC_WORLD_BOSS_MURLOC, pos, TEMPSUMMON_DEAD_DESPAWN, 0);
        }
    }

    // 召唤 4 个水晶体（环绕 BOSS 分布）
    void SummonWaterGlobules()
    {
        constexpr float TAU = 6.2831853f; // 2π
        for (uint8 i = 0; i < 4; ++i)
        {
            Position pos = me->GetNearPosition(8.0f, TAU * float(i) / 4.0f);
            me->SummonCreature(NPC_WORLD_BOSS_WATER_GLOBULE, pos, TEMPSUMMON_DEAD_DESPAWN, 0);
        }
    }

    void ExecuteEvent(uint32 eventId) override
    {
        switch (eventId)
        {
            case EVENT_TIDAL_WAVE:
                DoCastVictim(SPELL_TIDAL_WAVE);
                events.Repeat(20s);
                break;

            case EVENT_WATERY_GRAVE:
                Talk(SAY_SUMMON_BUBBLE);
                if (me->HealthAbovePct(25))
                {
                    // 血量高于 25%：对 4 个非坦克目标施放水之墓（复用原版法术脚本）
                    Talk(EMOTE_WATERY_GRAVE);
                    me->CastCustomSpell(SPELL_WATERY_GRAVE, SPELLVALUE_MAX_TARGETS, 4, me, false);
                }
                else
                {
                    // 血量低于 25%：改为召唤水晶体
                    Talk(EMOTE_WATERY_GLOBULES);
                    SummonWaterGlobules();
                }
                events.Repeat(25s);
                break;

            case EVENT_EARTHQUAKE:
                Talk(EMOTE_EARTHQUAKE);
                DoCastSelf(SPELL_EARTHQUAKE);
                events.ScheduleEvent(EVENT_SUMMON_MURLOCS, 8s);
                events.Repeat(45s, 60s);
                break;

            case EVENT_SUMMON_MURLOCS:
                Talk(SAY_SUMMON);
                SummonMurlocs();
                break;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (CheckLeash())
            return;

        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        while (uint32 eventId = events.ExecuteEvent())
            ExecuteEvent(eventId);

        DoMeleeAttackIfReady();
    }
};

// 潮行者鱼人（召唤物）：普通近战，进入战斗后攻击附近玩家
struct npc_world_boss_murloc : public WorldBossSummonAI
{
    npc_world_boss_murloc(Creature* creature) : WorldBossSummonAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        // 野外化：SetInCombatWithZone 在野外地图不生效（CreatureAI::DoZoneInCombat 对非副本直接返回），
        // 改为主动状态并显式攻击 BOSS 当前目标，其余目标由 UpdateVictim 兜底。
        me->SetReactState(REACT_AGGRESSIVE);
        if (summoner)
            if (Unit* boss = summoner->ToUnit())
                if (Unit* victim = boss->GetVictim())
                    AttackStart(victim);
    }
};

// 水晶体（召唤物）：缓慢追踪目标，接近后施放冻结（冰霜伤害 + 定身）后消失
struct npc_world_boss_water_globule : public WorldBossSummonAI
{
    npc_world_boss_water_globule(Creature* creature) : WorldBossSummonAI(creature) { }

    void IsSummonedBy(WorldObject* summoner) override
    {
        // 水晶体追踪 BOSS 的当前目标
        if (summoner)
        {
            if (Unit* boss = summoner->ToUnit())
            {
                if (Unit* victim = boss->GetVictim())
                {
                    me->AddThreat(victim, 1000000.0f);
                    AttackStart(victim);
                }
            }
        }
    }

    void UpdateAI(uint32 /*diff*/) override
    {
        if (!UpdateVictim())
            return;

        // 接近目标后施放冻结（冰霜伤害 + 定身），随后消失
        if (me->IsWithinDistInMap(me->GetVictim(), 5.0f))
        {
            // 冻结为瞬发直伤，预置施法记录以保证首跳伤害正确缩放
            SetLastCastSpellId(SPELL_FREEZE);
            DoCastVictim(SPELL_FREEZE, true);
            me->DespawnOrUnsummon(500ms);
            return;
        }

        DoMeleeAttackIfReady();
    }
};

void AddSC_boss_world_morogrim()
{
    RegisterCreatureAI(boss_world_morogrim);
    RegisterCreatureAI(npc_world_boss_murloc);
    RegisterCreatureAI(npc_world_boss_water_globule);
}
