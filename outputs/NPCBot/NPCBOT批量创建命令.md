# NPCBOT 批量创建命令（中文名）

共 100 条 `.npcbot createnew` 命令，5 个职业各 20 个。种族范围限定为：德莱尼(11)、血精灵(10)、人类(1)、兽人(2)、亡灵(5)、巨魔(8)、暗夜精灵(4)；性别随机；外观参数（皮肤/脸型/发型/发色/特征）均在该种族该性别的合法范围内随机；名称按种族与职业风格取中文名。

## 使用说明

1. 代码侧已就绪：`HandleNpcBotCreateNewCommand` 已不再校验 `playercreateinfo` 种族-职业组合（`botcommands.cpp:3874` 注释「不限制种族-职业组合」），仅保留 ARCHMAGE 固定人类模型这一条限制；血精灵萨满/牧师、德莱尼盗贼/术士、暗夜精灵法师、亡灵萨满都能创建。若 worldserver 是旧构建，需先重新编译重启。
2. 在游戏内聊天框逐条粘贴（保留开头的 `.`）；也可粘贴到 worldserver 控制台，此时**去掉开头的 `.`**（控制台带不带 `.` 都能识别）。中文名在两条路径下都正常：控制台输入按系统本地编码转 UTF-8，游戏内输入本身就是 UTF-8，代码已分别处理（需包含该修复的构建）。
3. 新条目从 70800 起按创建顺序自动顺延（`BOT_ENTRY_CREATE_BEGIN`，取 70800 起第一个空位）。当前库 70800-71349 已占满 550 条，因此本批正好从 71350 开始、到 71449 结束，创建完成后统一重启一次 worldserver 生效。
4. 重启后用 `.npcbot lookup <职业ID> 1` 查出未生成的条目，再 `.npcbot spawn <entry>` 生成到当前所在位置。
5. 命令参数顺序：`名称 职业 种族 性别 皮肤 脸型 发型 发色 特征 音色(1-3)`。名称不能带空格。

## 盗贼（职业 4）
71350-71369
```
.npcbot createnew 范森 4 1 0 3 5 8 2 1 1
.npcbot createnew 塞布尔 4 1 1 6 10 14 4 2 2
.npcbot createnew 格罗玛 4 2 0 5 6 7 3 4 1
.npcbot createnew 薇莉丝拉 4 10 1 4 7 12 5 3 3
.npcbot createnew 妮瑟拉 4 4 1 3 2 9 2 6 1
.npcbot createnew 莫德雷克 4 5 0 2 5 9 3 7 2
.npcbot createnew 泽卡尔 4 8 0 3 2 7 6 5 1
.npcbot createnew 阿诺拉 4 11 1 5 4 10 3 2 2
.npcbot createnew 米拉 4 1 1 8 12 18 6 4 3
.npcbot createnew 乌尔莎 4 2 1 6 5 9 4 3 1
.npcbot createnew 摩根斯 4 5 1 4 7 11 5 6 2
.npcbot createnew 萨洛里恩 4 10 0 6 5 10 7 4 2
.npcbot createnew 莫克拉 4 8 1 4 3 6 5 3 3
.npcbot createnew 伊瑟兰 4 4 0 5 6 8 4 3 2
.npcbot createnew 塔雷诺斯 4 11 0 7 6 9 2 5 1
.npcbot createnew 科尔文 4 1 0 6 9 12 5 6 2
.npcbot createnew 艾莉瑞亚 4 10 1 8 8 16 8 9 1
.npcbot createnew 达克拉 4 2 0 7 3 4 2 9 3
.npcbot createnew 格雷夫 4 5 0 3 8 13 8 12 3
.npcbot createnew 扎莉拉 4 8 1 2 5 8 8 4 2
```

