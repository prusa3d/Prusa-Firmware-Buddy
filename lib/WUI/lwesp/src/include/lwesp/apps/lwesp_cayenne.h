/**
 * \file            lwesp_cayenne.h
 * \brief           MQTT client for Cayenne
 */

/*
 * Copyright (c) 2020 Tilen MAJERLE
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of LwESP - Lightweight ESP-AT parser library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         v1.0.0
 */
#ifndef LWESP_HDR_APP_CAYENNE_H
#define LWESP_HDR_APP_CAYENNE_H

#include "lwesp/lwesp.h"
#include "lwesp/apps/lwesp_mqtt_client_api.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \ingroup         LWESP_APPS
 * \defgroup        LWESP_APP_CAYENNE_API MQTT client Cayenne API
 * \brief           MQTT client API for Cayenne
 * \{
 */

/**
 * \brief           Cayenne API version in string
 */
#ifndef LWESP_CAYENNE_API_VERSION
#define LWESP_CAYENNE_API_VERSION                 "v1"
#endif

/**
* \brief           Cayenne host server
*/
#ifndef LWESP_CAYENNE_HOST
#define LWESP_CAYENNE_HOST                        "mqtt.mydevices.com"
#endif

/**
 * \brief           Cayenne port number
 */
#ifndef LWESP_CAYENNE_PORT
#define LWESP_CAYENNE_PORT                        1883
#endif

#define LWESP_CAYENNE_NO_CHANNEL                  0xFFFE/*!< No channel macro */
#define LWESP_CAYENNE_ALL_CHANNELS                0xFFFF/*!< All channels macro */

/**
 * \brief           List of possible cayenne topics
 */
typedef enum {
    LWESP_CAYENNE_TOPIC_DATA,                   /*!< Data topic */
    LWESP_CAYENNE_TOPIC_COMMAND,                /*!< Command topic */
    LWESP_CAYENNE_TOPIC_CONFIG,
    LWESP_CAYENNE_TOPIC_RESPONSE,
    LWESP_CAYENNE_TOPIC_SYS_MODEL,
    LWESP_CAYENNE_TOPIC_SYS_VERSION,
    LWESP_CAYENNE_TOPIC_SYS_CPU_MODEL,
    LWESP_CAYENNE_TOPIC_SYS_CPU_SPEED,
    LWESP_CAYENNE_TOPIC_DIGITAL,
    LWESP_CAYENNE_TOPIC_DIGITAL_COMMAND,
    LWESP_CAYENNE_TOPIC_DIGITAL_CONFIG,
    LWESP_CAYENNE_TOPIC_ANALOG,
    LWESP_CAYENNE_TOPIC_ANALOG_COMMAND,
    LWESP_CAYENNE_TOPIC_ANALOG_CONFIG,
    LWESP_CAYENNE_TOPIC_END,                    /*!< Last entry */
} lwesp_cayenne_topic_t;

/**
 * \brief           Cayenne response types
 */
typedef enum {
    LWESP_CAYENNE_RLWESP_OK,                    /*!< Response OK */
    LWESP_CAYENNE_RLWESP_ERROR,                 /*!< Response error */
} lwesp_cayenne_rlwesp_t;

/**
 * \brief           Cayenne events
 */
typedef enum {
    LWESP_CAYENNE_EVT_CONNECT,                  /*!< Connect to Cayenne event */
    LWESP_CAYENNE_EVT_DISCONNECT,               /*!< Disconnect from Cayenne event */
    LWESP_CAYENNE_EVT_DATA,                     /*!< Data received event */
} lwesp_cayenne_evt_type_t;

/**
 * \brief           Key/Value pair structure
 */
typedef struct {
    const char* key;                            /*!< Key string */
    const char* value;                          /*!< Value string */
} lwesp_cayenne_key_value_t;

/**
 * \brief           Cayenne message
 */
typedef struct {
    lwesp_cayenne_topic_t topic;                /*!< Message topic */
    uint16_t channel;                           /*!< Message channel, optional, based on topic type */
    const char* seq;                            /*!< Sequence string on command */
    lwesp_cayenne_key_value_t values[2];        /*!< Key/Value pair of values */
    size_t values_count;                        /*!< Count of valid pairs in values member */
} lwesp_cayenne_msg_t;

/**
 * \brief           Cayenne event
 */
typedef struct {
    lwesp_cayenne_evt_type_t type;              /*!< Event type */
    union {
        struct {
            lwesp_cayenne_msg_t* msg;           /*!< Pointer to data message */
        } data;                                 /*!< Data event, used with \ref LWESP_CAYENNE_EVT_DATA event */
    } evt;                                      /*!< Event union */
} lwesp_cayenne_evt_t;

/**
 * \brief           Cayenne handle forward declaration
 */
struct lwesp_cayenne;

/**
 * \brief           Cayenne event callback function
 * \param[in]       c: Cayenne handle
 * \param[in]       evt: Event handle
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
typedef lwespr_t (*lwesp_cayenne_evt_fn)(struct lwesp_cayenne* c, lwesp_cayenne_evt_t* evt);

/**
 * \brief           Cayenne handle
 */
typedef struct lwesp_cayenne {
    lwesp_mqtt_client_api_p api_c;              /*!< MQTT API client */
    const lwesp_mqtt_client_info_t* info_c;     /*!< MQTT Client info structure */

    lwesp_cayenne_msg_t msg;                    /*!< Received data message */

    lwesp_cayenne_evt_t evt;                    /*!< Event handle */
    lwesp_cayenne_evt_fn evt_fn;                /*!< Event callback function */

    lwesp_sys_thread_t thread;                  /*!< Cayenne thread handle */
    lwesp_sys_sem_t sem;                        /*!< Sync semaphore handle */
} lwesp_cayenne_t;

lwespr_t    lwesp_cayenne_create(lwesp_cayenne_t* c, const lwesp_mqtt_client_info_t* client_info, lwesp_cayenne_evt_fn evt_fn);
lwespr_t    lwesp_cayenne_subscribe(lwesp_cayenne_t* c, lwesp_cayenne_topic_t topic, uint16_t channel);
lwespr_t    lwesp_cayenne_publish_data(lwesp_cayenne_t* c, lwesp_cayenne_topic_t topic, uint16_t channel, const char* type, const char* unit, const char* data);
lwespr_t    lwesp_cayenne_publish_float(lwesp_cayenne_t* c, lwesp_cayenne_topic_t topic, uint16_t channel, const char* type, const char* unit, float f);

lwespr_t    lwesp_cayenne_publish_response(lwesp_cayenne_t* c, lwesp_cayenne_msg_t* msg, lwesp_cayenne_rlwesp_t resp, const char* message);

#include "lwesp/apps/lwesp_cayenne_evt.h"

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_HDR_APP_CAYENNE_H */
