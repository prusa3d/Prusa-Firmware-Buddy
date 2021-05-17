/**
 * @file dhcp_client.c
 * @brief DHCP client (Dynamic Host Configuration Protocol)
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
 * The Dynamic Host Configuration Protocol is used to provide configuration
 * parameters to hosts. Refer to the following RFCs for complete details:
 * - RFC 2131: Dynamic Host Configuration Protocol
 * - RFC 2132: DHCP Options and BOOTP Vendor Extensions
 * - RFC 4039: Rapid Commit Option for the DHCP version 4
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL DHCP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "dhcp/dhcp_client.h"
#include "dhcp/dhcp_common.h"
#include "dhcp/dhcp_debug.h"
#include "mdns/mdns_responder.h"
#include "date_time.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV4_SUPPORT == ENABLED && DHCP_CLIENT_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t dhcpClientTickCounter;

//Requested DHCP options
const uint8_t dhcpOptionList[] =
{
   DHCP_OPT_SUBNET_MASK,
   DHCP_OPT_ROUTER,
   DHCP_OPT_DNS_SERVER,
   DHCP_OPT_INTERFACE_MTU,
   DHCP_OPT_IP_ADDRESS_LEASE_TIME,
   DHCP_OPT_RENEWAL_TIME_VALUE,
   DHCP_OPT_REBINDING_TIME_VALUE
};


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains DHCP client settings
 **/

void dhcpClientGetDefaultSettings(DhcpClientSettings *settings)
{
   //Use default interface
   settings->interface = netGetDefaultInterface();
   //Index of the IP address to be configured
   settings->ipAddrIndex = 0;

#if (DHCP_CLIENT_HOSTNAME_OPTION_SUPPORT == ENABLED)
   //Use default host name
   osStrcpy(settings->hostname, "");
#endif

#if (DHCP_CLIENT_ID_OPTION_SUPPORT == ENABLED)
   //Use default client identifier
   settings->clientIdLength = 0;
#endif

   //Support for quick configuration using rapid commit
   settings->rapidCommit = FALSE;
   //Use the DNS servers provided by the DHCP server
   settings->manualDnsConfig = FALSE;
   //DHCP configuration timeout
   settings->timeout = 0;
   //DHCP configuration timeout event
   settings->timeoutEvent = NULL;
   //Link state change event
   settings->linkChangeEvent = NULL;
   //FSM state change event
   settings->stateChangeEvent = NULL;
}


/**
 * @brief DHCP client initialization
 * @param[in] context Pointer to the DHCP client context
 * @param[in] settings DHCP client specific settings
 * @return Error code
 **/

