/**
 * @file ipv6_misc.c
 * @brief Helper functions for IPv6
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
#include "core/net.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "ipv6/ndp.h"
#include "ipv6/ndp_cache.h"
#include "mdns/mdns_responder.h"
#include "mibs/ip_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED)


/**
 * @brief Get the state of the specified IPv6 address
 * @param[in] interface Underlying network interface
 * @param[in] addr IPv6 address
 * @return Address state
 **/

Ipv6AddrState ipv6GetAddrState(NetInterface *interface, const Ipv6Addr *addr)
{
   uint_t i;
   Ipv6AddrEntry *entry;

   //Loop through the list of IPv6 addresses assigned to the interface
   for(i = 0; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.addrList[i];

      //Valid IPv6 address
      if(entry->state != IPV6_ADDR_STATE_INVALID)
      {
         //Check whether the current entry matches the specified address
         if(ipv6CompAddr(&entry->addr, addr))
         {
            //Return the state of the IPv6 address
            return entry->state;
         }
      }
   }

   //The specified IPv6 address is not valid
   return IPV6_ADDR_STATE_INVALID;
}


/**
 * @brief Set IPv6 address and address state
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[in] addr IPv6 address
 * @param[in] state State of the IPv6 address
 * @param[in] validLifetime Valid lifetime
 * @param[in] preferredLifetime Preferred lifetime
 * @param[in] permanent Permanently assigned address
 * @return Error code
 **/

error_t ipv6SetAddr(NetInterface *interface, uint_t index,
   const Ipv6Addr *addr, Ipv6AddrState state, systime_t validLifetime,
   systime_t preferredLifetime, bool_t permanent)
{
   error_t error;
   NetInterface *physicalInterface;
   Ipv6AddrEntry *entry;
   Ipv6Addr solicitedNodeAddr;

   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV6_ADDR_LIST_SIZE)
      return ERROR_OUT_OF_RANGE;

   //The IPv6 address must be a valid unicast address
   if(ipv6IsMulticastAddr(addr))
      return ERROR_INVALID_ADDRESS;

   //Initialize status code
   error = NO_ERROR;

   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);

   //Point to the corresponding entry
   entry = &interface->ipv6Context.addrList[index];

   //Check whether an IPv6 address is already assigned
   if(!ipv6CompAddr(&entry->addr, &IPV6_UNSPECIFIED_ADDR))
   {
      //Check the state of the IPv6 address
      if(entry->state != IPV6_ADDR_STATE_INVALID)
      {
         //Ethernet interface?
         if(physicalInterface->nicDriver != NULL &&
            physicalInterface->nicDriver->type == NIC_TYPE_ETHERNET)
         {
            //Form the Solicited-Node address
            ipv6ComputeSolicitedNodeAddr(&entry->addr, &solicitedNodeAddr);
            //Leave the Solicited-Node multicast group
            ipv6LeaveMulticastGroup(interface, &solicitedNodeAddr);
         }
      }
   }

   //The current IPv6 address is no more valid
   entry->state = IPV6_ADDR_STATE_INVALID;
   entry->validLifetime = 0;
   entry->preferredLifetime = 0;
   entry->permanent = FALSE;

   //Assign the new IPv6 address
   entry->addr = *addr;

   //Check whether the new IPv6 address is valid
   if(!ipv6CompAddr(addr, &IPV6_UNSPECIFIED_ADDR))
   {
      //Check the state of the IPv6 address
      if(state != IPV6_ADDR_STATE_INVALID)
      {
         //Ethernet interface?
         if(physicalInterface->nicDriver != NULL &&
            physicalInterface->nicDriver->type == NIC_TYPE_ETHERNET)
         {
            //Form the Solicited-Node address for the link-local address
            ipv6ComputeSolicitedNodeAddr(addr, &solicitedNodeAddr);
            //Join the Solicited-Node multicast group for each assigned address
            error = ipv6JoinMulticastGroup(interface, &solicitedNodeAddr);
         }
         //6LoWPAN interface?
         else if(interface->nicDriver != NULL &&
            interface->nicDriver->type == NIC_TYPE_6LOWPAN)
         {
            //There is no need to join the solicited-node multicast address,
            //since nobody multicasts Neighbor Solicitations in this type of
            //network (refer to RFC 6775 section 5.2)
         }
      }

      //Check status code
      if(!error)
      {
         //Set the state of the IPv6 address
         entry->state = state;

         //Clear duplicate flag
         entry->duplicate = FALSE;

         //Save preferred and valid lifetimes
         entry->preferredLifetime = preferredLifetime;
         entry->validLifetime = validLifetime;

         //Set time stamp
         entry->timestamp = osGetSystemTime();

         //Initialize DAD related variables
         entry->dadTimeout = 0;
         entry->dadRetransmitCount = 0;
      }

      //This flag tells whether the IPv6 address should be permanently assigned
      entry->permanent = permanent;
   }

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
   //Link-local address?
   if(index == 0)
   {
      //Restart mDNS probing process
      mdnsResponderStartProbing(interface->mdnsResponderContext);
   }
#endif

   //Return status code
   return error;
}


/**
 * @brief Add a new entry to the list of IPv6 addresses
 * @param[in] interface Underlying network interface
 * @param[in] addr IPv6 address
 * @param[in] validLifetime Valid lifetime, in seconds
 * @param[in] preferredLifetime Preferred lifetime, in seconds
 **/

