/**
 * @file mdns_responder.c
 * @brief mDNS responder (Multicast DNS)
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
#define TRACE_LEVEL MDNS_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include <ctype.h>
#include "core/net.h"
#include "mdns/mdns_responder.h"
#include "mdns/mdns_common.h"
#include "dns/dns_debug.h"
#include "dns_sd/dns_sd.h"
#include "str.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MDNS_RESPONDER_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t mdnsResponderTickCounter;


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains mDNS responder settings
 **/

void mdnsResponderGetDefaultSettings(MdnsResponderSettings *settings)
{
   //Use default interface
   settings->interface = netGetDefaultInterface();

   //Number of announcement packets
   settings->numAnnouncements = MDNS_ANNOUNCE_NUM;
   //TTL resource record
   settings->ttl = MDNS_DEFAULT_RR_TTL;
   //FSM state change event
   settings->stateChangeEvent = NULL;
}


/**
 * @brief mDNS responder initialization
 * @param[in] context Pointer to the mDNS responder context
 * @param[in] settings mDNS responder specific settings
 * @return Error code
 **/

error_t mdnsResponderInit(MdnsResponderContext *context,
   const MdnsResponderSettings *settings)
{
   NetInterface *interface;

   //Debug message
   TRACE_INFO("Initializing mDNS responder...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //Invalid network interface?
   if(settings->interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the underlying network interface
   interface = settings->interface;

   //Clear the mDNS responder context
   osMemset(context, 0, sizeof(MdnsResponderContext));
   //Save user settings
   context->settings = *settings;

   //mDNS responder is currently suspended
   context->running = FALSE;
   //Initialize state machine
   context->state = MDNS_STATE_INIT;

   //Attach the mDNS responder context to the network interface
   interface->mdnsResponderContext = context;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Start mDNS responder
 * @param[in] context Pointer to the mDNS responder context
 * @return Error code
 **/

error_t mdnsResponderStart(MdnsResponderContext *context)
{
   //Make sure the mDNS responder context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting mDNS responder...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Start mDNS responder
   context->running = TRUE;
   //Initialize state machine
   context->state = MDNS_STATE_INIT;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Stop mDNS responder
 * @param[in] context Pointer to the mDNS responder context
 * @return Error code
 **/

error_t mdnsResponderStop(MdnsResponderContext *context)
{
   //Make sure the mDNS responder context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Stopping mDNS responder...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Suspend mDNS responder
   context->running = FALSE;
   //Reinitialize state machine
   context->state = MDNS_STATE_INIT;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve current state
 * @param[in] context Pointer to the mDNS responder context
 * @return Current mDNS responder state
 **/

MdnsState mdnsResponderGetState(MdnsResponderContext *context)
{
   MdnsState state;

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
 * @brief Set hostname
 * @param[in] context Pointer to the mDNS responder context
 * @param[in] hostname NULL-terminated string that contains the hostname
 * @return Error code
 **/

error_t mdnsResponderSetHostname(MdnsResponderContext *context,
   const char_t *hostname)
{
   NetInterface *interface;

   //Check parameters
   if(context == NULL || hostname == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Check whether a hostname is already assigned
   if(context->hostname[0] != '\0')
   {
      //Check whether the link is up
      if(interface->linkState)
      {
         //Send a goodbye packet
         mdnsResponderSendGoodbye(context);
      }
   }

   //Set hostname
   strSafeCopy(context->hostname, hostname,
      MDNS_RESPONDER_MAX_HOSTNAME_LEN);

   //Restart probing process (hostname)
   mdnsResponderStartProbing(interface->mdnsResponderContext);

#if (DNS_SD_SUPPORT == ENABLED)
   //Restart probing process (service instance name)
   dnsSdStartProbing(interface->dnsSdContext);
#endif

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Generate domain name for reverse DNS lookup (IPv4)
 * @param[in] context Pointer to the mDNS responder context
 * @return Error code
 **/

error_t mdnsResponderSetIpv4ReverseName(MdnsResponderContext *context)
{
#if (IPV4_SUPPORT == ENABLED)
   uint8_t *addr;
   NetInterface *interface;

   //Check whether the mDNS responder has been properly instantiated
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Check whether the host address is valid
   if(interface->ipv4Context.addrList[0].state == IPV4_ADDR_STATE_VALID)
   {
      //Cast the IPv4 address as byte array
      addr = (uint8_t *) &interface->ipv4Context.addrList[0].addr;

      //Generate the domain name for reverse DNS lookup
      osSprintf(context->ipv4ReverseName, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8,
         addr[3], addr[2], addr[1], addr[0]);
   }
   else
   {
      //The host address is not valid
      context->ipv4ReverseName[0] = '\0';
   }
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Generate domain name for reverse DNS lookup (IPv6)
 * @param[in] context Pointer to the mDNS responder context
 * @return Error code
 **/

error_t mdnsResponderSetIpv6ReverseName(MdnsResponderContext *context)
{
#if (IPV6_SUPPORT == ENABLED)
   uint_t i;
   uint_t m;
   uint_t n;
   char_t *p;
   uint8_t *addr;
   NetInterface *interface;

   //Check whether the mDNS responder has been properly instantiated
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Point to the buffer where to format the reverse name
   p = context->ipv6ReverseName;

   //Check whether the link-local address is valid
   if(ipv6GetLinkLocalAddrState(interface) == IPV6_ADDR_STATE_PREFERRED)
   {
      //Cast the IPv4 address as byte array
      addr = interface->ipv6Context.addrList[0].addr.b;

      //Generate the domain name for reverse DNS lookup
      for(i = 0; i < 32; i++)
      {
         //Calculate the shift count
         n = (31 - i) / 2;
         m = (i % 2) * 4;

         //Format the current digit
         p += osSprintf(p, "%" PRIx8, (addr[n] >> m) & 0x0F);

         //Add a delimiter character
         if(i != 31)
            p += osSprintf(p, ".");
      }
   }
   else
   {
      //The link-local address is not valid
      p[0] = '\0';
   }
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Restart probing process
 * @param[in] context Pointer to the mDNS responder context
 * @return Error code
 **/

error_t mdnsResponderStartProbing(MdnsResponderContext *context)
{
   //Check whether the mDNS responder has been properly instantiated
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Generate domain names for reverse DNS lookup
   mdnsResponderSetIpv4ReverseName(context);
   mdnsResponderSetIpv6ReverseName(context);

   //Force mDNS responder to start probing again
   context->state = MDNS_STATE_INIT;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief mDNS responder timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * manage mDNS operation
 *
 * @param[in] context Pointer to the mDNS responder context
 **/

void mdnsResponderTick(MdnsResponderContext *context)
{
   bool_t valid;
   systime_t time;
   systime_t delay;
   NetInterface *interface;
   IpAddr destIpAddr;

   //Make sure the mDNS responder has been properly instantiated
   if(context == NULL)
      return;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Get current time
   time = osGetSystemTime();

   //Check current state
   if(context->state == MDNS_STATE_INIT)
   {
      //Wait for the link to be up before starting mDNS responder
      if(context->running && interface->linkState)
      {
         //Check whether a hostname has been assigned
         if(context->hostname[0] != '\0')
         {
            //Clear flag
            valid = FALSE;

#if (IPV4_SUPPORT == ENABLED)
            //Check whether the IPv4 host address is valid
            if(interface->ipv4Context.addrList[0].state == IPV4_ADDR_STATE_VALID)
               valid = TRUE;
#endif

#if (IPV6_SUPPORT == ENABLED)
            //Check whether the IPv6 link-local address is valid
            if(ipv6GetLinkLocalAddrState(interface) == IPV6_ADDR_STATE_PREFERRED)
               valid = TRUE;
#endif
            //Any valid IP address assigned to the network interface?
            if(valid)
            {
               //Wait until both IPv4 and IPv6 addresses are valid
               mdnsResponderChangeState(context, MDNS_STATE_WAITING, 0);
            }
         }
      }
   }
   else if(context->state == MDNS_STATE_WAITING)
   {
      //Set flag
      valid = TRUE;

      //Check current time
      if(timeCompare(time, context->timestamp + MDNS_MAX_WAITING_DELAY) < 0)
      {
#if (IPV4_SUPPORT == ENABLED)
         //Check whether the IPv4 host address is valid
         if(interface->ipv4Context.addrList[0].state != IPV4_ADDR_STATE_VALID)
            valid = FALSE;
#endif

#if (IPV6_SUPPORT == ENABLED)
         //Check whether the IPv6 link-local address is valid
         if(ipv6GetLinkLocalAddrState(interface) != IPV6_ADDR_STATE_PREFERRED)
            valid = FALSE;
#endif
      }

      //Start probing?
      if(valid)
      {
         //Initial random delay
         delay = netGetRandRange(MDNS_RAND_DELAY_MIN, MDNS_RAND_DELAY_MAX);
         //Perform probing
         mdnsResponderChangeState(context, MDNS_STATE_PROBING, delay);
      }
   }
   else if(context->state == MDNS_STATE_PROBING)
   {
      //Probing failed?
      if(context->conflict && context->retransmitCount > 0)
      {
         //Programmatically change the host name
         mdnsResponderChangeHostname(context);
         //Probe again, and repeat as necessary until a unique name is found
         mdnsResponderChangeState(context, MDNS_STATE_PROBING, 0);
      }
      //Tie-break lost?
      else if(context->tieBreakLost && context->retransmitCount > 0)
      {
         //The host defers to the winning host by waiting one second, and
         //then begins probing for this record again
         mdnsResponderChangeState(context, MDNS_STATE_PROBING, MDNS_PROBE_DEFER);
      }
      else
      {
         //Check current time
         if(timeCompare(time, context->timestamp + context->timeout) >= 0)
         {
            //Probing is on-going?
            if(context->retransmitCount < MDNS_PROBE_NUM)
            {
               //First probe?
               if(context->retransmitCount == 0)
               {
                  //Apparently conflicting mDNS responses received before the
                  //first probe packet is sent must be silently ignored
                  context->conflict = FALSE;
                  context->tieBreakLost = FALSE;
               }

               //Send probe packet
               mdnsResponderSendProbe(context);

               //Save the time at which the packet was sent
               context->timestamp = time;
               //Time interval between subsequent probe packets
               context->timeout = MDNS_PROBE_DELAY;
               //Increment retransmission counter
               context->retransmitCount++;
            }
            //Probing is complete?
            else
            {
               //The mDNS responder must send unsolicited mDNS responses
               //containing all of its newly registered resource records
               if(context->settings.numAnnouncements > 0)
                  mdnsResponderChangeState(context, MDNS_STATE_ANNOUNCING, 0);
               else
                  mdnsResponderChangeState(context, MDNS_STATE_IDLE, 0);
            }
         }
      }
   }
   else if(context->state == MDNS_STATE_ANNOUNCING)
   {
      //Whenever a mDNS responder receives any mDNS response (solicited or
      //otherwise) containing a conflicting resource record, the conflict
      //must be resolved
      if(context->conflict)
      {
         //Probe again, and repeat as necessary until a unique name is found
         mdnsResponderChangeState(context, MDNS_STATE_PROBING, 0);
      }
      else
      {
         //Check current time
         if(timeCompare(time, context->timestamp + context->timeout) >= 0)
         {
            //Send announcement packet
            mdnsResponderSendAnnouncement(context);

            //Save the time at which the packet was sent
            context->timestamp = time;
            //Increment retransmission counter
            context->retransmitCount++;

            //First announcement packet?
            if(context->retransmitCount == 1)
            {
               //The mDNS responder must send at least two unsolicited
               //responses, one second apart
               context->timeout = MDNS_ANNOUNCE_DELAY;
            }
            else
            {
               //To provide increased robustness against packet loss, a mDNS
               //responder may send up to eight unsolicited responses, provided
               //that the interval between unsolicited responses increases by
               //at least a factor of two with every response sent
               context->timeout *= 2;
            }

            //Last announcement packet?
            if(context->retransmitCount >= context->settings.numAnnouncements)
            {
               //A mDNS responder must not send regular periodic announcements
               mdnsResponderChangeState(context, MDNS_STATE_IDLE, 0);
            }
         }
      }
   }
   else if(context->state == MDNS_STATE_IDLE)
   {
      //Whenever a mDNS responder receives any mDNS response (solicited or
      //otherwise) containing a conflicting resource record, the conflict
      //must be resolved
      if(context->conflict)
      {
         //Probe again, and repeat as necessary until a unique name is found
         mdnsResponderChangeState(context, MDNS_STATE_PROBING, 0);
      }
   }

#if (IPV4_SUPPORT == ENABLED)
   //Any response message pending to be sent?
   if(context->ipv4Response.buffer != NULL)
   {
      //Check whether the time delay has elapsed
      if(timeCompare(time, context->ipv4Response.timestamp +
         context->ipv4Response.timeout) >= 0)
      {
#if (DNS_SD_SUPPORT == ENABLED)
         //Additional record generation (DNS-SD)
         dnsSdGenerateAdditionalRecords(interface,
            &context->ipv4Response, FALSE);
#endif
         //Additional record generation (mDNS)
         mdnsResponderGenerateAdditionalRecords(interface,
            &context->ipv4Response, FALSE);

         //Use mDNS IPv4 multicast address
         destIpAddr.length = sizeof(Ipv4Addr);
         destIpAddr.ipv4Addr = MDNS_IPV4_MULTICAST_ADDR;

         //Send mDNS response message
         mdnsSendMessage(interface, &context->ipv4Response, &destIpAddr,
            MDNS_PORT);

         //Free previously allocated memory
         mdnsDeleteMessage(&context->ipv4Response);
      }
   }
#endif

#if (IPV6_SUPPORT == ENABLED)
   //Any response message pending to be sent?
   if(context->ipv6Response.buffer != NULL)
   {
      //Check whether the time delay has elapsed
      if(timeCompare(time, context->ipv6Response.timestamp +
         context->ipv6Response.timeout) >= 0)
      {
#if (DNS_SD_SUPPORT == ENABLED)
         //Additional record generation (DNS-SD)
         dnsSdGenerateAdditionalRecords(interface,
            &context->ipv6Response, FALSE);
#endif
         //Additional record generation (mDNS)
         mdnsResponderGenerateAdditionalRecords(interface,
            &context->ipv6Response, FALSE);

         //Use mDNS IPv6 multicast address
         destIpAddr.length = sizeof(Ipv6Addr);
         destIpAddr.ipv6Addr = MDNS_IPV6_MULTICAST_ADDR;

         //Send mDNS response message
         mdnsSendMessage(interface, &context->ipv6Response, &destIpAddr,
            MDNS_PORT);

         //Free previously allocated memory
         mdnsDeleteMessage(&context->ipv6Response);
      }
   }
#endif
}


/**
 * @brief Callback function for link change event
 * @param[in] context Pointer to the mDNS responder context
 **/

void mdnsResponderLinkChangeEvent(MdnsResponderContext *context)
{
   //Make sure the mDNS responder has been properly instantiated
   if(context == NULL)
      return;

#if (IPV4_SUPPORT == ENABLED)
   //Free any response message pending to be sent
   mdnsDeleteMessage(&context->ipv4Response);
#endif

#if (IPV6_SUPPORT == ENABLED)
   //Free any response message pending to be sent
   mdnsDeleteMessage(&context->ipv6Response);
#endif

   //Whenever a mDNS responder receives an indication of a link
   //change event, it must perform probing and announcing
   mdnsResponderChangeState(context, MDNS_STATE_INIT, 0);
}


/**
 * @brief Update FSM state
 * @param[in] context Pointer to the mDNS responder context
 * @param[in] newState New state to switch to
 * @param[in] delay Initial delay
 **/

void mdnsResponderChangeState(MdnsResponderContext *context,
   MdnsState newState, systime_t delay)
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
 * @brief Programmatically change the host name
 * @param[in] context Pointer to the mDNS responder context
 **/

void mdnsResponderChangeHostname(MdnsResponderContext *context)
{
   size_t i;
   size_t m;
   size_t n;
   uint32_t index;
   char_t s[16];

   //Retrieve the length of the string
   n = osStrlen(context->hostname);

   //Parse the string backwards
   for(i = n; i > 0; i--)
   {
      //Check whether the current character is a digit
      if(!osIsdigit(context->hostname[i - 1]))
         break;
   }

   //Any number following the host name?
   if(context->hostname[i] != '\0')
   {
      //Retrieve the number at the end of the name
      index = atoi(context->hostname + i);
      //Increment the value
      index++;

      //Strip the digits
      context->hostname[i] = '\0';
   }
   else
   {
      //Append the digit "2" to the name
      index = 2;
   }

   //Convert the number to a string of characters
   m = osSprintf(s, "%" PRIu32, index);

   //Sanity check
   if((i + m) <= NET_MAX_HOSTNAME_LEN)
   {
      //Add padding if necessary
      while((i + m) < n)
         context->hostname[i++] = '0';

      //Properly terminate the string
      context->hostname[i] = '\0';
      //Programmatically change the host name
      osStrcat(context->hostname, s);
   }
}


/**
 * @brief Send probe packet
 * @param[in] context Pointer to the mDNS responder context
 * @return Error code
 **/

error_t mdnsResponderSendProbe(MdnsResponderContext *context)
{
   error_t error;
   NetInterface *interface;
   DnsQuestion *dnsQuestion;
   MdnsMessage message;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Create an empty mDNS query message
   error = mdnsCreateMessage(&message, FALSE);
   //Any error to report?
   if(error)
      return error;

   //Start of exception handling block
   do
   {
      //Encode the host name using the DNS name notation
      message.length += mdnsEncodeName(context->hostname, "",
         ".local", message.dnsHeader->questions);

      //Point to the corresponding question structure
      dnsQuestion = DNS_GET_QUESTION(message.dnsHeader, message.length);

      //The probes should be sent as QU questions with the unicast-response
      //bit set, to allow a defending host to respond immediately via unicast
      dnsQuestion->qtype = HTONS(DNS_RR_TYPE_ANY);
      dnsQuestion->qclass = HTONS(MDNS_QCLASS_QU | DNS_RR_CLASS_IN);

      //Update the length of the mDNS query message
      message.length += sizeof(DnsQuestion);

      //Format A resource record
      error = mdnsResponderAddIpv4AddrRecord(interface,
         &message, FALSE, MDNS_DEFAULT_RR_TTL);
      //Any error to report?
      if(error)
         break;

      //Format AAAA resource record
      error = mdnsResponderAddIpv6AddrRecord(interface,
         &message, FALSE, MDNS_DEFAULT_RR_TTL);
      //Any error to report?
      if(error)
         break;

      //Number of questions in the Question Section
      message.dnsHeader->qdcount = 1;
      //Number of resource records in the Authority Section
      message.dnsHeader->nscount = message.dnsHeader->ancount;
      //Number of resource records in the Answer Section
      message.dnsHeader->ancount = 0;

      //Send mDNS message
      error = mdnsSendMessage(interface, &message, NULL, MDNS_PORT);

      //End of exception handling block
   } while(0);

   //Free previously allocated memory
   mdnsDeleteMessage(&message);

   //Return status code
   return error;
}


/**
 * @brief Send announcement packet
 * @param[in] context Pointer to the mDNS responder context
 * @return Error code
 **/

error_t mdnsResponderSendAnnouncement(MdnsResponderContext *context)
{
   error_t error;
   NetInterface *interface;
   MdnsMessage message;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Create an empty mDNS response message
   error = mdnsCreateMessage(&message, TRUE);
   //Any error to report?
   if(error)
      return error;

   //Start of exception handling block
   do
   {
      //Format A resource record
      error = mdnsResponderAddIpv4AddrRecord(interface,
         &message, TRUE, MDNS_DEFAULT_RR_TTL);
      //Any error to report?
      if(error)
         break;

      //Format reverse address mapping PTR record (IPv4)
      error = mdnsResponderAddIpv4ReversePtrRecord(interface,
         &message, TRUE, MDNS_DEFAULT_RR_TTL);
      //Any error to report?
      if(error)
         break;

      //Format AAAA resource record
      error = mdnsResponderAddIpv6AddrRecord(interface,
         &message, TRUE, MDNS_DEFAULT_RR_TTL);
      //Any error to report?
      if(error)
         break;

      //Format reverse address mapping PTR record (IPv6)
      error = mdnsResponderAddIpv6ReversePtrRecord(interface,
         &message, TRUE, MDNS_DEFAULT_RR_TTL);
      //Any error to report?
      if(error)
         break;

      //Send mDNS message
      error = mdnsSendMessage(interface, &message, NULL, MDNS_PORT);

      //End of exception handling block
   } while(0);

   //Free previously allocated memory
   mdnsDeleteMessage(&message);

   //Return status code
   return error;
}


/**
 * @brief Send goodbye packet
 * @param[in] context Pointer to the mDNS responder context
 * @return Error code
 **/

error_t mdnsResponderSendGoodbye(MdnsResponderContext *context)
{
   error_t error;
   NetInterface *interface;
   MdnsMessage message;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Create an empty mDNS response message
   error = mdnsCreateMessage(&message, TRUE);
   //Any error to report?
   if(error)
      return error;

   //Start of exception handling block
   do
   {
      //Format A resource record
      error = mdnsResponderAddIpv4AddrRecord(interface, &message, TRUE, 0);
      //Any error to report?
      if(error)
         break;

      //Format reverse address mapping PTR record (IPv4)
      error = mdnsResponderAddIpv4ReversePtrRecord(interface, &message, TRUE, 0);
      //Any error to report?
      if(error)
         break;

      //Format AAAA resource record
      error = mdnsResponderAddIpv6AddrRecord(interface, &message, TRUE, 0);
      //Any error to report?
      if(error)
         break;

      //Format reverse address mapping PTR record (IPv6)
      error = mdnsResponderAddIpv6ReversePtrRecord(interface, &message, TRUE, 0);
      //Any error to report?
      if(error)
         break;

      //Send mDNS message
      error = mdnsSendMessage(interface, &message, NULL, MDNS_PORT);

      //End of exception handling block
   } while(0);

   //Free previously allocated memory
   mdnsDeleteMessage(&message);

   //Return status code
   return error;
}


/**
 * @brief Process mDNS query message
 * @param[in] interface Underlying network interface
 * @param[in] query Incoming mDNS query message
 **/

void mdnsResponderProcessQuery(NetInterface *interface, MdnsMessage *query)
{
   error_t error;
   uint_t i;
   size_t n;
   size_t offset;
   DnsQuestion *question;
   DnsResourceRecord *record;
   MdnsResponderContext *context;
   MdnsMessage *response;
   uint16_t destPort;
   IpAddr destIpAddr;

   //Point to the mDNS responder context
   context = interface->mdnsResponderContext;
   //Make sure the mDNS responder has been properly instantiated
   if(context == NULL)
      return;

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 query received?
   if(query->pseudoHeader->length == sizeof(Ipv4PseudoHeader))
   {
      //If the source UDP port in a received Multicast DNS query is not port 5353,
      //this indicates that the querier originating the query is a simple resolver
      if(ntohs(query->udpHeader->srcPort) != MDNS_PORT)
      {
         //The mDNS responder must send a UDP response directly back to the querier,
         //via unicast, to the query packet's source IP address and port
         destIpAddr.length = sizeof(Ipv4Addr);
         destIpAddr.ipv4Addr = query->pseudoHeader->ipv4Data.srcAddr;
      }
      else
      {
         //Use mDNS IPv4 multicast address
         destIpAddr.length = sizeof(Ipv4Addr);
         destIpAddr.ipv4Addr = MDNS_IPV4_MULTICAST_ADDR;
      }

      //Point to the mDNS response message
      response = &context->ipv4Response;
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 query received?
   if(query->pseudoHeader->length == sizeof(Ipv6PseudoHeader))
   {
      //If the source UDP port in a received Multicast DNS query is not port 5353,
      //this indicates that the querier originating the query is a simple resolver
      if(ntohs(query->udpHeader->srcPort) != MDNS_PORT)
      {
         //The mDNS responder must send a UDP response directly back to the querier,
         //via unicast, to the query packet's source IP address and port
         destIpAddr.length = sizeof(Ipv6Addr);
         destIpAddr.ipv6Addr = query->pseudoHeader->ipv6Data.srcAddr;
      }
      else
      {
         //Use mDNS IPv6 multicast address
         destIpAddr.length = sizeof(Ipv6Addr);
         destIpAddr.ipv6Addr = MDNS_IPV6_MULTICAST_ADDR;
      }

      //Point to the mDNS response message
      response = &context->ipv6Response;
   }
   else
#endif
   //Invalid query received?
   {
      //Discard the mDNS query message
      return;
   }

   //When possible, a responder should, for the sake of network
   //efficiency, aggregate as many responses as possible into a
   //single mDNS response message
   if(response->buffer == NULL)
   {
      //Create an empty mDNS response message
      error = mdnsCreateMessage(response, TRUE);
      //Any error to report?
      if(error)
         return;
   }

   //Take the identifier from the query message
   response->dnsHeader->id = query->dnsHeader->id;

   //Point to the first question
   offset = sizeof(DnsHeader);

   //Start of exception handling block
   do
   {
      //Parse the Question Section
      for(i = 0; i < ntohs(query->dnsHeader->qdcount); i++)
      {
         //Parse resource record name
         n = dnsParseName(query->dnsHeader, query->length, offset, NULL, 0);
         //Invalid name?
         if(!n)
            break;
         //Malformed mDNS message?
         if((n + sizeof(DnsQuestion)) > query->length)
            break;

         //Point to the corresponding entry
         question = DNS_GET_QUESTION(query->dnsHeader, n);

         //Parse question
         error = mdnsResponderParseQuestion(interface, query,
            offset, question, response);
         //Any error to report?
         if(error)
            break;

#if (DNS_SD_SUPPORT == ENABLED)
         //Parse resource record
         error = dnsSdParseQuestion(interface, query, offset,
            question, response);
         //Any error to report?
         if(error)
            break;
#endif
         //Point to the next question
         offset = n + sizeof(DnsQuestion);
      }

      //Any error while parsing the Question Section?
      if(i != ntohs(query->dnsHeader->qdcount))
         break;

      //Parse the Known-Answer Section
      for(i = 0; i < ntohs(query->dnsHeader->ancount); i++)
      {
         //Parse resource record name
         n = dnsParseName(query->dnsHeader, query->length, offset, NULL, 0);
         //Invalid name?
         if(!n)
            break;

         //Point to the associated resource record
         record = DNS_GET_RESOURCE_RECORD(query->dnsHeader, n);
         //Point to the resource data
         n += sizeof(DnsResourceRecord);

         //Make sure the resource record is valid
         if(n > query->length)
            break;
         if((n + ntohs(record->rdlength)) > query->length)
            break;

         //Parse resource record
         mdnsResponderParseKnownAnRecord(interface, query, offset,
            record, response);

         //Point to the next resource record
         offset = n + ntohs(record->rdlength);
      }

      //Any error while parsing the Answer Section?
      if(i != ntohs(query->dnsHeader->ancount))
         break;

      //Parse Authority Section
      for(i = 0; i < ntohs(query->dnsHeader->nscount); i++)
      {
         //Parse resource record name
         n = dnsParseName(query->dnsHeader, query->length, offset, NULL, 0);
         //Invalid name?
         if(!n)
            break;

         //Point to the associated resource record
         record = DNS_GET_RESOURCE_RECORD(query->dnsHeader, n);
         //Point to the resource data
         n += sizeof(DnsResourceRecord);

         //Make sure the resource record is valid
         if(n > query->length)
            break;
         if((n + ntohs(record->rdlength)) > query->length)
            break;

         //Check for host name conflict
         mdnsResponderParseNsRecord(interface, query, offset, record);

#if (DNS_SD_SUPPORT == ENABLED)
         //Check for service instance name conflict
         dnsSdParseNsRecord(interface, query, offset, record);
#endif
         //Point to the next resource record
         offset = n + ntohs(record->rdlength);
      }

      //Any error while parsing the Authority Section?
      if(i != ntohs(query->dnsHeader->nscount))
         break;

      //End of exception handling block
   } while(0);

   //Should a mDNS message be send in response to the query?
   if(response->dnsHeader->ancount > 0)
   {
      //If the source UDP port in a received Multicast DNS query is not port 5353,
      //this indicates that the querier originating the query is a simple resolver
      if(ntohs(query->udpHeader->srcPort) != MDNS_PORT)
      {
#if (DNS_SD_SUPPORT == ENABLED)
         //Additional record generation (DNS-SD)
         dnsSdGenerateAdditionalRecords(interface, response, TRUE);
#endif
         //Additional record generation (mDNS)
         mdnsResponderGenerateAdditionalRecords(interface, response, TRUE);

         //Destination port
         destPort = ntohs(query->udpHeader->srcPort);

         //Send mDNS response message
         mdnsSendMessage(interface, response, &destIpAddr, destPort);
         //Free previously allocated memory
         mdnsDeleteMessage(response);
      }
      else
      {
         //Check whether the answer should be delayed
         if(query->dnsHeader->tc)
         {
            //In the case where the query has the TC (truncated) bit set, indicating
            //that subsequent Known-Answer packets will follow, responders should
            //delay their responses by a random amount of time selected with uniform
            //random distribution in the range 400-500 ms
            response->timeout = netGetRandRange(400, 500);

            //Save current time
            response->timestamp = osGetSystemTime();
         }
         else if(response->sharedRecordCount > 0)
         {
            //In any case where there may be multiple responses, such as queries
            //where the answer is a member of a shared resource record set, each
            //responder should delay its response by a random amount of time
            //selected with uniform random distribution in the range 20-120 ms
            response->timeout = netGetRandRange(20, 120);

            //Save current time
            response->timestamp = osGetSystemTime();
         }
         else
         {
#if (DNS_SD_SUPPORT == ENABLED)
            //Additional record generation (refer to RFC 6763 section 12)
            dnsSdGenerateAdditionalRecords(interface, response, FALSE);
#endif
            //Additional record generation (mDNS)
            mdnsResponderGenerateAdditionalRecords(interface, response, FALSE);

            //Send mDNS response message
            mdnsSendMessage(interface, response, &destIpAddr, MDNS_PORT);
            //Free previously allocated memory
            mdnsDeleteMessage(response);
         }
      }
   }
   else
   {
      //Free mDNS response message
      mdnsDeleteMessage(response);
   }
}


/**
 * @brief Parse a question
 * @param[in] interface Underlying network interface
 * @param[in] query Incoming mDNS query message
 * @param[in] offset Offset to first byte of the question
 * @param[in] question Pointer to the question
 * @param[in,out] response mDNS response message
 * @return Error code
 **/

error_t mdnsResponderParseQuestion(NetInterface *interface, const MdnsMessage *query,
   size_t offset, const DnsQuestion *question, MdnsMessage *response)
{
   error_t error;
   uint16_t qclass;
   uint16_t qtype;
   uint32_t ttl;
   bool_t cacheFlush;
   MdnsResponderContext *context;

   //Point to the mDNS responder context
   context = interface->mdnsResponderContext;

   //Check the state of the mDNS responder
   if(context->state != MDNS_STATE_ANNOUNCING &&
      context->state != MDNS_STATE_IDLE)
   {
      //Do not respond to mDNS queries during probing
      return NO_ERROR;
   }

   //Convert the query class to host byte order
   qclass = ntohs(question->qclass);
   //Discard QU flag
   qclass &= ~MDNS_QCLASS_QU;

   //Convert the query type to host byte order
   qtype = ntohs(question->qtype);

   //Get the TTL resource record
   ttl = context->settings.ttl;

   //Check whether the querier originating the query is a simple resolver
   if(ntohs(query->udpHeader->srcPort) != MDNS_PORT)
   {
      //The resource record TTL given in a legacy unicast response should
      //not be greater than ten seconds, even if the true TTL of the mDNS
      //resource record is higher
      ttl = MIN(ttl, MDNS_LEGACY_UNICAST_RR_TTL);

      //The cache-flush bit must not be set in legacy unicast responses
      cacheFlush = FALSE;
   }
   else
   {
      //The cache-bit should be set for unique resource records
      cacheFlush = TRUE;
   }

   //Check the class of the query
   if(qclass == DNS_RR_CLASS_IN || qclass == DNS_RR_CLASS_ANY)
   {
      //Compare domain name
      if(!mdnsCompareName(query->dnsHeader, query->length,
         offset, context->hostname, "", ".local", 0))
      {
#if (IPV4_SUPPORT == ENABLED)
         //A query?
         if(qtype == DNS_RR_TYPE_A)
         {
            //Format A resource record
            error = mdnsResponderAddIpv4AddrRecord(interface,
               response, cacheFlush, ttl);
            //Any error to report?
            if(error)
               return error;
         }
         else
#endif
#if (IPV6_SUPPORT == ENABLED)
         //AAAA query?
         if(qtype == DNS_RR_TYPE_AAAA)
         {
            //Format AAAA resource record
            error = mdnsResponderAddIpv6AddrRecord(interface,
               response, cacheFlush, ttl);
            //Any error to report?
            if(error)
               return error;
         }
         else
#endif
         //ANY query?
         if(qtype == DNS_RR_TYPE_ANY)
         {
            //Format A resource record
            error = mdnsResponderAddIpv4AddrRecord(interface,
               response, cacheFlush, ttl);
            //Any error to report?
            if(error)
               return error;

            //Format AAAA resource record
            error = mdnsResponderAddIpv6AddrRecord(interface,
               response, cacheFlush, ttl);
            //Any error to report?
            if(error)
               return error;

            //Format NSEC resource record
            error = mdnsResponderAddNsecRecord(interface,
               response, cacheFlush, ttl);
            //Any error to report?
            if(error)
               return error;
         }
         else
         {
            //Format NSEC resource record
            error = mdnsResponderAddNsecRecord(interface,
               response, cacheFlush, ttl);
            //Any error to report?
            if(error)
               return error;
         }
      }

#if (IPV4_SUPPORT == ENABLED)
      //Reverse DNS lookup?
      if(!mdnsCompareName(query->dnsHeader, query->length,
         offset, context->ipv4ReverseName, "in-addr", ".arpa", 0))
      {
         //PTR query?
         if(qtype == DNS_RR_TYPE_PTR || qtype == DNS_RR_TYPE_ANY)
         {
            //Format reverse address mapping PTR record (IPv4)
            error = mdnsResponderAddIpv4ReversePtrRecord(interface,
               response, cacheFlush, ttl);
            //Any error to report?
            if(error)
               return error;
         }
      }
#endif
#if (IPV6_SUPPORT == ENABLED)
      //Reverse DNS lookup?
      if(!mdnsCompareName(query->dnsHeader, query->length,
         offset, context->ipv6ReverseName, "ip6", ".arpa", 0))
      {
         //PTR query?
         if(qtype == DNS_RR_TYPE_PTR || qtype == DNS_RR_TYPE_ANY)
         {
            //Format reverse address mapping PTR record (IPv6)
            error = mdnsResponderAddIpv6ReversePtrRecord(interface,
               response, cacheFlush, ttl);
            //Any error to report?
            if(error)
               return error;
         }
      }
#endif
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse a resource record from the Known-Answer Section
 * @param[in] interface Underlying network interface
 * @param[in] query Incoming mDNS query message
 * @param[in] queryOffset Offset to first byte of the resource record
 * @param[in] queryRecord Pointer to the resource record
 * @param[in,out] response mDNS response message
 **/

void mdnsResponderParseKnownAnRecord(NetInterface *interface, const MdnsMessage *query,
   size_t queryOffset, const DnsResourceRecord *queryRecord, MdnsMessage *response)
{
   size_t i;
   size_t n;
   size_t responseOffset;
   DnsResourceRecord *responseRecord;

   //mDNS responses must not contain any questions in the Question Section
   if(response->dnsHeader->qdcount == 0)
   {
      //Point to the first resource record
      responseOffset = sizeof(DnsHeader);

      //Parse the Answer Section of the response
      for(i = 0; i < response->dnsHeader->ancount; i++)
      {
         //Parse resource record name
         n = dnsParseName(response->dnsHeader, response->length, responseOffset, NULL, 0);
         //Invalid name?
         if(!n)
            break;

         //Point to the associated resource record
         responseRecord = DNS_GET_RESOURCE_RECORD(response->dnsHeader, n);
         //Point to the resource data
         n += sizeof(DnsResourceRecord);

         //Make sure the resource record is valid
         if(n > response->length)
            break;

         //Point to the end of the resource record
         n += ntohs(responseRecord->rdlength);

         //Make sure the resource record is valid
         if(n > response->length)
            break;

         //Compare resource record names
         if(!dnsCompareEncodedName(query->dnsHeader, query->length, queryOffset,
            response->dnsHeader, response->length, responseOffset, 0))
         {
            //Compare the contents of the resource records
            if(!mdnsCompareRecord(query, queryOffset, queryRecord,
               response, responseOffset, responseRecord))
            {
               //A mDNS responder must not answer a mDNS query if the answer
               //it would give is already included in the Answer Section with
               //an RR TTL at least half the correct value
               if(ntohl(queryRecord->ttl) >= (ntohl(responseRecord->ttl) / 2))
               {
                  //Perform Known-Answer Suppression
                  osMemmove((uint8_t *) response->dnsHeader + responseOffset,
                     (uint8_t *) response->dnsHeader + n, response->length - n);

                  //Update the length of the mDNS response message
                  response->length -= (n - responseOffset);
                  //Update the number of resource records in the Answer Section
                  response->dnsHeader->ancount--;

                  //Keep at the same position
                  n = responseOffset;
                  i--;
               }
            }
         }

         //Point to the next resource record
         responseOffset = n;
      }
   }
}


/**
 * @brief Parse a resource record from the Authority Section
 * @param[in] interface Underlying network interface
 * @param[in] query Incoming mDNS query message
 * @param[in] offset Offset to first byte of the resource record
 * @param[in] record Pointer to the resource record
 **/

void mdnsResponderParseNsRecord(NetInterface *interface,
   const MdnsMessage *query, size_t offset, const DnsResourceRecord *record)
{
   bool_t tieBreakLost;
   uint16_t rclass;
   MdnsResponderContext *context;

   //Point to the mDNS responder context
   context = interface->mdnsResponderContext;

   //When a host that is probing for a record sees another host issue a query
   //for the same record, it consults the Authority Section of that query.
   //If it finds any resource record there which answers the query, then it
   //compares the data of that resource record with its own tentative data
   if(!mdnsCompareName(query->dnsHeader, query->length,
      offset, context->hostname, "", ".local", 0))
   {
      //Convert the class to host byte order
      rclass = ntohs(record->rclass);
      //Discard Cache Flush flag
      rclass &= ~MDNS_RCLASS_CACHE_FLUSH;

      //Check the class of the resource record
      if(rclass == DNS_RR_CLASS_IN)
      {
#if (IPV4_SUPPORT == ENABLED)
         //A resource record found?
         if(ntohs(record->rtype) == DNS_RR_TYPE_A)
         {
            //Apply tie-breaking rules
            tieBreakLost = TRUE;

            //Verify the length of the data field
            if(ntohs(record->rdlength) == sizeof(Ipv4Addr))
            {
               //Valid host address?
               if(interface->ipv4Context.addrList[0].state == IPV4_ADDR_STATE_VALID)
               {
                  //The two records are compared and the lexicographically
                  //later data wins
                  if(osMemcmp(&interface->ipv4Context.addrList[0].addr, record->rdata,
                     sizeof(Ipv4Addr)) >= 0)
                  {
                     tieBreakLost = FALSE;
                  }
               }
            }

            //Check whether the host has lost the tie-break
            if(tieBreakLost)
               context->tieBreakLost = TRUE;
         }
#endif
#if (IPV6_SUPPORT == ENABLED)
         //AAAA resource record found?
         if(ntohs(record->rtype) == DNS_RR_TYPE_AAAA)
         {
            //Apply tie-breaking rules
            tieBreakLost = TRUE;

            //Verify the length of the data field
            if(ntohs(record->rdlength) == sizeof(Ipv6Addr))
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
                     //The two records are compared and the lexicographically
                     //later data wins
                     if(osMemcmp(&interface->ipv6Context.addrList[i].addr,
                        record->rdata, sizeof(Ipv6Addr)) >= 0)
                     {
                        tieBreakLost = FALSE;
                     }
                  }
               }
            }

            //Check whether the host has lost the tie-break
            if(tieBreakLost)
               context->tieBreakLost = TRUE;
         }
#endif
      }
   }
}


/**
 * @brief Parse a resource record from the Answer Section
 * @param[in] interface Underlying network interface
 * @param[in] response Incoming mDNS response message
 * @param[in] offset Offset to first byte of the resource record to be checked
 * @param[in] record Pointer to the resource record
 **/

void mdnsResponderParseAnRecord(NetInterface *interface,
   const MdnsMessage *response, size_t offset, const DnsResourceRecord *record)
{
   bool_t conflict;
   uint16_t rclass;
   MdnsResponderContext *context;

   //Point to the mDNS responder context
   context = interface->mdnsResponderContext;

   //Check for conflicts
   if(!mdnsCompareName(response->dnsHeader, response->length,
      offset, context->hostname, "", ".local", 0))
   {
      //Convert the class to host byte order
      rclass = ntohs(record->rclass);
      //Discard Cache Flush flag
      rclass &= ~MDNS_RCLASS_CACHE_FLUSH;

      //Check the class of the resource record
      if(rclass == DNS_RR_CLASS_IN)
      {
#if (IPV4_SUPPORT == ENABLED)
         //A resource record found?
         if(ntohs(record->rtype) == DNS_RR_TYPE_A)
         {
            //A conflict occurs when a mDNS responder has a unique record for
            //which it is currently authoritative, and it receives a mDNS
            //response message containing a record with the same name, rrtype
            //and rrclass, but inconsistent rdata
            conflict = TRUE;

            //Verify the length of the data field
            if(ntohs(record->rdlength) == sizeof(Ipv4Addr))
            {
               //Valid host address?
               if(interface->ipv4Context.addrList[0].state == IPV4_ADDR_STATE_VALID)
               {
                  //Check whether the rdata field is consistent
                  if(ipv4CompAddr(&interface->ipv4Context.addrList[0].addr, record->rdata))
                     context->conflict = FALSE;
               }
            }

            //Check whether the hostname is already in use by some other host
            if(conflict)
               context->conflict = TRUE;
         }
#endif
#if (IPV6_SUPPORT == ENABLED)
         //AAAA resource record found?
         if(ntohs(record->rtype) == DNS_RR_TYPE_AAAA)
         {
            //A conflict occurs when a mDNS responder has a unique record for
            //which it is currently authoritative, and it receives a mDNS
            //response message containing a record with the same name, rrtype
            //and rrclass, but inconsistent rdata
            conflict = TRUE;

            //Verify the length of the data field
            if(ntohs(record->rdlength) == sizeof(Ipv6Addr))
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
                     //Check whether the rdata field is consistent
                     if(ipv6CompAddr(&interface->ipv6Context.addrList[i].addr, record->rdata))
                        conflict = FALSE;
                  }
               }
            }

            //Check whether the hostname is already in use by some other host
            if(conflict)
               context->conflict = TRUE;
         }
#endif
      }
   }
}


/**
 * @brief Additional record generation
 * @param[in] interface Underlying network interface
 * @param[in,out] response mDNS response message
 * @param[in] legacyUnicast This flag is set for legacy unicast responses
 **/

void mdnsResponderGenerateAdditionalRecords(NetInterface *interface,
   MdnsMessage *response, bool_t legacyUnicast)
{
   error_t error;
   uint_t i;
   uint_t k;
   size_t n;
   size_t offset;
   uint_t ancount;
   uint16_t rclass;
   uint32_t ttl;
   bool_t cacheFlush;
   MdnsResponderContext *context;
   DnsResourceRecord *record;

   //Point to the mDNS responder context
   context = interface->mdnsResponderContext;

   //mDNS responses must not contain any questions in the Question Section
   if(response->dnsHeader->qdcount != 0)
      return;

   //Get the TTL resource record
   ttl = context->settings.ttl;

   //Check whether the querier originating the query is a simple resolver
   if(legacyUnicast)
   {
      //The resource record TTL given in a legacy unicast response should
      //not be greater than ten seconds, even if the true TTL of the mDNS
      //resource record is higher
      ttl = MIN(ttl, MDNS_LEGACY_UNICAST_RR_TTL);

      //The cache-flush bit must not be set in legacy unicast responses
      cacheFlush = FALSE;
   }
   else
   {
      //The cache-bit should be set for unique resource records
      cacheFlush = TRUE;
   }

   //Point to the first resource record
   offset = sizeof(DnsHeader);

   //Save the number of resource records in the Answer Section
   ancount = response->dnsHeader->ancount;

   //Compute the total number of resource records
   k = response->dnsHeader->ancount + response->dnsHeader->nscount +
      response->dnsHeader->arcount;

   //Loop through the resource records
   for(i = 0; i < k; i++)
   {
      //Parse resource record name
      n = dnsParseName(response->dnsHeader, response->length, offset, NULL, 0);
      //Invalid name?
      if(!n)
         break;

      //Point to the associated resource record
      record = DNS_GET_RESOURCE_RECORD(response->dnsHeader, n);
      //Point to the resource data
      n += sizeof(DnsResourceRecord);

      //Make sure the resource record is valid
      if(n > response->length)
         break;
      if((n + ntohs(record->rdlength)) > response->length)
         break;

      //Convert the record class to host byte order
      rclass = ntohs(record->rclass);
      //Discard the cache-flush bit
      rclass &= ~MDNS_RCLASS_CACHE_FLUSH;

      //Check the class of the resource record
      if(rclass == DNS_RR_CLASS_IN)
      {
         //A record?
         if(ntohs(record->rtype) == DNS_RR_TYPE_A)
         {
#if (IPV6_SUPPORT == ENABLED)
            //When a mDNS responder places an IPv4 address record into a
            //response message, it should also place any IPv6 address records
            //with the same name into the Additional Section
            error = mdnsResponderAddIpv6AddrRecord(interface,
               response, cacheFlush, ttl);
#else
            //In the event that a device has only IPv4 addresses but no IPv6
            //addresses, then the appropriate NSEC record should be placed
            //into the Additional Section
            error = mdnsResponderAddNsecRecord(interface,
               response, cacheFlush, ttl);
#endif
            //Any error to report?
            if(error)
               return;
         }
         //AAAA record?
         else if(ntohs(record->rtype) == DNS_RR_TYPE_AAAA)
         {
#if (IPV4_SUPPORT == ENABLED)
            //When a mDNS responder places an IPv6 address record into a
            //response message, it should also place any IPv4 address records
            //with the same name into the Additional Section
            error = mdnsResponderAddIpv4AddrRecord(interface,
               response, cacheFlush, ttl);
#else
            //In the event that a device has only IPv6 addresses but no IPv4
            //addresses, then the appropriate NSEC record should be placed
            //into the Additional Section
            error = mdnsResponderAddNsecRecord(interface,
               response, cacheFlush, ttl);
#endif
            //Any error to report?
            if(error)
               return;
         }
         //SRV record?
         else if(ntohs(record->rtype) == DNS_RR_TYPE_SRV)
         {
            //Format A resource record
            error = mdnsResponderAddIpv4AddrRecord(interface,
               response, cacheFlush, ttl);
            //Any error to report?
            if(error)
               return;

            //Format AAAA resource record
            error = mdnsResponderAddIpv6AddrRecord(interface,
               response, cacheFlush, ttl);
            //Any error to report?
            if(error)
               return;

#if (IPV4_SUPPORT == DISABLED || IPV6_SUPPORT == DISABLED)
            //In the event that a device has only IPv4 addresses but no IPv6
            //addresses, or vice versa, then the appropriate NSEC record should
            //be placed into the additional section, so that queriers can know
            //with certainty that the device has no addresses of that kind
            error = mdnsResponderAddNsecRecord(interface,
               response, cacheFlush, ttl);
            //Any error to report?
            if(error)
               return;
#endif
         }
      }

      //Point to the next resource record
      offset = n + ntohs(record->rdlength);
   }

   //Number of resource records in the Additional Section
   response->dnsHeader->arcount += response->dnsHeader->ancount - ancount;
   //Number of resource records in the Answer Section
   response->dnsHeader->ancount = ancount;
}


/**
 * @brief Add A record to a mDNS message
 * @param[in] interface Underlying network interface
 * @param[in,out] message Pointer to the mDNS message
 * @param[in] cacheFlush Cache-flush bit
 * @param[in] ttl Resource record TTL (cache lifetime)
 * @return Error code
 **/

error_t mdnsResponderAddIpv4AddrRecord(NetInterface *interface,
   MdnsMessage *message, bool_t cacheFlush, uint32_t ttl)
{
#if (IPV4_SUPPORT == ENABLED)
   size_t n;
   size_t offset;
   bool_t duplicate;
   MdnsResponderContext *context;
   DnsResourceRecord *record;

   //Point to the mDNS responder context
   context = interface->mdnsResponderContext;

   //Valid IPv4 host address?
   if(interface->ipv4Context.addrList[0].state == IPV4_ADDR_STATE_VALID)
   {
      //Check whether the resource record is already present in the Answer
      //Section of the message
      duplicate = mdnsCheckDuplicateRecord(message, context->hostname,
         "", ".local", DNS_RR_TYPE_A);

      //The duplicates should be suppressed and the resource record should
      //appear only once in the list
      if(!duplicate)
      {
         //Set the position to the end of the buffer
         offset = message->length;

         //The first pass calculates the length of the DNS encoded host name
         n = mdnsEncodeName(context->hostname, "", ".local", NULL);

         //Check the length of the resulting mDNS message
         if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
            return ERROR_MESSAGE_TOO_LONG;

         //The second pass encodes the host name using the DNS name notation
         offset += mdnsEncodeName(context->hostname, "", ".local",
            (uint8_t *) message->dnsHeader + offset);

         //Consider the length of the resource record itself
         n = sizeof(DnsResourceRecord) + sizeof(Ipv4Addr);

         //Check the length of the resulting mDNS message
         if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
            return ERROR_MESSAGE_TOO_LONG;

         //Point to the corresponding resource record
         record = DNS_GET_RESOURCE_RECORD(message->dnsHeader, offset);

         //Fill in resource record
         record->rtype = HTONS(DNS_RR_TYPE_A);
         record->rclass = HTONS(DNS_RR_CLASS_IN);
         record->ttl = htonl(ttl);
         record->rdlength = HTONS(sizeof(Ipv4Addr));

         //Check whether the cache-flush bit should be set
         if(cacheFlush)
            record->rclass |= HTONS(MDNS_RCLASS_CACHE_FLUSH);

         //Copy IPv4 address
         ipv4CopyAddr(record->rdata, &interface->ipv4Context.addrList[0].addr);

         //Number of resource records in the answer section
         message->dnsHeader->ancount++;
         //Update the length of the mDNS response message
         message->length = offset + n;
      }
   }
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add AAAA record to a mDNS message
 * @param[in] interface Underlying network interface
 * @param[in,out] message Pointer to the mDNS message
 * @param[in] cacheFlush Cache-flush bit
 * @param[in] ttl Resource record TTL (cache lifetime)
 * @return Error code
 **/

error_t mdnsResponderAddIpv6AddrRecord(NetInterface *interface,
   MdnsMessage *message, bool_t cacheFlush, uint32_t ttl)
{
#if (IPV6_SUPPORT == ENABLED)
   size_t n;
   size_t offset;
   bool_t duplicate;
   MdnsResponderContext *context;
   DnsResourceRecord *record;

   //Point to the mDNS responder context
   context = interface->mdnsResponderContext;

   //Valid IPv6 link-local address?
   if(ipv6GetLinkLocalAddrState(interface) == IPV6_ADDR_STATE_PREFERRED)
   {
      //Check whether the resource record is already present in the Answer
      //Section of the message
      duplicate = mdnsCheckDuplicateRecord(message, context->hostname,
         "", ".local", DNS_RR_TYPE_AAAA);

      //The duplicates should be suppressed and the resource record should
      //appear only once in the list
      if(!duplicate)
      {
         //Set the position to the end of the buffer
         offset = message->length;

         //The first pass calculates the length of the DNS encoded host name
         n = mdnsEncodeName(context->hostname, "", ".local", NULL);

         //Check the length of the resulting mDNS message
         if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
            return ERROR_MESSAGE_TOO_LONG;

         //The second pass encodes the host name using the DNS name notation
         offset += mdnsEncodeName(context->hostname, "", ".local",
            (uint8_t *) message->dnsHeader + offset);

         //Consider the length of the resource record itself
         n = sizeof(DnsResourceRecord) + sizeof(Ipv6Addr);

         //Check the length of the resulting mDNS message
         if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
            return ERROR_MESSAGE_TOO_LONG;

         //Point to the corresponding resource record
         record = DNS_GET_RESOURCE_RECORD(message->dnsHeader, offset);

         //Fill in resource record
         record->rtype = HTONS(DNS_RR_TYPE_AAAA);
         record->rclass = HTONS(DNS_RR_CLASS_IN);
         record->ttl = htonl(ttl);
         record->rdlength = HTONS(sizeof(Ipv6Addr));

         //Check whether the cache-flush bit should be set
         if(cacheFlush)
            record->rclass |= HTONS(MDNS_RCLASS_CACHE_FLUSH);

         //Copy IPv6 address
         ipv6CopyAddr(record->rdata, &interface->ipv6Context.addrList[0].addr);

         //Number of resource records in the answer section
         message->dnsHeader->ancount++;
         //Update the length of the mDNS response message
         message->length = offset + n;
      }
   }
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add reverse address mapping PTR record (IPv4)
 * @param[in] interface Underlying network interface
 * @param[in,out] message Pointer to the mDNS message
 * @param[in] cacheFlush Cache-flush bit
 * @param[in] ttl Resource record TTL (cache lifetime)
 * @return Error code
 **/

error_t mdnsResponderAddIpv4ReversePtrRecord(NetInterface *interface,
   MdnsMessage *message, bool_t cacheFlush, uint32_t ttl)
{
#if (IPV4_SUPPORT == ENABLED)
   size_t n;
   size_t offset;
   bool_t duplicate;
   MdnsResponderContext *context;
   DnsResourceRecord *record;

   //Point to the mDNS responder context
   context = interface->mdnsResponderContext;

   //Valid reverse name?
   if(context->ipv4ReverseName[0] != '\0')
   {
      //Check whether the resource record is already present in the Answer
      //Section of the message
      duplicate = mdnsCheckDuplicateRecord(message, context->ipv4ReverseName,
         "in-addr", ".arpa", DNS_RR_TYPE_PTR);

      //The duplicates should be suppressed and the resource record should
      //appear only once in the list
      if(!duplicate)
      {
         //Set the position to the end of the buffer
         offset = message->length;

         //The first pass calculates the length of the DNS encoded reverse name
         n = mdnsEncodeName(context->ipv4ReverseName, "in-addr", ".arpa", NULL);

         //Check the length of the resulting mDNS message
         if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
            return ERROR_MESSAGE_TOO_LONG;

         //The second pass encodes the reverse name using the DNS name notation
         offset += mdnsEncodeName(context->ipv4ReverseName, "in-addr", ".arpa",
            (uint8_t *) message->dnsHeader + offset);

         //Consider the length of the resource record itself
         n = sizeof(DnsResourceRecord) + sizeof(Ipv4Addr);

         //Check the length of the resulting mDNS message
         if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
            return ERROR_MESSAGE_TOO_LONG;

         //Point to the corresponding resource record
         record = DNS_GET_RESOURCE_RECORD(message->dnsHeader, offset);

         //Fill in resource record
         record->rtype = HTONS(DNS_RR_TYPE_PTR);
         record->rclass = HTONS(DNS_RR_CLASS_IN);
         record->ttl = htonl(ttl);

         //Check whether the cache-flush bit should be set
         if(cacheFlush)
            record->rclass |= HTONS(MDNS_RCLASS_CACHE_FLUSH);

         //Advance write index
         offset += sizeof(DnsResourceRecord);

         //The first pass calculates the length of the DNS encoded host name
         n = mdnsEncodeName("", context->hostname, ".local", NULL);

         //Check the length of the resulting mDNS message
         if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
            return ERROR_MESSAGE_TOO_LONG;

         //The second pass encodes the host name using DNS notation
         n = mdnsEncodeName("", context->hostname, ".local", record->rdata);

         //Convert length field to network byte order
         record->rdlength = htons(n);

         //Number of resource records in the answer section
         message->dnsHeader->ancount++;
         //Update the length of the mDNS response message
         message->length = offset + n;
      }
   }
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add reverse address mapping PTR record (IPv6)
 * @param[in] interface Underlying network interface
 * @param[in,out] message Pointer to the mDNS message
 * @param[in] cacheFlush Cache-flush bit
 * @param[in] ttl Resource record TTL (cache lifetime)
 * @return Error code
 **/

error_t mdnsResponderAddIpv6ReversePtrRecord(NetInterface *interface,
   MdnsMessage *message, bool_t cacheFlush, uint32_t ttl)
{
#if (IPV6_SUPPORT == ENABLED)
   size_t n;
   size_t offset;
   bool_t duplicate;
   MdnsResponderContext *context;
   DnsResourceRecord *record;

   //Point to the mDNS responder context
   context = interface->mdnsResponderContext;

   //Valid reverse name?
   if(context->ipv6ReverseName[0] != '\0')
   {
      //Check whether the resource record is already present in the Answer
      //Section of the message
      duplicate = mdnsCheckDuplicateRecord(message, context->ipv6ReverseName,
         "ip6", ".arpa", DNS_RR_TYPE_PTR);

      //The duplicates should be suppressed and the resource record should
      //appear only once in the list
      if(!duplicate)
      {
         //Set the position to the end of the buffer
         offset = message->length;

         //The first pass calculates the length of the DNS encoded reverse name
         n = mdnsEncodeName(context->ipv6ReverseName, "ip6", ".arpa", NULL);

         //Check the length of the resulting mDNS message
         if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
            return ERROR_MESSAGE_TOO_LONG;

         //The second pass encodes the reverse name using the DNS name notation
         offset += mdnsEncodeName(context->ipv6ReverseName, "ip6", ".arpa",
            (uint8_t *) message->dnsHeader + offset);

         //Consider the length of the resource record itself
         n = sizeof(DnsResourceRecord) + sizeof(Ipv4Addr);

         //Check the length of the resulting mDNS message
         if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
            return ERROR_MESSAGE_TOO_LONG;

         //Point to the corresponding resource record
         record = DNS_GET_RESOURCE_RECORD(message->dnsHeader, offset);

         //Fill in resource record
         record->rtype = HTONS(DNS_RR_TYPE_PTR);
         record->rclass = HTONS(DNS_RR_CLASS_IN);
         record->ttl = htonl(ttl);

         //Check whether the cache-flush bit should be set
         if(cacheFlush)
            record->rclass |= HTONS(MDNS_RCLASS_CACHE_FLUSH);

         //Advance write index
         offset += sizeof(DnsResourceRecord);

         //The first pass calculates the length of the DNS encoded host name
         n = mdnsEncodeName("", context->hostname, ".local", NULL);

         //Check the length of the resulting mDNS message
         if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
            return ERROR_MESSAGE_TOO_LONG;

         //The second pass encodes the host name using DNS notation
         n = mdnsEncodeName("", context->hostname, ".local", record->rdata);

         //Convert length field to network byte order
         record->rdlength = htons(n);

         //Number of resource records in the answer section
         message->dnsHeader->ancount++;
         //Update the length of the mDNS response message
         message->length = offset + n;
      }
   }
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add NSEC record to a mDNS message
 * @param[in] interface Underlying network interface
 * @param[in,out] message Pointer to the mDNS message
 * @param[in] cacheFlush Cache-flush bit
 * @param[in] ttl Resource record TTL (cache lifetime)
 * @return Error code
 **/

error_t mdnsResponderAddNsecRecord(NetInterface *interface,
   MdnsMessage *message, bool_t cacheFlush, uint32_t ttl)
{
   size_t n;
   size_t offset;
   bool_t duplicate;
   size_t bitmapLength;
   uint8_t bitmap[8];
   MdnsResponderContext *context;
   DnsResourceRecord *record;

   //Point to the mDNS responder context
   context = interface->mdnsResponderContext;

   //Check whether the resource record is already present in the Answer
   //Section of the message
   duplicate = mdnsCheckDuplicateRecord(message, context->hostname,
      "", ".local", DNS_RR_TYPE_NSEC);

   //The duplicates should be suppressed and the resource record should
   //appear only once in the list
   if(!duplicate)
   {
      //The bitmap identifies the resource record types that exist
      osMemset(bitmap, 0, sizeof(bitmap));

#if (IPV4_SUPPORT == ENABLED)
      //A resource record is supported
      DNS_SET_NSEC_BITMAP(bitmap, DNS_RR_TYPE_A);
#endif

#if (IPV6_SUPPORT == ENABLED)
      //A resource record is supported
      DNS_SET_NSEC_BITMAP(bitmap, DNS_RR_TYPE_AAAA);
#endif

      //Compute the length of the bitmap
      for(bitmapLength = sizeof(bitmap); bitmapLength > 0; bitmapLength--)
      {
         //Trailing zero octets in the bitmap must be omitted...
         if(bitmap[bitmapLength - 1] != 0x00)
            break;
      }

      //Set the position to the end of the buffer
      offset = message->length;

      //The first pass calculates the length of the DNS encoded host name
      n = mdnsEncodeName(context->hostname, "", ".local", NULL);

      //Check the length of the resulting mDNS message
      if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //The second pass encodes the host name using the DNS name notation
      offset += mdnsEncodeName(context->hostname, "", ".local",
         (uint8_t *) message->dnsHeader + offset);

      //Consider the length of the resource record itself
      if((offset + sizeof(DnsResourceRecord)) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //Point to the corresponding resource record
      record = DNS_GET_RESOURCE_RECORD(message->dnsHeader, offset);

      //Fill in resource record
      record->rtype = HTONS(DNS_RR_TYPE_NSEC);
      record->rclass = HTONS(DNS_RR_CLASS_IN);
      record->ttl = htonl(ttl);

      //Check whether the cache-flush bit should be set
      if(cacheFlush)
         record->rclass |= HTONS(MDNS_RCLASS_CACHE_FLUSH);

      //Advance write index
      offset += sizeof(DnsResourceRecord);

      //Check the length of the resulting mDNS message
      if((offset + n + 2 + bitmapLength) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //The Next Domain Name field contains the record's own name
      mdnsEncodeName(context->hostname, "", ".local", record->rdata);

      //DNS NSEC record is limited to Window Block number zero
      record->rdata[n++] = 0;
      //The Bitmap Length is a value in the range 1-32
      record->rdata[n++] = bitmapLength;

      //The Bitmap data identifies the resource record types that exist
      osMemcpy(record->rdata + n, bitmap, bitmapLength);

      //Convert length field to network byte order
      record->rdlength = htons(n + bitmapLength);

      //Number of resource records in the answer section
      message->dnsHeader->ancount++;
      //Update the length of the DNS message
      message->length = offset + n + bitmapLength;
   }

   //Successful processing
   return NO_ERROR;
}

#endif
