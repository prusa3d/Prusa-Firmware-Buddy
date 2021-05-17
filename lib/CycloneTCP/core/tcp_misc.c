/**
 * @file tcp_misc.c
 * @brief Helper functions for TCP
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
#define TRACE_LEVEL TCP_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include <string.h>
#include "core/net.h"
#include "core/socket.h"
#include "core/tcp.h"
#include "core/tcp_misc.h"
#include "core/tcp_timer.h"
#include "core/ip.h"
#include "ipv4/ipv4.h"
#include "ipv6/ipv6.h"
#include "mibs/mib2_module.h"
#include "mibs/tcp_mib_module.h"
#include "date_time.h"
#include "debug.h"

//Secure initial sequence number generation?
#if (TCP_SECURE_ISN_SUPPORT == ENABLED)
   #include "hash/md5.h"
#endif

//Check TCP/IP stack configuration
#if (TCP_SUPPORT == ENABLED)


/**
 * @brief Send a TCP segment
 * @param[in] socket Handle referencing a socket
 * @param[in] flags Value that contains bitwise OR of flags (see #TcpFlags enumeration)
 * @param[in] seqNum Sequence number
 * @param[in] ackNum Acknowledgment number
 * @param[in] length Length of the segment data
 * @param[in] addToQueue Add the segment to retransmission queue
 * @return Error code
 **/

error_t tcpSendSegment(Socket *socket, uint8_t flags, uint32_t seqNum,
   uint32_t ackNum, size_t length, bool_t addToQueue)
{
   error_t error;
   size_t offset;
   size_t totalLength;
   NetBuffer *buffer;
   TcpHeader *segment;
   TcpQueueItem *queueItem;
   IpPseudoHeader pseudoHeader;
   NetTxAncillary ancillary;

   //Maximum segment size
   uint16_t mss = HTONS(socket->rmss);

   //Allocate a memory buffer to hold the TCP segment
   buffer = ipAllocBuffer(TCP_MAX_HEADER_LENGTH, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the TCP segment
   segment = netBufferAt(buffer, offset);

   //Format TCP header
   segment->srcPort = htons(socket->localPort);
   segment->destPort = htons(socket->remotePort);
   segment->seqNum = htonl(seqNum);
   segment->ackNum = (flags & TCP_FLAG_ACK) ? htonl(ackNum) : 0;
   segment->reserved1 = 0;
   segment->dataOffset = 5;
   segment->flags = flags;
   segment->reserved2 = 0;
   segment->window = htons(socket->rcvWnd);
   segment->checksum = 0;
   segment->urgentPointer = 0;

   //SYN flag set?
   if((flags & TCP_FLAG_SYN) != 0)
   {
      //Append MSS option
      tcpAddOption(segment, TCP_OPTION_MAX_SEGMENT_SIZE, &mss, sizeof(mss));

#if (TCP_SACK_SUPPORT == ENABLED)
      //Append SACK Permitted option
      tcpAddOption(segment, TCP_OPTION_SACK_PERMITTED, NULL, 0);
#endif
   }

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + segment->dataOffset * 4);

   //Any data to send?
   if(length > 0)
   {
      //Copy data
      error = tcpReadTxBuffer(socket, seqNum, buffer, length);
      //Any error to report?
      if(error)
      {
         //Clean up side effects
         netBufferFree(buffer);
         //Exit immediately
         return error;
      }
   }

   //Calculate the length of the complete TCP segment
   totalLength = segment->dataOffset * 4 + length;

#if (IPV4_SUPPORT == ENABLED)
   //Destination address is an IPv4 address?
   if(socket->remoteIpAddr.length == sizeof(Ipv4Addr))
   {
      //Format IPv4 pseudo header
      pseudoHeader.length = sizeof(Ipv4PseudoHeader);
      pseudoHeader.ipv4Data.srcAddr = socket->localIpAddr.ipv4Addr;
      pseudoHeader.ipv4Data.destAddr = socket->remoteIpAddr.ipv4Addr;
      pseudoHeader.ipv4Data.reserved = 0;
      pseudoHeader.ipv4Data.protocol = IPV4_PROTOCOL_TCP;
      pseudoHeader.ipv4Data.length = htons(totalLength);

      //Calculate TCP header checksum
      segment->checksum = ipCalcUpperLayerChecksumEx(&pseudoHeader.ipv4Data,
         sizeof(Ipv4PseudoHeader), buffer, offset, totalLength);
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //Destination address is an IPv6 address?
   if(socket->remoteIpAddr.length == sizeof(Ipv6Addr))
   {
      //Format IPv6 pseudo header
      pseudoHeader.length = sizeof(Ipv6PseudoHeader);
      pseudoHeader.ipv6Data.srcAddr = socket->localIpAddr.ipv6Addr;
      pseudoHeader.ipv6Data.destAddr = socket->remoteIpAddr.ipv6Addr;
      pseudoHeader.ipv6Data.length = htonl(totalLength);
      pseudoHeader.ipv6Data.reserved[0] = 0;
      pseudoHeader.ipv6Data.reserved[1] = 0;
      pseudoHeader.ipv6Data.reserved[2] = 0;
      pseudoHeader.ipv6Data.nextHeader = IPV6_TCP_HEADER;

      //Calculate TCP header checksum
      segment->checksum = ipCalcUpperLayerChecksumEx(&pseudoHeader.ipv6Data,
         sizeof(Ipv6PseudoHeader), buffer, offset, totalLength);
   }
   else
#endif
   //Destination address is not valid?
   {
      //Free previously allocated memory
      netBufferFree(buffer);
      //This should never occur...
      return ERROR_INVALID_ADDRESS;
   }

   //Add current segment to retransmission queue?
   if(addToQueue)
   {
      //Empty retransmission queue?
      if(socket->retransmitQueue == NULL)
      {
         //Create a new item
         queueItem = memPoolAlloc(sizeof(TcpQueueItem));
         //Add the newly created item to the queue
         socket->retransmitQueue = queueItem;
      }
      else
      {
         //Point to the very first item
         queueItem = socket->retransmitQueue;
         //Reach the last item of the retransmission queue
         while(queueItem->next) queueItem = queueItem->next;
         //Create a new item
         queueItem->next = memPoolAlloc(sizeof(TcpQueueItem));
         //Point to the newly created item
         queueItem = queueItem->next;
      }

      //Failed to allocate memory?
      if(queueItem == NULL)
      {
         //Free previously allocated memory
         netBufferFree(buffer);
         //Return status
         return ERROR_OUT_OF_MEMORY;
      }

      //Retransmission mechanism requires additional information
      queueItem->next = NULL;
      queueItem->length = length;
      queueItem->sacked = FALSE;
      //Save TCP header
      osMemcpy(queueItem->header, segment, segment->dataOffset * 4);
      //Save pseudo header
      queueItem->pseudoHeader = pseudoHeader;

      //Take one RTT measurement at a time
      if(!socket->rttBusy)
      {
         //Save round-trip start time
         socket->rttStartTime = osGetSystemTime();
         //Record current sequence number
         socket->rttSeqNum = ntohl(segment->seqNum);
         //Wait for an acknowledgment that covers that sequence number...
         socket->rttBusy = TRUE;

#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
         //Reset the byte counter
         socket->n = 0;
#endif
      }

      //Check whether the RTO timer is running or not
      if(!netTimerRunning(&socket->retransmitTimer))
      {
         //If the timer is not running, start it running so that it will expire
         //after RTO seconds
         netStartTimer(&socket->retransmitTimer, socket->rto);

         //Reset retransmission counter
         socket->retransmitCount = 0;
      }
   }

#if (TCP_KEEP_ALIVE_SUPPORT == ENABLED)
   //Check whether TCP keep-alive mechanism is enabled
   if(socket->keepAliveEnabled)
   {
      //Idle condition?
      if(socket->keepAliveProbeCount == 0)
      {
         //SYN or data packet?
         if((flags & TCP_FLAG_SYN) != 0 || length > 0)
         {
            //Restart keep-alive timer
            socket->keepAliveTimestamp = osGetSystemTime();
         }
      }
   }
#endif

   //Total number of segments sent
   MIB2_INC_COUNTER32(tcpGroup.tcpOutSegs, 1);
   TCP_MIB_INC_COUNTER32(tcpOutSegs, 1);
   TCP_MIB_INC_COUNTER64(tcpHCOutSegs, 1);

   //RST flag set?
   if((flags & TCP_FLAG_RST) != 0)
   {
      //Number of TCP segments sent containing the RST flag
      MIB2_INC_COUNTER32(tcpGroup.tcpOutRsts, 1);
      TCP_MIB_INC_COUNTER32(tcpOutRsts, 1);
   }

   //Debug message
   TRACE_DEBUG("%s: Sending TCP segment (%" PRIuSIZE " data bytes)...\r\n",
      formatSystemTime(osGetSystemTime(), NULL), length);

   //Dump TCP header contents for debugging purpose
   tcpDumpHeader(segment, length, socket->iss, socket->irs);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;
   //Set the TTL value to be used
   ancillary.ttl = socket->ttl;

#if (ETH_VLAN_SUPPORT == ENABLED)
   //Set VLAN PCP and DEI fields
   ancillary.vlanPcp = socket->vlanPcp;
   ancillary.vlanDei = socket->vlanDei;
#endif

#if (ETH_VMAN_SUPPORT == ENABLED)
   //Set VMAN PCP and DEI fields
   ancillary.vmanPcp = socket->vmanPcp;
   ancillary.vmanDei = socket->vmanDei;
#endif

   //Send TCP segment
   error = ipSendDatagram(socket->interface, &pseudoHeader, buffer, offset,
      &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);

   //Return error code
   return error;
}


/**
 * @brief Send a TCP reset segment
 * @param[in] socket Handle referencing a socket
 * @param[in] seqNum Sequence number
 * @return Error code
 **/

error_t tcpSendResetSegment(Socket *socket, uint32_t seqNum)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

   //Check current state
   if(socket->state == TCP_STATE_SYN_RECEIVED ||
      socket->state == TCP_STATE_ESTABLISHED ||
      socket->state == TCP_STATE_FIN_WAIT_1 ||
      socket->state == TCP_STATE_FIN_WAIT_2 ||
      socket->state == TCP_STATE_CLOSE_WAIT)
   {
      //Send a reset segment
      error = tcpSendSegment(socket, TCP_FLAG_RST, seqNum, 0, 0, FALSE);
   }

   //Return status code
   return error;
}


