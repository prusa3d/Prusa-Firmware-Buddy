// http_client.h
// author: Migi

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "lwip/tcp.h"

#ifdef __cplusplus
extern "C" {
#endif

void buddy_http_client_init();

void buddy_http_client_loop();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //HTTP_CLIENT_H
