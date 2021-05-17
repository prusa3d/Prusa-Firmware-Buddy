/**
 * @file ipv6.c
 * @brief IPv6 (Internet Protocol Version 6)
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
 * IP version 6 (IPv6) is a new version of the Internet Protocol, designed
 * as the successor to IP version 4 (IPv4). Refer to RFC 2460
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL IPV6_TRACE_LEVEL

//Dependencies
#include <string.h>
#include <ctype.h>
#include "core/net.h"
#include "core/ip.h"
#include "core/udp.h"
#include "core/tcp_fsm.h"
#include "core/raw_socket.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_frag.h"
#include "ipv6/ipv6_misc.h"
#include "ipv6/ipv6_pmtu.h"
#include "ipv6/ipv6_routing.h"
#include "ipv6/icmpv6.h"
#include "ipv6/mld.h"
#include "ipv6/ndp.h"
#include "ipv6/ndp_cache.h"
#include "ipv6/ndp_misc.h"
#include "ipv6/ndp_router_adv.h"
#include "ipv6/slaac.h"
#include "dhcpv6/dhcpv6_client.h"
#include "mibs/ip_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED)

//Unspecified IPv6 address
const Ipv6Addr IPV6_UNSPECIFIED_ADDR =
   IPV6_ADDR(0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000);

//Loopback IPv6 address
const Ipv6Addr IPV6_LOOPBACK_ADDR =
   IPV6_ADDR(0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001);

//Link-local All-Nodes IPv6 address
const Ipv6Addr IPV6_LINK_LOCAL_ALL_NODES_ADDR =
   IPV6_ADDR(0xFF02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001);

//Link-local All-Routers IPv6 address
const Ipv6Addr IPV6_LINK_LOCAL_ALL_ROUTERS_ADDR =
   IPV6_ADDR(0xFF02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0002);

//Link-local IPv6 address prefix
const Ipv6Addr IPV6_LINK_LOCAL_ADDR_PREFIX =
   IPV6_ADDR(0xFE80, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000);

//Solicited-node IPv6 address prefix
const Ipv6Addr IPV6_SOLICITED_NODE_ADDR_PREFIX =
   IPV6_ADDR(0xFF02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0xFF00, 0x0000);


/**
 * @brief IPv6 related initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ipv6Init(NetInterface *interface)
{
   Ipv6Context *context;
   NetInterface *physicalInterface;

   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);

   //Point to the IPv6 context
   context = &interface->ipv6Context;

   //Clear the IPv6 context
   osMemset(context, 0, sizeof(Ipv6Context));

   //Initialize interface specific variables
   context->linkMtu = physicalInterface->nicDriver->mtu;
   context->isRouter = FALSE;
   context->curHopLimit = IPV6_DEFAULT_HOP_LIMIT;

   //Multicast ICMPv6 Echo Request messages are allowed by default
   context->enableMulticastEchoReq = TRUE;

   //Initialize the list of IPv6 addresses assigned to the interface
   osMemset(context->addrList, 0, sizeof(context->addrList));
   //Initialize the Prefix List
   osMemset(context->prefixList, 0, sizeof(context->prefixList));
   //Initialize the Default Router List
   osMemset(context->routerList, 0, sizeof(context->routerList));
   //Initialize the list of DNS servers
   osMemset(context->dnsServerList, 0, sizeof(context->dnsServerList));
   //Initialize the multicast filter table
   osMemset(context->multicastFilter, 0, sizeof(context->multicastFilter));

#if (IPV6_FRAG_SUPPORT == ENABLED)
   //Identification field is used to identify fragments of an original IP datagram
   context->identification = 0;
   //Initialize the reassembly queue
   osMemset(context->fragQueue, 0, sizeof(context->fragQueue));
#endif

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Change the MTU of a network interface
 * @param[in] interface Pointer to the desired network interface
 * @param[in] mtu Maximum transmit unit
 * @return Error code
 **/

