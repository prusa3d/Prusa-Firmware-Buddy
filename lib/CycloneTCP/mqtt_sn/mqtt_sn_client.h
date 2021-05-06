/**
 * @file mqtt_sn_client.h
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

#ifndef _MQTT_SN_CLIENT_H
#define _MQTT_SN_CLIENT_H

//Dependencies
#include "core/net.h"
#include "mqtt_sn/mqtt_sn_common.h"
#include "mqtt_sn/mqtt_sn_message.h"

//MQTT-SN client support
#ifndef MQTT_SN_CLIENT_SUPPORT
   #define MQTT_SN_CLIENT_SUPPORT ENABLED
#elif (MQTT_SN_CLIENT_SUPPORT != ENABLED && MQTT_SN_CLIENT_SUPPORT != DISABLED)
   #error MQTT_SN_CLIENT_SUPPORT parameter is not valid
#endif

//MQTT-SN over DTLS
#ifndef MQTT_SN_CLIENT_DTLS_SUPPORT
   #define MQTT_SN_CLIENT_DTLS_SUPPORT DISABLED
#elif (MQTT_SN_CLIENT_DTLS_SUPPORT != ENABLED && MQTT_SN_CLIENT_DTLS_SUPPORT != DISABLED)
   #error MQTT_SN_CLIENT_DTLS_SUPPORT parameter is not valid
#endif

//Maximum number of topics that can be registered
#ifndef MQTT_SN_CLIENT_TOPIC_TABLE_SIZE
   #define MQTT_SN_CLIENT_TOPIC_TABLE_SIZE 10
#elif (MQTT_SN_CLIENT_TOPIC_TABLE_SIZE < 0)
   #error MQTT_SN_CLIENT_TOPIC_TABLE_SIZE parameter is not valid
#endif

//Maximum number of QoS 2 messages that can be accepted
#ifndef MQTT_SN_CLIENT_MSG_ID_TABLE_SIZE
   #define MQTT_SN_CLIENT_MSG_ID_TABLE_SIZE 10
#elif (MQTT_SN_CLIENT_MSG_ID_TABLE_SIZE < 0)
   #error MQTT_SN_CLIENT_MSG_ID_TABLE_SIZE parameter is not valid
#endif

//Maximum length of the client identifier
#ifndef MQTT_SN_CLIENT_MAX_ID_LEN
   #define MQTT_SN_CLIENT_MAX_ID_LEN 23
#elif (MQTT_SN_CLIENT_MAX_ID_LEN < 0)
   #error MQTT_SN_CLIENT_MAX_ID_LEN parameter is not valid
#endif

//Maximum length of topic names
#ifndef MQTT_SN_CLIENT_MAX_TOPIC_NAME_LEN
   #define MQTT_SN_CLIENT_MAX_TOPIC_NAME_LEN 32
#elif (MQTT_SN_CLIENT_MAX_TOPIC_NAME_LEN < 0)
   #error MQTT_SN_CLIENT_MAX_TOPIC_NAME_LEN parameter is not valid
#endif

//Maximum length of the will topic
#ifndef MQTT_SN_CLIENT_MAX_WILL_TOPIC_LEN
   #define MQTT_SN_CLIENT_MAX_WILL_TOPIC_LEN 32
#elif (MQTT_SN_CLIENT_MAX_WILL_TOPIC_LEN < 0)
   #error MQTT_SN_CLIENT_MAX_WILL_TOPIC_LEN parameter is not valid
#endif

//Maximum length of the will message payload
#ifndef MQTT_SN_CLIENT_MAX_WILL_PAYLOAD_LEN
   #define MQTT_SN_CLIENT_MAX_WILL_PAYLOAD_LEN 32
#elif (MQTT_SN_CLIENT_MAX_WILL_PAYLOAD_LEN < 0)
   #error MQTT_SN_CLIENT_MAX_WILL_PAYLOAD_LEN parameter is not valid
#endif

//MQTT-SN client tick interval
#ifndef MQTT_SN_CLIENT_TICK_INTERVAL
   #define MQTT_SN_CLIENT_TICK_INTERVAL 100
#elif (MQTT_SN_CLIENT_TICK_INTERVAL < 10)
   #error MQTT_SN_CLIENT_TICK_INTERVAL parameter is not valid
#endif

//Default timeout
#ifndef MQTT_SN_CLIENT_DEFAULT_TIMEOUT
   #define MQTT_SN_CLIENT_DEFAULT_TIMEOUT 30000
#elif (MQTT_SN_CLIENT_DEFAULT_TIMEOUT < 0)
   #error MQTT_SN_CLIENT_DEFAULT_TIMEOUT parameter is not valid
#endif

//Default keep-alive time interval
#ifndef MQTT_SN_CLIENT_DEFAULT_KEEP_ALIVE
   #define MQTT_SN_CLIENT_DEFAULT_KEEP_ALIVE 0
#elif (MQTT_SN_CLIENT_DEFAULT_KEEP_ALIVE < 0)
   #error MQTT_SN_CLIENT_DEFAULT_KEEP_ALIVE parameter is not valid
#endif

//Number of retransmissions of keep-alive probes
#ifndef MQTT_SN_CLIENT_MAX_KEEP_ALIVE_PROBES
   #define MQTT_SN_CLIENT_MAX_KEEP_ALIVE_PROBES 5
#elif (MQTT_SN_CLIENT_MAX_KEEP_ALIVE_PROBES < 0)
   #error MQTT_SN_CLIENT_MAX_KEEP_ALIVE_PROBES parameter is not valid
#endif

//Initial delay before the client starts searching for gateways
#ifndef MQTT_SN_CLIENT_SEARCH_DELAY
   #define MQTT_SN_CLIENT_SEARCH_DELAY 0
#elif (MQTT_SN_CLIENT_SEARCH_DELAY < 0)
   #error MQTT_SN_CLIENT_SEARCH_DELAY parameter is not valid
#endif

//Retransmission timeout
#ifndef MQTT_SN_CLIENT_RETRY_TIMEOUT
   #define MQTT_SN_CLIENT_RETRY_TIMEOUT 5000
#elif (MQTT_SN_CLIENT_RETRY_TIMEOUT < 1000)
   #error MQTT_SN_CLIENT_RETRY_TIMEOUT parameter is not valid
#endif

//DTLS supported?
#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "tls.h"
#endif

//Forward declaration of MqttSnClientContext structure
struct _MqttSnClientContext;
#define MqttSnClientContext struct _MqttSnClientContext

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief MQTT-SN client states
 **/

