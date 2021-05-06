/**
 * @file ppp_hdlc.h
 * @brief PPP HDLC driver
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

#ifndef _PPP_HDLC_H
#define _PPP_HDLC_H

//Dependencies
#include "core/nic.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//PPP HDLC driver
extern const NicDriver pppHdlcDriver;

//PPP HDLC driver related functions
error_t pppHdlcDriverInit(NetInterface *interface);

void pppHdlcDriverTick(NetInterface *interface);

void pppHdlcDriverEnableIrq(NetInterface *interface);
void pppHdlcDriverDisableIrq(NetInterface *interface);
void pppHdlcDriverEventHandler(NetInterface *interface);

error_t pppHdlcDriverSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t pppHdlcDriverReceivePacket(NetInterface *interface);

error_t pppHdlcDriverUpdateMacAddrFilter(NetInterface *interface);

error_t pppHdlcDriverSendAtCommand(NetInterface *interface, const char_t *data);

error_t pppHdlcDriverReceiveAtCommand(NetInterface *interface, char_t *data,
   size_t size);

error_t pppHdlcDriverPurgeTxBuffer(PppContext *context);
error_t pppHdlcDriverPurgeRxBuffer(PppContext *context);

void pppHdlcDriverWriteTxQueue(PppContext *context, uint8_t c);
uint8_t pppHdlcDriverReadRxQueue(PppContext *context);

bool_t pppHdlcDriverReadTxQueue(NetInterface *interface, int_t *c);
bool_t pppHdlcDriverWriteRxQueue(NetInterface *interface, uint8_t c);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
