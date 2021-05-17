/**
 * @file ftp_server_control.c
 * @brief FTP control connection
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
#include "ftp/ftp_server_commands.h"
#include "ftp/ftp_server_control.h"
#include "ftp/ftp_server_transport.h"
#include "ftp/ftp_server_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (FTP_SERVER_SUPPORT == ENABLED)


/**
 * @brief Register control connection events
 * @param[in] connection Pointer to the client connection
 * @param[in] eventDesc Event to be registered
 **/

void ftpServerRegisterControlChannelEvents(FtpClientConnection *connection,
   SocketEventDesc *eventDesc)
{
   //Check the state of the control connection
   if(connection->controlChannel.state == FTP_CHANNEL_STATE_CONNECT_TLS)
   {
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
      //Any data pending in the send buffer?
      if(tlsIsTxReady(connection->controlChannel.tlsContext))
      {
         //Wait until there is more room in the send buffer
         eventDesc->socket = connection->controlChannel.socket;
         eventDesc->eventMask = SOCKET_EVENT_TX_READY;
      }
      else
      {
         //Wait for data to be available for reading
         eventDesc->socket = connection->controlChannel.socket;
         eventDesc->eventMask = SOCKET_EVENT_RX_READY;
      }
#endif
   }
   else if(connection->responseLen > 0)
   {
      //Wait until there is more room in the send buffer
      eventDesc->socket = connection->controlChannel.socket;
      eventDesc->eventMask = SOCKET_EVENT_TX_READY;
   }
   else if(connection->controlChannel.state == FTP_CHANNEL_STATE_AUTH_TLS_2)
   {
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
      //Any data pending in the send buffer?
      if(tlsIsTxReady(connection->controlChannel.tlsContext))
      {
         //Wait until there is more room in the send buffer
         eventDesc->socket = connection->controlChannel.socket;
         eventDesc->eventMask = SOCKET_EVENT_TX_READY;
      }
      else
      {
         //Wait for data to be available for reading
         eventDesc->socket = connection->controlChannel.socket;
         eventDesc->eventMask = SOCKET_EVENT_RX_READY;
      }
#endif
   }
   else if(connection->controlChannel.state == FTP_CHANNEL_STATE_WAIT_ACK)
   {
      //Wait for all the data to be transmitted and acknowledged
      eventDesc->socket = connection->controlChannel.socket;
      eventDesc->eventMask = SOCKET_EVENT_TX_ACKED;
   }
   else if(connection->controlChannel.state == FTP_CHANNEL_STATE_SHUTDOWN_TX)
   {
      //Wait for the FIN to be acknowledged
      eventDesc->socket = connection->controlChannel.socket;
      eventDesc->eventMask = SOCKET_EVENT_TX_SHUTDOWN;
   }
   else if(connection->controlChannel.state == FTP_CHANNEL_STATE_SHUTDOWN_RX)
   {
      //Wait for a FIN to be received
      eventDesc->socket = connection->controlChannel.socket;
      eventDesc->eventMask = SOCKET_EVENT_RX_SHUTDOWN;
   }
   else
   {
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
      //Any data pending in the receive buffer?
      if(connection->controlChannel.tlsContext != NULL &&
         tlsIsRxReady(connection->controlChannel.tlsContext))
      {
         //No need to poll the underlying socket for incoming traffic
         eventDesc->eventFlags = SOCKET_EVENT_RX_READY;
      }
      else
#endif
      {
         //Wait for data to be available for reading
         eventDesc->socket = connection->controlChannel.socket;
         eventDesc->eventMask = SOCKET_EVENT_RX_READY;
      }
   }
}


/**
 * @brief Control connection event handler
 * @param[in] connection Pointer to the client connection
 * @param[in] eventFlags Event to be processed
 **/

