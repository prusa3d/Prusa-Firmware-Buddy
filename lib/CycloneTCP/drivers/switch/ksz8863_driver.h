/**
 * @file ksz8863_driver.h
 * @brief KSZ8863 3-port Ethernet switch driver
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _KSZ8863_DRIVER_H
#define _KSZ8863_DRIVER_H

//Dependencies
#include "core/nic.h"

//Port identifiers
#define KSZ8863_PORT1 1
#define KSZ8863_PORT2 2
#define KSZ8863_PORT3 3

//Port masks
#define KSZ8863_PORT_MASK  0x07
#define KSZ8863_PORT1_MASK 0x01
#define KSZ8863_PORT2_MASK 0x02
#define KSZ8863_PORT3_MASK 0x04

//SPI command byte
#define KSZ8863_SPI_CMD_WRITE 0x02
#define KSZ8863_SPI_CMD_READ  0x03

//Size of static and dynamic MAC tables
#define KSZ8863_STATIC_MAC_TABLE_SIZE  8
#define KSZ8863_DYNAMIC_MAC_TABLE_SIZE 1024

//Tail tag rules (host to KSZ8863)
#define KSZ8863_TAIL_TAG_PRIORITY           0x0C
#define KSZ8863_TAIL_TAG_DEST_PORT2         0x02
#define KSZ8863_TAIL_TAG_DEST_PORT1         0x01
#define KSZ8863_TAIL_TAG_NORMAL_ADDR_LOOKUP 0x00

//Tail tag rules (KSZ8863 to host)
#define KSZ8863_TAIL_TAG_SRC_PORT           0x01

//KSZ8863 PHY registers
#define KSZ8863_BMCR                                               0x00
#define KSZ8863_BMSR                                               0x01
#define KSZ8863_PHYID1                                             0x02
#define KSZ8863_PHYID2                                             0x03
#define KSZ8863_ANAR                                               0x04
#define KSZ8863_ANLPAR                                             0x05
#define KSZ8863_LINKMD                                             0x1D
#define KSZ8863_PHYSCS                                             0x1F

//KSZ8863 Switch registers
#define KSZ8863_CHIP_ID0                                           0x00
#define KSZ8863_CHIP_ID1                                           0x01
#define KSZ8863_GLOBAL_CTRL0                                       0x02
#define KSZ8863_GLOBAL_CTRL1                                       0x03
#define KSZ8863_GLOBAL_CTRL2                                       0x04
#define KSZ8863_GLOBAL_CTRL3                                       0x05
#define KSZ8863_GLOBAL_CTRL4                                       0x06
#define KSZ8863_GLOBAL_CTRL5                                       0x07
#define KSZ8863_GLOBAL_CTRL6                                       0x08
#define KSZ8863_GLOBAL_CTRL7                                       0x09
#define KSZ8863_GLOBAL_CTRL8                                       0x0A
#define KSZ8863_GLOBAL_CTRL9                                       0x0B
#define KSZ8863_GLOBAL_CTRL10                                      0x0C
#define KSZ8863_GLOBAL_CTRL11                                      0x0D
#define KSZ8863_GLOBAL_CTRL12                                      0x0E
#define KSZ8863_GLOBAL_CTRL13                                      0x0F
#define KSZ8863_PORT1_CTRL0                                        0x10
#define KSZ8863_PORT1_CTRL1                                        0x11
#define KSZ8863_PORT1_CTRL2                                        0x12
#define KSZ8863_PORT1_CTRL3                                        0x13
#define KSZ8863_PORT1_CTRL4                                        0x14
#define KSZ8863_PORT1_CTRL5                                        0x15
#define KSZ8863_PORT1_Q0_IG_LIMIT                                  0x16
#define KSZ8863_PORT1_Q1_IG_LIMIT                                  0x17
#define KSZ8863_PORT1_Q2_IG_LIMIT                                  0x18
#define KSZ8863_PORT1_Q3_IG_LIMIT                                  0x19
#define KSZ8863_PORT1_PSCS                                         0x1A
#define KSZ8863_PORT1_LINKMD                                       0x1B
#define KSZ8863_PORT1_CTRL12                                       0x1C
#define KSZ8863_PORT1_CTRL13                                       0x1D
#define KSZ8863_PORT1_STAT0                                        0x1E
#define KSZ8863_PORT1_STAT1                                        0x1F
#define KSZ8863_PORT2_CTRL0                                        0x20
#define KSZ8863_PORT2_CTRL1                                        0x21
#define KSZ8863_PORT2_CTRL2                                        0x22
#define KSZ8863_PORT2_CTRL3                                        0x23
#define KSZ8863_PORT2_CTRL4                                        0x24
#define KSZ8863_PORT2_CTRL5                                        0x25
#define KSZ8863_PORT2_Q0_IG_LIMIT                                  0x26
#define KSZ8863_PORT2_Q1_IG_LIMIT                                  0x27
#define KSZ8863_PORT2_Q2_IG_LIMIT                                  0x28
#define KSZ8863_PORT2_Q3_IG_LIMIT                                  0x29
#define KSZ8863_PORT2_PSCS                                         0x2A
#define KSZ8863_PORT2_LINKMD                                       0x2B
#define KSZ8863_PORT2_CTRL12                                       0x2C
#define KSZ8863_PORT2_CTRL13                                       0x2D
#define KSZ8863_PORT2_STAT0                                        0x2E
#define KSZ8863_PORT2_STAT1                                        0x2F
#define KSZ8863_PORT3_CTRL0                                        0x30
#define KSZ8863_PORT3_CTRL1                                        0x31
#define KSZ8863_PORT3_CTRL2                                        0x32
#define KSZ8863_PORT3_CTRL3                                        0x33
#define KSZ8863_PORT3_CTRL4                                        0x34
#define KSZ8863_PORT3_CTRL5                                        0x35
#define KSZ8863_PORT3_Q0_IG_LIMIT                                  0x36
#define KSZ8863_PORT3_Q1_IG_LIMIT                                  0x37
#define KSZ8863_PORT3_Q2_IG_LIMIT                                  0x38
#define KSZ8863_PORT3_Q3_IG_LIMIT                                  0x39
#define KSZ8863_PORT3_STAT0                                        0x3E
#define KSZ8863_PORT3_STAT1                                        0x3F
#define KSZ8863_RESET                                              0x43
#define KSZ8863_TOS_PRIO_CTRL0                                     0x60
#define KSZ8863_TOS_PRIO_CTRL1                                     0x61
#define KSZ8863_TOS_PRIO_CTRL2                                     0x62
#define KSZ8863_TOS_PRIO_CTRL3                                     0x63
#define KSZ8863_TOS_PRIO_CTRL4                                     0x64
#define KSZ8863_TOS_PRIO_CTRL5                                     0x65
#define KSZ8863_TOS_PRIO_CTRL6                                     0x66
#define KSZ8863_TOS_PRIO_CTRL7                                     0x67
#define KSZ8863_TOS_PRIO_CTRL8                                     0x68
#define KSZ8863_TOS_PRIO_CTRL9                                     0x69
#define KSZ8863_TOS_PRIO_CTRL10                                    0x6A
#define KSZ8863_TOS_PRIO_CTRL11                                    0x6B
#define KSZ8863_TOS_PRIO_CTRL12                                    0x6C
#define KSZ8863_TOS_PRIO_CTRL13                                    0x6D
#define KSZ8863_TOS_PRIO_CTRL14                                    0x6E
#define KSZ8863_TOS_PRIO_CTRL15                                    0x6F
#define KSZ8863_MAC_ADDR0                                          0x70
#define KSZ8863_MAC_ADDR1                                          0x71
#define KSZ8863_MAC_ADDR2                                          0x72
#define KSZ8863_MAC_ADDR3                                          0x73
#define KSZ8863_MAC_ADDR4                                          0x74
#define KSZ8863_MAC_ADDR5                                          0x75
#define KSZ8863_UDR1                                               0x76
#define KSZ8863_UDR2                                               0x77
#define KSZ8863_UDR3                                               0x78
#define KSZ8863_INDIRECT_CTRL0                                     0x79
#define KSZ8863_INDIRECT_CTRL1                                     0x7A
#define KSZ8863_INDIRECT_DATA8                                     0x7B
#define KSZ8863_INDIRECT_DATA7                                     0x7C
#define KSZ8863_INDIRECT_DATA6                                     0x7D
#define KSZ8863_INDIRECT_DATA5                                     0x7E
#define KSZ8863_INDIRECT_DATA4                                     0x7F
#define KSZ8863_INDIRECT_DATA3                                     0x80
#define KSZ8863_INDIRECT_DATA2                                     0x81
#define KSZ8863_INDIRECT_DATA1                                     0x82
#define KSZ8863_INDIRECT_DATA0                                     0x83
#define KSZ8863_MACA1                                              0x8E
#define KSZ8863_MACA2                                              0x94
#define KSZ8863_PORT1_Q0_EG_LIMIT                                  0x9A
#define KSZ8863_PORT1_Q1_EG_LIMIT                                  0x9B
#define KSZ8863_PORT1_Q2_EG_LIMIT                                  0x9C
#define KSZ8863_PORT1_Q3_EG_LIMIT                                  0x9D
#define KSZ8863_PORT2_Q0_EG_LIMIT                                  0x9E
#define KSZ8863_PORT2_Q1_EG_LIMIT                                  0x9F
#define KSZ8863_PORT2_Q2_EG_LIMIT                                  0xA0
#define KSZ8863_PORT2_Q3_EG_LIMIT                                  0xA1
#define KSZ8863_PORT3_Q0_EG_LIMIT                                  0xA2
#define KSZ8863_PORT3_Q1_EG_LIMIT                                  0xA3
#define KSZ8863_PORT3_Q2_EG_LIMIT                                  0xA4
#define KSZ8863_PORT3_Q3_EG_LIMIT                                  0xA5
#define KSZ8863_MODE_INDICATOR                                     0xA6
#define KSZ8863_HIGH_PRIO_PKT_BUF_Q3                               0xA7
#define KSZ8863_HIGH_PRIO_PKT_BUF_Q2                               0xA8
#define KSZ8863_HIGH_PRIO_PKT_BUF_Q1                               0xA9
#define KSZ8863_HIGH_PRIO_PKT_BUF_Q0                               0xAA
#define KSZ8863_PM_USAGE_FLOW_CTRL_SEL_MODE1                       0xAB
#define KSZ8863_PM_USAGE_FLOW_CTRL_SEL_MODE2                       0xAC
#define KSZ8863_PM_USAGE_FLOW_CTRL_SEL_MODE3                       0xAD
#define KSZ8863_PM_USAGE_FLOW_CTRL_SEL_MODE4                       0xAE
#define KSZ8863_PORT1_Q3_TXQ_SPLIT                                 0xAF
#define KSZ8863_PORT1_Q2_TXQ_SPLIT                                 0xB0
#define KSZ8863_PORT1_Q1_TXQ_SPLIT                                 0xB1
#define KSZ8863_PORT1_Q0_TXQ_SPLIT                                 0xB2
#define KSZ8863_PORT2_Q3_TXQ_SPLIT                                 0xB3
#define KSZ8863_PORT2_Q2_TXQ_SPLIT                                 0xB4
#define KSZ8863_PORT2_Q1_TXQ_SPLIT                                 0xB5
#define KSZ8863_PORT2_Q0_TXQ_SPLIT                                 0xB6
#define KSZ8863_PORT3_Q3_TXQ_SPLIT                                 0xB7
#define KSZ8863_PORT3_Q2_TXQ_SPLIT                                 0xB8
#define KSZ8863_PORT3_Q1_TXQ_SPLIT                                 0xB9
#define KSZ8863_PORT3_Q0_TXQ_SPLIT                                 0xBA
#define KSZ8863_INT_EN                                             0xBB
#define KSZ8863_LINK_CHANGE_INT                                    0xBC
#define KSZ8863_FORCE_PAUSE_OFF_LIMIT_EN                           0xBD
#define KSZ8863_FIBER_SIGNAL_THRESHOLD                             0xC0
#define KSZ8863_INTERNAL_LDO_CTRL                                  0xC1
#define KSZ8863_INSERT_SRC_PVID                                    0xC2
#define KSZ8863_PWR_MGMT_LED_MODE                                  0xC3
#define KSZ8863_SLEEP_MODE                                         0xC4
#define KSZ8863_FWD_INVALID_VID_HOST_MODE                          0xC6

//KSZ8863 Switch register access macros
#define KSZ8863_PORTn_CTRL0(port)                                  (0x00 + ((port) * 0x10))
#define KSZ8863_PORTn_CTRL1(port)                                  (0x01 + ((port) * 0x10))
#define KSZ8863_PORTn_CTRL2(port)                                  (0x02 + ((port) * 0x10))
#define KSZ8863_PORTn_CTRL3(port)                                  (0x03 + ((port) * 0x10))
#define KSZ8863_PORTn_CTRL4(port)                                  (0x04 + ((port) * 0x10))
#define KSZ8863_PORTn_CTRL5(port)                                  (0x05 + ((port) * 0x10))
#define KSZ8863_PORTn_Q0_IG_LIMIT(port)                            (0x06 + ((port) * 0x10))
#define KSZ8863_PORTn_Q1_IG_LIMIT(port)                            (0x07 + ((port) * 0x10))
#define KSZ8863_PORTn_Q2_IG_LIMIT(port)                            (0x08 + ((port) * 0x10))
#define KSZ8863_PORTn_Q3_IG_LIMIT(port)                            (0x09 + ((port) * 0x10))
#define KSZ8863_PORTn_PSCS(port)                                   (0x0A + ((port) * 0x10))
#define KSZ8863_PORTn_LINKMD(port)                                 (0x0B + ((port) * 0x10))
#define KSZ8863_PORTn_CTRL12(port)                                 (0x0C + ((port) * 0x10))
#define KSZ8863_PORTn_CTRL13(port)                                 (0x0D + ((port) * 0x10))
#define KSZ8863_PORTn_STAT0(port)                                  (0x0E + ((port) * 0x10))
#define KSZ8863_PORTn_STAT1(port)                                  (0x0F + ((port) * 0x10))
#define KSZ8863_PORTn_Q0_EG_LIMIT(port)                            (0x96 + ((port) * 0x04))
#define KSZ8863_PORTn_Q1_EG_LIMIT(port)                            (0x97 + ((port) * 0x04))
#define KSZ8863_PORTn_Q2_EG_LIMIT(port)                            (0x98 + ((port) * 0x04))
#define KSZ8863_PORTn_Q3_EG_LIMIT(port)                            (0x99 + ((port) * 0x04))
#define KSZ8863_PORTn_Q3_TXQ_SPLIT(port)                           (0xAB + ((port) * 0x04))
#define KSZ8863_PORTn_Q2_TXQ_SPLIT(port)                           (0xAC + ((port) * 0x04))
#define KSZ8863_PORTn_Q1_TXQ_SPLIT(port)                           (0xAD + ((port) * 0x04))
#define KSZ8863_PORTn_Q0_TXQ_SPLIT(port)                           (0xAE + ((port) * 0x04))

//MII Basic Control register
#define KSZ8863_BMCR_RESET                                         0x8000
#define KSZ8863_BMCR_LOOPBACK                                      0x4000
#define KSZ8863_BMCR_FORCE_100                                     0x2000
#define KSZ8863_BMCR_AN_EN                                         0x1000
#define KSZ8863_BMCR_POWER_DOWN                                    0x0800
#define KSZ8863_BMCR_ISOLATE                                       0x0400
#define KSZ8863_BMCR_RESTART_AN                                    0x0200
#define KSZ8863_BMCR_FORCE_FULL_DUPLEX                             0x0100
#define KSZ8863_BMCR_COL_TEST                                      0x0080
#define KSZ8863_BMCR_HP_MDIX                                       0x0020
#define KSZ8863_BMCR_FORCE_MDI                                     0x0010
#define KSZ8863_BMCR_AUTO_MDIX_DIS                                 0x0008
#define KSZ8863_BMCR_FAR_END_FAULT_DIS                             0x0004
#define KSZ8863_BMCR_TRANSMIT_DIS                                  0x0002
#define KSZ8863_BMCR_LED_DIS                                       0x0001

//MII Basic Status register
#define KSZ8863_BMSR_100BT4                                        0x8000
#define KSZ8863_BMSR_100BTX_FD                                     0x4000
#define KSZ8863_BMSR_100BTX_HD                                     0x2000
#define KSZ8863_BMSR_10BT_FD                                       0x1000
#define KSZ8863_BMSR_10BT_HD                                       0x0800
#define KSZ8863_BMSR_PREAMBLE_SUPPR                                0x0040
#define KSZ8863_BMSR_AN_COMPLETE                                   0x0020
#define KSZ8863_BMSR_FAR_END_FAULT                                 0x0010
#define KSZ8863_BMSR_AN_CAPABLE                                    0x0008
#define KSZ8863_BMSR_LINK_STATUS                                   0x0004
#define KSZ8863_BMSR_JABBER_TEST                                   0x0002
#define KSZ8863_BMSR_EXTENDED_CAPABLE                              0x0001

//PHYID High register
#define KSZ8863_PHYID1_DEFAULT                                     0x0022

//PHYID Low register
#define KSZ8863_PHYID2_DEFAULT                                     0x1430

//Auto-Negotiation Advertisement Ability register
#define KSZ8863_ANAR_NEXT_PAGE                                     0x8000
#define KSZ8863_ANAR_REMOTE_FAULT                                  0x2000
#define KSZ8863_ANAR_PAUSE                                         0x0400
#define KSZ8863_ANAR_100BTX_FD                                     0x0100
#define KSZ8863_ANAR_100BTX_HD                                     0x0080
#define KSZ8863_ANAR_10BT_FD                                       0x0040
#define KSZ8863_ANAR_10BT_HD                                       0x0020
#define KSZ8863_ANAR_SELECTOR                                      0x001F
#define KSZ8863_ANAR_SELECTOR_DEFAULT                              0x0001

//Auto-Negotiation Link Partner Ability register
#define KSZ8863_ANLPAR_NEXT_PAGE                                   0x8000
#define KSZ8863_ANLPAR_LP_ACK                                      0x4000
#define KSZ8863_ANLPAR_REMOTE_FAULT                                0x2000
#define KSZ8863_ANLPAR_PAUSE                                       0x0400
#define KSZ8863_ANLPAR_100BTX_FD                                   0x0100
#define KSZ8863_ANLPAR_100BTX_HD                                   0x0080
#define KSZ8863_ANLPAR_10BT_FD                                     0x0040
#define KSZ8863_ANLPAR_10BT_HD                                     0x0020

//LinkMD Control/Status register
#define KSZ8863_LINKMD_TEST_EN                                     0x8000
#define KSZ8863_LINKMD_RESULT                                      0x6000
#define KSZ8863_LINKMD_SHORT                                       0x1000
#define KSZ8863_LINKMD_FAULT_COUNT                                 0x01FF

//PHY Special Control/Status register
#define KSZ8863_PHYSCS_OP_MODE                                     0x0700
#define KSZ8863_PHYSCS_OP_MODE_AN                                  0x0100
#define KSZ8863_PHYSCS_OP_MODE_10BT_HD                             0x0200
#define KSZ8863_PHYSCS_OP_MODE_100BTX_HD                           0x0300
#define KSZ8863_PHYSCS_OP_MODE_10BT_FD                             0x0500
#define KSZ8863_PHYSCS_OP_MODE_100BTX_FD                           0x0600
#define KSZ8863_PHYSCS_OP_MODE_ISOLATE                             0x0700
#define KSZ8863_PHYSCS_POLRVS                                      0x0020
#define KSZ8863_PHYSCS_MDIX_STATUS                                 0x0010
#define KSZ8863_PHYSCS_FORCE_LINK                                  0x0008
#define KSZ8863_PHYSCS_PWRSAVE                                     0x0004
#define KSZ8863_PHYSCS_REMOTE_LOOPBACK                             0x0002

//Chip ID0 register
#define KSZ8863_CHIP_ID0_FAMILY_ID                                 0xFF
#define KSZ8863_CHIP_ID0_FAMILY_ID_DEFAULT                         0x88

//Chip ID1 / Start Switch register
#define KSZ8863_CHIP_ID1_CHIP_ID                                   0xF0
#define KSZ8863_CHIP_ID1_CHIP_ID_DEFAULT                           0x30
#define KSZ8863_CHIP_ID1_REVISION_ID                               0x0E
#define KSZ8863_CHIP_ID1_START_SWITCH                              0x01

//Global Control 0 register
#define KSZ8863_GLOBAL_CTRL0_NEW_BACK_OFF_EN                       0x80
#define KSZ8863_GLOBAL_CTRL0_FLUSH_DYNAMIC_MAC_TABLE               0x20
#define KSZ8863_GLOBAL_CTRL0_FLUSH_STATIC_MAC_TABLE                0x10
#define KSZ8863_GLOBAL_CTRL0_PASS_FLOW_CTRL_PKT                    0x08

//Global Control 1 register
#define KSZ8863_GLOBAL_CTRL1_PASS_ALL_FRAMES                       0x80
#define KSZ8863_GLOBAL_CTRL1_TAIL_TAG_EN                           0x40
#define KSZ8863_GLOBAL_CTRL1_TX_FLOW_CTRL_EN                       0x20
#define KSZ8863_GLOBAL_CTRL1_RX_FLOW_CTRL_EN                       0x10
#define KSZ8863_GLOBAL_CTRL1_FRAME_LEN_CHECK_EN                    0x08
#define KSZ8863_GLOBAL_CTRL1_AGING_EN                              0x04
#define KSZ8863_GLOBAL_CTRL1_FAST_AGE_EN                           0x02
#define KSZ8863_GLOBAL_CTRL1_AGGRESSIVE_BACK_OFF_EN                0x01

//Global Control 2 register
#define KSZ8863_GLOBAL_CTRL2_UNI_VLAN_MISMATCH_DISCARD             0x80
#define KSZ8863_GLOBAL_CTRL2_MCAST_STORM_PROTECT_DIS               0x40
#define KSZ8863_GLOBAL_CTRL2_BACK_PRESSURE_MODE                    0x20
#define KSZ8863_GLOBAL_CTRL2_FLOW_CTRL_FAIR_MODE                   0x10
#define KSZ8863_GLOBAL_CTRL2_NO_EXCESSIVE_COL_DROP                 0x08
#define KSZ8863_GLOBAL_CTRL2_HUGE_PACKET_SUPPORT                   0x04
#define KSZ8863_GLOBAL_CTRL2_MAX_PACKET_SIZE_CHECK_EN              0x02

//Global Control 3 register
#define KSZ8863_GLOBAL_CTRL3_VLAN_EN                               0x80
#define KSZ8863_GLOBAL_CTRL3_IGMP_SNOOP_EN                         0x40
#define KSZ8863_GLOBAL_CTRL3_WEIGHTED_FAIR_QUEUE_EN                0x08
#define KSZ8863_GLOBAL_CTRL3_SNIFF_MODE_SEL                        0x01

//Global Control 4 register
#define KSZ8863_GLOBAL_CTRL4_SW_MII_HALF_DUPLEX_MODE               0x40
#define KSZ8863_GLOBAL_CTRL4_SW_MII_FLOW_CTRL_EN                   0x20
#define KSZ8863_GLOBAL_CTRL4_SW_MII_10BT                           0x10
#define KSZ8863_GLOBAL_CTRL4_NULL_VID_REPLACEMENT                  0x08
#define KSZ8863_GLOBAL_CTRL4_BCAST_STORM_PROTECT_RATE_MSB          0x07

//Global Control 5 register
#define KSZ8863_GLOBAL_CTRL5_BCAST_STORM_PROTECT_RATE_LSB          0xFF

//Global Control 6 register
#define KSZ8863_GLOBAL_CTRL6_FACTORY_TESTING                       0xFF

//Global Control 7 register
#define KSZ8863_GLOBAL_CTRL7_FACTORY_TESTING                       0xFF

//Global Control 8 register
#define KSZ8863_GLOBAL_CTRL8_FACTORY_TESTING                       0xFF

//Global Control 9 register
#define KSZ8863_GLOBAL_CTRL9_CPU_IF_CLK_SEL                        0xC0
#define KSZ8863_GLOBAL_CTRL9_CPU_IF_CLK_SEL_31_25MHZ               0x00
#define KSZ8863_GLOBAL_CTRL9_CPU_IF_CLK_SEL_62_5MHZ                0x40
#define KSZ8863_GLOBAL_CTRL9_CPU_IF_CLK_SEL_125MHZ                 0x80

//Global Control 10 register
#define KSZ8863_GLOBAL_CTRL10_TAG3                                 0xC0
#define KSZ8863_GLOBAL_CTRL10_TAG2                                 0x30
#define KSZ8863_GLOBAL_CTRL10_TAG1                                 0x0C
#define KSZ8863_GLOBAL_CTRL10_TAG0                                 0x03

//Global Control 11 register
#define KSZ8863_GLOBAL_CTRL11_TAG7                                 0xC0
#define KSZ8863_GLOBAL_CTRL11_TAG6                                 0x30
#define KSZ8863_GLOBAL_CTRL11_TAG5                                 0x0C
#define KSZ8863_GLOBAL_CTRL11_TAG4                                 0x03

//Global Control 12 register
#define KSZ8863_GLOBAL_CTRL12_UNKNOWN_PKT_DEFAULT_PORT_EN          0x80
#define KSZ8863_GLOBAL_CTRL12_DRIVER_STRENGTH                      0x40
#define KSZ8863_GLOBAL_CTRL12_DRIVER_STRENGTH_8MA                  0x00
#define KSZ8863_GLOBAL_CTRL12_DRIVER_STRENGTH_16MA                 0x40
#define KSZ8863_GLOBAL_CTRL12_UNKNOWN_PKT_DEFAULT_PORT             0x07
#define KSZ8863_GLOBAL_CTRL12_UNKNOWN_PKT_DEFAULT_PORT_PORT1       0x01
#define KSZ8863_GLOBAL_CTRL12_UNKNOWN_PKT_DEFAULT_PORT_PORT2       0x02
#define KSZ8863_GLOBAL_CTRL12_UNKNOWN_PKT_DEFAULT_PORT_PORT3       0x04

//Global Control 13 register
#define KSZ8863_GLOBAL_CTRL13_PHY_ADDR                             0xF8

//Port N Control 0 register
#define KSZ8863_PORTn_CTRL0_BCAST_STORM_PROTECT_EN                 0x80
#define KSZ8863_PORTn_CTRL0_DIFFSERV_PRIO_CLASS_EN                 0x40
#define KSZ8863_PORTn_CTRL0_802_1P_PRIO_CLASS_EN                   0x20
#define KSZ8863_PORTn_CTRL0_PORT_PRIO_CLASS_EN                     0x18
#define KSZ8863_PORTn_CTRL0_TAG_INSERTION                          0x04
#define KSZ8863_PORTn_CTRL0_TAG_REMOVAL                            0x02
#define KSZ8863_PORTn_CTRL0_TXQ_SPLIT_EN                           0x01

//Port N Control 1 register
#define KSZ8863_PORTn_CTRL1_SNIFFER_PORT                           0x80
#define KSZ8863_PORTn_CTRL1_RECEIVE_SNIFF                          0x40
#define KSZ8863_PORTn_CTRL1_TRANSMIT_SNIFF                         0x20
#define KSZ8863_PORTn_CTRL1_DOUBLE_TAG                             0x10
#define KSZ8863_PORTn_CTRL1_USER_PRIO_CEILING                      0x08
#define KSZ8863_PORTn_CTRL1_PORT_VLAN_MEMBERSHIP                   0x07

//Port N Control 2 register
#define KSZ8863_PORTn_CTRL2_TWO_QUEUE_SPLIT_EN                     0x80
#define KSZ8863_PORTn_CTRL2_INGRESS_VLAN_FILT                      0x40
#define KSZ8863_PORTn_CTRL2_DISCARD_NON_PVID_PKT                   0x20
#define KSZ8863_PORTn_CTRL2_FORCE_FLOW_CTRL                        0x10
#define KSZ8863_PORTn_CTRL2_BACK_PRESSURE_EN                       0x08
#define KSZ8863_PORTn_CTRL2_TRANSMIT_EN                            0x04
#define KSZ8863_PORTn_CTRL2_RECEIVE_EN                             0x02
#define KSZ8863_PORTn_CTRL2_LEARNING_DIS                           0x01

//Port N Control 3 register
#define KSZ8863_PORTn_CTRL3_DEFAULT_USER_PRIO                      0xE0
#define KSZ8863_PORTn_CTRL3_DEFAULT_CFI                            0x10
#define KSZ8863_PORTn_CTRL3_DEFAULT_VID_MSB                        0x0F

//Port N Control 4 register
#define KSZ8863_PORTn_CTRL4_DEFAULT_VID_LSB                        0xFF

//Port N Control 5 register
#define KSZ8863_PORTn_CTRL5_PORT3_MII_MODE_SEL                     0x80
#define KSZ8863_PORTn_CTRL5_PORT3_MII_MODE_SEL_PHY                 0x00
#define KSZ8863_PORTn_CTRL5_PORT3_MII_MODE_SEL_MAC                 0x80
#define KSZ8863_PORTn_CTRL5_SELF_ADDR_FILTER_EN_MACA1              0x40
#define KSZ8863_PORTn_CTRL5_SELF_ADDR_FILTER_EN_MACA2              0x20
#define KSZ8863_PORTn_CTRL5_DROP_IG_TAGGED_FRAME                   0x10
#define KSZ8863_PORTn_CTRL5_LIMIT_MODE                             0x0C
#define KSZ8863_PORTn_CTRL5_COUNT_IFG                              0x02
#define KSZ8863_PORTn_CTRL5_COUNT_PRE                              0x01

//Port 3 Q0 Ingress Data Rate Limit register
#define KSZ8863_PORTn_Q0_IG_LIMIT_RMII_REFCLK_INVERT               0x80
#define KSZ8863_PORTn_Q0_IG_LIMIT_Q0_IG_DATA_RATE_LIMIT            0x7F

//Port 3 Q1 Ingress Data Rate Limit register
#define KSZ8863_PORTn_Q1_IG_LIMIT_Q1_IG_DATA_RATE_LIMIT            0x7F

//Port 3 Q2 Ingress Data Rate Limit register
#define KSZ8863_PORTn_Q2_IG_LIMIT_Q2_IG_DATA_RATE_LIMIT            0x7F

//Port 3 Q3 Ingress Data Rate Limit register
#define KSZ8863_PORTn_Q3_IG_LIMIT_Q3_IG_DATA_RATE_LIMIT            0x7F

//Port N PHY Special Control/Status register
#define KSZ8863_PORTn_PSCS_VCT_10M_SHORT                           0x80
#define KSZ8863_PORTn_PSCS_VCT_RESULT                              0x60
#define KSZ8863_PORTn_PSCS_VCT_EN                                  0x10
#define KSZ8863_PORTn_PSCS_FORCE_LNK                               0x08
#define KSZ8863_PORTn_PSCS_REMOTE_LOOPBACK                         0x02
#define KSZ8863_PORTn_PSCS_VCT_FAULT_COUNT_MSB                     0x01

//Port N LinkMD Result register
#define KSZ8863_PORTn_LINKMD_VCT_FAULT_COUNT_LSB                   0xFF

//Port N Control 12 register
#define KSZ8863_PORTn_CTRL12_AN_EN                                 0x80
#define KSZ8863_PORTn_CTRL12_FORCE_SPEED                           0x40
#define KSZ8863_PORTn_CTRL12_FORCE_DUPLEX                          0x20
#define KSZ8863_PORTn_CTRL12_ADV_FLOW_CTRL                         0x10
#define KSZ8863_PORTn_CTRL12_ADV_100BT_FD                          0x08
#define KSZ8863_PORTn_CTRL12_ADV_100BT_HD                          0x04
#define KSZ8863_PORTn_CTRL12_ADV_10BT_FD                           0x02
#define KSZ8863_PORTn_CTRL12_ADV_10BT_HD                           0x01

//Port N Control 13 register
#define KSZ8863_PORTn_CTRL13_LED_OFF                               0x80
#define KSZ8863_PORTn_CTRL13_TX_DIS                                0x40
#define KSZ8863_PORTn_CTRL13_RESTART_AN                            0x20
#define KSZ8863_PORTn_CTRL13_FAR_END_FAULT_DIS                     0x10
#define KSZ8863_PORTn_CTRL13_POWER_DOWN                            0x08
#define KSZ8863_PORTn_CTRL13_AUTO_MDIX_DIS                         0x04
#define KSZ8863_PORTn_CTRL13_FORCE_MDI                             0x02
#define KSZ8863_PORTn_CTRL13_LOOPBACK                              0x01

//Port N Status 0 register
#define KSZ8863_PORTn_STAT0_MDIX_STATUS                            0x80
#define KSZ8863_PORTn_STAT0_AN_DONE                                0x40
#define KSZ8863_PORTn_STAT0_LINK_GOOD                              0x20
#define KSZ8863_PORTn_STAT0_LP_FLOW_CTRL_CAPABLE                   0x10
#define KSZ8863_PORTn_STAT0_LP_100BTX_FD_CAPABLE                   0x08
#define KSZ8863_PORTn_STAT0_LP_100BTX_HF_CAPABLE                   0x04
#define KSZ8863_PORTn_STAT0_LP_10BT_FD_CAPABLE                     0x02
#define KSZ8863_PORTn_STAT0_LP_10BT_HD_CAPABLE                     0x01

//Port N Status 1 register
#define KSZ8863_PORTn_STAT1_HP_MDIX                                0x80
#define KSZ8863_PORTn_STAT1_POLRVS                                 0x20
#define KSZ8863_PORTn_STAT1_TX_FLOW_CTRL_EN                        0x10
#define KSZ8863_PORTn_STAT1_RX_FLOW_CTRL_EN                        0x08
#define KSZ8863_PORTn_STAT1_OP_SPEED                               0x04
#define KSZ8863_PORTn_STAT1_OP_DUPLEX                              0x02
#define KSZ8863_PORTn_STAT1_FAR_END_FAULT                          0x01

//Reset register
#define KSZ8863_RESET_SOFT_RESET                                   0x10
#define KSZ8863_RESET_PCS_RESET                                    0x01

//Indirect Access Control 0 register
#define KSZ8863_INDIRECT_CTRL0_WRITE                               0x00
#define KSZ8863_INDIRECT_CTRL0_READ                                0x10
#define KSZ8863_INDIRECT_CTRL0_TABLE_SEL                           0x0C
#define KSZ8863_INDIRECT_CTRL0_TABLE_SEL_STATIC_MAC                0x00
#define KSZ8863_INDIRECT_CTRL0_TABLE_SEL_VLAN                      0x04
#define KSZ8863_INDIRECT_CTRL0_TABLE_SEL_DYNAMIC_MAC               0x08
#define KSZ8863_INDIRECT_CTRL0_TABLE_SEL_MIB_COUNTER               0x0C
#define KSZ8863_INDIRECT_CTRL0_ADDR_H                              0x03

//Indirect Access Control 1 register
#define KSZ8863_INDIRECT_CTRL1_ADDR_L                              0xFF

//Indirect Data 8 register
#define KSZ8863_INDIRECT_DATA8_CPU_READ_STATUS                     0x80
#define KSZ8863_INDIRECT_DATA8_DATA                                0x07

//KSZ8863 Mode Indicator register
#define KSZ8863_MODE_INDICATOR_FLL                                 0x41
#define KSZ8863_MODE_INDICATOR_MLL                                 0x43
#define KSZ8863_MODE_INDICATOR_RLL                                 0x53

//TXQ Split for Q3 in Port N register
#define KSZ8863_PORTn_Q3_TXQ_SPLIT_PRIORITY_SEL                    0x80

//TXQ Split for Q2 in Port N register
#define KSZ8863_PORTn_Q2_TXQ_SPLIT_PRIORITY_SEL                    0x80

//TXQ Split for Q1 in Port N register
#define KSZ8863_PORTn_Q1_TXQ_SPLIT_PRIORITY_SEL                    0x80

//TXQ Split for Q0 in Port N register
#define KSZ8863_PORTn_Q0_TXQ_SPLIT_PRIORITY_SEL                    0x80

//Interrupt Enable register
#define KSZ8863_INT_EN_P1_OR_P2_LC_INT                             0x80
#define KSZ8863_INT_EN_P3_LC_INT                                   0x04
#define KSZ8863_INT_EN_P2_LC_INT                                   0x02
#define KSZ8863_INT_EN_P1_LC_INT                                   0x01

//Link Change Interrupt register
#define KSZ8863_LINK_CHANGE_INT_P1_OR_P2_LC_INT                    0x80
#define KSZ8863_LINK_CHANGE_INT_P3_LC_INT                          0x04
#define KSZ8863_LINK_CHANGE_INT_P2_LC_INT                          0x02
#define KSZ8863_LINK_CHANGE_INT_P1_LC_INT                          0x01

//Fiber Signal Threshold register
#define KSZ8863_FIBER_SIGNAL_THRESHOLD_P2_THRESHOLD                0x80
#define KSZ8863_FIBER_SIGNAL_THRESHOLD_P1_THRESHOLD                0x40

//Internal 1.8V LDO Control register
#define KSZ8863_INTERNAL_LDO_CTRL_INTERNAL_LDO_DIS                 0x40

//Insert SRC PVID register
#define KSZ8863_INSERT_SRC_PVID_INSERT_SRC_P1_PVID_AT_P2           0x20
#define KSZ8863_INSERT_SRC_PVID_INSERT_SRC_P1_PVID_AT_P3           0x10
#define KSZ8863_INSERT_SRC_PVID_INSERT_SRC_P2_PVID_AT_P1           0x08
#define KSZ8863_INSERT_SRC_PVID_INSERT_SRC_P2_PVID_AT_P3           0x04
#define KSZ8863_INSERT_SRC_PVID_INSERT_SRC_P3_PVID_AT_P1           0x02
#define KSZ8863_INSERT_SRC_PVID_INSERT_SRC_P3_PVID_AT_P2           0x01

//Power Management and LED Mode register
#define KSZ8863_PWR_MGMT_LED_MODE_CPU_IF_POWER_DOWN                0x80
#define KSZ8863_PWR_MGMT_LED_MODE_SWITCH_POWER_DOWN                0x40
#define KSZ8863_PWR_MGMT_LED_MODE_LED_MODE                         0x30
#define KSZ8863_PWR_MGMT_LED_MODE_LED_MODE_LED0_LNK_ACT_LED1_SPD   0x00
#define KSZ8863_PWR_MGMT_LED_MODE_LED_MODE_LED0_LNK_LED1_ACT       0x10
#define KSZ8863_PWR_MGMT_LED_MODE_LED_MODE_LED0_LNK_ACT_LED1_DPLX  0x20
#define KSZ8863_PWR_MGMT_LED_MODE_LED_MODE_LED0_LNK_LED1_DPLX      0x30
#define KSZ8863_PWR_MGMT_LED_MODE_LED_OUT_MODE                     0x08
#define KSZ8863_PWR_MGMT_LED_MODE_PLL_OFF_EN                       0x04
#define KSZ8863_PWR_MGMT_LED_MODE_POWER_MGMT_MODE                  0x03

//Forward Invalid VID Frame and Host Mode register
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_FWD_INVALID_VID_FRAME    0x70
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_P3_RMII_CLK_SEL          0x08
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_P3_RMII_CLK_SEL_EXTERNAL 0x00
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_P3_RMII_CLK_SEL_INTERNAL 0x08
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_P1_RMII_CLK_SEL          0x04
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_P1_RMII_CLK_SEL_EXTERNAL 0x00
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_P1_RMII_CLK_SEL_INTERNAL 0x04
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_HOST_MODE                0x03
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_HOST_MODE_I2C_MASTER     0x00
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_HOST_MODE_I2C_SLAVE      0x01
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_HOST_MODE_SPI_SLAVE      0x02
#define KSZ8863_FWD_INVALID_VID_HOST_MODE_HOST_MODE_SMI            0x03

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief Static MAC table entry
 **/

