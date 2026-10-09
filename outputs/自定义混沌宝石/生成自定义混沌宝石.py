# -*- coding: utf-8 -*-
"""生成自定义混沌宝石的服务端 SQL 与客户端 DBC CSV。"""
import csv, os

OUT = os.path.dirname(os.path.abspath(__file__))
SQLDIR = OUT
CSVDIR = OUT

# ============================================================
# Aura 常量
# ============================================================
A_MOD_THREAT          = 10   # 威胁值 +X%
A_MOD_DMG_PCT_DONE    = 79   # 伤害 +X%
A_MOD_DMG_PCT_TAKEN   = 87   # 受到伤害 -X%（负值）
A_MOD_INC_ENERGY_PCT  = 132  # 法力/能量上限 +X%（MiscValue=0 法力）
A_MOD_INC_HEALTH_PCT  = 133  # 生命上限 +X%
A_MOD_HEALING_PCT     = 136  # 治疗效果 +X%
A_MOD_TOTAL_STAT_PCT  = 137  # 全属性 +X%（MiscValue=-1）
A_MOD_CRIT_DMG_BONUS  = 163  # 暴击伤害 +X%

MISC_ALL = 127   # 全部伤害/治疗 school mask
MISC_MANA = 0    # POWER_MANA
MISC_ALLSTAT = -1

# ============================================================
# 16 个唯一 Spell
# ============================================================
# (spell_id, zh名, en名, [(aura, base, misc), ...])
SPELLS = [
    (93000, "源力·伤害强化",   "Source Power: Damage",          [(A_MOD_DMG_PCT_DONE,   4,  MISC_ALL)]),
    (93001, "源力·坚韧",       "Source Power: Fortitude",       [(A_MOD_THREAT,         4,  MISC_ALL), (A_MOD_DMG_PCT_TAKEN, -5,  MISC_ALL)]),
    (93002, "源力·治疗强化",   "Source Power: Healing",         [(A_MOD_HEALING_PCT,    4,  MISC_ALL)]),
    (93003, "极品·伤害强化",   "Superior Power: Damage",        [(A_MOD_DMG_PCT_DONE,   6,  MISC_ALL)]),
    (93004, "极品·暴击伤害",   "Superior Power: Crit Damage",   [(A_MOD_CRIT_DMG_BONUS, 4,  MISC_ALL)]),
    (93005, "极品·坚韧",       "Superior Power: Fortitude",     [(A_MOD_THREAT,         9,  MISC_ALL), (A_MOD_DMG_PCT_TAKEN, -7,  MISC_ALL)]),
    (93006, "极品·生命强化",   "Superior Power: Health",        [(A_MOD_INC_HEALTH_PCT, 4,  0)]),
    (93007, "极品·治疗强化",   "Superior Power: Healing",       [(A_MOD_HEALING_PCT,    6,  MISC_ALL)]),
    (93008, "极品·法力强化",   "Superior Power: Mana",          [(A_MOD_INC_ENERGY_PCT, 4,  MISC_MANA)]),
    (93009, "完美·伤害强化",   "Perfect Power: Damage",         [(A_MOD_DMG_PCT_DONE,   9,  MISC_ALL)]),
    (93010, "完美·暴击伤害",   "Perfect Power: Crit Damage",    [(A_MOD_CRIT_DMG_BONUS, 9,  MISC_ALL)]),
    (93011, "完美·全属性",     "Perfect Power: All Stats",      [(A_MOD_TOTAL_STAT_PCT, 4,  MISC_ALLSTAT)]),
    (93012, "完美·坚韧",       "Perfect Power: Fortitude",      [(A_MOD_THREAT,         14, MISC_ALL), (A_MOD_DMG_PCT_TAKEN, -9,  MISC_ALL)]),
    (93013, "完美·生命强化",   "Perfect Power: Health",         [(A_MOD_INC_HEALTH_PCT, 9,  0)]),
    (93014, "完美·治疗强化",   "Perfect Power: Healing",        [(A_MOD_HEALING_PCT,    9,  MISC_ALL)]),
    (93015, "完美·法力强化",   "Perfect Power: Mana",           [(A_MOD_INC_ENERGY_PCT, 9,  MISC_MANA)]),
]

# ============================================================
# 附魔显示文本颜色（WoW 游戏内颜色代码：|cAARRGGBB文本|r）
# 按宝石品阶区分颜色：源力=绿、极品=蓝、完美=紫（WoW 品质标准色）。
# ============================================================
ENCH_COLORS = {"源力": "|cff1eff00", "极品": "|cff0070dd", "完美": "|cffa335ee"}

