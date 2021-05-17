/**
 * @file ipv6.h
 * @brief IPv6 (Internet Protocol Version 6)
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

#ifndef _IPV6_H
#define _IPV6_H

//Forward declaration of structures
struct _Ipv6Header;
#define Ipv6Header struct _Ipv6Header

struct _Ipv6FragmentHeader;
#define Ipv6FragmentHeader struct _Ipv6FragmentHeader

struct _Ipv6PseudoHeader;
#define Ipv6PseudoHeader struct _Ipv6PseudoHeader

//Dependencies
#include <string.h>
#include "core/net.h"
#include "core/ethernet.h"
#include "ipv6/ipv6_frag.h"

//IPv6 support
#ifndef IPV6_SUPPORT
   #define IPV6_SUPPORT DISABLED
#elif (IPV6_SUPPORT != ENABLED && IPV6_SUPPORT != DISABLED)
   #error IPV6_SUPPORT parameter is not valid
#endif

//Default IPv6 Hop Limit field
#ifndef IPV6_DEFAULT_HOP_LIMIT
   #define IPV6_DEFAULT_HOP_LIMIT 64
#elif (IPV6_DEFAULT_HOP_LIMIT < 1)
   #error IPV6_DEFAULT_HOP_LIMIT parameter is not valid
#endif

//Maximum number of IPv6 unicast addresses
#ifndef IPV6_ADDR_LIST_SIZE
   #define IPV6_ADDR_LIST_SIZE 3
#elif (IPV6_ADDR_LIST_SIZE < 2)
   #error IPV6_ADDR_LIST_SIZE parameter is not valid
#endif

//Maximum number of IPv6 anycast addresses
#ifndef IPV6_ANYCAST_ADDR_LIST_SIZE
   #define IPV6_ANYCAST_ADDR_LIST_SIZE 1
#elif (IPV6_ANYCAST_ADDR_LIST_SIZE < 1)
   #error IPV6_ANYCAST_ADDR_LIST_SIZE parameter is not valid
#endif

//Size of the prefix list
#ifndef IPV6_PREFIX_LIST_SIZE
   #define IPV6_PREFIX_LIST_SIZE 2
#elif (IPV6_PREFIX_LIST_SIZE < 1)
   #error IPV6_PREFIX_LIST_SIZE parameter is not valid
#endif

//Maximum number number of default routers
#ifndef IPV6_ROUTER_LIST_SIZE
   #define IPV6_ROUTER_LIST_SIZE 2
#elif (IPV6_ROUTER_LIST_SIZE < 1)
   #error IPV6_ROUTER_LIST_SIZE parameter is not valid
#endif

//Maximum number of DNS servers
#ifndef IPV6_DNS_SERVER_LIST_SIZE
   #define IPV6_DNS_SERVER_LIST_SIZE 2
#elif (IPV6_DNS_SERVER_LIST_SIZE < 1)
   #error IPV6_DNS_SERVER_LIST_SIZE parameter is not valid
#endif

//Size of the IPv6 multicast filter
#ifndef IPV6_MULTICAST_FILTER_SIZE
   #define IPV6_MULTICAST_FILTER_SIZE 8
#elif (IPV6_MULTICAST_FILTER_SIZE < 1)
   #error IPV6_MULTICAST_FILTER_SIZE parameter is not valid
#endif

//Version number for IPv6
#define IPV6_VERSION 6
//Minimum MTU that routers and physical links are required to handle
#define IPV6_DEFAULT_MTU 1280

//Macro used for defining an IPv6 address
#define IPV6_ADDR(a, b, c, d, e, f, g, h) {{{ \
   MSB(a), LSB(a), MSB(b), LSB(b), MSB(c), LSB(c), MSB(d), LSB(d), \
   MSB(e), LSB(e), MSB(f), LSB(f), MSB(g), LSB(g), MSB(h), LSB(h)}}}

//Copy IPv6 address
#define ipv6CopyAddr(destIpAddr, srcIpAddr) \
   osMemcpy(destIpAddr, srcIpAddr, sizeof(Ipv6Addr))

//Compare IPv6 addresses
#define ipv6CompAddr(ipAddr1, ipAddr2) \
   (!osMemcmp(ipAddr1, ipAddr2, sizeof(Ipv6Addr)))

//Determine whether an IPv6 address is a link-local unicast address
#define ipv6IsLinkLocalUnicastAddr(ipAddr) \
   ((ipAddr)->b[0] == 0xFE && ((ipAddr)->b[1] & 0xC0) == 0x80)

//Determine whether an IPv6 address is a site-local unicast address
#define ipv6IsSiteLocalUnicastAddr(ipAddr) \
   ((ipAddr)->b[0] == 0xFE && ((ipAddr)->b[1] & 0xC0) == 0xC0)

//Determine whether an IPv6 address is a multicast address
#define ipv6IsMulticastAddr(ipAddr) \
   ((ipAddr)->b[0] == 0xFF)

//Determine whether an IPv6 address is a solicited-node address
#define ipv6IsSolicitedNodeAddr(ipAddr) \
   ipv6CompPrefix(ipAddr, &IPV6_SOLICITED_NODE_ADDR_PREFIX, 104)

//Get the state of the link-local address
#define ipv6GetLinkLocalAddrState(interface) \
   (interface->ipv6Context.addrList[0].state)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief IPv6 address scopes
 **/

