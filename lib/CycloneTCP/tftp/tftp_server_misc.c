/**
 * @file tftp_server_misc.c
 * @brief Helper functions for TFTP server
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
#include "tftp/tftp_server.h"
#include "tftp/tftp_server_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (TFTP_SERVER_SUPPORT == ENABLED)


/**
 * @brief Handle periodic operations
 * @param[in] context Pointer to the TFTP server context
 **/

void tftpServerTick(TftpServerContext *context)
{
   uint_t i;
   systime_t time;
   TftpClientConnection *connection;

   //Get current time
   time = osGetSystemTime();

   //Handle periodic operations
   for(i = 0; i < TFTP_SERVER_MAX_CONNECTIONS; i++)
   {
      //Point to the structure describing the current connection
      connection = &context->connection[i];

      //Check current state
      if(connection->state == TFTP_STATE_READING ||
         connection->state == TFTP_STATE_WRITING ||
         connection->state == TFTP_STATE_READ_COMPLETE)
      {
         //Check current time
         if(timeCompare(time, connection->timestamp + TFTP_SERVER_TIMEOUT) >= 0)
         {
            //Handle retransmissions
            if(connection->retransmitCount < TFTP_SERVER_MAX_RETRIES)
            {
               //Retransmit last packet
               tftpServerRetransmitPacket(connection);

               //Save the time at which the packet was sent
               connection->timestamp = osGetSystemTime();
               //Increment retransmission counter
               connection->retransmitCount++;
            }
            else
            {
               //Close connection
               tftpServerCloseConnection(connection);
            }
         }
      }
      else if(connection->state == TFTP_STATE_WRITE_COMPLETE)
      {
         //The host sending the final ACK will wait for a while before terminating
         //in order to retransmit the final ACK if it has been lost
         if(timeCompare(time, connection->timestamp + TFTP_SERVER_FINAL_DELAY) >= 0)
         {
            //Close connection
            tftpServerCloseConnection(connection);
         }
      }
   }
}


/**
 * @brief Create client connection
 * @param[in] context Pointer to the TFTP server context
 * @param[in] clientIpAddr IP address of the client
 * @param[in] clientPort Port number used by the client
 * @return Pointer to the structure describing the connection
 **/

TftpClientConnection *tftpServerOpenConnection(TftpServerContext *context,
   const IpAddr *clientIpAddr, uint16_t clientPort)
{
   error_t error;
   uint_t i;
   systime_t time;
   TftpClientConnection *connection;
   TftpClientConnection *oldestConnection;

   //Get current time
   time = osGetSystemTime();

   //Keep track of the oldest connection that is waiting to retransmit
   //the final ACK
   oldestConnection = NULL;

   //Loop through the connection table
   for(i = 0; i < TFTP_SERVER_MAX_CONNECTIONS; i++)
   {
      //Point to the current entry
      connection = &context->connection[i];

      //Check the state of the current connection
      if(connection->state == TFTP_STATE_CLOSED)
      {
         //The current entry is available
         break;
      }
      else if(connection->state == TFTP_STATE_WRITE_COMPLETE)
      {
         //Keep track of the oldest connection that is waiting to retransmit
         //the final ACK
         if(oldestConnection == NULL)
         {
            oldestConnection = connection;
         }
         else if((time - connection->timestamp) > (time - oldestConnection->timestamp))
         {
            oldestConnection = connection;
         }
      }
   }

   //The oldest connection that is waiting to retransmit the final ACK can be
   //reused when the connection table runs out of space
   if(i >= TFTP_SERVER_MAX_CONNECTIONS)
   {
      //Close the oldest connection
      tftpServerCloseConnection(oldestConnection);
      //Reuse the connection
      connection = oldestConnection;
   }

   //Failed to create a new connection?
   if(connection == NULL)
      return NULL;

   //Clear the structure describing the connection
   osMemset(connection, 0, sizeof(TftpClientConnection));

   //Open a UDP socket
   connection->socket = socketOpen(SOCKET_TYPE_DGRAM, SOCKET_IP_PROTO_UDP);

   //Failed to open socket?
   if(connection->socket == NULL)
      return NULL;

   //Associate the socket with the relevant interface
   error = socketBindToInterface(connection->socket, context->settings.interface);

   //Any error to report?
   if(error)
   {
      //Clean up side effects
      socketClose(connection->socket);
      connection->socket = NULL;
      //Exit immediately
      return NULL;
   }

   //Connect the socket to the remote TFTP client
   error = socketConnect(connection->socket, clientIpAddr, clientPort);

   //Any error to report?
   if(error)
   {
      //Clean up side effects
      socketClose(connection->socket);
      connection->socket = NULL;
      //Exit immediately
      return NULL;
   }

   //Reference to the TFTP server settings
   connection->settings = &context->settings;
   //Update connection state
   connection->state = TFTP_STATE_OPEN;

   //Pointer to the structure describing the connection
   return connection;
}


