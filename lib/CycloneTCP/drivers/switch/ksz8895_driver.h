/**
 * @file ksz8895_driver.h
 * @brief KSZ8895 5-port Ethernet switch driver
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

#ifndef _KSZ8895_DRIVER_H
#define _KSZ8895_DRIVER_H

//Dependencies
#include "core/nic.h"

//Port identifiers
#define KSZ8895_PORT1 1
#define KSZ8895_PORT2 2
#define KSZ8895_PORT3 3
#define KSZ8895_PORT4 4
#define KSZ8895_PORT5 5

//Port masks
#define KSZ8895_PORT_MASK  0x1F
#define KSZ8895_PORT1_MASK 0x01
#define KSZ8895_PORT2_MASK 0x02
#define KSZ8895_PORT3_MASK 0x04
#define KSZ8895_PORT4_MASK 0x08
#define KSZ8895_PORT5_MASK 0x10

//SPI command byte
#define KSZ8895_SPI_CMD_WRITE 0x02
#define KSZ8895_SPI_CMD_READ  0x03

//Size of static and dynamic MAC tables
#define KSZ8895_STATIC_MAC_TABLE_SIZE  32
#define KSZ8895_DYNAMIC_MAC_TABLE_SIZE 1024
#define KSZ8895_VLAN_TABLE_SIZE        4096

//Tail tag rules (host to KSZ8895)
#define KSZ8895_TAIL_TAG_NORMAL_ADDR_LOOKUP 0x80
#define KSZ8895_TAIL_TAG_PORT_SEL           0x40
#define KSZ8895_TAIL_TAG_DEST_QUEUE         0x30
#define KSZ8895_TAIL_TAG_DEST_PORT4         0x08
#define KSZ8895_TAIL_TAG_DEST_PORT3         0x04
#define KSZ8895_TAIL_TAG_DEST_PORT2         0x02
#define KSZ8895_TAIL_TAG_DEST_PORT1         0x01

//Tail tag rules (KSZ8895 to host)
#define KSZ8895_TAIL_TAG_SRC_PORT           0x03

//KSZ8895 PHY registers
#define KSZ8895_BMCR                                         0x00
#define KSZ8895_BMSR                                         0x01
#define KSZ8895_PHYID1                                       0x02
#define KSZ8895_PHYID2                                       0x03
#define KSZ8895_ANAR                                         0x04
#define KSZ8895_ANLPAR                                       0x05
#define KSZ8895_LINKMD                                       0x1D
#define KSZ8895_PHYSCS                                       0x1F

//KSZ8895 Switch registers
#define KSZ8895_CHIP_ID0                                     0x00
#define KSZ8895_CHIP_ID1                                     0x01
#define KSZ8895_GLOBAL_CTRL0                                 0x02
#define KSZ8895_GLOBAL_CTRL1                                 0x03
#define KSZ8895_GLOBAL_CTRL2                                 0x04
#define KSZ8895_GLOBAL_CTRL3                                 0x05
#define KSZ8895_GLOBAL_CTRL4                                 0x06
#define KSZ8895_GLOBAL_CTRL5                                 0x07
#define KSZ8895_GLOBAL_CTRL6                                 0x08
#define KSZ8895_GLOBAL_CTRL7                                 0x09
#define KSZ8895_GLOBAL_CTRL8                                 0x0A
#define KSZ8895_GLOBAL_CTRL9                                 0x0B
#define KSZ8895_GLOBAL_CTRL10                                0x0C
#define KSZ8895_GLOBAL_CTRL11                                0x0D
#define KSZ8895_PD_MGMT_CTRL1                                0x0E
#define KSZ8895_PD_MGMT_CTRL2                                0x0F
#define KSZ8895_PORT1_CTRL0                                  0x10
#define KSZ8895_PORT1_CTRL1                                  0x11
#define KSZ8895_PORT1_CTRL2                                  0x12
#define KSZ8895_PORT1_CTRL3                                  0x13
#define KSZ8895_PORT1_CTRL4                                  0x14
#define KSZ8895_PORT1_STAT0                                  0x19
#define KSZ8895_PORT1_PSCS                                   0x1A
#define KSZ8895_PORT1_LINKMD                                 0x1B
#define KSZ8895_PORT1_CTRL5                                  0x1C
#define KSZ8895_PORT1_CTRL6                                  0x1D
#define KSZ8895_PORT1_STAT1                                  0x1E
#define KSZ8895_PORT1_CTRL7_STAT2                            0x1F
#define KSZ8895_PORT2_CTRL0                                  0x20
#define KSZ8895_PORT2_CTRL1                                  0x21
#define KSZ8895_PORT2_CTRL2                                  0x22
#define KSZ8895_PORT2_CTRL3                                  0x23
#define KSZ8895_PORT2_CTRL4                                  0x24
#define KSZ8895_PORT2_STAT0                                  0x29
#define KSZ8895_PORT2_PSCS                                   0x2A
#define KSZ8895_PORT2_LINKMD                                 0x2B
#define KSZ8895_PORT2_CTRL5                                  0x2C
#define KSZ8895_PORT2_CTRL6                                  0x2D
#define KSZ8895_PORT2_STAT1                                  0x2E
#define KSZ8895_PORT2_CTRL7_STAT2                            0x2F
#define KSZ8895_PORT3_CTRL0                                  0x30
#define KSZ8895_PORT3_CTRL1                                  0x31
#define KSZ8895_PORT3_CTRL2                                  0x32
#define KSZ8895_PORT3_CTRL3                                  0x33
#define KSZ8895_PORT3_CTRL4                                  0x34
#define KSZ8895_PORT3_STAT0                                  0x39
#define KSZ8895_PORT3_PSCS                                   0x3A
#define KSZ8895_PORT3_LINKMD                                 0x3B
#define KSZ8895_PORT3_CTRL5                                  0x3C
#define KSZ8895_PORT3_CTRL6                                  0x3D
#define KSZ8895_PORT3_STAT1                                  0x3E
#define KSZ8895_PORT3_CTRL7_STAT2                            0x3F
#define KSZ8895_PORT4_CTRL0                                  0x40
#define KSZ8895_PORT4_CTRL1                                  0x41
#define KSZ8895_PORT4_CTRL2                                  0x42
#define KSZ8895_PORT4_CTRL3                                  0x43
#define KSZ8895_PORT4_CTRL4                                  0x44
#define KSZ8895_PORT4_STAT0                                  0x49
#define KSZ8895_PORT4_PSCS                                   0x4A
#define KSZ8895_PORT4_LINKMD                                 0x4B
#define KSZ8895_PORT4_CTRL5                                  0x4C
#define KSZ8895_PORT4_CTRL6                                  0x4D
#define KSZ8895_PORT4_STAT1                                  0x4E
#define KSZ8895_PORT4_CTRL7_STAT2                            0x4F
#define KSZ8895_PORT5_CTRL0                                  0x50
#define KSZ8895_PORT5_CTRL1                                  0x51
#define KSZ8895_PORT5_CTRL2                                  0x52
#define KSZ8895_PORT5_CTRL3                                  0x53
#define KSZ8895_PORT5_CTRL4                                  0x54
#define KSZ8895_RMII_MGMT_CTRL                               0x57
#define KSZ8895_PORT5_STAT0                                  0x59
#define KSZ8895_PORT5_PSCS                                   0x5A
#define KSZ8895_PORT5_LINKMD                                 0x5B
#define KSZ8895_PORT5_CTRL5                                  0x5C
#define KSZ8895_PORT5_CTRL6                                  0x5D
#define KSZ8895_PORT5_STAT1                                  0x5E
#define KSZ8895_PORT5_CTRL7_STAT2                            0x5F
#define KSZ8895_MAC_ADDR0                                    0x68
#define KSZ8895_MAC_ADDR1                                    0x69
#define KSZ8895_MAC_ADDR2                                    0x6A
#define KSZ8895_MAC_ADDR3                                    0x6B
#define KSZ8895_MAC_ADDR4                                    0x6C
#define KSZ8895_MAC_ADDR5                                    0x6D
#define KSZ8895_INDIRECT_CTRL0                               0x6E
#define KSZ8895_INDIRECT_CTRL1                               0x6F
#define KSZ8895_INDIRECT_DATA8                               0x70
#define KSZ8895_INDIRECT_DATA7                               0x71
#define KSZ8895_INDIRECT_DATA6                               0x72
#define KSZ8895_INDIRECT_DATA5                               0x73
#define KSZ8895_INDIRECT_DATA4                               0x74
#define KSZ8895_INDIRECT_DATA3                               0x75
#define KSZ8895_INDIRECT_DATA2                               0x76
#define KSZ8895_INDIRECT_DATA1                               0x77
#define KSZ8895_INDIRECT_DATA0                               0x78
#define KSZ8895_INT_STAT                                     0x7C
#define KSZ8895_INT_MASK                                     0x7D
#define KSZ8895_GLOBAL_CTRL12                                0x80
#define KSZ8895_GLOBAL_CTRL13                                0x81
#define KSZ8895_GLOBAL_CTRL14                                0x82
#define KSZ8895_GLOBAL_CTRL15                                0x83
#define KSZ8895_GLOBAL_CTRL16                                0x84
#define KSZ8895_GLOBAL_CTRL17                                0x85
#define KSZ8895_GLOBAL_CTRL18                                0x86
#define KSZ8895_GLOBAL_CTRL19                                0x87
#define KSZ8895_ID                                           0x89
#define KSZ8895_TOS_PRIO_CTRL0                               0x90
#define KSZ8895_TOS_PRIO_CTRL1                               0x91
#define KSZ8895_TOS_PRIO_CTRL2                               0x92
#define KSZ8895_TOS_PRIO_CTRL3                               0x93
#define KSZ8895_TOS_PRIO_CTRL4                               0x94
#define KSZ8895_TOS_PRIO_CTRL5                               0x95
#define KSZ8895_TOS_PRIO_CTRL6                               0x96
#define KSZ8895_TOS_PRIO_CTRL7                               0x97
#define KSZ8895_TOS_PRIO_CTRL8                               0x98
#define KSZ8895_TOS_PRIO_CTRL9                               0x99
#define KSZ8895_TOS_PRIO_CTRL10                              0x9A
#define KSZ8895_TOS_PRIO_CTRL11                              0x9B
#define KSZ8895_TOS_PRIO_CTRL12                              0x9C
#define KSZ8895_TOS_PRIO_CTRL13                              0x9D
#define KSZ8895_TOS_PRIO_CTRL14                              0x9E
#define KSZ8895_TOS_PRIO_CTRL15                              0x9F
#define KSZ8895_PORT1_CTRL8                                  0xB0
#define KSZ8895_PORT1_CTRL9                                  0xB1
#define KSZ8895_PORT1_CTRL10                                 0xB2
#define KSZ8895_PORT1_CTRL11                                 0xB3
#define KSZ8895_PORT1_CTRL12                                 0xB4
#define KSZ8895_PORT1_CTRL13                                 0xB5
#define KSZ8895_PORT1_RATE_LIMIT_CTRL                        0xB6
#define KSZ8895_PORT1_PRIO0_IG_LIMIT_CTRL1                   0xB7
#define KSZ8895_PORT1_PRIO1_IG_LIMIT_CTRL2                   0xB8
#define KSZ8895_PORT1_PRIO2_IG_LIMIT_CTRL3                   0xB9
#define KSZ8895_PORT1_PRIO3_IG_LIMIT_CTRL4                   0xBA
#define KSZ8895_PORT1_QUEUE0_EG_LIMIT_CTRL1                  0xBB
#define KSZ8895_PORT1_QUEUE1_EG_LIMIT_CTRL2                  0xBC
#define KSZ8895_PORT1_QUEUE2_EG_LIMIT_CTRL3                  0xBD
#define KSZ8895_PORT1_QUEUE3_EG_LIMIT_CTRL4                  0xBE
#define KSZ8895_TEST1                                        0xBF
#define KSZ8895_PORT2_CTRL8                                  0xC0
#define KSZ8895_PORT2_CTRL9                                  0xC1
#define KSZ8895_PORT2_CTRL10                                 0xC2
#define KSZ8895_PORT2_CTRL11                                 0xC3
#define KSZ8895_PORT2_CTRL12                                 0xC4
#define KSZ8895_PORT2_CTRL13                                 0xC5
#define KSZ8895_PORT2_RATE_LIMIT_CTRL                        0xC6
#define KSZ8895_PORT2_PRIO0_IG_LIMIT_CTRL1                   0xC7
#define KSZ8895_PORT2_PRIO1_IG_LIMIT_CTRL2                   0xC8
#define KSZ8895_PORT2_PRIO2_IG_LIMIT_CTRL3                   0xC9
#define KSZ8895_PORT2_PRIO3_IG_LIMIT_CTRL4                   0xCA
#define KSZ8895_PORT2_QUEUE0_EG_LIMIT_CTRL1                  0xCB
#define KSZ8895_PORT2_QUEUE1_EG_LIMIT_CTRL2                  0xCC
#define KSZ8895_PORT2_QUEUE2_EG_LIMIT_CTRL3                  0xCD
#define KSZ8895_PORT2_QUEUE3_EG_LIMIT_CTRL4                  0xCE
#define KSZ8895_PORT3_CTRL8                                  0xD0
#define KSZ8895_PORT3_CTRL9                                  0xD1
#define KSZ8895_PORT3_CTRL10                                 0xD2
#define KSZ8895_PORT3_CTRL11                                 0xD3
#define KSZ8895_PORT3_CTRL12                                 0xD4
#define KSZ8895_PORT3_CTRL13                                 0xD5
#define KSZ8895_PORT3_RATE_LIMIT_CTRL                        0xD6
#define KSZ8895_PORT3_PRIO0_IG_LIMIT_CTRL1                   0xD7
#define KSZ8895_PORT3_PRIO1_IG_LIMIT_CTRL2                   0xD8
#define KSZ8895_PORT3_PRIO2_IG_LIMIT_CTRL3                   0xD9
#define KSZ8895_PORT3_PRIO3_IG_LIMIT_CTRL4                   0xDA
#define KSZ8895_PORT3_QUEUE0_EG_LIMIT_CTRL1                  0xDB
#define KSZ8895_PORT3_QUEUE1_EG_LIMIT_CTRL2                  0xDC
#define KSZ8895_PORT3_QUEUE2_EG_LIMIT_CTRL3                  0xDD
#define KSZ8895_PORT3_QUEUE3_EG_LIMIT_CTRL4                  0xDE
#define KSZ8895_TEST2                                        0xDF
#define KSZ8895_PORT4_CTRL8                                  0xE0
#define KSZ8895_PORT4_CTRL9                                  0xE1
#define KSZ8895_PORT4_CTRL10                                 0xE2
#define KSZ8895_PORT4_CTRL11                                 0xE3
#define KSZ8895_PORT4_CTRL12                                 0xE4
#define KSZ8895_PORT4_CTRL13                                 0xE5
#define KSZ8895_PORT4_RATE_LIMIT_CTRL                        0xE6
#define KSZ8895_PORT4_PRIO0_IG_LIMIT_CTRL1                   0xE7
#define KSZ8895_PORT4_PRIO1_IG_LIMIT_CTRL2                   0xE8
#define KSZ8895_PORT4_PRIO2_IG_LIMIT_CTRL3                   0xE9
#define KSZ8895_PORT4_PRIO3_IG_LIMIT_CTRL4                   0xEA
#define KSZ8895_PORT4_QUEUE0_EG_LIMIT_CTRL1                  0xEB
#define KSZ8895_PORT4_QUEUE1_EG_LIMIT_CTRL2                  0xEC
#define KSZ8895_PORT4_QUEUE2_EG_LIMIT_CTRL3                  0xED
#define KSZ8895_PORT4_QUEUE3_EG_LIMIT_CTRL4                  0xEE
#define KSZ8895_PORT3_COPPER_FIBER_CTRL                      0xEF
#define KSZ8895_PORT5_CTRL8                                  0xF0
#define KSZ8895_PORT5_CTRL9                                  0xF1
#define KSZ8895_PORT5_CTRL10                                 0xF2
#define KSZ8895_PORT5_CTRL11                                 0xF3
#define KSZ8895_PORT5_CTRL12                                 0xF4
#define KSZ8895_PORT5_CTRL13                                 0xF5
#define KSZ8895_PORT5_RATE_LIMIT_CTRL                        0xF6
#define KSZ8895_PORT5_PRIO0_IG_LIMIT_CTRL1                   0xF7
#define KSZ8895_PORT5_PRIO1_IG_LIMIT_CTRL2                   0xF8
#define KSZ8895_PORT5_PRIO2_IG_LIMIT_CTRL3                   0xF9
#define KSZ8895_PORT5_PRIO3_IG_LIMIT_CTRL4                   0xFA
#define KSZ8895_PORT5_QUEUE0_EG_LIMIT_CTRL1                  0xFB
#define KSZ8895_PORT5_QUEUE1_EG_LIMIT_CTRL2                  0xFC
#define KSZ8895_PORT5_QUEUE2_EG_LIMIT_CTRL3                  0xFD
#define KSZ8895_PORT5_QUEUE3_EG_LIMIT_CTRL4                  0xFE
#define KSZ8895_TEST3                                        0xFF

//KSZ8895 Switch register access macros
#define KSZ8895_PORTn_CTRL0(port)                            (0x00 + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL1(port)                            (0x01 + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL2(port)                            (0x02 + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL3(port)                            (0x03 + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL4(port)                            (0x04 + ((port) * 0x10))
#define KSZ8895_PORTn_STAT0(port)                            (0x09 + ((port) * 0x10))
#define KSZ8895_PORTn_PSCS(port)                             (0x0A + ((port) * 0x10))
#define KSZ8895_PORTn_LINKMD(port)                           (0x0B + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL5(port)                            (0x0C + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL6(port)                            (0x0D + ((port) * 0x10))
#define KSZ8895_PORTn_STAT1(port)                            (0x0E + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL7_STAT2(port)                      (0x0F + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL8(port)                            (0xA0 + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL9(port)                            (0xA1 + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL10(port)                           (0xA2 + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL11(port)                           (0xA3 + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL12(port)                           (0xA4 + ((port) * 0x10))
#define KSZ8895_PORTn_CTRL13(port)                           (0xA5 + ((port) * 0x10))
#define KSZ8895_PORTn_RATE_LIMIT_CTRL(port)                  (0xA6 + ((port) * 0x10))
#define KSZ8895_PORTn_PRIO0_IG_LIMIT_CTRL1(port)             (0xA7 + ((port) * 0x10))
#define KSZ8895_PORTn_PRIO1_IG_LIMIT_CTRL2(port)             (0xA8 + ((port) * 0x10))
#define KSZ8895_PORTn_PRIO2_IG_LIMIT_CTRL3(port)             (0xA9 + ((port) * 0x10))
#define KSZ8895_PORTn_PRIO3_IG_LIMIT_CTRL4(port)             (0xAA + ((port) * 0x10))
#define KSZ8895_PORTn_QUEUE0_EG_LIMIT_CTRL1(port)            (0xAB + ((port) * 0x10))
#define KSZ8895_PORTn_QUEUE1_EG_LIMIT_CTRL2(port)            (0xAC + ((port) * 0x10))
#define KSZ8895_PORTn_QUEUE2_EG_LIMIT_CTRL3(port)            (0xAD + ((port) * 0x10))
#define KSZ8895_PORTn_QUEUE3_EG_LIMIT_CTRL4(port)            (0xAE + ((port) * 0x10))

//MII Control register
#define KSZ8895_BMCR_RESET                                   0x8000
#define KSZ8895_BMCR_LOOPBACK                                0x4000
#define KSZ8895_BMCR_FORCE_100                               0x2000
#define KSZ8895_BMCR_AN_EN                                   0x1000
#define KSZ8895_BMCR_POWER_DOWN                              0x0800
#define KSZ8895_BMCR_ISOLATE                                 0x0400
#define KSZ8895_BMCR_RESTART_AN                              0x0200
#define KSZ8895_BMCR_FORCE_FULL_DUPLEX                       0x0100
#define KSZ8895_BMCR_COL_TEST                                0x0080
#define KSZ8895_BMCR_HP_MDIX                                 0x0020
#define KSZ8895_BMCR_FORCE_MDI                               0x0010
#define KSZ8895_BMCR_AUTO_MDIX_DIS                           0x0008
#define KSZ8895_BMCR_FAR_END_FAULT_DIS                       0x0004
#define KSZ8895_BMCR_TRANSMIT_DIS                            0x0002
#define KSZ8895_BMCR_LED_DIS                                 0x0001

//MII Status register
#define KSZ8895_BMSR_100BT4                                  0x8000
#define KSZ8895_BMSR_100BTX_FD                               0x4000
#define KSZ8895_BMSR_100BTX_HD                               0x2000
#define KSZ8895_BMSR_10BT_FD                                 0x1000
#define KSZ8895_BMSR_10BT_HD                                 0x0800
#define KSZ8895_BMSR_PREAMBLE_SUPPR                          0x0040
#define KSZ8895_BMSR_AN_COMPLETE                             0x0020
#define KSZ8895_BMSR_FAR_END_FAULT                           0x0010
#define KSZ8895_BMSR_AN_CAPABLE                              0x0008
#define KSZ8895_BMSR_LINK_STATUS                             0x0004
#define KSZ8895_BMSR_JABBER_TEST                             0x0002
#define KSZ8895_BMSR_EXTENDED_CAPABLE                        0x0001

//PHYID High register
#define KSZ8895_PHYID1_DEFAULT                               0x0022

//PHYID Low register
#define KSZ8895_PHYID2_DEFAULT                               0x1450

//Advertisement Ability register
#define KSZ8895_ANAR_NEXT_PAGE                               0x8000
#define KSZ8895_ANAR_REMOTE_FAULT                            0x2000
#define KSZ8895_ANAR_PAUSE                                   0x0400
#define KSZ8895_ANAR_100BTX_FD                               0x0100
#define KSZ8895_ANAR_100BTX_HD                               0x0080
#define KSZ8895_ANAR_10BT_FD                                 0x0040
#define KSZ8895_ANAR_10BT_HD                                 0x0020
#define KSZ8895_ANAR_SELECTOR                                0x001F
#define KSZ8895_ANAR_SELECTOR_DEFAULT                        0x0001

//Link Partner Ability register
#define KSZ8895_ANLPAR_NEXT_PAGE                             0x8000
#define KSZ8895_ANLPAR_LP_ACK                                0x4000
#define KSZ8895_ANLPAR_REMOTE_FAULT                          0x2000
#define KSZ8895_ANLPAR_PAUSE                                 0x0400
#define KSZ8895_ANLPAR_100BTX_FD                             0x0100
#define KSZ8895_ANLPAR_100BTX_HD                             0x0080
#define KSZ8895_ANLPAR_10BT_FD                               0x0040
#define KSZ8895_ANLPAR_10BT_HD                               0x0020

//LinkMD Control/Status register
#define KSZ8895_LINKMD_TEST_EN                               0x8000
#define KSZ8895_LINKMD_RESULT                                0x6000
#define KSZ8895_LINKMD_SHORT                                 0x1000
#define KSZ8895_LINKMD_FAULT_COUNT                           0x01FF

//PHY Special Control/Status register
#define KSZ8895_PHYSCS_OP_MODE                               0x0700
#define KSZ8895_PHYSCS_OP_MODE_AN                            0x0100
#define KSZ8895_PHYSCS_OP_MODE_10BT_HD                       0x0200
#define KSZ8895_PHYSCS_OP_MODE_100BTX_HD                     0x0300
#define KSZ8895_PHYSCS_OP_MODE_10BT_FD                       0x0500
#define KSZ8895_PHYSCS_OP_MODE_100BTX_FD                     0x0600
#define KSZ8895_PHYSCS_OP_MODE_ISOLATE                       0x0700
#define KSZ8895_PHYSCS_POLRVS                                0x0020
#define KSZ8895_PHYSCS_MDIX_STATUS                           0x0010
#define KSZ8895_PHYSCS_FORCE_LINK                            0x0008
#define KSZ8895_PHYSCS_PWRSAVE                               0x0004
#define KSZ8895_PHYSCS_REMOTE_LOOPBACK                       0x0002

//Chip ID0 register
#define KSZ8895_CHIP_ID0_FAMILY_ID                           0xFF
#define KSZ8895_CHIP_ID0_FAMILY_ID_DEFAULT                   0x95

//Chip ID1 / Start Switch register
#define KSZ8895_CHIP_ID1_CHIP_ID                             0xF0
#define KSZ8895_CHIP_ID1_CHIP_ID_MQX_FQX_MLX                 0x40
#define KSZ8895_CHIP_ID1_CHIP_ID_RQX                         0x60
#define KSZ8895_CHIP_ID1_REVISION_ID                         0x0E
#define KSZ8895_CHIP_ID1_START_SWITCH                        0x01

//Global Control 0 register
#define KSZ8895_GLOBAL_CTRL0_NEW_BACK_OFF_EN                 0x80
#define KSZ8895_GLOBAL_CTRL0_FLUSH_DYNAMIC_MAC_TABLE         0x20
#define KSZ8895_GLOBAL_CTRL0_FLUSH_STATIC_MAC_TABLE          0x10
#define KSZ8895_GLOBAL_CTRL0_PHY_MII_RMII_EN                 0x08
#define KSZ8895_GLOBAL_CTRL0_UNH_MODE                        0x02
#define KSZ8895_GLOBAL_CTRL0_LINK_CHANGE_AGE                 0x01

//Global Control 1 register
#define KSZ8895_GLOBAL_CTRL1_PASS_ALL_FRAMES                 0x80
#define KSZ8895_GLOBAL_CTRL1_2KB_PKT_SUPPORT                 0x40
#define KSZ8895_GLOBAL_CTRL1_TX_FLOW_CTRL_DIS                0x20
#define KSZ8895_GLOBAL_CTRL1_RX_FLOW_CTRL_DIS                0x10
#define KSZ8895_GLOBAL_CTRL1_FRAME_LEN_CHECK_EN              0x08
#define KSZ8895_GLOBAL_CTRL1_AGING_EN                        0x04
#define KSZ8895_GLOBAL_CTRL1_FAST_AGE_EN                     0x02
#define KSZ8895_GLOBAL_CTRL1_AGGRESSIVE_BACK_OFF_EN          0x01

//Global Control 2 register
#define KSZ8895_GLOBAL_CTRL2_UNI_VLAN_MISMATCH_DISCARD       0x80
#define KSZ8895_GLOBAL_CTRL2_MCAST_STORM_PROTECT_DIS         0x40
#define KSZ8895_GLOBAL_CTRL2_BACK_PRESSURE_MODE              0x20
#define KSZ8895_GLOBAL_CTRL2_FLOW_CTRL_FAIR_MODE             0x10
#define KSZ8895_GLOBAL_CTRL2_NO_EXCESSIVE_COL_DROP           0x08
#define KSZ8895_GLOBAL_CTRL2_HUGE_PACKET_SUPPORT             0x04
#define KSZ8895_GLOBAL_CTRL2_MAX_PACKET_SIZE_CHECK_DIS       0x02

//Global Control 3 register
#define KSZ8895_GLOBAL_CTRL3_VLAN_EN                         0x80
#define KSZ8895_GLOBAL_CTRL3_SW5_IGMP_SNOOP_EN               0x40
#define KSZ8895_GLOBAL_CTRL3_SW5_DIRECT_MODE_EN              0x20
#define KSZ8895_GLOBAL_CTRL3_SW5_PRE_TAG_EN                  0x10
#define KSZ8895_GLOBAL_CTRL3_TAG_MASK_EN                     0x02
#define KSZ8895_GLOBAL_CTRL3_SNIFF_MODE_SEL                  0x01

//Global Control 4 register
#define KSZ8895_GLOBAL_CTRL4_SW5_BACK_PRESSURE_EN            0x80
#define KSZ8895_GLOBAL_CTRL4_SW5_HALF_DUPLEX_MODE            0x40
#define KSZ8895_GLOBAL_CTRL4_SW5_FLOW_CTRL_EN                0x20
#define KSZ8895_GLOBAL_CTRL4_SW5_SPEED                       0x10
#define KSZ8895_GLOBAL_CTRL4_NULL_VID_REPLACEMENT            0x08
#define KSZ8895_GLOBAL_CTRL4_BCAST_STORM_PROTECT_RATE_MSB    0x07

//Global Control 5 register
#define KSZ8895_GLOBAL_CTRL5_BCAST_STORM_PROTECT_RATE_LSB    0xFF

//Global Control 6 register
#define KSZ8895_GLOBAL_CTRL6_FACTORY_TESTING                 0xFF

//Global Control 7 register
#define KSZ8895_GLOBAL_CTRL7_FACTORY_TESTING                 0xFF

//Global Control 8 register
#define KSZ8895_GLOBAL_CTRL8_FACTORY_TESTING                 0xFF

//Global Control 9 register
#define KSZ8895_GLOBAL_CTRL9_SW5_REFCLK_EDGE_SEL             0x40
#define KSZ8895_GLOBAL_CTRL9_SW5_REFCLK_EDGE_SEL_RISING      0x00
#define KSZ8895_GLOBAL_CTRL9_SW5_REFCLK_EDGE_SEL_FALLING     0x40
#define KSZ8895_GLOBAL_CTRL9_PHY_PWR_SAVE                    0x08
#define KSZ8895_GLOBAL_CTRL9_LED_MODE                        0x02
#define KSZ8895_GLOBAL_CTRL9_SPI_READ_CLK_EDGE_SEL           0x01
#define KSZ8895_GLOBAL_CTRL9_SPI_READ_CLK_EDGE_SEL_FALLING   0x00
#define KSZ8895_GLOBAL_CTRL9_SPI_READ_CLK_EDGE_SEL_RISING    0x01

//Global Control 10 register
#define KSZ8895_GLOBAL_CTRL10_CLK_MODE                       0x40
#define KSZ8895_GLOBAL_CTRL10_CPU_CLK_SEL                    0x30
#define KSZ8895_GLOBAL_CTRL10_RESTORE_PREAMBLE_EN            0x04
#define KSZ8895_GLOBAL_CTRL10_TAIL_TAG_EN                    0x02
#define KSZ8895_GLOBAL_CTRL10_PASS_FLOW_CTRL_PKT             0x01

//Global Control 11 register
#define KSZ8895_GLOBAL_CTRL11_FACTORY_TESTING                0xFF

//Power-Down Management Control 1 register
#define KSZ8895_PD_MGMT_CTRL1_PLL_PWR_DOWN                   0x20
#define KSZ8895_PD_MGMT_CTRL1_PWR_MGMT_MODE                  0x18
#define KSZ8895_PD_MGMT_CTRL1_PWR_MGMT_MODE_NORMAL           0x00
#define KSZ8895_PD_MGMT_CTRL1_PWR_MGMT_MODE_ENERGY_DETECT    0x08
#define KSZ8895_PD_MGMT_CTRL1_PWR_MGMT_MODE_SOFT_PWR_DOWN    0x10
#define KSZ8895_PD_MGMT_CTRL1_PWR_MGMT_MODE_PWR_SAVING       0x18

//Power-Down Management Control 2 register
#define KSZ8895_PD_MGMT_CTRL2_GO_SLEEP_TIME                  0xFF

//Port N Control 0 register
#define KSZ8895_PORTn_CTRL0_BCAST_STORM_PROTECT_EN           0x80
#define KSZ8895_PORTn_CTRL0_DIFFSERV_PRIO_CLASS_EN           0x40
#define KSZ8895_PORTn_CTRL0_802_1P_PRIO_CLASS_EN             0x20
#define KSZ8895_PORTn_CTRL0_PORT_PRIO_CLASS_EN               0x18
#define KSZ8895_PORTn_CTRL0_TAG_INSERTION                    0x04
#define KSZ8895_PORTn_CTRL0_TAG_REMOVAL                      0x02
#define KSZ8895_PORTn_CTRL0_TWO_QUEUE_SPLIT_EN               0x01

//Port N Control 1 register
#define KSZ8895_PORTn_CTRL1_SNIFFER_PORT                     0x80
#define KSZ8895_PORTn_CTRL1_RECEIVE_SNIFF                    0x40
#define KSZ8895_PORTn_CTRL1_TRANSMIT_SNIFF                   0x20
#define KSZ8895_PORTn_CTRL1_PORT_VLAN_MEMBERSHIP             0x1F

//Port N Control 2 register
#define KSZ8895_PORTn_CTRL2_USER_PRIO_CEILING                0x80
#define KSZ8895_PORTn_CTRL2_INGRESS_VLAN_FILT                0x40
#define KSZ8895_PORTn_CTRL2_DISCARD_NON_PVID_PKT             0x20
#define KSZ8895_PORTn_CTRL2_FORCE_FLOW_CTRL                  0x10
#define KSZ8895_PORTn_CTRL2_BACK_PRESSURE_EN                 0x08
#define KSZ8895_PORTn_CTRL2_TRANSMIT_EN                      0x04
#define KSZ8895_PORTn_CTRL2_RECEIVE_EN                       0x02
#define KSZ8895_PORTn_CTRL2_LEARNING_DIS                     0x01

//Port N Control 3 register
#define KSZ8895_PORTn_CTRL3_DEFAULT_USER_PRIO                0xE0
#define KSZ8895_PORTn_CTRL3_DEFAULT_CFI                      0x10
#define KSZ8895_PORTn_CTRL3_DEFAULT_VID_MSB                  0x0F

//Port N Control 4 register
#define KSZ8895_PORTn_CTRL4_DEFAULT_VID_LSB                  0xFF

//RMII Management Control register
#define KSZ8895_RMII_MGMT_CTRL_SW5_CLK_OUT_DIS               0x08
#define KSZ8895_RMII_MGMT_CTRL_P5_CLK_OUT_DIS                0x04

//Port N Status 0 register
#define KSZ8895_PORTn_STAT0_HP_MDIX                          0x80
#define KSZ8895_PORTn_STAT0_POLRVS                           0x20
#define KSZ8895_PORTn_STAT0_TX_FLOW_CTRL_EN                  0x10
#define KSZ8895_PORTn_STAT0_RX_FLOW_CTRL_EN                  0x08
#define KSZ8895_PORTn_STAT0_OP_SPEED                         0x04
#define KSZ8895_PORTn_STAT0_OP_DUPLEX                        0x02

//Port N PHY Special Control/Status register
#define KSZ8895_PORTn_PSCS_VCT_10M_SHORT                     0x80
#define KSZ8895_PORTn_PSCS_VCT_RESULT                        0x60
#define KSZ8895_PORTn_PSCS_VCT_EN                            0x10
#define KSZ8895_PORTn_PSCS_FORCE_LNK                         0x08
#define KSZ8895_PORTn_PSCS_PWRSAVE                           0x04
#define KSZ8895_PORTn_PSCS_REMOTE_LOOPBACK                   0x02
#define KSZ8895_PORTn_PSCS_VCT_FAULT_COUNT_MSB               0x01

//Port N LinkMD Result register
#define KSZ8895_PORTn_LINKMD_VCT_FAULT_COUNT_LSB             0xFF

//Port N Control 5 register
#define KSZ8895_PORTn_CTRL5_AN_DIS                           0x80
#define KSZ8895_PORTn_CTRL5_FORCED_SPEED                     0x40
#define KSZ8895_PORTn_CTRL5_FORCED_DUPLEX                    0x20
#define KSZ8895_PORTn_CTRL5_ADV_FLOW_CTRL                    0x10
#define KSZ8895_PORTn_CTRL5_ADV_100BT_FD                     0x08
#define KSZ8895_PORTn_CTRL5_ADV_100BT_HD                     0x04
#define KSZ8895_PORTn_CTRL5_ADV_10BT_FD                      0x02
#define KSZ8895_PORTn_CTRL5_ADV_10BT_HD                      0x01

//Port N Control 6 register
#define KSZ8895_PORTn_CTRL6_LED_OFF                          0x80
#define KSZ8895_PORTn_CTRL6_TX_DIS                           0x40
#define KSZ8895_PORTn_CTRL6_RESTART_AN                       0x20
#define KSZ8895_PORTn_CTRL6_FAR_END_FAULT_DIS                0x10
#define KSZ8895_PORTn_CTRL6_POWER_DOWN                       0x08
#define KSZ8895_PORTn_CTRL6_AUTO_MDIX_DIS                    0x04
#define KSZ8895_PORTn_CTRL6_FORCED_MDI                       0x02
#define KSZ8895_PORTn_CTRL6_MAC_LOOPBACK                     0x01

//Port N Status 1 register
#define KSZ8895_PORTn_STAT1_MDIX_STATUS                      0x80
#define KSZ8895_PORTn_STAT1_AN_DONE                          0x40
#define KSZ8895_PORTn_STAT1_LINK_GOOD                        0x20
#define KSZ8895_PORTn_STAT1_LP_FLOW_CTRL_CAPABLE             0x10
#define KSZ8895_PORTn_STAT1_LP_100BTX_FD_CAPABLE             0x08
#define KSZ8895_PORTn_STAT1_LP_100BTX_HF_CAPABLE             0x04
#define KSZ8895_PORTn_STAT1_LP_10BT_FD_CAPABLE               0x02
#define KSZ8895_PORTn_STAT1_LP_10BT_HD_CAPABLE               0x01

//Port N Control 7 / Status 2 register
#define KSZ8895_PORTn_CTRL7_STAT2_PHY_LOOPBACK               0x80
#define KSZ8895_PORTn_CTRL7_STAT2_PHY_ISOLATE                0x20
#define KSZ8895_PORTn_CTRL7_STAT2_SOFT_RESET                 0x10
#define KSZ8895_PORTn_CTRL7_STAT2_FORCE_LINK                 0x08
#define KSZ8895_PORTn_CTRL7_STAT2_OP_MODE                    0x07
#define KSZ8895_PORTn_CTRL7_STAT2_OP_MODE_AN                 0x01
#define KSZ8895_PORTn_CTRL7_STAT2_OP_MODE_10BT_HD            0x02
#define KSZ8895_PORTn_CTRL7_STAT2_OP_MODE_100BTX_HD          0x03
#define KSZ8895_PORTn_CTRL7_STAT2_OP_MODE_10BT_FD            0x05
#define KSZ8895_PORTn_CTRL7_STAT2_OP_MODE_100BTX_FD          0x06

//Indirect Access Control 0 register
#define KSZ8895_INDIRECT_CTRL0_WRITE                         0x00
#define KSZ8895_INDIRECT_CTRL0_READ                          0x10
#define KSZ8895_INDIRECT_CTRL0_TABLE_SEL                     0x0C
#define KSZ8895_INDIRECT_CTRL0_TABLE_SEL_STATIC_MAC          0x00
#define KSZ8895_INDIRECT_CTRL0_TABLE_SEL_VLAN                0x04
#define KSZ8895_INDIRECT_CTRL0_TABLE_SEL_DYNAMIC_MAC         0x08
#define KSZ8895_INDIRECT_CTRL0_TABLE_SEL_MIB_COUNTER         0x0C
#define KSZ8895_INDIRECT_CTRL0_ADDR_H                        0x03

//Indirect Access Control 1 register
#define KSZ8895_INDIRECT_CTRL1_ADDR_L                        0xFF

//Interrupt Status register
#define KSZ8895_INT_STAT_PORT5                               0x10
#define KSZ8895_INT_STAT_PORT4                               0x08
#define KSZ8895_INT_STAT_PORT3                               0x04
#define KSZ8895_INT_STAT_PORT2                               0x02
#define KSZ8895_INT_STAT_PORT1                               0x01

//Interrupt Mask register
#define KSZ8895_INT_MASK_PORT5                               0x10
#define KSZ8895_INT_MASK_PORT4                               0x08
#define KSZ8895_INT_MASK_PORT3                               0x04
#define KSZ8895_INT_MASK_PORT2                               0x02
#define KSZ8895_INT_MASK_PORT1                               0x01

//Global Control 12 register
#define KSZ8895_GLOBAL_CTRL12_TAG3                           0xC0
#define KSZ8895_GLOBAL_CTRL12_TAG2                           0x30
#define KSZ8895_GLOBAL_CTRL12_TAG1                           0x0C
#define KSZ8895_GLOBAL_CTRL12_TAG0                           0x03

//Global Control 13 register
#define KSZ8895_GLOBAL_CTRL13_TAG7                           0xC0
#define KSZ8895_GLOBAL_CTRL13_TAG6                           0x30
#define KSZ8895_GLOBAL_CTRL13_TAG5                           0x0C
#define KSZ8895_GLOBAL_CTRL13_TAG4                           0x03

//Global Control 14 register
#define KSZ8895_GLOBAL_CTRL14_PRI_2Q                         0xC0

//Global Control 15 register
#define KSZ8895_GLOBAL_CTRL15_UNKNOWN_UNICAST_FWD            0x20
#define KSZ8895_GLOBAL_CTRL15_UNKNOWN_UNICAST_FWD_MAP        0x1F
#define KSZ8895_GLOBAL_CTRL15_UNKNOWN_UNICAST_FWD_MAP_FILT   0x00
#define KSZ8895_GLOBAL_CTRL15_UNKNOWN_UNICAST_FWD_MAP_PORT1  0x01
#define KSZ8895_GLOBAL_CTRL15_UNKNOWN_UNICAST_FWD_MAP_PORT2  0x02
#define KSZ8895_GLOBAL_CTRL15_UNKNOWN_UNICAST_FWD_MAP_PORT3  0x04
#define KSZ8895_GLOBAL_CTRL15_UNKNOWN_UNICAST_FWD_MAP_PORT4  0x08
#define KSZ8895_GLOBAL_CTRL15_UNKNOWN_UNICAST_FWD_MAP_PORT5  0x10
#define KSZ8895_GLOBAL_CTRL15_UNKNOWN_UNICAST_FWD_MAP_ALL    0x1F

//Global Control 16 register
#define KSZ8895_GLOBAL_CTRL16_CHIP_OUT_DRIVE_STRENGTH        0xC0
#define KSZ8895_GLOBAL_CTRL16_CHIP_OUT_DRIVE_STRENGTH_4MA    0x00
#define KSZ8895_GLOBAL_CTRL16_CHIP_OUT_DRIVE_STRENGTH_8MA    0x40
#define KSZ8895_GLOBAL_CTRL16_CHIP_OUT_DRIVE_STRENGTH_10MA   0x80
#define KSZ8895_GLOBAL_CTRL16_CHIP_OUT_DRIVE_STRENGTH_14MA   0xC0
#define KSZ8895_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD              0x20
#define KSZ8895_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD_MAP          0x1F
#define KSZ8895_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD_MAP_FILT     0x00
#define KSZ8895_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD_MAP_PORT1    0x01
#define KSZ8895_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD_MAP_PORT2    0x02
#define KSZ8895_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD_MAP_PORT3    0x04
#define KSZ8895_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD_MAP_PORT4    0x08
#define KSZ8895_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD_MAP_PORT5    0x10
#define KSZ8895_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD_MAP_ALL      0x1F

//Global Control 17 register
#define KSZ8895_GLOBAL_CTRL17_UNKNOWN_VID_FWD                0x20
#define KSZ8895_GLOBAL_CTRL17_UNKNOWN_VID_FWD_MAP            0x1F
#define KSZ8895_GLOBAL_CTRL17_UNKNOWN_VID_FWD_MAP_FILT       0x00
#define KSZ8895_GLOBAL_CTRL17_UNKNOWN_VID_FWD_MAP_PORT1      0x01
#define KSZ8895_GLOBAL_CTRL17_UNKNOWN_VID_FWD_MAP_PORT2      0x02
#define KSZ8895_GLOBAL_CTRL17_UNKNOWN_VID_FWD_MAP_PORT3      0x04
#define KSZ8895_GLOBAL_CTRL17_UNKNOWN_VID_FWD_MAP_PORT4      0x08
#define KSZ8895_GLOBAL_CTRL17_UNKNOWN_VID_FWD_MAP_PORT5      0x10
#define KSZ8895_GLOBAL_CTRL17_UNKNOWN_VID_FWD_MAP_ALL        0x1F

//Global Control 18 register
#define KSZ8895_GLOBAL_CTRL18_SELF_ADDR_FILTER_EN            0x40
#define KSZ8895_GLOBAL_CTRL18_UNKNOWN_IP_MCAST_FWD           0x20
#define KSZ8895_GLOBAL_CTRL18_UNKNOWN_IP_MCAST_FWD_MAP       0x1F
#define KSZ8895_GLOBAL_CTRL18_UNKNOWN_IP_MCAST_FWD_MAP_FILT  0x00
#define KSZ8895_GLOBAL_CTRL18_UNKNOWN_IP_MCAST_FWD_MAP_PORT1 0x01
#define KSZ8895_GLOBAL_CTRL18_UNKNOWN_IP_MCAST_FWD_MAP_PORT2 0x02
#define KSZ8895_GLOBAL_CTRL18_UNKNOWN_IP_MCAST_FWD_MAP_PORT3 0x04
#define KSZ8895_GLOBAL_CTRL18_UNKNOWN_IP_MCAST_FWD_MAP_PORT4 0x08
#define KSZ8895_GLOBAL_CTRL18_UNKNOWN_IP_MCAST_FWD_MAP_PORT5 0x10
#define KSZ8895_GLOBAL_CTRL18_UNKNOWN_IP_MCAST_FWD_MAP_ALL   0x1F

//Global Control 19 register
#define KSZ8895_GLOBAL_CTRL19_IG_RATE_LIMIT_PERIOD           0x30
#define KSZ8895_GLOBAL_CTRL19_IG_RATE_LIMIT_PERIOD_16MS      0x00
#define KSZ8895_GLOBAL_CTRL19_IG_RATE_LIMIT_PERIOD_64MS      0x10
#define KSZ8895_GLOBAL_CTRL19_IG_RATE_LIMIT_PERIOD_256MS     0x20
#define KSZ8895_GLOBAL_CTRL19_QUEUE_BASED_EG_RATE_LIMITE_EN  0x08
#define KSZ8895_GLOBAL_CTRL19_INSERT_SRC_PORT_PVID_TAG_EN    0x04

//Identification register
#define KSZ8895_ID_REVISION_ID                               0xF0
#define KSZ8895_ID_REVISION_ID_MQX_RQX_FQX_REV_A2            0x40
#define KSZ8895_ID_REVISION_ID_ML_REV_B2                     0x40
#define KSZ8895_ID_REVISION_ID_MQX_RQX_FQX_REV_A3            0x50
#define KSZ8895_ID_REVISION_ID_ML_REV_B3                     0x50
#define KSZ8895_ID_REVISION_ID_MQX_RQX_FQX_REV_A4            0x60

//Port N Control 9 register
#define KSZ8895_PORTn_CTRL9_FOUR_QUEUE_SPLIT_EN              0x02
#define KSZ8895_PORTn_CTRL9_DROPPING_TAG_EN                  0x01

//Port N Control 10 register
#define KSZ8895_PORTn_CTRL10_PORT_TX_QUEUE3_RATIO_EN         0x80
#define KSZ8895_PORTn_CTRL10_PORT_TX_QUEUE3_RATIO            0x7F

//Port N Control 11 register
#define KSZ8895_PORTn_CTRL11_PORT_TX_QUEUE2_RATIO_EN         0x80
#define KSZ8895_PORTn_CTRL11_PORT_TX_QUEUE2_RATIO            0x7F

//Port N Control 12 register
#define KSZ8895_PORTn_CTRL12_PORT_TX_QUEUE1_RATIO_EN         0x80
#define KSZ8895_PORTn_CTRL12_PORT_TX_QUEUE1_RATIO            0x7F

//Port N Control 13 register
#define KSZ8895_PORTn_CTRL13_PORT_TX_QUEUE0_RATIO_EN         0x80
#define KSZ8895_PORTn_CTRL13_PORT_TX_QUEUE0_RATIO            0x7F

//Port N Rate Limit Control register
#define KSZ8895_PORTn_RATE_LIMIT_CTRL_LIMIT_EN               0x10
#define KSZ8895_PORTn_RATE_LIMIT_CTRL_LIMIT_MODE             0x0C
#define KSZ8895_PORTn_RATE_LIMIT_CTRL_COUNT_IFG              0x02
#define KSZ8895_PORTn_RATE_LIMIT_CTRL_COUNT_PRE              0x01

//Port 3 Copper or Fiber Control register
#define KSZ8895_PORT3_COPPER_FIBER_CTRL_FIBER_SEL            0x80

//Testing 3 register
#define KSZ8895_TEST3_SW5_RMII_INV_SMTXC                     0x40
#define KSZ8895_TEST3_SW5_RMII_INV_SMRXC                     0x10

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief Static MAC table entry (read operation)
 **/

