/**
 * @file icmpv6.c
 * @brief ICMPv6 (Internet Control Message Protocol Version 6)
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
 * ICMPv6 is used by IPv6 nodes to report errors encountered in
 * processing packets, and to perform other Internet-layer functions.
 * ICMPv6 is an integral part of IPv6 and must be fully implemented
 * by every IPv6 node. Refer to the RFC 2463 for more details
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL ICMPV6_TRACE_LEVEL

//Dependencies
#include <string.h>
#include "core/net.h"
#include "core/ip.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "ipv6/ipv6_pmtu.h"
#include "ipv6/icmpv6.h"
#include "ipv6/mld.h"
#include "ipv6/ndp.h"
#include "ipv6/ndp_router_adv.h"
#include "mibs/ip_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED)


/**
 * @brief Enable support for multicast Echo Request messages
 * @param[in] interface Underlying network interface
 * @param[in] enable When the flag is set to TRUE, the host will respond to
 *   multicast Echo Requests. When the flag is set to FALSE, incoming Echo
 *   Request messages destined to a multicast address will be dropped
 * @return Error code
 **/

error_t icmpv6EnableMulticastEchoRequest(NetInterface *interface, bool_t enable)
{
   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Enable or disable support for multicast Echo Request messages
   interface->ipv6Context.enableMulticastEchoReq = enable;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Incoming ICMPv6 message processing
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv6 pseudo header
 * @param[in] buffer Multi-part buffer containing the incoming ICMPv6 message
 * @param[in] offset Offset to the first byte of the ICMPv6 message
 * @param[in] hopLimit Hop Limit field from IPv6 header
 **/

void icmpv6ProcessMessage(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit)
{
   size_t length;
   Icmpv6Header *header;

   //Total number of ICMP messages which the entity received
   IP_MIB_INC_COUNTER32(icmpv6Stats.icmpStatsInMsgs, 1);

   //Retrieve the length of the ICMPv6 message
   length = netBufferGetLength(buffer) - offset;

   //Ensure the message length is correct
   if(length < sizeof(Icmpv6Header))
   {
      //Number of ICMP messages which the entity received but determined
      //as having ICMP-specific errors
      IP_MIB_INC_COUNTER32(icmpv6Stats.icmpStatsInErrors, 1);

      //Silently discard incoming message
      return;
   }

   //Point to the ICMPv6 message header
   header = netBufferAt(buffer, offset);

   //Sanity check
   if(header == NULL)
      return;

   //Debug message
   TRACE_INFO("ICMPv6 message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message contents for debugging purpose
   icmpv6DumpMessage(header);

   //Verify checksum value
   if(ipCalcUpperLayerChecksumEx(pseudoHeader,
      sizeof(Ipv6PseudoHeader), buffer, offset, length) != 0x0000)
   {
      //Debug message
      TRACE_WARNING("Wrong ICMPv6 header checksum!\r\n");

      //Number of ICMP messages which the entity received but determined
      //as having ICMP-specific errors
      IP_MIB_INC_COUNTER32(icmpv6Stats.icmpStatsInErrors, 1);

      //Exit immediately
      return;
   }

   //Check whether the destination address is the tentative address
   if(ipv6IsTentativeAddr(interface, &pseudoHeader->destAddr))
   {
      //The interface must accept Neighbor Solicitation and
      //Neighbor Advertisement messages
      if(header->type != ICMPV6_TYPE_NEIGHBOR_SOL &&
         header->type != ICMPV6_TYPE_NEIGHBOR_ADV)
      {
         //Other packets addressed to the tentative address
         //should be silently discarded
         return;
      }
   }

   //Increment per-message type ICMP counter
   IP_MIB_INC_COUNTER32(icmpv6MsgStatsTable.icmpMsgStatsInPkts[header->type], 1);

   //Check the type of message
   switch(header->type)
   {
   //Destination Unreachable message?
   case ICMPV6_TYPE_DEST_UNREACHABLE:
      //Process Destination Unreachable message
      icmpv6ProcessDestUnreachable(interface, pseudoHeader, buffer, offset);
      break;
   //Packet Too Big message?
   case ICMPV6_TYPE_PACKET_TOO_BIG:
      //Process Packet Too Big message
      icmpv6ProcessPacketTooBig(interface, pseudoHeader, buffer, offset);
      break;
   //Echo Request message?
   case ICMPV6_TYPE_ECHO_REQUEST:
      //Process Echo Request message
      icmpv6ProcessEchoRequest(interface, pseudoHeader, buffer, offset);
      break;
#if (MLD_SUPPORT == ENABLED)
   //Multicast Listener Query message?
   case ICMPV6_TYPE_MULTICAST_LISTENER_QUERY:
      //Process Multicast Listener Query message
      mldProcessListenerQuery(interface, pseudoHeader, buffer, offset, hopLimit);
      break;
   //Version 1 Multicast Listener Report message?
   case ICMPV6_TYPE_MULTICAST_LISTENER_REPORT_V1:
      //Process Version 1 Multicast Listener Report message
      mldProcessListenerReport(interface, pseudoHeader, buffer, offset, hopLimit);
      break;
#endif
#if (NDP_ROUTER_ADV_SUPPORT == ENABLED)
   //Router Solicitation message?
   case ICMPV6_TYPE_ROUTER_SOL:
      //Process Router Solicitation message
      ndpProcessRouterSol(interface, pseudoHeader, buffer, offset, hopLimit);
      break;
#endif
#if (NDP_SUPPORT == ENABLED)
   //Router Advertisement message?
   case ICMPV6_TYPE_ROUTER_ADV:
      //Process Router Advertisement message
      ndpProcessRouterAdv(interface, pseudoHeader, buffer, offset, hopLimit);
      break;
   //Neighbor Solicitation message?
   case ICMPV6_TYPE_NEIGHBOR_SOL:
      //Process Neighbor Solicitation message
      ndpProcessNeighborSol(interface, pseudoHeader, buffer, offset, hopLimit);
      break;
   //Neighbor Advertisement message?
   case ICMPV6_TYPE_NEIGHBOR_ADV:
      //Process Neighbor Advertisement message
      ndpProcessNeighborAdv(interface, pseudoHeader, buffer, offset, hopLimit);
      break;
   //Redirect message?
   case ICMPV6_TYPE_REDIRECT:
      //Process Redirect message
      ndpProcessRedirect(interface, pseudoHeader, buffer, offset, hopLimit);
      break;
#endif
   //Unknown type?
   default:
      //Debug message
      TRACE_WARNING("Unknown ICMPv6 message type!\r\n");
      //Discard incoming ICMPv6 message
      break;
   }
}


/**
 * @brief Destination Unreachable message processing
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv6 pseudo header
 * @param[in] buffer Multi-part buffer containing the incoming ICMPv6 message
 * @param[in] offset Offset to the first byte of the ICMPv6 message
 **/

void icmpv6ProcessDestUnreachable(NetInterface *interface,
   Ipv6PseudoHeader *pseudoHeader, const NetBuffer *buffer, size_t offset)
{
   size_t length;
   Icmpv6DestUnreachableMessage *icmpHeader;

   //Retrieve the length of the Destination Unreachable message
   length = netBufferGetLength(buffer) - offset;

   //Ensure the packet length is correct
   if(length < sizeof(Icmpv6DestUnreachableMessage))
      return;

   //Point to the ICMPv6 header
   icmpHeader = netBufferAt(buffer, offset);

   //Sanity check
   if(icmpHeader == NULL)
      return;

   //Debug message
   TRACE_INFO("ICMPv6 Destination Unreachable message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message contents for debugging purpose
   icmpv6DumpDestUnreachableMessage(icmpHeader);
}


/**
 * @brief Packet Too Big message processing
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv6 pseudo header
 * @param[in] buffer Multi-part buffer containing the incoming ICMPv6 message
 * @param[in] offset Offset to the first byte of the ICMPv6 message
 **/

void icmpv6ProcessPacketTooBig(NetInterface *interface,
   Ipv6PseudoHeader *pseudoHeader, const NetBuffer *buffer, size_t offset)
{
   size_t length;
   Icmpv6PacketTooBigMessage *icmpHeader;

#if (IPV6_PMTU_SUPPORT == ENABLED)
   uint32_t tentativePathMtu;
   Ipv6Header *ipHeader;
#endif

   //Retrieve the length of the Packet Too Big message
   length = netBufferGetLength(buffer) - offset;

   //Ensure the packet length is correct
   if(length < sizeof(Icmpv6PacketTooBigMessage))
      return;

   //Point to the ICMPv6 header
   icmpHeader = netBufferAt(buffer, offset);

   //Sanity check
   if(icmpHeader == NULL)
      return;

   //Debug message
   TRACE_INFO("ICMPv6 Packet Too Big message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message contents for debugging purpose
   icmpv6DumpPacketTooBigMessage(icmpHeader);

#if (IPV6_PMTU_SUPPORT == ENABLED)
   //Move to the beginning of the original IPv6 packet
   offset += sizeof(Icmpv6PacketTooBigMessage);
   length -= sizeof(Icmpv6PacketTooBigMessage);

   //Ensure the packet length is correct
   if(length < sizeof(Ipv6Header))
      return;

   //Point to the original IPv6 header
   ipHeader = netBufferAt(buffer, offset);

   //Sanity check
   if(ipHeader == NULL)
      return;

   //The node uses the value in the MTU field in the Packet Too Big
   //message as a tentative PMTU value
   tentativePathMtu = ntohl(icmpHeader->mtu);

   //Update the PMTU for the specified destination address
   ipv6UpdatePathMtu(interface, &ipHeader->destAddr, tentativePathMtu);
#endif
}


/**
 * @brief Echo Request message processing
 * @param[in] interface Underlying network interface
 * @param[in] requestPseudoHeader IPv6 pseudo header
 * @param[in] request Multi-part buffer containing the incoming ICMPv6 message
 * @param[in] requestOffset Offset to the first byte of the ICMPv6 message
 **/

void icmpv6ProcessEchoRequest(NetInterface *interface, Ipv6PseudoHeader *requestPseudoHeader,
   const NetBuffer *request, size_t requestOffset)
{
   error_t error;
   size_t requestLength;
   size_t replyOffset;
   size_t replyLength;
   NetBuffer *reply;
   Icmpv6EchoMessage *requestHeader;
   Icmpv6EchoMessage *replyHeader;
   Ipv6PseudoHeader replyPseudoHeader;

   //Retrieve the length of the Echo Request message
   requestLength = netBufferGetLength(request) - requestOffset;

   //Ensure the packet length is correct
   if(requestLength < sizeof(Icmpv6EchoMessage))
      return;

   //Point to the Echo Request header
   requestHeader = netBufferAt(request, requestOffset);

   //Sanity check
   if(requestHeader == NULL)
      return;

   //Debug message
   TRACE_INFO("ICMPv6 Echo Request message received (%" PRIuSIZE " bytes)...\r\n", requestLength);
   //Dump message contents for debugging purpose
   icmpv6DumpEchoMessage(requestHeader);

   //Check whether the destination address of the Echo Request message is
   //a multicast address
   if(ipv6IsMulticastAddr(&requestPseudoHeader->destAddr))
   {
      //If support for multicast Echo Request messages has been explicitly
      //disabled, then the host shall not respond to the incoming request
      if(!interface->ipv6Context.enableMulticastEchoReq)
         return;

      //The source address of the reply must be a unicast address belonging to
      //the interface on which the multicast Echo Request message was received
      error = ipv6SelectSourceAddr(&interface, &requestPseudoHeader->srcAddr,
         &replyPseudoHeader.srcAddr);
      //Any error to report?
      if(error)
         return;
   }
   else
   {
      //The destination address of the Echo Request message is a unicast address
      replyPseudoHeader.srcAddr = requestPseudoHeader->destAddr;
   }

   //Allocate memory to hold the Echo Reply message
   reply = ipAllocBuffer(sizeof(Icmpv6EchoMessage), &replyOffset);
   //Failed to allocate memory?
   if(reply == NULL)
      return;

   //Point to the Echo Reply header
   replyHeader = netBufferAt(reply, replyOffset);

   //Format Echo Reply header
   replyHeader->type = ICMPV6_TYPE_ECHO_REPLY;
   replyHeader->code = 0;
   replyHeader->checksum = 0;
   replyHeader->identifier = requestHeader->identifier;
   replyHeader->sequenceNumber = requestHeader->sequenceNumber;

   //Point to the first data byte
   requestOffset += sizeof(Icmpv6EchoMessage);
   requestLength -= sizeof(Icmpv6EchoMessage);

   //The data received in the ICMPv6 Echo Request message must be returned
   //entirely and unmodified in the ICMPv6 Echo Reply message
   error = netBufferConcat(reply, request, requestOffset, requestLength);

   //Check status code
   if(!error)
   {
      NetTxAncillary ancillary;

      //Get the length of the resulting message
      replyLength = netBufferGetLength(reply) - replyOffset;

      //Format IPv6 pseudo header
      replyPseudoHeader.destAddr = requestPseudoHeader->srcAddr;
      replyPseudoHeader.length = htonl(replyLength);
      replyPseudoHeader.reserved[0] = 0;
      replyPseudoHeader.reserved[1] = 0;
      replyPseudoHeader.reserved[2] = 0;
      replyPseudoHeader.nextHeader = IPV6_ICMPV6_HEADER;

      //Message checksum calculation
      replyHeader->checksum = ipCalcUpperLayerChecksumEx(&replyPseudoHeader,
         sizeof(Ipv6PseudoHeader), reply, replyOffset, replyLength);

      //Total number of ICMP messages which this entity attempted to send
      IP_MIB_INC_COUNTER32(icmpv6Stats.icmpStatsOutMsgs, 1);
      //Increment per-message type ICMP counter
      IP_MIB_INC_COUNTER32(icmpv6MsgStatsTable.icmpMsgStatsOutPkts[ICMPV6_TYPE_ECHO_REPLY], 1);

      //Debug message
      TRACE_INFO("Sending ICMPv6 Echo Reply message (%" PRIuSIZE " bytes)...\r\n", replyLength);
      //Dump message contents for debugging purpose
      icmpv6DumpEchoMessage(replyHeader);

      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_TX_ANCILLARY;

      //Send Echo Reply message
      ipv6SendDatagram(interface, &replyPseudoHeader, reply, replyOffset,
         &ancillary);
   }

   //Free previously allocated memory block
   netBufferFree(reply);
}


/**
 * @brief Send an ICMPv6 Error message
 * @param[in] interface Underlying network interface
 * @param[in] type Message type
 * @param[in] code Specific message code
 * @param[in] parameter Specific message parameter
 * @param[in] ipPacket Multi-part buffer that holds the invoking IPv6 packet
 * @param[in] ipPacketOffset Offset to the first byte of the IPv6 packet
 * @return Error code
 **/

error_t icmpv6SendErrorMessage(NetInterface *interface, uint8_t type, uint8_t code,
   uint32_t parameter, const NetBuffer *ipPacket, size_t ipPacketOffset)
{
   error_t error;
   size_t offset;
   size_t length;
   NetBuffer *icmpMessage;
   Icmpv6ErrorMessage *icmpHeader;
   Ipv6Header *ipHeader;
   Ipv6PseudoHeader pseudoHeader;

   //Retrieve the length of the invoking IPv6 packet
   length = netBufferGetLength(ipPacket) - ipPacketOffset;

   //Check the length of the IPv6 packet
   if(length < sizeof(Ipv6Header))
      return ERROR_INVALID_LENGTH;

   //Point to the header of the invoking packet
   ipHeader = netBufferAt(ipPacket, ipPacketOffset);

   //Sanity check
   if(ipHeader == NULL)
      return ERROR_FAILURE;

   //Check the type of the invoking packet
   if(ipHeader->nextHeader == IPV6_ICMPV6_HEADER)
   {
      //Make sure the ICMPv6 message is valid
      if(length >= (sizeof(Ipv6Header) + sizeof(Icmpv6Header)))
      {
         //Point to the ICMPv6 header
         icmpHeader = netBufferAt(ipPacket, ipPacketOffset + sizeof(Ipv6Header));

         //Sanity check
         if(icmpHeader != NULL)
         {
            //An ICMPv6 error message must not be originated as a result
            //of receiving an ICMPv6 error or redirect message
            if(icmpHeader->type == ICMPV6_TYPE_DEST_UNREACHABLE ||
               icmpHeader->type == ICMPV6_TYPE_PACKET_TOO_BIG ||
               icmpHeader->type == ICMPV6_TYPE_TIME_EXCEEDED ||
               icmpHeader->type == ICMPV6_TYPE_PARAM_PROBLEM ||
               icmpHeader->type == ICMPV6_TYPE_REDIRECT)
            {
               //Do not send the ICMPv6 error message...
               return ERROR_INVALID_TYPE;
            }
         }
      }
   }

   //An ICMPv6 error message must not be originated as a result of
   //receiving a packet destined to an IPv6 multicast address
   if(ipv6IsMulticastAddr(&ipHeader->destAddr))
   {
      //There are two exceptions to this rule
      if(type == ICMPV6_TYPE_PACKET_TOO_BIG)
      {
         //The Packet Too Big Message to allow Path MTU discovery to
         //work for IPv6 multicast
      }
      else if(type == ICMPV6_TYPE_PARAM_PROBLEM &&
         code == ICMPV6_CODE_UNKNOWN_IPV6_OPTION)
      {
         //The Parameter Problem Message, reporting an unrecognized IPv6
         //option that has the Option Type highest-order two bits set to 10
      }
      else
      {
         //Do not send the ICMPv6 error message...
         return ERROR_INVALID_ADDRESS;
      }
   }

   //An ICMPv6 error message must not be originated as a result of receiving a
   //packet whose source address does not uniquely identify a single node (e.g.
   //the IPv6 unspecified address, an IPv6 multicast address, or an address
   //known by the ICMPv6 message originator to be an IPv6 anycast address)
   if(ipv6IsAnycastAddr(interface, &ipHeader->srcAddr))
      return ERROR_INVALID_ADDRESS;

   //Return as much of invoking IPv6 packet as possible without
   //the ICMPv6 packet exceeding the minimum IPv6 MTU
   length = MIN(length, IPV6_DEFAULT_MTU -
      sizeof(Ipv6Header) - sizeof(Icmpv6ErrorMessage));

   //Allocate a memory buffer to hold the ICMPv6 message
   icmpMessage = ipAllocBuffer(sizeof(Icmpv6ErrorMessage), &offset);

   //Failed to allocate memory?
   if(icmpMessage == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the ICMPv6 header
   icmpHeader = netBufferAt(icmpMessage, offset);

   //Format ICMPv6 Error message
   icmpHeader->type = type;
   icmpHeader->code = code;
   icmpHeader->checksum = 0;
   icmpHeader->parameter = htonl(parameter);

   //Copy incoming IPv6 packet contents
   error = netBufferConcat(icmpMessage, ipPacket, ipPacketOffset, length);

   //Check status code
   if(!error)
   {
      //Get the length of the resulting message
      length = netBufferGetLength(icmpMessage) - offset;

      //Format IPv6 pseudo header
      pseudoHeader.destAddr = ipHeader->srcAddr;
      pseudoHeader.length = htonl(length);
      pseudoHeader.reserved[0] = 0;
      pseudoHeader.reserved[1] = 0;
      pseudoHeader.reserved[2] = 0;
      pseudoHeader.nextHeader = IPV6_ICMPV6_HEADER;

      //Select the relevant source address
      error = ipv6SelectSourceAddr(&interface, &pseudoHeader.destAddr,
         &pseudoHeader.srcAddr);

      //Check status code
      if(!error)
      {
         NetTxAncillary ancillary;

         //Message checksum calculation
         icmpHeader->checksum = ipCalcUpperLayerChecksumEx(&pseudoHeader,
            sizeof(Ipv6PseudoHeader), icmpMessage, offset, length);

         //Total number of ICMP messages which this entity attempted to send
         IP_MIB_INC_COUNTER32(icmpv6Stats.icmpStatsOutMsgs, 1);
         //Increment per-message type ICMP counter
         IP_MIB_INC_COUNTER32(icmpv6MsgStatsTable.icmpMsgStatsOutPkts[type], 1);

         //Debug message
         TRACE_INFO("Sending ICMPv6 Error message (%" PRIuSIZE " bytes)...\r\n", length);
         //Dump message contents for debugging purpose
         icmpv6DumpErrorMessage(icmpHeader);

         //Additional options can be passed to the stack along with the packet
         ancillary = NET_DEFAULT_TX_ANCILLARY;

         //Send ICMPv6 Error message
         error = ipv6SendDatagram(interface, &pseudoHeader, icmpMessage, offset,
            &ancillary);
      }
   }

   //Free previously allocated memory
   netBufferFree(icmpMessage);

   //Return status code
   return error;
}


/**
 * @brief Dump ICMPv6 message for debugging purpose
 * @param[in] message Pointer to the ICMP message
 **/

void icmpv6DumpMessage(const Icmpv6Header *message)
{
   //Dump ICMPv6 message
   TRACE_DEBUG("  Type = %" PRIu8 "\r\n", message->type);
   TRACE_DEBUG("  Code = %" PRIu8 "\r\n", message->code);
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(message->checksum));
}


/**
 * @brief Dump ICMPv6 Destination Unreachable message
 * @param[in] message Pointer to the ICMPv6 message
 **/

void icmpv6DumpDestUnreachableMessage(const Icmpv6DestUnreachableMessage *message)
{
   //Dump ICMPv6 message
   TRACE_DEBUG("  Type = %" PRIu8 "\r\n", message->type);
   TRACE_DEBUG("  Code = %" PRIu8 "\r\n", message->code);
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(message->checksum));
}


/**
 * @brief Dump ICMPv6 Packet Too Big message
 * @param[in] message Pointer to the ICMPv6 message
 **/

void icmpv6DumpPacketTooBigMessage(const Icmpv6PacketTooBigMessage *message)
{
   //Dump ICMPv6 message
   TRACE_DEBUG("  Type = %" PRIu8 "\r\n", message->type);
   TRACE_DEBUG("  Code = %" PRIu8 "\r\n", message->code);
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(message->checksum));
   TRACE_DEBUG("  MTU = %" PRIu32 "\r\n", ntohl(message->mtu));
}


/**
 * @brief Dump ICMPv6 Echo Request or Echo Reply message
 * @param[in] message Pointer to the ICMPv6 message
 **/

void icmpv6DumpEchoMessage(const Icmpv6EchoMessage *message)
{
   //Dump ICMPv6 message
   TRACE_DEBUG("  Type = %" PRIu8 "\r\n", message->type);
   TRACE_DEBUG("  Code = %" PRIu8 "\r\n", message->code);
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(message->checksum));
   TRACE_DEBUG("  Identifier = 0x%04" PRIX16 "\r\n", ntohs(message->identifier));
   TRACE_DEBUG("  Sequence Number = 0x%04" PRIX16 "\r\n", ntohs(message->sequenceNumber));
}


/**
 * @brief Dump generic ICMPv6 Error message
 * @param[in] message Pointer to the ICMPv6 message
 **/

void icmpv6DumpErrorMessage(const Icmpv6ErrorMessage *message)
{
   //Dump ICMP message
   TRACE_DEBUG("  Type = %" PRIu8 "\r\n", message->type);
   TRACE_DEBUG("  Code = %" PRIu8 "\r\n", message->code);
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(message->checksum));
   TRACE_DEBUG("  Parameter = %" PRIu32 "\r\n", ntohl(message->parameter));
}

#endif
