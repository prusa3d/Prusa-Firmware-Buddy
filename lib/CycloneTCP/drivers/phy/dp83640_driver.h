/**
 * @file dp83640_driver.h
 * @brief DP83640 Ethernet PHY driver
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

#ifndef _DP83640_DRIVER_H
#define _DP83640_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef DP83640_PHY_ADDR
   #define DP83640_PHY_ADDR 1
#elif (DP83640_PHY_ADDR < 0 || DP83640_PHY_ADDR > 31)
   #error DP83640_PHY_ADDR parameter is not valid
#endif

//DP83640 PHY registers (page 0)
#define DP83640_BMCR                             0x00
#define DP83640_BMSR                             0x01
#define DP83640_PHYIDR1                          0x02
#define DP83640_PHYIDR2                          0x03
#define DP83640_ANAR                             0x04
#define DP83640_ANLPAR                           0x05
#define DP83640_ANER                             0x06
#define DP83640_ANNPTR                           0x07
#define DP83640_PHYSTS                           0x10
#define DP83640_MICR                             0x11
#define DP83640_MISR                             0x12
#define DP83640_PAGSR                            0x13
#define DP83640_FCSCR                            0x14
#define DP83640_RECR                             0x15
#define DP83640_PCSR                             0x16
#define DP83640_RBR                              0x17
#define DP83640_LEDCR                            0x18
#define DP83640_PHYCR                            0x19
#define DP83640_10BTSCR                          0x1A
#define DP83640_CDCTRL1                          0x1B
#define DP83640_PHYCR2                           0x1C
#define DP83640_EDCR                             0x1D
#define DP83640_PCFCR                            0x1F

//DP83640 PHY registers (page 1)
#define DP83640_SD_CNFG                          0x1E

//DP83640 PHY registers (page 2)
#define DP83640_LEN100_DET                       0x14
#define DP83640_FREQ100                          0x15
#define DP83640_TDR_CTRL                         0x16
#define DP83640_TDR_WIN                          0x17
#define DP83640_TDR_PEAK                         0x18
#define DP83640_TDR_THR                          0x19
#define DP83640_VAR_CTRL                         0x1A
#define DP83640_VAR_DAT                          0x1B
#define DP83640_LQMR                             0x1D
#define DP83640_LQDR                             0x1E
#define DP83640_LQMR2                            0x1F

//DP83640 PHY registers (page 4)
#define DP83640_PTP_CTL                          0x14
#define DP83640_PTP_TDR                          0x15
#define DP83640_PTP_STS                          0x16
#define DP83640_PTP_TSTS                         0x17
#define DP83640_PTP_RATEL                        0x18
#define DP83640_PTP_RATEH                        0x19
#define DP83640_PTP_RDCKSUM                      0x1A
#define DP83640_PTP_WRCKSUM                      0x1B
#define DP83640_PTP_TXTS                         0x1C
#define DP83640_PTP_RXTS                         0x1D
#define DP83640_PTP_ESTS                         0x1E
#define DP83640_PTP_EDATA                        0x1F

//DP83640 PHY registers (page 5)
#define DP83640_PTP_TRIG                         0x14
#define DP83640_PTP_EVNT                         0x15
#define DP83640_PTP_TXCFG0                       0x16
#define DP83640_PTP_TXCFG1                       0x17
#define DP83640_PSF_CFG0                         0x18
#define DP83640_PTP_RXCFG0                       0x19
#define DP83640_PTP_RXCFG1                       0x1A
#define DP83640_PTP_RXCFG2                       0x1B
#define DP83640_PTP_RXCFG3                       0x1C
#define DP83640_PTP_RXCFG4                       0x1D
#define DP83640_PTP_TRDL                         0x1E
#define DP83640_PTP_TRDH                         0x1F

//DP83640 PHY registers (page 6)
#define DP83640_PTP_COC                          0x14
#define DP83640_PSF_CFG1                         0x15
#define DP83640_PSF_CFG2                         0x16
#define DP83640_PSF_CFG3                         0x17
#define DP83640_PSF_CFG4                         0x18
#define DP83640_PTP_SFDCFG                       0x19
#define DP83640_PTP_INTCTL                       0x1A
#define DP83640_PTP_CLKSRC                       0x1B
#define DP83640_PTP_ETR                          0x1C
#define DP83640_PTP_OFF                          0x1D
#define DP83640_PTP_GPIOMON                      0x1E
#define DP83640_PTP_RXHASH                       0x1F

//Basic Mode Control register
#define DP83640_BMCR_RESET                       0x8000
#define DP83640_BMCR_LOOPBACK                    0x4000
#define DP83640_BMCR_SPEED_SEL                   0x2000
#define DP83640_BMCR_AN_EN                       0x1000
#define DP83640_BMCR_POWER_DOWN                  0x0800
#define DP83640_BMCR_ISOLATE                     0x0400
#define DP83640_BMCR_RESTART_AN                  0x0200
#define DP83640_BMCR_DUPLEX_MODE                 0x0100
#define DP83640_BMCR_COL_TEST                    0x0080
#define DP83640_BMCR_UNIDIRECTIONAL_EN           0x0020

//Basic Mode Status register
#define DP83640_BMSR_100BT4                      0x8000
#define DP83640_BMSR_100BTX_FD                   0x4000
#define DP83640_BMSR_100BTX_HD                   0x2000
#define DP83640_BMSR_10BT_FD                     0x1000
#define DP83640_BMSR_10BT_HD                     0x0800
#define DP83640_BMSR_UNIDIRECTIONAL_ABLE         0x0080
#define DP83640_BMSR_MF_PREAMBLE_SUPPR           0x0040
#define DP83640_BMSR_AN_COMPLETE                 0x0020
#define DP83640_BMSR_REMOTE_FAULT                0x0010
#define DP83640_BMSR_AN_CAPABLE                  0x0008
#define DP83640_BMSR_LINK_STATUS                 0x0004
#define DP83640_BMSR_JABBER_DETECT               0x0002
#define DP83640_BMSR_EXTENDED_CAPABLE            0x0001

//PHY Identifier 1 register
#define DP83640_PHYIDR1_OUI_MSB                  0xFFFF
#define DP83640_PHYIDR1_OUI_MSB_DEFAULT          0x2000

//PHY Identifier 2 register
#define DP83640_PHYIDR2_OUI_LSB                  0xFC00
#define DP83640_PHYIDR2_OUI_LSB_DEFAULT          0x5C00
#define DP83640_PHYIDR2_VNDR_MDL                 0x03F0
#define DP83640_PHYIDR2_VNDR_MDL_DEFAULT         0x00E0
#define DP83640_PHYIDR2_MDL_REV                  0x000F

//Auto-Negotiation Advertisement register
#define DP83640_ANAR_NEXT_PAGE                   0x8000
#define DP83640_ANAR_REMOTE_FAULT                0x2000
#define DP83640_ANAR_ASM_DIR                     0x0800
#define DP83640_ANAR_PAUSE                       0x0400
#define DP83640_ANAR_100BT4                      0x0200
#define DP83640_ANAR_100BTX_FD                   0x0100
#define DP83640_ANAR_100BTX_HD                   0x0080
#define DP83640_ANAR_10BT_FD                     0x0040
#define DP83640_ANAR_10BT_HD                     0x0020
#define DP83640_ANAR_SELECTOR                    0x001F
#define DP83640_ANAR_SELECTOR_DEFAULT            0x0001

//Auto-Negotiation Link Partner Ability register
#define DP83640_ANLPAR_NEXT_PAGE                 0x8000
#define DP83640_ANLPAR_ACK                       0x4000
#define DP83640_ANLPAR_REMOTE_FAULT              0x2000
#define DP83640_ANLPAR_ASM_DIR                   0x0800
#define DP83640_ANLPAR_PAUSE                     0x0400
#define DP83640_ANLPAR_100BT4                    0x0200
#define DP83640_ANLPAR_100BTX_FD                 0x0100
#define DP83640_ANLPAR_100BTX_HD                 0x0080
#define DP83640_ANLPAR_10BT_FD                   0x0040
#define DP83640_ANLPAR_10BT_HD                   0x0020
#define DP83640_ANLPAR_SELECTOR                  0x001F
#define DP83640_ANLPAR_SELECTOR_DEFAULT          0x0001

//Auto-Negotiation Expansion register
#define DP83640_ANER_PAR_DETECT_FAULT            0x0010
#define DP83640_ANER_LP_NP_ABLE                  0x0008
#define DP83640_ANER_NP_ABLE                     0x0004
#define DP83640_ANER_PAGE_RX                     0x0002
#define DP83640_ANER_LP_AN_ABLE                  0x0001

//Auto-Negotiation Next Page TX register
#define DP83640_ANNPTR_NEXT_PAGE                 0x8000
#define DP83640_ANNPTR_MSG_PAGE                  0x2000
#define DP83640_ANNPTR_ACK2                      0x1000
#define DP83640_ANNPTR_TOGGLE                    0x0800
#define DP83640_ANNPTR_CODE                      0x07FF

//PHY Status register
#define DP83640_PHYSTS_MDIX_MODE                 0x4000
#define DP83640_PHYSTS_RECEIVE_ERROR_LATCH       0x2000
#define DP83640_PHYSTS_POLARITY_STATUS           0x1000
#define DP83640_PHYSTS_FALSE_CARRIER_SENSE_LATCH 0x0800
#define DP83640_PHYSTS_SIGNAL_DETECT             0x0400
#define DP83640_PHYSTS_DESCRAMBLER_LOCK          0x0200
#define DP83640_PHYSTS_PAGE_RECEIVED             0x0100
#define DP83640_PHYSTS_MII_INTERRUPT             0x0080
#define DP83640_PHYSTS_REMOTE_FAULT              0x0040
#define DP83640_PHYSTS_JABBER_DETECT             0x0020
#define DP83640_PHYSTS_AN_COMPLETE               0x0010
#define DP83640_PHYSTS_LOOPBACK_STATUS           0x0008
#define DP83640_PHYSTS_DUPLEX_STATUS             0x0004
#define DP83640_PHYSTS_SPEED_STATUS              0x0002
#define DP83640_PHYSTS_LINK_STATUS               0x0001

//MII Interrupt Control register
#define DP83640_MICR_PTP_INT_SEL                 0x0008
#define DP83640_MICR_TINT                        0x0004
#define DP83640_MICR_INTEN                       0x0002
#define DP83640_MICR_INT_OE                      0x0001

//MII Interrupt Status register
#define DP83640_MISR_LQ_INT                      0x8000
#define DP83640_MISR_ED_INT                      0x4000
#define DP83640_MISR_LINK_INT                    0x2000
#define DP83640_MISR_SPD_INT                     0x1000
#define DP83640_MISR_DUP_INT                     0x0800
#define DP83640_MISR_ANC_INT                     0x0400
#define DP83640_MISR_FHF_INT                     0x0200
#define DP83640_MISR_RHF_INT                     0x0100
#define DP83640_MISR_LQ_INT_EN                   0x0080
#define DP83640_MISR_ED_INT_EN                   0x0040
#define DP83640_MISR_LINK_INT_EN                 0x0020
#define DP83640_MISR_SPD_INT_EN                  0x0010
#define DP83640_MISR_DUP_INT_EN                  0x0008
#define DP83640_MISR_ANC_INT_EN                  0x0004
#define DP83640_MISR_FHF_INT_EN                  0x0002
#define DP83640_MISR_RHF_INT_EN                  0x0001

//Page Select register
#define DP83640_PAGSR_PAGE_SEL                   0x0007

//False Carrier Sense Counter register
#define DP83640_FCSCR_FCSCNT                     0x00FF

//Receive Error Counter register
#define DP83640_RECR_RXERCNT                     0x00FF

//PCS Configuration and Status register
#define DP83640_PCSR_FREE_CLK                    0x0800
#define DP83640_PCSR_TQ_EN                       0x0400
#define DP83640_PCSR_SD_FORCE_PMA                0x0200
#define DP83640_PCSR_SD_OPTION                   0x0100
#define DP83640_PCSR_DESC_TIME                   0x0080
#define DP83640_PCSR_FX_EN                       0x0040
#define DP83640_PCSR_FORCE_100_OK                0x0020
#define DP83640_PCSR_FEFI_EN                     0x0008
#define DP83640_PCSR_NRZI_BYPASS                 0x0004
#define DP83640_PCSR_SCRAM_BYPASS                0x0002
#define DP83640_PCSR_DESCRAM_BYPASS              0x0001

//RMII and Bypass register
#define DP83640_RBR_RMII_MASTER                  0x4000
#define DP83640_RBR_DIS_TX_OPT                   0x2000
#define DP83640_RBR_PMD_LOOP                     0x0100
#define DP83640_RBR_SCMII_RX                     0x0080
#define DP83640_RBR_SCMII_TX                     0x0040
#define DP83640_RBR_RMII_MODE                    0x0020
#define DP83640_RBR_RMII_REV1_0                  0x0010
#define DP83640_RBR_RX_OVF_STS                   0x0008
#define DP83640_RBR_RX_UNF_STS                   0x0004
#define DP83640_RBR_ELAST_BUF                    0x0003

//LED Direct Control register
#define DP83640_LEDCR_DIS_SPDLED                 0x0800
#define DP83640_LEDCR_DIS_LNKLED                 0x0400
#define DP83640_LEDCR_DIS_ACTLED                 0x0200
#define DP83640_LEDCR_LEDACT_RX                  0x0100
#define DP83640_LEDCR_BLINK_FREQ                 0x00C0
#define DP83640_LEDCR_BLINK_FREQ_6HZ             0x0000
#define DP83640_LEDCR_BLINK_FREQ_12HZ            0x0040
#define DP83640_LEDCR_BLINK_FREQ_24HZ            0x0080
#define DP83640_LEDCR_BLINK_FREQ_48HZ            0x00C0
#define DP83640_LEDCR_DRV_SPDLED                 0x0020
#define DP83640_LEDCR_DRV_LNKLED                 0x0010
#define DP83640_LEDCR_DRV_ACTLED                 0x0008
#define DP83640_LEDCR_SPDLED                     0x0004
#define DP83640_LEDCR_LNKLED                     0x0002
#define DP83640_LEDCR_ACTLED                     0x0001

//PHY Control register
#define DP83640_PHYCR_MDIX_EN                    0x8000
#define DP83640_PHYCR_FORCE_MDIX                 0x4000
#define DP83640_PHYCR_PAUSE_RX                   0x2000
#define DP83640_PHYCR_PAUSE_TX                   0x1000
#define DP83640_PHYCR_BIST_FE                    0x0800
#define DP83640_PHYCR_PSR_15                     0x0400
#define DP83640_PHYCR_BIST_STATUS                0x0200
#define DP83640_PHYCR_BIST_START                 0x0100
#define DP83640_PHYCR_BP_STRETCH                 0x0080
#define DP83640_PHYCR_LED_CNFG                   0x0060
#define DP83640_PHYCR_PHYADDR                    0x001F

//10Base-T Status/Control register
#define DP83640_10BTSCR_SQUELCH                  0x0E00
#define DP83640_10BTSCR_LOOPBACK_10_DIS          0x0100
#define DP83640_10BTSCR_LP_DIS                   0x0080
#define DP83640_10BTSCR_FORCE_LINK_10            0x0040
#define DP83640_10BTSCR_FORCE_POL_COR            0x0020
#define DP83640_10BTSCR_POLARITY                 0x0010
#define DP83640_10BTSCR_AUTOPOL_DIS              0x0008
#define DP83640_10BTSCR_10BT_SCALE_MSB           0x0004
#define DP83640_10BTSCR_HEARTBEAT_DIS            0x0002
#define DP83640_10BTSCR_JABBER_DIS               0x0001

//CD Test Control and BIST Extensions register
#define DP83640_CDCTRL1_BIST_ERROR_COUNT         0xFF00
#define DP83640_CDCTRL1_MII_CLOCK_EN             0x0040
#define DP83640_CDCTRL1_BIST_CONT                0x0020
#define DP83640_CDCTRL1_CDPATTEN_10              0x0010
#define DP83640_CDCTRL1_MDIO_PULL_EN             0x0008
#define DP83640_CDCTRL1_PATT_GAP_10M             0x0004
#define DP83640_CDCTRL1_CDPATTSEL                0x0003

//PHY Control 2 register
#define DP83640_PHYCR2_SYNC_ENET_EN              0x2000
#define DP83640_PHYCR2_CLK_OUT_RXCLK             0x1000
#define DP83640_PHYCR2_BC_WRITE                  0x0800
#define DP83640_PHYCR2_PHYTER_COMP               0x0400
#define DP83640_PHYCR2_SOFT_RESET                0x0200
#define DP83640_PHYCR2_CLK_OUT_DIS               0x0002

//Energy Detect Control register
#define DP83640_EDCR_ED_EN                       0x8000
#define DP83640_EDCR_ED_AUTO_UP                  0x4000
#define DP83640_EDCR_ED_AUTO_DOWN                0x2000
#define DP83640_EDCR_ED_MAN                      0x1000
#define DP83640_EDCR_ED_BURST_DIS                0x0800
#define DP83640_EDCR_ED_PWR_STATE                0x0400
#define DP83640_EDCR_ED_ERR_MET                  0x0200
#define DP83640_EDCR_ED_DATA_MET                 0x0100
#define DP83640_EDCR_ED_ERR_COUNT                0x00F0
#define DP83640_EDCR_ED_DATA_COUNT               0x000F

//PHY Control Frames Configuration register
#define DP83640_PCFCR_PCF_STS_ERR                0x8000
#define DP83640_PCFCR_PCF_STS_OK                 0x4000
#define DP83640_PCFCR_PCF_DA_SEL                 0x0100
#define DP83640_PCFCR_PCF_INT_CTL                0x00C0
#define DP83640_PCFCR_PCF_BC_DIS                 0x0020
#define DP83640_PCFCR_PCF_BUF                    0x001E
#define DP83640_PCFCR_PCF_EN                     0x0001

//Signal Detect Configuration register
#define DP83640_SD_CNFG_SD_TIME                  0x0100

//100 Mb Length Detect register
#define DP83640_LEN100_DET_CABLE_LEN             0x00FF

//100 Mb Frequency Offset Indication register
#define DP83640_FREQ100_SAMPLE_FREQ              0x8000
#define DP83640_FREQ100_SEL_FC                   0x0100
#define DP83640_FREQ100_FREQ_OFFSET              0x00FF

//TDR Control register
#define DP83640_TDR_CTRL_TDR_ENABLE              0x8000
#define DP83640_TDR_CTRL_TDR_100MB               0x4000
#define DP83640_TDR_CTRL_TX_CHANNEL              0x2000
#define DP83640_TDR_CTRL_RX_CHANNEL              0x1000
#define DP83640_TDR_CTRL_SEND_TDR                0x0800
#define DP83640_TDR_CTRL_TDR_WIDTH               0x0700
#define DP83640_TDR_CTRL_TDR_MIN_MODE            0x0080
#define DP83640_TDR_CTRL_RX_THRESHOLD            0x003F

//TDR Window register
#define DP83640_TDR_WIN_TDR_START                0xFF00
#define DP83640_TDR_WIN_TDR_STOP                 0x00FF

//TDR Peak register
#define DP83640_TDR_PEAK_TDR_PEAK                0x3F00
#define DP83640_TDR_PEAK_TDR_PEAK_TIME           0x00FF

//TDR Threshold register
#define DP83640_TDR_THR_TDR_THR_MET              0x0100
#define DP83640_TDR_THR_TDR_THR_TIME             0x00FF

//Variance Control register
#define DP83640_VAR_CTRL_VAR_RDY                 0x8000
#define DP83640_VAR_CTRL_VAR_FREEZE              0x0008
#define DP83640_VAR_CTRL_VAR_TIMER               0x0006
#define DP83640_VAR_CTRL_VAR_ENABLE              0x0001

//Link Quality Monitor register
#define DP83640_LQMR_LQM_ENABLE                  0x8000
#define DP83640_LQMR_RESTART_ON_FC               0x4000
#define DP83640_LQMR_RESTART_ON_FREQ             0x2000
#define DP83640_LQMR_RESTART_ON_DBLW             0x1000
#define DP83640_LQMR_RESTART_ON_DAGC             0x0800
#define DP83640_LQMR_RESTART_ON_C1               0x0400
#define DP83640_LQMR_FC_HI_WARN                  0x0200
#define DP83640_LQMR_FC_LO_WARN                  0x0100
#define DP83640_LQMR_FREQ_HI_WARN                0x0080
#define DP83640_LQMR_FREQ_LO_WARN                0x0040
#define DP83640_LQMR_DBLW_HI_WARN                0x0020
#define DP83640_LQMR_DBLW_LO_WARN                0x0010
#define DP83640_LQMR_DAGC_HI_WARN                0x0008
#define DP83640_LQMR_DAGC_LO_WARN                0x0004
#define DP83640_LQMR_C1_HI_WARN                  0x0002
#define DP83640_LQMR_C1_LO_WARN                  0x0001

//Link Quality Data register
#define DP83640_LQDR_SAMPLE_PARAM                0x2000
#define DP83640_LQDR_WRITE_LQ_THR                0x1000
#define DP83640_LQDR_LQ_PARAM_SEL                0x0E00
#define DP83640_LQDR_LQ_THR_SEL                  0x0100
#define DP83640_LQDR_LQ_THR_DATA                 0x00FF

//Link Quality Monitor 2 register
#define DP83640_LQMR2_RESTART_ON_VAR             0x0400
#define DP83640_LQMR2_VAR_HI_WARN                0x0002

//PTP Control register
#define DP83640_PTP_CTL_TRIG_SEL                 0x1C00
#define DP83640_PTP_CTL_TRIG_DIS                 0x0200
#define DP83640_PTP_CTL_TRIG_EN                  0x0100
#define DP83640_PTP_CTL_TRIG_READ                0x0080
#define DP83640_PTP_CTL_TRIG_LOAD                0x0040
#define DP83640_PTP_CTL_PTP_RD_CLK               0x0020
#define DP83640_PTP_CTL_PTP_LOAD_CLK             0x0010
#define DP83640_PTP_CTL_PTP_STEP_CLK             0x0008
#define DP83640_PTP_CTL_PTP_ENABLE               0x0004
#define DP83640_PTP_CTL_PTP_DISABLE              0x0002
#define DP83640_PTP_CTL_PTP_RESET                0x0001

//PTP Time Data register
#define DP83640_PTP_TDR_TIME_DATA                0xFFFF

//PTP Status register
#define DP83640_PTP_STS_TXTS_RDY                 0x0800
#define DP83640_PTP_STS_RXTS_RDY                 0x0400
#define DP83640_PTP_STS_TRIG_DONE                0x0200
#define DP83640_PTP_STS_EVENT_RDY                0x0100
#define DP83640_PTP_STS_TXTS_IE                  0x0008
#define DP83640_PTP_STS_RXTS_IE                  0x0004
#define DP83640_PTP_STS_TRIG_IE                  0x0002
#define DP83640_PTP_STS_EVENT_IE                 0x0001

//PTP Trigger Status register
#define DP83640_PTP_TSTS_TRIG7_ERROR             0x8000
#define DP83640_PTP_TSTS_TRIG7_ACTIVE            0x4000
#define DP83640_PTP_TSTS_TRIG6_ERROR             0x2000
#define DP83640_PTP_TSTS_TRIG6_ACTIVE            0x2000
#define DP83640_PTP_TSTS_TRIG5_ERROR             0x0800
#define DP83640_PTP_TSTS_TRIG5_ACTIVE            0x0400
#define DP83640_PTP_TSTS_TRIG4_ERROR             0x0200
#define DP83640_PTP_TSTS_TRIG4_ACTIVE            0x0100
#define DP83640_PTP_TSTS_TRIG3_ERROR             0x0080
#define DP83640_PTP_TSTS_TRIG3_ACTIVE            0x0040
#define DP83640_PTP_TSTS_TRIG2_ERROR             0x0020
#define DP83640_PTP_TSTS_TRIG2_ACTIVE            0x0010
#define DP83640_PTP_TSTS_TRIG1_ERROR             0x0008
#define DP83640_PTP_TSTS_TRIG1_ACTIVE            0x0004
#define DP83640_PTP_TSTS_TRIG0_ERROR             0x0002
#define DP83640_PTP_TSTS_TRIG0_ACTIVE            0x0001

//PTP Rate Low register
#define DP83640_PTP_RATEL_PTP_RATE_LO            0xFFFF

//PTP Rate High register
#define DP83640_PTP_RATEH_PTP_RATE_DIR           0x8000
#define DP83640_PTP_RATEH_PTP_TMP_RATE           0x4000
#define DP83640_PTP_RATEH_PTP_RATE_HI            0x03FF

//PTP Event Status register
#define DP83640_PTP_ESTS_EVNTS_MISSED            0x0700
#define DP83640_PTP_ESTS_EVNT_TS_LEN             0x00C0
#define DP83640_PTP_ESTS_EVNT_RF                 0x0020
#define DP83640_PTP_ESTS_EVNT_NUM                0x001C
#define DP83640_PTP_ESTS_MULT_EVNT               0x0002
#define DP83640_PTP_ESTS_EVENT_DET               0x0001

//PTP Event Data register
#define DP83640_PTP_EDATA_E7_RISE                0x8000
#define DP83640_PTP_EDATA_E7_DET                 0x4000
#define DP83640_PTP_EDATA_E6_RISE                0x2000
#define DP83640_PTP_EDATA_E6_DET                 0x1000
#define DP83640_PTP_EDATA_E5_RISE                0x0800
#define DP83640_PTP_EDATA_E5_DET                 0x0400
#define DP83640_PTP_EDATA_E4_RISE                0x0200
#define DP83640_PTP_EDATA_E4_DET                 0x0100
#define DP83640_PTP_EDATA_E3_RISE                0x0080
#define DP83640_PTP_EDATA_E3_DET                 0x0040
#define DP83640_PTP_EDATA_E2_RISE                0x0020
#define DP83640_PTP_EDATA_E2_DET                 0x0010
#define DP83640_PTP_EDATA_E1_RISE                0x0008
#define DP83640_PTP_EDATA_E1_DET                 0x0004
#define DP83640_PTP_EDATA_E0_RISE                0x0002
#define DP83640_PTP_EDATA_E0_DET                 0x0001

//PTP Trigger Configuration register
#define DP83640_PTP_TRIG_TRIG_PULSE              0x8000
#define DP83640_PTP_TRIG_TRIG_PER                0x4000
#define DP83640_PTP_TRIG_TRIG_IF_LATE            0x2000
#define DP83640_PTP_TRIG_TRIG_NOTIFY             0x1000
#define DP83640_PTP_TRIG_TRIG_GPIO               0x0F00
#define DP83640_PTP_TRIG_TRIG_TOGGLE             0x0080
#define DP83640_PTP_TRIG_TRIG_CSEL               0x000E
#define DP83640_PTP_TRIG_TRIG_WR                 0x0001

//PTP Event Configuration register
#define DP83640_PTP_EVNT_EVNT_RISE               0x4000
#define DP83640_PTP_EVNT_EVNT_FALL               0x2000
#define DP83640_PTP_EVNT_EVNT_SINGLE             0x1000
#define DP83640_PTP_EVNT_EVNT_GPIO               0x0F00
#define DP83640_PTP_EVNT_EVNT_SEL                0x000E
#define DP83640_PTP_EVNT_EVNT_WR                 0x0001

//PTP Transmit Configuration 0 register
#define DP83640_PTP_TXCFG0_SYNC_1STEP            0x8000
#define DP83640_PTP_TXCFG0_DR_INSERT             0x2000
#define DP83640_PTP_TXCFG0_NTP_TS_EN             0x1000
#define DP83640_PTP_TXCFG0_IGNORE_2STEP          0x0800
#define DP83640_PTP_TXCFG0_CRC_1STEP             0x0400
#define DP83640_PTP_TXCFG0_CHK_1STEP             0x0200
#define DP83640_PTP_TXCFG0_IP1588_EN             0x0100
#define DP83640_PTP_TXCFG0_TX_L2_EN              0x0080
#define DP83640_PTP_TXCFG0_TX_IPV6_EN            0x0040
#define DP83640_PTP_TXCFG0_TX_IPV4_EN            0x0020
#define DP83640_PTP_TXCFG0_TX_PTP_VER            0x001E
#define DP83640_PTP_TXCFG0_TX_TS_EN              0x0001

//PTP Transmit Configuration 1 register
#define DP83640_PTP_TXCFG1_BYTE0_MASK            0xFF00
#define DP83640_PTP_TXCFG1_BYTE0_DATA            0x00FF

//PHY Status Frames Configuration 0 register
#define DP83640_PSF_CFG0_MAC_SRC_ADD             0x1800
#define DP83640_PSF_CFG0_MIN_PRE                 0x0700
#define DP83640_PSF_CFG0_PSF_ENDIAN              0x0080
#define DP83640_PSF_CFG0_PSF_IPV4                0x0040
#define DP83640_PSF_CFG0_PSF_PCF_RD              0x0020
#define DP83640_PSF_CFG0_PSF_ERR_EN              0x0010
#define DP83640_PSF_CFG0_PSF_TXTS_EN             0x0008
#define DP83640_PSF_CFG0_PSF_RXTS_EN             0x0004
#define DP83640_PSF_CFG0_PSF_TRIG_EN             0x0002
#define DP83640_PSF_CFG0_PSF_EVNT_EN             0x0001

//PTP Receive Configuration 0 register
#define DP83640_PTP_RXCFG0_DOMAIN_EN             0x8000
#define DP83640_PTP_RXCFG0_ALT_MAST_DIS          0x4000
#define DP83640_PTP_RXCFG0_USER_IP_SEL           0x2000
#define DP83640_PTP_RXCFG0_USER_IP_EN            0x1000
#define DP83640_PTP_RXCFG0_RX_SLAVE              0x0800
#define DP83640_PTP_RXCFG0_IP1588_EN             0x0700
#define DP83640_PTP_RXCFG0_RX_L2_EN              0x0080
#define DP83640_PTP_RXCFG0_RX_IPV6_EN            0x0040
#define DP83640_PTP_RXCFG0_RX_IPV4_EN            0x0020
#define DP83640_PTP_RXCFG0_RX_PTP_VER            0x001E
#define DP83640_PTP_RXCFG0_RX_TS_EN              0x0001

//PTP Receive Configuration 1 register
#define DP83640_PTP_RXCFG1_BYTE0_MASK            0xFF00
#define DP83640_PTP_RXCFG1_BYTE0_DATA            0x00FF

//PTP Receive Configuration 2 register
#define DP83640_PTP_RXCFG2_IP_ADDR_DATA          0xFFFF

//PTP Receive Configuration 3 register
#define DP83640_PTP_RXCFG3_TS_MIN_IFG            0xF000
#define DP83640_PTP_RXCFG3_ACC_UDP               0x0800
#define DP83640_PTP_RXCFG3_ACC_CRC               0x0400
#define DP83640_PTP_RXCFG3_TS_APPEND             0x0200
#define DP83640_PTP_RXCFG3_TS_INSERT             0x0100
#define DP83640_PTP_RXCFG3_PTP_DOMAIN            0x00FF

//PTP Receive Configuration 4 register
#define DP83640_PTP_RXCFG4_IPV4_UDP_MOD          0x8000
#define DP83640_PTP_RXCFG4_TS_SEC_EN             0x4000
#define DP83640_PTP_RXCFG4_TS_SEC_LEN            0x3000
#define DP83640_PTP_RXCFG4_RXTS_NS_OFF           0x0FC0
#define DP83640_PTP_RXCFG4_RXTS_SEC_OFF          0x003F

//PTP Clock Output Control register
#define DP83640_PTP_COC_PTP_CLKOUT_EN            0x8000
#define DP83640_PTP_COC_PTP_CLKOUT_SEL           0x4000
#define DP83640_PTP_COC_PTP_CLKOUT_SPEEDSEL      0x2000
#define DP83640_PTP_COC_PTP_CLKDIV               0x00FF

//PHY Status Frames Configuration 1 register
#define DP83640_PSF_CFG1_PTPRESERVED             0xF000
#define DP83640_PSF_CFG1_VERSIONPTP              0x0F00
#define DP83640_PSF_CFG1_TRANSPORT_SPECIFIC      0x00F0
#define DP83640_PSF_CFG1_MESSAGETYPE             0x000F

//PHY Status Frames Configuration 2 register
#define DP83640_PSF_CFG2_IP_SA_BYTE1             0xFF00
#define DP83640_PSF_CFG2_IP_SA_BYTE0             0x00FF

//PHY Status Frames Configuration 3 register
#define DP83640_PSF_CFG3_IP_SA_BYTE3             0xFF00
#define DP83640_PSF_CFG3_IP_SA_BYTE2             0x00FF

//PHY Status Frames Configuration 4 register
#define DP83640_PSF_CFG4_IP_CHKSUM               0xFFFF

//PTP SFD Configuration register
#define DP83640_PTP_SFDCFG_TX_SFD_GPIO           0x00F0
#define DP83640_PTP_SFDCFG_RX_SFD_GPIO           0x000F

//PTP Interrupt Control register
#define DP83640_PTP_INTCTL_PTP_INT_GPIO          0x000F

//PTP Clock Source register
#define DP83640_PTP_CLKSRC_CLK_SRC               0xC000
#define DP83640_PTP_CLKSRC_CLK_SRC_PER           0x007F

//PTP Offset register
#define DP83640_PTP_OFF_PTP_OFFSET               0x00FF

//PTP GPIO Monitor register
#define DP83640_PTP_GPIOMON_PTP_GPIO_IN          0x0FFF

//PTP Receive Hash register
#define DP83640_PTP_RXHASH_RX_HASH_EN            0x1000
#define DP83640_PTP_RXHASH_PTP_RX_HASH           0x0FFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//DP83640 Ethernet PHY driver
extern const PhyDriver dp83640PhyDriver;

//DP83640 related functions
error_t dp83640Init(NetInterface *interface);

void dp83640Tick(NetInterface *interface);

void dp83640EnableIrq(NetInterface *interface);
void dp83640DisableIrq(NetInterface *interface);

void dp83640EventHandler(NetInterface *interface);

void dp83640WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t dp83640ReadPhyReg(NetInterface *interface, uint8_t address);

void dp83640DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
