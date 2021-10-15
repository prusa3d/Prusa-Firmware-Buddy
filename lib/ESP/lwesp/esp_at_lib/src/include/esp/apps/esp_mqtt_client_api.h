/**
 * \file            esp_mqtt_client_api.h
 * \brief           MQTT client API
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
#ifndef ESP_HDR_APP_MQTT_CLIENT_API_H
#define ESP_HDR_APP_MQTT_CLIENT_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"
#include "esp/apps/esp_mqtt_client.h"

/**
 * \ingroup         ESP_APPS
 * \defgroup        ESP_APP_MQTT_CLIENT_API MQTT client API
 * \brief           Sequential, single thread MQTT client API
 * \{
 */

/**
 * \brief           MQTT API client structure
 */
struct esp_mqtt_client_api;

/**
 * \brief           MQTT API RX buffer
 */
typedef struct esp_mqtt_client_api_buf {
    char* topic;                                /*!< Topic data */
    size_t topic_len;                           /*!< Topic length */
    uint8_t* payload;                           /*!< Payload data */
    size_t payload_len;                         /*!< Payload length */
    esp_mqtt_qos_t qos;                         /*!< Quality of service */
} esp_mqtt_client_api_buf_t;

/**
 * \brief           Pointer to \ref esp_mqtt_client_api structure
 */
typedef struct esp_mqtt_client_api* esp_mqtt_client_api_p;

/**
 * \brief           Pointer to \ref esp_mqtt_client_api_buf_t structure
 */
typedef struct esp_mqtt_client_api_buf* esp_mqtt_client_api_buf_p;

esp_mqtt_client_api_p   esp_mqtt_client_api_new(size_t tx_buff_len, size_t rx_buff_len);
void                    esp_mqtt_client_api_delete(esp_mqtt_client_api_p client);
esp_mqtt_conn_status_t  esp_mqtt_client_api_connect(esp_mqtt_client_api_p client, const char* host, esp_port_t port, const esp_mqtt_client_info_t* info);
espr_t                  esp_mqtt_client_api_close(esp_mqtt_client_api_p client);
espr_t                  esp_mqtt_client_api_subscribe(esp_mqtt_client_api_p client, const char* topic, esp_mqtt_qos_t qos);
espr_t                  esp_mqtt_client_api_unsubscribe(esp_mqtt_client_api_p client, const char* topic);
espr_t                  esp_mqtt_client_api_publish(esp_mqtt_client_api_p client, const char* topic, const void* data, size_t btw, esp_mqtt_qos_t qos, uint8_t retain);
uint8_t                 esp_mqtt_client_api_is_connected(esp_mqtt_client_api_p client);
espr_t                  esp_mqtt_client_api_receive(esp_mqtt_client_api_p client, esp_mqtt_client_api_buf_p* p, uint32_t timeout);
void                    esp_mqtt_client_api_buf_free(esp_mqtt_client_api_buf_p p);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif

#endif /* ESP_HDR_APP_MQTT_CLIENT_H */
