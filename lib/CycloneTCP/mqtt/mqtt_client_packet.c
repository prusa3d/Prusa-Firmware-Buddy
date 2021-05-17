/**
 * @file mqtt_client_packet.c
 * @brief MQTT packet parsing and formatting
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

//MQTT control packets
static const char_t *packetLabel[16] =
{
   "Reserved",    //0
   "CONNECT",     //1
   "CONNACK",     //2
   "PUBLISH",     //3
   "PUBACK",      //4
   "PUBREC",      //5
   "PUBREL",      //6
   "PUBCOMP",     //7
   "SUBSCRIBE",   //8
   "SUBACK",      //9
   "UNSUBSCRIBE", //10
   "UNSUBACK",    //11
   "PINGREQ",     //12
   "PINGRESP",    //13
   "DISCONNECT",  //14
   "Reserved"     //15
};


/**
 * @brief Receive MQTT packet
 * @param[in] context Pointer to the MQTT client context
 * @return Error code
 **/

error_t mqttClientReceivePacket(MqttClientContext *context)
{
   error_t error;
   size_t n;
   uint8_t value;

   //Initialize status code
   error = NO_ERROR;

   //Receive incoming packet
   while(1)
   {
      //Packet header is being received?
      if(context->packetLen == 0)
      {
         //Read a single byte
         error = mqttClientReceiveData(context, &value, sizeof(uint8_t), &n, 0);

         //Any data received?
         if(!error)
         {
            //Save the current byte
            context->packet[context->packetPos] = value;

            //The Remaining Length is encoded using a variable length encoding scheme
            if(context->packetPos > 0)
            {
               //The most significant bit is used to indicate that there are
               //following bytes in the representation
               if(value & 0x80)
               {
                  //Applications can send control packets of size up to 256 MB
                  if(context->packetPos < 4)
                  {
                     //The least significant seven bits of each byte encode the data
                     context->remainingLen |= (value & 0x7F) << (7 * (context->packetPos - 1));
                  }
                  else
                  {
                     //Report an error
                     error = ERROR_INVALID_SYNTAX;
                  }
               }
               else
               {
                  //The least significant seven bits of each byte encode the data
                  context->remainingLen |= value << (7 * (context->packetPos - 1));
                  //Calculate the length of the control packet
                  context->packetLen = context->packetPos + 1 + context->remainingLen;

                  //Sanity check
                  if(context->packetLen > MQTT_CLIENT_BUFFER_SIZE)
                     error = ERROR_INVALID_LENGTH;
               }
            }

            //Advance data pointer
            context->packetPos++;
         }
      }
      //Variable header or payload is being received?
      else
      {
         //Any remaining data?
         if(context->packetPos < context->packetLen)
         {
            //Read more data
            error = mqttClientReceiveData(context, context->packet + context->packetPos,
               context->packetLen - context->packetPos, &n, 0);

            //Advance data pointer
            context->packetPos += n;
         }
         else
         {
            //The packet has been successfully received
            break;
         }
      }

      //Any error to report?
      if(error)
         break;
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming MQTT packet
 * @param[in] context Pointer to the MQTT client context
 * @return Error code
 **/

error_t mqttClientProcessPacket(MqttClientContext *context)
{
   error_t error;
   bool_t dup;
   bool_t retain;
   size_t remainingLen;
   MqttQosLevel qos;
   MqttPacketType type;

   //Point to the first byte of the packet
   context->packetPos = 0;

   //Read the fixed header from the input buffer
   error = mqttDeserializeHeader(context->packet, context->packetLen,
      &context->packetPos, &type, &dup, &qos, &retain, &remainingLen);

   //Failed to deserialize fixed header?
   if(error)
      return error;

   //Debug message
   TRACE_INFO("MQTT: %s packet received (%" PRIuSIZE " bytes)...\r\n",
      packetLabel[type], context->packetLen);

   //Dump the contents of the packet
   TRACE_DEBUG_ARRAY("  ", context->packet, context->packetLen);

   //Check MQTT control packet type
   switch(type)
   {
   //CONNACK packet received?
   case MQTT_PACKET_TYPE_CONNACK:
      //Process incoming CONNACK packet
      error = mqttClientProcessConnAck(context, dup, qos, retain, remainingLen);
      break;
   //PUBLISH packet received?
   case MQTT_PACKET_TYPE_PUBLISH:
      //Process incoming PUBLISH packet
      error = mqttClientProcessPublish(context, dup, qos, retain, remainingLen);
      break;
   //PUBACK packet received?
   case MQTT_PACKET_TYPE_PUBACK:
      //Process incoming PUBACK packet
      error = mqttClientProcessPubAck(context, dup, qos, retain, remainingLen);
      break;
   //PUBREC packet received?
   case MQTT_PACKET_TYPE_PUBREC:
      //Process incoming PUBREC packet
      error = mqttClientProcessPubRec(context, dup, qos, retain, remainingLen);
      break;
   //PUBREL packet received?
   case MQTT_PACKET_TYPE_PUBREL:
      //Process incoming PUBREL packet
      error = mqttClientProcessPubRel(context, dup, qos, retain, remainingLen);
      break;
   //PUBCOMP packet received?
   case MQTT_PACKET_TYPE_PUBCOMP:
      //Process incoming PUBCOMP packet
      error = mqttClientProcessPubComp(context, dup, qos, retain, remainingLen);
      break;
   //SUBACK packet received?
   case MQTT_PACKET_TYPE_SUBACK:
      //Process incoming SUBACK packet
      error = mqttClientProcessSubAck(context, dup, qos, retain, remainingLen);
      break;
   //UNSUBACK packet received?
   case MQTT_PACKET_TYPE_UNSUBACK:
      //Process incoming UNSUBACK packet
      error = mqttClientProcessUnsubAck(context, dup, qos, retain, remainingLen);
      break;
   //PINGRESP packet received?
   case MQTT_PACKET_TYPE_PINGRESP:
      //Process incoming PINGRESP packet
      error = mqttClientProcessPingResp(context, dup, qos, retain, remainingLen);
      break;
   //Unknown packet received?
   default:
      //Report an error
      error = ERROR_INVALID_PACKET;
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming CONNACK packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] dup DUP flag from the fixed header
 * @param[in] qos QoS field from the fixed header
 * @param[in] retain RETAIN flag from the fixed header
 * @param[in] remainingLen Length of the variable header and the payload
 **/

error_t mqttClientProcessConnAck(MqttClientContext *context,
   bool_t dup, MqttQosLevel qos, bool_t retain, size_t remainingLen)
{
   error_t error;
   uint8_t connectAckFlags;
   uint8_t connectReturnCode;

   //If invalid flags are received, the receiver must close the network connection
   if(dup != FALSE && qos != MQTT_QOS_LEVEL_0 && retain != FALSE)
      return ERROR_INVALID_PACKET;

   //The first byte of the variable header is the Connect Acknowledge Flags
   error = mqttDeserializeByte(context->packet, context->packetLen,
      &context->packetPos, &connectAckFlags);

   //Failed to deserialize the Connect Acknowledge Flags?
   if(error)
      return error;

   //The second byte of the variable header is the Connect Return Code
   error = mqttDeserializeByte(context->packet, context->packetLen,
      &context->packetPos, &connectReturnCode);

   //Failed to deserialize the Connect Return Code?
   if(error)
      return error;

   //Any registered callback?
   if(context->callbacks.connAckCallback != NULL)
   {
      //Invoke user callback function
      context->callbacks.connAckCallback(context,
         connectAckFlags, connectReturnCode);
   }

   //Make sure the connection is accepted
   if(connectReturnCode != MQTT_CONNECT_RET_CODE_ACCEPTED)
      return ERROR_CONNECTION_REFUSED;

   //Notify the application that a CONNACK packet has been received
   if(context->packetType == MQTT_PACKET_TYPE_CONNECT)
      mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process incoming PUBLISH packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] dup DUP flag from the fixed header
 * @param[in] qos QoS field from the fixed header
 * @param[in] retain RETAIN flag from the fixed header
 * @param[in] remainingLen Length of the variable header and the payload
 **/

error_t mqttClientProcessPublish(MqttClientContext *context,
   bool_t dup, MqttQosLevel qos, bool_t retain, size_t remainingLen)
{
   error_t error;
   uint16_t packetId;
   char_t *topic;
   size_t topicLen;
   uint8_t *message;
   size_t messageLen;

   //The Topic Name must be present as the first field in the PUBLISH
   //packet variable header
   error = mqttDeserializeString(context->packet, context->packetLen,
      &context->packetPos, &topic, &topicLen);

   //Failed to deserialize Topic Name?
   if(error)
      return error;

   //Check QoS level
   if(qos != MQTT_QOS_LEVEL_0)
   {
      //The Packet Identifier field is only present in PUBLISH packets
      //where the QoS level is 1 or 2
      error = mqttDeserializeShort(context->packet, context->packetLen,
         &context->packetPos, &packetId);

      //Failed to deserialize Packet Identifier field?
      if(error)
         return error;
   }
   else
   {
      //No packet identifier
      packetId = 0;
   }

   //The payload contains the Application Message that is being published
   message = context->packet + context->packetPos;

   //The length of the payload can be calculated by subtracting the length of the
   //variable header from the Remaining Length field that is in the fixed header
   messageLen = context->packetLen - context->packetPos;

   //Make room for the NULL character at the end of the Topic Name
   osMemmove(topic - 1, topic, topicLen);
   //Properly terminate the string with a NULL character
   topic[topicLen - 1] = '\0';
   //Point to the first character of the Topic Name
   topic--;

   //Any registered callback?
   if(context->callbacks.publishCallback != NULL)
   {
      //Invoke user callback function
      context->callbacks.publishCallback(context, topic,
         message, messageLen, dup, qos, retain, packetId);
   }

   //Check QoS level
   if(qos == MQTT_QOS_LEVEL_1)
   {
      //A PUBACK packet is the response to a PUBLISH packet with QoS level 1
      error = mqttClientFormatPubAck(context, packetId);

      //Check status code
      if(!error)
      {
         //Debug message
         TRACE_INFO("MQTT: Sending PUBACK packet (%" PRIuSIZE " bytes)...\r\n", context->packetLen);
         TRACE_DEBUG_ARRAY("  ", context->packet, context->packetLen);

         //Point to the beginning of the packet
         context->packetPos = 0;

         //Send PUBACK packet
         mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
      }
   }
   else if(qos == MQTT_QOS_LEVEL_2)
   {
      //A PUBREC packet is the response to a PUBLISH packet with QoS 2. It is
      //the second packet of the QoS 2 protocol exchange
      error = mqttClientFormatPubRec(context, packetId);

      //Check status code
      if(!error)
      {
         //Debug message
         TRACE_INFO("MQTT: Sending PUBREC packet (%" PRIuSIZE " bytes)...\r\n", context->packetLen);
         TRACE_DEBUG_ARRAY("  ", context->packet, context->packetLen);

         //Point to the beginning of the packet
         context->packetPos = 0;

         //Send PUBREC packet
         mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming PUBACK packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] dup DUP flag from the fixed header
 * @param[in] qos QoS field from the fixed header
 * @param[in] retain RETAIN flag from the fixed header
 * @param[in] remainingLen Length of the variable header and the payload
 **/

error_t mqttClientProcessPubAck(MqttClientContext *context,
   bool_t dup, MqttQosLevel qos, bool_t retain, size_t remainingLen)
{
   error_t error;
   uint16_t packetId;

   //If invalid flags are received, the receiver must close the network connection
   if(dup != FALSE && qos != MQTT_QOS_LEVEL_0 && retain != FALSE)
      return ERROR_INVALID_PACKET;

   //The variable header contains the Packet Identifier from the PUBLISH
   //packet that is being acknowledged
   error = mqttDeserializeShort(context->packet, context->packetLen,
      &context->packetPos, &packetId);

   //Failed to deserialize Packet Identifier field?
   if(error)
      return error;

   //Any registered callback?
   if(context->callbacks.pubAckCallback != NULL)
   {
      //Invoke user callback function
      context->callbacks.pubAckCallback(context, packetId);
   }

   //Notify the application that a PUBACK packet has been received
   if(context->packetType == MQTT_PACKET_TYPE_PUBLISH && context->packetId == packetId)
      mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

   //Return status code
   return error;
}


/**
 * @brief Process incoming PUBREC packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] dup DUP flag from the fixed header
 * @param[in] qos QoS field from the fixed header
 * @param[in] retain RETAIN flag from the fixed header
 * @param[in] remainingLen Length of the variable header and the payload
 **/

error_t mqttClientProcessPubRec(MqttClientContext *context,
   bool_t dup, MqttQosLevel qos, bool_t retain, size_t remainingLen)
{
   error_t error;
   uint16_t packetId;

   //If invalid flags are received, the receiver must close the network connection
   if(dup != FALSE && qos != MQTT_QOS_LEVEL_0 && retain != FALSE)
      return ERROR_INVALID_PACKET;

   //The variable header contains the Packet Identifier from the PUBLISH
   //packet that is being acknowledged
   error = mqttDeserializeShort(context->packet, context->packetLen,
      &context->packetPos, &packetId);

   //Failed to deserialize Packet Identifier field?
   if(error)
      return error;

   //Any registered callback?
   if(context->callbacks.pubRecCallback != NULL)
   {
      //Invoke user callback function
      context->callbacks.pubRecCallback(context, packetId);
   }

   //A PUBREL packet is the response to a PUBREC packet. It is the third
   //packet of the QoS 2 protocol exchange
   error = mqttClientFormatPubRel(context, packetId);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("MQTT: Sending PUBREL packet (%" PRIuSIZE " bytes)...\r\n", context->packetLen);
      TRACE_DEBUG_ARRAY("  ", context->packet, context->packetLen);

      //Point to the beginning of the packet
      context->packetPos = 0;

      //Send PUBREL packet
      mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming PUBREL packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] dup DUP flag from the fixed header
 * @param[in] qos QoS field from the fixed header
 * @param[in] retain RETAIN flag from the fixed header
 * @param[in] remainingLen Length of the variable header and the payload
 **/

error_t mqttClientProcessPubRel(MqttClientContext *context,
   bool_t dup, MqttQosLevel qos, bool_t retain, size_t remainingLen)
{
   error_t error;
   uint16_t packetId;

   //If invalid flags are received, the receiver must close the network connection
   if(dup != FALSE && qos != MQTT_QOS_LEVEL_1 && retain != FALSE)
      return ERROR_INVALID_PACKET;

   //The variable header contains the same Packet Identifier as the PUBREC
   //packet that is being acknowledged
   error = mqttDeserializeShort(context->packet, context->packetLen,
      &context->packetPos, &packetId);

   //Failed to deserialize Packet Identifier field?
   if(error)
      return error;

   //Any registered callback?
   if(context->callbacks.pubRelCallback != NULL)
   {
      //Invoke user callback function
      context->callbacks.pubRelCallback(context, packetId);
   }

   //A PUBCOMP packet is the response to a PUBREL packet. It is the fourth and
   //final packet of the QoS 2 protocol exchange
   error = mqttClientFormatPubComp(context, packetId);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("MQTT: Sending PUBCOMP packet (%" PRIuSIZE " bytes)...\r\n", context->packetLen);
      TRACE_DEBUG_ARRAY("  ", context->packet, context->packetLen);

      //Point to the beginning of the packet
      context->packetPos = 0;

      //Send PUBCOMP packet
      mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming PUBCOMP packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] dup DUP flag from the fixed header
 * @param[in] qos QoS field from the fixed header
 * @param[in] retain RETAIN flag from the fixed header
 * @param[in] remainingLen Length of the variable header and the payload
 **/

error_t mqttClientProcessPubComp(MqttClientContext *context,
   bool_t dup, MqttQosLevel qos, bool_t retain, size_t remainingLen)
{
   error_t error;
   uint16_t packetId;

   //If invalid flags are received, the receiver must close the network connection
   if(dup != FALSE && qos != MQTT_QOS_LEVEL_0 && retain != FALSE)
      return ERROR_INVALID_PACKET;

   //The variable header contains the same Packet Identifier as the PUBREL
   //packet that is being acknowledged
   error = mqttDeserializeShort(context->packet, context->packetLen,
      &context->packetPos, &packetId);

   //Failed to deserialize Packet Identifier field?
   if(error)
      return error;

   //Any registered callback?
   if(context->callbacks.pubCompCallback != NULL)
   {
      //Invoke user callback function
      context->callbacks.pubCompCallback(context, packetId);
   }

   //Notify the application that a PUBCOMP packet has been received
   if(context->packetType == MQTT_PACKET_TYPE_PUBLISH && context->packetId == packetId)
      mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process incoming SUBACK packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] dup DUP flag from the fixed header
 * @param[in] qos QoS field from the fixed header
 * @param[in] retain RETAIN flag from the fixed header
 * @param[in] remainingLen Length of the variable header and the payload
 **/

error_t mqttClientProcessSubAck(MqttClientContext *context,
   bool_t dup, MqttQosLevel qos, bool_t retain, size_t remainingLen)
{
   error_t error;
   uint16_t packetId;

   //If invalid flags are received, the receiver must close the network connection
   if(dup != FALSE && qos != MQTT_QOS_LEVEL_0 && retain != FALSE)
      return ERROR_INVALID_PACKET;

   //The variable header contains the Packet Identifier from the SUBSCRIBE
   //packet that is being acknowledged
   error = mqttDeserializeShort(context->packet, context->packetLen,
      &context->packetPos, &packetId);

   //Failed to deserialize Packet Identifier field?
   if(error)
      return error;

   //Any registered callback?
   if(context->callbacks.subAckCallback != NULL)
   {
      //Invoke user callback function
      context->callbacks.subAckCallback(context, packetId);
   }

   //Notify the application that a SUBACK packet has been received
   if(context->packetType == MQTT_PACKET_TYPE_SUBSCRIBE && context->packetId == packetId)
      mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process incoming UNSUBACK packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] dup DUP flag from the fixed header
 * @param[in] qos QoS field from the fixed header
 * @param[in] retain RETAIN flag from the fixed header
 * @param[in] remainingLen Length of the variable header and the payload
 **/

error_t mqttClientProcessUnsubAck(MqttClientContext *context,
   bool_t dup, MqttQosLevel qos, bool_t retain, size_t remainingLen)
{
   error_t error;
   uint16_t packetId;

   //If invalid flags are received, the receiver must close the network connection
   if(dup != FALSE && qos != MQTT_QOS_LEVEL_0 && retain != FALSE)
      return ERROR_INVALID_PACKET;

   //The variable header contains the Packet Identifier from the UNSUBSCRIBE
   //packet that is being acknowledged
   error = mqttDeserializeShort(context->packet, context->packetLen,
      &context->packetPos, &packetId);

   //Failed to deserialize Packet Identifier field?
   if(error)
      return error;

   //Any registered callback?
   if(context->callbacks.unsubAckCallback != NULL)
   {
      //Invoke user callback function
      context->callbacks.unsubAckCallback(context, packetId);
   }

   //Notify the application that an UNSUBACK packet has been received
   if(context->packetType == MQTT_PACKET_TYPE_UNSUBSCRIBE && context->packetId == packetId)
      mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process incoming PINGRESP packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] dup DUP flag from the fixed header
 * @param[in] qos QoS field from the fixed header
 * @param[in] retain RETAIN flag from the fixed header
 * @param[in] remainingLen Length of the variable header and the payload
 **/

error_t mqttClientProcessPingResp(MqttClientContext *context,
   bool_t dup, MqttQosLevel qos, bool_t retain, size_t remainingLen)
{
   //If invalid flags are received, the receiver must close the network connection
   if(dup != FALSE && qos != MQTT_QOS_LEVEL_0 && retain != FALSE)
      return ERROR_INVALID_PACKET;

   //Any registered callback?
   if(context->callbacks.pingRespCallback != NULL)
   {
      //Invoke user callback function
      context->callbacks.pingRespCallback(context);
   }

   //Notify the application that an PINGRESP packet has been received
   if(context->packetType == MQTT_PACKET_TYPE_PINGREQ)
      mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format CONNECT packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] cleanSession If this flag is set, then the client and server
 *   must discard any previous session and start a new one
 * @return Error code
 **/

error_t mqttClientFormatConnect(MqttClientContext *context,
   bool_t cleanSession)
{
   error_t error;
   size_t n;
   uint8_t connectFlags;
   MqttClientWillMessage *willMessage;

   //Make room for the fixed header
   n = MQTT_MAX_HEADER_SIZE;

   //Check protocol version
   if(context->settings.version == MQTT_VERSION_3_1)
   {
      //The Protocol Name is a UTF-8 encoded string that represents the
      //protocol name "MQIsdp"
      error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
         &n, MQTT_PROTOCOL_NAME_3_1, osStrlen(MQTT_PROTOCOL_NAME_3_1));
   }
   else if(context->settings.version == MQTT_VERSION_3_1_1)
   {
      //The Protocol Name is a UTF-8 encoded string that represents the
      //protocol name "MQTT"
      error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
         &n, MQTT_PROTOCOL_NAME_3_1_1, osStrlen(MQTT_PROTOCOL_NAME_3_1_1));
   }
   else
   {
      //Invalid protocol level
      error = ERROR_INVALID_VERSION;
   }

   //Any error to report?
   if(error)
      return error;

   //The Protocol Level represents the revision level of the protocol
   //used by the client
   error = mqttSerializeByte(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, context->settings.version);

   //Failed to serialize data?
   if(error)
      return error;

   //The Connect Flags byte contains a number of parameters specifying
   //the behavior of the MQTT connection
   connectFlags = 0;

   //If CleanSession is set to 1, the client and server must discard any
   //previous session and start a new one
   if(cleanSession)
      connectFlags |= MQTT_CONNECT_FLAG_CLEAN_SESSION;

   //If the client supplies a zero-byte Client Identifier, the client must
   //also set CleanSession to 1
   if(context->settings.clientId[0] == '\0')
      connectFlags |= MQTT_CONNECT_FLAG_CLEAN_SESSION;

   //Point to the Will message
   willMessage = &context->settings.willMessage;

   //Check whether a valid Will message has been specified
   if(willMessage->topic[0] != '\0')
   {
      //Set the Will flag
      connectFlags |= MQTT_CONNECT_FLAG_WILL;

      //Check the Will QoS level
      if(willMessage->qos == MQTT_QOS_LEVEL_1)
         connectFlags |= MQTT_CONNECT_FLAG_WILL_QOS_1;
      else if(willMessage->qos == MQTT_QOS_LEVEL_2)
         connectFlags |= MQTT_CONNECT_FLAG_WILL_QOS_2;

      //The Will Retain flag specifies if the Will Message is to be
      //retained when it is published
      if(willMessage->retain)
         connectFlags |= MQTT_CONNECT_FLAG_WILL_RETAIN;
   }

   //Check whether a valid user name has been specified
   if(context->settings.username[0] != '\0')
      connectFlags |= MQTT_CONNECT_FLAG_USERNAME;

   //Check whether a valid password has been specified
   if(context->settings.password[0] != '\0')
      connectFlags |= MQTT_CONNECT_FLAG_PASSWORD;

   //Write the Connect Flags to the output buffer
   error = mqttSerializeByte(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, connectFlags);

   //Failed to serialize data?
   if(error)
      return error;

   //The Keep Alive is a time interval measured in seconds. It is the maximum
   //time interval that is permitted to elapse between the point at which the
   //client finishes transmitting one control packet and the point it starts
   //sending the next
   error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, context->settings.keepAlive);

   //Failed to serialize data?
   if(error)
      return error;

   //The Client Identifier identifies the client to the server. The Client
   //Identifier must be present and must be the first field in the CONNECT
   //packet payload
   error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, context->settings.clientId, osStrlen(context->settings.clientId));

   //Failed to serialize data?
   if(error)
      return error;

   //If the Will Flag is set to 1, the Will Topic is the next field in
   //the payload
   if(willMessage->topic[0] != '\0')
   {
      //Write the Will Topic to the output buffer
      error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
         &n, willMessage->topic, osStrlen(willMessage->topic));

      //Failed to serialize data?
      if(error)
         return error;

      //Write the Will message to the output buffer
      error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
         &n, willMessage->payload, willMessage->length);

      //Failed to serialize data?
      if(error)
         return error;
   }

   //If the User Name Flag is set to 1, this is the next field in the payload
   if(context->settings.username[0] != '\0')
   {
      //Write the User Name to the output buffer
      error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
         &n, context->settings.username, osStrlen(context->settings.username));

      //Failed to serialize data?
      if(error)
         return error;
   }

   //If the Password Flag is set to 1, this is the next field in the payload
   if(context->settings.password[0] != '\0')
   {
      //Write the Password to the output buffer
      error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
         &n, context->settings.password, osStrlen(context->settings.password));

      //Failed to serialize data?
      if(error)
         return error;
   }

   //Calculate the length of the variable header and the payload
   context->packetLen = n - MQTT_MAX_HEADER_SIZE;

   //The fixed header will be encoded in reverse order
   n = MQTT_MAX_HEADER_SIZE;

   //Prepend the variable header and the payload with the fixed header
   error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_CONNECT,
      FALSE, MQTT_QOS_LEVEL_0, FALSE, context->packetLen);

   //Failed to serialize fixed header?
   if(error)
      return error;

   //Point to the first byte of the MQTT packet
   context->packet = context->buffer + n;
   //Calculate the length of the MQTT packet
   context->packetLen += MQTT_MAX_HEADER_SIZE - n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format PUBLISH packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] topic Topic name
 * @param[in] message Message payload
 * @param[in] length Length of the message payload
 * @param[in] qos QoS level to be used when publishing the message
 * @param[in] retain This flag specifies if the message is to be retained
 * @return Error code
 **/

