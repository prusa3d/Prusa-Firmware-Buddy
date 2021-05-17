/**
 * @file mrf24wg_driver.h
 * @brief MRF24WG Wi-Fi controller
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

#ifndef _MRF24WG_DRIVER_H
#define _MRF24WG_DRIVER_H

//Dependencies
#include "core/nic.h"

//TX buffer size
#ifndef MRF24WG_TX_BUFFER_SIZE
   #define MRF24WG_TX_BUFFER_SIZE 1536
#elif (MRF24WG_TX_BUFFER_SIZE != 1536)
   #error MRF24WG_TX_BUFFER_SIZE parameter is not valid
#endif

//RX buffer size
#ifndef MRF24WG_RX_BUFFER_SIZE
   #define MRF24WG_RX_BUFFER_SIZE 1536
#elif (MRF24WG_RX_BUFFER_SIZE != 1536)
   #error MRF24WG_RX_BUFFER_SIZE parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief TX buffer
 **/

typedef struct
{
   bool_t used;
   size_t length;
   uint8_t data[MRF24WG_TX_BUFFER_SIZE];
} Mrf24wgBuffer;


//MRF24WG driver
extern const NicDriver mrf24wgDriver;

//MRF24WG related functions
error_t mrf24wgInit(NetInterface *interface);

void mrf24wgTick(NetInterface *interface);

void mrf24wgEnableIrq(NetInterface *interface);
void mrf24wgDisableIrq(NetInterface *interface);
void mrf24wgEventHandler(NetInterface *interface);

error_t mrf24wgSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t mrf24wgUpdateMacAddrFilter(NetInterface *interface);

void mrf24wgAppWifiEvent(uint8_t msgType, void *msg);
void mrf24wgAppEthEvent(uint8_t msgType, void *msg, void *ctrlBuf);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
