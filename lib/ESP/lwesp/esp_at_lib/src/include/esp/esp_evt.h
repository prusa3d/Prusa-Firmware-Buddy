/**
 * \file            esp_evt.h
 * \brief           Event helper functions
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
#ifndef ESP_HDR_EVT_H
#define ESP_HDR_EVT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"

/**
 * \ingroup         ESP
 * \defgroup        ESP_EVT Events management
 * \brief           Event helper functions
 * \{
 */

espr_t          esp_evt_register(esp_evt_fn fn);
espr_t          esp_evt_unregister(esp_evt_fn fn);
esp_evt_type_t  esp_evt_get_type(esp_evt_t* cc);

/**
 * \anchor          ESP_EVT_RESET_DETECTED
 * \name            Reset detected
 * \brief           Event helper functions for \ref ESP_EVT_RESET_DETECTED event
 */

uint8_t     esp_evt_reset_detected_is_forced(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_RESET
 * \name            Reset event
 * \brief           Event helper functions for \ref ESP_EVT_RESET event
 */

espr_t      esp_evt_reset_get_result(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_RESTORE
 * \name            Restore event
 * \brief           Event helper functions for \ref ESP_EVT_RESTORE event
 */

espr_t      esp_evt_restore_get_result(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_AP_IP_STA
 * \name            Access point or station IP or MAC
 * \brief           Event helper functions for \ref ESP_EVT_AP_IP_STA event
 */

esp_mac_t*  esp_evt_ap_ip_sta_get_mac(esp_evt_t* cc);
esp_ip_t*   esp_evt_ap_ip_sta_get_ip(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_AP_CONNECTED_STA
 * \name            Connected station to access point
 * \brief           Event helper functions for \ref ESP_EVT_AP_CONNECTED_STA event
 */

esp_mac_t*  esp_evt_ap_connected_sta_get_mac(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_AP_DISCONNECTED_STA
 * \name            Disconnected station from access point
 * \brief           Event helper functions for \ref ESP_EVT_AP_DISCONNECTED_STA event
 */

esp_mac_t*  esp_evt_ap_disconnected_sta_get_mac(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_CONN_RECV
 * \name            Connection data received
 * \brief           Event helper functions for \ref ESP_EVT_CONN_RECV event
 */

esp_pbuf_p  esp_evt_conn_recv_get_buff(esp_evt_t* cc);
esp_conn_p  esp_evt_conn_recv_get_conn(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_CONN_SEND
 * \name            Connection data send
 * \brief           Event helper functions for \ref ESP_EVT_CONN_SEND event
 */

esp_conn_p  esp_evt_conn_send_get_conn(esp_evt_t* cc);
size_t      esp_evt_conn_send_get_length(esp_evt_t* cc);
espr_t      esp_evt_conn_send_get_result(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_CONN_ACTIVE
 * \name            Connection active
 * \brief           Event helper functions for \ref ESP_EVT_CONN_ACTIVE event
 */

esp_conn_p  esp_evt_conn_active_get_conn(esp_evt_t* cc);
uint8_t     esp_evt_conn_active_is_client(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_CONN_CLOSE
 * \name            Connection close event
 * \brief           Event helper functions for \ref ESP_EVT_CONN_CLOSE event
 */

esp_conn_p  esp_evt_conn_close_get_conn(esp_evt_t* cc);
uint8_t     esp_evt_conn_close_is_client(esp_evt_t* cc);
uint8_t     esp_evt_conn_close_is_forced(esp_evt_t* cc);
espr_t      esp_evt_conn_close_get_result(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_CONN_POLL
 * \name            Connection poll
 * \brief           Event helper functions for \ref ESP_EVT_CONN_POLL event
 */

esp_conn_p  esp_evt_conn_poll_get_conn(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_CONN_ERROR
 * \name            Connection error
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
 * \anchor          ESP_EVT_STA_LIST_AP
 * \name            List access points
 * \brief           Event helper functions for \ref ESP_EVT_STA_LIST_AP event
 */

espr_t      esp_evt_sta_list_ap_get_result(esp_evt_t* cc);
esp_ap_t*   esp_evt_sta_list_ap_get_aps(esp_evt_t* cc);
size_t      esp_evt_sta_list_ap_get_length(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_STA_JOIN_AP
 * \name            Join access point
 * \brief           Event helper functions for \ref ESP_EVT_STA_JOIN_AP event
 */

espr_t      esp_evt_sta_join_ap_get_result(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_STA_INFO_AP
 * \name            Get access point info
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
 * \anchor          ESP_EVT_DNS_HOSTBYNAME
 * \name            Get host address by name
 * \brief           Event helper functions for \ref ESP_EVT_DNS_HOSTBYNAME event
 */

espr_t      esp_evt_dns_hostbyname_get_result(esp_evt_t* cc);
const char* esp_evt_dns_hostbyname_get_host(esp_evt_t* cc);
esp_ip_t*   esp_evt_dns_hostbyname_get_ip(esp_evt_t* cc);

/**
 * \}
 */

/**
 * \anchor          ESP_EVT_PING
 * \name            Ping
 * \brief           Event helper functions for \ref ESP_EVT_PING event
 */

espr_t      esp_evt_ping_get_result(esp_evt_t* cc);
const char* esp_evt_ping_get_host(esp_evt_t* cc);
uint32_t    esp_evt_ping_get_time(esp_evt_t* cc);


/**
 * \}
 */

/**
 * \anchor          ESP_EVT_SERVER
 * \name            Server
 * \brief           Event helper functions for \ref ESP_EVT_SERVER event
 */

espr_t      esp_evt_server_get_result(esp_evt_t* cc);
esp_port_t  esp_evt_server_get_port(esp_evt_t* cc);
uint8_t     esp_evt_server_is_enable(esp_evt_t* cc);

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
