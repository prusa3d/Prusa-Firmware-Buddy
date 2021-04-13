/*
 * http.c
 *
 *  Created on: Mar 30, 2021
 *      Author: joshy
 */
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "httpc.h"
#include "buddy_socket.h"
#include "cmsis_os.h"
#include "lan_interface.h"

#define RX_BUFFER_LEN 10
char rx_buffer[RX_BUFFER_LEN];
char tx_buffer[10] = "message";
static int sock_httpc = -1;
static struct sockaddr_in address_httpc;
typedef enum {
    HTTPC_STATE_INIT = 0,
    HTTPC_STATE_CONNECT,
    HTTPC_STATE_SEND,
    HTTPC_STATE_RECEIVE,
    HTTPC_STATE_PARSE,
    HTTC_STATE_CLOSE
} HTTPC_STATE;
static HTTPC_STATE httpcstate = HTTPC_STATE_INIT;
static int httpc_rec_len = 0;

void httpc_init() {
    // address settings
    address_httpc.sin_addr.s_addr = inet_addr("192.168.88.189");
    address_httpc.sin_family = AF_INET;
    address_httpc.sin_port = htons(8000);
}

void httpc_parse_recv(void) {
}

void httpc_loop() {
    switch (httpcstate) {
    case HTTPC_STATE_INIT:
        if (sock_httpc > -1) {
            httpcstate = HTTPC_STATE_CONNECT;
            break;
        }
        if ((sock_httpc = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) != -1)
            httpcstate = HTTPC_STATE_CONNECT;
        break;
    case HTTPC_STATE_CONNECT: {
        int ret = connect(sock_httpc, (struct sockaddr *)&address_httpc, sizeof(address_httpc));

        if (ERR_ISCONN == ret) // already connected
            httpcstate = HTTPC_STATE_SEND;
        else if (0 == ret) // new connection
            httpcstate = HTTPC_STATE_SEND;
        else // error occurred
            httpcstate = HTTC_STATE_CLOSE;
    } break;
    case HTTPC_STATE_SEND: {
        int ret = send(sock_httpc, tx_buffer, strlen(tx_buffer), 0);
        //todo comparison
        if (strlen(tx_buffer) == ret)
            httpcstate = HTTPC_STATE_RECEIVE;
        else if (ret < 0)
            httpcstate = HTTC_STATE_CLOSE;
    } break;
    case HTTPC_STATE_RECEIVE:
        httpc_rec_len = recv(sock_httpc, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT);
        if (httpc_rec_len) {
            // rx_buffer[httpc_rec_len] = 0; // Null-terminate whatever we received and treat like a string
            httpcstate = HTTPC_STATE_PARSE;
        }
        break;
    case HTTPC_STATE_PARSE:
        httpc_parse_recv();
        httpc_rec_len = 0;
        memset(rx_buffer, 0, RX_BUFFER_LEN);
        httpcstate = HTTPC_STATE_INIT;
        break;
    case HTTC_STATE_CLOSE:
        close(sock_httpc);
        sock_httpc = -1;
        httpcstate = HTTPC_STATE_INIT;
        break;
    default:
        break;
    }
    osDelay(1000);
}

void StarthttpcTask(void const *argument) {
    httpc_init();
    for (;;) {
        httpc_loop();
        osDelay(10);
    }
}
