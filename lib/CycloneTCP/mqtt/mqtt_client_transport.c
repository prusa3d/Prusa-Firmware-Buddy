/**
 * @file mqtt_client_transport.c
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
#define TRACE_LEVEL MQTT_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "core/tcp_misc.h"
#include "mqtt/mqtt_client.h"
#include "mqtt/mqtt_client_packet.h"
#include "mqtt/mqtt_client_transport.h"
#include "mqtt/mqtt_client_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MQTT_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Open network connection
 * @param[in] context Pointer to the MQTT client context
 * @return Error code
 **/

error_t mqttClientOpenConnection(MqttClientContext *context)
{
   error_t error;

   //TCP transport protocol?
   if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TCP)
   {
      //Open a TCP socket
      context->socket = socketOpen(SOCKET_TYPE_STREAM, SOCKET_IP_PROTO_TCP);

      //Valid socket handle?
      if(context->socket != NULL)
      {
         //Associate the socket with the relevant interface
         error = socketBindToInterface(context->socket, context->interface);
      }
      else
      {
         //Report an error
         error = ERROR_OPEN_FAILED;
      }
   }
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   //TLS transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TLS)
   {
      //Open a TCP socket
      context->socket = socketOpen(SOCKET_TYPE_STREAM, SOCKET_IP_PROTO_TCP);

      //Valid socket handle?
      if(context->socket != NULL)
      {
         //Associate the socket with the relevant interface
         error = socketBindToInterface(context->socket, context->interface);

         //Check status code
         if(!error)
         {
            //Allocate TLS context
            context->tlsContext = tlsInit();

            //Valid TLS handle?
            if(context->tlsContext != NULL)
            {
               //Select client operation mode
               error = tlsSetConnectionEnd(context->tlsContext,
                  TLS_CONNECTION_END_CLIENT);

               //Check status code
               if(!error)
               {
                  //Bind TLS to the relevant socket
                  error = tlsSetSocket(context->tlsContext, context->socket);
               }

               //Check status code
               if(!error)
               {
                  //Restore TLS session, if any
                  error = tlsRestoreSessionState(context->tlsContext,
                     &context->tlsSession);
               }

               //Check status code
               if(!error)
               {
                  //Invoke user-defined callback, if any
                  if(context->callbacks.tlsInitCallback != NULL)
                  {
                     //Perform TLS related initialization
                     error = context->callbacks.tlsInitCallback(context,
                        context->tlsContext);
                  }
               }
            }
            else
            {
               //Report an error
               error = ERROR_OPEN_FAILED;
            }
         }
      }
      else
      {
         //Report an error
         error = ERROR_OPEN_FAILED;
      }
   }
#endif
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   //WebSocket transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WS)
   {
      //Open a WebSocket
      context->webSocket = webSocketOpen();

      //Valid WebSocket handle?
      if(context->webSocket != NULL)
      {
         //Associate the WebSocket with the relevant interface
         error = webSocketBindToInterface(context->webSocket,
            context->interface);
      }
      else
      {
         //Report an error
         error = ERROR_OPEN_FAILED;
      }
   }
#endif
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED && MQTT_CLIENT_TLS_SUPPORT == ENABLED && \
   WEB_SOCKET_TLS_SUPPORT == ENABLED)
   //Secure WebSocket transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WSS)
   {
      //Open a WebSocket
      context->webSocket = webSocketOpen();

      //Valid WebSocket handle?
      if(context->webSocket != NULL)
      {
         //Associate the WebSocket with the relevant interface
         error = webSocketBindToInterface(context->webSocket,
            context->interface);

         //Check status code
         if(!error)
         {
            //Register TLS initialization callback
            error = webSocketRegisterTlsInitCallback(context->webSocket,
               (WebSocketTlsInitCallback) context->callbacks.tlsInitCallback);
         }
      }
      else
      {
         //Report an error
         error = ERROR_OPEN_FAILED;
      }
   }
#endif
   //Unknown transport protocol?
   else
   {
      //Report an error
      error = ERROR_INVALID_PROTOCOL;
   }

   //Return status code
   return error;
}


/**
 * @brief Establish network connection
 * @param[in] context Pointer to the MQTT client context
 * @param[in] serverIpAddr IP address of the MQTT server to connect to
 * @param[in] serverPort TCP port number that will be used to establish the
 *   connection
 * @return Error code
 **/

