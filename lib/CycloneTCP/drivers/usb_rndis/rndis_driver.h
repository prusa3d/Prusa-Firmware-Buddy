/**
 * @file rndis_driver.h
 * @brief RNDIS driver
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

#ifndef _RNDIS_DRIVER_H
#define _RNDIS_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef RNDIS_TX_BUFFER_COUNT
   #define RNDIS_TX_BUFFER_COUNT 2
#elif (RNDIS_TX_BUFFER_COUNT < 1)
   #error RNDIS_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef RNDIS_TX_BUFFER_SIZE
   #define RNDIS_TX_BUFFER_SIZE 2048
#elif (RNDIS_TX_BUFFER_SIZE != 2048)
   #error RNDIS_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef RNDIS_RX_BUFFER_COUNT
   #define RNDIS_RX_BUFFER_COUNT 1
#elif (RNDIS_RX_BUFFER_COUNT < 1)
   #error RNDIS_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef RNDIS_RX_BUFFER_SIZE
   #define RNDIS_RX_BUFFER_SIZE 2048
#elif (RNDIS_RX_BUFFER_SIZE != 2048)
   #error RNDIS_RX_BUFFER_SIZE parameter is not valid
#endif


/**
 * @brief TX buffer descriptor
 **/

typedef struct
{
   bool_t ready;
   size_t length;
   uint8_t data[RNDIS_TX_BUFFER_SIZE];
} RndisTxBufferDesc;


/**
 * @brief RX buffer descriptor
 **/

typedef struct
{
   bool_t ready;
   size_t length;
   uint8_t data[RNDIS_RX_BUFFER_SIZE];
} RndisRxBufferDesc;


//RNDIS driver
extern const NicDriver rndisDriver;
//Underlying network interface
extern NetInterface *rndisDriverInterface;

//TX and RX buffers
extern RndisTxBufferDesc rndisTxBuffer[RNDIS_TX_BUFFER_COUNT];
extern RndisRxBufferDesc rndisRxBuffer[RNDIS_RX_BUFFER_COUNT];

//Buffer indexes
extern uint_t rndisTxWriteIndex;
extern uint_t rndisTxReadIndex;
extern uint_t rndisRxWriteIndex;
extern uint_t rndisRxReadIndex;

//RNDIS driver related functions
error_t rndisDriverInit(NetInterface *interface);

void rndisDriverTick(NetInterface *interface);

void rndisDriverEnableIrq(NetInterface *interface);
void rndisDriverDisableIrq(NetInterface *interface);
void rndisDriverEventHandler(NetInterface *interface);

error_t rndisDriverSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t rndisDriverReceivePacket(NetInterface *interface,
   uint8_t *buffer, size_t size, size_t *length);

error_t rndisDriverSetMulticastFilter(NetInterface *interface);

#endif