error_t ipv6SetMtu(NetInterface *interface, size_t mtu)
{
   error_t error;
   NetInterface *physicalInterface;

   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);

   //Make sure the specified MTU is greater than or equal to the minimum
   //IPv6 MTU and does not exceed the maximum MTU of the interface
   if(mtu >= IPV6_DEFAULT_MTU && mtu <= physicalInterface->nicDriver->mtu)
   {
      //Set the MTU to be used
      interface->ipv6Context.linkMtu = mtu;
      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The specified MTU is not valid
      error = ERROR_OUT_OF_RANGE;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief Retrieve the MTU for the specified interface
 * @param[in] interface Pointer to the desired network interface
 * @param[out] mtu Maximum transmit unit
 * @return Error code
 **/

error_t ipv6GetMtu(NetInterface *interface, size_t *mtu)
{
   //Check parameters
   if(interface == NULL || mtu == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Return the current MTU value
   *mtu = interface->ipv6Context.linkMtu;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Assign link-local address
 * @param[in] interface Pointer to the desired network interface
 * @param[in] addr Link-local address
 * @return Error code
 **/

error_t ipv6SetLinkLocalAddr(NetInterface *interface, const Ipv6Addr *addr)
{
   error_t error;

   //Get exclusive access
   osAcquireMutex(&netMutex);

#if (NDP_SUPPORT == ENABLED)
   //Check whether Duplicate Address Detection should be performed
   if(interface->ndpContext.dupAddrDetectTransmits > 0)
   {
      //Use the link-local address as a tentative address
      error = ipv6SetAddr(interface, 0, addr, IPV6_ADDR_STATE_TENTATIVE,
         NDP_INFINITE_LIFETIME, NDP_INFINITE_LIFETIME, TRUE);
   }
   else
#endif
   {
      //The use of the link-local address is now unrestricted
      error = ipv6SetAddr(interface, 0, addr, IPV6_ADDR_STATE_PREFERRED,
         NDP_INFINITE_LIFETIME, NDP_INFINITE_LIFETIME, TRUE);
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief Retrieve link-local address
 * @param[in] interface Pointer to the desired network interface
 * @param[out] addr link-local address
 * @return Error code
 **/

error_t ipv6GetLinkLocalAddr(NetInterface *interface, Ipv6Addr *addr)
{
   Ipv6AddrEntry *entry;

   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the corresponding entry
   entry = &interface->ipv6Context.addrList[0];

   //Check whether the IPv6 address is valid
   if(entry->state == IPV6_ADDR_STATE_PREFERRED ||
      entry->state == IPV6_ADDR_STATE_DEPRECATED)
   {
      //Get IPv6 address
      *addr = entry->addr;
   }
   else
   {
      //Return the unspecified address when no address has been assigned
      *addr = IPV6_UNSPECIFIED_ADDR;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Assign global address
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[in] addr Global address
 * @return Error code
 **/

error_t ipv6SetGlobalAddr(NetInterface *interface, uint_t index, const Ipv6Addr *addr)
{
   error_t error;

   //Get exclusive access
   osAcquireMutex(&netMutex);

#if (NDP_SUPPORT == ENABLED)
   //Check whether Duplicate Address Detection should be performed
   if(interface->ndpContext.dupAddrDetectTransmits > 0)
   {
      //Use the global address as a tentative address
      error = ipv6SetAddr(interface, index + 1, addr, IPV6_ADDR_STATE_TENTATIVE,
         NDP_INFINITE_LIFETIME, NDP_INFINITE_LIFETIME, TRUE);
   }
   else
#endif
   {
      //The use of the global address is now unrestricted
      error = ipv6SetAddr(interface, index + 1, addr, IPV6_ADDR_STATE_PREFERRED,
         NDP_INFINITE_LIFETIME, NDP_INFINITE_LIFETIME, TRUE);
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief Retrieve global address
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[out] addr Global address
 * @return Error code
 **/

error_t ipv6GetGlobalAddr(NetInterface *interface, uint_t index, Ipv6Addr *addr)
{
   Ipv6AddrEntry *entry;

   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if((index + 1) >= IPV6_ADDR_LIST_SIZE)
   {
      //Return the unspecified address when the index is out of range
      *addr = IPV6_UNSPECIFIED_ADDR;
      //Report an error
      return ERROR_OUT_OF_RANGE;
   }

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the corresponding entry
   entry = &interface->ipv6Context.addrList[index + 1];

   //Check whether the IPv6 address is valid
   if(entry->state == IPV6_ADDR_STATE_PREFERRED ||
      entry->state == IPV6_ADDR_STATE_DEPRECATED)
   {
      //Get IPv6 address
      *addr = entry->addr;
   }
   else
   {
      //Return the unspecified address when no address has been assigned
      *addr = IPV6_UNSPECIFIED_ADDR;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Assign anycast address
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[in] addr Anycast address
 * @return Error code
 **/

error_t ipv6SetAnycastAddr(NetInterface *interface, uint_t index, const Ipv6Addr *addr)
{
   error_t error;
   NetInterface *physicalInterface;
   Ipv6Addr *anycastAddrList;
   Ipv6Addr solicitedNodeAddr;

   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV6_ANYCAST_ADDR_LIST_SIZE)
      return ERROR_OUT_OF_RANGE;

   //The IPv6 address must be a valid unicast address
   if(ipv6IsMulticastAddr(addr))
      return ERROR_INVALID_ADDRESS;

   //Initialize status code
   error = NO_ERROR;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);

   //Point to the list of anycast addresses assigned to the interface
   anycastAddrList = interface->ipv6Context.anycastAddrList;

   //Check whether an anycast address is already assigned
   if(!ipv6CompAddr(&anycastAddrList[index], &IPV6_UNSPECIFIED_ADDR))
   {
      //Ethernet interface?
      if(physicalInterface->nicDriver != NULL &&
         physicalInterface->nicDriver->type == NIC_TYPE_ETHERNET)
      {
         //Form the Solicited-Node address
         ipv6ComputeSolicitedNodeAddr(&anycastAddrList[index], &solicitedNodeAddr);
         //Leave the Solicited-Node multicast group
         ipv6LeaveMulticastGroup(interface, &solicitedNodeAddr);
      }
   }

   //Assign the specified anycast address to the interface
   anycastAddrList[index] = *addr;

   //Check whether the anycast address is valid
   if(!ipv6CompAddr(addr, &IPV6_UNSPECIFIED_ADDR))
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
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief Retrieve anycast address
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[out] addr Anycast address
 * @return Error code
 **/

error_t ipv6GetAnycastAddr(NetInterface *interface, uint_t index, Ipv6Addr *addr)
{
   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV6_ANYCAST_ADDR_LIST_SIZE)
   {
      //Return the unspecified address when the index is out of range
      *addr = IPV6_UNSPECIFIED_ADDR;
      //Report an error
      return ERROR_OUT_OF_RANGE;
   }

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Return the corresponding entry
   *addr = interface->ipv6Context.anycastAddrList[index];
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Configure IPv6 prefix
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[in] prefix IPv6 prefix
 * @param[in] length The number of leading bits in the prefix that are valid
 **/

error_t ipv6SetPrefix(NetInterface *interface,
   uint_t index, const Ipv6Addr *prefix, uint_t length)
{
   Ipv6PrefixEntry *entry;

   //Check parameters
   if(interface == NULL || prefix == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV6_PREFIX_LIST_SIZE)
      return ERROR_OUT_OF_RANGE;

   //Make sure the prefix length is valid
   if(length >= 128)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the corresponding entry
   entry = &interface->ipv6Context.prefixList[index];

   //Set up IPv6 prefix
   entry->prefix = *prefix;
   entry->prefixLen = length;

   //Check prefix length
   if(length > 0)
   {
      //Set On-link and Autonomous flags
      entry->onLinkFlag = TRUE;
      entry->autonomousFlag = FALSE;

      //Manually assigned prefixes have infinite lifetime
      entry->validLifetime = INFINITE_DELAY;
      entry->preferredLifetime = INFINITE_DELAY;
      entry->permanent = TRUE;
   }
   else
   {
      //Immediately time-out the entry
      entry->validLifetime = 0;
      entry->preferredLifetime = 0;
      entry->permanent = FALSE;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve IPv6 prefix
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[out] prefix IPv6 prefix
 * @param[out] length The number of leading bits in the prefix that are valid
 * @return Error code
 **/

error_t ipv6GetPrefix(NetInterface *interface,
   uint_t index, Ipv6Addr *prefix, uint_t *length)
{
   Ipv6PrefixEntry *entry;

   //Check parameters
   if(interface == NULL || prefix == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV6_PREFIX_LIST_SIZE)
   {
      //Return the ::/0 prefix when the index is out of range
      *prefix = IPV6_UNSPECIFIED_ADDR;
      *length = 0;
      //Report an error
      return ERROR_OUT_OF_RANGE;
   }

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the corresponding entry
   entry = &interface->ipv6Context.prefixList[index];

   //Check whether the prefix is valid
   if(entry->validLifetime > 0)
   {
      //Get IPv6 prefix
      *prefix = entry->prefix;
      *length = entry->prefixLen;
   }
   else
   {
      //Return the ::/0 prefix when the valid lifetime has expired
      *prefix = IPV6_UNSPECIFIED_ADDR;
      *length = 0;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Configure default router
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[in] addr Default router address
 * @return Error code
 **/

error_t ipv6SetDefaultRouter(NetInterface *interface, uint_t index, const Ipv6Addr *addr)
{
   Ipv6RouterEntry *entry;

   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV6_ROUTER_LIST_SIZE)
      return ERROR_OUT_OF_RANGE;

   //The IPv6 address must be a valid unicast address
   if(ipv6IsMulticastAddr(addr))
      return ERROR_INVALID_ADDRESS;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the corresponding entry
   entry = &interface->ipv6Context.routerList[index];

   //Set up router address
   entry->addr = *addr;
   //Preference value
   entry->preference = 0;

   //Valid IPv6 address?
   if(!ipv6CompAddr(addr, &IPV6_UNSPECIFIED_ADDR))
   {
      //Manually assigned routers have infinite lifetime
      entry->lifetime = INFINITE_DELAY;
      entry->permanent = TRUE;
   }
   else
   {
      //Immediately time-out the entry
      entry->lifetime = 0;
      entry->permanent = FALSE;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve default router
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[out] addr Default router address
 * @return Error code
 **/

error_t ipv6GetDefaultRouter(NetInterface *interface, uint_t index, Ipv6Addr *addr)
{
   Ipv6RouterEntry *entry;

   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV6_ROUTER_LIST_SIZE)
   {
      //Return the unspecified address when the index is out of range
      *addr = IPV6_UNSPECIFIED_ADDR;
      //Report an error
      return ERROR_OUT_OF_RANGE;
   }

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the corresponding entry
   entry = &interface->ipv6Context.routerList[index];

   //Check the lifetime of the entry
   if(entry->lifetime > 0)
   {
      //Get router address
      *addr = entry->addr;
   }
   else
   {
      //Return the unspecified address when the lifetime has expired
      *addr = IPV6_UNSPECIFIED_ADDR;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Configure DNS server
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index This parameter selects between the primary and secondary DNS server
 * @param[in] addr DNS server address
 * @return Error code
 **/

error_t ipv6SetDnsServer(NetInterface *interface, uint_t index, const Ipv6Addr *addr)
{
   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV6_DNS_SERVER_LIST_SIZE)
      return ERROR_OUT_OF_RANGE;

   //The IPv6 address must be a valid unicast address
   if(ipv6IsMulticastAddr(addr))
      return ERROR_INVALID_ADDRESS;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set up DNS server address
   interface->ipv6Context.dnsServerList[index] = *addr;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve DNS server
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index This parameter selects between the primary and secondary DNS server
 * @param[out] addr DNS server address
 * @return Error code
 **/

error_t ipv6GetDnsServer(NetInterface *interface, uint_t index, Ipv6Addr *addr)
{
   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV6_DNS_SERVER_LIST_SIZE)
   {
      //Return the unspecified address when the index is out of range
      *addr = IPV6_UNSPECIFIED_ADDR;
      //Report an error
      return ERROR_OUT_OF_RANGE;
   }

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Get DNS server address
   *addr = interface->ipv6Context.dnsServerList[index];
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Callback function for link change event
 * @param[in] interface Underlying network interface
 **/

void ipv6LinkChangeEvent(NetInterface *interface)
{
   uint_t i;
   Ipv6Context *context;
   Ipv6AddrEntry *entry;
   NetInterface *physicalInterface;

   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);

   //Point to the IPv6 context
   context = &interface->ipv6Context;

   //Restore default parameters
   context->linkMtu = physicalInterface->nicDriver->mtu;
   context->curHopLimit = IPV6_DEFAULT_HOP_LIMIT;

   //Clear the list of IPv6 addresses
   ipv6FlushAddrList(interface);
   //Clear the Prefix List
   ipv6FlushPrefixList(interface);
   //Clear the Default Router List
   ipv6FlushDefaultRouterList(interface);

#if (IPV6_FRAG_SUPPORT == ENABLED)
   //Flush the reassembly queue
   ipv6FlushFragQueue(interface);
#endif

#if (MLD_SUPPORT == ENABLED)
   //Notify MLD of link state changes
   mldLinkChangeEvent(interface);
#endif

#if (NDP_SUPPORT == ENABLED)
   //Notify NDP of link state changes
   ndpLinkChangeEvent(interface);
#endif

#if (NDP_ROUTER_ADV_SUPPORT == ENABLED)
   //Notify the RA service of link state changes
   ndpRouterAdvLinkChangeEvent(interface->ndpRouterAdvContext);
#endif

#if (SLAAC_SUPPORT == ENABLED)
   //Notify the SLAAC service of link state changes
   slaacLinkChangeEvent(interface->slaacContext);
#endif

#if (DHCPV6_CLIENT_SUPPORT == ENABLED)
   //Notify the DHCPv6 client of link state changes
   dhcpv6ClientLinkChangeEvent(interface->dhcpv6ClientContext);
#endif

   //Go through the list of IPv6 addresses
   for(i = 0; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->addrList[i];

      //Check whether the IPv6 address has been manually assigned
      if(entry->permanent)
      {
#if (NDP_SUPPORT == ENABLED)
         //Check whether Duplicate Address Detection should be performed
         if(interface->ndpContext.dupAddrDetectTransmits > 0)
         {
            //Use the IPv6 address as a tentative address
            ipv6SetAddr(interface, i, &entry->addr, IPV6_ADDR_STATE_TENTATIVE,
               NDP_INFINITE_LIFETIME, NDP_INFINITE_LIFETIME, TRUE);
         }
         else
#endif
         {
            //The use of the IPv6 address is now unrestricted
            ipv6SetAddr(interface, i, &entry->addr, IPV6_ADDR_STATE_PREFERRED,
               NDP_INFINITE_LIFETIME, NDP_INFINITE_LIFETIME, TRUE);
         }
      }
   }
}


/**
 * @brief Incoming IPv6 packet processing
 * @param[in] interface Underlying network interface
 * @param[in] ipPacket Multi-part buffer that holds the incoming IPv6 packet
 * @param[in] ipPacketOffset Offset to the first byte of the IPv6 packet
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 **/

void ipv6ProcessPacket(NetInterface *interface, NetBuffer *ipPacket,
   size_t ipPacketOffset, NetRxAncillary *ancillary)
{
   error_t error;
   size_t i;
   size_t length;
   size_t nextHeaderOffset;
   uint8_t *type;
   Ipv6Header *ipHeader;
   IpPseudoHeader pseudoHeader;

   //Total number of input datagrams received, including those received in error
   IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInReceives, 1);
   IP_MIB_INC_COUNTER64(ipv6SystemStats.ipSystemStatsHCInReceives, 1);
   IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInReceives, 1);
   IP_MIB_INC_COUNTER64(ipv6IfStatsTable[interface->index].ipIfStatsHCInReceives, 1);

   //Retrieve the length of the IPv6 packet
   length = netBufferGetLength(ipPacket);

   //Total number of octets received in input IP datagrams
   IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInOctets, length);
   IP_MIB_INC_COUNTER64(ipv6SystemStats.ipSystemStatsHCInOctets, length);
   IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInOctets, length);
   IP_MIB_INC_COUNTER64(ipv6IfStatsTable[interface->index].ipIfStatsHCInOctets, length);

   //Ensure the packet length is greater than 40 bytes
   if(length < sizeof(Ipv6Header))
   {
      //Number of input IP datagrams discarded because the datagram frame
      //didn't carry enough data
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInTruncatedPkts, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInTruncatedPkts, 1);

      //Discard the received packet
      return;
   }

   //Point to the IPv6 header
   ipHeader = netBufferAt(ipPacket, ipPacketOffset);
   //Sanity check
   if(ipHeader == NULL)
      return;

   //Debug message
   TRACE_INFO("IPv6 packet received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump IPv6 header contents for debugging purpose
   ipv6DumpHeader(ipHeader);

   //Check IP version number
   if(ipHeader->version != IPV6_VERSION)
   {
      //Number of input datagrams discarded due to errors in their IP headers
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInHdrErrors, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInHdrErrors, 1);

      //Discard the received packet
      return;
   }

   //Ensure the payload length is correct before processing the packet
   if(ntohs(ipHeader->payloadLen) > (length - sizeof(Ipv6Header)))
   {
      //Number of input IP datagrams discarded because the datagram frame
      //didn't carry enough data
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInTruncatedPkts, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInTruncatedPkts, 1);

      //Discard the received packet
      return;
   }

   //Source address filtering
   if(ipv6CheckSourceAddr(interface, &ipHeader->srcAddr))
   {
      //Number of input datagrams discarded due to errors in their IP headers
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInHdrErrors, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInHdrErrors, 1);

      //Discard the received packet
      return;
   }

#if defined(IPV6_PACKET_FORWARD_HOOK)
   IPV6_PACKET_FORWARD_HOOK(interface, ipPacket, ipPacketOffset);
#else
   //Destination address filtering
   if(ipv6CheckDestAddr(interface, &ipHeader->destAddr))
   {
#if (IPV6_ROUTING_SUPPORT == ENABLED)
      //Forward the packet according to the routing table
      ipv6ForwardPacket(interface, ipPacket, ipPacketOffset);
#else
      //Number of input datagrams discarded because the destination IP address
      //was not a valid address
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInAddrErrors, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInAddrErrors, 1);
#endif
      //We are done
      return;
   }
#endif

   //Update IP statistics
   ipv6UpdateInStats(interface, &ipHeader->destAddr, length);

   //Calculate the effective length of the multi-part buffer
   length = ipPacketOffset + sizeof(Ipv6Header) +
      ntohs(ipHeader->payloadLen);

   //Adjust the length of the multi-part buffer if necessary
   netBufferSetLength(ipPacket, length);

   //Form the IPv6 pseudo header
   pseudoHeader.length = sizeof(Ipv6PseudoHeader);
   pseudoHeader.ipv6Data.srcAddr = ipHeader->srcAddr;
   pseudoHeader.ipv6Data.destAddr = ipHeader->destAddr;
   pseudoHeader.ipv6Data.reserved[0] = 0;
   pseudoHeader.ipv6Data.reserved[1] = 0;
   pseudoHeader.ipv6Data.reserved[2] = 0;

   //Save Hop Limit value
   ancillary->ttl = ipHeader->hopLimit;

   //Keep track of Next Header field
   nextHeaderOffset = ipPacketOffset + &ipHeader->nextHeader -
      (uint8_t *) ipHeader;

   //Point to the first extension header
   i = ipPacketOffset + sizeof(Ipv6Header);

   //Parse extension headers
   while(i < length)
   {
      //Retrieve the Next Header field of preceding header
      type = netBufferAt(ipPacket, nextHeaderOffset);
      //Sanity check
      if(type == NULL)
         return;

      //Update IPv6 pseudo header
      pseudoHeader.ipv6Data.length = htonl(length - i);
      pseudoHeader.ipv6Data.nextHeader = *type;

      //Each extension header is identified by the Next Header field of the
      //preceding header
      switch(*type)
      {
      //Hop-by-Hop Options header?
      case IPV6_HOP_BY_HOP_OPT_HEADER:
         //Parse current extension header
         error = ipv6ParseHopByHopOptHeader(interface, ipPacket, ipPacketOffset,
            &i, &nextHeaderOffset);
         //Continue processing
         break;

      //Destination Options header?
      case IPV6_DEST_OPT_HEADER:
         //Parse current extension header
         error = ipv6ParseDestOptHeader(interface, ipPacket, ipPacketOffset,
            &i, &nextHeaderOffset);
         //Continue processing
         break;

      //Routing header?
      case IPV6_ROUTING_HEADER:
         //Parse current extension header
         error = ipv6ParseRoutingHeader(interface, ipPacket, ipPacketOffset,
            &i, &nextHeaderOffset);
         //Continue processing
         break;

      //Fragment header?
      case IPV6_FRAGMENT_HEADER:
#if (IPV6_FRAG_SUPPORT == ENABLED)
         //Parse current extension header
         ipv6ParseFragmentHeader(interface, ipPacket, ipPacketOffset,
            i, nextHeaderOffset, ancillary);
#endif
         //Exit immediately
         return;

      //Authentication header?
      case IPV6_AH_HEADER:
         //Parse current extension header
         error = ipv6ParseAuthHeader(interface, ipPacket, ipPacketOffset,
            &i, &nextHeaderOffset);
         //Continue processing
         break;

      //Encapsulating Security Payload header?
      case IPV6_ESP_HEADER:
         //Parse current extension header
         error = ipv6ParseEspHeader(interface, ipPacket, ipPacketOffset,
            &i, &nextHeaderOffset);
         //Continue processing
         break;

      //ICMPv6 header?
      case IPV6_ICMPV6_HEADER:
         //Process incoming ICMPv6 message
         icmpv6ProcessMessage(interface, &pseudoHeader.ipv6Data, ipPacket,
            i, ipHeader->hopLimit);

#if (RAW_SOCKET_SUPPORT == ENABLED)
         //Packets addressed to the tentative address should be silently discarded
         if(!ipv6IsTentativeAddr(interface, &ipHeader->destAddr))
         {
            //Allow raw sockets to process ICMPv6 messages
            rawSocketProcessIpPacket(interface, &pseudoHeader, ipPacket, i,
               ancillary);
         }
#endif
         //Exit immediately
         return;

#if (TCP_SUPPORT == ENABLED)
      //TCP header?
      case IPV6_TCP_HEADER:
         //Packets addressed to the tentative address should be silently discarded
         if(!ipv6IsTentativeAddr(interface, &ipHeader->destAddr))
         {
            //Process incoming TCP segment
            tcpProcessSegment(interface, &pseudoHeader, ipPacket, i, ancillary);
         }
         else
         {
            //Number of input datagrams discarded because the destination IP address
            //was not a valid address
            IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInAddrErrors, 1);
            IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInAddrErrors, 1);
         }

         //Exit immediately
         return;
#endif

#if (UDP_SUPPORT == ENABLED)
      //UDP header?
      case IPV6_UDP_HEADER:
         //Packets addressed to the tentative address should be silently discarded
         if(!ipv6IsTentativeAddr(interface, &ipHeader->destAddr))
         {
            //Process incoming UDP datagram
            error = udpProcessDatagram(interface, &pseudoHeader, ipPacket, i,
               ancillary);

            //Unreachable port?
            if(error == ERROR_PORT_UNREACHABLE)
            {
               //A destination node should originate a Destination Unreachable
               //message with Code 4 in response to a packet for which the
               //transport protocol has no listener
               icmpv6SendErrorMessage(interface, ICMPV6_TYPE_DEST_UNREACHABLE,
                  ICMPV6_CODE_PORT_UNREACHABLE, 0, ipPacket, ipPacketOffset);
            }
         }
         else
         {
            //Number of input datagrams discarded because the destination IP address
            //was not a valid address
            IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInAddrErrors, 1);
            IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInAddrErrors, 1);
         }

         //Exit immediately
         return;
#endif

      //No next header?
      case IPV6_NO_NEXT_HEADER:
         //If the payload length field of the IPv6 header indicates the presence of
         //octets past the end of the previous header, these octets must be ignored
         return;

      //Unrecognized header type?
      default:
         //Debug message
         TRACE_WARNING("Unrecognized Next Header type\r\n");

         //Packets addressed to the tentative address should be silently discarded
         if(!ipv6IsTentativeAddr(interface, &ipHeader->destAddr))
         {
            //Compute the offset of the unrecognized Next Header field within the packet
            size_t n = nextHeaderOffset - ipPacketOffset;

            //Send an ICMP Parameter Problem message
            icmpv6SendErrorMessage(interface, ICMPV6_TYPE_PARAM_PROBLEM,
               ICMPV6_CODE_UNKNOWN_NEXT_HEADER, n, ipPacket, ipPacketOffset);
         }
         else
         {
            //Number of input datagrams discarded because the destination IP address
            //was not a valid address
            IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsInAddrErrors, 1);
            IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsInAddrErrors, 1);
         }

         //Discard incoming packet
         return;
      }

      //Any error while processing the current extension header?
      if(error)
         return;
   }
}


/**
 * @brief Parse Hop-by-Hop Options header
 * @param[in] interface Underlying network interface
 * @param[in] ipPacket Multi-part buffer containing the IPv6 packet
 * @param[in] ipPacketOffset Offset to the first byte of the IPv6 packet
 * @param[in,out] headerOffset Offset to the Hop-by-Hop Options header
 * @param[in,out] nextHeaderOffset Offset to the Next Header field
 * @brief Error code
 **/

error_t ipv6ParseHopByHopOptHeader(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t *headerOffset, size_t *nextHeaderOffset)
{
   error_t error;
   size_t n;
   size_t length;
   size_t headerLen;
   Ipv6HopByHopOptHeader *header;

   //Remaining bytes to process in the IPv6 packet
   length = netBufferGetLength(ipPacket) - *headerOffset;

   //Make sure the extension header is valid
   if(length < sizeof(Ipv6HopByHopOptHeader))
      return ERROR_INVALID_HEADER;

   //Point to the Hop-by-Hop Options header
   header = netBufferAt(ipPacket, *headerOffset);
   //Sanity check
   if(header == NULL)
      return ERROR_FAILURE;

   //Calculate the length of the entire header
   headerLen = (header->hdrExtLen * 8) + 8;

   //Check header length
   if(headerLen > length)
      return ERROR_INVALID_HEADER;

   //Debug message
   TRACE_DEBUG("  Hop-by-Hop Options header\r\n");

   //The Hop-by-Hop Options header, when present, must immediately follow
   //the IPv6 header
   if(*headerOffset != (ipPacketOffset + sizeof(Ipv6Header)))
   {
      //Compute the offset of the unrecognized Next Header field within the packet
      n = *nextHeaderOffset - ipPacketOffset;

      //Send an ICMP Parameter Problem message to the source of the packet
      icmpv6SendErrorMessage(interface, ICMPV6_TYPE_PARAM_PROBLEM,
         ICMPV6_CODE_UNKNOWN_NEXT_HEADER, n, ipPacket, ipPacketOffset);

      //Discard incoming packet
      return ERROR_INVALID_HEADER;
   }

   //Compute the length of the Options field
   n = headerLen - sizeof(Ipv6HopByHopOptHeader);

   //Parse options
   error = ipv6ParseOptions(interface, ipPacket, ipPacketOffset,
      *headerOffset + sizeof(Ipv6HopByHopOptHeader), n);

   //Any error to report?
   if(error)
      return error;

   //Keep track of Next Header field
   *nextHeaderOffset = *headerOffset + &header->nextHeader - (uint8_t *) header;
   //Point to the next extension header
   *headerOffset += headerLen;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Destination Options header
 * @param[in] interface Underlying network interface
 * @param[in] ipPacket Multi-part buffer containing the IPv6 packet
 * @param[in] ipPacketOffset Offset to the first byte of the IPv6 packet
 * @param[in,out] headerOffset Offset to the Destination Options header
 * @param[in,out] nextHeaderOffset Offset to the Next Header field
 * @brief Error code
 **/

error_t ipv6ParseDestOptHeader(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t *headerOffset, size_t *nextHeaderOffset)
{
   error_t error;
   size_t n;
   size_t length;
   size_t headerLen;
   Ipv6DestOptHeader *header;

   //Remaining bytes to process in the IPv6 packet
   length = netBufferGetLength(ipPacket) - *headerOffset;

   //Make sure the extension header is valid
   if(length < sizeof(Ipv6DestOptHeader))
      return ERROR_INVALID_HEADER;

   //Point to the Destination Options header
   header = netBufferAt(ipPacket, *headerOffset);
   //Sanity check
   if(header == NULL)
      return ERROR_FAILURE;

   //Calculate the length of the entire header
   headerLen = (header->hdrExtLen * 8) + 8;

   //Check header length
   if(headerLen > length)
      return ERROR_INVALID_HEADER;

   //Debug message
   TRACE_DEBUG("  Destination Options header\r\n");

   //Compute the length of the Options field
   n = headerLen - sizeof(Ipv6DestOptHeader);

   //Parse options
   error = ipv6ParseOptions(interface, ipPacket, ipPacketOffset,
      *headerOffset + sizeof(Ipv6DestOptHeader), n);

   //Any error to report?
   if(error)
      return error;

   //Keep track of Next Header field
   *nextHeaderOffset = *headerOffset + &header->nextHeader - (uint8_t *) header;
   //Point to the next extension header
   *headerOffset += headerLen;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Routing header
 * @param[in] interface Underlying network interface
 * @param[in] ipPacket Multi-part buffer containing the IPv6 packet
 * @param[in] ipPacketOffset Offset to the first byte of the IPv6 packet
 * @param[in,out] headerOffset Offset to the Routing header
 * @param[in,out] nextHeaderOffset Offset to the Next Header field
 * @brief Error code
 **/

error_t ipv6ParseRoutingHeader(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t *headerOffset, size_t *nextHeaderOffset)
{
   size_t n;
   size_t length;
   size_t headerLen;
   Ipv6RoutingHeader *header;

   //Remaining bytes to process in the IPv6 packet
   length = netBufferGetLength(ipPacket) - *headerOffset;

   //Make sure the extension header is valid
   if(length < sizeof(Ipv6RoutingHeader))
      return ERROR_INVALID_HEADER;

   //Point to the Routing header
   header = netBufferAt(ipPacket, *headerOffset);
   //Sanity check
   if(header == NULL)
      return ERROR_FAILURE;

   //Calculate the length of the entire header
   headerLen = (header->hdrExtLen * 8) + 8;

   //Check header length
   if(headerLen > length)
      return ERROR_INVALID_HEADER;

   //Debug message
   TRACE_DEBUG("  Routing header\r\n");

   //If, while processing a received packet, a node encounters a Routing
   //header with an unrecognized Routing Type value, the required behavior
   //of the node depends on the value of the Segments Left field
   if(header->segmentsLeft != 0)
   {
      //Retrieve the offset of the Routing header within the packet
      n = *headerOffset - ipPacketOffset;
      //Compute the exact offset of the Routing Type field
      n += (uint8_t *) &header->routingType - (uint8_t *) header;

      //If Segments Left is non-zero, send an ICMP Parameter Problem,
      //Code 0, message to the packet's Source Address, pointing to
      //the unrecognized Routing Type
      icmpv6SendErrorMessage(interface, ICMPV6_TYPE_PARAM_PROBLEM,
         ICMPV6_CODE_INVALID_HEADER_FIELD, n, ipPacket, ipPacketOffset);

      //The node must discard the packet
      return ERROR_INVALID_TYPE;
   }

   //Keep track of Next Header field
   *nextHeaderOffset = *headerOffset + &header->nextHeader - (uint8_t *) header;
   //Point to the next extension header
   *headerOffset += headerLen;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Authentication header
 * @param[in] interface Underlying network interface
 * @param[in] ipPacket Multi-part buffer containing the IPv6 packet
 * @param[in] ipPacketOffset Offset to the first byte of the IPv6 packet
 * @param[in,out] headerOffset Offset to the Authentication header
 * @param[in,out] nextHeaderOffset Offset to the Next Header field
 * @brief Error code
 **/

error_t ipv6ParseAuthHeader(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t *headerOffset, size_t *nextHeaderOffset)
{
   //Debug message
   TRACE_DEBUG("  Authentication header\r\n");
   //Authentication not supported
   return ERROR_FAILURE;
}


/**
 * @brief Parse Encapsulating Security Payload header
 * @param[in] interface Underlying network interface
 * @param[in] ipPacket Multi-part buffer containing the IPv6 packet
 * @param[in] ipPacketOffset Offset to the first byte of the IPv6 packet
 * @param[in,out] headerOffset Offset to the Encapsulating Security Payload header
 * @param[in,out] nextHeaderOffset Offset to the Next Header field
 * @brief Error code
 **/

error_t ipv6ParseEspHeader(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t *headerOffset, size_t *nextHeaderOffset)
{
   //Debug message
   TRACE_DEBUG("  Encapsulating Security Payload header\r\n");
   //Authentication not supported
   return ERROR_FAILURE;
}


/**
 * @brief Parse IPv6 options
 * @param[in] interface Underlying network interface
 * @param[in] ipPacket Multi-part buffer containing the IPv6 packet
 * @param[in] ipPacketOffset Offset to the first byte of the IPv6 packet
 * @param[in] optionOffset Offset to the first byte of the Options field
 * @param[in] optionLen Length of the Options field
 * @brief Error code
 **/

error_t ipv6ParseOptions(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t optionOffset, size_t optionLen)
{
   size_t i;
   size_t n;
   uint8_t type;
   uint8_t action;
   uint8_t *options;
   Ipv6Option *option;
   Ipv6Header *ipHeader;

   //Point to the first byte of the Options field
   options = netBufferAt(ipPacket, optionOffset);

   //Sanity check
   if(options == NULL)
      return ERROR_FAILURE;

   //Parse options
   for(i = 0; i < optionLen; )
   {
      //Point to the current option
      option = (Ipv6Option *) (options + i);
      //Get option type
      type = option->type & IPV6_OPTION_TYPE_MASK;

      //Pad1 option?
      if(type == IPV6_OPTION_TYPE_PAD1)
      {
         //Advance data pointer
         i++;
      }
      //PadN option?
      else if(type == IPV6_OPTION_TYPE_PADN)
      {
         //Malformed IPv6 packet?
         if((i + sizeof(Ipv6Option)) > optionLen)
            return ERROR_INVALID_LENGTH;

         //Advance data pointer
         i += sizeof(Ipv6Option) + option->length;
      }
      //Unrecognized option?
      else
      {
         //Point to the IPv6 header
         ipHeader = netBufferAt(ipPacket, ipPacketOffset);

         //Sanity check
         if(ipHeader == NULL)
            return ERROR_FAILURE;

         //Get the value of the highest-order two bits
         action = option->type & IPV6_ACTION_MASK;

         //The highest-order two bits specify the action that must be taken
         //if the processing IPv6 node does not recognize the option type
         if(action == IPV6_ACTION_SKIP_OPTION)
         {
            //Skip over this option and continue processing the header
         }
         else if(action == IPV6_ACTION_DISCARD_PACKET)
         {
            //Discard the packet
            return ERROR_INVALID_OPTION;
         }
         else if(action == IPV6_ACTION_SEND_ICMP_ERROR_ALL)
         {
            //Calculate the octet offset within the invoking packet
            //where the error was detected
            n = optionOffset + i - ipPacketOffset;

            //Send an ICMP Parameter Problem message to the source of the
            //packet, regardless of whether or not the destination address
            //was a multicast address
            icmpv6SendErrorMessage(interface, ICMPV6_TYPE_PARAM_PROBLEM,
               ICMPV6_CODE_UNKNOWN_IPV6_OPTION, n, ipPacket, ipPacketOffset);

            //Discard the packet
            return ERROR_INVALID_OPTION;
         }
         else if(action == IPV6_ACTION_SEND_ICMP_ERROR_UNI)
         {
            //Send an ICMP Parameter Problem message to the source of the
            //packet, only if the destination address was not a multicast
            //address
            if(!ipv6IsMulticastAddr(&ipHeader->destAddr))
            {
               //Calculate the octet offset within the invoking packet
               //where the error was detected
               n = optionOffset + i - ipPacketOffset;

               //Send the ICMP Parameter Problem message
               icmpv6SendErrorMessage(interface, ICMPV6_TYPE_PARAM_PROBLEM,
                  ICMPV6_CODE_UNKNOWN_IPV6_OPTION, n, ipPacket, ipPacketOffset);
            }

            //Discard the packet
            return ERROR_INVALID_OPTION;
         }

         //Malformed IPv6 packet?
         if((i + sizeof(Ipv6Option)) > optionLen)
            return ERROR_INVALID_LENGTH;

         //Advance data pointer
         i += sizeof(Ipv6Option) + option->length;
      }
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Send an IPv6 datagram
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv6 pseudo header
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in] offset Offset to the first byte of the payload
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ipv6SendDatagram(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   error_t error;
   size_t length;
   size_t pathMtu;

   //Total number of IP datagrams which local IP user-protocols supplied to IP
   //in requests for transmission
   IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsOutRequests, 1);
   IP_MIB_INC_COUNTER64(ipv6SystemStats.ipSystemStatsHCOutRequests, 1);
   IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsOutRequests, 1);
   IP_MIB_INC_COUNTER64(ipv6IfStatsTable[interface->index].ipIfStatsHCOutRequests, 1);

   //Retrieve the length of payload
   length = netBufferGetLength(buffer) - offset;

#if (IPV6_PMTU_SUPPORT == ENABLED)
   //Retrieve the PMTU for the specified destination address
   pathMtu = ipv6GetPathMtu(interface, &pseudoHeader->destAddr);

   //The PMTU should not exceed the MTU of the first-hop link
   if(pathMtu > interface->ipv6Context.linkMtu)
      pathMtu = interface->ipv6Context.linkMtu;
#else
   //The PMTU value for the path is assumed to be the MTU of the first-hop link
   pathMtu = interface->ipv6Context.linkMtu;
#endif

   //If the payload length is smaller than the PMTU then no fragmentation is
   //needed
   if((length + sizeof(Ipv6Header)) <= pathMtu)
   {
      //Send data as is
      error = ipv6SendPacket(interface, pseudoHeader, 0, 0, buffer, offset,
         ancillary);
   }
   //If the payload length exceeds the PMTU then the device must fragment the
   //data
   else
   {
#if (IPV6_FRAG_SUPPORT == ENABLED)
      //Fragment IP datagram into smaller packets
      error = ipv6FragmentDatagram(interface, pseudoHeader, buffer, offset,
         pathMtu, ancillary);
#else
      //Fragmentation is not supported
      error = ERROR_MESSAGE_TOO_LONG;
#endif
   }

   //Return status code
   return error;
}


/**
 * @brief Send an IPv6 packet
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv6 pseudo header
 * @param[in] fragId Fragment identification field
 * @param[in] fragOffset Fragment offset field
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in] offset Offset to the first byte of the payload
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ipv6SendPacket(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   uint32_t fragId, size_t fragOffset, NetBuffer *buffer, size_t offset,
   NetTxAncillary *ancillary)
{
   error_t error;
   size_t length;
   Ipv6Header *packet;
#if (ETH_SUPPORT == ENABLED)
   NetInterface *physicalInterface;
#endif

   //Calculate the length of the payload
   length = netBufferGetLength(buffer) - offset;

   //Add Fragment header?
   if(fragOffset != 0)
   {
      Ipv6FragmentHeader *header;

      //Is there enough space for the IPv6 header and the Fragment header?
      if(offset < (sizeof(Ipv6Header) + sizeof(Ipv6FragmentHeader)))
         return ERROR_INVALID_PARAMETER;

      //Make room for the Fragment header
      offset -= sizeof(Ipv6FragmentHeader);
      length += sizeof(Ipv6FragmentHeader);

      //Point to the Fragment header
      header = netBufferAt(buffer, offset);
      //Format the Fragment header
      header->nextHeader = pseudoHeader->nextHeader;
      header->reserved = 0;
      header->fragmentOffset = htons(fragOffset);
      header->identification = htonl(fragId);

      //Make room for the IPv6 header
      offset -= sizeof(Ipv6Header);
      length += sizeof(Ipv6Header);

      //Point to the IPv6 header
      packet = netBufferAt(buffer, offset);
      //Properly set the Next Header field
      packet->nextHeader = IPV6_FRAGMENT_HEADER;
   }
   else
   {
      //Is there enough space for the IPv6 header?
      if(offset < sizeof(Ipv6Header))
         return ERROR_INVALID_PARAMETER;

      //Make room for the IPv6 header
      offset -= sizeof(Ipv6Header);
      length += sizeof(Ipv6Header);

      //Point to the IPv6 header
      packet = netBufferAt(buffer, offset);
      //Properly set the Next Header field
      packet->nextHeader = pseudoHeader->nextHeader;
   }

   //Format IPv6 header
   packet->version = IPV6_VERSION;
   packet->trafficClassH = 0;
   packet->trafficClassL = 0;
   packet->flowLabelH = 0;
   packet->flowLabelL = 0;
   packet->payloadLen = htons(length - sizeof(Ipv6Header));
   packet->hopLimit = ancillary->ttl;
   packet->srcAddr = pseudoHeader->srcAddr;
   packet->destAddr = pseudoHeader->destAddr;

   //Check whether the Hop Limit value is zero
   if(packet->hopLimit == 0)
   {
      //Use default Hop Limit value
      packet->hopLimit = interface->ipv6Context.curHopLimit;
   }

#if (IP_DIFF_SERV_SUPPORT == ENABLED)
   //Set DSCP field
   packet->trafficClassH = (ancillary->dscp >> 2) & 0x0F;
   packet->trafficClassL = (ancillary->dscp << 2) & 0x0C;
#endif

   //Ensure the source address is valid
   error = ipv6CheckSourceAddr(interface, &pseudoHeader->srcAddr);
   //Invalid source address?
   if(error)
      return error;

   //Check destination address
   if(ipv6CompAddr(&pseudoHeader->destAddr, &IPV6_UNSPECIFIED_ADDR))
   {
      //The unspecified address must not appear on the public Internet
      error = ERROR_INVALID_ADDRESS;
   }
   else if(ipv6IsLocalHostAddr(&pseudoHeader->destAddr))
   {
#if (NET_LOOPBACK_IF_SUPPORT == ENABLED)
      uint_t i;

      //Initialize status code
      error = ERROR_NO_ROUTE;

      //Loop through network interfaces
      for(i = 0; i < NET_INTERFACE_COUNT; i++)
      {
         //Point to the current interface
         interface = &netInterface[i];

         //Loopback interface?
         if(interface->nicDriver != NULL &&
            interface->nicDriver->type == NIC_TYPE_LOOPBACK)
         {
            //Forward the packet to the loopback interface
            error = nicSendPacket(interface, buffer, offset, ancillary);
            break;
         }
      }
#else
      //The loopback address must not appear on the public Internet
      error = ERROR_NO_ROUTE;
#endif
   }
   else
   {
#if (ETH_SUPPORT == ENABLED)
      //Point to the physical interface
      physicalInterface = nicGetPhysicalInterface(interface);

      //Ethernet interface?
      if(physicalInterface->nicDriver != NULL &&
         physicalInterface->nicDriver->type == NIC_TYPE_ETHERNET)
      {
         Ipv6Addr destIpAddr;
         NdpDestCacheEntry *entry;

         //When the sending node has a packet to send, it first examines
         //the Destination Cache
         entry = ndpFindDestCacheEntry(interface, &pseudoHeader->destAddr);

         //Check whether a matching entry exists
         if(entry != NULL)
         {
            //Retrieve the address of the next-hop
            destIpAddr = entry->nextHop;
            //Update timestamp
            entry->timestamp = osGetSystemTime();
            //No error to report
            error = NO_ERROR;
         }
         else
         {
            //Perform next-hop determination
            error = ndpSelectNextHop(interface, &pseudoHeader->destAddr, NULL,
               &destIpAddr, ancillary->dontRoute);

            //Check status code
            if(error == NO_ERROR)
            {
               //Create a new Destination Cache entry
               entry = ndpCreateDestCacheEntry(interface);

               //Destination cache entry successfully created?
               if(entry != NULL)
               {
                  //Destination address
                  entry->destAddr = pseudoHeader->destAddr;
                  //Address of the next hop
                  entry->nextHop = destIpAddr;

                  //Initially, the PMTU value for a path is assumed to be the
                  //MTU of the first-hop link
                  entry->pathMtu = interface->ipv6Context.linkMtu;

                  //Set timestamp
                  entry->timestamp = osGetSystemTime();
               }
            }
            else if(error == ERROR_NO_ROUTE)
            {
               //Number of IP datagrams discarded because no route could be found
               //to transmit them to their destination
               IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsOutNoRoutes, 1);
            }
         }

         //Successful next-hop determination?
         if(error == NO_ERROR)
         {
            //Perform address resolution
            if(!macCompAddr(&ancillary->destMacAddr, &MAC_UNSPECIFIED_ADDR))
            {
               //The destination address is already resolved
               error = NO_ERROR;
            }
            else if(ipv6IsMulticastAddr(&destIpAddr))
            {
               //Map IPv6 multicast address to MAC-layer multicast address
               error = ipv6MapMulticastAddrToMac(&destIpAddr,
                  &ancillary->destMacAddr);
            }
            else
            {
               //Resolve host address using Neighbor Discovery protocol
               error = ndpResolve(interface, &destIpAddr, &ancillary->destMacAddr);
            }

            //Successful address resolution?
            if(error == NO_ERROR)
            {
               //Update IP statistics
               ipv6UpdateOutStats(interface, &destIpAddr, length);

               //Debug message
               TRACE_INFO("Sending IPv6 packet (%" PRIuSIZE " bytes)...\r\n", length);
               //Dump IP header contents for debugging purpose
               ipv6DumpHeader(packet);

               //Send Ethernet frame
               error = ethSendFrame(interface, &ancillary->destMacAddr,
                  ETH_TYPE_IPV6, buffer, offset, ancillary);
            }
            else if(error == ERROR_IN_PROGRESS)
            {
               //Debug message
               TRACE_INFO("Enqueuing IPv6 packet (%" PRIuSIZE " bytes)...\r\n", length);
               //Dump IP header contents for debugging purpose
               ipv6DumpHeader(packet);

               //Enqueue packets waiting for address resolution
               error = ndpEnqueuePacket(NULL, interface, &destIpAddr, buffer,
                  offset, ancillary);
            }
            else
            {
               //Debug message
               TRACE_WARNING("Cannot map IPv6 address to Ethernet address!\r\n");
            }
         }
      }
      else
#endif
#if (PPP_SUPPORT == ENABLED)
      //PPP interface?
      if(interface->nicDriver != NULL &&
         interface->nicDriver->type == NIC_TYPE_PPP)
      {
         //Update IP statistics
         ipv6UpdateOutStats(interface, &pseudoHeader->destAddr, length);

         //Debug message
         TRACE_INFO("Sending IPv6 packet (%" PRIuSIZE " bytes)...\r\n", length);
         //Dump IP header contents for debugging purpose
         ipv6DumpHeader(packet);

         //Send PPP frame
         error = pppSendFrame(interface, buffer, offset, PPP_PROTOCOL_IPV6);
      }
      else
#endif
      //6LoWPAN interface?
      if(interface->nicDriver != NULL &&
         interface->nicDriver->type == NIC_TYPE_6LOWPAN)
      {
         //Update IP statistics
         ipv6UpdateOutStats(interface, &pseudoHeader->destAddr, length);

         //Debug message
         TRACE_INFO("Sending IPv6 packet (%" PRIuSIZE " bytes)...\r\n", length);
         //Dump IP header contents for debugging purpose
         ipv6DumpHeader(packet);

         //Send the packet over the specified link
         error = nicSendPacket(interface, buffer, offset, ancillary);
      }
      else
      //Unknown interface type?
      {
         //Report an error
         error = ERROR_INVALID_INTERFACE;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Join an IPv6 multicast group
 * @param[in] interface Underlying network interface
 * @param[in] groupAddr IPv6 Multicast address to join
 * @return Error code
 **/

error_t ipv6JoinMulticastGroup(NetInterface *interface, const Ipv6Addr *groupAddr)
{
   error_t error;
   uint_t i;
   Ipv6FilterEntry *entry;
   Ipv6FilterEntry *firstFreeEntry;
#if (ETH_SUPPORT == ENABLED)
   NetInterface *physicalInterface;
   MacAddr macAddr;
#endif

   //The IPv6 address must be a valid multicast address
   if(!ipv6IsMulticastAddr(groupAddr))
      return ERROR_INVALID_ADDRESS;

#if (ETH_SUPPORT == ENABLED)
   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);
#endif

   //Initialize error code
   error = NO_ERROR;
   //Keep track of the first free entry
   firstFreeEntry = NULL;

   //Go through the multicast filter table
   for(i = 0; i < IPV6_MULTICAST_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.multicastFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Check whether the table already contains the specified IPv6 address
         if(ipv6CompAddr(&entry->addr, groupAddr))
         {
            //Increment the reference count
            entry->refCount++;
            //Successful processing
            return NO_ERROR;
         }
      }
      else
      {
         //Keep track of the first free entry
         if(firstFreeEntry == NULL)
            firstFreeEntry = entry;
      }
   }

   //Check whether the multicast filter table is full
   if(firstFreeEntry == NULL)
   {
      //A new entry cannot be added
      return ERROR_FAILURE;
   }

#if (ETH_SUPPORT == ENABLED)
   //Map the IPv6 multicast address to a MAC-layer address
   ipv6MapMulticastAddrToMac(groupAddr, &macAddr);

   //Add the corresponding address to the MAC filter table
   error = ethAcceptMacAddr(interface, &macAddr);

   //Check status code
   if(!error)
   {
      //Virtual interface?
      if(interface != physicalInterface)
      {
         //Configure the physical interface to accept the MAC address
         error = ethAcceptMacAddr(physicalInterface, &macAddr);

         //Any error to report?
         if(error)
         {
            //Clean up side effects
            ethDropMacAddr(interface, &macAddr);
         }
      }
   }
#endif

   //MAC filter table successfully updated?
   if(!error)
   {
      //Now we can safely add a new entry to the table
      firstFreeEntry->addr = *groupAddr;
      //Initialize the reference count
      firstFreeEntry->refCount = 1;

#if (MLD_SUPPORT == ENABLED)
      //Start listening to the multicast address
      mldStartListening(interface, firstFreeEntry);
#endif
   }

   //Return status code
   return error;
}


/**
 * @brief Leave an IPv6 multicast group
 * @param[in] interface Underlying network interface
 * @param[in] groupAddr IPv6 multicast address to drop
 * @return Error code
 **/

error_t ipv6LeaveMulticastGroup(NetInterface *interface, const Ipv6Addr *groupAddr)
{
   uint_t i;
   Ipv6FilterEntry *entry;
#if (ETH_SUPPORT == ENABLED)
   NetInterface *physicalInterface;
   MacAddr macAddr;
#endif

   //The IPv6 address must be a valid multicast address
   if(!ipv6IsMulticastAddr(groupAddr))
      return ERROR_INVALID_ADDRESS;

#if (ETH_SUPPORT == ENABLED)
   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);
#endif

   //Go through the multicast filter table
   for(i = 0; i < IPV6_MULTICAST_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.multicastFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Specified IPv6 address found?
         if(ipv6CompAddr(&entry->addr, groupAddr))
         {
            //Decrement the reference count
            entry->refCount--;

            //Remove the entry if the reference count drops to zero
            if(entry->refCount == 0)
            {
#if (MLD_SUPPORT == ENABLED)
               //Stop listening to the multicast address
               mldStopListening(interface, entry);
#endif
#if (ETH_SUPPORT == ENABLED)
               //Map the IPv6 multicast address to a MAC-layer address
               ipv6MapMulticastAddrToMac(groupAddr, &macAddr);
               //Drop the corresponding address from the MAC filter table
               ethDropMacAddr(interface, &macAddr);

               //Virtual interface?
               if(interface != physicalInterface)
               {
                  //Drop the corresponding address from the MAC filter table of
                  //the physical interface
                  ethDropMacAddr(physicalInterface, &macAddr);
               }
#endif
               //Remove the multicast address from the list
               entry->addr = IPV6_UNSPECIFIED_ADDR;
            }

            //Successful processing
            return NO_ERROR;
         }
      }
   }

   //The specified IPv6 address does not exist
   return ERROR_ADDRESS_NOT_FOUND;
}


