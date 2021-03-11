/**
 * \file            lwesp_cayenne.c
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
#include "lwesp/apps/lwesp_cayenne.h"
#include "lwesp/lwesp_mem.h"
#include "lwesp/lwesp_pbuf.h"

//#error "This driver is not ready-to-use yet and shall not be used in final product"

/* Tracing debug message */
#define LWESP_CFG_DBG_CAYENNE_TRACE               (LWESP_CFG_DBG_CAYENNE | LWESP_DBG_TYPE_TRACE)
#define LWESP_CFG_DBG_CAYENNE_TRACE_WARNING       (LWESP_CFG_DBG_CAYENNE | LWESP_DBG_TYPE_TRACE | LWESP_DBG_LVL_WARNING)
#define LWESP_CFG_DBG_CAYENNE_TRACE_SEVERE        (LWESP_CFG_DBG_CAYENNE | LWESP_DBG_TYPE_TRACE | LWESP_DBG_LVL_SEVERE)

#define LWESP_CAYENNE_API_VERSION_LEN             (sizeof(LWESP_CAYENNE_API_VERSION) - 1)

#define CHECK_FORWARDSLASH_AND_FORWARD(str)     do { if ((str) == NULL || *(str) != '/') { return lwespERR; } ++(str); } while (0)

#if !LWESP_CFG_NETCONN || !LWESP_CFG_MODE_STATION
#error "Netconn and station mode must be enabled!"
#endif /* !LWESP_CFG_NETCONN || !LWESP_CFG_MODE_STATION */

/**
 * \brief           Topic type and string key-value pair structure
 */
typedef struct {
    lwesp_cayenne_topic_t topic;                /*!< Topic name */
    const char* str;                            /*!< Topic string */
} topic_cmd_str_pair_t;

/**
 * \brief           List of key-value pairs for topic type and string
 */
const static topic_cmd_str_pair_t
topic_cmd_str_pairs[] = {
    { LWESP_CAYENNE_TOPIC_DATA, "data" },
    { LWESP_CAYENNE_TOPIC_COMMAND, "cmd" },
    { LWESP_CAYENNE_TOPIC_CONFIG, "conf" },
    { LWESP_CAYENNE_TOPIC_RESPONSE, "response" },
    { LWESP_CAYENNE_TOPIC_SYS_MODEL, "sys/model" },
    { LWESP_CAYENNE_TOPIC_SYS_VERSION, "sys/version" },
    { LWESP_CAYENNE_TOPIC_SYS_CPU_MODEL, "sys/cpu/model" },
    { LWESP_CAYENNE_TOPIC_SYS_CPU_SPEED, "sys/cpu/speed" },
    { LWESP_CAYENNE_TOPIC_DIGITAL, "digital" },
    { LWESP_CAYENNE_TOPIC_DIGITAL_COMMAND, "digital-cmd" },
    { LWESP_CAYENNE_TOPIC_DIGITAL_CONFIG, "digital-conf" },
    { LWESP_CAYENNE_TOPIC_ANALOG, "analog" },
    { LWESP_CAYENNE_TOPIC_ANALOG_COMMAND, "analog-cmd" },
    { LWESP_CAYENNE_TOPIC_ANALOG_CONFIG, "analog-conf" }
};

/**
 * \brief           Protection mutex
 */
static lwesp_sys_mutex_t
prot_mutex;

/**
 * \brief           Topic name for publish/subscribe
 */
static char
topic_name[256];

/**
 * \brief           Payload data
 */
static char
payload_data[128];

