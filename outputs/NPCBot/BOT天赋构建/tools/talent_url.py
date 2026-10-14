#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
talent_url.py —— WotLK 3.3.5 天赋模拟器 URL 生成器

逆向自 80.wowdb.cc / wotlk.evowow.com / wowhead 系共用的 TalentCalc.js。

编码规则
--------
1. 职业前缀 = ENCODING[oldId * 3]
   oldId: 德鲁伊=0 猎人=1 法师=2 圣骑士=3 牧师=4 盗贼=5 萨满=6 术士=7 战士=8 死骑=9
2. 每棵树：把各天赋 rank（含 0）按槽位顺序拼成数字串（每天赋占 1 位）
3. 数字串右侧去掉尾部 '0'
4. 若长度为奇数，右侧补 '0'（记录补齐后的位数 i）
5. 每 2 位一组 (d0,d1) → ENCODING[d0*6 + d1]
6. 若 i < 该树槽位总数，追加树分隔符 'Z'
7. 三棵树依次拼接；空树也需输出 'Z'（因 0 < slot_count）
8. 整个 build 末尾去掉尾部 'Z'
9. 雕文段独立编码，与天赋段之间用 ':' 分隔（见 talent.js 的 tc_readPound：
   `location.hash.substr(1).split(':')` → pounds[0]=build, pounds[1]=glyphs）

URL 形态
--------
  无雕文: https://80.wowdb.cc/?talent#<build>
  含雕文: https://80.wowdb.cc/?talent#<build>:<glyphs>

用法
----
  python talent_url.py --class 盗贼 --spec 刺杀            # 用内置默认构筑
  python talent_url.py --list                              # 列出内置构筑
  python talent_url.py --class 4 --talent "2:5 3:3 5:3"    # 手动指定槽位:rank
  python talent_url.py --class 4 --talent A:2:5 --talent C:2:5   # 指定树
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DATA_FILE = os.path.join(HERE, 'talent_data.json')
GLYPH_FILE = os.path.join(HERE, 'glyph_data.json')

# ============ 编码表（逆向自 TalentCalc.js）============
ENCODING = '0zMcmVokRsaqbdrfwihuGINALpTjnyxtgevElBCDFHJKOPQSUWXYZ123456789'
NEW_TREE = 'Z'

# classId -> oldId（用于取职业前缀）
CLASS_OLD_ID = {
    11: 0,  # Druid
    3: 1,   # Hunter
    8: 2,   # Mage
    2: 3,   # Paladin
    5: 4,   # Priest
    4: 5,   # Rogue
    7: 6,   # Shaman
    9: 7,   # Warlock
    1: 8,   # Warrior
    6: 9,   # Death Knight
}

# URL 模板
URL_TEMPLATE = 'https://80.wowdb.cc/?talent#{code}'
# 天赋段与雕文段的分隔符（talent.js: location.hash.substr(1).split(':')）
GLYPH_SEP = ':'

# 树索引别名
TREE_ALIAS = {
    'A': 0, 'T0': 0, '0': 0,
    'B': 1, 'T1': 1, '1': 1,
    'C': 2, 'T2': 2, '2': 2,
}


def load_data(path=DATA_FILE):
    with open(path, encoding='utf-8') as f:
        return json.load(f)


def resolve_class(data, key):
    """支持 classId 数字或职业名（中文/英文）"""
    key = str(key).strip()
    if key.isdigit() and key in data:
        return int(key)
    for cid, v in data.items():
        if key == v['name'] or key.lower() == v.get('name_en', '').lower():
            return int(cid)
    raise ValueError(f'未找到职业: {key}（可用: {", ".join(v["name"] for v in data.values())}）')


def resolve_spec(data, class_id, spec_name):
    """按树名匹配专精，返回树索引"""
    trees = data[str(class_id)]['trees']
    spec_name = spec_name.strip()
    for i, t in enumerate(trees):
        if spec_name == t['name']:
            return i
    # 支持 '刺杀'/'Assassination' 等模糊匹配
    for i, t in enumerate(trees):
        if spec_name in t['name'] or t['name'] in spec_name:
            return i
    raise ValueError(f'未找到专精: {spec_name}（{data[str(class_id)]["name"]} 可用: '
                     f'{", ".join(t["name"] for t in trees)}）')


# ==================== 编码核心 ====================

