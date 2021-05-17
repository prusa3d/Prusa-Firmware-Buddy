/**
 * @file http_server_auth.h
 * @brief HTTP authentication
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

#ifndef _HTTP_SERVER_AUTH_H
#define _HTTP_SERVER_AUTH_H

//Dependencies
#include "http/http_server.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//HTTP authentication related functions
bool_t httpCheckPassword(HttpConnection *connection,
   const char_t *password, HttpAuthMode mode);

void httpParseAuthorizationField(HttpConnection *connection, char_t *value);
size_t httpAddAuthenticateField(HttpConnection *connection, char_t *output);

error_t httpGenerateNonce(HttpServerContext *context,
   char_t *output, size_t *length);

error_t httpVerifyNonce(HttpServerContext *context,
   const char_t *nonce, const char_t *nc);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
