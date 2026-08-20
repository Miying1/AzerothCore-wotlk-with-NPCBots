import csv, sys, io

csv_path = r"E:\workbuddy\AzerothCore-wotlk-with-NPCBots\outputs\Spell_dbc.csv"

# 需要提取的卡拉赞法术 ID（按 Boss/召唤物分组）
targets = {
    # Attumen / Midnight
    29832: "Attumen:Shadow Cleave", 29833: "Attumen:Intangible Presence", 29847: "Attumen:Charge",
    29711: "Midnight:Knockdown", 29770: "Midnight:Mount",
    # Moroes
    37066: "Moroes:Garrote(dot)", 34694: "Moroes:Blind", 29425: "Moroes:Gouge",
    # Maiden
    29522: "Maiden:Holy Fire(dot)", 32445: "Maiden:Holy Wrath", 29523: "Maiden:Holy Ground",
    # Curator
    30383: "Curator:Hateful Bolt", 30254: "Curator:Evocation",
    # Aran
    29954: "Aran:Frostbolt", 29953: "Aran:Fireball", 29955: "Aran:Arcane Missiles",
    29961: "Aran:Arcane Explosion(cs)", 29973: "Aran:Arcane Explosion", 29978: "Aran:AoE Pyroblast",
    29946: "Aran:Flame Wreath", 29949: "Aran:Flame Wreath Explosion", 29991: "Aran:Chains of Ice",
    29964: "Aran:Dragon Breath", 30035: "Aran:Mass Slow",
    # Terestian / Kilrek / Imp
    30055: "Terestian:Shadow Bolt", 30115: "Terestian:Sacrifice", 30120: "Terestian:Demon Chains(summon)",
    30206: "DemonChain:Demon Chains", 30050: "Imp:Firebolt", 30065: "Kilrek:Broken Pact", 30053: "Kilrek:Amplify Flames",
    # Netherspite
    30522: "Netherspite:Netherburn", 37063: "Netherspite:Void Zone", 38523: "Netherspite:Netherbreath",
    38684: "Netherspite:Roar", 38549: "Netherspite:Empowerment", 38688: "Netherspite:Nether Infusion",
    # Malchezaar / infernal / axe
    30852: "Malchezaar:Shadow Nova", 30854: "Malchezaar:Shadow Word Pain(dot)", 30901: "Malchezaar:Sunder Armor",
    30843: "Malchezaar:Enfeeble", 30131: "Cleave(Nightbane/Malchezaar)", 30859: "Infernal:Hellfire",
    39095: "Malchezaar:Amplify Damage", 30834: "Infernal Relay",
    # Nightbane
    30210: "Nightbane:Smoldering Breath", 30129: "Nightbane:Charred Earth", 30128: "Nightbane:Smoking Blast",
    37057: "Nightbane:Smoking Blast(target)", 30130: "Nightbane:Distracting Ash", 30282: "Nightbane:Fireball Barrage",
    36922: "Nightbane:Bellowing Roar", 25653: "Nightbane:Tail Sweep",
    # Opera Oz
    31012: "Dorothee:Water Bolt", 31013: "Dorothee:Screech", 31046: "Strawman:Brain Bash",
    31075: "Strawman:Burning Straw", 31043: "Tinhead:Cleave", 31086: "Tinhead:Rust",
    31041: "Roar:Mangle", 31042: "Roar:Shred", 32337: "Crone:Chain Lightning", 32334: "Cyclone:Knockback",
    # Opera Hood
    30752: "BigBadWolf:Terrifying Howl", 30761: "BigBadWolf:Wide Swipe",
    # Opera RAJ
    30890: "Julianne:Blinding Passion", 30887: "Julianne:Devotion", 30889: "Julianne:Powerful Attraction",
    30878: "Julianne:Eternal Affection", 30815: "Romulo:Backward Lunge", 30841: "Romulo:Daring",
    30817: "Romulo:Deadly Swathe", 30822: "Romulo:Poison Thrust",
    # Servant quarters
    29901: "Hyakiss:Acidic Fang", 29896: "Hyakiss:Web", 29903: "Shadikith:Dive", 29904: "Shadikith:Sonic Burst",
    29905: "Shadikith:Wing Buffet", 29906: "Rokad:Ravage",
    # Tenris
    50846: "Tenris:Blood Mirror Damage", 51013: "Tenris:Exsanguinate", 51135: "Tenris:Blood Tap",
    # Generic
    26662: "Berserk", 32965: "Terestian:Berserk", 32437: "Rattled", 29766: "Overload",
}

with open(csv_path, newline='', encoding='utf-8-sig') as f:
    reader = csv.reader(f)
    header = next(reader)
    col = {name: i for i, name in enumerate(header)}

    # 打印关键列名与索引（用于确认字段）
    interesting = [n for n in header if any(k in n.lower() for k in
        ('id','effect1','effect2','effect3','diesides','basepoints','realpoints','applyaura',
         'amplitude','multiplevalue','school','name','description','duration','chain','miscvalue','mechanic'))]
    print("### COLUMNS ###")
    for n in interesting:
        print(f"{n} -> {col[n]}")
    print("### END COLUMNS ###")

    def g(row, name):
        i = col.get(name)
        if i is None:
            return ''
        return row[i] if i < len(row) else ''

    rows = {}
    for row in reader:
        try:
            sid = int(row[0])
        except (ValueError, IndexError):
            continue
        if sid in targets:
            rows[sid] = row

    print("### SPELL DATA ###")
    for sid in sorted(targets):
        r = rows.get(sid)
        if r is None:
            print(f"{sid}|{targets[sid]}|NOT_FOUND")
            continue
        eff1, eff2, eff3 = g(r,'Effect1'), g(r,'Effect2'), g(r,'Effect3')
        die1, die2, die3 = g(r,'EffectDieSides1'), g(r,'EffectDieSides2'), g(r,'EffectDieSides3')
        bp1, bp2, bp3 = g(r,'EffectBasePoints1'), g(r,'EffectBasePoints2'), g(r,'EffectBasePoints3')
        rp1, rp2, rp3 = g(r,'EffectRealPointsPerLevel1'), g(r,'EffectRealPointsPerLevel2'), g(r,'EffectRealPointsPerLevel3')
        aura1, aura2, aura3 = g(r,'EffectApplyAuraName1'), g(r,'EffectApplyAuraName2'), g(r,'EffectApplyAuraName3')
        amp1, amp2, amp3 = g(r,'EffectAmplitude1'), g(r,'EffectAmplitude2'), g(r,'EffectAmplitude3')
        mv1, mv2, mv3 = g(r,'EffectMultipleValue1'), g(r,'EffectMultipleValue2'), g(r,'EffectMultipleValue3')
        print(f"{sid}|{targets[sid]}|E={eff1},{eff2},{eff3}|DIE={die1},{die2},{die3}|BP={bp1},{bp2},{bp3}|RPL={rp1},{rp2},{rp3}|AURA={aura1},{aura2},{aura3}|AMP={amp1},{amp2},{amp3}|MV={mv1},{mv2},{mv3}")
