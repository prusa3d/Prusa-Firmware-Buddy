/**
 * @file mqtt_sn_message.c
 * @brief MQTT-SN message formatting and parsing
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
#include "mqtt_sn/mqtt_sn_message.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MQTT_SN_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Format MQTT-SN message header
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] type Message type
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnFormatHeader(MqttSnMessage *message, MqttSnMsgType type,
   size_t length)
{
   //Check the length of the message
   if((length + sizeof(MqttSnHeader)) < UINT8_MAX)
   {
      MqttSnHeader *header;

      //Make room for the message header (2 bytes)
      osMemmove(message->buffer + sizeof(MqttSnHeader), message->buffer,
         length);

      //The Length field specifies the total number of octets contained in
      //the message, including the Length field itself
      length += sizeof(MqttSnHeader);

      //Point to the buffer where to format the message header
      header = (MqttSnHeader *) message->buffer;

      //The Length field is 1-octet long
      header->length = (uint8_t) length;
      //The MsgType specifies the message type
      header->msgType = type;
   }
   else
   {
      MqttSnExtHeader *header;

      //Make room for the message header (4 bytes)
      osMemmove(message->buffer + sizeof(MqttSnExtHeader), message->buffer,
         length);

      //The Length field specifies the total number of octets contained in
      //the message, including the Length field itself
      length += sizeof(MqttSnExtHeader);

      //Point to the buffer where to format the message header
      header = (MqttSnExtHeader *) message->buffer;

      //The Length field is 3-octet long
      header->prefix = 0x01;
      header->length = htons(length);
      //The MsgType specifies the message type
      header->msgType = type;
   }

   //Terminate the payload with a NULL character
   message->buffer[length] = '\0';

   //Save the length of the message
   message->length = length;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format SEARCHGW message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] radius Broadcast radius of SEARCHGW message
 * @return Error code
 **/

error_t mqttSnFormatSearchGw(MqttSnMessage *message,
   uint8_t radius)
{
   size_t length;
   MqttSnSearchGw *searchGw;

   //Point to the buffer where to format the message
   searchGw = (MqttSnSearchGw *) message->buffer;

   //Format SEARCHGW message
   searchGw->radius = 0;

   //Compute the length of the message
   length = sizeof(MqttSnSearchGw);

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_SEARCHGW, length);
}


/**
 * @brief Format CONNECT message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] flags Flags
 * @param[in] duration Value of the keep-alive timer
 * @param[in] clientId Client identifier
 * @return Error code
 **/

error_t mqttSnFormatConnect(MqttSnMessage *message, MqttSnFlags flags,
   uint16_t duration, const char_t *clientId)
{
   size_t length;
   MqttSnConnect *connect;

   //Point to the buffer where to format the message
   connect = (MqttSnConnect *) message->buffer;

   //Format CONNECT message
   connect->flags = flags;
   connect->protocolId = MQTT_SN_PROTOCOL_ID;
   connect->duration = htons(duration);

   //Copy client identifier
   osStrcpy(connect->clientId, clientId);

   //Compute the length of the message
   length = sizeof(MqttSnConnect) + osStrlen(clientId);

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_CONNECT, length);
}


/**
 * @brief Format WILLTOPIC message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] flags Flags
 * @param[in] topicName Topic name
 * @return Error code
 **/

error_t mqttSnFormatWillTopic(MqttSnMessage *message, MqttSnFlags flags,
   const char_t *topicName)
{
   size_t length;
   MqttSnWillTopic *willTopic;

   //Point to the buffer where to format the message
   willTopic = (MqttSnWillTopic *) message->buffer;

   //Valid Will topic?
   if(topicName[0] != '\0')
   {
      //Format WILLTOPICUPD message
      willTopic->flags = flags;
      osStrcpy(willTopic->willTopic, topicName);

      //Compute the length of the message
      length = sizeof(MqttSnWillTopic) + osStrlen(topicName);
   }
   else
   {
      //An empty WILLTOPIC message (without Flags and WillTopic field) is
      //used by a client to delete the Will topic and the Will message stored
      //in the server
      length = 0;
   }

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_WILLTOPIC, length);
}


