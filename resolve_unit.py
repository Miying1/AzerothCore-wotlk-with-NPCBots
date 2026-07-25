#!/usr/bin/env python3
"""
Resolve Unit.cpp conflicts.
Strategy: keep test2 NPCBot hooks + upstream refactoring, merge both additions.
"""

FILE = "src/server/game/Entities/Unit/Unit.cpp"

def read_file():
    with open(FILE, 'r', encoding='utf-8', errors='replace') as f:
        return f.read()

def write_file(content):
    with open(FILE, 'w', encoding='utf-8') as f:
        f.write(content)

def resolve_block(head_lines, upstream_lines, block_num):
    head_text = ''.join(head_lines)
    up_text = ''.join(upstream_lines)
    
    # Pattern 1: NPCBot code, IsNPlayer, botpet, CombatStopWithPets -> keep test2
    bot_keywords = ['npcbot', 'isnplayer', 'botpet', 'combatstopwithpets',
                    '//npcbot', '//end npcbot', '机器人', 'GetPetsOwner',
                    'm_CreatedByPlayer', 'm_ControlledByPlayer', 'm_realRace',
                    'm_movedByPlayer', 'm_AutoRepeatFirstCast', 'm_procDeep',
                    'm_LastPetNumber', 'IsNPCBot']
    for kw in bot_keywords:
        if kw.lower() in head_text.lower():
            return head_lines
    
    # Pattern 2: Include headers -> merge both (keep test2 includes + add upstream new ones)
    is_head_include = any(l.strip().startswith('#include') or l.strip().startswith('//') for l in head_lines if l.strip())
    is_up_include = any(l.strip().startswith('#include') for l in upstream_lines if l.strip())
    if is_head_include and is_up_include:
        result = list(head_lines)
        existing = set()
        for l in result:
            m = __import__('re').search(r'#include\s+[<"]([^>"]+)[>"]', l)
            if m: existing.add(m.group(1))
        for l in upstream_lines:
            m = __import__('re').search(r'#include\s+[<"]([^>"]+)[>"]', l)
            if m and m.group(1) not in existing:
                result.append(l)
        return result
    
    # Pattern 3: Both sides have member initialization -> merge unique members
    if any('m_' in l for l in head_lines) and any('m_' in l for l in upstream_lines):
        result = list(head_lines)
        existing = set()
        for l in result:
            m = __import__('re').match(r'\s*,?\s*(m_\w+)', l.strip().rstrip(',('))
            if m: existing.add(m.group(1))
        for l in upstream_lines:
            m = __import__('re').match(r'\s*,?\s*(m_\w+)', l.strip().rstrip(',('))
            if m and m.group(1) not in existing:
                result.append(l)
        return result
    
    # Pattern 4: m_procEx vs m_hitMask conflict -> keep both
    if 'm_procEx' in head_text or 'm_hitMask' in up_text:
        return head_lines
    
    # Pattern 5: IsPlayer() -> keep head (npcbots_3.3.5 base)
    # In this merge direction, HEAD is npcbots_3.3.5 which already has IsPlayer
    # test2 has custom bot handling
    
    # Default: keep HEAD (npcbots_3.3.5 base) since that's the established codebase
    return head_lines

def main():
    content = read_file()
    lines = content.splitlines(keepends=True)
    resolved = []
    i = 0
    block_num = 0
    
    while i < len(lines):
        line = lines[i]
        
        if line.startswith('<<<<<<< '):
            block_num += 1
            head_lines = []
            upstream_lines = []
            
            i += 1
            while i < len(lines) and not lines[i].startswith('======='):
                head_lines.append(lines[i])
                i += 1
            i += 1  # skip =======
            while i < len(lines) and not lines[i].startswith('>>>>>>> '):
                upstream_lines.append(lines[i])
                i += 1
            i += 1  # skip >>>>>>>
            
            result = resolve_block(head_lines, upstream_lines, block_num)
            resolved.extend(result or head_lines)
        else:
            resolved.append(line)
            i += 1
    
    result_text = ''.join(resolved)
    remaining = result_text.count('<<<<<<< ')
    write_file(result_text)
    print(f"Resolved {block_num} blocks in Unit.cpp ({remaining} remaining conflict markers)")

if __name__ == '__main__':
    main()
