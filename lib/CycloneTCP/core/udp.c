/**
 * @file udp.c
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

//Switch to the appropriate trace level
#define TRACE_LEVEL UDP_TRACE_LEVEL

//Dependencies
#include <string.h>
#include "core/net.h"
#include "core/ip.h"
#include "core/udp.h"
#include "core/socket.h"
#include "ipv4/ipv4.h"
#include "ipv4/ipv4_misc.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "mibs/mib2_module.h"
#include "mibs/if_mib_module.h"
#include "mibs/udp_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (UDP_SUPPORT == ENABLED)

//Ephemeral ports are used for dynamic port assignment
static uint16_t udpDynamicPort;
//Mutex to prevent simultaneous access to the callback table
OsMutex udpCallbackMutex;
//Table that holds the registered user callbacks
UdpRxCallbackEntry udpCallbackTable[UDP_CALLBACK_TABLE_SIZE];


/**
 * @brief UDP related initialization
 * @return Error code
 **/

error_t udpInit(void)
{
   //Reset ephemeral port number
   udpDynamicPort = 0;

   //Create a mutex to prevent simultaneous access to the callback table
   if(!osCreateMutex(&udpCallbackMutex))
   {
      //Failed to create mutex
      return ERROR_OUT_OF_RESOURCES;
   }

   //Initialize callback table
   osMemset(udpCallbackTable, 0, sizeof(udpCallbackTable));

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Get an ephemeral port number
 * @return Ephemeral port
 **/

uint16_t udpGetDynamicPort(void)
{
   uint_t port;

   //Retrieve current port number
   port = udpDynamicPort;

   //Invalid port number?
   if(port < SOCKET_EPHEMERAL_PORT_MIN || port > SOCKET_EPHEMERAL_PORT_MAX)
   {
      //Generate a random port number
      port = SOCKET_EPHEMERAL_PORT_MIN + netGetRand() %
         (SOCKET_EPHEMERAL_PORT_MAX - SOCKET_EPHEMERAL_PORT_MIN + 1);
   }

   //Next dynamic port to use
   if(port < SOCKET_EPHEMERAL_PORT_MAX)
   {
      //Increment port number
      udpDynamicPort = port + 1;
   }
   else
   {
      //Wrap around if necessary
      udpDynamicPort = SOCKET_EPHEMERAL_PORT_MIN;
   }

   //Return an ephemeral port number
   return port;
}


/**
 * @brief Incoming UDP datagram processing
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @param[in] buffer Multi-part buffer containing the incoming UDP datagram
 * @param[in] offset Offset to the first byte of the UDP header
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t udpProcessDatagram(NetInterface *interface, IpPseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, NetRxAncillary *ancillary)
{
   error_t error;
   uint_t i;
   size_t length;
   UdpHeader *header;
   Socket *socket;
   SocketQueueItem *queueItem;
   NetBuffer *p;

   //Retrieve the length of the UDP datagram
   length = netBufferGetLength(buffer) - offset;

   //Ensure the UDP header is valid
   if(length < sizeof(UdpHeader))
   {
      //Number of received UDP datagrams that could not be delivered for
      //reasons other than the lack of an application at the destination port
      MIB2_INC_COUNTER32(udpGroup.udpInErrors, 1);
      UDP_MIB_INC_COUNTER32(udpInErrors, 1);

      //Report an error
      return ERROR_INVALID_HEADER;
   }

   //Point to the UDP header
   header = netBufferAt(buffer, offset);
   //Sanity check
   if(header == NULL)
      return ERROR_FAILURE;

   //Debug message
   TRACE_INFO("UDP datagram received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump UDP header contents for debugging purpose
   udpDumpHeader(header);

   //When UDP runs over IPv6, the checksum is mandatory
   if(header->checksum != 0x0000 || pseudoHeader->length == sizeof(Ipv6PseudoHeader))
   {
      //Verify UDP checksum
      if(ipCalcUpperLayerChecksumEx(pseudoHeader->data,
         pseudoHeader->length, buffer, offset, length) != 0x0000)
      {
         //Debug message
         TRACE_WARNING("Wrong UDP header checksum!\r\n");

         //Number of received UDP datagrams that could not be delivered for
         //reasons other than the lack of an application at the destination port
         MIB2_INC_COUNTER32(udpGroup.udpInErrors, 1);
         UDP_MIB_INC_COUNTER32(udpInErrors, 1);

         //Report an error
         return ERROR_WRONG_CHECKSUM;
      }
   }

   //Loop through opened sockets
   for(i = 0; i < SOCKET_MAX_COUNT; i++)
   {
      //Point to the current socket
      socket = socketTable + i;

      //UDP socket found?
      if(socket->type != SOCKET_TYPE_DGRAM)
         continue;
      //Check whether the socket is bound to a particular interface
      if(socket->interface && socket->interface != interface)
         continue;
      //Check destination port number
      if(socket->localPort == 0 || socket->localPort != ntohs(header->destPort))
         continue;
      //Source port number filtering
      if(socket->remotePort != 0 && socket->remotePort != ntohs(header->srcPort))
         continue;

#if (IPV4_SUPPORT == ENABLED)
      //IPv4 packet received?
      if(pseudoHeader->length == sizeof(Ipv4PseudoHeader))
      {
         //Destination IP address filtering
         if(socket->localIpAddr.length != 0)
         {
            //An IPv4 address is expected
            if(socket->localIpAddr.length != sizeof(Ipv4Addr))
               continue;
            //Filter out non-matching addresses
            if(socket->localIpAddr.ipv4Addr != pseudoHeader->ipv4Data.destAddr)
               continue;
         }

         //Source IP address filtering
         if(socket->remoteIpAddr.length != 0)
         {
            //An IPv4 address is expected
            if(socket->remoteIpAddr.length != sizeof(Ipv4Addr))
               continue;
            //Filter out non-matching addresses
            if(socket->remoteIpAddr.ipv4Addr != pseudoHeader->ipv4Data.srcAddr)
               continue;
         }
      }
      else
#endif
#if (IPV6_SUPPORT == ENABLED)
      //IPv6 packet received?
      if(pseudoHeader->length == sizeof(Ipv6PseudoHeader))
      {
         //Destination IP address filtering
         if(socket->localIpAddr.length != 0)
         {
            //An IPv6 address is expected
            if(socket->localIpAddr.length != sizeof(Ipv6Addr))
               continue;
            //Filter out non-matching addresses
            if(!ipv6CompAddr(&socket->localIpAddr.ipv6Addr, &pseudoHeader->ipv6Data.destAddr))
               continue;
         }

         //Source IP address filtering
         if(socket->remoteIpAddr.length != 0)
         {
            //An IPv6 address is expected
            if(socket->remoteIpAddr.length != sizeof(Ipv6Addr))
               continue;
            //Filter out non-matching addresses
            if(!ipv6CompAddr(&socket->remoteIpAddr.ipv6Addr, &pseudoHeader->ipv6Data.srcAddr))
               continue;
         }
      }
      else
#endif
      //Invalid packet received?
      {
         //This should never occur...
         continue;
      }

      //The current socket meets all the criteria
      break;
   }

   //Point to the payload
   offset += sizeof(UdpHeader);
   length -= sizeof(UdpHeader);

   //No matching socket found?
   if(i >= SOCKET_MAX_COUNT)
   {
      //Invoke user callback, if any
      error = udpInvokeRxCallback(interface, pseudoHeader, header, buffer,
         offset, ancillary);
      //Return status code
      return error;
   }

   //Empty receive queue?
   if(socket->receiveQueue == NULL)
   {
      //Allocate a memory buffer to hold the data and the associated descriptor
      p = netBufferAlloc(sizeof(SocketQueueItem) + length);

      //Successful memory allocation?
      if(p != NULL)
      {
         //Point to the newly created item
         queueItem = netBufferAt(p, 0);
         queueItem->buffer = p;
         //Add the newly created item to the queue
         socket->receiveQueue = queueItem;
      }
      else
      {
         //Memory allocation failed
         queueItem = NULL;
      }
   }
   else
   {
      //Point to the very first item
      queueItem = socket->receiveQueue;

      //Reach the last item in the receive queue
      for(i = 1; queueItem->next; i++)
      {
         queueItem = queueItem->next;
      }

      //Check whether the receive queue is full
      if(i >= UDP_RX_QUEUE_SIZE)
      {
         //Number of inbound packets which were chosen to be discarded even
         //though no errors had been detected
         MIB2_INC_COUNTER32(ifGroup.ifTable[interface->index].ifInDiscards, 1);
         IF_MIB_INC_COUNTER32(ifTable[interface->index].ifInDiscards, 1);

         //Report an error
         return ERROR_RECEIVE_QUEUE_FULL;
      }

      //Allocate a memory buffer to hold the data and the associated descriptor
      p = netBufferAlloc(sizeof(SocketQueueItem) + length);

      //Successful memory allocation?
      if(p != NULL)
      {
         //Add the newly created item to the queue
         queueItem->next = netBufferAt(p, 0);
         //Point to the newly created item
         queueItem = queueItem->next;
         queueItem->buffer = p;
      }
      else
      {
         //Memory allocation failed
         queueItem = NULL;
      }
   }

   //Not enough resources to properly handle the packet?
   if(queueItem == NULL)
   {
      //Number of inbound packets which were chosen to be discarded even
      //though no errors had been detected
      MIB2_INC_COUNTER32(ifGroup.ifTable[interface->index].ifInDiscards, 1);
      IF_MIB_INC_COUNTER32(ifTable[interface->index].ifInDiscards, 1);

      //Report an error
      return ERROR_OUT_OF_MEMORY;
   }

   //Initialize next field
   queueItem->next = NULL;
   //Network interface where the packet was received
   queueItem->interface = interface;
   //Record the source port number
   queueItem->srcPort = ntohs(header->srcPort);

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 remote address?
   if(pseudoHeader->length == sizeof(Ipv4PseudoHeader))
   {
      //Save the source IPv4 address
      queueItem->srcIpAddr.length = sizeof(Ipv4Addr);
      queueItem->srcIpAddr.ipv4Addr = pseudoHeader->ipv4Data.srcAddr;

      //Save the destination IPv4 address
      queueItem->destIpAddr.length = sizeof(Ipv4Addr);
      queueItem->destIpAddr.ipv4Addr = pseudoHeader->ipv4Data.destAddr;
   }
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 remote address?
   if(pseudoHeader->length == sizeof(Ipv6PseudoHeader))
   {
      //Save the source IPv6 address
      queueItem->srcIpAddr.length = sizeof(Ipv6Addr);
      queueItem->srcIpAddr.ipv6Addr = pseudoHeader->ipv6Data.srcAddr;

      //Save the destination IPv6 address
      queueItem->destIpAddr.length = sizeof(Ipv6Addr);
      queueItem->destIpAddr.ipv6Addr = pseudoHeader->ipv6Data.destAddr;
   }
#endif

   //Offset to the payload
   queueItem->offset = sizeof(SocketQueueItem);
   //Copy the payload
   netBufferCopy(queueItem->buffer, queueItem->offset, buffer, offset, length);

   //Additional options can be passed to the stack along with the packet
   queueItem->ancillary = *ancillary;

   //Notify user that data is available
   udpUpdateEvents(socket);

   //Total number of UDP datagrams delivered to UDP users
   MIB2_INC_COUNTER32(udpGroup.udpInDatagrams, 1);
   UDP_MIB_INC_COUNTER32(udpInDatagrams, 1);
   UDP_MIB_INC_COUNTER64(udpHCInDatagrams, 1);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Send a UDP datagram
 * @param[in] socket Handle referencing the socket
 * @param[in] message Pointer to the structure describing the datagram
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t udpSendDatagram(Socket *socket, const SocketMsg *message, uint_t flags)
{
   error_t error;
   size_t offset;
   NetBuffer *buffer;
   NetInterface *interface;
   NetTxAncillary ancillary;

   //Select the relevant network interface
   if(message->interface != NULL)
   {
      interface = message->interface;
   }
   else
   {
      interface = socket->interface;
   }

   //Allocate a memory buffer to hold the UDP datagram
   buffer = udpAllocBuffer(0, &offset);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Copy data payload
   error = netBufferAppend(buffer, message->data, message->length);

   //Successful processing?
   if(!error)
   {
      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_TX_ANCILLARY;

      //Set the TTL value to be used
      if(message->ttl != 0)
      {
         ancillary.ttl = message->ttl;
      }
      else if(ipIsMulticastAddr(&message->destIpAddr))
      {
         ancillary.ttl = socket->multicastTtl;
      }
      else
      {
         ancillary.ttl = socket->ttl;
      }

      //This flag tells the stack that the destination is on a locally attached
      //network and not to perform a lookup of the routing table
      if((flags & SOCKET_FLAG_DONT_ROUTE) != 0)
      {
         ancillary.dontRoute = TRUE;
      }

#if (IP_DIFF_SERV_SUPPORT == ENABLED)
      //Set DSCP field
      ancillary.dscp = socket->dscp;
#endif

#if (ETH_SUPPORT == ENABLED)
      //Set source and destination MAC addresses
      ancillary.srcMacAddr = message->srcMacAddr;
      ancillary.destMacAddr = message->destMacAddr;
#endif

#if (ETH_VLAN_SUPPORT == ENABLED)
      //Set VLAN PCP and DEI fields
      ancillary.vlanPcp = socket->vlanPcp;
      ancillary.vlanDei = socket->vlanDei;
#endif

#if (ETH_VMAN_SUPPORT == ENABLED)
      //Set VMAN PCP and DEI fields
      ancillary.vmanPcp = socket->vmanPcp;
      ancillary.vmanDei = socket->vmanDei;
#endif

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
      //Set switch port identifier
      ancillary.port = message->switchPort;
#endif

#if (ETH_TIMESTAMP_SUPPORT == ENABLED)
      //Unique identifier for hardware time stamping
      ancillary.timestampId = message->timestampId;
#endif

      //Send UDP datagram
      error = udpSendBuffer(interface, &message->srcIpAddr, socket->localPort,
         &message->destIpAddr, message->destPort, buffer, offset, &ancillary);
   }

   //Free previously allocated memory
   netBufferFree(buffer);

   //Return status code
   return error;
}


/**
 * @brief Send a UDP datagram
 * @param[in] interface Underlying network interface
 * @param[in] srcIpAddr Source IP address (optional parameter)
 * @param[in] srcPort Source port
 * @param[in] destIpAddr IP address of the target host
 * @param[in] destPort Target port number
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in] offset Offset to the first payload byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t udpSendBuffer(NetInterface *interface, const IpAddr *srcIpAddr,
   uint16_t srcPort, const IpAddr *destIpAddr, uint16_t destPort,
   NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   error_t error;
   size_t length;
   UdpHeader *header;
   IpPseudoHeader pseudoHeader;

   //Make room for the UDP header
   offset -= sizeof(UdpHeader);
   //Retrieve the length of the datagram
   length = netBufferGetLength(buffer) - offset;

   //Point to the UDP header
   header = netBufferAt(buffer, offset);
   //Sanity check
   if(header == NULL)
      return ERROR_FAILURE;

   //Format UDP header
   header->srcPort = htons(srcPort);
   header->destPort = htons(destPort);
   header->length = htons(length);
   header->checksum = 0;

#if (IPV4_SUPPORT == ENABLED)
   //Destination address is an IPv4 address?
   if(destIpAddr->length == sizeof(Ipv4Addr))
   {
      //Valid source IP address?
      if(srcIpAddr != NULL && srcIpAddr->length == sizeof(Ipv4Addr))
      {
         //Copy the source IP address
         pseudoHeader.ipv4Data.srcAddr = srcIpAddr->ipv4Addr;
      }
      else
      {
         Ipv4Addr ipAddr;

         //Select the source IPv4 address and the relevant network interface
         //to use when sending data to the specified destination host
         error = ipv4SelectSourceAddr(&interface, destIpAddr->ipv4Addr,
            &ipAddr);

         //Check status code
         if(!error)
         {
            //Copy the resulting source IP address
            pseudoHeader.ipv4Data.srcAddr = ipAddr;
         }
         else
         {
            //Handle the special case where the destination address is the
            //broadcast address
            if(destIpAddr->ipv4Addr == IPV4_BROADCAST_ADDR && interface != NULL)
            {
               //Use the unspecified address as source address
               pseudoHeader.ipv4Data.srcAddr = IPV4_UNSPECIFIED_ADDR;
            }
            else
            {
               //Source address selection failed
               return error;
            }
         }
      }

      //Format IPv4 pseudo header
      pseudoHeader.length = sizeof(Ipv4PseudoHeader);
      pseudoHeader.ipv4Data.destAddr = destIpAddr->ipv4Addr;
      pseudoHeader.ipv4Data.reserved = 0;
      pseudoHeader.ipv4Data.protocol = IPV4_PROTOCOL_UDP;
      pseudoHeader.ipv4Data.length = htons(length);

      //Calculate UDP header checksum
      header->checksum = ipCalcUpperLayerChecksumEx(&pseudoHeader.ipv4Data,
         sizeof(Ipv4PseudoHeader), buffer, offset, length);
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //Destination address is an IPv6 address?
   if(destIpAddr->length == sizeof(Ipv6Addr))
   {
      //Valid source IP address?
      if(srcIpAddr != NULL && srcIpAddr->length == sizeof(Ipv6Addr))
      {
         //Copy the source IP address
         pseudoHeader.ipv6Data.srcAddr = srcIpAddr->ipv6Addr;
      }
      else
      {
         //Select the source IPv6 address and the relevant network interface
         //to use when sending data to the specified destination host
         error = ipv6SelectSourceAddr(&interface, &destIpAddr->ipv6Addr,
            &pseudoHeader.ipv6Data.srcAddr);
         //Any error to report?
         if(error)
            return error;
      }

      //Format IPv6 pseudo header
      pseudoHeader.length = sizeof(Ipv6PseudoHeader);
      pseudoHeader.ipv6Data.destAddr = destIpAddr->ipv6Addr;
      pseudoHeader.ipv6Data.length = htonl(length);
      pseudoHeader.ipv6Data.reserved[0] = 0;
      pseudoHeader.ipv6Data.reserved[1] = 0;
      pseudoHeader.ipv6Data.reserved[2] = 0;
      pseudoHeader.ipv6Data.nextHeader = IPV6_UDP_HEADER;

      //Calculate UDP header checksum
      header->checksum = ipCalcUpperLayerChecksumEx(&pseudoHeader.ipv6Data,
         sizeof(Ipv6PseudoHeader), buffer, offset, length);
   }
   else
#endif
   //Invalid destination address?
   {
      //An internal error has occurred
      return ERROR_FAILURE;
   }

   //If the computed checksum is zero, it is transmitted as all ones. An all
   //zero transmitted checksum value means that the transmitter generated no
   //checksum
   if(header->checksum == 0x0000)
   {
      header->checksum = 0xFFFF;
   }

   //Total number of UDP datagrams sent from this entity
   MIB2_INC_COUNTER32(udpGroup.udpOutDatagrams, 1);
   UDP_MIB_INC_COUNTER32(udpOutDatagrams, 1);
   UDP_MIB_INC_COUNTER64(udpHCOutDatagrams, 1);

   //Debug message
   TRACE_INFO("Sending UDP datagram (%" PRIuSIZE " bytes)\r\n", length);
   //Dump UDP header contents for debugging purpose
   udpDumpHeader(header);

   //Send UDP datagram
   error = ipSendDatagram(interface, &pseudoHeader, buffer, offset, ancillary);

   //Return status code
   return error;
}


/**
 * @brief Receive data from a UDP socket
 * @param[in] socket Handle referencing the socket
 * @param[out] message Received UDP datagram and ancillary data
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t udpReceiveDatagram(Socket *socket, SocketMsg *message, uint_t flags)
{
   error_t error;
   SocketQueueItem *queueItem;

   //The SOCKET_FLAG_DONT_WAIT enables non-blocking operation
   if((flags & SOCKET_FLAG_DONT_WAIT) == 0)
   {
      //Check whether the receive queue is empty
      if(socket->receiveQueue == NULL)
      {
         //Set the events the application is interested in
         socket->eventMask = SOCKET_EVENT_RX_READY;

         //Reset the event object
         osResetEvent(&socket->event);

         //Release exclusive access
         osReleaseMutex(&netMutex);
         //Wait until an event is triggered
         osWaitForEvent(&socket->event, socket->timeout);
         //Get exclusive access
         osAcquireMutex(&netMutex);
      }
   }

   //Any datagram received?
   if(socket->receiveQueue != NULL)
   {
      //Point to the first item in the receive queue
      queueItem = socket->receiveQueue;

      //Copy data to user buffer
      message->length = netBufferRead(message->data, queueItem->buffer,
         queueItem->offset, message->size);

      //Network interface where the packet was received
      message->interface = queueItem->interface;
      //Save the source IP address
      message->srcIpAddr = queueItem->srcIpAddr;
      //Save the source port number
      message->srcPort = queueItem->srcPort;
      //Save the destination IP address
      message->destIpAddr = queueItem->destIpAddr;

      //Save TTL value
      message->ttl = queueItem->ancillary.ttl;

#if (ETH_SUPPORT == ENABLED)
      //Save source and destination MAC addresses
      message->srcMacAddr = queueItem->ancillary.srcMacAddr;
      message->destMacAddr = queueItem->ancillary.destMacAddr;
#endif

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
      //Save switch port identifier
      message->switchPort = queueItem->ancillary.port;
#endif

#if (ETH_TIMESTAMP_SUPPORT == ENABLED)
      //Save captured time stamp
      message->timestamp = queueItem->ancillary.timestamp;
#endif

      //If the SOCKET_FLAG_PEEK flag is set, the data is copied into the
      //buffer but is not removed from the input queue
      if((flags & SOCKET_FLAG_PEEK) == 0)
      {
         //Remove the item from the receive queue
         socket->receiveQueue = queueItem->next;

         //Deallocate memory buffer
         netBufferFree(queueItem->buffer);
      }

      //Update the state of events
      udpUpdateEvents(socket);

      //Successful read operation
      error = NO_ERROR;
   }
   else
   {
      //Total number of data that have been received
      message->length = 0;

      //Report a timeout error
      error = ERROR_TIMEOUT;
   }

   //Return status code
   return error;
}


/**
 * @brief Allocate a buffer to hold a UDP packet
 * @param[in] length Desired payload length
 * @param[out] offset Offset to the first byte of the payload
 * @return The function returns a pointer to the newly allocated
 *   buffer. If the system is out of resources, NULL is returned
 **/

NetBuffer *udpAllocBuffer(size_t length, size_t *offset)
{
   NetBuffer *buffer;

   //Allocate a buffer to hold the UDP header and the payload
   buffer = ipAllocBuffer(length + sizeof(UdpHeader), offset);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return NULL;

   //Offset to the first byte of the payload
   *offset += sizeof(UdpHeader);

   //Return a pointer to the freshly allocated buffer
   return buffer;
}


/**
 * @brief Update UDP related events
 * @param[in] socket Handle referencing the socket
 **/

void udpUpdateEvents(Socket *socket)
{
   //Clear event flags
   socket->eventFlags = 0;

   //The socket is marked as readable if a datagram is pending in the queue
   if(socket->receiveQueue)
      socket->eventFlags |= SOCKET_EVENT_RX_READY;

   //Check whether the socket is bound to a particular network interface
   if(socket->interface != NULL)
   {
      //Handle link up and link down events
      if(socket->interface->linkState)
         socket->eventFlags |= SOCKET_EVENT_LINK_UP;
      else
         socket->eventFlags |= SOCKET_EVENT_LINK_DOWN;
   }

   //Mask unused events
   socket->eventFlags &= socket->eventMask;

   //Any event to signal?
   if(socket->eventFlags)
   {
      //Unblock I/O operations currently in waiting state
      osSetEvent(&socket->event);

      //Set user event to signaled state if necessary
      if(socket->userEvent != NULL)
      {
         osSetEvent(socket->userEvent);
      }
   }
}


/**
 * @brief Register user callback
 * @param[in] interface Underlying network interface
 * @param[in] port UDP port number
 * @param[in] callback Callback function to be called when a datagram is received
 * @param[in] param Callback function parameter (optional)
 * @return Error code
 **/

error_t udpAttachRxCallback(NetInterface *interface, uint16_t port,
   UdpRxCallback callback, void *param)
{
   uint_t i;
   UdpRxCallbackEntry *entry;

   //Acquire exclusive access to the callback table
   osAcquireMutex(&udpCallbackMutex);

   //Loop through the table
   for(i = 0; i < UDP_CALLBACK_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &udpCallbackTable[i];

      //Check whether the entry is currently in use
      if(entry->callback == NULL)
      {
         //Create a new entry
         entry->interface = interface;
         entry->port = port;
         entry->callback = callback;
         entry->param = param;
         //We are done
         break;
      }
   }

   //Release exclusive access to the callback table
   osReleaseMutex(&udpCallbackMutex);

   //Failed to attach the specified user callback?
   if(i >= UDP_CALLBACK_TABLE_SIZE)
      return ERROR_OUT_OF_RESOURCES;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Unregister user callback
 * @param[in] interface Underlying network interface
 * @param[in] port UDP port number
 * @return Error code
 **/

error_t udpDetachRxCallback(NetInterface *interface, uint16_t port)
{
   error_t error;
   uint_t i;
   UdpRxCallbackEntry *entry;

   //Initialize status code
   error = ERROR_FAILURE;

   //Acquire exclusive access to the callback table
   osAcquireMutex(&udpCallbackMutex);

   //Loop through the table
   for(i = 0; i < UDP_CALLBACK_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &udpCallbackTable[i];

      //Check whether the entry is currently in use
      if(entry->callback != NULL)
      {
         //Does the specified port number match the current entry?
         if(entry->port == port && entry->interface == interface)
         {
            //Unregister user callback
            entry->callback = NULL;
            //A matching entry has been found
            error = NO_ERROR;
         }
      }
   }

   //Release exclusive access to the callback table
   osReleaseMutex(&udpCallbackMutex);

   //Return status code
   return error;
}


/**
 * @brief Invoke user callback
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @param[in] header UDP header
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in] offset Offset to the first byte of the payload
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t udpInvokeRxCallback(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *header,
   const NetBuffer *buffer, size_t offset, NetRxAncillary *ancillary)
{
   error_t error;
   uint_t i;
   void *param;
   UdpRxCallbackEntry *entry;

   //Initialize status code
   error = ERROR_PORT_UNREACHABLE;

   //Acquire exclusive access to the callback table
   osAcquireMutex(&udpCallbackMutex);

   //Loop through the table
   for(i = 0; i < UDP_CALLBACK_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &udpCallbackTable[i];

      //Check whether the entry is currently in use
      if(entry->callback != NULL)
      {
         //Bound to a particular interface?
         if(entry->interface == NULL || entry->interface == interface)
         {
            //Does the specified port number match the current entry?
            if(entry->port == ntohs(header->destPort))
            {
               //Retrieve callback parameter
               param = entry->param;

               //Release mutex to prevent any deadlock
               if(param == NULL)
                  osReleaseMutex(&udpCallbackMutex);

               //Invoke user callback function
               entry->callback(interface, pseudoHeader, header, buffer, offset,
                  ancillary, param);

               //Acquire mutex
               if(param == NULL)
                  osAcquireMutex(&udpCallbackMutex);

               //A matching entry has been found
               error = NO_ERROR;
            }
         }
      }
   }

   //Release exclusive access to the callback table
   osReleaseMutex(&udpCallbackMutex);

   //Check status code
   if(error)
   {
      //Total number of received UDP datagrams for which there was
      //no application at the destination port
      MIB2_INC_COUNTER32(udpGroup.udpNoPorts, 1);
      UDP_MIB_INC_COUNTER32(udpNoPorts, 1);
   }
   else
   {
      //Total number of UDP datagrams delivered to UDP users
      MIB2_INC_COUNTER32(udpGroup.udpInDatagrams, 1);
      UDP_MIB_INC_COUNTER32(udpInDatagrams, 1);
      UDP_MIB_INC_COUNTER64(udpHCInDatagrams, 1);
   }

   //Return status code
   return error;
}


/**
 * @brief Dump UDP header for debugging purpose
 * @param[in] datagram Pointer to the UDP header
 **/

void udpDumpHeader(const UdpHeader *datagram)
{
   //Dump UDP header contents
   TRACE_DEBUG("  Source Port = %" PRIu16 "\r\n", ntohs(datagram->srcPort));
   TRACE_DEBUG("  Destination Port = %" PRIu16 "\r\n", ntohs(datagram->destPort));
   TRACE_DEBUG("  Length = %" PRIu16 "\r\n", ntohs(datagram->length));
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(datagram->checksum));
}

#endif
