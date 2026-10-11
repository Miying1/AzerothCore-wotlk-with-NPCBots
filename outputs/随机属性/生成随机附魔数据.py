# -*- coding: utf-8 -*-
"""生成随机附魔（ItemRandomProperties）的服务端 SQL 与客户端 DBC CSV。

数据链路（自上而下）：
  获得装备 → 脚本 item_random_enchant 按主属性推断分组（法系/物理/防御/治疗）
        │ 从自定义表 mod_random_enchant 按 chance 加权随机挑一个组合
        ▼
  mod_random_enchant.ench = ItemRandomProperties.dbc 的 ID（组合）
        │ 每个组合 = 单条属性（一个 SpellItemEnchantment）
        ▼
  ItemRandomProperties.dbc.Enchant[0] = SpellItemEnchantment.dbc 的 ID（基础附魔）
        │ type=5（STAT），EffectArg=属性ID，EffectPointsMin=数值
        ▼
  SpellItemEnchantment.dbc → 最终作用到装备上的属性

属性数值落地：每个属性从最小值到最大值按步长生成多档（每档一条附魔 + 一个组合），
掉落时随机命中一个档位。步长规则：
  - 默认 1（如急速 20, 21, ..., 100）
  - 攻强 3、法强 2、格挡值 5、护甲值 50

分组约定（entry）：
  90001 法系 / 90101 物理 / 90201 防御 / 90301 治疗

名称约定：
  - ItemRandomProperties.dbc：保持分组前缀「法· / 愈· / 攻· / 防·」，客户端显示在
    装备名旁；该名称会拼进物品名与物品链接，因此不能带颜色码；
  - SpellItemEnchantment.dbc：统一为「+数值 属性」格式（如「+10 力量」），
    用于附魔窗口等展示。
  两张 DBC 的名称均同时写入 deDE 与 zhCN 两列（客户端按本地化列读取，两列都写保险）。

产物：
  - 两个 CSV 输出到本脚本目录（outputs/随机属性/）
  - mod_random_enchant 建表 SQL（custom_random_enchant.sql）
    输出到 modules/mod-random-enchants/sql/world/
"""
import csv
import os

# CSV 输出目录：与本脚本同目录
OUT = os.path.dirname(os.path.abspath(__file__))
# SQL 输出目录：mod-random-enchants 模块的 sql/world
SQL_OUT = os.path.abspath(os.path.join(OUT, "..", "..", "modules", "mod-random-enchants", "sql", "world"))

# ============================================================
# ItemModType（SpellItemEnchantment type=5 STAT 时的 EffectArg）
# 与 C++ 端 ItemTemplate.h 的 enum ItemModType 一致
# ============================================================
ITEM_MOD_AGILITY                  = 3
ITEM_MOD_STRENGTH                 = 4
ITEM_MOD_INTELLECT                = 5
ITEM_MOD_STAMINA                  = 7
ITEM_MOD_DEFENSE_SKILL_RATING     = 12
ITEM_MOD_DODGE_RATING             = 13
ITEM_MOD_PARRY_RATING             = 14
ITEM_MOD_HIT_RATING               = 31
ITEM_MOD_CRIT_RATING              = 32
ITEM_MOD_HASTE_RATING             = 36
ITEM_MOD_EXPERTISE_RATING         = 37
ITEM_MOD_ATTACK_POWER             = 38
ITEM_MOD_MANA_REGENERATION        = 43
ITEM_MOD_ARMOR_PENETRATION_RATING = 44
ITEM_MOD_SPELL_POWER              = 45
ITEM_MOD_SPELL_PENETRATION        = 47
ITEM_MOD_BLOCK_VALUE              = 48

# 附魔类型：5 = 属性（STAT）
ENCHANT_TYPE_STAT = 5

# 本地化掩码（与既有混沌宝石脚本保持一致）
NAME_LANG_MASK = 16712190

# 属性词条文本颜色：标准蓝色品质（Rare）装备对应的颜色
BLUE = "|cff0070dd"

# ID 起始（两张 DBC 为独立 ID 空间，均从 10000 起，避开原版 DBC 与混沌宝石 50001～50009）
ENCHANT_BASE = 10000   # SpellItemEnchantment 起始 ID
PROPERTY_BASE = 10000  # ItemRandomProperties 起始 ID

