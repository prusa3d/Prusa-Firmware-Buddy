/**
 * @file ndp_misc.c
 * @brief Helper functions for NDP (Neighbor Discovery Protocol)
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
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "ipv6/ndp.h"
#include "ipv6/ndp_cache.h"
#include "ipv6/ndp_misc.h"
#include "mdns/mdns_responder.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED && NDP_SUPPORT == ENABLED)


/**
 * @brief Parse Prefix Information Option
 * @param[in] interface Underlying network interface
 * @param[in] option Pointer to the Prefix Information option
 **/

void ndpParsePrefixInfoOption(NetInterface *interface, NdpPrefixInfoOption *option)
{
   //Make sure the Prefix Information option is valid
   if(option == NULL || option->length != 4)
      return;

   //A prefix Information option that have the on-link flag set indicates a
   //prefix identifying a range of addresses that should be considered on-link
   if(!option->l)
      return;

   //If the prefix is the link-local prefix, silently ignore the
   //Prefix Information option
   if(ipv6CompPrefix(&option->prefix, &IPV6_LINK_LOCAL_ADDR_PREFIX, 10))
      return;

   //If the preferred lifetime is greater than the valid lifetime,
   //silently ignore the Prefix Information option
   if(ntohl(option->preferredLifetime) > ntohl(option->validLifetime))
      return;

   //Check whether the Valid Lifetime field is non-zero
   if(ntohl(option->validLifetime) != 0)
   {
      //If the prefix is not already present in the Prefix List, create a new
      //entry for the prefix. If the prefix is already present in the list,
      //reset its invalidation timer
      ipv6AddPrefix(interface, &option->prefix, option->prefixLength, option->l,
         option->a, ntohl(option->validLifetime), ntohl(option->preferredLifetime));
   }
   else
   {
      //If the new Lifetime value is zero, time-out the prefix immediately
      ipv6RemovePrefix(interface, &option->prefix, option->prefixLength);
   }
}


/**
 * @brief Manage the lifetime of IPv6 addresses
 * @param[in] interface Underlying network interface
 **/