typedef struct
{
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t reserved : 6;     //0
   uint8_t fidH : 2;
   uint8_t fidL : 2;         //1
   uint8_t useFid : 1;
   uint8_t override : 1;
   uint8_t valid : 1;
   uint8_t forwardPorts : 3;
#else
   uint8_t fidH : 2;         //0
   uint8_t reserved : 6;
   uint8_t forwardPorts : 3; //1
   uint8_t valid : 1;
   uint8_t override : 1;
   uint8_t useFid : 1;
   uint8_t fidL : 2;
#endif
   MacAddr macAddr;          //2-7
} Ksz8863StaticMacEntry;


/**
 * @brief Dynamic MAC table entry
 **/

typedef struct
{
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t dataNotReady : 1;     //0
   uint8_t reserved : 4;
   uint8_t macEmpty : 1;
   uint8_t numValidEntriesH : 2;
   uint8_t numValidEntriesL;     //1
   uint8_t timestamp : 2;        //2
   uint8_t sourcePort : 2;
   uint8_t fid : 4;
#else
   uint8_t numValidEntriesH : 2; //0
   uint8_t macEmpty : 1;
   uint8_t reserved : 4;
   uint8_t dataNotReady : 1;
   uint8_t numValidEntriesL;     //1
   uint8_t fid : 4;              //2
   uint8_t sourcePort : 2;
   uint8_t timestamp : 2;
#endif
   MacAddr macAddr;              //3-8
} Ksz8863DynamicMacEntry;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif

