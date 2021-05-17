/**
 * @file ipv6_frag.c
 * @brief IPv6 fragmentation and reassembly
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
#define TRACE_LEVEL IPV6_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "core/ip.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_frag.h"
#include "ipv6/icmpv6.h"
#include "mibs/ip_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED && IPV6_FRAG_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t ipv6FragTickCounter;


/**
 * @brief Fragment IPv6 datagram into smaller packets
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv6 pseudo header
 * @param[in] payload Multi-part buffer containing the payload
 * @param[in] payloadOffset Offset to the first payload byte
 * @param[in] pathMtu PMTU value
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ipv6FragmentDatagram(NetInterface *interface,
   Ipv6PseudoHeader *pseudoHeader, const NetBuffer *payload,
   size_t payloadOffset, size_t pathMtu, NetTxAncillary *ancillary)
{
   error_t error;
   uint32_t id;
   size_t offset;
   size_t length;
   size_t payloadLen;
   size_t fragmentOffset;
   size_t maxFragmentSize;
   NetBuffer *fragment;

   //Number of IP datagrams that would require fragmentation in order to be transmitted
   IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsOutFragReqds, 1);
   IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsOutFragReqds, 1);

   //Retrieve the length of the payload
   payloadLen = netBufferGetLength(payload) - payloadOffset;

   //Allocate a memory buffer to hold IP fragments
   fragment = ipAllocBuffer(0, &fragmentOffset);
   //Failed to allocate memory?
   if(!fragment)
      return ERROR_OUT_OF_MEMORY;

   //Identification field is used to identify fragments of an original IP datagram
   id = interface->ipv6Context.identification++;

   //The node should never set its PMTU estimate below the IPv6 minimum link MTU
   pathMtu = MAX(pathMtu, IPV6_DEFAULT_MTU);

   //Determine the maximum payload size for fragmented packets
   maxFragmentSize = pathMtu - sizeof(Ipv6Header) - sizeof(Ipv6FragmentHeader);
   //The size shall be a multiple of 8-byte blocks
   maxFragmentSize -= (maxFragmentSize % 8);

   //Initialize error code
   error = NO_ERROR;

   //Split the payload into multiple IP fragments
   for(offset = 0; offset < payloadLen; offset += length)
   {
      //Flush the contents of the fragment
      error = netBufferSetLength(fragment, fragmentOffset);
      //Sanity check
      if(error)
         break;

      //Process the last fragment?
      if((payloadLen - offset) <= maxFragmentSize)
      {
         //Size of the current fragment
         length = payloadLen - offset;
         //Copy fragment data
         netBufferConcat(fragment, payload, payloadOffset + offset, length);

         //Do not set the MF flag for the last fragment
         error = ipv6SendPacket(interface, pseudoHeader, id, offset, fragment,
            fragmentOffset, ancillary);
      }
      else
      {
         //Size of the current fragment (must be a multiple of 8-byte blocks)
         length = maxFragmentSize;
         //Copy fragment data
         netBufferConcat(fragment, payload, payloadOffset + offset, length);

         //Fragmented packets must have the M flag set
         error = ipv6SendPacket(interface, pseudoHeader, id, IPV6_FLAG_M |
            offset, fragment, fragmentOffset, ancillary);
      }

      //Failed to send current IP fragment?
      if(error)
         break;

      //Number of IP datagram fragments that have been generated as a result
      //of fragmentation at this entity
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsOutFragCreates, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsOutFragCreates, 1);
   }

   //Check status code
   if(error)
   {
      //Number of IP datagrams that have been discarded because they needed
      //to be fragmented at this entity but could not be
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsOutFragFails, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsOutFragFails, 1);
   }
   else
   {
      //Number of IP datagrams that have been successfully fragmented at this
      //entity
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsOutFragOKs, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsOutFragOKs, 1);
   }

   //Free previously allocated memory
   netBufferFree(fragment);
   //Return status code
   return error;
}


/**
 * @brief Parse Fragment header and reassemble original datagram
 * @param[in] interface Underlying network interface
 * @param[in] ipPacket Multi-part buffer containing the incoming IPv6 packet
 * @param[in] ipPacketOffset Offset to the first byte of the IPv6 packet
 * @param[in] fragHeaderOffset Offset to the Fragment header
 * @param[in] nextHeaderOffset Offset to the Next Header field of the previous header
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 **/