## 牧师（职业 5）
71370-71389
```
.npcbot createnew 艾琳 5 1 1 2 4 10 2 1 1
.npcbot createnew 贝内迪克 5 1 0 4 7 11 3 5 2
.npcbot createnew 内穆拉 5 11 1 9 8 13 5 4 1
.npcbot createnew 伊里丹 5 11 0 11 7 11 4 6 3
.npcbot createnew 艾露娜拉 5 4 1 6 7 10 6 8 2
.npcbot createnew 费洛尔 5 4 0 4 5 10 3 4 1
.npcbot createnew 娜瑟拉 5 5 1 5 6 12 7 5 2
.npcbot createnew 塞普尔 5 5 0 4 9 14 9 15 3
.npcbot createnew 奎拉拉 5 10 1 3 6 14 4 5 1
.npcbot createnew 维利斯 5 10 0 5 6 13 2 7 2
.npcbot createnew 森加拉 5 8 1 3 4 7 7 2 3
.npcbot createnew 拉斯塔里 5 8 0 4 3 8 8 7 1
.npcbot createnew 斯露娜 5 2 1 7 7 11 6 5 2
.npcbot createnew 杜尔纳克 5 2 0 6 4 9 5 8 1
.npcbot createnew 玛丽安 5 1 1 7 13 21 7 5 3
.npcbot createnew 托比亚斯 5 1 0 9 11 15 8 7 1
.npcbot createnew 伊莉亚娜 5 4 1 7 3 8 5 3 2
.npcbot createnew 维斯嘉 5 11 1 12 6 14 2 5 3
.npcbot createnew 塞莉薇恩 5 10 1 9 9 17 9 10 2
.npcbot createnew 格里姆沃 5 5 0 5 7 10 4 11 1
```

## 法师（职业 8）
71390-71409
```
.npcbot createnew 奥德里克 8 1 0 5 8 13 4 3 1
.npcbot createnew 埃琳娜 8 1 1 4 9 16 3 2 2
.npcbot createnew 瑟隆纳斯 8 10 0 7 8 14 6 8 3
.npcbot createnew 奎莉丝拉 8 10 1 6 5 11 3 7 1
.npcbot createnew 奈莉拉 8 4 1 5 6 11 4 7 2
.npcbot createnew 埃洛瑞 8 4 0 7 7 9 6 5 1
.npcbot createnew 莫提斯 8 5 0 2 6 11 5 9 3
.npcbot createnew 莉兰达 8 5 1 3 8 13 6 4 2
.npcbot createnew 沃尔特拉 8 8 0 5 4 9 7 9 1
.npcbot createnew 祖尔玛拉 8 8 1 5 2 9 9 4 2
.npcbot createnew 阿诺里姆 8 11 0 9 9 12 5 7 3
.npcbot createnew 凯莉拉 8 11 1 10 7 12 4 6 1
.npcbot createnew 塞德里克 8 1 0 7 10 14 6 8 2
.npcbot createnew 娜格拉 8 2 1 5 6 10 5 4 3
.npcbot createnew 沃加斯 8 2 0 4 7 10 3 7 2
.npcbot createnew 罗莎琳 8 1 1 8 14 23 8 6 1
.npcbot createnew 贝拉松 8 10 0 9 4 15 9 9 2
.npcbot createnew 德希拉 8 5 1 4 5 9 4 7 3
.npcbot createnew 雷扎拉 8 8 1 4 5 8 6 5 1
.npcbot createnew 伊莉拉 8 11 1 7 9 15 6 5 2
```

## 术士（职业 9）
71410-71429
```
.npcbot createnew 戈德里克 9 1 0 8 6 10 7 4 1
.npcbot createnew 伊索尔德 9 1 1 9 11 20 9 5 2
.npcbot createnew 马尔里克 9 5 0 4 4 12 6 13 3
.npcbot createnew 薇兰 9 5 1 2 9 14 8 6 1
.npcbot createnew 纳兹格罗 9 2 0 8 8 11 7 10 2
.npcbot createnew 格鲁姆卡 9 2 1 8 8 12 7 6 3
.npcbot createnew 维莱萨尔 9 10 0 3 7 12 8 6 1
.npcbot createnew 伊丝瑞尔 9 10 1 7 4 13 7 8 2
.npcbot createnew 扎尔金 9 8 0 2 1 5 5 8 3
.npcbot createnew 奈克瑟拉 9 8 1 5 4 9 8 5 1
.npcbot createnew 沃拉斯 9 11 0 12 5 13 6 4 2
.npcbot createnew 阿莉丝 9 11 1 4 9 15 6 3 3
.npcbot createnew 莫薇恩 9 4 1 8 8 11 7 9 1
.npcbot createnew 达内尔 9 4 0 6 8 11 5 5 2
.npcbot createnew 卢瑟 9 1 0 2 9 16 9 6 3
.npcbot createnew 乌兹卡 9 2 1 4 3 7 2 5 1
.npcbot createnew 克拉文 9 5 0 5 9 14 7 16 2
.npcbot createnew 席尔薇娜 9 10 1 8 9 18 9 9 3
.npcbot createnew 库扎克 9 8 0 4 4 9 9 10 1
.npcbot createnew 梅丽珊 9 1 1 6 12 19 5 4 2
```

