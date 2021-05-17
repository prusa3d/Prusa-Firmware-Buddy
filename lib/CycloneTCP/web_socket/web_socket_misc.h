/**
 * @file web_socket_misc.h
 * @brief Helper functions for WebSockets
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

#ifndef _WEB_SOCKET_MISC_H
#define _WEB_SOCKET_MISC_H

//Dependencies
#include "core/net.h"
#include "web_socket/web_socket.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief HTTP status code
 **/

typedef struct
{
   uint_t value;
   const char_t message[28];
} WebSocketStatusCodeDesc;


//WebSocket related functions
void webSocketChangeState(WebSocket *webSocket, WebSocketState newState);

error_t webSocketParseHandshake(WebSocket *webSocket);
error_t webSocketParseRequestLine(WebSocket *webSocket, char_t *line);
error_t webSocketParseStatusLine(WebSocket *webSocket, char_t *line);
error_t webSocketParseHeaderField(WebSocket *webSocket, char_t *line);

void webSocketParseConnectionField(WebSocket *webSocket, char_t *value);

error_t webSocketFormatClientHandshake(WebSocket *webSocket, uint16_t serverPort);
error_t webSocketFormatServerHandshake(WebSocket *webSocket);

error_t webSocketFormatErrorResponse(WebSocket *webSocket,
   uint_t statusCode, const char_t *message);

error_t webSocketVerifyClientHandshake(WebSocket *webSocket);
error_t webSocketVerifyServerHandshake(WebSocket *webSocket);

error_t webSocketGenerateClientKey(WebSocket *webSocket);
error_t webSocketGenerateServerKey(WebSocket *webSocket);

error_t webSocketVerifyClientKey(WebSocket *webSocket);
error_t webSocketVerifyServerKey(WebSocket *webSocket);

bool_t webSocketCheckStatusCode(uint16_t statusCode);

error_t webSocketDecodePercentEncodedString(const char_t *input,
   char_t *output, size_t outputSize);

bool_t webSocketCheckUtf8Stream(WebSocketUtf8Context *context,
   const uint8_t *data, size_t length, size_t remaining);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
