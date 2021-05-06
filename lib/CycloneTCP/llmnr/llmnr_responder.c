/**
 * @file llmnr_responder.c
 * @brief LLMNR responder (Link-Local Multicast Name Resolution)
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
#include "llmnr/llmnr_responder.h"
#include "llmnr/llmnr_common.h"
#include "dns/dns_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (LLMNR_RESPONDER_SUPPORT == ENABLED)


/**
 * @brief LLMNR responder initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t llmnrResponderInit(NetInterface *interface)
{
   error_t error;

#if (IPV4_SUPPORT == ENABLED)
   //Join the LLMNR IPv4 multicast group
   error = ipv4JoinMulticastGroup(interface, LLMNR_IPV4_MULTICAST_ADDR);
   //Any error to report?
   if(error)
      return error;
#endif

#if (IPV6_SUPPORT == ENABLED)
   //Join the LLMNR IPv6 multicast group
   error = ipv6JoinMulticastGroup(interface, &LLMNR_IPV6_MULTICAST_ADDR);
   //Any error to report?
   if(error)
      return error;
#endif

   //LLMNR responders must listen on UDP port 5355
   error = udpAttachRxCallback(interface, LLMNR_PORT, llmnrProcessQuery,
      NULL);
   //Any error to report?
   if(error)
      return error;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Process LLMNR query message
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @param[in] udpHeader UDP header
 * @param[in] buffer Multi-part buffer containing the incoming LLMNR message
 * @param[in] offset Offset to the first byte of the LLMNR message
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @param[in] param Callback function parameter (not used)
 **/

