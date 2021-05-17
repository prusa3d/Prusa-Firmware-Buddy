/**
 * @file mib2_module.h
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
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _MIB2_MODULE_H
#define _MIB2_MODULE_H

//Dependencies
#include "core/net.h"
#include "core/udp.h"
#include "core/tcp.h"
#include "ipv4/ipv4.h"
#include "mibs/mib_common.h"

//MIB-II module support
#ifndef MIB2_SUPPORT
   #define MIB2_SUPPORT DISABLED
#elif (MIB2_SUPPORT != ENABLED && MIB2_SUPPORT != DISABLED)
   #error MIB2_SUPPORT parameter is not valid
#endif

//System group support
#ifndef MIB2_SYS_GROUP_SUPPORT
   #define MIB2_SYS_GROUP_SUPPORT ENABLED
#elif (MIB2_SYS_GROUP_SUPPORT != ENABLED && MIB2_SYS_GROUP_SUPPORT != DISABLED)
   #error MIB2_SYS_GROUP_SUPPORT parameter is not valid
#endif

//Interface group support
#ifndef MIB2_IF_GROUP_SUPPORT
   #define MIB2_IF_GROUP_SUPPORT ENABLED
#elif (MIB2_IF_GROUP_SUPPORT != ENABLED && MIB2_IF_GROUP_SUPPORT != DISABLED)
   #error MIB2_IF_GROUP_SUPPORT parameter is not valid
#endif

//IP group support
#ifndef MIB2_IP_GROUP_SUPPORT
   #define MIB2_IP_GROUP_SUPPORT ENABLED
#elif (MIB2_IP_GROUP_SUPPORT != ENABLED && MIB2_IP_GROUP_SUPPORT != DISABLED)
   #error MIB2_IP_GROUP_SUPPORT parameter is not valid
#endif

//ICMP group support
#ifndef MIB2_ICMP_GROUP_SUPPORT
   #define MIB2_ICMP_GROUP_SUPPORT ENABLED
#elif (MIB2_ICMP_GROUP_SUPPORT != ENABLED && MIB2_ICMP_GROUP_SUPPORT != DISABLED)
   #errorMIB2_ICMP_GROUP_SUPPORT parameter is not valid
#endif

//TCP group support
#ifndef MIB2_TCP_GROUP_SUPPORT
   #define MIB2_TCP_GROUP_SUPPORT ENABLED
#elif (MIB2_TCP_GROUP_SUPPORT != ENABLED && MIB2_TCP_GROUP_SUPPORT != DISABLED)
   #error MIB2_TCP_GROUP_SUPPORT parameter is not valid
#endif

//UDP group support
#ifndef MIB2_UDP_GROUP_SUPPORT
   #define MIB2_UDP_GROUP_SUPPORT ENABLED
#elif (MIB2_UDP_GROUP_SUPPORT != ENABLED && MIB2_UDP_GROUP_SUPPORT != DISABLED)
   #error MIB2_UDP_GROUP_SUPPORT parameter is not valid
#endif

//SNMP group support
#ifndef MIB2_SNMP_GROUP_SUPPORT
   #define MIB2_SNMP_GROUP_SUPPORT ENABLED
#elif (MIB2_SNMP_GROUP_SUPPORT != ENABLED && MIB2_SNMP_GROUP_SUPPORT != DISABLED)
   #error MIB2_SNMP_GROUP_SUPPORT parameter is not valid
#endif

//Size of sysDescr object
#ifndef MIB2_SYS_DESCR_SIZE
   #define MIB2_SYS_DESCR_SIZE 16
#elif (MIB2_SYS_DESCR_SIZE < 0)
   #error MIB2_SYS_DESCR_SIZE parameter is not valid
#endif

//Size of sysObjectID object
#ifndef MIB2_SYS_OBJECT_ID_SIZE
   #define MIB2_SYS_OBJECT_ID_SIZE 16
#elif (MIB2_SYS_OBJECT_ID_SIZE < 0)
   #error MIB2_SYS_OBJECT_ID_SIZE parameter is not valid
#endif

//Size of sysContact object
#ifndef MIB2_SYS_CONTACT_SIZE
   #define MIB2_SYS_CONTACT_SIZE 16
#elif (MIB2_SYS_CONTACT_SIZE < 0)
   #error MIB2_SYS_CONTACT_SIZE parameter is not valid
#endif

//Size of sysName object
#ifndef MIB2_SYS_NAME_SIZE
   #define MIB2_SYS_NAME_SIZE 16
#elif (MIB2_SYS_NAME_SIZE < 0)
   #error MIB2_SYS_NAME_SIZE parameter is not valid
#endif

//Size of sysLocation object
#ifndef MIB2_SYS_LOCATION_SIZE
   #define MIB2_SYS_LOCATION_SIZE 16
#elif (MIB2_SYS_LOCATION_SIZE < 0)
   #error MIB2_SYS_LOCATION_SIZE parameter is not valid
#endif

//Size of ifSpecific object
#ifndef MIB2_IF_SPECIFIC_SIZE
   #define MIB2_IF_SPECIFIC_SIZE 16
#elif (MIB2_IF_SPECIFIC_SIZE < 1)
   #error MIB2_IF_SPECIFIC_SIZE parameter is not valid
#endif

//Size of PhysAddress data type
#ifndef MIB2_PHYS_ADDRESS_SIZE
   #define MIB2_PHYS_ADDRESS_SIZE 6
#elif (MIB2_PHYS_ADDRESS_SIZE < 6)
   #error MIB2_PHYS_ADDRESS_SIZE parameter is not valid
#endif

//Size of IpAddress data type
#ifndef MIB2_IP_ADDRESS_SIZE
   #define MIB2_IP_ADDRESS_SIZE 4
#elif (MIB2_IP_ADDRESS_SIZE != 4)
   #error MIB2_IP_ADDRESS_SIZE parameter is not valid
#endif

//Macro definitions
#if (MIB2_SUPPORT == ENABLED)
   #define MIB2_SET_TIME_TICKS(name, value) mib2Base.name = value
   #define MIB2_INC_COUNTER32(name, value) mib2Base.name += value
#else
   #define MIB2_SET_TIME_TICKS(name, value)
   #define MIB2_INC_COUNTER32(name, value)
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief System services
 **/

