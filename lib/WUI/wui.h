/*
 * wui.h
 * \brief main interface functions for Web User Interface (WUI) thread
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
 */

#ifndef SRC_WUI_WUI_H_
#define SRC_WUI_WUI_H_

#define BUDDY_WEB_STACK_SIZE 1024
#include <marlin_vars.h>
#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!****************************************************************************
* \brief Webserver thread function
*
*****************************************************************************/

#define HIGH_CMD_MAX_ARGS_CNT 5

void StartWebServerTask(void const *argument);

extern osMessageQId tcp_wui_queue_id;
extern osSemaphoreId tcp_wui_semaphore_id;
extern osMutexId wui_thread_mutex_id;
extern osPoolId tcp_wui_mpool_id;
extern osSemaphoreId tcp_wui_semaphore_id;

typedef enum {
    CMD_UNKNOWN,
    CMD_SEND_INFO,
} HTTPC_HIGH_LVL_CMD;

typedef enum {
    HIGH_LVL_CMD,
    LOW_LVL_CMD,
} CMD_LVL;

typedef struct {
    CMD_LVL lvl;
    HTTPC_HIGH_LVL_CMD high_lvl_cmd;
    char arg[100];
    // TODO: other possible arg's data types
} wui_cmd_t;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* SRC_WUI_WUI_H_ */