/**
 * @brief Format WILLMSG message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] data Will message
 * @param[in] dataLen Length of the Will message
 * @return Error code
 **/

error_t mqttSnFormatWillMsg(MqttSnMessage *message, const void *data,
   size_t dataLen)
{
   MqttSnWillMsg *willMsg;

   //Point to the buffer where to format the message
   willMsg = (MqttSnWillMsg *) message->buffer;

   //Copy Will message
   osMemcpy(willMsg, data, dataLen);

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_WILLMSG, dataLen);
}


/**
 * @brief Format REGISTER message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] msgId Message identifier
 * @param[in] topicId Topic identifier
 * @param[in] topicName Topic name
 * @return Error code
 **/

error_t mqttSnFormatRegister(MqttSnMessage *message,
   uint16_t msgId, uint16_t topicId, const char_t *topicName)
{
   size_t length;
   MqttSnRegister *reg;

   //Point to the buffer where to format the message
   reg = (MqttSnRegister *) message->buffer;

   //Format REGISTER message
   reg->topicId = htons(topicId);
   reg->msgId = htons(msgId);

   //Copy topic name
   osStrcpy(reg->topicName, topicName);

   //Compute the length of the message
   length = sizeof(MqttSnRegister) + osStrlen(topicName);

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_REGISTER, length);
}


/**
 * @brief Format REGACK message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] msgId Message identifier
 * @param[in] topicId Topic identifier
 * @param[in] returnCode Return code
 * @return Error code
 **/

error_t mqttSnFormatRegAck(MqttSnMessage *message, uint16_t msgId,
   uint16_t topicId, MqttSnReturnCode returnCode)
{
   size_t length;
   MqttSnRegAck *regAck;

   //Point to the buffer where to format the message
   regAck = (MqttSnRegAck *) message->buffer;

   //Format REGACK message
   regAck->topicId = htons(topicId);
   regAck->msgId = htons(msgId);
   regAck->returnCode = returnCode;

   //Compute the length of the message
   length = sizeof(MqttSnRegAck);

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_REGACK, length);
}


/**
 * @brief Format PUBLISH message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] flags Flags
 * @param[in] msgId Message identifier
 * @param[in] topicId Topic identifier
 * @param[in] topicName Short topic name
 * @param[in] data Message payload
 * @param[in] dataLen Length of the message payload
 * @return Error code
 **/

error_t mqttSnFormatPublish(MqttSnMessage *message, MqttSnFlags flags,
   uint16_t msgId, uint16_t topicId, const char_t *topicName,
   const uint8_t *data, size_t dataLen)
{
   error_t error;
   size_t length;
   MqttSnPublish *publish;

   //Initialize status code
   error = NO_ERROR;

   //Point to the buffer where to format the message
   publish = (MqttSnPublish *) message->buffer;

   //Format PUBLISH message
   publish->flags = flags;
   publish->msgId = htons(msgId);

   //Check the type of topic identifier
   if(flags.topicIdType == MQTT_SN_NORMAL_TOPIC_ID ||
      flags.topicIdType == MQTT_SN_PREDEFINED_TOPIC_ID)
   {
      //Copy normal or predefined topic ID
      publish->topicId = htons(topicId);
   }
   else if(flags.topicIdType == MQTT_SN_SHORT_TOPIC_NAME)
   {
      //Make sure the short topic name is valid
      if(topicName != NULL && osStrlen(topicName) == 2)
      {
         //Short topic names are topic names that have a fixed length of two
         //octets. They are short enough for being carried together with the
         //data within PUBLISH messages
         publish->topicId = htons((topicName[0] << 8) | topicName[1]);
      }
      else
      {
         //Report an error
         error = ERROR_INVALID_PARAMETER;
      }
   }
   else
   {
      //Report an error
      error = ERROR_INVALID_PARAMETER;
   }

   //Check status code
   if(!error)
   {
      //Copy message payload
      osMemcpy(publish->data, data, dataLen);

      //Compute the length of the message
      length = sizeof(MqttSnPublish) + dataLen;

      //Format MQTT-SN message header
      error = mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_PUBLISH, length);
   }

   //Return status code
   return error;
}