# ============================================================
# 9 个 SpellItemEnchantment  (id, 品阶, zh效果描述, [spell_id...])
# ============================================================
ENCHANTS = [
    (50001, "源力", "伤害提高5%。",                                        [93000]),
    (50002, "源力", "威胁值提高5%，受到的伤害降低4%。",                    [93001]),
    (50003, "源力", "治疗效果提高5%。",                                    [93002]),
    (50004, "极品", "伤害提高7%，暴击伤害提高5%。",                        [93003, 93004]),
    (50005, "极品", "威胁值提高10%，受到的伤害降低6%，生命值上限提高5%。",  [93005, 93006]),
    (50006, "极品", "治疗效果提高7%，法力值上限提高5%。",                  [93007, 93008]),
    (50007, "完美", "伤害提高10%，暴击伤害提高10%，所有属性提高5%。",       [93009, 93010, 93011]),
    (50008, "完美", "威胁值提高15%，受到的伤害降低8%，生命值上限提高10%，所有属性提高5%。", [93012, 93013, 93011]),
    (50009, "完美", "治疗效果提高10%，法力值上限提高10%，所有属性提高5%。", [93014, 93015, 93011]),
]

# ============================================================
# 9 个 GemProperties (id, Enchant_Id)
# ============================================================
GEMPROPS = [
    (5001, 50001), (5002, 50002), (5003, 50003),
    (5004, 50004), (5005, 50005), (5006, 50006),
    (5007, 50007), (5008, 50008), (5009, 50009),
]

# ============================================================
# 物品本体（精简字段，未列出的字段使用数据库默认值）
# ============================================================

# 宝石图标按类别映射
ITEM_ICONS = {"强攻": 58601, "坚韧": 56636, "祝福": 58714}

def gem_item(entry, name, gprops, quality):
    """构造一颗多彩宝石的精简 item_template 字段。"""
    icon = next(ITEM_ICONS[c] for c in ITEM_ICONS if c in name)
    return {"entry": entry, "class": 3, "subclass": 6, "name": name,
            "displayid": icon, "Quality": quality, "Flags": 4096,
            "BuyCount": 1, "BuyPrice": 240000, "SellPrice": 60000,
            "ItemLevel": 80, "description": "只能镶嵌在多彩宝石插槽中。",
            "Material": 4, "BagFamily": 512, "GemProperties": gprops,
            "VerifiedBuild": 12340}

# 材料物品：源力碎片 / 源力核心（class=7 贸易货物, subclass=11 材料）
MATERIALS = [
    {"entry": 63100, "class": 7, "subclass": 11, "name": "源力碎片",
     "displayid": 20977, "Quality": 4, "Flags": 4096, "BuyCount": 1,
     "BuyPrice": 15000000, "SellPrice": 350000, "ItemLevel": 80, "stackable": 100,
     "delay": 1000, "Material": 4, "description": "用于合成源力核心。", "VerifiedBuild": 12340},
    {"entry": 63101, "class": 7, "subclass": 11, "name": "源力核心",
     "displayid": 49259, "Quality": 5, "Flags": 4096, "BuyCount": 1,
     "BuyPrice": 45000000, "SellPrice": 700000, "ItemLevel": 80, "stackable": 100,
     "delay": 1000, "Material": 4, "description": "用于合成源力宝石。", "VerifiedBuild": 12340},
]

# 9 颗多彩宝石（entry 从 63102 起）
ITEMS = [
    gem_item(63102, "源力强攻宝石",   5001, 3),
    gem_item(63103, "源力坚韧宝石",   5002, 3),
    gem_item(63104, "源力祝福宝石",   5003, 3),
    gem_item(63105, "极品源力强攻宝石", 5004, 4),
    gem_item(63106, "极品源力坚韧宝石", 5005, 4),
    gem_item(63107, "极品源力祝福宝石", 5006, 4),
    gem_item(63108, "完美源力强攻宝石", 5007, 5),
    gem_item(63109, "完美源力坚韧宝石", 5008, 5),
    gem_item(63110, "完美源力祝福宝石", 5009, 5),
]

# 附魔ID -> 提供该附魔的宝石物品ID（客户端据此在插槽中显示宝石图标）
ENCH_ITEM = {}
for _item in ITEMS:
    for _gid, _eid in GEMPROPS:
        if _gid == _item["GemProperties"]:
            ENCH_ITEM[_eid] = _item["entry"]

