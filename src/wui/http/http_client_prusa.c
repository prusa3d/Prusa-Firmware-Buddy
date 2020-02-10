/*
 * http_client.c
 * \brief
 *
 *  Created on: Feb 5, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#include <http_client_prusa.h>

#include "stdbool.h"

struct tcp_pcb* testpcb;
static uint32_t data = 0xdeadbeef;
extern struct netif  eth0;
static uint32_t client_interval = 0;
static bool init_tick = false;
/** http client tcp sent callback */
static err_t
tcpSendCallback(void *arg, struct tcp_pcb *pcb, u16_t len)
{
  /* nothing to do here for now */
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(len);
  return ERR_OK;
}

/** http client tcp err callback */
static void
tcpErrorHandler(void *arg, err_t err)
{
    LWIP_UNUSED_ARG(arg);
}

static err_t tcpRecvCallback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{

    if (p == NULL) {

        tcp_close(testpcb);
        return ERR_ABRT;
    } else {
        // parse here the message from CONNECT server and accept "command"
        pbuf_clen(p);
        (char *)p->payload;
    }
    tcp_close(testpcb);
    return 0;
}

uint32_t tcp_send_packet(void)
{
    char *header_plus_data = "POST /api/printer HTTP/1.0\r\nHost: 192.168.88.210\r\n\r\n{\"temperature\":{"
            "\"tool0\":{\"actual\":20, \"target\":50},"
            "\"bed\":{\"actual\":56, \"target\":89}},"
            "\"xyz_pos_mm\":{"
            "\"x\":23, \"y\":23, \"z\":23},"
            "\"print_settings\":{"
            "\"printing_speed\":100, \"flow_factor\":89, \"filament_material\":\"PLA\"} }";

    uint32_t len = strlen(header_plus_data);
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
err_t connectCallback(void *arg, struct tcp_pcb *tpcb, err_t err)
{

    tcp_send_packet();
    return 0;
}

void buddy_http_client_init() {

    ip_addr_t ip;
    IP4_ADDR(&ip, 192,168,88,220);    //IP of my PHP server
    /* create the control block */
    testpcb = tcp_new();
    tcp_arg(testpcb, &data);

    /* register callbacks with the pcb */
    tcp_err(testpcb, tcpErrorHandler);
    tcp_recv(testpcb, tcpRecvCallback);
    tcp_sent(testpcb, tcpSendCallback);
    /* now connect */
    err_t er = tcp_connect(testpcb, &ip, 9000, connectCallback);
    if( ERR_OK == er) {
        int x =23;
    }

}

void buddy_http_client_loop() {

    if(!init_tick) {
        client_interval = HAL_GetTick();
        init_tick = true;
    }

    if(netif_ip4_addr(&eth0)->addr != 0 && ((HAL_GetTick() - client_interval) > 1050)) {
        buddy_http_client_init();
        client_interval = HAL_GetTick();
    }

}
