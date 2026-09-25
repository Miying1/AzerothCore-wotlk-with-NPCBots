/*
 * 世界BOSS统一战斗机制（归属锁定 + 脱战距离 + 技能伤害缩放）
 *
 * 机制说明：
 *  1. 归属锁定：进入战斗时记录当前位置并锁定开怪玩家所在队伍/团队，脱战后重置。
 *  2. 脱战距离：战斗中移动超过 WORLD_BOSS_LEASH_RANGE 码则脱战，防止风筝拉脱。
 *  3. 技能伤害缩放：按倍率表（GetWorldBossSpellScale）在 DamageDealt 里统一放大，
 *     只对继承本基类的自定义世界BOSS及其召唤物生效，不影响原版内容。
 *
 * “其他玩家无法攻击”通过全局 reaction hook（world_boss_reaction_guard）实现：
 * WoW 引擎中单位阵营对所有观察者一致，无法做到“不同玩家看到不同阵营”，
 * 因此只能通过 Unit::GetReactionTo 层面的 IfNormalReaction 将非开怪队伍视为中立。
 */

#include "world_boss_guard.h"

#include "Creature.h"
#include "Group.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellInfo.h"
#include "ThreatManager.h"
#include "Unit.h"
#include "World.h"

#include <limits>
#include <unordered_map>

