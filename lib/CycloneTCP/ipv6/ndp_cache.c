/**
 * @file ndp_cache.c
 * @brief Neighbor and destination cache management
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
#define TRACE_LEVEL NDP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "ipv6/ipv6_misc.h"
#include "ipv6/icmpv6.h"
#include "ipv6/ndp.h"
#include "ipv6/ndp_cache.h"
#include "ipv6/ndp_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED && NDP_SUPPORT == ENABLED)


/**
 * @brief Create a new entry in the Neighbor cache
 * @param[in] interface Underlying network interface
 * @return Pointer to the newly created entry
 **/

NdpNeighborCacheEntry *ndpCreateNeighborCacheEntry(NetInterface *interface)
{
   uint_t i;
   systime_t time;
   NdpNeighborCacheEntry *entry;
   NdpNeighborCacheEntry *oldestEntry;

   //Get current time
   time = osGetSystemTime();

   //Keep track of the oldest entry
   oldestEntry = &interface->ndpContext.neighborCache[0];

   //Loop through Neighbor cache entries
   for(i = 0; i < NDP_NEIGHBOR_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ndpContext.neighborCache[i];

      //Check whether the entry is currently in use or not
      if(entry->state == NDP_STATE_NONE)
      {
         //Erase contents
         osMemset(entry, 0, sizeof(NdpNeighborCacheEntry));
         //Return a pointer to the Neighbor cache entry
         return entry;
      }

      //Keep track of the oldest entry in the table
      if((time - entry->timestamp) > (time - oldestEntry->timestamp))
      {
         oldestEntry = entry;
      }
   }

   //Drop any pending packets
   ndpFlushQueuedPackets(interface, oldestEntry);
   //The oldest entry is removed whenever the table runs out of space
   osMemset(oldestEntry, 0, sizeof(NdpNeighborCacheEntry));

   //Return a pointer to the Neighbor cache entry
   return oldestEntry;
}


/**
 * @brief Search the Neighbor cache for a given IPv6 address
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr IPv6 address
 * @return A pointer to the matching entry is returned. NULL is returned if
 *   the specified IPv6 address could not be found in the Neighbor cache
 **/

NdpNeighborCacheEntry *ndpFindNeighborCacheEntry(NetInterface *interface, const Ipv6Addr *ipAddr)
{
   uint_t i;
   NdpNeighborCacheEntry *entry;

   //Loop through Neighbor cache entries
   for(i = 0; i < NDP_NEIGHBOR_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ndpContext.neighborCache[i];

      //Check whether the entry is currently in use
      if(entry->state != NDP_STATE_NONE)
      {
         //Current entry matches the specified address?
         if(ipv6CompAddr(&entry->ipAddr, ipAddr))
            return entry;
      }
   }

   //No matching entry in Neighbor cache...
   return NULL;
}


/**
 * @brief Periodically update Neighbor cache
 * @param[in] interface Underlying network interface
 **/

