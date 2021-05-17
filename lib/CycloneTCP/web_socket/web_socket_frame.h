/**
 * @file web_socket_frame.h
 * @brief WebSocket frame parsing and formatting
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

#ifndef _WEB_SOCKET_FRAME_H
#define _WEB_SOCKET_FRAME_H

//Dependencies
#include "core/net.h"
#include "web_socket/web_socket.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//WebSocket related functions
error_t webSocketFormatFrameHeader(WebSocket *webSocket,
   bool_t fin, WebSocketFrameType type, size_t payloadLen);

error_t webSocketParseFrameHeader(WebSocket *webSocket,
   const WebSocketFrame *frame, WebSocketFrameType *type);

error_t webSocketFormatCloseFrame(WebSocket *webSocket);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
