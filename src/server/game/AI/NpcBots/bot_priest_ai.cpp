#include "bot_ai.h"
#include "botdatamgr.h"
#include "botlogtraits.h"
#include "botmgr.h"
#include "botspell.h"
#include "bottext.h"
#include "bottraits.h"
#include "Group.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
/*
Priest NpcBot (reworked by Trickerer onlysuffering@gmail.com)
Complete - Around 95%
战斗逻辑按专精拆分（戒律 / 神圣 / 暗影），风格参考术士 BOT 重构
*/

enum PriestBaseSpells
{
    DISPEL_MAGIC_1                      = 527,
    MASS_DISPEL_1                       = 32375,
    CURE_DISEASE_1                      = 528,
    ABOLISH_DISEASE_1                   = 552,
    FEAR_WARD_1                         = 6346,
    PAIN_SUPPRESSION_1                  = 33206,
    PSYCHIC_SCREAM_1                    = 8122,
    FADE_1                              = 586,
    PSYCHIC_HORROR_1                    = 64044,
    SILENCE_1                           = 15487,
    PENANCE_1                           = 47540,
    VAMPIRIC_EMBRACE_1                  = 15286,
    DISPERSION_1                        = 47585,
    MIND_SEAR_1                         = 48045,
    GUARDIAN_SPIRIT_1                   = 47788,
    SHACKLE_UNDEAD_1                    = 9484,
    LESSER_HEAL_1                       = 2050,
    NORMAL_HEAL_1                       = 2054,
    GREATER_HEAL_1                      = 2060,
    RENEW_1                             = 139,
    FLASH_HEAL_1                        = 2061,
    BINDING_HEAL_1                      = 32546,
    PRAYER_OF_HEALING_1                 = 596,
    CIRCLE_OF_HEALING_1                 = 34861,
    DIVINE_HYMN_1                       = 64843,
    PRAYER_OF_MENDING_1                 = 33076,
    RESURRECTION_1                      = 2006,
    PW_SHIELD_1                         = 17,
    INNER_FIRE_1                        = 588,
    PW_FORTITUDE_1                      = 1243,
    SHADOW_PROTECTION_1                 = 976,
    DIVINE_SPIRIT_1                     = 14752,
    HOLY_FIRE_1                         = 14914,
    HOLY_NOVA_1                         = 15237,
    SMITE_1                             = 585,
    SW_PAIN_1                           = 589,
    MIND_BLAST_1                        = 8092,
    SW_DEATH_1                          = 32379,
    DEVOURING_PLAGUE_1                  = 2944,
    MIND_FLAY_1                         = 15407,
    VAMPIRIC_TOUCH_1                    = 34914,
    SHADOWFORM_1                        = 15473,
    INNER_FOCUS_1                       = 14751,
    DESPERATE_PRAYER_1                  = 19236,
    POWER_INFUSION_1                    = 10060,
    HYMN_OF_HOPE_1                      = 64901,
    MANA_BURN_1                         = 8129,
    LIGHTWELL_1                         = 724,

    LEVITATE_1                          = 1706
};
enum PriestPassives
{
//Talents
    UNBREAKABLE_WILL                = 14791,//rank 5坚定意志
    SPIRIT_TAP                      = 15336,//rank 3精神分流
    IMPROVED_SPIRIT_TAP             = 15338,//rank 2强化精神分流
    HEIAN                           = 15310,//rank 2黑暗
    QH_ANYANSHU = 15317,//rank 2强化暗言术：痛
    QH_JIZHONG = 15328,//暗影集中
    QH_XINGLINGBAOZHENG = 15316,//强化心灵震爆
    QH_ZHEBIZHIYING = 15311,//遮蔽之影
    SHADOW_WEAVING1                 = 15257,//暗影交织
    SHADOW_WEAVING2                 = 15331,
    SHADOW_WEAVING3                 = 15332,
    QH_XIXUEGUI = 27840,//强化吸血鬼的拥抱
    QH_RONGHUA = 33371,//心灵熔化
    QH_ANYINGNENGLIANG = 33225,//暗影能量
    MISERY1                         = 33191,
    MISERY2                         = 33192,
    MISERY3                         = 33193,//悲惨
    QH_ANYINGXINGTAI = 47570,//强化暗影形态
    IMPROVED_DEVOURING_PLAGUE       = 63627,//rank 3强化噬灵疫病
    BAOSHOUZHEMO = 47582,//饱受折磨
    NIUQUXINGYANG = 51167,//扭曲信仰
    SHUANGSHENGJIELV = 52803,//双生戒律
    XINGLINGZHIHUO = 14771,//强化心灵之火
    SHENSHENGZHUANJING = 15011,//神圣专精
    MEDITATION                      = 14777,//rank 3冥想
    INSPIRATION1                    = 14892,//灵感
    INSPIRATION2                    = 15362,
    INSPIRATION3                    = 15363,//灵感
    SURGE_OF_LIGHT                  = 33154,//rank 2圣光涌动
    QH_ZHILIAOSHU = 15014,//rank 3强化治疗术
    QH_ZHILIAOQIDAO = 15018,//rank 3治疗祈祷
    JINGSHENZHIYING = 15031,//rank 5精神指引
    JINGSHENZHILIAO = 15356,//rank 5精神治疗
    QUANXINGQUANYI = 64129,//rank 全心全意
    ZHILIAOZENGXIAO = 33162,//rank 5治疗增效
    SHENSHENGJUANGU = 47567,//rank 5神圣眷顾
    EMPOWERED_RENEW1                = 63534,//恢复增效
    EMPOWERED_RENEW2                = 63542,
    EMPOWERED_RENEW3                = 63543,
    HOLY_CONCENTRATION              = 34860,//rank 3神圣专注
    RENEWED_HOPE                    = 57472,//rank 3新生希望
    RAPTURE                         = 47537,//rank 3全神贯注
    BODY_AND_SOUL1                  = 64127,//全心全意
    SERENDIPITY                     = 63737,//rank 3好运
    IMPROVED_SHADOWFORM             = 47570,//rank 2强化暗影形态
    DIVINE_AEGIS                    = 47515,//rank 3神圣庇护
    GRACE                           = 47517,//rank 2恩赐
    BORROWED_TIME                   = 52800,//rank 5争分夺秒
    QH_DUN = 14769,//强化真言术：盾
    QH_SHEMINA = 33172,//赦免
    JINGSHENMINGRUI = 14781,//精神敏锐
    XINGLINGZHILI = 18555,//心灵之力
    XINGLINGSHOUHU = 63574,//灵魂守护
    NENGLIANGJIZHONG = 33190,//能量集中
    QIDI = 34910,//启迪
    KUAISUZHIL = 63506,//强化快速治疗
    KEWANG = 47508,//渴望
//Glyphs
    GLYPH_SW_PAIN                   = 55681,//痛点雕文：暗言术：痛 +10% 爆击
    GLYPH_KUXIU = 63235,//苦修雕文
    GLYPH_PW_SHIELD                 = 55672,
    GLYPH_DISPEL_MAGIC              = 55677,
    GLYPH_PRAYER_OF_HEALING         = 55680,
    GLYPH_SHADOW                    = 55689,//暗影雕文
    GLYPH_BIANCHI = 55687,//精神鞭笞雕文
//套装与其他
    PRIEST_T10_2P_BONUS             = 70770,//T10 2件套：恢复 33% 额外治疗
    PRIEST_T10_SHADOW_2P_BONUS      = 49868 //T10 暗影 2件套：吸血鬼之触 +6 秒持续时间
};
enum PriestSpecial
{
    SHADOW_WEAVING_BUFF             = 15258,
    MIND_FLAY_DAMAGE                = 58381,
    MIND_SEAR_DAMAGE_1              = 49821,
    SW_DEATH_BACKLASH               = 32409,
    WEAKENED_SOUL_DEBUFF            = 6788,
    SURGE_OF_LIGHT_BUFF             = 33151,
    SERENDIPITY_BUFF                = 63734,
    DIVINE_HYMN_HEAL                = 64844,
    PRAYER_OF_MENDING_AURA_1        = 41635,
    PRAYER_OF_MENDING_HEAL          = 33110,
    PENANCE_HEAL_1                  = 47750,
    IMPROVED_MIND_BLAST_DEBUFF      = 48301,//Mind Trauma
    HYMN_OF_HOPE_BUFF               = 64904,

    MISERY_DEBUFF                   = 33198,//悲惨：目标受到法术伤害 +3%
    SHADOWFIEND_1                   = 34433
};

static const std::vector<uint32> Priest_spells_damage
{ DEVOURING_PLAGUE_1, HOLY_FIRE_1, HOLY_NOVA_1, MIND_BLAST_1, MIND_FLAY_1, MIND_SEAR_1, PENANCE_1, SMITE_1, SW_PAIN_1, SW_DEATH_1, VAMPIRIC_TOUCH_1 };
static const std::vector<uint32> Priest_spells_cc
{ PSYCHIC_HORROR_1, PSYCHIC_SCREAM_1, SHACKLE_UNDEAD_1, SILENCE_1 };
static const std::vector<uint32> Priest_spells_heal
{ RENEW_1, FLASH_HEAL_1, BINDING_HEAL_1, LESSER_HEAL_1, NORMAL_HEAL_1, GREATER_HEAL_1, PRAYER_OF_HEALING_1, PRAYER_OF_MENDING_1,
GUARDIAN_SPIRIT_1, PENANCE_1, DIVINE_HYMN_1, CIRCLE_OF_HEALING_1, DESPERATE_PRAYER_1, LIGHTWELL_1 };
static const std::vector<uint32> Priest_spells_support
{ PW_FORTITUDE_1, DIVINE_SPIRIT_1, SHADOW_PROTECTION_1, ABOLISH_DISEASE_1, CURE_DISEASE_1,
DISPEL_MAGIC_1, MASS_DISPEL_1, DISPERSION_1, FADE_1, FEAR_WARD_1, HYMN_OF_HOPE_1, INNER_FIRE_1, INNER_FOCUS_1,
LEVITATE_1, PAIN_SUPPRESSION_1, POWER_INFUSION_1, PW_SHIELD_1, RESURRECTION_1, SHADOWFORM_1, VAMPIRIC_EMBRACE_1, MANA_BURN_1 };

class priest_bot : public CreatureScript
{
public:
    priest_bot() : CreatureScript("priest_bot") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new priest_botAI(creature);
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

    struct priest_botAI : public bot_ai
    {
        priest_botAI(Creature* creature) : bot_ai(creature)
        {
            _botclass = BOT_CLASS_PRIEST;

            InitUnitFlags();
        }

        bool doCast(Unit* victim, uint32 spellId)
        {
            if (CheckBotCast(victim, spellId) != SPELL_CAST_OK)
                return false;
            return bot_ai::doCast(victim, spellId);
        }

        void CheckShadowfiend(uint32 diff)
        {
            if (!HasRole(BOT_ROLE_HEAL) || GetSpec() == BOT_SPEC_PRIEST_SHADOW ||
                !IsSpellReady(SHADOWFIEND_1, diff) || IsCasting() || GetManaPCT(me) >= 60 || GetBotsPet())
                return;

            Unit* target = me->GetVictim();
            if (!target && !IAmFree())
                target = master->GetVictim();
            if (!target || !target->IsAlive() || !target->IsInCombat() ||
                !me->IsValidAttackTarget(target) || me->GetDistance(target) > 30)
            {
                target = nullptr;
                std::list<Unit*> targets;
                GetNearbyTargetsList(targets, 30.f, 0);
                for (Unit* candidate : targets)
                {
                    if (candidate->IsAlive() && candidate->IsInCombat() && me->IsValidAttackTarget(candidate))
                    {
                        target = candidate;
                        break;
                    }
                }
            }

            if (!target)
                return;

            SummonBotPet(target);
            if (GetBotsPet())
                SetSpellCooldown(SHADOWFIEND_1, 300000);
        }

        void CheckHymnOfHope(uint32 diff)
        {
            if (!IsSpellReady(HYMN_OF_HOPE_1, diff) || Rand() > 45 || IsCasting() || IsTank())
                return;

            Group const* gr = !IAmFree() ? master->GetGroup() : GetGroup();
            if (!gr)
                return;

            uint8 LMPcount = 0;
            uint8 lowHealerCount = 0;
            for (Unit const* member : BotMgr::GetAllGroupMembers(gr))
            {
                if (me->GetMap() != member->FindMap() || !member->IsAlive() || !member->IsInCombat() ||
                    me->GetDistance(member) > 40 || (member->IsNPCBot() && member->ToCreature()->IsTempBot()))
                    continue;

                //治疗牧不在队友濒危或预计即将濒危时开始引导
                if (HasRole(BOT_ROLE_HEAL) && (GetHealthPCT(member) < 40 ||
                    int32(member->GetHealth()) + GetHPS(member) * 3 < int32(member->GetMaxHealth() * 0.4f)))
                    return;

                if (member->GetPowerType() != POWER_MANA || member->GetMaxPower(POWER_MANA) <= 1 ||
                    member->GetAuraEffect(SPELL_AURA_MOD_INCREASE_ENERGY, SPELLFAMILY_PRIEST, 0x0, 0x0, 0x10))
                    continue;
                if (GetManaPCT(member) < 40)
                    ++lowHealerCount;
                if (GetManaPCT(member) <= 50)
                    ++LMPcount;
            }

            bool needsMana = HasRole(BOT_ROLE_HEAL) ? (GetManaPCT(me) < 40 || lowHealerCount >= 2) : LMPcount > 2;
            if (needsMana && doCast(me, GetSpell(HYMN_OF_HOPE_1)))
                return;
        }

