/**
 * @file mv88e1512_driver.h
 * @brief 88E1512 Gigabit Ethernet PHY driver
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

#ifndef _MV88E1512_DRIVER_H
#define _MV88E1512_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef MV88E1512_PHY_ADDR
   #define MV88E1512_PHY_ADDR 0
#elif (MV88E1512_PHY_ADDR < 0 || MV88E1512_PHY_ADDR > 31)
   #error MV88E1512_PHY_ADDR parameter is not valid
#endif

//MV88E1512 PHY registers (page 0)
#define MV88E1512_COPPER_CTRL                                 0x00
#define MV88E1512_COPPER_STAT                                 0x01
#define MV88E1512_COPPER_PHYID1                               0x02
#define MV88E1512_COPPER_PHYID2                               0x03
#define MV88E1512_COPPER_ANAR                                 0x04
#define MV88E1512_COPPER_ANLPAR                               0x05
#define MV88E1512_COPPER_ANER                                 0x06
#define MV88E1512_COPPER_ANNPR                                0x07
#define MV88E1512_COPPER_ANLPNPR                              0x08
#define MV88E1512_GBCR                                        0x09
#define MV88E1512_GBSR                                        0x0A
#define MV88E1512_GBESR                                       0x0F
#define MV88E1512_COPPER_CTRL1                                0x10
#define MV88E1512_COPPER_STAT1                                0x11
#define MV88E1512_COPPER_INT_EN                               0x12
#define MV88E1512_COPPER_INT_STAT                             0x13
#define MV88E1512_COPPER_CTRL2                                0x14
#define MV88E1512_COPPER_RX_ERR_COUNTER                       0x15
#define MV88E1512_PAGSR                                       0x16
#define MV88E1512_GISR                                        0x17

//MV88E1512 PHY registers (page 1)
#define MV88E1512_FIBER_CTRL                                  0x00
#define MV88E1512_FIBER_STAT                                  0x01
#define MV88E1512_FIBER_PHYID1                                0x02
#define MV88E1512_FIBER_PHYID2                                0x03
#define MV88E1512_FIBER_ANAR                                  0x04
#define MV88E1512_FIBER_ANLPAR                                0x05
#define MV88E1512_FIBER_ANER                                  0x06
#define MV88E1512_FIBER_ANNPR                                 0x07
#define MV88E1512_FIBER_ANLPNPR                               0x08
#define MV88E1512_EXTENDED_STATUS                             0x0F
#define MV88E1512_FIBER_CTRL1                                 0x10
#define MV88E1512_FIBER_STAT1                                 0x11
#define MV88E1512_FIBER_INT_EN                                0x12
#define MV88E1512_FIBER_INT_STAT                              0x13
#define MV88E1512_PRBS_CTRL                                   0x17
#define MV88E1512_PRBS_ERR_COUNTER_LSB                        0x18
#define MV88E1512_PRBS_ERR_COUNTER_MSB                        0x19
#define MV88E1512_FIBER_CTRL2                                 0x1A

//MV88E1512 PHY registers (page 2)
#define MV88E1512_MAC_CTRL1                                   0x10
#define MV88E1512_MAC_INT_EN                                  0x12
#define MV88E1512_MAC_INT_STAT                                0x13
#define MV88E1512_MAC_CTRL2                                   0x15
#define MV88E1512_RGMII_OUT_IMP_CAL                           0x18

//MV88E1512 PHY registers (page 3)
#define MV88E1512_LED_FUNC_CTRL                               0x10
#define MV88E1512_LED_POL_CTRL                                0x11
#define MV88E1512_LED_TIMER_CTRL                              0x12

//MV88E1512 PHY registers (page 5)
#define MV88E1512_1000BT_PAIR_SKEW                            0x14
#define MV88E1512_1000BT_PAIR_SWAP_POL                        0x15

//MV88E1512 PHY registers (page 6)
#define MV88E1512_COPPER_PKT_GEN                              0x10
#define MV88E1512_COPPER_CRC_COUNTERS                         0x11
#define MV88E1512_CHECKER_CTRL                                0x12
#define MV88E1512_COPPER_PKT_GEN_IPG_CTRL                     0x13
#define MV88E1512_LATE_COL_COUNTERS_1_2                       0x17
#define MV88E1512_LATE_COL_COUNTERS_3_4                       0x18
#define MV88E1512_LATE_COL_WIN_ADJ_LINK_DISCONNECT            0x19
#define MV88E1512_MISC_TEST                                   0x1A
#define MV88E1512_TEMP_SENSOR                                 0x1B

//Copper Control register
#define MV88E1512_COPPER_CTRL_RESET                           0x8000
#define MV88E1512_COPPER_CTRL_LOOPBACK                        0x4000
#define MV88E1512_COPPER_CTRL_SPEED_SEL_LSB                   0x2000
#define MV88E1512_COPPER_CTRL_AN_EN                           0x1000
#define MV88E1512_COPPER_CTRL_POWER_DOWN                      0x0800
#define MV88E1512_COPPER_CTRL_ISOLATE                         0x0400
#define MV88E1512_COPPER_CTRL_RESTART_AN                      0x0200
#define MV88E1512_COPPER_CTRL_DUPLEX_MODE                     0x0100
#define MV88E1512_COPPER_CTRL_COL_TEST                        0x0080
#define MV88E1512_COPPER_CTRL_SPEED_SEL_MSB                   0x0040

//Copper Status register
#define MV88E1512_COPPER_STAT_100BT4                          0x8000
#define MV88E1512_COPPER_STAT_100BTX_FD                       0x4000
#define MV88E1512_COPPER_STAT_100BTX_HD                       0x2000
#define MV88E1512_COPPER_STAT_10BT_FD                         0x1000
#define MV88E1512_COPPER_STAT_10BT_HD                         0x0800
#define MV88E1512_COPPER_STAT_100BT2_FD                       0x0400
#define MV88E1512_COPPER_STAT_100BT2_HD                       0x0200
#define MV88E1512_COPPER_STAT_EXTENDED_STATUS                 0x0100
#define MV88E1512_COPPER_STAT_MF_PREAMBLE_SUPPR               0x0040
#define MV88E1512_COPPER_STAT_AN_COMPLETE                     0x0020
#define MV88E1512_COPPER_STAT_REMOTE_FAULT                    0x0010
#define MV88E1512_COPPER_STAT_AN_CAPABLE                      0x0008
#define MV88E1512_COPPER_STAT_LINK_STATUS                     0x0004
#define MV88E1512_COPPER_STAT_JABBER_DETECT                   0x0002
#define MV88E1512_COPPER_STAT_EXTENDED_CAPABLE                0x0001

//PHY Identifier 1 register
#define MV88E1512_COPPER_PHYID1_OUI_MSB                       0xFFFF
#define MV88E1512_COPPER_PHYID1_OUI_MSB_DEFAULT               0x0141

//PHY Identifier 2 register
#define MV88E1512_COPPER_PHYID2_OUI_LSB                       0xFC00
#define MV88E1512_COPPER_PHYID2_OUI_LSB_DEFAULT               0x0C00
#define MV88E1512_COPPER_PHYID2_MODEL_NUM                     0x03F0
#define MV88E1512_COPPER_PHYID2_MODEL_NUM_DEFAULT             0x01D0
#define MV88E1512_COPPER_PHYID2_REVISION_NUM                  0x000F

//Copper Auto-Negotiation Advertisement register
#define MV88E1512_COPPER_ANAR_NEXT_PAGE                       0x8000
#define MV88E1512_COPPER_ANAR_ACK                             0x4000
#define MV88E1512_COPPER_ANAR_REMOTE_FAULT                    0x2000
#define MV88E1512_COPPER_ANAR_ASYM_PAUSE                      0x0800
#define MV88E1512_COPPER_ANAR_PAUSE                           0x0400
#define MV88E1512_COPPER_ANAR_100BT4                          0x0200
#define MV88E1512_COPPER_ANAR_100BTX_FD                       0x0100
#define MV88E1512_COPPER_ANAR_100BTX_HD                       0x0080
#define MV88E1512_COPPER_ANAR_10BT_FD                         0x0040
#define MV88E1512_COPPER_ANAR_10BT_HD                         0x0020
#define MV88E1512_COPPER_ANAR_SELECTOR                        0x001F
#define MV88E1512_COPPER_ANAR_SELECTOR_DEFAULT                0x0001

//Copper Link Partner Ability register
#define MV88E1512_COPPER_ANLPAR_NEXT_PAGE                     0x8000
#define MV88E1512_COPPER_ANLPAR_ACK                           0x4000
#define MV88E1512_COPPER_ANLPAR_REMOTE_FAULT                  0x2000
#define MV88E1512_COPPER_ANLPAR_TECH_ABLE                     0x1000
#define MV88E1512_COPPER_ANLPAR_ASYM_PAUSE                    0x0800
#define MV88E1512_COPPER_ANLPAR_PAUSE                         0x0400
#define MV88E1512_COPPER_ANLPAR_100BT4                        0x0200
#define MV88E1512_COPPER_ANLPAR_100BTX_FD                     0x0100
#define MV88E1512_COPPER_ANLPAR_100BTX_HD                     0x0080
#define MV88E1512_COPPER_ANLPAR_10BT_FD                       0x0040
#define MV88E1512_COPPER_ANLPAR_10BT_HD                       0x0020
#define MV88E1512_COPPER_ANLPAR_SELECTOR                      0x001F
#define MV88E1512_COPPER_ANLPAR_SELECTOR_DEFAULT              0x0001

//Copper Auto-Negotiation Expansion register
#define MV88E1512_COPPER_ANER_PAR_DETECT_FAULT                0x0010
#define MV88E1512_COPPER_ANER_LP_NEXT_PAGE_ABLE               0x0008
#define MV88E1512_COPPER_ANER_NEXT_PAGE_ABLE                  0x0004
#define MV88E1512_COPPER_ANER_PAGE_RECEIVED                   0x0002
#define MV88E1512_COPPER_ANER_LP_AN_ABLE                      0x0001

//Copper Next Page Transmit register
#define MV88E1512_COPPER_ANNPR_NEXT_PAGE                      0x8000
#define MV88E1512_COPPER_ANNPR_MSG_PAGE                       0x2000
#define MV88E1512_COPPER_ANNPR_ACK2                           0x1000
#define MV88E1512_COPPER_ANNPR_TOGGLE                         0x0800
#define MV88E1512_COPPER_ANNPR_MESSAGE                        0x07FF

//Copper Link Partner Next Page register
#define MV88E1512_COPPER_ANLPNPR_NEXT_PAGE                    0x8000
#define MV88E1512_COPPER_ANLPNPR_ACK                          0x4000
#define MV88E1512_COPPER_ANLPNPR_MSG_PAGE                     0x2000
#define MV88E1512_COPPER_ANLPNPR_ACK2                         0x1000
#define MV88E1512_COPPER_ANLPNPR_TOGGLE                       0x0800
#define MV88E1512_COPPER_ANLPNPR_MESSAGE                      0x07FF

//1000BASE-T Control register
#define MV88E1512_GBCR_TEST_MODE                              0xE000
#define MV88E1512_GBCR_MS_MAN_CONF_EN                         0x1000
#define MV88E1512_GBCR_MS_MAN_CONF_VAL                        0x0800
#define MV88E1512_GBCR_PORT_TYPE                              0x0400
#define MV88E1512_GBCR_1000BT_FD                              0x0200
#define MV88E1512_GBCR_1000BT_HD                              0x0100

//1000BASE-T Status register
#define MV88E1512_GBSR_MS_CONF_FAULT                          0x8000
#define MV88E1512_GBSR_MS_CONF_RES                            0x4000
#define MV88E1512_GBSR_LOCAL_RECEIVER_STATUS                  0x2000
#define MV88E1512_GBSR_REMOTE_RECEIVER_STATUS                 0x1000
#define MV88E1512_GBSR_LP_1000BT_FD                           0x0800
#define MV88E1512_GBSR_LP_1000BT_HD                           0x0400
#define MV88E1512_GBSR_IDLE_ERR_COUNT                         0x00FF

//Extended Status register
#define MV88E1512_GBESR_1000BX_FD                             0x8000
#define MV88E1512_GBESR_1000BX_HD                             0x4000
#define MV88E1512_GBESR_1000BT_FD                             0x2000
#define MV88E1512_GBESR_1000BT_HD                             0x1000

//Copper Specific Control 1 register
#define MV88E1512_COPPER_CTRL1_LINK_PULSE_DIS                 0x8000
#define MV88E1512_COPPER_CTRL1_FORCE_LINK_GOOD                0x0400
#define MV88E1512_COPPER_CTRL1_ENERGY_DETECT                  0x0300
#define MV88E1512_COPPER_CTRL1_MDI_CROSSOVER_MODE             0x0060
#define MV88E1512_COPPER_CTRL1_MDI_CROSSOVER_MODE_MANUAL_MDI  0x0000
#define MV88E1512_COPPER_CTRL1_MDI_CROSSOVER_MODE_MANUAL_MDIX 0x0020
#define MV88E1512_COPPER_CTRL1_MDI_CROSSOVER_MODE_AUTO        0x0060
#define MV88E1512_COPPER_CTRL1_TX_DIS                         0x0008
#define MV88E1512_COPPER_CTRL1_POWER_DOWN                     0x0004
#define MV88E1512_COPPER_CTRL1_POLARITY_REVERSAL_DIS          0x0002
#define MV88E1512_COPPER_CTRL1_JABBER_DIS                     0x0001

//Copper Specific Status 1 register
#define MV88E1512_COPPER_STAT1_SPEED                          0xC000
#define MV88E1512_COPPER_STAT1_SPEED_10MBPS                   0x0000
#define MV88E1512_COPPER_STAT1_SPEED_100MBPS                  0x4000
#define MV88E1512_COPPER_STAT1_SPEED_1000MBPS                 0x8000
#define MV88E1512_COPPER_STAT1_DUPLEX                         0x2000
#define MV88E1512_COPPER_STAT1_PAGE_RECEIVED                  0x1000
#define MV88E1512_COPPER_STAT1_SPEED_DUPLEX_RESOLVED          0x0800
#define MV88E1512_COPPER_STAT1_LINK                           0x0400
#define MV88E1512_COPPER_STAT1_TX_PAUSE_EN                    0x0200
#define MV88E1512_COPPER_STAT1_RX_PAUSE_EN                    0x0100
#define MV88E1512_COPPER_STAT1_MDI_CROSSOVER_STATUS           0x0040
#define MV88E1512_COPPER_STAT1_ENERGY_DETECT_STATUS           0x0010
#define MV88E1512_COPPER_STAT1_GLOBAL_LINK_STATUS             0x0008
#define MV88E1512_COPPER_STAT1_POLARITY                       0x0002
#define MV88E1512_COPPER_STAT1_JABBER                         0x0001

//Copper Specific Interrupt Enable register
#define MV88E1512_COPPER_INT_EN_AN_ERROR                      0x8000
#define MV88E1512_COPPER_INT_EN_SPEED_CHANGED                 0x4000
#define MV88E1512_COPPER_INT_EN_DUPLEX_CHANGED                0x2000
#define MV88E1512_COPPER_INT_EN_PAGE_RECEIVED                 0x1000
#define MV88E1512_COPPER_INT_EN_AN_COMPLETE                   0x0800
#define MV88E1512_COPPER_INT_EN_LINK_STATUS_CHANGED           0x0400
#define MV88E1512_COPPER_INT_EN_SYMBOL_ERROR                  0x0200
#define MV88E1512_COPPER_INT_EN_FALSE_CARRIER                 0x0100
#define MV88E1512_COPPER_INT_EN_MDI_CROSSOVER_CHANGED         0x0040
#define MV88E1512_COPPER_INT_EN_ENERGY_DETECT                 0x0010
#define MV88E1512_COPPER_INT_EN_FLP_COMPLETE                  0x0008
#define MV88E1512_COPPER_INT_EN_POLARITY_CHANGED              0x0002
#define MV88E1512_COPPER_INT_EN_JABBER                        0x0001

//Copper Interrupt Status register
#define MV88E1512_COPPER_INT_STAT_AN_ERROR                    0x8000
#define MV88E1512_COPPER_INT_STAT_SPEED_CHANGED               0x4000
#define MV88E1512_COPPER_INT_STAT_DUPLEX_CHANGED              0x2000
#define MV88E1512_COPPER_INT_STAT_PAGE_RECEIVED               0x1000
#define MV88E1512_COPPER_INT_STAT_AN_COMPLETE                 0x0800
#define MV88E1512_COPPER_INT_STAT_LINK_STATUS_CHANGED         0x0400
#define MV88E1512_COPPER_INT_STAT_SYMBOL_ERROR                0x0200
#define MV88E1512_COPPER_INT_STAT_FALSE_CARRIER               0x0100
#define MV88E1512_COPPER_INT_STAT_MDI_CROSSOVER_CHANGED       0x0040
#define MV88E1512_COPPER_INT_STAT_ENERGY_DETECT               0x0010
#define MV88E1512_COPPER_INT_STAT_FLP_COMPLETE                0x0008
#define MV88E1512_COPPER_INT_STAT_POLARITY_CHANGED            0x0002
#define MV88E1512_COPPER_INT_STAT_JABBER                      0x0001

//Copper Specific Control 2 register
#define MV88E1512_COPPER_CTRL2_BREAK_LINK_ON_INSUF_IPG        0x0040
#define MV88E1512_COPPER_CTRL2_REVERSE_MDIPN3_TX_POL          0x0008
#define MV88E1512_COPPER_CTRL2_REVERSE_MDIPN2_TX_POL          0x0004
#define MV88E1512_COPPER_CTRL2_REVERSE_MDIPN1_TX_POL          0x0002
#define MV88E1512_COPPER_CTRL2_REVERSE_MDIPN0_TX_POL          0x0001

//Page Address register
#define MV88E1512_PAGSR_PAGE_SEL                              0x00FF

//Global Interrupt Status register
#define MV88E1512_GISR_INTERRUPT                              0x0001

//Fiber Control register
#define MV88E1512_FIBER_CTRL_RESET                            0x8000
#define MV88E1512_FIBER_CTRL_LOOPBACK                         0x4000
#define MV88E1512_FIBER_CTRL_SPEED_SEL_LSB                    0x2000
#define MV88E1512_FIBER_CTRL_AN_EN                            0x1000
#define MV88E1512_FIBER_CTRL_POWER_DOWN                       0x0800
#define MV88E1512_FIBER_CTRL_ISOLATE                          0x0400
#define MV88E1512_FIBER_CTRL_RESTART_AN                       0x0200
#define MV88E1512_FIBER_CTRL_DUPLEX_MODE                      0x0100
#define MV88E1512_FIBER_CTRL_COL_TEST                         0x0080
#define MV88E1512_FIBER_CTRL_SPEED_SEL_MSB                    0x0040

//Fiber Status register
#define MV88E1512_FIBER_STAT_100BT4                           0x8000
#define MV88E1512_FIBER_STAT_100BTX_FD                        0x4000
#define MV88E1512_FIBER_STAT_100BTX_HD                        0x2000
#define MV88E1512_FIBER_STAT_10BT_FD                          0x1000
#define MV88E1512_FIBER_STAT_10BT_HD                          0x0800
#define MV88E1512_FIBER_STAT_100BT2_FD                        0x0400
#define MV88E1512_FIBER_STAT_100BT2_HD                        0x0200
#define MV88E1512_FIBER_STAT_EXTENDED_STATUS                  0x0100
#define MV88E1512_FIBER_STAT_MF_PREAMBLE_SUPPR                0x0040
#define MV88E1512_FIBER_STAT_AN_COMPLETE                      0x0020
#define MV88E1512_FIBER_STAT_REMOTE_FAULT                     0x0010
#define MV88E1512_FIBER_STAT_AN_CAPABLE                       0x0008
#define MV88E1512_FIBER_STAT_LINK_STATUS                      0x0004
#define MV88E1512_FIBER_STAT_EXTENDED_CAPABLE                 0x0001

//PHY Identifier 1 register
#define MV88E1512_FIBER_PHYID1_OUI_MSB                        0xFFFF
#define MV88E1512_FIBER_PHYID1_OUI_MSB_DEFAULT                0x0141

//PHY Identifier 2 register
#define MV88E1512_FIBER_PHYID2_OUI_LSB                        0xFC00
#define MV88E1512_FIBER_PHYID2_OUI_LSB_DEFAULT                0x0C00
#define MV88E1512_FIBER_PHYID2_MODEL_NUM                      0x03F0
#define MV88E1512_FIBER_PHYID2_MODEL_NUM_DEFAULT              0x01D0
#define MV88E1512_FIBER_PHYID2_REVISION_NUM                   0x000F

//Fiber Auto-Negotiation Advertisement register
#define MV88E1512_FIBER_ANAR_NEXT_PAGE                        0x8000
#define MV88E1512_FIBER_ANAR_REMOTE_FAULT                     0x3000
#define MV88E1512_FIBER_ANAR_REMOTE_FAULT_NO_ERROR            0x0000
#define MV88E1512_FIBER_ANAR_REMOTE_FAULT_LINK_FAILURE        0x1000
#define MV88E1512_FIBER_ANAR_REMOTE_FAULT_OFFLINE             0x2000
#define MV88E1512_FIBER_ANAR_REMOTE_FAULT_AN_ERROR            0x3000
#define MV88E1512_FIBER_ANAR_PAUSE                            0x0180
#define MV88E1512_FIBER_ANAR_1000BX_HD                        0x0040
#define MV88E1512_FIBER_ANAR_1000BX_FD                        0x0020

//Fiber Link Partner Ability register
#define MV88E1512_FIBER_ANLPAR_NEXT_PAGE                      0x8000
#define MV88E1512_FIBER_ANLPAR_ACK                            0x4000
#define MV88E1512_FIBER_ANLPAR_REMOTE_FAULT                   0x3000
#define MV88E1512_FIBER_ANLPAR_REMOTE_FAULT_NO_ERROR          0x0000
#define MV88E1512_FIBER_ANLPAR_REMOTE_FAULT_LINK_FAILURE      0x1000
#define MV88E1512_FIBER_ANLPAR_REMOTE_FAULT_OFFLINE           0x2000
#define MV88E1512_FIBER_ANLPAR_REMOTE_FAULT_AN_ERROR          0x3000
#define MV88E1512_FIBER_ANLPAR_ASYM_PAUSE                     0x0180
#define MV88E1512_FIBER_ANLPAR_1000BX_HD                      0x0040
#define MV88E1512_FIBER_ANLPAR_1000BX_FD                      0x0020

//Fiber Auto-Negotiation Expansion register
#define MV88E1512_FIBER_ANER_LP_NEXT_PAGE_ABLE                0x0008
#define MV88E1512_FIBER_ANER_NEXT_PAGE_ABLE                   0x0004
#define MV88E1512_FIBER_ANER_PAGE_RECEIVED                    0x0002
#define MV88E1512_FIBER_ANER_LP_AN_ABLE                       0x0001

//Fiber Next Page Transmit register
#define MV88E1512_FIBER_ANNPR_NEXT_PAGE                       0x8000
#define MV88E1512_FIBER_ANNPR_MSG_PAGE                        0x2000
#define MV88E1512_FIBER_ANNPR_ACK2                            0x1000
#define MV88E1512_FIBER_ANNPR_TOGGLE                          0x0800
#define MV88E1512_FIBER_ANNPR_MESSAGE                         0x07FF

//Fiber Link Partner Next Page register
#define MV88E1512_FIBER_ANLPNPR_NEXT_PAGE                     0x8000
#define MV88E1512_FIBER_ANLPNPR_ACK                           0x4000
#define MV88E1512_FIBER_ANLPNPR_MSG_PAGE                      0x2000
#define MV88E1512_FIBER_ANLPNPR_ACK2                          0x1000
#define MV88E1512_FIBER_ANLPNPR_TOGGLE                        0x0800
#define MV88E1512_FIBER_ANLPNPR_MESSAGE                       0x07FF

//Extended Status register
#define MV88E1512_EXTENDED_STATUS_1000BX_FD                   0x8000
#define MV88E1512_EXTENDED_STATUS_1000BX_HD                   0x4000
#define MV88E1512_EXTENDED_STATUS_1000BT_FD                   0x2000
#define MV88E1512_EXTENDED_STATUS_1000BT_HD                   0x1000

//Fiber Specific Control 1 register
#define MV88E1512_FIBER_CTRL1_BLOCK_CARRIER_EXTENSION         0x2000
#define MV88E1512_FIBER_CTRL1_ASSERT_CRS_TX                   0x0800
#define MV88E1512_FIBER_CTRL1_FORCE_LINK_GOOD                 0x0400
#define MV88E1512_FIBER_CTRL1_SERDES_LOOPBACK_TYPE            0x0100
#define MV88E1512_FIBER_CTRL1_MARVELL_REMOTE_FAULT_IND        0x0020
#define MV88E1512_FIBER_CTRL1_IEEE_REMOTE_FAULT_IND           0x0010
#define MV88E1512_FIBER_CTRL1_INT_POLARITY                    0x0004
#define MV88E1512_FIBER_CTRL1_MODE                            0x0003

//Fiber Specific Status register
#define MV88E1512_FIBER_STAT1_SPEED                           0xC000
#define MV88E1512_FIBER_STAT1_SPEED_10MBPS                    0x0000
#define MV88E1512_FIBER_STAT1_SPEED_100MBPS                   0x4000
#define MV88E1512_FIBER_STAT1_SPEED_1000MBPS                  0x8000
#define MV88E1512_FIBER_STAT1_DUPLEX                          0x2000
#define MV88E1512_FIBER_STAT1_PAGE_RECEIVED                   0x1000
#define MV88E1512_FIBER_STAT1_SPEED_DUPLEX_RESOLVED           0x0800
#define MV88E1512_FIBER_STAT1_LINK                            0x0400
#define MV88E1512_FIBER_STAT1_REMOTE_FAULT_RECEIVED           0x00C0
#define MV88E1512_FIBER_STAT1_SYNC_STATUS                     0x0020
#define MV88E1512_FIBER_STAT1_ENERGY_DETECT_STATUS            0x0010
#define MV88E1512_FIBER_STAT1_TX_PAUSE_EN                     0x0008
#define MV88E1512_FIBER_STAT1_RX_PAUSE_EN                     0x0004

//Copper Specific Interrupt Enable register
#define MV88E1512_FIBER_INT_EN_SPEED_CHANGED                  0x4000
#define MV88E1512_FIBER_INT_EN_DUPLEX_CHANGED                 0x2000
#define MV88E1512_FIBER_INT_EN_PAGE_RECEIVED                  0x1000
#define MV88E1512_FIBER_INT_EN_AN_COMPLETE                    0x0800
#define MV88E1512_FIBER_INT_EN_LINK_STATUS_CHANGED            0x0400
#define MV88E1512_FIBER_INT_EN_SYMBOL_ERROR                   0x0200
#define MV88E1512_FIBER_INT_EN_FALSE_CARRIER                  0x0100
#define MV88E1512_FIBER_INT_EN_FIFO_OVER_UNDERFLOW            0x0080
#define MV88E1512_FIBER_INT_EN_REMOTE_FAULT_RECEIVE           0x0020
#define MV88E1512_FIBER_INT_EN_ENERGY_DETECT                  0x0010

//Copper Interrupt Status register
#define MV88E1512_FIBER_INT_STAT_SPEED_CHANGED                0x4000
#define MV88E1512_FIBER_INT_STAT_DUPLEX_CHANGED               0x2000
#define MV88E1512_FIBER_INT_STAT_PAGE_RECEIVED                0x1000
#define MV88E1512_FIBER_INT_STAT_AN_COMPLETE                  0x0800
#define MV88E1512_FIBER_INT_STAT_LINK_STATUS_CHANGED          0x0400
#define MV88E1512_FIBER_INT_STAT_SYMBOL_ERROR                 0x0200
#define MV88E1512_FIBER_INT_STAT_FALSE_CARRIER                0x0100
#define MV88E1512_FIBER_INT_STAT_FIFO_OVER_UNDERFLOW          0x0080
#define MV88E1512_FIBER_INT_STAT_REMOTE_FAULT_RECEIVE         0x0020
#define MV88E1512_FIBER_INT_STAT_ENERGY_DETECT                0x0010

//PRBS Control register
#define MV88E1512_PRBS_CTRL_INVERT_CHECK_POL                  0x0080
#define MV88E1512_PRBS_CTRL_INVERT_GEN_POL                    0x0040
#define MV88E1512_PRBS_CTRL_PRBS_LOCK                         0x0020
#define MV88E1512_PRBS_CTRL_CLEAR_COUNTER                     0x0010
#define MV88E1512_PRBS_CTRL_PATTERN_SEL                       0x000C
#define MV88E1512_PRBS_CTRL_PRBS_CHECK_EN                     0x0002
#define MV88E1512_PRBS_CTRL_PRBS_GEN_EN                       0x0001

//Fiber Specific Control 2 register
#define MV88E1512_FIBER_CTRL2_FORCE_INT                       0x8000
#define MV88E1512_FIBER_CTRL2_1000BX_NOISE_FILT               0x4000
#define MV88E1512_FIBER_CTRL2_FEFI_EN                         0x0200
#define MV88E1512_FIBER_CTRL2_SERIAL_IF_AN_BYPASS_EN          0x0040
#define MV88E1512_FIBER_CTRL2_SERIAL_IF_AN_BYPASS_STATUS      0x0020
#define MV88E1512_FIBER_CTRL2_FIBER_TX_DIS                    0x0008
#define MV88E1512_FIBER_CTRL2_SGMII_FIBER_OUT_AMPL            0x0007

//MAC Specific Control 1 register
#define MV88E1512_MAC_CTRL1_PASS_ODD_NIBBLE_PREAMBLES         0x0040
#define MV88E1512_MAC_CTRL1_RGMII_IF_POWER_DOWN               0x0008

//MAC Specific Interrupt Enable register
#define MV88E1512_MAC_INT_EN_FIFO_OVER_UNDERFLOW              0x0080
#define MV88E1512_MAC_INT_EN_FIFO_IDLE_INSERTED               0x0008
#define MV88E1512_MAC_INT_EN_FIFO_IDLE_DELETED                0x0004

//MAC Specific Status register
#define MV88E1512_MAC_INT_STAT_FIFO_OVER_UNDERFLOW            0x0080
#define MV88E1512_MAC_INT_STAT_FIFO_IDLE_INSERTED             0x0008
#define MV88E1512_MAC_INT_STAT_FIFO_IDLE_DELETED              0x0004

//MAC Specific Control 2 register
#define MV88E1512_MAC_CTRL2_COPPER_LINE_LOOPBACK              0x4000
#define MV88E1512_MAC_CTRL2_DEFAULT_MAC_IF_SPEED_LSB          0x2000
#define MV88E1512_MAC_CTRL2_DEFAULT_MAC_IF_SPEED_MSB          0x0040
#define MV88E1512_MAC_CTRL2_RGMII_RX_TIMING_CTRL              0x0020
#define MV88E1512_MAC_CTRL2_RGMII_TX_TIMING_CTRL              0x0010
#define MV88E1512_MAC_CTRL2_BLOCK_CARRIER_EXTENSION           0x0008

//RGMII Output Impedance Calibration Override register
#define MV88E1512_RGMII_OUT_IMP_CAL_VDDO_LEVEL                0x2000
#define MV88E1512_RGMII_OUT_IMP_CAL_1V8_VDDO_USED             0x1000

//LED[2:0] Function Control register
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL                     0x0F00
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_LINK                0x0000
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_LINK_ACT            0x0100
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_DUPLEX_COL          0x0200
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_ACT                 0x0300
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_ACT_BLINK           0x0400
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_TX                  0x0500
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_SPEED_10_1000       0x0600
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_SPEED_10            0x0700
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_FORCE_OFF           0x0800
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_FORCE_ON            0x0900
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_FORCE_HIZ           0x0A00
#define MV88E1512_LED_FUNC_CTRL_LED2_CTRL_FORCE_BLINK         0x0B00
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL                     0x00F0
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_LINK                0x0000
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_LINK_ACT            0x0010
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_DUPLEX_COL          0x0020
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_ACT                 0x0030
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_ACT_BLINK           0x0040
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_TX                  0x0050
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_SPEED_10_1000       0x0060
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_SPEED_10            0x0070
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_FORCE_OFF           0x0080
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_FORCE_ON            0x0090
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_FORCE_HIZ           0x00A0
#define MV88E1512_LED_FUNC_CTRL_LED1_CTRL_FORCE_BLINK         0x00B0
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL                     0x000F
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_LINK                0x0000
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_LINK_ACT            0x0001
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_DUPLEX_COL          0x0002
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_ACT                 0x0003
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_ACT_BLINK           0x0004
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_TX                  0x0005
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_SPEED_10_1000       0x0006
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_SPEED_10            0x0007
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_FORCE_OFF           0x0008
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_FORCE_ON            0x0009
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_FORCE_HIZ           0x000A
#define MV88E1512_LED_FUNC_CTRL_LED0_CTRL_FORCE_BLINK         0x000B

//LED[2:0] Polarity Control register
#define MV88E1512_LED_POL_CTRL_LED1_MIX_PERCENT               0xF000
#define MV88E1512_LED_POL_CTRL_LED0_MIX_PERCENT               0x0F00
#define MV88E1512_LED_POL_CTRL_LED2_POL                       0x0030
#define MV88E1512_LED_POL_CTRL_LED2_POL_ON_LOW_OFF_HIGH       0x0000
#define MV88E1512_LED_POL_CTRL_LED2_POL_ON_HIGH_OFF_LOW       0x0010
#define MV88E1512_LED_POL_CTRL_LED2_POL_ON_LOW_OFF_TRISTATE   0x0020
#define MV88E1512_LED_POL_CTRL_LED2_POL_ON_HIGH_OFF_TRISTATE  0x0030
#define MV88E1512_LED_POL_CTRL_LED1_POL                       0x000C
#define MV88E1512_LED_POL_CTRL_LED1_POL_ON_LOW_OFF_HIGH       0x0000
#define MV88E1512_LED_POL_CTRL_LED1_POL_ON_HIGH_OFF_LOW       0x0004
#define MV88E1512_LED_POL_CTRL_LED1_POL_ON_LOW_OFF_TRISTATE   0x0008
#define MV88E1512_LED_POL_CTRL_LED1_POL_ON_HIGH_OFF_TRISTATE  0x000C
#define MV88E1512_LED_POL_CTRL_LED0_POL                       0x0003
#define MV88E1512_LED_POL_CTRL_LED0_POL_ON_LOW_OFF_HIGH       0x0000
#define MV88E1512_LED_POL_CTRL_LED0_POL_ON_HIGH_OFF_LOW       0x0001
#define MV88E1512_LED_POL_CTRL_LED0_POL_ON_LOW_OFF_TRISTATE   0x0002
#define MV88E1512_LED_POL_CTRL_LED0_POL_ON_HIGH_OFF_TRISTATE  0x0003

//LED Timer Control register
#define MV88E1512_LED_TIMER_CTRL_FORCE_INT                    0x8000
#define MV88E1512_LED_TIMER_CTRL_PULSE_STRETCH                0x7000
#define MV88E1512_LED_TIMER_CTRL_PULSE_STRETCH_NO             0x0000
#define MV88E1512_LED_TIMER_CTRL_PULSE_STRETCH_21MS_TO_42MS   0x1000
#define MV88E1512_LED_TIMER_CTRL_PULSE_STRETCH_42MS_TO_84MS   0x2000
#define MV88E1512_LED_TIMER_CTRL_PULSE_STRETCH_84MS_TO_170MS  0x3000
#define MV88E1512_LED_TIMER_CTRL_PULSE_STRETCH_170MS_TO_340MS 0x4000
#define MV88E1512_LED_TIMER_CTRL_PULSE_STRETCH_340MS_TO_670MS 0x5000
#define MV88E1512_LED_TIMER_CTRL_PULSE_STRETCH_670MS_TO_1_3S  0x6000
#define MV88E1512_LED_TIMER_CTRL_PULSE_STRETCH_1_3S_TO_2_7S   0x7000
#define MV88E1512_LED_TIMER_CTRL_INT_POL                      0x0800
#define MV88E1512_LED_TIMER_CTRL_BLINK_RATE                   0x0700
#define MV88E1512_LED_TIMER_CTRL_BLINK_RATE_42MS              0x0000
#define MV88E1512_LED_TIMER_CTRL_BLINK_RATE_84MS              0x0100
#define MV88E1512_LED_TIMER_CTRL_BLINK_RATE_170MS             0x0200
#define MV88E1512_LED_TIMER_CTRL_BLINK_RATE_340MS             0x0300
#define MV88E1512_LED_TIMER_CTRL_BLINK_RATE_670MS             0x0400
#define MV88E1512_LED_TIMER_CTRL_INT_EN                       0x0080
#define MV88E1512_LED_TIMER_CTRL_SPEED_OFF_PULSE              0x000C
#define MV88E1512_LED_TIMER_CTRL_SPEED_OFF_PULSE_84MS         0x0000
#define MV88E1512_LED_TIMER_CTRL_SPEED_OFF_PULSE_170MS        0x0004
#define MV88E1512_LED_TIMER_CTRL_SPEED_OFF_PULSE_340MS        0x0008
#define MV88E1512_LED_TIMER_CTRL_SPEED_OFF_PULSE_670MS        0x000C
#define MV88E1512_LED_TIMER_CTRL_SPEED_ON_PULSE               0x0003
#define MV88E1512_LED_TIMER_CTRL_SPEED_ON_PULSE_84MS          0x0000
#define MV88E1512_LED_TIMER_CTRL_SPEED_ON_PULSE_170MS         0x0001
#define MV88E1512_LED_TIMER_CTRL_SPEED_ON_PULSE_340MS         0x0002
#define MV88E1512_LED_TIMER_CTRL_SPEED_ON_PULSE_670MS         0x0003

//1000BASE-T Pair Skew register
#define MV88E1512_1000BT_PAIR_SKEW_PAIR_7_8_MDI3              0xF000
#define MV88E1512_1000BT_PAIR_SKEW_PAIR_4_5_MDI2              0x0F00
#define MV88E1512_1000BT_PAIR_SKEW_PAIR_3_6_MDI1              0x00F0
#define MV88E1512_1000BT_PAIR_SKEW_PAIR_1_2_MDI0              0x000F

//1000BASE-T Pair Swap and Polarity register
#define MV88E1512_1000BT_PAIR_SWAP_POL_REG_20_5_21_5_VALID    0x0040
#define MV88E1512_1000BT_PAIR_SWAP_POL_C_D_CROSSOVER          0x0020
#define MV88E1512_1000BT_PAIR_SWAP_POL_A_B_CROSSOVER          0x0010
#define MV88E1512_1000BT_PAIR_SWAP_POL_PAIR_7_8_POL           0x0008
#define MV88E1512_1000BT_PAIR_SWAP_POL_PAIR_4_5_POL           0x0004
#define MV88E1512_1000BT_PAIR_SWAP_POL_PAIR_3_6_POL           0x0002
#define MV88E1512_1000BT_PAIR_SWAP_POL_PAIR_1_2_POL           0x0001

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//88E1512 Ethernet PHY driver
extern const PhyDriver mv88e1512PhyDriver;

//88E1512 related functions
error_t mv88e1512Init(NetInterface *interface);

void mv88e1512Tick(NetInterface *interface);

void mv88e1512EnableIrq(NetInterface *interface);
void mv88e1512DisableIrq(NetInterface *interface);

void mv88e1512EventHandler(NetInterface *interface);

void mv88e1512WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t mv88e1512ReadPhyReg(NetInterface *interface, uint8_t address);

void mv88e1512DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
