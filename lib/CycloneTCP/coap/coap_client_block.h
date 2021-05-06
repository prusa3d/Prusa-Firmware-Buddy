/**
 * @file coap_client_block.h
 * @brief CoAP block-wise transfer
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

#ifndef _COAP_CLIENT_BLOCK_H
#define _COAP_CLIENT_BLOCK_H

//Dependencies
#include "core/net.h"
#include "coap/coap_client.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//CoAP client related functions
error_t coapClientSetTxBlockSize(CoapClientRequest *request, uint_t blockSize);
error_t coapClientSetRxBlockSize(CoapClientRequest *request, uint_t blockSize);

error_t coapClientWriteBody(CoapClientRequest *request, const void *data,
   size_t length, size_t *written, bool_t last);

error_t coapClientReadBody(CoapClientRequest *request, void *data,
   size_t size, size_t *received);

CoapBlockSize coapClientGetMaxBlockSize(void);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
