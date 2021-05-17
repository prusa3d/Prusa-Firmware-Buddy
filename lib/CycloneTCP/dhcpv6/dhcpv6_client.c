/**
 * @file dhcpv6_client.c
 * @brief DHCPv6 client (Dynamic Host Configuration Protocol for IPv6)
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
 * The Dynamic Host Configuration Protocol for IPv6 enables DHCP servers to
 * pass configuration parameters such as IPv6 network addresses to IPv6
 * nodes. This protocol is a stateful counterpart to IPv6 Stateless Address
 * Autoconfiguration (RFC 2462), and can be used separately or concurrently
 * with the latter to obtain configuration parameters. Refer to RFC 3315
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL DHCPV6_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include "core/net.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "ipv6/ndp.h"
#include "dhcpv6/dhcpv6_client.h"
#include "dhcpv6/dhcpv6_common.h"
#include "dhcpv6/dhcpv6_debug.h"
#include "dns/dns_common.h"
#include "date_time.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED && DHCPV6_CLIENT_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t dhcpv6ClientTickCounter;

//Requested DHCPv6 options
static const uint16_t dhcpv6OptionList[] =
{
   HTONS(DHCPV6_OPTION_DNS_SERVERS),
   HTONS(DHCPV6_OPTION_DOMAIN_LIST),
   HTONS(DHCPV6_OPTION_FQDN)
};


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains DHCPv6 client settings
 **/

void dhcpv6ClientGetDefaultSettings(Dhcpv6ClientSettings *settings)
{
   //Use default interface
   settings->interface = netGetDefaultInterface();

   //Support for quick configuration using rapid commit
   settings->rapidCommit = FALSE;
   //Use the DNS servers provided by the DHCPv6 server
   settings->manualDnsConfig = FALSE;
   //DHCPv6 configuration timeout
   settings->timeout = 0;
   //DHCPv6 configuration timeout event
   settings->timeoutEvent = NULL;
   //Link state change event
   settings->linkChangeEvent = NULL;
   //FSM state change event
   settings->stateChangeEvent = NULL;
}


/**
 * @brief DHCPv6 client initialization
 * @param[in] context Pointer to the DHCPv6 client context
 * @param[in] settings DHCPv6 client specific settings
 * @return Error code
 **/