/**
 * @brief Close client connection
 * @param[in] connection Pointer to the client connection
 **/

void tftpServerCloseConnection(TftpClientConnection *connection)
{
   //Valid connection?
   if(connection != NULL)
   {
      //Debug message
      TRACE_INFO("TFTP Server: Closing connection...\r\n");

      //Any active connection?
      if(connection->socket != NULL)
      {
         //Close UDP socket
         socketClose(connection->socket);
         connection->socket = NULL;
      }

      //Check whether a read or write operation is in progress...
      if(connection->file != NULL)
      {
         //Properly close the file before closing the connection
         if(connection->settings->closeFileCallback != NULL)
         {
            //Invoke user callback function
            connection->settings->closeFileCallback(connection->file);
         }

         //Mark the file as closed
         connection->file = NULL;
      }

      //Mark the connection as closed
      connection->state = TFTP_STATE_CLOSED;
   }
}


/**
 * @brief Accept connection request
 * @param[in] context Pointer to the TFTP server context
 **/

void tftpServerAcceptRequest(TftpServerContext *context)
{
   error_t error;
   size_t length;
   uint16_t opcode;
   IpAddr clientIpAddr;
   uint16_t clientPort;

   //Read incoming TFTP packet
   error = socketReceiveFrom(context->socket, &clientIpAddr, &clientPort,
      context->packet, TFTP_SERVER_MAX_PACKET_SIZE, &length, 0);

   //Failed to read packet?
   if(error)
      return;

   //Debug message
   TRACE_INFO("TFTP Server: Accepting connection from %s port %" PRIu16 "...\r\n",
      ipAddrToString(&clientIpAddr, NULL), clientPort);

   //Sanity check
   if(length < sizeof(uint16_t))
      return;

   //Retrieve TFTP packet type
   opcode = LOAD16BE(context->packet);

   //Read request received?
   if(opcode == TFTP_OPCODE_RRQ)
   {
      //Process RRQ packet
      tftpServerProcessRrqPacket(context, &clientIpAddr,
         clientPort, (TftpRrqPacket *) context->packet, length);
   }
   //Write request received?
   else if(opcode == TFTP_OPCODE_WRQ)
   {
      //Process WRQ packet
      tftpServerProcessWrqPacket(context, &clientIpAddr,
         clientPort, (TftpWrqPacket *) context->packet, length);
   }
   //Invalid request received?
   else
   {
      //Discard incoming packet
   }
}


/**
 * @brief Process incoming packet
 * @param[in] context Pointer to the TFTP server context
 * @param[in] connection Pointer to the client connection
 **/

void tftpServerProcessPacket(TftpServerContext *context,
   TftpClientConnection *connection)
{
   error_t error;
   size_t length;
   uint16_t opcode;
   IpAddr clientIpAddr;
   uint16_t clientPort;

   //Read incoming TFTP packet
   error = socketReceiveFrom(connection->socket, &clientIpAddr, &clientPort,
      context->packet, TFTP_SERVER_MAX_PACKET_SIZE, &length, 0);

   //Failed to read packet?
   if(error)
      return;

   //Sanity check
   if(length < sizeof(uint16_t))
      return;

   //Retrieve TFTP packet type
   opcode = LOAD16BE(context->packet);

   //Data packet received?
   if(opcode == TFTP_OPCODE_DATA)
   {
      //Process DATA packet
      tftpServerProcessDataPacket(connection,
         (TftpDataPacket *) context->packet, length);
   }
   //Acknowledgment packet received?
   else if(opcode == TFTP_OPCODE_ACK)
   {
      //Process ACK packet
      tftpServerProcessAckPacket(connection,
         (TftpAckPacket *) context->packet, length);
   }
   //Error packet received?
   else if(opcode == TFTP_OPCODE_ERROR)
   {
      //Process ERROR packet
      tftpServerProcessErrorPacket(connection,
         (TftpErrorPacket *) context->packet, length);
   }
   //Invalid packet received?
   else
   {
      //Discard incoming packet
   }
}


