/**
 * @file
 * Application layered TCP connection API (to be used from TCPIP thread)\n
 * This interface mimics the tcp callback API to the application while preventing
 * direct linking (much like virtual functions).
 * This way, an application can make use of other application layer protocols
 * on top of TCP without knowing the details (e.g. TLS, proxy connection).
 *
 * This file contains the base implementation calling into tcp.
 */

/*
 * Copyright (c) 2021 Marek Mosna
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the ESP TCP/IP stack.
 *
 * Author: Marek Mosna <marek.mosna@prusa3d.cz>
 *
 */
#ifndef ESP_HDR_ALTCP_TCP_H
#define ESP_HDR_ALTCP_TCP_H

#include "esp/esp_config.h"
#include "sockets/lwesp_sockets_priv.h"

#define EPCB_POOL_SIZE 10

#if ESP_ALTCP /* don't build if not configured for use in lwipopts.h */

    #include "lwip/altcp.h"

    #ifdef __cplusplus
extern "C" {
    #endif

struct altcp_pcb *altcp_esp_new_ip_type(u8_t ip_type);

    #define altcp_esp_new()     altcp_esp_new_ip_type(IPADDR_TYPE_V4)
    #define altcp_esp_new_ip6() altcp_esp_new_ip_type(IPADDR_TYPE_V6)

struct altcp_pcb *altcp_esp_alloc(void *arg, u8_t ip_type);

typedef struct {
    struct esp_conn *econn;   // ESP conn connection
    struct altcp_pcb *alconn; // Application layer TCP connection
    esp_port_t listen_port;
    uint16_t conn_timeout;
    size_t rcv_packets;            // Received packets
    size_t rcv_bytes;              // Received bytes
    char host[IP4ADDR_STRLEN_MAX]; // Connect host
} esp_pcb;

    #ifdef __cplusplus
}
    #endif

#endif /* ESP_ALTCP */

#endif /* ESP_HDR_ALTCP_TCP_H */
