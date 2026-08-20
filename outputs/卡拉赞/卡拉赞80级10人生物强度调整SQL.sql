-- 卡拉赞（地图 532）升级为 80 级 10 人：生物强度调整
-- 依据《卡拉赞80级10人升级设计文档》v1.3 §4
-- 仅改 creature_template 的 minlevel / maxlevel / exp / HealthModifier / DamageModifier，不改机制与脚本
-- 幂等：UPDATE 可重复执行（无副作用）
-- 说明：特殊机制怪（奥术异常体 16488）等级文档未明确，暂定为 81 级（与 C 档精英一致），可自行调整。

-- ===== 主线 Boss（83 级，exp=2，ICC 10 人普通基准 ×1.1）=====
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=385, `DamageModifier`=77 WHERE `entry`=15550; -- 猎手阿图门 Attumen
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=385, `DamageModifier`=77 WHERE `entry`=16152; -- 猎手阿图门（骑乘）Attumen
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=385, `DamageModifier`=66 WHERE `entry`=16151; -- 午夜 Midnight
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=385, `DamageModifier`=72 WHERE `entry`=15687; -- 莫罗斯 Moroes
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=462, `DamageModifier`=99 WHERE `entry`=16457; -- 贞洁圣女 Maiden of Virtue
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=550, `DamageModifier`=90 WHERE `entry`=15691; -- 馆长 The Curator
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=528, `DamageModifier`=83 WHERE `entry`=15688; -- 泰雷斯蒂安 Terestian Illhoof
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=638, `DamageModifier`=55 WHERE `entry`=16524; -- 埃兰之影 Shade of Aran
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=770, `DamageModifier`=97 WHERE `entry`=15689; -- 虚空幽龙 Netherspite
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=616, `DamageModifier`=92 WHERE `entry`=15690; -- 玛克扎尔王子 Prince Malchezaar
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=704, `DamageModifier`=110 WHERE `entry`=17225; -- 夜之魇 Nightbane

-- ===== 可选 Boss（83 级，exp=2，仆从区/节日）=====
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=275, `DamageModifier`=61 WHERE `entry`=16179; -- 潜伏者希亚基斯 Hyakiss
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=275, `DamageModifier`=61 WHERE `entry`=16180; -- 滑翔者沙迪基斯 Shadikith
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=253, `DamageModifier`=61 WHERE `entry`=16181; -- 蹂躏者罗卡德 Rokad
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=330, `DamageModifier`=66 WHERE `entry`=28194; -- 泰恩里斯·米尔克布拉德王子 Tenris

-- ===== 歌剧院 Boss（83 级，exp=2）=====
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=220, `DamageModifier`=66 WHERE `entry`=17535; -- 桃乐丝 Dorothee
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=220, `DamageModifier`=72 WHERE `entry`=17546; -- 狮吼 Roar
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=220, `DamageModifier`=72 WHERE `entry`=17543; -- 稻草人 Strawman
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=220, `DamageModifier`=72 WHERE `entry`=17547; -- 铁皮人 Tinhead
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=385, `DamageModifier`=83 WHERE `entry`=18168; -- 老巫婆 The Crone
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=385, `DamageModifier`=88 WHERE `entry`=17521; -- 大灰狼 The Big Bad Wolf
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=275, `DamageModifier`=72 WHERE `entry`=17533; -- 罗密欧 Romulo
UPDATE `creature_template` SET `minlevel`=83, `maxlevel`=83, `exp`=2, `HealthModifier`=275, `DamageModifier`=66 WHERE `entry`=17534; -- 朱丽叶 Julianne

