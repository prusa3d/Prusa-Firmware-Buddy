/**
 * @file udp.h
 * @brief UDP (User Datagram Protocol)
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

#ifndef _UDP_H
#define _UDP_H

//Dependencies
#include "core/net.h"
#include "core/tcp.h"

//UDP support
#ifndef UDP_SUPPORT
   #define UDP_SUPPORT ENABLED
#elif (UDP_SUPPORT != ENABLED && UDP_SUPPORT != DISABLED)
   #error UDP_SUPPORT parameter is not valid
#endif

//Maximum number of callback functions that can be registered
//to process incoming UDP datagrams
#ifndef UDP_CALLBACK_TABLE_SIZE
   #define UDP_CALLBACK_TABLE_SIZE 10
#elif (UDP_CALLBACK_TABLE_SIZE < 1)
   #error UDP_CALLBACK_TABLE_SIZE parameter is not valid
#endif

//Receive queue depth for connectionless sockets
#ifndef UDP_RX_QUEUE_SIZE
   #define UDP_RX_QUEUE_SIZE 4
#elif (UDP_RX_QUEUE_SIZE < 1)
   #error UDP_RX_QUEUE_SIZE parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief UDP header
 **/

typedef __start_packed struct
{
   uint16_t srcPort;  //0-1
   uint16_t destPort; //2-3
   uint16_t length;   //4-5
   uint16_t checksum; //6-7
   uint8_t data[];    //8
} __end_packed UdpHeader;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


/**
 * @brief UDP receive callback
 **/

typedef void (*UdpRxCallback)(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *header,
   const NetBuffer *buffer, size_t offset, const NetRxAncillary *ancillary,
   void *param);


/**
 * @brief UDP receive callback entry
 **/

typedef struct
{
   NetInterface *interface;
   uint16_t port;
   UdpRxCallback callback;
   void *param;
} UdpRxCallbackEntry;


//Global variables
extern OsMutex udpCallbackMutex;
extern UdpRxCallbackEntry udpCallbackTable[UDP_CALLBACK_TABLE_SIZE];

//UDP related functions
error_t udpInit(void);
uint16_t udpGetDynamicPort(void);

error_t udpProcessDatagram(NetInterface *interface, IpPseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, NetRxAncillary *ancillary);

error_t udpSendDatagram(Socket *socket, const SocketMsg *message, uint_t flags);

error_t udpSendBuffer(NetInterface *interface, const IpAddr *srcIpAddr,
   uint16_t srcPort, const IpAddr *destIpAddr, uint16_t destPort,
   NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t udpReceiveDatagram(Socket *socket, SocketMsg *message, uint_t flags);

NetBuffer *udpAllocBuffer(size_t length, size_t *offset);

void udpUpdateEvents(Socket *socket);

error_t udpAttachRxCallback(NetInterface *interface, uint16_t port,
   UdpRxCallback callback, void *param);

error_t udpDetachRxCallback(NetInterface *interface, uint16_t port);

error_t udpInvokeRxCallback(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *header,
   const NetBuffer *buffer, size_t offset, NetRxAncillary *ancillary);

void udpDumpHeader(const UdpHeader *datagram);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
