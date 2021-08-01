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

#define NETDEV_ETH_ID 0
#define NETDEV_ESP_ID 1

#ifdef __cplusplus
extern "C" {
#endif

extern osMutexId wui_thread_mutex_id;

/*!****************************************************************************
* \brief Webserver thread function
*
*****************************************************************************/
void StartWebServerTask(void const *argument);

struct altcp_pcb *prusa_alloc(void *arg, uint8_t ip_type);

uint32_t netdev_set_dhcp(uint32_t netdev_id);
uint32_t netdev_set_up(uint32_t netdev_id);
uint32_t netdev_set_down(uint32_t netdev_id);
uint32_t netdev_set_static(uint32_t netdev_id);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* SRC_WUI_WUI_H_ */
