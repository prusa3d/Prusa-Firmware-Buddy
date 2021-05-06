/**
 * @file mqtt_sn_client_misc.h
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

#ifndef _MQTT_SN_CLIENT_MISC_H
#define _MQTT_SN_CLIENT_MISC_H

//Dependencies
#include "core/net.h"
#include "mqtt_sn/mqtt_sn_client.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//MQTT-SN client related functions
error_t mqttSnClientProcessEvents(MqttSnClientContext *context,
   systime_t timeout);

MqttSnReturnCode mqttSnDeliverPublishMessage(MqttSnClientContext *context,
   MqttSnFlags flags, uint16_t topicId, const uint8_t *data, size_t dataLen);

error_t mqttSnClientAddTopic(MqttSnClientContext *context,
   const char_t *topicName, uint16_t topicId);

error_t mqttSnClientDeleteTopic(MqttSnClientContext *context,
   const char_t *topicName);

const char_t *mqttSnClientFindTopicId(MqttSnClientContext *context,
   uint16_t topicId);

uint16_t mqttSnClientFindTopicName(MqttSnClientContext *context,
   const char_t *topicName);

const char_t *mqttSnClientFindPredefTopicId(MqttSnClientContext *context,
   uint16_t topicId);

uint16_t mqttSnClientFindPredefTopicName(MqttSnClientContext *context,
   const char_t *topicName);

uint16_t mqttSnClientGenerateMessageId(MqttSnClientContext *context);

error_t mqttSnClientStoreMessageId(MqttSnClientContext *context,
   uint16_t msgId);

error_t mqttSnClientDiscardMessageId(MqttSnClientContext *context,
   uint16_t msgId);

bool_t mqttSnClientIsDuplicateMessageId(MqttSnClientContext *context,
   uint16_t msgId);

bool_t mqttSnClientIsShortTopicName(const char_t *topicName);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
