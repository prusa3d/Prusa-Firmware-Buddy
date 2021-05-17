/**
 * @file snmp_community_mib_module.c
 * @brief SNMP COMMUNITY MIB module
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
 * The SNMP-COMMUNITY-MIB describes managed objects for mapping between
 * community strings and version-independent SNMP message parameters. Refer
 * to RFC 3584 for more details
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "mibs/mib_common.h"
#include "mibs/snmp_community_mib_module.h"
#include "mibs/snmp_community_mib_impl.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_COMMUNITY_MIB_SUPPORT == ENABLED)


/**
 * @brief SNMP COMMUNITY MIB base
 **/

SnmpCommunityMibBase snmpCommunityMibBase;


/**
 * @brief SNMP COMMUNITY MIB objects
 **/

const MibObject snmpCommunityMibObjects[] =
{
   //snmpCommunityName object (1.3.6.1.6.3.18.1.1.1.2)
   {
      "snmpCommunityName",
      {43, 6, 1, 6, 3, 18, 1, 1, 1, 2},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpCommunityMibSetCommunityEntry,
      snmpCommunityMibGetCommunityEntry,
      snmpCommunityMibGetNextCommunityEntry
   },
   //snmpCommunitySecurityName object (1.3.6.1.6.3.18.1.1.1.3)
   {
      "snmpCommunitySecurityName",
      {43, 6, 1, 6, 3, 18, 1, 1, 1, 3},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpCommunityMibSetCommunityEntry,
      snmpCommunityMibGetCommunityEntry,
      snmpCommunityMibGetNextCommunityEntry
   },
   //snmpCommunityContextEngineID object (1.3.6.1.6.3.18.1.1.1.4)
   {
      "snmpCommunityContextEngineID",
      {43, 6, 1, 6, 3, 18, 1, 1, 1, 4},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpCommunityMibSetCommunityEntry,
      snmpCommunityMibGetCommunityEntry,
      snmpCommunityMibGetNextCommunityEntry
   },
   //snmpCommunityContextName object (1.3.6.1.6.3.18.1.1.1.5)
   {
      "snmpCommunityContextName",
      {43, 6, 1, 6, 3, 18, 1, 1, 1, 5},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpCommunityMibSetCommunityEntry,
      snmpCommunityMibGetCommunityEntry,
      snmpCommunityMibGetNextCommunityEntry
   },
   //snmpCommunityTransportTag object (1.3.6.1.6.3.18.1.1.1.6)
   {
      "snmpCommunityTransportTag",
      {43, 6, 1, 6, 3, 18, 1, 1, 1, 6},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      snmpCommunityMibSetCommunityEntry,
      snmpCommunityMibGetCommunityEntry,
      snmpCommunityMibGetNextCommunityEntry
   },
   //snmpCommunityStorageType object (1.3.6.1.6.3.18.1.1.1.7)
   {
      "snmpCommunityStorageType",
      {43, 6, 1, 6, 3, 18, 1, 1, 1, 7},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      snmpCommunityMibSetCommunityEntry,
      snmpCommunityMibGetCommunityEntry,
      snmpCommunityMibGetNextCommunityEntry
   },
   //snmpCommunityStatus object (1.3.6.1.6.3.18.1.1.1.8)
   {
      "snmpCommunityStatus",
      {43, 6, 1, 6, 3, 18, 1, 1, 1, 8},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      snmpCommunityMibSetCommunityEntry,
      snmpCommunityMibGetCommunityEntry,
      snmpCommunityMibGetNextCommunityEntry
   },
};


/**
 * @brief SNMP COMMUNITY MIB module
 **/

const MibModule snmpCommunityMibModule =
{
   "SNMP-COMMUNITY-MIB",
   {43, 6, 1, 6, 3, 18},
   6,
   snmpCommunityMibObjects,
   arraysize(snmpCommunityMibObjects),
   snmpCommunityMibInit,
   snmpCommunityMibLoad,
   snmpCommunityMibUnload,
   snmpCommunityMibLock,
   snmpCommunityMibUnlock
};

#endif