# ============================================================
# 属性清单：(名称, ItemModType, 最小值, 最大值, 步长)
# ============================================================
ATTRIBUTES = [
    ("急速等级",   ITEM_MOD_HASTE_RATING,             20,  100, 1),
    ("暴击等级",   ITEM_MOD_CRIT_RATING,              20,  100, 1),
    ("护甲穿透",   ITEM_MOD_ARMOR_PENETRATION_RATING, 20,  100, 1),
    ("命中等级",   ITEM_MOD_HIT_RATING,               15,   50, 1),
    ("精准等级",   ITEM_MOD_EXPERTISE_RATING,         15,   50, 1),
    ("躲闪",       ITEM_MOD_DODGE_RATING,             30,  100, 1),
    ("招架",       ITEM_MOD_PARRY_RATING,             30,  100, 1),
    ("防御",       ITEM_MOD_DEFENSE_SKILL_RATING,     20,   80, 1),
    ("攻击强度",   ITEM_MOD_ATTACK_POWER,             55,  190, 3),
    ("法术强度",   ITEM_MOD_SPELL_POWER,              35,  120, 2),
    ("法术穿透",   ITEM_MOD_SPELL_PENETRATION,        20,   60, 1),
    ("格挡值",     ITEM_MOD_BLOCK_VALUE,             100,  300, 5),
    ("每5秒法力回复", ITEM_MOD_MANA_REGENERATION,        10,   40, 1),
    ("力量",       ITEM_MOD_STRENGTH,                 20,   60, 1),
    ("敏捷",       ITEM_MOD_AGILITY,                  20,   60, 1),
    ("耐力",       ITEM_MOD_STAMINA,                  50,  100, 1),
    ("智力",       ITEM_MOD_INTELLECT,                30,   80, 1),
]

# 属性名 -> (ItemModType, 最小值, 最大值, 步长)，供按组生成时查询
ATTR_MAP = {name: (stat, vmin, vmax, step) for name, stat, vmin, vmax, step in ATTRIBUTES}

# ============================================================
# 分组：(entry, 名称, [属性名...])
# ============================================================
GROUP_DEFS = [
    (90001, "法系", ["智力", "急速等级", "暴击等级", "命中等级", "法术强度", "法术穿透"]),
    (90301, "治疗", ["智力", "急速等级", "暴击等级", "法术强度", "每5秒法力回复"]),
    (90101, "物理", ["力量", "敏捷", "急速等级", "暴击等级", "护甲穿透", "攻击强度", "命中等级", "精准等级"]),
    (90201, "防御", ["躲闪", "招架", "防御", "格挡值"]),
]

# ============================================================
# 附魔描述名（SpellItemEnchantment.dbc 的 Name_Lang_*）：
# 统一为「+数值 属性」（如 +10 力量），用于附魔窗口等展示。
# ============================================================
def display_name(attr_name, value):
    return "+{} {}".format(value, attr_name)


# ============================================================
# 装备名标记（ItemRandomProperties.dbc 的 Name / Name_Lang_*）：
# 保持分组前缀写法，客户端显示在装备名旁（如「法·迅捷之刃」）。
# 该字段会拼进物品名与物品链接，不能带颜色码。
# ============================================================
GROUP_PREFIX = {
    "法系": "法·",
    "治疗": "愈·",
    "物理": "攻·",
    "防御": "防·",
}


# ============================================================
# 生成：按「组 × 属性 × 档位」各生成一条附魔 + 一个组合
# 同一属性被多组复用时各组持有独立 ID，保证名称前缀能按组区分（法·/愈·/攻·/防·）
# ============================================================
def generate():
    enchants = []      # (附魔ID, 属性名, ItemModType, 数值)
    properties = []    # (组合ID, 属性名, [附魔ID])
    prop_meta = {}     # 组合ID -> (属性名, 数值, 同属性内序号, ItemModType, 条目权重)

    eid = ENCHANT_BASE
    pid = PROPERTY_BASE

    groups = []
    for entry, gname, attr_names in GROUP_DEFS:
        combos = []
        for name in attr_names:
            if name not in ATTR_MAP:
                continue
            stat, vmin, vmax, step = ATTR_MAP[name]
            values = list(range(vmin, vmax + 1, step))
            max_tier = len(values)
            for tier, value in enumerate(values, start=1):
                weight = max_tier - tier + 1  # 序号越大权重越低（线性递减）
                enchants.append((eid, name, stat, value))
                properties.append((pid, name, [eid]))
                prop_meta[pid] = (name, value, tier, stat, weight)
                combos.append(pid)
                eid += 1
                pid += 1
        groups.append((entry, gname, combos))

    return enchants, properties, groups, prop_meta