error_t mqttClientEstablishConnection(MqttClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort)
{
   error_t error;

   //TCP transport protocol?
   if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TCP)
   {
      //Set timeout
      error = socketSetTimeout(context->socket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //Connect to the MQTT server using TCP
         error = socketConnect(context->socket, serverIpAddr, serverPort);
      }
   }
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   //TLS transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TLS)
   {
      //Set timeout
      error = socketSetTimeout(context->socket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //Connect to the MQTT server using TCP
         error = socketConnect(context->socket, serverIpAddr, serverPort);
      }

      //Check status code
      if(!error)
      {
         //Perform TLS handshake
         error = tlsConnect(context->tlsContext);
      }

      //Successful connection?
      if(!error)
      {
         //Save TLS session
         error = tlsSaveSessionState(context->tlsContext, &context->tlsSession);
      }
   }
#endif
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   //WebSocket transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WS ||
      context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WSS)
   {
      //Set timeout
      error = webSocketSetTimeout(context->webSocket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //Set the hostname of the remote server
         error = webSocketSetHost(context->webSocket, context->settings.host);
      }

      //Check status code
      if(!error)
      {
         //The client MUST include "mqtt" in the list of WebSocket
         //sub-protocols it offers
         error = webSocketSetSubProtocol(context->webSocket, "mqtt");
      }

      //Check status code
      if(!error)
      {
         //Connect to the MQTT server using WebSocket
         error = webSocketConnect(context->webSocket, serverIpAddr,
            serverPort, context->settings.uri);
      }
   }
#endif
   //Unknown transport protocol?
   else
   {
      //Report an error
      error = ERROR_INVALID_PROTOCOL;
   }

   //Return status code
   return error;
}


/**
 * @brief Shutdown network connection
 * @param[in] context Pointer to the MQTT client context
 * @return Error code
 **/

error_t mqttClientShutdownConnection(MqttClientContext *context)
{
   error_t error;

   //TCP transport protocol?
   if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TCP)
   {
      //Set timeout
      error = socketSetTimeout(context->socket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //Shutdown TCP connection
         error = socketShutdown(context->socket, SOCKET_SD_BOTH);
      }
   }
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   //TLS transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TLS)
   {
      //Set timeout
      error = socketSetTimeout(context->socket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //Shutdown TLS session
         error = tlsShutdown(context->tlsContext);
      }

      //Check status code
      if(!error)
      {
         //Shutdown TCP connection
         error = socketShutdown(context->socket, SOCKET_SD_BOTH);
      }
   }
#endif
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   //WebSocket transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WS ||
      context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WSS)
   {
      //Set timeout
      error = webSocketSetTimeout(context->webSocket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //Shutdown WebSocket connection
         error = webSocketShutdown(context->webSocket);
      }
   }
#endif
   //Unknown transport protocol?
   else
   {
      //Report an error
      error = ERROR_INVALID_PROTOCOL;
   }

   //Return status code
   return error;
}


/**
 * @brief Close network connection
 * @param[in] context Pointer to the MQTT client context
 **/

void mqttClientCloseConnection(MqttClientContext *context)
{
   //TCP transport protocol?
   if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TCP)
   {
      //Close TCP connection
      if(context->socket != NULL)
      {
         socketClose(context->socket);
         context->socket = NULL;
      }
   }
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   //TLS transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TLS)
   {
      //Release TLS context
      if(context->tlsContext != NULL)
      {
         tlsFree(context->tlsContext);
         context->tlsContext = NULL;
      }

      //Close TCP connection
      if(context->socket != NULL)
      {
         socketClose(context->socket);
         context->socket = NULL;
      }
   }
#endif
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   //WebSocket transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WS ||
      context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WSS)
   {
      //Close WebSocket connection
      if(context->webSocket != NULL)
      {
         webSocketClose(context->webSocket);
         context->webSocket = NULL;
      }
   }
#endif
}