/**
 * @brief Format PUBACK message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] msgId Message identifier of the corresponding PUBLISH message
 * @param[in] topicId Topic identifier
 * @param[in] returnCode Return code
 * @return Error code
 **/

error_t mqttSnFormatPubAck(MqttSnMessage *message, uint16_t msgId,
   uint16_t topicId, MqttSnReturnCode returnCode)
{
   size_t length;
   MqttSnPubAck *pubAck;

   //Point to the buffer where to format the message
   pubAck = (MqttSnPubAck *) message->buffer;

   //Format PUBACK message
   pubAck->topicId = htons(topicId);
   pubAck->msgId = htons(msgId);
   pubAck->returnCode = returnCode;

   //Compute the length of the message
   length = sizeof(MqttSnPubAck);

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_PUBACK, length);
}


/**
 * @brief Format PUBREC message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] msgId Message identifier of the corresponding PUBLISH message
 * @return Error code
 **/

error_t mqttSnFormatPubRec(MqttSnMessage *message, uint16_t msgId)
{
   size_t length;
   MqttSnPubRec *pubRec;

   //Point to the buffer where to format the message
   pubRec = (MqttSnPubRec *) message->buffer;

   //Format PUBREC message
   pubRec->msgId = htons(msgId);

   //Compute the length of the message
   length = sizeof(MqttSnPubRec);

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_PUBREC, length);
}


/**
 * @brief Format PUBREL message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] msgId Message identifier of the corresponding PUBLISH message
 * @return Error code
 **/

error_t mqttSnFormatPubRel(MqttSnMessage *message, uint16_t msgId)
{
   size_t length;
   MqttSnPubRel *pubRel;

   //Point to the buffer where to format the message
   pubRel = (MqttSnPubRel *) message->buffer;

   //Format PUBREL message
   pubRel->msgId = htons(msgId);

   //Compute the length of the message
   length = sizeof(MqttSnPubRel);

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_PUBREL, length);
}


/**
 * @brief Format PUBCOMP message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] msgId Message identifier of the corresponding PUBLISH message
 * @return Error code
 **/

error_t mqttSnFormatPubComp(MqttSnMessage *message, uint16_t msgId)
{
   size_t length;
   MqttSnPubComp *pubComp;

   //Point to the buffer where to format the message
   pubComp = (MqttSnPubComp *) message->buffer;

   //Format PUBCOMP message
   pubComp->msgId = htons(msgId);

   //Compute the length of the message
   length = sizeof(MqttSnPubComp);

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_PUBCOMP, length);
}


/**
 * @brief Format SUBSCRIBE message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] flags Flags
 * @param[in] msgId Message identifier
 * @param[in] topicId Topic identifier
 * @param[in] topicName Topic name
 * @return Error code
 **/

error_t mqttSnFormatSubscribe(MqttSnMessage *message, MqttSnFlags flags,
   uint16_t msgId, uint16_t topicId, const char_t *topicName)
{
   error_t error;
   size_t length;
   MqttSnSubscribe *subscribe;

   //Initialize status code
   error = NO_ERROR;

   //Point to the buffer where to format the message
   subscribe = (MqttSnSubscribe *) message->buffer;

   //Format SUBSCRIBE message
   subscribe->flags = flags;
   subscribe->msgId = htons(msgId);

   //Check the type of topic identifier
   if(flags.topicIdType == MQTT_SN_NORMAL_TOPIC_NAME)
   {
      //Make sure the topic name is valid
      if(topicName != NULL)
      {
         //Copy topic name
         osStrcpy(subscribe->topicName, topicName);
         //Compute the length of the message
         length = sizeof(MqttSnSubscribe) + osStrlen(topicName);
      }
      else
      {
         //Report an error
         error = ERROR_INVALID_PARAMETER;
      }
   }
   else if(flags.topicIdType == MQTT_SN_SHORT_TOPIC_NAME)
   {
      //Make sure the short topic name is valid
      if(topicName != NULL && osStrlen(topicName) == 2)
      {
         //Copy topic name
         osStrcpy(subscribe->topicName, topicName);
         //Compute the length of the message
         length = sizeof(MqttSnSubscribe) + osStrlen(topicName);
      }
      else
      {
         //Report an error
         error = ERROR_INVALID_PARAMETER;
      }
   }
   else if(flags.topicIdType == MQTT_SN_PREDEFINED_TOPIC_ID)
   {
      //Copy predefined topic ID
      STORE16BE(topicId, subscribe->topicName);
      //Compute the length of the message
      length = sizeof(MqttSnSubscribe) + sizeof(uint16_t);
   }
   else
   {
      //Report an error
      error = ERROR_INVALID_PARAMETER;
   }

   //Check status code
   if(!error)
   {
      //Format MQTT-SN message header
      error = mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_SUBSCRIBE, length);
   }

   //Return status code
   return error;
}


