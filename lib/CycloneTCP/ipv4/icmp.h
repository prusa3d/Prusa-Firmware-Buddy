/**
 * @file icmp.h
 * @brief ICMP (Internet Control Message Protocol)
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

#ifndef _ICMP_H
#define _ICMP_H

//Dependencies
#include "core/net.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief ICMP message type
 *
 * The type field indicates the type of the message. Its
 * value determines the format of the remaining data
 *
 **/

typedef enum
{
   ICMP_TYPE_ECHO_REPLY        = 0,
   ICMP_TYPE_DEST_UNREACHABLE  = 3,
   ICMP_TYPE_SOURCE_QUENCH     = 4,
   ICMP_TYPE_REDIRECT          = 5,
   ICMP_TYPE_ECHO_REQUEST      = 8,
   ICMP_TYPE_TIME_EXCEEDED     = 11,
   ICMP_TYPE_PARAM_PROBLEM     = 12,
   ICMP_TYPE_TIMESTAMP_REQUEST = 13,
   ICMP_TYPE_TIMESTAMP_REPLY   = 14,
   ICMP_TYPE_INFO_REQUEST      = 15,
   ICMP_TYPE_INFO_REPLY        = 16,
   ICMP_TYPE_ADDR_MASK_REQUEST = 17,
   ICMP_TYPE_ADDR_MASK_REPLY   = 18
} IcmpType;


/**
 * @brief Destination Unreachable message codes
 **/

typedef enum
{
   ICMP_CODE_NET_UNREACHABLE        = 0,
   ICMP_CODE_HOST_UNREACHABLE       = 1,
   ICMP_CODE_PROTOCOL_UNREACHABLE   = 2,
   ICMP_CODE_PORT_UNREACHABLE       = 3,
   ICMP_CODE_FRAG_NEEDED_AND_DF_SET = 4,
   ICMP_CODE_SOURCE_ROUTE_FAILED    = 5
} IcmpDestUnreachableCode;


/**
 * @brief Time Exceeded message codes
 **/

typedef enum
{
   ICMP_CODE_TTL_EXCEEDED             = 0,
   ICMP_CODE_REASSEMBLY_TIME_EXCEEDED = 1
} IcmpTimeExceededCode;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief ICMP header
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t code;      //1
   uint16_t checksum; //2-3
   uint8_t data[];    //4
} __end_packed IcmpHeader;


/**
 * @brief ICMP Error message
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t code;      //1
   uint16_t checksum; //2-3
   uint8_t parameter; //4
   uint8_t unused[3]; //5-7
   uint8_t data[];    //8
} __end_packed IcmpErrorMessage;


/**
 * @brief ICMP Destination Unreachable message
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t code;      //1
   uint16_t checksum; //2-3
   uint32_t unused;   //4-7
   uint8_t data[];    //8
} __end_packed IcmpDestUnreachableMessage;


/**
 * @brief ICMP Time Exceeded message
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t code;      //1
   uint16_t checksum; //2-3
   uint32_t unused;   //4-7
   uint8_t data[];    //8
} __end_packed IcmpTimeExceededMessage;


/**
 * @brief ICMP Parameter Problem message
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t code;      //1
   uint16_t checksum; //2-3
   uint8_t pointer;   //4
   uint8_t unused[3]; //5-7
   uint8_t data[];    //8
} __end_packed IcmpParamProblemMessage;


/**
 * @brief ICMP Echo Request and Echo Reply messages
 **/

typedef __start_packed struct
{
   uint8_t type;            //0
   uint8_t code;            //1
   uint16_t checksum;       //2-3
   uint16_t identifier;     //4-5
   uint16_t sequenceNumber; //6-7
   uint8_t data[];          //8
} __end_packed IcmpEchoMessage;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


//ICMP related functions
void icmpProcessMessage(NetInterface *interface,
   Ipv4PseudoHeader *requestPseudoHeader, const NetBuffer *buffer,
   size_t offset);

void icmpProcessEchoRequest(NetInterface *interface,
   Ipv4PseudoHeader *requestPseudoHeader, const NetBuffer *request,
   size_t requestOffset);

error_t icmpSendErrorMessage(NetInterface *interface, uint8_t type, uint8_t code,
   uint8_t parameter, const NetBuffer *ipPacket, size_t ipPacketOffset);

void icmpUpdateInStats(uint8_t type);
void icmpUpdateOutStats(uint8_t type);

void icmpDumpMessage(const IcmpHeader *message);
void icmpDumpEchoMessage(const IcmpEchoMessage *message);
void icmpDumpErrorMessage(const IcmpErrorMessage *message);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
