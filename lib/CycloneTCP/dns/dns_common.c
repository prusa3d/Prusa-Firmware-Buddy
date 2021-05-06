/**
 * @file dns_common.c
 * @brief Common DNS routines
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
#include <string.h>
#include "core/net.h"
#include "dns/dns_client.h"
#include "dns/dns_common.h"
#include "mdns/mdns_client.h"
#include "mdns/mdns_responder.h"
#include "mdns/mdns_common.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (DNS_CLIENT_SUPPORT == ENABLED || MDNS_CLIENT_SUPPORT == ENABLED || \
   MDNS_RESPONDER_SUPPORT == ENABLED)


/**
 * @brief Encode a domain name using the DNS name notation
 * @param[in] src Pointer to the domain name to encode
 * @param[out] dest Pointer to the encoded domain name (optional parameter)
 * @return Length of the encoded domain name
 **/

size_t dnsEncodeName(const char_t *src, uint8_t *dest)
{
   uint_t i = 0;
   size_t length = 0;

   //Parse input name
   while(1)
   {
      //End of string detected?
      if(src[i] == '\0')
      {
         //Check label length
         if(i < 1 || i > DNS_LABEL_MAX_SIZE)
            return 0;

         //Save label length
         if(dest != NULL)
         {
            dest[0] = i;
            dest[i + 1] = 0;
         }

         //Adjust the length of the resulting string
         length += i + 2;

         //Stop parsing the input string
         return length;
      }
      //Separator detected?
      else if(src[i] == '.')
      {
         //Check label length
         if(i < 1 || i > DNS_LABEL_MAX_SIZE)
            return 0;

         //Save label length
         if(dest != NULL)
            dest[0] = i;

         //Adjust the length of the resulting string
         length += i + 1;

         //Advance write pointer
         if(dest != NULL)
            dest += i + 1;

         //Prepare to decode the next label
         src += i + 1;
         i = 0;
      }
      //Any other character?
      else
      {
         //Copy current character
         if(dest != NULL)
            dest[i + 1] = src[i];

         //Point to the next character
         i++;
      }
   }
}


/**
 * @brief Decode a domain name that uses the DNS name encoding
 * @param[in] message Pointer to the DNS message
 * @param[in] length Length of the DNS message
 * @param[in] pos Offset of the name to decode
 * @param[out] dest Pointer to the decoded name (optional)
 * @param[in] level Current level of recursion
 * @return The position of the resource record that immediately follows the domain name
 **/

size_t dnsParseName(const DnsHeader *message,
   size_t length, size_t pos, char_t *dest, uint_t level)
{
   size_t n;
   size_t pointer;
   uint8_t *src;

   //Recursion limit exceeded?
   if(level >= DNS_NAME_MAX_RECURSION)
      return 0;

   //Cast the input DNS message to byte array
   src = (uint8_t *) message;

   //Parse encoded domain name
   while(pos < length)
   {
      //End marker found?
      if(src[pos] == 0)
      {
         //Properly terminate the string
         if(dest != NULL)
            *dest = '\0';

         //Return the position of the resource record that
         //is immediately following the domain name
         return (pos + 1);
      }
      //Compression tag found?
      else if(src[pos] >= DNS_COMPRESSION_TAG)
      {
         //Malformed DNS message?
         if((pos + 1) >= length)
            return 0;

         //Read the most significant byte of the pointer
         pointer = (src[pos] & ~DNS_COMPRESSION_TAG) << 8;
         //Read the least significant byte of the pointer
         pointer |= src[pos + 1];

         //Decode the remaining part of the domain name
         if(!dnsParseName(message, length, pointer, dest, level + 1))
         {
            //Domain name decoding failed
            return 0;
         }

         //Return the position of the resource record that
         //is immediately following the domain name
         return (pos + 2);
      }
      //Valid label length?
      else if(src[pos] < DNS_LABEL_MAX_SIZE)
      {
         //Get the length of the current label
         n = src[pos++];

         //Malformed DNS message?
         if((pos + n) > length)
            return 0;

         //The last parameter is optional
         if(dest != NULL)
         {
            //Copy current label
            osMemcpy(dest, src + pos, n);

            //Advance read pointer
            pos += n;
            //Advance write pointer
            dest += n;

            //Append a separator if necessary
            if(pos < length && src[pos] != '\0')
               *(dest++) = '.';
         }
         else
         {
            //Advance read pointer
            pos += n;
         }
      }
      //Invalid label length?
      else
      {
         //Properly terminate the string
         if(dest != NULL)
            *dest = '\0';
         //Domain name decoding failed
         return 0;
      }
   }

   //Domain name decoding failed
   return 0;
}


/**
 * @brief Compare domain names
 * @param[in] message Pointer to the DNS message
 * @param[in] length Length of the DNS message
 * @param[in] pos Offset of the encoded domain name
 * @param[in] name NULL-terminated string that holds a domain name
 * @param[in] level Current level of recursion
 * @return The function returns 0 if the domain names match, -1 if the first
 *   domain name lexicographically precedes the second name, or 1 if the
 *   second domain name lexicographically precedes the first name
 **/