typedef struct
{
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t fid : 7;          //0
   uint8_t useFid : 1;
   uint8_t reserved : 1;     //1
   uint8_t override : 1;
   uint8_t valid : 1;
   uint8_t forwardPorts : 5;
#else
   uint8_t useFid : 1;       //0
   uint8_t fid : 7;
   uint8_t forwardPorts : 5; //1
   uint8_t valid : 1;
   uint8_t override : 1;
   uint8_t reserved : 1;
#endif
   MacAddr macAddr;          //2-7
} Ksz8895StaticMacEntryR;


/**
 * @brief Static MAC table entry (write operation)
 **/

typedef struct
{
   uint8_t fid;              //0
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t useFid : 1;       //1
   uint8_t override : 1;
   uint8_t valid : 1;
   uint8_t forwardPorts : 5;
#else
   uint8_t forwardPorts : 5; //1
   uint8_t valid : 1;
   uint8_t override : 1;
   uint8_t useFid : 1;
#endif
   MacAddr macAddr;          //2-7
} Ksz8895StaticMacEntryW;


/**
 * @brief Dynamic MAC table entry
 **/

typedef struct
{
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t macEmpty : 1;         //0
   uint8_t numValidEntriesH : 7;
   uint8_t numValidEntriesL : 3; //1
   uint8_t timestamp : 2;
   uint8_t sourcePort : 3;
   uint8_t dataNotReady : 1;     //2
   uint8_t fid : 7;
#else
   uint8_t numValidEntriesH : 7; //0
   uint8_t macEmpty : 1;
   uint8_t sourcePort : 3;       //1
   uint8_t timestamp : 2;
   uint8_t numValidEntriesL : 3;
   uint8_t fid : 7;              //2
   uint8_t dataNotReady : 1;
#endif
   MacAddr macAddr;              //3-8
} Ksz8895DynamicMacEntry;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif

