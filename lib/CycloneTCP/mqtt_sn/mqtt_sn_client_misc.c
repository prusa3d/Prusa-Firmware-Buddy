/**
 * @file mqtt_sn_client_misc.c
 * @brief Helper functions for MQTT-SN client
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
 * @brief Process MQTT-SN client events
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] timeout Maximum time to wait before returning
 * @return Error code
 **/

error_t mqttSnClientProcessEvents(MqttSnClientContext *context,
   systime_t timeout)
{
   error_t error;
   systime_t d;
   systime_t startTime;
   systime_t currentTime;
   IpAddr ipAddr;
   uint16_t port;

   //Flush buffer
   context->message.length = 0;

   //Save current time
   currentTime = osGetSystemTime();
   startTime = currentTime;

   //Initialize status code
   error = NO_ERROR;

   //Process events
   do
   {
      //Maximum time to wait for an incoming datagram
      if(timeCompare(startTime + timeout, currentTime) > 0)
         d = startTime + timeout - currentTime;
      else
         d = 0;

      //Limit the delay
      d = MIN(d, MQTT_SN_CLIENT_TICK_INTERVAL);

      //Wait for an incoming datagram
      error = mqttSnClientReceiveDatagram(context, &ipAddr, &port,
         context->message.buffer, MQTT_SN_MAX_MSG_SIZE,
         &context->message.length, d);

      //Get current time
      currentTime = osGetSystemTime();

      //Any datagram received?
      if(error == NO_ERROR)
      {
         //Terminate the payload with a NULL character
         context->message.buffer[context->message.length] = '\0';

         //Process the received MQTT-SN message
         mqttSnClientProcessMessage(context, &context->message, &ipAddr, port);
      }
      else if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
      {
         //No datagram has been received
         error = NO_ERROR;
      }
      else
      {
         //A communication error has occurred
      }

      //Check status code
      if(!error)
      {
         //A keep-alive value of zero has the effect of turning off the keep
         //alive mechanism
         if(context->keepAlive != 0)
         {
            //Make sure the MQTT-SN client is connected
            if(context->state == MQTT_SN_CLIENT_STATE_ACTIVE ||
               context->state == MQTT_SN_CLIENT_STATE_SENDING_REQ ||
               context->state == MQTT_SN_CLIENT_STATE_RESP_RECEIVED)
            {
               //Check whether the keep-alive timer has expired
               if(timeCompare(currentTime, context->keepAliveTimestamp +
                  context->keepAlive) >= 0)
               {
                  //Check retransmission counter
                  if(context->keepAliveCounter < MQTT_SN_CLIENT_MAX_KEEP_ALIVE_PROBES)
                  {
                     //Send a PINGREQ message to the gateway
                     error = mqttSnClientSendPingReq(context);

                     //Increment retransmission counter
                     context->keepAliveCounter++;
                  }
                  else
                  {
                     //If a client does not receive a PINGRESP from the gateway
                     //even after multiple retransmissions of the PINGREQ message,
                     //then the gateway is considered offline
                     context->state = MQTT_SN_CLIENT_STATE_DISCONNECTING;

                     //The connection is lost
                     error = ERROR_NOT_CONNECTED;
                  }
               }
            }
         }
      }

      //Check whether the timeout has elapsed
   } while(error == NO_ERROR && context->message.length == 0 && d > 0);

   //Return status code
   return error;
}


/**
 * @brief Deliver a PUBLISH message to the application
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] flags Flags
 * @param[in] topicId Topic identifier
 * @param[in] data Message payload
 * @param[in] dataLen Length of the message payload
 * @return Error code
 **/

