/**
 * @file ip.c
 * @brief IPv4 and IPv6 common routines
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
#define TRACE_LEVEL IP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "core/ethernet.h"
#include "core/ip.h"
#include "ipv4/ipv4.h"
#include "ipv4/ipv4_misc.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "debug.h"

//Special IP addresses
const IpAddr IP_ADDR_ANY = {0};
const IpAddr IP_ADDR_UNSPECIFIED = {0};


/**
 * @brief Send an IP datagram
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IP pseudo header
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in] offset Offset to the first payload byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ipSendDatagram(NetInterface *interface, IpPseudoHeader *pseudoHeader,
   NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   error_t error;

#if (IPV4_SUPPORT == ENABLED)
   //Destination address is an IPv4 address?
   if(pseudoHeader->length == sizeof(Ipv4PseudoHeader))
   {
      //Form an IPv4 packet and send it
      error = ipv4SendDatagram(interface, &pseudoHeader->ipv4Data, buffer,
         offset, ancillary);
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //Destination address is an IPv6 address?
   if(pseudoHeader->length == sizeof(Ipv6PseudoHeader))
   {
      //Form an IPv6 packet and send it
      error = ipv6SendDatagram(interface, &pseudoHeader->ipv6Data, buffer,
         offset, ancillary);
   }
   else
#endif
   //Destination address is invalid
   {
      //Report an error
      error = ERROR_INVALID_ADDRESS;
   }

   //Return status code
   return error;
}


/**
 * @brief IP source address selection
 *
 * This function selects the source address and the relevant network interface
 * to be used in order to join the specified destination address
 *
 * @param[in,out] interface A pointer to a valid network interface may be
 *   provided as a hint. The function returns a pointer identifying the
 *   interface to be used
 * @param[in] destAddr Destination IP address
 * @param[out] srcAddr Local IP address to be used
 * @return Error code
 **/

