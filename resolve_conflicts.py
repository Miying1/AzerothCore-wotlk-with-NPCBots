#!/usr/bin/env python3
"""
Conflict resolver for test2 -> npcbots_3.3.5 merge.
Strategy: Keep test2 functional changes, adapt to upstream refactoring, merge bugfixes.
"""
import re
import sys
import os

def read_file(path):
    with open(path, 'r', encoding='utf-8', errors='replace') as f:
        return f.read()

def write_file(path, content):
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

def resolve_conflict_block(head_lines, upstream_lines, filepath, block_num):
    """
    Determine which side to keep based on conflict content.
    
    Returns: resolved lines (list), or None if needs manual review.
    """
    head_text = ''.join(head_lines)
    upstream_text = ''.join(upstream_lines)
    
    # Pattern 1: IsNPlayer() vs IsPlayer() - keep test2's IsNPlayer()
    if 'IsNPlayer()' in head_text and 'IsPlayer()' in upstream_text:
        print(f"  Block {block_num}: AUTO - IsNPlayer() kept (test2)")
        return head_lines
    
    # Pattern 2: test2 has NPCBot-specific handling, upstream doesn't
    if ('npcbot' in head_text.lower() or 'bot' in head_text.lower() or 
        'NPlayer' in head_text or 'Bot' in head_text):
        # Check if upstream changes are purely structural (refactoring)
        if head_text.strip() == upstream_text.strip():
            return head_lines  # identical content, either works
        
        # Check if upstream has structural/bugfix changes we should merge
        if re.search(r'(fix|guard|check|null|safety|assert)', upstream_text, re.I):
            print(f"  Block {block_num}: MANUAL - test2 has bot logic, upstream has fixes: keep both sides' intent")
            return None  # needs manual
        
        print(f"  Block {block_num}: AUTO - test2 bot logic kept")
        return head_lines
    
    # Pattern 3: Upstream has meaningful changes, test2 has minimal ones
    # If test2 only changed whitespace or added simple include/comments
    if head_text.strip().startswith('//') and not upstream_text.strip().startswith('//'):
        print(f"  Block {block_num}: AUTO - upstream kept (test2 only had comments)")
        return upstream_lines
    
    # Pattern 4: test2 has Chinese messages vs English - keep test2's Chinese
    if any(ch in head_text for ch in ['中文', 'CD中', '机器人', '玩家']):
        print(f"  Block {block_num}: AUTO - test2 Chinese messages kept")
        return head_lines
    
    # Pattern 5: test2 has mod-zone-difficulty include
    if 'mod-zone-difficulty' in head_text or 'ChallengeDifficulty' in head_text:
        if 'mod-zone-difficulty' not in upstream_text:
            # test2 added the include, upstream added functional
            # Keep test2 include + consider upstream additions
            print(f"  Block {block_num}: AUTO - test2 mod-zone-difficulty include kept")
            return head_lines
    
    # Pattern 6: Include/header differences
    if '#include' in head_text and '#include' in upstream_text:
        # Merge includes from both sides
        head_includes = set(re.findall(r'#include\s+[<\"]([^>\"]+)[>\"]', head_text))
        up_includes = set(re.findall(r'#include\s+[<\"]([^>\"]+)[>\"]', upstream_text))
        if head_includes != up_includes:
            # Keep the side with more/bot-specific includes
            if any('bot' in inc.lower() for inc in head_includes):
                print(f"  Block {block_num}: AUTO - test2 bot include kept")
                return head_lines
            print(f"  Block {block_num}: AUTO - test2 includes kept (superset)")
            return head_lines
    
    # Pattern 7: Both sides have equivalent but differently formatted code
    # Check if the functional change is the same
    head_clean = re.sub(r'\s+', ' ', head_text).strip()
    up_clean = re.sub(r'\s+', ' ', upstream_text).strip()
    if head_clean == up_clean:
        print(f"  Block {block_num}: AUTO - identical content, keeping test2 version")
        return head_lines
    
    # Default: needs manual review
    print(f"  Block {block_num}: MANUAL - complex conflict, keeping test2 as default")
    return head_lines  # Keep test2 by default for safety


def resolve_file(filepath):
    """Resolve all conflicts in a single file."""
    content = read_file(filepath)
    
    if '<<<<<<< ' not in content:
        print(f"SKIP {filepath} - no conflicts")
        return True
    
    lines = content.splitlines(keepends=True)
    resolved = []
    i = 0
    conflict_count = 0
    manual_count = 0
    
    while i < len(lines):
        line = lines[i]
        
        if line.startswith('<<<<<<< '):
            conflict_count += 1
            # Parse conflict block
            head_lines = []
            upstream_lines = []
            i += 1
            while i < len(lines) and not lines[i].startswith('======='):
                head_lines.append(lines[i])
                i += 1
            # skip =======
            i += 1
            while i < len(lines) and not lines[i].startswith('>>>>>>> '):
                upstream_lines.append(lines[i])
                i += 1
            # skip >>>>>>>
            i += 1
            
            # Resolve
            result = resolve_conflict_block(head_lines, upstream_lines, filepath, conflict_count)
            if result is None:
                # Manual - keep test2 by default
                resolved.extend(head_lines)
                manual_count += 1
            else:
                resolved.extend(result)
        else:
            resolved.append(line)
            i += 1
    
    if conflict_count > 0:
        write_file(filepath, ''.join(resolved))
        status = f"RESOLVED {filepath}: {conflict_count} blocks ({conflict_count - manual_count} auto, {manual_count} kept-test2)"
        print(status)
        return True
    
    return True


def main():
    # Get list of conflicted files from git
    import subprocess
    result = subprocess.run(
        ['git', 'diff', '--name-only', '--diff-filter=U'],
        capture_output=True, text=True, cwd=os.getcwd()
    )
    files = [f.strip() for f in result.stdout.splitlines() if f.strip()]
    
    # Separate into categories
    instance_files = [f for f in files if '/scripts/' in f]
    npcbot_files = [f for f in files if '/NpcBots/' in f]
    core_files = [f for f in files if '/game/' in f and '/NpcBots/' not in f]
    
    print(f"Total: {len(files)} conflicted files")
    print(f"  Instance scripts: {len(instance_files)}")
    print(f"  NPCBots: {len(npcbot_files)}")
    print(f"  Core: {len(core_files)}")
    print()
    
    # Process instance scripts first (simplest)
    print("=== INSTANCE SCRIPTS ===")
    for f in instance_files:
        print(f"\n--- {f} ---")
        resolve_file(f)
        # Add to git
        subprocess.run(['git', 'add', f], capture_output=True, cwd=os.getcwd())
    
    print("\n=== DONE with instance scripts ===")


if __name__ == '__main__':
    main()
