/**
 * @file loopback_driver.h
 * @brief Loopback interface driver
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

#ifndef _LOOPBACK_DRIVER_H
#define _LOOPBACK_DRIVER_H

//Dependencies
#include "core/nic.h"

//Queue size
#ifndef LOOPBACK_DRIVER_QUEUE_SIZE
   #define LOOPBACK_DRIVER_QUEUE_SIZE 6
#elif (LOOPBACK_DRIVER_QUEUE_SIZE < 1)
   #error LOOPBACK_DRIVER_QUEUE_SIZE parameter is not valid
#endif


/**
 * @brief Loopback interface queue entry
 **/

typedef struct
{
   size_t length;
   uint8_t data[ETH_MTU];
} LoopbackDriverQueueEntry;


//Loopback interface driver
extern const NicDriver loopbackDriver;

//Loopback interface related functions
error_t loopbackDriverInit(NetInterface *interface);

void loopbackDriverTick(NetInterface *interface);

void loopbackDriverEnableIrq(NetInterface *interface);
void loopbackDriverDisableIrq(NetInterface *interface);
void loopbackDriverEventHandler(NetInterface *interface);

error_t loopbackDriverSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t loopbackDriverReceivePacket(NetInterface *interface);

error_t loopbackDriverUpdateMacAddrFilter(NetInterface *interface);

#endif
