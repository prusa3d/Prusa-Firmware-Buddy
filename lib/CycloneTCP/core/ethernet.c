/**
 * @file ethernet.c
 * @brief Ethernet
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
#define TRACE_LEVEL ETH_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "core/net.h"
#include "core/nic.h"
#include "core/ethernet.h"
#include "core/ethernet_misc.h"
#include "core/socket.h"
#include "core/raw_socket.h"
#include "core/tcp_timer.h"
#include "ipv4/arp.h"
#include "ipv4/ipv4.h"
#include "ipv6/ipv6.h"
#include "mibs/mib2_module.h"
#include "mibs/if_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (ETH_SUPPORT == ENABLED)

//Unspecified MAC address
const MacAddr MAC_UNSPECIFIED_ADDR = {{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}};
//Broadcast MAC address
const MacAddr MAC_BROADCAST_ADDR = {{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}};


/**
 * @brief Ethernet related initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ethInit(NetInterface *interface)
{
   //Clear the MAC filter table contents
   osMemset(interface->macAddrFilter, 0,
      sizeof(interface->macAddrFilter));

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Process an incoming Ethernet frame
 * @param[in] interface Underlying network interface
 * @param[in] frame Incoming Ethernet frame to process
 * @param[in] length Total frame length
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 **/

void ethProcessFrame(NetInterface *interface, uint8_t *frame, size_t length,
   NetRxAncillary *ancillary)
{
   error_t error;
   uint_t i;
   uint16_t type;
   uint8_t *data;
   EthHeader *header;
   NetInterface *virtualInterface;

#if (IPV6_SUPPORT == ENABLED)
   NetBuffer1 buffer;
#endif
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   uint8_t port = 0;
#endif
#if (ETH_VLAN_SUPPORT == ENABLED)
   uint16_t vlanId = 0;
#endif
#if (ETH_VMAN_SUPPORT == ENABLED)
   uint16_t vmanId = 0;
#endif

   //Initialize status code
   error = NO_ERROR;

   //Initialize variables
   type = 0;
   data = NULL;
   header = NULL;

   //Start of exception handling block
   do
   {
      //Check whether the CRC is included in the received frame
      if(!interface->nicDriver->autoCrcStrip)
      {
         //Perform CRC verification
         error = ethCheckCrc(interface, frame, length);
         //CRC error?
         if(error)
            break;

         //Strip CRC field from Ethernet frame
         length -= ETH_CRC_SIZE;
      }

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
      //Check whether port tagging is supported by the switch
      if(interface->switchDriver != NULL &&
         interface->switchDriver->untagFrame != NULL)
      {
         //Decode VLAN tag (SMSC switches) or tail tag (Micrel switches)
         error = interface->switchDriver->untagFrame(interface, &frame,
            &length, ancillary);
         //Any error to report?
         if(error)
            break;
      }
#endif

      //Point to the beginning of the frame
      header = (EthHeader *) frame;

      //Total number of octets received on the interface
      MIB2_INC_COUNTER32(ifGroup.ifTable[interface->index].ifInOctets, length);
      IF_MIB_INC_COUNTER32(ifTable[interface->index].ifInOctets, length);
      IF_MIB_INC_COUNTER64(ifXTable[interface->index].ifHCInOctets, length);

      //Malformed Ethernet frame?
      if(length < sizeof(EthHeader))
      {
         //Drop the received frame
         error = ERROR_INVALID_LENGTH;
         break;
      }

      //Debug message
      TRACE_DEBUG("Ethernet frame received (%" PRIuSIZE " bytes)...\r\n", length);
      //Dump Ethernet header contents for debugging purpose
      ethDumpHeader(header);

#if defined(ETH_FRAME_FORWARD_HOOK)
      ETH_FRAME_FORWARD_HOOK(interface, header, length);
#endif

      //Retrieve the value of the EtherType field
      type = ntohs(header->type);

      //Point to the data payload
      data = header->data;
      //Calculate the length of the data payload
      length -= sizeof(EthHeader);

#if (ETH_VMAN_SUPPORT == ENABLED)
      //VMAN tag found?
      if(type == ETH_TYPE_VMAN)
      {
         //Decode VMAN tag
         error = ethDecodeVlanTag(data, length, &vmanId, &type);
         //Any error to report?
         if(error)
            break;

         //Advance data pointer over the VMAN tag
         data += sizeof(VlanTag);
         length -= sizeof(VlanTag);
      }
#endif

#if (ETH_VLAN_SUPPORT == ENABLED)
      //VLAN tag found?
      if(type == ETH_TYPE_VLAN)
      {
         //Decode VLAN tag
         error = ethDecodeVlanTag(data, length, &vlanId, &type);
         //Any error to report?
         if(error)
            break;

         //Advance data pointer over the VLAN tag
         data += sizeof(VlanTag);
         length -= sizeof(VlanTag);
      }
#endif

      //End of exception handling block
   } while(0);

   //Invalid frame received?
   if(error)
   {
      //Update Ethernet statistics
      ethUpdateErrorStats(interface, error);
      //Drop the received frame
      return;
   }

#if (ETH_VLAN_SUPPORT == ENABLED)
   //Dump VLAN identifier
   if(vlanId != 0)
   {
      TRACE_DEBUG("  VLAN Id = %" PRIu16 "\r\n", vlanId);
   }
#endif
#if (ETH_VMAN_SUPPORT == ENABLED)
   //Dump VMAN identifier
   if(vmanId != 0)
   {
      TRACE_DEBUG("  VMAN Id = %" PRIu16 "\r\n", vmanId);
   }
#endif
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Dump switch port identifier
   if(ancillary->port != 0)
   {
      TRACE_DEBUG("  Switch Port = %" PRIu8 "\r\n", ancillary->port);
   }
#endif

   //802.1Q allows a single physical interface to be bound to multiple
   //virtual interfaces
   for(i = 0; i < NET_INTERFACE_COUNT; i++)
   {
      //Point to the current interface
      virtualInterface = &netInterface[i];

      //Check whether the current virtual interface is attached to the
      //physical interface where the packet was received
      if(nicGetPhysicalInterface(virtualInterface) != interface)
         continue;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
      //Retrieve switch port identifier
      port = nicGetSwitchPort(virtualInterface);

      //Check switch port identifier
      if(port != 0 && port != ancillary->port)
         continue;
#endif
#if (ETH_VLAN_SUPPORT == ENABLED)
      //Check VLAN identifier
      if((nicGetVlanId(virtualInterface) & VLAN_VID_MASK) != vlanId)
         continue;
#endif
#if (ETH_VMAN_SUPPORT == ENABLED)
      //Check VMAN identifier
      if((nicGetVmanId(virtualInterface) & VLAN_VID_MASK) != vmanId)
         continue;
#endif

      //The host must silently discards an incoming frame whose destination
      //address does not correspond to the physical interface through which
      //it was received
      error = ethCheckDestAddr(virtualInterface, &header->destAddr);

      //Valid destination address?
      if(!error)
      {
         //Save source and destination MAC addresses
         ancillary->srcMacAddr = header->srcAddr;
         ancillary->destMacAddr = header->destAddr;

         //Update Ethernet statistics
         ethUpdateInStats(virtualInterface, &header->destAddr);

#if (ETH_LLC_SUPPORT == ENABLED)
         //Values of 1500 and below mean that it is used to indicate the size
         //of the payload in octets
         if(type <= ETH_MTU && type <= length)
         {
            //Any registered callback?
            if(virtualInterface->llcRxCallback != NULL)
            {
               //Process incoming LLC frame
               virtualInterface->llcRxCallback(virtualInterface, header, data,
                  type, ancillary, virtualInterface->llcRxParam);
            }
         }
#endif

#if (RAW_SOCKET_SUPPORT == ENABLED)
         //Allow raw sockets to process Ethernet packets
         rawSocketProcessEthPacket(virtualInterface, header, data, length,
            ancillary);
#endif
         //Check Ethernet type field
         switch(type)
         {
#if (IPV4_SUPPORT == ENABLED)
         //ARP packet received?
         case ETH_TYPE_ARP:
            //Process incoming ARP packet
            arpProcessPacket(virtualInterface, (ArpPacket *) data, length);
            //Continue processing
            break;

         //IPv4 packet received?
         case ETH_TYPE_IPV4:
            //Process incoming IPv4 packet
            ipv4ProcessPacket(virtualInterface, (Ipv4Header *) data, length,
               ancillary);
            //Continue processing
            break;
#endif
#if (IPV6_SUPPORT == ENABLED)
         //IPv6 packet received?
         case ETH_TYPE_IPV6:
            //The incoming Ethernet frame fits in a single chunk
            buffer.chunkCount = 1;
            buffer.maxChunkCount = 1;
            buffer.chunk[0].address = data;
            buffer.chunk[0].length = (uint16_t) length;
            buffer.chunk[0].size = 0;

            //Process incoming IPv6 packet
            ipv6ProcessPacket(virtualInterface, (NetBuffer *) &buffer, 0,
               ancillary);
            //Continue processing
            break;
#endif
         //Unknown packet received?
         default:
            //Drop the received frame
            error = ERROR_INVALID_PROTOCOL;
            break;
         }
      }

      //Invalid frame received?
      if(error)
      {
         //Update Ethernet statistics
         ethUpdateErrorStats(virtualInterface, error);
      }
   }
}