def encode_tree(ranks, slot_count):
    """
    ranks: 按槽位顺序的 rank 列表（长度 = slot_count）
    返回该树的编码片段（可能带尾部分隔符 'Z'）
    """
    digits = ''.join(str(r) for r in ranks)
    digits = digits.rstrip('0')
    if not digits:
        # 空树：JS 中 i=0，仍满足 0 < slot_count，故同样追加分隔符
        return NEW_TREE if slot_count > 0 else ''
    i = len(digits)
    if i % 2 == 1:
        i += 1
        digits += '0'
    out = ''
    for k in range(0, len(digits), 2):
        out += ENCODING[int(digits[k]) * 6 + int(digits[k + 1])]
    # 注意：JS 比较的是「补齐后的位数 i」与槽位数，不是编码字符数
    if i < slot_count:
        out += NEW_TREE
    return out


def encode_build(class_id, per_tree_ranks):
    """
    class_id: classId（1-11，不含 10）
    per_tree_ranks: [[tree0 的 rank 列表], [tree1], [tree2]]
    """
    if class_id not in CLASS_OLD_ID:
        raise ValueError(f'classId {class_id} 无天赋树（classId 10 是 GM 专用）')
    prefix = ENCODING[CLASS_OLD_ID[class_id] * 3]
    body = ''
    for ranks in per_tree_ranks:
        body += encode_tree(ranks, len(ranks))
    body = body.rstrip(NEW_TREE)
    return prefix + body


def make_url(class_id, per_tree_ranks, glyphs=None):
    """
    glyphs: 可选。按槽位顺序的雕文物品 ID 列表（6 项，None/0 表示空槽）
            槽位顺序: 大雕文 [15, 30, 80]、小雕文 [15, 50, 70]（等级门槛）
            实际槽位序号 0/3/5 = 大雕文, 1/2/4 = 小雕文
    返回形如 https://80.wowdb.cc/?talent#<build>[:<glyphs>]
    """
    code = encode_build(class_id, per_tree_ranks)
    if glyphs:
        gcode = encode_glyphs(glyphs)
        if gcode:
            code += GLYPH_SEP + gcode
    return URL_TEMPLATE.format(code=code)


# ==================== 雕文编码 ====================
# 槽位类型: 大雕文=1, 小雕文=2
GLYPH_SLOT_TYPE = {0: 1, 3: 1, 5: 1, 1: 2, 2: 2, 4: 2}
# 每组雕文需达到的等级（仅用于文档说明）
GLYPH_LEVELS = {1: [15, 30, 80], 2: [15, 50, 70]}


def load_glyphs(path=GLYPH_FILE):
    with open(path, encoding='utf-8') as f:
        return json.load(f)


def encode_glyphs(glyph_item_ids, glyph_db=None):
    """
    glyph_item_ids: 6 项列表（按槽位 0..5），值为雕文物品 ID 或 None/0
    返回雕文编码片段（无雕文则返回空串）
    """
    if glyph_db is None:
        glyph_db = load_glyphs()
    groups = {1: '', 2: ''}
    for slot, item_id in enumerate(glyph_item_ids):
        if not item_id or int(item_id) == 0:
            continue
        entry = glyph_db.get(str(item_id))
        if entry is None:
            raise ValueError(f'未知雕文物品 ID: {item_id}')
        gtype = entry['t']
        expect = GLYPH_SLOT_TYPE.get(slot)
        if expect != gtype:
            kind = {1: '大', 2: '小'}
            raise ValueError(f'雕文 {entry["n"]}（{kind[gtype]}雕文）不适用于槽位 {slot}'
                             f'（应为{kind[expect]}雕文）')
        groups[gtype] += ENCODING[entry['i']]
    res, encode_len = '', 0
    for gtype in (1, 2):
        encode_len += len(GLYPH_LEVELS[gtype])
        if groups[gtype]:
            res += groups[gtype]
        if len(res) < encode_len:
            res += NEW_TREE
    return res.rstrip(NEW_TREE)



# ==================== 便捷层：槽位/天赋名 -> rank 向量 ====================

def empty_vectors(data, class_id):
    trees = data[str(class_id)]['trees']
    return [[0] * len(t['talents']) for t in trees]


def apply_slots(data, class_id, vectors, tree_idx, slot_ranks):
    """slot_ranks: {slot: rank}"""
    trees = data[str(class_id)]['trees']
    n = len(trees[tree_idx]['talents'])
    for slot, rank in slot_ranks.items():
        slot = int(slot)
        if not (0 <= slot < n):
            raise ValueError(f'树 {tree_idx} 槽位 {slot} 越界（0-{n - 1}）')
        maxr = trees[tree_idx]['talents'][slot]['m']
        if not (0 <= rank <= maxr):
            raise ValueError(f'槽位 {slot}（{trees[tree_idx]["talents"][slot]["n"]}）'
                             f' rank {rank} 超出 maxRank {maxr}')
        vectors[tree_idx][slot] = rank