namespace
{
    // 世界BOSS（83级 10 人奥杜尔强度）法术伤害倍率表。
    // 把 60/70 级副本首领复刻为 83 级世界BOSS后，将其技能伤害统一校准到 10 人奥杜尔强度：
    // 倍率 = 奥杜尔 10N 目标伤害 / 原始 DBC 伤害（减伤前，数据取自 outputs/Spell.csv）。
    // 校准锚点（奥杜尔 10N，82-83 级 BOSS 对 80 级玩家）：
    //   常规单体/AOE 约 8000-12000，重型技能 12000-18000，DOT 每跳 2000-3500。
    std::unordered_map<uint32, WorldBossSpellScale> const BuildWorldBossSpellScaling()
    {
        std::unordered_map<uint32, WorldBossSpellScale> map;

        // ===================== 奥（Al'ar，风暴要塞复刻） =====================

        map[35181] = { 2.5f, 0.0f }; // 俯冲轰炸（AOE，DBC 5700-6300）：-> 14250-15750

        // 召唤物技能（施法者为烈焰之痕 120501，通过 WorldBossSummonAI 缩放）。
        // 烈焰之痕伤害（触发链 35380->35383）：35383 为直伤（DBC 2188-2812），3 倍 -> 6564-8436；
        // 35380 为周期触发光环，一并加入以覆盖首跳伤害时施法记录仍停留在 35380 的时序。
        map[35383] = { 3.0f, 0.0f };
        map[35380] = { 3.0f, 0.0f };

        // ===================== 伊利丹（Illidan，黑暗神庙复刻） =====================
        map[40598] = { 3.0f, 0.0f }; // 火球术（单体，DBC 2550-3450）：-> 7650-10350
        map[40832] = { 5.0f, 0.0f }; // 火焰碰撞（小 AOE，DBC 925-1075）：-> 4625-5375
        map[40904] = { 2.5f, 0.0f }; // 吸取灵魂（正面 AOE + 治疗，DBC 4500-5500）：-> 11250-13750

        map[39908] = { 0.0f, 2.0f }; // 眼棱（飞行阶段引导，伤害由脚本造成，periodic 为参考倍率）
        map[41917] = { 0.0f, 1.5f }; // 寄生暗影魔（DOT，DBC 3000/跳）：保持原值（已对齐奥杜尔 DOT 区间）
        map[40932] = { 2.5f, 2.0f }; // 痛苦烈焰（着陆阶段 AOE 4000 + DOT 3000/跳）：直伤 -> 10000，DOT 保持 3000
        map[41078] = { 2.0f, 0.0f }; // 暗影冲击（恶魔阶段单体，DBC 8750-11250）：-> 13125-16875
        map[41126] = { 2.0f, 0.0f }; // 烈焰爆发（恶魔阶段，SCRIPT_EFFECT，伤害由脚本造成，参考倍率）

        // 召唤物技能（施法者为阿兹诺斯烈焰 120504，通过 WorldBossSummonAI 缩放）。
        map[40631] = { 1.25f, 0.0f }; // 烈焰冲击（单体火焰，DBC 7000-9000）：-> 8750-11250
        map[42003] = { 1.0f, 0.0f }; // 冲锋（位移效果，无直接伤害，占位）

        // ===================== 拉格纳罗斯（Ragnaros，熔火之心复刻） =====================
        map[20566] = { 7.0f, 0.0f }; // 拉格纳罗斯之怒（直伤 1000 + 召唤）：直伤 -> 5000
        map[20565] = { 2.0f, 0.0f }; // 岩浆爆裂（远程火焰单体，DBC 6000 固定）：-> 9000
        map[21154] = { 1.0f, 0.0f }; // 拉格纳罗斯之力（击退效果，无直接伤害，占位）
        // 注：熔岩喷发陷阱（21158）由游戏对象施放，伤害不经过本 AI 的 DamageDealt，故不在此缩放。

        // ===================== 勒什雷尔（Broodlord Lashlayer，黑翼之巢复刻） =====================
        map[23331] = { 3.0f, 0.0f }; // 冲击波（AOE 火焰，DBC 2625-3375）：-> 7875-10125
        // 注：顺劈斩（26350）/ 致死打击（24573）/ 击退（25778）均为物理近战，不缩放。

        // ===================== 战争守卫沙尔图拉（Battleguard Sartura，安其拉神殿复刻） =====================
        // 皇家守卫旋风斩（触发链 26038->26686）：26686 为直伤（DBC 1080-1320），4 倍 -> 4320-5280；
        // 26038 为周期触发光环，一并加入以覆盖首跳伤害时施法记录仍停留在 26038 的时序。
        map[26686] = { 4.0f, 0.0f };
        map[26038] = { 4.0f, 0.0f };
        // 注：沙尔图拉本体旋风斩（26084）为物理武器伤害，破甲顺劈（25174）为物理近战，均不缩放。

        // ===================== 库林纳克斯（Kurinnaxx，安其拉废墟复刻） =====================
        // 注：致死创伤（25646）为减治疗 debuff，横扫（25814）/ 痛击（3391）为物理近战，均不缩放；
        //     沙陷阱（25656）由游戏对象施放，伤害不经过本 AI 的 DamageDealt，故不在此缩放。

        // ===================== 苏普雷姆斯（Supremus，黑暗神庙复刻） =====================
        // 憎恨打击（打血量最高目标的重击，DBC 27750-32250 为 25 人黑暗神庙值）：
        // 校准到奥杜尔 10N 重击 15000-20000，0.6 倍 -> 16650-19350。
        map[41926] = { 1.0f, 0.0f };

        // 熔岩烈焰（触发链 40980->40253->40265，施法者为熔岩拳巡者 120509）：
        // 40265 为直伤（DBC 3325-3675），2 倍 -> 6650-7350；
        // 40253 为周期触发光环，一并加入以覆盖首跳伤害时施法记录仍停留在 40253 的时序。
        map[40253] = { 2.0f, 0.0f };
        map[40265] = { 2.0f, 0.0f };

        // 火山间歇泉（触发链 40117->42055->42052，施法者为火山 120510）：
        // 42052 为直伤（DBC 4163-4837），2 倍 -> 8326-9674；42055 同理覆盖首跳时序。
        map[42055] = { 2.0f, 0.0f };
        map[42052] = { 2.0f, 0.0f };

        // ===================== 古尔图格·血沸（Gurtogg Bloodboil，黑暗神庙复刻） =====================
        // 邪酸吐息：直伤（DBC 2850-3150）+ DOT（DBC 2750/跳）。
        // 直伤 3 倍 -> 8550-9450；DOT 保持原值 2750（已对齐奥杜尔 DOT 区间）。
        map[40508] = { 3.0f, 1.0f };
        map[40595] = { 3.0f, 1.0f }; // 邪酸吐息（邪能狂怒期间，DOT 周期更短）
        map[42005] = { 0.0f, 4.0f }; // 血沸（DOT，DBC 600/跳）：-> 2400/跳

        // 召唤物技能（施法者为邪能间歇泉 120511，通过 WorldBossSummonAI 缩放）。
        map[40593] = { 2.5f, 0.0f }; // 邪能间歇泉伤害（AOE + 击退，DBC 3238-3762）：-> 8095-9405
        // 注：弧光粉碎（40457/40599）为物理近战，不缩放。

        // ===================== 盲眼者莱欧瑟拉斯（Leotheras the Blind，毒蛇神殿复刻） =====================
        // 旋风斩（触发链 37640->37641）：37641 为直伤（DBC 3000）+ 流血 DOT（DBC 2500/跳），
        // 直伤 3 倍 -> 9000，DOT 保持 2500；37640 为周期触发光环，一并加入以覆盖首跳伤害时施法记录仍停留在 37640 的时序。
        map[37641] = { 3.0f, 1.0f };
        map[37640] = { 3.0f, 0.0f };

        // 混沌冲击（触发链 37674->37675，施法者为莱欧瑟拉斯本体及莱欧瑟拉斯之影 120512）：
        // 37675 伤害由脚本造成（E2 为增伤 debuff），参考倍率 2.0；37674 为 DUMMY 入口，覆盖触发时序。
        map[37675] = { 2.0f, 0.0f };
        map[37674] = { 2.0f, 0.0f };

        // 召唤物技能（施法者为内心的恶灵 120513，通过 WorldBossSummonAI 缩放）。
        map[39309] = { 2.0f, 0.0f }; // 暗影箭（内心的恶灵，DBC 3375-4125）：-> 6750-8250
        // 注：疯狂吞噬（37749）为范围魅惑（AOE_CHARM）惩罚机制，非标准周期伤害，不缩放；其后的死亡由 37750 脚本实现。

        // ===================== 莫洛格里·踏潮者（Morogrim Tidewalker，毒蛇神殿复刻） =====================
        map[37730] = { 2.5f, 0.0f }; // 潮汐波（坦克正面 AOE，DBC 3938-5062）：-> 9845-12655
        map[37764] = { 2.0f, 0.0f }; // 地震（全团 AOE，DBC 4000-4200）：-> 8000-8400

        // 水之墓（触发链 38028->38023/38024/38025/37850->37852）：
        // 37852 为直伤（DBC 4625-5375），2 倍 -> 9250-10750；
        // 38028 为 DUMMY 入口，一并加入以覆盖触发伤害时施法记录仍停留在 38028 的时序。
        map[37852] = { 2.0f, 0.0f };
        map[38028] = { 2.0f, 0.0f };

        // 召唤物技能（施法者为水晶体 120515，通过 WorldBossSummonAI 缩放）。
        map[37871] = { 1.5f, 0.0f }; // 冻结（水晶体，DBC 6563-7437）：-> 9844-11155

        // ===================== 阿克蒙德（Archimonde，海加尔山之战复刻） =====================
        map[32014] = { 3.0f, 0.0f }; // 气爆（随机目标 3000 自然 + 击飞）：-> 9000
        map[31972] = { 0.0f, 1.2f }; // 军团之握（诅咒 DOT，DBC 2500/跳）：-> 3000/跳
        map[31984] = { 1.0f, 0.0f }; // 死亡一指（无近战目标惩罚，DBC 20000）：保持原值

        // 毁灭之火伤害链（施法者为毁灭之火 120516，通过 WorldBossSummonAI 缩放）：
        // 31945（周期触发）-> 31943（区域光环）-> 31944（2400 火焰直伤）。
        // 31944 直伤结算时施法记录：首跳停留在 31943，后续跳停留在 31944（OnSpellCast 在伤害之后触发），
        // 故 31943 与 31944 一并按直伤 2 倍缩放 -> 4800/跳。
        map[31943] = { 2.0f, 0.0f };
        map[31944] = { 2.0f, 0.0f };

        // ===================== 阿兹加洛（Azgalor，海加尔山之战复刻） =====================
        // 火雨（31340 区域周期伤害 DBC 1618-1881 + 触发 31341 火雨 DOT DBC 1250/跳）。
        // 31341 施加到目标身上，2 倍 -> 2500/跳；31340 的区域周期伤害会借助 31341 的 periodic 倍率缩放（时序相关）。
        map[31341] = { 0.0f, 2.0f };
        // 注：阿兹加洛的嚎叫（31344）为范围沉默、顺劈斩（31345）为物理近战、末日诅咒（31347）到期由核心硬编码 Unit::Kill 秒杀，均不缩放。

        return map;
    }

