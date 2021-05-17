/**
 * @file mdns_client.c
 * @brief mDNS client (Multicast DNS)
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
#include "core/net.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "mdns/mdns_client.h"
#include "mdns/mdns_responder.h"
#include "mdns/mdns_common.h"
#include "dns/dns_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MDNS_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Resolve a host name using mDNS
 * @param[in] interface Underlying network interface
 * @param[in] name Name of the host to be resolved
 * @param[in] type Host type (IPv4 or IPv6)
 * @param[out] ipAddr IP address corresponding to the specified host name
 **/

error_t mdnsClientResolve(NetInterface *interface, const char_t *name,
   HostType type, IpAddr *ipAddr)
{
   error_t error;
   DnsCacheEntry *entry;

#if (NET_RTOS_SUPPORT == ENABLED)
   systime_t delay;

   //Debug message
   TRACE_INFO("Resolving host name %s (mDNS resolver)...\r\n", name);
#endif

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Search the DNS cache for the specified host name
   entry = dnsFindEntry(interface, name, type, HOST_NAME_RESOLVER_MDNS);

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
      entry->protocol = HOST_NAME_RESOLVER_MDNS;
      entry->interface = interface;

      //Initialize retransmission counter
      entry->retransmitCount = MDNS_CLIENT_MAX_RETRIES;
      //Send mDNS query
      error = mdnsClientSendQuery(entry);

      //mDNS message successfully sent?
      if(!error)
      {
         //Save the time at which the query message was sent
         entry->timestamp = osGetSystemTime();
         //Set timeout value
         entry->timeout = MDNS_CLIENT_INIT_TIMEOUT;
         entry->maxTimeout = MDNS_CLIENT_MAX_TIMEOUT;
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
      entry = dnsFindEntry(interface, name, type, HOST_NAME_RESOLVER_MDNS);

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
 * @brief Send a mDNS query message
 * @param[in] entry Pointer to a valid DNS cache entry
 * @return Error code
 **/

error_t mdnsClientSendQuery(DnsCacheEntry *entry)
{
   error_t error;
   DnsQuestion *dnsQuestion;
   MdnsMessage message;

   //Create an empty mDNS query message
   error = mdnsCreateMessage(&message, FALSE);
   //Any error to report?
   if(error)
      return error;

   //Encode the host name using the DNS name notation
   message.length += dnsEncodeName(entry->name, message.dnsHeader->questions);

   //Point to the corresponding question structure
   dnsQuestion = DNS_GET_QUESTION(message.dnsHeader, message.length);

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

   //Update the length of the mDNS query message
   message.length += sizeof(DnsQuestion);
   //Number of questions in the Question Section
   message.dnsHeader->qdcount = 1;

   //Send mDNS message
   error = mdnsSendMessage(entry->interface, &message, NULL, MDNS_PORT);

   //Free previously allocated memory
   mdnsDeleteMessage(&message);

   //Return status code
   return error;
}


/**
 * @brief Parse a resource record from the Answer Section
 * @param[in] interface Underlying network interface
 * @param[in] message Pointer to the mDNS message
 * @param[in] offset Offset to first byte of the resource record
 * @param[in] record Pointer to the resource record
 **/

void mdnsClientParseAnRecord(NetInterface *interface,
   const MdnsMessage *message, size_t offset, const DnsResourceRecord *record)
{
   uint_t i;
   uint16_t rclass;
   DnsCacheEntry *entry;

   //Loop through DNS cache entries
   for(i = 0; i < DNS_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &dnsCache[i];

      //mDNS name resolution in progress?
      if(entry->state == DNS_STATE_IN_PROGRESS &&
         entry->protocol == HOST_NAME_RESOLVER_MDNS)
      {
         //Compare resource record name
         if(!dnsCompareName(message->dnsHeader, message->length, offset, entry->name, 0))
         {
            //Convert the class to host byte order
            rclass = ntohs(record->rclass);
            //Discard Cache Flush flag
            rclass &= ~MDNS_RCLASS_CACHE_FLUSH;

            //Check the class of the resource record
            if(rclass == DNS_RR_CLASS_IN)
            {
#if (IPV4_SUPPORT == ENABLED)
               //IPv4 address expected?
               if(entry->type == HOST_TYPE_IPV4)
               {
                  //A resource record found?
                  if(ntohs(record->rtype) == DNS_RR_TYPE_A)
                  {
                     //Verify the length of the data field
                     if(ntohs(record->rdlength) == sizeof(Ipv4Addr))
                     {
                        //Copy the IPv4 address
                        entry->ipAddr.length = sizeof(Ipv4Addr);
                        ipv4CopyAddr(&entry->ipAddr.ipv4Addr, record->rdata);

                        //Save current time
                        entry->timestamp = osGetSystemTime();
                        //Save TTL value
                        entry->timeout = ntohl(record->ttl) * 1000;
                        //Limit the lifetime of the mDNS cache entries
                        entry->timeout = MIN(entry->timeout, MDNS_MAX_LIFETIME);

                        //Host name successfully resolved
                        entry->state = DNS_STATE_RESOLVED;
                     }
                  }
               }
#endif
#if (IPV6_SUPPORT == ENABLED)
               //IPv6 address expected?
               if(entry->type == HOST_TYPE_IPV6)
               {
                  //AAAA resource record found?
                  if(ntohs(record->rtype) == DNS_RR_TYPE_AAAA)
                  {
                     //Verify the length of the data field
                     if(ntohs(record->rdlength) == sizeof(Ipv6Addr))
                     {
                        //Copy the IPv6 address
                        entry->ipAddr.length = sizeof(Ipv6Addr);
                        ipv6CopyAddr(&entry->ipAddr.ipv6Addr, record->rdata);

                        //Save current time
                        entry->timestamp = osGetSystemTime();
                        //Save TTL value
                        entry->timeout = ntohl(record->ttl) * 1000;
                        //Limit the lifetime of the mDNS cache entries
                        entry->timeout = MIN(entry->timeout, MDNS_MAX_LIFETIME);

                        //Host name successfully resolved
                        entry->state = DNS_STATE_RESOLVED;
                     }
                  }
               }
#endif
            }
         }
      }
   }
}

#endif