void ipv6AddAddr(NetInterface *interface, const Ipv6Addr *addr,
   uint32_t validLifetime, uint32_t preferredLifetime)
{
   uint_t i;
   Ipv6AddrEntry *entry;

   //Check the valid lifetime
   if(validLifetime != NDP_INFINITE_LIFETIME)
   {
      //The length of time in seconds that the address is valid
      if(validLifetime < (MAX_DELAY / 1000))
         validLifetime *= 1000;
      else
         validLifetime = MAX_DELAY;
   }
   else
   {
      //A value of all one bits (0xffffffff) represents infinity
      validLifetime = INFINITE_DELAY;
   }

   //Check the preferred lifetime
   if(preferredLifetime != NDP_INFINITE_LIFETIME)
   {
      //The length of time in seconds that the address remains preferred
      if(preferredLifetime < (MAX_DELAY / 1000))
         preferredLifetime *= 1000;
      else
         preferredLifetime = MAX_DELAY;
   }
   else
   {
      //A value of all one bits (0xffffffff) represents infinity
      preferredLifetime = INFINITE_DELAY;
   }

   //Loop through the list of IPv6 addresses assigned to the interface
   for(i = 0; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.addrList[i];

      //Check the state of the IPv6 address
      if(entry->state == IPV6_ADDR_STATE_PREFERRED ||
         entry->state == IPV6_ADDR_STATE_DEPRECATED)
      {
         //Check whether the current entry matches the specified address
         if(ipv6CompAddr(&entry->addr, addr))
         {
            //The IPv6 address should be preserved if it has been manually assigned
            if(!entry->permanent)
            {
               //Update the lifetimes of the entry
               entry->validLifetime = validLifetime;
               entry->preferredLifetime = preferredLifetime;

               //Save current time
               entry->timestamp = osGetSystemTime();
               //Update the state of the IPv6 address
               entry->state = IPV6_ADDR_STATE_PREFERRED;
            }

            //Exit immediately
            return;
         }
      }
   }

   //If no matching entry was found, then try to create a new entry
   for(i = 0; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.addrList[i];

      //Check the state of the IPv6 address
      if(entry->state == IPV6_ADDR_STATE_INVALID)
      {
         //The IPv6 address should be preserved if it has been manually assigned
         if(!entry->permanent)
         {
#if (NDP_SUPPORT == ENABLED)
            //Check whether Duplicate Address Detection should be performed
            if(interface->ndpContext.dupAddrDetectTransmits > 0)
            {
               //Use the IPv6 address as a tentative address
               ipv6SetAddr(interface, i, addr, IPV6_ADDR_STATE_TENTATIVE,
                  validLifetime, preferredLifetime, FALSE);
            }
            else
#endif
            {
               //The use of the IPv6 address is now unrestricted
               ipv6SetAddr(interface, i, addr, IPV6_ADDR_STATE_PREFERRED,
                  validLifetime, preferredLifetime, FALSE);
            }

            //Exit immediately
            return;
         }
      }
   }
}


/**
 * @brief Remove an entry from the list of IPv6 addresses
 * @param[in] interface Underlying network interface
 * @param[in] addr IPv6 address
 **/

void ipv6RemoveAddr(NetInterface *interface, const Ipv6Addr *addr)
{
   uint_t i;
   Ipv6AddrEntry *entry;

   //Loop through the list of IPv6 addresses assigned to the interface
   for(i = 0; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.addrList[i];

      //Valid IPv6 address?
      if(entry->validLifetime)
      {
         //Check whether the current entry matches the specified address
         if(ipv6CompAddr(&entry->addr, addr))
         {
            //The IPv6 address should be preserved if it has been manually assigned
            if(!entry->permanent)
            {
               //Remove the IPv6 address from the list
               ipv6SetAddr(interface, i, &IPV6_UNSPECIFIED_ADDR,
                  IPV6_ADDR_STATE_INVALID, 0, 0, FALSE);
            }
         }
      }
   }
}


/**
 * @brief Add a new entry to the Prefix List
 * @param[in] interface Underlying network interface
 * @param[in] prefix IPv6 prefix
 * @param[in] length The number of leading bits in the prefix that are valid
 * @param[in] onLinkFlag On-link flag
 * @param[in] autonomousFlag Autonomous flag
 * @param[in] validLifetime Valid lifetime, in seconds
 * @param[in] preferredLifetime Preferred lifetime, in seconds
 **/

