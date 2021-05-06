/**
 * @file ipv4.c
 * @brief IPv4 (Internet Protocol Version 4)
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
 * The Internet Protocol (IP) provides the functions necessary to deliver a
 * datagram from a source to a destination over an interconnected system of
 * networks. Refer to RFC 791 for complete details
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL IPV4_TRACE_LEVEL

//Dependencies
#include <string.h>
#include <ctype.h>
#include "core/net.h"
#include "core/ethernet.h"
#include "core/ip.h"
#include "core/udp.h"
#include "core/tcp_fsm.h"
#include "core/raw_socket.h"
#include "ipv4/arp.h"
#include "ipv4/ipv4.h"
#include "ipv4/ipv4_misc.h"
#include "ipv4/ipv4_routing.h"
#include "ipv4/icmp.h"
#include "ipv4/igmp.h"
#include "ipv4/auto_ip.h"
#include "dhcp/dhcp_client.h"
#include "mdns/mdns_responder.h"
#include "mibs/mib2_module.h"
#include "mibs/ip_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV4_SUPPORT == ENABLED)


/**
 * @brief IPv4 related initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ipv4Init(NetInterface *interface)
{
   Ipv4Context *context;
   NetInterface *physicalInterface;

   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);

   //Point to the IPv4 context
   context = &interface->ipv4Context;

   //Clear the IPv4 context
   osMemset(context, 0, sizeof(Ipv4Context));

   //Initialize interface specific variables
   context->linkMtu = physicalInterface->nicDriver->mtu;
   context->isRouter = FALSE;

   //Broadcast ICMP Echo Request messages are allowed by default
   context->enableBroadcastEchoReq = TRUE;

   //Identification field is primarily used to identify
   //fragments of an original IP datagram
   context->identification = 0;

   //Initialize the list of DNS servers
   osMemset(context->dnsServerList, 0, sizeof(context->dnsServerList));
   //Initialize the multicast filter table
   osMemset(context->multicastFilter, 0, sizeof(context->multicastFilter));

#if (IPV4_FRAG_SUPPORT == ENABLED)
   //Initialize the reassembly queue
   osMemset(context->fragQueue, 0, sizeof(context->fragQueue));
#endif

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Assign host address
 * @param[in] interface Pointer to the desired network interface
 * @param[in] addr IPv4 host address
 * @return Error code
 **/

error_t ipv4SetHostAddr(NetInterface *interface, Ipv4Addr addr)
{
   //Set IPv4 host address
   return ipv4SetHostAddrEx(interface, 0, addr);
}


/**
 * @brief Assign host address
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[in] addr IPv4 host address
 * @return Error code
 **/

error_t ipv4SetHostAddrEx(NetInterface *interface, uint_t index, Ipv4Addr addr)
{
   Ipv4AddrEntry *entry;

   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV4_ADDR_LIST_SIZE)
      return ERROR_OUT_OF_RANGE;

   //The IPv4 address must be a valid unicast address
   if(ipv4IsMulticastAddr(addr))
      return ERROR_INVALID_ADDRESS;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the corresponding entry
   entry = &interface->ipv4Context.addrList[index];

   //Set up host address
   entry->addr = addr;
   //Clear conflict flag
   entry->conflict = FALSE;

   //Check whether the new host address is valid
   if(addr != IPV4_UNSPECIFIED_ADDR)
   {
      //The use of the IPv4 address is now unrestricted
      entry->state = IPV4_ADDR_STATE_VALID;
   }
   else
   {
      //The IPv4 address is no longer valid
      entry->state = IPV4_ADDR_STATE_INVALID;
   }

#if (MDNS_RESPONDER_SUPPORT == ENABLED)
   //Restart mDNS probing process
   mdnsResponderStartProbing(interface->mdnsResponderContext);
#endif

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve host address
 * @param[in] interface Pointer to the desired network interface
 * @param[out] addr IPv4 host address
 * @return Error code
 **/

error_t ipv4GetHostAddr(NetInterface *interface, Ipv4Addr *addr)
{
   //Get IPv4 host address
   return ipv4GetHostAddrEx(interface, 0, addr);
}


/**
 * @brief Retrieve host address
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[out] addr IPv4 host address
 * @return Error code
 **/

