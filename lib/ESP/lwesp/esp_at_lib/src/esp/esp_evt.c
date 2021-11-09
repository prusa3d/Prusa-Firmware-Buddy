/**
 * \file            esp_evt.c
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
#include "esp/esp_private.h"
#include "esp/esp_evt.h"
#include "esp/esp_mem.h"

/**
 * \brief           Register event function for global (non-connection based) events
 * \param[in]       fn: Callback function to call on specific event
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_evt_register(esp_evt_fn fn) {
    espr_t res = espOK;
    esp_evt_func_t* func, *newFunc;

    ESP_ASSERT("fn != NULL", fn != NULL);

    esp_core_lock();

    /* Check if function already exists on list */
    for (func = esp.evt_func; func != NULL; func = func->next) {
        if (func->fn == fn) {
            res = espERR;
            break;
        }
    }

    if (res == espOK) {
        newFunc = esp_mem_malloc(sizeof(*newFunc));
        if (newFunc != NULL) {
            ESP_MEMSET(newFunc, 0x00, sizeof(*newFunc));
            newFunc->fn = fn;                   /* Set function pointer */
            for (func = esp.evt_func; func != NULL && func->next != NULL; func = func->next) {}
            if (func != NULL) {
                func->next = newFunc;           /* Set new function as next */
                res = espOK;
            } else {
                esp_mem_free_s((void**)& newFunc);
                res = espERRMEM;
            }
        } else {
            res = espERRMEM;
        }
    }
    esp_core_unlock();
    return res;
}

/**
 * \brief           Unregister callback function for global (non-connection based) events
 * \note            Function must be first registered using \ref esp_evt_register
 * \param[in]       fn: Callback function to remove from event list
 * \return          \ref espOK on success, member of \ref espr_t enumeration otherwise
 */
espr_t
esp_evt_unregister(esp_evt_fn fn) {
    esp_evt_func_t* func, *prev;

    ESP_ASSERT("fn != NULL", fn != NULL);

    esp_core_lock();
    for (prev = esp.evt_func, func = esp.evt_func->next; func != NULL; prev = func, func = func->next) {
        if (func->fn == fn) {
            prev->next = func->next;
            esp_mem_free_s((void **)&func);
            break;
        }
    }
    esp_core_unlock();
    return espOK;
}

/**
 * \brief           Get event type
 * \param[in]       cc: Event handle
 * \return          Event type. Member of \ref esp_evt_type_t enumeration
 */
esp_evt_type_t
esp_evt_get_type(esp_evt_t* cc) {
    return cc->type;
}

/**
 * \brief           Check if detected reset was forced by user
 * \param[in]       cc: Event handle
 * \return          `1` if forced by user, `0` otherwise
 */
uint8_t
esp_evt_reset_detected_is_forced(esp_evt_t* cc) {
    return ESP_U8(!!cc->evt.reset_detected.forced);
}

/**
 * \brief           Get reset sequence operation status
 * \param[in]       cc: Event data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_evt_reset_get_result(esp_evt_t* cc) {
    return cc->evt.reset.res;
}

/**
 * \brief           Get restore sequence operation status
 * \param[in]       cc: Event data
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_evt_restore_get_result(esp_evt_t* cc) {
    return cc->evt.restore.res;
}

#if ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__

/**
 * \brief           Get MAC address from station
 * \param[in]       cc: Event handle
 * \return          MAC address
 */
esp_mac_t *
esp_evt_ap_ip_sta_get_mac(esp_evt_t* cc) {
    return cc->evt.ap_ip_sta.mac;
}

/**
 * \brief           Get IP address from station
 * \param[in]       cc: Event handle
 * \return          IP address
 */
esp_ip_t *
esp_evt_ap_ip_sta_get_ip(esp_evt_t* cc) {
    return cc->evt.ap_ip_sta.ip;
}

/**
 * \brief           Get MAC address from connected station
 * \param[in]       cc: Event handle
 * \return          MAC address
 */
esp_mac_t *
esp_evt_ap_connected_sta_get_mac(esp_evt_t* cc) {
    return cc->evt.ap_conn_disconn_sta.mac;
}

/**
 * \brief           Get MAC address from disconnected station
 * \param[in]       cc: Event handle
 * \return          MAC address
 */
esp_mac_t *
esp_evt_ap_disconnected_sta_get_mac(esp_evt_t* cc) {
    return cc->evt.ap_conn_disconn_sta.mac;
}

#endif /* ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */

/**
 * \brief           Get buffer from received data
 * \param[in]       cc: Event handle
 * \return          Buffer handle
 */