/**
 * @brief Send a TCP reset in response to an invalid segment
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader TCP pseudo header describing the incoming segment
 * @param[in] segment Incoming TCP segment
 * @param[in] length Length of the incoming segment data
 * @return Error code
 **/

error_t tcpRejectSegment(NetInterface *interface, IpPseudoHeader *pseudoHeader,
   TcpHeader *segment, size_t length)
{
   error_t error;
   size_t offset;
   uint8_t flags;
   uint32_t seqNum;
   uint32_t ackNum;
   NetBuffer *buffer;
   TcpHeader *segment2;
   IpPseudoHeader pseudoHeader2;
   NetTxAncillary ancillary;

   //Check whether the ACK bit is set
   if((segment->flags & TCP_FLAG_ACK) != 0)
   {
      //If the incoming segment has an ACK field, the reset takes
      //its sequence number from the ACK field of the segment
      flags = TCP_FLAG_RST;
      seqNum = segment->ackNum;
      ackNum = 0;
   }
   else
   {
      //Otherwise the reset has sequence number zero and the ACK field is set to
      //the sum of the sequence number and segment length of the incoming segment
      flags = TCP_FLAG_RST | TCP_FLAG_ACK;
      seqNum = 0;
      ackNum = segment->seqNum + length;

      //Advance the acknowledgment number over the SYN or the FIN
      if((segment->flags & TCP_FLAG_SYN) != 0)
      {
         ackNum++;
      }

      if((segment->flags & TCP_FLAG_FIN) != 0)
      {
         ackNum++;
      }
   }

   //Allocate a memory buffer to hold the reset segment
   buffer = ipAllocBuffer(sizeof(TcpHeader), &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the TCP segment
   segment2 = netBufferAt(buffer, offset);

   //Format TCP header
   segment2->srcPort = htons(segment->destPort);
   segment2->destPort = htons(segment->srcPort);
   segment2->seqNum = htonl(seqNum);
   segment2->ackNum = htonl(ackNum);
   segment2->reserved1 = 0;
   segment2->dataOffset = 5;
   segment2->flags = flags;
   segment2->reserved2 = 0;
   segment2->window = 0;
   segment2->checksum = 0;
   segment2->urgentPointer = 0;

#if (IPV4_SUPPORT == ENABLED)
   //Destination address is an IPv4 address?
   if(pseudoHeader->length == sizeof(Ipv4PseudoHeader))
   {
      //Format IPv4 pseudo header
      pseudoHeader2.length = sizeof(Ipv4PseudoHeader);
      pseudoHeader2.ipv4Data.srcAddr = pseudoHeader->ipv4Data.destAddr;
      pseudoHeader2.ipv4Data.destAddr = pseudoHeader->ipv4Data.srcAddr;
      pseudoHeader2.ipv4Data.reserved = 0;
      pseudoHeader2.ipv4Data.protocol = IPV4_PROTOCOL_TCP;
      pseudoHeader2.ipv4Data.length = HTONS(sizeof(TcpHeader));

      //Calculate TCP header checksum
      segment2->checksum = ipCalcUpperLayerChecksumEx(&pseudoHeader2.ipv4Data,
         sizeof(Ipv4PseudoHeader), buffer, offset, sizeof(TcpHeader));
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //Destination address is an IPv6 address?
   if(pseudoHeader->length == sizeof(Ipv6PseudoHeader))
   {
      //Format IPv6 pseudo header
      pseudoHeader2.length = sizeof(Ipv6PseudoHeader);
      pseudoHeader2.ipv6Data.srcAddr = pseudoHeader->ipv6Data.destAddr;
      pseudoHeader2.ipv6Data.destAddr = pseudoHeader->ipv6Data.srcAddr;
      pseudoHeader2.ipv6Data.length = HTONL(sizeof(TcpHeader));
      pseudoHeader2.ipv6Data.reserved[0] = 0;
      pseudoHeader2.ipv6Data.reserved[1] = 0;
      pseudoHeader2.ipv6Data.reserved[2] = 0;
      pseudoHeader2.ipv6Data.nextHeader = IPV6_TCP_HEADER;

      //Calculate TCP header checksum
      segment2->checksum = ipCalcUpperLayerChecksumEx(&pseudoHeader2.ipv6Data,
         sizeof(Ipv6PseudoHeader), buffer, offset, sizeof(TcpHeader));
   }
   else
#endif
   //Destination address is not valid?
   {
      //Free previously allocated memory
      netBufferFree(buffer);
      //This should never occur...
      return ERROR_INVALID_ADDRESS;
   }

   //Total number of segments sent
   MIB2_INC_COUNTER32(tcpGroup.tcpOutSegs, 1);
   TCP_MIB_INC_COUNTER32(tcpOutSegs, 1);
   TCP_MIB_INC_COUNTER64(tcpHCOutSegs, 1);

   //Number of TCP segments sent containing the RST flag
   MIB2_INC_COUNTER32(tcpGroup.tcpOutRsts, 1);
   TCP_MIB_INC_COUNTER32(tcpOutRsts, 1);

   //Debug message
   TRACE_DEBUG("%s: Sending TCP reset segment...\r\n",
      formatSystemTime(osGetSystemTime(), NULL));

   //Dump TCP header contents for debugging purpose
   tcpDumpHeader(segment2, length, 0, 0);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //Send TCP segment
   error = ipSendDatagram(interface, &pseudoHeader2, buffer, offset,
      &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);

   //Return error code
   return error;
}


/**
 * @brief Append an option to a TCP segment
 * @param[in] segment Pointer to the TCP header
 * @param[in] kind Option code
 * @param[in] value Option value
 * @param[in] length Length of the option value, in bytes
 * @return Error code
 **/

error_t tcpAddOption(TcpHeader *segment, uint8_t kind, const void *value,
   uint8_t length)
{
   error_t error;
   size_t i;
   size_t paddingSize;
   TcpOption *option;

   //The option-length counts the two octets of option-kind and option-length
   //as well as the option-data octets (refer to RFC 793, section 3.1)
   length += sizeof(TcpOption);

   //Make sure there is enough room to add the option
   if((segment->dataOffset * 4 + length) <= TCP_MAX_HEADER_LENGTH)
   {
      //Index of the first available byte
      i = (segment->dataOffset * 4) - sizeof(TcpHeader);

      //Calculate the number of padding bytes
      paddingSize = (length % 4) ? 4 - (length % 4) : 0;

      //Write padding bytes
      while(paddingSize--)
      {
         segment->options[i++] = TCP_OPTION_NOP;
      }

      //Point to the current location
      option = (TcpOption *) (segment->options + i);

      //Format option
      option->kind = kind;
      option->length = length;
      osMemcpy(option->value, value, length - sizeof(TcpOption));

      //Adjust index value
      i += length;

      //Update the length of the TCP header
      segment->dataOffset = (sizeof(TcpHeader) + i) / 4;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //Report an error
      error = ERROR_FAILURE;
   }

   //Return status code
   return error;
}


/**
 * @brief Find a specified option in a TCP segment
 * @param[in] segment Pointer to the TCP header
 * @param[in] kind Code of the option to find
 * @return If the specified option is found, a pointer to the corresponding
 *   option is returned. Otherwise NULL pointer is returned
 **/

TcpOption *tcpGetOption(TcpHeader *segment, uint8_t kind)
{
   size_t i;
   size_t length;
   TcpOption *option;

   //Make sure the TCP header is valid
   if(segment->dataOffset >= (sizeof(TcpHeader) / 4))
   {
      //Compute the length of the options field
      length = (segment->dataOffset * 4) - sizeof(TcpHeader);

      //Point to the very first option
      i = 0;

      //Loop through the list of options
      while(i < length)
      {
         //Point to the current option
         option = (TcpOption *) (segment->options + i);

         //Check option code
         if(option->kind == TCP_OPTION_END)
         {
            //This option code indicates the end of the option list
            break;
         }
         else if(option->kind == TCP_OPTION_NOP)
         {
            //This option consists of a single octet
            i++;
         }
         else
         {
            //The option code is followed by a one-byte length field
            if((i + 1) >= length)
               break;

            //Check the length of the option
            if(option->length < sizeof(TcpOption) || (i + option->length) > length)
               break;

            //Matching option code?
            if(option->kind == kind)
               return option;

            //Jump to the next option
            i += option->length;
         }
      }
   }

   //The specified option code does not exist
   return NULL;
}


/**
 * @brief Initial sequence number generation
 * @param[in] localIpAddr Local IP address
 * @param[in] localPort Local port
 * @param[in] remoteIpAddr Remote IP address
 * @param[in] remotePort Remote port
 * @return Value of the initial sequence number
 **/

uint32_t tcpGenerateInitialSeqNum(const IpAddr *localIpAddr,
   uint16_t localPort, const IpAddr *remoteIpAddr, uint16_t remotePort)
{
#if (TCP_SECURE_ISN_SUPPORT == ENABLED)
   uint32_t isn;
   Md5Context md5Context;

   //Generate the initial sequence number as per RFC 6528
   md5Init(&md5Context);
   md5Update(&md5Context, localIpAddr, sizeof(IpAddr));
   md5Update(&md5Context, &localPort, sizeof(uint16_t));
   md5Update(&md5Context, remoteIpAddr, sizeof(IpAddr));
   md5Update(&md5Context, &remotePort, sizeof(uint16_t));
   md5Update(&md5Context, netContext.randSeed, NET_RAND_SEED_SIZE);
   md5Final(&md5Context, NULL);

   //Extract the first 32 bits from the digest value
   isn = LOAD32BE(md5Context.digest);

   //Calculate ISN = M + F(localip, localport, remoteip, remoteport, secretkey)
   return isn + netGetSystemTickCount();
#else
   //Generate a random initial sequence number
   return netGetRand();
#endif
}


/**
 * @brief Test the sequence number of an incoming segment
 * @param[in] socket Handle referencing the current socket
 * @param[in] segment Pointer to the TCP segment to check
 * @param[in] length Length of the segment data
 * @return NO_ERROR if the incoming segment is acceptable, ERROR_FAILURE otherwise
 **/

error_t tcpCheckSeqNum(Socket *socket, TcpHeader *segment, size_t length)
{
   bool_t acceptable;

   //Due to zero windows and zero length segments, we have four cases for the
   //acceptability of an incoming segment (refer to RFC 793, section 3.3)
   if(length == 0 && socket->rcvWnd == 0)
   {
      //If both segment length and receive window are zero, then test if
      //SEG.SEQ = RCV.NXT
      if(segment->seqNum == socket->rcvNxt)
      {
         acceptable = TRUE;
      }
      else
      {
         acceptable = FALSE;
      }
   }
   else if(length == 0 && socket->rcvWnd != 0)
   {
      //If segment length is zero and receive window is non zero, then test if
      //RCV.NXT <= SEG.SEQ < RCV.NXT+RCV.WND
      if(TCP_CMP_SEQ(segment->seqNum, socket->rcvNxt) >= 0 &&
         TCP_CMP_SEQ(segment->seqNum, socket->rcvNxt + socket->rcvWnd) < 0)
      {
         acceptable = TRUE;
      }
      else
      {
         acceptable = FALSE;
      }
   }
   else if(length != 0 && socket->rcvWnd == 0)
   {
      //If segment length is non zero and receive window is zero, then the
      //sequence number is not acceptable
      acceptable = FALSE;
   }
   else
   {
      //If both segment length and receive window are non zero, then test if
      //RCV.NXT <= SEG.SEQ < RCV.NXT+RCV.WND or
      //RCV.NXT <= SEG.SEQ+SEG.LEN-1 < RCV.NXT+RCV.WND
      if(TCP_CMP_SEQ(segment->seqNum, socket->rcvNxt) >= 0 &&
         TCP_CMP_SEQ(segment->seqNum, socket->rcvNxt + socket->rcvWnd) < 0)
      {
         acceptable = TRUE;
      }
      else if(TCP_CMP_SEQ(segment->seqNum + length - 1, socket->rcvNxt) >= 0 &&
         TCP_CMP_SEQ(segment->seqNum + length - 1, socket->rcvNxt + socket->rcvWnd) < 0)
      {
         acceptable = TRUE;
      }
      else
      {
         acceptable = FALSE;
      }
   }

   //Non acceptable sequence number?
   if(!acceptable)
   {
      //Debug message
      TRACE_WARNING("Sequence number is not acceptable!\r\n");

      //If an incoming segment is not acceptable, an acknowledgment should
      //be sent in reply (unless the RST bit is set)
      if((segment->flags & TCP_FLAG_RST) == 0)
      {
         tcpSendSegment(socket, TCP_FLAG_ACK, socket->sndNxt, socket->rcvNxt,
            0, FALSE);
      }

      //Return status code
      return ERROR_FAILURE;
   }

   //Sequence number is acceptable
   return NO_ERROR;
}


/**
 * @brief Check the SYN bit of an incoming segment
 * @param[in] socket Handle referencing the current socket
 * @param[in] segment Pointer to the TCP segment to check
 * @param[in] length Length of the segment data
 * @return ERROR_FAILURE if the SYN is in the window, NO_ERROR otherwise
 **/

error_t tcpCheckSyn(Socket *socket, TcpHeader *segment, size_t length)
{
   //Check whether the SYN bit is set
   if((segment->flags & TCP_FLAG_SYN) != 0)
   {
      //If this step is reached, the SYN is in the window. It is an error
      //and a reset shall be sent in response
      if((segment->flags & TCP_FLAG_ACK) != 0)
      {
         tcpSendResetSegment(socket, segment->ackNum);
      }
      else
      {
         tcpSendSegment(socket, TCP_FLAG_RST | TCP_FLAG_ACK, 0,
            segment->seqNum + length + 1, 0, FALSE);
      }

      //Return immediately
      return ERROR_FAILURE;
   }

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Test the ACK field of an incoming segment
 * @param[in] socket Handle referencing the current socket
 * @param[in] segment Pointer to the TCP segment to check
 * @param[in] length Length of the segment data
 * @return NO_ERROR if the acknowledgment is acceptable, ERROR_FAILURE otherwise
 **/

error_t tcpCheckAck(Socket *socket, TcpHeader *segment, size_t length)
{
   uint_t n;
   uint_t ownd;
   uint_t thresh;
   bool_t duplicateFlag;
   bool_t updateFlag;

   //If the ACK bit is off drop the segment and return
   if((segment->flags & TCP_FLAG_ACK) == 0)
      return ERROR_FAILURE;

#if (TCP_KEEP_ALIVE_SUPPORT == ENABLED)
   //Check whether TCP keep-alive mechanism is enabled
   if(socket->keepAliveEnabled)
   {
      //Reset keep-alive probe counter
      socket->keepAliveProbeCount = 0;
   }
#endif

   //Test the case where SEG.ACK < SND.UNA
   if(TCP_CMP_SEQ(segment->ackNum, socket->sndUna) < 0)
   {
      //An old duplicate ACK has been received
      return NO_ERROR;
   }
   //Test the case where SEG.ACK > SND.NXT
   else if(TCP_CMP_SEQ(segment->ackNum, socket->sndNxt) > 0)
   {
      //Send an ACK segment indicating the current send sequence number
      //and the acknowledgment number expected to be received
      tcpSendSegment(socket, TCP_FLAG_ACK, socket->sndNxt, socket->rcvNxt, 0,
         FALSE);

      //The ACK segment acknowledges something not yet sent
      return ERROR_FAILURE;
   }

   //Check whether the ACK is a duplicate
   duplicateFlag = tcpIsDuplicateAck(socket, segment, length);

   //The send window should be updated
   tcpUpdateSendWindow(socket, segment);

   //The incoming ACK segment acknowledges new data?
   if(TCP_CMP_SEQ(segment->ackNum, socket->sndUna) > 0)
   {
#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
      //Compute the number of bytes acknowledged by the incoming ACK
      n = segment->ackNum - socket->sndUna;

      //Check whether the ACK segment acknowledges our SYN
      if(socket->sndUna == socket->iss)
      {
         n--;
      }

      //Total number of bytes acknowledged during the whole round-trip
      socket->n += n;
#endif
      //Update SND.UNA pointer
      socket->sndUna = segment->ackNum;

      //Compute retransmission timeout
      updateFlag = tcpComputeRto(socket);

      //Any segments on the retransmission queue which are thereby
      //entirely acknowledged are removed
      tcpUpdateRetransmitQueue(socket);

#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
      //Check congestion state
      if(socket->congestState == TCP_CONGEST_STATE_RECOVERY)
      {
         //Invoke fast recovery (refer to RFC 6582)
         tcpFastRecovery(socket, segment, n);
      }
      else
      {
         //Reset duplicate ACK counter
         socket->dupAckCount = 0;

         //Check congestion state
         if(socket->congestState == TCP_CONGEST_STATE_LOSS_RECOVERY)
         {
            //Invoke fast loss recovery
            tcpFastLossRecovery(socket, segment);
         }

         //Slow start algorithm is used when cwnd is lower than ssthresh
         if(socket->cwnd < socket->ssthresh)
         {
            //During slow start, TCP increments cwnd by at most SMSS bytes
            //for each ACK received that cumulatively acknowledges new data
            socket->cwnd += MIN(n, socket->smss);
         }
         //Congestion avoidance algorithm is used when cwnd exceeds ssthres
         else
         {
            //Congestion window is updated once per RTT
            if(updateFlag)
            {
               //TCP must not increment cwnd by more than SMSS bytes
               socket->cwnd += MIN(socket->n, socket->smss);
            }
         }
      }

      //Limit the size of the congestion window
      socket->cwnd = MIN(socket->cwnd, socket->txBufferSize);
#endif
   }
   //The incoming ACK segment does not acknowledge new data?
   else
   {
#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
      //Check whether the acknowledgment is a duplicate
      if(duplicateFlag)
      {
         //Increment duplicate ACK counter
         socket->dupAckCount++;
         //Debug message
         TRACE_INFO("TCP duplicate ACK #%u\r\n", socket->dupAckCount);
      }
      else
      {
         //Reset duplicate ACK counter
         socket->dupAckCount = 0;
      }

      //Check congestion state
      if(socket->congestState == TCP_CONGEST_STATE_IDLE)
      {
         //Use default duplicate ACK threshold
         thresh = TCP_FAST_RETRANSMIT_THRES;
         //Amount of data sent but not yet acknowledged
         ownd = socket->sndNxt - socket->sndUna;

         //Test if there is either no unsent data ready for transmission at
         //the sender, or the advertised receive window does not permit new
         //segments to be transmitted (refer to RFC 5827 section 3.1)
         if(socket->sndUser == 0 || socket->sndWnd <= (socket->sndNxt - socket->sndUna))
         {
            //Compute the duplicate ACK threshold used to trigger a
            //retransmission
            if(ownd <= (3 * socket->smss))
            {
               thresh = 1;
            }
            else if(ownd <= (4 * socket->smss))
            {
               thresh = 2;
            }
            else
            {
            }
         }

         //Check the number of duplicate ACKs that have been received
         if(socket->dupAckCount >= thresh)
         {
            //The TCP sender first checks the value of recover to see if the
            //cumulative acknowledgment field covers more than recover
            if(TCP_CMP_SEQ(segment->ackNum, socket->recover + 1) > 0)
            {
               //Invoke Fast Retransmit (refer to RFC 6582)
               tcpFastRetransmit(socket);
            }
            else
            {
               //If not, the TCP does not enter fast retransmit and does not
               //reset ssthres...
            }
         }
      }
      else if(socket->congestState == TCP_CONGEST_STATE_RECOVERY)
      {
         //Duplicate ACK received?
         if(duplicateFlag)
         {
            //For each additional duplicate ACK received (after the third),
            //cwnd must be incremented by SMSS. This artificially inflates
            //the congestion window in order to reflect the additional
            //segment that has left the network
            socket->cwnd += socket->smss;
         }
      }

      //Limit the size of the congestion window
      socket->cwnd = MIN(socket->cwnd, socket->txBufferSize);
#endif
   }

   //Update TX events
   tcpUpdateEvents(socket);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Test whether the incoming SYN segment is a duplicate
 * @param[in] socket Handle referencing the current socket
 * @param[in] pseudoHeader TCP pseudo header
 * @param[in] segment Pointer to the TCP segment to check
 * @return TRUE if the SYN segment is duplicate, else FALSE
 **/

bool_t tcpIsDuplicateSyn(Socket *socket, IpPseudoHeader *pseudoHeader,
   TcpHeader *segment)
{
   bool_t flag;
   TcpSynQueueItem *queueItem;

   //Initialize flag
   flag = FALSE;

   //Point to the very first item
   queueItem = socket->synQueue;

   //Loop through the SYN queue
   while(queueItem != NULL)
   {
#if (IPV4_SUPPORT == ENABLED)
      //IPv4 packet received?
      if(queueItem->srcAddr.length == sizeof(Ipv4Addr) &&
         queueItem->destAddr.length == sizeof(Ipv4Addr) &&
         pseudoHeader->length == sizeof(Ipv4PseudoHeader))
      {
         //Check source and destination addresses
         if(queueItem->srcAddr.ipv4Addr == pseudoHeader->ipv4Data.srcAddr &&
            queueItem->destAddr.ipv4Addr == pseudoHeader->ipv4Data.destAddr)
         {
            //Check source port
            if(queueItem->srcPort == segment->srcPort)
            {
               //Duplicate SYN
               flag = TRUE;
            }
         }
      }
      else
#endif
#if (IPV6_SUPPORT == ENABLED)
      //IPv6 packet received?
      if(queueItem->srcAddr.length == sizeof(Ipv6Addr) &&
         queueItem->destAddr.length == sizeof(Ipv6Addr) &&
         pseudoHeader->length == sizeof(Ipv6PseudoHeader))
      {
         //Check source and destination addresses
         if(ipv6CompAddr(&queueItem->srcAddr.ipv6Addr, &pseudoHeader->ipv6Data.srcAddr) &&
            ipv6CompAddr(&queueItem->destAddr.ipv6Addr, &pseudoHeader->ipv6Data.destAddr))
         {
            //Check source port
            if(queueItem->srcPort == segment->srcPort)
            {
               //Duplicate SYN
               flag = TRUE;
            }
         }
      }
      else
#endif
      {
         //Just for sanity
      }

      //Next item
      queueItem = queueItem->next;
   }

   //Return TRUE if the SYN segment is a duplicate
   return flag;
}


/**
 * @brief Test whether the incoming acknowledgment is a duplicate
 * @param[in] socket Handle referencing the current socket
 * @param[in] segment Pointer to the TCP segment to check
 * @param[in] length Length of the segment data
 * @return TRUE if the ACK is duplicate, else FALSE
 **/

bool_t tcpIsDuplicateAck(Socket *socket, TcpHeader *segment, size_t length)
{
   bool_t flag;

   //An ACK is considered a duplicate when the following conditions are met
   flag = FALSE;

   //The receiver of the ACK has outstanding data
   if(socket->retransmitQueue != NULL)
   {
      //The incoming acknowledgment carries no data
      if(length == 0)
      {
         //The SYN and FIN bits are both off
         if((segment->flags & (TCP_FLAG_SYN | TCP_FLAG_FIN)) == 0)
         {
            //The acknowledgment number is equal to the greatest acknowledgment
            //received on the given connection
            if(segment->ackNum == socket->sndUna)
            {
               //The advertised window in the incoming acknowledgment equals
               //the advertised window in the last incoming acknowledgment
               if(segment->window == socket->sndWnd)
               {
                  //Duplicate ACK
                  flag = TRUE;
               }
            }
         }
      }
   }

   //Return TRUE if the acknowledgment is a duplicate
   return flag;
}


/**
 * @brief Fast retransmit procedure
 * @param[in] socket Handle referencing the current socket
 **/

void tcpFastRetransmit(Socket *socket)
{
#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
   uint32_t flightSize;

   //Amount of data that has been sent but not yet acknowledged
   flightSize = socket->sndNxt - socket->sndUna;
   //After receiving 3 duplicate ACKs, ssthresh must be adjusted
   socket->ssthresh = MAX(flightSize / 2, 2 * socket->smss);

   //The value of recover is incremented to the value of the highest
   //sequence number transmitted by the TCP so far
   socket->recover = socket->sndNxt - 1;

   //Debug message
   TRACE_INFO("TCP fast retransmit...\r\n");

   //TCP performs a retransmission of what appears to be the missing segment,
   //without waiting for the retransmission timer to expire
   tcpRetransmitSegment(socket);

   //cwnd must set to ssthresh plus 3*SMSS. This artificially inflates the
   //congestion window by the number of segments (three) that have left the
   //network and which the receiver has buffered
   socket->cwnd = socket->ssthresh + TCP_FAST_RETRANSMIT_THRES * socket->smss;

   //Enter the fast recovery procedure
   socket->congestState = TCP_CONGEST_STATE_RECOVERY;
#endif
}


/**
 * @brief Fast recovery procedure
 * @param[in] socket Handle referencing the current socket
 * @param[in] segment Pointer to the incoming TCP segment
 * @param[in] n Number of bytes acknowledged by the incoming ACK
 **/

void tcpFastRecovery(Socket *socket, TcpHeader *segment, uint_t n)
{
#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
   //Check whether this ACK acknowledges all of the data up to and including
   //recover
   if(TCP_CMP_SEQ(segment->ackNum, socket->recover) > 0)
   {
      //This is a full acknowledgment
      TRACE_INFO("TCP full acknowledgment\r\n");

      //Set cwnd to ssthresh
      socket->cwnd = socket->ssthresh;
      //Exit the fast recovery procedure
      socket->congestState = TCP_CONGEST_STATE_IDLE;
   }
   else
   {
      //If this ACK does not acknowledge all of the data up to and including
      //recover, then this is a partial ACK
      TRACE_INFO("TCP partial acknowledgment\r\n");

      //Retransmit the first unacknowledged segment
      tcpRetransmitSegment(socket);

      //Deflate the congestion window by the amount of new data acknowledged
      //by the cumulative acknowledgment field
      if(socket->cwnd > n)
         socket->cwnd -= n;

      //If the partial ACK acknowledges at least one SMSS of new data, then
      //add back SMSS bytes to the congestion window. This artificially
      //inflates the congestion window in order to reflect the additional
      //segment that has left the network
      if(n >= socket->smss)
         socket->cwnd += socket->smss;

      //Do not exit the fast recovery procedure...
      socket->congestState = TCP_CONGEST_STATE_RECOVERY;
   }
#endif
}


/**
 * @brief Fast loss recovery procedure
 * @param[in] socket Handle referencing the current socket
 * @param[in] segment Pointer to the incoming TCP segment
 **/

void tcpFastLossRecovery(Socket *socket, TcpHeader *segment)
{
#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
   //Check whether this ACK acknowledges all of the data up to and
   //including recover
   if(TCP_CMP_SEQ(segment->ackNum, socket->recover) > 0)
   {
      //This is a full acknowledgment
      TRACE_INFO("TCP full acknowledgment\r\n");

      //Exit the fast loss recovery procedure
      socket->congestState = TCP_CONGEST_STATE_IDLE;
   }
   else
   {
      //If this ACK does not acknowledge all of the data up to and including
      //recover, then this is a partial ACK
      TRACE_INFO("TCP partial acknowledgment\r\n");

      //Retransmit the first unacknowledged segment
      tcpRetransmitSegment(socket);

      //Do not exit the fast loss recovery procedure...
      socket->congestState = TCP_CONGEST_STATE_LOSS_RECOVERY;
   }
#endif
}


/**
 * @brief Process the segment text
 * @param[in] socket Handle referencing the current socket
 * @param[in] segment Pointer to the TCP header
 * @param[in] buffer Multi-part buffer containing the incoming TCP segment
 * @param[in] offset Offset to the first data byte
 * @param[in] length Length of the segment data
 **/

void tcpProcessSegmentData(Socket *socket, TcpHeader *segment,
   const NetBuffer *buffer, size_t offset, size_t length)
{
   uint32_t leftEdge;
   uint32_t rightEdge;

   //First sequence number occupied by the incoming segment
   leftEdge = segment->seqNum;
   //Sequence number immediately following the incoming segment
   rightEdge = segment->seqNum + length;

   //Check whether some data falls outside the receive window
   if(TCP_CMP_SEQ(leftEdge, socket->rcvNxt) < 0)
   {
      //Position of the first byte to be read
      offset += socket->rcvNxt - leftEdge;
      //Ignore the data that falls outside the receive window
      leftEdge = socket->rcvNxt;
   }
   if(TCP_CMP_SEQ(rightEdge, socket->rcvNxt + socket->rcvWnd) > 0)
   {
      //Ignore the data that falls outside the receive window
      rightEdge = socket->rcvNxt + socket->rcvWnd;
   }

   //Copy the incoming data to the receive buffer
   tcpWriteRxBuffer(socket, leftEdge, buffer, offset, rightEdge - leftEdge);

   //Update the list of non-contiguous blocks of data that
   //have been received and queued
   tcpUpdateSackBlocks(socket, &leftEdge, &rightEdge);

   //Check whether the segment was received out of order
   if(TCP_CMP_SEQ(leftEdge, socket->rcvNxt) > 0)
   {
      //Out of order data segments should be acknowledged immediately, in
      //order to accelerate loss recovery
      tcpSendSegment(socket, TCP_FLAG_ACK, socket->sndNxt, socket->rcvNxt, 0,
         FALSE);
   }
   else
   {
      //Number of contiguous bytes that have been received
      length = rightEdge - leftEdge;

      //Next sequence number expected on incoming segments
      socket->rcvNxt += length;
      //Number of data available in the receive buffer
      socket->rcvUser += length;
      //Update the receive window
      socket->rcvWnd -= length;

      //Acknowledge the received data (delayed ACK not supported)
      tcpSendSegment(socket, TCP_FLAG_ACK, socket->sndNxt, socket->rcvNxt, 0,
         FALSE);

      //Notify user task that data is available
      tcpUpdateEvents(socket);
   }
}


/**
 * @brief Delete TCB structure
 * @param[in] socket Handle referencing the socket
 **/

void tcpDeleteControlBlock(Socket *socket)
{
   //Delete retransmission queue
   tcpFlushRetransmitQueue(socket);

   //Delete SYN queue
   tcpFlushSynQueue(socket);

   //Release transmit buffer
   netBufferSetLength((NetBuffer *) &socket->txBuffer, 0);

   //Release receive buffer
   netBufferSetLength((NetBuffer *) &socket->rxBuffer, 0);
}


/**
 * @brief Remove acknowledged segments from retransmission queue
 * @param[in] socket Handle referencing the socket
 **/

void tcpUpdateRetransmitQueue(Socket *socket)
{
   size_t length;
   TcpQueueItem *prevQueueItem;
   TcpQueueItem *queueItem;
   TcpHeader *header;

   //Point to the first item of the retransmission queue
   prevQueueItem = NULL;
   queueItem = socket->retransmitQueue;

   //Loop through retransmission queue
   while(queueItem != NULL)
   {
      //Point to the TCP header
      header = (TcpHeader *) queueItem->header;

      //Calculate the length of the TCP segment
      if(header->flags & TCP_FLAG_SYN)
      {
         length = 1;
      }
      else if(header->flags & TCP_FLAG_FIN)
      {
         length = queueItem->length + 1;
      }
      else
      {
         length = queueItem->length;
      }

      //If an acknowledgment is received for a segment before its timer
      //expires, the segment is removed from the retransmission queue
      if(TCP_CMP_SEQ(socket->sndUna, ntohl(header->seqNum) + length) >= 0)
      {
         //First item of the queue?
         if(prevQueueItem == NULL)
         {
            //Remove the current item from the queue
            socket->retransmitQueue = queueItem->next;
            //The item can now be safely deleted
            memPoolFree(queueItem);
            //Point to the next item
            queueItem = socket->retransmitQueue;
         }
         else
         {
            //Remove the current item from the queue
            prevQueueItem->next = queueItem->next;
            //The item can now be safely deleted
            memPoolFree(queueItem);
            //Point to the next item
            queueItem = prevQueueItem->next;
         }

         //When an ACK is received that acknowledges new data, restart the
         //retransmission timer so that it will expire after RTO seconds
         netStartTimer(&socket->retransmitTimer, socket->rto);
         //Reset retransmission counter
         socket->retransmitCount = 0;
      }
      //No acknowledgment received for the current segment...
      else
      {
         //Point to the next item
         prevQueueItem = queueItem;
         queueItem = queueItem->next;
      }
   }

   //When all outstanding data has been acknowledged,
   //turn off the retransmission timer
   if(socket->retransmitQueue == NULL)
      netStopTimer(&socket->retransmitTimer);
}


/**
 * @brief Flush retransmission queue
 * @param[in] socket Handle referencing the socket
 **/

void tcpFlushRetransmitQueue(Socket *socket)
{
   //Point to the first item in the retransmission queue
   TcpQueueItem *queueItem = socket->retransmitQueue;

   //Loop through retransmission queue
   while(queueItem != NULL)
   {
      //Keep track of the next item in the queue
      TcpQueueItem *nextQueueItem = queueItem->next;
      //Free previously allocated memory
      memPoolFree(queueItem);
      //Point to the next item
      queueItem = nextQueueItem;
   }

   //The retransmission queue is now flushed
   socket->retransmitQueue = NULL;

   //Turn off the retransmission timer
   netStopTimer(&socket->retransmitTimer);
}


/**
 * @brief Flush SYN queue
 * @param[in] socket Handle referencing the socket
 **/

void tcpFlushSynQueue(Socket *socket)
{
   //Point to the first item in the SYN queue
   TcpSynQueueItem *queueItem = socket->synQueue;

   //Loop through SYN queue
   while(queueItem != NULL)
   {
      //Keep track of the next item in the queue
      TcpSynQueueItem *nextQueueItem = queueItem->next;
      //Free previously allocated memory
      memPoolFree(queueItem);
      //Point to the next item
      queueItem = nextQueueItem;
   }

   //SYN queue was successfully flushed
   socket->synQueue = NULL;
}


/**
 * @brief Update the list of non-contiguous blocks that have been received
 * @param[in] socket Handle referencing the socket
 * @param[in,out] leftEdge First sequence number occupied by the incoming data
 * @param[in,out] rightEdge Sequence number immediately following the incoming data
 **/

void tcpUpdateSackBlocks(Socket *socket, uint32_t *leftEdge, uint32_t *rightEdge)
{
   uint_t i = 0;

   //Loop through the blocks
   while(i < socket->sackBlockCount)
   {
      //Find each block that overlaps the specified one
      if(TCP_CMP_SEQ(*rightEdge, socket->sackBlock[i].leftEdge) >= 0 &&
         TCP_CMP_SEQ(*leftEdge, socket->sackBlock[i].rightEdge) <= 0)
      {
         //Merge blocks to form a contiguous one
         *leftEdge = MIN(*leftEdge, socket->sackBlock[i].leftEdge);
         *rightEdge = MAX(*rightEdge, socket->sackBlock[i].rightEdge);

         //Delete current block
         osMemmove(socket->sackBlock + i, socket->sackBlock + i + 1,
            (TCP_MAX_SACK_BLOCKS - i - 1) * sizeof(TcpSackBlock));

         //Decrement the number of non-contiguous blocks
         socket->sackBlockCount--;
      }
      else
      {
         //Point to the next block
         i++;
      }
   }

   //Check whether the incoming segment was received out of order
   if(TCP_CMP_SEQ(*leftEdge, socket->rcvNxt) > 0)
   {
      //Make room for the new non-contiguous block
      osMemmove(socket->sackBlock + 1, socket->sackBlock,
         (TCP_MAX_SACK_BLOCKS - 1) * sizeof(TcpSackBlock));

      //Insert the element in the list
      socket->sackBlock[0].leftEdge = *leftEdge;
      socket->sackBlock[0].rightEdge = *rightEdge;

      //Increment the number of non-contiguous blocks
      if(socket->sackBlockCount < TCP_MAX_SACK_BLOCKS)
         socket->sackBlockCount++;
   }
}


/**
 * @brief Update send window
 * @param[in] socket Handle referencing the socket
 * @param[in] segment Pointer to the incoming TCP segment
 **/

void tcpUpdateSendWindow(Socket *socket, TcpHeader *segment)
{
   //Case where neither the sequence nor the acknowledgment number is increased
   if(segment->seqNum == socket->sndWl1 && segment->ackNum == socket->sndWl2)
   {
      //TCP may ignore a window update with a smaller window than previously
      //offered if neither the sequence number nor the acknowledgment number
      //is increased (refer to RFC 1122, section 4.2.2.16)
      if(segment->window > socket->sndWnd)
      {
         //Update the send window and record the sequence number and the
         //acknowledgment number used to update SND.WND
         socket->sndWnd = segment->window;
         socket->sndWl1 = segment->seqNum;
         socket->sndWl2 = segment->ackNum;

         //Maximum send window it has seen so far on the connection
         socket->maxSndWnd = MAX(socket->maxSndWnd, segment->window);
      }
   }
   //Case where the sequence or the acknowledgment number is increased
   else if(TCP_CMP_SEQ(segment->seqNum, socket->sndWl1) >= 0 &&
      TCP_CMP_SEQ(segment->ackNum, socket->sndWl2) >= 0)
   {
      //Check whether the remote host advertises a zero window
      if(segment->window == 0 && socket->sndWnd != 0)
      {
         //Start the persist timer
         socket->wndProbeCount = 0;
         socket->wndProbeInterval = TCP_DEFAULT_PROBE_INTERVAL;
         netStartTimer(&socket->persistTimer, socket->wndProbeInterval);
      }

      //Update the send window and record the sequence number and the
      //acknowledgment number used to update SND.WND
      socket->sndWnd = segment->window;
      socket->sndWl1 = segment->seqNum;
      socket->sndWl2 = segment->ackNum;

      //Maximum send window it has seen so far on the connection
      socket->maxSndWnd = MAX(socket->maxSndWnd, segment->window);
   }
}


/**
 * @brief Update receive window so as to avoid Silly Window Syndrome
 * @param[in] socket Handle referencing the socket
 **/

void tcpUpdateReceiveWindow(Socket *socket)
{
   uint16_t reduction;

   //Space available but not yet advertised
   reduction = socket->rxBufferSize - socket->rcvUser - socket->rcvWnd;

   //To avoid SWS, the receiver should not advertise small windows
   if((socket->rcvWnd + reduction) >= MIN(socket->rmss, socket->rxBufferSize / 2))
   {
      //Check whether a window update should be sent
      if(socket->rcvWnd < MIN(socket->rmss, socket->rxBufferSize / 2))
      {
         //Debug message
         TRACE_INFO("%s: TCP sending window update...\r\n",
            formatSystemTime(osGetSystemTime(), NULL));

         //Update the receive window
         socket->rcvWnd += reduction;
         //Send an ACK segment to advertise the new window size
         tcpSendSegment(socket, TCP_FLAG_ACK, socket->sndNxt, socket->rcvNxt, 0, FALSE);
      }
      else
      {
         //The receive window can be updated
         socket->rcvWnd += reduction;
      }
   }
}


/**
 * @brief Compute retransmission timeout
 * @param[in] socket Handle referencing the socket
 * @return TRUE if the RTT measurement is complete, else FALSE
 **/

bool_t tcpComputeRto(Socket *socket)
{
   bool_t flag;
   systime_t r;
   systime_t delta;

   //Clear flag
   flag = FALSE;

   //TCP implementation takes one RTT measurement at a time
   if(socket->rttBusy)
   {
      //Ensure the incoming ACK number covers the expected sequence number
      if(TCP_CMP_SEQ(socket->sndUna, socket->rttSeqNum) > 0)
      {
         //Calculate round-time trip
         r = osGetSystemTime() - socket->rttStartTime;

         //First RTT measurement?
         if(socket->srtt == 0 && socket->rttvar == 0)
         {
            //Initialize RTO calculation algorithm
            socket->srtt = r;
            socket->rttvar = r / 2;
         }
         else
         {
            //Calculate the difference between the measured value and the
            //current RTT estimator
            delta = (r > socket->srtt) ? (r - socket->srtt) : (socket->srtt - r);

            //Implement Van Jacobson's algorithm (as specified in RFC 6298 2.3)
            socket->rttvar = (3 * socket->rttvar + delta) / 4;
            socket->srtt = (7 * socket->srtt + r) / 8;
         }

         //Calculate the next retransmission timeout
         socket->rto = socket->srtt + 4 * socket->rttvar;

         //Whenever RTO is computed, if it is less than 1 second, then the RTO
         //should be rounded up to 1 second
         socket->rto = MAX(socket->rto, TCP_MIN_RTO);

         //A maximum value may be placed on RTO provided it is at least 60
         //seconds
         socket->rto = MIN(socket->rto, TCP_MAX_RTO);

         //Debug message
         TRACE_DEBUG("R=%" PRIu32 ", SRTT=%" PRIu32 ", RTTVAR=%" PRIu32 ", RTO=%" PRIu32 "\r\n",
            r, socket->srtt, socket->rttvar, socket->rto);

         //RTT measurement is complete
         socket->rttBusy = FALSE;
         //Set flag
         flag = TRUE;
      }
   }

   //Return TRUE if the RTT measurement is complete
   return flag;
}


/**
 * @brief TCP segment retransmission
 * @param[in] socket Handle referencing the socket
 * @return Error code
 **/

error_t tcpRetransmitSegment(Socket *socket)
{
   error_t error;
   size_t offset;
   size_t length;
   NetBuffer *buffer;
   TcpQueueItem *queueItem;
   TcpHeader *header;
   NetTxAncillary ancillary;

   //Initialize error code
   error = NO_ERROR;
   //Total number of bytes that have been retransmitted
   length = 0;

   //Point to the retransmission queue
   queueItem = socket->retransmitQueue;

   //Any segment in the retransmission queue?
   while(queueItem != NULL)
   {
      //Total number of bytes that have been retransmitted
      length += queueItem->length;

      //The amount of data that can be sent cannot exceed the MSS
      if(length > socket->smss)
      {
         //We are done
         error = NO_ERROR;
         //Exit immediately
         break;
      }

      //Point to the TCP header
      header = (TcpHeader *) queueItem->header;

      //Allocate a memory buffer to hold the TCP segment
      buffer = ipAllocBuffer(0, &offset);
      //Failed to allocate memory?
      if(buffer == NULL)
      {
         //Report an error
         error = ERROR_OUT_OF_MEMORY;
         //Exit immediately
         break;
      }

      //Start of exception handling block
      do
      {
         //Copy TCP header
         error = netBufferAppend(buffer, header, header->dataOffset * 4);
         //Any error to report?
         if(error)
            break;

         //Copy data from send buffer
         error = tcpReadTxBuffer(socket, ntohl(header->seqNum), buffer,
            queueItem->length);
         //Any error to report?
         if(error)
            break;

         //Total number of segments retransmitted
         MIB2_INC_COUNTER32(tcpGroup.tcpRetransSegs, 1);
         TCP_MIB_INC_COUNTER32(tcpRetransSegs, 1);

         //Dump TCP header contents for debugging purpose
         tcpDumpHeader(header, queueItem->length, socket->iss, socket->irs);

         //Additional options can be passed to the stack along with the packet
         ancillary = NET_DEFAULT_TX_ANCILLARY;
         //Set the TTL value to be used
         ancillary.ttl = socket->ttl;

#if (ETH_VLAN_SUPPORT == ENABLED)
         //Set VLAN PCP and DEI fields
         ancillary.vlanPcp = socket->vlanPcp;
         ancillary.vlanDei = socket->vlanDei;
#endif

#if (ETH_VMAN_SUPPORT == ENABLED)
         //Set VMAN PCP and DEI fields
         ancillary.vmanPcp = socket->vmanPcp;
         ancillary.vmanDei = socket->vmanDei;
#endif
         //Retransmit the lost segment without waiting for the retransmission
         //timer to expire
         error = ipSendDatagram(socket->interface, &queueItem->pseudoHeader,
            buffer, offset, &ancillary);

         //End of exception handling block
      } while(0);

      //Free previously allocated memory
      netBufferFree(buffer);

      //Any error to report?
      if(error)
      {
         //Exit immediately
         break;
      }

      //Point to the next segment in the queue
      queueItem = queueItem->next;
   }

   //Return status code
   return error;
}


/**
 * @brief Nagle algorithm implementation
 * @param[in] socket Handle referencing the socket
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t tcpNagleAlgo(Socket *socket, uint_t flags)
{
   error_t error;
   uint32_t n;
   uint32_t u;

   //The amount of data that can be sent at any given time is
   //limited by the receiver window and the congestion window
   n = MIN(socket->sndWnd, socket->txBufferSize);

#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
   //Check the congestion window
   n = MIN(n, socket->cwnd);
#endif

   //Retrieve the size of the usable window
   u = n - (socket->sndNxt - socket->sndUna);

   //The Nagle algorithm discourages sending tiny segments when
   //the data to be sent increases in small increments
   while(socket->sndUser > 0)
   {
      //The usable window size may become zero or negative,
      //preventing packet transmission
      if((int32_t) u <= 0)
         break;

      //Calculate the number of bytes to send at a time
      n = MIN(u, socket->sndUser);
      n = MIN(n, socket->smss);

      //Disable Nagle algorithm?
      if((flags & SOCKET_FLAG_NO_DELAY) != 0)
      {
         //All packets will be send no matter what size they have
         if(n > 0)
         {
            //Send TCP segment
            error = tcpSendSegment(socket, TCP_FLAG_PSH | TCP_FLAG_ACK,
               socket->sndNxt, socket->rcvNxt, n, TRUE);
            //Failed to send TCP segment?
            if(error)
               return error;
         }
         else
         {
            //We are done...
            break;
         }
      }
      else if((flags & SOCKET_FLAG_DELAY) != 0)
      {
         //Transmit data if a maximum-sized segment can be sent
         if(MIN(socket->sndUser, u) >= socket->smss)
         {
            //Send TCP segment
            error = tcpSendSegment(socket, TCP_FLAG_PSH | TCP_FLAG_ACK,
               socket->sndNxt, socket->rcvNxt, n, TRUE);
            //Failed to send TCP segment?
            if(error)
               return error;
         }
         else
         {
            //Prevent the sender from sending tiny segments...
            break;
         }
      }
      else
      {
         //Transmit data if a maximum-sized segment can be sent
         if(MIN(socket->sndUser, u) >= socket->smss)
         {
            //Send TCP segment
            error = tcpSendSegment(socket, TCP_FLAG_PSH | TCP_FLAG_ACK,
               socket->sndNxt, socket->rcvNxt, n, TRUE);
            //Failed to send TCP segment?
            if(error)
               return error;
         }
         //Or if all queued data can be sent now
         else if(socket->sndNxt == socket->sndUna && socket->sndUser <= u)
         {
            //Send TCP segment
            error = tcpSendSegment(socket, TCP_FLAG_PSH | TCP_FLAG_ACK,
               socket->sndNxt, socket->rcvNxt, n, TRUE);
            //Failed to send TCP segment?
            if(error)
               return error;
         }
         //Or if at least a fraction of the maximum window can be sent
         else if(MIN(socket->sndUser, u) >= (socket->maxSndWnd / 2))
         {
            //Send TCP segment
            error = tcpSendSegment(socket, TCP_FLAG_PSH | TCP_FLAG_ACK,
               socket->sndNxt, socket->rcvNxt, n, TRUE);
            //Failed to send TCP segment?
            if(error)
               return error;
         }
         else
         {
            //Prevent the sender from sending tiny segments...
            break;
         }
      }

      //Advance SND.NXT pointer
      socket->sndNxt += n;
      //Update the number of data buffered but not yet sent
      socket->sndUser -= n;
      //Update the size of the usable window
      u -= n;
   }

   //Check whether the transmitter can accept more data
   tcpUpdateEvents(socket);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Update TCP FSM current state
 * @param[in] socket Handle referencing the socket
 * @param[in] newState New TCP state to switch to
 **/

void tcpChangeState(Socket *socket, TcpState newState)
{
   //Enter CLOSED state?
   if(newState == TCP_STATE_CLOSED)
   {
      //Check previous state
      if(socket->state == TCP_STATE_LAST_ACK ||
         socket->state == TCP_STATE_TIME_WAIT)
      {
         //The connection has been closed properly
         socket->closedFlag = TRUE;
      }
      else
      {
         //The connection has been reset by the peer
         socket->resetFlag = TRUE;
      }
   }

   //Enter the desired state
   socket->state = newState;
   //Update TCP related events
   tcpUpdateEvents(socket);
}


/**
 * @brief Update TCP related events
 * @param[in] socket Handle referencing the socket
 **/

void tcpUpdateEvents(Socket *socket)
{
   //Clear event flags
   socket->eventFlags = 0;

   //Check current TCP state
   switch(socket->state)
   {
   //ESTABLISHED or FIN-WAIT-1 state?
   case TCP_STATE_ESTABLISHED:
   case TCP_STATE_FIN_WAIT_1:
      socket->eventFlags |= SOCKET_EVENT_CONNECTED;
      break;

   //FIN-WAIT-2 state?
   case TCP_STATE_FIN_WAIT_2:
      socket->eventFlags |= SOCKET_EVENT_CONNECTED;
      socket->eventFlags |= SOCKET_EVENT_TX_SHUTDOWN;
      break;

   //CLOSE-WAIT, LAST-ACK or CLOSING state?
   case TCP_STATE_CLOSE_WAIT:
   case TCP_STATE_LAST_ACK:
   case TCP_STATE_CLOSING:
      socket->eventFlags |= SOCKET_EVENT_CONNECTED;
      socket->eventFlags |= SOCKET_EVENT_RX_SHUTDOWN;
      break;

   //TIME-WAIT or CLOSED state?
   case TCP_STATE_TIME_WAIT:
   case TCP_STATE_CLOSED:
      socket->eventFlags |= SOCKET_EVENT_CLOSED;
      socket->eventFlags |= SOCKET_EVENT_TX_SHUTDOWN;
      socket->eventFlags |= SOCKET_EVENT_RX_SHUTDOWN;
      break;

   //Any other state
   default:
      break;
   }

   //Handle TX specific events
   if(socket->state == TCP_STATE_SYN_SENT ||
      socket->state == TCP_STATE_SYN_RECEIVED)
   {
      //Disallow write operations until the connection is established
      socket->eventFlags |= SOCKET_EVENT_TX_DONE;
      socket->eventFlags |= SOCKET_EVENT_TX_ACKED;
   }
   else if(socket->state == TCP_STATE_ESTABLISHED ||
      socket->state == TCP_STATE_CLOSE_WAIT)
   {
      //Check whether the send buffer is full or not
      if((socket->sndUser + socket->sndNxt - socket->sndUna) < socket->txBufferSize)
      {
         socket->eventFlags |= SOCKET_EVENT_TX_READY;
      }

      //Check whether all the data in the send buffer has been transmitted
      if(socket->sndUser == 0)
      {
         //All the pending data has been sent out
         socket->eventFlags |= SOCKET_EVENT_TX_DONE;

         //Check whether an acknowledgment has been received
         if(TCP_CMP_SEQ(socket->sndUna, socket->sndNxt) >= 0)
         {
            socket->eventFlags |= SOCKET_EVENT_TX_ACKED;
         }
      }
   }
   else if(socket->state != TCP_STATE_LISTEN)
   {
      //Unblock user task if the connection is being closed
      socket->eventFlags |= SOCKET_EVENT_TX_READY;
      socket->eventFlags |= SOCKET_EVENT_TX_DONE;
      socket->eventFlags |= SOCKET_EVENT_TX_ACKED;
   }

   //Handle RX specific events
   if(socket->state == TCP_STATE_ESTABLISHED ||
      socket->state == TCP_STATE_FIN_WAIT_1 ||
      socket->state == TCP_STATE_FIN_WAIT_2)
   {
      //Data is available for reading?
      if(socket->rcvUser > 0)
      {
         socket->eventFlags |= SOCKET_EVENT_RX_READY;
      }
   }
   else if(socket->state == TCP_STATE_LISTEN)
   {
      //If the socket is currently in the listen state, it will be marked
      //as readable if an incoming connection request has been received
      if(socket->synQueue != NULL)
      {
         socket->eventFlags |= SOCKET_EVENT_ACCEPT;
         socket->eventFlags |= SOCKET_EVENT_RX_READY;
      }
   }
   else if(socket->state != TCP_STATE_SYN_SENT &&
      socket->state != TCP_STATE_SYN_RECEIVED)
   {
      //Readability can also indicate that a request to close
      //the socket has been received from the peer
      socket->eventFlags |= SOCKET_EVENT_RX_READY;
   }

   //Check whether the socket is bound to a particular network interface
   if(socket->interface != NULL)
   {
      //Handle link up and link down events
      if(socket->interface->linkState)
      {
         socket->eventFlags |= SOCKET_EVENT_LINK_UP;
      }
      else
      {
         socket->eventFlags |= SOCKET_EVENT_LINK_DOWN;
      }
   }

   //Mask unused events
   socket->eventFlags &= socket->eventMask;

   //Any event to signal?
   if(socket->eventFlags)
   {
      //Unblock I/O operations currently in waiting state
      osSetEvent(&socket->event);

      //Set user event to signaled state if necessary
      if(socket->userEvent != NULL)
      {
         osSetEvent(socket->userEvent);
      }
   }
}


/**
 * @brief Wait for a particular TCP event
 * @param[in] socket Handle referencing the socket
 * @param[in] eventMask Logic OR of all the TCP events that will complete the wait
 * @param[in] timeout Maximum time to wait
 * @return Logic OR of all the TCP events that satisfied the wait
 **/

uint_t tcpWaitForEvents(Socket *socket, uint_t eventMask, systime_t timeout)
{
   //Sanity check
   if(socket == NULL)
      return 0;

   //Only one of the events listed here may complete the wait
   socket->eventMask = eventMask;
   //Update TCP related events
   tcpUpdateEvents(socket);

   //No event is signaled?
   if(socket->eventFlags == 0)
   {
      //Reset the event object
      osResetEvent(&socket->event);

      //Release exclusive access
      osReleaseMutex(&netMutex);
      //Wait until an event is triggered
      osWaitForEvent(&socket->event, timeout);
      //Get exclusive access
      osAcquireMutex(&netMutex);
   }

   //Return the list of TCP events that satisfied the wait
   return socket->eventFlags;
}


/**
 * @brief Copy incoming data to the send buffer
 * @param[in] socket Handle referencing the socket
 * @param[in] seqNum First sequence number occupied by the incoming data
 * @param[in] data Data to write
 * @param[in] length Number of data to write
 **/

void tcpWriteTxBuffer(Socket *socket, uint32_t seqNum,
   const uint8_t *data, size_t length)
{
   //Offset of the first byte to write in the circular buffer
   size_t offset = (seqNum - socket->iss - 1) % socket->txBufferSize;

   //Check whether the specified data crosses buffer boundaries
   if((offset + length) <= socket->txBufferSize)
   {
      //Copy the payload
      netBufferWrite((NetBuffer *) &socket->txBuffer,
         offset, data, length);
   }
   else
   {
      //Copy the first part of the payload
      netBufferWrite((NetBuffer *) &socket->txBuffer,
         offset, data, socket->txBufferSize - offset);

      //Wrap around to the beginning of the circular buffer
      netBufferWrite((NetBuffer *) &socket->txBuffer, 0,
         data + socket->txBufferSize - offset,
         length - socket->txBufferSize + offset);
   }
}


/**
 * @brief Copy data from the send buffer
 * @param[in] socket Handle referencing the socket
 * @param[in] seqNum Sequence number of the first data to read
 * @param[out] buffer Pointer to the output buffer
 * @param[in] length Number of data to read
 * @return Error code
 **/

error_t tcpReadTxBuffer(Socket *socket, uint32_t seqNum,
   NetBuffer *buffer, size_t length)
{
   error_t error;

   //Offset of the first byte to read in the circular buffer
   size_t offset = (seqNum - socket->iss - 1) % socket->txBufferSize;

   //Check whether the specified data crosses buffer boundaries
   if((offset + length) <= socket->txBufferSize)
   {
      //Copy the payload
      error = netBufferConcat(buffer, (NetBuffer *) &socket->txBuffer,
         offset, length);
   }
   else
   {
      //Copy the first part of the payload
      error = netBufferConcat(buffer, (NetBuffer *) &socket->txBuffer,
         offset, socket->txBufferSize - offset);

      //Check status code
      if(!error)
      {
         //Wrap around to the beginning of the circular buffer
         error = netBufferConcat(buffer, (NetBuffer *) &socket->txBuffer,
            0, length - socket->txBufferSize + offset);
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Copy incoming data to the receive buffer
 * @param[in] socket Handle referencing the socket
 * @param[in] seqNum First sequence number occupied by the incoming data
 * @param[in] data Multi-part buffer containing the incoming data
 * @param[in] dataOffset Offset to the first data byte
 * @param[in] length Number of data to write
 **/

void tcpWriteRxBuffer(Socket *socket, uint32_t seqNum,
   const NetBuffer *data, size_t dataOffset, size_t length)
{
   //Offset of the first byte to write in the circular buffer
   size_t offset = (seqNum - socket->irs - 1) % socket->rxBufferSize;

   //Check whether the specified data crosses buffer boundaries
   if((offset + length) <= socket->rxBufferSize)
   {
      //Copy the payload
      netBufferCopy((NetBuffer *) &socket->rxBuffer,
         offset, data, dataOffset, length);
   }
   else
   {
      //Copy the first part of the payload
      netBufferCopy((NetBuffer *) &socket->rxBuffer,
         offset, data, dataOffset, socket->rxBufferSize - offset);

      //Wrap around to the beginning of the circular buffer
      netBufferCopy((NetBuffer *) &socket->rxBuffer, 0, data,
         dataOffset + socket->rxBufferSize - offset,
         length - socket->rxBufferSize + offset);
   }
}


/**
 * @brief Copy data from the receive buffer
 * @param[in] socket Handle referencing the socket
 * @param[in] seqNum Sequence number of the first data to read
 * @param[out] data Pointer to the output buffer
 * @param[in] length Number of data to read
 **/

void tcpReadRxBuffer(Socket *socket, uint32_t seqNum, uint8_t *data,
   size_t length)
{
   //Offset of the first byte to read in the circular buffer
   size_t offset = (seqNum - socket->irs - 1) % socket->rxBufferSize;

   //Check whether the specified data crosses buffer boundaries
   if((offset + length) <= socket->rxBufferSize)
   {
      //Copy the payload
      netBufferRead(data, (NetBuffer *) &socket->rxBuffer,
         offset, length);
   }
   else
   {
      //Copy the first part of the payload
      netBufferRead(data, (NetBuffer *) &socket->rxBuffer,
         offset, socket->rxBufferSize - offset);

      //Wrap around to the beginning of the circular buffer
      netBufferRead(data + socket->rxBufferSize - offset,
         (NetBuffer *) &socket->rxBuffer, 0,
         length - socket->rxBufferSize + offset);
   }
}


/**
 * @brief Dump TCP header for debugging purpose
 * @param[in] segment Pointer to the TCP header
 * @param[in] length Length of the segment data
 * @param[in] iss Initial send sequence number (needed to compute relative SEQ number)
 * @param[in] irs Initial receive sequence number (needed to compute relative ACK number)
 **/

void tcpDumpHeader(const TcpHeader *segment, size_t length, uint32_t iss,
   uint32_t irs)
{
   //Dump TCP header contents
   TRACE_DEBUG("%" PRIu16 " > %" PRIu16 ": %c%c%c%c%c%c seq=%" PRIu32 "(%" PRIu32 ") "
      "ack=%" PRIu32 "(%" PRIu32 ") win=%" PRIu16 " len=%" PRIuSIZE "\r\n",
      ntohs(segment->srcPort), ntohs(segment->destPort),
      (segment->flags & TCP_FLAG_FIN) ? 'F' : '-',
      (segment->flags & TCP_FLAG_SYN) ? 'S' : '-',
      (segment->flags & TCP_FLAG_RST) ? 'R' : '-',
      (segment->flags & TCP_FLAG_PSH) ? 'P' : '-',
      (segment->flags & TCP_FLAG_ACK) ? 'A' : '-',
      (segment->flags & TCP_FLAG_URG) ? 'U' : '-',
      ntohl(segment->seqNum), ntohl(segment->seqNum) - iss,
      ntohl(segment->ackNum), ntohl(segment->ackNum) - irs,
      ntohs(segment->window), length);
}

#endif