/**
 * @brief Send an Ethernet frame
 * @param[in] interface Underlying network interface
 * @param[in] destAddr MAC address of the destination host
 * @param[in] type Ethernet type
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in] offset Offset to the first payload byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ethSendFrame(NetInterface *interface, const MacAddr *destAddr,
   uint16_t type, NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   error_t error;
   uint32_t crc;
   size_t length;
   EthHeader *header;
   NetInterface *physicalInterface;
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   uint8_t port;
#endif
#if (ETH_VLAN_SUPPORT == ENABLED)
   uint16_t vlanId;
#endif
#if (ETH_VMAN_SUPPORT == ENABLED)
   uint16_t vmanId;
#endif

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Get the switch port identifier assigned to the interface
   port = nicGetSwitchPort(interface);

   //Port separation mode?
   if(port != 0)
   {
      //Force the destination port
      ancillary->port = port;
   }
#endif

#if (ETH_VLAN_SUPPORT == ENABLED)
   //Get the VLAN identifier assigned to the interface
   vlanId = nicGetVlanId(interface);

   //Valid VLAN identifier?
   if(vlanId != 0)
   {
      //The VLAN tag is inserted in the Ethernet frame
      error = ethEncodeVlanTag(buffer, &offset, vlanId, ancillary->vlanPcp,
         ancillary->vlanDei, type);
      //Any error to report?
      if(error)
         return error;

      //A distinct EtherType has been allocated for use in the TPID field
      type = ETH_TYPE_VLAN;
   }
#endif

#if (ETH_VMAN_SUPPORT == ENABLED)
   //Get the VMAN identifier assigned to the interface
   vmanId = nicGetVmanId(interface);

   //Valid VMAN identifier?
   if(vmanId != 0)
   {
      //The VMAN tag is inserted in the Ethernet frame
      error = ethEncodeVlanTag(buffer, &offset, vmanId, ancillary->vmanPcp,
         ancillary->vmanDei, type);
      //Any error to report?
      if(error)
         return error;

      //A distinct EtherType has been allocated for use in the TPID field
      type = ETH_TYPE_VMAN;
   }
#endif

   //If the source address is not specified, then use the MAC address of the
   //interface as source address
   if(macCompAddr(&ancillary->srcMacAddr, &MAC_UNSPECIFIED_ADDR))
   {
      NetInterface *logicalInterface;

      //Point to the logical interface
      logicalInterface = nicGetLogicalInterface(interface);
      //Get the MAC address of the interface
      ancillary->srcMacAddr = logicalInterface->macAddr;
   }

   //Sanity check
   if(offset < sizeof(EthHeader))
      return ERROR_INVALID_PARAMETER;

   //Make room for the Ethernet header
   offset -= sizeof(EthHeader);
   //Calculate the length of the frame
   length = netBufferGetLength(buffer) - offset;

   //Point to the beginning of the frame
   header = netBufferAt(buffer, offset);

   //Format Ethernet header
   header->destAddr = *destAddr;
   header->srcAddr = ancillary->srcMacAddr;
   header->type = htons(type);

   //Update Ethernet statistics
   ethUpdateOutStats(interface, &header->destAddr, length);

   //Debug message
   TRACE_DEBUG("Sending Ethernet frame (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump Ethernet header contents for debugging purpose
   ethDumpHeader(header);

#if (ETH_VLAN_SUPPORT == ENABLED)
   //Dump VLAN identifier
   if(vlanId != 0)
   {
      TRACE_DEBUG("  VLAN Id = %" PRIu16 "\r\n", vlanId);
   }
#endif
#if (ETH_VMAN_SUPPORT == ENABLED)
   //Dump VMAN identifier
   if(vmanId != 0)
   {
      TRACE_DEBUG("  VMAN Id = %" PRIu16 "\r\n", vmanId);
   }
#endif
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Dump switch port identifier
   if(ancillary->port != 0)
   {
      TRACE_DEBUG("  Switch Port = %" PRIu8 "\r\n", ancillary->port);
   }
#endif

   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Check whether port tagging is supported by the switch
   if(physicalInterface->switchDriver != NULL &&
      physicalInterface->switchDriver->tagFrame != NULL)
   {
      //Add VLAN tag (SMSC switches) or tail tag (Micrel switches)
      error = physicalInterface->switchDriver->tagFrame(physicalInterface,
         buffer, &offset, ancillary);
      //Any error to report?
      if(error)
         return error;

      //Recalculate the length of the frame
      length = netBufferGetLength(buffer) - offset;
   }
#endif

   //Valid NIC driver?
   if(physicalInterface->nicDriver != NULL)
   {
      //Automatic padding not supported by hardware?
      if(!physicalInterface->nicDriver->autoPadding)
      {
         //The host controller should manually add padding to the packet before
         //transmitting it
         error = ethPadFrame(buffer, &length);
         //Any error to report?
         if(error)
            return error;
      }

      //CRC calculation not supported by hardware?
      if(!physicalInterface->nicDriver->autoCrcCalc)
      {
         //Compute CRC over the header and payload
         crc = ethCalcCrcEx(buffer, offset, length);
         //Convert from host byte order to little-endian byte order
         crc = htole32(crc);

         //Append the calculated CRC value
         error = netBufferAppend(buffer, &crc, sizeof(crc));
         //Any error to report?
         if(error)
            return error;

         //Adjust the length of the frame
         length += sizeof(crc);
      }
   }

   //Forward the frame to the physical interface
   error = nicSendPacket(physicalInterface, buffer, offset, ancillary);

   //Return status code
   return error;
}


/**
 * @brief Add a unicast/multicast address to the MAC filter table
 * @param[in] interface Underlying network interface
 * @param[in] macAddr MAC address to accept
 * @return Error code
 **/

