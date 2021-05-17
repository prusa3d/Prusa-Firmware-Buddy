/**
 * @file ipv4_misc.c
 * @brief Helper functions for IPv4
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
#define TRACE_LEVEL IPV4_TRACE_LEVEL

//Dependencies
#include <string.h>
#include <ctype.h>
#include "core/net.h"
#include "ipv4/ipv4.h"
#include "ipv4/ipv4_misc.h"
#include "mibs/mib2_module.h"
#include "mibs/ip_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV4_SUPPORT == ENABLED)


/**
 * @brief Append a Router Alert option to an IPv4 packet
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in,out] offset Offset to the first payload byte
 * @return Error code
 **/

error_t ipv4AddRouterAlertOption(NetBuffer *buffer, size_t *offset)
{
   error_t error;
   Ipv4Option *option;

   //Make sure there is enough room to add the option
   if(*offset >= sizeof(uint32_t))
   {
      //Make room for the IPv4 option
      *offset -= sizeof(uint32_t);
      //Point to the IPv4 option
      option = netBufferAt(buffer, *offset);

      //Format Router Alert option
      option->type = IPV4_OPTION_RTRALT;
      option->length = sizeof(uint32_t);
      option->value[0] = 0;
      option->value[1] = 0;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //Report an error
      error = ERROR_INVALID_PARAMETER;
   }

   //Return status code
   return error;
}


/**
 * @brief Source IPv4 address filtering
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr Source IPv4 address to be checked
 * @return Error code
 **/

error_t ipv4CheckSourceAddr(NetInterface *interface, Ipv4Addr ipAddr)
{
   //Broadcast and multicast addresses must not be used as source address
   //(refer to RFC 1122, section 3.2.1.3)
   if(ipv4IsBroadcastAddr(interface, ipAddr) || ipv4IsMulticastAddr(ipAddr))
   {
      //Debug message
      TRACE_WARNING("Wrong source IPv4 address!\r\n");
      //The source address not is acceptable
      return ERROR_INVALID_ADDRESS;
   }

   //The source address is acceptable
   return NO_ERROR;
}


/**
 * @brief Destination IPv4 address filtering
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr Destination IPv4 address to be checked
 * @return Error code
 **/