//KSZ8863 Ethernet switch driver
extern const SwitchDriver ksz8863SwitchDriver;

//KSZ8863 related functions
error_t ksz8863Init(NetInterface *interface);

void ksz8863Tick(NetInterface *interface);

void ksz8863EnableIrq(NetInterface *interface);
void ksz8863DisableIrq(NetInterface *interface);

void ksz8863EventHandler(NetInterface *interface);

error_t ksz8863TagFrame(NetInterface *interface, NetBuffer *buffer,
   size_t *offset, NetTxAncillary *ancillary);

error_t ksz8863UntagFrame(NetInterface *interface, uint8_t **frame,
   size_t *length, NetRxAncillary *ancillary);

bool_t ksz8863GetLinkState(NetInterface *interface, uint8_t port);
uint32_t ksz8863GetLinkSpeed(NetInterface *interface, uint8_t port);
NicDuplexMode ksz8863GetDuplexMode(NetInterface *interface, uint8_t port);

void ksz8863SetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state);

SwitchPortState ksz8863GetPortState(NetInterface *interface, uint8_t port);

void ksz8863SetAgingTime(NetInterface *interface, uint32_t agingTime);

void ksz8863EnableIgmpSnooping(NetInterface *interface, bool_t enable);
void ksz8863EnableMldSnooping(NetInterface *interface, bool_t enable);
void ksz8863EnableRsvdMcastTable(NetInterface *interface, bool_t enable);

error_t ksz8863AddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t ksz8863DeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t ksz8863GetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void ksz8863FlushStaticFdbTable(NetInterface *interface);

error_t ksz8863GetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void ksz8863FlushDynamicFdbTable(NetInterface *interface, uint8_t port);

void ksz8863SetUnknownMcastFwdPorts(NetInterface *interface,
   bool_t enable, uint32_t forwardPorts);

void ksz8863WritePhyReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data);

uint16_t ksz8863ReadPhyReg(NetInterface *interface, uint8_t port,
   uint8_t address);

void ksz8863DumpPhyReg(NetInterface *interface, uint8_t port);

void ksz8863WriteSwitchReg(NetInterface *interface, uint8_t address,
   uint8_t data);

uint8_t ksz8863ReadSwitchReg(NetInterface *interface, uint8_t address);

void ksz8863DumpSwitchReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
