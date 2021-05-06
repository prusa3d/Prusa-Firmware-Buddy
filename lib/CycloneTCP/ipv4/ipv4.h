/**
 * @file ipv4.h
 * @brief IPv4 (Internet Protocol Version 4)
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

#ifndef _IPV4_H
#define _IPV4_H

//Forward declaration of structures
struct _Ipv4Header;
#define Ipv4Header struct _Ipv4Header

struct _Ipv4PseudoHeader;
#define Ipv4PseudoHeader struct _Ipv4PseudoHeader

//Dependencies
#include <string.h>
#include "core/net.h"
#include "core/ethernet.h"
#include "ipv4/ipv4_frag.h"

//IPv4 support
#ifndef IPV4_SUPPORT
   #define IPV4_SUPPORT ENABLED
#elif (IPV4_SUPPORT != ENABLED && IPV4_SUPPORT != DISABLED)
   #error IPV4_SUPPORT parameter is not valid
#endif

//Default IPv4 time-to-live value
#ifndef IPV4_DEFAULT_TTL
   #define IPV4_DEFAULT_TTL 64
#elif (IPV4_DEFAULT_TTL < 1)
   #error IPV4_DEFAULT_TTL parameter is not valid
#endif

//Maximum number of IPv4 addresses
#ifndef IPV4_ADDR_LIST_SIZE
   #define IPV4_ADDR_LIST_SIZE 1
#elif (IPV4_ADDR_LIST_SIZE < 1)
   #error IPV4_ADDR_LIST_SIZE parameter is not valid
#endif

//Maximum number of DNS servers
#ifndef IPV4_DNS_SERVER_LIST_SIZE
   #define IPV4_DNS_SERVER_LIST_SIZE 2
#elif (IPV4_DNS_SERVER_LIST_SIZE < 1)
   #error IPV4_DNS_SERVER_LIST_SIZE parameter is not valid
#endif

//Size of the IPv4 multicast filter
#ifndef IPV4_MULTICAST_FILTER_SIZE
   #define IPV4_MULTICAST_FILTER_SIZE 4
#elif (IPV4_MULTICAST_FILTER_SIZE < 1)
   #error IPV4_MULTICAST_FILTER_SIZE parameter is not valid
#endif

//Version number for IPv4
#define IPV4_VERSION 4
//Minimum MTU
#define IPV4_MINIMUM_MTU 68
//Default MTU
#define IPV4_DEFAULT_MTU 576
//Minimum header length
#define IPV4_MIN_HEADER_LENGTH 20
//Maximum header length
#define IPV4_MAX_HEADER_LENGTH 60

//Shortcut to data field
#define IPV4_DATA(packet) PTR_OFFSET(packet, packet->headerLength * 4)

//Macro used for defining an IPv4 address
#ifdef _CPU_BIG_ENDIAN
   #define IPV4_ADDR(a, b, c, d) (((uint32_t) (a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#else
   #define IPV4_ADDR(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((uint32_t) (d) << 24))
#endif

//Unspecified IPv4 address
#define IPV4_UNSPECIFIED_ADDR IPV4_ADDR(0, 0, 0, 0)
//Broadcast IPV4 address
#define IPV4_BROADCAST_ADDR IPV4_ADDR(255, 255, 255, 255)

//Loopback IPv4 address
#define IPV4_LOOPBACK_ADDR IPV4_ADDR(127, 0, 0, 1)
#define IPV4_LOOPBACK_PREFIX IPV4_ADDR(127, 0, 0, 0)
#define IPV4_LOOPBACK_MASK IPV4_ADDR(255, 0, 0, 0)

//Link-local addresses
#define IPV4_LINK_LOCAL_PREFIX IPV4_ADDR(169, 254, 0, 0)
#define IPV4_LINK_LOCAL_MASK IPV4_ADDR(255, 255, 0, 0)

//Multicast addresses
#define IPV4_MULTICAST_PREFIX IPV4_ADDR(224, 0, 0, 0)
#define IPV4_MULTICAST_MASK IPV4_ADDR(240, 0, 0, 0)

//Local Network Control Block (RFC 5771)
#define IPV4_MULTICAST_LNCB_PREFIX IPV4_ADDR(224, 0, 0, 0)
#define IPV4_MULTICAST_LNCB_MASK IPV4_ADDR(255, 255, 255, 0)

//Internetwork Control Block (RFC 5771)
#define IPV4_MULTICAST_INCB_PREFIX IPV4_ADDR(224, 0, 1, 0)
#define IPV4_MULTICAST_INCB_MASK IPV4_ADDR(255, 255, 255, 0)

//IPv4 address classes
#define IPV4_CLASS_A_ADDR IPV4_ADDR(0, 0, 0, 0)
#define IPV4_CLASS_A_MASK IPV4_ADDR(128, 0, 0, 0)
#define IPV4_CLASS_B_ADDR IPV4_ADDR(128, 0, 0, 0)
#define IPV4_CLASS_B_MASK IPV4_ADDR(192, 0, 0, 0)
#define IPV4_CLASS_C_ADDR IPV4_ADDR(192, 0, 0, 0)
#define IPV4_CLASS_C_MASK IPV4_ADDR(224, 0, 0, 0)
#define IPV4_CLASS_D_ADDR IPV4_ADDR(224, 0, 0, 0)
#define IPV4_CLASS_D_MASK IPV4_ADDR(240, 0, 0, 0)
#define IPV4_CLASS_E_ADDR IPV4_ADDR(240, 0, 0, 0)
#define IPV4_CLASS_E_MASK IPV4_ADDR(240, 0, 0, 0)

//Copy IPv4 address
#define ipv4CopyAddr(destIpAddr, srcIpAddr) \
   osMemcpy(destIpAddr, srcIpAddr, sizeof(Ipv4Addr))

//Compare IPv4 addresses
#define ipv4CompAddr(ipAddr1, ipAddr2) \
   (!osMemcmp(ipAddr1, ipAddr2, sizeof(Ipv4Addr)))

//Determine whether an IPv4 address belongs to the subnet
#define ipv4IsOnSubnet(entry, ipAddr) \
   (((ipAddr) & (entry)->subnetMask) == ((entry)->addr & (entry)->subnetMask))

//Determine whether an IPv4 address is a loopback address
#define ipv4IsLoopbackAddr(ipAddr) \
   (((ipAddr) & IPV4_LOOPBACK_MASK) == IPV4_LOOPBACK_PREFIX)

//Determine whether an IPv4 address is a link-local address
#define ipv4IsLinkLocalAddr(ipAddr) \
   (((ipAddr) & IPV4_LINK_LOCAL_MASK) == IPV4_LINK_LOCAL_PREFIX)

//Determine whether an IPv4 address is a multicast address
#define ipv4IsMulticastAddr(ipAddr) \
   (((ipAddr) & IPV4_MULTICAST_MASK) == IPV4_MULTICAST_PREFIX)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief IPv4 address scopes
 **/