typedef enum
{
   MQTT_SN_CLIENT_STATE_DISCONNECTED  = 0,
   MQTT_SN_CLIENT_STATE_SEARCHING     = 1,
   MQTT_SN_CLIENT_STATE_CONNECTING    = 2,
   MQTT_SN_CLIENT_STATE_ACTIVE        = 3,
   MQTT_SN_CLIENT_STATE_SENDING_REQ   = 4,
   MQTT_SN_CLIENT_STATE_RESP_RECEIVED = 5,
   MQTT_SN_CLIENT_STATE_LOST          = 6,
   MQTT_SN_CLIENT_STATE_ASLEEP        = 7,
   MQTT_SN_CLIENT_STATE_AWAKE         = 8,
   MQTT_SN_CLIENT_STATE_DISCONNECTING = 9
} MqttSnClientState;


//DTLS supported?
#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)

/**
 * @brief DTLS initialization callback
 **/

typedef error_t (*MqttSnClientDtlsInitCallback)(MqttSnClientContext *context,
   TlsContext *dtlsContext);

#endif


/**
 * @brief PUBLISH message received callback
 **/

typedef void (*MqttSnClientPublishCallback)(MqttSnClientContext *context,
   const char_t *topic, const uint8_t *message, size_t length,
   MqttSnQosLevel qos, bool_t retain);


/**
 * @brief Will message
 **/

typedef struct
{
   char_t topic[MQTT_SN_CLIENT_MAX_WILL_TOPIC_LEN + 1];  ///<Will topic name
   uint8_t payload[MQTT_SN_CLIENT_MAX_WILL_PAYLOAD_LEN]; ///<Will message payload
   size_t length;                                        ///<Length of the Will message payload
   MqttSnFlags flags;                                    ///<Flags
} MqttSnClientWillMessage;


/**
 * @brief Mapping between a topic name and a topic ID
 **/

typedef struct
{
   char_t topicName[MQTT_SN_CLIENT_MAX_TOPIC_NAME_LEN + 1]; ///<Topic name
   uint16_t topicId;                                        ///<Topic identifier
} MqttSnClientTopicEntry;


/**
 * @brief QoS 2 message state
 **/

typedef struct
{
   bool_t ownership; ///<Ownership of message accepted by the client
   uint16_t msgId;   ///<Message identifier
} MqttSnClientMsgIdEntry;


/**
 * @brief MQTT-SN client context
 **/

