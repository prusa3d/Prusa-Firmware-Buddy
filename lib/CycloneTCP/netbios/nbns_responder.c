/**
 * @file nbns_responder.c
 * @brief NBNS responder (NetBIOS Name Service)
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
#include "netbios/nbns_responder.h"
#include "netbios/nbns_common.h"
#include "dns/dns_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (NBNS_RESPONDER_SUPPORT == ENABLED && IPV4_SUPPORT == ENABLED)


/**
 * @brief Process NBNS query message
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @param[in] udpHeader UDP header
 * @param[in] message Pointer to the NBNS query message
 * @param[in] length Length of the message
 **/

void nbnsProcessQuery(NetInterface *interface, const Ipv4PseudoHeader *pseudoHeader,
   const UdpHeader *udpHeader, const NbnsHeader *message, size_t length)
{
   size_t pos;
   DnsQuestion *question;

   //The NBNS query shall contain one question
   if(ntohs(message->qdcount) != 1)
      return;

   //Parse NetBIOS name
   pos = nbnsParseName(message, length, sizeof(DnsHeader), NULL);

   //Invalid name?
   if(!pos)
      return;
   //Malformed NBNS query message?
   if((pos + sizeof(DnsQuestion)) > length)
      return;

   //Point to the corresponding entry
   question = DNS_GET_QUESTION(message, pos);

   //Check the class and the type of the request
   if(ntohs(question->qclass) != DNS_RR_CLASS_IN)
      return;
   if(ntohs(question->qtype) != DNS_RR_TYPE_NB)
      return;

   //Compare NetBIOS names
   if(nbnsCompareName(message, length, sizeof(DnsHeader), interface->hostname))
   {
      uint16_t destPort;
      IpAddr destIpAddr;

      //A response packet is always sent to the source UDP port and
      //source IP address of the request packet
      destIpAddr.length = sizeof(Ipv4Addr);
      destIpAddr.ipv4Addr = pseudoHeader->srcAddr;

      //Convert the port number to host byte order
      destPort = ntohs(udpHeader->srcPort);

      //Send NBNS response
      nbnsSendResponse(interface, &destIpAddr, destPort, message->id);
   }
}


/**
 * @brief Send NBNS response message
 * @param[in] interface Underlying network interface
 * @param[in] destIpAddr Destination IP address
 * @param[in] destPort destination port
 * @param[in] id 16-bit identifier to be used when sending NBNS query
 **/

error_t nbnsSendResponse(NetInterface *interface,
   const IpAddr *destIpAddr, uint16_t destPort, uint16_t id)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   NbnsHeader *message;
   NbnsAddrEntry *addrEntry;
   DnsResourceRecord *record;
   NetTxAncillary ancillary;

   //Allocate a memory buffer to hold the NBNS response message
   buffer = udpAllocBuffer(DNS_MESSAGE_MAX_SIZE, &offset);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the NBNS header
   message = netBufferAt(buffer, offset);

   //Take the identifier from the query message
   message->id = id;

   //Format NBNS response header
   message->qr = 1;
   message->opcode = DNS_OPCODE_QUERY;
   message->aa = 1;
   message->tc = 0;
   message->rd = 1;
   message->ra = 1;
   message->z = 0;
   message->b = 0;
   message->rcode = DNS_RCODE_NO_ERROR;

   //The NBNS response contains 1 answer resource record
   message->qdcount = 0;
   message->ancount = HTONS(1);
   message->nscount = 0;
   message->arcount = 0;

   //NBNS response message length
   length = sizeof(DnsHeader);

   //Encode the host name using the NBNS name notation
   length += nbnsEncodeName(interface->hostname, (uint8_t *) message + length);

   //Point to the corresponding resource record
   record = DNS_GET_RESOURCE_RECORD(message, length);

   //Fill in resource record
   record->rtype = HTONS(DNS_RR_TYPE_NB);
   record->rclass = HTONS(DNS_RR_CLASS_IN);
   record->ttl = HTONL(NBNS_DEFAULT_RESOURCE_RECORD_TTL);
   record->rdlength = HTONS(sizeof(NbnsAddrEntry));

   //Point to the address entry array
   addrEntry = (NbnsAddrEntry *) record->rdata;

   //Fill in address entry
   addrEntry->flags = HTONS(NBNS_G_UNIQUE | NBNS_ONT_BNODE);
   addrEntry->addr = interface->ipv4Context.addrList[0].addr;

   //Update the length of the NBNS response message
   length += sizeof(DnsResourceRecord) + sizeof(NbnsAddrEntry);

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + length);

   //Debug message
   TRACE_INFO("Sending NBNS message (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message
   dnsDumpMessage((DnsHeader *) message, length);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //This flag tells the stack that the destination is on a locally attached
   //network and not to perform a lookup of the routing table
   ancillary.dontRoute = TRUE;

   //A response packet is always sent to the source UDP port and source IP
   //address of the request packet
   error = udpSendBuffer(interface, NULL, NBNS_PORT, destIpAddr, destPort,
      buffer, offset, &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);

   //Return status code
   return error;
}

#endif
