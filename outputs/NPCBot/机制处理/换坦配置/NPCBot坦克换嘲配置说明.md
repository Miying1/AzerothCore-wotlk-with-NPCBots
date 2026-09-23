# NPCBot 坦克换嘲数据库配置说明

> 本文档说明 `npcbot_tank_swap` 表的字段含义与 SQL 编写规范，并沉淀**如何判断一个技能是否该配置换嘲、换嘲层数如何取值**的方法论（第 6~8 节）。具体各团本配置见同级 `NPCBot坦克换嘲配置-合并.sql` 及各副本分文件。

## 1. 表结构

World 数据库专用表 `npcbot_tank_swap`：

```sql
CREATE TABLE IF NOT EXISTS `npcbot_tank_swap` (
  `map_id` SMALLINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '地图ID，0表示所有地图',
  `boss_entry` INT UNSIGNED NOT NULL COMMENT 'BOSS生物Entry',
  `spell_id` INT UNSIGNED NOT NULL COMMENT 'BOSS施加的换嘲Aura或技能ID',
  `aura_stacks` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '触发换嘲所需BUFF层数',
  `enabled` TINYINT UNSIGNED NOT NULL DEFAULT 1 COMMENT '是否启用',
  `comment` VARCHAR(255) NOT NULL DEFAULT '' COMMENT '配置说明',
  PRIMARY KEY (`map_id`, `boss_entry`, `spell_id`),
  KEY `idx_boss_entry` (`boss_entry`),
  KEY `idx_spell_id` (`spell_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='NPCBot坦克换嘲配置';
```

## 2. 字段说明

| 字段 | 类型 | 说明 |
|---|---|---|
| `map_id` | SMALLINT UNSIGNED | 地图 ID；`0` 表示所有地图通用 |
| `boss_entry` | INT UNSIGNED | BOSS 的 Creature Entry，不使用实例 GUID |
| `spell_id` | INT UNSIGNED | BOSS 施加的换嘲 Aura（Debuff）ID |
| `aura_stacks` | TINYINT UNSIGNED | 触发换嘲所需的最小 BUFF 层数 |
| `enabled` | TINYINT UNSIGNED | `1` 启用、`0` 禁用（加载时跳过） |
| `comment` | VARCHAR(255) | 中文配置说明，不参与运行时判断 |

> `boss_entry` 使用 Creature Entry 而非实例 GUID：实例中的 BOSS GUID 每次可能不同，Entry 才适合通用配置。

## 3. 主键约束

使用：

```text
map_id + boss_entry + spell_id
```

作为主键，允许同一 BOSS 配置多个换嘲 Aura（例如不同阶段的叠层技能各自触发换嘲）。

## 4. 配置示例

### 4.1 坦克两层换嘲

当 BOSS 当前攻击目标身上的换嘲 Aura `21414` 达到 2 层时，由正在攻击该 BOSS 的坦克 BOT 嘲讽 BOSS：

```sql
DELETE FROM `npcbot_tank_swap`
WHERE `map_id` = 531
  AND `boss_entry` = 41413
  AND `spell_id` = 21414;

INSERT INTO `npcbot_tank_swap`
    (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`)
VALUES
    (531, 41413, 21414, 2, 1, '坦克两层换嘲');
```

### 4.2 全局通用配置

`map_id = 0` 表示所有地图通用（地图专用配置优先于全局配置）：

```sql
INSERT INTO `npcbot_tank_swap`
    (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`)
VALUES
    (0, 41413, 21414, 3, 1, '全地图通用：三层换嘲');
```

### 4.3 禁用配置

`enabled = 0` 的配置在加载时被跳过，不参与运行时处理：

```sql
INSERT INTO `npcbot_tank_swap`
    (`map_id`, `boss_entry`, `spell_id`, `aura_stacks`, `enabled`, `comment`)
VALUES
    (531, 41413, 21414, 2, 0, '临时禁用');
