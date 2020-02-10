/*
 * api.h
 * \brief   interface functions for REST API calls from http server and clients
 *
 *  Created on: Jan 24, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#ifndef _WUI_API_H_
#define _WUI_API_H_

#include "httpd.h"
#include "lwip/apps/fs.h"
#include "jsmn.h"

#ifdef __cplusplus
extern "C" {
#endif
#define MAX_MARLIN_REQUEST_LEN 100

struct fs_file* wui_api_main(char* uri, struct fs_file* file);

void json_parse_jsmn(const char * json, uint16_t len);
void send_request_to_server(const char * request);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _WUI_API_H_ */