error_t ethAcceptMacAddr(NetInterface *interface, const MacAddr *macAddr)
{
   uint_t i;
   MacFilterEntry *entry;
   MacFilterEntry *firstFreeEntry;

   //Keep track of the first free entry
   firstFreeEntry = NULL;

   //Go through the MAC filter table
   for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->macAddrFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Check whether the table already contains the specified MAC address
         if(macCompAddr(&entry->addr, macAddr))
         {
            //Increment the reference count
            entry->refCount++;
            //No error to report
            return NO_ERROR;
         }
      }
      else
      {
         //Keep track of the first free entry
         if(firstFreeEntry == NULL)
            firstFreeEntry = entry;
      }
   }

   //Check whether the multicast filter table is full
   if(firstFreeEntry == NULL)
   {
      //A new entry cannot be added
      return ERROR_FAILURE;
   }

   //Add a new entry to the table
   firstFreeEntry->addr = *macAddr;
   //Initialize the reference count
   firstFreeEntry->refCount = 1;

   //Force the network interface controller to add the current
   //entry to its MAC filter table
   firstFreeEntry->addFlag = TRUE;
   firstFreeEntry->deleteFlag = FALSE;

   //Update the MAC filter table
   nicUpdateMacAddrFilter(interface);

   //Clear the flag
   firstFreeEntry->addFlag = FALSE;

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Remove a unicast/multicast address from the MAC filter table
 * @param[in] interface Underlying network interface
 * @param[in] macAddr MAC address to drop
 * @return Error code
 **/

