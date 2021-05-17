/**
 * @file mqtt_sn_client_transport.c
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
#define TRACE_LEVEL MQTT_SN_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "mqtt_sn/mqtt_sn_client.h"
#include "mqtt_sn/mqtt_sn_client_transport.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MQTT_SN_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Open network connection
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] secure Secure/non-secure connection
 * @return Error code
 **/

error_t mqttSnClientOpenConnection(MqttSnClientContext *context, bool_t secure)
{
   error_t error;

   //Open a UDP socket
   context->socket = socketOpen(SOCKET_TYPE_DGRAM, SOCKET_IP_PROTO_UDP);
   //Failed to open socket?
   if(context->socket == NULL)
      return ERROR_OPEN_FAILED;

   //Associate the socket with the relevant interface
   error = socketBindToInterface(context->socket, context->interface);
   //Any error to report?
   if(error)
      return error;

   //Set timeout
   error = socketSetTimeout(context->socket, MQTT_SN_CLIENT_TICK_INTERVAL);
   //Any error to report?
   if(error)
      return error;

#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)
   //DTLS transport protocol?
   if(secure && context->transportProtocol == MQTT_SN_TRANSPORT_PROTOCOL_DTLS)
   {
      //Allocate DTLS context
      context->dtlsContext = tlsInit();
      //Failed to allocate DTLS context?
      if(context->dtlsContext == NULL)
         return ERROR_OPEN_FAILED;

      //Select client operation mode
      error = tlsSetConnectionEnd(context->dtlsContext,
         TLS_CONNECTION_END_CLIENT);
      //Any error to report?
      if(error)
         return error;

      //Set the transport protocol to be used (DTLS)
      error = tlsSetTransportProtocol(context->dtlsContext,
         TLS_TRANSPORT_PROTOCOL_DATAGRAM);
      //Any error to report?
      if(error)
         return error;

      //Bind DTLS to the relevant socket
      error = tlsSetSocket(context->dtlsContext, context->socket);
      //Any error to report?
      if(error)
         return error;

      //Set timeout
      error = tlsSetTimeout(context->dtlsContext, MQTT_SN_CLIENT_TICK_INTERVAL);
      //Any error to report?
      if(error)
         return error;

      //Restore DTLS session, if any
      error = tlsRestoreSessionState(context->dtlsContext, &context->dtlsSession);
      //Any error to report?
      if(error)
         return error;

      //Invoke user-defined callback, if any
      if(context->dtlsInitCallback != NULL)
      {
         //Perform DTLS related initialization
         error = context->dtlsInitCallback(context, context->dtlsContext);
         //Any error to report?
         if(error)
            return error;
      }
   }
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Establish network connection
 * @param[in] context Pointer to the MQTT-SN client context
 * @return Error code
 **/

error_t mqttSnClientEstablishConnection(MqttSnClientContext *context)
{
#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)
   //DTLS transport protocol?
   if(context->transportProtocol == MQTT_SN_TRANSPORT_PROTOCOL_DTLS)
   {
      error_t error;

      //Only accept datagrams from the specified MQTT-SN gateway
      error = socketConnect(context->socket, &context->gwIpAddr,
         context->gwPort);
      //Any error to report?
      if(error)
         return error;

      //Perform DTLS handshake
      error = tlsConnect(context->dtlsContext);
      //Any error to report?
      if(error)
         return error;

      //Save DTLS session
      error = tlsSaveSessionState(context->dtlsContext, &context->dtlsSession);
      //Any error to report?
      if(error)
         return error;
   }
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Shutdown network connection
 * @param[in] context Pointer to the MQTT-SN client context
 * @return Error code
 **/

error_t mqttSnClientShutdownConnection(MqttSnClientContext *context)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)
   //DTLS transport protocol?
   if(context->transportProtocol == MQTT_SN_TRANSPORT_PROTOCOL_DTLS)
   {
      //Shutdown DTLS session
      error = tlsShutdown(context->dtlsContext);
   }
#endif

   //Return status code
   return error;
}


/**
 * @brief Close network connection
 * @param[in] context Pointer to the MQTT-SN client context
 **/

void mqttSnClientCloseConnection(MqttSnClientContext *context)
{
#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)
   //DTLS transport protocol?
   if(context->transportProtocol == MQTT_SN_TRANSPORT_PROTOCOL_DTLS)
   {
      //Valid DTLS context?
      if(context->dtlsContext != NULL)
      {
         //Release DTLS context
         tlsFree(context->dtlsContext);
         context->dtlsContext = NULL;
      }
   }
#endif

   //Valid socket?
   if(context->socket != NULL)
   {
      //Close UDP socket
      socketClose(context->socket);
      context->socket = NULL;
   }
}


/**
 * @brief Broadcast a datagram
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] destIpAddr Destination IP address
 * @param[in] destPort Destination port number
 * @param[in] data Pointer to a buffer containing the datagram to be transmitted
 * @param[in] length Length of the datagram, in bytes
 * @return Error code
 **/

error_t mqttSnClientBroadcastDatagram(MqttSnClientContext *context,
   const IpAddr *destIpAddr, uint16_t destPort, const void *data,
   size_t length)
{
   //Transmit datagram
   return socketSendTo(context->socket, destIpAddr, destPort, data, length,
      NULL, 0);
}


/**
 * @brief Send a datagram
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] data Pointer to a buffer containing the datagram to be transmitted
 * @param[in] length Length of the datagram, in bytes
 * @return Error code
 **/

error_t mqttSnClientSendDatagram(MqttSnClientContext *context,
   const void *data, size_t length)
{
   error_t error;

#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)
   //DTLS transport protocol?
   if(context->transportProtocol == MQTT_SN_TRANSPORT_PROTOCOL_DTLS)
   {
      //Transmit datagram
      error = tlsWrite(context->dtlsContext, data, length, NULL, 0);
   }
   else
#endif
   //UDP transport protocol?
   {
      //Transmit datagram
      error = socketSendTo(context->socket, &context->gwIpAddr,
         context->gwPort, data, length, NULL, 0);
   }

   //Return status code
   return error;
}


/**
 * @brief Receive a datagram
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[out] srcIpAddr Source IP address
 * @param[out] srcPort Source port number
 * @param[out] data Buffer into which the received datagram will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @param[in] timeout Maximum time to wait before returning
 * @return Error code
 **/

error_t mqttSnClientReceiveDatagram(MqttSnClientContext *context,
   IpAddr *srcIpAddr, uint16_t *srcPort, void *data, size_t size,
   size_t *received, systime_t timeout)
{
   error_t error;

   //No data has been read yet
   *received = 0;

   //Set timeout
   socketSetTimeout(context->socket, timeout);

#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)
   //DTLS transport protocol?
   if(context->transportProtocol == MQTT_SN_TRANSPORT_PROTOCOL_DTLS)
   {
      //Set timeout
      tlsSetTimeout(context->dtlsContext, timeout);

      //Receive datagram
      error = tlsRead(context->dtlsContext, data, size, received, 0);

      //Use the gateway's address as source address
      *srcIpAddr = context->gwIpAddr;
      *srcPort = context->gwPort;
   }
   else
#endif
   //UDP transport protocol?
   {
      //Receive datagram
      error = socketReceiveFrom(context->socket, srcIpAddr, srcPort, data,
         size, received, 0);
   }

   //Return status code
   return error;
}

#endif