/**
 * \brief           Parse received topic string
 * \param[in]       c: Cayenne handle
 * \param[in]       buf: MQTT buffer with received data
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
parse_topic(lwesp_cayenne_t* c, lwesp_mqtt_client_api_buf_p buf) {
    lwesp_cayenne_msg_t* msg;
    const char* topic;
    size_t len, i;

    LWESP_ASSERT("c != NULL", c != NULL);
    LWESP_ASSERT("buf != NULL && buf->topic != NULL", buf != NULL && buf->topic != NULL);

    msg = &c->msg;                              /* Get message handle */
    topic = (void*)buf->topic;                  /* Get topic data */

    LWESP_DEBUGF(LWESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Parsing received topic: %s\r\n", topic);

    /* Topic starts with API version */
    if (strncmp(topic, LWESP_CAYENNE_API_VERSION, LWESP_CAYENNE_API_VERSION_LEN)) {
        return lwespERR;
    }
    topic += LWESP_CAYENNE_API_VERSION_LEN;
    CHECK_FORWARDSLASH_AND_FORWARD(topic);

    /* Check username */
    len = strlen(c->info_c->user);
    if (strncmp(topic, c->info_c->user, len)) {
        return lwespERR;
    }
    topic += len;

    /* Check for /things/ */
    len = sizeof("/things/") - 1;
    if (strncmp(topic, "/things/", len)) {
        return lwespERR;
    }
    topic += len;

    /* Check client ID */
    len = strlen(c->info_c->id);
    if (strncmp(topic, c->info_c->id, len)) {
        return lwespERR;
    }
    topic += len;
    CHECK_FORWARDSLASH_AND_FORWARD(topic);

    /* Now parse topic string */
    msg->topic = LWESP_CAYENNE_TOPIC_END;
    for (i = 0; i < LWESP_ARRAYSIZE(topic_cmd_str_pairs); ++i) {
        len = strlen(topic_cmd_str_pairs[i].str);
        if (!strncmp(topic_cmd_str_pairs[i].str, topic, len)) {
            msg->topic = topic_cmd_str_pairs[i].topic;
            topic += len;
            break;
        }
    }
    /* This means error, no topic found */
    if (i == LWESP_ARRAYSIZE(topic_cmd_str_pairs)) {
        return lwespERR;
    }

    /* Parse channel */
    msg->channel = LWESP_CAYENNE_NO_CHANNEL;
    if (*topic == '/') {
        ++topic;
        if (*topic == '+') {
            msg->channel = LWESP_CAYENNE_ALL_CHANNELS;
        } else if (*topic == '#') {
            msg->channel = LWESP_CAYENNE_ALL_CHANNELS;
        } else if (*topic >= '0' && *topic <= '9') {
            msg->channel = 0;
            while (*topic >= '0' && *topic <= '9') {
                msg->channel = 10 * msg->channel + *topic - '0';
                ++topic;
            }
        } else {
            return lwespERR;
        }
    }

    return lwespOK;
}

/**
 * \brief           Parse received data from MQTT channel
 * \param[in]       c: Cayenne handle
 * \param[in]       buf: MQTT buffer with received data
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
parse_payload(lwesp_cayenne_t* c, lwesp_mqtt_client_api_buf_p buf) {
    lwesp_cayenne_msg_t* msg;
    char* payload;

    LWESP_ASSERT("c != NULL", c != NULL);
    LWESP_ASSERT("buf != NULL", buf != NULL);

    msg = &c->msg;                              /* Get message handle */
    payload = (void*)buf->payload;              /* Get payload data */

    LWESP_DEBUGF(LWESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Parsing received payload\r\n");

    msg->seq = NULL;                            /* Reset sequence string */

    /* Parse topic format here */
    switch (msg->topic) {
        case LWESP_CAYENNE_TOPIC_DATA: {
            LWESP_DEBUGF(LWESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] TOPIC DATA: %*s\r\n", (int)buf->payload_len, (const char*)buf->payload);
            /* Parse data with '=' separator */
            break;
        }
        case LWESP_CAYENNE_TOPIC_COMMAND:
        case LWESP_CAYENNE_TOPIC_ANALOG_COMMAND:
        case LWESP_CAYENNE_TOPIC_DIGITAL_COMMAND: {
            /* Parsing "sequence,value" */
            char* comm = strchr(payload, ',');
            if (comm != NULL) {
                *comm = 0;
                msg->seq = payload;
                msg->values[0].key = NULL;
                msg->values[0].value = comm + 1;
                msg->values_count = 1;
            } else {
                return lwespERR;
            }
            /* Here parse sequence,value */
            break;
        }
        case LWESP_CAYENNE_TOPIC_ANALOG: {
            LWESP_DEBUGF(LWESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] TOPIC ANALOG: %*s\r\n", (int)buf->payload_len, (const char*)buf->payload);
            /* Here parse type,value */
        }
        default:
            break;
    }

    return lwespOK;
}

