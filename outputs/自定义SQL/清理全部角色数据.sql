-- ============================================================================
-- 清理全部角色数据脚本
-- ============================================================================
-- 作用：清空 acore_characters 库中所有角色数据，并重置 NPCBot 的 owner 与装备。
-- 注意：
--   1. 本脚本【不包含账号信息】—— account_data / account_tutorial /
--      account_instance_times / addons 等账号级数据一律保留。
--   2. 世界事件、世界状态、聊天频道、GM 工单、审计日志等运营数据默认保留，
--      如需一并清理，请见文件末尾【可选清理】部分（默认已注释）。
--   3. 本脚本使用 TRUNCATE，会重置自增主键（新角色 guid 从 1 开始），且不可回滚。
--      执行前请务必备份数据库！
-- ============================================================================

-- 关闭外键检查，避免跨表清理时因约束报错
SET FOREIGN_KEY_CHECKS = 0;

-- === 账号数据 ===
TRUNCATE TABLE `account_data`;
TRUNCATE TABLE `account_instance_times`;
TRUNCATE TABLE `account_tutorial`; 
-- ----------------------------------------------------------------------------
-- 一、角色本体与角色级数据
-- ----------------------------------------------------------------------------
TRUNCATE TABLE `characters`;
TRUNCATE TABLE `character_account_data`;
TRUNCATE TABLE `character_achievement`;
TRUNCATE TABLE `character_achievement_offline_updates`;
TRUNCATE TABLE `character_achievement_progress`;
TRUNCATE TABLE `character_action`;
TRUNCATE TABLE `character_arena_stats`;
TRUNCATE TABLE `character_aura`;
TRUNCATE TABLE `character_banned`;
TRUNCATE TABLE `character_battleground_random`;
TRUNCATE TABLE `character_brew_of_the_month`;
TRUNCATE TABLE `character_declinedname`;
TRUNCATE TABLE `character_entry_point`;
TRUNCATE TABLE `character_equipmentsets`;
TRUNCATE TABLE `character_gifts`;
TRUNCATE TABLE `character_glyphs`;
TRUNCATE TABLE `character_homebind`;
TRUNCATE TABLE `character_instance`;
TRUNCATE TABLE `character_inventory`;
TRUNCATE TABLE `character_pet`;
TRUNCATE TABLE `character_pet_declinedname`;
TRUNCATE TABLE `character_queststatus`;
TRUNCATE TABLE `character_queststatus_daily`;
TRUNCATE TABLE `character_queststatus_monthly`;
TRUNCATE TABLE `character_queststatus_rewarded`;
TRUNCATE TABLE `character_queststatus_seasonal`;
TRUNCATE TABLE `character_queststatus_weekly`;
TRUNCATE TABLE `character_reputation`;
TRUNCATE TABLE `character_settings`;
TRUNCATE TABLE `character_skills`;
TRUNCATE TABLE `character_social`;
TRUNCATE TABLE `character_spell`;
TRUNCATE TABLE `character_spell_cooldown`;
TRUNCATE TABLE `character_stats`;
TRUNCATE TABLE `character_talent`;
TRUNCATE TABLE `character_transmog`;
TRUNCATE TABLE `account_bank_item`;
TRUNCATE TABLE `account_bank_slots`;
-- ----------------------------------------------------------------------------
-- 二、物品 / 邮件 / 宠物 / 尸体 / 拍卖行
-- ----------------------------------------------------------------------------
TRUNCATE TABLE `item_instance`;
TRUNCATE TABLE `item_loot_storage`;
TRUNCATE TABLE `item_refund_instance`;
TRUNCATE TABLE `item_soulbound_trade_data`;
TRUNCATE TABLE `recovery_item`;
TRUNCATE TABLE `mail`;
TRUNCATE TABLE `mail_items`;
TRUNCATE TABLE `mail_server_character`;
TRUNCATE TABLE `pet_aura`;
TRUNCATE TABLE `pet_spell`;
TRUNCATE TABLE `pet_spell_cooldown`;
TRUNCATE TABLE `corpse`;
TRUNCATE TABLE `auctionhouse`;

-- ----------------------------------------------------------------------------
-- 三、组队 / 公会 / 竞技场
-- ----------------------------------------------------------------------------
TRUNCATE TABLE `group_member`;
TRUNCATE TABLE `groups`;
TRUNCATE TABLE `guild`;
TRUNCATE TABLE `guild_bank_eventlog`;
TRUNCATE TABLE `guild_bank_item`;
TRUNCATE TABLE `guild_bank_right`;
TRUNCATE TABLE `guild_bank_tab`;
TRUNCATE TABLE `guild_eventlog`;
TRUNCATE TABLE `guild_member`;
TRUNCATE TABLE `guild_member_withdraw`;
TRUNCATE TABLE `guild_rank`;
TRUNCATE TABLE `arena_team`;
TRUNCATE TABLE `arena_team_member`;