error_t mqttClientFormatPublish(MqttClientContext *context, const char_t *topic,
   const void *message, size_t length, MqttQosLevel qos, bool_t retain)
{
   error_t error;
   size_t n;

   //Make room for the fixed header
   n = MQTT_MAX_HEADER_SIZE;

   //The Topic Name must be present as the first field in the PUBLISH
   //packet variable header
   error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, topic, osStrlen(topic));

   //Failed to serialize Topic Name?
   if(error)
      return error;

   //Check QoS level
   if(qos != MQTT_QOS_LEVEL_0)
   {
      //Each time a client sends a new PUBLISH packet it must assign it
      //a currently unused packet identifier
      context->packetId++;

      //The Packet Identifier field is only present in PUBLISH packets
      //where the QoS level is 1 or 2
      error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
         &n, context->packetId);

      //Failed to serialize Packet Identifier field?
      if(error)
         return error;
   }

   //The payload contains the Application Message that is being published
   error = mqttSerializeData(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, message, length);

   //Failed to serialize Application Message?
   if(error)
      return error;

   //Calculate the length of the variable header and the payload
   context->packetLen = n - MQTT_MAX_HEADER_SIZE;

   //The fixed header will be encoded in reverse order
   n = MQTT_MAX_HEADER_SIZE;

   //Prepend the variable header and the payload with the fixed header
   error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PUBLISH,
      FALSE, qos, retain, context->packetLen);

   //Failed to serialize fixed header?
   if(error)
      return error;

   //Point to the first byte of the MQTT packet
   context->packet = context->buffer + n;
   //Calculate the length of the MQTT packet
   context->packetLen += MQTT_MAX_HEADER_SIZE - n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format PUBACK packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] packetId Packet identifier
 * @return Error code
 **/

