/**
 * @file f28m35x_eth_driver.h
 * @brief F28M35x Ethernet MAC driver
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

#ifndef _F28M35X_ETH_DRIVER_H
#define _F28M35X_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Interrupt priority grouping
#ifndef F28M35X_ETH_IRQ_PRIORITY_GROUPING
   #define F28M35X_ETH_IRQ_PRIORITY_GROUPING 3
#elif (F28M35X_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error F28M35X_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt priority
#ifndef F28M35X_ETH_IRQ_PRIORITY
   #define F28M35X_ETH_IRQ_PRIORITY 192
#elif (F28M35X_ETH_IRQ_PRIORITY < 0)
   #error F28M35X_ETH_IRQ_PRIORITY parameter is not valid
#endif

//F28M35x Ethernet MAC registers
#define MAC_RIS_R    HWREG(ETH_BASE + MAC_O_RIS)
#define MAC_IACK_R   HWREG(ETH_BASE + MAC_O_IACK)
#define MAC_IM_R     HWREG(ETH_BASE + MAC_O_IM)
#define MAC_RCTL_R   HWREG(ETH_BASE + MAC_O_RCTL)
#define MAC_TCTL_R   HWREG(ETH_BASE + MAC_O_TCTL)
#define MAC_DATA_R   HWREG(ETH_BASE + MAC_O_DATA)
#define MAC_IA0_R    HWREG(ETH_BASE + MAC_O_IA0)
#define MAC_IA1_R    HWREG(ETH_BASE + MAC_O_IA1)
#define MAC_THR_R    HWREG(ETH_BASE + MAC_O_THR)
#define MAC_MCTL_R   HWREG(ETH_BASE + MAC_O_MCTL)
#define MAC_MDV_R    HWREG(ETH_BASE + MAC_O_MDV)
#define MAC_MAR_R    HWREG(ETH_BASE + 0x28)
#define MAC_MTXD_R   HWREG(ETH_BASE + MAC_O_MTXD)
#define MAC_MRXD_R   HWREG(ETH_BASE + MAC_O_MRXD)
#define MAC_NP_R     HWREG(ETH_BASE + MAC_O_NP)
#define MAC_TR_R     HWREG(ETH_BASE + MAC_O_TR)
#define MAC_TS_R     HWREG(ETH_BASE + MAC_O_TS)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//F28M35x Ethernet MAC driver
extern const NicDriver f28m35xEthDriver;

//F28M35x Ethernet MAC related functions
error_t f28m35xEthInit(NetInterface *interface);
void f28m35xEthInitGpio(NetInterface *interface);

void f28m35xEthTick(NetInterface *interface);

void f28m35xEthEnableIrq(NetInterface *interface);
void f28m35xEthDisableIrq(NetInterface *interface);
void f28m35xEthIrqHandler(void);
void f28m35xEthEventHandler(NetInterface *interface);

error_t f28m35xEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t f28m35xEthReceivePacket(NetInterface *interface);

error_t f28m35xEthUpdateMacAddrFilter(NetInterface *interface);
error_t f28m35xEthUpdateMacConfig(NetInterface *interface);

void f28m35xEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t f28m35xEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