typedef enum
{
   MIB2_SYS_SERVICE_PHYSICAL     = 0x01,
   MIB2_SYS_SERVICE_DATALINK     = 0x02,
   MIB2_SYS_SERVICE_INTERNET     = 0x04,
   MIB2_SYS_SERVICE_END_TO_END   = 0x08,
   MIB2_SYS_SERVICE_APPLICATIONS = 0x40
} Mib2SysService;


/**
 * @brief Interface types
 **/

typedef enum
{
   MIB2_IF_TYPE_OTHER           = 1,
   MIB2_IF_TYPE_ETHERNET_CSMACD = 6,
   MIB2_IF_TYPE_PROP_PTP_SERIAL = 22,
   MIB2_IF_TYPE_PPP             = 23,
   MIB2_IF_TYPE_SOFT_LOOPBACK   = 24,
   MIB2_IF_TYPE_SLIP            = 28,
   MIB2_IF_TYPE_RS232           = 33,
   MIB2_IF_TYPE_PARA            = 34,
   MIB2_IF_TYPE_IEEE_802_11     = 71,
   MIB2_IF_TYPE_HDLC            = 118,
   MIB2_IF_TYPE_TUNNEL          = 131,
   MIB2_IF_TYPE_L2_VLAN         = 135,
   MIB2_IF_TYPE_USB             = 160,
   MIB2_IF_TYPE_PLC             = 174,
   MIB2_IF_TYPE_BRIDGE          = 209,
   MIB2_IF_TYPE_IEEE_802_15_4   = 259
} Mib2IfType;


/**
 * @brief The desired state of the interface
 **/

typedef enum
{
   MIB2_IF_ADMIN_STATUS_UP      = 1,
   MIB2_IF_ADMIN_STATUS_DOWN    = 2,
   MIB2_IF_ADMIN_STATUS_TESTING = 3
} Mib2IfAdminStatus;


/**
 * @brief The operational state of the interface
 **/

typedef enum
{
   MIB2_IF_OPER_STATUS_UP      = 1,
   MIB2_IF_OPER_STATUS_DOWN    = 2,
   MIB2_IF_OPER_STATUS_TESTING = 3
} Mib2IfOperStatus;


/**
 * @brief IP forwarding state
 **/

typedef enum
{
   MIB2_IP_FORWARDING_ENABLED  = 1,
   MIB2_IP_FORWARDING_DISABLED = 2
} Mib2IpForwarding;


/**
 * @brief Type of mapping
 **/

typedef enum
{
   MIB2_IP_NET_TO_MEDIA_TYPE_OTHER   = 1,
   MIB2_IP_NET_TO_MEDIA_TYPE_INVALID = 2,
   MIB2_IP_NET_TO_MEDIA_TYPE_DYNAMIC = 3,
   MIB2_IP_NET_TO_MEDIA_TYPE_STATIC  = 4
} Mib2IpNetToMediaType;


