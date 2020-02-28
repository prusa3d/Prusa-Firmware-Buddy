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
#include "lwip.h"

#define CLIENT_CONNECT_DELAY 1000 // 1 Sec.
#define CLIENT_PORT_NO       9000
#define IP4_ADDR_STR_SIZE    16
#define HEADER_MAX_SIZE      128
#define API_TOKEN_LEN        20

struct tcp_pcb *client_pcb;
static uint32_t client_interval = 0;
static bool init_tick = false;

static err_t tcpSendCallback(void *arg, struct tcp_pcb *pcb, u16_t len) {
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(len);
    tcp_close(client_pcb);
    return ERR_OK;
}

// http client tcp err callback
static void tcpErrorHandler(void *arg, err_t err) {
    LWIP_UNUSED_ARG(arg);
}

#if 0 // disabled for now!
static err_t tcpRecvCallback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {

    if (p == NULL) {

        tcp_close(client_pcb);
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
    tcp_close(client_pcb);
    return 0;
}
#endif

uint32_t tcp_send_packet(void) {

    char host_ip4_str[IP4_ADDR_STR_SIZE];
    char header[HEADER_MAX_SIZE];
    char *uri = "/p/telemetry";
    char printer_token[API_TOKEN_LEN + 1]; // extra space of end of line
    ip4_addr_t host_ip4;

    host_ip4.addr = eeprom_get_var(EEVAR_CONNECT_IP).ui32;
    strlcpy(host_ip4_str, ip4addr_ntoa(&host_ip4), IP4_ADDR_STR_SIZE);
    eeprom_get_string(EEVAR_CONNECT_KEY_START, printer_token, API_TOKEN_LEN);
    printer_token[API_TOKEN_LEN] = 0;
    snprintf(header, HEADER_MAX_SIZE, "POST %s HTTP/1.0\r\nHost: %s\nPrinter-Token: %s", uri, host_ip4_str, printer_token);
    const char *header_plus_data = get_update_str(header);

    uint32_t packet_len = strlen(header_plus_data);
    err_t error = tcp_write(client_pcb, header_plus_data, packet_len, TCP_WRITE_FLAG_COPY);

    if (error) {
        return 1;
    }
    /* now send */
    error = tcp_output(client_pcb);

    if (error) {
        return 1;
    }

    return 0;
}

// connection established callback
err_t connectCallback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    tcp_send_packet();
    return 0;
}

void buddy_http_client_init() {

    ip_addr_t ip;
    ip.addr = eeprom_get_var(EEVAR_CONNECT_IP).ui32;
    // create the control block
    client_pcb = tcp_new();
    // register callbacks with the pcb
    tcp_err(client_pcb, tcpErrorHandler);
    //tcp_recv(client_pcb, tcpRecvCallback); // temporarily disabled
    tcp_sent(client_pcb, tcpSendCallback);
    // make connection
    tcp_connect(client_pcb, &ip, CLIENT_PORT_NO, connectCallback);
}

void buddy_http_client_loop() {

    //    if (!init_tick) {
    //        client_interval = HAL_GetTick();
    //        init_tick = true;
    //    }
    //
    //    if (netif_ip4_addr(&eth0)->addr != 0 && ((HAL_GetTick() - client_interval) > CLIENT_CONNECT_DELAY)) {
    //        buddy_http_client_init();
    //        client_interval = HAL_GetTick();
    //    }

    if (!init_tick) {
        client_interval = xTaskGetTickCount();
        init_tick = true;
    }
    xTaskGetTickCount();
    if (netif_ip4_addr(&eth0)->addr != 0 && ((xTaskGetTickCount() - client_interval) > CLIENT_CONNECT_DELAY)) {
        buddy_http_client_init();
        client_interval = xTaskGetTickCount();
    }
}