error_t ipv4GetHostAddrEx(NetInterface *interface, uint_t index, Ipv4Addr *addr)
{
   Ipv4AddrEntry *entry;

   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV4_ADDR_LIST_SIZE)
   {
      //Return the unspecified address when the index is out of range
      *addr = IPV4_UNSPECIFIED_ADDR;
      //Report an error
      return ERROR_OUT_OF_RANGE;
   }

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the corresponding entry
   entry = &interface->ipv4Context.addrList[index];

   //Check whether the host address is valid
   if(entry->state == IPV4_ADDR_STATE_VALID)
   {
      //Get IPv4 address
      *addr = entry->addr;
   }
   else
   {
      //Return the unspecified address when no address has been assigned
      *addr = IPV4_UNSPECIFIED_ADDR;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Configure subnet mask
 * @param[in] interface Pointer to the desired network interface
 * @param[in] mask Subnet mask
 * @return Error code
 **/

error_t ipv4SetSubnetMask(NetInterface *interface, Ipv4Addr mask)
{
   //Set subnet mask
   return ipv4SetSubnetMaskEx(interface, 0, mask);
}


/**
 * @brief Configure subnet mask
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[in] mask Subnet mask
 * @return Error code
 **/

error_t ipv4SetSubnetMaskEx(NetInterface *interface, uint_t index, Ipv4Addr mask)
{
   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV4_ADDR_LIST_SIZE)
      return ERROR_OUT_OF_RANGE;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set up subnet mask
   interface->ipv4Context.addrList[index].subnetMask = mask;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve subnet mask
 * @param[in] interface Pointer to the desired network interface
 * @param[out] mask Subnet mask
 * @return Error code
 **/

error_t ipv4GetSubnetMask(NetInterface *interface, Ipv4Addr *mask)
{
   //Get subnet mask
   return ipv4GetSubnetMaskEx(interface, 0, mask);
}


/**
 * @brief Retrieve subnet mask
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[out] mask Subnet mask
 * @return Error code
 **/

error_t ipv4GetSubnetMaskEx(NetInterface *interface, uint_t index, Ipv4Addr *mask)
{
   //Check parameters
   if(interface == NULL || mask == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV4_ADDR_LIST_SIZE)
   {
      //Return the default mask when the index is out of range
      *mask = IPV4_UNSPECIFIED_ADDR;
      //Report an error
      return ERROR_OUT_OF_RANGE;
   }

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Get subnet mask
   *mask = interface->ipv4Context.addrList[index].subnetMask;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Configure default gateway
 * @param[in] interface Pointer to the desired network interface
 * @param[in] addr Default gateway address
 * @return Error code
 **/

error_t ipv4SetDefaultGateway(NetInterface *interface, Ipv4Addr addr)
{
   //Set default gateway
   return ipv4SetDefaultGatewayEx(interface, 0, addr);
}


/**
 * @brief Configure default gateway
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[in] addr Default gateway address
 * @return Error code
 **/

error_t ipv4SetDefaultGatewayEx(NetInterface *interface, uint_t index,
   Ipv4Addr addr)
{
   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV4_ADDR_LIST_SIZE)
      return ERROR_OUT_OF_RANGE;

   //The IPv4 address must be a valid unicast address
   if(ipv4IsMulticastAddr(addr))
      return ERROR_INVALID_ADDRESS;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set up default gateway address
   interface->ipv4Context.addrList[index].defaultGateway = addr;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve default gateway
 * @param[in] interface Pointer to the desired network interface
 * @param[out] addr Default gateway address
 * @return Error code
 **/

error_t ipv4GetDefaultGateway(NetInterface *interface, Ipv4Addr *addr)
{
   //Get default gateway
   return ipv4GetDefaultGatewayEx(interface, 0, addr);
}


/**
 * @brief Retrieve default gateway
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index Zero-based index
 * @param[out] addr Default gateway address
 * @return Error code
 **/

error_t ipv4GetDefaultGatewayEx(NetInterface *interface, uint_t index,
   Ipv4Addr *addr)
{
   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV4_ADDR_LIST_SIZE)
   {
      //Return the unspecified address when the index is out of range
      *addr = IPV4_UNSPECIFIED_ADDR;
      //Report an error
      return ERROR_OUT_OF_RANGE;
   }

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Get default gateway address
   *addr = interface->ipv4Context.addrList[index].defaultGateway;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Configure DNS server
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index This parameter selects between the primary and secondary DNS server
 * @param[in] addr DNS server address
 * @return Error code
 **/

error_t ipv4SetDnsServer(NetInterface *interface, uint_t index, Ipv4Addr addr)
{
   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV4_DNS_SERVER_LIST_SIZE)
      return ERROR_OUT_OF_RANGE;

   //The IPv4 address must be a valid unicast address
   if(ipv4IsMulticastAddr(addr))
      return ERROR_INVALID_ADDRESS;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set up DNS server address
   interface->ipv4Context.dnsServerList[index] = addr;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve DNS server
 * @param[in] interface Pointer to the desired network interface
 * @param[in] index This parameter selects between the primary and secondary DNS server
 * @param[out] addr DNS server address
 * @return Error code
 **/

error_t ipv4GetDnsServer(NetInterface *interface, uint_t index, Ipv4Addr *addr)
{
   //Check parameters
   if(interface == NULL || addr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure that the index is valid
   if(index >= IPV4_DNS_SERVER_LIST_SIZE)
   {
      //Return the unspecified address when the index is out of range
      *addr = IPV4_UNSPECIFIED_ADDR;
      //Report an error
      return ERROR_OUT_OF_RANGE;
   }

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Get DNS server address
   *addr = interface->ipv4Context.dnsServerList[index];
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Callback function for link change event
 * @param[in] interface Underlying network interface
 **/

void ipv4LinkChangeEvent(NetInterface *interface)
{
   Ipv4Context *context;
   NetInterface *physicalInterface;

   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);

   //Point to the IPv4 context
   context = &interface->ipv4Context;

   //Restore default MTU
   context->linkMtu = physicalInterface->nicDriver->mtu;

#if (ETH_SUPPORT == ENABLED)
   //Flush ARP cache contents
   arpFlushCache(interface);
#endif

#if (IPV4_FRAG_SUPPORT == ENABLED)
   //Flush the reassembly queue
   ipv4FlushFragQueue(interface);
#endif

#if (IGMP_SUPPORT == ENABLED)
   //Notify IGMP of link state changes
   igmpLinkChangeEvent(interface);
#endif

#if (AUTO_IP_SUPPORT == ENABLED)
   //Notify Auto-IP of link state changes
   autoIpLinkChangeEvent(interface->autoIpContext);
#endif

#if (DHCP_CLIENT_SUPPORT == ENABLED)
   //Notify the DHCP client of link state changes
   dhcpClientLinkChangeEvent(interface->dhcpClientContext);
#endif
}


/**
 * @brief Incoming IPv4 packet processing
 * @param[in] interface Underlying network interface
 * @param[in] packet Incoming IPv4 packet
 * @param[in] length Packet length including header and payload
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 **/

void ipv4ProcessPacket(NetInterface *interface, Ipv4Header *packet,
   size_t length, NetRxAncillary *ancillary)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

   //Total number of input datagrams received, including those received in error
   MIB2_INC_COUNTER32(ipGroup.ipInReceives, 1);
   IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsInReceives, 1);
   IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCInReceives, 1);
   IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsInReceives, 1);
   IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCInReceives, 1);

   //Total number of octets received in input IP datagrams
   IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsInOctets, length);
   IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCInOctets, length);
   IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsInOctets, length);
   IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCInOctets, length);

   //Start of exception handling block
   do
   {
      //Ensure the packet length is greater than 20 bytes
      if(length < sizeof(Ipv4Header))
      {
         //Discard the received packet
         error = ERROR_INVALID_LENGTH;
         break;
      }

      //Debug message
      TRACE_INFO("IPv4 packet received (%" PRIuSIZE " bytes)...\r\n", length);
      //Dump IP header contents for debugging purpose
      ipv4DumpHeader(packet);

      //A packet whose version number is not 4 must be silently discarded
      if(packet->version != IPV4_VERSION)
      {
         //Discard the received packet
         error = ERROR_INVALID_HEADER;
         break;
      }

      //Valid IPv4 header shall contains more than five 32-bit words
      if(packet->headerLength < 5)
      {
         //Discard the received packet
         error = ERROR_INVALID_HEADER;
         break;
      }

      //Ensure the total length is correct before processing the packet
      if(ntohs(packet->totalLength) < (packet->headerLength * 4))
      {
         //Discard the received packet
         error = ERROR_INVALID_HEADER;
         break;
      }

      //Truncated packet?
      if(length < ntohs(packet->totalLength))
      {
         //Discard the received packet
         error = ERROR_INVALID_LENGTH;
         break;
      }

      //Source address filtering
      if(ipv4CheckSourceAddr(interface, packet->srcAddr))
      {
         //Discard the received packet
         error = ERROR_INVALID_HEADER;
         break;
      }

#if defined(IPV4_PACKET_FORWARD_HOOK)
      IPV4_PACKET_FORWARD_HOOK(interface, packet, length);
#else
      //Destination address filtering
      if(ipv4CheckDestAddr(interface, packet->destAddr))
      {
#if (IPV4_ROUTING_SUPPORT == ENABLED)
         NetBuffer1 buffer;

         //Unfragmented datagrams fit in a single chunk
         buffer.chunkCount = 1;
         buffer.maxChunkCount = 1;
         buffer.chunk[0].address = packet;
         buffer.chunk[0].length = length;

         //Forward the packet according to the routing table
         ipv4ForwardPacket(interface, (NetBuffer *) &buffer, 0);
#else
         //Discard the received packet
         error = ERROR_INVALID_ADDRESS;
#endif
         //We are done
         break;
      }
#endif

      //Packets addressed to a tentative address should be silently discarded
      if(ipv4IsTentativeAddr(interface, packet->destAddr))
      {
         //Discard the received packet
         error = ERROR_INVALID_ADDRESS;
         break;
      }

      //The host must verify the IP header checksum on every received datagram
      //and silently discard every datagram that has a bad checksum (refer to
      //RFC 1122, section 3.2.1.2)
      if(ipCalcChecksum(packet, packet->headerLength * 4) != 0x0000)
      {
         //Debug message
         TRACE_WARNING("Wrong IP header checksum!\r\n");

         //Discard incoming packet
         error = ERROR_INVALID_HEADER;
         break;
      }

      //Update IP statistics
      ipv4UpdateInStats(interface, packet->destAddr, length);

      //Convert the total length from network byte order
      length = ntohs(packet->totalLength);

      //A fragmented packet was received?
      if(ntohs(packet->fragmentOffset) & (IPV4_FLAG_MF | IPV4_OFFSET_MASK))
      {
#if (IPV4_FRAG_SUPPORT == ENABLED)
         //Reassemble the original datagram
         ipv4ReassembleDatagram(interface, packet, length, ancillary);
#endif
      }
      else
      {
         NetBuffer1 buffer;

         //Unfragmented datagrams fit in a single chunk
         buffer.chunkCount = 1;
         buffer.maxChunkCount = 1;
         buffer.chunk[0].address = packet;
         buffer.chunk[0].length = (uint16_t) length;

         //Pass the IPv4 datagram to the higher protocol layer
         ipv4ProcessDatagram(interface, (NetBuffer *) &buffer, ancillary);
      }

      //End of exception handling block
   } while(0);

   //Invalid IPv4 packet received?
   if(error)
   {
      //Update IP statistics
      ipv4UpdateErrorStats(interface, error);
   }
}


