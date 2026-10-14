#include "bot_ai.h"
#include "botdatamgr.h"
#include "botmgr.h"
#include "bottext.h"
#include "bottraits.h"
#include "Creature.h"
#include "Group.h"
#include "Item.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "WorldSession.h"
/*
Rogue NpcBot (reworked by Trickerer onlysuffering@gmail.com)
Complete - 90%
TODO:
*/

enum RogueBaseSpells
{
    KICK_1                              = 1766,
    EXPOSE_ARMOR_1                      = 8647, //NYI
    FEINT_1                             = 1966,
    DISMANTLE_1                         = 51722,

    BACKSTAB_1                          = 53,
    SINISTER_STRIKE_1                   = 1752,
    EVISCERATE_1                        = 2098,
    ENVENOM_1                           = 32645,
    RUPTURE_1                           = 1943,
    MUTILATE_1                          = 1329,
    HEMORRHAGE_1                        = 16511,
    GHOSTLY_STRIKE_1                    = 14278,
    RIPOSTE_1                           = 14251,
    DEADLY_THROW_1                      = 26679,
    FAN_OF_KNIVES_1                     = 51723,

    SPRINT_1                            = 2983,
    EVASION_1                           = 5277,
    BLIND_1                             = 2094,
    VANISH_1                            = 1856,
    COLD_BLOOD_1                        = 14177,
    HUNGER_FOR_BLOOD_1                  = 51662,
    ADRENALINE_RUSH_1                   = 13750,
    KILLING_SPREE_1                     = 51690,
    PREPARATION_1                       = 14185,
    PREMEDITATION_1                     = 14183,

    GOUGE_1                             = 1776,

    KIDNEY_SHOT_1                       = 408,
    SLICE_DICE_1                        = 5171,
    BLADE_FLURRY_1                      = 13877,
    SHADOWSTEP_1                        = 36554,
    CLOAK_OF_SHADOWS_1                  = 31224,
    TRICKS_OF_THE_TRADE_1               = 57934,
    SHADOW_DANCE_1                      = 51713,

    STEALTH_1                           = 1784,
    SAP_1                               = 6770,
    GARROTE_1                           = 703,
    CHEAP_SHOT_1                        = 1833,
    AMBUSH_1                            = 8676,

    DISTRACT_1                          = 1725, //NYI
    DISARM_TRAP_1                       = 1842, //Unused, see bot_ai::ProcessImmediateNonAttackTarget()

    //Poisons
    CRIPPLING_POISON_1                  = 3408,
    INSTANT_POISON_1                    = 8679,
    DEADLY_POISON_1                     = 2823,
    WOUND_POISON_1                      = 13219,
    MIND_NUMBING_POISON_1               = 5761, //manual use only
    ANESTHETIC_POISON_1                 = 26785,

    PICK_LOCK_1                         = 1804
};

//其他职业的破甲类 debuff（破甲检测用，避免重复施放破甲）
//只列与破甲同槽位、互相覆盖的“-20% 护甲”效果；
//不含精灵之火（仅 -5% 护甲，与破甲叠加，不应据此跳过破甲）
enum RogueExternalArmorDebuffs
{
    SUNDER_ARMOR_DEBUFF                 = 58567 //战士破甲（WotLK 各等级共用同一 debuff）
};

enum RoguePassives
{
    //Talents
    SEAL_FATE1                          = 14189,
    SEAL_FATE2                          = 14190,
    SEAL_FATE3                          = 14193,
    SEAL_FATE4                          = 14194,
    SEAL_FATE5                          = 14195,
    COMBAT_POTENCY1                     = 35541,
    COMBAT_POTENCY2                     = 35550,
    COMBAT_POTENCY3                     = 35551,
    COMBAT_POTENCY4                     = 35552,
    COMBAT_POTENCY5                     = 35553,
    QUICK_RECOVERY1                     = 31244,
    QUICK_RECOVERY2                     = 31245,
    //BLADE_TWISTING1                     = 31124,
    //BLADE_TWISTING2                     = 31126,
    DEADLY_BREW                         = 51626,//rank 2
    IMPROVED_KIDNEY_SHOT                = 14176,//rank 3
    VIGOR                               = 14983,
    REMORSELESS_ATTACKS                 = 14148,//rank 2
    FLEET_FOOTED                        = 31209,//rank 2
    MURDER                              = 14159,//rank 2
    OVERKILL                            = 58426,
    FOCUSED_ATTACKS                     = 51636,//rank 3
    MASTER_POISONER                     = 58410,//rank 3
    DUAL_WIELD_SPECIALIZATION           = 13852,//rank 5
    IMPROVED_KICK                       = 13867,//rank 2
    IMPROVED_SPRINT                     = 13875,//rank 2
    HACK_AND_SLASH                      = 13964,//rank 5
    VITALITY                            = 61329,//rank 3
    NERVES_OF_STEEL                     = 31131,//rank 2
    THROWING_SPECIALIZATION             = 51679,//rank 2
    //SAVAGE_COMBAT                       = 58413,//rank 2
    UNFAIR_ADVANTAGE                    = 51674,//rank 2
    SURPRISE_ATTACKS                    = 32601,
    PREY_ON_THE_WEAK                    = 51689,//rank 5
    MASTER_OF_DECEPTION                 = 13971,//rank 3
    SETUP                               = 14071,//rank 3
    INITIATIVE                          = 13980,//rank 3
    DIRTY_DEEDS                         = 14083,//rank 2
    MASTER_OF_SUBTLETY                  = 31223,//rank 3
    CHEAT_DEATH                         = 31230,//rank 3
    ENVELOPING_SHADOWS                  = 31213,//rank 3
    TURN_THE_TABLES                     = 51629,//rank 3
    HONOR_AMONG_THIEVES                 = 51701,//rank 3

    //Other
    VIGOR_GLADIATOR                     = 21975,
    GLYPH_BACKSTAB                      = 56800,

    ROGUE_PASSIVE_DND                   = 21184 //from playercreateinfo_spell
};

enum RogueSpecial
{
    MUTILATE_DAMAGE_MAINHAND_1          = 5374,
    MUTILATE_DAMAGE_OFFHAND_1           = 27576,

    //TURN_THE_TABLES_BUFF                = 52910,//'rank 3'
    HUNGER_FOR_BLOOD_BUFF               = 63848,
    WAYLAY_DEBUFF                       = 51693,
    REMORSELESS_ATTACKS_BUFF            = 14149,
    CHEATING_DEATH_BUFF                 = 45182, //hidden
    TRICKS_OF_THE_TRADE_BUFF            = 57933,

    RELENTLESS_STRIKES_EFFECT           = 14181,
    RUTHLESSNESS_EFFECT                 = 14157,
    SEAL_FATE_EFFECT                    = 14189,
    SETUP_EFFECT                        = 15250,
    INITIATIVE_EFFECT                   = 13977,
    HONOR_AMONG_THIEVES_EFFECT          = 51699,

    VANISH_TRIGGERED_1                  = 11327,
    VANISH_TRIGGERED_2                  = 11329,
    VANISH_TRIGGERED_3                  = 26888,

    //Poisons
    CRIPPLING_POISON_PROC_1             = 3409,
    //INSTANT_POISON_PROC_1               = 8680,
    DEADLY_POISON_PROC_1                = 2818,
    WOUND_POISON_PROC_1                 = 13218,
    MIND_NUMBING_POISON_PROC_1          = 5760,
    //ANESTHETIC_POISON_PROC_1            = 26688,

    THISTLE_TEA                         = 9512 //'Restore Energy' 1 min cd
};

static const std::vector<uint32> Rogue_spells_damage
{ AMBUSH_1, BACKSTAB_1, DEADLY_THROW_1, EVISCERATE_1, ENVENOM_1, FAN_OF_KNIVES_1, GARROTE_1, GHOSTLY_STRIKE_1, GOUGE_1,
HEMORRHAGE_1, KILLING_SPREE_1, MUTILATE_1, RIPOSTE_1, RUPTURE_1, SINISTER_STRIKE_1 };
static const std::vector<uint32> Rogue_spells_cc{ BLIND_1, CHEAP_SHOT_1, /*DEADLY_THROW_1, */DISMANTLE_1, GOUGE_1, KICK_1, KIDNEY_SHOT_1, /*SAP_1*/ };
static const std::vector<uint32> Rogue_spells_support
{ EXPOSE_ARMOR_1, /*DISTRACT_1, PICK_LOCK_1,*/ STEALTH_1, ADRENALINE_RUSH_1, BLADE_FLURRY_1, CLOAK_OF_SHADOWS_1,
COLD_BLOOD_1, DISMANTLE_1, EVASION_1, FEINT_1, HUNGER_FOR_BLOOD_1, PREMEDITATION_1, PREPARATION_1, SHADOW_DANCE_1,
SHADOWSTEP_1, SLICE_DICE_1, SPRINT_1, TRICKS_OF_THE_TRADE_1, VANISH_1, DISARM_TRAP_1, THISTLE_TEA,
/*CRIPPLING_POISON_1, INSTANT_POISON_1, DEADLY_POISON_1, WOUND_POISON_1, MIND_NUMBING_POISON_1, ANESTHETIC_POISON_1*/ };

//天赋触发概率具名常量（与硬编码天赋 rank 对应，为后续按 rank 计算预留）
static constexpr uint32 RUTHLESSNESS_PROC_PCT = 60;   //无情打击 3/3
static constexpr uint32 RELENTLESS_RS_PCT_PER_CP = 20; //无情打击(终结技充能) 每连击点
static constexpr uint32 GLYPH_SS_PROC_PCT = 50;        //邪恶攻击雕文

//天赋描述表项：专精 + [minLvl, maxLvl) 半开等级区间 + 被动光环
//maxLvl == 0 表示无等级上限
struct RogueTalentDef
{
    uint8  spec;
    uint8  minLvl;
    uint8  maxLvl;
    uint32 auraId;
};

//“任意专精”哨兵值：用于通用天赋，避免遍历时漏项
static constexpr uint8 ROGUE_ANY_SPEC = 0xFF;

//天赋描述表：取代 ApplyClassPassives 中散落的 level/spec 三元表达式
static constexpr RogueTalentDef RogueTalents[] =
{
    //通用（任意专精）
    { ROGUE_ANY_SPEC,               10,  0, REMORSELESS_ATTACKS },
    { ROGUE_ANY_SPEC,               10,  0, DUAL_WIELD_SPECIALIZATION },
    { ROGUE_ANY_SPEC,               10,  0, MASTER_OF_DECEPTION },
    { ROGUE_ANY_SPEC,               15,  0, GLYPH_BACKSTAB },
    { ROGUE_ANY_SPEC,               20,  0, VIGOR },
    { ROGUE_ANY_SPEC,               70,  0, VIGOR_GLADIATOR },
    //刺杀
    { BOT_SPEC_ROGUE_ASSASINATION,  30, 35, QUICK_RECOVERY1 },
    { BOT_SPEC_ROGUE_ASSASINATION,  35,  0, QUICK_RECOVERY2 },
    { BOT_SPEC_ROGUE_ASSASINATION,  30,  0, IMPROVED_KIDNEY_SHOT },
    { BOT_SPEC_ROGUE_ASSASINATION,  30,  0, FLEET_FOOTED },
    { BOT_SPEC_ROGUE_ASSASINATION,  35, 37, SEAL_FATE1 },
    { BOT_SPEC_ROGUE_ASSASINATION,  37, 39, SEAL_FATE2 },
    { BOT_SPEC_ROGUE_ASSASINATION,  39, 42, SEAL_FATE3 },
    { BOT_SPEC_ROGUE_ASSASINATION,  42, 45, SEAL_FATE4 },
    { BOT_SPEC_ROGUE_ASSASINATION,  45,  0, SEAL_FATE5 },
    { BOT_SPEC_ROGUE_ASSASINATION,  35,  0, MURDER },
    { BOT_SPEC_ROGUE_ASSASINATION,  40,  0, DEADLY_BREW },
    { BOT_SPEC_ROGUE_ASSASINATION,  40,  0, OVERKILL },
    { BOT_SPEC_ROGUE_ASSASINATION,  50,  0, MASTER_POISONER },
    //战斗
    { BOT_SPEC_ROGUE_COMBAT,        25,  0, IMPROVED_KICK },
    { BOT_SPEC_ROGUE_COMBAT,        25,  0, IMPROVED_SPRINT },
    { BOT_SPEC_ROGUE_COMBAT,        30,  0, HACK_AND_SLASH },
    { BOT_SPEC_ROGUE_COMBAT,        40,  0, VITALITY },
    { BOT_SPEC_ROGUE_COMBAT,        40,  0, NERVES_OF_STEEL },
    { BOT_SPEC_ROGUE_COMBAT,        45, 47, COMBAT_POTENCY1 },
    { BOT_SPEC_ROGUE_COMBAT,        47, 49, COMBAT_POTENCY2 },
    { BOT_SPEC_ROGUE_COMBAT,        49, 52, COMBAT_POTENCY3 },
    { BOT_SPEC_ROGUE_COMBAT,        52, 55, COMBAT_POTENCY4 },
    { BOT_SPEC_ROGUE_COMBAT,        55,  0, COMBAT_POTENCY5 },
    { BOT_SPEC_ROGUE_COMBAT,        45,  0, THROWING_SPECIALIZATION },
    { BOT_SPEC_ROGUE_COMBAT,        50,  0, UNFAIR_ADVANTAGE },
    { BOT_SPEC_ROGUE_COMBAT,        50,  0, SURPRISE_ATTACKS },
    { BOT_SPEC_ROGUE_COMBAT,        55,  0, PREY_ON_THE_WEAK },
    //敏锐
    { BOT_SPEC_ROGUE_SUBTLETY,      25,  0, SETUP },
    { BOT_SPEC_ROGUE_SUBTLETY,      25,  0, INITIATIVE },
    { BOT_SPEC_ROGUE_SUBTLETY,      30,  0, DIRTY_DEEDS },
    { BOT_SPEC_ROGUE_SUBTLETY,      35,  0, MASTER_OF_SUBTLETY },
    { BOT_SPEC_ROGUE_SUBTLETY,      40,  0, CHEAT_DEATH },
    { BOT_SPEC_ROGUE_SUBTLETY,      40,  0, ENVELOPING_SHADOWS },
};

//涂毒方案表项：专精 + [lvlMin, lvlMax) 半开等级区间 + 主/副手毒药
//lvlMax == 0 表示无等级上限
struct RoguePoisonPlan
{
    uint8  spec;
    uint8  lvlMin;
    uint8  lvlMax;
    uint32 mh;
    uint32 oh;
};