esp_pbuf_p
esp_evt_conn_recv_get_buff(esp_evt_t* cc) {
    return cc->evt.conn_data_recv.buff;
}

/**
 * \brief           Get connection handle for receive
 * \param[in]       cc: Event handle
 * \return          Connection handle
 */
esp_conn_p
esp_evt_conn_recv_get_conn(esp_evt_t* cc) {
    return cc->evt.conn_data_recv.conn;
}

/**
 * \brief           Get connection handle for data sent event
 * \param[in]       cc: Event handle
 * \return          Connection handle
 */
esp_conn_p
esp_evt_conn_send_get_conn(esp_evt_t* cc) {
    return cc->evt.conn_data_send.conn;
}

/**
 * \brief           Get number of bytes sent on connection
 * \param[in]       cc: Event handle
 * \return          Number of bytes sent
 */
size_t
esp_evt_conn_send_get_length(esp_evt_t* cc) {
    return cc->evt.conn_data_send.sent;
}

/**
 * \brief           Check if connection send was successful
 * \param[in]       cc: Event handle
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_evt_conn_send_get_result(esp_evt_t* cc) {
    return cc->evt.conn_data_send.res;
}

/**
 * \brief           Get connection handle
 * \param[in]       cc: Event handle
 * \return          Connection handle
 */
esp_conn_p
esp_evt_conn_active_get_conn(esp_evt_t* cc) {
    return cc->evt.conn_active_close.conn;
}

/**
 * \brief           Check if new connection is client
 * \param[in]       cc: Event handle
 * \return          `1` if client, `0` otherwise
 */
uint8_t
esp_evt_conn_active_is_client(esp_evt_t* cc) {
    return ESP_U8(cc->evt.conn_active_close.client > 0);
}

/**
 * \brief           Get connection handle
 * \param[in]       cc: Event handle
 * \return          Connection handle
 */
esp_conn_p
esp_evt_conn_close_get_conn(esp_evt_t* cc) {
    return cc->evt.conn_active_close.conn;
}

/**
 * \brief           Check if just closed connection was client
 * \param[in]       cc: Event handle
 * \return          `1` if client, `0` otherwise
 */
uint8_t
esp_evt_conn_close_is_client(esp_evt_t* cc) {
    return cc->evt.conn_active_close.client;
}

/**
 * \brief           Check if connection close even was forced by user
 * \param[in]       cc: Event handle
 * \return          `1` if forced, `0` otherwise
 */
uint8_t
esp_evt_conn_close_is_forced(esp_evt_t* cc) {
    return cc->evt.conn_active_close.forced;
}

/**
 * \brief           Get connection close event result
 * \param[in]       cc: Event handle
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_evt_conn_close_get_result(esp_evt_t* cc) {
    return cc->evt.conn_active_close.res;
}

/**
 * \brief           Get connection handle
 * \param[in]       cc: Event handle
 * \return          Connection handle
 */
esp_conn_p
esp_evt_conn_poll_get_conn(esp_evt_t* cc) {
    return cc->evt.conn_poll.conn;
}

/**
 * \brief           Get connection error type
 * \param[in]       cc: Event handle
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_evt_conn_error_get_error(esp_evt_t* cc) {
    return cc->evt.conn_error.err;
}

/**
 * \brief           Get connection type
 * \param[in]       cc: Event handle
 * \return          Member of \ref espr_t enumeration
 */
esp_conn_type_t
esp_evt_conn_error_get_type(esp_evt_t* cc) {
    return cc->evt.conn_error.type;
}

/**
 * \brief           Get connection host
 * \param[in]       cc: Event handle
 * \return          Host name for connection
 */
const char *
esp_evt_conn_error_get_host(esp_evt_t* cc) {
    return cc->evt.conn_error.host;
}

/**
 * \brief           Get connection port
 * \param[in]       cc: Event handle
 * \return          Host port number
 */
esp_port_t
esp_evt_conn_error_get_port(esp_evt_t* cc) {
    return cc->evt.conn_error.port;
}

/**
 * \brief           Get user argument
 * \param[in]       cc: Event handle
 * \return          User argument
 */
void *
esp_evt_conn_error_get_arg(esp_evt_t* cc) {
    return cc->evt.conn_error.arg;
}

#if ESP_CFG_MODE_STATION || __DOXYGEN__

/**
 * \brief           Get command success result
 * \param[in]       cc: Event handle
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_evt_sta_list_ap_get_result(esp_evt_t* cc) {
    return cc->evt.sta_list_ap.res;
}

/**
 * \brief           Get access points
 * \param[in]       cc: Event handle
 * \return          Pointer to \ref esp_ap_t with first access point description
 */