typedef enum
{
   IPV4_ADDR_SCOPE_INTERFACE_LOCAL = 1,
   IPV4_ADDR_SCOPE_LINK_LOCAL      = 2,
   IPV4_ADDR_SCOPE_GLOBAL          = 3
} Ipv4AddrScope;


/**
 * @brief IPv4 address state
 **/

typedef enum
{
   IPV4_ADDR_STATE_INVALID   = 0, ///<An address that is not assigned to any interface
   IPV4_ADDR_STATE_TENTATIVE = 1, ///<An address whose uniqueness on a link is being verified
   IPV4_ADDR_STATE_VALID     = 2  ///<An address assigned to an interface whose use is unrestricted
} Ipv4AddrState;


/**
 * @brief IPv4 fragment offset field
 **/

typedef enum
{
   IPV4_FLAG_RES    = 0x8000,
   IPV4_FLAG_DF     = 0x4000,
   IPV4_FLAG_MF     = 0x2000,
   IPV4_OFFSET_MASK = 0x1FFF
} Ipv4FragmentOffset;


/**
 * @brief IPv4 protocol field
 **/

typedef enum
{
   IPV4_PROTOCOL_ICMP = 1,
   IPV4_PROTOCOL_IGMP = 2,
   IPV4_PROTOCOL_TCP  = 6,
   IPV4_PROTOCOL_UDP  = 17,
   IPV4_PROTOCOL_ESP  = 50,
   IPV4_PROTOCOL_AH   = 51
} Ipv4Protocol;


/**
 * @brief IPv4 option types
 **/

typedef enum
{
   IPV4_OPTION_EEOL   = 0,
   IPV4_OPTION_NOP    = 1,
   IPV4_OPTION_RTRALT = 148
} Ipv4OptionType;


/**
 * @brief IPv4 network address
 **/

typedef uint32_t Ipv4Addr;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief IPv4 header
 **/

__start_packed struct _Ipv4Header
{
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t version : 4;      //0
   uint8_t headerLength : 4;
#else
   uint8_t headerLength : 4; //0
   uint8_t version : 4;
#endif
   uint8_t typeOfService;    //1
   uint16_t totalLength;     //2-3
   uint16_t identification;  //4-5
   uint16_t fragmentOffset;  //6-7
   uint8_t timeToLive;       //8
   uint8_t protocol;         //9
   uint16_t headerChecksum;  //10-11
   Ipv4Addr srcAddr;         //12-15
   Ipv4Addr destAddr;        //16-19
   uint8_t options[];        //20
} __end_packed;


/**
 * @brief IPv4 pseudo header
 **/

__start_packed struct _Ipv4PseudoHeader
{
   Ipv4Addr srcAddr;  //0-3
   Ipv4Addr destAddr; //4-7
   uint8_t reserved;  //8
   uint8_t protocol;  //9
   uint16_t length;   //10-11
} __end_packed;


