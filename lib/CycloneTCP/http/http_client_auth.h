/**
 * @file http_client_auth.h
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

#ifndef _HTTP_CLIENT_AUTH_H
#define _HTTP_CLIENT_AUTH_H

//Dependencies
#include "core/net.h"
#include "http/http_client.h"

//Maximum digest context size
#if (HTTP_CLIENT_SHA512_256_SUPPORT == ENABLED)
   #define HTTP_CLIENT_MAX_HASH_CONTEXT_SIZE sizeof(Sha512_256Context)
#elif (HTTP_CLIENT_SHA256_SUPPORT == ENABLED)
   #define HTTP_CLIENT_MAX_HASH_CONTEXT_SIZE sizeof(Sha256Context)
#else
   #define HTTP_CLIENT_MAX_HASH_CONTEXT_SIZE sizeof(Md5Context)
#endif

//Maximum digest size
#if (HTTP_CLIENT_SHA512_256_SUPPORT == ENABLED)
   #define HTTP_CLIENT_MAX_HASH_DIGEST_SIZE 32
#elif (HTTP_CLIENT_SHA256_SUPPORT == ENABLED)
   #define HTTP_CLIENT_MAX_HASH_DIGEST_SIZE 32
#else
   #define HTTP_CLIENT_MAX_HASH_DIGEST_SIZE 16
#endif

//Maximum response length
#if (HTTP_CLIENT_SHA512_256_SUPPORT == ENABLED)
   #define HTTP_CLIENT_MAX_RESPONSE_LEN 64
#elif (HTTP_CLIENT_SHA256_SUPPORT == ENABLED)
   #define HTTP_CLIENT_MAX_RESPONSE_LEN 64
#else
   #define HTTP_CLIENT_MAX_RESPONSE_LEN 32
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief WWW-Authenticate header field
 **/

typedef struct
{
   HttpAuthMode mode;            ///<Authentication scheme
   const char_t *realm;          ///<Realm
   size_t realmLen;              ///<Length of the realm
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
   HttpAuthQop qop;              ///<Quality of protection
   const HashAlgo *algorithm;    ///<Digest algorithm
   const char_t *nonce;          ///<Nonce value
   size_t nonceLen;              ///<Length of the nonce value
   const char_t *opaque;         ///<Opaque parameter
   size_t opaqueLen;             ///<Length of the opaque parameter
   bool_t stale;                 ///<Stale flag
#endif
} HttpWwwAuthenticateHeader;


//HTTP client related functions
void httpClientInitAuthParams(HttpClientAuthParams *authParams);

error_t httpClientFormatAuthorizationField(HttpClientContext *context);

error_t httpClientParseWwwAuthenticateField(HttpClientContext *context,
   const char_t *value);

void httpClientParseQopParam(const HttpParam *param,
   HttpWwwAuthenticateHeader *authHeader);

void httpClientParseAlgorithmParam(const HttpParam *param,
   HttpWwwAuthenticateHeader *authHeader);

error_t httpClientComputeDigest(HttpClientAuthParams *authParams,
   const char_t *method, size_t methodLen, const char_t *uri,
   size_t uriLen, char_t *response);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
