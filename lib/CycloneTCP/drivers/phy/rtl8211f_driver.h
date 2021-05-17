/**
 * @file rtl8211f_driver.h
 * @brief RTL8211F Gigabit Ethernet PHY driver
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

#ifndef _RTL8211F_DRIVER_H
#define _RTL8211F_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef RTL8211F_PHY_ADDR
   #define RTL8211F_PHY_ADDR 1
#elif (RTL8211F_PHY_ADDR < 0 || RTL8211F_PHY_ADDR > 31)
   #error RTL8211F_PHY_ADDR parameter is not valid
#endif

//RTL8211F PHY registers
#define RTL8211F_BMCR                          0x00
#define RTL8211F_BMSR                          0x01
#define RTL8211F_PHYID1                        0x02
#define RTL8211F_PHYID2                        0x03
#define RTL8211F_ANAR                          0x04
#define RTL8211F_ANLPAR                        0x05
#define RTL8211F_ANER                          0x06
#define RTL8211F_ANNPTR                        0x07
#define RTL8211F_ANNPRR                        0x08
#define RTL8211F_GBCR                          0x09
#define RTL8211F_GBSR                          0x0A
#define RTL8211F_MMDACR                        0x0D
#define RTL8211F_MMDAADR                       0x0E
#define RTL8211F_GBESR                         0x0F
#define RTL8211F_INER                          0x12
#define RTL8211F_PHYCR1                        0x18
#define RTL8211F_PHYCR2                        0x19
#define RTL8211F_PHYSR                         0x1A
#define RTL8211F_INSR                          0x1D
#define RTL8211F_PAGSR                         0x1F

//RTL8211F MMD registers
#define RTL8211F_PC1R                          0x03, 0x00
#define RTL8211F_PS1R                          0x03, 0x01
#define RTL8211F_EEECR                         0x03, 0x14
#define RTL8211F_EEEWER                        0x03, 0x16
#define RTL8211F_EEEAR                         0x07, 0x3C
#define RTL8211F_EEELPAR                       0x07, 0x3D

//Basic Mode Control register
#define RTL8211F_BMCR_RESET                    0x8000
#define RTL8211F_BMCR_LOOPBACK                 0x4000
#define RTL8211F_BMCR_SPEED_SEL_LSB            0x2000
#define RTL8211F_BMCR_AN_EN                    0x1000
#define RTL8211F_BMCR_POWER_DOWN               0x0800
#define RTL8211F_BMCR_ISOLATE                  0x0400
#define RTL8211F_BMCR_RESTART_AN               0x0200
#define RTL8211F_BMCR_DUPLEX_MODE              0x0100
#define RTL8211F_BMCR_COL_TEST                 0x0080
#define RTL8211F_BMCR_SPEED_SEL_MSB            0x0040
#define RTL8211F_BMCR_UNI_DIR_EN               0x0020

//Basic Mode Status register
#define RTL8211F_BMSR_100BT4                   0x8000
#define RTL8211F_BMSR_100BTX_FD                0x4000
#define RTL8211F_BMSR_100BTX_HD                0x2000
#define RTL8211F_BMSR_10BT_FD                  0x1000
#define RTL8211F_BMSR_10BT_HD                  0x0800
#define RTL8211F_BMSR_100BT2_FD                0x0400
#define RTL8211F_BMSR_100BT2_HD                0x0200
#define RTL8211F_BMSR_EXTENDED_STATUS          0x0100
#define RTL8211F_BMSR_UNI_DIR_CAPABLE          0x0080
#define RTL8211F_BMSR_PREAMBLE_SUPPR           0x0040
#define RTL8211F_BMSR_AN_COMPLETE              0x0020
#define RTL8211F_BMSR_REMOTE_FAULT             0x0010
#define RTL8211F_BMSR_AN_CAPABLE               0x0008
#define RTL8211F_BMSR_LINK_STATUS              0x0004
#define RTL8211F_BMSR_JABBER_DETECT            0x0002
#define RTL8211F_BMSR_EXTENDED_CAPABLE         0x0001

//PHY Identifier 1 register
#define RTL8211F_PHYID1_OUI_MSB                0xFFFF
#define RTL8211F_PHYID1_OUI_MSB_DEFAULT        0x001C

//PHY Identifier 2 register
#define RTL8211F_PHYID2_OUI_LSB                0xFC00
#define RTL8211F_PHYID2_OUI_LSB_DEFAULT        0xC800
#define RTL8211F_PHYID2_MODEL_NUM              0x03F0
#define RTL8211F_PHYID2_MODEL_NUM_DEFAULT      0x0110
#define RTL8211F_PHYID2_REVISION_NUM           0x000F
#define RTL8211F_PHYID2_REVISION_NUM_DEFAULT   0x0006

//Auto-Negotiation Advertisement register
#define RTL8211F_ANAR_NEXT_PAGE                0x8000
#define RTL8211F_ANAR_REMOTE_FAULT             0x2000
#define RTL8211F_ANAR_ASYM_PAUSE               0x0800
#define RTL8211F_ANAR_PAUSE                    0x0400
#define RTL8211F_ANAR_100BT4                   0x0200
#define RTL8211F_ANAR_100BTX_FD                0x0100
#define RTL8211F_ANAR_100BTX_HD                0x0080
#define RTL8211F_ANAR_10BT_FD                  0x0040
#define RTL8211F_ANAR_10BT_HD                  0x0020
#define RTL8211F_ANAR_SELECTOR                 0x001F
#define RTL8211F_ANAR_SELECTOR_DEFAULT         0x0001

//Auto-Negotiation Link Partner Ability register
#define RTL8211F_ANLPAR_NEXT_PAGE              0x8000
#define RTL8211F_ANLPAR_ACK                    0x4000
#define RTL8211F_ANLPAR_REMOTE_FAULT           0x2000
#define RTL8211F_ANLPAR_ASYM_PAUSE             0x0800
#define RTL8211F_ANLPAR_PAUSE                  0x0400
#define RTL8211F_ANLPAR_100BT4                 0x0200
#define RTL8211F_ANLPAR_100BTX_FD              0x0100
#define RTL8211F_ANLPAR_100BTX_HD              0x0080
#define RTL8211F_ANLPAR_10BT_FD                0x0040
#define RTL8211F_ANLPAR_10BT_HD                0x0020
#define RTL8211F_ANLPAR_SELECTOR               0x001F
#define RTL8211F_ANLPAR_SELECTOR_DEFAULT       0x0001

//Auto-Negotiation Expansion register
#define RTL8211F_ANER_RX_NP_LOCATION_ABLE      0x0040
#define RTL8211F_ANER_RX_NP_LOCATION           0x0020
#define RTL8211F_ANER_PAR_DETECT_FAULT         0x0010
#define RTL8211F_ANER_LP_NEXT_PAGE_ABLE        0x0008
#define RTL8211F_ANER_NEXT_PAGE_ABLE           0x0004
#define RTL8211F_ANER_PAGE_RECEIVED            0x0002
#define RTL8211F_ANER_LP_AN_ABLE               0x0001

//Auto-Negotiation Next Page Transmit register
#define RTL8211F_ANNPTR_NEXT_PAGE              0x8000
#define RTL8211F_ANNPTR_MSG_PAGE               0x2000
#define RTL8211F_ANNPTR_ACK2                   0x1000
#define RTL8211F_ANNPTR_TOGGLE                 0x0800
#define RTL8211F_ANNPTR_MESSAGE                0x07FF

//Auto-Negotiation Next Page Receive register
#define RTL8211F_ANNPRR_NEXT_PAGE              0x8000
#define RTL8211F_ANNPRR_ACK                    0x4000
#define RTL8211F_ANNPRR_MSG_PAGE               0x2000
#define RTL8211F_ANNPRR_ACK2                   0x1000
#define RTL8211F_ANNPRR_TOGGLE                 0x0800
#define RTL8211F_ANNPRR_MESSAGE                0x07FF

//1000Base-T Control register
#define RTL8211F_GBCR_TEST_MODE                0xE000
#define RTL8211F_GBCR_MS_MAN_CONF_EN           0x1000
#define RTL8211F_GBCR_MS_MAN_CONF_VAL          0x0800
#define RTL8211F_GBCR_PORT_TYPE                0x0400
#define RTL8211F_GBCR_1000BT_FD                0x0200

//1000Base-T Status register
#define RTL8211F_GBSR_MS_CONF_FAULT            0x8000
#define RTL8211F_GBSR_MS_CONF_RES              0x4000
#define RTL8211F_GBSR_LOCAL_RECEIVER_STATUS    0x2000
#define RTL8211F_GBSR_REMOTE_RECEIVER_STATUS   0x1000
#define RTL8211F_GBSR_LP_1000BT_FD             0x0800
#define RTL8211F_GBSR_LP_1000BT_HD             0x0400
#define RTL8211F_GBSR_IDLE_ERR_COUNT           0x00FF

//MMD Access Control register
#define RTL8211F_MMDACR_FUNC                   0xC000
#define RTL8211F_MMDACR_FUNC_ADDR              0x0000
#define RTL8211F_MMDACR_FUNC_DATA_NO_POST_INC  0x4000
#define RTL8211F_MMDACR_FUNC_DATA_POST_INC_RW  0x8000
#define RTL8211F_MMDACR_FUNC_DATA_POST_INC_W   0xC000
#define RTL8211F_MMDACR_DEVAD                  0x001F

//1000Base-T Extended Status register
#define RTL8211F_GBESR_1000BX_FD               0x8000
#define RTL8211F_GBESR_1000BX_HD               0x4000
#define RTL8211F_GBESR_1000BT_FD               0x2000
#define RTL8211F_GBESR_1000BT_HD               0x1000

//Interrupt Enable register
#define RTL8211F_INER_JABBER                   0x0400
#define RTL8211F_INER_ALDPS_STATE              0x0200
#define RTL8211F_INER_PME                      0x0080
#define RTL8211F_INER_PHY_REG_ACCESS           0x0020
#define RTL8211F_INER_LINK_STATUS              0x0010
#define RTL8211F_INER_AN_COMPLETE              0x0008
#define RTL8211F_INER_PAGE_RECEIVED            0x0004
#define RTL8211F_INER_AN_ERROR                 0x0001

//PHY Specific Control 1 register
#define RTL8211F_PHYCR1_PHYAD_0_EN             0x2000
#define RTL8211F_PHYCR1_MDI_MODE_MANUAL_CONFIG 0x0200
#define RTL8211F_PHYCR1_MDI_MODE               0x0100
#define RTL8211F_PHYCR1_TX_CRS_EN              0x0080
#define RTL8211F_PHYCR1_PHYAD_NON_ZERO_DETECT  0x0040
#define RTL8211F_PHYCR1_PREAMBLE_CHECK_EN      0x0010
#define RTL8211F_PHYCR1_JABBER_DETECT_EN       0x0008
#define RTL8211F_PHYCR1_ALDPS_EN               0x0004

//PHY Specific Control 2 register
#define RTL8211F_PHYCR2_CLKOUT_FREQ_SEL        0x0800
#define RTL8211F_PHYCR2_CLKOUT_SSC_EN          0x0080
#define RTL8211F_PHYCR2_RXC_SSC_EN             0x0008
#define RTL8211F_PHYCR2_RXC_EN                 0x0002
#define RTL8211F_PHYCR2_CLKOUT_EN              0x0001

//PHY Specific Status register
#define RTL8211F_PHYSR_ALDPS_STATE             0x4000
#define RTL8211F_PHYSR_MDI_PLUG                0x2000
#define RTL8211F_PHYSR_NWAY_EN                 0x1000
#define RTL8211F_PHYSR_MASTER_MODE             0x0800
#define RTL8211F_PHYSR_EEE_CAPABLE             0x0100
#define RTL8211F_PHYSR_RX_FLOW_EN              0x0080
#define RTL8211F_PHYSR_TX_FLOW_EN              0x0040
#define RTL8211F_PHYSR_SPEED                   0x0030
#define RTL8211F_PHYSR_SPEED_10MBPS            0x0000
#define RTL8211F_PHYSR_SPEED_100MBPS           0x0010
#define RTL8211F_PHYSR_SPEED_1000MBPS          0x0020
#define RTL8211F_PHYSR_DUPLEX                  0x0008
#define RTL8211F_PHYSR_LINK                    0x0004
#define RTL8211F_PHYSR_MDI_CROSSOVER_STATUS    0x0002
#define RTL8211F_PHYSR_JABBER                  0x0001

//Interrupt Status register
#define RTL8211F_INSR_JABBER                   0x0400
#define RTL8211F_INSR_ALDPS_STATE              0x0200
#define RTL8211F_INSR_PME                      0x0080
#define RTL8211F_INSR_PHY_REG_ACCESS           0x0020
#define RTL8211F_INSR_LINK_STATUS              0x0010
#define RTL8211F_INSR_AN_COMPLETE              0x0008
#define RTL8211F_INSR_PAGE_RECEIVED            0x0004
#define RTL8211F_INSR_AN_ERROR                 0x0001

//Page Select register
#define RTL8211F_PAGSR_PAGE_SEL                0x0007

//PCS Control 1 register
#define RTL8211F_PC1R_CLK_STOP_EN              0x0400

//PCS Status 1 register
#define RTL8211F_PS1R_TX_LPI_RCVD              0x0800
#define RTL8211F_PS1R_RX_LPI_RCVD              0x0400
#define RTL8211F_PS1R_TX_LPI_IND               0x0200
#define RTL8211F_PS1R_RX_LPI_IND               0x0100
#define RTL8211F_PS1R_CLK_STOP_CAPABLE         0x0040

//EEE Capability register
#define RTL8211F_EEECR_1000BT_EEE              0x0004
#define RTL8211F_EEECR_100BTX_EEE              0x0002

//EEE Advertisement register
#define RTL8211F_EEEAR_1000BT_EEE              0x0004
#define RTL8211F_EEEAR_100BTX_EEE              0x0002

//EEE Link Partner Ability register
#define RTL8211F_EEELPAR_LP_1000BT_EEE         0x0004
#define RTL8211F_EEELPAR_LP_100BTX_EEE         0x0002

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//RTL8211F Ethernet PHY driver
extern const PhyDriver rtl8211fPhyDriver;

//RTL8211F related functions
error_t rtl8211fInit(NetInterface *interface);

void rtl8211fTick(NetInterface *interface);

void rtl8211fEnableIrq(NetInterface *interface);
void rtl8211fDisableIrq(NetInterface *interface);

void rtl8211fEventHandler(NetInterface *interface);

void rtl8211fWritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t rtl8211fReadPhyReg(NetInterface *interface, uint8_t address);

void rtl8211fDumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
