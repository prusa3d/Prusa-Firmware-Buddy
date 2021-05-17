/**
 * @file ndp.c
 * @brief NDP (Neighbor Discovery Protocol)
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
 * The Neighbor Discovery Protocol is responsible for address autoconfiguration
 * of nodes, discovery of the link-layer addresses of other nodes, duplicate
 * address detection, finding available routers and address prefix discovery.
 * Refer to RFC 4861 for more details
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL NDP_TRACE_LEVEL

//Dependencies
#include <limits.h>
#include <string.h>
#include "core/net.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "ipv6/icmpv6.h"
#include "ipv6/ndp.h"
#include "ipv6/ndp_cache.h"
#include "ipv6/ndp_misc.h"
#include "ipv6/slaac.h"
#include "mibs/ip_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED && NDP_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t ndpTickCounter;


/**
 * @brief Neighbor cache initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ndpInit(NetInterface *interface)
{
   NdpContext *context;

   //Point to the NDP context
   context = &interface->ndpContext;

   //Clear the NDP context
   osMemset(context, 0, sizeof(NdpContext));

   //Initialize interface specific variables
   context->reachableTime = NDP_REACHABLE_TIME;
   context->retransTimer = NDP_RETRANS_TIMER;
   context->dupAddrDetectTransmits = NDP_DUP_ADDR_DETECT_TRANSMITS;
   context->minRtrSolicitationDelay = NDP_MIN_RTR_SOLICITATION_DELAY;
   context->maxRtrSolicitationDelay = NDP_MAX_RTR_SOLICITATION_DELAY;
   context->rtrSolicitationInterval = NDP_RTR_SOLICITATION_INTERVAL;
   context->maxRtrSolicitations = NDP_MAX_RTR_SOLICITATIONS;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Address resolution using Neighbor Discovery protocol
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr IPv6 address
 * @param[in] macAddr Physical address matching the specified IPv6 address
 * @return Error code
 **/

