 
DELETE FROM `creature_template` WHERE (`entry` = 61000);
INSERT INTO `creature_template` (`entry`, `difficulty_entry_1`, `difficulty_entry_2`, `difficulty_entry_3`, `KillCredit1`, `KillCredit2`, `modelid1`, `modelid2`, `modelid3`, `modelid4`, `name`, `subname`, `IconName`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`, `faction`, `npcflag`, `speed_walk`, `speed_run`, `speed_swim`, `speed_flight`, `detection_range`, `scale`, `rank`, `dmgschool`, `DamageModifier`, `BaseAttackTime`, `RangeAttackTime`, `BaseVariance`, `RangeVariance`, `unit_class`, `unit_flags`, `unit_flags2`, `dynamicflags`, `family`, `trainer_type`, `trainer_spell`, `trainer_class`, `trainer_race`, `type`, `type_flags`, `lootid`, `pickpocketloot`, `skinloot`, `PetSpellDataId`, `VehicleId`, `mingold`, `maxgold`, `AIName`, `MovementType`, `HoverHeight`, `HealthModifier`, `ManaModifier`, `ArmorModifier`, `ExperienceModifier`, `RacialLeader`, `movementId`, `RegenHealth`, `mechanic_immune_mask`, `spell_school_immune_mask`, `flags_extra`, `ScriptName`, `VerifiedBuild`) VALUES
(61000, 0, 0, 0, 0, 0, 10008, 0, 0, 0, '克劳莉娅', '时光地下城主', NULL, 0, 83, 83, 0, 35, 1, 1, 1.14286, 1, 1, 20, 1.2, 0, 0, 1, 2000, 2000, 1, 1, 1, 33536, 2048, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, '', 0, 1, 1.35, 1, 1, 1, 0, 0, 1, 0, 0, 2, 'mod_zone_difficulty_dungeonmaster', 0);

SET @text_id 		:= 61000;
DELETE FROM `npc_text` WHERE `ID`>=@text_id and ID<=@text_id+2;
INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES (@text_id, '你好, $n. 听说你们很强力？我可以帮你开启更有挑战性的冒险！');
INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES (@text_id+1, '如果你反悔了，那么我可以帮你停止挑战！');
INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES (@text_id+2, '我可以开启不同难度的挑战，你必须有足够的资历才能开启更高难度的挑战。');