typedef enum
{
   IPV6_ADDR_SCOPE_INTERFACE_LOCAL    = 1,
   IPV6_ADDR_SCOPE_LINK_LOCAL         = 2,
   IPV6_ADDR_SCOPE_ADMIN_LOCAL        = 4,
   IPV6_ADDR_SCOPE_SITE_LOCAL         = 5,
   IPV6_ADDR_SCOPE_ORGANIZATION_LOCAL = 8,
   IPV6_ADDR_SCOPE_GLOBAL             = 14
} Ipv6AddrScope;


/**
 * @brief IPv6 address state
 **/

typedef enum
{
   IPV6_ADDR_STATE_INVALID    = 0, ///<An address that is not assigned to any interface
   IPV6_ADDR_STATE_TENTATIVE  = 1, ///<An address whose uniqueness on a link is being verified
   IPV6_ADDR_STATE_PREFERRED  = 2, ///<An address assigned to an interface whose use is unrestricted
   IPV6_ADDR_STATE_DEPRECATED = 3  ///<An address assigned to an interface whose use is discouraged
} Ipv6AddrState;


/**
 * @brief IPv6 Next Header types
 **/

typedef enum
{
   IPV6_HOP_BY_HOP_OPT_HEADER = 0,
   IPV6_TCP_HEADER            = 6,
   IPV6_UDP_HEADER            = 17,
   IPV6_ROUTING_HEADER        = 43,
   IPV6_FRAGMENT_HEADER       = 44,
   IPV6_ESP_HEADER            = 50,
   IPV6_AH_HEADER             = 51,
   IPV6_ICMPV6_HEADER         = 58,
   IPV6_NO_NEXT_HEADER        = 59,
   IPV6_DEST_OPT_HEADER       = 60
} Ipv6NextHeaderType;


/**
 * @brief IPv6 fragment offset field
 **/

typedef enum
{
   IPV6_OFFSET_MASK = 0xFFF8,
   IPV6_FLAG_RES1   = 0x0004,
   IPV6_FLAG_RES2   = 0x0002,
   IPV6_FLAG_M      = 0x0001
} Ipv6FragmentOffset;


/**
 * @brief IPv6 option types
 **/

typedef enum
{
   IPV6_OPTION_TYPE_MASK = 0x1F,
   IPV6_OPTION_TYPE_PAD1 = 0x00,
   IPV6_OPTION_TYPE_PADN = 0x01
} Ipv6OptionType;


/**
 * @brief Actions to be taken for unrecognized options
 **/

typedef enum
{
   IPV6_ACTION_MASK                = 0xC0,
   IPV6_ACTION_SKIP_OPTION         = 0x00,
   IPV6_ACTION_DISCARD_PACKET      = 0x40,
   IPV6_ACTION_SEND_ICMP_ERROR_ALL = 0x80,
   IPV6_ACTION_SEND_ICMP_ERROR_UNI = 0xC0
} Ipv6Actions;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief IPv6 network address
 **/

typedef __start_packed struct
{
   __start_packed union
   {
      uint8_t b[16];
      uint16_t w[8];
      uint32_t dw[4];
   };
} __end_packed Ipv6Addr;


/**
 * @brief IPv6 header
 **/

__start_packed struct _Ipv6Header
{
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t version : 4;       //0
   uint8_t trafficClassH : 4;
   uint8_t trafficClassL : 4; //1
   uint8_t flowLabelH : 4;
#else
   uint8_t trafficClassH : 4; //0
   uint8_t version : 4;
   uint8_t flowLabelH : 4;    //1
   uint8_t trafficClassL : 4;
#endif
   uint16_t flowLabelL;       //2-3
   uint16_t payloadLen;       //4-5
   uint8_t nextHeader;        //6
   uint8_t hopLimit;          //7
   Ipv6Addr srcAddr;          //8-23
   Ipv6Addr destAddr;         //24-39
   uint8_t payload[];         //40
} __end_packed;


/**
 * @brief IPv6 Hop-by-Hop Options header
 **/

typedef __start_packed struct
{
   uint8_t nextHeader; //0
   uint8_t hdrExtLen;  //1
   uint8_t options[];  //2
} __end_packed Ipv6HopByHopOptHeader;


