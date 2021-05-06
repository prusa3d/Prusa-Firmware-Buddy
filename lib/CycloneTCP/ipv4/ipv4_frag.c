/**
 * @file ipv4_frag.c
 * @brief IPv4 fragmentation and reassembly
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
 * The Internet Protocol (IP) implements datagram fragmentation, so that
 * packets may be formed that can pass through a link with a smaller maximum
 * transmission unit (MTU) than the original datagram size. Refer to the
 * following RFCs for complete details:
 * - RFC 791: Internet Protocol specification
 * - RFC 815: IP datagram reassembly algorithms
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL IPV4_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "core/ip.h"
#include "ipv4/ipv4.h"
#include "ipv4/ipv4_frag.h"
#include "ipv4/icmp.h"
#include "mibs/mib2_module.h"
#include "mibs/ip_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV4_SUPPORT == ENABLED && IPV4_FRAG_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t ipv4FragTickCounter;


/**
 * @brief Fragment an IPv4 datagram into smaller packets
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv4 pseudo header
 * @param[in] id Fragment identification
 * @param[in] payload Multi-part buffer containing the payload
 * @param[in] payloadOffset Offset to the first payload byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ipv4FragmentDatagram(NetInterface *interface,
   Ipv4PseudoHeader *pseudoHeader, uint16_t id, const NetBuffer *payload,
   size_t payloadOffset, NetTxAncillary *ancillary)
{
   error_t error;
   size_t offset;
   size_t length;
   size_t payloadLen;
   size_t fragmentOffset;
   size_t maxFragmentSize;
   NetBuffer *fragment;

   //Number of IP datagrams that would require fragmentation in order to be
   //transmitted
   IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsOutFragReqds, 1);
   IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsOutFragReqds, 1);

   //Retrieve the length of the payload
   payloadLen = netBufferGetLength(payload) - payloadOffset;

   //Allocate a memory buffer to hold IP fragments
   fragment = ipAllocBuffer(0, &fragmentOffset);
   //Failed to allocate memory?
   if(!fragment)
      return ERROR_OUT_OF_MEMORY;

   //Determine the maximum payload size for fragmented packets
   maxFragmentSize = interface->ipv4Context.linkMtu - sizeof(Ipv4Header);
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
         error = ipv4SendPacket(interface, pseudoHeader, id, offset / 8,
            fragment, fragmentOffset, ancillary);
      }
      else
      {
         //Size of the current fragment (must be a multiple of 8-byte blocks)
         length = maxFragmentSize;
         //Copy fragment data
         netBufferConcat(fragment, payload, payloadOffset + offset, length);

         //Fragmented packets must have the MF flag set
         error = ipv4SendPacket(interface, pseudoHeader, id, IPV4_FLAG_MF |
            (offset / 8), fragment, fragmentOffset, ancillary);
      }

      //Failed to send current IP packet?
      if(error)
         break;

      //Number of IP datagram fragments that have been generated as a result
      //of fragmentation at this entity
      MIB2_INC_COUNTER32(ipGroup.ipFragCreates, 1);
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsOutFragCreates, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsOutFragCreates, 1);
   }

   //Check status code
   if(error)
   {
      //Number of IP datagrams that have been discarded because they needed
      //to be fragmented at this entity but could not be
      MIB2_INC_COUNTER32(ipGroup.ipFragFails, 1);
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsOutFragFails, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsOutFragFails, 1);
   }
   else
   {
      //Number of IP datagrams that have been successfully fragmented at this
      //entity
      MIB2_INC_COUNTER32(ipGroup.ipFragOKs, 1);
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsOutFragOKs, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsOutFragOKs, 1);
   }

   //Free previously allocated memory
   netBufferFree(fragment);
   //Return status code
   return error;
}


/**
 * @brief IPv4 datagram reassembly algorithm
 * @param[in] interface Underlying network interface
 * @param[in] packet Pointer to the IPv4 fragmented packet
 * @param[in] length Packet length including header and payload
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 **/

