# 武器模型清单（分类核对版 · 仅有效模型）

> **数据来源**：`ItemDisplayInfo.csv` 自定义段 `100000~100022`、`420014~421072`，原始 1082 条 DisplayID。
> **模型校验目录**：`D:\wow工具\源补丁\patch-zhCN-Y\Item\OBJECTCOMPONENTS`（递归；`.m2` 与 `.mdx` 同源）+ 暂存目录 `D:\wow工具\mpq\Item`。
> **筛选口径**：模型与贴图文件齐全者才保留，共 1075 条；剔除 7 条缺失条目（100008, 100009, 100010, 100011, 100016, 100017, 100022）。
> **使用情况**：本地库 `acore_world.item_template.displayid`。
> **配套文件**：`武器物品_64040_65063.sql`、`武器物品_64040_65063_Item.dbc.csv`。

## 总览

| 项目 | 数量 |
|---|---:|
| 有效 DisplayID | 1075 |
| 去重模型文件数 | 327 |
| 已被 item_template 引用 | 50 |
| 本次新生成物品 | 1024 |
| 完全重复条目（同模型同贴图） | 4 |
| 物品 entry 区间 | 64040 ~ 65063 |

> 本段数据中没有 `axe_1h_`（单手斧）模型，故不设该类章节；`100000` 为肩甲（护甲）模型，只登记不生成物品。

### 状态图例

| 标记 | 含义 |
|---|---|
| ✔ 正常 | 本次已分配新物品 entry |
| ◑ 已使用 | 已被现有物品引用，不重复生成，表内给出原 entry |
| ✔※mpq | 模型只在 `mpq\Item` 暂存目录，当前 `patch-zhCN-Y` 内没有 |
| — 非武器 | 护甲模型，只登记不生成物品 |

### 命名规范

名称**完全由模型文件名推断**：`[左·]系列名D版本号[_子编号][特性] 颜色`

| 模型文件 | 名称 | 说明 |
|---|---|---|
| `sword_1h_artifactfelomelorn_d_01` + `..._d_01_blue` | `菲洛梅隆D01 蓝` | `_d_01` → D01，贴图后缀 `blue` → 蓝（末尾空格分隔） |
| `mace_1h_artifactdoomhammer_d_04` + `..._04_red` | `毁灭之锤D04 红` | 颜色取自 ModelTexture |
| `hand_1h_artifactskywall_d_05_1` | `苍穹D05_1` | 文件名第二段数字 → `_1` 子编号 |
| `l_glaive_1h_artifactazgalor_d_02_1` | `左·阿兹加洛D02_1` | `L_` 前缀 → 左· |
| `glaive_1h_artifactaldrochi_d_06left` | `阿尔德罗奇D06左` | `left` → 左 |
| `axe_2h_gorehowl_d_01_hd` | `血吼D01高清` | 非颜色后缀按含义译出 |
| `sword_1h_artifactnorgannon_d_01` + `..._01v1red` | `诺甘农D01v1 红` | 贴图 `v1` 变体保留 |

- **武器类型不进名称**，见「武器类型」列。
- 同一系列跨类型同名（如 `艾萨拉D03 蓝` 的锤/盾两版）靠「武器类型」列区分；完全重复的条目状态列注明「与 xxx 相同」。

## DBC 配置校验

对 `1075` 条有效 DisplayID，逐个核对补丁内 `ItemDisplayInfo.dbc` 是否含该行且 ModelName/ModelTexture 与工作 CSV 一致：

| 补丁目录 | ItemDisplayInfo.dbc 行数 | 命中有效 ID | 结论 |
|---|---:|---:|---|
| `patch-zhCN-Y` | 59747 | 1067 / 1075 | ⚠ 部分缺失（缺 8） |
| `patch-zhCN-Z` | 59747 | 1067 / 1075 | ⚠ 部分缺失（缺 8） |
| `patch-zhCN-Z2` | 57988 | 0 / 1075 | ✘ 完全不含自定义段 |
| `patch-zhCN-Z翅膀` | 无该 DBC | — | — |

**结论**：

1. `420xxx` 段（1059 条）在 `patch-zhCN-Y`、`patch-zhCN-Z` 的 `ItemDisplayInfo.dbc` 中**全部存在，模型名与贴图完全一致** —— 配置本身没问题。
2. `patch-zhCN-Z2` 的 `ItemDisplayInfo.dbc` 是**原版干净文件**（最大 ID 68744，不含任何自定义行）。而 Z2 的 `Item.dbc` 已含 64000~64129 的自定义物品，说明 Z2 在客户端是生效的 —— 按补丁覆盖规则，Z2 的 ItemDisplayInfo.dbc 会盖掉 Y/Z 里的自定义段，`420xxx`/`100xxx` 的模型在客户端将查不到。**建议把含自定义行的 `ItemDisplayInfo.dbc` 重新放进 Z2（或最高优先级补丁）再打包。**
3. `patch-zhCN-Z翅膀` 没有 ItemDisplayInfo.dbc，不参与覆盖。

`100xxx` 段有效条目与 `patch-zhCN-Y\ItemDisplayInfo.dbc` 的实际内容对比：

| DisplayID | 名称 | 模型文件 | DBC 中的配置 |
|---:|---|---|---|
| 100000 | 挑战圣骑士肩甲 蓝 | `lshoulder_plate_challengepaladin_d_01.mdx` | DBC 无此 ID |
| 100001 | 德拉诺团队D01 绿 | `sword_2h_draenorraid_d_01.mdx` | 不一致（DBC=helm_plate_challengepaladin_d_01.mdx / helm_plate_challengepaladin_d_01blue） |
| 100002 | 德拉诺挑战D01 红 | `sword_2h_draenorchallenge_d_01.mdx` | 不一致（DBC=lshoulder_plate_challengepaladin_d_01.mdx / plate_challengepaladin_d_01blue） |
| 100003 | 后羿之弓 | `houyiwuqi.mdx` | 不一致（DBC=空 / 空） |
| 100004 | 吕布·方天画戟 | `lvbu1wq.mdx` | 不一致（DBC=空 / 空） |
| 100005 | 王昭君之法杖 | `wangzhaojunwuqi.mdx` | 不一致（DBC=空 / 空） |
| 100006 | 亚瑟·王者之剑 | `yasewuqi.mdx` | 不一致（DBC=空 / 空） |
| 100007 | 诸葛亮之羽扇杖 | `zhugeliangwuqi.mdx` | 不一致（DBC=空 / 空） |
| 100012 | 泰坦阿古斯D01 红 | `polearm_2h_titanargus_d_01.mdx` | 不一致（DBC=sword_2h_draenorchallenge_d_01.mdx / sword_2h_draenorchallenge_d_01_red） |
| 100013 | 泰坦阿古斯D01 绿 | `polearm_2h_titanargus_d_01.mdx` | DBC 无此 ID |
| 100014 | 泰坦阿古斯D01 品红 | `polearm_2h_titanargus_d_01.mdx` | DBC 无此 ID |
| 100015 | 泰坦阿古斯D01 | `polearm_2h_titanargus_d_01.mdx` | DBC 无此 ID |
| 100018 | 灰烬使者·雷霆D03 青 | `sword_2h_artifactashbringerlightning_d_03.mdx` | DBC 无此 ID |
| 100019 | 灰烬使者·雷霆D03 紫 | `sword_2h_artifactashbringerlightning_d_03.mdx` | DBC 无此 ID |
| 100020 | 灰烬使者·雷霆D03 白 | `sword_2h_artifactashbringerlightning_d_03.mdx` | DBC 无此 ID |
| 100021 | 灰烬使者·雷霆D03 黄 | `sword_2h_artifactashbringerlightning_d_03.mdx` | DBC 无此 ID |

**结论**：`100xxx` 段在补丁 DBC 中是**旧版本**——编号整体错位（DBC 的 100001/100002 还是头盔、肩甲，100012 还是德拉诺挑战巨剑），多个 ID 直接不存在，说明工作 CSV 中这批行**尚未导出进 `ItemDisplayInfo.dbc`**，需要用 WDBX 重新导出后再打包。

Item.dbc 占用情况（本次生成的 entry 区间 `64040~65063`）：

| 补丁目录 | Item.dbc 行数 | 该区间已有条目 |
|---|---:|---:|
| `patch-zhCN-Y` | 445766 | 0 |
| `patch-zhCN-Z` | 445766 | 0 |
| `patch-zhCN-Z2` | 81173 | 792 |
| `patch-zhCN-Z翅膀` | 46096 | 0 |

**结论**：服务端 `item_template` 在 `64040~65063` 区间原本为空（现有自定义物品只到 64039），服务端不冲突；但 `patch-zhCN-Z2` 的 `Item.dbc` 里该区间已有大量 **Blizzard 原生物品** 行，导入本表的 Item.dbc CSV 会覆盖它们——这些 ID 在服务端没有对应物品，覆盖不影响游戏；若要绝对零风险，可把整段 entry 挪到 68745 以上（Z2 的 Item.dbc 在 68745+ 为空）。

## 一、单手剑（123 条：已使用 5 / 新生成 118）

