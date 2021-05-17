/**
 * @file auto_ip.c
 * @brief Auto-IP (Dynamic Configuration of IPv4 Link-Local Addresses)
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
 * Auto-IP describes a method by which a host may automatically configure an
 * interface with an IPv4 address in the 169.254/16 prefix that is valid for
 * Link-Local communication on that interface. This is especially valuable in
 * environments where no other configuration mechanism is available. Refer to
 * the following RFCs for complete details:
 * - RFC 3927: Dynamic Configuration of IPv4 Link-Local Addresses
 * - RFC 5227: IPv4 Address Conflict Detection
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL AUTO_IP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "core/ethernet.h"
#include "ipv4/arp.h"
#include "ipv4/auto_ip.h"
#include "mdns/mdns_responder.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV4_SUPPORT == ENABLED && AUTO_IP_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t autoIpTickCounter;


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains Auto-IP settings
 **/

void autoIpGetDefaultSettings(AutoIpSettings *settings)
{
   //Use default interface
   settings->interface = netGetDefaultInterface();
   //Index of the IP address to be configured
   settings->ipAddrIndex = 0;

   //Initial link-local address to be used
   settings->linkLocalAddr = IPV4_UNSPECIFIED_ADDR;
   //Link state change event
   settings->linkChangeEvent = NULL;
   //FSM state change event
   settings->stateChangeEvent = NULL;
}


/**
 * @brief Auto-IP initialization
 * @param[in] context Pointer to the Auto-IP context
 * @param[in] settings Auto-IP specific settings
 * @return Error code
 **/

