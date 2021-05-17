/**
 * \file            esp_evt.h
 * \brief           Event helper functions
 */

/*
 * Copyright (c) 2018 Tilen Majerle
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
 */
#ifndef ESP_HDR_EVT_H
#define ESP_HDR_EVT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"

/**
 * \ingroup         ESP
 * \defgroup        ESP_EVT Event helpers
 * \brief           Event helper functions
 * \{
 */

esp_evt_type_t  esp_evt_get_type(esp_evt_t* cc);

/**
 * \name            ESP_EVT_RESET_DETECTED
 * \anchor          ESP_EVT_RESET_DETECTED
 * \brief           Event helper functions for \ref ESP_EVT_RESET_DETECTED event
 */

uint8_t     esp_evt_reset_detected_is_forced(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_RESET
 * \anchor          ESP_EVT_RESET
 * \brief           Event helper functions for \ref ESP_EVT_RESET event
 */

espr_t      esp_evt_reset_get_result(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_RESTORE
 * \anchor          ESP_EVT_RESTORE
 * \brief           Event helper functions for \ref ESP_EVT_RESTORE event
 */

espr_t      esp_evt_restore_get_result(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_AP_IP_STA
 * \anchor          ESP_EVT_AP_IP_STA
 * \brief           Event helper functions for \ref ESP_EVT_AP_IP_STA event
 */

esp_mac_t*  esp_evt_ap_ip_sta_get_mac(esp_evt_t* cc);
esp_ip_t*   esp_evt_ap_ip_sta_get_ip(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_AP_CONNECTED_STA
 * \anchor          ESP_EVT_AP_CONNECTED_STA
 * \brief           Event helper functions for \ref ESP_EVT_AP_CONNECTED_STA event
 */

esp_mac_t*  esp_evt_ap_connected_sta_get_mac(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_AP_DISCONNECTED_STA
 * \anchor          ESP_EVT_AP_DISCONNECTED_STA
 * \brief           Event helper functions for \ref ESP_EVT_AP_DISCONNECTED_STA event
 */

esp_mac_t*  esp_evt_ap_disconnected_sta_get_mac(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_RECV
 * \anchor          ESP_EVT_CONN_RECV
 * \brief           Event helper functions for \ref ESP_EVT_CONN_RECV event
 */

esp_pbuf_p  esp_evt_conn_recv_get_buff(esp_evt_t* cc);
esp_conn_p  esp_evt_conn_recv_get_conn(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_SEND
 * \anchor          ESP_EVT_CONN_SEND
 * \brief           Event helper functions for \ref ESP_EVT_CONN_SEND event
 */

esp_conn_p  esp_evt_conn_send_get_conn(esp_evt_t* cc);
size_t      esp_evt_conn_send_get_length(esp_evt_t* cc);
espr_t      esp_evt_conn_send_get_result(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_ACTIVE
 * \anchor          ESP_EVT_CONN_ACTIVE
 * \brief           Event helper functions for \ref ESP_EVT_CONN_ACTIVE event
 */

esp_conn_p  esp_evt_conn_active_get_conn(esp_evt_t* cc);
uint8_t     esp_evt_conn_active_is_client(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_CLOSED
 * \anchor          ESP_EVT_CONN_CLOSED
 * \brief           Event helper functions for \ref ESP_EVT_CONN_CLOSED event
 */

esp_conn_p  esp_evt_conn_closed_get_conn(esp_evt_t* cc);
uint8_t     esp_evt_conn_closed_is_client(esp_evt_t* cc);
uint8_t     esp_evt_conn_closed_is_forced(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_POLL
 * \anchor          ESP_EVT_CONN_POLL
 * \brief           Event helper functions for \ref ESP_EVT_CONN_POLL event
 */

esp_conn_p  esp_evt_conn_poll_get_conn(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_CONN_ERROR
 * \anchor          ESP_EVT_CONN_ERROR
 * \brief           Event helper functions for \ref ESP_EVT_CONN_ERROR event
 */

espr_t              esp_evt_conn_error_get_error(esp_evt_t* cc);
esp_conn_type_t     esp_evt_conn_error_get_type(esp_evt_t* cc);
const char*         esp_evt_conn_error_get_host(esp_evt_t* cc);
esp_port_t          esp_evt_conn_error_get_port(esp_evt_t* cc);
void*               esp_evt_conn_error_get_arg(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_STA_LIST_AP
 * \anchor          ESP_EVT_STA_LIST_AP
 * \brief           Event helper functions for \ref ESP_EVT_STA_LIST_AP event
 */

espr_t      esp_evt_sta_list_ap_get_result(esp_evt_t* cc);
esp_ap_t*   esp_evt_sta_list_ap_get_aps(esp_evt_t* cc);
size_t      esp_evt_sta_list_ap_get_length(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_STA_JOIN_AP
 * \anchor          ESP_EVT_STA_JOIN_AP
 * \brief           Event helper functions for \ref ESP_EVT_STA_JOIN_AP event
 */

espr_t      esp_evt_sta_join_ap_get_result(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_STA_INFO_AP
 * \anchor          ESP_EVT_STA_INFO_AP
 * \brief           Event helper functions for \ref ESP_EVT_STA_INFO_AP event
 */

espr_t      esp_evt_sta_info_ap_get_result(esp_evt_t* cc);
const char* esp_evt_sta_info_ap_get_ssid(esp_evt_t* cc);
esp_mac_t   esp_evt_sta_info_ap_get_mac(esp_evt_t* cc);
uint8_t     esp_evt_sta_info_ap_get_channel(esp_evt_t* cc);
int16_t     esp_evt_sta_info_ap_get_rssi(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_DNS_HOSTBYNAME
 * \anchor          ESP_EVT_DNS_HOSTBYNAME
 * \brief           Event helper functions for \ref ESP_EVT_DNS_HOSTBYNAME event
 */

espr_t      esp_evt_dns_hostbyname_get_result(esp_evt_t* cc);
const char* esp_evt_dns_hostbyname_get_host(esp_evt_t* cc);
esp_ip_t*   esp_evt_dns_hostbyname_get_ip(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_SERVER
 * \anchor          ESP_EVT_SERVER
 * \brief           Event helper functions for \ref ESP_EVT_SERVER event
 */

espr_t      esp_evt_server_get_result(esp_evt_t* cc);
esp_port_t  esp_evt_server_get_port(esp_evt_t* cc);
uint8_t     esp_evt_server_is_enable(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \name            ESP_EVT_PING
 * \anchor          ESP_EVT_PING
 * \brief           Event helper functions for \ref ESP_EVT_PING event
 */

espr_t      esp_evt_ping_get_result(esp_evt_t* cc);
const char* esp_evt_ping_get_host(esp_evt_t* cc);
uint32_t    esp_evt_ping_get_time(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \}
 */

#ifdef __cplusplus
}
#endif

#endif /* ESP_HDR_EVT_H */