/**
 * @brief Send data using the relevant transport protocol
 * @param[in] context Pointer to the MQTT client context
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of bytes to be transmitted
 * @param[out] written Actual number of bytes written (optional parameter)
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t mqttClientSendData(MqttClientContext *context,
   const void *data, size_t length, size_t *written, uint_t flags)
{
   error_t error;

   //TCP transport protocol?
   if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TCP)
   {
      //Set timeout
      error = socketSetTimeout(context->socket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //Transmit data
         error = socketSend(context->socket, data, length, written, flags);
      }
   }
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   //TLS transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TLS)
   {
      //Set timeout
      error = socketSetTimeout(context->socket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //Transmit data
         error = tlsWrite(context->tlsContext, data, length, written, flags);
      }
   }
#endif
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   //WebSocket transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WS ||
      context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WSS)
   {
      //Set timeout
      error = webSocketSetTimeout(context->webSocket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //MQTT control packets must be sent in WebSocket binary data frames
         error = webSocketSend(context->webSocket, data, length,
            WS_FRAME_TYPE_BINARY, written);
      }
   }
#endif
   //Unknown transport protocol?
   else
   {
      //Report an error
      error = ERROR_INVALID_PROTOCOL;
   }

   //Return status code
   return error;
}


/**
 * @brief Receive data using the relevant transport protocol
 * @param[in] context Pointer to the MQTT client context
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t mqttClientReceiveData(MqttClientContext *context,
   void *data, size_t size, size_t *received, uint_t flags)
{
   error_t error;

   //No data has been read yet
   *received = 0;

   //TCP transport protocol?
   if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TCP)
   {
      //Set timeout
      error = socketSetTimeout(context->socket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //Receive data
         error = socketReceive(context->socket, data, size, received, flags);
      }
   }
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   //TLS transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TLS)
   {
      //Set timeout
      error = socketSetTimeout(context->socket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //Receive data
         error = tlsRead(context->tlsContext, data, size, received, flags);
      }
   }
#endif
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   //WebSocket transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WS ||
      context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WSS)
   {
      WebSocketFrameType type;

      //Set timeout
      error = webSocketSetTimeout(context->webSocket, context->settings.timeout);

      //Check status code
      if(!error)
      {
         //Receive data
         error = webSocketReceive(context->webSocket, data, size, &type, received);
      }

      //Check status code
      if(!error)
      {
         //MQTT control packets must be sent in WebSocket binary data frames. If
         //any other type of data frame is received the recipient must close the
         //network connection
         if(type != WS_FRAME_TYPE_BINARY && type != WS_FRAME_TYPE_CONTINUATION)
            error = ERROR_INVALID_TYPE;
      }
   }
#endif
   //Unknown transport protocol?
   else
   {
      //Report an error
      error = ERROR_INVALID_PROTOCOL;
   }

   //Return status code
   return error;
}


/**
 * @brief Wait for incoming data
 * @param[in] context Pointer to the MQTT client context
 * @param[in] timeout Maximum time to wait before returning
 * @return Error code
 **/

error_t mqttClientWaitForData(MqttClientContext *context, systime_t timeout)
{
   uint_t event;

   //TCP transport protocol?
   if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TCP)
   {
      //Get exclusive access
      osAcquireMutex(&netMutex);
      //Wait for some data to be available for reading
      event = tcpWaitForEvents(context->socket, SOCKET_EVENT_RX_READY, timeout);
      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   //TLS transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_TLS)
   {
      //Sanity check
      if(context->tlsContext == NULL)
         return ERROR_FAILURE;

      //Check whether some data is pending in the receive buffer
      if(context->tlsContext->rxBufferLen > 0)
      {
         //No need to poll the underlying socket for incoming traffic...
         event = SOCKET_EVENT_RX_READY;
      }
      else
      {
         //Get exclusive access
         osAcquireMutex(&netMutex);
         //Wait for some data to be available for reading
         event = tcpWaitForEvents(context->socket, SOCKET_EVENT_RX_READY, timeout);
         //Release exclusive access
         osReleaseMutex(&netMutex);
      }
   }
#endif
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   //WebSocket transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WS)
   {
      //Sanity check
      if(context->webSocket == NULL)
         return ERROR_FAILURE;

      //Get exclusive access
      osAcquireMutex(&netMutex);
      //Wait for some data to be available for reading
      event = tcpWaitForEvents(context->webSocket->socket, SOCKET_EVENT_RX_READY, timeout);
      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
#endif
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED && WEB_SOCKET_TLS_SUPPORT)
   //Secure WebSocket transport protocol?
   else if(context->settings.transportProtocol == MQTT_TRANSPORT_PROTOCOL_WSS)
   {
      //Sanity check
      if(context->webSocket == NULL || context->webSocket->tlsContext == NULL)
         return ERROR_FAILURE;

      //Check whether some data is pending in the receive buffer
      if(context->webSocket->tlsContext->rxBufferLen > 0)
      {
         //No need to poll the underlying socket for incoming traffic...
         event = SOCKET_EVENT_RX_READY;
      }
      else
      {
         //Get exclusive access
         osAcquireMutex(&netMutex);
         //Wait for some data to be available for reading
         event = tcpWaitForEvents(context->webSocket->socket, SOCKET_EVENT_RX_READY, timeout);
         //Release exclusive access
         osReleaseMutex(&netMutex);
      }
   }
#endif
   //Unknown transport protocol?
   else
   {
      //Report an error
      return ERROR_INVALID_PROTOCOL;
   }

   //Check whether some data is available for reading
   if(event == SOCKET_EVENT_RX_READY)
      return NO_ERROR;
   else
      return ERROR_TIMEOUT;
}

#endif