int_t dnsCompareName(const DnsHeader *message, size_t length,
   size_t pos, const char_t *name, uint_t level)
{
   int_t res;
   size_t n;
   size_t pointer;
   uint8_t *p;

   //Recursion limit exceeded?
   if(level >= DNS_NAME_MAX_RECURSION)
      return -2;

   //Cast the DNS message to byte array
   p = (uint8_t *) message;

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
         if(*name != '\0')
            return -1;

         //The domain names match each other
         return 0;
      }
      //Compression tag found?
      else if(n >= DNS_COMPRESSION_TAG)
      {
         //Malformed DNS message?
         if((pos + 1) >= length)
            return FALSE;

         //Read the most significant byte of the pointer
         pointer = (p[pos] & ~DNS_COMPRESSION_TAG) << 8;
         //Read the least significant byte of the pointer
         pointer |= p[pos + 1];

         //Compare the remaining part
         res = dnsCompareName(message, length, pointer, name, level + 1);

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
         res = osStrncasecmp((char_t *) p + pos, name, n);
         //Any mismatch?
         if(res)
            return res;

         //Advance data pointer
         pos += n;
         name += n;

         //The domain name which still has remaining data is deemed
         //lexicographically later
         if(*name != '\0' && *name != '.')
            return -1;

         //Skip the separator character, if any
         if(*name == '.')
            name++;
      }
   }

   //Malformed DNS message
   return -2;
}


/**
 * @brief Compare domain names encoded with DNS notation
 * @param[in] message1 Pointer to the first DNS message
 * @param[in] length1 Length of the first DNS message
 * @param[in] pos1 Offset of the encoded domain name within the first message
 * @param[in] message2 Pointer to the second DNS message
 * @param[in] length2 Length of the second DNS message
 * @param[in] pos2 Offset of the encoded domain name within the second message
 * @param[in] level Current level of recursion
 * @return The function returns 0 if the domain names match, -1 if the first
 *   domain name lexicographically precedes the second name, or 1 if the
 *   second domain name lexicographically precedes the first name
 **/

int_t dnsCompareEncodedName(const DnsHeader *message1, size_t length1, size_t pos1,
   const DnsHeader *message2, size_t length2, size_t pos2, uint_t level)
{
   int_t res;
   size_t n;
   size_t n1;
   size_t n2;
   size_t pointer1;
   size_t pointer2;
   uint8_t *p1;
   uint8_t *p2;

   //Recursion limit exceeded?
   if(level >= DNS_NAME_MAX_RECURSION)
      return -2;

   //Cast DNS messages to byte array
   p1 = (uint8_t *) message1;
   p2 = (uint8_t *) message2;

   //Compare encoded domain names
   while(pos1 < length1 && pos2 < length2)
   {
      //Retrieve the length of each label
      n1 = p1[pos1];
      n2 = p2[pos2];

      //End marker found?
      if(n1 == 0 || n2 == 0)
      {
         //The domain name which still has remaining data is deemed
         //lexicographically later
         if(n1 < n2)
            return -1;
         else if(n1 > n2)
            return 1;

         //The domain names match each other
         return 0;
      }
      //Compression tag found?
      else if(n1 >= DNS_COMPRESSION_TAG || n2 >= DNS_COMPRESSION_TAG)
      {
         //First domain name compressed?
         if(n1 >= DNS_COMPRESSION_TAG)
         {
            //Malformed DNS message?
            if((pos1 + 1) >= length1)
               return -2;

            //Read the most significant byte of the pointer
            pointer1 = (p1[pos1] & ~DNS_COMPRESSION_TAG) << 8;
            //Read the least significant byte of the pointer
            pointer1 |= p1[pos1 + 1];
         }
         else
         {
            //The first domain name is not compressed
            pointer1 = pos1;
         }

         //Second domain name compressed?
         if(n2 >= DNS_COMPRESSION_TAG)
         {
            //Malformed DNS message?
            if((pos2 + 1) >= length2)
               return -2;

            //Read the most significant byte of the pointer
            pointer2 = (p2[pos2] & ~DNS_COMPRESSION_TAG) << 8;
            //Read the least significant byte of the pointer
            pointer2 |= p2[pos2 + 1];
         }
         else
         {
            //The second domain name is not compressed
            pointer2 = pos2;
         }

         //Compare the remaining part
         res = dnsCompareEncodedName(message1, length1, pointer1,
            message2, length2, pointer2, level + 1);

         //Return comparison result
         return res;
      }
      else
      {
         //Advance data pointer
         pos1++;
         pos2++;

         //Malformed DNS message?
         if((pos1 + n1) > length1 || (pos2 + n2) > length2)
            return -2;

         //Compare as much data as possible
         n = MIN(n1, n2);

         //Compare labels
         res = osStrncasecmp((char_t *) p1 + pos1, (char_t *) p2 + pos2, n);
         //Any mismatch?
         if(res)
            return res;

         //The domain name which still has remaining data is deemed
         //lexicographically later
         if(n1 < n2)
            return -1;
         else if(n1 > n2)
            return 1;

         //Advance data pointer
         pos1 += n1;
         pos2 += n2;
      }
   }

   //Malformed DNS message
   return -2;
}

#endif