error_t ipv4CheckDestAddr(NetInterface *interface, Ipv4Addr ipAddr)
{
   error_t error;
   uint_t i;

   //Filter out any invalid addresses
   error = ERROR_INVALID_ADDRESS;

   //Broadcast address?
   if(ipv4IsBroadcastAddr(interface, ipAddr))
   {
      //Always accept broadcast address
      error = NO_ERROR;
   }
   //Multicast address?
   else if(ipv4IsMulticastAddr(ipAddr))
   {
      //Go through the multicast filter table
      for(i = 0; i < IPV4_MULTICAST_FILTER_SIZE; i++)
      {
         Ipv4FilterEntry *entry;

         //Point to the current entry
         entry = &interface->ipv4Context.multicastFilter[i];

         //Valid entry?
         if(entry->refCount > 0)
         {
            //Check whether the destination IPv4 address matches
            //a relevant multicast address
            if(entry->addr == ipAddr)
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
      //Loop through the list of IPv4 addresses assigned to the interface
      for(i = 0; i < IPV4_ADDR_LIST_SIZE; i++)
      {
         Ipv4AddrEntry *entry;

         //Point to the current entry
         entry = &interface->ipv4Context.addrList[i];

         //Valid entry?
         if(entry->state != IPV4_ADDR_STATE_INVALID)
         {
            //Check whether the destination address matches a valid unicast
            //address assigned to the interface
            if(entry->addr == ipAddr)
            {
               //The destination address is acceptable
               error = NO_ERROR;
               //We are done
               break;
            }
         }
      }
   }

   //Return status code
   return error;
}


/**
 * @brief IPv4 source address selection
 *
 * This function selects the source address and the relevant network interface
 * to be used in order to join the specified destination address
 *
 * @param[in,out] interface A pointer to a valid network interface may be provided as
 *   a hint. The function returns a pointer identifying the interface to be used
 * @param[in] destAddr Destination IPv4 address
 * @param[out] srcAddr Local IPv4 address to be used
 * @return Error code
 **/

error_t ipv4SelectSourceAddr(NetInterface **interface,
   Ipv4Addr destAddr, Ipv4Addr *srcAddr)
{
   error_t error;
   uint_t i;
   uint_t j;
   NetInterface *currentInterface;
   NetInterface *bestInterface;
   Ipv4AddrEntry *currentAddr;
   Ipv4AddrEntry *bestAddr;

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

      //A sort of the candidate source addresses is being performed
      for(j = 0; j < IPV4_ADDR_LIST_SIZE; j++)
      {
         //Point to the current entry
         currentAddr = &currentInterface->ipv4Context.addrList[j];

         //Check the state of the address
         if(currentAddr->state == IPV4_ADDR_STATE_VALID)
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

            //Prefer same address
            if(bestAddr->addr == destAddr)
            {
               //Select the next address in the list
               continue;
            }
            else if(currentAddr->addr == destAddr)
            {
               //Give the current source address the higher precedence
               bestInterface = currentInterface;
               bestAddr = currentAddr;

               //Select the next address in the list
               continue;
            }

            //Check whether the destination address matches the default gateway
            if(bestAddr->defaultGateway == destAddr)
            {
               //Select the next address in the list
               continue;
            }
            else if(currentAddr->defaultGateway == destAddr)
            {
               //Give the current source address the higher precedence
               bestInterface = currentInterface;
               bestAddr = currentAddr;

               //Select the next address in the list
               continue;
            }

            //Prefer appropriate scope
            if(ipv4GetAddrScope(currentAddr->addr) < ipv4GetAddrScope(bestAddr->addr))
            {
               if(ipv4GetAddrScope(currentAddr->addr) >= ipv4GetAddrScope(destAddr))
               {
                  //Give the current source address the higher precedence
                  bestInterface = currentInterface;
                  bestAddr = currentAddr;
               }

               //Select the next address in the list
               continue;
            }
            else if(ipv4GetAddrScope(bestAddr->addr) < ipv4GetAddrScope(currentAddr->addr))
            {
               if(ipv4GetAddrScope(bestAddr->addr) < ipv4GetAddrScope(destAddr))
               {
                  //Give the current source address the higher precedence
                  bestInterface = currentInterface;
                  bestAddr = currentAddr;
               }

               //Select the next address in the list
               continue;
            }

            //If the destination address lies on one of the subnets to which
            //the host is directly connected, the corresponding source address
            //may be chosen (refer to RFC 1122, section 3.3.4.3)
            if(ipv4IsOnSubnet(bestAddr, destAddr))
            {
               if(ipv4IsOnSubnet(currentAddr, destAddr))
               {
                  //Use longest subnet mask
                  if(ipv4GetPrefixLength(currentAddr->subnetMask) >
                     ipv4GetPrefixLength(bestAddr->subnetMask))
                  {
                     //Give the current source address the higher precedence
                     bestInterface = currentInterface;
                     bestAddr = currentAddr;
                  }
               }

               //Select the next address in the list
               continue;
            }
            else
            {
               if(ipv4IsOnSubnet(currentAddr, destAddr))
               {
                  //Give the current source address the higher precedence
                  bestInterface = currentInterface;
                  bestAddr = currentAddr;

                  //Select the next address in the list
                  continue;
               }
            }

            //The default gateways may be consulted. If these gateways are
            //assigned to different interfaces, the interface corresponding
            //to the gateway with the highest preference may be chosen
            if(bestAddr->defaultGateway != IPV4_UNSPECIFIED_ADDR)
            {
               //Select the next address in the list
               continue;
            }
            else if(currentAddr->defaultGateway != IPV4_UNSPECIFIED_ADDR)
            {
               //Give the current source address the higher precedence
               bestInterface = currentInterface;
               bestAddr = currentAddr;

               //Select the next address in the list
               continue;
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
 * @brief Default gateway selection
 * @param[in] interface Underlying network interface
 * @param[in] srcAddr Source IPv4 address
 * @param[out] defaultGatewayAddr IPv4 address of the gateway
 * @return Error code
 **/

error_t ipv4SelectDefaultGateway(NetInterface *interface, Ipv4Addr srcAddr,
   Ipv4Addr *defaultGatewayAddr)
{
   uint_t i;
   Ipv4AddrEntry *entry;

   //Loop through the list of default gateways
   for(i = 0; i < IPV4_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv4Context.addrList[i];

      //Check whether the gateway address is valid
      if(entry->state == IPV4_ADDR_STATE_VALID &&
         entry->defaultGateway != IPV4_UNSPECIFIED_ADDR)
      {
         //Under the strong ES model, the source address is included as a
         //parameter in order to select a gateway that is directly reachable
         //on the corresponding physical interface (refer to RFC 1122,
         //section 3.3.4.2)
         if(entry->addr == srcAddr)
         {
            //Return the IPv4 address of the default gateway
            *defaultGatewayAddr = entry->defaultGateway;
            //Successful default gateway selection
            return NO_ERROR;
         }
      }
   }

   //No default gateway found
   return ERROR_NO_ROUTE;
}


/**
 * @brief Check whether an IPv4 address is on-link
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr IPv4 address to be checked
 * @return TRUE if the IPv4 address is on-link, else FALSE
 **/

bool_t ipv4IsOnLink(NetInterface *interface, Ipv4Addr ipAddr)
{
   uint_t i;
   Ipv4AddrEntry *entry;

   //Loop through the list of IPv4 addresses assigned to the interface
   for(i = 0; i < IPV4_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv4Context.addrList[i];

      //Valid entry?
      if(entry->state != IPV4_ADDR_STATE_INVALID)
      {
         //Check whether the specified IPv4 address belongs to the same subnet
         if(ipv4IsOnSubnet(entry, ipAddr))
         {
            //The specified IPv4 address is on-link
            return TRUE;
         }
      }
   }

   //The specified IPv4 address is off-link
   return FALSE;
}


/**
 * @brief Check whether an IPv4 address is a broadcast address
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr IPv4 address to be checked
 * @return TRUE if the IPv4 address is a broadcast address, else FALSE
 **/

bool_t ipv4IsBroadcastAddr(NetInterface *interface, Ipv4Addr ipAddr)
{
   uint_t i;
   Ipv4AddrEntry *entry;

   //Check whether the specified IPv4 address is the broadcast address
   if(ipAddr == IPV4_BROADCAST_ADDR)
      return TRUE;

   //Loop through the list of IPv4 addresses assigned to the interface
   for(i = 0; i < IPV4_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv4Context.addrList[i];

      //Valid entry?
      if(entry->state != IPV4_ADDR_STATE_INVALID)
      {
         //Check whether the specified IPv4 address belongs to the same subnet
         if(ipv4IsOnSubnet(entry, ipAddr))
         {
            //Make sure the subnet mask is not 255.255.255.255
            if(entry->subnetMask != IPV4_BROADCAST_ADDR)
            {
               //Directed broadcast address?
               if((ipAddr | entry->subnetMask) == IPV4_BROADCAST_ADDR)
               {
                  return TRUE;
               }
            }
         }
      }
   }

   //The specified IPv4 address is not a broadcast address
   return FALSE;
}


/**
 * @brief Check whether an IPv4 address is a tentative address
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr IPv4 address to be checked
 * @return TRUE if the IPv4 address is a tentative address, else FALSE
 **/

bool_t ipv4IsTentativeAddr(NetInterface *interface, Ipv4Addr ipAddr)
{
   uint_t i;
   Ipv4AddrEntry *entry;

   //Loop through the list of IPv4 addresses assigned to the interface
   for(i = 0; i < IPV4_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv4Context.addrList[i];

      //Tentative address?
      if(entry->state == IPV4_ADDR_STATE_TENTATIVE)
      {
         //Check whether the specified address matches a valid unicast
         //address assigned to the interface
         if(entry->addr == ipAddr)
         {
            //The specified IPv4 address is a tentative address
            return TRUE;
         }
      }
   }

   //The specified IPv4 address is not a tentative address
   return FALSE;
}


/**
 * @brief Check whether the specified IPv4 is assigned to the host
 * @param[in] ipAddr IPv4 address to be checked
 * @return TRUE if the IPv4 address matches any address assigned to the host,
 *   else FALSE
 **/

bool_t ipv4IsLocalHostAddr(Ipv4Addr ipAddr)
{
   uint_t i;
   uint_t j;
   bool_t flag;
   NetInterface *interface;
   Ipv4AddrEntry *entry;

   //Initialize flag
   flag = FALSE;

   //Loopback address?
   if(ipv4IsLoopbackAddr(ipAddr))
   {
      //The 127.0.0.0/8 block is assigned for use as the host loopback address.
      //A datagram sent by a higher-level protocol to an address anywhere within
      //this block loops back inside the host (refer to RFC 5735, section 3)
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
         for(j = 0; j < IPV4_ADDR_LIST_SIZE && !flag; j++)
         {
            //Point to the current entry
            entry = &interface->ipv4Context.addrList[j];

            //Check whether the specified IPv4 address matches any address
            //assigned to the host
            if(entry->state == IPV4_ADDR_STATE_VALID &&
               entry->addr == ipAddr)
            {
               flag = TRUE;
            }
         }
      }
   }

   //Return TRUE if the specified address matches any address assigned to the host
   return flag;
}


/**
 * @brief Retrieve the scope of an IPv4 address
 * @param[in] ipAddr IPv4 address
 * @return IPv4 address scope
 **/

uint_t ipv4GetAddrScope(Ipv4Addr ipAddr)
{
   uint_t scope;

   //Broadcast address?
   if(ipAddr == IPV4_BROADCAST_ADDR)
   {
      //The broadcast address is never forwarded by the routers connecting
      //the local network to other networks
      scope = IPV4_ADDR_SCOPE_LINK_LOCAL;
   }
   //Multicast address?
   else if(ipv4IsMulticastAddr(ipAddr))
   {
      //Local Network Control Block?
      if((ipAddr & IPV4_MULTICAST_LNCB_MASK) == IPV4_MULTICAST_LNCB_PREFIX)
      {
         //Addresses in the Local Network Control Block are used for protocol
         //control traffic that is not forwarded off link
         scope = IPV4_ADDR_SCOPE_LINK_LOCAL;
      }
      //Any other multicast address?
      else
      {
         //Other addresses are assigned global scope
         scope = IPV4_ADDR_SCOPE_GLOBAL;
      }
   }
   //Unicast address?
   else
   {
      //Loopback address?
      if((ipAddr & IPV4_LOOPBACK_MASK) == IPV4_LOOPBACK_PREFIX)
      {
         //IPv4 loopback addresses, which have the prefix 127.0.0.0/8,
         //are assigned interface-local scope
         scope = IPV4_ADDR_SCOPE_INTERFACE_LOCAL;
      }
      //Link-local address?
      else if((ipAddr & IPV4_LINK_LOCAL_MASK) == IPV4_LINK_LOCAL_PREFIX)
      {
         //IPv4 auto-configuration addresses, which have the prefix
         //169.254.0.0/16, are assigned link-local scope
         scope = IPV4_ADDR_SCOPE_LINK_LOCAL;
      }
      //Any other unicast address?
      else
      {
         //Other addresses are assigned global scope
         scope = IPV4_ADDR_SCOPE_GLOBAL;
      }
   }

   //Return the scope of the specified IPv4 address
   return scope;
}


/**
 * @brief Calculate prefix length for a given subnet mask
 * @param[in] mask Subnet mask
 * @return Prefix length
 **/

uint_t ipv4GetPrefixLength(Ipv4Addr mask)
{
   uint_t i;

   //Convert from network byte order to host byte order
   mask = ntohl(mask);

   //Count of the number of leading 1 bits in the network mask
   for(i = 0; i < 32; i++)
   {
      //Check the value of the current bit
      if(!(mask & (1U << (31 - i))))
         break;
   }

   //Return prefix length
   return i;
}


/**
 * @brief Get IPv4 broadcast address
 * @param[in] interface Pointer to the desired network interface
 * @param[out] addr IPv4 broadcast address
 **/

error_t ipv4GetBroadcastAddr(NetInterface *interface, Ipv4Addr *addr)
{
   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //The broadcast address is obtained by performing a bitwise OR operation
   //between the bit complement of the subnet mask and the host IP address
   *addr = interface->ipv4Context.addrList[0].addr;
   *addr |= ~interface->ipv4Context.addrList[0].subnetMask;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Map an host group address to a MAC-layer multicast address
 * @param[in] ipAddr IPv4 host group address
 * @param[out] macAddr Corresponding MAC-layer multicast address
 * @return Error code
 **/

error_t ipv4MapMulticastAddrToMac(Ipv4Addr ipAddr, MacAddr *macAddr)
{
   uint8_t *p;

   //Ensure the specified IPv4 address is a valid host group address
   if(!ipv4IsMulticastAddr(ipAddr))
      return ERROR_INVALID_ADDRESS;

   //Cast the address to byte array
   p = (uint8_t *) &ipAddr;

   //An IP host group address is mapped to an Ethernet multicast address
   //by placing the low-order 23-bits of the IP address into the low-order
   //23 bits of the Ethernet multicast address 01-00-5E-00-00-00
   macAddr->b[0] = 0x01;
   macAddr->b[1] = 0x00;
   macAddr->b[2] = 0x5E;
   macAddr->b[3] = p[1] & 0x7F;
   macAddr->b[4] = p[2];
   macAddr->b[5] = p[3];

   //The specified host group address was successfully
   //mapped to a MAC-layer address
   return NO_ERROR;
}


/**
 * @brief Update IPv4 input statistics
 * @param[in] interface Underlying network interface
 * @param[in] destIpAddr Destination IP address
 * @param[in] length Length of the incoming IP packet
 **/

void ipv4UpdateInStats(NetInterface *interface, Ipv4Addr destIpAddr,
   size_t length)
{
   //Check whether the destination address is a unicast, broadcast or multicast address
   if(ipv4IsBroadcastAddr(interface, destIpAddr))
   {
      //Number of IP broadcast datagrams transmitted
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsInBcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCInBcastPkts, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsInBcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCInBcastPkts, 1);
   }
   else if(ipv4IsMulticastAddr(destIpAddr))
   {
      //Number of IP multicast datagrams transmitted
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsInMcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCInMcastPkts, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsInMcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCInMcastPkts, 1);

      //Total number of octets transmitted in IP multicast datagrams
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsInMcastOctets, length);
      IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCInMcastOctets, length);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsInMcastOctets, length);
      IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCInMcastOctets, length);
   }
}