error_t mqttClientFormatPubAck(MqttClientContext *context, uint16_t packetId)
{
   error_t error;
   size_t n;

   //Make room for the fixed header
   n = MQTT_MAX_HEADER_SIZE;

   //The variable header contains the Packet Identifier from the PUBLISH
   //packet that is being acknowledged
   error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, packetId);

   //Failed to serialize Packet Identifier field?
   if(error)
      return error;

   //Calculate the length of the variable header and the payload
   context->packetLen = n - MQTT_MAX_HEADER_SIZE;

   //The fixed header will be encoded in reverse order
   n = MQTT_MAX_HEADER_SIZE;

   //Prepend the variable header and the payload with the fixed header
   error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PUBACK,
      FALSE, MQTT_QOS_LEVEL_0, FALSE, context->packetLen);

   //Failed to serialize fixed header?
   if(error)
      return error;

   //Point to the first byte of the MQTT packet
   context->packet = context->buffer + n;
   //Calculate the length of the MQTT packet
   context->packetLen += MQTT_MAX_HEADER_SIZE - n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format PUBREC packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] packetId Packet identifier
 * @return Error code
 **/

error_t mqttClientFormatPubRec(MqttClientContext *context, uint16_t packetId)
{
   error_t error;
   size_t n;

   //Make room for the fixed header
   n = MQTT_MAX_HEADER_SIZE;

   //The variable header contains the Packet Identifier from the PUBLISH
   //packet that is being acknowledged
   error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, packetId);

   //Failed to serialize Packet Identifier field?
   if(error)
      return error;

   //Calculate the length of the variable header and the payload
   context->packetLen = n - MQTT_MAX_HEADER_SIZE;

   //The fixed header will be encoded in reverse order
   n = MQTT_MAX_HEADER_SIZE;

   //Prepend the variable header and the payload with the fixed header
   error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PUBREC,
      FALSE, MQTT_QOS_LEVEL_0, FALSE, context->packetLen);

   //Failed to serialize fixed header?
   if(error)
      return error;

   //Point to the first byte of the MQTT packet
   context->packet = context->buffer + n;
   //Calculate the length of the MQTT packet
   context->packetLen += MQTT_MAX_HEADER_SIZE - n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format PUBREL packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] packetId Packet identifier
 * @return Error code
 **/

