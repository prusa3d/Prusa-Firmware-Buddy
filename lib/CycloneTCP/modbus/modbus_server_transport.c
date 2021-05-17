/**
 * @file modbus_server_transport.c
 * @brief Transport protocol abstraction layer
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
#define TRACE_LEVEL MODBUS_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "modbus/modbus_server.h"
#include "modbus/modbus_server_security.h"
#include "modbus/modbus_server_transport.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MODBUS_SERVER_SUPPORT == ENABLED)


/**
 * @brief Accept connection request
 * @param[in] context Pointer to the Modbus/TCP server context
 **/

void modbusServerAcceptConnection(ModbusServerContext *context)
{
   uint_t i;
   Socket *socket;
   IpAddr clientIpAddr;
   uint16_t clientPort;
   ModbusClientConnection *connection;

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
      for(i = 0; i < MODBUS_SERVER_MAX_CONNECTIONS; i++)
      {
         //Check the state of the current connection
         if(context->connection[i].state == MODBUS_CONNECTION_STATE_CLOSED)
         {
            //The current entry is free
            connection = &context->connection[i];
            break;
         }
      }

      //If the connection table runs out of space, then the client's connection
      //request is rejected
      if(connection != NULL)
      {
         //Debug message
         TRACE_INFO("Modbus Server: Connection established with client %s port %"
            PRIu16 "...\r\n", ipAddrToString(&clientIpAddr, NULL), clientPort);

         //Clear the structure describing the connection
         osMemset(connection, 0, sizeof(ModbusClientConnection));

         //Attach Modbus/TCP server context
         connection->context = context;
         //Save socket handle
         connection->socket = socket;
         //Initialize time stamp
         connection->timestamp = osGetSystemTime();

#if (MODBUS_SERVER_TLS_SUPPORT == ENABLED)
         //TLS-secured connection?
         if(context->settings.tlsInitCallback != NULL)
         {
            error_t error;

            //TLS initialization
            error = modbusServerOpenSecureConnection(context, connection);

            //Check status code
            if(!error)
            {
               //Perform TLS handshake
               connection->state = MODBUS_CONNECTION_STATE_CONNECT_TLS;
            }
            else
            {
               //Close connection with the client
               modbusServerCloseConnection(connection);
            }
         }
         else
#endif
         {
            //Wait for incoming Modbus requests
            connection->state = MODBUS_CONNECTION_STATE_RECEIVE;
         }
      }
      else
      {
         //Debug message
         TRACE_INFO("Modbus Server: Connection refused with client %s port %"
            PRIu16 "...\r\n", ipAddrToString(&clientIpAddr, NULL), clientPort);

         //The Modbus/TCP server cannot accept the incoming connection request
         socketClose(socket);
      }
   }
}


/**
 * @brief Shutdown network connection
 * @param[in] connection Pointer to the client connection
 * @return Error code
 **/

error_t modbusServerShutdownConnection(ModbusClientConnection *connection)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

   //Check the state of the connection
   if(connection->state == MODBUS_CONNECTION_STATE_RECEIVE)
   {
#if (MODBUS_SERVER_TLS_SUPPORT == ENABLED)
      //TLS-secured connection?
      if(connection->tlsContext != NULL)
      {
         //Gracefully close TLS connection
         connection->state = MODBUS_CONNECTION_STATE_SHUTDOWN_TLS;
      }
      else
#endif
      {
         //Shutdown transmission
         error = socketShutdown(connection->socket, SOCKET_SD_SEND);
         //Update connection state
         connection->state = MODBUS_CONNECTION_STATE_SHUTDOWN_TX;
      }
   }
   else if(connection->state == MODBUS_CONNECTION_STATE_SHUTDOWN_TLS)
   {
#if (MODBUS_SERVER_TLS_SUPPORT == ENABLED)
      //Gracefully close TLS session
      error = tlsShutdown(connection->tlsContext);

      //Check status code
      if(!error)
      {
         //Shutdown transmission
         error = socketShutdown(connection->socket, SOCKET_SD_SEND);
         //Update connection state
         connection->state = MODBUS_CONNECTION_STATE_SHUTDOWN_TX;
      }
#else
      //Modbus/TCP security is not implemented
      error = ERROR_WRONG_STATE;
#endif
   }
   else if(connection->state == MODBUS_CONNECTION_STATE_SHUTDOWN_TX)
   {
      //Shutdown reception
      error = socketShutdown(connection->socket, SOCKET_SD_RECEIVE);
      //Update connection state
      connection->state = MODBUS_CONNECTION_STATE_SHUTDOWN_RX;
   }
   else
   {
      //Invalid state
      error = ERROR_WRONG_STATE;
   }

   //Return status code
   return error;
}


/**
 * @brief Close network connection
 * @param[in] connection Pointer to the client connection
 **/

void modbusServerCloseConnection(ModbusClientConnection *connection)
{
   //Debug message
   TRACE_INFO("Modbus Server: Closing connection...\r\n");

#if (MODBUS_SERVER_TLS_SUPPORT == ENABLED)
   //Release TLS context
   if(connection->tlsContext != NULL)
   {
      tlsFree(connection->tlsContext);
      connection->tlsContext = NULL;
   }
#endif

   //Close TCP connection
   if(connection->socket != NULL)
   {
      socketClose(connection->socket);
      connection->socket = NULL;
   }

   //Mark the connection as closed
   connection->state = MODBUS_CONNECTION_STATE_CLOSED;
}


/**
 * @brief Send data using the relevant transport protocol
 * @param[in] connection Pointer to the client connection
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of bytes to be transmitted
 * @param[out] written Actual number of bytes written (optional parameter)
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t modbusServerSendData(ModbusClientConnection *connection,
   const void *data, size_t length, size_t *written, uint_t flags)
{
   error_t error;

#if (MODBUS_SERVER_TLS_SUPPORT == ENABLED)
   //TLS-secured connection?
   if(connection->tlsContext != NULL)
   {
      //Send TLS-encrypted data
      error = tlsWrite(connection->tlsContext, data, length, written, flags);
   }
   else
#endif
   {
      //Transmit data
      error = socketSend(connection->socket, data, length, written, flags);
   }

   //Return status code
   return error;
}


/**
 * @brief Receive data using the relevant transport protocol
 * @param[in] connection Pointer to the client connection
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t modbusServerReceiveData(ModbusClientConnection *connection,
   void *data, size_t size, size_t *received, uint_t flags)
{
   error_t error;

#if (MODBUS_SERVER_TLS_SUPPORT == ENABLED)
   //TLS-secured connection?
   if(connection->tlsContext != NULL)
   {
      //Receive TLS-encrypted data
      error = tlsRead(connection->tlsContext, data, size, received, flags);
   }
   else
#endif
   {
      //Receive data
      error = socketReceive(connection->socket, data, size, received, flags);
   }

   //Return status code
   return error;
}

#endif
