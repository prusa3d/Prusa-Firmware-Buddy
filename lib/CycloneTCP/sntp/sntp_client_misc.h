/**
 * @file sntp_client.h
 * @brief Helper functions for SNTP client
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

#ifndef _SNTP_CLIENT_MISC_H
#define _SNTP_CLIENT_MISC_H

//Dependencies
#include "core/net.h"
#include "sntp/sntp_client.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//SNTP client related functions
error_t sntpClientOpenConnection(SntpClientContext *context);
void sntpClientCloseConnection(SntpClientContext *context);

error_t sntpClientSendRequest(SntpClientContext *context);
error_t sntpClientReceiveResponse(SntpClientContext *context);

error_t sntpClientCheckResponse(SntpClientContext *context,
   const IpAddr *ipAddr, uint16_t port, const uint8_t *message,
   size_t length);

error_t sntpClientParseResponse(SntpClientContext *context,
   NtpTimestamp *timestamp);

error_t sntpClientCheckTimeout(SntpClientContext *context);

void sntpClientDumpMessage(const uint8_t *message, size_t length);
void sntpClientDumpTimestamp(const NtpTimestamp *timestamp);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