/**
 * @brief Convert a string representation of an IPv6 address to a binary IPv6 address
 * @param[in] str NULL-terminated string representing the IPv6 address
 * @param[out] ipAddr Binary representation of the IPv6 address
 * @return Error code
 **/

error_t ipv6StringToAddr(const char_t *str, Ipv6Addr *ipAddr)
{
   error_t error;
   int_t i = 0;
   int_t j = -1;
   int_t k = 0;
   int32_t value = -1;

   //Parse input string
   while(1)
   {
      //Hexadecimal digit found?
      if(isxdigit((uint8_t) *str))
      {
         //First digit to be decoded?
         if(value < 0)
            value = 0;

         //Update the value of the current 16-bit word
         if(osIsdigit(*str))
         {
            value = (value * 16) + (*str - '0');
         }
         else if(osIsupper(*str))
         {
            value = (value * 16) + (*str - 'A' + 10);
         }
         else
         {
            value = (value * 16) + (*str - 'a' + 10);
         }

         //Check resulting value
         if(value > 0xFFFF)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }
      }
      //"::" symbol found?
      else if(!osStrncmp(str, "::", 2))
      {
         //The "::" can only appear once in an IPv6 address
         if(j >= 0)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }

         //The "::" symbol is preceded by a number?
         if(value >= 0)
         {
            //Save the current 16-bit word
            ipAddr->w[i++] = htons(value);
            //Prepare to decode the next 16-bit word
            value = -1;
         }

         //Save the position of the "::" symbol
         j = i;
         //Point to the next character
         str++;
      }
      //":" symbol found?
      else if(*str == ':' && i < 8)
      {
         //Each ":" must be preceded by a valid number
         if(value < 0)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }

         //Save the current 16-bit word
         ipAddr->w[i++] = htons(value);
         //Prepare to decode the next 16-bit word
         value = -1;
      }
      //End of string detected?
      else if(*str == '\0' && i == 7 && j < 0)
      {
         //The NULL character must be preceded by a valid number
         if(value < 0)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
         }
         else
         {
            //Save the last 16-bit word of the IPv6 address
            ipAddr->w[i] = htons(value);
            //The conversion succeeded
            error = NO_ERROR;
         }

         //We are done
         break;
      }
      else if(*str == '\0' && i < 7 && j >= 0)
      {
         //Save the last 16-bit word of the IPv6 address
         if(value >= 0)
            ipAddr->w[i++] = htons(value);

         //Move the part of the address that follows the "::" symbol
         for(k = 0; k < (i - j); k++)
            ipAddr->w[7 - k] = ipAddr->w[i - 1 - k];
         //A sequence of zeroes can now be written in place of "::"
         for(k = 0; k < (8 - i); k++)
            ipAddr->w[j + k] = 0;

         //The conversion succeeded
         error = NO_ERROR;
         break;
      }
      //Invalid character...
      else
      {
         //The conversion failed
         error = ERROR_INVALID_SYNTAX;
         break;
      }

      //Point to the next character
      str++;
   }

   //Return status code
   return error;
}