/**
 * @brief Incoming IPv4 datagram processing
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer that holds the incoming IPv4 datagram
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 **/

void ipv4ProcessDatagram(NetInterface *interface, const NetBuffer *buffer,
   NetRxAncillary *ancillary)
{
   error_t error;
   size_t offset;
   size_t length;
   Ipv4Header *header;
   IpPseudoHeader pseudoHeader;

   //Retrieve the length of the IPv4 datagram
   length = netBufferGetLength(buffer);

   //Point to the IPv4 header
   header = netBufferAt(buffer, 0);
   //Sanity check
   if(header == NULL)
      return;

   //Debug message
   TRACE_INFO("IPv4 datagram received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump IP header contents for debugging purpose
   ipv4DumpHeader(header);

   //Get the offset to the payload
   offset = header->headerLength * 4;
   //Compute the length of the payload
   length -= header->headerLength * 4;

   //Form the IPv4 pseudo header
   pseudoHeader.length = sizeof(Ipv4PseudoHeader);
   pseudoHeader.ipv4Data.srcAddr = header->srcAddr;
   pseudoHeader.ipv4Data.destAddr = header->destAddr;
   pseudoHeader.ipv4Data.reserved = 0;
   pseudoHeader.ipv4Data.protocol = header->protocol;
   pseudoHeader.ipv4Data.length = htons(length);

   //Save TTL value
   ancillary->ttl = header->timeToLive;

#if defined(IPV4_DATAGRAM_FORWARD_HOOK)
   IPV4_DATAGRAM_FORWARD_HOOK(interface, &pseudoHeader, buffer, offset);
#endif

   //Check the protocol field
   switch(header->protocol)
   {
   //ICMP protocol?
   case IPV4_PROTOCOL_ICMP:
      //Process incoming ICMP message
      icmpProcessMessage(interface, &pseudoHeader.ipv4Data, buffer, offset);

#if (RAW_SOCKET_SUPPORT == ENABLED)
      //Allow raw sockets to process ICMP messages
      rawSocketProcessIpPacket(interface, &pseudoHeader, buffer, offset,
         ancillary);
#endif

      //No error to report
      error = NO_ERROR;
      //Continue processing
      break;

#if (IGMP_SUPPORT == ENABLED)
   //IGMP protocol?
   case IPV4_PROTOCOL_IGMP:
      //Process incoming IGMP message
      igmpProcessMessage(interface, &pseudoHeader.ipv4Data, buffer, offset);

#if (RAW_SOCKET_SUPPORT == ENABLED)
      //Allow raw sockets to process IGMP messages
      rawSocketProcessIpPacket(interface, &pseudoHeader, buffer, offset,
         ancillary);
#endif

      //No error to report
      error = NO_ERROR;
      //Continue processing
      break;
#endif

#if (TCP_SUPPORT == ENABLED)
   //TCP protocol?
   case IPV4_PROTOCOL_TCP:
      //Process incoming TCP segment
      tcpProcessSegment(interface, &pseudoHeader, buffer, offset, ancillary);
      //No error to report
      error = NO_ERROR;
      //Continue processing
      break;
#endif

#if (UDP_SUPPORT == ENABLED)
   //UDP protocol?
   case IPV4_PROTOCOL_UDP:
      //Process incoming UDP datagram
      error = udpProcessDatagram(interface, &pseudoHeader, buffer, offset,
         ancillary);
      //Continue processing
      break;
#endif

   //Unknown protocol?
   default:
#if (RAW_SOCKET_SUPPORT == ENABLED)
      //Allow raw sockets to process IPv4 packets
      error = rawSocketProcessIpPacket(interface, &pseudoHeader, buffer, offset,
         ancillary);
#else
      //Report an error
      error = ERROR_PROTOCOL_UNREACHABLE;
#endif
      //Continue processing
      break;
   }

   //Unreachable protocol?
   if(error == ERROR_PROTOCOL_UNREACHABLE)
   {
      //Update IP statistics
      ipv4UpdateErrorStats(interface, error);

      //Send a Destination Unreachable message
      icmpSendErrorMessage(interface, ICMP_TYPE_DEST_UNREACHABLE,
         ICMP_CODE_PROTOCOL_UNREACHABLE, 0, buffer, 0);
   }
   else
   {
      //Total number of input datagrams successfully delivered to IP
      //user-protocols
      MIB2_INC_COUNTER32(ipGroup.ipInDelivers, 1);
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsInDelivers, 1);
      IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCInDelivers, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsInDelivers, 1);
      IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCInDelivers, 1);
   }

   //Unreachable port?
   if(error == ERROR_PORT_UNREACHABLE)
   {
      //Send a Destination Unreachable message
      icmpSendErrorMessage(interface, ICMP_TYPE_DEST_UNREACHABLE,
         ICMP_CODE_PORT_UNREACHABLE, 0, buffer, 0);
   }
}