void ipv6AddPrefix(NetInterface *interface, const Ipv6Addr *prefix,
   uint_t length, bool_t onLinkFlag, bool_t autonomousFlag,
   uint32_t validLifetime, uint32_t preferredLifetime)
{
   uint_t i;
   Ipv6PrefixEntry *entry;
   Ipv6PrefixEntry *firstFreeEntry;

   //Initialize variables
   entry = NULL;
   firstFreeEntry = NULL;

   //Loop through the Prefix List
   for(i = 0; i < IPV6_PREFIX_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.prefixList[i];

      //Valid prefix?
      if(entry->validLifetime)
      {
         //Compare prefix length against the specified value
         if(entry->prefixLen == length)
         {
            //Check whether the current entry matches the specified prefix
            if(ipv6CompPrefix(&entry->prefix, prefix, length))
               break;
         }
      }
      else
      {
         //The IPv6 prefix should be preserved if it has been manually assigned
         if(!entry->permanent)
         {
            //Keep track of the first free entry
            if(firstFreeEntry == NULL)
               firstFreeEntry = entry;
         }
      }
   }

   //No matching entry found?
   if(i >= IPV6_PREFIX_LIST_SIZE)
      entry = firstFreeEntry;

   //Update the entry if necessary
   if(entry != NULL)
   {
      //The IPv6 prefix should be preserved if it has been manually assigned
      if(!entry->permanent)
      {
         //Save the IPv6 prefix
         entry->prefix = *prefix;
         entry->prefixLen = length;

         //Save On-link and Autonomous flags
         entry->onLinkFlag = onLinkFlag;
         entry->autonomousFlag = autonomousFlag;

         //Check the valid lifetime
         if(validLifetime != NDP_INFINITE_LIFETIME)
         {
            //The length of time in seconds that the prefix is valid
            //for the purpose of on-link determination
            if(validLifetime < (MAX_DELAY / 1000))
               entry->validLifetime = validLifetime * 1000;
            else
               entry->validLifetime = MAX_DELAY;
         }
         else
         {
            //A value of all one bits (0xffffffff) represents infinity
            entry->validLifetime = INFINITE_DELAY;
         }

         //Check the preferred lifetime
         if(preferredLifetime != NDP_INFINITE_LIFETIME)
         {
            //The length of time in seconds that addresses generated from the
            //prefix via stateless address autoconfiguration remain preferred
            if(preferredLifetime < (MAX_DELAY / 1000))
               entry->preferredLifetime = preferredLifetime * 1000;
            else
               entry->preferredLifetime = MAX_DELAY;
         }
         else
         {
            //A value of all one bits (0xffffffff) represents infinity
            entry->preferredLifetime = INFINITE_DELAY;
         }

         //Save current time
         entry->timestamp = osGetSystemTime();
      }
   }
}


/**
 * @brief Remove an entry from the Prefix List
 * @param[in] interface Underlying network interface
 * @param[in] prefix IPv6 prefix
 * @param[in] length The number of leading bits in the prefix that are valid
 **/

void ipv6RemovePrefix(NetInterface *interface, const Ipv6Addr *prefix,
   uint_t length)
{
   uint_t i;
   Ipv6PrefixEntry *entry;

   //Loop through the Prefix List
   for(i = 0; i < IPV6_PREFIX_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.prefixList[i];

      //Valid prefix?
      if(entry->validLifetime)
      {
         //Compare prefix length against the specified value
         if(entry->prefixLen == length)
         {
            //Check whether the current entry matches the specified prefix
            if(ipv6CompPrefix(&entry->prefix, prefix, length))
            {
               //The IPv6 prefix should be preserved if it has been manually assigned
               if(!entry->permanent)
               {
                  //When removing an entry from the Prefix List, there is no need
                  //to purge any entries from the Destination or Neighbor Caches
                  entry->prefix = IPV6_UNSPECIFIED_ADDR;
                  entry->prefixLen = 0;
                  entry->validLifetime = 0;
               }
            }
         }
      }
   }
}


/**
 * @brief Add a new entry to the Default Router List
 * @param[in] interface Underlying network interface
 * @param[in] addr IPv6 address of the router
 * @param[in] lifetime Router lifetime, in seconds
 * @param[in] preference Preference value
 **/

void ipv6AddDefaultRouter(NetInterface *interface, const Ipv6Addr *addr,
   uint16_t lifetime, uint8_t preference)
{
   uint_t i;
   Ipv6RouterEntry *entry;
   Ipv6RouterEntry *firstFreeEntry;

   //Initialize variables
   entry = NULL;
   firstFreeEntry = NULL;

   //Loop through the Default Router List
   for(i = 0; i < IPV6_ROUTER_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.routerList[i];

      //Check the lifetime associated with the default router
      if(entry->lifetime)
      {
         //Check whether the current entry matches the specified router address
         if(ipv6CompAddr(&entry->addr, addr))
            break;
      }
      else
      {
         //The router address should be preserved if it has been manually assigned
         if(!entry->permanent)
         {
            //Keep track of the first free entry
            if(firstFreeEntry == NULL)
               firstFreeEntry = entry;
         }
      }
   }

   //No matching entry found?
   if(i >= IPV6_ROUTER_LIST_SIZE)
      entry = firstFreeEntry;

   //Update the entry if necessary
   if(entry != NULL)
   {
      //The router address should be preserved if it has been manually assigned
      if(!entry->permanent)
      {
         //Save the IPv6 address of the router
         entry->addr = *addr;
         //The lifetime associated with the default router
         entry->lifetime = lifetime * 1000;
         //Save preference value
         entry->preference = preference;
         //Save current time
         entry->timestamp = osGetSystemTime();
      }
   }
}


/**
 * @brief Remove an entry from the Default Router List
 * @param[in] interface Underlying network interface
 * @param[in] addr IPv6 address of the router to be removed from the list
 **/