-- ===== Boss 战斗型召唤物（82 级，沿用 v1.1，不改 exp）=====
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=25, `DamageModifier`=12 WHERE `entry`=17229; -- 基尔雷克 Kil'rek
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=2, `DamageModifier`=7.5 WHERE `entry`=17267; -- 恶魔小鬼 Fiendish Imp
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=2, `DamageModifier`=7.5 WHERE `entry`=17261; -- 不安的骷髅 Restless Skeleton
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=2, `DamageModifier`=7.5 WHERE `entry`=17167; -- 召唤元素 Conjured Elemental
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=8, `DamageModifier`=7.5 WHERE `entry`=17548; -- 托托 Tito（桃乐丝宠物）
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=25, `DamageModifier`=10 WHERE `entry`=17007; -- 莫罗斯宾客（1/6）
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=25, `DamageModifier`=10 WHERE `entry`=19872; -- 莫罗斯宾客（2/6）
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=25, `DamageModifier`=10 WHERE `entry`=19873; -- 莫罗斯宾客（3/6）
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=25, `DamageModifier`=10 WHERE `entry`=19874; -- 莫罗斯宾客（4/6）
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=25, `DamageModifier`=10 WHERE `entry`=19875; -- 莫罗斯宾客（5/6）
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=25, `DamageModifier`=10 WHERE `entry`=19876; -- 莫罗斯宾客（6/6）
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=2, `DamageModifier`=7.5 WHERE `entry`=17646; -- 虚空地狱火 Netherspite Infernal

-- ===== 精英怪 A 档（高危，82 级，v1.1 上调 30%，不改 exp）=====
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=65, `DamageModifier`=23 WHERE `entry`=16596; -- 巨型血肉兽 Greater Fleshbeast
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=59, `DamageModifier`=21 WHERE `entry`=16481; -- 鬼魅游魂 Ghastly Haunt
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=65, `DamageModifier`=20 WHERE `entry`=16504; -- 奥术守卫者 Arcane Protector
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=52, `DamageModifier`=20 WHERE `entry`=16408; -- 幻影侍从 Phantom Valet
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=52, `DamageModifier`=20 WHERE `entry`=16472; -- 幻影舞台工 Phantom Stagehand
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=59, `DamageModifier`=18 WHERE `entry`=16545; -- 虚无窃法者 Ethereal Spellfilcher
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=52, `DamageModifier`=18 WHERE `entry`=16544; -- 虚无盗贼 Ethereal Thief
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=59, `DamageModifier`=18 WHERE `entry`=16482; -- 被困灵魂 Trapped Soul
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=59, `DamageModifier`=17 WHERE `entry`=16595; -- 血肉兽 Fleshbeast
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=62, `DamageModifier`=17 WHERE `entry`=16471; -- 骷髅引座员 Skeletal Usher
UPDATE `creature_template` SET `minlevel`=82, `maxlevel`=82, `HealthModifier`=62, `DamageModifier`=17 WHERE `entry`=16485; -- 奥术看守 Arcane Watchman

-- ===== 精英怪 B 档（中危，81–82 级，v1.1 上调 30%，不改 exp）=====
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=39, `DamageModifier`=14 WHERE `entry`=16410; -- 幽灵侍从 Spectral Retainer
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=39, `DamageModifier`=14 WHERE `entry`=16473; -- 幽灵演员 Spectral Performer
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=36, `DamageModifier`=14 WHERE `entry`=16411; -- 幽灵厨师 Spectral Chef
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=36, `DamageModifier`=13 WHERE `entry`=16177; -- 恐惧兽 Dreadbeast
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=39, `DamageModifier`=13 WHERE `entry`=16174; -- 巨型暗影蝠 Greater Shadowbat
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=39, `DamageModifier`=13 WHERE `entry`=16489; -- 混乱意识 Chaotic Sentience
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=39, `DamageModifier`=13 WHERE `entry`=15547; -- 幽灵战马 Spectral Charger
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=34, `DamageModifier`=13 WHERE `entry`=16415; -- 骷髅侍者 Skeletal Waiter
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=34, `DamageModifier`=13 WHERE `entry`=16171; -- 冷雾寡妇 Coldmist Widow
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=36, `DamageModifier`=12 WHERE `entry`=16459; -- 浪荡女主人 Wanton Hostess
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=36, `DamageModifier`=12 WHERE `entry`=16460; -- 黑夜女主人 Night Mistress
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=36, `DamageModifier`=12 WHERE `entry`=16461; -- 妾室 Concubine
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=36, `DamageModifier`=12 WHERE `entry`=16526; -- 巫师之影 Sorcerous Shade
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=36, `DamageModifier`=12 WHERE `entry`=16529; -- 魔法恐魔 Magical Horror
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=33, `DamageModifier`=12 WHERE `entry`=16424; -- 幽灵哨兵 Spectral Sentry
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=33, `DamageModifier`=12 WHERE `entry`=16175; -- 吸血暗影蝠 Vampiric Shadowbat
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=82, `HealthModifier`=33, `DamageModifier`=12 WHERE `entry`=16525; -- 法术之影 Spell Shade