# ============================================================
# spell 默认模板（基于 spell_dbc ID=38405 被动光环）
# ============================================================
# spell_dbc 表字段顺序（234 列，含 unk_320_2/3）
SPELL_FIELDS = ["ID","Category","DispelType","Mechanic","Attributes","AttributesEx",
"AttributesEx2","AttributesEx3","AttributesEx4","AttributesEx5","AttributesEx6","AttributesEx7",
"ShapeshiftMask","unk_320_2","ShapeshiftExclude","unk_320_3","Targets","TargetCreatureType",
"RequiresSpellFocus","FacingCasterFlags","CasterAuraState","TargetAuraState","ExcludeCasterAuraState",
"ExcludeTargetAuraState","CasterAuraSpell","TargetAuraSpell","ExcludeCasterAuraSpell","ExcludeTargetAuraSpell",
"CastingTimeIndex","RecoveryTime","CategoryRecoveryTime","InterruptFlags","AuraInterruptFlags",
"ChannelInterruptFlags","ProcTypeMask","ProcChance","ProcCharges","MaxLevel","BaseLevel","SpellLevel",
"DurationIndex","PowerType","ManaCost","ManaCostPerLevel","ManaPerSecond","ManaPerSecondPerLevel",
"RangeIndex","Speed","ModalNextSpell","CumulativeAura","Totem_1","Totem_2",
"Reagent_1","Reagent_2","Reagent_3","Reagent_4","Reagent_5","Reagent_6","Reagent_7","Reagent_8",
"ReagentCount_1","ReagentCount_2","ReagentCount_3","ReagentCount_4","ReagentCount_5","ReagentCount_6",
"ReagentCount_7","ReagentCount_8","EquippedItemClass","EquippedItemSubclass","EquippedItemInvTypes",
"Effect_1","Effect_2","Effect_3","EffectDieSides_1","EffectDieSides_2","EffectDieSides_3",
"EffectRealPointsPerLevel_1","EffectRealPointsPerLevel_2","EffectRealPointsPerLevel_3",
"EffectBasePoints_1","EffectBasePoints_2","EffectBasePoints_3","EffectMechanic_1","EffectMechanic_2",
"EffectMechanic_3","ImplicitTargetA_1","ImplicitTargetA_2","ImplicitTargetA_3","ImplicitTargetB_1",
"ImplicitTargetB_2","ImplicitTargetB_3","EffectRadiusIndex_1","EffectRadiusIndex_2","EffectRadiusIndex_3",
"EffectAura_1","EffectAura_2","EffectAura_3","EffectAuraPeriod_1","EffectAuraPeriod_2","EffectAuraPeriod_3",
"EffectMultipleValue_1","EffectMultipleValue_2","EffectMultipleValue_3","EffectChainTargets_1",
"EffectChainTargets_2","EffectChainTargets_3","EffectItemType_1","EffectItemType_2","EffectItemType_3",
"EffectMiscValue_1","EffectMiscValue_2","EffectMiscValue_3","EffectMiscValueB_1","EffectMiscValueB_2",
"EffectMiscValueB_3","EffectTriggerSpell_1","EffectTriggerSpell_2","EffectTriggerSpell_3",
"EffectPointsPerCombo_1","EffectPointsPerCombo_2","EffectPointsPerCombo_3","EffectSpellClassMaskA_1",
"EffectSpellClassMaskA_2","EffectSpellClassMaskA_3","EffectSpellClassMaskB_1","EffectSpellClassMaskB_2",
"EffectSpellClassMaskB_3","EffectSpellClassMaskC_1","EffectSpellClassMaskC_2","EffectSpellClassMaskC_3",
"SpellVisualID_1","SpellVisualID_2","SpellIconID","ActiveIconID","SpellPriority",
"Name_Lang_enUS","Name_Lang_enGB","Name_Lang_koKR","Name_Lang_frFR","Name_Lang_deDE","Name_Lang_enCN",
"Name_Lang_zhCN","Name_Lang_enTW","Name_Lang_zhTW","Name_Lang_esES","Name_Lang_esMX","Name_Lang_ruRU",
"Name_Lang_ptPT","Name_Lang_ptBR","Name_Lang_itIT","Name_Lang_Unk","Name_Lang_Mask",
"NameSubtext_Lang_enUS","NameSubtext_Lang_enGB","NameSubtext_Lang_koKR","NameSubtext_Lang_frFR",
"NameSubtext_Lang_deDE","NameSubtext_Lang_enCN","NameSubtext_Lang_zhCN","NameSubtext_Lang_enTW",
"NameSubtext_Lang_zhTW","NameSubtext_Lang_esES","NameSubtext_Lang_esMX","NameSubtext_Lang_ruRU",
"NameSubtext_Lang_ptPT","NameSubtext_Lang_ptBR","NameSubtext_Lang_itIT","NameSubtext_Lang_Unk",
"NameSubtext_Lang_Mask","Description_Lang_enUS","Description_Lang_enGB","Description_Lang_koKR",
"Description_Lang_frFR","Description_Lang_deDE","Description_Lang_enCN","Description_Lang_zhCN",
"Description_Lang_enTW","Description_Lang_zhTW","Description_Lang_esES","Description_Lang_esMX",
"Description_Lang_ruRU","Description_Lang_ptPT","Description_Lang_ptBR","Description_Lang_itIT",
"Description_Lang_Unk","Description_Lang_Mask","AuraDescription_Lang_enUS","AuraDescription_Lang_enGB",
"AuraDescription_Lang_koKR","AuraDescription_Lang_frFR","AuraDescription_Lang_deDE","AuraDescription_Lang_enCN",
"AuraDescription_Lang_zhCN","AuraDescription_Lang_enTW","AuraDescription_Lang_zhTW","AuraDescription_Lang_esES",
"AuraDescription_Lang_esMX","AuraDescription_Lang_ruRU","AuraDescription_Lang_ptPT","AuraDescription_Lang_ptBR",
"AuraDescription_Lang_itIT","AuraDescription_Lang_Unk","AuraDescription_Lang_Mask","ManaCostPct",
"StartRecoveryCategory","StartRecoveryTime","MaxTargetLevel","SpellClassSet","SpellClassMask_1",
"SpellClassMask_2","SpellClassMask_3","MaxTargets","DefenseType","PreventionType","StanceBarOrder",
"EffectChainAmplitude_1","EffectChainAmplitude_2","EffectChainAmplitude_3","MinFactionID","MinReputation",
"RequiredAuraVision","RequiredTotemCategoryID_1","RequiredTotemCategoryID_2","RequiredAreasID","SchoolMask",
"RuneCostID","SpellMissileID","PowerDisplayID","EffectBonusMultiplier_1","EffectBonusMultiplier_2",
"EffectBonusMultiplier_3","SpellDescriptionVariableID","SpellDifficultyID"]

