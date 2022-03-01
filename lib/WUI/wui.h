/*
 * wui.h
 * \brief main interface functions for Web User Interface (WUI) thread
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
 */

#ifndef SRC_WUI_WUI_H_
#define SRC_WUI_WUI_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void start_network_task();

// TODO: Less colliding names? Or C++ namespace and such?
void notify_ethernet_data();
void notify_esp_data();
void notify_reconfigure();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* SRC_WUI_WUI_H_ */
