/**
 * @file mqtt_sn_client_message.h
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

#ifndef _MQTT_SN_CLIENT_MESSAGE_H
#define _MQTT_SN_CLIENT_MESSAGE_H

//Dependencies
#include "core/net.h"
#include "mqtt_sn/mqtt_sn_client.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//MQTT-SN client related functions
error_t mqttSnClientProcessMessage(MqttSnClientContext *context,
   MqttSnMessage *message, const IpAddr *ipAddr, uint16_t port);

error_t mqttSnClientProcessGwInfo(MqttSnClientContext *context,
   const MqttSnMessage *message, const IpAddr *ipAddr, uint16_t port);

error_t mqttSnClientProcessConnAck(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessWillTopicReq(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessWillMsgReq(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessRegister(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessRegAck(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessPublish(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessPubAck(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessPubRec(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessPubRel(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessPubComp(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessSubAck(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessUnsubAck(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessPingReq(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessPingResp(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessDisconnect(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessWillTopicResp(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientProcessWillMsgResp(MqttSnClientContext *context,
   const MqttSnMessage *message);

error_t mqttSnClientSendSearchGw(MqttSnClientContext *context, uint8_t radius,
   const IpAddr *destIpAddr, uint16_t destPort);

error_t mqttSnClientSendConnect(MqttSnClientContext *context,
   bool_t cleanSession);

error_t mqttSnClientSendWillTopic(MqttSnClientContext *context);
error_t mqttSnClientSendWillMsg(MqttSnClientContext *context);

error_t mqttSnClientSendRegister(MqttSnClientContext *context,
   const char_t *topicName);

error_t mqttSnClientSendRegAck(MqttSnClientContext *context, uint16_t msgId,
   uint16_t topicId, MqttSnReturnCode returnCode);

error_t mqttSnClientSendPublish(MqttSnClientContext *context,
   uint16_t msgId, const char_t *topicName, const uint8_t *data,
   size_t length, MqttSnQosLevel qos, bool_t retain, bool_t dup);

error_t mqttSnClientSendPubAck(MqttSnClientContext *context, uint16_t msgId,
   uint16_t topicId, MqttSnReturnCode returnCode);

error_t mqttSnClientSendPubRec(MqttSnClientContext *context, uint16_t msgId);
error_t mqttSnClientSendPubRel(MqttSnClientContext *context, uint16_t msgId);
error_t mqttSnClientSendPubComp(MqttSnClientContext *context, uint16_t msgId);

error_t mqttSnClientSendSubscribe(MqttSnClientContext *context,
   const char_t *topicName, MqttSnQosLevel qos);

error_t mqttSnClientSendUnsubscribe(MqttSnClientContext *context,
   const char_t *topicName);

error_t mqttSnClientSendPingReq(MqttSnClientContext *context);
error_t mqttSnClientSendPingResp(MqttSnClientContext *context);

error_t mqttSnClientSendDisconnect(MqttSnClientContext *context,
   uint16_t duration);

error_t mqttSnClientSendWillTopicUpd(MqttSnClientContext *context);
error_t mqttSnClientSendWillMsgUpd(MqttSnClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