/**
 * @brief RTO calculation algorithm
 **/

typedef enum
{
   MIB2_TCP_RTO_ALGORITHM_OTHER    = 1,
   MIB2_TCP_RTO_ALGORITHM_CONSTANT = 2,
   MIB2_TCP_RTO_ALGORITHM_RSRE     = 3,
   MIB2_TCP_RTO_ALGORITHM_VANJ     = 4
} Mib2TcpRtoAlgorithm;


/**
 * @brief TCP connection states
 **/

typedef enum
{
   MIB2_TCP_CONN_STATE_CLOSED       = 1,
   MIB2_TCP_CONN_STATE_LISTEN       = 2,
   MIB2_TCP_CONN_STATE_SYN_SENT     = 3,
   MIB2_TCP_CONN_STATE_SYN_RECEIVED = 4,
   MIB2_TCP_CONN_STATE_ESTABLISHED  = 5,
   MIB2_TCP_CONN_STATE_FIN_WAIT_1   = 6,
   MIB2_TCP_CONN_STATE_FIN_WAIT_2   = 7,
   MIB2_TCP_CONN_STATE_CLOSE_WAIT   = 8,
   MIB2_TCP_CONN_STATE_LAST_ACK     = 9,
   MIB2_TCP_CONN_STATE_CLOSING      = 10,
   MIB2_TCP_CONN_STATE_TIME_WAIT    = 11,
   MIB2_TCP_CONN_STATE_DELETE_TCB   = 12
} Mib2TcpConnState;


/**
 * @brief Enabled/disabled state of authentication failure traps
 **/

typedef enum
{
   MIB2_AUTHEN_TRAPS_ENABLED  = 1,
   MIB2_AUTHEN_TRAPS_DISABLED = 2
} Mib2EnableAuthenTraps;


/**
 * @brief System group
 **/

typedef struct
{
#if (MIB2_SYS_DESCR_SIZE > 0)
   char_t sysDescr[MIB2_SYS_DESCR_SIZE];
   size_t sysDescrLen;
#endif
#if (MIB2_SYS_OBJECT_ID_SIZE > 0)
   uint8_t sysObjectID[MIB2_SYS_OBJECT_ID_SIZE];
   size_t sysObjectIDLen;
#endif
   uint32_t sysUpTime;
#if (MIB2_SYS_CONTACT_SIZE > 0)
   char_t sysContact[MIB2_SYS_CONTACT_SIZE];
   size_t sysContactLen;
#endif
#if (MIB2_SYS_NAME_SIZE > 0)
   char_t sysName[MIB2_SYS_NAME_SIZE];
   size_t sysNameLen;
#endif
#if (MIB2_SYS_LOCATION_SIZE > 0)
   char_t sysLocation[MIB2_SYS_LOCATION_SIZE];
   size_t sysLocationLen;
#endif
   int32_t sysServices;
} Mib2SysGroup;


/**
 * @brief Interfaces table entry
 **/

typedef struct
{
   uint32_t ifLastChange;
   uint32_t ifInOctets;
   uint32_t ifInUcastPkts;
   uint32_t ifInNUcastPkts;
   uint32_t ifInDiscards;
   uint32_t ifInErrors;
   uint32_t ifInUnknownProtos;
   uint32_t ifOutOctets;
   uint32_t ifOutUcastPkts;
   uint32_t ifOutNUcastPkts;
   uint32_t ifOutDiscards;
   uint32_t ifOutErrors;
   uint32_t ifOutQLen;
   uint8_t ifSpecific[MIB2_IF_SPECIFIC_SIZE];
   size_t ifSpecificLen;
} Mib2IfEntry;


/**
 * @brief Interfaces group
 **/

typedef struct
{
   int32_t ifNumber;
   Mib2IfEntry ifTable[NET_INTERFACE_COUNT];
} Mib2IfGroup;


/**
 * @brief IP group
 **/

typedef struct
{
   int32_t ipForwarding;
   int32_t ipDefaultTTL;
   uint32_t ipInReceives;
   uint32_t ipInHdrErrors;
   uint32_t ipInAddrErrors;
   uint32_t ipForwDatagrams;
   uint32_t ipInUnknownProtos;
   uint32_t ipInDiscards;
   uint32_t ipInDelivers;
   uint32_t ipOutRequests;
   uint32_t ipOutDiscards;
   uint32_t ipOutNoRoutes;
   int32_t ipReasmTimeout;
   uint32_t ipReasmReqds;
   uint32_t ipReasmOKs;
   uint32_t ipReasmFails;
   uint32_t ipFragOKs;
   uint32_t ipFragFails;
   uint32_t ipFragCreates;
   uint32_t ipRoutingDiscards;
} Mib2IpGroup;


