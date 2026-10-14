# 项目长期约定

## 环境与惯例

- 本地 MySQL：`root` / `wow@80`；库 `acore_auth`、`acore_characters`、`acore_world`。MySQL 8.0.46 CLI 在 `D:/Program Files (x86)/mysql/bin/mysql.exe`（可用于临时 schema 校验 SQL，用完即删）。
- `outputs/` 下交付给用户的报告/设计文档/资源清单用中文文件名；文档内部交叉引用同步用中文路径。源码、脚本、SQL 迁移文件仍用工程命名。
- 新增/修改源码注释用简体中文；代码标识符、API 名、Spell/Talent/Glyph ID、DB 字段保持原名。
- 模块自定义表 SQL 放 `data/sql/custom/db_world/` 与 `custom/db_characters/`（非 `pending_db_*`），风格 `SET FOREIGN_KEY_CHECKS=0; DROP TABLE IF EXISTS; CREATE TABLE`。NPCBots 旁挂表家族：`characters_npcbot_transmog`/`_gear_set`/`_gear_storage`/`_group_member`/`_settings`/`_logs`。
- 构建：VS 生成器可用（`CMakePresets.json` 的 `win-vs2026-x64` → `build/AzerothCore.slnx`）；Ninja 备选在 `build-ninja/`。工具链 MSVC 14.51（VS2026）、CMake 4.3.1/4.4.2、Boost 1.91.0（`C:/local/boost_1_91_0`）、MySQL 8.0.46、WinSDK 10.0.22621.0。重配置须手动注入 MSVC/SDK 环境变量并显式传 `-DMYSQL_INCLUDE_DIR`/`-DMYSQL_LIBRARY`（PowerShell 禁用 cmd.exe，`Launch-VsDevShell.ps1` 非交互会卡）。

## 五人英雄裂隙

源码统一在 `src/server/scripts/HeroicDungeonRift/`：根目录放共享/配置/传送/管理代码；普通文件与 `AddSC_` 用 `rift_` 前缀，Boss 用 `boss_rift_`，入口 `AddHeroicDungeonRiftScripts()`；每副本独立子目录，副本适配 + 该副本全部 Boss 脚本放子目录内。

Entry 分配（不可重叠）：入口 NPC 100000；69 个 Boss 的 T1/T2/T3 连续 100001–100207（`100001 + (boss_id-1)*3 + (tier-1)`；60 级 boss_id 1–47 = 100001–100141，70 级 48–69 = 100142–100207）；专用同伴/召唤物 102000–102058（60 级 102000–102033、70 级 102034–102058）；出口门 100500–100502。主 Boss 83 级、召唤物 82 级；同 Boss 三档共用同一 `boss_rift_*`；双 Boss 同伴用 `npc_rift_*`。

## 物品学习技能

AzerothCore 特殊学习物品用 `spellid_1=483`、`spelltrigger_1=0`，目标法术放 `spellid_2`、`spelltrigger_2=6`。自定义物品 `指位技能` Entry 93003，更新文件 `data/sql/updates/pending_db_world/rev_20260815_01.sql`。

## DBC 读取

