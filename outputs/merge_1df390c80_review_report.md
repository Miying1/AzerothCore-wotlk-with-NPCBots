# 合并提交 `1df390c80` 全面审查报告

## 1. 审查对象与结论

- 当前分支：`merge_test2_from_npcbots`
- 目标提交：`1df390c80587e810884d8c59dd10566c95e21775`
- 提交说明：`Merge test2 custom features into npcbots_3.3.5 base`
- 第一父提交（新基线）：`5c882898a53adf41445a2a35a4357f3f09304e6e`
- 第二父提交（test2）：`c331574c1df26b9b871a55dd491fd4883edc3be3`
- 共同祖先：`6f6a3449c0669e23bf0ef5de545d43d6996783b4`
- 分叉规模：基线侧约 5,592 个提交，test2 侧 120 个提交；test2 累计改动 373 个文件。
- 冲突规模：提交说明称 72 个文件、239 个冲突块；`git show --remerge-diff` 同样识别 72 个冲突文件。

### 总体判断

**该合并不能判定为正确，当前结果不具备可编译、可发布条件。**

审查确认至少存在：

- **7 处确定性编译阻断问题**；
- **6 组高风险功能回退或语义错误**；
- 若干中低风险逻辑漂移与代码质量问题。

因此，冲突解决并非整体合理。合并确实保留了大量 test2 自定义模块和部分关键功能，但**原 test2 功能已受到实质影响**，包括同 IP 机器人限制、装备查看/改名接口、ICC 巫妖王机器人目标兼容、挑战模式注册、死亡骑士宠物行为等。

> 本次审查只读取代码、Git 历史和差异，没有修改任何源码。

---

## 2. 审查方法与限制

使用了以下只读检查：

1. 确认提交拓扑、双方父提交、共同祖先和分叉规模；
2. 检查普通 diff、相对双方父提交的 diff、`remerge-diff`；
3. 逐项核对原冲突分析文档中列出的 72 个冲突文件与关键 test2 功能；
4. 扫描残留冲突标记；
5. 运行 `git diff --check`；
6. 对核心引擎、NPCBots、副本脚本进行独立静态复核；
7. 尝试配置构建。

限制：当前环境未安装或未配置可调用的 CMake/C++ 编译器，因此无法完成真实编译。尽管如此，下述多项问题属于语法、声明匹配或函数结构层面的确定性错误，不依赖完整构建即可确认。

---

## 3. 阻断级问题：当前代码确定无法正常编译

### B-01 `Unit.cpp` 构造函数初始化列表被错误拼接

- 文件：`src/server/game/Entities/Unit/Unit.cpp`
- 位置：约 190–197 行，`DamageInfo::DamageInfo(SpellNonMeleeDamage ..., uint32 hitMask)`
- 严重度：**阻断**

当前代码在 `m_hitMask(hitMask)` 后无逗号直接追加第二组重复初始化，并包含当前类已不存在的 `m_procEx`：

```cpp
m_cleanDamage(...), m_hitMask(hitMask)
m_absorb(...), m_resist(...), m_block(...),
, m_procEx(...)
```

这是将新基线的 `m_hitMask` 数据模型与 test2 旧版 `m_procEx` 数据模型机械拼接的结果。当前 `DamageInfo` 声明中没有 `m_procEx`。

**影响**：核心 `Unit.cpp` 必然编译失败。

### B-02 `MotionMaster::MoveTakeoff` 缺少右花括号

- 文件：`src/server/game/Movement/MotionMaster.cpp`
- 位置：约 588–609 行
- 严重度：**阻断**

`if (speed > 0.01f) {` 没有在 `init.SetVelocity(speed);` 后闭合，后续动画和函数尾结构被吞入该分支。

**影响**：语法结构损坏，后续函数解析失败。

### B-03 `CreatureAI::_EnterEvadeMode` 缺少右花括号

- 文件：`src/server/game/AI/CreatureAI.cpp`
- 位置：约 424–446 行
- 严重度：**阻断**

`else if (CreatureGroup* formation = me->GetFormation()) {` 在 `formation->MemberEvaded(me);` 后未闭合，导致 TempSummon 通知、`EngagementOver()` 和函数尾的结构错误。

**影响**：编译失败；即使仅补在错误位置，也可能把通用召唤物通知错误限制到“存在编队”条件中。

### B-04 `InstanceScript.cpp` 文件尾多余右花括号

- 文件：`src/server/game/Instances/InstanceScript.cpp`
- 位置：约 1061–1062 行
- 严重度：**阻断**

