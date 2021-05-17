/**
 * @file if_mib_module.c
 * @brief Interfaces Group MIB module
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
 * The IF-MIB describes managed objects used for managing network
 * interfaces. Refer to the following RFCs for complete details:
 * - RFC 2233: The Interfaces Group MIB using SMIv2
 * - RFC 2863: The Interfaces Group MIB
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "mibs/mib_common.h"
#include "mibs/if_mib_module.h"
#include "mibs/if_mib_impl.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IF_MIB_SUPPORT == ENABLED)


/**
 * @brief Interfaces Group MIB base
 **/

IfMibBase ifMibBase;


/**
 * @brief Interfaces Group MIB objects
 **/

const MibObject ifMibObjects[] =
{
   //ifNumber object (1.3.6.1.2.1.2.1)
   {
      "ifNumber",
      {43, 6, 1, 2, 1, 2, 1},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      &ifMibBase.ifNumber,
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      0,
      NULL,
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibSetIfEntry,
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
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
      ifMibGetIfEntry,
      ifMibGetNextIfEntry
   },
   //ifName object (1.3.6.1.2.1.31.1.1.1.1)
   {
      "ifName",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 1},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      0,
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifInMulticastPkts object (1.3.6.1.2.1.31.1.1.1.2)
   {
      "ifInMulticastPkts",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 2},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifInBroadcastPkts object (1.3.6.1.2.1.31.1.1.1.3)
   {
      "ifInBroadcastPkts",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 3},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifOutMulticastPkts object (1.3.6.1.2.1.31.1.1.1.4)
   {
      "ifOutMulticastPkts",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 4},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifOutBroadcastPkts object (1.3.6.1.2.1.31.1.1.1.5)
   {
      "ifOutBroadcastPkts",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 5},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifHCInOctets object (1.3.6.1.2.1.31.1.1.1.6)
   {
      "ifHCInOctets",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 6},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifHCInUcastPkts object (1.3.6.1.2.1.31.1.1.1.7)
   {
      "ifHCInUcastPkts",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 7},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifHCInMulticastPkts object (1.3.6.1.2.1.31.1.1.1.8)
   {
      "ifHCInMulticastPkts",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 8},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifHCInBroadcastPkts object (1.3.6.1.2.1.31.1.1.1.9)
   {
      "ifHCInBroadcastPkts",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 9},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifHCOutOctets object (1.3.6.1.2.1.31.1.1.1.10)
   {
      "ifHCOutOctets",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 10},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifHCOutUcastPkts object (1.3.6.1.2.1.31.1.1.1.11)
   {
      "ifHCOutUcastPkts",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 11},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifHCOutMulticastPkts object (1.3.6.1.2.1.31.1.1.1.12)
   {
      "ifHCOutMulticastPkts",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 12},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifHCOutBroadcastPkts object (1.3.6.1.2.1.31.1.1.1.13)
   {
      "ifHCOutBroadcastPkts",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 13},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifLinkUpDownTrapEnable object (1.3.6.1.2.1.31.1.1.1.14)
   {
      "ifLinkUpDownTrapEnable",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 14},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      ifMibSetIfXEntry,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifHighSpeed object (1.3.6.1.2.1.31.1.1.1.15)
   {
      "ifHighSpeed",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 15},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_GAUGE32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifPromiscuousMode object (1.3.6.1.2.1.31.1.1.1.16)
   {
      "ifPromiscuousMode",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 16},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      ifMibSetIfXEntry,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifConnectorPresent object (1.3.6.1.2.1.31.1.1.1.17)
   {
      "ifConnectorPresent",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 17},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifAlias object (1.3.6.1.2.1.31.1.1.1.18)
   {
      "ifAlias",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 18},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      0,
      ifMibSetIfXEntry,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifCounterDiscontinuityTime object (1.3.6.1.2.1.31.1.1.1.19)
   {
      "ifCounterDiscontinuityTime",
      {43, 6, 1, 2, 1, 31, 1, 1, 1, 19},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ifMibGetIfXEntry,
      ifMibGetNextIfXEntry
   },
   //ifStackStatus object (1.3.6.1.2.1.31.1.2.1.3)
   {
      "ifStackStatus",
      {43, 6, 1, 2, 1, 31, 1, 2, 1, 3},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ifMibSetIfStackEntry,
      ifMibGetIfStackEntry,
      ifMibGetNextIfStackEntry
   },
   //ifRcvAddressStatus object (1.3.6.1.2.1.31.1.4.1.2)
   {
      "ifRcvAddressStatus",
      {43, 6, 1, 2, 1, 31, 1, 4, 1, 2},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ifMibSetIfRcvAddressEntry,
      ifMibGetIfRcvAddressEntry,
      ifMibGetNextIfRcvAddressEntry
   },
   //ifRcvAddressType object (1.3.6.1.2.1.31.1.4.1.3)
   {
      "ifRcvAddressType",
      {43, 6, 1, 2, 1, 31, 1, 4, 1, 3},
      10,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ifMibSetIfRcvAddressEntry,
      ifMibGetIfRcvAddressEntry,
      ifMibGetNextIfRcvAddressEntry
   },
   //ifTableLastChange object (1.3.6.1.2.1.31.1.5)
   {
      "ifTableLastChange",
      {43, 6, 1, 2, 1, 31, 1, 5},
      8,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      &ifMibBase.ifTableLastChange,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ifStackLastChange object (1.3.6.1.2.1.31.1.6)
   {
      "ifStackLastChange",
      {43, 6, 1, 2, 1, 31, 1, 6},
      8,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      &ifMibBase.ifStackLastChange,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   }
};


/**
 * @brief Interfaces Group MIB module
 **/

const MibModule ifMibModule =
{
   "IF-MIB",
   {43, 6, 1, 2, 1, 31},
   6,
   ifMibObjects,
   arraysize(ifMibObjects),
   ifMibInit,
   NULL,
   NULL,
   NULL,
   NULL
};

#endif
