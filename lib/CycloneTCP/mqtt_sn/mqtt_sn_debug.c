/**
 * @file mqtt_sn_debug.c
 * @brief Data logging functions for debugging purpose (MQTT-SN)
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
#include "mqtt_sn/mqtt_sn_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MQTT_SN_CLIENT_SUPPORT == ENABLED)

//MQTT-SN message types
const char_t *mqttSnMsgTypeLabel[] =
{
   "ADVERTISE",     //0x00
   "SEARCHGW",      //0x01
   "GWINFO",        //0x02
   "Reserved",      //0x03
   "CONNECT",       //0x04
   "CONNACK",       //0x05
   "WILLTOPICREQ",  //0x06
   "WILLTOPIC",     //0x07
   "WILLMSGREQ",    //0x08
   "WILLMSG",       //0x09
   "REGISTER",      //0x0A
   "REGACK",        //0x0B
   "PUBLISH",       //0x0C
   "PUBACK",        //0x0D
   "PUBCOMP",       //0x0E
   "PUBREC",        //0x0F
   "PUBREL",        //0x10
   "Reserved",      //0x11
   "SUBSCRIBE",     //0x12
   "SUBACK",        //0x13
   "UNSUBSCRIBE",   //0x14
   "UNSUBACK",      //0x15
   "PINGREQ",       //0x16
   "PINGRESP",      //0x17
   "DISCONNECT",    //0x18
   "Reserved",      //0x19
   "WILLTOPICUPD",  //0x1A
   "WILLTOPICRESP", //0x1B
   "WILLMSGUPD",    //0x1C
   "WILLMSGRESP"    //0x1D
};

//MQTT-SN return codes
const char_t *mqttSnReturnCodeLabel[] =
{
   "Accepted",                   //0x00
   "Rejected: congestion",       //0x01
   "Rejected: invalid topic ID", //0x02
   "Rejected: not supported",    //0x03
};


/**
 * @brief Dump MQTT-SN message for debugging purpose
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpMessage(const uint8_t *message, size_t length)
{
#if (MQTT_SN_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   error_t error;
   uint8_t type;

   //Sanity check
   if(length == 0)
      return ERROR_INVALID_LENGTH;

   //Check whether the first octet is 0x01
   if(message[0] == 0x01)
   {
      const MqttSnExtHeader *header;

      //Point to message header
      header = (MqttSnExtHeader *) message;

      //Malformed message?
      if(length < sizeof(MqttSnExtHeader))
         return ERROR_INVALID_LENGTH;
      if(length < ntohs(header->length))
         return ERROR_INVALID_LENGTH;

      //Dump message length
      TRACE_DEBUG("  Length = %" PRIuSIZE "\r\n", ntohs(header->length));

      //The Length field specifies the total number of octets contained in
      //the message, including the Length field itself
      length = ntohs(header->length) - sizeof(MqttSnExtHeader);

      //Retrieve message type
      type = header->msgType;
      //Point to the payload
      message = header->data;
   }
   else
   {
      const MqttSnHeader *header;

      //Point to message header
      header = (MqttSnHeader *) message;

      //Malformed message?
      if(length < sizeof(MqttSnHeader))
         return ERROR_INVALID_LENGTH;
      if(length < header->length)
         return ERROR_INVALID_LENGTH;

      //Dump message length
      TRACE_DEBUG("  Length = %" PRIuSIZE "\r\n", header->length);

      //The Length field specifies the total number of octets contained in
      //the message, including the Length field itself
      length = header->length - sizeof(MqttSnHeader);

      //Retrieve message type
      type = header->msgType;
      //Point to the payload
      message = header->data;
   }

   //Dump message type
   TRACE_DEBUG("  MsgType = 0x%02" PRIX8 " (%s)\r\n", type,
      mqttSnGetMessageName(type));

   //Check message type
   switch(type)
   {
   //ADVERTISE message?
   case MQTT_SN_MSG_TYPE_ADVERTISE:
      //Dump ADVERTISE message
      error = mqttSnDumpAdvertise((MqttSnAdvertise *) message, length);
      break;
   //SEARCHGW message?
   case MQTT_SN_MSG_TYPE_SEARCHGW:
      //Dump SEARCHGW message
      error = mqttSnDumpSearchGw((MqttSnSearchGw *) message, length);
      break;
   //GWINFO message?
   case MQTT_SN_MSG_TYPE_GWINFO:
      //Dump GWINFO message
      error = mqttSnDumpGwInfo((MqttSnGwInfo *) message, length);
      break;
   //CONNECT message?
   case MQTT_SN_MSG_TYPE_CONNECT:
      //Dump CONNECT message
      error = mqttSnDumpConnect((MqttSnConnect *) message, length);
      break;
   //CONNACK message?
   case MQTT_SN_MSG_TYPE_CONNACK:
      //Dump CONNACK message
      error = mqttSnDumpConnAck((MqttSnConnAck *) message, length);
      break;
   //WILLTOPICREQ message?
   case MQTT_SN_MSG_TYPE_WILLTOPICREQ:
      //Dump WILLTOPICREQ message
      error = mqttSnDumpWillTopicReq((MqttSnWillTopicReq *) message, length);
      break;
   //WILLTOPIC message?
   case MQTT_SN_MSG_TYPE_WILLTOPIC:
      //Dump WILLTOPIC message
      error = mqttSnDumpWillTopic((MqttSnWillTopic *) message, length);
      break;
   //WILLMSGREQ message?
   case MQTT_SN_MSG_TYPE_WILLMSGREQ:
      //Dump WILLMSGREQ message
      error = mqttSnDumpWillMsgReq((MqttSnWillMsgReq *) message, length);
      break;
   //WILLMSG message?
   case MQTT_SN_MSG_TYPE_WILLMSG:
      //Dump WILLMSG message
      error = mqttSnDumpWillMsg((MqttSnWillMsg *) message, length);
      break;
   //REGISTER message?
   case MQTT_SN_MSG_TYPE_REGISTER:
      //Dump REGISTER message
      error = mqttSnDumpRegister((MqttSnRegister *) message, length);
      break;
   //REGACK message?
   case MQTT_SN_MSG_TYPE_REGACK:
      //Dump REGACK message
      error = mqttSnDumpRegAck((MqttSnRegAck *) message, length);
      break;
   //PUBLISH message?
   case MQTT_SN_MSG_TYPE_PUBLISH:
      //Dump PUBLISH message
      error = mqttSnDumpPublish((MqttSnPublish *) message, length);
      break;
   //PUBACK message?
   case MQTT_SN_MSG_TYPE_PUBACK:
      //Dump PUBACK message
      error = mqttSnDumpPubAck((MqttSnPubAck *) message, length);
      break;
   //PUBREC message?
   case MQTT_SN_MSG_TYPE_PUBREC:
      //Dump PUBREC message
      error = mqttSnDumpPubRec((MqttSnPubRec *) message, length);
      break;
   //PUBREL message?
   case MQTT_SN_MSG_TYPE_PUBREL:
      //Dump PUBREL message
      error = mqttSnDumpPubRel((MqttSnPubRel *) message, length);
      break;
   //PUBCOMP message?
   case MQTT_SN_MSG_TYPE_PUBCOMP:
      //Dump PUBCOMP message
      error = mqttSnDumpPubComp((MqttSnPubComp *) message, length);
      break;
   //SUBSCRIBE message?
   case MQTT_SN_MSG_TYPE_SUBSCRIBE:
      //Dump SUBSCRIBE message
      error = mqttSnDumpSubscribe((MqttSnSubscribe *) message, length);
      break;
   //SUBACK message?
   case MQTT_SN_MSG_TYPE_SUBACK:
      //Dump SUBACK message
      error = mqttSnDumpSubAck((MqttSnSubAck *) message, length);
      break;
   //UNSUBSCRIBE message?
   case MQTT_SN_MSG_TYPE_UNSUBSCRIBE:
      //Dump UNSUBSCRIBE message
      error = mqttSnDumpUnsubscribe((MqttSnUnsubscribe *) message, length);
      break;
   //UNSUBACK message?
   case MQTT_SN_MSG_TYPE_UNSUBACK:
      //Dump UNSUBACK message
      error = mqttSnDumpUnsubAck((MqttSnUnsubAck *) message, length);
      break;
   //PINGREQ message?
   case MQTT_SN_MSG_TYPE_PINGREQ:
      //Dump PINGREQ message
      error = mqttSnDumpPingReq((MqttSnPingReq *) message, length);
      break;
   //PINGRESP message?
   case MQTT_SN_MSG_TYPE_PINGRESP:
      //Dump PINGRESP message
      error = mqttSnDumpPingResp((MqttSnPingResp *) message, length);
      break;
   //DISCONNECT message?
   case MQTT_SN_MSG_TYPE_DISCONNECT:
      //Dump DISCONNECT message
      error = mqttSnDumpDisconnect((MqttSnDisconnect *) message, length);
      break;
   //WILLTOPICUPD message?
   case MQTT_SN_MSG_TYPE_WILLTOPICUPD:
      //Dump WILLTOPICUPD message
      error = mqttSnDumpWillTopicUpd((MqttSnWillTopicUpd *) message, length);
      break;
   //WILLTOPICRESP message?
   case MQTT_SN_MSG_TYPE_WILLTOPICRESP:
      //Dump WILLTOPICRESP message
      error = mqttSnDumpWillTopicResp((MqttSnWillTopicResp *) message, length);
      break;
   //WILLMSGUPD message?
   case MQTT_SN_MSG_TYPE_WILLMSGUPD:
      //Dump WILLMSGUPD message
      error = mqttSnDumpWillMsgUpd((MqttSnWillMsgUpd *) message, length);
      break;
   //WILLMSGRESP message?
   case MQTT_SN_MSG_TYPE_WILLMSGRESP:
      //Dump DISCONNECT message
      error = mqttSnDumpWillMsgResp((MqttSnWillMsgResp *) message, length);
      break;
   //Unknown message?
   default:
      //Report an error
      error = ERROR_INVALID_TYPE;
      break;
   }

   //Return error code
   return error;
#else
   //Not implemented
   return NO_ERROR;
#endif
}


/**
 * @brief Dump ADVERTISE message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpAdvertise(const MqttSnAdvertise *message,
   size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnAdvertise))
      return ERROR_INVALID_LENGTH;

   //Dump ADVERTISE message
   TRACE_DEBUG("  GwId = %" PRIu8 "\r\n", message->gwId);
   TRACE_DEBUG("  Duration = %" PRIu16 "\r\n", message->duration);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump SEARCHGW message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpSearchGw(const MqttSnSearchGw *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnSearchGw))
      return ERROR_INVALID_LENGTH;

   //Dump SEARCHGW message
   TRACE_DEBUG("  Radius = %" PRIu8 "\r\n", message->radius);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump GWINFO message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpGwInfo(const MqttSnGwInfo *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnGwInfo))
      return ERROR_INVALID_LENGTH;

   //Retrieve the length of the GwAdd field
   length -= sizeof(MqttSnGwInfo);

   //Dump GWINFO message
   TRACE_DEBUG("  GwId = %" PRIu8 "\r\n", message->gwId);
   TRACE_DEBUG_ARRAY("  GwAdd = ", message->gwAdd, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump CONNECT message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpConnect(const MqttSnConnect *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnConnect))
      return ERROR_INVALID_LENGTH;

   //Dump CONNECT message
   mqttSnDumpFlags(message->flags);
   TRACE_DEBUG("  ProtocolId = %" PRIu8 "\r\n", message->protocolId);
   TRACE_DEBUG("  Duration = %" PRIu16 "\r\n", ntohs(message->duration));
   TRACE_DEBUG("  ClientId = %s\r\n", message->clientId);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump CONNACK message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpConnAck(const MqttSnConnAck *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnConnAck))
      return ERROR_INVALID_LENGTH;

   //Dump CONNACK message
   TRACE_DEBUG("  ReturnCode = %" PRIu8 " (%s)\r\n", message->returnCode,
      mqttSnGetReturnCodeDesc(message->returnCode));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump WILLTOPICREQ message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpWillTopicReq(const MqttSnWillTopicReq *message, size_t length)
{
   //The WILLTOPICREQ message has only a header and no variable part
   return NO_ERROR;
}


/**
 * @brief Dump WILLTOPIC message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpWillTopic(const MqttSnWillTopic *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnWillTopic))
      return ERROR_INVALID_LENGTH;

   //Dump WILLTOPIC message
   mqttSnDumpFlags(message->flags);
   TRACE_DEBUG("  WillTopic = %s\r\n", message->willTopic);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump WILLMSGREQ message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpWillMsgReq(const MqttSnWillMsgReq *message, size_t length)
{
   //The WILLMSGREQ message has only a header and no variable part
   return NO_ERROR;
}


/**
 * @brief Dump WILLMSG message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpWillMsg(const MqttSnWillMsg *message, size_t length)
{
   //Dump WILLMSG message
   TRACE_DEBUG("  WillMsg (%" PRIuSIZE " bytes)\r\n", length);
   TRACE_DEBUG_ARRAY("    ", message, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump REGISTER message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpRegister(const MqttSnRegister *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnRegister))
      return ERROR_INVALID_LENGTH;

   //Dump REGISTER message
   TRACE_DEBUG("  TopicId = 0x%04" PRIX16 "\r\n", ntohs(message->topicId));
   TRACE_DEBUG("  MsgId = 0x%04" PRIX16 "\r\n", ntohs(message->msgId));
   TRACE_DEBUG("  TopicName = %s\r\n", message->topicName);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump REGACK message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpRegAck(const MqttSnRegAck *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnRegAck))
      return ERROR_INVALID_LENGTH;

   //Dump REGACK message
   TRACE_DEBUG("  TopicId = 0x%04" PRIX16 "\r\n", ntohs(message->topicId));
   TRACE_DEBUG("  MsgId = 0x%04" PRIX16 "\r\n", ntohs(message->msgId));
   TRACE_DEBUG("  ReturnCode = %" PRIu8 " (%s)\r\n", message->returnCode,
      mqttSnGetReturnCodeDesc(message->returnCode));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump PUBLISH message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpPublish(const MqttSnPublish *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnPublish))
      return ERROR_INVALID_LENGTH;

   //Retrieve the length of the published data
   length -= sizeof(MqttSnPublish);

   //Dump flags
   mqttSnDumpFlags(message->flags);

   //Check the type of topic identifier
   if(message->flags.topicIdType == MQTT_SN_NORMAL_TOPIC_ID)
   {
      //Dump normal topic ID
      TRACE_DEBUG("  TopicId = 0x%04" PRIX16 "\r\n",
         ntohs(message->topicId));
   }
   else if(message->flags.topicIdType == MQTT_SN_PREDEFINED_TOPIC_ID)
   {
      //Dump predefined topic ID
      TRACE_DEBUG("  PredefinedTopicId = 0x%04" PRIX16 "\r\n",
         ntohs(message->topicId));
   }
   else if(message->flags.topicIdType == MQTT_SN_SHORT_TOPIC_NAME)
   {
      //Dump short topic name
      TRACE_DEBUG("  ShortTopicName = %c%c\r\n",
         MSB(ntohs(message->topicId)), LSB(ntohs(message->topicId)));
   }
   else
   {
      //Just for sanity
   }

   //Debug message
   TRACE_DEBUG("  MsgId = 0x%04" PRIX16 "\r\n", ntohs(message->msgId));
   TRACE_DEBUG("  Data (%" PRIuSIZE " bytes)\r\n", length);
   TRACE_DEBUG_ARRAY("    ", message->data, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump PUBACK message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpPubAck(const MqttSnPubAck *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnPubAck))
      return ERROR_INVALID_LENGTH;

   //Dump PUBACK message
   TRACE_DEBUG("  TopicId = 0x%04" PRIX16 "\r\n", ntohs(message->topicId));
   TRACE_DEBUG("  MsgId = 0x%04" PRIX16 "\r\n", ntohs(message->msgId));
   TRACE_DEBUG("  ReturnCode = %" PRIu8 " (%s)\r\n", message->returnCode,
      mqttSnGetReturnCodeDesc(message->returnCode));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump PUBREC message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpPubRec(const MqttSnPubRec *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnPubRec))
      return ERROR_INVALID_LENGTH;

   //Dump PUBREC message
   TRACE_DEBUG("  MsgId = 0x%04" PRIX16 "\r\n", ntohs(message->msgId));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump PUBREL message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpPubRel(const MqttSnPubRel *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnPubRel))
      return ERROR_INVALID_LENGTH;

   //Dump PUBREL message
   TRACE_DEBUG("  MsgId = 0x%04" PRIX16 "\r\n", ntohs(message->msgId));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump PUBCOMP message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpPubComp(const MqttSnPubComp *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnPubComp))
      return ERROR_INVALID_LENGTH;

   //Dump PUBCOMP message
   TRACE_DEBUG("  MsgId = 0x%04" PRIX16 "\r\n", ntohs(message->msgId));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump SUBSCRIBE message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpSubscribe(const MqttSnSubscribe *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnSubscribe))
      return ERROR_INVALID_LENGTH;

   //Dump SUBSCRIBE message
   mqttSnDumpFlags(message->flags);
   TRACE_DEBUG("  MsgId = 0x%04" PRIX16 "\r\n", ntohs(message->msgId));

   //Check the type of topic identifier
   if(message->flags.topicIdType == MQTT_SN_NORMAL_TOPIC_NAME)
   {
      //Dump topic name
      TRACE_DEBUG("  TopicName = %s\r\n", message->topicName);
   }
   else if(message->flags.topicIdType == MQTT_SN_SHORT_TOPIC_NAME)
   {
      //Dump short topic name
      TRACE_DEBUG("  ShortTopicName = %s\r\n", message->topicName);
   }
   else if(message->flags.topicIdType == MQTT_SN_PREDEFINED_TOPIC_ID)
   {
      //Dump predefined topic ID
      TRACE_DEBUG("  PredefinedTopicId = 0x%04" PRIX16 "\r\n",
         LOAD16BE(message->topicName));
   }
   else
   {
      //Just for sanity
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump SUBACK message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpSubAck(const MqttSnSubAck *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnSubAck))
      return ERROR_INVALID_LENGTH;

   //Dump SUBACK message
   mqttSnDumpFlags(message->flags);
   TRACE_DEBUG("  TopicId = 0x%04" PRIX16 "\r\n", ntohs(message->topicId));
   TRACE_DEBUG("  MsgId = 0x%04" PRIX16 "\r\n", ntohs(message->msgId));
   TRACE_DEBUG("  ReturnCode = %" PRIu8 " (%s)\r\n", message->returnCode,
      mqttSnGetReturnCodeDesc(message->returnCode));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump UNSUBSCRIBE message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpUnsubscribe(const MqttSnUnsubscribe *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnUnsubscribe))
      return ERROR_INVALID_LENGTH;

   //Dump UNSUBSCRIBE message
   mqttSnDumpFlags(message->flags);
   TRACE_DEBUG("  MsgId = 0x%04" PRIX16 "\r\n", ntohs(message->msgId));

   //Check the type of topic identifier
   if(message->flags.topicIdType == MQTT_SN_NORMAL_TOPIC_NAME)
   {
      //Dump topic name
      TRACE_DEBUG("  TopicName = %s\r\n", message->topicName);
   }
   else if(message->flags.topicIdType == MQTT_SN_SHORT_TOPIC_NAME)
   {
      //Dump short topic name
      TRACE_DEBUG("  ShortTopicName = %s\r\n", message->topicName);
   }
   else if(message->flags.topicIdType == MQTT_SN_PREDEFINED_TOPIC_ID)
   {
      //Dump predefined topic ID
      TRACE_DEBUG("  PredefinedTopicId = 0x%04" PRIX16 "\r\n",
         LOAD16BE(message->topicName));
   }
   else
   {
      //Just for sanity
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump UNSUBACK message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpUnsubAck(const MqttSnUnsubAck *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnUnsubAck))
      return ERROR_INVALID_LENGTH;

   //Dump UNSUBACK message
   TRACE_DEBUG("  MsgId = 0x%04" PRIX16 "\r\n", ntohs(message->msgId));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump PINGREQ message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpPingReq(const MqttSnPingReq *message, size_t length)
{
   //Dump PINGREQ message
   TRACE_DEBUG("  ClientId = %s\r\n", (char_t *) message);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump PINGRESP message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpPingResp(const MqttSnPingResp *message, size_t length)
{
   //The PINGRESP message has only a header and no variable part
   return NO_ERROR;
}


/**
 * @brief Dump DISCONNECT message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpDisconnect(const MqttSnDisconnect *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnDisconnect))
      return ERROR_INVALID_LENGTH;

   //Dump DISCONNECT message
   TRACE_DEBUG("  Duration = %" PRIu16 "\r\n", ntohs(message->duration));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump WILLTOPICUPD message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpWillTopicUpd(const MqttSnWillTopicUpd *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnWillTopicUpd))
      return ERROR_INVALID_LENGTH;

   //Dump WILLTOPICUPD message
   mqttSnDumpFlags(message->flags);
   TRACE_DEBUG("  WillTopic = %s\r\n", message->willTopic);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump WILLTOPICRESP message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpWillTopicResp(const MqttSnWillTopicResp *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnWillTopicResp))
      return ERROR_INVALID_LENGTH;

   //Dump WILLTOPICRESP message
   TRACE_DEBUG("  ReturnCode = %" PRIu8 " (%s)\r\n", message->returnCode,
      mqttSnGetReturnCodeDesc(message->returnCode));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump WILLMSGUPD message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpWillMsgUpd(const MqttSnWillMsgUpd *message, size_t length)
{
   //Dump WILLMSGUPD message
   TRACE_DEBUG("  WillMsg (%" PRIuSIZE " bytes)\r\n", length);
   TRACE_DEBUG_ARRAY("    ", message, length);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump WILLMSGRESP message
 * @param[in] message Pointer to the message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t mqttSnDumpWillMsgResp(const MqttSnWillMsgResp *message, size_t length)
{
   //Malformed message?
   if(length < sizeof(MqttSnWillMsgResp))
      return ERROR_INVALID_LENGTH;

   //Dump WILLMSGRESP message
   TRACE_DEBUG("  ReturnCode = %" PRIu8 " (%s)\r\n", message->returnCode,
      mqttSnGetReturnCodeDesc(message->returnCode));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump flags
 * @param[in] flags Value of the flags
 **/

