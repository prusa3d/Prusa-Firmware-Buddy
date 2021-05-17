/**
 * @file mqtt_client.h
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

#ifndef _MQTT_CLIENT_H
#define _MQTT_CLIENT_H

//Dependencies
#include "core/net.h"
#include "mqtt/mqtt_common.h"

//MQTT client support
#ifndef MQTT_CLIENT_SUPPORT
   #define MQTT_CLIENT_SUPPORT ENABLED
#elif (MQTT_CLIENT_SUPPORT != ENABLED && MQTT_CLIENT_SUPPORT != DISABLED)
   #error MQTT_CLIENT_SUPPORT parameter is not valid
#endif

//MQTT over TLS
#ifndef MQTT_CLIENT_TLS_SUPPORT
   #define MQTT_CLIENT_TLS_SUPPORT DISABLED
#elif (MQTT_CLIENT_TLS_SUPPORT != ENABLED && MQTT_CLIENT_TLS_SUPPORT != DISABLED)
   #error MQTT_CLIENT_TLS_SUPPORT parameter is not valid
#endif

//MQTT over WebSocket
#ifndef MQTT_CLIENT_WS_SUPPORT
   #define MQTT_CLIENT_WS_SUPPORT DISABLED
#elif (MQTT_CLIENT_WS_SUPPORT != ENABLED && MQTT_CLIENT_WS_SUPPORT != DISABLED)
   #error MQTT_CLIENT_WS_SUPPORT parameter is not valid
#endif

//Default keep-alive time interval, in seconds
#ifndef MQTT_CLIENT_DEFAULT_KEEP_ALIVE
   #define MQTT_CLIENT_DEFAULT_KEEP_ALIVE 0
#elif (MQTT_CLIENT_DEFAULT_KEEP_ALIVE < 0)
   #error MQTT_CLIENT_DEFAULT_KEEP_ALIVE parameter is not valid
#endif

//Default communication timeout, in milliseconds
#ifndef MQTT_CLIENT_DEFAULT_TIMEOUT
   #define MQTT_CLIENT_DEFAULT_TIMEOUT 20000
#elif (MQTT_CLIENT_DEFAULT_TIMEOUT < 0)
   #error MQTT_CLIENT_DEFAULT_TIMEOUT parameter is not valid
#endif

//Maximum length of the hostname
#ifndef MQTT_CLIENT_MAX_HOST_LEN
   #define MQTT_CLIENT_MAX_HOST_LEN 32
#elif (MQTT_CLIENT_MAX_HOST_LEN < 1)
   #error MQTT_CLIENT_MAX_HOST_LEN parameter is not valid
#endif

//Maximum length of the resource name
#ifndef MQTT_CLIENT_MAX_URI_LEN
   #define MQTT_CLIENT_MAX_URI_LEN 16
#elif (MQTT_CLIENT_MAX_URI_LEN < 1)
   #error MQTT_CLIENT_MAX_URI_LEN parameter is not valid
#endif

//Maximum length of the client identifier
#ifndef MQTT_CLIENT_MAX_ID_LEN
   #define MQTT_CLIENT_MAX_ID_LEN 23
#elif (MQTT_CLIENT_MAX_ID_LEN < 0)
   #error MQTT_CLIENT_MAX_ID_LEN parameter is not valid
#endif

//Maximum length of the user name
#ifndef MQTT_CLIENT_MAX_USERNAME_LEN
   #define MQTT_CLIENT_MAX_USERNAME_LEN 16
#elif (MQTT_CLIENT_MAX_USERNAME_LEN < 0)
   #error MQTT_CLIENT_MAX_USERNAME_LEN parameter is not valid
#endif

//Maximum length of the password
#ifndef MQTT_CLIENT_MAX_PASSWORD_LEN
   #define MQTT_CLIENT_MAX_PASSWORD_LEN 16
#elif (MQTT_CLIENT_MAX_PASSWORD_LEN < 0)
   #error MQTT_CLIENT_MAX_PASSWORD_LEN parameter is not valid
#endif

//Maximum length of the will topic
#ifndef MQTT_CLIENT_MAX_WILL_TOPIC_LEN
   #define MQTT_CLIENT_MAX_WILL_TOPIC_LEN 16
#elif (MQTT_CLIENT_MAX_WILL_TOPIC_LEN < 0)
   #error MQTT_CLIENT_MAX_WILL_TOPIC_LEN parameter is not valid
#endif

//Maximum length of the will message payload
#ifndef MQTT_CLIENT_MAX_WILL_PAYLOAD_LEN
   #define MQTT_CLIENT_MAX_WILL_PAYLOAD_LEN 16
#elif (MQTT_CLIENT_MAX_WILL_PAYLOAD_LEN < 0)
   #error MQTT_CLIENT_MAX_WILL_PAYLOAD_LEN parameter is not valid
#endif

//Size of the MQTT client buffer
#ifndef MQTT_CLIENT_BUFFER_SIZE
   #define MQTT_CLIENT_BUFFER_SIZE 1024
#elif (MQTT_CLIENT_BUFFER_SIZE < 1)
   #error MQTT_CLIENT_BUFFER_SIZE parameter is not valid
#endif

//TLS supported?
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   #include "core/crypto.h"
   #include "tls.h"
#endif

//WebSocket supported?
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   #include "web_socket/web_socket.h"
#endif

//Forward declaration of MqttClientContext structure
struct _MqttClientContext;
#define MqttClientContext struct _MqttClientContext

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief MQTT client states
 **/