# 默认值（来自 38405）
SPELL_DEFAULTS = {
    "Category":0,"DispelType":0,"Mechanic":0,"Attributes":256,"AttributesEx":268435456,
    "AttributesEx2":0,"AttributesEx3":0,"AttributesEx4":0,"AttributesEx5":0,"AttributesEx6":0,"AttributesEx7":0,
    "ShapeshiftMask":0,"unk_320_2":0,"ShapeshiftExclude":0,"unk_320_3":0,"Targets":0,"TargetCreatureType":0,
    "RequiresSpellFocus":0,"FacingCasterFlags":0,"CasterAuraState":0,"TargetAuraState":0,
    "ExcludeCasterAuraState":0,"ExcludeTargetAuraState":0,"CasterAuraSpell":0,"TargetAuraSpell":0,
    "ExcludeCasterAuraSpell":0,"ExcludeTargetAuraSpell":0,"CastingTimeIndex":1,"RecoveryTime":0,
    "CategoryRecoveryTime":0,"InterruptFlags":0,"AuraInterruptFlags":0,"ChannelInterruptFlags":0,
    "ProcTypeMask":0,"ProcChance":101,"ProcCharges":0,"MaxLevel":0,"BaseLevel":0,"SpellLevel":0,
    "DurationIndex":21,"PowerType":0,"ManaCost":0,"ManaCostPerLevel":0,"ManaPerSecond":0,
    "ManaPerSecondPerLevel":0,"RangeIndex":1,"Speed":0,"ModalNextSpell":0,"CumulativeAura":10,
    "Totem_1":0,"Totem_2":0,"Reagent_1":0,"Reagent_2":0,"Reagent_3":0,"Reagent_4":0,"Reagent_5":0,
    "Reagent_6":0,"Reagent_7":0,"Reagent_8":0,"ReagentCount_1":0,"ReagentCount_2":0,"ReagentCount_3":0,
    "ReagentCount_4":0,"ReagentCount_5":0,"ReagentCount_6":0,"ReagentCount_7":0,"ReagentCount_8":0,
    "EquippedItemClass":-1,"EquippedItemSubclass":0,"EquippedItemInvTypes":0,
    "EffectRealPointsPerLevel_1":0,"EffectRealPointsPerLevel_2":0,"EffectRealPointsPerLevel_3":0,
    "EffectMechanic_1":0,"EffectMechanic_2":0,"EffectMechanic_3":0,
    "ImplicitTargetB_1":0,"ImplicitTargetB_2":0,"ImplicitTargetB_3":0,
    "EffectRadiusIndex_1":0,"EffectRadiusIndex_2":0,"EffectRadiusIndex_3":0,
    "EffectAuraPeriod_1":0,"EffectAuraPeriod_2":0,"EffectAuraPeriod_3":0,
    "EffectMultipleValue_1":0,"EffectMultipleValue_2":0,"EffectMultipleValue_3":0,
    "EffectChainTargets_1":0,"EffectChainTargets_2":0,"EffectChainTargets_3":0,
    "EffectItemType_1":0,"EffectItemType_2":0,"EffectItemType_3":0,
    "EffectMiscValueB_1":0,"EffectMiscValueB_2":0,"EffectMiscValueB_3":0,
    "EffectTriggerSpell_1":0,"EffectTriggerSpell_2":0,"EffectTriggerSpell_3":0,
    "EffectPointsPerCombo_1":0,"EffectPointsPerCombo_2":0,"EffectPointsPerCombo_3":0,
    "EffectSpellClassMaskA_1":0,"EffectSpellClassMaskA_2":0,"EffectSpellClassMaskA_3":0,
    "EffectSpellClassMaskB_1":0,"EffectSpellClassMaskB_2":0,"EffectSpellClassMaskB_3":0,
    "EffectSpellClassMaskC_1":0,"EffectSpellClassMaskC_2":0,"EffectSpellClassMaskC_3":0,
    "SpellVisualID_1":0,"SpellVisualID_2":0,"SpellIconID":0,"ActiveIconID":0,"SpellPriority":0,
    "Name_Lang_Mask":16712190,"NameSubtext_Lang_Mask":16712190,
    "Description_Lang_Mask":16712190,"AuraDescription_Lang_Mask":16712190,
    "ManaCostPct":0,"StartRecoveryCategory":0,"StartRecoveryTime":0,"MaxTargetLevel":0,
    "SpellClassSet":0,"SpellClassMask_1":0,"SpellClassMask_2":0,"SpellClassMask_3":0,
    "MaxTargets":0,"DefenseType":0,"PreventionType":0,"StanceBarOrder":0,
    "EffectChainAmplitude_1":1,"EffectChainAmplitude_2":1,"EffectChainAmplitude_3":0,
    "MinFactionID":0,"MinReputation":0,"RequiredAuraVision":0,
    "RequiredTotemCategoryID_1":0,"RequiredTotemCategoryID_2":0,"RequiredAreasID":0,"SchoolMask":1,
    "RuneCostID":0,"SpellMissileID":0,"PowerDisplayID":0,
    "EffectBonusMultiplier_1":0,"EffectBonusMultiplier_2":0,"EffectBonusMultiplier_3":0,
    "SpellDescriptionVariableID":0,"SpellDifficultyID":0,
}

