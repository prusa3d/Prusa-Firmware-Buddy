/**
 * @file ksz8463_driver.h
 * @brief KSZ8463 3-port Ethernet switch driver
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

#ifndef _KSZ8463_DRIVER_H
#define _KSZ8463_DRIVER_H

//Dependencies
#include "core/nic.h"

//Port identifiers
#define KSZ8463_PORT1 1
#define KSZ8463_PORT2 2
#define KSZ8463_PORT3 3

//Port masks
#define KSZ8463_PORT_MASK  0x07
#define KSZ8463_PORT1_MASK 0x01
#define KSZ8463_PORT2_MASK 0x02
#define KSZ8463_PORT3_MASK 0x04

//SPI command byte
#define KSZ8463_SPI_CMD_READ  0x0000
#define KSZ8463_SPI_CMD_WRITE 0x8000
#define KSZ8463_SPI_CMD_ADDR  0x7FC0
#define KSZ8463_SPI_CMD_B3    0x0020
#define KSZ8463_SPI_CMD_B2    0x0010
#define KSZ8463_SPI_CMD_B1    0x0008
#define KSZ8463_SPI_CMD_B0    0x0004

//Size of static and dynamic MAC tables
#define KSZ8463_STATIC_MAC_TABLE_SIZE  8
#define KSZ8463_DYNAMIC_MAC_TABLE_SIZE 1024

//Tail tag rules (host to KSZ8463)
#define KSZ8463_TAIL_TAG_PRIORITY           0x0C
#define KSZ8463_TAIL_TAG_DEST_PORT2         0x02
#define KSZ8463_TAIL_TAG_DEST_PORT1         0x01
#define KSZ8463_TAIL_TAG_NORMAL_ADDR_LOOKUP 0x00

//Tail tag rules (KSZ8463 to host)
#define KSZ8463_TAIL_TAG_SRC_PORT           0x01

//KSZ8463 PHY registers
#define KSZ8463_BMCR                                        0x00
#define KSZ8463_BMSR                                        0x01
#define KSZ8463_PHYID1                                      0x02
#define KSZ8463_PHYID2                                      0x03
#define KSZ8463_ANAR                                        0x04
#define KSZ8463_ANLPAR                                      0x05
#define KSZ8463_LINKMD                                      0x1D
#define KSZ8463_PHYSCS                                      0x1F

//KSZ8463 Switch registers
#define KSZ8463_CIDER                                       0x0000
#define KSZ8463_SGCR1                                       0x0002
#define KSZ8463_SGCR2                                       0x0004
#define KSZ8463_SGCR3                                       0x0006
#define KSZ8463_SGCR6                                       0x000C
#define KSZ8463_SGCR7                                       0x000E
#define KSZ8463_MACAR1                                      0x0010
#define KSZ8463_MACAR2                                      0x0012
#define KSZ8463_MACAR3                                      0x0014
#define KSZ8463_TOSR1                                       0x0016
#define KSZ8463_TOSR2                                       0x0018
#define KSZ8463_TOSR3                                       0x001A
#define KSZ8463_TOSR4                                       0x001C
#define KSZ8463_TOSR5                                       0x001E
#define KSZ8463_TOSR6                                       0x0020
#define KSZ8463_TOSR7                                       0x0022
#define KSZ8463_TOSR8                                       0x0024
#define KSZ8463_IADR1                                       0x0026
#define KSZ8463_IADR2                                       0x0028
#define KSZ8463_IADR3                                       0x002A
#define KSZ8463_IADR4                                       0x002C
#define KSZ8463_IADR5                                       0x002E
#define KSZ8463_IACR                                        0x0030
#define KSZ8463_PMCTRL                                      0x0032
#define KSZ8463_GST                                         0x0036
#define KSZ8463_CTPDC                                       0x0038
#define KSZ8463_P1MBCR                                      0x004C
#define KSZ8463_P1MBSR                                      0x004E
#define KSZ8463_PHY1ILR                                     0x0050
#define KSZ8463_PHY1IHR                                     0x0052
#define KSZ8463_P1ANAR                                      0x0054
#define KSZ8463_P1ANLPR                                     0x0056
#define KSZ8463_P2MBCR                                      0x0058
#define KSZ8463_P2MBSR                                      0x005A
#define KSZ8463_PHY2ILR                                     0x005C
#define KSZ8463_PHY2IHR                                     0x005E
#define KSZ8463_P2ANAR                                      0x0060
#define KSZ8463_P2ANLPR                                     0x0062
#define KSZ8463_P1PHYCTRL                                   0x0066
#define KSZ8463_P2PHYCTRL                                   0x006A
#define KSZ8463_P1CR1                                       0x006C
#define KSZ8463_P1CR2                                       0x006E
#define KSZ8463_P1VIDCR                                     0x0070
#define KSZ8463_P1CR3                                       0x0072
#define KSZ8463_P1IRCR0                                     0x0074
#define KSZ8463_P1IRCR1                                     0x0076
#define KSZ8463_P1ERCR0                                     0x0078
#define KSZ8463_P1ERCR1                                     0x007A
#define KSZ8463_P1SCSLMD                                    0x007C
#define KSZ8463_P1CR4                                       0x007E
#define KSZ8463_P1SR                                        0x0080
#define KSZ8463_P2CR1                                       0x0084
#define KSZ8463_P2CR2                                       0x0086
#define KSZ8463_P2VIDCR                                     0x0088
#define KSZ8463_P2CR3                                       0x008A
#define KSZ8463_P2IRCR0                                     0x008C
#define KSZ8463_P2IRCR1                                     0x008E
#define KSZ8463_P2ERCR0                                     0x0090
#define KSZ8463_P2ERCR1                                     0x0092
#define KSZ8463_P2SCSLMD                                    0x0094
#define KSZ8463_P2CR4                                       0x0096
#define KSZ8463_P2SR                                        0x0098
#define KSZ8463_P3CR1                                       0x009C
#define KSZ8463_P3CR2                                       0x009E
#define KSZ8463_P3VIDCR                                     0x00A0
#define KSZ8463_P3CR3                                       0x00A2
#define KSZ8463_P3IRCR0                                     0x00A4
#define KSZ8463_P3IRCR1                                     0x00A6
#define KSZ8463_P3ERCR0                                     0x00A8
#define KSZ8463_P3ERCR1                                     0x00AA
#define KSZ8463_SGCR8                                       0x00AC
#define KSZ8463_SGCR9                                       0x00AE
#define KSZ8463_SAFMACA1L                                   0x00B0
#define KSZ8463_SAFMACA1M                                   0x00B2
#define KSZ8463_SAFMACA1H                                   0x00B4
#define KSZ8463_SAFMACA2L                                   0x00B6
#define KSZ8463_SAFMACA2M                                   0x00B8
#define KSZ8463_SAFMACA2H                                   0x00BA
#define KSZ8463_P1TXQRCR1                                   0x00C8
#define KSZ8463_P1TXQRCR2                                   0x00CA
#define KSZ8463_P2TXQRCR1                                   0x00CC
#define KSZ8463_P2TXQRCR2                                   0x00CE
#define KSZ8463_P3TXQRCR1                                   0x00D0
#define KSZ8463_P3TXQRCR2                                   0x00D2
#define KSZ8463_IOMXSEL                                     0x00D6
#define KSZ8463_CFGR                                        0x00D8
#define KSZ8463_P1ANPT                                      0x00DC
#define KSZ8463_P1ALPRNP                                    0x00DE
#define KSZ8463_P1EEEA                                      0x00E0
#define KSZ8463_P1EEEWEC                                    0x00E2
#define KSZ8463_P1EEECS                                     0x00E4
#define KSZ8463_P1LPIRTC                                    0x00E6
#define KSZ8463_BL2LPIC1                                    0x00E7
#define KSZ8463_P2ANPT                                      0x00E8
#define KSZ8463_P2ALPRNP                                    0x00EA
#define KSZ8463_P2EEEA                                      0x00EC
#define KSZ8463_P2EEEWEC                                    0x00EE
#define KSZ8463_P2EEECS                                     0x00F0
#define KSZ8463_P2LPIRTC                                    0x00F2
#define KSZ8463_PCSEEEC                                     0x00F3
#define KSZ8463_ETLWTC                                      0x00F4
#define KSZ8463_BL2LPIC2                                    0x00F6
#define KSZ8463_MBIR                                        0x0124
#define KSZ8463_GRR                                         0x0126
#define KSZ8463_IER                                         0x0190
#define KSZ8463_ISR                                         0x0192
#define KSZ8463_TRIG_ERR                                    0x0200
#define KSZ8463_TRIG_ACTIVE                                 0x0202
#define KSZ8463_TRIG_DONE                                   0x0204
#define KSZ8463_TRIG_EN                                     0x0206
#define KSZ8463_TRIG_SW_RST                                 0x0208
#define KSZ8463_TRIG12_PPS_WIDTH                            0x020A
#define KSZ8463_TRIG1_TGT_NSL                               0x0220
#define KSZ8463_TRIG1_TGT_NSH                               0x0222
#define KSZ8463_TRIG1_TGT_SL                                0x0224
#define KSZ8463_TRIG1_TGT_SH                                0x0226
#define KSZ8463_TRIG1_CFG_1                                 0x0228
#define KSZ8463_TRIG1_CFG_2                                 0x022A
#define KSZ8463_TRIG1_CFG_3                                 0x022C
#define KSZ8463_TRIG1_CFG_4                                 0x022E
#define KSZ8463_TRIG1_CFG_5                                 0x0230
#define KSZ8463_TRIG1_CFG_6                                 0x0232
#define KSZ8463_TRIG1_CFG_7                                 0x0234
#define KSZ8463_TRIG1_CFG_8                                 0x0236
#define KSZ8463_TRIG2_TGT_NSL                               0x0240
#define KSZ8463_TRIG2_TGT_NSH                               0x0242
#define KSZ8463_TRIG2_TGT_SL                                0x0244
#define KSZ8463_TRIG2_TGT_SH                                0x0246
#define KSZ8463_TRIG2_CFG_1                                 0x0248
#define KSZ8463_TRIG2_CFG_2                                 0x024A
#define KSZ8463_TRIG2_CFG_3                                 0x024C
#define KSZ8463_TRIG2_CFG_4                                 0x024E
#define KSZ8463_TRIG2_CFG_5                                 0x0250
#define KSZ8463_TRIG2_CFG_6                                 0x0252
#define KSZ8463_TRIG2_CFG_7                                 0x0254
#define KSZ8463_TRIG2_CFG_8                                 0x0256
#define KSZ8463_TRIG3_TGT_NSL                               0x0260
#define KSZ8463_TRIG3_TGT_NSH                               0x0262
#define KSZ8463_TRIG3_TGT_SL                                0x0264
#define KSZ8463_TRIG3_TGT_SH                                0x0266
#define KSZ8463_TRIG3_CFG_1                                 0x0268
#define KSZ8463_TRIG3_CFG_2                                 0x026A
#define KSZ8463_TRIG3_CFG_3                                 0x026C
#define KSZ8463_TRIG3_CFG_4                                 0x026E
#define KSZ8463_TRIG3_CFG_5                                 0x0270
#define KSZ8463_TRIG3_CFG_6                                 0x0272
#define KSZ8463_TRIG3_CFG_7                                 0x0274
#define KSZ8463_TRIG3_CFG_8                                 0x0276
#define KSZ8463_TRIG4_TGT_NSL                               0x0280
#define KSZ8463_TRIG4_TGT_NSH                               0x0282
#define KSZ8463_TRIG4_TGT_SL                                0x0284
#define KSZ8463_TRIG4_TGT_SH                                0x0286
#define KSZ8463_TRIG4_CFG_1                                 0x0288
#define KSZ8463_TRIG4_CFG_2                                 0x028A
#define KSZ8463_TRIG4_CFG_3                                 0x028C
#define KSZ8463_TRIG4_CFG_4                                 0x028E
#define KSZ8463_TRIG4_CFG_5                                 0x0290
#define KSZ8463_TRIG4_CFG_6                                 0x0292
#define KSZ8463_TRIG4_CFG_7                                 0x0294
#define KSZ8463_TRIG4_CFG_8                                 0x0296
#define KSZ8463_TRIG5_TGT_NSL                               0x02A0
#define KSZ8463_TRIG5_TGT_NSH                               0x02A2
#define KSZ8463_TRIG5_TGT_SL                                0x02A4
#define KSZ8463_TRIG5_TGT_SH                                0x02A6
#define KSZ8463_TRIG5_CFG_1                                 0x02A8
#define KSZ8463_TRIG5_CFG_2                                 0x02AA
#define KSZ8463_TRIG5_CFG_3                                 0x02AC
#define KSZ8463_TRIG5_CFG_4                                 0x02AE
#define KSZ8463_TRIG5_CFG_5                                 0x02B0
#define KSZ8463_TRIG5_CFG_6                                 0x02B2
#define KSZ8463_TRIG5_CFG_7                                 0x02B4
#define KSZ8463_TRIG5_CFG_8                                 0x02B6
#define KSZ8463_TRIG6_TGT_NSL                               0x02C0
#define KSZ8463_TRIG6_TGT_NSH                               0x02C2
#define KSZ8463_TRIG6_TGT_SL                                0x02C4
#define KSZ8463_TRIG6_TGT_SH                                0x02C6
#define KSZ8463_TRIG6_CFG_1                                 0x02C8
#define KSZ8463_TRIG6_CFG_2                                 0x02CA
#define KSZ8463_TRIG6_CFG_3                                 0x02CC
#define KSZ8463_TRIG6_CFG_4                                 0x02CE
#define KSZ8463_TRIG6_CFG_5                                 0x02D0
#define KSZ8463_TRIG6_CFG_6                                 0x02D2
#define KSZ8463_TRIG6_CFG_7                                 0x02D4
#define KSZ8463_TRIG6_CFG_8                                 0x02D6
#define KSZ8463_TRIG7_TGT_NSL                               0x02E0
#define KSZ8463_TRIG7_TGT_NSH                               0x02E2
#define KSZ8463_TRIG7_TGT_SL                                0x02E4
#define KSZ8463_TRIG7_TGT_SH                                0x02E6
#define KSZ8463_TRIG7_CFG_1                                 0x02E8
#define KSZ8463_TRIG7_CFG_2                                 0x02EA
#define KSZ8463_TRIG7_CFG_3                                 0x02EC
#define KSZ8463_TRIG7_CFG_4                                 0x02EE
#define KSZ8463_TRIG7_CFG_5                                 0x02F0
#define KSZ8463_TRIG7_CFG_6                                 0x02F2
#define KSZ8463_TRIG7_CFG_7                                 0x02F4
#define KSZ8463_TRIG7_CFG_8                                 0x02F6
#define KSZ8463_TRIG8_TGT_NSL                               0x0300
#define KSZ8463_TRIG8_TGT_NSH                               0x0302
#define KSZ8463_TRIG8_TGT_SL                                0x0304
#define KSZ8463_TRIG8_TGT_SH                                0x0306
#define KSZ8463_TRIG8_CFG_1                                 0x0308
#define KSZ8463_TRIG8_CFG_2                                 0x030A
#define KSZ8463_TRIG8_CFG_3                                 0x030C
#define KSZ8463_TRIG8_CFG_4                                 0x030E
#define KSZ8463_TRIG8_CFG_5                                 0x0310
#define KSZ8463_TRIG8_CFG_6                                 0x0312
#define KSZ8463_TRIG8_CFG_7                                 0x0314
#define KSZ8463_TRIG8_CFG_8                                 0x0316
#define KSZ8463_TRIG9_TGT_NSL                               0x0320
#define KSZ8463_TRIG9_TGT_NSH                               0x0322
#define KSZ8463_TRIG9_TGT_SL                                0x0324
#define KSZ8463_TRIG9_TGT_SH                                0x0326
#define KSZ8463_TRIG9_CFG_1                                 0x0328
#define KSZ8463_TRIG9_CFG_2                                 0x032A
#define KSZ8463_TRIG9_CFG_3                                 0x032C
#define KSZ8463_TRIG9_CFG_4                                 0x032E
#define KSZ8463_TRIG9_CFG_5                                 0x0330
#define KSZ8463_TRIG9_CFG_6                                 0x0332
#define KSZ8463_TRIG9_CFG_7                                 0x0334
#define KSZ8463_TRIG9_CFG_8                                 0x0336
#define KSZ8463_TRIG10_TGT_NSL                              0x0340
#define KSZ8463_TRIG10_TGT_NSH                              0x0342
#define KSZ8463_TRIG10_TGT_SL                               0x0344
#define KSZ8463_TRIG10_TGT_SH                               0x0346
#define KSZ8463_TRIG10_CFG_1                                0x0348
#define KSZ8463_TRIG10_CFG_2                                0x034A
#define KSZ8463_TRIG10_CFG_3                                0x034C
#define KSZ8463_TRIG10_CFG_4                                0x034E
#define KSZ8463_TRIG10_CFG_5                                0x0350
#define KSZ8463_TRIG10_CFG_6                                0x0352
#define KSZ8463_TRIG10_CFG_7                                0x0354
#define KSZ8463_TRIG10_CFG_8                                0x0356
#define KSZ8463_TRIG11_TGT_NSL                              0x0360
#define KSZ8463_TRIG11_TGT_NSH                              0x0362
#define KSZ8463_TRIG11_TGT_SL                               0x0364
#define KSZ8463_TRIG11_TGT_SH                               0x0366
#define KSZ8463_TRIG11_CFG_1                                0x0368
#define KSZ8463_TRIG11_CFG_2                                0x036A
#define KSZ8463_TRIG11_CFG_3                                0x036C
#define KSZ8463_TRIG11_CFG_4                                0x036E
#define KSZ8463_TRIG11_CFG_5                                0x0370
#define KSZ8463_TRIG11_CFG_6                                0x0372
#define KSZ8463_TRIG11_CFG_7                                0x0374
#define KSZ8463_TRIG11_CFG_8                                0x0376
#define KSZ8463_TRIG12_TGT_NSL                              0x0380
#define KSZ8463_TRIG12_TGT_NSH                              0x0382
#define KSZ8463_TRIG12_TGT_SL                               0x0384
#define KSZ8463_TRIG12_TGT_SH                               0x0386
#define KSZ8463_TRIG12_CFG_1                                0x0388
#define KSZ8463_TRIG12_CFG_2                                0x038A
#define KSZ8463_TRIG12_CFG_3                                0x038C
#define KSZ8463_TRIG12_CFG_4                                0x038E
#define KSZ8463_TRIG12_CFG_5                                0x0390
#define KSZ8463_TRIG12_CFG_6                                0x0392
#define KSZ8463_TRIG12_CFG_7                                0x0394
#define KSZ8463_TRIG12_CFG_8                                0x0396
#define KSZ8463_TS_RDY                                      0x0400
#define KSZ8463_TS_EN                                       0x0402
#define KSZ8463_TS_SW_RST                                   0x0404
#define KSZ8463_TS1_STATUS                                  0x0420
#define KSZ8463_TS1_CFG                                     0x0422
#define KSZ8463_TS1_SMPL1_NSL                               0x0424
#define KSZ8463_TS1_SMPL1_NSH                               0x0426
#define KSZ8463_TS1_SMPL1_SL                                0x0428
#define KSZ8463_TS1_SMPL1_SH                                0x042A
#define KSZ8463_TS1_SMPL1_SUB_NS                            0x042C
#define KSZ8463_TS1_SMPL2_NSL                               0x0434
#define KSZ8463_TS1_SMPL2_NSH                               0x0436
#define KSZ8463_TS1_SMPL2_SL                                0x0438
#define KSZ8463_TS1_SMPL2_SH                                0x043A
#define KSZ8463_TS1_SMPL2_SUB_NS                            0x043C
#define KSZ8463_TS2_STATUS                                  0x0440
#define KSZ8463_TS2_CFG                                     0x0442
#define KSZ8463_TS2_SMPL1_NSL                               0x0444
#define KSZ8463_TS2_SMPL1_NSH                               0x0446
#define KSZ8463_TS2_SMPL1_SL                                0x0448
#define KSZ8463_TS2_SMPL1_SH                                0x044A
#define KSZ8463_TS2_SMPL1_SUB_NS                            0x044C
#define KSZ8463_TS2_SMPL2_NSL                               0x0454
#define KSZ8463_TS2_SMPL2_NSH                               0x0456
#define KSZ8463_TS2_SMP2_SL                                 0x0458
#define KSZ8463_TS2_SMPL2_SH                                0x045A
#define KSZ8463_TS2_SMPL2_SUB_NS                            0x045C
#define KSZ8463_TS3_STATUS                                  0x0460
#define KSZ8463_TS3_CFG                                     0x0462
#define KSZ8463_TS3_SMPL1_NSL                               0x0464
#define KSZ8463_TS3_SMPL1_NSH                               0x0466
#define KSZ8463_TS3_SMPL1_SL                                0x0468
#define KSZ8463_TS3_SMPL1_SH                                0x046A
#define KSZ8463_TS3_SMPL1_SUB_NS                            0x046C
#define KSZ8463_TS3_SMPL2_NSL                               0x0474
#define KSZ8463_TS3_SMPL2_NSH                               0x0476
#define KSZ8463_TS3_SMP2_SL                                 0x0478
#define KSZ8463_TS3_SMPL2_SH                                0x047A
#define KSZ8463_TS3_SMPL2_SUB_NS                            0x047C
#define KSZ8463_TS4_STATUS                                  0x0480
#define KSZ8463_TS4_CFG                                     0x0482
#define KSZ8463_TS4_SMPL1_NSL                               0x0484
#define KSZ8463_TS4_SMPL1_NSH                               0x0486
#define KSZ8463_TS4_SMPL1_SL                                0x0488
#define KSZ8463_TS4_SMPL1_SH                                0x048A
#define KSZ8463_TS4_SMPL1_SUB_NS                            0x048C
#define KSZ8463_TS4_SMPL2_NSL                               0x0494
#define KSZ8463_TS4_SMPL2_NSH                               0x0496
#define KSZ8463_TS4_SMP2_SL                                 0x0498
#define KSZ8463_TS4_SMPL2_SH                                0x049A
#define KSZ8463_TS4_SMPL2_SUB_NS                            0x049C
#define KSZ8463_TS5_STATUS                                  0x04A0
#define KSZ8463_TS5_STATUS                                  0x04A0
#define KSZ8463_TS5_CFG                                     0x04A2
#define KSZ8463_TS5_SMPL1_NSL                               0x04A4
#define KSZ8463_TS5_SMPL1_NSH                               0x04A6
#define KSZ8463_TS5_SMPL1_SL                                0x04A8
#define KSZ8463_TS5_SMPL1_SH                                0x04AA
#define KSZ8463_TS5_SMPL1_SUB_NS                            0x04AC
#define KSZ8463_TS5_SMPL2_NSL                               0x04B4
#define KSZ8463_TS5_SMPL2_NSH                               0x04B6
#define KSZ8463_TS5_SMP2_SL                                 0x04B8
#define KSZ8463_TS5_SMPL2_SH                                0x04BA
#define KSZ8463_TS5_SMPL2_SUB_NS                            0x04BC
#define KSZ8463_TS6_STATUS                                  0x04C0
#define KSZ8463_TS6_CFG                                     0x04C2
#define KSZ8463_TS6_SMPL1_NSL                               0x04C4
#define KSZ8463_TS6_SMPL1_NSH                               0x04C6
#define KSZ8463_TS6_SMPL1_SL                                0x04C8
#define KSZ8463_TS6_SMPL1_SH                                0x04CA
#define KSZ8463_TS6_SMPL1_SUB_NS                            0x04CC
#define KSZ8463_TS6_SMPL2_NSL                               0x04D4
#define KSZ8463_TS6_SMPL2_NSH                               0x04D6
#define KSZ8463_TS6_SMP2_SL                                 0x04D8
#define KSZ8463_TS6_SMPL2_SH                                0x04DA
#define KSZ8463_TS6_SMPL2_SUB_NS                            0x04DC
#define KSZ8463_TS7_STATUS                                  0x04E0
#define KSZ8463_TS7_CFG                                     0x04E2
#define KSZ8463_TS7_SMPL1_NSL                               0x04E4
#define KSZ8463_TS7_SMPL1_NSH                               0x04E6
#define KSZ8463_TS7_SMPL1_SL                                0x04E8
#define KSZ8463_TS7_SMPL1_SH                                0x04EA
#define KSZ8463_TS7_SMPL1_SUB_NS                            0x04EC
#define KSZ8463_TS7_SMPL2_NSL                               0x04F4
#define KSZ8463_TS7_SMPL2_NSH                               0x04F6
#define KSZ8463_TS7_SMP2_SL                                 0x04F8
#define KSZ8463_TS7_SMPL2_SH                                0x04FA
#define KSZ8463_TS7_SMPL2_SUB_NS                            0x04FC
#define KSZ8463_TS8_STATUS                                  0x0500
#define KSZ8463_TS8_CFG                                     0x0502
#define KSZ8463_TS8_SMPL1_NSL                               0x0504
#define KSZ8463_TS8_SMPL1_NSH                               0x0506
#define KSZ8463_TS8_SMPL1_SL                                0x0508
#define KSZ8463_TS8_SMPL1_SH                                0x050A
#define KSZ8463_TS8_SMPL1_SUB_NS                            0x050C
#define KSZ8463_TS8_SMPL2_NSL                               0x0514
#define KSZ8463_TS8_SMPL2_NSH                               0x0516
#define KSZ8463_TS8_SMP2_SL                                 0x0518
#define KSZ8463_TS8_SMPL2_SH                                0x051A
#define KSZ8463_TS8_SMPL2_SUB_NS                            0x051C
#define KSZ8463_TS9_STATUS                                  0x0520
#define KSZ8463_TS9_CFG                                     0x0522
#define KSZ8463_TS9_SMPL1_NSL                               0x0524
#define KSZ8463_TS9_SMPL1_NSH                               0x0526
#define KSZ8463_TS9_SMPL1_SL                                0x0528
#define KSZ8463_TS9_SMPL1_SH                                0x052A
#define KSZ8463_TS9_SMPL1_SUB_NS                            0x052C
#define KSZ8463_TS9_SMPL2_NSL                               0x0534
#define KSZ8463_TS9_SMPL2_NSH                               0x0536
#define KSZ8463_TS9_SMP2_SL                                 0x0538
#define KSZ8463_TS9_SMPL2_SH                                0x053A
#define KSZ8463_TS9_SMPL2_SUB_NS                            0x053C
#define KSZ8463_TS10_STATUS                                 0x0540
#define KSZ8463_TS10_CFG                                    0x0542
#define KSZ8463_TS10_SMPL1_NSL                              0x0544
#define KSZ8463_TS10_SMPL1_NSH                              0x0546
#define KSZ8463_TS10_SMPL1_SL                               0x0548
#define KSZ8463_TS10_SMPL1_SH                               0x054A
#define KSZ8463_TS10_SMPL1_SUB_NS                           0x054C
#define KSZ8463_TS10_SMPL2_NSL                              0x0554
#define KSZ8463_TS10_SMPL2_NSH                              0x0556
#define KSZ8463_TS10_SMP2_SL                                0x0558
#define KSZ8463_TS10_SMPL2_SH                               0x055A
#define KSZ8463_TS10_SMPL2_SUB_NS                           0x055C
#define KSZ8463_TS11_STATUS                                 0x0560
#define KSZ8463_TS11_CFG                                    0x0562
#define KSZ8463_TS11_SMPL1_NSL                              0x0564
#define KSZ8463_TS11_SMPL1_NSH                              0x0566
#define KSZ8463_TS11_SMPL1_SL                               0x0568
#define KSZ8463_TS11_SMPL1_SH                               0x056A
#define KSZ8463_TS11_SMPL1_SUB_NS                           0x056C
#define KSZ8463_TS11_SMPL2_NSL                              0x0574
#define KSZ8463_TS11_SMPL2_NSH                              0x0576
#define KSZ8463_TS11_SMP2_SL                                0x0578
#define KSZ8463_TS11_SMPL2_SH                               0x057A
#define KSZ8463_TS11_SMPL2_SUB_NS                           0x057C
#define KSZ8463_TS12_STATUS                                 0x0580
#define KSZ8463_TS12_CFG                                    0x0582
#define KSZ8463_TS12_SMPL1_NSL                              0x0584
#define KSZ8463_TS12_SMPL1_NSH                              0x0586
#define KSZ8463_TS12_SMPL1_SL                               0x0588
#define KSZ8463_TS12_SMPL1_SH                               0x058A
#define KSZ8463_TS12_SMPL1_SUB_NS                           0x058C
#define KSZ8463_TS12_SMPL2_NSL                              0x0594
#define KSZ8463_TS12_SMPL2_NSH                              0x0596
#define KSZ8463_TS12_SMP2_SL                                0x0598
#define KSZ8463_TS12_SMPL2_SH                               0x059A
#define KSZ8463_TS12_SMPL2_SUB_NS                           0x059C
#define KSZ8463_TS12_SMPL3_NSL                              0x05A4
#define KSZ8463_TS12_SMPL3_NSH                              0x05A6
#define KSZ8463_TS12_SMPL3_SL                               0x05A8
#define KSZ8463_TS12_SMPL3_SH                               0x05AA
#define KSZ8463_TS12_SMPL3_SUB_NS                           0x05AC
#define KSZ8463_TS12_SMPL4_NSL                              0x05B4
#define KSZ8463_TS12_SMPL4_NSH                              0x05B6
#define KSZ8463_TS12_SMPL4_SL                               0x05B8
#define KSZ8463_TS12_SMPL4_SH                               0x05BA
#define KSZ8463_TS12_SMPL4_SUB_NS                           0x05BC
#define KSZ8463_TS12_SMPL5_NSL                              0x05C4
#define KSZ8463_TS12_SMPL5_NSH                              0x05C6
#define KSZ8463_TS12_SMPL5_SL                               0x05C8
#define KSZ8463_TS12_SMPL5_SH                               0x05CA
#define KSZ8463_TS12_SMPL5_SUB_NS                           0x05CC
#define KSZ8463_TS12_SMPL6_NSL                              0x05D4
#define KSZ8463_TS12_SMPL6_NSH                              0x05D6
#define KSZ8463_TS12_SMPL6_SL                               0x05D8
#define KSZ8463_TS12_SMPL6_SH                               0x05DA
#define KSZ8463_TS12_SMPL6_SUB_NS                           0x05DC
#define KSZ8463_TS12_SMPL7_NSL                              0x05E4
#define KSZ8463_TS12_SMPL7_NSH                              0x05E6
#define KSZ8463_TS12_SMPL7_SL                               0x05E8
#define KSZ8463_TS12_SMPL7_SH                               0x05EA
#define KSZ8463_TS12_SMPL7_SUB_NS                           0x05EC
#define KSZ8463_TS12_SMPL8_NSL                              0x05F4
#define KSZ8463_TS12_SMPL8_NSH                              0x05F6
#define KSZ8463_TS12_SMPL8_SL                               0x05F8
#define KSZ8463_TS12_SMPL8_SH                               0x05FA
#define KSZ8463_TS12_SMPL8_SUB_NS                           0x05FC
#define KSZ8463_PTP_CLK_CTL                                 0x0600
#define KSZ8463_PTP_RTC_NSL                                 0x0604
#define KSZ8463_PTP_RTC_NSH                                 0x0606
#define KSZ8463_PTP_RTC_SL                                  0x0608
#define KSZ8463_PTP_RTC_SH                                  0x060A
#define KSZ8463_PTP_RTC_PHASE                               0x060C
#define KSZ8463_PTP_SNS_RATE_L                              0x0610
#define KSZ8463_PTP_SNS_RATE_H                              0x0612
#define KSZ8463_PTP_TEMP_ADJ_DURA_L                         0x0614
#define KSZ8463_PTP_TEMP_ADJ_DURA_H                         0x0616
#define KSZ8463_PTP_MSG_CFG_1                               0x0620
#define KSZ8463_PTP_MSG_CFG_2                               0x0622
#define KSZ8463_PTP_DOMAIN_VER                              0x0624
#define KSZ8463_P1_PTP_RX_LATENCY                           0x0640
#define KSZ8463_P1_PTP_TX_LATENCY                           0x0642
#define KSZ8463_P1_PTP_ASYM_COR                             0x0644
#define KSZ8463_P1_PTP_LINK_DLY                             0x0646
#define KSZ8463_P1_PTP_XDLY_REQ_TSL                         0x0648
#define KSZ8463_P1_PTP_XDLY_REQ_TSH                         0x064A
#define KSZ8463_P1_PTP_SYNC_TSL                             0x064C
#define KSZ8463_P1_PTP_SYNC_TSH                             0x064E
#define KSZ8463_P1_PTP_PDLY_RESP_TSL                        0x0650
#define KSZ8463_P1_PTP_PDLY_RESP_TSH                        0x0652
#define KSZ8463_P2_PTP_RX_LATENCY                           0x0660
#define KSZ8463_P2_PTP_TX_LATENCY                           0x0662
#define KSZ8463_P2_PTP_ASYM_COR                             0x0664
#define KSZ8463_P2_PTP_LINK_DLY                             0x0666
#define KSZ8463_P2_PTP_XDLY_REQ_TSL                         0x0668
#define KSZ8463_P2_PTP_XDLY_REQ_TSH                         0x066A
#define KSZ8463_P2_PTP_SYNC_TSL                             0x066C
#define KSZ8463_P2_PTP_SYNC_TSH                             0x066E
#define KSZ8463_P2_PTP_PDLY_RESP_TSL                        0x0670
#define KSZ8463_P2_PTP_PDLY_RESP_TSH                        0x0672
#define KSZ8463_GPIO_MONITOR                                0x0680
#define KSZ8463_GPIO_OEN                                    0x0682
#define KSZ8463_PTP_TRIG_IS                                 0x0688
#define KSZ8463_PTP_TRIG_IE                                 0x068A
#define KSZ8463_PTP_TS_IS                                   0x068C
#define KSZ8463_PTP_TS_IE                                   0x068E
#define KSZ8463_DSP_CNTRL_6                                 0x0734
#define KSZ8463_ANA_CNTRL_1                                 0x0748
#define KSZ8463_ANA_CNTRL_3                                 0x074C

//KSZ8463 Switch register access macros
#define KSZ8463_PnMBCR(port)                                (0x0040 + ((port) * 0x000C))
#define KSZ8463_PnMBSR(port)                                (0x0042 + ((port) * 0x000C))
#define KSZ8463_PHYnILR(port)                               (0x0044 + ((port) * 0x000C))
#define KSZ8463_PHYnIHR(port)                               (0x0046 + ((port) * 0x000C))
#define KSZ8463_PnANAR(port)                                (0x0048 + ((port) * 0x000C))
#define KSZ8463_PnANLPR(port)                               (0x004A + ((port) * 0x000C))
#define KSZ8463_PnPHYCTRL(port)                             (0x0062 + ((port) * 0x0004))
#define KSZ8463_PnCR1(port)                                 (0x0054 + ((port) * 0x0018))
#define KSZ8463_PnCR2(port)                                 (0x0056 + ((port) * 0x0018))
#define KSZ8463_PnVIDCR(port)                               (0x0058 + ((port) * 0x0018))
#define KSZ8463_PnCR3(port)                                 (0x005A + ((port) * 0x0018))
#define KSZ8463_PnIRCR0(port)                               (0x005C + ((port) * 0x0018))
#define KSZ8463_PnIRCR1(port)                               (0x005E + ((port) * 0x0018))
#define KSZ8463_PnERCR0(port)                               (0x0060 + ((port) * 0x0018))
#define KSZ8463_PnERCR1(port)                               (0x0062 + ((port) * 0x0018))
#define KSZ8463_PnSCSLMD(port)                              (0x0064 + ((port) * 0x0018))
#define KSZ8463_PnCR4(port)                                 (0x0066 + ((port) * 0x0018))
#define KSZ8463_PnSR(port)                                  (0x0068 + ((port) * 0x0018))
#define KSZ8463_PnTXQRCR1(port)                             (0x00C4 + ((port) * 0x0004))
#define KSZ8463_PnTXQRCR2(port)                             (0x00C6 + ((port) * 0x0004))
#define KSZ8463_PnANPT(port)                                (0x00D0 + ((port) * 0x000C))
#define KSZ8463_PnALPRNP(port)                              (0x00D2 + ((port) * 0x000C))
#define KSZ8463_PnEEEA(port)                                (0x00D4 + ((port) * 0x000C))
#define KSZ8463_PnEEEWEC(port)                              (0x00D6 + ((port) * 0x000C))
#define KSZ8463_PnEEECS(port)                               (0x00D8 + ((port) * 0x000C))
#define KSZ8463_PnLPIRTC(port)                              (0x00DA + ((port) * 0x000C))
#define KSZ8463_Pn_PTP_RX_LATENCY(port)                     (0x0620 + ((port) * 0x0020))
#define KSZ8463_Pn_PTP_TX_LATENCY(port)                     (0x0622 + ((port) * 0x0020))
#define KSZ8463_Pn_PTP_ASYM_COR(port)                       (0x0624 + ((port) * 0x0020))
#define KSZ8463_Pn_PTP_LINK_DLY(port)                       (0x0626 + ((port) * 0x0020))
#define KSZ8463_Pn_PTP_XDLY_REQ_TSL(port)                   (0x0628 + ((port) * 0x0020))
#define KSZ8463_Pn_PTP_XDLY_REQ_TSH(port)                   (0x062A + ((port) * 0x0020))
#define KSZ8463_Pn_PTP_SYNC_TSL(port)                       (0x062C + ((port) * 0x0020))
#define KSZ8463_Pn_PTP_SYNC_TSH(port)                       (0x062E + ((port) * 0x0020))
#define KSZ8463_Pn_PTP_PDLY_RESP_TSL(port)                  (0x0630 + ((port) * 0x0020))
#define KSZ8463_Pn_PTP_PDLY_RESP_TSH(port)                  (0x0632 + ((port) * 0x0020))

//Basic Control register
#define KSZ8463_BMCR_LOOPBACK                               0x4000
#define KSZ8463_BMCR_FORCE_100                              0x2000
#define KSZ8463_BMCR_AN_EN                                  0x1000
#define KSZ8463_BMCR_POWER_DOWN                             0x0800
#define KSZ8463_BMCR_ISOLATE                                0x0400
#define KSZ8463_BMCR_RESTART_AN                             0x0200
#define KSZ8463_BMCR_FORCE_FULL_DUPLEX                      0x0100
#define KSZ8463_BMCR_COL_TEST                               0x0080
#define KSZ8463_BMCR_HP_MDIX                                0x0020
#define KSZ8463_BMCR_FORCE_MDI                              0x0010
#define KSZ8463_BMCR_AUTO_MDIX_DIS                          0x0008
#define KSZ8463_BMCR_FAR_END_FAULT_DIS                      0x0004
#define KSZ8463_BMCR_TRANSMIT_DIS                           0x0002
#define KSZ8463_BMCR_LED_DIS                                0x0001

//Basic Status register
#define KSZ8463_BMSR_100BT4                                 0x8000
#define KSZ8463_BMSR_100BTX_FD                              0x4000
#define KSZ8463_BMSR_100BTX_HD                              0x2000
#define KSZ8463_BMSR_10BT_FD                                0x1000
#define KSZ8463_BMSR_10BT_HD                                0x0800
#define KSZ8463_BMSR_PREAMBLE_SUPPR                         0x0040
#define KSZ8463_BMSR_AN_COMPLETE                            0x0020
#define KSZ8463_BMSR_FAR_END_FAULT                          0x0010
#define KSZ8463_BMSR_AN_CAPABLE                             0x0008
#define KSZ8463_BMSR_LINK_STATUS                            0x0004
#define KSZ8463_BMSR_JABBER_TEST                            0x0002
#define KSZ8463_BMSR_EXTENDED_CAPABLE                       0x0001

//PHYID High register
#define KSZ8463_PHYID1_DEFAULT                              0x0022

//PHYID Low register
#define KSZ8463_PHYID2_DEFAULT                              0x1430

//Auto-Negotiation Advertisement Ability register
#define KSZ8463_ANAR_NEXT_PAGE                              0x8000
#define KSZ8463_ANAR_REMOTE_FAULT                           0x2000
#define KSZ8463_ANAR_PAUSE                                  0x0400
#define KSZ8463_ANAR_100BTX_FD                              0x0100
#define KSZ8463_ANAR_100BTX_HD                              0x0080
#define KSZ8463_ANAR_10BT_FD                                0x0040
#define KSZ8463_ANAR_10BT_HD                                0x0020
#define KSZ8463_ANAR_SELECTOR                               0x001F
#define KSZ8463_ANAR_SELECTOR_DEFAULT                       0x0001

//Auto-Negotiation Link Partner Ability register
#define KSZ8463_ANLPAR_NEXT_PAGE                            0x8000
#define KSZ8463_ANLPAR_LP_ACK                               0x4000
#define KSZ8463_ANLPAR_REMOTE_FAULT                         0x2000
#define KSZ8463_ANLPAR_PAUSE                                0x0400
#define KSZ8463_ANLPAR_100BTX_FD                            0x0100
#define KSZ8463_ANLPAR_100BTX_HD                            0x0080
#define KSZ8463_ANLPAR_10BT_FD                              0x0040
#define KSZ8463_ANLPAR_10BT_HD                              0x0020

//LinkMD Control/Status register
#define KSZ8463_LINKMD_TEST_EN                              0x8000
#define KSZ8463_LINKMD_RESULT                               0x6000
#define KSZ8463_LINKMD_SHORT                                0x1000
#define KSZ8463_LINKMD_FAULT_COUNT                          0x01FF

//PHY Special Control/Status register
#define KSZ8463_PHYSCS_POL_REVERSE                          0x0020
#define KSZ8463_PHYSCS_MDIX_STATUS                          0x0010
#define KSZ8463_PHYSCS_FORCE_LINK                           0x0008
#define KSZ8463_PHYSCS_EEE_EN                               0x0004
#define KSZ8463_PHYSCS_REMOTE_LOOPBACK                      0x0002

//Chip ID And Enable register
#define KSZ8463_CIDER_FAMILY_ID                             0xFF00
#define KSZ8463_CIDER_FAMILY_ID_DEFAULT                     0x8400
#define KSZ8463_CIDER_CHIP_ID                               0x00F0
#define KSZ8463_CIDER_CHIP_ID_ML_FML                        0x0040
#define KSZ8463_CIDER_CHIP_ID_RL_FRL                        0x0050
#define KSZ8463_CIDER_REVISION_ID                           0x000E
#define KSZ8463_CIDER_START_SWITCH                          0x0001

//Switch Global Control 1 register
#define KSZ8463_SGCR1_PASS_ALL_FRAMES                       0x8000
#define KSZ8463_SGCR1_RX_2K_PKT_LEN_EN                      0x4000
#define KSZ8463_SGCR1_TX_FLOW_CTRL_EN                       0x2000
#define KSZ8463_SGCR1_RX_FLOW_CTRL_EN                       0x1000
#define KSZ8463_SGCR1_FRAME_LEN_CHECK_EN                    0x0800
#define KSZ8463_SGCR1_AGING_EN                              0x0400
#define KSZ8463_SGCR1_FAST_AGE_EN                           0x0200
#define KSZ8463_SGCR1_AGGRESSIVE_BACK_OFF_EN                0x0100
#define KSZ8463_SGCR1_IG_LIMIT_FLOW_CTRL_EN                 0x0020
#define KSZ8463_SGCR1_RX_2K_PKT_EN                          0x0010
#define KSZ8463_SGCR1_PASS_FLOW_CTRL_PKT                    0x0008
#define KSZ8463_SGCR1_LINK_CHANGE_AGE                       0x0001

//Switch Global Control 2 register
#define KSZ8463_SGCR2_VLAN_EN                               0x8000
#define KSZ8463_SGCR2_IGMP_SNOOP_EN                         0x4000
#define KSZ8463_SGCR2_IPV6_MLD_SNOOP_EN                     0x2000
#define KSZ8463_SGCR2_IPV6_MLD_SNOOP_OPT_SEL                0x1000
#define KSZ8463_SGCR2_SNIFF_MODE_SEL                        0x0100
#define KSZ8463_SGCR2_UNI_VLAN_MISMATCH_DISCARD             0x0080
#define KSZ8463_SGCR2_MCAST_STORM_PROTECT_DIS               0x0040
#define KSZ8463_SGCR2_BACK_PRESSURE_MODE                    0x0020
#define KSZ8463_SGCR2_FLOW_CTRL_FAIR_MODE                   0x0010
#define KSZ8463_SGCR2_NO_EXCESSIVE_COL_DROP                 0x0008
#define KSZ8463_SGCR2_HUGE_PACKET_SUPPORT                   0x0004
#define KSZ8463_SGCR2_MAX_PACKET_SIZE_CHECK_EN              0x0002
#define KSZ8463_SGCR2_PRIO_BUFFER_RESERVE                   0x0001

//Switch Global Control 3 register
#define KSZ8463_SGCR3_BCAST_STORM_PROTECT_RATE_LSB          0xFF00
#define KSZ8463_SGCR3_SW_HOST_PORT_HALF_DUPLEX_MODE         0x0040
#define KSZ8463_SGCR3_SW_HOST_PORT_FLOW_CTRL_EN             0x0020
#define KSZ8463_SGCR3_SW_MII_10BT                           0x0010
#define KSZ8463_SGCR3_NULL_VID_REPLACEMENT                  0x0008
#define KSZ8463_SGCR3_BCAST_STORM_PROTECT_RATE_MSB          0x0007

//Switch Global Control 6 register
#define KSZ8463_SGCR6_TAG7                                  0xC000
#define KSZ8463_SGCR6_TAG6                                  0x3000
#define KSZ8463_SGCR6_TAG5                                  0x0C00
#define KSZ8463_SGCR6_TAG4                                  0x0300
#define KSZ8463_SGCR6_TAG3                                  0x00C0
#define KSZ8463_SGCR6_TAG2                                  0x0030
#define KSZ8463_SGCR6_TAG1                                  0x000C
#define KSZ8463_SGCR6_TAG0                                  0x0003

//Switch Global Control 7 register
#define KSZ8463_SGCR7_PORT_LED_MODE                         0x0300
#define KSZ8463_SGCR7_PORT_LED_MODE_LED1_SPD_LED2_LINK_ACT  0x0000
#define KSZ8463_SGCR7_PORT_LED_MODE_LED1_ACT_LED2_LINK      0x0100
#define KSZ8463_SGCR7_PORT_LED_MODE_LED1_FD_LED2_LINK_ACT   0x0200
#define KSZ8463_SGCR7_PORT_LED_MODE_LED1_FD_LED2_LINK       0x0300
#define KSZ8463_SGCR7_UNKNOWN_DEFAULT_PORT_EN               0x0080
#define KSZ8463_SGCR7_DRIVER_STRENGTH_SEL                   0x0060
#define KSZ8463_SGCR7_DRIVER_STRENGTH_SEL_4MA               0x0000
#define KSZ8463_SGCR7_DRIVER_STRENGTH_SEL_8MA               0x0020
#define KSZ8463_SGCR7_DRIVER_STRENGTH_SEL_12MA              0x0040
#define KSZ8463_SGCR7_DRIVER_STRENGTH_SEL_16MA              0x0060
#define KSZ8463_SGCR7_UNKNOWN_PKT_DEFAULT_PORT              0x0007
#define KSZ8463_SGCR7_UNKNOWN_PKT_DEFAULT_PORT_PORT1        0x0001
#define KSZ8463_SGCR7_UNKNOWN_PKT_DEFAULT_PORT_PORT2        0x0002
#define KSZ8463_SGCR7_UNKNOWN_PKT_DEFAULT_PORT_PORT3        0x0004

//Indirect Access Data 1 register
#define KSZ8463_IADR1_CPU_READ_STATUS                       0x0080
#define KSZ8463_IADR1_DATA                                  0x0007

//Indirect Access Control register
#define KSZ8463_IACR_WRITE                                  0x0000
#define KSZ8463_IACR_READ                                   0x1000
#define KSZ8463_IACR_TABLE_SEL                              0x0C00
#define KSZ8463_IACR_TABLE_SEL_STATIC_MAC                   0x0000
#define KSZ8463_IACR_TABLE_SEL_VLAN                         0x0400
#define KSZ8463_IACR_TABLE_SEL_DYNAMIC_MAC                  0x0800
#define KSZ8463_IACR_TABLE_SEL_MIB_COUNTER                  0x0C00
#define KSZ8463_IACR_ADDR                                   0x03FF

//Power Management Control and Wake-up Event Status register
#define KSZ8463_PMCTRL_LINK_UP_DETECT_STATUS                0x0008
#define KSZ8463_PMCTRL_ENERGY_DETECT_STATUS                 0x0004
#define KSZ8463_PMCTRL_PWR_MGMT_MODE                        0x0003
#define KSZ8463_PMCTRL_PWR_MGMT_MODE_NORMAL                 0x0000
#define KSZ8463_PMCTRL_PWR_MGMT_MODE_ENERGY_DETECT          0x0001
#define KSZ8463_PMCTRL_PWR_MGMT_MODE_SOFT_PWR_DOWN          0x0002

//Go Sleep Time register
#define KSZ8463_GST_GO_SLEEP_TIME                           0x00FF

//Clock Tree Power Down Control register
#define KSZ8463_CTPDC_PLL_AUTO_PWR_DOWN_EN                  0x0010
#define KSZ8463_CTPDC_SWITCH_CLK_AUTO_SHUTDOWN_EN           0x0008
#define KSZ8463_CTPDC_CPU_CLK_AUTO_SHUTDOWN_EN              0x0004
#define KSZ8463_CTPDC_SHUTDOWN_WAIT_PERIOD                  0x0003
#define KSZ8463_CTPDC_SHUTDOWN_WAIT_PERIOD_5_3S             0x0000
#define KSZ8463_CTPDC_SHUTDOWN_WAIT_PERIOD_1_6S             0x0001
#define KSZ8463_CTPDC_SHUTDOWN_WAIT_PERIOD_1_0MS            0x0002
#define KSZ8463_CTPDC_SHUTDOWN_WAIT_PERIOD_3_2US            0x0003

//Port N Control 2 register
#define KSZ8463_PnCR2_INGRESS_VLAN_FILT                     0x4000
#define KSZ8463_PnCR2_DISCARD_NON_PVID_PKT                  0x2000
#define KSZ8463_PnCR2_FORCE_FLOW_CTRL                       0x1000
#define KSZ8463_PnCR2_BACK_PRESSURE_EN                      0x0800
#define KSZ8463_PnCR2_TRANSMIT_EN                           0x0400
#define KSZ8463_PnCR2_RECEIVE_EN                            0x0200
#define KSZ8463_PnCR2_LEARNING_DIS                          0x0100
#define KSZ8463_PnCR2_SNIFFER_PORT                          0x0080
#define KSZ8463_PnCR2_RECEIVE_SNIFF                         0x0040
#define KSZ8463_PnCR2_TRANSMIT_SNIFF                        0x0020
#define KSZ8463_PnCR2_USER_PRIO_CEILING                     0x0008
#define KSZ8463_PnCR2_PORT_VLAN_MEMBERSHIP                  0x0007

//Port N VID Control register
#define KSZ8463_PnVIDCR_PRIORITY                            0xE000
#define KSZ8463_PnVIDCR_CFI                                 0x1000
#define KSZ8463_PnVIDCR_VID                                 0x0FFF

//Port N Status register
#define KSZ8463_PnSR_HP_MDIX                                0x8000
#define KSZ8463_PnSR_POL_REVERSE                            0x2000
#define KSZ8463_PnSR_TX_FLOW_CTRL_EN                        0x1000
#define KSZ8463_PnSR_RX_FLOW_CTRL_EN                        0x0800
#define KSZ8463_PnSR_OP_SPEED                               0x0400
#define KSZ8463_PnSR_OP_DUPLEX                              0x0200
#define KSZ8463_PnSR_FAR_END_FAULT                          0x0100
#define KSZ8463_PnSR_MDIX_STATUS                            0x0080
#define KSZ8463_PnSR_AN_DONE                                0x0040
#define KSZ8463_PnSR_LINK_STATUS                            0x0020
#define KSZ8463_PnSR_LP_FLOW_CTRL_CAPABLE                   0x0010
#define KSZ8463_PnSR_LP_100BTX_FD_CAPABLE                   0x0008
#define KSZ8463_PnSR_LP_100BTX_HF_CAPABLE                   0x0004
#define KSZ8463_PnSR_LP_10BT_FD_CAPABLE                     0x0002
#define KSZ8463_PnSR_LP_10BT_HD_CAPABLE                     0x0001

//Switch Global Control 8 register
#define KSZ8463_SGCR8_QUEUE_PRIO_MAPPING                    0xC000
#define KSZ8463_SGCR8_FLUSH_DYNAMIC_MAC_TABLE               0x0400
#define KSZ8463_SGCR8_FLUSH_STATIC_MAC_TABLE                0x0200
#define KSZ8463_SGCR8_TAIL_TAG_EN                           0x0100
#define KSZ8463_SGCR8_PAUSE_OFF_LIMIT_TIME                  0x00FF

//Global Reset register
#define KSZ8463_GRR_MEMORY_BIST_START                       0x0008
#define KSZ8463_GRR_PTP_SOFT_RESET                          0x0004
#define KSZ8463_GRR_GLOBAL_SOFT_RESET                       0x0001

//Interrupt Enable register
#define KSZ8463_IER_LC_IE                                   0x8000
#define KSZ8463_IER_PTP_TS_IE                               0x1000
#define KSZ8463_IER_PTP_TRIG_IE                             0x0400
#define KSZ8463_IER_LD_IE                                   0x0008
#define KSZ8463_IER_ED_IE                                   0x0004

//Interrupt Status register
#define KSZ8463_ISR_LC_IS                                   0x8000
#define KSZ8463_ISR_PTP_TS_IS                               0x1000
#define KSZ8463_ISR_PTP_TRIG_IS                             0x0400
#define KSZ8463_ISR_LD_IS                                   0x0008
#define KSZ8463_ISR_ED_IS                                   0x0004

//Trigger Output Unit Error register
#define KSZ8463_TRIG_ERR_UNIT12                             0x0800
#define KSZ8463_TRIG_ERR_UNIT11                             0x0400
#define KSZ8463_TRIG_ERR_UNIT10                             0x0200
#define KSZ8463_TRIG_ERR_UNIT9                              0x0100
#define KSZ8463_TRIG_ERR_UNIT8                              0x0080
#define KSZ8463_TRIG_ERR_UNIT7                              0x0040
#define KSZ8463_TRIG_ERR_UNIT6                              0x0020
#define KSZ8463_TRIG_ERR_UNIT5                              0x0010
#define KSZ8463_TRIG_ERR_UNIT4                              0x0008
#define KSZ8463_TRIG_ERR_UNIT3                              0x0004
#define KSZ8463_TRIG_ERR_UNIT2                              0x0002
#define KSZ8463_TRIG_ERR_UNIT1                              0x0001

//Trigger Output Unit Active register
#define KSZ8463_TRIG_ACTIVE_UNIT12                          0x0800
#define KSZ8463_TRIG_ACTIVE_UNIT11                          0x0400
#define KSZ8463_TRIG_ACTIVE_UNIT10                          0x0200
#define KSZ8463_TRIG_ACTIVE_UNIT9                           0x0100
#define KSZ8463_TRIG_ACTIVE_UNIT8                           0x0080
#define KSZ8463_TRIG_ACTIVE_UNIT7                           0x0040
#define KSZ8463_TRIG_ACTIVE_UNIT6                           0x0020
#define KSZ8463_TRIG_ACTIVE_UNIT5                           0x0010
#define KSZ8463_TRIG_ACTIVE_UNIT4                           0x0008
#define KSZ8463_TRIG_ACTIVE_UNIT3                           0x0004
#define KSZ8463_TRIG_ACTIVE_UNIT2                           0x0002
#define KSZ8463_TRIG_ACTIVE_UNIT1                           0x0001

//Trigger Output Unit Done register
#define KSZ8463_TRIG_DONE_UNIT12                            0x0800
#define KSZ8463_TRIG_DONE_UNIT11                            0x0400
#define KSZ8463_TRIG_DONE_UNIT10                            0x0200
#define KSZ8463_TRIG_DONE_UNIT9                             0x0100
#define KSZ8463_TRIG_DONE_UNIT8                             0x0080
#define KSZ8463_TRIG_DONE_UNIT7                             0x0040
#define KSZ8463_TRIG_DONE_UNIT6                             0x0020
#define KSZ8463_TRIG_DONE_UNIT5                             0x0010
#define KSZ8463_TRIG_DONE_UNIT4                             0x0008
#define KSZ8463_TRIG_DONE_UNIT3                             0x0004
#define KSZ8463_TRIG_DONE_UNIT2                             0x0002
#define KSZ8463_TRIG_DONE_UNIT1                             0x0001

//Trigger Output Unit Enable register
#define KSZ8463_TRIG_EN_UNIT12                              0x0800
#define KSZ8463_TRIG_EN_UNIT11                              0x0400
#define KSZ8463_TRIG_EN_UNIT10                              0x0200
#define KSZ8463_TRIG_EN_UNIT9                               0x0100
#define KSZ8463_TRIG_EN_UNIT8                               0x0080
#define KSZ8463_TRIG_EN_UNIT7                               0x0040
#define KSZ8463_TRIG_EN_UNIT6                               0x0020
#define KSZ8463_TRIG_EN_UNIT5                               0x0010
#define KSZ8463_TRIG_EN_UNIT4                               0x0008
#define KSZ8463_TRIG_EN_UNIT3                               0x0004
#define KSZ8463_TRIG_EN_UNIT2                               0x0002
#define KSZ8463_TRIG_EN_UNIT1                               0x0001

//Trigger Output Unit Software Reset register
#define KSZ8463_TRIG_SW_RST_UNIT12                          0x0800
#define KSZ8463_TRIG_SW_RST_UNIT11                          0x0400
#define KSZ8463_TRIG_SW_RST_UNIT10                          0x0200
#define KSZ8463_TRIG_SW_RST_UNIT9                           0x0100
#define KSZ8463_TRIG_SW_RST_UNIT8                           0x0080
#define KSZ8463_TRIG_SW_RST_UNIT7                           0x0040
#define KSZ8463_TRIG_SW_RST_UNIT6                           0x0020
#define KSZ8463_TRIG_SW_RST_UNIT5                           0x0010
#define KSZ8463_TRIG_SW_RST_UNIT4                           0x0008
#define KSZ8463_TRIG_SW_RST_UNIT3                           0x0004
#define KSZ8463_TRIG_SW_RST_UNIT2                           0x0002
#define KSZ8463_TRIG_SW_RST_UNIT1                           0x0001

//Trigger Output Unit 12 PPS Pulse Width register
#define KSZ8463_TRIG12_PPS_WIDTH_PATH_DELAY_TRIG1           0x0700
#define KSZ8463_TRIG12_PPS_WIDTH_PPS_PULSE_WIDTH_MSB_TRIG12 0x00FF

//Trigger Output Unit 1 Configuration/Control 1 register
#define KSZ8463_TRIG1_CFG_1_CASCADE_MODE                    0x8000
#define KSZ8463_TRIG1_CFG_1_TAIL_UNIT                       0x4000
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT              0x3C00
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1        0x0000
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2        0x0400
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3        0x0800
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4        0x0C00
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5        0x1000
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6        0x1400
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7        0x1800
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8        0x1C00
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9        0x2000
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10       0x2400
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11       0x2800
#define KSZ8463_TRIG1_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12       0x2C00
#define KSZ8463_TRIG1_CFG_1_TRIGGER_NOW                     0x0200
#define KSZ8463_TRIG1_CFG_1_TRIGGER_NOTIFY                  0x0100
#define KSZ8463_TRIG1_CFG_1_PATTERN                         0x0070
#define KSZ8463_TRIG1_CFG_1_PATTERN_TRIG_NEG_EDGE           0x0000
#define KSZ8463_TRIG1_CFG_1_PATTERN_TRIG_POS_EDGE           0x0010
#define KSZ8463_TRIG1_CFG_1_PATTERN_TRIG_NEG_PULSE          0x0020
#define KSZ8463_TRIG1_CFG_1_PATTERN_TRIG_POS_PULSE          0x0030
#define KSZ8463_TRIG1_CFG_1_PATTERN_TRIG_NEG_CYCLE          0x0040
#define KSZ8463_TRIG1_CFG_1_PATTERN_TRIG_POS_CYCLE          0x0050
#define KSZ8463_TRIG1_CFG_1_PATTERN_TRIG_REG_OUTPUT         0x0060
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL                        0x000F
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO0                  0x0000
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO1                  0x0001
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO2                  0x0002
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO3                  0x0003
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO4                  0x0004
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO5                  0x0005
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO6                  0x0006
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO7                  0x0007
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO8                  0x0008
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO9                  0x0009
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO10                 0x000A
#define KSZ8463_TRIG1_CFG_1_GPIO_SEL_GPIO11                 0x000B

//Trigger Output Unit 2 Configuration/Control 1 register
#define KSZ8463_TRIG2_CFG_1_CASCADE_MODE                    0x8000
#define KSZ8463_TRIG2_CFG_1_TAIL_UNIT                       0x4000
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT              0x3C00
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1        0x0000
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2        0x0400
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3        0x0800
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4        0x0C00
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5        0x1000
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6        0x1400
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7        0x1800
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8        0x1C00
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9        0x2000
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10       0x2400
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11       0x2800
#define KSZ8463_TRIG2_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12       0x2C00
#define KSZ8463_TRIG2_CFG_1_TRIGGER_NOW                     0x0200
#define KSZ8463_TRIG2_CFG_1_TRIGGER_NOTIFY                  0x0100
#define KSZ8463_TRIG2_CFG_1_PATTERN                         0x0070
#define KSZ8463_TRIG2_CFG_1_PATTERN_TRIG_NEG_EDGE           0x0000
#define KSZ8463_TRIG2_CFG_1_PATTERN_TRIG_POS_EDGE           0x0010
#define KSZ8463_TRIG2_CFG_1_PATTERN_TRIG_NEG_PULSE          0x0020
#define KSZ8463_TRIG2_CFG_1_PATTERN_TRIG_POS_PULSE          0x0030
#define KSZ8463_TRIG2_CFG_1_PATTERN_TRIG_NEG_CYCLE          0x0040
#define KSZ8463_TRIG2_CFG_1_PATTERN_TRIG_POS_CYCLE          0x0050
#define KSZ8463_TRIG2_CFG_1_PATTERN_TRIG_REG_OUTPUT         0x0060
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL                        0x000F
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO0                  0x0000
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO1                  0x0001
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO2                  0x0002
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO3                  0x0003
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO4                  0x0004
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO5                  0x0005
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO6                  0x0006
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO7                  0x0007
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO8                  0x0008
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO9                  0x0009
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO10                 0x000A
#define KSZ8463_TRIG2_CFG_1_GPIO_SEL_GPIO11                 0x000B

//Trigger Output Unit 3 Configuration/Control 1 register
#define KSZ8463_TRIG3_CFG_1_CASCADE_MODE                    0x8000
#define KSZ8463_TRIG3_CFG_1_TAIL_UNIT                       0x4000
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT              0x3C00
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1        0x0000
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2        0x0400
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3        0x0800
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4        0x0C00
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5        0x1000
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6        0x1400
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7        0x1800
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8        0x1C00
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9        0x2000
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10       0x2400
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11       0x2800
#define KSZ8463_TRIG3_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12       0x2C00
#define KSZ8463_TRIG3_CFG_1_TRIGGER_NOW                     0x0200
#define KSZ8463_TRIG3_CFG_1_TRIGGER_NOTIFY                  0x0100
#define KSZ8463_TRIG3_CFG_1_PATTERN                         0x0070
#define KSZ8463_TRIG3_CFG_1_PATTERN_TRIG_NEG_EDGE           0x0000
#define KSZ8463_TRIG3_CFG_1_PATTERN_TRIG_POS_EDGE           0x0010
#define KSZ8463_TRIG3_CFG_1_PATTERN_TRIG_NEG_PULSE          0x0020
#define KSZ8463_TRIG3_CFG_1_PATTERN_TRIG_POS_PULSE          0x0030
#define KSZ8463_TRIG3_CFG_1_PATTERN_TRIG_NEG_CYCLE          0x0040
#define KSZ8463_TRIG3_CFG_1_PATTERN_TRIG_POS_CYCLE          0x0050
#define KSZ8463_TRIG3_CFG_1_PATTERN_TRIG_REG_OUTPUT         0x0060
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL                        0x000F
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO0                  0x0000
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO1                  0x0001
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO2                  0x0002
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO3                  0x0003
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO4                  0x0004
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO5                  0x0005
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO6                  0x0006
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO7                  0x0007
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO8                  0x0008
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO9                  0x0009
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO10                 0x000A
#define KSZ8463_TRIG3_CFG_1_GPIO_SEL_GPIO11                 0x000B

//Trigger Output Unit 4 Configuration/Control 1 register
#define KSZ8463_TRIG4_CFG_1_CASCADE_MODE                    0x8000
#define KSZ8463_TRIG4_CFG_1_TAIL_UNIT                       0x4000
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT              0x3C00
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1        0x0000
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2        0x0400
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3        0x0800
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4        0x0C00
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5        0x1000
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6        0x1400
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7        0x1800
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8        0x1C00
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9        0x2000
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10       0x2400
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11       0x2800
#define KSZ8463_TRIG4_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12       0x2C00
#define KSZ8463_TRIG4_CFG_1_TRIGGER_NOW                     0x0200
#define KSZ8463_TRIG4_CFG_1_TRIGGER_NOTIFY                  0x0100
#define KSZ8463_TRIG4_CFG_1_PATTERN                         0x0070
#define KSZ8463_TRIG4_CFG_1_PATTERN_TRIG_NEG_EDGE           0x0000
#define KSZ8463_TRIG4_CFG_1_PATTERN_TRIG_POS_EDGE           0x0010
#define KSZ8463_TRIG4_CFG_1_PATTERN_TRIG_NEG_PULSE          0x0020
#define KSZ8463_TRIG4_CFG_1_PATTERN_TRIG_POS_PULSE          0x0030
#define KSZ8463_TRIG4_CFG_1_PATTERN_TRIG_NEG_CYCLE          0x0040
#define KSZ8463_TRIG4_CFG_1_PATTERN_TRIG_POS_CYCLE          0x0050
#define KSZ8463_TRIG4_CFG_1_PATTERN_TRIG_REG_OUTPUT         0x0060
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL                        0x000F
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO0                  0x0000
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO1                  0x0001
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO2                  0x0002
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO3                  0x0003
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO4                  0x0004
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO5                  0x0005
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO6                  0x0006
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO7                  0x0007
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO8                  0x0008
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO9                  0x0009
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO10                 0x000A
#define KSZ8463_TRIG4_CFG_1_GPIO_SEL_GPIO11                 0x000B

//Trigger Output Unit 5 Configuration/Control 1 register
#define KSZ8463_TRIG5_CFG_1_CASCADE_MODE                    0x8000
#define KSZ8463_TRIG5_CFG_1_TAIL_UNIT                       0x4000
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT              0x3C00
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1        0x0000
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2        0x0400
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3        0x0800
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4        0x0C00
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5        0x1000
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6        0x1400
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7        0x1800
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8        0x1C00
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9        0x2000
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10       0x2400
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11       0x2800
#define KSZ8463_TRIG5_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12       0x2C00
#define KSZ8463_TRIG5_CFG_1_TRIGGER_NOW                     0x0200
#define KSZ8463_TRIG5_CFG_1_TRIGGER_NOTIFY                  0x0100
#define KSZ8463_TRIG5_CFG_1_PATTERN                         0x0070
#define KSZ8463_TRIG5_CFG_1_PATTERN_TRIG_NEG_EDGE           0x0000
#define KSZ8463_TRIG5_CFG_1_PATTERN_TRIG_POS_EDGE           0x0010
#define KSZ8463_TRIG5_CFG_1_PATTERN_TRIG_NEG_PULSE          0x0020
#define KSZ8463_TRIG5_CFG_1_PATTERN_TRIG_POS_PULSE          0x0030
#define KSZ8463_TRIG5_CFG_1_PATTERN_TRIG_NEG_CYCLE          0x0040
#define KSZ8463_TRIG5_CFG_1_PATTERN_TRIG_POS_CYCLE          0x0050
#define KSZ8463_TRIG5_CFG_1_PATTERN_TRIG_REG_OUTPUT         0x0060
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL                        0x000F
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO0                  0x0000
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO1                  0x0001
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO2                  0x0002
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO3                  0x0003
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO4                  0x0004
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO5                  0x0005
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO6                  0x0006
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO7                  0x0007
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO8                  0x0008
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO9                  0x0009
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO10                 0x000A
#define KSZ8463_TRIG5_CFG_1_GPIO_SEL_GPIO11                 0x000B

//Trigger Output Unit 6 Configuration/Control 1 register
#define KSZ8463_TRIG6_CFG_1_CASCADE_MODE                    0x8000
#define KSZ8463_TRIG6_CFG_1_TAIL_UNIT                       0x4000
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT              0x3C00
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1        0x0000
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2        0x0400
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3        0x0800
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4        0x0C00
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5        0x1000
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6        0x1400
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7        0x1800
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8        0x1C00
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9        0x2000
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10       0x2400
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11       0x2800
#define KSZ8463_TRIG6_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12       0x2C00
#define KSZ8463_TRIG6_CFG_1_TRIGGER_NOW                     0x0200
#define KSZ8463_TRIG6_CFG_1_TRIGGER_NOTIFY                  0x0100
#define KSZ8463_TRIG6_CFG_1_PATTERN                         0x0070
#define KSZ8463_TRIG6_CFG_1_PATTERN_TRIG_NEG_EDGE           0x0000
#define KSZ8463_TRIG6_CFG_1_PATTERN_TRIG_POS_EDGE           0x0010
#define KSZ8463_TRIG6_CFG_1_PATTERN_TRIG_NEG_PULSE          0x0020
#define KSZ8463_TRIG6_CFG_1_PATTERN_TRIG_POS_PULSE          0x0030
#define KSZ8463_TRIG6_CFG_1_PATTERN_TRIG_NEG_CYCLE          0x0040
#define KSZ8463_TRIG6_CFG_1_PATTERN_TRIG_POS_CYCLE          0x0050
#define KSZ8463_TRIG6_CFG_1_PATTERN_TRIG_REG_OUTPUT         0x0060
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL                        0x000F
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO0                  0x0000
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO1                  0x0001
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO2                  0x0002
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO3                  0x0003
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO4                  0x0004
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO5                  0x0005
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO6                  0x0006
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO7                  0x0007
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO8                  0x0008
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO9                  0x0009
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO10                 0x000A
#define KSZ8463_TRIG6_CFG_1_GPIO_SEL_GPIO11                 0x000B

//Trigger Output Unit 7 Configuration/Control 1 register
#define KSZ8463_TRIG7_CFG_1_CASCADE_MODE                    0x8000
#define KSZ8463_TRIG7_CFG_1_TAIL_UNIT                       0x4000
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT              0x3C00
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1        0x0000
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2        0x0400
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3        0x0800
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4        0x0C00
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5        0x1000
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6        0x1400
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7        0x1800
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8        0x1C00
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9        0x2000
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10       0x2400
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11       0x2800
#define KSZ8463_TRIG7_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12       0x2C00
#define KSZ8463_TRIG7_CFG_1_TRIGGER_NOW                     0x0200
#define KSZ8463_TRIG7_CFG_1_TRIGGER_NOTIFY                  0x0100
#define KSZ8463_TRIG7_CFG_1_PATTERN                         0x0070
#define KSZ8463_TRIG7_CFG_1_PATTERN_TRIG_NEG_EDGE           0x0000
#define KSZ8463_TRIG7_CFG_1_PATTERN_TRIG_POS_EDGE           0x0010
#define KSZ8463_TRIG7_CFG_1_PATTERN_TRIG_NEG_PULSE          0x0020
#define KSZ8463_TRIG7_CFG_1_PATTERN_TRIG_POS_PULSE          0x0030
#define KSZ8463_TRIG7_CFG_1_PATTERN_TRIG_NEG_CYCLE          0x0040
#define KSZ8463_TRIG7_CFG_1_PATTERN_TRIG_POS_CYCLE          0x0050
#define KSZ8463_TRIG7_CFG_1_PATTERN_TRIG_REG_OUTPUT         0x0060
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL                        0x000F
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO0                  0x0000
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO1                  0x0001
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO2                  0x0002
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO3                  0x0003
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO4                  0x0004
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO5                  0x0005
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO6                  0x0006
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO7                  0x0007
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO8                  0x0008
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO9                  0x0009
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO10                 0x000A
#define KSZ8463_TRIG7_CFG_1_GPIO_SEL_GPIO11                 0x000B

//Trigger Output Unit 8 Configuration/Control 1 register
#define KSZ8463_TRIG8_CFG_1_CASCADE_MODE                    0x8000
#define KSZ8463_TRIG8_CFG_1_TAIL_UNIT                       0x4000
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT              0x3C00
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1        0x0000
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2        0x0400
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3        0x0800
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4        0x0C00
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5        0x1000
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6        0x1400
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7        0x1800
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8        0x1C00
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9        0x2000
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10       0x2400
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11       0x2800
#define KSZ8463_TRIG8_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12       0x2C00
#define KSZ8463_TRIG8_CFG_1_TRIGGER_NOW                     0x0200
#define KSZ8463_TRIG8_CFG_1_TRIGGER_NOTIFY                  0x0100
#define KSZ8463_TRIG8_CFG_1_PATTERN                         0x0070
#define KSZ8463_TRIG8_CFG_1_PATTERN_TRIG_NEG_EDGE           0x0000
#define KSZ8463_TRIG8_CFG_1_PATTERN_TRIG_POS_EDGE           0x0010
#define KSZ8463_TRIG8_CFG_1_PATTERN_TRIG_NEG_PULSE          0x0020
#define KSZ8463_TRIG8_CFG_1_PATTERN_TRIG_POS_PULSE          0x0030
#define KSZ8463_TRIG8_CFG_1_PATTERN_TRIG_NEG_CYCLE          0x0040
#define KSZ8463_TRIG8_CFG_1_PATTERN_TRIG_POS_CYCLE          0x0050
#define KSZ8463_TRIG8_CFG_1_PATTERN_TRIG_REG_OUTPUT         0x0060
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL                        0x000F
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO0                  0x0000
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO1                  0x0001
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO2                  0x0002
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO3                  0x0003
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO4                  0x0004
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO5                  0x0005
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO6                  0x0006
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO7                  0x0007
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO8                  0x0008
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO9                  0x0009
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO10                 0x000A
#define KSZ8463_TRIG8_CFG_1_GPIO_SEL_GPIO11                 0x000B

//Trigger Output Unit 9 Configuration/Control 1 register
#define KSZ8463_TRIG9_CFG_1_CASCADE_MODE                    0x8000
#define KSZ8463_TRIG9_CFG_1_TAIL_UNIT                       0x4000
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT              0x3C00
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1        0x0000
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2        0x0400
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3        0x0800
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4        0x0C00
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5        0x1000
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6        0x1400
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7        0x1800
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8        0x1C00
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9        0x2000
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10       0x2400
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11       0x2800
#define KSZ8463_TRIG9_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12       0x2C00
#define KSZ8463_TRIG9_CFG_1_TRIGGER_NOW                     0x0200
#define KSZ8463_TRIG9_CFG_1_TRIGGER_NOTIFY                  0x0100
#define KSZ8463_TRIG9_CFG_1_PATTERN                         0x0070
#define KSZ8463_TRIG9_CFG_1_PATTERN_TRIG_NEG_EDGE           0x0000
#define KSZ8463_TRIG9_CFG_1_PATTERN_TRIG_POS_EDGE           0x0010
#define KSZ8463_TRIG9_CFG_1_PATTERN_TRIG_NEG_PULSE          0x0020
#define KSZ8463_TRIG9_CFG_1_PATTERN_TRIG_POS_PULSE          0x0030
#define KSZ8463_TRIG9_CFG_1_PATTERN_TRIG_NEG_CYCLE          0x0040
#define KSZ8463_TRIG9_CFG_1_PATTERN_TRIG_POS_CYCLE          0x0050
#define KSZ8463_TRIG9_CFG_1_PATTERN_TRIG_REG_OUTPUT         0x0060
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL                        0x000F
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO0                  0x0000
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO1                  0x0001
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO2                  0x0002
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO3                  0x0003
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO4                  0x0004
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO5                  0x0005
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO6                  0x0006
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO7                  0x0007
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO8                  0x0008
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO9                  0x0009
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO10                 0x000A
#define KSZ8463_TRIG9_CFG_1_GPIO_SEL_GPIO11                 0x000B

//Trigger Output Unit 10 Configuration/Control 1 register
#define KSZ8463_TRIG10_CFG_1_CASCADE_MODE                   0x8000
#define KSZ8463_TRIG10_CFG_1_TAIL_UNIT                      0x4000
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT             0x3C00
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1       0x0000
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2       0x0400
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3       0x0800
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4       0x0C00
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5       0x1000
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6       0x1400
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7       0x1800
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8       0x1C00
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9       0x2000
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10      0x2400
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11      0x2800
#define KSZ8463_TRIG10_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12      0x2C00
#define KSZ8463_TRIG10_CFG_1_TRIGGER_NOW                    0x0200
#define KSZ8463_TRIG10_CFG_1_TRIGGER_NOTIFY                 0x0100
#define KSZ8463_TRIG10_CFG_1_PATTERN                        0x0070
#define KSZ8463_TRIG10_CFG_1_PATTERN_TRIG_NEG_EDGE          0x0000
#define KSZ8463_TRIG10_CFG_1_PATTERN_TRIG_POS_EDGE          0x0010
#define KSZ8463_TRIG10_CFG_1_PATTERN_TRIG_NEG_PULSE         0x0020
#define KSZ8463_TRIG10_CFG_1_PATTERN_TRIG_POS_PULSE         0x0030
#define KSZ8463_TRIG10_CFG_1_PATTERN_TRIG_NEG_CYCLE         0x0040
#define KSZ8463_TRIG10_CFG_1_PATTERN_TRIG_POS_CYCLE         0x0050
#define KSZ8463_TRIG10_CFG_1_PATTERN_TRIG_REG_OUTPUT        0x0060
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL                       0x000F
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO0                 0x0000
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO1                 0x0001
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO2                 0x0002
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO3                 0x0003
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO4                 0x0004
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO5                 0x0005
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO6                 0x0006
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO7                 0x0007
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO8                 0x0008
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO9                 0x0009
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO10                0x000A
#define KSZ8463_TRIG10_CFG_1_GPIO_SEL_GPIO11                0x000B

//Trigger Output Unit 11 Configuration/Control 1 register
#define KSZ8463_TRIG11_CFG_1_CASCADE_MODE                   0x8000
#define KSZ8463_TRIG11_CFG_1_TAIL_UNIT                      0x4000
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT             0x3C00
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1       0x0000
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2       0x0400
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3       0x0800
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4       0x0C00
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5       0x1000
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6       0x1400
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7       0x1800
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8       0x1C00
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9       0x2000
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10      0x2400
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11      0x2800
#define KSZ8463_TRIG11_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12      0x2C00
#define KSZ8463_TRIG11_CFG_1_TRIGGER_NOW                    0x0200
#define KSZ8463_TRIG11_CFG_1_TRIGGER_NOTIFY                 0x0100
#define KSZ8463_TRIG11_CFG_1_PATTERN                        0x0070
#define KSZ8463_TRIG11_CFG_1_PATTERN_TRIG_NEG_EDGE          0x0000
#define KSZ8463_TRIG11_CFG_1_PATTERN_TRIG_POS_EDGE          0x0010
#define KSZ8463_TRIG11_CFG_1_PATTERN_TRIG_NEG_PULSE         0x0020
#define KSZ8463_TRIG11_CFG_1_PATTERN_TRIG_POS_PULSE         0x0030
#define KSZ8463_TRIG11_CFG_1_PATTERN_TRIG_NEG_CYCLE         0x0040
#define KSZ8463_TRIG11_CFG_1_PATTERN_TRIG_POS_CYCLE         0x0050
#define KSZ8463_TRIG11_CFG_1_PATTERN_TRIG_REG_OUTPUT        0x0060
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL                       0x000F
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO0                 0x0000
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO1                 0x0001
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO2                 0x0002
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO3                 0x0003
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO4                 0x0004
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO5                 0x0005
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO6                 0x0006
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO7                 0x0007
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO8                 0x0008
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO9                 0x0009
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO10                0x000A
#define KSZ8463_TRIG11_CFG_1_GPIO_SEL_GPIO11                0x000B

//Trigger Output Unit 12 Configuration/Control 1 register
#define KSZ8463_TRIG12_CFG_1_CASCADE_MODE                   0x8000
#define KSZ8463_TRIG12_CFG_1_TAIL_UNIT                      0x4000
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT             0x3C00
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT1       0x0000
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT2       0x0400
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT3       0x0800
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT4       0x0C00
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT5       0x1000
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT6       0x1400
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT7       0x1800
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT8       0x1C00
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT9       0x2000
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT10      0x2400
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT11      0x2800
#define KSZ8463_TRIG12_CFG_1_UPSTREAM_TRIG_UNIT_UNIT12      0x2C00
#define KSZ8463_TRIG12_CFG_1_TRIGGER_NOW                    0x0200
#define KSZ8463_TRIG12_CFG_1_TRIGGER_NOTIFY                 0x0100
#define KSZ8463_TRIG12_CFG_1_PATTERN                        0x0070
#define KSZ8463_TRIG12_CFG_1_PATTERN_TRIG_NEG_EDGE          0x0000
#define KSZ8463_TRIG12_CFG_1_PATTERN_TRIG_POS_EDGE          0x0010
#define KSZ8463_TRIG12_CFG_1_PATTERN_TRIG_NEG_PULSE         0x0020
#define KSZ8463_TRIG12_CFG_1_PATTERN_TRIG_POS_PULSE         0x0030
#define KSZ8463_TRIG12_CFG_1_PATTERN_TRIG_NEG_CYCLE         0x0040
#define KSZ8463_TRIG12_CFG_1_PATTERN_TRIG_POS_CYCLE         0x0050
#define KSZ8463_TRIG12_CFG_1_PATTERN_TRIG_REG_OUTPUT        0x0060
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL                       0x000F
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO0                 0x0000
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO1                 0x0001
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO2                 0x0002
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO3                 0x0003
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO4                 0x0004
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO5                 0x0005
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO6                 0x0006
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO7                 0x0007
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO8                 0x0008
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO9                 0x0009
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO10                0x000A
#define KSZ8463_TRIG12_CFG_1_GPIO_SEL_GPIO11                0x000B

//PTP Clock Control register
#define KSZ8463_PTP_CLK_CTL_PTP_STEP_ADJ_CLK                0x0040
#define KSZ8463_PTP_CLK_CTL_PTP_STEP_DIR                    0x0020
#define KSZ8463_PTP_CLK_CTL_PTP_READ_CLK                    0x0010
#define KSZ8463_PTP_CLK_CTL_PTP_LOAD_CLK                    0x0008
#define KSZ8463_PTP_CLK_CTL_PTP_CONTINU_ADJ_CLK             0x0004
#define KSZ8463_PTP_CLK_CTL_EN_PTP_CLK                      0x0002
#define KSZ8463_PTP_CLK_CTL_RESET_PTP_CLK                   0x0001

//PTP Sub-nanosecond Rate High-Word and Configuration register
#define KSZ8463_PTP_SNS_RATE_H_PTP_RATE_DIR                 0x8000
#define KSZ8463_PTP_SNS_RATE_H_PTP_TEMP_ADJ_CLK             0x4000
#define KSZ8463_PTP_SNS_RATE_H_PTP_SNS_RATE_H               0x3FFF

//PTP Message Configuration 1 register
#define KSZ8463_PTP_MSG_CFG_1_IEEE_802_1AS_MODE_EN          0x0080
#define KSZ8463_PTP_MSG_CFG_1_IEEE_1588_PTP_MODE_EN         0x0040
#define KSZ8463_PTP_MSG_CFG_1_ETH_PTP_DETECT                0x0020
#define KSZ8463_PTP_MSG_CFG_1_IPV4_UDP_PTP_DETECT           0x0010
#define KSZ8463_PTP_MSG_CFG_1_IPV6_UDP_PTP_DETECT           0x0008
#define KSZ8463_PTP_MSG_CFG_1_E2E_CLK_MODE                  0x0000
#define KSZ8463_PTP_MSG_CFG_1_P2P_CLK_MODE                  0x0004
#define KSZ8463_PTP_MSG_CFG_1_SLAVE_OC_CLK_MODE             0x0000
#define KSZ8463_PTP_MSG_CFG_1_MASTER_OC_CLK_MODE            0x0002
#define KSZ8463_PTP_MSG_CFG_1_TWO_STEP_CLK_MODE             0x0000
#define KSZ8463_PTP_MSG_CFG_1_ONE_STEP_CLK_MODE             0x0001

//PTP Message Configuration 2 register
#define KSZ8463_PTP_MSG_CFG_2_UNICAST_PTP_EN                0x1000
#define KSZ8463_PTP_MSG_CFG_2_ALT_MASTER_EN                 0x0800
#define KSZ8463_PTP_MSG_CFG_2_PTP_PRIO_TX_QUEUE             0x0400
#define KSZ8463_PTP_MSG_CFG_2_SYNC_FOLLOW_UP_EN             0x0200
#define KSZ8463_PTP_MSG_CFG_2_DELAY_REQ_RESP_EN             0x0100
#define KSZ8463_PTP_MSG_CFG_2_PDELAY_REQ_RESP_EN            0x0080
#define KSZ8463_PTP_MSG_CFG_2_DOMAIN_EN                     0x0010
#define KSZ8463_PTP_MSG_CFG_2_EG_CHECKSUM_EN                0x0004
#define KSZ8463_PTP_MSG_CFG_2_ANNOUNCE_PORT1                0x0002
#define KSZ8463_PTP_MSG_CFG_2_ANNOUNCE_PORT2                0x0001

//PTP Domain and Version register
#define KSZ8463_PTP_DOMAIN_VER_PTP_VERSION                  0x0F00
#define KSZ8463_PTP_DOMAIN_VER_PTP_DOMAIN                   0x00FF

//PTP GPIO Monitor register
#define KSZ8463_GPIO_MONITOR_INPUT11                        0x0800
#define KSZ8463_GPIO_MONITOR_INPUT10                        0x0400
#define KSZ8463_GPIO_MONITOR_INPUT9                         0x0200
#define KSZ8463_GPIO_MONITOR_INPUT8                         0x0100
#define KSZ8463_GPIO_MONITOR_INPUT7                         0x0080
#define KSZ8463_GPIO_MONITOR_INPUT6                         0x0040
#define KSZ8463_GPIO_MONITOR_INPUT5                         0x0020
#define KSZ8463_GPIO_MONITOR_INPUT4                         0x0010
#define KSZ8463_GPIO_MONITOR_INPUT3                         0x0008
#define KSZ8463_GPIO_MONITOR_INPUT2                         0x0004
#define KSZ8463_GPIO_MONITOR_INPUT1                         0x0002
#define KSZ8463_GPIO_MONITOR_INPUT0                         0x0001

//PTP GPIO Output Enable register
#define KSZ8463_GPIO_OEN_OUTPUT11                           0x0800
#define KSZ8463_GPIO_OEN_OUTPUT10                           0x0400
#define KSZ8463_GPIO_OEN_OUTPUT9                            0x0200
#define KSZ8463_GPIO_OEN_OUTPUT8                            0x0100
#define KSZ8463_GPIO_OEN_OUTPUT7                            0x0080
#define KSZ8463_GPIO_OEN_OUTPUT6                            0x0040
#define KSZ8463_GPIO_OEN_OUTPUT5                            0x0020
#define KSZ8463_GPIO_OEN_OUTPUT4                            0x0010
#define KSZ8463_GPIO_OEN_OUTPUT3                            0x0008
#define KSZ8463_GPIO_OEN_OUTPUT2                            0x0004
#define KSZ8463_GPIO_OEN_OUTPUT1                            0x0002
#define KSZ8463_GPIO_OEN_OUTPUT0                            0x0001

//PTP Trigger Unit Interrupt Status register
#define KSZ8463_PTP_TRIG_IS_UNIT12                          0x0800
#define KSZ8463_PTP_TRIG_IS_UNIT11                          0x0400
#define KSZ8463_PTP_TRIG_IS_UNIT10                          0x0200
#define KSZ8463_PTP_TRIG_IS_UNIT9                           0x0100
#define KSZ8463_PTP_TRIG_IS_UNIT8                           0x0080
#define KSZ8463_PTP_TRIG_IS_UNIT7                           0x0040
#define KSZ8463_PTP_TRIG_IS_UNIT6                           0x0020
#define KSZ8463_PTP_TRIG_IS_UNIT5                           0x0010
#define KSZ8463_PTP_TRIG_IS_UNIT4                           0x0008
#define KSZ8463_PTP_TRIG_IS_UNIT3                           0x0004
#define KSZ8463_PTP_TRIG_IS_UNIT2                           0x0002
#define KSZ8463_PTP_TRIG_IS_UNIT1                           0x0001

//PTP Trigger Unit Interrupt Enable register
#define KSZ8463_PTP_TRIG_IE_UNIT12                          0x0800
#define KSZ8463_PTP_TRIG_IE_UNIT11                          0x0400
#define KSZ8463_PTP_TRIG_IE_UNIT10                          0x0200
#define KSZ8463_PTP_TRIG_IE_UNIT9                           0x0100
#define KSZ8463_PTP_TRIG_IE_UNIT8                           0x0080
#define KSZ8463_PTP_TRIG_IE_UNIT7                           0x0040
#define KSZ8463_PTP_TRIG_IE_UNIT6                           0x0020
#define KSZ8463_PTP_TRIG_IE_UNIT5                           0x0010
#define KSZ8463_PTP_TRIG_IE_UNIT4                           0x0008
#define KSZ8463_PTP_TRIG_IE_UNIT3                           0x0004
#define KSZ8463_PTP_TRIG_IE_UNIT2                           0x0002
#define KSZ8463_PTP_TRIG_IE_UNIT1                           0x0001

//PTP Time stamp Unit Interrupt Status register
#define KSZ8463_PTP_TS_IS_P2_XDLY_TS_IS                     0x8000
#define KSZ8463_PTP_TS_IS_P2_SYNC_TS_IS                     0x4000
#define KSZ8463_PTP_TS_IS_P1_XDLY_TS_IS                     0x2000
#define KSZ8463_PTP_TS_IS_P1_SYNC_TS_IS                     0x1000
#define KSZ8463_PTP_TS_IS_TSU_IS                            0x0FFF

//PTP Time stamp Unit Interrupt Enable register
#define KSZ8463_PTP_TS_IE_P2_XDLY_TS_IE                     0x8000
#define KSZ8463_PTP_TS_IE_P2_SYNC_TS_IE                     0x4000
#define KSZ8463_PTP_TS_IE_P1_XDLY_TS_IE                     0x2000
#define KSZ8463_PTP_TS_IE_P1_SYNC_TS_IE                     0x1000
#define KSZ8463_PTP_TS_IE_TSU_IE                            0x0FFF

//DSP Control 1 register
#define KSZ8463_DSP_CNTRL_6_RECEIVER_ADJ                    0x2000

//Analog Control 1 register
#define KSZ8463_ANA_CNTRL_1_LDO_OFF                         0x0080

//Analog Control 3 register
#define KSZ8463_ANA_CNTRL_3_HIPLS3_MASK                     0x8000
#define KSZ8463_ANA_CNTRL_3_BTRX_REDUCE                     0x0008

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
} Ksz8463StaticMacEntry;


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
} Ksz8463DynamicMacEntry;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif

//KSZ8463 Ethernet switch driver
extern const SwitchDriver ksz8463SwitchDriver;

//KSZ8463 related functions
error_t ksz8463Init(NetInterface *interface);

void ksz8463Tick(NetInterface *interface);

void ksz8463EnableIrq(NetInterface *interface);
void ksz8463DisableIrq(NetInterface *interface);

void ksz8463EventHandler(NetInterface *interface);

error_t ksz8463TagFrame(NetInterface *interface, NetBuffer *buffer,
   size_t *offset, NetTxAncillary *ancillary);

error_t ksz8463UntagFrame(NetInterface *interface, uint8_t **frame,
   size_t *length, NetRxAncillary *ancillary);

bool_t ksz8463GetLinkState(NetInterface *interface, uint8_t port);
uint32_t ksz8463GetLinkSpeed(NetInterface *interface, uint8_t port);
NicDuplexMode ksz8463GetDuplexMode(NetInterface *interface, uint8_t port);

void ksz8463SetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state);

SwitchPortState ksz8463GetPortState(NetInterface *interface, uint8_t port);

void ksz8463SetAgingTime(NetInterface *interface, uint32_t agingTime);

void ksz8463EnableIgmpSnooping(NetInterface *interface, bool_t enable);
void ksz8463EnableMldSnooping(NetInterface *interface, bool_t enable);
void ksz8463EnableRsvdMcastTable(NetInterface *interface, bool_t enable);

error_t ksz8463AddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t ksz8463DeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t ksz8463GetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void ksz8463FlushStaticFdbTable(NetInterface *interface);

error_t ksz8463GetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void ksz8463FlushDynamicFdbTable(NetInterface *interface, uint8_t port);

void ksz8463SetUnknownMcastFwdPorts(NetInterface *interface,
   bool_t enable, uint32_t forwardPorts);

void ksz8463WritePhyReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data);

uint16_t ksz8463ReadPhyReg(NetInterface *interface, uint8_t port,
   uint8_t address);

void ksz8463DumpPhyReg(NetInterface *interface, uint8_t port);

void ksz8463WriteSwitchReg(NetInterface *interface, uint16_t address,
   uint16_t data);

uint16_t ksz8463ReadSwitchReg(NetInterface *interface, uint16_t address);

void ksz8463DumpSwitchReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