//KSZ8895 Ethernet switch driver
extern const SwitchDriver ksz8895SwitchDriver;

//KSZ8895 related functions
error_t ksz8895Init(NetInterface *interface);

void ksz8895Tick(NetInterface *interface);

void ksz8895EnableIrq(NetInterface *interface);
void ksz8895DisableIrq(NetInterface *interface);

void ksz8895EventHandler(NetInterface *interface);

error_t ksz8895TagFrame(NetInterface *interface, NetBuffer *buffer,
   size_t *offset, NetTxAncillary *ancillary);

error_t ksz8895UntagFrame(NetInterface *interface, uint8_t **frame,
   size_t *length, NetRxAncillary *ancillary);

bool_t ksz8895GetLinkState(NetInterface *interface, uint8_t port);
uint32_t ksz8895GetLinkSpeed(NetInterface *interface, uint8_t port);
NicDuplexMode ksz8895GetDuplexMode(NetInterface *interface, uint8_t port);

void ksz8895SetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state);

SwitchPortState ksz8895GetPortState(NetInterface *interface, uint8_t port);

void ksz8895SetAgingTime(NetInterface *interface, uint32_t agingTime);

void ksz8895EnableIgmpSnooping(NetInterface *interface, bool_t enable);
void ksz8895EnableMldSnooping(NetInterface *interface, bool_t enable);
void ksz8895EnableRsvdMcastTable(NetInterface *interface, bool_t enable);

error_t ksz8895AddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t ksz8895DeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t ksz8895GetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void ksz8895FlushStaticFdbTable(NetInterface *interface);

error_t ksz8895GetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void ksz8895FlushDynamicFdbTable(NetInterface *interface, uint8_t port);

void ksz8895SetUnknownMcastFwdPorts(NetInterface *interface,
   bool_t enable, uint32_t forwardPorts);

void ksz8895WritePhyReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data);

uint16_t ksz8895ReadPhyReg(NetInterface *interface, uint8_t port,
   uint8_t address);

void ksz8895DumpPhyReg(NetInterface *interface, uint8_t port);

void ksz8895WriteSwitchReg(NetInterface *interface, uint8_t address,
   uint8_t data);

uint8_t ksz8895ReadSwitchReg(NetInterface *interface, uint8_t address);

void ksz8895DumpSwitchReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
