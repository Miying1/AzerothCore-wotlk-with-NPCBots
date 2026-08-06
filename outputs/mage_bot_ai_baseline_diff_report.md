# 法师 BOT 战斗逻辑与基线差异分析

## 1. 对比范围

- 当前分支：`merge_test2_from_npcbots`
- 当前提交：`024831590`（`HEAD`）
- 权威基线：`upstream/npcbots_3.3.5`，当前提交为 `d914b8288`
- 主要文件：`src/server/game/AI/NpcBots/bot_mage_ai.cpp`
- 关联文件：`bot_ai.cpp`、`bot_ai.h`、`botspell.h`

当前工作树还有一处未提交的 `bot_ai.cpp::CheckOwnerExpiry()` 改动。该改动与法师战斗逻辑无关，以下报告只比较 **HEAD 与 upstream 基线**，不把工作树临时改动混入结论。

## 2. 总体差异结论

当前分支不是简单同步基线，而是在法师 AI 上保留了多项 test2 定制：

- 收窄部分跨专精技能的自动使用范围；
- 提高冰霜新星的群体触发门槛；
- 关闭常规冰枪爆发；
- 重写奥术飞弹/奥术冲击资源轮转；
- 更早使用唤醒；
- 增加奥术和火焰专精的额外被动；
- 调整水元素战斗接触距离；
- 增加宠物生命周期安全保护。

总体风格从基线的“跨专精自适应、兼顾低等级和免疫目标”变为：

> 更强调专精隔离和高等级数值强化，但牺牲部分低等级兼容性、元素免疫兜底、冰霜瞬发爆发和小规模控场能力。

## 3. 输出轮转差异

### 3.1 冰霜新星：群体门槛从 2 个提高到 3 个

当前 `bot_mage_ai.cpp::457-469`：

```cpp
if (IsSpellReady(FROST_NOVA_1, diff) &&
    (targets.size() > 2 || oneOnOne))
```

基线约 `440-452`：

```cpp
if (IsSpellReady(FROST_NOVA_1, diff) &&
    (targets.size() > 1 || oneOnOne))
```

#### 行为影响

- 基线在附近有 2 个以上目标时可以使用冰霜新星；
- 当前分支通常要求至少 3 个目标；
- 两目标战斗中，法师更少使用冰霜新星来定身和脱离近战；
- `oneOnOne` 本身仍只是判断列表首元素是否是当前目标，并不是真正的单目标判断，因此双目标时仍可能受容器顺序影响。

**判断：明确语义变化，属于 test2 的保守控场定制。**

### 3.2 冰锥术：从所有专精可用改为仅冰霜专精

当前约 `490-505`：

```cpp
if (is_frost && IsSpellReady(CONE_OF_COLD_1, diff))
```

基线约 `473-489`：

```cpp
if (IsSpellReady(CONE_OF_COLD_1, diff))
```

#### 行为影响

`CONE_OF_COLD_1` 在基础法术初始化中仍是各专精都可获得的法术，但当前只有冰霜专精会自动施放。因此：

- 奥术、火焰法师失去近距离瞬发伤害和减速手段；
- 火法和奥法被近战贴身时的自动脱身能力下降；
- 火法仍可使用龙息术，但需要满足火法专属初始化条件。

**判断：明确语义变化。**

### 3.3 常规冰枪术：基线启用，当前完全注释

当前约 `579-586` 的冰枪代码被整体注释；基线约 `562-569` 正常执行：

```cpp
if (fbCasted &&
    (!me->GetMap()->IsDungeon() || mytar->IsControlledByPlayer()) &&
    IsSpellReady(ICE_LANCE_1, diff) &&
    can_do_frost &&
    dist < CalcSpellMaxRange(ICE_LANCE_1) &&
    (mytar->isFrozen() ||
     me->HasAuraType(SPELL_AURA_ABILITY_IGNORE_AURASTATE)))
```

#### 行为影响

当前仍保留用于消耗法术反射的

分支，但不再在冻结目标、寒冰指或类似瞬发窗口中使用冰枪：

