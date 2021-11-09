/**
 * \file            esp_conn.h
 * \brief           Connection API
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
#ifndef ESP_HDR_CONN_H
#define ESP_HDR_CONN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"

/**
 * \ingroup         ESP
 * \defgroup        ESP_CONN Connection API
 * \brief           Connection API functions
 * \{
 */
    
espr_t      esp_conn_start(esp_conn_p* conn, esp_conn_type_t type, const char* const remote_host, esp_port_t remote_port, void* const arg, esp_evt_fn conn_evt_fn, const uint32_t blocking);
espr_t      esp_conn_startex(esp_conn_p* conn, esp_conn_start_t* start_struct, void* const arg, esp_evt_fn conn_evt_fn, const uint32_t blocking);

espr_t      esp_conn_close(esp_conn_p conn, const uint32_t blocking);
espr_t      esp_conn_send(esp_conn_p conn, const void* data, size_t btw, size_t* const bw, const uint32_t blocking);
espr_t      esp_conn_sendto(esp_conn_p conn, const esp_ip_t* const ip, esp_port_t port, const void* data, size_t btw, size_t* bw, const uint32_t blocking);
espr_t      esp_conn_set_arg(esp_conn_p conn, void* const arg);
void *      esp_conn_get_arg(esp_conn_p conn);
uint8_t     esp_conn_is_client(esp_conn_p conn);
uint8_t     esp_conn_is_server(esp_conn_p conn);
uint8_t     esp_conn_is_active(esp_conn_p conn);
uint8_t     esp_conn_is_closed(esp_conn_p conn);
int8_t      esp_conn_getnum(esp_conn_p conn);
espr_t      esp_conn_set_ssl_buffersize(size_t size, const uint32_t blocking);
espr_t      esp_get_conns_status(const uint32_t blocking);
esp_conn_p  esp_conn_get_from_evt(esp_evt_t* evt);
espr_t      esp_conn_write(esp_conn_p conn, const void* data, size_t btw, uint8_t flush, size_t* const mem_available);
espr_t      esp_conn_recved(esp_conn_p conn, esp_pbuf_p pbuf);
size_t      esp_conn_get_total_recved_count(esp_conn_p conn);

uint8_t     esp_conn_get_remote_ip(esp_conn_p conn, esp_ip_t* ip);
esp_port_t  esp_conn_get_remote_port(esp_conn_p conn);
esp_port_t  esp_conn_get_local_port(esp_conn_p conn);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif

#endif /* ESP_HDR_CONN_H */