/**
 * @brief IPv6 Destination Options header
 **/

typedef __start_packed struct
{
   uint8_t nextHeader; //0
   uint8_t hdrExtLen;  //1
   uint8_t options[];  //2
} __end_packed Ipv6DestOptHeader;


/**
 * @brief IPv6 Type 0 Routing header
 **/

typedef __start_packed struct
{
   uint8_t nextHeader;   //0
   uint8_t hdrExtLen;    //1
   uint8_t routingType;  //2
   uint8_t segmentsLeft; //3
   uint32_t reserved;    //4-7
   Ipv6Addr address[];   //8
} __end_packed Ipv6RoutingHeader;


/**
 * @brief IPv6 Fragment header
 **/

__start_packed struct _Ipv6FragmentHeader
{
   uint8_t nextHeader;      //0
   uint8_t reserved;        //1
   uint16_t fragmentOffset; //2-3
   uint32_t identification; //4-7
} __end_packed;


/**
 * @brief IPv6 Authentication header
 **/

typedef __start_packed struct
{
   uint8_t nextHeader;          //0
   uint8_t payloadLen;          //1
   uint16_t reserved;           //2-3
   uint32_t securityParamIndex; //4-7
   uint32_t sequenceNumber;     //8-11
   uint8_t authData[];          //12
} __end_packed Ipv6AuthHeader;


/**
 * @brief IPv6 Encapsulating Security Payload header
 **/

typedef __start_packed struct
{
   uint32_t securityParamIndex; //0-3
   uint32_t sequenceNumber;     //4-7
   uint8_t payloadData[];       //8
} __end_packed Ipv6EspHeader;


/**
 * @brief IPv6 option
 **/

typedef __start_packed struct
{
   uint8_t type;   //0
   uint8_t length; //1
   uint8_t data[]; //2
} __end_packed Ipv6Option;


/**
 * @brief IPv6 pseudo header
 **/

__start_packed struct _Ipv6PseudoHeader
{
   Ipv6Addr srcAddr;    //0-15
   Ipv6Addr destAddr;   //16-31
   uint32_t length;     //32-35
   uint8_t reserved[3]; //36-38
   uint8_t nextHeader;  //39
} __end_packed;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


/**
 * @brief IPv6 address entry
 **/

typedef struct
{
   Ipv6Addr addr;               ///<IPv6 address
   Ipv6AddrState state;         ///<IPv6 address state
   bool_t duplicate;            ///<The address is a duplicate
   systime_t validLifetime;     ///<Valid lifetime
   systime_t preferredLifetime; ///<Preferred lifetime
   bool_t permanent;            ///<Permanently assigned address
   systime_t timestamp;         ///<Timestamp to manage entry lifetime
   systime_t dadTimeout;        ///<Timeout value for Duplicate Address Detection
   uint_t dadRetransmitCount;   ///<Retransmission counter for Duplicate Address Detection
} Ipv6AddrEntry;


/**
 * @brief Prefix list entry
 **/

typedef struct
{
   Ipv6Addr prefix;             ///<IPv6 prefix information
   uint8_t prefixLen;           ///<IPv6 prefix length
   bool_t onLinkFlag;           ///<On-link flag
   bool_t autonomousFlag;       ///<Autonomous flag
   systime_t validLifetime;     ///<Valid lifetime
   systime_t preferredLifetime; ///<Preferred lifetime
   bool_t permanent;            ///<Permanently assigned prefix
   systime_t timestamp;         ///<Timestamp to manage entry lifetime
} Ipv6PrefixEntry;


/**
 * @brief Default router list entry
 **/

typedef struct
{
   Ipv6Addr addr;       ///<Router address
   systime_t lifetime;  ///<Router lifetime
   uint8_t preference;  ///<Preference value
   bool_t permanent;    ///<Permanently assigned router
   systime_t timestamp; ///<Timestamp to manage entry lifetime
} Ipv6RouterEntry;


/**
 * @brief IPv6 multicast filter entry
 **/

typedef struct
{
   Ipv6Addr addr;   ///<Multicast address
   uint_t refCount; ///<Reference count for the current entry
   uint_t state;    ///<MLD node state
   bool_t flag;     ///<MLD flag
   systime_t timer; ///<Delay timer
} Ipv6FilterEntry;


/**
 * @brief IPv6 context
 **/