void ndpUpdateNeighborCache(NetInterface *interface)
{
   uint_t i;
   systime_t time;
   NdpNeighborCacheEntry *entry;

   //Get current time
   time = osGetSystemTime();

   //Go through Neighbor cache
   for(i = 0; i < NDP_NEIGHBOR_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ndpContext.neighborCache[i];

      //INCOMPLETE state?
      if(entry->state == NDP_STATE_INCOMPLETE)
      {
         //The Neighbor Solicitation timed out?
         if(timeCompare(time, entry->timestamp + entry->timeout) >= 0)
         {
            //Increment retransmission counter
            entry->retransmitCount++;

            //Check whether the maximum number of retransmissions has been exceeded
            if(entry->retransmitCount < NDP_MAX_MULTICAST_SOLICIT)
            {
               //Retransmit the multicast Neighbor Solicitation message
               ndpSendNeighborSol(interface, &entry->ipAddr, TRUE);

               //Save the time at which the message was sent
               entry->timestamp = time;
               //Set timeout value
               entry->timeout = interface->ndpContext.retransTimer;
            }
            else
            {
               //Drop packets that are waiting for address resolution
               ndpFlushQueuedPackets(interface, entry);
               //The entry should be deleted since address resolution has failed
               entry->state = NDP_STATE_NONE;
            }
         }
      }
      //REACHABLE state?
      else if(entry->state == NDP_STATE_REACHABLE)
      {
         //Periodically time out Neighbor cache entries
         if(timeCompare(time, entry->timestamp + entry->timeout) >= 0)
         {
            //Save current time
            entry->timestamp = osGetSystemTime();
            //Enter STALE state
            entry->state = NDP_STATE_STALE;
         }
      }
      //STALE state?
      else if(entry->state == NDP_STATE_STALE)
      {
         //The neighbor is no longer known to be reachable but until traffic
         //is sent to the neighbor, no attempt should be made to verify its
         //reachability
      }
      //DELAY state?
      else if(entry->state == NDP_STATE_DELAY)
      {
         //Wait for the specified delay before sending the first probe
         if(timeCompare(time, entry->timestamp + entry->timeout) >= 0)
         {
            Ipv6Addr ipAddr;

            //Save the time at which the message was sent
            entry->timestamp = time;
            //Set timeout value
            entry->timeout = interface->ndpContext.retransTimer;
            //Switch to the PROBE state
            entry->state = NDP_STATE_PROBE;

            //Target address
            ipAddr = entry->ipAddr;

            //Send a unicast Neighbor Solicitation message
            ndpSendNeighborSol(interface, &ipAddr, FALSE);
         }
      }
      //PROBE state?
      else if(entry->state == NDP_STATE_PROBE)
      {
         //The request timed out?
         if(timeCompare(time, entry->timestamp + entry->timeout) >= 0)
         {
            //Increment retransmission counter
            entry->retransmitCount++;

            //Check whether the maximum number of retransmissions has been exceeded
            if(entry->retransmitCount < NDP_MAX_UNICAST_SOLICIT)
            {
               Ipv6Addr ipAddr;

               //Save the time at which the packet was sent
               entry->timestamp = time;
               //Set timeout value
               entry->timeout = interface->ndpContext.retransTimer;

               //Target address
               ipAddr = entry->ipAddr;

               //Send a unicast Neighbor Solicitation message
               ndpSendNeighborSol(interface, &ipAddr, FALSE);
            }
            else
            {
               //The entry should be deleted since the host is not reachable anymore
               entry->state = NDP_STATE_NONE;

               //If at some point communication ceases to proceed, as determined
               //by the Neighbor Unreachability Detection algorithm, next-hop
               //determination may need to be performed again...
               ndpUpdateNextHop(interface, &entry->ipAddr);
            }
         }
      }
   }
}


/**
 * @brief Flush Neighbor cache
 * @param[in] interface Underlying network interface
 **/

void ndpFlushNeighborCache(NetInterface *interface)
{
   uint_t i;
   NdpNeighborCacheEntry *entry;

   //Loop through Neighbor cache entries
   for(i = 0; i < NDP_NEIGHBOR_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ndpContext.neighborCache[i];

      //Drop packets that are waiting for address resolution
      ndpFlushQueuedPackets(interface, entry);
      //Release Neighbor cache entry
      entry->state = NDP_STATE_NONE;
   }
}


/**
 * @brief Send packets that are waiting for address resolution
 * @param[in] interface Underlying network interface
 * @param[in] entry Pointer to a Neighbor cache entry
 * @return The number of packets that have been sent
 **/

uint_t ndpSendQueuedPackets(NetInterface *interface, NdpNeighborCacheEntry *entry)
{
   uint_t i;
   NdpQueueItem *item;
#if (ETH_SUPPORT == ENABLED)
   NetInterface *physicalInterface;

   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);
#endif

   //Reset packet counter
   i = 0;

   //Check current state
   if(entry->state == NDP_STATE_INCOMPLETE)
   {
      //Loop through the queued packets
      for(i = 0; i < entry->queueSize; i++)
      {
         //Point to the current queue item
         item = &entry->queue[i];

#if (ETH_SUPPORT == ENABLED)
         //Ethernet interface?
         if(physicalInterface->nicDriver != NULL &&
            physicalInterface->nicDriver->type == NIC_TYPE_ETHERNET)
         {
            size_t length;

            //Retrieve the length of the IPv6 packet
            length = netBufferGetLength(item->buffer) - item->offset;
            //Update IP statistics
            ipv6UpdateOutStats(interface, &entry->ipAddr, length);

            //Send the IPv6 packet
            ethSendFrame(interface, &entry->macAddr, ETH_TYPE_IPV6,
               item->buffer, item->offset, &item->ancillary);
         }
#endif
         //Release memory buffer
         netBufferFree(item->buffer);
      }
   }

   //The queue is now empty
   entry->queueSize = 0;

   //Return the number of packets that have been sent
   return i;
}


