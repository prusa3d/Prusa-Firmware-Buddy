/**
 * @file lan9303_driver.h
 * @brief LAN9303 3-port Ethernet switch driver
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

#ifndef _LAN9303_DRIVER_H
#define _LAN9303_DRIVER_H

//Dependencies
#include "core/nic.h"

//Port identifiers
#define LAN9303_PORT0 3
#define LAN9303_PORT1 1
#define LAN9303_PORT2 2

//Port masks
#define LAN9303_PORT_MASK      0x07
#define LAN9303_PORT0_MASK     0x04
#define LAN9303_PORT1_MASK     0x01
#define LAN9303_PORT2_MASK     0x02
#define LAN9303_PORT0_1_MASK   0x05
#define LAN9303_PORT0_2_MASK   0x06
#define LAN9303_PORT1_2_MASK   0x03
#define LAN9303_PORT0_1_2_MASK 0x07

//Size of of the MAC address lookup table
#define LAN9303_ALR_TABLE_SIZE 512

//Special VLAN tag (host to LAN9303)
#define LAN9303_VID_VLAN_RULES    0x0040
#define LAN9303_VID_CALC_PRIORITY 0x0020
#define LAN9303_VID_STP_OVERRIDE  0x0010
#define LAN9303_VID_ALR_LOOKUP    0x0008
#define LAN9303_VID_BROADCAST     0x0003
#define LAN9303_VID_DEST_PORT2    0x0002
#define LAN9303_VID_DEST_PORT1    0x0001
#define LAN9303_VID_DEST_PORT0    0x0000

//Special VLAN tag (LAN9303 to host)
#define LAN9303_VID_PRIORITY      0x0380
#define LAN9303_VID_PRIORITY_EN   0x0040
#define LAN9303_VID_STATIC        0x0020
#define LAN9303_VID_STP_OVERRIDE  0x0010
#define LAN9303_VID_IGMP_PACKET   0x0008
#define LAN9303_VID_SRC_PORT      0x0003

//LAN9303 PHY registers
#define LAN9303_BMCR                                       0x00
#define LAN9303_BMSR                                       0x01
#define LAN9303_PHYID1                                     0x02
#define LAN9303_PHYID2                                     0x03
#define LAN9303_ANAR                                       0x04
#define LAN9303_ANLPAR                                     0x05
#define LAN9303_ANER                                       0x06
#define LAN9303_PMCSR                                      0x11
#define LAN9303_PSMR                                       0x12
#define LAN9303_PSCSIR                                     0x1B
#define LAN9303_PISR                                       0x1D
#define LAN9303_PIMR                                       0x1E
#define LAN9303_PSCSR                                      0x1F

//LAN9303 System registers
#define LAN9303_ID_REV                                     0x0050
#define LAN9303_IRQ_CFG                                    0x0054
#define LAN9303_INT_STS                                    0x0058
#define LAN9303_INT_EN                                     0x005C
#define LAN9303_BYTE_TEST                                  0x0064
#define LAN9303_HW_CFG                                     0x0074
#define LAN9303_GPT_CFG                                    0x008C
#define LAN9303_GPT_CNT                                    0x0090
#define LAN9303_FREE_RUN                                   0x009C
#define LAN9303_PMI_DATA                                   0x00A4
#define LAN9303_PMI_ACCESS                                 0x00A8
#define LAN9303_MANUAL_FC_1                                0x01A0
#define LAN9303_MANUAL_FC_2                                0x01A4
#define LAN9303_MANUAL_FC_0                                0x01A8
#define LAN9303_SWITCH_CSR_DATA                            0x01AC
#define LAN9303_SWITCH_CSR_CMD                             0x01B0
#define LAN9303_E2P_CMD                                    0x01B4
#define LAN9303_E2P_DATA                                   0x01B8
#define LAN9303_LED_CFG                                    0x01BC
#define LAN9303_VPHY_BASIC_CTRL                            0x01C0
#define LAN9303_VPHY_BASIC_STATUS                          0x01C4
#define LAN9303_VPHY_ID_MSB                                0x01C8
#define LAN9303_VPHY_ID_LSB                                0x01CC
#define LAN9303_VPHY_AN_ADV                                0x01D0
#define LAN9303_VPHY_AN_LP_BASE_ABILITY                    0x01D4
#define LAN9303_VPHY_AN_EXP                                0x01D8
#define LAN9303_VPHY_SPECIAL_CONTROL_STATUS                0x01DC
#define LAN9303_GPIO_CFG                                   0x01E0
#define LAN9303_GPIO_DATA_DIR                              0x01E4
#define LAN9303_GPIO_INT_STS_EN                            0x01E8
#define LAN9303_SWITCH_MAC_ADDRH                           0x01F0
#define LAN9303_SWITCH_MAC_ADDRL                           0x01F4
#define LAN9303_RESET_CTL                                  0x01F8
#define LAN9303_SWITCH_CSR_DIRECT_DATA                     0x0200

//LAN9303 Switch Fabric registers
#define LAN9303_SW_DEV_ID                                  0x0000
#define LAN9303_SW_RESET                                   0x0001
#define LAN9303_SW_IMR                                     0x0004
#define LAN9303_SW_IPR                                     0x0005
#define LAN9303_MAC_VER_ID_0                               0x0400
#define LAN9303_MAC_RX_CFG_0                               0x0401
#define LAN9303_MAC_RX_UNDSZE_CNT_0                        0x0410
#define LAN9303_MAC_RX_64_CNT_0                            0x0411
#define LAN9303_MAC_RX_65_TO_127_CNT_0                     0x0412
#define LAN9303_MAC_RX_128_TO_255_CNT_0                    0x0413
#define LAN9303_MAC_RX_256_TO_511_CNT_0                    0x0414
#define LAN9303_MAC_RX_512_TO_1023_CNT_0                   0x0415
#define LAN9303_MAC_RX_1024_TO_MAX_CNT_0                   0x0416
#define LAN9303_MAC_RX_OVRSZE_CNT_0                        0x0417
#define LAN9303_MAC_RX_PKTOK_CNT_0                         0x0418
#define LAN9303_MAC_RX_CRCERR_CNT_0                        0x0419
#define LAN9303_MAC_RX_MULCST_CNT_0                        0x041A
#define LAN9303_MAC_RX_BRDCST_CNT_0                        0x041B
#define LAN9303_MAC_RX_PAUSE_CNT_0                         0x041C
#define LAN9303_MAC_RX_FRAG_CNT_0                          0x041D
#define LAN9303_MAC_RX_JABB_CNT_0                          0x041E
#define LAN9303_MAC_RX_ALIGN_CNT_0                         0x041F
#define LAN9303_MAC_RX_PKTLEN_CNT_0                        0x0420
#define LAN9303_MAC_RX_GOODPKTLEN_CNT_0                    0x0421
#define LAN9303_MAC_RX_SYMBL_CNT_0                         0x0422
#define LAN9303_MAC_RX_CTLFRM_CNT_0                        0x0423
#define LAN9303_MAC_TX_CFG_0                               0x0440
#define LAN9303_MAC_TX_FC_SETTINGS_0                       0x0441
#define LAN9303_MAC_TX_DEFER_CNT_0                         0x0451
#define LAN9303_MAC_TX_PAUSE_CNT_0                         0x0452
#define LAN9303_MAC_TX_PKTOK_CNT_0                         0x0453
#define LAN9303_MAC_TX_64_CNT_0                            0x0454
#define LAN9303_MAC_TX_65_TO_127_CNT_0                     0x0455
#define LAN9303_MAC_TX_128_TO_255_CNT_0                    0x0456
#define LAN9303_MAC_TX_256_TO_511_CNT_0                    0x0457
#define LAN9303_MAC_TX_512_TO_1023_CNT_0                   0x0458
#define LAN9303_MAC_TX_1024_TO_MAX_CNT_0                   0x0459
#define LAN9303_MAC_TX_UNDSZE_CNT_0                        0x045A
#define LAN9303_MAC_TX_PKTLEN_CNT_0                        0x045C
#define LAN9303_MAC_TX_BRDCST_CNT_0                        0x045D
#define LAN9303_MAC_TX_MULCST_CNT_0                        0x045E
#define LAN9303_MAC_TX_LATECOL_0                           0x045F
#define LAN9303_MAC_TX_EXCOL_CNT_0                         0x0460
#define LAN9303_MAC_TX_SNGLECOL_CNT_0                      0x0461
#define LAN9303_MAC_TX_MULTICOL_CNT_0                      0x0462
#define LAN9303_MAC_TX_TOTALCOL_CNT_0                      0x0463
#define LAN9303_MAC_IMR_0                                  0x0480
#define LAN9303_MAC_IPR_0                                  0x0481
#define LAN9303_MAC_VER_ID_1                               0x0800
#define LAN9303_MAC_RX_CFG_1                               0x0801
#define LAN9303_MAC_RX_UNDSZE_CNT_1                        0x0810
#define LAN9303_MAC_RX_64_CNT_1                            0x0811
#define LAN9303_MAC_RX_65_TO_127_CNT_1                     0x0812
#define LAN9303_MAC_RX_128_TO_255_CNT_1                    0x0813
#define LAN9303_MAC_RX_256_TO_511_CNT_1                    0x0814
#define LAN9303_MAC_RX_512_TO_1023_CNT_1                   0x0815
#define LAN9303_MAC_RX_1024_TO_MAX_CNT_1                   0x0816
#define LAN9303_MAC_RX_OVRSZE_CNT_1                        0x0817
#define LAN9303_MAC_RX_PKTOK_CNT_1                         0x0818
#define LAN9303_MAC_RX_CRCERR_CNT_1                        0x0819
#define LAN9303_MAC_RX_MULCST_CNT_1                        0x081A
#define LAN9303_MAC_RX_BRDCST_CNT_1                        0x081B
#define LAN9303_MAC_RX_PAUSE_CNT_1                         0x081C
#define LAN9303_MAC_RX_FRAG_CNT_1                          0x081D
#define LAN9303_MAC_RX_JABB_CNT_1                          0x081E
#define LAN9303_MAC_RX_ALIGN_CNT_1                         0x081F
#define LAN9303_MAC_RX_PKTLEN_CNT_1                        0x0820
#define LAN9303_MAC_RX_GOODPKTLEN_CNT_1                    0x0821
#define LAN9303_MAC_RX_SYMBL_CNT_1                         0x0822
#define LAN9303_MAC_RX_CTLFRM_CNT_1                        0x0823
#define LAN9303_MAC_TX_CFG_1                               0x0840
#define LAN9303_MAC_TX_FC_SETTINGS_1                       0x0841
#define LAN9303_MAC_TX_DEFER_CNT_1                         0x0851
#define LAN9303_MAC_TX_PAUSE_CNT_1                         0x0852
#define LAN9303_MAC_TX_PKTOK_CNT_1                         0x0853
#define LAN9303_MAC_TX_64_CNT_1                            0x0854
#define LAN9303_MAC_TX_65_TO_127_CNT_1                     0x0855
#define LAN9303_MAC_TX_128_TO_255_CNT_1                    0x0856
#define LAN9303_MAC_TX_256_TO_511_CNT_1                    0x0857
#define LAN9303_MAC_TX_512_TO_1023_CNT_1                   0x0858
#define LAN9303_MAC_TX_1024_TO_MAX_CNT_1                   0x0859
#define LAN9303_MAC_TX_UNDSZE_CNT_1                        0x085A
#define LAN9303_MAC_TX_PKTLEN_CNT_1                        0x085C
#define LAN9303_MAC_TX_BRDCST_CNT_1                        0x085D
#define LAN9303_MAC_TX_MULCST_CNT_1                        0x085E
#define LAN9303_MAC_TX_LATECOL_1                           0x085F
#define LAN9303_MAC_TX_EXCOL_CNT_1                         0x0860
#define LAN9303_MAC_TX_SNGLECOL_CNT_1                      0x0861
#define LAN9303_MAC_TX_MULTICOL_CNT_1                      0x0862
#define LAN9303_MAC_TX_TOTALCOL_CNT_1                      0x0863
#define LAN9303_MAC_IMR_1                                  0x0880
#define LAN9303_MAC_IPR_1                                  0x0881
#define LAN9303_MAC_VER_ID_2                               0x0C00
#define LAN9303_MAC_RX_CFG_2                               0x0C01
#define LAN9303_MAC_RX_UNDSZE_CNT_2                        0x0C10
#define LAN9303_MAC_RX_64_CNT_2                            0x0C11
#define LAN9303_MAC_RX_65_TO_127_CNT_2                     0x0C12
#define LAN9303_MAC_RX_128_TO_255_CNT_2                    0x0C13
#define LAN9303_MAC_RX_256_TO_511_CNT_2                    0x0C14
#define LAN9303_MAC_RX_512_TO_1023_CNT_2                   0x0C15
#define LAN9303_MAC_RX_1024_TO_MAX_CNT_2                   0x0C16
#define LAN9303_MAC_RX_OVRSZE_CNT_2                        0x0C17
#define LAN9303_MAC_RX_PKTOK_CNT_2                         0x0C18
#define LAN9303_MAC_RX_CRCERR_CNT_2                        0x0C19
#define LAN9303_MAC_RX_MULCST_CNT_2                        0x0C1A
#define LAN9303_MAC_RX_BRDCST_CNT_2                        0x0C1B
#define LAN9303_MAC_RX_PAUSE_CNT_2                         0x0C1C
#define LAN9303_MAC_RX_FRAG_CNT_2                          0x0C1D
#define LAN9303_MAC_RX_JABB_CNT_2                          0x0C1E
#define LAN9303_MAC_RX_ALIGN_CNT_2                         0x0C1F
#define LAN9303_MAC_RX_PKTLEN_CNT_2                        0x0C20
#define LAN9303_MAC_RX_GOODPKTLEN_CNT_2                    0x0C21
#define LAN9303_MAC_RX_SYMBL_CNT_2                         0x0C22
#define LAN9303_MAC_RX_CTLFRM_CNT_2                        0x0C23
#define LAN9303_MAC_TX_CFG_2                               0x0C40
#define LAN9303_MAC_TX_FC_SETTINGS_2                       0x0C41
#define LAN9303_MAC_TX_DEFER_CNT_2                         0x0C51
#define LAN9303_MAC_TX_PAUSE_CNT_2                         0x0C52
#define LAN9303_MAC_TX_PKTOK_CNT_2                         0x0C53
#define LAN9303_MAC_TX_64_CNT_2                            0x0C54
#define LAN9303_MAC_TX_65_TO_127_CNT_2                     0x0C55
#define LAN9303_MAC_TX_128_TO_255_CNT_2                    0x0C56
#define LAN9303_MAC_TX_256_TO_511_CNT_2                    0x0C57
#define LAN9303_MAC_TX_512_TO_1023_CNT_2                   0x0C58
#define LAN9303_MAC_TX_1024_TO_MAX_CNT_2                   0x0C59
#define LAN9303_MAC_TX_UNDSZE_CNT_2                        0x0C5A
#define LAN9303_MAC_TX_PKTLEN_CNT_2                        0x0C5C
#define LAN9303_MAC_TX_BRDCST_CNT_2                        0x0C5D
#define LAN9303_MAC_TX_MULCST_CNT_2                        0x0C5E
#define LAN9303_MAC_TX_LATECOL_2                           0x0C5F
#define LAN9303_MAC_TX_EXCOL_CNT_2                         0x0C60
#define LAN9303_MAC_TX_SNGLECOL_CNT_2                      0x0C61
#define LAN9303_MAC_TX_MULTICOL_CNT_2                      0x0C62
#define LAN9303_MAC_TX_TOTALCOL_CNT_2                      0x0C63
#define LAN9303_MAC_IMR_2                                  0x0C80
#define LAN9303_MAC_IPR_2                                  0x0C81
#define LAN9303_SWE_ALR_CMD                                0x1800
#define LAN9303_SWE_ALR_WR_DAT_0                           0x1801
#define LAN9303_SWE_ALR_WR_DAT_1                           0x1802
#define LAN9303_SWE_ALR_RD_DAT_0                           0x1805
#define LAN9303_SWE_ALR_RD_DAT_1                           0x1806
#define LAN9303_SWE_ALR_CMD_STS                            0x1808
#define LAN9303_SWE_ALR_CFG                                0x1809
#define LAN9303_SWE_VLAN_CMD                               0x180B
#define LAN9303_SWE_VLAN_WR_DATA                           0x180C
#define LAN9303_SWE_VLAN_RD_DATA                           0x180E
#define LAN9303_SWE_VLAN_CMD_STS                           0x1810
#define LAN9303_SWE_DIFFSERV_TBL_CMD                       0x1811
#define LAN9303_SWE_DIFFSERV_TBL_WR_DATA                   0x1812
#define LAN9303_SWE_DIFFSERV_TBL_RD_DATA                   0x1813
#define LAN9303_SWE_DIFFSERV_TBL_CMD_STS                   0x1814
#define LAN9303_SWE_GLB_INGRESS_CFG                        0x1840
#define LAN9303_SWE_PORT_INGRESS_CFG                       0x1841
#define LAN9303_SWE_ADMT_ONLY_VLAN                         0x1842
#define LAN9303_SWE_PORT_STATE                             0x1843
#define LAN9303_SWE_PRI_TO_QUE                             0x1845
#define LAN9303_SWE_PORT_MIRROR                            0x1846
#define LAN9303_SWE_INGRSS_PORT_TYP                        0x1847
#define LAN9303_SWE_BCST_THROT                             0x1848
#define LAN9303_SWE_ADMT_N_MEMBER                          0x1849
#define LAN9303_SWE_INGRESS_RATE_CFG                       0x184A
#define LAN9303_SWE_INGRESS_RATE_CMD                       0x184B
#define LAN9303_SWE_INGRESS_RATE_CMD_STS                   0x184C
#define LAN9303_SWE_INGRESS_RATE_WR_DATA                   0x184D
#define LAN9303_SWE_INGRESS_RATE_RD_DATA                   0x184E
#define LAN9303_SWE_FILTERED_CNT_0                         0x1850
#define LAN9303_SWE_FILTERED_CNT_1                         0x1851
#define LAN9303_SWE_FILTERED_CNT_2                         0x1852
#define LAN9303_SWE_INGRESS_REGEN_TBL_0                    0x1855
#define LAN9303_SWE_INGRESS_REGEN_TBL_1                    0x1856
#define LAN9303_SWE_INGRESS_REGEN_TBL_2                    0x1857
#define LAN9303_SWE_LRN_DISCRD_CNT_0                       0x1858
#define LAN9303_SWE_LRN_DISCRD_CNT_1                       0x1859
#define LAN9303_SWE_LRN_DISCRD_CNT_2                       0x185A
#define LAN9303_SWE_IMR                                    0x1880
#define LAN9303_SWE_IPR                                    0x1881
#define LAN9303_BM_CFG                                     0x1C00
#define LAN9303_BM_DROP_LVL                                0x1C01
#define LAN9303_BM_FC_PAUSE_LVL                            0x1C02
#define LAN9303_BM_FC_RESUME_LVL                           0x1C03
#define LAN9303_BM_BCST_LVL                                0x1C04
#define LAN9303_BM_DRP_CNT_SRC_0                           0x1C05
#define LAN9303_BM_DRP_CNT_SRC_1                           0x1C06
#define LAN9303_BM_DRP_CNT_SRC_2                           0x1C07
#define LAN9303_BM_RST_STS                                 0x1C08
#define LAN9303_BM_RNDM_DSCRD_TBL_CMD                      0x1C09
#define LAN9303_BM_RNDM_DSCRD_TBL_WDATA                    0x1C0A
#define LAN9303_BM_RNDM_DSCRD_TBL_RDATA                    0x1C0B
#define LAN9303_BM_EGRSS_PORT_TYPE                         0x1C0C
#define LAN9303_BM_EGRSS_RATE_00_01                        0x1C0D
#define LAN9303_BM_EGRSS_RATE_02_03                        0x1C0E
#define LAN9303_BM_EGRSS_RATE_10_11                        0x1C0F
#define LAN9303_BM_EGRSS_RATE_12_13                        0x1C10
#define LAN9303_BM_EGRSS_RATE_20_21                        0x1C11
#define LAN9303_BM_EGRSS_RATE_22_23                        0x1C12
#define LAN9303_BM_VLAN_0                                  0x1C13
#define LAN9303_BM_VLAN_1                                  0x1C14
#define LAN9303_BM_VLAN_2                                  0x1C15
#define LAN9303_BM_RATE_DRP_CNT_SRC_0                      0x1C16
#define LAN9303_BM_RATE_DRP_CNT_SRC_1                      0x1C17
#define LAN9303_BM_RATE_DRP_CNT_SRC_2                      0x1C18
#define LAN9303_BM_IMR                                     0x1C20
#define LAN9303_BM_IPR                                     0x1C21

//LAN9303 Switch Fabric register access macros
#define LAN9303_MAC_VER_ID(port)                           (0x0400 + ((port) * 0x0400))
#define LAN9303_MAC_RX_CFG(port)                           (0x0401 + ((port) * 0x0400))
#define LAN9303_MAC_RX_UNDSZE_CNT(port)                    (0x0410 + ((port) * 0x0400))
#define LAN9303_MAC_RX_64_CNT(port)                        (0x0411 + ((port) * 0x0400))
#define LAN9303_MAC_RX_65_TO_127_CNT(port)                 (0x0412 + ((port) * 0x0400))
#define LAN9303_MAC_RX_128_TO_255_CNT(port)                (0x0413 + ((port) * 0x0400))
#define LAN9303_MAC_RX_256_TO_511_CNT(port)                (0x0414 + ((port) * 0x0400))
#define LAN9303_MAC_RX_512_TO_1023_CNT(port)               (0x0415 + ((port) * 0x0400))
#define LAN9303_MAC_RX_1024_TO_MAX_CNT(port)               (0x0416 + ((port) * 0x0400))
#define LAN9303_MAC_RX_OVRSZE_CNT(port)                    (0x0417 + ((port) * 0x0400))
#define LAN9303_MAC_RX_PKTOK_CNT(port)                     (0x0418 + ((port) * 0x0400))
#define LAN9303_MAC_RX_CRCERR_CNT(port)                    (0x0419 + ((port) * 0x0400))
#define LAN9303_MAC_RX_MULCST_CNT(port)                    (0x041A + ((port) * 0x0400))
#define LAN9303_MAC_RX_BRDCST_CNT(port)                    (0x041B + ((port) * 0x0400))
#define LAN9303_MAC_RX_PAUSE_CNT(port)                     (0x041C + ((port) * 0x0400))
#define LAN9303_MAC_RX_FRAG_CNT(port)                      (0x041D + ((port) * 0x0400))
#define LAN9303_MAC_RX_JABB_CNT(port)                      (0x041E + ((port) * 0x0400))
#define LAN9303_MAC_RX_ALIGN_CNT(port)                     (0x041F + ((port) * 0x0400))
#define LAN9303_MAC_RX_PKTLEN_CNT(port)                    (0x0420 + ((port) * 0x0400))
#define LAN9303_MAC_RX_GOODPKTLEN_CNT(port)                (0x0421 + ((port) * 0x0400))
#define LAN9303_MAC_RX_SYMBL_CNT(port)                     (0x0422 + ((port) * 0x0400))
#define LAN9303_MAC_RX_CTLFRM_CNT(port)                    (0x0423 + ((port) * 0x0400))
#define LAN9303_MAC_TX_CFG(port)                           (0x0440 + ((port) * 0x0400))
#define LAN9303_MAC_TX_FC_SETTINGS(port)                   (0x0441 + ((port) * 0x0400))
#define LAN9303_MAC_TX_DEFER_CNT(port)                     (0x0451 + ((port) * 0x0400))
#define LAN9303_MAC_TX_PAUSE_CNT(port)                     (0x0452 + ((port) * 0x0400))
#define LAN9303_MAC_TX_PKTOK_CNT(port)                     (0x0453 + ((port) * 0x0400))
#define LAN9303_MAC_TX_64_CNT(port)                        (0x0454 + ((port) * 0x0400))
#define LAN9303_MAC_TX_65_TO_127_CNT(port)                 (0x0455 + ((port) * 0x0400))
#define LAN9303_MAC_TX_128_TO_255_CNT(port)                (0x0456 + ((port) * 0x0400))
#define LAN9303_MAC_TX_256_TO_511_CNT(port)                (0x0457 + ((port) * 0x0400))
#define LAN9303_MAC_TX_512_TO_1023_CNT(port)               (0x0458 + ((port) * 0x0400))
#define LAN9303_MAC_TX_1024_TO_MAX_CNT(port)               (0x0459 + ((port) * 0x0400))
#define LAN9303_MAC_TX_UNDSZE_CNT(port)                    (0x045A + ((port) * 0x0400))
#define LAN9303_MAC_TX_PKTLEN_CNT(port)                    (0x045C + ((port) * 0x0400))
#define LAN9303_MAC_TX_BRDCST_CNT(port)                    (0x045D + ((port) * 0x0400))
#define LAN9303_MAC_TX_MULCST_CNT(port)                    (0x045E + ((port) * 0x0400))
#define LAN9303_MAC_TX_LATECOL(port)                       (0x045F + ((port) * 0x0400))
#define LAN9303_MAC_TX_EXCOL_CNT(port)                     (0x0460 + ((port) * 0x0400))
#define LAN9303_MAC_TX_SNGLECOL_CNT(port)                  (0x0461 + ((port) * 0x0400))
#define LAN9303_MAC_TX_MULTICOL_CNT(port)                  (0x0462 + ((port) * 0x0400))
#define LAN9303_MAC_TX_TOTALCOL_CNT(port)                  (0x0463 + ((port) * 0x0400))
#define LAN9303_MAC_IMR(port)                              (0x0480 + ((port) * 0x0400))
#define LAN9303_MAC_IPR(port)                              (0x0481 + ((port) * 0x0400))

//PHY Basic Control register
#define LAN9303_BMCR_RESET                                 0x8000
#define LAN9303_BMCR_LOOPBACK                              0x4000
#define LAN9303_BMCR_SPEED_SEL                             0x2000
#define LAN9303_BMCR_AN_EN                                 0x1000
#define LAN9303_BMCR_POWER_DOWN                            0x0800
#define LAN9303_BMCR_RESTART_AN                            0x0200
#define LAN9303_BMCR_DUPLEX_MODE                           0x0100
#define LAN9303_BMCR_COL_TEST                              0x0080

//PHY Basic Status register
#define LAN9303_BMSR_100BT4                                0x8000
#define LAN9303_BMSR_100BTX_FD                             0x4000
#define LAN9303_BMSR_100BTX_HD                             0x2000
#define LAN9303_BMSR_10BT_FD                               0x1000
#define LAN9303_BMSR_10BT_HD                               0x0800
#define LAN9303_BMSR_100BT2_FD                             0x0400
#define LAN9303_BMSR_100BT2_HD                             0x0200
#define LAN9303_BMSR_AN_COMPLETE                           0x0020
#define LAN9303_BMSR_REMOTE_FAULT                          0x0010
#define LAN9303_BMSR_AN_CAPABLE                            0x0008
#define LAN9303_BMSR_LINK_STATUS                           0x0004
#define LAN9303_BMSR_JABBER_DETECT                         0x0002
#define LAN9303_BMSR_EXTENDED_CAPABLE                      0x0001

//PHY Identification MSB register
#define LAN9303_PHYID1_PHY_ID_MSB                          0xFFFF
#define LAN9303_PHYID1_PHY_ID_MSB_DEFAULT                  0x0007

//PHY Identification LSB register
#define LAN9303_PHYID2_PHY_ID_LSB                          0xFFFF
#define LAN9303_PHYID2_PHY_ID_LSB_DEFAULT                  0x0030
#define LAN9303_PHYID2_MODEL_NUM                           0x03F0
#define LAN9303_PHYID2_MODEL_NUM_DEFAULT                   0x00D0
#define LAN9303_PHYID2_REVISION_NUM                        0x000F

//PHY Auto-Negotiation Advertisement register
#define LAN9303_ANAR_REMOTE_FAULT                          0x2000
#define LAN9303_ANAR_ASYM_PAUSE                            0x0800
#define LAN9303_ANAR_SYM_PAUSE                             0x0400
#define LAN9303_ANAR_100BTX_FD                             0x0100
#define LAN9303_ANAR_100BTX_HD                             0x0080
#define LAN9303_ANAR_10BT_FD                               0x0040
#define LAN9303_ANAR_10BT_HD                               0x0020
#define LAN9303_ANAR_SELECTOR                              0x001F
#define LAN9303_ANAR_SELECTOR_DEFAULT                      0x0001

//PHY Auto-Negotiation Link Partner Base Page Ability register
#define LAN9303_ANLPAR_NEXT_PAGE                           0x8000
#define LAN9303_ANLPAR_ACK                                 0x4000
#define LAN9303_ANLPAR_REMOTE_FAULT                        0x2000
#define LAN9303_ANLPAR_ASYM_PAUSE                          0x0800
#define LAN9303_ANLPAR_SYM_PAUSE                           0x0400
#define LAN9303_ANLPAR_100BT4                              0x0200
#define LAN9303_ANLPAR_100BTX_FD                           0x0100
#define LAN9303_ANLPAR_100BTX_HD                           0x0080
#define LAN9303_ANLPAR_10BT_FD                             0x0040
#define LAN9303_ANLPAR_10BT_HD                             0x0020
#define LAN9303_ANLPAR_SELECTOR                            0x001F
#define LAN9303_ANLPAR_SELECTOR_DEFAULT                    0x0001

//PHY Auto-Negotiation Expansion register
#define LAN9303_ANER_PAR_DETECT_FAULT                      0x0010
#define LAN9303_ANER_LP_NEXT_PAGE_ABLE                     0x0008
#define LAN9303_ANER_NEXT_PAGE_ABLE                        0x0004
#define LAN9303_ANER_PAGE_RECEIVED                         0x0002
#define LAN9303_ANER_LP_AN_ABLE                            0x0001

//PHY Mode Control/Status register
#define LAN9303_PMCSR_EDPWRDOWN                            0x2000
#define LAN9303_PMCSR_ENERGYON                             0x0002

//PHY Special Modes register
#define LAN9303_PSMR_MODE                                  0x00E0
#define LAN9303_PSMR_MODE_10BT_HD                          0x0000
#define LAN9303_PSMR_MODE_10BT_FD                          0x0020
#define LAN9303_PSMR_MODE_100BTX_HD                        0x0040
#define LAN9303_PSMR_MODE_100BTX_FD                        0x0060
#define LAN9303_PSMR_MODE_POWER_DOWN                       0x00C0
#define LAN9303_PSMR_MODE_AN                               0x00E0
#define LAN9303_PSMR_PHYAD                                 0x001F

//PHY Special Control/Status Indication register
#define LAN9303_PSCSIR_AMDIXCTRL                           0x8000
#define LAN9303_PSCSIR_AMDIXEN                             0x4000
#define LAN9303_PSCSIR_AMDIXSTATE                          0x2000
#define LAN9303_PSCSIR_SQEOFF                              0x0800
#define LAN9303_PSCSIR_VCOOFF_LP                           0x0400
#define LAN9303_PSCSIR_XPOL                                0x0010

//PHY Interrupt Source Flags register
#define LAN9303_PISR_ENERGYON                              0x0080
#define LAN9303_PISR_AN_COMPLETE                           0x0040
#define LAN9303_PISR_REMOTE_FAULT                          0x0020
#define LAN9303_PISR_LINK_DOWN                             0x0010
#define LAN9303_PISR_AN_LP_ACK                             0x0008
#define LAN9303_PISR_PAR_DETECT_FAULT                      0x0004
#define LAN9303_PISR_AN_PAGE_RECEIVED                      0x0002

//PHY Interrupt Mask register
#define LAN9303_PIMR_ENERGYON                              0x0080
#define LAN9303_PIMR_AN_COMPLETE                           0x0040
#define LAN9303_PIMR_REMOTE_FAULT                          0x0020
#define LAN9303_PIMR_LINK_DOWN                             0x0010
#define LAN9303_PIMR_AN_LP_ACK                             0x0008
#define LAN9303_PIMR_PAR_DETECT_FAULT                      0x0004
#define LAN9303_PIMR_AN_PAGE_RECEIVED                      0x0002

//PHY Special Control/Status register
#define LAN9303_PSCSR_AUTODONE                             0x1000
#define LAN9303_PSCSR_SPEED                                0x001C
#define LAN9303_PSCSR_SPEED_10BT_HD                        0x0004
#define LAN9303_PSCSR_SPEED_100BTX_HD                      0x0008
#define LAN9303_PSCSR_SPEED_10BT_FD                        0x0014
#define LAN9303_PSCSR_SPEED_100BTX_FD                      0x0018

//Byte Order Test register
#define LAN9303_BYTE_TEST_DEFAULT                          0x87654321

//Hardware Configuration register
#define LAN9303_HW_CFG_DEVICE_READY                        0x08000000
#define LAN9303_HW_CFG_AMDIX_EN_STRAP_STATE_PORT2          0x04000000
#define LAN9303_HW_CFG_AMDIX_EN_STRAP_STATE_PORT1          0x02000000

//Switch Fabric CSR Interface Command register
#define LAN9303_SWITCH_CSR_CMD_BUSY                        0x80000000
#define LAN9303_SWITCH_CSR_CMD_READ                        0x40000000
#define LAN9303_SWITCH_CSR_CMD_AUTO_INC                    0x20000000
#define LAN9303_SWITCH_CSR_CMD_AUTO_DEC                    0x10000000
#define LAN9303_SWITCH_CSR_CMD_BE                          0x000F0000
#define LAN9303_SWITCH_CSR_CMD_BE_0                        0x00010000
#define LAN9303_SWITCH_CSR_CMD_BE_1                        0x00020000
#define LAN9303_SWITCH_CSR_CMD_BE_2                        0x00040000
#define LAN9303_SWITCH_CSR_CMD_BE_3                        0x00080000
#define LAN9303_SWITCH_CSR_CMD_ADDR                        0x0000FFFF

//Switch Device ID register
#define LAN9303_SW_DEV_ID_DEVICE_TYPE                      0x00FF0000
#define LAN9303_SW_DEV_ID_DEVICE_TYPE_DEFAULT              0x00030000
#define LAN9303_SW_DEV_ID_CHIP_VERSION                     0x0000FF00
#define LAN9303_SW_DEV_ID_CHIP_VERSION_DEFAULT             0x00000400
#define LAN9303_SW_DEV_ID_REVISION                         0x000000FF
#define LAN9303_SW_DEV_ID_REVISION_DEFAULT                 0x00000007

//Switch Reset register
#define LAN9303_SW_RESET_SW_RESET                          0x00000001

//Switch Global Interrupt Mask register
#define LAN9303_SW_IMR_BM                                  0x00000040
#define LAN9303_SW_IMR_SWE                                 0x00000020
#define LAN9303_SW_IMR_MAC2                                0x00000004
#define LAN9303_SW_IMR_MAC1                                0x00000002
#define LAN9303_SW_IMR_MAC0                                0x00000001

//Switch Global Interrupt Pending register
#define LAN9303_SW_IPR_BM                                  0x00000040
#define LAN9303_SW_IPR_SWE                                 0x00000020
#define LAN9303_SW_IPR_MAC2                                0x00000004
#define LAN9303_SW_IPR_MAC1                                0x00000002
#define LAN9303_SW_IPR_MAC0                                0x00000001

//Port x MAC Version ID register
#define LAN9303_MAC_VER_ID_DEVICE_TYPE                     0x00000F00
#define LAN9303_MAC_VER_ID_DEVICE_TYPE_DEFAULT             0x00000500
#define LAN9303_MAC_VER_ID_CHIP_VERSION                    0x000000F0
#define LAN9303_MAC_VER_ID_CHIP_VERSION_DEFAULT            0x00000080
#define LAN9303_MAC_VER_ID_REVISION                        0x0000000F
#define LAN9303_MAC_VER_ID_REVISION_DEFAULT                0x00000003

//Port x MAC Receive Configuration register
#define LAN9303_MAC_RX_CFG_RECEIVE_OWN_TRANSMIT_EN         0x00000020
#define LAN9303_MAC_RX_CFG_JUMBO_2K                        0x00000008
#define LAN9303_MAC_RX_CFG_REJECT_MAC_TYPES                0x00000002
#define LAN9303_MAC_RX_CFG_RX_EN                           0x00000001

//Port x MAC Transmit Configuration register
#define LAN9303_MAC_TX_CFG_MAC_COUNTER_TEST                0x00000080
#define LAN9303_MAC_TX_CFG_IFG_CONFIG                      0x0000007C
#define LAN9303_MAC_TX_CFG_IFG_CONFIG_DEFAULT              0x00000054
#define LAN9303_MAC_TX_CFG_TX_PAD_EN                       0x00000002
#define LAN9303_MAC_TX_CFG_TX_EN                           0x00000001

//Switch Engine ALR Command register
#define LAN9303_SWE_ALR_CMD_MAKE_ENTRY                     0x00000004
#define LAN9303_SWE_ALR_CMD_GET_FIRST_ENTRY                0x00000002
#define LAN9303_SWE_ALR_CMD_GET_NEXT_ENTRY                 0x00000001

//Switch Engine ALR Write Data 0 register
#define LAN9303_SWE_ALR_WR_DAT_0_MAC_ADDR                  0xFFFFFFFF

//Switch Engine ALR Write Data 1 register
#define LAN9303_SWE_ALR_WR_DAT_1_VALID                     0x04000000
#define LAN9303_SWE_ALR_WR_DAT_1_AGE_OVERRIDE              0x02000000
#define LAN9303_SWE_ALR_WR_DAT_1_STATIC                    0x01000000
#define LAN9303_SWE_ALR_WR_DAT_1_FILTER                    0x00800000
#define LAN9303_SWE_ALR_WR_DAT_1_PRIORITY_EN               0x00400000
#define LAN9303_SWE_ALR_WR_DAT_1_PRIORITY                  0x00380000
#define LAN9303_SWE_ALR_WR_DAT_1_PORT                      0x00070000
#define LAN9303_SWE_ALR_WR_DAT_1_PORT_0                    0x00000000
#define LAN9303_SWE_ALR_WR_DAT_1_PORT_1                    0x00010000
#define LAN9303_SWE_ALR_WR_DAT_1_PORT_2                    0x00020000
#define LAN9303_SWE_ALR_WR_DAT_1_PORT_RESERVED             0x00030000
#define LAN9303_SWE_ALR_WR_DAT_1_PORT_0_1                  0x00040000
#define LAN9303_SWE_ALR_WR_DAT_1_PORT_0_2                  0x00050000
#define LAN9303_SWE_ALR_WR_DAT_1_PORT_1_2                  0x00060000
#define LAN9303_SWE_ALR_WR_DAT_1_PORT_0_1_2                0x00070000
#define LAN9303_SWE_ALR_WR_DAT_1_MAC_ADDR                  0x0000FFFF

//Switch Engine ALR Read Data 0 register
#define LAN9303_SWE_ALR_RD_DAT_0_MAC_ADDR                  0xFFFFFFFF

//Switch Engine ALR Read Data 1 register
#define LAN9303_SWE_ALR_RD_DAT_1_VALID                     0x04000000
#define LAN9303_SWE_ALR_RD_DAT_1_END_OF_TABLE              0x02000000
#define LAN9303_SWE_ALR_RD_DAT_1_STATIC                    0x01000000
#define LAN9303_SWE_ALR_RD_DAT_1_FILTER                    0x00800000
#define LAN9303_SWE_ALR_RD_DAT_1_PRIORITY_EN               0x00400000
#define LAN9303_SWE_ALR_RD_DAT_1_PRIORITY                  0x00380000
#define LAN9303_SWE_ALR_RD_DAT_1_PORT                      0x00070000
#define LAN9303_SWE_ALR_RD_DAT_1_PORT_0                    0x00000000
#define LAN9303_SWE_ALR_RD_DAT_1_PORT_1                    0x00010000
#define LAN9303_SWE_ALR_RD_DAT_1_PORT_2                    0x00020000
#define LAN9303_SWE_ALR_RD_DAT_1_PORT_RESERVED             0x00030000
#define LAN9303_SWE_ALR_RD_DAT_1_PORT_0_1                  0x00040000
#define LAN9303_SWE_ALR_RD_DAT_1_PORT_0_2                  0x00050000
#define LAN9303_SWE_ALR_RD_DAT_1_PORT_1_2                  0x00060000
#define LAN9303_SWE_ALR_RD_DAT_1_PORT_0_1_2                0x00070000
#define LAN9303_SWE_ALR_RD_DAT_1_MAC_ADDR                  0x0000FFFF

//Switch Engine ALR Command Status register
#define LAN9303_SWE_ALR_CMD_STS_ALR_INIT_DONE              0x00000002
#define LAN9303_SWE_ALR_CMD_STS_MAKE_PENDING               0x00000001

//Switch Engine ALR Configuration register
#define LAN9303_SWE_ALR_CFG_ALR_AGE_TEST                   0x00000001

//Switch Engine VLAN Command register
#define LAN9303_SWE_VLAN_CMD_WRITE                         0x00000000
#define LAN9303_SWE_VLAN_CMD_READ                          0x00000020
#define LAN9303_SWE_VLAN_CMD_VLAN                          0x00000000
#define LAN9303_SWE_VLAN_CMD_PVID                          0x00000010
#define LAN9303_SWE_VLAN_CMD_VLAN_PORT                     0x0000000F

//Switch Engine Global Ingress Configuration register
#define LAN9303_SWE_GLB_INGRESS_CFG_802_1Q_VLAN_DIS        0x00008000
#define LAN9303_SWE_GLB_INGRESS_CFG_USE_TAG                0x00004000
#define LAN9303_SWE_GLB_INGRESS_CFG_ALLOW_MONITOR_ECHO     0x00002000
#define LAN9303_SWE_GLB_INGRESS_CFG_IGMP_MONITOR_PORT      0x00001C00
#define LAN9303_SWE_GLB_INGRESS_CFG_IGMP_MONITOR_PORT_0    0x00000000
#define LAN9303_SWE_GLB_INGRESS_CFG_IGMP_MONITOR_PORT_1    0x00000400
#define LAN9303_SWE_GLB_INGRESS_CFG_IGMP_MONITOR_PORT_2    0x00000800
#define LAN9303_SWE_GLB_INGRESS_CFG_USE_IP                 0x00000200
#define LAN9303_SWE_GLB_INGRESS_CFG_IGMP_MONITORING_EN     0x00000080
#define LAN9303_SWE_GLB_INGRESS_CFG_SWE_COUNTER_TEST       0x00000040
#define LAN9303_SWE_GLB_INGRESS_CFG_DA_HIGHEST_PRIORITY    0x00000020
#define LAN9303_SWE_GLB_INGRESS_CFG_FILTER_MULTICAST       0x00000010
#define LAN9303_SWE_GLB_INGRESS_CFG_DROP_UNKNOWN           0x00000008
#define LAN9303_SWE_GLB_INGRESS_CFG_USE_PRECEDENCE         0x00000004
#define LAN9303_SWE_GLB_INGRESS_CFG_VL_HIGHER_PRIORITY     0x00000002
#define LAN9303_SWE_GLB_INGRESS_CFG_VLAN_EN                0x00000001

//Switch Engine Port Ingress Configuration register
#define LAN9303_SWE_PORT_INGRESS_CFG_LEARNING_ON_INGRESS   0x00000038
#define LAN9303_SWE_PORT_INGRESS_CFG_LEARNING_ON_INGRESS_0 0x00000000
#define LAN9303_SWE_PORT_INGRESS_CFG_LEARNING_ON_INGRESS_1 0x00000008
#define LAN9303_SWE_PORT_INGRESS_CFG_LEARNING_ON_INGRESS_2 0x00000010
#define LAN9303_SWE_PORT_INGRESS_CFG_MEMBERSHIP_CHECKING   0x00000007
#define LAN9303_SWE_PORT_INGRESS_CFG_MEMBERSHIP_CHECKING_0 0x00000000
#define LAN9303_SWE_PORT_INGRESS_CFG_MEMBERSHIP_CHECKING_1 0x00000001
#define LAN9303_SWE_PORT_INGRESS_CFG_MEMBERSHIP_CHECKING_2 0x00000002

//Switch Engine Admit Only VLAN register
#define LAN9303_SWE_ADMT_ONLY_VLAN_ADMIT_ONLY_VLAN         0x00000007
#define LAN9303_SWE_ADMT_ONLY_VLAN_ADMIT_ONLY_VLAN_0       0x00000000
#define LAN9303_SWE_ADMT_ONLY_VLAN_ADMIT_ONLY_VLAN_1       0x00000001
#define LAN9303_SWE_ADMT_ONLY_VLAN_ADMIT_ONLY_VLAN_2       0x00000002

//Switch Engine Port State register
#define LAN9303_SWE_PORT_STATE_PORT2                       0x00000030
#define LAN9303_SWE_PORT_STATE_PORT2_FORWARDING            0x00000000
#define LAN9303_SWE_PORT_STATE_PORT2_LISTENING             0x00000010
#define LAN9303_SWE_PORT_STATE_PORT2_LEARNING              0x00000020
#define LAN9303_SWE_PORT_STATE_PORT2_DISABLED              0x00000030
#define LAN9303_SWE_PORT_STATE_PORT1                       0x0000000C
#define LAN9303_SWE_PORT_STATE_PORT1_FORWARDING            0x00000000
#define LAN9303_SWE_PORT_STATE_PORT1_LISTENING             0x00000004
#define LAN9303_SWE_PORT_STATE_PORT1_LEARNING              0x00000008
#define LAN9303_SWE_PORT_STATE_PORT1_DISABLED              0x0000000C
#define LAN9303_SWE_PORT_STATE_PORT0                       0x00000003
#define LAN9303_SWE_PORT_STATE_PORT0_FORWARDING            0x00000000
#define LAN9303_SWE_PORT_STATE_PORT0_LISTENING             0x00000001
#define LAN9303_SWE_PORT_STATE_PORT0_LEARNING              0x00000002
#define LAN9303_SWE_PORT_STATE_PORT0_DISABLED              0x00000003

//Switch Engine Priority to Queue register
#define LAN9303_SWE_PRI_TO_QUE_PRIO_7_TRAFFIC_CLASS        0x0000C000
#define LAN9303_SWE_PRI_TO_QUE_PRIO_6_TRAFFIC_CLASS        0x00003000
#define LAN9303_SWE_PRI_TO_QUE_PRIO_5_TRAFFIC_CLASS        0x00000C00
#define LAN9303_SWE_PRI_TO_QUE_PRIO_4_TRAFFIC_CLASS        0x00000300
#define LAN9303_SWE_PRI_TO_QUE_PRIO_3_TRAFFIC_CLASS        0x000000C0
#define LAN9303_SWE_PRI_TO_QUE_PRIO_2_TRAFFIC_CLASS        0x00000030
#define LAN9303_SWE_PRI_TO_QUE_PRIO_1_TRAFFIC_CLASS        0x0000000C
#define LAN9303_SWE_PRI_TO_QUE_PRIO_0_TRAFFIC_CLASS        0x00000003

//Switch Engine Port Mirroring register
#define LAN9303_SWE_PORT_MIRROR_RX_MIRRORING_FILT_EN       0x00000100
#define LAN9303_SWE_PORT_MIRROR_SNIFFER                    0x000000E0
#define LAN9303_SWE_PORT_MIRROR_SNIFFER_PORT0              0x00000020
#define LAN9303_SWE_PORT_MIRROR_SNIFFER_PORT1              0x00000040
#define LAN9303_SWE_PORT_MIRROR_SNIFFER_PORT2              0x00000080
#define LAN9303_SWE_PORT_MIRROR_MIRRORED                   0x0000001C
#define LAN9303_SWE_PORT_MIRROR_MIRRORED_PORT0             0x00000004
#define LAN9303_SWE_PORT_MIRROR_MIRRORED_PORT1             0x00000008
#define LAN9303_SWE_PORT_MIRROR_MIRRORED_PORT2             0x00000010
#define LAN9303_SWE_PORT_MIRROR_RX_MIRRORING_EN            0x00000002
#define LAN9303_SWE_PORT_MIRROR_TX_MIRRORING_EN            0x00000001

//Switch Engine Ingress Port Type register
#define LAN9303_SWE_INGRSS_PORT_TYP_PORT2                  0x00000030
#define LAN9303_SWE_INGRSS_PORT_TYP_PORT2_DIS              0x00000000
#define LAN9303_SWE_INGRSS_PORT_TYP_PORT2_EN               0x00000030
#define LAN9303_SWE_INGRSS_PORT_TYP_PORT1                  0x0000000C
#define LAN9303_SWE_INGRSS_PORT_TYP_PORT1_DIS              0x00000000
#define LAN9303_SWE_INGRSS_PORT_TYP_PORT1_EN               0x0000000C
#define LAN9303_SWE_INGRSS_PORT_TYP_PORT0                  0x00000003
#define LAN9303_SWE_INGRSS_PORT_TYP_PORT0_DIS              0x00000000
#define LAN9303_SWE_INGRSS_PORT_TYP_PORT0_EN               0x00000003

//Buffer Manager Egress Port Type register
#define LAN9303_BM_EGRSS_PORT_TYPE_VID_SEL_PORT2           0x00400000
#define LAN9303_BM_EGRSS_PORT_TYPE_INSERT_TAG_PORT2        0x00200000
#define LAN9303_BM_EGRSS_PORT_TYPE_CHANGE_VID_PORT2        0x00100000
#define LAN9303_BM_EGRSS_PORT_TYPE_CHANGE_PRIO_PORT2       0x00080000
#define LAN9303_BM_EGRSS_PORT_TYPE_CHANGE_TAG_PORT2        0x00040000
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT2                   0x00030000
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT2_DUMB              0x00000000
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT2_ACCESS            0x00010000
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT2_HYBRID            0x00020000
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT2_CPU               0x00030000
#define LAN9303_BM_EGRSS_PORT_TYPE_VID_SEL_PORT1           0x00004000
#define LAN9303_BM_EGRSS_PORT_TYPE_INSERT_TAG_PORT1        0x00002000
#define LAN9303_BM_EGRSS_PORT_TYPE_CHANGE_VID_PORT1        0x00001000
#define LAN9303_BM_EGRSS_PORT_TYPE_CHANGE_PRIO_PORT1       0x00000800
#define LAN9303_BM_EGRSS_PORT_TYPE_CHANGE_TAG_PORT1        0x00000400
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT1                   0x00000300
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT1_DUMB              0x00000000
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT1_ACCESS            0x00000100
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT1_HYBRID            0x00000200
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT1_CPU               0x00000300
#define LAN9303_BM_EGRSS_PORT_TYPE_VID_SEL_PORT0           0x00000040
#define LAN9303_BM_EGRSS_PORT_TYPE_INSERT_TAG_PORT0        0x00000020
#define LAN9303_BM_EGRSS_PORT_TYPE_CHANGE_VID_PORT0        0x00000010
#define LAN9303_BM_EGRSS_PORT_TYPE_CHANGE_PRIO_PORT0       0x00000008
#define LAN9303_BM_EGRSS_PORT_TYPE_CHANGE_TAG_PORT0        0x00000004
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT0                   0x00000003
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT0_DUMB              0x00000000
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT0_ACCESS            0x00000001
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT0_HYBRID            0x00000002
#define LAN9303_BM_EGRSS_PORT_TYPE_PORT0_CPU               0x00000003

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//LAN9303 Ethernet switch driver
extern const SwitchDriver lan9303SwitchDriver;

//LAN9303 related functions
error_t lan9303Init(NetInterface *interface);

void lan9303Tick(NetInterface *interface);

void lan9303EnableIrq(NetInterface *interface);
void lan9303DisableIrq(NetInterface *interface);

void lan9303EventHandler(NetInterface *interface);

error_t lan9303TagFrame(NetInterface *interface, NetBuffer *buffer,
   size_t *offset, NetTxAncillary *ancillary);

error_t lan9303UntagFrame(NetInterface *interface, uint8_t **frame,
   size_t *length, NetRxAncillary *ancillary);

bool_t lan9303GetLinkState(NetInterface *interface, uint8_t port);
uint32_t lan9303GetLinkSpeed(NetInterface *interface, uint8_t port);
NicDuplexMode lan9303GetDuplexMode(NetInterface *interface, uint8_t port);

void lan9303SetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state);

SwitchPortState lan9303GetPortState(NetInterface *interface, uint8_t port);

void lan9303SetAgingTime(NetInterface *interface, uint32_t agingTime);

void lan9303EnableIgmpSnooping(NetInterface *interface, bool_t enable);
void lan9303EnableMldSnooping(NetInterface *interface, bool_t enable);
void lan9303EnableRsvdMcastTable(NetInterface *interface, bool_t enable);

error_t lan9303AddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t lan9303DeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t lan9303GetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void lan9303FlushStaticFdbTable(NetInterface *interface);

error_t lan9303GetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void lan9303FlushDynamicFdbTable(NetInterface *interface, uint8_t port);

void lan9303SetUnknownMcastFwdPorts(NetInterface *interface,
   bool_t enable, uint32_t forwardPorts);

void lan9303WritePhyReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data);

uint16_t lan9303ReadPhyReg(NetInterface *interface, uint8_t port,
   uint8_t address);

void lan9303DumpPhyReg(NetInterface *interface, uint8_t port);

void lan9303WriteSysReg(NetInterface *interface, uint16_t address,
   uint32_t data);

uint32_t lan9303ReadSysReg(NetInterface *interface, uint16_t address);

void lan9303DumpSysReg(NetInterface *interface);

void lan9303WriteSwitchReg(NetInterface *interface, uint16_t address,
   uint32_t data);

uint32_t lan9303ReadSwitchReg(NetInterface *interface, uint16_t address);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
