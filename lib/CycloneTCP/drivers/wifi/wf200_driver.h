/**
 * @file wf200_driver.h
 * @brief WF200 Wi-Fi controller
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

#ifndef _WF200_DRIVER_H
#define _WF200_DRIVER_H

//Dependencies
#include "core/nic.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//WF200 driver (STA mode)
extern const NicDriver wf200StaDriver;
//WF200 driver (AP mode)
extern const NicDriver wf200ApDriver;

//WF200 related functions
error_t wf200Init(NetInterface *interface);

void wf200Tick(NetInterface *interface);

void wf200EnableIrq(NetInterface *interface);
void wf200DisableIrq(NetInterface *interface);
void wf200EventHandler(NetInterface *interface);

error_t wf200SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t wf200UpdateMacAddrFilter(NetInterface *interface);

void wf200ConnectCallback(void);
void wf200DisconnectCallback(void);
void wf200StartApCallback(void);
void wf200StopApCallback(void);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
