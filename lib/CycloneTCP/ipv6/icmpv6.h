/**
 * @file icmpv6.h
 * @brief ICMPv6 (Internet Control Message Protocol Version 6)
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

#ifndef _ICMPV6_H
#define _ICMPV6_H

//Dependencies
#include "core/net.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief ICMPv6 message type
 *
 * The type field indicates the type of the message. Its
 * value determines the format of the remaining data
 *
 **/

typedef enum
{
   ICMPV6_TYPE_DEST_UNREACHABLE             = 1,
   ICMPV6_TYPE_PACKET_TOO_BIG               = 2,
   ICMPV6_TYPE_TIME_EXCEEDED                = 3,
   ICMPV6_TYPE_PARAM_PROBLEM                = 4,
   ICMPV6_TYPE_ECHO_REQUEST                 = 128,
   ICMPV6_TYPE_ECHO_REPLY                   = 129,
   ICMPV6_TYPE_MULTICAST_LISTENER_QUERY     = 130,
   ICMPV6_TYPE_MULTICAST_LISTENER_REPORT_V1 = 131,
   ICMPV6_TYPE_MULTICAST_LISTENER_DONE_V1   = 132,
   ICMPV6_TYPE_ROUTER_SOL                   = 133,
   ICMPV6_TYPE_ROUTER_ADV                   = 134,
   ICMPV6_TYPE_NEIGHBOR_SOL                 = 135,
   ICMPV6_TYPE_NEIGHBOR_ADV                 = 136,
   ICMPV6_TYPE_REDIRECT                     = 137,
   ICMPV6_TYPE_MULTICAST_LISTENER_REPORT_V2 = 143
} Icmpv6Type;


/**
 * @brief Destination Unreachable message codes
 **/

typedef enum
{
   ICMPV6_CODE_NO_ROUTE_TO_DEST         = 0,
   ICMPV6_CODE_ADMIN_PROHIBITED         = 1,
   ICMPV6_CODE_BEYOND_SCOPE_OF_SRC_ADDR = 2,
   ICMPV6_CODE_ADDR_UNREACHABLE         = 3,
   ICMPV6_CODE_PORT_UNREACHABLE         = 4
} Icmpv6DestUnreachableCode;


/**
 * @brief Time Exceeded message codes
 **/

typedef enum
{
   ICMPV6_CODE_HOP_LIMIT_EXCEEDED       = 0,
   ICMPV6_CODE_REASSEMBLY_TIME_EXCEEDED = 1
} Icmpv6TimeExceededCode;


/**
 * @brief Parameter Problem message codes
 **/
typedef enum
{
   ICMPV6_CODE_INVALID_HEADER_FIELD = 0,
   ICMPV6_CODE_UNKNOWN_NEXT_HEADER  = 1,
   ICMPV6_CODE_UNKNOWN_IPV6_OPTION  = 2
} Icmpv6ParamProblemCode;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief ICMPv6 header
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t code;      //1
   uint16_t checksum; //2-3
   uint8_t data[];    //4
} __end_packed Icmpv6Header;


/**
 * @brief ICMPv6 Error message
 **/

typedef __start_packed struct
{
   uint8_t type;       //0
   uint8_t code;       //1
   uint16_t checksum;  //2-3
   uint32_t parameter; //4-7
   uint8_t data[];     //8
} __end_packed Icmpv6ErrorMessage;


/**
 * @brief ICMPv6 Destination Unreachable message
 *
 * A Destination Unreachable message is generated in response to a
 * packet that cannot be delivered to its destination address for
 * reasons other than congestion
 *
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t code;      //1
   uint16_t checksum; //2-3
   uint32_t unused;   //4-7
   uint8_t data[];    //8
} __end_packed Icmpv6DestUnreachableMessage;


/**
 * @brief ICMPv6 Packet Too Big message
 *
 * A Packet Too Big message is sent by a router in response
 * to a packet that it cannot forward because the packet is
 * larger than the MTU of the outgoing link
 *
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t code;      //1
   uint16_t checksum; //2-3
   uint32_t mtu;      //4-7
   uint8_t data[];    //8
} __end_packed Icmpv6PacketTooBigMessage;


/**
 * @brief ICMPv6 Time Exceeded message
 *
 * A Time Exceeded message is sent by a router when it receives
 * a packet with a Hop Limit of zero
 *
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t code;      //1
   uint16_t checksum; //2-3
   uint32_t unused;   //4-7
   uint8_t data[];    //8
} __end_packed Icmpv6TimeExceededMessage;


/**
 * @brief ICMPv6 Parameter Problem message
 *
 * A Parameter Problem message is sent by an IPv6 node when it finds a
 * problem with a field in the IPv6 header or extension headers such
 * that it cannot complete processing the packet
 *
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t code;      //1
   uint16_t checksum; //2-3
   uint32_t pointer;  //4-7
   uint8_t data[];    //8
} __end_packed Icmpv6ParamProblemMessage;


/**
 * @brief ICMPv6 Echo Request and Echo Reply messages
 *
 * Every node must implement an ICMPv6 Echo responder function that
 * receives Echo Requests and sends corresponding Echo Replies
 *
 **/

typedef __start_packed struct
{
   uint8_t type;            //0
   uint8_t code;            //1
   uint16_t checksum;       //2-3
   uint16_t identifier;     //4-6
   uint16_t sequenceNumber; //7-8
   uint8_t data[];          //8
} __end_packed Icmpv6EchoMessage;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


//ICMPv6 related functions
error_t icmpv6EnableMulticastEchoRequest(NetInterface *interface, bool_t enable);

void icmpv6ProcessMessage(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit);

void icmpv6ProcessDestUnreachable(NetInterface *interface,
   Ipv6PseudoHeader *pseudoHeader, const NetBuffer *buffer, size_t offset);

void icmpv6ProcessPacketTooBig(NetInterface *interface,
   Ipv6PseudoHeader *pseudoHeader, const NetBuffer *buffer, size_t offset);

void icmpv6ProcessEchoRequest(NetInterface *interface, Ipv6PseudoHeader *requestPseudoHeader,
   const NetBuffer *request, size_t requestOffset);

error_t icmpv6SendErrorMessage(NetInterface *interface, uint8_t type, uint8_t code,
   uint32_t parameter, const NetBuffer *ipPacket, size_t ipPacketOffset);

void icmpv6DumpMessage(const Icmpv6Header *message);
void icmpv6DumpDestUnreachableMessage(const Icmpv6DestUnreachableMessage *message);
void icmpv6DumpPacketTooBigMessage(const Icmpv6PacketTooBigMessage *message);
void icmpv6DumpEchoMessage(const Icmpv6EchoMessage *message);
void icmpv6DumpErrorMessage(const Icmpv6ErrorMessage *message);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
