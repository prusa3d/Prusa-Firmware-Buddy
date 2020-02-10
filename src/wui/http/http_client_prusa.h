/*
 * http_client.h
 * \brief
 *
 *  Created on: Feb 5, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#ifndef HTTP_CLIENT_PRUSA_H_
#define HTTP_CLIENT_PRUSA_H_

#include "lwip/tcp.h"

#ifdef __cplusplus
extern "C" {
#endif

void buddy_http_client_init();

void buddy_http_client_loop();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* HTTP_CLIENT_PRUSA_H_ */
