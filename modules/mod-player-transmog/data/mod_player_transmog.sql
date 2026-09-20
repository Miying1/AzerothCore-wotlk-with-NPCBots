/*
 Navicat Premium Data Transfer

 Source Server         : 118.195.206.234
 Source Server Type    : MySQL
 Source Server Version : 80046
 Source Host           : 118.195.206.234:3306
 Source Schema         : acore_characters

 Target Server Type    : MySQL
 Target Server Version : 80046
 File Encoding         : 65001

 Date: 20/09/2026 15:39:07
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for mod_player_transmog
-- ----------------------------
DROP TABLE IF EXISTS `mod_player_transmog`;
CREATE TABLE `mod_player_transmog`  (
  `account_id` int NOT NULL,
  `modelid` int NOT NULL DEFAULT 0,
  `modelname` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci NULL DEFAULT NULL,
  `ccflag` int NULL DEFAULT 0,
  `quality` int NULL DEFAULT 1,
  PRIMARY KEY (`account_id`, `modelid`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_general_ci ROW_FORMAT = Dynamic;

SET FOREIGN_KEY_CHECKS = 1;