/**
 * @brief ICMP group
 **/

typedef struct
{
   uint32_t icmpInMsgs;
   uint32_t icmpInErrors;
   uint32_t icmpInDestUnreachs;
   uint32_t icmpInTimeExcds;
   uint32_t icmpInParmProbs;
   uint32_t icmpInSrcQuenchs;
   uint32_t icmpInRedirects;
   uint32_t icmpInEchos;
   uint32_t icmpInEchoReps;
   uint32_t icmpInTimestamps;
   uint32_t icmpInTimestampReps;
   uint32_t icmpInAddrMasks;
   uint32_t icmpInAddrMaskReps;
   uint32_t icmpOutMsgs;
   uint32_t icmpOutErrors;
   uint32_t icmpOutDestUnreachs;
   uint32_t icmpOutTimeExcds;
   uint32_t icmpOutParmProbs;
   uint32_t icmpOutSrcQuenchs;
   uint32_t icmpOutRedirects;
   uint32_t icmpOutEchos;
   uint32_t icmpOutEchoReps;
   uint32_t icmpOutTimestamps;
   uint32_t icmpOutTimestampReps;
   uint32_t icmpOutAddrMasks;
   uint32_t icmpOutAddrMaskReps;
} Mib2IcmpGroup;


/**
 * @brief TCP group
 **/

typedef struct
{
   int32_t tcpRtoAlgorithm;
   int32_t tcpRtoMin;
   int32_t tcpRtoMax;
   int32_t tcpMaxConn;
   uint32_t tcpActiveOpens;
   uint32_t tcpPassiveOpens;
   uint32_t tcpAttemptFails;
   uint32_t tcpEstabResets;
   uint32_t tcpInSegs;
   uint32_t tcpOutSegs;
   uint32_t tcpRetransSegs;
   uint32_t tcpInErrs;
   uint32_t tcpOutRsts;
} Mib2TcpGroup;


/**
 * @brief UDP group
 **/

typedef struct
{
   uint32_t udpInDatagrams;
   uint32_t udpNoPorts;
   uint32_t udpInErrors;
   uint32_t udpOutDatagrams;
} Mib2UdpGroup;


/**
 * @brief SNMP group
 **/

typedef struct
{
   uint32_t snmpInPkts;
   uint32_t snmpOutPkts;
   uint32_t snmpInBadVersions;
   uint32_t snmpInBadCommunityNames;
   uint32_t snmpInBadCommunityUses;
   uint32_t snmpInASNParseErrs;
   uint32_t snmpInTooBigs;
   uint32_t snmpInNoSuchNames;
   uint32_t snmpInBadValues;
   uint32_t snmpInReadOnlys;
   uint32_t snmpInGenErrs;
   uint32_t snmpInTotalReqVars;
   uint32_t snmpInTotalSetVars;
   uint32_t snmpInGetRequests;
   uint32_t snmpInGetNexts;
   uint32_t snmpInSetRequests;
   uint32_t snmpInGetResponses;
   uint32_t snmpInTraps;
   uint32_t snmpOutTooBigs;
   uint32_t snmpOutNoSuchNames;
   uint32_t snmpOutBadValues;
   uint32_t snmpOutGenErrs;
   uint32_t snmpOutGetRequests;
   uint32_t snmpOutGetNexts;
   uint32_t snmpOutSetRequests;
   uint32_t snmpOutGetResponses;
   uint32_t snmpOutTraps;
   int32_t snmpEnableAuthenTraps;
} Mib2SnmpGroup;


/**
 * @brief MIB-II base
 **/

typedef struct
{
   Mib2SysGroup sysGroup;
   Mib2IfGroup ifGroup;
#if (IPV4_SUPPORT == ENABLED)
   Mib2IpGroup ipGroup;
   Mib2IcmpGroup icmpGroup;
#endif
#if (TCP_SUPPORT == ENABLED)
   Mib2TcpGroup tcpGroup;
#endif
#if (UDP_SUPPORT == ENABLED)
   Mib2UdpGroup udpGroup;
#endif
   Mib2SnmpGroup snmpGroup;
} Mib2Base;


//MIB-II related constants
extern Mib2Base mib2Base;
extern const MibObject mib2Objects[];
extern const MibModule mib2Module;

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