    std::unordered_map<uint32, WorldBossSpellScale> const WorldBossSpellScaling = BuildWorldBossSpellScaling();

    // 归属锁定状态（供全局 reaction hook 查询）。key 为 BOSS 本体 GUID。
    struct WorldBossLockState
    {
        ObjectGuid ownerGuid; // 开怪玩家
        ObjectGuid groupGuid; // 开怪玩家所在队伍/团队（空则单人）
    };

    std::unordered_map<ObjectGuid, WorldBossLockState> g_worldBossLocks;

    // 扫描目标身上由 caster 施加、且在缩放表内的周期伤害法术，返回其 spellId。
    // 同一目标身上可能同时存在多条缩放表内的 DOT（如血沸 42005 为物理、邪酸吐息 40508/40595 为火焰），
    // 仅按“命中缩放表”取第一条会套用错误倍数，故再按本次结算的法术学派过滤。
    uint32 GetWorldBossPeriodicSpellId(Unit* caster, Unit* victim, SpellSchoolMask schoolMask)
    {
        if (!victim)
            return 0;

        for (AuraEffect const* auraEffect : victim->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE))
        {
            if (!auraEffect || auraEffect->GetCasterGUID() != caster->GetGUID())
                continue;

            SpellInfo const* auraSpell = auraEffect->GetSpellInfo();
            if (auraSpell->GetSchoolMask() != schoolMask)
                continue;

            if (GetWorldBossSpellScale(auraSpell->Id))
                return auraSpell->Id;
        }