# ============================================================
# 光环 -> 描述文案（百分比实际值 = abs(EffectBasePoints + 1)）
# ============================================================
AURA_TEXT = {
    A_MOD_THREAT:         ("造成的威胁值提高{v}%", "Increases threat caused by {v}%."),
    A_MOD_DMG_PCT_DONE:   ("造成的伤害提高{v}%",   "Increases all damage caused by {v}%."),
    A_MOD_DMG_PCT_TAKEN:  ("受到的伤害降低{v}%",   "Reduces damage taken by {v}%."),
    A_MOD_INC_ENERGY_PCT: ("法力值上限提高{v}%",   "Increases maximum mana by {v}%."),
    A_MOD_INC_HEALTH_PCT: ("生命值上限提高{v}%",   "Increases maximum health by {v}%."),
    A_MOD_HEALING_PCT:    ("治疗效果提高{v}%",     "Increases healing done by {v}%."),
    A_MOD_TOTAL_STAT_PCT: ("所有属性提高{v}%",     "Increases all stats by {v}%."),
    A_MOD_CRIT_DMG_BONUS: ("暴击伤害提高{v}%",     "Increases critical strike damage by {v}%."),
}

def build_desc(effects):
    """根据光环效果生成含具体数值的 (中文描述, 中文光环描述, 英文描述, 英文光环描述)。"""
    zh, en = [], []
    for aura, base, _misc in effects:
        v = abs(base + 1)
        zt, et = AURA_TEXT[aura]
        zh.append(zt.format(v=v))
        en.append(et.format(v=v))
    return "使" + "，".join(zh) + "。", "，".join(zh) + "。", " ".join(en), " ".join(en)