`RefreshChallengeBuff()` 已闭合，之后又多出一个 `}`，文件不存在对应命名空间开括号。

**影响**：编译失败。

### B-05 `botcommands.cpp` 丢失 WP 命令函数头

- 文件：`src/server/game/AI/NpcBots/botcommands.cpp`
- 位置：约 957 行
- 严重度：**阻断**

当前位置只有裸代码块，内部使用 `handler`，但函数签名 `HandleNpcBotWPSpawnAllCommand(ChatHandler* handler)` 被冲突解决丢失；命令注册表仍引用该函数。

**影响**：`handler` 未声明，注册函数不存在，NPCBots 命令模块无法编译。

### B-06 `bot_ai.cpp` 多个成员实现没有头文件声明

- 文件：
  - `src/server/game/AI/NpcBots/bot_ai.cpp`
  - `src/server/game/AI/NpcBots/bot_ai.h`
- 符号：
  - `SendEquipsToOwner()`（约 439 行）
  - `SendEquipList(Player*)`（约 21470 行）
  - `LoadEquipPartName(uint8)`（约 21500 行）
- 严重度：**阻断**

这些 test2 实现被保留，但合并后的 `bot_ai.h` 没有相应声明；调用点仍存在，例如 `botcommands.cpp` 的装备查看命令。

**影响**：`no declaration matches` / `no member named` 编译错误；装备查看功能不可用。

### B-07 `BotDataMgr` 与 `BotMgr` 实现/声明脱节

- 文件：
  - `src/server/game/AI/NpcBots/botdatamgr.cpp/.h`
  - `src/server/game/AI/NpcBots/botmgr.cpp/.h`
- 符号：
  - `BotDataMgr::GetNpcBotCountByIp(std::string)`
  - `BotDataMgr::SetBotName(Creature*, std::string)`
  - `BotMgr::GetIPMaxBots()`
- 严重度：**阻断**

`.cpp` 中保留了这些 test2 实现，但合并后的头文件中相应声明缺失。`SetBotName` 仍被 `bot_ai.cpp` 调用。

**影响**：声明匹配失败；机器人改名和 IP 统计功能被阻断。

---

## 4. 高风险功能回退和语义错误

### H-01 同 IP 最大机器人数量限制被实际删除

- 文件：`src/server/game/AI/NpcBots/botgiver.cpp`
- 位置：约 78–96 行，雇佣菜单
- 严重度：**高**

原 test2 会同时检查：

- 玩家等级对应的个人机器人上限；
- `GetNpcBotCountByIp(RemoteAddress)`；
- `NpcBot.IpMaxBots`；
- VIP 豁免；
- 同 IP 绝对上限 18。

合并结果只保留 `BotCfg::GetMaxNpcBots()` 和账号上限检查。虽然 `botmgr.cpp` 中仍读取 `NpcBot.IpMaxBots`，也残留 IP 统计实现，但雇佣路径不再调用，且声明还缺失。

**影响**：同 IP 多账号可绕过 test2 的限制；提交说明中“保留 IP-based bot limits”的结论不成立。

### H-02 巫妖王战斗中的 NPCBot 玩家兼容部分丢失

- 文件：`src/server/scripts/Northrend/IcecrownCitadel/boss_the_lich_king.cpp`
- 关键位置：约 409、2342、3519 行
- 严重度：**高**

至少以下 test2 的 `IsNPlayer()` 被上游 `IsPlayer()` 覆盖：

- `NonTankLKTargetSelector`；
- `VehicleCheck`；
- 邪恶灵魂 `CanAIAttack`。

**影响**：NPCBot 可能不会成为部分非坦克机制、载具/污染过滤或邪恶灵魂的合法目标，战斗机制和难度发生明显变化。

正面情况：LK 低血量阶段的“存活玩家检查”仍有保留，说明不是整个功能都丢失，而是冲突处理不一致。

### H-03 挑战模式注册和重置钩子部分丢失

- 文件：
  - `src/server/scripts/Northrend/UtgardeKeep/UtgardePinnacle/instance_utgarde_pinnacle.cpp`
  - `src/server/scripts/Northrend/VioletHold/instance_violet_hold.cpp`
  - `src/server/scripts/Northrend/UtgardeKeep/UtgardePinnacle/boss_skadi.cpp`
- 严重度：**高**

确认丢失：

- 乌特加德之巅 `OnCreatureCreate` 中的 `AddChallengeCreature`；
- 紫罗兰监狱 `OnCreatureCreate` 中的 `AddChallengeCreature`；
- Skadi `Reset()` 中的 `SetChallengeMode`。