void ipv6RemoveDefaultRouter(NetInterface *interface, const Ipv6Addr *addr)
{
   uint_t i;
   bool_t flag;
   Ipv6RouterEntry *entry;

   //This flag will be set if any entry has been removed from
   //the Default Router List
   flag = FALSE;

   //Loop through the Default Router List
   for(i = 0; i < IPV6_ROUTER_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.routerList[i];

      //Check the lifetime associated with the default router
      if(entry->lifetime)
      {
         //Check whether the current entry matches the specified router address
         if(ipv6CompAddr(&entry->addr, addr))
         {
            //The router address should be preserved if it has been manually assigned
            if(!entry->permanent)
            {
               //Immediately time-out the entry
               entry->addr = IPV6_UNSPECIFIED_ADDR;
               entry->lifetime = 0;

               //Set flag
               flag = TRUE;
            }
         }
      }
   }

   //Check whether an entry has been removed from the list
   if(flag)
   {
#if (NDP_SUPPORT == ENABLED)
      //When removing a router from the Default Router list, the node must
      //update the Destination Cache in such a way that all entries using
      //the router perform next-hop determination again
      ndpFlushDestCache(interface);
#endif
   }
}


/**
 * @brief Flush the list of IPv6 addresses
 * @param[in] interface Underlying network interface
 **/

void ipv6FlushAddrList(NetInterface *interface)
{
   uint_t i;
   Ipv6AddrEntry *entry;

   //Go through the list of IPv6 addresses
   for(i = 0; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.addrList[i];

      //Valid IPv6 address?
      if(entry->validLifetime > 0)
      {
         //The IPv6 address should be preserved if it has been manually assigned
         if(!entry->permanent)
         {
            //The IPv6 address is not longer valid
            ipv6SetAddr(interface, i, &IPV6_UNSPECIFIED_ADDR,
               IPV6_ADDR_STATE_INVALID, 0, 0, FALSE);
         }
      }
   }
}


/**
 * @brief Flush the Prefix List
 * @param[in] interface Underlying network interface
 **/

void ipv6FlushPrefixList(NetInterface *interface)
{
   uint_t i;
   Ipv6PrefixEntry *entry;

   //Go through the Prefix List
   for(i = 0; i < IPV6_PREFIX_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.prefixList[i];

      //Valid IPv6 prefix?
      if(entry->validLifetime > 0)
      {
         //The IPv6 prefix should be preserved if it has been manually assigned
         if(!entry->permanent)
         {
            //Remove the entry from the Prefix List
            entry->prefix = IPV6_UNSPECIFIED_ADDR;
            entry->prefixLen = 0;
            entry->validLifetime = 0;
         }
      }
   }
}


/**
 * @brief Flush the Default Router List
 * @param[in] interface Underlying network interface
 **/

void ipv6FlushDefaultRouterList(NetInterface *interface)
{
   uint_t i;
   Ipv6RouterEntry *entry;

   //Go through the Default Router List
   for(i = 0; i < IPV6_ROUTER_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.routerList[i];

      //Valid entry?
      if(entry->lifetime > 0)
      {
         //The router address should be preserved if it has been manually assigned
         if(!entry->permanent)
         {
            //Clear the current entry
            entry->addr = IPV6_UNSPECIFIED_ADDR;
            //Remove the entry from the Default Router List
            entry->lifetime = 0;
         }
      }
   }
}


/**
 * @brief Flush the list of DNS servers
 * @param[in] interface Underlying network interface
 **/

void ipv6FlushDnsServerList(NetInterface *interface)
{
   //Clear the list of DNS servers
   osMemset(interface->ipv6Context.dnsServerList, 0,
      sizeof(interface->ipv6Context.dnsServerList));
}


/**
 * @brief Source IPv6 address filtering
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr Source IPv6 address to be checked
 * @return Error code
 **/

error_t ipv6CheckSourceAddr(NetInterface *interface, const Ipv6Addr *ipAddr)
{
   //Multicast addresses cannot be used as source address
   if(ipv6IsMulticastAddr(ipAddr))
   {
      //Debug message
      TRACE_WARNING("Wrong source IPv6 address!\r\n");
      //The source address not is acceptable
      return ERROR_INVALID_ADDRESS;
   }

   //The source address is acceptable
   return NO_ERROR;
}


/**
 * @brief Destination IPv6 address filtering
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr Destination IPv6 address to be checked
 * @return Error code
 **/

error_t ipv6CheckDestAddr(NetInterface *interface, const Ipv6Addr *ipAddr)
{
   error_t error;
   uint_t i;

   //Filter out any invalid addresses
   error = ERROR_INVALID_ADDRESS;

   //Multicast address?
   if(ipv6IsMulticastAddr(ipAddr))
   {
      //Go through the multicast filter table
      for(i = 0; i < IPV6_MULTICAST_FILTER_SIZE; i++)
      {
         Ipv6FilterEntry *entry;

         //Point to the current entry
         entry = &interface->ipv6Context.multicastFilter[i];

         //Valid entry?
         if(entry->refCount > 0)
         {
            //Check whether the destination IPv6 address matches
            //a relevant multicast address
            if(ipv6CompAddr(&entry->addr, ipAddr))
            {
               //The multicast address is acceptable
               error = NO_ERROR;
               //Stop immediately
               break;
            }
         }
      }
   }
   //Unicast address?
   else
   {
      //Loop through the list of IPv6 unicast addresses
      for(i = 0; i < IPV6_ADDR_LIST_SIZE; i++)
      {
         Ipv6AddrEntry *entry;

         //Point to the current entry
         entry = &interface->ipv6Context.addrList[i];

         //Valid entry?
         if(entry->state != IPV6_ADDR_STATE_INVALID)
         {
            //Check whether the destination address matches a valid unicast
            //address assigned to the interface
            if(ipv6CompAddr(&entry->addr, ipAddr))
            {
               //The destination address is acceptable
               error = NO_ERROR;
               //We are done
               break;
            }
         }
      }

      //Check whether the specified is a valid unicast address
      if(error == ERROR_INVALID_ADDRESS)
      {
         Ipv6Addr *anycastAddrList;

         //Point to the list of anycast addresses assigned to the interface
         anycastAddrList = interface->ipv6Context.anycastAddrList;

         //Loop through the list of IPv6 anycast addresses
         for(i = 0; i < IPV6_ANYCAST_ADDR_LIST_SIZE; i++)
         {
            //Valid entry?
            if(!ipv6CompAddr(&anycastAddrList[i], &IPV6_UNSPECIFIED_ADDR))
            {
               //Check whether the destination address matches a valid anycast
               //address assigned to the interface
               if(ipv6CompAddr(&anycastAddrList[i], ipAddr))
               {
                  //The destination address is acceptable
                  error = NO_ERROR;
                  //We are done
                  break;
               }
            }
         }
      }
   }

   //Return status code
   return error;
}


