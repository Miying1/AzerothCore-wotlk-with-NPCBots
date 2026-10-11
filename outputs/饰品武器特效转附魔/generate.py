# -*- coding: utf-8 -*-
"""
饰品特效转附魔卷轴 —— 独立生成脚本
====================================

读取「本地服务端库 item_template + 客户端 Spell.csv」，提取饰品（InventoryType=12）
上的 proc 特效，生成：

  1. 服务端 SQL  -> modules/mod-random-enchants/sql/world/trinket_enchant_scrolls.sql
       · item_template                 附魔卷轴（class=0 消耗品 / subclass=8 附魔）
       · spell_enchant_proc_data       命中触发型附魔的 proc 数据（本批饰品为空）
       · mod_trinket_essence_map       特效法术 -> 附魔卷轴 对应表（C++ 运行时查表用）
       附魔定义 / 附魔法术默认**不写库**（WRITE_DBC_TABLES = False），只随下面的 DBC CSV 导入客户端 DBC；
       SQL 里仍保留对应区间的 DELETE，用于清理早期版本写进 spellitemenchantment_dbc / spell_dbc 的覆盖行。
  2. 客户端 DBC CSV -> dbc/*.csv（WDBX 导入用）
       · TrinketEnchant_SpellItemEnchantment.dbc.csv
       · TrinketEnchant_Spell.dbc.csv
       · TrinketEnchant_Item.dbc.csv
  3. 提取清单 -> 可用饰品特效清单.csv

配套的运行时脚本：modules/mod-random-enchants/src/TrinketEssence.cpp（道具「饰品特效提取器」60105，消耗品）

链路：
  饰品 spelltrigger=1 (装备被动 proc)
      └─> SpellItemEnchantment(Effect=3, EffectArg=原法术ID)
            └─> Spell(Effect=53, EffectMiscValue=附魔ID)
                  └─> item_template 卷轴(spellid_1=法术, spelltrigger_1=0)

用法：
  python generate.py            # 生成全部产物
  python generate.py --list     # 仅打印将要生成的特效清单

依赖：本机 mysql 客户端可执行文件（见下方 MYSQL_EXE）、<仓库根>/outputs/Spell.csv
"""

import csv
import os
import subprocess
import sys


def safe_open(name, directory):
    """输出文件被 Excel 等程序占用时，改写到 <同名>.new，避免生成流程中断。"""
    path = os.path.join(directory, name)
    try:
        return open(path, "w", encoding="utf-8-sig", newline=""), path
    except PermissionError:
        alt = path + ".new"
        print("!! %s 被占用，改写到 %s" % (name, alt))
        return open(alt, "w", encoding="utf-8-sig", newline=""), alt

# ============================================================
# 一、运行环境配置
# ============================================================
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))                  # outputs/饰品武器特效转附魔
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))        # 仓库根
MODULE_DIR = os.path.join(REPO_ROOT, "modules", "mod-random-enchants")   # 模块目录

MYSQL_EXE = r"E:\wlk80Server\mysql8.0\bin\mysql.exe"
DB_USER = "root"
DB_PASS = "wow@80"
DB_NAME = "acore_world"

SPELL_CSV = os.path.join(REPO_ROOT, "outputs", "Spell.csv")

SQL_OUT = os.path.join(MODULE_DIR, "sql", "world", "trinket_enchant_scrolls.sql")
DBC_OUT_DIR = os.path.join(SCRIPT_DIR, "dbc")
LIST_OUT = os.path.join(SCRIPT_DIR, "可用饰品特效清单.csv")

# 是否把 DBC 数据（附魔定义 / 附魔法术）也写进数据库表。
# False = DBC 数据只走客户端 DBC 导入（推荐）：SQL 里只保留 DELETE 用于清理早期版本写入的覆盖行，
#         实际数据由 spellitemenchantment_dbc / spell_dbc 之外的两份 DBC CSV 导入客户端 DBC 得到。
# True  = 额外生成 spellitemenchantment_dbc / spell_dbc 的 INSERT（服务端 LoadFromDB 覆盖）。
WRITE_DBC_TABLES = False

# ============================================================
# 二、提取范围
# ============================================================
MIN_ITEM_LEVEL = 200
INVENTORY_TYPE_TRINKET = 12          # INVTYPE_TRINKET
TRIGGER_ON_EQUIP = 1                 # ITEM_SPELLTRIGGER_ON_EQUIP：装备被动，走 EQUIP_SPELL
TRIGGER_CHANCE_ON_HIT = 2            # ITEM_SPELLTRIGGER_CHANCE_ON_HIT：命中触发，走 COMBAT_SPELL

# ============================================================
# 三、ID 分配（已核对：三张表在该区间均为空）
# ============================================================
ENCHANT_ID_START = 30000             # spellitemenchantment_dbc.ID
SPELL_ID_START = 94500               # spell_dbc.ID
ITEM_ID_START = 63200                # item_template.entry
ID_BLOCK = 100                       # 每类预留 100 个 ID

