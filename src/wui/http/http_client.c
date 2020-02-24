/*
 * http_client.c
 * \brief
 *
 *  Created on: Feb 5, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#include "http_client.h"
#include <stdbool.h>
#include "wui_api.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include "eeprom.h"

#define CLIENT_CONNECT_DELAY 10000 // 1 Sec.
#define IP4_ADDR_STR_SIZE    16
#define HEADER_MAX_SIZE      128
struct tcp_pcb *testpcb;
static uint32_t data = 0xdeadbeef;
extern struct netif eth0;
static uint32_t client_interval = 0;
static bool init_tick = false;
/** http client tcp sent callback */
static err_t
tcpSendCallback(void *arg, struct tcp_pcb *pcb, u16_t len) {
    /* nothing to do here for now */
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(len);
    return ERR_OK;
}

/** http client tcp err callback */
static void
tcpErrorHandler(void *arg, err_t err) {
    LWIP_UNUSED_ARG(arg);
}

static err_t tcpRecvCallback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {

    if (p == NULL) {

        tcp_close(testpcb);
        return ERR_ABRT;
    } else {

        uint16_t pbuf_count = pbuf_clen(p);
        for (int i = 0; i < pbuf_count; i++) {
            // isolate reqeust body
            uint16_t body_tok = pbuf_memfind(p, "{", 1, 0); // Easiest way to find body start (Can '{' be in a header?)
            if (body_tok != 0xFFFF) {
                // parse with jsmn & execute command
                json_parse_jsmn(((char *)p->payload) + body_tok, p->tot_len - body_tok);
            }
            // point to next pbuf
            p = p->next;
        }
    }
    tcp_close(testpcb);
    return 0;
}

uint32_t tcp_send_packet(void) {
    char host_ip4_str[IP4_ADDR_STR_SIZE], header[HEADER_MAX_SIZE], *uri = "/api/printer";
    ip4_addr_t host_ip4;
    host_ip4.addr = eeprom_get_var(EEVAR_CONNECT_IP).ui32;
    strlcpy(host_ip4_str, ip4addr_ntoa(&host_ip4), IP4_ADDR_STR_SIZE);
    snprintf(header, HEADER_MAX_SIZE, "POST %s HTTP/1.0\r\nHost: %s", uri, host_ip4_str);
    const char *header_plus_data = get_update_str(header);

    uint16_t len = strlen(header_plus_data);
    /* push to buffer */
    err_t error = tcp_write(testpcb, header_plus_data, len, TCP_WRITE_FLAG_COPY);

    if (error) {

        return 1;
    }

    /* now send */
    error = tcp_output(testpcb);
    if (error) {

        return 1;
    }
    return 0;
}

/* connection established callback, err is unused and only return 0 */
err_t connectCallback(void *arg, struct tcp_pcb *tpcb, err_t err) {

    tcp_send_packet();
    return 0;
}

void buddy_http_client_init() {

    ip_addr_t ip;
    ip.addr = eeprom_get_var(EEVAR_CONNECT_IP).ui32;
    /* create the control block */
    testpcb = tcp_new();
    tcp_arg(testpcb, &data);

    /* register callbacks with the pcb */
    tcp_err(testpcb, tcpErrorHandler);
    tcp_recv(testpcb, tcpRecvCallback);
    tcp_sent(testpcb, tcpSendCallback);
    /* now connect */
    tcp_connect(testpcb, &ip, 9000, connectCallback);
}

void buddy_http_client_loop() {

    if (!init_tick) {
        client_interval = HAL_GetTick();
        init_tick = true;
    }

    if (netif_ip4_addr(&eth0)->addr != 0 && ((HAL_GetTick() - client_interval) > CLIENT_CONNECT_DELAY)) {
        buddy_http_client_init();
        client_interval = HAL_GetTick();
    }
}