        bool MassGroupHeal(uint32 diff)
        {
            if (!HasRole(BOT_ROLE_HEAL) || IsCasting() || Rand() > (65 + 40 * me->GetMap()->IsRaid()))
                return false;

            Group const* gr = !IAmFree() ? master->GetGroup() : GetGroup();
            if (!gr)
                return false;

            bool canHymn = IsSpellReady(DIVINE_HYMN_1, diff, false);
            bool canPray = !!GetSpell(PRAYER_OF_HEALING_1);
            bool canCirc = IsSpellReady(CIRCLE_OF_HEALING_1, diff, false);

            uint8 LHPcount1, LHPcount2, LHPcount3;
            LHPcount1 = LHPcount2 = LHPcount3 = 0;
            uint8 criticalPartyCount = 0;
            uint8 lowestPCT = 100;
            Unit* castTarget = nullptr;

            for (Unit* member : BotMgr::GetAllGroupMembers(gr))
            {
                if (me->GetMap() != member->FindMap() || !member->IsAlive() || !member->IsInCombat() ||
                    member->isPossessed() || member->IsCharmed() || (member->IsNPCBot() && member->ToCreature()->IsTempBot()) ||
                    member->GetAuraEffect(SPELL_AURA_MOD_INCREASE_ENERGY, SPELLFAMILY_PRIEST, 0x0, 0x0, 0x10))
                    continue;

                float dist = me->GetDistance(member);
                uint8 pct = GetHealthPCT(member);
                if (canHymn && pct < std::min<uint32>(80, 50 + member->getAttackers().size()*10) && GetLostHP(member) > 4000 && dist < 40)
                    ++LHPcount1;
                //治疗祷言只统计实际能命中的自身小队成员
                if (canPray && pct < 65 && dist < 30 && me->IsInPartyWith(member))
                {
                    ++LHPcount2;
                    if (pct < 50)
                        ++criticalPartyCount;
                }
                //治疗之环放宽：≥1 人 <80% 且 15 码内有第二个低血单位（5 人小队可正常触发）
                if (canCirc && pct < 80 && dist < 40 && (!castTarget || castTarget->GetDistance(member) < 15))
                {
                    ++LHPcount3;
                    if (pct < lowestPCT)
                    {
                        lowestPCT = pct;
                        castTarget = member;
                    }
                }
            }

            if (LHPcount1 > 2 && doCast(me, GetSpell(DIVINE_HYMN_1)))
                return true;
            if (LHPcount2 >= 3 || criticalPartyCount >= 2)
            {
                if (me->IsInCombat() && IsSpellReady(INNER_FOCUS_1, diff) && GetManaPCT(me) < 70 &&
                    doCast(me, GetSpell(INNER_FOCUS_1)))
                {}
                if (doCast(me, GetSpell(PRAYER_OF_HEALING_1)))
                    return true;
            }
            if (LHPcount3 > 1 && castTarget && doCast(castTarget, GetSpell(CIRCLE_OF_HEALING_1)))
                return true;

            return false;
        }

        bool ShieldGroup(uint32 diff)
        {
            if (!IsSpellReady(PW_SHIELD_1, false, diff) || IsCasting() || Rand() > 65 + 100 * (me->GetMap()->IsRaid()))
                return false;
            if (!IAmFree() && !(me->GetLevel() >= 30 && _spec == BOT_SPEC_PRIEST_DISCIPLINE) &&
                master->GetBotMgr()->HasBotWithSpec(BOT_SPEC_PRIEST_DISCIPLINE))
                return false;

            Group const* gr = !IAmFree() ? master->GetGroup() : GetGroup();
            if (!gr)
            {
                Unit* u = master;
                if (u->IsAlive() && !u->getAttackers().empty() && (IsTank(u) || GetHealthPCT(u) < 75) && me->GetDistance(u) < 40 &&
                    ShieldTarget(u, diff))
                    return true;
                if (!IAmFree())
                {
                    for (auto const& [_, bot] : *master->GetBotMgr()->GetBotMap())
                    {
                        u = bot;
                        if (u->IsAlive() && !u->getAttackers().empty() && !u->ToCreature()->IsTempBot() &&
                            (IsTank(u) || GetHealthPCT(u) < 75) && me->GetDistance(u) < 40 &&
                            ShieldTarget(u, diff))
                            return true;
                    }
                    for (Unit* m : master->m_Controlled)
                    {
                        u = m;
                        if (!u || !u->IsPet() || me->GetMap() != u->FindMap())
                            continue;
                        if (u->IsAlive() && !u->getAttackers().empty() && (IsTank(u) || GetHealthPCT(u) < 75) && me->GetDistance(u) < 40 &&
                            ShieldTarget(u, diff))
                            return true;
                    }
                }
            }
            else
            {
                std::vector<Unit*> members = BotMgr::GetAllGroupMembers(gr);
                for (auto i : NPCBots::index_array<uint8, 2>)
                {
                    for (Unit* member : members)
                    {
                        if (!(i == 0 ? member->IsPlayer() : member->IsNPCBot()) || me->GetMap() != member->FindMap() ||
                            !member->IsAlive() || me->GetDistance(member) > 40 || member->isPossessed() || member->IsCharmed() ||
                            member->getAttackers().empty() || (!IsTank(member) && !IsFlagCarrier(member) && GetHealthPCT(member) > 75) ||
                            (member->IsNPCBot() && member->ToCreature()->IsTempBot()) ||
                            member->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_PALADIN, 0x0, 0x80000, 0x0))
                            continue;
                        if (ShieldTarget(member, diff))
                            return true;
                    }
                }
            }
            return false;
        }

        bool ShieldTarget(Unit* target, uint32 diff)
        {
            if (!IsSpellReady(PW_SHIELD_1, diff) || IsCasting())
                return false;
            if (_spec == BOT_SPEC_PRIEST_DISCIPLINE &&
                GetHealthPCT(target) >= ((IsTank(target) || IsPointedHealTarget(target)) ? 90 : 70))
                return false;
            if (target->HasAuraTypeWithFamilyFlags(SPELL_AURA_MECHANIC_IMMUNITY, SPELLFAMILY_PRIEST, 0x20000000) ||
                target->HasAuraTypeWithFamilyFlags(SPELL_AURA_SCHOOL_ABSORB, SPELLFAMILY_PRIEST, 0x1))
                return false;

            if (doCast(target, GetSpell(PW_SHIELD_1)))
                return true;

            return false;
        }

        //戒律系：为坦克/被点名目标维持真言术：盾，用于激活新生希望与争分夺秒（G2）
        //返回 true 表示本次已施放盾；未施放时交由 ShieldGroup 继续为全团补盾（无队伍时亦由此兜底）
        bool MaintainWeakenedSoul(uint32 diff)
        {
            if (GetSpec() != BOT_SPEC_PRIEST_DISCIPLINE)
                return false;
            if (!IsSpellReady(PW_SHIELD_1, false, diff) || IsCasting())
                return false;

            Group const* gr = !IAmFree() ? master->GetGroup() : GetGroup();
            if (!gr)
                return false;

            for (Unit* member : BotMgr::GetAllGroupMembers(gr))
            {
                if (me->GetMap() != member->FindMap() || !member->IsAlive() || !member->IsInCombat() ||
                    member->isPossessed() || member->IsCharmed() || (member->IsNPCBot() && member->ToCreature()->IsTempBot()) ||
                    (!IsTank(member) && !IsPointedHealTarget(member)))
                    continue;
                //虚弱灵魂与已有盾的判重统一由 ShieldTarget 处理，避免此处重复查询光环
                if (ShieldTarget(member, diff))
                    return true;
            }

            return false;
        }

        //光明之泉：团队低血量时放置，供队友自行点击治疗（G5）
        void CheckLightwell(uint32 diff)
        {
            if (!GetSpell(LIGHTWELL_1) || Lightwell_Timer > diff || IsCasting() || Rand() > 25)
                return;

            Lightwell_Timer = urand(15000, 25000);

            Group const* gr = !IAmFree() ? master->GetGroup() : GetGroup();
            if (!gr)
                return;

            uint8 lowCount = 0;
            for (Unit const* member : BotMgr::GetAllGroupMembers(gr))
            {
                if (me->GetMap() != member->FindMap() || !member->IsAlive() || !member->IsInCombat() ||
                    me->GetDistance(member) > 30)
                    continue;
                if (GetHealthPCT(member) < 70 && ++lowCount > 1)
                    break;
            }

            if (lowCount > 1 && doCast(me, GetSpell(LIGHTWELL_1)))
                return;
        }

        //法力燃烧：对高蓝量法系目标抽取法力（G5）
        void CheckManaBurn(uint32 diff)
        {
            if (!GetSpell(MANA_BURN_1) || ManaBurn_Timer > diff || IsCasting() || Rand() > 40)
                return;

            ManaBurn_Timer = urand(1000, 2000);

            if (Unit* target = me->GetVictim())
            {
                if (target->GetPowerType() == POWER_MANA && GetManaPCT(target) > 30 &&
                    target->GetMaxPower(POWER_MANA) > me->GetMaxPower(POWER_MANA) &&
                    me->GetDistance(target) < CalcSpellMaxRange(MANA_BURN_1) &&
                    doCast(target, GetSpell(MANA_BURN_1)))
                    return;
            }
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
        void JustDied(Unit* u) override { UnsummonAll(false); bot_ai::JustDied(u); }

        bool removeShapeshiftForm() override
        {
            ShapeshiftForm form = me->GetShapeshiftForm();
            if (form != FORM_NONE)
            {
                switch (form)
                {
                    case FORM_SHADOW:
                        me->RemoveAurasDueToSpell(SHADOWFORM_1);
                        break;
                    default:
                        break;
                }
            }

            return true;
        }

        void BreakCC(uint32 diff) override
        {
            //Improved Shadowform: Fade
            if (IsSpellReady(FADE_1, diff) && me->GetShapeshiftForm() == FORM_SHADOW && me->GetLevel() >= 45 &&
                Rand() < 35 && me->HasAuraWithMechanic((1u<<MECHANIC_SNARE)|(1u<<MECHANIC_ROOT)))
            {
                if (doCast(me, GetSpell(FADE_1)))
                    return;
            }
            bot_ai::BreakCC(diff);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!GlobalUpdate(diff))
                return;

            DoVehicleActions(diff);
            if (!CanBotAttackOnVehicle())
                return;

            Disperse(diff);

            DoDevCheck(diff);
            DoShackCheck(diff);

            if (IsPotionReady())
            {
                if (GetManaPCT(me) < 33)
                    DrinkPotion(true);
                else if (GetHealthPCT(me) < 50 && (!HasRole(BOT_ROLE_HEAL) || me->HasAuraType(SPELL_AURA_MOD_SILENCE)))
                    DrinkPotion(false);
            }

            CheckRacials(diff);

            doDefend(diff);

            //治疗系（戒律/神圣）：团刷 → 戒律先保坦克盾 → 全团补盾 → 愈合祷言 → 单体治疗
            if (HasRole(BOT_ROLE_HEAL) && GetSpec() != BOT_SPEC_PRIEST_SHADOW)
            {
                //戒律系优先为坦克/被点名目标维持盾（新生希望与争分夺秒），随后仍由 ShieldGroup 覆盖其余成员
                if (GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE)
                    MaintainWeakenedSoul(diff);

                if (me->GetMap()->IsRaid())
                {
                    CureGroup(GetSpell(DISPEL_MAGIC_1), diff);
                    CureGroup(GetSpell(ABOLISH_DISEASE_1) ? GetSpell(ABOLISH_DISEASE_1) : GetSpell(CURE_DISEASE_1), diff);
                    MassGroupHeal(diff);
                    ShieldGroup(diff);
                    CheckMending(diff);
                    CheckLightwell(diff);
                    BuffAndHealGroup(diff);
                }
                else
                {
                    MassGroupHeal(diff);
                    ShieldGroup(diff);
                    CheckMending(diff);
                    CheckLightwell(diff);
                    BuffAndHealGroup(diff);
                    CureGroup(GetSpell(DISPEL_MAGIC_1), diff);
                    CureGroup(GetSpell(CURE_DISEASE_1), diff);
                }
            }
            else
            {
                BuffAndHealGroup(diff);
                CureGroup(GetSpell(DISPEL_MAGIC_1), diff);
            }

            if (master->IsInCombat() || me->IsInCombat())
            {
                CheckSilence(diff);
                CheckDispel(diff);
                CheckShadowfiend(diff);
                CheckHymnOfHope(diff);
                CheckManaBurn(diff);
            }

            Counter(diff);

            if (me->IsInCombat())
            {
                CheckShackles(diff);
                CheckPowerInfusion(diff);
            }
            else
                DoNonCombatActions(diff);

            if (IsCasting())
                return;

            //暗影形态维护：纯输出暗影牧战斗中主动进入
            if (IsSpellReady(SHADOWFORM_1, diff) && HasRole(BOT_ROLE_DPS) && !HasRole(BOT_ROLE_HEAL) &&
                me->GetShapeshiftForm() != FORM_SHADOW)
            {
                if (doCast(me, SHADOWFORM_1))
                    return;
            }

            if (ProcessImmediateNonAttackTarget())
                return;

            if (!CheckAttackTarget())
                return;

            CheckUsableItems(diff);

            DoNormalAttack(diff);
        }

        //目标是否仍可用于施法：存在、存活、且与自身同地图
        bool CanCastOnTarget(Unit const* target) const
        {
            return target && target->IsInWorld() && target->IsAlive() && me->GetMap() == target->FindMap();
        }

        //判断自身施放的 DoT 是否需补：光环缺失，或剩余时间 ≤ 1 秒（避免断档）时按概率补
        //randLimit：Rand() 阈值（Rand() 取值 0~100，越大触发概率越高）
        bool AuraNeedsRefresh(AuraEffect const* eff, uint16 randLimit) const
        {
            return (!eff || eff->GetBase()->GetDuration() < 1000) && Rand() < randLimit;
        }

        //光环是否缺失或即将到期（剩余 < remainMs）：用于延长类 DoT/HoT 的提前补刷
        bool AuraExpiringSoon(AuraEffect const* eff, uint32 remainMs) const
        {
            return !eff || eff->GetBase()->GetDuration() < int32(remainMs);
        }

        //目标血量是否足以让长持续 DoT 划算（短命目标跳过）
        bool TargetWorthDotting(Unit const* target) const
        {
            return target->GetHealth() > me->GetMaxHealth() / 4 * (1 + target->getAttackers().size());
        }