# 运行时映射表：饰品特效法术 -> 附魔卷轴（供 src/TrinketEssence.cpp 查询）
MAPPING_TABLE = "mod_trinket_essence_map"
MAPPING_DDL = """
CREATE TABLE IF NOT EXISTS `%s` (
  `spell_id`         INT UNSIGNED NOT NULL COMMENT '饰品上的原特效法术 ID',
  `enchant_id`       INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'spellitemenchantment_dbc.ID',
  `enchant_spell_id` INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '附魔法术 spell_dbc.ID',
  `scroll_item`      INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '附魔卷轴 item_template.entry',
  `name`             VARCHAR(100) NOT NULL DEFAULT '' COMMENT '显示名',
  `src_item`         INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '来源饰品 entry',
  `src_spell_name`   VARCHAR(100) NOT NULL DEFAULT '' COMMENT '原特效法术名',
  PRIMARY KEY (`spell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='饰品特效提取器：特效法术 -> 附魔卷轴 对应表';
""" % MAPPING_TABLE
MAPPING_FIELDS = ["spell_id", "enchant_id", "enchant_spell_id", "scroll_item", "name",
                  "src_item", "src_spell_name"]

# ============================================================
# 四、命名与显示
#    卷轴物品名 与 附魔名/附魔法术名 用不同前缀
# ============================================================
SCROLL_NAME_PREFIX = "武器附魔·"       # 附魔卷轴物品名（item_template.name），颜色由 Quality=4 呈现
ENCHANT_NAME_PREFIX = "魄·"            # 附魔名（SpellItemEnchantment.dbc）/ 附魔法术名（Spell.dbc）
NAME_COLOR = "|cffa335ee"            # 附魔名紫色（|cAARRGGBB）
NAME_COLOR_END = "|r"

# ============================================================
# 五、附魔法术模板（取自 Spell.csv 中的真实武器附魔法术）
#    35793 = 附魔武器 - 猫鼬（Effect_1=53 ENCHANT_ITEM）
# ============================================================
TEMPLATE_SPELL_ID = 35793

EQUIPPED_ITEM_CLASS = 2              # 只能附魔到武器（ITEM_CLASS_WEAPON）
# 武器子类掩码：0-20 位，排除 9/11/12（已废弃/未使用子类），覆盖单手/双手/远程等
WEAPON_SUBCLASS_MASK = sum(1 << b for b in range(21) if b not in (9, 11, 12))
EQUIPPED_ITEM_INV_TYPES = 0          # 真实武器附魔均为 0（过滤靠子类掩码）
SPELL_BASE_LEVEL = 0                 # 0 = 不限制目标装备等级（Spell.cpp 中 requiredLevel < BaseLevel 才拦截）
SPELL_MAX_LEVEL = 0                  # 0 = 不限制最高装等
SPELL_ICON_ID = 241                  # 技能图标：241 = 模板法术 35793（附魔武器 - 猫鼬）的图标
SPELL_ACTIVE_ICON_ID = 0             # 高亮图标：0 = 默认

# ============================================================
# 六、卷轴物品模板（参考库内 29839「附魔卷轴」）
# ============================================================
SCROLL_DISPLAY_ID = 11161            # 附魔卷轴图标（ItemDisplayInfo.dbc）
SCROLL_QUALITY = 4                   # 4 紫色品质（客户端按品质着色）
SCROLL_MATERIAL = 8
SCROLL_ITEM_LEVEL = 1
SCROLL_REQUIRED_LEVEL = 0
SCROLL_STACKABLE = 1
SCROLL_BONDING = 0
SCROLL_BUY_PRICE = 0
SCROLL_SELL_PRICE = 0

