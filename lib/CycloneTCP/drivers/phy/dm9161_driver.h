/**
 * @file dm9161_driver.h
 * @brief DM9161 Ethernet PHY driver
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

#ifndef _DM9161_DRIVER_H
#define _DM9161_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef DM9161_PHY_ADDR
   #define DM9161_PHY_ADDR 0
#elif (DM9161_PHY_ADDR < 0 || DM9161_PHY_ADDR > 31)
   #error DM9161_PHY_ADDR parameter is not valid
#endif

//DM9161 PHY registers
#define DM9161_BMCR                          0x00
#define DM9161_BMSR                          0x01
#define DM9161_PHYID1                        0x02
#define DM9161_PHYID2                        0x03
#define DM9161_ANAR                          0x04
#define DM9161_ANLPAR                        0x05
#define DM9161_ANER                          0x06
#define DM9161_DSCR                          0x10
#define DM9161_DSCSR                         0x11
#define DM9161_10BTCSR                       0x12
#define DM9161_MDINTR                        0x15
#define DM9161_RECR                          0x16
#define DM9161_DISCR                         0x17
#define DM9161_RLSR                          0x18

//Basic Mode Control register
#define DM9161_BMCR_RESET                    0x8000
#define DM9161_BMCR_LOOPBACK                 0x4000
#define DM9161_BMCR_SPEED_SEL                0x2000
#define DM9161_BMCR_AN_EN                    0x1000
#define DM9161_BMCR_POWER_DOWN               0x0800
#define DM9161_BMCR_ISOLATE                  0x0400
#define DM9161_BMCR_RESTART_AN               0x0200
#define DM9161_BMCR_DUPLEX_MODE              0x0100
#define DM9161_BMCR_COL_TEST                 0x0080

//Basic Mode Status register
#define DM9161_BMSR_100BT4                   0x8000
#define DM9161_BMSR_100BTX_FD                0x4000
#define DM9161_BMSR_100BTX_HD                0x2000
#define DM9161_BMSR_10BT_FD                  0x1000
#define DM9161_BMSR_10BT_HD                  0x0800
#define DM9161_BMSR_MF_PREAMBLE_SUPPR        0x0040
#define DM9161_BMSR_AN_COMPLETE              0x0020
#define DM9161_BMSR_REMOTE_FAULT             0x0010
#define DM9161_BMSR_AN_CAPABLE               0x0008
#define DM9161_BMSR_LINK_STATUS              0x0004
#define DM9161_BMSR_JABBER_DETECT            0x0002
#define DM9161_BMSR_EXTENDED_CAPABLE         0x0001

//PHY Identifier 1 register
#define DM9161_PHYID1_OUI_MSB                0xFFFF
#define DM9161_PHYID1_OUI_MSB_DEFAULT        0x0181

//PHY Identifier 2 register
#define DM9161_PHYID2_OUI_LSB                0xFC00
#define DM9161_PHYID2_OUI_LSB_DEFAULT        0xB800
#define DM9161_PHYID2_VNDR_MDL               0x03F0
#define DM9161_PHYID2_VNDR_MDL_DEFAULT       0x0080
#define DM9161_PHYID2_MDL_REV                0x000F

//Auto-Negotiation Advertisement register
#define DM9161_ANAR_NEXT_PAGE                0x8000
#define DM9161_ANAR_ACK                      0x4000
#define DM9161_ANAR_REMOTE_FAULT             0x2000
#define DM9161_ANAR_FCS                      0x0400
#define DM9161_ANAR_100BT4                   0x0200
#define DM9161_ANAR_100BTX_FD                0x0100
#define DM9161_ANAR_100BTX_HD                0x0080
#define DM9161_ANAR_10BT_FD                  0x0040
#define DM9161_ANAR_10BT_HD                  0x0020
#define DM9161_ANAR_SELECTOR                 0x001F
#define DM9161_ANAR_SELECTOR_DEFAULT         0x0001

//Auto-Negotiation Link Partner Ability register
#define DM9161_ANLPAR_NEXT_PAGE              0x8000
#define DM9161_ANLPAR_ACK                    0x4000
#define DM9161_ANLPAR_REMOTE_FAULT           0x2000
#define DM9161_ANLPAR_FCS                    0x0400
#define DM9161_ANLPAR_100BT4                 0x0200
#define DM9161_ANLPAR_100BTX_FD              0x0100
#define DM9161_ANLPAR_100BTX_HD              0x0080
#define DM9161_ANLPAR_10BT_FD                0x0040
#define DM9161_ANLPAR_10BT_HD                0x0020
#define DM9161_ANLPAR_SELECTOR               0x001F
#define DM9161_ANLPAR_SELECTOR_DEFAULT       0x0001

//Auto-Negotiation Expansion register
#define DM9161_ANER_PAR_DETECT_FAULT         0x0010
#define DM9161_ANER_LP_NP_ABLE               0x0008
#define DM9161_ANER_NP_ABLE                  0x0004
#define DM9161_ANER_PAGE_RX                  0x0002
#define DM9161_ANER_LP_AN_ABLE               0x0001

//DAVICOM Specified Configuration register
#define DM9161_DSCR_BP_4B5B                  0x8000
#define DM9161_DSCR_BP_SCR                   0x4000
#define DM9161_DSCR_BP_ALIGN                 0x2000
#define DM9161_DSCR_BP_ADPOK                 0x1000
#define DM9161_DSCR_REPEATER                 0x0800
#define DM9161_DSCR_TX                       0x0400
#define DM9161_DSCR_FEF                      0x0200
#define DM9161_DSCR_RMII_EN                  0x0100
#define DM9161_DSCR_F_LINK_100               0x0080
#define DM9161_DSCR_SPLED_CTL                0x0040
#define DM9161_DSCR_COLLED_CTL               0x0020
#define DM9161_DSCR_RPDCTR_EN                0x0010
#define DM9161_DSCR_SMRST                    0x0008
#define DM9161_DSCR_MFPSC                    0x0004
#define DM9161_DSCR_SLEEP                    0x0002
#define DM9161_DSCR_RLOUT                    0x0001

//DAVICOM Specified Configuration and Status register
#define DM9161_DSCSR_100FDX                  0x8000
#define DM9161_DSCSR_100HDX                  0x4000
#define DM9161_DSCSR_10FDX                   0x2000
#define DM9161_DSCSR_10HDX                   0x1000
#define DM9161_DSCSR_PHYADR                  0x01F0
#define DM9161_DSCSR_ANMB                    0x000F
#define DM9161_DSCSR_ANMB_IDLE               0x0000
#define DM9161_DSCSR_ANMB_ABILITY_MATCH      0x0001
#define DM9161_DSCSR_ANMB_ACK_MATCH          0x0002
#define DM9161_DSCSR_ANMB_ACK_MATCH_FAIL     0x0003
#define DM9161_DSCSR_ANMB_CONSIST_MATCH      0x0004
#define DM9161_DSCSR_ANMB_CONSIST_MATCH_FAIL 0x0005
#define DM9161_DSCSR_ANMB_LINK_READY         0x0006
#define DM9161_DSCSR_ANMB_LINK_READY_FAIL    0x0007
#define DM9161_DSCSR_ANMB_AN_COMPLETE        0x0008

//10BASE-T Configuration/Status register
#define DM9161_10BTCSR_LP_EN                 0x4000
#define DM9161_10BTCSR_HBE                   0x2000
#define DM9161_10BTCSR_SQUELCH               0x1000
#define DM9161_10BTCSR_JABEN                 0x0800
#define DM9161_10BTCSR_10BT_SER              0x0400
#define DM9161_10BTCSR_POLR                  0x0001

//DAVICOM Specified Interrupt register
#define DM9161_MDINTR_INTR_PEND              0x8000
#define DM9161_MDINTR_FDX_MASK               0x0800
#define DM9161_MDINTR_SPD_MASK               0x0400
#define DM9161_MDINTR_LINK_MASK              0x0200
#define DM9161_MDINTR_INTR_MASK              0x0100
#define DM9161_MDINTR_FDX_CHANGE             0x0010
#define DM9161_MDINTR_SPD_CHANGE             0x0008
#define DM9161_MDINTR_LINK_CHANGE            0x0004
#define DM9161_MDINTR_INTR_STATUS            0x0001

//DAVICOM Specified Disconnect Counter register
#define DM9161_DISCR_DISCONNECT_COUNT        0x00FF

//DAVICOM Hardware Reset Latch State register
#define DM9161_RLSR_LH_LEDST                 0x2000
#define DM9161_RLSR_LH_CSTS                  0x1000
#define DM9161_RLSR_LH_RMII                  0x0800
#define DM9161_RLSR_LH_SCRAM                 0x0400
#define DM9161_RLSR_LH_REPTR                 0x0200
#define DM9161_RLSR_LH_TSTMOD                0x0100
#define DM9161_RLSR_LH_OP                    0x00E0
#define DM9161_RLSR_LH_PH                    0x001F

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//DM9161 Ethernet PHY driver
extern const PhyDriver dm9161PhyDriver;

//DM9161 related functions
error_t dm9161Init(NetInterface *interface);

void dm9161Tick(NetInterface *interface);

void dm9161EnableIrq(NetInterface *interface);
void dm9161DisableIrq(NetInterface *interface);

void dm9161EventHandler(NetInterface *interface);

void dm9161WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t dm9161ReadPhyReg(NetInterface *interface, uint8_t address);

void dm9161DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
