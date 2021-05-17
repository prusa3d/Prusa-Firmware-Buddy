/**
 * @file llmnr_client.c
 * @brief LLMNR client (Link-Local Multicast Name Resolution)
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
#define TRACE_LEVEL LLMNR_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "ipv4/ipv4_misc.h"
#include "llmnr/llmnr_client.h"
#include "llmnr/llmnr_common.h"
#include "dns/dns_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (LLMNR_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Resolve a host name using LLMNR
 * @param[in] interface Underlying network interface
 * @param[in] name Name of the host to be resolved
 * @param[in] type Host type (IPv4 or IPv6)
 * @param[out] ipAddr IP address corresponding to the specified host name
 **/

error_t llmnrResolve(NetInterface *interface, const char_t *name,
   HostType type, IpAddr *ipAddr)
{
   error_t error;
   DnsCacheEntry *entry;

#if (NET_RTOS_SUPPORT == ENABLED)
   systime_t delay;

   //Debug message
   TRACE_INFO("Resolving host name %s (LLMNR resolver)...\r\n", name);
#endif

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Search the DNS cache for the specified host name
   entry = dnsFindEntry(interface, name, type, HOST_NAME_RESOLVER_LLMNR);

   //Check whether a matching entry has been found
   if(entry)
   {
      //Host name already resolved?
      if(entry->state == DNS_STATE_RESOLVED ||
         entry->state == DNS_STATE_PERMANENT)
      {
         //Return the corresponding IP address
         *ipAddr = entry->ipAddr;
         //Successful host name resolution
         error = NO_ERROR;
      }
      else
      {
         //Host name resolution is in progress...
         error = ERROR_IN_PROGRESS;
      }
   }
   else
   {
      //If no entry exists, then create a new one
      entry = dnsCreateEntry();

      //Record the host name whose IP address is unknown
      osStrcpy(entry->name, name);

      //Initialize DNS cache entry
      entry->type = type;
      entry->protocol = HOST_NAME_RESOLVER_LLMNR;
      entry->interface = interface;

      //Get an ephemeral port number
      entry->port = udpGetDynamicPort();

      //An identifier is used by the LLMNR client to match replies
      //with corresponding requests
      entry->id = (uint16_t) netGetRand();

      //Callback function to be called when a LLMNR response is received
      error = udpAttachRxCallback(interface, entry->port, llmnrProcessResponse,
         NULL);

      //Check status code
      if(!error)
      {
         //Initialize retransmission counter
         entry->retransmitCount = LLMNR_CLIENT_MAX_RETRIES;
         //Send LLMNR query
         error = llmnrSendQuery(entry);

         //LLMNR message successfully sent?
         if(!error)
         {
            //Save the time at which the query message was sent
            entry->timestamp = osGetSystemTime();
            //Set timeout value
            entry->timeout = LLMNR_CLIENT_INIT_TIMEOUT;
            entry->maxTimeout = LLMNR_CLIENT_MAX_TIMEOUT;
            //Decrement retransmission counter
            entry->retransmitCount--;

            //Switch state
            entry->state = DNS_STATE_IN_PROGRESS;
            //Host name resolution is in progress
            error = ERROR_IN_PROGRESS;
         }
         else
         {
            //Unregister callback function
            udpDetachRxCallback(interface, entry->port);
         }
      }
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

#if (NET_RTOS_SUPPORT == ENABLED)
   //Set default polling interval
   delay = DNS_CACHE_INIT_POLLING_INTERVAL;

   //Wait the host name resolution to complete
   while(error == ERROR_IN_PROGRESS)
   {
      //Wait until the next polling period
      osDelayTask(delay);

      //Get exclusive access
      osAcquireMutex(&netMutex);

      //Search the DNS cache for the specified host name
      entry = dnsFindEntry(interface, name, type, HOST_NAME_RESOLVER_LLMNR);

      //Check whether a matching entry has been found
      if(entry)
      {
         //Host name successfully resolved?
         if(entry->state == DNS_STATE_RESOLVED)
         {
            //Return the corresponding IP address
            *ipAddr = entry->ipAddr;
            //Successful host name resolution
            error = NO_ERROR;
         }
      }
      else
      {
         //Host name resolution failed
         error = ERROR_FAILURE;
      }

      //Release exclusive access
      osReleaseMutex(&netMutex);

      //Backoff support for less aggressive polling
      delay = MIN(delay * 2, DNS_CACHE_MAX_POLLING_INTERVAL);
   }

   //Check status code
   if(error)
   {
      //Failed to resolve host name
      TRACE_INFO("Host name resolution failed!\r\n");
   }
   else
   {
      //Successful host name resolution
      TRACE_INFO("Host name resolved to %s...\r\n", ipAddrToString(ipAddr, NULL));
   }
#endif

   //Return status code
   return error;
}


/**
 * @brief Send a LLMNR query message
 * @param[in] entry Pointer to a valid DNS cache entry
 * @return Error code
 **/

error_t llmnrSendQuery(DnsCacheEntry *entry)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   LlmnrHeader *message;
   DnsQuestion *dnsQuestion;
   IpAddr destIpAddr;
   NetTxAncillary ancillary;

#if (IPV4_SUPPORT == ENABLED)
   //An IPv4 address is expected?
   if(entry->type == HOST_TYPE_IPV4)
   {
      //The IPv4 link-scope multicast address to which a sender sends queries
      //is 224.0.0.252
      destIpAddr.length = sizeof(Ipv4Addr);
      destIpAddr.ipv4Addr = LLMNR_IPV4_MULTICAST_ADDR;
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //An IPv6 address is expected?
   if(entry->type == HOST_TYPE_IPV6)
   {
      //The IPv6 link-scope multicast address to which a sender sends queries
      //is ff02:0:0:0:0:0:1:3
      destIpAddr.length = sizeof(Ipv6Addr);
      destIpAddr.ipv6Addr = LLMNR_IPV6_MULTICAST_ADDR;
   }
   else
#endif
   //Invalid host type?
   {
      //Report an error
      return ERROR_INVALID_PARAMETER;
   }

   //Allocate a memory buffer to hold the LLMNR query message
   buffer = udpAllocBuffer(DNS_MESSAGE_MAX_SIZE, &offset);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the LLMNR header
   message = netBufferAt(buffer, offset);

   //Format LLMNR query message
   message->id = htons(entry->id);
   message->qr = 0;
   message->opcode = DNS_OPCODE_QUERY;
   message->c = 0;
   message->tc = 0;
   message->t = 0;
   message->z = 0;
   message->rcode = DNS_RCODE_NO_ERROR;

   //The LLMNR query contains one question
   message->qdcount = HTONS(1);
   message->ancount = 0;
   message->nscount = 0;
   message->arcount = 0;

   //Length of the LLMNR query message
   length = sizeof(DnsHeader);

   //Encode the host name using the DNS name notation
   length += dnsEncodeName(entry->name, message->questions);

   //Point to the corresponding question structure
   dnsQuestion = DNS_GET_QUESTION(message, length);

#if (IPV4_SUPPORT == ENABLED)
   //An IPv4 address is expected?
   if(entry->type == HOST_TYPE_IPV4)
   {
      //Fill in question structure
      dnsQuestion->qtype = HTONS(DNS_RR_TYPE_A);
      dnsQuestion->qclass = HTONS(DNS_RR_CLASS_IN);
   }
#endif
#if (IPV6_SUPPORT == ENABLED)
   //An IPv6 address is expected?
   if(entry->type == HOST_TYPE_IPV6)
   {
      //Fill in question structure
      dnsQuestion->qtype = HTONS(DNS_RR_TYPE_AAAA);
      dnsQuestion->qclass = HTONS(DNS_RR_CLASS_IN);
   }
#endif

   //Update the length of the LLMNR query message
   length += sizeof(DnsQuestion);

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + length);

   //Debug message
   TRACE_INFO("Sending LLMNR message (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message
   dnsDumpMessage((DnsHeader *) message, length);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //LLMNR queries are sent to and received on port 5355
   error = udpSendBuffer(entry->interface, NULL, entry->port, &destIpAddr,
      LLMNR_PORT, buffer, offset, &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);

   //Return status code
   return error;
}


/**
 * @brief Process LLMNR response message
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @param[in] udpHeader UDP header
 * @param[in] buffer Multi-part buffer containing the incoming LLMNR message
 * @param[in] offset Offset to the first byte of the LLMNR message
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @param[in] param Callback function parameter (not used)
 **/

void llmnrProcessResponse(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *udpHeader,
   const NetBuffer *buffer, size_t offset, const NetRxAncillary *ancillary,
   void *param)
{
   uint_t i;
   uint_t j;
   size_t pos;
   size_t length;
   LlmnrHeader *message;
   DnsQuestion *question;
   DnsResourceRecord *record;
   DnsCacheEntry *entry;

   //Retrieve the length of the LLMNR message
   length = netBufferGetLength(buffer) - offset;

   //Ensure the LLMNR message is valid
   if(length < sizeof(LlmnrHeader))
      return;
   if(length > DNS_MESSAGE_MAX_SIZE)
      return;

   //Point to the LLMNR message header
   message = netBufferAt(buffer, offset);
   //Sanity check
   if(message == NULL)
      return;

   //Debug message
   TRACE_INFO("LLMNR message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message
   dnsDumpMessage((DnsHeader *) message, length);

   //Discard LLMNR queries
   if(!message->qr)
      return;

   //LLMNR messages received with an opcode other than zero must be silently
   //ignored
   if(message->opcode != DNS_OPCODE_QUERY)
      return;

   //LLMNR messages received with non-zero response codes must be silently
   //ignored
   if(message->rcode != DNS_RCODE_NO_ERROR)
      return;

   //LLMNR senders must silently discard LLMNR responses with QDCOUNT not
   //equal to one (refer to RFC 4795, section 2.1.1)
   if(ntohs(message->qdcount) != 1)
      return;

   //Loop through DNS cache entries
   for(i = 0; i < DNS_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &dnsCache[i];

      //LLMNR name resolution in progress?
      if(entry->state == DNS_STATE_IN_PROGRESS &&
         entry->protocol == HOST_NAME_RESOLVER_LLMNR)
      {
         //Check destination port number
         if(entry->port == ntohs(udpHeader->destPort))
         {
            //Compare identifier against the expected one
            if(ntohs(message->id) != entry->id)
               break;

            //Point to the first question
            pos = sizeof(DnsHeader);
            //Parse domain name
            pos = dnsParseName((DnsHeader *) message, length, pos, NULL, 0);

            //Invalid name?
            if(!pos)
               break;
            //Malformed DNS message?
            if((pos + sizeof(DnsQuestion)) > length)
               break;

            //Compare domain name
            if(dnsCompareName((DnsHeader *) message, length, sizeof(DnsHeader),
               entry->name, 0))
            {
               break;
            }

            //Point to the corresponding entry
            question = DNS_GET_QUESTION(message, pos);

            //Check the class of the query
            if(ntohs(question->qclass) != DNS_RR_CLASS_IN)
               break;

            //Check the type of the query
            if(entry->type == HOST_TYPE_IPV4 && ntohs(question->qtype) != DNS_RR_TYPE_A)
               break;
            if(entry->type == HOST_TYPE_IPV6 && ntohs(question->qtype) != DNS_RR_TYPE_AAAA)
               break;

            //Point to the first answer
            pos += sizeof(DnsQuestion);

            //Parse answer resource records
            for(j = 0; j < ntohs(message->ancount); j++)
            {
               //Parse domain name
               pos = dnsParseName((DnsHeader *) message, length, pos, NULL, 0);
               //Invalid name?
               if(!pos)
                  break;

               //Point to the associated resource record
               record = DNS_GET_RESOURCE_RECORD(message, pos);
               //Point to the resource data
               pos += sizeof(DnsResourceRecord);

               //Make sure the resource record is valid
               if(pos > length)
                  break;
               if((pos + ntohs(record->rdlength)) > length)
                  break;

#if (IPV4_SUPPORT == ENABLED)
               //IPv4 address expected?
               if(entry->type == HOST_TYPE_IPV4)
               {
                  //A resource record found?
                  if(ntohs(record->rtype) == DNS_RR_TYPE_A &&
                     ntohs(record->rdlength) == sizeof(Ipv4Addr))
                  {
                     //Copy the IPv4 address
                     entry->ipAddr.length = sizeof(Ipv4Addr);
                     ipv4CopyAddr(&entry->ipAddr.ipv4Addr, record->rdata);

                     //Save current time
                     entry->timestamp = osGetSystemTime();
                     //Save TTL value
                     entry->timeout = ntohl(record->ttl) * 1000;
                     //Limit the lifetime of the NBNS cache entries
                     entry->timeout = MIN(entry->timeout, LLMNR_MAX_LIFETIME);

                     //Unregister UDP callback function
                     udpDetachRxCallback(interface, entry->port);
                     //Host name successfully resolved
                     entry->state = DNS_STATE_RESOLVED;
                     //Exit immediately
                     break;
                  }
               }
#endif
#if (IPV6_SUPPORT == ENABLED)
               //IPv6 address expected?
               if(entry->type == HOST_TYPE_IPV6)
               {
                  //AAAA resource record found?
                  if(ntohs(record->rtype) == DNS_RR_TYPE_AAAA &&
                     ntohs(record->rdlength) == sizeof(Ipv6Addr))
                  {
                     //Copy the IPv6 address
                     entry->ipAddr.length = sizeof(Ipv6Addr);
                     ipv6CopyAddr(&entry->ipAddr.ipv6Addr, record->rdata);

                     //Save current time
                     entry->timestamp = osGetSystemTime();
                     //Save TTL value
                     entry->timeout = ntohl(record->ttl) * 1000;
                     //Limit the lifetime of the NBNS cache entries
                     entry->timeout = MIN(entry->timeout, LLMNR_MAX_LIFETIME);

                     //Unregister UDP callback function
                     udpDetachRxCallback(interface, entry->port);
                     //Host name successfully resolved
                     entry->state = DNS_STATE_RESOLVED;
                     //Exit immediately
                     break;
                  }
               }
#endif
               //Point to the next resource record
               pos += ntohs(record->rdlength);
            }

            //We are done
            break;
         }
      }
   }
}

#endif