# ============================================================
# 七、表字段定义
# ============================================================
# spell_dbc 表列顺序（234 列，含 unk_320_2 / unk_320_3 / EffectBonusMultiplier_*）
SPELL_DBC_FIELDS = [
    "ID", "Category", "DispelType", "Mechanic", "Attributes", "AttributesEx",
    "AttributesEx2", "AttributesEx3", "AttributesEx4", "AttributesEx5", "AttributesEx6", "AttributesEx7",
    "ShapeshiftMask", "unk_320_2", "ShapeshiftExclude", "unk_320_3", "Targets", "TargetCreatureType",
    "RequiresSpellFocus", "FacingCasterFlags", "CasterAuraState", "TargetAuraState", "ExcludeCasterAuraState",
    "ExcludeTargetAuraState", "CasterAuraSpell", "TargetAuraSpell", "ExcludeCasterAuraSpell", "ExcludeTargetAuraSpell",
    "CastingTimeIndex", "RecoveryTime", "CategoryRecoveryTime", "InterruptFlags", "AuraInterruptFlags",
    "ChannelInterruptFlags", "ProcTypeMask", "ProcChance", "ProcCharges", "MaxLevel", "BaseLevel", "SpellLevel",
    "DurationIndex", "PowerType", "ManaCost", "ManaCostPerLevel", "ManaPerSecond", "ManaPerSecondPerLevel",
    "RangeIndex", "Speed", "ModalNextSpell", "CumulativeAura", "Totem_1", "Totem_2",
    "Reagent_1", "Reagent_2", "Reagent_3", "Reagent_4", "Reagent_5", "Reagent_6", "Reagent_7", "Reagent_8",
    "ReagentCount_1", "ReagentCount_2", "ReagentCount_3", "ReagentCount_4", "ReagentCount_5",
    "ReagentCount_6", "ReagentCount_7", "ReagentCount_8", "EquippedItemClass", "EquippedItemSubclass",
    "EquippedItemInvTypes", "Effect_1", "Effect_2", "Effect_3", "EffectDieSides_1", "EffectDieSides_2",
    "EffectDieSides_3", "EffectRealPointsPerLevel_1", "EffectRealPointsPerLevel_2", "EffectRealPointsPerLevel_3",
    "EffectBasePoints_1", "EffectBasePoints_2", "EffectBasePoints_3", "EffectMechanic_1", "EffectMechanic_2",
    "EffectMechanic_3", "ImplicitTargetA_1", "ImplicitTargetA_2", "ImplicitTargetA_3", "ImplicitTargetB_1",
    "ImplicitTargetB_2", "ImplicitTargetB_3", "EffectRadiusIndex_1", "EffectRadiusIndex_2", "EffectRadiusIndex_3",
    "EffectAura_1", "EffectAura_2", "EffectAura_3", "EffectAuraPeriod_1", "EffectAuraPeriod_2", "EffectAuraPeriod_3",
    "EffectMultipleValue_1", "EffectMultipleValue_2", "EffectMultipleValue_3", "EffectChainTargets_1",
    "EffectChainTargets_2", "EffectChainTargets_3", "EffectItemType_1", "EffectItemType_2", "EffectItemType_3",
    "EffectMiscValue_1", "EffectMiscValue_2", "EffectMiscValue_3", "EffectMiscValueB_1", "EffectMiscValueB_2",
    "EffectMiscValueB_3", "EffectTriggerSpell_1", "EffectTriggerSpell_2", "EffectTriggerSpell_3",
    "EffectPointsPerCombo_1", "EffectPointsPerCombo_2", "EffectPointsPerCombo_3", "EffectSpellClassMaskA_1",
    "EffectSpellClassMaskA_2", "EffectSpellClassMaskA_3", "EffectSpellClassMaskB_1", "EffectSpellClassMaskB_2",
    "EffectSpellClassMaskB_3", "EffectSpellClassMaskC_1", "EffectSpellClassMaskC_2", "EffectSpellClassMaskC_3",
    "SpellVisualID_1", "SpellVisualID_2", "SpellIconID", "ActiveIconID", "SpellPriority",
    "Name_Lang_enUS", "Name_Lang_enGB", "Name_Lang_koKR", "Name_Lang_frFR", "Name_Lang_deDE", "Name_Lang_enCN",
    "Name_Lang_zhCN", "Name_Lang_enTW", "Name_Lang_zhTW", "Name_Lang_esES", "Name_Lang_esMX", "Name_Lang_ruRU",
    "Name_Lang_ptPT", "Name_Lang_ptBR", "Name_Lang_itIT", "Name_Lang_Unk", "Name_Lang_Mask",
    "NameSubtext_Lang_enUS", "NameSubtext_Lang_enGB", "NameSubtext_Lang_koKR", "NameSubtext_Lang_frFR",
    "NameSubtext_Lang_deDE", "NameSubtext_Lang_enCN", "NameSubtext_Lang_zhCN", "NameSubtext_Lang_enTW",
    "NameSubtext_Lang_zhTW", "NameSubtext_Lang_esES", "NameSubtext_Lang_esMX", "NameSubtext_Lang_ruRU",
    "NameSubtext_Lang_ptPT", "NameSubtext_Lang_ptBR", "NameSubtext_Lang_itIT", "NameSubtext_Lang_Unk",
    "NameSubtext_Lang_Mask", "Description_Lang_enUS", "Description_Lang_enGB", "Description_Lang_koKR",
    "Description_Lang_frFR", "Description_Lang_deDE", "Description_Lang_enCN", "Description_Lang_zhCN",
    "Description_Lang_enTW", "Description_Lang_zhTW", "Description_Lang_esES", "Description_Lang_esMX",
    "Description_Lang_ruRU", "Description_Lang_ptPT", "Description_Lang_ptBR", "Description_Lang_itIT",
    "Description_Lang_Unk", "Description_Lang_Mask", "AuraDescription_Lang_enUS", "AuraDescription_Lang_enGB",
    "AuraDescription_Lang_koKR", "AuraDescription_Lang_frFR", "AuraDescription_Lang_deDE", "AuraDescription_Lang_enCN",
    "AuraDescription_Lang_zhCN", "AuraDescription_Lang_enTW", "AuraDescription_Lang_zhTW", "AuraDescription_Lang_esES",
    "AuraDescription_Lang_esMX", "AuraDescription_Lang_ruRU", "AuraDescription_Lang_ptPT", "AuraDescription_Lang_ptBR",
    "AuraDescription_Lang_itIT", "AuraDescription_Lang_Unk", "AuraDescription_Lang_Mask", "ManaCostPct",
    "StartRecoveryCategory", "StartRecoveryTime", "MaxTargetLevel", "SpellClassSet", "SpellClassMask_1",
    "SpellClassMask_2", "SpellClassMask_3", "MaxTargets", "DefenseType", "PreventionType", "StanceBarOrder",
    "EffectChainAmplitude_1", "EffectChainAmplitude_2", "EffectChainAmplitude_3", "MinFactionID", "MinReputation",
    "RequiredAuraVision", "RequiredTotemCategoryID_1", "RequiredTotemCategoryID_2", "RequiredAreasID", "SchoolMask",
    "RuneCostID", "SpellMissileID", "PowerDisplayID", "EffectBonusMultiplier_1", "EffectBonusMultiplier_2",
    "EffectBonusMultiplier_3", "SpellDescriptionVariableID", "SpellDifficultyID",
]