```

## 5. 加载校验

加载阶段对每行配置做如下校验，非法配置被跳过并记录日志：

| 校验项 | 处理 |
|---|---|
| `enabled = 0` | 跳过，不参与处理 |
| `map_id` 非 0 但地图不存在 | 跳过并记录日志 |
| `boss_entry` 不存在 | 跳过并记录日志 |
| `spell_id` 不存在 | 跳过并记录日志 |
| `aura_stacks = 0` | 跳过并记录日志 |

查询优先级：**地图专用配置优先，回退到 `map_id = 0` 的全局配置**。

---

## 6. 换嘲配置的核心规则（最重要）

### 6.1 `spell_id` 必须填「施加在坦克身上的 Debuff 光环 ID」

- **不能填「施法触发技能」**。一个技能常分两层：
  - **施法触发技**（BOSS 读条/挥击的那个 ID，`EffectTriggerSpell` 指向下游）；
  - **真正的 Debuff 光环**（挂在坦克身上的那个 ID，才是 `aura_stacks` 要数的层数）。
- 判据：`EffectAura=23`（周期性触发）是「施法触发/周期触发」，不是 debuff 本身，需沿 `EffectTriggerSpell` 链追踪到真正的光环。
- 例：艾尔加隆警戒冲击 64412/64389/64678 是施法触发技，实际 debuff 是 **64392(10 人)/64679(25 人)**；阿努巴拉克的 65882 是纯视觉特效（Aura=23），真换嘲点是 66012。

### 6.2 只有「对坦克施放」的技能才是换嘲 debuff

脚本中必须是 `DoCastVictim(...)` 或 `CastSpell(GetVictim(), ...)`、`CastSpell(target=当前坦, ...)` 才构成换嘲。以下一律**不是**换嘲，不写配置：

| 错误类型 | 特征 | 实例（均已从配置移除） |
|---|---|---|
| 随机点名远程 | `SelectTarget(Random)` 或按距离分组挑远程 | 威札斯无面者印记 63276、墨吉姆静电瓦解 61912 |
| 对自身施放 | `CastSpell(me, ...)` | 墨吉姆符文护盾 62274、闪电爆裂 62054 |
| 视觉/触发特效 | `Aura=23` 且无实际 debuff 语义 | 阿努巴拉克永冻视觉 65882 |
| 攻击者自身的反馈 DOT | 攻击 BOSS 者自身叠 DOT（非 BOSS 对坦挂） | 辛德拉苟莎寒霜刺骨 70106 |
| 技能 ID 归属错误 | 张冠李戴到别的 BOSS | 「静电瓦解 61912」实为破钢者技能，非墨吉姆 |

### 6.3 换嘲层数必须满足「持续时间约束」

**核心公式**：叠层间隔 `I` × 层数 `N` ≤ 持续时长 `D`，即旧坦身上的 debuff 须在下一次轮到它接怪前自然消失，否则层数越积越多、最终爆炸。

分三类取值：

1. **真叠层换嘲（`D ≥ I×N`）**：层数按官方标准取，让 debuff 恰好轮转消失。
   - 脓肠胃胀气 72219：`D=100s、I≈10~12s、10 层爆炸` → **9 层**（官方标准，提前换嘲会 100s debuff 残留叠到爆炸）。
   - 教授畸变瘟疫 72451：`I=10s、全团 DOT 每层 ×3、死亡回血` → **2 层**（官方标准）。
   - 柯洛刚恩压碎护甲 64002：25 人可叠、`D=45s` → **2 层**（官方标准）。
   - 戈莫克穿刺 66331：`I=9~10s、D=30s` → **3 层**（`9~10×3≈30s` 恰好消失）。
   - 莫克札破甲攻击 30901：`I=5~10s、D=20s、可叠 5 层` → **3 层**。

2. **刷新式 debuff（`D ≪ I`，永远叠不到 2 层）**：`D` 远小于施放间隔，每次刷新、层数不会累积。设 2 层会导致换嘲**永不触发**，只能设 **1 层（命中即换）** 或不配。
   - 若确需配置（如轮流分散易伤承伤），1 层即可；若无换嘲价值则删除。
   - 例：纳罗拉克裂伤 42389（脚本 `HasAura` 判断防重、刷新式）、纳罗拉克撕裂 42397（`D=5s < I=6~21s`）。

3. **单次强标记/硬控/爆发 DOT（不可叠、命中即换）**：Aura 码为昏迷(12)、标记(42)、增伤(79/87/138)、周期爆发伤害(3) 等**单次** debuff，`aura_stacks=1`（命中即换）。例：巫妖王灵魂收割 69409（Aura=3 周期伤害，非 69410——69410 是巫妖王自身急速 buff）、阿努巴拉克寒冰打击 66012、萨鲁法尔符文之血 72410、辛德拉苟莎冰霜吐息 69649。

---

## 7. 数据源与验证流程

### 7.1 权威数据源

- **Spell 数据**：`G:\wow\dbc_csv\Spell.csv`（WLK 3.3.5 Spell.dbc 导出）。
  - ⚠️ 该导出**列错位**：中文技能名在 **index 138**、中文描述 index 172、光环描述 index 189（不是表头标注的列号）。
  - 关键列：`DurationIndex`=38、`EffectAura_1/2/3`=93/94/95、`EffectAuraPeriod`=96-98、`EffectTriggerSpell`=114-116、`Effect`（枚举）=69-71。
- **持续时间**：`G:\wow\dbc_csv\SpellDuration.csv`，列为 `(ID, Duration, DurationPerLevel, MaxDuration)`，`Duration` 单位**毫秒**。由 `Spell.DurationIndex` 关联。

### 7.2 SpellDuration 常用换算表（勿凭记忆）

| DurationIndex | 秒 |
|---|---|
| 1 | 10s |
| 3 | 60s |
| 8 | 15s |
| 9 | 30s |
| 18 | 20s |
| 22 | 45s |
| 25 | 180s |
| 27 | 3s |
| 28 | 5s |
| 32 | 6s |
| 35 | 4s |
| 64 | 40s |
| 572 | 100s |

> 教训：曾凭记忆把 `25` 误当 4s、`28` 误当 10s、`572` 误当未知，导致多处 debuff 持续时间与层数判断错误。**一律以 SpellDuration.csv 为准**。

### 7.3 EffectAura 代码速查（3.3.5）

| Aura 码 | 含义 | 是否可当换嘲 debuff |
|---|---|---|
| 3 | 周期伤害(DOT) | ✅ 若对坦施放 |
| 12 | 昏迷/冻结 | ✅ 命中即换 |
| 22 / 101 | 降护甲（数值/百分比） | ✅ |
| 42 | 标记/虚拟光环 | ✅ 需看触发 |
| 79 / 87 / 138 | 增伤/法术易伤/特殊增伤 | ✅ |
| 118 | 降治疗 | ✅ |
| 226 | 周期性偷取生命 | ✅ 若对坦施放 |
| 23 | **周期性触发（非 debuff）** | ❌ 需沿 Trigger 追踪 |
| 255 | 虚拟标记载体 | 需看描述 |
| 33 | 降移速 | 一般非换嘲 |

### 7.4 三重验证流程

对每个候选换嘲配置，必须依次核对：

1. **DBC 语义**：中文名/描述 + `EffectAura` + `EffectTriggerSpell` 触发链，确认它究竟是「debuff 光环」还是「施法触发技/视觉效果」。
2. **脚本是否真对坦施放**：grep 源码里 `DoCastVictim` / `CastSpell(GetVictim())` 及目标选择（是否 `SelectTarget(Random)`、是否 `CastSpell(me,...)`）。
3. **持续时间 ↔ 层数匹配**：按 6.3 的 `D vs I×N` 判断层数是否可达、是否会累积。

> 仅凭「技能名 / ID 集群」判断极易把「随机点名」「视觉效果(Aura=23)」「施法触发技能」误当坦克 debuff，历史上已多次踩坑，务必走完三重验证。

---

## 8. 已覆盖副本与配置汇总

| 副本 | map_id | 难度结构 | 换嘲配置（spell_id : 层数） |
|---|---|---|---|
| 奥杜尔 | 603 | 10/25 双 entry | 柯洛刚恩 63355(1)/64002(2)；艾尔加隆 64392(1)/64679(1)；尤格萨伦 63612(2)/63673(2) |
| 冰冠堡垒 | 631 | 4 难度 | 巫妖王 69409/73797/73798/73799(1)；萨鲁法尔 72410(1)；辛德拉苟莎 69649/71056/71057/71058(1)；脓肠 72219/72551/72552/72553(9)；教授 72451/72463/72671/72672(2) |
| 卡拉赞 | 532 | 10 人唯一 | 莫克札王子 30901(3) |
| 十字军试炼 | 649 | 4 难度 | 戈莫克 66331/67477/67478/67479(3)；阿努巴拉克 66012(1) |
| 祖阿曼 | 568 | 10 人唯一 | 纳罗拉克 42389(1)、42397(1) |

**已删除（不配置换嘲）**：破钢者熔化冲压 61903/63493、墨吉姆静电瓦解 61912/63494、威札斯无面者印记 63276、血腥女王疯狂斩杀 71623（均为刷新式短 DOT 或随机点名/错归属，详见 6.2、6.3）。
