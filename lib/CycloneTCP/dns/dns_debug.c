/**
 * @file dns_debug.c
 * @brief Data logging functions for debugging purpose (DNS)
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
#define TRACE_LEVEL DNS_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "dns/dns_debug.h"
#include "netbios/nbns_client.h"
#include "netbios/nbns_responder.h"
#include "netbios/nbns_common.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (DNS_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)


/**
 * @brief Dump DNS message for debugging purpose
 * @param[in] message Pointer to the DNS message
 * @param[in] length Length of the DNS message
 **/

void dnsDumpMessage(const DnsHeader *message, size_t length)
{
   uint_t i;
   size_t pos;
   char_t *buffer;

   //Make sure the DNS message is valid
   if(length >= sizeof(DnsHeader))
   {
      //Dump DNS message header
      TRACE_DEBUG("  Identifier (ID) = %" PRIu16 "\r\n", ntohs(message->id));
      TRACE_DEBUG("  Query Response (QR) = %" PRIu8 "\r\n", message->qr);
      TRACE_DEBUG("  Opcode (OPCODE) = %" PRIu8 "\r\n", message->opcode);
      TRACE_DEBUG("  Authoritative Answer (AA) = %" PRIu8 "\r\n", message->aa);
      TRACE_DEBUG("  TrunCation (TC) = %" PRIu8 "\r\n", message->tc);
      TRACE_DEBUG("  Recursion Desired (RD) = %" PRIu8 "\r\n", message->rd);
      TRACE_DEBUG("  Recursion Available (RA) = %" PRIu8 "\r\n", message->ra);
      TRACE_DEBUG("  Reserved (Z) = %" PRIu8 "\r\n", message->z);
      TRACE_DEBUG("  Response Code (RCODE) = %" PRIu8 "\r\n", message->rcode);
      TRACE_DEBUG("  Question Count (QDCOUNT) = %" PRIu8 "\r\n", ntohs(message->qdcount));
      TRACE_DEBUG("  Answer Count (ANCOUNT) = %" PRIu8 "\r\n", ntohs(message->ancount));
      TRACE_DEBUG("  Name Server Count (NSCOUNT) = %" PRIu8 "\r\n", ntohs(message->nscount));
      TRACE_DEBUG("  Additional Record Count (ARCOUNT) = %" PRIu8 "\r\n", ntohs(message->arcount));

      //Allocate a memory buffer to holds domain names
      buffer = memPoolAlloc(DNS_NAME_MAX_SIZE);
      //Failed to allocate memory
      if(buffer == NULL)
         return;

      //Point to the first question
      pos = sizeof(DnsHeader);

      //Start of exception handling block
      do
      {
         //Debug message
         TRACE_DEBUG("  Questions\r\n");

         //Parse questions
         for(i = 0; i < ntohs(message->qdcount); i++)
         {
            //Dump current question
            pos = dnsDumpQuestion(message, length, pos, buffer);
            //Any error to report?
            if(!pos)
               break;
         }

         //Parsing error?
         if(!pos)
            break;

         //Debug message
         TRACE_DEBUG("  Answer RRs\r\n");

         //Parse answer resource records
         for(i = 0; i < ntohs(message->ancount); i++)
         {
            //Dump current resource record
            pos = dnsDumpResourceRecord(message, length, pos, buffer);
            //Any error to report?
            if(!pos)
               break;
         }

         //Parsing error?
         if(!pos)
            break;

         //Debug message
         TRACE_DEBUG("  Authority RRs\r\n");

         //Parse authority resource records
         for(i = 0; i < ntohs(message->nscount); i++)
         {
            //Dump current resource record
            pos = dnsDumpResourceRecord(message, length, pos, buffer);
            //Any error to report?
            if(!pos)
               break;
         }

         //Parsing error?
         if(!pos)
            break;

         //Debug message
         TRACE_DEBUG("  Additional RRs\r\n");

         //Parse additional resource records
         for(i = 0; i < ntohs(message->arcount); i++)
         {
            //Dump current resource record
            pos = dnsDumpResourceRecord(message, length, pos, buffer);
            //Any error to report?
            if(!pos)
               break;
         }

         //End of exception handling block
      } while(0);

      //Free previously allocated memory
      memPoolFree(buffer);
   }
}


/**
 * @brief Dump DNS question for debugging purpose
 * @param[in] message Pointer to the DNS message
 * @param[in] length Length of the DNS message
 * @param[in] pos Offset of the question to decode
 * @param[in] buffer Memory buffer to holds domain names
 * @return Offset to the next question
 **/

size_t dnsDumpQuestion(const DnsHeader *message, size_t length, size_t pos, char_t *buffer)
{
   size_t n;
   DnsQuestion *question;

   //Parse domain name
   n = dnsParseName(message, length, pos, buffer, 0);
   //Invalid name?
   if(!n)
      return 0;

   //Make sure the DNS question is valid
   if((n + sizeof(DnsQuestion)) > length)
      return 0;

   //Point to the corresponding entry
   question = DNS_GET_QUESTION(message, n);

   //NB question found?
   if(ntohs(question->qtype) == DNS_RR_TYPE_NB)
   {
#if (NBNS_CLIENT_SUPPORT == ENABLED || NBNS_RESPONDER_SUPPORT == ENABLED)
#if (IPV4_SUPPORT == ENABLED)
      //Decode NetBIOS name
      pos = nbnsParseName((NbnsHeader *) message, length, pos, buffer);
      //Invalid NetBIOS name?
      if(!pos)
         return 0;
#endif
#endif
   }

   //Dump DNS question
   TRACE_DEBUG("    Name (QNAME) = %s\r\n", buffer);
   TRACE_DEBUG("      Query Type (QTYPE) = %" PRIu16 "\r\n", ntohs(question->qtype));
   TRACE_DEBUG("      Query Class (QCLASS) = %" PRIu16 "\r\n", ntohs(question->qclass));

   //Point to the next question
   n += sizeof(DnsQuestion);
   //Return the current position
   return n;
}


