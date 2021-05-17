/**
 * @file ip_mib_module.h
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
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _IP_MIB_MODULE_H
#define _IP_MIB_MODULE_H

//Dependencies
#include "mibs/mib_common.h"

//IP MIB module support
#ifndef IP_MIB_SUPPORT
   #define IP_MIB_SUPPORT DISABLED
#elif (IP_MIB_SUPPORT != ENABLED && IP_MIB_SUPPORT != DISABLED)
   #error IP_MIB_SUPPORT parameter is not valid
#endif

//Macro definitions
#if (IP_MIB_SUPPORT == ENABLED)
   #define IP_MIB_INC_COUNTER32(name, value) ipMibBase.name += value
   #define IP_MIB_INC_COUNTER64(name, value) ipMibBase.name += value
#else
   #define IP_MIB_INC_COUNTER32(name, value)
   #define IP_MIB_INC_COUNTER64(name, value)
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief IP forwarding state
 **/

typedef enum
{
   IP_MIB_IP_FORWARDING_ENABLED  = 1,
   IP_MIB_IP_FORWARDING_DISABLED = 2
} IpMibIpForwarding;


/**
 * @brief IP status
 **/

typedef enum
{
   IP_MIB_IP_STATUS_UP   = 1,
   IP_MIB_IP_STATUS_DOWN = 2
} IpMibIpStatus;


/**
 * @brief IP address type
 **/

typedef enum
{
   IP_MIB_ADDR_TYPE_UNICAST   = 1,
   IP_MIB_ADDR_TYPE_ANYCAST   = 2,
   IP_MIB_ADDR_TYPE_BROADCAST = 3
} IpMibAddrType;


/**
 * @brief IP address origin
 **/

typedef enum
{
   IP_MIB_ADDR_ORIGIN_MANUAL     = 2,
   IP_MIB_ADDR_ORIGIN_DHCP       = 4,
   IP_MIB_ADDR_ORIGIN_LINK_LAYER = 5,
   IP_MIB_ADDR_ORIGIN_RANDOM     = 6,
} IpMibAddrOrigin;

/**
 * @brief IP address status
 **/

typedef enum
{
   IP_MIB_ADDR_STATUS_PREFERRED    = 1,
   IP_MIB_ADDR_STATUS_DEPRECATED   = 2,
   IP_MIB_ADDR_STATUS_INVALID      = 3,
   IP_MIB_ADDR_STATUS_INACCESSIBLE = 4,
   IP_MIB_ADDR_STATUS_UNKNOWN      = 5,
   IP_MIB_ADDR_STATUS_TENTATIVE    = 6,
   IP_MIB_ADDR_STATUS_DUPLICATE    = 7,
   IP_MIB_ADDR_STATUS_OPTIMISTIC   = 8
} IpMibAddrStatus;


/**
 * @brief Prefix origin
 **/

typedef enum
{
   IP_MIB_PREFIX_ORIGIN_MANUAL     = 2,
   IP_MIB_PREFIX_ORIGIN_WELL_KNOWN = 3,
   IP_MIB_PREFIX_ORIGIN_DHCP       = 4,
   IP_MIB_PREFIX_ORIGIN_ROUTER_ADV = 5,
} IpMibPrefixOrigin;


/**
 * @brief Type of mapping
 **/

typedef enum
{
   IP_MIB_NET_TO_PHYS_TYPE_OTHER   = 1,
   IP_MIB_NET_TO_PHYS_TYPE_INVALID = 2,
   IP_MIB_NET_TO_PHYS_TYPE_DYNAMIC = 3,
   IP_MIB_NET_TO_PHYS_TYPE_STATIC  = 4,
   IP_MIB_NET_TO_PHYS_TYPE_LOCAL   = 5
} IpMibNetToPhysType;


/**
 * @brief Entry state
 **/