typedef struct
{
   size_t linkMtu;                                              ///<Maximum transmission unit
   bool_t isRouter;                                             ///<A flag indicating whether routing is enabled on this interface
   uint8_t curHopLimit;                                         ///<Default value for the Hop Limit field
   bool_t enableMulticastEchoReq;                               ///<Support for multicast ICMPv6 Echo Request messages
   Ipv6AddrEntry addrList[IPV6_ADDR_LIST_SIZE];                 ///<IPv6 unicast address list
   Ipv6Addr anycastAddrList[IPV6_ANYCAST_ADDR_LIST_SIZE];       ///<IPv6 anycast address list
   Ipv6PrefixEntry prefixList[IPV6_PREFIX_LIST_SIZE];           ///<Prefix list
   Ipv6RouterEntry routerList[IPV6_ROUTER_LIST_SIZE];           ///<Default router list
   Ipv6Addr dnsServerList[IPV6_DNS_SERVER_LIST_SIZE];           ///<DNS servers
   Ipv6FilterEntry multicastFilter[IPV6_MULTICAST_FILTER_SIZE]; ///<Multicast filter table
#if (IPV6_FRAG_SUPPORT == ENABLED)
   uint32_t identification;                                     ///<IPv6 fragment identification field
   Ipv6FragDesc fragQueue[IPV6_MAX_FRAG_DATAGRAMS];             ///<IPv6 fragment reassembly queue
#endif
} Ipv6Context;


//IPv6 related constants
extern const Ipv6Addr IPV6_UNSPECIFIED_ADDR;
extern const Ipv6Addr IPV6_LOOPBACK_ADDR;
extern const Ipv6Addr IPV6_LINK_LOCAL_ALL_NODES_ADDR;
extern const Ipv6Addr IPV6_LINK_LOCAL_ALL_ROUTERS_ADDR;
extern const Ipv6Addr IPV6_LINK_LOCAL_ADDR_PREFIX;
extern const Ipv6Addr IPV6_SOLICITED_NODE_ADDR_PREFIX;

//IPv6 related functions
error_t ipv6Init(NetInterface *interface);

error_t ipv6SetMtu(NetInterface *interface, size_t mtu);
error_t ipv6GetMtu(NetInterface *interface, size_t *mtu);

error_t ipv6SetLinkLocalAddr(NetInterface *interface, const Ipv6Addr *addr);
error_t ipv6GetLinkLocalAddr(NetInterface *interface, Ipv6Addr *addr);

error_t ipv6SetGlobalAddr(NetInterface *interface, uint_t index, const Ipv6Addr *addr);
error_t ipv6GetGlobalAddr(NetInterface *interface, uint_t index, Ipv6Addr *addr);

error_t ipv6SetAnycastAddr(NetInterface *interface, uint_t index, const Ipv6Addr *addr);
error_t ipv6GetAnycastAddr(NetInterface *interface, uint_t index, Ipv6Addr *addr);

error_t ipv6SetPrefix(NetInterface *interface,
   uint_t index, const Ipv6Addr *prefix, uint_t length);

error_t ipv6GetPrefix(NetInterface *interface,
   uint_t index, Ipv6Addr *prefix, uint_t *length);

error_t ipv6SetDefaultRouter(NetInterface *interface, uint_t index, const Ipv6Addr *addr);
error_t ipv6GetDefaultRouter(NetInterface *interface, uint_t index, Ipv6Addr *addr);

error_t ipv6SetDnsServer(NetInterface *interface, uint_t index, const Ipv6Addr *addr);
error_t ipv6GetDnsServer(NetInterface *interface, uint_t index, Ipv6Addr *addr);

void ipv6LinkChangeEvent(NetInterface *interface);

void ipv6ProcessPacket(NetInterface *interface, NetBuffer *ipPacket,
   size_t ipPacketOffset, NetRxAncillary *ancillary);

error_t ipv6ParseHopByHopOptHeader(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t *headerOffset, size_t *nextHeaderOffset);

error_t ipv6ParseDestOptHeader(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t *headerOffset, size_t *nextHeaderOffset);

error_t ipv6ParseRoutingHeader(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t *headerOffset, size_t *nextHeaderOffset);

error_t ipv6ParseAuthHeader(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t *headerOffset, size_t *nextHeaderOffset);

error_t ipv6ParseEspHeader(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t *headerOffset, size_t *nextHeaderOffset);

error_t ipv6ParseOptions(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t optionOffset, size_t optionLen);

error_t ipv6SendDatagram(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t ipv6SendPacket(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   uint32_t fragId, size_t fragOffset, NetBuffer *buffer, size_t offset,
   NetTxAncillary *ancillary);

error_t ipv6JoinMulticastGroup(NetInterface *interface, const Ipv6Addr *groupAddr);
error_t ipv6LeaveMulticastGroup(NetInterface *interface, const Ipv6Addr *groupAddr);

error_t ipv6StringToAddr(const char_t *str, Ipv6Addr *ipAddr);
char_t *ipv6AddrToString(const Ipv6Addr *ipAddr, char_t *str);

void ipv6DumpHeader(const Ipv6Header *ipHeader);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
