/*
 * wui_REST_api.h
 * \brief   interface functions for REST API calls from http server and clients
 *
 *  Created on: Jan 24, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 *  Modify on 09/17/2021
 *      Author: Marek Mosna <marek.mosna[at]prusa3d.cz>
 */

#ifndef _WUI_REST_API_H_
#define _WUI_REST_API_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PL_VERSION_MAJOR    2
#define PL_VERSION_MINOR    0
#define PL_VERSION_REVISION 0

#define PL_VERSTR2(x) #x
#define PL_VERSTR(x)  PL_VERSTR2(x)

#define PL_VERSION_STRING       \
    PL_VERSTR(PL_VERSION_MAJOR) \
    "." PL_VERSTR(PL_VERSION_MINOR) "." PL_VERSTR(PL_VERSION_REVISION)

void get_printer(char *data, const uint32_t buf_len);
void get_version(char *data, const uint32_t buf_len);
void get_job(char *data, const uint32_t buf_len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _WUI_REST_API_H_ */