/**
 * @brief Convert a binary IPv6 address to a string representation
 *
 * Call ipv6AddrToString() to convert an IPv6 address to a text representation. The
 * implementation of ipv6AddrToString() function follows RFC 5952 recommendations
 *
 * @param[in] ipAddr Binary representation of the IPv6 address
 * @param[out] str NULL-terminated string representing the IPv6 address
 * @return Pointer to the formatted string
 **/

char_t *ipv6AddrToString(const Ipv6Addr *ipAddr, char_t *str)
{
   static char_t buffer[40];
   uint_t i;
   uint_t j;
   char_t *p;

   //Best run of zeroes
   uint_t zeroRunStart = 0;
   uint_t zeroRunEnd = 0;

   //If the NULL pointer is given as parameter, then the internal buffer is used
   if(str == NULL)
      str = buffer;

   //Find the longest run of zeros for "::" short-handing
   for(i = 0; i < 8; i++)
   {
      //Compute the length of the current sequence of zeroes
      for(j = i; j < 8 && !ipAddr->w[j]; j++);

      //Keep track of the longest one
      if((j - i) > 1 && (j - i) > (zeroRunEnd - zeroRunStart))
      {
         //The symbol "::" should not be used to shorten just one zero field
         zeroRunStart = i;
         zeroRunEnd = j;
      }
   }

   //Format IPv6 address
   for(p = str, i = 0; i < 8; i++)
   {
      //Are we inside the best run of zeroes?
      if(i >= zeroRunStart && i < zeroRunEnd)
      {
         //Append a separator
         *(p++) = ':';
         //Skip the sequence of zeroes
         i = zeroRunEnd - 1;
      }
      else
      {
         //Add a separator between each 16-bit word
         if(i > 0)
            *(p++) = ':';

         //Convert the current 16-bit word to string
         p += osSprintf(p, "%" PRIx16, ntohs(ipAddr->w[i]));
      }
   }

   //A trailing run of zeroes has been found?
   if(zeroRunEnd == 8)
      *(p++) = ':';

   //Properly terminate the string
   *p = '\0';

   //Return a pointer to the formatted string
   return str;
}


