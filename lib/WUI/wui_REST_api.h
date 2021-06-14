/*
 * wui_REST_api.h
 * \brief   interface functions for REST API calls from http server and clients
 *
 *  Created on: Jan 24, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#ifndef _WUI_REST_API_H_
#define _WUI_REST_API_H_

#include "lwip/apps/fs.h"
#include "marlin_vars.h"

#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif

// for data exchange between wui thread and HTTP thread
extern marlin_vars_t webserver_marlin_vars;
extern osMutexId wui_web_mutex_id;

void get_printer(char *data, const uint32_t buf_len);
void get_version(char *data, const uint32_t buf_len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _WUI_REST_API_H_ */