- 冰霜法师冻结后的瞬发爆发显著降低；
- 移动中输出能力下降；
- `FINGERS_OF_FROST` 和 `GLYPH_ICE_LANCE` 仍存在，但常规轮转不再消费其收益；
- 冰霜风格从“冻结后冰枪爆发”转向持续施法。

**判断：重大语义变化，建议优先复核。**

### 3.4 霜火箭/火球瞬发条件被替换

当前约 `587-591`：

```cpp
if (me->HasAuraType(SPELL_AURA_ABILITY_IGNORE_AURASTATE) &&
    IsSpellReady(FROSTFIREBOLT, diff) &&
    (can_do_frost | can_do_fire) &&
    dist < CalcSpellMaxRange(FROSTFIREBOLT) &&
    Rand() < 150)
```

基线约 `570-576`：

```cpp
if (IsSpellReady(FROSTFIREBOLT, diff) &&
    can_do_frost_or_fire &&
    dist < CalcSpellMaxRange(FROSTFIREBOLT) &&
    Rand() < 150 &&
    ((((CCed(mytar, true) || b_attackers.empty()) &&
       me->HasAura(COMBUSTION_BUFF)) ||
      me->HasAura(BRAIN_FREEZE_BUFF)) ||
     !GetSpell(FROSTBOLT_1)))
```

#### 基线原本覆盖的场景

1. 有燃烧层数且适合消耗时使用；
2. 有 `BRAIN_FREEZE_BUFF` 时使用瞬发霜火箭/火球；
3. 低等级尚未学会寒冰箭时提供兜底输出。

当前统一换成 `SPELL_AURA_ABILITY_IGNORE_AURASTATE` 后：

- 丢失对 `BRAIN_FREEZE_BUFF` 的显式判断；
- 丢失燃烧层数和站桩环境判断；
- 丢失低等级寒冰箭缺失时的兜底条件；
- 可能把其他“忽略 AuraState”状态误当作霜火箭瞬发条件。

此外，`(can_do_frost | can_do_fire)` 对两个 `bool` 的结果通常和 `||` 相同，但没有短路语义，主要是写法问题，不是独立行为变化。

**判断：明确语义变化，疑似错误替换或不完整移植。**

### 3.5 奥术轮转被重写

当前约 `593-620` 的逻辑：

```text
奥术强化且法力 > 30%：奥术冲击
法力 < 15% 且奥冲 >= 2 层：奥术飞弹
法力 < 30% 且奥冲 >= 3 层：奥术飞弹
奥冲 >= 4 层且有飞弹速射：奥术飞弹
否则继续奥术冲击
```

基线约 `578-592` 则主要根据：

- 等级是否低于 45；
- 是否拥有奥术冲击；
- 奥冲层数是否达到 3 层；
- 下一发奥冲实际耗蓝是否超过当前法力；
- 是否存在相关天赋效果。

#### 行为影响

当前分支：

- 由固定法力百分比代替实际法力成本判断；
- 更倾向持续堆叠奥术冲击；
- 通常要到 4 层才在飞弹速射时泄层；
- 低蓝时即使没有飞弹速射也可能使用奥术飞弹；
- 不再显式处理低等级奥术法师的专门轮转。

#### 高可信度回归：低等级奥术法师可能只剩魔杖

当前奥术飞弹依赖 `arcaneBlastStack >= 2/3/4`。如果尚未学会奥术冲击，层数无法建立；后续火球和寒冰箭又分别被火焰/冰霜专精条件限制，奥术专精可能无法命中正常填充分支，最终落到魔杖。

基线中的：

```cpp
me->GetLevel() < 45
```

正是低等级奥术飞弹兜底逻辑。当前改写覆盖了这一兼容性处理。

**判断：重大语义变化，低等级部分疑似回归。**

### 3.6 火焰法师失去火免目标的寒冰箭兜底

当前约 `636-639`：

```cpp
if (is_frost && IsSpellReady(FROSTBOLT_1, diff) && can_do_frost)
```