/**
 * @brief IPv6 source address selection
 *
 * This function selects the source address and the relevant network interface
 * to be used in order to join the specified destination address
 *
 * @param[in,out] interface A pointer to a valid network interface may be provided as
 *   a hint. The function returns a pointer identifying the interface to be used
 * @param[in] destAddr Destination IPv6 address
 * @param[out] srcAddr Local IPv6 address to be used
 * @return Error code
 **/

error_t ipv6SelectSourceAddr(NetInterface **interface,
   const Ipv6Addr *destAddr, Ipv6Addr *srcAddr)
{
   error_t error;
   uint_t i;
   uint_t j;
   NetInterface *currentInterface;
   NetInterface *bestInterface;
   Ipv6AddrEntry *currentAddr;
   Ipv6AddrEntry *bestAddr;

   //Initialize variables
   bestInterface = NULL;
   bestAddr = NULL;

   //Loop through network interfaces
   for(i = 0; i < NET_INTERFACE_COUNT; i++)
   {
      //Point to the current interface
      currentInterface = &netInterface[i];

      //A network interface may be provided as a hint
      if(*interface != currentInterface && *interface != NULL)
      {
         //Select the next interface in the list
         continue;
      }

      //A sort of the candidate source addresses is being performed, where a
      //set of rules define the ordering among addresses (refer to RFC 6724)
      for(j = 0; j < IPV6_ADDR_LIST_SIZE; j++)
      {
         //Point to the current entry
         currentAddr = &currentInterface->ipv6Context.addrList[j];

         //Check the state of the address
         if(currentAddr->state == IPV6_ADDR_STATE_PREFERRED ||
            currentAddr->state == IPV6_ADDR_STATE_DEPRECATED)
         {
            //Select the first address as default
            if(bestAddr == NULL)
            {
               //Give the current source address the higher precedence
               bestInterface = currentInterface;
               bestAddr = currentAddr;

               //Select the next address in the list
               continue;
            }

            //Rule 1: Prefer same address
            if(ipv6CompAddr(&bestAddr->addr, destAddr))
            {
               //Select the next address in the list
               continue;
            }
            else if(ipv6CompAddr(&currentAddr->addr, destAddr))
            {
               //Give the current source address the higher precedence
               bestInterface = currentInterface;
               bestAddr = currentAddr;

               //Select the next address in the list
               continue;
            }

            //Rule 2: Prefer appropriate scope
            if(ipv6GetAddrScope(&currentAddr->addr) < ipv6GetAddrScope(&bestAddr->addr))
            {
               if(ipv6GetAddrScope(&currentAddr->addr) >= ipv6GetAddrScope(destAddr))
               {
                  //Give the current source address the higher precedence
                  bestInterface = currentInterface;
                  bestAddr = currentAddr;
               }

               //Select the next address in the list
               continue;
            }
            else if(ipv6GetAddrScope(&bestAddr->addr) < ipv6GetAddrScope(&currentAddr->addr))
            {
               if(ipv6GetAddrScope(&bestAddr->addr) < ipv6GetAddrScope(destAddr))
               {
                  //Give the current source address the higher precedence
                  bestInterface = currentInterface;
                  bestAddr = currentAddr;
               }

               //Select the next address in the list
               continue;
            }

            //Rule 3: Avoid deprecated addresses
            if(bestAddr->state == IPV6_ADDR_STATE_PREFERRED &&
               currentAddr->state == IPV6_ADDR_STATE_DEPRECATED)
            {
               //Select the next address in the list
               continue;
            }
            else if(currentAddr->state == IPV6_ADDR_STATE_PREFERRED &&
               bestAddr->state == IPV6_ADDR_STATE_DEPRECATED)
            {
               //Give the current source address the higher precedence
               bestInterface = currentInterface;
               bestAddr = currentAddr;

               //Select the next address in the list
               continue;
            }

            //Rule 8: Use longest matching prefix
            if(ipv6GetCommonPrefixLength(&currentAddr->addr, destAddr) >
               ipv6GetCommonPrefixLength(&bestAddr->addr, destAddr))
            {
               //Give the current source address the higher precedence
               bestInterface = currentInterface;
               bestAddr = currentAddr;
            }
         }
      }
   }

   //Valid source address?
   if(bestAddr != NULL)
   {
      //Return the out-going interface and the source address to be used
      *interface = bestInterface;
      *srcAddr = bestAddr->addr;

      //Successful source address selection
      error = NO_ERROR;
   }
   else
   {
      //Report an error
      error = ERROR_NO_ADDRESS;
   }

   //Return status code
   return error;
}


