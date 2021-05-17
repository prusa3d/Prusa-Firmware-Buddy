/**
 * @file ftp_server_data.c
 * @brief FTP data connection
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
#define TRACE_LEVEL FTP_TRACE_LEVEL

//Dependencies
#include "ftp/ftp_server.h"
#include "ftp/ftp_server_data.h"
#include "ftp/ftp_server_transport.h"
#include "ftp/ftp_server_misc.h"
#include "path.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (FTP_SERVER_SUPPORT == ENABLED)


/**
 * @brief Register data connection events
 * @param[in] connection Pointer to the client connection
 * @param[in] eventDesc Event to be registered
 **/

void ftpServerRegisterDataChannelEvents(FtpClientConnection *connection,
   SocketEventDesc *eventDesc)
{
   //Check the state of the data connection
   if(connection->dataChannel.state == FTP_CHANNEL_STATE_LISTEN)
   {
      //Wait for data to be available for reading
      eventDesc->socket = connection->dataChannel.socket;
      eventDesc->eventMask = SOCKET_EVENT_RX_READY;
   }
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_CONNECT_TLS)
   {
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
      //Any data pending in the send buffer?
      if(tlsIsTxReady(connection->dataChannel.tlsContext))
      {
         //Wait until there is more room in the send buffer
         eventDesc->socket = connection->dataChannel.socket;
         eventDesc->eventMask = SOCKET_EVENT_TX_READY;
      }
      else
      {
         //Wait for data to be available for reading
         eventDesc->socket = connection->dataChannel.socket;
         eventDesc->eventMask = SOCKET_EVENT_RX_READY;
      }
#endif
   }
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_RECEIVE)
   {
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
      //Any data pending in the receive buffer?
      if(connection->dataChannel.tlsContext != NULL &&
         tlsIsRxReady(connection->dataChannel.tlsContext))
      {
         //No need to poll the underlying socket for incoming traffic
         eventDesc->eventFlags = SOCKET_EVENT_RX_READY;
      }
      else
#endif
      {
         //Wait for data to be available for reading
         eventDesc->socket = connection->dataChannel.socket;
         eventDesc->eventMask = SOCKET_EVENT_RX_READY;
      }
   }
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_SEND)
   {
      //Wait until there is more room in the send buffer
      eventDesc->socket = connection->dataChannel.socket;
      eventDesc->eventMask = SOCKET_EVENT_TX_READY;
   }
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_SHUTDOWN_TLS)
   {
      //Wait until there is more room in the send buffer
      eventDesc->socket = connection->dataChannel.socket;
      eventDesc->eventMask = SOCKET_EVENT_TX_READY;
   }
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_WAIT_ACK)
   {
      //Wait for all the data to be transmitted and acknowledged
      eventDesc->socket = connection->dataChannel.socket;
      eventDesc->eventMask = SOCKET_EVENT_TX_ACKED;
   }
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_SHUTDOWN_TX)
   {
      //Wait for the FIN to be acknowledged
      eventDesc->socket = connection->dataChannel.socket;
      eventDesc->eventMask = SOCKET_EVENT_TX_SHUTDOWN;
   }
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_SHUTDOWN_RX)
   {
      //Wait for a FIN to be received
      eventDesc->socket = connection->dataChannel.socket;
      eventDesc->eventMask = SOCKET_EVENT_RX_SHUTDOWN;
   }
}


/**
 * @brief Data connection event handler
 * @param[in] connection Pointer to the client connection
 * @param[in] eventFlags Event to be processed
 **/