def apply_names(data, class_id, vectors, name_ranks):
    """按天赋名设置，自动定位树与槽位。name_ranks: {'天赋名': rank}"""
    trees = data[str(class_id)]['trees']
    for name, rank in name_ranks.items():
        found = False
        for ti, tree in enumerate(trees):
            for slot, tal in enumerate(tree['talents']):
                if tal['n'] == name:
                    if not (0 <= rank <= tal['m']):
                        raise ValueError(f'{name} rank {rank} 超出 maxRank {tal["m"]}')
                    vectors[ti][slot] = rank
                    found = True
                    break
            if found:
                break
        if not found:
            raise ValueError(f'未找到天赋: {name}')


def summarize(data, class_id, vectors):
    trees = data[str(class_id)]['trees']
    parts, total = [], 0
    for i, tree in enumerate(trees):
        s = sum(vectors[i])
        total += s
        if s:
            parts.append(f'{tree["name"]} {s}')
    return ' | '.join(parts) + f'  →  共 {total} 点'


# ==================== 内置默认构筑 ====================
# 槽位索引来自 talent_data.json（顺序 = Talent.dbc 的 Row/Col 升序）
# glyphs: 6 项，按槽位 0..5（0/3/5 大雕文，1/2/4 小雕文），值为雕文物品 ID
BUILTIN = {
    ('潜行者', '刺杀'): {
        'name': '刺杀 51/13/7（Icy Veins 毁伤流）',
        'trees': {
            0: {2: 5, 3: 3, 5: 3, 6: 1, 8: 4, 9: 3, 10: 5, 11: 2, 12: 1,
                15: 5, 16: 2, 18: 1, 20: 3, 21: 3, 22: 3, 23: 1, 25: 5, 26: 1},
            1: {2: 5, 5: 5, 8: 3},
            2: {0: 5, 2: 2},
        },
        'glyphs': [45768, 43380, 43376, 45761, 43378, 45767],
        # 槽位 0=毁伤(大) 1=消失(小) 2=扰乱(小) 3=血之饥渴(大) 4=安全降落(小) 5=嫁祸诀窍(大)
    },
    ('潜行者', '战斗'): {
        'name': '战斗 20/51/0（Icy Veins 剑/匕首）',
        'trees': {
            0: {2: 5, 3: 3, 4: 2, 8: 5, 9: 1, 10: 4},
            1: {1: 2, 2: 5, 3: 2, 5: 5, 6: 1, 8: 5, 11: 3, 12: 5, 14: 1,
                16: 2, 17: 2, 18: 3, 19: 1, 22: 5, 24: 1, 25: 2, 26: 5, 27: 1},
            2: {},
        },
        'glyphs': [45767, 43380, 43376, 45762, 43378, 40902],
        # 槽位 0=嫁祸诀窍(大) 1=消失(小) 2=扰乱(小) 3=杀戮盛筵(大) 4=安全降落(小) 5=割裂(大)
    },
    ('潜行者', '敏锐'): {
        'name': '敏锐 19/0/52（Icy Veins 背刺/出血）',
        'trees': {
            0: {2: 5, 3: 3, 4: 2, 8: 5, 9: 1, 10: 3},
            1: {},
            2: {0: 5, 2: 2, 5: 3, 6: 2, 7: 1, 8: 3, 10: 3, 11: 2, 13: 1,
                14: 2, 15: 1, 16: 3, 17: 5, 19: 1, 20: 1, 21: 5, 23: 3,
                24: 1, 25: 2, 26: 5, 27: 1},
        },
        'glyphs': [45767, 43380, 43376, 45764, 43378, 42972],
        # 槽位 0=嫁祸诀窍(大) 1=消失(小) 2=扰乱(小) 3=暗影之舞(大) 4=安全降落(小) 5=影袭(大)
    },
}


def build_from_builtin(data, class_name, spec_name):
    key = (class_name, spec_name)
    if key not in BUILTIN:
        # 尝试模糊匹配
        for (cn, sn), v in BUILTIN.items():
            if cn == class_name and (spec_name in sn or spec_name in v['name']):
                key = (cn, sn)
                break
        else:
            raise ValueError(f'无内置构筑: {class_name}/{spec_name}')
    cid = resolve_class(data, class_name)
    vectors = empty_vectors(data, cid)
    for ti, slots in BUILTIN[key]['trees'].items():
        apply_slots(data, cid, vectors, ti, slots)
    return cid, vectors, BUILTIN[key]['name'], BUILTIN[key].get('glyphs')


