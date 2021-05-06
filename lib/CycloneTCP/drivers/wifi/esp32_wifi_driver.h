/**
 * @file esp32_wifi_driver.h
 * @brief ESP32 Wi-Fi controller
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

#ifndef _ESP32_WIFI_DRIVER_H
#define _ESP32_WIFI_DRIVER_H

//Dependencies
#include "core/nic.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//ESP32 Wi-Fi driver (STA mode)
extern const NicDriver esp32WifiStaDriver;
//ESP32 Wi-Fi driver (AP mode)
extern const NicDriver esp32WifiApDriver;

//ESP32 Wi-Fi related functions
error_t esp32WifiInit(NetInterface *interface);

void esp32WifiTick(NetInterface *interface);

void esp32WifiEnableIrq(NetInterface *interface);
void esp32WifiDisableIrq(NetInterface *interface);
void esp32WifiEventHandler(NetInterface *interface);

error_t esp32WifiSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t esp32WifiUpdateMacAddrFilter(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
