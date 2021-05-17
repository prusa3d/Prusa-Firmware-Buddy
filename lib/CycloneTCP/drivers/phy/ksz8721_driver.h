/**
 * @file ksz8721_driver.h
 * @brief KSZ8721 Ethernet PHY driver
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

#ifndef _KSZ8721_DRIVER_H
#define _KSZ8721_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef KSZ8721_PHY_ADDR
   #define KSZ8721_PHY_ADDR 1
#elif (KSZ8721_PHY_ADDR < 0 || KSZ8721_PHY_ADDR > 31)
   #error KSZ8721_PHY_ADDR parameter is not valid
#endif

//KSZ8721 PHY registers
#define KSZ8721_BMCR                       0x00
#define KSZ8721_BMSR                       0x01
#define KSZ8721_PHYID1                     0x02
#define KSZ8721_PHYID2                     0x03
#define KSZ8721_ANAR                       0x04
#define KSZ8721_ANLPAR                     0x05
#define KSZ8721_ANER                       0x06
#define KSZ8721_ANNPR                      0x07
#define KSZ8721_ANLPNPR                    0x08
#define KSZ8721_RXERCTR                    0x15
#define KSZ8721_ICSR                       0x1B
#define KSZ8721_PHYCON                     0x1F

//Basic Control register
#define KSZ8721_BMCR_RESET                 0x8000
#define KSZ8721_BMCR_LOOPBACK              0x4000
#define KSZ8721_BMCR_SPEED_SEL             0x2000
#define KSZ8721_BMCR_AN_EN                 0x1000
#define KSZ8721_BMCR_POWER_DOWN            0x0800
#define KSZ8721_BMCR_ISOLATE               0x0400
#define KSZ8721_BMCR_RESTART_AN            0x0200
#define KSZ8721_BMCR_DUPLEX_MODE           0x0100
#define KSZ8721_BMCR_COL_TEST              0x0080
#define KSZ8721_BMCR_TX_DIS                0x0001

//Basic Status register
#define KSZ8721_BMSR_100BT4                0x8000
#define KSZ8721_BMSR_100BTX_FD             0x4000
#define KSZ8721_BMSR_100BTX_HD             0x2000
#define KSZ8721_BMSR_10BT_FD               0x1000
#define KSZ8721_BMSR_10BT_HD               0x0800
#define KSZ8721_BMSR_NO_PREAMBLE           0x0040
#define KSZ8721_BMSR_AN_COMPLETE           0x0020
#define KSZ8721_BMSR_REMOTE_FAULT          0x0010
#define KSZ8721_BMSR_AN_CAPABLE            0x0008
#define KSZ8721_BMSR_LINK_STATUS           0x0004
#define KSZ8721_BMSR_JABBER_DETECT         0x0002
#define KSZ8721_BMSR_EXTENDED_CAPABLE      0x0001

//PHY Identifier 1 register
#define KSZ8721_PHYID1_PHY_ID_MSB          0xFFFF
#define KSZ8721_PHYID1_PHY_ID_MSB_DEFAULT  0x0022

//PHY Identifier 2 register
#define KSZ8721_PHYID2_PHY_ID_LSB          0xFC00
#define KSZ8721_PHYID2_PHY_ID_LSB_DEFAULT  0x1400
#define KSZ8721_PHYID2_MODEL_NUM           0x03F0
#define KSZ8721_PHYID2_MODEL_NUM_DEFAULT   0x0210
#define KSZ8721_PHYID2_REVISION_NUM        0x000F

//Auto-Negotiation Advertisement register
#define KSZ8721_ANAR_NEXT_PAGE             0x8000
#define KSZ8721_ANAR_REMOTE_FAULT          0x2000
#define KSZ8721_ANAR_PAUSE                 0x0C00
#define KSZ8721_ANAR_100BT4                0x0200
#define KSZ8721_ANAR_100BTX_FD             0x0100
#define KSZ8721_ANAR_100BTX_HD             0x0080
#define KSZ8721_ANAR_10BT_FD               0x0040
#define KSZ8721_ANAR_10BT_HD               0x0020
#define KSZ8721_ANAR_SELECTOR              0x001F
#define KSZ8721_ANAR_SELECTOR_DEFAULT      0x0001

//Auto-Negotiation Link Partner Ability register
#define KSZ8721_ANLPAR_NEXT_PAGE           0x8000
#define KSZ8721_ANLPAR_ACK                 0x4000
#define KSZ8721_ANLPAR_REMOTE_FAULT        0x2000
#define KSZ8721_ANLPAR_PAUSE               0x0C00
#define KSZ8721_ANLPAR_100BT4              0x0200
#define KSZ8721_ANLPAR_100BTX_FD           0x0100
#define KSZ8721_ANLPAR_100BTX_HD           0x0080
#define KSZ8721_ANLPAR_10BT_FD             0x0040
#define KSZ8721_ANLPAR_10BT_HD             0x0020
#define KSZ8721_ANLPAR_SELECTOR            0x001F
#define KSZ8721_ANLPAR_SELECTOR_DEFAULT    0x0001

//Auto-Negotiation Expansion register
#define KSZ8721_ANER_PAR_DETECT_FAULT      0x0010
#define KSZ8721_ANER_LP_NEXT_PAGE_ABLE     0x0008
#define KSZ8721_ANER_NEXT_PAGE_ABLE        0x0004
#define KSZ8721_ANER_PAGE_RECEIVED         0x0002
#define KSZ8721_ANER_LP_AN_ABLE            0x0001

//Auto-Negotiation Next Page register
#define KSZ8721_ANNPR_NEXT_PAGE            0x8000
#define KSZ8721_ANNPR_MSG_PAGE             0x2000
#define KSZ8721_ANNPR_ACK2                 0x1000
#define KSZ8721_ANNPR_TOGGLE               0x0800
#define KSZ8721_ANNPR_MESSAGE              0x07FF

//Link Partner Next Page Ability register
#define KSZ8721_ANLPNPR_NEXT_PAGE          0x8000
#define KSZ8721_ANLPNPR_ACK                0x4000
#define KSZ8721_ANLPNPR_MSG_PAGE           0x2000
#define KSZ8721_ANLPNPR_ACK2               0x1000
#define KSZ8721_ANLPNPR_TOGGLE             0x0800
#define KSZ8721_ANLPNPR_MESSAGE            0x07FF

//Interrupt Control/Status register
#define KSZ8721_ICSR_JABBER_IE             0x8000
#define KSZ8721_ICSR_RECEIVE_ERROR_IE      0x4000
#define KSZ8721_ICSR_PAGE_RECEIVED_IE      0x2000
#define KSZ8721_ICSR_PAR_DETECT_FAULT_IE   0x1000
#define KSZ8721_ICSR_LP_ACK_IE             0x0800
#define KSZ8721_ICSR_LINK_DOWN_IE          0x0400
#define KSZ8721_ICSR_REMOTE_FAULT_IE       0x0200
#define KSZ8721_ICSR_LINK_UP_IE            0x0100
#define KSZ8721_ICSR_JABBER_IF             0x0080
#define KSZ8721_ICSR_RECEIVE_ERROR_IF      0x0040
#define KSZ8721_ICSR_PAGE_RECEIVED_IF      0x0020
#define KSZ8721_ICSR_PAR_DETECT_FAULT_IF   0x0010
#define KSZ8721_ICSR_LP_ACK_IF             0x0008
#define KSZ8721_ICSR_LINK_DOWN_IF          0x0004
#define KSZ8721_ICSR_REMOTE_FAULT_IF       0x0002
#define KSZ8721_ICSR_LINK_UP_IF            0x0001

//100BASE-TX PHY Control register
#define KSZ8721_PHYCON_PAIR_SWAP_DIS       0x2000
#define KSZ8721_PHYCON_ENERGY_DETECT       0x1000
#define KSZ8721_PHYCON_FORCE_LINK          0x0800
#define KSZ8721_PHYCON_POWER_SAVING        0x0400
#define KSZ8721_PHYCON_INT_LEVEL           0x0200
#define KSZ8721_PHYCON_JABBER_EN           0x0100
#define KSZ8721_PHYCON_AN_COMPLETE         0x0080
#define KSZ8721_PHYCON_PAUSE_EN            0x0040
#define KSZ8721_PHYCON_PHY_ISOLATE         0x0020
#define KSZ8721_PHYCON_OP_MODE             0x001C
#define KSZ8721_PHYCON_OP_MODE_AN          0x0000
#define KSZ8721_PHYCON_OP_MODE_10BT_HD     0x0004
#define KSZ8721_PHYCON_OP_MODE_100BTX_HD   0x0008
#define KSZ8721_PHYCON_OP_MODE_10BT_FD     0x0014
#define KSZ8721_PHYCON_OP_MODE_100BTX_FD   0x0018
#define KSZ8721_PHYCON_OP_MODE_ISOLATE     0x001C
#define KSZ8721_PHYCON_SQE_TEST_EN         0x0002
#define KSZ8721_PHYCON_DATA_SCRAMBLING_DIS 0x0001

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//KSZ8721 Ethernet PHY driver
extern const PhyDriver ksz8721PhyDriver;

//KSZ8721 related functions
error_t ksz8721Init(NetInterface *interface);

void ksz8721Tick(NetInterface *interface);

void ksz8721EnableIrq(NetInterface *interface);
void ksz8721DisableIrq(NetInterface *interface);

void ksz8721EventHandler(NetInterface *interface);

void ksz8721WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t ksz8721ReadPhyReg(NetInterface *interface, uint8_t address);

void ksz8721DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
