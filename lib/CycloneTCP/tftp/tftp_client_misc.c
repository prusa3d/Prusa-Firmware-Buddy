/**
 * @file tftp_client_misc.c
 * @brief Helper functions for TFTP client
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
#define TRACE_LEVEL TFTP_TRACE_LEVEL

//Dependencies
#include "tftp/tftp_client.h"
#include "tftp/tftp_client_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (TFTP_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Open connection with the TFTP server
 * @param[in] context Pointer to the TFTP client context
 * @return Error code
 **/

error_t tftpClientOpenConnection(TftpClientContext *context)
{
   error_t error;

   //Properly close existing connection
   tftpClientCloseConnection(context);

   //Open a UDP socket
   context->socket = socketOpen(SOCKET_TYPE_DGRAM, SOCKET_IP_PROTO_UDP);

   //Valid socket?
   if(context->socket != NULL)
   {
      //Associate the socket with the relevant interface
      error = socketBindToInterface(context->socket, context->interface);

      //Check status code
      if(!error)
      {
         //Set timeout
         error = socketSetTimeout(context->socket, TFTP_CLIENT_TICK_INTERVAL);
      }

      //Any error to report?
      if(error)
      {
         //Clean up side effects
         tftpClientCloseConnection(context);
      }
   }
   else
   {
      //Report an error
      error = ERROR_OPEN_FAILED;
   }

   //Return status code
   return error;
}


/**
 * @brief Close connection with the TFTP server
 * @param[in] context Pointer to the TFTP client context
 **/

void tftpClientCloseConnection(TftpClientContext *context)
{
   //Sanity check
   if(context->socket != NULL)
   {
      //Close UDP socket
      socketClose(context->socket);
      context->socket = NULL;
   }
}


/**
 * @brief Process TFTP client events
 * @param[in] context Pointer to the TFTP client context
 * @return Error code
 **/

