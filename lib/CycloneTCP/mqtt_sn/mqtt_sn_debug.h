/**
 * @file mqtt_sn_debug.h
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

#ifndef _MQTT_SN_DEBUG_H
#define _MQTT_SN_DEBUG_H

//Dependencies
#include "core/net.h"
#include "mqtt_sn/mqtt_sn_common.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//MQTT-SN related functions
error_t mqttSnDumpMessage(const uint8_t *message, size_t length);

error_t mqttSnDumpAdvertise(const MqttSnAdvertise *message, size_t length);
error_t mqttSnDumpSearchGw(const MqttSnSearchGw *message, size_t length);
error_t mqttSnDumpGwInfo(const MqttSnGwInfo *message, size_t length);
error_t mqttSnDumpConnect(const MqttSnConnect *message, size_t length);
error_t mqttSnDumpConnAck(const MqttSnConnAck *message, size_t length);
error_t mqttSnDumpWillTopicReq(const MqttSnWillTopicReq *message, size_t length);
error_t mqttSnDumpWillTopic(const MqttSnWillTopic *message, size_t length);
error_t mqttSnDumpWillMsgReq(const MqttSnWillMsgReq *message, size_t length);
error_t mqttSnDumpWillMsg(const MqttSnWillMsg *message, size_t length);
error_t mqttSnDumpRegister(const MqttSnRegister *message, size_t length);
error_t mqttSnDumpRegAck(const MqttSnRegAck *message, size_t length);
error_t mqttSnDumpPublish(const MqttSnPublish *message, size_t length);
error_t mqttSnDumpPubAck(const MqttSnPubAck *message, size_t length);
error_t mqttSnDumpPubRec(const MqttSnPubRec *message, size_t length);
error_t mqttSnDumpPubRel(const MqttSnPubRel *message, size_t length);
error_t mqttSnDumpPubComp(const MqttSnPubComp *message, size_t length);
error_t mqttSnDumpSubscribe(const MqttSnSubscribe *message, size_t length);
error_t mqttSnDumpSubAck(const MqttSnSubAck *message, size_t length);
error_t mqttSnDumpUnsubscribe(const MqttSnUnsubscribe *message, size_t length);
error_t mqttSnDumpUnsubAck(const MqttSnUnsubAck *message, size_t length);
error_t mqttSnDumpPingReq(const MqttSnPingReq *message, size_t length);
error_t mqttSnDumpPingResp(const MqttSnPingResp *message, size_t length);
error_t mqttSnDumpDisconnect(const MqttSnDisconnect *message, size_t length);
error_t mqttSnDumpWillTopicUpd(const MqttSnWillTopicUpd *message, size_t length);
error_t mqttSnDumpWillTopicResp(const MqttSnWillTopicResp *message, size_t length);
error_t mqttSnDumpWillMsgUpd(const MqttSnWillMsgUpd *message, size_t length);
error_t mqttSnDumpWillMsgResp(const MqttSnWillMsgResp *message, size_t length);

void mqttSnDumpFlags(MqttSnFlags flags);

const char_t *mqttSnGetMessageName(uint16_t msgType);
const char_t *mqttSnGetReturnCodeDesc(uint16_t returnCode);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