typedef enum
{
   IP_MIB_NET_TO_PHYS_STATE_REACHABLE  = 1,
   IP_MIB_NET_TO_PHYS_STATE_STALE      = 2,
   IP_MIB_NET_TO_PHYS_STATE_DELAY      = 3,
   IP_MIB_NET_TO_PHYS_STATE_PROBE      = 4,
   IP_MIB_NET_TO_PHYS_STATE_INVALID    = 5,
   IP_MIB_NET_TO_PHYS_STATE_UNKNOWN    = 6,
   IP_MIB_NET_TO_PHYS_STATE_INCOMPLETE = 7
} IpMibNetToPhysState;


/**
 * @brief Router preferences
 **/

typedef enum
{
   IP_MIB_ROUTER_PREFERENCE_RESERVED = -2,
   IP_MIB_ROUTER_PREFERENCE_LOW      = -1,
   IP_MIB_ROUTER_PREFERENCE_MEDIUM   = 0,
   IP_MIB_ROUTER_PREFERENCE_HIGH     = 1
} IpMibRouterPreference;


/**
 * @brief System-wide IP statistics
 **/

typedef struct
{
   uint32_t ipSystemStatsInReceives;
   uint64_t ipSystemStatsHCInReceives;
   uint32_t ipSystemStatsInOctets;
   uint64_t ipSystemStatsHCInOctets;
   uint32_t ipSystemStatsInHdrErrors;
   uint32_t ipSystemStatsInNoRoutes;
   uint32_t ipSystemStatsInAddrErrors;
   uint32_t ipSystemStatsInUnknownProtos;
   uint32_t ipSystemStatsInTruncatedPkts;
   uint32_t ipSystemStatsInForwDatagrams;
   uint64_t ipSystemStatsHCInForwDatagrams;
   uint32_t ipSystemStatsReasmReqds;
   uint32_t ipSystemStatsReasmOKs;
   uint32_t ipSystemStatsReasmFails;
   uint32_t ipSystemStatsInDiscards;
   uint32_t ipSystemStatsInDelivers;
   uint64_t ipSystemStatsHCInDelivers;
   uint32_t ipSystemStatsOutRequests;
   uint64_t ipSystemStatsHCOutRequests;
   uint32_t ipSystemStatsOutNoRoutes;
   uint32_t ipSystemStatsOutForwDatagrams;
   uint64_t ipSystemStatsHCOutForwDatagrams;
   uint32_t ipSystemStatsOutDiscards;
   uint32_t ipSystemStatsOutFragReqds;
   uint32_t ipSystemStatsOutFragOKs;
   uint32_t ipSystemStatsOutFragFails;
   uint32_t ipSystemStatsOutFragCreates;
   uint32_t ipSystemStatsOutTransmits;
   uint64_t ipSystemStatsHCOutTransmits;
   uint32_t ipSystemStatsOutOctets;
   uint64_t ipSystemStatsHCOutOctets;
   uint32_t ipSystemStatsInMcastPkts;
   uint64_t ipSystemStatsHCInMcastPkts;
   uint32_t ipSystemStatsInMcastOctets;
   uint64_t ipSystemStatsHCInMcastOctets;
   uint32_t ipSystemStatsOutMcastPkts;
   uint64_t ipSystemStatsHCOutMcastPkts;
   uint32_t ipSystemStatsOutMcastOctets;
   uint64_t ipSystemStatsHCOutMcastOctets;
   uint32_t ipSystemStatsInBcastPkts;
   uint64_t ipSystemStatsHCInBcastPkts;
   uint32_t ipSystemStatsOutBcastPkts;
   uint64_t ipSystemStatsHCOutBcastPkts;
   uint32_t ipSystemStatsDiscontinuityTime;
   uint32_t ipSystemStatsRefreshRate;
} IpMibIpSystemStatsEntry;


/**
 * @brief Per-interface IP statistics
 **/

