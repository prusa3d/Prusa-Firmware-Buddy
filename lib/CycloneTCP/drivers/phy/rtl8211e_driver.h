/**
 * @file rtl8211e_driver.h
 * @brief RTL8211E Gigabit Ethernet PHY driver
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

#ifndef _RTL8211E_DRIVER_H
#define _RTL8211E_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef RTL8211E_PHY_ADDR
   #define RTL8211E_PHY_ADDR 1
#elif (RTL8211E_PHY_ADDR < 0 || RTL8211E_PHY_ADDR > 31)
   #error RTL8211E_PHY_ADDR parameter is not valid
#endif

//RTL8211E PHY registers
#define RTL8211E_BMCR                         0x00
#define RTL8211E_BMSR                         0x01
#define RTL8211E_PHYID1                       0x02
#define RTL8211E_PHYID2                       0x03
#define RTL8211E_ANAR                         0x04
#define RTL8211E_ANLPAR                       0x05
#define RTL8211E_ANER                         0x06
#define RTL8211E_ANNPTR                       0x07
#define RTL8211E_ANNPRR                       0x08
#define RTL8211E_GBCR                         0x09
#define RTL8211E_GBSR                         0x0A
#define RTL8211E_MMDACR                       0x0D
#define RTL8211E_MMDAADR                      0x0E
#define RTL8211E_GBESR                        0x0F
#define RTL8211E_PHYCR                        0x10
#define RTL8211E_PHYSR                        0x11
#define RTL8211E_INER                         0x12
#define RTL8211E_INSR                         0x13
#define RTL8211E_RXERC                        0x18
#define RTL8211E_LDPSR                        0x1B
#define RTL8211E_EPAGSR                       0x1E
#define RTL8211E_PAGSR                        0x1F

//Basic Mode Control register
#define RTL8211E_BMCR_RESET                   0x8000
#define RTL8211E_BMCR_LOOPBACK                0x4000
#define RTL8211E_BMCR_SPEED_SEL_LSB           0x2000
#define RTL8211E_BMCR_AN_EN                   0x1000
#define RTL8211E_BMCR_POWER_DOWN              0x0800
#define RTL8211E_BMCR_ISOLATE                 0x0400
#define RTL8211E_BMCR_RESTART_AN              0x0200
#define RTL8211E_BMCR_DUPLEX_MODE             0x0100
#define RTL8211E_BMCR_COL_TEST                0x0080
#define RTL8211E_BMCR_SPEED_SEL_MSB           0x0040

//Basic Mode Status register
#define RTL8211E_BMSR_100BT4                  0x8000
#define RTL8211E_BMSR_100BTX_FD               0x4000
#define RTL8211E_BMSR_100BTX_HD               0x2000
#define RTL8211E_BMSR_10BT_FD                 0x1000
#define RTL8211E_BMSR_10BT_HD                 0x0800
#define RTL8211E_BMSR_100BT2_FD               0x0400
#define RTL8211E_BMSR_100BT2_HD               0x0200
#define RTL8211E_BMSR_EXTENDED_STATUS         0x0100
#define RTL8211E_BMSR_PREAMBLE_SUPPR          0x0040
#define RTL8211E_BMSR_AN_COMPLETE             0x0020
#define RTL8211E_BMSR_REMOTE_FAULT            0x0010
#define RTL8211E_BMSR_AN_CAPABLE              0x0008
#define RTL8211E_BMSR_LINK_STATUS             0x0004
#define RTL8211E_BMSR_JABBER_DETECT           0x0002
#define RTL8211E_BMSR_EXTENDED_CAPABLE        0x0001

//PHY Identifier 1 register
#define RTL8211E_PHYID1_OUI_MSB               0xFFFF
#define RTL8211E_PHYID1_OUI_MSB_DEFAULT       0x001C

//PHY Identifier 2 register
#define RTL8211E_PHYID2_OUI_LSB               0xFC00
#define RTL8211E_PHYID2_OUI_LSB_DEFAULT       0xC800
#define RTL8211E_PHYID2_MODEL_NUM             0x03F0
#define RTL8211E_PHYID2_MODEL_NUM_DEFAULT     0x0110
#define RTL8211E_PHYID2_REVISION_NUM          0x000F
#define RTL8211E_PHYID2_REVISION_NUM_DEFAULT  0x0005

//Auto-Negotiation Advertisement register
#define RTL8211E_ANAR_NEXT_PAGE               0x8000
#define RTL8211E_ANAR_REMOTE_FAULT            0x2000
#define RTL8211E_ANAR_ASYM_PAUSE              0x0800
#define RTL8211E_ANAR_PAUSE                   0x0400
#define RTL8211E_ANAR_100BT4                  0x0200
#define RTL8211E_ANAR_100BTX_FD               0x0100
#define RTL8211E_ANAR_100BTX_HD               0x0080
#define RTL8211E_ANAR_10BT_FD                 0x0040
#define RTL8211E_ANAR_10BT_HD                 0x0020
#define RTL8211E_ANAR_SELECTOR                0x001F
#define RTL8211E_ANAR_SELECTOR_DEFAULT        0x0001

//Auto-Negotiation Link Partner Ability register
#define RTL8211E_ANLPAR_NEXT_PAGE             0x8000
#define RTL8211E_ANLPAR_ACK                   0x4000
#define RTL8211E_ANLPAR_REMOTE_FAULT          0x2000
#define RTL8211E_ANLPAR_ASYM_PAUSE            0x0800
#define RTL8211E_ANLPAR_PAUSE                 0x0400
#define RTL8211E_ANLPAR_100BT4                0x0200
#define RTL8211E_ANLPAR_100BTX_FD             0x0100
#define RTL8211E_ANLPAR_100BTX_HD             0x0080
#define RTL8211E_ANLPAR_10BT_FD               0x0040
#define RTL8211E_ANLPAR_10BT_HD               0x0020
#define RTL8211E_ANLPAR_SELECTOR              0x001F
#define RTL8211E_ANLPAR_SELECTOR_DEFAULT      0x0001

//Auto-Negotiation Expansion register
#define RTL8211E_ANER_PAR_DETECT_FAULT        0x0010
#define RTL8211E_ANER_LP_NEXT_PAGE_ABLE       0x0008
#define RTL8211E_ANER_NEXT_PAGE_ABLE          0x0004
#define RTL8211E_ANER_PAGE_RECEIVED           0x0002
#define RTL8211E_ANER_LP_AN_ABLE              0x0001

//Auto-Negotiation Next Page Transmit register
#define RTL8211E_ANNPTR_NEXT_PAGE             0x8000
#define RTL8211E_ANNPTR_MSG_PAGE              0x2000
#define RTL8211E_ANNPTR_ACK2                  0x1000
#define RTL8211E_ANNPTR_TOGGLE                0x0800
#define RTL8211E_ANNPTR_MESSAGE               0x07FF

//Auto-Negotiation Next Page Receive register
#define RTL8211E_ANNPRR_NEXT_PAGE             0x8000
#define RTL8211E_ANNPRR_ACK                   0x4000
#define RTL8211E_ANNPRR_MSG_PAGE              0x2000
#define RTL8211E_ANNPRR_ACK2                  0x1000
#define RTL8211E_ANNPRR_TOGGLE                0x0800
#define RTL8211E_ANNPRR_MESSAGE               0x07FF

//1000Base-T Control register
#define RTL8211E_GBCR_TEST_MODE               0xE000
#define RTL8211E_GBCR_MS_MAN_CONF_EN          0x1000
#define RTL8211E_GBCR_MS_MAN_CONF_VAL         0x0800
#define RTL8211E_GBCR_PORT_TYPE               0x0400
#define RTL8211E_GBCR_1000BT_FD               0x0200

//1000Base-T Status register
#define RTL8211E_GBSR_MS_CONF_FAULT           0x8000
#define RTL8211E_GBSR_MS_CONF_RES             0x4000
#define RTL8211E_GBSR_LOCAL_RECEIVER_STATUS   0x2000
#define RTL8211E_GBSR_REMOTE_RECEIVER_STATUS  0x1000
#define RTL8211E_GBSR_LP_1000BT_FD            0x0800
#define RTL8211E_GBSR_LP_1000BT_HD            0x0400
#define RTL8211E_GBSR_IDLE_ERR_COUNT          0x00FF

//MMD Access Control register
#define RTL8211E_MMDACR_FUNC                  0xC000
#define RTL8211E_MMDACR_FUNC_ADDR             0x0000
#define RTL8211E_MMDACR_FUNC_DATA_NO_POST_INC 0x4000
#define RTL8211E_MMDACR_FUNC_DATA_POST_INC_RW 0x8000
#define RTL8211E_MMDACR_FUNC_DATA_POST_INC_W  0xC000
#define RTL8211E_MMDACR_DEVAD                 0x001F

//1000Base-T Extended Status register
#define RTL8211E_GBESR_1000BX_FD              0x8000
#define RTL8211E_GBESR_1000BX_HD              0x4000
#define RTL8211E_GBESR_1000BT_FD              0x2000
#define RTL8211E_GBESR_1000BT_HD              0x1000

//PHY Specific Control register
#define RTL8211E_PHYCR_RXC_DIS                0x8000
#define RTL8211E_PHYCR_FPR_FAIL_SEL           0x7000
#define RTL8211E_PHYCR_ASSERT_CRS_ON_TX       0x0800
#define RTL8211E_PHYCR_FORCE_LINK_GOOD        0x0400
#define RTL8211E_PHYCR_CROSSOVER_EN           0x0040
#define RTL8211E_PHYCR_MDI_MODE               0x0020
#define RTL8211E_PHYCR_CLK125_DIS             0x0010
#define RTL8211E_PHYCR_JABBER_DIS             0x0001

//PHY Specific Status register
#define RTL8211E_PHYSR_SPEED                  0xC000
#define RTL8211E_PHYSR_SPEED_10MBPS           0x0000
#define RTL8211E_PHYSR_SPEED_100MBPS          0x4000
#define RTL8211E_PHYSR_SPEED_1000MBPS         0x8000
#define RTL8211E_PHYSR_DUPLEX                 0x2000
#define RTL8211E_PHYSR_PAGE_RECEIVED          0x1000
#define RTL8211E_PHYSR_SPEED_DUPLEX_RESOLVED  0x0800
#define RTL8211E_PHYSR_LINK                   0x0400
#define RTL8211E_PHYSR_MDI_CROSSOVER_STATUS   0x0040
#define RTL8211E_PHYSR_PRE_LINKOK             0x0002
#define RTL8211E_PHYSR_JABBER                 0x0001

//Interrupt Enable register
#define RTL8211E_INER_AN_ERROR                0x8000
#define RTL8211E_INER_PAGE_RECEIVED           0x1000
#define RTL8211E_INER_AN_COMPLETE             0x0800
#define RTL8211E_INER_LINK_STATUS             0x0400
#define RTL8211E_INER_SYMBOL_ERROR            0x0200
#define RTL8211E_INER_FALSE_CARRIER           0x0100
#define RTL8211E_INER_JABBER                  0x0001

//Interrupt Status register
#define RTL8211E_INSR_AN_ERROR                0x8000
#define RTL8211E_INSR_PAGE_RECEIVED           0x1000
#define RTL8211E_INSR_AN_COMPLETE             0x0800
#define RTL8211E_INSR_LINK_STATUS             0x0400
#define RTL8211E_INSR_SYMBOL_ERROR            0x0200
#define RTL8211E_INSR_FALSE_CARRIER           0x0100
#define RTL8211E_INSR_JABBER                  0x0001

//Link Down Power Saving register
#define RTL8211E_LDPSR_POWER_SAVE_MODE        0x0001

//Extension Page Select register
#define RTL8211E_EPAGSR_EXT_PAGE_SEL          0x00FF

//Page Select register
#define RTL8211E_PAGSR_PAGE_SEL               0x0007

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//RTL8211E Ethernet PHY driver
extern const PhyDriver rtl8211ePhyDriver;

//RTL8211E related functions
error_t rtl8211eInit(NetInterface *interface);

void rtl8211eTick(NetInterface *interface);

void rtl8211eEnableIrq(NetInterface *interface);
void rtl8211eDisableIrq(NetInterface *interface);

void rtl8211eEventHandler(NetInterface *interface);

void rtl8211eWritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t rtl8211eReadPhyReg(NetInterface *interface, uint8_t address);

void rtl8211eDumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