/**
 * @brief Process incoming RRQ packet
 * @param[in] context Pointer to the TFTP server context
 * @param[in] clientIpAddr IP address of the client
 * @param[in] clientPort Port number used by the client
 * @param[in] rrqPacket Pointer to the RRQ packet
 * @param[in] length Length of the packet, in bytes
 **/

void tftpServerProcessRrqPacket(TftpServerContext *context, const IpAddr *clientIpAddr,
   uint16_t clientPort, const TftpRrqPacket *rrqPacket, size_t length)
{
   const char_t *mode;
   TftpClientConnection *connection;

   //Debug message
   TRACE_DEBUG("TFTP Server: RRQ packet received (%" PRIuSIZE " bytes)...\r\n",
      length);

   //Make sure the length of the RRQ packet is acceptable
   if(length <= sizeof(TftpRrqPacket))
      return;

   //Compute the number of bytes that follows the 2-byte opcode
   length -= sizeof(TftpRrqPacket);

   //Point to the incoming RRQ packet
   rrqPacket = (TftpRrqPacket *) context->packet;

   //Malformed RRQ packet?
   if(rrqPacket->filename[length - 1] != '\0')
      return;

   //Compute the length of the mode string
   length -= osStrlen(rrqPacket->filename) + 1;

   //Malformed RRQ packet?
   if(length == 0)
      return;

   //Point to the mode string
   mode = rrqPacket->filename + osStrlen(rrqPacket->filename) + 1;

   //Debug message
   TRACE_DEBUG("  Opcode = %u\r\n", ntohs(rrqPacket->opcode));
   TRACE_DEBUG("  Filename = %s\r\n", rrqPacket->filename);
   TRACE_DEBUG("  Mode = %s\r\n", mode);

   //Create a new connection
   connection = tftpServerOpenConnection(context, clientIpAddr, clientPort);

   //Any error to report?
   if(connection == NULL)
      return;

   //Open the specified file for reading
   if(context->settings.openFileCallback != NULL)
   {
      //Invoke user callback function
      connection->file = context->settings.openFileCallback(rrqPacket->filename,
         mode, FALSE);
   }
   else
   {
      //No callback function defined
      connection->file = NULL;
   }

   //Check if the file was successfully opened
   if(connection->file != NULL)
   {
      //The read operation is in progress...
      connection->state = TFTP_STATE_READING;
      //Initialize block number
      connection->block = 1;

      //Send the first DATA packet
      tftpServerSendDataPacket(connection);
   }
   else
   {
      //If the reply is an error packet, then the request has been denied
      tftpServerSendErrorPacket(connection, TFTP_ERROR_NOT_DEFINED,
         "Failed to open file");

      //Close the connection
      tftpServerCloseConnection(connection);
   }
}


/**
 * @brief Process incoming WRQ packet
 * @param[in] context Pointer to the TFTP server context
 * @param[in] clientIpAddr IP address of the client
 * @param[in] clientPort Port number used by the client
 * @param[in] wrqPacket Pointer to the WRQ packet
 * @param[in] length Length of the packet, in bytes
 **/

