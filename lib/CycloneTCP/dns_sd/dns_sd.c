/**
 * @file dns_sd.c
 * @brief DNS-SD (DNS-Based Service Discovery)
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
 * DNS-SD allows clients to discover a list of named instances of that
 * desired service, using standard DNS queries. Refer to the following
 * RFCs for complete details:
 * - RFC 6763: DNS-Based Service Discovery
 * - RFC 2782: A DNS RR for specifying the location of services (DNS SRV)
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL DNS_SD_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include <ctype.h>
#include "core/net.h"
#include "mdns/mdns_common.h"
#include "mdns/mdns_responder.h"
#include "dns/dns_debug.h"
#include "dns_sd/dns_sd.h"
#include "str.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (DNS_SD_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t dnsSdTickCounter;


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains DNS-SD settings
 **/

void dnsSdGetDefaultSettings(DnsSdSettings *settings)
{
   //Use default interface
   settings->interface = netGetDefaultInterface();

   //Number of announcement packets
   settings->numAnnouncements = MDNS_ANNOUNCE_NUM;
   //TTL resource record
   settings->ttl = DNS_SD_DEFAULT_RR_TTL;
   //FSM state change event
   settings->stateChangeEvent = NULL;
}


/**
 * @brief DNS-DS initialization
 * @param[in] context Pointer to the DNS-SD context
 * @param[in] settings DNS-SD specific settings
 * @return Error code
 **/