/**
 * @brief Send an IPv4 datagram
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv4 pseudo header
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in] offset Offset to the first byte of the payload
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ipv4SendDatagram(NetInterface *interface, Ipv4PseudoHeader *pseudoHeader,
   NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   error_t error;
   size_t length;
   uint16_t id;

   //Total number of IP datagrams which local IP user-protocols supplied to IP
   //in requests for transmission
   MIB2_INC_COUNTER32(ipGroup.ipOutRequests, 1);
   IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsOutRequests, 1);
   IP_MIB_INC_COUNTER64(ipv4SystemStats.ipSystemStatsHCOutRequests, 1);
   IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsOutRequests, 1);
   IP_MIB_INC_COUNTER64(ipv4IfStatsTable[interface->index].ipIfStatsHCOutRequests, 1);

   //Retrieve the length of payload
   length = netBufferGetLength(buffer) - offset;

   //Identification field is primarily used to identify fragments of an
   //original IP datagram
   id = interface->ipv4Context.identification++;

   //If the payload length is smaller than the network interface MTU then no
   //fragmentation is needed
   if((length + sizeof(Ipv4Header)) <= interface->ipv4Context.linkMtu)
   {
      //Send data as is
      error = ipv4SendPacket(interface, pseudoHeader, id, 0, buffer, offset,
         ancillary);
   }
   //If the payload length exceeds the network interface MTU then the device
   //must fragment the data
   else
   {
#if (IPV4_FRAG_SUPPORT == ENABLED)
      //Fragment IP datagram into smaller packets
      error = ipv4FragmentDatagram(interface, pseudoHeader, id, buffer, offset,
         ancillary);
#else
      //Fragmentation is not supported
      error = ERROR_MESSAGE_TOO_LONG;
#endif
   }

   //Return status code
   return error;
}


/**
 * @brief Send an IPv4 packet
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv4 pseudo header
 * @param[in] fragId Fragment identification field
 * @param[in] fragOffset Fragment offset field
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in] offset Offset to the first byte of the payload
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ipv4SendPacket(NetInterface *interface, Ipv4PseudoHeader *pseudoHeader,
   uint16_t fragId, size_t fragOffset, NetBuffer *buffer, size_t offset,
   NetTxAncillary *ancillary)
{
   error_t error;
   size_t length;
   Ipv4Header *packet;
#if (ETH_SUPPORT == ENABLED)
   NetInterface *physicalInterface;
#endif

   //Check whether an IP Router Alert option should be added
   if(ancillary->routerAlert)
   {
      //Add an IP Router Alert option
      error = ipv4AddRouterAlertOption(buffer, &offset);
      //Any error to report?
      if(error)
         return error;
   }

   //Is there enough space for the IPv4 header?
   if(offset < sizeof(Ipv4Header))
      return ERROR_INVALID_PARAMETER;

   //Make room for the header
   offset -= sizeof(Ipv4Header);
   //Calculate the size of the entire packet, including header and data
   length = netBufferGetLength(buffer) - offset;

   //Point to the IPv4 header
   packet = netBufferAt(buffer, offset);

   //Format IPv4 header
   packet->version = IPV4_VERSION;
   packet->headerLength = 5;
   packet->typeOfService = 0;
   packet->totalLength = htons(length);
   packet->identification = htons(fragId);
   packet->fragmentOffset = htons(fragOffset);
   packet->timeToLive = ancillary->ttl;
   packet->protocol = pseudoHeader->protocol;
   packet->headerChecksum = 0;
   packet->srcAddr = pseudoHeader->srcAddr;
   packet->destAddr = pseudoHeader->destAddr;

   //The IHL field is the length of the internet header in 32-bit words, and
   //thus points to the beginning of the data. Note that the minimum value for
   //a correct header is 5 (refer to RFC 791, section 3.1)
   if(ancillary->routerAlert)
   {
      packet->headerLength = 6;
   }

   //Check whether the TTL value is zero
   if(packet->timeToLive == 0)
   {
      //Use default Time-To-Live value
      packet->timeToLive = IPV4_DEFAULT_TTL;
   }

#if (IP_DIFF_SERV_SUPPORT == ENABLED)
   //Set DSCP field
   packet->typeOfService = (ancillary->dscp << 2) & 0xFC;
#endif

   //Calculate IP header checksum
   packet->headerChecksum = ipCalcChecksumEx(buffer, offset,
      packet->headerLength * 4);

   //Ensure the source address is valid
   error = ipv4CheckSourceAddr(interface, pseudoHeader->srcAddr);
   //Invalid source address?
   if(error)
      return error;

   //Check destination address
   if(pseudoHeader->destAddr == IPV4_UNSPECIFIED_ADDR)
   {
      //The unspecified address must not appear on the public Internet
      error = ERROR_INVALID_ADDRESS;
   }
   else if(ipv4IsLocalHostAddr(pseudoHeader->destAddr))
   {
#if (NET_LOOPBACK_IF_SUPPORT == ENABLED)
      uint_t i;

      //Initialize status code
      error = ERROR_NO_ROUTE;

      //Loop through network interfaces
      for(i = 0; i < NET_INTERFACE_COUNT; i++)
      {
         //Point to the current interface
         interface = &netInterface[i];

         //Loopback interface?
         if(interface->nicDriver != NULL &&
            interface->nicDriver->type == NIC_TYPE_LOOPBACK)
         {
            //Forward the packet to the loopback interface
            error = nicSendPacket(interface, buffer, offset, ancillary);
            break;
         }
      }
#else
      //Addresses within the entire 127.0.0.0/8 block do not legitimately
      //appear on any network anywhere
      error = ERROR_NO_ROUTE;
#endif
   }
   else
   {
#if (ETH_SUPPORT == ENABLED)
      //Point to the physical interface
      physicalInterface = nicGetPhysicalInterface(interface);

      //Ethernet interface?
      if(physicalInterface->nicDriver != NULL &&
         physicalInterface->nicDriver->type == NIC_TYPE_ETHERNET)
      {
         Ipv4Addr destIpAddr;

         //Get the destination IPv4 address
         destIpAddr = pseudoHeader->destAddr;

         //Perform address resolution
         if(!macCompAddr(&ancillary->destMacAddr, &MAC_UNSPECIFIED_ADDR))
         {
            //The destination address is already resolved
            error = NO_ERROR;
         }
         else if(ipv4IsBroadcastAddr(interface, destIpAddr))
         {
            //Use of the broadcast MAC address to send the packet
            ancillary->destMacAddr = MAC_BROADCAST_ADDR;
            //Successful address resolution
            error = NO_ERROR;
         }
         else if(ipv4IsMulticastAddr(destIpAddr))
         {
            //Map IPv4 multicast address to MAC-layer multicast address
            error = ipv4MapMulticastAddrToMac(destIpAddr, &ancillary->destMacAddr);
         }
         else if(ipv4IsLinkLocalAddr(pseudoHeader->srcAddr) ||
            ipv4IsLinkLocalAddr(destIpAddr))
         {
            //Packets with a link-local source or destination address are not
            //routable off the link
            error = arpResolve(interface, destIpAddr, &ancillary->destMacAddr);
         }
         else if(ipv4IsOnLink(interface, destIpAddr))
         {
            //Resolve destination address before sending the packet
            error = arpResolve(interface, destIpAddr, &ancillary->destMacAddr);
         }
         else if(ancillary->dontRoute)
         {
            //Do not send the packet via a gateway
            error = arpResolve(interface, destIpAddr, &ancillary->destMacAddr);
         }
         else
         {
            //Default gateway selection
            error = ipv4SelectDefaultGateway(interface, pseudoHeader->srcAddr,
               &destIpAddr);

            //Check status code
            if(!error)
            {
               //Use the selected gateway to forward the packet
               error = arpResolve(interface, destIpAddr, &ancillary->destMacAddr);
            }
            else
            {
               //Number of IP datagrams discarded because no route could be found
               //to transmit them to their destination
               MIB2_INC_COUNTER32(ipGroup.ipOutNoRoutes, 1);
               IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsOutNoRoutes, 1);
            }
         }

         //Successful address resolution?
         if(error == NO_ERROR)
         {
            //Update IP statistics
            ipv4UpdateOutStats(interface, destIpAddr, length);

            //Debug message
            TRACE_INFO("Sending IPv4 packet (%" PRIuSIZE " bytes)...\r\n", length);
            //Dump IP header contents for debugging purpose
            ipv4DumpHeader(packet);

            //Send Ethernet frame
            error = ethSendFrame(interface, &ancillary->destMacAddr,
               ETH_TYPE_IPV4, buffer, offset, ancillary);
         }
         else if(error == ERROR_IN_PROGRESS)
         {
            //Debug message
            TRACE_INFO("Enqueuing IPv4 packet (%" PRIuSIZE " bytes)...\r\n", length);
            //Dump IP header contents for debugging purpose
            ipv4DumpHeader(packet);

            //Enqueue packets waiting for address resolution
            error = arpEnqueuePacket(interface, destIpAddr, buffer, offset,
               ancillary);
         }
         else
         {
            //Debug message
            TRACE_WARNING("Cannot map IPv4 address to Ethernet address!\r\n");
         }
      }
      else
#endif
#if (PPP_SUPPORT == ENABLED)
      //PPP interface?
      if(interface->nicDriver != NULL &&
         interface->nicDriver->type == NIC_TYPE_PPP)
      {
         //Update IP statistics
         ipv4UpdateOutStats(interface, pseudoHeader->destAddr, length);

         //Debug message
         TRACE_INFO("Sending IPv4 packet (%" PRIuSIZE " bytes)...\r\n", length);
         //Dump IP header contents for debugging purpose
         ipv4DumpHeader(packet);

         //Send PPP frame
         error = pppSendFrame(interface, buffer, offset, PPP_PROTOCOL_IP);
      }
      else
#endif
      //Unknown interface type?
      {
         //Report an error
         error = ERROR_INVALID_INTERFACE;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Join the specified host group
 * @param[in] interface Underlying network interface
 * @param[in] groupAddr IPv4 address identifying the host group to join
 * @return Error code
 **/

