/**
 * @file raw_socket.c
 * @brief TCP/IP raw sockets
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
 * @section Description
 *
 * A raw socket is a type of socket that allows access to the
 * underlying transport provider
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL RAW_SOCKET_TRACE_LEVEL

//Dependencies
#include <string.h>
#include "core/net.h"
#include "core/socket.h"
#include "core/raw_socket.h"
#include "core/ethernet_misc.h"
#include "ipv4/ipv4.h"
#include "ipv4/ipv4_misc.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "mibs/mib2_module.h"
#include "mibs/if_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (RAW_SOCKET_SUPPORT == ENABLED)


/**
 * @brief Process incoming IP packet
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv4 or IPv6 pseudo header
 * @param[in] buffer Multi-part buffer containing the IP packet
 * @param[in] offset Offset to the first byte of the IP packet
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t rawSocketProcessIpPacket(NetInterface *interface,
   IpPseudoHeader *pseudoHeader, const NetBuffer *buffer, size_t offset,
   NetRxAncillary *ancillary)
{
   uint_t i;
   size_t length;
   Socket *socket;
   SocketQueueItem *queueItem;
   NetBuffer *p;

   //Retrieve the length of the raw IP packet
   length = netBufferGetLength(buffer) - offset;

   //Loop through opened sockets
   for(i = 0; i < SOCKET_MAX_COUNT; i++)
   {
      //Point to the current socket
      socket = socketTable + i;

      //Raw socket found?
      if(socket->type != SOCKET_TYPE_RAW_IP)
         continue;
      //Check whether the socket is bound to a particular interface
      if(socket->interface && socket->interface != interface)
         continue;

#if (IPV4_SUPPORT == ENABLED)
      //IPv4 packet received?
      if(pseudoHeader->length == sizeof(Ipv4PseudoHeader))
      {
         //Check protocol field
         if(socket->protocol != pseudoHeader->ipv4Data.protocol)
            continue;

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
         //Check protocol field
         if(socket->protocol != pseudoHeader->ipv6Data.nextHeader)
            continue;

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

   //Drop incoming packet if no matching socket was found
   if(i >= SOCKET_MAX_COUNT)
      return ERROR_PROTOCOL_UNREACHABLE;

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
      if(i >= RAW_SOCKET_RX_QUEUE_SIZE)
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
   //Port number is unused
   queueItem->srcPort = 0;

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

   //Offset to the raw IP packet
   queueItem->offset = sizeof(SocketQueueItem);
   //Copy the raw data
   netBufferCopy(queueItem->buffer, queueItem->offset, buffer, offset, length);

   //Additional options can be passed to the stack along with the packet
   queueItem->ancillary = *ancillary;

   //Notify user that data is available
   rawSocketUpdateEvents(socket);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process incoming Ethernet packet
 * @param[in] interface Underlying network interface
 * @param[in] header Pointer to the Ethernet header
 * @param[in] data Pointer to the payload data
 * @param[in] length Length of the payload data, in bytes
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 **/

void rawSocketProcessEthPacket(NetInterface *interface, EthHeader *header,
   const uint8_t *data, size_t length, NetRxAncillary *ancillary)
{
   uint_t i;
   Socket *socket;
   SocketQueueItem *queueItem;
   NetBuffer *p;

   //Loop through opened sockets
   for(i = 0; i < SOCKET_MAX_COUNT; i++)
   {
      //Point to the current socket
      socket = socketTable + i;

      //Raw socket found?
      if(socket->type != SOCKET_TYPE_RAW_ETH)
         continue;
      //Check whether the socket is bound to a particular interface
      if(socket->interface && socket->interface != interface)
         continue;

      //Check protocol field
      if(socket->protocol == SOCKET_ETH_PROTO_ALL)
      {
         //Accept all EtherType values
      }
      else if(socket->protocol == SOCKET_ETH_PROTO_LLC)
      {
         //Only accept LLC frames
         if(ntohs(header->type) > ETH_MTU)
            continue;
      }
      else
      {
         //Only accept frames with the correct EtherType value
         if(ntohs(header->type) != socket->protocol)
            continue;
      }

      //The current socket meets all the criteria
      break;
   }

   //Drop incoming packet if no matching socket was found
   if(i >= SOCKET_MAX_COUNT)
      return;

   //Empty receive queue?
   if(socket->receiveQueue == NULL)
   {
      //Allocate a memory buffer to hold the data and the associated descriptor
      p = netBufferAlloc(sizeof(SocketQueueItem) + sizeof(EthHeader) + length);

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
      if(i >= RAW_SOCKET_RX_QUEUE_SIZE)
      {
         //Number of inbound packets which were chosen to be discarded even
         //though no errors had been detected
         MIB2_INC_COUNTER32(ifGroup.ifTable[interface->index].ifInDiscards, 1);
         IF_MIB_INC_COUNTER32(ifTable[interface->index].ifInDiscards, 1);

         //Exit immediately
         return;
      }

      //Allocate a memory buffer to hold the data and the associated descriptor
      p = netBufferAlloc(sizeof(SocketQueueItem) + sizeof(EthHeader) + length);

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

      //Exit immediately
      return;
   }

   //Initialize next field
   queueItem->next = NULL;
   //Network interface where the packet was received
   queueItem->interface = interface;

   //Other fields are meaningless
   queueItem->srcPort = 0;
   queueItem->srcIpAddr = IP_ADDR_ANY;
   queueItem->destIpAddr = IP_ADDR_ANY;

   //Offset to the raw datagram
   queueItem->offset = sizeof(SocketQueueItem);

   //Copy the Ethernet header
   netBufferWrite(queueItem->buffer, queueItem->offset, header,
      sizeof(EthHeader));

   //Copy the payload
   netBufferWrite(queueItem->buffer, queueItem->offset + sizeof(EthHeader),
      data, length);

   //Additional options can be passed to the stack along with the packet
   queueItem->ancillary = *ancillary;

   //Notify user that data is available
   rawSocketUpdateEvents(socket);
}