typedef enum
{
   MQTT_CLIENT_STATE_DISCONNECTED        = 0,
   MQTT_CLIENT_STATE_CONNECTING          = 1,
   MQTT_CLIENT_STATE_CONNECTED           = 2,
   MQTT_CLIENT_STATE_IDLE                = 3,
   MQTT_CLIENT_STATE_SENDING_PACKET      = 4,
   MQTT_CLIENT_STATE_PACKET_SENT         = 5,
   MQTT_CLIENT_STATE_WAITING_PACKET      = 6,
   MQTT_CLIENT_STATE_RECEIVING_PACKET    = 7,
   MQTT_CLIENT_STATE_PACKET_RECEIVED     = 8,
   MQTT_CLIENT_STATE_DISCONNECTING       = 9
} MqttClientState;


/**
 * @brief CONNACK message received callback
 **/

typedef void (*MqttClientConnAckCallback)(MqttClientContext *context,
   uint8_t connectAckFlags, uint8_t connectReturnCode);


/**
 * @brief PUBLISH message received callback
 **/

typedef void (*MqttClientPublishCallback)(MqttClientContext *context,
   const char_t *topic, const uint8_t *message, size_t length,
   bool_t dup, MqttQosLevel qos, bool_t retain, uint16_t packetId);


/**
 * @brief PUBACK message received callback
 **/

typedef void (*MqttClientPubAckCallback)(MqttClientContext *context,
   uint16_t packetId);


/**
 * @brief PUBREC message received callback
 **/

typedef void (*MqttClientPubRecCallback)(MqttClientContext *context,
   uint16_t packetId);


/**
 * @brief PUBREL message received callback
 **/

typedef void (*MqttClientPubRelCallback)(MqttClientContext *context,
   uint16_t packetId);


/**
 * @brief PUBCOMP message received callback
 **/

typedef void (*MqttClientPubCompCallback)(MqttClientContext *context,
   uint16_t packetId);


/**
 * @brief SUBACK message received callback
 **/

typedef void (*MqttClientSubAckCallback)(MqttClientContext *context,
   uint16_t packetId);


/**
 * @brief UNSUBACK message received callback
 **/

typedef void (*MqttClientUnsubAckCallback)(MqttClientContext *context,
   uint16_t packetId);


/**
 * @brief PINGRESP message received callback
 **/

typedef void (*MqttClientPingRespCallback)(MqttClientContext *context);


//TLS supported?
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)

/**
 * @brief TLS initialization callback
 **/

typedef error_t (*MqttClientTlsInitCallback)(MqttClientContext *context,
   TlsContext *tlsContext);

#endif


/**
 * @brief Will message
 **/

typedef struct
{
   char_t topic[MQTT_CLIENT_MAX_WILL_TOPIC_LEN + 1];  ///<Will topic name
   uint8_t payload[MQTT_CLIENT_MAX_WILL_PAYLOAD_LEN]; ///<Will message payload
   size_t length;                                     ///<Length of the Will message payload
   MqttQosLevel qos;                                  ///<QoS level to be used when publishing the Will message
   bool_t retain;                                     ///<Specifies if the Will message is to be retained
} MqttClientWillMessage;


/**
 * @brief MQTT client callback functions
 **/

typedef struct
{
   MqttClientConnAckCallback connAckCallback;   ///<CONNACK message received callback
   MqttClientPublishCallback publishCallback;   ///<PUBLISH message received callback
   MqttClientPubAckCallback pubAckCallback;     ///<PUBACK message received callback
   MqttClientPubAckCallback pubRecCallback;     ///<PUBREC message received callback
   MqttClientPubAckCallback pubRelCallback;     ///<PUBREL message received callback
   MqttClientPubAckCallback pubCompCallback;    ///<PUBCOMP message received callback
   MqttClientPubAckCallback subAckCallback;     ///<SUBACK message received callback
   MqttClientPubAckCallback unsubAckCallback;   ///<UNSUBACK message received callback
   MqttClientPingRespCallback pingRespCallback; ///<PINGRESP message received callback
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   MqttClientTlsInitCallback tlsInitCallback;   ///<TLS initialization callback
#endif
} MqttClientCallbacks;


