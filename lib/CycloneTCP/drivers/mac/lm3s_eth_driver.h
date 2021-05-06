/**
 * @file lm3s_eth_driver.h
 * @brief Luminary Stellaris LM3S Ethernet controller
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

#ifndef _LM3S_ETH_DRIVER_H
#define _LM3S_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Interrupt priority grouping
#ifndef LM3S_ETH_IRQ_PRIORITY_GROUPING
   #define LM3S_ETH_IRQ_PRIORITY_GROUPING 3
#elif (LM3S_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error LM3S_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt priority
#ifndef LM3S_ETH_IRQ_PRIORITY
   #define LM3S_ETH_IRQ_PRIORITY 192
#elif (LM3S_ETH_IRQ_PRIORITY < 0)
   #error LM3S_ETH_IRQ_PRIORITY parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//Stellaris LM3S Ethernet driver
extern const NicDriver lm3sEthDriver;

//Stellaris LM3S Ethernet related functions
error_t lm3sEthInit(NetInterface *interface);
void lm3sEthInitGpio(NetInterface *interface);

void lm3sEthTick(NetInterface *interface);

void lm3sEthEnableIrq(NetInterface *interface);
void lm3sEthDisableIrq(NetInterface *interface);
void lm3sEthEventHandler(NetInterface *interface);

error_t lm3sEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t lm3sEthReceivePacket(NetInterface *interface);

error_t lm3sEthUpdateMacAddrFilter(NetInterface *interface);

void lm3sEthWritePhyReg(uint8_t address, uint16_t data);
uint16_t lm3sEthReadPhyReg(uint8_t address);

void lm3sEthDumpPhyReg(void);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