- WDBC 文件头固定 **20 字节**（magic `WDBC` + recordCount/fieldCount/recordSize/stringBlockSize）。按 24 字节读会整体错位、ID 列看似全 0。
- 字段顺序权威是 `src/server/shared/DataStores/DBCfmt.h` 的格式串，勿只凭 `DBCStructure.h` 推断。
- 客户端 DBC：`G:/wow/dbc/`（= `E:/WoW Spell Editor/DBC_335_wotlk/`）。实测规模：`Talent.dbc` 892×23×92（`niiiiiiiixxxxixxixxixxx`）、`TalentTab.dbc` 33×24×96（`nxxxxxxxxxxxxxxxxxxxiiix`）、`GlyphProperties.dbc` 362×4×16（`niix`）、`GlyphSlot.dbc` 10×3×12（`nii`）。服务端确实加载（`DBCStores.cpp:307-308、370-371`），可用 `sTalentStore`/`sTalentTabStore`/`sGlyphSlotStore`/`sGlyphPropertiesStore`。
- **法术 CSV 列错位**：`G:/wow/dbc_csv/Spell.csv` 中文名只在 **index 138（`Name_Lang_deDE`）**；index 134（enUS）/140（zhCN）均为空。法术族用 **index 206 `SpellClassSet`**（盗贼=8），此导出**没有** `SpellFamilyName` 列。描述 172、光环描述 189；换嘲用 DurationIndex 38、EffectAura_1/2/3 93-95、EffectAuraPeriod 96-98、EffectTriggerSpell 114-116、Effect 枚举 69-71。
- `SpellDuration.csv`（ID,Duration,PerLevel,MaxDuration，毫秒）常用值：1=10s、3=60s、8=15s、9=30s、18=20s、22=45s、25=180s、27=3s、28=5s、32=6s、35=4s、64=40s、572=100s。
- 天赋/雕文 DBC 语义（已实测）：**`TalentTab.ClassMask`（字段20）= `1 << (classID-1)`**，且 `BOT_CLASS_*` 真实职业值直接等于 classID（`botcommon.h:222-232`），无需映射表。盗贼 3 树 = **182 刺杀 / 181 战斗 / 183 敏锐**（tabpage 0/1/2）；其余职业：战士 161/164/163、圣骑 382/383/381、猎人 361/363/362、牧师 201/202/203、死骑 398/399/400、萨满 261/263/262、法师 81/41/61、术士 302/303/301、德鲁伊 283/281/282；409/410/411 的 ClassMask=0（非职业系）。**`Talent.dbc` 字段**：TalentID(0)、TalentTab(1)、Row(2)、Col(3)、RankID[5](4-8)、DependsOn(13)、DependsOnRank(16)、addToSpellBook(19)。**层级门槛 = `row R` 需本系已投 `5*R` 点**（row 0-based）。**两套 class mask 勿混用**：`GetBotClassMask0()=1<<(class-1)`（与 TalentTab.ClassMask、`Player::GetClassMask()` 同约定，天赋校验用）；`GetBotClassMask1()=1<<class`（模块自有 `BOT_CLASS_MASK_*`）。`BOT_CLASS_BM` 及其后 9 个自定义职业（从 12 起）无天赋树。
- 雕文：`GlyphProperties{Id, SpellId, TypeFlags}`，**`TypeFlags` 0=大雕文、1=小雕文**；`GlyphSlot.dbc` 中 `Order!=0` 的 6 条才是玩家槽位（`Id` 21–26、槽位索引=`Order-1`），索引 **0/3/5 为大、1/2/4 为小**（3 大 3 小）；Order=0 的 4 条不参与。零星的 TypeFlags 2/64/128/162 因等值比较天然排除，无需黑名单。校验照抄 `GlyphProperties.TypeFlags == GlyphSlot.TypeFlags`（`PlayerStorage.cpp:5932`、`SpellEffects.cpp:4611`），勿自造枚举。

## 换嘲配置判据

`spell_id` 必须填施加在坦克身上的 **Debuff 光环 ID**（非 Aura=23 触发技），层数须满足「叠层间隔×层数 ≤ 持续时间」。持续时间远小于施放间隔的 debuff 是刷新式、永远叠不到 2 层，应设 1 层（命中即换）。随机点名远程玩家的技能（`SelectTarget(Random)`，如威札斯印记、静电瓦解）不是换嘲机制。

## NPCBots 技能/天赋/雕文重构

**权威文档**：`outputs/NPCBot/BOT天赋构建/` 下 —— `NPCBot技能天赋雕文应用重构方案.md`（实施导向主文档，2026-10-14 重写，取代旧 `...重构分析.md`）、`..._核查过程存档.md`（核查方法存档）、`NPCBots职业脚本硬编码效果台账.csv`（战士 90 条范本）。

### 数据模型（三张表）