ENCHANTS, PROPERTIES, GROUPS, PROP_META = generate()

# 组合ID -> 分组前缀（写入 ItemRandomProperties.dbc 的 Name 字段，如「法·」）
PROP_PREFIX = {}
for _entry, _gname, _combos in GROUPS:
    for _cid in _combos:
        PROP_PREFIX[_cid] = GROUP_PREFIX.get(_gname, "")


def csv_cell(v):
    return "" if v is None else v


# ============================================================
# 1) SpellItemEnchantment.dbc.csv（WDBX 格式：全值加引号）
# 字段名与既有「生成自定义混沌宝石.py」的 ENCH_CSV_FIELDS 完全一致
# ============================================================
ENCH_CSV_FIELDS = ["ID", "Charges", "Effect_1", "Effect_2", "Effect_3",
    "EffectPointsMin_1", "EffectPointsMin_2", "EffectPointsMin_3",
    "EffectPointsMax_1", "EffectPointsMax_2", "EffectPointsMax_3",
    "EffectArg_1", "EffectArg_2", "EffectArg_3",
    "Name_Lang_enUS", "Name_Lang_enGB", "Name_Lang_koKR", "Name_Lang_frFR", "Name_Lang_deDE",
    "Name_Lang_enCN", "Name_Lang_zhCN", "Name_Lang_enTW", "Name_Lang_zhTW",
    "Name_Lang_esES", "Name_Lang_esMX", "Name_Lang_ruRU", "Name_Lang_ptPT", "Name_Lang_ptBR",
    "Name_Lang_itIT", "Name_Lang_Unk", "Name_Lang_Mask",
    "ItemVisual", "Flags", "Src_ItemID", "Condition_Id", "RequiredSkillID", "RequiredSkillRank", "MinLevel"]


def write_enchantment_csv():
    path = os.path.join(OUT, "随机属性_SpellItemEnchantment.dbc.csv")
    with open(path, "w", newline="", encoding="utf-8-sig") as f:
        w = csv.writer(f, quoting=csv.QUOTE_ALL, lineterminator="\n")
        w.writerow(ENCH_CSV_FIELDS)
        for eid, name, stat_type, value in ENCHANTS:
            # type=STAT：EffectArg 填属性 ID，EffectPointsMin/Max 填数值
            vals = {
                "ID": eid, "Charges": 0,
                "Effect_1": ENCHANT_TYPE_STAT, "Effect_2": 0, "Effect_3": 0,
                "EffectPointsMin_1": value, "EffectPointsMin_2": 0, "EffectPointsMin_3": 0,
                "EffectPointsMax_1": value, "EffectPointsMax_2": 0, "EffectPointsMax_3": 0,
                "EffectArg_1": stat_type, "EffectArg_2": 0, "EffectArg_3": 0,
                # 名称统一为「+数值 属性」；中文写 deDE，同时补写 zhCN（保险）
                "Name_Lang_deDE": f"{BLUE}{display_name(name, value)}|r",
                "Name_Lang_zhCN": f"{BLUE}{display_name(name, value)}|r",
                "Name_Lang_Mask": NAME_LANG_MASK,
                "ItemVisual": 0, "Flags": 0, "Src_ItemID": 0,
                "Condition_Id": 0, "RequiredSkillID": 0, "RequiredSkillRank": 0, "MinLevel": 0,
            }
            w.writerow([csv_cell(vals.get(c)) for c in ENCH_CSV_FIELDS])
    print("已生成 ->", path, "（{} 条）".format(len(ENCHANTS)))


# ============================================================
# 2) ItemRandomProperties.dbc.csv
# 字段顺序：ID, 内部名, Enchant_1~5, 本地化 Name_Lang_*
# ============================================================
PROP_CSV_FIELDS = ["ID", "Name",
    "Enchant_1", "Enchant_2", "Enchant_3", "Enchant_4", "Enchant_5",
    "Name_Lang_enUS", "Name_Lang_enGB", "Name_Lang_koKR", "Name_Lang_frFR", "Name_Lang_deDE",
    "Name_Lang_enCN", "Name_Lang_zhCN", "Name_Lang_enTW", "Name_Lang_zhTW",
    "Name_Lang_esES", "Name_Lang_esMX", "Name_Lang_ruRU", "Name_Lang_ptPT", "Name_Lang_ptBR",
    "Name_Lang_itIT", "Name_Lang_Unk", "Name_Lang_Mask"]


