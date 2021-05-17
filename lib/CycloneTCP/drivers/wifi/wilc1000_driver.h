/**
 * @file wilc1000_driver.h
 * @brief WILC1000 Wi-Fi controller
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

#ifndef _WILC1000_DRIVER_H
#define _WILC1000_DRIVER_H

//Dependencies
#include "core/nic.h"

//TX buffer size
#ifndef WILC1000_TX_BUFFER_SIZE
   #define WILC1000_TX_BUFFER_SIZE 1600
#elif (WILC1000_TX_BUFFER_SIZE != 1600)
   #error WILC1000_TX_BUFFER_SIZE parameter is not valid
#endif

//RX buffer size
#ifndef WILC1000_RX_BUFFER_SIZE
   #define WILC1000_RX_BUFFER_SIZE 1600
#elif (WILC1000_RX_BUFFER_SIZE != 1600)
   #error WILC1000_RX_BUFFER_SIZE parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//WILC1000 driver (STA mode)
extern const NicDriver wilc1000StaDriver;
//WILC1000 driver (AP mode)
extern const NicDriver wilc1000ApDriver;

//WILC1000 related functions
error_t wilc1000Init(NetInterface *interface);

void wilc1000Tick(NetInterface *interface);

void wilc1000EnableIrq(NetInterface *interface);
void wilc1000DisableIrq(NetInterface *interface);
bool_t wilc1000IrqHandler(void);
void wilc1000EventHandler(NetInterface *interface);

error_t wilc1000SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t wilc1000UpdateMacAddrFilter(NetInterface *interface);
bool_t wilc1000GetAddrRefCount(NetInterface *interface, const MacAddr *macAddr);

void wilc1000AppWifiEvent(uint8_t msgType, void *msg);
void wilc1000AppEthEvent(uint8_t msgType, void *msg, void *ctrlBuf);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