//PVE 涂毒方案（保持既有策略：刺杀主速效/副致命，其余主致伤、副致残/速效/麻醉）
static constexpr RoguePoisonPlan RoguePoisonPlansPVE[] =
{
    { BOT_SPEC_ROGUE_ASSASINATION, 20,  0, INSTANT_POISON_1, DEADLY_POISON_1 },
    { BOT_SPEC_ROGUE_COMBAT,       20, 32, INSTANT_POISON_1, CRIPPLING_POISON_1 },
    { BOT_SPEC_ROGUE_COMBAT,       32, 68, WOUND_POISON_1,   INSTANT_POISON_1 },
    { BOT_SPEC_ROGUE_COMBAT,       68,  0, WOUND_POISON_1,   ANESTHETIC_POISON_1 },
    { BOT_SPEC_ROGUE_SUBTLETY,     20, 32, INSTANT_POISON_1, CRIPPLING_POISON_1 },
    { BOT_SPEC_ROGUE_SUBTLETY,     32, 68, WOUND_POISON_1,   INSTANT_POISON_1 },
    { BOT_SPEC_ROGUE_SUBTLETY,     68,  0, WOUND_POISON_1,   ANESTHETIC_POISON_1 },
};

//PVP 涂毒方案（仅战场 / 竞技场生效：主致残减速，副脑麻痹降法速）
static constexpr RoguePoisonPlan RoguePoisonPlansPVP[] =
{
    { ROGUE_ANY_SPEC,              20,  0, CRIPPLING_POISON_1, MIND_NUMBING_POISON_1 },
};

static constexpr size_t ROGUE_POISON_PVE_COUNT = sizeof(RoguePoisonPlansPVE) / sizeof(RoguePoisonPlansPVE[0]);
static constexpr size_t ROGUE_POISON_PVP_COUNT = sizeof(RoguePoisonPlansPVP) / sizeof(RoguePoisonPlansPVP[0]);

//判断涂毒方案条目是否匹配当前专精与等级（maxLvl == 0 表示无上限）
inline bool RoguePoisonPlanMatches(RoguePoisonPlan const& p, uint8 spec, uint8 lvl)
{
    return (p.spec == ROGUE_ANY_SPEC || p.spec == spec) && lvl >= p.lvlMin && (!p.lvlMax || lvl < p.lvlMax);
}

//输出循环阈值常量（替代散落的魔法数字）
static constexpr uint32 ROGUE_SND_REFRESH_MS = 4000;      //切割剩余时长刷新阈值
//割裂基础时长显著短于切割，阈值过大会导致被提前覆盖、浪费连击点，
//因此仅在临近断档时补（1000ms）
static constexpr uint32 ROGUE_RUPTURE_REFRESH_MS = 1000;  //割裂剩余时长刷新阈值
static constexpr int32  ROGUE_SS_ENERGY_MARGIN = 15;      //邪恶攻击能量余量
static constexpr uint32 ROGUE_COUNTER_CHECK_INTERVAL = 150; //打断检查定频(ms)
static constexpr uint8  ROGUE_DEADLY_STACKS_ENOUGH = 4;   //Envenom 所需的致命毒层数

//切割 5 连击点基准时长组成（Cut to the Chase 重算用）
static constexpr uint32 ROGUE_SND_5CP_BASE_MS = 21000;    //5 CP 基础时长
static constexpr uint32 ROGUE_SND_GLYPH_MS = 3000;        //切割雕文
static constexpr uint32 ROGUE_SND_IMPROVED_MS = 12000;    //强化切割(天赋)

//一次性只读快照：本轮输出决策共享的上下文
//设计原则：只存“单次读取/单步判断”即可得到的字段，不做跨 tick 累计统计
struct RogueSnapshot
{
    Unit*  target = nullptr;
    float  dist = 0.f;
    int32  energy = 0;
    int32  maxEnergy = 0;
    uint8  comboPoints = 0;
    bool   stealthed = false;
    bool   shadowDance = false;
    bool   inCombat = false;
    bool   isTank = false;
    bool   isDPS = false;
    DiminishingLevels stunDrv = DIMINISHING_LEVEL_1;

    //Buff / Debuff 生命周期
    bool   hasHfB = false;
    bool   hasSnD = false;
    uint32 snDRemainMs = 0;
    bool   hasRupture = false;
    uint32 ruptureRemainMs = 0;
    uint8  targetDeadlyStacks = 0;  //按 me->GetGUID() 过滤后的致命毒层数

    //多目标分流依据
    uint32 enemiesIn7y = 0;

    //生存判定（极简：只用两个直接可读的输入）
    uint8  selfHealthPct = 0;   //GetHealthPCT(me)
    bool   hasAttackers = false;//!me->getAttackers().empty()

    //单步条件推导
    bool   targetBleeding = false;   //目标带 AURA_STATE_BLEEDING
    bool   isExecutePhase = false;   //目标血量 < 35%

    //目标控制状态（用于“凿击/致盲后等待回能”，仅 PVP 使用，非跨 tick 累计）
    bool   targetNormalStun = false; //目标处于普通昏迷/困惑(受击不醒)
    int32  targetCCDuration = 0;     //目标昏迷/困惑剩余时长(ms)
};