/**
 * @brief Dump IPv6 header for debugging purpose
 * @param[in] ipHeader IPv6 header
 **/

void ipv6DumpHeader(const Ipv6Header *ipHeader)
{
   //Dump IPv6 header contents
   TRACE_DEBUG("  Version = %" PRIu8 "\r\n", ipHeader->version);
   TRACE_DEBUG("  Traffic Class = %u\r\n", (ipHeader->trafficClassH << 4) | ipHeader->trafficClassL);
   TRACE_DEBUG("  Flow Label = 0x%05X\r\n", (ipHeader->flowLabelH << 16) | ntohs(ipHeader->flowLabelL));
   TRACE_DEBUG("  Payload Length = %" PRIu16 "\r\n", ntohs(ipHeader->payloadLen));
   TRACE_DEBUG("  Next Header = %" PRIu8 "\r\n", ipHeader->nextHeader);
   TRACE_DEBUG("  Hop Limit = %" PRIu8 "\r\n", ipHeader->hopLimit);
   TRACE_DEBUG("  Src Addr = %s\r\n", ipv6AddrToString(&ipHeader->srcAddr, NULL));
   TRACE_DEBUG("  Dest Addr = %s\r\n", ipv6AddrToString(&ipHeader->destAddr, NULL));
}

#endif
