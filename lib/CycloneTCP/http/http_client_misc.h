/**
 * @file http_client_misc.h
 * @brief Helper functions for HTTP client
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

#ifndef _HTTP_CLIENT_MISC_H
#define _HTTP_CLIENT_MISC_H

//Dependencies
#include "core/net.h"
#include "http/http_client.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//HTTP client related functions
void httpClientChangeState(HttpClientContext *context,
   HttpClientState newState);

void httpClientChangeRequestState(HttpClientContext *context,
   HttpRequestState newState);

error_t httpClientFormatRequestHeader(HttpClientContext *context);

error_t httpClientFormatChunkSize(HttpClientContext *context, size_t length);

error_t httpClientParseStatusLine(HttpClientContext *context, char_t *line,
   size_t length);

error_t httpClientParseHeaderField(HttpClientContext *context, char_t *line,
   size_t length);

error_t httpClientParseConnectionField(HttpClientContext *context,
   const char_t *value);

error_t httpClientParseTransferEncodingField(HttpClientContext *context,
   const char_t *value);

error_t httpClientParseContentLengthField(HttpClientContext *context,
   const char_t *value);

error_t httpClientParseChunkSize(HttpClientContext *context, char_t *line,
   size_t length);

error_t httpClientCheckTimeout(HttpClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
