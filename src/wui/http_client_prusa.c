// http_client_prusa.c
// author: Migi

#include "http_client_prusa.h"
#include "string.h"

#define RECV_BUFFSIZE 512
#define DEST_PORT 8

web_client_t web_client;

char recv_buf[RECV_BUFFSIZE];
struct tcp_pcb * web_client_pcb;

// Private function prototypes
static err_t tcp_web_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t tcp_web_client_recv(void * arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_web_client_connection_close(struct tcp_pcb *tpcb, struct web_client_pass_t * es);
static err_t tcp_web_client_poll(void *arg, struct tcp_pcb *tpcb);
static err_t tcp_web_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void tcp_web_client_send(struct tcp_pcb *tpcb, struct web_client_pass_t * es);


void tcp_web_client_connect(void){
    ip_addr_t dest_ip;
    web_client_pcb = tcp_new();
    if(web_client_pcb != NULL){
        //netif get addrs
        //IP4_ADDR(&dest_ip, 1, 2, 3, 4);
        tcp_connect(web_client_pcb, &dest_ip, DEST_PORT, tcp_web_client_connected);
    }
}

static err_t tcp_web_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err){
    struct web_client_pass_t * wcp;

    if(err != ERR_OK){
        tcp_web_client_connection_close(tpcb, wcp);
        return err;
    }

    wcp = (struct web_client_pass_t *)mem_malloc(sizeof(web_client_pass_t));
    if(wcp != NULL){
        // CHECKPOINT
    }
}


void web_client_init(void){
    memset(&web_client, 0, sizeof(web_client_t));
}

void web_client_queue_cycle(void){
    osEvent ose;
    char ch;

    if(web_client.flags & WUI_FLG_PEND_REQ){
        if(process_server_request()){
            web_client.flags &= ~WUI_FLG_PEND_REQ;
            web_client.request_len = 0;
        }
    }

    while ((ose = osMessageGet(web_client_queue_id, 0)).status == osEventMessage) {
        ch = (char)((uint8_t)(ose.value.v));
        switch (ch) {
        case '\r':
        case '\n':
            ch = 0;
            break;
        }
        if (web_client.request_len < MAX_REQUEST_LEN)
            web_client.request[web_client.request_len++] = ch;
        else {
            //TOO LONG
            web_client.request_len = 0;
        }
        if ((ch == 0) && (web_client.request_len > 1)) {
            if (process_server_request()) {
                web_client.request_len = 0;
            } else {
                web_client.flags |= WUI_FLG_PEND_REQ;
                break;
            }
        }
    }
}

int process_server_request(void){
    if(strncmp(web_client.request, "!g", 2) == 0){
        if(web_client.request_len < 5){
            return 2;
        }
        char gcode_str[MAX_MARLIN_REQUEST_LEN];
        strncpy(gcode_str, web_client.request + 3, web_client.request_len - 3);
        marlin_gcode_printf(gcode_str);
        return 1;
    }
    return 0;
}

void send_request_to_server(){

}
