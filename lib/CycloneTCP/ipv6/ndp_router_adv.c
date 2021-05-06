/**
 * @file ndp_router_adv.c
 * @brief Router advertisement service
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
#include "core/ip.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "ipv6/icmpv6.h"
#include "ipv6/ndp.h"
#include "ipv6/ndp_cache.h"
#include "ipv6/ndp_misc.h"
#include "ipv6/ndp_router_adv.h"
#include "mibs/ip_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED && NDP_ROUTER_ADV_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t ndpRouterAdvTickCounter;


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains the RA service configuration variables
 **/

void ndpRouterAdvGetDefaultSettings(NdpRouterAdvSettings *settings)
{
   //Underlying network interface
   settings->interface = netGetDefaultInterface();

   //The maximum time allowed between sending unsolicited multicast
   //Router Advertisements from the interface
   settings->maxRtrAdvInterval = NDP_MAX_RTR_ADVERT_INTERVAL;

   //The minimum time allowed between sending unsolicited multicast
   //Router Advertisements from the interface
   settings->minRtrAdvInterval = NDP_MAX_RTR_ADVERT_INTERVAL / 3;

   //The default value to be placed in the Cur Hop Limit field in the
   //Router Advertisement messages sent by the router
   settings->curHopLimit = 0;

   //The value to be placed in the Managed Address Configuration
   //flag in the Router Advertisement
   settings->managedFlag = FALSE;

   //The value to be placed in the Other Configuration flag
   //in the Router Advertisement
   settings->otherConfigFlag = FALSE;

   //The value to be placed in the Mobile IPv6 Home Agent
   //flag in the Router Advertisement
   settings->homeAgentFlag = FALSE;

   //The value to be placed in the Router Selection Preferences
   //field in the Router Advertisement
   settings->preference = NDP_ROUTER_SEL_PREFERENCE_MEDIUM;

   //The value to be placed in the Neighbor Discovery Proxy
   //flag in the Router Advertisement
   settings->proxyFlag = FALSE;

   //The value to be placed in the Router Lifetime field of
   //Router Advertisements sent from the interface
   settings->defaultLifetime = 3 * (NDP_MAX_RTR_ADVERT_INTERVAL / 1000);

   //The value to be placed in the Reachable Time field in the
   //Router Advertisement messages sent by the router
   settings->reachableTime = 0;

   //The value to be placed in the Retrans Timer field in the
   //Router Advertisement messages sent by the router
   settings->retransTimer = 0;

   //The value to be placed in the MTU option sent by the router
   settings->linkMtu = 0;

   //A list of prefixes to be placed in Prefix Information options (PIO)
   //in Router Advertisement messages sent from the interface
   settings->prefixList = NULL;
   settings->prefixListLength = 0;

   //A list of routes to be placed in Route Information options (RIO)
   //in Router Advertisement messages sent from the interface
   settings->routeList = NULL;
   settings->routeListLength = 0;

   //A list of header compression contexts to be placed in the 6LoWPAN Context
   //options (6CO) in Router Advertisement messages sent from the interface
   settings->contextList = NULL;
   settings->contextListLength = 0;
}


/**
 * @brief RA service initialization
 * @param[in] context Pointer to the RA service context
 * @param[in] settings RA service configuration variables
 * @return Error code
 **/