error_t ipSelectSourceAddr(NetInterface **interface,
   const IpAddr *destAddr, IpAddr *srcAddr)
{
   error_t error;

#if (IPV4_SUPPORT == ENABLED)
   //The destination address is an IPv4 address?
   if(destAddr->length == sizeof(Ipv4Addr))
   {
      //An IPv4 address is expected
      srcAddr->length = sizeof(Ipv4Addr);

      //Get the most appropriate source address to use
      error = ipv4SelectSourceAddr(interface, destAddr->ipv4Addr,
         &srcAddr->ipv4Addr);
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //The destination address is an IPv6 address?
   if(destAddr->length == sizeof(Ipv6Addr))
   {
      //An IPv6 address is expected
      srcAddr->length = sizeof(Ipv6Addr);

      //Get the most appropriate source address to use
      error = ipv6SelectSourceAddr(interface, &destAddr->ipv6Addr,
         &srcAddr->ipv6Addr);
   }
   else
#endif
   //The destination address is not valid?
   {
      //Report an error
      error = ERROR_INVALID_ADDRESS;
   }

   //Return status code
   return error;
}


/**
 * @brief Compare IP addresses
 * @param[in] ipAddr1 First IP address
 * @param[in] ipAddr2 Second IP address
 * @return Comparison result
 **/

bool_t ipCompAddr(const IpAddr *ipAddr1, const IpAddr *ipAddr2)
{
   bool_t result;

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 addresses?
   if(ipAddr1->length == sizeof(Ipv4Addr) && ipAddr2->length == sizeof(Ipv4Addr))
   {
      //Compare IPv4 addresses
      if(ipAddr1->ipv4Addr == ipAddr2->ipv4Addr)
      {
         result = TRUE;
      }
      else
      {
         result = FALSE;
      }
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 addresses?
   if(ipAddr1->length == sizeof(Ipv6Addr) && ipAddr2->length == sizeof(Ipv6Addr))
   {
      //Compare IPv6 addresses
      result = ipv6CompAddr(&ipAddr1->ipv6Addr, &ipAddr2->ipv6Addr);
   }
   else
#endif
   //Unspecified IP addresses?
   if(ipAddr1->length == 0 && ipAddr2->length == 0)
   {
      result = TRUE;
   }
   //Invalid IP addresses?
   else
   {
      result = FALSE;
   }

   //Return TRUE if the IP addresses match, else FALSE
   return result;
}


/**
 * @brief Compare an IP address against the unspecified address
 * @param[in] ipAddr IP address
 * @return TRUE if the IP address is unspecified, else FALSE
 **/

bool_t ipIsUnspecifiedAddr(const IpAddr *ipAddr)
{
   bool_t result;

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 address?
   if(ipAddr->length == sizeof(Ipv4Addr))
   {
      //Compare IPv4 address
      if(ipAddr->ipv4Addr == IPV4_UNSPECIFIED_ADDR)
      {
         result = TRUE;
      }
      else
      {
         result = FALSE;
      }
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 address?
   if(ipAddr->length == sizeof(Ipv6Addr))
   {
      //Compare IPv6 address
      result = ipv6CompAddr(&ipAddr->ipv6Addr, &IPV6_UNSPECIFIED_ADDR);
   }
   else
#endif
   //Invalid IP address?
   {
      result = FALSE;
   }

   //Return TRUE if the IP address is unspecified, else FALSE
   return result;
}


/**
 * @brief Determine whether an IP address is a multicast address
 * @param[in] ipAddr IP address
 * @return TRUE if the IP address is a multicast address, else FALSE
 **/

bool_t ipIsMulticastAddr(const IpAddr *ipAddr)
{
   bool_t result;

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 address?
   if(ipAddr->length == sizeof(Ipv4Addr))
   {
      //Check whether the IPv4 address is a multicast address
      result = ipv4IsMulticastAddr(ipAddr->ipv4Addr);
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 address?
   if(ipAddr->length == sizeof(Ipv6Addr))
   {
      //Check whether the IPv6 address is a multicast address
      result = ipv6IsMulticastAddr(&ipAddr->ipv6Addr);
   }
   else
#endif
   //Invalid IP address?
   {
      result = FALSE;
   }

   //Return TRUE if the IP address is a multicast address, else FALSE
   return result;
}


/**
 * @brief Join the specified host group
 * @param[in] interface Underlying network interface (optional parameter)
 * @param[in] groupAddr IP address identifying the host group to join
 * @return Error code
 **/

error_t ipJoinMulticastGroup(NetInterface *interface, const IpAddr *groupAddr)
{
   error_t error;

   //Use default network interface?
   if(interface == NULL)
      interface = netGetDefaultInterface();

   //Get exclusive access
   osAcquireMutex(&netMutex);

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 multicast address?
   if(groupAddr->length == sizeof(Ipv4Addr))
   {
      //Join the specified host group
      error = ipv4JoinMulticastGroup(interface, groupAddr->ipv4Addr);
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 multicast address?
   if(groupAddr->length == sizeof(Ipv6Addr))
   {
      //Join the specified host group
      error = ipv6JoinMulticastGroup(interface, &groupAddr->ipv6Addr);
   }
   else
#endif
   //Invalid IP address?
   {
      //Report an error
      error = ERROR_INVALID_ADDRESS;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief Leave the specified host group
 * @param[in] interface Underlying network interface (optional parameter)
 * @param[in] groupAddr IP address identifying the host group to leave
 * @return Error code
 **/

error_t ipLeaveMulticastGroup(NetInterface *interface, const IpAddr *groupAddr)
{
   error_t error;

   //Use default network interface?
   if(interface == NULL)
      interface = netGetDefaultInterface();

   //Get exclusive access
   osAcquireMutex(&netMutex);

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 multicast address?
   if(groupAddr->length == sizeof(Ipv4Addr))
   {
      //Drop membership
      error = ipv4LeaveMulticastGroup(interface, groupAddr->ipv4Addr);
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 multicast address?
   if(groupAddr->length == sizeof(Ipv6Addr))
   {
      //Drop membership
      error = ipv6LeaveMulticastGroup(interface, &groupAddr->ipv6Addr);
   }
   else
#endif
   //Invalid IP address?
   {
      error = ERROR_INVALID_ADDRESS;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief IP checksum calculation
 * @param[in] data Pointer to the data over which to calculate the IP checksum
 * @param[in] length Number of bytes to process
 * @return Checksum value
 **/

uint16_t ipCalcChecksum(const void *data, size_t length)
{
   uint32_t temp;
   uint32_t checksum;
   const uint8_t *p;

   //Checksum preset value
   checksum = 0x0000;

   //Point to the data over which to calculate the IP checksum
   p = (const uint8_t *) data;

   //Pointer not aligned on a 16-bit boundary?
   if(((uintptr_t) p & 1) != 0)
   {
      if(length >= 1)
      {
#ifdef _CPU_BIG_ENDIAN
         //Update checksum value
         checksum += (uint32_t) *p;
#else
         //Update checksum value
         checksum += (uint32_t) *p << 8;
#endif
         //Restore the alignment on 16-bit boundaries
         p++;
         //Number of bytes left to process
         length--;
      }
   }

   //Pointer not aligned on a 32-bit boundary?
   if(((uintptr_t) p & 2) != 0)
   {
      if(length >= 2)
      {
         //Update checksum value
         checksum += (uint32_t) *((uint16_t *) p);

         //Restore the alignment on 32-bit boundaries
         p += 2;
         //Number of bytes left to process
         length -= 2;
      }
   }

   //Process the data 4 bytes at a time
   while(length >= 4)
   {
      //Update checksum value
      temp = checksum + *((uint32_t *) p);

      //Add carry bit, if any
      if(temp < checksum)
      {
         checksum = temp + 1;
      }
      else
      {
         checksum = temp;
      }

      //Point to the next 32-bit word
      p += 4;
      //Number of bytes left to process
      length -= 4;
   }

   //Fold 32-bit sum to 16 bits
   checksum = (checksum & 0xFFFF) + (checksum >> 16);

   //Add left-over 16-bit word, if any
   if(length >= 2)
   {
      //Update checksum value
      checksum += (uint32_t) *((uint16_t *) p);

      //Point to the next byte
      p += 2;
      //Number of bytes left to process
      length -= 2;
   }

   //Add left-over byte, if any
   if(length >= 1)
   {
#ifdef _CPU_BIG_ENDIAN
      //Update checksum value
      checksum += (uint32_t) *p << 8;
#else
      //Update checksum value
      checksum += (uint32_t) *p;
#endif
   }

   //Fold 32-bit sum to 16 bits (first pass)
   checksum = (checksum & 0xFFFF) + (checksum >> 16);
   //Fold 32-bit sum to 16 bits (second pass)
   checksum = (checksum & 0xFFFF) + (checksum >> 16);

   //Restore checksum endianness
   if(((uintptr_t) data & 1) != 0)
   {
      //Swap checksum value
      checksum = ((checksum >> 8) | (checksum << 8)) & 0xFFFF;
   }

   //Return 1's complement value
   return checksum ^ 0xFFFF;
}


/**
 * @brief Calculate IP checksum over a multi-part buffer
 * @param[in] buffer Pointer to the multi-part buffer
 * @param[in] offset Offset from the beginning of the buffer
 * @param[in] length Number of bytes to process
 * @return Checksum value
 **/

uint16_t ipCalcChecksumEx(const NetBuffer *buffer, size_t offset, size_t length)
{
   uint_t i;
   uint_t n;
   uint_t pos;
   uint8_t *data;
   uint32_t checksum;

   //Checksum preset value
   checksum = 0x0000;

   //Current position in the multi-part buffer
   pos = 0;

   //Loop through data chunks
   for(i = 0; i < buffer->chunkCount && pos < length; i++)
   {
      //Is there any data to process in the current chunk?
      if(offset < buffer->chunk[i].length)
      {
         //Point to the first data byte
         data = (uint8_t *) buffer->chunk[i].address + offset;

         //Number of bytes available in the current chunk
         n = buffer->chunk[i].length - offset;
         //Limit the number of byte to process
         n = MIN(n, length - pos);

         //Take care of alignment issues
         if((pos & 1) != 0)
         {
            //Swap checksum value
            checksum = ((checksum >> 8) | (checksum << 8)) & 0xFFFF;
         }

         //Process data chunk
         checksum += ipCalcChecksum(data, n) ^ 0xFFFF;
         //Fold 32-bit sum to 16 bits
         checksum = (checksum & 0xFFFF) + (checksum >> 16);

         //Restore checksum endianness
         if((pos & 1) != 0)
         {
            //Swap checksum value
            checksum = ((checksum >> 8) | (checksum << 8)) & 0xFFFF;
         }

         //Advance current position
         pos += n;
         //Process the next block from the start
         offset = 0;
      }
      else
      {
         //Skip the current chunk
         offset -= buffer->chunk[i].length;
      }
   }

   //Return 1's complement value
   return checksum ^ 0xFFFF;
}


/**
 * @brief Calculate IP upper-layer checksum
 * @param[in] pseudoHeader Pointer to the pseudo header
 * @param[in] pseudoHeaderLen Pseudo header length
 * @param[in] data Pointer to the upper-layer data
 * @param[in] dataLen Upper-layer data length
 * @return Checksum value
 **/

uint16_t ipCalcUpperLayerChecksum(const void *pseudoHeader,
   size_t pseudoHeaderLen, const void *data, size_t dataLen)
{
   uint32_t checksum;

   //Process pseudo header
   checksum = ipCalcChecksum(pseudoHeader, pseudoHeaderLen) ^ 0xFFFF;
   //Process upper-layer data
   checksum += ipCalcChecksum(data, dataLen) ^ 0xFFFF;
   //Fold 32-bit sum to 16 bits
   checksum = (checksum & 0xFFFF) + (checksum >> 16);

   //Return 1's complement value
   return checksum ^ 0xFFFF;
}


/**
 * @brief Calculate IP upper-layer checksum over a multi-part buffer
 * @param[in] pseudoHeader Pointer to the pseudo header
 * @param[in] pseudoHeaderLen Pseudo header length
 * @param[in] buffer Multi-part buffer containing the upper-layer data
 * @param[in] offset Offset from the first data byte to process
 * @param[in] length Number of data bytes to process
 * @return Checksum value
 **/

uint16_t ipCalcUpperLayerChecksumEx(const void *pseudoHeader,
   size_t pseudoHeaderLen, const NetBuffer *buffer, size_t offset, size_t length)
{
   uint32_t checksum;

   //Process pseudo header
   checksum = ipCalcChecksum(pseudoHeader, pseudoHeaderLen) ^ 0xFFFF;
   //Process upper-layer data
   checksum += ipCalcChecksumEx(buffer, offset, length) ^ 0xFFFF;
   //Fold 32-bit sum to 16 bits
   checksum = (checksum & 0xFFFF) + (checksum >> 16);

   //Return 1's complement value
   return checksum ^ 0xFFFF;
}


/**
 * @brief Allocate a buffer to hold an IP packet
 * @param[in] length Desired payload length
 * @param[out] offset Offset to the first byte of the payload
 * @return The function returns a pointer to the newly allocated
 *   buffer. If the system is out of resources, NULL is returned
 **/

NetBuffer *ipAllocBuffer(size_t length, size_t *offset)
{
   size_t headerLen;
   NetBuffer *buffer;

#if (IPV6_SUPPORT == ENABLED)
   //Maximum overhead when using IPv6
   headerLen = sizeof(Ipv6Header) + sizeof(Ipv6FragmentHeader);
#else
   //Maximum overhead when using IPv4
   headerLen = sizeof(Ipv4Header);
#endif

#if (ETH_SUPPORT == ENABLED)
   //Allocate a buffer to hold the Ethernet header and the IP packet
   buffer = ethAllocBuffer(length + headerLen, offset);
#elif (PPP_SUPPORT == ENABLED)
   //Allocate a buffer to hold the PPP header and the IP packet
   buffer = pppAllocBuffer(length + headerLen, offset);
#else
   //Allocate a buffer to hold the IP packet
   buffer = netBufferAlloc(length + headerLen);
   //Clear offset value
   *offset = 0;
#endif

   //Successful memory allocation?
   if(buffer != NULL)
   {
      //Offset to the first byte of the payload
      *offset += headerLen;
   }

   //Return a pointer to the freshly allocated buffer
   return buffer;
}


/**
 * @brief Convert a string representation of an IP address to a binary IP address
 * @param[in] str NULL-terminated string representing the IP address
 * @param[out] ipAddr Binary representation of the IP address
 * @return Error code
 **/

error_t ipStringToAddr(const char_t *str, IpAddr *ipAddr)
{
   error_t error;

#if (IPV6_SUPPORT == ENABLED)
   //IPv6 address?
   if(osStrchr(str, ':') != NULL)
   {
      //IPv6 addresses are 16-byte long
      ipAddr->length = sizeof(Ipv6Addr);
      //Convert the string to IPv6 address
      error = ipv6StringToAddr(str, &ipAddr->ipv6Addr);
   }
   else
#endif
#if (IPV4_SUPPORT == ENABLED)
   //IPv4 address?
   if(osStrchr(str, '.') != NULL)
   {
      //IPv4 addresses are 4-byte long
      ipAddr->length = sizeof(Ipv4Addr);
      //Convert the string to IPv4 address
      error = ipv4StringToAddr(str, &ipAddr->ipv4Addr);
   }
   else
#endif
   //Invalid IP address?
   {
      //Report an error
      error = ERROR_FAILURE;
   }

   //Return status code
   return error;
}


/**
 * @brief Convert a binary IP address to a string representation
 * @param[in] ipAddr Binary representation of the IP address
 * @param[out] str NULL-terminated string representing the IP address
 * @return Pointer to the formatted string
 **/

char_t *ipAddrToString(const IpAddr *ipAddr, char_t *str)
{
#if (IPV4_SUPPORT == ENABLED)
   //IPv4 address?
   if(ipAddr->length == sizeof(Ipv4Addr))
   {
      //Convert IPv4 address to string representation
      return ipv4AddrToString(ipAddr->ipv4Addr, str);
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 address?
   if(ipAddr->length == sizeof(Ipv6Addr))
   {
      //Convert IPv6 address to string representation
      return ipv6AddrToString(&ipAddr->ipv6Addr, str);
   }
   else
#endif
   //Invalid IP address?
   {
      static char_t c;

      //The last parameter is optional
      if(str == NULL)
      {
         str = &c;
      }

      //Properly terminate the string
      str[0] = '\0';

      //Return an empty string
      return str;
   }
}
