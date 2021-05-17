/**
 * @file web_socket_transport.c
 * @brief WebSocket transport layer
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
#define TRACE_LEVEL WEB_SOCKET_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "web_socket/web_socket.h"
#include "web_socket/web_socket_transport.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (WEB_SOCKET_SUPPORT == ENABLED)


/**
 * @brief Open network connection
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketOpenConnection(WebSocket *webSocket)
{
   error_t error;

   //Invalid socket handle?
   if(webSocket->socket == NULL)
   {
      //Open a TCP socket
      webSocket->socket = socketOpen(SOCKET_TYPE_STREAM, SOCKET_IP_PROTO_TCP);
      //Failed to open socket?
      if(webSocket->socket == NULL)
         return ERROR_OPEN_FAILED;

      //Associate the WebSocket with the relevant interface
      error = socketBindToInterface(webSocket->socket, webSocket->interface);
      //Any error to report?
      if(error)
         return error;
   }

   //Set timeout
   error = socketSetTimeout(webSocket->socket, webSocket->timeout);
   //Any error to report?
   if(error)
      return error;

#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
      //TLS-secured connection?
      if(webSocket->tlsInitCallback != NULL)
      {
         TlsConnectionEnd connectionEnd;

         //Allocate TLS context
         webSocket->tlsContext = tlsInit();
         //Failed to allocate TLS context?
         if(webSocket->tlsContext == NULL)
            return ERROR_OUT_OF_MEMORY;

         //Client or server operation?
         if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
            connectionEnd = TLS_CONNECTION_END_CLIENT;
         else
            connectionEnd = TLS_CONNECTION_END_SERVER;

         //Select the relevant operation mode
         error = tlsSetConnectionEnd(webSocket->tlsContext, connectionEnd);
         //Any error to report?
         if(error)
            return error;

         //Bind TLS to the relevant socket
         error = tlsSetSocket(webSocket->tlsContext, webSocket->socket);
         //Any error to report?
         if(error)
            return error;

         //Restore TLS session, if any
         error = tlsRestoreSessionState(webSocket->tlsContext, &webSocket->tlsSession);
         //Any error to report?
         if(error)
            return error;

         //Invoke user-defined callback, if any
         if(webSocket->tlsInitCallback != NULL)
         {
            //Perform TLS related initialization
            error = webSocket->tlsInitCallback(webSocket, webSocket->tlsContext);
            //Any error to report?
            if(error)
               return error;
         }
      }
#endif

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Establish network connection
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] serverIpAddr IP address of the WebSocket server to connect to
 * @param[in] serverPort TCP port number that will be used to establish the
 *   connection
 * @return Error code
 **/

error_t webSocketEstablishConnection(WebSocket *webSocket,
   const IpAddr *serverIpAddr, uint16_t serverPort)
{
   error_t error;

   //Connect to WebSocket server
   error = socketConnect(webSocket->socket, serverIpAddr, serverPort);

#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
   //TLS-secured connection?
   if(webSocket->tlsInitCallback != NULL)
   {
      //Check status code
      if(!error)
      {
         //Establish TLS connection
         error = tlsConnect(webSocket->tlsContext);
      }
   }
#endif

   //Return status code
   return error;
}


/**
 * @brief Shutdown network connection
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketShutdownConnection(WebSocket *webSocket)
{
   error_t error;
   size_t n;

   //Initialize status code
   error = NO_ERROR;

#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
   //Check whether a secure connection is being used
   if(webSocket->tlsContext != NULL)
   {
      //Shutdown TLS connection
      error = tlsShutdown(webSocket->tlsContext);
   }
#endif

   //Check status code
   if(!error)
   {
      //Further transmissions are disallowed
      error = socketShutdown(webSocket->socket, SOCKET_SD_SEND);
   }

   //Receive data until until the peer has also performed an orderly shutdown
   while(!error)
   {
      //Discard data
      error = socketReceive(webSocket->socket, webSocket->rxContext.buffer,
         WEB_SOCKET_BUFFER_SIZE, &n, 0);
   }

   //Check whether the connection has been properly closed
   if(error == ERROR_END_OF_STREAM)
      error = NO_ERROR;

   //Return status code
   return error;
}


/**
 * @brief Close network connection
 * @param[in] webSocket Handle to a WebSocket
 **/

void webSocketCloseConnection(WebSocket *webSocket)
{
#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
   //Check whether a secure connection is being used
   if(webSocket->tlsContext != NULL)
   {
      //Save TLS session
      tlsSaveSessionState(webSocket->tlsContext, &webSocket->tlsSession);

      //Release TLS context
      tlsFree(webSocket->tlsContext);
      webSocket->tlsContext = NULL;
   }
#endif

   //Close TCP connection
   if(webSocket->socket != NULL)
   {
      socketClose(webSocket->socket);
      webSocket->socket = NULL;
   }
}


/**
 * @brief Send data using the relevant transport protocol
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of bytes to be transmitted
 * @param[out] written Actual number of bytes written (optional parameter)
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t webSocketSendData(WebSocket *webSocket, const void *data,
   size_t length, size_t *written, uint_t flags)
{
   error_t error;

#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
   //Check whether a secure connection is being used
   if(webSocket->tlsContext != NULL)
   {
      //Use TLS to transmit data
      error = tlsWrite(webSocket->tlsContext, data, length, written, flags);
   }
   else
#endif
   {
      //Transmit data
      error = socketSend(webSocket->socket, data, length, written, flags);
   }

   //Return status code
   return error;
}


/**
 * @brief Receive data using the relevant transport protocol
 * @param[in] webSocket Handle to a WebSocket
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t webSocketReceiveData(WebSocket *webSocket, void *data,
   size_t size, size_t *received, uint_t flags)
{
   error_t error;

#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
   //Check whether a secure connection is being used
   if(webSocket->tlsContext != NULL)
   {
      //Use TLS to receive data
      error = tlsRead(webSocket->tlsContext, data, size, received, flags);
   }
   else
#endif
   {
      //Receive data
      error = socketReceive(webSocket->socket, data, size, received, flags);
   }

   //Return status code
   return error;
}

#endif