error_t ndpRouterAdvInit(NdpRouterAdvContext *context,
   const NdpRouterAdvSettings *settings)
{
   NetInterface *interface;

   //Debug message
   TRACE_INFO("Initializing Router Advertisement service...\r\n");

   //Ensure the parameters are valid
   if(!context || !settings)
      return ERROR_INVALID_PARAMETER;

   //Valid network interface?
   if(!settings->interface)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the underlying network interface
   interface = settings->interface;

   //Clear the RA service context
   osMemset(context, 0, sizeof(NdpRouterAdvContext));
   //Save user settings
   context->settings = *settings;

   //The RA service is currently disabled on the interface
   context->running = FALSE;
   //Attach the RA service context to the network interface
   interface->ndpRouterAdvContext = context;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Start RA service
 * @param[in] context Pointer to the RA service context
 * @return Error code
 **/

error_t ndpRouterAdvStart(NdpRouterAdvContext *context)
{
   error_t error;
   NetInterface *interface;

   //Make sure the RA service context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting Router Advertisement service...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Check whether the service is running
   if(!context->running)
   {
      //Point to the underlying network interface
      interface = context->settings.interface;

      //Join the All-Routers multicast address
      error = ipv6JoinMulticastGroup(interface, &IPV6_LINK_LOCAL_ALL_ROUTERS_ADDR);

      //Successful membership registration?
      if(!error)
      {
         //Reset variables
         context->timestamp = osGetSystemTime();
         context->timeout = 0;
         context->routerAdvCount = 0;

         //Enable the router to forward packets to or from the interface
         interface->ipv6Context.isRouter = TRUE;

         //Default Hop Limit value
         if(context->settings.curHopLimit != 0)
            interface->ipv6Context.curHopLimit = context->settings.curHopLimit;

         //The time a node assumes a neighbor is reachable
         if(context->settings.reachableTime != 0)
            interface->ndpContext.reachableTime = context->settings.reachableTime;

         //The time between retransmissions of NS messages
         if(context->settings.retransTimer != 0)
            interface->ndpContext.retransTimer = context->settings.retransTimer;

         //Start transmitting Router Advertisements
         context->running = TRUE;
      }
   }
   else
   {
      //The service is already running...
      error = NO_ERROR;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief Stop RA service
 * @param[in] context Pointer to the RA service context
 * @return Error code
 **/

error_t ndpRouterAdvStop(NdpRouterAdvContext *context)
{
   error_t error;
   NetInterface *interface;

   //Make sure the RA service context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Stopping Router Advertisement service...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Check whether the service is running
   if(context->running)
   {
      //Point to the underlying network interface
      interface = context->settings.interface;

      //The router should transmit one or more final multicast Router
      //Advertisements with a Router Lifetime field of zero
      ndpSendRouterAdv(context, 0);

      //Leave the All-Routers multicast address
      error = ipv6LeaveMulticastGroup(interface, &IPV6_LINK_LOCAL_ALL_ROUTERS_ADDR);

      //Restore default parameters
      interface->ipv6Context.curHopLimit = IPV6_DEFAULT_HOP_LIMIT;
      interface->ndpContext.reachableTime = NDP_REACHABLE_TIME;
      interface->ndpContext.retransTimer = NDP_RETRANS_TIMER;

      //Stop transmitting Router Advertisements
      context->running = FALSE;
   }
   else
   {
      //The service is not running...
      error = NO_ERROR;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief RA service timer handler
 * @param[in] context Pointer to the RA service context
 **/

void ndpRouterAdvTick(NdpRouterAdvContext *context)
{
   systime_t time;
   NetInterface *interface;
   NdpRouterAdvSettings *settings;

   //Make sure the RA service has been properly instantiated
   if(context == NULL)
      return;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Point to the router configuration variables
   settings = &context->settings;

   //Get current time
   time = osGetSystemTime();

   //Make sure that the link is up and the service is running
   if(interface->linkState && context->running)
   {
      //Make sure that a valid link-local address has been assigned to the
      //interface
      if(ipv6GetLinkLocalAddrState(interface) == IPV6_ADDR_STATE_PREFERRED)
      {
         //Check current time
         if(timeCompare(time, context->timestamp + context->timeout) >= 0)
         {
            //Send an unsolicited Router Advertisement
            ndpSendRouterAdv(context, context->settings.defaultLifetime);

            //Save the time at which the message was sent
            context->timestamp = time;

            //Whenever a multicast advertisement is sent from an interface, the
            //timer is reset to a uniformly distributed random value between
            //MinRtrAdvInterval and MaxRtrAdvInterval
            context->timeout = netGetRandRange(settings->minRtrAdvInterval,
               settings->maxRtrAdvInterval);

            //First Router Advertisements to be sent from this interface?
            if(context->routerAdvCount < NDP_MAX_INITIAL_RTR_ADVERTISEMENTS)
            {
               //For the first few advertisements sent from an interface when it
               //becomes an advertising interface, the randomly chosen interval
               //should not be greater than MAX_INITIAL_RTR_ADVERT_INTERVAL
               context->timeout = MIN(context->timeout, NDP_MAX_INITIAL_RTR_ADVERT_INTERVAL);
            }

            //Increment counter
            context->routerAdvCount++;
         }
      }
   }
}


/**
 * @brief Callback function for link change event
 * @param[in] context Pointer to the RA service context
 **/

void ndpRouterAdvLinkChangeEvent(NdpRouterAdvContext *context)
{
   NetInterface *interface;

   //Make sure the RA service has been properly instantiated
   if(context == NULL)
      return;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Reset variables
   context->timestamp = osGetSystemTime();
   context->timeout = 0;
   context->routerAdvCount = 0;

   //Default Hop Limit value
   if(context->settings.curHopLimit != 0)
      interface->ipv6Context.curHopLimit = context->settings.curHopLimit;

   //The time a node assumes a neighbor is reachable
   if(context->settings.reachableTime != 0)
      interface->ndpContext.reachableTime = context->settings.reachableTime;

   //The time between retransmissions of NS messages
   if(context->settings.retransTimer != 0)
      interface->ndpContext.retransTimer = context->settings.retransTimer;
}


/**
 * @brief Router Solicitation message processing
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv6 pseudo header
 * @param[in] buffer Multi-part buffer containing the Router Advertisement message
 * @param[in] offset Offset to the first byte of the message
 * @param[in] hopLimit Hop Limit field from IPv6 header
 **/

void ndpProcessRouterSol(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit)
{
   error_t error;
   uint_t n;
   size_t length;
   systime_t time;
   systime_t delay;
   NdpRouterAdvContext *context;
   NdpRouterSolMessage *message;
   NdpLinkLayerAddrOption *option;
   NdpNeighborCacheEntry *entry;

   //Point to the RA service context
   context = interface->ndpRouterAdvContext;

   //A host must silently discard any received Router Solicitation
   if(context == NULL)
      return;

   //Get current time
   time = osGetSystemTime();

   //Retrieve the length of the message
   length = netBufferGetLength(buffer) - offset;

   //Check the length of the Router Solicitation message
   if(length < sizeof(NdpRouterSolMessage))
      return;

   //Point to the beginning of the message
   message = netBufferAt(buffer, offset);
   //Sanity check
   if(message == NULL)
      return;

   //Debug message
   TRACE_INFO("Router Solicitation message received (%" PRIuSIZE " bytes)...\r\n",
      length);

   //Dump message contents for debugging purpose
   ndpDumpRouterSolMessage(message);

   //The IPv6 Hop Limit field must have a value of 255 to ensure that the
   //packet has not been forwarded by a router
   if(hopLimit != NDP_HOP_LIMIT)
      return;

   //ICMPv6 Code must be 0
   if(message->code)
      return;

   //Calculate the length of the Options field
   length -= sizeof(NdpRouterSolMessage);

   //Parse Options field
   error = ndpCheckOptions(message->options, length);
   //All included options must have a length that is greater than zero
   if(error)
      return;

   //Search for the Source Link-Layer Address option
   option = ndpGetOption(message->options,
      length, NDP_OPT_SOURCE_LINK_LAYER_ADDR);

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
      entry = ndpFindNeighborCacheEntry(interface, &pseudoHeader->srcAddr);

      //No matching entry has been found?
      if(!entry)
      {
         //Create an entry
         entry = ndpCreateNeighborCacheEntry(interface);

         //Neighbor Cache entry successfully created?
         if(entry)
         {
            //Record the IPv6 and the corresponding MAC address
            entry->ipAddr = pseudoHeader->srcAddr;
            entry->macAddr = option->linkLayerAddr;
            //The IsRouter flag must be set to FALSE
            entry->isRouter = FALSE;
            //Save current time
            entry->timestamp = time;
            //Enter the STALE state
            entry->state = NDP_STATE_STALE;
         }
      }
      else
      {
         //If a Neighbor Cache entry for the solicitation's sender exists the
         //entry's IsRouter flag must be set to FALSE
         entry->isRouter = FALSE;

         //INCOMPLETE state?
         if(entry->state == NDP_STATE_INCOMPLETE)
         {
            //Record link-layer address
            entry->macAddr = option->linkLayerAddr;
            //Send all the packets that are pending for transmission
            n = ndpSendQueuedPackets(interface, entry);
            //Save current time
            entry->timestamp = time;

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
            if(!macCompAddr(&entry->macAddr, &option->linkLayerAddr))
            {
               //Update link-layer address
               entry->macAddr = option->linkLayerAddr;
               //Save current time
               entry->timestamp = time;
               //Enter the STALE state
               entry->state = NDP_STATE_STALE;
            }
         }
      }
   }

   //Upon receipt of a Router Solicitation, compute a random delay within the
   //range 0 through MAX_RA_DELAY_TIME
   delay = netGetRandRange(0, NDP_MAX_RA_DELAY_TIME);

   //If the computed value corresponds to a time later than the time the next
   //multicast Router Advertisement is scheduled to be sent, ignore the random
   //delay and send the advertisement at the already-scheduled time
   if(timeCompare(time + delay, context->timestamp + context->timeout) > 0)
      return;

   //Check whether the router sent a multicast Router Advertisement (solicited
   //or unsolicited) within the last MIN_DELAY_BETWEEN_RAS seconds
   if(timeCompare(time, context->timestamp + NDP_MIN_DELAY_BETWEEN_RAS) < 0)
   {
      //Schedule the advertisement to be sent at a time corresponding to
      //MIN_DELAY_BETWEEN_RAS plus the random value after the previous
      //advertisement was sent. This ensures that the multicast Router
      //Advertisements are rate limited
      context->timeout = NDP_MIN_DELAY_BETWEEN_RAS + delay;
   }
   else
   {
      //Schedule the sending of a Router Advertisement at the time given by the
      //random value
      context->timeout = time + delay - context->timestamp;
   }
}


/**
 * @brief Send a Router Advertisement message
 * @param[in] context Pointer to the RA service context
 * @param[in] routerLifetime Router Lifetime field
 * @return Error code
 **/

error_t ndpSendRouterAdv(NdpRouterAdvContext *context, uint16_t routerLifetime)
{
   error_t error;
   uint_t i;
   uint32_t n;
   size_t offset;
   size_t length;
   NetBuffer *buffer;
   NetInterface *interface;
   NdpRouterAdvMessage *message;
   NdpRouterAdvSettings *settings;
   Ipv6PseudoHeader pseudoHeader;
   NetTxAncillary ancillary;
#if (ETH_SUPPORT == ENABLED)
   NetInterface *logicalInterface;
#endif

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Point to the router configuration variables
   settings = &context->settings;

   //The destination address is typically the all-nodes multicast address
   pseudoHeader.destAddr = IPV6_LINK_LOCAL_ALL_NODES_ADDR;

   //Routers must use their link-local address as the source for Router
   //Advertisement messages so that hosts can uniquely identify routers
   error = ipv6SelectSourceAddr(&interface, &pseudoHeader.destAddr,
      &pseudoHeader.srcAddr);

   //No link-local address assigned to the interface?
   if(error)
      return error;

   //Compute the maximum size of the Router Advertisement message
   length = sizeof(NdpRouterAdvMessage) +
      sizeof(NdpLinkLayerAddrOption) + sizeof(NdpMtuOption) +
      settings->prefixListLength * sizeof(NdpPrefixInfoOption) +
      settings->routeListLength * sizeof(NdpRouteInfoOption) +
      settings->contextListLength * sizeof(NdpContextOption);

   //Sanity check
   if((length + sizeof(Ipv6Header)) > IPV6_DEFAULT_MTU)
      return ERROR_FAILURE;

   //Allocate a memory buffer to hold the Router Advertisement message
   buffer = ipAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the message
   message = netBufferAt(buffer, offset);

   //Format Router Advertisement message
   message->type = ICMPV6_TYPE_ROUTER_ADV;
   message->code = 0;
   message->checksum = 0;
   message->curHopLimit = settings->curHopLimit;
   message->m = settings->managedFlag;
   message->o = settings->otherConfigFlag;
   message->h = settings->homeAgentFlag;
   message->prf = settings->preference;
   message->p = settings->proxyFlag;
   message->reserved = 0;
   message->routerLifetime = htons(routerLifetime);
   message->reachableTime = htonl(settings->reachableTime);
   message->retransTimer = htonl(settings->retransTimer);

   //If the Router Lifetime is zero, the preference value must be set to
   //zero by the sender
   if(routerLifetime == 0)
      message->prf = NDP_ROUTER_SEL_PREFERENCE_MEDIUM;

   //Length of the message, excluding any option
   length = sizeof(NdpRouterAdvMessage);

#if (ETH_SUPPORT == ENABLED)
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

   //A value of zero indicates that no MTU option is sent
   if(settings->linkMtu > 0)
   {
      NdpMtuOption mtuOption;

      //The MTU option specifies the recommended MTU for the link
      mtuOption.reserved = 0;
      mtuOption.mtu = htonl(settings->linkMtu);

      //Add MTU option
      ndpAddOption(message, &length, NDP_OPT_MTU,
         (uint8_t *) &mtuOption + sizeof(NdpOption),
         sizeof(NdpMtuOption) - sizeof(NdpOption));
   }

   //Loop through the list of IPv6 prefixes
   for(i = 0; i < settings->prefixListLength; i++)
   {
      NdpPrefixInfoOption prefixInfoOption;

      //The Prefix Information option provide hosts with on-link prefixes and
      //prefixes for Address Autoconfiguration
      prefixInfoOption.prefixLength = settings->prefixList[i].length;
      prefixInfoOption.l = settings->prefixList[i].onLinkFlag;
      prefixInfoOption.a = settings->prefixList[i].autonomousFlag;
      prefixInfoOption.r = 0;
      prefixInfoOption.reserved1 = 0;
      prefixInfoOption.validLifetime = htonl(settings->prefixList[i].validLifetime);
      prefixInfoOption.preferredLifetime = htonl(settings->prefixList[i].preferredLifetime);
      prefixInfoOption.reserved2 = 0;
      prefixInfoOption.prefix = settings->prefixList[i].prefix;

      //Add Prefix Information option (PIO)
      ndpAddOption(message, &length, NDP_OPT_PREFIX_INFORMATION,
         (uint8_t *) &prefixInfoOption + sizeof(NdpOption),
         sizeof(NdpPrefixInfoOption) - sizeof(NdpOption));
   }

   //Loop through the list of routes
   for(i = 0; i < settings->routeListLength; i++)
   {
      NdpRouteInfoOption routeInfoOption;

      //The Route Information option specifies prefixes that are reachable via
      //the router
      routeInfoOption.prefixLength = settings->routeList[i].length;
      routeInfoOption.reserved1 = 0;
      routeInfoOption.prf = settings->routeList[i].preference;
      routeInfoOption.reserved2 = 0;
      routeInfoOption.routeLifetime = htonl(settings->routeList[i].routeLifetime);
      routeInfoOption.prefix = settings->routeList[i].prefix;

      //Add Route Information option (RIO)
      ndpAddOption(message, &length, NDP_OPT_ROUTE_INFORMATION,
         (uint8_t *) &routeInfoOption + sizeof(NdpOption),
         sizeof(NdpRouteInfoOption) - sizeof(NdpOption));
   }

   //Loop through the list of 6LoWPAN compression contexts
   for(i = 0; i < settings->contextListLength; i++)
   {
      NdpContextOption contextOption;

      //The 6LoWPAN Context option (6CO) carries prefix information for LoWPAN
      //header compression
      contextOption.contextLength = settings->contextList[i].length;
      contextOption.reserved1 = 0;
      contextOption.c = settings->contextList[i].compression;
      contextOption.cid = settings->contextList[i].cid;
      contextOption.reserved2 = 0;
      contextOption.validLifetime = htons(settings->contextList[i].validLifetime);
      contextOption.contextPrefix = settings->contextList[i].prefix;

      //Calculate the length of the option in bytes
      n = sizeof(NdpContextOption) - sizeof(Ipv6Addr) + (contextOption.contextLength / 8);

      //Add 6LoWPAN Context option (6CO)
      ndpAddOption(message, &length, NDP_OPT_6LOWPAN_CONTEXT,
         (uint8_t *) &contextOption + sizeof(NdpOption), n - sizeof(NdpOption));
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
   IP_MIB_INC_COUNTER32(icmpv6MsgStatsTable.icmpMsgStatsOutPkts[ICMPV6_TYPE_ROUTER_ADV], 1);

   //Debug message
   TRACE_INFO("Sending Router Advertisement message (%" PRIuSIZE " bytes)...\r\n",
      length);

   //Dump message contents for debugging purpose
   ndpDumpRouterAdvMessage(message);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //By setting the Hop Limit to 255, Neighbor Discovery is immune to off-link
   //senders that accidentally or intentionally send NDP messages (refer to
   //RFC 4861, section 3.1)
   ancillary.ttl = NDP_HOP_LIMIT;

   //Send Router Advertisement message
   error = ipv6SendDatagram(interface, &pseudoHeader, buffer, offset,
      &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);
   //Return status code
   return error;
}

#endif
