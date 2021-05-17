/**
 * @file ipv6_routing.c
 * @brief IPv6 routing
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
#define TRACE_LEVEL IPV6_TRACE_LEVEL

//Dependencies
#include <limits.h>
#include "core/net.h"
#include "core/ip.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "ipv6/ipv6_routing.h"
#include "ipv6/icmpv6.h"
#include "ipv6/ndp.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED && IPV6_ROUTING_SUPPORT == ENABLED)

//IPv6 routing table
static Ipv6RoutingTableEntry ipv6RoutingTable[IPV6_ROUTING_TABLE_SIZE];


/**
 * @brief Initialize IPv6 routing table
 * @return Error code
 **/

error_t ipv6InitRouting(void)
{
   //Clear the routing table
   osMemset(ipv6RoutingTable, 0, sizeof(ipv6RoutingTable));

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Enable routing for the specified interface
 * @param[in] interface Underlying network interface
 * @param[in] enable When the flag is set to TRUE, routing is enabled on the
 *   interface and the router can forward packets to or from the interface
 * @return Error code
 **/

error_t ipv6EnableRouting(NetInterface *interface, bool_t enable)
{
   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Enable or disable routing
   interface->ipv6Context.isRouter = enable;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add a new entry in the IPv6 routing table
 * @param[in] prefix Network destination
 * @param[in] prefixLen Length of the prefix, in bits
 * @param[in] interface Network interface where to forward the packet
 * @param[in] nextHop IPv6 address of the next hop
 * @param[in] metric Metric value
 * @return Error code
 **/

error_t ipv6AddRoute(const Ipv6Addr *prefix, uint_t prefixLen,
   NetInterface *interface, const Ipv6Addr *nextHop, uint_t metric)
{
   error_t error;
   uint_t i;
   Ipv6RoutingTableEntry *entry;
   Ipv6RoutingTableEntry *firstFreeEntry;

   //Check parameters
   if(prefix == NULL || interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Keep track of the first free entry
   firstFreeEntry = NULL;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Loop through routing table entries
   for(i = 0; i < IPV6_ROUTING_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &ipv6RoutingTable[i];

      //Valid entry?
      if(entry->valid)
      {
         //Check prefix length
         if(entry->prefixLen == prefixLen)
         {
            //Check whether the current entry matches the specified destination
            if(ipv6CompPrefix(&entry->prefix, prefix, prefixLen))
               break;
         }
      }
      else
      {
         //Keep track of the first free entry
         if(firstFreeEntry == NULL)
            firstFreeEntry = entry;
      }
   }

   //If the routing table does not contain the specified destination,
   //then a new entry should be created
   if(i >= IPV6_ROUTING_TABLE_SIZE)
      entry = firstFreeEntry;

   //Check whether the routing table runs out of space
   if(entry != NULL)
   {
      //Network destination
      entry->prefix = *prefix;
      entry->prefixLen = prefixLen;

      //Interface where to forward the packet
      entry->interface = interface;

      //Address of the next hop
      if(nextHop != NULL)
         entry->nextHop = *nextHop;
      else
         entry->nextHop = IPV6_UNSPECIFIED_ADDR;

      //Metric value
      entry->metric = metric;
      //The entry is now valid
      entry->valid = TRUE;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The routing table is full
      error = ERROR_FAILURE;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief Remove an entry from the IPv6 routing table
 * @param[in] prefix Network destination
 * @param[in] prefixLen Length of the prefix, in bits
 * @return Error code
 **/

error_t ipv6DeleteRoute(const Ipv6Addr *prefix, uint_t prefixLen)
{
   error_t error;
   uint_t i;
   Ipv6RoutingTableEntry *entry;

   //Initialize status code
   error = ERROR_NOT_FOUND;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Loop through routing table entries
   for(i = 0; i < IPV6_ROUTING_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &ipv6RoutingTable[i];

      //Valid entry?
      if(entry->valid)
      {
         //Check prefix length
         if(entry->prefixLen == prefixLen)
         {
            //Check whether the current entry matches the specified destination
            if(ipv6CompPrefix(&entry->prefix, prefix, prefixLen))
            {
               //Delete current entry
               entry->valid = FALSE;
               //The route was successfully deleted from the routing table
               error = NO_ERROR;
            }
         }
      }
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief Delete all routes from the IPv6 routing table
 * @return Error code
 **/

error_t ipv6DeleteAllRoutes(void)
{
   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Clear the routing table
   osMemset(ipv6RoutingTable, 0, sizeof(ipv6RoutingTable));
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Forward an IPv6 packet
 * @param[in] srcInterface Network interface on which the packet was received
 * @param[in] ipPacket Multi-part buffer that holds the IPv6 packet to forward
 * @param[in] ipPacketOffset Offset to the first byte of the IPv6 packet
 * @return Error code
 **/

error_t ipv6ForwardPacket(NetInterface *srcInterface, NetBuffer *ipPacket,
   size_t ipPacketOffset)
{
   error_t error;
   uint_t i;
   uint_t metric;
   uint_t prefixLen;
   bool_t match;
   size_t length;
   size_t destOffset;
   NetInterface *destInterface;
   NetBuffer *destBuffer;
   Ipv6Header *ipHeader;
   Ipv6RoutingTableEntry *entry;
   Ipv6Addr destIpAddr;
#if (ETH_SUPPORT == ENABLED)
   NetInterface *physicalInterface;
#endif

   //Silently drop any IP packets received on an interface that has
   //not been assigned a valid link-local address
   if(ipv6GetLinkLocalAddrState(srcInterface) != IPV6_ADDR_STATE_PREFERRED)
      return ERROR_NOT_CONFIGURED;

   //If routing is not enabled on the interface, then the router cannot
   //forward packets from the interface
   if(!srcInterface->ipv6Context.isRouter)
      return ERROR_FAILURE;

   //Calculate the length of the IPv6 packet
   length = netBufferGetLength(ipPacket) - ipPacketOffset;

   //Ensure the packet length is greater than 40 bytes
   if(length < sizeof(Ipv6Header))
      return ERROR_INVALID_LENGTH;

   //Point to the IPv6 header
   ipHeader = netBufferAt(ipPacket, ipPacketOffset);

   //Sanity check
   if(ipHeader == NULL)
      return ERROR_FAILURE;

   //An IPv6 packet with a source address of unspecified must never be
   //forwarded by an IPv6 router (refer to RFC section 3513 2.5.2)
   if(ipv6CompAddr(&ipHeader->srcAddr, &IPV6_UNSPECIFIED_ADDR))
      return ERROR_INVALID_ADDRESS;

   //The unspecified address must not be used as the destination address
   //of IPv6 packets (refer to RFC section 3513 2.5.2)
   if(ipv6CompAddr(&ipHeader->destAddr, &IPV6_UNSPECIFIED_ADDR))
      return ERROR_INVALID_ADDRESS;

   //An IPv6 packet with a destination address of loopback must never be
   //forwarded by an IPv6 router (refer to RFC 3513 section 2.5.3)
   if(ipv6CompAddr(&ipHeader->destAddr, &IPV6_LOOPBACK_ADDR))
      return ERROR_INVALID_ADDRESS;

   //Check whether the destination address is a link-local address
   if(ipv6IsLinkLocalUnicastAddr(&ipHeader->destAddr))
   {
      //Forward the packet on the same network interface
      destInterface = srcInterface;
      //Next hop
      destIpAddr = ipHeader->destAddr;
   }
   else
   {
      //Lowest metric value
      metric = UINT_MAX;
      //Longest prefix length
      prefixLen = 0;
      //Outgoing network interface
      destInterface = NULL;

      //Route determination process
      for(i = 0; i < IPV6_ROUTING_TABLE_SIZE; i++)
      {
         //Point to the current entry
         entry = &ipv6RoutingTable[i];

         //Valid entry?
         if(entry->valid && entry->interface != NULL)
         {
            //Clear flag
            match = FALSE;

            //Do not forward any IP packets to an interface that has not
            //been assigned a valid link-local address...
            if(ipv6GetLinkLocalAddrState(entry->interface) == IPV6_ADDR_STATE_PREFERRED)
            {
               //If routing is enabled on the interface, then the router
               //can forward packets to the interface
               if(entry->interface->ipv6Context.isRouter)
               {
                  //Compare the destination address with the current entry for a match
                  if(ipv6CompPrefix(&ipHeader->destAddr, &entry->prefix, entry->prefixLen))
                  {
                     //The longest matching route is the most specific route to the
                     //destination IPv6 address...
                     if(entry->prefixLen > prefixLen)
                     {
                        //Give the current route the higher precedence
                        match = TRUE;
                     }
                     else if(entry->prefixLen == prefixLen)
                     {
                        //If multiple entries with the longest match are found, the
                        //router uses the lowest metric to select the best route
                        if(entry->metric < metric)
                        {
                           //Give the current route the higher precedence
                           match = TRUE;
                        }
                     }
                  }
               }
            }

            //Matching entry?
            if(match)
            {
               //Select the current route
               metric = entry->metric;
               prefixLen = entry->prefixLen;

               //Outgoing interface on which to forward the packet
               destInterface = entry->interface;

               //Next hop
               if(!ipv6CompAddr(&entry->nextHop, &IPV6_UNSPECIFIED_ADDR))
                  destIpAddr = entry->nextHop;
               else
                  destIpAddr = ipHeader->destAddr;
            }
         }
      }
   }

   //No route to the destination?
   if(destInterface == NULL)
   {
      //A Destination Unreachable message should be generated by a router
      //in response to a packet that cannot be delivered
      icmpv6SendErrorMessage(srcInterface, ICMPV6_TYPE_DEST_UNREACHABLE,
         ICMPV6_CODE_NO_ROUTE_TO_DEST, 0, ipPacket, ipPacketOffset);

      //Exit immediately
      return ERROR_NO_ROUTE;
   }

   //Check whether the length of the IPv6 packet is larger than the link MTU
   if(length > destInterface->ipv6Context.linkMtu)
   {
      //A Packet Too Big must be sent by a router in response to a packet
      //that it cannot forward because the packet is larger than the MTU
      //of the outgoing link
      icmpv6SendErrorMessage(srcInterface, ICMPV6_TYPE_PACKET_TOO_BIG,
         0, destInterface->ipv6Context.linkMtu, ipPacket, ipPacketOffset);

      //Exit immediately
      return ERROR_INVALID_LENGTH;
   }

   //Check whether the packet is explicitly addressed to the router itself
   if(!ipv6CheckDestAddr(destInterface, &ipHeader->destAddr))
   {
      //Valid unicast address?
      if(!ipv6IsMulticastAddr(&ipHeader->destAddr))
      {
         //Process IPv6 packet
         //ipv6ProcessPacket(destInterface, ipPacket, ipPacketOffset);
         //Exit immediately
         return NO_ERROR;
      }
   }

   //Check whether the IPv6 packet is about to be sent out the interface
   //on which it was received
   if(destInterface == srcInterface)
   {
#if (NDP_SUPPORT == ENABLED)
      //A router should send a Redirect message whenever it forwards a packet
      //that is not explicitly addressed to itself in which the source address
      //identifies a neighbor, and
      if(ipv6IsOnLink(srcInterface, &ipHeader->srcAddr))
      {
         //The router determines that a better first-hop node resides on the
         //same link as the sending node for the destination address of the
         //packet being forwarded, and
         if(ipv6IsOnLink(destInterface, &destIpAddr))
         {
            //The destination address of the packet is not a multicast address
            if(!ipv6IsMulticastAddr(&ipHeader->destAddr))
            {
               //Transmit a Redirect message
               ndpSendRedirect(srcInterface, &destIpAddr, ipPacket, ipPacketOffset);
            }
         }
      }
#endif
   }
   else
   {
      //Check whether the scope of the source address is smaller than the
      //scope of the destination address
      if(ipv6GetAddrScope(&ipHeader->srcAddr) < ipv6GetAddrScope(&ipHeader->destAddr))
      {
         //A Destination Unreachable message should be generated by a router
         //in response to a packet that cannot be delivered without leaving
         //the scope of the source address
         icmpv6SendErrorMessage(srcInterface, ICMPV6_TYPE_DEST_UNREACHABLE,
            ICMPV6_CODE_BEYOND_SCOPE_OF_SRC_ADDR, 0, ipPacket, ipPacketOffset);

         //Exit immediately
         return ERROR_INVALID_ADDRESS;
      }
   }

   //Hop Limit exceeded in transit?
   if(ipHeader->hopLimit <= 1)
   {
      //If a router receives a packet with a Hop Limit of zero, or if a router
      //decrements a packet's Hop Limit to zero, it must discard the packet
      //and originate an ICMPv6 Time Exceeded message
      icmpv6SendErrorMessage(srcInterface, ICMPV6_TYPE_TIME_EXCEEDED,
         ICMPV6_CODE_HOP_LIMIT_EXCEEDED, 0, ipPacket, ipPacketOffset);

      //Exit immediately
      return ERROR_FAILURE;
   }

   //The Hop-by-Hop Options header, when present, must immediately follow
   //the IPv6 header. Its presence is indicated by the value zero in the
   //Next Header field of the IPv6 header
   if(ipHeader->nextHeader == IPV6_HOP_BY_HOP_OPT_HEADER)
   {
      //Point to the extension header
      size_t headerOffset = ipPacketOffset + sizeof(Ipv6Header);

      //Calculate the offset of the Next Header field
      size_t nextHeaderOffset = ipPacketOffset +
         &ipHeader->nextHeader - (uint8_t *) ipHeader;

      //The Hop-by-Hop Options header is used to carry optional information
      //that must be examined by every node along a packet's delivery path
      error = ipv6ParseHopByHopOptHeader(srcInterface,
         ipPacket, ipPacketOffset, &headerOffset, &nextHeaderOffset);

      //Any error while processing the extension header?
      if(error)
         return error;
   }

   //Allocate a buffer to hold the IPv6 packet
   destBuffer = ethAllocBuffer(length, &destOffset);

   //Successful memory allocation?
   if(destBuffer != NULL)
   {
      //Copy IPv6 header
      error = netBufferCopy(destBuffer, destOffset, ipPacket, ipPacketOffset,
         length);

      //Check status code
      if(!error)
      {
         //Point to the IPv6 header
         ipHeader = netBufferAt(destBuffer, destOffset);
         //Every time a router forwards a packet, it decrements the Hop Limit field
         ipHeader->hopLimit--;

#if (ETH_SUPPORT == ENABLED)
         //Point to the physical interface
         physicalInterface = nicGetPhysicalInterface(destInterface);

         //Ethernet interface?
         if(physicalInterface->nicDriver != NULL &&
            physicalInterface->nicDriver->type == NIC_TYPE_ETHERNET)
         {
            MacAddr destMacAddr;
            NetTxAncillary ancillary;

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_TX_ANCILLARY;

            //Destination IPv6 address
            if(ipv6CompAddr(&destIpAddr, &IPV6_UNSPECIFIED_ADDR))
               destIpAddr = ipHeader->destAddr;

            //Check whether the destination IPv6 address is a multicast address?
            if(ipv6IsMulticastAddr(&destIpAddr))
            {
               //Map IPv6 multicast address to MAC-layer multicast address
               error = ipv6MapMulticastAddrToMac(&destIpAddr, &destMacAddr);
            }
            else
            {
               //Resolve host address using Neighbor Discovery protocol
               error = ndpResolve(destInterface, &destIpAddr, &destMacAddr);
            }

            //Successful address resolution?
            if(!error)
            {
               //Debug message
               TRACE_INFO("Forwarding IPv6 packet to %s (%" PRIuSIZE " bytes)...\r\n",
                  destInterface->name, length);
               //Dump IP header contents for debugging purpose
               ipv6DumpHeader(ipHeader);

               //Send Ethernet frame
               error = ethSendFrame(destInterface, NULL, &destMacAddr, ETH_TYPE_IPV6,
                  destBuffer, destOffset, &ancillary);
            }
            //Address resolution is in progress?
            else if(error == ERROR_IN_PROGRESS)
            {
               //Debug message
               TRACE_INFO("Enqueuing IPv6 packet (%" PRIuSIZE " bytes)...\r\n", length);
               //Dump IP header contents for debugging purpose
               ipv6DumpHeader(ipHeader);

               //Enqueue packets waiting for address resolution
               error = ndpEnqueuePacket(srcInterface, destInterface, &destIpAddr,
                  destBuffer, destOffset, &ancillary);
            }
            //Address resolution failed?
            else
            {
               //Debug message
               TRACE_WARNING("Cannot map IPv6 address to Ethernet address!\r\n");
            }
         }
         else
#endif
#if (PPP_SUPPORT == ENABLED)
         //PPP interface?
         if(destInterface->nicDriver != NULL &&
            destInterface->nicDriver->type == NIC_TYPE_PPP)
         {
            //Debug message
            TRACE_INFO("Forwarding IPv6 packet to %s (%" PRIuSIZE " bytes)...\r\n",
               destInterface->name, length);
            //Dump IP header contents for debugging purpose
            ipv6DumpHeader(ipHeader);

            //Send PPP frame
            error = pppSendFrame(destInterface, destBuffer, destOffset,
               PPP_PROTOCOL_IPV6);
         }
         else
#endif
         //6LoWPAN interface?
         if(destInterface->nicDriver != NULL &&
            destInterface->nicDriver->type == NIC_TYPE_6LOWPAN)
         {
            NetTxAncillary ancillary;

            //Debug message
            TRACE_INFO("Forwarding IPv6 packet to %s (%" PRIuSIZE " bytes)...\r\n",
               destInterface->name, length);
            //Dump IP header contents for debugging purpose
            ipv6DumpHeader(ipHeader);

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_TX_ANCILLARY;

            //Send the packet over the specified link
            error = nicSendPacket(destInterface, destBuffer, destOffset,
               &ancillary);
         }
         else
         //Unknown interface type?
         {
            //Report an error
            error = ERROR_INVALID_INTERFACE;
         }
      }

      //Free previously allocated memory
      netBufferFree(destBuffer);
   }
   else
   {
      //Failed to allocate memory
      error = ERROR_OUT_OF_MEMORY;
   }

   //Return status code
   return error;
}

#endif
