/**
 * @file mdns_common.c
 * @brief Definitions common to mDNS client and mDNS responder
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
 * Multicast DNS and its companion technology DNS-Based Service Discovery
 * were created to provide ease-of-use and autoconfiguration to IP networks.
 * Refer to the following RFCs for complete details:
 * - RFC 6762: Multicast DNS
 * - RFC 6763: DNS-Based Service Discovery
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL MDNS_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "core/net.h"
#include "ipv4/ipv4_misc.h"
#include "ipv6/ipv6_misc.h"
#include "mdns/mdns_client.h"
#include "mdns/mdns_responder.h"
#include "mdns/mdns_common.h"
#include "dns/dns_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MDNS_CLIENT_SUPPORT == ENABLED || MDNS_RESPONDER_SUPPORT == ENABLED)

//mDNS IPv6 multicast group (ff02::fb)
const Ipv6Addr MDNS_IPV6_MULTICAST_ADDR =
   IPV6_ADDR(0xFF02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00FB);


/**
 * @brief mDNS related initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mdnsInit(NetInterface *interface)
{
   error_t error;

#if (IPV4_SUPPORT == ENABLED)
   //Join the mDNS IPv4 multicast group
   error = ipv4JoinMulticastGroup(interface, MDNS_IPV4_MULTICAST_ADDR);
   //Any error to report?
   if(error)
      return error;
#endif

#if (IPV6_SUPPORT == ENABLED)
   //Join the mDNS IPv6 multicast group
   error = ipv6JoinMulticastGroup(interface, &MDNS_IPV6_MULTICAST_ADDR);
   //Any error to report?
   if(error)
      return error;
#endif

   //Callback function to be called when a mDNS message is received
   error = udpAttachRxCallback(interface, MDNS_PORT, mdnsProcessMessage, NULL);
   //Any error to report?
   if(error)
      return error;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Process incoming mDNS message
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @param[in] udpHeader UDP header
 * @param[in] buffer Multi-part buffer containing the incoming mDNS message
 * @param[in] offset Offset to the first byte of the mDNS message
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @param[in] param Callback function parameter (not used)
 **/

void mdnsProcessMessage(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *udpHeader,
   const NetBuffer *buffer, size_t offset, const NetRxAncillary *ancillary,
   void *param)
{
   size_t length;
   DnsHeader *dnsHeader;
   MdnsMessage message;

   //Retrieve the length of the mDNS message
   length = netBufferGetLength(buffer) - offset;

   //Ensure the mDNS message is valid
   if(length < sizeof(DnsHeader))
      return;
   if(length > MDNS_MESSAGE_MAX_SIZE)
      return;

   //Point to the mDNS message header
   dnsHeader = netBufferAt(buffer, offset);
   //Sanity check
   if(dnsHeader == NULL)
      return;

   //Debug message
   TRACE_INFO("mDNS message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message
   dnsDumpMessage(dnsHeader, length);

   //mDNS messages received with an opcode other than zero must be silently
   //ignored
   if(dnsHeader->opcode != DNS_OPCODE_QUERY)
      return;

   //mDNS messages received with non-zero response codes must be silently
   //ignored
   if(dnsHeader->rcode != DNS_RCODE_NO_ERROR)
      return;

   //Save mDNS message
   message.buffer = (NetBuffer *) buffer;
   message.offset = offset;
   message.length = length;
   message.pseudoHeader = pseudoHeader;
   message.udpHeader = udpHeader;
   message.dnsHeader = dnsHeader;

   //mDNS query received?
   if(!dnsHeader->qr)
   {
#if (MDNS_RESPONDER_SUPPORT == ENABLED)
      //Process incoming mDNS query message
      mdnsResponderProcessQuery(interface, &message);
#endif
   }
   //mDNS response received?
   else
   {
      //Process incoming mDNS response message
      mdnsProcessResponse(interface, &message);
   }
}


/**
 * @brief Process mDNS response message
 * @param[in] interface Underlying network interface
 * @param[in] response Incoming mDNS response message
 **/

void mdnsProcessResponse(NetInterface *interface, MdnsMessage *response)
{
   uint_t i;
   uint_t k;
   size_t n;
   size_t offset;
   DnsResourceRecord *record;

   //Source address check (refer to RFC 6762 section 11)
   if(!mdnsCheckSourceAddr(interface, response->pseudoHeader))
      return;

   //mDNS implementations must silently ignore any mDNS responses they
   //receive where the source UDP port is not 5353
   if(ntohs(response->udpHeader->srcPort) != MDNS_PORT)
      return;

   //Point to the question section
   offset = sizeof(DnsHeader);

   //Any questions in the question section of a received mDNS response
   //must be silently ignored
   for(i = 0; i < ntohs(response->dnsHeader->qdcount); i++)
   {
      //Parse domain name
      offset = dnsParseName(response->dnsHeader, response->length, offset, NULL, 0);
      //Invalid name?
      if(!offset)
         break;

      //Point to the next question
      offset += sizeof(DnsQuestion);
      //Make sure the mDNS message is valid
      if(offset > response->length)
         break;
   }

   //Malformed mDNS message?
   if(i != ntohs(response->dnsHeader->qdcount))
      return;

   //Compute the total number of resource records
   k = ntohs(response->dnsHeader->ancount) +
      ntohs(response->dnsHeader->nscount) +
      ntohs(response->dnsHeader->arcount);

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

#if (MDNS_CLIENT_SUPPORT == ENABLED)
      //Parse the resource record
      mdnsClientParseAnRecord(interface, response, offset, record);
#endif

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
      //Parse the resource record
      mdnsResponderParseAnRecord(interface, response, offset, record);
#endif

#if (DNS_SD_SUPPORT == ENABLED)
      //Parse the resource record
      dnsSdParseAnRecord(interface, response, offset, record);
#endif

      //Point to the next resource record
      offset = n + ntohs(record->rdlength);
   }
}


