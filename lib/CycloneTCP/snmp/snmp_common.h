/**
 * @file snmp_common.h
 * @brief Definitions common to SNMP agent and SNMP manager
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

#ifndef _SNMP_COMMON_H
#define _SNMP_COMMON_H

//Dependencies
#include "core/net.h"

//SNMPv1 support
#ifndef SNMP_V1_SUPPORT
   #define SNMP_V1_SUPPORT ENABLED
#elif (SNMP_V1_SUPPORT != ENABLED && SNMP_V1_SUPPORT != DISABLED)
   #error SNMP_V1_SUPPORT parameter is not valid
#endif

//SNMPv2c support
#ifndef SNMP_V2C_SUPPORT
   #define SNMP_V2C_SUPPORT ENABLED
#elif (SNMP_V2C_SUPPORT != ENABLED && SNMP_V2C_SUPPORT != DISABLED)
   #error SNMP_V2C_SUPPORT parameter is not valid
#endif

//SNMPv3 support
#ifndef SNMP_V3_SUPPORT
   #define SNMP_V3_SUPPORT DISABLED
#elif (SNMP_V3_SUPPORT != ENABLED && SNMP_V3_SUPPORT != DISABLED)
   #error SNMP_V3_SUPPORT parameter is not valid
#endif

//Maximum size of SNMP messages
#ifndef SNMP_MAX_MSG_SIZE
   #define SNMP_MAX_MSG_SIZE 1452
#elif (SNMP_MAX_MSG_SIZE < 484 || SNMP_MAX_MSG_SIZE > 65535)
   #error SNMP_MAX_MSG_SIZE parameter is not valid
#endif

//Maximum size for context engine identifier
#ifndef SNMP_MAX_CONTEXT_ENGINE_SIZE
   #define SNMP_MAX_CONTEXT_ENGINE_SIZE 32
#elif (SNMP_MAX_CONTEXT_ENGINE_SIZE < 1)
   #error SNMP_MAX_CONTEXT_ENGINE_SIZE parameter is not valid
#endif

//Maximum length for context name
#ifndef SNMP_MAX_CONTEXT_NAME_LEN
   #define SNMP_MAX_CONTEXT_NAME_LEN 32
#elif (SNMP_MAX_CONTEXT_NAME_LEN < 1)
   #error SNMP_MAX_CONTEXT_NAME_LEN parameter is not valid
#endif

//Maximum length for user names and community names
#ifndef SNMP_MAX_USER_NAME_LEN
   #define SNMP_MAX_USER_NAME_LEN 32
#elif (SNMP_MAX_USER_NAME_LEN < 1)
   #error SNMP_MAX_USER_NAME_LEN parameter is not valid
#endif

//Maximum size for user public values
#ifndef SNMP_MAX_PUBLIC_VALUE_SIZE
   #define SNMP_MAX_PUBLIC_VALUE_SIZE 32
#elif (SNMP_MAX_PUBLIC_VALUE_SIZE < 1)
   #error SNMP_MAX_PUBLIC_VALUE_SIZE parameter is not valid
#endif

//Maximum length for group names
#ifndef SNMP_MAX_GROUP_NAME_LEN
   #define SNMP_MAX_GROUP_NAME_LEN 32
#elif (SNMP_MAX_GROUP_NAME_LEN < 1)
   #error SNMP_MAX_GROUP_NAME_LEN parameter is not valid
#endif

//Maximum length for view names
#ifndef SNMP_MAX_VIEW_NAME_LEN
   #define SNMP_MAX_VIEW_NAME_LEN 32
#elif (SNMP_MAX_VIEW_NAME_LEN < 1)
   #error SNMP_MAX_VIEW_NAME_LEN parameter is not valid
#endif

//Maximum length for bit masks
#ifndef SNMP_MAX_BIT_MASK_SIZE
   #define SNMP_MAX_BIT_MASK_SIZE 16
#elif (SNMP_MAX_BIT_MASK_SIZE < 1)
   #error SNMP_MAX_MASK_SIZE parameter is not valid
#endif

//Maximum size for object identifiers
#ifndef SNMP_MAX_OID_SIZE
   #define SNMP_MAX_OID_SIZE 16
#elif (SNMP_MAX_OID_SIZE < 1)
   #error SNMP_MAX_OID_SIZE parameter is not valid
#endif

//SNMP port number
#define SNMP_PORT 161
//SNMP trap port number
#define SNMP_TRAP_PORT 162

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief SNMP version identifiers
 **/