MqttSnReturnCode mqttSnDeliverPublishMessage(MqttSnClientContext *context,
   MqttSnFlags flags, uint16_t topicId, const uint8_t *data, size_t dataLen)
{
   const char_t *topicName;
   char_t shortTopicName[3];
   MqttSnReturnCode returnCode;

   //Check the type of topic identifier
   if(flags.topicIdType == MQTT_SN_NORMAL_TOPIC_ID)
   {
      //Retrieve the topic name associated with the normal topic ID
      topicName = mqttSnClientFindTopicId(context, topicId);
   }
   else if(flags.topicIdType == MQTT_SN_PREDEFINED_TOPIC_ID)
   {
      //Retrieve the topic name associated with the predefined topic ID
      topicName = mqttSnClientFindPredefTopicId(context, topicId);
   }
   else if(flags.topicIdType == MQTT_SN_SHORT_TOPIC_NAME)
   {
      //Short topic names are topic names that have a fixed length of two
      //octets. They are short enough for being carried together with the
      //data within PUBLISH messages
      shortTopicName[0] = MSB(topicId);
      shortTopicName[1] = LSB(topicId);
      shortTopicName[2] = '\0';

      //Point to the resulting topic name
      topicName = shortTopicName;
   }
   else
   {
      //The value of the TopicIdType flag is not valid
      topicName = NULL;
   }

   //Check whether the topic name has been successfully resolved
   if(topicName != NULL)
   {
      //Any registered callback?
      if(context->publishCallback != NULL)
      {
         //Deliver the message to the application
         context->publishCallback(context, topicName, data, dataLen,
            (MqttSnQosLevel) flags.qos, flags.retain);
      }

      //Successfull processing
      returnCode = MQTT_SN_RETURN_CODE_ACCEPTED;
   }
   else
   {
      //The client has received a PUBLISH message with an unknown topic ID
      returnCode = MQTT_SN_RETURN_CODE_REJECTED_INVALID_TOPIC_ID;
   }

   //The return code indicates whether the PUBLISH message has been accepted
   //or rejected
   return returnCode;
}


/**
 * @brief Add a new entry to the topic table
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicName Topic name
 * @param[in] topicId Topic identifier
 * @return Error code
 **/