        //单体输出入口：控制 → 反射破除 → 按专精分发循环 → 兜底填充（魔杖）
        void DoNormalAttack(uint32 diff)
        {
            Unit* mytar = opponent ? opponent : disttarget ? disttarget : nullptr;
            if (!mytar)
                return;

            StartAttack(mytar, IsMelee());

            CheckAttackState();
            if (!me->IsAlive() || !mytar->IsAlive())
                return;

            MoveBehind(mytar);

            if (GC_Timer > diff)
                return;

            //shadow skills range
            if (me->GetDistance(mytar) > CalcSpellMaxRange(MIND_FLAY_1))
                return;

            auto [can_do_shadow, can_do_holy] = CanAffectVictimBools(mytar, SPELL_SCHOOL_SHADOW, SPELL_SCHOOL_HOLY);

            //心灵恐慌：反制持械近战目标
            if (IsSpellReady(PSYCHIC_HORROR_1, diff) && can_do_shadow && Rand() < 20 &&
                mytar->GetHealth() > me->GetMaxHealth()/8 && !CCed(mytar) &&
                !mytar->HasAuraType(SPELL_AURA_MOD_DISARM) &&
                (mytar->IsPlayer() ?
                mytar->ToPlayer()->GetWeaponForAttack(BASE_ATTACK) && mytar->ToPlayer()->GetWeaponForAttack(WeaponAttackType(BASE_ATTACK), true) :
                mytar->GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID) && mytar->CanUseAttackType(BASE_ATTACK)))
            {
                if (doCast(mytar, GetSpell(PSYCHIC_HORROR_1)))
                    return;
            }

            //spell reflections
            if (IsSpellReady(SW_PAIN_1, diff) && can_do_shadow && CanRemoveReflectSpells(mytar, SW_PAIN_1) &&
                doCast(mytar, SW_PAIN_1)) //yes, using rank 1
                return;

            if (!HasRole(BOT_ROLE_DPS))
                return;

            //暗影魔：蓝量吃紧或爆发期召唤（配合 Veiled Shadows 缩短 CD）
            if (GetSpec() == BOT_SPEC_PRIEST_SHADOW && IsSpellReady(SHADOWFIEND_1, diff) && GetManaPCT(me) < 60)
            {
                SummonBotPet(mytar);
                SetSpellCooldown(SHADOWFIEND_1, 180000); //基础 5 分钟，Veiled Shadows -2 分钟
                return;
            }

            //混合角色的蓝量门控：蓝量过低且无宠物回蓝时暂停输出（保留 1 分蓝量给治疗）
            if (HasRole(BOT_ROLE_HEAL) && GetManaPCT(me) <= 35 && !GetBotsPet())
                return;

            //AOE 输出分支：神圣新星（神圣/戒律，近身多目标）
            if (DoAoeRotation(diff, mytar, can_do_holy))
                return;

            //按专精运行对应的输出优先级
            bool casted = false;
            switch (GetSpec())
            {
                case BOT_SPEC_PRIEST_DISCIPLINE:
                    casted = DoDisciplineRotation(diff, mytar, can_do_shadow, can_do_holy);
                    break;
                case BOT_SPEC_PRIEST_HOLY:
                    casted = DoHolyRotation(diff, mytar, can_do_holy);
                    break;
                case BOT_SPEC_PRIEST_SHADOW:
                    casted = DoShadowRotation(diff, mytar, can_do_shadow);
                    break;
                default:
                    casted = DoShadowRotation(diff, mytar, can_do_shadow);
                    break;
            }

            if (casted)
                return;

            if (DoFiller(diff, mytar, can_do_shadow, can_do_holy))
                return;