基线约 `604-607`：

```cpp
if (IsSpellReady(FROSTBOLT_1, diff) &&
    can_do_frost &&
    (GetSpec() != BOT_SPEC_MAGE_FIRE || !can_do_fire))
```

#### 行为影响

基线允许火焰专精在目标免疫火焰、但不免疫冰霜时改用寒冰箭。当前把寒冰箭限制给冰霜专精后：

- 火法面对火焰免疫目标时不再自动切换寒冰箭；
- 尚未学会霜火箭的等级区间尤其容易退化为魔杖；
- 覆盖了基线已有的元素免疫适配修复。

**判断：明确回归风险，建议恢复基线兜底语义。**

## 4. 资源和生存差异

### 4.1 唤醒阈值从 15% 提高到 32%

当前约 `689-695`：

```cpp
GetManaPCT(me) < 32
```

基线约 `657-663`：

```cpp
GetManaPCT(me) < 15
```

#### 行为影响

当前分支更早进入唤醒：

- 更不容易完全空蓝；
- 但更早中断输出并消耗唤醒冷却；
- 由于唤醒检查在法力宝石和法力药水之前，恢复策略更偏向引导唤醒。

**判断：明确资源策略变化，整体更保守。**

### 4.2 火法获得额外“玩火自焚”被动

当前约 `1771`：

```cpp
RefreshAura(WANHUOZIFENG, isFire && level >= 60 ? 1 : 0);
```

基线没有该 Aura。

#### 行为影响

火法获得额外输出收益，但通常伴随更高承伤，形成更明显的玻璃大炮风格。

**判断：明确数值变化。**

### 4.3 其他生存逻辑基本未改变

以下与基线没有发现实质战斗差异：

- 寒冰屏障；
- 寒冰护体；
- 闪现脱困；
- 生命药水阈值；
- 解诅咒；
- 焦点魔法；
- 奥术智慧。

## 5. 被动和职业数值差异

当前新增或额外启用的法师被动包括：

```cpp
ARCANE_JIZHONG   = 12840;
XINGLINGXUEZHE   = 44399;
AOSHUXINZHI      = 12503;
XINLINGZHANGWO   = 31588;
LINGFENGFUMIAN   = 44403;
WANHUOZIFENG     = 31640;
```

对应应用：

```cpp
RefreshAura(ARCANE_JIZHONG, isArca && level >= 45 ? 1 : 0);
RefreshAura(XINGLINGXUEZHE, isArca && level >= 45 ? 1 : 0);
RefreshAura(AOSHUXINZHI, isArca && level >= 75 ? 1 : 0);
RefreshAura(XINLINGZHANGWO, isArca && level >= 75 ? 1 : 0);
RefreshAura(LINGFENGFUMIAN, isArca && level >= 75 ? 1 : 0);
RefreshAura(WANHUOZIFENG, isFire && level >= 60 ? 1 : 0);
```

### 整体影响

- 45 级以上奥法开始获得额外奥术相关被动；
- 75 级以上奥法获得额外智力、法强转换和急速收益；
- 60 级以上火法获得玩火自焚效果；
- 当前分支的高等级奥法纸面属性明显高于基线；
- 火法输出增强的同时，生存压力可能增加。

## 6. 水元素和共享基类差异

### 6.1 水元素战斗接触距离增大

当前约 `1593-1604` 增加：

```cpp
myPet->SetFloatValue(
    UNIT_FIELD_COMBATREACH,
    2.0f * DEFAULT_COMBAT_REACH * me->GetObjectScale());
```

基线没有该设置。

#### 行为影响

改变水元素的接触、距离和部分移动/碰撞判定。它更像路径和卡位修正，不是直接伤害加成。由于使用法师本体 `GetObjectScale()`，若宠物缩放不同，可能存在边界副作用。

### 6.2 宠物生命周期安全保护

共享 `bot_ai.cpp` 增加了若干保护：

- 更新宠物属性前检查 `GetBotPetAI()`；
- 对不在世界中的宠物不执行区域光环施法；
- 更新团队光环时先获取并检查宠物 AI。