        return 0;
    }

    // 查询某法术在指定伤害类型下的倍率；未登记或该侧不缩放（0）时返回 0。
    float GetWorldBossSpellMultiplier(uint32 spellId, DamageEffectType damageType)
    {
        WorldBossSpellScale const* scale = GetWorldBossSpellScale(spellId);
        if (!scale)
            return 0.0f;

        return (damageType == DOT) ? scale->periodic : scale->direct;
    }

    // 施法者当前正在结算的法术（读条 / 引导 / 非触发型瞬发均会登记，触发型瞬发不会）。
    uint32 GetWorldBossCastingSpellId(Unit* caster)
    {
        for (CurrentSpellTypes spellType : { CURRENT_GENERIC_SPELL, CURRENT_CHANNELED_SPELL })
            if (Spell const* spell = caster->GetCurrentSpell(spellType))
                if (SpellInfo const* spellInfo = spell->GetSpellInfo())
                    return spellInfo->Id;

        return 0;
    }

    // 共享缩放：按倍率表对一次伤害结算统一放大（参考裂隙 BossAIBase::DamageDealt）。
    void ScaleWorldBossSpellDamage(Unit* caster, Unit* victim, uint32& damage,
        DamageEffectType damageType, SpellSchoolMask schoolMask, uint32 lastCastSpellId)
    {
        // 近战（DIRECT_DAMAGE）不缩放。
        if (damageType == DIRECT_DAMAGE)
            return;

        float multiplier = 0.0f;

        if (damageType == DOT)
        {
            // DOT 通过扫描目标身上的周期伤害光环识别。
            multiplier = GetWorldBossSpellMultiplier(GetWorldBossPeriodicSpellId(caster, victim, schoolMask), damageType);
        }
        else
        {
            // 直伤通过最近施放的法术识别。
            multiplier = GetWorldBossSpellMultiplier(lastCastSpellId, damageType);

            // 瞬发法术不触发 OnSpellStart，且 OnSpellCast 在伤害结算之后才更新记录，
            // 故结算时记录到的仍是上一个法术（如邪酸吐息 40595 结算时记录停在血沸 42005）。
            // 技能直伤在记录法术上取不到倍率时，回退用施法者当前正在结算的法术。
            if (multiplier <= 0.0f && damageType == SPELL_DIRECT_DAMAGE)
                multiplier = GetWorldBossSpellMultiplier(GetWorldBossCastingSpellId(caster), damageType);
        }

        if (multiplier <= 0.0f)
            return;

        damage = uint32(std::min<double>(double(damage) * multiplier, std::numeric_limits<uint32>::max()));
    }
}

WorldBossSpellScale const* GetWorldBossSpellScale(uint32 spellId)
{
    auto const it = WorldBossSpellScaling.find(spellId);
    return it == WorldBossSpellScaling.end() ? nullptr : &it->second;
}

// ===================== WorldBossGuardAI =====================

WorldBossGuardAI::WorldBossGuardAI(Creature* creature) : WorldBossAI(creature) { }

void WorldBossGuardAI::JustEngagedWith(Unit* who)
{
    _JustEngagedWith();
    LockToGroup(who);
}

