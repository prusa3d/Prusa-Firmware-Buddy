/**
 * @file nbns_client.c
 * @brief NBNS client (NetBIOS Name Service)
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
#define TRACE_LEVEL NBNS_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "ipv4/ipv4_misc.h"
#include "netbios/nbns_client.h"
#include "netbios/nbns_common.h"
#include "dns/dns_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (NBNS_CLIENT_SUPPORT == ENABLED && IPV4_SUPPORT == ENABLED)


/**
 * @brief Resolve a host name using NBNS
 * @param[in] interface Underlying network interface
 * @param[in] name Name of the host to be resolved
 * @param[out] ipAddr IP address corresponding to the specified host name
 **/

error_t nbnsResolve(NetInterface *interface, const char_t *name, IpAddr *ipAddr)
{
   error_t error;
   DnsCacheEntry *entry;

#if (NET_RTOS_SUPPORT == ENABLED)
   systime_t delay;

   //Debug message
   TRACE_INFO("Resolving host name %s (NBNS resolver)...\r\n", name);
#endif

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Search the DNS cache for the specified host name
   entry = dnsFindEntry(interface, name, HOST_TYPE_IPV4, HOST_NAME_RESOLVER_NBNS);

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
      entry->type = HOST_TYPE_IPV4;
      entry->protocol = HOST_NAME_RESOLVER_NBNS;
      entry->interface = interface;

      //Initialize retransmission counter
      entry->retransmitCount = NBNS_CLIENT_MAX_RETRIES;
      //Send NBNS query
      error = nbnsSendQuery(entry);

      //NBNS message successfully sent?
      if(!error)
      {
         //Save the time at which the query message was sent
         entry->timestamp = osGetSystemTime();
         //Set timeout value
         entry->timeout = NBNS_CLIENT_INIT_TIMEOUT;
         entry->maxTimeout = NBNS_CLIENT_MAX_TIMEOUT;
         //Decrement retransmission counter
         entry->retransmitCount--;

         //Switch state
         entry->state = DNS_STATE_IN_PROGRESS;
         //Host name resolution is in progress
         error = ERROR_IN_PROGRESS;
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
      entry = dnsFindEntry(interface, name, HOST_TYPE_IPV4, HOST_NAME_RESOLVER_NBNS);

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
 * @brief Send a NBNS query message
 * @param[in] entry Pointer to a valid DNS cache entry
 * @return Error code
 **/

error_t nbnsSendQuery(DnsCacheEntry *entry)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   NbnsHeader *message;
   DnsQuestion *dnsQuestion;
   IpAddr destIpAddr;
   NetTxAncillary ancillary;

   //Allocate a memory buffer to hold the NBNS query message
   buffer = udpAllocBuffer(DNS_MESSAGE_MAX_SIZE, &offset);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the NBNS header
   message = netBufferAt(buffer, offset);

   //Format NBNS query message
   message->id = htons(entry->id);
   message->qr = 0;
   message->opcode = DNS_OPCODE_QUERY;
   message->aa = 0;
   message->tc = 0;
   message->rd = 0;
   message->ra = 0;
   message->z = 0;
   message->b = 1;
   message->rcode = DNS_RCODE_NO_ERROR;

   //The NBNS query contains one question
   message->qdcount = HTONS(1);
   message->ancount = 0;
   message->nscount = 0;
   message->arcount = 0;

   //Length of the NBNS query message
   length = sizeof(DnsHeader);

   //Encode the NetBIOS name
   length += nbnsEncodeName(entry->name, message->questions);

   //Point to the corresponding question structure
   dnsQuestion = DNS_GET_QUESTION(message, length);
   //Fill in question structure
   dnsQuestion->qtype = HTONS(DNS_RR_TYPE_NB);
   dnsQuestion->qclass = HTONS(DNS_RR_CLASS_IN);

   //Update the length of the NBNS query message
   length += sizeof(DnsQuestion);

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + length);

   //Debug message
   TRACE_INFO("Sending NBNS message (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message
   dnsDumpMessage((DnsHeader *) message, length);

   //The destination address is the broadcast address
   destIpAddr.length = sizeof(Ipv4Addr);
   ipv4GetBroadcastAddr(entry->interface, &destIpAddr.ipv4Addr);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //A request packet is always sent to the well known port 137
   error = udpSendBuffer(entry->interface, NULL, NBNS_PORT, &destIpAddr,
      NBNS_PORT, buffer, offset, &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);

   //Return status code
   return error;
}


/**
 * @brief Process NBNS response message
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @param[in] udpHeader UDP header
 * @param[in] message Pointer to the NBNS response message
 * @param[in] length Length of the message
 **/

void nbnsProcessResponse(NetInterface *interface, const Ipv4PseudoHeader *pseudoHeader,
   const UdpHeader *udpHeader, const NbnsHeader *message, size_t length)
{
   uint_t i;
   size_t pos;
   DnsCacheEntry *entry;
   DnsResourceRecord *record;
   NbnsAddrEntry *addrEntry;

   //The NBNS response shall contain one answer
   if(ntohs(message->qdcount) != 0 && ntohs(message->ancount) != 1)
      return;

   //Parse NetBIOS name
   pos = nbnsParseName(message, length, sizeof(DnsHeader), NULL);
   //Invalid name?
   if(!pos)
      return;

   //Point to the associated resource record
   record = DNS_GET_RESOURCE_RECORD(message, pos);
   //Point to the resource data
   pos += sizeof(DnsResourceRecord);

   //Make sure the resource record is valid
   if(pos > length)
      return;
   if((pos + ntohs(record->rdlength)) > length)
      return;

   //Check the class and the type of the resource record
   if(ntohs(record->rclass) != DNS_RR_CLASS_IN)
      return;
   if(ntohs(record->rtype) != DNS_RR_TYPE_NB)
      return;

   //Verify the length of the data field
   if(ntohs(record->rdlength) < sizeof(NbnsAddrEntry))
      return;

   //Loop through DNS cache entries
   for(i = 0; i < DNS_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &dnsCache[i];

      //NBNS name resolution in progress?
      if(entry->state == DNS_STATE_IN_PROGRESS &&
         entry->protocol == HOST_NAME_RESOLVER_NBNS &&
         entry->type == HOST_TYPE_IPV4)
      {
         //Compare identifiers
         if(entry->id == ntohs(message->id))
         {
            //Compare NetBIOS names
            if(nbnsCompareName(message, length, sizeof(DnsHeader), entry->name))
            {
               //Point to the address entry array
               addrEntry = (NbnsAddrEntry *) record->rdata;
               //Copy the IPv4 address
               entry->ipAddr.length = sizeof(Ipv4Addr);
               entry->ipAddr.ipv4Addr = addrEntry->addr;

               //Save current time
               entry->timestamp = osGetSystemTime();
               //Save TTL value
               entry->timeout = ntohl(record->ttl) * 1000;
               //Limit the lifetime of the NBNS cache entries
               entry->timeout = MIN(entry->timeout, NBNS_MAX_LIFETIME);

               //Host name successfully resolved
               entry->state = DNS_STATE_RESOLVED;
            }
         }
      }
   }
}

#endif