①**默认构筑** `npcbot_default_talent_plan` 放 **world 库**，主键 `bot_class+spec`，静态内容；②**玩家预设** `characters_npcbot_talent_plan`，主键 `(owner, bot_class, spec, preset_index)`，一系多套；③**生效快照** `characters_npcbot_applied_plan` 单独建表，主键 `entry+spec`，含 `plan_source`(0系统默认/1玩家预设/2玩家直编)、`preset_ref`(仅 =1 有效，仅 UI 溯源)、`built_at_level`、`plan_total_points`、`applied_points`。**不加列到 `characters_npcbot`**。三表 `talent_data` 语义统一为「**完整序列**」（不截断），必须存 **talentId 非 spellId**；表3 另记 `applied_points` 作**生效前缀**——**生效天赋 = 前 `applied_points` 项**。雕文不参与截断（`glyph_data` 即所选方案，<20 级不生效）；雕文槽 20 级后全开（6 槽）。

### 关键规则

- **新雇佣 Bot 一律用系统默认构筑**，不继承玩家任何偏好。
- **「应用」与「保存为预设」是两个互相独立的操作，不可合并**（2026-10-14 用户明确）：`ApplyBotPlan`（含「界面上改几项后直接点应用」这一最常用场景）只写生效快照、`plan_source=2`、**不落预设库**、快照即唯一副本；`SaveBotTalentPreset` 只写预设库、**不改变任何 Bot**。**不要提供「保存并应用」复合接口**。
- **`plan_source` 由操作方式决定，不由内容决定**（勿反查内容匹配预设，否则 UI 溯源会撒谎）。
- 生效层**存快照不存引用**；删预设只影响 `plan_source=1` 的 Bot（重置为 0，下次重建回默认），`=0/2` 不受影响。
- 校验放 BotDataMgr，应用编排放 bot_ai。客户端天赋树 UI 由 mod-ale 实现，服务端接口经 mod-ale `RegisterFunctions()`（`modules/mod-ale/src/LuaEngine/LuaFunctions.cpp:1898`）注册。

### 天赋点数规则（勿改回玩家公式）

Bot **不套用玩家 71 点上限**，**构筑声明的点数也不设上限**。`生效预算 A(等级) = 等级基准 + 额外天赋点`；**等级基准 = `max(0, 等级-9)`**（10 级起每级 1 点，与玩家一致）；**额外天赋点由外部机制提供、只预留 `GetBonusTalentPoints()` 接口、不设计来源**。**`A(等级)` 是截断阈值，不是拒绝条件**——120 点构筑用在 40 级 Bot 上不报错，只生效前 31 点并记 `plan_total_points`/`applied_points` 差额。拒绝条件**只有结构性非法**（`talentId` 不存在、职业树不匹配、超 `maxRank`、层级未解锁、前置未满足、序列前缀不合法）。等级上限 83（`DEFAULT_MAX_LEVEL+3`）无需钳制。保留逐级回放（每级最多消费 1 点）。

### 档位规则：升级是否自动补点

- **自动档**（`plan_source=0` 系统默认 / `1` 玩家预设，来源是完整方案）→ 升级**自动**按新预算向后补齐。
- **手动档**（`2` 玩家直编，手改稿件）→ 升级**不自动补点**；`applied_points` = 玩家确认生效的点数（**可小于预算**，天赋本就可点不满）。**未分配点数** = `A(等级) - applied_points`（自动档恒 0），不是错误状态。
- **`applied_points` 确定规则（所有写入路径通用）**：`min(提交序列点数, A(当前等级))`。40 级提交 80 级配满的 120 点 → 生效 31；40 级只点 15 点 → `applied_points=15`、未分配 16。两者都进手动档。
- **三条出口**：①自己配置（再调 `ApplyBotPlan`，仍手动档）；②`ApplyBotDefaultPlan` **恢复默认**（→ 自动档 0，**是玩家的正常操作**）；③`ApplyBotTalentPreset` **使用预设**（→ 自动档 1）。
- **非对称规则**：升级不自动补（手动档），但**降级时两档都必须截断到 `A(等级)`**。
- **复用判据按档分化**：自动档 = `built_at_level == 当前等级` **且** `applied_points == A(等级)`（额外点可能在等级不变时变化）；**手动档 = 只看 `applied_points <= A(等级)`**（等级上升**不触发重建**，否则每次升级白跑一遍解析；只有降级才截断）；手动档下 `built_at_level` 仅作审计/显示。UI 须显示未分配点数，可提供「按当前方案补满剩余 X 点」快捷操作（= 提升 `applied_points` 到 `min(plan_total_points, A(等级))` 后再调一次 `ApplyBotPlan`，**玩家触发而非自动**）。