void WorldBossGuardAI::Reset()
{
    _Reset();
    Unlock();
}

void WorldBossGuardAI::JustDied(Unit* killer)
{
    _JustDied();
    Unlock();
}

void WorldBossGuardAI::EnterEvadeMode(EvadeReason why)
{
    // 必须在基类的 MoveTargetedHome() 之前还原回家基准（CheckLeash() 在战斗中会临时改写它），
    // 否则“走回家”的目标点会变成最后一个战斗位置。
    RestoreCoreHome();

    WorldBossAI::EnterEvadeMode(why);
}

void WorldBossGuardAI::DamageDealt(Unit* victim, uint32& damage, DamageEffectType damageType, SpellSchoolMask schoolMask)
{
    ScaleWorldBossSpellDamage(me, victim, damage, damageType, schoolMask, _lastCastSpellId);
}

void WorldBossGuardAI::OnSpellCast(SpellInfo const* spell)
{
    if (spell)
        _lastCastSpellId = spell->Id;
}

void WorldBossGuardAI::OnSpellStart(SpellInfo const* spell)
{
    if (spell)
        _lastCastSpellId = spell->Id;
}

bool WorldBossGuardAI::CheckLeash()
{
    // 脱战防护：核心 CanCreatureAttack 存在若干「对全体目标同时生效」的 BOSS 侧否决条件，
    // 任一命中都会让所有仇恨引用被置为 OFFLINE（ThreatReference::ShouldBeOffline），
    // GetCurrentVictim() 因此返回 nullptr，Unit::SelectVictim 走到末尾用 EVADE_REASON_OTHER 兜底脱战
    // （本框架BOSS带 CREATURE_FLAG_EXTRA_HARD_RESET，脱战即消失，观感就是“打着打着BOSS没了”）。
    // 本体不使用这些状态表达机制（需要“打不到”时用 UNIT_FLAG_NOT_SELECTABLE + REACT_PASSIVE），
    // 因此锁定期间直接清理；正常脱战会走 Reset()→Unlock()，_locked 先变 false，不会干扰退场流程。
    if (_locked)
    {
        // 1) 核心回家距离判定（CreatureLeashRadius，默认 30 码）：
        //    本体保留 CREATURE_TYPE_FLAG_BOSS_MOB（“??”等级与免疫击退），核心因此不对其启用
        //    「近期受伤即可离开刷新点」豁免，该判定恒定生效。化解分两层，缺一不可：
        //    ① 基准点：CanCreatureAttack 优先取 IDLE 运动生成器的 GetResetPosition()，
        //       返回 false 时才回落到 m_homePosition。本体 MovementType=1（随机漫游），IDLE 槽里的
        //       RandomMovementGenerator 返回的是漫游目标点/初始点（≈刷新点），
        //       故先把它换成默认 idle（IdleMovementGenerator 未重写该方法，基类返回 false）。
        //    ② 基准值：让 m_homePosition 跟随 BOSS 自身，使 30 码判定恒为真。
        //    脱战/退场前 RestoreCoreHome() 还原真实出生点；IDLE 槽由核心 MotionMaster::InitDefault() 自动重建。
        MovementGenerator* idleSlot = me->GetMotionMaster()->GetMotionSlot(MOTION_SLOT_IDLE);
        float rx, ry, rz;
        if (idleSlot && idleSlot->GetResetPosition(rx, ry, rz))
            me->GetMotionMaster()->MoveIdle();

        if (me->GetDistance(me->GetHomePosition()) > 1.0f)
            me->SetHomePosition(me->GetPosition());

        // 2) 仇恨列表缓存：核心只在构造时算一次，为 false 时 Unit::AddThreat 会被拒、
        //    SelectVictim 不再调用 GetCurrentVictim()，已有引用会永久停在 OFFLINE 并最终兜底脱战。
        if (!me->GetThreatMgr().CanHaveThreatList())
            me->GetThreatMgr().Initialize();

        // 3) 免PC/免NPC 旗标：Unit::_IsValidAttackTarget 双向拦截，ThreatReference::FlagsAllowFighting 也会拦。
        if (me->IsImmuneToPC() || me->IsImmuneToNPC())
        {
            me->SetImmuneToPC(false, false);
            me->SetImmuneToNPC(false, false);
        }

        // 4) 残留 UNIT_STATE_EVADE：Creature::CanCreatureAttack 开头的 IsInEvadeMode() 直接返回 false。
        if (me->IsInEvadeMode())
            me->ClearUnitState(UNIT_STATE_EVADE);
    }

    if (_locked && me->IsInCombat() && me->GetDistance(_homePosition) > WORLD_BOSS_LEASH_RANGE)
    {
        EnterEvadeMode(EVADE_REASON_BOUNDARY);
        return true;
    }
    return false;
}

