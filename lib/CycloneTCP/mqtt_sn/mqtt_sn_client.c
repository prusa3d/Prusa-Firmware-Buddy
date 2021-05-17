/**
 * @file mqtt_sn_client.c
 * @brief MQTT-SN client
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
#include "mqtt_sn/mqtt_sn_client_message.h"
#include "mqtt_sn/mqtt_sn_client_transport.h"
#include "mqtt_sn/mqtt_sn_client_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MQTT_SN_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Initialize MQTT-SN client context
 * @param[in] context Pointer to the MQTT-SN client context
 * @return Error code
 **/

error_t mqttSnClientInit(MqttSnClientContext *context)
{
#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)
   error_t error;
#endif

   //Make sure the MQTT-SN client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Clear MQTT-SN client context
   osMemset(context, 0, sizeof(MqttSnClientContext));

#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)
   //Initialize DTLS session state
   error = tlsInitSessionState(&context->dtlsSession);
   //Any error to report?
   if(error)
      return error;
#endif

   //Initialize MQTT-SN client state
   context->state = MQTT_SN_CLIENT_STATE_DISCONNECTED;

   //Default transport protocol
   context->transportProtocol = MQTT_SN_TRANSPORT_PROTOCOL_UDP;
   //Default timeout
   context->timeout = MQTT_SN_CLIENT_DEFAULT_TIMEOUT;
   //Default keep-alive time interval
   context->keepAlive = MQTT_SN_CLIENT_DEFAULT_KEEP_ALIVE;

   //Initialize message identifier
   context->msgId = 0;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Set the transport protocol to be used
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] transportProtocol Transport protocol to be used (UDP or DTLS)
 * @return Error code
 **/

error_t mqttSnClientSetTransportProtocol(MqttSnClientContext *context,
   MqttSnTransportProtocol transportProtocol)
{
   //Make sure the MQTT-SN client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save the transport protocol to be used
   context->transportProtocol = transportProtocol;

   //Successful processing
   return NO_ERROR;
}


#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)

/**
 * @brief Register DTLS initialization callback function
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] callback DTLS initialization callback function
 * @return Error code
 **/