void ftpServerProcessControlChannelEvents(FtpClientConnection *connection,
   uint_t eventFlags)
{
   error_t error;
   size_t n;
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   //TLS session establishment in progress?
   if(connection->controlChannel.state == FTP_CHANNEL_STATE_CONNECT_TLS ||
      connection->controlChannel.state == FTP_CHANNEL_STATE_AUTH_TLS_2)
   {
      //Perform TLS handshake
      error = ftpServerEstablishSecureChannel(&connection->controlChannel);

      //Check status code
      if(error == NO_ERROR)
      {
         //Update the state of the control connection
         connection->controlChannel.state = FTP_CHANNEL_STATE_IDLE;
      }
      else if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
      {
      }
      else
      {
         //Close connection with the client
         ftpServerCloseConnection(connection);
      }
   }
   else
#endif
   {
      //Check event flags
      if(eventFlags == SOCKET_EVENT_TX_READY)
      {
         //Transmit data
         error = ftpServerWriteChannel(&connection->controlChannel,
            connection->response + connection->responsePos,
            connection->responseLen, &n, 0);

         //Check status code
         if(error == NO_ERROR || error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
         {
            //Advance data pointer
            connection->responsePos += n;
            //Number of bytes still available in the response buffer
            connection->responseLen -= n;

            //Check whether the AUTH response has been transmitted
            if(connection->responseLen == 0 &&
               connection->controlChannel.state == FTP_CHANNEL_STATE_AUTH_TLS_1)
            {
               //TLS initialization
               error = ftpServerOpenSecureChannel(context,
                  &connection->controlChannel, FTP_SERVER_TLS_TX_BUFFER_SIZE,
                  FTP_SERVER_MIN_TLS_RX_BUFFER_SIZE);

               //Check status code
               if(!error)
               {
                  //Perform TLS handshake
                  connection->controlChannel.state = FTP_CHANNEL_STATE_AUTH_TLS_2;
               }
               else
               {
                  //Close connection with the client
                  ftpServerCloseConnection(connection);
               }
            }
         }
         else
         {
            //Close connection with the client
            ftpServerCloseConnection(connection);
         }
      }
      else if(eventFlags == SOCKET_EVENT_RX_READY)
      {
         //Receive data
         error = ftpServerReadChannel(&connection->controlChannel,
            connection->command + connection->commandLen,
            FTP_SERVER_MAX_LINE_LEN - connection->commandLen, &n, 0);

         //Check status code
         if(error == NO_ERROR || error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
         {
            //Number of bytes available in the command buffer
            connection->commandLen += n;
            //Process incoming command
            ftpServerProcessCommand(connection);
         }
         else if(error == ERROR_END_OF_STREAM)
         {
            //Gracefully disconnect from the remote host
            connection->controlChannel.state = FTP_CHANNEL_STATE_WAIT_ACK;
         }
         else
         {
            //Close connection with the client
            ftpServerCloseConnection(connection);
         }
      }
      else if(eventFlags == SOCKET_EVENT_TX_ACKED)
      {
         //Disable transmission
         socketShutdown(connection->controlChannel.socket, SOCKET_SD_SEND);
         //Next state
         connection->controlChannel.state = FTP_CHANNEL_STATE_SHUTDOWN_TX;
      }
      else if(eventFlags == SOCKET_EVENT_TX_SHUTDOWN)
      {
         //Disable reception
         socketShutdown(connection->controlChannel.socket, SOCKET_SD_RECEIVE);
         //Next state
         connection->controlChannel.state = FTP_CHANNEL_STATE_SHUTDOWN_RX;
      }
      else if(eventFlags == SOCKET_EVENT_RX_SHUTDOWN)
      {
         //Properly close connection
         ftpServerCloseConnection(connection);
      }
   }
}


/**
 * @brief Accept control connection
 * @param[in] context Pointer to the FTP server context
 **/

void ftpServerAcceptControlChannel(FtpServerContext *context)
{
   error_t error;
   uint_t i;
   Socket *socket;
   IpAddr clientIpAddr;
   uint16_t clientPort;
   FtpClientConnection *connection;

   //Accept incoming connection
   socket = socketAccept(context->socket, &clientIpAddr, &clientPort);

   //Make sure the socket handle is valid
   if(socket != NULL)
   {
      //Force the socket to operate in non-blocking mode
      socketSetTimeout(socket, 0);

      //Initialize pointer
      connection = NULL;

      //Loop through the connection table
      for(i = 0; i < context->settings.maxConnections; i++)
      {
         //Check the state of the current connection
         if(context->connections[i].controlChannel.state == FTP_CHANNEL_STATE_CLOSED &&
            context->connections[i].dataChannel.state == FTP_CHANNEL_STATE_CLOSED)
         {
            //The current entry is free
            connection = &context->connections[i];
            break;
         }
      }

      //If the connection table runs out of space, then the client's connection
      //request is rejected
      if(connection != NULL)
      {
         //Clear the structure describing the connection
         osMemset(connection, 0, sizeof(FtpClientConnection));

         //Attach FTP server context
         connection->context = context;
         //Underlying network interface
         connection->interface = socketGetInterface(socket);
         //Save socket handle
         connection->controlChannel.socket = socket;
         //Initialize time stamp
         connection->timestamp = osGetSystemTime();
         //Set home directory
         osStrcpy(connection->homeDir, context->settings.rootDir);
         //Set current directory
         osStrcpy(connection->currentDir, context->settings.rootDir);
         //Format greeting message
         osStrcpy(connection->response, "220 Service ready for new user\r\n");

         //Any registered callback?
         if(context->settings.connectCallback != NULL)
         {
            //Invoke user callback function
            error = context->settings.connectCallback(connection, &clientIpAddr,
               clientPort);
         }
         else
         {
            //No callback function defined
            error = NO_ERROR;
         }

         //Check status code
         if(!error)
         {
            //Debug message
            TRACE_INFO("FTP Server: Control connection established with client %s port %"
               PRIu16 "...\r\n", ipAddrToString(&clientIpAddr, NULL), clientPort);

            //Debug message
            TRACE_DEBUG("FTP server: %s", connection->response);

            //Number of bytes in the response buffer
            connection->responseLen = osStrlen(connection->response);
            connection->responsePos = 0;

            //Implicit TLS mode supported by the server?
            if((context->settings.mode & FTP_SERVER_MODE_IMPLICIT_TLS) != 0)
            {
               //TLS initialization
               error = ftpServerOpenSecureChannel(context,
                  &connection->controlChannel, FTP_SERVER_TLS_TX_BUFFER_SIZE,
                  FTP_SERVER_MIN_TLS_RX_BUFFER_SIZE);

               //Check status code
               if(!error)
               {
                  //Perform TLS handshake
                  connection->controlChannel.state = FTP_CHANNEL_STATE_CONNECT_TLS;
               }
               else
               {
                  //Close connection with the client
                  ftpServerCloseConnection(connection);
               }
            }
            else
            {
               //Enter default state
               connection->controlChannel.state = FTP_CHANNEL_STATE_IDLE;
            }
         }
         else
         {
            //The connection attempt has been refused
            osMemset(connection, 0, sizeof(FtpClientConnection));
         }
      }
      else
      {
         //The connection table runs out of space
         error = ERROR_OUT_OF_RESOURCES;
      }

      //Check status code
      if(error)
      {
         //Debug message
         TRACE_INFO("FTP Server: Connection refused with client %s port %"
            PRIu16 "...\r\n", ipAddrToString(&clientIpAddr, NULL), clientPort);

         //The FTP server cannot accept the incoming connection request
         socketClose(socket);
      }
   }
}


/**
 * @brief Close control connection
 * @param[in] connection Pointer to the client connection
 **/

void ftpServerCloseControlChannel(FtpClientConnection *connection)
{
   IpAddr clientIpAddr;
   uint16_t clientPort;
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

   //Check whether the control connection is active
   if(connection->controlChannel.socket != NULL)
   {
      //Retrieve the address of the peer to which a socket is connected
      socketGetRemoteAddr(connection->controlChannel.socket, &clientIpAddr,
         &clientPort);

      //Debug message
      TRACE_INFO("FTP server: Closing control connection with client %s port %"
         PRIu16 "...\r\n", ipAddrToString(&clientIpAddr, NULL), clientPort);

      //Any registered callback?
      if(context->settings.disconnectCallback != NULL)
      {
         //Invoke user callback function
         context->settings.disconnectCallback(connection, &clientIpAddr,
            clientPort);
      }

#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
      //Valid TLS context?
      if(connection->controlChannel.tlsContext != NULL)
      {
         //Release TLS context
         tlsFree(connection->controlChannel.tlsContext);
         connection->controlChannel.tlsContext = NULL;
      }
#endif

      //Valid socket?
      if(connection->controlChannel.socket != NULL)
      {
         //Close control connection
         socketClose(connection->controlChannel.socket);
         connection->controlChannel.socket = NULL;
      }

      //Mark the connection as closed
      connection->controlChannel.state = FTP_CHANNEL_STATE_CLOSED;
   }
}

#endif
