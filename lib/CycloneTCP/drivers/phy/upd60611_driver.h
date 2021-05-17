/**
 * @file upd60611_driver.h
 * @brief uPD60611 Ethernet PHY driver
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

#ifndef _UPD60611_DRIVER_H
#define _UPD60611_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef UPD60611_PHY_ADDR
   #define UPD60611_PHY_ADDR 0
#elif (UPD60611_PHY_ADDR < 0 || UPD60611_PHY_ADDR > 31)
   #error UPD60611_PHY_ADDR parameter is not valid
#endif

//uPD60611 PHY registers
#define UPD60611_BMCR                          0x00
#define UPD60611_BMSR                          0x01
#define UPD60611_PHYID1                        0x02
#define UPD60611_PHYID2                        0x03
#define UPD60611_ANAR                          0x04
#define UPD60611_ANLPAR                        0x05
#define UPD60611_ANER                          0x06
#define UPD60611_ANNPR                         0x07
#define UPD60611_SRR                           0x10
#define UPD60611_MCSR                          0x11
#define UPD60611_SMR                           0x12
#define UPD60611_EBSR                          0x13
#define UPD60611_BER                           0x17
#define UPD60611_FEQMR                         0x18
#define UPD60611_DCSR                          0x19
#define UPD60611_DCR                           0x1A
#define UPD60611_SCSIR                         0x1B
#define UPD60611_ISR                           0x1D
#define UPD60611_IER                           0x1E
#define UPD60611_PSCSR                         0x1F

//Basic Control register
#define UPD60611_BMCR_RESET                    0x8000
#define UPD60611_BMCR_LOOPBACK                 0x4000
#define UPD60611_BMCR_SPEED_SEL                0x2000
#define UPD60611_BMCR_AN_EN                    0x1000
#define UPD60611_BMCR_POWER_DOWN               0x0800
#define UPD60611_BMCR_ISOLATE                  0x0400
#define UPD60611_BMCR_RESTART_AN               0x0200
#define UPD60611_BMCR_DUPLEX_MODE              0x0100
#define UPD60611_BMCR_COL_TEST                 0x0080

//Basic Status register
#define UPD60611_BMSR_100BT4                   0x8000
#define UPD60611_BMSR_100BTX_FD                0x4000
#define UPD60611_BMSR_100BTX_HD                0x2000
#define UPD60611_BMSR_10BT_FD                  0x1000
#define UPD60611_BMSR_10BT_HD                  0x0800
#define UPD60611_BMSR_AN_COMPLETE              0x0020
#define UPD60611_BMSR_REMOTE_FAULT             0x0010
#define UPD60611_BMSR_AN_CAPABLE               0x0008
#define UPD60611_BMSR_LINK_STATUS              0x0004
#define UPD60611_BMSR_JABBER_DETECT            0x0002
#define UPD60611_BMSR_EXTENDED_CAPABLE         0x0001

//PHY Identifier 1 register
#define UPD60611_PHYID1_PHY_ID_NUM_MSB         0xFFFF
#define UPD60611_PHYID1_PHY_ID_NUM_MSB_DEFAULT 0xB824

//PHY Identifier 2 register
#define UPD60611_PHYID2_PHY_ID_NUM_LSB         0xFC00
#define UPD60611_PHYID2_PHY_ID_NUM_LSB_DEFAULT 0x2800
#define UPD60611_PHYID2_MODEL_NUM              0x03F0
#define UPD60611_PHYID2_MODEL_NUM_DEFAULT      0x0010
#define UPD60611_PHYID2_REVISION_NUM           0x000F

//Auto-Negotiation Advertisement register
#define UPD60611_ANAR_NEXT_PAGE                0x8000
#define UPD60611_ANAR_REMOTE_FAULT             0x2000
#define UPD60611_ANAR_PAUSE                    0x0C00
#define UPD60611_ANAR_100BT4                   0x0200
#define UPD60611_ANAR_100BTX_FD                0x0100
#define UPD60611_ANAR_100BTX_HD                0x0080
#define UPD60611_ANAR_10BT_FD                  0x0040
#define UPD60611_ANAR_10BT_HD                  0x0020
#define UPD60611_ANAR_SELECTOR                 0x001F
#define UPD60611_ANAR_SELECTOR_DEFAULT         0x0001

//Auto-Negotiation Link Partner Ability register
#define UPD60611_ANLPAR_NEXT_PAGE              0x8000
#define UPD60611_ANLPAR_ACK                    0x4000
#define UPD60611_ANLPAR_REMOTE_FAULT           0x2000
#define UPD60611_ANLPAR_PAUSE                  0x0400
#define UPD60611_ANLPAR_100BT4                 0x0200
#define UPD60611_ANLPAR_100BTX_FD              0x0100
#define UPD60611_ANLPAR_100BTX_HD              0x0080
#define UPD60611_ANLPAR_10BT_FD                0x0040
#define UPD60611_ANLPAR_10BT_HD                0x0020
#define UPD60611_ANLPAR_SELECTOR               0x001F
#define UPD60611_ANLPAR_SELECTOR_DEFAULT       0x0001

//Auto-Negotiation Expansion register
#define UPD60611_ANER_PAR_DETECT_FAULT         0x0010
#define UPD60611_ANER_LP_NEXT_PAGE_ABLE        0x0008
#define UPD60611_ANER_NEXT_PAGE_ABLE           0x0004
#define UPD60611_ANER_PAGE_RECEIVED            0x0002
#define UPD60611_ANER_LP_AN_ABLE               0x0001

//Auto-Negotiation Next Page Transmit register
#define UPD60611_ANNPR_NEXT_PAGE               0x8000
#define UPD60611_ANNPR_MSG_PAGE                0x2000
#define UPD60611_ANNPR_ACK2                    0x1000
#define UPD60611_ANNPR_TOGGLE                  0x0800
#define UPD60611_ANNPR_MESSAGE                 0x07FF

//Silicon Revision register
#define UPD60611_SRR_SILICON_REV               0x03C0

//Mode Control/Status register
#define UPD60611_MCSR_EDPWRDOWN                0x2000
#define UPD60611_MCSR_FARLOOPBACK              0x0200
#define UPD60611_MCSR_FASTEST                  0x0100
#define UPD60611_MCSR_AUTOMDIX_EN              0x0080
#define UPD60611_MCSR_MDI_MODE                 0x0040
#define UPD60611_MCSR_FORCE_GOOD_LINK          0x0004
#define UPD60611_MCSR_ENERGYON                 0x0002

//Special Modes register
#define UPD60611_SMR_FX_MODE                   0x0400
#define UPD60611_SMR_PHY_MODE                  0x01E0
#define UPD60611_SMR_PHY_ADD_DEV               0x0018
#define UPD60611_SMR_PHY_ADD_MOD               0x0007

//Elastic Buffer Status register
#define UPD60611_EBSR_T_EL_BUF_OVF             0x0080
#define UPD60611_EBSR_T_EL_BUF_UDF             0x0040
#define UPD60611_EBSR_R_EL_BUF_OVF             0x0020
#define UPD60611_EBSR_R_EL_BUF_UDF             0x0010

//BER Counter register
#define UPD60611_BER_LNK_OK                    0x8000
#define UPD60611_BER_CNT_LNK_EN                0x4000
#define UPD60611_BER_CNT_TRIG                  0x3800
#define UPD60611_BER_WINDOW                    0x0780
#define UPD60611_BER_COUNT                     0x007F

//Diagnosis Control/Status register
#define UPD60611_DCSR_DIAG_INIT                0x4000
#define UPD60611_DCSR_ADC_MAX_VALUE            0x3F00
#define UPD60611_DCSR_DIAG_DONE                0x0080
#define UPD60611_DCSR_DIAG_POL                 0x0040
#define UPD60611_DCSR_DIAG_SEL_LINE            0x0020
#define UPD60611_DCSR_PW_DIAG                  0x001F

//Diagnosis Counter register
#define UPD60611_DCR_CNT_WINDOW                0xFF00
#define UPD60611_DCR_DIAGCNT                   0x00FF

//Special Control/Status Indications register
#define UPD60611_SCSIR_SWRST_FAST              0x1000
#define UPD60611_SCSIR_SQEOFF                  0x0800
#define UPD60611_SCSIR_FEFIEN                  0x0020
#define UPD60611_SCSIR_XPOL                    0x0010

//Interrupt Source Flags register
#define UPD60611_ISR_BER                       0x0400
#define UPD60611_ISR_FEQ                       0x0200
#define UPD60611_ISR_ENERGYON                  0x0080
#define UPD60611_ISR_AN_COMPLETE               0x0040
#define UPD60611_ISR_REMOTE_FAULT              0x0020
#define UPD60611_ISR_LINK_DOWN                 0x0010
#define UPD60611_ISR_AN_LP_ACK                 0x0008
#define UPD60611_ISR_PAR_DETECT_FAULT          0x0004
#define UPD60611_ISR_AN_PAGE_RECEIVED          0x0002

//Interrupt Enable register
#define UPD60611_IER_BER                       0x0400
#define UPD60611_IER_FEQ                       0x0200
#define UPD60611_IER_ENERGYON                  0x0080
#define UPD60611_IER_AN_COMPLETE               0x0040
#define UPD60611_IER_REMOTE_FAULT              0x0020
#define UPD60611_IER_LINK_DOWN                 0x0010
#define UPD60611_IER_AN_LP_ACK                 0x0008
#define UPD60611_IER_PAR_DETECT_FAULT          0x0004
#define UPD60611_IER_AN_PAGE_RECEIVED          0x0002

//PHY Special Control/Status register
#define UPD60611_PSCSR_AUTODONE                0x1000
#define UPD60611_PSCSR_4B5B_EN                 0x0040
#define UPD60611_PSCSR_HCDSPEED                0x001C
#define UPD60611_PSCSR_HCDSPEED_10BT_HD        0x0004
#define UPD60611_PSCSR_HCDSPEED_100BTX_HD      0x0008
#define UPD60611_PSCSR_HCDSPEED_10BT_FD        0x0014
#define UPD60611_PSCSR_HCDSPEED_100BTX_FD      0x0018
#define UPD60611_PSCSR_RX_DV_J2T               0x0002
#define UPD60611_PSCSR_SCRAMBLE_DIS            0x0001

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//uPD60611 Ethernet PHY driver
extern const PhyDriver upd60611PhyDriver;

//uPD60611 related functions
error_t upd60611Init(NetInterface *interface);

void upd60611Tick(NetInterface *interface);

void upd60611EnableIrq(NetInterface *interface);
void upd60611DisableIrq(NetInterface *interface);

void upd60611EventHandler(NetInterface *interface);

void upd60611WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t upd60611ReadPhyReg(NetInterface *interface, uint8_t address);

void upd60611DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
