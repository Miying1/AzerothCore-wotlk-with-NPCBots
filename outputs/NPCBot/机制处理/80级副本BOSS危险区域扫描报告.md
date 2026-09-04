# 80级副本 Boss 危险区域扫描报告

## 1. 扫描范围与判定标准

- **说明依据**：`危险区域配置说明.md`。
- **源码范围**：项目 `src/server/scripts` 中的 80 级巫妖王之怒内容，包括 5 人本、团队副本及中国服常见的 80 级旧世界团本 Onyxia。
- **DBC 来源**：`G:/wow/dbc_csv/Spell.csv`。
- **扫描重点**：Boss 脚本中的 `SummonCreature` / `SummonCreatureGroup`，以及召唤物的 `CastSpell`、`AddAura`、周期伤害或地面区域语义。
- **严格纳入条件**：优先纳入“固定生物作为危险源，并由该生物自己施放伤害法术或持续伤害光环”的机制；只由 Boss 直接施放的 DynamicObject/AreaTrigger，不作为本表的主要候选。

> 说明：`Spell.csv` 是当前导出的 DBC 表，法术名称列为 `Name_Lang_*`，但本文件中相关行的名称字段为空；因此下表的技能名称主要依据源码常量名、源码注释和 WoW 3.3.5 常见技能名称整理，DBC 数值字段仍以文件实际行核对。`EffectRadiusIndex` 是半径索引，不是最终码制距离，不能直接当作 yards 使用；运行时应按说明通过 `SpellInfo::CalcRadius()` 计算。

## 2. 结论摘要

### 2.1 本轮严格筛选口径

本轮仅保留同时满足以下条件的机制：

1. 由 Boss 释放的技能，或由 Boss 技能脚本明确触发召唤；
2. 召唤结果是 Creature；
3. 伤害技能由该 Creature 自身施放或承载；
4. 危险范围可以近似为“以该 Creature 当前坐标为中心的圆形区域”。

因此，移动墙体、线性射线、跟随目标的 Beam、空中落点技能、Boss 直接施放的地面效果、仅用于控制/视觉的召唤物，均不纳入最终候选。

### 2.2 严格符合条件的机制

以下机制与说明中的“生物当前位置为中心的圆形危险区域”最吻合：仅列入满足上述四项条件的项目；对证据不完整的项目单独标为待确认，不作为最终配置。

| 优先级 | 副本 / 地图 | Boss | 危险生物 Entry | 伤害法术 ID | 机制 / 依据 |
|---|---|---|---:|---:|---|
| P0 | 奥杜尔 / `MAP_ULDUAR` 603 | 米米尔隆硬模式 | `NPC_FLAMES_SPREAD` = 34121 | `SPELL_FLAMES_AURA` = 64561 | `boss_mimiron.cpp:2213-2217, 2228-2234`：Boss 硬模式技能链生成火焰生物，并由火焰生物自身承载持续 Aura；检测每个火焰生物当前位置可近似为圆形危险区。 |
| P0 | 十字军试炼 / `MAP_TRIAL_OF_THE_CRUSADER` 649 | 诺森德猛兽（冰吼相关） | `NPC_FIRE_BOMB` = 34854 | `SPELL_FIRE_BOMB` = 66313；`SPELL_FIRE_BOMB_AURA` = 66318 | `boss_northrend_beasts.cpp:190-193`：明确把视觉/初始伤害和周期伤害光环施放到召唤的 Fire Bomb 生物上；高置信度生物型危险区。 |
| P0 | 十字军试炼 / `MAP_TRIAL_OF_THE_CRUSADER` 649 | 诺森德猛兽（酸喉/冰吼相关） | `NPC_SLIME_POOL` = 35176 | `SPELL_SLIME_POOL_EFFECT` = 66882 | `boss_northrend_beasts.cpp:643-645`：召唤生物并由其自身施放池效果；高置信度。 |
| P0 | 冰冠堡垒 / `MAP_ICECROWN_CITADEL` 631 | 辛达苟萨 | `NPC_ICY_BLAST` = 38223 | `SPELL_ICY_BLAST_AREA` = 71380 | `boss_sindragosa.cpp:1549-1550`：Boss 技能链生成 Icy Blast 生物，并由其自身施放区域法术；DBC 具有区域半径索引。 |
| P0 | 冰冠堡垒 / `MAP_ICECROWN_CITADEL` 631 | 腐面 | `NPC_LITTLE_OOZE` / `NPC_BIG_OOZE`（Entry 需查头部枚举/World DB） | `SPELL_WEAK_RADIATING_OOZE` = 69750；`SPELL_RADIATING_OOZE` = 69760 | `boss_rotface.cpp:382-384`、`464-468`：大小软泥生物由自身施放持续辐射伤害；高置信度。建议分别配置两个 Entry。 |

