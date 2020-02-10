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

void tcp_connect_to_server(const char * msg);
#endif //HTTP_CLIENT_PRUSA_H
