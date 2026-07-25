# test2 → merge_test2_from_npcbots 合并冲突详细分析

> **基分支**: `npcbots_3.3.5` (最新上游)
> **合并分支**: `test2` 
> **目标分支**: `merge_test2_from_npcbots`
> **状态**: 已创建分支并执行 merge，保留冲突待手动解决

---

## 总览

| 统计项 | 数值 |
|--------|------|
| 冲突文件数 | **73** |
| 冲突块总数 | **239** |
| test2 提交数 | ~108 |
| npcbots_3.3.5 新提交 (含上游 AC) | 5,592 |

---

## 一、核心引擎层 (10 文件, 106 冲突块) 🔴 最关键

### 1. `src/server/game/Entities/Unit/Unit.cpp` — 92 冲突块

**冲突性质**：test2 在 Unit 中注入了大量 NPCBot 钩子代码，上游也重构了核心系统。

| 冲突位置 | test2 改动 | 上游改动 | 解决建议 |
|----------|-----------|---------|---------|
| 头文件区 (line ~79) | 引用了 `botconfig.h`, `botdatamgr.h` 等 | 新增了 `botconfig.h` | **保留 test2 的 include**，同时确保上游新增的也都有 |
| 构造函数 (lines 117,135) | 添加了 `m_procEx`, `m_procDeep(0)`, `m_AutoRepeatFirstCast(false)` 等 | 添加了 `m_hitMask(0)` | **合并双方**：把 test2 成员和上游 `m_hitMask` 都加进去 |
| 成员初始化列表 (line 371) | 添加了大量 NPCBot 相关初始化：`m_movedByPlayer`, `m_ControlledByPlayer`, `m_CreatedByPlayer`, `m_realRace`, `m_race` | 上游修改了部分初始化顺序 | **保留 test2 的 NPCBot 成员**，融合上游的初始化 |
| 大量分散冲突 | 在每个方法中添加 `//npcbot` 标记的定制代码 | 上游修改了对应方法的核心逻辑 | **逐一比对**，保留 test2 的 bot 逻辑但适配上游新 API |

**影响**：这是最核心的文件，一旦解决错误会导致编译失败或运行时崩溃。建议优先解决。

---

### 2. `src/server/game/AI/CreatureAI.cpp` — 3 冲突块

| 冲突位置 | test2 改动 | 上游改动 | 解决建议 |
|----------|-----------|---------|---------|
| line 33 (include) | `#include "../../../../modules/mod-zone-difficulty/src/ChallengeDifficulty.h"` | `#include <functional>` | **保留 test2 的**（副本难度挑战模块依赖），同时加上 `<functional>` |
| line 280 | test2 在 AI 框架中添加了挑战难度判定逻辑 | 上游修改了 AI 状态机 | **保留 test2 的挑战难度逻辑**，适配上游状态机 |
| line 425 | test2 修改了 `OnCharmed`/`MoveInLineOfSight` 等 | 上游重构了 AI 事件系统 | **保留 test2**，注意上游重构的 API |

---

### 3. `src/server/game/Instances/InstanceScript.h` — 3 冲突块

| 冲突位置 | test2 改动 | 上游改动 | 解决建议 |
|----------|-----------|---------|---------|
| line ~183 (`OnPlayerEnter`) | test2 把 `OnPlayerLeave` 改成了带实现 `{}` 的非虚函数 | 上游把两者都改成 `virtual` 纯虚声明，无默认实现 | **采用上游**（虚函数），因为 test2 的实现内容在 `.cpp` 中已有 |
| line ~1150 | test2 添加了 `IsTwoFactionInstance()` 等方法 | 上游重构了副本实例 API | **保留 test2 的新方法**，融入上游重构 |
| line ~1450 | test2 添加了挑战难度副本相关声明 | 上游新增了 `OnBattlefieldWarEnd` 钩子 | **保留双方** |

---

### 4. `src/server/game/Instances/InstanceScript.cpp` — 1 冲突块

| 冲突位置 | test2 改动 | 上游改动 | 解决建议 |
|----------|-----------|---------|---------|
| line ~973 (`IsTwoFactionInstance`) | test2 添加了双阵营副本判断（Shattered Halls, Nexus, ICC 等） | 上游将此逻辑移到其他地方或简化 | **保留 test2 的**（bot 战场/副本需要） |

---