esp_ap_t *
esp_evt_sta_list_ap_get_aps(esp_evt_t* cc) {
    return cc->evt.sta_list_ap.aps;
}

/**
 * \brief           Get number of access points found
 * \param[in]       cc: Event handle
 * \return          Number of access points found
 */
size_t
esp_evt_sta_list_ap_get_length(esp_evt_t* cc) {
    return cc->evt.sta_list_ap.len;
}

/**
 * \brief           Get command success result
 * \param[in]       cc: Event handle
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_evt_sta_join_ap_get_result(esp_evt_t* cc) {
    return cc->evt.sta_join_ap.res;
}

/**
 * \brief           Get current AP name
 * \param[in]       cc: Event handle
 * \return          AP name
 */
const char *
esp_evt_sta_info_ap_get_ssid(esp_evt_t* cc) {
    return cc->evt.sta_info_ap.info->ssid;
}

/**
 * \brief           Get current AP MAC address
 * \param[in]       cc: Event handle
 * \return          AP MAC address
 */
esp_mac_t
esp_evt_sta_info_ap_get_mac(esp_evt_t* cc) {
    return cc->evt.sta_info_ap.info->mac;
}

/**
 * \brief           Get current AP channel
 * \param[in]       cc: Event handle
 * \return          AP channel
 */
uint8_t
esp_evt_sta_info_ap_get_channel(esp_evt_t* cc) {
    return cc->evt.sta_info_ap.info->ch;
}

/**
 * \brief           Get current AP rssi
 * \param[in]       cc: Event handle
 * \return          AP rssi
 */
int16_t
esp_evt_sta_info_ap_get_rssi(esp_evt_t* cc) {
    return cc->evt.sta_info_ap.info->rssi;
}

/**
 * \brief           Get command result
 * \param[in]       cc: Event handle
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_evt_sta_info_ap_get_result(esp_evt_t* cc) {
    return cc->evt.sta_info_ap.res;
}

#endif /* ESP_CFG_MODE_STATION || __DOXYGEN__ */

#if ESP_CFG_DNS || __DOXYGEN__

/**
 * \brief           Get resolve result
 * \param[in]       cc: Event handle
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_evt_dns_hostbyname_get_result(esp_evt_t* cc) {
    return cc->evt.dns_hostbyname.res;
}

/**
 * \brief           Get hostname used to resolve IP address
 * \param[in]       cc: Event handle
 * \return          Hostname
 */
const char *
esp_evt_dns_hostbyname_get_host(esp_evt_t* cc) {
    return cc->evt.dns_hostbyname.host;
}

/**
 * \brief           Get IP address from DNS function
 * \param[in]       cc: Event handle
 * \return          IP address
 */
esp_ip_t *
esp_evt_dns_hostbyname_get_ip(esp_evt_t* cc) {
    return cc->evt.dns_hostbyname.ip;
}

#endif /* ESP_CFG_DNS || __DOXYGEN__ */

#if ESP_CFG_PING || __DOXYGEN__

/**
 * \brief           Get ping status
 * \param[in]       cc: Event handle
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_evt_ping_get_result(esp_evt_t* cc) {
    return cc->evt.ping.res;
}

/**
 * \brief           Get hostname used to ping
 * \param[in]       cc: Event handle
 * \return          Hostname
 */
const char*
esp_evt_ping_get_host(esp_evt_t* cc) {
    return cc->evt.ping.host;
}

/**
 * \brief           Get time required for ping
 * \param[in]       cc: Event handle
 * \return          Ping time
 */
uint32_t
esp_evt_ping_get_time(esp_evt_t* cc) {
    return cc->evt.ping.time;
}

#endif /* ESP_CFG_PING || __DOXYGEN__ */


/**
 * \brief           Get server command result
 * \param[in]       cc: Event handle
 * \return          Member of \ref espr_t enumeration
 */
espr_t
esp_evt_server_get_result(esp_evt_t* cc) {
    return cc->evt.server.res;
}

/**
 * \brief           Get port for server operation
 * \param[in]       cc: Event handle
 * \return          Server port
 */
esp_port_t
esp_evt_server_get_port(esp_evt_t* cc) {
    return cc->evt.server.port;
}

/**
* \brief           Check if operation was to enable or disable server
* \param[in]       cc: Event handle
* \return          `1` if enable, `0` otherwise
*/
uint8_t
esp_evt_server_is_enable(esp_evt_t* cc) {
    return cc->evt.server.en;
}