void ftpServerProcessDataChannelEvents(FtpClientConnection *connection,
   uint_t eventFlags)
{
   //Check current state
   if(connection->dataChannel.state == FTP_CHANNEL_STATE_LISTEN)
   {
      //Accept data connection
      ftpServerAcceptDataChannel(connection);
   }
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_CONNECT_TLS)
   {
      error_t error;

      //Perform TLS handshake
      error = ftpServerEstablishSecureChannel(&connection->dataChannel);

      //Check status code
      if(error == NO_ERROR)
      {
         //Update the state of the data connection
         if(connection->controlChannel.state == FTP_CHANNEL_STATE_LIST ||
            connection->controlChannel.state == FTP_CHANNEL_STATE_NLST ||
            connection->controlChannel.state == FTP_CHANNEL_STATE_RETR)
         {
            //Prepare to send data
            connection->dataChannel.state = FTP_CHANNEL_STATE_SEND;
         }
         else if(connection->controlChannel.state == FTP_CHANNEL_STATE_STOR ||
            connection->controlChannel.state == FTP_CHANNEL_STATE_APPE)
         {
            //Prepare to receive data
            connection->dataChannel.state = FTP_CHANNEL_STATE_RECEIVE;
         }
         else
         {
            //Data transfer direction is unknown...
            connection->dataChannel.state = FTP_CHANNEL_STATE_IDLE;
         }
      }
      else if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
      {
      }
      else
      {
         //Close the data connection
         ftpServerCloseDataChannel(connection);

         //Release previously allocated resources
         fsCloseFile(connection->file);
         connection->file = NULL;

         //Back to idle state
         connection->controlChannel.state = FTP_CHANNEL_STATE_IDLE;

         //Transfer status
         osStrcpy(connection->response, "451 Transfer aborted\r\n");
         //Debug message
         TRACE_DEBUG("FTP server: %s", connection->response);

         //Number of bytes in the response buffer
         connection->responseLen = osStrlen(connection->response);
         connection->responsePos = 0;
      }
   }
#endif
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_SEND)
   {
      //Send more data to the remote host
      ftpServerWriteDataChannel(connection);
   }
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_RECEIVE)
   {
      //Process incoming data
      ftpServerReadDataChannel(connection);
   }
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_SHUTDOWN_TLS)
   {
      error_t error;

      //Gracefully close TLS session
      error = tlsShutdown(connection->dataChannel.tlsContext);

      //Check status code
      if(error != ERROR_WOULD_BLOCK && error != ERROR_TIMEOUT)
      {
         //Wait for all the data to be transmitted and acknowledged
         connection->dataChannel.state = FTP_CHANNEL_STATE_WAIT_ACK;
      }
   }
#endif
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_WAIT_ACK)
   {
      //Disable transmission
      socketShutdown(connection->dataChannel.socket, SOCKET_SD_SEND);
      //Next state
      connection->dataChannel.state = FTP_CHANNEL_STATE_SHUTDOWN_TX;
   }
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_SHUTDOWN_TX)
   {
      //Disable reception
      socketShutdown(connection->dataChannel.socket, SOCKET_SD_RECEIVE);
      //Next state
      connection->dataChannel.state = FTP_CHANNEL_STATE_SHUTDOWN_RX;
   }
   else if(connection->dataChannel.state == FTP_CHANNEL_STATE_SHUTDOWN_RX)
   {
      //Close the data connection
      ftpServerCloseDataChannel(connection);

      //Back to idle state
      connection->controlChannel.state = FTP_CHANNEL_STATE_IDLE;

      //Transfer status
      osStrcpy(connection->response, "226 Transfer complete\r\n");
      //Debug message
      TRACE_DEBUG("FTP server: %s", connection->response);

      //Number of bytes in the response buffer
      connection->responseLen = osStrlen(connection->response);
      connection->responsePos = 0;
   }
}


/**
 * @brief Open data connection
 * @param[in] connection Pointer to the client connection
 * @return Error code
 **/