### 必须替换职业脚本硬编码，禁止两套并行

效果分散 **3 类载体**（10 个玩家职业文件统计）：**A** `ApplyClassPassives()` 内 `RefreshAura(spellId, 等级/专精条件 ? 1 : 0)`（**425** 处）；**B** `ApplyClass*` 数值/时长/冷却修正（**160** 个覆写，内嵌 `crit_chance += N`/`pctbonus *= x`/`cooldown -= N` + **178** 处 `//Glyph of ...` 注释，多数**连常量都没有**）；**C** AI 决策分支。

**残留后果按载体不同**：A 类因 `RefreshAura` 内部先 remove 再 add → **互相覆盖**（效果时有时无，难查）；**B 类 `+=`/`*=` 累加累乘 → 双重累加直接超模，不报错不崩溃**。→ **B/C 必须先删后用，禁止并行**；A 类可短暂「先建后拆」但窗口不得发布。**迁移固定四步（同批次）**：登记 → 建立 → 验证等价 → 删除。**阶段 A 清点单位是「效果」不是常量**。**非迁移范围（套装/技能自带/Bot 平衡）必须登记白名单**，否则阶段 D′ 零残留复查会误删。

**等级基准误用范围**：`master->GetLevel()` 在 **10 个玩家职业文件**（warrior:2038、dk:1905、druid:2979、hunter:2387、mage:1867、paladin:2349、priest:2215、rogue:2383、shaman:2636、warlock:2115）+ `bot_ai.cpp:7721/7775`（非「12 个文件」）。新代码一律 `me->GetLevel()`。

### NPCBots 等级与被动机制（不可凭记忆）

①Bot 实际等级在 `bot_ai::SetStats()`（`bot_ai.cpp:2394`）计算：雇佣 Bot = `min(master等级) + GetLevelBonusForBotRank(rank)`，受 `GetMinLevelForBotClass` 下限与 `DEFAULT_MAX_LEVEL+3`(=83) 上限；自由 Bot 走独立 `_baseLevel`/`_killsCount`。②`RefreshAura(spellId, count, target)` = `RemoveAurasDueToSpell` 后按 count（**仅 0/1**）`AddAura`，幂等但**不支持叠层**；天赋按 rank 用**不同 spell ID** 互斥刷新（如 `DEEP_WOUNDS_1/2/3`）。③重建序列注释锁定 `InitSpells()` 必须先于 `ApplyClassPassives()`（`bot_ai.cpp:2448`）。④等级变化触发链：`Player::GiveLevel` → `OnPlayerLevelChanged`（`Player.cpp:2686`）→ `BotMgr::SetBotsShouldUpdateStats()`（`botmgr.cpp:1477`）→ `UpdateAI` 中 `SetStats(false)`；但自由 Bot 击杀累积（`bot_ai.cpp:17052`）、招募重召（`botmgr.cpp:549`）、装备变更（`bot_ai.cpp:14405`）**不经过**该回调，故统一入口 `ReconcileBotProgression()` 须挂进 `SetStats()` 的 `if (force)` 块。⑤解雇清理是**两条**路径：`NPCBOT_UPDATE_OWNER`（`botdatamgr.cpp:3159`，批量置 owner=0）与 `NPCBOT_UPDATE_ERASE`（`:3136`，删 `characters_npcbot` 行，调用点 `botcommands.cpp:3654/3706`）；批量分支语句须在 `UPDATE owner` **之前**求值。⑥已有重建序列 `SetSpec(activate=true)`（`bot_ai.cpp:16031-16060`）：写库 → `UnsummonAll` → `removeShapeshiftForm` → `RemoveAllAurasExceptType` → `ApplyItemsSpells` → `ApplyRacials` → `InitSpells` → `ApplyClassPassives` → `InitHeals` → 清三系能量；构筑应用步骤须插在 `InitSpells()` **之前**。

