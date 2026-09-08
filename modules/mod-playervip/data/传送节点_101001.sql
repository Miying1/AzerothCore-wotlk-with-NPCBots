-- 自定义传送节点（可见标记 gameobject，仅作视觉展示，传送直接按记录坐标进行）
-- gameobject entry：101001
-- 显示模型：displayId 6395（水晶/心脏样式，官方已有模型，参见 gameobject_template 180402/193794 先例）
-- 名字：传送节点（gameobject 名字固定读自 gameobject_template.name，无法运行时动态指定）
-- 用途：脚本「节点传送」在玩家当前位置召唤该 GO 作为视觉标记，2 小时后自动销毁；
--       传送时不再查找 GO，直接使用记录的 mapId + instanceId + 坐标
-- 执行数据库：acore_world
-- 依赖：npc_baihu_gossip.cpp（ScriptName 'npc_baihu_gossip' 已绑定到生物 101000）

-- 幂等清理：先删后插（重复执行无副作用）
DELETE FROM `gameobject_template` WHERE `entry` = 101001;

-- type=5（GO_TYPE_GENERIC 通用静态物体，无交互），displayId=6395，size=2（可自行调整大小）
INSERT INTO `gameobject_template`
(`entry`, `type`, `displayId`, `name`, `size`, `VerifiedBuild`) VALUES
(101001, 5, 6395, '传送节点', 1, 0);

-- ============================================================
-- 节点传送子菜单问候语（窗口顶部显示）
-- ID 101023 独立于随机欢迎语（欢迎语仅取 101000~101022），不会被随机选中
-- ============================================================
DELETE FROM `npc_text` WHERE `ID` = 101023;

INSERT INTO `npc_text` (`ID`, `text0_0`, `Probability0`) VALUES
(101023,'在你的当前位置建立一个传送节点，当处于同一个地图的时候，就可以直接传送到这个节点。',1);