error_t ipv4JoinMulticastGroup(NetInterface *interface, Ipv4Addr groupAddr)
{
   error_t error;
   uint_t i;
   Ipv4FilterEntry *entry;
   Ipv4FilterEntry *firstFreeEntry;
#if (ETH_SUPPORT == ENABLED)
   NetInterface *physicalInterface;
   MacAddr macAddr;
#endif

   //The IPv4 address must be a valid multicast address
   if(!ipv4IsMulticastAddr(groupAddr))
      return ERROR_INVALID_ADDRESS;

#if (ETH_SUPPORT == ENABLED)
   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);
#endif

   //Initialize error code
   error = NO_ERROR;
   //Keep track of the first free entry
   firstFreeEntry = NULL;

   //Go through the multicast filter table
   for(i = 0; i < IPV4_MULTICAST_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv4Context.multicastFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Check whether the table already contains the specified IPv4 address
         if(entry->addr == groupAddr)
         {
            //Increment the reference count
            entry->refCount++;
            //Successful processing
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

#if (ETH_SUPPORT == ENABLED)
   //Map the IPv4 multicast address to a MAC-layer address
   ipv4MapMulticastAddrToMac(groupAddr, &macAddr);
   //Add the corresponding address to the MAC filter table
   error = ethAcceptMacAddr(interface, &macAddr);

   //Check status code
   if(!error)
   {
      //Virtual interface?
      if(interface != physicalInterface)
      {
         //Configure the physical interface to accept the MAC address
         error = ethAcceptMacAddr(physicalInterface, &macAddr);

         //Any error to report?
         if(error)
         {
            //Clean up side effects
            ethDropMacAddr(interface, &macAddr);
         }
      }
   }
#endif

   //MAC filter table successfully updated?
   if(!error)
   {
      //Now we can safely add a new entry to the table
      firstFreeEntry->addr = groupAddr;
      //Initialize the reference count
      firstFreeEntry->refCount = 1;

#if (IGMP_SUPPORT == ENABLED)
      //Report multicast group membership to the router
      igmpJoinGroup(interface, firstFreeEntry);
#endif
   }

   //Return status code
   return error;
}


/**
 * @brief Leave the specified host group
 * @param[in] interface Underlying network interface
 * @param[in] groupAddr IPv4 address identifying the host group to leave
 * @return Error code
 **/

error_t ipv4LeaveMulticastGroup(NetInterface *interface, Ipv4Addr groupAddr)
{
   uint_t i;
   Ipv4FilterEntry *entry;
#if (ETH_SUPPORT == ENABLED)
   NetInterface *physicalInterface;
   MacAddr macAddr;
#endif

   //The IPv4 address must be a valid multicast address
   if(!ipv4IsMulticastAddr(groupAddr))
      return ERROR_INVALID_ADDRESS;

#if (ETH_SUPPORT == ENABLED)
   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);
#endif

   //Go through the multicast filter table
   for(i = 0; i < IPV4_MULTICAST_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv4Context.multicastFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Specified IPv4 address found?
         if(entry->addr == groupAddr)
         {
            //Decrement the reference count
            entry->refCount--;

            //Remove the entry if the reference count drops to zero
            if(entry->refCount == 0)
            {
#if (IGMP_SUPPORT == ENABLED)
               //Report group membership termination
               igmpLeaveGroup(interface, entry);
#endif
#if (ETH_SUPPORT == ENABLED)
               //Map the IPv4 multicast address to a MAC-layer address
               ipv4MapMulticastAddrToMac(groupAddr, &macAddr);
               //Drop the corresponding address from the MAC filter table
               ethDropMacAddr(interface, &macAddr);

               //Virtual interface?
               if(interface != physicalInterface)
               {
                  //Drop the corresponding address from the MAC filter table of
                  //the physical interface
                  ethDropMacAddr(physicalInterface, &macAddr);
               }
#endif
               //Remove the multicast address from the list
               entry->addr = IPV4_UNSPECIFIED_ADDR;
            }

            //Successful processing
            return NO_ERROR;
         }
      }
   }

   //The specified IPv4 address does not exist
   return ERROR_ADDRESS_NOT_FOUND;
}