void ipv4ReassembleDatagram(NetInterface *interface, const Ipv4Header *packet,
   size_t length, NetRxAncillary *ancillary)
{
   error_t error;
   uint16_t offset;
   uint16_t dataFirst;
   uint16_t dataLast;
   Ipv4FragDesc *frag;
   Ipv4HoleDesc *hole;
   Ipv4HoleDesc *prevHole;

   //Number of IP fragments received which needed to be reassembled
   MIB2_INC_COUNTER32(ipGroup.ipReasmReqds, 1);
   IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsReasmReqds, 1);
   IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsReasmReqds, 1);

   //Get the length of the payload
   length -= packet->headerLength * 4;
   //Convert the fragment offset from network byte order
   offset = ntohs(packet->fragmentOffset);

   //Every fragment except the last must contain a multiple of 8 bytes of data
   if((offset & IPV4_FLAG_MF) && (length % 8))
   {
      //Number of failures detected by the IP reassembly algorithm
      MIB2_INC_COUNTER32(ipGroup.ipReasmFails, 1);
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsReasmFails, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

      //Drop the incoming fragment
      return;
   }

   //Calculate the index of the first byte
   dataFirst = (offset & IPV4_OFFSET_MASK) * 8;
   //Calculate the index immediately following the last byte
   dataLast = dataFirst + (uint16_t) length;

   //Search for a matching IP datagram being reassembled
   frag = ipv4SearchFragQueue(interface, packet);

   //No matching entry in the reassembly queue?
   if(frag == NULL)
   {
      //Number of failures detected by the IP reassembly algorithm
      MIB2_INC_COUNTER32(ipGroup.ipReasmFails, 1);
      IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsReasmFails, 1);
      IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

      //Drop the incoming fragment
      return;
   }

   //The very first fragment requires special handling
   if(!(offset & IPV4_OFFSET_MASK))
   {
      //Calculate the length of the IP header including options
      frag->headerLength = packet->headerLength * 4;

      //Enforce the size of the reconstructed datagram
      if((frag->headerLength + frag->dataLen) > IPV4_MAX_FRAG_DATAGRAM_SIZE)
      {
         //Number of failures detected by the IP reassembly algorithm
         MIB2_INC_COUNTER32(ipGroup.ipReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

         //Drop the reconstructed datagram
         netBufferSetLength((NetBuffer *) &frag->buffer, 0);
         //Exit immediately
         return;
      }

      //Make sure the IP header entirely fits in the first chunk
      if(frag->headerLength > frag->buffer.chunk[0].size)
      {
         //Number of failures detected by the IP reassembly algorithm
         MIB2_INC_COUNTER32(ipGroup.ipReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

         //Drop the reconstructed datagram
         netBufferSetLength((NetBuffer *) &frag->buffer, 0);
         //Exit immediately
         return;
      }

      //Fix the length of the first chunk
      frag->buffer.chunk[0].length = (uint16_t) frag->headerLength;
      //Always take the IP header from the first fragment
      netBufferWrite((NetBuffer *) &frag->buffer, 0, packet, frag->headerLength);
   }

   //It may be necessary to increase the size of the buffer...
   if(dataLast > frag->dataLen)
   {
      //Enforce the size of the reconstructed datagram
      if((frag->headerLength + dataLast) > IPV4_MAX_FRAG_DATAGRAM_SIZE)
      {
         //Number of failures detected by the IP reassembly algorithm
         MIB2_INC_COUNTER32(ipGroup.ipReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

         //Drop the reconstructed datagram
         netBufferSetLength((NetBuffer *) &frag->buffer, 0);
         //Exit immediately
         return;
      }

      //Adjust the size of the reconstructed datagram
      error = netBufferSetLength((NetBuffer *) &frag->buffer,
         frag->headerLength + dataLast + sizeof(Ipv4HoleDesc));

      //Any error to report?
      if(error)
      {
         //Number of failures detected by the IP reassembly algorithm
         MIB2_INC_COUNTER32(ipGroup.ipReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

         //Drop the reconstructed datagram
         netBufferSetLength((NetBuffer *) &frag->buffer, 0);
         //Exit immediately
         return;
      }

      //Actual length of the payload
      frag->dataLen = dataLast;
   }

   //Select the first hole descriptor from the list
   hole = ipv4FindHole(frag, frag->firstHole);
   //Keep track of the previous hole in the list
   prevHole = NULL;

   //Iterate through the hole descriptors
   while(hole != NULL)
   {
      //Save lower and upper boundaries for later use
      uint16_t holeFirst = hole->first;
      uint16_t holeLast = hole->last;

      //Check whether the newly arrived fragment interacts with this hole in
      //some way
      if(dataFirst < holeLast && dataLast > holeFirst)
      {
         //The current descriptor is no longer valid. We will destroy it, and
         //in the next two steps, we will determine whether or not it is
         //necessary to create any new hole descriptors
         if(prevHole != NULL)
            prevHole->next = hole->next;
         else
            frag->firstHole = hole->next;

         //Is there still a hole at the beginning of the segment?
         if(dataFirst > holeFirst)
         {
            //Create a new entry that describes this hole
            hole = ipv4FindHole(frag, holeFirst);
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
         if(dataLast < holeLast && (offset & IPV4_FLAG_MF))
         {
            //Create a new entry that describes this hole
            hole = ipv4FindHole(frag, dataLast);
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
      hole = ipv4FindHole(frag, prevHole ? prevHole->next : frag->firstHole);
   }

   //Copy data from the fragment to the reassembly buffer
   netBufferWrite((NetBuffer *) &frag->buffer,
      frag->headerLength + dataFirst, IPV4_DATA(packet), length);

   //Dump hole descriptor list
   ipv4DumpHoleList(frag);

   //If the hole descriptor list is empty, the reassembly process is now
   //complete
   if(!ipv4FindHole(frag, frag->firstHole))
   {
      //Discard the extra hole descriptor that follows the reconstructed
      //datagram
      error = netBufferSetLength((NetBuffer *) &frag->buffer,
         frag->headerLength + frag->dataLen);

      //Check status code
      if(error)
      {
         //Number of failures detected by the IP reassembly algorithm
         MIB2_INC_COUNTER32(ipGroup.ipReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsReasmFails, 1);
         IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsReasmFails, 1);
      }
      else
      {
         //Point to the IP header
         Ipv4Header *datagram = netBufferAt((NetBuffer *) &frag->buffer, 0);

         //Fix IP header
         datagram->totalLength = htons(frag->headerLength + frag->dataLen);
         datagram->fragmentOffset = 0;
         datagram->headerChecksum = 0;

         //Number of IP datagrams successfully reassembled
         MIB2_INC_COUNTER32(ipGroup.ipReasmOKs, 1);
         IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsReasmOKs, 1);
         IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsReasmOKs, 1);

         //Pass the original IPv4 datagram to the higher protocol layer
         ipv4ProcessDatagram(interface, (NetBuffer *) &frag->buffer, ancillary);
      }

      //Release previously allocated memory
      netBufferSetLength((NetBuffer *) &frag->buffer, 0);
   }
}


/**
 * @brief Fragment reassembly timeout handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * handle IPv4 fragment reassembly timeout
 *
 * @param[in] interface Underlying network interface
 **/

void ipv4FragTick(NetInterface *interface)
{
   error_t error;
   uint_t i;
   systime_t time;
   Ipv4HoleDesc *hole;

   //Get current time
   time = osGetSystemTime();

   //Loop through the reassembly queue
   for(i = 0; i < IPV4_MAX_FRAG_DATAGRAMS; i++)
   {
      //Point to the current entry in the reassembly queue
      Ipv4FragDesc *frag = &interface->ipv4Context.fragQueue[i];

      //Make sure the entry is currently in use
      if(frag->buffer.chunkCount > 0)
      {
         //If the timer runs out, the partially-reassembled datagram must be
         //discarded and ICMP Time Exceeded message sent to the source host
         if((time - frag->timestamp) >= IPV4_FRAG_TIME_TO_LIVE)
         {
            //Debug message
            TRACE_INFO("IPv4 fragment reassembly timeout...\r\n");
            //Dump IP header contents for debugging purpose
            ipv4DumpHeader(frag->buffer.chunk[0].address);

            //Number of failures detected by the IP reassembly algorithm
            MIB2_INC_COUNTER32(ipGroup.ipReasmFails, 1);
            IP_MIB_INC_COUNTER32(ipv4SystemStats.ipSystemStatsReasmFails, 1);
            IP_MIB_INC_COUNTER32(ipv4IfStatsTable[interface->index].ipIfStatsReasmFails, 1);

            //Point to the first hole descriptor
            hole = ipv4FindHole(frag, frag->firstHole);

            //Make sure the fragment zero has been received before sending an
            //ICMP message
            if(hole != NULL && hole->first > 0)
            {
               //Fix the size of the reconstructed datagram
               error = netBufferSetLength((NetBuffer *) &frag->buffer,
                  frag->headerLength + hole->first);

               //Check status code
               if(!error)
               {
                  //Send an ICMP Time Exceeded message
                  icmpSendErrorMessage(interface, ICMP_TYPE_TIME_EXCEEDED,
                     ICMP_CODE_REASSEMBLY_TIME_EXCEEDED, 0, (NetBuffer *) &frag->buffer, 0);
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
 * @param[in] packet Incoming IPv4 packet
 * @return Matching fragment descriptor
 **/

Ipv4FragDesc *ipv4SearchFragQueue(NetInterface *interface,
   const Ipv4Header *packet)
{
   error_t error;
   uint_t i;
   Ipv4Header *datagram;
   Ipv4FragDesc *frag;
   Ipv4HoleDesc *hole;

   //Search for a matching IP datagram being reassembled
   for(i = 0; i < IPV4_MAX_FRAG_DATAGRAMS; i++)
   {
      //Point to the current entry in the reassembly queue
      frag = &interface->ipv4Context.fragQueue[i];

      //Check whether the current entry is used?
      if(frag->buffer.chunkCount > 0)
      {
         //Point to the corresponding datagram
         datagram = netBufferAt((NetBuffer *) &frag->buffer, 0);

         //Check source and destination addresses
         if(datagram->srcAddr != packet->srcAddr)
            continue;
         if(datagram->destAddr != packet->destAddr)
            continue;
         //Compare identification and protocol fields
         if(datagram->identification != packet->identification)
            continue;
         if(datagram->protocol != packet->protocol)
            continue;

         //A matching entry has been found in the reassembly queue
         return frag;
      }
   }

   //If the current packet does not match an existing entry in the reassembly
   //queue, then create a new entry
   for(i = 0; i < IPV4_MAX_FRAG_DATAGRAMS; i++)
   {
      //Point to the current entry in the reassembly queue
      frag = &interface->ipv4Context.fragQueue[i];

      //The current entry is free?
      if(!frag->buffer.chunkCount)
      {
         //Number of chunks that comprise the reassembly buffer
         frag->buffer.maxChunkCount = arraysize(frag->buffer.chunk);

         //Allocate sufficient memory to hold the IPv4 header and
         //the first hole descriptor
         error = netBufferSetLength((NetBuffer *) &frag->buffer,
            NET_MEM_POOL_BUFFER_SIZE + sizeof(Ipv4HoleDesc));

         //Failed to allocate memory?
         if(error)
         {
            //Clean up side effects
            netBufferSetLength((NetBuffer *) &frag->buffer, 0);
            //Exit immediately
            return NULL;
         }

         //Initial length of the reconstructed datagram
         frag->headerLength = packet->headerLength * 4;
         frag->dataLen = 0;

         //Fix the length of the first chunk
         frag->buffer.chunk[0].length = (uint16_t) frag->headerLength;
         //Copy IPv4 header from the incoming fragment
         netBufferWrite((NetBuffer *) &frag->buffer, 0, packet, frag->headerLength);

         //Save current time
         frag->timestamp = osGetSystemTime();
         //Create a new entry in the hole descriptor list
         frag->firstHole = 0;

         //Point to first hole descriptor
         hole = ipv4FindHole(frag, frag->firstHole);
         //The entry describes the datagram as being completely missing
         hole->first = 0;
         hole->last = IPV4_INFINITY;
         hole->next = IPV4_INFINITY;

         //Dump hole descriptor list
         ipv4DumpHoleList(frag);

         //Return the matching fragment descriptor
         return frag;
      }
   }

   //The reassembly queue is full
   return NULL;
}


/**
 * @brief Flush IPv4 reassembly queue
 * @param[in] interface Underlying network interface
 **/

void ipv4FlushFragQueue(NetInterface *interface)
{
   uint_t i;

   //Loop through the reassembly queue
   for(i = 0; i < IPV4_MAX_FRAG_DATAGRAMS; i++)
   {
      //Drop any partially reconstructed datagram
      netBufferSetLength((NetBuffer *) &interface->ipv4Context.fragQueue[i].buffer, 0);
   }
}


/**
 * @brief Retrieve hole descriptor
 * @param[in] frag IPv4 fragment descriptor
 * @param[in] offset Offset of the hole
 * @return A pointer to the hole descriptor is returned if the specified
 *   offset is valid. Otherwise NULL is returned
 **/

Ipv4HoleDesc *ipv4FindHole(Ipv4FragDesc *frag, uint16_t offset)
{
   //Return a pointer to the hole descriptor
   return netBufferAt((NetBuffer *) &frag->buffer, frag->headerLength + offset);
}


/**
 * @brief Dump hole descriptor list
 * @param[in] frag IPv4 fragment descriptor
 **/

void ipv4DumpHoleList(Ipv4FragDesc *frag)
{
//Check debugging level
#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   Ipv4HoleDesc *hole;

   //Debug message
   TRACE_DEBUG("Hole descriptor list:\r\n");
   //Select the first hole descriptor from the list
   hole = ipv4FindHole(frag, frag->firstHole);

   //Loop through the hole descriptor list
   while(hole != NULL)
   {
      //Display current hole
      TRACE_DEBUG("  %" PRIu16 " - %" PRIu16 "\r\n", hole->first, hole->last);
      //Select the next hole descriptor from the list
      hole = ipv4FindHole(frag, hole->next);
   }
#endif
}

#endif
