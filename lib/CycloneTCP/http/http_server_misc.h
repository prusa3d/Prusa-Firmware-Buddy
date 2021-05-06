/**
 * @file http_server_misc.h
 * @brief HTTP server (miscellaneous functions)
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

#ifndef _HTTP_SERVER_MISC_H
#define _HTTP_SERVER_MISC_H

//Dependencies
#include "http/http_server.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//HTTP server related functions
error_t httpReadRequestHeader(HttpConnection *connection);
error_t httpParseRequestLine(HttpConnection *connection, char_t *requestLine);

error_t httpReadHeaderField(HttpConnection *connection,
   char_t *buffer, size_t size, char_t *firstChar);

void httpParseHeaderField(HttpConnection *connection,
   const char_t *name, char_t *value);

void httpParseConnectionField(HttpConnection *connection,
   char_t *value);

void httpParseContentTypeField(HttpConnection *connection,
   char_t *value);

void httpParseAcceptEncodingField(HttpConnection *connection,
   char_t *value);

void httpParseCookieField(HttpConnection *connection, char_t *value);

error_t httpReadChunkSize(HttpConnection *connection);

void httpInitResponseHeader(HttpConnection *connection);
error_t httpFormatResponseHeader(HttpConnection *connection, char_t *buffer);

error_t httpSend(HttpConnection *connection,
   const void *data, size_t length, uint_t flags);

error_t httpReceive(HttpConnection *connection,
   void *data, size_t size, size_t *received, uint_t flags);

void httpGetAbsolutePath(HttpConnection *connection,
   const char_t *relative, char_t *absolute, size_t maxLen);

bool_t httpCompExtension(const char_t *filename, const char_t *extension);

error_t httpDecodePercentEncodedString(const char_t *input,
   char_t *output, size_t outputSize);

void httpConvertArrayToHexString(const uint8_t *input,
   size_t inputLen, char_t *output);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