# 客户端 Spell.csv 列名 <-> spell_dbc 列名 差异映射
CSV_TO_DB_COL = {
    "AttributesExB": "AttributesEx2", "AttributesExC": "AttributesEx3", "AttributesExD": "AttributesEx4",
    "AttributesExE": "AttributesEx5", "AttributesExF": "AttributesEx6", "AttributesExG": "AttributesEx7",
    "Field227": "EffectBonusMultiplier_1", "Field228": "EffectBonusMultiplier_2",
    "Field229": "EffectBonusMultiplier_3",
}
DB_TO_CSV_COL = {v: k for k, v in CSV_TO_DB_COL.items()}

# spellitemenchantment_dbc 列顺序（38 列）
ENCHANT_DBC_FIELDS = [
    "ID", "Charges", "Effect_1", "Effect_2", "Effect_3", "EffectPointsMin_1", "EffectPointsMin_2",
    "EffectPointsMin_3", "EffectPointsMax_1", "EffectPointsMax_2", "EffectPointsMax_3", "EffectArg_1",
    "EffectArg_2", "EffectArg_3", "Name_Lang_enUS", "Name_Lang_enGB", "Name_Lang_koKR", "Name_Lang_frFR",
    "Name_Lang_deDE", "Name_Lang_enCN", "Name_Lang_zhCN", "Name_Lang_enTW", "Name_Lang_zhTW",
    "Name_Lang_esES", "Name_Lang_esMX", "Name_Lang_ruRU", "Name_Lang_ptPT", "Name_Lang_ptBR",
    "Name_Lang_itIT", "Name_Lang_Unk", "Name_Lang_Mask", "ItemVisual", "Flags", "Src_ItemID",
    "Condition_Id", "RequiredSkillID", "RequiredSkillRank", "MinLevel",
]

# item_template 精简插入字段（未列出的字段使用数据库默认值）
ITEM_FIELDS = [
    "entry", "class", "subclass", "SoundOverrideSubclass", "name", "displayid", "Quality", "Flags",
    "BuyCount", "BuyPrice", "SellPrice", "InventoryType", "AllowableClass", "AllowableRace", "ItemLevel",
    "RequiredLevel", "stackable", "bonding", "Material", "delay", "description",
    "spellid_1", "spelltrigger_1", "spellcharges_1", "spellppmRate_1",
    "spellcooldown_1", "spellcategory_1", "spellcategorycooldown_1",
]

# Item.dbc 列顺序
ITEM_DBC_FIELDS = ["ID", "ClassID", "SubclassID", "Sound_Override_Subclassid", "Material",
                   "DisplayInfoID", "InventoryType", "SheatheType"]

# spell_enchant_proc_data 列顺序
PROC_FIELDS = ["entry", "customChance", "PPMChance", "procEx", "attributeMask"]

# 本地化列后缀（除 deDE 外，其余置 NULL/空）
LOCALE_SUFFIXES = ("enUS", "enGB", "koKR", "frFR", "deDE", "enCN", "zhCN", "enTW",
                   "zhTW", "esES", "esMX", "ruRU", "ptPT", "ptBR", "itIT", "Unk")


# ============================================================
# 工具函数
# ============================================================
def run_mysql(query):
    """执行 SQL 并返回制表符分隔的行列表（utf-8 解码）。"""
    cmd = [MYSQL_EXE, "-u", DB_USER, "-p" + DB_PASS, "-D", DB_NAME,
           "--default-character-set=utf8mb4", "-B", "-N", "-e", query]
    proc = subprocess.run(cmd, capture_output=True)
    if proc.returncode != 0:
        raise RuntimeError("mysql 执行失败：\n" + proc.stderr.decode("utf-8", "replace"))
    out = proc.stdout.decode("utf-8", "replace")
    return [line.split("\t") for line in out.splitlines() if line.strip()]


def sql_quote(value):
    """转义为 SQL 字符串字面量。"""
    return "'" + str(value).replace("\\", "\\\\").replace("'", "\\'") + "'"


def sql_value(value):
    """按类型输出 SQL 字面量。"""
    if value is None:
        return "NULL"
    if isinstance(value, str):
        return sql_quote(value)
    return str(value)


def is_locale_col(col):
    for suffix in LOCALE_SUFFIXES:
        if col.endswith("_Lang_" + suffix):
            return True
    return False


def display_name(item_name):
    """附魔显示名：紫色 + 魄· + 饰品名（附魔窗口里显示的名字）。"""
    return NAME_COLOR + ENCHANT_NAME_PREFIX + item_name + NAME_COLOR_END