/**
 * @brief Update IPv4 output statistics
 * @param[in] interface Underlying network interface
 * @param[in] destIpAddr Destination IP address
 * @param[in] length Length of the outgoing IP packet
 **/

void ipv4UpdateOutStats(NetInterface *interface, Ipv4Addr destIpAddr,
   size_t length)
{
   //Check whether the destination address is a unicast, broadcast or multicast address
   if(ipv4IsBroadcastAddr(interface, destIpAddr))
   {
      //Number of IP broadcast datagrams transmitted
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsOutBcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCOutBcastPkts, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsOutBcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCOutBcastPkts, 1);
   }
   else if(ipv4IsMulticastAddr(destIpAddr))
   {
      //Number of IP multicast datagrams transmitted
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsOutMcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCOutMcastPkts, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsOutMcastPkts, 1);
      IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCOutMcastPkts, 1);

      //Total number of octets transmitted in IP multicast datagrams
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsOutMcastOctets, length);
      IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCOutMcastOctets, length);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsOutMcastOctets, length);
      IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCOutMcastOctets, length);
   }

   //Total number of IP datagrams that this entity supplied to the lower
   //layers for transmission
   IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsOutTransmits, 1);
   IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCOutTransmits, 1);
   IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsOutTransmits, 1);
   IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCOutTransmits, 1);

   //Total number of octets in IP datagrams delivered to the lower layers
   //for transmission
   IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsOutOctets, length);
   IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCOutOctets, length);
   IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsOutOctets, length);
   IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCOutOctets, length);
}


