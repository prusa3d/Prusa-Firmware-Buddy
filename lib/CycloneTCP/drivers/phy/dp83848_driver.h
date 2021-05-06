/**
 * @file dp83848_driver.h
 * @brief DP83848 Ethernet PHY driver
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

#ifndef _DP83848_DRIVER_H
#define _DP83848_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef DP83848_PHY_ADDR
   #define DP83848_PHY_ADDR 1
#elif (DP83848_PHY_ADDR < 0 || DP83848_PHY_ADDR > 31)
   #error DP83848_PHY_ADDR parameter is not valid
#endif

//DP83848 PHY registers
#define DP83848_BMCR                             0x00
#define DP83848_BMSR                             0x01
#define DP83848_PHYIDR1                          0x02
#define DP83848_PHYIDR2                          0x03
#define DP83848_ANAR                             0x04
#define DP83848_ANLPAR                           0x05
#define DP83848_ANER                             0x06
#define DP83848_ANNPTR                           0x07
#define DP83848_PHYSTS                           0x10
#define DP83848_MICR                             0x11
#define DP83848_MISR                             0x12
#define DP83848_FCSCR                            0x14
#define DP83848_RECR                             0x15
#define DP83848_PCSR                             0x16
#define DP83848_RBR                              0x17
#define DP83848_LEDCR                            0x18
#define DP83848_PHYCR                            0x19
#define DP83848_10BTSCR                          0x1A
#define DP83848_CDCTRL1                          0x1B
#define DP83848_EDCR                             0x1D

//Basic Mode Control register
#define DP83848_BMCR_RESET                       0x8000
#define DP83848_BMCR_LOOPBACK                    0x4000
#define DP83848_BMCR_SPEED_SEL                   0x2000
#define DP83848_BMCR_AN_EN                       0x1000
#define DP83848_BMCR_POWER_DOWN                  0x0800
#define DP83848_BMCR_ISOLATE                     0x0400
#define DP83848_BMCR_RESTART_AN                  0x0200
#define DP83848_BMCR_DUPLEX_MODE                 0x0100
#define DP83848_BMCR_COL_TEST                    0x0080

//Basic Mode Status register
#define DP83848_BMSR_100BT4                      0x8000
#define DP83848_BMSR_100BTX_FD                   0x4000
#define DP83848_BMSR_100BTX_HD                   0x2000
#define DP83848_BMSR_10BT_FD                     0x1000
#define DP83848_BMSR_10BT_HD                     0x0800
#define DP83848_BMSR_MF_PREAMBLE_SUPPR           0x0040
#define DP83848_BMSR_AN_COMPLETE                 0x0020
#define DP83848_BMSR_REMOTE_FAULT                0x0010
#define DP83848_BMSR_AN_CAPABLE                  0x0008
#define DP83848_BMSR_LINK_STATUS                 0x0004
#define DP83848_BMSR_JABBER_DETECT               0x0002
#define DP83848_BMSR_EXTENDED_CAPABLE            0x0001

//PHY Identifier 1 register
#define DP83848_PHYIDR1_OUI_MSB                  0xFFFF
#define DP83848_PHYIDR1_OUI_MSB_DEFAULT          0x2000

//PHY Identifier 2 register
#define DP83848_PHYIDR2_OUI_LSB                  0xFC00
#define DP83848_PHYIDR2_OUI_LSB_DEFAULT          0x5C00
#define DP83848_PHYIDR2_VNDR_MDL                 0x03F0
#define DP83848_PHYIDR2_VNDR_MDL_DEFAULT         0x0090
#define DP83848_PHYIDR2_MDL_REV                  0x000F

//Auto-Negotiation Advertisement register
#define DP83848_ANAR_NEXT_PAGE                   0x8000
#define DP83848_ANAR_REMOTE_FAULT                0x2000
#define DP83848_ANAR_ASM_DIR                     0x0800
#define DP83848_ANAR_PAUSE                       0x0400
#define DP83848_ANAR_100BT4                      0x0200
#define DP83848_ANAR_100BTX_FD                   0x0100
#define DP83848_ANAR_100BTX_HD                   0x0080
#define DP83848_ANAR_10BT_FD                     0x0040
#define DP83848_ANAR_10BT_HD                     0x0020
#define DP83848_ANAR_SELECTOR                    0x001F
#define DP83848_ANAR_SELECTOR_DEFAULT            0x0001

//Auto-Negotiation Link Partner Ability register
#define DP83848_ANLPAR_NEXT_PAGE                 0x8000
#define DP83848_ANLPAR_ACK                       0x4000
#define DP83848_ANLPAR_REMOTE_FAULT              0x2000
#define DP83848_ANLPAR_ASM_DIR                   0x0800
#define DP83848_ANLPAR_PAUSE                     0x0400
#define DP83848_ANLPAR_100BT4                    0x0200
#define DP83848_ANLPAR_100BTX_FD                 0x0100
#define DP83848_ANLPAR_100BTX_HD                 0x0080
#define DP83848_ANLPAR_10BT_FD                   0x0040
#define DP83848_ANLPAR_10BT_HD                   0x0020
#define DP83848_ANLPAR_SELECTOR                  0x001F
#define DP83848_ANLPAR_SELECTOR_DEFAULT          0x0001

//Auto-Negotiation Expansion register
#define DP83848_ANER_PAR_DETECT_FAULT            0x0010
#define DP83848_ANER_LP_NP_ABLE                  0x0008
#define DP83848_ANER_NP_ABLE                     0x0004
#define DP83848_ANER_PAGE_RX                     0x0002
#define DP83848_ANER_LP_AN_ABLE                  0x0001

//Auto-Negotiation Next Page TX register
#define DP83848_ANNPTR_NEXT_PAGE                 0x8000
#define DP83848_ANNPTR_MSG_PAGE                  0x2000
#define DP83848_ANNPTR_ACK2                      0x1000
#define DP83848_ANNPTR_TOGGLE                    0x0800
#define DP83848_ANNPTR_CODE                      0x07FF

//PHY Status register
#define DP83848_PHYSTS_MDIX_MODE                 0x4000
#define DP83848_PHYSTS_RECEIVE_ERROR_LATCH       0x2000
#define DP83848_PHYSTS_POLARITY_STATUS           0x1000
#define DP83848_PHYSTS_FALSE_CARRIER_SENSE_LATCH 0x0800
#define DP83848_PHYSTS_SIGNAL_DETECT             0x0400
#define DP83848_PHYSTS_DESCRAMBLER_LOCK          0x0200
#define DP83848_PHYSTS_PAGE_RECEIVED             0x0100
#define DP83848_PHYSTS_MII_INTERRUPT             0x0080
#define DP83848_PHYSTS_REMOTE_FAULT              0x0040
#define DP83848_PHYSTS_JABBER_DETECT             0x0020
#define DP83848_PHYSTS_AN_COMPLETE               0x0010
#define DP83848_PHYSTS_LOOPBACK_STATUS           0x0008
#define DP83848_PHYSTS_DUPLEX_STATUS             0x0004
#define DP83848_PHYSTS_SPEED_STATUS              0x0002
#define DP83848_PHYSTS_LINK_STATUS               0x0001

//MII Interrupt Control register
#define DP83848_MICR_TINT                        0x0004
#define DP83848_MICR_INTEN                       0x0002
#define DP83848_MICR_INT_OE                      0x0001

//MII Interrupt Status register
#define DP83848_MISR_ED_INT                      0x4000
#define DP83848_MISR_LINK_INT                    0x2000
#define DP83848_MISR_SPD_INT                     0x1000
#define DP83848_MISR_DUP_INT                     0x0800
#define DP83848_MISR_ANC_INT                     0x0400
#define DP83848_MISR_FHF_INT                     0x0200
#define DP83848_MISR_RHF_INT                     0x0100
#define DP83848_MISR_ED_INT_EN                   0x0040
#define DP83848_MISR_LINK_INT_EN                 0x0020
#define DP83848_MISR_SPD_INT_EN                  0x0010
#define DP83848_MISR_DUP_INT_EN                  0x0008
#define DP83848_MISR_ANC_INT_EN                  0x0004
#define DP83848_MISR_FHF_INT_EN                  0x0002
#define DP83848_MISR_RHF_INT_EN                  0x0001

//False Carrier Sense Counter register
#define DP83848_FCSCR_FCSCNT                     0x00FF

//Receive Error Counter register
#define DP83848_RECR_RXERCNT                     0x00FF

//PCS Sub-Layer Configuration and Status register
#define DP83848_PCSR_TQ_EN                       0x0400
#define DP83848_PCSR_SD_FORCE_PMA                0x0200
#define DP83848_PCSR_SD_OPTION                   0x0100
#define DP83848_PCSR_DESC_TIME                   0x0080
#define DP83848_PCSR_FORCE_100_OK                0x0020
#define DP83848_PCSR_NRZI_BYPASS                 0x0004

//RMII and Bypass register
#define DP83848_RBR_RMII_MODE                    0x0020
#define DP83848_RBR_RMII_REV1_0                  0x0010
#define DP83848_RBR_RX_OVF_STS                   0x0008
#define DP83848_RBR_RX_UNF_STS                   0x0004
#define DP83848_RBR_ELAST_BUF                    0x0003

//LED Direct Control register
#define DP83848_LEDCR_DRV_SPDLED                 0x0020
#define DP83848_LEDCR_DRV_LNKLED                 0x0010
#define DP83848_LEDCR_DRV_ACTLED                 0x0008
#define DP83848_LEDCR_SPDLED                     0x0004
#define DP83848_LEDCR_LNKLED                     0x0002
#define DP83848_LEDCR_ACTLED                     0x0001

//PHY Control register
#define DP83848_PHYCR_MDIX_EN                    0x8000
#define DP83848_PHYCR_FORCE_MDIX                 0x4000
#define DP83848_PHYCR_PAUSE_RX                   0x2000
#define DP83848_PHYCR_PAUSE_TX                   0x1000
#define DP83848_PHYCR_BIST_FE                    0x0800
#define DP83848_PHYCR_PSR_15                     0x0400
#define DP83848_PHYCR_BIST_STATUS                0x0200
#define DP83848_PHYCR_BIST_START                 0x0100
#define DP83848_PHYCR_BP_STRETCH                 0x0080
#define DP83848_PHYCR_LED_CNFG                   0x0060
#define DP83848_PHYCR_PHYADDR                    0x001F

//10Base-T Status/Control register
#define DP83848_10BTSCR_10BT_SERIAL              0x8000
#define DP83848_10BTSCR_SQUELCH                  0x0E00
#define DP83848_10BTSCR_LOOPBACK_10_DIS          0x0100
#define DP83848_10BTSCR_LP_DIS                   0x0080
#define DP83848_10BTSCR_FORCE_LINK_10            0x0040
#define DP83848_10BTSCR_POLARITY                 0x0010
#define DP83848_10BTSCR_HEARTBEAT_DIS            0x0002
#define DP83848_10BTSCR_JABBER_DIS               0x0001

//CD Test Control and BIST Extensions register
#define DP83848_CDCTRL1_BIST_ERROR_COUNT         0xFF00
#define DP83848_CDCTRL1_BIST_CONT_MODE           0x0020
#define DP83848_CDCTRL1_CDPATTEN_10              0x0010
#define DP83848_CDCTRL1_10MEG_PATT_GAP           0x0004
#define DP83848_CDCTRL1_CDPATTSEL                0x0003

//Energy Detect Control register
#define DP83848_EDCR_ED_EN                       0x8000
#define DP83848_EDCR_ED_AUTO_UP                  0x4000
#define DP83848_EDCR_ED_AUTO_DOWN                0x2000
#define DP83848_EDCR_ED_MAN                      0x1000
#define DP83848_EDCR_ED_BURST_DIS                0x0800
#define DP83848_EDCR_ED_PWR_STATE                0x0400
#define DP83848_EDCR_ED_ERR_MET                  0x0200
#define DP83848_EDCR_ED_DATA_MET                 0x0100
#define DP83848_EDCR_ED_ERR_COUNT                0x00F0
#define DP83848_EDCR_ED_DATA_COUNT               0x000F

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//DP83848 Ethernet PHY driver
extern const PhyDriver dp83848PhyDriver;

//DP83848 related functions
error_t dp83848Init(NetInterface *interface);

void dp83848Tick(NetInterface *interface);

void dp83848EnableIrq(NetInterface *interface);
void dp83848DisableIrq(NetInterface *interface);

void dp83848EventHandler(NetInterface *interface);

void dp83848WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t dp83848ReadPhyReg(NetInterface *interface, uint8_t address);

void dp83848DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