### 2.3 严格剔除项

以下机制本轮不再作为候选：

| 机制 | 剔除原因 |
|---|---|
| 魔环奥术光束 | 生物 `28239` 会跟随目标移动，属于移动射线，不是以自身当前位置为中心的圆形区域。 |
| 黑曜石圣殿 Flame Tsunami | 移动墙体/线性扫过区域，不是圆形危险源。 |
| 奥杜尔尤格-萨隆 Void Zone | 当前源码未确认是由 Boss 技能直接召唤的独立危险 Creature；暂不纳入。 |
| 奥杜尔米米尔隆 Rocket Strike | 火箭、警告触发物、落点伤害分阶段处理，不满足单一 Creature 圆形中心模型。 |
| 奥杜尔烈焰巨兽援助技能 | 召唤物在空中向目标/落点施法，不是生物自身坐标圆形区域。 |
| 纳克萨玛斯 Blizzard | 当前 Boss 文件未证明 Blizzard 生物自身承载伤害技能。 |
| 红玉圣殿陨石、燃烧、灵魂消耗 | 包含标记、扩散、相位和移动关系，不能用单一圆形中心模型表达。 |
| 冰冠堡垒 Ice Tomb | 主要是目标禁锢/冰墓对象，不是独立圆形持续伤害 Creature。 |

### 2.4 明确属于危险机制，但不适合直接用本方案表达

| 副本 / Boss | 法术或生物 | 原因 |
|---|---|---|
| 魔环 / 奥术光束 | `SPELL_ARCANE_BEAM_PERIODIC_DAMAGE` | 需要确认生物是否沿目标/射线移动；圆形中心模型可能误报或漏报。 |
| 奥杜尔 / 米米尔隆火箭打击 | `NPC_ROCKET_STRIKE_N`；`SPELL_ROCKET_STRIKE` = 63681 | 脚本中先生成目标位置的 strike trigger，随后由火箭/触发流程落点；可作为固定点危险源，但需区分 Warning、导弹和最终伤害。 |
| 奥杜尔 / 霍迪尔冰冻 | `NPC_FLASH_FREEZE_NPC/PLR` | 主要是控制/封锁，不是普通圆形地板伤害。 |
| 奥杜尔 / 弗蕾亚自然炸弹 | `NPC_NATURE_BOMB` | 具备落点危险语义，但当前 Boss 片段未确认召唤物实际伤害施法。 |
| 尤格-萨隆死亡射线 | Boss/触须直接施放 | 主要是射线/线性方向，不符合圆形生物当前位置模型。 |
| 红玉圣殿 / 海里昂陨石、燃烧/灵魂消耗 | `NPC_METEOR_STRIKE_*`、`SPELL_METEOR_STRIKE_AOE_DAMAGE` = 74648、`SPELL_COMBUSTION_DAMAGE_AURA` = 74629、`SPELL_CONSUMPTION_DAMAGE_AURA` = 74803 | 召唤物存在，但机制包含圆环扩散、移动/标记和相位；不能仅靠 Entry + 圆形半径完整表达。适合作为后续专用策略。 |
| 黑曜石圣殿 / 火焰旋风 | `NPC_FIRE_CYCLONE` | 移动危险源，可以近似为圆形，但需确认该生物具体伤害光环与周期。 |
| 冰冠堡垒 / 辛达苟萨冰墓 | `NPC_ICE_TOMB` | 主要是目标禁锢和冰墓对象；`SPELL_ICE_TOMB_DAMAGE` = 70157 可能与对象伤害/目标处理相关，不能直接按生物持续危险区配置。 |

## 3. 已核对的关键源码证据

### 3.1 十字军试炼：Fire Bomb 与 Slime Pool

- 文件：`src/server/scripts/Northrend/CrusadersColiseum/TrialOfTheCrusader/boss_northrend_beasts.cpp`
- `190`：召唤 `NPC_FIRE_BOMB = 34854`。
- `192`：向召唤物施放 `SPELL_FIRE_BOMB_AURA = 66318`，源码注释明确为 periodic damage aura。
- `193`：向召唤物施放 `SPELL_FIRE_BOMB = 66313`，源码注释明确为 visual + initial damage。
- `643-645`：召唤 `NPC_SLIME_POOL = 35176`，由池生物自身施放 `SPELL_SLIME_POOL_EFFECT = 66882`。