void ndpUpdateAddrList(NetInterface *interface)
{
   uint_t i;
   systime_t time;
   Ipv6AddrEntry *entry;
   NdpContext *context;

   //Point to the NDP context
   context = &interface->ndpContext;

   //Get current time
   time = osGetSystemTime();

   //Go through the list of IPv6 addresses
   for(i = 0; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.addrList[i];

      //Tentative address?
      if(entry->state == IPV6_ADDR_STATE_TENTATIVE)
      {
         //Check whether the link is up
         if(interface->linkState)
         {
            //To check an address, a node should send Neighbor Solicitation messages
            if(entry->dadRetransmitCount == 0)
            {
               //Set time stamp
               entry->timestamp = time;

               //Check whether Duplicate Address Detection should be performed
               if(context->dupAddrDetectTransmits > 0)
               {
                  //Link-local address?
                  if(i == 0)
                  {
                     //Delay before transmitting the first solicitation
                     entry->dadTimeout = netGetRandRange(0, NDP_MAX_RTR_SOLICITATION_DELAY);
                     //Prepare to send the first Neighbor Solicitation message
                     entry->dadRetransmitCount = 1;
                  }
                  else
                  {
                     //Valid link-local address?
                     if(ipv6GetLinkLocalAddrState(interface) == IPV6_ADDR_STATE_PREFERRED)
                     {
                        //Prepare to send the first Neighbor Solicitation message
                        entry->dadTimeout = 0;
                        entry->dadRetransmitCount = 1;
                     }
                  }
               }
               else
               {
                  //Do not perform Duplicate Address Detection
                  entry->state = IPV6_ADDR_STATE_PREFERRED;
               }
            }
            else
            {
               //Check current time
               if(timeCompare(time, entry->timestamp + entry->dadTimeout) >= 0)
               {
                  //Duplicate Address Detection failed?
                  if(entry->duplicate)
                  {
                     //A tentative address that is determined to be a duplicate
                     //must not be assigned to an interface
                     if(entry->permanent)
                     {
                        //The IPv6 address should be preserved if it has been
                        //manually assigned
                        ipv6SetAddr(interface, i, &entry->addr,
                           IPV6_ADDR_STATE_INVALID, 0, 0, TRUE);
                     }
                     else
                     {
                        //The IPv6 address is no more valid and should be
                        //removed from the list
                        ipv6SetAddr(interface, i, &IPV6_UNSPECIFIED_ADDR,
                           IPV6_ADDR_STATE_INVALID, 0, 0, FALSE);
                     }
                  }
                  //Duplicate Address Detection is on-going?
                  else if(entry->dadRetransmitCount <= context->dupAddrDetectTransmits)
                  {
                     //Send a multicast Neighbor Solicitation message
                     ndpSendNeighborSol(interface, &entry->addr, TRUE);

                     //Set timeout value
                     entry->dadTimeout += context->retransTimer;
                     //Increment retransmission counter
                     entry->dadRetransmitCount++;
                  }
                  //Duplicate Address Detection is complete?
                  else
                  {
                     //The use of the IPv6 address is now unrestricted
                     entry->state = IPV6_ADDR_STATE_PREFERRED;

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
                     //Link-local address?
                     if(i == 0)
                     {
                        //Restart mDNS probing process
                        mdnsResponderStartProbing(interface->mdnsResponderContext);
                     }
#endif
                  }
               }
            }
         }
      }
      //Preferred address?
      else if(entry->state == IPV6_ADDR_STATE_PREFERRED)
      {
         //An IPv6 address with an infinite preferred lifetime is never timed out
         if(entry->preferredLifetime != NDP_INFINITE_LIFETIME)
         {
            //When the preferred lifetime expires, the address becomes deprecated
            if(timeCompare(time, entry->timestamp + entry->preferredLifetime) >= 0)
            {
               //A deprecated address should continue to be used as a source
               //address in existing communications, but should not be used
               //to initiate new communications
               entry->state = IPV6_ADDR_STATE_DEPRECATED;
            }
         }
      }
      //Deprecated address?
      else if(entry->state == IPV6_ADDR_STATE_DEPRECATED)
      {
         //An IPv6 address with an infinite valid lifetime is never timed out
         if(entry->validLifetime != NDP_INFINITE_LIFETIME)
         {
            //When the valid lifetime expires, the address becomes invalid
            if(timeCompare(time, entry->timestamp + entry->validLifetime) >= 0)
            {
               //The IPv6 address is no more valid and should be removed from the list
               ipv6SetAddr(interface, i, &IPV6_UNSPECIFIED_ADDR,
                  IPV6_ADDR_STATE_INVALID, 0, 0, FALSE);
            }
         }
      }
   }
}


/**
 * @brief Periodically update Prefix List
 * @param[in] interface Underlying network interface
 **/

void ndpUpdatePrefixList(NetInterface *interface)
{
   uint_t i;
   systime_t time;
   Ipv6PrefixEntry *entry;

   //Get current time
   time = osGetSystemTime();

   //Go through the Prefix List
   for(i = 0; i < IPV6_PREFIX_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.prefixList[i];

      //Check the lifetime value
      if(entry->validLifetime > 0 && entry->validLifetime < INFINITE_DELAY)
      {
         //A node should retain entries in the Prefix List until their
         //lifetimes expire
         if(timeCompare(time, entry->timestamp + entry->validLifetime) >= 0)
         {
            //When removing an entry from the Prefix List, there is no need
            //to purge any entries from the Destination or Neighbor Caches
            ipv6RemovePrefix(interface, &entry->prefix, entry->prefixLen);
         }
      }
   }
}


/**
 * @brief Periodically update Default Router List
 * @param[in] interface Underlying network interface
 **/