### 5. `src/server/game/Entities/Player/Player.h` — 2 冲突块

| 冲突位置 | test2 改动 | 上游改动 | 解决建议 |
|----------|-----------|---------|---------|
| `IsNPlayer()` 声明 | test2 添加了 NPCBot 伪玩家判断（bot 被视为玩家） | 上游没有此概念 | **保留 test2** |
| `CanSeeSpellClickOn` | test2 修改了点击施法权限 | 上游修复了权限检查 bug | **合并**：test2 的 bot 判断 + 上游的修复 |

---

### 6. `src/server/game/Entities/Creature/CreatureData.h` — 1 冲突块

test2 添加了 bot 相关的生物数据结构扩展。

**解决**：保留 test2 的，确保不与上游新字段冲突。

---

### 7. `src/server/game/Cache/WhoListCacheMgr.cpp` — 1 冲突块

test2 实现了在线假玩家 WhoList 功能（`WhoListOnlineBot=1`）。

**解决**：保留 test2，注意上游是否修改了 `Update()` 方法结构。

---

### 8. `src/server/game/Entities/Unit/StatSystem.cpp` — 1 冲突块

test2 修正了乘数计算逻辑（`fix(stats): 修正乘数计算逻辑`）。

**解决**：保留 test2。

---

### 9. `src/server/game/Spells/Spell.cpp` — 1 冲突块

test2 在 `CastSpell` 中 `TryGetSpellInfoOverride` 后添加了空指针检查。

**解决**：保留 test2 的空指针检查，看上游是否已有类似修复。

---

### 10. `src/server/game/Movement/MotionMaster.cpp` — 1 冲突块

test2 添加了机器人从受限地图移除前的载具/交通工具清理。

**解决**：保留 test2。

---

## 二、NPCBots 模块 (14 文件, 58 冲突块) 🔴 最密集

### 1. `bot_ai.cpp` — 23 冲突块（最多）

**核心冲突**：上游进行了大规模重构——

- **API 重命名**：`BotMgr::GetOwnershipExpireTime()` → `BotCfg::GetOwnershipExpireTime()`
- **威胁系统**：从 TC 移植了 bot 威胁计算
- **装备生成**：新增副本机器人专用装备生成逻辑
- **中断施法队列**：实现了 bot 施法中断排队系统
- **解雇/重连**：修改了解雇逻辑，副本机器人留队

test2 的定制改动：
- bot 战斗行为定制（目标选择、技能优先级）
- 特定职业伤害修正（SS DoT 加强、法师修正等）
- 绑定物品/装备检查
- `IsNPlayer()` 判断逻辑

**解决**：这是最复杂的文件。建议：
1. 全局替换 `BotMgr::` → `BotCfg::`（上游重命名）
2. 逐个冲突块检查，test2 的功能性改动保留，上游的重构采纳
3. 特别注意上游的威胁系统和中断队列——test2 的定制可能需要适配这些新系统

---

### 2. `botcommands.cpp` — 9 冲突块

| 冲突位置 | test2 改动 | 上游改动 | 解决建议 |
|----------|-----------|---------|---------|
| 查看装备命令 | test2 新增了 `.npcbot equip` 查看装备命令 | 无 | **保留 test2** |
| 消息格式 | test2 将英文消息改成了中文 | 上游是英文 | **选择中文或英文，保持一致**（test2 已有中文环境） |
| 指挥 bot 施法 (star 标记) | test2 增加了通过 raid 标记来指挥 bot | 上游修改了 bot 指令系统 | **保留 test2 的标记指令** |
| 跟随距离显示 | test2 修改了消息内容 | 上游无变化 | **保留 test2** |
| WP 生成器 | 双方都有修改 | 双方都有修改 | **逐一比对** |

---

### 3. `botmgr.cpp` — 8 冲突块

| 冲突位置 | test2 改动 | 上游改动 | 解决建议 |
|----------|-----------|---------|---------|
| line 49 配置区 | test2 添加了同IP最大 bot 数量配置 | 上游修改了配置结构 | **保留 test2 的配置项** |
| line 275 bot 登录 | test2 修改了 bot 登录/下线逻辑 | 上游改了解雇/重连 | **保留 test2 的登录逻辑** |
| line 648 副本bot管理 | test2 的副本 bot 管理 | 上游实现了 dungeon bot 留队机制 | **采纳上游**（更完善的实现） |
| line 1093 解雇清理 | test2 的清理逻辑 | 上游修改了解雇流程 | **合并**：上游的新解雇 + test2 的额外清理 |
| line 1690 队伍管理 | test2 的组队逻辑 | 上游修改了队伍/副本处理 | **逐一比对** |