error_t tftpClientProcessEvents(TftpClientContext *context)
{
   error_t error;
   systime_t time;
   IpAddr srcIpAddr;
   uint16_t srcPort;

   //Wait for an incoming packet
   error = socketReceiveFrom(context->socket, &srcIpAddr, &srcPort,
      context->inPacket, TFTP_CLIENT_MAX_PACKET_SIZE, &context->inPacketLen, 0);

   //Check status code
   if(error == NO_ERROR)
   {
      //Process incoming packet
      tftpClientProcessPacket(context, &srcIpAddr, srcPort);
   }
   else if(error == ERROR_TIMEOUT)
   {
#if (NET_RTOS_SUPPORT == ENABLED)
      error = NO_ERROR;
#else
      error = ERROR_WOULD_BLOCK;
#endif
   }

   //Check status code
   if(error == NO_ERROR || error == ERROR_WOULD_BLOCK)
   {
      //Check current state
      if(context->state == TFTP_CLIENT_STATE_RRQ ||
         context->state == TFTP_CLIENT_STATE_WRQ ||
         context->state == TFTP_CLIENT_STATE_DATA ||
         context->state == TFTP_CLIENT_STATE_ACK ||
         context->state == TFTP_CLIENT_STATE_LAST_DATA)
      {
         //Get current time
         time = osGetSystemTime();

         //Check current time
         if(timeCompare(time, context->timestamp + TFTP_CLIENT_TIMEOUT) >= 0)
         {
            //Handle retransmissions
            if(context->retransmitCount < TFTP_CLIENT_MAX_RETRIES)
            {
               //Retransmit last packet
               tftpClientRetransmitPacket(context);

               //Save the time at which the packet was sent
               context->timestamp = osGetSystemTime();
               //Increment retransmission counter
               context->retransmitCount++;
            }
            else
            {
               //Report a timeout error
               error = ERROR_TIMEOUT;
            }
         }
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming packet
 * @param[in] context Pointer to the TFTP client context
 * @param[in] srcIpAddr Source IP address
 * @param[in] srcPort Source port number
 **/

void tftpClientProcessPacket(TftpClientContext *context,
   const IpAddr *srcIpAddr, uint16_t srcPort)
{
   uint16_t opcode;

   //Check the source IP address of the incoming packet
   if(!ipCompAddr(srcIpAddr, &context->serverIpAddr))
      return;

   //Malformed packet?
   if(context->inPacketLen < sizeof(uint16_t))
      return;

   //Retrieve TFTP packet type
   opcode = LOAD16BE(context->inPacket);

   //Data packet received?
   if(opcode == TFTP_OPCODE_DATA)
   {
      //Process DATA packet
      tftpClientProcessDataPacket(context, srcPort,
         (TftpDataPacket *) context->inPacket, context->inPacketLen);
   }
   //Acknowledgment packet received?
   else if(opcode == TFTP_OPCODE_ACK)
   {
      //Process ACK packet
      tftpClientProcessAckPacket(context, srcPort,
         (TftpAckPacket *) context->inPacket, context->inPacketLen);
   }
   //Error packet received?
   else if(opcode == TFTP_OPCODE_ERROR)
   {
      //Process ERROR packet
      tftpClientProcessErrorPacket(context, srcPort,
         (TftpErrorPacket *) context->inPacket, context->inPacketLen);
   }
   //Invalid packet received?
   else
   {
      //Discard incoming packet
   }
}


/**
 * @brief Process incoming DATA packet
 * @param[in] context Pointer to the TFTP client context
 * @param[in] srcPort Source port number
 * @param[in] dataPacket Pointer to the DATA packet
 * @param[in] length Length of the packet, in bytes
 **/

void tftpClientProcessDataPacket(TftpClientContext *context,
   uint16_t srcPort, const TftpDataPacket *dataPacket, size_t length)
{
   //Debug message
   TRACE_DEBUG("TFTP Client: DATA packet received (%" PRIuSIZE " bytes)...\r\n",
      length);

   //Make sure the length of the DATA packet is acceptable
   if(length < sizeof(TftpDataPacket))
      return;

   //Calculate the length of the data
   length -= sizeof(TftpDataPacket);

   //Debug message
   TRACE_DEBUG("  Opcode = %" PRIu16 "\r\n", ntohs(dataPacket->opcode));
   TRACE_DEBUG("  Block = %" PRIu16 "\r\n", ntohs(dataPacket->block));

   //Check current state
   if(context->state == TFTP_CLIENT_STATE_RRQ)
   {
      //Check block number
      if(ntohs(dataPacket->block) == context->block)
      {
         //Save the TID chosen by the server
         context->serverTid = srcPort;

         //A valid DATA packet has been received
         context->state = TFTP_CLIENT_STATE_DATA;

         //Save the length of the DATA packet
         context->inDataLen = length;
         context->inDataPos = 0;
      }
   }
   else if(context->state == TFTP_CLIENT_STATE_ACK)
   {
      //The host should make sure that the source TID matches the value
      //that was agreed
      if(srcPort == context->serverTid)
      {
         //Check block number
         if(ntohs(dataPacket->block) == context->block)
         {
            //A valid DATA packet has been received
            context->state = TFTP_CLIENT_STATE_DATA;

            //Save the length of the DATA packet
            context->inDataLen = length;
            context->inDataPos = 0;
         }
         else
         {
            //Retransmit ACK packet
            tftpClientRetransmitPacket(context);
         }
      }
   }
}


/**
 * @brief Process incoming ACK packet
 * @param[in] context Pointer to the TFTP client context
 * @param[in] srcPort Source port number
 * @param[in] ackPacket Pointer to the ACK packet
 * @param[in] length Length of the packet, in bytes
 **/

void tftpClientProcessAckPacket(TftpClientContext *context,
   uint16_t srcPort, const TftpAckPacket *ackPacket, size_t length)
{
   //Debug message
   TRACE_DEBUG("TFTP Client: ACK packet received (%" PRIuSIZE " bytes)...\r\n",
      length);

   //Make sure the length of the ACK packet is acceptable
   if(length < sizeof(TftpAckPacket))
      return;

   //Debug message
   TRACE_DEBUG("  Opcode = %" PRIu16 "\r\n", ntohs(ackPacket->opcode));
   TRACE_DEBUG("  Block = %" PRIu16 "\r\n", ntohs(ackPacket->block));

   //Check current state
   if(context->state == TFTP_CLIENT_STATE_WRQ)
   {
      //Check block number
      if(ntohs(ackPacket->block) == context->block)
      {
         //Save the TID chosen by the server
         context->serverTid = srcPort;

         //A valid ACK packet has been received
         context->state = TFTP_CLIENT_STATE_ACK;

         //Flush the output data buffer
         context->outDataLen = 0;
      }
   }
   else if(context->state == TFTP_CLIENT_STATE_DATA)
   {
      //The host should make sure that the source TID matches the value
      //that was agreed
      if(srcPort == context->serverTid)
      {
         //Make sure the ACK is not a duplicate
         if(ntohs(ackPacket->block) == context->block)
         {
            //A valid ACK packet has been received
            context->state = TFTP_CLIENT_STATE_ACK;

            //Flush the output data buffer
            context->outDataLen = 0;
         }
         else
         {
            //Implementations must never resend the current DATA packet on
            //receipt of a duplicate ACK (refer to RFC 1123, section 4.2.3.1)
         }
      }
   }
   else if(context->state == TFTP_CLIENT_STATE_LAST_DATA)
   {
      //The host should make sure that the source TID matches the value
      //that was agreed
      if(srcPort == context->serverTid)
      {
         //Make sure the ACK is not a duplicate
         if(ntohs(ackPacket->block) == context->block)
         {
            //Normal termination of the transfer
            context->state = TFTP_CLIENT_STATE_COMPLETE;

            //Flush the output data buffer
            context->outDataLen = 0;
         }
         else
         {
            //Implementations must never resend the current DATA packet on
            //receipt of a duplicate ACK (refer to RFC 1123, section 4.2.3.1)
         }
      }
   }
}


/**
 * @brief Process incoming ERROR packet
 * @param[in] context Pointer to the TFTP client context
 * @param[in] srcPort Source port number
 * @param[in] errorPacket Pointer to the ERROR packet
 * @param[in] length Length of the packet, in bytes
 **/

void tftpClientProcessErrorPacket(TftpClientContext *context,
   uint16_t srcPort, const TftpErrorPacket *errorPacket, size_t length)
{
   //Debug message
   TRACE_DEBUG("TFTP Client: ERROR packet received (%" PRIuSIZE " bytes)...\r\n",
      length);

   //Make sure the length of the ERROR packet is acceptable
   if(length < sizeof(TftpErrorPacket))
      return;

   //Debug message
   TRACE_DEBUG("  Opcode = %" PRIu16 "\r\n", ntohs(errorPacket->opcode));
   TRACE_DEBUG("  Error Code = %" PRIu16 "\r\n", ntohs(errorPacket->errorCode));

   //Compute the length of the error message
   length -= sizeof(TftpErrorPacket);

   //Make sure the error message is terminated with a zero byte
   if(length > 1 && errorPacket->errorMsg[length - 1] == '\0')
   {
      //Debug message
      TRACE_DEBUG("  Error Msg = %s\r\n", errorPacket->errorMsg);
   }

   //Report an error
   context->state = TFTP_CLIENT_STATE_ERROR;
}


/**
 * @brief Send RRQ packet
 * @param[in] context Pointer to the TFTP client context
 * @param[in] filename NULL-terminated string specifying the filename
 * @param[in] mode NULL-terminated string specifying the transfer mode
 * @return Error code
 **/

error_t tftpClientSendRrqPacket(TftpClientContext *context,
   const char_t *filename, const char_t *mode)
{
   error_t error;
   size_t m;
   size_t n;
   TftpRrqPacket *rrqPacket;

   //Retrieve the length of the filename
   m = osStrlen(filename);
   //Retrieve the length of the transfer mode
   n = osStrlen(mode);

   //Check the length of the filename
   if((m + n) > TFTP_CLIENT_BLOCK_SIZE)
      return ERROR_INVALID_PARAMETER;

   //Point to the buffer where to format the packet
   rrqPacket = (TftpRrqPacket *) context->outPacket;

   //Format RRQ packet
   rrqPacket->opcode = HTONS(TFTP_OPCODE_RRQ);
   osStrcpy(rrqPacket->filename, filename);
   osStrcpy(rrqPacket->filename + m + 1, mode);

   //Compute the length of the RRQ packet
   context->outPacketLen = sizeof(TftpRrqPacket) + n + m + 2;

   //Debug message
   TRACE_DEBUG("TFTP Client: Sending RRQ packet (%" PRIuSIZE " bytes)...\r\n", context->outPacketLen);
   TRACE_DEBUG("  Opcode = %u\r\n", ntohs(rrqPacket->opcode));
   TRACE_DEBUG("  Filename = %s\r\n", rrqPacket->filename);
   TRACE_DEBUG("  Mode = %s\r\n", mode);

   //Send RRQ packet
   error = socketSendTo(context->socket, &context->serverIpAddr,
      context->serverPort, context->outPacket, context->outPacketLen, NULL, 0);

   //Save the time at which the packet was sent
   context->timestamp = osGetSystemTime();
   //Reset retransmission counter
   context->retransmitCount = 0;

   //Return status code
   return error;
}


/**
 * @brief Send WRQ packet
 * @param[in] context Pointer to the TFTP client context
 * @param[in] filename NULL-terminated string specifying the filename
 * @param[in] mode NULL-terminated string specifying the transfer mode
 * @return Error code
 **/

error_t tftpClientSendWrqPacket(TftpClientContext *context,
   const char_t *filename, const char_t *mode)
{
   error_t error;
   size_t m;
   size_t n;
   TftpWrqPacket *wrqPacket;

   //Retrieve the length of the filename
   m = osStrlen(filename);
   //Retrieve the length of the transfer mode
   n = osStrlen(mode);

   //Check the length of the filename
   if((m + n) > TFTP_CLIENT_BLOCK_SIZE)
      return ERROR_INVALID_PARAMETER;

   //Point to the buffer where to format the packet
   wrqPacket = (TftpWrqPacket *) context->outPacket;

   //Format WRQ packet
   wrqPacket->opcode = HTONS(TFTP_OPCODE_WRQ);
   osStrcpy(wrqPacket->filename, filename);
   osStrcpy(wrqPacket->filename + m + 1, mode);

   //Compute the length of the WRQ packet
   context->outPacketLen = sizeof(TftpRrqPacket) + n + m + 2;

   //Debug message
   TRACE_DEBUG("TFTP Client: Sending WRQ packet (%" PRIuSIZE " bytes)...\r\n", context->outPacketLen);
   TRACE_DEBUG("  Opcode = %u\r\n", ntohs(wrqPacket->opcode));
   TRACE_DEBUG("  Filename = %s\r\n", wrqPacket->filename);
   TRACE_DEBUG("  Mode = %s\r\n", mode);

   //Send WRQ packet
   error = socketSendTo(context->socket, &context->serverIpAddr,
      context->serverPort, context->outPacket, context->outPacketLen, NULL, 0);

   //Save the time at which the packet was sent
   context->timestamp = osGetSystemTime();
   //Reset retransmission counter
   context->retransmitCount = 0;

   //Return status code
   return error;
}


/**
 * @brief Send DATA packet
 * @param[in] context Pointer to the TFTP client context
 * @return Error code
 **/

error_t tftpClientSendDataPacket(TftpClientContext *context)
{
   error_t error;
   TftpDataPacket *dataPacket;

   //Point to the buffer where to format the packet
   dataPacket = (TftpDataPacket *) context->outPacket;

   //Format DATA packet
   dataPacket->opcode = HTONS(TFTP_OPCODE_DATA);
   dataPacket->block = htons(context->block);

   //Compute the length of the DATA packet
   context->outPacketLen = sizeof(TftpDataPacket) + context->outDataLen;

   //Debug message
   TRACE_DEBUG("TFTP Client: Sending DATA packet (%" PRIuSIZE " bytes)...\r\n", context->outPacketLen);
   TRACE_DEBUG("  Opcode = %" PRIu16 "\r\n", ntohs(dataPacket->opcode));
   TRACE_DEBUG("  Block = %" PRIu16 "\r\n", ntohs(dataPacket->block));

   //Send DATA packet
   error = socketSendTo(context->socket, &context->serverIpAddr,
      context->serverTid, context->outPacket, context->outPacketLen, NULL, 0);

   //Save the time at which the packet was sent
   context->timestamp = osGetSystemTime();
   //Reset retransmission counter
   context->retransmitCount = 0;

   //Return status code
   return error;
}


/**
 * @brief Send ACK packet
 * @param[in] context Pointer to the TFTP client context
 * @return Error code
 **/

error_t tftpClientSendAckPacket(TftpClientContext *context)
{
   error_t error;
   TftpAckPacket *ackPacket;

   //Point to the buffer where to format the packet
   ackPacket = (TftpAckPacket *) context->outPacket;

   //Format ACK packet
   ackPacket->opcode = HTONS(TFTP_OPCODE_ACK);
   ackPacket->block = htons(context->block);

   //Length of the ACK packet
   context->outPacketLen = sizeof(TftpAckPacket);

   //Debug message
   TRACE_DEBUG("TFTP Client: Sending ACK packet (%" PRIuSIZE " bytes)...\r\n", context->outPacketLen);
   TRACE_DEBUG("  Opcode = %" PRIu16 "\r\n", ntohs(ackPacket->opcode));
   TRACE_DEBUG("  Block = %" PRIu16 "\r\n", ntohs(ackPacket->block));

   //Send ACK packet
   error = socketSendTo(context->socket, &context->serverIpAddr,
      context->serverTid, context->outPacket, context->outPacketLen, NULL, 0);

   //Save the time at which the packet was sent
   context->timestamp = osGetSystemTime();
   //Reset retransmission counter
   context->retransmitCount = 0;

   //Return status code
   return error;
}


/**
 * @brief Send ERROR packet
 * @param[in] context Pointer to the TFTP client context
 * @param[in] errorCode Integer indicating the nature of the error
 * @param[in] errorMsg Error message intended for human consumption
 * @return Error code
 **/

error_t tftpClientSendErrorPacket(TftpClientContext *context,
   uint16_t errorCode, const char_t *errorMsg)
{
   error_t error;
   TftpErrorPacket *errorPacket;

   //Check the length of the error message
   if(osStrlen(errorMsg) >= TFTP_CLIENT_BLOCK_SIZE)
      return ERROR_INVALID_PARAMETER;

   //Point to the buffer where to format the packet
   errorPacket = (TftpErrorPacket *) context->outPacket;

   //Format ERROR packet
   errorPacket->opcode = HTONS(TFTP_OPCODE_ERROR);
   errorPacket->errorCode = htons(errorCode);

   //Copy error message
   osStrcpy(errorPacket->errorMsg, errorMsg);

   //Compute the length of the ERROR packet
   context->outPacketLen = sizeof(TftpErrorPacket) + osStrlen(errorMsg) + 1;

   //Debug message
   TRACE_DEBUG("TFTP Client: Sending ERROR packet (%" PRIuSIZE " bytes)...\r\n", context->outPacketLen);
   TRACE_DEBUG("  Opcode = %" PRIu16 "\r\n", ntohs(errorPacket->opcode));
   TRACE_DEBUG("  Error Code = %" PRIu16 "\r\n", ntohs(errorPacket->errorCode));
   TRACE_DEBUG("  Error Msg = %s\r\n", errorPacket->errorMsg);

   //Send ERROR packet
   error = socketSendTo(context->socket, &context->serverIpAddr,
      context->serverTid, context->outPacket, context->outPacketLen, NULL, 0);

   //Save the time at which the packet was sent
   context->timestamp = osGetSystemTime();
   //Reset retransmission counter
   context->retransmitCount = 0;

   //Return status code
   return error;
}


/**
 * @brief Retransmit the last packet
 * @param[in] context Pointer to the TFTP client context
 * @return Error code
 **/

error_t tftpClientRetransmitPacket(TftpClientContext *context)
{
   error_t error;
   uint16_t destPort;

   //Select the relevant destination port
   if(context->state == TFTP_CLIENT_STATE_RRQ ||
      context->state == TFTP_CLIENT_STATE_WRQ)
   {
      //The client sends its initial request to the known TID 69
      destPort = context->serverPort;
   }
   else
   {
      //Under normal operation, the client uses the TID chosen by the server
      destPort = context->serverTid;
   }

   //Debug message
   TRACE_DEBUG("TFTP Client: Retransmitting packet (%" PRIuSIZE " bytes)...\r\n",
      context->outPacketLen);

   //Retransmit the last packet
   error = socketSendTo(context->socket, &context->serverIpAddr,
      destPort, context->outPacket, context->outPacketLen, NULL, 0);

   //Return status code
   return error;
}

#endif
