/**
 * @file ipv4_frag.h
 * @brief IPv4 fragmentation and reassembly
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

#ifndef _IPV4_FRAG_H
#define _IPV4_FRAG_H

//Dependencies
#include "core/net.h"
#include "ipv4/ipv4.h"

//IPv4 fragmentation support
#ifndef IPV4_FRAG_SUPPORT
   #define IPV4_FRAG_SUPPORT ENABLED
#elif (IPV4_FRAG_SUPPORT != ENABLED && IPV4_FRAG_SUPPORT != DISABLED)
   #error IPV4_FRAG_SUPPORT parameter is not valid
#endif

//Reassembly algorithm tick interval
#ifndef IPV4_FRAG_TICK_INTERVAL
   #define IPV4_FRAG_TICK_INTERVAL 1000
#elif (IPV4_FRAG_TICK_INTERVAL < 10)
   #error IPV4_FRAG_TICK_INTERVAL parameter is not valid
#endif

//Maximum number of fragmented packets the host will accept
//and hold in the reassembly queue simultaneously
#ifndef IPV4_MAX_FRAG_DATAGRAMS
   #define IPV4_MAX_FRAG_DATAGRAMS 4
#elif (IPV4_MAX_FRAG_DATAGRAMS < 1)
   #error IPV4_MAX_FRAG_DATAGRAMS parameter is not valid
#endif

//Maximum datagram size the host will accept when reassembling fragments
#ifndef IPV4_MAX_FRAG_DATAGRAM_SIZE
   #define IPV4_MAX_FRAG_DATAGRAM_SIZE 8192
#elif (IPV4_MAX_FRAG_DATAGRAM_SIZE < 576)
   #error IPV4_MAX_FRAG_DATAGRAM_SIZE parameter is not valid
#endif

//Maximum time an IPv4 fragment can spend waiting to be reassembled
#ifndef IPV4_FRAG_TIME_TO_LIVE
   #define IPV4_FRAG_TIME_TO_LIVE 15000
#elif (IPV4_FRAG_TIME_TO_LIVE < 1000)
   #error IPV4_FRAG_TIME_TO_LIVE parameter is not valid
#endif

//Infinity is implemented by a very large integer
#define IPV4_INFINITY 0xFFFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief Hole descriptor
 **/

typedef __start_packed struct
{
   uint16_t first;
   uint16_t last;
   uint16_t next;
} __end_packed Ipv4HoleDesc;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


/**
 * @brief Reassembly buffer
 **/

typedef struct
{
   uint_t chunkCount;
   uint_t maxChunkCount;
   ChunkDesc chunk[N(IPV4_MAX_FRAG_DATAGRAM_SIZE) + 1];
} Ipv4ReassemblyBuffer;


/**
 * @brief Fragmented packet descriptor
 **/

typedef struct
{
   systime_t timestamp;         ///<Time at which the first fragment was received
   size_t headerLength;         ///<Length of the header
   size_t dataLen;              ///<Length of the payload
   uint16_t firstHole;          ///<Index of the first hole
   Ipv4ReassemblyBuffer buffer; ///<Buffer containing the reassembled datagram
} Ipv4FragDesc;


//Tick counter to handle periodic operations
extern systime_t ipv4FragTickCounter;

//IPv4 datagram fragmentation and reassembly
error_t ipv4FragmentDatagram(NetInterface *interface,
   Ipv4PseudoHeader *pseudoHeader, uint16_t id, const NetBuffer *payload,
   size_t payloadOffset, NetTxAncillary *ancillary);

void ipv4ReassembleDatagram(NetInterface *interface, const Ipv4Header *packet,
   size_t length, NetRxAncillary *ancillary);

void ipv4FragTick(NetInterface *interface);

Ipv4FragDesc *ipv4SearchFragQueue(NetInterface *interface,
   const Ipv4Header *packet);

void ipv4FlushFragQueue(NetInterface *interface);

Ipv4HoleDesc *ipv4FindHole(Ipv4FragDesc *frag, uint16_t offset);
void ipv4DumpHoleList(Ipv4FragDesc *frag);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
