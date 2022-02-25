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

/*!****************************************************************************
* \brief Webserver thread function
*
*****************************************************************************/
void StartWebServerTask(void const *argument);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* SRC_WUI_WUI_H_ */
