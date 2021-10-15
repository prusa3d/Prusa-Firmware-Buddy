/**
 * \file            esp_cayenne.c
 * \brief           MQTT client for Cayenne
 */

/*
 * Copyright (c) 2019 Tilen MAJERLE
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
 * This file is part of ESP-AT library.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 * Version:         $_version_$
 */
#include "esp/apps/esp_cayenne.h"
#include "esp/esp_mem.h"
#include "esp/esp_pbuf.h"

//#error "This driver is not ready-to-use yet and shall not be used in final product"

/* Tracing debug message */
#define ESP_CFG_DBG_CAYENNE_TRACE               (ESP_CFG_DBG_CAYENNE | ESP_DBG_TYPE_TRACE)
#define ESP_CFG_DBG_CAYENNE_TRACE_WARNING       (ESP_CFG_DBG_CAYENNE | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_WARNING)
#define ESP_CFG_DBG_CAYENNE_TRACE_SEVERE        (ESP_CFG_DBG_CAYENNE | ESP_DBG_TYPE_TRACE | ESP_DBG_LVL_SEVERE)

#define ESP_CAYENNE_API_VERSION_LEN             (sizeof(ESP_CAYENNE_API_VERSION) - 1)

#define CHECK_FORWARDSLASH_AND_FORWARD(str)     do { if ((str) == NULL || *(str) != '/') { return espERR; } (str)++; } while (0)

#if !ESP_CFG_NETCONN || !ESP_CFG_MODE_STATION
#error "Netconn and station mode must be enabled!"
#endif /* !ESP_CFG_NETCONN || !ESP_CFG_MODE_STATION */

/**
 * \brief           Topic type and string key-value pair structure
 */
typedef struct {
    esp_cayenne_topic_t topic;                  /*!< Topic name */
    const char* str;                            /*!< Topic string */
} topic_cmd_str_pair_t;

/**
 * \brief           List of key-value pairs for topic type and string
 */
const static topic_cmd_str_pair_t
topic_cmd_str_pairs[] = {
    { ESP_CAYENNE_TOPIC_DATA, "data" },
    { ESP_CAYENNE_TOPIC_COMMAND, "cmd" },
    { ESP_CAYENNE_TOPIC_CONFIG, "conf" },
    { ESP_CAYENNE_TOPIC_RESPONSE, "response" },
    { ESP_CAYENNE_TOPIC_SYS_MODEL, "sys/model" },
    { ESP_CAYENNE_TOPIC_SYS_VERSION, "sys/version" },
    { ESP_CAYENNE_TOPIC_SYS_CPU_MODEL, "sys/cpu/model" },
    { ESP_CAYENNE_TOPIC_SYS_CPU_SPEED, "sys/cpu/speed" },
    { ESP_CAYENNE_TOPIC_DIGITAL, "digital" },
    { ESP_CAYENNE_TOPIC_DIGITAL_COMMAND, "digital-cmd" },
    { ESP_CAYENNE_TOPIC_DIGITAL_CONFIG, "digital-conf" },
    { ESP_CAYENNE_TOPIC_ANALOG, "analog" },
    { ESP_CAYENNE_TOPIC_ANALOG_COMMAND, "analog-cmd" },
    { ESP_CAYENNE_TOPIC_ANALOG_CONFIG, "analog-conf" }
};

/**
 * \brief           Protection mutex
 */