/**
 * @brief Update Ethernet error statistics
 * @param[in] interface Underlying network interface
 * @param[in] error Status code describing the error
 **/

void ipv4UpdateErrorStats(NetInterface *interface, error_t error)
{
   //Check error code
   switch(error)
   {
   case ERROR_INVALID_HEADER:
      //Number of input datagrams discarded due to errors in their IP headers
      MIB2_INC_COUNTER32(ipGroup.ipInHdrErrors, 1);
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsInHdrErrors, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsInHdrErrors, 1);
      break;
   case ERROR_INVALID_ADDRESS:
      //Number of input datagrams discarded because the destination IP address
      //was not a valid address
      MIB2_INC_COUNTER32(ipGroup.ipInAddrErrors, 1);
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsInAddrErrors, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsInAddrErrors, 1);
      break;
   case ERROR_PROTOCOL_UNREACHABLE:
      //Number of locally-addressed datagrams received successfully but discarded
      //because of an unknown or unsupported protocol
      MIB2_INC_COUNTER32(ipGroup.ipInUnknownProtos, 1);
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsInUnknownProtos, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsInUnknownProtos, 1);
      break;
   case ERROR_INVALID_LENGTH:
      //Number of input IP datagrams discarded because the datagram frame
      //didn't carry enough data
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsInTruncatedPkts, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsInTruncatedPkts, 1);
      break;
   default:
      //Just for sanity
      break;
   }
}

#endif