-- ===== 精英怪 C 档（低危，81 级，v1.1 上调 30%，不改 exp）=====
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=29, `DamageModifier`=10 WHERE `entry`=16470; -- 幽灵慈善家 Ghostly Philanthropist
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=29, `DamageModifier`=10 WHERE `entry`=16414; -- 幽灵管家 Ghostly Steward
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=26, `DamageModifier`=10 WHERE `entry`=16406; -- 幻影侍者 Phantom Attendant
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=26, `DamageModifier`=10 WHERE `entry`=16407; -- 幽灵仆人 Spectral Servant
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=26, `DamageModifier`=10 WHERE `entry`=16412; -- 幽灵面包师 Ghostly Baker
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=26, `DamageModifier`=10 WHERE `entry`=16176; -- 暗影兽 Shadowbeast
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=23, `DamageModifier`=10 WHERE `entry`=16389; -- 幽灵学徒 Spectral Apprentice
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=23, `DamageModifier`=10 WHERE `entry`=16425; -- 幻影卫兵 Phantom Guardsman
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=23, `DamageModifier`=10 WHERE `entry`=15551; -- 幽灵马夫 Spectral Stable Hand
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=23, `DamageModifier`=10 WHERE `entry`=16540; -- 暗影掠夺者 Shadow Pillager
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=18, `DamageModifier`=10 WHERE `entry`=16178; -- 相位猎犬 Phase Hound
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=16, `DamageModifier`=10 WHERE `entry`=16170; -- 冷雾追踪者 Coldmist Stalker
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=16, `DamageModifier`=10 WHERE `entry`=16173; -- 暗影蝠 Shadowbat
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=23, `DamageModifier`=10 WHERE `entry`=16530; -- 法力扭曲 Mana Warp
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=16, `DamageModifier`=10 WHERE `entry`=16492; -- 汲取者 Syphoner
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=16, `DamageModifier`=10 WHERE `entry`=16491; -- 法力吞噬者 Mana Feeder
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=29, `DamageModifier`=10 WHERE `entry`=15548; -- 幽灵公马 Spectral Stallion
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=16, `DamageModifier`=10 WHERE `entry`=16539; -- 荷蒙克鲁斯 Homunculus

-- ===== 特殊机制怪（图书馆爆炸机制怪，保留脆皮高威胁，等级暂定 81）=====
UPDATE `creature_template` SET `minlevel`=81, `maxlevel`=81, `HealthModifier`=0.7, `DamageModifier`=13 WHERE `entry`=16488; -- 奥术异常体 Arcane Anomaly

-- ===== 普通怪（rank=0，仅升等级 80）=====
UPDATE `creature_template` SET `minlevel`=80, `maxlevel`=80 WHERE `entry`=16409; -- 幻影宾客 Phantom Guest
UPDATE `creature_template` SET `minlevel`=80, `maxlevel`=80 WHERE `entry`=16468; -- 幽灵赞助人 Spectral Patron
