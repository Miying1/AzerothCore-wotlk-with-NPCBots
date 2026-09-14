# 裂隙 Boss 战斗流程续审报告

## 1. 审计范围与结论边界

本轮继续复核 `src/server/scripts/HeroicDungeonRift/` 中的事件派发、技能目标、`triggered` 语义、召唤物派生状态、特殊 Aura 和双 Boss 重置流程。

已完成静态验证并修复确定缺陷；未执行 CMake、编译或进服测试，因此本文不把静态结论描述为运行时最终验证。

## 2. 本轮已确认并修复

### 2.1 全模块 EventMap 单帧连发与事件吞失

`BossAIBase` 和 `RiftSummonAI` 原先使用 `while (ExecuteEvent())`，同一帧会取出全部到期事件。前一事件开始正常读条后，后续正常施法可能失败但仍被重排；多个 triggered 技能则会同帧叠放。

现统一改为每个 AI 帧最多处理一个事件。并反查所有自定义 `UpdateAI()`，同步修正：

- Kael'thas 第二阶段；
- Whitemane 独立状态机；
- Nethekurse；
- Kargath；
- Mennu 四种图腾；
- Archaedas 石像苏醒 AI。

全目录已无 `while (uint32 eventId = ...ExecuteEvent())`。

### 2.2 召唤物一次性状态未 Reset

增加派生类 Reset：

- Argelmach Golem：`_frenzied = false`；
- Kargath Heathen：`_enraged = false`；
- Kargath Reaver：`_enraged = false`。

修复召唤物独立脱战后再次战斗不会重新触发狂乱/激怒的问题。

### 2.3 Mograine/Whitemane 灭团重置

Mograine 的基类 Reset 会先将旧 Whitemane 加入删除队列，随后 `PrepareWhitemane()` 仍可能通过旧 GUID 找到待删除对象并错误复用。

新增 `SummonedCreatureDespawn()`，旧 Whitemane 反召唤时同步清空 `_whitemaneGuid`，确保 Reset 后创建新的有效 Whitemane。

### 2.4 特殊 Aura 清理

- Maladaar Stolen Soul：死亡、自身 Reset、Boss 强制反召唤均按 Maladaar caster GUID 移除 `32346`；
- Muselek：记录实际施加 Hunter's Mark 的目标，在 Reset/死亡时按 Muselek caster GUID 清理；
- Laj：Reset 时先移除五种形态 Aura，再恢复默认形态；
- Ikiss：Reset 时显式移除 Mana Shield 与 Arcane Bubble；
- Doan：沿用前轮已加入的 Arcane Bubble Reset 清理。

### 2.5 父 Aura 脚本数据库绑定

实际查询 `acore_world.spell_script_names` 后确认 `15790`、`16247` 均无绑定，专用脚本不会加载。

新增：

`data/sql/updates/pending_db_world/rev_20260816_01.sql`

绑定：

- `15790 -> spell_rift_doan_arcane_missiles`；
- `16247 -> spell_rift_lethtendris_curse_of_thorns`。

SQL 尚未执行。

## 3. 已确认当前实现有依据，不修改

- Baelgar `13880 Magma Splash`：目标和多效果槽位匹配，当前保持正常施法；
- Gandling 六房间传送：初始 `17950` 非 triggered，定向传送 triggered，与原脚本一致；
- Pathaleon `39096 Polarity Shift`：原标准脚本同样使用 triggered；
- Nethekurse `30496 Shadow Fissure`：随机玩家位置目标解析正确，triggered 与原脚本一致；
- Anastari `14515 Dominate Mind`：当前注释明确为 T3 瞬发设计，不仅因 DBC 有读条就擅自修改。

## 4. 尚未改动的设计/运行时争议

### 4.1 Warp Splinter `34761 Plant Seedlings`

DBC 目标是 `TARGET_DEST_CASTER_FRONT`，脚本传入的随机玩家不会决定召唤点；实际召唤普通 Entry `19969`，不会经过裂隙专用 `102049`、`SummonTieredCreature()` 和 Tier 属性继承。

需确认设计是“Boss 前方生成独立毒性幼苗”，还是“随机玩家位置生成裂隙缩放幼苗”。未在设计不明确时改写机制。

### 4.2 Nethekurse Shadow Fissure 伤害链

`30496 -> 17471 -> 30497 -> 30498` 的后续伤害由普通召唤物触发，不直接进入 Boss 调谐链。目标与施法方式正确，但 T1/T2/T3 最终伤害是否应缩放需进服实测或设计确认。

### 4.3 Soccothrates `35322 Shadow Power`

DBC 有 2 秒读条，当前 T2/T3 使用 triggered 瞬发；标准脚本使用正常施法。是保留预警/打断窗口，还是刻意设计为即时 Buff，需要遭遇设计确认。

### 4.4 Kael'thas 第二阶段

进入 50% 后 `events.Reset()` 永久移除 P1 的 T2/T3 技能，只保留重力流逝链。该行为已确认，但是否应让新增技能贯穿 P2 尚无权威设计结论。

### 4.5 `15790` 的共享影响

`15790` 同时被 Doan T2/T3 和 Gandling T1 使用。专用 AuraScript 只在裂隙 Tier >= 2 时接管，因此 Gandling T2/T3 也会应用同一固定每跳基线。需确认这是共享调谐意图，还是只应限定 Doan。

### 4.6 旧空接口

`SetRaidSpellDamageMultiplier()` 已为空实现，但仍有大量 Boss 调用。当前不会导致重复倍率，但会误导维护；建议后续单独清理，不与战斗行为修复混在一起。

## 5. 验证结果

- `git diff --check`：通过，仅提示一个文件未来会由 CRLF 转为 LF；
- 伤害调谐覆盖：69 个 Boss、112 个裂隙 C++ 文件、202 个伤害 Spell ID、42 个召唤物伤害 Spell ID，`missing=[]`、`missing_summon=[]`；
- SQL 风格：本文件内容检查已无 SQL 规则报错；检查脚本随后因远端没有 `origin/master` 异常退出；
- C++ 全库风格：仍被 NPCBots 等既有问题阻断；本轮模块唯一命中的多余空行已修复；
- 构建/编译/进服：按项目约定未执行。

## 6. 建议的运行时验证顺序

1. 数据库应用 pending SQL 后确认 `15790/16247` 脚本加载；
2. Doan 与 Lethtendris 父 Aura 是否只触发一次自定义子伤害；
3. Warp Splinter `34761` 的召唤位置、Entry、ScriptName、生命周期和伤害；
4. Nethekurse `30496 -> 30498` 的 T1/T2/T3 实际伤害；
5. Mograine 灭团后第二次进入假死/Whitemane 阶段；
6. Maladaar、Muselek、Laj、Ikiss 在技能生效中途灭团后的 Aura 清理；
7. Argelmach、Kargath 召唤物独立脱战后的第二次低血量触发；
8. 同时间到期技能是否按相邻 AI 帧依次执行且不再吞失。
