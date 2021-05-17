/**
 * @file mib_common.h
 * @brief Common definitions for MIB modules
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

#ifndef _MIB_COMMON_H
#define _MIB_COMMON_H

//Dependencies
#include "core/net.h"

//Maximum OID size
#ifndef MIB_MAX_OID_SIZE
   #define MIB_MAX_OID_SIZE 16
#elif (MIB_MAX_OID_SIZE < 1)
   #error MIB_MAX_OID_SIZE parameter is not valid
#endif

//Forward declaration of MibObject structure
struct _MibObject;
#define MibObject struct _MibObject

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief MIB object types
 **/

typedef enum
{
   MIB_TYPE_IP_ADDRESS        = 0,
   MIB_TYPE_COUNTER32         = 1,
   MIB_TYPE_GAUGE32           = 2,
   MIB_TYPE_UNSIGNED32        = 2,
   MIB_TYPE_TIME_TICKS        = 3,
   MIB_TYPE_OPAQUE            = 4,
   MIB_TYPE_COUNTER64         = 6
} MibType;


/**
 * @brief Access modes
 **/

typedef enum
{
   MIB_ACCESS_NONE        = 0,
   MIB_ACCESS_FOR_NOTIFY  = 1,
   MIB_ACCESS_WRITE_ONLY  = 2,
   MIB_ACCESS_READ_ONLY   = 3,
   MIB_ACCESS_READ_WRITE  = 4,
   MIB_ACCESS_READ_CREATE = 5,
} MibAccess;


/**
 * @brief Truth value
 **/

typedef enum
{
   MIB_TRUTH_VALUE_TRUE  = 1,
   MIB_TRUTH_VALUE_FALSE = 2
} MibTruthValue;


/**
 * @brief Row status
 **/

typedef enum
{
   MIB_ROW_STATUS_UNUSED          = 0,
   MIB_ROW_STATUS_ACTIVE          = 1,
   MIB_ROW_STATUS_NOT_IN_SERVICE  = 2,
   MIB_ROW_STATUS_NOT_READY       = 3,
   MIB_ROW_STATUS_CREATE_AND_GO   = 4,
   MIB_ROW_STATUS_CREATE_AND_WAIT = 5,
   MIB_ROW_STATUS_DESTROY         = 6
} MibRowStatus;


/**
 * @brief Storage type
 **/

typedef enum
{
   MIB_STORAGE_TYPE_OTHER        = 1,
   MIB_STORAGE_TYPE_VOLATILE     = 2,
   MIB_STORAGE_TYPE_NON_VOLATILE = 3,
   MIB_STORAGE_TYPE_PERMANENT    = 4,
   MIB_STORAGE_TYPE_READ_ONLY    = 5,
} MibStorageType;


/**
 * @brief Internet address types
 **/

typedef enum
{
   INET_ADDR_TYPE_UNKNOWN = 0,
   INET_ADDR_TYPE_IPV4    = 1,
   INET_ADDR_TYPE_IPV6    = 2,
   INET_ADDR_TYPE_IPV4Z   = 3,
   INET_ADDR_TYPE_IPV6Z   = 4,
   INET_ADDR_TYPE_DNS     = 16
} InetAddrType;


/**
 * @brief Internet address scope
 **/

typedef enum
{
   INET_SCOPE_TYPE_RESERVED           = 0,
   INET_SCOPE_TYPE_INTERFACE_LOCAL    = 1,
   INET_SCOPE_TYPE_LINK_LOCAL         = 2,
   INET_SCOPE_TYPE_SUBNET_LOCAL       = 3,
   INET_SCOPE_TYPE_ADMIN_LOCAL        = 4,
   INET_SCOPE_TYPE_SITE_LOCAL         = 5,
   INET_SCOPE_TYPE_ORGANIZATION_LOCAL = 8,
   INET_SCOPE_TYPE_GLOBAL             = 14
} InetScopeType;


/**
 * @brief IP protocol version
 **/

typedef enum
{
   INET_VERSION_UNKNOWN = 0,
   INET_VERSION_IPV4    = 1,
   INET_VERSION_IPV6    = 2
} InetVersion;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief Variant data type
 **/

typedef __start_packed struct
{
   __start_packed union
   {
      int32_t integer;
      uint8_t octetString[1];
      uint8_t oid[1];
      uint8_t ipAddr[4];
      uint32_t counter32;
      uint32_t gauge32;
      uint32_t unsigned32;
      uint32_t timeTicks;
      uint64_t counter64;
   };
} __end_packed MibVariant;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


/**
 * @brief Set object value
 **/

typedef error_t (*MibSetValue)(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit);


/**
 * @brief Get object value
 **/

typedef error_t (*MibGetValue)(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen);


/**
 * @brief Get next object
 **/

typedef error_t (*MibGetNext)(const MibObject *object, const uint8_t *oid,
   size_t oidLen, uint8_t *nextOid, size_t *nextOidLen);