### 效果应用三路径

天赋/雕文效果技能**无需手工准备**，运行时反查（天赋 `TalentEntry->RankID[rank-1]`；雕文 `GlyphPropertiesEntry->SpellId`），只需准备「加点顺序 + 雕文选择」。**默认路径 1 DBC 光环**（复用 `RefreshAura`，已内建 apply/remove 对称语义）；②光环挂不上或依赖 `NpcBotStats` 自有计算时走代码数值修正（有 DBC 漂移风险）；③效果是 AI 决策时走行为分支。每条路径都须标注对应 Talent/Glyph/Spell ID 作可追溯锚点。

## 盗贼实施项（首个落地职业，2026-10-14）

交付物在 `outputs/NPCBot/BOT天赋构建/`：`NPCBot盗贼默认构筑与脚本改造台账.md`（主文档）、`npcbot_default_talent_plan_rogue.sql`（表1 建表+三系 INSERT，**已在临时 schema 实测执行通过**）、`NPCBot盗贼脚本硬编码效果台账.csv`（108 项）、`盗贼天赋树全量.csv`（83 天赋）、`盗贼雕文全量.csv`（35 雕文）。

- **三系构筑**（`bot_class=4`；spec 10 刺杀 / 11 战斗 / 12 敏锐）：刺杀 89 点、战斗 93 点、敏锐 89 点。均通过层级（row R 需本系 5*R 点）+ `DependsOn`/`DependsOnRank` + 雕文槽位类型三重校验。主系配到 64–69 点（因**点数不设上限**，可配满而非仅 51）。
- **雕文默认**：刺杀 `0:409影袭 3:733毁伤 5:410切割 1:466 2:469 4:468`；战斗 `0:409 3:394剑刃乱舞 5:410 …`；敏锐 `0:393背刺 3:404出血 5:410 …`（小雕文统一 466/469/468）。
- **盗贼脚本基线**：`bot_rogue_ai.cpp`（2495 行）。A 类已半程表驱动 `RogueTalentDef RogueTalents[]`（`:208-253`，39 项，`RefreshAura` 4 处），但仍是 `{spec, minLvl, maxLvl, auraId}`、**天赋与雕文混表**（`:214` `GLYPH_BACKSTAB`）、用 `master->GetLevel()`（`:2383`）、`maxLvl` 半开区间近似 rank（`SEAL_FATE1[35,37)`…）。B 类 11 个 `ApplyClass*` 覆写 + `SpellHitTarget`，共 **59 处**；C 类 7 处；总 **108 项效果**（天赋 77 / 雕文 25 / 非迁移 6）。
- **盗贼雕文漏删风险**：35 条雕文中**仅 1 条在 A 类**（背刺雕文），其余 **24 条只在 B/C 类**（刺骨/刀扇/影袭/剑刃乱舞/佯攻/凿击/毁伤/开锁/杀戮盛筵/锁喉/致命投掷/血之饥渴/暗影斗篷/疾跑/消失/冲动/闪避/切割/暗影之舞/割裂/破甲/嫁祸诀窍/伏击）——按「改造 `ApplyClassPassives()`」理解范围会漏 **96%**。
- 改造顺序：B 统一 `me->GetLevel()` → C 接入天赋（同批删 B/C 类天赋数值）→ D 接入雕文（先删后用）→ D′ 零残留复查对账。