void ipv6ParseFragmentHeader(NetInterface *interface, const NetBuffer *ipPacket,
   size_t ipPacketOffset, size_t fragHeaderOffset, size_t nextHeaderOffset,
   NetRxAncillary *ancillary)
{
   error_t error;
   size_t n;
   size_t length;
   uint16_t offset;
   uint16_t dataFirst;
   uint16_t dataLast;
   Ipv6FragDesc *frag;
   Ipv6HoleDesc *hole;
   Ipv6HoleDesc *prevHole;
   Ipv6Header *ipHeader;
   Ipv6FragmentHeader *fragHeader;

   //Number of IP fragments received which needed to be reassembled
   IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmReqds, 1);
   IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmReqds, 1);

   //Remaining bytes to process in the payload
   length = netBufferGetLength(ipPacket) - fragHeaderOffset;

   //Ensure the fragment header is valid
   if(length < sizeof(Ipv6FragmentHeader))
   {
      //Number of failures detected by the IP reassembly algorithm
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmFails, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

      //Drop the incoming fragment
      return;
   }

   //Point to the IPv6 header
   ipHeader = netBufferAt(ipPacket, ipPacketOffset);
   //Sanity check
   if(ipHeader == NULL)
      return;

   //Point to the Fragment header
   fragHeader = netBufferAt(ipPacket, fragHeaderOffset);
   //Sanity check
   if(fragHeader == NULL)
      return;

   //Calculate the length of the fragment
   length -= sizeof(Ipv6FragmentHeader);
   //Convert the fragment offset from network byte order
   offset = ntohs(fragHeader->fragmentOffset);

   //Every fragment except the last must contain a multiple of 8 bytes of data
   if((offset & IPV6_FLAG_M) && (length % 8))
   {
      //Number of failures detected by the IP reassembly algorithm
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmFails, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

      //Compute the offset of the Payload Length field within the packet
      n = (uint8_t *) &ipHeader->payloadLen - (uint8_t *) ipHeader;

      //The fragment must be discarded and an ICMP Parameter Problem
      //message should be sent to the source of the fragment, pointing
      //to the Payload Length field of the fragment packet
      icmpv6SendErrorMessage(interface, ICMPV6_TYPE_PARAM_PROBLEM,
         ICMPV6_CODE_INVALID_HEADER_FIELD, n, ipPacket, ipPacketOffset);

      //Drop the incoming fragment
      return;
   }

   //Calculate the index of the first byte
   dataFirst = offset & IPV6_OFFSET_MASK;
   //Calculate the index immediately following the last byte
   dataLast = dataFirst + (uint16_t) length;

   //Search for a matching IP datagram being reassembled
   frag = ipv6SearchFragQueue(interface, ipHeader, fragHeader);

   //No matching entry in the reassembly queue?
   if(frag == NULL)
   {
      //Number of failures detected by the IP reassembly algorithm
      IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmFails, 1);
      IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

      //Drop the incoming fragment
      return;
   }

   //The very first fragment requires special handling
   if(!(offset & IPV6_OFFSET_MASK))
   {
      uint8_t *p;

      //Calculate the length of the unfragmentable part
      frag->unfragPartLength = fragHeaderOffset - ipPacketOffset;

      //The size of the reconstructed datagram exceeds the maximum value?
      if((frag->unfragPartLength + frag->fragPartLength) > IPV6_MAX_FRAG_DATAGRAM_SIZE)
      {
         //Number of failures detected by the IP reassembly algorithm
         IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

         //Retrieve the offset of the Fragment header within the packet
         n = fragHeaderOffset - ipPacketOffset;
         //Compute the exact offset of the Fragment Offset field
         n += (uint8_t *) &fragHeader->fragmentOffset - (uint8_t *) fragHeader;

         //The fragment must be discarded and an ICMP Parameter Problem
         //message should be sent to the source of the fragment, pointing
         //to the Fragment Offset field of the fragment packet
         icmpv6SendErrorMessage(interface, ICMPV6_TYPE_PARAM_PROBLEM,
            ICMPV6_CODE_INVALID_HEADER_FIELD, n, ipPacket, ipPacketOffset);

         //Drop the reconstructed datagram
         netBufferSetLength((NetBuffer *) &frag->buffer, 0);
         //Exit immediately
         return;
      }

      //Make sure the unfragmentable part entirely fits in the first chunk
      if(frag->unfragPartLength > frag->buffer.chunk[0].size)
      {
         //Number of failures detected by the IP reassembly algorithm
         IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

         //Drop the reconstructed datagram
         netBufferSetLength((NetBuffer *) &frag->buffer, 0);
         //Exit immediately
         return;
      }

      //Fix the length of the first chunk
      frag->buffer.chunk[0].length = (uint16_t) frag->unfragPartLength;

      //The unfragmentable part of the reassembled packet consists
      //of all headers up to, but not including, the Fragment header
      //of the first fragment packet
      netBufferCopy((NetBuffer *) &frag->buffer, 0, ipPacket,
         ipPacketOffset, frag->unfragPartLength);

      //Point to the Next Header field of the last header
      p = netBufferAt((NetBuffer *) &frag->buffer,
         nextHeaderOffset - ipPacketOffset);

      //The Next Header field of the last header of the unfragmentable
      //part is obtained from the Next Header field of the first
      //fragment's Fragment header
      *p = fragHeader->nextHeader;
   }

   //It may be necessary to increase the size of the buffer...
   if(dataLast > frag->fragPartLength)
   {
      //The size of the reconstructed datagram exceeds the maximum value?
      if((frag->unfragPartLength + dataLast) > IPV6_MAX_FRAG_DATAGRAM_SIZE)
      {
         //Number of failures detected by the IP reassembly algorithm
         IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

         //Retrieve the offset of the Fragment header within the packet
         n = fragHeaderOffset - ipPacketOffset;
         //Compute the exact offset of the Fragment Offset field
         n += (uint8_t *) &fragHeader->fragmentOffset - (uint8_t *) fragHeader;

         //The fragment must be discarded and an ICMP Parameter Problem
         //message should be sent to the source of the fragment, pointing
         //to the Fragment Offset field of the fragment packet
         icmpv6SendErrorMessage(interface, ICMPV6_TYPE_PARAM_PROBLEM,
            ICMPV6_CODE_INVALID_HEADER_FIELD, n, ipPacket, ipPacketOffset);

         //Drop the reconstructed datagram
         netBufferSetLength((NetBuffer *) &frag->buffer, 0);
         //Exit immediately
         return;
      }

      //Adjust the size of the reconstructed datagram
      error = netBufferSetLength((NetBuffer *) &frag->buffer,
         frag->unfragPartLength + dataLast + sizeof(Ipv6HoleDesc));

      //Any error to report?
      if(error)
      {
         //Number of failures detected by the IP reassembly algorithm
         IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

         //Drop the reconstructed datagram
         netBufferSetLength((NetBuffer *) &frag->buffer, 0);
         //Exit immediately
         return;
      }

      //Actual length of the fragmentable part
      frag->fragPartLength = dataLast;
   }

   //Select the first hole descriptor from the list
   hole = ipv6FindHole(frag, frag->firstHole);
   //Keep track of the previous hole in the list
   prevHole = NULL;

   //Iterate through the hole descriptors
   while(hole != NULL)
   {
      //Save lower and upper boundaries for later use
      uint16_t holeFirst = hole->first;
      uint16_t holeLast = hole->last;

      //Check whether the newly arrived fragment interacts with this hole
      //in some way
      if(dataFirst < holeLast && dataLast > holeFirst)
      {
#if (IPV6_OVERLAPPING_FRAG_SUPPORT == DISABLED)
         //When reassembling an IPv6 datagram, if one or more its constituent
         //fragments is determined to be an overlapping fragment, the entire
         //datagram must be silently discarded (refer to RFC 5722, section 4)
         if(dataFirst < holeFirst || dataLast > holeLast)
         {
            //Number of failures detected by the IP reassembly algorithm
            IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmFails, 1);
            IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

            //Drop the reconstructed datagram
            netBufferSetLength((NetBuffer *) &frag->buffer, 0);
            //Exit immediately
            return;
         }
#endif
         //The current descriptor is no longer valid. We will destroy it,
         //and in the next two steps, we will determine whether or not it
         //is necessary to create any new hole descriptors
         if(prevHole != NULL)
            prevHole->next = hole->next;
         else
            frag->firstHole = hole->next;

         //Is there still a hole at the beginning of the segment?
         if(dataFirst > holeFirst)
         {
            //Create a new entry that describes this hole
            hole = ipv6FindHole(frag, holeFirst);
            hole->first = holeFirst;
            hole->last = dataFirst;

            //Insert the newly created entry into the hole descriptor list
            if(prevHole != NULL)
            {
               hole->next = prevHole->next;
               prevHole->next = hole->first;
            }
            else
            {
               hole->next = frag->firstHole;
               frag->firstHole = hole->first;
            }

            //Always keep track of the previous hole
            prevHole = hole;
         }

         //Is there still a hole at the end of the segment?
         if(dataLast < holeLast && (offset & IPV6_FLAG_M))
         {
            //Create a new entry that describes this hole
            hole = ipv6FindHole(frag, dataLast);
            hole->first = dataLast;
            hole->last = holeLast;

            //Insert the newly created entry into the hole descriptor list
            if(prevHole != NULL)
            {
               hole->next = prevHole->next;
               prevHole->next = hole->first;
            }
            else
            {
               hole->next = frag->firstHole;
               frag->firstHole = hole->first;
            }

            //Always keep track of the previous hole
            prevHole = hole;
         }
      }
      else
      {
         //The newly arrived fragment does not interact with the current hole
         prevHole = hole;
      }

      //Select the next hole descriptor from the list
      hole = ipv6FindHole(frag, prevHole ? prevHole->next : frag->firstHole);
   }

   //Copy data from the fragment to the reassembly buffer
   netBufferCopy((NetBuffer *) &frag->buffer, frag->unfragPartLength + dataFirst,
      ipPacket, fragHeaderOffset + sizeof(Ipv6FragmentHeader), length);

   //Dump hole descriptor list
   ipv6DumpHoleList(frag);

   //If the hole descriptor list is empty, the reassembly process is now complete
   if(!ipv6FindHole(frag, frag->firstHole))
   {
      //Discard the extra hole descriptor that follows the reconstructed datagram
      error = netBufferSetLength((NetBuffer *) &frag->buffer,
         frag->unfragPartLength + frag->fragPartLength);

      //Check status code
      if(error)
      {
         //Number of failures detected by the IP reassembly algorithm
         IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmFails, 1);
      }
      else
      {
         //Point to the IPv6 header
         Ipv6Header *datagram = netBufferAt((NetBuffer *) &frag->buffer, 0);

         //Fix the Payload Length field
         datagram->payloadLen = htons(frag->unfragPartLength +
            frag->fragPartLength - sizeof(Ipv6Header));

         //Number of IP datagrams successfully reassembled
         IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmOKs, 1);
         IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmOKs, 1);

         //Pass the original IPv6 datagram to the higher protocol layer
         ipv6ProcessPacket(interface, (NetBuffer *) &frag->buffer, 0, ancillary);
      }

      //Release previously allocated memory
      netBufferSetLength((NetBuffer *) &frag->buffer, 0);
   }
}


