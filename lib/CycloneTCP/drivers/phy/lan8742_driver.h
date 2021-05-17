/**
 * @file lan8742_driver.h
 * @brief LAN8742 Ethernet PHY driver
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

#ifndef _LAN8742_DRIVER_H
#define _LAN8742_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef LAN8742_PHY_ADDR
   #define LAN8742_PHY_ADDR 0
#elif (LAN8742_PHY_ADDR < 0 || LAN8742_PHY_ADDR > 31)
   #error LAN8742_PHY_ADDR parameter is not valid
#endif

//LAN8742 PHY registers
#define LAN8742_BMCR                         0x00
#define LAN8742_BMSR                         0x01
#define LAN8742_PHYID1                       0x02
#define LAN8742_PHYID2                       0x03
#define LAN8742_ANAR                         0x04
#define LAN8742_ANLPAR                       0x05
#define LAN8742_ANER                         0x06
#define LAN8742_ANNPR                        0x07
#define LAN8742_ANLPNPR                      0x08
#define LAN8742_MMDACR                       0x0D
#define LAN8742_MMDAADR                      0x0E
#define LAN8742_ENCTR                        0x10
#define LAN8742_MCSR                         0x11
#define LAN8742_SMR                          0x12
#define LAN8742_TDRPDCR                      0x18
#define LAN8742_TDRCSR                       0x19
#define LAN8742_SECR                         0x1A
#define LAN8742_SCSIR                        0x1B
#define LAN8742_CLR                          0x1C
#define LAN8742_ISR                          0x1D
#define LAN8742_IMR                          0x1E
#define LAN8742_PSCSR                        0x1F

//Basic Control register
#define LAN8742_BMCR_RESET                   0x8000
#define LAN8742_BMCR_LOOPBACK                0x4000
#define LAN8742_BMCR_SPEED_SEL               0x2000
#define LAN8742_BMCR_AN_EN                   0x1000
#define LAN8742_BMCR_POWER_DOWN              0x0800
#define LAN8742_BMCR_ISOLATE                 0x0400
#define LAN8742_BMCR_RESTART_AN              0x0200
#define LAN8742_BMCR_DUPLEX_MODE             0x0100

//Basic Status register
#define LAN8742_BMSR_100BT4                  0x8000
#define LAN8742_BMSR_100BTX_FD               0x4000
#define LAN8742_BMSR_100BTX_HD               0x2000
#define LAN8742_BMSR_10BT_FD                 0x1000
#define LAN8742_BMSR_10BT_HD                 0x0800
#define LAN8742_BMSR_100BT2_FD               0x0400
#define LAN8742_BMSR_100BT2_HD               0x0200
#define LAN8742_BMSR_EXTENDED_STATUS         0x0100
#define LAN8742_BMSR_AN_COMPLETE             0x0020
#define LAN8742_BMSR_REMOTE_FAULT            0x0010
#define LAN8742_BMSR_AN_CAPABLE              0x0008
#define LAN8742_BMSR_LINK_STATUS             0x0004
#define LAN8742_BMSR_JABBER_DETECT           0x0002
#define LAN8742_BMSR_EXTENDED_CAPABLE        0x0001

//PHY Identifier 1 register
#define LAN8742_PHYID1_PHY_ID_MSB            0xFFFF
#define LAN8742_PHYID1_PHY_ID_MSB_DEFAULT    0x0007

//PHY Identifier 2 register
#define LAN8742_PHYID2_PHY_ID_LSB            0xFC00
#define LAN8742_PHYID2_PHY_ID_LSB_DEFAULT    0xC000
#define LAN8742_PHYID2_MODEL_NUM             0x03F0
#define LAN8742_PHYID2_MODEL_NUM_DEFAULT     0x0130
#define LAN8742_PHYID2_REVISION_NUM          0x000F

//Auto-Negotiation Advertisement register
#define LAN8742_ANAR_NEXT_PAGE               0x8000
#define LAN8742_ANAR_REMOTE_FAULT            0x2000
#define LAN8742_ANAR_PAUSE                   0x0C00
#define LAN8742_ANAR_100BTX_FD               0x0100
#define LAN8742_ANAR_100BTX_HD               0x0080
#define LAN8742_ANAR_10BT_FD                 0x0040
#define LAN8742_ANAR_10BT_HD                 0x0020
#define LAN8742_ANAR_SELECTOR                0x001F
#define LAN8742_ANAR_SELECTOR_DEFAULT        0x0001

//Auto-Negotiation Link Partner Ability register
#define LAN8742_ANLPAR_NEXT_PAGE             0x8000
#define LAN8742_ANLPAR_ACK                   0x4000
#define LAN8742_ANLPAR_REMOTE_FAULT          0x2000
#define LAN8742_ANLPAR_PAUSE                 0x0400
#define LAN8742_ANLPAR_100BT4                0x0200
#define LAN8742_ANLPAR_100BTX_FD             0x0100
#define LAN8742_ANLPAR_100BTX_HD             0x0080
#define LAN8742_ANLPAR_10BT_FD               0x0040
#define LAN8742_ANLPAR_10BT_HD               0x0020
#define LAN8742_ANLPAR_SELECTOR              0x001F
#define LAN8742_ANLPAR_SELECTOR_DEFAULT      0x0001

//Auto-Negotiation Expansion register
#define LAN8742_ANER_RECEIVE_NP_LOC_ABLE     0x0040
#define LAN8742_ANER_RECEIVE_NP_STOR_LOC     0x0020
#define LAN8742_ANER_PAR_DETECT_FAULT        0x0010
#define LAN8742_ANER_LP_NEXT_PAGE_ABLE       0x0008
#define LAN8742_ANER_NEXT_PAGE_ABLE          0x0004
#define LAN8742_ANER_PAGE_RECEIVED           0x0002
#define LAN8742_ANER_LP_AN_ABLE              0x0001

//Auto Negotiation Next Page TX register
#define LAN8742_ANNPR_NEXT_PAGE              0x8000
#define LAN8742_ANNPR_MSG_PAGE               0x2000
#define LAN8742_ANNPR_ACK2                   0x1000
#define LAN8742_ANNPR_TOGGLE                 0x0800
#define LAN8742_ANNPR_MESSAGE                0x07FF

//Auto Negotiation Next Page RX register
#define LAN8742_ANLPNPR_NEXT_PAGE            0x8000
#define LAN8742_ANLPNPR_ACK                  0x4000
#define LAN8742_ANLPNPR_MSG_PAGE             0x2000
#define LAN8742_ANLPNPR_ACK2                 0x1000
#define LAN8742_ANLPNPR_TOGGLE               0x0800
#define LAN8742_ANLPNPR_MESSAGE              0x07FF

//MMD Access Control register
#define LAN8742_MMDACR_FUNC                  0xC000
#define LAN8742_MMDACR_FUNC_ADDR             0x0000
#define LAN8742_MMDACR_FUNC_DATA_NO_POST_INC 0x4000
#define LAN8742_MMDACR_DEVAD                 0x001F

//EDPD NLP/Crossover Time Configuration register
#define LAN8742_ENCTR_EDPD_TX_NLP_EN         0x8000
#define LAN8742_ENCTR_EDPD_TX_NLP_ITS        0x6000
#define LAN8742_ENCTR_EDPD_RX_NLP_WAKE_EN    0x1000
#define LAN8742_ENCTR_EDPD_RX_NLP_MIDS       0x0C00
#define LAN8742_ENCTR_EDPD_EXT_CROSSOVER     0x0002
#define LAN8742_ENCTR_EXT_CROSSOVER_TIME     0x0001

//Mode Control/Status register
#define LAN8742_MCSR_EDPWRDOWN               0x2000
#define LAN8742_MCSR_FARLOOPBACK             0x0200
#define LAN8742_MCSR_ALTINT                  0x0040
#define LAN8742_MCSR_ENERGYON                0x0002

//Special Modes register
#define LAN8742_SMR_MODE                     0x00E0
#define LAN8742_SMR_PHYAD                    0x001F

//TDR Patterns/Delay Control register
#define LAN8742_TDRPDCR_DELAY_IN             0x8000
#define LAN8742_TDRPDCR_LINE_BREAK_COUNT     0x7000
#define LAN8742_TDRPDCR_PATTERN_HIGH         0x0FC0
#define LAN8742_TDRPDCR_PATTERN_LOW          0x003F

//TDR Control/Status register
#define LAN8742_TDRCSR_TDR_EN                0x8000
#define LAN8742_TDRCSR_AD_FILTER_EN          0x4000
#define LAN8742_TDRCSR_CH_CABLE_TYPE         0x0600
#define LAN8742_TDRCSR_CH_STATUS             0x0100
#define LAN8742_TDRCSR_CH_LENGTH             0x00FF

//Symbol Error Counter register
#define LAN8742_SECR_SYM_ERR_CNT             0xFFFF

//Special Control/Status Indication register
#define LAN8742_SCSIR_AMDIXCTRL              0x8000
#define LAN8742_SCSIR_CH_SELECT              0x2000
#define LAN8742_SCSIR_SQEOFF                 0x0800
#define LAN8742_SCSIR_XPOL                   0x0010

//Cable Length register
#define LAN8742_CLR_CBLN                     0xF000

//Interrupt Source Flag register
#define LAN8742_ISR_WOL                      0x0100
#define LAN8742_ISR_ENERGYON                 0x0080
#define LAN8742_ISR_AN_COMPLETE              0x0040
#define LAN8742_ISR_REMOTE_FAULT             0x0020
#define LAN8742_ISR_LINK_DOWN                0x0010
#define LAN8742_ISR_AN_LP_ACK                0x0008
#define LAN8742_ISR_PAR_DETECT_FAULT         0x0004
#define LAN8742_ISR_AN_PAGE_RECEIVED         0x0002

//Interrupt Mask register
#define LAN8742_IMR_WOL                      0x0100
#define LAN8742_IMR_ENERGYON                 0x0080
#define LAN8742_IMR_AN_COMPLETE              0x0040
#define LAN8742_IMR_REMOTE_FAULT             0x0020
#define LAN8742_IMR_LINK_DOWN                0x0010
#define LAN8742_IMR_AN_LP_ACK                0x0008
#define LAN8742_IMR_PAR_DETECT_FAULT         0x0004
#define LAN8742_IMR_AN_PAGE_RECEIVED         0x0002

//PHY Special Control/Status register
#define LAN8742_PSCSR_AUTODONE               0x1000
#define LAN8742_PSCSR_HCDSPEED               0x001C
#define LAN8742_PSCSR_HCDSPEED_10BT_HD       0x0004
#define LAN8742_PSCSR_HCDSPEED_100BTX_HD     0x0008
#define LAN8742_PSCSR_HCDSPEED_10BT_FD       0x0014
#define LAN8742_PSCSR_HCDSPEED_100BTX_FD     0x0018

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//LAN8742 Ethernet PHY driver
extern const PhyDriver lan8742PhyDriver;

//LAN8742 related functions
error_t lan8742Init(NetInterface *interface);

void lan8742Tick(NetInterface *interface);

void lan8742EnableIrq(NetInterface *interface);
void lan8742DisableIrq(NetInterface *interface);

void lan8742EventHandler(NetInterface *interface);

void lan8742WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t lan8742ReadPhyReg(NetInterface *interface, uint8_t address);

void lan8742DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