void mqttSnDumpFlags(MqttSnFlags flags)
{
   //Check whether any flag is set
   if(flags.all != 0)
   {
      //Dump the value of the Flags field
      TRACE_DEBUG("  Flags = 0x%02" PRIX8 " (", flags.all);

      //Dump flags
      while(1)
      {
         if(flags.dup)
         {
            TRACE_DEBUG("DUP");
            flags.dup = FALSE;
         }
         else if(flags.qos == MQTT_SN_QOS_LEVEL_1)
         {
            TRACE_DEBUG("QoS Level 1");
            flags.qos = 0;
         }
         else if(flags.qos == MQTT_SN_QOS_LEVEL_2)
         {
            TRACE_DEBUG("QoS Level 2");
            flags.qos = 0;
         }
         else if(flags.qos == MQTT_SN_QOS_LEVEL_MINUS_1)
         {
            TRACE_DEBUG("QoS Level -1");
            flags.qos = 0;
         }
         else if(flags.retain)
         {
            TRACE_DEBUG("Retain");
            flags.retain = FALSE;
         }
         else if(flags.will)
         {
            TRACE_DEBUG("Will");
            flags.will = FALSE;
         }
         else if(flags.cleanSession)
         {
            TRACE_DEBUG("Clean Session");
            flags.cleanSession = FALSE;
         }
         else if(flags.topicIdType == MQTT_SN_PREDEFINED_TOPIC_ID)
         {
            TRACE_DEBUG("Predefined Topic ID");
            flags.topicIdType = 0;
         }
         else if(flags.topicIdType == MQTT_SN_SHORT_TOPIC_NAME)
         {
            TRACE_DEBUG("Short Topic Name");
            flags.topicIdType = 0;
         }
         else
         {
         }

         if(flags.all != 0)
         {
            TRACE_DEBUG(", ");
         }
         else
         {
            TRACE_DEBUG(")\r\n");
            break;
         }
      }
   }
   else
   {
      //Dump the value of the Flags field
      TRACE_DEBUG("  Flags = 0x%02" PRIX8 "\r\n", flags.all);
   }
}


/**
 * @brief Get the name of the specified MQTT-SN message
 * @param[in] msgType Message type
 * @return Message name (NULL-terminated string)
 **/

const char_t *mqttSnGetMessageName(uint16_t msgType)
{
   //Default description
   static const char_t defaultLabel[] = "Unknown";

   //Get the name associated with the message type
   if(msgType < arraysize(mqttSnMsgTypeLabel))
      return mqttSnMsgTypeLabel[msgType];
   else
      return defaultLabel;
}


/**
 * @brief Get the description of the specified return code
 * @param[in] returnCode Value of the return code
 * @return Description of the return code (NULL-terminated string)
 **/

const char_t *mqttSnGetReturnCodeDesc(uint16_t returnCode)
{
   //Default description
   static const char_t defaultLabel[] = "Unknown";

   //Get the description associated with the return code
   if(returnCode < arraysize(mqttSnReturnCodeLabel))
      return mqttSnReturnCodeLabel[returnCode];
   else
      return defaultLabel;
}

#endif
