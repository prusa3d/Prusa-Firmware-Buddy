/**
 * @file dp83620_driver.h
 * @brief DP83620 Ethernet PHY driver
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

#ifndef _DP83620_DRIVER_H
#define _DP83620_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef DP83620_PHY_ADDR
   #define DP83620_PHY_ADDR 1
#elif (DP83620_PHY_ADDR < 0 || DP83620_PHY_ADDR > 31)
   #error DP83620_PHY_ADDR parameter is not valid
#endif

//DP83620 PHY registers (page 0)
#define DP83620_BMCR                             0x00
#define DP83620_BMSR                             0x01
#define DP83620_PHYIDR1                          0x02
#define DP83620_PHYIDR2                          0x03
#define DP83620_ANAR                             0x04
#define DP83620_ANLPAR                           0x05
#define DP83620_ANER                             0x06
#define DP83620_ANNPTR                           0x07
#define DP83620_PHYSTS                           0x10
#define DP83620_MICR                             0x11
#define DP83620_MISR                             0x12
#define DP83620_PAGSR                            0x13
#define DP83620_FCSCR                            0x14
#define DP83620_RECR                             0x15
#define DP83620_PCSR                             0x16
#define DP83620_RBR                              0x17
#define DP83620_LEDCR                            0x18
#define DP83620_PHYCR                            0x19
#define DP83620_10BTSCR                          0x1A
#define DP83620_CDCTRL1                          0x1B
#define DP83620_PHYCR2                           0x1C
#define DP83620_EDCR                             0x1D
#define DP83620_PCFCR                            0x1F

//DP83620 PHY registers (page 1)
#define DP83620_SD_CNFG                          0x1E

//DP83620 PHY registers (page 2)
#define DP83620_LEN100_DET                       0x14
#define DP83620_FREQ100                          0x15
#define DP83620_TDR_CTRL                         0x16
#define DP83620_TDR_WIN                          0x17
#define DP83620_TDR_PEAK                         0x18
#define DP83620_TDR_THR                          0x19
#define DP83620_VAR_CTRL                         0x1A
#define DP83620_VAR_DAT                          0x1B
#define DP83620_LQMR                             0x1D
#define DP83620_LQDR                             0x1E
#define DP83620_LQMR2                            0x1F

//DP83620 PHY registers (page 5)
#define DP83620_PSF_CFG                          0x18

//Basic Mode Control register
#define DP83620_BMCR_RESET                       0x8000
#define DP83620_BMCR_LOOPBACK                    0x4000
#define DP83620_BMCR_SPEED_SEL                   0x2000
#define DP83620_BMCR_AN_EN                       0x1000
#define DP83620_BMCR_POWER_DOWN                  0x0800
#define DP83620_BMCR_ISOLATE                     0x0400
#define DP83620_BMCR_RESTART_AN                  0x0200
#define DP83620_BMCR_DUPLEX_MODE                 0x0100
#define DP83620_BMCR_COL_TEST                    0x0080
#define DP83620_BMCR_UNIDIRECTIONAL_EN           0x0020

//Basic Mode Status register
#define DP83620_BMSR_100BT4                      0x8000
#define DP83620_BMSR_100BTX_FD                   0x4000
#define DP83620_BMSR_100BTX_HD                   0x2000
#define DP83620_BMSR_10BT_FD                     0x1000
#define DP83620_BMSR_10BT_HD                     0x0800
#define DP83620_BMSR_UNIDIRECTIONAL_ABLE         0x0080
#define DP83620_BMSR_MF_PREAMBLE_SUPPR           0x0040
#define DP83620_BMSR_AN_COMPLETE                 0x0020
#define DP83620_BMSR_REMOTE_FAULT                0x0010
#define DP83620_BMSR_AN_CAPABLE                  0x0008
#define DP83620_BMSR_LINK_STATUS                 0x0004
#define DP83620_BMSR_JABBER_DETECT               0x0002
#define DP83620_BMSR_EXTENDED_CAPABLE            0x0001

//PHY Identifier 1 register
#define DP83620_PHYIDR1_OUI_MSB                  0xFFFF
#define DP83620_PHYIDR1_OUI_MSB_DEFAULT          0x2000

//PHY Identifier 2 register
#define DP83620_PHYIDR2_OUI_LSB                  0xFC00
#define DP83620_PHYIDR2_OUI_LSB_DEFAULT          0x5C00
#define DP83620_PHYIDR2_VNDR_MDL                 0x03F0
#define DP83620_PHYIDR2_VNDR_MDL_DEFAULT         0x00E0
#define DP83620_PHYIDR2_MDL_REV                  0x000F

//Auto-Negotiation Advertisement register
#define DP83620_ANAR_NEXT_PAGE                   0x8000
#define DP83620_ANAR_REMOTE_FAULT                0x2000
#define DP83620_ANAR_ASM_DIR                     0x0800
#define DP83620_ANAR_PAUSE                       0x0400
#define DP83620_ANAR_100BT4                      0x0200
#define DP83620_ANAR_100BTX_FD                   0x0100
#define DP83620_ANAR_100BTX_HD                   0x0080
#define DP83620_ANAR_10BT_FD                     0x0040
#define DP83620_ANAR_10BT_HD                     0x0020
#define DP83620_ANAR_SELECTOR                    0x001F
#define DP83620_ANAR_SELECTOR_DEFAULT            0x0001

//Auto-Negotiation Link Partner Ability register
#define DP83620_ANLPAR_NEXT_PAGE                 0x8000
#define DP83620_ANLPAR_ACK                       0x4000
#define DP83620_ANLPAR_REMOTE_FAULT              0x2000
#define DP83620_ANLPAR_ASM_DIR                   0x0800
#define DP83620_ANLPAR_PAUSE                     0x0400
#define DP83620_ANLPAR_100BT4                    0x0200
#define DP83620_ANLPAR_100BTX_FD                 0x0100
#define DP83620_ANLPAR_100BTX_HD                 0x0080
#define DP83620_ANLPAR_10BT_FD                   0x0040
#define DP83620_ANLPAR_10BT_HD                   0x0020
#define DP83620_ANLPAR_SELECTOR                  0x001F
#define DP83620_ANLPAR_SELECTOR_DEFAULT          0x0001

//Auto-Negotiation Expansion register
#define DP83620_ANER_PAR_DETECT_FAULT            0x0010
#define DP83620_ANER_LP_NP_ABLE                  0x0008
#define DP83620_ANER_NP_ABLE                     0x0004
#define DP83620_ANER_PAGE_RX                     0x0002
#define DP83620_ANER_LP_AN_ABLE                  0x0001

//Auto-Negotiation Next Page TX register
#define DP83620_ANNPTR_NEXT_PAGE                 0x8000
#define DP83620_ANNPTR_MSG_PAGE                  0x2000
#define DP83620_ANNPTR_ACK2                      0x1000
#define DP83620_ANNPTR_TOGGLE                    0x0800
#define DP83620_ANNPTR_CODE                      0x07FF

//PHY Status register
#define DP83620_PHYSTS_MDIX_MODE                 0x4000
#define DP83620_PHYSTS_RECEIVE_ERROR_LATCH       0x2000
#define DP83620_PHYSTS_POLARITY_STATUS           0x1000
#define DP83620_PHYSTS_FALSE_CARRIER_SENSE_LATCH 0x0800
#define DP83620_PHYSTS_SIGNAL_DETECT             0x0400
#define DP83620_PHYSTS_DESCRAMBLER_LOCK          0x0200
#define DP83620_PHYSTS_PAGE_RECEIVED             0x0100
#define DP83620_PHYSTS_MII_INTERRUPT             0x0080
#define DP83620_PHYSTS_REMOTE_FAULT              0x0040
#define DP83620_PHYSTS_JABBER_DETECT             0x0020
#define DP83620_PHYSTS_AN_COMPLETE               0x0010
#define DP83620_PHYSTS_LOOPBACK_STATUS           0x0008
#define DP83620_PHYSTS_DUPLEX_STATUS             0x0004
#define DP83620_PHYSTS_SPEED_STATUS              0x0002
#define DP83620_PHYSTS_LINK_STATUS               0x0001

//MII Interrupt Control register
#define DP83620_MICR_TINT                        0x0004
#define DP83620_MICR_INTEN                       0x0002
#define DP83620_MICR_INT_OE                      0x0001

//MII Interrupt Status register
#define DP83620_MISR_LQ_INT                      0x8000
#define DP83620_MISR_ED_INT                      0x4000
#define DP83620_MISR_LINK_INT                    0x2000
#define DP83620_MISR_SPD_INT                     0x1000
#define DP83620_MISR_DUP_INT                     0x0800
#define DP83620_MISR_ANC_INT                     0x0400
#define DP83620_MISR_FHF_INT                     0x0200
#define DP83620_MISR_RHF_INT                     0x0100
#define DP83620_MISR_LQ_INT_EN                   0x0080
#define DP83620_MISR_ED_INT_EN                   0x0040
#define DP83620_MISR_LINK_INT_EN                 0x0020
#define DP83620_MISR_SPD_INT_EN                  0x0010
#define DP83620_MISR_DUP_INT_EN                  0x0008
#define DP83620_MISR_ANC_INT_EN                  0x0004
#define DP83620_MISR_FHF_INT_EN                  0x0002
#define DP83620_MISR_RHF_INT_EN                  0x0001

//Page Select register
#define DP83620_PAGSR_PAGE_SEL                   0x0007

//False Carrier Sense Counter register
#define DP83620_FCSCR_FCSCNT                     0x00FF

//Receive Error Counter register
#define DP83620_RECR_RXERCNT                     0x00FF

//PCS Configuration and Status register
#define DP83620_PCSR_AUTO_CROSSOVER              0x8000
#define DP83620_PCSR_FREE_CLK                    0x0800
#define DP83620_PCSR_TQ_EN                       0x0400
#define DP83620_PCSR_SD_FORCE_PMA                0x0200
#define DP83620_PCSR_SD_OPTION                   0x0100
#define DP83620_PCSR_DESC_TIME                   0x0080
#define DP83620_PCSR_FX_EN                       0x0040
#define DP83620_PCSR_FORCE_100_OK                0x0020
#define DP83620_PCSR_FEFI_EN                     0x0008
#define DP83620_PCSR_NRZI_BYPASS                 0x0004
#define DP83620_PCSR_SCRAM_BYPASS                0x0002
#define DP83620_PCSR_DESCRAM_BYPASS              0x0001

//RMII and Bypass register
#define DP83620_RBR_RMII_MASTER                  0x4000
#define DP83620_RBR_DIS_TX_OPT                   0x2000
#define DP83620_RBR_PMD_LOOP                     0x0100
#define DP83620_RBR_SCMII_RX                     0x0080
#define DP83620_RBR_SCMII_TX                     0x0040
#define DP83620_RBR_RMII_MODE                    0x0020
#define DP83620_RBR_RMII_REV1_0                  0x0010
#define DP83620_RBR_RX_OVF_STS                   0x0008
#define DP83620_RBR_RX_UNF_STS                   0x0004
#define DP83620_RBR_ELAST_BUF                    0x0003

//LED Direct Control register
#define DP83620_LEDCR_DIS_SPDLED                 0x0800
#define DP83620_LEDCR_DIS_LNKLED                 0x0400
#define DP83620_LEDCR_DIS_ACTLED                 0x0200
#define DP83620_LEDCR_LEDACT_RX                  0x0100
#define DP83620_LEDCR_BLINK_FREQ                 0x00C0
#define DP83620_LEDCR_BLINK_FREQ_6HZ             0x0000
#define DP83620_LEDCR_BLINK_FREQ_12HZ            0x0040
#define DP83620_LEDCR_BLINK_FREQ_24HZ            0x0080
#define DP83620_LEDCR_BLINK_FREQ_48HZ            0x00C0
#define DP83620_LEDCR_DRV_SPDLED                 0x0020
#define DP83620_LEDCR_DRV_LNKLED                 0x0010
#define DP83620_LEDCR_DRV_ACTLED                 0x0008
#define DP83620_LEDCR_SPDLED                     0x0004
#define DP83620_LEDCR_LNKLED                     0x0002
#define DP83620_LEDCR_ACTLED                     0x0001

//PHY Control register
#define DP83620_PHYCR_MDIX_EN                    0x8000
#define DP83620_PHYCR_FORCE_MDIX                 0x4000
#define DP83620_PHYCR_PAUSE_RX                   0x2000
#define DP83620_PHYCR_PAUSE_TX                   0x1000
#define DP83620_PHYCR_BIST_FE                    0x0800
#define DP83620_PHYCR_PSR_15                     0x0400
#define DP83620_PHYCR_BIST_STATUS                0x0200
#define DP83620_PHYCR_BIST_START                 0x0100
#define DP83620_PHYCR_BP_STRETCH                 0x0080
#define DP83620_PHYCR_LED_CNFG                   0x0060
#define DP83620_PHYCR_PHYADDR                    0x001F

//10Base-T Status/Control register
#define DP83620_10BTSCR_SQUELCH                  0x0E00
#define DP83620_10BTSCR_LOOPBACK_10_DIS          0x0100
#define DP83620_10BTSCR_LP_DIS                   0x0080
#define DP83620_10BTSCR_FORCE_LINK_10            0x0040
#define DP83620_10BTSCR_FORCE_POL_COR            0x0020
#define DP83620_10BTSCR_POLARITY                 0x0010
#define DP83620_10BTSCR_AUTOPOL_DIS              0x0008
#define DP83620_10BTSCR_10BT_SCALE_MSB           0x0004
#define DP83620_10BTSCR_HEARTBEAT_DIS            0x0002
#define DP83620_10BTSCR_JABBER_DIS               0x0001

//CD Test Control and BIST Extensions register
#define DP83620_CDCTRL1_BIST_ERROR_COUNT         0xFF00
#define DP83620_CDCTRL1_MII_CLOCK_EN             0x0040
#define DP83620_CDCTRL1_BIST_CONT                0x0020
#define DP83620_CDCTRL1_CDPATTEN_10              0x0010
#define DP83620_CDCTRL1_MDIO_PULL_EN             0x0008
#define DP83620_CDCTRL1_PATT_GAP_10M             0x0004
#define DP83620_CDCTRL1_CDPATTSEL                0x0003

//PHY Control 2 register
#define DP83620_PHYCR2_SYNC_ENET_EN              0x2000
#define DP83620_PHYCR2_CLK_OUT_RXCLK             0x1000
#define DP83620_PHYCR2_BC_WRITE                  0x0800
#define DP83620_PHYCR2_PHYTER_COMP               0x0400
#define DP83620_PHYCR2_SOFT_RESET                0x0200
#define DP83620_PHYCR2_CLK_OUT_DIS               0x0002

//Energy Detect Control register
#define DP83620_EDCR_ED_EN                       0x8000
#define DP83620_EDCR_ED_AUTO_UP                  0x4000
#define DP83620_EDCR_ED_AUTO_DOWN                0x2000
#define DP83620_EDCR_ED_MAN                      0x1000
#define DP83620_EDCR_ED_BURST_DIS                0x0800
#define DP83620_EDCR_ED_PWR_STATE                0x0400
#define DP83620_EDCR_ED_ERR_MET                  0x0200
#define DP83620_EDCR_ED_DATA_MET                 0x0100
#define DP83620_EDCR_ED_ERR_COUNT                0x00F0
#define DP83620_EDCR_ED_DATA_COUNT               0x000F

//PHY Control Frames Configuration register
#define DP83620_PCFCR_PCF_STS_ERR                0x8000
#define DP83620_PCFCR_PCF_STS_OK                 0x4000
#define DP83620_PCFCR_PCF_DA_SEL                 0x0100
#define DP83620_PCFCR_PCF_INT_CTL                0x00C0
#define DP83620_PCFCR_PCF_BC_DIS                 0x0020
#define DP83620_PCFCR_PCF_BUF                    0x001E
#define DP83620_PCFCR_PCF_EN                     0x0001

//Signal Detect Configuration register
#define DP83620_SD_CNFG_SD_TIME                  0x0100

//100 Mb Length Detect register
#define DP83620_LEN100_DET_CABLE_LEN             0x00FF

//100 Mb Frequency Offset Indication register
#define DP83620_FREQ100_SAMPLE_FREQ              0x8000
#define DP83620_FREQ100_SEL_FC                   0x0100
#define DP83620_FREQ100_FREQ_OFFSET              0x00FF

//TDR Control register
#define DP83620_TDR_CTRL_TDR_ENABLE              0x8000
#define DP83620_TDR_CTRL_TDR_100MB               0x4000
#define DP83620_TDR_CTRL_TX_CHANNEL              0x2000
#define DP83620_TDR_CTRL_RX_CHANNEL              0x1000
#define DP83620_TDR_CTRL_SEND_TDR                0x0800
#define DP83620_TDR_CTRL_TDR_WIDTH               0x0700
#define DP83620_TDR_CTRL_TDR_MIN_MODE            0x0080
#define DP83620_TDR_CTRL_RX_THRESHOLD            0x003F

//TDR Window register
#define DP83620_TDR_WIN_TDR_START                0xFF00
#define DP83620_TDR_WIN_TDR_STOP                 0x00FF

//TDR Peak register
#define DP83620_TDR_PEAK_TDR_PEAK                0x3F00
#define DP83620_TDR_PEAK_TDR_PEAK_TIME           0x00FF

//TDR Threshold register
#define DP83620_TDR_THR_TDR_THR_MET              0x0100
#define DP83620_TDR_THR_TDR_THR_TIME             0x00FF

//Variance Control register
#define DP83620_VAR_CTRL_VAR_RDY                 0x8000
#define DP83620_VAR_CTRL_VAR_FREEZE              0x0008
#define DP83620_VAR_CTRL_VAR_TIMER               0x0006
#define DP83620_VAR_CTRL_VAR_ENABLE              0x0001

//Link Quality Monitor register
#define DP83620_LQMR_LQM_ENABLE                  0x8000
#define DP83620_LQMR_RESTART_ON_FC               0x4000
#define DP83620_LQMR_RESTART_ON_FREQ             0x2000
#define DP83620_LQMR_RESTART_ON_DBLW             0x1000
#define DP83620_LQMR_RESTART_ON_DAGC             0x0800
#define DP83620_LQMR_RESTART_ON_C1               0x0400
#define DP83620_LQMR_FC_HI_WARN                  0x0200
#define DP83620_LQMR_FC_LO_WARN                  0x0100
#define DP83620_LQMR_FREQ_HI_WARN                0x0080
#define DP83620_LQMR_FREQ_LO_WARN                0x0040
#define DP83620_LQMR_DBLW_HI_WARN                0x0020
#define DP83620_LQMR_DBLW_LO_WARN                0x0010
#define DP83620_LQMR_DAGC_HI_WARN                0x0008
#define DP83620_LQMR_DAGC_LO_WARN                0x0004
#define DP83620_LQMR_C1_HI_WARN                  0x0002
#define DP83620_LQMR_C1_LO_WARN                  0x0001

//Link Quality Data register
#define DP83620_LQDR_SAMPLE_PARAM                0x2000
#define DP83620_LQDR_WRITE_LQ_THR                0x1000
#define DP83620_LQDR_LQ_PARAM_SEL                0x0E00
#define DP83620_LQDR_LQ_THR_SEL                  0x0100
#define DP83620_LQDR_LQ_THR_DATA                 0x00FF

//Link Quality Monitor 2 register
#define DP83620_LQMR2_RESTART_ON_VAR             0x0400
#define DP83620_LQMR2_VAR_HI_WARN                0x0002

//PHY Status Frame Configuration register
#define DP83620_PSF_CFG_MAC_SRC_ADD              0x1800
#define DP83620_PSF_CFG_MIN_PRE                  0x0700
#define DP83620_PSF_CFG_PSF_ENDIAN               0x0080
#define DP83620_PSF_CFG_PSF_IPV4                 0x0040
#define DP83620_PSF_CFG_PSF_PCF_RD               0x0020
#define DP83620_PSF_CFG_PSF_ERR_EN               0x0010

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//DP83620 Ethernet PHY driver
extern const PhyDriver dp83620PhyDriver;

//DP83620 related functions
error_t dp83620Init(NetInterface *interface);

void dp83620Tick(NetInterface *interface);

void dp83620EnableIrq(NetInterface *interface);
void dp83620DisableIrq(NetInterface *interface);

void dp83620EventHandler(NetInterface *interface);

void dp83620WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t dp83620ReadPhyReg(NetInterface *interface, uint8_t address);

void dp83620DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