void ndpUpdateDefaultRouterList(NetInterface *interface)
{
   uint_t i;
   bool_t flag;
   systime_t time;
   Ipv6RouterEntry *entry;

   //This flag will be set if any entry has been removed from
   //the Default Router List
   flag = FALSE;

   //Get current time
   time = osGetSystemTime();

   //Go through the Default Router List
   for(i = 0; i < IPV6_ROUTER_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.routerList[i];

      //Check the lifetime value
      if(entry->lifetime > 0 && entry->lifetime < INFINITE_DELAY)
      {
         //A node should retain entries in the Default Router List until
         //their lifetimes expire
         if(timeCompare(time, entry->timestamp + entry->lifetime) >= 0)
         {
            //Immediately time-out the entry
            entry->addr = IPV6_UNSPECIFIED_ADDR;
            entry->lifetime = 0;

            //Set flag
            flag = TRUE;
         }
      }
   }

   //Check whether an entry has been removed from the list
   if(flag)
   {
      //When removing an entry from the Default Router List, any entries
      //in the Destination Cache that go through that router must perform
      //next-hop determination again to select a new default router
      ndpFlushDestCache(interface);
   }
}


/**
 * @brief Default Router Selection
 * @param[in] interface Underlying network interface
 * @param[in] unreachableAddr IPv6 address of the unreachable router (optional parameter)
 * @param[out] addr IPv6 address of the default router to be used
 * @return Error code
 **/

error_t ndpSelectDefaultRouter(NetInterface *interface,
   const Ipv6Addr *unreachableAddr, Ipv6Addr *addr)
{
   uint_t i;
   uint_t j;
   uint_t k;
   Ipv6RouterEntry *routerEntry;
   NdpNeighborCacheEntry *neighborCacheEntry;

   //Initialize index
   i = 0;

   //This parameter is optional...
   if(unreachableAddr != NULL)
   {
      //Search the Default Router List for the router whose reachability is suspect
      for(j = 0; j < IPV6_ROUTER_LIST_SIZE; j++)
      {
         //Point to the current entry
         routerEntry = &interface->ipv6Context.routerList[j];

         //Check the lifetime associated with the default router
         if(routerEntry->lifetime)
         {
            //Check the router address against the address whose reachability is suspect
            if(ipv6CompAddr(&routerEntry->addr, unreachableAddr))
            {
               //Routers should be selected in a round-robin fashion
               i = j + 1;
               //We are done
               break;
            }
         }
      }
   }

   //Routers that are reachable or probably reachable should be preferred
   //over routers whose reachability is unknown or suspect
   for(j = 0; j < IPV6_ROUTER_LIST_SIZE; j++)
   {
      //Get current index
      k = (i + j) % IPV6_ROUTER_LIST_SIZE;

      //Point to the corresponding entry
      routerEntry = &interface->ipv6Context.routerList[k];

      //Check the lifetime associated with the default router
      if(routerEntry->lifetime)
      {
         //Search the Neighbor Cache for the router address
         neighborCacheEntry = ndpFindNeighborCacheEntry(interface, &routerEntry->addr);

         //Check whether the router is reachable or probably reachable
         if(neighborCacheEntry != NULL)
         {
            //Any state other than INCOMPLETE?
            if(neighborCacheEntry->state != NDP_STATE_INCOMPLETE)
            {
               //Return the IPv6 address of the default router
               *addr = routerEntry->addr;
               //Successful default router selection
               return NO_ERROR;
            }
         }
      }
   }

   //When no routers on the list are known to be reachable or probably
   //reachable, routers should be selected in a round-robin fashion, so
   //that subsequent requests for a default router do not return the
   //same router until all other routers have been selected
   for(j = 0; j < IPV6_ROUTER_LIST_SIZE; j++)
   {
      //Get current index
      k = (i + j) % IPV6_ROUTER_LIST_SIZE;

      //Point to the corresponding entry
      routerEntry = &interface->ipv6Context.routerList[k];

      //Check the lifetime associated with the default router
      if(routerEntry->lifetime)
      {
         //Return the IPv6 address of the default router
         *addr = routerEntry->addr;
         //Successful default router selection
         return NO_ERROR;
      }
   }

   //No default router found
   return ERROR_NO_ROUTE;
}


