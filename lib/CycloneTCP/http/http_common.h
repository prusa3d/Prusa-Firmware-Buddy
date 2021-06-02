/**
 * @file http_common.h
 * @brief Definitions common to HTTP client and server
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

#ifndef _HTTP_COMMON_H
#define _HTTP_COMMON_H

//Dependencies
#include "net.h"

//HTTP port number
#define HTTP_PORT 80
//HTTPS port number (HTTP over TLS)
#define HTTPS_PORT 443

//Test macros for HTTP status codes
#define HTTP_STATUS_CODE_1YZ(code) ((code) >= 100 && (code) < 200)
#define HTTP_STATUS_CODE_2YZ(code) ((code) >= 200 && (code) < 300)
#define HTTP_STATUS_CODE_3YZ(code) ((code) >= 300 && (code) < 400)
#define HTTP_STATUS_CODE_4YZ(code) ((code) >= 400 && (code) < 500)
#define HTTP_STATUS_CODE_5YZ(code) ((code) >= 500 && (code) < 600)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief HTTP version numbers
 **/

typedef enum
{
   HTTP_VERSION_0_9 = 0x0009,
   HTTP_VERSION_1_0 = 0x0100,
   HTTP_VERSION_1_1 = 0x0101
} HttpVersion;


/**
 * @brief HTTP authentication schemes
 **/

typedef enum
{
   HTTP_AUTH_MODE_NONE   = 0,
   HTTP_AUTH_MODE_BASIC  = 1,
   HTTP_AUTH_MODE_DIGEST = 2
} HttpAuthMode;


/**
 * @brief Quality of protection (digest authentication)
 **/

typedef enum
{
   HTTP_AUTH_QOP_NONE     = 0,
   HTTP_AUTH_QOP_AUTH     = 1,
   HTTP_AUTH_QOP_AUTH_INT = 2
} HttpAuthQop;


/**
 * @brief Flags used by I/O functions
 **/

typedef enum
{
   HTTP_FLAG_WAIT_ALL   = 0x0800,
   HTTP_FLAG_BREAK_CHAR = 0x1000,
   HTTP_FLAG_BREAK_CRLF = 0x100A,
   HTTP_FLAG_NO_DELAY   = 0x4000,
   HTTP_FLAG_DELAY      = 0x8000
} HttpFlags;


/**
 * @brief HTTP request states
 */

typedef enum
{
   HTTP_REQ_STATE_INIT                = 0,
   HTTP_REQ_STATE_FORMAT_HEADER       = 1,
   HTTP_REQ_STATE_SEND_HEADER         = 2,
   HTTP_REQ_STATE_FORMAT_BODY         = 3,
   HTTP_REQ_STATE_SEND_BODY           = 4,
   HTTP_REQ_STATE_SEND_CHUNK_SIZE     = 5,
   HTTP_REQ_STATE_SEND_CHUNK_DATA     = 6,
   HTTP_REQ_STATE_FORMAT_TRAILER      = 7,
   HTTP_REQ_STATE_SEND_TRAILER        = 8,
   HTTP_REQ_STATE_RECEIVE_STATUS_LINE = 9,
   HTTP_REQ_STATE_RECEIVE_HEADER      = 10,
   HTTP_REQ_STATE_PARSE_HEADER        = 11,
   HTTP_REQ_STATE_RECEIVE_BODY        = 12,
   HTTP_REQ_STATE_RECEIVE_CHUNK_SIZE  = 13,
   HTTP_REQ_STATE_RECEIVE_CHUNK_DATA  = 14,
   HTTP_REQ_STATE_PARSE_BODY          = 15,
   HTTP_REQ_STATE_RECEIVE_TRAILER     = 16,
   HTTP_REQ_STATE_PARSE_TRAILER       = 17,
   HTTP_REQ_STATE_COMPLETE            = 18
} HttpRequestState;


/**
 * @brief HTTP character sets
 */

typedef enum
{
   HTTP_CHARSET_OCTET    = 0x0001,
   HTTP_CHARSET_CTL      = 0x0002,
   HTTP_CHARSET_LWS      = 0x0004,
   HTTP_CHARSET_ALPHA    = 0x0008,
   HTTP_CHARSET_DIGIT    = 0x0010,
   HTTP_CHARSET_HEX      = 0x0020,
   HTTP_CHARSET_VCHAR    = 0x0040,
   HTTP_CHARSET_TCHAR    = 0x0080,
   HTTP_CHARSET_TEXT     = 0x0100,
   HTTP_CHARSET_OBS_TEXT = 0x0200
} HttpCharset;


/**
 * @brief Attribute-value pair
 **/

typedef struct
{
   const char_t *name;
   size_t nameLen;
   const char_t *value;
   size_t valueLen;
} HttpParam;


//HTTP related functions
error_t httpCheckCharset(const char_t *s, size_t length, uint_t charset);

error_t httpParseParam(const char_t **pos, HttpParam *param);
bool_t httpCompareParamName(const HttpParam *param, const char_t *name);
bool_t httpCompareParamValue(const HttpParam *param, const char_t *value);

error_t httpCopyParamValue(const HttpParam *param, char_t *value,
   size_t maxLen);

void httpEncodeHexString(const uint8_t *input, size_t inputLen, char_t *output);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