void WorldBossGuardAI::LockToGroup(Unit* who)
{
    _homePosition = me->GetPosition();
    _savedHomePosition = me->GetHomePosition(); // 保存真实出生点，脱战/退场前还原（见 RestoreCoreHome）
    _hasSavedHome = true;
    _locked = true;
    _ownerGuid.Clear();
    _groupGuid.Clear();

    // 追溯开怪者背后的玩家（覆盖宠物 / NPCBot 开怪）。
    Player* owner = who ? who->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
    if (!owner)
        return; // 非玩家开怪（极少见），不锁定具体队伍

    _ownerGuid = owner->GetGUID();
    if (Group const* group = owner->GetGroup())
        _groupGuid = group->GetGUID();

    // 同步到全局锁定表，供 reaction hook 使用。
    g_worldBossLocks[me->GetGUID()] = { _ownerGuid, _groupGuid };
}

void WorldBossGuardAI::Unlock()
{
    _locked = false;
    _ownerGuid.Clear();
    _groupGuid.Clear();
    g_worldBossLocks.erase(me->GetGUID());

    RestoreCoreHome();
}

void WorldBossGuardAI::RestoreCoreHome()
{
    if (!_hasSavedHome)
        return;

    me->SetHomePosition(_savedHomePosition);
    _hasSavedHome = false;
}

// ===================== WorldBossSummonAI =====================

WorldBossSummonAI::WorldBossSummonAI(Creature* creature) : ScriptedAI(creature) { }

void WorldBossSummonAI::DamageDealt(Unit* victim, uint32& damage, DamageEffectType damageType, SpellSchoolMask schoolMask)
{
    ScaleWorldBossSpellDamage(me, victim, damage, damageType, schoolMask, _lastCastSpellId);
}

void WorldBossSummonAI::OnSpellCast(SpellInfo const* spell)
{
    if (spell)
        _lastCastSpellId = spell->Id;
}

void WorldBossSummonAI::OnSpellStart(SpellInfo const* spell)
{
    if (spell)
        _lastCastSpellId = spell->Id;
}

// ===================== 全局 reaction hook =====================

// 非开怪队伍成员对已锁定的世界BOSS视为中立，从而无法攻击。
// 该机制必须在 Unit::GetReactionTo 层面实现，AI 基类无法覆盖，因此保留此轻量全局 hook。
class world_boss_reaction_guard : public UnitScript
{
public:
    world_boss_reaction_guard()
        : UnitScript("world_boss_reaction_guard", true, { UNITHOOK_IF_NORMAL_REACTION }) { }

    bool IfNormalReaction(Unit const* unit, Unit const* target, ReputationRank& repRank) override
    {
        // 定位 BOSS 本体与“玩家方”（玩家本人或其宠物 / NPCBot）。
        Unit const* boss = nullptr;
        Unit const* playerSide = nullptr;

        if (unit && unit->IsCreature() && IsWorldBossBodyEntry(unit->GetEntry()))
        {
            boss = unit;
            playerSide = target;
        }
        else if (target && target->IsCreature() && IsWorldBossBodyEntry(target->GetEntry()))
        {
            boss = target;
            playerSide = unit;
        }
        else
        {
            return true; // 与自定义世界BOSS本体无关，正常处理
        }

        auto const it = g_worldBossLocks.find(boss->GetGUID());
        if (it == g_worldBossLocks.end())
            return true; // 未锁定，正常敌对

        Player* player = playerSide ? playerSide->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!player)
            return true; // 非玩家控制，正常处理

        // 开怪者本人。
        if (player->GetGUID() == it->second.ownerGuid)
            return true;

        // 同一队伍/团队的其他成员。
        if (!it->second.groupGuid.IsEmpty())
            if (Group const* group = player->GetGroup())
                if (group->GetGUID() == it->second.groupGuid)
                    return true;

        // 其他玩家：中立，无法攻击。
        repRank = REP_NEUTRAL;
        return false;
    }
};

void AddSC_world_boss_guard()
{
    new world_boss_reaction_guard();
}