error_t ftpServerOpenDataChannel(FtpClientConnection *connection)
{
   error_t error;
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

   //Release previously allocated resources
   ftpServerCloseDataChannel(connection);

   //No port specified?
   if(!connection->remotePort)
      return ERROR_FAILURE;

   //Debug message
   TRACE_INFO("FTP server: Opening data connection with client %s port %" PRIu16 "...\r\n",
      ipAddrToString(&connection->remoteIpAddr, NULL), connection->remotePort);

   //Open data socket
   connection->dataChannel.socket = socketOpen(SOCKET_TYPE_STREAM,
      SOCKET_IP_PROTO_TCP);
   //Failed to open socket?
   if(!connection->dataChannel.socket)
      return ERROR_OPEN_FAILED;

   //Start of exception handling block
   do
   {
      //Force the socket to operate in non-blocking mode
      error = socketSetTimeout(connection->dataChannel.socket, 0);
      //Any error to report?
      if(error)
         break;

      //Adjust the size of the TX buffer
      error = socketSetTxBufferSize(connection->dataChannel.socket,
         FTP_SERVER_MAX_TCP_BUFFER_SIZE);
      //Any error to report?
      if(error)
         break;

      //Adjust the size of the RX buffer
      error = socketSetRxBufferSize(connection->dataChannel.socket,
         FTP_SERVER_MAX_TCP_BUFFER_SIZE);
      //Any error to report?
      if(error)
         break;

      //Associate the socket with the relevant interface
      error = socketBindToInterface(connection->dataChannel.socket,
         connection->interface);
      //Unable to bind the socket to the desired interface?
      if(error)
         break;

      //The server initiates the data connection from port 20
      error = socketBind(connection->dataChannel.socket, &IP_ADDR_ANY,
         context->settings.dataPort);
      //Any error to report?
      if(error)
         break;

      //Establish data connection
      error = socketConnect(connection->dataChannel.socket,
         &connection->remoteIpAddr, connection->remotePort);
      //Any error to report?
      if(error != NO_ERROR && error != ERROR_TIMEOUT)
         break;

      //Connection is being established
      error = NO_ERROR;

      //End of exception handling block
   } while(0);

   //Any error to report?
   if(error)
   {
      //Clean up side effects
      ftpServerCloseDataChannel(connection);
      //Exit immediately
      return error;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Accept data connection
 * @param[in] connection Pointer to the client connection
 **/

void ftpServerAcceptDataChannel(FtpClientConnection *connection)
{
   error_t error;
   Socket *socket;
   IpAddr clientIpAddr;
   uint16_t clientPort;

   //Accept incoming connection
   socket = socketAccept(connection->dataChannel.socket, &clientIpAddr,
      &clientPort);
   //Failure detected?
   if(socket == NULL)
      return;

   //Debug message
   TRACE_INFO("FTP server: Data connection established with client %s port %" PRIu16 "...\r\n",
      ipAddrToString(&clientIpAddr, NULL), clientPort);

   //Close the listening socket
   socketClose(connection->dataChannel.socket);
   //Save socket handle
   connection->dataChannel.socket = socket;

   //Force the socket to operate in non-blocking mode
   error = socketSetTimeout(connection->dataChannel.socket, 0);
   //Any error to report?
   if(error)
   {
      //Clean up side effects
      socketClose(connection->dataChannel.socket);
      //Exit immediately
      return;
   }

#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   //TLS-secured connection?
   if(connection->controlChannel.tlsContext != NULL)
   {
      //Check data transfer direction
      if(connection->controlChannel.state == FTP_CHANNEL_STATE_STOR ||
         connection->controlChannel.state == FTP_CHANNEL_STATE_APPE)
      {
         //TLS initialization
         error = ftpServerOpenSecureChannel(connection->context,
            &connection->dataChannel, FTP_SERVER_TLS_TX_BUFFER_SIZE,
            FTP_SERVER_MAX_TLS_RX_BUFFER_SIZE);
      }
      else
      {
         //TLS initialization
         error = ftpServerOpenSecureChannel(connection->context,
            &connection->dataChannel, FTP_SERVER_TLS_TX_BUFFER_SIZE,
            FTP_SERVER_MIN_TLS_RX_BUFFER_SIZE);
      }

      //Perform TLS handshake
      connection->dataChannel.state = FTP_CHANNEL_STATE_CONNECT_TLS;
   }
   else
#endif
   {
      //Check current state
      if(connection->controlChannel.state == FTP_CHANNEL_STATE_LIST ||
         connection->controlChannel.state == FTP_CHANNEL_STATE_NLST ||
         connection->controlChannel.state == FTP_CHANNEL_STATE_RETR)
      {
         //Prepare to send data
         connection->dataChannel.state = FTP_CHANNEL_STATE_SEND;
      }
      else if(connection->controlChannel.state == FTP_CHANNEL_STATE_STOR ||
         connection->controlChannel.state == FTP_CHANNEL_STATE_APPE)
      {
         //Prepare to receive data
         connection->dataChannel.state = FTP_CHANNEL_STATE_RECEIVE;
      }
      else
      {
         //Data transfer direction is unknown...
         connection->dataChannel.state = FTP_CHANNEL_STATE_IDLE;
      }
   }
}


/**
 * @brief Write data to the data connection
 * @param[in] connection Pointer to the client connection
 **/

void ftpServerWriteDataChannel(FtpClientConnection *connection)
{
   error_t error;
   size_t n;

   //Any data waiting for transmission?
   if(connection->bufferLength > 0)
   {
      //Transmit data
      error = ftpServerWriteChannel(&connection->dataChannel,
         connection->buffer + connection->bufferPos,
         connection->bufferLength, &n, 0);

      //Failed to send data?
      if(error != NO_ERROR && error != ERROR_TIMEOUT)
      {
         //Close the data connection
         ftpServerCloseDataChannel(connection);

         //Release previously allocated resources
         if(connection->file != NULL)
         {
            fsCloseFile(connection->file);
            connection->file = NULL;
         }

         if(connection->dir != NULL)
         {
            fsCloseDir(connection->dir);
            connection->dir = NULL;
         }

         //Back to idle state
         connection->controlChannel.state = FTP_CHANNEL_STATE_IDLE;

         //Transfer status
         osStrcpy(connection->response, "451 Transfer aborted\r\n");
         //Debug message
         TRACE_DEBUG("FTP server: %s", connection->response);

         //Number of bytes in the response buffer
         connection->responseLen = osStrlen(connection->response);
         connection->responsePos = 0;

         //Exit immediately
         return;
      }

      //Advance data pointer
      connection->bufferPos += n;
      //Number of bytes still available in the buffer
      connection->bufferLength -= n;
   }

   //Empty transmission buffer?
   if(connection->bufferLength == 0)
   {
      //File transfer in progress?
      if(connection->controlChannel.state == FTP_CHANNEL_STATE_RETR)
      {
         //Read more data
         error = fsReadFile(connection->file,
            connection->buffer, FTP_SERVER_BUFFER_SIZE, &n);

         //End of stream?
         if(error)
         {
            //Close file
            fsCloseFile(connection->file);
            connection->file = NULL;

#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
            //TLS-secured connection?
            if(connection->dataChannel.tlsContext != NULL)
            {
               //Gracefully close TLS session
               connection->dataChannel.state = FTP_CHANNEL_STATE_SHUTDOWN_TLS;
            }
            else
#endif
            {
               //Wait for all the data to be transmitted and acknowledged
               connection->dataChannel.state = FTP_CHANNEL_STATE_WAIT_ACK;
            }

            //Exit immediately
            return;
         }
      }
      //Directory listing in progress?
      else if(connection->controlChannel.state == FTP_CHANNEL_STATE_LIST ||
         connection->controlChannel.state == FTP_CHANNEL_STATE_NLST)
      {
         uint_t perm;
         char_t *path;
         FsDirEntry dirEntry;

         //Read a new entry from the directory
         error = fsReadDir(connection->dir, &dirEntry);

         //End of stream?
         if(error)
         {
            //Close directory
            fsCloseDir(connection->dir);
            connection->dir = NULL;

#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
            //TLS-secured connection?
            if(connection->dataChannel.tlsContext != NULL)
            {
               //Gracefully close TLS session
               connection->dataChannel.state = FTP_CHANNEL_STATE_SHUTDOWN_TLS;
            }
            else
#endif
            {
               //Wait for all the data to be transmitted and acknowledged
               connection->dataChannel.state = FTP_CHANNEL_STATE_WAIT_ACK;
            }

            //Exit immediately
            return;
         }

         //Point to the scratch buffer
         path = connection->buffer;

         //Get the pathname of the directory being listed
         osStrcpy(path, connection->path);
         //Retrieve the full pathname
         pathCombine(path, dirEntry.name, FTP_SERVER_MAX_PATH_LEN);
         pathCanonicalize(path);

         //Get permissions for the specified file
         perm = ftpServerGetFilePermissions(connection, path);

         //Enforce access rights
         if((perm & FTP_FILE_PERM_LIST) != 0)
         {
            //LIST or NLST command?
            if(connection->controlChannel.state == FTP_CHANNEL_STATE_LIST)
            {
               //Format the directory entry in UNIX-style format
               n = ftpServerFormatDirEntry(&dirEntry, perm, connection->buffer);
            }
            else
            {
               //The server returns a stream of names of files and no other
               //information (refer to RFC 959, section 4.1.3)
               osStrcpy(connection->buffer, dirEntry.name);

               //Check whether the current entry is a directory
               if((dirEntry.attributes & FS_FILE_ATTR_DIRECTORY) != 0)
               {
                  osStrcat(connection->buffer, "/");
               }

               //Terminate the name with a CRLF sequence
               osStrcat(connection->buffer, "\r\n");
               //Calculate the length of the resulting string
               n = osStrlen(connection->buffer);
            }

            //Debug message
            TRACE_DEBUG("FTP server: %s", connection->buffer);
         }
         else
         {
            //Insufficient access rights
            n = 0;
         }
      }
      //Invalid state?
      else
      {
         //The FTP server has encountered a critical error
         ftpServerCloseConnection(connection);
         //Exit immediately
         return;
      }

      //Number of bytes in the buffer
      connection->bufferPos = 0;
      connection->bufferLength = n;
   }
}


/**
 * @brief Read data from the data connection
 * @param[in] connection Pointer to the client connection
 **/

void ftpServerReadDataChannel(FtpClientConnection *connection)
{
   error_t error;
   bool_t eof;
   size_t n;

   //File transfer in progress?
   if(connection->controlChannel.state == FTP_CHANNEL_STATE_STOR ||
      connection->controlChannel.state == FTP_CHANNEL_STATE_APPE)
   {
      //Receive data
      error = ftpServerReadChannel(&connection->dataChannel,
         connection->buffer + connection->bufferPos,
         FTP_SERVER_BUFFER_SIZE - connection->bufferLength, &n, 0);

      //Check status code
      if(error == NO_ERROR || error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
      {
         //Successful read operation
         eof = FALSE;

         //Advance data pointer
         connection->bufferPos += n;
         connection->bufferLength += n;
      }
      else
      {
         //Cannot read more data
         eof = TRUE;
      }

      //Read data until the buffer is full or the end of the file is reached
      if(eof || connection->bufferLength >= FTP_SERVER_BUFFER_SIZE)
      {
         //Any data to be written?
         if(connection->bufferLength > 0)
         {
            //Write data to the specified file
            error = fsWriteFile(connection->file,
               connection->buffer, connection->bufferLength);

            //Any error to report?
            if(error)
            {
               //Close the data connection
               ftpServerCloseDataChannel(connection);

               //Release previously allocated resources
               fsCloseFile(connection->file);
               connection->file = NULL;

               //Back to idle state
               connection->controlChannel.state = FTP_CHANNEL_STATE_IDLE;

               //Transfer status
               osStrcpy(connection->response, "451 Transfer aborted\r\n");
               //Debug message
               TRACE_DEBUG("FTP server: %s", connection->response);

               //Number of bytes in the response buffer
               connection->responseLen = osStrlen(connection->response);
               connection->responsePos = 0;

               //Exit immediately
               return;
            }
         }

         //Flush reception buffer
         connection->bufferLength = 0;
         connection->bufferPos = 0;
      }

      //End of stream?
      if(eof)
      {
         //Close file
         fsCloseFile(connection->file);
         connection->file = NULL;

#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
         //TLS-secured connection?
         if(connection->dataChannel.tlsContext != NULL)
         {
            //Gracefully close TLS session
            connection->dataChannel.state = FTP_CHANNEL_STATE_SHUTDOWN_TLS;
         }
         else
#endif
         {
            //Wait for all the data to be transmitted and acknowledged
            connection->dataChannel.state = FTP_CHANNEL_STATE_WAIT_ACK;
         }
      }
   }
   //Invalid state?
   else
   {
      //The FTP server has encountered a critical error
      ftpServerCloseConnection(connection);
   }
}


/**
 * @brief Close data connection
 * @param[in] connection Pointer to the client connection
 **/

void ftpServerCloseDataChannel(FtpClientConnection *connection)
{
   IpAddr clientIpAddr;
   uint16_t clientPort;

   //Check whether the data connection is active
   if(connection->dataChannel.socket != NULL)
   {
      //Retrieve the address of the peer to which a socket is connected
      socketGetRemoteAddr(connection->dataChannel.socket, &clientIpAddr,
         &clientPort);

      //Check whether the data connection is established
      if(clientPort != 0)
      {
         //Debug message
         TRACE_INFO("FTP server: Closing data connection with client %s port %"
            PRIu16 "...\r\n", ipAddrToString(&clientIpAddr, NULL), clientPort);
      }

#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
      //Valid TLS context?
      if(connection->dataChannel.tlsContext != NULL)
      {
         //Release TLS context
         tlsFree(connection->dataChannel.tlsContext);
         connection->dataChannel.tlsContext = NULL;
      }
#endif

      //Valid socket?
      if(connection->dataChannel.socket != NULL)
      {
         //Close data connection
         socketClose(connection->dataChannel.socket);
         connection->dataChannel.socket = NULL;
      }

      //Re initialize data connection
      connection->passiveMode = FALSE;
      connection->remotePort = 0;

      //Mark the connection as closed
      connection->dataChannel.state = FTP_CHANNEL_STATE_CLOSED;
   }
}

#endif