error_t mqttClientFormatPubRel(MqttClientContext *context, uint16_t packetId)
{
   error_t error;
   size_t n;

   //Make room for the fixed header
   n = MQTT_MAX_HEADER_SIZE;

   //The variable header contains the same Packet Identifier as the PUBREC
   //packet that is being acknowledged
   error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, packetId);

   //Failed to serialize Packet Identifier field?
   if(error)
      return error;

   //Calculate the length of the variable header and the payload
   context->packetLen = n - MQTT_MAX_HEADER_SIZE;

   //The fixed header will be encoded in reverse order
   n = MQTT_MAX_HEADER_SIZE;

   //Prepend the variable header and the payload with the fixed header
   error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PUBREL,
      FALSE, MQTT_QOS_LEVEL_1, FALSE, context->packetLen);

   //Failed to serialize fixed header?
   if(error)
      return error;

   //Point to the first byte of the MQTT packet
   context->packet = context->buffer + n;
   //Calculate the length of the MQTT packet
   context->packetLen += MQTT_MAX_HEADER_SIZE - n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format PUBCOMP packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] packetId Packet identifier
 * @return Error code
 **/

error_t mqttClientFormatPubComp(MqttClientContext *context, uint16_t packetId)
{
   error_t error;
   size_t n;

   //Make room for the fixed header
   n = MQTT_MAX_HEADER_SIZE;

   //The variable header contains the same Packet Identifier as the PUBREL
   //packet that is being acknowledged
   error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, packetId);

   //Failed to serialize Packet Identifier field?
   if(error)
      return error;

   //Calculate the length of the variable header and the payload
   context->packetLen = n - MQTT_MAX_HEADER_SIZE;

   //The fixed header will be encoded in reverse order
   n = MQTT_MAX_HEADER_SIZE;

   //Prepend the variable header and the payload with the fixed header
   error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PUBCOMP,
      FALSE, MQTT_QOS_LEVEL_0, FALSE, context->packetLen);

   //Failed to serialize fixed header?
   if(error)
      return error;

   //Point to the first byte of the MQTT packet
   context->packet = context->buffer + n;
   //Calculate the length of the MQTT packet
   context->packetLen += MQTT_MAX_HEADER_SIZE - n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format SUBSCRIBE packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] topic Topic filter
 * @param[in] qos Maximum QoS level at which the server can send application
 *   messages to the client
 * @return Error code
 **/