class rogue_bot : public CreatureScript
{
public:
    rogue_bot() : CreatureScript("rogue_bot") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new rogue_botAI(creature);
    }

    bool OnGossipHello(Player* player, Creature* creature) override
    {
        return creature->GetBotAI()->OnGossipHello(player, 0);
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (bot_ai* ai = creature->GetBotAI())
            return ai->OnGossipSelect(player, creature, sender, action);
        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, char const* code) override
    {
        if (bot_ai* ai = creature->GetBotAI())
            return ai->OnGossipSelectCode(player, creature, sender, action, code);
        return true;
    }

    struct rogue_botAI : public bot_ai
    {
        rogue_botAI(Creature* creature) : bot_ai(creature)
        {
            _botclass = BOT_CLASS_ROGUE;

            mhEnchantExpireTimer = 1;
            ohEnchantExpireTimer = 1;
            mhEnchant = 0;
            ohEnchant = 0;
            needChooseMHEnchant = true;
            needChooseOHEnchant = true;

            InitUnitFlags();
        }

        bool doCast(Unit* victim, uint32 spellId)
        {
            if (CheckBotCast(victim, spellId) != SPELL_CAST_OK)
                return false;
            return bot_ai::doCast(victim, spellId);
        }

        void StartAttack(Unit* u, bool force = false)
        {
            if (!bot_ai::StartAttack(u, force))
                return;
            GetInPosition(force, u);
        }

        void JustEngagedWith(Unit* u) override { bot_ai::JustEngagedWith(u); }
        void KilledUnit(Unit* u) override { bot_ai::KilledUnit(u); }
        void EnterEvadeMode(EvadeReason why = EVADE_REASON_OTHER) override { bot_ai::EnterEvadeMode(why); }
        void MoveInLineOfSight(Unit* u) override { bot_ai::MoveInLineOfSight(u); }
        void JustDied(Unit* u) override { comboPoints = 0; bot_ai::JustDied(u); }

        void getenergy()
        {
            energy = me->GetPower(POWER_ENERGY);
        }

        int32 ecost(uint32 spellId) const
        {
            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
                return spellInfo->CalcPowerCost(me, spellInfo->GetSchoolMask());
            return 0;
        }

        //打断：去掉随机门控，改为按 diff 定频的确定性判断
        //团队协同去重由基类 EnqueueCounterSpellAction -> BotMgr::ClaimInterruptTarget 统一完成，
        //同一目标同一时段只允许一个机器人认领，无需在此重复实现覆盖检测
        void Counter(uint32 diff)
        {
            _counterCheckTimer += diff;
            if (_counterCheckTimer < ROGUE_COUNTER_CHECK_INTERVAL)
                return;
            _counterCheckTimer = 0;

            if (me->HasAuraType(SPELL_AURA_MOD_STEALTH) || me->HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY))
                return;

            //当前目标正在读条（含引导）：优先本职踢击，冷却中改用凿击
            if (Unit const* u = me->GetVictim(); u && u->IsNonMeleeSpellCast(false,false,true))
            {
                if (IsSpellReady(KICK_1, diff, false) && !HasQueuedSpellAction(KICK_1))
                    if (EnqueueCounterSpellAction(u->GetGUID(), KICK_1, true))
                        return;
                if (IsSpellReady(GOUGE_1, diff, false) && HasRole(BOT_ROLE_DPS) && !HasQueuedSpellAction(GOUGE_1) &&
                    u->HasInArc(float(M_PI), me) && !CCed(u))
                    if (EnqueueCounterSpellAction(u->GetGUID(), GOUGE_1, true))
                        return;
            }

            //附近无人能打断的长读条：致盲兜底
            if (IsSpellReady(BLIND_1, diff, false) && !HasQueuedSpellAction(BLIND_1))
                if (Unit const* target = FindCastingTarget(CalcSpellMaxRange(BLIND_1), 0, BLIND_1))
                    if (EnqueueCounterSpellAction(target->GetGUID(), BLIND_1, true))
                        return;
        }

        void UpdateAI(uint32 diff) override
        {
            if (combopointsSpent)
            {
                combopointsSpent = false;
                comboPoints = 0;
            }

            getenergy();

            if (!GlobalUpdate(diff))
                return;

            DoVehicleActions(diff);
            if (!CanBotAttackOnVehicle())
                return;

            if (IsPotionReady())
            {
                if (GetHealthPCT(me) < 50)
                    DrinkPotion(false);
            }

            CheckRacials(diff);

            if (!me->IsInCombat())
                DoNonCombatActions(diff);

            Counter(diff);

            if (IsCasting())
                return;

            if (ProcessImmediateNonAttackTarget())
                return;

            CheckUsableItems(diff);

            CheckSprint(diff);
            //斗篷 / 消失已并入 Attack() 的生存段（方案 A），此处仅保留“无目标也可逃”的消失兜底
            CheckVanishFallback(diff);

            if (!CheckAttackTarget())
            {
                if (!me->IsInCombat() && RogueRoll(5) && me->HasAuraType(SPELL_AURA_MOD_STEALTH) &&
                    !me->GetAuraEffect(SPELL_AURA_MOD_INCREASE_SPEED, SPELLFAMILY_ROGUE, 0x800, 0x0, 0x0) && //vanish
                    !(!HasRole(BOT_ROLE_DPS) && GetLastWMOArea() == 29476))
                    me->RemoveAurasDueToSpell(STEALTH_1);
                return;
            }

            CheckBlind(diff);
            CheckPreparation(diff);
            CheckTricksOfTheTrade(diff);

            Attack(diff);
        }

        //输出决策入口：优先级由分段函数的调用顺序显式表达
        void Attack(uint32 diff)
        {
            Unit* mytar = opponent ? opponent : disttarget ? disttarget : nullptr;
            if (!mytar)
                return;

            StartAttack(mytar, IsMelee());

            CheckAttackState();
            if (!me->IsAlive() || !mytar->IsAlive())
                return;

            RogueSnapshot const snap = BuildSnapshot();

            //非战斗时择机进入潜行（行为抖动型随机，不消耗本回合）
            TryEnterStealth(snap, diff);

            if (!CanAffectVictimAny(mytar, SPELL_SCHOOL_NORMAL))
                return;

            //1) 紧急生存：最高优先级
            if (TryEmergencySurvival(snap, diff))
                return;

            //2) 必须控制：团队 CC / 灭团技读条 / 逃跑目标
            if (TryMustControl(snap, diff))
                return;

            //3) 常规生存 / 机动
            if (TryDefensiveUtilities(snap, diff))
                return;

            //超出近战范围：仅保留远程手段
            if (snap.dist > 5.f)
            {
                TryRangedAttack(snap, diff);
                return;
            }

            MoveBehind(mytar);

            //茶（续航，无 GCD）
            if (IsSpellReady(THISTLE_TEA, diff, false) && !snap.targetNormalStun && snap.targetCCDuration < 1000 &&
                snap.energy <= std::max<int32>(snap.maxEnergy - 110, 10))
            {
                if (doCast(me, THISTLE_TEA))
                    getenergy();
            }

            //No IsSpellReady checks for spells with no cd below
            if (GC_Timer > diff)
                return;

            //Feint：仇恨过高时泄怒（去随机）
            if (mytar->CanHaveThreatList() && IsSpellReady(FEINT_1, diff) && !snap.stealthed && !snap.isTank &&
                mytar->GetVictim() == me && energy >= ecost(FEINT_1) &&
                int32(mytar->GetThreatMgr().GetThreatListSize()) > 1 && int32(mytar->getAttackers().size()) > 1)
            {
                if (doCast(mytar, GetSpell(FEINT_1)))
                    return;
            }

            //4) 爆发窗口（自带 CP / CD 门控）
            if (TryBurstAbilities(snap, diff))
                return;

            //5) Buff 维持（SnD / Rupture / HfB / Hemo / 破甲）
            if (TryMaintainBuffs(snap, diff))
                return;

            //目标被凿击 / 致盲中：若能量未满则等待回能，避免打破控制
            //仅限 PVP：PVE 单体长时间等待会直接损失输出，不做此保留
            if (IsPvpContext() && !snap.targetNormalStun && snap.targetCCDuration > 300 &&
                uint32(snap.energy) < snap.maxEnergy)
                return;

            //6) 多目标（3+）
            if (snap.enemiesIn7y >= 3 && TryMultiTarget(snap, diff))
                return;

            //7) 清 CP
            if (TryFinishers(snap, diff))
                return;

            //8) 攒 CP
            if (TryBuilders(snap, diff))
                return;
        }

        //构建一次性只读快照
        RogueSnapshot BuildSnapshot() const
        {
            RogueSnapshot snap;
            snap.target = opponent ? opponent : disttarget ? disttarget : nullptr;
            if (!snap.target)
                return snap;

            Unit* mytar = snap.target;
            snap.dist = me->GetDistance(mytar);
            snap.energy = energy;
            snap.maxEnergy = me->GetMaxPower(POWER_ENERGY);
            snap.comboPoints = comboPoints;
            snap.stealthed = me->HasAuraType(SPELL_AURA_MOD_STEALTH);
            snap.shadowDance = me->HasAuraType(SPELL_AURA_MOD_IGNORE_SHAPESHIFT);
            snap.inCombat = me->IsInCombat();
            snap.isTank = IsTank();
            snap.isDPS = HasRole(BOT_ROLE_DPS);
            snap.stunDrv = mytar->GetDiminishing(DIMINISHING_OPENING_STUN);
            snap.selfHealthPct = GetHealthPCT(me);
            snap.hasAttackers = !me->getAttackers().empty();
            snap.targetBleeding = mytar->HasAuraState(AURA_STATE_BLEEDING);
            snap.isExecutePhase = mytar->GetHealthPct() < 35.f;

            //Hunger for Blood
            snap.hasHfB = me->GetAuraEffect(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, SPELLFAMILY_ROGUE, 0x0, 0x1000000, 0x0) != nullptr;
            //Slice and Dice
            if (uint32 snd = GetSpell(SLICE_DICE_1))
            {
                if (Aura const* dice = me->GetAura(snd))
                {
                    snap.hasSnD = true;
                    snap.snDRemainMs = uint32(std::max<int32>(dice->GetDuration(), 0));
                }
            }
            //Rupture（按施法者过滤）
            if (uint32 rupt = GetSpell(RUPTURE_1))
            {
                if (Aura const* aura = mytar->GetAura(rupt, me->GetGUID()))
                {
                    snap.hasRupture = true;
                    snap.ruptureRemainMs = uint32(std::max<int32>(aura->GetDuration(), 0));
                }
            }
            //致命毒层数（先按施法者过滤，避免把他人毒药算进自己的 Envenom 收益）
            if (Aura const* deadly = mytar->GetAura(DEADLY_POISON_PROC_1, me->GetGUID()))
                snap.targetDeadlyStacks = deadly->GetStackAmount();

            //多目标分流依据
            std::list<Unit*> targets;
            GetNearbyTargetsList(targets, 7.f, 1);
            snap.enemiesIn7y = uint32(targets.size());

            //目标控制状态
            for (AuraEffect const* aeff : mytar->GetAuraEffectsByType(SPELL_AURA_MOD_STUN))
            {
                if (!(aeff->GetSpellInfo()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_TAKE_DAMAGE) &&
                    aeff->GetBase()->GetDuration() > 2000)
                {
                    snap.targetNormalStun = true;
                    break;
                }
                if (aeff->GetBase()->GetDuration() > snap.targetCCDuration)
                    snap.targetCCDuration = aeff->GetBase()->GetDuration();
            }
            if (!snap.targetNormalStun)
            {
                for (AuraEffect const* aeff : mytar->GetAuraEffectsByType(SPELL_AURA_MOD_CONFUSE))
                {
                    if (!(aeff->GetSpellInfo()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_TAKE_DAMAGE) &&
                        aeff->GetBase()->GetDuration() > 2000)
                    {
                        snap.targetNormalStun = true;
                        break;
                    }
                    if (aeff->GetBase()->GetDuration() > snap.targetCCDuration)
                        snap.targetCCDuration = aeff->GetBase()->GetDuration();
                }
            }

            return snap;
        }

        //择机进入潜行（不消耗回合）
        void TryEnterStealth(RogueSnapshot const& snap, uint32 diff)
        {
            //Stealth (for Cooldown handling see bot_ai::ReleaseSpellCooldown)
            //we don't want rogue to switch into stealth for no purpose
            if (!IsSpellReady(STEALTH_1, diff, false) || me->IsInCombat() || IsTank() || !RogueRoll(50) ||
                snap.dist >= 28 || IsFlagCarrier(me))
                return;
            if (me->HasAuraType(SPELL_AURA_PERIODIC_DAMAGE) && !(snap.target->IsPlayer() && snap.dist < 6))
                return;
            if (me->GetLevel() < 35 && !(energy >= 40 && me->GetLevel() >= 30) && snap.dist <= 8)
                return;

            doCast(me, GetSpell(STEALTH_1));
        }

        //—— 抢占段 ——
        //紧急生存：只用“血量 + 有无攻击者”，固定分档（Cloak -> Vanish）
        bool TryEmergencySurvival(RogueSnapshot const& snap, uint32 diff)
        {
            //斗篷：魔法危机判定本身即“紧急”（debuff / 读条计数 count > 1，原样保留），
            //因此不受下方 35% 血量档位限制，作为紧急段的一部分单独检查
            if (TryCloakOfShadows(diff))
                return true;

            //紧急档以外不处理（常规档 Evasion 由 TryDefensiveUtilities 负责）
            if (snap.selfHealthPct >= 35 || !snap.hasAttackers)
                return false;

            //消失：重置仇恨 / 规避即将命中的法术
            if (!snap.stealthed && !snap.shadowDance && !IsTank() && IsSpellReady(VANISH_1, diff, false) &&
                !me->HasAuraType(SPELL_AURA_PERIODIC_DAMAGE) && !IsFlagCarrier(me) &&
                !me->HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY))
            {
                bool cast = false;
                //case 1: restealth for opener
                if (!snap.targetNormalStun && snap.targetCCDuration < 500 && me->IsInCombat() && snap.dist <= 5)
                    cast = true;
                //case 2: evade casted spell
                if (!cast)
                {
                    if (Spell const* spell = snap.target->GetCurrentSpell(CURRENT_GENERIC_SPELL))
                    {
                        if (spell->m_targets.GetUnitTarget() == me && spell->GetTimer() < 500 &&
                            spell->GetSpellInfo()->HasEffect(SPELL_EFFECT_SCHOOL_DAMAGE))
                            cast = true;
                    }
                }
                //case 3: reset threat / evade in CheckVanish (regardless of mytar availability)
                if (cast && doCast(me, GetSpell(VANISH_1)))
                    return true; //custom: do not skip animation
            }

            return false;
        }

        //必须控制：仅在明确的控制需求下触发；PVE 单体常规不占用 Kidney Shot / Gouge
        bool TryMustControl(RogueSnapshot const& snap, uint32 diff)
        {
            Unit* mytar = snap.target;
            if (!snap.isDPS || snap.stealthed || mytar->IsPlayer() || mytar->IsTotem() ||
                me->HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY) || CCed(mytar))
                return false;

            bool const mustLock = mytar->IsNonMeleeSpellCast(false,false,true); //正在读条，需锁
            bool const markedCC = IsPointedNoDPSTarget(mytar);                  //团队标记的 CC 目标
            bool const fleeing = mytar->HasUnitState(UNIT_STATE_FLEEING);       //低血逃跑
            if (!mustLock && !markedCC && !fleeing)
                return false;

            //Kidney Shot：仅在真正的“必须锁”场景消耗 CP，且晕眩递减允许
            if (mustLock && !markedCC && GetSpell(KIDNEY_SHOT_1) && comboPoints >= 4 &&
                snap.stunDrv < DIMINISHING_LEVEL_3 && !mytar->IsImmunedToSpell(sSpellMgr->GetSpellInfo(KIDNEY_SHOT_1)) &&
                energy >= ecost(KIDNEY_SHOT_1))
            {
                if (doCast(mytar, GetSpell(KIDNEY_SHOT_1)))
                    return true;
            }

            //Gouge：逃跑 / 需锁目标时作为短控兜底
            if ((fleeing || mustLock) && IsSpellReady(GOUGE_1, diff) && snap.dist <= 5 &&
                energy >= ecost(GOUGE_1) && mytar->HasInArc(float(M_PI), me))
            {
                if (doCast(mytar, GetSpell(GOUGE_1)))
                    return true;
            }

            //Dismantle：缴械（物理控制手段），目标是持械的近战单位且未处于缴械状态
            if (IsSpellReady(DISMANTLE_1, diff) && snap.dist <= 5 && mytar->GetHealth() >= me->GetHealth() / 2 &&
                energy >= ecost(DISMANTLE_1) && !mytar->HasAuraType(SPELL_AURA_MOD_DISARM) &&
                (mytar->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID) && mytar->CanUseAttackType(BASE_ATTACK)))
            {
                if (doCast(mytar, GetSpell(DISMANTLE_1)))
                    return true;
            }

            return false;
        }

        //常规生存 / 机动（非紧急）
        bool TryDefensiveUtilities(RogueSnapshot const& snap, uint32 diff)
        {
            Unit* mytar = snap.target;

            //Evasion：固定阈值（血量 < 55% 且有怪在打我），不再按攻击者数量线性放大
            if (!snap.stealthed && snap.selfHealthPct < 55 && snap.hasAttackers &&
                IsSpellReady(EVASION_1, diff, false) &&
                !me->GetAuraEffect(SPELL_AURA_MOD_DODGE_PERCENT, SPELLFAMILY_ROGUE, 0x20, 0x0, 0x0))
            {
                if (doCast(me, GetSpell(EVASION_1)))
                    return true; //custom: do not skip animation
            }

            //Shadowstep：接近 / 脱离
            if (IsSpellReady(SHADOWSTEP_1, diff, false) && !snap.isTank && snap.isDPS && snap.dist < 25 &&
                energy >= ecost(SHADOWSTEP_1) && (!mytar->IsPlayer() || snap.dist > 12 || CCed(me, true)) &&
                (mytar->IsPlayer() || mytar->GetVictim() != me) &&
                ((!snap.stealthed && !snap.shadowDance) || me->HasAuraWithMechanic(1u<<MECHANIC_SNARE)))
            {
                if (doCast(mytar, GetSpell(SHADOWSTEP_1)))
                    getenergy();
            }

            //Sprint (no GCD)：被控脱身 / 远距离追击（行为抖动型随机保留）
            if (IsSpellReady(SPRINT_1, diff, false) && !HasBotCommandState(BOT_COMMAND_STAY) &&
                ((me->GetLevel() >= 20 && CCed(me, true) && RogueRoll(35)) ||
                (RogueRoll(uint32(25 + 10*snap.stealthed + 40*snap.shadowDance)) &&
                snap.dist > (20 - (5*snap.stealthed + 10*snap.shadowDance)))) &&
                !me->GetAuraEffect(SPELL_AURA_MOD_INCREASE_SPEED, SPELLFAMILY_ROGUE, 0x40, 0x0, 0x0))
            {
                doCast(me, GetSpell(SPRINT_1));
            }

            return false;
        }

        //远程手段：致死投掷（超出近战范围时）
        bool TryRangedAttack(RogueSnapshot const& snap, uint32 diff)
        {
            Unit* mytar = snap.target;
            if (snap.stealthed || snap.shadowDance || !snap.isDPS || comboPoints == 0)
                return false;
            if (!IsSpellReady(DEADLY_THROW_1, diff) || snap.dist >= 30 || energy < ecost(DEADLY_THROW_1))
                return false;
            if (_spec == BOT_SPEC_ROGUE_COMBAT && !mytar->IsNonMeleeSpellCast(false,false,true))
                return false;

            Item const* thrown = GetEquips(BOT_SLOT_RANGED);
            if (!thrown || thrown->GetTemplate()->Class != ITEM_CLASS_WEAPON ||
                thrown->GetTemplate()->SubClass != ITEM_SUBCLASS_WEAPON_THROWN)
                return false;

            return doCast(mytar, GetSpell(DEADLY_THROW_1));
        }

        //斗篷：debuff / 读条计数（原样保留 count > 1 判定）
        bool TryCloakOfShadows(uint32 diff)
        {
            if (!IsSpellReady(CLOAK_OF_SHADOWS_1, diff) || !me->IsInCombat() || me->IsMounted())
                return false;

            uint32 count = 0;

            //dispel debuffs
            uint32 const dispelMask = DISPEL_ALL_MASK;
            for (auto const& [_, auraApp] : me->GetAppliedAuras())
            {
                // remove all harmful spells on you...
                SpellInfo const* spellInfo = auraApp->GetBase()->GetSpellInfo();
                if ((spellInfo->DmgClass == SPELL_DAMAGE_CLASS_MAGIC || (spellInfo->GetDispelMask() & dispelMask)) &&
                    !auraApp->IsPositive() && !auraApp->GetBase()->IsPassive())
                {
                    if (spellInfo->HasAura(SPELL_AURA_PERIODIC_DAMAGE) ||
                        spellInfo->HasAura(SPELL_AURA_MOD_SPEED_SLOW_ALL) ||
                        spellInfo->HasAura(SPELL_AURA_HASTE_SPELLS))
                        if (++count > 1)
                            break;
                }
            }

            //defend from enemy cast
            if (Unit const* target = FindCastingTarget(50))
            {
                if (Spell const* spell = target->GetCurrentSpell(CURRENT_GENERIC_SPELL))
                {
                    if (spell->GetTimer() < 1000 && !spell->GetSpellInfo()->IsPassive() && !spell->GetSpellInfo()->IsPositive() &&
                        !(spell->GetSpellInfo()->Attributes & (SPELL_ATTR0_IS_ABILITY|SPELL_ATTR0_NO_IMMUNITIES)))
                    {
                        //direct spell
                        if (spell->m_targets.GetUnitTarget() == me &&
                            spell->GetSpellInfo()->DmgClass == SPELL_DAMAGE_CLASS_MAGIC &&
                            me->IsWithinLOSInMap(target, VMAP::ModelIgnoreFlags::M2, LINEOFSIGHT_ALL_CHECKS))
                        {
                            count += 2;
                        }
                        //area spell
                        if ((spell->GetSpellInfo()->Effects[0].IsEffect() &&
                            spell->GetSpellInfo()->Effects[0].TargetB.GetSelectionCategory() == TARGET_SELECT_CATEGORY_NEARBY) ||
                            (spell->GetSpellInfo()->Effects[1].IsEffect() &&
                            spell->GetSpellInfo()->Effects[1].TargetB.GetSelectionCategory() == TARGET_SELECT_CATEGORY_NEARBY))
                        {
                            count += 2;
                        }
                    }
                }
            }

            if (count <= 1)
                return false;

            return doCast(me, GetSpell(CLOAK_OF_SHADOWS_1));
        }

        //—— 输出段 ——
        //爆发段：自带 CP / CD 门控，不满足时让位给维持段
        bool TryBurstAbilities(RogueSnapshot const& snap, uint32 diff)
        {
            Unit* mytar = snap.target;
            if (!snap.isDPS)
                return false;

            //Blade Flurry：有溅射目标或目标较强时开启（去随机）
            if (IsSpellReady(BLADE_FLURRY_1, diff) && !snap.stealthed && !snap.shadowDance && snap.dist <= 5 &&
                energy >= ecost(BLADE_FLURRY_1) && !CCed(mytar) &&
                !me->GetAuraEffect(SPELL_AURA_MOD_MELEE_HASTE, SPELLFAMILY_ROGUE, 0x40000000, 0x800, 0x0) &&
                (mytar->IsPlayer() || mytar->GetHealth() > me->GetHealth() || FindSplashTarget(7, mytar)))
            {
                if (doCast(me, GetSpell(BLADE_FLURRY_1)))
                    return true;
            }

            //Killing Spree：仅在 CP 已攒够时施放（去随机）
            if (IsSpellReady(KILLING_SPREE_1, diff) && !snap.stealthed && !snap.shadowDance && snap.dist < 10 &&
                snap.selfHealthPct > 25 && (!CCed(mytar) || snap.dist > 5) && snap.comboPoints >= 4 &&
                (mytar->getAttackers().size() < 4 || mytar->GetMaxHealth() > me->GetMaxHealth() * 2) &&
                (mytar->IsControlledByPlayer() || mytar->GetHealth() > me->GetMaxHealth() / 2))
            {
                if (doCast(mytar, GetSpell(KILLING_SPREE_1)))
                    return true;
            }

            //Adrenaline Rush：能量低于 25 时开启（去随机）
            if (GetSpell(ADRENALINE_RUSH_1) && !snap.stealthed && !snap.shadowDance &&
                (snap.targetNormalStun || snap.targetCCDuration < 1300) && energy < 25 && snap.selfHealthPct > 35 &&
                (mytar->getAttackers().size() < 3 || mytar->GetMaxHealth() > me->GetMaxHealth() * 2) &&
                (mytar->GetHealth() > me->GetMaxHealth() / 2 || !me->getAttackers().empty()))
            {
                if (doCast(me, GetSpell(ADRENALINE_RUSH_1)))
                    return true;
            }

            //Shadow Dance：有能量或处于冲动期时开启（去随机）
            if (IsSpellReady(SHADOW_DANCE_1, diff, false) && !snap.stealthed && snap.selfHealthPct > 40 &&
                (snap.stunDrv == DIMINISHING_LEVEL_1 || CCed(mytar)) &&
                (energy >= 60 || (energy >= 40 && me->GetAuraEffect(SPELL_AURA_MOD_POWER_REGEN_PERCENT, SPELLFAMILY_ROGUE, 0x0, 0x80, 0x0))) &&
                (mytar->IsPlayer() || mytar->GetHealth() > (me->GetMaxHealth() / 4) * mytar->getAttackers().size()))
            {
                doCast(me, GetSpell(SHADOW_DANCE_1));
            }

            //Premeditation
            if (IsSpellReady(PREMEDITATION_1, diff, false) && (snap.stealthed || snap.shadowDance) &&
                comboPoints < 4 && snap.dist < 15 &&
                (comboPoints == 0 || mytar->GetHealth() > me->GetMaxHealth() / 4))
            {
                doCast(mytar, GetSpell(PREMEDITATION_1));
            }

            return false;
        }

        //维持段：HfB / SnD / 破甲 / Rupture / Hemo
        bool TryMaintainBuffs(RogueSnapshot const& snap, uint32 diff)
        {
            Unit* mytar = snap.target;
            if (!snap.isDPS || snap.stealthed || snap.shadowDance)
                return false;

            //Hunger for Blood：保 buff（需目标流血）
            if (IsSpellReady(HUNGER_FOR_BLOOD_1, diff) && snap.dist < 30 && !snap.hasHfB &&
                snap.targetBleeding && energy >= ecost(HUNGER_FOR_BLOOD_1))
            {
                if (doCast(mytar, GetSpell(HUNGER_FOR_BLOOD_1)))
                    return true;
            }

            //Slice and Dice：剩余 < 阈值且 CP >= 1 时才刷新，避免满时长白刷
            if (IsSpellReady(SLICE_DICE_1, diff) && comboPoints > 0 && energy >= ecost(SLICE_DICE_1) && snap.dist < 10 &&
                (!snap.hasSnD || snap.snDRemainMs < ROGUE_SND_REFRESH_MS) &&
                (me->getAttackers().size() <= 1 || !IsSpellReady(BLADE_FLURRY_1, diff)))
            {
                if (doCast(mytar, GetSpell(SLICE_DICE_1)))
                    return true;
            }

            //Expose Armor：团队缺破甲时补
            if (TryExposeArmor(snap, diff))
                return true;

            //Rupture：临近断档时补，且预计可完整跳完
            //门槛对齐旧版（存在流血加成的刺杀需至少 2 点，其余 1 点即可），
            //避免因强求满连击点而让割裂长时间断档
            uint8 const minRuptureCP = (snap.hasHfB || !GetSpell(HUNGER_FOR_BLOOD_1)) ? 2 : 1;
            if (GetSpell(RUPTURE_1) && comboPoints >= minRuptureCP && energy >= ecost(RUPTURE_1) &&
                (!snap.hasRupture || snap.ruptureRemainMs < ROGUE_RUPTURE_REFRESH_MS) &&
                mytar->CanHaveThreatList() && !mytar->IsPlayer() && snap.targetCCDuration < 2000 &&
                mytar->GetHealth() > me->GetMaxHealth() / 4 * (1 + mytar->getAttackers().size()) &&
                !(snap.hasHfB && GetSpec() == BOT_SPEC_ROGUE_ASSASINATION && GetSpell(ENVENOM_1)) &&
                (comboPoints < 4 || !GetSpell(KIDNEY_SHOT_1) || snap.stunDrv > DIMINISHING_LEVEL_2))
            {
                if (doCast(mytar, GetSpell(RUPTURE_1)))
                    return true;
            }

            //Hemorrhage：维持团队物理增益（不叠层，仅补 debuff）
            if (GetSpell(HEMORRHAGE_1) && !isdaggerMH && comboPoints < 5 && !snap.isTank &&
                energy >= ecost(HEMORRHAGE_1) &&
                !mytar->GetAuraEffect(SPELL_AURA_MOD_DAMAGE_TAKEN, SPELLFAMILY_ROGUE, 0x800000, 0x0, 0x0))
            {
                if (doCast(mytar, GetSpell(HEMORRHAGE_1)))
                    return true;
            }

            return false;
        }

        //Expose Armor：目标缺破甲且自身有 CP 时补一次
        bool TryExposeArmor(RogueSnapshot const& snap, uint32 diff)
        {
            Unit* mytar = snap.target;
            if (!GetSpell(EXPOSE_ARMOR_1) || comboPoints < 3 || !IsSpellReady(EXPOSE_ARMOR_1, diff) ||
                energy < ecost(EXPOSE_ARMOR_1) || mytar->IsPlayer() ||
                (mytar->CanHaveThreatList() && mytar->GetHealth() < me->GetMaxHealth() / 3))
                return false;

            //已有同槽位的破甲类 debuff 则不重复补
            //注意：不能按 SPELL_AURA_MOD_RESISTANCE_PCT（=101）/ SPELL_AURA_MOD_TARGET_RESISTANCE（=123）判断，
            //这两个光环类型被精灵之火以及目标自身的抗性类 buff 共用，
            //按类型判断会在大量正常场景下误判为“已有破甲”而永久不补
            if (mytar->GetAura(EXPOSE_ARMOR_1)) //破甲（含其他盗贼机器人施放的）
                return false;

            //战士破甲：需满 5 层才等效于破甲（同为 -20% 护甲），层数不足时补破甲更划算
            if (AuraEffect const* sunder = mytar->GetAuraEffect(SUNDER_ARMOR_DEBUFF, 0))
            {
                if (sunder->GetBase()->GetStackAmount() >= 5 && sunder->GetBase()->GetDuration() > 6000)
                    return false;
            }

            return doCast(mytar, GetSpell(EXPOSE_ARMOR_1));
        }

        //多目标段（仅 3+ 敌人）
        bool TryMultiTarget(RogueSnapshot const& snap, uint32 diff)
        {
            Unit* mytar = snap.target;
            if (!snap.isDPS || snap.stealthed || snap.shadowDance)
                return false;

            //Fan of Knives：能量不足时跳过（不阻断后续单体循环）
            if (GetSpell(FAN_OF_KNIVES_1) && !mytar->IsControlledByPlayer() &&
                energy >= ecost(FAN_OF_KNIVES_1))
            {
                if (doCast(me, GetSpell(FAN_OF_KNIVES_1)))
                    return true;
            }

            return false;
        }

        //清 CP 段
        bool TryFinishers(RogueSnapshot const& snap, uint32 diff)
        {
            Unit* mytar = snap.target;
            if (!snap.isDPS || comboPoints == 0 || snap.stealthed || snap.shadowDance)
                return false;

            //Envenom / Eviscerate：刺杀用 Envenom（需致命毒层数达标），其余用 Eviscerate
            uint32 finisher = 0;
            if (GetSpec() == BOT_SPEC_ROGUE_ASSASINATION && GetSpell(ENVENOM_1) &&
                snap.targetDeadlyStacks >= ROGUE_DEADLY_STACKS_ENOUGH)
                finisher = ENVENOM_1;
            else if (GetSpell(EVISCERATE_1))
                finisher = EVISCERATE_1;

            if (finisher && IsSpellReady(finisher, diff) && energy >= ecost(finisher) &&
                (comboPoints >= 4 || (snap.isExecutePhase && comboPoints >= 3)))
            {
                //Cold Blood：只配 5 CP 的终结技（去随机）
                if (IsSpellReady(COLD_BLOOD_1, diff, false) && comboPoints == 5)
                    doCast(me, GetSpell(COLD_BLOOD_1));

                if (doCast(mytar, GetSpell(finisher)))
                    return true;
            }

            return false;
        }

        //攒 CP 段
        bool TryBuilders(RogueSnapshot const& snap, uint32 diff)
        {
            Unit* mytar = snap.target;
            if (!snap.isDPS)
                return false;

            //潜行 / 暗影之舞起手
            if (snap.stealthed || snap.shadowDance)
                return TryOpeners(snap, diff);

            //Riposte：仅招架后
            if (IsSpellReady(RIPOSTE_1, diff) && comboPoints < 5 && me->HasReactive(REACTIVE_DEFENSE) &&
                energy >= ecost(RIPOSTE_1))
            {
                if (doCast(mytar, GetSpell(RIPOSTE_1)))
                    return true;
            }

            //Ghostly Strike：坦克模式
            if (IsSpellReady(GHOSTLY_STRIKE_1, diff) && comboPoints < 5 && snap.isTank && !me->getAttackers().empty() &&
                energy >= ecost(GHOSTLY_STRIKE_1))
            {
                if (doCast(mytar, GetSpell(GHOSTLY_STRIKE_1)))
                    return true;
            }

            //Sinister Strike
            if (GetSpell(SINISTER_STRIKE_1) && comboPoints < 5 &&
                (!isdaggerMH || snap.isTank || (mytar->GetVictim() == me && energy >= 60 && mytar->HasInArc(float(M_PI), me)) || !GetSpell(BACKSTAB_1)) &&
                energy >= ecost(SINISTER_STRIKE_1) + ROGUE_SS_ENERGY_MARGIN)
            {
                if (doCast(mytar, GetSpell(SINISTER_STRIKE_1)))
                    return true;
            }

            //Backstab / Mutilate
            uint32 mutistab =
                isdaggerMH && isdaggerOH && GetSpell(MUTILATE_1) ? MUTILATE_1 :
                isdaggerMH && GetSpell(BACKSTAB_1) ? BACKSTAB_1 : 0;
            if (mutistab && comboPoints < 4 && energy >= ecost(mutistab) &&
                (mutistab == MUTILATE_1 || !mytar->HasInArc(float(M_PI), me)))
            {
                if (doCast(mytar, GetSpell(mutistab)))
                    return true;
            }

            return false;
        }

        //潜行 / 暗影之舞起手技
        bool TryOpeners(RogueSnapshot const& snap, uint32 diff)
        {
            Unit* mytar = snap.target;

            //Sap：潜行开场闷棍（仅 PVE 单人 / 指定目标，暗影之舞不适用）
            if (TrySap(snap, diff))
                return true;

            uint32 opener =
                GetSpell(CHEAP_SHOT_1) &&
                !mytar->HasAuraType(SPELL_AURA_MOD_STUN) && snap.stunDrv < DIMINISHING_LEVEL_3 &&
                (mytar->IsPlayer() || (!IAmFree() && master->GetNpcBotsCount() > 1)) ? CHEAP_SHOT_1 :
                GetSpell(GARROTE_1) && mytar->GetHealth() > me->GetMaxHealth() / 4 &&
                !IsImmunedToMySpellEffect(mytar, sSpellMgr->GetSpellInfo(GARROTE_1), EFFECT_0) &&
                (!isdaggerMH || (mytar->IsPlayer() &&
                (mytar->GetClass() == CLASS_MAGE || mytar->GetClass() == CLASS_PRIEST || mytar->GetClass() == CLASS_WARLOCK))) &&
                !mytar->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_ROGUE, 0x100, 0x0, 0x0, me->GetGUID()) ? GARROTE_1 :
                GetSpell(AMBUSH_1) && isdaggerMH ? AMBUSH_1 :
                GetSpell(BACKSTAB_1) && isdaggerMH ? BACKSTAB_1 : 0;

            //all opener spells disabled/unusable
            if (!opener)
            {
                if (snap.stealthed && HasRole(BOT_ROLE_DPS))
                    me->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);
                //if (shadowdance)
                //    me->RemoveAurasByType(SPELL_AURA_MOD_IGNORE_SHAPESHIFT);
                return false;
            }
            //openers from behind (backstab too)
            if (opener != CHEAP_SHOT_1 && mytar->HasInArc(float(M_PI), me))
                return false;

            //We do not check combo points amount
            if (energy >= ecost(opener))
                return doCast(mytar, GetSpell(opener));

            return false;
        }

        //Sap：潜行起手闷棍
        //规则分档：
        //  PVP —— 允许常规起手闷棍（自主挑选一个可控的次要目标打破敌方阵型）
        //  PVE —— 仅对团队标记的“需要控制的目标”闷棍，不自主挑目标（避免浪费开场输出窗口）
        bool TrySap(RogueSnapshot const& snap, uint32 diff)
        {
            if (!snap.stealthed || snap.shadowDance || !GetSpell(SAP_1) || !IAmFree())
                return false;

            //仅在尚未进入战斗时起手闷棍，避免战斗中滥用
            if (me->IsInCombat())
                return false;

            bool const pvp = IsPvpContext();

            //团队标记的 CC 目标：PVE / PVP 均优先处理
            if (IsPointedNoDPSTarget(snap.target))
                return doCast(snap.target, GetSpell(SAP_1));

            //常规起手闷棍（自主选目标）仅 PVP 使用
            if (pvp)
            {
                Unit* sapTarget = FindSapTarget();
                if (sapTarget && sapTarget != snap.target)
                    return doCast(sapTarget, GetSpell(SAP_1));
            }

            return false;
        }

        //找一个可闷棍的目标（潜行开场用，仅人形 / 野兽 / 恶魔等非机械目标）
        //挑选最近的可控目标，并满足闷棍的正面朝向要求
        Unit* FindSapTarget() const
        {
            Unit* victim = me->GetVictim();
            std::list<Unit*> targets;
            GetNearbyTargetsList(targets, 20.f, 0);

            Unit* best = nullptr;
            float bestDist = 0.f;
            for (Unit* u : targets)
            {
                if (u == victim || u->IsPlayer() || u->IsTotem() || CCed(u) || u->HasAuraType(SPELL_AURA_MOD_STUN))
                    continue;
                if (u->GetCreatureType() != CREATURE_TYPE_HUMANOID &&
                    u->GetCreatureType() != CREATURE_TYPE_BEAST &&
                    u->GetCreatureType() != CREATURE_TYPE_DEMON &&
                    u->GetCreatureType() != CREATURE_TYPE_DRAGONKIN &&
                    u->GetCreatureType() != CREATURE_TYPE_UNDEAD)
                    continue;
                //闷棍的 FacingCasterFlags 为 SPELL_FACING_FLAG_INFRONT：必须站在目标正面
                //（与背刺 / 锁喉等“必须绕背”的起手技相反）
                if (!me->HasInArc(float(M_PI), u))
                    continue;

                float const dist = me->GetDistance(u);
                if (!best || dist < bestDist)
                {
                    best = u;
                    bestDist = dist;
                }
            }
            return best;
        }

        //刻意行为抖动型的随机数统一入口，便于集中调参
        bool RogueRoll(uint32 chancePct) const
        {
            return urand(1, 100) <= chancePct;
        }

        void BreakCC(uint32 diff) override
        {
            if (me->IsInCombat() && RogueRoll(25))
            {
                bool canVanish = IsSpellReady(VANISH_1, diff, false) && !IsFlagCarrier(me);
                bool canSprint = (GetSpec() == BOT_SPEC_ROGUE_COMBAT) && me->GetLevel() >= 25 && !HasBotCommandState(BOT_COMMAND_STAY) && IsSpellReady(SPRINT_1, diff, false);
                if ((canVanish || canSprint) && me->HasAuraWithMechanic((1u<<MECHANIC_SNARE)|(1u<<MECHANIC_ROOT)))
                {
                    uint32 Spanish = canSprint ? SPRINT_1 : VANISH_1;
                    if (doCast(me, GetSpell(Spanish)))
                        return;
                }
            }
            bot_ai::BreakCC(diff);
        }

        //仅战场 / 竞技场视为 PvP 涂毒场景；野外 PvP 仍按 PVE 方案涂毒，避免毒药在野外来回切换
        bool IsPvpContext() const
        {
            return me->GetMap()->IsBattlegroundOrArena();
        }

        //按涂毒方案表选择主 / 副手毒药（返回所选条目，未命中返回 nullptr）
        void ChoosePoisons(uint8 lvl, bool mhReady, bool ohReady)
        {
            bool const pvp = IsPvpContext();
            RoguePoisonPlan const* plan = nullptr;

            if (pvp)
            {
                for (size_t i = 0; i < ROGUE_POISON_PVP_COUNT && !plan; ++i)
                    plan = RoguePoisonPlanMatches(RoguePoisonPlansPVP[i], GetSpec(), lvl) ? &RoguePoisonPlansPVP[i] : nullptr;
            }
            else
            {
                for (size_t i = 0; i < ROGUE_POISON_PVE_COUNT && !plan; ++i)
                    plan = RoguePoisonPlanMatches(RoguePoisonPlansPVE[i], GetSpec(), lvl) ? &RoguePoisonPlansPVE[i] : nullptr;
            }

            if (needChooseMHEnchant && mhReady)
                mhEnchant = plan ? plan->mh : 0;
            if (needChooseOHEnchant && ohReady)
                ohEnchant = plan ? plan->oh : 0;
        }

        void DoNonCombatActions(uint32 diff)
        {
            if (GC_Timer > diff || me->IsMounted() || IsCasting() || !RogueRoll(25))
                return;

            if (mhEnchantExpireTimer > 0 && mhEnchantExpireTimer <= diff)
                RemoveItemClassEnchantment(BOT_SLOT_MAINHAND);
            if (ohEnchantExpireTimer > 0 && ohEnchantExpireTimer <= diff)
                RemoveItemClassEnchantment(BOT_SLOT_OFFHAND);

            // Weapon Enchants
            if (me->isMoving())
                return;
            uint8 lvl = me->GetLevel();
            if (lvl < 20)
                return;

            Item* mhWeapon = GetEquips(BOT_SLOT_MAINHAND);
            Item* ohWeapon = GetEquips(BOT_SLOT_OFFHAND);

            bool mhReady = mhWeapon && !mhWeapon->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT);
            bool ohReady = ohWeapon && !ohWeapon->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT);
            if (!mhReady && !ohReady)
                return; //no ecnhantable weapons

            //按描述表选择毒药（PVP 场景使用致残 / 脑麻痹）
            ChoosePoisons(lvl, mhReady, ohReady);

            uint32 MhPoison = !mhReady ? 0 : GetSpell(mhEnchant);
            uint32 OhPoison = !ohReady ? 0 : GetSpell(ohEnchant);

            SpellInfo const* MhPoisonInfo = mhReady && MhPoison ? sSpellMgr->GetSpellInfo(MhPoison) : nullptr;
            SpellInfo const* OhPoisonInfo = ohReady && OhPoison ? sSpellMgr->GetSpellInfo(OhPoison) : nullptr;

            Item* targetWeapon = nullptr;
            SpellInfo const* targetInfo = nullptr;

            if (mhReady && MhPoison && mhWeapon->IsFitToSpellRequirements(MhPoisonInfo))
            {
                targetWeapon = mhWeapon;
                targetInfo = MhPoisonInfo;
            }
            if (!targetWeapon && ohReady && OhPoison && ohWeapon->IsFitToSpellRequirements(OhPoisonInfo))
            {
                targetWeapon = ohWeapon;
                targetInfo = OhPoisonInfo;
            }
            if (targetWeapon)
            {
                Spell* spell = new Spell(me, targetInfo, TRIGGERED_NONE);
                SpellCastTargets targets;
                targets.SetItemTarget(targetWeapon);
                spell->prepare(&targets);
                return;
            }
        }

        //消失兜底：无输出目标时仍需能靠消失脱离（Attack() 无目标会直接 return）
        //其余消失 / 斗篷逻辑已并入 Attack() 的生存段，此处不重复触发
        void CheckVanishFallback(uint32 diff)
        {
            if (!IsSpellReady(VANISH_1, diff, false) || !me->IsInCombat() || me->IsMounted() || IsTank() ||
                me->getAttackers().empty() || IsFlagCarrier(me) || me->HasAuraType(SPELL_AURA_MOD_STEALTH) ||
                me->HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY) || me->HasAuraType(SPELL_AURA_PERIODIC_DAMAGE))
                return;

            //固定阈值：血量过低且正在被攻击时脱离
            if (GetHealthPCT(me) < 35)
                doCast(me, GetSpell(VANISH_1));
        }

        void CheckBlind(uint32 diff)
        {
            if (!IsSpellReady(BLIND_1, diff) || !me->IsInCombat() || me->IsMounted() || IsTank() || !RogueRoll(60) ||
                me->HasAuraType(SPELL_AURA_MOD_STEALTH) || me->HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY) ||
                IsSpellReady(BLADE_FLURRY_1, diff, false) || IsSpellReady(EVASION_1, diff, false) ||
                me->GetAuraEffect(SPELL_AURA_MOD_DODGE_PERCENT, SPELLFAMILY_ROGUE, 0x20, 0x0, 0x0) ||//evasion
                me->GetAuraEffect(SPELL_AURA_MOD_MELEE_HASTE, SPELLFAMILY_ROGUE, 0x40000000, 0x800, 0x0))
                return;

            Unit* u = FindStunTarget(15); //improved always (base 10, improved 15)
            if (!u)
                u = FindCastingTarget(15, 0, BLIND_1);

            if (u && doCast(u, GetSpell(BLIND_1)))
                return;
        }

        void CheckPreparation(uint32 diff)
        {
            if (!IsSpellReady(PREPARATION_1, diff) || !me->IsInCombat() || me->IsMounted() || !RogueRoll(70))
                return;

            //TODO: recheck priorities
            uint32 needFactor = 0;
            uint32 cooldown;
            cooldown = GetSpellCooldown(EVASION_1);
            needFactor += !cooldown ? 0 : 3 * cooldown / 1200; //1-100 x3
            cooldown = GetSpellCooldown(SPRINT_1);
            needFactor += !cooldown ? 0 : 1 * cooldown / 1200; //1-100
            cooldown = GetSpellCooldown(VANISH_1);
            needFactor += !cooldown ? 0 : 3 * cooldown / 1200; //1-100 x3
            cooldown = GetSpellCooldown(COLD_BLOOD_1);
            needFactor += !cooldown ? 0 : 2 * cooldown / 1800; //1-100 x2
            cooldown = GetSpellCooldown(SHADOWSTEP_1);
            needFactor += !cooldown ? 0 : 2 * cooldown / 200;  //1-100 x2
            cooldown = GetSpellCooldown(BLADE_FLURRY_1);
            needFactor += !cooldown ? 0 : 1 * cooldown / 1200; //1-100
            cooldown = GetSpellCooldown(DISMANTLE_1);
            needFactor += !cooldown ? 0 : 1 * cooldown / 600;  //1-100
            //0-1300
            //ignore Kick

            if (needFactor >= 800 && doCast(me, GetSpell(PREPARATION_1)))
                return;
        }

        void CheckTricksOfTheTrade(uint32 diff)
        {
            if (!IsSpellReady(TRICKS_OF_THE_TRADE_1, diff) || !me->IsInCombat() || me->IsMounted() || IAmFree() ||
                IsTank() || !RogueRoll(70) || !me->GetMap()->IsDungeon() ||
                me->HasAuraType(SPELL_AURA_MOD_STEALTH) || me->HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY))
                return;

            Group const* group = master->GetGroup();
            if (!group)
                return;
            Unit* victim = me->GetVictim();
            if (!victim)
                return;

            Unit* target = nullptr;
            for (GroupReference const* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* player = itr->GetSource();
                if (!player || !player->IsInWorld() || !player->IsAlive() || me->GetMap() != player->FindMap() ||
                    me->GetDistance(player) > 20)
                    continue;

                if (IsTank(player) || player->GetVictim() == victim)
                {
                    if (!victim->CanHaveThreatList() ||
                        victim->GetThreatMgr().GetThreat(player) < victim->GetThreatMgr().GetThreat(me) * 0.75f)
                    {
                        target = player;
                        break;
                    }
                }
            }

            if (target)
                if (doCast(target, GetSpell(TRICKS_OF_THE_TRADE_1)))
                    return;
        }

        void CheckSprint(uint32 diff)
        {
            if (!IsSpellReady(SPRINT_1, diff) || (!IAmFree() && !HasBotCommandState(BOT_COMMAND_FOLLOW)) || !RogueRoll(65) || me->IsMounted())
                return;

            if (IAmFree())
            {
                InstanceTemplate const* instt = sObjectMgr->GetInstanceTemplate(me->GetMap()->GetId());
                bool map_allows_mount = (!me->GetMap()->IsDungeon() || me->GetMap()->IsBattlegroundOrArena()) && (!instt || instt->AllowMount);
                if (me->HasUnitMovementFlag(MOVEMENTFLAG_FORWARD) &&
                    (!me->GetVictim() ? (!map_allows_mount || me->IsInCombat() || IsFlagCarrier(me)) : !me->IsWithinDist(me->GetVictim(), 8.0f)))
                {
                    if (doCast(me, GetSpell(SPRINT_1)))
                        return;
                }

                return;
            }

            if (me->GetExactDist2d(master) > std::max<uint8>(master->GetBotMgr()->GetBotFollowDist(), 45))
            {
                if (doCast(me, GetSpell(SPRINT_1)))
                    return;
            }
        }

        void ApplyClassSpellCritMultiplierAll(Unit const* /*victim*/, float& crit_chance, SpellInfo const* spellInfo, SpellSchoolMask /*schoolMask*/, WeaponAttackType /*attackType*/) const override
        {
            if (spellInfo->DmgClass != SPELL_DAMAGE_CLASS_MELEE)
                return;

            //uint32 spellId = spellInfo->Id;
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            uint8 lvl = me->GetLevel();

            //Cold Blood
            if (AuraEffect const* bloo = me->GetAuraEffect(COLD_BLOOD_1, 0, me->GetGUID()))
                if (bloo->IsAffectedOnSpell(spellInfo))
                    crit_chance += 100.f;

            //Puncturing Wounds:
            if (lvl >= 15)
            {
                //30% additional critical chance for Backstab
                if (baseId == BACKSTAB_1)
                    crit_chance += 30.f;
                //Puncturing Wounds: 15% additional critical chance for Mutilate
                else if (baseId == MUTILATE_1 ||
                    baseId == MUTILATE_DAMAGE_MAINHAND_1 || baseId == MUTILATE_DAMAGE_OFFHAND_1)
                    crit_chance += 15.f;
            }
            //Glyph of Eviscerate: 10% additional critical chance for Eviscerate
            if (lvl >= 15 && baseId == EVISCERATE_1)
                crit_chance += 10.f;
            //Improved Ambush: 50% additional critical chance for Ambush
            if ((GetSpec() == BOT_SPEC_ROGUE_SUBTLETY) && lvl >= 25 && baseId == AMBUSH_1)
                crit_chance += 50.f;
            //Turn the Tables:
            if (lvl >= 50 &&
                ((spellInfo->SpellFamilyFlags[0] & 0x2600070E) || (spellInfo->SpellFamilyFlags[1] & 0x7900106)) &&
                me->GetAuraEffect(SPELL_AURA_ADD_FLAT_MODIFIER, SPELLFAMILY_ROGUE, 0x0, 0x200000, 0x0))
                crit_chance += 6.f;
            //Remorseless Attacks:
            if (AuraEffect const* remo = me->GetAuraEffect(REMORSELESS_ATTACKS_BUFF, 0, me->GetGUID()))
                if (remo->IsAffectedOnSpell(spellInfo))
                    crit_chance += 40.f;
        }

        void ApplyClassDamageMultiplierMeleeSpell(int32& damage, SpellNonMeleeDamage& /*damageinfo*/, SpellInfo const* spellInfo, WeaponAttackType /*attackType*/, bool iscrit) const override
        {
            //uint32 spellId = spellInfo->Id;
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            uint8 lvl = me->GetLevel();
            float fdamage = float(damage);

            //apply bonus damage mods
            float pctbonus = 0.0f;
            if (iscrit)
            {
                //!!!Melee spell damage is not yet critical, all reduced by half
                //Lethality: 30% crit damage bonus for non-stealth combo-generating abilities (on 25 lvl)
                if (lvl >= 25 &&
                    (baseId == SINISTER_STRIKE_1 || baseId == BACKSTAB_1 || baseId == MUTILATE_1 || baseId == RIPOSTE_1 ||
                    baseId == GOUGE_1 || baseId == HEMORRHAGE_1 || baseId == GHOSTLY_STRIKE_1
                    /*|| baseId == SHIV_1*/ || baseId == MUTILATE_DAMAGE_MAINHAND_1 || baseId == MUTILATE_DAMAGE_OFFHAND_1))
                    pctbonus += 0.15f;
            }

            //DeathDealer set bonus: 15% damage bonus for Eviscerate
            if (lvl >= 60 && baseId == EVISCERATE_1)
                pctbonus += 0.15f;
            //Find Weakness: 6% bonus damage to all abilities
            if ((GetSpec() == BOT_SPEC_ROGUE_ASSASINATION) && lvl >= 45)
                pctbonus += 0.06f;
            //Improved Eviscerate: 20% damage bonus for Eviscerate
            if (lvl >= 10 && baseId == EVISCERATE_1)
                pctbonus += 0.2f;
            //Opportunity: 20% damage bonus for Backstab, Mutilate, Garrote and Ambush
            if (lvl >= 10 &&
                (baseId == BACKSTAB_1 || baseId == MUTILATE_1 || baseId == MUTILATE_DAMAGE_MAINHAND_1 ||
                baseId == MUTILATE_DAMAGE_OFFHAND_1 || baseId == GARROTE_1 || baseId == AMBUSH_1))
                pctbonus += 0.2f;
            //Aggression: 15% damage bonus for Sinister Strike, Backstab and Eviscerate
            if ((GetSpec() == BOT_SPEC_ROGUE_COMBAT) &&
                lvl >= 25 && (baseId == SINISTER_STRIKE_1 || baseId == BACKSTAB_1 || baseId == EVISCERATE_1))
                pctbonus += 0.15f;
            //Blood Spatter: 30% bonus damage for Rupture and Garrote
            if (lvl >= 15 && (baseId == RUPTURE_1 || baseId == GARROTE_1))
                pctbonus += 0.3f;
            //Vile Poisons: 20% damage bonus for Poisons and Envenom
            if (lvl >= 25 && ((spellInfo->SpellFamilyFlags[0] & 0x10012000) || (spellInfo->SpellFamilyFlags[1] & 0x18)))
                pctbonus += 0.2f;
            //Serrated Blades part 2: 30% bonus damage for Rupture
            if (lvl >= 20 && baseId == RUPTURE_1)
                pctbonus += 0.3f;
            //Surprise Attacks: 10% bonus damage for Sinister Strike, Backstab, Shiv, Hemmorhage and Gouge
            if ((GetSpec() == BOT_SPEC_ROGUE_COMBAT) &&
                lvl >= 50 && (baseId == SINISTER_STRIKE_1 || baseId == BACKSTAB_1 ||
                /*baseId == SHIV_1 || */baseId == HEMORRHAGE_1 || baseId == GOUGE_1))
                pctbonus += 0.1f;
            //Blade Twisting: 10% bonus damage for Sinister Strike and Backstab
            if ((GetSpec() == BOT_SPEC_ROGUE_COMBAT) && lvl >= 35 && (baseId == SINISTER_STRIKE_1 || baseId == BACKSTAB_1))
                pctbonus += 0.1f;
            //Sinister Calling: 10% bonus percentage damage for Backstab and Hemorrhage
            //We add bonus damage pct because SpellMods are not handled
            if ((GetSpec() == BOT_SPEC_ROGUE_SUBTLETY) && lvl >= 45 && (baseId == BACKSTAB_1 || baseId == HEMORRHAGE_1))
                pctbonus += 0.1f;
            //Glyph of Fan of Knives: 20% bonus damage for Fan of Knives
            if (lvl >= 80 && baseId == FAN_OF_KNIVES_1)
                pctbonus += 0.2f;

            //Glyph of Sinister Strike: 50% chance to add 1 cp on crit
            if (baseId == SINISTER_STRIKE_1)
                glyphSSProc = iscrit && lvl >= 15 && urand(1,100) <= GLYPH_SS_PROC_PCT;

            damage = int32(fdamage * (1.0f + pctbonus));
        }

        void ApplyClassSpellCostMods(SpellInfo const* spellInfo, int32& cost) const override
        {
            //uint32 spellId = spellInfo->Id;
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            uint8 lvl = me->GetLevel();
            float fcost = float(cost);
            int32 flatbonus = 0;
            float pctbonus = 0.0f;

            //100% mods
            //Glyph of Blade Flurry: -100% cost for Blade Flurry
            if (lvl >= 30 && baseId == BLADE_FLURRY_1)
                pctbonus += 1.0f;

            //percent mods
            //Dirty Tricks: -50% cost for Blind and Sap
            if (lvl >= 15 && (baseId == BLIND_1 || baseId == SAP_1))
                pctbonus += 0.5f;

            //flat mods
            //Improved Expose Armor: -10 energy cost for Expose Armor
            if (lvl >= 20 && baseId == EXPOSE_ARMOR_1)
                flatbonus += 10;
            //Improved Sinister Strike: -5 energy cost for Sinister Strike
            if (lvl >= 10 && baseId == SINISTER_STRIKE_1)
                flatbonus += 5;
            //Dirty Deeds part 1: -20 energy cost for Cheap Shot and Garrote
            if ((GetSpec() == BOT_SPEC_ROGUE_SUBTLETY) && lvl >= 30 && (baseId == CHEAP_SHOT_1 || baseId == GARROTE_1))
                flatbonus += 20;
            //Filthy Tricks part 2: -10 energy cost for Tricks of the Trade, Distract and Shadowstep
            if ((GetSpec() == BOT_SPEC_ROGUE_SUBTLETY) &&
                lvl >= 50 && (baseId == TRICKS_OF_THE_TRADE_1 || baseId == DISTRACT_1 || baseId == SHADOWSTEP_1))
                flatbonus += 10;
            //Slaugher from the Shadows part 1: -20 energy cost for Backstab and Ambush
            if ((GetSpec() == BOT_SPEC_ROGUE_SUBTLETY) && lvl >= 55 && (baseId == BACKSTAB_1 || baseId == AMBUSH_1))
                flatbonus += 20;
            //Slaugher from the Shadows part 2: -5 energy cost for Hemorrhage
            if ((GetSpec() == BOT_SPEC_ROGUE_SUBTLETY) && lvl >= 55 && baseId == HEMORRHAGE_1)
                flatbonus += 5;
            //Glyph of Feint: -20 energy cost for Feint
            if (lvl >= 16 && baseId == FEINT_1)
                flatbonus += 20;
            //Glyph of Gouge: -15 energy cost for Gouge
            if (lvl >= 15 && baseId == GOUGE_1)
                flatbonus += 15;
            //Glyph of Mutilate: -5 energy cost for Mutilate
            if (lvl >= 50 && baseId == MUTILATE_1)
                flatbonus += 5;

            //cost can be < 0
            cost = int32(fcost * (1.0f - pctbonus)) - flatbonus;
        }

        void ApplyClassSpellCastTimeMods(SpellInfo const* spellInfo, int32& casttime) const override
        {
            //casttime is in milliseconds
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            uint8 lvl = me->GetLevel();
            int32 timebonus = 0;
            //float pctbonus = 0.0f;

            //100% mods
            //Improved Slam: -100% cast time for Slam
            //if (lvl >= 40 && spellId == GetSpell(SLAM_1) && me->HasAura(BLOODSURGE_BUFF))
            //    timebonus += casttime;

            //flat mods
            //Glyph of Pick Lock: 100% cast time for Pick Lock (reduced for bots)
            if (lvl >= 16 && baseId == PICK_LOCK_1)
                timebonus += 4000;

            casttime = std::max<int32>(casttime - timebonus, 0);
        }

        void ApplyClassSpellCooldownMods(SpellInfo const* spellInfo, uint32& cooldown) const override
        {
            //cooldown is in milliseconds
            //uint32 spellId = spellInfo->Id;
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            uint8 lvl = me->GetLevel();
            int32 timebonus = 0;
            float pctbonus = 0.0f;

            //pct mods
            //Intensify Rage: -33% cooldown for Bloodrage, Berserker Rage, Recklessness and Death Wish
            //if (lvl >= 40 &&
            //    (spellId == GetSpell(BLOODRAGE_1) || spellId == GetSpell(BERSERKERRAGE_1) ||
            //    spellId == GetSpell(RECKLESSNESS_1) || spellId == GetSpell(DEATHWISH_1)))
            //    pctbonus += 0.33f;

            //flat mods
            //Elusiveness part 2: -60 sec cooldown for Blind
            if (lvl >= 20 && baseId == BLIND_1)
                timebonus += 60000;
            //Elusiveness part 3: -30 sec cooldown for Cloak of Shadows
            if (lvl >= 20 && baseId == CLOAK_OF_SHADOWS_1)
                timebonus += 30000;
            //Filthy Tricks part 1: -10 sec cooldown for Tricks of the Trade, Distract and Shadowstep
            if ((GetSpec() == BOT_SPEC_ROGUE_SUBTLETY) &&
                lvl >= 50 && (baseId == TRICKS_OF_THE_TRADE_1 || baseId == DISTRACT_1 || baseId == SHADOWSTEP_1))
                timebonus += 10000;
            //Filthy Tricks part 3: -3 min cooldown for Preparation
            if ((GetSpec() == BOT_SPEC_ROGUE_SUBTLETY) && lvl >= 50 && baseId == PREPARATION_1)
                timebonus += 180000;
            //Glyph of Killing Spree: -45 sec cooldown for Killing Spree
            if (lvl >= 60 && baseId == KILLING_SPREE_1)
                timebonus += 45000;

            cooldown = std::max<int32>((float(cooldown) * (1.0f - pctbonus)) - timebonus, 0);
        }

        void ApplyClassSpellCategoryCooldownMods(SpellInfo const* spellInfo, uint32& cooldown) const override
        {
            //cooldown is in milliseconds
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            uint8 lvl = me->GetLevel();
            int32 timebonus = 0;
            float pctbonus = 0.0f;

            //Endurance: -1 min cooldown for Sprint and Evasion
            if (lvl >= 20 && (baseId == SPRINT_1 || baseId == EVASION_1))
                timebonus += 60000;
            //Elusiveness part 1: -60 sec cooldown for Vanish
            if (lvl >= 20 && baseId == VANISH_1)
                timebonus += 60000;
            //Camouflage part 2: -6 sec cooldown for Stealth
            if (lvl >= 15 && baseId == STEALTH_1)
                timebonus += 6000;

            cooldown = std::max<int32>((float(cooldown) * (1.0f - pctbonus)) - timebonus, 0);
        }

        void ApplyClassSpellGlobalCooldownMods(SpellInfo const* /*spellInfo*/, float& cooldown) const override
        {
            //cooldown is in milliseconds
            //uint32 spellId = spellInfo->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            //uint8 lvl = me->GetLevel();
            float timebonus = 0.0f;
            float pctbonus = 0.0f;

            //Unrelenting Assault (part 1, special): -0.5 sec global cooldown for Overpower and Revenge (not for tanks)
            //if (lvl >= 50 && !IsTank() && (spellId == GetSpell(OVERPOWER_1) || spellId == GetSpell(REVENGE_1)))
            //    timebonus += 500.f;

            cooldown = (cooldown * (1.0f - pctbonus)) - timebonus;
        }

        void ApplyClassSpellRadiusMods(SpellInfo const* /*spellInfo*/, float& radius) const override
        {
            //uint32 spellId = spellInfo->Id;
            //uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            //uint8 lvl = me->GetLevel();
            float flatbonus = 0.0f;
            float pctbonus = 0.0f;

            //pct mods
            //Booming Voice
            //if (lvl >= 10 && ((spellInfo->SpellFamilyFlags[0] & 0x30000) || (spellInfo->SpellFamilyFlags[1] & 0x80)))
            //    pctbonus += 1.0f;

            //flat mods
            //Glyph of Thunder Clap
            //if (spellInfo->SpellFamilyFlags[0] & 0x80)
            //    flatbonus += 4.f;

            radius = radius * (1.0f + pctbonus) + flatbonus;
        }

        void ApplyClassSpellRangeMods(SpellInfo const* spellInfo, float& maxrange) const override
        {
            //uint32 spellId = spellInfo->Id;
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            uint8 lvl = me->GetLevel();
            float flatbonus = 0.0f;
            float pctbonus = 0.0f;

            //pct mods
            //Booming Voice
            //if (lvl >= 10 && ((spellInfo->SpellFamilyFlags[0] & 0x30000) || (spellInfo->SpellFamilyFlags[1] & 0x80)))
            //    pctbonus += 1.0f;

            //flat mods
            //Throwing Specialization: + 4 yd range for Deadly Throw
            if ((GetSpec() == BOT_SPEC_ROGUE_COMBAT) && lvl >= 45 && baseId == DEADLY_THROW_1)
                flatbonus += 4.f;
            //Dirty Tricks: + 5 yd range for Blind and Sap
            if (lvl >= 15 && (baseId == BLIND_1 || baseId == SAP_1))
                flatbonus += 5.f;
            //Glyph of Ambush: + 5 yd range for Ambush
            if (/*lvl >= 18 && */baseId == AMBUSH_1)
                flatbonus += 5.f;
            if (baseId == DISARM_TRAP_1)
                flatbonus += 10.f;

            maxrange = maxrange * (1.0f + pctbonus) + flatbonus;
        }

        void ApplyClassSpellMaxTargetsMods(SpellInfo const* /*spellInfo*/, uint32& targets) const override
        {
            uint32 bonusTargets = 0;

            //Improved Revenge: +1 target (actually 2 in dbc)
            //if (spellInfo->SpellFamilyFlags[0] & 0x400)
            //    bonusTargets += 1;

            targets = targets + bonusTargets;
        }

        void OnClassSpellGo(SpellInfo const* spellInfo) override
        {
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            uint8 lvl = me->GetLevel();

            //Thistle Tea: cooldown
            if (baseId == THISTLE_TEA)
                SetSpellCooldown(THISTLE_TEA, 300000); //5 min (item cd)

            //Remorseless Attacks: proc consume buff
            if (AuraEffect const* remo = me->GetAuraEffect(REMORSELESS_ATTACKS_BUFF, 0, me->GetGUID()))
                if (remo->IsAffectedOnSpell(spellInfo))
                    me->RemoveAurasDueToSpell(REMORSELESS_ATTACKS_BUFF);

            //Relentless Strikes
            if (spellInfo->NeedsComboPoints() && comboPoints)
            {
                if (lvl >= 10)
                {
                    if (irand(1,100) <= int32(RELENTLESS_RS_PCT_PER_CP * comboPoints))
                    {
                        me->CastSpell(me, RELENTLESS_STRIKES_EFFECT, true);
                        //BOT_LOG_ERROR("entities.player", "rogue_bot CP SPEND1: RS proc!");
                    }
                }
            }

            //Item enchant
            //We don't know which item is targeted
            //Actually it is mh, then oh
            if (baseId == CRIPPLING_POISON_1 || baseId == INSTANT_POISON_1 || baseId == DEADLY_POISON_1 ||
                baseId == WOUND_POISON_1 || baseId == ANESTHETIC_POISON_1 || baseId == MIND_NUMBING_POISON_1)
            {
                //We set duration to 2 seconds to prevent exploiting unequip mechanic
                //to get enchanted weapons for player (for non-shaman bots it won't work)
                uint32 slot = TEMP_ENCHANTMENT_SLOT;
                uint32 duration = 2 * IN_MILLISECONDS;
                uint32 charges = 0;
                uint32 enchant_id = spellInfo->Effects[0].MiscValue;
                //SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchant_id);
                Item* mh = GetEquips(BOT_SLOT_MAINHAND);
                Item* oh = GetEquips(BOT_SLOT_OFFHAND);
                Item* item = nullptr;
                uint8 itemSlot = 0;

                if (mh && !mh->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT)/* && mh->IsFitToSpellRequirements(spellInfo)*/)
                {
                    item = mh;
                    itemSlot = BOT_SLOT_MAINHAND;
                }
                else if (oh && !oh->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT)/* && oh->IsFitToSpellRequirements(spellInfo)*/)
                {
                    item = oh;
                    itemSlot = BOT_SLOT_OFFHAND;
                }
                else
                    ASSERT(false, "rogue bot attempted to enchant his weapons but cannot find a weapon to apply it!");

                if (!IAmFree())
                    master->GetSession()->SendEnchantmentLog(me->GetGUID(), me->GetGUID(), item->GetEntry(), enchant_id);

                item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_ID_OFFSET, enchant_id);
                item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_DURATION_OFFSET, duration);
                item->SetUInt32Value(ITEM_FIELD_ENCHANTMENT_1_1 + slot*MAX_ENCHANTMENT_OFFSET + ENCHANTMENT_CHARGES_OFFSET, charges);
                ApplyItemEnchantment(item, TEMP_ENCHANTMENT_SLOT, itemSlot);
                if (itemSlot == BOT_SLOT_MAINHAND)
                    mhEnchantExpireTimer = ITEM_ENCHANTMENT_EXPIRE_TIMER;
                else if (itemSlot == BOT_SLOT_OFFHAND)
                    ohEnchantExpireTimer = ITEM_ENCHANTMENT_EXPIRE_TIMER;
                //GC_Timer = 1500; //needed
            }
        }

        void SpellHit(Unit* wcaster, SpellInfo const* spell) override
        {
            Unit* caster = wcaster->ToUnit();
            if (!caster)
                return;

            uint32 baseId = spell->GetFirstRankSpell()->Id;

            //Vanish: handle stealth add
            if (baseId == VANISH_TRIGGERED_1 || baseId == VANISH_TRIGGERED_2 || baseId == VANISH_TRIGGERED_3)
            {
                if (!me->GetAuraEffect(SPELL_AURA_MOD_SHAPESHIFT, SPELLFAMILY_ROGUE, 0x400000, 0x0, 0x0))
                {
                    //SetSpellCooldown(STEALTH_1, 0);
                    me->CastSpell(me, STEALTH_1, true);
                }
            }
            //Cheat Death: assume resilience bonus
            if (baseId == CHEATING_DEATH_BUFF)
            {
                if (AuraEffect* chea = me->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_GENERIC, 2109, 0))
                {
                    chea->ChangeAmount(-100);
                }
            }
            //Camouflage part 1: +15% speed while stealthed
            if (baseId == STEALTH_1 && me->GetLevel() >= 15)
            {
                if (AuraEffect* stea = me->GetAuraEffect(spell->Id, 2))
                {
                    stea->ChangeAmount(stea->GetAmount() + 15);
                }
            }

            OnSpellHit(caster, spell);
        }

        void SpellHitTarget(Unit* wtarget, SpellInfo const* spell) override
        {
            Unit* target = wtarget->ToUnit();
            if (!target)
                return;

            uint32 spellId = spell->Id;
            uint32 baseId = spell->GetFirstRankSpell()->Id;
            uint8 lvl = me->GetLevel();

            //Cold Blood: handle proc
            if (AuraEffect const* bloo = me->GetAuraEffect(COLD_BLOOD_1, 0, me->GetGUID()))
                if (bloo->IsAffectedOnSpell(spell))
                    me->RemoveAurasDueToSpell(COLD_BLOOD_1);

            //Combo point generating from effects
            if (baseId == SEAL_FATE_EFFECT || baseId == RUTHLESSNESS_EFFECT ||
                baseId == SETUP_EFFECT || baseId == INITIATIVE_EFFECT || baseId == HONOR_AMONG_THIEVES_EFFECT)
            {
                ++comboPoints;
                //BOT_LOG_ERROR("entities.player", "rogue_bot CP GEN2: %s adds 1, now %u", spell->SpellName[0], uint32(comboPoints));
                if (comboPoints > 5)
                {
                    comboPoints = 5;
                    //BOT_LOG_ERROR("entities.player", "rogue_bot CP NOR2: now %u", uint32(comboPoints));
                }
            }
            //Combo point generating from spells
            if (baseId == SINISTER_STRIKE_1 || baseId == BACKSTAB_1 || baseId == MUTILATE_1 ||
                baseId == GOUGE_1 || baseId == HEMORRHAGE_1 || baseId == GHOSTLY_STRIKE_1 ||
                baseId == RIPOSTE_1 || baseId == PREMEDITATION_1 ||
                baseId == AMBUSH_1 || baseId == GARROTE_1 || baseId == CHEAP_SHOT_1/* || baseId == SHIV_1*/)
            {
                (baseId == MUTILATE_1 || baseId == PREMEDITATION_1 || baseId == CHEAP_SHOT_1) ?
                    comboPoints += 2 : ++comboPoints;

                //BOT_LOG_ERROR("entities.player", "rogue_bot CP GEN1: %s adds %u, now %u",
                //    spell->SpellName[0], (baseId == MUTILATE_1 || baseId == PREMEDITATION_1 || baseId == CHEAP_SHOT_1) ?
                //    2 : 1, uint32(comboPoints));

                //Glyph of Sinister Strike: handle proc
                if (baseId == SINISTER_STRIKE_1 && glyphSSProc)
                {
                    ++comboPoints;
                    //BOT_LOG_ERROR("entities.player", "rogue_bot CP GEN1: glyphSS proc, now %u", uint32(comboPoints));
                }

                if (comboPoints > 5)
                {
                    comboPoints = 5;
                    //BOT_LOG_ERROR("entities.player", "rogue_bot CP NOR1: now %u", uint32(comboPoints));
                }
            }
            //if (spellId == EVISCERATE || spellId == KIDNEY_SHOT || spellId == SLICE_DICE || spellId == RUPTURE || spellId == EXPOSE_ARMOR || spellId == ENVENOM)
            //some abilities like relentless strikes require combo points thus tries to proc itself
            else if (spell->NeedsComboPoints() && comboPoints)
            {
                //uint32 tempCP = comboPoints;
                //comboPoints = 0;
                combopointsSpent = true; //envenom problem - cps spent before aura application

                //BOT_LOG_ERROR("entities.player", "rogue_bot CP SPEND1: %u to 0", tempCP);

                //Relentless Strikes: moved to OnClassSpellGo (triggered even without hitting the target)

                //Ruthlessness
                if (lvl >= 15)
                {
                    if (urand(1,100) <= RUTHLESSNESS_PROC_PCT)
                    {
                        me->CastSpell(target, RUTHLESSNESS_EFFECT, true);
                        //BOT_LOG_ERROR("entities.player", "rogue_bot CP SPEND1: RU proc!");
                    }
                }
            }

            //Preparation: handle effect
            if (baseId == PREPARATION_1)
            {
                //BOT_LOG_ERROR("entities.player", "rogue_bot Preparation hit!");
                if (GetSpell(EVASION_1))
                    SetSpellCooldown(EVASION_1, 0);
                if (GetSpell(SPRINT_1))
                    SetSpellCooldown(SPRINT_1, 0);
                if (GetSpell(VANISH_1))
                    SetSpellCooldown(VANISH_1, 0);
                if (GetSpell(COLD_BLOOD_1))
                    SetSpellCooldown(COLD_BLOOD_1, 0);
                if (GetSpell(SHADOWSTEP_1))
                    SetSpellCooldown(SHADOWSTEP_1, 0);

                //Glyph of Preparation
                //if (lvl >= 30) // same level as spell itself
                {
                    if (GetSpell(BLADE_FLURRY_1))
                        SetSpellCooldown(BLADE_FLURRY_1, 0);
                    if (GetSpell(DISMANTLE_1))
                        SetSpellCooldown(DISMANTLE_1, 0);
                    if (GetSpell(KICK_1))
                        SetSpellCooldown(KICK_1, 0);
                }
            }

            //Glyph of Garrote
            if (lvl >= 15 && baseId == GARROTE_1)
            {
                if (Aura* garr = target->GetAura(spellId, me->GetGUID()))
                {
                    uint32 dur = garr->GetMaxDuration() - 3000;
                    garr->SetDuration(dur);
                    garr->SetMaxDuration(dur);
                    if (AuraEffect* garrEff = garr->GetEffect(0))
                    {
                        garrEff->ChangeAmount(int32(garrEff->GetAmount() * 1.44f)); //not a mistake
                    }
                }
            }
            //Glyph of Deadly Throw
            if (lvl >= 64 && baseId == DEADLY_THROW_1)
            {
                if (AuraEffect* thro = target->GetAuraEffect(spellId, 1, me->GetGUID()))
                {
                    thro->ChangeAmount(thro->GetAmount() - 20);
                }
            }
            //Glyph of Hunger for Blood
            if (lvl >= 60 && baseId == HUNGER_FOR_BLOOD_BUFF)
            {
                if (AuraEffect* hung = me->GetAuraEffect(spellId, 0))
                {
                    hung->ChangeAmount(hung->GetAmount() + 3);
                }
            }
            //Glyph of Cloak of Shadows
            if (lvl >= 66 && baseId == CLOAK_OF_SHADOWS_1)
            {
                if (AuraEffect* cloa = me->GetAuraEffect(spellId, 2))
                {
                    cloa->ChangeAmount(cloa->GetAmount() - 40);
                }
            }
            //Glyph of Sprint
            if (lvl >= 15 && baseId == SPRINT_1)
            {
                if (AuraEffect* spri = me->GetAuraEffect(spellId, 0))
                {
                    spri->ChangeAmount(spri->GetAmount() + 30);
                }
            }
            //Glyph of Vanish
            if (lvl >= 22 && baseId == VANISH_1)
            {
                if (AuraEffect* vani = me->GetAuraEffect(spellId, 2))
                {
                    vani->ChangeAmount(vani->GetAmount() + 30);
                }
            }
            //Glyph of Adrenaline Rush
            if (lvl >= 40 && baseId == ADRENALINE_RUSH_1)
            {
                if (Aura* rush = me->GetAura(spellId))
                {
                    uint32 dur = rush->GetMaxDuration() + 5000;
                    rush->SetDuration(dur);
                    rush->SetMaxDuration(dur);
                }
            }
            //Glyph of Evasion
            if (lvl >= 15 && baseId == EVASION_1)
            {
                if (Aura* evas = me->GetAura(spellId))
                {
                    uint32 dur = evas->GetMaxDuration() + 5000;
                    evas->SetDuration(dur);
                    evas->SetMaxDuration(dur);
                }
            }
            //Glyph of Slice and Dice
            //Improved Slice and Dice
            if (lvl >= 15 && baseId == SLICE_DICE_1)
            {
                if (Aura* dice = me->GetAura(spellId))
                {
                    uint32 dur = dice->GetMaxDuration() + 3000;
                    dur = dur + dur / 2;
                    dice->SetDuration(dur);
                    dice->SetMaxDuration(dur);
                }
            }
            //Glyph of Shadow Dance: 4 sec for bots
            if (lvl >= 60 && baseId == SHADOW_DANCE_1)
            {
                if (Aura* danc = me->GetAura(spellId))
                {
                    uint32 dur = danc->GetMaxDuration() + 4000;
                    danc->SetDuration(dur);
                    danc->SetMaxDuration(dur);
                }
            }
            //Glyph of Rupture
            if (lvl >= 20 && baseId == RUPTURE_1)
            {
                if (Aura* rupt = target->GetAura(spellId, me->GetGUID()))
                {
                    uint32 dur = rupt->GetMaxDuration() + 4000;
                    rupt->SetDuration(dur);
                    rupt->SetMaxDuration(dur);
                }
            }
            //Glyph of Expose Armor
            if (lvl >= 15 && baseId == EXPOSE_ARMOR_1)
            {
                if (Aura* expo = target->GetAura(spellId, me->GetGUID()))
                {
                    uint32 dur = expo->GetMaxDuration() + 12000;
                    expo->SetDuration(dur);
                    expo->SetMaxDuration(dur);
                }
            }
            //Improved Gouge: Increased duration by 1.5 sec
            if (lvl >= 10 && baseId == GOUGE_1)
            {
                if (Aura* goug = target->GetAura(spellId, me->GetGUID()))
                {
                    int32 duration = goug->GetMaxDuration() + 1500;
                    goug->SetDuration(duration);
                    goug->SetMaxDuration(duration);
                }
            }
            //Glyph of Tricks of Trade
            if (lvl >= 75 && baseId == TRICKS_OF_THE_TRADE_BUFF)
            {
                if (Aura* tric = target->GetAura(spellId, me->GetGUID()))
                {
                    int32 duration = tric->GetMaxDuration() + 4000;
                    tric->SetDuration(duration);
                    tric->SetMaxDuration(duration);
                }
            }
            //Cut to the Chase: Eviscerate and Envenom will refresh Slice and Dice duration as for 5 points
            //按 5 CP 基准时长重算（基础 + 雕文 + 强化切割），而非固定拼接魔法数字
            if (GetSpec() == BOT_SPEC_ROGUE_ASSASINATION && lvl >= 55 && (baseId == EVISCERATE_1 || baseId == ENVENOM_1) && GetSpell(SLICE_DICE_1))
            {
                if (Aura* dice = me->GetAura(GetSpell(SLICE_DICE_1)))
                {
                    int32 duration = int32(ROGUE_SND_5CP_BASE_MS + ROGUE_SND_GLYPH_MS + ROGUE_SND_IMPROVED_MS);
                    dice->SetDuration(duration);
                    dice->SetMaxDuration(duration);
                }
            }
            //Waylay
            if ((GetSpec() == BOT_SPEC_ROGUE_SUBTLETY) && lvl >= 45 && (baseId == BACKSTAB_1 || baseId == AMBUSH_1))
                me->CastSpell(target, WAYLAY_DEBUFF, true);

            //Stun: move behind
            if (baseId == CHEAP_SHOT_1 || baseId == KIDNEY_SHOT_1 || baseId == GOUGE_1)
                if (target == opponent)
                    MoveBehind(target);

            OnSpellHitTarget(target, spell);
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType damageType, SpellSchoolMask damageSchoolMask) override
        {
            bot_ai::DamageDealt(victim, damage, damageType, damageSchoolMask);
        }

        void DamageTaken(Unit* u, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellSchoolMask /*schoolMask*/) override
        {
            if (!u)
                return;
            if (!u->IsInCombat() && !me->IsInCombat())
                return;
            OnOwnerDamagedBy(u);
        }

        void OwnerAttackedBy(Unit* u) override
        {
            if (!u)
                return;
            OnOwnerDamagedBy(u);
        }

        void CheckAttackState() override
        {
            if (me->GetVictim() && HasRole(BOT_ROLE_DPS) && !me->HasAuraType(SPELL_AURA_MOD_STEALTH) &&
                (me->isAttackReady() || me->isAttackReady(OFF_ATTACK)) &&
                (!me->GetVictim()->GetAuraEffect(SPELL_AURA_MOD_STUN, SPELLFAMILY_ROGUE, 0x8, 0x0, 0x0) &&
                !me->GetVictim()->GetAuraEffect(SPELL_AURA_MOD_CONFUSE, SPELLFAMILY_ROGUE, 0x01000000, 0x0, 0x0)))
                DoMeleeAttackIfReady();
        }

        uint32 GetAIMiscValue(uint32 data) const override
        {
            switch (data)
            {
                case BOTAI_MISC_COMBO_POINTS:
                    return comboPoints;
                case BOTAI_MISC_ENCHANT_IS_AUTO_MH:
                    return needChooseMHEnchant;
                case BOTAI_MISC_ENCHANT_IS_AUTO_OH:
                    return needChooseOHEnchant;
                case BOTAI_MISC_ENCHANT_TIMER_MH:
                    return mhEnchantExpireTimer;
                case BOTAI_MISC_ENCHANT_TIMER_OH:
                    return ohEnchantExpireTimer;
                case BOTAI_MISC_ENCHANT_CURRENT_MH:
                    return mhEnchant;
                case BOTAI_MISC_ENCHANT_CURRENT_OH:
                    return ohEnchant;
                case BOTAI_MISC_ENCHANT_AVAILABLE_1:
                    return GetSpell(CRIPPLING_POISON_1) ? CRIPPLING_POISON_1 : 0;
                case BOTAI_MISC_ENCHANT_AVAILABLE_2:
                    return GetSpell(INSTANT_POISON_1) ? INSTANT_POISON_1 : 0;
                case BOTAI_MISC_ENCHANT_AVAILABLE_3:
                    return GetSpell(MIND_NUMBING_POISON_1) ? MIND_NUMBING_POISON_1 : 0;
                case BOTAI_MISC_ENCHANT_AVAILABLE_4:
                    return GetSpell(DEADLY_POISON_1) ? DEADLY_POISON_1 : 0;
                case BOTAI_MISC_ENCHANT_AVAILABLE_5:
                    return GetSpell(WOUND_POISON_1) ? WOUND_POISON_1 : 0;
                case BOTAI_MISC_ENCHANT_AVAILABLE_6:
                    return GetSpell(ANESTHETIC_POISON_1) ? ANESTHETIC_POISON_1 : 0;
                default:
                    return 0;
            }
        }

        void SetAIMiscValue(uint32 data, uint32 value) override
        {
            switch (data)
            {
                case BOTAI_MISC_DAGGER_MAINHAND:
                    isdaggerMH = bool(value);
                    break;
                case BOTAI_MISC_DAGGER_OFFHAND:
                    isdaggerOH = bool(value);
                    break;
                case BOTAI_MISC_ENCHANT_IS_AUTO_MH:
                    needChooseMHEnchant = bool(value);
                    break;
                case BOTAI_MISC_ENCHANT_IS_AUTO_OH:
                    needChooseOHEnchant = bool(value);
                    break;
                case BOTAI_MISC_ENCHANT_TIMER_MH:
                    if (value == 0)
                        mhEnchantExpireTimer = value;
                    break;
                case BOTAI_MISC_ENCHANT_TIMER_OH:
                    if (value == 0)
                        ohEnchantExpireTimer = value;
                    break;
                case BOTAI_MISC_ENCHANT_CURRENT_MH:
                    mhEnchant = value;
                    SetAIMiscValue(BOTAI_MISC_ENCHANT_IS_AUTO_MH, value ? false : true);
                    break;
                case BOTAI_MISC_ENCHANT_CURRENT_OH:
                    ohEnchant = value;
                    SetAIMiscValue(BOTAI_MISC_ENCHANT_IS_AUTO_OH, value ? false : true);
                    break;
                default:
                    break;
            }

            bot_ai::SetAIMiscValue(data, value);
        }

        void Reset() override
        {
            energy = 0;
            comboPoints = 0;
            combopointsSpent = false;
            glyphSSProc = false;
            _counterCheckTimer = 0;

            mhEnchantExpireTimer = std::min<uint32>(mhEnchantExpireTimer, 1);
            ohEnchantExpireTimer = std::min<uint32>(ohEnchantExpireTimer, 1);

            DefaultInit();

            //after InitEquips
            Item const* mh = GetEquips(BOT_SLOT_MAINHAND);
            Item const* oh = GetEquips(BOT_SLOT_OFFHAND);
            isdaggerMH = mh && mh->GetTemplate()->Class == ITEM_CLASS_WEAPON && mh->GetTemplate()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER;
            isdaggerOH = oh && oh->GetTemplate()->Class == ITEM_CLASS_WEAPON && oh->GetTemplate()->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER;
        }

        void ReduceCD(uint32 diff) override
        {
            if (mhEnchantExpireTimer > diff)            mhEnchantExpireTimer -= diff;
            if (ohEnchantExpireTimer > diff)            ohEnchantExpireTimer -= diff;
        }

        void InitPowers() override
        {
            //Hack for power
            me->SetPowerType(POWER_ENERGY);

            if (energy)
                me->SetPower(POWER_ENERGY, energy);
            else
                me->SetPower(POWER_ENERGY, me->GetMaxPower(POWER_ENERGY));
        }

        void InitSpells() override
        {
            uint8 lvl = me->GetLevel();
            bool isAssa = GetSpec() == BOT_SPEC_ROGUE_ASSASINATION;
            bool isComb = GetSpec() == BOT_SPEC_ROGUE_COMBAT;
            bool isSubt = GetSpec() == BOT_SPEC_ROGUE_SUBTLETY;

            InitSpellMap(KICK_1);
            InitSpellMap(EXPOSE_ARMOR_1);
            InitSpellMap(DISMANTLE_1);
            InitSpellMap(FEINT_1);
            InitSpellMap(DISARM_TRAP_1);

            InitSpellMap(BACKSTAB_1);
            InitSpellMap(SINISTER_STRIKE_1);
            InitSpellMap(EVISCERATE_1);
            InitSpellMap(ENVENOM_1);
            InitSpellMap(RUPTURE_1);
            InitSpellMap(DEADLY_THROW_1);
            InitSpellMap(FAN_OF_KNIVES_1);

            InitSpellMap(SPRINT_1);
            InitSpellMap(EVASION_1);
            InitSpellMap(BLIND_1);
            InitSpellMap(VANISH_1);

            InitSpellMap(GOUGE_1);

            InitSpellMap(KIDNEY_SHOT_1);
            InitSpellMap(SLICE_DICE_1);
            InitSpellMap(CLOAK_OF_SHADOWS_1);
            InitSpellMap(TRICKS_OF_THE_TRADE_1);

            InitSpellMap(STEALTH_1);
            InitSpellMap(SAP_1);
            InitSpellMap(GARROTE_1);
            InitSpellMap(CHEAP_SHOT_1);
            InitSpellMap(AMBUSH_1);

            lvl >= 30 && isAssa ? InitSpellMap(COLD_BLOOD_1) : RemoveSpell(COLD_BLOOD_1);
            lvl >= 50 && isAssa ? InitSpellMap(MUTILATE_1) : RemoveSpell(MUTILATE_1);
            lvl >= 60 && isAssa ? InitSpellMap(HUNGER_FOR_BLOOD_1) : RemoveSpell(HUNGER_FOR_BLOOD_1);

            lvl >= 20 && isComb ? InitSpellMap(RIPOSTE_1) : RemoveSpell(RIPOSTE_1);
            lvl >= 30 && isComb ? InitSpellMap(BLADE_FLURRY_1) : RemoveSpell(BLADE_FLURRY_1);
            lvl >= 40 && isComb ? InitSpellMap(ADRENALINE_RUSH_1) : RemoveSpell(ADRENALINE_RUSH_1);
            lvl >= 60 && isComb ? InitSpellMap(KILLING_SPREE_1) : RemoveSpell(KILLING_SPREE_1);

            lvl >= 20 && isSubt ? InitSpellMap(GHOSTLY_STRIKE_1) : RemoveSpell(GHOSTLY_STRIKE_1);
            lvl >= 30 && isSubt ? InitSpellMap(HEMORRHAGE_1) : RemoveSpell(HEMORRHAGE_1);
            lvl >= 30 && isSubt ? InitSpellMap(PREPARATION_1) : RemoveSpell(PREPARATION_1);
            lvl >= 40 && isSubt ? InitSpellMap(PREMEDITATION_1) : RemoveSpell(PREMEDITATION_1);
            lvl >= 50 && isSubt ? InitSpellMap(SHADOWSTEP_1) : RemoveSpell(SHADOWSTEP_1);
            lvl >= 60 && isSubt ? InitSpellMap(SHADOW_DANCE_1) : RemoveSpell(SHADOW_DANCE_1);

            //InitSpellMap(DISTRACT_1);

            InitSpellMap(CRIPPLING_POISON_1);
            InitSpellMap(INSTANT_POISON_1);
            InitSpellMap(DEADLY_POISON_1);
            InitSpellMap(WOUND_POISON_1);
            InitSpellMap(MIND_NUMBING_POISON_1);
            InitSpellMap(ANESTHETIC_POISON_1);

            lvl >= 10 ? InitSpellMap(THISTLE_TEA) : RemoveSpell(THISTLE_TEA);
        }

        void ApplyClassPassives() const override
        {
            uint8 level = master->GetLevel();
            uint8 spec = GetSpec();

            //遍历天赋描述表：按 [minLvl, maxLvl) 区间与专精（含“任意专精”哨兵）刷新被动
            for (RogueTalentDef const& t : RogueTalents)
                RefreshAura(t.auraId, (t.spec == ROGUE_ANY_SPEC || t.spec == spec) &&
                    level >= t.minLvl && (!t.maxLvl || level < t.maxLvl) ? 1 : 0);

            //非“专精 + 等级区间”描述的特殊条目
            RefreshAura(TURN_THE_TABLES, !IAmFree() && spec == BOT_SPEC_ROGUE_SUBTLETY && level >= 55 ? 1 : 0);
            //RefreshAura(HONOR_AMONG_THIEVES, spec == BOT_SPEC_ROGUE_SUBTLETY && level >= 55 ? 1 : 0);

            RefreshAura(ROGUE_PASSIVE_DND);
        }

        bool CanUseManually(uint32 basespell) const override
        {
            switch (basespell)
            {
                case STEALTH_1:
                case SPRINT_1:
                case VANISH_1:
                case BLADE_FLURRY_1:
                case FAN_OF_KNIVES_1:
                case TRICKS_OF_THE_TRADE_1:
                case PREPARATION_1:
                case EXPOSE_ARMOR_1:
                case SAP_1:
                    return true;
                default:
                    return false;
            }
        }

        float GetBotArmorPenetrationCoef() const override
        {
            float bonus = 0.0f;

            //Serrated Blades part 1
            if (me->GetLevel() >= 20)
                bonus += 9.f;

            //Mace Specialization: 15% armor penetration
            if (me->GetLevel() >= 30)
                if (Item const* weap = GetEquips(BOT_SLOT_MAINHAND))
                    if (weap->GetTemplate()->SubClass == ITEM_SUBCLASS_WEAPON_MACE)
                        bonus += 15.f;

            return bonus + bot_ai::GetBotArmorPenetrationCoef();
        }

        bool HasAbilitiesSpecifics() const override { return true; }
        void FillAbilitiesSpecifics(Player const* player, std::list<std::string> &specList) override
        {
            uint32 textId1, textId2;
            switch (mhEnchant)
            {
                case CRIPPLING_POISON_1:    textId1 = BOT_TEXT_CRIPPLING;   break;
                case INSTANT_POISON_1:      textId1 = BOT_TEXT_INSTANT;     break;
                case DEADLY_POISON_1:       textId1 = BOT_TEXT_DEADLY;      break;
                case WOUND_POISON_1:        textId1 = BOT_TEXT_WOUND;       break;
                case MIND_NUMBING_POISON_1: textId1 = BOT_TEXT_MINDNUMBING; break;
                case ANESTHETIC_POISON_1:   textId1 = BOT_TEXT_ANESTHETIC;  break;
                default:                    textId1 = BOT_TEXT_NOTHING_C;   break;
            }
            switch (ohEnchant)
            {
                case CRIPPLING_POISON_1:    textId2 = BOT_TEXT_CRIPPLING;   break;
                case INSTANT_POISON_1:      textId2 = BOT_TEXT_INSTANT;     break;
                case DEADLY_POISON_1:       textId2 = BOT_TEXT_DEADLY;      break;
                case WOUND_POISON_1:        textId2 = BOT_TEXT_WOUND;       break;
                case MIND_NUMBING_POISON_1: textId2 = BOT_TEXT_MINDNUMBING; break;
                case ANESTHETIC_POISON_1:   textId2 = BOT_TEXT_ANESTHETIC;  break;
                default:                    textId2 = BOT_TEXT_NOTHING_C;   break;
            }
            specList.push_back(LocalizedNpcText(player, BOT_TEXT_SLOT_MH) + ": " + LocalizedNpcText(player, textId1));
            specList.push_back(LocalizedNpcText(player, BOT_TEXT_SLOT_OH) + ": " + LocalizedNpcText(player, textId2));
        }

        std::vector<uint32> const* GetDamagingSpellsList() const override
        {
            return &Rogue_spells_damage;
        }
        std::vector<uint32> const* GetCCSpellsList() const override
        {
            return &Rogue_spells_cc;
        }
        //std::vector<uint32> const* GetHealingSpellsList() const override
        //{
        //    return &Rogue_spells_heal;
        //}
        std::vector<uint32> const* GetSupportSpellsList() const override
        {
            return &Rogue_spells_support;
        }

    private:
        mutable bool glyphSSProc;
        int32 energy;
        uint8 comboPoints;
        bool combopointsSpent;
        bool isdaggerMH, isdaggerOH;
        uint32 mhEnchantExpireTimer, ohEnchantExpireTimer;
        uint32 mhEnchant, ohEnchant;
        bool needChooseMHEnchant, needChooseOHEnchant;
        uint32 _counterCheckTimer = 0; //打断检查定频计时器
    };
};

void AddSC_rogue_bot()
{
    new rogue_bot();
}
