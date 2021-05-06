/**
 * @file nbns_common.c
 * @brief Definitions common to NBNS client and NBNS responder
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "core/net.h"
#include "netbios/nbns_client.h"
#include "netbios/nbns_responder.h"
#include "netbios/nbns_common.h"
#include "dns/dns_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (NBNS_CLIENT_SUPPORT == ENABLED || NBNS_RESPONDER_SUPPORT == ENABLED)
#if (IPV4_SUPPORT == ENABLED)


/**
 * @brief NBNS related initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t nbnsInit(NetInterface *interface)
{
   error_t error;

   //Callback function to be called when a NBNS message is received
   error = udpAttachRxCallback(interface, NBNS_PORT, nbnsProcessMessage, NULL);
   //Any error to report?
   if(error)
      return error;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Process incoming NBNS message
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @param[in] udpHeader UDP header
 * @param[in] buffer Multi-part buffer containing the incoming NBNS message
 * @param[in] offset Offset to the first byte of the NBNS message
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @param[in] param Callback function parameter (not used)
 **/

void nbnsProcessMessage(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *udpHeader,
   const NetBuffer *buffer, size_t offset, const NetRxAncillary *ancillary,
   void *param)
{
   size_t length;
   NbnsHeader *message;

   //Make sure the NBNS message was received from an IPv4 peer
   if(pseudoHeader->length != sizeof(Ipv4PseudoHeader))
      return;

   //Retrieve the length of the NBNS message
   length = netBufferGetLength(buffer) - offset;

   //Ensure the NBNS message is valid
   if(length < sizeof(NbnsHeader))
      return;
   if(length > DNS_MESSAGE_MAX_SIZE)
      return;

   //Point to the NBNS message header
   message = netBufferAt(buffer, offset);
   //Sanity check
   if(message == NULL)
      return;

   //Debug message
   TRACE_INFO("NBNS message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message
   dnsDumpMessage((DnsHeader *) message, length);

   //NBNS messages received with an opcode other than zero must be silently
   //ignored
   if(message->opcode != DNS_OPCODE_QUERY)
      return;

   //NBNS messages received with non-zero response codes must be silently
   //ignored
   if(message->rcode != DNS_RCODE_NO_ERROR)
      return;

   //NBNS query received?
   if(!message->qr)
   {
#if (NBNS_RESPONDER_SUPPORT == ENABLED)
      //Process incoming NBNS query message
      nbnsProcessQuery(interface, &pseudoHeader->ipv4Data,
         udpHeader, message, length);
#endif
   }
   //NBNS response received?
   else
   {
#if (NBNS_CLIENT_SUPPORT == ENABLED)
      //Process incoming NBNS response message
      nbnsProcessResponse(interface, &pseudoHeader->ipv4Data,
         udpHeader, message, length);
#endif
   }
}


/**
 * @brief Encode a NetBIOS name
 * @param[in] src Pointer to the name to encode
 * @param[out] dest Pointer to the encoded NetBIOS name
 * @return Length of the encoded NetBIOS name
 **/

size_t nbnsEncodeName(const char_t *src, uint8_t *dest)
{
   size_t i;
   size_t j;
   char_t c;

   //Point to first byte of the output buffer
   j = 0;

   //NetBIOS names are 32-byte long
   dest[j++] = 32;

   //Parse input name
   for(i = 0; i < 15 && src[i] != '\0'; i++)
   {
      //Convert current character to uppercase
      c = osToupper(src[i]);

      //Encode character
      dest[j++] = NBNS_ENCODE_H(c);
      dest[j++] = NBNS_ENCODE_L(c);
   }

   //Pad NetBIOS name with space characters
   for(; i < 15; i++)
   {
      //Encoded space character
      dest[j++] = NBNS_ENCODE_H(' ');
      dest[j++] = NBNS_ENCODE_L(' ');
   }

   //The 16th character is the NetBIOS suffix
   dest[j++] = NBNS_ENCODE_H(0);
   dest[j++] = NBNS_ENCODE_L(0);

   //Terminate the NetBIOS name with a zero length count
   dest[j++] = 0;

   //Return the length of the encoded NetBIOS name
   return j;
}


/**
 * @brief Decode a NetBIOS name
 * @param[in] message Pointer to the NBNS message
 * @param[in] length Length of the NBNS message
 * @param[in] pos Offset of the name to decode
 * @param[out] dest Pointer to the decoded name (optional)
 * @return The position of the resource record that immediately follows the NetBIOS name
 **/

size_t nbnsParseName(const NbnsHeader *message,
   size_t length, size_t pos, char_t *dest)
{
   size_t i;
   size_t n;
   char_t c;

   //Cast the input NBNS message to byte array
   uint8_t *src = (uint8_t *) message;

   //Malformed NBNS message?
   if((pos + 34) >= length)
      return 0;

   //Retrieve the length of the first label
   n = src[pos++];

   //NetBIOS names must be 32-byte long
   if(n != 32)
      return 0;

   //Parse the NetBIOS name
   for(i = 0; i < 15; i++)
   {
      //Make sure the characters of the sequence are valid
      if(src[pos] < 'A' || src[pos] > 'P')
         return 0;
      if(src[pos + 1] < 'A' || src[pos + 1] > 'P')
         return 0;

      //Combine nibbles to restore the original ASCII character
      c = ((src[pos] - 'A') << 4) | (src[pos + 1] - 'A');

      //Padding character found?
      if(c == ' ')
         break;

      //Save current ASCII character
      if(dest != NULL)
         *(dest++) = c;

      //Advance data pointer
      pos += 2;
   }

   //Skip padding characters
   for(; i < 16; i++)
   {
      //Make sure the nibbles are valid
      if(src[pos] < 'A' || src[pos] > 'P')
         return 0;
      if(src[pos + 1] < 'A' || src[pos + 1] > 'P')
         return 0;

      //Advance data pointer
      pos += 2;
   }

   //Retrieve the length of the next label
   n = src[pos++];

   //NetBIOS names are terminated with a zero length count
   if(n != 0)
      return 0;

   //Properly terminate the string
   if(dest != NULL)
      *(dest++) = '\0';

   //Return the position of the resource record that
   //is immediately following the NetBIOS name
   return pos;
}


/**
 * @brief Compare NetBIOS names
 * @param[in] message Pointer to the NBNS message
 * @param[in] length Length of the NBNS message
 * @param[in] pos Offset of the encoded domain name
 * @param[in] name NULL-terminated string that holds a domain name
 * @return TRUE if the NetBIOS names match, else FALSE
 **/

bool_t nbnsCompareName(const NbnsHeader *message,
   size_t length, size_t pos, const char_t *name)
{
   size_t i;
   size_t n;
   char_t c;

   //Cast the input NBNS message to byte array
   uint8_t *src = (uint8_t *) message;

   //Malformed NBNS message?
   if((pos + 34) >= length)
      return FALSE;

   //Retrieve the length of the first label
   n = src[pos++];

   //NetBIOS names must be 32-byte long
   if(n != 32)
      return FALSE;

   //Parse the NetBIOS name
   for(i = 0; i < 15; i++)
   {
      //Make sure the characters of the sequence are valid
      if(src[pos] < 'A' || src[pos] > 'P')
         return FALSE;
      if(src[pos + 1] < 'A' || src[pos + 1] > 'P')
         return FALSE;

      //Combine nibbles to restore the original ASCII character
      c = ((src[pos] - 'A') << 4) | (src[pos + 1] - 'A');

      //Padding character found?
      if(c == ' ' && *name == '\0')
         break;

      //Perform case insensitive comparison
      if(osToupper(c) != osToupper(*name))
         return FALSE;

      //Advance data pointer
      pos += 2;
      name++;
   }

   //Skip padding characters
   for(; i < 16; i++)
   {
      //Make sure the nibbles are valid
      if(src[pos] < 'A' || src[pos] > 'P')
         return FALSE;
      if(src[pos + 1] < 'A' || src[pos + 1] > 'P')
         return FALSE;

      //Advance data pointer
      pos += 2;
   }

   //Retrieve the length of the next label
   n = src[pos];

   //NetBIOS names are terminated with a zero length count
   if(n != 0)
      return FALSE;

   //The NetBIOS names match
   return TRUE;
}

#endif
#endif