**特别注意**：上游新增了 `DungeonBots.MaxItemLevel.Ratio` 配置和副本 bot 专用装备生成——这些是 test2 没有的，应该完整保留。

---

### 4-14. 其他 NPCBots 文件 (各 1-3 冲突块)

| 文件 | test2 改动 | 上游改动 | 解决建议 |
|------|-----------|---------|---------|
| `bot_ai.h` | 少量声明修改 | API 重命名 | **使用上游 API 名称，保留 test2 声明** |
| `bot_death_knight_ai.cpp` | DK 技能触发调整 | DK AI 优化 | **保留 test2 的调整** |
| `bot_druid_ai.cpp` | 德鲁伊专精修复 | 专精/装备生成修复 | **合并** |
| `bot_hunter_ai.cpp` | 猎人宠物修复 | 猎人 AI 优化 | **保留 test2 的宠物修复** |
| `bot_mage_ai.cpp` | 法师伤害修正 | 法师 AI 优化 | **保留 test2 的伤害修正** |
| `bot_warlock_ai.cpp` | 术士 DoT 强化 | 术士 AI 优化 | **保留 test2** |
| `botdatamgr.h` | 数据结构扩展 | 数据结构扩展 | **合并双方新字段** |
| `botgiver.cpp` | 雇佣金额定制 | 装备生成器重写 | **保留 test2 的金额逻辑** |
| `botgossip.h` | 对话选项修改 | 上游修改了 gossip 菜单 | **合并** |
| `botmgr.h` | 声明修改 | 上游重写了管理器接口 | **采纳上游接口，移植 test2 新增方法** |
| `bpet_ai.cpp` | 修复 `m_creator` 空指针 | 上游也修复了同类问题 | **检查是否重复修复，选择更完善的** |

---

## 三、副本/Boss 脚本 (48 文件, 66 冲突块) 🟡 中等

### 冲突模式分类

所有副本脚本的冲突可归纳为以下几种模式：

#### 模式 A：`IsNPlayer()` vs `IsPlayer()` — 最常见

test2 在所有需要将 bot 视为玩家的地方使用了 `IsNPlayer()`：
```cpp
// test2 (HEAD - 要保留的)
if (!target->IsNPlayer())
// 或
if (_playerOnly && !target->IsNPlayer())

// 上游 (npcbots_3.3.5 基分支)
if (!target->IsPlayer())
```

**解决**：**全部保留 test2 的 `IsNPlayer()`**。`IsNPlayer()` 是 test2 扩展的判定方法，能让 NPCBot 被当作玩家处理（如 boss 技能目标选择、副本机制触发等）。

涉及文件（模式 A）：
- ICC: `boss_the_lich_king.cpp`, `boss_professor_putricide.cpp`, `boss_sindragosa.cpp`
- 冰冠 5 人本: `boss_devourer_of_souls.cpp`, `boss_falric.cpp`, `boss_scourgelord_tyrannus.cpp`, `halls_of_reflection.cpp`
- 乌特加德: `boss_ingvar_the_plunderer.cpp`, `boss_keleseth.cpp`, `boss_skadi.cpp`, `boss_ymiron.cpp`
- 闪电/岩石: `boss_bjarngrim.cpp`, `boss_volkhan.cpp`, `boss_loken.cpp`, `instance_halls_of_lightning.cpp`, `instance_halls_of_stone.cpp`
- 纳克萨玛斯: `instance_naxxramas.cpp`
- 奥杜尔: `boss_freya.cpp`, `instance_ulduar.cpp`
- 艾卓: `boss_hadronox.cpp`, `instance_azjol_nerub.cpp`
- 达克萨隆: `boss_novos.cpp`, `instance_drak_tharon_keep.cpp`
- 紫罗兰: `instance_violet_hold.cpp`
- 古达克: `boss_drakkari_colossus.cpp`
- 魔枢: `boss_commander_stoutbeard_kolurg.cpp`, `instance_nexus.cpp`
- 黑翼: `boss_razorgore.cpp`, `instance_blackwing_lair.cpp`
- 黑石: `boss_high_interrogator_gerstahn.cpp`, `instance_blackrock_depths.cpp`
- 太阳井: `boss_kalecgos.cpp`
- 毒蛇神殿: `boss_fathomlord_karathress.cpp`
- 试炼: `instance_trial_of_the_crusader.cpp`
- 红玉: `boss_halion.cpp`
- 冬拥湖: `zone_wintergrasp.cpp`