/**
 * @brief Source address check
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @return TRUE if the source address is valid, else FALSE
 **/

bool_t mdnsCheckSourceAddr(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader)
{
   bool_t valid;

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 packet received?
   if(pseudoHeader->length == sizeof(Ipv4PseudoHeader))
   {
      //Perform source address check (refer to RFC 6762 section 11)
      if(pseudoHeader->ipv4Data.destAddr == MDNS_IPV4_MULTICAST_ADDR)
      {
         //All responses received with the destination address 224.0.0.251
         //are necessarily deemed to have originated on the local link,
         //regardless of source IP address
         valid = TRUE;
      }
      else if(ipv4IsLinkLocalAddr(pseudoHeader->ipv4Data.srcAddr) ||
         ipv4IsLinkLocalAddr(pseudoHeader->ipv4Data.destAddr))
      {
         //Packets with a link-local source or destination address
         //originate from the local link
         valid = TRUE;
      }
      else if(ipv4IsOnLink(interface, pseudoHeader->ipv4Data.srcAddr))
      {
         //The source IP address is on the local link
         valid = TRUE;
      }
      else
      {
         //Only accept responses that originate from the local link, and
         //silently discard any other response packets
         valid = FALSE;
      }
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 packet received?
   if(pseudoHeader->length == sizeof(Ipv6PseudoHeader))
   {
      //Perform source address check (refer to RFC 6762 section 11)
      if(ipv6CompAddr(&pseudoHeader->ipv6Data.destAddr, &MDNS_IPV6_MULTICAST_ADDR))
      {
         //All responses received with the destination address ff02::fb
         //are necessarily deemed to have originated on the local link,
         //regardless of source IP address
         valid = TRUE;
      }
      else if(ipv6IsOnLink(interface, &pseudoHeader->ipv6Data.srcAddr))
      {
         //The source IP address is on the local link
         valid = TRUE;
      }
      else
      {
         //Only accept responses that originate from the local link, and
         //silently discard any other response packets
         valid = FALSE;
      }
   }
   else
#endif
   //Invalid packet received?
   {
      //Discard the response packet
      valid = FALSE;
   }

   //Return flag value
   return valid;
}


/**
 * @brief Create an empty mDNS message
 * @param[in,out] message Newly created mDNS message
 * @param[in] queryResponse This flag specifies whether the message is a query or a response
 * @return Error code
 **/

error_t mdnsCreateMessage(MdnsMessage *message, bool_t queryResponse)
{
   error_t error;

   //Allocate a memory buffer to hold the mDNS message
   message->buffer = udpAllocBuffer(MDNS_MESSAGE_MAX_SIZE, &message->offset);

   //Successful memory allocation?
   if(message->buffer != NULL)
   {
      //Point to the mDNS message header
      message->dnsHeader = netBufferAt(message->buffer, message->offset);

      //Sanity check
      if(message->dnsHeader != NULL)
      {
         //Format mDNS message header
         message->dnsHeader->id = 0;
         message->dnsHeader->opcode = DNS_OPCODE_QUERY;
         message->dnsHeader->tc = 0;
         message->dnsHeader->rd = 0;
         message->dnsHeader->ra = 0;
         message->dnsHeader->z = 0;
         message->dnsHeader->rcode = DNS_RCODE_NO_ERROR;
         message->dnsHeader->qdcount = 0;
         message->dnsHeader->ancount = 0;
         message->dnsHeader->nscount = 0;
         message->dnsHeader->arcount = 0;

         //Query or response mDNS message?
         if(!queryResponse)
         {
            //In query messages, QR and AA bits must be set to zero
            message->dnsHeader->qr = 0;
            message->dnsHeader->aa = 0;
         }
         else
         {
            //In response messages, QR and AA bits must be set to one
            message->dnsHeader->qr = 1;
            message->dnsHeader->aa = 1;
         }

         //Number of shared resource records
         message->sharedRecordCount = 0;
         //Length of the mDNS message
         message->length = sizeof(DnsHeader);

         //Successful processing
         error = NO_ERROR;
      }
      else
      {
         //Clean up side effects
         mdnsDeleteMessage(message);

         //Report an error
         error = ERROR_FAILURE;
      }
   }
   else
   {
      //Failed to allocate memory
      error = ERROR_OUT_OF_RESOURCES;
   }

   //Return status code
   return error;
}


/**
 * @brief release a mDNS message
 * @param[in] message mDNS message to be released
 **/

void mdnsDeleteMessage(MdnsMessage *message)
{
   //Valid mDNS message?
   if(message->buffer != NULL)
   {
      //Free previously allocated memory
      netBufferFree(message->buffer);

      //The mDNS message is no more valid
      message->buffer = NULL;
      message->length = 0;
   }
}


/**
 * @brief Send mDNS message
 * @param[in] interface Underlying network interface
 * @param[in] message mDNS message to be sent
 * @param[in] destIpAddr Destination IP address (optional parameter)
 * @param[in] destPort Destination port
 * @return Error code
 **/

error_t mdnsSendMessage(NetInterface *interface, const MdnsMessage *message,
   const IpAddr *destIpAddr, uint_t destPort)
{
   error_t error;
   IpAddr ipAddr;
   NetTxAncillary ancillary;

   //Make sure the mDNS message is valid
   if(message->buffer == NULL)
      return ERROR_FAILURE;

   //Convert 16-bit values to network byte order
   message->dnsHeader->qdcount = htons(message->dnsHeader->qdcount);
   message->dnsHeader->nscount = htons(message->dnsHeader->nscount);
   message->dnsHeader->ancount = htons(message->dnsHeader->ancount);
   message->dnsHeader->arcount = htons(message->dnsHeader->arcount);

   //Start of exception handling block
   do
   {
      //Adjust the length of the multi-part buffer
      error = netBufferSetLength(message->buffer, message->offset + message->length);
      //Any error to report?
      if(error)
         break;

      //Debug message
      TRACE_INFO("Sending mDNS message (%" PRIuSIZE " bytes)...\r\n", message->length);
      //Dump message
      dnsDumpMessage(message->dnsHeader, message->length);

      //Check whether the message should be sent to a specific IP address
      if(destIpAddr != NULL)
      {
         //Additional options can be passed to the stack along with the packet
         ancillary = NET_DEFAULT_TX_ANCILLARY;

         //All multicast DNS responses should be sent with an IP TTL set to 255
         //(refer to RFC 6762, section 11)
         ancillary.ttl = MDNS_DEFAULT_IP_TTL;

         //Send mDNS message
         error = udpSendBuffer(interface, NULL, MDNS_PORT, destIpAddr, destPort,
            message->buffer, message->offset, &ancillary);
         //Any error to report?
         if(error)
            break;
      }
      else
      {
#if (IPV4_SUPPORT == ENABLED)
         //Select the relevant multicast address (224.0.0.251)
         ipAddr.length = sizeof(Ipv4Addr);
         ipAddr.ipv4Addr = MDNS_IPV4_MULTICAST_ADDR;

         //Additional options can be passed to the stack along with the packet
         ancillary = NET_DEFAULT_TX_ANCILLARY;
         //Set the TTL value to be used
         ancillary.ttl = MDNS_DEFAULT_IP_TTL;

         //Send mDNS message
         error = udpSendBuffer(interface, NULL, MDNS_PORT, &ipAddr, MDNS_PORT,
            message->buffer, message->offset, &ancillary);
         //Any error to report?
         if(error)
            break;
#endif

#if (IPV6_SUPPORT == ENABLED)
         //Select the relevant multicast address (ff02::fb)
         ipAddr.length = sizeof(Ipv6Addr);
         ipAddr.ipv6Addr = MDNS_IPV6_MULTICAST_ADDR;

         //Additional options can be passed to the stack along with the packet
         ancillary = NET_DEFAULT_TX_ANCILLARY;
         //Set the TTL value to be used
         ancillary.ttl = MDNS_DEFAULT_IP_TTL;

         //Send mDNS message
         error = udpSendBuffer(interface, NULL, MDNS_PORT, &ipAddr, MDNS_PORT,
            message->buffer, message->offset, &ancillary);
         //Any error to report?
         if(error)
            break;
#endif
      }

      //End of exception handling block
   } while(0);

   //Return status code
   return error;
}


/**
 * @brief Encode instance, service and domain names using the DNS name notation
 * @param[in] instance Instance name
 * @param[in] service Service name
 * @param[in] domain Domain name
 * @param[out] dest Pointer to the encoded name (optional parameter)
 * @return Length of the encoded domain name
 **/

size_t mdnsEncodeName(const char_t *instance, const char_t *service,
   const char_t *domain, uint8_t *dest)
{
   size_t n;
   size_t length;

   //Total length of the encoded name
   length = 0;

   //Any instance name?
   if(*instance != '\0')
   {
      //Encode instance name
      n = dnsEncodeName(instance, dest);

      //Failed to encode instance name?
      if(!n)
         return 0;

      //Update the length of the encoded name
      length += n;
   }

   //Any service name?
   if(*service != '\0')
   {
      //If an instance name precedes the service name, then
      //remove the null label
      if(length > 0)
         length--;

      //Encode service name
      if(dest != NULL)
         n = dnsEncodeName(service, dest + length);
      else
         n = dnsEncodeName(service, NULL);

      //Failed to encode instance name?
      if(!n)
         return 0;

      //Update the length of the encoded name
      length += n;
   }

   //Skip the separator that may precede the domain name
   if(*domain == '.')
      domain++;

   //Any domain name to encode?
   if(*domain != '\0')
   {
      //If an instance or a service name precedes the domain name, then
      //remove the null label
      if(length > 0)
         length--;

      //Encode domain name
      if(dest != NULL)
         n = dnsEncodeName(domain, dest + length);
      else
         n = dnsEncodeName(domain, NULL);

      //Failed to encode instance name?
      if(!n)
         return 0;

      //Update the length of the encoded name
      length += n;
   }

   //Return the length of the encoded string
   return length;
}


/**
 * @brief Compare instance, service and domain names
 * @param[in] message Pointer to the DNS message
 * @param[in] length Length of the DNS message
 * @param[in] pos Offset of the encoded name
 * @param[in] instance Instance name
 * @param[in] service Service name
 * @param[in] domain Domain name
 * @param[in] level Current level of recursion
 * @return The function returns 0 if the domain names match, -1 if the first
 *   domain name lexicographically precedes the second name, or 1 if the
 *   second domain name lexicographically precedes the first name
 **/

int_t mdnsCompareName(const DnsHeader *message, size_t length, size_t pos,
   const char_t *instance, const char_t *service, const char_t *domain, uint_t level)
{
   int_t res;
   size_t n;
   size_t pointer;
   uint8_t *p;

   //Check parameters
   if(instance == NULL || service == NULL || domain == NULL)
      return -2;

   //Recursion limit exceeded?
   if(level >= DNS_NAME_MAX_RECURSION)
      return -2;

   //Cast the DNS message to byte array
   p = (uint8_t *) message;

   //Skip the separator that may precede the domain name
   if(*domain == '.')
      domain++;

   //Parse encoded domain name
   while(pos < length)
   {
      //Retrieve the length of the current label
      n = p[pos];

      //End marker found?
      if(n == 0)
      {
         //The domain name which still has remaining data is deemed
         //lexicographically later
         if(*instance != '\0' || *service != '\0' || *domain != '\0')
            return -1;

         //The domain names match each other
         return 0;
      }
      //Compression tag found?
      if(n >= DNS_COMPRESSION_TAG)
      {
         //Malformed DNS message?
         if((pos + 1) >= length)
            return -2;

         //Read the most significant byte of the pointer
         pointer = (p[pos] & ~DNS_COMPRESSION_TAG) << 8;
         //Read the least significant byte of the pointer
         pointer |= p[pos + 1];

         //Compare the remaining part
         res = mdnsCompareName(message, length, pointer,
            instance, service, domain, level + 1);

         //Return comparison result
         return res;
      }
      else
      {
         //Advance data pointer
         pos++;

         //Malformed DNS message?
         if((pos + n) > length)
            return -2;

         //Compare current label
         if(*instance != '\0')
         {
            //Compare instance name
            res = osStrncasecmp((char_t *) p + pos, instance, n);
            //Any mismatch?
            if(res)
               return res;

            //Advance data pointer
            instance += n;

            //The instance name which still has remaining data is deemed
            //lexicographically later
            if(*instance != '\0' && *instance != '.')
               return -1;

            //Skip the separator character, if any
            if(*instance == '.')
               instance++;
         }
         else if(*service != '\0')
         {
            //Compare service name
            res = osStrncasecmp((char_t *) p + pos, service, n);
            //Any mismatch?
            if(res)
               return res;

            //Advance data pointer
            service += n;

            //The service name which still has remaining data is deemed
            //lexicographically later
            if(*service != '\0' && *service != '.')
               return -1;

            //Any separator in service name?
            if(*service == '.')
               service++;
         }
         else
         {
            //Compare domain name
            res = osStrncasecmp((char_t *) p + pos, domain, n);
            //Any mismatch?
            if(res)
               return res;

            //Advance data pointer
            domain += n;

            //The domain name which still has remaining data is deemed
            //lexicographically later
            if(*domain != '\0' && *domain != '.')
               return -1;

            //Any separator in domain name?
            if(*domain == '.')
               domain++;
         }

         //Advance data pointer
         pos += n;
      }
   }

   //Malformed DNS message
   return -2;
}


/**
 * @brief Compare resource records
 * @param[in] message1 Pointer to the first mDNS message
 * @param[in] offset1 Offset of the first but of the resource record
 * @param[in] record1 Pointer the first resource record
 * @param[in] message2 Pointer to the second mDNS message
 * @param[in] offset2 Offset of the first but of the resource record
 * @param[in] record2 Pointer the second resource record
 * @return The function returns 0 if the resource records match, -1 if the first
 *   resource record lexicographically precedes the second one, or 1 if the
 *   second resource record lexicographically precedes the first one
 **/

int_t mdnsCompareRecord(const MdnsMessage *message1, size_t offset1,
   const DnsResourceRecord *record1, const MdnsMessage *message2,
   size_t offset2, const DnsResourceRecord *record2)
{
   int_t res;
   size_t n1;
   size_t n2;
   uint16_t value1;
   uint16_t value2;

   //Convert the record class to host byte order
   value1 = ntohs(record1->rclass);
   value2 = ntohs(record2->rclass);

   //Discard cache-flush bit
   value1 &= ~MDNS_RCLASS_CACHE_FLUSH;
   value2 &= ~MDNS_RCLASS_CACHE_FLUSH;

   //The determination of lexicographically later record is performed by
   //first comparing the record class (excluding the cache-flush bit)
   if(value1 < value2)
      return -1;
   else if(value1 > value2)
      return 1;

   //Convert the record type to host byte order
   value1 = ntohs(record1->rtype);
   value2 = ntohs(record2->rtype);

   //Then compare the record type
   if(value1 < value2)
      return -1;
   else if(value1 > value2)
      return 1;

   //If the rrtype and rrclass both match, then the rdata is compared
   if(value1 == DNS_RR_TYPE_NS || value1 == DNS_RR_TYPE_SOA ||
      value1 == DNS_RR_TYPE_CNAME || value1 == DNS_RR_TYPE_PTR)
   {
      //Compute the offset of the first byte of the rdata
      n1 = record1->rdata - (uint8_t *) message1->dnsHeader;
      n2 = record2->rdata - (uint8_t *) message2->dnsHeader;

      //The names must be uncompressed before comparison
      res = dnsCompareEncodedName(message1->dnsHeader, message1->length,
         n1, message2->dnsHeader, message2->length, n2, 0);
   }
   else
   {
      //Retrieve the length of the rdata fields
      n1 = htons(record1->rdlength);
      n2 = htons(record2->rdlength);

      //The bytes of the raw uncompressed rdata are compared in turn, interpreting
      //the bytes as eight-bit unsigned values, until a byte is found whose value
      //is greater than that of its counterpart (in which case, the rdata whose
      //byte has the greater value is deemed lexicographically later) or one of the
      //resource records runs out of rdata (in which case, the resource record which
      //still has remaining data first is deemed lexicographically later)
      if(n1 < n2)
      {
         //Raw comparison of the binary content of the rdata
         res = osMemcmp(record1->rdata, record2->rdata, n1);

         //Check comparison result
         if(!res)
         {
            //The first resource records runs out of rdata
            res = -1;
         }
      }
      else if(n1 > n2)
      {
         //Raw comparison of the binary content of the rdata
         res = osMemcmp(record1->rdata, record2->rdata, n2);

         //Check comparison result
         if(!res)
         {
            //The second resource records runs out of rdata
            res = 1;
         }
      }
      else
      {
         //Raw comparison of the binary content of the rdata
         res = osMemcmp(record1->rdata, record2->rdata, n1);
      }
   }

   //Return comparison result
   return res;
}


/**
 * @brief Check for duplicate resource records
 * @param[in] message Pointer to the mDNS message
 * @param[in] instance Instance name
 * @param[in] service Service name
 * @param[in] domain Domain name
 * @param[in] rtype Resource record type
 * @return The function returns TRUE is the specified resource record is a
 *   duplicate. Otherwise FALSE is returned
 **/

bool_t mdnsCheckDuplicateRecord(const MdnsMessage *message, const char_t *instance,
   const char_t *service, const char_t *domain, uint16_t rtype)
{
   uint_t i;
   uint_t k;
   size_t n;
   size_t offset;
   uint16_t rclass;
   bool_t duplicate;
   DnsResourceRecord *record;

   //Clear flag
   duplicate = FALSE;

   //Point to the first question
   offset = sizeof(DnsHeader);

   //Parse the Question Section
   for(i = 0; i < message->dnsHeader->qdcount; i++)
   {
      //Parse domain name
      offset = dnsParseName(message->dnsHeader, message->length, offset, NULL, 0);
      //Invalid name?
      if(!offset)
         break;

      //Point to the next question
      offset += sizeof(DnsQuestion);
      //Make sure the mDNS message is valid
      if(offset > message->length)
         break;
   }

   //Successful processing?
   if(i == message->dnsHeader->qdcount)
   {
      //Compute the total number of resource records
      k = message->dnsHeader->ancount + message->dnsHeader->nscount +
         message->dnsHeader->arcount;

      //Loop through the resource records
      for(i = 0; i < k; i++)
      {
         //Parse resource record name
         n = dnsParseName(message->dnsHeader, message->length, offset, NULL, 0);
         //Invalid name?
         if(!n)
            break;

         //Point to the associated resource record
         record = DNS_GET_RESOURCE_RECORD(message->dnsHeader, n);
         //Point to the resource data
         n += sizeof(DnsResourceRecord);

         //Make sure the resource record is valid
         if(n > message->length)
            break;
         if((n + ntohs(record->rdlength)) > message->length)
            break;

         //Convert the record class to host byte order
         rclass = ntohs(record->rclass);
         //Discard cache-flush bit
         rclass &= ~MDNS_RCLASS_CACHE_FLUSH;

         //Check the class and the type of the resource record
         if(rclass == DNS_RR_CLASS_IN && ntohs(record->rtype) == rtype)
         {
            //Compare resource record name
            if(!mdnsCompareName(message->dnsHeader, message->length,
               offset, instance, service, domain, 0))
            {
               //The resource record is already present in the Answer Section
               duplicate = TRUE;
               //We are done
               break;
            }
         }

         //Point to the next resource record
         offset = n + ntohs(record->rdlength);
      }
   }

   //The function returns TRUE is the specified resource record is a duplicate
   return duplicate;
}

#endif