void tftpServerProcessWrqPacket(TftpServerContext *context, const IpAddr *clientIpAddr,
   uint16_t clientPort, const TftpWrqPacket *wrqPacket, size_t length)
{
   const char_t *mode;
   TftpClientConnection *connection;

   //Debug message
   TRACE_DEBUG("TFTP Server: WRQ packet received (%" PRIuSIZE " bytes)...\r\n",
      length);

   //Make sure the length of the WRQ packet is acceptable
   if(length <= sizeof(TftpWrqPacket))
      return;

   //Compute the number of bytes that follows the 2-byte opcode
   length -= sizeof(TftpWrqPacket);

   //Point to the incoming WRQ packet
   wrqPacket = (TftpWrqPacket *) context->packet;

   //Malformed WRQ packet?
   if(wrqPacket->filename[length - 1] != '\0')
      return;

   //Compute the length of the mode string
   length -= osStrlen(wrqPacket->filename) + 1;

   //Malformed WRQ packet?
   if(length == 0)
      return;

   //Point to the mode string
   mode = wrqPacket->filename + osStrlen(wrqPacket->filename) + 1;

   //Debug message
   TRACE_DEBUG("  Opcode = %u\r\n", ntohs(wrqPacket->opcode));
   TRACE_DEBUG("  Filename = %s\r\n", wrqPacket->filename);
   TRACE_DEBUG("  Mode = %s\r\n", mode);

   //Create a new connection
   connection = tftpServerOpenConnection(context, clientIpAddr, clientPort);

   //Any error to report?
   if(connection == NULL)
      return;

   //Open the specified file for writing
   if(context->settings.openFileCallback != NULL)
   {
      //Invoke user callback function
      connection->file = context->settings.openFileCallback(wrqPacket->filename,
         mode, TRUE);
   }
   else
   {
      //No callback function defined
      connection->file = NULL;
   }

   //Check if the file was successfully opened
   if(connection->file != NULL)
   {
      //The write operation is in progress...
      connection->state = TFTP_STATE_WRITING;
      //Initialize block number
      connection->block = 0;

      //The positive response to a write request is an acknowledgment
      //packet with block number zero
      tftpServerSendAckPacket(connection);

      //Increment block number
      connection->block++;
   }
   else
   {
      //If the reply is an error packet, then the request has been denied
      tftpServerSendErrorPacket(connection, TFTP_ERROR_NOT_DEFINED,
         "Failed to open file");

      //Close the connection
      tftpServerCloseConnection(connection);
   }
}


/**
 * @brief Process incoming DATA packet
 * @param[in] connection Pointer to the client connection
 * @param[in] dataPacket Pointer to the DATA packet
 * @param[in] length Length of the packet, in bytes
 **/