error_t mqttClientFormatSubscribe(MqttClientContext *context,
   const char_t *topic, MqttQosLevel qos)
{
   error_t error;
   size_t n;

   //Make room for the fixed header
   n = MQTT_MAX_HEADER_SIZE;

   //Each time a client sends a new SUBSCRIBE packet it must assign it
   //a currently unused packet identifier
   context->packetId++;

   //Write Packet Identifier to the output buffer
   error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, context->packetId);

   //Failed to serialize data?
   if(error)
      return error;

   //Write the Topic Filter to the output buffer
   error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, topic, osStrlen(topic));

   //Failed to serialize data?
   if(error)
      return error;

   //Write the Requested QoS to the output buffer
   error = mqttSerializeByte(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, qos);

   //Failed to serialize data?
   if(error)
      return error;

   //Calculate the length of the variable header and the payload
   context->packetLen = n - MQTT_MAX_HEADER_SIZE;

   //The fixed header will be encoded in reverse order
   n = MQTT_MAX_HEADER_SIZE;

   //Prepend the variable header and the payload with the fixed header
   error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_SUBSCRIBE,
      FALSE, MQTT_QOS_LEVEL_1, FALSE, context->packetLen);

   //Failed to serialize fixed header?
   if(error)
      return error;

   //Point to the first byte of the MQTT packet
   context->packet = context->buffer + n;
   //Calculate the length of the MQTT packet
   context->packetLen += MQTT_MAX_HEADER_SIZE - n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format UNSUBSCRIBE packet
 * @param[in] context Pointer to the MQTT client context
 * @param[in] topic Topic filter
 * @return Error code
 **/