/**
 * @brief Flush packet queue
 * @param[in] interface Underlying network interface
 * @param[in] entry Pointer to a Neighbor cache entry
 **/

void ndpFlushQueuedPackets(NetInterface *interface, NdpNeighborCacheEntry *entry)
{
   uint_t i;
   NdpQueueItem *item;

   //Check current state
   if(entry->state == NDP_STATE_INCOMPLETE)
   {
      //Loop through the queued packets
      for(i = 0; i < entry->queueSize; i++)
      {
         //Point to the current queue item
         item = &entry->queue[i];

         //Check whether the address resolution has failed
         if(entry->retransmitCount >= NDP_MAX_MULTICAST_SOLICIT)
         {
            //Check whether the packet has been forwarded
            if(item->srcInterface != NULL)
            {
               //A Destination Unreachable message should be generated by a
               //router in response to a packet that cannot be delivered
               icmpv6SendErrorMessage(item->srcInterface, ICMPV6_TYPE_DEST_UNREACHABLE,
                  ICMPV6_CODE_ADDR_UNREACHABLE, 0, item->buffer, item->offset);
            }
         }

         //Release memory buffer
         netBufferFree(item->buffer);
      }
   }

   //The queue is now empty
   entry->queueSize = 0;
}


/**
 * @brief Create a new entry in the Destination Cache
 * @param[in] interface Underlying network interface
 * @return Pointer to the newly created entry
 **/

NdpDestCacheEntry *ndpCreateDestCacheEntry(NetInterface *interface)
{
   uint_t i;
   systime_t time;
   NdpDestCacheEntry *entry;
   NdpDestCacheEntry *oldestEntry;

   //Get current time
   time = osGetSystemTime();

   //Keep track of the oldest entry
   oldestEntry = &interface->ndpContext.destCache[0];

   //Loop through Destination cache entries
   for(i = 0; i < NDP_DEST_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ndpContext.destCache[i];

      //Check whether the entry is currently in use or not
      if(ipv6CompAddr(&entry->destAddr, &IPV6_UNSPECIFIED_ADDR))
      {
         //Erase contents
         osMemset(entry, 0, sizeof(NdpDestCacheEntry));
         //Return a pointer to the Destination cache entry
         return entry;
      }

      //Keep track of the oldest entry in the table
      if((time - entry->timestamp) > (time - oldestEntry->timestamp))
      {
         oldestEntry = entry;
      }
   }

   //The oldest entry is removed whenever the table runs out of space
   osMemset(oldestEntry, 0, sizeof(NdpDestCacheEntry));

   //Return a pointer to the Destination cache entry
   return oldestEntry;
}


/**
 * @brief Search the Destination Cache for a given destination address
 * @param[in] interface Underlying network interface
 * @param[in] destAddr Destination IPv6 address
 * @return A pointer to the matching entry is returned. NULL is returned if
 *   the specified address could not be found in the Destination cache
 **/

NdpDestCacheEntry *ndpFindDestCacheEntry(NetInterface *interface, const Ipv6Addr *destAddr)
{
   uint_t i;
   NdpDestCacheEntry *entry;

   //Loop through Destination Cache entries
   for(i = 0; i < NDP_DEST_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ndpContext.destCache[i];

      //Current entry matches the specified destination address?
      if(ipv6CompAddr(&entry->destAddr, destAddr))
         return entry;
   }

   //No matching entry in Destination Cache...
   return NULL;
}


/**
 * @brief Flush Destination Cache
 * @param[in] interface Underlying network interface
 **/

void ndpFlushDestCache(NetInterface *interface)
{
   //Clear the Destination Cache
   osMemset(interface->ndpContext.destCache, 0,
      sizeof(interface->ndpContext.destCache));
}

#endif