def build_spell_row(spid, zh, en, effects):
    row = dict(SPELL_DEFAULTS)
    row["ID"] = spid
    row["Name_Lang_zhCN"] = zh
    row["Name_Lang_enUS"] = en
    desc_zh, aura_zh, desc_en, aura_en = build_desc(effects)
    row["Description_Lang_zhCN"] = desc_zh
    row["Description_Lang_enUS"] = desc_en
    row["AuraDescription_Lang_zhCN"] = aura_zh
    row["AuraDescription_Lang_enUS"] = aura_en
    for i in range(1, 4):
        row[f"Effect_{i}"] = 0
        row[f"EffectDieSides_{i}"] = 0
        row[f"EffectBasePoints_{i}"] = 0
        row[f"ImplicitTargetA_{i}"] = 0
        row[f"EffectAura_{i}"] = 0
        row[f"EffectMiscValue_{i}"] = 0
    for idx, (aura, base, misc) in enumerate(effects, start=1):
        row[f"Effect_{idx}"] = 6  # APPLY_AURA
        row[f"EffectDieSides_{idx}"] = 1
        row[f"EffectBasePoints_{idx}"] = base
        row[f"ImplicitTargetA_{idx}"] = 1  # self
        row[f"EffectAura_{idx}"] = aura
        row[f"EffectMiscValue_{idx}"] = misc
    return row

# ============================================================
# 生成服务端 SQL
# ============================================================
def sql_quote(s):
    if s is None:
        return "NULL"
    return "'" + str(s).replace("\\", "\\\\").replace("'", "\\'") + "'"

# 1) spell_dbc
with open(os.path.join(SQLDIR, "01_混沌宝石法术.sql"), "w", encoding="utf-8") as f:
    f.write("-- 自定义混沌宝石 16 个法术（被动光环）\n")
    f.write("DELETE FROM `spell_dbc` WHERE `ID` BETWEEN 93000 AND 93015;\n")
    for spid, zh, en, effects in SPELLS:
        row = build_spell_row(spid, zh, en, effects)
        cols = ",".join(f"`{c}`" for c in SPELL_FIELDS)
        vals = []
        for c in SPELL_FIELDS:
            v = row.get(c)
            if c.endswith(("_Lang_deDE", "_Lang_enGB", "_Lang_koKR", "_Lang_frFR", "_Lang_enCN",
                           "_Lang_enTW", "_Lang_zhTW", "_Lang_esES", "_Lang_esMX", "_Lang_ruRU",
                           "_Lang_ptPT", "_Lang_ptBR", "_Lang_itIT", "_Lang_Unk")):
                vals.append("NULL")
            elif isinstance(v, str):
                vals.append(sql_quote(v))
            else:
                vals.append(str(v) if v is not None else "0")
        f.write(f"INSERT INTO `spell_dbc` ({cols}) VALUES ({','.join(vals)});\n")

# 2) spellitemenchantment_dbc
ENCH_FIELDS = ["ID","Charges","Effect_1","Effect_2","Effect_3","EffectPointsMin_1","EffectPointsMin_2",
"EffectPointsMin_3","EffectPointsMax_1","EffectPointsMax_2","EffectPointsMax_3","EffectArg_1","EffectArg_2",
"EffectArg_3","Name_Lang_enUS","Name_Lang_enGB","Name_Lang_koKR","Name_Lang_frFR","Name_Lang_deDE",
"Name_Lang_enCN","Name_Lang_zhCN","Name_Lang_enTW","Name_Lang_zhTW","Name_Lang_esES","Name_Lang_esMX",
"Name_Lang_ruRU","Name_Lang_ptPT","Name_Lang_ptBR","Name_Lang_itIT","Name_Lang_Unk","Name_Lang_Mask",
"ItemVisual","Flags","Src_ItemID","Condition_Id","RequiredSkillID","RequiredSkillRank","MinLevel"]