def scroll_item_name(item_name):
    """附魔卷轴物品名：武器附魔· + 饰品名（不带颜色码）。"""
    return SCROLL_NAME_PREFIX + item_name


# ============================================================
# 步骤 1：读取 Spell.csv（法术定义）
# ============================================================
def load_spell_csv():
    if not os.path.isfile(SPELL_CSV):
        sys.exit("找不到 Spell.csv：%s\n请在配置区修正 SPELL_CSV 路径。" % SPELL_CSV)

    spells, header, rows = {}, None, {}
    with open(SPELL_CSV, encoding="utf-8-sig", newline="") as f:
        reader = csv.reader(f)
        header = next(reader)
        idx = {name: i for i, name in enumerate(header)}
        need = [TEMPLATE_SPELL_ID]
        for row in reader:
            if not row or not row[0]:
                continue
            sid = int(row[idx["ID"]])
            if sid in need:
                rows[sid] = [c for c in row]
            # 只保留判定 proc 需要的列，控制内存
            spells[sid] = {
                "proc": int(row[idx["ProcTypeMask"]] or 0),
                "chance": int(row[idx["ProcChance"]] or 0),
                "zh": row[idx["Name_Lang_deDE"]] or row[idx["Name_Lang_zhCN"]],
                "en": row[idx["Name_Lang_enUS"]],
            }
    if TEMPLATE_SPELL_ID not in rows:
        sys.exit("Spell.csv 中找不到附魔法术模板 %d。" % TEMPLATE_SPELL_ID)
    return spells, header, rows


# ============================================================
# 步骤 2：从 item_template 提取饰品特效
# ============================================================
def extract_effects():
    cols = ["entry", "name", "ItemLevel", "InventoryType"]
    for i in range(1, 6):
        cols += ["spellid_%d" % i, "spelltrigger_%d" % i, "spellppmRate_%d" % i]
    query = ("SELECT %s FROM item_template WHERE ItemLevel >= %d AND InventoryType = %d ORDER BY entry"
             % (",".join(cols), MIN_ITEM_LEVEL, INVENTORY_TYPE_TRINKET))

    effects = {}
    for r in run_mysql(query):
        entry, name, ilvl, inv_type = int(r[0]), r[1], int(r[2]), int(r[3])
        for s in range(5):
            base = 4 + s * 3
            sid, trigger = int(r[base] or 0), int(r[base + 1] or 0)
            ppm = float(r[base + 2] or 0)
            if sid <= 0 or trigger not in (TRIGGER_ON_EQUIP, TRIGGER_CHANCE_ON_HIT):
                continue
            info = effects.setdefault(sid, {"spell": sid, "trigger": trigger, "ppm": ppm, "items": []})
            info["ppm"] = max(info["ppm"], ppm)
            info["trigger"] = min(info["trigger"], trigger)      # 装备被动优先
            info["items"].append((entry, ilvl, name))
    return effects


def pick_item_name(info):
    """取来源饰品中 entry 最小的那个的名字。"""
    if not info["items"]:
        return ""
    return sorted(info["items"], key=lambda x: x[0])[0][2].strip()


# ============================================================
# 步骤 3：判定可用 proc 特效
# ============================================================
def classify(effects, spells):
    usable, excluded = [], []
    for sid in sorted(effects):
        info = effects[sid]
        sp = spells.get(sid)
        if sp is None:
            info["reason"] = "Spell.csv 未找到定义"
            excluded.append(info)
            continue
        info["spell_zh"], info["spell_en"] = sp["zh"], sp["en"]
        info["proc_mask"], info["proc_chance"] = sp["proc"], sp["chance"]
        info["item_name"] = pick_item_name(info)
        if info["trigger"] == TRIGGER_CHANCE_ON_HIT:
            # 命中触发：由物品/附魔的 PPM / ProcChance 机制驱动，不看法术自身 ProcTypeMask
            usable.append(info)
        elif sp["proc"] > 0:
            usable.append(info)
        else:
            info["reason"] = "ProcTypeMask=0（纯被动属性光环）"
            excluded.append(info)
    return usable, excluded


# ============================================================
# 步骤 4：构造附魔法术行（基于 Spell.csv 模板）
# ============================================================
def build_spell_row(template_dict, spell_id, enchant_id, zh_name, en_name, zh_desc, en_desc):
    """返回 (客户端列名 -> 值) 的字典，键与 Spell.csv 表头一致。"""
    row = dict(template_dict)

    def clear_locale(prefix):
        for h in list(row):
            if h.startswith(prefix) and not h.endswith("_Mask"):
                row[h] = ""

    clear_locale("Name_Lang_")
    clear_locale("NameSubtext_Lang_")
    clear_locale("Description_Lang_")
    clear_locale("AuraDescription_Lang_")

    row["ID"] = str(spell_id)
    row["EquippedItemClass"] = str(EQUIPPED_ITEM_CLASS)
    row["EquippedItemSubclass"] = str(WEAPON_SUBCLASS_MASK)
    row["EquippedItemInvTypes"] = str(EQUIPPED_ITEM_INV_TYPES)
    row["BaseLevel"] = str(SPELL_BASE_LEVEL)
    row["MaxLevel"] = str(SPELL_MAX_LEVEL)
    row["SpellIconID"] = str(SPELL_ICON_ID)
    row["ActiveIconID"] = str(SPELL_ACTIVE_ICON_ID)
    row["EffectMiscValue_1"] = str(enchant_id)
    # 中文写 deDE 列（本项目客户端中文均存放于 deDE），英文写 enUS
    row["Name_Lang_deDE"] = zh_name
    row["Name_Lang_enUS"] = en_name
    row["Description_Lang_deDE"] = zh_desc
    row["Description_Lang_enUS"] = en_desc
    return row


