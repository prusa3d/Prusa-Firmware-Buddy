/**
 * @file ping.c
 * @brief Ping utility
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
#define TRACE_LEVEL PING_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "core/ping.h"
#include "core/ip.h"
#include "ipv4/ipv4.h"
#include "ipv4/ipv4_misc.h"
#include "ipv4/icmp.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "ipv6/icmpv6.h"
#include "core/socket.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (PING_SUPPORT == ENABLED)

//Sequence number field
static uint16_t pingSequenceNumber = 0;


/**
 * @brief Test the reachability of a host
 *
 * Ping operates by sending an ICMP Echo Request message to the
 * target host and waiting for an ICMP Echo Reply message
 *
 * @param[in] interface Underlying network interface (optional parameter)
 * @param[in] targetIpAddr IP address of the host to reach
 * @param[in] size Size of the data payload in bytes
 * @param[in] ttl Time-To-Live value to be used
 * @param[in] timeout Maximum time to wait before giving up
 * @param[out] rtt Round-trip time (optional parameter)
 * @return Error code
 **/

error_t ping(NetInterface *interface, const IpAddr *targetIpAddr,
   size_t size, uint8_t ttl, systime_t timeout, systime_t *rtt)
{
   error_t error;
   PingContext context;

   //Check parameters
   if(targetIpAddr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize context
   pingInit(&context);

   //Start of exception handling block
   do
   {
      //Select the specified network interface
      error = pingBindToInterface(&context, interface);
      //Any error to report?
      if(error)
         break;

      //Set timeout value
      error = pingSetTimeout(&context, timeout);
      //Any error to report?
      if(error)
         break;

      //Send an ICMP Echo Request message
      error = pingSendRequest(&context, targetIpAddr, size, ttl);
      //Any error to report?
      if(error)
         break;

      //Wait for a matching Echo Reply message
      error = pingWaitForReply(&context, NULL, rtt);
      //Any error to report?
      if(error)
         break;

      //End of exception handling block
   } while(0);

   //Release resources
   pingRelease(&context);

   //Return status code
   return error;
}


/**
 * @brief Initialize ping context
 * @param[in] context Pointer to the ping context
 **/

void pingInit(PingContext *context)
{
   //Make sure the context is valid
   if(context != NULL)
   {
      //Initialize context
      osMemset(context, 0, sizeof(PingContext));

      //Set the default timeout to be used
      context->timeout = PING_DEFAULT_TIMEOUT;
   }
}


/**
 * @brief Set timeout value
 * @param[in] context Pointer to the ping context
 * @param[in] timeout Maximum time to wait
 * @return Error code
 **/

error_t pingSetTimeout(PingContext *context, systime_t timeout)
{
   //Invalid context?
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save timeout value
   context->timeout = timeout;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Select a particular network interface
 * @param[in] context Pointer to the ping context
 * @param[in] interface Network interface to be used
 * @return Error code
 **/

error_t pingBindToInterface(PingContext *context, NetInterface *interface)
{
   //Invalid context?
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Select the specified network interface
   context->interface = interface;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Send an ICMP Echo Request message
 * @param[in] context Pointer to the ping context
 * @param[in] targetIpAddr IP address of the host to reach
 * @param[in] size Size of the data payload, in bytes
 * @param[in] ttl Time-To-Live value to be used
 * @return Error code
 **/

error_t pingSendRequest(PingContext *context,
   const IpAddr *targetIpAddr, size_t size, uint8_t ttl)
{
   error_t error;
   size_t i;
   size_t length;
   NetInterface *interface;
   IcmpEchoMessage *message;

   //Invalid context?
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Limit the size of the data payload
   context->dataPayloadSize = MIN (size, PING_MAX_DATA_SIZE);

   //Close existing socket, if necessary
   if(context->socket != NULL)
   {
      socketClose(context->socket);
      context->socket = NULL;
   }

   //Identifier field is used to help matching requests and replies
   context->identifier = netGetRand();

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Sequence Number field is increment each time an Echo Request is sent
   context->sequenceNumber = pingSequenceNumber++;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Point to the buffer where to format the ICMP message
   message = (IcmpEchoMessage *) context->buffer;

   //Format ICMP Echo Request message
   message->type = ICMP_TYPE_ECHO_REQUEST;
   message->code = 0;
   message->checksum = 0;
   message->identifier = context->identifier;
   message->sequenceNumber = context->sequenceNumber;

   //Initialize data payload
   for(i = 0; i < context->dataPayloadSize; i++)
      message->data[i] = i & 0xFF;

   //Length of the complete ICMP message including header and data
   length = sizeof(IcmpEchoMessage) + context->dataPayloadSize;

   //Select the relevant network interface
   interface = context->interface;

#if (IPV4_SUPPORT == ENABLED)
   //Is target address an IPv4 address?
   if(targetIpAddr->length == sizeof(Ipv4Addr))
   {
      Ipv4Addr srcIpAddr;

      //Select the source IPv4 address and the relevant network
      //interface to use when pinging the specified host
      error = ipv4SelectSourceAddr(&interface, targetIpAddr->ipv4Addr,
         &srcIpAddr);
      //Any error to report?
      if(error)
         return error;

      //ICMP Echo Request message
      message->type = ICMP_TYPE_ECHO_REQUEST;
      //Message checksum calculation
      message->checksum = ipCalcChecksum(message, length);

      //Open a raw socket
      context->socket = socketOpen(SOCKET_TYPE_RAW_IP, SOCKET_IP_PROTO_ICMP);
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //Is target address an IPv6 address?
   if(targetIpAddr->length == sizeof(Ipv6Addr))
   {
      Ipv6PseudoHeader pseudoHeader;

      //Select the source IPv6 address and the relevant network interface
      //to use when pinging the specified host
      error = ipv6SelectSourceAddr(&interface, &targetIpAddr->ipv6Addr,
         &pseudoHeader.srcAddr);
      //Any error to report?
      if(error)
         return error;

      //ICMPv6 Echo Request message
      message->type = ICMPV6_TYPE_ECHO_REQUEST;

      //Format IPv6 pseudo header
      pseudoHeader.destAddr = targetIpAddr->ipv6Addr;
      pseudoHeader.length = htonl(length);
      pseudoHeader.reserved[0] = 0;
      pseudoHeader.reserved[1] = 0;
      pseudoHeader.reserved[2] = 0;
      pseudoHeader.nextHeader = IPV6_ICMPV6_HEADER;

      //Message checksum calculation
      message->checksum = ipCalcUpperLayerChecksum(&pseudoHeader,
         sizeof(Ipv6PseudoHeader), message, length);

      //Open a raw socket
      context->socket = socketOpen(SOCKET_TYPE_RAW_IP, SOCKET_IP_PROTO_ICMPV6);
   }
   else
#endif
   //Invalid target address?
   {
      //Report an error
      return ERROR_INVALID_ADDRESS;
   }

   //Failed to open socket?
   if(context->socket == NULL)
      return ERROR_OPEN_FAILED;

   //Set the TTL value to be used
   context->socket->ttl = ttl;

   //Start of exception handling block
   do
   {
      //Associate the newly created socket with the relevant interface
      error = socketBindToInterface(context->socket, interface);
      //Unable to bind the socket to the desired interface?
      if(error)
         break;

      //Debug message
      TRACE_INFO("Sending ICMP echo request to %s (%" PRIuSIZE " bytes)...\r\n",
         ipAddrToString(targetIpAddr, NULL), length);

      //Send Echo Request message
      error = socketSendTo(context->socket, targetIpAddr, 0,
         message, length, NULL, 0);
      //Failed to send message ?
      if(error)
         break;

      //Save the time at which the request was sent
      context->timestamp = osGetSystemTime();

      //End of exception handling block
   } while(0);

   //Any error to report?
   if(error)
   {
      //Clean up side effects
      socketClose(context->socket);
      context->socket = NULL;
   }

   //Return status code
   return error;
}


/**
 * @brief Check whether an incoming ICMP message is acceptable
 * @param[in] context Pointer to the ping context
 * @param[in] srcIpAddr Source IP address
 * @param[in] destIpAddr Destination IP address
 * @param[in] message Pointer to the incoming ICMP message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t pingCheckReply(PingContext *context, const IpAddr *srcIpAddr,
   const IpAddr *destIpAddr, const IcmpEchoMessage *message, size_t length)
{
   size_t i;

   //Check message length
   if(length != (sizeof(IcmpEchoMessage) + context->dataPayloadSize))
      return ERROR_INVALID_MESSAGE;

#if (IPV4_SUPPORT == ENABLED)
   //Is target address an IPv4 address?
   if(context->socket->protocol == SOCKET_IP_PROTO_ICMP)
   {
      //Check address type
      if(destIpAddr->length != sizeof(Ipv4Addr))
         return ERROR_INVALID_MESSAGE;

      //Check message type
      if(message->type != ICMP_TYPE_ECHO_REPLY)
         return ERROR_INVALID_MESSAGE;

      //Verify checksum value
      if(ipCalcChecksum(message, length) != 0x0000)
         return ERROR_INVALID_MESSAGE;
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //Is target address an IPv6 address?
   if(context->socket->protocol == SOCKET_IP_PROTO_ICMPV6)
   {
      Ipv6PseudoHeader pseudoHeader;

      //Check address type
      if(destIpAddr->length != sizeof(Ipv6Addr))
         return ERROR_INVALID_MESSAGE;

      //Check message type
      if(message->type != ICMPV6_TYPE_ECHO_REPLY)
         return ERROR_INVALID_MESSAGE;

      //Format IPv6 pseudo header
      pseudoHeader.srcAddr = srcIpAddr->ipv6Addr;
      pseudoHeader.destAddr = destIpAddr->ipv6Addr;
      pseudoHeader.length = htonl(length);
      pseudoHeader.reserved[0] = 0;
      pseudoHeader.reserved[1] = 0;
      pseudoHeader.reserved[2] = 0;
      pseudoHeader.nextHeader = IPV6_ICMPV6_HEADER;

      //Verify checksum value
      if(ipCalcUpperLayerChecksum(&pseudoHeader,
         sizeof(Ipv6PseudoHeader), message, length) != 0x0000)
      {
         //The checksum is not valid
         return ERROR_INVALID_MESSAGE;
      }
   }
   else
#endif
   //Invalid target address?
   {
      //Report an error
      return ERROR_INVALID_ADDRESS;
   }

   //Make sure the response identifier matches the request identifier
   if(message->identifier != context->identifier)
      return ERROR_INVALID_MESSAGE;
   //Make sure the sequence number is correct
   if(message->sequenceNumber != context->sequenceNumber)
      return ERROR_INVALID_MESSAGE;

   //Verify data payload
   for(i = 0; i < context->dataPayloadSize; i++)
   {
      //Compare received data against expected data pattern
      if(message->data[i] != (i & 0xFF))
         return ERROR_INVALID_MESSAGE;
   }

   //The ICMP Echo Reply message is acceptable
   return NO_ERROR;
}


/**
 * @brief Wait for a matching ICMP Echo Reply message
 * @param[in] context Pointer to the ping context
 * @param[out] targetIpAddr IP address of the remote host (optional parameter)
 * @param[out] rtt Round-trip time (optional parameter)
 * @return Error code
 **/

error_t pingWaitForReply(PingContext *context,
   IpAddr *targetIpAddr, systime_t *rtt)
{
   error_t error;
   size_t length;
   systime_t time;
   systime_t timeout;
   IpAddr srcIpAddr;
   IpAddr destIpAddr;

   //Invalid context?
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Wait for an ICMP Echo Reply message
   do
   {
      //Get current time
      time = osGetSystemTime();

      //Compute the timeout to be used
      if(timeCompare(time, context->timestamp + context->timeout) < 0)
         timeout = context->timestamp + context->timeout - time;
      else
         timeout = 0;

      //Adjust receive timeout
      error = socketSetTimeout(context->socket, timeout);
      //Any error to report?
      if(error)
         break;

      //Wait for an incoming ICMP message
      error = socketReceiveEx(context->socket, &srcIpAddr, NULL,
         &destIpAddr, context->buffer, PING_BUFFER_SIZE, &length, 0);

#if (NET_RTOS_SUPPORT == DISABLED)
      //Catch timeout exception
      if(error == ERROR_TIMEOUT)
         error = ERROR_WOULD_BLOCK;
#endif

      //Get current time
      time = osGetSystemTime();

      //Check status code
      if(!error)
      {
         //Check whether the incoming ICMP message is acceptable
         error = pingCheckReply(context, &srcIpAddr, &destIpAddr,
            (IcmpEchoMessage *) context->buffer, length);
      }

      //Check status code
      if(!error)
      {
         //Calculate round-trip time
         context->rtt = time - context->timestamp;

         //Debug message
         TRACE_INFO("ICMP echo reply received from %s (%" PRIu32 " ms)...\r\n",
            ipAddrToString(&srcIpAddr, NULL), context->rtt);

         //Return the IP address of the host
         if(targetIpAddr != NULL)
            *targetIpAddr = srcIpAddr;

         //Return the round-trip time
         if(rtt != NULL)
            *rtt = context->rtt;
      }
      else
      {
         //Timeout value exceeded?
         if(timeCompare(time, context->timestamp + context->timeout) >= 0)
         {
            //Report an error
            error = ERROR_TIMEOUT;
         }
      }

      //Wait for the next incoming ICMP message
   } while(error == ERROR_INVALID_MESSAGE);

   //Return status code
   return error;
}


/**
 * @brief Release ping context
 * @param[in] context Pointer to the ping context
 **/

void pingRelease(PingContext *context)
{
   //Make sure the context is valid
   if(context != NULL)
   {
      //Close underlying socket
      if(context->socket != NULL)
      {
         socketClose(context->socket);
         context->socket = NULL;
      }
   }
}

#endif
