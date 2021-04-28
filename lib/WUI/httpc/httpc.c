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

#define RX_BUFFER_LEN 20
#define TX_BUFFER_LEN 20
char rx_buffer[RX_BUFFER_LEN];
char tx_buffer[TX_BUFFER_LEN] = "message";
static uint32_t message_count = 0;
static int sock_httpc = -1;
static struct sockaddr_in address_httpc;
typedef enum {
    HTTPC_STATE_INIT = 0,
    HTTPC_STATE_CONNECT,
    HTTPC_STATE_SEND,
    HTTPC_STATE_RECEIVE,
    HTTPC_STATE_PARSE,
    HTTPC_STATE_CLOSE
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
        if ((sock_httpc = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) != -1) {
            httpcstate = HTTPC_STATE_CONNECT;
        } else {
            httpcstate = HTTPC_STATE_CLOSE;
        }
        break;
    case HTTPC_STATE_CONNECT: {
        int ret = connect(sock_httpc, (struct sockaddr *)&address_httpc, sizeof(address_httpc));

        if (ERR_ISCONN == ret) // already connected
            httpcstate = HTTPC_STATE_SEND;
        else if (0 == ret) // new connection
            httpcstate = HTTPC_STATE_SEND;
        else // error occurred
            httpcstate = HTTPC_STATE_CLOSE;
    } break;
    case HTTPC_STATE_SEND: {
        snprintf(tx_buffer, TX_BUFFER_LEN, "%s:%ld", "message", message_count);
        int ret = send(sock_httpc, tx_buffer, strlen(tx_buffer), 0);
        if (0 > ret) {
            httpcstate = HTTPC_STATE_CLOSE;
        } else if (strlen(tx_buffer) == (unsigned int)ret) {
            httpcstate = HTTPC_STATE_RECEIVE;
        }
    } break;
    case HTTPC_STATE_RECEIVE:
        httpc_rec_len = recv(sock_httpc, rx_buffer, sizeof(rx_buffer) - 1, MSG_DONTWAIT);
        if (httpc_rec_len) {
            httpcstate = HTTPC_STATE_PARSE;
        } else {
            httpcstate = HTTPC_STATE_CLOSE;
        }
        break;
    case HTTPC_STATE_PARSE:
        message_count++;
        httpc_parse_recv();
        httpc_rec_len = 0;
        httpcstate = HTTPC_STATE_CLOSE;
        break;
    case HTTPC_STATE_CLOSE:
        close(sock_httpc);
        sock_httpc = -1;
        httpcstate = HTTPC_STATE_INIT;
        break;
    default:
        break;
    }
}

void StarthttpcTask(void const *argument) {
    httpc_init();
    for (;;) {
        httpc_loop();
        osDelay(1000);
    }
}