error_t mqttSnClientAddTopic(MqttSnClientContext *context,
   const char_t *topicName, uint16_t topicId)
{
   uint_t i;

   //Make sure the name of the topic name is acceptable
   if(osStrlen(topicName) > MQTT_SN_CLIENT_MAX_TOPIC_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Loop through the topic table
   for(i = 0; i < MQTT_SN_CLIENT_TOPIC_TABLE_SIZE; i++)
   {
      //Check whether the topic name has already been registered
      if(!osStrcmp(context->topicTable[i].topicName, topicName))
      {
         //Update topic identifier
         context->topicTable[i].topicId = topicId;

         //We are done
         return NO_ERROR;
      }
   }

   //Loop through the topic table
   for(i = 0; i < MQTT_SN_CLIENT_TOPIC_TABLE_SIZE; i++)
   {
      //Check whether the current entry is free
      if(context->topicTable[i].topicName[0] == '\0')
      {
         //Save mapping between topic name and topic ID
         osStrcpy(context->topicTable[i].topicName, topicName);
         context->topicTable[i].topicId = topicId;

         //A new entry has been successfully created
         return NO_ERROR;
      }
   }

   //The table runs out of entries
   return ERROR_OUT_OF_RESOURCES;
}


/**
 * @brief Remove an entry in the topic table
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicName Topic name
 * @return Error code
 **/

error_t mqttSnClientDeleteTopic(MqttSnClientContext *context,
   const char_t *topicName)
{
   uint_t i;

   //Loop through the list of topics that have been registered
   for(i = 0; i < MQTT_SN_CLIENT_TOPIC_TABLE_SIZE; i++)
   {
      //Matching topic name?
      if(!osStrcmp(context->topicTable[i].topicName, topicName))
      {
         //Release current entry
         context->topicTable[i].topicName[0] = '\0';

         //We are done
         return NO_ERROR;
      }
   }

   //The specified topic name does not exist
   return ERROR_NOT_FOUND;
}


/**
 * @brief Retrieve the topic name associated with a given topic ID
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicId Topic identifier
 * @return Topic name
 **/

const char_t *mqttSnClientFindTopicId(MqttSnClientContext *context,
   uint16_t topicId)
{
   uint_t i;
   const char_t *topicName;

   //Initialize topic name
   topicName = NULL;

   //Valid topic identifier?
   if(topicId != MQTT_SN_INVALID_TOPIC_ID)
   {
      //Loop through the list of topics that have been registered
      for(i = 0; i < MQTT_SN_CLIENT_TOPIC_TABLE_SIZE; i++)
      {
         //Matching topic identifier?
         if(context->topicTable[i].topicId == topicId)
         {
            //Retrieve the corresponding topic name
            topicName = context->topicTable[i].topicName;
            break;
         }
      }
   }

   //Return the corresponding topic name
   return topicName;
}


/**
 * @brief Retrieve the topic ID associated with a given topic name
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicName Topic name
 * @return Topic identifier
 **/

uint16_t mqttSnClientFindTopicName(MqttSnClientContext *context,
   const char_t *topicName)
{
   uint_t i;
   uint16_t topicId;

   //Initialize topic identifier
   topicId = MQTT_SN_INVALID_TOPIC_ID;

   //Valid topic name?
   if(topicName != NULL)
   {
      //Loop through the list of registered topics that have been registered
      for(i = 0; i < MQTT_SN_CLIENT_TOPIC_TABLE_SIZE; i++)
      {
         //Matching topic name?
         if(!osStrcmp(context->topicTable[i].topicName, topicName))
         {
            //Retrieve the corresponding topic identifier
            topicId = context->topicTable[i].topicId;
            break;
         }
      }
   }

   //Return the corresponding topic identifier
   return topicId;
}


/**
 * @brief Retrieve the topic name associated with a predefined topic ID
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicId Predefined topic identifier
 * @return Topic name
 **/

const char_t *mqttSnClientFindPredefTopicId(MqttSnClientContext *context,
   uint16_t topicId)
{
   uint_t i;
   const char_t *topicName;

   //Initialize topic name
   topicName = NULL;

   //Valid topic identifier?
   if(topicId != MQTT_SN_INVALID_TOPIC_ID)
   {
      //Loop through the list of predefined topics
      for(i = 0; i < context->predefinedTopicTableSize; i++)
      {
         //Matching topic identifier?
         if(context->predefinedTopicTable[i].topicId == topicId)
         {
            //Retrieve the corresponding topic name
            topicName = context->predefinedTopicTable[i].topicName;
            break;
         }
      }
   }

   //Return the corresponding topic name
   return topicName;
}


/**
 * @brief Retrieve the topic ID associated with a predefined topic name
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] topicName Predefined topic name
 * @return Topic identifier
 **/

uint16_t mqttSnClientFindPredefTopicName(MqttSnClientContext *context,
   const char_t *topicName)
{
   uint_t i;
   uint16_t topicId;

   //Initialize topic identifier
   topicId = MQTT_SN_INVALID_TOPIC_ID;

   //Valid topic name?
   if(topicName != NULL)
   {
      //Loop through the list of predefined topics
      for(i = 0; i < context->predefinedTopicTableSize; i++)
      {
         //Matching topic name?
         if(!osStrcmp(context->predefinedTopicTable[i].topicName, topicName))
         {
            //Retrieve the corresponding topic identifier
            topicId = context->predefinedTopicTable[i].topicId;
            break;
         }
      }
   }

   //Return the corresponding topic identifier
   return topicId;
}


/**
 * @brief Generate a new message identifier
 * @param[in] context Pointer to the MQTT-SN client context
 * @return 16-bit message identifier
 **/

uint16_t mqttSnClientGenerateMessageId(MqttSnClientContext *context)
{
   //Increment message identifier and wrap around if necessary
   if(context->msgId < UINT16_MAX)
      context->msgId++;
   else
      context->msgId = 1;

   //Return current value
   return context->msgId;
}


/**
 * @brief Store message ID (QoS 2 message processing)
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] msgId Message identifier
 * @return Error code
 **/

error_t mqttSnClientStoreMessageId(MqttSnClientContext *context,
   uint16_t msgId)
{
   uint_t i;

   //Loop through the list of message identifiers
   for(i = 0; i < MQTT_SN_CLIENT_MSG_ID_TABLE_SIZE; i++)
   {
      //Check whether the message ID has already been accepted by the client
      if(context->msgIdTable[i].ownership &&
         context->msgIdTable[i].msgId == msgId)
      {
         //We are done
         return NO_ERROR;
      }
   }

   //Loop through the list of message identifiers
   for(i = 0; i < MQTT_SN_CLIENT_MSG_ID_TABLE_SIZE; i++)
   {
      //Check whether the current entry is free
      if(!context->msgIdTable[i].ownership)
      {
         //Create a new entry
         context->msgIdTable[i].msgId = msgId;
         context->msgIdTable[i].ownership = TRUE;

         //We are done
         return NO_ERROR;
      }
   }

   //The table runs out of entries
   return ERROR_OUT_OF_RESOURCES;
}


/**
 * @brief Discard message ID (QoS 2 message processing)
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] msgId Message identifier
 * @return Error code
 **/

error_t mqttSnClientDiscardMessageId(MqttSnClientContext *context,
   uint16_t msgId)
{
   uint_t i;

   //Loop through the list of message identifiers
   for(i = 0; i < MQTT_SN_CLIENT_MSG_ID_TABLE_SIZE; i++)
   {
      //Matching message identifier?
      if(context->msgIdTable[i].ownership &&
         context->msgIdTable[i].msgId == msgId)
      {
         //Release current entry
         context->msgIdTable[i].msgId = 0;
         context->msgIdTable[i].ownership = FALSE;

         //We are done
         return NO_ERROR;
      }
   }

   //The specified message identifier does not exist
   return ERROR_NOT_FOUND;
}


/**
 * @brief Check whether the message ID is a duplicate (QoS 2 message processing)
 * @param[in] context Pointer to the MQTT-SN client context
 * @param[in] msgId Message identifier
 * @return TRUE if the message ID is a duplicate, else FALSE
 **/

bool_t mqttSnClientIsDuplicateMessageId(MqttSnClientContext *context,
   uint16_t msgId)
{
   uint_t i;
   bool_t duplicate;

   //Initialize flag
   duplicate = FALSE;

   //Loop through the list of message identifiers
   for(i = 0; i < MQTT_SN_CLIENT_MSG_ID_TABLE_SIZE; i++)
   {
      //Check whether the message ID has already been accepted by the client
      if(context->msgIdTable[i].ownership &&
         context->msgIdTable[i].msgId == msgId)
      {
         //The message ID is a duplicate
         duplicate = TRUE;
         break;
      }
   }

   //Return TRUE if the message ID is a duplicate
   return duplicate;
}


/**
 * @brief Check whether a topic name is a short topic name
 * @param[in] topicName Topic name
 * @return TRUE if the specified topic name is a short topic name, else FALSE
 **/

bool_t mqttSnClientIsShortTopicName(const char_t *topicName)
{
   bool_t res;

   //Initialize variable
   res = FALSE;

   //A short topic name is a topic name that has a fixed length of two octets
   if(osStrlen(topicName) == 2)
   {
      //Ensure the topic name does not contains wildcard characters
      if(osStrchr(topicName, '#') == NULL && osStrchr(topicName, '+') == NULL)
      {
         //The short topic name is a valid
         res = TRUE;
      }
   }

   //Return TRUE if the specified topic name is a short topic name
   return res;
}

#endif
