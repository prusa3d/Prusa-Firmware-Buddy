/**
 * @file ip_mib_module.c
 * @brief IP MIB module
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
 * The IP-MIB describes managed objects used for implementations of the
 * Internet Protocol (IP) in an IP version independent manner. Refer to
 * the following RFCs for complete details:
 * - RFC 4293: MIB for the Internet Protocol (IP)
 * - RFC 4001: Textual Conventions for Internet Network Addresses
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "mibs/mib_common.h"
#include "mibs/ip_mib_module.h"
#include "mibs/ip_mib_impl.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IP_MIB_SUPPORT == ENABLED)


/**
 * @brief IP MIB base
 **/

IpMibBase ipMibBase;


/**
 * @brief IP MIB objects
 **/

const MibObject ipMibObjects[] =
{
#if (IPV4_SUPPORT == ENABLED)
   //ipForwarding object (1.3.6.1.2.1.4.1)
   {
      "ipForwarding",
      {43, 6, 1, 2, 1, 4, 1},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      &ipMibBase.ipForwarding,
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
      &ipMibBase.ipDefaultTTL,
      NULL,
      sizeof(int32_t),
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
      &ipMibBase.ipReasmTimeout,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
#endif
#if (IPV6_SUPPORT == ENABLED)
   //ipv6IpForwarding object (1.3.6.1.2.1.4.25)
   {
      "ipv6IpForwarding",
      {43, 6, 1, 2, 1, 4, 25},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      &ipMibBase.ipv6IpForwarding,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
   //ipv6IpDefaultHopLimit object (1.3.6.1.2.1.4.26)
   {
      "ipv6IpDefaultHopLimit",
      {43, 6, 1, 2, 1, 4, 26},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      &ipMibBase.ipv6IpDefaultHopLimit,
      NULL,
      sizeof(int32_t),
      NULL,
      NULL,
      NULL
   },
#endif
#if (IPV4_SUPPORT == ENABLED)
   //ipv4InterfaceTableLastChange object (1.3.6.1.2.1.4.27)
   {
      "ipv4InterfaceTableLastChange",
      {43, 6, 1, 2, 1, 4, 27},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      &ipMibBase.ipv4InterfaceTableLastChange,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipv4InterfaceReasmMaxSize object (1.3.6.1.2.1.4.28.1.2)
   {
      "ipv4InterfaceReasmMaxSize",
      {43, 6, 1, 2, 1, 4, 28, 1, 2},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      ipMibGetIpv4InterfaceEntry,
      ipMibGetNextIpv4InterfaceEntry
   },
   //ipv4InterfaceEnableStatus object (1.3.6.1.2.1.4.28.1.3)
   {
      "ipv4InterfaceEnableStatus",
      {43, 6, 1, 2, 1, 4, 28, 1, 3},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpv4InterfaceEntry,
      ipMibGetIpv4InterfaceEntry,
      ipMibGetNextIpv4InterfaceEntry
   },
   //ipv4InterfaceRetransmitTime object (1.3.6.1.2.1.4.28.1.4)
   {
      "ipv4InterfaceRetransmitTime",
      {43, 6, 1, 2, 1, 4, 28, 1, 4},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv4InterfaceEntry,
      ipMibGetNextIpv4InterfaceEntry
   },
#endif
#if (IPV6_SUPPORT == ENABLED)
   //ipv6InterfaceTableLastChange object (1.3.6.1.2.1.4.29)
   {
      "ipv6InterfaceTableLastChange",
      {43, 6, 1, 2, 1, 4, 29},
      7,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      &ipMibBase.ipv6InterfaceTableLastChange,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipv6InterfaceReasmMaxSize object (1.3.6.1.2.1.4.30.1.2)
   {
      "ipv6InterfaceReasmMaxSize",
      {43, 6, 1, 2, 1, 4, 30, 1, 2},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6InterfaceEntry,
      ipMibGetNextIpv6InterfaceEntry
   },
   //ipv6InterfaceIdentifier object (1.3.6.1.2.1.4.30.1.3)
   {
      "ipv6InterfaceIdentifier",
      {43, 6, 1, 2, 1, 4, 30, 1, 3},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      0,
      NULL,
      ipMibGetIpv6InterfaceEntry,
      ipMibGetNextIpv6InterfaceEntry
   },
   //ipv6InterfaceEnableStatus object (1.3.6.1.2.1.4.30.1.5)
   {
      "ipv6InterfaceEnableStatus",
      {43, 6, 1, 2, 1, 4, 30, 1, 5},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpv6InterfaceEntry,
      ipMibGetIpv6InterfaceEntry,
      ipMibGetNextIpv6InterfaceEntry
   },
   //ipv6InterfaceReachableTime object (1.3.6.1.2.1.4.30.1.6)
   {
      "ipv6InterfaceReachableTime",
      {43, 6, 1, 2, 1, 4, 30, 1, 6},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6InterfaceEntry,
      ipMibGetNextIpv6InterfaceEntry
   },
   //ipv6InterfaceRetransmitTime object (1.3.6.1.2.1.4.30.1.7)
   {
      "ipv6InterfaceRetransmitTime",
      {43, 6, 1, 2, 1, 4, 30, 1, 7},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6InterfaceEntry,
      ipMibGetNextIpv6InterfaceEntry
   },
   //ipv6InterfaceForwarding object (1.3.6.1.2.1.4.30.1.8)
   {
      "ipv6InterfaceForwarding",
      {43, 6, 1, 2, 1, 4, 30, 1, 8},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpv6InterfaceEntry,
      ipMibGetIpv6InterfaceEntry,
      ipMibGetNextIpv6InterfaceEntry
   },
#endif
   //ipSystemStatsInReceives object (1.3.6.1.2.1.4.31.1.1.3)
   {
      "ipSystemStatsInReceives",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 3},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCInReceives object (1.3.6.1.2.1.4.31.1.1.4)
   {
      "ipSystemStatsHCInReceives",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 4},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInOctets object (1.3.6.1.2.1.4.31.1.1.5)
   {
      "ipSystemStatsInOctets",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 5},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCInOctets object (1.3.6.1.2.1.4.31.1.1.6)
   {
      "ipSystemStatsHCInOctets",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 6},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInHdrErrors object (1.3.6.1.2.1.4.31.1.1.7)
   {
      "ipSystemStatsInHdrErrors",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 7},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInNoRoutes object (1.3.6.1.2.1.4.31.1.1.8)
   {
      "ipSystemStatsInNoRoutes",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 8},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInAddrErrors object (1.3.6.1.2.1.4.31.1.1.9)
   {
      "ipSystemStatsInAddrErrors",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 9},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInUnknownProtos object (1.3.6.1.2.1.4.31.1.1.10)
   {
      "ipSystemStatsInUnknownProtos",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 10},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInTruncatedPkts object (1.3.6.1.2.1.4.31.1.1.11)
   {
      "ipSystemStatsInTruncatedPkts",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 11},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInForwDatagrams object (1.3.6.1.2.1.4.31.1.1.12)
   {
      "ipSystemStatsInForwDatagrams",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 12},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCInForwDatagrams object (1.3.6.1.2.1.4.31.1.1.13)
   {
      "ipSystemStatsHCInForwDatagrams",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 13},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsReasmReqds object (1.3.6.1.2.1.4.31.1.1.14)
   {
      "ipSystemStatsReasmReqds",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 14},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsReasmOKs object (1.3.6.1.2.1.4.31.1.1.15)
   {
      "ipSystemStatsReasmOKs",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 15},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsReasmFails object (1.3.6.1.2.1.4.31.1.1.16)
   {
      "ipSystemStatsReasmFails",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 16},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInDiscards object (1.3.6.1.2.1.4.31.1.1.17)
   {
      "ipSystemStatsInDiscards",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 17},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInDelivers object (1.3.6.1.2.1.4.31.1.1.18)
   {
      "ipSystemStatsInDelivers",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 18},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCInDelivers object (1.3.6.1.2.1.4.31.1.1.19)
   {
      "ipSystemStatsHCInDelivers",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 19},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutRequests object (1.3.6.1.2.1.4.31.1.1.20)
   {
      "ipSystemStatsOutRequests",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 20},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCOutRequests object (1.3.6.1.2.1.4.31.1.1.21)
   {
      "ipSystemStatsHCOutRequests",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 21},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutNoRoutes object (1.3.6.1.2.1.4.31.1.1.22)
   {
      "ipSystemStatsOutNoRoutes",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 22},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutForwDatagrams object (1.3.6.1.2.1.4.31.1.1.23)
   {
      "ipSystemStatsOutForwDatagrams",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 23},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCOutForwDatagrams object (1.3.6.1.2.1.4.31.1.1.24)
   {
      "ipSystemStatsHCOutForwDatagrams",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 24},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutDiscards object (1.3.6.1.2.1.4.31.1.1.25)
   {
      "ipSystemStatsOutDiscards",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 25},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutFragReqds object (1.3.6.1.2.1.4.31.1.1.26)
   {
      "ipSystemStatsOutFragReqds",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 26},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutFragOKs object (1.3.6.1.2.1.4.31.1.1.27)
   {
      "ipSystemStatsOutFragOKs",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 27},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutFragFails object (1.3.6.1.2.1.4.31.1.1.28)
   {
      "ipSystemStatsOutFragFails",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 28},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutFragCreates object (1.3.6.1.2.1.4.31.1.1.29)
   {
      "ipSystemStatsOutFragCreates",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 29},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutTransmits object (1.3.6.1.2.1.4.31.1.1.30)
   {
      "ipSystemStatsOutTransmits",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 30},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCOutTransmits object (1.3.6.1.2.1.4.31.1.1.31)
   {
      "ipSystemStatsHCOutTransmits",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 31},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutOctets object (1.3.6.1.2.1.4.31.1.1.32)
   {
      "ipSystemStatsOutOctets",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 32},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCOutOctets object (1.3.6.1.2.1.4.31.1.1.33)
   {
      "ipSystemStatsHCOutOctets",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 33},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInMcastPkts object (1.3.6.1.2.1.4.31.1.1.34)
   {
      "ipSystemStatsInMcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 34},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCInMcastPkts object (1.3.6.1.2.1.4.31.1.1.35)
   {
      "ipSystemStatsHCInMcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 35},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInMcastOctets object (1.3.6.1.2.1.4.31.1.1.36)
   {
      "ipSystemStatsInMcastOctets",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 36},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCInMcastOctets object (1.3.6.1.2.1.4.31.1.1.37)
   {
      "ipSystemStatsHCInMcastOctets",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 37},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutMcastPkts object (1.3.6.1.2.1.4.31.1.1.38)
   {
      "ipSystemStatsOutMcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 38},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCOutMcastPkts object (1.3.6.1.2.1.4.31.1.1.39)
   {
      "ipSystemStatsHCOutMcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 39},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutMcastOctets object (1.3.6.1.2.1.4.31.1.1.40)
   {
      "ipSystemStatsOutMcastOctets",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 40},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCOutMcastOctets object (1.3.6.1.2.1.4.31.1.1.41)
   {
      "ipSystemStatsHCOutMcastOctets",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 41},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsInBcastPkts object (1.3.6.1.2.1.4.31.1.1.42)
   {
      "ipSystemStatsInBcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 42},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCInBcastPkts object (1.3.6.1.2.1.4.31.1.1.43)
   {
      "ipSystemStatsHCInBcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 43},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsOutBcastPkts object (1.3.6.1.2.1.4.31.1.1.44)
   {
      "ipSystemStatsOutBcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 44},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsHCOutBcastPkts object (1.3.6.1.2.1.4.31.1.1.45)
   {
      "ipSystemStatsHCOutBcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 45},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsDiscontinuityTime object (1.3.6.1.2.1.4.31.1.1.46)
   {
      "ipSystemStatsDiscontinuityTime",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 46},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipSystemStatsRefreshRate object (1.3.6.1.2.1.4.31.1.1.47)
   {
      "ipSystemStatsRefreshRate",
      {43, 6, 1, 2, 1, 4, 31, 1, 1, 47},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpSystemStatsEntry,
      ipMibGetNextIpSystemStatsEntry
   },
   //ipIfStatsTableLastChange object (1.3.6.1.2.1.4.31.2)
   {
      "ipIfStatsTableLastChange",
      {43, 6, 1, 2, 1, 4, 31, 2},
      8,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      &ipMibBase.ipIfStatsTableLastChange,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //ipIfStatsInReceives object (1.3.6.1.2.1.4.31.3.1.3)
   {
      "ipIfStatsInReceives",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 3},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCInReceives object (1.3.6.1.2.1.4.31.3.1.4)
   {
      "ipIfStatsHCInReceives",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 4},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInOctets object (1.3.6.1.2.1.4.31.3.1.5)
   {
      "ipIfStatsInOctets",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 5},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCInOctets object (1.3.6.1.2.1.4.31.3.1.6)
   {
      "ipIfStatsHCInOctets",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 6},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInHdrErrors object (1.3.6.1.2.1.4.31.3.1.7)
   {
      "ipIfStatsInHdrErrors",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 7},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInNoRoutes object (1.3.6.1.2.1.4.31.3.1.8)
   {
      "ipIfStatsInNoRoutes",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 8},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInAddrErrors object (1.3.6.1.2.1.4.31.3.1.9)
   {
      "ipIfStatsInAddrErrors",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 9},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInUnknownProtos object (1.3.6.1.2.1.4.31.3.1.10)
   {
      "ipIfStatsInUnknownProtos",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 10},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInTruncatedPkts object (1.3.6.1.2.1.4.31.3.1.11)
   {
      "ipIfStatsInTruncatedPkts",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 11},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInForwDatagrams object (1.3.6.1.2.1.4.31.3.1.12)
   {
      "ipIfStatsInForwDatagrams",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 12},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCInForwDatagrams object (1.3.6.1.2.1.4.31.3.1.13)
   {
      "ipIfStatsHCInForwDatagrams",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 13},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsReasmReqds object (1.3.6.1.2.1.4.31.3.1.14)
   {
      "ipIfStatsReasmReqds",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 14},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsReasmOKs object (1.3.6.1.2.1.4.31.3.1.15)
   {
      "ipIfStatsReasmOKs",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 15},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsReasmFails object (1.3.6.1.2.1.4.31.3.1.16)
   {
      "ipIfStatsReasmFails",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 16},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInDiscards object (1.3.6.1.2.1.4.31.3.1.17)
   {
      "ipIfStatsInDiscards",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 17},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInDelivers object (1.3.6.1.2.1.4.31.3.1.18)
   {
      "ipIfStatsInDelivers",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 18},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCInDelivers object (1.3.6.1.2.1.4.31.3.1.19)
   {
      "ipIfStatsHCInDelivers",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 19},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutRequests object (1.3.6.1.2.1.4.31.3.1.20)
   {
      "ipIfStatsOutRequests",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 20},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCOutRequests object (1.3.6.1.2.1.4.31.3.1.21)
   {
      "ipIfStatsHCOutRequests",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 21},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutForwDatagrams object (1.3.6.1.2.1.4.31.3.1.23)
   {
      "ipIfStatsOutForwDatagrams",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 23},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCOutForwDatagrams object (1.3.6.1.2.1.4.31.3.1.24)
   {
      "ipIfStatsHCOutForwDatagrams",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 24},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutDiscards object (1.3.6.1.2.1.4.31.3.1.25)
   {
      "ipIfStatsOutDiscards",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 25},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutFragReqds object (1.3.6.1.2.1.4.31.3.1.26)
   {
      "ipIfStatsOutFragReqds",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 26},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutFragOKs object (1.3.6.1.2.1.4.31.3.1.27)
   {
      "ipIfStatsOutFragOKs",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 27},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutFragFails object (1.3.6.1.2.1.4.31.3.1.28)
   {
      "ipIfStatsOutFragFails",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 28},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutFragCreates object (1.3.6.1.2.1.4.31.3.1.29)
   {
      "ipIfStatsOutFragCreates",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 29},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutTransmits object (1.3.6.1.2.1.4.31.3.1.30)
   {
      "ipIfStatsOutTransmits",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 30},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCOutTransmits object (1.3.6.1.2.1.4.31.3.1.31)
   {
      "ipIfStatsHCOutTransmits",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 31},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutOctets object (1.3.6.1.2.1.4.31.3.1.32)
   {
      "ipIfStatsOutOctets",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 32},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCOutOctets object (1.3.6.1.2.1.4.31.3.1.33)
   {
      "ipIfStatsHCOutOctets",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 33},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInMcastPkts object (1.3.6.1.2.1.4.31.3.1.34)
   {
      "ipIfStatsInMcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 34},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCInMcastPkts object (1.3.6.1.2.1.4.31.3.1.35)
   {
      "ipIfStatsHCInMcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 35},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInMcastOctets object (1.3.6.1.2.1.4.31.3.1.36)
   {
      "ipIfStatsInMcastOctets",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 36},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCInMcastOctets object (1.3.6.1.2.1.4.31.3.1.37)
   {
      "ipIfStatsHCInMcastOctets",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 37},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutMcastPkts object (1.3.6.1.2.1.4.31.3.1.38)
   {
      "ipIfStatsOutMcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 38},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCOutMcastPkts object (1.3.6.1.2.1.4.31.3.1.39)
   {
      "ipIfStatsHCOutMcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 39},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutMcastOctets object (1.3.6.1.2.1.4.31.3.1.40)
   {
      "ipIfStatsOutMcastOctets",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 40},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCOutMcastOctets object (1.3.6.1.2.1.4.31.3.1.41)
   {
      "ipIfStatsHCOutMcastOctets",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 41},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsInBcastPkts object (1.3.6.1.2.1.4.31.3.1.42)
   {
      "ipIfStatsInBcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 42},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCInBcastPkts object (1.3.6.1.2.1.4.31.3.1.43)
   {
      "ipIfStatsHCInBcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 43},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsOutBcastPkts object (1.3.6.1.2.1.4.31.3.1.44)
   {
      "ipIfStatsOutBcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 44},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsHCOutBcastPkts object (1.3.6.1.2.1.4.31.3.1.45)
   {
      "ipIfStatsHCOutBcastPkts",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 45},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER64,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint64_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsDiscontinuityTime object (1.3.6.1.2.1.4.31.3.1.46)
   {
      "ipIfStatsDiscontinuityTime",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 46},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipIfStatsRefreshRate object (1.3.6.1.2.1.4.31.3.1.47)
   {
      "ipIfStatsRefreshRate",
      {43, 6, 1, 2, 1, 4, 31, 3, 1, 47},
      10,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpIfStatsEntry,
      ipMibGetNextIpIfStatsEntry
   },
   //ipAddressPrefixOrigin object (1.3.6.1.2.1.4.32.1.5)
   {
      "ipAddressPrefixOrigin",
      {43, 6, 1, 2, 1, 4, 32, 1, 5},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      ipMibGetIpAddressPrefixEntry,
      ipMibGetNextIpAddressPrefixEntry
   },
   //ipAddressPrefixOnLinkFlag object (1.3.6.1.2.1.4.32.1.6)
   {
      "ipAddressPrefixOnLinkFlag",
      {43, 6, 1, 2, 1, 4, 32, 1, 6},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      ipMibGetIpAddressPrefixEntry,
      ipMibGetNextIpAddressPrefixEntry
   },
   //ipAddressPrefixAutonomousFlag object (1.3.6.1.2.1.4.32.1.7)
   {
      "ipAddressPrefixAutonomousFlag",
      {43, 6, 1, 2, 1, 4, 32, 1, 7},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      ipMibGetIpAddressPrefixEntry,
      ipMibGetNextIpAddressPrefixEntry
   },
   //ipAddressPrefixAdvPreferredLifetime object (1.3.6.1.2.1.4.32.1.8)
   {
      "ipAddressPrefixAdvPreferredLifetime",
      {43, 6, 1, 2, 1, 4, 32, 1, 8},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpAddressPrefixEntry,
      ipMibGetNextIpAddressPrefixEntry
   },
   //ipAddressPrefixAdvValidLifetime object (1.3.6.1.2.1.4.32.1.9)
   {
      "ipAddressPrefixAdvValidLifetime",
      {43, 6, 1, 2, 1, 4, 32, 1, 9},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpAddressPrefixEntry,
      ipMibGetNextIpAddressPrefixEntry
   },
   //ipAddressSpinLock object (1.3.6.1.2.1.4.33)
   {
      "ipAddressSpinLock",
      {43, 6, 1, 2, 1, 4, 33},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpAddressSpinLock,
      ipMibGetIpAddressSpinLock,
      NULL
   },
   //ipAddressIfIndex object (1.3.6.1.2.1.4.34.1.3)
   {
      "ipAddressIfIndex",
      {43, 6, 1, 2, 1, 4, 34, 1, 3},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpAddressEntry,
      ipMibGetIpAddressEntry,
      ipMibGetNextIpAddressEntry
   },
   //ipAddressType object (1.3.6.1.2.1.4.34.1.4)
   {
      "ipAddressType",
      {43, 6, 1, 2, 1, 4, 34, 1, 4},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpAddressEntry,
      ipMibGetIpAddressEntry,
      ipMibGetNextIpAddressEntry
   },
   //ipAddressPrefix object (1.3.6.1.2.1.4.34.1.5)
   {
      "ipAddressPrefix",
      {43, 6, 1, 2, 1, 4, 34, 1, 5},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OBJECT_IDENTIFIER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      0,
      NULL,
      ipMibGetIpAddressEntry,
      ipMibGetNextIpAddressEntry
   },
   //ipAddressOrigin object (1.3.6.1.2.1.4.34.1.6)
   {
      "ipAddressOrigin",
      {43, 6, 1, 2, 1, 4, 34, 1, 6},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      ipMibGetIpAddressEntry,
      ipMibGetNextIpAddressEntry
   },
   //ipAddressStatus object (1.3.6.1.2.1.4.34.1.7)
   {
      "ipAddressStatus",
      {43, 6, 1, 2, 1, 4, 34, 1, 7},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpAddressEntry,
      ipMibGetIpAddressEntry,
      ipMibGetNextIpAddressEntry
   },
   //ipAddressCreated object (1.3.6.1.2.1.4.34.1.8)
   {
      "ipAddressCreated",
      {43, 6, 1, 2, 1, 4, 34, 1, 8},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpAddressEntry,
      ipMibGetNextIpAddressEntry
   },
   //ipAddressLastChanged object (1.3.6.1.2.1.4.34.1.9)
   {
      "ipAddressLastChanged",
      {43, 6, 1, 2, 1, 4, 34, 1, 9},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpAddressEntry,
      ipMibGetNextIpAddressEntry
   },
   //ipAddressRowStatus object (1.3.6.1.2.1.4.34.1.10)
   {
      "ipAddressRowStatus",
      {43, 6, 1, 2, 1, 4, 34, 1, 10},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpAddressEntry,
      ipMibGetIpAddressEntry,
      ipMibGetNextIpAddressEntry
   },
   //ipAddressStorageType object (1.3.6.1.2.1.4.34.1.11)
   {
      "ipAddressStorageType",
      {43, 6, 1, 2, 1, 4, 34, 1, 11},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpAddressEntry,
      ipMibGetIpAddressEntry,
      ipMibGetNextIpAddressEntry
   },
   //ipNetToPhysicalPhysAddress object (1.3.6.1.2.1.4.35.1.4)
   {
      "ipNetToPhysicalPhysAddress",
      {43, 6, 1, 2, 1, 4, 35, 1, 4},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_OCTET_STRING,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      0,
      ipMibSetIpNetToPhysicalEntry,
      ipMibGetIpNetToPhysicalEntry,
      ipMibGetNextIpNetToPhysicalEntry
   },
   //ipNetToPhysicalLastUpdated object (1.3.6.1.2.1.4.35.1.5)
   {
      "ipNetToPhysicalLastUpdated",
      {43, 6, 1, 2, 1, 4, 35, 1, 5},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_TIME_TICKS,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpNetToPhysicalEntry,
      ipMibGetNextIpNetToPhysicalEntry
   },
   //ipNetToPhysicalType object (1.3.6.1.2.1.4.35.1.6)
   {
      "ipNetToPhysicalType",
      {43, 6, 1, 2, 1, 4, 35, 1, 6},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpNetToPhysicalEntry,
      ipMibGetIpNetToPhysicalEntry,
      ipMibGetNextIpNetToPhysicalEntry
   },
   //ipNetToPhysicalState object (1.3.6.1.2.1.4.35.1.7)
   {
      "ipNetToPhysicalState",
      {43, 6, 1, 2, 1, 4, 35, 1, 7},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      ipMibGetIpNetToPhysicalEntry,
      ipMibGetNextIpNetToPhysicalEntry
   },
   //ipNetToPhysicalRowStatus object (1.3.6.1.2.1.4.35.1.8)
   {
      "ipNetToPhysicalRowStatus",
      {43, 6, 1, 2, 1, 4, 35, 1, 8},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpNetToPhysicalEntry,
      ipMibGetIpNetToPhysicalEntry,
      ipMibGetNextIpNetToPhysicalEntry
   },
#if (IPV6_SUPPORT == ENABLED)
   //ipv6ScopeZoneIndexLinkLocal object (1.3.6.1.2.1.4.36.1.2)
   {
      "ipv6ScopeZoneIndexLinkLocal",
      {43, 6, 1, 2, 1, 4, 36, 1, 2},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
   //ipv6ScopeZoneIndex3 object (1.3.6.1.2.1.4.36.1.3)
   {
      "ipv6ScopeZoneIndex3",
      {43, 6, 1, 2, 1, 4, 36, 1, 3},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
   //ipv6ScopeZoneIndexAdminLocal object (1.3.6.1.2.1.4.36.1.4)
   {
      "ipv6ScopeZoneIndexAdminLocal",
      {43, 6, 1, 2, 1, 4, 36, 1, 4},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
   //ipv6ScopeZoneIndexSiteLocal object (1.3.6.1.2.1.4.36.1.5)
   {
      "ipv6ScopeZoneIndexSiteLocal",
      {43, 6, 1, 2, 1, 4, 36, 1, 5},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
   //ipv6ScopeZoneIndex6 object (1.3.6.1.2.1.4.36.1.6)
   {
      "ipv6ScopeZoneIndex6",
      {43, 6, 1, 2, 1, 4, 36, 1, 6},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
   //ipv6ScopeZoneIndex7 object (1.3.6.1.2.1.4.36.1.7)
   {
      "ipv6ScopeZoneIndex7",
      {43, 6, 1, 2, 1, 4, 36, 1, 7},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
   //ipv6ScopeZoneIndexOrganizationLocal object (1.3.6.1.2.1.4.36.1.8)
   {
      "ipv6ScopeZoneIndexOrganizationLocal",
      {43, 6, 1, 2, 1, 4, 36, 1, 8},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
   //ipv6ScopeZoneIndex9 object (1.3.6.1.2.1.4.36.1.9)
   {
      "ipv6ScopeZoneIndex9",
      {43, 6, 1, 2, 1, 4, 36, 1, 9},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
   //ipv6ScopeZoneIndexA object (1.3.6.1.2.1.4.36.1.10)
   {
      "ipv6ScopeZoneIndexA",
      {43, 6, 1, 2, 1, 4, 36, 1, 10},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
   //ipv6ScopeZoneIndexB object (1.3.6.1.2.1.4.36.1.11)
   {
      "ipv6ScopeZoneIndexB",
      {43, 6, 1, 2, 1, 4, 36, 1, 11},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
   //ipv6ScopeZoneIndexC object (1.3.6.1.2.1.4.36.1.12)
   {
      "ipv6ScopeZoneIndexC",
      {43, 6, 1, 2, 1, 4, 36, 1, 12},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
   //ipv6ScopeZoneIndexD object (1.3.6.1.2.1.4.36.1.13)
   {
      "ipv6ScopeZoneIndexD",
      {43, 6, 1, 2, 1, 4, 36, 1, 13},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpv6ScopeZoneIndexEntry,
      ipMibGetNextIpv6ScopeZoneIndexEntry
   },
#endif
   //ipDefaultRouterLifetime object (1.3.6.1.2.1.4.37.1.4)
   {
      "ipDefaultRouterLifetime",
      {43, 6, 1, 2, 1, 4, 37, 1, 4},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIpDefaultRouterEntry,
      ipMibGetNextIpDefaultRouterEntry
   },
   //ipDefaultRouterPreference object (1.3.6.1.2.1.4.37.1.5)
   {
      "ipDefaultRouterPreference",
      {43, 6, 1, 2, 1, 4, 37, 1, 5},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(int32_t),
      NULL,
      ipMibGetIpDefaultRouterEntry,
      ipMibGetNextIpDefaultRouterEntry
   },
#if (IPV6_SUPPORT == ENABLED)
   //ipv6RouterAdvertSpinLock object (1.3.6.1.2.1.4.38)
   {
      "ipv6RouterAdvertSpinLock",
      {43, 6, 1, 2, 1, 4, 38},
      7,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_WRITE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpv6RouterAdvertSpinLock,
      ipMibGetIpv6RouterAdvertSpinLock,
      NULL
   },
   //ipv6RouterAdvertSendAdverts object (1.3.6.1.2.1.4.39.1.2)
   {
      "ipv6RouterAdvertSendAdverts",
      {43, 6, 1, 2, 1, 4, 39, 1, 2},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpv6RouterAdvertEntry,
      ipMibGetIpv6RouterAdvertEntry,
      ipMibGetNextIpv6RouterAdvertEntry
   },
   //ipv6RouterAdvertMaxInterval object (1.3.6.1.2.1.4.39.1.3)
   {
      "ipv6RouterAdvertMaxInterval",
      {43, 6, 1, 2, 1, 4, 39, 1, 3},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(uint32_t),
      ipMibSetIpv6RouterAdvertEntry,
      ipMibGetIpv6RouterAdvertEntry,
      ipMibGetNextIpv6RouterAdvertEntry
   },
   //ipv6RouterAdvertMinInterval object (1.3.6.1.2.1.4.39.1.4)
   {
      "ipv6RouterAdvertMinInterval",
      {43, 6, 1, 2, 1, 4, 39, 1, 4},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(uint32_t),
      ipMibSetIpv6RouterAdvertEntry,
      ipMibGetIpv6RouterAdvertEntry,
      ipMibGetNextIpv6RouterAdvertEntry
   },
   //ipv6RouterAdvertManagedFlag object (1.3.6.1.2.1.4.39.1.5)
   {
      "ipv6RouterAdvertManagedFlag",
      {43, 6, 1, 2, 1, 4, 39, 1, 5},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpv6RouterAdvertEntry,
      ipMibGetIpv6RouterAdvertEntry,
      ipMibGetNextIpv6RouterAdvertEntry
   },
   //ipv6RouterAdvertOtherConfigFlag object (1.3.6.1.2.1.4.39.1.6)
   {
      "ipv6RouterAdvertOtherConfigFlag",
      {43, 6, 1, 2, 1, 4, 39, 1, 6},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpv6RouterAdvertEntry,
      ipMibGetIpv6RouterAdvertEntry,
      ipMibGetNextIpv6RouterAdvertEntry
   },
   //ipv6RouterAdvertLinkMTU object (1.3.6.1.2.1.4.39.1.7)
   {
      "ipv6RouterAdvertLinkMTU",
      {43, 6, 1, 2, 1, 4, 39, 1, 7},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(uint32_t),
      ipMibSetIpv6RouterAdvertEntry,
      ipMibGetIpv6RouterAdvertEntry,
      ipMibGetNextIpv6RouterAdvertEntry
   },
   //ipv6RouterAdvertReachableTime object (1.3.6.1.2.1.4.39.1.8)
   {
      "ipv6RouterAdvertReachableTime",
      {43, 6, 1, 2, 1, 4, 39, 1, 8},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(uint32_t),
      ipMibSetIpv6RouterAdvertEntry,
      ipMibGetIpv6RouterAdvertEntry,
      ipMibGetNextIpv6RouterAdvertEntry
   },
   //ipv6RouterAdvertRetransmitTime object (1.3.6.1.2.1.4.39.1.9)
   {
      "ipv6RouterAdvertRetransmitTime",
      {43, 6, 1, 2, 1, 4, 39, 1, 9},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(uint32_t),
      ipMibSetIpv6RouterAdvertEntry,
      ipMibGetIpv6RouterAdvertEntry,
      ipMibGetNextIpv6RouterAdvertEntry
   },
   //ipv6RouterAdvertCurHopLimit object (1.3.6.1.2.1.4.39.1.10)
   {
      "ipv6RouterAdvertCurHopLimit",
      {43, 6, 1, 2, 1, 4, 39, 1, 10},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(uint32_t),
      ipMibSetIpv6RouterAdvertEntry,
      ipMibGetIpv6RouterAdvertEntry,
      ipMibGetNextIpv6RouterAdvertEntry
   },
   //ipv6RouterAdvertDefaultLifetime object (1.3.6.1.2.1.4.39.1.11)
   {
      "ipv6RouterAdvertDefaultLifetime",
      {43, 6, 1, 2, 1, 4, 39, 1, 11},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_UNSIGNED32,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(uint32_t),
      ipMibSetIpv6RouterAdvertEntry,
      ipMibGetIpv6RouterAdvertEntry,
      ipMibGetNextIpv6RouterAdvertEntry
   },
   //ipv6RouterAdvertRowStatus object (1.3.6.1.2.1.4.39.1.12)
   {
      "ipv6RouterAdvertRowStatus",
      {43, 6, 1, 2, 1, 4, 39, 1, 12},
      9,
      ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_INTEGER,
      MIB_ACCESS_READ_CREATE,
      NULL,
      NULL,
      sizeof(int32_t),
      ipMibSetIpv6RouterAdvertEntry,
      ipMibGetIpv6RouterAdvertEntry,
      ipMibGetNextIpv6RouterAdvertEntry
   },
#endif
   //icmpStatsInMsgs object (1.3.6.1.2.1.5.29.1.2)
   {
      "icmpStatsInMsgs",
      {43, 6, 1, 2, 1, 5, 29, 1, 2},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIcmpStatsEntry,
      ipMibGetNextIcmpStatsEntry
   },
   //icmpStatsInErrors object (1.3.6.1.2.1.5.29.1.3)
   {
      "icmpStatsInErrors",
      {43, 6, 1, 2, 1, 5, 29, 1, 3},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIcmpStatsEntry,
      ipMibGetNextIcmpStatsEntry
   },
   //icmpStatsOutMsgs object (1.3.6.1.2.1.5.29.1.4)
   {
      "icmpStatsOutMsgs",
      {43, 6, 1, 2, 1, 5, 29, 1, 4},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIcmpStatsEntry,
      ipMibGetNextIcmpStatsEntry
   },
   //icmpStatsOutErrors object (1.3.6.1.2.1.5.29.1.5)
   {
      "icmpStatsOutErrors",
      {43, 6, 1, 2, 1, 5, 29, 1, 5},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIcmpStatsEntry,
      ipMibGetNextIcmpStatsEntry
   },
   //icmpMsgStatsInPkts object (1.3.6.1.2.1.5.30.1.3)
   {
      "icmpMsgStatsInPkts",
      {43, 6, 1, 2, 1, 5, 30, 1, 3},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIcmpMsgStatsEntry,
      ipMibGetNextIcmpMsgStatsEntry
   },
   //icmpMsgStatsOutPkts object (1.3.6.1.2.1.5.30.1.4)
   {
      "icmpMsgStatsOutPkts",
      {43, 6, 1, 2, 1, 5, 30, 1, 4},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      NULL,
      NULL,
      sizeof(uint32_t),
      NULL,
      ipMibGetIcmpMsgStatsEntry,
      ipMibGetNextIcmpMsgStatsEntry
   }
};


/**
 * @brief IP MIB module
 **/

const MibModule ipMibModule =
{
   "IP-MIB",
   {43, 6, 1, 2, 1, 48},
   6,
   ipMibObjects,
   arraysize(ipMibObjects),
   ipMibInit,
   NULL,
   NULL,
   NULL,
   NULL
};

#endif