error_t dnsSdInit(DnsSdContext *context, const DnsSdSettings *settings)
{
   NetInterface *interface;

   //Debug message
   TRACE_INFO("Initializing DNS-SD...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //Invalid network interface?
   if(settings->interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the underlying network interface
   interface = settings->interface;

   //Clear the DNS-SD context
   osMemset(context, 0, sizeof(DnsSdContext));
   //Save user settings
   context->settings = *settings;

   //DNS-SD is currently suspended
   context->running = FALSE;
   //Initialize state machine
   context->state = MDNS_STATE_INIT;

   //Attach the DNS-SD context to the network interface
   interface->dnsSdContext = context;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Start mDNS responder
 * @param[in] context Pointer to the DNS-SD context
 * @return Error code
 **/

error_t dnsSdStart(DnsSdContext *context)
{
   //Make sure the DNS-SD context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting DNS-SD...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Start DNS-SD
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
 * @param[in] context Pointer to the DNS-SD context
 * @return Error code
 **/

error_t dnsSdStop(DnsSdContext *context)
{
   //Make sure the DNS-SD context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Stopping DNS-SD...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Suspend DNS-SD
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
 * @param[in] context Pointer to the DNS-SD context
 * @return Current DNS-SD state
 **/

MdnsState dnsSdGetState(DnsSdContext *context)
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
 * @brief Set service instance name
 * @param[in] context Pointer to the DNS-SD context
 * @param[in] instanceName NULL-terminated string that contains the service
 *   instance name
 * @return Error code
 **/

error_t dnsSdSetInstanceName(DnsSdContext *context, const char_t *instanceName)
{
   NetInterface *interface;

   //Check parameters
   if(context == NULL || instanceName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Any registered services?
   if(dnsSdGetNumServices(context) > 0)
   {
      //Check whether the link is up
      if(interface->linkState)
      {
         //Send a goodbye packet
         dnsSdSendGoodbye(context, NULL);
      }
   }

   //Set instance name
   strSafeCopy(context->instanceName, instanceName,
      DNS_SD_MAX_INSTANCE_NAME_LEN);

   //Restart probing process
   dnsSdStartProbing(context);

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Register a DNS-SD service
 * @param[in] context Pointer to the DNS-SD context
 * @param[in] serviceName NULL-terminated string that contains the name of the
 *   service to be registered
 * @param[in] priority Priority field
 * @param[in] weight Weight field
 * @param[in] port Port number
 * @param[in] metadata NULL-terminated string that contains the discovery-time
 *   metadata (TXT record)
 * @return Error code
 **/

error_t dnsSdRegisterService(DnsSdContext *context, const char_t *serviceName,
   uint16_t priority, uint16_t weight, uint16_t port, const char_t *metadata)
{
   error_t error;
   size_t i;
   size_t j;
   size_t k;
   size_t n;
   DnsSdService *entry;
   DnsSdService *firstFreeEntry;

   //Check parameters
   if(context == NULL || serviceName == NULL || metadata == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Keep track of the first free entry
   firstFreeEntry = NULL;

   //Loop through the list of registered services
   for(i = 0; i < DNS_SD_SERVICE_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->serviceList[i];

      //Check if the entry is currently in use
      if(entry->name[0] != '\0')
      {
         //Check whether the specified service is already registered
         if(!osStrcasecmp(entry->name, serviceName))
            break;
      }
      else
      {
         //Keep track of the first free entry
         if(firstFreeEntry == NULL)
            firstFreeEntry = entry;
      }
   }

   //If the specified service is not yet registered, then a new
   //entry should be created
   if(i >= DNS_SD_SERVICE_LIST_SIZE)
      entry = firstFreeEntry;

   //Check whether the service list runs out of space
   if(entry != NULL)
   {
      //Service name
      strSafeCopy(entry->name, serviceName, DNS_SD_MAX_SERVICE_NAME_LEN);

      //Priority field
      entry->priority = priority;
      //Weight field
      entry->weight = weight;
      //Port number
      entry->port = port;

      //Clear TXT record
      entry->metadataLength = 0;

      //Point to the beginning of the information string
      i = 0;
      j = 0;

      //Point to the beginning of the resulting TXT record data
      k = 0;

      //Format TXT record
      while(1)
      {
         //End of text data?
         if(metadata[i] == '\0' || metadata[i] == ';')
         {
            //Calculate the length of the text data
            n = MIN(i - j, UINT8_MAX);

            //Check the length of the resulting TXT record
            if((entry->metadataLength + n + 1) > DNS_SD_MAX_METADATA_LEN)
               break;

            //Write length field
            entry->metadata[k] = n;
            //Write text data
            osMemcpy(entry->metadata + k + 1, metadata + j, n);

            //Jump to the next text data
            j = i + 1;
            //Advance write index
            k += n + 1;

            //Update the length of the TXT record
            entry->metadataLength += n + 1;

            //End of string detected?
            if(metadata[i] == '\0')
               break;
         }

         //Advance read index
         i++;
      }

      //Empty TXT record?
      if(!entry->metadataLength)
      {
         //An empty TXT record shall contain a single zero byte
         entry->metadata[0] = 0;
         entry->metadataLength = 1;
      }

      //Restart probing process
      dnsSdStartProbing(context);

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The service list is full
      error = ERROR_FAILURE;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return error code
   return error;
}


/**
 * @brief Unregister a DNS-SD service
 * @param[in] context Pointer to the DNS-SD context
 * @param[in] serviceName NULL-terminated string that contains the name of the
 *   service to be unregistered
 * @return Error code
 **/

error_t dnsSdUnregisterService(DnsSdContext *context, const char_t *serviceName)
{
   uint_t i;
   DnsSdService *entry;

   //Check parameters
   if(context == NULL || serviceName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Loop through the list of registered services
   for(i = 0; i < DNS_SD_SERVICE_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->serviceList[i];

      //Service name found?
      if(!osStrcasecmp(entry->name, serviceName))
      {
         //Send a goodbye packet
         dnsSdSendGoodbye(context, entry);
         //Remove the service from the list
         entry->name[0] = '\0';
      }
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get the number of registered services
 * @param[in] context Pointer to the DNS-SD context
 * @return Number of registered services
 **/

uint_t dnsSdGetNumServices(DnsSdContext *context)
{
   uint_t i;
   uint_t n;

   //Number of registered services
   n = 0;

   //Check parameter
   if(context != NULL)
   {
      //Valid instance name?
      if(context->instanceName[0] != '\0')
      {
         //Loop through the list of registered services
         for(i = 0; i < DNS_SD_SERVICE_LIST_SIZE; i++)
         {
            //Check if the entry is currently in use
            if(context->serviceList[i].name[0] != '\0')
               n++;
         }
      }
   }

   //Return the number of registered services
   return n;
}


/**
 * @brief Restart probing process
 * @param[in] context Pointer to the DNS-SD context
 * @return Error code
 **/

error_t dnsSdStartProbing(DnsSdContext *context)
{
   //Check parameter
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Force DNS-SD to start probing again
   context->state = MDNS_STATE_INIT;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief DNS-SD responder timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * manage DNS-SD operation
 *
 * @param[in] context Pointer to the DNS-SD context
 **/

void dnsSdTick(DnsSdContext *context)
{
   systime_t time;
   systime_t delay;
   NetInterface *interface;

   //Make sure DNS-SD has been properly instantiated
   if(context == NULL)
      return;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Get current time
   time = osGetSystemTime();

   //Check current state
   if(context->state == MDNS_STATE_INIT)
   {
      //Ensure the mDNS and DNS-SD services are running
      if(context->running && interface->mdnsResponderContext != NULL)
      {
         //Wait for mDNS probing to complete
         if(interface->mdnsResponderContext->state == MDNS_STATE_ANNOUNCING ||
            interface->mdnsResponderContext->state == MDNS_STATE_IDLE)
         {
            //Any registered services?
            if(dnsSdGetNumServices(context) > 0)
            {
               //Initial random delay
               delay = netGetRandRange(MDNS_RAND_DELAY_MIN, MDNS_RAND_DELAY_MAX);
               //Perform probing
               dnsSdChangeState(context, MDNS_STATE_PROBING, delay);
            }
         }
      }
   }
   else if(context->state == MDNS_STATE_PROBING)
   {
      //Probing failed?
      if(context->conflict && context->retransmitCount > 0)
      {
         //Programmatically change the service instance name
         dnsSdChangeInstanceName(context);
         //Probe again, and repeat as necessary until a unique name is found
         dnsSdChangeState(context, MDNS_STATE_PROBING, 0);
      }
      //Tie-break lost?
      else if(context->tieBreakLost && context->retransmitCount > 0)
      {
         //The host defers to the winning host by waiting one second, and
         //then begins probing for this record again
         dnsSdChangeState(context, MDNS_STATE_PROBING, MDNS_PROBE_DEFER);
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
               dnsSdSendProbe(context);

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
                  dnsSdChangeState(context, MDNS_STATE_ANNOUNCING, 0);
               else
                  dnsSdChangeState(context, MDNS_STATE_IDLE, 0);
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
         dnsSdChangeState(context, MDNS_STATE_PROBING, 0);
      }
      else
      {
         //Check current time
         if(timeCompare(time, context->timestamp + context->timeout) >= 0)
         {
            //Send announcement packet
            dnsSdSendAnnouncement(context);

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
               dnsSdChangeState(context, MDNS_STATE_IDLE, 0);
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
         dnsSdChangeState(context, MDNS_STATE_PROBING, 0);
      }
   }
}


/**
 * @brief Callback function for link change event
 * @param[in] context Pointer to the DNS-SD context
 **/

void dnsSdLinkChangeEvent(DnsSdContext *context)
{
   //Make sure DNS-SD has been properly instantiated
   if(context == NULL)
      return;

   //Whenever a mDNS responder receives an indication of a link
   //change event, it must perform probing and announcing
   dnsSdChangeState(context, MDNS_STATE_INIT, 0);
}


/**
 * @brief Update FSM state
 * @param[in] context Pointer to the DNS-SD context
 * @param[in] newState New state to switch to
 * @param[in] delay Initial delay
 **/

void dnsSdChangeState(DnsSdContext *context,
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
 * @brief Programmatically change the service instance name
 * @param[in] context Pointer to the DNS-SD context
 **/

void dnsSdChangeInstanceName(DnsSdContext *context)
{
   size_t i;
   size_t m;
   size_t n;
   uint32_t index;
   char_t s[16];

   //Retrieve the length of the string
   n = osStrlen(context->instanceName);

   //Parse the string backwards
   for(i = n; i > 0; i--)
   {
      //Last character?
      if(i == n)
      {
         //Check whether the last character is a bracket
         if(context->instanceName[i - 1] != ')')
            break;
      }
      else
      {
         //Check whether the current character is a digit
         if(!osIsdigit(context->instanceName[i - 1]))
            break;
      }
   }

   //Any number following the service instance name?
   if(context->instanceName[i] != '\0')
   {
      //Retrieve the number at the end of the name
      index = atoi(context->instanceName + i);
      //Increment the value
      index++;

      //Check the length of the name
      if(i >= 2)
      {
         //Discard any space and bracket that may precede the number
         if(context->instanceName[i - 2] == ' ' &&
            context->instanceName[i - 1] == '(')
         {
            i -= 2;
         }
      }

      //Strip the digits
      context->instanceName[i] = '\0';
   }
   else
   {
      //Append the digit "2" to the name
      index = 2;
   }

   //Convert the number to a string of characters
   m = osSprintf(s, " (%" PRIu32 ")", index);

   //Sanity check
   if((i + m) <= DNS_SD_MAX_INSTANCE_NAME_LEN)
   {
      //Programmatically change the service instance name
      osStrcat(context->instanceName, s);
   }
}


/**
 * @brief Send probe packet
 * @param[in] context Pointer to the DNS-SD context
 * @return Error code
 **/

error_t dnsSdSendProbe(DnsSdContext *context)
{
   error_t error;
   uint_t i;
   NetInterface *interface;
   DnsQuestion *dnsQuestion;
   DnsSdService *service;
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
      //For all those resource records that a mDNS responder desires to be
      //unique on the local link, it must send a mDNS query asking for those
      //resource records, to see if any of them are already in use
      if(dnsSdGetNumServices(context) > 0)
      {
         //Loop through the list of registered services
         for(i = 0; i < DNS_SD_SERVICE_LIST_SIZE; i++)
         {
            //Point to the current entry
            service = &context->serviceList[i];

            //Valid service?
            if(service->name[0] != '\0')
            {
               //Encode the service name using DNS notation
               message.length += mdnsEncodeName(context->instanceName, service->name,
                  ".local", (uint8_t *) message.dnsHeader + message.length);

               //Point to the corresponding question structure
               dnsQuestion = DNS_GET_QUESTION(message.dnsHeader, message.length);

               //The probes should be sent as QU questions with the unicast-response
               //bit set, to allow a defending host to respond immediately via unicast
               dnsQuestion->qtype = HTONS(DNS_RR_TYPE_ANY);
               dnsQuestion->qclass = HTONS(MDNS_QCLASS_QU | DNS_RR_CLASS_IN);

               //Update the length of the mDNS query message
               message.length += sizeof(DnsQuestion);

               //Number of questions in the Question Section
               message.dnsHeader->qdcount++;
            }
         }
      }

      //A probe query can be distinguished from a normal query by the fact that
      //a probe query contains a proposed record in the Authority Section that
      //answers the question in the Question Section
      if(dnsSdGetNumServices(context) > 0)
      {
         //Loop through the list of registered services
         for(i = 0; i < DNS_SD_SERVICE_LIST_SIZE; i++)
         {
            //Point to the current entry
            service = &context->serviceList[i];

            //Valid service?
            if(service->name[0] != '\0')
            {
               //Format SRV resource record
               error = dnsSdAddSrvRecord(interface, &message,
                  service, FALSE, DNS_SD_DEFAULT_RR_TTL);
               //Any error to report?
               if(error)
                  break;

               //Format TXT resource record
               error = dnsSdAddTxtRecord(interface, &message,
                  service, FALSE, DNS_SD_DEFAULT_RR_TTL);
               //Any error to report?
               if(error)
                  break;
            }
         }
      }

      //Propagate exception if necessary
      if(error)
         break;

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
 * @param[in] context Pointer to the DNS-SD context
 * @return Error code
 **/

error_t dnsSdSendAnnouncement(DnsSdContext *context)
{
   error_t error;
   uint_t i;
   NetInterface *interface;
   DnsSdService *service;
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
      //Send an unsolicited mDNS response containing, in the Answer Section,
      //all of its newly registered resource records
      if(dnsSdGetNumServices(context) > 0)
      {
         //Loop through the list of registered services
         for(i = 0; i < DNS_SD_SERVICE_LIST_SIZE; i++)
         {
            //Point to the current entry
            service = &context->serviceList[i];

            //Valid service?
            if(service->name[0] != '\0')
            {
               //Format PTR resource record (service type enumeration)
               error = dnsSdAddServiceEnumPtrRecord(interface,
                  &message, service, DNS_SD_DEFAULT_RR_TTL);
               //Any error to report?
               if(error)
                  break;

               //Format PTR resource record
               error = dnsSdAddPtrRecord(interface, &message,
                  service, DNS_SD_DEFAULT_RR_TTL);
               //Any error to report?
               if(error)
                  break;

               //Format SRV resource record
               error = dnsSdAddSrvRecord(interface, &message,
                  service, TRUE, DNS_SD_DEFAULT_RR_TTL);
               //Any error to report?
               if(error)
                  break;

               //Format TXT resource record
               error = dnsSdAddTxtRecord(interface, &message,
                  service, TRUE, DNS_SD_DEFAULT_RR_TTL);
               //Any error to report?
               if(error)
                  break;
            }
         }
      }

      //Propagate exception if necessary
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
 * @param[in] context Pointer to the DNS-SD context
 * @param[in] service Pointer to a DNS-SD service
 * @return Error code
 **/

error_t dnsSdSendGoodbye(DnsSdContext *context, const DnsSdService *service)
{
   error_t error;
   uint_t i;
   NetInterface *interface;
   DnsSdService *entry;
   MdnsMessage message;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Create an empty mDNS response message
   error = mdnsCreateMessage(&message, TRUE);
   //Any error to report?
   if(error)
      return error;

   //Loop through the list of registered services
   for(i = 0; i < DNS_SD_SERVICE_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->serviceList[i];

      //Valid service?
      if(entry->name[0] != '\0')
      {
         if(service == entry || service == NULL)
         {
            //Format PTR resource record (service type enumeration)
            error = dnsSdAddServiceEnumPtrRecord(interface, &message, entry, 0);
            //Any error to report?
            if(error)
               break;

            //Format PTR resource record
            error = dnsSdAddPtrRecord(interface, &message, entry, 0);
            //Any error to report?
            if(error)
               break;

            //Format SRV resource record
            error = dnsSdAddSrvRecord(interface, &message, entry, TRUE, 0);
            //Any error to report?
            if(error)
               break;

            //Format TXT resource record
            error = dnsSdAddTxtRecord(interface, &message, entry, TRUE, 0);
            //Any error to report?
            if(error)
               break;
         }
      }
   }

   //Check status code
   if(!error)
   {
      //Send mDNS message
      error = mdnsSendMessage(interface, &message, NULL, MDNS_PORT);
   }

   //Free previously allocated memory
   mdnsDeleteMessage(&message);

   //Return status code
   return error;
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

error_t dnsSdParseQuestion(NetInterface *interface, const MdnsMessage *query,
   size_t offset, const DnsQuestion *question, MdnsMessage *response)
{
   error_t error;
   uint_t i;
   uint16_t qclass;
   uint16_t qtype;
   uint32_t ttl;
   bool_t cacheFlush;
   DnsSdContext *context;
   DnsSdService *service;

   //Point to the DNS-SD context
   context = interface->dnsSdContext;
   //Make sure DNS-SD has been properly instantiated
   if(context == NULL)
      return NO_ERROR;

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

   //Any registered services?
   if(dnsSdGetNumServices(context) > 0)
   {
      //Loop through the list of registered services
      for(i = 0; i < DNS_SD_SERVICE_LIST_SIZE; i++)
      {
         //Point to the current entry
         service = &context->serviceList[i];

         //Valid service?
         if(service->name[0] != '\0')
         {
            //Check the class of the query
            if(qclass == DNS_RR_CLASS_IN || qclass == DNS_RR_CLASS_ANY)
            {
               //Compare service name
               if(!mdnsCompareName(query->dnsHeader, query->length,
                  offset, "", "_services._dns-sd._udp", ".local", 0))
               {
                  //PTR query?
                  if(qtype == DNS_RR_TYPE_PTR || qtype == DNS_RR_TYPE_ANY)
                  {
                     //Format PTR resource record (service type enumeration)
                     error = dnsSdAddServiceEnumPtrRecord(interface,
                        response, service, ttl);
                     //Any error to report?
                     if(error)
                        return error;

                     //Update the number of shared resource records
                     response->sharedRecordCount++;
                  }
               }
               else if(!mdnsCompareName(query->dnsHeader, query->length,
                  offset, "", service->name, ".local", 0))
               {
                  //PTR query?
                  if(qtype == DNS_RR_TYPE_PTR || qtype == DNS_RR_TYPE_ANY)
                  {
                     //Format PTR resource record
                     error = dnsSdAddPtrRecord(interface, response,
                        service, ttl);
                     //Any error to report?
                     if(error)
                        return error;

                     //Update the number of shared resource records
                     response->sharedRecordCount++;
                  }
               }
               else if(!mdnsCompareName(query->dnsHeader, query->length, offset,
                  context->instanceName, service->name, ".local", 0))
               {
                  //SRV query?
                  if(qtype == DNS_RR_TYPE_SRV || qtype == DNS_RR_TYPE_ANY)
                  {
                     //Format SRV resource record
                     error = dnsSdAddSrvRecord(interface, response,
                        service, cacheFlush, ttl);
                     //Any error to report?
                     if(error)
                        return error;
                  }

                  //TXT query?
                  if(qtype == DNS_RR_TYPE_TXT || qtype == DNS_RR_TYPE_ANY)
                  {
                     //Format TXT resource record
                     error = dnsSdAddTxtRecord(interface, response,
                        service, cacheFlush, ttl);
                     //Any error to report?
                     if(error)
                        return error;
                  }

                  //NSEC query?
                  if(qtype != DNS_RR_TYPE_SRV && qtype != DNS_RR_TYPE_TXT)
                  {
                     //Format NSEC resource record
                     error = dnsSdAddNsecRecord(interface, response,
                        service, cacheFlush, ttl);
                     //Any error to report?
                     if(error)
                        return error;
                  }
               }
            }
         }
      }
   }

   //Successful processing
   return NO_ERROR;
}



/**
 * @brief Parse a resource record from the Authority Section
 * @param[in] interface Underlying network interface
 * @param[in] query Incoming mDNS query message
 * @param[in] offset Offset to first byte of the resource record
 * @param[in] record Pointer to the resource record
 **/

void dnsSdParseNsRecord(NetInterface *interface, const MdnsMessage *query,
   size_t offset, const DnsResourceRecord *record)
{
   uint_t i;
   uint16_t rclass;
   DnsSdContext *context;
   DnsSdService *service;
   DnsSrvResourceRecord *srvRecord;

   //Point to the DNS-SD context
   context = interface->dnsSdContext;
   //Make sure DNS-SD has been properly instantiated
   if(context == NULL)
      return;

   //Any services registered?
   if(dnsSdGetNumServices(context) > 0)
   {
      //Loop through the list of registered services
      for(i = 0; i < DNS_SD_SERVICE_LIST_SIZE; i++)
      {
         //Point to the current entry
         service = &context->serviceList[i];

         //Valid service?
         if(service->name[0] != '\0')
         {
            //Apply tie-breaking rules
            if(!mdnsCompareName(query->dnsHeader, query->length, offset,
               context->instanceName, service->name, ".local", 0))
            {
               //Convert the class to host byte order
               rclass = ntohs(record->rclass);
               //Discard Cache Flush flag
               rclass &= ~MDNS_RCLASS_CACHE_FLUSH;

               //Check the class of the resource record
               if(rclass == DNS_RR_CLASS_IN)
               {
                  //SRV resource record found?
                  if(ntohs(record->rtype) == DNS_RR_TYPE_SRV)
                  {
                     //Cast resource record
                     srvRecord = (DnsSrvResourceRecord *) record;

                     //Compare Priority fields
                     if(ntohs(srvRecord->priority) > service->priority)
                     {
                        context->tieBreakLost = TRUE;
                     }
                     else if(ntohs(srvRecord->priority) == service->priority)
                     {
                        //Compare Weight fields
                        if(ntohs(srvRecord->weight) > service->weight)
                        {
                           context->tieBreakLost = TRUE;
                        }
                        else if(ntohs(srvRecord->weight) == service->weight)
                        {
                           //Compare Port fields
                           if(ntohs(srvRecord->port) > service->port)
                           {
                              context->tieBreakLost = TRUE;
                           }
                           else if(ntohs(srvRecord->port) == service->port)
                           {
                              //Compute the offset of the first byte of the target
                              offset = srvRecord->target - (uint8_t *) query->dnsHeader;

                              if(mdnsCompareName(query->dnsHeader, query->length, offset,
                                 context->instanceName, "", ".local", 0) > 0)
                              {
                                 //The host has lost the tie-break
                                 context->tieBreakLost = TRUE;
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
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

void dnsSdParseAnRecord(NetInterface *interface, const MdnsMessage *response,
   size_t offset, const DnsResourceRecord *record)
{
   uint_t i;
   uint16_t rclass;
   DnsSdContext *context;
   DnsSdService *service;

   //Point to the DNS-SD context
   context = interface->dnsSdContext;
   //Make sure DNS-SD has been properly instantiated
   if(context == NULL)
      return;

   //Any services registered?
   if(dnsSdGetNumServices(context) > 0)
   {
      //Loop through the list of registered services
      for(i = 0; i < DNS_SD_SERVICE_LIST_SIZE; i++)
      {
         //Point to the current entry
         service = &context->serviceList[i];

         //Valid service?
         if(service->name[0] != '\0')
         {
            //Check for conflicts
            if(!mdnsCompareName(response->dnsHeader, response->length, offset,
               context->instanceName, service->name, ".local", 0))
            {
               //Convert the class to host byte order
               rclass = ntohs(record->rclass);
               //Discard Cache Flush flag
               rclass &= ~MDNS_RCLASS_CACHE_FLUSH;

               //Check the class of the resource record
               if(rclass == DNS_RR_CLASS_IN)
               {
                  //SRV resource record found?
                  if(ntohs(record->rtype) == DNS_RR_TYPE_SRV)
                  {
                     //Compute the offset of the first byte of the rdata
                     offset = record->rdata - (uint8_t *) response->dnsHeader;

                     //A conflict occurs when a mDNS responder has a unique record for
                     //which it is currently authoritative, and it receives a mDNS
                     //response message containing a record with the same name, rrtype
                     //and rrclass, but inconsistent rdata
                     if(mdnsCompareName(response->dnsHeader, response->length, offset,
                        context->instanceName, "", ".local", 0))
                     {
                        //The service instance name is already in use by some other host
                        context->conflict = TRUE;
                     }
                  }
               }
            }
         }
      }
   }
}


/**
 * @brief Additional record generation
 * @param[in] interface Underlying network interface
 * @param[in,out] response mDNS response message
 * @param[in] legacyUnicast This flag is set for legacy unicast responses
 **/

void dnsSdGenerateAdditionalRecords(NetInterface *interface,
   MdnsMessage *response, bool_t legacyUnicast)
{
   error_t error;
   uint_t i;
   uint_t j;
   size_t n;
   size_t offset;
   uint_t ancount;
   uint16_t rclass;
   uint32_t ttl;
   bool_t cacheFlush;
   DnsSdContext *context;
   DnsSdService *service;
   DnsResourceRecord *record;

   //Point to the DNS-SD context
   context = interface->dnsSdContext;
   //Make sure DNS-SD has been properly instantiated
   if(context == NULL)
      return;

   //No registered services?
   if(dnsSdGetNumServices(context) == 0)
      return;

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

   //Parse the Answer Section
   for(i = 0; i < ancount; i++)
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

      //Loop through the list of registered services
      for(j = 0; j < DNS_SD_SERVICE_LIST_SIZE; j++)
      {
         //Point to the current entry
         service = &context->serviceList[j];

         //Valid service?
         if(service->name[0] != '\0')
         {
            //Check the class of the resource record
            if(rclass == DNS_RR_CLASS_IN)
            {
               //PTR record?
               if(ntohs(record->rtype) == DNS_RR_TYPE_PTR)
               {
                  //Compare service name
                  if(!mdnsCompareName(response->dnsHeader, response->length,
                     offset, "", service->name, ".local", 0))
                  {
                     //Format SRV resource record
                     error = dnsSdAddSrvRecord(interface,
                        response, service, cacheFlush, ttl);
                     //Any error to report?
                     if(error)
                        return;

                     //Format TXT resource record
                     error = dnsSdAddTxtRecord(interface,
                        response, service, cacheFlush, ttl);
                     //Any error to report?
                     if(error)
                        return;
                  }
               }
               //SRV record?
               else if(ntohs(record->rtype) == DNS_RR_TYPE_SRV)
               {
                  //Compare service name
                  if(!mdnsCompareName(response->dnsHeader, response->length,
                     offset, context->instanceName, service->name, ".local", 0))
                  {
                     //Format TXT resource record
                     error = dnsSdAddTxtRecord(interface,
                        response, service, cacheFlush, ttl);
                     //Any error to report?
                     if(error)
                        return;
                  }
               }
            }
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
 * @brief Add PTR record to a mDNS message (in response to a meta-query)
 * @param[in] interface Underlying network interface
 * @param[in,out] message Pointer to the mDNS message
 * @param[in] service Pointer to a DNS-SD service
 * @param[in] ttl Resource record TTL (cache lifetime)
 * @return Error code
 **/

error_t dnsSdAddServiceEnumPtrRecord(NetInterface *interface,
   MdnsMessage *message, const DnsSdService *service, uint32_t ttl)
{
   size_t n;
   size_t offset;
   DnsResourceRecord *record;

   //Set the position to the end of the buffer
   offset = message->length;

   //The first pass calculates the length of the DNS encoded service name
   n = mdnsEncodeName("", "_services._dns-sd._udp", ".local", NULL);

   //Check the length of the resulting mDNS message
   if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
      return ERROR_MESSAGE_TOO_LONG;

   //The second pass encodes the service name using the DNS name notation
   offset += mdnsEncodeName("", "_services._dns-sd._udp",
      ".local", (uint8_t *) message->dnsHeader + offset);

   //Consider the length of the resource record itself
   if((offset + sizeof(DnsResourceRecord)) > MDNS_MESSAGE_MAX_SIZE)
      return ERROR_MESSAGE_TOO_LONG;

   //Point to the corresponding resource record
   record = DNS_GET_RESOURCE_RECORD(message->dnsHeader, offset);

   //Fill in resource record
   record->rtype = HTONS(DNS_RR_TYPE_PTR);
   record->rclass = HTONS(DNS_RR_CLASS_IN);
   record->ttl = htonl(ttl);

   //Advance write index
   offset += sizeof(DnsResourceRecord);

   //The first pass calculates the length of the DNS encoded service name
   n = mdnsEncodeName("", service->name, ".local", NULL);

   //Check the length of the resulting mDNS message
   if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
      return ERROR_MESSAGE_TOO_LONG;

   //The second pass encodes the service name using DNS notation
   n = mdnsEncodeName("", service->name,
      ".local", record->rdata);

   //Convert length field to network byte order
   record->rdlength = htons(n);

   //Number of resource records in the answer section
   message->dnsHeader->ancount++;
   //Update the length of the DNS message
   message->length = offset + n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add PTR record to a mDNS message
 * @param[in] interface Underlying network interface
 * @param[in,out] message Pointer to the mDNS message
 * @param[in] service Pointer to a DNS-SD service
 * @param[in] ttl Resource record TTL (cache lifetime)
 * @return Error code
 **/

error_t dnsSdAddPtrRecord(NetInterface *interface,
   MdnsMessage *message, const DnsSdService *service, uint32_t ttl)
{
   size_t n;
   size_t offset;
   bool_t duplicate;
   DnsSdContext *context;
   DnsResourceRecord *record;

   //Point to the DNS-SD context
   context = interface->dnsSdContext;

   //Check whether the resource record is already present in the Answer
   //Section of the message
   duplicate = mdnsCheckDuplicateRecord(message, "",
      service->name, ".local", DNS_RR_TYPE_PTR);

   //The duplicates should be suppressed and the resource record should
   //appear only once in the list
   if(!duplicate)
   {
      //Set the position to the end of the buffer
      offset = message->length;

      //The first pass calculates the length of the DNS encoded service name
      n = mdnsEncodeName("", service->name, ".local", NULL);

      //Check the length of the resulting mDNS message
      if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //Encode the service name using the DNS name notation
      offset += mdnsEncodeName("", service->name,
         ".local", (uint8_t *) message->dnsHeader + offset);

      //Consider the length of the resource record itself
      if((offset + sizeof(DnsResourceRecord)) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //Point to the corresponding resource record
      record = DNS_GET_RESOURCE_RECORD(message->dnsHeader, offset);

      //Fill in resource record
      record->rtype = HTONS(DNS_RR_TYPE_PTR);
      record->rclass = HTONS(DNS_RR_CLASS_IN);
      record->ttl = htonl(ttl);

      //Advance write index
      offset += sizeof(DnsResourceRecord);

      //The first pass calculates the length of the DNS encoded instance name
      n = mdnsEncodeName(context->instanceName, service->name, ".local", NULL);

      //Check the length of the resulting mDNS message
      if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //The second pass encodes the instance name using DNS notation
      n = mdnsEncodeName(context->instanceName,
         service->name, ".local", record->rdata);

      //Convert length field to network byte order
      record->rdlength = htons(n);

      //Number of resource records in the answer section
      message->dnsHeader->ancount++;
      //Update the length of the DNS message
      message->length = offset + n;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add SRV record to a mDNS message
 * @param[in] interface Underlying network interface
 * @param[in,out] message Pointer to the mDNS message
 * @param[in] service Pointer to a DNS-SD service
 * @param[in] cacheFlush Cache-flush bit
 * @param[in] ttl Resource record TTL (cache lifetime)
 * @return Error code
 **/

error_t dnsSdAddSrvRecord(NetInterface *interface, MdnsMessage *message,
   const DnsSdService *service, bool_t cacheFlush, uint32_t ttl)
{
   size_t n;
   size_t offset;
   bool_t duplicate;
   MdnsResponderContext *mdnsResponderContext;
   DnsSdContext *dnsSdContext;
   DnsSrvResourceRecord *record;

   //Point to the mDNS responder context
   mdnsResponderContext = interface->mdnsResponderContext;
   //Point to the DNS-SD context
   dnsSdContext = interface->dnsSdContext;

   //Check whether the resource record is already present in the Answer
   //Section of the message
   duplicate = mdnsCheckDuplicateRecord(message, dnsSdContext->instanceName,
      service->name, ".local", DNS_RR_TYPE_SRV);

   //The duplicates should be suppressed and the resource record should
   //appear only once in the list
   if(!duplicate)
   {
      //Set the position to the end of the buffer
      offset = message->length;

      //The first pass calculates the length of the DNS encoded instance name
      n = mdnsEncodeName(dnsSdContext->instanceName,
         service->name, ".local", NULL);

      //Check the length of the resulting mDNS message
      if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //The second pass encodes the instance name using DNS notation
      offset += mdnsEncodeName(dnsSdContext->instanceName,
         service->name, ".local", (uint8_t *) message->dnsHeader + offset);

      //Consider the length of the resource record itself
      if((offset + sizeof(DnsSrvResourceRecord)) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //Point to the corresponding resource record
      record = (DnsSrvResourceRecord *) DNS_GET_RESOURCE_RECORD(message->dnsHeader, offset);

      //Fill in resource record
      record->rtype = HTONS(DNS_RR_TYPE_SRV);
      record->rclass = HTONS(DNS_RR_CLASS_IN);
      record->ttl = htonl(ttl);
      record->priority = htons(service->priority);
      record->weight = htons(service->weight);
      record->port = htons(service->port);

      //Check whether the cache-flush bit should be set
      if(cacheFlush)
         record->rclass |= HTONS(MDNS_RCLASS_CACHE_FLUSH);

      //Advance write index
      offset += sizeof(DnsSrvResourceRecord);

      //The first pass calculates the length of the DNS encoded target name
      n = mdnsEncodeName("", mdnsResponderContext->hostname,
         ".local", NULL);

      //Check the length of the resulting mDNS message
      if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //The second pass encodes the target name using DNS notation
      n = mdnsEncodeName("", mdnsResponderContext->hostname,
         ".local", record->target);

      //Calculate data length
      record->rdlength = htons(sizeof(DnsSrvResourceRecord) -
         sizeof(DnsResourceRecord) + n);

      //Number of resource records in the answer section
      message->dnsHeader->ancount++;
      //Update the length of the DNS message
      message->length = offset + n;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add TXT record to a mDNS message
 * @param[in] interface Underlying network interface
 * @param[in,out] message Pointer to the mDNS message
 * @param[in] service Pointer to a DNS-SD service
 * @param[in] cacheFlush Cache-flush bit
 * @param[in] ttl Resource record TTL (cache lifetime)
 * @return Error code
 **/

error_t dnsSdAddTxtRecord(NetInterface *interface, MdnsMessage *message,
   const DnsSdService *service, bool_t cacheFlush, uint32_t ttl)
{
   size_t n;
   size_t offset;
   bool_t duplicate;
   DnsSdContext *context;
   DnsResourceRecord *record;

   //Point to the DNS-SD context
   context = interface->dnsSdContext;

   //Check whether the resource record is already present in the Answer
   //Section of the message
   duplicate = mdnsCheckDuplicateRecord(message, context->instanceName,
      service->name, ".local", DNS_RR_TYPE_TXT);

   //The duplicates should be suppressed and the resource record should
   //appear only once in the list
   if(!duplicate)
   {
      //Set the position to the end of the buffer
      offset = message->length;

      //The first pass calculates the length of the DNS encoded instance name
      n = mdnsEncodeName(context->instanceName, service->name, ".local", NULL);

      //Check the length of the resulting mDNS message
      if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //The second pass encodes the instance name using DNS notation
      offset += mdnsEncodeName(context->instanceName,
         service->name, ".local", (uint8_t *) message->dnsHeader + offset);

      //Consider the length of the resource record itself
      if((offset + sizeof(DnsResourceRecord)) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //Point to the corresponding resource record
      record = DNS_GET_RESOURCE_RECORD(message->dnsHeader, offset);

      //Fill in resource record
      record->rtype = HTONS(DNS_RR_TYPE_TXT);
      record->rclass = HTONS(DNS_RR_CLASS_IN);
      record->ttl = htonl(ttl);
      record->rdlength = htons(service->metadataLength);

      //Check whether the cache-flush bit should be set
      if(cacheFlush)
         record->rclass |= HTONS(MDNS_RCLASS_CACHE_FLUSH);

      //Advance write index
      offset += sizeof(DnsResourceRecord);

      //Check the length of the resulting mDNS message
      if((offset + service->metadataLength) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //Copy metadata
      osMemcpy(record->rdata, service->metadata, service->metadataLength);

      //Update the length of the DNS message
      message->length = offset + service->metadataLength;
      //Number of resource records in the answer section
      message->dnsHeader->ancount++;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add NSEC record to a mDNS message
 * @param[in] interface Underlying network interface
 * @param[in,out] message Pointer to the mDNS message
 * @param[in] service Pointer to a DNS-SD service
 * @param[in] cacheFlush Cache-flush bit
 * @param[in] ttl Resource record TTL (cache lifetime)
 * @return Error code
 **/

error_t dnsSdAddNsecRecord(NetInterface *interface, MdnsMessage *message,
   const DnsSdService *service, bool_t cacheFlush, uint32_t ttl)
{
   size_t n;
   size_t offset;
   bool_t duplicate;
   size_t bitmapLength;
   uint8_t bitmap[8];
   DnsSdContext *context;
   DnsResourceRecord *record;

   //Point to the DNS-SD context
   context = interface->dnsSdContext;

   //Check whether the resource record is already present in the Answer
   //Section of the message
   duplicate = mdnsCheckDuplicateRecord(message, context->instanceName,
      service->name, ".local", DNS_RR_TYPE_NSEC);

   //The duplicates should be suppressed and the resource record should
   //appear only once in the list
   if(!duplicate)
   {
      //The bitmap identifies the resource record types that exist
      osMemset(bitmap, 0, sizeof(bitmap));

      //TXT resource record is supported
      DNS_SET_NSEC_BITMAP(bitmap, DNS_RR_TYPE_TXT);
      //SRV resource record is supported
      DNS_SET_NSEC_BITMAP(bitmap, DNS_RR_TYPE_SRV);

      //Compute the length of the bitmap
      for(bitmapLength = sizeof(bitmap); bitmapLength > 0; bitmapLength--)
      {
         //Trailing zero octets in the bitmap must be omitted...
         if(bitmap[bitmapLength - 1] != 0x00)
            break;
      }

      //Set the position to the end of the buffer
      offset = message->length;

      //The first pass calculates the length of the DNS encoded instance name
      n = mdnsEncodeName(context->instanceName, service->name, ".local", NULL);

      //Check the length of the resulting mDNS message
      if((offset + n) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //The second pass encodes the instance name using the DNS name notation
      offset += mdnsEncodeName(context->instanceName, service->name,
         ".local", (uint8_t *) message->dnsHeader + offset);

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
      if((offset + n + 2) > MDNS_MESSAGE_MAX_SIZE)
         return ERROR_MESSAGE_TOO_LONG;

      //The Next Domain Name field contains the record's own name
      mdnsEncodeName(context->instanceName, service->name,
         ".local", record->rdata);

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