#### 行为影响

正常宠物战斗基本不变，但在传送、移除、重建或 AI 尚未初始化时更不容易出现非法调用或崩溃。

**判断：稳定性修复，不属于正常轮转变化。**

## 7. 非战斗逻辑但会影响法师实战能力的共享差异

### 7.1 70330 法师 Bot 的武器槽锁定

共享 `bot_ai.cpp` 中对 entry `70330` 增加限制，导致该特定 Bot 不能更换主手、副手和远程武器。普通法师 Bot 不受影响，但该 Bot 的法术强度、命中、急速等武器属性会被固定。

### 7.2 可使用物品范围收窄

共享物品筛选逻辑额外要求：

```cpp
proto->Material == 3;
proto->SubClass == 3;
proto->ItemLevel >= 80;
```

这会减少法师可自动使用的药剂、保命物品和 on-use 战斗物品。`CheckUsableItems()` 本身未改变，但它可获得的候选物品明显减少。

## 8. 纯重构或不影响战斗的差异

以下差异不应算作战斗行为变化：

- 法术枚举和被动枚举的分组、排版及注释变化；
- `manaPct` 局部缓存；
- `is_fire`、`is_frost` 局部变量；
- 被注释代码的保留或格式变化；
- `bot_ai.h` 中装备查看、改名、管理接口的新增声明；
- `botspell.h` 在当前基线范围内无差异。

## 9. 基线已有、当前未新增的问题

以下问题在当前分支和基线中都存在，不应归因于 test2 合并：

### 9.1 52 级思维冷却等级空档

```cpp
RefreshAura(BRAIN_FREEZE3, isFros && level >= 53 ? 1 : 0);
RefreshAura(BRAIN_FREEZE2, isFros && level >= 51 && level < 52 ? 1 : 0);
RefreshAura(BRAIN_FREEZE1, isFros && level >= 50 && level < 51 ? 1 : 0);
```

52 级不匹配任何 Rank 条件，疑似边界错误。

### 9.2 PvP 减速和法力护盾仍未实现

文件头和 `CheckShield()` 中均保留 TODO，当前与基线一致。

### 9.3 `Rand() < 150` 通常不是有效概率门槛

当前与基线均存在，普通队伍中基本恒真，不是本次分支新增。

## 10. 优先级建议

如果只修复会直接影响法师战斗正确性的差异，建议按以下顺序复核：

1. **恢复低等级奥术法师的可用填充逻辑**，避免未学奥冲时只剩魔杖；
2. **恢复火法面对火焰免疫目标时的寒冰箭兜底**；
3. **重新核对 `BRAIN_FREEZE_BUFF` 与 `SPELL_AURA_ABILITY_IGNORE_AURASTATE` 的替换是否有意**；
4. **确认是否确实要关闭常规冰枪轮转**；
5. **确认冰霜新星从双目标提高到三目标是否符合设计**；
6. **确认奥术飞弹的法力阈值和 4 层泄层条件**；
7. 再评估额外奥术/火焰被动是否属于 test2 预期增强；
8. 最后处理水元素接触距离和共享物品筛选等外围影响。

## 11. 最终判断

当前分支相对 `upstream/npcbots_3.3.5` 的法师变化，核心不是普通同步，而是一次带有明确设计取向的 test2 定制：

- **增强**：高等级奥法被动、火法额外被动、较早回蓝、水元素接触距离、宠物异常状态安全性；
- **削弱/收窄**：非冰法冰锥、双目标冰霜新星、冰霜常规冰枪、跨元素免疫适配；
- **高风险回归**：低等级奥法填充、火免目标寒冰箭兜底、思维冷却 proc 条件替换。

若目标是保留 test2 的职业特色，建议只保留高等级被动和水元素安全修复，并重新审查上述三个高风险轮转差异；如果目标是尽量贴近权威基线，则应优先恢复奥术低等级兜底、火法寒冰箭免疫兜底和基线冰枪/proc 逻辑。