/**
 * @brief Format UNSUBSCRIBE message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] flags Flags
 * @param[in] msgId Message identifier
 * @param[in] topicId Topic identifier
 * @param[in] topicName Topic name
 * @return Error code
 **/

error_t mqttSnFormatUnsubscribe(MqttSnMessage *message, MqttSnFlags flags,
   uint16_t msgId, uint16_t topicId, const char_t *topicName)
{
   error_t error;
   size_t length;
   MqttSnUnsubscribe *unsubscribe;

   //Initialize status code
   error = NO_ERROR;

   //Point to the buffer where to format the message
   unsubscribe = (MqttSnUnsubscribe *) message->buffer;

   //Format UNSUBSCRIBE message
   unsubscribe->flags = flags;
   unsubscribe->msgId = htons(msgId);

   //Check the type of topic identifier
   if(flags.topicIdType == MQTT_SN_NORMAL_TOPIC_NAME)
   {
      //Make sure the topic name is valid
      if(topicName != NULL)
      {
         //Copy topic name
         osStrcpy(unsubscribe->topicName, topicName);
         //Compute the length of the message
         length = sizeof(MqttSnSubscribe) + osStrlen(topicName);
      }
      else
      {
         //Report an error
         error = ERROR_INVALID_PARAMETER;
      }
   }
   else if(flags.topicIdType == MQTT_SN_SHORT_TOPIC_NAME)
   {
      //Make sure the short topic name is valid
      if(topicName != NULL && osStrlen(topicName) == 2)
      {
         //Copy topic name
         osStrcpy(unsubscribe->topicName, topicName);
         //Compute the length of the message
         length = sizeof(MqttSnSubscribe) + osStrlen(topicName);
      }
      else
      {
         //Report an error
         error = ERROR_INVALID_PARAMETER;
      }
   }
   else if(flags.topicIdType == MQTT_SN_PREDEFINED_TOPIC_ID)
   {
      //Copy predefined topic ID
      STORE16BE(topicId, unsubscribe->topicName);
      //Compute the length of the message
      length = sizeof(MqttSnSubscribe) + sizeof(uint16_t);
   }
   else
   {
      //Report an error
      error = ERROR_INVALID_PARAMETER;
   }

   //Check status code
   if(!error)
   {
      //Format MQTT-SN message header
      error = mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_UNSUBSCRIBE, length);
   }

   //Return status code
   return error;
}


/**
 * @brief Format PINGREQ message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] clientId Client identifier
 * @return Error code
 **/

error_t mqttSnFormatPingReq(MqttSnMessage *message, const char_t *clientId)
{
   size_t length;
   MqttSnPingReq *pingReq;

   //Point to the buffer where to format the message
   pingReq = (MqttSnPingReq *) message->buffer;

   //Copy client identifier
   osStrcpy(pingReq, clientId);

   //Compute the length of the message
   length = osStrlen(clientId);

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_PINGREQ, length);
}


/**
 * @brief Format PINGRESP message
 * @param[in] message Pointer to the MQTT-SN message
 * @return Error code
 **/

error_t mqttSnFormatPingResp(MqttSnMessage *message)
{
   size_t length;

   //The PINGRESP message has only a header and no variable part
   length = 0;

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_PINGRESP, length);
}


/**
 * @brief Format DISCONNECT message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] duration Value of the sleep timer
 * @return Error code
 **/