error_t ethDropMacAddr(NetInterface *interface, const MacAddr *macAddr)
{
   uint_t i;
   MacFilterEntry *entry;

   //Go through the MAC filter table
   for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->macAddrFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Specified MAC address found?
         if(macCompAddr(&entry->addr, macAddr))
         {
            //Decrement the reference count
            entry->refCount--;

            //Remove the entry if the reference count drops to zero
            if(entry->refCount == 0)
            {
               //Force the network interface controller to remove the current
               //entry from its MAC filter table
               entry->deleteFlag = TRUE;

               //Update the MAC filter table
               nicUpdateMacAddrFilter(interface);

               //Clear the flag
               entry->deleteFlag = FALSE;
               //Remove the multicast address from the list
               entry->addr = MAC_UNSPECIFIED_ADDR;
            }

            //No error to report
            return NO_ERROR;
         }
      }
   }

   //The specified MAC address does not exist
   return ERROR_ADDRESS_NOT_FOUND;
}


/**
 * @brief Register LLC frame received callback
 * @param[in] interface Underlying network interface
 * @param[in] callback Callback function to be called when a LLC frame is received
 * @param[in] param Callback function parameter (optional)
 * @return Error code
 **/

error_t ethAttachLlcRxCalback(NetInterface *interface, LlcRxCallback callback,
   void *param)
{
#if (ETH_LLC_SUPPORT == ENABLED)
   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Register LLC frame received callback
   interface->llcRxCallback = callback;
   //This opaque pointer will be directly passed to the callback function
   interface->llcRxParam = param;

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Unregister LLC frame received callback
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ethDetachLlcRxCalback(NetInterface *interface)
{
#if (ETH_LLC_SUPPORT == ENABLED)
   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Unregister LLC frame received callback
   interface->llcRxCallback = NULL;
   interface->llcRxParam = 0;

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Allocate a buffer to hold an Ethernet frame
 * @param[in] length Desired payload length
 * @param[out] offset Offset to the first byte of the payload
 * @return The function returns a pointer to the newly allocated
 *   buffer. If the system is out of resources, NULL is returned
 **/

NetBuffer *ethAllocBuffer(size_t length, size_t *offset)
{
   size_t n;
   NetBuffer *buffer;

   //Ethernet frame overhead
   n = sizeof(EthHeader);

#if (ETH_VLAN_SUPPORT == ENABLED)
   //VLAN tagging overhead (802.1Q)
   n += sizeof(VlanTag);
#endif

#if (ETH_VMAN_SUPPORT == ENABLED)
   //VMAN tagging overhead (802.1ad)
   n += sizeof(VlanTag);
#endif

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Special VLAN tagging overhead
   n += sizeof(VlanTag);
#endif

   //Allocate a buffer to hold the Ethernet header and the payload
   buffer = netBufferAlloc(length + n);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return NULL;

   //Offset to the first byte of the payload
   *offset = n;

   //Return a pointer to the freshly allocated buffer
   return buffer;
}


/**
 * @brief Convert a string representation of a MAC address to a binary MAC address
 * @param[in] str NULL-terminated string representing the MAC address
 * @param[out] macAddr Binary representation of the MAC address
 * @return Error code
 **/

error_t macStringToAddr(const char_t *str, MacAddr *macAddr)
{
   error_t error;
   int_t i = 0;
   int_t value = -1;

   //Parse input string
   while(1)
   {
      //Hexadecimal digit found?
      if(isxdigit((uint8_t) *str))
      {
         //First digit to be decoded?
         if(value < 0)
            value = 0;

         //Update the value of the current byte
         if(osIsdigit(*str))
         {
            value = (value * 16) + (*str - '0');
         }
         else if(osIsupper(*str))
         {
            value = (value * 16) + (*str - 'A' + 10);
         }
         else
         {
            value = (value * 16) + (*str - 'a' + 10);
         }

         //Check resulting value
         if(value > 0xFF)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }
      }
      //Dash or colon separator found?
      else if((*str == '-' || *str == ':') && i < 6)
      {
         //Each separator must be preceded by a valid number
         if(value < 0)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }

         //Save the current byte
         macAddr->b[i++] = value;
         //Prepare to decode the next byte
         value = -1;
      }
      //End of string detected?
      else if(*str == '\0' && i == 5)
      {
         //The NULL character must be preceded by a valid number
         if(value < 0)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
         }
         else
         {
            //Save the last byte of the MAC address
            macAddr->b[i] = value;
            //The conversion succeeded
            error = NO_ERROR;
         }

         //We are done
         break;
      }
      //Invalid character...
      else
      {
         //The conversion failed
         error = ERROR_INVALID_SYNTAX;
         break;
      }

      //Point to the next character
      str++;
   }

   //Return status code
   return error;
}