error_t mqttSnClientRegisterDtlsInitCallback(MqttSnClientContext *context,
   MqttSnClientDtlsInitCallback callback)
{
   //Check parameters
   if(context == NULL || callback == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save callback function
   context->dtlsInitCallback = callback;

   //Successful processing
   return NO_ERROR;
}

#endif


/**
 * @brief Register publish callback function
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] callback Callback function to be called when a PUBLISH message
 *   is received
 * @return Error code
 **/

error_t mqttSnClientRegisterPublishCallback(MqttSnClientContext *context,
   MqttSnClientPublishCallback callback)
{
   //Make sure the MQTT-SN client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save callback function
   context->publishCallback = callback;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set the list of predefined topics
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] predefinedTopics List of predefined topics
 * @param[in] size Number of predefined topics
 * @return Error code
 **/

error_t mqttSnClientSetPredefinedTopics(MqttSnClientContext *context,
   MqttSnPredefinedTopic *predefinedTopics, uint_t size)
{
   //Make sure the MQTT-SN client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check parameters
   if(predefinedTopics == NULL && size != 0)
      return ERROR_INVALID_PARAMETER;

   //Save the list of predefined topics
   context->predefinedTopicTable = predefinedTopics;
   context->predefinedTopicTableSize = size;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set communication timeout
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] timeout Timeout value, in milliseconds
 * @return Error code
 **/

error_t mqttSnClientSetTimeout(MqttSnClientContext *context, systime_t timeout)
{
   //Make sure the MQTT-SN client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save timeout value
   context->timeout = timeout;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set keep-alive value
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] keepAlive Keep-alive interval, in milliseconds
 * @return Error code
 **/

error_t mqttSnClientSetKeepAlive(MqttSnClientContext *context, systime_t keepAlive)
{
   //Make sure the MQTT-SN client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save keep-alive value
   context->keepAlive = keepAlive;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set client identifier
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] clientId NULL-terminated string containing the client identifier
 * @return Error code
 **/

error_t mqttSnClientSetIdentifier(MqttSnClientContext *context,
   const char_t *clientId)
{
   //Check parameters
   if(context == NULL || clientId == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the length of the client identifier is acceptable
   if(osStrlen(clientId) > MQTT_SN_CLIENT_MAX_ID_LEN)
      return ERROR_INVALID_LENGTH;

   //Save client identifier
   osStrcpy(context->clientId, clientId);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Specify the Will message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topic Will topic name
 * @param[in] message Will message
 * @param[in] length Length of the Will message
 * @param[in] qos QoS level to be used when publishing the Will message
 * @param[in] retain This flag specifies if the Will message is to be retained
 * @return Error code
 **/

error_t mqttSnClientSetWillMessage(MqttSnClientContext *context,
   const char_t *topic, const void *message, size_t length,
   MqttSnQosLevel qos, bool_t retain)
{
   //Check parameters
   if(context == NULL || topic == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the length of the Will topic is acceptable
   if(osStrlen(topic) > MQTT_SN_CLIENT_MAX_WILL_TOPIC_LEN)
      return ERROR_INVALID_LENGTH;

   //Save Will topic
   osStrcpy(context->willMessage.topic, topic);

   //Any message payload
   if(length > 0)
   {
      //Sanity check
      if(message == NULL)
         return ERROR_INVALID_PARAMETER;

      //Make sure the length of the Will message payload is acceptable
      if(osStrlen(message) > MQTT_SN_CLIENT_MAX_WILL_PAYLOAD_LEN)
         return ERROR_INVALID_LENGTH;

      //Save Will message payload
      osMemcpy(context->willMessage.payload, message, length);
   }

   //Length of the Will message payload
   context->willMessage.length = length;

   //QoS level to be used when publishing the Will message
   context->willMessage.flags.qos = qos;
   //This flag specifies if the Will message is to be retained
   context->willMessage.flags.retain = retain;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Bind the MQTT-SN client to a particular network interface
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] interface Network interface to be used
 * @return Error code
 **/

error_t mqttSnClientBindToInterface(MqttSnClientContext *context,
   NetInterface *interface)
{
   //Make sure the MQTT-SN client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Explicitly associate the MQTT client with the specified interface
   context->interface = interface;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Specify the address of the gateway
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] gwIpAddr Gateway IP address
 * @param[in] gwPort Gateway port number
 * @return Error code
 **/

error_t mqttSnClientSetGateway(MqttSnClientContext *context,
   const IpAddr *gwIpAddr, uint16_t gwPort)
{
   //Check parameters
   if(context == NULL || gwIpAddr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save the IP address and the port number of the MQTT-SN gateway
   context->gwIpAddr = *gwIpAddr;
   context->gwPort = gwPort;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Search for a gateway
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] destIpAddr Destination IP address
 * @param[in] destPort Destination port number
 * @return Error code
 **/

error_t mqttSnClientSearchGateway(MqttSnClientContext *context,
   const IpAddr *destIpAddr, uint16_t destPort)
{
   error_t error;
   systime_t time;

   //Check parameters
   if(context == NULL || destIpAddr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Gateway discovery procedure
   while(!error)
   {
      //Get current time
      time = osGetSystemTime();

      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_DISCONNECTED)
      {
         //Open network connection
         error = mqttSnClientOpenConnection(context, FALSE);

         //Check status code
         if(!error)
         {
            //Save current time
            context->startTime = time;
            context->retransmitStartTime = time;

            //To prevent broadcast storms when multiple clients start searching
            //for GW almost at the same time, the sending of the SEARCHGW message
            //is delayed by a random time between 0 and TSEARCHGW
            context->retransmitTimeout = netGetRandRange(0,
               MQTT_SN_CLIENT_SEARCH_DELAY);

            //Start searching for gateways
            context->state = MQTT_SN_CLIENT_STATE_SEARCHING;
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_SEARCHING)
      {
         //Check whether the timeout has elapsed
         if(timeCompare(time, context->startTime + context->timeout) >= 0)
         {
            //Abort the retransmission procedure
            error = ERROR_TIMEOUT;
         }
         else if(timeCompare(time, context->retransmitStartTime +
            context->retransmitTimeout) >= 0)
         {
            //Set retransmission timeout
            context->retransmitTimeout = MQTT_SN_CLIENT_RETRY_TIMEOUT;

            //If the retry timer times out and the expected gateway's reply
            //is not received, the client retransmits the message
            error = mqttSnClientSendSearchGw(context, 0, destIpAddr, destPort);
         }
         else
         {
            //Wait for the gateway's reply
            error = mqttSnClientProcessEvents(context, MQTT_SN_CLIENT_TICK_INTERVAL);
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_RESP_RECEIVED)
      {
         //Check the type of the received message
         if(context->msgType == MQTT_SN_MSG_TYPE_GWINFO)
         {
            //Close network connection
            mqttSnClientCloseConnection(context);

            //A MQTT-SN gateway has been found
            context->state = MQTT_SN_CLIENT_STATE_DISCONNECTED;
            break;
         }
         else
         {
            //Report an error
            error = ERROR_UNEXPECTED_RESPONSE;
         }
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }
   }

   //Any error to report?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Clean up side effects
      mqttSnClientCloseConnection(context);
      //Update MQTT-SN client state
      context->state = MQTT_SN_CLIENT_STATE_DISCONNECTED;
   }

   //Return status code
   return error;
}


/**
 * @brief Establish connection with the MQTT-SN gateway
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] cleanSession If this flag is set, then the client and server
 *   must discard any previous session and start a new one
 * @return Error code
 **/

error_t mqttSnClientConnect(MqttSnClientContext *context, bool_t cleanSession)
{
   error_t error;
   systime_t time;

   //Make sure the MQTT-SN client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Establish connection with the MQTT-SN gateway
   while(!error)
   {
      //Get current time
      time = osGetSystemTime();

      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_DISCONNECTED)
      {
         //Open network connection
         error = mqttSnClientOpenConnection(context, TRUE);

         //Check status code
         if(!error)
         {
            //Save current time
            context->startTime = time;
            //Update MQTT-SN client state
            context->state = MQTT_SN_CLIENT_STATE_CONNECTING;
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_CONNECTING)
      {
         //Establish DTLS connection
         error = mqttSnClientEstablishConnection(context);

         //Check status code
         if(error == NO_ERROR)
         {
            //Check whether the CleanSession flag is set
            if(cleanSession)
            {
               //Discard previous session state
               osMemset(context->topicTable, 0, sizeof(context->topicTable));
               osMemset(context->msgIdTable, 0, sizeof(context->msgIdTable));
            }

            //The CONNECT message is sent by a client to setup a connection
            error = mqttSnClientSendConnect(context, cleanSession);
         }
         else if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
         {
            //Check whether the timeout has elapsed
            if(timeCompare(time, context->startTime + context->timeout) >= 0)
            {
               //Report an error
               error = ERROR_TIMEOUT;
            }
         }
         else
         {
            //Failed to establish DTLS connection
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ)
      {
         //Check whether the timeout has elapsed
         if(timeCompare(time, context->startTime + context->timeout) >= 0)
         {
            //Abort the retransmission procedure
            error = ERROR_TIMEOUT;
         }
         else if(timeCompare(time, context->retransmitStartTime +
            MQTT_SN_CLIENT_RETRY_TIMEOUT) >= 0)
         {
            //If the retry timer times out and the expected gateway's reply
            //is not received, the client retransmits the message
            error = mqttSnClientSendConnect(context, cleanSession);
         }
         else
         {
            //Wait for the gateway's reply
            error = mqttSnClientProcessEvents(context, MQTT_SN_CLIENT_TICK_INTERVAL);
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_RESP_RECEIVED)
      {
         //Check the type of the received message
         if(context->msgType == MQTT_SN_MSG_TYPE_CONNACK)
         {
            //If the connection request has not been accepted, the failure reason
            //is encoded in the return code field of the CONNACK message
            if(context->returnCode == MQTT_SN_RETURN_CODE_ACCEPTED)
            {
               //The connection request has been accepted by the gateway
               context->state = MQTT_SN_CLIENT_STATE_ACTIVE;
            }
            else
            {
               //Terminate DTLS connection
               mqttSnClientShutdownConnection(context);

               //The connection request has been rejected by the gateway
               error = ERROR_REQUEST_REJECTED;
            }
         }
         else
         {
            //Report an error
            error = ERROR_UNEXPECTED_RESPONSE;
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_ACTIVE)
      {
         //The MQTT-SN client is connected
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }
   }

   //Any error to report?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Clean up side effects
      mqttSnClientCloseConnection(context);
      //Update MQTT-SN client state
      context->state = MQTT_SN_CLIENT_STATE_DISCONNECTED;
   }

   //Return status code
   return error;
}


/**
 * @brief Publish message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicName Topic name
 * @param[in] message Message payload
 * @param[in] length Length of the message payload
 * @param[in] qos QoS level to be used when publishing the message
 * @param[in] retain This flag specifies if the message is to be retained
 * @param[in] dup This flag specifies if the message is sent for the first
 *   time or if the message is retransmitted
 * @param[in,out] msgId Message identifier used to send the PUBLISH message
 * @return Error code
 **/

error_t mqttSnClientPublish(MqttSnClientContext *context,
   const char_t *topicName, const void *message, size_t length,
   MqttSnQosLevel qos, bool_t retain, bool_t dup, uint16_t *msgId)
{
   error_t error;
   systime_t time;
   uint16_t publishMsgId;

   //Check parameters
   if(context == NULL || topicName == NULL)
      return ERROR_INVALID_PARAMETER;
   if(message == NULL && length != 0)
      return ERROR_INVALID_PARAMETER;
   if(dup && msgId == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Initialize message identifier
   if(dup)
      publishMsgId = *msgId;
   else
      publishMsgId = 0;

   //Publish procedure
   while(!error)
   {
      //Get current time
      time = osGetSystemTime();

      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_ACTIVE)
      {
         //Save current time
         context->startTime = time;

         //Check whether the register procedure is needed
         if(mqttSnClientIsShortTopicName(topicName) == FALSE &&
            mqttSnClientFindTopicName(context, topicName) == 0 &&
            mqttSnClientFindPredefTopicName(context, topicName) == 0)
         {
            //The message identifier allows the sender to match a message with
            //its corresponding acknowledgment
            mqttSnClientGenerateMessageId(context);

            //To register a topic name a client sends a REGISTER message to
            //the gateway
            error = mqttSnClientSendRegister(context, topicName);
         }
         else
         {
            //The message ID is only relevant in case of QoS levels 1 and 2
            if(qos == MQTT_SN_QOS_LEVEL_1 || qos == MQTT_SN_QOS_LEVEL_2)
            {
               //The message identifier allows the sender to match a message with
               //its corresponding acknowledgment
               if(!dup)
                  publishMsgId = mqttSnClientGenerateMessageId(context);
            }
            else
            {
               //For QoS level 0, the message identifier is coded 0x0000
               publishMsgId = 0;
            }

            //The client can start publishing data relating to the registered
            //topic name by sending PUBLISH messages to the gateway
            error = mqttSnClientSendPublish(context, publishMsgId, topicName,
               message, length, qos, retain, dup);

            //In the QoS 0, no response is sent by the receiver and no retry
            //is performed by the sender
            if(qos != MQTT_SN_QOS_LEVEL_1 && qos != MQTT_SN_QOS_LEVEL_2)
               break;
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ)
      {
         //Check whether the transmission of the PUBLISH message has started
         if(context->msgType == MQTT_SN_MSG_TYPE_PUBLISH ||
            context->msgType == MQTT_SN_MSG_TYPE_PUBREL)
         {
            //Restore the message identifier that was used to send the first
            //PUBLISH message
            if(!dup)
               publishMsgId = context->msgId;
         }

         //Check whether the timeout has elapsed
         if(timeCompare(time, context->startTime + context->timeout) >= 0)
         {
            //Abort the retransmission procedure
            context->state = MQTT_SN_CLIENT_STATE_DISCONNECTING;
            //Report a timeout error
            error = ERROR_TIMEOUT;
         }
         else if(timeCompare(time, context->retransmitStartTime +
            MQTT_SN_CLIENT_RETRY_TIMEOUT) >= 0)
         {
            //If the retry timer times out and the expected gateway's reply
            //is not received, the client retransmits the message
            if(context->msgType == MQTT_SN_MSG_TYPE_REGISTER)
            {
               //Retransmit REGISTER message
               error = mqttSnClientSendRegister(context, topicName);
            }
            else if(context->msgType == MQTT_SN_MSG_TYPE_PUBLISH)
            {
               //Retransmit PUBLISH message
               error = mqttSnClientSendPublish(context, publishMsgId,
                  topicName, message, length, qos, retain, TRUE);
            }
            else if(context->msgType == MQTT_SN_MSG_TYPE_PUBREL)
            {
               //Retransmit PUBREL message
               error = mqttSnClientSendPubRel(context, context->msgId);
            }
            else
            {
               //Report an error
               error = ERROR_INVALID_TYPE;
            }
         }
         else
         {
            //Wait for the gateway's reply
            error = mqttSnClientProcessEvents(context, MQTT_SN_CLIENT_TICK_INTERVAL);
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_RESP_RECEIVED)
      {
         //Update MQTT-SN client state
         context->state = MQTT_SN_CLIENT_STATE_ACTIVE;

         //Check whether the transmission of the PUBLISH message has started
         if(context->msgType == MQTT_SN_MSG_TYPE_PUBACK ||
            context->msgType == MQTT_SN_MSG_TYPE_PUBREC ||
            context->msgType == MQTT_SN_MSG_TYPE_PUBCOMP)
         {
            //Restore the message identifier that was used to send the first
            //PUBLISH message
            if(!dup)
               publishMsgId = context->msgId;
         }

         //Check the type of the received message
         if(context->msgType == MQTT_SN_MSG_TYPE_REGACK)
         {
            //If the registration has not been accepted, the failure reason is
            //encoded in the return code field of the REGACK message
            if(context->returnCode == MQTT_SN_RETURN_CODE_ACCEPTED)
            {
               //Save the topic ID assigned by the gateway
               error = mqttSnClientAddTopic(context, topicName, context->topicId);
            }
            else
            {
               //The registration request has been rejected by the gateway
               error = ERROR_REQUEST_REJECTED;
            }
         }
         else if(context->msgType == MQTT_SN_MSG_TYPE_PUBACK)
         {
            //If the publish request has not been accepted, the failure reason
            //is encoded in the return code field of the PUBACK message
            if(context->returnCode == MQTT_SN_RETURN_CODE_ACCEPTED)
            {
               //Check QoS level
               if(qos == MQTT_SN_QOS_LEVEL_2)
               {
                  //Unexpected PUBREC message received
                  error = ERROR_UNEXPECTED_MESSAGE;
               }
               else
               {
                  //A PUBACK message has been received
                  break;
               }
            }
            else
            {
               //The publish request has been rejected by the gateway
               error = ERROR_REQUEST_REJECTED;
            }
         }
         else if(context->msgType == MQTT_SN_MSG_TYPE_PUBREC)
         {
            //Check QoS level
            if(qos == MQTT_SN_QOS_LEVEL_2)
            {
               //A PUBREL packet is the response to a PUBREC packet. It is the
               //third packet of the QoS 2 protocol exchange
               error = mqttSnClientSendPubRel(context, context->msgId);
            }
            else
            {
               //Unexpected PUBREC message received
               error = ERROR_UNEXPECTED_MESSAGE;
            }
         }
         else if(context->msgType == MQTT_SN_MSG_TYPE_PUBCOMP)
         {
            //A PUBCOMP message has been received
            break;
         }
         else
         {
            //Report an error
            error = ERROR_UNEXPECTED_RESPONSE;
         }
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return the message identifier that was used to send the PUBLISH message
   if(msgId != NULL)
      *msgId = publishMsgId;

   //Return status code
   return error;
}


/**
 * @brief Subscribe to topic
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicName Topic filter
 * @param[in] qos Maximum QoS level at which the server can send application
 *   messages to the client
 * @return Error code
 **/

error_t mqttSnClientSubscribe(MqttSnClientContext *context,
   const char_t *topicName, MqttSnQosLevel qos)
{
   error_t error;
   systime_t time;

   //Check parameters
   if(context == NULL || topicName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Topic subscribe procedure
   while(!error)
   {
      //Get current time
      time = osGetSystemTime();

      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_ACTIVE)
      {
         //The message identifier allows the sender to match a message with
         //its corresponding acknowledgment
         mqttSnClientGenerateMessageId(context);

         //Save current time
         context->startTime = time;

         //Send SUBSCRIBE message
         error = mqttSnClientSendSubscribe(context, topicName, qos);
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ)
      {
         //Check whether the timeout has elapsed
         if(timeCompare(time, context->startTime + context->timeout) >= 0)
         {
            //Abort the retransmission procedure
            context->state = MQTT_SN_CLIENT_STATE_DISCONNECTING;
            //Report a timeout error
            error = ERROR_TIMEOUT;
         }
         else if(timeCompare(time, context->retransmitStartTime +
            MQTT_SN_CLIENT_RETRY_TIMEOUT) >= 0)
         {
            //If the retry timer times out and the expected gateway's reply
            //is not received, the client retransmits the message
            error = mqttSnClientSendSubscribe(context, topicName, qos);
         }
         else
         {
            //Wait for the gateway's reply
            error = mqttSnClientProcessEvents(context, MQTT_SN_CLIENT_TICK_INTERVAL);
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_RESP_RECEIVED)
      {
         //Update MQTT-SN client state
         context->state = MQTT_SN_CLIENT_STATE_ACTIVE;

         //Check the type of the received message
         if(context->msgType == MQTT_SN_MSG_TYPE_SUBACK)
         {
            //If the subscribe request has not been accepted, the failure reason
            //is encoded in the return code field of the SUBACK message
            if(context->returnCode == MQTT_SN_RETURN_CODE_ACCEPTED)
            {
               //The topic ID field is not relevant in case of subscriptions to a
               //topic name which contains wildcard characters
               if(osStrchr(topicName, '#') == NULL && osStrchr(topicName, '+') == NULL)
               {
                  //Save the topic ID assigned by the gateway
                  error = mqttSnClientAddTopic(context, topicName, context->topicId);
               }

               //A SUBACK message has been received
               break;
            }
            else
            {
               //The subscribe request has been rejected by the gateway
               error = ERROR_REQUEST_REJECTED;
            }
         }
         else
         {
            //Report an error
            error = ERROR_UNEXPECTED_RESPONSE;
         }
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Unsubscribe from topic
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicName Topic filter
 * @return Error code
 **/

error_t mqttSnClientUnsubscribe(MqttSnClientContext *context,
   const char_t *topicName)
{
   error_t error;
   systime_t time;

   //Check parameters
   if(context == NULL || topicName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Topic unsubscribe procedure
   while(!error)
   {
      //Get current time
      time = osGetSystemTime();

      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_ACTIVE)
      {
         //The message identifier allows the sender to match a message with
         //its corresponding acknowledgment
         mqttSnClientGenerateMessageId(context);

         //Save current time
         context->startTime = time;

         //Send UNSUBSCRIBE message
         error = mqttSnClientSendUnsubscribe(context, topicName);
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ)
      {
         //Check whether the timeout has elapsed
         if(timeCompare(time, context->startTime + context->timeout) >= 0)
         {
            //Abort the retransmission procedure
            context->state = MQTT_SN_CLIENT_STATE_DISCONNECTING;
            //Report a timeout error
            error = ERROR_TIMEOUT;
         }
         else if(timeCompare(time, context->retransmitStartTime +
            MQTT_SN_CLIENT_RETRY_TIMEOUT) >= 0)
         {
            //If the retry timer times out and the expected gateway's reply
            //is not received, the client retransmits the message
            error = mqttSnClientSendUnsubscribe(context, topicName);
         }
         else
         {
            //Wait for the gateway's reply
            error = mqttSnClientProcessEvents(context, MQTT_SN_CLIENT_TICK_INTERVAL);
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_RESP_RECEIVED)
      {
         //Update MQTT-SN client state
         context->state = MQTT_SN_CLIENT_STATE_ACTIVE;

         //Check the type of the received message
         if(context->msgType == MQTT_SN_MSG_TYPE_UNSUBACK)
         {
            //An UNSUBACK message has been received
            break;
         }
         else
         {
            //Report an error
            error = ERROR_UNEXPECTED_RESPONSE;
         }
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Send ping request
 * @param[in] context Pointer to the MQTT-SN client context
 * @return Error code
 **/

error_t mqttSnClientPing(MqttSnClientContext *context)
{
   error_t error;
   systime_t time;

   //Make sure the MQTT-SN client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Send PINGREQ packet and wait for PINGRESP packet to be received
   while(!error)
   {
      //Get current time
      time = osGetSystemTime();

      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_ACTIVE)
      {
         //Save current time
         context->startTime = time;
         context->retransmitStartTime = time;

         //Send PINGREQ message
         error = mqttSnClientSendPingReq(context);

         //Update MQTT-SN client state
         context->state = MQTT_SN_CLIENT_STATE_SENDING_REQ;
         context->msgType = MQTT_SN_MSG_TYPE_PINGREQ;
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ)
      {
         //Check whether the timeout has elapsed
         if(timeCompare(time, context->startTime + context->timeout) >= 0)
         {
            //Abort the retransmission procedure
            context->state = MQTT_SN_CLIENT_STATE_DISCONNECTING;
            //Report a timeout error
            error = ERROR_TIMEOUT;
         }
         else if(timeCompare(time, context->retransmitStartTime +
            MQTT_SN_CLIENT_RETRY_TIMEOUT) >= 0)
         {
            //If the retry timer times out and the expected gateway's reply
            //is not received, the client retransmits the message
            error = mqttSnClientSendPingReq(context);

            //Save the time at which the message was sent
            context->retransmitStartTime = time;
         }
         else
         {
            //Wait for the gateway's reply
            error = mqttSnClientProcessEvents(context, MQTT_SN_CLIENT_TICK_INTERVAL);
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_RESP_RECEIVED)
      {
         //Update MQTT-SN client state
         context->state = MQTT_SN_CLIENT_STATE_ACTIVE;

         //Check the type of the received message
         if(context->msgType == MQTT_SN_MSG_TYPE_PINGRESP)
         {
            //A PINGRESP message has been received
            break;
         }
         else
         {
            //Report an error
            error = ERROR_UNEXPECTED_RESPONSE;
         }
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Update the Will message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topic Will topic name
 * @param[in] message Will message
 * @param[in] length Length of the Will message
 * @param[in] qos QoS level to be used when publishing the Will message
 * @param[in] retain This flag specifies if the Will message is to be retained
 * @return Error code
 **/

error_t mqttSnClientUpdateWillMessage(MqttSnClientContext *context,
   const char_t *topic, const void *message, size_t length,
   MqttSnQosLevel qos, bool_t retain)
{
   error_t error;
   systime_t time;

   //Check parameters
   if(context == NULL || topic == NULL)
      return ERROR_INVALID_PARAMETER;
   if(message == NULL && length != 0)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Publish procedure
   while(!error)
   {
      //Get current time
      time = osGetSystemTime();

      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_ACTIVE)
      {
         //Update the Will message
         error = mqttSnClientSetWillMessage(context, topic, message, length,
            qos, retain);

         //Check status code
         if(!error)
         {
            //Save current time
            context->startTime = time;

            //Send WILLTOPICUPD message
            error = mqttSnClientSendWillTopicUpd(context);
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ)
      {
         //Check whether the timeout has elapsed
         if(timeCompare(time, context->startTime + context->timeout) >= 0)
         {
            //Abort the retransmission procedure
            context->state = MQTT_SN_CLIENT_STATE_DISCONNECTING;
            //Report a timeout error
            error = ERROR_TIMEOUT;
         }
         else if(timeCompare(time, context->retransmitStartTime +
            MQTT_SN_CLIENT_RETRY_TIMEOUT) >= 0)
         {
            //If the retry timer times out and the expected gateway's reply
            //is not received, the client retransmits the message
            if(context->msgType == MQTT_SN_MSG_TYPE_WILLTOPICUPD)
            {
               //Retransmit WILLTOPICUPD message
               error = mqttSnClientSendWillTopicUpd(context);
            }
            else if(context->msgType == MQTT_SN_MSG_TYPE_WILLMSGUPD)
            {
               //Retransmit WILLMSGUPD message
               error = mqttSnClientSendWillMsgUpd(context);
            }
            else
            {
               //Report an error
               error = ERROR_INVALID_TYPE;
            }
         }
         else
         {
            //Wait for the gateway's reply
            error = mqttSnClientProcessEvents(context, MQTT_SN_CLIENT_TICK_INTERVAL);
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_RESP_RECEIVED)
      {
         //Update MQTT-SN client state
         context->state = MQTT_SN_CLIENT_STATE_ACTIVE;

         //Check the type of the received message
         if(context->msgType == MQTT_SN_MSG_TYPE_WILLTOPICRESP)
         {
            //If the WILLTOPICUPD has not been accepted, the failure reason
            //is encoded in the return code field of the WILLTOPICRESP
            if(context->returnCode == MQTT_SN_RETURN_CODE_ACCEPTED)
            {
               //Valid Will topic?
               if(context->willMessage.topic[0] != '\0')
               {
                  //Send WILLMSGUPD message
                  error = mqttSnClientSendWillMsgUpd(context);
               }
               else
               {
                  //An empty WILLTOPIC message is used by a client to delete
                  //the Will topic and the Will message stored in the server
                  break;
               }
            }
            else
            {
               //The WILLTOPICUPD request has been rejected by the gateway
               error = ERROR_REQUEST_REJECTED;
            }
         }
         else if(context->msgType == MQTT_SN_MSG_TYPE_WILLMSGRESP)
         {
            //If the WILLMSGUPD has not been accepted, the failure reason
            //is encoded in the return code field of the WILLMSGRESP
            if(context->returnCode == MQTT_SN_RETURN_CODE_ACCEPTED)
            {
               //The WILLMSGUPD request has been accepted by the gateway
               break;
            }
            else
            {
               //The WILLMSGUPD request has been rejected by the gateway
               error = ERROR_REQUEST_REJECTED;
            }
         }
         else
         {
            //Report an error
            error = ERROR_UNEXPECTED_RESPONSE;
         }
      }
      else
      {
         //Invalid state
         error = ERROR_NOT_CONNECTED;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Retrieve return code
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[out] returnCode Return code
 * @return Error code
 **/

error_t mqttSnClientGetReturnCode(MqttSnClientContext *context,
   MqttSnReturnCode *returnCode)
{
   //Check parameters
   if(context == NULL || returnCode == NULL)
      return ERROR_INVALID_PARAMETER;

   //Retrieve return code
   *returnCode = context->returnCode;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process MQTT-SN client events
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] timeout Maximum time to wait before returning
 * @return Error code
 **/

error_t mqttSnClientTask(MqttSnClientContext *context, systime_t timeout)
{
   error_t error;

   //Make sure the MQTT-SN client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the MQTT-SN client is connected
   if(context->state == MQTT_SN_CLIENT_STATE_ACTIVE ||
      context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ ||
      context->state == MQTT_SN_CLIENT_STATE_RESP_RECEIVED)
   {
      //Process MQTT-SN client events
      error = mqttSnClientProcessEvents(context, timeout);
   }
   else
   {
      //Invalid state
      error = ERROR_NOT_CONNECTED;
   }

   //Return status code
   return error;
}


/**
 * @brief Disconnect from the MQTT-SN gateway
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] duration Sleep duration, in milliseconds
 * @return Error code
 **/

error_t mqttSnClientDisconnect(MqttSnClientContext *context,
   systime_t duration)
{
   error_t error;
   systime_t time;

   //Make sure the MQTT-SN client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Disconnect procedure
   while(!error)
   {
      //Get current time
      time = osGetSystemTime();

      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_ACTIVE)
      {
         //Save current time
         context->startTime = time;

         //The DISCONNECT message is sent by a client to indicate that it
         //wants to close the connection
         error = mqttSnClientSendDisconnect(context, duration / 1000);
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ)
      {
         //Check whether the timeout has elapsed
         if(timeCompare(time, context->startTime + context->timeout) >= 0)
         {
            //Terminate DTLS connection
            mqttSnClientShutdownConnection(context);

            //Report a timeout error
            error = ERROR_TIMEOUT;
         }
         else if(timeCompare(time, context->retransmitStartTime +
            MQTT_SN_CLIENT_RETRY_TIMEOUT) >= 0)
         {
            //If the retry timer times out and the expected gateway's reply
            //is not received, the client retransmits the message
            error = mqttSnClientSendDisconnect(context, duration / 1000);
         }
         else
         {
            //Wait for the gateway's reply
            error = mqttSnClientProcessEvents(context, MQTT_SN_CLIENT_TICK_INTERVAL);
         }
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_DISCONNECTING)
      {
         //Terminate DTLS connection
         error = mqttSnClientShutdownConnection(context);
         //Close network connection
         mqttSnClientCloseConnection(context);

         //The connection is closed
         context->state = MQTT_SN_CLIENT_STATE_DISCONNECTED;
      }
      else if(context->state == MQTT_SN_CLIENT_STATE_DISCONNECTED)
      {
         //The MQTT-SN client is disconnected
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }
   }

   //Any error to report?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Close network connection
      mqttSnClientCloseConnection(context);
      //Update MQTT-SN client state
      context->state = MQTT_SN_CLIENT_STATE_DISCONNECTED;
   }

   //Return status code
   return error;
}


/**
 * @brief Release MQTT-SN client context
 * @param[in] context Pointer to the MQTT-SN client context
 **/

void mqttSnClientDeinit(MqttSnClientContext *context)
{
   //Make sure the MQTT-SN client context is valid
   if(context != NULL)
   {
      //Close connection
      mqttSnClientCloseConnection(context);

#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)
      //Release DTLS session state
      tlsFreeSessionState(&context->dtlsSession);
#endif

      //Clear MQTT-SN client context
      osMemset(context, 0, sizeof(MqttSnClientContext));
   }
}

#endif
