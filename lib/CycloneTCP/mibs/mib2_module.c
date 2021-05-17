/**
 * @file mib2_module.c
 * @brief MIB-II module
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
 * The second version of the Management Information Base (MIB-II) is used to
 * manage TCP/IP-based hosts. Refer to the following RFCs for complete details:
 * - RFC 1156: MIB for Network Management of TCP/IP-based internets
 * - RFC 1213: MIB for Network Management of TCP/IP-based internets (version 2)
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "mibs/mib2_module.h"
#include "mibs/mib2_impl.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MIB2_SUPPORT == ENABLED)


/**
 * @brief MIB-II base
 **/

Mib2Base mib2Base;


/**
 * @brief MIB-II objects
 **/

const MibObject mib2Objects[] =
{
#if (MIB2_SYS_GROUP_SUPPORT == ENABLED)
   //sysDescr object (1.3.6.1.2.1.1.1)
   {
      "sysDescr",
      {43, 6, 1, 2, 1, 1, 1},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_ONLY,
#if (MIB2_SYS_DESCR_SIZE > 0)
      &mib2Base.sysGroup.sysDescr,
      &mib2Base.sysGroup.sysDescrLen,
      MIB2_SYS_DESCR_SIZE,
      NULL,
      NULL,
      NULL
#else
      NULL,
      NULL,
      0,
      NULL,
      mib2GetSysDescr,
      NULL
#endif
   },
   //sysObjectID object (1.3.6.1.2.1.1.2)
   {
      "sysObjectID",
      {43, 6, 1, 2, 1, 1, 2},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OBJECT_IDENTIFIER,
      MIB_ACCESS_READ_ONLY,
#if (MIB2_SYS_OBJECT_ID_SIZE > 0)
      &mib2Base.sysGroup.sysObjectID,
      &mib2Base.sysGroup.sysObjectIDLen,
      MIB2_SYS_OBJECT_ID_SIZE,
      NULL,
      NULL,
      NULL
#else
      NULL,
      NULL,
      0,
      NULL,
      mib2GetSysObjectID,
      NULL
#endif
   },
   //sysUpTime object (1.3.6.1.2.1.1.3)
   {
      "sysUpTime",
      {43, 6, 1, 2, 1, 1, 3},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetSysUpTime,
      NULL
   },
   //sysContact object (1.3.6.1.2.1.1.4)
   {
      "sysContact",
      {43, 6, 1, 2, 1, 1, 4},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_WRITE,
#if (MIB2_SYS_CONTACT_SIZE > 0)
      &mib2Base.sysGroup.sysContact,
      &mib2Base.sysGroup.sysContactLen,
      MIB2_SYS_CONTACT_SIZE,
      NULL,
      NULL,
      NULL
#else
      NULL,
      NULL,
      0,
      mib2SetSysContact,
      mib2GetSysContact,
      NULL
#endif
   },
   //sysName object (1.3.6.1.2.1.1.5)
   {
      "sysName",
      {43, 6, 1, 2, 1, 1, 5},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_WRITE,
#if (MIB2_SYS_NAME_SIZE > 0)
      &mib2Base.sysGroup.sysName,
      &mib2Base.sysGroup.sysNameLen,
      MIB2_SYS_NAME_SIZE,
      NULL,
      NULL,
      NULL
#else
      NULL,
      NULL,
      0,
      mib2SetSysName,
      mib2GetSysName,
      NULL
#endif
   },
   //sysLocation object (1.3.6.1.2.1.1.6)
   {
      "sysLocation",
      {43, 6, 1, 2, 1, 1, 6},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_WRITE,
#if (MIB2_SYS_LOCATION_SIZE > 0)
      &mib2Base.sysGroup.sysLocation,
      &mib2Base.sysGroup.sysLocationLen,
      MIB2_SYS_LOCATION_SIZE,
      NULL,
      NULL,
      NULL
#else
      NULL,
      NULL,
      0,
      mib2SetSysLocation,
      mib2GetSysLocation,
      NULL
#endif
   },
   //sysServices object (1.3.6.1.2.1.1.7)
   {
      "sysServices",
      {43, 6, 1, 2, 1, 1, 7},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.sysGroup.sysServices,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
#endif
#if (MIB2_IF_GROUP_SUPPORT == ENABLED)
   //ifNumber object (1.3.6.1.2.1.2.1)
   {
      "ifNumber",
      {43, 6, 1, 2, 1, 2, 1},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ifGroup.ifNumber,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
   //ifIndex object (1.3.6.1.2.1.2.2.1.1)
   {
      "ifIndex",
      {43, 6, 1, 2, 1, 2, 2, 1, 1},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifDescr object (1.3.6.1.2.1.2.2.1.2)
   {
      "ifDescr",
      {43, 6, 1, 2, 1, 2, 2, 1, 2},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      0,
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifType object (1.3.6.1.2.1.2.2.1.3)
   {
      "ifType",
      {43, 6, 1, 2, 1, 2, 2, 1, 3},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifMtu object (1.3.6.1.2.1.2.2.1.4)
   {
      "ifMtu",
      {43, 6, 1, 2, 1, 2, 2, 1, 4},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifSpeed object (1.3.6.1.2.1.2.2.1.5)
   {
      "ifSpeed",
      {43, 6, 1, 2, 1, 2, 2, 1, 5},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_GAUGE32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
#if (ETH_SUPPORT == ENABLED)
   //ifPhysAddress object (1.3.6.1.2.1.2.2.1.6)
   {
      "ifPhysAddress",
      {43, 6, 1, 2, 1, 2, 2, 1, 6},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      MIB2_PHYS_ADDRESS_SIZE,
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
#endif
   //ifAdminStatus object (1.3.6.1.2.1.2.2.1.7)
   {
      "ifAdminStatus",
      {43, 6, 1, 2, 1, 2, 2, 1, 7},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      mib2SetIfEntry,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifOperStatus object (1.3.6.1.2.1.2.2.1.8)
   {
      "ifOperStatus",
      {43, 6, 1, 2, 1, 2, 2, 1, 8},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifLastChange object (1.3.6.1.2.1.2.2.1.9)
   {
      "ifLastChange",
      {43, 6, 1, 2, 1, 2, 2, 1, 9},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifInOctets object (1.3.6.1.2.1.2.2.1.10)
   {
      "ifInOctets",
      {43, 6, 1, 2, 1, 2, 2, 1, 10},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifInUcastPkts object (1.3.6.1.2.1.2.2.1.11)
   {
      "ifInUcastPkts",
      {43, 6, 1, 2, 1, 2, 2, 1, 11},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifInNUcastPkts object (1.3.6.1.2.1.2.2.1.12)
   {
      "ifInNUcastPkts",
      {43, 6, 1, 2, 1, 2, 2, 1, 12},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifInDiscards object (1.3.6.1.2.1.2.2.1.13)
   {
      "ifInDiscards",
      {43, 6, 1, 2, 1, 2, 2, 1, 13},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifInErrors object (1.3.6.1.2.1.2.2.1.14)
   {
      "ifInErrors",
      {43, 6, 1, 2, 1, 2, 2, 1, 14},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifInUnknownProtos object (1.3.6.1.2.1.2.2.1.15)
   {
      "ifInUnknownProtos",
      {43, 6, 1, 2, 1, 2, 2, 1, 15},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifOutOctets object (1.3.6.1.2.1.2.2.1.16)
   {
      "ifOutOctets",
      {43, 6, 1, 2, 1, 2, 2, 1, 16},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifOutUcastPkts object (1.3.6.1.2.1.2.2.1.17)
   {
      "ifOutUcastPkts",
      {43, 6, 1, 2, 1, 2, 2, 1, 17},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifOutNUcastPkts object (1.3.6.1.2.1.2.2.1.18)
   {
      "ifOutNUcastPkts",
      {43, 6, 1, 2, 1, 2, 2, 1, 18},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifOutDiscards object (1.3.6.1.2.1.2.2.1.19)
   {
      "ifOutDiscards",
      {43, 6, 1, 2, 1, 2, 2, 1, 19},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifOutErrors object (1.3.6.1.2.1.2.2.1.20)
   {
      "ifOutErrors",
      {43, 6, 1, 2, 1, 2, 2, 1, 20},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifOutQLen object (1.3.6.1.2.1.2.2.1.21)
   {
      "ifOutQLen",
      {43, 6, 1, 2, 1, 2, 2, 1, 21},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_GAUGE32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
   //ifSpecific object (1.3.6.1.2.1.2.2.1.22)
   {
      "ifSpecific",
      {43, 6, 1, 2, 1, 2, 2, 1, 22},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OBJECT_IDENTIFIER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      MIB2_IF_SPECIFIC_SIZE,
      NULL,
      mib2GetIfEntry,
      mib2GetNextIfEntry
   },
#endif
#if (MIB2_IP_GROUP_SUPPORT == ENABLED && IPV4_SUPPORT == ENABLED)
   //ipForwarding object (1.3.6.1.2.1.4.1)
   {
      "ipForwarding",
      {43, 6, 1, 2, 1, 4, 1},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      &mib2Base.ipGroup.ipForwarding,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
   //ipDefaultTTL object (1.3.6.1.2.1.4.2)
   {
      "ipDefaultTTL",
      {43, 6, 1, 2, 1, 4, 2},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      &mib2Base.ipGroup.ipDefaultTTL,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
   //ipInReceives object (1.3.6.1.2.1.4.3)
   {
      "ipInReceives",
      {43, 6, 1, 2, 1, 4, 3},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipInReceives,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipInHdrErrors object (1.3.6.1.2.1.4.4)
   {
      "ipInHdrErrors",
      {43, 6, 1, 2, 1, 4, 4},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipInHdrErrors,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipInAddrErrors object (1.3.6.1.2.1.4.5)
   {
      "ipInAddrErrors",
      {43, 6, 1, 2, 1, 4, 5},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipInAddrErrors,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipForwDatagrams object (1.3.6.1.2.1.4.6)
   {
      "ipForwDatagrams",
      {43, 6, 1, 2, 1, 4, 6},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipForwDatagrams,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipInUnknownProtos object (1.3.6.1.2.1.4.7)
   {
      "ipInUnknownProtos",
      {43, 6, 1, 2, 1, 4, 7},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipInUnknownProtos,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipInDiscards object (1.3.6.1.2.1.4.8)
   {
      "ipInDiscards",
      {43, 6, 1, 2, 1, 4, 8},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipInDiscards,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipInDelivers object (1.3.6.1.2.1.4.9)
   {
      "ipInDelivers",
      {43, 6, 1, 2, 1, 4, 9},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipInDelivers,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipOutRequests object (1.3.6.1.2.1.4.10)
   {
      "ipOutRequests",
      {43, 6, 1, 2, 1, 4, 10},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipOutRequests,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipOutDiscards object (1.3.6.1.2.1.4.11)
   {
      "ipOutDiscards",
      {43, 6, 1, 2, 1, 4, 11},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipOutDiscards,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipOutNoRoutes object (1.3.6.1.2.1.4.12)
   {
      "ipOutNoRoutes",
      {43, 6, 1, 2, 1, 4, 12},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipOutNoRoutes,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipReasmTimeout object (1.3.6.1.2.1.4.13)
   {
      "ipReasmTimeout",
      {43, 6, 1, 2, 1, 4, 13},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipReasmTimeout,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
   //ipReasmReqds object (1.3.6.1.2.1.4.14)
   {
      "ipReasmReqds",
      {43, 6, 1, 2, 1, 4, 14},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipReasmReqds,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipReasmOKs object (1.3.6.1.2.1.4.15)
   {
      "ipReasmOKs",
      {43, 6, 1, 2, 1, 4, 15},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipReasmOKs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipReasmFails object (1.3.6.1.2.1.4.16)
   {
      "ipReasmFails",
      {43, 6, 1, 2, 1, 4, 16},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipReasmFails,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipFragOKs object (1.3.6.1.2.1.4.17)
   {
      "ipFragOKs",
      {43, 6, 1, 2, 1, 4, 17},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipFragOKs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipFragFails object (1.3.6.1.2.1.4.18)
   {
      "ipFragFails",
      {43, 6, 1, 2, 1, 4, 18},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipFragFails,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipFragCreates object (1.3.6.1.2.1.4.19)
   {
      "ipFragCreates",
      {43, 6, 1, 2, 1, 4, 19},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipFragCreates,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipAdEntAddr object (1.3.6.1.2.1.4.20.1.1)
   {
      "ipAdEntAddr",
      {43, 6, 1, 2, 1, 4, 20, 1, 1},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_IP_ADDRESS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      MIB2_IP_ADDRESS_SIZE,
      NULL,
      mib2GetIpAddrEntry,
      mib2GetNextIpAddrEntry
   },
   //ipAdEntIfIndex object (1.3.6.1.2.1.4.20.1.2)
   {
      "ipAdEntIfIndex",
      {43, 6, 1, 2, 1, 4, 20, 1, 2},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      mib2GetIpAddrEntry,
      mib2GetNextIpAddrEntry
   },
   //ipAdEntNetMask object (1.3.6.1.2.1.4.20.1.3)
   {
      "ipAdEntNetMask",
      {43, 6, 1, 2, 1, 4, 20, 1, 3},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_IP_ADDRESS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      MIB2_IP_ADDRESS_SIZE,
      NULL,
      mib2GetIpAddrEntry,
      mib2GetNextIpAddrEntry
   },
   //ipAdEntBcastAddr object (1.3.6.1.2.1.4.20.1.4)
   {
      "ipAdEntBcastAddr",
      {43, 6, 1, 2, 1, 4, 20, 1, 4},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      mib2GetIpAddrEntry,
      mib2GetNextIpAddrEntry
   },
   //ipAdEntReasmMaxSize object (1.3.6.1.2.1.4.20.1.5)
   {
      "ipAdEntReasmMaxSize",
      {43, 6, 1, 2, 1, 4, 20, 1, 5},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      mib2GetIpAddrEntry,
      mib2GetNextIpAddrEntry
   },
   //ipNetToMediaIfIndex object (1.3.6.1.2.1.4.22.1.1)
   {
      "ipNetToMediaIfIndex",
      {43, 6, 1, 2, 1, 4, 22, 1, 1},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      mib2SetIpNetToMediaEntry,
      mib2GetIpNetToMediaEntry,
      mib2GetNextIpNetToMediaEntry
   },
   //ipNetToMediaPhysAddress object (1.3.6.1.2.1.4.22.1.2)
   {
      "ipNetToMediaPhysAddress",
      {43, 6, 1, 2, 1, 4, 22, 1, 2},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      MIB2_PHYS_ADDRESS_SIZE,
      mib2SetIpNetToMediaEntry,
      mib2GetIpNetToMediaEntry,
      mib2GetNextIpNetToMediaEntry
   },
   //ipNetToMediaNetAddress object (1.3.6.1.2.1.4.22.1.3)
   {
      "ipNetToMediaNetAddress",
      {43, 6, 1, 2, 1, 4, 22, 1, 3},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_IP_ADDRESS,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      MIB2_IP_ADDRESS_SIZE,
      mib2SetIpNetToMediaEntry,
      mib2GetIpNetToMediaEntry,
      mib2GetNextIpNetToMediaEntry
   },
   //ipNetToMediaType object (1.3.6.1.2.1.4.22.1.4)
   {
      "ipNetToMediaType",
      {43, 6, 1, 2, 1, 4, 22, 1, 4},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      mib2SetIpNetToMediaEntry,
      mib2GetIpNetToMediaEntry,
      mib2GetNextIpNetToMediaEntry
   },
   //ipRoutingDiscards object (1.3.6.1.2.1.4.23)
   {
      "ipRoutingDiscards",
      {43, 6, 1, 2, 1, 4, 23},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.ipGroup.ipRoutingDiscards,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
#endif
#if (MIB2_ICMP_GROUP_SUPPORT == ENABLED && IPV4_SUPPORT == ENABLED)
   //icmpInMsgs object (1.3.6.1.2.1.5.1)
   {
      "icmpInMsgs",
      {43, 6, 1, 2, 1, 5, 1},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInMsgs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInErrors object (1.3.6.1.2.1.5.2)
   {
      "icmpInErrors",
      {43, 6, 1, 2, 1, 5, 2},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInErrors,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInDestUnreachs object (1.3.6.1.2.1.5.3)
   {
      "icmpInDestUnreachs",
      {43, 6, 1, 2, 1, 5, 3},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInDestUnreachs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInTimeExcds object (1.3.6.1.2.1.5.4)
   {
      "icmpInTimeExcds",
      {43, 6, 1, 2, 1, 5, 4},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInTimeExcds,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInParmProbs object (1.3.6.1.2.1.5.5)
   {
      "icmpInParmProbs",
      {43, 6, 1, 2, 1, 5, 5},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInParmProbs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInSrcQuenchs object (1.3.6.1.2.1.5.6)
   {
      "icmpInSrcQuenchs",
      {43, 6, 1, 2, 1, 5, 6},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInSrcQuenchs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInRedirects object (1.3.6.1.2.1.5.7)
   {
      "icmpInRedirects",
      {43, 6, 1, 2, 1, 5, 7},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInRedirects,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInEchos object (1.3.6.1.2.1.5.8)
   {
      "icmpInEchos",
      {43, 6, 1, 2, 1, 5, 8},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInEchos,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInEchoReps object (1.3.6.1.2.1.5.9)
   {
      "icmpInEchoReps",
      {43, 6, 1, 2, 1, 5, 9},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInEchoReps,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInTimestamps object (1.3.6.1.2.1.5.10)
   {
      "icmpInTimestamps",
      {43, 6, 1, 2, 1, 5, 10},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInTimestamps,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInTimestampReps object (1.3.6.1.2.1.5.11)
   {
      "icmpInTimestampReps",
      {43, 6, 1, 2, 1, 5, 11},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInTimestampReps,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInAddrMasks object (1.3.6.1.2.1.5.12)
   {
      "icmpInAddrMasks",
      {43, 6, 1, 2, 1, 5, 12},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInAddrMasks,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpInAddrMaskReps object (1.3.6.1.2.1.5.13)
   {
      "icmpInAddrMaskReps",
      {43, 6, 1, 2, 1, 5, 13},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpInAddrMaskReps,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutMsgs object (1.3.6.1.2.1.5.14)
   {
      "icmpOutMsgs",
      {43, 6, 1, 2, 1, 5, 14},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutMsgs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutErrors object (1.3.6.1.2.1.5.15)
   {
      "icmpOutErrors",
      {43, 6, 1, 2, 1, 5, 15},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutErrors,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutDestUnreachs object (1.3.6.1.2.1.5.16)
   {
      "icmpOutDestUnreachs",
      {43, 6, 1, 2, 1, 5, 16},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutDestUnreachs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutTimeExcds object (1.3.6.1.2.1.5.17)
   {
      "icmpOutTimeExcds",
      {43, 6, 1, 2, 1, 5, 17},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutTimeExcds,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutParmProbs object (1.3.6.1.2.1.5.18)
   {
      "icmpOutParmProbs",
      {43, 6, 1, 2, 1, 5, 18},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutParmProbs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutSrcQuenchs object (1.3.6.1.2.1.5.19)
   {
      "icmpOutSrcQuenchs",
      {43, 6, 1, 2, 1, 5, 19},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutSrcQuenchs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutRedirects object (1.3.6.1.2.1.5.20)
   {
      "icmpOutRedirects",
      {43, 6, 1, 2, 1, 5, 20},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutRedirects,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutEchos object (1.3.6.1.2.1.5.21)
   {
      "icmpOutEchos",
      {43, 6, 1, 2, 1, 5, 21},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutEchos,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutEchoReps object (1.3.6.1.2.1.5.22)
   {
      "icmpOutEchoReps",
      {43, 6, 1, 2, 1, 5, 22},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutEchoReps,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutTimestamps object (1.3.6.1.2.1.5.23)
   {
      "icmpOutTimestamps",
      {43, 6, 1, 2, 1, 5, 23},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutTimestamps,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutTimestampReps object (1.3.6.1.2.1.5.24)
   {
      "icmpOutTimestampReps",
      {43, 6, 1, 2, 1, 5, 24},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutTimestampReps,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutAddrMasks object (1.3.6.1.2.1.5.25)
   {
      "icmpOutAddrMasks",
      {43, 6, 1, 2, 1, 5, 25},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutAddrMasks,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //icmpOutAddrMaskReps object (1.3.6.1.2.1.5.26)
   {
      "icmpOutAddrMaskReps",
      {43, 6, 1, 2, 1, 5, 26},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.icmpGroup.icmpOutAddrMaskReps,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
#endif
#if (MIB2_TCP_GROUP_SUPPORT == ENABLED && TCP_SUPPORT == ENABLED && IPV4_SUPPORT == ENABLED)
   //tcpRtoAlgorithm object (1.3.6.1.2.1.6.1)
   {
      "tcpRtoAlgorithm",
      {43, 6, 1, 2, 1, 6, 1},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpRtoAlgorithm,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpRtoMin object (1.3.6.1.2.1.6.2)
   {
      "tcpRtoMin",
      {43, 6, 1, 2, 1, 6, 2},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpRtoMin,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpRtoMax object (1.3.6.1.2.1.6.3)
   {
      "tcpRtoMax",
      {43, 6, 1, 2, 1, 6, 3},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpRtoMax,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpMaxConn object (1.3.6.1.2.1.6.4)
   {
      "tcpMaxConn",
      {43, 6, 1, 2, 1, 6, 4},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpMaxConn,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpActiveOpens object (1.3.6.1.2.1.6.5)
   {
      "tcpActiveOpens",
      {43, 6, 1, 2, 1, 6, 5},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpActiveOpens,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpPassiveOpens object (1.3.6.1.2.1.6.6)
   {
      "tcpPassiveOpens",
      {43, 6, 1, 2, 1, 6, 6},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpPassiveOpens,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpAttemptFails object (1.3.6.1.2.1.6.7)
   {
      "tcpAttemptFails",
      {43, 6, 1, 2, 1, 6, 7},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpAttemptFails,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpEstabResets object (1.3.6.1.2.1.6.8)
   {
      "tcpEstabResets",
      {43, 6, 1, 2, 1, 6, 8},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpEstabResets,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpCurrEstab object (1.3.6.1.2.1.6.9)
   {
      "tcpCurrEstab",
      {43, 6, 1, 2, 1, 6, 9},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_GAUGE32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      mib2GetTcpCurrEstab,
      NULL
   },
   //tcpInSegs object (1.3.6.1.2.1.6.10)
   {
      "tcpInSegs",
      {43, 6, 1, 2, 1, 6, 10},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpInSegs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpOutSegs object (1.3.6.1.2.1.6.11)
   {
      "tcpOutSegs",
      {43, 6, 1, 2, 1, 6, 11},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpOutSegs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpRetransSegs object (1.3.6.1.2.1.6.12)
   {
      "tcpRetransSegs",
      {43, 6, 1, 2, 1, 6, 12},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpRetransSegs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpConnState object (1.3.6.1.2.1.6.13.1.1)
   {
      "tcpConnState",
      {43, 6, 1, 2, 1, 6, 13, 1, 1},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      mib2SetTcpConnEntry,
      mib2GetTcpConnEntry,
      mib2GetNextTcpConnEntry
   },
   //tcpConnLocalAddress object (1.3.6.1.2.1.6.13.1.2)
   {
      "tcpConnLocalAddress",
      {43, 6, 1, 2, 1, 6, 13, 1, 2},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_IP_ADDRESS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      MIB2_IP_ADDRESS_SIZE,
      NULL,
      mib2GetTcpConnEntry,
      mib2GetNextTcpConnEntry
   },
   //tcpConnLocalPort object (1.3.6.1.2.1.6.13.1.3)
   {
      "tcpConnLocalPort",
      {43, 6, 1, 2, 1, 6, 13, 1, 3},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      mib2GetTcpConnEntry,
      mib2GetNextTcpConnEntry
   },
   //tcpConnRemAddress object (1.3.6.1.2.1.6.13.1.4)
   {
      "tcpConnRemAddress",
      {43, 6, 1, 2, 1, 6, 13, 1, 4},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_IP_ADDRESS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      MIB2_IP_ADDRESS_SIZE,
      NULL,
      mib2GetTcpConnEntry,
      mib2GetNextTcpConnEntry
   },
   //tcpConnRemPort object (1.3.6.1.2.1.6.13.1.5)
   {
      "tcpConnRemPort",
      {43, 6, 1, 2, 1, 6, 13, 1, 5},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      mib2GetTcpConnEntry,
      mib2GetNextTcpConnEntry
   },
   //tcpInErrs object (1.3.6.1.2.1.6.14)
   {
      "tcpInErrs",
      {43, 6, 1, 2, 1, 6, 14},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpInErrs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //tcpOutRsts object (1.3.6.1.2.1.6.15)
   {
      "tcpOutRsts",
      {43, 6, 1, 2, 1, 6, 15},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.tcpGroup.tcpOutRsts,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
#endif
#if (MIB2_UDP_GROUP_SUPPORT == ENABLED && UDP_SUPPORT == ENABLED && IPV4_SUPPORT == ENABLED)
   //udpInDatagrams object (1.3.6.1.2.1.7.1)
   {
      "udpInDatagrams",
      {43, 6, 1, 2, 1, 7, 1},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.udpGroup.udpInDatagrams,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //udpNoPorts object (1.3.6.1.2.1.7.2)
   {
      "udpNoPorts",
      {43, 6, 1, 2, 1, 7, 2},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.udpGroup.udpNoPorts,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //udpInErrors object (1.3.6.1.2.1.7.3)
   {
      "udpInErrors",
      {43, 6, 1, 2, 1, 7, 3},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.udpGroup.udpInErrors,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //udpOutDatagrams object (1.3.6.1.2.1.7.4)
   {
      "udpOutDatagrams",
      {43, 6, 1, 2, 1, 7, 4},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.udpGroup.udpOutDatagrams,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //udpLocalAddress object (1.3.6.1.2.1.7.5.1.1)
   {
      "udpLocalAddress",
      {43, 6, 1, 2, 1, 7, 5, 1, 1},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_IP_ADDRESS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      MIB2_IP_ADDRESS_SIZE,
      NULL,
      mib2GetUdpEntry,
      mib2GetNextUdpEntry
   },
   //udpLocalPort object (1.3.6.1.2.1.7.5.1.2)
   {
      "udpLocalPort",
      {43, 6, 1, 2, 1, 7, 5, 1, 2},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      mib2GetUdpEntry,
      mib2GetNextUdpEntry
   },
#endif
#if (MIB2_SNMP_GROUP_SUPPORT == ENABLED)
   //snmpInPkts object (1.3.6.1.2.1.11.1)
   {
      "snmpInPkts",
      {43, 6, 1, 2, 1, 11, 1},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInPkts,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpOutPkts object (1.3.6.1.2.1.11.2)
   {
      "snmpOutPkts",
      {43, 6, 1, 2, 1, 11, 2},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpOutPkts,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInBadVersions object (1.3.6.1.2.1.11.3)
   {
      "snmpInBadVersions",
      {43, 6, 1, 2, 1, 11, 3},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInBadVersions,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInBadCommunityNames object (1.3.6.1.2.1.11.4)
   {
      "snmpInBadCommunityNames",
      {43, 6, 1, 2, 1, 11, 4},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInBadCommunityNames,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInBadCommunityUses object (1.3.6.1.2.1.11.5)
   {
      "snmpInBadCommunityUses",
      {43, 6, 1, 2, 1, 11, 5},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInBadCommunityUses,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInASNParseErrs object (1.3.6.1.2.1.11.6)
   {
      "snmpInASNParseErrs",
      {43, 6, 1, 2, 1, 11, 6},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInASNParseErrs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInTooBigs object (1.3.6.1.2.1.11.8)
   {
      "snmpInTooBigs",
      {43, 6, 1, 2, 1, 11, 8},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInTooBigs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInNoSuchNames object (1.3.6.1.2.1.11.9)
   {
      "snmpInNoSuchNames",
      {43, 6, 1, 2, 1, 11, 9},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInNoSuchNames,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInBadValues object (1.3.6.1.2.1.11.10)
   {
      "snmpInBadValues",
      {43, 6, 1, 2, 1, 11, 10},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInBadValues,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInReadOnlys object (1.3.6.1.2.1.11.11)
   {
      "snmpInReadOnlys",
      {43, 6, 1, 2, 1, 11, 11},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInReadOnlys,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInGenErrs object (1.3.6.1.2.1.11.12)
   {
      "snmpInGenErrs",
      {43, 6, 1, 2, 1, 11, 12},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInGenErrs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInTotalReqVars object (1.3.6.1.2.1.11.13)
   {
      "snmpInTotalReqVars",
      {43, 6, 1, 2, 1, 11, 13},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInTotalReqVars,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInTotalSetVars object (1.3.6.1.2.1.11.14)
   {
      "snmpInTotalSetVars",
      {43, 6, 1, 2, 1, 11, 14},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInTotalSetVars,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInGetRequests object (1.3.6.1.2.1.11.15)
   {
      "snmpInGetRequests",
      {43, 6, 1, 2, 1, 11, 15},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInGetRequests,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInGetNexts object (1.3.6.1.2.1.11.16)
   {
      "snmpInGetNexts",
      {43, 6, 1, 2, 1, 11, 16},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInGetNexts,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInSetRequests object (1.3.6.1.2.1.11.17)
   {
      "snmpInSetRequests",
      {43, 6, 1, 2, 1, 11, 17},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInSetRequests,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInGetResponses object (1.3.6.1.2.1.11.18)
   {
      "snmpInGetResponses",
      {43, 6, 1, 2, 1, 11, 18},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInGetResponses,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInTraps object (1.3.6.1.2.1.11.19)
   {
      "snmpInTraps",
      {43, 6, 1, 2, 1, 11, 19},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpInTraps,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpOutTooBigs object (1.3.6.1.2.1.11.20)
   {
      "snmpOutTooBigs",
      {43, 6, 1, 2, 1, 11, 20},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpOutTooBigs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpOutNoSuchNames object (1.3.6.1.2.1.11.21)
   {
      "snmpOutNoSuchNames",
      {43, 6, 1, 2, 1, 11, 21},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpOutNoSuchNames,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpOutBadValues object (1.3.6.1.2.1.11.22)
   {
      "snmpOutBadValues",
      {43, 6, 1, 2, 1, 11, 22},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpOutBadValues,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpOutGenErrs object (1.3.6.1.2.1.11.24)
   {
      "snmpOutGenErrs",
      {43, 6, 1, 2, 1, 11, 24},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpOutGenErrs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpOutGetRequests object (1.3.6.1.2.1.11.25)
   {
      "snmpOutGetRequests",
      {43, 6, 1, 2, 1, 11, 25},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpOutGetRequests,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpOutGetNexts object (1.3.6.1.2.1.11.26)
   {
      "snmpOutGetNexts",
      {43, 6, 1, 2, 1, 11, 26},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpOutGetNexts,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpOutSetRequests object (1.3.6.1.2.1.11.27)
   {
      "snmpOutSetRequests",
      {43, 6, 1, 2, 1, 11, 27},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpOutSetRequests,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpOutGetResponses object (1.3.6.1.2.1.11.28)
   {
      "snmpOutGetResponses",
      {43, 6, 1, 2, 1, 11, 28},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpOutGetResponses,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpOutTraps object (1.3.6.1.2.1.11.29)
   {
      "snmpOutTraps",
      {43, 6, 1, 2, 1, 11, 29},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &mib2Base.snmpGroup.snmpOutTraps,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpEnableAuthenTraps object (1.3.6.1.2.1.11.30)
   {
      "snmpEnableAuthenTraps",
      {43, 6, 1, 2, 1, 11, 30},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      &mib2Base.snmpGroup.snmpEnableAuthenTraps,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   }
#endif
};


/**
 * @brief MIB-II module
 **/

const MibModule mib2Module =
{
   "RFC1213-MIB",
   {43, 6, 1, 2, 1},
   5,
   mib2Objects,
   arraysize(mib2Objects),
   mib2Init,
   NULL,
   NULL,
   NULL,
   NULL
};

#endif