/**
 * @brief Check whether an IPv6 address is on-link
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr IPv6 address to be checked
 * @return TRUE if the IPv6 address is on-link, else FALSE
 **/

bool_t ipv6IsOnLink(NetInterface *interface, const Ipv6Addr *ipAddr)
{
   uint_t i;
   Ipv6PrefixEntry *entry;

   //Link-local prefix?
   if(ipv6IsLinkLocalUnicastAddr(ipAddr))
   {
      //The link-local prefix is considered to be on the prefix list with
      //an infinite invalidation timer regardless of whether routers are
      //advertising a prefix for it
      return TRUE;
   }

   //Loop through the Prefix List
   for(i = 0; i < IPV6_PREFIX_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.prefixList[i];

      //Valid prefix?
      if(entry->validLifetime > 0)
      {
         //Check the specified address against the prefix
         if(ipv6CompPrefix(ipAddr, &entry->prefix, entry->prefixLen))
         {
            //The specified IPv6 address is on-link
            return TRUE;
         }
      }
   }

   //The specified IPv6 address is off-link
   return FALSE;
}


/**
 * @brief Check whether an IPv6 address is an anycast address
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr IPv6 address to be checked
 * @return TRUE if the IPv6 address is an anycast address, else FALSE
 **/

bool_t ipv6IsAnycastAddr(NetInterface *interface, const Ipv6Addr *ipAddr)
{
   uint_t i;
   Ipv6Addr *anycastAddrList;

   //Point to the list of anycast addresses assigned to the interface
   anycastAddrList = interface->ipv6Context.anycastAddrList;

   //Loop through the list of IPv6 anycast addresses
   for(i = 0; i < IPV6_ANYCAST_ADDR_LIST_SIZE; i++)
   {
      //Valid entry?
      if(!ipv6CompAddr(&anycastAddrList[i], &IPV6_UNSPECIFIED_ADDR))
      {
         //Check whether the specified address matches a valid anycast
         //address assigned to the interface
         if(ipv6CompAddr(&anycastAddrList[i], ipAddr))
         {
            //The specified IPv6 address is an anycast address
            return TRUE;
         }
      }
   }

   //The specified IPv6 address is not an anycast address
   return FALSE;
}


/**
 * @brief Check whether an IPv6 address is a tentative address
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr IPv6 address to be checked
 * @return TRUE if the IPv6 address is a tentative address, else FALSE
 **/

bool_t ipv6IsTentativeAddr(NetInterface *interface, const Ipv6Addr *ipAddr)
{
   uint_t i;
   Ipv6AddrEntry *entry;

   //Loop through the list of IPv6 unicast addresses
   for(i = 0; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.addrList[i];

      //Tentative address?
      if(entry->state == IPV6_ADDR_STATE_TENTATIVE)
      {
         //Check whether the specified address matches a valid unicast
         //address assigned to the interface
         if(ipv6CompAddr(&entry->addr, ipAddr))
         {
            //The specified IPv6 address is a tentative address
            return TRUE;
         }
      }
   }

   //The specified IPv6 address is not a tentative address
   return FALSE;
}


/**
 * @brief Check whether the specified IPv6 is assigned to the host
 * @param[in] ipAddr IPv6 address to be checked
 * @return TRUE if the IPv6 address matches any address assigned to the host,
 *   else FALSE
 **/

bool_t ipv6IsLocalHostAddr(const Ipv6Addr *ipAddr)
{
   uint_t i;
   uint_t j;
   bool_t flag;
   NetInterface *interface;
   Ipv6AddrEntry *entry;

   //Initialize flag
   flag = FALSE;

   //Loopback address?
   if(ipv6CompAddr(ipAddr, &IPV6_LOOPBACK_ADDR))
   {
      //If an application in a host sends packets to this address, the IPv6
      //stack will loop these packets back on the same virtual interface
      flag = TRUE;
   }
   else
   {
      //Loop through network interfaces
      for(i = 0; i < NET_INTERFACE_COUNT && !flag; i++)
      {
         //Point to the current interface
         interface = &netInterface[i];

         //Iterate through the list of addresses assigned to the interface
         for(j = 0; j < IPV6_ADDR_LIST_SIZE && !flag; j++)
         {
            //Point to the current entry
            entry = &interface->ipv6Context.addrList[j];

            //Valid entry?
            if(entry->state == IPV6_ADDR_STATE_PREFERRED ||
               entry->state == IPV6_ADDR_STATE_DEPRECATED)
            {
               //Check whether the specified IPv6 address matches any address
               //assigned to the host
               if(ipv6CompAddr(&entry->addr, ipAddr))
               {
                  flag = TRUE;
               }
            }
         }
      }
   }

   //Return TRUE if the specified address matches any address assigned to the host
   return flag;
}


/**
 * @brief Compare IPv6 address prefixes
 * @param[in] ipAddr1 Pointer to the first IPv6 address
 * @param[in] ipAddr2 Pointer to the second IPv6 address
 * @param[in] length Prefix length
 * @return TRUE if the prefixes match each other, else FALSE
 **/