void llmnrProcessQuery(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *udpHeader,
   const NetBuffer *buffer, size_t offset, const NetRxAncillary *ancillary,
   void *param)
{
   size_t n;
   size_t pos;
   size_t length;
   uint16_t destPort;
   IpAddr destIpAddr;
   LlmnrHeader *message;
   DnsQuestion *question;

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

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 query received?
   if(pseudoHeader->length == sizeof(Ipv4PseudoHeader))
   {
      //Unicast UDP queries must be silently discarded (refer to RFC 4795,
      //section 2.4)
      if(!ipv4IsMulticastAddr(pseudoHeader->ipv4Data.destAddr))
         return;

      //A responder responds to a multicast query by sending a unicast UDP
      //response to the sender (refer to RFC 4795, section 2)
      destIpAddr.length = sizeof(Ipv4Addr);
      destIpAddr.ipv4Addr = pseudoHeader->ipv4Data.srcAddr;
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 query received?
   if(pseudoHeader->length == sizeof(Ipv6PseudoHeader))
   {
      //Unicast UDP queries must be silently discarded (refer to RFC 4795,
      //section 2.4)
      if(!ipv6IsMulticastAddr(&pseudoHeader->ipv6Data.destAddr))
         return;

      //A responder responds to a multicast query by sending a unicast UDP
      //response to the sender (refer to RFC 4795, section 2)
      destIpAddr.length = sizeof(Ipv6Addr);
      destIpAddr.ipv6Addr = pseudoHeader->ipv6Data.srcAddr;
   }
   else
#endif
   //Invalid query received?
   {
      //Discard the LLMNR query message
      return;
   }

   //Discard LLMNR responses received on port 5355
   if(message->qr)
      return;

   //LLMNR messages received with an opcode other than zero must be silently
   //ignored
   if(message->opcode != DNS_OPCODE_QUERY)
      return;

   //LLMNR messages received with non-zero response codes must be silently
   //ignored
   if(message->rcode != DNS_RCODE_NO_ERROR)
      return;

   //LLMNR responders must silently discard LLMNR queries with QDCOUNT not
   //equal to one (refer to RFC 4795, section 2.1.1)
   if(ntohs(message->qdcount) != 1)
      return;

   //LLMNR responders must silently discard LLMNR queries with ANCOUNT or
   //NSCOUNT not equal to zero
   if(ntohs(message->ancount) != 0 || ntohs(message->nscount) != 0)
      return;

   //Point to the first question
   pos = sizeof(LlmnrHeader);

   //Parse resource record name
   n = dnsParseName((DnsHeader *) message, length, pos, NULL, 0);
   //Invalid name?
   if(n == 0)
      return;

   //Malformed LLMNR message?
   if((n + sizeof(DnsQuestion)) > length)
      return;

   //Point to the corresponding entry
   question = DNS_GET_QUESTION(message, n);

   //Check the class of the query
   if(ntohs(question->qclass) == DNS_RR_CLASS_IN ||
      ntohs(question->qclass) == DNS_RR_CLASS_ANY)
   {
      //Responders must respond to LLMNR queries for names and addresses for
      //which they are authoritative
      if(!dnsCompareName((DnsHeader *) message, length, pos,
         interface->hostname, 0))
      {
         //Responders must direct responses to the port from which the query
         //was sent
         destPort = ntohs(udpHeader->srcPort);

         //Send LLMNR response
         llmnrSendResponse(interface, &destIpAddr, destPort, message->id,
            ntohs(question->qtype), ntohs(question->qclass));
      }
   }
}


/**
 * @brief Send LLMNR response message
 * @param[in] interface Underlying network interface
 * @param[in] destIpAddr Destination IP address
 * @param[in] destPort destination port
 * @param[in] id 16-bit identifier to be used when sending LLMNR query
 * @param[in] qtype Resource record type
 * @param[in] qclass Resource record class
 **/

error_t llmnrSendResponse(NetInterface *interface, const IpAddr *destIpAddr,
   uint16_t destPort, uint16_t id, uint16_t qtype, uint16_t qclass)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   LlmnrHeader *message;
   DnsQuestion *question;
   DnsResourceRecord *record;
   NetTxAncillary ancillary;

   //Initialize status code
   error = NO_ERROR;

   //Allocate a memory buffer to hold the LLMNR response message
   buffer = udpAllocBuffer(DNS_MESSAGE_MAX_SIZE, &offset);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the LLMNR header
   message = netBufferAt(buffer, offset);

   //Take the identifier from the query message
   message->id = id;

   //Format LLMNR response header
   message->qr = 1;
   message->opcode = DNS_OPCODE_QUERY;
   message->c = 0;
   message->tc = 0;
   message->t = 0;
   message->z = 0;
   message->rcode = DNS_RCODE_NO_ERROR;
   message->qdcount = HTONS(1);
   message->ancount = 0;
   message->nscount = 0;
   message->arcount = 0;

   //Set the length of the LLMNR response message
   length = sizeof(DnsHeader);

   //Encode the requested host name using the DNS name notation
   length += dnsEncodeName(interface->hostname,
      (uint8_t *) message + length);

   //Point to the corresponding entry
   question = DNS_GET_QUESTION(message, length);

   //Fill in resource record
   question->qtype = htons(qtype);
   question->qclass = htons(qclass);

   //Update the length of the response message
   length += sizeof(DnsQuestion);

#if (IPV4_SUPPORT == ENABLED)
   //A resource record requested?
   if(qtype == DNS_RR_TYPE_A || qtype == DNS_RR_TYPE_ANY)
   {
      //Valid IPv4 host address?
      if(interface->ipv4Context.addrList[0].state == IPV4_ADDR_STATE_VALID)
      {
         //Encode the host name using the DNS name notation
         length += dnsEncodeName(interface->hostname,
            (uint8_t *) message + length);

         //Point to the corresponding resource record
         record = DNS_GET_RESOURCE_RECORD(message, length);

         //Fill in resource record
         record->rtype = HTONS(DNS_RR_TYPE_A);
         record->rclass = HTONS(DNS_RR_CLASS_IN);
         record->ttl = HTONL(LLMNR_DEFAULT_RESOURCE_RECORD_TTL);
         record->rdlength = HTONS(sizeof(Ipv4Addr));

         //Copy IPv4 address
         ipv4CopyAddr(record->rdata, &interface->ipv4Context.addrList[0].addr);

         //Number of resource records in the answer section
         message->ancount++;

         //Update the length of the response message
         length += sizeof(DnsResourceRecord) + sizeof(Ipv4Addr);
      }
   }
#endif

#if (IPV6_SUPPORT == ENABLED)
   //AAAA resource record requested?
   if(qtype == DNS_RR_TYPE_AAAA || qtype == DNS_RR_TYPE_ANY)
   {
      //Valid IPv6 link-local address?
      if(ipv6GetLinkLocalAddrState(interface) == IPV6_ADDR_STATE_PREFERRED)
      {
         //Encode the host name using the DNS name notation
         length += dnsEncodeName(interface->hostname,
            (uint8_t *) message + length);

         //Point to the corresponding resource record
         record = DNS_GET_RESOURCE_RECORD(message, length);

         //Fill in resource record
         record->rtype = HTONS(DNS_RR_TYPE_AAAA);
         record->rclass = HTONS(DNS_RR_CLASS_IN);
         record->ttl = HTONL(LLMNR_DEFAULT_RESOURCE_RECORD_TTL);
         record->rdlength = HTONS(sizeof(Ipv6Addr));

         //Copy IPv6 address
         ipv6CopyAddr(record->rdata, &interface->ipv6Context.addrList[0].addr);

         //Number of resource records in the answer section
         message->ancount++;

         //Update the length of the response message
         length += sizeof(DnsResourceRecord) + sizeof(Ipv6Addr);
      }
   }
#endif

   //Valid LLMNR response?
   if(message->ancount > 0)
   {
      //The ANCOUNT field specifies the number of resource records in the
      //answer section
      message->ancount = htons(message->ancount);

      //Adjust the length of the multi-part buffer
      netBufferSetLength(buffer, offset + length);

      //Debug message
      TRACE_INFO("Sending LLMNR message (%" PRIuSIZE " bytes)...\r\n", length);
      //Dump message
      dnsDumpMessage((DnsHeader *) message, length);

      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_TX_ANCILLARY;

      //For UDP responses, the Hop Limit field in the IPv6 header and the TTL
      //field in the IPV4 header MAY be set to any value. However, it is
      //recommended that the value 255 be used for compatibility with early
      //implementations (refer to RFC 4795, section 2.5)
      ancillary.ttl = LLMNR_DEFAULT_IP_TTL;

      //This flag tells the stack that the destination is on a locally attached
      //network and not to perform a lookup of the routing table
      ancillary.dontRoute = TRUE;

      //Send LLMNR response
      error = udpSendBuffer(interface, NULL, LLMNR_PORT, destIpAddr, destPort,
         buffer, offset, &ancillary);
   }

   //Free previously allocated memory
   netBufferFree(buffer);

   //Return status code
   return error;
}

#endif