typedef struct
{
   uint32_t ipIfStatsInReceives;
   uint64_t ipIfStatsHCInReceives;
   uint32_t ipIfStatsInOctets;
   uint64_t ipIfStatsHCInOctets;
   uint32_t ipIfStatsInHdrErrors;
   uint32_t ipIfStatsInNoRoutes;
   uint32_t ipIfStatsInAddrErrors;
   uint32_t ipIfStatsInUnknownProtos;
   uint32_t ipIfStatsInTruncatedPkts;
   uint32_t ipIfStatsInForwDatagrams;
   uint64_t ipIfStatsHCInForwDatagrams;
   uint32_t ipIfStatsReasmReqds;
   uint32_t ipIfStatsReasmOKs;
   uint32_t ipIfStatsReasmFails;
   uint32_t ipIfStatsInDiscards;
   uint32_t ipIfStatsInDelivers;
   uint64_t ipIfStatsHCInDelivers;
   uint32_t ipIfStatsOutRequests;
   uint64_t ipIfStatsHCOutRequests;
   uint32_t ipIfStatsOutForwDatagrams;
   uint64_t ipIfStatsHCOutForwDatagrams;
   uint32_t ipIfStatsOutDiscards;
   uint32_t ipIfStatsOutFragReqds;
   uint32_t ipIfStatsOutFragOKs;
   uint32_t ipIfStatsOutFragFails;
   uint32_t ipIfStatsOutFragCreates;
   uint32_t ipIfStatsOutTransmits;
   uint64_t ipIfStatsHCOutTransmits;
   uint32_t ipIfStatsOutOctets;
   uint64_t ipIfStatsHCOutOctets;
   uint32_t ipIfStatsInMcastPkts;
   uint64_t ipIfStatsHCInMcastPkts;
   uint32_t ipIfStatsInMcastOctets;
   uint64_t ipIfStatsHCInMcastOctets;
   uint32_t ipIfStatsOutMcastPkts;
   uint64_t ipIfStatsHCOutMcastPkts;
   uint32_t ipIfStatsOutMcastOctets;
   uint64_t ipIfStatsHCOutMcastOctets;
   uint32_t ipIfStatsInBcastPkts;
   uint64_t ipIfStatsHCInBcastPkts;
   uint32_t ipIfStatsOutBcastPkts;
   uint64_t ipIfStatsHCOutBcastPkts;
   uint32_t ipIfStatsDiscontinuityTime;
   uint32_t ipIfStatsRefreshRate;
} IpMibIpIfStatsEntry;


/**
 * @brief ICMP statistics
 **/

typedef struct
{
   uint32_t icmpStatsInMsgs;
   uint32_t icmpStatsInErrors;
   uint32_t icmpStatsOutMsgs;
   uint32_t icmpStatsOutErrors;
} IpMibIcmpStatsEntry;


/**
 * @brief Per-message ICMP statistics
 **/

typedef struct
{
   uint32_t icmpMsgStatsInPkts[256];
   uint32_t icmpMsgStatsOutPkts[256];
} IpMibIcmpMsgStatsEntry;


/**
 * @brief IP MIB base
 **/

typedef struct
{
   uint32_t ipIfStatsTableLastChange;
   int32_t ipAddressSpinLock;
#if (IPV4_SUPPORT == ENABLED)
   int32_t ipForwarding;
   int32_t ipDefaultTTL;
   int32_t ipReasmTimeout;
   uint32_t ipv4InterfaceTableLastChange;
   IpMibIpSystemStatsEntry ipv4SystemStats;
   IpMibIpIfStatsEntry ipv4IfStatsTable[NET_INTERFACE_COUNT];
   IpMibIcmpStatsEntry icmpStats;
   IpMibIcmpMsgStatsEntry icmpMsgStatsTable;
#endif
#if (IPV6_SUPPORT == ENABLED)
   int32_t ipv6IpForwarding;
   int32_t ipv6IpDefaultHopLimit;
   uint32_t ipv6InterfaceTableLastChange;
   IpMibIpSystemStatsEntry ipv6SystemStats;
   IpMibIpIfStatsEntry ipv6IfStatsTable[NET_INTERFACE_COUNT];
   int32_t ipv6RouterAdvertSpinLock;
   IpMibIcmpStatsEntry icmpv6Stats;
   IpMibIcmpMsgStatsEntry icmpv6MsgStatsTable;
#endif
} IpMibBase;


//IP MIB related constants
extern IpMibBase ipMibBase;
extern const MibObject ipMibObjects[];
extern const MibModule ipMibModule;

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
