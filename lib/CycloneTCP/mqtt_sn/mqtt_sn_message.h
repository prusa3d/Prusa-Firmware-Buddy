/**
 * @file mqtt_sn_message.h
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

#ifndef _MQTT_SN_MESSAGE_H
#define _MQTT_SN_MESSAGE_H

//Dependencies
#include "core/net.h"
#include "mqtt_sn/mqtt_sn_common.h"

//Maximum size of MQTT-SN messages
#ifndef MQTT_SN_MAX_MSG_SIZE
   #define MQTT_SN_MAX_MSG_SIZE 1024
#elif (MQTT_SN_MAX_MSG_SIZE < 16)
   #error MQTT_SN_MAX_MSG_SIZE parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief MQTT-SN message
 **/

typedef struct
{
   uint8_t buffer[MQTT_SN_MAX_MSG_SIZE + 1];
   size_t length;
   size_t pos;
} MqttSnMessage;


//MQTT-SN related functions
error_t mqttSnFormatHeader(MqttSnMessage *message, MqttSnMsgType type,
   size_t length);

error_t mqttSnFormatSearchGw(MqttSnMessage *message,
   uint8_t radius);

error_t mqttSnFormatConnect(MqttSnMessage *message, MqttSnFlags flags,
   uint16_t duration, const char_t *clientId);

error_t mqttSnFormatWillTopic(MqttSnMessage *message, MqttSnFlags flags,
   const char_t *topicName);

error_t mqttSnFormatWillMsg(MqttSnMessage *message, const void *data,
   size_t dataLen);

error_t mqttSnFormatRegister(MqttSnMessage *message,
   uint16_t msgId, uint16_t topicId, const char_t *topicName);

error_t mqttSnFormatRegAck(MqttSnMessage *message, uint16_t msgId,
   uint16_t topicId, MqttSnReturnCode returnCode);

error_t mqttSnFormatPublish(MqttSnMessage *message, MqttSnFlags flags,
   uint16_t msgId, uint16_t topicId, const char_t *topicName,
   const uint8_t *data, size_t dataLen);

error_t mqttSnFormatPubAck(MqttSnMessage *message, uint16_t msgId,
   uint16_t topicId, MqttSnReturnCode returnCode);

error_t mqttSnFormatPubRec(MqttSnMessage *message, uint16_t msgId);
error_t mqttSnFormatPubRel(MqttSnMessage *message, uint16_t msgId);
error_t mqttSnFormatPubComp(MqttSnMessage *message, uint16_t msgId);

error_t mqttSnFormatSubscribe(MqttSnMessage *message, MqttSnFlags flags,
   uint16_t msgId, uint16_t topicId, const char_t *topicName);

error_t mqttSnFormatUnsubscribe(MqttSnMessage *message, MqttSnFlags flags,
   uint16_t msgId, uint16_t topicId, const char_t *topicName);

error_t mqttSnFormatPingReq(MqttSnMessage *message, const char_t *clientId);
error_t mqttSnFormatPingResp(MqttSnMessage *message);

error_t mqttSnFormatDisconnect(MqttSnMessage *message,
   uint16_t duration);

error_t mqttSnFormatWillTopicUpd(MqttSnMessage *message, MqttSnFlags flags,
   const char_t *topicName);

error_t mqttSnFormatWillMsgUpd(MqttSnMessage *message, const void *data,
   size_t dataLen);

error_t mqttSnParseHeader(MqttSnMessage *message, MqttSnMsgType *type);

error_t mqttSnParseGwInfo(const MqttSnMessage *message, uint8_t *gwId,
   const uint8_t **gwAdd, size_t *gwAddLen);

error_t mqttSnParseConnAck(const MqttSnMessage *message,
   MqttSnReturnCode *returnCode);

error_t mqttSnParseWillTopicReq(const MqttSnMessage *message);
error_t mqttSnParseWillMsgReq(const MqttSnMessage *message);

error_t mqttSnParseRegister(const MqttSnMessage *message, uint16_t *msgId,
   uint16_t *topicId, const char_t **topicName);

error_t mqttSnParseRegAck(const MqttSnMessage *message, uint16_t *msgId,
   uint16_t *topicId, MqttSnReturnCode *returnCode);

error_t mqttSnParsePublish(const MqttSnMessage *message, MqttSnFlags *flags,
   uint16_t *msgId, uint16_t *topicId, const uint8_t **data, size_t *dataLen);

error_t mqttSnParsePubAck(const MqttSnMessage *message, uint16_t *msgId,
   uint16_t *topicId, MqttSnReturnCode *returnCode);

error_t mqttSnParsePubRec(const MqttSnMessage *message, uint16_t *msgId);
error_t mqttSnParsePubRel(const MqttSnMessage *message, uint16_t *msgId);
error_t mqttSnParsePubComp(const MqttSnMessage *message, uint16_t *msgId);

error_t mqttSnParseSubAck(const MqttSnMessage *message, MqttSnFlags *flags,
   uint16_t *msgId, uint16_t *topicId, MqttSnReturnCode *returnCode);

error_t mqttSnParseUnsubAck(const MqttSnMessage *message, uint16_t *msgId);

error_t mqttSnParsePingReq(const MqttSnMessage *message,
   const char_t **clientId);

error_t mqttSnParsePingResp(const MqttSnMessage *message);

error_t mqttSnParseDisconnect(const MqttSnMessage *message,
   uint16_t *duration);

error_t mqttSnParseWillTopicResp(const MqttSnMessage *message,
   MqttSnReturnCode *returnCode);

error_t mqttSnParseWillMsgResp(const MqttSnMessage *message,
   MqttSnReturnCode *returnCode);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
