/**
 * @file ip101_driver.h
 * @brief IC+ IP101 Ethernet PHY driver
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

#ifndef _IP101_DRIVER_H
#define _IP101_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef IP101_PHY_ADDR
   #define IP101_PHY_ADDR 1
#elif (IP101_PHY_ADDR < 0 || IP101_PHY_ADDR > 31)
   #error IP101_PHY_ADDR parameter is not valid
#endif

//IP101 PHY registers
#define IP101_BMCR                         0x00
#define IP101_BMSR                         0x01
#define IP101_PHYID1                       0x02
#define IP101_PHYID2                       0x03
#define IP101_ANAR                         0x04
#define IP101_ANLPAR                       0x05
#define IP101_ANER                         0x06
#define IP101_ANNPR                        0x07
#define IP101_ANLPNPR                      0x08
#define IP101_MMDACR                       0x0D
#define IP101_MMDAADR                      0x0E
#define IP101_PHYSCR                       0x10
#define IP101_ICSR                         0x11
#define IP101_PHYSMR                       0x12
#define IP101_IOSCR                        0x1D
#define IP101_PHYMCSSR                     0x1E

//Control register
#define IP101_BMCR_RESET                   0x8000
#define IP101_BMCR_LOOPBACK                0x4000
#define IP101_BMCR_SPEED_SEL               0x2000
#define IP101_BMCR_AN_EN                   0x1000
#define IP101_BMCR_POWER_DOWN              0x0800
#define IP101_BMCR_ISOLATE                 0x0400
#define IP101_BMCR_RESTART_AN              0x0200
#define IP101_BMCR_DUPLEX_MODE             0x0100
#define IP101_BMCR_COL_TEST                0x0080

//Basic Status register
#define IP101_BMSR_100BT4                  0x8000
#define IP101_BMSR_100BTX_FD               0x4000
#define IP101_BMSR_100BTX_HD               0x2000
#define IP101_BMSR_10BT_FD                 0x1000
#define IP101_BMSR_10BT_HD                 0x0800
#define IP101_BMSR_MF_PREAMBLE_SUPPR       0x0040
#define IP101_BMSR_AN_COMPLETE             0x0020
#define IP101_BMSR_REMOTE_FAULT            0x0010
#define IP101_BMSR_AN_CAPABLE              0x0008
#define IP101_BMSR_LINK_STATUS             0x0004
#define IP101_BMSR_JABBER_DETECT           0x0002
#define IP101_BMSR_EXTENDED_CAPABLE        0x0001

//PHY Identifier 1 register
#define IP101_PHYID1_PHY_ID_MSB            0xFFFF
#define IP101_PHYID1_PHY_ID_MSB_DEFAULT    0x0243

//PHY Identifier 2 register
#define IP101_PHYID2__DEFAULT              0xC54

//Auto-Negotiation Advertisement register
#define IP101_ANAR_NEXT_PAGE               0x8000
#define IP101_ANAR_REMOTE_FAULT            0x2000
#define IP101_ANAR_ASYM_PAUSE              0x0800
#define IP101_ANAR_PAUSE                   0x0400
#define IP101_ANAR_100BT4                  0x0200
#define IP101_ANAR_100BTX_FD               0x0100
#define IP101_ANAR_100BTX_HD               0x0080
#define IP101_ANAR_10BT_FD                 0x0040
#define IP101_ANAR_10BT_HD                 0x0020
#define IP101_ANAR_SELECTOR                0x001F
#define IP101_ANAR_SELECTOR_DEFAULT        0x0001

//Auto-Negotiation Link Partner Ability register
#define IP101_ANLPAR_NEXT_PAGE             0x8000
#define IP101_ANLPAR_ACK                   0x4000
#define IP101_ANLPAR_REMOTE_FAULT          0x2000
#define IP101_ANLPAR_ASYM_PAUSE            0x0800
#define IP101_ANLPAR_PAUSE                 0x0400
#define IP101_ANLPAR_100BT4                0x0200
#define IP101_ANLPAR_100BTX_FD             0x0100
#define IP101_ANLPAR_100BTX_HD             0x0080
#define IP101_ANLPAR_10BT_FD               0x0040
#define IP101_ANLPAR_10BT_HD               0x0020
#define IP101_ANLPAR_SELECTOR              0x001F
#define IP101_ANLPAR_SELECTOR_DEFAULT      0x0001

//Auto-Negotiation Expansion register
#define IP101_ANER_MLF                     0x0010
#define IP101_ANER_LP_NP_ABLE              0x0008
#define IP101_ANER_NP_ABLE                 0x0004
#define IP101_ANER_PAGE_RX                 0x0002
#define IP101_ANER_LP_AN_ABLE              0x0001

//Auto-Negotiation Next Page Transmit register
#define IP101_ANNPR_NEXT_PAGE              0x8000
#define IP101_ANNPR_MSG_PAGE               0x2000
#define IP101_ANNPR_ACK2                   0x1000
#define IP101_ANNPR_TOGGLE                 0x0800
#define IP101_ANNPR_MESSAGE                0x07FF

//Auto-Negotiation Link Partner Next Page register
#define IP101_ANLPNPR_NEXT_PAGE            0x8000
#define IP101_ANLPNPR_ACK                  0x4000
#define IP101_ANLPNPR_MSG_PAGE             0x2000
#define IP101_ANLPNPR_ACK2                 0x1000
#define IP101_ANLPNPR_TOGGLE               0x0800
#define IP101_ANLPNPR_MESSAGE              0x07FF

//MMD Access Control register
#define IP101_MMDACR_FUNC                  0xC000
#define IP101_MMDACR_FUNC_ADDR             0x0000
#define IP101_MMDACR_FUNC_DATA_NO_POST_INC 0x4000
#define IP101_MMDACR_FUNC_DATA_POST_INC_RW 0x8000
#define IP101_MMDACR_FUNC_DATA_POST_INC_W  0xC000
#define IP101_MMDACR_DEVAD                 0x001F

//PHY Specific Control register
#define IP101_PHYSCR_RMII_V10              0x2000
#define IP101_PHYSCR_RMII_V12              0x1000
#define IP101_PHYSCR_AUTO_MDIX_DIS         0x0800
#define IP101_PHYSCR_JABBER_EN             0x0200
#define IP101_PHYSCR_FEF_DIS               0x0100
#define IP101_PHYSCR_NWAY_PSAVE_DIS        0x0080
#define IP101_PHYSCR_BYPASS_DSP_RESET      0x0020
#define IP101_PHYSCR_REPEATER_MODE         0x0004
#define IP101_PHYSCR_LDPS_EN               0x0002
#define IP101_PHYSCR_ANALOG_OFF            0x0001

//Interrupt Control/Status register
#define IP101_ICSR_INTR_EN                 0x8000
#define IP101_ICSR_ALL_MASK                0x0800
#define IP101_ICSR_SPEED_MASK              0x0400
#define IP101_ICSR_DUPLEX_MASK             0x0200
#define IP101_ICSR_LINK_MASK               0x0100
#define IP101_ICSR_INTR_STATUS             0x0008
#define IP101_ICSR_SPEED_CHANGE            0x0004
#define IP101_ICSR_DUPLEX_CHANGE           0x0002
#define IP101_ICSR_LINK_CHANGE             0x0001

//PHY Status Monitoring register
#define IP101_PHYSMR_SPEED                 0x4000
#define IP101_PHYSMR_DUPLEX                0x2000
#define IP101_PHYSMR_AN_COMPLETE           0x0800
#define IP101_PHYSMR_LINK_UP               0x0400
#define IP101_PHYSMR_MDIX                  0x0200
#define IP101_PHYSMR_POLARITY              0x0100
#define IP101_PHYSMR_JABBER                0x0080
#define IP101_PHYSMR_AN_ARBIT_STATE        0x000F

//Digital I/O Specific Control register
#define IP101_IOSCR_RMII_WITH_ER           0x0080
#define IP101_IOSCR_SEL_INTR32             0x0004

//PHY MDI/MDIX Control and Specific Status register
#define IP101_PHYMCSSR_LINK_UP             0x0100
#define IP101_PHYMCSSR_FORCE_MDIX          0x0008
#define IP101_PHYMCSSR_OP_MODE             0x0007
#define IP101_PHYMCSSR_OP_MODE_LINK_OFF    0x0000
#define IP101_PHYMCSSR_OP_MODE_10M_HD      0x0001
#define IP101_PHYMCSSR_OP_MODE_100M_HD     0x0002
#define IP101_PHYMCSSR_OP_MODE_10M_FD      0x0005
#define IP101_PHYMCSSR_OP_MODE_100M_FD     0x0006

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//IP101 Ethernet PHY driver
extern const PhyDriver ip101PhyDriver;

//IP101 related functions
error_t ip101Init(NetInterface *interface);

void ip101Tick(NetInterface *interface);

void ip101EnableIrq(NetInterface *interface);
void ip101DisableIrq(NetInterface *interface);

void ip101EventHandler(NetInterface *interface);

void ip101WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t ip101ReadPhyReg(NetInterface *interface, uint8_t address);

void ip101DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