/**
 * @brief Check whether an address is the first-hop router for the specified destination
 * @param[in] interface Underlying network interface
 * @param[in] destAddr Destination address
 * @param[in] nextHop First-hop address to be checked
 * @return TRUE if the address is the first-hop router, else FALSE
 **/

bool_t ndpIsFirstHopRouter(NetInterface *interface,
   const Ipv6Addr *destAddr, const Ipv6Addr *nextHop)
{
   uint_t i;
   bool_t isFirstHopRouter;
   Ipv6RouterEntry *routerEntry;
   NdpDestCacheEntry *destCacheEntry;

   //Clear flag
   isFirstHopRouter = FALSE;

   //Search the cache for the specified destination address
   destCacheEntry = ndpFindDestCacheEntry(interface, destAddr);

   //Any matching entry?
   if(destCacheEntry != NULL)
   {
      //Check if the address is the same as the current first-hop
      //router for the specified destination
      if(ipv6CompAddr(&destCacheEntry->nextHop, nextHop))
         isFirstHopRouter = TRUE;
   }
   else
   {
      //Loop through the Default Router List
      for(i = 0; i < IPV6_ROUTER_LIST_SIZE; i++)
      {
         //Point to the current entry
         routerEntry = &interface->ipv6Context.routerList[i];

         //Check the lifetime associated with the default router
         if(routerEntry->lifetime)
         {
            //Check whether the current entry matches the specified address
            if(ipv6CompAddr(&routerEntry->addr, nextHop))
            {
               //The specified address is a valid first-hop router
               isFirstHopRouter = TRUE;
               //We are done
               break;
            }
         }
      }
   }

   //Return TRUE if the address is the same as the current first-hop
   //router for the specified destination
   return isFirstHopRouter;
}


/**
 * @brief Next-hop determination
 * @param[in] interface Underlying network interface
 * @param[in] destAddr Destination address
 * @param[in] unreachableNextHop Address of the unreachable next-hop (optional
 *   parameter)
 * @param[out] nextHop Next-hop address to be used
 * @param[in] dontRoute Do not send the packet via a router
 * @return Error code
 **/