void tftpServerProcessDataPacket(TftpClientConnection *connection,
   const TftpDataPacket *dataPacket, size_t length)
{
   error_t error;
   size_t offset;

   //Debug message
   TRACE_DEBUG("TFTP Server: DATA packet received (%" PRIuSIZE " bytes)...\r\n",
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
   if(connection->state == TFTP_STATE_WRITING)
   {
      //Check block number
      if(ntohs(dataPacket->block) == connection->block)
      {
         //Write data to the output file
         if(connection->settings->writeFileCallback != NULL)
         {
            //Calculate the offset relative to the beginning of the file
            offset = (connection->block - 1) * TFTP_SERVER_BLOCK_SIZE;

            //Invoke user callback function
            error = connection->settings->writeFileCallback(connection->file,
               offset, dataPacket->data, length);
         }
         else
         {
            //No callback function defined
            error = ERROR_WRITE_FAILED;
         }

         //Check status code
         if(!error)
         {
            //Acknowledge the DATA packet
            tftpServerSendAckPacket(connection);

            //Increment block number
            connection->block++;

            //A data packet of less than 512 bytes signals termination of the transfer
            if(length < TFTP_SERVER_BLOCK_SIZE)
            {
               //Properly close the file
               if(connection->settings->closeFileCallback != NULL)
               {
                  //Invoke user callback function
                  connection->settings->closeFileCallback(connection->file);
               }

               //Mark the file as closed
               connection->file = NULL;

               //The host sending the final ACK will wait for a while before terminating
               //in order to retransmit the final ACK if it has been lost
               connection->state = TFTP_STATE_WRITE_COMPLETE;

               //Save current time
               connection->timestamp = osGetSystemTime();
            }
         }
         else
         {
            //An error occurs during the transfer
            tftpServerSendErrorPacket(connection, TFTP_ERROR_NOT_DEFINED,
               "Failed to write file");

            //A TFTP server may terminate after sending an error message
            tftpServerCloseConnection(connection);
         }
      }
      else
      {
         //Retransmit ACK packet
         tftpServerRetransmitPacket(connection);
      }
   }
   else if(connection->state == TFTP_STATE_WRITE_COMPLETE)
   {
      //The acknowledger will know that the ACK has been lost if it
      //receives the final DATA packet again
      tftpServerRetransmitPacket(connection);
   }
}


/**
 * @brief Process incoming ACK packet
 * @param[in] connection Pointer to the client connection
 * @param[in] ackPacket Pointer to the ACK packet
 * @param[in] length Length of the packet, in bytes
 **/

void tftpServerProcessAckPacket(TftpClientConnection *connection,
   const TftpAckPacket *ackPacket, size_t length)
{
   //Debug message
   TRACE_DEBUG("TFTP Server: ACK packet received (%" PRIuSIZE " bytes)...\r\n",
      length);

   //Make sure the length of the ACK packet is acceptable
   if(length < sizeof(TftpAckPacket))
      return;

   //Debug message
   TRACE_DEBUG("  Opcode = %" PRIu16 "\r\n", ntohs(ackPacket->opcode));
   TRACE_DEBUG("  Block = %" PRIu16 "\r\n", ntohs(ackPacket->block));

   //Check current state
   if(connection->state == TFTP_STATE_READING)
   {
      //Make sure the ACK is not a duplicate
      if(ntohs(ackPacket->block) == connection->block)
      {
         //The block number increases by one for each new block of data
         connection->block++;

         //Send DATA packet
         tftpServerSendDataPacket(connection);
      }
      else
      {
         //Implementations must never resend the current DATA packet on
         //receipt of a duplicate ACK (refer to RFC 1123, section 4.2.3.1)
      }
   }
   else if(connection->state == TFTP_STATE_READ_COMPLETE)
   {
      //Make sure the ACK is not a duplicate
      if(ntohs(ackPacket->block) == connection->block)
      {
         //The host sending the last DATA must retransmit it until the packet is
         //acknowledged or the sending host times out. If the response is an ACK,
         //the transmission was completed successfully
         tftpServerCloseConnection(connection);
      }
   }
}


/**
 * @brief Process incoming ERROR packet
 * @param[in] connection Pointer to the client connection
 * @param[in] errorPacket Pointer to the ERROR packet
 * @param[in] length Length of the packet, in bytes
 **/

void tftpServerProcessErrorPacket(TftpClientConnection *connection,
   const TftpErrorPacket *errorPacket, size_t length)
{
   //Debug message
   TRACE_DEBUG("TFTP Server: ERROR packet received (%" PRIuSIZE " bytes)...\r\n",
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

   //Close connection
   tftpServerCloseConnection(connection);
}


/**
 * @brief Send DATA packet
 * @param[in] connection Pointer to the client connection
 * @return Error code
 **/

error_t tftpServerSendDataPacket(TftpClientConnection *connection)
{
   error_t error;
   size_t offset;
   TftpDataPacket *dataPacket;

   //Point to the buffer where to format the packet
   dataPacket = (TftpDataPacket *) connection->packet;

   //Format DATA packet
   dataPacket->opcode = HTONS(TFTP_OPCODE_DATA);
   dataPacket->block = htons(connection->block);

   //Read more data from the input file
   if(connection->settings->readFileCallback != NULL)
   {
      //Calculate the offset relative to the beginning of the file
      offset = (connection->block - 1) * TFTP_SERVER_BLOCK_SIZE;

      //Invoke user callback function
      error = connection->settings->readFileCallback(connection->file, offset,
         dataPacket->data, TFTP_SERVER_BLOCK_SIZE, &connection->packetLen);
   }
   else
   {
      //No callback function defined
      error = ERROR_READ_FAILED;
   }

   //End of file?
   if(error == ERROR_END_OF_FILE || error == ERROR_END_OF_STREAM)
   {
      //Catch exception
      error = NO_ERROR;
      //This is the the last block of data
      connection->packetLen = 0;
   }

   //Check status code
   if(!error)
   {
      //A data packet of less than 512 bytes signals termination of the transfer
      if(connection->packetLen < TFTP_SERVER_BLOCK_SIZE)
      {
         //Properly close the file
         if(connection->settings->closeFileCallback != NULL)
         {
            //Invoke user callback function
            connection->settings->closeFileCallback(connection->file);
         }

         //Mark the file as closed
         connection->file = NULL;

         //The host sending the last DATA must retransmit it until the packet
         //is acknowledged or the sending host times out
         connection->state = TFTP_STATE_READ_COMPLETE;
      }

      //Length of the DATA packet
      connection->packetLen += sizeof(TftpAckPacket);

      //Debug message
      TRACE_DEBUG("TFTP Server: Sending DATA packet (%" PRIuSIZE " bytes)...\r\n", connection->packetLen);
      TRACE_DEBUG("  Opcode = %" PRIu16 "\r\n", ntohs(dataPacket->opcode));
      TRACE_DEBUG("  Block = %" PRIu16 "\r\n", ntohs(dataPacket->block));

      //Send DATA packet
      error = socketSend(connection->socket, connection->packet,
         connection->packetLen, NULL, 0);

      //Save the time at which the packet was sent
      connection->timestamp = osGetSystemTime();
      //Reset retransmission counter
      connection->retransmitCount = 0;
   }
   else
   {
      //An error occurs during the transfer
      tftpServerSendErrorPacket(connection, TFTP_ERROR_NOT_DEFINED,
         "Failed to read file");

      //A TFTP server may terminate after sending an error message
      tftpServerCloseConnection(connection);
   }

   //Return status code
   return error;
}


/**
 * @brief Send ACK packet
 * @param[in] connection Pointer to the client connection
 * @return Error code
 **/

error_t tftpServerSendAckPacket(TftpClientConnection *connection)
{
   error_t error;
   TftpAckPacket *ackPacket;

   //Point to the buffer where to format the packet
   ackPacket = (TftpAckPacket *) connection->packet;

   //Format ACK packet
   ackPacket->opcode = HTONS(TFTP_OPCODE_ACK);
   ackPacket->block = htons(connection->block);

   //Length of the ACK packet
   connection->packetLen = sizeof(TftpAckPacket);

   //Debug message
   TRACE_DEBUG("TFTP Server: Sending ACK packet (%" PRIuSIZE " bytes)...\r\n", connection->packetLen);
   TRACE_DEBUG("  Opcode = %" PRIu16 "\r\n", ntohs(ackPacket->opcode));
   TRACE_DEBUG("  Block = %" PRIu16 "\r\n", ntohs(ackPacket->block));

   //Send ACK packet
   error = socketSend(connection->socket, connection->packet,
      connection->packetLen, NULL, 0);

   //Save the time at which the packet was sent
   connection->timestamp = osGetSystemTime();
   //Reset retransmission counter
   connection->retransmitCount = 0;

   //Return status code
   return error;
}


/**
 * @brief Send ERROR packet
 * @param[in] connection Pointer to the client connection
 * @param[in] errorCode Integer indicating the nature of the error
 * @param[in] errorMsg Error message intended for human consumption
 * @return Error code
 **/

error_t tftpServerSendErrorPacket(TftpClientConnection *connection,
   uint16_t errorCode, const char_t *errorMsg)
{
   error_t error;
   TftpErrorPacket *errorPacket;

   //Check the length of the error message
   if(osStrlen(errorMsg) >= TFTP_SERVER_BLOCK_SIZE)
      return ERROR_INVALID_PARAMETER;

   //Point to the buffer where to format the packet
   errorPacket = (TftpErrorPacket *) connection->packet;

   //Format ERROR packet
   errorPacket->opcode = HTONS(TFTP_OPCODE_ERROR);
   errorPacket->errorCode = htons(errorCode);

   //Copy error message
   osStrcpy(errorPacket->errorMsg, errorMsg);

   //Length of the ERROR packet
   connection->packetLen = sizeof(TftpErrorPacket) + osStrlen(errorMsg) + 1;

   //Debug message
   TRACE_DEBUG("TFTP Server: Sending ERROR packet (%" PRIuSIZE " bytes)...\r\n", connection->packetLen);
   TRACE_DEBUG("  Opcode = %" PRIu16 "\r\n", ntohs(errorPacket->opcode));
   TRACE_DEBUG("  Error Code = %" PRIu16 "\r\n", ntohs(errorPacket->errorCode));
   TRACE_DEBUG("  Error Msg = %s\r\n", errorPacket->errorMsg);

   //Send ERROR packet
   error = socketSend(connection->socket, connection->packet,
      connection->packetLen, NULL, 0);

   //Save the time at which the packet was sent
   connection->timestamp = osGetSystemTime();
   //Reset retransmission counter
   connection->retransmitCount = 0;

   //Return status code
   return error;
}


/**
 * @brief Retransmit the last packet
 * @param[in] connection Pointer to the client connection
 * @return Error code
 **/

error_t tftpServerRetransmitPacket(TftpClientConnection *connection)
{
   error_t error;

   //Debug message
   TRACE_DEBUG("TFTP Server: Retransmitting packet (%" PRIuSIZE " bytes)...\r\n",
      connection->packetLen);

   //Retransmit the last packet
   error = socketSend(connection->socket, connection->packet,
      connection->packetLen, NULL, 0);

   //Return status code
   return error;
}

#endif