## 萨满（职业 7）
71430-71449
```
.npcbot createnew 戈拉什 7 2 0 3 5 8 4 6 1
.npcbot createnew 乌尔格拉 7 2 1 6 4 11 3 4 2
.npcbot createnew 祖尔玛 7 8 0 5 3 6 3 6 3
.npcbot createnew 塞莎特拉 7 8 1 3 5 9 4 3 1
.npcbot createnew 沃瑞克 7 11 0 6 8 10 3 5 2
.npcbot createnew 妮莫拉 7 11 1 8 5 11 5 4 3
.npcbot createnew 莫尔丹 7 5 0 3 3 8 2 8 1
.npcbot createnew 薇拉娜 7 4 1 4 4 10 3 5 2
.npcbot createnew 奥林 7 1 0 6 5 9 2 3 3
.npcbot createnew 艾尔瑞克 7 10 0 4 9 11 5 5 1
.npcbot createnew 萨琳德拉 7 10 1 5 8 15 6 6 2
.npcbot createnew 科马克 7 2 0 7 7 10 6 9 3
.npcbot createnew 泽卡娜 7 8 1 5 3 7 7 4 1
.npcbot createnew 塔罗什 7 11 0 10 4 12 6 6 2
.npcbot createnew 奈索尔 7 4 0 8 4 11 7 5 3
.npcbot createnew 拉瑟拉 7 5 1 5 7 13 6 5 1
.npcbot createnew 布琳 7 1 1 5 7 15 4 3 2
.npcbot createnew 鲁卡尔 7 8 0 4 2 8 8 7 3
.npcbot createnew 德拉克卡 7 2 1 7 8 12 5 6 1
.npcbot createnew 艾拉 7 11 1 11 8 13 4 5 2
```

## 附：外观参数上限对照表

格式：`皮肤 脸型 发型 发色 特征`

| 种族 | 男性上限 | 女性上限 |
| --- | --- | --- |
| 人类 | 9 11 16 9 8 | 9 14 23 9 6 |
| 兽人 | 8 8 11 7 10 | 8 8 12 7 6 |
| 暗夜精灵 | 8 8 11 7 5 | 8 8 11 7 9 |
| 亡灵 | 5 9 14 9 16 | 5 9 14 9 7 |
| 巨魔 | 5 4 9 9 10 | 5 5 9 9 5 |
| 血精灵 | 9 9 15 9 9 | 9 9 18 9 10 |
| 德莱尼 | 13 9 13 6 7 | 13 9 15 6 6 |

超出上限会返回“race '种族名' 的参数无效”，需要按此表调整。

## 注意事项（已核对）

- 本批 100 条已按代码里的 `MAX_PLAYER_VISUALS`、`SOUND_SETS_COUNT = 3`、可玩种族掩码与职业掩码逐条校验：0 处越界，0 处重名，5 个职业各 20 条。
- 非法种族-职业组合（德莱尼盗贼 `阿诺拉`/`塔雷诺斯`、德莱尼术士 `沃拉斯`/`阿莉丝`，以及血精灵萨满/牧师、暗夜精灵法师、亡灵萨满）不影响创建，因为 NPCBot 种族只是外观 + 种族技能。德莱尼缺盗贼/术士版本时，`bot_ai::RaceSpellForClass()` 已就近回退（盗贼→战士版 28880，法系→法师版 59548），不会再刷 `RaceSpellForClass(): unknows race:class combo 11, 4 / 11, 9` 和 `No SpellInfo found for base spell 0`。
- 新 bot 的模板（装备、掉落、等级区间等）取自 `creature_template_npcbot_extras` 中该职业的第一条记录（`class = X LIMIT 1`），只覆盖名称、模型与外观。
- `createnew` 不校验名称重复，重名会让依赖名称的 `.npcbot command ...` 定向失效；本批已确认无重复。
- 本批全部创建完后条目占用 71350-71449，`.npcbot lookup <职业ID> 1` 可核对。