bool_t ipv6CompPrefix(const Ipv6Addr *ipAddr1, const Ipv6Addr *ipAddr2,
   size_t length)
{
   size_t n;
   size_t m;
   uint8_t mask;

   //Ensure the prefix length is valid
   if(length > 128)
      return FALSE;

   //Number of complete bytes
   n = length / 8;
   //Number of bits in the last byte, if any
   m = length % 8;

   //Compare the first part
   if(n > 0)
   {
      if(osMemcmp(ipAddr1, ipAddr2, n))
         return FALSE;
   }

   //Compare the remaining bits, if any
   if(m > 0)
   {
      //Calculate the mask to be applied
      mask = ((1 << m) - 1) << (8 - m);

      //Check remaining bits
      if((ipAddr1->b[n] & mask) != (ipAddr2->b[n] & mask))
         return FALSE;
   }

   //The prefixes match each other
   return TRUE;
}


/**
 * @brief Retrieve the scope of an IPv6 address
 * @param[in] ipAddr Pointer to an IPv6 address
 * @return IPv6 address scope
 **/

uint_t ipv6GetAddrScope(const Ipv6Addr *ipAddr)
{
   uint_t scope;

   //Multicast address?
   if(ipv6IsMulticastAddr(ipAddr))
   {
      //Retrieve the scope of the multicast address
      scope = ipv6GetMulticastAddrScope(ipAddr);
   }
   //Loopback address?
   else if(ipv6CompAddr(ipAddr, &IPV6_LOOPBACK_ADDR))
   {
      //The loopback address may be used by a node to send an IPv6 packet
      //to itself
      scope = IPV6_ADDR_SCOPE_INTERFACE_LOCAL;
   }
   //Link-local unicast address?
   else if(ipv6IsLinkLocalUnicastAddr(ipAddr))
   {
      //A link-local address is for use on a single link
      scope = IPV6_ADDR_SCOPE_LINK_LOCAL;
   }
   //Site-local unicast address?
   else if(ipv6IsSiteLocalUnicastAddr(ipAddr))
   {
      //A site-local address is for use in a single site
      scope = IPV6_ADDR_SCOPE_SITE_LOCAL;
   }
   //Global address?
   else
   {
      //Global scope
      scope = IPV6_ADDR_SCOPE_GLOBAL;
   }

   //Return the scope of the specified IPv6 address
   return scope;
}


/**
 * @brief Retrieve the scope of an IPv6 multicast address
 * @param[in] ipAddr Pointer to an IPv6 multicast address
 * @return IPv6 address scope
 **/

uint_t ipv6GetMulticastAddrScope(const Ipv6Addr *ipAddr)
{
   uint_t scope;

   //The scope field is a 4-bit value
   scope = ipAddr->b[1] & 0x0F;

   //If the scope field contains the reserved value F, an IPv6 packet
   //must be treated the same as packets destined to a global multicast
   //address (refer to RFC 3513 section 2.7)
   if(scope == 0x0F)
   {
      scope = IPV6_ADDR_SCOPE_GLOBAL;
   }

   //Return the scope of the specified IPv6 multicast address
   return scope;
}


/**
 * @brief Compute the length of the longest common prefix
 * @param[in] ipAddr1 Pointer to the first IPv6 address
 * @param[in] ipAddr2 Pointer to the second IPv6 address
 * @return The length of the longest common prefix, in bits
 **/

uint_t ipv6GetCommonPrefixLength(const Ipv6Addr *ipAddr1,
   const Ipv6Addr *ipAddr2)
{
   uint_t i;
   uint_t j;
   uint8_t mask;

   //Clear bit counter
   j = 0;

   //Perform a byte-for-byte comparison
   for(i = 0; i < sizeof(Ipv6Addr); i++)
   {
      //Loop as long as prefixes match
      if(ipAddr1->b[i] != ipAddr2->b[i])
      {
         break;
      }
   }

   //Any mismatch?
   if(i < sizeof(Ipv6Addr))
   {
      //Perform a bit-for-bit comparison
      for(j = 0; j < 8; j++)
      {
         //Calculate the mask to be applied
         mask = 1 << (7 - j);

         //Loop as long as prefixes match
         if((ipAddr1->b[i] & mask) != (ipAddr2->b[i] & mask))
         {
            break;
         }
      }
   }

   //Return the length of the longest prefix that the two addresses
   //have in common
   return i * 8 + j;
}


/**
 * @brief Form a solicited-node address from an IPv6 address
 * @param[in] ipAddr Unicast or anycast address
 * @param[out] solicitedNodeAddr Corresponding solicited-node address
 * @return Error code
 **/

error_t ipv6ComputeSolicitedNodeAddr(const Ipv6Addr *ipAddr,
   Ipv6Addr *solicitedNodeAddr)
{
   error_t error;

   //Ensure the specified address is a valid unicast or anycast address
   if(!ipv6IsMulticastAddr(ipAddr))
   {
      //Copy the 104-bit prefix
      ipv6CopyAddr(solicitedNodeAddr, &IPV6_SOLICITED_NODE_ADDR_PREFIX);

      //Take the low-order 24 bits of the address (unicast or anycast) and
      //append those bits to the prefix
      solicitedNodeAddr->b[13] = ipAddr->b[13];
      solicitedNodeAddr->b[14] = ipAddr->b[14];
      solicitedNodeAddr->b[15] = ipAddr->b[15];

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //Report an error
      error = ERROR_INVALID_ADDRESS;
   }

   //Return status code
   return error;
}