/**
 * @brief Convert a dot-decimal string to a binary IPv4 address
 * @param[in] str NULL-terminated string representing the IPv4 address
 * @param[out] ipAddr Binary representation of the IPv4 address
 * @return Error code
 **/

error_t ipv4StringToAddr(const char_t *str, Ipv4Addr *ipAddr)
{
   error_t error;
   int_t i = 0;
   int_t value = -1;

   //Parse input string
   while(1)
   {
      //Decimal digit found?
      if(osIsdigit(*str))
      {
         //First digit to be decoded?
         if(value < 0)
            value = 0;

         //Update the value of the current byte
         value = (value * 10) + (*str - '0');

         //The resulting value shall be in range 0 to 255
         if(value > 255)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }
      }
      //Dot separator found?
      else if(*str == '.' && i < 4)
      {
         //Each dot must be preceded by a valid number
         if(value < 0)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
            break;
         }

         //Save the current byte
         ((uint8_t *) ipAddr)[i++] = value;
         //Prepare to decode the next byte
         value = -1;
      }
      //End of string detected?
      else if(*str == '\0' && i == 3)
      {
         //The NULL character must be preceded by a valid number
         if(value < 0)
         {
            //The conversion failed
            error = ERROR_INVALID_SYNTAX;
         }
         else
         {
            //Save the last byte of the IPv4 address
            ((uint8_t *) ipAddr)[i] = value;
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
 * @brief Convert a binary IPv4 address to dot-decimal notation
 * @param[in] ipAddr Binary representation of the IPv4 address
 * @param[out] str NULL-terminated string representing the IPv4 address
 * @return Pointer to the formatted string
 **/

char_t *ipv4AddrToString(Ipv4Addr ipAddr, char_t *str)
{
   uint8_t *p;
   static char_t buffer[16];

   //If the NULL pointer is given as parameter, then the internal buffer is used
   if(str == NULL)
      str = buffer;

   //Cast the address to byte array
   p = (uint8_t *) &ipAddr;
   //Format IPv4 address
   osSprintf(str, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 "", p[0], p[1], p[2], p[3]);

   //Return a pointer to the formatted string
   return str;
}


/**
 * @brief Dump IPv4 header for debugging purpose
 * @param[in] ipHeader Pointer to the IPv4 header
 **/

void ipv4DumpHeader(const Ipv4Header *ipHeader)
{
   //Dump IP header contents
   TRACE_DEBUG("  Version = %" PRIu8 "\r\n", ipHeader->version);
   TRACE_DEBUG("  Header Length = %" PRIu8 "\r\n", ipHeader->headerLength);
   TRACE_DEBUG("  Type Of Service = %" PRIu8 "\r\n", ipHeader->typeOfService);
   TRACE_DEBUG("  Total Length = %" PRIu16 "\r\n", ntohs(ipHeader->totalLength));
   TRACE_DEBUG("  Identification = %" PRIu16 "\r\n", ntohs(ipHeader->identification));
   TRACE_DEBUG("  Flags = 0x%01X\r\n", ntohs(ipHeader->fragmentOffset) >> 13);
   TRACE_DEBUG("  Fragment Offset = %" PRIu16 "\r\n", ntohs(ipHeader->fragmentOffset) & 0x1FFF);
   TRACE_DEBUG("  Time To Live = %" PRIu8 "\r\n", ipHeader->timeToLive);
   TRACE_DEBUG("  Protocol = %" PRIu8 "\r\n", ipHeader->protocol);
   TRACE_DEBUG("  Header Checksum = 0x%04" PRIX16 "\r\n", ntohs(ipHeader->headerChecksum));
   TRACE_DEBUG("  Src Addr = %s\r\n", ipv4AddrToString(ipHeader->srcAddr, NULL));
   TRACE_DEBUG("  Dest Addr = %s\r\n", ipv4AddrToString(ipHeader->destAddr, NULL));
}

#endif
