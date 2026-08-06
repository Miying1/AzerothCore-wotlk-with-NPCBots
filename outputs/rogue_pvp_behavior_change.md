# 盗贼 PvP 战斗条件调整

修改文件：`src/server/game/AI/NpcBots/bot_rogue_ai.cpp`

## 调整内容

- 目标为玩家控制单位时，`Killing Spree` 不再要求目标当前生命值高于盗贼最大生命值的 50%。
- 目标为玩家控制单位时，跳过 `Fan of Knives` 释放分支，因此 PvP 目标战斗中不释放刀扇。
- PvP 判定采用 `mytar->IsControlledByPlayer()`，覆盖玩家及玩家控制的 NPCBot/Pet，不依赖是否在战场地图。

## 检查

- `git diff --check -- src/server/game/AI/NpcBots/bot_rogue_ai.cpp`：通过
- 未运行构建或测试。