/**
 * @brief Dump DNS resource record for debugging purpose
 * @param[in] message Pointer to the DNS message
 * @param[in] length Length of the DNS message
 * @param[in] pos Offset of the question to decode
 * @param[in] buffer Memory buffer to holds domain names
 * @return Offset to the next question
 **/

size_t dnsDumpResourceRecord(const DnsHeader *message, size_t length, size_t pos, char_t *buffer)
{
   size_t n;
   DnsResourceRecord *record;
   DnsSrvResourceRecord *srvRecord;

   //Parse domain name
   n = dnsParseName(message, length, pos, buffer, 0);
   //Invalid name?
   if(!n)
      return 0;

   //Point to the corresponding entry
   record = DNS_GET_RESOURCE_RECORD(message, n);

   //Make sure the resource record is valid
   if((n + sizeof(DnsResourceRecord)) > length)
      return 0;
   if((n + sizeof(DnsResourceRecord) + ntohs(record->rdlength)) > length)
      return 0;

   //NB resource record found?
   if(ntohs(record->rtype) == DNS_RR_TYPE_NB)
   {
#if (NBNS_CLIENT_SUPPORT == ENABLED || NBNS_RESPONDER_SUPPORT == ENABLED)
#if (IPV4_SUPPORT == ENABLED)
      //Decode NetBIOS name
      pos = nbnsParseName((NbnsHeader *) message, length, pos, buffer);
      //Invalid NetBIOS name?
      if(!pos)
         return 0;
#endif
#endif
   }

   //Dump DNS resource record
   TRACE_DEBUG("    Name (NAME) = %s\r\n", buffer);
   TRACE_DEBUG("      Query Type (TYPE) = %" PRIu16 "\r\n", ntohs(record->rtype));
   TRACE_DEBUG("      Query Class (CLASS) = %" PRIu16 "\r\n", ntohs(record->rclass));
   TRACE_DEBUG("      Time-To-Live (TTL) = %" PRIu32 "\r\n", ntohl(record->ttl));
   TRACE_DEBUG("      Data Length (RDLENGTH) = %" PRIu16 "\r\n", ntohs(record->rdlength));

   //Dump resource data
#if (IPV4_SUPPORT == ENABLED)
   if(ntohs(record->rtype) == DNS_RR_TYPE_A &&
      ntohs(record->rdlength) == sizeof(Ipv4Addr))
   {
      Ipv4Addr ipAddr;

      //Copy IPv4 address
      ipv4CopyAddr(&ipAddr, record->rdata);
      //Dump IPv4 address
      TRACE_DEBUG("      Data (RDATA) = %s\r\n", ipv4AddrToString(ipAddr, NULL));
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   if(ntohs(record->rtype) == DNS_RR_TYPE_AAAA &&
      ntohs(record->rdlength) == sizeof(Ipv6Addr))
   {
      Ipv6Addr ipAddr;

      //Copy IPv6 address
      ipv6CopyAddr(&ipAddr, record->rdata);
      //Dump IPv6 address
      TRACE_DEBUG("      Data (RDATA) = %s\r\n", ipv6AddrToString(&ipAddr, NULL));
   }
   else
#endif
   if(ntohs(record->rtype) == DNS_RR_TYPE_PTR)
   {
      //Decode domain name
      pos = dnsParseName(message, length, n + sizeof(DnsResourceRecord), buffer, 0);
      //Invalid domain name?
      if(!pos)
         return 0;

      //Dump name
      TRACE_DEBUG("      Domain Name (PTRDNAME) = %s\r\n", buffer);
   }
   else if(ntohs(record->rtype) == DNS_RR_TYPE_SRV)
   {
      //Cast resource record
      srvRecord = (DnsSrvResourceRecord *) record;

      //Dump SRV resource record
      TRACE_DEBUG("      Priority = %" PRIu16 "\r\n", ntohs(srvRecord->priority));
      TRACE_DEBUG("      Weight = %" PRIu16 "\r\n", ntohs(srvRecord->weight));
      TRACE_DEBUG("      Port = %" PRIu16 "\r\n", ntohs(srvRecord->port));

      //Decode target name
      pos = dnsParseName(message, length, n + sizeof(DnsSrvResourceRecord), buffer, 0);
      //Invalid domain name?
      if(!pos)
         return 0;

      //Dump name
      TRACE_DEBUG("      Target = %s\r\n", buffer);
   }
   else
   {
      //Dump resource data
      TRACE_DEBUG("      Data (RDATA)\r\n");
      TRACE_DEBUG_ARRAY("        ", record->rdata, ntohs(record->rdlength));
   }

   //Point to the next resource record
   n += sizeof(DnsResourceRecord) + ntohs(record->rdlength);
   //Return the current position
   return n;
}

#endif