/**
 * \brief           Build topic string based on input parameters
 * \param[in]       topic_str: Output variable for created topic
 * \param[in]       topic_str_len: Length of topic_str param including NULL termination
 * \param[in]       username: MQTT username
 * \param[in]       client_id: MQTT client id
 * \param[in]       topic: Cayenne topic
 * \param[in]       channel: Cayenne channel
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
static lwespr_t
build_topic(char* topic_str, size_t topic_str_len, const char* username,
            const char* client_id, lwesp_cayenne_topic_t topic, uint16_t channel) {
    size_t rem_len;
    char ch_token[6];

    LWESP_ASSERT("topic_str != NULL", topic_str != NULL);
    LWESP_ASSERT("username != NULL", username != NULL);
    LWESP_ASSERT("client_id != NULL", client_id != NULL);
    LWESP_ASSERT("topic < LWESP_CAYENNE_TOPIC_END", topic < LWESP_CAYENNE_TOPIC_END);

    /* Assert for basic part without topic */
    LWESP_ASSERT("topic_str_len > string_length", topic_str_len > (strlen(LWESP_CAYENNE_API_VERSION) + strlen(username) + strlen(client_id) + 11));

    topic_str[0] = 0;

    /* Base part */
    strcat(topic_str, LWESP_CAYENNE_API_VERSION);
    strcat(topic_str, "/");
    strcat(topic_str, username);
    strcat(topic_str, "/things/");
    strcat(topic_str, client_id);
    strcat(topic_str, "/");
    rem_len = topic_str_len - strlen(topic_str) - 1;

    /* Topic string */
    for (size_t i = 0; i < LWESP_ARRAYSIZE(topic_cmd_str_pairs); ++i) {
        if (topic == topic_cmd_str_pairs[i].topic) {
            LWESP_ASSERT("strlen(topic_cmd_str_pairs[i].str) <= rem_len", strlen(topic_cmd_str_pairs[i].str) <= rem_len);
            strcat(topic_str, topic_cmd_str_pairs[i].str);
            break;
        }
    }
    rem_len = topic_str_len - strlen(topic_str) - 1;

    /* Channel */
    if (channel != LWESP_CAYENNE_NO_CHANNEL) {
        if (channel == LWESP_CAYENNE_ALL_CHANNELS) {
            LWESP_ASSERT("rem_len >= 2", rem_len >= 2);
            strcat(topic_str, "/+");
        } else {
            lwesp_u16_to_str(channel, ch_token);
            strcat(topic_str, "/");
            LWESP_ASSERT("strlen(ch_token) <= rem_len", strlen(ch_token) <= rem_len - 1);
            strcat(topic_str, ch_token);
        }
    }

    LWESP_DEBUGF(LWESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Topic: %s\r\n", topic_name);

    return lwespOK;
}

/**
 * \brief           Cayenne thread
 * \param[in]       arg: Thread argument. Pointer to \ref lwesp_mqtt_client_cayenne_t structure
 */