> `class=2, subclass=7, InventoryType=13, sheath=3, Material=1`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420819 | 单手剑 | 64040 | 菲洛梅隆D01 蓝 | `sword_1h_artifactfelomelorn_d_01.mdx` | `sword_1h_artifactfelomelorn_d_01_blue` | ✔ 正常 |
| 420820 | 单手剑 | 64041 | 菲洛梅隆D01 绿 | `sword_1h_artifactfelomelorn_d_01.mdx` | `sword_1h_artifactfelomelorn_d_01_green` | ✔ 正常 |
| 420821 | 单手剑 | 64042 | 菲洛梅隆D01 紫 | `sword_1h_artifactfelomelorn_d_01.mdx` | `sword_1h_artifactfelomelorn_d_01_purple` | ✔ 正常 |
| 420822 | 单手剑 | 64043 | 菲洛梅隆D01 红 | `sword_1h_artifactfelomelorn_d_01.mdx` | `sword_1h_artifactfelomelorn_d_01_red` | ✔ 正常 |
| 420823 | 单手剑 | 64044 | 菲洛梅隆D02 蓝 | `sword_1h_artifactfelomelorn_d_02.mdx` | `sword_1h_artifactfelomelorn_d_02blue` | ✔ 正常 |
| 420824 | 单手剑 | 64045 | 菲洛梅隆D02 绿 | `sword_1h_artifactfelomelorn_d_02.mdx` | `sword_1h_artifactfelomelorn_d_02green` | ✔ 正常 |
| 420825 | 单手剑 | 64046 | 菲洛梅隆D02 紫 | `sword_1h_artifactfelomelorn_d_02.mdx` | `sword_1h_artifactfelomelorn_d_02purple` | ✔ 正常 |
| 420826 | 单手剑 | 64047 | 菲洛梅隆D02 红 | `sword_1h_artifactfelomelorn_d_02.mdx` | `sword_1h_artifactfelomelorn_d_02red` | ✔ 正常 |
| 420827 | 单手剑 | 64048 | 菲洛梅隆D03 蓝 | `sword_1h_artifactfelomelorn_d_03.mdx` | `sword_1h_artifactfelomelorn_d_03blue` | ✔ 正常 |
| 420828 | 单手剑 | 64049 | 菲洛梅隆D03 绿 | `sword_1h_artifactfelomelorn_d_03.mdx` | `sword_1h_artifactfelomelorn_d_03green` | ✔ 正常 |
| 420829 | 单手剑 | 64050 | 菲洛梅隆D03 紫 | `sword_1h_artifactfelomelorn_d_03.mdx` | `sword_1h_artifactfelomelorn_d_03purple` | ✔ 正常 |
| 420830 | 单手剑 | 64051 | 菲洛梅隆D03 红 | `sword_1h_artifactfelomelorn_d_03.mdx` | `sword_1h_artifactfelomelorn_d_03red` | ✔ 正常 |
| 420831 | 单手剑 | 64052 | 菲洛梅隆D04 蓝 | `sword_1h_artifactfelomelorn_d_04.mdx` | `sword_1h_artifactfelomelorn_d_04blue` | ✔ 正常 |
| 420832 | 单手剑 | 64053 | 菲洛梅隆D04 绿 | `sword_1h_artifactfelomelorn_d_04.mdx` | `sword_1h_artifactfelomelorn_d_04green` | ✔ 正常 |
| 420833 | 单手剑 | 64054 | 菲洛梅隆D04 紫 | `sword_1h_artifactfelomelorn_d_04.mdx` | `sword_1h_artifactfelomelorn_d_04purple` | ✔ 正常 |
| 420834 | 单手剑 | 64055 | 菲洛梅隆D04 红 | `sword_1h_artifactfelomelorn_d_04.mdx` | `sword_1h_artifactfelomelorn_d_04red` | ✔ 正常 |
| 420835 | 单手剑 | 64056 | 菲洛梅隆D05 金 | `sword_1h_artifactfelomelorn_d_05.mdx` | `sword_1h_artifactfelomelorn_d_05gold` | ✔ 正常 |
| 420836 | 单手剑 | 64057 | 菲洛梅隆D05 绿 | `sword_1h_artifactfelomelorn_d_05.mdx` | `sword_1h_artifactfelomelorn_d_05green` | ✔ 正常 |
| 420837 | 单手剑 | 64058 | 菲洛梅隆D05 紫 | `sword_1h_artifactfelomelorn_d_05.mdx` | `sword_1h_artifactfelomelorn_d_05purple` | ✔ 正常 |
| 420838 | 单手剑 | 64059 | 菲洛梅隆D05 红 | `sword_1h_artifactfelomelorn_d_05.mdx` | `sword_1h_artifactfelomelorn_d_05red` | ✔ 正常 |
| 420839 | 单手剑 | 64060 | 菲洛梅隆D06 暗 | `sword_1h_artifactfelomelorn_d_06.mdx` | `sword_1h_artifactfelomelorn_d_06_dark` | ✔ 正常 |
| 420840 | 单手剑 | 50427 | 菲洛梅隆D06 光 | `sword_1h_artifactfelomelorn_d_06.mdx` | `sword_1h_artifactfelomelorn_d_06_light` | ◑ 已使用 |
| 420841 | 单手剑 | 64061 | 菲洛梅隆D06 红 | `sword_1h_artifactfelomelorn_d_06.mdx` | `sword_1h_artifactfelomelorn_d_06_red` | ✔ 正常 |
| 420842 | 单手剑 | 64062 | 菲洛梅隆D06 黄 | `sword_1h_artifactfelomelorn_d_06.mdx` | `sword_1h_artifactfelomelorn_d_06_yellow` | ✔ 正常 |
| 420843 | 单手剑 | 64063 | 玛格纳D01 蓝 | `sword_1h_artifactmagnar_d_01.mdx` | `sword_1h_artifactmagnar_d_01blue` | ✔ 正常 |
| 420844 | 单手剑 | 64064 | 玛格纳D01 绿 | `sword_1h_artifactmagnar_d_01.mdx` | `sword_1h_artifactmagnar_d_01green` | ✔ 正常 |
| 420845 | 单手剑 | 64065 | 玛格纳D01 紫 | `sword_1h_artifactmagnar_d_01.mdx` | `sword_1h_artifactmagnar_d_01purple` | ✔ 正常 |
| 420846 | 单手剑 | 64066 | 玛格纳D01 红 | `sword_1h_artifactmagnar_d_01.mdx` | `sword_1h_artifactmagnar_d_01red` | ✔ 正常 |
| 420847 | 单手剑 | 64067 | 玛格纳D02 蓝 | `sword_1h_artifactmagnar_d_02.mdx` | `sword_1h_artifactmagnar_d_02_blue` | ✔ 正常 |
| 420848 | 单手剑 | 64068 | 玛格纳D02 绿 | `sword_1h_artifactmagnar_d_02.mdx` | `sword_1h_artifactmagnar_d_02_green` | ✔ 正常 |
| 420849 | 单手剑 | 64069 | 玛格纳D02 紫 | `sword_1h_artifactmagnar_d_02.mdx` | `sword_1h_artifactmagnar_d_02_purple` | ✔ 正常 |
| 420850 | 单手剑 | 64070 | 玛格纳D02 红 | `sword_1h_artifactmagnar_d_02.mdx` | `sword_1h_artifactmagnar_d_02_red` | ✔ 正常 |
| 420851 | 单手剑 | 64071 | 玛格纳D04 蓝 | `sword_1h_artifactmagnar_d_04.mdx` | `sword_1h_artifactmagnar_d_04_blue` | ✔ 正常 |
| 420852 | 单手剑 | 64072 | 玛格纳D04 绿 | `sword_1h_artifactmagnar_d_04.mdx` | `sword_1h_artifactmagnar_d_04_green` | ✔ 正常 |
| 420853 | 单手剑 | 64073 | 玛格纳D04 紫 | `sword_1h_artifactmagnar_d_04.mdx` | `sword_1h_artifactmagnar_d_04_purple` | ✔ 正常 |
| 420854 | 单手剑 | 64074 | 玛格纳D04 红 | `sword_1h_artifactmagnar_d_04.mdx` | `sword_1h_artifactmagnar_d_04_red` | ✔ 正常 |
| 420855 | 单手剑 | 64075 | 诺甘农D01v1 蓝 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v1blue` | ✔ 正常 |
| 420856 | 单手剑 | 64076 | 诺甘农D01v1 紫 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v1purple` | ✔ 正常 |
| 420857 | 单手剑 | 64077 | 诺甘农D01v1 红 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v1red` | ✔ 正常 |
| 420858 | 单手剑 | 64078 | 诺甘农D01v1 黄 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v1yellow` | ✔ 正常 |
| 420859 | 单手剑 | 64079 | 诺甘农D01v2 蓝 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v2blue` | ✔ 正常 |
| 420860 | 单手剑 | 64080 | 诺甘农D01v2 红 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v2red` | ✔ 正常 |
| 420861 | 单手剑 | 64081 | 诺甘农D01v2 白 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v2white` | ✔ 正常 |
| 420862 | 单手剑 | 64082 | 诺甘农D01v2 黄 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v2yellow` | ✔ 正常 |
| 420863 | 单手剑 | 64083 | 诺甘农D01v3 蓝 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v3blue` | ✔ 正常 |
| 420864 | 单手剑 | 64084 | 诺甘农D01v3 绿 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v3green` | ✔ 正常 |
| 420865 | 单手剑 | 64085 | 诺甘农D01v3 红 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v3red` | ✔ 正常 |
| 420866 | 单手剑 | 64086 | 诺甘农D01v3 黄 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v3yellow` | ✔ 正常 |
| 420867 | 单手剑 | 64087 | 诺甘农D01v4 蓝 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v4blue` | ✔ 正常 |
| 420868 | 单手剑 | 64088 | 诺甘农D01v4 紫 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v4purple` | ✔ 正常 |
| 420869 | 单手剑 | 64089 | 诺甘农D01v4 红 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v4red` | ✔ 正常 |
| 420870 | 单手剑 | 64090 | 诺甘农D01v4 黄 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v4yellow` | ✔ 正常 |
| 420871 | 单手剑 | 64091 | 诺甘农D01v5 绿 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v5green` | ✔ 正常 |
| 420872 | 单手剑 | 64092 | 诺甘农D01v5 紫 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v5purple` | ✔ 正常 |
| 420873 | 单手剑 | 64093 | 诺甘农D01v5 红 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v5red` | ✔ 正常 |
| 420874 | 单手剑 | 64094 | 诺甘农D01v5 黄 | `sword_1h_artifactnorgannon_d_01.mdx` | `sword_1h_artifactnorgannon_d_01v5yellow` | ✔ 正常 |
| 420875 | 单手剑 | 64095 | 符文剑D01 蓝 | `sword_1h_artifactruneblade_d_01.mdx` | `sword_1h_artifactruneblade_d_01_blue` | ✔ 正常 |
| 420876 | 单手剑 | 64096 | 符文剑D01 绿 | `sword_1h_artifactruneblade_d_01.mdx` | `sword_1h_artifactruneblade_d_01_green` | ✔ 正常 |
| 420877 | 单手剑 | 64097 | 符文剑D01 红 | `sword_1h_artifactruneblade_d_01.mdx` | `sword_1h_artifactruneblade_d_01_red` | ✔ 正常 |
| 420878 | 单手剑 | 64098 | 符文剑D01 青 | `sword_1h_artifactruneblade_d_01.mdx` | `sword_1h_artifactruneblade_d_01_teal` | ✔ 正常 |
| 420879 | 单手剑 | 64099 | 符文剑D02 蓝 | `sword_1h_artifactruneblade_d_02.mdx` | `sword_1h_artifactruneblade_d_02_blue` | ✔ 正常 |
| 420880 | 单手剑 | 64100 | 符文剑D02 绿 | `sword_1h_artifactruneblade_d_02.mdx` | `sword_1h_artifactruneblade_d_02_green` | ✔ 正常 |
| 420881 | 单手剑 | 64101 | 符文剑D02 红 | `sword_1h_artifactruneblade_d_02.mdx` | `sword_1h_artifactruneblade_d_02_red` | ✔ 正常 |
| 420882 | 单手剑 | 50068 | 符文剑D02 青 | `sword_1h_artifactruneblade_d_02.mdx` | `sword_1h_artifactruneblade_d_02_teal` | ◑ 已使用 |
| 420883 | 单手剑 | 64102 | 符文剑D03 蓝 | `sword_1h_artifactruneblade_d_03.mdx` | `sword_1h_artifactruneblade_d_03_blue` | ✔ 正常 |
| 420884 | 单手剑 | 51010 | 符文剑D03 绿 | `sword_1h_artifactruneblade_d_03.mdx` | `sword_1h_artifactruneblade_d_03_green` | ◑ 已使用 |
| 420885 | 单手剑 | 64103 | 符文剑D03 紫 | `sword_1h_artifactruneblade_d_03.mdx` | `sword_1h_artifactruneblade_d_03_purple` | ✔ 正常 |
| 420886 | 单手剑 | 64104 | 符文剑D03 红 | `sword_1h_artifactruneblade_d_03.mdx` | `sword_1h_artifactruneblade_d_03_red` | ✔ 正常 |
| 420887 | 单手剑 | 64105 | 符文剑D04 蓝 | `sword_1h_artifactruneblade_d_04.mdx` | `sword_1h_artifactruneblade_d_04_blue` | ✔ 正常 |
| 420888 | 单手剑 | 64106 | 符文剑D04 绿 | `sword_1h_artifactruneblade_d_04.mdx` | `sword_1h_artifactruneblade_d_04_green` | ✔ 正常 |
| 420889 | 单手剑 | 64107 | 符文剑D04 紫 | `sword_1h_artifactruneblade_d_04.mdx` | `sword_1h_artifactruneblade_d_04_purple` | ✔ 正常 |
| 420890 | 单手剑 | 64108 | 符文剑D04 红 | `sword_1h_artifactruneblade_d_04.mdx` | `sword_1h_artifactruneblade_d_04_red` | ✔ 正常 |
| 420891 | 单手剑 | 64109 | 符文剑D05 蓝 | `sword_1h_artifactruneblade_d_05.mdx` | `sword_1h_artifactruneblade_d_05_blue` | ✔ 正常 |
| 420892 | 单手剑 | 64110 | 符文剑D05 绿 | `sword_1h_artifactruneblade_d_05.mdx` | `sword_1h_artifactruneblade_d_05_green` | ✔ 正常 |
| 420893 | 单手剑 | 64111 | 符文剑D05 橙 | `sword_1h_artifactruneblade_d_05.mdx` | `sword_1h_artifactruneblade_d_05_orange` | ✔ 正常 |
| 420894 | 单手剑 | 64112 | 符文剑D05 黄 | `sword_1h_artifactruneblade_d_05.mdx` | `sword_1h_artifactruneblade_d_05_yellow` | ✔ 正常 |
| 420895 | 单手剑 | 64113 | 符文剑D06 蓝 | `sword_1h_artifactruneblade_d_06.mdx` | `sword_1h_artifactruneblade_d_06blue` | ✔ 正常 |
| 420896 | 单手剑 | 64114 | 符文剑D06 紫 | `sword_1h_artifactruneblade_d_06.mdx` | `sword_1h_artifactruneblade_d_06purple` | ✔ 正常 |
| 420897 | 单手剑 | 64115 | 符文剑D06 红 | `sword_1h_artifactruneblade_d_06.mdx` | `sword_1h_artifactruneblade_d_06red` | ✔ 正常 |
| 420898 | 单手剑 | 64116 | 苍穹D01 蓝 | `sword_1h_artifactskywall_d_01.mdx` | `sword_1h_artifactskywall_d_01_blue` | ✔ 正常 |
| 420899 | 单手剑 | 64117 | 苍穹D01 紫 | `sword_1h_artifactskywall_d_01.mdx` | `sword_1h_artifactskywall_d_01_purple` | ✔ 正常 |
| 420900 | 单手剑 | 64118 | 苍穹D01 红 | `sword_1h_artifactskywall_d_01.mdx` | `sword_1h_artifactskywall_d_01_red` | ✔ 正常 |
| 420901 | 单手剑 | 64119 | 苍穹D01 黄 | `sword_1h_artifactskywall_d_01.mdx` | `sword_1h_artifactskywall_d_01_yellow` | ✔ 正常 |
| 420902 | 单手剑 | 64120 | 苍穹D02 蓝 | `sword_1h_artifactskywall_d_02.mdx` | `sword_1h_artifactskywall_d_02_blue` | ✔ 正常 |
| 420903 | 单手剑 | 64121 | 苍穹D02 紫 | `sword_1h_artifactskywall_d_02.mdx` | `sword_1h_artifactskywall_d_02_purple` | ✔ 正常 |
| 420904 | 单手剑 | 64122 | 苍穹D02 红 | `sword_1h_artifactskywall_d_02.mdx` | `sword_1h_artifactskywall_d_02_red` | ✔ 正常 |
| 420905 | 单手剑 | 64123 | 苍穹D02 黄 | `sword_1h_artifactskywall_d_02.mdx` | `sword_1h_artifactskywall_d_02_yellow` | ✔ 正常 |
| 420906 | 单手剑 | 64124 | 苍穹D03 蓝 | `sword_1h_artifactskywall_d_03.mdx` | `sword_1h_artifactskywall_d_03_blue` | ✔ 正常 |
| 420907 | 单手剑 | 64125 | 苍穹D03 紫 | `sword_1h_artifactskywall_d_03.mdx` | `sword_1h_artifactskywall_d_03_purple` | ✔ 正常 |
| 420908 | 单手剑 | 110004, 110014 | 苍穹D03 红 | `sword_1h_artifactskywall_d_03.mdx` | `sword_1h_artifactskywall_d_03_red` | ◑ 已使用 |
| 420909 | 单手剑 | 64126 | 苍穹D03 黄 | `sword_1h_artifactskywall_d_03.mdx` | `sword_1h_artifactskywall_d_03_yellow` | ✔ 正常 |
| 420910 | 单手剑 | 64127 | 苍穹D04 蓝 | `sword_1h_artifactskywall_d_04.mdx` | `sword_1h_artifactskywall_d_04_blue` | ✔ 正常 |
| 420911 | 单手剑 | 64128 | 苍穹D04 橙 | `sword_1h_artifactskywall_d_04.mdx` | `sword_1h_artifactskywall_d_04_orange` | ✔ 正常 |
| 420912 | 单手剑 | 64129 | 苍穹D04 青 | `sword_1h_artifactskywall_d_04.mdx` | `sword_1h_artifactskywall_d_04_teal` | ✔ 正常 |
| 420913 | 单手剑 | 50412 | 苍穹D04 白 | `sword_1h_artifactskywall_d_04.mdx` | `sword_1h_artifactskywall_d_04_white` | ◑ 已使用 |
| 420914 | 单手剑 | 64130 | 苍穹D05 蓝 | `sword_1h_artifactskywall_d_05.mdx` | `sword_1h_artifactskywall_d_05_blue` | ✔ 正常 |
| 420915 | 单手剑 | 64131 | 苍穹D05 绿 | `sword_1h_artifactskywall_d_05.mdx` | `sword_1h_artifactskywall_d_05_green` | ✔ 正常 |
| 420916 | 单手剑 | 64132 | 苍穹D05 红 | `sword_1h_artifactskywall_d_05.mdx` | `sword_1h_artifactskywall_d_05_red` | ✔ 正常 |
| 420917 | 单手剑 | 64133 | 苍穹D05 青 | `sword_1h_artifactskywall_d_05.mdx` | `sword_1h_artifactskywall_d_05_teal` | ✔ 正常 |
| 420918 | 单手剑 | 64134 | 苍穹D06 绿 | `sword_1h_artifactskywall_d_06.mdx` | `sword_1h_artifactskywall_d_06green` | ✔ 正常 |
| 420919 | 单手剑 | 64135 | 苍穹D06 橙 | `sword_1h_artifactskywall_d_06.mdx` | `sword_1h_artifactskywall_d_06orange` | ✔ 正常 |
| 420920 | 单手剑 | 64136 | 苍穹D06 紫 | `sword_1h_artifactskywall_d_06.mdx` | `sword_1h_artifactskywall_d_06purple` | ✔ 正常 |
| 420921 | 单手剑 | 64137 | 苍穹D06 | `sword_1h_artifactskywall_d_06.mdx` | `sword_1h_artifactskywall_d_06` | ✔ 正常 |
| 420922 | 单手剑 | 64138 | 维格弗斯D01 原版 | `sword_1h_artifactvigfus_d_01.mdx` | `sword_1h_artifactvigfus_d_01_base` | ✔ 正常 |
| 420923 | 单手剑 | 64139 | 维格弗斯D01 绿 | `sword_1h_artifactvigfus_d_01.mdx` | `sword_1h_artifactvigfus_d_01_green` | ✔ 正常 |
| 420924 | 单手剑 | 64140 | 维格弗斯D01 紫 | `sword_1h_artifactvigfus_d_01.mdx` | `sword_1h_artifactvigfus_d_01_purple` | ✔ 正常 |
| 420925 | 单手剑 | 64141 | 维格弗斯D01 红 | `sword_1h_artifactvigfus_d_01.mdx` | `sword_1h_artifactvigfus_d_01_red` | ✔ 正常 |
| 420926 | 单手剑 | 64142 | 维格弗斯D02 原版 | `sword_1h_artifactvigfus_d_02.mdx` | `sword_1h_artifactvigfus_d_02_base` | ✔ 正常 |
| 420927 | 单手剑 | 64143 | 维格弗斯D02 绿 | `sword_1h_artifactvigfus_d_02.mdx` | `sword_1h_artifactvigfus_d_02_green` | ✔ 正常 |
| 420928 | 单手剑 | 64144 | 维格弗斯D02 紫 | `sword_1h_artifactvigfus_d_02.mdx` | `sword_1h_artifactvigfus_d_02_purple` | ✔ 正常 |
| 420929 | 单手剑 | 64145 | 维格弗斯D02 红 | `sword_1h_artifactvigfus_d_02.mdx` | `sword_1h_artifactvigfus_d_02_red` | ✔ 正常 |
| 420930 | 单手剑 | 64146 | 维格弗斯D03_1 | `Sword_1h_ArtifactVigfus_D_03_1.mdx` | — | ✔ 正常 |
| 420931 | 单手剑 | 64147 | 维格弗斯D03_2 | `Sword_1h_ArtifactVigfus_D_03_2.mdx` | — | ✔ 正常 |
| 420932 | 单手剑 | 64148 | 维格弗斯D03_3 | `Sword_1h_ArtifactVigfus_D_03_3.mdx` | — | ✔ 正常 |
| 420933 | 单手剑 | 64149 | 维格弗斯D03_4 | `Sword_1h_ArtifactVigfus_D_03_4.mdx` | — | ✔ 正常 |
| 420934 | 单手剑 | 64150 | 维格弗斯D04 原版 | `sword_1h_artifactvigfus_d_04.mdx` | `sword_1h_artifactvigfus_d_04_base` | ✔ 正常 |
| 420935 | 单手剑 | 64151 | 维格弗斯D04 绿 | `sword_1h_artifactvigfus_d_04.mdx` | `sword_1h_artifactvigfus_d_04_green` | ✔ 正常 |
| 420936 | 单手剑 | 64152 | 维格弗斯D04 紫 | `sword_1h_artifactvigfus_d_04.mdx` | `sword_1h_artifactvigfus_d_04_purple` | ✔ 正常 |
| 420937 | 单手剑 | 64153 | 维格弗斯D04 红 | `sword_1h_artifactvigfus_d_04.mdx` | `sword_1h_artifactvigfus_d_04_red` | ✔ 正常 |
| 420938 | 单手剑 | 64154 | 维格弗斯D05 原版 | `sword_1h_artifactvigfus_d_05.mdx` | `sword_1h_artifactvigfus_d_05_base` | ✔ 正常 |
| 420939 | 单手剑 | 64155 | 维格弗斯D05 绿 | `sword_1h_artifactvigfus_d_05.mdx` | `sword_1h_artifactvigfus_d_05_green` | ✔ 正常 |
| 420940 | 单手剑 | 64156 | 维格弗斯D05 紫 | `sword_1h_artifactvigfus_d_05.mdx` | `sword_1h_artifactvigfus_d_05_purple` | ✔ 正常 |
| 420941 | 单手剑 | 64157 | 维格弗斯D05 红 | `sword_1h_artifactvigfus_d_05.mdx` | `sword_1h_artifactvigfus_d_05_red` | ✔ 正常 |

## 二、双手剑（76 条：已使用 13 / 新生成 63）

> `class=2, subclass=8, InventoryType=17, sheath=1, Material=1`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420942 | 双手剑 | 64158 | 阿拉索D01 蓝 | `sword_2h_artifactarathor_d_01.mdx` | `sword_2h_artifactarathor_d_01_blue` | ✔ 正常 |
| 420943 | 双手剑 | 64159 | 阿拉索D01 绿 | `sword_2h_artifactarathor_d_01.mdx` | `sword_2h_artifactarathor_d_01_green` | ✔ 正常 |
| 420944 | 双手剑 | 64160 | 阿拉索D01 紫 | `sword_2h_artifactarathor_d_01.mdx` | `sword_2h_artifactarathor_d_01_purple` | ✔ 正常 |
| 420945 | 双手剑 | 64161 | 阿拉索D01 红 | `sword_2h_artifactarathor_d_01.mdx` | `sword_2h_artifactarathor_d_01_red` | ✔ 正常 |
| 420946 | 双手剑 | 64162 | 阿拉索D02 蓝 | `sword_2h_artifactarathor_d_02.mdx` | `sword_2h_artifactarathor_d_02_blue` | ✔ 正常 |
| 420947 | 双手剑 | 64163 | 阿拉索D02 绿 | `sword_2h_artifactarathor_d_02.mdx` | `sword_2h_artifactarathor_d_02_green` | ✔ 正常 |
| 420948 | 双手剑 | 64164 | 阿拉索D02 紫 | `sword_2h_artifactarathor_d_02.mdx` | `sword_2h_artifactarathor_d_02_purple` | ✔ 正常 |
| 420949 | 双手剑 | 64165 | 阿拉索D02 红 | `sword_2h_artifactarathor_d_02.mdx` | `sword_2h_artifactarathor_d_02_red` | ✔ 正常 |
| 420950 | 双手剑 | 64166 | 阿拉索D03 蓝 | `sword_2h_artifactarathor_d_03.mdx` | `sword_2h_artifactarathor_d_03_blue` | ✔ 正常 |
| 420951 | 双手剑 | 64167 | 阿拉索D03 绿 | `sword_2h_artifactarathor_d_03.mdx` | `sword_2h_artifactarathor_d_03_green` | ✔ 正常 |
| 420952 | 双手剑 | 64168 | 阿拉索D03 紫 | `sword_2h_artifactarathor_d_03.mdx` | `sword_2h_artifactarathor_d_03_purple` | ✔ 正常 |
| 420953 | 双手剑 | 64169 | 阿拉索D03 红 | `sword_2h_artifactarathor_d_03.mdx` | `sword_2h_artifactarathor_d_03_red` | ✔ 正常 |
| 420954 | 双手剑 | 64170 | 阿拉索D04 绿 | `sword_2h_artifactarathor_d_04.mdx` | `sword_2h_artifactarathor_d_04_green` | ✔ 正常 |
| 420955 | 双手剑 | 64171 | 阿拉索D04 紫 | `sword_2h_artifactarathor_d_04.mdx` | `sword_2h_artifactarathor_d_04_purple` | ✔ 正常 |
| 420956 | 双手剑 | 64172 | 阿拉索D04 红 | `sword_2h_artifactarathor_d_04.mdx` | `sword_2h_artifactarathor_d_04_red` | ✔ 正常 |
| 420957 | 双手剑 | 64173 | 阿拉索D04 黄 | `sword_2h_artifactarathor_d_04.mdx` | `sword_2h_artifactarathor_d_04_yellow` | ✔ 正常 |
| 420958 | 双手剑 | 64174 | 阿拉索D05 蓝 | `sword_2h_artifactarathor_d_05.mdx` | `sword_2h_artifactarathor_d_05_blue` | ✔ 正常 |
| 420959 | 双手剑 | 64175 | 阿拉索D05 绿 | `sword_2h_artifactarathor_d_05.mdx` | `sword_2h_artifactarathor_d_05_green` | ✔ 正常 |
| 420960 | 双手剑 | 64176 | 阿拉索D05 紫 | `sword_2h_artifactarathor_d_05.mdx` | `sword_2h_artifactarathor_d_05_purple` | ✔ 正常 |
| 420961 | 双手剑 | 33478 | 阿拉索D05 黄 | `sword_2h_artifactarathor_d_05.mdx` | `sword_2h_artifactarathor_d_05_yellow` | ◑ 已使用 |
| 420962 | 双手剑 | 64177 | 灰烬使者D01 蓝 | `sword_2h_artifactashbringer_d_01.mdx` | `sword_2h_artifactashbringer_d_01_blue` | ✔ 正常 |
| 420963 | 双手剑 | 64178 | 灰烬使者D01 金 | `sword_2h_artifactashbringer_d_01.mdx` | `sword_2h_artifactashbringer_d_01_gold` | ✔ 正常 |
| 420964 | 双手剑 | 64179 | 灰烬使者D01 绿 | `sword_2h_artifactashbringer_d_01.mdx` | `sword_2h_artifactashbringer_d_01_green` | ✔ 正常 |
| 420965 | 双手剑 | 50070 | 灰烬使者D01 红 | `sword_2h_artifactashbringer_d_01.mdx` | `sword_2h_artifactashbringer_d_01_red` | ◑ 已使用 |
| 420966 | 双手剑 | 64180 | 灰烬使者D06 绿 | `sword_2h_artifactashbringer_d_06.mdx` | `sword_2h_artifactashbringer_d_06_green` | ✔ 正常 |
| 420967 | 双手剑 | 64181 | 灰烬使者D06 紫 | `sword_2h_artifactashbringer_d_06.mdx` | `sword_2h_artifactashbringer_d_06_purple` | ✔ 正常 |
| 420968 | 双手剑 | 64182 | 灰烬使者D06 红 | `sword_2h_artifactashbringer_d_06.mdx` | `sword_2h_artifactashbringer_d_06_red` | ✔ 正常 |
| 420969 | 双手剑 | 64183 | 灰烬使者D06 黄 | `sword_2h_artifactashbringer_d_06.mdx` | `sword_2h_artifactashbringer_d_06_yellow` | ✔ 正常 |
| 420970 | 双手剑 | 61044 | 灰烬使者·烈焰D03 蓝 | `sword_2h_artifactashbringerfire_d_03.mdx` | `sword_2h_artifactashbringerfire_d_03_blue` | ◑ 已使用 |
| 420971 | 双手剑 | 61045 | 灰烬使者·烈焰D03 绿 | `sword_2h_artifactashbringerfire_d_03.mdx` | `sword_2h_artifactashbringerfire_d_03_green` | ◑ 已使用 |
| 420972 | 双手剑 | 61046 | 灰烬使者·烈焰D03 红 | `sword_2h_artifactashbringerfire_d_03.mdx` | `sword_2h_artifactashbringerfire_d_03_red` | ◑ 已使用 |
| 420973 | 双手剑 | 64184 | 灰烬使者·烈焰D03 黄 | `sword_2h_artifactashbringerfire_d_03.mdx` | `sword_2h_artifactashbringerfire_d_03_yellow` | ✔ 正常 |
| 100018 | 双手剑 | 64185 | 灰烬使者·雷霆D03 青 | `sword_2h_artifactashbringerlightning_d_03.mdx` | `sword_2h_artifactashbringerlightning_d_03teal` | ✔ 正常 |
| 100019 | 双手剑 | 64186 | 灰烬使者·雷霆D03 紫 | `sword_2h_artifactashbringerlightning_d_03.mdx` | `sword_2h_artifactashbringerlightning_d_03purple` | ✔ 正常 |
| 100020 | 双手剑 | 64187 | 灰烬使者·雷霆D03 白 | `sword_2h_artifactashbringerlightning_d_03.mdx` | `sword_2h_artifactashbringerlightning_d_03white` | ✔ 正常 |
| 100021 | 双手剑 | 64188 | 灰烬使者·雷霆D03 黄 | `sword_2h_artifactashbringerlightning_d_03.mdx` | `sword_2h_artifactashbringerlightning_d_03yellow` | ✔ 正常 |
| 420974 | 双手剑 | 64189 | 灰烬使者·雷霆D03 紫 | `sword_2h_artifactashbringerlightning_d_03.mdx` | `sword_2h_artifactashbringerlightning_d_03purple` | ✔ 正常（与 100019 相同） |
| 420975 | 双手剑 | 64190 | 灰烬使者·雷霆D03 青 | `sword_2h_artifactashbringerlightning_d_03.mdx` | `sword_2h_artifactashbringerlightning_d_03teal` | ✔ 正常（与 100018 相同） |
| 420976 | 双手剑 | 64191 | 灰烬使者·雷霆D03 白 | `sword_2h_artifactashbringerlightning_d_03.mdx` | `sword_2h_artifactashbringerlightning_d_03white` | ✔ 正常（与 100020 相同） |
| 420977 | 双手剑 | 64192 | 灰烬使者·雷霆D03 黄 | `sword_2h_artifactashbringerlightning_d_03.mdx` | `sword_2h_artifactashbringerlightning_d_03yellow` | ✔ 正常（与 100021 相同） |
| 420978 | 双手剑 | 64193 | 灰烬使者·净化D03 蓝 | `sword_2h_artifactashbringerpurified_d_03.mdx` | `sword_2h_artifactashbringerpurified_d_03_blue` | ✔ 正常 |
| 420979 | 双手剑 | 64194 | 灰烬使者·净化D03 金 | `sword_2h_artifactashbringerpurified_d_03.mdx` | `sword_2h_artifactashbringerpurified_d_03_gold` | ✔ 正常 |
| 420980 | 双手剑 | 64195 | 灰烬使者·净化D03 绿 | `sword_2h_artifactashbringerpurified_d_03.mdx` | `sword_2h_artifactashbringerpurified_d_03_green` | ✔ 正常 |
| 420981 | 双手剑 | 64196 | 灰烬使者·净化D03 红 | `sword_2h_artifactashbringerpurified_d_03.mdx` | `sword_2h_artifactashbringerpurified_d_03_red` | ✔ 正常 |
| 420982 | 双手剑 | 64197 | 灰烬使者·暗影D03 蓝 | `sword_2h_artifactashbringershadow_d_03.mdx` | `sword_2h_artifactashbringershadow_d_03_blue` | ✔ 正常 |
| 420983 | 双手剑 | 64198 | 灰烬使者·暗影D03 绿 | `sword_2h_artifactashbringershadow_d_03.mdx` | `sword_2h_artifactashbringershadow_d_03_green` | ✔ 正常 |
| 420984 | 双手剑 | 64199 | 灰烬使者·暗影D03 紫 | `sword_2h_artifactashbringershadow_d_03.mdx` | `sword_2h_artifactashbringershadow_d_03_purple` | ✔ 正常 |
| 420985 | 双手剑 | 93002 | 灰烬使者·暗影D03 红 | `sword_2h_artifactashbringershadow_d_03.mdx` | `sword_2h_artifactashbringershadow_d_03_red` | ◑ 已使用 |
| 420986 | 双手剑 | 64200 | 灵魂撕裂者D01 蓝 | `sword_2h_artifactsoulrend_d_01.mdx` | `sword_2h_artifactsoulrend_d_01_blue` | ✔ 正常 |
| 420987 | 双手剑 | 64201 | 灵魂撕裂者D01 绿 | `sword_2h_artifactsoulrend_d_01.mdx` | `sword_2h_artifactsoulrend_d_01_green` | ✔ 正常 |
| 420988 | 双手剑 | 64202 | 灵魂撕裂者D01 紫 | `sword_2h_artifactsoulrend_d_01.mdx` | `sword_2h_artifactsoulrend_d_01_purple` | ✔ 正常 |
| 420989 | 双手剑 | 64203 | 灵魂撕裂者D01 红 | `sword_2h_artifactsoulrend_d_01.mdx` | `sword_2h_artifactsoulrend_d_01_red` | ✔ 正常 |
| 420990 | 双手剑 | 64204 | 灵魂撕裂者D02 蓝 | `sword_2h_artifactsoulrend_d_02.mdx` | `sword_2h_artifactsoulrend_d_02_blue` | ✔ 正常 |
| 420991 | 双手剑 | 64205 | 灵魂撕裂者D02 绿 | `sword_2h_artifactsoulrend_d_02.mdx` | `sword_2h_artifactsoulrend_d_02_green` | ✔ 正常 |
| 420992 | 双手剑 | 64206 | 灵魂撕裂者D02 橙 | `sword_2h_artifactsoulrend_d_02.mdx` | `sword_2h_artifactsoulrend_d_02_orange` | ✔ 正常 |
| 420993 | 双手剑 | 64207 | 灵魂撕裂者D02 红 | `sword_2h_artifactsoulrend_d_02.mdx` | `sword_2h_artifactsoulrend_d_02_red` | ✔ 正常 |
| 420994 | 双手剑 | 64208 | 灵魂撕裂者D03 蓝 | `sword_2h_artifactsoulrend_d_03.mdx` | `sword_2h_artifactsoulrend_d_03_blue` | ✔ 正常 |
| 420995 | 双手剑 | 64209 | 灵魂撕裂者D03 红 | `sword_2h_artifactsoulrend_d_03.mdx` | `sword_2h_artifactsoulrend_d_03_red` | ✔ 正常 |
| 420996 | 双手剑 | 64210 | 灵魂撕裂者D03 青 | `sword_2h_artifactsoulrend_d_03.mdx` | `sword_2h_artifactsoulrend_d_03_teal` | ✔ 正常 |
| 420997 | 双手剑 | 64211 | 灵魂撕裂者D03 黄 | `sword_2h_artifactsoulrend_d_03.mdx` | `sword_2h_artifactsoulrend_d_03_yellow` | ✔ 正常 |
| 420998 | 双手剑 | 64212 | 灵魂撕裂者D04 水蓝 | `sword_2h_artifactsoulrend_d_04.mdx` | `sword_2h_artifactsoulrend_d_04_aqua` | ✔ 正常 |
| 420999 | 双手剑 | 64213 | 灵魂撕裂者D04 蓝 | `sword_2h_artifactsoulrend_d_04.mdx` | `sword_2h_artifactsoulrend_d_04_blue` | ✔ 正常 |
| 421000 | 双手剑 | 64214 | 灵魂撕裂者D04 绿 | `sword_2h_artifactsoulrend_d_04.mdx` | `sword_2h_artifactsoulrend_d_04_green` | ✔ 正常 |
| 421001 | 双手剑 | 64215 | 灵魂撕裂者D04 红 | `sword_2h_artifactsoulrend_d_04.mdx` | `sword_2h_artifactsoulrend_d_04_red` | ✔ 正常 |
| 421002 | 双手剑 | 64216 | 灵魂撕裂者D04 白 | `sword_2h_artifactsoulrend_d_04.mdx` | `sword_2h_artifactsoulrend_d_04_white` | ✔ 正常 |
| 421003 | 双手剑 | 61033, 102854, 104054 | 灵魂撕裂者D05 蓝 | `sword_2h_artifactsoulrend_d_05.mdx` | `sword_2h_artifactsoulrend_d_05_blue` | ◑ 已使用 |
| 421004 | 双手剑 | 61034, 102855, 104055 | 灵魂撕裂者D05 紫 | `sword_2h_artifactsoulrend_d_05.mdx` | `sword_2h_artifactsoulrend_d_05_purple` | ◑ 已使用 |
| 421005 | 双手剑 | 61035, 102856, 104056 | 灵魂撕裂者D05 红 | `sword_2h_artifactsoulrend_d_05.mdx` | `sword_2h_artifactsoulrend_d_05_red` | ◑ 已使用 |
| 421006 | 双手剑 | 64217 | 灵魂撕裂者D05 青 | `sword_2h_artifactsoulrend_d_05.mdx` | `sword_2h_artifactsoulrend_d_05_teal` | ✔ 正常 |
| 421007 | 双手剑 | 61030, 99954, 100531, 101124, 101701, 102278, 103478 | 灵魂撕裂者D06 蓝 | `sword_2h_artifactsoulrend_d_06.mdx` | `sword_2h_artifactsoulrend_d_06_blue` | ◑ 已使用 |
| 421008 | 双手剑 | 64218 | 灵魂撕裂者D06 绿 | `sword_2h_artifactsoulrend_d_06.mdx` | `sword_2h_artifactsoulrend_d_06_green` | ✔ 正常 |
| 421009 | 双手剑 | 61031, 99955, 100532, 101125, 101702, 102279, 103479 | 灵魂撕裂者D06 红 | `sword_2h_artifactsoulrend_d_06.mdx` | `sword_2h_artifactsoulrend_d_06_red` | ◑ 已使用 |
| 421010 | 双手剑 | 61032, 102853, 104053 | 灵魂撕裂者D06 黄 | `sword_2h_artifactsoulrend_d_06.mdx` | `sword_2h_artifactsoulrend_d_06_yellow` | ◑ 已使用 |
| 100002 | 双手剑 | 64219 | 德拉诺挑战D01 红 | `sword_2h_draenorchallenge_d_01.mdx` | `sword_2h_draenorchallenge_d_01_red` | ✔ 正常 |
| 100001 | 双手剑 | 64220 | 德拉诺团队D01 绿 | `sword_2h_draenorraid_d_01.mdx` | `sword_2h_draenorraid_d_01green` | ✔ 正常 |
| 100006 | 双手剑 | 93001 | 亚瑟·王者之剑 | `yasewuqi.mdx` | — | ◑ 已使用 |

## 三、双手斧（14 条：已使用 2 / 新生成 12）

> `class=2, subclass=1, InventoryType=17, sheath=1, Material=1`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420014 | 双手斧 | 47285 | 玛乌D03 红 | `axe_2h_artifactmaw_d_03.mdx` | `axe_2h_artifactmaw_d_03red` | ◑ 已使用 |
| 420015 | 双手斧 | 47069 | 玛乌D03 青 | `axe_2h_artifactmaw_d_03.mdx` | `axe_2h_artifactmaw_d_03teal` | ◑ 已使用 |
| 420016 | 双手斧 | 64221 | 玛乌D03 白蓝 | `axe_2h_artifactmaw_d_03.mdx` | `axe_2h_artifactmaw_d_03whiteblue` | ✔ 正常 |
| 420017 | 双手斧 | 64222 | 玛乌D04 蓝 | `axe_2h_artifactmaw_d_04.mdx` | `axe_2h_artifactmaw_d_04blue` | ✔ 正常 |
| 420018 | 双手斧 | 64223 | 玛乌D04 绿 | `axe_2h_artifactmaw_d_04.mdx` | `axe_2h_artifactmaw_d_04green` | ✔ 正常 |
| 420019 | 双手斧 | 64224 | 玛乌D04 红 | `axe_2h_artifactmaw_d_04.mdx` | `axe_2h_artifactmaw_d_04red` | ✔ 正常 |
| 420020 | 双手斧 | 64225 | 玛乌D04 白 | `axe_2h_artifactmaw_d_04.mdx` | `axe_2h_artifactmaw_d_04white` | ✔ 正常 |
| 420021 | 双手斧 | 64226 | 玛乌D05 蓝 | `axe_2h_artifactmaw_d_05.mdx` | `axe_2h_artifactmaw_d_05blue` | ✔ 正常 |
| 420022 | 双手斧 | 64227 | 玛乌D05 紫 | `axe_2h_artifactmaw_d_05.mdx` | `axe_2h_artifactmaw_d_05purple` | ✔ 正常 |
| 420023 | 双手斧 | 64228 | 玛乌D05 红 | `axe_2h_artifactmaw_d_05.mdx` | `axe_2h_artifactmaw_d_05red` | ✔ 正常 |
| 420024 | 双手斧 | 64229 | 玛乌D05 白 | `axe_2h_artifactmaw_d_05.mdx` | `axe_2h_artifactmaw_d_05white` | ✔ 正常 |
| 420025 | 双手斧 | 64230 | 玛乌D06 绿 | `axe_2h_artifactmaw_d_06.mdx` | `axe_2h_artifactmaw_d_06green` | ✔ 正常 |
| 420026 | 双手斧 | 64231 | 玛乌D06 紫 | `axe_2h_artifactmaw_d_06.mdx` | `axe_2h_artifactmaw_d_06purple` | ✔ 正常 |
| 420027 | 双手斧 | 64232 | 玛乌D06 红 | `axe_2h_artifactmaw_d_06.mdx` | `axe_2h_artifactmaw_d_06red` | ✔ 正常 |

## 四、单手锤（68 条：已使用 3 / 新生成 65）

> `class=2, subclass=4, InventoryType=13, sheath=3, Material=2`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420276 | 单手锤 | 64233 | 艾萨拉D01 蓝 | `mace_1h_artifactazshara_d_01.mdx` | `mace_1h_artifactazshara_d_01blue` | ✔ 正常 |
| 420277 | 单手锤 | 64234 | 艾萨拉D01 绿 | `mace_1h_artifactazshara_d_01.mdx` | `mace_1h_artifactazshara_d_01green` | ✔ 正常 |
| 420278 | 单手锤 | 50028 | 艾萨拉D01 橙 | `mace_1h_artifactazshara_d_01.mdx` | `mace_1h_artifactazshara_d_01orange` | ◑ 已使用 |
| 420279 | 单手锤 | 64235 | 艾萨拉D01 紫 | `mace_1h_artifactazshara_d_01.mdx` | `mace_1h_artifactazshara_d_01purple` | ✔ 正常 |
| 420280 | 单手锤 | 64236 | 艾萨拉D02 蓝 | `mace_1h_artifactazshara_d_02.mdx` | `mace_1h_artifactazshara_d_02blue` | ✔ 正常 |
| 420281 | 单手锤 | 64237 | 艾萨拉D02 深蓝 | `mace_1h_artifactazshara_d_02.mdx` | `mace_1h_artifactazshara_d_02deepblue` | ✔ 正常 |
| 420282 | 单手锤 | 64238 | 艾萨拉D02 紫 | `mace_1h_artifactazshara_d_02.mdx` | `mace_1h_artifactazshara_d_02purple` | ✔ 正常 |
| 420283 | 单手锤 | 64239 | 艾萨拉D02 黄 | `mace_1h_artifactazshara_d_02.mdx` | `mace_1h_artifactazshara_d_02yellow` | ✔ 正常 |
| 420284 | 单手锤 | 64240 | 艾萨拉D03 蓝 | `mace_1h_artifactazshara_d_03.mdx` | `mace_1h_artifactazshara_d_03blue` | ✔ 正常 |
| 420285 | 单手锤 | 64241 | 艾萨拉D03 绿 | `mace_1h_artifactazshara_d_03.mdx` | `mace_1h_artifactazshara_d_03green` | ✔ 正常 |
| 420286 | 单手锤 | 64242 | 艾萨拉D03 紫 | `mace_1h_artifactazshara_d_03.mdx` | `mace_1h_artifactazshara_d_03purple` | ✔ 正常 |
| 420287 | 单手锤 | 64243 | 艾萨拉D03 红 | `mace_1h_artifactazshara_d_03.mdx` | `mace_1h_artifactazshara_d_03red` | ✔ 正常 |
| 420288 | 单手锤 | 64244 | 艾萨拉D04 绿 | `mace_1h_artifactazshara_d_04.mdx` | `mace_1h_artifactazshara_d_04green` | ✔ 正常 |
| 420289 | 单手锤 | 64245 | 艾萨拉D04 橙 | `mace_1h_artifactazshara_d_04.mdx` | `mace_1h_artifactazshara_d_04orange` | ✔ 正常 |
| 420290 | 单手锤 | 64246 | 艾萨拉D04 红 | `mace_1h_artifactazshara_d_04.mdx` | `mace_1h_artifactazshara_d_04red` | ✔ 正常 |
| 420291 | 单手锤 | 51798 | 艾萨拉D04 黄 | `mace_1h_artifactazshara_d_04.mdx` | `mace_1h_artifactazshara_d_04yellow` | ◑ 已使用 |
| 420292 | 单手锤 | 64247 | 艾萨拉D05 蓝 | `mace_1h_artifactazshara_d_05.mdx` | `mace_1h_artifactazshara_d_05blue` | ✔ 正常 |
| 420293 | 单手锤 | 64248 | 艾萨拉D05 绿 | `mace_1h_artifactazshara_d_05.mdx` | `mace_1h_artifactazshara_d_05green` | ✔ 正常 |
| 420294 | 单手锤 | 64249 | 艾萨拉D05 红 | `mace_1h_artifactazshara_d_05.mdx` | `mace_1h_artifactazshara_d_05red` | ✔ 正常 |
| 420295 | 单手锤 | 64250 | 艾萨拉D05 黄 | `mace_1h_artifactazshara_d_05.mdx` | `mace_1h_artifactazshara_d_05yellow` | ✔ 正常 |
| 420296 | 单手锤 | 64251 | 艾萨拉D06 蓝 | `mace_1h_artifactazshara_d_06.mdx` | `mace_1h_artifactazshara_d_06_blue` | ✔ 正常 |
| 420297 | 单手锤 | 64252 | 艾萨拉D06 绿 | `mace_1h_artifactazshara_d_06.mdx` | `mace_1h_artifactazshara_d_06_green` | ✔ 正常 |
| 420298 | 单手锤 | 64253 | 艾萨拉D06 光 | `mace_1h_artifactazshara_d_06.mdx` | `mace_1h_artifactazshara_d_06_light` | ✔ 正常 |
| 420299 | 单手锤 | 64254 | 艾萨拉D06 红 | `mace_1h_artifactazshara_d_06.mdx` | `mace_1h_artifactazshara_d_06_red` | ✔ 正常 |
| 420300 | 单手锤 | 64255 | 毁灭之锤D01 原版 | `mace_1h_artifactdoomhammer_d_01.mdx` | `mace_1h_artifactdoomhammer_d_01_base` | ✔ 正常 |
| 420301 | 单手锤 | 64256 | 毁灭之锤D01 暗 | `mace_1h_artifactdoomhammer_d_01.mdx` | `mace_1h_artifactdoomhammer_d_01_dark` | ✔ 正常 |
| 420302 | 单手锤 | 64257 | 毁灭之锤D01 金 | `mace_1h_artifactdoomhammer_d_01.mdx` | `mace_1h_artifactdoomhammer_d_01_gold` | ✔ 正常 |
| 420303 | 单手锤 | 64258 | 毁灭之锤D01 绿 | `mace_1h_artifactdoomhammer_d_01.mdx` | `mace_1h_artifactdoomhammer_d_01_green` | ✔ 正常 |
| 420304 | 单手锤 | 64259 | 毁灭之锤D02_1 | `Mace_1H_ArtifactDoomhammer_D_02_1.mdx` | — | ✔ 正常 |
| 420305 | 单手锤 | 64260 | 毁灭之锤D02_2 | `Mace_1H_ArtifactDoomhammer_D_02_2.mdx` | — | ✔ 正常 |
| 420306 | 单手锤 | 64261 | 毁灭之锤D02_3 | `Mace_1H_ArtifactDoomhammer_D_02_3.mdx` | — | ✔ 正常 |
| 420307 | 单手锤 | 64262 | 毁灭之锤D02_4 | `Mace_1H_ArtifactDoomhammer_D_02_4.mdx` | — | ✔ 正常 |
| 420308 | 单手锤 | 64263 | 毁灭之锤D03 绿 | `mace_1h_artifactdoomhammer_d_03.mdx` | `mace_1h_artifactdoomhammer_d_03green` | ✔ 正常 |
| 420309 | 单手锤 | 64264 | 毁灭之锤D03 蓝 | `mace_1h_artifactdoomhammer_d_03.mdx` | `mace_1h_artifactdoomhammer_d_03_blue` | ✔ 正常 |
| 420310 | 单手锤 | 64265 | 毁灭之锤D03 绿 | `mace_1h_artifactdoomhammer_d_03.mdx` | `mace_1h_artifactdoomhammer_d_03_green` | ✔ 正常 |
| 420311 | 单手锤 | 64266 | 毁灭之锤D03 橙 | `mace_1h_artifactdoomhammer_d_03.mdx` | `mace_1h_artifactdoomhammer_d_03_orange` | ✔ 正常 |
| 420312 | 单手锤 | 64267 | 毁灭之锤D03 紫 | `mace_1h_artifactdoomhammer_d_03.mdx` | `mace_1h_artifactdoomhammer_d_03_purple` | ✔ 正常 |
| 420313 | 单手锤 | 64268 | 毁灭之锤D04 红 | `mace_1h_artifactdoomhammer_d_04.mdx` | `mace_1h_artifactdoomhammer_d_04red` | ✔ 正常 |
| 420314 | 单手锤 | 64269 | 毁灭之锤D04 红 蓝 | `mace_1h_artifactdoomhammer_d_04.mdx` | `mace_1h_artifactdoomhammer_d_04red_blue` | ✔ 正常 |
| 420315 | 单手锤 | 64270 | 毁灭之锤D04 红 绿 | `mace_1h_artifactdoomhammer_d_04.mdx` | `mace_1h_artifactdoomhammer_d_04red_green` | ✔ 正常 |
| 420316 | 单手锤 | 64271 | 毁灭之锤D04 红 紫 | `mace_1h_artifactdoomhammer_d_04.mdx` | `mace_1h_artifactdoomhammer_d_04red_purple` | ✔ 正常 |
| 420317 | 单手锤 | 64272 | 毁灭之锤D04 红 红 | `mace_1h_artifactdoomhammer_d_04.mdx` | `mace_1h_artifactdoomhammer_d_04red_red` | ✔ 正常 |
| 420318 | 单手锤 | 64273 | 毁灭之锤D04 蓝 | `mace_1h_artifactdoomhammer_d_04.mdx` | `mace_1h_artifactdoomhammer_d_04_blue` | ✔ 正常 |
| 420319 | 单手锤 | 64274 | 毁灭之锤D04 绿 | `mace_1h_artifactdoomhammer_d_04.mdx` | `mace_1h_artifactdoomhammer_d_04_green` | ✔ 正常 |
| 420320 | 单手锤 | 64275 | 毁灭之锤D04 紫 | `mace_1h_artifactdoomhammer_d_04.mdx` | `mace_1h_artifactdoomhammer_d_04_purple` | ✔ 正常 |
| 420321 | 单手锤 | 64276 | 毁灭之锤D04 红 | `mace_1h_artifactdoomhammer_d_04.mdx` | `mace_1h_artifactdoomhammer_d_04_red` | ✔ 正常 |
| 420322 | 单手锤 | 64277 | 毁灭之锤D05 蓝 | `mace_1h_artifactdoomhammer_d_05.mdx` | `mace_1h_artifactdoomhammer_d_05_blue` | ✔ 正常 |
| 420323 | 单手锤 | 64278 | 毁灭之锤D05 金 | `mace_1h_artifactdoomhammer_d_05.mdx` | `mace_1h_artifactdoomhammer_d_05_gold` | ✔ 正常 |
| 420324 | 单手锤 | 64279 | 毁灭之锤D05 绿 | `mace_1h_artifactdoomhammer_d_05.mdx` | `mace_1h_artifactdoomhammer_d_05_green` | ✔ 正常 |
| 420325 | 单手锤 | 64280 | 毁灭之锤D05 白 | `mace_1h_artifactdoomhammer_d_05.mdx` | `mace_1h_artifactdoomhammer_d_05_white` | ✔ 正常 |
| 420326 | 单手锤 | 64281 | 毁灭之锤D06 蓝 | `mace_1h_artifactdoomhammer_d_06.mdx` | `mace_1h_artifactdoomhammer_d_06_blue` | ✔ 正常 |
| 420327 | 单手锤 | 110001, 110011 | 毁灭之锤D06 橙 | `mace_1h_artifactdoomhammer_d_06.mdx` | `mace_1h_artifactdoomhammer_d_06_orange` | ◑ 已使用 |
| 420328 | 单手锤 | 64282 | 毁灭之锤D06 紫 | `mace_1h_artifactdoomhammer_d_06.mdx` | `mace_1h_artifactdoomhammer_d_06_purple` | ✔ 正常 |
| 420329 | 单手锤 | 64283 | 毁灭之锤D06 青 | `mace_1h_artifactdoomhammer_d_06.mdx` | `mace_1h_artifactdoomhammer_d_06_teal` | ✔ 正常 |
| 420338 | 单手锤 | 64284 | 玛格纳D05 蓝 | `mace_1h_artifactmagnar_d_05.mdx` | `mace_1h_artifactmagnar_d_05_blue` | ✔ 正常 |
| 420339 | 单手锤 | 64285 | 玛格纳D05 绿 | `mace_1h_artifactmagnar_d_05.mdx` | `mace_1h_artifactmagnar_d_05_green` | ✔ 正常 |
| 420340 | 单手锤 | 64286 | 玛格纳D05 橙 | `mace_1h_artifactmagnar_d_05.mdx` | `mace_1h_artifactmagnar_d_05_orange` | ✔ 正常 |
| 420341 | 单手锤 | 64287 | 玛格纳D05 青 | `mace_1h_artifactmagnar_d_05.mdx` | `mace_1h_artifactmagnar_d_05_teal` | ✔ 正常 |
| 420330 | 单手锤 | 64288 | 诺甘农D04 蓝 | `mace_1h_artifactnorgannon_d_04.mdx` | `mace_1h_artifactnorgannon_d_04_blue` | ✔ 正常 |
| 420331 | 单手锤 | 64289 | 诺甘农D04 紫 | `mace_1h_artifactnorgannon_d_04.mdx` | `mace_1h_artifactnorgannon_d_04_purple` | ✔ 正常 |
| 420332 | 单手锤 | 64290 | 诺甘农D04 红 | `mace_1h_artifactnorgannon_d_04.mdx` | `mace_1h_artifactnorgannon_d_04_red` | ✔ 正常 |
| 420333 | 单手锤 | 64291 | 诺甘农D04 黄 | `mace_1h_artifactnorgannon_d_04.mdx` | `mace_1h_artifactnorgannon_d_04_yellow` | ✔ 正常 |
| 420334 | 单手锤 | 64292 | 诺甘农D06 蓝 | `mace_1h_artifactnorgannon_d_06.mdx` | `mace_1h_artifactnorgannon_d_06_blue` | ✔ 正常 |
| 420335 | 单手锤 | 64293 | 诺甘农D06 绿 | `mace_1h_artifactnorgannon_d_06.mdx` | `mace_1h_artifactnorgannon_d_06_green` | ✔ 正常 |
| 420336 | 单手锤 | 64294 | 诺甘农D06 红 | `mace_1h_artifactnorgannon_d_06.mdx` | `mace_1h_artifactnorgannon_d_06_red` | ✔ 正常 |
| 420337 | 单手锤 | 64295 | 诺甘农D06 黄 | `mace_1h_artifactnorgannon_d_06.mdx` | `mace_1h_artifactnorgannon_d_06_yellow` | ✔ 正常 |
| 420342 | 单手锤 | 64296 | 德拉诺挑战D01 蓝 亮 | `mace_1h_draenorchallenge_d_01.mdx` | `mace_1h_draenorchallenge_d_01_blue_bright` | ✔ 正常 |
| 420343 | 单手锤 | 64297 | 德拉诺挑战D02 特效 | `mace_1h_draenorchallenge_d_02.mdx` | `mace_1h_draenorchallenge_d_02_fx` | ✔ 正常 |

## 五、双手锤（25 条：已使用 2 / 新生成 23）

> `class=2, subclass=5, InventoryType=17, sheath=1, Material=2`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420344 | 双手锤 | 64298 | 圣光之锤D01 绿 | `mace_2h_artifactsilverhand_d_01.mdx` | `mace_2h_artifactsilverhand_d_01green` | ✔ 正常 |
| 420345 | 双手锤 | 64299 | 圣光之锤D01 紫 | `mace_2h_artifactsilverhand_d_01.mdx` | `mace_2h_artifactsilverhand_d_01purple` | ✔ 正常 |
| 420346 | 双手锤 | 64300 | 圣光之锤D01 红 | `mace_2h_artifactsilverhand_d_01.mdx` | `mace_2h_artifactsilverhand_d_01red` | ✔ 正常 |
| 420347 | 双手锤 | 51796 | 圣光之锤D01 黄 | `mace_2h_artifactsilverhand_d_01.mdx` | `mace_2h_artifactsilverhand_d_01yellow` | ◑ 已使用 |
| 420348 | 双手锤 | 64301 | 圣光之锤D02 蓝 | `mace_2h_artifactsilverhand_d_02.mdx` | `mace_2h_artifactsilverhand_d_02blue` | ✔ 正常 |
| 420349 | 双手锤 | 64302 | 圣光之锤D02 紫 | `mace_2h_artifactsilverhand_d_02.mdx` | `mace_2h_artifactsilverhand_d_02purple` | ✔ 正常 |
| 420350 | 双手锤 | 64303 | 圣光之锤D02 红 | `mace_2h_artifactsilverhand_d_02.mdx` | `mace_2h_artifactsilverhand_d_02red` | ✔ 正常 |
| 420351 | 双手锤 | 64304 | 圣光之锤D02 黄 | `mace_2h_artifactsilverhand_d_02.mdx` | `mace_2h_artifactsilverhand_d_02yellow` | ✔ 正常 |
| 420352 | 双手锤 | 49919 | 圣光之锤D03 蓝 | `mace_2h_artifactsilverhand_d_03.mdx` | `mace_2h_artifactsilverhand_d_03blue` | ◑ 已使用 |
| 420353 | 双手锤 | 64305 | 圣光之锤D03 绿 | `mace_2h_artifactsilverhand_d_03.mdx` | `mace_2h_artifactsilverhand_d_03green` | ✔ 正常 |
| 420354 | 双手锤 | 64306 | 圣光之锤D03 紫 | `mace_2h_artifactsilverhand_d_03.mdx` | `mace_2h_artifactsilverhand_d_03purple` | ✔ 正常 |
| 420355 | 双手锤 | 64307 | 圣光之锤D03 红 | `mace_2h_artifactsilverhand_d_03.mdx` | `mace_2h_artifactsilverhand_d_03red` | ✔ 正常 |
| 420356 | 双手锤 | 64308 | 圣光之锤D04 蓝 | `mace_2h_artifactsilverhand_d_04.mdx` | `mace_2h_artifactsilverhand_d_04blue` | ✔ 正常 |
| 420357 | 双手锤 | 64309 | 圣光之锤D04 绿 | `mace_2h_artifactsilverhand_d_04.mdx` | `mace_2h_artifactsilverhand_d_04green` | ✔ 正常 |
| 420358 | 双手锤 | 64310 | 圣光之锤D04 紫 | `mace_2h_artifactsilverhand_d_04.mdx` | `mace_2h_artifactsilverhand_d_04purple` | ✔ 正常 |
| 420359 | 双手锤 | 64311 | 圣光之锤D04 红 | `mace_2h_artifactsilverhand_d_04.mdx` | `mace_2h_artifactsilverhand_d_04red` | ✔ 正常 |
| 420360 | 双手锤 | 64312 | 圣光之锤D05 蓝 | `mace_2h_artifactsilverhand_d_05.mdx` | `mace_2h_artifactsilverhand_d_05blue` | ✔ 正常 |
| 420361 | 双手锤 | 64313 | 圣光之锤D05 紫 | `mace_2h_artifactsilverhand_d_05.mdx` | `mace_2h_artifactsilverhand_d_05purplr` | ✔ 正常 |
| 420362 | 双手锤 | 64314 | 圣光之锤D05 红 | `mace_2h_artifactsilverhand_d_05.mdx` | `mace_2h_artifactsilverhand_d_05red` | ✔ 正常 |
| 420363 | 双手锤 | 64315 | 圣光之锤D05 黄 | `mace_2h_artifactsilverhand_d_05.mdx` | `mace_2h_artifactsilverhand_d_05yellow` | ✔ 正常 |
| 420364 | 双手锤 | 64316 | 圣光之锤D06 | `mace_2h_artifactsilverhand_d_06.mdx` | `mace_2h_artifactsilverhand_d_06` | ✔ 正常 |
| 420365 | 双手锤 | 64317 | 圣光之锤D06 金 | `mace_2h_artifactsilverhand_d_06.mdx` | `mace_2h_artifactsilverhand_d_06gold` | ✔ 正常 |
| 420366 | 双手锤 | 64318 | 圣光之锤D06 紫 | `mace_2h_artifactsilverhand_d_06.mdx` | `mace_2h_artifactsilverhand_d_06purple` | ✔ 正常 |
| 420367 | 双手锤 | 64319 | 圣光之锤D06 红 | `mace_2h_artifactsilverhand_d_06.mdx` | `mace_2h_artifactsilverhand_d_06red` | ✔ 正常 |
| 420368 | 双手锤 | 64320 | 德拉诺挑战D01 激活 | `mace_2h_draenorchallenge_d_01.mdx` | `mace_2h_draenorchallenge_d_01_active` | ✔ 正常 |

## 六、长柄武器（29 条：已使用 2 / 新生成 27）

> `class=2, subclass=6, InventoryType=17, sheath=1, Material=1`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 100004 | 长柄武器 | 64321 | 吕布·方天画戟 | `lvbu1wq.mdx` | — | ✔ 正常 |
| 420446 | 长柄武器 | 64322 | 雄鹰D01 蓝 | `polearm_2h_artifacteagle_d_01.mdx` | `polearm_2h_artifacteagle_d_01_blue` | ✔ 正常 |
| 420447 | 长柄武器 | 64323 | 雄鹰D01 绿 | `polearm_2h_artifacteagle_d_01.mdx` | `polearm_2h_artifacteagle_d_01_green` | ✔ 正常 |
| 420448 | 长柄武器 | 64324 | 雄鹰D01 红 | `polearm_2h_artifacteagle_d_01.mdx` | `polearm_2h_artifacteagle_d_01_red` | ✔ 正常 |
| 420449 | 长柄武器 | 64325 | 雄鹰D01 棕褐 | `polearm_2h_artifacteagle_d_01.mdx` | `polearm_2h_artifacteagle_d_01_tan` | ✔ 正常 |
| 420450 | 长柄武器 | 64326 | 雄鹰D02 蓝 | `polearm_2h_artifacteagle_d_02.mdx` | `polearm_2h_artifacteagle_d_02_blue` | ✔ 正常 |
| 420451 | 长柄武器 | 64327 | 雄鹰D02 绿 | `polearm_2h_artifacteagle_d_02.mdx` | `polearm_2h_artifacteagle_d_02_green` | ✔ 正常 |
| 420452 | 长柄武器 | 64328 | 雄鹰D02 红 | `polearm_2h_artifacteagle_d_02.mdx` | `polearm_2h_artifacteagle_d_02_red` | ✔ 正常 |
| 420453 | 长柄武器 | 64329 | 雄鹰D02 棕褐 | `polearm_2h_artifacteagle_d_02.mdx` | `polearm_2h_artifacteagle_d_02_tan` | ✔ 正常 |
| 420454 | 长柄武器 | 64330 | 雄鹰D03 蓝 | `polearm_2h_artifacteagle_d_03.mdx` | `polearm_2h_artifacteagle_d_03_blue` | ✔ 正常 |
| 420455 | 长柄武器 | 64331 | 雄鹰D03 绿 | `polearm_2h_artifacteagle_d_03.mdx` | `polearm_2h_artifacteagle_d_03_green` | ✔ 正常 |
| 420456 | 长柄武器 | 64332 | 雄鹰D03 红 | `polearm_2h_artifacteagle_d_03.mdx` | `polearm_2h_artifacteagle_d_03_red` | ✔ 正常 |
| 420457 | 长柄武器 | 64333 | 雄鹰D03 青 | `polearm_2h_artifacteagle_d_03.mdx` | `polearm_2h_artifacteagle_d_03_teal` | ✔ 正常 |
| 420458 | 长柄武器 | 50425 | 雄鹰D04 蓝 | `polearm_2h_artifacteagle_d_04.mdx` | `polearm_2h_artifacteagle_d_04_blue` | ◑ 已使用 |
| 420459 | 长柄武器 | 64334 | 雄鹰D04 绿 | `polearm_2h_artifacteagle_d_04.mdx` | `polearm_2h_artifacteagle_d_04_green` | ✔ 正常 |
| 420460 | 长柄武器 | 64335 | 雄鹰D04 紫 | `polearm_2h_artifacteagle_d_04.mdx` | `polearm_2h_artifacteagle_d_04_purple` | ✔ 正常 |
| 420461 | 长柄武器 | 64336 | 雄鹰D04 红 | `polearm_2h_artifacteagle_d_04.mdx` | `polearm_2h_artifacteagle_d_04_red` | ✔ 正常 |
| 420462 | 长柄武器 | 64337 | 雄鹰D05 蓝 | `polearm_2h_artifacteagle_d_05.mdx` | `polearm_2h_artifacteagle_d_05_blue` | ✔ 正常 |
| 420463 | 长柄武器 | 64338 | 雄鹰D05 绿 | `polearm_2h_artifacteagle_d_05.mdx` | `polearm_2h_artifacteagle_d_05_green` | ✔ 正常 |
| 420464 | 长柄武器 | 64339 | 雄鹰D05 紫 | `polearm_2h_artifacteagle_d_05.mdx` | `polearm_2h_artifacteagle_d_05_purple` | ✔ 正常 |
| 420465 | 长柄武器 | 64340 | 雄鹰D05 红 | `polearm_2h_artifacteagle_d_05.mdx` | `polearm_2h_artifacteagle_d_05_red` | ✔ 正常 |
| 420466 | 长柄武器 | 64341 | 雄鹰D06 蓝 | `polearm_2h_artifacteagle_d_06.mdx` | `polearm_2h_artifacteagle_d_06_blue` | ✔ 正常 |
| 420467 | 长柄武器 | 64342 | 雄鹰D06 粉 | `polearm_2h_artifacteagle_d_06.mdx` | `polearm_2h_artifacteagle_d_06_pink` | ✔ 正常 |
| 420468 | 长柄武器 | 64343 | 雄鹰D06 紫 | `polearm_2h_artifacteagle_d_06.mdx` | `polearm_2h_artifacteagle_d_06_purple` | ✔ 正常 |
| 420469 | 长柄武器 | 64344 | 雄鹰D06 红 | `polearm_2h_artifacteagle_d_06.mdx` | `polearm_2h_artifacteagle_d_06_red` | ✔ 正常 |
| 100012 | 长柄武器 | 50178 | 泰坦阿古斯D01 红 | `polearm_2h_titanargus_d_01.mdx` | `polearm_2h_titanargus_d_01red` | ✔※mpq |
| 100013 | 长柄武器 | 64345 | 泰坦阿古斯D01 绿 | `polearm_2h_titanargus_d_01.mdx` | `polearm_2h_titanargus_d_01green` | ✔※mpq |
| 100014 | 长柄武器 | 64346 | 泰坦阿古斯D01 品红 | `polearm_2h_titanargus_d_01.mdx` | `polearm_2h_titanargus_d_01magenta` | ✔※mpq |
| 100015 | 长柄武器 | 64347 | 泰坦阿古斯D01 | `polearm_2h_titanargus_d_01.mdx` | `polearm_2h_titanargus_d_01` | ✔※mpq |

## 七、法杖（244 条：已使用 9 / 新生成 235）

> `class=2, subclass=10, InventoryType=17, sheath=2, Material=2`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420577 | 法杖 | 64348 | 艾格文之杖D01 蓝 | `staff_2h_artifactaegwynsstaff_d_01.mdx` | `staff_2h_artifactaegwynsstaff_d_01blue` | ✔ 正常 |
| 420578 | 法杖 | 64349 | 艾格文之杖D01 绿 | `staff_2h_artifactaegwynsstaff_d_01.mdx` | `staff_2h_artifactaegwynsstaff_d_01green` | ✔ 正常 |
| 420579 | 法杖 | 64350 | 艾格文之杖D01 紫 | `staff_2h_artifactaegwynsstaff_d_01.mdx` | `staff_2h_artifactaegwynsstaff_d_01purple` | ✔ 正常 |
| 420580 | 法杖 | 64351 | 艾格文之杖D01 白 | `staff_2h_artifactaegwynsstaff_d_01.mdx` | `staff_2h_artifactaegwynsstaff_d_01white` | ✔ 正常 |
| 420581 | 法杖 | 64352 | 艾格文之杖D02 蓝 | `staff_2h_artifactaegwynsstaff_d_02.mdx` | `staff_2h_artifactaegwynsstaff_d_02blue` | ✔ 正常 |
| 420582 | 法杖 | 64353 | 艾格文之杖D02 绿 | `staff_2h_artifactaegwynsstaff_d_02.mdx` | `staff_2h_artifactaegwynsstaff_d_02green` | ✔ 正常 |
| 420583 | 法杖 | 64354 | 艾格文之杖D02 紫 | `staff_2h_artifactaegwynsstaff_d_02.mdx` | `staff_2h_artifactaegwynsstaff_d_02purple` | ✔ 正常 |
| 420584 | 法杖 | 64355 | 艾格文之杖D02 白 | `staff_2h_artifactaegwynsstaff_d_02.mdx` | `staff_2h_artifactaegwynsstaff_d_02white` | ✔ 正常 |
| 420585 | 法杖 | 64356 | 艾格文之杖D03 蓝 | `staff_2h_artifactaegwynsstaff_d_03.mdx` | `staff_2h_artifactaegwynsstaff_d_03blue` | ✔ 正常 |
| 420586 | 法杖 | 64357 | 艾格文之杖D03 绿 | `staff_2h_artifactaegwynsstaff_d_03.mdx` | `staff_2h_artifactaegwynsstaff_d_03green` | ✔ 正常 |
| 420587 | 法杖 | 64358 | 艾格文之杖D03 紫 | `staff_2h_artifactaegwynsstaff_d_03.mdx` | `staff_2h_artifactaegwynsstaff_d_03purple` | ✔ 正常 |
| 420588 | 法杖 | 64359 | 艾格文之杖D03 白 | `staff_2h_artifactaegwynsstaff_d_03.mdx` | `staff_2h_artifactaegwynsstaff_d_03white` | ✔ 正常 |
| 420589 | 法杖 | 64360 | 艾格文之杖D04 蓝 | `staff_2h_artifactaegwynsstaff_d_04.mdx` | `staff_2h_artifactaegwynsstaff_d_04blue` | ✔ 正常 |
| 420590 | 法杖 | 64361 | 艾格文之杖D04 绿 | `staff_2h_artifactaegwynsstaff_d_04.mdx` | `staff_2h_artifactaegwynsstaff_d_04green` | ✔ 正常 |
| 420591 | 法杖 | 64362 | 艾格文之杖D04 粉 | `staff_2h_artifactaegwynsstaff_d_04.mdx` | `staff_2h_artifactaegwynsstaff_d_04pink` | ✔ 正常 |
| 420592 | 法杖 | 64363 | 艾格文之杖D04 紫 | `staff_2h_artifactaegwynsstaff_d_04.mdx` | `staff_2h_artifactaegwynsstaff_d_04purple` | ✔ 正常 |
| 420593 | 法杖 | 64364 | 艾格文之杖D05 蓝 | `staff_2h_artifactaegwynsstaff_d_05.mdx` | `staff_2h_artifactaegwynsstaff_d_05blue` | ✔ 正常 |
| 420594 | 法杖 | 64365 | 艾格文之杖D05 铜 | `staff_2h_artifactaegwynsstaff_d_05.mdx` | `staff_2h_artifactaegwynsstaff_d_05copper` | ✔ 正常 |
| 420595 | 法杖 | 64366 | 艾格文之杖D05 金 | `staff_2h_artifactaegwynsstaff_d_05.mdx` | `staff_2h_artifactaegwynsstaff_d_05gold` | ✔ 正常 |
| 420596 | 法杖 | 64367 | 艾格文之杖D05 银 | `staff_2h_artifactaegwynsstaff_d_05.mdx` | `staff_2h_artifactaegwynsstaff_d_05silver` | ✔ 正常 |
| 420597 | 法杖 | 64368 | 艾格文之杖D06 蓝 | `staff_2h_artifactaegwynsstaff_d_06.mdx` | `staff_2h_artifactaegwynsstaff_d_06_blue` | ✔ 正常 |
| 420598 | 法杖 | 64369 | 艾格文之杖D06 暗紫 | `staff_2h_artifactaegwynsstaff_d_06.mdx` | `staff_2h_artifactaegwynsstaff_d_06_darkpurple` | ✔ 正常 |
| 420599 | 法杖 | 64370 | 艾格文之杖D06 紫 | `staff_2h_artifactaegwynsstaff_d_06.mdx` | `staff_2h_artifactaegwynsstaff_d_06_purple` | ✔ 正常 |
| 420600 | 法杖 | 64371 | 艾格文之杖D06 青 | `staff_2h_artifactaegwynsstaff_d_06.mdx` | `staff_2h_artifactaegwynsstaff_d_06_teal` | ✔ 正常 |
| 420601 | 法杖 | 64372 | 安东尼达斯之杖D01 蓝 | `staff_2h_artifactantonidas_d_01.mdx` | `staff_2h_artifactantonidas_d_01_blue` | ✔ 正常 |
| 420602 | 法杖 | 64373 | 安东尼达斯之杖D01 绿 | `staff_2h_artifactantonidas_d_01.mdx` | `staff_2h_artifactantonidas_d_01_green` | ✔ 正常 |
| 420603 | 法杖 | 61036, 99961, 100538, 101131, 101708, 102285, 103485 | 安东尼达斯之杖D01 紫 | `staff_2h_artifactantonidas_d_01.mdx` | `staff_2h_artifactantonidas_d_01_purple` | ◑ 已使用 |
| 420604 | 法杖 | 61037, 99962, 100539, 101132, 101709, 102286, 103486 | 安东尼达斯之杖D01 红 | `staff_2h_artifactantonidas_d_01.mdx` | `staff_2h_artifactantonidas_d_01_red` | ◑ 已使用 |
| 420605 | 法杖 | 64374 | 安东尼达斯之杖D02 蓝 | `staff_2h_artifactantonidas_d_02.mdx` | `staff_2h_artifactantonidas_d_02_blue` | ✔ 正常 |
| 420606 | 法杖 | 64375 | 安东尼达斯之杖D02 绿 | `staff_2h_artifactantonidas_d_02.mdx` | `staff_2h_artifactantonidas_d_02_green` | ✔ 正常 |
| 420607 | 法杖 | 64376 | 安东尼达斯之杖D02 紫 | `staff_2h_artifactantonidas_d_02.mdx` | `staff_2h_artifactantonidas_d_02_purple` | ✔ 正常 |
| 420608 | 法杖 | 64377 | 安东尼达斯之杖D02 红 | `staff_2h_artifactantonidas_d_02.mdx` | `staff_2h_artifactantonidas_d_02_red` | ✔ 正常 |
| 420609 | 法杖 | 64378 | 安东尼达斯之杖D03 蓝 | `staff_2h_artifactantonidas_d_03.mdx` | `staff_2h_artifactantonidas_d_03_blue` | ✔ 正常 |
| 420610 | 法杖 | 33465 | 安东尼达斯之杖D03 金 | `staff_2h_artifactantonidas_d_03.mdx` | `staff_2h_artifactantonidas_d_03_gold` | ◑ 已使用 |
| 420611 | 法杖 | 64379 | 安东尼达斯之杖D03 紫 | `staff_2h_artifactantonidas_d_03.mdx` | `staff_2h_artifactantonidas_d_03_purple` | ✔ 正常 |
| 420612 | 法杖 | 64380 | 安东尼达斯之杖D03 红 | `staff_2h_artifactantonidas_d_03.mdx` | `staff_2h_artifactantonidas_d_03_red` | ✔ 正常 |
| 420613 | 法杖 | 64381 | 安东尼达斯之杖D04 蓝 | `staff_2h_artifactantonidas_d_04.mdx` | `staff_2h_artifactantonidas_d_04blue` | ✔ 正常 |
| 420614 | 法杖 | 64382 | 安东尼达斯之杖D04 深蓝 | `staff_2h_artifactantonidas_d_04.mdx` | `staff_2h_artifactantonidas_d_04darkblue` | ✔ 正常 |
| 420615 | 法杖 | 64383 | 安东尼达斯之杖D04 绿 | `staff_2h_artifactantonidas_d_04.mdx` | `staff_2h_artifactantonidas_d_04green` | ✔ 正常 |
| 420616 | 法杖 | 64384 | 安东尼达斯之杖D04 薄荷蓝 | `staff_2h_artifactantonidas_d_04.mdx` | `staff_2h_artifactantonidas_d_04mintblue` | ✔ 正常 |
| 420617 | 法杖 | 64385 | 安东尼达斯之杖D05 蓝 | `staff_2h_artifactantonidas_d_05.mdx` | `staff_2h_artifactantonidas_d_05blue` | ✔ 正常 |
| 420618 | 法杖 | 64386 | 安东尼达斯之杖D05 绿 | `staff_2h_artifactantonidas_d_05.mdx` | `staff_2h_artifactantonidas_d_05green` | ✔ 正常 |
| 420619 | 法杖 | 61038 | 安东尼达斯之杖D05 橙 | `staff_2h_artifactantonidas_d_05.mdx` | `staff_2h_artifactantonidas_d_05orange` | ◑ 已使用 |
| 420620 | 法杖 | 61039 | 安东尼达斯之杖D05 紫 | `staff_2h_artifactantonidas_d_05.mdx` | `staff_2h_artifactantonidas_d_05purple` | ◑ 已使用 |
| 420621 | 法杖 | 64387 | 安东尼达斯之杖D06 蓝 | `staff_2h_artifactantonidas_d_06.mdx` | `staff_2h_artifactantonidas_d_06_blue` | ✔ 正常 |
| 420622 | 法杖 | 64388 | 安东尼达斯之杖D06 绿 | `staff_2h_artifactantonidas_d_06.mdx` | `staff_2h_artifactantonidas_d_06_green` | ✔ 正常 |
| 420623 | 法杖 | 110000, 110010 | 安东尼达斯之杖D06 紫 | `staff_2h_artifactantonidas_d_06.mdx` | `staff_2h_artifactantonidas_d_06_purple` | ◑ 已使用 |
| 420624 | 法杖 | 64389 | 安东尼达斯之杖D06 红 | `staff_2h_artifactantonidas_d_06.mdx` | `staff_2h_artifactantonidas_d_06_red` | ✔ 正常 |
| 420625 | 法杖 | 64390 | 死亡之风D01 金 | `staff_2h_artifactdeadwind_d_01.mdx` | `staff_2h_artifactdeadwind_d_01_gold` | ✔ 正常 |
| 420626 | 法杖 | 64391 | 死亡之风D01 绿 | `staff_2h_artifactdeadwind_d_01.mdx` | `staff_2h_artifactdeadwind_d_01_green` | ✔ 正常 |
| 420627 | 法杖 | 64392 | 死亡之风D01 紫 | `staff_2h_artifactdeadwind_d_01.mdx` | `staff_2h_artifactdeadwind_d_01_purple` | ✔ 正常 |
| 420628 | 法杖 | 64393 | 死亡之风D01 红 | `staff_2h_artifactdeadwind_d_01.mdx` | `staff_2h_artifactdeadwind_d_01_red` | ✔ 正常 |
| 420629 | 法杖 | 64394 | 死亡之风D02 金 | `staff_2h_artifactdeadwind_d_02.mdx` | `staff_2h_artifactdeadwind_d_02_gold` | ✔ 正常 |
| 420630 | 法杖 | 64395 | 死亡之风D02 绿 | `staff_2h_artifactdeadwind_d_02.mdx` | `staff_2h_artifactdeadwind_d_02_green` | ✔ 正常 |
| 420631 | 法杖 | 64396 | 死亡之风D02 紫 | `staff_2h_artifactdeadwind_d_02.mdx` | `staff_2h_artifactdeadwind_d_02_purple` | ✔ 正常 |
| 420632 | 法杖 | 64397 | 死亡之风D02 红 | `staff_2h_artifactdeadwind_d_02.mdx` | `staff_2h_artifactdeadwind_d_02_red` | ✔ 正常 |
| 420633 | 法杖 | 64398 | 死亡之风D03 金 | `staff_2h_artifactdeadwind_d_03.mdx` | `staff_2h_artifactdeadwind_d_03_gold` | ✔ 正常 |
| 420634 | 法杖 | 64399 | 死亡之风D03 绿 | `staff_2h_artifactdeadwind_d_03.mdx` | `staff_2h_artifactdeadwind_d_03_green` | ✔ 正常 |
| 420635 | 法杖 | 64400 | 死亡之风D03 紫 | `staff_2h_artifactdeadwind_d_03.mdx` | `staff_2h_artifactdeadwind_d_03_purple` | ✔ 正常 |
| 420636 | 法杖 | 64401 | 死亡之风D03 红 | `staff_2h_artifactdeadwind_d_03.mdx` | `staff_2h_artifactdeadwind_d_03_red` | ✔ 正常 |
| 420637 | 法杖 | 64402 | 死亡之风D04 蓝 | `staff_2h_artifactdeadwind_d_04.mdx` | `staff_2h_artifactdeadwind_d_04_blue` | ✔ 正常 |
| 420638 | 法杖 | 64403 | 死亡之风D04 绿 | `staff_2h_artifactdeadwind_d_04.mdx` | `staff_2h_artifactdeadwind_d_04_green` | ✔ 正常 |
| 420639 | 法杖 | 64404 | 死亡之风D04 红 | `staff_2h_artifactdeadwind_d_04.mdx` | `staff_2h_artifactdeadwind_d_04_red` | ✔ 正常 |
| 420640 | 法杖 | 64405 | 死亡之风D04 银 | `staff_2h_artifactdeadwind_d_04.mdx` | `staff_2h_artifactdeadwind_d_04_silver` | ✔ 正常 |
| 420641 | 法杖 | 64406 | 死亡之风D05 蓝 | `staff_2h_artifactdeadwind_d_05.mdx` | `staff_2h_artifactdeadwind_d_05_blue` | ✔ 正常 |
| 420642 | 法杖 | 64407 | 死亡之风D05 橙 | `staff_2h_artifactdeadwind_d_05.mdx` | `staff_2h_artifactdeadwind_d_05_orange` | ✔ 正常 |
| 420643 | 法杖 | 64408 | 死亡之风D05 紫 | `staff_2h_artifactdeadwind_d_05.mdx` | `staff_2h_artifactdeadwind_d_05_purple` | ✔ 正常 |
| 420644 | 法杖 | 64409 | 死亡之风D05 红 | `staff_2h_artifactdeadwind_d_05.mdx` | `staff_2h_artifactdeadwind_d_05_red` | ✔ 正常 |
| 420645 | 法杖 | 64410 | 死亡之风D06 绿 | `staff_2h_artifactdeadwind_d_06.mdx` | `staff_2h_artifactdeadwind_d_06green` | ✔ 正常 |
| 420646 | 法杖 | 64411 | 死亡之风D06 橙 | `staff_2h_artifactdeadwind_d_06.mdx` | `staff_2h_artifactdeadwind_d_06orange` | ✔ 正常 |
| 420647 | 法杖 | 64412 | 死亡之风D06 紫 | `staff_2h_artifactdeadwind_d_06.mdx` | `staff_2h_artifactdeadwind_d_06purple` | ✔ 正常 |
| 420648 | 法杖 | 64413 | 死亡之风D06 红 | `staff_2h_artifactdeadwind_d_06.mdx` | `staff_2h_artifactdeadwind_d_06red` | ✔ 正常 |
| 420649 | 法杖 | 64414 | 艾露恩D01 蓝 | `staff_2h_artifactelune_d_01.mdx` | `staff_2h_artifactelune_d_01_blue` | ✔ 正常 |
| 420650 | 法杖 | 64415 | 艾露恩D01 绿 | `staff_2h_artifactelune_d_01.mdx` | `staff_2h_artifactelune_d_01_green` | ✔ 正常 |
| 420651 | 法杖 | 64416 | 艾露恩D01 紫 | `staff_2h_artifactelune_d_01.mdx` | `staff_2h_artifactelune_d_01_purple` | ✔ 正常 |
| 420652 | 法杖 | 64417 | 艾露恩D01 黄 | `staff_2h_artifactelune_d_01.mdx` | `staff_2h_artifactelune_d_01_yellow` | ✔ 正常 |
| 420653 | 法杖 | 64418 | 艾露恩D02 蓝 | `staff_2h_artifactelune_d_02.mdx` | `staff_2h_artifactelune_d_02_blue` | ✔ 正常 |
| 420654 | 法杖 | 64419 | 艾露恩D02 绿 | `staff_2h_artifactelune_d_02.mdx` | `staff_2h_artifactelune_d_02_green` | ✔ 正常 |
| 420655 | 法杖 | 64420 | 艾露恩D02 紫 | `staff_2h_artifactelune_d_02.mdx` | `staff_2h_artifactelune_d_02_purple` | ✔ 正常 |
| 420656 | 法杖 | 64421 | 艾露恩D02 黄 | `staff_2h_artifactelune_d_02.mdx` | `staff_2h_artifactelune_d_02_yellow` | ✔ 正常 |
| 420657 | 法杖 | 64422 | 艾露恩D03 蓝 | `staff_2h_artifactelune_d_03.mdx` | `staff_2h_artifactelune_d_03_blue` | ✔ 正常 |
| 420658 | 法杖 | 64423 | 艾露恩D03 绿 | `staff_2h_artifactelune_d_03.mdx` | `staff_2h_artifactelune_d_03_green` | ✔ 正常 |
| 420659 | 法杖 | 64424 | 艾露恩D03 紫 | `staff_2h_artifactelune_d_03.mdx` | `staff_2h_artifactelune_d_03_purple` | ✔ 正常 |
| 420660 | 法杖 | 64425 | 艾露恩D03 白 | `staff_2h_artifactelune_d_03.mdx` | `staff_2h_artifactelune_d_03_white` | ✔ 正常 |
| 420661 | 法杖 | 64426 | 艾露恩D04 蓝 | `staff_2h_artifactelune_d_04.mdx` | `staff_2h_artifactelune_d_04_blue` | ✔ 正常 |
| 420662 | 法杖 | 64427 | 艾露恩D04 紫 | `staff_2h_artifactelune_d_04.mdx` | `staff_2h_artifactelune_d_04_purple` | ✔ 正常 |
| 420663 | 法杖 | 64428 | 艾露恩D04 红 | `staff_2h_artifactelune_d_04.mdx` | `staff_2h_artifactelune_d_04_red` | ✔ 正常 |
| 420664 | 法杖 | 64429 | 艾露恩D04 黄 | `staff_2h_artifactelune_d_04.mdx` | `staff_2h_artifactelune_d_04_yellow` | ✔ 正常 |
| 420665 | 法杖 | 64430 | 艾露恩D05 蓝 | `staff_2h_artifactelune_d_05.mdx` | `staff_2h_artifactelune_d_05_blue` | ✔ 正常 |
| 420666 | 法杖 | 64431 | 艾露恩D05 绿 | `staff_2h_artifactelune_d_05.mdx` | `staff_2h_artifactelune_d_05_green` | ✔ 正常 |
| 420667 | 法杖 | 64432 | 艾露恩D05 红 | `staff_2h_artifactelune_d_05.mdx` | `staff_2h_artifactelune_d_05_red` | ✔ 正常 |
| 420668 | 法杖 | 64433 | 艾露恩D05 黄 | `staff_2h_artifactelune_d_05.mdx` | `staff_2h_artifactelune_d_05_yellow` | ✔ 正常 |
| 420669 | 法杖 | 64434 | 艾露恩D06 蓝 | `staff_2h_artifactelune_d_06.mdx` | `staff_2h_artifactelune_d_06_blue` | ✔ 正常 |
| 420670 | 法杖 | 64435 | 艾露恩D06 红 | `staff_2h_artifactelune_d_06.mdx` | `staff_2h_artifactelune_d_06_red` | ✔ 正常 |
| 420671 | 法杖 | 64436 | 艾露恩D06 青 | `staff_2h_artifactelune_d_06.mdx` | `staff_2h_artifactelune_d_06_teal` | ✔ 正常 |
| 420672 | 法杖 | 64437 | 艾露恩D06 黄 | `staff_2h_artifactelune_d_06.mdx` | `staff_2h_artifactelune_d_06_yellow` | ✔ 正常 |
| 420673 | 法杖 | 64438 | 库雷之心D01 蓝 | `staff_2h_artifactheartofkure_d_01.mdx` | `staff_2h_artifactheartofkure_d_01blue` | ✔ 正常 |
| 420674 | 法杖 | 64439 | 库雷之心D01 绿 | `staff_2h_artifactheartofkure_d_01.mdx` | `staff_2h_artifactheartofkure_d_01green` | ✔ 正常 |
| 420675 | 法杖 | 64440 | 库雷之心D01 紫 | `staff_2h_artifactheartofkure_d_01.mdx` | `staff_2h_artifactheartofkure_d_01purple` | ✔ 正常 |
| 420676 | 法杖 | 64441 | 库雷之心D01 红 | `staff_2h_artifactheartofkure_d_01.mdx` | `staff_2h_artifactheartofkure_d_01red` | ✔ 正常 |
| 420677 | 法杖 | 64442 | 库雷之心D02 蓝 | `staff_2h_artifactheartofkure_d_02.mdx` | `staff_2h_artifactheartofkure_d_02blue` | ✔ 正常 |
| 420678 | 法杖 | 64443 | 库雷之心D02 紫 | `staff_2h_artifactheartofkure_d_02.mdx` | `staff_2h_artifactheartofkure_d_02purple` | ✔ 正常 |
| 420679 | 法杖 | 64444 | 库雷之心D02 红 | `staff_2h_artifactheartofkure_d_02.mdx` | `staff_2h_artifactheartofkure_d_02red` | ✔ 正常 |
| 420680 | 法杖 | 64445 | 库雷之心D02 黄 | `staff_2h_artifactheartofkure_d_02.mdx` | `staff_2h_artifactheartofkure_d_02yellow` | ✔ 正常 |
| 420681 | 法杖 | 64446 | 库雷之心D03 蓝 | `staff_2h_artifactheartofkure_d_03.mdx` | `staff_2h_artifactheartofkure_d_03_blue` | ✔ 正常 |
| 420682 | 法杖 | 64447 | 库雷之心D03 紫 | `staff_2h_artifactheartofkure_d_03.mdx` | `staff_2h_artifactheartofkure_d_03_purple` | ✔ 正常 |
| 420683 | 法杖 | 64448 | 库雷之心D03 红 | `staff_2h_artifactheartofkure_d_03.mdx` | `staff_2h_artifactheartofkure_d_03_red` | ✔ 正常 |
| 420684 | 法杖 | 64449 | 库雷之心D03 黄 | `staff_2h_artifactheartofkure_d_03.mdx` | `staff_2h_artifactheartofkure_d_03_yellow` | ✔ 正常 |
| 420685 | 法杖 | 64450 | 库雷之心D04 蓝 | `staff_2h_artifactheartofkure_d_04.mdx` | `staff_2h_artifactheartofkure_d_04blue` | ✔ 正常 |
| 420686 | 法杖 | 64451 | 库雷之心D04 紫 | `staff_2h_artifactheartofkure_d_04.mdx` | `staff_2h_artifactheartofkure_d_04purple` | ✔ 正常 |
| 420687 | 法杖 | 64452 | 库雷之心D04 红 | `staff_2h_artifactheartofkure_d_04.mdx` | `staff_2h_artifactheartofkure_d_04red` | ✔ 正常 |
| 420688 | 法杖 | 64453 | 库雷之心D04 黄 | `staff_2h_artifactheartofkure_d_04.mdx` | `staff_2h_artifactheartofkure_d_04yellow` | ✔ 正常 |
| 420689 | 法杖 | 64454 | 库雷之心D05 绿 | `staff_2h_artifactheartofkure_d_05.mdx` | `staff_2h_artifactheartofkure_d_05green` | ✔ 正常 |
| 420690 | 法杖 | 64455 | 库雷之心D05 紫 | `staff_2h_artifactheartofkure_d_05.mdx` | `staff_2h_artifactheartofkure_d_05purple` | ✔ 正常 |
| 420691 | 法杖 | 64456 | 库雷之心D05 红 | `staff_2h_artifactheartofkure_d_05.mdx` | `staff_2h_artifactheartofkure_d_05red` | ✔ 正常 |
| 420692 | 法杖 | 64457 | 库雷之心D05 黄 | `staff_2h_artifactheartofkure_d_05.mdx` | `staff_2h_artifactheartofkure_d_05yellow` | ✔ 正常 |
| 420693 | 法杖 | 64458 | 库雷之心D06 暗 | `staff_2h_artifactheartofkure_d_06.mdx` | `staff_2h_artifactheartofkure_d_06_dark` | ✔ 正常 |
| 420694 | 法杖 | 51799 | 库雷之心D06 金 | `staff_2h_artifactheartofkure_d_06.mdx` | `staff_2h_artifactheartofkure_d_06_gold` | ◑ 已使用 |
| 420695 | 法杖 | 64459 | 库雷之心D06 绿 | `staff_2h_artifactheartofkure_d_06.mdx` | `staff_2h_artifactheartofkure_d_06_green` | ✔ 正常 |
| 420696 | 法杖 | 64460 | 库雷之心D06 银 | `staff_2h_artifactheartofkure_d_06.mdx` | `staff_2h_artifactheartofkure_d_06_silver` | ✔ 正常 |
| 420697 | 法杖 | 64461 | 美猴王D01 蓝 | `staff_2h_artifactmonkeyking_d_01.mdx` | `staff_2h_artifactmonkeyking_d_01blue` | ✔ 正常 |
| 420698 | 法杖 | 64462 | 美猴王D01 绿 | `staff_2h_artifactmonkeyking_d_01.mdx` | `staff_2h_artifactmonkeyking_d_01green` | ✔ 正常 |
| 420699 | 法杖 | 64463 | 美猴王D01 紫 | `staff_2h_artifactmonkeyking_d_01.mdx` | `staff_2h_artifactmonkeyking_d_01purple` | ✔ 正常 |
| 420700 | 法杖 | 64464 | 美猴王D01 黄 | `staff_2h_artifactmonkeyking_d_01.mdx` | `staff_2h_artifactmonkeyking_d_01yellow` | ✔ 正常 |
| 420701 | 法杖 | 64465 | 美猴王D02 蓝 | `staff_2h_artifactmonkeyking_d_02.mdx` | `staff_2h_artifactmonkeyking_d_02blue` | ✔ 正常 |
| 420702 | 法杖 | 64466 | 美猴王D02 绿 | `staff_2h_artifactmonkeyking_d_02.mdx` | `staff_2h_artifactmonkeyking_d_02green` | ✔ 正常 |
| 420703 | 法杖 | 64467 | 美猴王D02 红 | `staff_2h_artifactmonkeyking_d_02.mdx` | `staff_2h_artifactmonkeyking_d_02red` | ✔ 正常 |
| 420704 | 法杖 | 64468 | 美猴王D02 黄 | `staff_2h_artifactmonkeyking_d_02.mdx` | `staff_2h_artifactmonkeyking_d_02yellow` | ✔ 正常 |
| 420705 | 法杖 | 64469 | 美猴王D03 蓝 | `staff_2h_artifactmonkeyking_d_03.mdx` | `staff_2h_artifactmonkeyking_d_03blue` | ✔ 正常 |
| 420706 | 法杖 | 64470 | 美猴王D03 绿 | `staff_2h_artifactmonkeyking_d_03.mdx` | `staff_2h_artifactmonkeyking_d_03green` | ✔ 正常 |
| 420707 | 法杖 | 64471 | 美猴王D03 红 | `staff_2h_artifactmonkeyking_d_03.mdx` | `staff_2h_artifactmonkeyking_d_03red` | ✔ 正常 |
| 420708 | 法杖 | 64472 | 美猴王D03 黄 | `staff_2h_artifactmonkeyking_d_03.mdx` | `staff_2h_artifactmonkeyking_d_03yellow` | ✔ 正常 |
| 420709 | 法杖 | 64473 | 美猴王D04 蓝 | `staff_2h_artifactmonkeyking_d_04.mdx` | `staff_2h_artifactmonkeyking_d_04blue` | ✔ 正常 |
| 420710 | 法杖 | 64474 | 美猴王D04 绿 | `staff_2h_artifactmonkeyking_d_04.mdx` | `staff_2h_artifactmonkeyking_d_04green` | ✔ 正常 |
| 420711 | 法杖 | 64475 | 美猴王D04 红 | `staff_2h_artifactmonkeyking_d_04.mdx` | `staff_2h_artifactmonkeyking_d_04red` | ✔ 正常 |
| 420712 | 法杖 | 64476 | 美猴王D04 黄 | `staff_2h_artifactmonkeyking_d_04.mdx` | `staff_2h_artifactmonkeyking_d_04yellow` | ✔ 正常 |
| 420713 | 法杖 | 64477 | 美猴王D05 蓝 | `staff_2h_artifactmonkeyking_d_05.mdx` | `staff_2h_artifactmonkeyking_d_05blue` | ✔ 正常 |
| 420714 | 法杖 | 64478 | 美猴王D05 绿 | `staff_2h_artifactmonkeyking_d_05.mdx` | `staff_2h_artifactmonkeyking_d_05green` | ✔ 正常 |
| 420715 | 法杖 | 64479 | 美猴王D05 韧光 | `staff_2h_artifactmonkeyking_d_05.mdx` | `staff_2h_artifactmonkeyking_d_05res` | ✔ 正常 |
| 420716 | 法杖 | 64480 | 美猴王D05 黄 | `staff_2h_artifactmonkeyking_d_05.mdx` | `staff_2h_artifactmonkeyking_d_05yellow` | ✔ 正常 |
| 420717 | 法杖 | 64481 | 美猴王D06 蓝 | `staff_2h_artifactmonkeyking_d_06.mdx` | `staff_2h_artifactmonkeyking_d_06_blue` | ✔ 正常 |
| 420718 | 法杖 | 64482 | 美猴王D06 绿 | `staff_2h_artifactmonkeyking_d_06.mdx` | `staff_2h_artifactmonkeyking_d_06_green` | ✔ 正常 |
| 420719 | 法杖 | 64483 | 美猴王D06 月 | `staff_2h_artifactmonkeyking_d_06.mdx` | `staff_2h_artifactmonkeyking_d_06_moon` | ✔ 正常 |
| 420720 | 法杖 | 64484 | 美猴王D06 紫 | `staff_2h_artifactmonkeyking_d_06.mdx` | `staff_2h_artifactmonkeyking_d_06_purple` | ✔ 正常 |
| 420721 | 法杖 | 64485 | 美猴王D06 红 | `staff_2h_artifactmonkeyking_d_06.mdx` | `staff_2h_artifactmonkeyking_d_06_red` | ✔ 正常 |
| 420722 | 法杖 | 64486 | 诺达希尔D01 绿 | `staff_2h_artifactnordrassil_d_01.mdx` | `staff_2h_artifactnordrassil_d_01_green` | ✔ 正常 |
| 420723 | 法杖 | 64487 | 诺达希尔D01 紫 | `staff_2h_artifactnordrassil_d_01.mdx` | `staff_2h_artifactnordrassil_d_01_purple` | ✔ 正常 |
| 420724 | 法杖 | 64488 | 诺达希尔D01 青 | `staff_2h_artifactnordrassil_d_01.mdx` | `staff_2h_artifactnordrassil_d_01_teal` | ✔ 正常 |
| 420725 | 法杖 | 64489 | 诺达希尔D01 黄 | `staff_2h_artifactnordrassil_d_01.mdx` | `staff_2h_artifactnordrassil_d_01_yellow` | ✔ 正常 |
| 420726 | 法杖 | 64490 | 诺达希尔D02 绿 | `staff_2h_artifactnordrassil_d_02.mdx` | `staff_2h_artifactnordrassil_d_02_green` | ✔ 正常 |
| 420727 | 法杖 | 64491 | 诺达希尔D02 紫 | `staff_2h_artifactnordrassil_d_02.mdx` | `staff_2h_artifactnordrassil_d_02_purple` | ✔ 正常 |
| 420728 | 法杖 | 64492 | 诺达希尔D02 青 | `staff_2h_artifactnordrassil_d_02.mdx` | `staff_2h_artifactnordrassil_d_02_teal` | ✔ 正常 |
| 420729 | 法杖 | 64493 | 诺达希尔D02 黄 | `staff_2h_artifactnordrassil_d_02.mdx` | `staff_2h_artifactnordrassil_d_02_yellow` | ✔ 正常 |
| 420730 | 法杖 | 64494 | 诺达希尔D03 绿 | `staff_2h_artifactnordrassil_d_03.mdx` | `staff_2h_artifactnordrassil_d_03_green` | ✔ 正常 |
| 420731 | 法杖 | 64495 | 诺达希尔D03 紫 | `staff_2h_artifactnordrassil_d_03.mdx` | `staff_2h_artifactnordrassil_d_03_purple` | ✔ 正常 |
| 420732 | 法杖 | 64496 | 诺达希尔D03 红 | `staff_2h_artifactnordrassil_d_03.mdx` | `staff_2h_artifactnordrassil_d_03_red` | ✔ 正常 |
| 420733 | 法杖 | 64497 | 诺达希尔D03 青 | `staff_2h_artifactnordrassil_d_03.mdx` | `staff_2h_artifactnordrassil_d_03_teal` | ✔ 正常 |
| 420734 | 法杖 | 64498 | 诺达希尔D04 绿 | `staff_2h_artifactnordrassil_d_04.mdx` | `staff_2h_artifactnordrassil_d_04_green` | ✔ 正常 |
| 420735 | 法杖 | 49992 | 诺达希尔D04 紫 | `staff_2h_artifactnordrassil_d_04.mdx` | `staff_2h_artifactnordrassil_d_04_purple` | ◑ 已使用 |
| 420736 | 法杖 | 64499 | 诺达希尔D04 红 | `staff_2h_artifactnordrassil_d_04.mdx` | `staff_2h_artifactnordrassil_d_04_red` | ✔ 正常 |
| 420737 | 法杖 | 64500 | 诺达希尔D04 青 | `staff_2h_artifactnordrassil_d_04.mdx` | `staff_2h_artifactnordrassil_d_04_teal` | ✔ 正常 |
| 420738 | 法杖 | 64501 | 诺达希尔D05 绿 | `staff_2h_artifactnordrassil_d_05.mdx` | `staff_2h_artifactnordrassil_d_05_green` | ✔ 正常 |
| 420739 | 法杖 | 64502 | 诺达希尔D05 紫 | `staff_2h_artifactnordrassil_d_05.mdx` | `staff_2h_artifactnordrassil_d_05_purple` | ✔ 正常 |
| 420740 | 法杖 | 64503 | 诺达希尔D05 红 | `staff_2h_artifactnordrassil_d_05.mdx` | `staff_2h_artifactnordrassil_d_05_red` | ✔ 正常 |
| 420741 | 法杖 | 64504 | 诺达希尔D05 青 | `staff_2h_artifactnordrassil_d_05.mdx` | `staff_2h_artifactnordrassil_d_05_teal` | ✔ 正常 |
| 420742 | 法杖 | 64505 | 诺达希尔D06 紫 | `staff_2h_artifactnordrassil_d_06.mdx` | `staff_2h_artifactnordrassil_d_06_purple` | ✔ 正常 |
| 420743 | 法杖 | 64506 | 诺达希尔D06 红 | `staff_2h_artifactnordrassil_d_06.mdx` | `staff_2h_artifactnordrassil_d_06_red` | ✔ 正常 |
| 420744 | 法杖 | 64507 | 诺达希尔D06 青 | `staff_2h_artifactnordrassil_d_06.mdx` | `staff_2h_artifactnordrassil_d_06_teal` | ✔ 正常 |
| 420745 | 法杖 | 64508 | 诺达希尔D06 黄 | `staff_2h_artifactnordrassil_d_06.mdx` | `staff_2h_artifactnordrassil_d_06_yellow` | ✔ 正常 |
| 420746 | 法杖 | 64509 | 萨格拉斯D01 蓝 | `staff_2h_artifactsargeras_d_01.mdx` | `staff_2h_artifactsargeras_d_01blue` | ✔ 正常 |
| 420747 | 法杖 | 64510 | 萨格拉斯D01 绿 | `staff_2h_artifactsargeras_d_01.mdx` | `staff_2h_artifactsargeras_d_01green` | ✔ 正常 |
| 420748 | 法杖 | 64511 | 萨格拉斯D01 紫 | `staff_2h_artifactsargeras_d_01.mdx` | `staff_2h_artifactsargeras_d_01purple` | ✔ 正常 |
| 420749 | 法杖 | 64512 | 萨格拉斯D01 红 | `staff_2h_artifactsargeras_d_01.mdx` | `staff_2h_artifactsargeras_d_01red` | ✔ 正常 |
| 420750 | 法杖 | 64513 | 萨格拉斯D02 蓝 | `staff_2h_artifactsargeras_d_02.mdx` | `staff_2h_artifactsargeras_d_02blue` | ✔ 正常 |
| 420751 | 法杖 | 64514 | 萨格拉斯D02 绿 | `staff_2h_artifactsargeras_d_02.mdx` | `staff_2h_artifactsargeras_d_02green` | ✔ 正常 |
| 420752 | 法杖 | 64515 | 萨格拉斯D02 紫 | `staff_2h_artifactsargeras_d_02.mdx` | `staff_2h_artifactsargeras_d_02purple` | ✔ 正常 |
| 420753 | 法杖 | 64516 | 萨格拉斯D02 红 | `staff_2h_artifactsargeras_d_02.mdx` | `staff_2h_artifactsargeras_d_02red` | ✔ 正常 |
| 420754 | 法杖 | 64517 | 萨格拉斯D03 蓝 | `staff_2h_artifactsargeras_d_03.mdx` | `staff_2h_artifactsargeras_d_03blue` | ✔ 正常 |
| 420755 | 法杖 | 64518 | 萨格拉斯D03 紫 | `staff_2h_artifactsargeras_d_03.mdx` | `staff_2h_artifactsargeras_d_03purple` | ✔ 正常 |
| 420756 | 法杖 | 64519 | 萨格拉斯D03 红 | `staff_2h_artifactsargeras_d_03.mdx` | `staff_2h_artifactsargeras_d_03red` | ✔ 正常 |
| 420757 | 法杖 | 64520 | 萨格拉斯D03 黄 | `staff_2h_artifactsargeras_d_03.mdx` | `staff_2h_artifactsargeras_d_03yellow` | ✔ 正常 |
| 420758 | 法杖 | 64521 | 萨格拉斯D04 蓝 | `staff_2h_artifactsargeras_d_04.mdx` | `staff_2h_artifactsargeras_d_04blue` | ✔ 正常 |
| 420759 | 法杖 | 64522 | 萨格拉斯D04 绿 | `staff_2h_artifactsargeras_d_04.mdx` | `staff_2h_artifactsargeras_d_04green` | ✔ 正常 |
| 420760 | 法杖 | 64523 | 萨格拉斯D04 紫 | `staff_2h_artifactsargeras_d_04.mdx` | `staff_2h_artifactsargeras_d_04purple` | ✔ 正常 |
| 420761 | 法杖 | 64524 | 萨格拉斯D04 红 | `staff_2h_artifactsargeras_d_04.mdx` | `staff_2h_artifactsargeras_d_04red` | ✔ 正常 |
| 420762 | 法杖 | 64525 | 萨格拉斯D05 蓝 | `staff_2h_artifactsargeras_d_05.mdx` | `staff_2h_artifactsargeras_d_05blue` | ✔ 正常 |
| 420763 | 法杖 | 64526 | 萨格拉斯D05 绿 | `staff_2h_artifactsargeras_d_05.mdx` | `staff_2h_artifactsargeras_d_05green` | ✔ 正常 |
| 420764 | 法杖 | 64527 | 萨格拉斯D05 紫 | `staff_2h_artifactsargeras_d_05.mdx` | `staff_2h_artifactsargeras_d_05purple` | ✔ 正常 |
| 420765 | 法杖 | 64528 | 萨格拉斯D05 红 | `staff_2h_artifactsargeras_d_05.mdx` | `staff_2h_artifactsargeras_d_05red` | ✔ 正常 |
| 420766 | 法杖 | 64529 | 萨格拉斯D06 | `staff_2h_artifactsargeras_d_06.mdx` | `staff_2h_artifactsargeras_d_06` | ✔ 正常 |
| 420767 | 法杖 | 64530 | 萨格拉斯D06 绿 | `staff_2h_artifactsargeras_d_06.mdx` | `staff_2h_artifactsargeras_d_06green` | ✔ 正常 |
| 420768 | 法杖 | 64531 | 萨格拉斯D06 紫 | `staff_2h_artifactsargeras_d_06.mdx` | `staff_2h_artifactsargeras_d_06purple` | ✔ 正常 |
| 420769 | 法杖 | 64532 | 萨格拉斯D06 黄 | `staff_2h_artifactsargeras_d_06.mdx` | `staff_2h_artifactsargeras_d_06yellow` | ✔ 正常 |
| 420770 | 法杖 | 64533 | 少昊D01 蓝 | `staff_2h_artifactshaohao_d_01.mdx` | `staff_2h_artifactshaohao_d_01_blue` | ✔ 正常 |
| 420771 | 法杖 | 64534 | 少昊D01 绿 | `staff_2h_artifactshaohao_d_01.mdx` | `staff_2h_artifactshaohao_d_01_green` | ✔ 正常 |
| 420772 | 法杖 | 64535 | 少昊D01 红 | `staff_2h_artifactshaohao_d_01.mdx` | `staff_2h_artifactshaohao_d_01_red` | ✔ 正常 |
| 420773 | 法杖 | 64536 | 少昊D01 黄 | `staff_2h_artifactshaohao_d_01.mdx` | `staff_2h_artifactshaohao_d_01_yellow` | ✔ 正常 |
| 420774 | 法杖 | 64537 | 少昊D02 蓝 | `staff_2h_artifactshaohao_d_02.mdx` | `staff_2h_artifactshaohao_d_02_blue` | ✔ 正常 |
| 420775 | 法杖 | 64538 | 少昊D02 绿 | `staff_2h_artifactshaohao_d_02.mdx` | `staff_2h_artifactshaohao_d_02_green` | ✔ 正常 |
| 420776 | 法杖 | 64539 | 少昊D02 红 | `staff_2h_artifactshaohao_d_02.mdx` | `staff_2h_artifactshaohao_d_02_red` | ✔ 正常 |
| 420777 | 法杖 | 64540 | 少昊D02 黄 | `staff_2h_artifactshaohao_d_02.mdx` | `staff_2h_artifactshaohao_d_02_yellow` | ✔ 正常 |
| 420778 | 法杖 | 64541 | 少昊D03 蔚蓝 | `staff_2h_artifactshaohao_d_03.mdx` | `staff_2h_artifactshaohao_d_03_azure` | ✔ 正常 |
| 420779 | 法杖 | 64542 | 少昊D03 天界 | `staff_2h_artifactshaohao_d_03.mdx` | `staff_2h_artifactshaohao_d_03_heavenly` | ✔ 正常 |
| 420780 | 法杖 | 64543 | 少昊D03 玉 | `staff_2h_artifactshaohao_d_03.mdx` | `staff_2h_artifactshaohao_d_03_jade` | ✔ 正常 |
| 420781 | 法杖 | 64544 | 少昊D03 红宝石 | `staff_2h_artifactshaohao_d_03.mdx` | `staff_2h_artifactshaohao_d_03_ruby` | ✔ 正常 |
| 420782 | 法杖 | 64545 | 少昊D04 愤怒 | `staff_2h_artifactshaohao_d_04.mdx` | `staff_2h_artifactshaohao_d_04_anger` | ✔ 正常 |
| 420783 | 法杖 | 64546 | 少昊D04 绝望 | `staff_2h_artifactshaohao_d_04.mdx` | `staff_2h_artifactshaohao_d_04_despair` | ✔ 正常 |
| 420784 | 法杖 | 64547 | 少昊D04 恐惧 | `staff_2h_artifactshaohao_d_04.mdx` | `staff_2h_artifactshaohao_d_04_fear` | ✔ 正常 |
| 420785 | 法杖 | 64548 | 少昊D04 傲慢 | `staff_2h_artifactshaohao_d_04.mdx` | `staff_2h_artifactshaohao_d_04_pride` | ✔ 正常 |
| 420786 | 法杖 | 64549 | 少昊D05 蓝 | `staff_2h_artifactshaohao_d_05.mdx` | `staff_2h_artifactshaohao_d_05_blue` | ✔ 正常 |
| 420787 | 法杖 | 64550 | 少昊D05 绿 | `staff_2h_artifactshaohao_d_05.mdx` | `staff_2h_artifactshaohao_d_05_green` | ✔ 正常 |
| 420788 | 法杖 | 64551 | 少昊D05 紫 | `staff_2h_artifactshaohao_d_05.mdx` | `staff_2h_artifactshaohao_d_05_purple` | ✔ 正常 |
| 420789 | 法杖 | 64552 | 少昊D05 红 | `staff_2h_artifactshaohao_d_05.mdx` | `staff_2h_artifactshaohao_d_05_red` | ✔ 正常 |
| 420790 | 法杖 | 64553 | 少昊D06 蓝 | `staff_2h_artifactshaohao_d_06.mdx` | `staff_2h_artifactshaohao_d_06_blue` | ✔ 正常 |
| 420791 | 法杖 | 64554 | 少昊D06 玉 | `staff_2h_artifactshaohao_d_06.mdx` | `staff_2h_artifactshaohao_d_06_jade` | ✔ 正常 |
| 420792 | 法杖 | 64555 | 少昊D06 橙 | `staff_2h_artifactshaohao_d_06.mdx` | `staff_2h_artifactshaohao_d_06_orange` | ✔ 正常 |
| 420793 | 法杖 | 64556 | 少昊D06 白 | `staff_2h_artifactshaohao_d_06.mdx` | `staff_2h_artifactshaohao_d_06_white` | ✔ 正常 |
| 420794 | 法杖 | 64557 | 创世典籍D01 蓝 | `staff_2h_artifacttome_d_01.mdx` | `staff_2h_artifacttome_d_01_blue` | ✔ 正常 |
| 420795 | 法杖 | 64558 | 创世典籍D01 紫 | `staff_2h_artifacttome_d_01.mdx` | `staff_2h_artifacttome_d_01_purple` | ✔ 正常 |
| 420796 | 法杖 | 64559 | 创世典籍D01 红 | `staff_2h_artifacttome_d_01.mdx` | `staff_2h_artifacttome_d_01_red` | ✔ 正常 |
| 420797 | 法杖 | 64560 | 创世典籍D01 黄 | `staff_2h_artifacttome_d_01.mdx` | `staff_2h_artifacttome_d_01_yellow` | ✔ 正常 |
| 420798 | 法杖 | 64561 | 创世典籍D02 蓝 | `staff_2h_artifacttome_d_02.mdx` | `staff_2h_artifacttome_d_02_blue` | ✔ 正常 |
| 420799 | 法杖 | 64562 | 创世典籍D02 绿 | `staff_2h_artifacttome_d_02.mdx` | `staff_2h_artifacttome_d_02_green` | ✔ 正常 |
| 420800 | 法杖 | 64563 | 创世典籍D02 红 | `staff_2h_artifacttome_d_02.mdx` | `staff_2h_artifacttome_d_02_red` | ✔ 正常 |
| 420801 | 法杖 | 64564 | 创世典籍D02 黄 | `staff_2h_artifacttome_d_02.mdx` | `staff_2h_artifacttome_d_02_yellow` | ✔ 正常 |
| 420802 | 法杖 | 64565 | 创世典籍D03 蓝 | `staff_2h_artifacttome_d_03.mdx` | `staff_2h_artifacttome_d_03_blue` | ✔ 正常 |
| 420803 | 法杖 | 64566 | 创世典籍D03 绿 | `staff_2h_artifacttome_d_03.mdx` | `staff_2h_artifacttome_d_03_green` | ✔ 正常 |
| 420804 | 法杖 | 64567 | 创世典籍D03 紫 | `staff_2h_artifacttome_d_03.mdx` | `staff_2h_artifacttome_d_03_purple` | ✔ 正常 |
| 420805 | 法杖 | 64568 | 创世典籍D03 红 | `staff_2h_artifacttome_d_03.mdx` | `staff_2h_artifacttome_d_03_red` | ✔ 正常 |
| 420806 | 法杖 | 64569 | 创世典籍D04 绿 | `staff_2h_artifacttome_d_04.mdx` | `staff_2h_artifacttome_d_04_green` | ✔ 正常 |
| 420807 | 法杖 | 64570 | 创世典籍D04 紫 | `staff_2h_artifacttome_d_04.mdx` | `staff_2h_artifacttome_d_04_purple` | ✔ 正常 |
| 420808 | 法杖 | 64571 | 创世典籍D04 红 | `staff_2h_artifacttome_d_04.mdx` | `staff_2h_artifacttome_d_04_red` | ✔ 正常 |
| 420809 | 法杖 | 64572 | 创世典籍D04 黄 | `staff_2h_artifacttome_d_04.mdx` | `staff_2h_artifacttome_d_04_yellow` | ✔ 正常 |
| 420810 | 法杖 | 64573 | 创世典籍D05 蓝 | `staff_2h_artifacttome_d_05.mdx` | `staff_2h_artifacttome_d_05_blue` | ✔ 正常 |
| 420811 | 法杖 | 64574 | 创世典籍D05 绿 | `staff_2h_artifacttome_d_05.mdx` | `staff_2h_artifacttome_d_05_green` | ✔ 正常 |
| 420812 | 法杖 | 64575 | 创世典籍D05 紫 | `staff_2h_artifacttome_d_05.mdx` | `staff_2h_artifacttome_d_05_purple` | ✔ 正常 |
| 420813 | 法杖 | 64576 | 创世典籍D05 黄 | `staff_2h_artifacttome_d_05.mdx` | `staff_2h_artifacttome_d_05_yellow` | ✔ 正常 |
| 420814 | 法杖 | 64577 | 创世典籍D06 蓝 | `staff_2h_artifacttome_d_06.mdx` | `staff_2h_artifacttome_d_06_blue` | ✔ 正常 |
| 420815 | 法杖 | 64578 | 创世典籍D06 绿 | `staff_2h_artifacttome_d_06.mdx` | `staff_2h_artifacttome_d_06_green` | ✔ 正常 |
| 420816 | 法杖 | 64579 | 创世典籍D06 紫 | `staff_2h_artifacttome_d_06.mdx` | `staff_2h_artifacttome_d_06_purple` | ✔ 正常 |
| 420817 | 法杖 | 64580 | 创世典籍D06 黄 | `staff_2h_artifacttome_d_06.mdx` | `staff_2h_artifacttome_d_06_yellow` | ✔ 正常 |
| 420818 | 法杖 | 64581 | 德拉诺挑战D01 紫辉 | `staff_2h_draenorchallenge_d_01.mdx` | `staff_2h_draenorchallenge_d_01_purpleglow` | ✔ 正常 |
| 100005 | 法杖 | 92000 | 王昭君之法杖 | `wangzhaojunwuqi.mdx` | — | ◑ 已使用 |
| 100007 | 法杖 | 64582 | 诸葛亮之羽扇杖 | `zhugeliangwuqi.mdx` | — | ✔ 正常 |

## 八、匕首（99 条：已使用 3 / 新生成 96）

> `class=2, subclass=15, InventoryType=13, sheath=3, Material=1`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420177 | 匕首 | 64583 | 克苏恩D01 蓝 | `knife_1h_artifactcthun_d_01.mdx` | `knife_1h_artifactcthun_d_01blue` | ✔ 正常 |
| 420178 | 匕首 | 64584 | 克苏恩D01 紫 | `knife_1h_artifactcthun_d_01.mdx` | `knife_1h_artifactcthun_d_01purple` | ✔ 正常 |
| 420179 | 匕首 | 64585 | 克苏恩D01 青 | `knife_1h_artifactcthun_d_01.mdx` | `knife_1h_artifactcthun_d_01teal` | ✔ 正常 |
| 420180 | 匕首 | 64586 | 克苏恩D01 白 | `knife_1h_artifactcthun_d_01.mdx` | `knife_1h_artifactcthun_d_01white` | ✔ 正常 |
| 420181 | 匕首 | 64587 | 克苏恩D02 黑 | `knife_1h_artifactcthun_d_02.mdx` | `knife_1h_artifactcthun_d_02black` | ✔ 正常 |
| 420182 | 匕首 | 64588 | 克苏恩D02 绿 | `knife_1h_artifactcthun_d_02.mdx` | `knife_1h_artifactcthun_d_02green` | ✔ 正常 |
| 420183 | 匕首 | 64589 | 克苏恩D02 紫 | `knife_1h_artifactcthun_d_02.mdx` | `knife_1h_artifactcthun_d_02purple` | ✔ 正常 |
| 420184 | 匕首 | 64590 | 克苏恩D02 青 | `knife_1h_artifactcthun_d_02.mdx` | `knife_1h_artifactcthun_d_02teal` | ✔ 正常 |
| 420185 | 匕首 | 64591 | 克苏恩D03 黑 | `knife_1h_artifactcthun_d_03.mdx` | `knife_1h_artifactcthun_d_03black` | ✔ 正常 |
| 420186 | 匕首 | 64592 | 克苏恩D03 绿 | `knife_1h_artifactcthun_d_03.mdx` | `knife_1h_artifactcthun_d_03green` | ✔ 正常 |
| 420187 | 匕首 | 64593 | 克苏恩D03 紫 | `knife_1h_artifactcthun_d_03.mdx` | `knife_1h_artifactcthun_d_03purple` | ✔ 正常 |
| 420188 | 匕首 | 64594 | 克苏恩D03 红 | `knife_1h_artifactcthun_d_03.mdx` | `knife_1h_artifactcthun_d_03red` | ✔ 正常 |
| 420189 | 匕首 | 64595 | 克苏恩D04 绿 | `knife_1h_artifactcthun_d_04.mdx` | `knife_1h_artifactcthun_d_04green` | ✔ 正常 |
| 420190 | 匕首 | 64596 | 克苏恩D04 紫 | `knife_1h_artifactcthun_d_04.mdx` | `knife_1h_artifactcthun_d_04purple` | ✔ 正常 |
| 420191 | 匕首 | 64597 | 克苏恩D04 青 | `knife_1h_artifactcthun_d_04.mdx` | `knife_1h_artifactcthun_d_04teal` | ✔ 正常 |
| 420192 | 匕首 | 64598 | 克苏恩D04 白 | `knife_1h_artifactcthun_d_04.mdx` | `knife_1h_artifactcthun_d_04white` | ✔ 正常 |
| 420193 | 匕首 | 64599 | 克苏恩D05 蓝 | `knife_1h_artifactcthun_d_05.mdx` | `knife_1h_artifactcthun_d_05blue` | ✔ 正常 |
| 420194 | 匕首 | 64600 | 克苏恩D05 玉 | `knife_1h_artifactcthun_d_05.mdx` | `knife_1h_artifactcthun_d_05jade` | ✔ 正常 |
| 420195 | 匕首 | 64601 | 克苏恩D05 亮黄 | `knife_1h_artifactcthun_d_05.mdx` | `knife_1h_artifactcthun_d_05lighyellow` | ✔ 正常 |
| 420196 | 匕首 | 64602 | 克苏恩D05 紫 | `knife_1h_artifactcthun_d_05.mdx` | `knife_1h_artifactcthun_d_05purple` | ✔ 正常 |
| 420197 | 匕首 | 50183 | 克苏恩D06 绿 | `knife_1h_artifactcthun_d_06.mdx` | `knife_1h_artifactcthun_d_06_green` | ◑ 已使用 |
| 420198 | 匕首 | 64603 | 克苏恩D06 橙 | `knife_1h_artifactcthun_d_06.mdx` | `knife_1h_artifactcthun_d_06_orange` | ✔ 正常 |
| 420199 | 匕首 | 64604 | 克苏恩D06 紫 | `knife_1h_artifactcthun_d_06.mdx` | `knife_1h_artifactcthun_d_06_purple` | ✔ 正常 |
| 420200 | 匕首 | 64605 | 克苏恩D06 红 | `knife_1h_artifactcthun_d_06.mdx` | `knife_1h_artifactcthun_d_06_red` | ✔ 正常 |
| 420201 | 匕首 | 64606 | 毒牙D01 蓝 | `knife_1h_artifactfangs_d_01.mdx` | `knife_1h_artifactfangs_d_01_blue` | ✔ 正常 |
| 420202 | 匕首 | 64607 | 毒牙D01 绿 | `knife_1h_artifactfangs_d_01.mdx` | `knife_1h_artifactfangs_d_01_green` | ✔ 正常 |
| 420203 | 匕首 | 64608 | 毒牙D01 紫 | `knife_1h_artifactfangs_d_01.mdx` | `knife_1h_artifactfangs_d_01_purple` | ✔ 正常 |
| 420204 | 匕首 | 64609 | 毒牙D01 青 | `knife_1h_artifactfangs_d_01.mdx` | `knife_1h_artifactfangs_d_01_teal` | ✔ 正常 |
| 420205 | 匕首 | 64610 | 毒牙D02 蓝 | `knife_1h_artifactfangs_d_02.mdx` | `knife_1h_artifactfangs_d_02_blue` | ✔ 正常 |
| 420206 | 匕首 | 64611 | 毒牙D02 绿 | `knife_1h_artifactfangs_d_02.mdx` | `knife_1h_artifactfangs_d_02_green` | ✔ 正常 |
| 420207 | 匕首 | 64612 | 毒牙D02 紫 | `knife_1h_artifactfangs_d_02.mdx` | `knife_1h_artifactfangs_d_02_purple` | ✔ 正常 |
| 420208 | 匕首 | 64613 | 毒牙D02 黄 | `knife_1h_artifactfangs_d_02.mdx` | `knife_1h_artifactfangs_d_02_yellow` | ✔ 正常 |
| 420209 | 匕首 | 64614 | 毒牙D03 蓝 | `knife_1h_artifactfangs_d_03.mdx` | `knife_1h_artifactfangs_d_03_blue` | ✔ 正常 |
| 420210 | 匕首 | 64615 | 毒牙D03 绿 | `knife_1h_artifactfangs_d_03.mdx` | `knife_1h_artifactfangs_d_03_green` | ✔ 正常 |
| 420211 | 匕首 | 64616 | 毒牙D03 紫 | `knife_1h_artifactfangs_d_03.mdx` | `knife_1h_artifactfangs_d_03_purple` | ✔ 正常 |
| 420212 | 匕首 | 64617 | 毒牙D03 青 | `knife_1h_artifactfangs_d_03.mdx` | `knife_1h_artifactfangs_d_03_teal` | ✔ 正常 |
| 420213 | 匕首 | 64618 | 毒牙D04 绿 | `knife_1h_artifactfangs_d_04.mdx` | `knife_1h_artifactfangs_d_04_green` | ✔ 正常 |
| 420214 | 匕首 | 64619 | 毒牙D04 紫 | `knife_1h_artifactfangs_d_04.mdx` | `knife_1h_artifactfangs_d_04_purple` | ✔ 正常 |
| 420215 | 匕首 | 64620 | 毒牙D04 红 | `knife_1h_artifactfangs_d_04.mdx` | `knife_1h_artifactfangs_d_04_red` | ✔ 正常 |
| 420216 | 匕首 | 64621 | 毒牙D04 幽魂 | `knife_1h_artifactfangs_d_04.mdx` | `knife_1h_artifactfangs_d_04_spooky` | ✔ 正常 |
| 420217 | 匕首 | 64622 | 毒牙D05 蓝 | `knife_1h_artifactfangs_d_05.mdx` | `knife_1h_artifactfangs_d_05_blue` | ✔ 正常 |
| 420218 | 匕首 | 64623 | 毒牙D05 绿 | `knife_1h_artifactfangs_d_05.mdx` | `knife_1h_artifactfangs_d_05_green` | ✔ 正常 |
| 420219 | 匕首 | 64624 | 毒牙D05 紫 | `knife_1h_artifactfangs_d_05.mdx` | `knife_1h_artifactfangs_d_05_purple` | ✔ 正常 |
| 420220 | 匕首 | 64625 | 毒牙D05 青 | `knife_1h_artifactfangs_d_05.mdx` | `knife_1h_artifactfangs_d_05_teal` | ✔ 正常 |
| 420221 | 匕首 | 64626 | 毒牙D06 蓝 | `knife_1h_artifactfangs_d_06.mdx` | `knife_1h_artifactfangs_d_06_blue` | ✔ 正常 |
| 420222 | 匕首 | 64627 | 毒牙D06 绿 | `knife_1h_artifactfangs_d_06.mdx` | `knife_1h_artifactfangs_d_06_green` | ✔ 正常 |
| 420223 | 匕首 | 64628 | 毒牙D06 橙 | `knife_1h_artifactfangs_d_06.mdx` | `knife_1h_artifactfangs_d_06_orange` | ✔ 正常 |
| 420224 | 匕首 | 64629 | 毒牙D06 红 | `knife_1h_artifactfangs_d_06.mdx` | `knife_1h_artifactfangs_d_06_red` | ✔ 正常 |
| 420225 | 匕首 | 64630 | 霜刃D01 蓝 | `knife_1h_artifactfrostsaber_d_01.mdx` | `knife_1h_artifactfrostsaber_d_01_blue` | ✔ 正常 |
| 420226 | 匕首 | 64631 | 霜刃D01 绿 | `knife_1h_artifactfrostsaber_d_01.mdx` | `knife_1h_artifactfrostsaber_d_01_green` | ✔ 正常 |
| 420227 | 匕首 | 64632 | 霜刃D01 紫 | `knife_1h_artifactfrostsaber_d_01.mdx` | `knife_1h_artifactfrostsaber_d_01_purple` | ✔ 正常 |
| 420228 | 匕首 | 64633 | 霜刃D01 红 | `knife_1h_artifactfrostsaber_d_01.mdx` | `knife_1h_artifactfrostsaber_d_01_red` | ✔ 正常 |
| 420229 | 匕首 | 64634 | 迦罗娜D01 紫 | `knife_1h_artifactgarona_d_01.mdx` | `knife_1h_artifactgarona_d_01_purple` | ✔ 正常 |
| 420230 | 匕首 | 64635 | 迦罗娜D01 红 | `knife_1h_artifactgarona_d_01.mdx` | `knife_1h_artifactgarona_d_01_red` | ✔ 正常 |
| 420231 | 匕首 | 64636 | 迦罗娜D01 青 | `knife_1h_artifactgarona_d_01.mdx` | `knife_1h_artifactgarona_d_01_teal` | ✔ 正常 |
| 420232 | 匕首 | 64637 | 迦罗娜D01 黄 | `knife_1h_artifactgarona_d_01.mdx` | `knife_1h_artifactgarona_d_01_yellow` | ✔ 正常 |
| 420233 | 匕首 | 64638 | 迦罗娜D02 橙 | `knife_1h_artifactgarona_d_02.mdx` | `knife_1h_artifactgarona_d_02_orange` | ✔ 正常 |
| 420234 | 匕首 | 64639 | 迦罗娜D02 紫 | `knife_1h_artifactgarona_d_02.mdx` | `knife_1h_artifactgarona_d_02_purple` | ✔ 正常 |
| 420235 | 匕首 | 49968 | 迦罗娜D02 红 | `knife_1h_artifactgarona_d_02.mdx` | `knife_1h_artifactgarona_d_02_red` | ◑ 已使用 |
| 420236 | 匕首 | 64640 | 迦罗娜D02 青 | `knife_1h_artifactgarona_d_02.mdx` | `knife_1h_artifactgarona_d_02_teal` | ✔ 正常 |
| 420237 | 匕首 | 64641 | 迦罗娜D03 蓝 | `knife_1h_artifactgarona_d_03.mdx` | `knife_1h_artifactgarona_d_03_blue` | ✔ 正常 |
| 420238 | 匕首 | 64642 | 迦罗娜D03 橙 | `knife_1h_artifactgarona_d_03.mdx` | `knife_1h_artifactgarona_d_03_orange` | ✔ 正常 |
| 420239 | 匕首 | 64643 | 迦罗娜D03 青 | `knife_1h_artifactgarona_d_03.mdx` | `knife_1h_artifactgarona_d_03_teal` | ✔ 正常 |
| 420240 | 匕首 | 64644 | 迦罗娜D03 黄 | `knife_1h_artifactgarona_d_03.mdx` | `knife_1h_artifactgarona_d_03_yellow` | ✔ 正常 |
| 420241 | 匕首 | 64645 | 迦罗娜D04 蓝 | `knife_1h_artifactgarona_d_04.mdx` | `knife_1h_artifactgarona_d_04_blue` | ✔ 正常 |
| 420242 | 匕首 | 64646 | 迦罗娜D04 绿 | `knife_1h_artifactgarona_d_04.mdx` | `knife_1h_artifactgarona_d_04_green` | ✔ 正常 |
| 420243 | 匕首 | 64647 | 迦罗娜D04 红 | `knife_1h_artifactgarona_d_04.mdx` | `knife_1h_artifactgarona_d_04_red` | ✔ 正常 |
| 420244 | 匕首 | 64648 | 迦罗娜D04 青 | `knife_1h_artifactgarona_d_04.mdx` | `knife_1h_artifactgarona_d_04_teal` | ✔ 正常 |
| 420245 | 匕首 | 64649 | 迦罗娜D05 蓝 | `knife_1h_artifactgarona_d_05.mdx` | `knife_1h_artifactgarona_d_05_blue` | ✔ 正常 |
| 420246 | 匕首 | 64650 | 迦罗娜D05 红 | `knife_1h_artifactgarona_d_05.mdx` | `knife_1h_artifactgarona_d_05_red` | ✔ 正常 |
| 420247 | 匕首 | 64651 | 迦罗娜D05 青 | `knife_1h_artifactgarona_d_05.mdx` | `knife_1h_artifactgarona_d_05_teal` | ✔ 正常 |
| 420248 | 匕首 | 51803 | 迦罗娜D05 黄 | `knife_1h_artifactgarona_d_05.mdx` | `knife_1h_artifactgarona_d_05_yellow` | ◑ 已使用 |
| 420249 | 匕首 | 64652 | 迦罗娜D06 蓝 | `knife_1h_artifactgarona_d_06.mdx` | `knife_1h_artifactgarona_d_06blue` | ✔ 正常 |
| 420250 | 匕首 | 64653 | 迦罗娜D06 绿 | `knife_1h_artifactgarona_d_06.mdx` | `knife_1h_artifactgarona_d_06green` | ✔ 正常 |
| 420251 | 匕首 | 64654 | 迦罗娜D06 红 | `knife_1h_artifactgarona_d_06.mdx` | `knife_1h_artifactgarona_d_06red` | ✔ 正常 |
| 420252 | 匕首 | 64655 | 艾瑞达之颅D01 | `knife_1h_artifactskulloferedar_d_01.mdx` | `knife_1h_artifactskulloferedar_d_01` | ✔ 正常 |
| 420253 | 匕首 | 64656 | 艾瑞达之颅D01 绿 | `knife_1h_artifactskulloferedar_d_01.mdx` | `knife_1h_artifactskulloferedar_d_01green` | ✔ 正常 |
| 420254 | 匕首 | 64657 | 艾瑞达之颅D01 红 | `knife_1h_artifactskulloferedar_d_01.mdx` | `knife_1h_artifactskulloferedar_d_01red` | ✔ 正常 |
| 420255 | 匕首 | 64658 | 艾瑞达之颅D01 黄 | `knife_1h_artifactskulloferedar_d_01.mdx` | `knife_1h_artifactskulloferedar_d_01yellow` | ✔ 正常 |
| 420256 | 匕首 | 64659 | 艾瑞达之颅D02 | `knife_1h_artifactskulloferedar_d_02.mdx` | `knife_1h_artifactskulloferedar_d_02` | ✔ 正常 |
| 420257 | 匕首 | 64660 | 艾瑞达之颅D02 绿 | `knife_1h_artifactskulloferedar_d_02.mdx` | `knife_1h_artifactskulloferedar_d_02green` | ✔ 正常 |
| 420258 | 匕首 | 64661 | 艾瑞达之颅D02 红 | `knife_1h_artifactskulloferedar_d_02.mdx` | `knife_1h_artifactskulloferedar_d_02red` | ✔ 正常 |
| 420259 | 匕首 | 64662 | 艾瑞达之颅D02 黄 | `knife_1h_artifactskulloferedar_d_02.mdx` | `knife_1h_artifactskulloferedar_d_02yellow` | ✔ 正常 |
| 420260 | 匕首 | 64663 | 艾瑞达之颅D03 | `knife_1h_artifactskulloferedar_d_03.mdx` | `knife_1h_artifactskulloferedar_d_03` | ✔ 正常 |
| 420261 | 匕首 | 64664 | 艾瑞达之颅D03 紫 | `knife_1h_artifactskulloferedar_d_03.mdx` | `knife_1h_artifactskulloferedar_d_03purple` | ✔ 正常 |
| 420262 | 匕首 | 64665 | 艾瑞达之颅D03 红 | `knife_1h_artifactskulloferedar_d_03.mdx` | `knife_1h_artifactskulloferedar_d_03red` | ✔ 正常 |
| 420263 | 匕首 | 64666 | 艾瑞达之颅D03 黄 | `knife_1h_artifactskulloferedar_d_03.mdx` | `knife_1h_artifactskulloferedar_d_03yellow` | ✔ 正常 |
| 420264 | 匕首 | 64667 | 艾瑞达之颅D04 | `knife_1h_artifactskulloferedar_d_04.mdx` | `knife_1h_artifactskulloferedar_d_04` | ✔ 正常 |
| 420265 | 匕首 | 64668 | 艾瑞达之颅D04 蓝 | `knife_1h_artifactskulloferedar_d_04.mdx` | `knife_1h_artifactskulloferedar_d_04blue` | ✔ 正常 |
| 420266 | 匕首 | 64669 | 艾瑞达之颅D04 绿 | `knife_1h_artifactskulloferedar_d_04.mdx` | `knife_1h_artifactskulloferedar_d_04green` | ✔ 正常 |
| 420267 | 匕首 | 64670 | 艾瑞达之颅D04 紫 | `knife_1h_artifactskulloferedar_d_04.mdx` | `knife_1h_artifactskulloferedar_d_04purple` | ✔ 正常 |
| 420268 | 匕首 | 64671 | 艾瑞达之颅D05 | `knife_1h_artifactskulloferedar_d_05.mdx` | `knife_1h_artifactskulloferedar_d_05` | ✔ 正常 |
| 420269 | 匕首 | 64672 | 艾瑞达之颅D05 蓝 | `knife_1h_artifactskulloferedar_d_05.mdx` | `knife_1h_artifactskulloferedar_d_05blue` | ✔ 正常 |
| 420270 | 匕首 | 64673 | 艾瑞达之颅D05 紫 | `knife_1h_artifactskulloferedar_d_05.mdx` | `knife_1h_artifactskulloferedar_d_05purple` | ✔ 正常 |
| 420271 | 匕首 | 64674 | 艾瑞达之颅D05 黄 | `knife_1h_artifactskulloferedar_d_05.mdx` | `knife_1h_artifactskulloferedar_d_05yellow` | ✔ 正常 |
| 420272 | 匕首 | 64675 | 艾瑞达之颅D06 | `knife_1h_artifactskulloferedar_d_06.mdx` | `knife_1h_artifactskulloferedar_d_06` | ✔ 正常 |
| 420273 | 匕首 | 64676 | 艾瑞达之颅D06 苍白 | `knife_1h_artifactskulloferedar_d_06.mdx` | `knife_1h_artifactskulloferedar_d_06pale` | ✔ 正常 |
| 420274 | 匕首 | 64677 | 艾瑞达之颅D06 紫 | `knife_1h_artifactskulloferedar_d_06.mdx` | `knife_1h_artifactskulloferedar_d_06purple` | ✔ 正常 |
| 420275 | 匕首 | 64678 | 艾瑞达之颅D06 红 | `knife_1h_artifactskulloferedar_d_06.mdx` | `knife_1h_artifactskulloferedar_d_06red` | ✔ 正常 |

## 九、拳套（68 条：已使用 0 / 新生成 68）

> `class=2, subclass=13, InventoryType=13, sheath=3, Material=1`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420125 | 拳套 | 64679 | 苍穹D01 蓝 | `hand_1h_artifactskywall_d_01.mdx` | `hand_1h_artifactskywall_d_01_blue` | ✔ 正常 |
| 420126 | 拳套 | 64680 | 苍穹D01 绿 | `hand_1h_artifactskywall_d_01.mdx` | `hand_1h_artifactskywall_d_01_green` | ✔ 正常 |
| 420127 | 拳套 | 64681 | 苍穹D01 红 | `hand_1h_artifactskywall_d_01.mdx` | `hand_1h_artifactskywall_d_01_red` | ✔ 正常 |
| 420128 | 拳套 | 64682 | 苍穹D01 黄 | `hand_1h_artifactskywall_d_01.mdx` | `hand_1h_artifactskywall_d_01_yellow` | ✔ 正常 |
| 420129 | 拳套 | 64683 | 苍穹D02 蓝 | `hand_1h_artifactskywall_d_02.mdx` | `hand_1h_artifactskywall_d_02_blue` | ✔ 正常 |
| 420130 | 拳套 | 64684 | 苍穹D02 绿 | `hand_1h_artifactskywall_d_02.mdx` | `hand_1h_artifactskywall_d_02_green` | ✔ 正常 |
| 420131 | 拳套 | 64685 | 苍穹D02 红 | `hand_1h_artifactskywall_d_02.mdx` | `hand_1h_artifactskywall_d_02_red` | ✔ 正常 |
| 420132 | 拳套 | 64686 | 苍穹D02 黄 | `hand_1h_artifactskywall_d_02.mdx` | `hand_1h_artifactskywall_d_02_yellow` | ✔ 正常 |
| 420133 | 拳套 | 64687 | 苍穹D03 玉 | `hand_1h_artifactskywall_d_03.mdx` | `hand_1h_artifactskywall_d_03_jade` | ✔ 正常 |
| 420134 | 拳套 | 64688 | 苍穹D03 红 | `hand_1h_artifactskywall_d_03.mdx` | `hand_1h_artifactskywall_d_03_red` | ✔ 正常 |
| 420135 | 拳套 | 64689 | 苍穹D03 白 | `hand_1h_artifactskywall_d_03.mdx` | `hand_1h_artifactskywall_d_03_white` | ✔ 正常 |
| 420136 | 拳套 | 64690 | 苍穹D03 黄 | `hand_1h_artifactskywall_d_03.mdx` | `hand_1h_artifactskywall_d_03_yellow` | ✔ 正常 |
| 420137 | 拳套 | 64691 | 苍穹D04 蓝 | `hand_1h_artifactskywall_d_04.mdx` | `hand_1h_artifactskywall_d_04_blue` | ✔ 正常 |
| 420138 | 拳套 | 64692 | 苍穹D04 绿 | `hand_1h_artifactskywall_d_04.mdx` | `hand_1h_artifactskywall_d_04_green` | ✔ 正常 |
| 420139 | 拳套 | 64693 | 苍穹D04 红 | `hand_1h_artifactskywall_d_04.mdx` | `hand_1h_artifactskywall_d_04_red` | ✔ 正常 |
| 420140 | 拳套 | 64694 | 苍穹D04 黄 | `hand_1h_artifactskywall_d_04.mdx` | `hand_1h_artifactskywall_d_04_yellow` | ✔ 正常 |
| 420141 | 拳套 | 64695 | 苍穹D05_1 | `Hand_1H_ArtifactSkywall_D_05_1.mdx` | — | ✔ 正常 |
| 420142 | 拳套 | 64696 | 苍穹D05_2 | `Hand_1H_ArtifactSkywall_D_05_2.mdx` | — | ✔ 正常 |
| 420143 | 拳套 | 64697 | 苍穹D05_3 | `Hand_1H_ArtifactSkywall_D_05_3.mdx` | — | ✔ 正常 |
| 420144 | 拳套 | 64698 | 苍穹D05_4 | `Hand_1H_ArtifactSkywall_D_05_4.mdx` | — | ✔ 正常 |
| 420145 | 拳套 | 64699 | 苍穹D06 玉 | `hand_1h_artifactskywall_d_06.mdx` | `hand_1h_artifactskywall_d_06_jade` | ✔ 正常 |
| 420146 | 拳套 | 64700 | 苍穹D06 红 | `hand_1h_artifactskywall_d_06.mdx` | `hand_1h_artifactskywall_d_06_red` | ✔ 正常 |
| 420147 | 拳套 | 64701 | 苍穹D06 白 | `hand_1h_artifactskywall_d_06.mdx` | `hand_1h_artifactskywall_d_06_white` | ✔ 正常 |
| 420148 | 拳套 | 64702 | 苍穹D06 黄 | `hand_1h_artifactskywall_d_06.mdx` | `hand_1h_artifactskywall_d_06_yellow` | ✔ 正常 |
| 420149 | 拳套 | 64703 | 风暴之拳D01 蓝 | `hand_1h_artifactstormfist_d_01.mdx` | `hand_1h_artifactstormfist_d_01blue` | ✔ 正常 |
| 420150 | 拳套 | 64704 | 风暴之拳D01 紫 | `hand_1h_artifactstormfist_d_01.mdx` | `hand_1h_artifactstormfist_d_01purple` | ✔ 正常 |
| 420151 | 拳套 | 64705 | 风暴之拳D01 白 | `hand_1h_artifactstormfist_d_01.mdx` | `hand_1h_artifactstormfist_d_01white` | ✔ 正常 |
| 420152 | 拳套 | 64706 | 风暴之拳D01 黄 | `hand_1h_artifactstormfist_d_01.mdx` | `hand_1h_artifactstormfist_d_01yellow` | ✔ 正常 |
| 420153 | 拳套 | 64707 | 风暴之拳D02 蓝 | `hand_1h_artifactstormfist_d_02.mdx` | `hand_1h_artifactstormfist_d_02blue` | ✔ 正常 |
| 420154 | 拳套 | 64708 | 风暴之拳D02 绿 | `hand_1h_artifactstormfist_d_02.mdx` | `hand_1h_artifactstormfist_d_02green` | ✔ 正常 |
| 420155 | 拳套 | 64709 | 风暴之拳D02 白 | `hand_1h_artifactstormfist_d_02.mdx` | `hand_1h_artifactstormfist_d_02white` | ✔ 正常 |
| 420156 | 拳套 | 64710 | 风暴之拳D02 黄 | `hand_1h_artifactstormfist_d_02.mdx` | `hand_1h_artifactstormfist_d_02yellow` | ✔ 正常 |
| 420157 | 拳套 | 64711 | 风暴之拳D03 蓝 | `hand_1h_artifactstormfist_d_03.mdx` | `hand_1h_artifactstormfist_d_03_blue` | ✔ 正常 |
| 420158 | 拳套 | 64712 | 风暴之拳D03 绿 | `hand_1h_artifactstormfist_d_03.mdx` | `hand_1h_artifactstormfist_d_03_green` | ✔ 正常 |
| 420159 | 拳套 | 64713 | 风暴之拳D03 橙 | `hand_1h_artifactstormfist_d_03.mdx` | `hand_1h_artifactstormfist_d_03_orange` | ✔ 正常 |
| 420160 | 拳套 | 64714 | 风暴之拳D03 紫 | `hand_1h_artifactstormfist_d_03.mdx` | `hand_1h_artifactstormfist_d_03_purple` | ✔ 正常 |
| 420161 | 拳套 | 64715 | 风暴之拳D04 蓝 | `hand_1h_artifactstormfist_d_04.mdx` | `hand_1h_artifactstormfist_d_04_blue` | ✔ 正常 |
| 420162 | 拳套 | 64716 | 风暴之拳D04 绿 | `hand_1h_artifactstormfist_d_04.mdx` | `hand_1h_artifactstormfist_d_04_green` | ✔ 正常 |
| 420163 | 拳套 | 64717 | 风暴之拳D04 红 | `hand_1h_artifactstormfist_d_04.mdx` | `hand_1h_artifactstormfist_d_04_red` | ✔ 正常 |
| 420164 | 拳套 | 64718 | 风暴之拳D04 黄 | `hand_1h_artifactstormfist_d_04.mdx` | `hand_1h_artifactstormfist_d_04_yellow` | ✔ 正常 |
| 420165 | 拳套 | 64719 | 风暴之拳D05 蓝 | `hand_1h_artifactstormfist_d_05.mdx` | `hand_1h_artifactstormfist_d_05_blue` | ✔ 正常 |
| 420166 | 拳套 | 64720 | 风暴之拳D05 绿 | `hand_1h_artifactstormfist_d_05.mdx` | `hand_1h_artifactstormfist_d_05_green` | ✔ 正常 |
| 420167 | 拳套 | 64721 | 风暴之拳D05 紫 | `hand_1h_artifactstormfist_d_05.mdx` | `hand_1h_artifactstormfist_d_05_purple` | ✔ 正常 |
| 420168 | 拳套 | 64722 | 风暴之拳D05 红 | `hand_1h_artifactstormfist_d_05.mdx` | `hand_1h_artifactstormfist_d_05_red` | ✔ 正常 |
| 420169 | 拳套 | 64723 | 风暴之拳D06 蓝 | `hand_1h_artifactstormfist_d_06.mdx` | `hand_1h_artifactstormfist_d_06_blue` | ✔ 正常 |
| 420170 | 拳套 | 64724 | 风暴之拳D06 绿 | `hand_1h_artifactstormfist_d_06.mdx` | `hand_1h_artifactstormfist_d_06_green` | ✔ 正常 |
| 420171 | 拳套 | 64725 | 风暴之拳D06 橙 | `hand_1h_artifactstormfist_d_06.mdx` | `hand_1h_artifactstormfist_d_06_orange` | ✔ 正常 |
| 420172 | 拳套 | 64726 | 风暴之拳D06 青 | `hand_1h_artifactstormfist_d_06.mdx` | `hand_1h_artifactstormfist_d_06_teal` | ✔ 正常 |
| 420173 | 拳套 | 64727 | 乌索克D01 橙 | `hand_1h_artifactursoc_d_01.mdx` | `hand_1h_artifactursoc_d_01_orange` | ✔ 正常 |
| 420174 | 拳套 | 64728 | 乌索克D01 紫 | `hand_1h_artifactursoc_d_01.mdx` | `hand_1h_artifactursoc_d_01_purple` | ✔ 正常 |
| 420175 | 拳套 | 64729 | 乌索克D01 红 | `hand_1h_artifactursoc_d_01.mdx` | `hand_1h_artifactursoc_d_01_red` | ✔ 正常 |
| 420176 | 拳套 | 64730 | 乌索克D01 青 | `hand_1h_artifactursoc_d_01.mdx` | `hand_1h_artifactursoc_d_01_teal` | ✔ 正常 |
| 421055 | 拳套 | 64731 | 左·苍穹D04_1 | `L_Hand_1H_ArtifactSkywall_D_04_1.mdx` | — | ✔ 正常 |
| 421056 | 拳套 | 64732 | 左·苍穹D04_2 | `L_Hand_1H_ArtifactSkywall_D_04_2.mdx` | — | ✔ 正常 |
| 421057 | 拳套 | 64733 | 左·苍穹D04_3 | `L_Hand_1H_ArtifactSkywall_D_04_3.mdx` | — | ✔ 正常 |
| 421058 | 拳套 | 64734 | 左·苍穹D04_4 | `L_Hand_1H_ArtifactSkywall_D_04_4.mdx` | — | ✔ 正常 |
| 421059 | 拳套 | 64735 | 左·苍穹D05_1 | `L_Hand_1H_ArtifactSkywall_D_05_1.mdx` | — | ✔ 正常 |
| 421060 | 拳套 | 64736 | 左·苍穹D05_2 | `L_Hand_1H_ArtifactSkywall_D_05_2.mdx` | — | ✔ 正常 |
| 421061 | 拳套 | 64737 | 左·苍穹D05_3 | `L_Hand_1H_ArtifactSkywall_D_05_3.mdx` | — | ✔ 正常 |
| 421062 | 拳套 | 64738 | 左·苍穹D05_4 | `L_Hand_1H_ArtifactSkywall_D_05_4.mdx` | — | ✔ 正常 |
| 421063 | 拳套 | 64739 | 左·苍穹D06_1 | `L_Hand_1H_ArtifactSkywall_D_06_1.mdx` | — | ✔ 正常 |
| 421064 | 拳套 | 64740 | 左·苍穹D06_2 | `L_Hand_1H_ArtifactSkywall_D_06_2.mdx` | — | ✔ 正常 |
| 421065 | 拳套 | 64741 | 左·苍穹D06_3 | `L_Hand_1H_ArtifactSkywall_D_06_3.mdx` | — | ✔ 正常 |
| 421066 | 拳套 | 64742 | 左·苍穹D06_4 | `L_Hand_1H_ArtifactSkywall_D_06_4.mdx` | — | ✔ 正常 |
| 421067 | 拳套 | 64743 | 左·乌索克D01_1 | `L_Hand_1H_ArtifactUrsoc_D_01_1.mdx` | — | ✔ 正常 |
| 421068 | 拳套 | 64744 | 左·乌索克D01_2 | `L_Hand_1H_ArtifactUrsoc_D_01_2.mdx` | — | ✔ 正常 |
| 421069 | 拳套 | 64745 | 左·乌索克D01_3 | `L_Hand_1H_ArtifactUrsoc_D_01_3.mdx` | — | ✔ 正常 |
| 421070 | 拳套 | 64746 | 左·乌索克D01_4 | `L_Hand_1H_ArtifactUrsoc_D_01_4.mdx` | — | ✔ 正常 |

## 十、战刃（90 条：已使用 4 / 新生成 86）

> `class=2, subclass=7, InventoryType=13, sheath=3, Material=1`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420081 | 战刃 | 64747 | 阿尔德罗奇D01 金 | `glaive_1h_artifactaldrochi_d_01.mdx` | `glaive_1h_artifactaldrochi_d_01_gold` | ✔ 正常 |
| 420082 | 战刃 | 64748 | 阿尔德罗奇D01 绿 | `glaive_1h_artifactaldrochi_d_01.mdx` | `glaive_1h_artifactaldrochi_d_01_green` | ✔ 正常 |
| 420083 | 战刃 | 64749 | 阿尔德罗奇D01 紫 | `glaive_1h_artifactaldrochi_d_01.mdx` | `glaive_1h_artifactaldrochi_d_01_purple` | ✔ 正常 |
| 420084 | 战刃 | 64750 | 阿尔德罗奇D01 银 | `glaive_1h_artifactaldrochi_d_01.mdx` | `glaive_1h_artifactaldrochi_d_01_silver` | ✔ 正常 |
| 420085 | 战刃 | 64751 | 阿尔德罗奇D02 蓝 | `glaive_1h_artifactaldrochi_d_02.mdx` | `glaive_1h_artifactaldrochi_d_02_blue` | ✔ 正常 |
| 420086 | 战刃 | 64752 | 阿尔德罗奇D02 金 | `glaive_1h_artifactaldrochi_d_02.mdx` | `glaive_1h_artifactaldrochi_d_02_gold` | ✔ 正常 |
| 420087 | 战刃 | 64753 | 阿尔德罗奇D02 绿 | `glaive_1h_artifactaldrochi_d_02.mdx` | `glaive_1h_artifactaldrochi_d_02_green` | ✔ 正常 |
| 420088 | 战刃 | 64754 | 阿尔德罗奇D02 紫 | `glaive_1h_artifactaldrochi_d_02.mdx` | `glaive_1h_artifactaldrochi_d_02_purple` | ✔ 正常 |
| 420089 | 战刃 | 64755 | 阿尔德罗奇D03 蓝 | `glaive_1h_artifactaldrochi_d_03.mdx` | `glaive_1h_artifactaldrochi_d_03_blue` | ✔ 正常 |
| 420090 | 战刃 | 64756 | 阿尔德罗奇D03 金 | `glaive_1h_artifactaldrochi_d_03.mdx` | `glaive_1h_artifactaldrochi_d_03_gold` | ✔ 正常 |
| 420091 | 战刃 | 64757 | 阿尔德罗奇D03 紫 | `glaive_1h_artifactaldrochi_d_03.mdx` | `glaive_1h_artifactaldrochi_d_03_purple` | ✔ 正常 |
| 420092 | 战刃 | 64758 | 阿尔德罗奇D03 红 | `glaive_1h_artifactaldrochi_d_03.mdx` | `glaive_1h_artifactaldrochi_d_03_red` | ✔ 正常 |
| 420093 | 战刃 | 64759 | 阿尔德罗奇D04 蓝 | `glaive_1h_artifactaldrochi_d_04.mdx` | `glaive_1h_artifactaldrochi_d_04_blue` | ✔ 正常 |
| 420094 | 战刃 | 64760 | 阿尔德罗奇D04 金 | `glaive_1h_artifactaldrochi_d_04.mdx` | `glaive_1h_artifactaldrochi_d_04_gold` | ✔ 正常 |
| 420095 | 战刃 | 64761 | 阿尔德罗奇D04 绿 | `glaive_1h_artifactaldrochi_d_04.mdx` | `glaive_1h_artifactaldrochi_d_04_green` | ✔ 正常 |
| 420096 | 战刃 | 64762 | 阿尔德罗奇D04 红 | `glaive_1h_artifactaldrochi_d_04.mdx` | `glaive_1h_artifactaldrochi_d_04_red` | ✔ 正常 |
| 420097 | 战刃 | 64763 | 阿尔德罗奇D05 蓝 | `glaive_1h_artifactaldrochi_d_05.mdx` | `glaive_1h_artifactaldrochi_d_05_blue` | ✔ 正常 |
| 420098 | 战刃 | 64764 | 阿尔德罗奇D05 绿 | `glaive_1h_artifactaldrochi_d_05.mdx` | `glaive_1h_artifactaldrochi_d_05_green` | ✔ 正常 |
| 420099 | 战刃 | 64765 | 阿尔德罗奇D05 紫 | `glaive_1h_artifactaldrochi_d_05.mdx` | `glaive_1h_artifactaldrochi_d_05_purple` | ✔ 正常 |
| 420100 | 战刃 | 64766 | 阿尔德罗奇D05 红 | `glaive_1h_artifactaldrochi_d_05.mdx` | `glaive_1h_artifactaldrochi_d_05_red` | ✔ 正常 |
| 421071 | 战刃 | 64767 | 阿尔德罗奇D06_左 | `Glaive_1H_ArtifactAldrochi_D_06left.mdx` | — | ✔ 正常 |
| 421072 | 战刃 | 64768 | 阿尔德罗奇D06_右 | `Glaive_1H_ArtifactAldrochi_D_06right.mdx` | — | ✔ 正常 |
| 420101 | 战刃 | 64769 | 阿兹加洛D01 蓝 | `glaive_1h_artifactazgalor_d_01.mdx` | `glaive_1h_artifactazgalor_d_01blue` | ✔ 正常 |
| 420102 | 战刃 | 64770 | 阿兹加洛D01 绿 | `glaive_1h_artifactazgalor_d_01.mdx` | `glaive_1h_artifactazgalor_d_01green` | ✔ 正常 |
| 420103 | 战刃 | 64771 | 阿兹加洛D01 紫 | `glaive_1h_artifactazgalor_d_01.mdx` | `glaive_1h_artifactazgalor_d_01purple` | ✔ 正常 |
| 420104 | 战刃 | 64772 | 阿兹加洛D01 红 | `glaive_1h_artifactazgalor_d_01.mdx` | `glaive_1h_artifactazgalor_d_01red` | ✔ 正常 |
| 420105 | 战刃 | 64773 | 阿兹加洛D02 蓝 | `glaive_1h_artifactazgalor_d_02.mdx` | `glaive_1h_artifactazgalor_d_02blue` | ✔ 正常 |
| 420106 | 战刃 | 61040, 99945, 100522, 101115, 101692, 102269, 103469 | 阿兹加洛D02 绿 | `glaive_1h_artifactazgalor_d_02.mdx` | `glaive_1h_artifactazgalor_d_02green` | ◑ 已使用 |
| 420107 | 战刃 | 64774 | 阿兹加洛D02 紫 | `glaive_1h_artifactazgalor_d_02.mdx` | `glaive_1h_artifactazgalor_d_02purple` | ✔ 正常 |
| 420108 | 战刃 | 61042, 99947, 100524, 101117, 101694, 102271, 103471 | 阿兹加洛D02 红 | `glaive_1h_artifactazgalor_d_02.mdx` | `glaive_1h_artifactazgalor_d_02red` | ◑ 已使用 |
| 420109 | 战刃 | 64775 | 阿兹加洛D03 暗红 | `glaive_1h_artifactazgalor_d_03.mdx` | `glaive_1h_artifactazgalor_d_03darkred` | ✔ 正常 |
| 420110 | 战刃 | 64776 | 阿兹加洛D03 绿 | `glaive_1h_artifactazgalor_d_03.mdx` | `glaive_1h_artifactazgalor_d_03green` | ✔ 正常 |
| 420111 | 战刃 | 64777 | 阿兹加洛D03 紫 | `glaive_1h_artifactazgalor_d_03.mdx` | `glaive_1h_artifactazgalor_d_03puple` | ✔ 正常 |
| 420112 | 战刃 | 64778 | 阿兹加洛D03 红 | `glaive_1h_artifactazgalor_d_03.mdx` | `glaive_1h_artifactazgalor_d_03red` | ✔ 正常 |
| 420113 | 战刃 | 64779 | 阿兹加洛D04 暗红 | `glaive_1h_artifactazgalor_d_04.mdx` | `glaive_1h_artifactazgalor_d_04darkred` | ✔ 正常 |
| 420114 | 战刃 | 64780 | 阿兹加洛D04 绿 | `glaive_1h_artifactazgalor_d_04.mdx` | `glaive_1h_artifactazgalor_d_04green` | ✔ 正常 |
| 420115 | 战刃 | 64781 | 阿兹加洛D04 橙 | `glaive_1h_artifactazgalor_d_04.mdx` | `glaive_1h_artifactazgalor_d_04orange` | ✔ 正常 |
| 420116 | 战刃 | 64782 | 阿兹加洛D04 紫 | `glaive_1h_artifactazgalor_d_04.mdx` | `glaive_1h_artifactazgalor_d_04purple` | ✔ 正常 |
| 420117 | 战刃 | 64783 | 阿兹加洛D05 蓝 | `glaive_1h_artifactazgalor_d_05.mdx` | `glaive_1h_artifactazgalor_d_05blue` | ✔ 正常 |
| 420118 | 战刃 | 64784 | 阿兹加洛D05 绿 | `glaive_1h_artifactazgalor_d_05.mdx` | `glaive_1h_artifactazgalor_d_05green` | ✔ 正常 |
| 420119 | 战刃 | 64785 | 阿兹加洛D05 紫 | `glaive_1h_artifactazgalor_d_05.mdx` | `glaive_1h_artifactazgalor_d_05purple` | ✔ 正常 |
| 420120 | 战刃 | 64786 | 阿兹加洛D05 黄 | `glaive_1h_artifactazgalor_d_05.mdx` | `glaive_1h_artifactazgalor_d_05yellow` | ✔ 正常 |
| 420121 | 战刃 | 64787 | 阿兹加洛D06 水蓝 | `glaive_1h_artifactazgalor_d_06.mdx` | `glaive_1h_artifactazgalor_d_06_aqua` | ✔ 正常 |
| 420122 | 战刃 | 64788 | 阿兹加洛D06 绿 | `glaive_1h_artifactazgalor_d_06.mdx` | `glaive_1h_artifactazgalor_d_06_green` | ✔ 正常 |
| 420123 | 战刃 | 64789 | 阿兹加洛D06 紫 | `glaive_1h_artifactazgalor_d_06.mdx` | `glaive_1h_artifactazgalor_d_06_purple` | ✔ 正常 |
| 420124 | 战刃 | 64790 | 阿兹加洛D06 红 | `glaive_1h_artifactazgalor_d_06.mdx` | `glaive_1h_artifactazgalor_d_06_red` | ✔ 正常 |
| 421011 | 战刃 | 64791 | 左·阿尔德罗奇D01_1 | `L_Glaive_1H_ArtifactAldrochi_D_01_1.mdx` | — | ✔ 正常 |
| 421012 | 战刃 | 64792 | 左·阿尔德罗奇D01_2 | `L_Glaive_1H_ArtifactAldrochi_D_01_2.mdx` | — | ✔ 正常 |
| 421013 | 战刃 | 64793 | 左·阿尔德罗奇D01_3 | `L_Glaive_1H_ArtifactAldrochi_D_01_3.mdx` | — | ✔ 正常 |
| 421014 | 战刃 | 64794 | 左·阿尔德罗奇D01_4 | `L_Glaive_1H_ArtifactAldrochi_D_01_4.mdx` | — | ✔ 正常 |
| 421015 | 战刃 | 64795 | 左·阿尔德罗奇D02_1 | `L_Glaive_1H_ArtifactAldrochi_D_02_1.mdx` | — | ✔ 正常 |
| 421016 | 战刃 | 64796 | 左·阿尔德罗奇D02_2 | `L_Glaive_1H_ArtifactAldrochi_D_02_2.mdx` | — | ✔ 正常 |
| 421017 | 战刃 | 64797 | 左·阿尔德罗奇D02_3 | `L_Glaive_1H_ArtifactAldrochi_D_02_3.mdx` | — | ✔ 正常 |
| 421018 | 战刃 | 64798 | 左·阿尔德罗奇D02_4 | `L_Glaive_1H_ArtifactAldrochi_D_02_4.mdx` | — | ✔ 正常 |
| 421019 | 战刃 | 64799 | 左·阿尔德罗奇D03_1 | `L_glaive_1h_artifactaldrochi_d_03_1.mdx` | — | ✔ 正常 |
| 421020 | 战刃 | 64800 | 左·阿尔德罗奇D03_2 | `L_glaive_1h_artifactaldrochi_d_03_2.mdx` | — | ✔ 正常 |
| 421021 | 战刃 | 64801 | 左·阿尔德罗奇D03_3 | `L_glaive_1h_artifactaldrochi_d_03_3.mdx` | — | ✔ 正常 |
| 421022 | 战刃 | 64802 | 左·阿尔德罗奇D03_4 | `L_glaive_1h_artifactaldrochi_d_03_4.mdx` | — | ✔ 正常 |
| 421023 | 战刃 | 64803 | 左·阿尔德罗奇D04_1 | `L_Glaive_1H_ArtifactAldrochi_D_04_1.mdx` | — | ✔ 正常 |
| 421024 | 战刃 | 64804 | 左·阿尔德罗奇D04_2 | `L_Glaive_1H_ArtifactAldrochi_D_04_2.mdx` | — | ✔ 正常 |
| 421025 | 战刃 | 64805 | 左·阿尔德罗奇D04_3 | `L_Glaive_1H_ArtifactAldrochi_D_04_3.mdx` | — | ✔ 正常 |
| 421026 | 战刃 | 64806 | 左·阿尔德罗奇D04_4 | `L_Glaive_1H_ArtifactAldrochi_D_04_4.mdx` | — | ✔ 正常 |
| 421027 | 战刃 | 64807 | 左·阿尔德罗奇D05_1 | `L_Glaive_1H_ArtifactAldrochi_D_05_1.mdx` | — | ✔ 正常 |
| 421028 | 战刃 | 64808 | 左·阿尔德罗奇D05_2 | `L_Glaive_1H_ArtifactAldrochi_D_05_2.mdx` | — | ✔ 正常 |
| 421029 | 战刃 | 64809 | 左·阿尔德罗奇D05_3 | `L_Glaive_1H_ArtifactAldrochi_D_05_3.mdx` | — | ✔ 正常 |
| 421030 | 战刃 | 64810 | 左·阿尔德罗奇D05_4 | `L_Glaive_1H_ArtifactAldrochi_D_05_4.mdx` | — | ✔ 正常 |
| 421035 | 战刃 | 64811 | 左·阿兹加洛D01_1 | `L_Glaive_1H_ArtifactAzgalor_D_01_1.mdx` | — | ✔ 正常 |
| 421036 | 战刃 | 64812 | 左·阿兹加洛D01_2 | `L_Glaive_1H_ArtifactAzgalor_D_01_2.mdx` | — | ✔ 正常 |
| 421037 | 战刃 | 64813 | 左·阿兹加洛D01_3 | `L_Glaive_1H_ArtifactAzgalor_D_01_3.mdx` | — | ✔ 正常 |
| 421038 | 战刃 | 64814 | 左·阿兹加洛D01_4 | `L_Glaive_1H_ArtifactAzgalor_D_01_4.mdx` | — | ✔ 正常 |
| 421039 | 战刃 | 61041, 99946, 100523, 101116, 101693, 102270, 103470 | 左·阿兹加洛D02_1 | `L_Glaive_1H_ArtifactAzgalor_D_02_1.mdx` | — | ◑ 已使用 |
| 421040 | 战刃 | 64815 | 左·阿兹加洛D02_2 | `L_Glaive_1H_ArtifactAzgalor_D_02_2.mdx` | — | ✔ 正常 |
| 421041 | 战刃 | 64816 | 左·阿兹加洛D02_3 | `L_Glaive_1H_ArtifactAzgalor_D_02_3.mdx` | — | ✔ 正常 |
| 421042 | 战刃 | 61043, 99948, 100525, 101118, 101695, 102272, 103472 | 左·阿兹加洛D02_4 | `L_Glaive_1H_ArtifactAzgalor_D_02_4.mdx` | — | ◑ 已使用 |
| 421031 | 战刃 | 64817 | 左·阿兹加洛D03_1 | `L_Glaive_1H_ArtifactAzgalor_D_03_1.mdx` | — | ✔ 正常 |
| 421032 | 战刃 | 64818 | 左·阿兹加洛D03_2 | `L_Glaive_1H_ArtifactAzgalor_D_03_2.mdx` | — | ✔ 正常 |
| 421033 | 战刃 | 64819 | 左·阿兹加洛D03_3 | `L_Glaive_1H_ArtifactAzgalor_D_03_3.mdx` | — | ✔ 正常 |
| 421034 | 战刃 | 64820 | 左·阿兹加洛D03_4 | `L_Glaive_1H_ArtifactAzgalor_D_03_4.mdx` | — | ✔ 正常 |
| 421043 | 战刃 | 64821 | 左·阿兹加洛D04_1 | `L_Glaive_1H_ArtifactAzgalor_D_04_1.mdx` | — | ✔ 正常 |
| 421044 | 战刃 | 64822 | 左·阿兹加洛D04_2 | `L_Glaive_1H_ArtifactAzgalor_D_04_2.mdx` | — | ✔ 正常 |
| 421045 | 战刃 | 64823 | 左·阿兹加洛D04_3 | `L_Glaive_1H_ArtifactAzgalor_D_04_3.mdx` | — | ✔ 正常 |
| 421046 | 战刃 | 64824 | 左·阿兹加洛D04_4 | `L_Glaive_1H_ArtifactAzgalor_D_04_4.mdx` | — | ✔ 正常 |
| 421047 | 战刃 | 64825 | 左·阿兹加洛D05_1 | `L_Glaive_1H_ArtifactAzgalor_D_05_1.mdx` | — | ✔ 正常 |
| 421048 | 战刃 | 64826 | 左·阿兹加洛D05_2 | `L_Glaive_1H_ArtifactAzgalor_D_05_2.mdx` | — | ✔ 正常 |
| 421049 | 战刃 | 64827 | 左·阿兹加洛D05_3 | `L_Glaive_1H_ArtifactAzgalor_D_05_3.mdx` | — | ✔ 正常 |
| 421050 | 战刃 | 64828 | 左·阿兹加洛D05_4 | `L_Glaive_1H_ArtifactAzgalor_D_05_4.mdx` | — | ✔ 正常 |
| 421051 | 战刃 | 64829 | 左·阿兹加洛D06_1 | `L_Glaive_1H_ArtifactAzgalor_D_06_1.mdx` | — | ✔ 正常 |
| 421052 | 战刃 | 64830 | 左·阿兹加洛D06_2 | `L_Glaive_1H_ArtifactAzgalor_D_06_2.mdx` | — | ✔ 正常 |
| 421053 | 战刃 | 64831 | 左·阿兹加洛D06_3 | `L_Glaive_1H_ArtifactAzgalor_D_06_3.mdx` | — | ✔ 正常 |
| 421054 | 战刃 | 64832 | 左·阿兹加洛D06_4 | `L_Glaive_1H_ArtifactAzgalor_D_06_4.mdx` | — | ✔ 正常 |

## 十一、弓（29 条：已使用 2 / 新生成 27）

> `class=2, subclass=2, InventoryType=15, sheath=0, Material=2`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420028 | 弓 | 64833 | 军团D06 蓝 | `bow_1h_artifactlegion_d_06.mdx` | `bow_1h_artifactlegion_d_06_blue` | ✔ 正常 |
| 420029 | 弓 | 64834 | 军团D06 紫 | `bow_1h_artifactlegion_d_06.mdx` | `bow_1h_artifactlegion_d_06_purple` | ✔ 正常 |
| 420030 | 弓 | 64835 | 军团D06 钢 | `bow_1h_artifactlegion_d_06.mdx` | `bow_1h_artifactlegion_d_06_steel` | ✔ 正常 |
| 420031 | 弓 | 64836 | 风行者D01 蓝 | `bow_1h_artifactwindrunner_d_01.mdx` | `bow_1h_artifactwindrunner_d_01_blue` | ✔ 正常 |
| 420032 | 弓 | 64837 | 风行者D01 绿 | `bow_1h_artifactwindrunner_d_01.mdx` | `bow_1h_artifactwindrunner_d_01_green` | ✔ 正常 |
| 420033 | 弓 | 64838 | 风行者D01 紫 | `bow_1h_artifactwindrunner_d_01.mdx` | `bow_1h_artifactwindrunner_d_01_purple` | ✔ 正常 |
| 420034 | 弓 | 64839 | 风行者D01 红 | `bow_1h_artifactwindrunner_d_01.mdx` | `bow_1h_artifactwindrunner_d_01_red` | ✔ 正常 |
| 420035 | 弓 | 64840 | 风行者D02 蓝 | `bow_1h_artifactwindrunner_d_02.mdx` | `bow_1h_artifactwindrunner_d_02_blue` | ✔ 正常 |
| 420036 | 弓 | 64841 | 风行者D02 绿 | `bow_1h_artifactwindrunner_d_02.mdx` | `bow_1h_artifactwindrunner_d_02_green` | ✔ 正常 |
| 420037 | 弓 | 64842 | 风行者D02 紫 | `bow_1h_artifactwindrunner_d_02.mdx` | `bow_1h_artifactwindrunner_d_02_purple` | ✔ 正常 |
| 420038 | 弓 | 64843 | 风行者D02 红 | `bow_1h_artifactwindrunner_d_02.mdx` | `bow_1h_artifactwindrunner_d_02_red` | ✔ 正常 |
| 420039 | 弓 | 64844 | 风行者D03 水蓝 | `bow_1h_artifactwindrunner_d_03.mdx` | `bow_1h_artifactwindrunner_d_03_aqua` | ✔ 正常 |
| 420040 | 弓 | 64845 | 风行者D03 蓝 | `bow_1h_artifactwindrunner_d_03.mdx` | `bow_1h_artifactwindrunner_d_03_blue` | ✔ 正常 |
| 420041 | 弓 | 64846 | 风行者D03 绿 | `bow_1h_artifactwindrunner_d_03.mdx` | `bow_1h_artifactwindrunner_d_03_green` | ✔ 正常 |
| 420042 | 弓 | 64847 | 风行者D03 紫 | `bow_1h_artifactwindrunner_d_03.mdx` | `bow_1h_artifactwindrunner_d_03_purple` | ✔ 正常 |
| 420043 | 弓 | 64848 | 风行者D04 蓝 | `bow_1h_artifactwindrunner_d_04.mdx` | `bow_1h_artifactwindrunner_d_04_blue` | ✔ 正常 |
| 420044 | 弓 | 64849 | 风行者D04 绿 | `bow_1h_artifactwindrunner_d_04.mdx` | `bow_1h_artifactwindrunner_d_04_green` | ✔ 正常 |
| 420045 | 弓 | 50034 | 风行者D04 紫 | `bow_1h_artifactwindrunner_d_04.mdx` | `bow_1h_artifactwindrunner_d_04_purple` | ◑ 已使用 |
| 420046 | 弓 | 50776 | 风行者D04 红 | `bow_1h_artifactwindrunner_d_04.mdx` | `bow_1h_artifactwindrunner_d_04_red` | ◑ 已使用 |
| 420047 | 弓 | 64850 | 风行者D05 蓝 | `bow_1h_artifactwindrunner_d_05.mdx` | `bow_1h_artifactwindrunner_d_05_blue` | ✔ 正常 |
| 420048 | 弓 | 64851 | 风行者D05 绿 | `bow_1h_artifactwindrunner_d_05.mdx` | `bow_1h_artifactwindrunner_d_05_green` | ✔ 正常 |
| 420049 | 弓 | 64852 | 风行者D05 红 | `bow_1h_artifactwindrunner_d_05.mdx` | `bow_1h_artifactwindrunner_d_05_red` | ✔ 正常 |
| 420050 | 弓 | 64853 | 风行者D05 黄 | `bow_1h_artifactwindrunner_d_05.mdx` | `bow_1h_artifactwindrunner_d_05_yellow` | ✔ 正常 |
| 420051 | 弓 | 64854 | 风行者D06 绿 | `bow_1h_artifactwindrunner_d_06.mdx` | `bow_1h_artifactwindrunner_d_06_green` | ✔ 正常 |
| 420052 | 弓 | 64855 | 风行者D06 粉 | `bow_1h_artifactwindrunner_d_06.mdx` | `bow_1h_artifactwindrunner_d_06_pink` | ✔ 正常 |
| 420053 | 弓 | 64856 | 风行者D06 紫 | `bow_1h_artifactwindrunner_d_06.mdx` | `bow_1h_artifactwindrunner_d_06_purple` | ✔ 正常 |
| 420054 | 弓 | 64857 | 风行者D06 红 | `bow_1h_artifactwindrunner_d_06.mdx` | `bow_1h_artifactwindrunner_d_06_red` | ✔ 正常 |
| 420055 | 弓 | 64858 | 德拉诺挑战D01 | `bow_1h_draenorchallenge_d_01.mdx` | — | ✔ 正常 |
| 100003 | 弓 | 64859 | 后羿之弓 | `houyiwuqi.mdx` | — | ✔ 正常 |

## 十二、弩（4 条：已使用 0 / 新生成 4）

> `class=2, subclass=18, InventoryType=26, sheath=0, Material=2`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420056 | 弩 | 64860 | 风行者D05 蓝 | `bow_2h_crossbow_artifactwindrunner_d_05.mdx` | `bow_2h_crossbow_artifactwindrunner_d_05_blue` | ✔ 正常 |
| 420057 | 弩 | 64861 | 风行者D05 绿 | `bow_2h_crossbow_artifactwindrunner_d_05.mdx` | `bow_2h_crossbow_artifactwindrunner_d_05_green` | ✔ 正常 |
| 420058 | 弩 | 64862 | 风行者D05 红 | `bow_2h_crossbow_artifactwindrunner_d_05.mdx` | `bow_2h_crossbow_artifactwindrunner_d_05_red` | ✔ 正常 |
| 420059 | 弩 | 64863 | 风行者D05 青 | `bow_2h_crossbow_artifactwindrunner_d_05.mdx` | `bow_2h_crossbow_artifactwindrunner_d_05_teal` | ✔ 正常 |

## 十三、枪械（21 条：已使用 0 / 新生成 21）

> `class=2, subclass=3, InventoryType=26, sheath=0, Material=1`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420060 | 枪械 | 64864 | 军团D01 蓝 | `firearm_2h_artifactlegion_d_01.mdx` | `firearm_2h_artifactlegion_d_01blue` | ✔ 正常 |
| 420061 | 枪械 | 64865 | 军团D01 紫 | `firearm_2h_artifactlegion_d_01.mdx` | `firearm_2h_artifactlegion_d_01purple` | ✔ 正常 |
| 420062 | 枪械 | 64866 | 军团D01 红 | `firearm_2h_artifactlegion_d_01.mdx` | `firearm_2h_artifactlegion_d_01red` | ✔ 正常 |
| 420063 | 枪械 | 64867 | 军团D01 黄 | `firearm_2h_artifactlegion_d_01.mdx` | `firearm_2h_artifactlegion_d_01yellow` | ✔ 正常 |
| 420064 | 枪械 | 64868 | 军团D02 蓝 | `firearm_2h_artifactlegion_d_02.mdx` | `firearm_2h_artifactlegion_d_02blue` | ✔ 正常 |
| 420065 | 枪械 | 64869 | 军团D02 绿 | `firearm_2h_artifactlegion_d_02.mdx` | `firearm_2h_artifactlegion_d_02green` | ✔ 正常 |
| 420066 | 枪械 | 64870 | 军团D02 紫 | `firearm_2h_artifactlegion_d_02.mdx` | `firearm_2h_artifactlegion_d_02purple` | ✔ 正常 |
| 420067 | 枪械 | 64871 | 军团D02 红 | `firearm_2h_artifactlegion_d_02.mdx` | `firearm_2h_artifactlegion_d_02red` | ✔ 正常 |
| 420068 | 枪械 | 64872 | 军团D03 蓝 | `firearm_2h_artifactlegion_d_03.mdx` | `firearm_2h_artifactlegion_d_03_blue` | ✔ 正常 |
| 420069 | 枪械 | 64873 | 军团D03 绿 | `firearm_2h_artifactlegion_d_03.mdx` | `firearm_2h_artifactlegion_d_03_green` | ✔ 正常 |
| 420070 | 枪械 | 64874 | 军团D03 红 | `firearm_2h_artifactlegion_d_03.mdx` | `firearm_2h_artifactlegion_d_03_red` | ✔ 正常 |
| 420071 | 枪械 | 64875 | 军团D03 黄 | `firearm_2h_artifactlegion_d_03.mdx` | `firearm_2h_artifactlegion_d_03_yellow` | ✔ 正常 |
| 420072 | 枪械 | 64876 | 军团D04 蓝 | `firearm_2h_artifactlegion_d_04.mdx` | `firearm_2h_artifactlegion_d_04_blue` | ✔ 正常 |
| 420073 | 枪械 | 64877 | 军团D04 绿 | `firearm_2h_artifactlegion_d_04.mdx` | `firearm_2h_artifactlegion_d_04_green` | ✔ 正常 |
| 420074 | 枪械 | 64878 | 军团D04 紫 | `firearm_2h_artifactlegion_d_04.mdx` | `firearm_2h_artifactlegion_d_04_purple` | ✔ 正常 |
| 420075 | 枪械 | 64879 | 军团D04 黄 | `firearm_2h_artifactlegion_d_04.mdx` | `firearm_2h_artifactlegion_d_04_yellow` | ✔ 正常 |
| 420076 | 枪械 | 64880 | 军团D05 蓝 | `firearm_2h_artifactlegion_d_05.mdx` | `firearm_2h_artifactlegion_d_05_blue` | ✔ 正常 |
| 420077 | 枪械 | 64881 | 军团D05 绿 | `firearm_2h_artifactlegion_d_05.mdx` | `firearm_2h_artifactlegion_d_05_green` | ✔ 正常 |
| 420078 | 枪械 | 64882 | 军团D05 紫 | `firearm_2h_artifactlegion_d_05.mdx` | `firearm_2h_artifactlegion_d_05_purple` | ✔ 正常 |
| 420079 | 枪械 | 64883 | 军团D05 红 | `firearm_2h_artifactlegion_d_05.mdx` | `firearm_2h_artifactlegion_d_05_red` | ✔ 正常 |
| 420080 | 枪械 | 64884 | 德拉诺挑战D01 | `firearm_2h_rifle_draenorchallenge_d_01.mdx` | — | ✔ 正常 |

## 十四、盾牌（107 条：已使用 4 / 新生成 103）

> `class=4, subclass=6, InventoryType=14, sheath=4, Material=1`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420470 | 盾牌 | 64885 | 艾萨拉D01 蓝 | `shield_1h_artifactazshara_d_01.mdx` | `shield_1h_artifactazshara_d_01blue` | ✔ 正常 |
| 420471 | 盾牌 | 64886 | 艾萨拉D01 绿 | `shield_1h_artifactazshara_d_01.mdx` | `shield_1h_artifactazshara_d_01green` | ✔ 正常 |
| 420472 | 盾牌 | 64887 | 艾萨拉D01 橙 | `shield_1h_artifactazshara_d_01.mdx` | `shield_1h_artifactazshara_d_01orange` | ✔ 正常 |
| 420473 | 盾牌 | 64888 | 艾萨拉D01 紫 | `shield_1h_artifactazshara_d_01.mdx` | `shield_1h_artifactazshara_d_01purple` | ✔ 正常 |
| 420474 | 盾牌 | 64889 | 艾萨拉D03 蓝 | `shield_1h_artifactazshara_d_03.mdx` | `shield_1h_artifactazshara_d_03blue` | ✔ 正常 |
| 420475 | 盾牌 | 64890 | 艾萨拉D03 绿 | `shield_1h_artifactazshara_d_03.mdx` | `shield_1h_artifactazshara_d_03green` | ✔ 正常 |
| 420476 | 盾牌 | 64891 | 艾萨拉D03 紫 | `shield_1h_artifactazshara_d_03.mdx` | `shield_1h_artifactazshara_d_03purple` | ✔ 正常 |
| 420477 | 盾牌 | 64892 | 艾萨拉D03 红 | `shield_1h_artifactazshara_d_03.mdx` | `shield_1h_artifactazshara_d_03red` | ✔ 正常 |
| 420478 | 盾牌 | 64893 | 艾萨拉D03 黄 | `shield_1h_artifactazshara_d_03.mdx` | `shield_1h_artifactazshara_d_03yellow` | ✔ 正常 |
| 420479 | 盾牌 | 64894 | 艾萨拉D03 蓝 | `shield_1h_artifactazshara_d_03.mdx` | `shield_1h_artifactazshara_d_03_blue` | ✔ 正常 |
| 420480 | 盾牌 | 64895 | 艾萨拉D03 金 | `shield_1h_artifactazshara_d_03.mdx` | `shield_1h_artifactazshara_d_03_gold` | ✔ 正常 |
| 420481 | 盾牌 | 64896 | 艾萨拉D03 绿 | `shield_1h_artifactazshara_d_03.mdx` | `shield_1h_artifactazshara_d_03_green` | ✔ 正常 |
| 420482 | 盾牌 | 64897 | 艾萨拉D03 红 | `shield_1h_artifactazshara_d_03.mdx` | `shield_1h_artifactazshara_d_03_red` | ✔ 正常 |
| 420483 | 盾牌 | 64898 | 艾萨拉D04 蓝 | `shield_1h_artifactazshara_d_04.mdx` | `shield_1h_artifactazshara_d_04blue` | ✔ 正常 |
| 420484 | 盾牌 | 64899 | 艾萨拉D04 绿 | `shield_1h_artifactazshara_d_04.mdx` | `shield_1h_artifactazshara_d_04green` | ✔ 正常 |
| 420485 | 盾牌 | 64900 | 艾萨拉D04 红 | `shield_1h_artifactazshara_d_04.mdx` | `shield_1h_artifactazshara_d_04red` | ✔ 正常 |
| 420486 | 盾牌 | 64901 | 艾萨拉D04 黄 | `shield_1h_artifactazshara_d_04.mdx` | `shield_1h_artifactazshara_d_04yellow` | ✔ 正常 |
| 420487 | 盾牌 | 64902 | 艾萨拉D04 绿 | `shield_1h_artifactazshara_d_04.mdx` | `shield_1h_artifactazshara_d_04_green` | ✔ 正常 |
| 420488 | 盾牌 | 64903 | 艾萨拉D04 橙 | `shield_1h_artifactazshara_d_04.mdx` | `shield_1h_artifactazshara_d_04_orange` | ✔ 正常 |
| 420489 | 盾牌 | 64904 | 艾萨拉D04 红 | `shield_1h_artifactazshara_d_04.mdx` | `shield_1h_artifactazshara_d_04_red` | ✔ 正常 |
| 420490 | 盾牌 | 64905 | 艾萨拉D04 黄 | `shield_1h_artifactazshara_d_04.mdx` | `shield_1h_artifactazshara_d_04_yellow` | ✔ 正常 |
| 420491 | 盾牌 | 64906 | 艾萨拉D05 蓝 | `shield_1h_artifactazshara_d_05.mdx` | `shield_1h_artifactazshara_d_05blue` | ✔ 正常 |
| 420492 | 盾牌 | 64907 | 艾萨拉D05 绿 | `shield_1h_artifactazshara_d_05.mdx` | `shield_1h_artifactazshara_d_05green` | ✔ 正常 |
| 420493 | 盾牌 | 64908 | 艾萨拉D05 红 | `shield_1h_artifactazshara_d_05.mdx` | `shield_1h_artifactazshara_d_05red` | ✔ 正常 |
| 420494 | 盾牌 | 64909 | 艾萨拉D05 黄 | `shield_1h_artifactazshara_d_05.mdx` | `shield_1h_artifactazshara_d_05yellow` | ✔ 正常 |
| 420495 | 盾牌 | 64910 | 艾萨拉D05 蓝 | `shield_1h_artifactazshara_d_05.mdx` | `shield_1h_artifactazshara_d_05_blue` | ✔ 正常 |
| 420496 | 盾牌 | 64911 | 艾萨拉D05 绿 | `shield_1h_artifactazshara_d_05.mdx` | `shield_1h_artifactazshara_d_05_green` | ✔ 正常 |
| 420497 | 盾牌 | 64912 | 艾萨拉D05 红 | `shield_1h_artifactazshara_d_05.mdx` | `shield_1h_artifactazshara_d_05_red` | ✔ 正常 |
| 420498 | 盾牌 | 64913 | 艾萨拉D05 黄 | `shield_1h_artifactazshara_d_05.mdx` | `shield_1h_artifactazshara_d_05_yellow` | ✔ 正常 |
| 420499 | 盾牌 | 64914 | 艾萨拉D06 蓝 | `shield_1h_artifactazshara_d_06.mdx` | `shield_1h_artifactazshara_d_06_blue` | ✔ 正常 |
| 420500 | 盾牌 | 64915 | 艾萨拉D06 绿 | `shield_1h_artifactazshara_d_06.mdx` | `shield_1h_artifactazshara_d_06_green` | ✔ 正常 |
| 420501 | 盾牌 | 64916 | 艾萨拉D06 光 | `shield_1h_artifactazshara_d_06.mdx` | `shield_1h_artifactazshara_d_06_light` | ✔ 正常 |
| 420502 | 盾牌 | 64917 | 艾萨拉D06 红 | `shield_1h_artifactazshara_d_06.mdx` | `shield_1h_artifactazshara_d_06_red` | ✔ 正常 |
| 420503 | 盾牌 | 64918 | 玛格纳D01 蓝 | `shield_1h_artifactmagnar_d_01.mdx` | `shield_1h_artifactmagnar_d_01blue` | ✔ 正常 |
| 420504 | 盾牌 | 64919 | 玛格纳D01 绿 | `shield_1h_artifactmagnar_d_01.mdx` | `shield_1h_artifactmagnar_d_01green` | ✔ 正常 |
| 420505 | 盾牌 | 64920 | 玛格纳D01 紫 | `shield_1h_artifactmagnar_d_01.mdx` | `shield_1h_artifactmagnar_d_01purple` | ✔ 正常 |
| 420506 | 盾牌 | 64921 | 玛格纳D01 红 | `shield_1h_artifactmagnar_d_01.mdx` | `shield_1h_artifactmagnar_d_01red` | ✔ 正常 |
| 420507 | 盾牌 | 64922 | 玛格纳D02 蓝 | `shield_1h_artifactmagnar_d_02.mdx` | `shield_1h_artifactmagnar_d_02blue` | ✔ 正常 |
| 420508 | 盾牌 | 64923 | 玛格纳D02 绿 | `shield_1h_artifactmagnar_d_02.mdx` | `shield_1h_artifactmagnar_d_02green` | ✔ 正常 |
| 420509 | 盾牌 | 64924 | 玛格纳D02 紫 | `shield_1h_artifactmagnar_d_02.mdx` | `shield_1h_artifactmagnar_d_02purple` | ✔ 正常 |
| 420510 | 盾牌 | 64925 | 玛格纳D02 红 | `shield_1h_artifactmagnar_d_02.mdx` | `shield_1h_artifactmagnar_d_02red` | ✔ 正常 |
| 420511 | 盾牌 | 64926 | 玛格纳D03 蓝 | `shield_1h_artifactmagnar_d_03.mdx` | `shield_1h_artifactmagnar_d_03blue` | ✔ 正常 |
| 420512 | 盾牌 | 64927 | 玛格纳D03 绿 | `shield_1h_artifactmagnar_d_03.mdx` | `shield_1h_artifactmagnar_d_03green` | ✔ 正常 |
| 420513 | 盾牌 | 64928 | 玛格纳D03 红 | `shield_1h_artifactmagnar_d_03.mdx` | `shield_1h_artifactmagnar_d_03red` | ✔ 正常 |
| 420514 | 盾牌 | 64929 | 玛格纳D03 黄 | `shield_1h_artifactmagnar_d_03.mdx` | `shield_1h_artifactmagnar_d_03yellow` | ✔ 正常 |
| 420515 | 盾牌 | 64930 | 玛格纳D04 蓝 | `shield_1h_artifactmagnar_d_04.mdx` | `shield_1h_artifactmagnar_d_04blue` | ✔ 正常 |
| 420516 | 盾牌 | 64931 | 玛格纳D04 绿 | `shield_1h_artifactmagnar_d_04.mdx` | `shield_1h_artifactmagnar_d_04green` | ✔ 正常 |
| 420517 | 盾牌 | 64932 | 玛格纳D04 紫 | `shield_1h_artifactmagnar_d_04.mdx` | `shield_1h_artifactmagnar_d_04purple` | ✔ 正常 |
| 420518 | 盾牌 | 64933 | 玛格纳D04 红 | `shield_1h_artifactmagnar_d_04.mdx` | `shield_1h_artifactmagnar_d_04red` | ✔ 正常 |
| 420519 | 盾牌 | 64934 | 玛格纳D05 蓝 | `shield_1h_artifactmagnar_d_05.mdx` | `shield_1h_artifactmagnar_d_05blue` | ✔ 正常 |
| 420520 | 盾牌 | 64935 | 玛格纳D05 绿 | `shield_1h_artifactmagnar_d_05.mdx` | `shield_1h_artifactmagnar_d_05green` | ✔ 正常 |
| 420521 | 盾牌 | 64936 | 玛格纳D05 橙 | `shield_1h_artifactmagnar_d_05.mdx` | `shield_1h_artifactmagnar_d_05orange` | ✔ 正常 |
| 420522 | 盾牌 | 64937 | 玛格纳D05 青 | `shield_1h_artifactmagnar_d_05.mdx` | `shield_1h_artifactmagnar_d_05teal` | ✔ 正常 |
| 420523 | 盾牌 | 64938 | 诺甘农D01 金 | `shield_1h_artifactnorgannon_d_01.mdx` | `shield_1h_artifactnorgannon_d_01gold` | ✔ 正常 |
| 420524 | 盾牌 | 64939 | 诺甘农D01 绿 | `shield_1h_artifactnorgannon_d_01.mdx` | `shield_1h_artifactnorgannon_d_01green` | ✔ 正常 |
| 420525 | 盾牌 | 64940 | 诺甘农D01 红 | `shield_1h_artifactnorgannon_d_01.mdx` | `shield_1h_artifactnorgannon_d_01red` | ✔ 正常 |
| 420526 | 盾牌 | 64941 | 诺甘农D01 黄 | `shield_1h_artifactnorgannon_d_01.mdx` | `shield_1h_artifactnorgannon_d_01yellow` | ✔ 正常 |
| 420527 | 盾牌 | 64942 | 诺甘农D02 蓝 | `shield_1h_artifactnorgannon_d_02.mdx` | `shield_1h_artifactnorgannon_d_02blue` | ✔ 正常 |
| 420528 | 盾牌 | 64943 | 诺甘农D02 红 | `shield_1h_artifactnorgannon_d_02.mdx` | `shield_1h_artifactnorgannon_d_02red` | ✔ 正常 |
| 420529 | 盾牌 | 49976 | 诺甘农D02 白 | `shield_1h_artifactnorgannon_d_02.mdx` | `shield_1h_artifactnorgannon_d_02white` | ◑ 已使用 |
| 420530 | 盾牌 | 50794 | 诺甘农D02 黄 | `shield_1h_artifactnorgannon_d_02.mdx` | `shield_1h_artifactnorgannon_d_02yellow` | ◑ 已使用 |
| 420531 | 盾牌 | 64944 | 诺甘农D03 蓝 | `shield_1h_artifactnorgannon_d_03.mdx` | `shield_1h_artifactnorgannon_d_03blue` | ✔ 正常 |
| 420532 | 盾牌 | 64945 | 诺甘农D03 绿 | `shield_1h_artifactnorgannon_d_03.mdx` | `shield_1h_artifactnorgannon_d_03green` | ✔ 正常 |
| 420533 | 盾牌 | 64946 | 诺甘农D03 红 | `shield_1h_artifactnorgannon_d_03.mdx` | `shield_1h_artifactnorgannon_d_03red` | ✔ 正常 |
| 420534 | 盾牌 | 110003, 110013 | 诺甘农D03 黄 | `shield_1h_artifactnorgannon_d_03.mdx` | `shield_1h_artifactnorgannon_d_03yellow` | ◑ 已使用 |
| 420535 | 盾牌 | 64947 | 诺甘农D04 蓝 | `shield_1h_artifactnorgannon_d_04.mdx` | `shield_1h_artifactnorgannon_d_04blue` | ✔ 正常 |
| 420536 | 盾牌 | 64948 | 诺甘农D04 紫 | `shield_1h_artifactnorgannon_d_04.mdx` | `shield_1h_artifactnorgannon_d_04purple` | ✔ 正常 |
| 420537 | 盾牌 | 64949 | 诺甘农D04 红 | `shield_1h_artifactnorgannon_d_04.mdx` | `shield_1h_artifactnorgannon_d_04red` | ✔ 正常 |
| 420538 | 盾牌 | 64950 | 诺甘农D04 黄 | `shield_1h_artifactnorgannon_d_04.mdx` | `shield_1h_artifactnorgannon_d_04yellow` | ✔ 正常 |
| 420539 | 盾牌 | 64951 | 诺甘农D05 绿 | `shield_1h_artifactnorgannon_d_05.mdx` | `shield_1h_artifactnorgannon_d_05green` | ✔ 正常 |
| 420540 | 盾牌 | 64952 | 诺甘农D05 紫 | `shield_1h_artifactnorgannon_d_05.mdx` | `shield_1h_artifactnorgannon_d_05purple` | ✔ 正常 |
| 420541 | 盾牌 | 64953 | 诺甘农D05 红 | `shield_1h_artifactnorgannon_d_05.mdx` | `shield_1h_artifactnorgannon_d_05red` | ✔ 正常 |
| 420542 | 盾牌 | 64954 | 诺甘农D05 黄 | `shield_1h_artifactnorgannon_d_05.mdx` | `shield_1h_artifactnorgannon_d_05yellow` | ✔ 正常 |
| 420543 | 盾牌 | 64955 | 诺甘农D06 蓝 | `shield_1h_artifactnorgannon_d_06.mdx` | `shield_1h_artifactnorgannon_d_06_blue` | ✔ 正常 |
| 420544 | 盾牌 | 64956 | 诺甘农D06 绿 | `shield_1h_artifactnorgannon_d_06.mdx` | `shield_1h_artifactnorgannon_d_06_green` | ✔ 正常 |
| 420545 | 盾牌 | 64957 | 诺甘农D06 红 | `shield_1h_artifactnorgannon_d_06.mdx` | `shield_1h_artifactnorgannon_d_06_red` | ✔ 正常 |
| 420546 | 盾牌 | 47079 | 诺甘农D06 黄 | `shield_1h_artifactnorgannon_d_06.mdx` | `shield_1h_artifactnorgannon_d_06_yellow` | ◑ 已使用 |
| 420575 | 盾牌 | 64958 | 圣光之锤D06 | `shield_1h_artifactsilverhand_d_06.mdx` | `shield_1h_artifactsilverhand_d_06` | ✔ 正常 |
| 420547 | 盾牌 | 64959 | 风暴之拳D01v1 蓝 | `shield_1h_artifactstormfist_d_01.mdx` | `shield_1h_artifactstormfist_d_01v1blue` | ✔ 正常 |
| 420548 | 盾牌 | 64960 | 风暴之拳D01v1 紫 | `shield_1h_artifactstormfist_d_01.mdx` | `shield_1h_artifactstormfist_d_01v1purple` | ✔ 正常 |
| 420549 | 盾牌 | 64961 | 风暴之拳D01v1 白 | `shield_1h_artifactstormfist_d_01.mdx` | `shield_1h_artifactstormfist_d_01v1white` | ✔ 正常 |
| 420550 | 盾牌 | 64962 | 风暴之拳D01v1 黄 | `shield_1h_artifactstormfist_d_01.mdx` | `shield_1h_artifactstormfist_d_01v1yellow` | ✔ 正常 |
| 420551 | 盾牌 | 64963 | 风暴之拳D01 蓝 | `shield_1h_artifactstormfist_d_01.mdx` | `shield_1h_artifactstormfist_d_01_blue` | ✔ 正常 |
| 420552 | 盾牌 | 64964 | 风暴之拳D01 紫 | `shield_1h_artifactstormfist_d_01.mdx` | `shield_1h_artifactstormfist_d_01_purple` | ✔ 正常 |
| 420553 | 盾牌 | 64965 | 风暴之拳D01 白 | `shield_1h_artifactstormfist_d_01.mdx` | `shield_1h_artifactstormfist_d_01_white` | ✔ 正常 |
| 420554 | 盾牌 | 64966 | 风暴之拳D01 黄 | `shield_1h_artifactstormfist_d_01.mdx` | `shield_1h_artifactstormfist_d_01_yellow` | ✔ 正常 |
| 420555 | 盾牌 | 64967 | 风暴之拳D02 蓝 | `shield_1h_artifactstormfist_d_02.mdx` | `shield_1h_artifactstormfist_d_02_blue` | ✔ 正常 |
| 420556 | 盾牌 | 64968 | 风暴之拳D02 绿 | `shield_1h_artifactstormfist_d_02.mdx` | `shield_1h_artifactstormfist_d_02_green` | ✔ 正常 |
| 420557 | 盾牌 | 64969 | 风暴之拳D02 白 | `shield_1h_artifactstormfist_d_02.mdx` | `shield_1h_artifactstormfist_d_02_white` | ✔ 正常 |
| 420558 | 盾牌 | 64970 | 风暴之拳D02 黄 | `shield_1h_artifactstormfist_d_02.mdx` | `shield_1h_artifactstormfist_d_02_yellow` | ✔ 正常 |
| 420559 | 盾牌 | 64971 | 风暴之拳D03 蓝 | `shield_1h_artifactstormfist_d_03.mdx` | `shield_1h_artifactstormfist_d_03_blue` | ✔ 正常 |
| 420560 | 盾牌 | 64972 | 风暴之拳D03 绿 | `shield_1h_artifactstormfist_d_03.mdx` | `shield_1h_artifactstormfist_d_03_green` | ✔ 正常 |
| 420561 | 盾牌 | 64973 | 风暴之拳D03 橙 | `shield_1h_artifactstormfist_d_03.mdx` | `shield_1h_artifactstormfist_d_03_orange` | ✔ 正常 |
| 420562 | 盾牌 | 64974 | 风暴之拳D03 紫 | `shield_1h_artifactstormfist_d_03.mdx` | `shield_1h_artifactstormfist_d_03_purple` | ✔ 正常 |
| 420563 | 盾牌 | 64975 | 风暴之拳D04 蓝 | `shield_1h_artifactstormfist_d_04.mdx` | `shield_1h_artifactstormfist_d_04_blue` | ✔ 正常 |
| 420564 | 盾牌 | 64976 | 风暴之拳D04 绿 | `shield_1h_artifactstormfist_d_04.mdx` | `shield_1h_artifactstormfist_d_04_green` | ✔ 正常 |
| 420565 | 盾牌 | 64977 | 风暴之拳D04 红 | `shield_1h_artifactstormfist_d_04.mdx` | `shield_1h_artifactstormfist_d_04_red` | ✔ 正常 |
| 420566 | 盾牌 | 64978 | 风暴之拳D04 黄 | `shield_1h_artifactstormfist_d_04.mdx` | `shield_1h_artifactstormfist_d_04_yellow` | ✔ 正常 |
| 420567 | 盾牌 | 64979 | 风暴之拳D05 蓝 | `shield_1h_artifactstormfist_d_05.mdx` | `shield_1h_artifactstormfist_d_05_blue` | ✔ 正常 |
| 420568 | 盾牌 | 64980 | 风暴之拳D05 绿 | `shield_1h_artifactstormfist_d_05.mdx` | `shield_1h_artifactstormfist_d_05_green` | ✔ 正常 |
| 420569 | 盾牌 | 64981 | 风暴之拳D05 紫 | `shield_1h_artifactstormfist_d_05.mdx` | `shield_1h_artifactstormfist_d_05_purple` | ✔ 正常 |
| 420570 | 盾牌 | 64982 | 风暴之拳D05 红 | `shield_1h_artifactstormfist_d_05.mdx` | `shield_1h_artifactstormfist_d_05_red` | ✔ 正常 |
| 420571 | 盾牌 | 64983 | 风暴之拳D06 蓝 | `shield_1h_artifactstormfist_d_06.mdx` | `shield_1h_artifactstormfist_d_06_blue` | ✔ 正常 |
| 420572 | 盾牌 | 64984 | 风暴之拳D06 绿 | `shield_1h_artifactstormfist_d_06.mdx` | `shield_1h_artifactstormfist_d_06_green` | ✔ 正常 |
| 420573 | 盾牌 | 64985 | 风暴之拳D06 橙 | `shield_1h_artifactstormfist_d_06.mdx` | `shield_1h_artifactstormfist_d_06_orange` | ✔ 正常 |
| 420574 | 盾牌 | 64986 | 风暴之拳D06 青 | `shield_1h_artifactstormfist_d_06.mdx` | `shield_1h_artifactstormfist_d_06_teal` | ✔ 正常 |
| 420576 | 盾牌 | 64987 | 德拉诺挑战D01 橙 | `shield_draenorchallenge_d_01.mdx` | `shield_draenorchallenge_d_01_orange` | ✔ 正常 |

## 十五、副手物品（77 条：已使用 1 / 新生成 76）

> `class=4, subclass=0, InventoryType=23, sheath=0, Material=1`

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 420369 | 副手物品 | 64988 | 克苏恩D01 蓝 | `offhand_1h_artifactcthun_d_01.mdx` | `offhand_1h_artifactcthun_d_01_blue` | ✔ 正常 |
| 420370 | 副手物品 | 64989 | 克苏恩D01 紫 | `offhand_1h_artifactcthun_d_01.mdx` | `offhand_1h_artifactcthun_d_01_purple` | ✔ 正常 |
| 420371 | 副手物品 | 64990 | 克苏恩D01 青 | `offhand_1h_artifactcthun_d_01.mdx` | `offhand_1h_artifactcthun_d_01_teal` | ✔ 正常 |
| 420372 | 副手物品 | 64991 | 克苏恩D01 白 | `offhand_1h_artifactcthun_d_01.mdx` | `offhand_1h_artifactcthun_d_01_white` | ✔ 正常 |
| 420373 | 副手物品 | 64992 | 克苏恩D02 黑 | `offhand_1h_artifactcthun_d_02.mdx` | `offhand_1h_artifactcthun_d_02_black` | ✔ 正常 |
| 420374 | 副手物品 | 64993 | 克苏恩D02 绿 | `offhand_1h_artifactcthun_d_02.mdx` | `offhand_1h_artifactcthun_d_02_green` | ✔ 正常 |
| 420375 | 副手物品 | 64994 | 克苏恩D02 紫 | `offhand_1h_artifactcthun_d_02.mdx` | `offhand_1h_artifactcthun_d_02_purple` | ✔ 正常 |
| 420376 | 副手物品 | 64995 | 克苏恩D02 青 | `offhand_1h_artifactcthun_d_02.mdx` | `offhand_1h_artifactcthun_d_02_teal` | ✔ 正常 |
| 420377 | 副手物品 | 64996 | 克苏恩D03 黑 | `offhand_1h_artifactcthun_d_03.mdx` | `offhand_1h_artifactcthun_d_03_black` | ✔ 正常 |
| 420378 | 副手物品 | 64997 | 克苏恩D03 绿 | `offhand_1h_artifactcthun_d_03.mdx` | `offhand_1h_artifactcthun_d_03_green` | ✔ 正常 |
| 420379 | 副手物品 | 64998 | 克苏恩D03 紫 | `offhand_1h_artifactcthun_d_03.mdx` | `offhand_1h_artifactcthun_d_03_purple` | ✔ 正常 |
| 420380 | 副手物品 | 64999 | 克苏恩D03 红 | `offhand_1h_artifactcthun_d_03.mdx` | `offhand_1h_artifactcthun_d_03_red` | ✔ 正常 |
| 420381 | 副手物品 | 65000 | 克苏恩D04 绿 | `offhand_1h_artifactcthun_d_04.mdx` | `offhand_1h_artifactcthun_d_04_green` | ✔ 正常 |
| 420382 | 副手物品 | 65001 | 克苏恩D04 紫 | `offhand_1h_artifactcthun_d_04.mdx` | `offhand_1h_artifactcthun_d_04_purple` | ✔ 正常 |
| 420383 | 副手物品 | 65002 | 克苏恩D04 青 | `offhand_1h_artifactcthun_d_04.mdx` | `offhand_1h_artifactcthun_d_04_teal` | ✔ 正常 |
| 420384 | 副手物品 | 65003 | 克苏恩D04 白 | `offhand_1h_artifactcthun_d_04.mdx` | `offhand_1h_artifactcthun_d_04_white` | ✔ 正常 |
| 420385 | 副手物品 | 65004 | 克苏恩D05 蓝 | `offhand_1h_artifactcthun_d_05.mdx` | `offhand_1h_artifactcthun_d_05_blue` | ✔ 正常 |
| 420386 | 副手物品 | 65005 | 克苏恩D05 玉 | `offhand_1h_artifactcthun_d_05.mdx` | `offhand_1h_artifactcthun_d_05_jade` | ✔ 正常 |
| 420387 | 副手物品 | 65006 | 克苏恩D05 亮黄 | `offhand_1h_artifactcthun_d_05.mdx` | `offhand_1h_artifactcthun_d_05_lightyellow` | ✔ 正常 |
| 420388 | 副手物品 | 65007 | 克苏恩D05 紫 | `offhand_1h_artifactcthun_d_05.mdx` | `offhand_1h_artifactcthun_d_05_purple` | ✔ 正常 |
| 420389 | 副手物品 | 65008 | 克苏恩D06 绿 | `offhand_1h_artifactcthun_d_06.mdx` | `offhand_1h_artifactcthun_d_06_green` | ✔ 正常 |
| 420390 | 副手物品 | 65009 | 克苏恩D06 橙 | `offhand_1h_artifactcthun_d_06.mdx` | `offhand_1h_artifactcthun_d_06_orange` | ✔ 正常 |
| 420391 | 副手物品 | 65010 | 克苏恩D06 紫 | `offhand_1h_artifactcthun_d_06.mdx` | `offhand_1h_artifactcthun_d_06_purple` | ✔ 正常 |
| 420392 | 副手物品 | 65011 | 克苏恩D06 红 | `offhand_1h_artifactcthun_d_06.mdx` | `offhand_1h_artifactcthun_d_06_red` | ✔ 正常 |
| 420393 | 副手物品 | 65012 | 毁灭之锤D01 橙 | `offhand_1h_artifactdoomhammer_d_01.mdx` | `offhand_1h_artifactdoomhammer_d_01_orange` | ✔ 正常 |
| 420394 | 副手物品 | 65013 | 菲洛梅隆D01 蓝 | `offhand_1h_artifactfelomelorn_d_01.mdx` | `offhand_1h_artifactfelomelorn_d_01_blue` | ✔ 正常 |
| 420395 | 副手物品 | 65014 | 菲洛梅隆D01 绿 | `offhand_1h_artifactfelomelorn_d_01.mdx` | `offhand_1h_artifactfelomelorn_d_01_green` | ✔ 正常 |
| 420396 | 副手物品 | 65015 | 菲洛梅隆D01 紫 | `offhand_1h_artifactfelomelorn_d_01.mdx` | `offhand_1h_artifactfelomelorn_d_01_purple` | ✔ 正常 |
| 420397 | 副手物品 | 65016 | 菲洛梅隆D01 红 | `offhand_1h_artifactfelomelorn_d_01.mdx` | `offhand_1h_artifactfelomelorn_d_01_red` | ✔ 正常 |
| 420398 | 副手物品 | 65017 | 菲洛梅隆D02 蓝 | `offhand_1h_artifactfelomelorn_d_02.mdx` | `offhand_1h_artifactfelomelorn_d_02_blue` | ✔ 正常 |
| 420399 | 副手物品 | 65018 | 菲洛梅隆D02 绿 | `offhand_1h_artifactfelomelorn_d_02.mdx` | `offhand_1h_artifactfelomelorn_d_02_green` | ✔ 正常 |
| 420400 | 副手物品 | 65019 | 菲洛梅隆D02 紫 | `offhand_1h_artifactfelomelorn_d_02.mdx` | `offhand_1h_artifactfelomelorn_d_02_purple` | ✔ 正常 |
| 420401 | 副手物品 | 65020 | 菲洛梅隆D02 红 | `offhand_1h_artifactfelomelorn_d_02.mdx` | `offhand_1h_artifactfelomelorn_d_02_red` | ✔ 正常 |
| 420402 | 副手物品 | 65021 | 菲洛梅隆D03 蓝 | `offhand_1h_artifactfelomelorn_d_03.mdx` | `offhand_1h_artifactfelomelorn_d_03_blue` | ✔ 正常 |
| 420403 | 副手物品 | 65022 | 菲洛梅隆D03 绿 | `offhand_1h_artifactfelomelorn_d_03.mdx` | `offhand_1h_artifactfelomelorn_d_03_green` | ✔ 正常 |
| 420404 | 副手物品 | 65023 | 菲洛梅隆D03 紫 | `offhand_1h_artifactfelomelorn_d_03.mdx` | `offhand_1h_artifactfelomelorn_d_03_purple` | ✔ 正常 |
| 420405 | 副手物品 | 65024 | 菲洛梅隆D03 红 | `offhand_1h_artifactfelomelorn_d_03.mdx` | `offhand_1h_artifactfelomelorn_d_03_red` | ✔ 正常 |
| 420406 | 副手物品 | 65025 | 菲洛梅隆D04 蓝 | `offhand_1h_artifactfelomelorn_d_04.mdx` | `offhand_1h_artifactfelomelorn_d_04_blue` | ✔ 正常 |
| 420407 | 副手物品 | 65026 | 菲洛梅隆D04 绿 | `offhand_1h_artifactfelomelorn_d_04.mdx` | `offhand_1h_artifactfelomelorn_d_04_green` | ✔ 正常 |
| 420408 | 副手物品 | 65027 | 菲洛梅隆D04 紫 | `offhand_1h_artifactfelomelorn_d_04.mdx` | `offhand_1h_artifactfelomelorn_d_04_purple` | ✔ 正常 |
| 420409 | 副手物品 | 65028 | 菲洛梅隆D04 红 | `offhand_1h_artifactfelomelorn_d_04.mdx` | `offhand_1h_artifactfelomelorn_d_04_red` | ✔ 正常 |
| 420410 | 副手物品 | 65029 | 菲洛梅隆D05 蓝 | `offhand_1h_artifactfelomelorn_d_05.mdx` | `offhand_1h_artifactfelomelorn_d_05_blue` | ✔ 正常 |
| 420411 | 副手物品 | 65030 | 菲洛梅隆D05 绿 | `offhand_1h_artifactfelomelorn_d_05.mdx` | `offhand_1h_artifactfelomelorn_d_05_green` | ✔ 正常 |
| 420412 | 副手物品 | 65031 | 菲洛梅隆D05 紫 | `offhand_1h_artifactfelomelorn_d_05.mdx` | `offhand_1h_artifactfelomelorn_d_05_purple` | ✔ 正常 |
| 420413 | 副手物品 | 65032 | 菲洛梅隆D05 红 | `offhand_1h_artifactfelomelorn_d_05.mdx` | `offhand_1h_artifactfelomelorn_d_05_red` | ✔ 正常 |
| 420414 | 副手物品 | 65033 | 菲洛梅隆D06 暗 | `offhand_1h_artifactfelomelorn_d_06.mdx` | `offhand_1h_artifactfelomelorn_d_06_dark` | ✔ 正常 |
| 420415 | 副手物品 | 65034 | 菲洛梅隆D06 光 | `offhand_1h_artifactfelomelorn_d_06.mdx` | `offhand_1h_artifactfelomelorn_d_06_light` | ✔ 正常 |
| 420416 | 副手物品 | 65035 | 菲洛梅隆D06 红 | `offhand_1h_artifactfelomelorn_d_06.mdx` | `offhand_1h_artifactfelomelorn_d_06_red` | ✔ 正常 |
| 420417 | 副手物品 | 65036 | 菲洛梅隆D06 黄 | `offhand_1h_artifactfelomelorn_d_06.mdx` | `offhand_1h_artifactfelomelorn_d_06_yellow` | ✔ 正常 |
| 420418 | 副手物品 | 65037 | 圣光之锤D01 蓝 | `offhand_1h_artifactsilverhand_d_01.mdx` | `offhand_1h_artifactsilverhand_d_01blue` | ✔ 正常 |
| 420419 | 副手物品 | 65038 | 圣光之锤D01 紫 | `offhand_1h_artifactsilverhand_d_01.mdx` | `offhand_1h_artifactsilverhand_d_01purple` | ✔ 正常 |
| 420420 | 副手物品 | 65039 | 圣光之锤D01 红 | `offhand_1h_artifactsilverhand_d_01.mdx` | `offhand_1h_artifactsilverhand_d_01red` | ✔ 正常 |
| 420421 | 副手物品 | 65040 | 圣光之锤D01 黄 | `offhand_1h_artifactsilverhand_d_01.mdx` | `offhand_1h_artifactsilverhand_d_01yellow` | ✔ 正常 |
| 420422 | 副手物品 | 50173 | 艾瑞达之颅D01 绿 | `offhand_1h_artifactskulloferedar_d_01.mdx` | `offhand_1h_artifactskulloferedar_d_01_green` | ◑ 已使用 |
| 420423 | 副手物品 | 65041 | 艾瑞达之颅D01 紫 | `offhand_1h_artifactskulloferedar_d_01.mdx` | `offhand_1h_artifactskulloferedar_d_01_purple` | ✔ 正常 |
| 420424 | 副手物品 | 65042 | 艾瑞达之颅D01 红 | `offhand_1h_artifactskulloferedar_d_01.mdx` | `offhand_1h_artifactskulloferedar_d_01_red` | ✔ 正常 |
| 420425 | 副手物品 | 65043 | 艾瑞达之颅D01 黄 | `offhand_1h_artifactskulloferedar_d_01.mdx` | `offhand_1h_artifactskulloferedar_d_01_yellow` | ✔ 正常 |
| 420426 | 副手物品 | 65044 | 艾瑞达之颅D02 绿 | `offhand_1h_artifactskulloferedar_d_02.mdx` | `offhand_1h_artifactskulloferedar_d_02_green` | ✔ 正常 |
| 420427 | 副手物品 | 65045 | 艾瑞达之颅D02 紫 | `offhand_1h_artifactskulloferedar_d_02.mdx` | `offhand_1h_artifactskulloferedar_d_02_purple` | ✔ 正常 |
| 420428 | 副手物品 | 65046 | 艾瑞达之颅D02 红 | `offhand_1h_artifactskulloferedar_d_02.mdx` | `offhand_1h_artifactskulloferedar_d_02_red` | ✔ 正常 |
| 420429 | 副手物品 | 65047 | 艾瑞达之颅D02 黄 | `offhand_1h_artifactskulloferedar_d_02.mdx` | `offhand_1h_artifactskulloferedar_d_02_yellow` | ✔ 正常 |
| 420430 | 副手物品 | 65048 | 艾瑞达之颅D03 绿 | `offhand_1h_artifactskulloferedar_d_03.mdx` | `offhand_1h_artifactskulloferedar_d_03_green` | ✔ 正常 |
| 420431 | 副手物品 | 65049 | 艾瑞达之颅D03 紫 | `offhand_1h_artifactskulloferedar_d_03.mdx` | `offhand_1h_artifactskulloferedar_d_03_purple` | ✔ 正常 |
| 420432 | 副手物品 | 65050 | 艾瑞达之颅D03 红 | `offhand_1h_artifactskulloferedar_d_03.mdx` | `offhand_1h_artifactskulloferedar_d_03_red` | ✔ 正常 |
| 420433 | 副手物品 | 65051 | 艾瑞达之颅D03 黄 | `offhand_1h_artifactskulloferedar_d_03.mdx` | `offhand_1h_artifactskulloferedar_d_03_yellow` | ✔ 正常 |
| 420434 | 副手物品 | 65052 | 艾瑞达之颅D04 蓝 | `offhand_1h_artifactskulloferedar_d_04.mdx` | `offhand_1h_artifactskulloferedar_d_04_blue` | ✔ 正常 |
| 420435 | 副手物品 | 65053 | 艾瑞达之颅D04 绿 | `offhand_1h_artifactskulloferedar_d_04.mdx` | `offhand_1h_artifactskulloferedar_d_04_green` | ✔ 正常 |
| 420436 | 副手物品 | 65054 | 艾瑞达之颅D04 橙 | `offhand_1h_artifactskulloferedar_d_04.mdx` | `offhand_1h_artifactskulloferedar_d_04_orange` | ✔ 正常 |
| 420437 | 副手物品 | 65055 | 艾瑞达之颅D04 紫 | `offhand_1h_artifactskulloferedar_d_04.mdx` | `offhand_1h_artifactskulloferedar_d_04_purple` | ✔ 正常 |
| 420438 | 副手物品 | 65056 | 艾瑞达之颅D05 蓝 | `offhand_1h_artifactskulloferedar_d_05.mdx` | `offhand_1h_artifactskulloferedar_d_05_blue` | ✔ 正常 |
| 420439 | 副手物品 | 65057 | 艾瑞达之颅D05 紫 | `offhand_1h_artifactskulloferedar_d_05.mdx` | `offhand_1h_artifactskulloferedar_d_05_purple` | ✔ 正常 |
| 420440 | 副手物品 | 65058 | 艾瑞达之颅D05 红 | `offhand_1h_artifactskulloferedar_d_05.mdx` | `offhand_1h_artifactskulloferedar_d_05_red` | ✔ 正常 |
| 420441 | 副手物品 | 65059 | 艾瑞达之颅D05 黄 | `offhand_1h_artifactskulloferedar_d_05.mdx` | `offhand_1h_artifactskulloferedar_d_05_yellow` | ✔ 正常 |
| 420442 | 副手物品 | 65060 | 艾瑞达之颅D06 | `offhand_1h_artifactskulloferedar_d_06.mdx` | `offhand_1h_artifactskulloferedar_d_06` | ✔ 正常 |
| 420443 | 副手物品 | 65061 | 艾瑞达之颅D06 苍白 | `offhand_1h_artifactskulloferedar_d_06.mdx` | `offhand_1h_artifactskulloferedar_d_06pale` | ✔ 正常 |
| 420444 | 副手物品 | 65062 | 艾瑞达之颅D06 紫 | `offhand_1h_artifactskulloferedar_d_06.mdx` | `offhand_1h_artifactskulloferedar_d_06purple` | ✔ 正常 |
| 420445 | 副手物品 | 65063 | 艾瑞达之颅D06 红 | `offhand_1h_artifactskulloferedar_d_06.mdx` | `offhand_1h_artifactskulloferedar_d_06red` | ✔ 正常 |

## 十六、其它（非武器）（1 条：已使用 0 / 新生成 0）

> `class=4, subclass=0, InventoryType=3, sheath=0, Material=1`
>
> 护甲（肩甲）模型，只登记不生成物品。

| DisplayID | 武器类型 | 物品 entry | 名称 | 模型文件 | 贴图 | 状态 |
|---:|---|---:|---|---|---|---|
| 100000 | 其它（非武器） | — | 挑战圣骑士肩甲 蓝 | `lshoulder_plate_challengepaladin_d_01.mdx`<br>`rshoulder_plate_challengepaladin_d_01.mdx` | `plate_challengepaladin_d_01blue` | — 非武器 |

## 十七、已使用清单（50 条 DisplayID）

| DisplayID | 武器类型 | 名称 | 现有 entry | 物品名 | InventoryType | sheath |
|---:|---|---|---:|---|---:|---:|
| 100005 | 法杖 | 王昭君之法杖 | 92000 | 小魔女法杖 | 17 | 2 |
| 100006 | 双手剑 | 亚瑟·王者之剑 | 93001 | 王者之剑 | 17 | 1 |
| 100012 | 长柄武器 | 泰坦阿古斯D01 红 | 50178 | 血瀑 | 17 | 1 |
| 420014 | 双手斧 | 玛乌D03 红 | 47285 | 双刃屠斧 | 17 | 1 |
| 420015 | 双手斧 | 玛乌D03 青 | 47069 | 公正使者 | 17 | 1 |
| 420045 | 弓 | 风行者D04 紫 | 50034 | 佐德的机括长弓 | 15 | 0 |
| 420046 | 弓 | 风行者D04 红 | 50776 | 海洋之神骨弓 | 15 | 0 |
| 420106 | 战刃 | 阿兹加洛D02 绿 | 61040 | 主-阿兹加洛战刃·绿 | 13 | 1 |
|  |  |  | 99945 | 镇岳短剑 | 21 | 3 |
|  |  |  | 100522 | 诅咒之城之军刀 | 21 | 3 |
|  |  |  | 101115 | 圣贤军刀 | 21 | 3 |
|  |  |  | 101692 | 亘古之佩剑 | 21 | 3 |
|  |  |  | 102269 | 守誓之新月 | 21 | 3 |
|  |  |  | 103469 | 不屈守护军刀 | 21 | 3 |
| 420108 | 战刃 | 阿兹加洛D02 红 | 61042 | 主-阿兹加洛战刃·红 | 13 | 1 |
|  |  |  | 99947 | 月刃驭天军刀 | 21 | 3 |
|  |  |  | 100524 | 逐日者之短剑 | 21 | 3 |
|  |  |  | 101117 | 复苏·短剑 | 21 | 3 |
|  |  |  | 101694 | 星陨短剑 | 21 | 3 |
|  |  |  | 102271 | 北风苔原之利刃 | 21 | 3 |
|  |  |  | 103471 | 群星回响符文剑 | 21 | 3 |
| 420197 | 匕首 | 克苏恩D06 绿 | 50183 | 肺穿 | 13 | 3 |
| 420235 | 匕首 | 迦罗娜D02 红 | 49968 | 冰封骨针 | 21 | 3 |
| 420248 | 匕首 | 迦罗娜D05 黄 | 51803 | 泰尔萨斯，鲜血之王的匕首 | 21 | 3 |
| 420278 | 单手锤 | 艾萨拉D01 橙 | 50028 | 创伤 | 21 | 3 |
| 420291 | 单手锤 | 艾萨拉D04 黄 | 51798 | 瓦琉斯，圣光使者之锤 | 21 | 3 |
| 420327 | 单手锤 | 毁灭之锤D06 橙 | 110001 | 晨露圣辉·初 | 13 | 3 |
|  |  |  | 110011 | 晨露圣辉 | 13 | 3 |
| 420347 | 双手锤 | 圣光之锤D01 黄 | 51796 | 米奈希尔战槌 | 17 | 1 |
| 420352 | 双手锤 | 圣光之锤D03 蓝 | 49919 | 掘墓者 | 17 | 1 |
| 420422 | 副手物品 | 艾瑞达之颅D01 绿 | 50173 | 影丝纺锤 | 23 | 0 |
| 420458 | 长柄武器 | 雄鹰D04 蓝 | 50425 | 缚誓者，游侠将军的号令 | 17 | 1 |
| 420529 | 盾牌 | 诺甘农D02 白 | 49976 | 燃铁壁垒 | 14 | 4 |
| 420530 | 盾牌 | 诺甘农D02 黄 | 50794 | 无尽寒冬 | 14 | 4 |
| 420534 | 盾牌 | 诺甘农D03 黄 | 110003 | 永恒壁垒·初 | 14 | 4 |
|  |  |  | 110013 | 永恒壁垒 | 14 | 4 |
| 420546 | 盾牌 | 诺甘农D06 黄 | 47079 | 圣洁壁垒 | 14 | 4 |
| 420603 | 法杖 | 安东尼达斯之杖D01 紫 | 61036 | 安东尼达斯法杖·紫 | 17 | 2 |
|  |  |  | 99961 | 血月之秘杖 | 17 | 2 |
|  |  |  | 100538 | 血月冻结权杖 | 17 | 2 |
|  |  |  | 101131 | 血色修道院秘杖 | 17 | 2 |
|  |  |  | 101708 | 冲击手杖 | 17 | 2 |
|  |  |  | 102285 | 霜裔之慈悲 | 17 | 2 |
|  |  |  | 103485 | 符文大师的长杖 | 17 | 2 |
| 420604 | 法杖 | 安东尼达斯之杖D01 红 | 61037 | 安东尼达斯法杖·红 | 17 | 2 |
|  |  |  | 99962 | 镇渊神杖 | 17 | 2 |
|  |  |  | 100539 | 青铜·权杖 | 17 | 2 |
|  |  |  | 101132 | 辉光法杖 | 17 | 2 |
|  |  |  | 101709 | 圣光权杖 | 17 | 2 |
|  |  |  | 102286 | 屠龙者镇压节杖 | 17 | 2 |
|  |  |  | 103486 | 黑锋骑士的祭司杖 | 17 | 2 |
| 420610 | 法杖 | 安东尼达斯之杖D03 金 | 33465 | 大法师守护法杖 | 17 | 2 |
| 420619 | 法杖 | 安东尼达斯之杖D05 橙 | 61038 | 涌动法杖·橙 | 17 | 2 |
| 420620 | 法杖 | 安东尼达斯之杖D05 紫 | 61039 | 涌动法杖·紫 | 17 | 2 |
| 420623 | 法杖 | 安东尼达斯之杖D06 紫 | 110000 | 星穹织法者·初 | 17 | 2 |
|  |  |  | 110010 | 星穹织法者 | 17 | 2 |
| 420694 | 法杖 | 库雷之心D06 金 | 51799 | 海里昂，遗忘之爱 | 17 | 2 |
| 420735 | 法杖 | 诺达希尔D04 紫 | 49992 | 尼伯龙根 | 17 | 2 |
| 420840 | 单手剑 | 菲洛梅隆D06 光 | 50427 | 血涌，克尔苏加德的苦痛之剑 | 21 | 3 |
| 420882 | 单手剑 | 符文剑D02 青 | 50068 | 死僵长剑 | 21 | 3 |
| 420884 | 单手剑 | 符文剑D03 绿 | 51010 | 整容刀 | 13 | 3 |
| 420908 | 单手剑 | 苍穹D03 红 | 110004 | 月影猎杀者·初 | 13 | 3 |
|  |  |  | 110014 | 月影猎杀者 | 13 | 3 |
| 420913 | 单手剑 | 苍穹D04 白 | 50412 | 血毒长剑 | 13 | 3 |
| 420961 | 双手剑 | 阿拉索D05 黄 | 33478 | 狂斩之刃 | 17 | 1 |
| 420965 | 双手剑 | 灰烬使者D01 红 | 50070 | 格罗萨格，白银之手骑士团圣剑 | 17 | 1 |
| 420970 | 双手剑 | 灰烬使者·烈焰D03 蓝 | 61044 | 烈焰灰烬·蓝 | 17 | 1 |
| 420971 | 双手剑 | 灰烬使者·烈焰D03 绿 | 61045 | 烈焰灰烬·绿 | 17 | 1 |
| 420972 | 双手剑 | 灰烬使者·烈焰D03 红 | 61046 | 烈焰灰烬·红 | 17 | 1 |
| 420985 | 双手剑 | 灰烬使者·暗影D03 红 | 93002 | 暗影灰烬红 | 17 | 1 |
| 421003 | 双手剑 | 灵魂撕裂者D05 蓝 | 61033 | 亡者·蓝 | 17 | 1 |
|  |  |  | 102854 | 圣贤洞察断钢剑 | 17 | 1 |
|  |  |  | 104054 | 永恒之眼之双手剑 | 17 | 1 |
| 421004 | 双手剑 | 灵魂撕裂者D05 紫 | 61034 | 亡者·紫 | 17 | 1 |
|  |  |  | 102855 | 龙眠的流光 | 17 | 1 |
|  |  |  | 104055 | 永恒之眼赤焰符文巨刃 | 17 | 1 |
| 421005 | 双手剑 | 灵魂撕裂者D05 红 | 61035 | 亡者·红 | 17 | 1 |
|  |  |  | 102856 | 失落荣光巨剑 | 17 | 1 |
|  |  |  | 104056 | 泰坦巨剑 | 17 | 1 |
| 421007 | 双手剑 | 灵魂撕裂者D06 蓝 | 61030 | 天启·蓝 | 17 | 1 |
|  |  |  | 99954 | 潜行者之阔剑 | 17 | 1 |
|  |  |  | 100531 | 银色北伐军之巨剑 | 17 | 1 |
|  |  |  | 101124 | 梦境之大剑 | 17 | 1 |
|  |  |  | 101701 | 月刃驭雷阔剑 | 17 | 1 |
|  |  |  | 102278 | 斩剑·不灭烽火 | 17 | 1 |
|  |  |  | 103478 | 破军之永恒征途 | 17 | 1 |
| 421009 | 双手剑 | 灵魂撕裂者D06 红 | 61031 | 天启·红 | 17 | 1 |
|  |  |  | 99955 | 先驱者大剑 | 17 | 1 |
|  |  |  | 100532 | 祖达克之阔剑 | 17 | 1 |
|  |  |  | 101125 | 祖尔法拉克阔剑 | 17 | 1 |
|  |  |  | 101702 | 战吼大剑 | 17 | 1 |
|  |  |  | 102279 | 怒火之无声裁决 | 17 | 1 |
|  |  |  | 103479 | 守望者之雷霆 | 17 | 1 |
| 421010 | 双手剑 | 灵魂撕裂者D06 黄 | 61032 | 天启·黄 | 17 | 1 |
|  |  |  | 102853 | 黑锋骑士裁断斩剑 | 17 | 1 |
|  |  |  | 104053 | 赤焰凝视符文巨刃 | 17 | 1 |
| 421039 | 战刃 | 左·阿兹加洛D02_1 | 61041 | 副-阿兹加洛战刃·绿 | 13 | 1 |
|  |  |  | 99946 | 星界佩剑 | 21 | 3 |
|  |  |  | 100523 | 深渊利剑 | 21 | 3 |
|  |  |  | 101116 | 潜行者之佩剑 | 21 | 3 |
|  |  |  | 101693 | 星陨佩剑 | 21 | 3 |
|  |  |  | 102270 | 圣贤追猎阔剑 | 21 | 3 |
|  |  |  | 103470 | 黎明圣所的破晓 | 21 | 3 |
| 421042 | 战刃 | 左·阿兹加洛D02_4 | 61043 | 副-阿兹加洛战刃·红 | 13 | 1 |
|  |  |  | 99948 | 冰冠堡垒之长剑 | 21 | 3 |
|  |  |  | 100525 | 命运·军刀 | 21 | 3 |
|  |  |  | 101118 | 大领主军刀 | 21 | 3 |
|  |  |  | 101695 | 破天长剑 | 21 | 3 |
|  |  |  | 102272 | 灵辉之静潮 | 21 | 3 |
|  |  |  | 103472 | 无声裁决·军刀 | 21 | 3 |

## 十八、生成文件说明

### 1. `武器物品_64040_65063.sql`

- entry 从 **64040** 连续到 **65063**，覆盖「有效且未被现有物品引用」的 **1024** 条 DisplayID；已被引用的 50 条沿用原 entry。
- 公共字段：`Quality=4`、`bonding=1`、`FlagsExtra=8192`、`BuyPrice=250000000`、`SellPrice=12`、`ItemLevel=80`、`RequiredLevel=80`、`VerifiedBuild=15595`。
- 武器伤害 `555~777`、`delay=2900`；盾牌另给 `armor=5000`，盾牌与副手不写伤害。
- 开头 `DELETE FROM item_template WHERE entry BETWEEN 64040 AND 65063;`（该区间服务端原本为空），可重复执行。

### 2. `武器物品_64040_65063_Item.dbc.csv`

- 列：`ID,ClassID,SubclassID,Sound_Override_Subclassid,Material,DisplayInfoID,InventoryType,SheatheType`，与 SQL 逐行对应。
- 需同时把 `ItemDisplayInfo.dbc`（模型/贴图）导入到**加载顺序最高**的补丁，否则客户端不显示模型。
- 旧文件 `武器物品_64000_64129_Item.dbc.csv` 的 `64040~64129` 段为历史残留，导入前整段删除。

### 3. 各类别 DB/DBC 字段对照

| 武器类型 | class | subclass | InventoryType | SheatheType | Material |
|---|---:|---:|---:|---:|---:|
| 单手剑 | 2 | 7 | 13 | 3 | 1 |
| 双手剑 | 2 | 8 | 17 | 1 | 1 |
| 双手斧 | 2 | 1 | 17 | 1 | 1 |
| 单手锤 | 2 | 4 | 13 | 3 | 2 |
| 双手锤 | 2 | 5 | 17 | 1 | 2 |
| 长柄武器 | 2 | 6 | 17 | 1 | 1 |
| 法杖 | 2 | 10 | 17 | 2 | 2 |
| 匕首 | 2 | 15 | 13 | 3 | 1 |
| 拳套 | 2 | 13 | 13 | 3 | 1 |
| 战刃 | 2 | 7 | 13 | 3 | 1 |
| 弓 | 2 | 2 | 15 | 0 | 2 |
| 弩 | 2 | 18 | 26 | 0 | 2 |
| 枪械 | 2 | 3 | 26 | 0 | 1 |
| 盾牌 | 4 | 6 | 14 | 4 | 1 |
| 副手物品 | 4 | 0 | 23 | 0 | 1 |
| 其它（非武器） | 4 | 0 | 3 | 0 | 1 |