with open(os.path.join(SQLDIR, "02_混沌宝石附魔.sql"), "w", encoding="utf-8") as f:
    f.write("-- 自定义混沌宝石 9 个附魔定义（type=3 EQUIP_SPELL）\n")
    f.write("DELETE FROM `spellitemenchantment_dbc` WHERE `ID` BETWEEN 50001 AND 50009;\n")
    for eid, category, desc, spells in ENCHANTS:
        display = ENCH_COLORS[category] + desc + "|r"
        vals = {
            "ID": eid, "Charges": 0,
            "Effect_1": 3, "Effect_2": 3 if len(spells) > 1 else 0, "Effect_3": 3 if len(spells) > 2 else 0,
            "EffectPointsMin_1": 0, "EffectPointsMin_2": 0, "EffectPointsMin_3": 0,
            "EffectPointsMax_1": 0, "EffectPointsMax_2": 0, "EffectPointsMax_3": 0,
            "EffectArg_1": spells[0] if len(spells) > 0 else 0,
            "EffectArg_2": spells[1] if len(spells) > 1 else 0,
            "EffectArg_3": spells[2] if len(spells) > 2 else 0,
            "Name_Lang_deDE": display,
            "Name_Lang_Mask": 16712190, "ItemVisual": 0, "Flags": 0, "Src_ItemID": ENCH_ITEM.get(eid, 0),
            "Condition_Id": 0, "RequiredSkillID": 0, "RequiredSkillRank": 0, "MinLevel": 0,
        }
        cols = ",".join(f"`{c}`" for c in ENCH_FIELDS)
        vs = []
        for c in ENCH_FIELDS:
            v = vals.get(c)
            if c.startswith("Name_Lang_") and c not in ("Name_Lang_deDE", "Name_Lang_Mask"):
                vs.append("NULL")
            elif isinstance(v, str):
                vs.append(sql_quote(v))
            else:
                vs.append(str(v) if v is not None else "0")
        f.write(f"INSERT INTO `spellitemenchantment_dbc` ({cols}) VALUES ({','.join(vs)});\n")

# 3) gemproperties_dbc
GEM_FIELDS = ["ID","Enchant_Id","Maxcount_Inv","Maxcount_Item","Type"]
with open(os.path.join(SQLDIR, "03_混沌宝石属性.sql"), "w", encoding="utf-8") as f:
    f.write("-- 自定义混沌宝石 9 个 GemProperties（Type=1 多彩）\n")
    f.write("DELETE FROM `gemproperties_dbc` WHERE `ID` BETWEEN 5001 AND 5009;\n")
    for gid, eid in GEMPROPS:
        f.write(f"INSERT INTO `gemproperties_dbc` (`ID`,`Enchant_Id`,`Maxcount_Inv`,`Maxcount_Item`,`Type`) VALUES ({gid},{eid},0,0,1);\n")

# 4) item_template（精简字段，仅输出非默认字段）
with open(os.path.join(SQLDIR, "04_混沌宝石物品.sql"), "w", encoding="utf-8") as f:
    f.write("-- 自定义混沌宝石物品本体（材料 + 9 颗多彩宝石，精简字段）\n")
    f.write("DELETE FROM `item_template` WHERE `entry` BETWEEN 63100 AND 63110;\n")
    for item in MATERIALS + ITEMS:
        cols = ",".join(f"`{k}`" for k in item)
        vs = [sql_quote(v) if isinstance(v, str) else str(v) for v in item.values()]
        f.write(f"INSERT INTO `item_template` ({cols}) VALUES ({','.join(vs)});\n")

print("服务端 SQL 已生成 ->", SQLDIR)

# ============================================================
# 生成客户端 DBC CSV（WDBX 格式：全值加引号）
# ============================================================

# Spell.csv header 字段名 -> spell_dbc 字段名 映射（WDBX 命名差异）
HEADER_MAP = {
    "AttributesExB":"AttributesEx2", "AttributesExC":"AttributesEx3", "AttributesExD":"AttributesEx4",
    "AttributesExE":"AttributesEx5", "AttributesExF":"AttributesEx6", "AttributesExG":"AttributesEx7",
    "Field227":"EffectBonusMultiplier_1", "Field228":"EffectBonusMultiplier_2", "Field229":"EffectBonusMultiplier_3",
}

# 读取现有 Spell.csv 的 header（232 字段）
spell_csv_header = None
src_spell_csv = os.path.join(OUT, "..", "Spell.csv")
with open(src_spell_csv, encoding="utf-8-sig") as f:
    r = csv.reader(f)
    spell_csv_header = next(r)

def csv_cell(v):
    return "" if v is None else v

# 客户端 DBC CSV 的本地化列对齐：
# 参考 outputs/Spell.csv，中文文本实际存放在 *_Lang_deDE 列（其余非 enUS 本地化列为空），
# 因此输出 CSV 时把中文写入 deDE 列，与 Spell.csv 严格保持一致。
def csv_locale_cell(row, h):
    if h.endswith("_Lang_deDE"):
        zh_key = h[: -len("deDE")] + "zhCN"
        if row.get(zh_key) not in (None, ""):
            return row[zh_key]
    if h.endswith("_Lang_zhCN"):
        return ""
    return row.get(HEADER_MAP.get(h, h))

