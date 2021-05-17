/**
 * @file ksz9477_driver.h
 * @brief KSZ9477 7-port Gigabit Ethernet switch driver
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

#ifndef _KSZ9477_DRIVER_H
#define _KSZ9477_DRIVER_H

//Dependencies
#include "core/nic.h"

//Port identifiers
#define KSZ9477_PORT1 1
#define KSZ9477_PORT2 2
#define KSZ9477_PORT3 3
#define KSZ9477_PORT4 4
#define KSZ9477_PORT5 5
#define KSZ9477_PORT6 6
#define KSZ9477_PORT7 7

//Port masks
#define KSZ9477_PORT_MASK  0x7F
#define KSZ9477_PORT1_MASK 0x01
#define KSZ9477_PORT2_MASK 0x02
#define KSZ9477_PORT3_MASK 0x04
#define KSZ9477_PORT4_MASK 0x08
#define KSZ9477_PORT5_MASK 0x10
#define KSZ9477_PORT6_MASK 0x20
#define KSZ9477_PORT7_MASK 0x40

//SPI command byte
#define KSZ9477_SPI_CMD_WRITE 0x40000000
#define KSZ9477_SPI_CMD_READ  0x60000000
#define KSZ9477_SPI_CMD_ADDR  0x001FFFE0

//Size of static and dynamic MAC tables
#define KSZ9477_STATIC_MAC_TABLE_SIZE  16
#define KSZ9477_DYNAMIC_MAC_TABLE_SIZE 4096

//Tail tag rules (host to KSZ9477)
#define KSZ9477_TAIL_TAG_NORMAL_ADDR_LOOKUP     0x0400
#define KSZ9477_TAIL_TAG_PORT_BLOCKING_OVERRIDE 0x0200
#define KSZ9477_TAIL_TAG_PRIORITY               0x0180
#define KSZ9477_TAIL_TAG_DEST_PORT7             0x0040
#define KSZ9477_TAIL_TAG_DEST_PORT6             0x0020
#define KSZ9477_TAIL_TAG_DEST_PORT5             0x0010
#define KSZ9477_TAIL_TAG_DEST_PORT4             0x0008
#define KSZ9477_TAIL_TAG_DEST_PORT3             0x0004
#define KSZ9477_TAIL_TAG_DEST_PORT2             0x0002
#define KSZ9477_TAIL_TAG_DEST_PORT1             0x0001

//Tail tag rules (KSZ9477 to host)
#define KSZ9477_TAIL_TAG_PTP_MSG                0x80
#define KSZ9477_TAIL_TAG_SRC_PORT               0x07

//KSZ9477 PHY registers
#define KSZ9477_BMCR                                           0x00
#define KSZ9477_BMSR                                           0x01
#define KSZ9477_PHYID1                                         0x02
#define KSZ9477_PHYID2                                         0x03
#define KSZ9477_ANAR                                           0x04
#define KSZ9477_ANLPAR                                         0x05
#define KSZ9477_ANER                                           0x06
#define KSZ9477_ANNPR                                          0x07
#define KSZ9477_ANLPNPR                                        0x08
#define KSZ9477_GBCR                                           0x09
#define KSZ9477_GBSR                                           0x0A
#define KSZ9477_MMDACR                                         0x0D
#define KSZ9477_MMDAADR                                        0x0E
#define KSZ9477_GBESR                                          0x0F
#define KSZ9477_RLB                                            0x11
#define KSZ9477_LINKMD                                         0x12
#define KSZ9477_DPMAPCSS                                       0x13
#define KSZ9477_RXERCTR                                        0x15
#define KSZ9477_ICSR                                           0x1B
#define KSZ9477_AUTOMDI                                        0x1C
#define KSZ9477_PHYCON                                         0x1F

//KSZ9477 MMD registers
#define KSZ9477_MMD_SIGNAL_QUALITY_CH_A                        0x01, 0xAC
#define KSZ9477_MMD_SIGNAL_QUALITY_CH_B                        0x01, 0xAD
#define KSZ9477_MMD_SIGNAL_QUALITY_CH_C                        0x01, 0xAE
#define KSZ9477_MMD_SIGNAL_QUALITY_CH_D                        0x01, 0xAF
#define KSZ9477_MMD_LED_MODE                                   0x02, 0x00
#define KSZ9477_MMD_EEE_ADV                                    0x07, 0x3C
#define KSZ9477_MMD_QUIET_WIRE_CONFIG0                         0x1C, 0x25
#define KSZ9477_MMD_QUIET_WIRE_CONFIG1                         0x1C, 0x26
#define KSZ9477_MMD_QUIET_WIRE_CONFIG2                         0x1C, 0x27
#define KSZ9477_MMD_QUIET_WIRE_CONFIG3                         0x1C, 0x28
#define KSZ9477_MMD_QUIET_WIRE_CONFIG4                         0x1C, 0x29
#define KSZ9477_MMD_QUIET_WIRE_CONFIG5                         0x1C, 0x2A
#define KSZ9477_MMD_QUIET_WIRE_CONFIG6                         0x1C, 0x2B
#define KSZ9477_MMD_QUIET_WIRE_CONFIG7                         0x1C, 0x2C
#define KSZ9477_MMD_QUIET_WIRE_CONFIG8                         0x1C, 0x2D
#define KSZ9477_MMD_QUIET_WIRE_CONFIG9                         0x1C, 0x2E
#define KSZ9477_MMD_QUIET_WIRE_CONFIG10                        0x1C, 0x2F
#define KSZ9477_MMD_QUIET_WIRE_CONFIG11                        0x1C, 0x30
#define KSZ9477_MMD_QUIET_WIRE_CONFIG12                        0x1C, 0x31
#define KSZ9477_MMD_QUIET_WIRE_CONFIG13                        0x1C, 0x32
#define KSZ9477_MMD_QUIET_WIRE_CONFIG14                        0x1C, 0x33
#define KSZ9477_MMD_QUIET_WIRE_CONFIG15                        0x1C, 0x34

//KSZ9477 Switch registers
#define KSZ9477_CHIP_ID0                                       0x0000
#define KSZ9477_CHIP_ID1                                       0x0001
#define KSZ9477_CHIP_ID2                                       0x0002
#define KSZ9477_CHIP_ID3                                       0x0003
#define KSZ9477_PME_PIN_CTRL                                   0x0006
#define KSZ9477_GLOBAL_INT_STAT                                0x0010
#define KSZ9477_GLOBAL_INT_MASK                                0x0014
#define KSZ9477_GLOBAL_PORT_INT_STAT                           0x0018
#define KSZ9477_GLOBAL_PORT_INT_MASK                           0x001C
#define KSZ9477_SERIAL_IO_CTRL                                 0x0100
#define KSZ9477_OUT_CLK_CTRL                                   0x0103
#define KSZ9477_IBA_CTRL                                       0x0104
#define KSZ9477_IO_DRIVE_STRENGTH                              0x010D
#define KSZ9477_IBA_OP_STAT1                                   0x0110
#define KSZ9477_LED_OVERRIDE                                   0x0120
#define KSZ9477_LED_OUTPUT                                     0x0124
#define KSZ9477_LED2_0_LED2_1_SOURCE                           0x0128
#define KSZ9477_PWR_DOWN_CTRL0                                 0x0201
#define KSZ9477_LED_STRAP_IN                                   0x0210
#define KSZ9477_SWITCH_OP                                      0x0300
#define KSZ9477_SWITCH_MAC_ADDR0                               0x0302
#define KSZ9477_SWITCH_MAC_ADDR1                               0x0303
#define KSZ9477_SWITCH_MAC_ADDR2                               0x0304
#define KSZ9477_SWITCH_MAC_ADDR3                               0x0305
#define KSZ9477_SWITCH_MAC_ADDR4                               0x0306
#define KSZ9477_SWITCH_MAC_ADDR5                               0x0307
#define KSZ9477_SWITCH_MTU                                     0x0308
#define KSZ9477_SWITCH_ISP_TPID                                0x030A
#define KSZ9477_SWITCH_HSR_TPID                                0x030C
#define KSZ9477_AVB_CBS_STRATEGY                               0x030E
#define KSZ9477_SWITCH_LUE_CTRL0                               0x0310
#define KSZ9477_SWITCH_LUE_CTRL1                               0x0311
#define KSZ9477_SWITCH_LUE_CTRL2                               0x0312
#define KSZ9477_SWITCH_LUE_CTRL3                               0x0313
#define KSZ9477_ALU_TABLE_INT                                  0x0314
#define KSZ9477_ALU_TABLE_MASK                                 0x0315
#define KSZ9477_ALU_TABLE_ENTRY_INDEX0                         0x0316
#define KSZ9477_ALU_TABLE_ENTRY_INDEX1                         0x0318
#define KSZ9477_ALU_TABLE_ENTRY_INDEX2                         0x031A
#define KSZ9477_UNKNOWN_UNICAST_CTRL                           0x0320
#define KSZ9477_UNKONWN_MULTICAST_CTRL                         0x0324
#define KSZ9477_UNKNOWN_VLAN_ID_CTRL                           0x0328
#define KSZ9477_SWITCH_MAC_CTRL0                               0x0330
#define KSZ9477_SWITCH_MAC_CTRL1                               0x0331
#define KSZ9477_SWITCH_MAC_CTRL2                               0x0332
#define KSZ9477_SWITCH_MAC_CTRL3                               0x0333
#define KSZ9477_SWITCH_MAC_CTRL4                               0x0334
#define KSZ9477_SWITCH_MAC_CTRL5                               0x0335
#define KSZ9477_SWITCH_MIB_CTRL                                0x0336
#define KSZ9477_802_1P_PRIO_MAPPING0                           0x0338
#define KSZ9477_802_1P_PRIO_MAPPING1                           0x0339
#define KSZ9477_802_1P_PRIO_MAPPING2                           0x033A
#define KSZ9477_802_1P_PRIO_MAPPING3                           0x033B
#define KSZ9477_IP_DIFFSERV_PRIO_EN                            0x033E
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING0                      0x0340
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING1                      0x0341
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING2                      0x0342
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING3                      0x0343
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING4                      0x0344
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING5                      0x0345
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING6                      0x0346
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING7                      0x0347
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING8                      0x0348
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING9                      0x0349
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING10                     0x034A
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING11                     0x034B
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING12                     0x034C
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING13                     0x034D
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING14                     0x034E
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING15                     0x034F
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING16                     0x0350
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING17                     0x0351
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING18                     0x0352
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING19                     0x0353
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING20                     0x0354
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING21                     0x0355
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING22                     0x0356
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING23                     0x0357
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING24                     0x0358
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING25                     0x0359
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING26                     0x035A
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING27                     0x035B
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING28                     0x035C
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING29                     0x035D
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING30                     0x035E
#define KSZ9477_IP_DIFFSERV_PRIO_MAPPING31                     0x035F
#define KSZ9477_GLOBAL_PORT_MIRROR_SNOOP_CTRL                  0x0370
#define KSZ9477_WRED_DIFFSERV_COLOR_MAPPING                    0x0378
#define KSZ9477_PTP_EVENT_MSG_PRIO                             0x037C
#define KSZ9477_PTP_NON_EVENT_MSG_PRIO                         0x037D
#define KSZ9477_QUEUE_MGMT_CTRL0                               0x0390
#define KSZ9477_VLAN_TABLE_ENTRY0                              0x0400
#define KSZ9477_VLAN_TABLE_ENTRY1                              0x0404
#define KSZ9477_VLAN_TABLE_ENTRY2                              0x0408
#define KSZ9477_VLAN_TABLE_INDEX                               0x040C
#define KSZ9477_VLAN_TABLE_ACCESS_CTRL                         0x040E
#define KSZ9477_ALU_TABLE_INDEX0                               0x0410
#define KSZ9477_ALU_TABLE_INDEX1                               0x0414
#define KSZ9477_ALU_TABLE_CTRL                                 0x0418
#define KSZ9477_STATIC_RES_MCAST_TABLE_CTRL                    0x041C
#define KSZ9477_ALU_TABLE_ENTRY1                               0x0420
#define KSZ9477_STATIC_TABLE_ENTRY1                            0x0420
#define KSZ9477_ALU_TABLE_ENTRY2                               0x0424
#define KSZ9477_STATIC_TABLE_ENTRY2                            0x0424
#define KSZ9477_RES_MCAST_TABLE_ENTRY2                         0x0424
#define KSZ9477_ALU_TABLE_ENTRY3                               0x0428
#define KSZ9477_STATIC_TABLE_ENTRY3                            0x0428
#define KSZ9477_ALU_TABLE_ENTRY4                               0x042C
#define KSZ9477_STATIC_TABLE_ENTRY4                            0x042C
#define KSZ9477_GLOBAL_HSR_ALU_INDEX1                          0x0444
#define KSZ9477_GLOBAL_HSR_ALU_INDEX2                          0x0448
#define KSZ9477_GLOBAL_HSR_ALU_ACCESS_CTRL                     0x0450
#define KSZ9477_GLOBAL_HSR_ALU_VALUE_A                         0x0454
#define KSZ9477_GLOBAL_HSR_ALU_VALUE_B                         0x0458
#define KSZ9477_GLOBAL_HSR_ALU_VALUE_C                         0x045C
#define KSZ9477_GLOBAL_HSR_ALU_VALUE_D                         0x0460
#define KSZ9477_GLOBAL_HSR_ALU_VALUE_E                         0x0464
#define KSZ9477_GLOBAL_HSR_ALU_VALUE_F                         0x0468
#define KSZ9477_GLOBAL_HSR_ALU_VALUE_G                         0x046C
#define KSZ9477_GLOBAL_PTP_CLK_CTRL                            0x0500
#define KSZ9477_GLOBAL_PTP_RTC_CLK_PHASE                       0x0502
#define KSZ9477_GLOBAL_PTP_RTC_CLK_NS_H                        0x0504
#define KSZ9477_GLOBAL_PTP_RTC_CLK_NS_L                        0x0506
#define KSZ9477_GLOBAL_PTP_RTC_CLK_S_H                         0x0508
#define KSZ9477_GLOBAL_PTP_RTC_CLK_S_L                         0x050A
#define KSZ9477_GLOBAL_PTP_CLK_SUB_NS_RATE_H                   0x050C
#define KSZ9477_GLOBAL_PTP_CLK_SUB_NS_RATE_L                   0x050E
#define KSZ9477_GLOBAL_PTP_CLK_TEMP_ADJ_DURATION_H             0x0510
#define KSZ9477_GLOBAL_PTP_CLK_TEMP_ADJ_DURATION_L             0x0512
#define KSZ9477_GLOBAL_PTP_MSG_CONFIG1                         0x0514
#define KSZ9477_GLOBAL_PTP_MSG_CONFIG2                         0x0516
#define KSZ9477_GLOBAL_PTP_DOMAIN_VERSION                      0x0518
#define KSZ9477_GLOBAL_PTP_UNIT_INDEX                          0x0520
#define KSZ9477_GPIO_STATUS_MONITOR0                           0x0524
#define KSZ9477_GPIO_STATUS_MONITOR1                           0x0528
#define KSZ9477_TS_CTRL_STAT                                   0x052C
#define KSZ9477_TRIGGER_OUT_UNIT_TARGET_TIME_NS                0x0530
#define KSZ9477_TRIGGER_OUT_UNIT_TARGET_TIME_S                 0x0534
#define KSZ9477_TRIGGER_OUT_UNIT_CTRL1                         0x0538
#define KSZ9477_TRIGGER_OUT_UNIT_CTRL2                         0x053C
#define KSZ9477_TRIGGER_OUT_UNIT_CTRL3                         0x0540
#define KSZ9477_TRIGGER_OUT_UNIT_CTRL4                         0x0544
#define KSZ9477_TRIGGER_OUT_UNIT_CTRL5                         0x0548
#define KSZ9477_TS_STAT_CTRL                                   0x0550
#define KSZ9477_TS_SAMPLE1_TIME_NS                             0x0554
#define KSZ9477_TS_SAMPLE1_TIME_S                              0x0558
#define KSZ9477_TS_SAMPLE1_TIME_PHASE                          0x055C
#define KSZ9477_TS_SAMPLE2_TIME_NS                             0x0560
#define KSZ9477_TS_SAMPLE2_TIME_S                              0x0564
#define KSZ9477_TS_SAMPLE2_TIME_PHASE                          0x0568
#define KSZ9477_TS_SAMPLE3_TIME_NS                             0x056C
#define KSZ9477_TS_SAMPLE3_TIME_S                              0x0570
#define KSZ9477_TS_SAMPLE3_TIME_PHASE                          0x0574
#define KSZ9477_TS_SAMPLE4_TIME_NS                             0x0578
#define KSZ9477_TS_SAMPLE4_TIME_S                              0x057C
#define KSZ9477_TS_SAMPLE4_TIME_PHASE                          0x0580
#define KSZ9477_TS_SAMPLE5_TIME_NS                             0x0584
#define KSZ9477_TS_SAMPLE5_TIME_S                              0x0588
#define KSZ9477_TS_SAMPLE5_TIME_PHASE                          0x058C
#define KSZ9477_TS_SAMPLE6_TIME_NS                             0x0590
#define KSZ9477_TS_SAMPLE6_TIME_S                              0x0594
#define KSZ9477_TS_SAMPLE6_TIME_PHASE                          0x0598
#define KSZ9477_TS_SAMPLE7_TIME_NS                             0x059C
#define KSZ9477_TS_SAMPLE7_TIME_S                              0x05A0
#define KSZ9477_TS_SAMPLE7_TIME_PHASE                          0x05A4
#define KSZ9477_TS_SAMPLE8_TIME_NS                             0x05A8
#define KSZ9477_TS_SAMPLE8_TIME_S                              0x05AC
#define KSZ9477_TS_SAMPLE8_TIME_PHASE                          0x05B0
#define KSZ9477_GLOBAL_DLR_SRC_PORT                            0x0604
#define KSZ9477_GLOBAL_DLR_SRC_IP_ADDR                         0x0608
#define KSZ9477_GLOBAL_DLR_CTRL                                0x0610
#define KSZ9477_GLOBAL_DLR_STATE                               0x0611
#define KSZ9477_GLOBAL_DLR_SUPERVISOR_PRECEDENT                0x0612
#define KSZ9477_GLOBAL_DLR_BEACON_INTERVAL                     0x0614
#define KSZ9477_GLOBAL_DLR_BEACON_TIMEOUT                      0x0618
#define KSZ9477_GLOBAL_DLR_BEACON_TIMEOUT_WINDOW               0x061C
#define KSZ9477_GLOBAL_DLR_VLAN                                0x0620
#define KSZ9477_GLOBAL_DLR_DEST_ADDR                           0x0622
#define KSZ9477_GLOBAL_DLR_PORT_MAP                            0x0628
#define KSZ9477_GLOBAL_DLR_CLASS                               0x062C
#define KSZ9477_GLOBAL_HSR_PORT_MAP                            0x0640
#define KSZ9477_GLOBAL_HSR_AME_CTRL0                           0x0644
#define KSZ9477_GLOBAL_HSR_AME_CTRL1                           0x0645
#define KSZ9477_GLOBAL_HSR_AME_AGE_PERIOD                      0x0648
#define KSZ9477_GLOBAL_HSR_AME_INT                             0x064C
#define KSZ9477_GLOBAL_HSR_AME_INT_MASK                        0x064D
#define KSZ9477_PORT1_DEFAULT_TAG0                             0x1000
#define KSZ9477_PORT1_DEFAULT_TAG1                             0x1001
#define KSZ9477_PORT1_PME_WOL_EVENT                            0x1013
#define KSZ9477_PORT1_PME_WOL_EN                               0x1017
#define KSZ9477_PORT1_INT_STATUS                               0x101B
#define KSZ9477_PORT1_INT_MASK                                 0x101F
#define KSZ9477_PORT1_OP_CTRL0                                 0x1020
#define KSZ9477_PORT1_STATUS                                   0x1030
#define KSZ9477_PORT1_MAC_CTRL0                                0x1400
#define KSZ9477_PORT1_MAC_CTRL1                                0x1401
#define KSZ9477_PORT1_IG_RATE_LIMIT_CTRL                       0x1403
#define KSZ9477_PORT1_PRIO0_IG_LIMIT_CTRL                      0x1410
#define KSZ9477_PORT1_PRIO1_IG_LIMIT_CTRL                      0x1411
#define KSZ9477_PORT1_PRIO2_IG_LIMIT_CTRL                      0x1412
#define KSZ9477_PORT1_PRIO3_IG_LIMIT_CTRL                      0x1413
#define KSZ9477_PORT1_PRIO4_IG_LIMIT_CTRL                      0x1414
#define KSZ9477_PORT1_PRIO5_IG_LIMIT_CTRL                      0x1415
#define KSZ9477_PORT1_PRIO6_IG_LIMIT_CTRL                      0x1416
#define KSZ9477_PORT1_PRIO7_IG_LIMIT_CTRL                      0x1417
#define KSZ9477_PORT1_QUEUE0_EG_LIMIT_CTRL                     0x1420
#define KSZ9477_PORT1_QUEUE1_EG_LIMIT_CTRL                     0x1421
#define KSZ9477_PORT1_QUEUE2_EG_LIMIT_CTRL                     0x1422
#define KSZ9477_PORT1_QUEUE3_EG_LIMIT_CTRL                     0x1423
#define KSZ9477_PORT1_MIB_CTRL_STAT                            0x1500
#define KSZ9477_PORT1_MIB_DATA                                 0x1504
#define KSZ9477_PORT1_ACL_ACCESS0                              0x1600
#define KSZ9477_PORT1_ACL_ACCESS1                              0x1601
#define KSZ9477_PORT1_ACL_ACCESS2                              0x1602
#define KSZ9477_PORT1_ACL_ACCESS3                              0x1603
#define KSZ9477_PORT1_ACL_ACCESS4                              0x1604
#define KSZ9477_PORT1_ACL_ACCESS5                              0x1605
#define KSZ9477_PORT1_ACL_ACCESS6                              0x1606
#define KSZ9477_PORT1_ACL_ACCESS7                              0x1607
#define KSZ9477_PORT1_ACL_ACCESS8                              0x1608
#define KSZ9477_PORT1_ACL_ACCESS9                              0x1609
#define KSZ9477_PORT1_ACL_ACCESS10                             0x160A
#define KSZ9477_PORT1_ACL_ACCESS11                             0x160B
#define KSZ9477_PORT1_ACL_ACCESS12                             0x160C
#define KSZ9477_PORT1_ACL_ACCESS13                             0x160D
#define KSZ9477_PORT1_ACL_ACCESS14                             0x160E
#define KSZ9477_PORT1_ACL_ACCESS15                             0x160F
#define KSZ9477_PORT1_ACL_BYTE_EN_MSB                          0x1610
#define KSZ9477_PORT1_ACL_BYTE_EN_LSB                          0x1611
#define KSZ9477_PORT1_ACL_ACCESS_CTRL0                         0x1612
#define KSZ9477_PORT1_ACL_ACCESS_CTRL1                         0x1613
#define KSZ9477_PORT1_MIRRORING_CTRL                           0x1800
#define KSZ9477_PORT1_PRIO_CTRL                                0x1801
#define KSZ9477_PORT1_IG_MAC_CTRL                              0x1802
#define KSZ9477_PORT1_AUTH_CTRL                                0x1803
#define KSZ9477_PORT1_PTR                                      0x1804
#define KSZ9477_PORT1_PRIO_TO_QUEUE_MAPPING                    0x1808
#define KSZ9477_PORT1_POLICE_CTRL                              0x180C
#define KSZ9477_PORT1_POLICE_QUEUE_RATE                        0x1820
#define KSZ9477_PORT1_POLICE_QUEUE_BURST_SIZE                  0x1824
#define KSZ9477_PORT1_WRED_PKT_MEM_CTRL0                       0x1830
#define KSZ9477_PORT1_WRED_PKT_MEM_CTRL1                       0x1834
#define KSZ9477_PORT1_WRED_QUEUE_CTRL0                         0x1840
#define KSZ9477_PORT1_WRED_QUEUE_CTRL1                         0x1844
#define KSZ9477_PORT1_WRED_QUEUE_PERF_MON_CTRL                 0x1848
#define KSZ9477_PORT1_TX_QUEUE_INDEX                           0x1900
#define KSZ9477_PORT1_TX_QUEUE_PVID                            0x1904
#define KSZ9477_PORT1_TX_QUEUE_CTRL0                           0x1914
#define KSZ9477_PORT1_TX_QUEUE_CTRL1                           0x1915
#define KSZ9477_PORT1_TX_CREDIT_SHAPER_CTRL0                   0x1916
#define KSZ9477_PORT1_TX_CREDIT_SHAPER_CTRL1                   0x1918
#define KSZ9477_PORT1_TX_CREDIT_SHAPER_CTRL2                   0x191A
#define KSZ9477_PORT1_TAS_CTRL                                 0x1920
#define KSZ9477_PORT1_TAS_EVENT_INDEX                          0x1923
#define KSZ9477_PORT1_TAS_EVENT                                0x1924
#define KSZ9477_PORT1_CTRL0                                    0x1A00
#define KSZ9477_PORT1_CTRL1                                    0x1A04
#define KSZ9477_PORT1_CTRL2                                    0x1B00
#define KSZ9477_PORT1_MSTP_PTR                                 0x1B01
#define KSZ9477_PORT1_MSTP_STATE                               0x1B04
#define KSZ9477_PORT1_PTP_RX_LATENCY                           0x1C00
#define KSZ9477_PORT1_PTP_TX_LATENCY                           0x1C02
#define KSZ9477_PORT1_PTP_ASYM_CORRECTION                      0x1C04
#define KSZ9477_PORT1_PTP_XDLY_REQ_TSH                         0x1C08
#define KSZ9477_PORT1_PTP_XDLY_REQ_TSL                         0x1C0A
#define KSZ9477_PORT1_PTP_SYNC_TSH                             0x1C0C
#define KSZ9477_PORT1_PTP_SYNC_TSL                             0x1C0E
#define KSZ9477_PORT1_PTP_PDLY_RESP_TSH                        0x1C10
#define KSZ9477_PORT1_PTP_PDLY_RESP_TSL                        0x1C12
#define KSZ9477_PORT1_PTP_TS_INT_STAT                          0x1C14
#define KSZ9477_PORT1_PTP_TS_INT_EN                            0x1C16
#define KSZ9477_PORT1_PTP_LINK_DELAY                           0x1C18
#define KSZ9477_PORT2_DEFAULT_TAG0                             0x2000
#define KSZ9477_PORT2_DEFAULT_TAG1                             0x2001
#define KSZ9477_PORT2_PME_WOL_EVENT                            0x2013
#define KSZ9477_PORT2_PME_WOL_EN                               0x2017
#define KSZ9477_PORT2_INT_STATUS                               0x201B
#define KSZ9477_PORT2_INT_MASK                                 0x201F
#define KSZ9477_PORT2_OP_CTRL0                                 0x2020
#define KSZ9477_PORT2_STATUS                                   0x2030
#define KSZ9477_PORT2_MAC_CTRL0                                0x2400
#define KSZ9477_PORT2_MAC_CTRL1                                0x2401
#define KSZ9477_PORT2_IG_RATE_LIMIT_CTRL                       0x2403
#define KSZ9477_PORT2_PRIO0_IG_LIMIT_CTRL                      0x2410
#define KSZ9477_PORT2_PRIO1_IG_LIMIT_CTRL                      0x2411
#define KSZ9477_PORT2_PRIO2_IG_LIMIT_CTRL                      0x2412
#define KSZ9477_PORT2_PRIO3_IG_LIMIT_CTRL                      0x2413
#define KSZ9477_PORT2_PRIO4_IG_LIMIT_CTRL                      0x2414
#define KSZ9477_PORT2_PRIO5_IG_LIMIT_CTRL                      0x2415
#define KSZ9477_PORT2_PRIO6_IG_LIMIT_CTRL                      0x2416
#define KSZ9477_PORT2_PRIO7_IG_LIMIT_CTRL                      0x2417
#define KSZ9477_PORT2_QUEUE0_EG_LIMIT_CTRL                     0x2420
#define KSZ9477_PORT2_QUEUE1_EG_LIMIT_CTRL                     0x2421
#define KSZ9477_PORT2_QUEUE2_EG_LIMIT_CTRL                     0x2422
#define KSZ9477_PORT2_QUEUE3_EG_LIMIT_CTRL                     0x2423
#define KSZ9477_PORT2_MIB_CTRL_STAT                            0x2500
#define KSZ9477_PORT2_MIB_DATA                                 0x2504
#define KSZ9477_PORT2_ACL_ACCESS0                              0x2600
#define KSZ9477_PORT2_ACL_ACCESS1                              0x2601
#define KSZ9477_PORT2_ACL_ACCESS2                              0x2602
#define KSZ9477_PORT2_ACL_ACCESS3                              0x2603
#define KSZ9477_PORT2_ACL_ACCESS4                              0x2604
#define KSZ9477_PORT2_ACL_ACCESS5                              0x2605
#define KSZ9477_PORT2_ACL_ACCESS6                              0x2606
#define KSZ9477_PORT2_ACL_ACCESS7                              0x2607
#define KSZ9477_PORT2_ACL_ACCESS8                              0x2608
#define KSZ9477_PORT2_ACL_ACCESS9                              0x2609
#define KSZ9477_PORT2_ACL_ACCESS10                             0x260A
#define KSZ9477_PORT2_ACL_ACCESS11                             0x260B
#define KSZ9477_PORT2_ACL_ACCESS12                             0x260C
#define KSZ9477_PORT2_ACL_ACCESS13                             0x260D
#define KSZ9477_PORT2_ACL_ACCESS14                             0x260E
#define KSZ9477_PORT2_ACL_ACCESS15                             0x260F
#define KSZ9477_PORT2_ACL_BYTE_EN_MSB                          0x2610
#define KSZ9477_PORT2_ACL_BYTE_EN_LSB                          0x2611
#define KSZ9477_PORT2_ACL_ACCESS_CTRL0                         0x2612
#define KSZ9477_PORT2_ACL_ACCESS_CTRL1                         0x2613
#define KSZ9477_PORT2_MIRRORING_CTRL                           0x2800
#define KSZ9477_PORT2_PRIO_CTRL                                0x2801
#define KSZ9477_PORT2_IG_MAC_CTRL                              0x2802
#define KSZ9477_PORT2_AUTH_CTRL                                0x2803
#define KSZ9477_PORT2_PTR                                      0x2804
#define KSZ9477_PORT2_PRIO_TO_QUEUE_MAPPING                    0x2808
#define KSZ9477_PORT2_POLICE_CTRL                              0x280C
#define KSZ9477_PORT2_POLICE_QUEUE_RATE                        0x2820
#define KSZ9477_PORT2_POLICE_QUEUE_BURST_SIZE                  0x2824
#define KSZ9477_PORT2_WRED_PKT_MEM_CTRL0                       0x2830
#define KSZ9477_PORT2_WRED_PKT_MEM_CTRL1                       0x2834
#define KSZ9477_PORT2_WRED_QUEUE_CTRL0                         0x2840
#define KSZ9477_PORT2_WRED_QUEUE_CTRL1                         0x2844
#define KSZ9477_PORT2_WRED_QUEUE_PERF_MON_CTRL                 0x2848
#define KSZ9477_PORT2_TX_QUEUE_INDEX                           0x2900
#define KSZ9477_PORT2_TX_QUEUE_PVID                            0x2904
#define KSZ9477_PORT2_TX_QUEUE_CTRL0                           0x2914
#define KSZ9477_PORT2_TX_QUEUE_CTRL1                           0x2915
#define KSZ9477_PORT2_TX_CREDIT_SHAPER_CTRL0                   0x2916
#define KSZ9477_PORT2_TX_CREDIT_SHAPER_CTRL1                   0x2918
#define KSZ9477_PORT2_TX_CREDIT_SHAPER_CTRL2                   0x291A
#define KSZ9477_PORT2_TAS_CTRL                                 0x2920
#define KSZ9477_PORT2_TAS_EVENT_INDEX                          0x2923
#define KSZ9477_PORT2_TAS_EVENT                                0x2924
#define KSZ9477_PORT2_CTRL0                                    0x2A00
#define KSZ9477_PORT2_CTRL1                                    0x2A04
#define KSZ9477_PORT2_CTRL2                                    0x2B00
#define KSZ9477_PORT2_MSTP_PTR                                 0x2B01
#define KSZ9477_PORT2_MSTP_STATE                               0x2B04
#define KSZ9477_PORT2_PTP_RX_LATENCY                           0x2C00
#define KSZ9477_PORT2_PTP_TX_LATENCY                           0x2C02
#define KSZ9477_PORT2_PTP_ASYM_CORRECTION                      0x2C04
#define KSZ9477_PORT2_PTP_XDLY_REQ_TSH                         0x2C08
#define KSZ9477_PORT2_PTP_XDLY_REQ_TSL                         0x2C0A
#define KSZ9477_PORT2_PTP_SYNC_TSH                             0x2C0C
#define KSZ9477_PORT2_PTP_SYNC_TSL                             0x2C0E
#define KSZ9477_PORT2_PTP_PDLY_RESP_TSH                        0x2C10
#define KSZ9477_PORT2_PTP_PDLY_RESP_TSL                        0x2C12
#define KSZ9477_PORT2_PTP_TS_INT_STAT                          0x2C14
#define KSZ9477_PORT2_PTP_TS_INT_EN                            0x2C16
#define KSZ9477_PORT2_PTP_LINK_DELAY                           0x2C18
#define KSZ9477_PORT3_DEFAULT_TAG0                             0x3000
#define KSZ9477_PORT3_DEFAULT_TAG1                             0x3001
#define KSZ9477_PORT3_PME_WOL_EVENT                            0x3013
#define KSZ9477_PORT3_PME_WOL_EN                               0x3017
#define KSZ9477_PORT3_INT_STATUS                               0x301B
#define KSZ9477_PORT3_INT_MASK                                 0x301F
#define KSZ9477_PORT3_OP_CTRL0                                 0x3020
#define KSZ9477_PORT3_STATUS                                   0x3030
#define KSZ9477_PORT3_MAC_CTRL0                                0x3400
#define KSZ9477_PORT3_MAC_CTRL1                                0x3401
#define KSZ9477_PORT3_IG_RATE_LIMIT_CTRL                       0x3403
#define KSZ9477_PORT3_PRIO0_IG_LIMIT_CTRL                      0x3410
#define KSZ9477_PORT3_PRIO1_IG_LIMIT_CTRL                      0x3411
#define KSZ9477_PORT3_PRIO2_IG_LIMIT_CTRL                      0x3412
#define KSZ9477_PORT3_PRIO3_IG_LIMIT_CTRL                      0x3413
#define KSZ9477_PORT3_PRIO4_IG_LIMIT_CTRL                      0x3414
#define KSZ9477_PORT3_PRIO5_IG_LIMIT_CTRL                      0x3415
#define KSZ9477_PORT3_PRIO6_IG_LIMIT_CTRL                      0x3416
#define KSZ9477_PORT3_PRIO7_IG_LIMIT_CTRL                      0x3417
#define KSZ9477_PORT3_QUEUE0_EG_LIMIT_CTRL                     0x3420
#define KSZ9477_PORT3_QUEUE1_EG_LIMIT_CTRL                     0x3421
#define KSZ9477_PORT3_QUEUE2_EG_LIMIT_CTRL                     0x3422
#define KSZ9477_PORT3_QUEUE3_EG_LIMIT_CTRL                     0x3423
#define KSZ9477_PORT3_MIB_CTRL_STAT                            0x3500
#define KSZ9477_PORT3_MIB_DATA                                 0x3504
#define KSZ9477_PORT3_ACL_ACCESS0                              0x3600
#define KSZ9477_PORT3_ACL_ACCESS1                              0x3601
#define KSZ9477_PORT3_ACL_ACCESS2                              0x3602
#define KSZ9477_PORT3_ACL_ACCESS3                              0x3603
#define KSZ9477_PORT3_ACL_ACCESS4                              0x3604
#define KSZ9477_PORT3_ACL_ACCESS5                              0x3605
#define KSZ9477_PORT3_ACL_ACCESS6                              0x3606
#define KSZ9477_PORT3_ACL_ACCESS7                              0x3607
#define KSZ9477_PORT3_ACL_ACCESS8                              0x3608
#define KSZ9477_PORT3_ACL_ACCESS9                              0x3609
#define KSZ9477_PORT3_ACL_ACCESS10                             0x360A
#define KSZ9477_PORT3_ACL_ACCESS11                             0x360B
#define KSZ9477_PORT3_ACL_ACCESS12                             0x360C
#define KSZ9477_PORT3_ACL_ACCESS13                             0x360D
#define KSZ9477_PORT3_ACL_ACCESS14                             0x360E
#define KSZ9477_PORT3_ACL_ACCESS15                             0x360F
#define KSZ9477_PORT3_ACL_BYTE_EN_MSB                          0x3610
#define KSZ9477_PORT3_ACL_BYTE_EN_LSB                          0x3611
#define KSZ9477_PORT3_ACL_ACCESS_CTRL0                         0x3612
#define KSZ9477_PORT3_ACL_ACCESS_CTRL1                         0x3613
#define KSZ9477_PORT3_MIRRORING_CTRL                           0x3800
#define KSZ9477_PORT3_PRIO_CTRL                                0x3801
#define KSZ9477_PORT3_IG_MAC_CTRL                              0x3802
#define KSZ9477_PORT3_AUTH_CTRL                                0x3803
#define KSZ9477_PORT3_PTR                                      0x3804
#define KSZ9477_PORT3_PRIO_TO_QUEUE_MAPPING                    0x3808
#define KSZ9477_PORT3_POLICE_CTRL                              0x380C
#define KSZ9477_PORT3_POLICE_QUEUE_RATE                        0x3820
#define KSZ9477_PORT3_POLICE_QUEUE_BURST_SIZE                  0x3824
#define KSZ9477_PORT3_WRED_PKT_MEM_CTRL0                       0x3830
#define KSZ9477_PORT3_WRED_PKT_MEM_CTRL1                       0x3834
#define KSZ9477_PORT3_WRED_QUEUE_CTRL0                         0x3840
#define KSZ9477_PORT3_WRED_QUEUE_CTRL1                         0x3844
#define KSZ9477_PORT3_WRED_QUEUE_PERF_MON_CTRL                 0x3848
#define KSZ9477_PORT3_TX_QUEUE_INDEX                           0x3900
#define KSZ9477_PORT3_TX_QUEUE_PVID                            0x3904
#define KSZ9477_PORT3_TX_QUEUE_CTRL0                           0x3914
#define KSZ9477_PORT3_TX_QUEUE_CTRL1                           0x3915
#define KSZ9477_PORT3_TX_CREDIT_SHAPER_CTRL0                   0x3916
#define KSZ9477_PORT3_TX_CREDIT_SHAPER_CTRL1                   0x3918
#define KSZ9477_PORT3_TX_CREDIT_SHAPER_CTRL2                   0x391A
#define KSZ9477_PORT3_TAS_CTRL                                 0x3920
#define KSZ9477_PORT3_TAS_EVENT_INDEX                          0x3923
#define KSZ9477_PORT3_TAS_EVENT                                0x3924
#define KSZ9477_PORT3_CTRL0                                    0x3A00
#define KSZ9477_PORT3_CTRL1                                    0x3A04
#define KSZ9477_PORT3_CTRL2                                    0x3B00
#define KSZ9477_PORT3_MSTP_PTR                                 0x3B01
#define KSZ9477_PORT3_MSTP_STATE                               0x3B04
#define KSZ9477_PORT3_PTP_RX_LATENCY                           0x3C00
#define KSZ9477_PORT3_PTP_TX_LATENCY                           0x3C02
#define KSZ9477_PORT3_PTP_ASYM_CORRECTION                      0x3C04
#define KSZ9477_PORT3_PTP_XDLY_REQ_TSH                         0x3C08
#define KSZ9477_PORT3_PTP_XDLY_REQ_TSL                         0x3C0A
#define KSZ9477_PORT3_PTP_SYNC_TSH                             0x3C0C
#define KSZ9477_PORT3_PTP_SYNC_TSL                             0x3C0E
#define KSZ9477_PORT3_PTP_PDLY_RESP_TSH                        0x3C10
#define KSZ9477_PORT3_PTP_PDLY_RESP_TSL                        0x3C12
#define KSZ9477_PORT3_PTP_TS_INT_STAT                          0x3C14
#define KSZ9477_PORT3_PTP_TS_INT_EN                            0x3C16
#define KSZ9477_PORT3_PTP_LINK_DELAY                           0x3C18
#define KSZ9477_PORT4_DEFAULT_TAG0                             0x4000
#define KSZ9477_PORT4_DEFAULT_TAG1                             0x4001
#define KSZ9477_PORT4_PME_WOL_EVENT                            0x4013
#define KSZ9477_PORT4_PME_WOL_EN                               0x4017
#define KSZ9477_PORT4_INT_STATUS                               0x401B
#define KSZ9477_PORT4_INT_MASK                                 0x401F
#define KSZ9477_PORT4_OP_CTRL0                                 0x4020
#define KSZ9477_PORT4_STATUS                                   0x4030
#define KSZ9477_PORT4_MAC_CTRL0                                0x4400
#define KSZ9477_PORT4_MAC_CTRL1                                0x4401
#define KSZ9477_PORT4_IG_RATE_LIMIT_CTRL                       0x4403
#define KSZ9477_PORT4_PRIO0_IG_LIMIT_CTRL                      0x4410
#define KSZ9477_PORT4_PRIO1_IG_LIMIT_CTRL                      0x4411
#define KSZ9477_PORT4_PRIO2_IG_LIMIT_CTRL                      0x4412
#define KSZ9477_PORT4_PRIO3_IG_LIMIT_CTRL                      0x4413
#define KSZ9477_PORT4_PRIO4_IG_LIMIT_CTRL                      0x4414
#define KSZ9477_PORT4_PRIO5_IG_LIMIT_CTRL                      0x4415
#define KSZ9477_PORT4_PRIO6_IG_LIMIT_CTRL                      0x4416
#define KSZ9477_PORT4_PRIO7_IG_LIMIT_CTRL                      0x4417
#define KSZ9477_PORT4_QUEUE0_EG_LIMIT_CTRL                     0x4420
#define KSZ9477_PORT4_QUEUE1_EG_LIMIT_CTRL                     0x4421
#define KSZ9477_PORT4_QUEUE2_EG_LIMIT_CTRL                     0x4422
#define KSZ9477_PORT4_QUEUE3_EG_LIMIT_CTRL                     0x4423
#define KSZ9477_PORT4_MIB_CTRL_STAT                            0x4500
#define KSZ9477_PORT4_MIB_DATA                                 0x4504
#define KSZ9477_PORT4_ACL_ACCESS0                              0x4600
#define KSZ9477_PORT4_ACL_ACCESS1                              0x4601
#define KSZ9477_PORT4_ACL_ACCESS2                              0x4602
#define KSZ9477_PORT4_ACL_ACCESS3                              0x4603
#define KSZ9477_PORT4_ACL_ACCESS4                              0x4604
#define KSZ9477_PORT4_ACL_ACCESS5                              0x4605
#define KSZ9477_PORT4_ACL_ACCESS6                              0x4606
#define KSZ9477_PORT4_ACL_ACCESS7                              0x4607
#define KSZ9477_PORT4_ACL_ACCESS8                              0x4608
#define KSZ9477_PORT4_ACL_ACCESS9                              0x4609
#define KSZ9477_PORT4_ACL_ACCESS10                             0x460A
#define KSZ9477_PORT4_ACL_ACCESS11                             0x460B
#define KSZ9477_PORT4_ACL_ACCESS12                             0x460C
#define KSZ9477_PORT4_ACL_ACCESS13                             0x460D
#define KSZ9477_PORT4_ACL_ACCESS14                             0x460E
#define KSZ9477_PORT4_ACL_ACCESS15                             0x460F
#define KSZ9477_PORT4_ACL_BYTE_EN_MSB                          0x4610
#define KSZ9477_PORT4_ACL_BYTE_EN_LSB                          0x4611
#define KSZ9477_PORT4_ACL_ACCESS_CTRL0                         0x4612
#define KSZ9477_PORT4_ACL_ACCESS_CTRL1                         0x4613
#define KSZ9477_PORT4_MIRRORING_CTRL                           0x4800
#define KSZ9477_PORT4_PRIO_CTRL                                0x4801
#define KSZ9477_PORT4_IG_MAC_CTRL                              0x4802
#define KSZ9477_PORT4_AUTH_CTRL                                0x4803
#define KSZ9477_PORT4_PTR                                      0x4804
#define KSZ9477_PORT4_PRIO_TO_QUEUE_MAPPING                    0x4808
#define KSZ9477_PORT4_POLICE_CTRL                              0x480C
#define KSZ9477_PORT4_POLICE_QUEUE_RATE                        0x4820
#define KSZ9477_PORT4_POLICE_QUEUE_BURST_SIZE                  0x4824
#define KSZ9477_PORT4_WRED_PKT_MEM_CTRL0                       0x4830
#define KSZ9477_PORT4_WRED_PKT_MEM_CTRL1                       0x4834
#define KSZ9477_PORT4_WRED_QUEUE_CTRL0                         0x4840
#define KSZ9477_PORT4_WRED_QUEUE_CTRL1                         0x4844
#define KSZ9477_PORT4_WRED_QUEUE_PERF_MON_CTRL                 0x4848
#define KSZ9477_PORT4_TX_QUEUE_INDEX                           0x4900
#define KSZ9477_PORT4_TX_QUEUE_PVID                            0x4904
#define KSZ9477_PORT4_TX_QUEUE_CTRL0                           0x4914
#define KSZ9477_PORT4_TX_QUEUE_CTRL1                           0x4915
#define KSZ9477_PORT4_TX_CREDIT_SHAPER_CTRL0                   0x4916
#define KSZ9477_PORT4_TX_CREDIT_SHAPER_CTRL1                   0x4918
#define KSZ9477_PORT4_TX_CREDIT_SHAPER_CTRL2                   0x491A
#define KSZ9477_PORT4_TAS_CTRL                                 0x4920
#define KSZ9477_PORT4_TAS_EVENT_INDEX                          0x4923
#define KSZ9477_PORT4_TAS_EVENT                                0x4924
#define KSZ9477_PORT4_CTRL0                                    0x4A00
#define KSZ9477_PORT4_CTRL1                                    0x4A04
#define KSZ9477_PORT4_CTRL2                                    0x4B00
#define KSZ9477_PORT4_MSTP_PTR                                 0x4B01
#define KSZ9477_PORT4_MSTP_STATE                               0x4B04
#define KSZ9477_PORT4_PTP_RX_LATENCY                           0x4C00
#define KSZ9477_PORT4_PTP_TX_LATENCY                           0x4C02
#define KSZ9477_PORT4_PTP_ASYM_CORRECTION                      0x4C04
#define KSZ9477_PORT4_PTP_XDLY_REQ_TSH                         0x4C08
#define KSZ9477_PORT4_PTP_XDLY_REQ_TSL                         0x4C0A
#define KSZ9477_PORT4_PTP_SYNC_TSH                             0x4C0C
#define KSZ9477_PORT4_PTP_SYNC_TSL                             0x4C0E
#define KSZ9477_PORT4_PTP_PDLY_RESP_TSH                        0x4C10
#define KSZ9477_PORT4_PTP_PDLY_RESP_TSL                        0x4C12
#define KSZ9477_PORT4_PTP_TS_INT_STAT                          0x4C14
#define KSZ9477_PORT4_PTP_TS_INT_EN                            0x4C16
#define KSZ9477_PORT4_PTP_LINK_DELAY                           0x4C18
#define KSZ9477_PORT5_DEFAULT_TAG0                             0x5000
#define KSZ9477_PORT5_DEFAULT_TAG1                             0x5001
#define KSZ9477_PORT5_PME_WOL_EVENT                            0x5013
#define KSZ9477_PORT5_PME_WOL_EN                               0x5017
#define KSZ9477_PORT5_INT_STATUS                               0x501B
#define KSZ9477_PORT5_INT_MASK                                 0x501F
#define KSZ9477_PORT5_OP_CTRL0                                 0x5020
#define KSZ9477_PORT5_STATUS                                   0x5030
#define KSZ9477_PORT5_MAC_CTRL0                                0x5400
#define KSZ9477_PORT5_MAC_CTRL1                                0x5401
#define KSZ9477_PORT5_IG_RATE_LIMIT_CTRL                       0x5403
#define KSZ9477_PORT5_PRIO0_IG_LIMIT_CTRL                      0x5410
#define KSZ9477_PORT5_PRIO1_IG_LIMIT_CTRL                      0x5411
#define KSZ9477_PORT5_PRIO2_IG_LIMIT_CTRL                      0x5412
#define KSZ9477_PORT5_PRIO3_IG_LIMIT_CTRL                      0x5413
#define KSZ9477_PORT5_PRIO4_IG_LIMIT_CTRL                      0x5414
#define KSZ9477_PORT5_PRIO5_IG_LIMIT_CTRL                      0x5415
#define KSZ9477_PORT5_PRIO6_IG_LIMIT_CTRL                      0x5416
#define KSZ9477_PORT5_PRIO7_IG_LIMIT_CTRL                      0x5417
#define KSZ9477_PORT5_QUEUE0_EG_LIMIT_CTRL                     0x5420
#define KSZ9477_PORT5_QUEUE1_EG_LIMIT_CTRL                     0x5421
#define KSZ9477_PORT5_QUEUE2_EG_LIMIT_CTRL                     0x5422
#define KSZ9477_PORT5_QUEUE3_EG_LIMIT_CTRL                     0x5423
#define KSZ9477_PORT5_MIB_CTRL_STAT                            0x5500
#define KSZ9477_PORT5_MIB_DATA                                 0x5504
#define KSZ9477_PORT5_ACL_ACCESS0                              0x5600
#define KSZ9477_PORT5_ACL_ACCESS1                              0x5601
#define KSZ9477_PORT5_ACL_ACCESS2                              0x5602
#define KSZ9477_PORT5_ACL_ACCESS3                              0x5603
#define KSZ9477_PORT5_ACL_ACCESS4                              0x5604
#define KSZ9477_PORT5_ACL_ACCESS5                              0x5605
#define KSZ9477_PORT5_ACL_ACCESS6                              0x5606
#define KSZ9477_PORT5_ACL_ACCESS7                              0x5607
#define KSZ9477_PORT5_ACL_ACCESS8                              0x5608
#define KSZ9477_PORT5_ACL_ACCESS9                              0x5609
#define KSZ9477_PORT5_ACL_ACCESS10                             0x560A
#define KSZ9477_PORT5_ACL_ACCESS11                             0x560B
#define KSZ9477_PORT5_ACL_ACCESS12                             0x560C
#define KSZ9477_PORT5_ACL_ACCESS13                             0x560D
#define KSZ9477_PORT5_ACL_ACCESS14                             0x560E
#define KSZ9477_PORT5_ACL_ACCESS15                             0x560F
#define KSZ9477_PORT5_ACL_BYTE_EN_MSB                          0x5610
#define KSZ9477_PORT5_ACL_BYTE_EN_LSB                          0x5611
#define KSZ9477_PORT5_ACL_ACCESS_CTRL0                         0x5612
#define KSZ9477_PORT5_ACL_ACCESS_CTRL1                         0x5613
#define KSZ9477_PORT5_MIRRORING_CTRL                           0x5800
#define KSZ9477_PORT5_PRIO_CTRL                                0x5801
#define KSZ9477_PORT5_IG_MAC_CTRL                              0x5802
#define KSZ9477_PORT5_AUTH_CTRL                                0x5803
#define KSZ9477_PORT5_PTR                                      0x5804
#define KSZ9477_PORT5_PRIO_TO_QUEUE_MAPPING                    0x5808
#define KSZ9477_PORT5_POLICE_CTRL                              0x580C
#define KSZ9477_PORT5_POLICE_QUEUE_RATE                        0x5820
#define KSZ9477_PORT5_POLICE_QUEUE_BURST_SIZE                  0x5824
#define KSZ9477_PORT5_WRED_PKT_MEM_CTRL0                       0x5830
#define KSZ9477_PORT5_WRED_PKT_MEM_CTRL1                       0x5834
#define KSZ9477_PORT5_WRED_QUEUE_CTRL0                         0x5840
#define KSZ9477_PORT5_WRED_QUEUE_CTRL1                         0x5844
#define KSZ9477_PORT5_WRED_QUEUE_PERF_MON_CTRL                 0x5848
#define KSZ9477_PORT5_TX_QUEUE_INDEX                           0x5900
#define KSZ9477_PORT5_TX_QUEUE_PVID                            0x5904
#define KSZ9477_PORT5_TX_QUEUE_CTRL0                           0x5914
#define KSZ9477_PORT5_TX_QUEUE_CTRL1                           0x5915
#define KSZ9477_PORT5_TX_CREDIT_SHAPER_CTRL0                   0x5916
#define KSZ9477_PORT5_TX_CREDIT_SHAPER_CTRL1                   0x5918
#define KSZ9477_PORT5_TX_CREDIT_SHAPER_CTRL2                   0x591A
#define KSZ9477_PORT5_TAS_CTRL                                 0x5920
#define KSZ9477_PORT5_TAS_EVENT_INDEX                          0x5923
#define KSZ9477_PORT5_TAS_EVENT                                0x5924
#define KSZ9477_PORT5_CTRL0                                    0x5A00
#define KSZ9477_PORT5_CTRL1                                    0x5A04
#define KSZ9477_PORT5_CTRL2                                    0x5B00
#define KSZ9477_PORT5_MSTP_PTR                                 0x5B01
#define KSZ9477_PORT5_MSTP_STATE                               0x5B04
#define KSZ9477_PORT5_PTP_RX_LATENCY                           0x5C00
#define KSZ9477_PORT5_PTP_TX_LATENCY                           0x5C02
#define KSZ9477_PORT5_PTP_ASYM_CORRECTION                      0x5C04
#define KSZ9477_PORT5_PTP_XDLY_REQ_TSH                         0x5C08
#define KSZ9477_PORT5_PTP_XDLY_REQ_TSL                         0x5C0A
#define KSZ9477_PORT5_PTP_SYNC_TSH                             0x5C0C
#define KSZ9477_PORT5_PTP_SYNC_TSL                             0x5C0E
#define KSZ9477_PORT5_PTP_PDLY_RESP_TSH                        0x5C10
#define KSZ9477_PORT5_PTP_PDLY_RESP_TSL                        0x5C12
#define KSZ9477_PORT5_PTP_TS_INT_STAT                          0x5C14
#define KSZ9477_PORT5_PTP_TS_INT_EN                            0x5C16
#define KSZ9477_PORT5_PTP_LINK_DELAY                           0x5C18
#define KSZ9477_PORT6_DEFAULT_TAG0                             0x6000
#define KSZ9477_PORT6_DEFAULT_TAG1                             0x6001
#define KSZ9477_PORT6_PME_WOL_EVENT                            0x6013
#define KSZ9477_PORT6_PME_WOL_EN                               0x6017
#define KSZ9477_PORT6_INT_STATUS                               0x601B
#define KSZ9477_PORT6_INT_MASK                                 0x601F
#define KSZ9477_PORT6_OP_CTRL0                                 0x6020
#define KSZ9477_PORT6_STATUS                                   0x6030
#define KSZ9477_PORT6_XMII_CTRL0                               0x6300
#define KSZ9477_PORT6_XMII_CTRL1                               0x6301
#define KSZ9477_PORT6_MAC_CTRL0                                0x6400
#define KSZ9477_PORT6_MAC_CTRL1                                0x6401
#define KSZ9477_PORT6_IG_RATE_LIMIT_CTRL                       0x6403
#define KSZ9477_PORT6_PRIO0_IG_LIMIT_CTRL                      0x6410
#define KSZ9477_PORT6_PRIO1_IG_LIMIT_CTRL                      0x6411
#define KSZ9477_PORT6_PRIO2_IG_LIMIT_CTRL                      0x6412
#define KSZ9477_PORT6_PRIO3_IG_LIMIT_CTRL                      0x6413
#define KSZ9477_PORT6_PRIO4_IG_LIMIT_CTRL                      0x6414
#define KSZ9477_PORT6_PRIO5_IG_LIMIT_CTRL                      0x6415
#define KSZ9477_PORT6_PRIO6_IG_LIMIT_CTRL                      0x6416
#define KSZ9477_PORT6_PRIO7_IG_LIMIT_CTRL                      0x6417
#define KSZ9477_PORT6_QUEUE0_EG_LIMIT_CTRL                     0x6420
#define KSZ9477_PORT6_QUEUE1_EG_LIMIT_CTRL                     0x6421
#define KSZ9477_PORT6_QUEUE2_EG_LIMIT_CTRL                     0x6422
#define KSZ9477_PORT6_QUEUE3_EG_LIMIT_CTRL                     0x6423
#define KSZ9477_PORT6_MIB_CTRL_STAT                            0x6500
#define KSZ9477_PORT6_MIB_DATA                                 0x6504
#define KSZ9477_PORT6_ACL_ACCESS0                              0x6600
#define KSZ9477_PORT6_ACL_ACCESS1                              0x6601
#define KSZ9477_PORT6_ACL_ACCESS2                              0x6602
#define KSZ9477_PORT6_ACL_ACCESS3                              0x6603
#define KSZ9477_PORT6_ACL_ACCESS4                              0x6604
#define KSZ9477_PORT6_ACL_ACCESS5                              0x6605
#define KSZ9477_PORT6_ACL_ACCESS6                              0x6606
#define KSZ9477_PORT6_ACL_ACCESS7                              0x6607
#define KSZ9477_PORT6_ACL_ACCESS8                              0x6608
#define KSZ9477_PORT6_ACL_ACCESS9                              0x6609
#define KSZ9477_PORT6_ACL_ACCESS10                             0x660A
#define KSZ9477_PORT6_ACL_ACCESS11                             0x660B
#define KSZ9477_PORT6_ACL_ACCESS12                             0x660C
#define KSZ9477_PORT6_ACL_ACCESS13                             0x660D
#define KSZ9477_PORT6_ACL_ACCESS14                             0x660E
#define KSZ9477_PORT6_ACL_ACCESS15                             0x660F
#define KSZ9477_PORT6_ACL_BYTE_EN_MSB                          0x6610
#define KSZ9477_PORT6_ACL_BYTE_EN_LSB                          0x6611
#define KSZ9477_PORT6_ACL_ACCESS_CTRL0                         0x6612
#define KSZ9477_PORT6_ACL_ACCESS_CTRL1                         0x6613
#define KSZ9477_PORT6_MIRRORING_CTRL                           0x6800
#define KSZ9477_PORT6_PRIO_CTRL                                0x6801
#define KSZ9477_PORT6_IG_MAC_CTRL                              0x6802
#define KSZ9477_PORT6_AUTH_CTRL                                0x6803
#define KSZ9477_PORT6_PTR                                      0x6804
#define KSZ9477_PORT6_PRIO_TO_QUEUE_MAPPING                    0x6808
#define KSZ9477_PORT6_POLICE_CTRL                              0x680C
#define KSZ9477_PORT6_POLICE_QUEUE_RATE                        0x6820
#define KSZ9477_PORT6_POLICE_QUEUE_BURST_SIZE                  0x6824
#define KSZ9477_PORT6_WRED_PKT_MEM_CTRL0                       0x6830
#define KSZ9477_PORT6_WRED_PKT_MEM_CTRL1                       0x6834
#define KSZ9477_PORT6_WRED_QUEUE_CTRL0                         0x6840
#define KSZ9477_PORT6_WRED_QUEUE_CTRL1                         0x6844
#define KSZ9477_PORT6_WRED_QUEUE_PERF_MON_CTRL                 0x6848
#define KSZ9477_PORT6_TX_QUEUE_INDEX                           0x6900
#define KSZ9477_PORT6_TX_QUEUE_PVID                            0x6904
#define KSZ9477_PORT6_TX_QUEUE_CTRL0                           0x6914
#define KSZ9477_PORT6_TX_QUEUE_CTRL1                           0x6915
#define KSZ9477_PORT6_TX_CREDIT_SHAPER_CTRL0                   0x6916
#define KSZ9477_PORT6_TX_CREDIT_SHAPER_CTRL1                   0x6918
#define KSZ9477_PORT6_TX_CREDIT_SHAPER_CTRL2                   0x691A
#define KSZ9477_PORT6_TAS_CTRL                                 0x6920
#define KSZ9477_PORT6_TAS_EVENT_INDEX                          0x6923
#define KSZ9477_PORT6_TAS_EVENT                                0x6924
#define KSZ9477_PORT6_CTRL0                                    0x6A00
#define KSZ9477_PORT6_CTRL1                                    0x6A04
#define KSZ9477_PORT6_CTRL2                                    0x6B00
#define KSZ9477_PORT6_MSTP_PTR                                 0x6B01
#define KSZ9477_PORT6_MSTP_STATE                               0x6B04
#define KSZ9477_PORT6_PTP_RX_LATENCY                           0x6C00
#define KSZ9477_PORT6_PTP_TX_LATENCY                           0x6C02
#define KSZ9477_PORT6_PTP_ASYM_CORRECTION                      0x6C04
#define KSZ9477_PORT6_PTP_XDLY_REQ_TSH                         0x6C08
#define KSZ9477_PORT6_PTP_XDLY_REQ_TSL                         0x6C0A
#define KSZ9477_PORT6_PTP_SYNC_TSH                             0x6C0C
#define KSZ9477_PORT6_PTP_SYNC_TSL                             0x6C0E
#define KSZ9477_PORT6_PTP_PDLY_RESP_TSH                        0x6C10
#define KSZ9477_PORT6_PTP_PDLY_RESP_TSL                        0x6C12
#define KSZ9477_PORT6_PTP_TS_INT_STAT                          0x6C14
#define KSZ9477_PORT6_PTP_TS_INT_EN                            0x6C16
#define KSZ9477_PORT6_PTP_LINK_DELAY                           0x6C18
#define KSZ9477_PORT7_DEFAULT_TAG0                             0x7000
#define KSZ9477_PORT7_DEFAULT_TAG1                             0x7001
#define KSZ9477_PORT7_PME_WOL_EVENT                            0x7013
#define KSZ9477_PORT7_PME_WOL_EN                               0x7017
#define KSZ9477_PORT7_INT_STATUS                               0x701B
#define KSZ9477_PORT7_INT_MASK                                 0x701F
#define KSZ9477_PORT7_OP_CTRL0                                 0x7020
#define KSZ9477_PORT7_STATUS                                   0x7030
#define KSZ9477_PORT7_SGMII_ADDR                               0x7200
#define KSZ9477_PORT7_SGMII_DATA                               0x7206
#define KSZ9477_PORT7_XMII_CTRL0                               0x7300
#define KSZ9477_PORT7_XMII_CTRL1                               0x7301
#define KSZ9477_PORT7_MAC_CTRL0                                0x7400
#define KSZ9477_PORT7_MAC_CTRL1                                0x7401
#define KSZ9477_PORT7_IG_RATE_LIMIT_CTRL                       0x7403
#define KSZ9477_PORT7_PRIO0_IG_LIMIT_CTRL                      0x7410
#define KSZ9477_PORT7_PRIO1_IG_LIMIT_CTRL                      0x7411
#define KSZ9477_PORT7_PRIO2_IG_LIMIT_CTRL                      0x7412
#define KSZ9477_PORT7_PRIO3_IG_LIMIT_CTRL                      0x7413
#define KSZ9477_PORT7_PRIO4_IG_LIMIT_CTRL                      0x7414
#define KSZ9477_PORT7_PRIO5_IG_LIMIT_CTRL                      0x7415
#define KSZ9477_PORT7_PRIO6_IG_LIMIT_CTRL                      0x7416
#define KSZ9477_PORT7_PRIO7_IG_LIMIT_CTRL                      0x7417
#define KSZ9477_PORT7_QUEUE0_EG_LIMIT_CTRL                     0x7420
#define KSZ9477_PORT7_QUEUE1_EG_LIMIT_CTRL                     0x7421
#define KSZ9477_PORT7_QUEUE2_EG_LIMIT_CTRL                     0x7422
#define KSZ9477_PORT7_QUEUE3_EG_LIMIT_CTRL                     0x7423
#define KSZ9477_PORT7_MIB_CTRL_STAT                            0x7500
#define KSZ9477_PORT7_MIB_DATA                                 0x7504
#define KSZ9477_PORT7_ACL_ACCESS0                              0x7600
#define KSZ9477_PORT7_ACL_ACCESS1                              0x7601
#define KSZ9477_PORT7_ACL_ACCESS2                              0x7602
#define KSZ9477_PORT7_ACL_ACCESS3                              0x7603
#define KSZ9477_PORT7_ACL_ACCESS4                              0x7604
#define KSZ9477_PORT7_ACL_ACCESS5                              0x7605
#define KSZ9477_PORT7_ACL_ACCESS6                              0x7606
#define KSZ9477_PORT7_ACL_ACCESS7                              0x7607
#define KSZ9477_PORT7_ACL_ACCESS8                              0x7608
#define KSZ9477_PORT7_ACL_ACCESS9                              0x7609
#define KSZ9477_PORT7_ACL_ACCESS10                             0x760A
#define KSZ9477_PORT7_ACL_ACCESS11                             0x760B
#define KSZ9477_PORT7_ACL_ACCESS12                             0x760C
#define KSZ9477_PORT7_ACL_ACCESS13                             0x760D
#define KSZ9477_PORT7_ACL_ACCESS14                             0x760E
#define KSZ9477_PORT7_ACL_ACCESS15                             0x760F
#define KSZ9477_PORT7_ACL_BYTE_EN_MSB                          0x7610
#define KSZ9477_PORT7_ACL_BYTE_EN_LSB                          0x7611
#define KSZ9477_PORT7_ACL_ACCESS_CTRL0                         0x7612
#define KSZ9477_PORT7_ACL_ACCESS_CTRL1                         0x7613
#define KSZ9477_PORT7_MIRRORING_CTRL                           0x7800
#define KSZ9477_PORT7_PRIO_CTRL                                0x7801
#define KSZ9477_PORT7_IG_MAC_CTRL                              0x7802
#define KSZ9477_PORT7_AUTH_CTRL                                0x7803
#define KSZ9477_PORT7_PTR                                      0x7804
#define KSZ9477_PORT7_PRIO_TO_QUEUE_MAPPING                    0x7808
#define KSZ9477_PORT7_POLICE_CTRL                              0x780C
#define KSZ9477_PORT7_POLICE_QUEUE_RATE                        0x7820
#define KSZ9477_PORT7_POLICE_QUEUE_BURST_SIZE                  0x7824
#define KSZ9477_PORT7_WRED_PKT_MEM_CTRL0                       0x7830
#define KSZ9477_PORT7_WRED_PKT_MEM_CTRL1                       0x7834
#define KSZ9477_PORT7_WRED_QUEUE_CTRL0                         0x7840
#define KSZ9477_PORT7_WRED_QUEUE_CTRL1                         0x7844
#define KSZ9477_PORT7_WRED_QUEUE_PERF_MON_CTRL                 0x7848
#define KSZ9477_PORT7_TX_QUEUE_INDEX                           0x7900
#define KSZ9477_PORT7_TX_QUEUE_PVID                            0x7904
#define KSZ9477_PORT7_TX_QUEUE_CTRL0                           0x7914
#define KSZ9477_PORT7_TX_QUEUE_CTRL1                           0x7915
#define KSZ9477_PORT7_TX_CREDIT_SHAPER_CTRL0                   0x7916
#define KSZ9477_PORT7_TX_CREDIT_SHAPER_CTRL1                   0x7918
#define KSZ9477_PORT7_TX_CREDIT_SHAPER_CTRL2                   0x791A
#define KSZ9477_PORT7_TAS_CTRL                                 0x7920
#define KSZ9477_PORT7_TAS_EVENT_INDEX                          0x7923
#define KSZ9477_PORT7_TAS_EVENT                                0x7924
#define KSZ9477_PORT7_CTRL0                                    0x7A00
#define KSZ9477_PORT7_CTRL1                                    0x7A04
#define KSZ9477_PORT7_CTRL2                                    0x7B00
#define KSZ9477_PORT7_MSTP_PTR                                 0x7B01
#define KSZ9477_PORT7_MSTP_STATE                               0x7B04
#define KSZ9477_PORT7_PTP_RX_LATENCY                           0x7C00
#define KSZ9477_PORT7_PTP_TX_LATENCY                           0x7C02
#define KSZ9477_PORT7_PTP_ASYM_CORRECTION                      0x7C04
#define KSZ9477_PORT7_PTP_XDLY_REQ_TSH                         0x7C08
#define KSZ9477_PORT7_PTP_XDLY_REQ_TSL                         0x7C0A
#define KSZ9477_PORT7_PTP_SYNC_TSH                             0x7C0C
#define KSZ9477_PORT7_PTP_SYNC_TSL                             0x7C0E
#define KSZ9477_PORT7_PTP_PDLY_RESP_TSH                        0x7C10
#define KSZ9477_PORT7_PTP_PDLY_RESP_TSL                        0x7C12
#define KSZ9477_PORT7_PTP_TS_INT_STAT                          0x7C14
#define KSZ9477_PORT7_PTP_TS_INT_EN                            0x7C16
#define KSZ9477_PORT7_PTP_LINK_DELAY                           0x7C18

//KSZ9477 Switch register access macros
#define KSZ9477_PORTn_DEFAULT_TAG0(port)                       (0x0000 + ((port) * 0x1000))
#define KSZ9477_PORTn_DEFAULT_TAG1(port)                       (0x0001 + ((port) * 0x1000))
#define KSZ9477_PORTn_PME_WOL_EVENT(port)                      (0x0013 + ((port) * 0x1000))
#define KSZ9477_PORTn_PME_WOL_EN(port)                         (0x0017 + ((port) * 0x1000))
#define KSZ9477_PORTn_INT_STATUS(port)                         (0x001B + ((port) * 0x1000))
#define KSZ9477_PORTn_INT_MASK(port)                           (0x001F + ((port) * 0x1000))
#define KSZ9477_PORTn_OP_CTRL0(port)                           (0x0020 + ((port) * 0x1000))
#define KSZ9477_PORTn_STATUS(port)                             (0x0030 + ((port) * 0x1000))
#define KSZ9477_PORTn_XMII_CTRL0(port)                         (0x0300 + ((port) * 0x1000))
#define KSZ9477_PORTn_XMII_CTRL1(port)                         (0x0301 + ((port) * 0x1000))
#define KSZ9477_PORTn_MAC_CTRL0(port)                          (0x0400 + ((port) * 0x1000))
#define KSZ9477_PORTn_MAC_CTRL1(port)                          (0x0401 + ((port) * 0x1000))
#define KSZ9477_PORTn_IG_RATE_LIMIT_CTRL(port)                 (0x0403 + ((port) * 0x1000))
#define KSZ9477_PORTn_PRIO0_IG_LIMIT_CTRL(port)                (0x0410 + ((port) * 0x1000))
#define KSZ9477_PORTn_PRIO1_IG_LIMIT_CTRL(port)                (0x0411 + ((port) * 0x1000))
#define KSZ9477_PORTn_PRIO2_IG_LIMIT_CTRL(port)                (0x0412 + ((port) * 0x1000))
#define KSZ9477_PORTn_PRIO3_IG_LIMIT_CTRL(port)                (0x0413 + ((port) * 0x1000))
#define KSZ9477_PORTn_PRIO4_IG_LIMIT_CTRL(port)                (0x0414 + ((port) * 0x1000))
#define KSZ9477_PORTn_PRIO5_IG_LIMIT_CTRL(port)                (0x0415 + ((port) * 0x1000))
#define KSZ9477_PORTn_PRIO6_IG_LIMIT_CTRL(port)                (0x0416 + ((port) * 0x1000))
#define KSZ9477_PORTn_PRIO7_IG_LIMIT_CTRL(port)                (0x0417 + ((port) * 0x1000))
#define KSZ9477_PORTn_QUEUE0_EG_LIMIT_CTRL(port)               (0x0420 + ((port) * 0x1000))
#define KSZ9477_PORTn_QUEUE1_EG_LIMIT_CTRL(port)               (0x0421 + ((port) * 0x1000))
#define KSZ9477_PORTn_QUEUE2_EG_LIMIT_CTRL(port)               (0x0422 + ((port) * 0x1000))
#define KSZ9477_PORTn_QUEUE3_EG_LIMIT_CTRL(port)               (0x0423 + ((port) * 0x1000))
#define KSZ9477_PORTn_MIB_CTRL_STAT(port)                      (0x0500 + ((port) * 0x1000))
#define KSZ9477_PORTn_MIB_DATA(port)                           (0x0504 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS0(port)                        (0x0600 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS1(port)                        (0x0601 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS2(port)                        (0x0602 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS3(port)                        (0x0603 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS4(port)                        (0x0604 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS5(port)                        (0x0605 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS6(port)                        (0x0606 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS7(port)                        (0x0607 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS8(port)                        (0x0608 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS9(port)                        (0x0609 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS10(port)                       (0x060A + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS11(port)                       (0x060B + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS12(port)                       (0x060C + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS13(port)                       (0x060D + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS14(port)                       (0x060E + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS15(port)                       (0x060F + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_BYTE_EN_MSB(port)                    (0x0610 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_BYTE_EN_LSB(port)                    (0x0611 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS_CTRL0(port)                   (0x0612 + ((port) * 0x1000))
#define KSZ9477_PORTn_ACL_ACCESS_CTRL1(port)                   (0x0613 + ((port) * 0x1000))
#define KSZ9477_PORTn_MIRRORING_CTRL(port)                     (0x0800 + ((port) * 0x1000))
#define KSZ9477_PORTn_PRIO_CTRL(port)                          (0x0801 + ((port) * 0x1000))
#define KSZ9477_PORTn_IG_MAC_CTRL(port)                        (0x0802 + ((port) * 0x1000))
#define KSZ9477_PORTn_AUTH_CTRL(port)                          (0x0803 + ((port) * 0x1000))
#define KSZ9477_PORTn_PTR(port)                                (0x0804 + ((port) * 0x1000))
#define KSZ9477_PORTn_PRIO_TO_QUEUE_MAPPING(port)              (0x0808 + ((port) * 0x1000))
#define KSZ9477_PORTn_POLICE_CTRL(port)                        (0x080C + ((port) * 0x1000))
#define KSZ9477_PORTn_POLICE_QUEUE_RATE(port)                  (0x0820 + ((port) * 0x1000))
#define KSZ9477_PORTn_POLICE_QUEUE_BURST_SIZE(port)            (0x0824 + ((port) * 0x1000))
#define KSZ9477_PORTn_WRED_PKT_MEM_CTRL0(port)                 (0x0830 + ((port) * 0x1000))
#define KSZ9477_PORTn_WRED_PKT_MEM_CTRL1(port)                 (0x0834 + ((port) * 0x1000))
#define KSZ9477_PORTn_WRED_QUEUE_CTRL0(port)                   (0x0840 + ((port) * 0x1000))
#define KSZ9477_PORTn_WRED_QUEUE_CTRL1(port)                   (0x0844 + ((port) * 0x1000))
#define KSZ9477_PORTn_WRED_QUEUE_PERF_MON_CTRL(port)           (0x0848 + ((port) * 0x1000))
#define KSZ9477_PORTn_TX_QUEUE_INDEX(port)                     (0x0900 + ((port) * 0x1000))
#define KSZ9477_PORTn_TX_QUEUE_PVID(port)                      (0x0904 + ((port) * 0x1000))
#define KSZ9477_PORTn_TX_QUEUE_CTRL0(port)                     (0x0914 + ((port) * 0x1000))
#define KSZ9477_PORTn_TX_QUEUE_CTRL1(port)                     (0x0915 + ((port) * 0x1000))
#define KSZ9477_PORTn_TX_CREDIT_SHAPER_CTRL0(port)             (0x0916 + ((port) * 0x1000))
#define KSZ9477_PORTn_TX_CREDIT_SHAPER_CTRL1(port)             (0x0918 + ((port) * 0x1000))
#define KSZ9477_PORTn_TX_CREDIT_SHAPER_CTRL2(port)             (0x091A + ((port) * 0x1000))
#define KSZ9477_PORTn_TAS_CTRL(port)                           (0x0920 + ((port) * 0x1000))
#define KSZ9477_PORTn_TAS_EVENT_INDEX(port)                    (0x0923 + ((port) * 0x1000))
#define KSZ9477_PORTn_TAS_EVENT(port)                          (0x0924 + ((port) * 0x1000))
#define KSZ9477_PORTn_CTRL0(port)                              (0x0A00 + ((port) * 0x1000))
#define KSZ9477_PORTn_CTRL1(port)                              (0x0A04 + ((port) * 0x1000))
#define KSZ9477_PORTn_CTRL2(port)                              (0x0B00 + ((port) * 0x1000))
#define KSZ9477_PORTn_MSTP_PTR(port)                           (0x0B01 + ((port) * 0x1000))
#define KSZ9477_PORTn_MSTP_STATE(port)                         (0x0B04 + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_RX_LATENCY(port)                     (0x0C00 + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_TX_LATENCY(port)                     (0x0C02 + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_ASYM_CORRECTION(port)                (0x0C04 + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_XDLY_REQ_TSH(port)                   (0x0C08 + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_XDLY_REQ_TSL(port)                   (0x0C0A + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_SYNC_TSH(port)                       (0x0C0C + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_SYNC_TSL(port)                       (0x0C0E + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_PDLY_RESP_TSH(port)                  (0x0C10 + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_PDLY_RESP_TSL(port)                  (0x0C12 + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_TS_INT_STAT(port)                    (0x0C14 + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_TS_INT_EN(port)                      (0x0C16 + ((port) * 0x1000))
#define KSZ9477_PORTn_PTP_LINK_DELAY(port)                     (0x0C18 + ((port) * 0x1000))
#define KSZ9477_PORTn_ETH_PHY_REG(port, addr)                  (0x0100 + ((port) * 0x1000) + ((addr) * 2))

//PHY Basic Control register
#define KSZ9477_BMCR_RESET                                     0x8000
#define KSZ9477_BMCR_LOOPBACK                                  0x4000
#define KSZ9477_BMCR_SPEED_SEL_LSB                             0x2000
#define KSZ9477_BMCR_AN_EN                                     0x1000
#define KSZ9477_BMCR_POWER_DOWN                                0x0800
#define KSZ9477_BMCR_ISOLATE                                   0x0400
#define KSZ9477_BMCR_RESTART_AN                                0x0200
#define KSZ9477_BMCR_DUPLEX_MODE                               0x0100
#define KSZ9477_BMCR_COL_TEST                                  0x0080
#define KSZ9477_BMCR_SPEED_SEL_MSB                             0x0040

//PHY Basic Status register
#define KSZ9477_BMSR_100BT4                                    0x8000
#define KSZ9477_BMSR_100BTX_FD                                 0x4000
#define KSZ9477_BMSR_100BTX_HD                                 0x2000
#define KSZ9477_BMSR_10BT_FD                                   0x1000
#define KSZ9477_BMSR_10BT_HD                                   0x0800
#define KSZ9477_BMSR_EXTENDED_STATUS                           0x0100
#define KSZ9477_BMSR_MF_PREAMBLE_SUPPR                         0x0040
#define KSZ9477_BMSR_AN_COMPLETE                               0x0020
#define KSZ9477_BMSR_REMOTE_FAULT                              0x0010
#define KSZ9477_BMSR_AN_CAPABLE                                0x0008
#define KSZ9477_BMSR_LINK_STATUS                               0x0004
#define KSZ9477_BMSR_JABBER_DETECT                             0x0002
#define KSZ9477_BMSR_EXTENDED_CAPABLE                          0x0001

//PHY ID High register
#define KSZ9477_PHYID1_DEFAULT                                 0x0022

//PHY ID Low register
#define KSZ9477_PHYID2_DEFAULT                                 0x1631

//PHY Auto-Negotiation Advertisement register
#define KSZ9477_ANAR_NEXT_PAGE                                 0x8000
#define KSZ9477_ANAR_REMOTE_FAULT                              0x2000
#define KSZ9477_ANAR_PAUSE                                     0x0C00
#define KSZ9477_ANAR_100BT4                                    0x0200
#define KSZ9477_ANAR_100BTX_FD                                 0x0100
#define KSZ9477_ANAR_100BTX_HD                                 0x0080
#define KSZ9477_ANAR_10BT_FD                                   0x0040
#define KSZ9477_ANAR_10BT_HD                                   0x0020
#define KSZ9477_ANAR_SELECTOR                                  0x001F
#define KSZ9477_ANAR_SELECTOR_DEFAULT                          0x0001

//PHY Auto-Negotiation Link Partner Ability register
#define KSZ9477_ANLPAR_NEXT_PAGE                               0x8000
#define KSZ9477_ANLPAR_ACK                                     0x4000
#define KSZ9477_ANLPAR_REMOTE_FAULT                            0x2000
#define KSZ9477_ANLPAR_PAUSE                                   0x0C00
#define KSZ9477_ANLPAR_100BT4                                  0x0200
#define KSZ9477_ANLPAR_100BTX_FD                               0x0100
#define KSZ9477_ANLPAR_100BTX_HD                               0x0080
#define KSZ9477_ANLPAR_10BT_FD                                 0x0040
#define KSZ9477_ANLPAR_10BT_HD                                 0x0020
#define KSZ9477_ANLPAR_SELECTOR                                0x001F
#define KSZ9477_ANLPAR_SELECTOR_DEFAULT                        0x0001

//PHY Auto-Negotiation Expansion Status register
#define KSZ9477_ANER_PAR_DETECT_FAULT                          0x0010
#define KSZ9477_ANER_LP_NEXT_PAGE_ABLE                         0x0008
#define KSZ9477_ANER_NEXT_PAGE_ABLE                            0x0004
#define KSZ9477_ANER_PAGE_RECEIVED                             0x0002
#define KSZ9477_ANER_LP_AN_ABLE                                0x0001

//PHY Auto-Negotiation Next Page register
#define KSZ9477_ANNPR_NEXT_PAGE                                0x8000
#define KSZ9477_ANNPR_MSG_PAGE                                 0x2000
#define KSZ9477_ANNPR_ACK2                                     0x1000
#define KSZ9477_ANNPR_TOGGLE                                   0x0800
#define KSZ9477_ANNPR_MESSAGE                                  0x07FF

//PHY Auto-Negotiation Link Partner Next Page Ability register
#define KSZ9477_ANLPNPR_NEXT_PAGE                              0x8000
#define KSZ9477_ANLPNPR_ACK                                    0x4000
#define KSZ9477_ANLPNPR_MSG_PAGE                               0x2000
#define KSZ9477_ANLPNPR_ACK2                                   0x1000
#define KSZ9477_ANLPNPR_TOGGLE                                 0x0800
#define KSZ9477_ANLPNPR_MESSAGE                                0x07FF

//PHY 1000BASE-T Control register
#define KSZ9477_GBCR_TEST_MODE                                 0xE000
#define KSZ9477_GBCR_MS_MAN_CONF_EN                            0x1000
#define KSZ9477_GBCR_MS_MAN_CONF_VAL                           0x0800
#define KSZ9477_GBCR_PORT_TYPE                                 0x0400
#define KSZ9477_GBCR_1000BT_FD                                 0x0200
#define KSZ9477_GBCR_1000BT_HD                                 0x0100

//PHY 1000BASE-T Status register
#define KSZ9477_GBSR_MS_CONF_FAULT                             0x8000
#define KSZ9477_GBSR_MS_CONF_RES                               0x4000
#define KSZ9477_GBSR_LOCAL_RECEIVER_STATUS                     0x2000
#define KSZ9477_GBSR_REMOTE_RECEIVER_STATUS                    0x1000
#define KSZ9477_GBSR_LP_1000BT_FD                              0x0800
#define KSZ9477_GBSR_LP_1000BT_HD                              0x0400
#define KSZ9477_GBSR_IDLE_ERR_COUNT                            0x00FF

//PHY MMD Setup register
#define KSZ9477_MMDACR_FUNC                                    0xC000
#define KSZ9477_MMDACR_FUNC_ADDR                               0x0000
#define KSZ9477_MMDACR_FUNC_DATA_NO_POST_INC                   0x4000
#define KSZ9477_MMDACR_FUNC_DATA_POST_INC_RW                   0x8000
#define KSZ9477_MMDACR_FUNC_DATA_POST_INC_W                    0xC000
#define KSZ9477_MMDACR_DEVAD                                   0x001F

//PHY Extended Status register
#define KSZ9477_GBESR_1000BX_FD                                0x8000
#define KSZ9477_GBESR_1000BX_HD                                0x4000
#define KSZ9477_GBESR_1000BT_FD                                0x2000
#define KSZ9477_GBESR_1000BT_HD                                0x1000

//PHY Remote Loopback register
#define KSZ9477_RLB_REMOTE_LOOPBACK                            0x0100

//PHY LinkMD register
#define KSZ9477_LINKMD_TEST_EN                                 0x8000
#define KSZ9477_LINKMD_PAIR                                    0x3000
#define KSZ9477_LINKMD_PAIR_A                                  0x0000
#define KSZ9477_LINKMD_PAIR_B                                  0x1000
#define KSZ9477_LINKMD_PAIR_C                                  0x2000
#define KSZ9477_LINKMD_PAIR_D                                  0x3000
#define KSZ9477_LINKMD_STATUS                                  0x0300
#define KSZ9477_LINKMD_STATUS_NORMAL                           0x0000
#define KSZ9477_LINKMD_STATUS_OPEN                             0x0100
#define KSZ9477_LINKMD_STATUS_SHORT                            0x0200
#define KSZ9477_LINKMD_RESULT                                  0x00FF

//PHY Digital PMA/PCS Status register
#define KSZ9477_DPMAPCSS_1000BT_LINK_STATUS                    0x0002
#define KSZ9477_DPMAPCSS_100BTX_LINK_STATUS                    0x0001

//Port Interrupt Control/Status register
#define KSZ9477_ICSR_JABBER_IE                                 0x8000
#define KSZ9477_ICSR_RECEIVE_ERROR_IE                          0x4000
#define KSZ9477_ICSR_PAGE_RECEIVED_IE                          0x2000
#define KSZ9477_ICSR_PAR_DETECT_FAULT_IE                       0x1000
#define KSZ9477_ICSR_LP_ACK_IE                                 0x0800
#define KSZ9477_ICSR_LINK_DOWN_IE                              0x0400
#define KSZ9477_ICSR_REMOTE_FAULT_IE                           0x0200
#define KSZ9477_ICSR_LINK_UP_IE                                0x0100
#define KSZ9477_ICSR_JABBER_IF                                 0x0080
#define KSZ9477_ICSR_RECEIVE_ERROR_IF                          0x0040
#define KSZ9477_ICSR_PAGE_RECEIVED_IF                          0x0020
#define KSZ9477_ICSR_PAR_DETECT_FAULT_IF                       0x0010
#define KSZ9477_ICSR_LP_ACK_IF                                 0x0008
#define KSZ9477_ICSR_LINK_DOWN_IF                              0x0004
#define KSZ9477_ICSR_REMOTE_FAULT_IF                           0x0002
#define KSZ9477_ICSR_LINK_UP_IF                                0x0001

//PHY Auto MDI/MDI-X register
#define KSZ9477_AUTOMDI_MDI_SET                                0x0080
#define KSZ9477_AUTOMDI_SWAP_OFF                               0x0040

//PHY Control register
#define KSZ9477_PHYCON_JABBER_EN                               0x0200
#define KSZ9477_PHYCON_SPEED_1000BT                            0x0040
#define KSZ9477_PHYCON_SPEED_100BTX                            0x0020
#define KSZ9477_PHYCON_SPEED_10BT                              0x0010
#define KSZ9477_PHYCON_DUPLEX_STATUS                           0x0008
#define KSZ9477_PHYCON_1000BT_MS_STATUS                        0x0004

//MMD Signal Quality Channel A register
#define KSZ9477_MMD_SIGNAL_QUALITY_CH_A_QUALITY_INDICATOR      0x7F00

//MMD Signal Quality Channel B register
#define KSZ9477_MMD_SIGNAL_QUALITY_CH_B_QUALITY_INDICATOR      0x7F00

//MMD Signal Quality Channel C register
#define KSZ9477_MMD_SIGNAL_QUALITY_CH_C_QUALITY_INDICATOR      0x7F00

//MMD Signal Quality Channel D register
#define KSZ9477_MMD_SIGNAL_QUALITY_CH_D_QUALITY_INDICATOR      0x7F00

//MMD LED Mode register
#define KSZ9477_MMD_LED_MODE_LED_MODE                          0x0010
#define KSZ9477_MMD_LED_MODE_LED_MODE_TRI_COLOR_DUAL           0x0000
#define KSZ9477_MMD_LED_MODE_LED_MODE_SINGLE                   0x0010
#define KSZ9477_MMD_LED_MODE_RESERVED                          0x000F
#define KSZ9477_MMD_LED_MODE_RESERVED_DEFAULT                  0x0001

//MMD EEE Advertisement register
#define KSZ9477_MMD_EEE_ADV_1000BT_EEE_EN                      0x0004
#define KSZ9477_MMD_EEE_ADV_100BT_EEE_EN                       0x0002

//Global Chip ID 0 register
#define KSZ9477_CHIP_ID0_DEFAULT                               0x00

//Global Chip ID 1 register
#define KSZ9477_CHIP_ID1_DEFAULT                               0x94

//Global Chip ID 2 register
#define KSZ9477_CHIP_ID2_DEFAULT                               0x77

//Global Chip ID 3 register
#define KSZ9477_CHIP_ID3_REVISION_ID                           0xF0
#define KSZ9477_CHIP_ID3_GLOBAL_SOFT_RESET                     0x01

//PME Pin Control register
#define KSZ9477_PME_PIN_CTRL_PME_PIN_OUT_EN                    0x02
#define KSZ9477_PME_PIN_CTRL_PME_PIN_OUT_POL                   0x01

//Global Interrupt Status register
#define KSZ9477_GLOBAL_INT_STAT_LUE                            0x80000000
#define KSZ9477_GLOBAL_INT_STAT_GPIO_TRIG_TS_UNIT              0x40000000

//Global Interrupt Mask register
#define KSZ9477_GLOBAL_INT_MASK_LUE                            0x80000000
#define KSZ9477_GLOBAL_INT_MASK_GPIO_TRIG_TS_UNIT              0x40000000

//Global Port Interrupt Status register
#define KSZ9477_GLOBAL_PORT_INT_STAT_PORT7                     0x00000040
#define KSZ9477_GLOBAL_PORT_INT_STAT_PORT6                     0x00000020
#define KSZ9477_GLOBAL_PORT_INT_STAT_PORT5                     0x00000010
#define KSZ9477_GLOBAL_PORT_INT_STAT_PORT4                     0x00000008
#define KSZ9477_GLOBAL_PORT_INT_STAT_PORT3                     0x00000004
#define KSZ9477_GLOBAL_PORT_INT_STAT_PORT2                     0x00000002
#define KSZ9477_GLOBAL_PORT_INT_STAT_PORT1                     0x00000001

//Global Port Interrupt Mask register
#define KSZ9477_GLOBAL_PORT_INT_MASK_PORT7                     0x00000040
#define KSZ9477_GLOBAL_PORT_INT_MASK_PORT6                     0x00000020
#define KSZ9477_GLOBAL_PORT_INT_MASK_PORT5                     0x00000010
#define KSZ9477_GLOBAL_PORT_INT_MASK_PORT4                     0x00000008
#define KSZ9477_GLOBAL_PORT_INT_MASK_PORT3                     0x00000004
#define KSZ9477_GLOBAL_PORT_INT_MASK_PORT2                     0x00000002
#define KSZ9477_GLOBAL_PORT_INT_MASK_PORT1                     0x00000001

//Switch Operation register
#define KSZ9477_SWITCH_OP_DOUBLE_TAG_EN                        0x80
#define KSZ9477_SWITCH_OP_SOFT_HARD_RESET                      0x02
#define KSZ9477_SWITCH_OP_START_SWITCH                         0x01

//Switch Lookup Engine Control 0 register
#define KSZ9477_SWITCH_LUE_CTRL0_VLAN_EN                       0x80
#define KSZ9477_SWITCH_LUE_CTRL0_DROP_INVALID_VID              0x40
#define KSZ9477_SWITCH_LUE_CTRL0_AGE_COUNT                     0x38
#define KSZ9477_SWITCH_LUE_CTRL0_AGE_COUNT_DEFAULT             0x20
#define KSZ9477_SWITCH_LUE_CTRL0_RESERVED_MCAST_LOOKUP_EN      0x04
#define KSZ9477_SWITCH_LUE_CTRL0_HASH_OPTION                   0x03
#define KSZ9477_SWITCH_LUE_CTRL0_HASH_OPTION_NONE              0x00
#define KSZ9477_SWITCH_LUE_CTRL0_HASH_OPTION_CRC               0x01
#define KSZ9477_SWITCH_LUE_CTRL0_HASH_OPTION_XOR               0x02

//Switch Lookup Engine Control 1 register
#define KSZ9477_SWITCH_LUE_CTRL1_UNICAST_LEARNING_DIS          0x80
#define KSZ9477_SWITCH_LUE_CTRL1_SELF_ADDR_FILT                0x40
#define KSZ9477_SWITCH_LUE_CTRL1_FLUSH_ALU_TABLE               0x20
#define KSZ9477_SWITCH_LUE_CTRL1_FLUSH_MSTP_ENTRIES            0x10
#define KSZ9477_SWITCH_LUE_CTRL1_MCAST_SRC_ADDR_FILT           0x08
#define KSZ9477_SWITCH_LUE_CTRL1_AGING_EN                      0x04
#define KSZ9477_SWITCH_LUE_CTRL1_FAST_AGING                    0x02
#define KSZ9477_SWITCH_LUE_CTRL1_LINK_DOWN_FLUSH               0x01

//Switch Lookup Engine Control 2 register
#define KSZ9477_SWITCH_LUE_CTRL2_DOUBLE_TAG_MCAST_TRAP         0x40
#define KSZ9477_SWITCH_LUE_CTRL2_DYNAMIC_ENTRY_EG_VLAN_FILT    0x20
#define KSZ9477_SWITCH_LUE_CTRL2_STATIC_ENTRY_EG_VLAN_FILT     0x10
#define KSZ9477_SWITCH_LUE_CTRL2_FLUSH_OPTION                  0x0C
#define KSZ9477_SWITCH_LUE_CTRL2_FLUSH_OPTION_NONE             0x00
#define KSZ9477_SWITCH_LUE_CTRL2_FLUSH_OPTION_DYNAMIC          0x04
#define KSZ9477_SWITCH_LUE_CTRL2_FLUSH_OPTION_STATIC           0x08
#define KSZ9477_SWITCH_LUE_CTRL2_FLUSH_OPTION_BOTH             0x0C
#define KSZ9477_SWITCH_LUE_CTRL2_MAC_ADDR_PRIORITY             0x03

//Switch Lookup Engine Control 3 register
#define KSZ9477_SWITCH_LUE_CTRL3_AGE_PERIOD                    0xFF
#define KSZ9477_SWITCH_LUE_CTRL3_AGE_PERIOD_DEFAULT            0x4B

//Address Lookup Table Interrupt register
#define KSZ9477_ALU_TABLE_INT_LEARN_FAIL                       0x04
#define KSZ9477_ALU_TABLE_INT_ALMOST_FULL                      0x02
#define KSZ9477_ALU_TABLE_INT_WRITE_FAIL                       0x01

//Address Lookup Table Mask register
#define KSZ9477_ALU_TABLE_MASK_LEARN_FAIL                      0x04
#define KSZ9477_ALU_TABLE_MASK_ALMOST_FULL                     0x02
#define KSZ9477_ALU_TABLE_MASK_WRITE_FAIL                      0x01

//Address Lookup Table Entry Index 0 register
#define KSZ9477_ALU_TABLE_ENTRY_INDEX0_ALMOST_FULL_ENTRY_INDEX 0x0FFF
#define KSZ9477_ALU_TABLE_ENTRY_INDEX0_FAIL_WRITE_INDEX        0x03FF

//Address Lookup Table Entry Index 1 register
#define KSZ9477_ALU_TABLE_ENTRY_INDEX1_FAIL_LEARN_INDEX        0x03FF

//Address Lookup Table Entry Index 2 register
#define KSZ9477_ALU_TABLE_ENTRY_INDEX2_CPU_ACCESS_INDEX        0x03FF

//Unknown Unicast Control register
#define KSZ9477_UNKNOWN_UNICAST_CTRL_FWD                       0x80000000
#define KSZ9477_UNKNOWN_UNICAST_CTRL_FWD_MAP                   0x0000007F
#define KSZ9477_UNKNOWN_UNICAST_CTRL_FWD_MAP_PORT1             0x00000001
#define KSZ9477_UNKNOWN_UNICAST_CTRL_FWD_MAP_PORT2             0x00000002
#define KSZ9477_UNKNOWN_UNICAST_CTRL_FWD_MAP_PORT3             0x00000004
#define KSZ9477_UNKNOWN_UNICAST_CTRL_FWD_MAP_PORT4             0x00000008
#define KSZ9477_UNKNOWN_UNICAST_CTRL_FWD_MAP_PORT5             0x00000010
#define KSZ9477_UNKNOWN_UNICAST_CTRL_FWD_MAP_PORT6             0x00000020
#define KSZ9477_UNKNOWN_UNICAST_CTRL_FWD_MAP_PORT7             0x00000040
#define KSZ9477_UNKNOWN_UNICAST_CTRL_FWD_MAP_ALL               0x0000007F

//Unknown Multicast Control register
#define KSZ9477_UNKONWN_MULTICAST_CTRL_FWD                     0x80000000
#define KSZ9477_UNKONWN_MULTICAST_CTRL_FWD_MAP                 0x0000007F
#define KSZ9477_UNKONWN_MULTICAST_CTRL_FWD_MAP_PORT1           0x00000001
#define KSZ9477_UNKONWN_MULTICAST_CTRL_FWD_MAP_PORT2           0x00000002
#define KSZ9477_UNKONWN_MULTICAST_CTRL_FWD_MAP_PORT3           0x00000004
#define KSZ9477_UNKONWN_MULTICAST_CTRL_FWD_MAP_PORT4           0x00000008
#define KSZ9477_UNKONWN_MULTICAST_CTRL_FWD_MAP_PORT5           0x00000010
#define KSZ9477_UNKONWN_MULTICAST_CTRL_FWD_MAP_PORT6           0x00000020
#define KSZ9477_UNKONWN_MULTICAST_CTRL_FWD_MAP_PORT7           0x00000040
#define KSZ9477_UNKONWN_MULTICAST_CTRL_FWD_MAP_ALL             0x0000007F

//Unknown VLAN ID Control register
#define KSZ9477_UNKNOWN_VLAN_ID_CTRL_FWD                       0x80000000
#define KSZ9477_UNKNOWN_VLAN_ID_CTRL_FWD_MAP                   0x0000007F
#define KSZ9477_UNKNOWN_VLAN_ID_CTRL_FWD_MAP_PORT1             0x00000001
#define KSZ9477_UNKNOWN_VLAN_ID_CTRL_FWD_MAP_PORT2             0x00000002
#define KSZ9477_UNKNOWN_VLAN_ID_CTRL_FWD_MAP_PORT3             0x00000004
#define KSZ9477_UNKNOWN_VLAN_ID_CTRL_FWD_MAP_PORT4             0x00000008
#define KSZ9477_UNKNOWN_VLAN_ID_CTRL_FWD_MAP_PORT5             0x00000010
#define KSZ9477_UNKNOWN_VLAN_ID_CTRL_FWD_MAP_PORT6             0x00000020
#define KSZ9477_UNKNOWN_VLAN_ID_CTRL_FWD_MAP_PORT7             0x00000040
#define KSZ9477_UNKNOWN_VLAN_ID_CTRL_FWD_MAP_ALL               0x0000007F

//Switch MAC Control 0 register
#define KSZ9477_SWITCH_MAC_CTRL0_ALT_BACK_OFF_MODE             0x80
#define KSZ9477_SWITCH_MAC_CTRL0_FRAME_LEN_CHECK_EN            0x08
#define KSZ9477_SWITCH_MAC_CTRL0_FLOW_CTRL_PKT_DROP_MODE       0x02
#define KSZ9477_SWITCH_MAC_CTRL0_AGGRESSIVE_BACK_OFF_EN        0x01

//Switch MAC Control 1 register
#define KSZ9477_SWITCH_MAC_CTRL1_MCAST_STORM_PROTECT_DIS       0x40
#define KSZ9477_SWITCH_MAC_CTRL1_BACK_PRESSURE_MODE            0x20
#define KSZ9477_SWITCH_MAC_CTRL1_FLOW_CTRL_FAIR_MODE           0x10
#define KSZ9477_SWITCH_MAC_CTRL1_NO_EXCESSIVE_COL_DROP         0x08
#define KSZ9477_SWITCH_MAC_CTRL1_JUMBO_PACKET_SUPPORT          0x04
#define KSZ9477_SWITCH_MAC_CTRL1_MAX_PACKET_SIZE_CHECK_DIS     0x02
#define KSZ9477_SWITCH_MAC_CTRL1_PASS_SHORT_PACKET             0x01

//Switch MAC Control 2 register
#define KSZ9477_SWITCH_MAC_CTRL2_NULL_VID_REPLACEMENT          0x08
#define KSZ9477_SWITCH_MAC_CTRL2_BCAST_STORM_PROTECT_RATE_MSB  0x07

//Switch MAC Control 3 register
#define KSZ9477_SWITCH_MAC_CTRL3_BCAST_STORM_PROTECT_RATE_LSB  0xFF

//Switch MAC Control 4 register
#define KSZ9477_SWITCH_MAC_CTRL4_PASS_FLOW_CTRL_PKT            0x01

//Switch MAC Control 5 register
#define KSZ9477_SWITCH_MAC_CTRL5_IG_RATE_LIMIT_PERIOD          0x30
#define KSZ9477_SWITCH_MAC_CTRL5_IG_RATE_LIMIT_PERIOD_16MS     0x00
#define KSZ9477_SWITCH_MAC_CTRL5_IG_RATE_LIMIT_PERIOD_64MS     0x10
#define KSZ9477_SWITCH_MAC_CTRL5_IG_RATE_LIMIT_PERIOD_256MS    0x20
#define KSZ9477_SWITCH_MAC_CTRL5_QUEUE_BASED_EG_RATE_LIMITE_EN 0x08

//Switch MIB Control register
#define KSZ9477_SWITCH_MIB_CTRL_FLUSH                          0x80
#define KSZ9477_SWITCH_MIB_CTRL_FREEZE                         0x40

//Global Port Mirroring and Snooping Control register
#define KSZ9477_GLOBAL_PORT_MIRROR_SNOOP_CTRL_IGMP_SNOOP_EN    0x40
#define KSZ9477_GLOBAL_PORT_MIRROR_SNOOP_CTRL_MLD_SNOOP_OPT    0x08
#define KSZ9477_GLOBAL_PORT_MIRROR_SNOOP_CTRL_MLD_SNOOP_EN     0x04
#define KSZ9477_GLOBAL_PORT_MIRROR_SNOOP_CTRL_SNIFF_MODE_SEL   0x01

//VLAN Table Entry 0 register
#define KSZ9477_VLAN_TABLE_ENTRY0_VALID                        0x80000000
#define KSZ9477_VLAN_TABLE_ENTRY0_FORWARD_OPTION               0x08000000
#define KSZ9477_VLAN_TABLE_ENTRY0_PRIORITY                     0x07000000
#define KSZ9477_VLAN_TABLE_ENTRY0_MSTP_INDEX                   0x00007000
#define KSZ9477_VLAN_TABLE_ENTRY0_FID                          0x0000007F

//VLAN Table Entry 1 register
#define KSZ9477_VLAN_TABLE_ENTRY1_PORT_UNTAG                   0x0000007F
#define KSZ9477_VLAN_TABLE_ENTRY1_PORT7_UNTAG                  0x00000040
#define KSZ9477_VLAN_TABLE_ENTRY1_PORT6_UNTAG                  0x00000020
#define KSZ9477_VLAN_TABLE_ENTRY1_PORT5_UNTAG                  0x00000010
#define KSZ9477_VLAN_TABLE_ENTRY1_PORT4_UNTAG                  0x00000008
#define KSZ9477_VLAN_TABLE_ENTRY1_PORT3_UNTAG                  0x00000004
#define KSZ9477_VLAN_TABLE_ENTRY1_PORT2_UNTAG                  0x00000002
#define KSZ9477_VLAN_TABLE_ENTRY1_PORT1_UNTAG                  0x00000001

//VLAN Table Entry 2 register
#define KSZ9477_VLAN_TABLE_ENTRY2_PORT_FORWARD                 0x0000007F
#define KSZ9477_VLAN_TABLE_ENTRY2_PORT7_FORWARD                0x00000040
#define KSZ9477_VLAN_TABLE_ENTRY2_PORT6_FORWARD                0x00000020
#define KSZ9477_VLAN_TABLE_ENTRY2_PORT5_FORWARD                0x00000010
#define KSZ9477_VLAN_TABLE_ENTRY2_PORT4_FORWARD                0x00000008
#define KSZ9477_VLAN_TABLE_ENTRY2_PORT3_FORWARD                0x00000004
#define KSZ9477_VLAN_TABLE_ENTRY2_PORT2_FORWARD                0x00000002
#define KSZ9477_VLAN_TABLE_ENTRY2_PORT1_FORWARD                0x00000001

//VLAN Table Index register
#define KSZ9477_VLAN_TABLE_INDEX_VLAN_INDEX                    0x0FFF

//VLAN Table Access Control register
#define KSZ9477_VLAN_TABLE_ACCESS_CTRL_START_FINISH            0x80
#define KSZ9477_VLAN_TABLE_ACCESS_CTRL_ACTION                  0x03
#define KSZ9477_VLAN_TABLE_ACCESS_CTRL_ACTION_NOP              0x00
#define KSZ9477_VLAN_TABLE_ACCESS_CTRL_ACTION_WRITE            0x01
#define KSZ9477_VLAN_TABLE_ACCESS_CTRL_ACTION_READ             0x02
#define KSZ9477_VLAN_TABLE_ACCESS_CTRL_ACTION_CLEAR            0x03

//ALU Table Index 0 register
#define KSZ9477_ALU_TABLE_INDEX0_FID_INDEX                     0x007F0000
#define KSZ9477_ALU_TABLE_INDEX0_MAC_INDEX_MSB                 0x0000FFFF

//ALU Table Index 1 register
#define KSZ9477_ALU_TABLE_INDEX1_MAC_INDEX_LSB                 0xFFFFFFFF

//ALU Table Access Control register
#define KSZ9477_ALU_TABLE_CTRL_VALID_COUNT                     0x3FFF0000
#define KSZ9477_ALU_TABLE_CTRL_START_FINISH                    0x00000080
#define KSZ9477_ALU_TABLE_CTRL_VALID                           0x00000040
#define KSZ9477_ALU_TABLE_CTRL_VALID_ENTRY_OR_SEARCH_END       0x00000020
#define KSZ9477_ALU_TABLE_CTRL_DIRECT                          0x00000004
#define KSZ9477_ALU_TABLE_CTRL_ACTION                          0x00000003
#define KSZ9477_ALU_TABLE_CTRL_ACTION_NOP                      0x00000000
#define KSZ9477_ALU_TABLE_CTRL_ACTION_WRITE                    0x00000001
#define KSZ9477_ALU_TABLE_CTRL_ACTION_READ                     0x00000002
#define KSZ9477_ALU_TABLE_CTRL_ACTION_SEARCH                   0x00000003

//Static Address and Reserved Multicast Table Control register
#define KSZ9477_STATIC_RES_MCAST_TABLE_CTRL_TABLE_INDEX        0x003F0000
#define KSZ9477_STATIC_RES_MCAST_TABLE_CTRL_START_FINISH       0x00000080
#define KSZ9477_STATIC_RES_MCAST_TABLE_CTRL_TABLE_SELECT       0x00000002
#define KSZ9477_STATIC_RES_MCAST_TABLE_CTRL_ACTION             0x00000001
#define KSZ9477_STATIC_RES_MCAST_TABLE_CTRL_ACTION_READ        0x00000000
#define KSZ9477_STATIC_RES_MCAST_TABLE_CTRL_ACTION_WRITE       0x00000001

//ALU Table Entry 1 register
#define KSZ9477_ALU_TABLE_ENTRY1_STATIC                        0x80000000
#define KSZ9477_ALU_TABLE_ENTRY1_SRC_FILTER                    0x40000000
#define KSZ9477_ALU_TABLE_ENTRY1_DES_FILTER                    0x20000000
#define KSZ9477_ALU_TABLE_ENTRY1_PRIORITY                      0x1C000000
#define KSZ9477_ALU_TABLE_ENTRY1_AGE_COUNT                     0x1C000000
#define KSZ9477_ALU_TABLE_ENTRY1_MSTP                          0x00000007

//ALU Table Entry 2 register
#define KSZ9477_ALU_TABLE_ENTRY2_OVERRIDE                      0x80000000
#define KSZ9477_ALU_TABLE_ENTRY2_PORT_FORWARD                  0x0000007F
#define KSZ9477_ALU_TABLE_ENTRY2_PORT7_FORWARD                 0x00000040
#define KSZ9477_ALU_TABLE_ENTRY2_PORT6_FORWARD                 0x00000020
#define KSZ9477_ALU_TABLE_ENTRY2_PORT5_FORWARD                 0x00000010
#define KSZ9477_ALU_TABLE_ENTRY2_PORT4_FORWARD                 0x00000008
#define KSZ9477_ALU_TABLE_ENTRY2_PORT3_FORWARD                 0x00000004
#define KSZ9477_ALU_TABLE_ENTRY2_PORT2_FORWARD                 0x00000002
#define KSZ9477_ALU_TABLE_ENTRY2_PORT1_FORWARD                 0x00000001

//ALU Table Entry 3 register
#define KSZ9477_ALU_TABLE_ENTRY3_FID                           0x007F0000
#define KSZ9477_ALU_TABLE_ENTRY3_MAC_ADDR_MSB                  0x0000FFFF

//ALU Table Entry 4 register
#define KSZ9477_ALU_TABLE_ENTRY4_MAC_ADDR_LSB                  0xFFFFFFFF

//Static Address Table Entry 1 register
#define KSZ9477_STATIC_TABLE_ENTRY1_VALID                      0x80000000
#define KSZ9477_STATIC_TABLE_ENTRY1_SRC_FILTER                 0x40000000
#define KSZ9477_STATIC_TABLE_ENTRY1_DES_FILTER                 0x20000000
#define KSZ9477_STATIC_TABLE_ENTRY1_PRIORITY                   0x1C000000
#define KSZ9477_STATIC_TABLE_ENTRY1_MSTP                       0x00000007

//Static Address Table Entry 2 register
#define KSZ9477_STATIC_TABLE_ENTRY2_OVERRIDE                   0x80000000
#define KSZ9477_STATIC_TABLE_ENTRY2_USE_FID                    0x40000000
#define KSZ9477_STATIC_TABLE_ENTRY2_PORT_FORWARD               0x0000007F
#define KSZ9477_STATIC_TABLE_ENTRY2_PORT7_FORWARD              0x00000040
#define KSZ9477_STATIC_TABLE_ENTRY2_PORT6_FORWARD              0x00000020
#define KSZ9477_STATIC_TABLE_ENTRY2_PORT5_FORWARD              0x00000010
#define KSZ9477_STATIC_TABLE_ENTRY2_PORT4_FORWARD              0x00000008
#define KSZ9477_STATIC_TABLE_ENTRY2_PORT3_FORWARD              0x00000004
#define KSZ9477_STATIC_TABLE_ENTRY2_PORT2_FORWARD              0x00000002
#define KSZ9477_STATIC_TABLE_ENTRY2_PORT1_FORWARD              0x00000001

//Static Address Table Entry 3 register
#define KSZ9477_STATIC_TABLE_ENTRY3_FID                        0x007F0000
#define KSZ9477_STATIC_TABLE_ENTRY3_MAC_ADDR_MSB               0x0000FFFF

//Static Address Table Entry 4 register
#define KSZ9477_STATIC_TABLE_ENTRY4_MAC_ADDR_LSB               0xFFFFFFFF

//Reserved Multicast Table Entry 2 register
#define KSZ9477_RES_MCAST_TABLE_ENTRY2_PORT_FORWARD            0x0000007F
#define KSZ9477_RES_MCAST_TABLE_ENTRY2_PORT7_FORWARD           0x00000040
#define KSZ9477_RES_MCAST_TABLE_ENTRY2_PORT6_FORWARD           0x00000020
#define KSZ9477_RES_MCAST_TABLE_ENTRY2_PORT5_FORWARD           0x00000010
#define KSZ9477_RES_MCAST_TABLE_ENTRY2_PORT4_FORWARD           0x00000008
#define KSZ9477_RES_MCAST_TABLE_ENTRY2_PORT3_FORWARD           0x00000004
#define KSZ9477_RES_MCAST_TABLE_ENTRY2_PORT2_FORWARD           0x00000002
#define KSZ9477_RES_MCAST_TABLE_ENTRY2_PORT1_FORWARD           0x00000001

//Port N Default Tag 0 register
#define KSZ9477_PORTn_DEFAULT_TAG0_PCP                         0xE0
#define KSZ9477_PORTn_DEFAULT_TAG0_DEI                         0x10
#define KSZ9477_PORTn_DEFAULT_TAG0_VID_MSB                     0x0F

//Port N Default Tag 1 register
#define KSZ9477_PORTn_DEFAULT_TAG1_VID_LSB                     0xFF

//Port N Interrupt Status register
#define KSZ9477_PORTn_INT_STATUS_SGMII_AN_DONE                 0x08
#define KSZ9477_PORTn_INT_STATUS_PTP                           0x04
#define KSZ9477_PORTn_INT_STATUS_PHY                           0x02
#define KSZ9477_PORTn_INT_STATUS_ACL                           0x01

//Port N Interrupt Mask register
#define KSZ9477_PORTn_INT_MASK_SGMII_AN_DONE                   0x08
#define KSZ9477_PORTn_INT_MASK_PTP                             0x04
#define KSZ9477_PORTn_INT_MASK_PHY                             0x02
#define KSZ9477_PORTn_INT_MASK_ACL                             0x01

//Port N Operation Control 0 register
#define KSZ9477_PORTn_OP_CTRL0_LOCAL_LOOPBACK                  0x80
#define KSZ9477_PORTn_OP_CTRL0_REMOTE_LOOPBACK                 0x40
#define KSZ9477_PORTn_OP_CTRL0_TAIL_TAG_EN                     0x04
#define KSZ9477_PORTn_OP_CTRL0_TX_QUEUE_SPLIT_EN               0x03

//Port N Status register
#define KSZ9477_PORTn_STATUS_SPEED                             0x18
#define KSZ9477_PORTn_STATUS_SPEED_10MBPS                      0x00
#define KSZ9477_PORTn_STATUS_SPEED_100MBPS                     0x08
#define KSZ9477_PORTn_STATUS_SPEED_1000MBPS                    0x10
#define KSZ9477_PORTn_STATUS_DUPLEX                            0x04
#define KSZ9477_PORTn_STATUS_TX_FLOW_CTRL_EN                   0x02
#define KSZ9477_PORTn_STATUS_RX_FLOW_CTRL_EN                   0x01

//XMII Port N Control 0 register
#define KSZ9477_PORTn_XMII_CTRL0_DUPLEX                        0x40
#define KSZ9477_PORTn_XMII_CTRL0_TX_FLOW_CTRL_EN               0x20
#define KSZ9477_PORTn_XMII_CTRL0_SPEED_10_100                  0x10
#define KSZ9477_PORTn_XMII_CTRL0_RX_FLOW_CTRL_EN               0x08

//XMII Port N Control 1 register
#define KSZ9477_PORTn_XMII_CTRL1_SPEED_1000                    0x40
#define KSZ9477_PORTn_XMII_CTRL1_RGMII_ID_IG                   0x10
#define KSZ9477_PORTn_XMII_CTRL1_RGMII_ID_EG                   0x08
#define KSZ9477_PORTn_XMII_CTRL1_MII_RMII_MODE                 0x04
#define KSZ9477_PORTn_XMII_CTRL1_IF_TYPE                       0x03
#define KSZ9477_PORTn_XMII_CTRL1_IF_TYPE_RGMII                 0x00
#define KSZ9477_PORTn_XMII_CTRL1_IF_TYPE_RMII                  0x01
#define KSZ9477_PORTn_XMII_CTRL1_IF_TYPE_MII                   0x03

//Port N ACL Access Control 0 register
#define KSZ9477_PORTn_ACL_ACCESS_CTRL0_WRITE_STATUS            0x40
#define KSZ9477_PORTn_ACL_ACCESS_CTRL0_READ_STATUS             0x20
#define KSZ9477_PORTn_ACL_ACCESS_CTRL0_READ                    0x00
#define KSZ9477_PORTn_ACL_ACCESS_CTRL0_WRITE                   0x10
#define KSZ9477_PORTn_ACL_ACCESS_CTRL0_ACL_INDEX               0x0F

//Port N ACL Access Control 1 register
#define KSZ9477_PORTn_ACL_ACCESS_CTRL1_FORCE_DLR_MISS          0x01

//Port N Port Mirroring Control register
#define KSZ9477_PORTn_MIRRORING_CTRL_RECEIVE_SNIFF             0x40
#define KSZ9477_PORTn_MIRRORING_CTRL_TRANSMIT_SNIFF            0x20
#define KSZ9477_PORTn_MIRRORING_CTRL_SNIFFER_PORT              0x02

//Port N Authentication Control register
#define KSZ9477_PORTn_AUTH_CTRL_ACL_EN                         0x04
#define KSZ9477_PORTn_AUTH_CTRL_AUTH_MODE                      0x03
#define KSZ9477_PORTn_AUTH_CTRL_AUTH_MODE_PASS                 0x00
#define KSZ9477_PORTn_AUTH_CTRL_AUTH_MODE_BLOCK                0x01
#define KSZ9477_PORTn_AUTH_CTRL_AUTH_MODE_TRAP                 0x02

//Port N Pointer register
#define KSZ9477_PORTn_PTR_PORT_INDEX                           0x00070000
#define KSZ9477_PORTn_PTR_QUEUE_PTR                            0x00000003

//Port N MSTP Pointer register
#define KSZ9477_PORTn_MSTP_PTR_MSTP_PTR                        0x07

//Port N MSTP State register
#define KSZ9477_PORTn_MSTP_STATE_TRANSMIT_EN                   0x04
#define KSZ9477_PORTn_MSTP_STATE_RECEIVE_EN                    0x02
#define KSZ9477_PORTn_MSTP_STATE_LEARNING_DIS                  0x01

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//KSZ9477 Ethernet switch driver
extern const SwitchDriver ksz9477SwitchDriver;

//KSZ9477 related functions
error_t ksz9477Init(NetInterface *interface);

void ksz9477Tick(NetInterface *interface);

void ksz9477EnableIrq(NetInterface *interface);
void ksz9477DisableIrq(NetInterface *interface);

void ksz9477EventHandler(NetInterface *interface);

error_t ksz9477TagFrame(NetInterface *interface, NetBuffer *buffer,
   size_t *offset, NetTxAncillary *ancillary);

error_t ksz9477UntagFrame(NetInterface *interface, uint8_t **frame,
   size_t *length, NetRxAncillary *ancillary);

bool_t ksz9477GetLinkState(NetInterface *interface, uint8_t port);
uint32_t ksz9477GetLinkSpeed(NetInterface *interface, uint8_t port);
NicDuplexMode ksz9477GetDuplexMode(NetInterface *interface, uint8_t port);

void ksz9477SetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state);

SwitchPortState ksz9477GetPortState(NetInterface *interface, uint8_t port);

void ksz9477SetAgingTime(NetInterface *interface, uint32_t agingTime);

void ksz9477EnableIgmpSnooping(NetInterface *interface, bool_t enable);
void ksz9477EnableMldSnooping(NetInterface *interface, bool_t enable);
void ksz9477EnableRsvdMcastTable(NetInterface *interface, bool_t enable);

error_t ksz9477AddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t ksz9477DeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t ksz9477GetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void ksz9477FlushStaticFdbTable(NetInterface *interface);

error_t ksz9477GetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void ksz9477FlushDynamicFdbTable(NetInterface *interface, uint8_t port);

void ksz9477SetUnknownMcastFwdPorts(NetInterface *interface,
   bool_t enable, uint32_t forwardPorts);

void ksz9477WritePhyReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data);

uint16_t ksz9477ReadPhyReg(NetInterface *interface, uint8_t port,
   uint8_t address);

void ksz9477DumpPhyReg(NetInterface *interface, uint8_t port);

void ksz9477WriteMmdReg(NetInterface *interface, uint8_t port,
   uint8_t devAddr, uint16_t regAddr, uint16_t data);

uint16_t ksz9477ReadMmdReg(NetInterface *interface, uint8_t port,
   uint8_t devAddr, uint16_t regAddr);

void ksz9477WriteSwitchReg8(NetInterface *interface, uint16_t address,
   uint8_t data);

uint8_t ksz9477ReadSwitchReg8(NetInterface *interface, uint16_t address);

void ksz9477WriteSwitchReg16(NetInterface *interface, uint16_t address,
   uint16_t data);

uint16_t ksz9477ReadSwitchReg16(NetInterface *interface, uint16_t address);

void ksz9477WriteSwitchReg32(NetInterface *interface, uint16_t address,
   uint32_t data);

uint32_t ksz9477ReadSwitchReg32(NetInterface *interface, uint16_t address);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
