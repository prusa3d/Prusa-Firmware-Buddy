/*
 * wui.h
 * \brief main interface functions for Web User Interface (WUI) thread
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
 */

#ifndef SRC_WUI_WUI_H_
#define SRC_WUI_WUI_H_

#define BUDDY_WEB_STACK_SIZE 512
#include <marlin_vars.h>
#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!****************************************************************************
* \brief Webserver thread function
*
*****************************************************************************/
void StartWebServerTask(void const *argument);

extern osMessageQId tcpclient_wui_queue;
extern osSemaphoreId tcpclient_wui_sema;

extern osMutexId wui_thread_mutex_id;
#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* SRC_WUI_WUI_H_ */