-- ----------------------------------------------------------------------------
-- 四、副本 / PvP 统计 / 任务追踪 / LFG / 日历 / 公会章程
-- ----------------------------------------------------------------------------
TRUNCATE TABLE `instance`; 
TRUNCATE TABLE `instance_saved_go_state_data`;
TRUNCATE TABLE `zone_difficulty_instance_saves`;
TRUNCATE TABLE `pvpstats_battlegrounds`;
TRUNCATE TABLE `pvpstats_players`;
TRUNCATE TABLE `battleground_deserters`;
TRUNCATE TABLE `quest_tracker`;
TRUNCATE TABLE `lfg_data`;
TRUNCATE TABLE `calendar_events`;
TRUNCATE TABLE `calendar_invites`;
TRUNCATE TABLE `petition`;
TRUNCATE TABLE `petition_sign`;

-- ----------------------------------------------------------------------------
-- 五、世界重生状态（清档后世界恢复初始刷新状态）
-- ----------------------------------------------------------------------------
TRUNCATE TABLE `creature_respawn`;
TRUNCATE TABLE `gameobject_respawn`;

-- ----------------------------------------------------------------------------
-- 六、自定义模块角色数据
-- ----------------------------------------------------------------------------
TRUNCATE TABLE `mod_weapon_visual_effect`;
TRUNCATE TABLE `individualxp`;
TRUNCATE TABLE `zone_diffculty_playerlevel`;

-- ----------------------------------------------------------------------------
-- 七、NPCBot 重置（owner + 装备）
-- ----------------------------------------------------------------------------
-- 重置所有 BOT 的归属：owner 归零、清空共享拥有者、清空禁用技能与杂项值、
-- 清空雇佣时间，并将全部 18 个装备槽位置 0（装备模板数据不删除，只解除归属）。
UPDATE `characters_npcbot`
SET
    `owner`           = 0,
    `hire_time`       = '1970-01-01 08:00:01',
    `shared_owners`   = NULL,
    `spells_disabled` = NULL,
    `miscvalues`      = NULL,
    `equipMhEx`       = 0,
    `equipOhEx`       = 0,
    `equipRhEx`       = 0,
    `equipHead`       = 0,
    `equipShoulders`  = 0,
    `equipChest`      = 0,
    `equipWaist`      = 0,
    `equipLegs`       = 0,
    `equipFeet`       = 0,
    `equipWrist`      = 0,
    `equipHands`      = 0,
    `equipBack`       = 0,
    `equipBody`       = 0,
    `equipFinger1`    = 0,
    `equipFinger2`    = 0,
    `equipTrinket1`   = 0,
    `equipTrinket2`   = 0,
    `equipNeck`       = 0;

-- 清空 BOT 相关的装备仓库、套装、幻化、属性缓存、设置、组队与日志
TRUNCATE TABLE `characters_npcbot_gear_storage`;
TRUNCATE TABLE `characters_npcbot_gear_set`;
TRUNCATE TABLE `characters_npcbot_gear_set_item`;
TRUNCATE TABLE `characters_npcbot_transmog`;
TRUNCATE TABLE `characters_npcbot_stats`;
TRUNCATE TABLE `characters_npcbot_settings`;
TRUNCATE TABLE `characters_npcbot_group_member`;
TRUNCATE TABLE `characters_npcbot_logs`;

-- ----------------------------------------------------------------------------
-- 八、可选清理（默认注释，按需启用）
--     以下为运营日志 / 举报 / GM 工单等数据，严格来说不属于角色数据，
--     如需彻底清空可取消对应行的注释。
-- ----------------------------------------------------------------------------
TRUNCATE TABLE `log_money`;
TRUNCATE TABLE `log_arena_fights`;
TRUNCATE TABLE `log_arena_memberstats`;
TRUNCATE TABLE `log_encounter`;
-- TRUNCATE TABLE `gm_ticket`;
-- TRUNCATE TABLE `gm_survey`;
-- TRUNCATE TABLE `gm_subsurvey`;
-- TRUNCATE TABLE `spam_reports`;
-- TRUNCATE TABLE `lag_reports`;
TRUNCATE TABLE `players_reports_status`;
TRUNCATE TABLE `daily_players_reports`;
-- TRUNCATE TABLE `bugreport`;
-- TRUNCATE TABLE `reward_shop`;

-- 恢复外键检查
SET FOREIGN_KEY_CHECKS = 1;

-- ============================================================================
-- 完成
-- ============================================================================