static esp_sys_mutex_t
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
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
parse_topic(esp_cayenne_t* c, esp_mqtt_client_api_buf_p buf) {
    esp_cayenne_msg_t* msg;
    const char* topic;
    size_t len, i;

    ESP_ASSERT("c != NULL", c != NULL);
    ESP_ASSERT("buf != NULL && buf->topic != NULL", buf != NULL && buf->topic != NULL);

    msg = &c->msg;                              /* Get message handle */
    topic = (void *)buf->topic;                 /* Get topic data */

    ESP_DEBUGF(ESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Parsing received topic: %s\r\n", topic);

    /* Topic starts with API version */
    if (strncmp(topic, ESP_CAYENNE_API_VERSION, ESP_CAYENNE_API_VERSION_LEN)) {
        return espERR;
    }
    topic += ESP_CAYENNE_API_VERSION_LEN;
    CHECK_FORWARDSLASH_AND_FORWARD(topic);

    /* Check username */
    len = strlen(c->info_c->user);
    if (strncmp(topic, c->info_c->user, len)) {
        return espERR;
    }
    topic += len;

    /* Check for /things/ */
    len = sizeof("/things/") - 1;
    if (strncmp(topic, "/things/", len)) {
        return espERR;
    }
    topic += len;

    /* Check client ID */
    len = strlen(c->info_c->id);
    if (strncmp(topic, c->info_c->id, len)) {
        return espERR;
    }
    topic += len;
    CHECK_FORWARDSLASH_AND_FORWARD(topic);

    /* Now parse topic string */
    msg->topic = ESP_CAYENNE_TOPIC_END;
    for (i = 0; i < ESP_ARRAYSIZE(topic_cmd_str_pairs); i++) {
        len = strlen(topic_cmd_str_pairs[i].str);
        if (!strncmp(topic_cmd_str_pairs[i].str, topic, len)) {
            msg->topic = topic_cmd_str_pairs[i].topic;
            topic += len;
            break;
        }
    }
    /* This means error, no topic found */
    if (i == ESP_ARRAYSIZE(topic_cmd_str_pairs)) {
        return espERR;
    }

    /* Parse channel */
    msg->channel = ESP_CAYENNE_NO_CHANNEL;
    if (*topic == '/') {
        topic++;
        if (*topic == '+') {
            msg->channel = ESP_CAYENNE_ALL_CHANNELS;
        } else if (*topic == '#') {
            msg->channel = ESP_CAYENNE_ALL_CHANNELS;
        } else if (*topic >= '0' && *topic <= '9') {
            msg->channel = 0;
            while (*topic >= '0' && *topic <= '9') {
                msg->channel = 10 * msg->channel + *topic - '0';
                topic++;
            }
        } else {
            return espERR;
        }
    }

    return espOK;
}

/**
 * \brief           Parse received data from MQTT channel
 * \param[in]       c: Cayenne handle
 * \param[in]       buf: MQTT buffer with received data
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
parse_payload(esp_cayenne_t* c, esp_mqtt_client_api_buf_p buf) {
    esp_cayenne_msg_t* msg;
    char* payload;

    ESP_ASSERT("c != NULL", c != NULL);
    ESP_ASSERT("buf != NULL", buf != NULL);

    msg = &c->msg;                              /* Get message handle */
    payload = (void *)buf->payload;             /* Get payload data */

    ESP_DEBUGF(ESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Parsing received payload\r\n");

    msg->seq = NULL;                            /* Reset sequence string */

    /* Parse topic format here */
    switch (msg->topic) {
        case ESP_CAYENNE_TOPIC_DATA: {
            ESP_DEBUGF(ESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] TOPIC DATA: %*s\r\n", (int)buf->payload_len, (const char *)buf->payload);
            /* Parse data with '=' separator */
            break;
        }
        case ESP_CAYENNE_TOPIC_COMMAND:
        case ESP_CAYENNE_TOPIC_ANALOG_COMMAND:
        case ESP_CAYENNE_TOPIC_DIGITAL_COMMAND: {
            /* Parsing "sequence,value" */
            char* comm = strchr(payload, ',');
            if (comm != NULL) {
                *comm = 0;
                msg->seq = payload;
                msg->values[0].key = NULL;
                msg->values[0].value = comm + 1;
                msg->values_count = 1;
            } else {
                return espERR;
            }
            /* Here parse sequence,value */
            break;
        }
        case ESP_CAYENNE_TOPIC_ANALOG: {
            ESP_DEBUGF(ESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] TOPIC ANALOG: %*s\r\n", (int)buf->payload_len, (const char *)buf->payload);
            /* Here parse type,value */
        }
        default:
            break;
    }

    return espOK;
}

/**
 * \brief           Build topic string based on input parameters
 * \param[in]       topic_str: Output variable for created topic
 * \param[in]       topic_str_len: Length of topic_str param including NULL termination
 * \param[in]       username: MQTT username
 * \param[in]       client_id: MQTT client id
 * \param[in]       topic: Cayenne topic
 * \param[in]       channel: Cayenne channel
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
build_topic(char* topic_str, size_t topic_str_len, const char* username,
            const char* client_id, esp_cayenne_topic_t topic, uint16_t channel) {
    size_t rem_len;
    char ch_token[6];

    ESP_ASSERT("topic_str != NULL", topic_str != NULL);
    ESP_ASSERT("username != NULL", username != NULL);
    ESP_ASSERT("client_id != NULL", client_id != NULL);
    ESP_ASSERT("topic < ESP_CAYENNE_TOPIC_END", topic < ESP_CAYENNE_TOPIC_END);

    /* Assert for basic part without topic */
    ESP_ASSERT("topic_str_len > string_length", topic_str_len > (strlen(ESP_CAYENNE_API_VERSION) + strlen(username) + strlen(client_id) + 11));

    topic_str[0] = 0;

    /* Base part */
    strcat(topic_str, ESP_CAYENNE_API_VERSION);
    strcat(topic_str, "/");
    strcat(topic_str, username);
    strcat(topic_str, "/things/");
    strcat(topic_str, client_id);
    strcat(topic_str, "/");
    rem_len = topic_str_len - strlen(topic_str) - 1;

    /* Topic string */
    for (size_t i = 0; i < ESP_ARRAYSIZE(topic_cmd_str_pairs); i++) {
        if (topic == topic_cmd_str_pairs[i].topic) {
            ESP_ASSERT("strlen(topic_cmd_str_pairs[i].str) <= rem_len", strlen(topic_cmd_str_pairs[i].str) <= rem_len);
            strcat(topic_str, topic_cmd_str_pairs[i].str);
            break;
        }
    }
    rem_len = topic_str_len - strlen(topic_str) - 1;

    /* Channel */
    if (channel != ESP_CAYENNE_NO_CHANNEL) {
        if (channel == ESP_CAYENNE_ALL_CHANNELS) {
            ESP_ASSERT("rem_len >= 2", rem_len >= 2);
            strcat(topic_str, "/+");
        } else {
            esp_u16_to_str(channel, ch_token);
            strcat(topic_str, "/");
            ESP_ASSERT("strlen(ch_token) <= rem_len", strlen(ch_token) <= rem_len - 1);
            strcat(topic_str, ch_token);
        }
    }

    ESP_DEBUGF(ESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Topic: %s\r\n", topic_name);

    return espOK;
}

/**
 * \brief           Cayenne thread
 * \param[in]       arg: Thread argument. Pointer to \ref esp_mqtt_client_cayenne_t structure
 */
static void
mqtt_thread(void * const arg) {
    esp_cayenne_t* c = arg;
    esp_mqtt_conn_status_t status;
    esp_mqtt_client_api_buf_p buf;
    espr_t res;

    /* Create sync mutex for multiple cayenne accesses */
    esp_core_lock();
    if (!esp_sys_mutex_isvalid(&prot_mutex)) {
        esp_sys_mutex_create(&prot_mutex);

        ESP_DEBUGW(ESP_CFG_DBG_CAYENNE_TRACE, esp_sys_mutex_isvalid(&prot_mutex), "[CAYENNE] New mutex created\r\n");
        ESP_DEBUGW(ESP_CFG_DBG_CAYENNE_TRACE_SEVERE, !esp_sys_mutex_isvalid(&prot_mutex), "[CAYENNE] Cannot create mutex\r\n");
    }
    esp_core_unlock();

    /* Release calling thread now */
    if (esp_sys_sem_isvalid(&c->sem)) {
        esp_sys_sem_release(&c->sem);
    }

    while (1) {
        /* Device must be connected to access point */
        while (!esp_sta_has_ip()) {
            esp_delay(1000);
        }

        /* Connect to API server */
        status = esp_mqtt_client_api_connect(c->api_c, ESP_CAYENNE_HOST, ESP_CAYENNE_PORT, c->info_c);
        if (status != ESP_MQTT_CONN_STATUS_ACCEPTED) {
            /* Find out reason not to be accepted and decide accordingly */
        } else {
            /* Notify user */
            c->evt.type = ESP_CAYENNE_EVT_CONNECT;
            c->evt_fn(c, &c->evt);

            /* We are connected and ready to subscribe/publish/receive packets */
            esp_cayenne_subscribe(c, ESP_CAYENNE_TOPIC_COMMAND, ESP_CAYENNE_ALL_CHANNELS);

            /* Unlimited loop */
            while (1) {
                /* Wait for new received packet or connection closed */
                res = esp_mqtt_client_api_receive(c->api_c, &buf, 0);

                if (res == espOK) {
                    if (buf != NULL) {
                        ESP_DEBUGF(ESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Packet received\r\nTopic: %s\r\nData: %s\r\n\r\n", buf->topic, buf->payload);

                        /* Parse received topic and payload */
                        if (parse_topic(c, buf) == espOK && parse_payload(c, buf) == espOK) {
                            ESP_DEBUGF(ESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Topic and payload parsed!\r\n");
                            ESP_DEBUGF(ESP_CFG_DBG_CAYENNE_TRACE, "[CAYENNE] Channel: %d, Sequence: %s, Key: %s, Value: %s\r\n",
                                (int)c->msg.channel, c->msg.seq, c->msg.values[0].key, c->msg.values[0].value
                            );

                            /* Send notification to user */
                            c->evt.type = ESP_CAYENNE_EVT_DATA;
                            c->evt.evt.data.msg = &c->msg;
                            c->evt_fn(c, &c->evt);
                        }

                        esp_mqtt_client_api_buf_free(buf);
                        buf = NULL;
                    }
                } else if (res == espCLOSED) {
                    /* Connection closed at this point */
                    c->evt.type = ESP_CAYENNE_EVT_DISCONNECT;
                    c->evt_fn(c, &c->evt);
                    break;
                }
            }
        }
    }

    /* Clean resources */
    if (esp_sys_sem_isvalid(&c->sem)) {
        esp_sys_sem_delete(&c->sem);
        esp_sys_sem_invalid(&c->sem);
    }

    esp_sys_thread_terminate(NULL);             /* Terminate thread */
}

/**
 * \brief           Create new instance of cayenne MQTT connection
 * \note            Each call to this functions starts new thread for async receive processing.
 *                  Function will block until thread is created and successfully started
 * \param[in]       c: Cayenne empty handle
 * \param[in]       client_info: MQTT client info with username, password and id
 * \param[in]       evt_fn: Event function
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_cayenne_create(esp_cayenne_t* c, const esp_mqtt_client_info_t* client_info, esp_cayenne_evt_fn evt_fn) {
    ESP_ASSERT("c != NULL", c != NULL);
    ESP_ASSERT("client_info != NULL", client_info != NULL);
    ESP_ASSERT("evt_fn != NULL", evt_fn != NULL);

    c->api_c = esp_mqtt_client_api_new(256, 256);
    c->info_c = client_info;
    c->evt_fn = evt_fn;
    if (c->api_c == NULL) {
        return espERRMEM;
    }

    /* Create semaphore */
    if (!esp_sys_sem_create(&c->sem, 1)) {
        ESP_DEBUGF(ESP_CFG_DBG_CAYENNE_TRACE_SEVERE, "[CAYENNE] Cannot create semaphore\r\n");
        return espERRMEM;
    }

    /* Create and wait for thread to start */
    esp_sys_sem_wait(&c->sem, 0);
    if (!esp_sys_thread_create(&c->thread, "mqtt_cayenne", mqtt_thread, c, ESP_SYS_THREAD_SS, ESP_SYS_THREAD_PRIO)) {
        ESP_DEBUGF(ESP_CFG_DBG_CAYENNE_TRACE_SEVERE, "[CAYENNE] Cannot create new thread\r\n");
        esp_sys_sem_release(&c->sem);
        esp_sys_sem_delete(&c->sem);
        esp_sys_sem_invalid(&c->sem);
        esp_mem_free_s((void **)&c->api_c);
        c->info_c = NULL;
        return espERRMEM;
    }
    esp_sys_sem_wait(&c->sem, 0);
    esp_sys_sem_release(&c->sem);

    return espOK;
}

/**
 * \brief           Subscribe to cayenne based topics and channels
 * \param[in]       c: Cayenne handle
 * \param[in]       topic: Cayenne topic
 * \param[in]       channel: Optional channel number.
 *                      Use \ref ESP_CAYENNE_NO_CHANNEL when channel is not needed
 *                      or ESP_CAYENNE_ALL_CHANNELS to subscribe to all channels
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_cayenne_subscribe(esp_cayenne_t* c, esp_cayenne_topic_t topic, uint16_t channel) {
    espr_t res;

    ESP_ASSERT("c != NULL", c != NULL);

    esp_sys_mutex_lock(&prot_mutex);
    build_topic(topic_name, sizeof(topic_name), c->info_c->user, c->info_c->id, topic, channel);
    res = esp_mqtt_client_api_subscribe(c->api_c, topic_name, ESP_MQTT_QOS_EXACTLY_ONCE);

    ESP_DEBUGW(ESP_CFG_DBG_CAYENNE_TRACE, res == espOK,
        "[CAYENNE] Subscribed to topic %s\r\n", topic_name);
    ESP_DEBUGW(ESP_CFG_DBG_CAYENNE_TRACE, res != espOK,
        "[CAYENNE] Cannot subscribe to topic %s, error code: %d\r\n", topic_name, (int)res);

    esp_sys_mutex_unlock(&prot_mutex);

    return res;
}

/**
 * \brief           Unsubscribe from cayenne based topics and channels
 * \param[in]       c: Cayenne handle
 * \param[in]       topic: Cayenne topic
 * \param[in]       channel: Optional channel number.
 *                      Use \ref ESP_CAYENNE_NO_CHANNEL when channel is not needed
 *                      or ESP_CAYENNE_ALL_CHANNELS to unsubscribe from all channels
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_cayenne_unsubscribe(esp_cayenne_t* c, esp_cayenne_topic_t topic, uint16_t channel) {
    espr_t res;

    ESP_ASSERT("c != NULL", c != NULL);

    esp_sys_mutex_lock(&prot_mutex);
    build_topic(topic_name, sizeof(topic_name), c->info_c->user, c->info_c->id, topic, channel);
    res = esp_mqtt_client_api_unsubscribe(c->api_c, topic_name);

    ESP_DEBUGW(ESP_CFG_DBG_CAYENNE_TRACE, res == espOK,
        "[CAYENNE] Unsubscribed from topic %s\r\n", topic_name);
    ESP_DEBUGW(ESP_CFG_DBG_CAYENNE_TRACE, res != espOK,
        "[CAYENNE] Cannot unsubscribe from topic %s, error code: %d\r\n", topic_name, (int)res);

    esp_sys_mutex_unlock(&prot_mutex);

    return res;
}

espr_t
esp_cayenne_publish_data(esp_cayenne_t* c, esp_cayenne_topic_t topic, uint16_t channel,
                        const char* type, const char* unit, const char* data) {
    espr_t res = espOK;

    esp_sys_mutex_lock(&prot_mutex);
    if ((res = build_topic(topic_name, sizeof(topic_name), c->info_c->user, c->info_c->id, topic, channel)) != espOK) {
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

    res = esp_mqtt_client_api_publish(c->api_c, topic_name, payload_data, strlen(payload_data), ESP_MQTT_QOS_AT_LEAST_ONCE, 1);
exit:
    esp_sys_mutex_unlock(&prot_mutex);
    return res;
}

/**
 * \brief           Publish response message to command
 * \param[in]       c: Cayenne handle
 * \param[in]       msg: Received message with command topic
 * \param[in]       resp: Response type, either `OK` or `ERROR`
 * \param[in]       message: Message text in case of error to be displayed to Cayenne dashboard
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
espr_t
esp_cayenne_publish_response(esp_cayenne_t* c, esp_cayenne_msg_t* msg, esp_cayenne_resp_t resp, const char* message) {
    espr_t res = espOK;
    size_t len, msg_len;

    ESP_ASSERT("c != NULL", c != NULL);
    ESP_ASSERT("msg != NULL && msg->seq != NULL", msg != NULL && msg->seq != NULL);

    esp_sys_mutex_lock(&prot_mutex);
    if ((res = build_topic(topic_name, sizeof(topic_name), c->info_c->user, c->info_c->id, ESP_CAYENNE_TOPIC_RESPONSE, ESP_CAYENNE_NO_CHANNEL)) != espOK) {
        goto exit;
    }
    payload_data[0] = 0;
    strcat(payload_data, resp == ESP_CAYENNE_RESP_OK ? "ok," : "error,");
    strcat(payload_data, msg->seq);
    if (resp != ESP_CAYENNE_RESP_OK) {
        strcat(payload_data, "=");
        len = strlen(payload_data);
        msg_len = strlen(message);
        if (msg_len > (sizeof(payload_data) - 1 - len)) {
            msg_len = sizeof(payload_data) - 1 - len;
        }
        strncpy(&payload_data[len], message, msg_len);
        payload_data[len + msg_len] = 0;
    }
    res = esp_mqtt_client_api_publish(c->api_c, topic_name, payload_data, strlen(payload_data), ESP_MQTT_QOS_AT_LEAST_ONCE, 1);
exit:
    esp_sys_mutex_unlock(&prot_mutex);
    return res;
}