def client_row_to_db_row(client_row):
    """客户端列名 -> spell_dbc 列名（补 unk_320_2 / unk_320_3）。"""
    db_row = {}
    for col in SPELL_DBC_FIELDS:
        if col in ("unk_320_2", "unk_320_3"):
            db_row[col] = "0"
            continue
        db_row[col] = client_row.get(DB_TO_CSV_COL.get(col, col), "0")
    return db_row


# ============================================================
# 主流程
# ============================================================
def main():
    only_list = "--list" in sys.argv

    print("[1/4] 读取 Spell.csv：%s" % SPELL_CSV)
    spells, spell_header, template_rows = load_spell_csv()
    print("      法术定义 %d 条，模板法术 %d 已就绪" % (len(spells), TEMPLATE_SPELL_ID))

    print("[2/4] 读取 %s.item_template（ItemLevel >= %d，InventoryType = %d）"
          % (DB_NAME, MIN_ITEM_LEVEL, INVENTORY_TYPE_TRINKET))
    effects = extract_effects()
    print("      去重特效 %d 条" % len(effects))

    usable, excluded = classify(effects, spells)
    n_equip = sum(1 for u in usable if u["trigger"] == TRIGGER_ON_EQUIP)
    n_hit = sum(1 for u in usable if u["trigger"] == TRIGGER_CHANCE_ON_HIT)
    print("[3/4] 可用 proc 特效 %d 条（装备被动 %d / 命中触发 %d），排除 %d 条"
          % (len(usable), n_equip, n_hit, len(excluded)))

    if only_list:
        for i, info in enumerate(usable):
            print("  %d  法术%-6d 物品%-6d %s  <- %s"
                  % (ENCHANT_ID_START + i, info["spell"], info["items"][0][0],
                     display_name(info["item_name"]), info["spell_zh"]))
        return

    # ID 分配
    for i, info in enumerate(usable):
        info["enchant_id"] = ENCHANT_ID_START + i
        info["spell_id"] = SPELL_ID_START + i
        info["item_id"] = ITEM_ID_START + i

    os.makedirs(os.path.dirname(SQL_OUT), exist_ok=True)
    os.makedirs(DBC_OUT_DIR, exist_ok=True)

    enchant_rows, spell_client_rows, item_rows, proc_rows, mapping_rows = [], [], [], [], []
    template_client_row = {h: template_rows[TEMPLATE_SPELL_ID][i] for i, h in enumerate(spell_header)}

    for info in usable:
        name = display_name(info["item_name"])
        spell_name = (ENCHANT_NAME_PREFIX + info["item_name"]) if info["item_name"] \
            else ("%s特效 #%d" % (ENCHANT_NAME_PREFIX, info["spell"]))
        # 本地化口径与 自定义混沌宝石/混沌宝石_Spell.dbc.csv 保持一致：
        #   中文名/中文描述只写 deDE 列；enUS 列放英文（本项目这些 proc 法术没有英文名，故留空）
        spell_name_en = info.get("spell_en", "").strip()
        spell_zh_desc = "永久性地为一把武器附魔，使其获得「%s」的特效。" % (info["item_name"] or info["spell"])
        spell_en_desc = ("Permanently enchants a weapon with the effect of \"%s\"." % spell_name_en) \
            if spell_name_en else "Permanently enchants a weapon with a trinket's proc effect."

        # 1) 附魔定义
        enchant_rows.append({
            "ID": info["enchant_id"], "Charges": 0,
            # trigger=1 装备被动 -> 3 EQUIP_SPELL；trigger=2 命中触发 -> 1 COMBAT_SPELL
            "Effect_1": 3 if info["trigger"] == TRIGGER_ON_EQUIP else 1,
            "Effect_2": 0, "Effect_3": 0,
            "EffectPointsMin_1": 0, "EffectPointsMin_2": 0, "EffectPointsMin_3": 0,
            "EffectPointsMax_1": 0, "EffectPointsMax_2": 0, "EffectPointsMax_3": 0,
            "EffectArg_1": info["spell"], "EffectArg_2": 0, "EffectArg_3": 0,
            "Name_Lang_deDE": name, "Name_Lang_zhCN": name, "Name_Lang_Mask": 16712190,
            "ItemVisual": 0, "Flags": 0, "Src_ItemID": 0,
            "Condition_Id": 0, "RequiredSkillID": 0, "RequiredSkillRank": 0, "MinLevel": 0,
        })

        # 2) 附魔法术
        client_row = build_spell_row(template_client_row, info["spell_id"],
                                     info["enchant_id"], spell_name, spell_name_en,
                                     spell_zh_desc, spell_en_desc)
        spell_client_rows.append(client_row)

        # 3) 卷轴物品（物品名不带颜色码，紫色由 Quality 决定）
        item_rows.append({
            "entry": info["item_id"], "class": 0, "subclass": 8, "SoundOverrideSubclass": -1,
            "name": scroll_item_name(info["item_name"]),
            "displayid": SCROLL_DISPLAY_ID, "Quality": SCROLL_QUALITY, "Flags": 0,
            "BuyCount": 1, "BuyPrice": SCROLL_BUY_PRICE, "SellPrice": SCROLL_SELL_PRICE,
            "InventoryType": 0, "AllowableClass": -1, "AllowableRace": -1,
            "ItemLevel": SCROLL_ITEM_LEVEL, "RequiredLevel": SCROLL_REQUIRED_LEVEL,
            "stackable": SCROLL_STACKABLE, "bonding": SCROLL_BONDING, "Material": SCROLL_MATERIAL,
            "delay": 1000,
            "description": "将「%s」的特效永久附魔到一把武器上。" % (info["item_name"] or info["spell"]),
            "spellid_1": info["spell_id"], "spelltrigger_1": 0, "spellcharges_1": 0,
            "spellppmRate_1": 0, "spellcooldown_1": -1, "spellcategory_1": 0,
            "spellcategorycooldown_1": -1,
        })

        # 4) 命中触发的 proc 数据
        if info["trigger"] == TRIGGER_CHANCE_ON_HIT:
            proc_rows.append({"entry": info["enchant_id"], "customChance": 0,
                              "PPMChance": info["ppm"], "procEx": 0, "attributeMask": 0})

        # 5) 运行时映射：特效法术 -> 附魔卷轴
        mapping_rows.append({
            "spell_id": info["spell"], "enchant_id": info["enchant_id"],
            "enchant_spell_id": info["spell_id"], "scroll_item": info["item_id"],
            "name": name, "src_item": info["items"][0][0],
            "src_spell_name": info.get("spell_zh", ""),
        })

    # ---------------- 服务端 SQL ----------------
    lo_e, hi_e = ENCHANT_ID_START, ENCHANT_ID_START + ID_BLOCK - 1
    lo_s, hi_s = SPELL_ID_START, SPELL_ID_START + ID_BLOCK - 1
    lo_i, hi_i = ITEM_ID_START, ITEM_ID_START + ID_BLOCK - 1

    with open(SQL_OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("-- ============================================================\n")
        f.write("-- 饰品特效转附魔卷轴（由 outputs/饰品武器特效转附魔/generate.py 生成，请勿手改）\n")
        f.write("-- 范围：ItemLevel >= %d 的饰品（InventoryType=%d），ProcTypeMask>0 或 spelltrigger=2\n"
                % (MIN_ITEM_LEVEL, INVENTORY_TYPE_TRINKET))
        f.write("-- 共 %d 条特效；附魔 ID %d~%d；法术 ID %d~%d；卷轴 entry %d~%d\n"
                % (len(usable), lo_e, lo_e + len(usable) - 1,
                   lo_s, lo_s + len(usable) - 1, lo_i, lo_i + len(usable) - 1))
        f.write("-- 本脚本可重复执行（先 DELETE 再 INSERT）\n")
        f.write("-- ============================================================\n\n")

        if WRITE_DBC_TABLES:
            f.write("-- 1) 附魔定义 spellitemenchantment_dbc\n")
            f.write("DELETE FROM `spellitemenchantment_dbc` WHERE `ID` BETWEEN %d AND %d;\n" % (lo_e, hi_e))
            for row in enchant_rows:
                cols, vals = [], []
                for c in ENCHANT_DBC_FIELDS:
                    v = row.get(c)
                    cols.append("`%s`" % c)
                    if is_locale_col(c) and c not in ("Name_Lang_deDE", "Name_Lang_zhCN"):
                        vals.append("NULL")
                    else:
                        vals.append(sql_value(v))
                f.write("INSERT INTO `spellitemenchantment_dbc` (%s) VALUES (%s);\n"
                        % (",".join(cols), ",".join(vals)))

            f.write("\n-- 2) 附魔法术 spell_dbc（Effect_1=53 ENCHANT_ITEM）\n")
            f.write("DELETE FROM `spell_dbc` WHERE `ID` BETWEEN %d AND %d;\n" % (lo_s, hi_s))
            for client_row in spell_client_rows:
                db_row = client_row_to_db_row(client_row)
                cols, vals = [], []
                for c in SPELL_DBC_FIELDS:
                    v = db_row.get(c)
                    cols.append("`%s`" % c)
                    if is_locale_col(c):
                        # 中文同时写 deDE 与 zhCN，其余语言置 NULL
                        vals.append(sql_value(v) if c.endswith(("_Lang_deDE", "_Lang_zhCN")) and v else "NULL")
                    elif c.startswith("unk_320_"):
                        vals.append("0")
                    else:
                        vals.append(sql_value(v if v != "" else 0))
                f.write("INSERT INTO `spell_dbc` (%s) VALUES (%s);\n" % (",".join(cols), ",".join(vals)))
        else:
            f.write("-- 1) 附魔定义 / 附魔法术：DBC 数据通过客户端 DBC 导入，本脚本不写库（WRITE_DBC_TABLES = False）\n")
            f.write("--    下面两条 DELETE 只用于清理早期版本可能写入的库内覆盖行：\n")
            f.write("--    spellitemenchantment_dbc / spell_dbc 的库内行会覆盖客户端 DBC，残留会导致改了 DBC 也不生效。\n")
            f.write("DELETE FROM `spellitemenchantment_dbc` WHERE `ID` BETWEEN %d AND %d;\n" % (lo_e, hi_e))
            f.write("DELETE FROM `spell_dbc` WHERE `ID` BETWEEN %d AND %d;\n" % (lo_s, hi_s))

        f.write("\n-- 2) 附魔卷轴 item_template（class=0 消耗品 / subclass=8 附魔）\n")
        f.write("DELETE FROM `item_template` WHERE `entry` BETWEEN %d AND %d;\n" % (lo_i, hi_i))
        for row in item_rows:
            cols = ",".join("`%s`" % c for c in ITEM_FIELDS)
            vals = ",".join(sql_value(row[c]) for c in ITEM_FIELDS)
            f.write("INSERT INTO `item_template` (%s) VALUES (%s);\n" % (cols, vals))

        f.write("\n-- 3) 命中触发附魔的 proc 数据 spell_enchant_proc_data\n")
        f.write("DELETE FROM `spell_enchant_proc_data` WHERE `entry` BETWEEN %d AND %d;\n" % (lo_e, hi_e))
        for row in proc_rows:
            cols = ",".join("`%s`" % c for c in PROC_FIELDS)
            vals = ",".join(sql_value(row[c]) for c in PROC_FIELDS)
            f.write("INSERT INTO `spell_enchant_proc_data` (%s) VALUES (%s);\n" % (cols, vals))

        f.write("\n-- 4) 运行时映射表 %s（道具「饰品特效提取器」60105 查表用，见 src/TrinketEssence.cpp）\n"
                % MAPPING_TABLE)
        f.write(MAPPING_DDL)
        f.write("DELETE FROM `%s`;\n" % MAPPING_TABLE)
        for row in mapping_rows:
            cols = ",".join("`%s`" % c for c in MAPPING_FIELDS)
            vals = ",".join(sql_value(row[c]) for c in MAPPING_FIELDS)
            f.write("INSERT INTO `%s` (%s) VALUES (%s);\n" % (MAPPING_TABLE, cols, vals))

    print("[4/4] 服务端 SQL -> %s" % SQL_OUT)

    # ---------------- 客户端 DBC CSV ----------------
    f, _p = safe_open("TrinketEnchant_SpellItemEnchantment.dbc.csv", DBC_OUT_DIR)
    with f:
        w = csv.writer(f, quoting=csv.QUOTE_ALL)
        w.writerow(ENCHANT_DBC_FIELDS)
        for row in enchant_rows:
            # 客户端中文放 deDE，zhCN 留空（与 outputs/Spell.csv 一致）
            w.writerow([row.get(c, "") if c != "Name_Lang_zhCN" else "" for c in ENCHANT_DBC_FIELDS])

    f, _p = safe_open("TrinketEnchant_Spell.dbc.csv", DBC_OUT_DIR)
    with f:
        w = csv.writer(f, quoting=csv.QUOTE_ALL)
        w.writerow(spell_header)
        for client_row in spell_client_rows:
            w.writerow([client_row.get(h, "") for h in spell_header])

    f, _p = safe_open("TrinketEnchant_Item.dbc.csv", DBC_OUT_DIR)
    with f:
        w = csv.writer(f, quoting=csv.QUOTE_ALL)
        w.writerow(ITEM_DBC_FIELDS)
        for row in item_rows:
            w.writerow([row["entry"], row["class"], row["subclass"], row["SoundOverrideSubclass"],
                        row["Material"], row["displayid"], row["InventoryType"], 0])

    print("      客户端 DBC CSV -> %s" % DBC_OUT_DIR)

    # ---------------- 提取清单 ----------------
    f, LIST_OUT = safe_open("可用饰品特效清单.csv", SCRIPT_DIR)
    with f:
        w = csv.writer(f, quoting=csv.QUOTE_ALL)
        w.writerow(["附魔ID", "附魔法术ID", "卷轴entry", "原特效法术", "触发类型(spelltrigger)",
                    "附魔Effect", "ProcTypeMask", "ProcChance", "PPM", "显示名",
                    "原特效法术名", "来源饰品"])
        for info in usable:
            w.writerow([info["enchant_id"], info["spell_id"], info["item_id"], info["spell"],
                        info["trigger"], 3 if info["trigger"] == TRIGGER_ON_EQUIP else 1,
                        info.get("proc_mask", 0), info.get("proc_chance", 0), info["ppm"],
                        display_name(info["item_name"]), info.get("spell_zh", ""),
                        "%d(%s)" % (info["items"][0][0], info["item_name"])])
    print("      提取清单 -> %s" % LIST_OUT)

    if excluded:
        print("      已排除 %d 条：" % len(excluded))
        for info in excluded:
            print("        - 法术 %-6d %s" % (info["spell"], info["reason"]))


if __name__ == "__main__":
    main()