typedef enum
{
   SNMP_VERSION_1  = 0,
   SNMP_VERSION_2C = 1,
   SNMP_VERSION_3  = 3
} SnmpVersion;


/**
 * @brief SNMP PDU types
 **/

typedef enum
{
   SNMP_PDU_GET_REQUEST      = 0,
   SNMP_PDU_GET_NEXT_REQUEST = 1,
   SNMP_PDU_GET_RESPONSE     = 2,
   SNMP_PDU_SET_REQUEST      = 3,
   SNMP_PDU_TRAP             = 4,
   SNMP_PDU_GET_BULK_REQUEST = 5,
   SNMP_PDU_INFORM_REQUEST   = 6,
   SNMP_PDU_TRAP_V2          = 7,
   SNMP_PDU_REPORT           = 8
} SnmpPduType;


/**
 * @brief SNMP generic trap types
 **/

typedef enum
{
   SNMP_TRAP_COLD_START          = 0,
   SNMP_TRAP_WARM_START          = 1,
   SNMP_TRAP_LINK_DOWN           = 2,
   SNMP_TRAP_LINK_UP             = 3,
   SNMP_TRAP_AUTH_FAILURE        = 4,
   SNMP_TRAP_EGP_NEIGHBOR_LOSS   = 5,
   SNMP_TRAP_ENTERPRISE_SPECIFIC = 6
} SnmpGenericTrapType;


/**
 * @brief SNMP error status
 **/

typedef enum
{
   SNMP_ERROR_NONE                 = 0,
   SNMP_ERROR_TOO_BIG              = 1,
   SNMP_ERROR_NO_SUCH_NAME         = 2,
   SNMP_ERROR_BAD_VALUE            = 3,
   SNMP_ERROR_READ_ONLY            = 4,
   SNMP_ERROR_GENERIC              = 5,
   SNMP_ERROR_NO_ACCESS            = 6,
   SNMP_ERROR_WRONG_TYPE           = 7,
   SNMP_ERROR_WRONG_LENGTH         = 8,
   SNMP_ERROR_WRONG_ENCODING       = 9,
   SNMP_ERROR_WRONG_VALUE          = 10,
   SNMP_ERROR_NO_CREATION          = 11,
   SNMP_ERROR_INCONSISTENT_VALUE   = 12,
   SNMP_ERROR_RESOURCE_UNAVAILABLE = 13,
   SNMP_ERROR_COMMIT_FAILED        = 14,
   SNMP_ERROR_UNDO_FAILED          = 15,
   SNMP_ERROR_AUTHORIZATION        = 16,
   SNMP_ERROR_NOT_WRITABLE         = 17,
   SNMP_ERROR_INCONSISTENT_NAME    = 18
} SnmpErrorStatus;


/**
 * @brief SNMP exceptions
 **/

typedef enum
{
   SNMP_EXCEPTION_NO_SUCH_OBJECT   = 0,
   SNMP_EXCEPTION_NO_SUCH_INSTANCE = 1,
   SNMP_EXCEPTION_END_OF_MIB_VIEW  = 2
} SnmpException;


/**
 * @brief SNMP engine ID format
 **/

typedef enum
{
   SNMP_ENGINE_ID_FORMAT_IPV4   = 1,
   SNMP_ENGINE_ID_FORMAT_IPV6   = 2,
   SNMP_ENGINE_ID_FORMAT_MAC    = 3,
   SNMP_ENGINE_ID_FORMAT_TEXT   = 4,
   SNMP_ENGINE_ID_FORMAT_OCTETS = 5,
} SnmpEngineIdFormat;


//C++ guard
#ifdef __cplusplus
}
#endif

#endif