def write_properties_csv():
    path = os.path.join(OUT, "随机属性_ItemRandomProperties.dbc.csv")
    with open(path, "w", newline="", encoding="utf-8-sig") as f:
        w = csv.writer(f, quoting=csv.QUOTE_ALL, lineterminator="\n")
        w.writerow(PROP_CSV_FIELDS)
        for pid, _name, enchants in PROPERTIES:
            ench = (enchants + [0] * 5)[:5]
            # 分组前缀会显示在装备名旁（如「法·迅捷之刃」），不能带颜色码，
            # 否则 | 会破坏物品链接；客户端按本地化列读取，故 deDE 与 zhCN 都写。
            prefix = PROP_PREFIX.get(pid, "")
            vals = {
                "ID": pid, "Name": prefix,
                "Enchant_1": ench[0], "Enchant_2": ench[1], "Enchant_3": ench[2],
                "Enchant_4": ench[3], "Enchant_5": ench[4],
                "Name_Lang_deDE": prefix,
                "Name_Lang_zhCN": prefix,
                "Name_Lang_Mask": NAME_LANG_MASK,
            }
            w.writerow([csv_cell(vals.get(c)) for c in PROP_CSV_FIELDS])
    print("已生成 ->", path, "（{} 条）".format(len(PROPERTIES)))


# ============================================================
# 3) custom_random_enchant.sql（自定义分组表，幂等）
#    表名 mod_random_enchant，与 item_random_enchant.cpp 的查询保持一致；
#    不走原版 item_enchantment_template，由脚本 item_random_enchant 读取
# ============================================================
def write_custom_table_sql():
    os.makedirs(SQL_OUT, exist_ok=True)
    path = os.path.join(SQL_OUT, "custom_random_enchant.sql")
    entries = [g[0] for g in GROUPS]
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("-- 自定义随机附魔分组表（脚本 item_random_enchant 读取）\n")
        f.write("-- entry    ：分组入口（90001 法系 / 90101 物理 / 90201 防御 / 90301 治疗）\n")
        f.write("-- ench     ：ItemRandomProperties.dbc 的组合ID\n")
        f.write("-- attr     ：属性名\n")
        f.write("-- attr_type：属性类型（ItemModType，对应 SpellItemEnchantment 的 EffectArg）\n")
        f.write("-- tier     ：同属性内数值从小到大序号\n")
        f.write("-- chance   ：条目权重（同属性内序号越大权重越低）\n\n")
        f.write("DROP TABLE IF EXISTS `mod_random_enchant`;\n")
        f.write("CREATE TABLE `mod_random_enchant` (\n")
        f.write("  `entry` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT '分组入口',\n")
        f.write("  `ench` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT 'ItemRandomProperties.dbc 组合ID',\n")
        f.write("  `attr` VARCHAR(32) NOT NULL DEFAULT '' COMMENT '属性名',\n")
        f.write("  `attr_type` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT '属性类型 ItemModType',\n")
        f.write("  `tier` INT UNSIGNED NOT NULL DEFAULT '0' COMMENT '同属性内数值从小到大序号',\n")
        f.write("  `chance` FLOAT NOT NULL DEFAULT '0' COMMENT '条目权重（序号越大越小）',\n")
        f.write("  PRIMARY KEY (`entry`, `ench`)\n")
        f.write(") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='自定义随机附魔分组';\n\n")
        for entry, gname, combos in GROUPS:
            f.write("-- {}组 entry={}（{} 个组合）\n".format(gname, entry, len(combos)))
            f.write("INSERT INTO `mod_random_enchant` (`entry`, `ench`, `attr`, `attr_type`, `tier`, `chance`) VALUES\n")
            rows = ["({}, {}, '{}', {}, {}, {})".format(entry, cid, PROP_META[cid][0], PROP_META[cid][3], PROP_META[cid][2], PROP_META[cid][4])
                    for cid in combos]
            f.write(",\n".join(rows) + ";\n\n")
    print("已生成 ->", path, "（{} 行）".format(sum(len(g[2]) for g in GROUPS)))


if __name__ == "__main__":
    write_enchantment_csv()
    write_properties_csv()
    write_custom_table_sql()
    print("完成。请用 WDBX 将两个 CSV 分别导入 SpellItemEnchantment.dbc 与 ItemRandomProperties.dbc，")
    print("并执行 sql/world/ 下的 custom_random_enchant.sql。")