error_t mqttSnFormatDisconnect(MqttSnMessage *message,
   uint16_t duration)
{
   size_t length;
   MqttSnDisconnect *disconnect;

   //Point to the buffer where to format the message
   disconnect = (MqttSnDisconnect *) message->buffer;

   //Check the value of the sleep timer
   if(duration != 0)
   {
      //A DISCONNECT message with a Duration field is sent by a client when
      //it wants to go to the "asleep" state
      disconnect->duration = htons(duration);

      //Compute the length of the message
      length = sizeof(MqttSnDisconnect);
   }
   else
   {
      //A DISCONNECT message without a Duration field is sent by a client when
      //it wants to go to the "disconnected" state
      length = 0;
   }

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_DISCONNECT, length);
}


/**
 * @brief Format WILLTOPICUPD message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] flags Flags
 * @param[in] topicName Topic name
 * @return Error code
 **/

error_t mqttSnFormatWillTopicUpd(MqttSnMessage *message, MqttSnFlags flags,
   const char_t *topicName)
{
   size_t length;
   MqttSnWillTopicUpd *willTopicUpd;

   //Point to the buffer where to format the message
   willTopicUpd = (MqttSnWillTopicUpd *) message->buffer;

   //Valid Will topic?
   if(topicName[0] != '\0')
   {
      //Format WILLTOPICUPD message
      willTopicUpd->flags = flags;
      osStrcpy(willTopicUpd->willTopic, topicName);

      //Compute the length of the message
      length = sizeof(MqttSnWillTopicUpd) + osStrlen(topicName);
   }
   else
   {
      //An empty WILLTOPICUPD message (without Flags and WillTopic field) is
      //used by a client to delete the Will topic and the Will message stored
      //in the server
      length = 0;
   }

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_WILLTOPICUPD, length);
}


/**
 * @brief Format WILLMSGUPD message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[in] data Will message
 * @param[in] dataLen Length of the Will message
 * @return Error code
 **/

error_t mqttSnFormatWillMsgUpd(MqttSnMessage *message, const void *data,
   size_t dataLen)
{
   MqttSnWillMsgUpd *willMsgUpd;

   //Point to the buffer where to format the message
   willMsgUpd = (MqttSnWillMsgUpd *) message->buffer;

   //Copy Will message
   osMemcpy(willMsgUpd, data, dataLen);

   //Format MQTT-SN message header
   return mqttSnFormatHeader(message, MQTT_SN_MSG_TYPE_WILLMSGUPD, dataLen);
}


/**
 * @brief Format MQTT-SN message header
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] type Message type
 * @return Error code
 **/