error_t dhcpv6ClientInit(Dhcpv6ClientContext *context, const Dhcpv6ClientSettings *settings)
{
   error_t error;
   NetInterface *interface;

   //Debug message
   TRACE_INFO("Initializing DHCPv6 client...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //The DHCPv6 client must be bound to a valid interface
   if(settings->interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the underlying network interface
   interface = settings->interface;

   //Clear the DHCPv6 client context
   osMemset(context, 0, sizeof(Dhcpv6ClientContext));
   //Save user settings
   context->settings = *settings;

   //Generate client's DUID
   error = dhcpv6ClientGenerateDuid(context);
   //any error to report?
   if(error)
      return error;

   //Generate client's fully qualified domain name
   error = dhcpv6ClientGenerateFqdn(context);
   //any error to report?
   if(error)
      return error;

   //Callback function to be called when a DHCPv6 message is received
   error = udpAttachRxCallback(interface, DHCPV6_CLIENT_PORT,
      dhcpv6ClientProcessMessage, context);
   //Failed to register callback function?
   if(error)
      return error;

   //DHCPv6 client is currently suspended
   context->running = FALSE;
   //Initialize state machine
   context->state = DHCPV6_STATE_INIT;

   //Attach the DHCPv6 client context to the network interface
   interface->dhcpv6ClientContext = context;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Start DHCPv6 client
 * @param[in] context Pointer to the DHCPv6 client context
 * @return Error code
 **/

error_t dhcpv6ClientStart(Dhcpv6ClientContext *context)
{
   NetInterface *interface;

   //Make sure the DHCPv6 client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting DHCPv6 client...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Flush the list of IPv6 addresses from the client's IA
   dhcpv6ClientFlushAddrList(context);

   //Automatic DNS server configuration?
   if(!context->settings.manualDnsConfig)
   {
      //Clear the list of DNS servers
      ipv6FlushDnsServerList(interface);
   }

   //Check if the link is up?
   if(interface->linkState)
   {
      //A link-local address is formed by combining the well-known
      //link-local prefix fe80::/10 with the interface identifier
      dhcpv6ClientGenerateLinkLocalAddr(context);
   }

   //Start DHCPv6 client
   context->running = TRUE;
   //Initialize state machine
   context->state = DHCPV6_STATE_INIT;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Stop DHCPv6 client
 * @param[in] context Pointer to the DHCPv6 client context
 * @return Error code
 **/

error_t dhcpv6ClientStop(Dhcpv6ClientContext *context)
{
   //Make sure the DHCPv6 client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Stopping DHCPv6 client...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Stop DHCPv6 client
   context->running = FALSE;
   //Reinitialize state machine
   context->state = DHCPV6_STATE_INIT;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Release DHCPv6 lease
 * @param[in] context Pointer to the DHCPv6 client context
 * @return Error code
 **/

error_t dhcpv6ClientRelease(Dhcpv6ClientContext *context)
{
   uint_t i;
   NetInterface *interface;
   Dhcpv6ClientAddrEntry *entry;

   //Check parameter
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Releasing DHCPv6 lease...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Check whether the DHCPv6 client is running
   if(context->running)
   {
      //BOUND state?
      if(context->state == DHCPV6_STATE_BOUND)
      {
         //Loop through the IPv6 addresses recorded by the DHCPv6 client
         for(i = 0; i < DHCPV6_CLIENT_ADDR_LIST_SIZE; i++)
         {
            //Point to the current entry
            entry = &context->ia.addrList[i];

            //Valid IPv6 address?
            if(entry->validLifetime > 0)
            {
               //The client must stop using the addresses being released as soon
               //as the client begins the Release message exchange process
               ipv6RemoveAddr(interface, &entry->addr);
            }
         }

         //Switch to the RELEASE state
         dhcpv6ClientChangeState(context, DHCPV6_STATE_RELEASE, 0);
      }
      else
      {
         //Stop DHCPv6 client
         context->running = FALSE;
         //Reinitialize state machine
         context->state = DHCPV6_STATE_INIT;
      }
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve current state
 * @param[in] context Pointer to the DHCPv6 client context
 * @return Current DHCPv6 client state
 **/

Dhcpv6State dhcpv6ClientGetState(Dhcpv6ClientContext *context)
{
   Dhcpv6State state;

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
 * @brief DHCPv6 client timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * manage DHCPv6 client operation
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/


void dhcpv6ClientTick(Dhcpv6ClientContext *context)
{
   //Make sure the DHCPv6 client has been properly instantiated
   if(context == NULL)
      return;

   //DHCPv6 client finite state machine
   switch(context->state)
   {
   //Process INIT state
   case DHCPV6_STATE_INIT:
      //This is the initialization state, where a client begins the process of
      //acquiring a lease. It also returns here when a lease ends, or when a
      //lease negotiation fails
      dhcpv6ClientStateInit(context);
      break;

   //Process SOLICIT state
   case DHCPV6_STATE_SOLICIT:
      //The client sends a Solicit message to locate servers
      dhcpv6ClientStateSolicit(context);
      break;

   //Process REQUEST state
   case DHCPV6_STATE_REQUEST:
      //The client sends a Request message to request configuration
      //parameters, including IP addresses, from a specific server
      dhcpv6ClientStateRequest(context);
      break;

   //Process INIT-CONFIRM state
   case DHCPV6_STATE_INIT_CONFIRM:
      //When a client that already has a valid lease starts up after a
      //power-down or reboot, it starts here instead of the INIT state
      dhcpv6ClientStateInitConfirm(context);
      break;

   //Process CONFIRM state
   case DHCPV6_STATE_CONFIRM:
      //The client sends a Confirm message to any available server
      //to determine whether the addresses it was assigned are still
      //appropriate to the link to which the client is connected
      dhcpv6ClientStateConfirm(context);
      break;

   //Process DAD state
   case DHCPV6_STATE_DAD:
      //The client should perform duplicate address detection on each
      //of the addresses in any IAs it receives in the Reply message
      //before using that address for traffic
      dhcpv6ClientStateDad(context);
      break;

   //Process BOUND state
   case DHCPV6_STATE_BOUND:
      //The client has a valid lease and is in its normal operating state
      dhcpv6ClientStateBound(context);
      break;

   //Process RENEW state
   case DHCPV6_STATE_RENEW:
      //The client sends a Renew message to the server that originally
      //provided the client's addresses and configuration parameters to
      //extend the lifetimes on the addresses assigned to the client
      //and to update other configuration parameters
      dhcpv6ClientStateRenew(context);
      break;

   //Process REBIND state
   case DHCPV6_STATE_REBIND:
      //The client sends a Rebind message to any available server to extend
      //the lifetimes on the addresses assigned to the client and to update
      //other configuration parameters. This message is sent after a client
      //receives no response to a Renew message
      dhcpv6ClientStateRebind(context);
      break;

   //Process RELEASE state
   case DHCPV6_STATE_RELEASE:
      //To release one or more addresses, a client sends a Release message
      //to the server
      dhcpv6ClientStateRelease(context);
      break;

   //Process DECLINE state
   case DHCPV6_STATE_DECLINE:
      //If a client detects that one or more addresses assigned to it by a
      //server are already in use by another node, the client sends a Decline
      //message to the server to inform it that the address is suspect
      dhcpv6ClientStateDecline(context);
      break;

   //Invalid state
   default:
      //Switch to the default state
      context->state = DHCPV6_STATE_INIT;
      break;
   }
}


/**
 * @brief Callback function for link change event
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientLinkChangeEvent(Dhcpv6ClientContext *context)
{
   NetInterface *interface;

   //Make sure the DHCPv6 client has been properly instantiated
   if(context == NULL)
      return;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Check whether the DHCPv6 client is running
   if(context->running)
   {
      //Automatic DNS server configuration?
      if(!context->settings.manualDnsConfig)
      {
         //Clear the list of DNS servers
         ipv6FlushDnsServerList(interface);
      }

      //Link-up event?
      if(interface->linkState)
      {
         //A link-local address is formed by combining the well-known
         //link-local prefix fe80::/10 with the interface identifier
         dhcpv6ClientGenerateLinkLocalAddr(context);
      }
   }

   //Check the state of the DHCPv6 client
   switch(context->state)
   {
   case DHCPV6_STATE_INIT_CONFIRM:
   case DHCPV6_STATE_CONFIRM:
   case DHCPV6_STATE_DAD:
   case DHCPV6_STATE_BOUND:
   case DHCPV6_STATE_RENEW:
   case DHCPV6_STATE_REBIND:
      //The client already has a valid lease
      context->state = DHCPV6_STATE_INIT_CONFIRM;
      break;
   case DHCPV6_STATE_RELEASE:
      //Stop DHCPv6 client
      context->running = FALSE;
      //Reinitialize state machine
      context->state = DHCPV6_STATE_INIT;
      break;
   default:
      //Switch to the INIT state
      context->state = DHCPV6_STATE_INIT;
      break;
   }

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
 * @brief INIT state
 *
 * This is the initialization state, where a client begins the process of
 * acquiring a lease. It also returns here when a lease ends, or when a
 * lease negotiation fails
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientStateInit(Dhcpv6ClientContext *context)
{
   systime_t delay;
   NetInterface *interface;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Check whether the DHCPv6 client is running
   if(context->running)
   {
      //Wait for the link to be up before starting DHCPv6 configuration
      if(interface->linkState)
      {
         //Make sure that a valid link-local address has been assigned to the interface
         if(ipv6GetLinkLocalAddrState(interface) == IPV6_ADDR_STATE_PREFERRED)
         {
            //Flush the list of IPv6 addresses from the client's IA
            dhcpv6ClientFlushAddrList(context);

            //The first Solicit message from the client on the interface must be
            //delayed by a random amount of time between 0 and SOL_MAX_DELAY
            delay = dhcpv6RandRange(0, DHCPV6_CLIENT_SOL_MAX_DELAY);

            //Record the time at which the client started
            //the address acquisition process
            context->configStartTime = osGetSystemTime();
            //Clear flag
            context->timeoutEventDone = FALSE;

            //Switch to the SOLICIT state
            dhcpv6ClientChangeState(context, DHCPV6_STATE_SOLICIT, delay);
         }
      }
   }
}


/**
 * @brief SOLICIT state
 *
 * A client uses the Solicit message to discover DHCPv6 servers
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientStateSolicit(Dhcpv6ClientContext *context)
{
   systime_t time;

   //Get current time
   time = osGetSystemTime();

   //Check current time
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //Check retransmission counter
      if(context->retransmitCount == 0)
      {
         //Reset server preference value
         context->serverPreference = -1;
         //Generate a 24-bit transaction ID
         context->transactionId = netGetRand() & 0x00FFFFFF;

         //Send a Solicit message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_SOLICIT);

         //Save the time at which the message was sent
         context->exchangeStartTime = time;
         context->timestamp = time;

         //If the client is waiting for an Advertise message, the first RT
         //must be selected to be strictly greater than IRT
         context->timeout = DHCPV6_CLIENT_SOL_TIMEOUT +
            abs(dhcpv6Rand(DHCPV6_CLIENT_SOL_TIMEOUT));

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else
      {
         //Check whether a valid Advertise message has been received
         if(context->serverPreference >= 0)
         {
            //Continue configuration procedure
            dhcpv6ClientChangeState(context, DHCPV6_STATE_REQUEST, 0);
         }
         else
         {
            //Send a Solicit message
            dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_SOLICIT);

            //Save the time at which the message was sent
            context->timestamp = time;

            //The RT is doubled for each subsequent retransmission
            context->timeout = context->timeout * 2 + dhcpv6Rand(context->timeout);

            //MRT specifies an upper bound on the value of RT
            if(context->timeout > DHCPV6_CLIENT_SOL_MAX_RT)
            {
               //Compute retransmission timeout
               context->timeout = DHCPV6_CLIENT_SOL_MAX_RT +
                  dhcpv6Rand(DHCPV6_CLIENT_SOL_MAX_RT);
            }

            //Increment retransmission counter
            context->retransmitCount++;
         }
      }
   }

   //Manage DHCPv6 configuration timeout
   dhcpv6ClientCheckTimeout(context);
}


/**
 * @brief REQUEST state
 *
 * The client uses a Request message to populate IAs with addresses and obtain
 * other configuration information. The client includes one or more more IA
 * options in the Request message. The server then returns addresses and other
 * information about the IAs to the client in IA options in a Reply message
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientStateRequest(Dhcpv6ClientContext *context)
{
   systime_t time;

   //Get current time
   time = osGetSystemTime();

   //Check current time
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //Check retransmission counter
      if(context->retransmitCount == 0)
      {
         //Generate a 24-bit transaction ID
         context->transactionId = netGetRand() & 0x00FFFFFF;

         //Send a Request message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_REQUEST);

         //Save the time at which the message was sent
         context->exchangeStartTime = time;
         context->timestamp = time;

         //Initial retransmission timeout
         context->timeout = DHCPV6_CLIENT_REQ_TIMEOUT +
            dhcpv6Rand(DHCPV6_CLIENT_REQ_TIMEOUT);

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else if(context->retransmitCount < DHCPV6_CLIENT_REQ_MAX_RC)
      {
         //Send a Request message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_REQUEST);

         //Save the time at which the message was sent
         context->timestamp = time;

         //The RT is doubled for each subsequent retransmission
         context->timeout = context->timeout * 2 + dhcpv6Rand(context->timeout);

         //MRT specifies an upper bound on the value of RT
         if(context->timeout > DHCPV6_CLIENT_REQ_MAX_RT)
         {
            //Compute retransmission timeout
            context->timeout = DHCPV6_CLIENT_REQ_MAX_RT +
               dhcpv6Rand(DHCPV6_CLIENT_REQ_MAX_RT);
         }

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else
      {
         //If the client does not receive a response within a reasonable
         //period of time, then it restarts the initialization procedure
         dhcpv6ClientChangeState(context, DHCPV6_STATE_INIT, 0);
      }
   }

   //Manage DHCPv6 configuration timeout
   dhcpv6ClientCheckTimeout(context);
}


/**
 * @brief INIT-CONFIRM state
 *
 * When a client that already has a valid lease starts up after a
 * power-down or reboot, it starts here instead of the INIT state
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientStateInitConfirm(Dhcpv6ClientContext *context)
{
   systime_t delay;
   NetInterface *interface;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Check whether the DHCPv6 client is running
   if(context->running)
   {
      //Wait for the link to be up before starting DHCPv6 configuration
      if(interface->linkState)
      {
         //Make sure that a valid link-local address has been assigned to the interface
         if(ipv6GetLinkLocalAddrState(interface) == IPV6_ADDR_STATE_PREFERRED)
         {
            //The first Confirm message from the client on the interface must be
            //delayed by a random amount of time between 0 and CNF_MAX_DELAY
            delay = dhcpv6RandRange(0, DHCPV6_CLIENT_CNF_MAX_DELAY);

            //Record the time at which the client started
            //the address acquisition process
            context->configStartTime = osGetSystemTime();
            //Clear flag
            context->timeoutEventDone = FALSE;

            //Switch to the CONFIRM state
            dhcpv6ClientChangeState(context, DHCPV6_STATE_CONFIRM, delay);
         }
      }
   }
}


/**
 * @brief CONFIRM state
 *
 * Whenever a client may have moved to a new link, the prefixes from
 * the addresses assigned to the interfaces on that link may no longer
 * be appropriate for the link to which the client is attached. In such
 * the client must initiate a Confirm/Reply message exchange
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientStateConfirm(Dhcpv6ClientContext *context)
{
   systime_t time;

   //Get current time
   time = osGetSystemTime();

   //Check current time
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //Check retransmission counter
      if(context->retransmitCount == 0)
      {
         //Generate a 24-bit transaction ID
         context->transactionId = netGetRand() & 0x00FFFFFF;

         //Send a Confirm message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_CONFIRM);

         //Save the time at which the client sent the first message
         context->exchangeStartTime = time;
         context->timestamp = time;

         //Initial retransmission timeout
         context->timeout = DHCPV6_CLIENT_CNF_TIMEOUT +
            dhcpv6Rand(DHCPV6_CLIENT_CNF_TIMEOUT);

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else
      {
         //Send a Confirm message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_CONFIRM);

         //Save the time at which the message was sent
         context->timestamp = time;

         //The RT is doubled for each subsequent retransmission
         context->timeout = context->timeout * 2 + dhcpv6Rand(context->timeout);

         //MRT specifies an upper bound on the value of RT
         if(context->timeout > DHCPV6_CLIENT_CNF_MAX_RT)
         {
            //Compute retransmission timeout
            context->timeout = DHCPV6_CLIENT_CNF_MAX_RT +
               dhcpv6Rand(DHCPV6_CLIENT_CNF_MAX_RT);
         }

         //Increment retransmission counter
         context->retransmitCount++;
      }
   }
   else
   {
      //Check retransmission counter
      if(context->retransmitCount > 0)
      {
         //The message exchange fails once MRD seconds have elapsed since
         //the client first transmitted the message
         if(timeCompare(time, context->exchangeStartTime + DHCPV6_CLIENT_CNF_MAX_RD) >= 0)
         {
            //If the client receives no responses before the message transmission
            //process terminates, the client should continue to use any IP
            //addresses using the last known lifetimes for those addresses
            dhcpv6ClientChangeState(context, DHCPV6_STATE_INIT, 0);
         }
      }
   }

   //Manage DHCPv6 configuration timeout
   dhcpv6ClientCheckTimeout(context);
}


/**
 * @brief DAD state
 *
 * The client perform duplicate address detection on each
 * of the addresses in any IAs it receives in the Reply message
 * before using that address for traffic
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientStateDad(Dhcpv6ClientContext *context)
{
   uint_t i;
   NetInterface *interface;
   Ipv6AddrState state;
   Dhcpv6ClientAddrEntry *entry;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Loop through the IPv6 addresses recorded by the DHCPv6 client
   for(i = 0; i < DHCPV6_CLIENT_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->ia.addrList[i];

      //Check the IPv6 address is a tentative address?
      if(entry->validLifetime > 0)
      {
         //Get the state of the current IPv6 address
         state = ipv6GetAddrState(interface, &entry->addr);

         //Duplicate Address Detection in progress?
         if(state == IPV6_ADDR_STATE_TENTATIVE)
         {
            //Exit immediately
            return;
         }
         //Duplicate Address Detection failed?
         else if(state == IPV6_ADDR_STATE_INVALID)
         {
            //Switch to the DECLINE state
            dhcpv6ClientChangeState(context, DHCPV6_STATE_DECLINE, 0);
            //Exit immediately
            return;
         }
      }
   }

   //Dump current DHCPv6 configuration for debugging purpose
   dhcpv6ClientDumpConfig(context);
   //Switch to the BOUND state
   dhcpv6ClientChangeState(context, DHCPV6_STATE_BOUND, 0);
}


/**
 * @brief BOUND state
 *
 * Client has a valid lease and is in its normal operating state
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientStateBound(Dhcpv6ClientContext *context)
{
   systime_t t1;
   systime_t time;

   //Get current time
   time = osGetSystemTime();

   //A client will never attempt to extend the lifetime of any
   //address in an IA with T1 set to 0xffffffff
   if(context->ia.t1 != DHCPV6_INFINITE_TIME)
   {
      //Convert T1 to milliseconds
      if(context->ia.t1 < (MAX_DELAY / 1000))
         t1 = context->ia.t1 * 1000;
      else
         t1 = MAX_DELAY;

      //Check the time elapsed since the lease was obtained
      if(timeCompare(time, context->leaseStartTime + t1) >= 0)
      {
         //Record the time at which the client started the address renewal process
         context->configStartTime = time;

         //Enter the RENEW state
         dhcpv6ClientChangeState(context, DHCPV6_STATE_RENEW, 0);
      }
   }
}


/**
 * @brief RENEW state
 *
 * The client sends a Renew message to the server that originally
 * provided the client's addresses and configuration parameters to
 * extend the lifetimes on the addresses assigned to the client
 * and to update other configuration parameters
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientStateRenew(Dhcpv6ClientContext *context)
{
   systime_t t2;
   systime_t time;

   //Get current time
   time = osGetSystemTime();

   //Check current time
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //Check retransmission counter
      if(context->retransmitCount == 0)
      {
         //Generate a 24-bit transaction ID
         context->transactionId = netGetRand() & 0x00FFFFFF;

         //Send a Renew message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_RENEW);

         //Save the time at which the message was sent
         context->exchangeStartTime = time;
         context->timestamp = time;

         //Initial retransmission timeout
         context->timeout = DHCPV6_CLIENT_REN_TIMEOUT +
            dhcpv6Rand(DHCPV6_CLIENT_REN_TIMEOUT);
      }
      else
      {
         //Send a Renew message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_RENEW);

         //Save the time at which the message was sent
         context->timestamp = time;

         //The RT is doubled for each subsequent retransmission
         context->timeout = context->timeout * 2 + dhcpv6Rand(context->timeout);

         //MRT specifies an upper bound on the value of RT
         if(context->timeout > DHCPV6_CLIENT_REN_MAX_RT)
         {
            //Compute retransmission timeout
            context->timeout = DHCPV6_CLIENT_REN_MAX_RT +
               dhcpv6Rand(DHCPV6_CLIENT_REN_MAX_RT);
         }
      }

      //Increment retransmission counter
      context->retransmitCount++;
   }
   else
   {
      //A client will never attempt to use a Rebind message to locate a
      //different server to extend the lifetime of any address in an IA
      //with T2 set to 0xffffffff
      if(context->ia.t2 != DHCPV6_INFINITE_TIME)
      {
         //Convert T2 to milliseconds
         if(context->ia.t2 < (MAX_DELAY / 1000))
            t2 = context->ia.t2 * 1000;
         else
            t2 = MAX_DELAY;

         //Check whether T2 timer has expired
         if(timeCompare(time, context->leaseStartTime + t2) >= 0)
         {
            //Switch to the REBIND state
            dhcpv6ClientChangeState(context, DHCPV6_STATE_REBIND, 0);
         }
      }
   }
}


/**
 * @brief REBIND state
 *
 * The client sends a Rebind message to any available server to extend
 * the lifetimes on the addresses assigned to the client and to update
 * other configuration parameters. This message is sent after a client
 * receives no response to a Renew message
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientStateRebind(Dhcpv6ClientContext *context)
{
   uint_t i;
   systime_t time;
   NetInterface *interface;
   Dhcpv6ClientAddrEntry *entry;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Get current time
   time = osGetSystemTime();

   //Check current time
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //Check retransmission counter
      if(context->retransmitCount == 0)
      {
         //Generate a 24-bit transaction ID
         context->transactionId = netGetRand() & 0x00FFFFFF;

         //Send a Rebind message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_REBIND);

         //Save the time at which the message was sent
         context->exchangeStartTime = time;
         context->timestamp = time;

         //Initial retransmission timeout
         context->timeout = DHCPV6_CLIENT_REB_TIMEOUT +
            dhcpv6Rand(DHCPV6_CLIENT_REB_TIMEOUT);
      }
      else
      {
         //Send a Rebind message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_REBIND);

         //Save the time at which the message was sent
         context->timestamp = time;

         //The RT is doubled for each subsequent retransmission
         context->timeout = context->timeout * 2 + dhcpv6Rand(context->timeout);

         //MRT specifies an upper bound on the value of RT
         if(context->timeout > DHCPV6_CLIENT_REB_MAX_RT)
         {
            //Compute retransmission timeout
            context->timeout = DHCPV6_CLIENT_REB_MAX_RT +
               dhcpv6Rand(DHCPV6_CLIENT_REB_MAX_RT);
         }
      }

      //Increment retransmission counter
      context->retransmitCount++;
   }
   else
   {
      //Loop through the IPv6 addresses recorded by the DHCPv6 client
      for(i = 0; i < DHCPV6_CLIENT_ADDR_LIST_SIZE; i++)
      {
         //Point to the current entry
         entry = &context->ia.addrList[i];

         //Valid IPv6 address?
         if(entry->validLifetime > 0)
         {
            //Check whether the valid lifetime has expired
            if(ipv6GetAddrState(interface, &entry->addr) == IPV6_ADDR_STATE_INVALID)
            {
               //Restart DHCPv6 configuration
               dhcpv6ClientChangeState(context, DHCPV6_STATE_INIT, 0);
            }
         }
      }
   }
}


/**
 * @brief RELEASE state
 *
 * To release one or more addresses, a client sends a Release message
 * to the server
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientStateRelease(Dhcpv6ClientContext *context)
{
   systime_t time;

   //Get current time
   time = osGetSystemTime();

   //Check current time
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //Check retransmission counter
      if(context->retransmitCount == 0)
      {
         //Generate a 24-bit transaction ID
         context->transactionId = netGetRand() & 0x00FFFFFF;

         //Send a Release message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_RELEASE);

         //Save the time at which the message was sent
         context->exchangeStartTime = time;
         context->timestamp = time;

         //Initial retransmission timeout
         context->timeout = DHCPV6_CLIENT_REL_TIMEOUT +
            dhcpv6Rand(DHCPV6_CLIENT_REL_TIMEOUT);

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else if(context->retransmitCount < DHCPV6_CLIENT_REL_MAX_RC)
      {
         //Send a Release message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_RELEASE);

         //Save the time at which the message was sent
         context->timestamp = time;

         //The RT is doubled for each subsequent retransmission
         context->timeout = context->timeout * 2 + dhcpv6Rand(context->timeout);

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else
      {
         //Implementations should retransmit one or more times, but may
         //choose to terminate the retransmission procedure early
         context->running = FALSE;

         //Reinitialize state machine
         dhcpv6ClientChangeState(context, DHCPV6_STATE_INIT, 0);
      }
   }
}


/**
 * @brief DECLINE state
 *
 * If a client detects that one or more addresses assigned to it by a
 * server are already in use by another node, the client sends a Decline
 * message to the server to inform it that the address is suspect
 *
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientStateDecline(Dhcpv6ClientContext *context)
{
   systime_t time;

   //Get current time
   time = osGetSystemTime();

   //Check current time
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //Check retransmission counter
      if(context->retransmitCount == 0)
      {
         //Generate a 24-bit transaction ID
         context->transactionId = netGetRand() & 0x00FFFFFF;

         //Send a Decline message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_DECLINE);

         //Save the time at which the message was sent
         context->exchangeStartTime = time;
         context->timestamp = time;

         //Initial retransmission timeout
         context->timeout = DHCPV6_CLIENT_DEC_TIMEOUT +
            dhcpv6Rand(DHCPV6_CLIENT_DEC_TIMEOUT);

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else if(context->retransmitCount < DHCPV6_CLIENT_DEC_MAX_RC)
      {
         //Send a Decline message
         dhcpv6ClientSendMessage(context, DHCPV6_MSG_TYPE_DECLINE);

         //Save the time at which the message was sent
         context->timestamp = time;

         //The RT is doubled for each subsequent retransmission
         context->timeout = context->timeout * 2 + dhcpv6Rand(context->timeout);

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else
      {
         //If the client does not receive a response within a reasonable
         //period of time, then it restarts the initialization procedure
         dhcpv6ClientChangeState(context, DHCPV6_STATE_INIT, 0);
      }
   }
}


/**
 * @brief Send Solicit message
 * @param[in] context Pointer to the DHCPv6 client context
 * @param[in] type DHCPv6 message type
 * @return Error code
 **/

error_t dhcpv6ClientSendMessage(Dhcpv6ClientContext *context,
   Dhcpv6MessageType type)
{
   error_t error;
   uint_t i;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   NetInterface *interface;
   Dhcpv6Message *message;
   Dhcpv6Option *option;
   Dhcpv6IaNaOption iaNaOption;
   Dhcpv6IaAddrOption iaAddrOption;
   Dhcpv6FqdnOption *fqdnOption;
   Dhcpv6ElapsedTimeOption elapsedTimeOption;
   Dhcpv6ClientAddrEntry *entry;
   IpAddr destIpAddr;
   NetTxAncillary ancillary;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Allocate a memory buffer to hold the DHCPv6 message
   buffer = udpAllocBuffer(DHCPV6_MAX_MSG_SIZE, &offset);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the DHCPv6 message
   message = netBufferAt(buffer, offset);

   //Set DHCPv6 message type
   message->msgType = type;

   //The transaction ID is chosen by the client
   STORE24BE(context->transactionId, message->transactionId);

   //Size of the DHCPv6 message
   length = sizeof(Dhcpv6Message);

   //The client must include a Client Identifier option
   //to identify itself to the server
   dhcpv6AddOption(message, &length, DHCPV6_OPTION_CLIENTID,
      context->clientId, context->clientIdLength);

   //Request, Renew, Release or Decline message?
   if(type == DHCPV6_MSG_TYPE_REQUEST ||
      type == DHCPV6_MSG_TYPE_RENEW ||
      type == DHCPV6_MSG_TYPE_RELEASE ||
      type == DHCPV6_MSG_TYPE_DECLINE)
   {
      //The client places the identifier of the destination
      //server in a Server Identifier option
      dhcpv6AddOption(message, &length, DHCPV6_OPTION_SERVERID,
         context->serverId, context->serverIdLength);
   }

   //Solicit message?
   if(type == DHCPV6_MSG_TYPE_SOLICIT)
   {
      //Check whether rapid commit is enabled
      if(context->settings.rapidCommit)
      {
         //Include the Rapid Commit option if the client is prepared
         //to perform the Solicit/Reply message exchange
         dhcpv6AddOption(message, &length, DHCPV6_OPTION_RAPID_COMMIT, NULL, 0);
      }
   }

   //Solicit, Request, Confirm, Renew or Rebind message?
   if(type == DHCPV6_MSG_TYPE_SOLICIT ||
      type == DHCPV6_MSG_TYPE_REQUEST ||
      type == DHCPV6_MSG_TYPE_CONFIRM ||
      type == DHCPV6_MSG_TYPE_RENEW ||
      type == DHCPV6_MSG_TYPE_REBIND)
   {
      //Point to the client's fully qualified domain name
      fqdnOption = (Dhcpv6FqdnOption *) context->clientFqdn;

      //The FQDN option can be used by the client to convey its
      //fully qualified domain name to the server
      dhcpv6AddOption(message, &length, DHCPV6_OPTION_FQDN,
         fqdnOption, sizeof(Dhcpv6FqdnOption) + context->clientFqdnLength);

      //The client should include an Option Request option to indicate
      //the options the client is interested in receiving
      dhcpv6AddOption(message, &length, DHCPV6_OPTION_ORO,
         &dhcpv6OptionList, sizeof(dhcpv6OptionList));
   }

   //Prepare an IA_NA option for a the current interface
   iaNaOption.iaId = htonl(interface->id);

   //Solicit, Request or Confirm message?
   if(type == DHCPV6_MSG_TYPE_SOLICIT ||
      type == DHCPV6_MSG_TYPE_REQUEST ||
      type == DHCPV6_MSG_TYPE_CONFIRM)
   {
      //The client should set the T1 and T2 fields in any IA_NA options to 0
      iaNaOption.t1 = 0;
      iaNaOption.t2 = 0;
   }
   else
   {
      //T1 and T2 are provided as a hint
      iaNaOption.t1 = htonl(context->ia.t1);
      iaNaOption.t2 = htonl(context->ia.t2);
   }

   //The client includes IA options for any IAs to which
   //it wants the server to assign addresses
   option = dhcpv6AddOption(message, &length, DHCPV6_OPTION_IA_NA,
      &iaNaOption, sizeof(Dhcpv6IaNaOption));

   //Request, Confirm, Renew, Rebind, Release or Decline message?
   if(type == DHCPV6_MSG_TYPE_REQUEST ||
      type == DHCPV6_MSG_TYPE_CONFIRM ||
      type == DHCPV6_MSG_TYPE_RENEW ||
      type == DHCPV6_MSG_TYPE_REBIND ||
      type == DHCPV6_MSG_TYPE_RELEASE ||
      type == DHCPV6_MSG_TYPE_DECLINE)
   {
      //Loop through the IPv6 addresses recorded by the client
      for(i = 0; i < DHCPV6_CLIENT_ADDR_LIST_SIZE; i++)
      {
         //Point to the current entry
         entry = &context->ia.addrList[i];

         //Valid IPv6 address?
         if(entry->validLifetime > 0)
         {
            //Prepare an IA Address option
            iaAddrOption.address = entry->addr;

            //Confirm message?
            if(type == DHCPV6_MSG_TYPE_CONFIRM)
            {
               //The client should set the preferred and valid lifetimes to 0
               iaAddrOption.preferredLifetime = 0;
               iaAddrOption.validLifetime = 0;
            }
            else
            {
               //Preferred and valid lifetimes are provided as a hint
               iaAddrOption.preferredLifetime = htonl(entry->preferredLifetime);
               iaAddrOption.validLifetime = htonl(entry->validLifetime);
            }

            //Add the IA Address option
            dhcpv6AddSubOption(option, &length, DHCPV6_OPTION_IAADDR,
               &iaAddrOption, sizeof(iaAddrOption));
         }
      }
   }

   //Compute the time elapsed since the client sent the first message
   elapsedTimeOption.value = dhcpv6ClientComputeElapsedTime(context);

   //The client must include an Elapsed Time option in messages to indicate
   //how long the client has been trying to complete a DHCP message exchange
   dhcpv6AddOption(message, &length, DHCPV6_OPTION_ELAPSED_TIME,
      &elapsedTimeOption, sizeof(Dhcpv6ElapsedTimeOption));

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + length);

   //Destination address
   destIpAddr.length = sizeof(Ipv6Addr);
   destIpAddr.ipv6Addr = DHCPV6_ALL_RELAY_AGENTS_AND_SERVERS_ADDR;

   //Debug message
   TRACE_DEBUG("\r\n%s: Sending DHCPv6 message (%" PRIuSIZE " bytes)...\r\n",
      formatSystemTime(osGetSystemTime(), NULL), length);

   //Dump the contents of the message for debugging purpose
   dhcpv6DumpMessage(message, length);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //Send DHCPv6 message
   error = udpSendBuffer(interface, NULL, DHCPV6_CLIENT_PORT, &destIpAddr,
      DHCPV6_SERVER_PORT, buffer, offset, &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Process incoming DHCPv6 message
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @param[in] udpHeader UDP header
 * @param[in] buffer Multi-part buffer containing the incoming DHCPv6 message
 * @param[in] offset Offset to the first byte of the DHCPv6 message
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @param[in] param Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientProcessMessage(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *udpHeader,
   const NetBuffer *buffer, size_t offset, const NetRxAncillary *ancillary,
   void *param)
{
   size_t length;
   Dhcpv6ClientContext *context;
   Dhcpv6Message *message;

   //Point to the DHCPv6 client context
   context = (Dhcpv6ClientContext *) param;

   //Retrieve the length of the DHCPv6 message
   length = netBufferGetLength(buffer) - offset;

   //Make sure the DHCPv6 message is valid
   if(length < sizeof(Dhcpv6Message))
      return;

   //Point to the beginning of the DHCPv6 message
   message = netBufferAt(buffer, offset);
   //Sanity check
   if(message == NULL)
      return;

   //Debug message
   TRACE_DEBUG("\r\n%s: DHCPv6 message received (%" PRIuSIZE " bytes)...\r\n",
      formatSystemTime(osGetSystemTime(), NULL), length);

   //Dump the contents of the message for debugging purpose
   dhcpv6DumpMessage(message, length);

   //Check message type
   switch(message->msgType)
   {
   case DHCPV6_MSG_TYPE_ADVERTISE:
      //Parse Advertise message
      dhcpv6ClientParseAdvertise(context, message, length);
      break;
   case DHCPV6_MSG_TYPE_REPLY:
      //Parse Reply message
      dhcpv6ClientParseReply(context, message, length);
      break;
   default:
      //Silently drop incoming message
      break;
   }
}


/**
 * @brief Parse Advertise message
 * @param[in] context Pointer to the DHCPv6 client context
 * @param[in] message Pointer to the incoming message to parse
 * @param[in] length Length of the incoming message
 **/

void dhcpv6ClientParseAdvertise(Dhcpv6ClientContext *context,
   const Dhcpv6Message *message, size_t length)
{
   uint_t i;
   int_t serverPreference;
   NetInterface *interface;
   Dhcpv6StatusCode status;
   Dhcpv6Option *option;
   Dhcpv6Option *serverIdOption;
   Dhcpv6IaNaOption *iaNaOption;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Make sure that the Advertise message is received in response to
   //a Solicit message
   if(context->state != DHCPV6_STATE_SOLICIT)
      return;

   //Discard any received packet that does not match the transaction ID
   if(LOAD24BE(message->transactionId) != context->transactionId)
      return;

   //Get the length of the Options field
   length -= sizeof(Dhcpv6Message);

   //Search for the Client Identifier option
   option = dhcpv6GetOption(message->options, length, DHCPV6_OPTION_CLIENTID);

   //Discard any received packet that does not include a Client Identifier option
   if(option == NULL)
      return;
   //Check the length of the option
   if(ntohs(option->length) != context->clientIdLength)
      return;
   //Check whether the Client Identifier matches our identifier
   if(osMemcmp(option->value, context->clientId, context->clientIdLength))
      return;

   //Search for the Server Identifier option
   serverIdOption = dhcpv6GetOption(message->options, length, DHCPV6_OPTION_SERVERID);

   //Discard any received packet that does not include a Server Identifier option
   if(serverIdOption == NULL)
      return;
   //Check the length of the server DUID
   if(ntohs(serverIdOption->length) == 0)
      return;
   if(ntohs(serverIdOption->length) > DHCPV6_MAX_DUID_SIZE)
      return;

   //Get the status code returned by the server
   status = dhcpv6GetStatusCode(message->options, length);

   //If the message contains a Status Code option indicating a failure,
   //then the Advertise message is discarded by the client
   if(status != DHCPV6_STATUS_SUCCESS)
      return;

   //Search for the Preference option
   option = dhcpv6GetOption(message->options, length, DHCPV6_OPTION_PREFERENCE);

   //Check whether the option has been found
   if(option != NULL && ntohs(option->length) == sizeof(Dhcpv6PreferenceOption))
   {
      //Server server preference value
      serverPreference = option->value[0];
   }
   else
   {
      //Any Advertise that does not include a Preference option
      //is considered to have a preference value of 0
      serverPreference = 0;
   }

   //Select the Advertise message that offers the highest server preference value
   if(serverPreference > context->serverPreference)
   {
      //Save the length of the DUID
      context->serverIdLength = ntohs(serverIdOption->length);
      //Record the server DUID
      osMemcpy(context->serverId, serverIdOption->value, context->serverIdLength);

      //Flush the list of IPv6 addresses from the client's IA
      dhcpv6ClientFlushAddrList(context);
   }

   //Point to the first option
   i = 0;

   //Loop through DHCPv6 options
   while(i < length)
   {
      //Search for an IA_NA option
      option = dhcpv6GetOption(message->options + i, length - i, DHCPV6_OPTION_IA_NA);

      //Unable to find the specified option?
      if(option == NULL)
         break;

      //Make sure the IA_NA option is valid
      if(ntohs(option->length) >= sizeof(Dhcpv6IaNaOption))
      {
         //Get the parameters associated with the IA_NA
         iaNaOption = (Dhcpv6IaNaOption *) option->value;

         //Check the IA identifier
         if(ntohl(iaNaOption->iaId) == interface->id)
         {
            //The client examines the status code in each IA individually
            status = dhcpv6GetStatusCode(iaNaOption->options,
               ntohs(option->length) - sizeof(Dhcpv6IaNaOption));

            //The client must ignore any Advertise message that includes a Status
            //Code option containing the value NoAddrsAvail
            if(status == DHCPV6_STATUS_NO_ADDRS_AVAILABLE)
               return;
         }

         //Check the server preference value
         if(serverPreference > context->serverPreference)
         {
            //Parse the contents of the IA_NA option
            dhcpv6ClientParseIaNaOption(context, option);
         }
      }

      //Jump to the next option
      i += sizeof(Dhcpv6Option) + ntohs(option->length);
   }

   //Record the highest server preference value
   if(serverPreference > context->serverPreference)
      context->serverPreference = serverPreference;

   //If the client receives an Advertise message that includes a
   //Preference option with a preference value of 255, the client
   //immediately completes the message exchange
   if(serverPreference == DHCPV6_MAX_SERVER_PREFERENCE)
   {
      //Continue configuration procedure
      dhcpv6ClientChangeState(context, DHCPV6_STATE_REQUEST, 0);
   }
   //The message exchange is not terminated before the first RT has elapsed
   else if(context->retransmitCount > 1)
   {
      //Continue configuration procedure
      dhcpv6ClientChangeState(context, DHCPV6_STATE_REQUEST, 0);
   }
}


/**
 * @brief Parse Reply message
 * @param[in] context Pointer to the DHCPv6 client context
 * @param[in] message Pointer to the incoming message to parse
 * @param[in] length Length of the incoming message
 **/

void dhcpv6ClientParseReply(Dhcpv6ClientContext *context,
   const Dhcpv6Message *message, size_t length)
{
   error_t error;
   uint_t i;
   uint_t k;
   uint_t n;
   bool_t iaNaOptionFound;
   systime_t minPreferredLifetime;
   NetInterface *interface;
   Dhcpv6StatusCode status;
   Dhcpv6Option *option;
   Dhcpv6Option *serverIdOption;
   Dhcpv6ClientAddrEntry *entry;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Discard any received packet that does not match the transaction ID
   if(LOAD24BE(message->transactionId) != context->transactionId)
      return;

   //Get the length of the Options field
   length -= sizeof(Dhcpv6Message);

   //Search for the Client Identifier option
   option = dhcpv6GetOption(message->options, length, DHCPV6_OPTION_CLIENTID);

   //Discard any received packet that does not include a Client Identifier option
   if(option == NULL)
      return;
   //Check the length of the option
   if(ntohs(option->length) != context->clientIdLength)
      return;
   //Check whether the Client Identifier matches our identifier
   if(osMemcmp(option->value, context->clientId, context->clientIdLength))
      return;

   //Search for the Server Identifier option
   serverIdOption = dhcpv6GetOption(message->options, length, DHCPV6_OPTION_SERVERID);

   //Discard any received packet that does not include a Server Identifier option
   if(serverIdOption == NULL)
      return;
   //Check the length of the server DUID
   if(ntohs(serverIdOption->length) == 0)
      return;
   if(ntohs(serverIdOption->length) > DHCPV6_MAX_DUID_SIZE)
      return;

   //Get the status code returned by the server
   status = dhcpv6GetStatusCode(message->options, length);

   //Check current state
   if(context->state == DHCPV6_STATE_SOLICIT)
   {
      //A Reply message is not acceptable when rapid commit is disallowed
      if(!context->settings.rapidCommit)
         return;

      //Search for the Rapid Commit option
      option = dhcpv6GetOption(message->options, length, DHCPV6_OPTION_RAPID_COMMIT);

      //The client discards any message that does not include a Rapid Commit option
      if(option == NULL || ntohs(option->length) != 0)
         return;
   }
   else if(context->state == DHCPV6_STATE_REQUEST)
   {
      //The client must discard the Reply message if the contents of the
      //Server Identifier option do not match the server's DUID
      if(!dhcpv6ClientCheckServerId(context, serverIdOption))
         return;
   }
   else if(context->state == DHCPV6_STATE_CONFIRM)
   {
      //When the client receives a NotOnLink status from the server in response
      //to a Confirm message, the client performs DHCP server solicitation
      if(status == DHCPV6_STATUS_NOT_ON_LINK)
      {
         //Restart the DHCP server discovery process
         dhcpv6ClientChangeState(context, DHCPV6_STATE_INIT, 0);

         //Exit immediately
         return;
      }
   }
   else if(context->state == DHCPV6_STATE_RENEW)
   {
      //The client must discard the Reply message if the contents of the
      //Server Identifier option do not match the server's DUID
      if(!dhcpv6ClientCheckServerId(context, serverIdOption))
         return;
   }
   else if(context->state == DHCPV6_STATE_REBIND)
   {
      //Do not check the server's DUID when the Reply message is
      //received in response to a Rebind message
   }
   else if(context->state == DHCPV6_STATE_RELEASE)
   {
      //The client must discard the Reply message if the contents of the
      //Server Identifier option do not match the server's DUID
      if(!dhcpv6ClientCheckServerId(context, serverIdOption))
         return;

      //When the client receives a valid Reply message in response to a
      //Release message, the client considers the Release event completed,
      //regardless of the Status Code option(s) returned by the server
      context->running = FALSE;

      //Reinitialize state machine
      dhcpv6ClientChangeState(context, DHCPV6_STATE_INIT, 0);

      //Exit immediately
      return;
   }
   else if(context->state == DHCPV6_STATE_DECLINE)
   {
      //The client must discard the Reply message if the contents of the
      //Server Identifier option do not match the server's DUID
      if(!dhcpv6ClientCheckServerId(context, serverIdOption))
         return;

      //When the client receives a valid Reply message in response to a
      //Decline message, the client considers the Decline event completed,
      //regardless of the Status Code option returned by the server
      dhcpv6ClientChangeState(context, DHCPV6_STATE_INIT, 0);

      //Exit immediately
      return;
   }
   else
   {
      //Silently discard the Reply message
      return;
   }

   //Check status code
   if(status == DHCPV6_STATUS_USE_MULTICAST)
   {
      //When the client receives a Reply message with a Status Code option
      //with the value UseMulticast, the client records the receipt of the
      //message and sends subsequent messages to the server through the
      //interface on which the message was received using multicast
      return;
   }
   else if(status == DHCPV6_STATUS_UNSPEC_FAILURE)
   {
      //If the client receives a Reply message with a Status Code containing
      //UnspecFail, the server is indicating that it was unable to process
      //the message due to an unspecified failure condition
      return;
   }

   //This flag will be set if a valid IA_NA option is found
   iaNaOptionFound = FALSE;
   //Point to the first option
   i = 0;

   //Loop through DHCPv6 options
   while(i < length)
   {
      //Search for an IA_NA option
      option = dhcpv6GetOption(message->options + i, length - i, DHCPV6_OPTION_IA_NA);

      //Unable to find the specified option?
      if(option == NULL)
         break;

      //Parse the contents of the IA_NA option
      error = dhcpv6ClientParseIaNaOption(context, option);

      //Check error code
      if(error == NO_ERROR)
      {
         //A valid IA_NA option has been found
         iaNaOptionFound = TRUE;
      }
      else if(error == ERROR_NOT_ON_LINK)
      {
         //When the client receives a NotOnLink status from the server
         //in response to a Request, the client can either re-issue the
         //Request without specifying any addresses or restart the DHCP
         //server discovery process
         dhcpv6ClientChangeState(context, DHCPV6_STATE_INIT, 0);

         //Exit immediately
         return;
      }
      else if(error == ERROR_NO_BINDING)
      {
         //When the client receives a Reply message in response to a Renew
         //or Rebind message, the client sends a Request message if any of
         //the IAs in the Reply message contains the NoBinding status code
         dhcpv6ClientChangeState(context, DHCPV6_STATE_REQUEST, 0);

         //Exit immediately
         return;
      }
      else
      {
         //If an invalid option is received, the client discards
         //the option and process the rest of the message...
      }

      //Jump to the next option
      i += sizeof(Dhcpv6Option) + ntohs(option->length);
   }

   //No usable addresses in any of the IAs?
   if(!iaNaOptionFound)
   {
      //Check whether the client receives a Reply message in response
      //to a Renew or Rebind message
      if(context->state == DHCPV6_STATE_RENEW ||
         context->state == DHCPV6_STATE_REBIND)
      {
         //The client sends a Renew/Rebind if the IA is not in the Reply message
      }
      else
      {
         //If the client finds no usable addresses in any of the IAs, it may try
         //another server (perhaps restarting the DHCP server discovery process)
         dhcpv6ClientChangeState(context, DHCPV6_STATE_INIT, 0);
      }

      //Exit immediately
      return;
   }

   //Total number of valid IPv6 in the IA
   n = 0;
   //Number of new IPv6 addresses in the IA
   k = 0;
   //Minimum preferred lifetime observed in the IA
   minPreferredLifetime = 0;

   //Loop through the IPv6 addresses recorded by the DHCPv6 client
   for(i = 0; i < DHCPV6_CLIENT_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->ia.addrList[i];

      //Valid IPv6 address?
      if(entry->validLifetime > 0)
      {
         //Total number of valid IPv6 in the IA
         n++;

         //Save the minimum preferred lifetime that has been observed so far
         if(minPreferredLifetime < entry->preferredLifetime)
            minPreferredLifetime = entry->preferredLifetime;

         //Update lifetimes of the current IPv6 address
         ipv6AddAddr(interface, &entry->addr, entry->validLifetime,
            entry->preferredLifetime);

         //New IPv6 address added?
         if(ipv6GetAddrState(interface, &entry->addr) == IPV6_ADDR_STATE_TENTATIVE)
            k++;
      }
   }

   //Make sure that the IA contains at least one IPv6 address
   if(n > 0)
   {
      //Save the length of the DUID
      context->serverIdLength = ntohs(serverIdOption->length);
      //Record the server DUID
      osMemcpy(context->serverId, serverIdOption->value, context->serverIdLength);

      //Save the time a which the lease was obtained
      context->leaseStartTime = osGetSystemTime();

      //Check the value of T1
      if(context->ia.t1 == 0)
      {
         //If T1 is set to 0 by the server, the client may send
         //a Renew message at the client's discretion
         if(minPreferredLifetime == DHCPV6_INFINITE_TIME)
            context->ia.t1 = DHCPV6_INFINITE_TIME;
         else
            context->ia.t1 = minPreferredLifetime / 2;
      }

      //Check the value of T2
      if(context->ia.t2 == 0)
      {
         //If T2 is set to 0 by the server, the client may send
         //a Rebind message at the client's discretion
         if(context->ia.t1 == DHCPV6_INFINITE_TIME)
            context->ia.t2 = DHCPV6_INFINITE_TIME;
         else
            context->ia.t2 = context->ia.t1 + context->ia.t1 / 2;
      }

      //Any addresses added in the IA?
      if(k > 0)
      {
         //Perform Duplicate Address Detection for the new IPv6 addresses
         dhcpv6ClientChangeState(context, DHCPV6_STATE_DAD, 0);
      }
      else
      {
         //Switch to the BOUND state
         dhcpv6ClientChangeState(context, DHCPV6_STATE_BOUND, 0);
      }
   }
   else
   {
      //If the client finds no usable addresses in any of the IAs, it may try
      //another server (perhaps restarting the DHCP server discovery process)
      dhcpv6ClientChangeState(context, DHCPV6_STATE_INIT, 0);
   }
}


/**
 * @brief Parse IA_NA option
 * @param[in] context Pointer to the DHCPv6 client context
 * @param[in] option Pointer to the IA_NA option to parse
 * @return Error code
 **/

error_t dhcpv6ClientParseIaNaOption(Dhcpv6ClientContext *context,
   const Dhcpv6Option *option)
{
   error_t error;
   uint_t n;
   size_t i;
   size_t length;
   NetInterface *interface;
   Dhcpv6StatusCode status;
   Dhcpv6IaNaOption *iaNaOption;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Number of addresses found in the IA_NA option
   n = 0;

   //Make sure the IA_NA option is valid
   if(ntohs(option->length) < sizeof(Dhcpv6IaNaOption))
      return ERROR_INVALID_LENGTH;

   //Get the parameters associated with the IA_NA
   iaNaOption = (Dhcpv6IaNaOption *) option->value;
   //Compute the length of IA_NA Options field
   length = ntohs(option->length) - sizeof(Dhcpv6IaNaOption);

   //Check the IA identifier
   if(ntohl(iaNaOption->iaId) != interface->id)
      return ERROR_WRONG_IDENTIFIER;

   //If a client receives an IA_NA with T1 greater than T2, and both T1
   //and T2 are greater than 0, the client discards the IA_NA option and
   //processes the remainder of the message as though the server had not
   //included the invalid IA_NA option
   if(ntohl(iaNaOption->t1) > ntohl(iaNaOption->t2) && ntohl(iaNaOption->t2) > 0)
      return ERROR_INVALID_PARAMETER;

   //The client examines the status code in each IA individually
   status = dhcpv6GetStatusCode(iaNaOption->options, length);

   //Check error code
   if(status == DHCPV6_STATUS_NO_ADDRS_AVAILABLE)
   {
      //The client has received no usable address in the IA
      return ERROR_NO_ADDRESS;
   }
   else if(status == DHCPV6_STATUS_NO_BINDING)
   {
      //Client record (binding) unavailable
      return ERROR_NO_BINDING;
   }
   else if(status == DHCPV6_STATUS_NOT_ON_LINK)
   {
      //The prefix for the address is not appropriate for the link to which the
      //client is attached
      return ERROR_NOT_ON_LINK;
   }
   else if(status != DHCPV6_STATUS_SUCCESS)
   {
      //Failure, reason unspecified
      return ERROR_FAILURE;
   }

   //Record T1 and T2 times
   context->ia.t1 = ntohl(iaNaOption->t1);
   context->ia.t2 = ntohl(iaNaOption->t2);

   //Point to the first option
   i = 0;

   //Loop through IA_NA options
   while(i < length)
   {
      //Search for an IA Address option
      option = dhcpv6GetOption(iaNaOption->options + i, length - i, DHCPV6_OPTION_IAADDR);

      //Unable to find the specified option?
      if(option == NULL)
         break;

      //Parse the contents of the IA Address option
      error = dhcpv6ClientParseIaAddrOption(context, option);

      //Check status code
      if(!error)
      {
         //Increment the number of addresses found in the IA_NA option
         n++;
      }

      //Jump to the next option
      i += sizeof(Dhcpv6Option) + ntohs(option->length);
   }

   //No usable addresses in the IA_NA option?
   if(n == 0)
   {
      //Report an error
      return ERROR_NO_ADDRESS;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse IA Address option
 * @param[in] context Pointer to the DHCPv6 client context
 * @param[in] option Pointer to the IA Address option to parse
 * @return Error code
 **/

error_t dhcpv6ClientParseIaAddrOption(Dhcpv6ClientContext *context,
   const Dhcpv6Option *option)
{
   size_t length;
   uint32_t validLifetime;
   uint32_t preferredLifetime;
   Dhcpv6StatusCode status;
   Dhcpv6IaAddrOption *iaAddrOption;

   //Make sure the IA Address option is valid
   if(ntohs(option->length) < sizeof(Dhcpv6IaAddrOption))
      return ERROR_INVALID_LENGTH;

   //Point to the contents of the IA Address option
   iaAddrOption = (Dhcpv6IaAddrOption *) option->value;
   //Compute the length of IA Address Options field
   length = ntohs(option->length) - sizeof(Dhcpv6IaAddrOption);

   //Convert lifetimes to host byte order
   validLifetime = ntohl(iaAddrOption->validLifetime);
   preferredLifetime = ntohl(iaAddrOption->preferredLifetime);

   //A client discards any addresses for which the preferred lifetime
   //is greater than the valid lifetime
   if(preferredLifetime > validLifetime)
      return ERROR_INVALID_PARAMETER;

   //The client examines the status code in each IA Address
   status = dhcpv6GetStatusCode(iaAddrOption->options, length);

   //Any error to report?
   if(status != DHCPV6_STATUS_SUCCESS)
      return ERROR_FAILURE;

   //Check the value of the Valid Lifetime
   if(iaAddrOption->validLifetime > 0)
   {
      //Add any new addresses in the IA option to the IA as recorded
      //by the client
      dhcpv6ClientAddAddr(context, &iaAddrOption->address,
         validLifetime, preferredLifetime);
   }
   else
   {
      //Discard any addresses from the IA, as recorded by the client,
      //that have a valid lifetime of 0 in the IA Address option
      dhcpv6ClientRemoveAddr(context, &iaAddrOption->address);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add an IPv6 address to the IA
 * @param[in] context Pointer to the DHCPv6 client context
 * @param[in] addr IPv6 address to be added
 * @param[in] validLifetime Valid lifetime, in seconds
 * @param[in] preferredLifetime Preferred lifetime, in seconds
 **/

void dhcpv6ClientAddAddr(Dhcpv6ClientContext *context, const Ipv6Addr *addr,
   uint32_t validLifetime, uint32_t preferredLifetime)
{
   uint_t i;
   Dhcpv6ClientAddrEntry *entry;
   Dhcpv6ClientAddrEntry *firstFreeEntry;

   //Keep track of the first free entry
   firstFreeEntry = NULL;

   //Loop through the IPv6 addresses recorded by the DHCPv6 client
   for(i = 0; i < DHCPV6_CLIENT_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->ia.addrList[i];

      //Valid IPv6 address?
      if(entry->validLifetime > 0)
      {
         //Check whether the current entry matches the specified address
         if(ipv6CompAddr(&entry->addr, addr))
            break;
      }
      else
      {
         //Keep track of the first free entry
         if(firstFreeEntry == NULL)
            firstFreeEntry = entry;
      }
   }

   //No matching entry found?
   if(i >= IPV6_PREFIX_LIST_SIZE)
      entry = firstFreeEntry;

   //Update the entry if necessary
   if(entry != NULL)
   {
      //Save IPv6 address
      entry->addr = *addr;

      //Save lifetimes
      entry->validLifetime = validLifetime;
      entry->preferredLifetime = preferredLifetime;
   }
}


/**
 * @brief Remove an IPv6 address from the IA
 * @param[in] context Pointer to the DHCPv6 client context
 * @param[in] addr IPv6 address to be removed
 **/

void dhcpv6ClientRemoveAddr(Dhcpv6ClientContext *context, const Ipv6Addr *addr)
{
   uint_t i;
   NetInterface *interface;
   Dhcpv6ClientAddrEntry *entry;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Loop through the IPv6 addresses recorded by the DHCPv6 client
   for(i = 0; i < DHCPV6_CLIENT_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->ia.addrList[i];

      //Valid IPv6 address?
      if(entry->validLifetime > 0)
      {
         //Check whether the current entry matches the specified address
         if(ipv6CompAddr(&entry->addr, addr))
         {
            //The IPv6 address is no more valid and should be removed from
            //the list of IPv6 addresses assigned to the interface
            ipv6RemoveAddr(interface, addr);

            //Remove the IPv6 address from the IA
            entry->validLifetime = 0;
         }
      }
   }
}


/**
 * @brief Flush the list of IPv6 addresses from the IA
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientFlushAddrList(Dhcpv6ClientContext *context)
{
   uint_t i;
   NetInterface *interface;
   Dhcpv6ClientAddrEntry *entry;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Loop through the IPv6 addresses recorded by the DHCPv6 client
   for(i = 0; i < DHCPV6_CLIENT_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->ia.addrList[i];

      //Valid IPv6 address?
      if(entry->validLifetime > 0)
      {
         //The IPv6 address is no more valid and should be removed from
         //the list of IPv6 addresses assigned to the interface
         ipv6RemoveAddr(interface, &entry->addr);

         //Remove the IPv6 address from the IA
         entry->validLifetime = 0;
      }
   }
}


/**
 * @brief Generate client's DUID
 * @param[in] context Pointer to the DHCPv6 client context
 * @return Error code
 **/

error_t dhcpv6ClientGenerateDuid(Dhcpv6ClientContext *context)
{
   NetInterface *interface;
   Dhcpv6DuidLl *duid;
#if (ETH_SUPPORT == ENABLED)
   NetInterface *logicalInterface;
#endif

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Point to the buffer where to format the client's DUID
   duid = (Dhcpv6DuidLl *) context->clientId;

#if (ETH_SUPPORT == ENABLED)
   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Generate a DUID-LL from the MAC address
   duid->type = HTONS(DHCPV6_DUID_LL);
   duid->hardwareType = HTONS(DHCPV6_HARDWARE_TYPE_ETH);
   duid->linkLayerAddr = logicalInterface->macAddr;
#else
   //Generate a DUID-LL from the EUI-64 identifier
   duid->type = HTONS(DHCPV6_DUID_LL);
   duid->hardwareType = HTONS(DHCPV6_HARDWARE_TYPE_EUI64);
   duid->linkLayerAddr = interface->eui64;
#endif

   //Length of the newly generated DUID
   context->clientIdLength = sizeof(Dhcpv6DuidLl);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Generate client's fully qualified domain name
 * @param[in] context Pointer to the DHCPv6 client context
 * @return Error code
 **/

error_t dhcpv6ClientGenerateFqdn(Dhcpv6ClientContext *context)
{
   NetInterface *interface;
   Dhcpv6FqdnOption *fqdnOption;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Point to the buffer where to format the client's FQDN
   fqdnOption = (Dhcpv6FqdnOption *) context->clientFqdn;

   //Set flags
   fqdnOption->mbz = 0;
   fqdnOption->n = FALSE;
   fqdnOption->o = FALSE;
   fqdnOption->s = FALSE;

   //Encode client's FQDN
   context->clientFqdnLength = dnsEncodeName(interface->hostname,
      fqdnOption->domainName);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Generate a link-local address
 * @param[in] context Pointer to the DHCPv6 client context
 * @return Error code
 **/

error_t dhcpv6ClientGenerateLinkLocalAddr(Dhcpv6ClientContext *context)
{
   error_t error;
   NetInterface *interface;
   Ipv6Addr addr;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Check whether a link-local address has been manually assigned
   if(interface->ipv6Context.addrList[0].state != IPV6_ADDR_STATE_INVALID &&
      interface->ipv6Context.addrList[0].permanent)
   {
      //Keep using the current link-local address
      error = NO_ERROR;
   }
   else
   {
      //A link-local address is formed by combining the well-known
      //link-local prefix fe80::/10 with the interface identifier
      ipv6GenerateLinkLocalAddr(&interface->eui64, &addr);

#if (NDP_SUPPORT == ENABLED)
      //Check whether Duplicate Address Detection should be performed
      if(interface->ndpContext.dupAddrDetectTransmits > 0)
      {
         //Use the link-local address as a tentative address
         error = ipv6SetAddr(interface, 0, &addr, IPV6_ADDR_STATE_TENTATIVE,
            NDP_INFINITE_LIFETIME, NDP_INFINITE_LIFETIME, FALSE);
      }
      else
#endif
      {
         //The use of the link-local address is now unrestricted
         error = ipv6SetAddr(interface, 0, &addr, IPV6_ADDR_STATE_PREFERRED,
            NDP_INFINITE_LIFETIME, NDP_INFINITE_LIFETIME, FALSE);
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Check the Server Identifier option
 * @param[in] context Pointer to the DHCPv6 client context
 * @param[in] serverIdOption Pointer to the Server Identifier option
 * @return TRUE if the option matches the server's DUID, else FALSE
 **/

bool_t dhcpv6ClientCheckServerId(Dhcpv6ClientContext *context,
   Dhcpv6Option *serverIdOption)
{
   bool_t valid = FALSE;

   //Check the length of the Server Identifier option
   if(ntohs(serverIdOption->length) == context->serverIdLength)
   {
      //Check whether the Server Identifier option matches the server's DUID
      if(!osMemcmp(serverIdOption->value, context->serverId, context->serverIdLength))
         valid = TRUE;
   }

   //Return TRUE if the option matches the server's DUID
   return valid;
}


/**
 * @brief Manage DHCPv6 configuration timeout
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientCheckTimeout(Dhcpv6ClientContext *context)
{
   systime_t time;
   NetInterface *interface;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Get current time
   time = osGetSystemTime();

   //Any registered callback?
   if(context->settings.timeoutEvent != NULL)
   {
      //DHCPv6 configuration timeout?
      if(timeCompare(time, context->configStartTime + context->settings.timeout) >= 0)
      {
         //Ensure the callback function is only called once
         if(!context->timeoutEventDone)
         {
            //Release exclusive access
            osReleaseMutex(&netMutex);
            //Invoke user callback function
            context->settings.timeoutEvent(context, interface);
            //Get exclusive access
            osAcquireMutex(&netMutex);

            //Set flag
            context->timeoutEventDone = TRUE;
         }
      }
   }
}


/**
 * @brief Compute the time elapsed since the client sent the first message
 * @param[in] context Pointer to the DHCPv6 client context
 * @return The elapsed time expressed in hundredths of a second
 **/

uint16_t dhcpv6ClientComputeElapsedTime(Dhcpv6ClientContext *context)
{
   systime_t time;

   //Check retransmission counter
   if(context->retransmitCount == 0)
   {
      //The elapsed time must be 0 for the first message
      time = 0;
   }
   else
   {
      //Compute the time elapsed since the client sent the
      //first message (in hundredths of a second)
      time = (osGetSystemTime() - context->exchangeStartTime) / 10;

      //The value 0xFFFF is used to represent any elapsed time values
      //greater than the largest time value that can be represented
      time = MIN(time, 0xFFFF);
   }

   //Convert the 16-bit value to network byte order
   return htons(time);
}


/**
 * @brief Update DHCPv6 FSM state
 * @param[in] context Pointer to the DHCPv6 client context
 * @param[in] newState New DHCPv6 state to switch to
 * @param[in] delay Initial delay
 **/

void dhcpv6ClientChangeState(Dhcpv6ClientContext *context,
   Dhcpv6State newState, systime_t delay)
{
   systime_t time;

   //Get current time
   time = osGetSystemTime();

#if (DHCPV6_TRACE_LEVEL >= TRACE_LEVEL_INFO)
   //Sanity check
   if(newState <= DHCPV6_STATE_DECLINE)
   {
      //DHCPv6 FSM states
      static const char_t *stateLabel[] =
      {
         "INIT",
         "SOLICIT",
         "REQUEST",
         "INIT-CONFIRM",
         "CONFIRM",
         "DAD",
         "BOUND",
         "RENEW",
         "REBIND",
         "RELEASE",
         "DECLINE"
      };

      //Debug message
      TRACE_INFO("%s: DHCPv6 client %s state\r\n",
         formatSystemTime(time, NULL), stateLabel[newState]);
   }
#endif

   //Set time stamp
   context->timestamp = time;
   //Set initial delay
   context->timeout = delay;
   //Reset retransmission counter
   context->retransmitCount = 0;
   //Switch to the new state
   context->state = newState;

   //Any registered callback?
   if(context->settings.stateChangeEvent != NULL)
   {
      NetInterface *interface;

      //Point to the underlying network interface
      interface = context->settings.interface;

      //Release exclusive access
      osReleaseMutex(&netMutex);
      //Invoke user callback function
      context->settings.stateChangeEvent(context, interface, newState);
      //Get exclusive access
      osAcquireMutex(&netMutex);
   }
}


/**
 * @brief Dump DHCPv6 configuration for debugging purpose
 * @param[in] context Pointer to the DHCPv6 client context
 **/

void dhcpv6ClientDumpConfig(Dhcpv6ClientContext *context)
{
#if (DHCPV6_TRACE_LEVEL >= TRACE_LEVEL_INFO)
   uint_t i;
   NetInterface *interface;
   Ipv6Context *ipv6Context;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Point to the IPv6 context
   ipv6Context = &interface->ipv6Context;

   //Debug message
   TRACE_INFO("\r\n");
   TRACE_INFO("DHCPv6 configuration:\r\n");

   //Lease start time
   TRACE_INFO("  Lease Start Time = %s\r\n",
      formatSystemTime(context->leaseStartTime, NULL));

   //T1 parameter
   TRACE_INFO("  T1 = %" PRIu32 "s\r\n", context->ia.t1);
   //T2 parameter
   TRACE_INFO("  T2 = %" PRIu32 "s\r\n", context->ia.t2);

   //Global addresses
   for(i = 1; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      TRACE_INFO("  Global Address %u = %s\r\n", i,
         ipv6AddrToString(&ipv6Context->addrList[i].addr, NULL));
   }

   //DNS servers
   for(i = 0; i < IPV6_DNS_SERVER_LIST_SIZE; i++)
   {
      TRACE_INFO("  DNS Server %u = %s\r\n", i + 1,
         ipv6AddrToString(&ipv6Context->dnsServerList[i], NULL));
   }

   //Debug message
   TRACE_INFO("\r\n");
#endif
}

#endif