error_t autoIpInit(AutoIpContext *context, const AutoIpSettings *settings)
{
   NetInterface *interface;

   //Debug message
   TRACE_INFO("Initializing Auto-IP...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //The Auto-IP service must be bound to a valid interface
   if(settings->interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the underlying network interface
   interface = settings->interface;

   //Clear the Auto-IP context
   osMemset(context, 0, sizeof(AutoIpContext));
   //Save user settings
   context->settings = *settings;

   //Use default link-local address
   context->linkLocalAddr = settings->linkLocalAddr;
   //Reset conflict counter
   context->conflictCount = 0;

   //Auto-IP operation is currently suspended
   context->running = FALSE;
   //Initialize state machine
   context->state = AUTO_IP_STATE_INIT;

   //Attach the Auto-IP context to the network interface
   interface->autoIpContext = context;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Start Auto-IP process
 * @param[in] context Pointer to the Auto-IP context
 * @return Error code
 **/

error_t autoIpStart(AutoIpContext *context)
{
   uint_t i;
   NetInterface *interface;

   //Make sure the Auto-IP context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting Auto-IP...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Index of the IP address in the list of addresses assigned to the interface
   i = context->settings.ipAddrIndex;

   //The host address is not longer valid
   interface->ipv4Context.addrList[i].addr = IPV4_UNSPECIFIED_ADDR;
   interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_INVALID;

   //Clear subnet mask
   interface->ipv4Context.addrList[i].subnetMask = IPV4_UNSPECIFIED_ADDR;

   //Start Auto-IP operation
   context->running = TRUE;
   //Initialize state machine
   context->state = AUTO_IP_STATE_INIT;
   //Reset conflict counter
   context->conflictCount = 0;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Stop Auto-IP process
 * @param[in] context Pointer to the Auto-IP context
 * @return Error code
 **/

error_t autoIpStop(AutoIpContext *context)
{
   //Make sure the Auto-IP context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Stopping Auto-IP...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Suspend Auto-IP operation
   context->running = FALSE;
   //Reinitialize state machine
   context->state = AUTO_IP_STATE_INIT;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve current state
 * @param[in] context Pointer to the Auto-IP context
 * @return Current Auto-IP state
 **/

AutoIpState autoIpGetState(AutoIpContext *context)
{
   AutoIpState state;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Get current state
   state = context->state;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return current state
   return state;
}


/**
 * @brief Auto-IP timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * manage Auto-IP operation
 *
 * @param[in] context Pointer to the Auto-IP context
 **/

void autoIpTick(AutoIpContext *context)
{
   uint_t i;
   systime_t time;
   systime_t delay;
   NetInterface *interface;

   //Make sure Auto-IP has been properly instantiated
   if(context == NULL)
      return;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Index of the IP address in the list of addresses assigned to the interface
   i = context->settings.ipAddrIndex;

   //Get current time
   time = osGetSystemTime();

   //Check current state
   if(context->state == AUTO_IP_STATE_INIT)
   {
      //Wait for the link to be up before starting Auto-IP
      if(context->running && interface->linkState)
      {
         //Configure subnet mask
         interface->ipv4Context.addrList[i].subnetMask = AUTO_IP_MASK;

         //The address must be in the range from 169.54.1.0 to 169.254.254.255
         if(ntohl(context->linkLocalAddr) < ntohl(AUTO_IP_ADDR_MIN) ||
            ntohl(context->linkLocalAddr) > ntohl(AUTO_IP_ADDR_MAX))
         {
            //Generate a random link-local address
            autoIpGenerateAddr(&context->linkLocalAddr);
         }

         //Use the link-local address as a tentative address
         interface->ipv4Context.addrList[i].addr = context->linkLocalAddr;
         interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_TENTATIVE;

         //Clear conflict flag
         interface->ipv4Context.addrList[i].conflict = FALSE;

         //Initial random delay
         delay = netGetRandRange(0, AUTO_IP_PROBE_WAIT);

         //The number of conflicts exceeds the maximum acceptable value?
         if(context->conflictCount >= AUTO_IP_MAX_CONFLICTS)
         {
            //The host must limit the rate at which it probes for new addresses
            delay += AUTO_IP_RATE_LIMIT_INTERVAL;
         }

         //Verify the uniqueness of the link-local address
         autoIpChangeState(context, AUTO_IP_STATE_PROBING, delay);
      }
   }
   else if(context->state == AUTO_IP_STATE_PROBING)
   {
      //Any conflict detected?
      if(interface->ipv4Context.addrList[i].conflict)
      {
         //The address is already in use by some other host and
         //must not be assigned to the interface
         interface->ipv4Context.addrList[i].addr = IPV4_UNSPECIFIED_ADDR;
         interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_INVALID;

         //The host should maintain a counter of the number of address
         //conflicts it has experienced
         context->conflictCount++;

         //The host must pick a new random address...
         autoIpGenerateAddr(&context->linkLocalAddr);
         //...and repeat the process
         autoIpChangeState(context, AUTO_IP_STATE_INIT, 0);
      }
      else
      {
         //Check current time
         if(timeCompare(time, context->timestamp + context->timeout) >= 0)
         {
            //Address Conflict Detection is on-going?
            if(context->retransmitCount < AUTO_IP_PROBE_NUM)
            {
               //Conflict detection is done using ARP probes
               arpSendProbe(interface, context->linkLocalAddr);

               //Save the time at which the packet was sent
               context->timestamp = time;
               //Increment retransmission counter
               context->retransmitCount++;

               //Last probe packet sent?
               if(context->retransmitCount == AUTO_IP_PROBE_NUM)
               {
                  //Delay before announcing
                  context->timeout = AUTO_IP_ANNOUNCE_WAIT;
               }
               else
               {
                  //Maximum delay till repeated probe
                  context->timeout = netGetRandRange(AUTO_IP_PROBE_MIN,
                     AUTO_IP_PROBE_MAX);
               }
            }
            else
            {
               //The use of the IPv4 address is now unrestricted
               interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_VALID;

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
               //Restart mDNS probing process
               mdnsResponderStartProbing(interface->mdnsResponderContext);
#endif
               //The host must then announce its claimed address
               autoIpChangeState(context, AUTO_IP_STATE_ANNOUNCING, 0);
            }
         }
      }
   }
   else if(context->state == AUTO_IP_STATE_ANNOUNCING)
   {
      //Check current time
      if(timeCompare(time, context->timestamp + context->timeout) >= 0)
      {
         //An ARP announcement is identical to an ARP probe, except that
         //now the sender and target IP addresses are both set to the
         //host's newly selected IPv4 address
         arpSendRequest(interface, context->linkLocalAddr, &MAC_BROADCAST_ADDR);

         //Save the time at which the packet was sent
         context->timestamp = time;
         //Time interval between announcement packets
         context->timeout = AUTO_IP_ANNOUNCE_INTERVAL;
         //Increment retransmission counter
         context->retransmitCount++;

         //Announcing is complete?
         if(context->retransmitCount >= AUTO_IP_ANNOUNCE_NUM)
         {
            //Successful address autoconfiguration
            autoIpChangeState(context, AUTO_IP_STATE_CONFIGURED, 0);
            //Reset conflict counter
            context->conflictCount = 0;

            //Dump current IPv4 configuration for debugging purpose
            autoIpDumpConfig(context);
         }
      }
   }
   else if(context->state == AUTO_IP_STATE_CONFIGURED)
   {
      //Address Conflict Detection is an on-going process that is in effect
      //for as long as a host is using an IPv4 link-local address
      if(interface->ipv4Context.addrList[i].conflict)
      {
         //The host may elect to attempt to defend its address by recording
         //the time that the conflicting ARP packet was received, and then
         //broadcasting one single ARP announcement, giving its own IP and
         //hardware addresses as the sender addresses of the ARP
#if (AUTO_IP_BCT_SUPPORT == ENABLED)
         arpSendProbe(interface, context->linkLocalAddr);
#else
         arpSendRequest(interface, context->linkLocalAddr, &MAC_BROADCAST_ADDR);
#endif
         //Clear conflict flag
         interface->ipv4Context.addrList[i].conflict = FALSE;

         //The host can then continue to use the address normally without
         //any further special action
         autoIpChangeState(context, AUTO_IP_STATE_DEFENDING, 0);
      }
   }
   else if(context->state == AUTO_IP_STATE_DEFENDING)
   {
      //if this is not the first conflicting ARP packet the host has seen, and
      //the time recorded for the previous conflicting ARP packet is recent,
      //within DEFEND_INTERVAL seconds, then the host must immediately cease
      //using this address
      if(interface->ipv4Context.addrList[i].conflict)
      {
         //The link-local address cannot be used anymore
         interface->ipv4Context.addrList[i].addr = IPV4_UNSPECIFIED_ADDR;
         interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_INVALID;

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
         //Restart mDNS probing process
         mdnsResponderStartProbing(interface->mdnsResponderContext);
#endif
         //The host must pick a new random address...
         autoIpGenerateAddr(&context->linkLocalAddr);
         //...and probes/announces again
         autoIpChangeState(context, AUTO_IP_STATE_INIT, 0);
      }
      else
      {
         //Check whether the DEFEND_INTERVAL has elapsed
         if(timeCompare(time, context->timestamp + AUTO_IP_DEFEND_INTERVAL) >= 0)
         {
            //The host can continue to use its link-local address
            autoIpChangeState(context, AUTO_IP_STATE_CONFIGURED, 0);
         }
      }
   }
}


/**
 * @brief Callback function for link change event
 * @param[in] context Pointer to the Auto-IP context
 **/

void autoIpLinkChangeEvent(AutoIpContext *context)
{
   uint_t i;
   NetInterface *interface;

   //Make sure Auto-IP has been properly instantiated
   if(context == NULL)
      return;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Index of the IP address in the list of addresses assigned to the interface
   i = context->settings.ipAddrIndex;

   //Check whether Auto-IP is enabled
   if(context->running)
   {
      //The host address is not longer valid
      interface->ipv4Context.addrList[i].addr = IPV4_UNSPECIFIED_ADDR;
      interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_INVALID;

      //Clear subnet mask
      interface->ipv4Context.addrList[i].subnetMask = IPV4_UNSPECIFIED_ADDR;

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
      //Restart mDNS probing process
      mdnsResponderStartProbing(interface->mdnsResponderContext);
#endif
   }

   //Reinitialize state machine
   context->state = AUTO_IP_STATE_INIT;
   //Reset conflict counter
   context->conflictCount = 0;

   //Any registered callback?
   if(context->settings.linkChangeEvent != NULL)
   {
      //Release exclusive access
      osReleaseMutex(&netMutex);
      //Invoke user callback function
      context->settings.linkChangeEvent(context, interface, interface->linkState);
      //Get exclusive access
      osAcquireMutex(&netMutex);
   }
}


/**
 * @brief Update Auto-IP FSM state
 * @param[in] context Pointer to the Auto-IP context
 * @param[in] newState New Auto-IP state to switch to
 * @param[in] delay Initial delay
 **/

void autoIpChangeState(AutoIpContext *context,
   AutoIpState newState, systime_t delay)
{
   NetInterface *interface;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Set time stamp
   context->timestamp = osGetSystemTime();
   //Set initial delay
   context->timeout = delay;
   //Reset retransmission counter
   context->retransmitCount = 0;
   //Switch to the new state
   context->state = newState;

   //Any registered callback?
   if(context->settings.stateChangeEvent != NULL)
   {
      //Release exclusive access
      osReleaseMutex(&netMutex);
      //Invoke user callback function
      context->settings.stateChangeEvent(context, interface, newState);
      //Get exclusive access
      osAcquireMutex(&netMutex);
   }
}


/**
 * @brief Generate a random link-local address
 * @param[out] ipAddr Random link-local address
 **/

void autoIpGenerateAddr(Ipv4Addr *ipAddr)
{
   uint32_t n;

   //Generate a random address in the range from 169.254.1.0 to 169.254.254.255
   n = netGetRand() % ntohl(AUTO_IP_ADDR_MAX - AUTO_IP_ADDR_MIN);
   n += ntohl(AUTO_IP_ADDR_MIN);

   //Convert the resulting address to network byte order
   *ipAddr = htonl(n);
}


/**
 * @brief Dump Auto-IP configuration for debugging purpose
 * @param[in] context Pointer to the Auto-IP context
 **/

void autoIpDumpConfig(AutoIpContext *context)
{
#if (AUTO_IP_TRACE_LEVEL >= TRACE_LEVEL_INFO)
   uint_t i;
   NetInterface *interface;
   Ipv4Context *ipv4Context;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Point to the IPv4 context
   ipv4Context = &interface->ipv4Context;

   //Index of the IP address in the list of addresses assigned to the interface
   i = context->settings.ipAddrIndex;

   //Debug message
   TRACE_INFO("\r\n");
   TRACE_INFO("Auto-IP configuration:\r\n");

   //Link-local address
   TRACE_INFO("  Link-local Address = %s\r\n",
      ipv4AddrToString(ipv4Context->addrList[i].addr, NULL));

   //Subnet mask
   TRACE_INFO("  Subnet Mask = %s\r\n",
      ipv4AddrToString(ipv4Context->addrList[i].subnetMask, NULL));

   //Debug message
   TRACE_INFO("\r\n");
#endif
}

#endif