### 3.2 黑曜石圣殿：Flame Tsunami

- 文件：`src/server/scripts/Northrend/ChamberOfAspects/ObsidianSanctum/boss_sartharion.cpp`
- `126`：`NPC_FLAME_TSUNAMI = 30616`。
- `105-110`：定义 `SPELL_FLAME_TSUNAMI_VISUAL = 57494`、`SPELL_FLAME_TSUNAMI_DAMAGE_AURA = 57492`。
- `645-658`：生成左右两侧的火焰海啸，并施放视觉。
- `671-681`：海啸生物进入阶段后施放/移除伤害光环。

### 3.3 奥杜尔：Scorched Ground 与 Flames Spread

- 烈焰巨兽相关硬模式文件：`boss_ignis.cpp:393-416`。
- `406`：生成 `NPC_SCORCHED_GROUND = 33123`。
- `409`：危险生物自身施放 `SPELL_SCORCHED_GROUND = 62548`。
- 米米尔隆硬模式文件：`boss_mimiron.cpp:2206-2235`。
- `2213-2217`、`2228-2234`：生成 `NPC_FLAMES_SPREAD = 34121`，并由其自身施放 `SPELL_FLAMES_AURA = 64561`，且可继续扩散。

### 3.4 冰冠堡垒：辛达苟萨、腐面

- 辛达苟萨：`boss_sindragosa.cpp:1536-1556`。
- `1549-1550`：生成 `NPC_ICY_BLAST`，其自身施放 `SPELL_ICY_BLAST_AREA = 71380`。
- 腐面相关生物实际由 `boss_professor_putricide.cpp:354-385` 统一初始化。
- `NPC_GROWING_OOZE_PUDDLE = 37690`：生成后自身施放 `SPELL_GROW`，但当前片段未证明它自身直接施放伤害 Aura，因此不作为严格候选。
- `NPC_VOLATILE_OOZE = 37697`：生成后自身施放 `SPELL_OOZE_ERUPTION_SEARCH_PERIODIC`，需继续核对该法术是否为以自身为中心的圆形伤害；在未完成核对前不作为最终候选。
- `boss_rotface.cpp:338-505` 的 `npc_little_ooze` / `npc_big_ooze` 脚本证明 `69750/69760` 为软泥自身 Aura，但这两类软泥不是在该文件中由 Boss 直接 `SummonCreature` 生成；按本轮“Boss 技能召唤”限制，暂列待确认，不直接纳入最终严格清单。

### 3.5 纳克萨玛斯：萨菲隆 Blizzard

- 文件：`boss_sapphiron.cpp:266-281`。
- `NPC_BLIZZARD = 16474` 在随机玩家或 Boss 位置生成，并随机移动。
- Boss 文件中定义了 `SPELL_SUMMON_BLIZZARD = 28560`，但 Blizzard 生物的实际伤害法术未在该 Boss 文件中直接定义，必须继续从 World 数据库中的 CreatureTemplate/CreatureAddon/CreatureSpellData 查实。

## 4. Spell.csv 关键字段核对

以下为 `Spell.csv` 中已经找到的重点法术：