/**
 * @brief Map an IPv6 multicast address to a MAC-layer multicast address
 * @param[in] ipAddr IPv6 multicast address
 * @param[out] macAddr Corresponding MAC-layer multicast address
 * @return Error code
 **/

error_t ipv6MapMulticastAddrToMac(const Ipv6Addr *ipAddr, MacAddr *macAddr)
{
   error_t error;

   //Ensure the specified IPv6 address is a multicast address
   if(ipv6IsMulticastAddr(ipAddr))
   {
      //To support IPv6 multicasting, MAC address range of 33-33-00-00-00-00
      //to 33-33-FF-FF-FF-FF is reserved (refer to RFC 2464)
      macAddr->b[0] = 0x33;
      macAddr->b[1] = 0x33;

      //The low-order 32 bits of the IPv6 multicast address are mapped directly
      //to the low-order 32 bits in the MAC-layer multicast address
      macAddr->b[2] = ipAddr->b[12];
      macAddr->b[3] = ipAddr->b[13];
      macAddr->b[4] = ipAddr->b[14];
      macAddr->b[5] = ipAddr->b[15];

      //The specified IPv6 multicast address was successfully mapped to a
      //MAC-layer address
      error = NO_ERROR;
   }
   else
   {
      //Report an error
      error = ERROR_INVALID_ADDRESS;
   }

   //Return status code
   return error;
}


/**
 * @brief Generate a IPv6 link-local address from an interface identifier
 * @param[in] interfaceId Interface identifier
 * @param[out] ipAddr Corresponding IPv6 link-local address
 **/

void ipv6GenerateLinkLocalAddr(const Eui64 *interfaceId, Ipv6Addr *ipAddr)
{
   //A link-local address is formed by combining the well-known link-local
   //prefix fe80::/10 with the interface identifier
   ipAddr->w[0] = HTONS(0xFE80);
   ipAddr->w[1] = HTONS(0x0000);
   ipAddr->w[2] = HTONS(0x0000);
   ipAddr->w[3] = HTONS(0x0000);
   ipAddr->w[4] = interfaceId->w[0];
   ipAddr->w[5] = interfaceId->w[1];
   ipAddr->w[6] = interfaceId->w[2];
   ipAddr->w[7] = interfaceId->w[3];
}


/**
 * @brief Update IPv6 input statistics
 * @param[in] interface Underlying network interface
 * @param[in] destIpAddr Destination IP address
 * @param[in] length Length of the incoming IP packet
 **/

void ipv6UpdateInStats(NetInterface *interface, const Ipv6Addr *destIpAddr,
   size_t length)
{
   //Check whether the destination address is a unicast or multicast address
   if(ipv6IsMulticastAddr(destIpAddr))
   {
      //Number of IP multicast datagrams transmitted
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInMcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv6SystemStats.ipSystemStatsHCInMcastPkts, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInMcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv6IfStatsTable[interface->index].ipIfStatsHCInMcastPkts, 1);

      //Total number of octets transmitted in IP multicast datagrams
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInMcastOctets, length);
      IP_MIB_INC_COUNTER64(ipv6SystemStats.ipSystemStatsHCInMcastOctets, length);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInMcastOctets, length);
      IP_MIB_INC_COUNTER64(ipv6IfStatsTable[interface->index].ipIfStatsHCInMcastOctets, length);
   }
}


/**
 * @brief Update IPv6 output statistics
 * @param[in] interface Underlying network interface
 * @param[in] destIpAddr Destination IP address
 * @param[in] length Length of the outgoing IP packet
 **/

void ipv6UpdateOutStats(NetInterface *interface, const Ipv6Addr *destIpAddr,
   size_t length)
{
   //Check whether the destination address is a unicast or multicast address
   if(ipv6IsMulticastAddr(destIpAddr))
   {
      //Number of IP multicast datagrams transmitted
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsOutMcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv6SystemStats.ipSystemStatsHCOutMcastPkts, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsOutMcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv6IfStatsTable[interface->index].ipIfStatsHCOutMcastPkts, 1);

      //Total number of octets transmitted in IP multicast datagrams
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsOutMcastOctets, length);
      IP_MIB_INC_COUNTER64(ipv6SystemStats.ipSystemStatsHCOutMcastOctets, length);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsOutMcastOctets, length);
      IP_MIB_INC_COUNTER64(ipv6IfStatsTable[interface->index].ipIfStatsHCOutMcastOctets, length);
   }

   //Total number of IP datagrams that this entity supplied to the lower
   //layers for transmission
   IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsOutTransmits, 1);
   IP_MIB_INC_COUNTER64(ipv6SystemStats.ipSystemStatsHCOutTransmits, 1);
   IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsOutTransmits, 1);
   IP_MIB_INC_COUNTER64(ipv6IfStatsTable[interface->index].ipIfStatsHCOutTransmits, 1);

   //Total number of octets in IP datagrams delivered to the lower layers
   //for transmission
   IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsOutOctets, length);
   IP_MIB_INC_COUNTER64(ipv6SystemStats.ipSystemStatsHCOutOctets, length);
   IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsOutOctets, length);
   IP_MIB_INC_COUNTER64(ipv6IfStatsTable[interface->index].ipIfStatsHCOutOctets, length);
}

#endif