            if (Spell const* shot = me->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL))
            {
                if (shot->GetSpellInfo()->Id == SHOOT_WAND && shot->m_targets.GetUnitTarget() != mytar)
                    me->InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
            }
            else if (IsSpellReady(SHOOT_WAND, diff) && !me->isMoving() && me->GetDistance(mytar) < 30 && GetEquips(BOT_SLOT_RANGED) &&
                doCast(mytar, SHOOT_WAND))
                return;
        }

        //AOE 输出分支：神圣新星（神圣/戒律，近身 ≥3 目标且自身血量安全）
        bool DoAoeRotation(uint32 diff, Unit* mytar, bool can_do_holy)
        {
            if (GetSpec() == BOT_SPEC_PRIEST_SHADOW || !can_do_holy)
                return false;
            if (!GetSpell(HOLY_NOVA_1) || !IsSpellReady(HOLY_NOVA_1, diff))
                return false;
            if (GetHealthPCT(me) < 60 || me->GetDistance(mytar) > 10.f)
                return false;

            std::list<Unit*> targets;
            GetNearbyTargetsList(targets, 10.f, 0);
            if (targets.size() < 3)
                return false;

            if (doCast(me, GetSpell(HOLY_NOVA_1)))
                return true;

            return false;
        }

        //兜底填充：暗影形态下用心灵鞭笞，否则用惩击（圣光涌动触发时优先瞬发免蓝）
        bool DoFiller(uint32 diff, Unit* mytar, bool can_do_shadow, bool can_do_holy)
        {
            if (!CanCastOnTarget(mytar))
                return false;

            //圣光涌动：瞬发惩击出口（神圣/戒律）
            if (me->HasAura(SURGE_OF_LIGHT_BUFF) && IsSpellReady(SMITE_1, diff) && can_do_holy &&
                GetSpec() != BOT_SPEC_PRIEST_SHADOW && me->GetDistance(mytar) < CalcSpellMaxRange(SMITE_1))
                return doCast(mytar, GetSpell(SMITE_1));

            if (me->GetShapeshiftForm() == FORM_SHADOW && can_do_shadow && IsSpellReady(MIND_FLAY_1, diff) &&
                me->GetDistance(mytar) < CalcSpellMaxRange(MIND_FLAY_1))
                return doCast(mytar, GetSpell(MIND_FLAY_1));

            //非暗影形态（含 1~20 级尚未学会暗影形态）用惩击输出
            if (can_do_holy && GetSpell(SMITE_1) && GetSpec() != BOT_SPEC_PRIEST_SHADOW &&
                IsSpellReady(SMITE_1, diff) && me->GetDistance(mytar) < CalcSpellMaxRange(SMITE_1))
                return doCast(mytar, GetSpell(SMITE_1));

            return false;
        }

        //戒律系单体：苦修 → 神圣之火 → 惩击（DPS 形态）；带 DoT 时优先痛
        bool DoDisciplineRotation(uint32 diff, Unit* mytar, bool can_do_shadow, bool can_do_holy)
        {
            if (!CanCastOnTarget(mytar))
                return false;

            //暗言术：痛（即时挂，不绑定暗影交织层数）
            if (IsSpellReady(SW_PAIN_1, diff) && can_do_shadow && TargetWorthDotting(mytar) &&
                me->GetDistance(mytar) < CalcSpellMaxRange(SW_PAIN_1) &&
                AuraNeedsRefresh(mytar->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x8000, 0x0, 0x0, me->GetGUID()), 100))
                return doCast(mytar, GetSpell(SW_PAIN_1));

            //苦修（DPS 形态视为神圣伤害输出技能）
            if (IsSpellReady(PENANCE_1, diff) && can_do_holy && me->GetDistance(mytar) < CalcSpellMaxRange(PENANCE_1))
                return doCast(mytar, GetSpell(PENANCE_1));

            //神圣之火（DoT + 直伤）
            if (IsSpellReady(HOLY_FIRE_1, diff) && can_do_holy && me->GetDistance(mytar) < CalcSpellMaxRange(HOLY_FIRE_1))
                return doCast(mytar, GetSpell(HOLY_FIRE_1));

            //圣光涌动瞬发惩击
            if (me->HasAura(SURGE_OF_LIGHT_BUFF) && IsSpellReady(SMITE_1, diff) && can_do_holy &&
                me->GetDistance(mytar) < CalcSpellMaxRange(SMITE_1))
                return doCast(mytar, GetSpell(SMITE_1));

            return false;//交由 DoNormalAttack 统一兜底填充
        }

        //神圣系单体：神圣之火 → 惩击（圣光涌动瞬发优先）；非暗影形态
        bool DoHolyRotation(uint32 diff, Unit* mytar, bool can_do_holy)
        {
            if (!CanCastOnTarget(mytar))
                return false;

            //神圣之火
            if (IsSpellReady(HOLY_FIRE_1, diff) && can_do_holy && me->GetDistance(mytar) < CalcSpellMaxRange(HOLY_FIRE_1) &&
                AuraNeedsRefresh(mytar->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x1, 0x0, 0x0, me->GetGUID()), 95))
                return doCast(mytar, GetSpell(HOLY_FIRE_1));

            //圣光涌动瞬发惩击
            if (me->HasAura(SURGE_OF_LIGHT_BUFF) && IsSpellReady(SMITE_1, diff) && can_do_holy &&
                me->GetDistance(mytar) < CalcSpellMaxRange(SMITE_1))
                return doCast(mytar, GetSpell(SMITE_1));

            return false;//交由 DoNormalAttack 统一兜底填充
        }

        //暗影系单体：吸血鬼之触 → 痛 → 噬灵疫病 → 心灵震爆 → 灭(斩杀) → 心灵灼烧(AOE) → 心灵鞭笞填充
        bool DoShadowRotation(uint32 diff, Unit* mytar, bool can_do_shadow)
        {
            if (!CanCastOnTarget(mytar))
                return false;

            //1) 吸血鬼之触（回蓝 + DoT，优先）
            if (IsSpellReady(VAMPIRIC_TOUCH_1, diff) && can_do_shadow && me->GetDistance(mytar) < CalcSpellMaxRange(VAMPIRIC_TOUCH_1) &&
                TargetWorthDotting(mytar) &&
                AuraNeedsRefresh(mytar->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x0, 0x400, 0x0, me->GetGUID()), 100))
                return doCast(mytar, GetSpell(VAMPIRIC_TOUCH_1));

            //2) 暗言术：痛（立即挂，不绑定暗影交织层数：短血目标/爆发期也要保持）
            if (IsSpellReady(SW_PAIN_1, diff) && can_do_shadow && me->GetDistance(mytar) < CalcSpellMaxRange(SW_PAIN_1) &&
                mytar->GetHealth() > me->GetMaxHealth()/2 * (1 + mytar->getAttackers().size()) &&
                AuraNeedsRefresh(mytar->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x8000, 0x0, 0x0, me->GetGUID()), 100))
                return doCast(mytar, GetSpell(SW_PAIN_1));

            //3) 噬灵疫病（暗影系专属，避开免疫感染目标）
            if (IsSpellReady(DEVOURING_PLAGUE_1, diff) && can_do_shadow && !Devcheck &&
                (GetSpec() == BOT_SPEC_PRIEST_SHADOW || mytar->IsControlledByPlayer()) &&
                me->GetDistance(mytar) < CalcSpellMaxRange(DEVOURING_PLAGUE_1) &&
                mytar->GetHealth() > me->GetMaxHealth()/2 * (1 + mytar->getAttackers().size()) &&
                !(mytar->IsCreature() && (mytar->ToCreature()->HasMechanicTemplateImmunity(1ULL << MECHANIC_INFECTED))) &&
                AuraNeedsRefresh(mytar->GetAuraEffect(SPELL_AURA_PERIODIC_LEECH, SPELLFAMILY_PRIEST, 0x02000000, 0x0, 0x0, me->GetGUID()), 100))
                return doCast(mytar, GetSpell(DEVOURING_PLAGUE_1));

            //4) 暗言术：灭 斩杀期（反噬安全：自身血量 > 50%）
            if (IsSpellReady(SW_DEATH_1, diff) && can_do_shadow && GetHealthPCT(me) > 50 &&
                me->GetDistance(mytar) < CalcSpellMaxRange(SW_DEATH_1) &&
                (me->GetMap()->IsRaid() || GetHealthPCT(mytar) < 15 || mytar->GetHealth() < me->GetMaxHealth()/8) &&
                Rand() < 90)
                return doCast(mytar, GetSpell(SW_DEATH_1));

            //5) 心灵震爆（卡 CD）
            if (IsSpellReady(MIND_BLAST_1, diff) && can_do_shadow && me->GetDistance(mytar) < CalcSpellMaxRange(MIND_BLAST_1))
                return doCast(mytar, GetSpell(MIND_BLAST_1));

            //6) 心灵灼烧（≥3 目标聚团）
            if (IsSpellReady(MIND_SEAR_1, diff) && can_do_shadow && (!me->isMoving() || Rand() < 80) &&
                mytar->GetVictim() && mytar->GetVictim()->getAttackers().size() > 3)
            {
                if (Unit* u = FindSplashTarget(CalcSpellMaxRange(MIND_SEAR_1), mytar, 14.f, 3)) //glyphed, cluster of 4
                    if (doCast(u, GetSpell(MIND_SEAR_1)))
                        return true;
            }

            //7) 填充：心灵鞭笞（暗牧基础填充，移除 HEAL 角色耦合）
            if (IsSpellReady(MIND_FLAY_1, diff) && can_do_shadow && me->GetDistance(mytar) < CalcSpellMaxRange(MIND_FLAY_1))
                return doCast(mytar, GetSpell(MIND_FLAY_1));

            return false;//交由 DoNormalAttack 统一兜底填充
        }

        bool HealTarget(Unit* target, uint32 diff) override
        {
            if (!target || !target->IsAlive() || target->GetShapeshiftForm() == FORM_SPIRITOFREDEMPTION || me->GetDistance(target) > 40)
                return false;

            uint8 hp = GetHealthPCT(target);
            if (hp > GetHealHpPctThreshold())
                return false;
            bool pointed = IsPointedHealTarget(target);
            if (hp > 90 && !(pointed && me->GetMap()->IsRaid()) &&
                (!target->IsInCombat() || target->getAttackers().empty() || !IsTank(target) || !me->GetMap()->IsRaid()))
                return false;

            //掉血速率外推：按 2.0~2.5 秒窗口预判，攻击者越多掉血越快
            int32 hps = int32(GetHPS(target) * (1.f + target->getAttackers().size() * 0.3f));
            int32 xphp = target->GetHealth() + hps * (me->GetLevel() < 60 ? 2.5f : 2.0f);
            int32 hppctps = int32(hps * 100.f / float(target->GetMaxHealth()));
            int32 xphploss = xphp > int32(target->GetMaxHealth()) ? 0 : abs(int32(xphp - target->GetMaxHealth()));
            int32 xppct = hp + hppctps * (me->GetLevel() < 60 ? 2.5f : 2.0f);
            //BOT_LOG_ERROR("entities.player", "priest_bot:HealTarget(): %s's pct %u, hppctps %i, epct %i",
            //    target->GetName().c_str(), uint32(hp), int32(hppctps), int32(xppct));
            if (xppct >= 95 && hp >= 25 && !pointed)
                return false;

            //GUARDIAN SPIRIT no GCD
            if (IsSpellReady(GUARDIAN_SPIRIT_1, diff, false) && !IAmFree() && target->IsInCombat() && !target->getAttackers().empty() &&
                (xppct <= 0 || (hp <= 50 && hppctps <= -15) ||
                (me->GetMap()->Instanceable() && target->GetMaxHealth() > me->GetMaxHealth() << 5)) &&
                !target->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_HEALING_PCT, SPELLFAMILY_PRIEST, 0x40000000))
            {
                me->InterruptNonMeleeSpells(false);
                if (doCast(target, GetSpell(GUARDIAN_SPIRIT_1)))
                {
                    if (target->IsPlayer())
                        ReportSpellCast(GUARDIAN_SPIRIT_1, LocalizedNpcText(target->ToPlayer(), BOT_TEXT__ON_YOU), target->ToPlayer());

                    if (!IAmFree() && target != master)
                    {
                        std::string msg = target == me ? LocalizedNpcText(master, BOT_TEXT__ON_MYSELF) : (LocalizedNpcText(master, BOT_TEXT__ON_) + target->GetName() + '!');
                        ReportSpellCast(GUARDIAN_SPIRIT_1, msg, master);
                    }
                    //return true;
                }
            }

            //PAIN SUPPRESSION
            if (IsSpellReady(PAIN_SUPPRESSION_1, diff, false) && xppct >= 5 && hp >= 25 && hp <= 55 && hppctps <= -10 &&
                Rand() < 80 && !target->getAttackers().empty() &&
                !target->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_DISPEL_RESIST, SPELLFAMILY_PRIEST, 0x80000000) &&
                !target->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_HEALING_PCT, SPELLFAMILY_PRIEST, 0x40000000))
            {
                me->InterruptNonMeleeSpells(false);
                if (doCast(target, GetSpell(PAIN_SUPPRESSION_1)))
                {
                    if (target->IsPlayer())
                        ReportSpellCast(PAIN_SUPPRESSION_1, LocalizedNpcText(target->ToPlayer(), BOT_TEXT__ON_YOU), target->ToPlayer());

                    if (!IAmFree() && target != master)
                    {
                        std::string msg = target == me ? LocalizedNpcText(master, BOT_TEXT__ON_MYSELF) : (LocalizedNpcText(master, BOT_TEXT__ON_) + target->GetName() + '!');
                        ReportSpellCast(PAIN_SUPPRESSION_1, msg, master);
                    }
                    return true;
                }
            }

            if (target == me && IsSpellReady(DESPERATE_PRAYER_1, diff) && hp <= 50 && Rand() < 45 &&
                int32(GetLostHP(me)) > GetHealAmount(DESPERATE_PRAYER_1))
            {
                me->InterruptNonMeleeSpells(false);
                if (doCast(target, GetSpell(DESPERATE_PRAYER_1)))
                    return true;
            }

            if (IsCasting())
                return false;

            Unit const* u = target->GetVictim();
            bool tanking = u && IsTank(target) && u->IsCreature() && u->ToCreature()->isWorldBoss();
            //联结治疗可用性：神圣系天赋，且自身也处于低血（收益翻倍）
            bool canBinding = GetSpec() == BOT_SPEC_PRIEST_HOLY && GetSpell(BINDING_HEAL_1) &&
                GetHealthPCT(me) < hp + 10 && me->GetDistance(target) < CalcSpellMaxRange(BINDING_HEAL_1);

            //联结治疗：自己与目标同时低血（按目标最高效的单体治疗）
            if (canBinding && IsSpellReady(BINDING_HEAL_1, diff) && hp <= 75 &&
                xphploss > GetHealAmount(BINDING_HEAL_1) &&
                doCast(target, GetSpell(BINDING_HEAL_1)))
                return true;

            //苦修（戒律治疗核心：快速高效）
            if (IsSpellReady(PENANCE_1, diff) && !target->IsCharmed() && !target->isPossessed() && hp <= 80 &&
                Rand() < 90 && xphploss > GetHealAmount(PENANCE_1) &&
                doCast(target, GetSpell(PENANCE_1)))
                return true;

            //神圣系：仅在法力充足且目标未来仍有较大缺血时用快速治疗叠好运
            if (GetSpec() == BOT_SPEC_PRIEST_HOLY && GetManaPCT(me) > 60 && IsSpellReady(FLASH_HEAL_1, diff) &&
                IsSpellReady(HEAL, diff) && hp > 40 && hp <= 60 && !me->HasAura(SERENDIPITY_BUFF) &&
                xphploss > GetHealAmount(HEAL) && Rand() < 60 &&
                doCast(target, GetSpell(FLASH_HEAL_1)))
                return true;

            //强效治疗术
            if (IsSpellReady(HEAL, diff) && (xppct > 15 || !GetSpell(FLASH_HEAL_1)) && (tanking || xphploss > GetHealAmount(HEAL)))
            {
                if (me->IsInCombat() && IsSpellReady(INNER_FOCUS_1, diff) && GetManaPCT(me) < 70 &&
                    doCast(me, GetSpell(INNER_FOCUS_1)))
                {}
                if (doCast(target, GetSpell(HEAL)))
                    return true;
            }
            //恢复：新施放需缺血，提前续补另需持续掉血或更低血量
            AuraEffect const* renew = target->GetAuraEffect(
                SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_PRIEST, 0x40, 0x0, 0x0, me->GetGUID());
            if (IsSpellReady(RENEW_1, diff) && hp < (IsTank(target) ? 90 : 85) && xppct < 95 &&
                (!renew || hp < (IsTank(target) ? 85 : 75) || hps < 0) &&
                (tanking || !target->getAttackers().empty() || me->GetMap()->IsDungeon()) &&
                AuraExpiringSoon(renew, 3000))
            {
                if (doCast(target, GetSpell(RENEW_1)))
                    return true;
            }
            //快速治疗
            if (IsSpellReady(FLASH_HEAL_1, diff) && xphploss > GetHealAmount(FLASH_HEAL_1))
            {
                if (doCast(target, GetSpell(FLASH_HEAL_1)))
                    return true;
            }

            return false;
        }

        //估算指定治疗法术的治疗量（G3：不使用初始化快照，改为动态估算）
        //法强取缓存值（GetBotSpellPower，随装备/等级更新而非每 tick 计算），
        //按 3.5 秒标准施法时间的系数折算，即：基础治疗 + 法强 * 施法时长 / 3500
        int32 GetHealAmount(uint32 spellId) const
        {
            uint32 spell = InitSpell(me, spellId);
            if (!spell)
                return 0;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell);
            if (!spellInfo)
                return 0;

            int32 base = spellInfo->Effects[0].CalcValue(me);
            int32 castTime = spellInfo->CastTimeEntry ? spellInfo->CastTimeEntry->CastTime : 0;
            float coef = castTime > 0 ? std::min(1.f, float(castTime) / 3500.f) : 0.f;

            return base + int32(GetBotSpellPower() * coef);
        }

        bool BuffTarget(Unit* target, uint32 diff) override
        {
            if (IsSpellReady(FEAR_WARD_1, diff) && (!IAmFree() || target == me) &&
                !target->HasAuraTypeWithMiscvalue(SPELL_AURA_MECHANIC_IMMUNITY, MECHANIC_FEAR) &&
                doCast(target, GetSpell(FEAR_WARD_1)))
                return true;

            if (target == me)
            {
                if (GetSpell(INNER_FIRE_1) &&
                    !me->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_RESISTANCE, SPELLFAMILY_PRIEST, 0x2) &&
                    doCast(me, GetSpell(INNER_FIRE_1)))
                    return true;
                if (HasRole(BOT_ROLE_DPS) && GetSpell(VAMPIRIC_EMBRACE_1) &&
                    !me->HasAuraTypeWithFamilyFlags(SPELL_AURA_DUMMY, SPELLFAMILY_PRIEST, 0x4) &&
                    doCast(me, GetSpell(VAMPIRIC_EMBRACE_1)))
                    return true;
            }

            if ((me->IsInCombat() || !CanDoNonCombatActions()) && !master->GetMap()->IsRaid())
                return false;

            if (uint32 PW_FORTITUDE = GetSpell(PW_FORTITUDE_1))
            {
                if (!target->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_STAT, SPELLFAMILY_PRIEST, 0x8) &&
                    doCast(target, PW_FORTITUDE))
                    return true;
            }
            if (uint32 SHADOW_PROTECTION = GetSpell(SHADOW_PROTECTION_1))
            {
                if (!target->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE, SPELLFAMILY_PRIEST, 0x100) &&
                    doCast(target, SHADOW_PROTECTION))
                    return true;
            }
            if (uint32 DIVINE_SPIRIT = GetSpell(DIVINE_SPIRIT_1))
            {
                if ((target->GetMaxPower(POWER_MANA) > 1) &&
                    !target->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_STAT, SPELLFAMILY_PRIEST, 0x20) &&
                    doCast(target, DIVINE_SPIRIT))
                    return true;
            }

            return false;
        }

        void DoNonCombatActions(uint32 diff)
        {
            if (GC_Timer > diff || me->IsMounted() || IsCasting())
                return;

            ResurrectGroup(GetSpell(RESURRECTION_1));

            if (GetSpell(LEVITATE_1) && !IAmFree() && Rand() < 30)
            {
                Group const* gr = master->GetGroup();
                if (gr)
                {
                    for (GroupReference const* ref = gr->GetFirstMember(); ref != nullptr; ref = ref->next())
                    {
                        Player* pl = ref->GetSource();
                        if (pl && pl->IsAlive() && pl->FindMap() == me->GetMap() && pl->GetDistance(me) < 30 &&
                            pl->IsFalling() && pl->m_movementInfo.fallTime > 1000 &&
                            !pl->HasAuraType(SPELL_AURA_HOVER))
                        {
                            if (doCast(pl, GetSpell(LEVITATE_1)))
                                return;
                        }
                    }
                }
                else if (master->IsAlive() && master->GetDistance(me) < 30 && master->IsFalling() &&
                    master->m_movementInfo.fallTime > 1000 && !master->HasAuraType(SPELL_AURA_HOVER))
                {
                    if (doCast(master, GetSpell(LEVITATE_1)))
                        return;
                }
            }
        }

        void Counter(uint32 diff)
        {
            if (ShackcheckTimer > diff || Shackcheck || Rand() > 65 ||
                (HasRole(BOT_ROLE_HEAL) && (IsCasting() || GetManaPCT(me) < 20)))
                return;

            //always glyphed so <= 0.5 sec cast time
            if (IsSpellReady(SHACKLE_UNDEAD_1, diff, false) && !HasQueuedSpellAction(SHACKLE_UNDEAD_1))
                if (Unit const* target = FindCastingTarget(CalcSpellMaxRange(SHACKLE_UNDEAD_1), 0, SHACKLE_UNDEAD_1))
                    if (EnqueueCounterSpellAction(target->GetGUID(), SHACKLE_UNDEAD_1, true))
                        return;
        }

        void CheckDispel(uint32 diff)
        {
            if (HasRole(BOT_ROLE_HEAL) && !HasRole(BOT_ROLE_DPS))
                return;

            if (DispelcheckTimer > diff || IsCasting() || Rand() > 35)
                return;

            DispelcheckTimer = urand(750, 1000);

            uint32 DM = GetSpell(DISPEL_MAGIC_1);
            uint32 MD = (GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) ? GetSpell(MASS_DISPEL_1) : 0;

            if (!DM && !MD)
                return;

            if (Unit* target = FindHostileDispelTarget(CalcSpellMaxRange(DISPEL_MAGIC_1)))
            {
                uint32 dm = DM && !target->HasAuraWithMechanic(1u<<MECHANIC_IMMUNE_SHIELD) ? DM : MD;
                if (dm && doCast(target, dm))
                    return;
            }
        }

        void CheckMending(uint32 diff)
        {
            if (Mend_Timer > diff || !IsSpellReady(PRAYER_OF_MENDING_1, diff) || !HasRole(BOT_ROLE_HEAL) || IsCasting() || Rand() > 75)
                return;

            Mend_Timer = urand(1000, 3000);

            Group const* gr = !IAmFree() ? master->GetGroup() : GetGroup();
            if (!gr)
                return;

            uint32 MENDING_AURA = InitSpell(me, PRAYER_OF_MENDING_AURA_1);
            if (FindAffectedTarget(MENDING_AURA, me->GetGUID(), 70, 4))
                return;

            for (Unit* member : BotMgr::GetAllGroupMembers(gr))
            {
                if (me->GetMap() == member->FindMap() && member->IsAlive() && !member->getAttackers().empty() &&
                    (IsTank(member) || GetBG()) && GetHealthPCT(member) < 85 && me->IsWithinDistInMap(member, 40) &&
                    !member->HasAuraType(SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE))
                {
                    if (doCast(member, GetSpell(PRAYER_OF_MENDING_1)))
                        return;
                }
            }
        }

        void CheckShackles(uint32 diff)
        {
            if (Shackle_Timer > diff || !IsSpellReady(SHACKLE_UNDEAD_1, diff) || IsCasting() || Rand() > 50)
                return;

            Shackle_Timer = 500;

            if (FindAffectedTarget(GetSpell(SHACKLE_UNDEAD_1), me->GetGUID(), 60, 255))
                return;
            Unit* target = FindUndeadCCTarget(CalcSpellMaxRange(SHACKLE_UNDEAD_1), SHACKLE_UNDEAD_1);
            if (target && doCast(target, GetSpell(SHACKLE_UNDEAD_1)))
            {}
        }

        void CheckSilence(uint32 diff)
        {
            if (IsCasting() || Rand() > 40)
                return;

            if (IsSpellReady(SILENCE_1, diff, false))
            {
                if (Unit* target = FindCastingTarget(CalcSpellMaxRange(SILENCE_1), 0, SILENCE_1))
                    if (doCast(target, GetSpell(SILENCE_1)))
                        return;
            }
            if (IsSpellReady(PSYCHIC_HORROR_1, diff))
            {
                if (Unit* target = FindCastingTarget(CalcSpellMaxRange(PSYCHIC_HORROR_1), 0, PSYCHIC_HORROR_1))
                    if (doCast(target, GetSpell(PSYCHIC_HORROR_1)))
                        return;
            }
        }

        void CheckPowerInfusion(uint32 diff)
        {
            if (!IsSpellReady(POWER_INFUSION_1, diff, false) || IsCasting() || Rand() > 25)
                return;

            if (IAmFree())
            {
                if (me->GetVictim() && GetManaPCT(me) < 95 &&
                    doCast(me, GetSpell(POWER_INFUSION_1)))
                    return;

                return;
            }

            Group const* gr = master->GetGroup();
            Unit* u = nullptr;
            if (!gr)
            {
                u = master;
                if (u->IsAlive() && u->IsInWorld() && u->GetPowerType() == POWER_MANA && u->GetVictim() && !IsTank(u) &&
                    GetManaPCT(u) < 70 && me->IsWithinDistInMap(u, 30) &&
                    !u->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, SPELLFAMILY_PRIEST, 0x80000000) &&
                    doCast(u, GetSpell(POWER_INFUSION_1)))
                    return;

                for (auto const& [_, bot] : *master->GetBotMgr()->GetBotMap())
                {
                    u = bot;
                    if (u->IsAlive() && u->IsInWorld() && u->ToCreature()->GetBotAI()->HasRole(BOT_ROLE_HEAL) &&
                        u->ToCreature()->GetBotClass() < BOT_CLASS_EX_START &&
                        GetManaPCT(u) < 70 && me->IsWithinDistInMap(u, 30) &&
                        !u->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, SPELLFAMILY_PRIEST, 0x80000000) &&
                        doCast(u, GetSpell(POWER_INFUSION_1)))
                        return;
                }
                for (auto const& [_, bot] : *master->GetBotMgr()->GetBotMap())
                {
                    u = bot;
                    if (u->IsAlive() && u->IsInWorld() && u->GetPowerType() == POWER_MANA && u->GetVictim() && !IsTank(u) &&
                        u->ToCreature()->GetBotClass() < BOT_CLASS_EX_START &&
                        GetManaPCT(u) < 70 && me->IsWithinDistInMap(u, 30) &&
                        !u->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, SPELLFAMILY_PRIEST, 0x80000000) &&
                        doCast(u, GetSpell(POWER_INFUSION_1)))
                        return;
                }

                return;
            }

            for (GroupReference const* itr = gr->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                u = itr->GetSource();
                if (u && u->IsAlive() && u->IsInWorld() && u->GetPowerType() == POWER_MANA && u->GetVictim() && !IsTank(u) &&
                    GetManaPCT(u) < 70 && me->IsWithinDistInMap(u, 30) &&
                    !u->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, SPELLFAMILY_PRIEST, 0x80000000) &&
                    doCast(u, GetSpell(POWER_INFUSION_1)))
                    return;
            }
            for (GroupReference const* itr = gr->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player const* player = itr->GetSource();
                if (!player || !player->IsInWorld() || me->GetMap() != player->FindMap() || !player->HaveBot())
                    continue;
                for (auto const& [_, bot] : *player->GetBotMgr()->GetBotMap())
                {
                    u = bot;
                    if (u->IsAlive() && u->IsInWorld() && u->ToCreature()->GetBotAI()->HasRole(BOT_ROLE_HEAL) &&
                        !BotDataMgr::IsHeroExClass(u->ToCreature()->GetBotClass()) &&
                        GetManaPCT(u) < 70 && me->IsWithinDistInMap(u, 30) &&
                        !u->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, SPELLFAMILY_PRIEST, 0x80000000) &&
                        doCast(u, GetSpell(POWER_INFUSION_1)))
                        return;
                }
            }
            for (GroupReference const* itr = gr->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player const* player = itr->GetSource();
                if (!player || !player->IsInWorld() || me->GetMap() != player->FindMap() || !player->HaveBot())
                    continue;
                for (auto const& [_, bot] : *player->GetBotMgr()->GetBotMap())
                {
                    u = bot;
                    if (u->IsAlive() && u->IsInWorld() && u->GetPowerType() == POWER_MANA && u->GetVictim() && !IsTank(u) &&
                        u->ToCreature()->GetBotClass() < BOT_CLASS_EX_START &&
                        GetManaPCT(u) < 70 && me->IsWithinDistInMap(u, 30) &&
                        !u->HasAuraTypeWithFamilyFlags(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, SPELLFAMILY_PRIEST, 0x80000000) &&
                        doCast(u, GetSpell(POWER_INFUSION_1)))
                        return;
                }
            }

            SetSpellCooldown(POWER_INFUSION_1, 1500); //fail
        }

        void doDefend(uint32 diff)
        {
            if (Rand() > 50) return;

            Unit::AttackerSet const& m_attackers = master->getAttackers();
            Unit::AttackerSet const& b_attackers = me->getAttackers();

            //fear master's attackers
            if (IsSpellReady(PSYCHIC_SCREAM_1, diff))
            {
                if (!m_attackers.empty() && (!IsTank(master) || GetHealthPCT(master) < 75))
                {
                    uint8 tCount = 0;
                    for (Unit const* attacker : m_attackers)
                    {
                        if (!attacker) continue;
                        if (attacker->ToCreature() && attacker->GetCreatureType() == CREATURE_TYPE_UNDEAD) continue;
                        if (me->GetExactDistSq(attacker) > 7*7) continue;
                        if (CCed(attacker) && me->GetExactDistSq(attacker) > 5*5) continue;
                        if (me->IsValidAttackTarget(attacker))
                            ++tCount;
                    }
                    if (tCount > 1 && doCast(me, GetSpell(PSYCHIC_SCREAM_1)))
                        return;
                }

                // Defend myself (psychic horror)
                if (!b_attackers.empty())
                {
                    uint8 tCount = 0;
                    for (Unit const* attacker : b_attackers)
                    {
                        if (!attacker) continue;
                        if (attacker->ToCreature() && attacker->GetCreatureType() == CREATURE_TYPE_UNDEAD) continue;
                        if (me->GetExactDistSq(attacker) > 7*7) continue;
                        if (CCed(attacker) && me->GetExactDistSq(attacker) > 5*5) continue;
                        if (me->IsValidAttackTarget(attacker))
                            ++tCount;
                    }
                    if (tCount > 0 && doCast(me, GetSpell(PSYCHIC_SCREAM_1)))
                        return;
                }
            }
            // Heal myself（保持统一的套盾判定：虚弱灵魂期间不再重复套盾）
            if ((GetHealthPCT(me) < 95 && !b_attackers.empty()) || (IsWanderer() && IsFlagCarrier(me)))
            {
                if (!me->HasAura(WEAKENED_SOUL_DEBUFF) && ShieldTarget(me, diff)) return;

                if (IsSpellReady(FADE_1, diff) && me->IsInCombat() && !b_attackers.empty())
                {
                    uint8 Tattackers = 0;
                    for (Unit const* attacker : b_attackers)
                    {
                        if (!attacker) continue;
                        if (!attacker->IsAlive()) continue;
                        if (!(attacker)->CanHaveThreatList()) continue;
                        if (me->GetExactDistSq((attacker)) < 15*15)
                            Tattackers++;
                    }
                    if (Tattackers > 0)
                    {
                        if (doCast(me, GetSpell(FADE_1)))
                            return;
                    }
                }
            }
        }

        void DoDevCheck(uint32 diff)
        {
            if (DevcheckTimer <= diff)
            {
                DevcheckTimer = 1000;
                Devcheck = GetSpell(DEVOURING_PLAGUE_1) && FindAffectedTarget(GetSpell(DEVOURING_PLAGUE_1), me->GetGUID(), 70);
            }
        }

        void DoShackCheck(uint32 diff)
        {
            if (ShackcheckTimer <= diff)
            {
                ShackcheckTimer = 1000;
                Shackcheck = GetSpell(SHACKLE_UNDEAD_1) && FindAffectedTarget(GetSpell(SHACKLE_UNDEAD_1), me->GetGUID(), 70);
            }
        }

        void Disperse(uint32 diff)
        {
            if (me->GetVehicle())
                return;
            if (!IsSpellReady(DISPERSION_1, diff) || !me->IsInCombat() || HasRole(BOT_ROLE_HEAL) || IsCasting() || Rand() > 60)
                return;
            if ((me->getAttackers().size() > 3 && !IsSpellReady(FADE_1, diff, false) && GetHealthPCT(me) < 90) ||
                (GetHealthPCT(me) < 20 && (me->HasAuraType(SPELL_AURA_PERIODIC_DAMAGE) || !me->getAttackers().empty())) ||
                (GetManaPCT(me) < 35 && !IsPotionReady()) ||
                (me->getAttackers().size() > 1 && (CCed(me, true) || me->HasAuraWithMechanic(1u<<MECHANIC_SNARE))))
            {
                if (doCast(me, GetSpell(DISPERSION_1)))
                    return;
            }

            SetSpellCooldown(DISPERSION_1, 500); //fail
        }

        void ApplyClassSpellCritMultiplierAll(Unit const* victim, float& crit_chance, SpellInfo const* spellInfo, SpellSchoolMask schoolMask, WeaponAttackType /*attackType*/) const override
        {
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            uint8 lvl = me->GetLevel();

            //Inner Focus
            if (AuraEffect const* focu = me->GetAuraEffect(INNER_FOCUS_1, 0))
                if (focu->IsAffectedOnSpell(spellInfo))
                    crit_chance += 25.f;

            //Benediction (23236)
            if (lvl >= 60 && (schoolMask & SPELL_SCHOOL_MASK_HOLY))
                crit_chance += 2.f;
            //Increased Prayer of Healing Criticals (23550): 25% additional critical chance for Prayer of Healing
            if (lvl >= 60 && baseId == PRAYER_OF_HEALING_1)
                crit_chance += 25.f;
            //Item - Priest T9 Shadow 4P Bonus (67198)
            if (lvl >= 80 && baseId == MIND_FLAY_DAMAGE)
                crit_chance += 5.f;

            //Holy Specialization: 5% additional critical chance for Holy spells
            if (lvl >= 10 && (schoolMask & SPELL_SCHOOL_MASK_HOLY))
                crit_chance += 5.f;
            //Mind Melt (part 1): 4% additional critical chance for Mind Blast, Mind Flay and Mind Sear
            if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) &&
                lvl >= 35 && ((spellInfo->SpellFamilyFlags[0] & 0x802000) || (spellInfo->SpellFamilyFlags[1] & 0x80000)))
                crit_chance += 4.f;
            //Mind Melt (part 2): 6% additional critical chance for Vampiric Touch, Devouring Plague and SW: Pain
            if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) &&
                lvl >= 35 && ((spellInfo->SpellFamilyFlags[0] & 0x2008000) || (spellInfo->SpellFamilyFlags[1] & 0x400)))
                crit_chance += 6.f;
            //Improved Flash Heal (part 2): 10% additional critical chance on targets at or below 50% hp for Flash Heal
            if ((GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) && lvl >= 40 && baseId == FLASH_HEAL_1 && GetHealthPCT(victim) <= 50)
                crit_chance += 10.f;
            //Renewed Hope part 1: 4% additional critical chance on targets affected by Weakened Soul for Flash Heal, Greater Heal and Penance (Heal)
            if ((GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) &&
                lvl >= 45 && (baseId == FLASH_HEAL_1 || baseId == HEAL || baseId == PENANCE_HEAL_1) &&
                victim->HasAuraTypeWithFamilyFlags(SPELL_AURA_MECHANIC_IMMUNITY, SPELLFAMILY_PRIEST, 0x20000000))
                crit_chance += 4.f;
            //Glyph of Shadow Word: Pain (G7): +10% additional critical chance for SW: Pain
            if (lvl >= 15 && baseId == SW_PAIN_1)
                crit_chance += 10.f;
        }

        void ApplyClassDamageMultiplierSpell(int32& damage, SpellNonMeleeDamage& damageinfo, SpellInfo const* spellInfo, WeaponAttackType /*attackType*/, bool iscrit) const override
        {
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            uint8 lvl = me->GetLevel();
            float fdamage = float(damage);

            //apply bonus damage mods
            float pctbonus = 0.0f;
            if (iscrit)
            {
                //!!!spell damage is not yet critical and will be multiplied by 1.5
                //so we should put here bonus damage mult /1.5
                //Shadow Power: 50% additional crit damage bonus for Mind Blast, Mind Flay and SW:Death
                if (lvl >= 40 &&
                    (baseId == MIND_BLAST_1 || baseId == MIND_FLAY_DAMAGE || baseId == SW_DEATH_1))
                    pctbonus += 0.333f;
                //Shadowform crit damage increase
                if (me->GetShapeshiftForm() == FORM_SHADOW &&
                    (baseId == SW_PAIN_1 || baseId == DEVOURING_PLAGUE_1 || baseId == VAMPIRIC_TOUCH_1))
                    pctbonus += 0.333f;
            }
            //Improved Mind Flay and Smite (37571)
            if (lvl >= 10 && (baseId == MIND_FLAY_DAMAGE || baseId == SMITE_1))
                pctbonus += 0.05f;
            //Item - Priest T8 Shadow 2P Bonus (64906)
            if (lvl >= 80 && ((baseId == DEVOURING_PLAGUE_1) || (spellInfo->SpellFamilyFlags[2] & 0x8)))
                pctbonus += 0.15f;

            //Twin Disciplines (damage part): 5% bonus damage for instant spells
            if (lvl >= 10 && !spellInfo->CastTimeEntry)
                pctbonus += 0.05f;
            //Darkness: 10% bonus damage for shadow spells
            if (lvl >= 10 && (spellInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_SHADOW))
                pctbonus += 0.1f;
            //Improved Shadow Word: Pain: 6% bonus damage for Shadow Word: Pain
            if (lvl >= 15 && baseId == SW_PAIN_1)
                pctbonus += 0.46f;
            //Focused Power part 1: 4% bonus damage for all spells
            if ((GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) && lvl >= 35)
                pctbonus += 0.04f;
            //Improved Devouring Plague part 1: 15% bonus damage Devouring Plague
            if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) && lvl >= 35 && baseId == DEVOURING_PLAGUE_1)
                pctbonus += 0.55f;
            //Shadowform: 15% bonus damage for shadow spells (handled)
            //if (lvl >= 40 && (spellInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_SHADOW) && me->GetShapeshiftForm() == FORM_SHADOW)
            //    pctbonus += 0.15f;
            //Misery part 3: 15% bonus damage (from spellpower) for Mind Blast, Mind Flay and Mind Sear
            if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) && lvl >= 45)
            {
                if (baseId == MIND_BLAST_1 || baseId == MIND_FLAY_DAMAGE || baseId == MIND_SEAR_DAMAGE_1)
                    fdamage += me->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_MAGIC) * 0.15f * me->CalculateDefaultCoefficient(spellInfo, DIRECT_DAMAGE) * me->CalculateLevelPenalty(spellInfo);
            }

            //If target is affected BY SW: Pain
            if (lvl >= 20 && (baseId == MIND_BLAST_1 || baseId == MIND_FLAY_DAMAGE) && damageinfo.target &&
                damageinfo.target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_PRIEST, 0x8000, 0x0, 0x0, me->GetGUID()))
            {
                //Glyph of Mind Flay: 10% damage bonus for Mind Flay
                if (baseId == MIND_FLAY_DAMAGE)
                    pctbonus += 0.1f;
                //Twisted Faith (part 1): 10% bonus damage for Mind Blast and Mind Flay
                if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) && lvl >= 55)
                    pctbonus += 0.1f;
            }

            //Glyph of Shadow Word: Death: 10% bonus damage for Shadow Word: Death on targets below 35% health
            if (lvl >= 62 && baseId == SW_DEATH_1 && damageinfo.target->HasAuraState(AURA_STATE_HEALTHLESS_35_PERCENT))
                pctbonus += 0.1f;

            //other
            if (baseId == SW_DEATH_BACKLASH)
            {
                //not affected by +%talents
                pctbonus = 1.f;
                ////T13 Shadow 2P Bonus (Shadow Word: Death), part 2
                //if (lvl >= 60) //buffed
                //    pctbonus -= 0.95f;
                //Pain and Suffering (part 2): 30% reduced backlash damage
                if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) && lvl >= 50)
                    pctbonus -= 0.3f;
            }

            damage = int32(fdamage * (1.0f + pctbonus));
        }

        void ApplyClassDamageMultiplierHeal(Unit const* victim, float& heal, SpellInfo const* spellInfo, DamageEffectType damagetype, uint32 stack) const override
        {
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            uint8 lvl = me->GetLevel();
            float pctbonus = 0.0f;
            float flat_mod = 0.0f;

            //pct mods
            //Improved Greater Heal (38411): 5% bonus healing for Greater Heal
            if (lvl >= 60 && baseId == HEAL)
                pctbonus += 0.05f;
            //Priest T9 Healing 2P: 20% bonus healing for Prayer of Mending
            if (lvl >= 80 && baseId == PRAYER_OF_MENDING_HEAL)
                pctbonus += 0.2f;

            //Twin Disciplines (healing part): 5% bonus healing for instant spells
            if (lvl >= 10 && !spellInfo->CastTimeEntry)
                pctbonus += 0.05f;
            //Improved Renew: 15% bonus healing for Renew
            if (lvl >= 10 && baseId == RENEW_1)
                pctbonus += 0.15f;
            //Focused Power part 2: 4% bonus heal for all spells
            if ((GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) && lvl >= 35)
                pctbonus += 0.04f;
            //Spiritual Healing: 10% bonus healing for all spells
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) && lvl >= 35)
                pctbonus += 0.15f;
            //Blessend Resilience part 1: 3% bonus healing for all spells
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) && lvl >= 40)
                pctbonus += 0.03f;
            //Empowered Healing: 40% bonus (from spellpower) for Greater Heal and 20% bonus (from spellpower) for Flash Heal and Binding Heal
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) && lvl >= 45)
            {
                if (baseId == HEAL)
                    flat_mod += me->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_MAGIC) * 0.4f * me->CalculateDefaultCoefficient(spellInfo, damagetype) * 1.88f * me->CalculateLevelPenalty(spellInfo) * stack;
                else if (baseId == FLASH_HEAL_1/* || baseId == BINDING_HEAL_1*/)
                    flat_mod += me->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_MAGIC) * 0.2f * me->CalculateDefaultCoefficient(spellInfo, damagetype) * 1.88f * me->CalculateLevelPenalty(spellInfo) * stack;
            }
            //Empowered Renew (heal bonus part): 15% bonus healing (from spellpower) for Renew
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) && lvl >= 50 && baseId == RENEW_1)
                flat_mod += me->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_MAGIC) * 0.15f * me->CalculateDefaultCoefficient(spellInfo, damagetype) * 1.88f * me->CalculateLevelPenalty(spellInfo) * stack;
            //Test of Faith: 12% bonus healing on targets at or below 50% health
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) && lvl >= 50 && GetHealthPCT(victim) <= 50)
                pctbonus += 0.12f;
            //Divine Providence: 10% bonus healing for Circle of Healing, Binding Heal, Holy Nova, Prayer of Healing, Divine Hymn and Prayer of Mending
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) && lvl >= 55 &&
                ((spellInfo->SpellFamilyFlags[0] & 0x18000200) ||
                (spellInfo->SpellFamilyFlags[1] & 0x4) ||
                (spellInfo->SpellFamilyFlags[2] & 0x4)))
                pctbonus += 0.1f;

            //flat mods
            //Improved Prayer of Mending: 100 additional heal for Prayer of Mending
            if (baseId == PRAYER_OF_MENDING_HEAL)
                flat_mod += 100;

            heal = heal * (1.0f + pctbonus) + flat_mod;
        }

        void ApplyClassSpellCostMods(SpellInfo const* spellInfo, int32& cost) const override
        {
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            uint8 lvl = me->GetLevel();
            float fcost = float(cost);
            int32 flatbonus = 0;
            float pctbonus = 0.0f;

            //percent mods
            //Inner Focus
            if (AuraEffect const* focu = me->GetAuraEffect(INNER_FOCUS_1, 0))
                if (focu->IsAffectedOnSpell(spellInfo))
                    pctbonus += 1.f;
            //Surge of Light
            if (AuraEffect const* surg = me->GetAuraEffect(SURGE_OF_LIGHT_BUFF, 1))
                if (surg->IsAffectedOnSpell(spellInfo))
                    pctbonus += 1.f;

            //Reduced Prayer of Healing Cost (38410):
            if (lvl >= 60 && baseId == PRAYER_OF_HEALING_1)
                pctbonus += 0.1f;
            //Greater Heal Cost Reduction (60155):
            if (lvl >= 60 && baseId == HEAL)
                pctbonus += 0.05f;

            //Shadow Focus part 2
            if (lvl >= 15 && (spellInfo->GetSchoolMask() & SPELL_SCHOOL_MASK_SHADOW))
                pctbonus += 0.06f;
            //Absolution:
            if ((GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) && lvl >= 25 && (spellInfo->SpellFamilyFlags[1] & 0x81))
                pctbonus += 0.15f;
            //Mental Agility:
            if ((GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) && lvl >= 25 && !spellInfo->CastTimeEntry)
                pctbonus += 0.1f;
            //Improved Healing:
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) &&
                lvl >= 25 && (baseId == HEAL || baseId == DIVINE_HYMN_1 || baseId == PENANCE_HEAL_1))
                pctbonus += 0.15f;
            //Soul Warding part 2
            if ((GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) && lvl >= 30 && baseId == PW_SHIELD_1)
                pctbonus += 0.15f;
            //Healing Prayers:
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) &&
                lvl >= 30 && (baseId == PRAYER_OF_HEALING_1 || baseId == PRAYER_OF_MENDING_1))
                pctbonus += 0.2f;
            //Focused Mind
            if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) &&
                lvl >= 30 && (baseId == MIND_BLAST_1 || baseId == MIND_FLAY_1 ||
                baseId == MIND_SEAR_1/* || baseId == MIND_CONTROL_1*/))
                pctbonus += 0.15f;
            //Improved Flash Heal part 1
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) && lvl >= 40 && baseId == FLASH_HEAL_1)
                pctbonus += 0.15f;

            //Glyph of Fading
            if (lvl >= 15 && baseId == FADE_1)
                pctbonus += 0.3f;
            //Glyph of Fortitude
            if (lvl >= 15 && baseId == PW_FORTITUDE_1)
                pctbonus += 0.5f;
            //Glyph of Flash Heal
            if (lvl >= 20 && baseId == FLASH_HEAL_1)
                pctbonus += 0.1f;
            //Glyph of Mass Dispel
            if (lvl >= 70 && baseId == MASS_DISPEL_1)
                pctbonus += 0.35f;

            //flat mods
            //Cleanse Cost Reduced (id: 27847): -25 mana cost for Cleanse
            //if (lvl >= 40 && spellId == GetSpell(CLEANSE_1))
            //    flatbonus += 25;

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
            float pctbonus = 0.0f;

            //100% mods
            //Surge of Light
            if (AuraEffect const* surg = me->GetAuraEffect(SURGE_OF_LIGHT_BUFF, 1))
                if (surg->IsAffectedOnSpell(spellInfo))
                    pctbonus += 1.f;

            //pct mods
            //Serendipity: -12% per stack cast time for Prayer of Healing or Greater Heal
            if (baseId == GREATER_HEAL_1 || baseId == PRAYER_OF_HEALING_1)
            {
                if (AuraEffect const* sere = me->GetAuraEffect(SERENDIPITY_BUFF, 0))
                    if (sere->IsAffectedOnSpell(spellInfo))
                        pctbonus += 0.12f * sere->GetBase()->GetStackAmount();
            }

            //flat mods
            //Improved Prayer of Healing (21339)
            if (lvl >= 60 && baseId == PRAYER_OF_HEALING_1)
                timebonus += 100;
            //Master Healer (15027) rank 5
            if (lvl >= 60 && baseId == HEAL)
                timebonus += 500;
            //Prophesy Flash Heal Bonus (21973) part 1
            if (lvl >= 60 && baseId == FLASH_HEAL_1)
                timebonus += 100;

            //Divine Fury
            if (lvl >= 15 && (baseId == HEAL || baseId == SMITE_1 || baseId == HOLY_FIRE_1))
                timebonus += 500;
            //Focused Power part 3
            if ((GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) && lvl >= 35 && baseId == MASS_DISPEL_1)
                timebonus += 1000;
            //Improved Mana Burn
            //if (lvl >= 35 && baseId == MANA_BURN_1)
            //    timebonus += 1000;

            //Glyph of Scourge Imprisonment
            if (lvl >= 20 && baseId == SHACKLE_UNDEAD_1)
                timebonus += 1000;

            casttime = std::max<int32>((float(casttime) * (1.0f - pctbonus)) - timebonus, 0);
        }

        void ApplyClassSpellNotLoseCastTimeMods(SpellInfo const* spellInfo, int32& delayReduce) const override
        {
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchoolMask schools = spellInfo->GetSchoolMask();
            uint8 lvl = me->GetLevel();
            int32 reduceBonus = 0;

            if (lvl >= 10)
            {
                switch (baseId)
                {
                    case FLASH_HEAL_1: case LESSER_HEAL_1: case NORMAL_HEAL_1: case GREATER_HEAL_1: case PRAYER_OF_HEALING_1: case PENANCE_1: case DIVINE_HYMN_1:
                        reduceBonus += 70;
                        break;
                    default:
                        break;
                }
            }

            delayReduce += reduceBonus;
        }

        void ApplyClassSpellCooldownMods(SpellInfo const* spellInfo, uint32& cooldown) const override
        {
            //cooldown is in milliseconds
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            uint8 lvl = me->GetLevel();
            int32 timebonus = 0;
            float pctbonus = 0.0f;

            //pct mods
            //Aspiration
            if ((GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) &&
                lvl >= 45 && (baseId == INNER_FOCUS_1 || baseId == POWER_INFUSION_1 || baseId == PAIN_SUPPRESSION_1))
                pctbonus += 0.2f;

            //flat mods
            //Veiled Shadows part 2: 暗影魔 CD -2 分钟
            if (lvl >= 25 && baseId == SHADOWFIEND_1)
                timebonus += 120000;
            //Glyph of Dispersion:
            if (lvl >= 60 && baseId == DISPERSION_1)
                timebonus += 45000;
            //Glyph of Penance:
            if (lvl >= 60 && baseId == PENANCE_1)
                timebonus += 2000;

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

            //pct mods
            //Aspiration
            if ((GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) && lvl >= 45 && baseId == PENANCE_1)
                pctbonus += 0.2f;
            //Divine Providence:
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) && lvl >= 55 && baseId == PRAYER_OF_MENDING_1)
                pctbonus += 0.3f;

            //flat mods
            //Quick Fade (18388)
            if (lvl >= 40 && baseId == FADE_1)
                timebonus += 2000;

            //Improved Psychic Scream
            if (lvl >= 20 && baseId == PSYCHIC_SCREAM_1)
                timebonus += 4000;
            //Improved Mind Blast
            if (lvl >= 20 && baseId == MIND_BLAST_1)
                timebonus += 2500;
            //Veiled Shadows part 1
            if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) && lvl >= 25 && baseId == FADE_1)
                timebonus += 6000;
            //Soul Warding part 1
            if ((GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE) && lvl >= 30 && baseId == PW_SHIELD_1)
                timebonus += 4000;

            //Glyph of Fade
            if (lvl >= 15 && baseId == FADE_1)
                timebonus += 9000;

            cooldown = std::max<int32>((float(cooldown) * (1.0f - pctbonus)) - timebonus, 0);
        }

        void ApplyClassSpellGlobalCooldownMods(SpellInfo const* spellInfo, float& cooldown) const override
        {
            //cooldown is in milliseconds
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            uint8 lvl = me->GetLevel();
            float timebonus = 0.0f;
            float pctbonus = 0.0f;

            //Prophesy Flash Heal Bonus (21973) part 2
            if (lvl >= 60 && baseId == FLASH_HEAL_1)
                timebonus += 100;

            cooldown = (cooldown * (1.0f - pctbonus)) - timebonus;
        }

        void ApplyClassSpellRadiusMods(SpellInfo const* spellInfo, float& radius) const override
        {
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            uint8 lvl = me->GetLevel();
            float flatbonus = 0.0f;
            float pctbonus = 0.0f;

            //pct mods
            //Holy Reach
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) &&
                lvl >= 25 && ((spellInfo->SpellFamilyFlags[0] & 0x18400200) || (spellInfo->SpellFamilyFlags[2] & 0x4)))
                pctbonus += 0.2f;

            //flat mods
            //Glyph of Mind Sear
            if (lvl >= 75 && baseId == MIND_SEAR_DAMAGE_1)
                flatbonus += 5.f;

            radius = radius * (1.0f + pctbonus) + flatbonus;
        }

        void ApplyClassSpellRangeMods(SpellInfo const* spellInfo, float& maxrange) const override
        {
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            //SpellSchool school = GetFirstSchoolInMask(spellInfo->GetSchoolMask());
            uint8 lvl = me->GetLevel();
            float flatbonus = 0.0f;
            float pctbonus = 0.0f;

            //pct mods
            //Shadow Reach: +20% range for Shadow Spells
            if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) && lvl >= 25 &&
                ((spellInfo->SpellFamilyFlags[0] & 0x682A004) ||
                (spellInfo->SpellFamilyFlags[1] & 0x300502) ||
                (spellInfo->SpellFamilyFlags[2] & 0x2040)))
                pctbonus += 0.2f;
            //Holy Reach: +20% range for Holy Spells
            if ((GetSpec() == BOT_SPEC_PRIEST_HOLY) && lvl >= 25 && (spellInfo->SpellFamilyFlags[0] & 0x100080))
                pctbonus += 0.2f;

            //flat mods
            //Glyph of Shackle Undead: +5 yd range for Shackle Undead
            if (lvl >= 20 && baseId == SHACKLE_UNDEAD_1)
                flatbonus += 5.f;

            maxrange = maxrange * (1.0f + pctbonus) + flatbonus;
        }

        void ApplyClassSpellMaxTargetsMods(SpellInfo const* spellInfo, uint32& targets) const override
        {
            uint32 bonusTargets = 0;

            //Glyph of Circle of Healing: + 1 target
            if (spellInfo->SpellFamilyFlags[0] & 0x10000000)
                bonusTargets += 1;

            targets = targets + bonusTargets;
        }

        void ApplyClassEffectMods(SpellInfo const* spellInfo, uint8 effIndex, float& value) const override
        {
            uint32 baseId = spellInfo->GetFirstRankSpell()->Id;
            uint8 lvl = me->GetLevel();
            float pctbonus = 1.0f;

            //Improved Power Word: Fortitude
            if (lvl >= 15 && baseId == PW_FORTITUDE_1 && effIndex == EFFECT_0)
                pctbonus *= 1.3f;
            if (lvl >= 20 && baseId == PW_SHIELD_1 && effIndex == EFFECT_0)
            {
                //Improved PWSH: +15% effect
                pctbonus *= 1.15f;
                //Borrowed Time: +40% of spellpower
                if (GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE && lvl >= 55)
                    value += me->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_MAGIC) * 0.4f;
            }

            value = value * pctbonus;
        }

        void OnClassSpellGo(SpellInfo const* spellInfo) override
        {
            //Surge of Light
            //Inner Focus
            AuraEffect const* surg = me->GetAuraEffect(SURGE_OF_LIGHT_BUFF, 1);
            AuraEffect const* focu = me->GetAuraEffect(INNER_FOCUS_1, 0);
            if (surg && surg->IsAffectedOnSpell(spellInfo))
                me->RemoveAurasDueToSpell(SURGE_OF_LIGHT_BUFF);
            else if (focu && focu->IsAffectedOnSpell(spellInfo))
                me->RemoveAurasDueToSpell(INNER_FOCUS_1);

            //Serendipity
            if (AuraEffect const* sere = me->GetAuraEffect(SERENDIPITY_BUFF, 0))
                if (sere->IsAffectedOnSpell(spellInfo))
                    me->RemoveAurasDueToSpell(SERENDIPITY_BUFF);
        }

        void SpellHitTarget(Unit* wtarget, SpellInfo const* spell) override
        {
            Unit* target = wtarget->ToUnit();
            if (!target)
                return;

            uint32 spellId = spell->Id;
            uint32 baseId = spell->GetFirstRankSpell()->Id;
            uint8 lvl = me->GetLevel();

            //Glyph of Hymn of Hope: +2 sec duration
            if (lvl >= 60 && (baseId == HYMN_OF_HOPE_1 || baseId == HYMN_OF_HOPE_BUFF))
            {
                if (Aura* hymn = target->GetAura(spellId))
                {
                    hymn->SetDuration(hymn->GetDuration() + 2000);
                    hymn->SetDuration(hymn->GetMaxDuration() + 2000);
                }
            }
            //Priest T9 Shadow 2P Bonus (67193)
            if (lvl >= 80 && baseId == VAMPIRIC_TOUCH_1)
            {
                if (Aura* touc = target->GetAura(spellId))
                {
                    uint32 dur = touc->GetMaxDuration() + 6000;
                    touc->SetDuration(dur);
                    touc->SetMaxDuration(dur);
                }
            }

            //Improved Mind Blast part 2
            if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) && lvl >= 20 && baseId == MIND_BLAST_1)
                me->CastSpell(target, IMPROVED_MIND_BLAST_DEBUFF, true);

            //悲惨（Misery）：暗言术：痛 / 心灵震爆 / 心灵鞭笞 命中后使目标受到法术伤害 +3%（团队易伤）
            if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) && lvl >= 45 &&
                (baseId == SW_PAIN_1 || baseId == MIND_BLAST_1 || baseId == MIND_FLAY_1))
            {
                if (!target->HasAura(MISERY_DEBUFF, me->GetGUID()))
                    me->CastSpell(target, MISERY_DEBUFF, true);
            }

            //Weakened Soul Reduction (id: 33333): -2 sec to Weakened Soul duration
            if (lvl >= 51 && baseId == WEAKENED_SOUL_DEBUFF)
            {
                if (Aura* soul = target->GetAura(spellId))
                {
                    uint32 dur = soul->GetMaxDuration() - 2000;
                    soul->SetDuration(dur);
                    soul->SetMaxDuration(dur);
                }
            }
            //Pain and Suffering (part 1): 100% to refresh Shadow Word: Pain on target hit by Mind Flay
            if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) && lvl >= 50 && baseId == MIND_FLAY_1 && GetSpell(SW_PAIN_1))
                if (Aura* pain = target->GetAura(GetSpell(SW_PAIN_1), me->GetGUID()))
                    pain->RefreshDuration();
            if (baseId == FEAR_WARD_1)
            {
                //2 minutes bonus duration for Fear Ward
                if (Aura* ward = target->GetAura(spellId, me->GetGUID()))
                {
                    uint32 dur = ward->GetDuration() + 120000;
                    ward->SetDuration(dur);
                    ward->SetMaxDuration(dur);
                }
            }
            //buffs duration
            if (baseId == INNER_FIRE_1 || baseId == VAMPIRIC_EMBRACE_1 || baseId == PW_FORTITUDE_1 ||
                baseId == SHADOW_PROTECTION_1 || baseId == DIVINE_SPIRIT_1)
            {
                //1 hour duration for all buffs
                if (Aura* buff = target->GetAura(spellId, me->GetGUID()))
                {
                    uint32 dur = HOUR * IN_MILLISECONDS;
                    buff->SetDuration(dur);
                    buff->SetMaxDuration(dur);
                }
            }

            OnSpellHitTarget(target, spell);
        }

        void SpellHit(Unit* wcaster, SpellInfo const* spell) override
        {
            Unit* caster = wcaster->ToUnit();
            if (!caster)
                return;

            uint32 spellId = spell->Id;
            uint32 baseId = spell->GetFirstRankSpell()->Id;
            uint8 lvl = me->GetLevel();

            //Glyph of Inner Fire + Improved Inner Fire:
            if (lvl >= 15 && baseId == INNER_FIRE_1)
            {
                if (Aura* fire = me->GetAura(spellId))
                {
                    fire->SetCharges(fire->GetCharges() + 12);
                    for (auto i : NPCBots::index_array<uint8, MAX_SPELL_EFFECTS>)
                        if (AuraEffect* eff = fire->GetEffect(i))
                            eff->ChangeAmount(int32(eff->GetAmount() * (i == 0 ? 1.45f*1.5f : 1.45f)));
                }
            }
            //Improved Vampiric Embrace
            if ((GetSpec() == BOT_SPEC_PRIEST_SHADOW) && lvl >= 30 && baseId == VAMPIRIC_EMBRACE_1)
            {
                if (AuraEffect* vamp = me->GetAuraEffect(spellId, 0))
                    vamp->ChangeAmount(vamp->GetAmount() + 10); //67% is essentially this
            }

            OnSpellHit(caster, spell);
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

        float GetSpellAttackRange(bool longRange) const override
        {
            return longRange ? CalcSpellMaxRange(MIND_FLAY_1) : 20.f;
        }

        void SummonBotPet(Unit* target)
        {
            if (GetBotsPet())
                UnsummonAll(false);

            uint32 entry = BOT_PET_SHADOWFIEND;

            //Position pos;

            //15 sec duration
            Creature* myPet = me->SummonCreature(entry, *me, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000);
            if (!myPet)
                return;

            //me->GetNearPoint(myPet, pos.m_positionX, pos.m_positionY, pos.m_positionZ, 0, 2, me->GetOrientation());
            //myPet->GetMotionMaster()->MovePoint(me->GetMapId(), pos);
            myPet->SetCreator(master);
            myPet->SetOwnerGUID(me->GetGUID());
            myPet->SetFaction(master->GetFaction());
            myPet->SetControlledByPlayer(!IAmFree());
            myPet->SetPvP(me->IsPvP());
            myPet->SetByteValue(UNIT_FIELD_BYTES_2, 1, master->GetByteValue(UNIT_FIELD_BYTES_2, 1));
            myPet->SetUInt32Value(UNIT_CREATED_BY_SPELL, SHADOWFIEND_1);
            myPet->SetFloatValue(UNIT_FIELD_COMBATREACH, 2.0f * DEFAULT_COMBAT_REACH * me->GetObjectScale());
            botPetGUID = myPet->GetGUID();

            // 补发召唤法术日志，让 DPS 插件把暗影魔的伤害并入 bot(见 bot_ai::SendSummonCombatLog)
            bot_ai::SendSummonCombatLog(me, myPet, SHADOWFIEND_1);

            myPet->Attack(target, true);
            if (!HasBotCommandState(BOT_COMMAND_MASK_UNCHASE))
                myPet->GetMotionMaster()->MoveChase(target);
        }

        void UnsummonAll(bool savePets = true) override
        {
            UnsummonPet(savePets);
        }

        void SummonedCreatureDies(Creature* /*summon*/, Unit* /*killer*/) override
        {
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            //BOT_LOG_ERROR("entities.unit", "SummonedCreatureDespawn: %s's %s", me->GetName().c_str(), summon->GetName().c_str());
            if (summon->GetGUID() == botPetGUID)
            {
                botPetGUID.Clear();
            }
        }

        uint32 GetAIMiscValue(uint32 data) const override
        {
            switch (data)
            {
                case BOTAI_MISC_PET_TYPE:
                    return BOT_PET_SHADOWFIEND;
                default:
                    return 0;
            }
        }

        void Reset() override
        {
            UnsummonAll(false);

            Shackle_Timer = 0;
            Mend_Timer = 0;
            Lightwell_Timer = 0;
            ManaBurn_Timer = 0;

            DispelcheckTimer = 0;
            DevcheckTimer = 0;
            ShackcheckTimer = 0;

            Devcheck = false;
            Shackcheck = false;

            DefaultInit();
        }

        void ReduceCD(uint32 diff) override
        {
            if (Shackle_Timer > diff)               Shackle_Timer -= diff;
            if (Mend_Timer > diff)                  Mend_Timer -= diff;
            if (Lightwell_Timer > diff)             Lightwell_Timer -= diff;
            if (ManaBurn_Timer > diff)              ManaBurn_Timer -= diff;

            if (DispelcheckTimer > diff)            DispelcheckTimer -= diff;
            if (DevcheckTimer > diff)               DevcheckTimer -= diff;
            if (ShackcheckTimer > diff)             ShackcheckTimer -= diff;
        }

        void InitPowers() override
        {
            me->SetPowerType(POWER_MANA);
        }

        void InitSpells() override
        {
            uint8 lvl = me->GetLevel();
            bool isDisc = GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE;
            bool isHoly = GetSpec() == BOT_SPEC_PRIEST_HOLY;
            bool isShad = GetSpec() == BOT_SPEC_PRIEST_SHADOW;

            InitSpellMap(DISPEL_MAGIC_1);
            InitSpellMap(MASS_DISPEL_1);
            InitSpellMap(CURE_DISEASE_1);
            InitSpellMap(ABOLISH_DISEASE_1);
            InitSpellMap(FEAR_WARD_1);
            InitSpellMap(PSYCHIC_SCREAM_1);
            InitSpellMap(FADE_1);
            InitSpellMap(MIND_SEAR_1);
            InitSpellMap(SHACKLE_UNDEAD_1);
            InitSpellMap(GREATER_HEAL_1);
            InitSpellMap(NORMAL_HEAL_1);
            InitSpellMap(LESSER_HEAL_1);
            InitSpellMap(RENEW_1);
            InitSpellMap(FLASH_HEAL_1);
            InitSpellMap(PRAYER_OF_HEALING_1);
            InitSpellMap(DIVINE_HYMN_1);
            InitSpellMap(PRAYER_OF_MENDING_1);
            InitSpellMap(RESURRECTION_1);
            InitSpellMap(PW_SHIELD_1);
            InitSpellMap(INNER_FIRE_1);
            InitSpellMap(PW_FORTITUDE_1);
            InitSpellMap(SHADOW_PROTECTION_1);
            InitSpellMap(DIVINE_SPIRIT_1);
            InitSpellMap(HOLY_FIRE_1);
            InitSpellMap(HOLY_NOVA_1);
            InitSpellMap(SMITE_1);
            InitSpellMap(SW_PAIN_1);
            InitSpellMap(MIND_BLAST_1);
            InitSpellMap(SW_DEATH_1);
            InitSpellMap(DEVOURING_PLAGUE_1);
            InitSpellMap(HYMN_OF_HOPE_1);
            InitSpellMap(MANA_BURN_1);
            InitSpellMap(LEVITATE_1);
            InitSpellMap(SHADOWFIEND_1); //not casted

  /*Talent*/lvl >= 20 && isDisc ? InitSpellMap(INNER_FOCUS_1) : RemoveSpell(INNER_FOCUS_1);
  /*Talent*/lvl >= 40 && isDisc ? InitSpellMap(POWER_INFUSION_1) : RemoveSpell(POWER_INFUSION_1);
  /*Talent*/lvl >= 50 && isDisc ? InitSpellMap(PAIN_SUPPRESSION_1) : RemoveSpell(PAIN_SUPPRESSION_1);
  /*Talent*/lvl >= 60 && isDisc ? InitSpellMap(PENANCE_1) : RemoveSpell(PENANCE_1);

  /*Talent*/lvl >= 20 && isHoly ? InitSpellMap(DESPERATE_PRAYER_1) : RemoveSpell(DESPERATE_PRAYER_1);
  /*Talent*/lvl >= 40 && isHoly ? InitSpellMap(BINDING_HEAL_1) : RemoveSpell(BINDING_HEAL_1);
  /*Talent*/lvl >= 50 && isHoly ? InitSpellMap(CIRCLE_OF_HEALING_1) : RemoveSpell(CIRCLE_OF_HEALING_1);
  /*Talent*/lvl >= 60 && isHoly ? InitSpellMap(GUARDIAN_SPIRIT_1) : RemoveSpell(GUARDIAN_SPIRIT_1);
  /*Talent*/lvl >= 40 && (isHoly || isDisc) ? InitSpellMap(LIGHTWELL_1) : RemoveSpell(LIGHTWELL_1);

  /*Talent*/lvl >= 20 && isShad ? InitSpellMap(MIND_FLAY_1) : RemoveSpell(MIND_FLAY_1);
  /*Talent*/lvl >= 30 && isShad ? InitSpellMap(SILENCE_1) : RemoveSpell(SILENCE_1);
  /*Talent*/lvl >= 30 && isShad ? InitSpellMap(VAMPIRIC_EMBRACE_1) : RemoveSpell(VAMPIRIC_EMBRACE_1);
  /*Talent*/lvl >= 40 && isShad ? InitSpellMap(SHADOWFORM_1) : RemoveSpell(SHADOWFORM_1);
  /*Talent*/lvl >= 50 && isShad ? InitSpellMap(VAMPIRIC_TOUCH_1) : RemoveSpell(VAMPIRIC_TOUCH_1);
  /*Talent*/lvl >= 50 && isShad ? InitSpellMap(PSYCHIC_HORROR_1) : RemoveSpell(PSYCHIC_HORROR_1);
  /*Talent*/lvl >= 60 && isShad ? InitSpellMap(DISPERSION_1) : RemoveSpell(DISPERSION_1);

            HEAL = GetSpell(GREATER_HEAL_1) ? GREATER_HEAL_1 :
                GetSpell(NORMAL_HEAL_1) ? NORMAL_HEAL_1 :
                GetSpell(LESSER_HEAL_1) ? LESSER_HEAL_1 : 0;
        }

        void ApplyClassPassives() const override
        {
            uint8 level = master->GetLevel();
            bool isDisc = GetSpec() == BOT_SPEC_PRIEST_DISCIPLINE;
            bool isHoly = GetSpec() == BOT_SPEC_PRIEST_HOLY;
            bool isShad = GetSpec() == BOT_SPEC_PRIEST_SHADOW;
            RefreshAura(SHUANGSHENGJIELV, level >= 10 ? 1 : 0);
            RefreshAura(UNBREAKABLE_WILL, level >= 10 ? 1 : 0);
            RefreshAura(XINGLINGZHIHUO, level >= 60 ? 1 : 0);
            RefreshAura(MEDITATION, level >= 20 ? 1 : 0);
            RefreshAura(RENEWED_HOPE, isDisc && level >= 45 ? 1 : 0);
            RefreshAura(RAPTURE, isDisc && level >= 45 ? 1 : 0);
            RefreshAura(DIVINE_AEGIS, isDisc && level >= 50 ? 1 : 0);
            RefreshAura(GRACE, isDisc && level >= 50 ? 1 : 0);
            RefreshAura(BORROWED_TIME, isDisc && level >= 55 ? 1 : 0);
            //戒律系天赋等级门槛（G1 修正：>=70 → 各自正确需求等级）
            RefreshAura(QIDI, isDisc && level >= 10 ? 1 : 0);
            RefreshAura(QH_DUN, isDisc && level >= 20 ? 1 : 0);
            RefreshAura(QH_SHEMINA, isDisc && level >= 20 ? 1 : 0);
            RefreshAura(JINGSHENMINGRUI, isDisc && level >= 20 ? 1 : 0);
            RefreshAura(XINGLINGZHILI, isDisc && level >= 30 ? 1 : 0);
            RefreshAura(XINGLINGSHOUHU, isDisc && level >= 30 ? 1 : 0);
            RefreshAura(KUAISUZHIL, isDisc && level >= 40 ? 1 : 0);
            RefreshAura(NENGLIANGJIZHONG, isDisc && level >= 50 ? 1 : 0);
            RefreshAura(KEWANG, isDisc && level >= 60 ? 1 : 0);

            RefreshAura(INSPIRATION3, isHoly && level >= 25 ? 1 : 0);
            RefreshAura(INSPIRATION2, isHoly && level >= 23 && level < 25 ? 1 : 0);
            RefreshAura(INSPIRATION1, isHoly && level >= 20 && level < 23 ? 1 : 0);
            RefreshAura(SURGE_OF_LIGHT, isHoly && level >= 35 ? 1 : 0);
            RefreshAura(HOLY_CONCENTRATION, isHoly && level >= 40 ? 1 : 0);
            RefreshAura(BODY_AND_SOUL1, isHoly && level >= 45 ? 1 : 0);
            RefreshAura(SERENDIPITY, isHoly && level >= 45 ? 1 : 0);
            RefreshAura(EMPOWERED_RENEW3, isHoly && level >= 55 ? 1 : 0);
            RefreshAura(EMPOWERED_RENEW2, isHoly && level >= 53 && level < 55 ? 1 : 0);
            RefreshAura(EMPOWERED_RENEW1, isHoly && level >= 50 && level < 53 ? 1 : 0);
            //神圣系 7 项天赋（G1 修正：>=70 → >=60）
            RefreshAura(QH_ZHILIAOSHU, isHoly && level >= 60 ? 1 : 0);
            RefreshAura(QH_ZHILIAOQIDAO, isHoly && level >= 60 ? 1 : 0);
            RefreshAura(JINGSHENZHIYING, isHoly && level >= 60 ? 1 : 0);
            RefreshAura(JINGSHENZHILIAO, isHoly && level >= 60 ? 1 : 0);
            RefreshAura(QUANXINGQUANYI, isHoly && level >= 60 ? 1 : 0);
            RefreshAura(ZHILIAOZENGXIAO, isHoly && level >= 60 ? 1 : 0);
            RefreshAura(SHENSHENGJUANGU, isHoly && level >= 60 ? 1 : 0);
            //神圣专精（G1 修正：50 → 10）
            RefreshAura(SHENSHENGZHUANJING, isHoly && level >= 10 ? 1 : 0);

            RefreshAura(SPIRIT_TAP, isShad && level >= 10 ? 1 : 0);
            RefreshAura(IMPROVED_SPIRIT_TAP, isShad && level >= 10 ? 1 : 0);
            RefreshAura(SHADOW_WEAVING3, isShad && level >= 30 ? 1 : 0);
            RefreshAura(SHADOW_WEAVING2, isShad && level >= 28 && level < 30 ? 1 : 0);
            RefreshAura(SHADOW_WEAVING1, isShad && level >= 25 && level < 28 ? 1 : 0);
            RefreshAura(IMPROVED_DEVOURING_PLAGUE, isShad && level >= 35 ? 1 : 0);
            RefreshAura(IMPROVED_SHADOWFORM, isShad && level >= 45 ? 1 : 0);
            RefreshAura(MISERY3, isShad && level >= 50 ? 1 : 0);
            RefreshAura(MISERY2, isShad && level >= 48 && level < 50 ? 1 : 0);
            RefreshAura(MISERY1, isShad && level >= 45 && level < 48 ? 1 : 0);
            //暗影系天赋补等级门控（P2）
            RefreshAura(HEIAN, isShad && level >= 10 ? 1 : 0);
            RefreshAura(QH_ANYANSHU, isShad && level >= 15 ? 1 : 0);
            RefreshAura(QH_JIZHONG, isShad && level >= 15 ? 1 : 0);
            RefreshAura(QH_XINGLINGBAOZHENG, isShad && level >= 20 ? 1 : 0);
            RefreshAura(QH_ZHEBIZHIYING, isShad && level >= 20 ? 1 : 0);
            RefreshAura(QH_XIXUEGUI, isShad && level >= 30 ? 1 : 0);
            RefreshAura(QH_RONGHUA, isShad && level >= 35 ? 1 : 0);
            RefreshAura(QH_ANYINGNENGLIANG, isShad && level >= 40 ? 1 : 0);
            RefreshAura(QH_ANYINGXINGTAI, isShad && level >= 45 ? 1 : 0);
            RefreshAura(BAOSHOUZHEMO, isShad && level >= 50 ? 1 : 0);
            RefreshAura(NIUQUXINGYANG, isShad && level >= 55 ? 1 : 0);
            RefreshAura(PRIEST_T10_SHADOW_2P_BONUS, isShad && level >= 70 ? 1 : 0);

            RefreshAura(GLYPH_SW_PAIN, level >= 15 ? 1 : 0);//G7：痛点雕文启用
            RefreshAura(GLYPH_KUXIU, isDisc && level >= 75 ? 1 : 0);
            RefreshAura(GLYPH_PW_SHIELD, level >= 15 ? 1 : 0);
            RefreshAura(GLYPH_DISPEL_MAGIC, level >= 18 ? 1 : 0);
            RefreshAura(GLYPH_PRAYER_OF_HEALING, level >= 30 ? 1 : 0);
            RefreshAura(GLYPH_SHADOW, level >= 30 ? 1 : 0);
            RefreshAura(GLYPH_BIANCHI, isShad && level >= 30 ? 1 : 0);
            RefreshAura(PRIEST_T10_2P_BONUS, level >= 70 ? 1 : 0);
        }

        bool CanUseManually(uint32 basespell) const override
        {
            switch (basespell)
            {
                case MASS_DISPEL_1:
                case ABOLISH_DISEASE_1:
                case PAIN_SUPPRESSION_1:
                case FADE_1:
                case PENANCE_1:
                case VAMPIRIC_EMBRACE_1:
                case DISPERSION_1:
                case GUARDIAN_SPIRIT_1:
                case RENEW_1:
                case PRAYER_OF_HEALING_1:
                case CIRCLE_OF_HEALING_1:
                case DIVINE_HYMN_1:
                case PRAYER_OF_MENDING_1:
                case PW_SHIELD_1:
                case INNER_FIRE_1:
                case PW_FORTITUDE_1:
                case SHADOW_PROTECTION_1:
                case DIVINE_SPIRIT_1:
                case FEAR_WARD_1:
                case FLASH_HEAL_1:
                case GREATER_HEAL_1:
                case BINDING_HEAL_1:
                case LIGHTWELL_1:
                case MANA_BURN_1:
                case HOLY_NOVA_1:
                case LEVITATE_1:
                    return true;
                case NORMAL_HEAL_1:
                    return !GetSpell(GREATER_HEAL_1);
                case LESSER_HEAL_1:
                    return !GetSpell(NORMAL_HEAL_1) && !GetSpell(GREATER_HEAL_1);
                case SHADOWFORM_1:
                    return me->GetShapeshiftForm() != FORM_SHADOW;
                default:
                    return false;
            }
        }

        std::vector<uint32> const* GetDamagingSpellsList() const override
        {
            return &Priest_spells_damage;
        }
        std::vector<uint32> const* GetCCSpellsList() const override
        {
            return &Priest_spells_cc;
        }
        std::vector<uint32> const* GetHealingSpellsList() const override
        {
            return &Priest_spells_heal;
        }
        std::vector<uint32> const* GetSupportSpellsList() const override
        {
            return &Priest_spells_support;
        }

        //治疗量改为 GetHealAmount 动态估算，不再缓存初始化快照
        void InitHeals() override {}

    private:
        uint32 HEAL;
        uint32 Shackle_Timer, Mend_Timer, DispelcheckTimer, DevcheckTimer, ShackcheckTimer;
        uint32 Lightwell_Timer, ManaBurn_Timer;
/*Misc*/bool Devcheck, Shackcheck;
    };
};

void AddSC_priest_bot()
{
    new priest_bot();
}