# ==================== CLI ====================

def main():
    ap = argparse.ArgumentParser(
        description='WotLK 3.3.5 天赋模拟器 URL 生成器',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__)
    ap.add_argument('--class', dest='cls', help='职业名或 classId')
    ap.add_argument('--spec', help='专精（树名），使用内置构筑')
    ap.add_argument('--talent', action='append', default=[],
                    help='手动指定：树:槽位:rank（树可写 A/B/C 或 0/1/2）')
    ap.add_argument('--glyph', action='append', default=[],
                    help='雕文物品 ID（按 --glyph 出现顺序填入槽位 0..5）；'
                         '也可写 槽位:物品ID 指定槽位')
    ap.add_argument('--no-glyph', action='store_true', help='不输出雕文段')
    ap.add_argument('--list', action='store_true', help='列出内置构筑')
    ap.add_argument('--glyph-info', action='store_true', help='列出可用雕文物品 ID')
    ap.add_argument('--json', action='store_true', help='输出 JSON')
    args = ap.parse_args()

    data = load_data()

    if args.glyph_info:
        gdb = load_glyphs()
        for iid, v in sorted(gdb.items(), key=lambda x: (x[1]['t'], x[1]['i'])):
            kind = '大' if v['t'] == 1 else '小'
            print(f'{iid:>7}  {kind}  idx={v["i"]:>2}  {v["n"]}')
        return 0

    if args.list:
        for (cn, sn), v in BUILTIN.items():
            cid = resolve_class(data, cn)
            vv = empty_vectors(data, cid)
            for ti, slots in v['trees'].items():
                apply_slots(data, cid, vv, ti, slots)
            print(f'{cn} / {sn:<4}  {v["name"]}')
            print(f'    {make_url(cid, vv, v.get("glyphs"))}')
        return 0

    if not args.cls:
        ap.error('需要 --class')

    cid = resolve_class(data, args.cls)
    vectors = empty_vectors(data, cid)
    label = ''
    glyphs = None

    if args.spec:
        cid, vectors, label, glyphs = build_from_builtin(data, args.cls, args.spec)
    for spec in args.talent:
        parts = spec.split(':')
        if len(parts) != 3:
            ap.error(f'--talent 格式应为 树:槽位:rank，收到 {spec}')
        tkey, slot, rank = parts
        ti = TREE_ALIAS.get(tkey.upper())
        if ti is None:
            ap.error(f'树标识 {tkey} 无效（用 A/B/C 或 0/1/2）')
        apply_slots(data, cid, vectors, ti, {slot: int(rank)})

    # 雕文处理
    if args.glyph:
        if glyphs is None:
            glyphs = [0] * 6
        else:
            glyphs = list(glyphs)
        for idx, g in enumerate(args.glyph):
            if ':' in g:
                slot, iid = g.split(':')
                slot = int(slot)
            else:
                slot, iid = idx, int(g)
            if not (0 <= slot < 6):
                ap.error(f'雕文槽位 {slot} 越界（0-5）')
            glyphs[slot] = int(iid)
    if args.no_glyph:
        glyphs = None

    code = encode_build(cid, vectors)
    glyph_code = ''
    if glyphs:
        glyph_code = encode_glyphs(glyphs)
    url = URL_TEMPLATE.format(code=code + (GLYPH_SEP + glyph_code if glyph_code else ''))
    summary = summarize(data, cid, vectors)

    glyph_names = []
    if glyphs:
        gdb = load_glyphs()
        for slot, iid in enumerate(glyphs):
            if iid:
                e = gdb.get(str(iid))
                kind = '大' if e['t'] == 1 else '小'
                glyph_names.append(f'{slot}:{e["n"]}({kind})')

    if args.json:
        print(json.dumps({'class': data[str(cid)]['name'],
                          'code': code + (GLYPH_SEP + glyph_code if glyph_code else ''),
                          'talent_code': code,
                          'glyph_code': glyph_code,
                          'url': url, 'summary': summary,
                          'label': label, 'glyphs': glyph_names},
                         ensure_ascii=False, indent=2))
    else:
        if label:
            print(f'构筑: {label}')
        print(f'点数: {summary}')
        if glyph_names:
            print(f'雕文: {" ".join(glyph_names)}')
        print(f'编码: {code}{GLYPH_SEP + glyph_code if glyph_code else ""}')
        print(f'链接: {url}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
