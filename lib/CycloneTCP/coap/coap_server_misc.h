/**
 * @file coap_server_misc.h
 * @brief Helper functions for CoAP server
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

#ifndef _COAP_SERVER_MISC_H
#define _COAP_SERVER_MISC_H

//Dependencies
#include "core/net.h"
#include "coap/coap_server.h"

//C++ guard
#ifdef __cplusplus
   extern "C" {
#endif

//CoAP server related functions
void coapServerTick(CoapServerContext *context);

error_t coapServerProcessRequest(CoapServerContext *context,
   const uint8_t *data, size_t length);

error_t coapServerRejectRequest(CoapServerContext *context);

error_t coapServerInitResponse(CoapServerContext *context);

error_t coapServerSendResponse(CoapServerContext *context,
   const void *data, size_t length);

error_t coapServerFormatReset(CoapServerContext *context, uint16_t mid);

//C++ guard
#ifdef __cplusplus
   }
#endif

#endif