/**
 * @brief Send an raw IP packet
 * @param[in] socket Handle referencing the socket
 * @param[in] message Pointer to the structure describing the raw packet
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t rawSocketSendIpPacket(Socket *socket, const SocketMsg *message,
   uint_t flags)
{
   error_t error;
   size_t offset;
   NetBuffer *buffer;
   NetInterface *interface;
   IpPseudoHeader pseudoHeader;
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

   //Allocate a buffer memory to hold the raw IP datagram
   buffer = ipAllocBuffer(0, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Start of exception handling block
   do
   {
      //Copy the raw data
      error = netBufferAppend(buffer, message->data, message->length);
      //Any error to report?
      if(error)
         break;

#if (IPV4_SUPPORT == ENABLED)
      //Destination address is an IPv4 address?
      if(message->destIpAddr.length == sizeof(Ipv4Addr))
      {
         Ipv4Addr srcIpAddr;

         //Select the source IPv4 address and the relevant network interface
         //to use when sending data to the specified destination host
         error = ipv4SelectSourceAddr(&interface, message->destIpAddr.ipv4Addr,
            &srcIpAddr);
         //Any error to report?
         if(error)
            break;

         //Format IPv4 pseudo header
         pseudoHeader.length = sizeof(Ipv4PseudoHeader);
         pseudoHeader.ipv4Data.srcAddr = srcIpAddr;
         pseudoHeader.ipv4Data.destAddr = message->destIpAddr.ipv4Addr;
         pseudoHeader.ipv4Data.reserved = 0;
         pseudoHeader.ipv4Data.protocol = socket->protocol;
         pseudoHeader.ipv4Data.length = htons(message->length);
      }
      else
#endif
#if (IPV6_SUPPORT == ENABLED)
      //Destination address is an IPv6 address?
      if(message->destIpAddr.length == sizeof(Ipv6Addr))
      {
         //Select the source IPv6 address and the relevant network interface
         //to use when sending data to the specified destination host
         error = ipv6SelectSourceAddr(&interface, &message->destIpAddr.ipv6Addr,
            &pseudoHeader.ipv6Data.srcAddr);
         //Any error to report?
         if(error)
            break;

         //Format IPv6 pseudo header
         pseudoHeader.length = sizeof(Ipv6PseudoHeader);
         pseudoHeader.ipv6Data.destAddr = message->destIpAddr.ipv6Addr;
         pseudoHeader.ipv6Data.length = htonl(message->length);
         pseudoHeader.ipv6Data.reserved[0] = 0;
         pseudoHeader.ipv6Data.reserved[1] = 0;
         pseudoHeader.ipv6Data.reserved[2] = 0;
         pseudoHeader.ipv6Data.nextHeader = socket->protocol;
      }
      else
#endif
      //Invalid destination address?
      {
         //An internal error has occurred
         error = ERROR_FAILURE;
         //Exit immediately
         break;
      }

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

      //Send raw IP datagram
      error = ipSendDatagram(interface, &pseudoHeader, buffer, offset,
         &ancillary);
      //Failed to send data?
      if(error)
         break;

      //End of exception handling block
   } while(0);

   //Free previously allocated memory block
   netBufferFree(buffer);

   //Return status code
   return error;
}


/**
 * @brief Send an raw Ethernet packet
 * @param[in] socket Handle referencing the socket
 * @param[in] message Pointer to the structure describing the raw packet
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t rawSocketSendEthPacket(Socket *socket, const SocketMsg *message,
   uint_t flags)
{
   error_t error;

#if (ETH_SUPPORT == ENABLED)
   size_t length;
   NetBuffer *buffer;
   NetInterface *interface;

   //Select the relevant network interface
   if(message->interface != NULL)
   {
      interface = message->interface;
   }
   else if(socket->interface != NULL)
   {
      interface = socket->interface;
   }
   else
   {
      interface = netGetDefaultInterface();
   }

   //Forward the frame to the physical interface
   interface = nicGetPhysicalInterface(interface);

   //Ethernet interface?
   if(interface->nicDriver != NULL &&
      interface->nicDriver->type == NIC_TYPE_ETHERNET)
   {
      //Allocate a buffer memory to hold the raw Ethernet packet
      buffer = netBufferAlloc(0);
      //Failed to allocate buffer?
      if(buffer == NULL)
         return ERROR_OUT_OF_MEMORY;

      //Get the length of the raw data
      length = message->length;

      //Copy the raw data
      error = netBufferAppend(buffer, message->data, length);

      //Check status code
      if(!error)
      {
         //Automatic padding not supported by hardware?
         if(!interface->nicDriver->autoPadding)
         {
            //The host controller should manually add padding
            //to the packet before transmitting it
            if(length < (ETH_MIN_FRAME_SIZE - ETH_CRC_SIZE))
            {
               size_t n;

               //Add padding as necessary
               n = (ETH_MIN_FRAME_SIZE - ETH_CRC_SIZE) - length;

               //Append padding bytes
               error = netBufferAppend(buffer, ethPadding, n);

               //Adjust frame length
               length += n;
            }
         }
      }

      //Check status code
      if(!error)
      {
         //CRC calculation not supported by hardware?
         if(!interface->nicDriver->autoCrcCalc)
         {
            uint32_t crc;

            //Compute CRC over the header and payload
            crc = ethCalcCrcEx(buffer, 0, length);
            //Convert from host byte order to little-endian byte order
            crc = htole32(crc);

            //Append the calculated CRC value
            error = netBufferAppend(buffer, &crc, sizeof(crc));

            //Adjust frame length
            length += sizeof(crc);
         }
      }

      //Check status code
      if(!error)
      {
         NetTxAncillary ancillary;

         //Additional options can be passed to the stack along with the packet
         ancillary = NET_DEFAULT_TX_ANCILLARY;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
         //Set switch port identifier
         ancillary.port = message->switchPort;
#endif

#if (ETH_TIMESTAMP_SUPPORT == ENABLED)
         //Unique identifier for hardware time stamping
         ancillary.timestampId = message->timestampId;
#endif
         //Debug message
         TRACE_DEBUG("Sending raw Ethernet frame (%" PRIuSIZE " bytes)...\r\n", length);

         //Send the resulting packet over the specified link
         error = nicSendPacket(interface, buffer, 0, &ancillary);
      }

      //Free previously allocated memory block
      netBufferFree(buffer);
   }
   else
#endif
   //Unknown interface type?
   {
      //Report an error
      error = ERROR_INVALID_INTERFACE;
   }

   //Return status code
   return error;
}


/**
 * @brief Receive an IP packet from a raw socket
 * @param[in] socket Handle referencing the socket
 * @param[out] message Received IP packet and ancillary data
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t rawSocketReceiveIpPacket(Socket *socket, SocketMsg *message,
   uint_t flags)
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

   //Any packet received?
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
      rawSocketUpdateEvents(socket);

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
 * @brief Receive an Ethernet packet from a raw socket
 * @param[in] socket Handle referencing the socket
 * @param[out] message Received Ethernet packet and ancillary data
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t rawSocketReceiveEthPacket(Socket *socket, SocketMsg *message,
   uint_t flags)
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

   //Any packet received?
   if(socket->receiveQueue != NULL)
   {
      //Point to the first item in the receive queue
      queueItem = socket->receiveQueue;

      //Copy data to user buffer
      message->length = netBufferRead(message->data, queueItem->buffer,
         queueItem->offset, message->size);

      //Network interface where the packet was received
      message->interface = queueItem->interface;

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
      rawSocketUpdateEvents(socket);

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
 * @brief Update event state for raw sockets
 * @param[in] socket Handle referencing the socket
 **/

void rawSocketUpdateEvents(Socket *socket)
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

#endif