error_t mqttClientFormatUnsubscribe(MqttClientContext *context,
   const char_t *topic)
{
   error_t error;
   size_t n;

   //Make room for the fixed header
   n = MQTT_MAX_HEADER_SIZE;

   //Each time a client sends a new UNSUBSCRIBE packet it must assign it
   //a currently unused packet identifier
   context->packetId++;

   //Write Packet Identifier to the output buffer
   error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, context->packetId);

   //Failed to serialize data?
   if(error)
      return error;

   //Write the Topic Filter to the output buffer
   error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
      &n, topic, osStrlen(topic));

   //Failed to serialize data?
   if(error)
      return error;

   //Calculate the length of the variable header and the payload
   context->packetLen = n - MQTT_MAX_HEADER_SIZE;

   //The fixed header will be encoded in reverse order
   n = MQTT_MAX_HEADER_SIZE;

   //Prepend the variable header and the payload with the fixed header
   error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_UNSUBSCRIBE,
      FALSE, MQTT_QOS_LEVEL_1, FALSE, context->packetLen);

   //Failed to serialize fixed header?
   if(error)
      return error;

   //Point to the first byte of the MQTT packet
   context->packet = context->buffer + n;
   //Calculate the length of the MQTT packet
   context->packetLen += MQTT_MAX_HEADER_SIZE - n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format PINGREQ packet
 * @param[in] context Pointer to the MQTT client context
 * @return Error code
 **/