/**
 * @brief Convert a MAC address to a dash delimited string
 * @param[in] macAddr Pointer to the MAC address
 * @param[out] str NULL-terminated string representing the MAC address
 * @return Pointer to the formatted string
 **/

char_t *macAddrToString(const MacAddr *macAddr, char_t *str)
{
   static char_t buffer[18];

   //The str parameter is optional
   if(str == NULL)
      str = buffer;

   //Format MAC address
   osSprintf(str, "%02" PRIX8 "-%02" PRIX8 "-%02" PRIX8 "-%02" PRIX8 "-%02" PRIX8 "-%02" PRIX8,
      macAddr->b[0], macAddr->b[1], macAddr->b[2], macAddr->b[3], macAddr->b[4], macAddr->b[5]);

   //Return a pointer to the formatted string
   return str;
}


/**
 * @brief Map a MAC address to the IPv6 modified EUI-64 identifier
 * @param[in] macAddr Host MAC address
 * @param[out] interfaceId IPv6 modified EUI-64 identifier
 **/

void macAddrToEui64(const MacAddr *macAddr, Eui64 *interfaceId)
{
   //Copy the Organization Unique Identifier (OUI)
   interfaceId->b[0] = macAddr->b[0];
   interfaceId->b[1] = macAddr->b[1];
   interfaceId->b[2] = macAddr->b[2];

   //The middle 16 bits are given the value 0xFFFE
   interfaceId->b[3] = 0xFF;
   interfaceId->b[4] = 0xFE;

   //Copy the right-most 24 bits of the MAC address
   interfaceId->b[5] = macAddr->b[3];
   interfaceId->b[6] = macAddr->b[4];
   interfaceId->b[7] = macAddr->b[5];

   //Modified EUI-64 format interface identifiers are
   //formed by inverting the Universal/Local bit
   interfaceId->b[0] ^= MAC_ADDR_FLAG_LOCAL;
}


