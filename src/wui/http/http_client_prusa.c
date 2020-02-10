/*
 * http_client.c
 * \brief
 *
 *  Created on: Feb 5, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#include <http_client_prusa.h>

struct tcp_pcb* testpcb;
static uint32_t data = 0xdeadbeef;
static bool httpc_inited = false;
extern struct netif  eth0;

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

    //    tcp_close(testpcb);
        return ERR_ABRT;
    } else {
         pbuf_clen(p);
         (char *)p->payload;
    }

    return 0;
}

uint32_t tcp_send_packet(void)
{
    char *string = "HEAD /process.php?data1=12&data2=5 HTTP/1.0\r\nHost: mywebsite.com\r\n\r\n ";
    uint32_t len = strlen(string);

    /* push to buffer */
    err_t error = tcp_write(testpcb, string, strlen(string), TCP_WRITE_FLAG_COPY);

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
    if(netif_ip4_addr(&eth0)->addr != 0 && !httpc_inited) {
        buddy_http_client_init();
        httpc_inited = true;
    }
}
