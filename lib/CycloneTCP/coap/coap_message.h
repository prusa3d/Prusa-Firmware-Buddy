/**
 * @file coap_message.h
 * @brief CoAP message formatting and parsing
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

#ifndef _COAP_MESSAGE_H
#define _COAP_MESSAGE_H

//Dependencies
#include "core/net.h"
#include "coap/coap_common.h"

//Maximum size of CoAP messages
#ifndef COAP_MAX_MSG_SIZE
   #define COAP_MAX_MSG_SIZE 1152
#elif (COAP_MAX_MSG_SIZE < 16)
   #error COAP_MAX_MSG_SIZE parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief CoAP message
 **/

typedef struct
{
   uint8_t buffer[COAP_MAX_MSG_SIZE];
   size_t length;
   size_t pos;
} CoapMessage;


//CoAP related functions
error_t coapParseMessage(const CoapMessage *message);

error_t coapParseMessageHeader(const uint8_t *p, size_t length,
   size_t *consumed);

error_t coapSetType(CoapMessage *message, CoapMessageType type);
error_t coapGetType(const CoapMessage *message, CoapMessageType *type);

error_t coapSetCode(CoapMessage *message, CoapCode code);
error_t coapGetCode(const CoapMessage *message, CoapCode *code);

error_t coapSetPayload(CoapMessage *message, const void *payload,
   size_t payloadLen);

error_t coapGetPayload(const CoapMessage *message, const uint8_t **payload,
   size_t *payloadLen);

error_t coapWritePayload(CoapMessage *message, const void *data,
   size_t length);

error_t coapReadPayload(CoapMessage *message, void *data, size_t size,
   size_t *length);

bool_t coapCompareToken(const CoapMessageHeader *header1,
   const CoapMessageHeader *header2);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