error_t ndpSelectNextHop(NetInterface *interface, const Ipv6Addr *destAddr,
   const Ipv6Addr *unreachableNextHop, Ipv6Addr *nextHop, bool_t dontRoute)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

   //Destination IPv6 address is a multicast address?
   if(ipv6IsMulticastAddr(destAddr))
   {
      //For multicast packets, the next-hop is always the multicast destination
      //address and is considered to be on-link
      *nextHop = *destAddr;
   }
   else
   {
      //The sender performs a longest prefix match against the Prefix List to
      //determine whether the packet's destination is on-link or off-link
      if(ipv6IsOnLink(interface, destAddr))
      {
         //If the destination is on-link, the next-hop address is the same as
         //the packet's destination address
         *nextHop = *destAddr;
      }
      else if(dontRoute)
      {
         //Do not send the packet via a router
         *nextHop = *destAddr;
      }
      else
      {
         //If the destination is off-link, the sender selects a router from
         //the Default Router List
         error = ndpSelectDefaultRouter(interface, unreachableNextHop, nextHop);
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Update next-hop field of Destination Cache entries
 * @param[in] interface Underlying network interface
 * @param[in] unreachableNextHop Address of the unreachable next-hop
 **/

void ndpUpdateNextHop(NetInterface *interface, const Ipv6Addr *unreachableNextHop)
{
   error_t error;
   uint_t i;
   NdpDestCacheEntry *entry;

   //Go through Destination Cache
   for(i = 0; i < NDP_DEST_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ndpContext.destCache[i];

      //Check whether the unreachable IPv6 address is used a first-hop router
      if(ipv6CompAddr(&entry->nextHop, unreachableNextHop))
      {
         //Perform next-hop determination
         error = ndpSelectNextHop(interface, &entry->destAddr, &entry->nextHop,
            &entry->nextHop, FALSE);

         //Next-hop determination failed?
         if(error)
         {
            //Remove the current entry from the Destination Cache
            entry->destAddr = IPV6_UNSPECIFIED_ADDR;
         }
      }
   }
}


/**
 * @brief Append an option to a NDP message
 * @param[in] message Pointer to the NDP message
 * @param[in,out] messageLen Length of the entire message
 * @param[in] type Option type
 * @param[in] value Option value
 * @param[in] length Length of the option value
 **/

void ndpAddOption(void *message, size_t *messageLen, uint8_t type,
   const void *value, size_t length)
{
   size_t optionLen;
   size_t paddingLen;
   NdpOption *option;

   //Length of the option in units of 8 bytes including the type and length fields
   optionLen = (length + sizeof(NdpOption) + 7) / 8;

   //Sanity check
   if(optionLen <= UINT8_MAX)
   {
      //Point to the buffer where the option is to be written
      option = (NdpOption *) ((uint8_t *) message + *messageLen);

      //Option type
      option->type = type;
      //Option length
      option->length = (uint8_t) optionLen;
      //Option value
      osMemcpy(option->value, value, length);

      //Options should be padded when necessary to ensure that they end on
      //their natural 64-bit boundaries
      if((length + sizeof(NdpOption)) < (optionLen * 8))
      {
         //Determine the amount of padding data to append
         paddingLen = (optionLen * 8) - length - sizeof(NdpOption);
         //Write padding data
         osMemset(option->value + length, 0, paddingLen);
      }

      //Adjust the length of the NDP message
      *messageLen += optionLen * 8;
   }
}


/**
 * @brief Find a specified option in a NDP message
 * @param[in] options Pointer to the Options field
 * @param[in] length Length of the Options field
 * @param[in] type Type of the option to find
 * @return If the specified option is found, a pointer to the corresponding
 *   option is returned. Otherwise NULL pointer is returned
 **/

void *ndpGetOption(uint8_t *options, size_t length, uint8_t type)
{
   size_t i;
   NdpOption *option;

   //Point to the very first option of the NDP message
   i = 0;

   //Parse options
   while((i + sizeof(NdpOption)) <= length)
   {
      //Point to the current option
      option = (NdpOption *) (options + i);

      //Nodes must silently discard an NDP message that contains
      //an option with length zero
      if(option->length == 0)
         break;
      //Check option length
      if((i + option->length * 8) > length)
         break;

      //Current option type matches the specified one?
      if(option->type == type || type == NDP_OPT_ANY)
         return option;

      //Jump to the next option
      i += option->length * 8;
   }

   //Specified option type not found
   return NULL;
}


/**
 * @brief Check NDP message options
 * @param[in] options Pointer to the Options field
 * @param[in] length Length of the Options field
 * @return Error code
 **/

error_t ndpCheckOptions(const uint8_t *options, size_t length)
{
   size_t i;
   NdpOption *option;

   //Point to the very first option of the NDP message
   i = 0;

   //Parse options
   while((i + sizeof(NdpOption)) <= length)
   {
      //Point to the current option
      option = (NdpOption *) (options + i);

      //Nodes must silently discard an NDP message that contains
      //an option with length zero
      if(option->length == 0)
         return ERROR_INVALID_OPTION;

      //Jump to the next option
      i += option->length * 8;
   }

   //The Options field is valid
   return NO_ERROR;
}

#endif