static void
mqtt_thread(void* const arg) {
    lwesp_cayenne_t* c = arg;
    lwesp_mqtt_conn_status_t status;
    lwesp_mqtt_client_api_buf_p buf;
    lwespr_t res;

    /* Create sync mutex for multiple cayenne accesses */
    lwesp_core_lock();
    if (!lwesp_sys_mutex_isvalid(&prot_mutex)) {
        lwesp_sys_mutex_create(&prot_mutex);

        LWESP_DEBUGW(LWESP_CFG_DBG_CAYENNE_TRACE, lwesp_sys_mutex_isvalid(&prot_mutex), "[CAYENNE] New mutex created\r\n");
        LWESP_DEBUGW(LWESP_CFG_DBG_CAYENNE_TRACE_SEVERE, !lwesp_sys_mutex_isvalid(&prot_mutex), "[CAYENNE] Cannot create mutex\r\n");
    }
    lwesp_core_unlock();

    /* Release calling thread now */
    if (lwesp_sys_sem_isvalid(&c->sem)) {
        lwesp_sys_sem_release(&c->sem);
    }

    while (1) {
        /* Device must be connected to access point */
        while (!lwesp_sta_has_ip()) {
            lwesp_delay(1000);
        }

        /* Connect to API server */
        status = lwesp_mqtt_client_api_connect(c->api_c, LWESP_CAYENNE_HOST, LWESP_CAYENNE_PORT, c->info_c);
        if (status != LWESP_MQTT_CONN_STATUS_ACCEPTED) {
            /* Find out reason not to be accepted and decide accordingly */
        } else {
            /* Notify user */
            c->evt.type = LWESP_CAYENNE_EVT_CONNECT;
            c->evt_fn(c, &c->evt);

            /* We are connected and ready to subscribe/publish/receive packets */
            lwesp_cayenne_subscribe(c, LWESP_CAYENNE_TOPIC_COMMAND, LWESP_CAYENNE_ALL_CHANNELS);

            /* Unlimited loop */
            while (1) {
                /* Wait for new received packet or connection closed */
                res = lwesp_mqtt_client_api_receive(c->api_c, &buf, 0);

                if (res == lwespOK) {
                    if (buf != NULL) {
                        LWESP_DEBUGF(LWESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Packet received\r\nTopic: %s\r\nData: %s\r\n\r\n", buf->topic, buf->payload);

                        /* Parse received topic and payload */
                        if (parse_topic(c, buf) == lwespOK && parse_payload(c, buf) == lwespOK) {
                            LWESP_DEBUGF(LWESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Topic and payload parsed!\r\n");
                            LWESP_DEBUGF(LWESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Channel: %d, Sequence: %s, Key: %s, Value: %s\r\n",
                                       (int)c->msg.channel, c->msg.seq, c->msg.values[0].key, c->msg.values[0].value
                                      );

                            /* Send notification to user */
                            c->evt.type = LWESP_CAYENNE_EVT_DATA;
                            c->evt.evt.data.msg = &c->msg;
                            c->evt_fn(c, &c->evt);
                        }

                        lwesp_mqtt_client_api_buf_free(buf);
                        buf = NULL;
                    }
                } else if (res == lwespCLOSED) {
                    /* Connection closed at this point */
                    c->evt.type = LWESP_CAYENNE_EVT_DISCONNECT;
                    c->evt_fn(c, &c->evt);
                    break;
                }
            }
        }
    }

    /* Clean resources */
    if (lwesp_sys_sem_isvalid(&c->sem)) {
        lwesp_sys_sem_delete(&c->sem);
        lwesp_sys_sem_invalid(&c->sem);
    }

    lwesp_sys_thread_terminate(NULL);           /* Terminate thread */
}

/**
 * \brief           Create new instance of cayenne MQTT connection
 * \note            Each call to this functions starts new thread for async receive processing.
 *                  Function will block until thread is created and successfully started
 * \param[in]       c: Cayenne empty handle
 * \param[in]       client_info: MQTT client info with username, password and id
 * \param[in]       evt_fn: Event function
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
lwespr_t
lwesp_cayenne_create(lwesp_cayenne_t* c, const lwesp_mqtt_client_info_t* client_info, lwesp_cayenne_evt_fn evt_fn) {
    LWESP_ASSERT("c != NULL", c != NULL);
    LWESP_ASSERT("client_info != NULL", client_info != NULL);
    LWESP_ASSERT("evt_fn != NULL", evt_fn != NULL);

    c->api_c = lwesp_mqtt_client_api_new(256, 256);
    c->info_c = client_info;
    c->evt_fn = evt_fn;
    if (c->api_c == NULL) {
        return lwespERRMEM;
    }

    /* Create semaphore */
    if (!lwesp_sys_sem_create(&c->sem, 1)) {
        LWESP_DEBUGF(LWESP_CFG_DBG_CAYENNE_TRACE_SEVERE, "[CAYENNE] Cannot create semaphore\r\n");
        return lwespERRMEM;
    }

    /* Create and wait for thread to start */
    lwesp_sys_sem_wait(&c->sem, 0);
    if (!lwesp_sys_thread_create(&c->thread, "mqtt_cayenne", mqtt_thread, c, LWESP_SYS_THREAD_SS, LWESP_SYS_THREAD_PRIO)) {
        LWESP_DEBUGF(LWESP_CFG_DBG_CAYENNE_TRACE_SEVERE, "[CAYENNE] Cannot create new thread\r\n");
        lwesp_sys_sem_release(&c->sem);
        lwesp_sys_sem_delete(&c->sem);
        lwesp_sys_sem_invalid(&c->sem);
        lwesp_mem_free_s((void**)&c->api_c);
        c->info_c = NULL;
        return lwespERRMEM;
    }
    lwesp_sys_sem_wait(&c->sem, 0);
    lwesp_sys_sem_release(&c->sem);

    return lwespOK;
}

/**
 * \brief           Subscribe to cayenne based topics and channels
 * \param[in]       c: Cayenne handle
 * \param[in]       topic: Cayenne topic
 * \param[in]       channel: Optional channel number.
 *                      Use \ref LWESP_CAYENNE_NO_CHANNEL when channel is not needed
 *                      or LWESP_CAYENNE_ALL_CHANNELS to subscribe to all channels
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
lwespr_t
lwesp_cayenne_subscribe(lwesp_cayenne_t* c, lwesp_cayenne_topic_t topic, uint16_t channel) {
    lwespr_t res;

    LWESP_ASSERT("c != NULL", c != NULL);

    lwesp_sys_mutex_lock(&prot_mutex);
    build_topic(topic_name, sizeof(topic_name), c->info_c->user, c->info_c->id, topic, channel);
    res = lwesp_mqtt_client_api_subscribe(c->api_c, topic_name, LWESP_MQTT_QOS_EXACTLY_ONCE);

    LWESP_DEBUGW(LWESP_CFG_DBG_CAYENNE_TRACE, res == lwespOK,
               "[CAYENNE] Subscribed to topic %s\r\n", topic_name);
    LWESP_DEBUGW(LWESP_CFG_DBG_CAYENNE_TRACE, res != lwespOK,
               "[CAYENNE] Cannot subscribe to topic %s, error code: %d\r\n", topic_name, (int)res);

    lwesp_sys_mutex_unlock(&prot_mutex);

    return res;
}

/**
 * \brief           Unsubscribe from cayenne based topics and channels
 * \param[in]       c: Cayenne handle
 * \param[in]       topic: Cayenne topic
 * \param[in]       channel: Optional channel number.
 *                      Use \ref LWESP_CAYENNE_NO_CHANNEL when channel is not needed
 *                      or LWESP_CAYENNE_ALL_CHANNELS to unsubscribe from all channels
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
lwespr_t
lwesp_cayenne_unsubscribe(lwesp_cayenne_t* c, lwesp_cayenne_topic_t topic, uint16_t channel) {
    lwespr_t res;

    LWESP_ASSERT("c != NULL", c != NULL);

    lwesp_sys_mutex_lock(&prot_mutex);
    build_topic(topic_name, sizeof(topic_name), c->info_c->user, c->info_c->id, topic, channel);
    res = lwesp_mqtt_client_api_unsubscribe(c->api_c, topic_name);

    LWESP_DEBUGW(LWESP_CFG_DBG_CAYENNE_TRACE, res == lwespOK,
               "[CAYENNE] Unsubscribed from topic %s\r\n", topic_name);
    LWESP_DEBUGW(LWESP_CFG_DBG_CAYENNE_TRACE, res != lwespOK,
               "[CAYENNE] Cannot unsubscribe from topic %s, error code: %d\r\n", topic_name, (int)res);

    lwesp_sys_mutex_unlock(&prot_mutex);

    return res;
}

lwespr_t
lwesp_cayenne_publish_data(lwesp_cayenne_t* c, lwesp_cayenne_topic_t topic, uint16_t channel,
                         const char* type, const char* unit, const char* data) {
    lwespr_t res = lwespOK;

    lwesp_sys_mutex_lock(&prot_mutex);
    if ((res = build_topic(topic_name, sizeof(topic_name), c->info_c->user, c->info_c->id, topic, channel)) != lwespOK) {
        goto exit;
    }
    payload_data[0] = 0;
    if (type != NULL) {
        strcat(payload_data, type);
    }
    if (type != NULL && unit != NULL) {
        strcat(payload_data, ",");
    }
    if (unit != NULL) {
        strcat(payload_data, unit);
    }
    if (strlen(payload_data)) {
        strcat(payload_data, "=");
    }
    strcat(payload_data, data);

    res = lwesp_mqtt_client_api_publish(c->api_c, topic_name, payload_data, strlen(payload_data), LWESP_MQTT_QOS_AT_LEAST_ONCE, 1);
exit:
    lwesp_sys_mutex_unlock(&prot_mutex);
    return res;
}

/**
 * \brief           Publish response message to command
 * \param[in]       c: Cayenne handle
 * \param[in]       msg: Received message with command topic
 * \param[in]       resp: Response type, either `OK` or `ERROR`
 * \param[in]       message: Message text in case of error to be displayed to Cayenne dashboard
 * \return          \ref lwespOK on success, member of \ref lwespr_t otherwise
 */
lwespr_t
lwesp_cayenne_publish_response(lwesp_cayenne_t* c, lwesp_cayenne_msg_t* msg, lwesp_cayenne_rlwesp_t resp, const char* message) {
    lwespr_t res = lwespOK;
    size_t len, msg_len;

    LWESP_ASSERT("c != NULL", c != NULL);
    LWESP_ASSERT("msg != NULL && msg->seq != NULL", msg != NULL && msg->seq != NULL);

    lwesp_sys_mutex_lock(&prot_mutex);
    if ((res = build_topic(topic_name, sizeof(topic_name), c->info_c->user, c->info_c->id, LWESP_CAYENNE_TOPIC_RESPONSE, LWESP_CAYENNE_NO_CHANNEL)) != lwespOK) {
        goto exit;
    }
    payload_data[0] = 0;
    strcat(payload_data, resp == LWESP_CAYENNE_RLWESP_OK ? "ok," : "error,");
    strcat(payload_data, msg->seq);
    if (resp != LWESP_CAYENNE_RLWESP_OK) {
        strcat(payload_data, "=");
        len = strlen(payload_data);
        msg_len = strlen(message);
        if (msg_len > (sizeof(payload_data) - 1 - len)) {
            msg_len = sizeof(payload_data) - 1 - len;
        }
        strncpy(&payload_data[len], message, msg_len);
        payload_data[len + msg_len] = 0;
    }
    res = lwesp_mqtt_client_api_publish(c->api_c, topic_name, payload_data, strlen(payload_data), LWESP_MQTT_QOS_AT_LEAST_ONCE, 1);
exit:
    lwesp_sys_mutex_unlock(&prot_mutex);
    return res;
}