error_t ndpResolve(NetInterface *interface, const Ipv6Addr *ipAddr,
   MacAddr *macAddr)
{
   error_t error;
   NdpNeighborCacheEntry *entry;

   //Search the ndpCacheMutex cache for the specified IPv6 address
   entry = ndpFindNeighborCacheEntry(interface, ipAddr);

   //Check whether a matching entry has been found
   if(entry != NULL)
   {
      //Check the state of the Neighbor cache entry
      if(entry->state == NDP_STATE_INCOMPLETE)
      {
         //The address resolution is already in progress
         error = ERROR_IN_PROGRESS;
      }
      else if(entry->state == NDP_STATE_STALE)
      {
         //Copy the MAC address associated with the specified IPv6 address
         *macAddr = entry->macAddr;

         //Start delay timer
         entry->timestamp = osGetSystemTime();
         //Delay before sending the first probe
         entry->timeout = NDP_DELAY_FIRST_PROBE_TIME;
         //Switch to the DELAY state
         entry->state = NDP_STATE_DELAY;

         //Successful address resolution
         error = NO_ERROR;
      }
      else
      {
         //Copy the MAC address associated with the specified IPv6 address
         *macAddr = entry->macAddr;

         //Successful address resolution
         error = NO_ERROR;
      }
   }
   else
   {
      //If no entry exists, then create a new one
      entry = ndpCreateNeighborCacheEntry(interface);

      //Neighbor Cache entry successfully created?
      if(entry != NULL)
      {
         //Record the IPv6 address whose MAC address is unknown
         entry->ipAddr = *ipAddr;

         //Reset retransmission counter
         entry->retransmitCount = 0;
         //No packet are pending in the transmit queue
         entry->queueSize = 0;

         //Send a multicast Neighbor Solicitation message
         ndpSendNeighborSol(interface, ipAddr, TRUE);

         //Save the time at which the message was sent
         entry->timestamp = osGetSystemTime();
         //Set timeout value
         entry->timeout = interface->ndpContext.retransTimer;
         //Enter INCOMPLETE state
         entry->state = NDP_STATE_INCOMPLETE;

         //The address resolution is in progress
         error = ERROR_IN_PROGRESS;
      }
      else
      {
         //Failed to create Neighbor Cache entry...
         error = ERROR_OUT_OF_RESOURCES;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Enqueue an IPv6 packet waiting for address resolution
 * @param[in] srcInterface Interface from which the packet has been received
 * @param[in] destInterface Interface on which the packet should be sent
 * @param[in] ipAddr IPv6 address of the destination host
 * @param[in] buffer Multi-part buffer containing the packet to be enqueued
 * @param[in] offset Offset to the first byte of the packet
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ndpEnqueuePacket(NetInterface *srcInterface,
   NetInterface *destInterface, const Ipv6Addr *ipAddr, NetBuffer *buffer,
      size_t offset, NetTxAncillary *ancillary)
{
   error_t error;
   uint_t i;
   size_t length;
   NdpNeighborCacheEntry *entry;

   //Retrieve the length of the multi-part buffer
   length = netBufferGetLength(buffer);

   //Search the Neighbor cache for the specified IPv6 address
   entry = ndpFindNeighborCacheEntry(destInterface, ipAddr);

   //Check whether a matching entry exists
   if(entry != NULL)
   {
      //Check current state
      if(entry->state == NDP_STATE_INCOMPLETE)
      {
         //Check whether the packet queue is full
         if(entry->queueSize >= NDP_MAX_PENDING_PACKETS)
         {
            //When the queue overflows, the new arrival should replace the oldest entry
            netBufferFree(entry->queue[0].buffer);

            //Make room for the new packet
            for(i = 1; i < NDP_MAX_PENDING_PACKETS; i++)
            {
               entry->queue[i - 1] = entry->queue[i];
            }

            //Adjust the number of pending packets
            entry->queueSize--;
         }

         //Index of the entry to be filled in
         i = entry->queueSize;
         //Allocate a memory buffer to store the packet
         entry->queue[i].buffer = netBufferAlloc(length);

         //Successful memory allocation?
         if(entry->queue[i].buffer != NULL)
         {
            //If the IPv6 packet has been forwarded, record the network
            //interface from which the packet has been received
            entry->queue[i].srcInterface = srcInterface;

            //Copy the contents of the IPv6 packet
            netBufferCopy(entry->queue[i].buffer, 0, buffer, 0, length);
            //Offset to the first byte of the IPv6 header
            entry->queue[i].offset = offset;
            //Additional options passed to the stack along with the packet
            entry->queue[i].ancillary = *ancillary;

            //Increment the number of queued packets
            entry->queueSize++;
            //The packet was successfully enqueued
            error = NO_ERROR;
         }
         else
         {
            //Failed to allocate memory
            error = ERROR_OUT_OF_MEMORY;
         }
      }
      else
      {
         //The address is already resolved
         error = ERROR_UNEXPECTED_STATE;
      }
   }
   else
   {
      //No matching entry in Neighbor Cache
      error = ERROR_NOT_FOUND;
   }

   //Return status code
   return error;
}


/**
 * @brief NDP timer handler
 * @param[in] interface Underlying network interface
 **/

void ndpTick(NetInterface *interface)
{
   systime_t time;
   NdpContext *context;

   //Point to the NDP context
   context = &interface->ndpContext;

   //Get current time
   time = osGetSystemTime();

   //When an interface becomes enabled, a host may send some Router
   //Solicitation messages to obtain Router Advertisements quickly
   if(interface->linkState && !interface->ipv6Context.isRouter)
   {
      //Make sure that a valid link-local address has been assigned to the interface
      if(ipv6GetLinkLocalAddrState(interface) == IPV6_ADDR_STATE_PREFERRED)
      {
         //The host should transmit up to MAX_RTR_SOLICITATIONS Router
         //Solicitation messages
         if(context->rtrSolicitationCount == 0)
         {
            //Set time stamp
            context->timestamp = time;

            //Check whether the host has already performed Duplicate Address
            //Detection for the link-local address
            if(context->dupAddrDetectTransmits > 0)
            {
               //If a host has already performed a random delay since the interface
               //became enabled, there is no need to delay again before sending the
               //first Router Solicitation message
               context->timeout = 0;
            }
            else
            {
               //Before a host sends an initial solicitation, it should delay the
               //transmission for a random amount of time in order to alleviate
               //congestion when many hosts start up on a link at the same time
               context->timeout = netGetRandRange(context->minRtrSolicitationDelay,
                  context->maxRtrSolicitationDelay);
            }

            //Prepare to send the first Router Solicitation message
            context->rtrSolicitationCount = 1;
         }
         else if(context->rtrSolicitationCount <= context->maxRtrSolicitations)
         {
            //Once the host sends a Router Solicitation, and receives a valid
            //Router Advertisement with a non-zero Router Lifetime, the host must
            //desist from sending additional solicitations on that interface
            if(!context->rtrAdvReceived)
            {
               //Check current time
               if(timeCompare(time, context->timestamp + context->timeout) >= 0)
               {
                  //Send Router Solicitation message
                  ndpSendRouterSol(interface);

                  //Save the time at which the message was sent
                  context->timestamp = time;
                  //Set timeout value
                  context->timeout = context->rtrSolicitationInterval;
                  //Increment retransmission counter
                  context->rtrSolicitationCount++;
               }
            }
         }
      }
   }

   //Periodically update the Neighbor Cache
   ndpUpdateNeighborCache(interface);

   //Manage the lifetime of IPv6 addresses
   ndpUpdateAddrList(interface);

   //Periodically update the Prefix List
   ndpUpdatePrefixList(interface);

   //Periodically update the Default Router List
   ndpUpdateDefaultRouterList(interface);
}


/**
 * @brief Callback function for link change event
 * @param[in] interface Underlying network interface
 **/

void ndpLinkChangeEvent(NetInterface *interface)
{
   NdpContext *context;

   //Point to the NDP context
   context = &interface->ndpContext;

   //Restore default parameters
   context->reachableTime = NDP_REACHABLE_TIME;
   context->retransTimer = NDP_RETRANS_TIMER;
   context->dupAddrDetectTransmits = NDP_DUP_ADDR_DETECT_TRANSMITS;
   context->minRtrSolicitationDelay = NDP_MIN_RTR_SOLICITATION_DELAY;
   context->maxRtrSolicitationDelay = NDP_MAX_RTR_SOLICITATION_DELAY;
   context->rtrSolicitationInterval = NDP_RTR_SOLICITATION_INTERVAL;
   context->maxRtrSolicitations = NDP_MAX_RTR_SOLICITATIONS;

   //Reset retransmission counter for RS messages
   context->rtrSolicitationCount = 0;
   //Valid RA message not yet received
   context->rtrAdvReceived = FALSE;

   //Flush the Neighbor Cache
   ndpFlushNeighborCache(interface);
   //Flush the Destination Cache
   ndpFlushDestCache(interface);
}


/**
 * @brief Router Advertisement message processing
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv6 pseudo header
 * @param[in] buffer Multi-part buffer containing the Router Advertisement message
 * @param[in] offset Offset to the first byte of the message
 * @param[in] hopLimit Hop Limit field from IPv6 header
 **/

void ndpProcessRouterAdv(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit)
{
   error_t error;
   uint32_t n;
   size_t length;
   NdpRouterAdvMessage *message;
   NdpMtuOption *mtuOption;
   NdpPrefixInfoOption *prefixInfoOption;
#if (ETH_SUPPORT == ENABLED)
   NdpLinkLayerAddrOption *linkLayerAddrOption;
   NdpNeighborCacheEntry *entry;
#endif

   //Retrieve the length of the message
   length = netBufferGetLength(buffer) - offset;

   //Check the length of the Router Advertisement message
   if(length < sizeof(NdpRouterAdvMessage))
      return;

   //Point to the beginning of the message
   message = netBufferAt(buffer, offset);
   //Sanity check
   if(message == NULL)
      return;

   //Debug message
   TRACE_INFO("Router Advertisement message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message contents for debugging purpose
   ndpDumpRouterAdvMessage(message);

   //Routers must use their link-local address as the source for the
   //Router Advertisement so that hosts can uniquely identify routers
   if(!ipv6IsLinkLocalUnicastAddr(&pseudoHeader->srcAddr))
      return;

   //The IPv6 Hop Limit field must have a value of 255 to ensure
   //that the packet has not been forwarded by a router
   if(hopLimit != NDP_HOP_LIMIT)
      return;

   //ICMPv6 Code must be 0. An advertisement that passes the validity
   //checks is called a valid advertisement
   if(message->code)
      return;

   //Calculate the length of the Options field
   length -= sizeof(NdpRouterAdvMessage);

   //Parse Options field
   error = ndpCheckOptions(message->options, length);
   //All included options must have a length that is greater than zero
   if(error)
      return;

   //Check the Router Lifetime value
   if(ntohs(message->routerLifetime) != 0)
   {
      //Add a new entry in the Default Router List
      ipv6AddDefaultRouter(interface, &pseudoHeader->srcAddr,
         ntohs(message->routerLifetime), message->prf);

      //The host should send at least one solicitation in the case where
      //an advertisement is received prior to having sent a solicitation
      if(interface->ndpContext.rtrSolicitationCount > 1)
      {
         //Once the host sends a Router Solicitation, and receives a valid
         //Router Advertisement with a non-zero Router Lifetime, the host must
         //desist from sending additional solicitations on that interface
         interface->ndpContext.rtrAdvReceived = TRUE;
      }
   }
   else
   {
      //Immediately time-out the entry
      ipv6RemoveDefaultRouter(interface, &pseudoHeader->srcAddr);
   }

   //6LoWPAN interface?
   if(interface->nicDriver != NULL &&
      interface->nicDriver->type == NIC_TYPE_6LOWPAN)
   {
      //In all cases, the Router Solicitation retransmissions are terminated
      //when a Router Advertisement is received (refer to RFC 6675 5.3)
      interface->ndpContext.rtrAdvReceived = TRUE;
   }

   //A Router Advertisement field (Cur Hop Limit, Reachable Time, and
   //Retrans Timer) may contain a value denoting that it is unspecified.
   //In such cases, the parameter should be ignored and the host should
   //continue using whatever value it is already using
   if(message->curHopLimit != 0)
   {
      //Get the default value that should be placed in the Hop Count
      //field of the IP header for outgoing IP packets
      interface->ipv6Context.curHopLimit = message->curHopLimit;
   }

   //A value of zero means unspecified...
   if(message->reachableTime != 0)
   {
      //The Reachable Time field holds the time, in milliseconds, that
      //a node assumes a neighbor is reachable after having received a
      //reachability confirmation
      interface->ndpContext.reachableTime = ntohl(message->reachableTime);
   }

   //A value of zero means unspecified...
   if(message->retransTimer != 0)
   {
      //The Retrans Timer field holds the time, in milliseconds,
      //between retransmitted Neighbor Solicitation messages
      interface->ndpContext.retransTimer = ntohl(message->retransTimer);
   }

#if (ETH_SUPPORT == ENABLED)
   //Search for the Source Link-Layer Address option
   linkLayerAddrOption = ndpGetOption(message->options,
      length, NDP_OPT_SOURCE_LINK_LAYER_ADDR);

   //Source Link-Layer Address option found?
   if(linkLayerAddrOption != NULL && linkLayerAddrOption->length == 1)
   {
      //Debug message
      TRACE_DEBUG("  Source Link-Layer Address = %s\r\n",
         macAddrToString(&linkLayerAddrOption->linkLayerAddr, NULL));
   }
   else
   {
      //No valid Source Link-Layer Address option...
      linkLayerAddrOption = NULL;
   }

   //Search the Neighbor cache for the router
   entry = ndpFindNeighborCacheEntry(interface, &pseudoHeader->srcAddr);

   //No matching entry has been found?
   if(!entry)
   {
      //If the advertisement contains a Source Link-Layer Address option,
      //the link-layer address should be recorded in the Neighbor cache
      if(linkLayerAddrOption)
      {
         //Create an entry for the router
         entry = ndpCreateNeighborCacheEntry(interface);

         //Neighbor cache entry successfully created?
         if(entry)
         {
            //Record the IPv6 address and the corresponding MAC address
            entry->ipAddr = pseudoHeader->srcAddr;
            entry->macAddr = linkLayerAddrOption->linkLayerAddr;
            //The IsRouter flag must be set to TRUE
            entry->isRouter = TRUE;
            //Save current time
            entry->timestamp = osGetSystemTime();
            //The reachability state must be set to STALE
            entry->state = NDP_STATE_STALE;
         }
      }
   }
   else
   {
      //The sender of a Router Advertisement is implicitly assumed to be a router
      entry->isRouter = TRUE;

      //Check if the advertisement contains a Source Link-Layer Address option
      if(linkLayerAddrOption)
      {
         //INCOMPLETE state?
         if(entry->state == NDP_STATE_INCOMPLETE)
         {
            //Record link-layer address
            entry->macAddr = linkLayerAddrOption->linkLayerAddr;
            //Send all the packets that are pending for transmission
            n = ndpSendQueuedPackets(interface, entry);
            //Save current time
            entry->timestamp = osGetSystemTime();

            //Check whether any packets have been sent
            if(n > 0)
            {
               //Start delay timer
               entry->timeout = NDP_DELAY_FIRST_PROBE_TIME;
               //Switch to the DELAY state
               entry->state = NDP_STATE_DELAY;
            }
            else
            {
               //Enter the STALE state
               entry->state = NDP_STATE_STALE;
            }
         }
         //REACHABLE, STALE, DELAY or PROBE state?
         else
         {
            //Different link-layer address than cached?
            if(!macCompAddr(&entry->macAddr, &linkLayerAddrOption->linkLayerAddr))
            {
               //Update link-layer address
               entry->macAddr = linkLayerAddrOption->linkLayerAddr;
               //Save current time
               entry->timestamp = osGetSystemTime();
               //The reachability state must be set to STALE
               entry->state = NDP_STATE_STALE;
            }
         }
      }
   }
#endif

   //Search for the MTU option
   mtuOption = ndpGetOption(message->options, length, NDP_OPT_MTU);

   //MTU option found?
   if(mtuOption != NULL && mtuOption->length == 1)
   {
      NetInterface *physicalInterface;

      //Point to the physical interface
      physicalInterface = nicGetPhysicalInterface(interface);

      //This option specifies the recommended MTU for the link
      n = ntohl(mtuOption->mtu);

      //The host should copy the option's value so long as the value is greater
      //than or equal to the minimum IPv6 MTU and does not exceed the maximum
      //MTU of the interface
      if(n >= IPV6_DEFAULT_MTU && n <= physicalInterface->nicDriver->mtu)
      {
         //Save the MTU value
         interface->ipv6Context.linkMtu = n;
      }
   }

   //Point to the beginning of the Options field
   n = 0;

   //Parse Options field
   while(1)
   {
      //Search the Options field for any Prefix Information options
      prefixInfoOption = ndpGetOption(message->options + n,
         length - n, NDP_OPT_PREFIX_INFORMATION);

      //No more option of the specified type?
      if(prefixInfoOption == NULL)
         break;

      //Hosts use the advertised on-link prefixes to build and maintain
      //a list that is used in deciding when a packet's destination is
      //on-link or beyond a router
      ndpParsePrefixInfoOption(interface, prefixInfoOption);

      //Retrieve the offset to the current position
      n = (uint8_t *) prefixInfoOption - message->options;
      //Jump to the next option
      n += prefixInfoOption->length * 8;
   }

#if (SLAAC_SUPPORT == ENABLED)
   //Stateless Address Autoconfiguration is currently used?
   if(interface->slaacContext != NULL)
   {
      //Process the valid advertisement
      slaacParseRouterAdv(interface->slaacContext, message,
         length + sizeof(NdpRouterAdvMessage));
   }
#endif
}


/**
 * @brief Neighbor Solicitation message processing
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv6 pseudo header
 * @param[in] buffer Multi-part buffer containing the Neighbor Solicitation message
 * @param[in] offset Offset to the first byte of the message
 * @param[in] hopLimit Hop Limit field from IPv6 header
 **/

void ndpProcessNeighborSol(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit)
{
#if (ETH_SUPPORT == ENABLED)
   error_t error;
   uint_t i;
   uint_t n;
   size_t length;
   bool_t validTarget;
   NdpNeighborSolMessage *message;
   NdpLinkLayerAddrOption *option;
   NdpNeighborCacheEntry *neighborCacheEntry;
   Ipv6AddrEntry *addrEntry;

   //Retrieve the length of the message
   length = netBufferGetLength(buffer) - offset;

   //Check the length of the Neighbor Solicitation message
   if(length < sizeof(NdpNeighborSolMessage))
      return;

   //Point to the beginning of the message
   message = netBufferAt(buffer, offset);
   //Sanity check
   if(message == NULL)
      return;

   //Debug message
   TRACE_INFO("Neighbor Solicitation message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message contents for debugging purpose
   ndpDumpNeighborSolMessage(message);

   //The IPv6 Hop Limit field must have a value of 255 to ensure
   //that the packet has not been forwarded by a router
   if(hopLimit != NDP_HOP_LIMIT)
      return;

   //ICMPv6 Code must be 0
   if(message->code)
      return;

   //If the IP source address is the unspecified address, the IP
   //destination address must be a solicited-node multicast address
   if(ipv6CompAddr(&pseudoHeader->srcAddr, &IPV6_UNSPECIFIED_ADDR) &&
      !ipv6IsSolicitedNodeAddr(&pseudoHeader->destAddr))
   {
      //Debug message
      TRACE_WARNING("Destination address must be a solicited-node address!\r\n");
      //Exit immediately
      return;
   }

   //Calculate the length of the Options field
   length -= sizeof(NdpNeighborSolMessage);

   //Parse Options field
   error = ndpCheckOptions(message->options, length);
   //All included options must have a length that is greater than zero
   if(error)
      return;

   //Search for the Source Link-Layer Address option
   option = ndpGetOption(message->options,
      length, NDP_OPT_SOURCE_LINK_LAYER_ADDR);

   //The target address must a valid unicast or anycast address assigned to
   //the interface or a tentative address on which DAD is being performed
   validTarget = FALSE;

   //Loop through the IPv6 addresses assigned to the interface
   for(i = 0; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      addrEntry = &interface->ipv6Context.addrList[i];

      //Compare target address
      if(ipv6CompAddr(&addrEntry->addr, &message->targetAddr))
      {
         //Check address state
         if(addrEntry->state == IPV6_ADDR_STATE_TENTATIVE)
         {
            //If the source address of the Neighbor Solicitation is the
            //unspecified address, the solicitation is from a node
            //performing Duplicate Address Detection
            if(ipv6CompAddr(&pseudoHeader->srcAddr, &IPV6_UNSPECIFIED_ADDR))
            {
               //The source link-layer address must not be included when the
               //source IP address is the unspecified address...
               if(option == NULL)
               {
                  //Debug message
                  TRACE_WARNING("The tentative address %s is a duplicate!\r\n",
                     ipv6AddrToString(&addrEntry->addr, NULL));

                  //The tentative address is a duplicate and should not be used
                  addrEntry->duplicate = TRUE;
               }
            }

            //In all cases, a node must not respond to a Neighbor Solicitation
            //for a tentative address
            return;
         }
         else if(addrEntry->state != IPV6_ADDR_STATE_INVALID)
         {
            //The target address is a valid address assigned to the interface
            validTarget = TRUE;
            //We are done
            break;
         }
      }
   }

   //Invalid target address?
   if(!validTarget)
   {
      //The Neighbor Solicitation must be discarded if the target address
      //is not a valid anycast address assigned to the interface
      if(!ipv6IsAnycastAddr(interface, &message->targetAddr))
      {
         //Debug message
         TRACE_WARNING("Wrong target address!\r\n");
         //Exit immediately
         return;
      }
   }

   //Source Link-Layer Address option found?
   if(option != NULL && option->length == 1)
   {
      //Debug message
      TRACE_DEBUG("  Source Link-Layer Address = %s\r\n",
         macAddrToString(&option->linkLayerAddr, NULL));

      //The Source Link-Layer Address option must not be included when the
      //source IP address is the unspecified address
      if(ipv6CompAddr(&pseudoHeader->srcAddr, &IPV6_UNSPECIFIED_ADDR))
         return;

      //Search the Neighbor Cache for the source address of the solicitation
      neighborCacheEntry = ndpFindNeighborCacheEntry(interface, &pseudoHeader->srcAddr);

      //No matching entry has been found?
      if(!neighborCacheEntry)
      {
         //Create an entry
         neighborCacheEntry = ndpCreateNeighborCacheEntry(interface);

         //Neighbor Cache entry successfully created?
         if(neighborCacheEntry)
         {
            //Record the IPv6 and the corresponding MAC address
            neighborCacheEntry->ipAddr = pseudoHeader->srcAddr;
            neighborCacheEntry->macAddr = option->linkLayerAddr;
            //Save current time
            neighborCacheEntry->timestamp = osGetSystemTime();
            //Enter the STALE state
            neighborCacheEntry->state = NDP_STATE_STALE;
         }
      }
      else
      {
         //INCOMPLETE state?
         if(neighborCacheEntry->state == NDP_STATE_INCOMPLETE)
         {
            //Record link-layer address
            neighborCacheEntry->macAddr = option->linkLayerAddr;
            //Send all the packets that are pending for transmission
            n = ndpSendQueuedPackets(interface, neighborCacheEntry);
            //Save current time
            neighborCacheEntry->timestamp = osGetSystemTime();

            //Check whether any packets have been sent
            if(n > 0)
            {
               //Start delay timer
               neighborCacheEntry->timeout = NDP_DELAY_FIRST_PROBE_TIME;
               //Switch to the DELAY state
               neighborCacheEntry->state = NDP_STATE_DELAY;
            }
            else
            {
               //Enter the STALE state
               neighborCacheEntry->state = NDP_STATE_STALE;
            }
         }
         //REACHABLE, STALE, DELAY or PROBE state?
         else
         {
            //Different link-layer address than cached?
            if(!macCompAddr(&neighborCacheEntry->macAddr, &option->linkLayerAddr))
            {
               //Update link-layer address
               neighborCacheEntry->macAddr = option->linkLayerAddr;
               //Save current time
               neighborCacheEntry->timestamp = osGetSystemTime();
               //Enter the STALE state
               neighborCacheEntry->state = NDP_STATE_STALE;
            }
         }
      }
   }
   //Source Link-Layer Address option not found?
   else
   {
      //The Source Link-Layer Address option must not be included when the
      //source IP address is the unspecified address. Otherwise, this option
      //must be included in multicast solicitations
      if(!ipv6CompAddr(&pseudoHeader->srcAddr, &IPV6_UNSPECIFIED_ADDR) &&
         ipv6IsMulticastAddr(&pseudoHeader->destAddr))
      {
         //Debug message
         TRACE_WARNING("The Source Link-Layer Address must be included!\r\n");
         //Exit immediately
         return;
      }
   }

   //After any updates to the Neighbor cache, the node sends a Neighbor
   //Advertisement response as described in RFC 4861 7.2.4
   ndpSendNeighborAdv(interface, &message->targetAddr, &pseudoHeader->srcAddr);
#endif
}


/**
 * @brief Neighbor Advertisement message processing
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv6 pseudo header
 * @param[in] buffer Multi-part buffer containing the Neighbor Advertisement message
 * @param[in] offset Offset to the first byte of the message
 * @param[in] hopLimit Hop Limit field from IPv6 header
 **/

void ndpProcessNeighborAdv(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit)
{
#if (ETH_SUPPORT == ENABLED)
   error_t error;
   uint_t i;
   uint_t n;
   size_t length;
   bool_t differentLinkLayerAddr;
   NdpNeighborAdvMessage *message;
   NdpLinkLayerAddrOption *option;
   NdpNeighborCacheEntry *neighborCacheEntry;
   Ipv6AddrEntry *addrEntry;

   //Retrieve the length of the message
   length = netBufferGetLength(buffer) - offset;

   //Check the length of the Neighbor Advertisement message
   if(length < sizeof(NdpNeighborAdvMessage))
      return;

   //Point to the beginning of the message
   message = netBufferAt(buffer, offset);
   //Sanity check
   if(message == NULL)
      return;

   //Debug message
   TRACE_INFO("Neighbor Advertisement message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message contents for debugging purpose
   ndpDumpNeighborAdvMessage(message);

   //The IPv6 Hop Limit field must have a value of 255 to ensure
   //that the packet has not been forwarded by a router
   if(hopLimit != NDP_HOP_LIMIT)
      return;

   //ICMPv6 Code must be 0
   if(message->code)
      return;

   //The target address must not be a multicast address
   if(ipv6IsMulticastAddr(&message->targetAddr))
   {
      //Debug message
      TRACE_WARNING("Target address must not be a multicast address!\r\n");
      //Exit immediately
      return;
   }

   //If the destination address is a multicast address
   //then the Solicited flag must be zero
   if(ipv6IsMulticastAddr(&pseudoHeader->destAddr) && message->s)
   {
      //Debug message
      TRACE_WARNING("Solicited flag must be zero!\r\n");
      //Exit immediately
      return;
   }

   //Calculate the length of the Options field
   length -= sizeof(NdpNeighborAdvMessage);

   //Parse Options field
   error = ndpCheckOptions(message->options, length);
   //All included options must have a length that is greater than zero
   if(error)
      return;

   //Duplicate address detection
   for(i = 0; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      addrEntry = &interface->ipv6Context.addrList[i];

      //Valid entry?
      if(addrEntry->state != IPV6_ADDR_STATE_INVALID)
      {
         //Check whether the target address is tentative or matches
         //a unicast address assigned to the interface
         if(ipv6CompAddr(&addrEntry->addr, &message->targetAddr))
         {
            //Debug message
            TRACE_WARNING("The address %s is a duplicate!\r\n",
               ipv6AddrToString(&addrEntry->addr, NULL));

            //The address is a duplicate and should not be used
            addrEntry->duplicate = TRUE;
            //Exit immediately
            return;
         }
      }
   }

   //Search the Neighbor cache for the specified target address
   neighborCacheEntry = ndpFindNeighborCacheEntry(interface, &message->targetAddr);

   //If no entry exists, the advertisement should be silently discarded
   if(neighborCacheEntry)
   {
      //This flag tells whether the supplied link-layer
      //address differs from that in the cache
      differentLinkLayerAddr = FALSE;

      //Search for the Target Link-Layer Address option
      option = ndpGetOption(message->options,
         length, NDP_OPT_TARGET_LINK_LAYER_ADDR);

      //Target Link-Layer Address option found?
      if(option != NULL && option->length == 1)
      {
         //Debug message
         TRACE_DEBUG("  Target Link-Layer Address = %s\r\n",
            macAddrToString(&option->linkLayerAddr, NULL));

         //Different link-layer address than cached?
         if(!macCompAddr(&neighborCacheEntry->macAddr, &option->linkLayerAddr))
            differentLinkLayerAddr = TRUE;
      }

      //INCOMPLETE state?
      if(neighborCacheEntry->state == NDP_STATE_INCOMPLETE)
      {
         //If no Target Link-Layer Address option is included, the receiving
         //node should silently discard the received advertisement
         if(option != NULL && option->length == 1)
         {
            //Record the link-layer address
            neighborCacheEntry->macAddr = option->linkLayerAddr;
            //Send all the packets that are pending for transmission
            n = ndpSendQueuedPackets(interface, neighborCacheEntry);
            //Save current time
            neighborCacheEntry->timestamp = osGetSystemTime();

            //Solicited flag is set?
            if(message->s)
            {
               //Computing the random ReachableTime value
               neighborCacheEntry->timeout = interface->ndpContext.reachableTime;
               //Switch to the REACHABLE state
               neighborCacheEntry->state = NDP_STATE_REACHABLE;
            }
            else
            {
               //Check whether any packets have been sent
               if(n > 0)
               {
                  //Start delay timer
                  neighborCacheEntry->timeout = NDP_DELAY_FIRST_PROBE_TIME;
                  //Switch to the DELAY state
                  neighborCacheEntry->state = NDP_STATE_DELAY;
               }
               else
               {
                  //Enter the STALE state
                  neighborCacheEntry->state = NDP_STATE_STALE;
               }
            }
         }
      }
      //REACHABLE, STALE, DELAY or PROBE state?
      else
      {
         //Check whether the Override flag is clear and the supplied
         //link-layer address differs from that in the cache
         if(!message->o && differentLinkLayerAddr)
         {
            //REACHABLE state?
            if(neighborCacheEntry->state == NDP_STATE_REACHABLE)
            {
               //Save current time
               neighborCacheEntry->timestamp = osGetSystemTime();
               //Enter the STALE state
               neighborCacheEntry->state = NDP_STATE_STALE;
            }
         }
         else
         {
            //Solicited flag is set?
            if(message->s)
            {
               //Different link-layer address than cached?
               if(differentLinkLayerAddr)
               {
                  //The link-layer address must be inserted in the cache
                  neighborCacheEntry->macAddr = option->linkLayerAddr;
               }

               //Save current time
               neighborCacheEntry->timestamp = osGetSystemTime();
               //Computing the random ReachableTime value
               neighborCacheEntry->timeout = interface->ndpContext.reachableTime;
               //Switch to the REACHABLE state
               neighborCacheEntry->state = NDP_STATE_REACHABLE;
            }
            else
            {
               //Different link-layer address than cached?
               if(differentLinkLayerAddr)
               {
                  //The link-layer address must be inserted in the cache
                  neighborCacheEntry->macAddr = option->linkLayerAddr;
                  //Save current time
                  neighborCacheEntry->timestamp = osGetSystemTime();
                  //The state must be set to STALE
                  neighborCacheEntry->state = NDP_STATE_STALE;
               }
            }
         }
      }

      //The IsRouter flag in the cache entry must be set based
      //on the Router flag in the received advertisement
      if(message->r)
      {
         //The neighbor is a router
         neighborCacheEntry->isRouter = TRUE;
      }
      else
      {
         //Check whether the IsRouter flag changes from TRUE to FALSE
         //as a result of this update
         if(neighborCacheEntry->isRouter)
         {
            //The node must remove that router from the Default Router list
            //and update the Destination cache entries for all destinations
            //using that neighbor as a router
            ipv6RemoveDefaultRouter(interface, &neighborCacheEntry->ipAddr);
         }

         //The neighbor is a host
         neighborCacheEntry->isRouter = FALSE;
      }
   }
#endif
}


/**
 * @brief Redirect message processing
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv6 pseudo header
 * @param[in] buffer Multi-part buffer containing the Redirect message
 * @param[in] offset Offset to the first byte of the message
 * @param[in] hopLimit Hop Limit field from IPv6 header
 **/

void ndpProcessRedirect(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit)
{
#if (ETH_SUPPORT == ENABLED)
   error_t error;
   uint_t n;
   size_t length;
   NdpRedirectMessage *message;
   NdpLinkLayerAddrOption *option;
   NdpNeighborCacheEntry *neighborCacheEntry;
   NdpDestCacheEntry *destCacheEntry;

   //Retrieve the length of the message
   length = netBufferGetLength(buffer) - offset;

   //Check the length of the Redirect message
   if(length < sizeof(NdpRedirectMessage))
      return;

   //Point to the beginning of the message
   message = netBufferAt(buffer, offset);
   //Sanity check
   if(message == NULL)
      return;

   //Debug message
   TRACE_INFO("Redirect message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message contents for debugging purpose
   ndpDumpRedirectMessage(message);

   //The IPv6 Hop Limit field must have a value of 255 to ensure
   //that the packet has not been forwarded by a router
   if(hopLimit != NDP_HOP_LIMIT)
      return;

   //ICMPv6 Code must be 0
   if(message->code)
      return;

   //Routers must use their link-local address as the source for Redirect
   //messages so that hosts can uniquely identify routers
   if(!ipv6IsLinkLocalUnicastAddr(&pseudoHeader->srcAddr))
      return;

   //The IP source address of the Redirect must be the same as the current
   //first-hop router for the specified Destination address
   if(!ndpIsFirstHopRouter(interface, &message->destAddr, &pseudoHeader->srcAddr))
      return;

   //The Destination Address field in the Redirect message must not
   //contain a multicast address
   if(ipv6IsMulticastAddr(&message->destAddr))
      return;

   //The Target Address must be either a link-local address (when redirected
   //to a router) or the same as the Destination Address (when redirected to
   //the on-link destination)
   if(!ipv6IsLinkLocalUnicastAddr(&message->targetAddr) &&
      !ipv6CompAddr(&message->targetAddr, &message->destAddr))
   {
      //Silently discard the received Redirect message
      return;
   }

   //Calculate the length of the Options field
   length -= sizeof(NdpNeighborAdvMessage);

   //Parse Options field
   error = ndpCheckOptions(message->options, length);
   //All included options must have a length that is greater than zero
   if(error)
      return;

   //Search the Destination cache for the specified address
   destCacheEntry = ndpFindDestCacheEntry(interface, &message->destAddr);

   //Check whether a corresponding Destination cache entry exists
   if(destCacheEntry)
   {
      //The entry is updated with information learned from Redirect messages
      destCacheEntry->nextHop = message->targetAddr;
      //Save current time
      destCacheEntry->timestamp = osGetSystemTime();
   }
   else
   {
      //If no Destination Cache entry exists for the destination, an
      //implementation should create such an entry
      destCacheEntry = ndpCreateDestCacheEntry(interface);

      //Destination cache entry successfully created?
      if(destCacheEntry)
      {
         //Destination address
         destCacheEntry->destAddr = message->destAddr;
         //Address of the next hop
         destCacheEntry->nextHop = message->targetAddr;

         //Initially, the PMTU value for a path is assumed to be
         //the MTU of the first-hop link
         destCacheEntry->pathMtu = interface->ipv6Context.linkMtu;

         //Save current time
         destCacheEntry->timestamp = osGetSystemTime();
      }
   }

   //Search for the Target Link-Layer Address option
   option = ndpGetOption(message->options,
      length, NDP_OPT_TARGET_LINK_LAYER_ADDR);

   //If the Redirect contains a Target Link-Layer Address option, the host
   //either creates or updates the Neighbor Cache entry for the target
   if(option != NULL && option->length == 1)
   {
      //Debug message
      TRACE_DEBUG("  Target Link-Layer Address = %s\r\n",
         macAddrToString(&option->linkLayerAddr, NULL));

      //Search the Neighbor cache for the specified target address
      neighborCacheEntry = ndpFindNeighborCacheEntry(interface, &message->targetAddr);

      //No matching entry has been found?
      if(!neighborCacheEntry)
      {
         //Create an entry for the target
         neighborCacheEntry = ndpCreateNeighborCacheEntry(interface);

         //Neighbor cache entry successfully created?
         if(neighborCacheEntry)
         {
            //Record the Target address
            neighborCacheEntry->ipAddr = message->targetAddr;
            //The cached link-layer address is copied from the option
            neighborCacheEntry->macAddr = option->linkLayerAddr;
            //Newly created Neighbor Cache entries should set the IsRouter flag to FALSE
            neighborCacheEntry->isRouter = FALSE;
            //Save current time
            neighborCacheEntry->timestamp = osGetSystemTime();
            //The reachability state must be set to STALE
            neighborCacheEntry->state = NDP_STATE_STALE;
         }
      }
      else
      {
         //If the Target Address is not the same as the Destination Address,
         //the host must set IsRouter to TRUE for the target
         if(!ipv6CompAddr(&message->targetAddr, &message->destAddr))
            neighborCacheEntry->isRouter = TRUE;

         //INCOMPLETE state?
         if(neighborCacheEntry->state == NDP_STATE_INCOMPLETE)
         {
            //Record link-layer address
            neighborCacheEntry->macAddr = option->linkLayerAddr;
            //Send all the packets that are pending for transmission
            n = ndpSendQueuedPackets(interface, neighborCacheEntry);
            //Save current time
            neighborCacheEntry->timestamp = osGetSystemTime();

            //Check whether any packets have been sent
            if(n > 0)
            {
               //Start delay timer
               neighborCacheEntry->timeout = NDP_DELAY_FIRST_PROBE_TIME;
               //Switch to the DELAY state
               neighborCacheEntry->state = NDP_STATE_DELAY;
            }
            else
            {
               //Enter the STALE state
               neighborCacheEntry->state = NDP_STATE_STALE;
            }
         }
         //REACHABLE, STALE, DELAY or PROBE state?
         else
         {
            //Different link-layer address than cached?
            if(!macCompAddr(&neighborCacheEntry->macAddr, &option->linkLayerAddr))
            {
               //Update link-layer address
               neighborCacheEntry->macAddr = option->linkLayerAddr;
               //Save current time
               neighborCacheEntry->timestamp = osGetSystemTime();
               //The reachability state must be set to STALE
               neighborCacheEntry->state = NDP_STATE_STALE;
            }
         }
      }
   }
#endif
}


/**
 * @brief Send a Router Solicitation message
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ndpSendRouterSol(NetInterface *interface)
{
   error_t error;
   size_t offset;
   size_t length;
   NetBuffer *buffer;
   NdpRouterSolMessage *message;
   Ipv6PseudoHeader pseudoHeader;
   NetTxAncillary ancillary;

   //The destination address is typically the all-routers multicast address
   pseudoHeader.destAddr = IPV6_LINK_LOCAL_ALL_ROUTERS_ADDR;

   //Select the most appropriate source address to be used when sending the
   //Router Solicitation message
   error = ipv6SelectSourceAddr(&interface, &pseudoHeader.destAddr,
      &pseudoHeader.srcAddr);

   //No address assigned to the interface?
   if(error)
   {
      //Use the unspecified address if no address is assigned
      //to the sending interface
      pseudoHeader.srcAddr = IPV6_UNSPECIFIED_ADDR;
   }

   //The only defined option that may appear in a Router Solicitation
   //message is the Source Link-Layer Address option
   length = sizeof(NdpRouterSolMessage) + sizeof(NdpLinkLayerAddrOption);

   //Allocate a memory buffer to hold the Router Solicitation message
   buffer = ipAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the message
   message = netBufferAt(buffer, offset);

   //Format Router Solicitation message
   message->type = ICMPV6_TYPE_ROUTER_SOL;
   message->code = 0;
   message->checksum = 0;
   message->reserved = 0;

   //Length of the message, excluding any option
   length = sizeof(NdpRouterSolMessage);

   //The Source Link-Layer Address option must not be included
   //when the source IPv6 address is the unspecified address
   if(!ipv6CompAddr(&pseudoHeader.srcAddr, &IPV6_UNSPECIFIED_ADDR))
   {
#if (ETH_SUPPORT == ENABLED)
      NetInterface *logicalInterface;

      //Point to the logical interface
      logicalInterface = nicGetLogicalInterface(interface);

      //Check whether a MAC address has been assigned to the interface
      if(!macCompAddr(&logicalInterface->macAddr, &MAC_UNSPECIFIED_ADDR))
      {
         //Add Source Link-Layer Address option
         ndpAddOption(message, &length, NDP_OPT_SOURCE_LINK_LAYER_ADDR,
            &logicalInterface->macAddr, sizeof(MacAddr));
      }
#endif
   }

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + length);

   //Format IPv6 pseudo header
   pseudoHeader.length = htonl(length);
   pseudoHeader.reserved[0] = 0;
   pseudoHeader.reserved[1] = 0;
   pseudoHeader.reserved[2] = 0;
   pseudoHeader.nextHeader = IPV6_ICMPV6_HEADER;

   //Calculate ICMPv6 header checksum
   message->checksum = ipCalcUpperLayerChecksumEx(&pseudoHeader,
      sizeof(Ipv6PseudoHeader), buffer, offset, length);

   //Total number of ICMP messages which this entity attempted to send
   IP_MIB_INC_COUNTER32(icmpv6Stats.icmpStatsOutMsgs, 1);
   //Increment per-message type ICMP counter
   IP_MIB_INC_COUNTER32(icmpv6MsgStatsTable.icmpMsgStatsOutPkts[ICMPV6_TYPE_ROUTER_SOL], 1);

   //Debug message
   TRACE_INFO("Sending Router Solicitation message (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message contents for debugging purpose
   ndpDumpRouterSolMessage(message);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //By setting the Hop Limit to 255, Neighbor Discovery is immune to off-link
   //senders that accidentally or intentionally send NDP messages (refer to
   //RFC 4861, section 3.1)
   ancillary.ttl = NDP_HOP_LIMIT;

   //Send Router Solicitation message
   error = ipv6SendDatagram(interface, &pseudoHeader, buffer, offset,
      &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send a Neighbor Solicitation message
 * @param[in] interface Underlying network interface
 * @param[in] targetIpAddr Target IPv6 address
 * @param[in] multicast Unicast or unicast Neighbor Solicitation message
 * @return Error code
 **/

error_t ndpSendNeighborSol(NetInterface *interface,
   const Ipv6Addr *targetIpAddr, bool_t multicast)
{
   error_t error;
   size_t offset;
   size_t length;
   NetBuffer *buffer;
   NdpNeighborSolMessage *message;
   Ipv6PseudoHeader pseudoHeader;
   NetTxAncillary ancillary;

   //Multicast Neighbor Solicitation message?
   if(multicast)
   {
      //Compute the solicited-node multicast address that
      //corresponds to the target IPv6 address
      ipv6ComputeSolicitedNodeAddr(targetIpAddr, &pseudoHeader.destAddr);
   }
   else
   {
      //Unicast Neighbor Solicitation message
      pseudoHeader.destAddr = *targetIpAddr;
   }

   //Check whether the target address is a tentative address
   if(ipv6IsTentativeAddr(interface, targetIpAddr))
   {
      //The IPv6 source is set to the unspecified address
      pseudoHeader.srcAddr = IPV6_UNSPECIFIED_ADDR;
   }
   else
   {
      //Select the most appropriate source address to be used when sending
      //the Neighbor Solicitation message
      error = ipv6SelectSourceAddr(&interface, targetIpAddr,
         &pseudoHeader.srcAddr);

      //No address assigned to the interface?
      if(error)
         return error;
   }

   //The only defined option that may appear in a Neighbor Solicitation
   //message is the Source Link-Layer Address option
   length = sizeof(NdpNeighborSolMessage) + sizeof(NdpLinkLayerAddrOption);

   //Allocate a memory buffer to hold the Neighbor Solicitation message
   buffer = ipAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the message
   message = netBufferAt(buffer, offset);

   //Format Neighbor Solicitation message
   message->type = ICMPV6_TYPE_NEIGHBOR_SOL;
   message->code = 0;
   message->checksum = 0;
   message->reserved = 0;
   message->targetAddr = *targetIpAddr;

   //Length of the message, excluding any option
   length = sizeof(NdpNeighborSolMessage);

   //The Source Link-Layer Address option must not be included
   //when the source IPv6 address is the unspecified address
   if(!ipv6CompAddr(&pseudoHeader.srcAddr, &IPV6_UNSPECIFIED_ADDR))
   {
#if (ETH_SUPPORT == ENABLED)
      NetInterface *logicalInterface;

      //Point to the logical interface
      logicalInterface = nicGetLogicalInterface(interface);

      //Add Source Link-Layer Address option
      ndpAddOption(message, &length, NDP_OPT_SOURCE_LINK_LAYER_ADDR,
         &logicalInterface->macAddr, sizeof(MacAddr));
#endif
   }

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + length);

   //Format IPv6 pseudo header
   pseudoHeader.length = htonl(length);
   pseudoHeader.reserved[0] = 0;
   pseudoHeader.reserved[1] = 0;
   pseudoHeader.reserved[2] = 0;
   pseudoHeader.nextHeader = IPV6_ICMPV6_HEADER;

   //Calculate ICMPv6 header checksum
   message->checksum = ipCalcUpperLayerChecksumEx(&pseudoHeader,
      sizeof(Ipv6PseudoHeader), buffer, offset, length);

   //Total number of ICMP messages which this entity attempted to send
   IP_MIB_INC_COUNTER32(icmpv6Stats.icmpStatsOutMsgs, 1);
   //Increment per-message type ICMP counter
   IP_MIB_INC_COUNTER32(icmpv6MsgStatsTable.icmpMsgStatsOutPkts[ICMPV6_TYPE_NEIGHBOR_SOL], 1);

   //Debug message
   TRACE_INFO("Sending Neighbor Solicitation message (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message contents for debugging purpose
   ndpDumpNeighborSolMessage(message);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //By setting the Hop Limit to 255, Neighbor Discovery is immune to off-link
   //senders that accidentally or intentionally send NDP messages (refer to
   //RFC 4861, section 3.1)
   ancillary.ttl = NDP_HOP_LIMIT;

   //Send Neighbor Solicitation message
   error = ipv6SendDatagram(interface, &pseudoHeader, buffer, offset,
      &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send a Neighbor Advertisement message
 * @param[in] interface Underlying network interface
 * @param[in] targetIpAddr Target IPv6 address
 * @param[in] destIpAddr Destination IPv6 address
 * @return Error code
 **/

error_t ndpSendNeighborAdv(NetInterface *interface,
   const Ipv6Addr *targetIpAddr, const Ipv6Addr *destIpAddr)
{
   error_t error;
   size_t offset;
   size_t length;
   NetBuffer *buffer;
   NdpNeighborAdvMessage *message;
   Ipv6PseudoHeader pseudoHeader;
   NetTxAncillary ancillary;
#if (ETH_SUPPORT == ENABLED)
   NetInterface *logicalInterface;
#endif

   //Destination IP address is the unspecified address?
   if(ipv6CompAddr(destIpAddr, &IPV6_UNSPECIFIED_ADDR))
   {
      //If the destination is the unspecified address, the node must
      //multicast the advertisement to the all-nodes address
      pseudoHeader.destAddr = IPV6_LINK_LOCAL_ALL_NODES_ADDR;
   }
   else
   {
      //Otherwise, the node must unicast the advertisement to the
      //destination IP address
      pseudoHeader.destAddr = *destIpAddr;
   }

   //Check whether the target address is a valid anycast address assigned
   //to the interface
   if(ipv6IsAnycastAddr(interface, targetIpAddr))
   {
      //Select the most appropriate source address to be used when sending
      //the Neighbor Advertisement message
      error = ipv6SelectSourceAddr(&interface, targetIpAddr,
         &pseudoHeader.srcAddr);

      //No address assigned to the interface?
      if(error)
         return error;
   }
   else
   {
      //Set the source IP address
      pseudoHeader.srcAddr = *targetIpAddr;
   }

   //The only defined option that may appear in a Neighbor Advertisement
   //message is the Target Link-Layer Address option
   length = sizeof(NdpNeighborAdvMessage) + sizeof(NdpLinkLayerAddrOption);

   //Allocate a memory buffer to hold the Neighbor Advertisement message
   buffer = ipAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the message
   message = netBufferAt(buffer, offset);

   //Format Neighbor Advertisement message
   message->type = ICMPV6_TYPE_NEIGHBOR_ADV;
   message->code = 0;
   message->checksum = 0;
   message->reserved1 = 0;
   message->reserved2[0] = 0;
   message->reserved2[1] = 0;
   message->reserved2[2] = 0;
   message->targetAddr = *targetIpAddr;

   //The Router flag indicates that the sender is a router
   if(interface->ipv6Context.isRouter)
      message->r = TRUE;
   else
      message->r = FALSE;

   //If the destination is the unspecified address, the node must set
   //the Solicited flag to zero
   if(ipv6CompAddr(destIpAddr, &IPV6_UNSPECIFIED_ADDR))
      message->s = FALSE;
   else
      message->s = TRUE;

   //The Override flag should not be set in solicited advertisements
   //for anycast addresses
   if(ipv6IsAnycastAddr(interface, targetIpAddr))
      message->o = FALSE;
   else
      message->o = TRUE;

   //Length of the message, excluding any option
   length = sizeof(NdpNeighborAdvMessage);

#if (ETH_SUPPORT == ENABLED)
   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Add Target Link-Layer Address option
   ndpAddOption(message, &length, NDP_OPT_TARGET_LINK_LAYER_ADDR,
      &logicalInterface->macAddr, sizeof(MacAddr));
#endif

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + length);

   //Format IPv6 pseudo header
   pseudoHeader.length = htonl(length);
   pseudoHeader.reserved[0] = 0;
   pseudoHeader.reserved[1] = 0;
   pseudoHeader.reserved[2] = 0;
   pseudoHeader.nextHeader = IPV6_ICMPV6_HEADER;

   //Calculate ICMPv6 header checksum
   message->checksum = ipCalcUpperLayerChecksumEx(&pseudoHeader,
      sizeof(Ipv6PseudoHeader), buffer, offset, length);

   //Total number of ICMP messages which this entity attempted to send
   IP_MIB_INC_COUNTER32(icmpv6Stats.icmpStatsOutMsgs, 1);
   //Increment per-message type ICMP counter
   IP_MIB_INC_COUNTER32(icmpv6MsgStatsTable.icmpMsgStatsOutPkts[ICMPV6_TYPE_NEIGHBOR_ADV], 1);

   //Debug message
   TRACE_INFO("Sending Neighbor Advertisement message (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message contents for debugging purpose
   ndpDumpNeighborAdvMessage(message);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //By setting the Hop Limit to 255, Neighbor Discovery is immune to off-link
   //senders that accidentally or intentionally send NDP messages (refer to
   //RFC 4861, section 3.1)
   ancillary.ttl = NDP_HOP_LIMIT;

   //Send Neighbor Advertisement message
   error = ipv6SendDatagram(interface, &pseudoHeader, buffer, offset,
      &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send a Redirect message
 * @param[in] interface Underlying network interface
 * @param[in] targetAddr IPv6 address that is a better first hop to use
 *   for the destination address
 * @param[in] ipPacket Multi-part buffer that holds the IPv6 packet that
 *   triggered the sending of the Redirect
 * @param[in] ipPacketOffset Offset to the first byte of the IPv6 packet
 * @return Error code
 **/

error_t ndpSendRedirect(NetInterface *interface, const Ipv6Addr *targetAddr,
   const NetBuffer *ipPacket, size_t ipPacketOffset)
{
   error_t error;
   size_t offset;
   size_t length;
   size_t ipPacketLen;
   size_t optionLen;
   size_t paddingLen;
   NetBuffer *buffer;
   NdpRedirectMessage *message;
   NdpRedirectedHeaderOption *option;
   NdpNeighborCacheEntry *entry;
   Ipv6Header *ipHeader;
   Ipv6PseudoHeader pseudoHeader;
   NetTxAncillary ancillary;
   uint8_t padding[8];

   //Retrieve the length of the forwarded IPv6 packet
   ipPacketLen = netBufferGetLength(ipPacket) - ipPacketOffset;

   //Check the length of the IPv6 packet
   if(ipPacketLen < sizeof(Ipv6Header))
      return ERROR_INVALID_LENGTH;

   //Point to the header of the invoking packet
   ipHeader = netBufferAt(ipPacket, ipPacketOffset);
   //Sanity check
   if(ipHeader == NULL)
      return ERROR_FAILURE;

   //The only defined options that may appear in a Redirect message are the
   //Target Link-Layer Address option and the Redirected Header option
   length = sizeof(NdpRedirectMessage) + sizeof(NdpLinkLayerAddrOption) +
      sizeof(NdpRedirectedHeaderOption);

   //Allocate a memory buffer to hold the Redirect message
   buffer = ipAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the message
   message = netBufferAt(buffer, offset);

   //Format Redirect message
   message->type = ICMPV6_TYPE_REDIRECT;
   message->code = 0;
   message->checksum = 0;
   message->reserved = 0;
   message->targetAddr = *targetAddr;
   message->destAddr = ipHeader->destAddr;

   //Length of the message, excluding any option
   length = sizeof(NdpRedirectMessage);

   //Search the Neighbor cache for the specified target address
   entry = ndpFindNeighborCacheEntry(interface, targetAddr);

   //Include the link-layer address of the target, if known
   if(entry != NULL)
   {
      //Add Target Link-Layer Address option
      ndpAddOption(message, &length, NDP_OPT_TARGET_LINK_LAYER_ADDR,
         &entry->macAddr, sizeof(MacAddr));
   }

   //Retrieve the length of the IPv6 packet that triggered the sending
   //of the Redirect
   ipPacketLen = netBufferGetLength(ipPacket) - ipPacketOffset;

   //Return as much of the forwarded IPv6 packet as can fit without the
   //redirect packet exceeding the minimum IPv6 MTU
   ipPacketLen = MIN(ipPacketLen, IPV6_DEFAULT_MTU -
      sizeof(NdpRedirectedHeaderOption) - length);

   //Length of the Redirected Header option in units of 8 bytes including
   //the type and length fields
   optionLen = (ipPacketLen + sizeof(NdpOption) + 7) / 8;

   //Add Redirected Header option
   option = (NdpRedirectedHeaderOption *) ((uint8_t *) message + length);

   //Format Redirected Header option
   option->type = NDP_OPT_REDIRECTED_HEADER;
   option->length = (uint8_t) optionLen;
   option->reserved1 = 0;
   option->reserved2 = 0;

   //Update the length of Redirect message
   length += sizeof(NdpRedirectedHeaderOption);

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + length);

   //Copy the contents of the forwarded IPv6 packet
   error = netBufferConcat(buffer, ipPacket, ipPacketOffset, ipPacketLen);

   //Check status code
   if(!error)
   {
      //Options should be padded when necessary to ensure that they end on
      //their natural 64-bit boundaries
      if((ipPacketLen + sizeof(NdpRedirectedHeaderOption)) < (optionLen * 8))
      {
         //Determine the amount of padding data to append
         paddingLen = (optionLen * 8) - ipPacketLen -
            sizeof(NdpRedirectedHeaderOption);

         //Prepare padding data
         osMemset(padding, 0, paddingLen);
         //Append padding bytes
         error = netBufferAppend(buffer, padding, paddingLen);
      }
   }

   //Check status code
   if(!error)
   {
      //Get the length of the resulting message
      length = netBufferGetLength(buffer) - offset;

      //Format IPv6 pseudo header
      pseudoHeader.srcAddr = interface->ipv6Context.addrList[0].addr;
      pseudoHeader.destAddr = ipHeader->srcAddr;
      pseudoHeader.length = htonl(length);
      pseudoHeader.reserved[0] = 0;
      pseudoHeader.reserved[1] = 0;
      pseudoHeader.reserved[2] = 0;
      pseudoHeader.nextHeader = IPV6_ICMPV6_HEADER;

      //Message checksum calculation
      message->checksum = ipCalcUpperLayerChecksumEx(&pseudoHeader,
         sizeof(Ipv6PseudoHeader), buffer, offset, length);

      //Total number of ICMP messages which this entity attempted to send
      IP_MIB_INC_COUNTER32(icmpv6Stats.icmpStatsOutMsgs, 1);
      //Increment per-message type ICMP counter
      IP_MIB_INC_COUNTER32(icmpv6MsgStatsTable.icmpMsgStatsOutPkts[ICMPV6_TYPE_REDIRECT], 1);

      //Debug message
      TRACE_INFO("Sending Redirect message (%" PRIuSIZE " bytes)...\r\n", length);
      //Dump message contents for debugging purpose
      ndpDumpRedirectMessage(message);

      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_TX_ANCILLARY;

      //By setting the Hop Limit to 255, Neighbor Discovery is immune to off-link
      //senders that accidentally or intentionally send NDP messages (refer to
      //RFC 4861, section 3.1)
      ancillary.ttl = NDP_HOP_LIMIT;

      //Send Redirect message
      error = ipv6SendDatagram(interface, &pseudoHeader, buffer, offset,
         &ancillary);
   }

   //Free previously allocated memory
   netBufferFree(buffer);

   //Return status code
   return error;
}


/**
 * @brief Dump Router Solicitation message for debugging purpose
 * @param[in] message Router Solicitation message
 **/

void ndpDumpRouterSolMessage(const NdpRouterSolMessage *message)
{
   //Dump Router Solicitation message
   TRACE_DEBUG("  Type = %" PRIu8 "\r\n", message->type);
   TRACE_DEBUG("  Code = %" PRIu8 "\r\n", message->code);
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(message->checksum));
}


/**
 * @brief Dump Router Advertisement message for debugging purpose
 * @param[in] message Router Advertisement message
 **/

void ndpDumpRouterAdvMessage(const NdpRouterAdvMessage *message)
{
   //Dump Router Advertisement message
   TRACE_DEBUG("  Type = %" PRIu8 "\r\n", message->type);
   TRACE_DEBUG("  Code = %" PRIu8 "\r\n", message->code);
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(message->checksum));
   TRACE_DEBUG("  Cur Hop Limit = %" PRIu8 "\r\n", message->curHopLimit);
   TRACE_DEBUG("  M = %" PRIu8 "\r\n", message->m);
   TRACE_DEBUG("  O = %" PRIu8 "\r\n", message->o);
   TRACE_DEBUG("  Router Lifetime = %" PRIu16 "\r\n", ntohs(message->routerLifetime));
   TRACE_DEBUG("  Reachable Time = %" PRIu32 "\r\n", ntohl(message->reachableTime));
   TRACE_DEBUG("  Retrans Timer = %" PRIu32 "\r\n", ntohl(message->retransTimer));
}


/**
 * @brief Dump Neighbor Solicitation message for debugging purpose
 * @param[in] message Neighbor Solicitation message
 **/

void ndpDumpNeighborSolMessage(const NdpNeighborSolMessage *message)
{
   //Dump Neighbor Solicitation message
   TRACE_DEBUG("  Type = %" PRIu8 "\r\n", message->type);
   TRACE_DEBUG("  Code = %" PRIu8 "\r\n", message->code);
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(message->checksum));
   TRACE_DEBUG("  Target Address = %s\r\n", ipv6AddrToString(&message->targetAddr, NULL));
}


/**
 * @brief Dump Neighbor Advertisement message for debugging purpose
 * @param[in] message Neighbor Advertisement message
 **/

void ndpDumpNeighborAdvMessage(const NdpNeighborAdvMessage *message)
{
   //Dump Neighbor Advertisement message
   TRACE_DEBUG("  Type = %" PRIu8 "\r\n", message->type);
   TRACE_DEBUG("  Code = %" PRIu8 "\r\n", message->code);
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(message->checksum));
   TRACE_DEBUG("  R = %" PRIu8 "\r\n", message->r);
   TRACE_DEBUG("  S = %" PRIu8 "\r\n", message->s);
   TRACE_DEBUG("  O = %" PRIu8 "\r\n", message->o);
   TRACE_DEBUG("  Target Address = %s\r\n", ipv6AddrToString(&message->targetAddr, NULL));
}


/**
 * @brief Dump Redirect message for debugging purpose
 * @param[in] message Redirect message
 **/

void ndpDumpRedirectMessage(const NdpRedirectMessage *message)
{
   //Dump Neighbor Advertisement message
   TRACE_DEBUG("  Type = %" PRIu8 "\r\n", message->type);
   TRACE_DEBUG("  Code = %" PRIu8 "\r\n", message->code);
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(message->checksum));
   TRACE_DEBUG("  Target Address = %s\r\n", ipv6AddrToString(&message->targetAddr, NULL));
   TRACE_DEBUG("  Destination Address = %s\r\n", ipv6AddrToString(&message->destAddr, NULL));
}

#endif
