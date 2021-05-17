/**
 * @file mqtt_sn_client_message.c
 * @brief MQTT-SN message parsing and formatting
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
#include "mqtt_sn/mqtt_sn_message.h"
#include "mqtt_sn/mqtt_sn_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MQTT_SN_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Process incoming MQTT-SN message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @param[in] ipAddr Source IP address
 * @param[in] port Source port
 * @return Error code
 **/

error_t mqttSnClientProcessMessage(MqttSnClientContext *context,
   MqttSnMessage *message, const IpAddr *ipAddr, uint16_t port)
{
   error_t error;
   MqttSnMsgType type;

   //Parse MQTT-SN message header
   error = mqttSnParseHeader(message, &type);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("%s message received (%" PRIuSIZE " bytes)...\r\n",
         mqttSnGetMessageName(type), message->length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(message->buffer, message->length);

      //Check message type
      switch(type)
      {
      //GWINFO message received?
      case MQTT_SN_MSG_TYPE_GWINFO:
         //Process incoming GWINFO message
         error = mqttSnClientProcessGwInfo(context, message, ipAddr, port);
         break;
      //CONNACK message received?
      case MQTT_SN_MSG_TYPE_CONNACK:
         //Process incoming CONNACK message
         error = mqttSnClientProcessConnAck(context, message);
         break;
      //WILLTOPICREQ message received?
      case MQTT_SN_MSG_TYPE_WILLTOPICREQ:
         //Process incoming WILLTOPICREQ message
         error = mqttSnClientProcessWillTopicReq(context, message);
         break;
      //WILLMSGREQ message received?
      case MQTT_SN_MSG_TYPE_WILLMSGREQ:
         //Process incoming WILLMSGREQ message
         error = mqttSnClientProcessWillMsgReq(context, message);
         break;
      //REGISTER message received?
      case MQTT_SN_MSG_TYPE_REGISTER:
         //Process incoming REGISTER message
         error = mqttSnClientProcessRegister(context, message);
         break;
      //REGACK message received?
      case MQTT_SN_MSG_TYPE_REGACK:
         //Process incoming REGACK message
         error = mqttSnClientProcessRegAck(context, message);
         break;
      //PUBLISH message received?
      case MQTT_SN_MSG_TYPE_PUBLISH:
         //Process incoming PUBLISH message
         error = mqttSnClientProcessPublish(context, message);
         break;
      //PUBACK message received?
      case MQTT_SN_MSG_TYPE_PUBACK:
         //Process incoming PUBACK message
         error = mqttSnClientProcessPubAck(context, message);
         break;
      //PUBREC message received?
      case MQTT_SN_MSG_TYPE_PUBREC:
         //Process incoming PUBREC message
         error = mqttSnClientProcessPubRec(context, message);
         break;
      //PUBREL message received?
      case MQTT_SN_MSG_TYPE_PUBREL:
         //Process incoming PUBREL message
         error = mqttSnClientProcessPubRel(context, message);
         break;
      //PUBCOMP message received?
      case MQTT_SN_MSG_TYPE_PUBCOMP:
         //Process incoming PUBCOMP message
         error = mqttSnClientProcessPubComp(context, message);
         break;
      //SUBACK message received?
      case MQTT_SN_MSG_TYPE_SUBACK:
         //Process incoming SUBACK message
         error = mqttSnClientProcessSubAck(context, message);
         break;
      //UNSUBACK message received?
      case MQTT_SN_MSG_TYPE_UNSUBACK:
         //Process incoming UNSUBACK message
         error = mqttSnClientProcessUnsubAck(context, message);
         break;
      //PINGREQ message received?
      case MQTT_SN_MSG_TYPE_PINGREQ:
         //Process incoming PINGREQ message
         error = mqttSnClientProcessPingReq(context, message);
         break;
      //PINGRESP message received?
      case MQTT_SN_MSG_TYPE_PINGRESP:
         //Process incoming PINGRESP message
         error = mqttSnClientProcessPingResp(context, message);
         break;
      //DISCONNECT message received?
      case MQTT_SN_MSG_TYPE_DISCONNECT:
         //Process incoming DISCONNECT message
         error = mqttSnClientProcessDisconnect(context, message);
         break;
      //WILLTOPICRESP message received?
      case MQTT_SN_MSG_TYPE_WILLTOPICRESP:
         //Process incoming DISCONNECT message
         error = mqttSnClientProcessWillTopicResp(context, message);
         break;
      //WILLMSGRESP message received?
      case MQTT_SN_MSG_TYPE_WILLMSGRESP:
         //Process incoming DISCONNECT message
         error = mqttSnClientProcessWillMsgResp(context, message);
         break;
      //Unknown message received?
      default:
         //Report an error
         error = ERROR_INVALID_TYPE;
         break;
      }
   }
   else
   {
      //Debug message
      TRACE_WARNING("Invalid message received (%" PRIuSIZE " bytes)...\r\n",
         message->length);

      //Dump the contents of the message
      TRACE_DEBUG_ARRAY("  ", message->buffer, message->length);
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming GWINFO message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @param[in] ipAddr Source IP address
 * @param[in] port Source port
 * @return Error code
 **/

error_t mqttSnClientProcessGwInfo(MqttSnClientContext *context,
   const MqttSnMessage *message, const IpAddr *ipAddr, uint16_t port)
{
   error_t error;
   uint8_t gwId;
   const uint8_t *gwAdd;
   size_t gwAddLen;

   //Parse GWINFO message
   error = mqttSnParseGwInfo(message, &gwId, &gwAdd, &gwAddLen);

   //Valid message received?
   if(!error)
   {
      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_SEARCHING)
      {
         //Notify the application that a GWINFO message has been received
         context->state = MQTT_SN_CLIENT_STATE_RESP_RECEIVED;
         context->msgType = MQTT_SN_MSG_TYPE_GWINFO;
         context->gwIpAddr = *ipAddr;
         context->gwPort = port;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming CONNACK message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessConnAck(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   MqttSnReturnCode returnCode;

   //Parse CONNACK message
   error = mqttSnParseConnAck(message, &returnCode);

   //Valid message received?
   if(!error)
   {
      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ &&
         context->msgType == MQTT_SN_MSG_TYPE_CONNECT)
      {
         //Notify the application that a CONNACK message has been received
         context->state = MQTT_SN_CLIENT_STATE_RESP_RECEIVED;
         context->msgType = MQTT_SN_MSG_TYPE_CONNACK;
         context->returnCode = returnCode;

         //The MQTT-SN gateway is alive
         context->keepAliveCounter = 0;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming WILLTOPICREQ message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessWillTopicReq(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;

   //Parse WILLTOPICREQ message
   error = mqttSnParseWillTopicReq(message);

   //Valid message received?
   if(!error)
   {
      //The WILLTOPIC message is sent by a client as response to the WILLTOPICREQ
      //message for transferring its Will topic name to the GW
      error = mqttSnClientSendWillTopic(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming WILLMSGREQ message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessWillMsgReq(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;

   //Parse WILLMSGREQ message
   error = mqttSnParseWillMsgReq(message);

   //Valid message received?
   if(!error)
   {
      //The WILLMSG message is sent by a client as response to a WILLMSGREQ
      //for transferring its Will message to the GW
      error = mqttSnClientSendWillMsg(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming REGISTER message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessRegister(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   uint16_t msgId;
   uint16_t topicId;
   const char_t *topicName;
   MqttSnReturnCode returnCode;

   //Parse REGISTER message
   error = mqttSnParseRegister(message, &msgId, &topicId, &topicName);

   //Valid message received?
   if(!error)
   {
      //Save the topic ID assigned by the gateway
      error = mqttSnClientAddTopic(context, topicName, topicId);

      //Check status code
      if(!error)
         returnCode = MQTT_SN_RETURN_CODE_ACCEPTED;
      else
         returnCode = MQTT_SN_RETURN_CODE_REJECTED_CONGESTION;

      //The client sends a REGACK message to acknowledge the receipt and
      //processing of a REGISTER message
      error = mqttSnClientSendRegAck(context, msgId, topicId, returnCode);
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming REGACK message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessRegAck(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   uint16_t msgId;
   uint16_t topicId;
   MqttSnReturnCode returnCode;

   //Parse REGACK message
   error = mqttSnParseRegAck(message, &msgId, &topicId, &returnCode);

   //Valid message received?
   if(!error)
   {
      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ &&
         context->msgType == MQTT_SN_MSG_TYPE_REGISTER &&
         context->msgId == msgId)
      {
         //Notify the application that a REGACK message has been received
         context->state = MQTT_SN_CLIENT_STATE_RESP_RECEIVED;
         context->msgType = MQTT_SN_MSG_TYPE_REGACK;
         context->topicId = topicId;
         context->returnCode = returnCode;

         //The MQTT-SN gateway is alive
         context->keepAliveCounter = 0;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming PUBLISH message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessPublish(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   bool_t duplicate;
   uint16_t msgId;
   uint16_t topicId;
   const uint8_t *data;
   size_t dataLen;
   MqttSnFlags flags;
   MqttSnReturnCode returnCode;

   //Parse PUBLISH message
   error = mqttSnParsePublish(message, &flags, &msgId, &topicId,
      &data, &dataLen);

   //Valid message received?
   if(!error)
   {
      //Check QoS level
      if(flags.qos == MQTT_SN_QOS_LEVEL_1)
      {
         //Deliver the message to the application
         returnCode = mqttSnDeliverPublishMessage(context, flags, topicId,
            data, dataLen);

         //A PUBACK packet is the response to a PUBLISH packet with QoS level 1
         error = mqttSnClientSendPubAck(context, msgId, topicId, returnCode);
      }
      else if(flags.qos == MQTT_SN_QOS_LEVEL_2)
      {
         //Check whether the ownership of the QoS 2 message has already been
         //transferred to the client
         duplicate = mqttSnClientIsDuplicateMessageId(context, msgId);

         //Duplicate message?
         if(duplicate)
         {
            //In QoS 2, the receiver must not cause duplicate messages to be
            //delivered to any onward recipients
            error = mqttSnClientSendPubRec(context, msgId);
         }
         else
         {
            //Accept the ownership of the QoS 2 message
            error = mqttSnClientStoreMessageId(context, msgId);

            //Check status code
            if(!error)
            {
               //Initiate onward delivery of the application message
               returnCode = mqttSnDeliverPublishMessage(context, flags, topicId,
                  data, dataLen);
            }
            else
            {
               //The message identifier cannot be saved
               returnCode = MQTT_SN_RETURN_CODE_REJECTED_CONGESTION;
            }

            //Check whether the PUBLISH message has been accepted
            if(returnCode == MQTT_SN_RETURN_CODE_ACCEPTED)
            {
               //A PUBREC packet is the response to a PUBLISH packet with QoS 2.
               //It is the second packet of the QoS 2 protocol exchange
               error = mqttSnClientSendPubRec(context, msgId);
            }
            else
            {
               //Discard message identifier
               mqttSnClientDiscardMessageId(context, msgId);

               //Reject the PUBLISH message
               error = mqttSnClientSendPubAck(context, msgId, topicId,
                  returnCode);
            }
         }
      }
      else
      {
         //Deliver the message to the application
         returnCode = mqttSnDeliverPublishMessage(context, flags, topicId,
            data, dataLen);

         //In QoS 0, the receiver always accepts the message
         error = NO_ERROR;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming PUBACK message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessPubAck(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   uint16_t msgId;
   uint16_t topicId;
   MqttSnReturnCode returnCode;

   //Parse PUBACK message
   error = mqttSnParsePubAck(message, &msgId, &topicId, &returnCode);

   //Valid message received?
   if(!error)
   {
      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ &&
         context->msgType == MQTT_SN_MSG_TYPE_PUBLISH &&
         context->msgId == msgId)
      {
         //Notify the application that a PUBACK message has been received
         context->state = MQTT_SN_CLIENT_STATE_RESP_RECEIVED;
         context->msgType = MQTT_SN_MSG_TYPE_PUBACK;
         context->returnCode = returnCode;

         //The MQTT-SN gateway is alive
         context->keepAliveCounter = 0;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming PUBREC message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessPubRec(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   uint16_t msgId;

   //Parse PUBREC message
   error = mqttSnParsePubRec(message, &msgId);

   //Valid message received?
   if(!error)
   {
      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ &&
         context->msgType == MQTT_SN_MSG_TYPE_PUBLISH &&
         context->msgId == msgId)
      {
         //Notify the application that a PUBREC message has been received
         context->state = MQTT_SN_CLIENT_STATE_RESP_RECEIVED;
         context->msgType = MQTT_SN_MSG_TYPE_PUBREC;

         //The MQTT-SN gateway is alive
         context->keepAliveCounter = 0;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming PUBREL message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessPubRel(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   uint16_t msgId;

   //Parse PUBREL message
   error = mqttSnParsePubRel(message, &msgId);

   //Valid message received?
   if(!error)
   {
      //Discard message identifier
      mqttSnClientDiscardMessageId(context, msgId);

      //A PUBCOMP message is the response to a PUBREL packet. It is the fourth
      //and final message of the QoS 2 protocol exchange
      error = mqttSnClientSendPubComp(context, msgId);
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming PUBCOMP message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessPubComp(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   uint16_t msgId;

   //Parse PUBCOMP message
   error = mqttSnParsePubComp(message, &msgId);

   //Valid message received?
   if(!error)
   {
      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ &&
         context->msgType == MQTT_SN_MSG_TYPE_PUBREL &&
         context->msgId == msgId)
      {
         //Notify the application that a PUBCOMP message has been received
         context->state = MQTT_SN_CLIENT_STATE_RESP_RECEIVED;
         context->msgType = MQTT_SN_MSG_TYPE_PUBCOMP;

         //The MQTT-SN gateway is alive
         context->keepAliveCounter = 0;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming SUBACK message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessSubAck(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   uint16_t msgId;
   uint16_t topicId;
   MqttSnFlags flags;
   MqttSnReturnCode returnCode;

   //Parse SUBACK message
   error = mqttSnParseSubAck(message, &flags, &msgId, &topicId, &returnCode);

   //Valid message received?
   if(!error)
   {
      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ &&
         context->msgType == MQTT_SN_MSG_TYPE_SUBSCRIBE &&
         context->msgId == msgId)
      {
         //Notify the application that a SUBACK message has been received
         context->state = MQTT_SN_CLIENT_STATE_RESP_RECEIVED;
         context->msgType = MQTT_SN_MSG_TYPE_SUBACK;
         context->topicId = topicId;
         context->returnCode = returnCode;

         //The MQTT-SN gateway is alive
         context->keepAliveCounter = 0;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming UNSUBACK message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessUnsubAck(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   uint16_t msgId;

   //Parse UNSUBACK message
   error = mqttSnParseUnsubAck(message, &msgId);

   //Valid message received?
   if(!error)
   {
      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ &&
         context->msgType == MQTT_SN_MSG_TYPE_UNSUBSCRIBE &&
         context->msgId == msgId)
      {
         //Notify the application that a UNSUBACK message has been received
         context->state = MQTT_SN_CLIENT_STATE_RESP_RECEIVED;
         context->msgType = MQTT_SN_MSG_TYPE_UNSUBACK;

         //The MQTT-SN gateway is alive
         context->keepAliveCounter = 0;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming PINGREQ message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessPingReq(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   const char_t *clientId;

   //Parse PINGREQ message
   error = mqttSnParsePingReq(message, &clientId);

   //Valid message received?
   if(!error)
   {
      //A client shall answer with a PINGRESP message when it receives a
      //PINGREQ message from the GW to which it is connected
      error = mqttSnClientSendPingResp(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming PINGRESP message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessPingResp(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;

   //Parse PINGRESP message
   error = mqttSnParsePingResp(message);

   //Valid message received?
   if(!error)
   {
      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ &&
         context->msgType == MQTT_SN_MSG_TYPE_PINGREQ)
      {
         //Notify the application that a PINGRESP message has been received
         context->state = MQTT_SN_CLIENT_STATE_RESP_RECEIVED;
         context->msgType = MQTT_SN_MSG_TYPE_PINGRESP;
      }

      //The MQTT-SN gateway is alive
      context->keepAliveCounter = 0;
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming DISCONNECT message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessDisconnect(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   uint16_t duration;

   //Parse DISCONNECT message
   error = mqttSnParseDisconnect(message, &duration);

   //Valid message received?
   if(!error)
   {
      //The gateway indicates that it wants to close the connection
      context->state = MQTT_SN_CLIENT_STATE_DISCONNECTING;
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming WILLTOPICRESP message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessWillTopicResp(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   MqttSnReturnCode returnCode;

   //Parse WILLTOPICRESP message
   error = mqttSnParseWillTopicResp(message, &returnCode);

   //Valid message received?
   if(!error)
   {
      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ &&
         context->msgType == MQTT_SN_MSG_TYPE_WILLTOPICUPD)
      {
         //Notify the application that a WILLTOPICRESP message has been received
         context->state = MQTT_SN_CLIENT_STATE_RESP_RECEIVED;
         context->msgType = MQTT_SN_MSG_TYPE_WILLTOPICRESP;
         context->returnCode = returnCode;

         //The MQTT-SN gateway is alive
         context->keepAliveCounter = 0;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming WILLMSGRESP message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] message Pointer to the received MQTT-SN message
 * @return Error code
 **/

error_t mqttSnClientProcessWillMsgResp(MqttSnClientContext *context,
   const MqttSnMessage *message)
{
   error_t error;
   MqttSnReturnCode returnCode;

   //Parse WILLMSGRESP message
   error = mqttSnParseWillMsgResp(message, &returnCode);

   //Valid message received?
   if(!error)
   {
      //Check current state
      if(context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ &&
         context->msgType == MQTT_SN_MSG_TYPE_WILLMSGUPD)
      {
         //Notify the application that a WILLMSGRESP message has been received
         context->state = MQTT_SN_CLIENT_STATE_RESP_RECEIVED;
         context->msgType = MQTT_SN_MSG_TYPE_WILLMSGRESP;
         context->returnCode = returnCode;

         //The MQTT-SN gateway is alive
         context->keepAliveCounter = 0;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Send SEARCHGW message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] radius Broadcast radius of SEARCHGW message
 * @param[in] destIpAddr Destination IP address
 * @param[in] destPort Destination port number
 * @return Error code
 **/

error_t mqttSnClientSendSearchGw(MqttSnClientContext *context, uint8_t radius,
   const IpAddr *destIpAddr, uint16_t destPort)
{
   error_t error;

   //Format SEARCHGW message
   error = mqttSnFormatSearchGw(&context->message, radius);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending SEARCHGW message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientBroadcastDatagram(context, destIpAddr, destPort,
         context->message.buffer, context->message.length);

      //Save the time at which the message was sent
      context->retransmitStartTime = osGetSystemTime();
   }

   //Return status code
   return error;
}


/**
 * @brief Send CONNECT message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] cleanSession If this flag is set, then the client and server
 *   must discard any previous session and start a new one
 * @return Error code
 **/

error_t mqttSnClientSendConnect(MqttSnClientContext *context,
   bool_t cleanSession)
{
   error_t error;
   systime_t time;
   MqttSnFlags flags;

   //The flags contains a number of parameters specifying the behavior of
   //the MQTT-SN connection
   flags.all = 0;

   //If CleanSession is set to 1, the client and server must discard any
   //previous session and start a new one
   if(cleanSession)
      flags.cleanSession = TRUE;

   //If the client supplies a zero-byte Client Identifier, the client must
   //also set CleanSession to 1
   if(context->clientId[0] == '\0')
      flags.cleanSession = TRUE;

   //Check whether a valid Will message has been specified
   if(context->willMessage.topic[0] != '\0')
      flags.will = TRUE;

   //Format CONNECT message
   error = mqttSnFormatConnect(&context->message, flags,
      context->keepAlive / 1000, context->clientId);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending CONNECT message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);

      //Get current time
      time = osGetSystemTime();

      //Save the time at which the message was sent
      context->retransmitStartTime = time;
      context->keepAliveTimestamp = time;

      //Update MQTT-SN client state
      context->state = MQTT_SN_CLIENT_STATE_SENDING_REQ;
      context->msgType = MQTT_SN_MSG_TYPE_CONNECT;
   }

   //Return status code
   return error;
}


/**
 * @brief Send WILLTOPIC message
 * @param[in] context Pointer to the MQTT-SN client context
 * @return Error code
 **/

error_t mqttSnClientSendWillTopic(MqttSnClientContext *context)
{
   error_t error;

   //Format WILLTOPIC message
   error = mqttSnFormatWillTopic(&context->message,
      context->willMessage.flags, context->willMessage.topic);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending WILLTOPIC message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);
   }

   //Return status code
   return error;
}


/**
 * @brief Send WILLMSG message
 * @param[in] context Pointer to the MQTT-SN client context
 * @return Error code
 **/

error_t mqttSnClientSendWillMsg(MqttSnClientContext *context)
{
   error_t error;

   //Format WILLMSG message
   error = mqttSnFormatWillMsg(&context->message,
      context->willMessage.payload, context->willMessage.length);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending WILLMSG message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);
   }

   //Return status code
   return error;
}


/**
 * @brief Send REGISTER message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicName Topic name
 * @return Error code
 **/

error_t mqttSnClientSendRegister(MqttSnClientContext *context,
   const char_t *topicName)
{
   error_t error;
   systime_t time;

   //Format REGISTER message
   error = mqttSnFormatRegister(&context->message, context->msgId, 0,
      topicName);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending REGISTER message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);

      //Get current time
      time = osGetSystemTime();

      //Save the time at which the message was sent
      context->retransmitStartTime = time;
      context->keepAliveTimestamp = time;

      //Update MQTT-SN client state
      context->state = MQTT_SN_CLIENT_STATE_SENDING_REQ;
      context->msgType = MQTT_SN_MSG_TYPE_REGISTER;
   }

   //Return status code
   return error;
}


/**
 * @brief Send REGACK message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] msgId Message identifier
 * @param[in] topicId Topic identifier
 * @param[in] returnCode Return code
 * @return Error code
 **/

error_t mqttSnClientSendRegAck(MqttSnClientContext *context, uint16_t msgId,
   uint16_t topicId, MqttSnReturnCode returnCode)
{
   error_t error;

   //Format REGACK message
   error = mqttSnFormatRegAck(&context->message, msgId, topicId, returnCode);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending REGACK message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);
   }

   //Return status code
   return error;
}


/**
 * @brief Send PUBLISH message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] msgId Message identifier
 * @param[in] topicName Short topic name
 * @param[in] data Message payload
 * @param[in] length Length of the message payload
 * @param[in] qos QoS level to be used when publishing the message
 * @param[in] retain This flag specifies if the message is to be retained
 * @param[in] dup This flag specifies if the message is sent for the first
 *   time or if the message is retransmitted
 * @return Error code
 **/

error_t mqttSnClientSendPublish(MqttSnClientContext *context,
   uint16_t msgId, const char_t *topicName, const uint8_t *data,
   size_t length, MqttSnQosLevel qos, bool_t retain, bool_t dup)
{
   error_t error;
   systime_t time;
   uint16_t topicId;
   MqttSnFlags flags;

   //Initialize status code
   error = NO_ERROR;

   //Reset unused flags
   flags.all = 0;
   //Set QoS level
   flags.qos = qos;
   //Set RETAIN flag
   flags.retain = retain;
   //set DUP flag
   flags.dup = dup;

   //Short topic name?
   if(mqttSnClientIsShortTopicName(topicName))
   {
      //The PUBLISH message contains a short topic name
      flags.topicIdType = MQTT_SN_SHORT_TOPIC_NAME;
      //The value of the topic ID is not relevant
      topicId = 0;
   }
   else
   {
      //Check whether a predefined topic ID has been registered
      topicId = mqttSnClientFindPredefTopicName(context, topicName);

      //Predefined topic ID found?
      if(topicId != MQTT_SN_INVALID_TOPIC_ID)
      {
         //The PUBLISH message contains a predefined topic ID
         flags.topicIdType = MQTT_SN_PREDEFINED_TOPIC_ID;
      }
      else
      {
         //Check whether the topic name has been registered
         topicId = mqttSnClientFindTopicName(context, topicName);

         //Topic ID found?
         if(topicId != MQTT_SN_INVALID_TOPIC_ID)
         {
            //The PUBLISH message contains a normal topic ID
            flags.topicIdType = MQTT_SN_NORMAL_TOPIC_ID;
         }
         else
         {
            //The topic name has not been registered
            error = ERROR_NOT_FOUND;
         }
      }
   }

   //Check status code
   if(!error)
   {
      //Format PUBLISH message
      error = mqttSnFormatPublish(&context->message, flags, msgId,
         topicId, topicName, data, length);
   }

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending PUBLISH message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);

      //Get current time
      time = osGetSystemTime();

      //Check QoS level
      if(qos == MQTT_SN_QOS_LEVEL_1 || qos == MQTT_SN_QOS_LEVEL_2)
      {
         //Save the time at which the message was sent
         context->retransmitStartTime = time;
         context->keepAliveTimestamp = time;

         //Update MQTT-SN client state
         context->state = MQTT_SN_CLIENT_STATE_SENDING_REQ;
         context->msgType = MQTT_SN_MSG_TYPE_PUBLISH;
      }
      else
      {
         //In the QoS 0, no response is sent by the receiver and no retry
         //is performed by the sender
         context->state = MQTT_SN_CLIENT_STATE_ACTIVE;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Send PUBACK message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] msgId Message identifier of the corresponding PUBLISH message
 * @param[in] topicId Topic identifier
 * @param[in] returnCode Return code
 * @return Error code
 **/

error_t mqttSnClientSendPubAck(MqttSnClientContext *context, uint16_t msgId,
   uint16_t topicId, MqttSnReturnCode returnCode)
{
   error_t error;

   //Format PUBACK message
   error = mqttSnFormatPubAck(&context->message, msgId, topicId, returnCode);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending PUBACK message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);
   }

   //Return status code
   return error;
}


/**
 * @brief Send PUBREC message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] msgId Message identifier of the corresponding PUBLISH message
 * @return Error code
 **/

error_t mqttSnClientSendPubRec(MqttSnClientContext *context, uint16_t msgId)
{
   error_t error;

   //Format PUBREC message
   error = mqttSnFormatPubRec(&context->message, msgId);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending PUBREC message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);
   }

   //Return status code
   return error;
}


/**
 * @brief Send PUBREL message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] msgId Message identifier of the corresponding PUBLISH message
 * @return Error code
 **/

error_t mqttSnClientSendPubRel(MqttSnClientContext *context, uint16_t msgId)
{
   error_t error;
   systime_t time;

   //Format PUBREL message
   error = mqttSnFormatPubRel(&context->message, msgId);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending PUBREL message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);

      //Get current time
      time = osGetSystemTime();

      //Save the time at which the message was sent
      context->retransmitStartTime = time;
      context->keepAliveTimestamp = time;

      //Update MQTT-SN client state
      context->state = MQTT_SN_CLIENT_STATE_SENDING_REQ;
      context->msgType = MQTT_SN_MSG_TYPE_PUBREL;
   }

   //Return status code
   return error;
}


/**
 * @brief Send PUBCOMP message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] msgId Message identifier of the corresponding PUBLISH message
 * @return Error code
 **/

error_t mqttSnClientSendPubComp(MqttSnClientContext *context, uint16_t msgId)
{
   error_t error;

   //Format PUBCOMP message
   error = mqttSnFormatPubComp(&context->message, msgId);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending PUBCOMP message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);
   }

   //Return status code
   return error;
}


/**
 * @brief Send SUBSCRIBE message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicName Topic filter
 * @param[in] qos Maximum QoS level at which the server can send application
 *   messages to the client
 * @return Error code
 **/

error_t mqttSnClientSendSubscribe(MqttSnClientContext *context,
   const char_t *topicName, MqttSnQosLevel qos)
{
   error_t error;
   systime_t time;
   uint16_t topicId;
   MqttSnFlags flags;

   //Initialize status code
   error = NO_ERROR;

   //Reset unused flags
   flags.all = 0;
   //Set QoS level
   flags.qos = qos;

   //Check whether a predefined topic ID has been registered
   topicId = mqttSnClientFindPredefTopicName(context, topicName);

   //Predefined topic ID found?
   if(topicId != MQTT_SN_INVALID_TOPIC_ID)
   {
      //The SUBSCRIBE message contains a predefined topic ID
      flags.topicIdType = MQTT_SN_PREDEFINED_TOPIC_ID;
   }
   else
   {
      //Short topic name?
      if(mqttSnClientIsShortTopicName(topicName))
      {
         //The SUBSCRIBE message contains a short topic name
         flags.topicIdType = MQTT_SN_SHORT_TOPIC_NAME;
      }
      else
      {
         //The SUBSCRIBE message contains a normal topic name
         flags.topicIdType = MQTT_SN_NORMAL_TOPIC_NAME;
      }

      //Format SUBSCRIBE message
      error = mqttSnFormatSubscribe(&context->message, flags,
         context->msgId, topicId, topicName);
   }

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending SUBSCRIBE message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);

      //Get current time
      time = osGetSystemTime();

      //Save the time at which the message was sent
      context->retransmitStartTime = time;
      context->keepAliveTimestamp = time;

      //Update MQTT-SN client state
      context->state = MQTT_SN_CLIENT_STATE_SENDING_REQ;
      context->msgType = MQTT_SN_MSG_TYPE_SUBSCRIBE;
   }

   //Return status code
   return error;
}


/**
 * @brief Send UNSUBSCRIBE message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicName Topic filter
 * @return Error code
 **/

error_t mqttSnClientSendUnsubscribe(MqttSnClientContext *context,
   const char_t *topicName)
{
   error_t error;
   systime_t time;
   uint16_t topicId;
   MqttSnFlags flags;

   //Initialize status code
   error = NO_ERROR;

   //Reset unused flags
   flags.all = 0;

   //Check whether a predefined topic ID has been registered
   topicId = mqttSnClientFindPredefTopicName(context, topicName);

   //Predefined topic ID found?
   if(topicId != MQTT_SN_INVALID_TOPIC_ID)
   {
      //The UNSUBSCRIBE message contains a predefined topic ID
      flags.topicIdType = MQTT_SN_PREDEFINED_TOPIC_ID;
   }
   else
   {
      //Short topic name?
      if(osStrlen(topicName) == 2 && osStrchr(topicName, '#') == NULL &&
         osStrchr(topicName, '+') == NULL)
      {
         //The UNSUBSCRIBE message contains a short topic name
         flags.topicIdType = MQTT_SN_SHORT_TOPIC_NAME;
      }
      else
      {
         //The UNSUBSCRIBE message contains a normal topic name
         flags.topicIdType = MQTT_SN_NORMAL_TOPIC_NAME;
      }

      //Format UNSUBSCRIBE message
      error = mqttSnFormatUnsubscribe(&context->message, flags,
         context->msgId, topicId, topicName);
   }

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending UNSUBSCRIBE message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);

      //Get current time
      time = osGetSystemTime();

      //Save the time at which the message was sent
      context->retransmitStartTime = time;
      context->keepAliveTimestamp = time;

      //Update MQTT-SN client state
      context->state = MQTT_SN_CLIENT_STATE_SENDING_REQ;
      context->msgType = MQTT_SN_MSG_TYPE_UNSUBSCRIBE;
   }

   //Return status code
   return error;
}


/**
 * @brief Send PINGREQ message
 * @param[in] context Pointer to the MQTT-SN client context
 * @return Error code
 **/

error_t mqttSnClientSendPingReq(MqttSnClientContext *context)
{
   error_t error;

   //Format PINGREQ message
   error = mqttSnFormatPingReq(&context->message, context->clientId);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending PINGREQ message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);

      //Save the time at which the message was sent
      context->keepAliveTimestamp = osGetSystemTime();
   }

   //Return status code
   return error;
}


/**
 * @brief Send PINGRESP message
 * @param[in] context Pointer to the MQTT-SN client context
 * @return Error code
 **/

error_t mqttSnClientSendPingResp(MqttSnClientContext *context)
{
   error_t error;

   //Format PINGRESP message
   error = mqttSnFormatPingResp(&context->message);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending PINGRESP message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);
   }

   //Return status code
   return error;
}


/**
 * @brief Send DISCONNECT message
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] duration Value of the sleep timer
 * @return Error code
 **/

error_t mqttSnClientSendDisconnect(MqttSnClientContext *context,
   uint16_t duration)
{
   error_t error;

   //Format DISCONNECT message
   error = mqttSnFormatDisconnect(&context->message, duration);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending DISCONNECT message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);

      //Save the time at which the message was sent
      context->retransmitStartTime = osGetSystemTime();

      //Update MQTT-SN client state
      context->state = MQTT_SN_CLIENT_STATE_SENDING_REQ;
      context->msgType = MQTT_SN_MSG_TYPE_DISCONNECT;
   }

   //Return status code
   return error;
}


/**
 * @brief Send WILLTOPICUPD message
 * @param[in] context Pointer to the MQTT-SN client context
 * @return Error code
 **/

error_t mqttSnClientSendWillTopicUpd(MqttSnClientContext *context)
{
   error_t error;
   systime_t time;

   //Format WILLTOPICUPD message
   error = mqttSnFormatWillTopicUpd(&context->message,
      context->willMessage.flags, context->willMessage.topic);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending WILLTOPICUPD message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);

      //Get current time
      time = osGetSystemTime();

      //Save the time at which the message was sent
      context->retransmitStartTime = time;
      context->keepAliveTimestamp = time;

      //Update MQTT-SN client state
      context->state = MQTT_SN_CLIENT_STATE_SENDING_REQ;
      context->msgType = MQTT_SN_MSG_TYPE_WILLTOPICUPD;
   }

   //Return status code
   return error;
}


/**
 * @brief Send WILLMSGUPD message
 * @param[in] context Pointer to the MQTT-SN client context
 * @return Error code
 **/

error_t mqttSnClientSendWillMsgUpd(MqttSnClientContext *context)
{
   error_t error;
   systime_t time;

   //Format WILLMSGUPD message
   error = mqttSnFormatWillMsgUpd(&context->message,
      context->willMessage.payload, context->willMessage.length);

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending WILLMSGUPD message (%" PRIuSIZE " bytes)...\r\n",
         context->message.length);

      //Dump the contents of the message for debugging purpose
      mqttSnDumpMessage(context->message.buffer, context->message.length);

      //Send MQTT-SN message
      error = mqttSnClientSendDatagram(context, context->message.buffer,
         context->message.length);

      //Get current time
      time = osGetSystemTime();

      //Save the time at which the message was sent
      context->retransmitStartTime = time;
      context->keepAliveTimestamp = time;

      //Update MQTT-SN client state
      context->state = MQTT_SN_CLIENT_STATE_SENDING_REQ;
      context->msgType = MQTT_SN_MSG_TYPE_WILLMSGUPD;
   }

   //Return status code
   return error;
}

#endif