struct _MqttSnClientContext
{
   MqttSnClientState state;                           ///<MQTT-SN client state
   MqttSnTransportProtocol transportProtocol;         ///<Transport protocol (UDP or DTLS)
   const MqttSnPredefinedTopic *predefinedTopicTable; ///<List of predefined topics
   uint_t predefinedTopicTableSize;                   ///<Number of predefined topics
   systime_t timeout;                                 ///<Timeout value
   systime_t keepAlive;                               ///<Keep-alive interval
   char_t clientId[MQTT_SN_CLIENT_MAX_ID_LEN + 1];    ///<Client identifier
   MqttSnClientWillMessage willMessage;               ///<Will message
   NetInterface *interface;                           ///<Underlying network interface
   Socket *socket;                                    ///<Underlying TCP socket
#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)
   TlsContext *dtlsContext;                           ///<DTLS context
   TlsSessionState dtlsSession;                       ///<DTLS session state
   MqttSnClientDtlsInitCallback dtlsInitCallback;     ///<DTLS initialization callback
#endif
   MqttSnClientPublishCallback publishCallback;       ///<PUBLISH message received callback
   IpAddr gwIpAddr;                                   ///<Gateway IP address
   uint16_t gwPort;                                   ///<Gateway port number
   systime_t startTime;                               ///<Start time
   systime_t retransmitStartTime;                     ///<Time at which the last message was sent
   systime_t retransmitTimeout;                       ///<Retransmission timeout
   systime_t keepAliveTimestamp;                      ///<Timestamp used to manage keep-alive
   uint_t keepAliveCounter;                           ///<PINGREQ retransmission counter
   MqttSnMessage message;                             ///<MQTT-SN message
   MqttSnMsgType msgType;                             ///<Message type
   uint16_t msgId;                                    ///<Message identifier
   uint16_t topicId;                                  ///<Topic identifier returned by the gateway (REGACK/SUBACK)
   MqttSnReturnCode returnCode;                       ///<Status code returned by the gateway
   MqttSnClientTopicEntry topicTable[MQTT_SN_CLIENT_TOPIC_TABLE_SIZE];
   MqttSnClientMsgIdEntry msgIdTable[MQTT_SN_CLIENT_MSG_ID_TABLE_SIZE];
};


//MQTT-SN client related functions
error_t mqttSnClientInit(MqttSnClientContext *context);

error_t mqttSnClientSetTransportProtocol(MqttSnClientContext *context,
   MqttSnTransportProtocol transportProtocol);

#if (MQTT_SN_CLIENT_DTLS_SUPPORT == ENABLED)

error_t mqttSnClientRegisterDtlsInitCallback(MqttSnClientContext *context,
   MqttSnClientDtlsInitCallback callback);

#endif

error_t mqttSnClientRegisterPublishCallback(MqttSnClientContext *context,
   MqttSnClientPublishCallback callback);

error_t mqttSnClientSetPredefinedTopics(MqttSnClientContext *context,
   MqttSnPredefinedTopic *predefinedTopics, uint_t size);

error_t mqttSnClientSetTimeout(MqttSnClientContext *context,
   systime_t timeout);

error_t mqttSnClientSetKeepAlive(MqttSnClientContext *context,
   systime_t keepAlive);

error_t mqttSnClientSetIdentifier(MqttSnClientContext *context,
   const char_t *clientId);

error_t mqttSnClientSetWillMessage(MqttSnClientContext *context,
   const char_t *topic, const void *message, size_t length,
   MqttSnQosLevel qos, bool_t retain);

error_t mqttSnClientBindToInterface(MqttSnClientContext *context,
   NetInterface *interface);

error_t mqttSnClientSetGateway(MqttSnClientContext *context,
   const IpAddr *gwIpAddr, uint16_t gwPort);

error_t mqttSnClientSearchGateway(MqttSnClientContext *context,
   const IpAddr *destIpAddr, uint16_t destPort);

error_t mqttSnClientConnect(MqttSnClientContext *context, bool_t cleanSession);

error_t mqttSnClientPublish(MqttSnClientContext *context,
   const char_t *topicName, const void *message, size_t length,
   MqttSnQosLevel qos, bool_t retain, bool_t dup, uint16_t *msgId);

error_t mqttSnClientSubscribe(MqttSnClientContext *context,
   const char_t *topicName, MqttSnQosLevel qos);

error_t mqttSnClientUnsubscribe(MqttSnClientContext *context,
   const char_t *topicName);

error_t mqttSnClientPing(MqttSnClientContext *context);

error_t mqttSnClientUpdateWillMessage(MqttSnClientContext *context,
   const char_t *topic, const void *message, size_t length,
   MqttSnQosLevel qos, bool_t retain);

error_t mqttSnClientGetReturnCode(MqttSnClientContext *context,
   MqttSnReturnCode *returnCode);

error_t mqttSnClientTask(MqttSnClientContext *context, systime_t timeout);

error_t mqttSnClientDisconnect(MqttSnClientContext *context,
   systime_t duration);

void mqttSnClientDeinit(MqttSnClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