/**
 * @brief MIB object descriptor
 **/

struct _MibObject
{
   const char_t *name;
   uint8_t oid[MIB_MAX_OID_SIZE];
   size_t oidLen;
   uint_t objClass;
   uint_t objType;
   MibAccess access;
   void *value;
   size_t *valueLen;
   size_t valueSize;
   MibSetValue setValue;
   MibGetValue getValue;
   MibGetNext getNext;
};


/**
 * @brief MIB initialization
 **/

typedef error_t (*MibInit)(void);


/**
 * @brief Load MIB
 **/

typedef error_t (*MibLoad)(void *context);


/**
 * @brief Unload MIB
 **/

typedef void (*MibUnload)(void *context);


/**
 * @brief Lock MIB
 **/

typedef void (*MibLock)(void);


/**
 * @brief Unlock MIB
 **/

typedef void (*MibUnlock)(void);


/**
 * @brief MIB module
 **/

typedef struct
{
   const char_t *name;
   uint8_t oid[MIB_MAX_OID_SIZE];
   size_t oidLen;
   const MibObject *objects;
   uint_t numObjects;
   MibInit init;
   MibLoad load;
   MibUnload unload;
   MibLock lock;
   MibUnlock unlock;
} MibModule;


//MIB related functions
error_t mibEncodeIndex(uint8_t *oid, size_t maxOidLen, size_t *pos,
   uint_t index);

error_t mibDecodeIndex(const uint8_t *oid, size_t oidLen, size_t *pos,
   uint_t *index);

error_t mibEncodeUnsigned32(uint8_t *oid, size_t maxOidLen, size_t *pos,
   uint32_t value);

error_t mibDecodeUnsigned32(const uint8_t *oid, size_t oidLen, size_t *pos,
   uint32_t *value);

error_t mibEncodeString(uint8_t *oid, size_t maxOidLen, size_t *pos,
   const char_t *string, bool_t implied);

error_t mibDecodeString(const uint8_t *oid, size_t oidLen, size_t *pos,
   char_t *string, size_t maxStringLen, bool_t implied);

error_t mibEncodeOctetString(uint8_t *oid, size_t maxOidLen, size_t *pos,
   const uint8_t *data, size_t dataLen, bool_t implied);

error_t mibDecodeOctetString(const uint8_t *oid, size_t oidLen, size_t *pos,
   uint8_t *data, size_t maxDataLen, size_t *dataLen, bool_t implied);

error_t mibEncodeObjectIdentifier(uint8_t *oid, size_t maxOidLen, size_t *pos,
   const uint8_t *objectId, size_t objectIdLen, bool_t implied);

error_t mibDecodeObjectIdentifier(const uint8_t *oid, size_t oidLen, size_t *pos,
   uint8_t *objectId, size_t maxObjectIdLen, size_t *objectIdLen, bool_t implied);

error_t mibEncodePort(uint8_t *oid, size_t maxOidLen, size_t *pos,
   uint16_t port);

error_t mibDecodePort(const uint8_t *oid, size_t oidLen, size_t *pos,
   uint16_t *port);

error_t mibEncodeMacAddr(uint8_t *oid, size_t maxOidLen, size_t *pos,
   const MacAddr *macAddr);

error_t mibDecodeMacAddr(const uint8_t *oid, size_t oidLen, size_t *pos,
   MacAddr *macAddr);

error_t mibEncodePhysAddr(uint8_t *oid, size_t maxOidLen, size_t *pos,
   const MacAddr *macAddr);

error_t mibDecodePhysAddr(const uint8_t *oid, size_t oidLen, size_t *pos,
   MacAddr *macAddr);

error_t mibEncodeIpv4Addr(uint8_t *oid, size_t maxOidLen, size_t *pos,
   Ipv4Addr ipAddr);

error_t mibDecodeIpv4Addr(const uint8_t *oid, size_t oidLen, size_t *pos,
   Ipv4Addr *ipAddr);

error_t mibEncodeIpv6Addr(uint8_t *oid, size_t maxOidLen, size_t *pos,
   const Ipv6Addr *ipAddr);

error_t mibDecodeIpv6Addr(const uint8_t *oid, size_t oidLen, size_t *pos,
   Ipv6Addr *ipAddr);

error_t mibEncodeIpAddr(uint8_t *oid, size_t maxOidLen, size_t *pos,
   const IpAddr *ipAddr);

error_t mibDecodeIpAddr(const uint8_t *oid, size_t oidLen, size_t *pos,
   IpAddr *ipAddr);

int_t mibCompMacAddr(const MacAddr *macAddr1, const MacAddr *macAddr2);
int_t mibCompIpAddr(const IpAddr *ipAddr1, const IpAddr *ipAddr2);

error_t mibTestAndIncSpinLock(int32_t *spinLock, int32_t value, bool_t commit);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
