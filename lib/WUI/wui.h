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
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Start the networking task.
//
// If allow_full is set, then we also initiate a marlin client and run the link
// server (as set by configuration). If it is false, this is skipped and only
// minimal set of networking is set up. This is used to initiate networking in
// redscreen/bluescreen - we want to send error there, but not start anything
// else.
void start_network_task(bool allow_full);

// TODO: Less colliding names? Or C++ namespace and such?
void notify_ethernet_data();
void notify_esp_data();
void notify_reconfigure();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* SRC_WUI_WUI_H_ */
