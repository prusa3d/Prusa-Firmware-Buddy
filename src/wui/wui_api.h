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
#include "wui_helper_funcs.h"
#include "marlin_vars.h"

#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif

// for data exchange between wui thread and HTTP thread
extern marlin_vars_t webserver_marlin_vars;
extern osMutexId wui_web_mutex_id;

struct fs_file *wui_api_main(const char *uri);
const char *get_update_str(const char *header);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _WUI_API_H_ */