/**
 * @brief Dump Ethernet header for debugging purpose
 * @param[in] ethHeader Pointer to the Ethernet header
 **/

void ethDumpHeader(const EthHeader *ethHeader)
{
   //Dump Ethernet header contents
   TRACE_DEBUG("  Dest Addr = %s\r\n", macAddrToString(&ethHeader->destAddr, NULL));
   TRACE_DEBUG("  Src Addr = %s\r\n", macAddrToString(&ethHeader->srcAddr, NULL));
   TRACE_DEBUG("  Type = 0x%04" PRIX16 "\r\n", ntohs(ethHeader->type));
}

#endif
#if (ETH_SUPPORT == ENABLED || IPV6_SUPPORT == ENABLED)

//Unspecified EUI-64 address
const Eui64 EUI64_UNSPECIFIED_ADDR = {{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}};


/**
 * @brief Convert a string representation of an EUI-64 address to a binary EUI-64 address
 * @param[in] str NULL-terminated string representing the EUI-64 address
 * @param[out] eui64 Binary representation of the EUI-64 address
 * @return Error code
 **/

error_t eui64StringToAddr(const char_t *str, Eui64 *eui64)
{
   error_t error;
   int_t i = 0;
   int_t value = -1;

   //Parse input string
   while(1)
   {
      //Hexadecimal digit found?
      if(isxdigit((uint8_t) *str))
      {
         //First digit to be decoded?
         if(value < 0)
            value = 0;

         //Update the value of the current byte
         if(osIsdigit(*str))
         {
            value = (value * 16) + (*str - '0');
         }
         else if(osIsupper(*str))
         {
            value = (value * 16) + (*str - 'A' + 10);
         }
         else
         {
            value = (value * 16) + (*str - 'a' + 10);
         }

         //Check resulting value
         if(value > 0xFF)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }
      }
      //Dash or colon separator found?
      else if((*str == '-' || *str == ':') && i < 8)
      {
         //Each separator must be preceded by a valid number
         if(value < 0)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }

         //Save the current byte
         eui64->b[i++] = value;
         //Prepare to decode the next byte
         value = -1;
      }
      //End of string detected?
      else if(*str == '\0' && i == 7)
      {
         //The NULL character must be preceded by a valid number
         if(value < 0)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
         }
         else
         {
            //Save the last byte of the EUI-64 address
            eui64->b[i] = value;
            //The conversion succeeded
            error = NO_ERROR;
         }

         //We are done
         break;
      }
      //Invalid character...
      else
      {
         //The conversion failed
         error = ERROR_INVALID_SYNTAX;
         break;
      }

      //Point to the next character
      str++;
   }

   //Return status code
   return error;
}


/**
 * @brief Convert an EUI-64 address to a dash delimited string
 * @param[in] eui64 Pointer to the EUI-64 address
 * @param[out] str NULL-terminated string representing the EUI-64 address
 * @return Pointer to the formatted string
 **/

char_t *eui64AddrToString(const Eui64 *eui64, char_t *str)
{
   static char_t buffer[24];

   //The str parameter is optional
   if(str == NULL)
      str = buffer;

   //Format EUI-64 identifier
   osSprintf(str, "%02" PRIX8 "-%02" PRIX8 "-%02" PRIX8 "-%02" PRIX8
      "-%02" PRIX8 "-%02" PRIX8 "-%02" PRIX8 "-%02" PRIX8,
      eui64->b[0], eui64->b[1], eui64->b[2], eui64->b[3],
      eui64->b[4], eui64->b[5], eui64->b[6], eui64->b[7]);

   //Return a pointer to the formatted string
   return str;
}

#endif
