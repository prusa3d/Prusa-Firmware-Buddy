/**
 * @file mqtt_client.c
 * @brief MQTT client
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
#include "mqtt/mqtt_client.h"
#include "mqtt/mqtt_client_packet.h"
#include "mqtt/mqtt_client_transport.h"
#include "mqtt/mqtt_client_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MQTT_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Initialize MQTT client context
 * @param[in] context Pointer to the MQTT client context
 * @return Error code
 **/

error_t mqttClientInit(MqttClientContext *context)
{
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   error_t error;
#endif

   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Clear MQTT client context
   osMemset(context, 0, sizeof(MqttClientContext));

#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   //Initialize TLS session state
   error = tlsInitSessionState(&context->tlsSession);
   //Any error to report?
   if(error)
      return error;
#endif

   //Default protocol version
   context->settings.version = MQTT_VERSION_3_1_1;
   //Default transport protocol
   context->settings.transportProtocol = MQTT_TRANSPORT_PROTOCOL_TCP;
   //Default keep-alive time interval
   context->settings.keepAlive = MQTT_CLIENT_DEFAULT_KEEP_ALIVE;
   //Default communication timeout
   context->settings.timeout = MQTT_CLIENT_DEFAULT_TIMEOUT;

#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   //Default resource name (for WebSocket connections only)
   osStrcpy(context->settings.uri, "/");
#endif

   //Initialize MQTT client state
   context->state = MQTT_CLIENT_STATE_DISCONNECTED;
   //Initialize packet identifier
   context->packetId = 0;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Initialize callback structure
 * @param[in] callbacks Pointer to a structure that contains callback functions
 **/

void mqttClientInitCallbacks(MqttClientCallbacks *callbacks)
{
   //Initialize callback structure
   osMemset(callbacks, 0, sizeof(MqttClientCallbacks));
}


/**
 * @brief Register MQTT client callbacks
 * @param[in] context Pointer to the MQTT client context
 * @param[in] callbacks Pointer to a structure that contains callback functions
 * @return Error code
 **/

error_t mqttClientRegisterCallbacks(MqttClientContext *context,
   const MqttClientCallbacks *callbacks)
{
   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Attach callback functions
   context->callbacks = *callbacks;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set the MQTT protocol version to be used
 * @param[in] context Pointer to the MQTT client context
 * @param[in] version MQTT protocol version (3.1 or 3.1.1)
 * @return Error code
 **/

error_t mqttClientSetVersion(MqttClientContext *context, MqttVersion version)
{
   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save the MQTT protocol version to be used
   context->settings.version = version;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set the transport protocol to be used
 * @param[in] context Pointer to the MQTT client context
 * @param[in] transportProtocol Transport protocol to be used (TCP, TLS,
 *   WebSocket, or secure WebSocket)
 * @return Error code
 **/

error_t mqttClientSetTransportProtocol(MqttClientContext *context,
   MqttTransportProtocol transportProtocol)
{
   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save the transport protocol to be used
   context->settings.transportProtocol = transportProtocol;

   //Successful processing
   return NO_ERROR;
}


#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)

/**
 * @brief Register TLS initialization callback function
 * @param[in] context Pointer to the MQTT- client context
 * @param[in] callback TLS initialization callback function
 * @return Error code
 **/

error_t mqttClientRegisterTlsInitCallback(MqttClientContext *context,
   MqttClientTlsInitCallback callback)
{
   //Check parameters
   if(context == NULL || callback == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save callback function
   context->callbacks.tlsInitCallback = callback;

   //Successful processing
   return NO_ERROR;
}

#endif


/**
 * @brief Register publish callback function
 * @param[in] context Pointer to the MQTT client context
 * @param[in] callback Callback function to be called when a PUBLISH message
 *   is received
 * @return Error code
 **/

error_t mqttClientRegisterPublishCallback(MqttClientContext *context,
   MqttClientPublishCallback callback)
{
   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save callback function
   context->callbacks.publishCallback = callback;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set communication timeout
 * @param[in] context Pointer to the MQTT client context
 * @param[in] timeout Timeout value, in seconds
 * @return Error code
 **/

error_t mqttClientSetTimeout(MqttClientContext *context, systime_t timeout)
{
   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save timeout value
   context->settings.timeout = timeout;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set keep-alive value
 * @param[in] context Pointer to the MQTT client context
 * @param[in] keepAlive Maximum time interval that is permitted to elapse
 *   between the point at which the client finishes transmitting one control
 *   packet and the point it starts sending the next
 * @return Error code
 **/

error_t mqttClientSetKeepAlive(MqttClientContext *context, uint16_t keepAlive)
{
   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save keep-alive value
   context->settings.keepAlive = keepAlive;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set the domain name of the server (for virtual hosting)
 * @param[in] context Pointer to the MQTT client context
 * @param[in] host NULL-terminated string containing the hostname
 * @return Error code
 **/

error_t mqttClientSetHost(MqttClientContext *context, const char_t *host)
{
   //Check parameters
   if(context == NULL || host == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the length of the hostname is acceptable
   if(osStrlen(host) > MQTT_CLIENT_MAX_HOST_LEN)
      return ERROR_INVALID_LENGTH;

#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   //Save hostname (for WebSocket connections only)
   osStrcpy(context->settings.host, host);
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set the name of the resource being requested
 * @param[in] context Pointer to the MQTT client context
 * @param[in] uri NULL-terminated string that contains the resource name
 * @return Error code
 **/

error_t mqttClientSetUri(MqttClientContext *context, const char_t *uri)
{
   //Check parameters
   if(context == NULL || uri == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the length of the resource name is acceptable
   if(osStrlen(uri) > MQTT_CLIENT_MAX_URI_LEN)
      return ERROR_INVALID_LENGTH;

#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   //Save resource name (for WebSocket connections only)
   osStrcpy(context->settings.uri, uri);
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set client identifier
 * @param[in] context Pointer to the MQTT client context
 * @param[in] clientId NULL-terminated string containing the client identifier
 * @return Error code
 **/

error_t mqttClientSetIdentifier(MqttClientContext *context,
   const char_t *clientId)
{
   //Check parameters
   if(context == NULL || clientId == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the length of the client identifier is acceptable
   if(osStrlen(clientId) > MQTT_CLIENT_MAX_ID_LEN)
      return ERROR_INVALID_LENGTH;

   //Save client identifier
   osStrcpy(context->settings.clientId, clientId);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set authentication information
 * @param[in] context Pointer to the MQTT client context
 * @param[in] username NULL-terminated string containing the user name to be used
 * @param[in] password NULL-terminated string containing the password to be used
 * @return Error code
 **/

error_t mqttClientSetAuthInfo(MqttClientContext *context,
   const char_t *username, const char_t *password)
{
   //Check parameters
   if(context == NULL || username == NULL || password == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the length of the user name is acceptable
   if(osStrlen(username) > MQTT_CLIENT_MAX_USERNAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Save user name
   osStrcpy(context->settings.username, username);

   //Make sure the length of the password is acceptable
   if(osStrlen(password) > MQTT_CLIENT_MAX_PASSWORD_LEN)
      return ERROR_INVALID_LENGTH;

   //Save password
   osStrcpy(context->settings.password, password);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Specify the Will message
 * @param[in] context Pointer to the MQTT client context
 * @param[in] topic Will topic name
 * @param[in] message Will message
 * @param[in] length Length of the Will message
 * @param[in] qos QoS level to be used when publishing the Will message
 * @param[in] retain This flag specifies if the Will message is to be retained
 * @return Error code
 **/

error_t mqttClientSetWillMessage(MqttClientContext *context, const char_t *topic,
   const void *message, size_t length, MqttQosLevel qos, bool_t retain)
{
   MqttClientWillMessage *willMessage;

   //Check parameters
   if(context == NULL || topic == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the length of the Will topic is acceptable
   if(osStrlen(topic) > MQTT_CLIENT_MAX_WILL_TOPIC_LEN)
      return ERROR_INVALID_LENGTH;

   //Point to the Will message
   willMessage = &context->settings.willMessage;

   //Save Will topic
   osStrcpy(willMessage->topic, topic);

   //Any message payload
   if(length > 0)
   {
      //Sanity check
      if(message == NULL)
         return ERROR_INVALID_PARAMETER;

      //Make sure the length of the Will message payload is acceptable
      if(osStrlen(message) > MQTT_CLIENT_MAX_WILL_PAYLOAD_LEN)
         return ERROR_INVALID_LENGTH;

      //Save Will message payload
      osMemcpy(willMessage->payload, message, length);
   }

   //Length of the Will message payload
   willMessage->length = length;
   //QoS level to be used when publishing the Will message
   willMessage->qos = qos;
   //This flag specifies if the Will message is to be retained
   willMessage->retain = retain;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Bind the MQTT client to a particular network interface
 * @param[in] context Pointer to the MQTT client context
 * @param[in] interface Network interface to be used
 * @return Error code
 **/

error_t mqttClientBindToInterface(MqttClientContext *context,
   NetInterface *interface)
{
   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Explicitly associate the MQTT client with the specified interface
   context->interface = interface;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Establish connection with the MQTT server
 * @param[in] context Pointer to the MQTT client context
 * @param[in] serverIpAddr IP address of the MQTT server to connect to
 * @param[in] serverPort TCP port number that will be used to establish the
 *   connection
 * @param[in] cleanSession If this flag is set, then the client and server
 *   must discard any previous session and start a new one
 * @return Error code
 **/

error_t mqttClientConnect(MqttClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort, bool_t cleanSession)
{
   error_t error;

   //Check parameters
   if(context == NULL || serverIpAddr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Establish network connection
   while(!error)
   {
      //Check current state
      if(context->state == MQTT_CLIENT_STATE_DISCONNECTED)
      {
         //Open network connection
         error = mqttClientOpenConnection(context);

         //Check status code
         if(!error)
         {
            //Debug message
            TRACE_INFO("MQTT: Connecting to server %s port %" PRIu16 "...\r\n",
               ipAddrToString(serverIpAddr, NULL), serverPort);

            //The network connection is open
            mqttClientChangeState(context, MQTT_CLIENT_STATE_CONNECTING);
            //Save current time
            context->startTime = osGetSystemTime();
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_CONNECTING)
      {
         //Establish network connection
         error = mqttClientEstablishConnection(context, serverIpAddr,
            serverPort);

         //Check status code
         if(!error)
         {
            //Debug message
            TRACE_INFO("MQTT: Connected to server\r\n");

            //The network connection is established
            mqttClientChangeState(context, MQTT_CLIENT_STATE_CONNECTED);
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_CONNECTED)
      {
         //Format CONNECT packet
         error = mqttClientFormatConnect(context, cleanSession);

         //Check status code
         if(!error)
         {
            //Debug message
            TRACE_INFO("MQTT: Sending CONNECT packet (%" PRIuSIZE " bytes)...\r\n", context->packetLen);
            TRACE_DEBUG_ARRAY("  ", context->packet, context->packetLen);

            //Save the type of the MQTT packet to be sent
            context->packetType = MQTT_PACKET_TYPE_CONNECT;
            //Point to the beginning of the packet
            context->packetPos = 0;

            //Send CONNECT packet
            mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {
         //Send more data
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {
         //Wait for CONNACK packet
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
      {
         //Receive more data
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_PACKET_RECEIVED)
      {
         //Reset packet type
         context->packetType = MQTT_PACKET_TYPE_INVALID;
         //A CONNACK packet has been received
         mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
      }
      else if(context->state == MQTT_CLIENT_STATE_IDLE)
      {
         //The MQTT client is connected
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = mqttClientCheckTimeout(context);
   }

   //Failed to establish connection with the MQTT server?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Clean up side effects
      mqttClientCloseConnection(context);
      //Update MQTT client state
      mqttClientChangeState(context, MQTT_CLIENT_STATE_DISCONNECTED);
   }

   //Return status code
   return error;
}


/**
 * @brief Publish message
 * @param[in] context Pointer to the MQTT client context
 * @param[in] topic Topic name
 * @param[in] message Message payload
 * @param[in] length Length of the message payload
 * @param[in] qos QoS level to be used when publishing the message
 * @param[in] retain This flag specifies if the message is to be retained
 * @param[out] packetId Packet identifier used to send the PUBLISH packet
 * @return Error code
 **/

error_t mqttClientPublish(MqttClientContext *context,
   const char_t *topic, const void *message, size_t length,
   MqttQosLevel qos, bool_t retain, uint16_t *packetId)
{
   error_t error;

   //Check parameters
   if(context == NULL || topic == NULL)
      return ERROR_INVALID_PARAMETER;
   if(message == NULL && length != 0)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Send PUBLISH packet and wait for PUBACK/PUBCOMP packet to be received
   while(!error)
   {
      //Check current state
      if(context->state == MQTT_CLIENT_STATE_IDLE)
      {
         //Check for transmission completion
         if(context->packetType == MQTT_PACKET_TYPE_INVALID)
         {
            //Format PUBLISH packet
            error = mqttClientFormatPublish(context, topic, message,
               length, qos, retain);

            //Check status code
            if(!error)
            {
               //Save the packet identifier used to send the PUBLISH packet
               if(packetId != NULL)
                  *packetId = context->packetId;

               //Debug message
               TRACE_INFO("MQTT: Sending PUBLISH packet (%" PRIuSIZE " bytes)...\r\n",
                  context->packetLen);

               //Dump the contents of the PUBLISH packet
               TRACE_DEBUG_ARRAY("  ", context->packet, context->packetLen);

               //Save the type of the MQTT packet to be sent
               context->packetType = MQTT_PACKET_TYPE_PUBLISH;
               //Point to the beginning of the packet
               context->packetPos = 0;

               //Send PUBLISH packet
               mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
               //Save the time at which the packet was sent
               context->startTime = osGetSystemTime();
            }
         }
         else
         {
            //Reset packet type
            context->packetType = MQTT_PACKET_TYPE_INVALID;
            //We are done
            break;
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {
         //Send more data
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {
         //The last parameter is optional
         if(packetId != NULL)
         {
            //Do not wait for PUBACK/PUBCOMP packet
            mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
         }
         else
         {
            //Check QoS level
            if(qos == MQTT_QOS_LEVEL_0)
            {
               //No response is sent by the receiver and no retry is performed by the sender
               mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
            }
            else
            {
               //Wait for PUBACK/PUBCOMP packet
               error = mqttClientProcessEvents(context, context->settings.timeout);
            }
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
      {
         //Receive more data
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_PACKET_RECEIVED)
      {
         //A PUBACK/PUBCOMP packet has been received
         mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = mqttClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Subscribe to topic
 * @param[in] context Pointer to the MQTT client context
 * @param[in] topic Topic filter
 * @param[in] qos Maximum QoS level at which the server can send application
 *   messages to the client
 * @param[out] packetId Packet identifier used to send the SUBSCRIBE packet
 * @return Error code
 **/

error_t mqttClientSubscribe(MqttClientContext *context,
   const char_t *topic, MqttQosLevel qos, uint16_t *packetId)
{
   error_t error;

   //Check parameters
   if(context == NULL || topic == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Send SUBSCRIBE packet and wait for SUBACK packet to be received
   while(!error)
   {
      //Check current state
      if(context->state == MQTT_CLIENT_STATE_IDLE)
      {
         //Check for transmission completion
         if(context->packetType == MQTT_PACKET_TYPE_INVALID)
         {
            //Format SUBSCRIBE packet
            error = mqttClientFormatSubscribe(context, topic, qos);

            //Check status code
            if(!error)
            {
               //Save the packet identifier used to send the SUBSCRIBE packet
               if(packetId != NULL)
                  *packetId = context->packetId;

               //Debug message
               TRACE_INFO("MQTT: Sending SUBSCRIBE packet (%" PRIuSIZE " bytes)...\r\n",
                  context->packetLen);

               //Dump the contents of the SUBSCRIBE packet
               TRACE_DEBUG_ARRAY("  ", context->packet, context->packetLen);

               //Save the type of the MQTT packet to be sent
               context->packetType = MQTT_PACKET_TYPE_SUBSCRIBE;
               //Point to the beginning of the packet
               context->packetPos = 0;

               //Send SUBSCRIBE packet
               mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
               //Save the time at which the packet was sent
               context->startTime = osGetSystemTime();
            }
         }
         else
         {
            //Reset packet type
            context->packetType = MQTT_PACKET_TYPE_INVALID;
            //We are done
            break;
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {
         //Send more data
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {
         //The last parameter is optional
         if(packetId != NULL)
         {
            //Do not wait for SUBACK packet
            mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
         }
         else
         {
            //Wait for SUBACK packet
            error = mqttClientProcessEvents(context, context->settings.timeout);
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
      {
         //Receive more data
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_PACKET_RECEIVED)
      {
         //A SUBACK packet has been received
         mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = mqttClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Unsubscribe from topic
 * @param[in] context Pointer to the MQTT client context
 * @param[in] topic Topic filter
 * @param[out] packetId Packet identifier used to send the UNSUBSCRIBE packet
 * @return Error code
 **/

error_t mqttClientUnsubscribe(MqttClientContext *context,
   const char_t *topic, uint16_t *packetId)
{
   error_t error;

   //Check parameters
   if(context == NULL || topic == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Send UNSUBSCRIBE packet and wait for UNSUBACK packet to be received
   while(!error)
   {
      //Check current state
      if(context->state == MQTT_CLIENT_STATE_IDLE)
      {
         //Check for transmission completion
         if(context->packetType == MQTT_PACKET_TYPE_INVALID)
         {
            //Format UNSUBSCRIBE packet
            error = mqttClientFormatUnsubscribe(context, topic);

            //Check status code
            if(!error)
            {
               //Save the packet identifier used to send the UNSUBSCRIBE packet
               if(packetId != NULL)
                  *packetId = context->packetId;

               //Debug message
               TRACE_INFO("MQTT: Sending UNSUBSCRIBE packet (%" PRIuSIZE " bytes)...\r\n",
                  context->packetLen);

               //Dump the contents of the UNSUBSCRIBE packet
               TRACE_DEBUG_ARRAY("  ", context->packet, context->packetLen);

               //Save the type of the MQTT packet to be sent
               context->packetType = MQTT_PACKET_TYPE_UNSUBSCRIBE;
               //Point to the beginning of the packet
               context->packetPos = 0;

               //Send UNSUBSCRIBE packet
               mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
               //Save the time at which the packet was sent
               context->startTime = osGetSystemTime();
            }
         }
         else
         {
            //Reset packet type
            context->packetType = MQTT_PACKET_TYPE_INVALID;
            //We are done
            break;
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {
         //Send more data
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {
         //The last parameter is optional
         if(packetId != NULL)
         {
            //Do not wait for UNSUBACK packet
            mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
         }
         else
         {
            //Wait for UNSUBACK packet
            error = mqttClientProcessEvents(context, context->settings.timeout);
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
      {
         //Receive more data
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_PACKET_RECEIVED)
      {
         //An UNSUBACK packet has been received
         mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = mqttClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Send ping request
 * @param[in] context Pointer to the MQTT client context
 * @param[out] rtt Round-trip time (optional parameter)
 * @return Error code
 **/

error_t mqttClientPing(MqttClientContext *context, systime_t *rtt)
{
   error_t error;

   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Send PINGREQ packet and wait for PINGRESP packet to be received
   while(!error)
   {
      //Check current state
      if(context->state == MQTT_CLIENT_STATE_IDLE)
      {
         //Check for transmission completion
         if(context->packetType == MQTT_PACKET_TYPE_INVALID)
         {
            //Format PINGREQ packet
            error = mqttClientFormatPingReq(context);

            //Check status code
            if(!error)
            {
               //Debug message
               TRACE_INFO("MQTT: Sending PINGREQ packet (%" PRIuSIZE " bytes)...\r\n",
                  context->packetLen);

               //Dump the contents of the PINGREQ packet
               TRACE_DEBUG_ARRAY("  ", context->packet, context->packetLen);

               //Save the type of the MQTT packet to be sent
               context->packetType = MQTT_PACKET_TYPE_PINGREQ;
               //Point to the beginning of the packet
               context->packetPos = 0;

               //Send PINGREQ packet
               mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
               //Save the time at which the packet was sent
               context->startTime = osGetSystemTime();
            }
         }
         else
         {
            //Reset packet type
            context->packetType = MQTT_PACKET_TYPE_INVALID;
            //We are done
            break;
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {
         //Send more data
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {
         //The last parameter is optional
         if(rtt != NULL)
         {
            //Wait for PINGRESP packet
            error = mqttClientProcessEvents(context, context->settings.timeout);
         }
         else
         {
            //Do not wait for PINGRESP packet
            mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
      {
         //Receive more data
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_PACKET_RECEIVED)
      {
         //The last parameter is optional
         if(rtt != NULL)
         {
            //Compute round-trip time
            *rtt = osGetSystemTime() - context->startTime;
         }

         //A PINGRESP packet has been received
         mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = mqttClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Process MQTT client events
 * @param[in] context Pointer to the MQTT client context
 * @param[in] timeout Maximum time to wait before returning
 * @return Error code
 **/

error_t mqttClientTask(MqttClientContext *context, systime_t timeout)
{
   error_t error;

   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Process MQTT client events
   error = mqttClientProcessEvents(context, timeout);

   //Check status code
   if(error == ERROR_TIMEOUT)
   {
      //Catch exception
      error = NO_ERROR;
   }

   //Return status code
   return error;
}


/**
 * @brief Gracefully disconnect from the MQTT server
 * @param[in] context Pointer to the MQTT client context
 * @return Error code
 **/

error_t mqttClientDisconnect(MqttClientContext *context)
{
   error_t error;

   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Send DISCONNECT packet and shutdown network connection
   while(!error)
   {
      //Check current state
      if(context->state == MQTT_CLIENT_STATE_IDLE)
      {
         //Format DISCONNECT packet
         error = mqttClientFormatDisconnect(context);

         //Check status code
         if(!error)
         {
            //Debug message
            TRACE_INFO("MQTT: Sending DISCONNECT packet (%" PRIuSIZE " bytes)...\r\n", context->packetLen);
            TRACE_DEBUG_ARRAY("  ", context->packet, context->packetLen);

            //Save the type of the MQTT packet to be sent
            context->packetType = MQTT_PACKET_TYPE_DISCONNECT;
            //Point to the beginning of the packet
            context->packetPos = 0;

            //Send DISCONNECT packet
            mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
            //Save the time at which the packet was sent
            context->startTime = osGetSystemTime();
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {
         //Send more data
         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if(context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {
         //Debug message
         TRACE_INFO("MQTT: Shutting down connection...\r\n");

         //After sending a DISCONNECT packet the client must not send any
         //more control packets on that network connection
         mqttClientChangeState(context, MQTT_CLIENT_STATE_DISCONNECTING);
      }
      else if(context->state == MQTT_CLIENT_STATE_DISCONNECTING)
      {
         //Properly dispose the network connection
         error = mqttClientShutdownConnection(context);

         //Check status code
         if(!error)
         {
            //The MQTT client is disconnected
            mqttClientChangeState(context, MQTT_CLIENT_STATE_DISCONNECTED);
         }
      }
      else if(context->state == MQTT_CLIENT_STATE_DISCONNECTED)
      {
         //The MQTT client is disconnected
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = mqttClientCheckTimeout(context);
   }

   //Failed to gracefully disconnect from the MQTT server?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Close connection
      mqttClientCloseConnection(context);
      //Update MQTT client state
      mqttClientChangeState(context, MQTT_CLIENT_STATE_DISCONNECTED);
   }

   //Return status code
   return error;
}


/**
 * @brief Close the connection with the MQTT server
 * @param[in] context Pointer to the MQTT client context
 * @return Error code
 **/

error_t mqttClientClose(MqttClientContext *context)
{
   //Make sure the MQTT client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Close connection
   mqttClientCloseConnection(context);
   //Update MQTT client state
   mqttClientChangeState(context, MQTT_CLIENT_STATE_DISCONNECTED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Release MQTT client context
 * @param[in] context Pointer to the MQTT client context
 **/

void mqttClientDeinit(MqttClientContext *context)
{
   //Make sure the MQTT client context is valid
   if(context != NULL)
   {
      //Close connection
      mqttClientCloseConnection(context);

#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
      //Release TLS session state
      tlsFreeSessionState(&context->tlsSession);
#endif

      //Clear MQTT client context
      osMemset(context, 0, sizeof(MqttClientContext));
   }
}

#endif