error_t dhcpClientInit(DhcpClientContext *context, const DhcpClientSettings *settings)
{
   error_t error;
   size_t n;
   NetInterface *interface;

   //Debug message
   TRACE_INFO("Initializing DHCP client...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //The DHCP client must be bound to a valid interface
   if(settings->interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the underlying network interface
   interface = settings->interface;

   //Clear the DHCP client context
   osMemset(context, 0, sizeof(DhcpClientContext));
   //Save user settings
   context->settings = *settings;

#if (DHCP_CLIENT_HOSTNAME_OPTION_SUPPORT == ENABLED)
   //No DHCP host name defined?
   if(settings->hostname[0] == '\0')
   {
      //Use default host name
      n = osStrlen(interface->hostname);
      //Limit the length of the string
      n = MIN(n, DHCP_CLIENT_MAX_HOSTNAME_LEN);

      //Copy host name
      osStrncpy(context->settings.hostname, interface->hostname, n);
      //Properly terminate the string with a NULL character
      context->settings.hostname[n] = '\0';
   }
#endif

   //Callback function to be called when a DHCP message is received
   error = udpAttachRxCallback(interface, DHCP_CLIENT_PORT,
      dhcpClientProcessMessage, context);
   //Failed to register callback function?
   if(error)
      return error;

   //DHCP client is currently suspended
   context->running = FALSE;
   //Initialize state machine
   context->state = DHCP_STATE_INIT;

   //Attach the DHCP client context to the network interface
   interface->dhcpClientContext = context;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Start DHCP client
 * @param[in] context Pointer to the DHCP client context
 * @return Error code
 **/

error_t dhcpClientStart(DhcpClientContext *context)
{
   uint_t i;
   NetInterface *interface;

   //Make sure the DHCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting DHCP client...\r\n");

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

   //Start DHCP client
   context->running = TRUE;
   //Initialize state machine
   context->state = DHCP_STATE_INIT;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Stop DHCP client
 * @param[in] context Pointer to the DHCP client context
 * @return Error code
 **/

error_t dhcpClientStop(DhcpClientContext *context)
{
   //Make sure the DHCP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Stopping DHCP client...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Stop DHCP client
   context->running = FALSE;
   //Reinitialize state machine
   context->state = DHCP_STATE_INIT;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve current state
 * @param[in] context Pointer to the DHCP client context
 * @return Current DHCP client state
 **/

DhcpState dhcpClientGetState(DhcpClientContext *context)
{
   DhcpState state;

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
 * @brief DHCP client timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * manage DHCP client operation
 *
 * @param[in] context Pointer to the DHCP client context
 **/


void dhcpClientTick(DhcpClientContext *context)
{
   //Make sure the DHCP client has been properly instantiated
   if(context == NULL)
      return;

   //DHCP client finite state machine
   switch(context->state)
   {
   //Process INIT state
   case DHCP_STATE_INIT:
      //This is the initialization state, where a client begins the process of
      //acquiring a lease. It also returns here when a lease ends, or when a
      //lease negotiation fails
      dhcpClientStateInit(context);
      break;

   //Process SELECTING state
   case DHCP_STATE_SELECTING:
      //The client is waiting to receive DHCPOFFER messages from one or more
      //DHCP servers, so it can choose one
      dhcpClientStateSelecting(context);
      break;

   //Process REQUESTING state
   case DHCP_STATE_REQUESTING:
      //The client is waiting to hear back from the server to which
      //it sent its request
      dhcpClientStateRequesting(context);
      break;

   //Process INIT REBOOT state
   case DHCP_STATE_INIT_REBOOT:
      //When a client that already has a valid lease starts up after a
      //power-down or reboot, it starts here instead of the INIT state
      dhcpClientStateInitReboot(context);
      break;

   //Process REBOOTING state
   case DHCP_STATE_REBOOTING:
      //A client that has rebooted with an assigned address is waiting for
      //a confirming reply from a server
      dhcpClientStateRebooting(context);
      break;

   //Process PROBING state
   case DHCP_STATE_PROBING:
      //The client probes the newly received address
      dhcpClientStateProbing(context);
      break;

   //Process BOUND state
   case DHCP_STATE_BOUND:
      //Client has a valid lease and is in its normal operating state
      dhcpClientStateBound(context);
      break;

   //Process RENEWING state
   case DHCP_STATE_RENEWING:
      //Client is trying to renew its lease. It regularly sends DHCPREQUEST messages with
      //the server that gave it its current lease specified, and waits for a reply
      dhcpClientStateRenewing(context);
      break;

   //Process REBINDING state
   case DHCP_STATE_REBINDING:
      //The client has failed to renew its lease with the server that originally granted it,
      //and now seeks a lease extension with any server that can hear it. It periodically sends
      //DHCPREQUEST messages with no server specified until it gets a reply or the lease ends
      dhcpClientStateRebinding(context);
      break;

   //Invalid state
   default:
      //Switch to the default state
      context->state = DHCP_STATE_INIT;
      break;
   }
}


/**
 * @brief Callback function for link change event
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientLinkChangeEvent(DhcpClientContext *context)
{
   uint_t i;
   NetInterface *interface;

   //Make sure the DHCP client has been properly instantiated
   if(context == NULL)
      return;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Index of the IP address in the list of addresses assigned to the interface
   i = context->settings.ipAddrIndex;

   //Check whether the DHCP client is running
   if(context->running)
   {
      //The host address is no longer valid
      interface->ipv4Context.addrList[i].addr = IPV4_UNSPECIFIED_ADDR;
      interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_INVALID;

      //Clear subnet mask
      interface->ipv4Context.addrList[i].subnetMask = IPV4_UNSPECIFIED_ADDR;

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
      //Restart mDNS probing process
      mdnsResponderStartProbing(interface->mdnsResponderContext);
#endif
   }

   //Check whether the client already has a valid lease
   if(context->state >= DHCP_STATE_INIT_REBOOT)
   {
      //Switch to the INIT-REBOOT state
      context->state = DHCP_STATE_INIT_REBOOT;
   }
   else
   {
      //Switch to the INIT state
      context->state = DHCP_STATE_INIT;
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
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientStateInit(DhcpClientContext *context)
{
   systime_t delay;
   NetInterface *interface;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Check whether the DHCP client is running
   if(context->running)
   {
      //Wait for the link to be up before starting DHCP configuration
      if(interface->linkState)
      {
         //The client should wait for a random time to desynchronize
         //the use of DHCP at startup
         delay = netGetRandRange(0, DHCP_CLIENT_INIT_DELAY);

         //Record the time at which the client started the address
         //acquisition process
         context->configStartTime = osGetSystemTime();
         //Clear flag
         context->timeoutEventDone = FALSE;

         //Switch to the SELECTING state
         dhcpClientChangeState(context, DHCP_STATE_SELECTING, delay);
      }
   }
}


/**
 * @brief SELECTING state
 *
 * The client is waiting to receive DHCPOFFER messages from
 * one or more DHCP servers, so it can choose one
 *
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientStateSelecting(DhcpClientContext *context)
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
         //A transaction identifier is used by the client to
         //match incoming DHCP messages with pending requests
         context->transactionId = netGetRand();

         //Send a DHCPDISCOVER message
         dhcpClientSendDiscover(context);

         //Initial timeout value
         context->retransmitTimeout = DHCP_CLIENT_DISCOVER_INIT_RT;
      }
      else
      {
         //Send a DHCPDISCOVER message
         dhcpClientSendDiscover(context);

         //The timeout value is doubled for each subsequent retransmission
         context->retransmitTimeout *= 2;

         //Limit the timeout value to a maximum of 64 seconds
         if(context->retransmitTimeout > DHCP_CLIENT_DISCOVER_MAX_RT)
            context->retransmitTimeout = DHCP_CLIENT_DISCOVER_MAX_RT;
      }

      //Save the time at which the message was sent
      context->timestamp = time;

      //The timeout value should be randomized by the value of a uniform
      //number chosen from the range -1 to +1
      context->timeout = context->retransmitTimeout +
         netGetRandRange(-DHCP_CLIENT_RAND_FACTOR, DHCP_CLIENT_RAND_FACTOR);

      //Increment retransmission counter
      context->retransmitCount++;
   }

   //Manage DHCP configuration timeout
   dhcpClientCheckTimeout(context);
}


/**
 * @brief REQUESTING state
 *
 * The client is waiting to hear back from the server
 * to which it sent its request
 *
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientStateRequesting(DhcpClientContext *context)
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
         //A transaction identifier is used by the client to
         //match incoming DHCP messages with pending requests
         context->transactionId = netGetRand();

         //Send a DHCPREQUEST message
         dhcpClientSendRequest(context);

         //Initial timeout value
         context->retransmitTimeout = DHCP_CLIENT_REQUEST_INIT_RT;

         //Save the time at which the message was sent
         context->timestamp = time;

         //The timeout value should be randomized by the value of a uniform
         //number chosen from the range -1 to +1
         context->timeout = context->retransmitTimeout +
            netGetRandRange(-DHCP_CLIENT_RAND_FACTOR, DHCP_CLIENT_RAND_FACTOR);

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else if(context->retransmitCount < DHCP_CLIENT_REQUEST_MAX_RC)
      {
         //Send a DHCPREQUEST message
         dhcpClientSendRequest(context);

         //The timeout value is doubled for each subsequent retransmission
         context->retransmitTimeout *= 2;

         //Limit the timeout value to a maximum of 64 seconds
         if(context->retransmitTimeout > DHCP_CLIENT_REQUEST_MAX_RT)
            context->retransmitTimeout = DHCP_CLIENT_REQUEST_MAX_RT;

         //Save the time at which the message was sent
         context->timestamp = time;

         //The timeout value should be randomized by the value of a uniform
         //number chosen from the range -1 to +1
         context->timeout = context->retransmitTimeout +
            netGetRandRange(-DHCP_CLIENT_RAND_FACTOR, DHCP_CLIENT_RAND_FACTOR);

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else
      {
         //If the client does not receive a response within a reasonable
         //period of time, then it restarts the initialization procedure
         dhcpClientChangeState(context, DHCP_STATE_INIT, 0);
      }
   }

   //Manage DHCP configuration timeout
   dhcpClientCheckTimeout(context);
}


/**
 * @brief INIT-REBOOT state
 *
 * When a client that already has a valid lease starts up after a
 * power-down or reboot, it starts here instead of the INIT state
 *
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientStateInitReboot(DhcpClientContext *context)
{
   systime_t delay;
   NetInterface *interface;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Check whether the DHCP client is running
   if(context->running)
   {
      //Wait for the link to be up before starting DHCP configuration
      if(interface->linkState)
      {
         //The client should wait for a random time to desynchronize
         //the use of DHCP at startup
         delay = netGetRandRange(0, DHCP_CLIENT_INIT_DELAY);

         //Record the time at which the client started the address
         //acquisition process
         context->configStartTime = osGetSystemTime();
         //Clear flag
         context->timeoutEventDone = FALSE;

         //Switch to the REBOOTING state
         dhcpClientChangeState(context, DHCP_STATE_REBOOTING, delay);
      }
   }
}


/**
 * @brief REBOOTING state
 *
 * A client that has rebooted with an assigned address is
 * waiting for a confirming reply from a server
 *
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientStateRebooting(DhcpClientContext *context)
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
         //A transaction identifier is used by the client to
         //match incoming DHCP messages with pending requests
         context->transactionId = netGetRand();

         //Send a DHCPREQUEST message
         dhcpClientSendRequest(context);

         //Initial timeout value
         context->retransmitTimeout = DHCP_CLIENT_REQUEST_INIT_RT;

         //Save the time at which the message was sent
         context->timestamp = time;

         //The timeout value should be randomized by the value of a uniform
         //number chosen from the range -1 to +1
         context->timeout = context->retransmitTimeout +
            netGetRandRange(-DHCP_CLIENT_RAND_FACTOR, DHCP_CLIENT_RAND_FACTOR);

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else if(context->retransmitCount < DHCP_CLIENT_REQUEST_MAX_RC)
      {
         //Send a DHCPREQUEST message
         dhcpClientSendRequest(context);

         //The timeout value is doubled for each subsequent retransmission
         context->retransmitTimeout *= 2;

         //Limit the timeout value to a maximum of 64 seconds
         if(context->retransmitTimeout > DHCP_CLIENT_REQUEST_MAX_RT)
            context->retransmitTimeout = DHCP_CLIENT_REQUEST_MAX_RT;

         //Save the time at which the message was sent
         context->timestamp = time;

         //The timeout value should be randomized by the value of a uniform
         //number chosen from the range -1 to +1
         context->timeout = context->retransmitTimeout +
            netGetRandRange(-DHCP_CLIENT_RAND_FACTOR, DHCP_CLIENT_RAND_FACTOR);

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else
      {
         //If the client does not receive a response within a reasonable
         //period of time, then it restarts the initialization procedure
         dhcpClientChangeState(context, DHCP_STATE_INIT, 0);
      }
   }

   //Manage DHCP configuration timeout
   dhcpClientCheckTimeout(context);
}


/**
 * @brief PROBING state
 *
 * The client probes the newly received address
 *
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientStateProbing(DhcpClientContext *context)
{
   uint_t i;
   systime_t time;
   NetInterface *interface;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Index of the IP address in the list of addresses assigned to the interface
   i = context->settings.ipAddrIndex;

   //Get current time
   time = osGetSystemTime();

   //Check current time
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //The address is already in use?
      if(interface->ipv4Context.addrList[i].conflict)
      {
         //If the client detects that the address is already in use, the
         //client must send a DHCPDECLINE message to the server and
         //restarts the configuration process
         dhcpClientSendDecline(context);

         //The client should wait a minimum of ten seconds before
         //restarting the configuration process to avoid excessive
         //network traffic in case of looping
         dhcpClientChangeState(context, DHCP_STATE_INIT, 0);
      }
      //Probing is on-going?
      else if(context->retransmitCount < DHCP_CLIENT_PROBE_NUM)
      {
         //Conflict detection is done using ARP probes
         arpSendProbe(interface, interface->ipv4Context.addrList[i].addr);

         //Save the time at which the packet was sent
         context->timestamp = time;
         //Delay until repeated probe
         context->timeout = DHCP_CLIENT_PROBE_DELAY;
         //Increment retransmission counter
         context->retransmitCount++;
      }
      //Probing is complete?
      else
      {
         //The use of the IPv4 address is now unrestricted
         interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_VALID;

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
         //Restart mDNS probing process
         mdnsResponderStartProbing(interface->mdnsResponderContext);
#endif
         //Dump current DHCP configuration for debugging purpose
         dhcpClientDumpConfig(context);

         //The client transitions to the BOUND state
         dhcpClientChangeState(context, DHCP_STATE_BOUND, 0);
      }
   }
}


/**
 * @brief BOUND state
 *
 * Client has a valid lease and is in its normal operating state
 *
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientStateBound(DhcpClientContext *context)
{
   systime_t t1;
   systime_t time;

   //Get current time
   time = osGetSystemTime();

   //A client will never attempt to extend the lifetime
   //of the address when T1 set to 0xFFFFFFFF
   if(context->t1 != DHCP_INFINITE_TIME)
   {
      //Convert T1 to milliseconds
      if(context->t1 < (MAX_DELAY / 1000))
         t1 = context->t1 * 1000;
      else
         t1 = MAX_DELAY;

      //Check the time elapsed since the lease was obtained
      if(timeCompare(time, context->leaseStartTime + t1) >= 0)
      {
         //Record the time at which the client started the address renewal process
         context->configStartTime = time;

         //Enter the RENEWING state
         dhcpClientChangeState(context, DHCP_STATE_RENEWING, 0);
      }
   }
}


/**
 * @brief RENEWING state
 *
 * Client is trying to renew its lease. It regularly sends
 * DHCPREQUEST messages with the server that gave it its current
 * lease specified, and waits for a reply
 *
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientStateRenewing(DhcpClientContext *context)
{
   systime_t t2;
   systime_t time;

   //Get current time
   time = osGetSystemTime();

   //Check current time
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //Convert T2 to milliseconds
      if(context->t2 < (MAX_DELAY / 1000))
         t2 = context->t2 * 1000;
      else
         t2 = MAX_DELAY;

      //Check whether T2 timer has expired
      if(timeCompare(time, context->leaseStartTime + t2) < 0)
      {
         //First DHCPREQUEST message?
         if(context->retransmitCount == 0)
         {
            //A transaction identifier is used by the client to
            //match incoming DHCP messages with pending requests
            context->transactionId = netGetRand();
         }

         //Send a DHCPREQUEST message
         dhcpClientSendRequest(context);

         //Save the time at which the message was sent
         context->timestamp = time;

         //Compute the remaining time until T2 expires
         context->timeout = context->leaseStartTime + t2 - time;

         //The client should wait one-half of the remaining time until T2, down to
         //a minimum of 60 seconds, before retransmitting the DHCPREQUEST message
         if(context->timeout > (2 * DHCP_CLIENT_REQUEST_MIN_DELAY))
            context->timeout /= 2;

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else
      {
         //If no DHCPACK arrives before time T2, the client moves to REBINDING
         dhcpClientChangeState(context, DHCP_STATE_REBINDING, 0);
      }
   }
}


/**
 * @brief REBINDING state
 *
 * The client has failed to renew its lease with the server that originally
 * granted it, and now seeks a lease extension with any server that can
 * hear it. It periodically sends DHCPREQUEST messages with no server specified
 * until it gets a reply or the lease ends
 *
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientStateRebinding(DhcpClientContext *context)
{
   uint_t i;
   systime_t time;
   systime_t leaseTime;
   NetInterface *interface;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Index of the IP address in the list of addresses assigned to the interface
   i = context->settings.ipAddrIndex;

   //Get current time
   time = osGetSystemTime();

   //Check current time
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //Convert the lease time to milliseconds
      if(context->leaseTime < (MAX_DELAY / 1000))
         leaseTime = context->leaseTime * 1000;
      else
         leaseTime = MAX_DELAY;

      //Check whether the lease has expired
      if(timeCompare(time, context->leaseStartTime + leaseTime) < 0)
      {
         //First DHCPREQUEST message?
         if(context->retransmitCount == 0)
         {
            //A transaction identifier is used by the client to
            //match incoming DHCP messages with pending requests
            context->transactionId = netGetRand();
         }

         //Send a DHCPREQUEST message
         dhcpClientSendRequest(context);

         //Save the time at which the message was sent
         context->timestamp = time;

         //Compute the remaining time until the lease expires
         context->timeout = context->leaseStartTime + leaseTime - time;

         //The client should wait one-half of the remaining lease time, down to a
         //minimum of 60 seconds, before retransmitting the DHCPREQUEST message
         if(context->timeout > (2 * DHCP_CLIENT_REQUEST_MIN_DELAY))
            context->timeout /= 2;

         //Increment retransmission counter
         context->retransmitCount++;
      }
      else
      {
         //The host address is no longer valid...
         interface->ipv4Context.addrList[i].addr = IPV4_UNSPECIFIED_ADDR;
         interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_INVALID;

         //Clear subnet mask
         interface->ipv4Context.addrList[i].subnetMask = IPV4_UNSPECIFIED_ADDR;

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
         //Restart mDNS probing process
         mdnsResponderStartProbing(interface->mdnsResponderContext);
#endif

         //If the lease expires before the client receives
         //a DHCPACK, the client moves to INIT state
         dhcpClientChangeState(context, DHCP_STATE_INIT, 0);
      }
   }
}


/**
 * @brief Send DHCPDISCOVER message
 * @param[in] context Pointer to the DHCP client context
 * @return Error code
 **/

error_t dhcpClientSendDiscover(DhcpClientContext *context)
{
   error_t error;
   size_t offset;
   NetBuffer *buffer;
   NetInterface *interface;
   NetInterface *logicalInterface;
   DhcpMessage *message;
   IpAddr srcIpAddr;
   IpAddr destIpAddr;
   NetTxAncillary ancillary;
#if (DHCP_CLIENT_HOSTNAME_OPTION_SUPPORT == ENABLED)
   size_t length;
#endif

   //DHCP message type
   const uint8_t messageType = DHCP_MESSAGE_TYPE_DISCOVER;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Allocate a memory buffer to hold the DHCP message
   buffer = udpAllocBuffer(DHCP_MIN_MSG_SIZE, &offset);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the DHCP message
   message = netBufferAt(buffer, offset);
   //Clear memory buffer contents
   osMemset(message, 0, DHCP_MIN_MSG_SIZE);

   //Format DHCPDISCOVER message
   message->op = DHCP_OPCODE_BOOTREQUEST;
   message->htype = DHCP_HARDWARE_TYPE_ETH;
   message->hlen = sizeof(MacAddr);
   message->xid = htonl(context->transactionId);
   message->secs = dhcpClientComputeElapsedTime(context);
   message->flags = HTONS(DHCP_FLAG_BROADCAST);
   message->ciaddr = IPV4_UNSPECIFIED_ADDR;
   message->chaddr = logicalInterface->macAddr;

   //Write magic cookie before setting any option
   message->magicCookie = HTONL(DHCP_MAGIC_COOKIE);
   //Properly terminate options field
   message->options[0] = DHCP_OPT_END;

   //DHCP Message Type option
   dhcpAddOption(message, DHCP_OPT_DHCP_MESSAGE_TYPE,
      &messageType, sizeof(messageType));

#if (DHCP_CLIENT_HOSTNAME_OPTION_SUPPORT == ENABLED)
   //Retrieve the length of the host name
   length = osStrlen(context->settings.hostname);

   //Any host name defined?
   if(length > 0)
   {
      //The Host Name option specifies the name of the client
      dhcpAddOption(message, DHCP_OPT_HOST_NAME,
         context->settings.hostname, length);
   }
#endif

#if (DHCP_CLIENT_ID_OPTION_SUPPORT == ENABLED)
   //Any client identifier defined?
   if(context->settings.clientIdLength > 0)
   {
      //DHCP servers use this value to index their database of address bindings
      dhcpAddOption(message, DHCP_OPT_CLIENT_IDENTIFIER,
         context->settings.clientId, context->settings.clientIdLength);
   }
#endif

   //Check whether rapid commit is enabled
   if(context->settings.rapidCommit)
   {
      //Include the Rapid Commit option if the client is prepared
      //to perform the DHCPDISCOVER-DHCPACK message exchange
      dhcpAddOption(message, DHCP_OPT_RAPID_COMMIT, NULL, 0);
   }

   //DHCP messages broadcast by a client prior to that client obtaining its
   //IP address must have the source address field in the IP header set to 0
   //(refer to RFC 2131, section 4.1)
   srcIpAddr.length = sizeof(Ipv4Addr);
   srcIpAddr.ipv4Addr = IPV4_UNSPECIFIED_ADDR;

   //Set destination IP address
   destIpAddr.length = sizeof(Ipv4Addr);
   destIpAddr.ipv4Addr = IPV4_BROADCAST_ADDR;

   //Debug message
   TRACE_DEBUG("\r\n%s: Sending DHCP message (%" PRIuSIZE " bytes)...\r\n",
      formatSystemTime(osGetSystemTime(), NULL), DHCP_MIN_MSG_SIZE);

   //Dump the contents of the message for debugging purpose
   dhcpDumpMessage(message, DHCP_MIN_MSG_SIZE);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //Broadcast DHCPDISCOVER message
   error = udpSendBuffer(interface, &srcIpAddr, DHCP_CLIENT_PORT, &destIpAddr,
      DHCP_SERVER_PORT, buffer, offset, &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send DHCPREQUEST message
 * @param[in] context Pointer to the DHCP client context
 * @return Error code
 **/

error_t dhcpClientSendRequest(DhcpClientContext *context)
{
   uint_t i;
   error_t error;
   size_t offset;
   NetBuffer *buffer;
   NetInterface *interface;
   NetInterface *logicalInterface;
   DhcpMessage *message;
   IpAddr srcIpAddr;
   IpAddr destIpAddr;
   NetTxAncillary ancillary;
#if (DHCP_CLIENT_HOSTNAME_OPTION_SUPPORT == ENABLED)
   size_t length;
#endif

   //DHCP message type
   const uint8_t messageType = DHCP_MESSAGE_TYPE_REQUEST;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Index of the IP address in the list of addresses assigned to the interface
   i = context->settings.ipAddrIndex;

   //Allocate a memory buffer to hold the DHCP message
   buffer = udpAllocBuffer(DHCP_MIN_MSG_SIZE, &offset);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the DHCP message
   message = netBufferAt(buffer, offset);
   //Clear memory buffer contents
   osMemset(message, 0, DHCP_MIN_MSG_SIZE);

   //Format DHCPREQUEST message
   message->op = DHCP_OPCODE_BOOTREQUEST;
   message->htype = DHCP_HARDWARE_TYPE_ETH;
   message->hlen = sizeof(MacAddr);
   message->xid = htonl(context->transactionId);
   message->secs = dhcpClientComputeElapsedTime(context);

   //The client IP address must be included if the client is fully configured
   //and can respond to ARP requests
   if(context->state == DHCP_STATE_RENEWING ||
      context->state == DHCP_STATE_REBINDING)
   {
      message->flags = 0;
      message->ciaddr = interface->ipv4Context.addrList[i].addr;
   }
   else
   {
      message->flags = HTONS(DHCP_FLAG_BROADCAST);
      message->ciaddr = IPV4_UNSPECIFIED_ADDR;
   }

   //Client hardware address
   message->chaddr = logicalInterface->macAddr;
   //Write magic cookie before setting any option
   message->magicCookie = HTONL(DHCP_MAGIC_COOKIE);
   //Properly terminate options field
   message->options[0] = DHCP_OPT_END;

   //DHCP Message Type option
   dhcpAddOption(message, DHCP_OPT_DHCP_MESSAGE_TYPE,
      &messageType, sizeof(messageType));

#if (DHCP_CLIENT_HOSTNAME_OPTION_SUPPORT == ENABLED)
   //Retrieve the length of the host name
   length = osStrlen(context->settings.hostname);

   //Any host name defined?
   if(length > 0)
   {
      //The Host Name option specifies the name of the client
      dhcpAddOption(message, DHCP_OPT_HOST_NAME,
         context->settings.hostname, length);
   }
#endif

#if (DHCP_CLIENT_ID_OPTION_SUPPORT == ENABLED)
   //Any client identifier defined?
   if(context->settings.clientIdLength > 0)
   {
      //DHCP servers use this value to index their database of address bindings
      dhcpAddOption(message, DHCP_OPT_CLIENT_IDENTIFIER,
         context->settings.clientId, context->settings.clientIdLength);
   }
#endif

   //Server Identifier option
   if(context->state == DHCP_STATE_REQUESTING)
   {
      dhcpAddOption(message, DHCP_OPT_SERVER_IDENTIFIER,
         &context->serverIpAddr, sizeof(Ipv4Addr));
   }

   //Requested IP Address option
   if(context->state == DHCP_STATE_REQUESTING ||
      context->state == DHCP_STATE_REBOOTING)
   {
      dhcpAddOption(message, DHCP_OPT_REQUESTED_IP_ADDRESS,
         &context->requestedIpAddr, sizeof(Ipv4Addr));
   }

   //Parameter Request List option
   dhcpAddOption(message, DHCP_OPT_PARAM_REQUEST_LIST,
      dhcpOptionList, sizeof(dhcpOptionList));

   //IP address is being renewed?
   if(context->state == DHCP_STATE_RENEWING)
   {
      //Set source IP address
      srcIpAddr.length = sizeof(Ipv4Addr);
      srcIpAddr.ipv4Addr = interface->ipv4Context.addrList[i].addr;

      //The client transmits the message directly to the server that initially
      //granted the lease
      destIpAddr.length = sizeof(Ipv4Addr);
      destIpAddr.ipv4Addr = context->serverIpAddr;
   }
   else
   {
      //DHCP messages broadcast by a client prior to that client obtaining its
      //IP address must have the source address field in the IP header set to 0
      //(refer to RFC 2131, section 4.1)
      srcIpAddr.length = sizeof(Ipv4Addr);
      srcIpAddr.ipv4Addr = IPV4_UNSPECIFIED_ADDR;

      //Broadcast the message
      destIpAddr.length = sizeof(Ipv4Addr);
      destIpAddr.ipv4Addr = IPV4_BROADCAST_ADDR;
   }

   //Debug message
   TRACE_DEBUG("\r\n%s: Sending DHCP message (%" PRIuSIZE " bytes)...\r\n",
      formatSystemTime(osGetSystemTime(), NULL), DHCP_MIN_MSG_SIZE);

   //Dump the contents of the message for debugging purpose
   dhcpDumpMessage(message, DHCP_MIN_MSG_SIZE);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //Send DHCPREQUEST message
   error = udpSendBuffer(interface, &srcIpAddr, DHCP_CLIENT_PORT, &destIpAddr,
      DHCP_SERVER_PORT, buffer, offset, &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send DHCPDECLINE message
 * @param[in] context Pointer to the DHCP client context
 * @return Error code
 **/

error_t dhcpClientSendDecline(DhcpClientContext *context)
{
   error_t error;
   size_t offset;
   NetBuffer *buffer;
   NetInterface *interface;
   NetInterface *logicalInterface;
   DhcpMessage *message;
   IpAddr srcIpAddr;
   IpAddr destIpAddr;
   NetTxAncillary ancillary;

   //DHCP message type
   const uint8_t messageType = DHCP_MESSAGE_TYPE_DECLINE;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Allocate a memory buffer to hold the DHCP message
   buffer = udpAllocBuffer(DHCP_MIN_MSG_SIZE, &offset);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the DHCP message
   message = netBufferAt(buffer, offset);
   //Clear memory buffer contents
   osMemset(message, 0, DHCP_MIN_MSG_SIZE);

   //Format DHCPDECLINE message
   message->op = DHCP_OPCODE_BOOTREQUEST;
   message->htype = DHCP_HARDWARE_TYPE_ETH;
   message->hlen = sizeof(MacAddr);
   message->xid = htonl(context->transactionId);
   message->secs = 0;
   message->flags = 0;
   message->ciaddr = IPV4_UNSPECIFIED_ADDR;
   message->chaddr = logicalInterface->macAddr;

   //Write magic cookie before setting any option
   message->magicCookie = HTONL(DHCP_MAGIC_COOKIE);
   //Properly terminate options field
   message->options[0] = DHCP_OPT_END;

   //DHCP Message Type option
   dhcpAddOption(message, DHCP_OPT_DHCP_MESSAGE_TYPE,
      &messageType, sizeof(messageType));
   //Server Identifier option
   dhcpAddOption(message, DHCP_OPT_SERVER_IDENTIFIER,
      &context->serverIpAddr, sizeof(Ipv4Addr));
   //Requested IP Address option
   dhcpAddOption(message, DHCP_OPT_REQUESTED_IP_ADDRESS,
      &context->requestedIpAddr, sizeof(Ipv4Addr));

   //Use the unspecified address as source address
   srcIpAddr.length = sizeof(Ipv4Addr);
   srcIpAddr.ipv4Addr = IPV4_UNSPECIFIED_ADDR;

   //Set destination IP address
   destIpAddr.length = sizeof(Ipv4Addr);
   destIpAddr.ipv4Addr = IPV4_BROADCAST_ADDR;

   //Debug message
   TRACE_DEBUG("\r\n%s: Sending DHCP message (%" PRIuSIZE " bytes)...\r\n",
      formatSystemTime(osGetSystemTime(), NULL), DHCP_MIN_MSG_SIZE);

   //Dump the contents of the message for debugging purpose
   dhcpDumpMessage(message, DHCP_MIN_MSG_SIZE);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //Broadcast DHCPDECLINE message
   error = udpSendBuffer(interface, &srcIpAddr, DHCP_CLIENT_PORT, &destIpAddr,
      DHCP_SERVER_PORT, buffer, offset, &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Process incoming DHCP message
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @param[in] udpHeader UDP header
 * @param[in] buffer Multi-part buffer containing the incoming DHCP message
 * @param[in] offset Offset to the first byte of the DHCP message
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @param[in] param Pointer to the DHCP client context
 **/

void dhcpClientProcessMessage(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *udpHeader,
   const NetBuffer *buffer, size_t offset, const NetRxAncillary *ancillary,
   void *param)
{
   size_t length;
   DhcpClientContext *context;
   DhcpMessage *message;
   DhcpOption *option;

   //Point to the DHCP client context
   context = (DhcpClientContext *) param;

   //Retrieve the length of the DHCP message
   length = netBufferGetLength(buffer) - offset;

   //Make sure the DHCP message is valid
   if(length < sizeof(DhcpMessage) || length > DHCP_MAX_MSG_SIZE)
      return;

   //Point to the beginning of the DHCP message
   message = netBufferAt(buffer, offset);
   //Sanity check
   if(message == NULL)
      return;

   //Debug message
   TRACE_DEBUG("\r\n%s: DHCP message received (%" PRIuSIZE " bytes)...\r\n",
      formatSystemTime(osGetSystemTime(), NULL), length);

   //Dump the contents of the message for debugging purpose
   dhcpDumpMessage(message, length);

   //The DHCP server shall respond with a BOOTREPLY opcode
   if(message->op != DHCP_OPCODE_BOOTREPLY)
      return;

   //Enforce hardware type
   if(message->htype != DHCP_HARDWARE_TYPE_ETH)
      return;

   //Check the length of the hardware address
   if(message->hlen != sizeof(MacAddr))
      return;

   //Check magic cookie
   if(message->magicCookie != HTONL(DHCP_MAGIC_COOKIE))
      return;

   //The DHCP Message Type option must be included in every DHCP message
   option = dhcpGetOption(message, length, DHCP_OPT_DHCP_MESSAGE_TYPE);

   //Failed to retrieve the Message Type option?
   if(option == NULL || option->length != 1)
      return;

   //Check message type
   switch(option->value[0])
   {
   case DHCP_MESSAGE_TYPE_OFFER:
      //Parse DHCPOFFER message
      dhcpClientParseOffer(context, message, length);
      break;
   case DHCP_MESSAGE_TYPE_ACK:
      //Parse DHCPACK message
      dhcpClientParseAck(context, message, length);
      break;
   case DHCP_MESSAGE_TYPE_NAK:
      //Parse DHCPNAK message
      dhcpClientParseNak(context, message, length);
      break;
   default:
      //Silently drop incoming message
      break;
   }
}


/**
 * @brief Parse DHCPOFFER message
 * @param[in] context Pointer to the DHCP client context
 * @param[in] message Pointer to the incoming DHCP message
 * @param[in] length Length of the incoming message to parse
 **/

void dhcpClientParseOffer(DhcpClientContext *context,
   const DhcpMessage *message, size_t length)
{
   DhcpOption *serverIdOption;
   NetInterface *interface;
   NetInterface *logicalInterface;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Discard any received packet that does not match the transaction ID
   if(ntohl(message->xid) != context->transactionId)
      return;

   //Make sure the IP address offered to the client is valid
   if(message->yiaddr == IPV4_UNSPECIFIED_ADDR)
      return;

   //Check MAC address
   if(!macCompAddr(&message->chaddr, &logicalInterface->macAddr))
      return;

   //Make sure that the DHCPOFFER message is received in response to
   //a DHCPDISCOVER message
   if(context->state != DHCP_STATE_SELECTING)
      return;

   //A DHCP server always returns its own address in the Server Identifier option
   serverIdOption = dhcpGetOption(message, length, DHCP_OPT_SERVER_IDENTIFIER);

   //Failed to retrieve the Server Identifier option?
   if(serverIdOption == NULL || serverIdOption->length != 4)
      return;

   //Record the IP address of the DHCP server
   ipv4CopyAddr(&context->serverIpAddr, serverIdOption->value);
   //Record the IP address offered to the client
   context->requestedIpAddr = message->yiaddr;

   //Switch to the REQUESTING state
   dhcpClientChangeState(context, DHCP_STATE_REQUESTING, 0);
}


/**
 * @brief Parse DHCPACK message
 * @param[in] context Pointer to the DHCP client context
 * @param[in] message Pointer to the incoming DHCP message
 * @param[in] length Length of the incoming message to parse
 **/

void dhcpClientParseAck(DhcpClientContext *context,
   const DhcpMessage *message, size_t length)
{
   uint_t i;
   uint_t j;
   uint_t n;
   DhcpOption *option;
   DhcpOption *serverIdOption;
   NetInterface *interface;
   NetInterface *logicalInterface;
   NetInterface *physicalInterface;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);
   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);

   //Index of the IP address in the list of addresses assigned to the interface
   i = context->settings.ipAddrIndex;

   //Discard any received packet that does not match the transaction ID
   if(ntohl(message->xid) != context->transactionId)
      return;

   //Make sure the IP address assigned to the client is valid
   if(message->yiaddr == IPV4_UNSPECIFIED_ADDR)
      return;

   //Check MAC address
   if(!macCompAddr(&message->chaddr, &logicalInterface->macAddr))
      return;

   //A DHCP server always returns its own address in the Server Identifier option
   serverIdOption = dhcpGetOption(message, length, DHCP_OPT_SERVER_IDENTIFIER);

   //Failed to retrieve the Server Identifier option?
   if(serverIdOption == NULL || serverIdOption->length != 4)
      return;

   //Check current state
   if(context->state == DHCP_STATE_SELECTING)
   {
      //A DHCPACK message is not acceptable when rapid commit is disallowed
      if(!context->settings.rapidCommit)
         return;

      //Search for the Rapid Commit option
      option = dhcpGetOption(message, length, DHCP_OPT_RAPID_COMMIT);

      //A server must include this option in a DHCPACK message sent
      //in a response to a DHCPDISCOVER message when completing the
      //DHCPDISCOVER-DHCPACK message exchange
      if(option == NULL || option->length != 0)
         return;
   }
   else if(context->state == DHCP_STATE_REQUESTING ||
      context->state == DHCP_STATE_RENEWING)
   {
      //Check the server identifier
      if(!ipv4CompAddr(serverIdOption->value, &context->serverIpAddr))
         return;
   }
   else if(context->state == DHCP_STATE_REBOOTING ||
      context->state == DHCP_STATE_REBINDING)
   {
      //Do not check the server identifier
   }
   else
   {
      //Silently discard the DHCPACK message
      return;
   }

   //Retrieve IP Address Lease Time option
   option = dhcpGetOption(message, length, DHCP_OPT_IP_ADDRESS_LEASE_TIME);

   //Failed to retrieve specified option?
   if(option == NULL || option->length != 4)
      return;

   //Record the lease time
   context->leaseTime = LOAD32BE(option->value);

   //Retrieve Renewal Time Value option
   option = dhcpGetOption(message, length, DHCP_OPT_RENEWAL_TIME_VALUE);

   //Specified option found?
   if(option != NULL && option->length == 4)
   {
      //This option specifies the time interval from address assignment
      //until the client transitions to the RENEWING state
      context->t1 = LOAD32BE(option->value);
   }
   else if(context->leaseTime != DHCP_INFINITE_TIME)
   {
      //By default, T1 is set to 50% of the lease time
      context->t1 = context->leaseTime / 2;
   }
   else
   {
      //Infinite lease
      context->t1 = DHCP_INFINITE_TIME;
   }

   //Retrieve Rebinding Time value option
   option = dhcpGetOption(message, length, DHCP_OPT_REBINDING_TIME_VALUE);

   //Specified option found?
   if(option != NULL && option->length == 4)
   {
      //This option specifies the time interval from address assignment
      //until the client transitions to the REBINDING state
      context->t2 = LOAD32BE(option->value);
   }
   else if(context->leaseTime != DHCP_INFINITE_TIME)
   {
      //By default, T2 is set to 87.5% of the lease time
      context->t2 = context->leaseTime * 7 / 8;
   }
   else
   {
      //Infinite lease
      context->t2 = DHCP_INFINITE_TIME;
   }

   //Retrieve Subnet Mask option
   option = dhcpGetOption(message, length, DHCP_OPT_SUBNET_MASK);

   //The specified option has been found?
   if(option != NULL && option->length == sizeof(Ipv4Addr))
   {
      //Save subnet mask
      ipv4CopyAddr(&interface->ipv4Context.addrList[i].subnetMask,
         option->value);
   }

   //Retrieve Router option
   option = dhcpGetOption(message, length, DHCP_OPT_ROUTER);

   //The specified option has been found?
   if(option != NULL && !(option->length % sizeof(Ipv4Addr)))
   {
      //Save default gateway
      if(option->length >= sizeof(Ipv4Addr))
      {
         ipv4CopyAddr(&interface->ipv4Context.addrList[i].defaultGateway,
            option->value);
      }
   }

   //Use the DNS servers provided by the DHCP server?
   if(!context->settings.manualDnsConfig)
   {
      //Retrieve DNS Server option
      option = dhcpGetOption(message, length, DHCP_OPT_DNS_SERVER);

      //The specified option has been found?
      if(option != NULL && !(option->length % sizeof(Ipv4Addr)))
      {
         //Get the number of addresses provided in the response
         n = option->length / sizeof(Ipv4Addr);

         //Loop through the list of addresses
         for(j = 0; j < n && j < IPV4_DNS_SERVER_LIST_SIZE; j++)
         {
            //Save DNS server address
            ipv4CopyAddr(&interface->ipv4Context.dnsServerList[j],
               option->value + j * sizeof(Ipv4Addr));
         }
      }
   }

   //Retrieve MTU option
   option = dhcpGetOption(message, length, DHCP_OPT_INTERFACE_MTU);

   //The specified option has been found?
   if(option != NULL && option->length == 2)
   {
      //This option specifies the MTU to use on this interface
      n = LOAD16BE(option->value);

      //Make sure that the option's value is acceptable
      if(n >= IPV4_MINIMUM_MTU && n <= physicalInterface->nicDriver->mtu)
      {
         //Set the MTU to be used on the interface
         interface->ipv4Context.linkMtu = n;
      }
   }

   //Record the IP address of the DHCP server
   ipv4CopyAddr(&context->serverIpAddr, serverIdOption->value);
   //Record the IP address assigned to the client
   context->requestedIpAddr = message->yiaddr;

   //Save the time a which the lease was obtained
   context->leaseStartTime = osGetSystemTime();

   //Check current state
   if(context->state == DHCP_STATE_REQUESTING ||
      context->state == DHCP_STATE_REBOOTING)
   {
      //Use the IP address as a tentative address
      interface->ipv4Context.addrList[i].addr = message->yiaddr;
      interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_TENTATIVE;

      //Clear conflict flag
      interface->ipv4Context.addrList[i].conflict = FALSE;

      //The client should probe the newly received address
      dhcpClientChangeState(context, DHCP_STATE_PROBING, 0);
   }
   else
   {
      //Assign the IP address to the client
      interface->ipv4Context.addrList[i].addr = message->yiaddr;
      interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_VALID;

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
      //Restart mDNS probing process
      mdnsResponderStartProbing(interface->mdnsResponderContext);
#endif
      //The client transitions to the BOUND state
      dhcpClientChangeState(context, DHCP_STATE_BOUND, 0);
   }
}


/**
 * @brief Parse DHCPNAK message
 * @param[in] context Pointer to the DHCP client context
 * @param[in] message Pointer to the incoming DHCP message
 * @param[in] length Length of the incoming message to parse
 **/

void dhcpClientParseNak(DhcpClientContext *context,
   const DhcpMessage *message, size_t length)
{
   uint_t i;
   DhcpOption *serverIdOption;
   NetInterface *interface;
   NetInterface *logicalInterface;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Index of the IP address in the list of addresses assigned to the interface
   i = context->settings.ipAddrIndex;

   //Discard any received packet that does not match the transaction ID
   if(ntohl(message->xid) != context->transactionId)
      return;

   //Check MAC address
   if(!macCompAddr(&message->chaddr, &logicalInterface->macAddr))
      return;

   //A DHCP server always returns its own address in the Server Identifier option
   serverIdOption = dhcpGetOption(message, length, DHCP_OPT_SERVER_IDENTIFIER);

   //Failed to retrieve the Server Identifier option?
   if(serverIdOption == NULL || serverIdOption->length != 4)
      return;

   //Check current state
   if(context->state == DHCP_STATE_REQUESTING ||
      context->state == DHCP_STATE_RENEWING)
   {
      //Check the server identifier
      if(!ipv4CompAddr(serverIdOption->value, &context->serverIpAddr))
         return;
   }
   else if(context->state == DHCP_STATE_REBOOTING ||
      context->state == DHCP_STATE_REBINDING)
   {
      //Do not check the server identifier
   }
   else
   {
      //Silently discard the DHCPNAK message
      return;
   }

   //The host address is no longer appropriate for the link
   interface->ipv4Context.addrList[i].addr = IPV4_UNSPECIFIED_ADDR;
   interface->ipv4Context.addrList[i].state = IPV4_ADDR_STATE_INVALID;

   //Clear subnet mask
   interface->ipv4Context.addrList[i].subnetMask = IPV4_UNSPECIFIED_ADDR;

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
   //Restart mDNS probing process
   mdnsResponderStartProbing(interface->mdnsResponderContext);
#endif

   //Restart DHCP configuration
   dhcpClientChangeState(context, DHCP_STATE_INIT, 0);
}


/**
 * @brief Manage DHCP configuration timeout
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientCheckTimeout(DhcpClientContext *context)
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
      //DHCP configuration timeout?
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
 * @brief Compute the appropriate secs field
 *
 * Compute the number of seconds elapsed since the client began
 * address acquisition or renewal process
 *
 * @param[in] context Pointer to the DHCP client context
 * @return The elapsed time expressed in seconds
 **/

uint16_t dhcpClientComputeElapsedTime(DhcpClientContext *context)
{
   systime_t time;

   //Compute the time elapsed since the DHCP configuration process started
   time = (osGetSystemTime() - context->configStartTime) / 1000;

   //The value 0xFFFF is used to represent any elapsed time values
   //greater than the largest time value that can be represented
   time = MIN(time, 0xFFFF);

   //Convert the 16-bit value to network byte order
   return htons(time);
}


/**
 * @brief Update DHCP FSM state
 * @param[in] context Pointer to the DHCP client context
 * @param[in] newState New DHCP state to switch to
 * @param[in] delay Initial delay
 **/

void dhcpClientChangeState(DhcpClientContext *context,
   DhcpState newState, systime_t delay)
{
   systime_t time;

   //Get current time
   time = osGetSystemTime();

#if (DHCP_TRACE_LEVEL >= TRACE_LEVEL_INFO)
   //Sanity check
   if(newState <= DHCP_STATE_REBINDING)
   {
      //DHCP FSM states
      static const char_t *stateLabel[] =
      {
         "INIT",
         "SELECTING",
         "REQUESTING",
         "INIT-REBOOT",
         "REBOOTING",
         "PROBING",
         "BOUND",
         "RENEWING",
         "REBINDING"
      };

      //Debug message
      TRACE_INFO("%s: DHCP client %s state\r\n",
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
 * @brief Dump DHCP configuration for debugging purpose
 * @param[in] context Pointer to the DHCP client context
 **/

void dhcpClientDumpConfig(DhcpClientContext *context)
{
#if (DHCP_TRACE_LEVEL >= TRACE_LEVEL_INFO)
   uint_t i;
   uint_t j;
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
   TRACE_INFO("DHCP configuration:\r\n");

   //Lease start time
   TRACE_INFO("  Lease Start Time = %s\r\n",
      formatSystemTime(context->leaseStartTime, NULL));

   //Lease time
   TRACE_INFO("  Lease Time = %" PRIu32 "s\r\n", context->leaseTime);
   //Renewal time
   TRACE_INFO("  T1 = %" PRIu32 "s\r\n", context->t1);
   //Rebinding time
   TRACE_INFO("  T2 = %" PRIu32 "s\r\n", context->t2);

   //Host address
   TRACE_INFO("  IPv4 Address = %s\r\n",
      ipv4AddrToString(ipv4Context->addrList[i].addr, NULL));

   //Subnet mask
   TRACE_INFO("  Subnet Mask = %s\r\n",
      ipv4AddrToString(ipv4Context->addrList[i].subnetMask, NULL));

   //Default gateway
   TRACE_INFO("  Default Gateway = %s\r\n",
      ipv4AddrToString(ipv4Context->addrList[i].defaultGateway, NULL));

   //DNS servers
   for(j = 0; j < IPV4_DNS_SERVER_LIST_SIZE; j++)
   {
      TRACE_INFO("  DNS Server %u = %s\r\n", j + 1,
         ipv4AddrToString(ipv4Context->dnsServerList[j], NULL));
   }

   //Maximum transmit unit
   TRACE_INFO("  MTU = %" PRIuSIZE "\r\n", interface->ipv4Context.linkMtu);
   TRACE_INFO("\r\n");
#endif
}

#endif