/**
 * @brief MQTT client settings
 **/

typedef struct
{
   MqttVersion version;                               ///<MQTT protocol version
   MqttTransportProtocol transportProtocol;           ///<Transport protocol
   uint16_t keepAlive;                                ///<Keep-alive time interval
   systime_t timeout;                                 ///<Communication timeout
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   char_t host[MQTT_CLIENT_MAX_HOST_LEN + 1];         ///<Domain name of the server (for virtual hosting)
   char_t uri[MQTT_CLIENT_MAX_URI_LEN + 1];           ///<Resource name
#endif
   char_t clientId[MQTT_CLIENT_MAX_ID_LEN + 1];       ///<Client identifier
   char_t username[MQTT_CLIENT_MAX_USERNAME_LEN + 1]; ///<User name
   char_t password[MQTT_CLIENT_MAX_PASSWORD_LEN + 1]; ///<Password
   MqttClientWillMessage willMessage;                 ///<Will message
} MqttClientSettings;


/**
 * @brief MQTT client context
 **/

struct _MqttClientContext
{
   MqttClientSettings settings;             ///<MQTT client settings
   MqttClientCallbacks callbacks;           ///<MQTT client callback functions
   MqttClientState state;                   ///<MQTT client state
   NetInterface *interface;                 ///<Underlying network interface
   Socket *socket;                          ///<Underlying TCP socket
#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)
   TlsContext *tlsContext;                  ///<TLS context
   TlsSessionState tlsSession;              ///<TLS session state
#endif
#if (MQTT_CLIENT_WS_SUPPORT == ENABLED)
   WebSocket *webSocket;                    ///<Underlying WebSocket
#endif
   systime_t startTime;                     ///<Start time
   systime_t keepAliveTimestamp;            ///<Timestamp used to manage keep-alive
   uint8_t buffer[MQTT_CLIENT_BUFFER_SIZE]; ///<Internal buffer
   uint8_t *packet;                         ///<Pointer to the incoming/outgoing MQTT packet
   size_t packetPos;                        ///<Current position
   size_t packetLen;                        ///<Length of the entire MQTT packet
   MqttPacketType packetType;               ///<Control packet type
   uint16_t packetId;                       ///<Packet identifier
   size_t remainingLen;                     ///<Length of the variable header and payload
};


//MQTT client related functions
error_t mqttClientInit(MqttClientContext *context);
void mqttClientInitCallbacks(MqttClientCallbacks *callbacks);

error_t mqttClientRegisterCallbacks(MqttClientContext *context,
   const MqttClientCallbacks *callbacks);

error_t mqttClientSetVersion(MqttClientContext *context, MqttVersion version);

error_t mqttClientSetTransportProtocol(MqttClientContext *context,
   MqttTransportProtocol transportProtocol);

#if (MQTT_CLIENT_TLS_SUPPORT == ENABLED)

error_t mqttClientRegisterTlsInitCallback(MqttClientContext *context,
   MqttClientTlsInitCallback callback);

#endif

error_t mqttClientRegisterPublishCallback(MqttClientContext *context,
   MqttClientPublishCallback callback);

error_t mqttClientSetTimeout(MqttClientContext *context, systime_t timeout);
error_t mqttClientSetKeepAlive(MqttClientContext *context, uint16_t keepAlive);

error_t mqttClientSetHost(MqttClientContext *context, const char_t *host);
error_t mqttClientSetUri(MqttClientContext *context, const char_t *uri);

error_t mqttClientSetIdentifier(MqttClientContext *context,
   const char_t *clientId);

error_t mqttClientSetAuthInfo(MqttClientContext *context,
   const char_t *username, const char_t *password);

error_t mqttClientSetWillMessage(MqttClientContext *context, const char_t *topic,
   const void *message, size_t length, MqttQosLevel qos, bool_t retain);

error_t mqttClientBindToInterface(MqttClientContext *context,
   NetInterface *interface);

error_t mqttClientConnect(MqttClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort, bool_t cleanSession);

error_t mqttClientPublish(MqttClientContext *context,
   const char_t *topic, const void *message, size_t length,
   MqttQosLevel qos, bool_t retain, uint16_t *packetId);

error_t mqttClientSubscribe(MqttClientContext *context,
   const char_t *topic, MqttQosLevel qos, uint16_t *packetId);

error_t mqttClientUnsubscribe(MqttClientContext *context,
   const char_t *topic, uint16_t *packetId);

error_t mqttClientPing(MqttClientContext *context, systime_t *rtt);

error_t mqttClientTask(MqttClientContext *context, systime_t timeout);

error_t mqttClientDisconnect(MqttClientContext *context);
error_t mqttClientClose(MqttClientContext *context);

void mqttClientDeinit(MqttClientContext *context);

//Deprecated functions
error_t mqttClientProcessEvents(MqttClientContext *context, systime_t timeout);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