此外，紫罗兰监狱的新 `OnPlayerEnter` 也未保留 test2 的 `CheckChallengeMode()`。

**影响**：部分怪物未被登记到挑战系统，Boss 重置后可能不重新应用挑战缩放，玩家进入时挑战状态也可能未初始化。

### H-04 死亡骑士临时召唤物会错误解除现有食尸鬼

- 文件：`src/server/game/AI/NpcBots/bot_death_knight_ai.cpp`
- 位置：约 1720–1735 行，`JustSummoned`
- 严重度：**高**

函数先处理符文武器和石像鬼，然后无条件检查现有 `botPet` 并将其解除召唤。结果是召唤石像鬼或符文武器时，可能把永久食尸鬼清掉。

**影响**：死亡骑士机器人宠物和技能轮转被破坏；属于两侧逻辑直接串接、缺少对象关系判断的典型冲突错误。

### H-05 1–79 级角色的机器人所有权会被当作过期

- 文件：`src/server/game/AI/NpcBots/bot_ai.cpp`
- 位置：约 320–352 行，`CheckOwnerExpiry()`
- 严重度：**高**

当前过期条件为：

```cpp
if (timeNow >= baseTimeStamp + expireTime || ownerLevel < 80)
```

注释声称第二条件表示“owner does not exist”，但等级小于 80 并不等于角色不存在。

**影响**：开启所有权过期机制后，低等级玩家机器人会被强制解绑、装备邮寄并重置专精/职责。若这是有意业务规则，也应使用明确配置和日志，而不是伪装成“角色不存在”。

### H-06 `SpellEvent` 删除失败后不再中止进程

- 文件：`src/server/game/Spells/Spell.cpp`
- 位置：约 8429–8443 行，`SpellEvent::~SpellEvent`
- 严重度：**高**

新基线在不可安全删除 Spell 时执行 `ABORT()`；合并结果采用 test2 的 `//ABORT()`，仅记录错误并继续。

**影响**：在生命周期不变量已破坏时继续运行，可能造成持续泄漏、悬挂状态或后续崩溃。该变化未体现为明确设计决策，属于高风险回退。

---

## 5. 中低风险问题

### M-01 `spell_item.cpp` 对 60510 存在双实现

`spell_item_healing_trance` 与 `spell_item_soul_preserver` 都包含 60510 处理逻辑，并都注册。最新 SQL 通常只绑定新实现，因此未必立即双触发，但旧数据库或绑定漂移会产生不一致。

### M-02 海里昂假死判断从通用 Aura 类型收窄为固定 5384

- 文件：`boss_halion.cpp`

原 test2 使用通用 `SPELL_AURA_FEIGN_DEATH`；合并后只检查 `HasAura(5384)`。其他等价假死效果可能被误判为有效玩家。

### M-03 大量补丁格式问题

`git diff --check` 在核心 `src/server/game` 与 `src/server/scripts` 范围内返回约 120 条问题，主要是尾随空白。整个合并还引入大量第三方模块中的尾随空白、Tab 缩进和 EOF 空行问题。

这些通常不是运行时缺陷，但反映合并质量控制不足，并可能触发 CI 代码风格检查。

### M-04 不应将工作记忆文件提交到产品源码

合并提交新增 `.workbuddy/memory/2026-07-23.md`。这不是 test2 游戏功能，属于工作过程文件，进入正式源码历史会造成仓库污染和潜在内部信息泄露。

---

## 6. test2 功能保留评估

### 完整或基本完整保留

1. **test2 自定义模块目录**：重点检查的 16 个自定义模块相对 test2 父提交内容无差异，包括：
   - `mod-ah-bot`
   - `mod-congrats-on-level`
   - `mod-individual-xp`
   - `mod-learn-spells`
   - `mod-multi-client-check`
   - `mod-npc-gambler`
   - `mod-player-transmog`
   - `mod-playerchallenge-modes`
   - `mod-playervip`
   - `mod-raidleader-reawrd`
   - `mod-random-enchants`
   - `mod-reward-shop`
   - `mod-skip-dk-starting-area`
   - `mod-transmog`
   - `mod-weapon-visual`
   - `mod-zone-difficulty`