/**
 * @brief Fragment reassembly timeout handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * handle IPv6 fragment reassembly timeout
 *
 * @param[in] interface Underlying network interface
 **/

void ipv6FragTick(NetInterface *interface)
{
   error_t error;
   uint_t i;
   systime_t time;
   Ipv6HoleDesc *hole;

   //Get current time
   time = osGetSystemTime();

   //Loop through the reassembly queue
   for(i = 0; i < IPV6_MAX_FRAG_DATAGRAMS; i++)
   {
      //Point to the current entry in the reassembly queue
      Ipv6FragDesc *frag = &interface->ipv6Context.fragQueue[i];

      //Make sure the entry is currently in use
      if(frag->buffer.chunkCount > 0)
      {
         //If the timer runs out, the partially-reassembled datagram must be
         //discarded and ICMPv6 Time Exceeded message sent to the source host
         if((time - frag->timestamp) >= IPV6_FRAG_TIME_TO_LIVE)
         {
            //Debug message
            TRACE_INFO("IPv6 fragment reassembly timeout...\r\n");
            //Dump IP header contents for debugging purpose
            ipv6DumpHeader(frag->buffer.chunk[0].address);

            //Number of failures detected by the IP reassembly algorithm
            IP_MIB_INC_COUNTER32(ipv6SystemStats.ipSystemStatsReasmFails, 1);
            IP_MIB_INC_COUNTER32(ipv6IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

            //Point to the first hole descriptor
            hole = ipv6FindHole(frag, frag->firstHole);

            //Make sure the fragment zero has been received
            //before sending an ICMPv6 message
            if(hole != NULL && hole->first > 0)
            {
               //Fix the size of the reconstructed datagram
               error = netBufferSetLength((NetBuffer *) &frag->buffer,
                  frag->unfragPartLength + hole->first);

               //Check status code
               if(!error)
               {
                  //Send an ICMPv6 Time Exceeded message
                  icmpv6SendErrorMessage(interface, ICMPV6_TYPE_TIME_EXCEEDED,
                     ICMPV6_CODE_REASSEMBLY_TIME_EXCEEDED, 0, (NetBuffer *) &frag->buffer, 0);
               }
            }

            //Drop the partially reconstructed datagram
            netBufferSetLength((NetBuffer *) &frag->buffer, 0);
         }
      }
   }
}


/**
 * @brief Search for a matching datagram in the reassembly queue
 * @param[in] interface Underlying network interface
 * @param[in] packet Incoming IPv6 packet
 * @param[in] header Pointer to the Fragment header
 * @return Matching fragment descriptor
 **/

Ipv6FragDesc *ipv6SearchFragQueue(NetInterface *interface,
   Ipv6Header *packet, Ipv6FragmentHeader *header)
{
   error_t error;
   uint_t i;
   Ipv6Header *datagram;
   Ipv6FragDesc *frag;
   Ipv6HoleDesc *hole;

   //Search for a matching IP datagram being reassembled
   for(i = 0; i < IPV6_MAX_FRAG_DATAGRAMS; i++)
   {
      //Point to the current entry in the reassembly queue
      frag = &interface->ipv6Context.fragQueue[i];

      //Check whether the current entry is used?
      if(frag->buffer.chunkCount > 0)
      {
         //Point to the corresponding datagram
         datagram = netBufferAt((NetBuffer *) &frag->buffer, 0);

         //Check source and destination addresses
         if(!ipv6CompAddr(&datagram->srcAddr, &packet->srcAddr))
            continue;
         if(!ipv6CompAddr(&datagram->destAddr, &packet->destAddr))
            continue;
         //Compare fragment identification fields
         if(frag->identification != header->identification)
            continue;

         //A matching entry has been found in the reassembly queue
         return frag;
      }
   }

   //If the current packet does not match an existing entry
   //in the reassembly queue, then create a new entry
   for(i = 0; i < IPV6_MAX_FRAG_DATAGRAMS; i++)
   {
      //Point to the current entry in the reassembly queue
      frag = &interface->ipv6Context.fragQueue[i];

      //The current entry is free?
      if(!frag->buffer.chunkCount)
      {
         //Number of chunks that comprise the reassembly buffer
         frag->buffer.maxChunkCount = arraysize(frag->buffer.chunk);

         //Allocate sufficient memory to hold the IPv6 header and
         //the first hole descriptor
         error = netBufferSetLength((NetBuffer *) &frag->buffer,
            NET_MEM_POOL_BUFFER_SIZE + sizeof(Ipv6HoleDesc));

         //Failed to allocate memory?
         if(error)
         {
            //Clean up side effects
            netBufferSetLength((NetBuffer *) &frag->buffer, 0);
            //Exit immediately
            return NULL;
         }

         //Initial length of the reconstructed datagram
         frag->unfragPartLength = sizeof(Ipv6Header);
         frag->fragPartLength = 0;

         //Fix the length of the first chunk
         frag->buffer.chunk[0].length = (uint16_t) frag->unfragPartLength;
         //Copy IPv6 header from the incoming fragment
         netBufferWrite((NetBuffer *) &frag->buffer, 0, packet, frag->unfragPartLength);

         //Save current time
         frag->timestamp = osGetSystemTime();
         //Record fragment identification field
         frag->identification = header->identification;
         //Create a new entry in the hole descriptor list
         frag->firstHole = 0;

         //Point to first hole descriptor
         hole = ipv6FindHole(frag, frag->firstHole);
         //The entry describes the datagram as being completely missing
         hole->first = 0;
         hole->last = IPV6_INFINITY;
         hole->next = IPV6_INFINITY;

         //Dump hole descriptor list
         ipv6DumpHoleList(frag);

         //Return the matching fragment descriptor
         return frag;
      }
   }

   //The reassembly queue is full
   return NULL;
}


/**
 * @brief Flush IPv6 reassembly queue
 * @param[in] interface Underlying network interface
 **/

void ipv6FlushFragQueue(NetInterface *interface)
{
   uint_t i;

   //Loop through the reassembly queue
   for(i = 0; i < IPV6_MAX_FRAG_DATAGRAMS; i++)
   {
      //Drop any partially reconstructed datagram
      netBufferSetLength((NetBuffer *) &interface->ipv6Context.fragQueue[i].buffer, 0);
   }
}


/**
 * @brief Retrieve hole descriptor
 * @param[in] frag IPv6 fragment descriptor
 * @param[in] offset Offset of the hole
 * @return A pointer to the hole descriptor is returned if the
 *   specified offset is valid. Otherwise NULL is returned
 **/

Ipv6HoleDesc *ipv6FindHole(Ipv6FragDesc *frag, uint16_t offset)
{
   //Return a pointer to the hole descriptor
   return netBufferAt((NetBuffer *) &frag->buffer, frag->unfragPartLength + offset);
}


/**
 * @brief Dump hole descriptor list
 * @param[in] frag IPv6 fragment descriptor
 **/

void ipv6DumpHoleList(Ipv6FragDesc *frag)
{
//Check debugging level
#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   Ipv6HoleDesc *hole;

   //Debug message
   TRACE_DEBUG("Hole descriptor list:\r\n");
   //Select the first hole descriptor from the list
   hole = ipv6FindHole(frag, frag->firstHole);

   //Loop through the hole descriptor list
   while(hole != NULL)
   {
      //Display current hole
      TRACE_DEBUG("  %" PRIu16 " - %" PRIu16 "\r\n", hole->first, hole->last);
      //Select the next hole descriptor from the list
      hole = ipv6FindHole(frag, hole->next);
   }
#endif
}

#endif
