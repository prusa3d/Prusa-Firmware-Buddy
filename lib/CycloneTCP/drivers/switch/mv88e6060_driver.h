/**
 * @file mv88e6060_driver.h
 * @brief 88E6060 6-port Ethernet switch driver
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

#ifndef _MV88E6060_DRIVER_H
#define _MV88E6060_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef MV88E6060_PHY_ADDR
   #define MV88E6060_PHY_ADDR 16
#elif (MV88E6060_PHY_ADDR < 0 || MV88E6060_PHY_ADDR > 31)
   #error MV88E6060_PHY_ADDR parameter is not valid
#endif

//Port identifiers
#define MV88E6060_PORT0 1
#define MV88E6060_PORT1 2
#define MV88E6060_PORT2 3
#define MV88E6060_PORT3 4
#define MV88E6060_PORT4 5
#define MV88E6060_PORT5 6

//Port masks
#define MV88E6060_PORT_MASK  0x3F
#define MV88E6060_PORT0_MASK 0x01
#define MV88E6060_PORT1_MASK 0x02
#define MV88E6060_PORT2_MASK 0x04
#define MV88E6060_PORT3_MASK 0x08
#define MV88E6060_PORT4_MASK 0x10
#define MV88E6060_PORT5_MASK 0x20

//Size of of the MAC address lookup table
#define MV88E6060_ATU_TABLE_SIZE 1024

//Ingress trailer (CPU to 88E6060)
#define MV88E6060_IG_OVERRIDE      0x80000000
#define MV88E6060_IG_LEARN_DISABLE 0x20000000
#define MV88E6060_IG_IGNORE_FCS    0x10000000
#define MV88E6060_IG_DPV           0x003F0000
#define MV88E6060_IG_DPV_PORT5     0x00200000
#define MV88E6060_IG_DPV_PORT4     0x00100000
#define MV88E6060_IG_DPV_PORT3     0x00080000
#define MV88E6060_IG_DPV_PORT2     0x00040000
#define MV88E6060_IG_DPV_PORT1     0x00020000
#define MV88E6060_IG_DPV_PORT0     0x00010000
#define MV88E6060_IG_MGMT          0x00001000

//Egress trailer (88E6060 to CPU)
#define MV88E6060_EG_VALID         0x80000000
#define MV88E6060_EG_SPID          0x00070000
#define MV88E6060_EG_MGMT          0x00001000

//88E6060 PHY registers
#define MV88E6060_BMCR                                      0x00
#define MV88E6060_BMSR                                      0x01
#define MV88E6060_PHYID1                                    0x02
#define MV88E6060_PHYID2                                    0x03
#define MV88E6060_ANAR                                      0x04
#define MV88E6060_ANLPAR                                    0x05
#define MV88E6060_ANER                                      0x06
#define MV88E6060_ANNPR                                     0x07
#define MV88E6060_ANLPNPR                                   0x08
#define MV88E6060_PHY_SPEC_CTRL                             0x10
#define MV88E6060_PHY_SPEC_STAT                             0x11
#define MV88E6060_PHY_INT_EN                                0x12
#define MV88E6060_PHY_INT_STAT                              0x13
#define MV88E6060_PHY_INT_PORT_SUMMARY                      0x14
#define MV88E6060_RX_ERR_COUNTER                            0x15
#define MV88E6060_LED_PAR_SEL                               0x16
#define MV88E6060_LED_STREAM_SEL                            0x17
#define MV88E6060_PHY_LED_CTRL                              0x18
#define MV88E6060_PHY_MAN_LED_OVERRIDE                      0x19
#define MV88E6060_VCT_TXPN                                  0x1A
#define MV88E6060_VCT_RXPN                                  0x1B
#define MV88E6060_PHY_SPEC_CTRL2                            0x1C

//88E6060 Switch Port registers
#define MV88E6060_PORT_STAT                                 0x00
#define MV88E6060_SWITCH_ID                                 0x03
#define MV88E6060_PORT_CTRL                                 0x04
#define MV88E6060_PORT_VLAN_MAP                             0x06
#define MV88E6060_PORT_ASSOC_VECTOR                         0x0B
#define MV88E6060_RX_COUNTER                                0x10
#define MV88E6060_TX_COUNTER                                0x11

//88E6060 Switch Global registers
#define MV88E6060_SWITCH_GLOBAL_STAT                        0x00
#define MV88E6060_SWITCH_MAC_ADDR_0_1                       0x01
#define MV88E6060_SWITCH_MAC_ADDR_2_3                       0x02
#define MV88E6060_SWITCH_MAC_ADDR_4_5                       0x03
#define MV88E6060_SWITCH_GLOBAL_CTRL                        0x04
#define MV88E6060_ATU_CTRL                                  0x0A
#define MV88E6060_ATU_OPERATION                             0x0B
#define MV88E6060_ATU_DATA                                  0x0C
#define MV88E6060_ATU_MAC_ADDR_0_1                          0x0D
#define MV88E6060_ATU_MAC_ADDR_2_3                          0x0E
#define MV88E6060_ATU_MAC_ADDR_4_5                          0x0F

//PHY Control register
#define MV88E6060_BMCR_SW_RESET                             0x8000
#define MV88E6060_BMCR_LOOPBACK                             0x4000
#define MV88E6060_BMCR_SPEED_LSB                            0x2000
#define MV88E6060_BMCR_ANEG_EN                              0x1000
#define MV88E6060_BMCR_PWR_DWN                              0x0800
#define MV88E6060_BMCR_ISOLATE                              0x0400
#define MV88E6060_BMCR_RESTART_ANEG                         0x0200
#define MV88E6060_BMCR_DUPLEX                               0x0100
#define MV88E6060_BMCR_COL_TEST                             0x0080
#define MV88E6060_BMCR_SPEED_MSB                            0x0040

//PHY Status register
#define MV88E6060_BMSR_100T4                                0x8000
#define MV88E6060_BMSR_100FDX                               0x4000
#define MV88E6060_BMSR_100HDX                               0x2000
#define MV88E6060_BMSR_10FDX                                0x1000
#define MV88E6060_BMSR_10HPX                                0x0800
#define MV88E6060_BMSR_100T2FDX                             0x0400
#define MV88E6060_BMSR_100T2HDX                             0x0200
#define MV88E6060_BMSR_EXTD_STATUS                          0x0100
#define MV88E6060_BMSR_MF_PRE_SUP                           0x0040
#define MV88E6060_BMSR_ANEG_DONE                            0x0020
#define MV88E6060_BMSR_REMOTE_FAULT                         0x0010
#define MV88E6060_BMSR_ANEG_ABLE                            0x0008
#define MV88E6060_BMSR_LINK                                 0x0004
#define MV88E6060_BMSR_JABBER_DET                           0x0002
#define MV88E6060_BMSR_EXTD_REG                             0x0001

//PHY Identifier 1 register
#define MV88E6060_PHYID1_OUI_MSB                            0xFFFF
#define MV88E6060_PHYID1_OUI_MSB_DEFAULT                    0x0141

//PHY Identifier 2 register
#define MV88E6060_PHYID2_OUI_LSB                            0xFC00
#define MV88E6060_PHYID2_OUI_LSB_DEFAULT                    0x0C00
#define MV88E6060_PHYID2_MODEL_NUM                          0x03F0
#define MV88E6060_PHYID2_MODEL_NUM_DEFAULT                  0x0080
#define MV88E6060_PHYID2_REV_NUM                            0x000F

//Auto-Negotiation Advertisement register
#define MV88E6060_ANAR_ANEG_AD_NXT_PAGE                     0x8000
#define MV88E6060_ANAR_ACK                                  0x4000
#define MV88E6060_ANAR_ANEG_AD_RE_FAULT                     0x2000
#define MV88E6060_ANAR_ANEG_AD_PAUSE                        0x0400
#define MV88E6060_ANAR_ANEG_AD_100T4                        0x0200
#define MV88E6060_ANAR_ANEG_AD_100FDX                       0x0100
#define MV88E6060_ANAR_ANEG_AD_100HDX                       0x0080
#define MV88E6060_ANAR_ANEG_AD_10FDX                        0x0040
#define MV88E6060_ANAR_ANEG_AD_10HDX                        0x0020
#define MV88E6060_ANAR_ANEG_AD_SELECTOR                     0x001F
#define MV88E6060_ANAR_ANEG_AD_SELECTOR_DEFAULT             0x0001

//Link Partner Ability register
#define MV88E6060_ANLPAR_LP_NXT_PAGE                        0x8000
#define MV88E6060_ANLPAR_LP_ACK                             0x4000
#define MV88E6060_ANLPAR_LP_REMOTE_FAULT                    0x2000
#define MV88E6060_ANLPAR_LP_TECH_ABLE                       0x1FE0
#define MV88E6060_ANLPAR_LP_SELECTOR                        0x001F

//Auto-Negotiation Expansion register
#define MV88E6060_ANER_PAR_FAULT_DET                        0x0010
#define MV88E6060_ANER_LP_NXT_PG_ABLE                       0x0008
#define MV88E6060_ANER_LOCAL_NXT_PG_ABLE                    0x0004
#define MV88E6060_ANER_RX_NEW_PAGE                          0x0002
#define MV88E6060_ANER_LP_ANEG_ABLE                         0x0001

//Next Page Transmit register
#define MV88E6060_ANNPR_TX_NXT_PAGE                         0x8000
#define MV88E6060_ANNPR_TX_MESSAGE                          0x2000
#define MV88E6060_ANNPR_TX_ACK2                             0x1000
#define MV88E6060_ANNPR_TX_TOGGLE                           0x0800
#define MV88E6060_ANNPR_TX_DATA                             0x07FF

//Link Partner Next Page register
#define MV88E6060_ANLPNPR_RX_NXT_PAGE                       0x8000
#define MV88E6060_ANLPNPR_RX_ACK                            0x4000
#define MV88E6060_ANLPNPR_RX_MESSAGE                        0x2000
#define MV88E6060_ANLPNPR_RX_ACK2                           0x1000
#define MV88E6060_ANLPNPR_RX_TOGGLE                         0x0800
#define MV88E6060_ANLPNPR_RX_DATA                           0x07FF

//PHY Specific Control register
#define MV88E6060_PHY_SPEC_CTRL_E_DET                       0x4000
#define MV88E6060_PHY_SPEC_CTRL_DIS_NLP_CHECK               0x2000
#define MV88E6060_PHY_SPEC_CTRL_REG8_NXT_PG                 0x1000
#define MV88E6060_PHY_SPEC_CTRL_DIS_NLP_GEN                 0x0800
#define MV88E6060_PHY_SPEC_CTRL_FORCE_LINK                  0x0400
#define MV88E6060_PHY_SPEC_CTRL_DIS_SCRAMBLER               0x0200
#define MV88E6060_PHY_SPEC_CTRL_DIS_FEFI                    0x0100
#define MV88E6060_PHY_SPEC_CTRL_EXTD_DISTANCE               0x0080
#define MV88E6060_PHY_SPEC_CTRL_TP_SELECT                   0x0040
#define MV88E6060_PHY_SPEC_CTRL_AUTO_MDIX                   0x0030
#define MV88E6060_PHY_SPEC_CTRL_RX_FIFO_DEPTH               0x000C
#define MV88E6060_PHY_SPEC_CTRL_AUTO_POL                    0x0002
#define MV88E6060_PHY_SPEC_CTRL_DIS_JABBER                  0x0001

//PHY Specific Status register
#define MV88E6060_PHY_SPEC_STAT_RES_SPEED                   0x4000
#define MV88E6060_PHY_SPEC_STAT_RES_DUPLEX                  0x2000
#define MV88E6060_PHY_SPEC_STAT_RCV_PAGE                    0x1000
#define MV88E6060_PHY_SPEC_STAT_RESOLVED                    0x0800
#define MV88E6060_PHY_SPEC_STAT_RT_LINK                     0x0400
#define MV88E6060_PHY_SPEC_STAT_MDI_MDIX                    0x0040
#define MV88E6060_PHY_SPEC_STAT_SLEEP                       0x0010
#define MV88E6060_PHY_SPEC_STAT_RT_POLARITY                 0x0002
#define MV88E6060_PHY_SPEC_STAT_RT_JABBER                   0x0001

//PHY Interrupt Enable register
#define MV88E6060_PHY_INT_EN_SPEED_INT_EN                   0x4000
#define MV88E6060_PHY_INT_EN_DUPLEX_INT_EN                  0x2000
#define MV88E6060_PHY_INT_EN_RCV_PAGE_INT_EN                0x1000
#define MV88E6060_PHY_INT_EN_ANEG_DONE_INT_EN               0x0800
#define MV88E6060_PHY_INT_EN_LINK_INT_EN                    0x0400
#define MV88E6060_PHY_INT_EN_SYM_ERR_INT_EN                 0x0200
#define MV88E6060_PHY_INT_EN_FLS_CRS_INTEN                  0x0100
#define MV88E6060_PHY_INT_EN_FIFO_ERR_INT                   0x0080
#define MV88E6060_PHY_INT_EN_MDIX_INT_EN                    0x0040
#define MV88E6060_PHY_INT_EN_E_DET_INT_EN                   0x0010
#define MV88E6060_PHY_INT_EN_POLARITY_INT_EN                0x0002
#define MV88E6060_PHY_INT_EN_JABBER_INT_EN                  0x0001

//PHY Interrupt Status register
#define MV88E6060_PHY_INT_STAT_SPEED_INT                    0x4000
#define MV88E6060_PHY_INT_STAT_DUPLEX_INT                   0x2000
#define MV88E6060_PHY_INT_STAT_RCV_PAGE_INT                 0x1000
#define MV88E6060_PHY_INT_STAT_ANEG_DONE_INT                0x0800
#define MV88E6060_PHY_INT_STAT_LINK_INT                     0x0400
#define MV88E6060_PHY_INT_STAT_SYM_ERR_INT                  0x0200
#define MV88E6060_PHY_INT_STAT_FLS_CRS_INT                  0x0100
#define MV88E6060_PHY_INT_STAT_FIFO_ERR_INT                 0x0080
#define MV88E6060_PHY_INT_STAT_MDIX_INT                     0x0040
#define MV88E6060_PHY_INT_STAT_E_DET_INT                    0x0010
#define MV88E6060_PHY_INT_STAT_POLARITYINT                  0x0002
#define MV88E6060_PHY_INT_STAT_JABBERINT                    0x0001

//PHY Interrupt Port Summary register
#define MV88E6060_PHY_INT_PORT_SUMMARY_PORT4_INT_ACTIVE     0x0010
#define MV88E6060_PHY_INT_PORT_SUMMARY_PORT3_INT_ACTIVE     0x0008
#define MV88E6060_PHY_INT_PORT_SUMMARY_PORT2_INT_ACTIVE     0x0004
#define MV88E6060_PHY_INT_PORT_SUMMARY_PORT1_INT_ACTIVE     0x0002
#define MV88E6060_PHY_INT_PORT_SUMMARY_PORT0_INT_ACTIVE     0x0001

//LED Parallel Select register
#define MV88E6060_LED_PAR_SEL_LED2                          0x0F00
#define MV88E6060_LED_PAR_SEL_LED2_COLX                     0x0000
#define MV88E6060_LED_PAR_SEL_LED2_ERROR                    0x0100
#define MV88E6060_LED_PAR_SEL_LED2_DUPLEX                   0x0200
#define MV88E6060_LED_PAR_SEL_LED2_DUPLEX_COLX              0x0300
#define MV88E6060_LED_PAR_SEL_LED2_SPEED                    0x0400
#define MV88E6060_LED_PAR_SEL_LED2_LINK                     0x0500
#define MV88E6060_LED_PAR_SEL_LED2_TX                       0x0600
#define MV88E6060_LED_PAR_SEL_LED2_RX                       0x0700
#define MV88E6060_LED_PAR_SEL_LED2_ACT                      0x0800
#define MV88E6060_LED_PAR_SEL_LED2_LINK_RX                  0x0900
#define MV88E6060_LED_PAR_SEL_LED2_ACT_BLINK                0x0B00
#define MV88E6060_LED_PAR_SEL_LED2_FORCE_1                  0x0C00
#define MV88E6060_LED_PAR_SEL_LED1                          0x00F0
#define MV88E6060_LED_PAR_SEL_LED1_COLX                     0x0000
#define MV88E6060_LED_PAR_SEL_LED1_ERROR                    0x0010
#define MV88E6060_LED_PAR_SEL_LED1_DUPLEX                   0x0020
#define MV88E6060_LED_PAR_SEL_LED1_DUPLEX_COLX              0x0030
#define MV88E6060_LED_PAR_SEL_LED1_SPEED                    0x0040
#define MV88E6060_LED_PAR_SEL_LED1_LINK                     0x0050
#define MV88E6060_LED_PAR_SEL_LED1_TX                       0x0060
#define MV88E6060_LED_PAR_SEL_LED1_RX                       0x0070
#define MV88E6060_LED_PAR_SEL_LED1_ACT                      0x0080
#define MV88E6060_LED_PAR_SEL_LED1_LINK_RX                  0x0090
#define MV88E6060_LED_PAR_SEL_LED1_ACT_BLINK                0x00B0
#define MV88E6060_LED_PAR_SEL_LED1_FORCE_1                  0x00C0
#define MV88E6060_LED_PAR_SEL_LED0                          0x000F
#define MV88E6060_LED_PAR_SEL_LED0_COLX                     0x0000
#define MV88E6060_LED_PAR_SEL_LED0_ERROR                    0x0001
#define MV88E6060_LED_PAR_SEL_LED0_DUPLEX                   0x0002
#define MV88E6060_LED_PAR_SEL_LED0_DUPLEX_COLX              0x0003
#define MV88E6060_LED_PAR_SEL_LED0_SPEED                    0x0004
#define MV88E6060_LED_PAR_SEL_LED0_LINK                     0x0005
#define MV88E6060_LED_PAR_SEL_LED0_TX                       0x0006
#define MV88E6060_LED_PAR_SEL_LED0_RX                       0x0007
#define MV88E6060_LED_PAR_SEL_LED0_ACT                      0x0008
#define MV88E6060_LED_PAR_SEL_LED0_LINK_RX                  0x0009
#define MV88E6060_LED_PAR_SEL_LED0_ACT_BLINK                0x000B
#define MV88E6060_LED_PAR_SEL_LED0_FORCE_1                  0x000C

//LED Stream Select for Serial LEDs register
#define MV88E6060_LED_STREAM_SEL_LED_LNK_ACTY               0xC000
#define MV88E6060_LED_STREAM_SEL_LED_LNK_ACTY_OFF           0x0000
#define MV88E6060_LED_STREAM_SEL_LED_LNK_ACTY_DUAL          0x8000
#define MV88E6060_LED_STREAM_SEL_LED_LNK_ACTY_SINGLE        0xC000
#define MV88E6060_LED_STREAM_SEL_LED_RCV_LNK                0x3000
#define MV88E6060_LED_STREAM_SEL_LED_RCV_LNK_OFF            0x0000
#define MV88E6060_LED_STREAM_SEL_LED_RCV_LNK_DUAL           0x2000
#define MV88E6060_LED_STREAM_SEL_LED_RCV_LNK_SINGLE         0x3000
#define MV88E6060_LED_STREAM_SEL_LED_ACTY                   0x0C00
#define MV88E6060_LED_STREAM_SEL_LED_ACTY_OFF               0x0000
#define MV88E6060_LED_STREAM_SEL_LED_ACTY_DUAL              0x0800
#define MV88E6060_LED_STREAM_SEL_LED_ACTY_SINGLE            0x0C00
#define MV88E6060_LED_STREAM_SEL_LED_RCV                    0x0300
#define MV88E6060_LED_STREAM_SEL_LED_RCV_OFF                0x0000
#define MV88E6060_LED_STREAM_SEL_LED_RCV_DUAL               0x0200
#define MV88E6060_LED_STREAM_SEL_LED_RCV_SINGLE             0x0300
#define MV88E6060_LED_STREAM_SEL_LED_TX                     0x00C0
#define MV88E6060_LED_STREAM_SEL_LED_TX_OFF                 0x0000
#define MV88E6060_LED_STREAM_SEL_LED_TX_DUAL                0x0080
#define MV88E6060_LED_STREAM_SEL_LED_TX_SINGLE              0x00C0
#define MV88E6060_LED_STREAM_SEL_LED_LNK                    0x0030
#define MV88E6060_LED_STREAM_SEL_LED_LNK_OFF                0x0000
#define MV88E6060_LED_STREAM_SEL_LED_LNK_DUAL               0x0020
#define MV88E6060_LED_STREAM_SEL_LED_LNK_SINGLE             0x0030
#define MV88E6060_LED_STREAM_SEL_LED_SPD                    0x000C
#define MV88E6060_LED_STREAM_SEL_LED_SPD_OFF                0x0000
#define MV88E6060_LED_STREAM_SEL_LED_SPD_DUAL               0x0008
#define MV88E6060_LED_STREAM_SEL_LED_SPD_SINGLE             0x000C
#define MV88E6060_LED_STREAM_SEL_LED_DX_COLX                0x0003
#define MV88E6060_LED_STREAM_SEL_LED_DX_COLX_OFF            0x0000
#define MV88E6060_LED_STREAM_SEL_LED_DX_COLX_DUAL           0x0002
#define MV88E6060_LED_STREAM_SEL_LED_DX_COLX_SINGLE         0x0003

//PHY LED Control register
#define MV88E6060_PHY_LED_CTRL_PULSE_STRETCH                0x7000
#define MV88E6060_PHY_LED_CTRL_PULSE_STRETCH_NO             0x0000
#define MV88E6060_PHY_LED_CTRL_PULSE_STRETCH_21MS_TO_42MS   0x1000
#define MV88E6060_PHY_LED_CTRL_PULSE_STRETCH_42MS_TO_84MS   0x2000
#define MV88E6060_PHY_LED_CTRL_PULSE_STRETCH_84MS_TO_170MS  0x3000
#define MV88E6060_PHY_LED_CTRL_PULSE_STRETCH_170MS_TO_340MS 0x4000
#define MV88E6060_PHY_LED_CTRL_PULSE_STRETCH_340MS_TO_670MS 0x5000
#define MV88E6060_PHY_LED_CTRL_PULSE_STRETCH_670MS_TO_1_3S  0x6000
#define MV88E6060_PHY_LED_CTRL_PULSE_STRETCH_1_3S_TO_2_7S   0x7000
#define MV88E6060_PHY_LED_CTRL_BLINK_RATE                   0x0E00
#define MV88E6060_PHY_LED_CTRL_BLINK_RATE_42MS              0x0000
#define MV88E6060_PHY_LED_CTRL_BLINK_RATE_84MS              0x0200
#define MV88E6060_PHY_LED_CTRL_BLINK_RATE_170MS             0x0400
#define MV88E6060_PHY_LED_CTRL_BLINK_RATE_340MS             0x0600
#define MV88E6060_PHY_LED_CTRL_BLINK_RATE_670MS             0x0800
#define MV88E6060_PHY_LED_CTRL_SR_STR_UPDATE                0x01C0
#define MV88E6060_PHY_LED_CTRL_SR_STR_UPDATE_10MS           0x0000
#define MV88E6060_PHY_LED_CTRL_SR_STR_UPDATE_21MS           0x0040
#define MV88E6060_PHY_LED_CTRL_SR_STR_UPDATE_42MS           0x0080
#define MV88E6060_PHY_LED_CTRL_SR_STR_UPDATE_84MS           0x00C0
#define MV88E6060_PHY_LED_CTRL_SR_STR_UPDATE_170MS          0x0100
#define MV88E6060_PHY_LED_CTRL_SR_STR_UPDATE_340MS          0x0140
#define MV88E6060_PHY_LED_CTRL_DUPLEX                       0x0030
#define MV88E6060_PHY_LED_CTRL_DUPLEX_OFF                   0x0000
#define MV88E6060_PHY_LED_CTRL_DUPLEX_DUAL                  0x0020
#define MV88E6060_PHY_LED_CTRL_DUPLEX_SINGLE                0x0030
#define MV88E6060_PHY_LED_CTRL_ERROR                        0x000C
#define MV88E6060_PHY_LED_CTRL_ERROR_OFF                    0x0000
#define MV88E6060_PHY_LED_CTRL_ERROR_DUAL                   0x0008
#define MV88E6060_PHY_LED_CTRL_ERROR_SINGLE                 0x000C
#define MV88E6060_PHY_LED_CTRL_COLX                         0x0003
#define MV88E6060_PHY_LED_CTRL_COLX_OFF                     0x0000
#define MV88E6060_PHY_LED_CTRL_COLX_DUAL                    0x0002
#define MV88E6060_PHY_LED_CTRL_COLX_SINGLE                  0x0003

//PHY Manual LED Override register
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED2                 0x0030
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED2_NORMAL          0x0000
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED2_BLINK           0x0010
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED2_OFF             0x0020
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED2_ON              0x0030
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED1                 0x000C
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED1_NORMAL          0x0000
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED1_BLINK           0x0004
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED1_OFF             0x0008
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED1_ON              0x000C
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED0                 0x0003
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED0_NORMAL          0x0000
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED0_BLINK           0x0001
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED0_OFF             0x0002
#define MV88E6060_PHY_MAN_LED_OVERRIDE_LED0_ON              0x0003

//VCT for TXP/N Pins register
#define MV88E6060_VCT_TXPN_EN_VCT                           0x8000
#define MV88E6060_VCT_TXPN_VCT_TST                          0x6000
#define MV88E6060_VCT_TXPN_VCT_TST_VALID_NORMAL             0x0000
#define MV88E6060_VCT_TXPN_VCT_TST_VALID_SHORT              0x2000
#define MV88E6060_VCT_TXPN_VCT_TST_VALID_OPEN               0x4000
#define MV88E6060_VCT_TXPN_VCT_TST_FAIL                     0x6000
#define MV88E6060_VCT_TXPN_AMP_RFLN                         0x1F00
#define MV88E6060_VCT_TXPN_DIST_RFLN                        0x00FF

//VCT for RXP/N Pins register
#define MV88E6060_VCT_RXPN_VCT_TST                          0x6000
#define MV88E6060_VCT_RXPN_VCT_TST_VALID_NORMAL             0x0000
#define MV88E6060_VCT_RXPN_VCT_TST_VALID_SHORT              0x2000
#define MV88E6060_VCT_RXPN_VCT_TST_VALID_OPEN               0x4000
#define MV88E6060_VCT_RXPN_VCT_TST_FAIL                     0x6000
#define MV88E6060_VCT_RXPN_AMP_RFLN                         0x1F00
#define MV88E6060_VCT_RXPN_DIST_RFLN                        0x00FF

//PHY Specific Control 2 register
#define MV88E6060_PHY_SPEC_CTRL2_SEL_CLS_A                  0x0001

//Port Status register
#define MV88E6060_PORT_STAT_LINK_PAUSE                      0x8000
#define MV88E6060_PORT_STAT_MY_PAUSE                        0x4000
#define MV88E6060_PORT_STAT_RESOLVED                        0x2000
#define MV88E6060_PORT_STAT_LINK                            0x1000
#define MV88E6060_PORT_STAT_PORT_MODE                       0x0800
#define MV88E6060_PORT_STAT_PHY_MODE                        0x0400
#define MV88E6060_PORT_STAT_DUPLEX                          0x0200
#define MV88E6060_PORT_STAT_SPEED                           0x0100

//Switch Identifier register
#define MV88E6060_SWITCH_ID_DEVICE_ID                       0xFFF0
#define MV88E6060_SWITCH_ID_DEVICE_ID_DEFAULT               0x0600
#define MV88E6060_SWITCH_ID_REV_ID                          0x000F

//Port Control register
#define MV88E6060_PORT_CTRL_FORCE_FLOW_CONTROL              0x8000
#define MV88E6060_PORT_CTRL_TRAILER                         0x4000
#define MV88E6060_PORT_CTRL_HEADER                          0x0800
#define MV88E6060_PORT_CTRL_RESERVED                        0x0600
#define MV88E6060_PORT_CTRL_INGRESS_MODE                    0x0100
#define MV88E6060_PORT_CTRL_VLAN_TUNNEL                     0x0080
#define MV88E6060_PORT_CTRL_PORT_STATE                      0x0003
#define MV88E6060_PORT_CTRL_PORT_STATE_DISABLED             0x0000
#define MV88E6060_PORT_CTRL_PORT_STATE_BLOCKING             0x0001
#define MV88E6060_PORT_CTRL_PORT_STATE_LEARNING             0x0002
#define MV88E6060_PORT_CTRL_PORT_STATE_FORWARDING           0x0003

//Port Based VLAN Map register
#define MV88E6060_PORT_VLAN_MAP_DB_NUM                      0xF000
#define MV88E6060_PORT_VLAN_MAP_VLAN_TABLE                  0x003F
#define MV88E6060_PORT_VLAN_MAP_VLAN_TABLE_PORT0            0x0001
#define MV88E6060_PORT_VLAN_MAP_VLAN_TABLE_PORT1            0x0002
#define MV88E6060_PORT_VLAN_MAP_VLAN_TABLE_PORT2            0x0004
#define MV88E6060_PORT_VLAN_MAP_VLAN_TABLE_PORT3            0x0008
#define MV88E6060_PORT_VLAN_MAP_VLAN_TABLE_PORT4            0x0010
#define MV88E6060_PORT_VLAN_MAP_VLAN_TABLE_PORT5            0x0020

//Port Association Vector register
#define MV88E6060_PORT_ASSOC_VECTOR_INGRESS_MONITOR         0x8000
#define MV88E6060_PORT_ASSOC_VECTOR_PAV                     0x003F
#define MV88E6060_PORT_ASSOC_VECTOR_PAV_PORT0               0x0001
#define MV88E6060_PORT_ASSOC_VECTOR_PAV_PORT1               0x0002
#define MV88E6060_PORT_ASSOC_VECTOR_PAV_PORT2               0x0004
#define MV88E6060_PORT_ASSOC_VECTOR_PAV_PORT3               0x0008
#define MV88E6060_PORT_ASSOC_VECTOR_PAV_PORT4               0x0010
#define MV88E6060_PORT_ASSOC_VECTOR_PAV_PORT5               0x0020

//Switch Global Status register
#define MV88E6060_SWITCH_GLOBAL_STAT_SW_MODE                0x3000
#define MV88E6060_SWITCH_GLOBAL_STAT_INIT_READY             0x0800
#define MV88E6060_SWITCH_GLOBAL_STAT_ATU_FULL               0x0008
#define MV88E6060_SWITCH_GLOBAL_STAT_ATU_DONE               0x0004
#define MV88E6060_SWITCH_GLOBAL_STAT_PHY_INT                0x0002
#define MV88E6060_SWITCH_GLOBAL_STAT_EE_INT                 0x0001

//Switch MAC Address Bytes 0 and 1 register
#define MV88E6060_SWITCH_MAC_ADDR_0_1_MAC_BYTE0             0xFE00
#define MV88E6060_SWITCH_MAC_ADDR_0_1_DIFF_ADDR             0x0100
#define MV88E6060_SWITCH_MAC_ADDR_0_1_MAC_BYTE1             0x00FF

//Switch MAC Address Bytes 2 and 3 register
#define MV88E6060_SWITCH_MAC_ADDR_2_3_MAC_BYTE2             0xFF00
#define MV88E6060_SWITCH_MAC_ADDR_2_3_MAC_BYTE3             0x00FF

//Switch MAC Address Bytes 4 and 5 register
#define MV88E6060_SWITCH_MAC_ADDR_4_5_MAC_BYTE4             0xFF00
#define MV88E6060_SWITCH_MAC_ADDR_4_5_MAC_BYTE5             0x00FF

//Switch Global Control register
#define MV88E6060_SWITCH_GLOBAL_CTRL_DISCARD_EXCESSIVE      0x2000
#define MV88E6060_SWITCH_GLOBAL_CTRL_MAX_FRAME_SIZE         0x0400
#define MV88E6060_SWITCH_GLOBAL_CTRL_RE_LOAD                0x0200
#define MV88E6060_SWITCH_GLOBAL_CTRL_CTR_MODE               0x0100
#define MV88E6060_SWITCH_GLOBAL_CTRL_ATU_FULL_INT_EN        0x0008
#define MV88E6060_SWITCH_GLOBAL_CTRL_ATU_DONE_INT_EN        0x0004
#define MV88E6060_SWITCH_GLOBAL_CTRL_PHY_INT_EN             0x0002
#define MV88E6060_SWITCH_GLOBAL_CTRL_EE_INT_EN              0x0001

//ATU Control register
#define MV88E6060_ATU_CTRL_SW_RESET                         0x8000
#define MV88E6060_ATU_CTRL_LEARN_DIS                        0x4000
#define MV88E6060_ATU_CTRL_ATU_SIZE                         0x3000
#define MV88E6060_ATU_CTRL_ATU_SIZE_256                     0x0000
#define MV88E6060_ATU_CTRL_ATU_SIZE_512                     0x1000
#define MV88E6060_ATU_CTRL_ATU_SIZE_1024                    0x2000
#define MV88E6060_ATU_CTRL_AGE_TIME                         0x0FF0
#define MV88E6060_ATU_CTRL_AGE_TIME_DEFAULT                 0x0130

//ATU Operation register
#define MV88E6060_ATU_OPERATION_ATU_BUSY                    0x8000
#define MV88E6060_ATU_OPERATION_ATU_OP                      0x7000
#define MV88E6060_ATU_OPERATION_ATU_OP_NOP                  0x0000
#define MV88E6060_ATU_OPERATION_ATU_OP_FLUSH_ALL            0x1000
#define MV88E6060_ATU_OPERATION_ATU_OP_FLUSH_UNLOCKED       0x2000
#define MV88E6060_ATU_OPERATION_ATU_OP_LOAD_PURGE           0x3000
#define MV88E6060_ATU_OPERATION_ATU_OP_GET_NEXT_DB          0x4000
#define MV88E6060_ATU_OPERATION_ATU_OP_FLUSH_ALL_DB         0x5000
#define MV88E6060_ATU_OPERATION_ATU_OP_FLUSH_UNLOCKED_DB    0x6000
#define MV88E6060_ATU_OPERATION_DB_NUM                      0x000F

//ATU Data register
#define MV88E6060_ATU_DATA_DPV                              0x03F0
#define MV88E6060_ATU_DATA_DPV_PORT0                        0x0010
#define MV88E6060_ATU_DATA_DPV_PORT1                        0x0020
#define MV88E6060_ATU_DATA_DPV_PORT2                        0x0040
#define MV88E6060_ATU_DATA_DPV_PORT3                        0x0080
#define MV88E6060_ATU_DATA_DPV_PORT4                        0x0100
#define MV88E6060_ATU_DATA_DPV_PORT5                        0x0200
#define MV88E6060_ATU_DATA_ENTRY_STATE                      0x000F
#define MV88E6060_ATU_DATA_ENTRY_STATE_INVALID              0x0000
#define MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_MULTICAST     0x0007
#define MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_UNICAST       0x000F
#define MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_MGMT          0x000E

//ATU Switch MAC Address Bytes 0 and 1 register
#define MV88E6060_ATU_MAC_ADDR_0_1_ATU_BYTE0                0xFF00
#define MV88E6060_ATU_MAC_ADDR_0_1_ATU_BYTE1                0x00FF

//ATU Switch MAC Address Bytes 2 and 3 register
#define MV88E6060_ATU_MAC_ADDR_2_3_ATU_BYTE2                0xFF00
#define MV88E6060_ATU_MAC_ADDR_2_3_ATU_BYTE3                0x00FF

//ATU Switch MAC Address Bytes 4 and 5 register
#define MV88E6060_ATU_MAC_ADDR_4_5_ATU_BYTE4                0xFF00
#define MV88E6060_ATU_MAC_ADDR_4_5_ATU_BYTE5                0x00FF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//88E6060 Ethernet switch driver
extern const SwitchDriver mv88e6060SwitchDriver;

//MV88E6060 related functions
error_t mv88e6060Init(NetInterface *interface);

void mv88e6060Tick(NetInterface *interface);

void mv88e6060EnableIrq(NetInterface *interface);
void mv88e6060DisableIrq(NetInterface *interface);

void mv88e6060EventHandler(NetInterface *interface);

error_t mv88e6060TagFrame(NetInterface *interface, NetBuffer *buffer,
   size_t *offset, NetTxAncillary *ancillary);

error_t mv88e6060UntagFrame(NetInterface *interface, uint8_t **frame,
   size_t *length, NetRxAncillary *ancillary);

bool_t mv88e6060GetLinkState(NetInterface *interface, uint8_t port);
uint32_t mv88e6060GetLinkSpeed(NetInterface *interface, uint8_t port);
NicDuplexMode mv88e6060GetDuplexMode(NetInterface *interface, uint8_t port);

void mv88e6060SetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state);

SwitchPortState mv88e6060GetPortState(NetInterface *interface, uint8_t port);

void mv88e6060SetAgingTime(NetInterface *interface, uint32_t agingTime);

void mv88e6060EnableIgmpSnooping(NetInterface *interface, bool_t enable);
void mv88e6060EnableMldSnooping(NetInterface *interface, bool_t enable);
void mv88e6060EnableRsvdMcastTable(NetInterface *interface, bool_t enable);

error_t mv88e6060AddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t mv88e6060DeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry);

error_t mv88e6060GetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void mv88e6060FlushStaticFdbTable(NetInterface *interface);

error_t mv88e6060GetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry);

void mv88e6060FlushDynamicFdbTable(NetInterface *interface, uint8_t port);

void mv88e6060SetUnknownMcastFwdPorts(NetInterface *interface,
   bool_t enable, uint32_t forwardPorts);

void mv88e6060WritePhyReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data);

uint16_t mv88e6060ReadPhyReg(NetInterface *interface, uint8_t port,
   uint8_t address);

void mv88e6060DumpPhyReg(NetInterface *interface, uint8_t port);

void mv88e6060WriteSwitchPortReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data);

uint16_t mv88e6060ReadSwitchPortReg(NetInterface *interface, uint8_t port,
   uint8_t address);

void mv88e6060WriteSwitchGlobalReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t mv88e6060ReadSwitchGlobalReg(NetInterface *interface, uint8_t address);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
