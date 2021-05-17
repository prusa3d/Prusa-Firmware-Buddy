/**
 * @file llmnr_common.h
 * @brief Definitions common to LLMNR client and responder
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

#ifndef _LLMNR_COMMON_H
#define _LLMNR_COMMON_H

//Dependencies
#include "core/net.h"
#include "dns/dns_common.h"

//Default resource record TTL (cache lifetime)
#ifndef LLMNR_DEFAULT_RESOURCE_RECORD_TTL
   #define LLMNR_DEFAULT_RESOURCE_RECORD_TTL 30
#elif (LLMNR_DEFAULT_RESOURCE_RECORD_TTL < 1)
   #error LLMNR_DEFAULT_RESOURCE_RECORD_TTL parameter is not valid
#endif

//LLMNR port number
#define LLMNR_PORT 5355
//Default IP TTL value
#define LLMNR_DEFAULT_IP_TTL 255

//LLMNR IPv4 multicast group
#define LLMNR_IPV4_MULTICAST_ADDR IPV4_ADDR(224, 0, 0, 252)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief LLMNR message header
 **/

typedef __start_packed struct
{
   uint16_t id;         //0-1
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t qr : 1;      //2
   uint8_t opcode : 4;
   uint8_t c : 1;
   uint8_t tc : 1;
   uint8_t t : 1;
   uint8_t z : 4;       //3
   uint8_t rcode : 4;
#else
   uint8_t t : 1;       //2
   uint8_t tc : 1;
   uint8_t c : 1;
   uint8_t opcode : 4;
   uint8_t qr : 1;
   uint8_t rcode : 4;   //3
   uint8_t z : 4;
#endif
   uint16_t qdcount;    //4-5
   uint16_t ancount;    //6-7
   uint16_t nscount;    //8-9
   uint16_t arcount;    //10-11
   uint8_t questions[]; //12
} __end_packed LlmnrHeader;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


//LLMNR IPv6 multicast group
extern const Ipv6Addr LLMNR_IPV6_MULTICAST_ADDR;

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
