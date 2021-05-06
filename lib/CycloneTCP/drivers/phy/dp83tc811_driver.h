/**
 * @file dp83tc811_driver.h
 * @brief DP83TC811 Ethernet PHY driver
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

#ifndef _DP83TC811_DRIVER_H
#define _DP83TC811_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef DP83TC811_PHY_ADDR
   #define DP83TC811_PHY_ADDR 0
#elif (DP83TC811_PHY_ADDR < 0 || DP83TC811_PHY_ADDR > 31)
   #error DP83TC811_PHY_ADDR parameter is not valid
#endif

//DP83TC811 PHY registers
#define DP83TC811_BMCR                                     0x00
#define DP83TC811_BMSR                                     0x01
#define DP83TC811_PHYID1                                   0x02
#define DP83TC811_PHYID2                                   0x03
#define DP83TC811_TDR_AUTO                                 0x09
#define DP83TC811_REGCR                                    0x0D
#define DP83TC811_ADDAR                                    0x0E
#define DP83TC811_INT_TEST                                 0x11
#define DP83TC811_INT_STAT1                                0x12
#define DP83TC811_INT_STAT2                                0x13
#define DP83TC811_FCSCR                                    0x14
#define DP83TC811_RECR                                     0x15
#define DP83TC811_BISTCR                                   0x16
#define DP83TC811_XMII_CTRL                                0x17
#define DP83TC811_INT_STAT3                                0x18
#define DP83TC811_BICTSR1                                  0x1B
#define DP83TC811_BICTSR2                                  0x1C
#define DP83TC811_TDR                                      0x1E
#define DP83TC811_PHYRCR                                   0x1F

//DP83TC811 MMD registers
#define DP83TC811_PMA_CTRL1                                0x01, 0x0007
#define DP83TC811_PMA_EXT1                                 0x01, 0x000B
#define DP83TC811_PMA_EXT2                                 0x01, 0x0012
#define DP83TC811_PMA_CTRL2                                0x01, 0x0834
#define DP83TC811_TEST_CTRL                                0x01, 0x0836
#define DP83TC811_LSR                                      0x1F, 0x0133
#define DP83TC811_TDRR                                     0x1F, 0x016B
#define DP83TC811_TDRLR1                                   0x1F, 0x0180
#define DP83TC811_TDRLR2                                   0x1F, 0x0181
#define DP83TC811_TDRPT                                    0x1F, 0x018A
#define DP83TC811_AUTO_PHY                                 0x1F, 0x018B
#define DP83TC811_PWRM                                     0x1F, 0x018C
#define DP83TC811_SNR                                      0x1F, 0x0197
#define DP83TC811_SQI                                      0x1F, 0x0198
#define DP83TC811_LD_CTRL                                  0x1F, 0x0400
#define DP83TC811_LDG_CTRL1                                0x1F, 0x0401
#define DP83TC811_DLL_CTRL                                 0x1F, 0x0446
#define DP83TC811_ESDS                                     0x1F, 0x0448
#define DP83TC811_LED_CFG1                                 0x1F, 0x0460
#define DP83TC811_XMII_IMP_CTRL                            0x1F, 0x0461
#define DP83TC811_IO_CTRL1                                 0x1F, 0x0462
#define DP83TC811_IO_CTRL2                                 0x1F, 0x0463
#define DP83TC811_STRAP                                    0x1F, 0x0467
#define DP83TC811_LED_CFG2                                 0x1F, 0x0469
#define DP83TC811_PLR_CFG                                  0x1F, 0x0475
#define DP83TC811_MON_CFG1                                 0x1F, 0x0480
#define DP83TC811_MON_CFG2                                 0x1F, 0x0481
#define DP83TC811_MON_CFG3                                 0x1F, 0x0482
#define DP83TC811_MON_STAT1                                0x1F, 0x0483
#define DP83TC811_MON_STAT2                                0x1F, 0x0484
#define DP83TC811_PCS_CTRL1                                0x1F, 0x0485
#define DP83TC811_PCS_CTRL2                                0x1F, 0x0486
#define DP83TC811_LPS_CTRL2                                0x1F, 0x0487
#define DP83TC811_INTER_CFG                                0x1F, 0x0489
#define DP83TC811_LPS_CTRL3                                0x1F, 0x0493
#define DP83TC811_LPS_CTRL3                                0x1F, 0x0493
#define DP83TC811_JAB_CFG                                  0x1F, 0x0496
#define DP83TC811_TEST_MODE_CTRL                           0x1F, 0x0497
#define DP83TC811_WOL_CFG                                  0x1F, 0x04A0
#define DP83TC811_WOL_STAT                                 0x1F, 0x04A1
#define DP83TC811_WOL_DA1                                  0x1F, 0x04A2
#define DP83TC811_WOL_DA2                                  0x1F, 0x04A3
#define DP83TC811_WOL_DA3                                  0x1F, 0x04A4
#define DP83TC811_RXSOP1                                   0x1F, 0x04A5
#define DP83TC811_RXSOP2                                   0x1F, 0x04A6
#define DP83TC811_RXSOP3                                   0x1F, 0x04A7
#define DP83TC811_RXPAT1                                   0x1F, 0x04A8
#define DP83TC811_RXPAT2                                   0x1F, 0x04A9
#define DP83TC811_RXPAT3                                   0x1F, 0x04AA
#define DP83TC811_RXPAT4                                   0x1F, 0x04AB
#define DP83TC811_RXPAT5                                   0x1F, 0x04AC
#define DP83TC811_RXPAT6                                   0x1F, 0x04AD
#define DP83TC811_RXPAT7                                   0x1F, 0x04AE
#define DP83TC811_RXPAT8                                   0x1F, 0x04AF
#define DP83TC811_RXPAT9                                   0x1F, 0x04B0
#define DP83TC811_RXPAT10                                  0x1F, 0x04B1
#define DP83TC811_RXPAT11                                  0x1F, 0x04B2
#define DP83TC811_RXPAT12                                  0x1F, 0x04B3
#define DP83TC811_RXPAT13                                  0x1F, 0x04B4
#define DP83TC811_RXPAT14                                  0x1F, 0x04B5
#define DP83TC811_RXPAT15                                  0x1F, 0x04B6
#define DP83TC811_RXPAT16                                  0x1F, 0x04B7
#define DP83TC811_RXPAT17                                  0x1F, 0x04B8
#define DP83TC811_RXPAT18                                  0x1F, 0x04B9
#define DP83TC811_RXPAT19                                  0x1F, 0x04BA
#define DP83TC811_RXPAT20                                  0x1F, 0x04BB
#define DP83TC811_RXPAT21                                  0x1F, 0x04BC
#define DP83TC811_RXPAT22                                  0x1F, 0x04BD
#define DP83TC811_RXPAT23                                  0x1F, 0x04BE
#define DP83TC811_RXPAT24                                  0x1F, 0x04BF
#define DP83TC811_RXPAT25                                  0x1F, 0x04C0
#define DP83TC811_RXPAT26                                  0x1F, 0x04C1
#define DP83TC811_RXPAT27                                  0x1F, 0x04C2
#define DP83TC811_RXPAT28                                  0x1F, 0x04C3
#define DP83TC811_RXPAT29                                  0x1F, 0x04C4
#define DP83TC811_RXPAT30                                  0x1F, 0x04C5
#define DP83TC811_RXPAT31                                  0x1F, 0x04C6
#define DP83TC811_RXPAT32                                  0x1F, 0x04C7
#define DP83TC811_RXPBM1                                   0x1F, 0x04C8
#define DP83TC811_RXPBM2                                   0x1F, 0x04C9
#define DP83TC811_RXPBM3                                   0x1F, 0x04CA
#define DP83TC811_RXPBM4                                   0x1F, 0x04CB
#define DP83TC811_RXPATC                                   0x1F, 0x04CC
#define DP83TC811_RXD3CLK                                  0x1F, 0x04E0
#define DP83TC811_LPS_CFG                                  0x1F, 0x04E5

//Basic Mode Control register
#define DP83TC811_BMCR_RESET                               0x8000
#define DP83TC811_BMCR_LOOPBACK                            0x4000
#define DP83TC811_BMCR_SPEED_SEL                           0x2000
#define DP83TC811_BMCR_AN_EN                               0x1000
#define DP83TC811_BMCR_POWER_DOWN                          0x0800
#define DP83TC811_BMCR_ISOLATE                             0x0400

//Basic Mode Status register
#define DP83TC811_BMSR_100BT4                              0x8000
#define DP83TC811_BMSR_100BTX_FD                           0x4000
#define DP83TC811_BMSR_100BTX_HD                           0x2000
#define DP83TC811_BMSR_10BT_FD                             0x1000
#define DP83TC811_BMSR_10BT_HD                             0x0800
#define DP83TC811_BMSR_SMI_PREAMBLE_SUPPR                  0x0040
#define DP83TC811_BMSR_AN_COMPLETE                         0x0020
#define DP83TC811_BMSR_REMOTE_FAULT                        0x0010
#define DP83TC811_BMSR_AN_CAPABLE                          0x0008
#define DP83TC811_BMSR_LINK_STATUS                         0x0004
#define DP83TC811_BMSR_JABBER_DETECT                       0x0002
#define DP83TC811_BMSR_EXTENDED_CAPABLE                    0x0001

//PHY Identifier 1 register
#define DP83TC811_PHYID1_OUI_MSB                           0xFFFF
#define DP83TC811_PHYID1_OUI_MSB_DEFAULT                   0x2000

//PHY Identifier 2 register
#define DP83TC811_PHYID2_OUI_LSB                           0xFC00
#define DP83TC811_PHYID2_OUI_LSB_DEFAULT                   0xA000
#define DP83TC811_PHYID2_MODEL_NUM                         0x03F0
#define DP83TC811_PHYID2_MODEL_NUM_DEFAULT                 0x0250
#define DP83TC811_PHYID2_REVISION_NUM                      0x000F

//TDR Auto-Run register
#define DP83TC811_TDR_AUTO_TDR_AUTO_RUN                    0x0100

//Register Control register
#define DP83TC811_REGCR_COMMAND                            0xC000
#define DP83TC811_REGCR_COMMAND_ADDR                       0x0000
#define DP83TC811_REGCR_COMMAND_DATA_NO_POST_INC           0x4000
#define DP83TC811_REGCR_COMMAND_DATA_POST_INC_RW           0x8000
#define DP83TC811_REGCR_COMMAND_DATA_POST_INC_W            0xC000
#define DP83TC811_REGCR_DEVAD                              0x001F

//Interrupt Test register
#define DP83TC811_INT_TEST_INTERRUPT_POLARITY              0x0008
#define DP83TC811_INT_TEST_TEST_INTERRUPT                  0x0004

//Interrupt Status 1 register
#define DP83TC811_INT_STAT1_LINK_QUALITY_IF                0x8000
#define DP83TC811_INT_STAT1_ENERGY_DETECT_IF               0x4000
#define DP83TC811_INT_STAT1_LINK_STATUS_CHANGED_IF         0x2000
#define DP83TC811_INT_STAT1_WOL_IF                         0x1000
#define DP83TC811_INT_STAT1_ESD_EVENT_IF                   0x0800
#define DP83TC811_INT_STAT1_MS_TRAINING_COMPLETE_IF        0x0400
#define DP83TC811_INT_STAT1_FALSE_CARRIER_CNT_HF_IF        0x0200
#define DP83TC811_INT_STAT1_RECEIVE_ERROR_CNT_HF_IF        0x0100
#define DP83TC811_INT_STAT1_LINK_QUALITY_IE                0x0080
#define DP83TC811_INT_STAT1_ENERGY_DETECT_IE               0x0040
#define DP83TC811_INT_STAT1_LINK_STATUS_CHANGED_IE         0x0020
#define DP83TC811_INT_STAT1_WOL_IE                         0x0010
#define DP83TC811_INT_STAT1_ESD_EVENT_IE                   0x0008
#define DP83TC811_INT_STAT1_MS_TRAINING_COMPLETE_IE        0x0004
#define DP83TC811_INT_STAT1_FALSE_CARRIER_CNT_HF_IE        0x0002
#define DP83TC811_INT_STAT1_RECEIVE_ERROR_CNT_HF_IE        0x0001

//Interrupt Status 2 register
#define DP83TC811_INT_STAT2_UNDERVOLTAGE_IF                0x8000
#define DP83TC811_INT_STAT2_OVERVOLTAGE_IF                 0x4000
#define DP83TC811_INT_STAT2_OVERTEMPERATURE_IF             0x0800
#define DP83TC811_INT_STAT2_SLEEP_MODE_IF                  0x0400
#define DP83TC811_INT_STAT2_POLARITY_CHANGE_IF             0x0200
#define DP83TC811_INT_STAT2_JABBER_DETECT_IF               0x0100
#define DP83TC811_INT_STAT2_UNDERVOLTAGE_IE                0x0080
#define DP83TC811_INT_STAT2_OVERVOLTAGE_IE                 0x0040
#define DP83TC811_INT_STAT2_OVERTEMPERATURE_IE             0x0008
#define DP83TC811_INT_STAT2_SLEEP_MODE_IE                  0x0004
#define DP83TC811_INT_STAT2_POLARITY_CHANGE_IE             0x0002
#define DP83TC811_INT_STAT2_JABBER_DETECT_IE               0x0001

//False Carrier Sense Counter register
#define DP83TC811_FCSCR_FALSE_CARRIER_EVENT_CNT            0x00FF

//BIST Control register
#define DP83TC811_BISTCR_BIST_ERROR_COUNTER_MODE           0x4000
#define DP83TC811_BISTCR_PRBS_PACKET_TYPE                  0x2000
#define DP83TC811_BISTCR_PACKET_GENERATION_EN              0x1000
#define DP83TC811_BISTCR_PRBS_CHECKER_LOCK_SYNC            0x0800
#define DP83TC811_BISTCR_PRBS_CHECKER_SYNC_LOSS            0x0400
#define DP83TC811_BISTCR_PACKET_GENERATOR_STATUS           0x0200
#define DP83TC811_BISTCR_LOOPBACK_SELECT                   0x007C

//xMII Control register
#define DP83TC811_XMII_CTRL_RGMII_RX_CLOCK_DELAY           0x1000
#define DP83TC811_XMII_CTRL_RGMII_TX_CLOCK_DELAY           0x0800
#define DP83TC811_XMII_CTRL_RGMII_MODE                     0x0200
#define DP83TC811_XMII_CTRL_CLOCK_SELECT                   0x0080
#define DP83TC811_XMII_CTRL_RMII_MODE                      0x0020
#define DP83TC811_XMII_CTRL_RMII_REV_SELECT                0x0010
#define DP83TC811_XMII_CTRL_RMII_OVERFLOW_STATUS           0x0008
#define DP83TC811_XMII_CTRL_RMII_UNDERFLOW_STATUS          0x0004
#define DP83TC811_XMII_CTRL_RMII_RX_ELASTICITY_BUFFER_SIZE 0x0002

//Interrupt Status 3 register
#define DP83TC811_INT_STAT3_POR_DONE_IF                    0x1000
#define DP83TC811_INT_STAT3_NO_FRAME_DETECTED_IF           0x0800
#define DP83TC811_INT_STAT3_LPS_IF                         0x0100
#define DP83TC811_INT_STAT3_POR_DONE_IE                    0x0010
#define DP83TC811_INT_STAT3_NO_FRAME_DETECTED_IE           0x0008
#define DP83TC811_INT_STAT3_LPS_IE                         0x0001

//BIST Control and Status 1 register
#define DP83TC811_BICTSR1_BIST_ERROR_CNT                   0xFF00
#define DP83TC811_BICTSR1_BIST_IPG_LENGTH                  0x00FF

//BIST Control and Status 2 register
#define DP83TC811_BICTSR2_BIST_PACKET_LENGTH               0x07FF

//Time Domain Reflectometry register
#define DP83TC811_TDR_START                                0x8000
#define DP83TC811_TDR_STATUS                               0x0002
#define DP83TC811_TDR_TEST_FAIL                            0x0001

//PHY Reset Control register
#define DP83TC811_PHYRCR_HARDWARE_RESET                    0x8000
#define DP83TC811_PHYRCR_SOFTWARE_RESET                    0x4000
#define DP83TC811_PHYRCR_STANDBY_MODE                      0x0080

//Link Status Results register
#define DP83TC811_LSR_LINK_STATUS                          0x1000
#define DP83TC811_LSR_SCRAMBLER_LOCK                       0x0004
#define DP83TC811_LSR_LOCAL_RECEIVER_STATUS                0x0002
#define DP83TC811_LSR_REMOTE_RECEIVER_STATUS               0x0001

//TDR Results register
#define DP83TC811_TDRR_FAULT_STATUS                        0x0200
#define DP83TC811_TDRR_FAULT_TYPE                          0x0100
#define DP83TC811_TDRR_FAULT_LOCATION                      0x00FF

//TDR Location Result 1 register
#define DP83TC811_TDRLR1_LOCATION_2                        0xFF00
#define DP83TC811_TDRLR1_LOCATION_1                        0x00FF

//TDR Location Result 2 register
#define DP83TC811_TDRLR2_LOCATION_4                        0xFF00
#define DP83TC811_TDRLR2_LOCATION_3                        0x00FF

//TDR Peak Type register
#define DP83TC811_TDRPT_PEAK_4_TYPE                        0x4000
#define DP83TC811_TDRPT_PEAK_3_TYPE                        0x2000
#define DP83TC811_TDRPT_PEAK_2_TYPE                        0x1000
#define DP83TC811_TDRPT_PEAK_1_TYPE                        0x0800

//Autonomous PHY Control register
#define DP83TC811_AUTO_PHY_AUTONOMOUS_COMMAND              0x0040
#define DP83TC811_AUTO_PHY_SLEEP_EN                        0x0002
#define DP83TC811_AUTO_PHY_LPS_TRANSMISSION_EN             0x0001

//Power Mode Register register
#define DP83TC811_PWRM_SLEEP_REQUEST_COMMAND               0x0002
#define DP83TC811_PWRM_NORMAL_COMMAND                      0x0001

//Signal-to-Noise Ratio Result register
#define DP83TC811_SNR_SNR                                  0x01FF

//Signal Quality Indication register
#define DP83TC811_SQI_SQS                                  0x0300
#define DP83TC811_SQI_SQI                                  0x00FF

//Line Driver Control register
#define DP83TC811_LD_CTRL_LINE_DRIVER_SERIES_TERM          0x1F00

//Line Driver Gain Control 1 register
#define DP83TC811_LDG_CTRL1_FINE_GAIN_CONTROL_MDI          0x000F

//RGMII DLL Control register
#define DP83TC811_DLL_CTRL_DLL_TX_DELAY_CONTROL            0x00F0
#define DP83TC811_DLL_CTRL_DLL_RX_DELAY_CONTROL            0x000F

//Electrostatic Discharge Status register
#define DP83TC811_ESDS_XMII_ESD_EVENT_CNT                  0x3F00
#define DP83TC811_ESDS_MDI_ESD_EVENT_CNT                   0x003F

//LED Configuration 1 register
#define DP83TC811_LED_CFG1_LED_BLINK_RATE_CTRL             0x3000
#define DP83TC811_LED_CFG1_LED_BLINK_RATE_CTRL_20HZ        0x0000
#define DP83TC811_LED_CFG1_LED_BLINK_RATE_CTRL_10HZ        0x1000
#define DP83TC811_LED_CFG1_LED_BLINK_RATE_CTRL_5HZ         0x2000
#define DP83TC811_LED_CFG1_LED_BLINK_RATE_CTRL_2HZ         0x3000
#define DP83TC811_LED_CFG1_LED_2_CTRL                      0x0F00
#define DP83TC811_LED_CFG1_LED_2_CTRL_LINK_OK              0x0000
#define DP83TC811_LED_CFG1_LED_2_CTRL_LINK_OK_TX_RX_ACT    0x0100
#define DP83TC811_LED_CFG1_LED_2_CTRL_LINK_OK_TX_ACT       0x0200
#define DP83TC811_LED_CFG1_LED_2_CTRL_LINK_OK_RX_ACT       0x0300
#define DP83TC811_LED_CFG1_LED_2_CTRL_LINK_OK_MASTER       0x0400
#define DP83TC811_LED_CFG1_LED_2_CTRL_LINK_OK_SLAVE        0x0500
#define DP83TC811_LED_CFG1_LED_2_CTRL_TX_RX_ACT            0x0600
#define DP83TC811_LED_CFG1_LED_2_CTRL_LINK_LOST            0x0900
#define DP83TC811_LED_CFG1_LED_2_CTRL_PRBS_ERR             0x0A00
#define DP83TC811_LED_CFG1_LED_1_CTRL                      0x00F0
#define DP83TC811_LED_CFG1_LED_1_CTRL_LINK_OK              0x0000
#define DP83TC811_LED_CFG1_LED_1_CTRL_LINK_OK_TX_RX_ACT    0x0010
#define DP83TC811_LED_CFG1_LED_1_CTRL_LINK_OK_TX_ACT       0x0020
#define DP83TC811_LED_CFG1_LED_1_CTRL_LINK_OK_RX_ACT       0x0030
#define DP83TC811_LED_CFG1_LED_1_CTRL_LINK_OK_MASTER       0x0040
#define DP83TC811_LED_CFG1_LED_1_CTRL_LINK_OK_SLAVE        0x0050
#define DP83TC811_LED_CFG1_LED_1_CTRL_TX_RX_ACT            0x0060
#define DP83TC811_LED_CFG1_LED_1_CTRL_LINK_LOST            0x0090
#define DP83TC811_LED_CFG1_LED_1_CTRL_PRBS_ERR             0x00A0
#define DP83TC811_LED_CFG1_LED_0_CTRL                      0x000F
#define DP83TC811_LED_CFG1_LED_0_CTRL_LINK_OK              0x0000
#define DP83TC811_LED_CFG1_LED_0_CTRL_LINK_OK_TX_RX_ACT    0x0001
#define DP83TC811_LED_CFG1_LED_0_CTRL_LINK_OK_TX_ACT       0x0002
#define DP83TC811_LED_CFG1_LED_0_CTRL_LINK_OK_RX_ACT       0x0003
#define DP83TC811_LED_CFG1_LED_0_CTRL_LINK_OK_MASTER       0x0004
#define DP83TC811_LED_CFG1_LED_0_CTRL_LINK_OK_SLAVE        0x0005
#define DP83TC811_LED_CFG1_LED_0_CTRL_TX_RX_ACT            0x0006
#define DP83TC811_LED_CFG1_LED_0_CTRL_LINK_LOST            0x0009
#define DP83TC811_LED_CFG1_LED_0_CTRL_PRBS_ERR             0x000A

//xMII Impedance Control register
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL                   0x001E
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_99R               0x0000
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_91R               0x0002
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_84R               0x0004
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_78R               0x0006
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_73R               0x0008
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_69R               0x000A
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_65R               0x000C
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_61R               0x000E
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_58R               0x0010
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_55R               0x0012
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_53R               0x0014
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_50R               0x0016
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_48R               0x0018
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_46R               0x001A
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_44R               0x001C
#define DP83TC811_XMII_IMP_CTRL_IMP_CTRL_42R               0x001E

//GPIO Control 1 register
#define DP83TC811_IO_CTRL1_LED_1_CLOCK_SEL                 0x7000
#define DP83TC811_IO_CTRL1_LED_1_CLOCK_SEL_XI              0x0000
#define DP83TC811_IO_CTRL1_LED_1_CLOCK_SEL_TX_TCLK         0x1000
#define DP83TC811_IO_CTRL1_LED_1_GPIO_SEL                  0x0700
#define DP83TC811_IO_CTRL1_LED_1_GPIO_SEL_LED_1            0x0000
#define DP83TC811_IO_CTRL1_LED_1_GPIO_SEL_CLOCK            0x0100
#define DP83TC811_IO_CTRL1_LED_1_GPIO_SEL_WOL              0x0200
#define DP83TC811_IO_CTRL1_LED_1_GPIO_SEL_UNDERVOLTAGE     0x0300
#define DP83TC811_IO_CTRL1_LED_1_GPIO_SEL_TRANSMIT         0x0400
#define DP83TC811_IO_CTRL1_LED_1_GPIO_SEL_RECEIVE          0x0500
#define DP83TC811_IO_CTRL1_LED_1_GPIO_SEL_CONST_LOW        0x0600
#define DP83TC811_IO_CTRL1_LED_1_GPIO_SEL_CONST_HIGH       0x0700
#define DP83TC811_IO_CTRL1_LED_0_CLOCK_SEL                 0x0070
#define DP83TC811_IO_CTRL1_LED_0_CLOCK_SEL_XI              0x0000
#define DP83TC811_IO_CTRL1_LED_0_CLOCK_SEL_TX_TCLK         0x0010
#define DP83TC811_IO_CTRL1_LED_0_GPIO_SEL                  0x0007
#define DP83TC811_IO_CTRL1_LED_0_GPIO_SEL_LED_0            0x0000
#define DP83TC811_IO_CTRL1_LED_0_GPIO_SEL_CLOCK            0x0001
#define DP83TC811_IO_CTRL1_LED_0_GPIO_SEL_WOL              0x0002
#define DP83TC811_IO_CTRL1_LED_0_GPIO_SEL_UNDERVOLTAGE     0x0003
#define DP83TC811_IO_CTRL1_LED_0_GPIO_SEL_TRANSMIT         0x0004
#define DP83TC811_IO_CTRL1_LED_0_GPIO_SEL_RECEIVE          0x0005
#define DP83TC811_IO_CTRL1_LED_0_GPIO_SEL_CONST_LOW        0x0006
#define DP83TC811_IO_CTRL1_LED_0_GPIO_SEL_CONST_HIGH       0x0007

//GPIO Control 2 register
#define DP83TC811_IO_CTRL2_CLKOUT_CLOCK_SEL                0x00F0
#define DP83TC811_IO_CTRL2_CLKOUT_CLOCK_SEL_XI             0x0000
#define DP83TC811_IO_CTRL2_CLKOUT_CLOCK_SEL_TX_TCLK        0x0010
#define DP83TC811_IO_CTRL2_CLKOUT_GPIO_SEL                 0x0007
#define DP83TC811_IO_CTRL2_CLKOUT_GPIO_SEL_LED_2           0x0000
#define DP83TC811_IO_CTRL2_CLKOUT_GPIO_SEL_CLOCK           0x0001
#define DP83TC811_IO_CTRL2_CLKOUT_GPIO_SEL_WOL             0x0002
#define DP83TC811_IO_CTRL2_CLKOUT_GPIO_SEL_UNDERVOLTAGE    0x0003
#define DP83TC811_IO_CTRL2_CLKOUT_GPIO_SEL_TRANSMIT        0x0004
#define DP83TC811_IO_CTRL2_CLKOUT_GPIO_SEL_RECEIVE         0x0005
#define DP83TC811_IO_CTRL2_CLKOUT_GPIO_SEL_CONST_LOW       0x0006
#define DP83TC811_IO_CTRL2_CLKOUT_GPIO_SEL_CONST_HIGH      0x0007

//Strap Configuration register
#define DP83TC811_STRAP_LED_1                              0xC000
#define DP83TC811_STRAP_LED_1_MODE1                        0x0000
#define DP83TC811_STRAP_LED_1_MODE2                        0x4000
#define DP83TC811_STRAP_LED_1_MODE3                        0x8000
#define DP83TC811_STRAP_LED_1_MODE4                        0xC000
#define DP83TC811_STRAP_RX_DV                              0x3000
#define DP83TC811_STRAP_RX_DV_MODE1                        0x0000
#define DP83TC811_STRAP_RX_DV_MODE2                        0x1000
#define DP83TC811_STRAP_RX_DV_MODE3                        0x2000
#define DP83TC811_STRAP_RX_DV_MODE4                        0x3000
#define DP83TC811_STRAP_RX_ER                              0x0C00
#define DP83TC811_STRAP_RX_ER_MODE1                        0x0000
#define DP83TC811_STRAP_RX_ER_MODE2                        0x0400
#define DP83TC811_STRAP_RX_ER_MODE3                        0x0800
#define DP83TC811_STRAP_RX_ER_MODE4                        0x0C00
#define DP83TC811_STRAP_LED_0                              0x0300
#define DP83TC811_STRAP_LED_0_MODE1                        0x0000
#define DP83TC811_STRAP_LED_0_MODE2                        0x0100
#define DP83TC811_STRAP_LED_0_MODE3                        0x0200
#define DP83TC811_STRAP_LED_0_MODE4                        0x0300
#define DP83TC811_STRAP_RX_D0                              0x00C0
#define DP83TC811_STRAP_RX_D0_MODE1                        0x0000
#define DP83TC811_STRAP_RX_D0_MODE2                        0x0040
#define DP83TC811_STRAP_RX_D0_MODE3                        0x0080
#define DP83TC811_STRAP_RX_D0_MODE4                        0x00C0
#define DP83TC811_STRAP_RX_D1                              0x0030
#define DP83TC811_STRAP_RX_D1_MODE1                        0x0000
#define DP83TC811_STRAP_RX_D1_MODE2                        0x0010
#define DP83TC811_STRAP_RX_D1_MODE3                        0x0020
#define DP83TC811_STRAP_RX_D1_MODE4                        0x0030
#define DP83TC811_STRAP_RX_D2                              0x000C
#define DP83TC811_STRAP_RX_D2_MODE1                        0x0000
#define DP83TC811_STRAP_RX_D2_MODE2                        0x0004
#define DP83TC811_STRAP_RX_D2_MODE3                        0x0008
#define DP83TC811_STRAP_RX_D2_MODE4                        0x000C
#define DP83TC811_STRAP_RX_D3                              0x0003
#define DP83TC811_STRAP_RX_D3_MODE1                        0x0000
#define DP83TC811_STRAP_RX_D3_MODE2                        0x0001
#define DP83TC811_STRAP_RX_D3_MODE3                        0x0002
#define DP83TC811_STRAP_RX_D3_MODE4                        0x0003

//LED Configuration 2 register
#define DP83TC811_LED_CFG2_LED_2_POL                       0x0400
#define DP83TC811_LED_CFG2_LED_2_OVERRIDE_VAL              0x0200
#define DP83TC811_LED_CFG2_LED_2_OVERRIDE_EN               0x0100
#define DP83TC811_LED_CFG2_LED_1_POL                       0x0040
#define DP83TC811_LED_CFG2_LED_1_OVERRIDE_VAL              0x0020
#define DP83TC811_LED_CFG2_LED_1_OVERRIDE_EN               0x0010
#define DP83TC811_LED_CFG2_LED_0_POL                       0x0004
#define DP83TC811_LED_CFG2_LED_0_OVERRIDE_VAL              0x0002
#define DP83TC811_LED_CFG2_LED_0_OVERRIDE_EN               0x0001

//Polarity Auto-Correction Configuration register
#define DP83TC811_PLR_CFG_POL_VALUE                        0x2000
#define DP83TC811_PLR_CFG_FORCE_POL_EN                     0x1000

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//DP83TC811 Ethernet PHY driver
extern const PhyDriver dp83tc811PhyDriver;

//DP83TC811 related functions
error_t dp83tc811Init(NetInterface *interface);

void dp83tc811Tick(NetInterface *interface);

void dp83tc811EnableIrq(NetInterface *interface);
void dp83tc811DisableIrq(NetInterface *interface);

void dp83tc811EventHandler(NetInterface *interface);

void dp83tc811WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t dp83tc811ReadPhyReg(NetInterface *interface, uint8_t address);

void dp83tc811DumpPhyReg(NetInterface *interface);

void dp83tc811WriteMmdReg(NetInterface *interface, uint8_t devAddr,
   uint16_t regAddr, uint16_t data);

uint16_t dp83tc811ReadMmdReg(NetInterface *interface, uint8_t devAddr,
   uint16_t regAddr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
