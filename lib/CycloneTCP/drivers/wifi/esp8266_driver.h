/**
 * @file esp8266_driver.h
 * @brief ESP8266 Wi-Fi controller
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

#ifndef _ESP8266_WIFI_DRIVER_H
#define _ESP8266_WIFI_DRIVER_H

//Dependencies
#include "core/nic.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//ESP8266 Wi-Fi driver (STA mode)
extern const NicDriver esp8266WifiStaDriver;
//ESP8266 Wi-Fi driver (AP mode)
extern const NicDriver esp8266WifiApDriver;

//ESP8266 Wi-Fi related functions
error_t esp8266WifiInit(NetInterface *interface);

void esp8266WifiTick(NetInterface *interface);

void esp8266WifiEnableIrq(NetInterface *interface);
void esp8266WifiDisableIrq(NetInterface *interface);
void esp8266WifiEventHandler(NetInterface *interface);

error_t esp8266WifiSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t esp8266WifiUpdateMacAddrFilter(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
