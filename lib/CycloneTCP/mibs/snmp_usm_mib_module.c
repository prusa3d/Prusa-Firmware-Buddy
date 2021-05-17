/**
 * @file snmp_usm_mib_module.c
 * @brief SNMP USM MIB module
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
 * @section Description
 *
 * The SNMP-USM-MIB describes managed objects for remotely monitoring and
 * managing the configuration parameters for the User-Based Security Model.
 * Refer to RFC 3414 for more details
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "mibs/mib_common.h"
#include "mibs/snmp_usm_mib_module.h"
#include "mibs/snmp_usm_mib_impl.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_USM_MIB_SUPPORT == ENABLED)


/**
 * @brief SNMP USM MIB base
 **/

SnmpUsmMibBase snmpUsmMibBase;


/**
 * @brief SNMP USM MIB objects
 **/

const MibObject snmpUsmMibObjects[] =
{
   //usmStatsUnsupportedSecLevels object (1.3.6.1.6.3.15.1.1.1)
   {
      "usmStatsUnsupportedSecLevels",
      {43, 6, 1, 6, 3, 15, 1, 1, 1},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &snmpUsmMibBase.usmStatsUnsupportedSecLevels,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //usmStatsNotInTimeWindows object (1.3.6.1.6.3.15.1.1.2)
   {
      "usmStatsNotInTimeWindows",
      {43, 6, 1, 6, 3, 15, 1, 1, 2},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &snmpUsmMibBase.usmStatsNotInTimeWindows,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //usmStatsUnknownUserNames object (1.3.6.1.6.3.15.1.1.3)
   {
      "usmStatsUnknownUserNames",
      {43, 6, 1, 6, 3, 15, 1, 1, 3},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &snmpUsmMibBase.usmStatsUnknownUserNames,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //usmStatsUnknownEngineIDs object (1.3.6.1.6.3.15.1.1.4)
   {
      "usmStatsUnknownEngineIDs",
      {43, 6, 1, 6, 3, 15, 1, 1, 4},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &snmpUsmMibBase.usmStatsUnknownEngineIDs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //usmStatsWrongDigests object (1.3.6.1.6.3.15.1.1.5)
   {
      "usmStatsWrongDigests",
      {43, 6, 1, 6, 3, 15, 1, 1, 5},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &snmpUsmMibBase.usmStatsWrongDigests,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //usmStatsDecryptionErrors object (1.3.6.1.6.3.15.1.1.6)
   {
      "usmStatsDecryptionErrors",
      {43, 6, 1, 6, 3, 15, 1, 1, 6},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &snmpUsmMibBase.usmStatsDecryptionErrors,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //usmUserSpinLock object (1.3.6.1.6.3.15.1.2.1)
   {
      "usmUserSpinLock",
      {43, 6, 1, 6, 3, 15, 1, 2, 1},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      snmpUsmMibSetUserSpinLock,
      snmpUsmMibGetUserSpinLock,
      NULL
   },
   //usmUserSecurityName object (1.3.6.1.6.3.15.1.2.2.1.3)
   {
      "usmUserSecurityName",
      {43, 6, 1, 6, 3, 15, 1, 2, 2, 1, 3},
      11,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      0,
      NULL,
      snmpUsmMibGetUserEntry,
      snmpUsmMibGetNextUserEntry
   },
   //usmUserCloneFrom object (1.3.6.1.6.3.15.1.2.2.1.4)
   {
      "usmUserCloneFrom",
      {43, 6, 1, 6, 3, 15, 1, 2, 2, 1, 4},
      11,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OBJECT_IDENTIFIER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpUsmMibSetUserEntry,
      snmpUsmMibGetUserEntry,
      snmpUsmMibGetNextUserEntry
   },
   //usmUserAuthProtocol object (1.3.6.1.6.3.15.1.2.2.1.5)
   {
      "usmUserAuthProtocol",
      {43, 6, 1, 6, 3, 15, 1, 2, 2, 1, 5},
      11,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OBJECT_IDENTIFIER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpUsmMibSetUserEntry,
      snmpUsmMibGetUserEntry,
      snmpUsmMibGetNextUserEntry
   },
   //usmUserAuthKeyChange object (1.3.6.1.6.3.15.1.2.2.1.6)
   {
      "usmUserAuthKeyChange",
      {43, 6, 1, 6, 3, 15, 1, 2, 2, 1, 6},
      11,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpUsmMibSetUserEntry,
      snmpUsmMibGetUserEntry,
      snmpUsmMibGetNextUserEntry
   },
   //usmUserOwnAuthKeyChange object (1.3.6.1.6.3.15.1.2.2.1.7)
   {
      "usmUserOwnAuthKeyChange",
      {43, 6, 1, 6, 3, 15, 1, 2, 2, 1, 7},
      11,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpUsmMibSetUserEntry,
      snmpUsmMibGetUserEntry,
      snmpUsmMibGetNextUserEntry
   },
   //usmUserPrivProtocol object (1.3.6.1.6.3.15.1.2.2.1.8)
   {
      "usmUserPrivProtocol",
      {43, 6, 1, 6, 3, 15, 1, 2, 2, 1, 8},
      11,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OBJECT_IDENTIFIER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpUsmMibSetUserEntry,
      snmpUsmMibGetUserEntry,
      snmpUsmMibGetNextUserEntry
   },
   //usmUserPrivKeyChange object (1.3.6.1.6.3.15.1.2.2.1.9)
   {
      "usmUserPrivKeyChange",
      {43, 6, 1, 6, 3, 15, 1, 2, 2, 1, 9},
      11,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpUsmMibSetUserEntry,
      snmpUsmMibGetUserEntry,
      snmpUsmMibGetNextUserEntry
   },
   //usmUserOwnPrivKeyChange object (1.3.6.1.6.3.15.1.2.2.1.10)
   {
      "usmUserOwnPrivKeyChange",
      {43, 6, 1, 6, 3, 15, 1, 2, 2, 1, 10},
      11,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpUsmMibSetUserEntry,
      snmpUsmMibGetUserEntry,
      snmpUsmMibGetNextUserEntry
   },
   //usmUserPublic object (1.3.6.1.6.3.15.1.2.2.1.11)
   {
      "usmUserPublic",
      {43, 6, 1, 6, 3, 15, 1, 2, 2, 1, 11},
      11,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpUsmMibSetUserEntry,
      snmpUsmMibGetUserEntry,
      snmpUsmMibGetNextUserEntry
   },
   //usmUserStorageType object (1.3.6.1.6.3.15.1.2.2.1.12)
   {
      "usmUserStorageType",
      {43, 6, 1, 6, 3, 15, 1, 2, 2, 1, 12},
      11,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      snmpUsmMibSetUserEntry,
      snmpUsmMibGetUserEntry,
      snmpUsmMibGetNextUserEntry
   },
   //usmUserStatus object (1.3.6.1.6.3.15.1.2.2.1.13)
   {
      "usmUserStatus",
      {43, 6, 1, 6, 3, 15, 1, 2, 2, 1, 13},
      11,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      snmpUsmMibSetUserEntry,
      snmpUsmMibGetUserEntry,
      snmpUsmMibGetNextUserEntry
   }
};


/**
 * @brief SNMP USM MIB module
 **/

const MibModule snmpUsmMibModule =
{
   "SNMP-USM-MIB",
   {43, 6, 1, 6, 3, 15},
   6,
   snmpUsmMibObjects,
   arraysize(snmpUsmMibObjects),
   snmpUsmMibInit,
   snmpUsmMibLoad,
   snmpUsmMibUnload,
   snmpUsmMibLock,
   snmpUsmMibUnlock
};

#endif
