/**
 * @file arp.h
 * @brief ARP (Address Resolution Protocol)
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

#ifndef _ARP_H
#define _ARP_H

//Dependencies
#include "core/net.h"

//ARP tick interval
#ifndef ARP_TICK_INTERVAL
   #define ARP_TICK_INTERVAL 200
#elif (ARP_TICK_INTERVAL < 10)
   #error ARP_TICK_INTERVAL parameter is not valid
#endif

//Size of ARP cache
#ifndef ARP_CACHE_SIZE
   #define ARP_CACHE_SIZE 8
#elif (ARP_CACHE_SIZE < 4)
   #error ARP_CACHE_SIZE parameter is not valid
#endif

//Maximum number of packets waiting for address resolution to complete
#ifndef ARP_MAX_PENDING_PACKETS
   #define ARP_MAX_PENDING_PACKETS 2
#elif (ARP_MAX_PENDING_PACKETS < 1)
   #error ARP_MAX_PENDING_PACKETS parameter is not valid
#endif

//Maximum number of times that an ARP request will be retransmitted
#ifndef ARP_MAX_REQUESTS
   #define ARP_MAX_REQUESTS 3
#elif (ARP_MAX_REQUESTS < 1)
   #error ARP_MAX_REQUESTS parameter is not valid
#endif

//Time interval between subsequent retransmissions of ARP requests
#ifndef ARP_REQUEST_TIMEOUT
   #define ARP_REQUEST_TIMEOUT 1000
#elif (ARP_REQUEST_TIMEOUT < 100)
   #error ARP_REQUEST_TIMEOUT parameter is not valid
#endif

//Maximum number of times that a probe will be retransmitted
#ifndef ARP_MAX_PROBES
   #define ARP_MAX_PROBES 2
#elif (ARP_MAX_PROBES < 1)
   #error ARP_MAX_PROBES parameter is not valid
#endif

//time interval between subsequent retransmissions of probes
#ifndef ARP_PROBE_TIMEOUT
   #define ARP_PROBE_TIMEOUT 60000
#elif (ARP_PROBE_TIMEOUT < 1000)
   #error ARP_PROBE_TIMEOUT parameter is not valid
#endif

//The time a host is considered reachable after receiving a reachability confirmation
#ifndef ARP_REACHABLE_TIME
   #define ARP_REACHABLE_TIME 60000
#elif (ARP_REACHABLE_TIME < 1000)
   #error ARP_REACHABLE_TIME parameter is not valid
#endif

//Delay before sending the first probe
#ifndef ARP_DELAY_FIRST_PROBE_TIME
   #define ARP_DELAY_FIRST_PROBE_TIME 5000
#elif (ARP_DELAY_FIRST_PROBE_TIME < 1000)
   #error ARP_DELAY_FIRST_PROBE_TIME parameter is not valid
#endif

//Hardware type
#define ARP_HARDWARE_TYPE_ETH 0x0001
//Protocol type
#define ARP_PROTOCOL_TYPE_IPV4 0x0800

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief ARP opcodes
 **/

typedef enum
{
   ARP_OPCODE_ARP_REQUEST = 1,
   ARP_OPCODE_ARP_REPLY   = 2
} ArpOpcode;


/**
 * @brief ARP cache entry states
 **/

typedef enum
{
   ARP_STATE_NONE       = 0,
   ARP_STATE_INCOMPLETE = 1,
   ARP_STATE_REACHABLE  = 2,
   ARP_STATE_STALE      = 3,
   ARP_STATE_DELAY      = 4,
   ARP_STATE_PROBE      = 5,
   ARP_STATE_PERMANENT  = 6
} ArpState;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief ARP packet
 **/

typedef __start_packed struct
{
   uint16_t hrd; //0-1
   uint16_t pro; //2-3
   uint8_t hln;  //4
   uint8_t pln;  //5
   uint16_t op;  //6-7
   MacAddr sha;  //8-13
   Ipv4Addr spa; //14-17
   MacAddr tha;  //18-23
   Ipv4Addr tpa; //24-27
} __end_packed ArpPacket;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


/**
 * @brief ARP queue item
 **/

typedef struct
{
   NetBuffer *buffer;        ///<Packet waiting for address resolution
   size_t offset;            ///<Offset to the first byte of the packet
   NetTxAncillary ancillary; ///<Additional options
} ArpQueueItem;


/**
 * @brief ARP cache entry
 **/

typedef struct
{
   ArpState state;                              ///<Reachability state
   Ipv4Addr ipAddr;                             ///<Unicast IPv4 address
   MacAddr macAddr;                             ///<Link layer address associated with the IPv4 address
   systime_t timestamp;                         ///<Time stamp to manage entry lifetime
   systime_t timeout;                           ///<Timeout value
   uint_t retransmitCount;                      ///<Retransmission counter
   ArpQueueItem queue[ARP_MAX_PENDING_PACKETS]; ///<Packets waiting for address resolution to complete
   uint_t queueSize;                            ///<Number of queued packets
} ArpCacheEntry;


//Tick counter to handle periodic operations
extern systime_t arpTickCounter;

//ARP related functions
error_t arpInit(NetInterface *interface);
void arpFlushCache(NetInterface *interface);

ArpCacheEntry *arpCreateEntry(NetInterface *interface);
ArpCacheEntry *arpFindEntry(NetInterface *interface, Ipv4Addr ipAddr);

void arpSendQueuedPackets(NetInterface *interface, ArpCacheEntry *entry);
void arpFlushQueuedPackets(NetInterface *interface, ArpCacheEntry *entry);

error_t arpResolve(NetInterface *interface, Ipv4Addr ipAddr, MacAddr *macAddr);

error_t arpEnqueuePacket(NetInterface *interface, Ipv4Addr ipAddr,
   NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

void arpTick(NetInterface *interface);

void arpProcessPacket(NetInterface *interface, ArpPacket *arpPacket,
   size_t length);

void arpProcessRequest(NetInterface *interface, ArpPacket *arpRequest);
void arpProcessReply(NetInterface *interface, ArpPacket *arpResponse);

error_t arpSendProbe(NetInterface *interface, Ipv4Addr targetIpAddr);

error_t arpSendRequest(NetInterface *interface, Ipv4Addr targetIpAddr,
   const MacAddr *destMacAddr);

error_t arpSendReply(NetInterface *interface, Ipv4Addr senderIpAddr,
   Ipv4Addr targetIpAddr, const MacAddr *targetMacAddr);

void arpDumpPacket(const ArpPacket *arpPacket);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