#### 模式 B：bot 特殊处理逻辑

test2 在 boss 技能中为 bot 添加了特殊处理：
- 辛达苟萨冰墓 (line 717-753): 需要处理 bot 被冰墓困住的逻辑
- LK 12% 血量存活玩家检查 (line 2346-3527): 需要将 bot 视为存活玩家
- 海里昂假死光环检查
- 卡拉赞象棋 bot 处理

**解决**：**保留 test2 的 bot 特殊处理**，这些是 test2 弥补 NPCBots 模块在副本中的兼容性修复。

#### 模式 C：上游代码重构

上游对很多副本脚本进行了重构（修复 bug、优化逻辑），test2 也在相同位置有修改。

**解决**：如果冲突位置是 test2 的 bot 兼容修复，保留 test2；如果冲突位置是上游的 bug 修复且与 bot 无关，保留上游。

---

### 按区域汇总

| 区域 | 文件数 | 冲突块 | 典型文件 |
|------|--------|--------|----------|
| ICC 冰冠堡垒 | 4 | 7 | LK, 教授, 辛达, 实例 |
| 乌特加德(城堡+尖顶) | 5 | 9 | Ingvar, Keleseth, Skadi, Ymiron |
| 闪电大厅+岩石大厅 | 5 | 7 | Bjarngrim, Loken, Volkhan |
| 冰冠 5 人本 (3个) | 6 | 6 | 灵魂洪炉/映像/矿坑 |
| 祖阿曼 | 5 | 17 | 4 boss + 实例 |
| 黑翼+黑石 | 4 | 4 | Razorgore, Gerstahn |
| 奥杜尔 | 2 | 2 | Freya, Ulduar |
| 其他诺森德 | 7 | 9 | 艾卓, 达克萨隆, 紫罗兰, 古达克, 魔枢, 纳克萨玛斯 |
| 外域+东部王国 | 5 | 8 | 太阳井, 毒蛇神殿, 红玉, 冬拥湖 |
| 杂项 | 5 | - | spell_item, 试炼 |

---

## 四、杂项 (spell_item.cpp, etc.)

### `src/server/scripts/Spells/spell_item.cpp` — 4 冲突块

test2 修改了以下物品法术使 NPCBot 可用：
- 死神意志 (`Death's Choice/Verdict` 饰品)
- 哈哈镜 (`Nibelung` 法杖)

上游修复了多个物品法术的 bug。

**解决**：保留 test2 的 bot 可用性修改，同时采纳上游的 bug 修复。

---

## 五、合并操作步骤建议

### 推荐顺序（从易到难）

1. **副本脚本 (48 文件)** — 先解决最简单的
   - 所有 `IsPlayer()` → `IsNPlayer()` 直接保留 test2
   - bot 特殊逻辑直接保留 test2
   - 上游重构冲突逐个检查

2. **杂项文件** — spell_item.cpp 等

3. **NPCBots 模块 (14 文件)** — 中等难度
   - 先全局替换 API 重命名
   - 再逐个解决功能冲突

4. **核心引擎 (10 文件)** — 最后解决
   - `Unit.cpp` 放最最后（92 个冲突块）
   - 需要逐行对比，确保编译通过

### 每个文件解决后用 `git add <file>` 标记已解决

```bash
# 在 merge_test2_from_npcbots 分支上
# 查看剩余冲突
git diff --name-only --diff-filter=U

# 解决后标记
git add src/server/scripts/Northrend/IcecrownCitadel/boss_the_lich_king.cpp

# 全部解决后提交
git commit -m "merge: test2 custom features onto npcbots_3.3.5"
```

---

## 六、编译验证

解决完所有冲突后，建议先在本地编译验证：

```bash
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=./install
make -j$(nproc)
```

重点关注：
- NPCBots 模块编译是否通过
- 副本脚本编译是否通过
- Unit.cpp 编译是否通过