/**
 * @brief IPv4 option
 **/

typedef __start_packed struct
{
   uint8_t type;    //0
   uint8_t length;  //1
   uint8_t value[]; //2
} __end_packed Ipv4Option;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


/**
 * @brief IPv4 address entry
 **/

typedef struct
{
   Ipv4Addr addr;           ///<IPv4 address
   Ipv4AddrState state;     ///<IPv4 address state
   bool_t conflict;         ///<Address conflict detected
   Ipv4Addr subnetMask;     ///<Subnet mask
   Ipv4Addr defaultGateway; ///<Default gateway
} Ipv4AddrEntry;


/**
 * @brief IPv4 multicast filter entry
 **/

typedef struct
{
   Ipv4Addr addr;    ///<Multicast address
   uint_t refCount;  ///<Reference count for the current entry
   uint_t state;     ///<IGMP host state
   bool_t flag;      ///<IGMP flag
   systime_t timer;  ///<Delay timer
} Ipv4FilterEntry;


/**
 * @brief IPv4 context
 **/

typedef struct
{
   size_t linkMtu;                                              ///<Maximum transmission unit
   bool_t isRouter;                                             ///<A flag indicating whether routing is enabled on this interface
   bool_t enableBroadcastEchoReq;                               ///<Support for broadcast ICMP Echo Request messages
   uint16_t identification;                                     ///<IPv4 fragment identification field
   Ipv4AddrEntry addrList[IPV4_ADDR_LIST_SIZE];                 ///<IPv4 address list
   Ipv4Addr dnsServerList[IPV4_DNS_SERVER_LIST_SIZE];           ///<DNS servers
   Ipv4FilterEntry multicastFilter[IPV4_MULTICAST_FILTER_SIZE]; ///<Multicast filter table
#if (IPV4_FRAG_SUPPORT == ENABLED)
   Ipv4FragDesc fragQueue[IPV4_MAX_FRAG_DATAGRAMS];             ///<IPv4 fragment reassembly queue
#endif
} Ipv4Context;


//IPv4 related functions
error_t ipv4Init(NetInterface *interface);

error_t ipv4SetHostAddr(NetInterface *interface, Ipv4Addr addr);
error_t ipv4SetHostAddrEx(NetInterface *interface, uint_t index, Ipv4Addr addr);
error_t ipv4GetHostAddr(NetInterface *interface, Ipv4Addr *addr);
error_t ipv4GetHostAddrEx(NetInterface *interface, uint_t index, Ipv4Addr *addr);

error_t ipv4SetSubnetMask(NetInterface *interface, Ipv4Addr mask);
error_t ipv4SetSubnetMaskEx(NetInterface *interface, uint_t index, Ipv4Addr mask);
error_t ipv4GetSubnetMask(NetInterface *interface, Ipv4Addr *mask);
error_t ipv4GetSubnetMaskEx(NetInterface *interface, uint_t index, Ipv4Addr *mask);

error_t ipv4SetDefaultGateway(NetInterface *interface, Ipv4Addr addr);

error_t ipv4SetDefaultGatewayEx(NetInterface *interface, uint_t index,
   Ipv4Addr addr);

error_t ipv4GetDefaultGateway(NetInterface *interface, Ipv4Addr *addr);

error_t ipv4GetDefaultGatewayEx(NetInterface *interface, uint_t index,
   Ipv4Addr *addr);

error_t ipv4SetDnsServer(NetInterface *interface, uint_t index, Ipv4Addr addr);
error_t ipv4GetDnsServer(NetInterface *interface, uint_t index, Ipv4Addr *addr);

void ipv4LinkChangeEvent(NetInterface *interface);

void ipv4ProcessPacket(NetInterface *interface, Ipv4Header *packet,
   size_t length, NetRxAncillary *ancillary);

void ipv4ProcessDatagram(NetInterface *interface, const NetBuffer *buffer,
   NetRxAncillary *ancillary);

error_t ipv4SendDatagram(NetInterface *interface, Ipv4PseudoHeader *pseudoHeader,
   NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t ipv4SendPacket(NetInterface *interface, Ipv4PseudoHeader *pseudoHeader,
   uint16_t fragId, size_t fragOffset, NetBuffer *buffer, size_t offset,
   NetTxAncillary *ancillary);

error_t ipv4JoinMulticastGroup(NetInterface *interface, Ipv4Addr groupAddr);
error_t ipv4LeaveMulticastGroup(NetInterface *interface, Ipv4Addr groupAddr);

error_t ipv4StringToAddr(const char_t *str, Ipv4Addr *ipAddr);
char_t *ipv4AddrToString(Ipv4Addr ipAddr, char_t *str);

void ipv4DumpHeader(const Ipv4Header *ipHeader);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