# 1) Spell.csv（16 行追加，供 WDBX 导入）
with open(os.path.join(CSVDIR, "混沌宝石_Spell.dbc.csv"), "w", newline="", encoding="utf-8-sig") as f:
    w = csv.writer(f, quoting=csv.QUOTE_ALL)
    w.writerow(spell_csv_header)
    for spid, zh, en, effects in SPELLS:
        row = build_spell_row(spid, zh, en, effects)
        line = [csv_cell(csv_locale_cell(row, h)) for h in spell_csv_header]
        w.writerow(line)

# 2) SpellItemEnchantment.csv（9 行）
ENCH_CSV_FIELDS = ["ID","Charges","Effect_1","Effect_2","Effect_3","EffectPointsMin_1","EffectPointsMin_2",
"EffectPointsMin_3","EffectPointsMax_1","EffectPointsMax_2","EffectPointsMax_3","EffectArg_1","EffectArg_2",
"EffectArg_3","Name_Lang_enUS","Name_Lang_enGB","Name_Lang_koKR","Name_Lang_frFR","Name_Lang_deDE",
"Name_Lang_enCN","Name_Lang_zhCN","Name_Lang_enTW","Name_Lang_zhTW","Name_Lang_esES","Name_Lang_esMX",
"Name_Lang_ruRU","Name_Lang_ptPT","Name_Lang_ptBR","Name_Lang_itIT","Name_Lang_Unk","Name_Lang_Mask",
"ItemVisual","Flags","Src_ItemID","Condition_Id","RequiredSkillID","RequiredSkillRank","MinLevel"]

with open(os.path.join(CSVDIR, "混沌宝石_SpellItemEnchantment.dbc.csv"), "w", newline="", encoding="utf-8-sig") as f:
    w = csv.writer(f, quoting=csv.QUOTE_ALL)
    w.writerow(ENCH_CSV_FIELDS)
    for eid, category, desc, spells in ENCHANTS:
        display = ENCH_COLORS[category] + desc + "|r"
        vals = {
            "ID": eid, "Charges": 0,
            "Effect_1": 3, "Effect_2": 3 if len(spells) > 1 else 0, "Effect_3": 3 if len(spells) > 2 else 0,
            "EffectPointsMin_1": 0, "EffectPointsMin_2": 0, "EffectPointsMin_3": 0,
            "EffectPointsMax_1": 0, "EffectPointsMax_2": 0, "EffectPointsMax_3": 0,
            "EffectArg_1": spells[0] if len(spells) > 0 else 0,
            "EffectArg_2": spells[1] if len(spells) > 1 else 0,
            "EffectArg_3": spells[2] if len(spells) > 2 else 0,
            "Name_Lang_deDE": display,
            "Name_Lang_Mask": 16712190, "ItemVisual": 0, "Flags": 0, "Src_ItemID": ENCH_ITEM.get(eid, 0),
            "Condition_Id": 0, "RequiredSkillID": 0, "RequiredSkillRank": 0, "MinLevel": 0,
        }
        w.writerow([csv_cell(vals.get(c)) for c in ENCH_CSV_FIELDS])

# 3) GemProperties.csv（9 行）
GEM_CSV_FIELDS = ["ID","Enchant_Id","Maxcount_Inv","Maxcount_Item","Type"]
with open(os.path.join(CSVDIR, "混沌宝石_GemProperties.dbc.csv"), "w", newline="", encoding="utf-8-sig") as f:
    w = csv.writer(f, quoting=csv.QUOTE_ALL)
    w.writerow(GEM_CSV_FIELDS)
    for gid, eid in GEMPROPS:
        w.writerow([gid, eid, 0, 0, 1])

# 4) Item.dbc.csv（11 行：2 材料 + 9 宝石）
ITEM_DBC_FIELDS = ["ID","ClassID","SubclassID","Sound_Override_Subclassid","Material","DisplayInfoID","InventoryType","SheatheType"]
with open(os.path.join(CSVDIR, "混沌宝石_Item.dbc.csv"), "w", newline="", encoding="utf-8-sig") as f:
    w = csv.writer(f, quoting=csv.QUOTE_ALL)
    w.writerow(ITEM_DBC_FIELDS)
    for item in MATERIALS + ITEMS:
        w.writerow([item["entry"], item["class"], item["subclass"], -1,
                    item["Material"], item["displayid"], 0, 0])

print("客户端 DBC CSV 已生成 ->", CSVDIR)
print("Spell.csv header 字段数:", len(spell_csv_header))