2. **VIP Core API**：`Player::SetVip/IsVip` 声明、实现和初始化均保留。
3. **假玩家 WhoList**：`WhoListOnlineBot` 配置和 `WhoListCacheMgr::AddOnlineBot` 调用保留。
4. **辛达苟萨冰墓 NPCBot 处理**：核心分支保留。
5. **卡拉赞象棋修复**：关键回位及边界逻辑保留。
6. **StatSystem 伤害乘数修复**：与 2026-07-23 的 test2 修复意图一致，仍在合并结果中。
7. **上游 NPCBots 新能力**：新的 `BotCfg`、威胁系统入口、动作/反制队列、装备银行和副本机器人装备框架总体已纳入。

### 部分保留但当前不可用或不完整

1. **机器人装备查看**：命令和 `.cpp` 实现存在，但头文件声明丢失，无法编译。
2. **机器人改名**：调用和实现存在，但声明丢失。
3. **IP 限制**：配置读取和部分实现残留，但雇佣入口删除检查，功能实质失效。
4. **挑战模式**：核心 API 和部分副本钩子存在，但多个副本注册/重置钩子丢失，行为不完整。
5. **ICC 机器人机制兼容**：部分 `IsNPlayer()` 保留，部分关键位置退回 `IsPlayer()`。
6. **NPCBot 职业增强**：大量代码被保留，但 DK 召唤逻辑出现新的行为错误。

### 无法在当前条件下确认

- 数据库完整迁移和所有模块 SQL 的可执行性；
- Worldserver 启动与配置加载；
- 机器人离线自动重连的端到端行为；
- 所有副本/首领机制的运行时表现；
- 装备银行、威胁系统和 test2 职业增强之间的数值平衡。

这些必须在修复阻断问题并完成编译后，通过数据库和游戏内回归测试确认。

---

## 7. 冲突解决合理性评价

### 合理之处

- 大多数纯 test2 模块完整带入；
- 若干核心扩展（VIP、WhoList、挑战 API、`IsNPlayer` 基础设施）被保留；
- 部分文件确实采用了“上游新结构 + test2 功能”的正确策略；
- 对辛达苟萨、卡拉赞象棋、StatSystem 等功能的保留较好。

### 不合理之处

- 出现多处明显的花括号缺失、重复初始化列表、函数头丢失，说明冲突解决后没有进行最基本的编译验证；
- `.cpp` 实现与 `.h` 声明不同步，是典型的逐文件孤立解决，没有做符号级闭环检查；
- 提交说明声称保留的功能（IP 限制、完整副本兼容）与实际代码不一致；
- 对 `IsNPlayer()` 的选择不统一，同一 Boss 脚本中部分保留、部分回退；
- 将两侧语句直接串接，未重新验证控制流和对象关系，例如 DK `JustSummoned`；
- 将高风险行为变化（`ABORT()` 移除）混入冲突解决，缺乏明确理由和测试证据。

**综合评级：不合格。** 建议不要把 `1df390c80` 视为可用的合并基线。

---

## 8. 建议的修复与验证优先级

本报告不修改代码，以下仅为后续建议：

1. **先修复 7 个编译阻断项**，并要求完整 Debug/RelWithDebInfo 编译通过；
2. 对全部 72 个冲突文件再次执行 `remerge-diff` 人工复核，禁止只看普通 diff；
3. 建立 test2 功能清单，至少覆盖：
   - IP bot 限制与 VIP 豁免；
   - 装备查看、改名、雇佣金额；
   - 机器人自动重连和受限地图清理；
   - ICC LK、辛达、海里昂；
   - 挑战模式进入、怪物登记、Boss Reset、计时器；
   - DK/猎人/术士/法师等定制职业行为；
4. 对所有原 test2 `IsNPlayer()` 改动做自动化差异清单，逐个说明为何保留或删除；
5. 启动测试至少覆盖：worldserver 加载、模块配置、数据库 SQL、机器人雇佣/解雇/重连；
6. 游戏内回归至少覆盖 ICC、乌特加德之巅、紫罗兰监狱和挑战模式；
7. 清理工作过程文件和格式问题，再运行项目 CI/code-style。

---

## 9. 最终结论

- **合并代码是否正确：否。** 存在多个确定性编译错误。
- **冲突解决是否合理：部分合理，但整体不合格。** 大量功能被保留，但关键冲突存在机械拼接、结构丢失和声明脱节。
- **是否影响原 test2 功能：是。** 已确认影响 IP 限制、装备查看/改名、LK 机器人机制、挑战模式部分副本、DK 宠物行为、低等级所有权等。
- **是否建议继续基于该提交发布或部署：不建议。** 应先修复阻断项并完成编译与游戏内回归。