| Spell ID | 源码语义 | Effect_1/2/3 | EffectRadiusIndex_1/2/3 | 备注 |
|---:|---|---|---|---|
| 62548 | Scorched Ground | 6 / 6 / 0 | 0 / 0 / 0 | Aura 周期伤害语义，半径索引未直接给出，运行时 `CalcRadius()` 优先。 |
| 64561 | Flames Aura | 6 / 0 / 0 | 0 / 0 / 0 | 周期光环，建议用配置半径回退或运行时读取其他伤害效果。 |
| 66313 | Fire Bomb | 32 / 32 / 0 | 0 / 0 / 0 | 视觉与初始伤害组合。 |
| 66318 | Fire Bomb Aura | 6 / 0 / 0 | 0 / 0 / 0 | 周期伤害 Aura。 |
| 66882 | Slime Pool Effect | 6 / 3 / 0 | 0 / 10 / 0 | 具有区域半径索引，推荐作为 `damage_spell_id`。 |
| 57492 | Flame Tsunami Damage Aura | 6 / 6 / 0 | 0 / 0 / 0 | 周期伤害 Aura。 |
| 64384 | Void Zone Small | 6 / 0 / 0 | 0 / 0 / 0 | Aura 类型；是否为完整危险圈需结合源码与 DB。 |
| 64017 | Void Zone Large | 6 / 0 / 0 | 0 / 0 / 0 | Aura 类型。 |
| 71380 | Icy Blast Area | 27 / 27 / 0 | 37 / 37 / 0 | 具有明确区域半径索引，推荐作为 `damage_spell_id`。 |
| 69750 | Weak Radiating Ooze | 6 / 0 / 0 | 0 / 0 / 0 | 小软泥持续辐射。 |
| 69760 | Radiating Ooze | 6 / 0 / 0 | 0 / 0 / 0 | 大软泥持续辐射。 |
| 62549 / 63475 | Scorch Damage | 2 / 0 / 0 | 17 / 0 / 0 | Ignis 的直接伤害变体。 |
| 64587 | Nature Bomb Damage | 2 / 98 / 0 | 13 / 13 / 0 | 自然炸弹最终伤害候选。 |
| 63681 | Rocket Strike | 77 / 0 / 0 | 28 / 0 / 0 | 火箭打击落点候选。 |
| 70157 | Ice Tomb Damage | 2 / 6 / 6 | 13 / 13 / 13 | 与冰墓对象/目标处理相关，暂不直接配置。 |
| 74648 | Meteor Strike AOE Damage | 2 / 0 / 0 | 32 / 0 / 0 | 红玉圣殿陨石落点/范围伤害候选。 |
| 74629 | Combustion Damage Aura | 6 / 0 / 0 | 0 / 0 / 0 | 标记/燃烧相关 Aura。 |
| 74803 | Consumption Damage Aura | 6 / 0 / 0 | 0 / 0 / 0 | 标记/灵魂消耗相关 Aura。 |

## 5. 需要继续补查的关键数据

要生成最终可执行 SQL，还缺少以下信息：

1. `NPC_ARCANE_BEAM`、`NPC_ICY_BLAST`、`NPC_LITTLE_OOZE`、`NPC_BIG_OOZE`、`NPC_UNSTABLE_EXPLOSION_STALKER`、`NPC_SCORCHED_GROUND` 等所有候选的确切 Creature Entry；其中部分已在源码枚举，部分需要读取各文件头部或 World DB。
2. `boss_sapphiron.cpp` 中 Blizzard 生物 Entry 16474 的 World DB 技能配置。
3. `boss_yoggsaron.cpp:606` 中变量 `entry` 的实际来源及其对应触须 Entry。
4. 所有周期 Aura 的实际半径。`Spell.csv` 的 `EffectRadiusIndex` 不是 yard 值，最终应由核心的 `CalcRadius()` 取得；若 `CalcRadius()` 对 Aura 返回 0，应在 SQL 中填写经游戏实测或 DBC Radius 表换算后的 `radius` 回退值。
5. 每个候选危险源的生物 despawn 方式与伤害结束时机，用于设置 `deactivation_delay_ms`。

## 6. 推荐的下一步

建议下一轮先查 World 数据库并补齐 Entry，再生成 SQL：

```sql
SELECT entry, name, AIName, ScriptName
FROM creature_template
WHERE entry IN (...候选生物Entry...);

SELECT entry, spell1, spell2, spell3, spell4
FROM creature_template
WHERE entry IN (...候选生物Entry...);

SELECT *
FROM creature_addon
WHERE guid IN (...); -- 若技能通过 addon aura 配置
```

第一批推荐落库顺序（按本轮严格条件）：

1. `NPC_FLAMES_SPREAD = 34121` + `SPELL_FLAMES_AURA = 64561`；
2. `NPC_FIRE_BOMB = 34854` + `SPELL_FIRE_BOMB_AURA = 66318`（可同时记录 66313 为初始伤害证据）；
3. `NPC_SLIME_POOL = 35176` + `SPELL_SLIME_POOL_EFFECT = 66882`；
4. `NPC_SCORCHED_GROUND = 33123` + `SPELL_SCORCHED_GROUND = 62548`；
5. `NPC_ICY_BLAST = 38223` + `SPELL_ICY_BLAST_AREA = 71380`。

腐面软泥暂不落库，直到确认其生成链确实由 Boss 技能召唤且伤害 Aura 满足圆形中心模型。

根据本轮新增限制，最终严格候选暂定为 5 项：米米尔隆 Flames Spread、诺森德猛兽 Fire Bomb、诺森德猛兽 Slime Pool、Ignis Scorched Ground、辛达苟萨 Icy Blast。腐面软泥保留为待确认项，不计入最终 5 项。

当前未直接修改源码和数据库；本报告只提供扫描结果与待确认项。