error_t mqttClientFormatPingReq(MqttClientContext *context)
{
   error_t error;
   size_t n;

   //The fixed header will be encoded in reverse order
   n = MQTT_MAX_HEADER_SIZE;

   //The PINGREQ packet does not contain any variable header nor payload
   error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PINGREQ,
      FALSE, MQTT_QOS_LEVEL_0, FALSE, 0);

   //Failed to serialize fixed header?
   if(error)
      return error;

   //Point to the first byte of the MQTT packet
   context->packet = context->buffer + n;
   //Calculate the length of the MQTT packet
   context->packetLen = MQTT_MAX_HEADER_SIZE - n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format DISCONNECT packet
 * @param[in] context Pointer to the MQTT client context
 * @return Error code
 **/

error_t mqttClientFormatDisconnect(MqttClientContext *context)
{
   error_t error;
   size_t n;

   //The fixed header will be encoded in reverse order
   n = MQTT_MAX_HEADER_SIZE;

   //The DISCONNECT packet does not contain any variable header nor payload
   error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_DISCONNECT,
      FALSE, MQTT_QOS_LEVEL_0, FALSE, 0);

   //Failed to serialize fixed header?
   if(error)
      return error;

   //Point to the first byte of the MQTT packet
   context->packet = context->buffer + n;
   //Calculate the length of the MQTT packet
   context->packetLen = MQTT_MAX_HEADER_SIZE - n;

   //Successful processing
   return NO_ERROR;
}

#endif
