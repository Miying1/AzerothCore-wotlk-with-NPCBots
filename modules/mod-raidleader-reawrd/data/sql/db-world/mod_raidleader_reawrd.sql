/*
 Navicat Premium Data Transfer

 Source Server         : 125.122.24.125
 Source Server Type    : MySQL
 Source Server Version : 50743
 Source Host           : 125.122.24.125:3306
 Source Schema         : acore_world

 Target Server Type    : MySQL
 Target Server Version : 50743
 File Encoding         : 65001

 Date: 06/02/2024 15:00:47
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for mod_raidleader_reawrd
-- ----------------------------
DROP TABLE IF EXISTS `mod_raidleader_reawrd`;
CREATE TABLE `mod_raidleader_reawrd`  (
  `bossid` int(11) NOT NULL,
  `descp` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `player_count1` int(11) NULL DEFAULT 0,
  `reawrd1` int(11) NULL DEFAULT 0,
  `player_count2` int(11) NULL DEFAULT 0,
  `reawrd2` int(11) NULL DEFAULT 0,
  `active` int(11) NULL DEFAULT 0,
  PRIMARY KEY (`bossid`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;
