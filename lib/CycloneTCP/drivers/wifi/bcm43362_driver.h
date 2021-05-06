/**
 * @file bcm43362_driver.h
 * @brief BCM43362 Wi-Fi controller
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

#ifndef _BCM43362_DRIVER_H
#define _BCM43362_DRIVER_H

//Dependencies
#include "core/nic.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//BCM43362 driver (STA mode)
extern const NicDriver bcm43362StaDriver;
//BCM43362 driver (AP mode)
extern const NicDriver bcm43362ApDriver;

//BCM43362 related functions
error_t bcm43362Init(NetInterface *interface);

void bcm43362Tick(NetInterface *interface);

void bcm43362EnableIrq(NetInterface *interface);
void bcm43362DisableIrq(NetInterface *interface);
bool_t bcm43362IrqHandler(void);
void bcm43362EventHandler(NetInterface *interface);

error_t bcm43362SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t bcm43362UpdateMacAddrFilter(NetInterface *interface);

void bcm43362AppWifiEvent(uint8_t msgType, void *msg);
void bcm43362AppEthEvent(uint8_t msgType, void *msg, void *ctrlBuf);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