error_t mqttSnParseHeader(MqttSnMessage *message, MqttSnMsgType *type)
{
   //Malformed MQTT-SN message?
   if(message->length == 0)
      return ERROR_INVALID_LENGTH;

   //Check whether the first octet is 0x01
   if(message->buffer[0] == 0x01)
   {
      const MqttSnExtHeader *header;

      //Point to message header
      header = (MqttSnExtHeader *) message->buffer;

      //Malformed message?
      if(message->length < sizeof(MqttSnExtHeader))
         return ERROR_INVALID_LENGTH;
      if(message->length < ntohs(header->length))
         return ERROR_INVALID_LENGTH;

      //The Length field is 3-octet long
      message->length = ntohs(header->length);
      //Point to the variable part of the message
      message->pos = sizeof(MqttSnExtHeader);

      //Retrieve message type
      *type = (MqttSnMsgType) header->msgType;
   }
   else
   {
      const MqttSnHeader *header;

      //Point to message header
      header = (MqttSnHeader *) message->buffer;

      //Malformed message?
      if(message->length < sizeof(MqttSnHeader))
         return ERROR_INVALID_LENGTH;
      if(message->length < header->length)
         return ERROR_INVALID_LENGTH;

      //The Length field is 1-octet long
      message->length = header->length;
      //Point to the variable part of the message
      message->pos = sizeof(MqttSnHeader);

      //Retrieve message type
      *type = (MqttSnMsgType) header->msgType;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse GWINFO message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] gwId Gateway identifier
 * @param[out] gwAdd Gateway address
 * @param[out] gwAddLen Length of the gateway address
 * @return Error code
 **/

error_t mqttSnParseGwInfo(const MqttSnMessage *message, uint8_t *gwId,
   const uint8_t **gwAdd, size_t *gwAddLen)
{
   size_t n;
   const MqttSnGwInfo *gwInfo;

   //Point to the GWINFO message
   gwInfo = (MqttSnGwInfo *) (message->buffer + message->pos);
   //Calculate the length of the message
   n = message->length - message->pos;

   //Check the length of the message
   if(n < sizeof(MqttSnGwInfo))
      return ERROR_INVALID_LENGTH;

   //Get gateway identifier
   *gwId = gwInfo->gwId;
   //Get gateway address
   *gwAdd = gwInfo->gwAdd;

   //Calculate the length of the gateway address
   *gwAddLen = n - sizeof(MqttSnGwInfo);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse CONNACK message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] returnCode Return code
 * @return Error code
 **/

error_t mqttSnParseConnAck(const MqttSnMessage *message,
   MqttSnReturnCode *returnCode)
{
   size_t n;
   const MqttSnConnAck *connAck;

   //Point to the CONNACK message
   connAck = (MqttSnConnAck *) (message->buffer + message->pos);
   //Calculate the length of the message
   n = message->length - message->pos;

   //Check the length of the message
   if(n < sizeof(MqttSnConnAck))
      return ERROR_INVALID_LENGTH;

   //Get return code
   *returnCode = (MqttSnReturnCode) connAck->returnCode;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse WILLTOPICREQ message
 * @param[in] message Pointer to the MQTT-SN message
 * @return Error code
 **/

error_t mqttSnParseWillTopicReq(const MqttSnMessage *message)
{
   //The WILLTOPICREQ message has only a header and no variable part
   return NO_ERROR;
}


/**
 * @brief Parse WILLMSGREQ message
 * @param[in] message Pointer to the MQTT-SN message
 * @return Error code
 **/

error_t mqttSnParseWillMsgReq(const MqttSnMessage *message)
{
   //The WILLMSGREQ message has only a header and no variable part
   return NO_ERROR;
}


/**
 * @brief Parse REGISTER message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] msgId Message identifier
 * @param[out] topicId Topic identifier
 * @param[out] topicName Topic name
 * @return Error code
 **/

error_t mqttSnParseRegister(const MqttSnMessage *message, uint16_t *msgId,
   uint16_t *topicId, const char_t **topicName)
{
   size_t n;
   const MqttSnRegister *reg;

   //Point to the REGISTER message
   reg = (MqttSnRegister *) (message->buffer + message->pos);
   //Calculate the length of the message
   n = message->length - message->pos;

   //Check the length of the message
   if(n < sizeof(MqttSnRegister))
      return ERROR_INVALID_LENGTH;

   //Get message identifier
   *msgId = ntohs(reg->msgId);
   //Get topic identifier
   *topicId = ntohs(reg->topicId);
   //Get topic name
   *topicName = reg->topicName;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse REGACK message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] msgId Message identifier
 * @param[out] topicId Topic identifier
 * @param[out] returnCode Return code
 * @return Error code
 **/

error_t mqttSnParseRegAck(const MqttSnMessage *message, uint16_t *msgId,
   uint16_t *topicId, MqttSnReturnCode *returnCode)
{
   size_t n;
   const MqttSnRegAck *regAck;

   //Point to the REGACK message
   regAck = (MqttSnRegAck *) (message->buffer + message->pos);
   //Calculate the length of the message
   n = message->length - message->pos;

   //Check the length of the message
   if(n < sizeof(MqttSnRegAck))
      return ERROR_INVALID_LENGTH;

   //Get return code
   *msgId = ntohs(regAck->msgId);
   //Get return code
   *topicId = ntohs(regAck->topicId);
   //Get return code
   *returnCode = (MqttSnReturnCode) regAck->returnCode;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse PUBLISH message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] flags Flags
 * @param[out] msgId Message identifier
 * @param[out] topicId Topic identifier
 * @param[out] data Pointer to the published data
 * @param[out] dataLen Length of the published data
 * @return Error code
 **/

error_t mqttSnParsePublish(const MqttSnMessage *message, MqttSnFlags *flags,
   uint16_t *msgId, uint16_t *topicId, const uint8_t **data, size_t *dataLen)
{
   size_t length;
   const MqttSnPublish *publish;

   //Point to the PUBLISH message
   publish = (MqttSnPublish *) (message->buffer + message->pos);
   //Calculate the length of the message
   length = message->length - message->pos;

   //Check the length of the message
   if(length < sizeof(MqttSnPublish))
      return ERROR_INVALID_LENGTH;

   //Get flags
   *flags = publish->flags;
   //Get message identifier
   *msgId = ntohs(publish->msgId);
   //Get topic identifier
   *topicId = ntohs(publish->topicId);
   //Get published data
   *data = publish->data;

   //Calculate the length of the published data
   *dataLen = length - sizeof(MqttSnPublish);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse PUBACK message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] msgId Message identifier
 * @param[out] topicId Topic identifier
 * @param[out] returnCode Return code
 * @return Error code
 **/

error_t mqttSnParsePubAck(const MqttSnMessage *message, uint16_t *msgId,
   uint16_t *topicId, MqttSnReturnCode *returnCode)
{
   size_t length;
   const MqttSnPubAck *pubAck;

   //Point to the PUBACK message
   pubAck = (MqttSnPubAck *) (message->buffer + message->pos);
   //Calculate the length of the message
   length = message->length - message->pos;

   //Check the length of the message
   if(length < sizeof(MqttSnPubAck))
      return ERROR_INVALID_LENGTH;

   //Get message identifier
   *msgId = ntohs(pubAck->msgId);
   //Get topic identifier
   *topicId = ntohs(pubAck->topicId);
   //Get return code
   *returnCode = (MqttSnReturnCode) pubAck->returnCode;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse PUBREC message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] msgId Message identifier
 * @return Error code
 **/

error_t mqttSnParsePubRec(const MqttSnMessage *message, uint16_t *msgId)
{
   size_t length;
   const MqttSnPubRec *pubRec;

   //Point to the PUBREC message
   pubRec = (MqttSnPubRec *) (message->buffer + message->pos);
   //Calculate the length of the message
   length = message->length - message->pos;

   //Check the length of the message
   if(length < sizeof(MqttSnPubRec))
      return ERROR_INVALID_LENGTH;

   //Get message identifier
   *msgId = ntohs(pubRec->msgId);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse PUBREL message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] msgId Message identifier
 * @return Error code
 **/

error_t mqttSnParsePubRel(const MqttSnMessage *message, uint16_t *msgId)
{
   size_t length;
   const MqttSnPubRel *pubRel;

   //Point to the PUBREL message
   pubRel = (MqttSnPubRel *) (message->buffer + message->pos);
   //Calculate the length of the message
   length = message->length - message->pos;

   //Check the length of the message
   if(length < sizeof(MqttSnPubRel))
      return ERROR_INVALID_LENGTH;

   //Get message identifier
   *msgId = ntohs(pubRel->msgId);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse PUBCOMP message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] msgId Message identifier
 * @return Error code
 **/

error_t mqttSnParsePubComp(const MqttSnMessage *message, uint16_t *msgId)
{
   size_t length;
   const MqttSnPubComp *pubComp;

   //Point to the PUBCOMP message
   pubComp = (MqttSnPubComp *) (message->buffer + message->pos);
   //Calculate the length of the message
   length = message->length - message->pos;

   //Check the length of the message
   if(length < sizeof(MqttSnPubComp))
      return ERROR_INVALID_LENGTH;

   //Get message identifier
   *msgId = ntohs(pubComp->msgId);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse SUBACK message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] flags Flags
 * @param[out] msgId Message identifier
 * @param[out] topicId Topic identifier
 * @param[out] returnCode Return code
 * @return Error code
 **/

error_t mqttSnParseSubAck(const MqttSnMessage *message, MqttSnFlags *flags,
   uint16_t *msgId, uint16_t *topicId, MqttSnReturnCode *returnCode)
{
   size_t length;
   const MqttSnSubAck *subAck;

   //Point to the SUBACK message
   subAck = (MqttSnSubAck *) (message->buffer + message->pos);
   //Calculate the length of the message
   length = message->length - message->pos;

   //Check the length of the message
   if(length < sizeof(MqttSnSubAck))
      return ERROR_INVALID_LENGTH;

   //Get flags
   *flags = subAck->flags;
   //Get message identifier
   *msgId = ntohs(subAck->msgId);
   //Get topic identifier
   *topicId = ntohs(subAck->topicId);
   //Get return code
   *returnCode = (MqttSnReturnCode) subAck->returnCode;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse UNSUBACK message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] msgId Message identifier
 * @return Error code
 **/

error_t mqttSnParseUnsubAck(const MqttSnMessage *message, uint16_t *msgId)
{
   size_t length;
   const MqttSnUnsubAck *unsubAck;

   //Point to the UNSUBACK message
   unsubAck = (MqttSnUnsubAck *) (message->buffer + message->pos);
   //Calculate the length of the message
   length = message->length - message->pos;

   //Check the length of the message
   if(length < sizeof(MqttSnUnsubAck))
      return ERROR_INVALID_LENGTH;

   //Get message identifier
   *msgId = ntohs(unsubAck->msgId);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse PINGREQ message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] clientId Client identifier
 * @return Error code
 **/

error_t mqttSnParsePingReq(const MqttSnMessage *message,
   const char_t **clientId)
{
   const MqttSnPingReq *pingReq;

   //Point to the PINGREQ message
   pingReq = (MqttSnPingReq *) (message->buffer + message->pos);

   //Get client identifier
   *clientId = pingReq;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse PINGRESP message
 * @param[in] message Pointer to the MQTT-SN message
 * @return Error code
 **/

error_t mqttSnParsePingResp(const MqttSnMessage *message)
{
   //The PINGRESP message has only a header and no variable part
   return NO_ERROR;
}


/**
 * @brief Parse DISCONNECT message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] duration Value of the sleep timer
 * @return Error code
 **/

error_t mqttSnParseDisconnect(const MqttSnMessage *message,
   uint16_t *duration)
{
   size_t length;
   const MqttSnDisconnect *disconnect;

   //Point to the DISCONNECT message
   disconnect = (MqttSnDisconnect *) (message->buffer + message->pos);
   //Calculate the length of the message
   length = message->length - message->pos;

   //The Duration parameter is optional
   if(length == 0)
   {
      //The field is not present
      *duration = 0;
   }
   else
   {
      //Check the length of the message
      if(length < sizeof(MqttSnDisconnect))
         return ERROR_INVALID_LENGTH;

      //Get topic identifier
      *duration = ntohs(disconnect->duration);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse WILLTOPICRESP message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] returnCode Return code
 * @return Error code
 **/

error_t mqttSnParseWillTopicResp(const MqttSnMessage *message,
   MqttSnReturnCode *returnCode)
{
   size_t length;
   const MqttSnWillTopicResp *willTopicResp;

   //Point to the WILLTOPICRESP message
   willTopicResp = (MqttSnWillTopicResp *) (message->buffer + message->pos);
   //Calculate the length of the message
   length = message->length - message->pos;

   //Check the length of the message
   if(length < sizeof(MqttSnWillTopicResp))
      return ERROR_INVALID_LENGTH;

   //Get return code
   *returnCode = (MqttSnReturnCode) willTopicResp->returnCode;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse WILLMSGRESP message
 * @param[in] message Pointer to the MQTT-SN message
 * @param[out] returnCode Return code
 * @return Error code
 **/

error_t mqttSnParseWillMsgResp(const MqttSnMessage *message,
   MqttSnReturnCode *returnCode)
{
   size_t length;
   const MqttSnWillMsgResp *willMsgResp;

   //Point to the WILLMSGRESP message
   willMsgResp = (MqttSnWillMsgResp *) (message->buffer + message->pos);
   //Calculate the length of the message
   length = message->length - message->pos;

   //Check the length of the message
   if(length < sizeof(MqttSnWillMsgResp))
      return ERROR_INVALID_LENGTH;

   //Get return code
   *returnCode = (MqttSnReturnCode) willMsgResp->returnCode;

   //Successful processing
   return NO_ERROR;
}

#endif
