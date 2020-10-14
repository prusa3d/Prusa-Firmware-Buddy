/*
 * wui.h
 * \brief main interface functions for Web User Interface (WUI) thread
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
 */

#ifndef SRC_WUI_WUI_H_
#define SRC_WUI_WUI_H_

#include <marlin_vars.h>
#include "cmsis_os.h"
#include "wui_config.h"

#define BUDDY_WEB_STACK_SIZE 1024

#ifdef __cplusplus
extern "C" {
#endif

extern osMutexId wui_thread_mutex_id;

/*!****************************************************************************
* \brief Webserver thread function
*
*****************************************************************************/
void StartWebServerTask(void const *argument);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* SRC_WUI_WUI_H_ */
