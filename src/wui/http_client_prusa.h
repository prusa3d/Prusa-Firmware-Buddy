#ifndef HTTP_CLIENT_PRUSA_H
#define HTTP_CLIENT_PRUSA_H

#include "cmsis_os.h"
#include "marlin_client.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/memp.h"

#define MAX_REQUEST_LEN 512
#define MAX_MARLIN_REQUEST_LEN 100
#define WUI_FLG_PEND_REQ 0x0001

extern osMessageQId web_client_queue_id;
extern osSemaphoreId web_client_sema_id;

#define ERRMSG_OK 0
#define ERRMSG_OUT_MEM 1
#define ERRMSG_TIMEOUT 2
#define ERRMSG_NOT_FOUND 3
#define ERRMSG_GEN_ERR 4

#define HCS_NOT_CONNECTED 0
#define HCS_CONNECTED 1
#define HCS_RECEIVED 2
#define HCS_CLOSING 3

typedef struct {
    uint32_t flags;
    marlin_vars_t * wui_marlin_vars;
    char request[MAX_REQUEST_LEN];
    uint16_t request_len;
} web_client_t;

void http_client_init(void);
void http_client_queue_cycle(void);

int process_server_request(void);

void send_request_to_server(void);

void tcp_http_client_connect(void);




#endif //HTTP_CLIENT_PRUSA_H
